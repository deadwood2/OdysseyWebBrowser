/*
 * Copyright (C) 2006-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PlatformCookieJar.h"

#if USE(CFURLCONNECTION)

#include "Cookie.h"
#include "CookiesStrategy.h"
#include "NetworkStorageSession.h"
#include "NotImplemented.h"
#include "URL.h"
#include <CFNetwork/CFHTTPCookiesPriv.h>
#include <CoreFoundation/CoreFoundation.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <pal/spi/cf/CFNetworkSPI.h>
#include <windows.h>
#include <wtf/SoftLinking.h>
#include <wtf/cf/TypeCastsCF.h>
#include <wtf/text/WTFString.h>

enum {
    CFHTTPCookieStorageAcceptPolicyExclusivelyFromMainDocumentDomain = 3
};

namespace WTF {

#define DECLARE_CF_TYPE_TRAIT(ClassName) \
template <> \
struct CFTypeTrait<ClassName##Ref> { \
static inline CFTypeID typeID() { return ClassName##GetTypeID(); } \
};

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
DECLARE_CF_TYPE_TRAIT(CFHTTPCookie);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

#undef DECLARE_CF_TYPE_TRAIT
} // namespace WTF

namespace WebCore {

static const CFStringRef s_setCookieKeyCF = CFSTR("Set-Cookie");
static const CFStringRef s_cookieCF = CFSTR("Cookie");
static const CFStringRef s_createdCF = CFSTR("Created");

static inline RetainPtr<CFStringRef> cookieDomain(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyDomain(cookie));
}

static double canonicalCookieTime(double time)
{
    if (!time)
        return time;

    return (time + kCFAbsoluteTimeIntervalSince1970) * 1000;
}

static double cookieCreatedTime(CFHTTPCookieRef cookie)
{
    RetainPtr<CFDictionaryRef> props = adoptCF(CFHTTPCookieCopyProperties(cookie));
    auto value = CFDictionaryGetValue(props.get(), s_createdCF);

    auto asNumber = dynamic_cf_cast<CFNumberRef>(value);
    if (asNumber) {
        double asDouble;
        if (CFNumberGetValue(asNumber, kCFNumberFloat64Type, &asDouble))
            return canonicalCookieTime(asDouble);
        return 0.0;
    }

    auto asString = dynamic_cf_cast<CFStringRef>(value);
    if (asString)
        return canonicalCookieTime(CFStringGetDoubleValue(asString));

    return 0.0;
}

static inline CFAbsoluteTime cookieExpirationTime(CFHTTPCookieRef cookie)
{
    return canonicalCookieTime(CFHTTPCookieGetExpirationTime(cookie));
}

static inline RetainPtr<CFStringRef> cookieName(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyName(cookie));
}

static inline RetainPtr<CFStringRef> cookiePath(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyPath(cookie));
}

static inline RetainPtr<CFStringRef> cookieValue(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyValue(cookie));
}

static RetainPtr<CFArrayRef> filterCookies(CFArrayRef unfilteredCookies)
{
    ASSERT(unfilteredCookies);
    CFIndex count = CFArrayGetCount(unfilteredCookies);
    RetainPtr<CFMutableArrayRef> filteredCookies = adoptCF(CFArrayCreateMutable(0, count, &kCFTypeArrayCallBacks));
    for (CFIndex i = 0; i < count; ++i) {
        CFHTTPCookieRef cookie = (CFHTTPCookieRef)CFArrayGetValueAtIndex(unfilteredCookies, i);

        // <rdar://problem/5632883> CFHTTPCookieStorage would store an empty cookie,
        // which would be sent as "Cookie: =". We have a workaround in setCookies() to prevent
        // that, but we also need to avoid sending cookies that were previously stored, and
        // there's no harm to doing this check because such a cookie is never valid.
        if (!CFStringGetLength(cookieName(cookie).get()))
            continue;

        if (CFHTTPCookieIsHTTPOnly(cookie))
            continue;

        CFArrayAppendValue(filteredCookies.get(), cookie);
    }
    return filteredCookies;
}

static RetainPtr<CFArrayRef> copyCookiesForURLWithFirstPartyURL(const NetworkStorageSession& session, const URL& firstParty, const URL& url, IncludeSecureCookies includeSecureCookies)
{
    bool secure = includeSecureCookies == IncludeSecureCookies::Yes;

    ASSERT(!secure || (secure && url.protocolIs("https")));

    UNUSED_PARAM(firstParty);
    return adoptCF(CFHTTPCookieStorageCopyCookiesForURL(session.cookieStorage().get(), url.createCFURL().get(), secure));
}

static CFArrayRef createCookies(CFDictionaryRef headerFields, CFURLRef url)
{
    CFArrayRef parsedCookies = CFHTTPCookieCreateWithResponseHeaderFields(kCFAllocatorDefault, headerFields, url);
    if (!parsedCookies)
        parsedCookies = CFArrayCreate(kCFAllocatorDefault, 0, 0, &kCFTypeArrayCallBacks);

    return parsedCookies;
}

void setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, const String& value)
{
    UNUSED_PARAM(frameID);
    UNUSED_PARAM(pageID);
    // <rdar://problem/5632883> CFHTTPCookieStorage stores an empty cookie, which would be sent as "Cookie: =".
    if (value.isEmpty())
        return;

    RetainPtr<CFURLRef> urlCF = url.createCFURL();
    RetainPtr<CFURLRef> firstPartyForCookiesCF = firstParty.createCFURL();

    // <http://bugs.webkit.org/show_bug.cgi?id=6531>, <rdar://4409034>
    // cookiesWithResponseHeaderFields doesn't parse cookies without a value
    String cookieString = value.contains('=') ? value : value + "=";

    RetainPtr<CFStringRef> cookieStringCF = cookieString.createCFString();
    auto cookieStringCFPtr = cookieStringCF.get();
    RetainPtr<CFDictionaryRef> headerFieldsCF = adoptCF(CFDictionaryCreate(kCFAllocatorDefault,
        (const void**)&s_setCookieKeyCF, (const void**)&cookieStringCFPtr, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    RetainPtr<CFArrayRef> unfilteredCookies = adoptCF(createCookies(headerFieldsCF.get(), urlCF.get()));
    CFHTTPCookieStorageSetCookies(session.cookieStorage().get(), filterCookies(unfilteredCookies.get()).get(), urlCF.get(), firstPartyForCookiesCF.get());
}

static bool containsSecureCookies(CFArrayRef cookies)
{
    CFIndex cookieCount = CFArrayGetCount(cookies);
    while (cookieCount--) {
        if (CFHTTPCookieIsSecure(checked_cf_cast<CFHTTPCookieRef>(CFArrayGetValueAtIndex(cookies, cookieCount))))
            return true;
    }

    return false;
}

std::pair<String, bool> cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies includeSecureCookies)
{
    UNUSED_PARAM(frameID);
    UNUSED_PARAM(pageID);
    RetainPtr<CFArrayRef> cookiesCF = copyCookiesForURLWithFirstPartyURL(session, firstParty, url, includeSecureCookies);

    auto filteredCookies = filterCookies(cookiesCF.get());

    bool didAccessSecureCookies = containsSecureCookies(filteredCookies.get());

    RetainPtr<CFDictionaryRef> headerCF = adoptCF(CFHTTPCookieCopyRequestHeaderFields(kCFAllocatorDefault, filteredCookies.get()));
    String cookieString = checked_cf_cast<CFStringRef>(CFDictionaryGetValue(headerCF.get(), s_cookieCF));
    return { cookieString, didAccessSecureCookies };
}

std::pair<String, bool> cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& firstParty, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies includeSecureCookies)
{
    UNUSED_PARAM(frameID);
    UNUSED_PARAM(pageID);
    RetainPtr<CFArrayRef> cookiesCF = copyCookiesForURLWithFirstPartyURL(session, firstParty, url, includeSecureCookies);

    bool didAccessSecureCookies = containsSecureCookies(cookiesCF.get());

    RetainPtr<CFDictionaryRef> headerCF = adoptCF(CFHTTPCookieCopyRequestHeaderFields(kCFAllocatorDefault, cookiesCF.get()));
    String cookieString = checked_cf_cast<CFStringRef>(CFDictionaryGetValue(headerCF.get(), s_cookieCF));
    return { cookieString, didAccessSecureCookies };
}

bool cookiesEnabled(const NetworkStorageSession& session)
{
    CFHTTPCookieStorageAcceptPolicy policy = CFHTTPCookieStorageGetCookieAcceptPolicy(session.cookieStorage().get());
    return policy == CFHTTPCookieStorageAcceptPolicyOnlyFromMainDocumentDomain || policy == CFHTTPCookieStorageAcceptPolicyExclusivelyFromMainDocumentDomain || policy == CFHTTPCookieStorageAcceptPolicyAlways;
}

bool getRawCookies(const NetworkStorageSession& session, const URL& firstParty, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, Vector<Cookie>& rawCookies)
{
    UNUSED_PARAM(frameID);
    UNUSED_PARAM(pageID);
    rawCookies.clear();

    auto includeSecureCookies = url.protocolIs("https") ? IncludeSecureCookies::Yes : IncludeSecureCookies::No;

    RetainPtr<CFArrayRef> cookiesCF = copyCookiesForURLWithFirstPartyURL(session, firstParty, url, includeSecureCookies);

    CFIndex count = CFArrayGetCount(cookiesCF.get());
    rawCookies.reserveCapacity(count);

    for (CFIndex i = 0; i < count; i++) {
        CFHTTPCookieRef cookie = checked_cf_cast<CFHTTPCookieRef>(CFArrayGetValueAtIndex(cookiesCF.get(), i));
        String name = cookieName(cookie).get();
        String value = cookieValue(cookie).get();
        String domain = cookieDomain(cookie).get();
        String path = cookiePath(cookie).get();

        double created = cookieCreatedTime(cookie);
        double expires = cookieExpirationTime(cookie);

        bool httpOnly = CFHTTPCookieIsHTTPOnly(cookie);
        bool secure = CFHTTPCookieIsSecure(cookie);
        bool session = false; // FIXME: Need API for if a cookie is a session cookie.

        String comment;
        URL commentURL;
        Vector<uint16_t> ports;

        rawCookies.uncheckedAppend(Cookie(name, value, domain, path, created, expires, httpOnly, secure, session, comment, commentURL, ports));
    }

    return true;
}

void deleteCookie(const NetworkStorageSession& session, const URL& url, const String& name)
{
    RetainPtr<CFHTTPCookieStorageRef> cookieStorage = session.cookieStorage();

    RetainPtr<CFURLRef> urlCF = url.createCFURL();

    bool sendSecureCookies = url.protocolIs("https");
    RetainPtr<CFArrayRef> cookiesCF = adoptCF(CFHTTPCookieStorageCopyCookiesForURL(cookieStorage.get(), urlCF.get(), sendSecureCookies));

    CFIndex count = CFArrayGetCount(cookiesCF.get());
    for (CFIndex i = 0; i < count; i++) {
        CFHTTPCookieRef cookie = checked_cf_cast<CFHTTPCookieRef>(CFArrayGetValueAtIndex(cookiesCF.get(), i));
        if (String(cookieName(cookie).get()) == name) {
            CFHTTPCookieStorageDeleteCookie(cookieStorage.get(), cookie);
            break;
        }
    }
}

void getHostnamesWithCookies(const NetworkStorageSession& session, HashSet<String>& hostnames)
{
    RetainPtr<CFArrayRef> cookiesCF = adoptCF(CFHTTPCookieStorageCopyCookies(session.cookieStorage().get()));
    if (!cookiesCF)
        return;

    CFIndex count = CFArrayGetCount(cookiesCF.get());
    for (CFIndex i = 0; i < count; ++i) {
        CFHTTPCookieRef cookie = checked_cf_cast<CFHTTPCookieRef>(CFArrayGetValueAtIndex(cookiesCF.get(), i));
        RetainPtr<CFStringRef> domain = cookieDomain(cookie);
        hostnames.add(domain.get());
    }
}

void deleteAllCookies(const NetworkStorageSession& session)
{
    CFHTTPCookieStorageDeleteAllCookies(session.cookieStorage().get());
}

void deleteCookiesForHostnames(const NetworkStorageSession& session, const Vector<String>& hostnames)
{
}

void deleteAllCookiesModifiedSince(const NetworkStorageSession&, WallTime)
{
}

} // namespace WebCore

#endif // USE(CFURLCONNECTION)
