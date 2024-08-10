/*
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetworkStorageSession.h"

#if USE(CURL)

#include "Cookie.h"
#include "CookieJarCurlDatabase.h"
#include "CookieJarDB.h"
#include "CookieRequestHeaderFieldProxy.h"
#include "CurlContext.h"
#include "FileSystem.h"
#include "NetworkingContext.h"
#include "ResourceHandle.h"
#include "URL.h"

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static String defaultCookieJarPath()
{
    static const char* defaultFileName = "cookie.jar.db";
    char* cookieJarPath = getenv("CURL_COOKIE_JAR_PATH");
    if (cookieJarPath)
        return cookieJarPath;

    return FileSystem::pathByAppendingComponent(FileSystem::localUserSpecificStorageDirectory(), defaultFileName);
}

NetworkStorageSession::NetworkStorageSession(PAL::SessionID sessionID, NetworkingContext* context)
    : m_sessionID(sessionID)
    , m_context(context)
    , m_cookieStorage(makeUniqueRef<CookieJarCurlDatabase>())
    , m_cookieDatabase(makeUniqueRef<CookieJarDB>(defaultCookieJarPath()))
{
}

NetworkStorageSession::~NetworkStorageSession()
{
}

NetworkingContext* NetworkStorageSession::context() const
{
    return m_context.get();
}

void NetworkStorageSession::setCookieDatabase(UniqueRef<CookieJarDB>&& cookieDatabase) const
{
    m_cookieDatabase = WTFMove(cookieDatabase);
}

CookieJarDB& NetworkStorageSession::cookieDatabase() const
{
    m_cookieDatabase->open();
    return m_cookieDatabase;
}

static std::unique_ptr<NetworkStorageSession>& defaultSession()
{
    ASSERT(isMainThread());
    static NeverDestroyed<std::unique_ptr<NetworkStorageSession>> session;
    return session;
}

NetworkStorageSession& NetworkStorageSession::defaultStorageSession()
{
    if (!defaultSession())
        defaultSession() = std::make_unique<NetworkStorageSession>(PAL::SessionID::defaultSessionID(), nullptr);
    return *defaultSession();
}

void NetworkStorageSession::ensureSession(PAL::SessionID, const String&)
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::switchToNewTestingSession()
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::setCookiesFromDOM(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, const String& value) const
{
    cookieStorage().setCookiesFromDOM(*this, firstParty, sameSiteInfo, url, frameID, pageID, value);
}

bool NetworkStorageSession::cookiesEnabled() const
{
    return cookieStorage().cookiesEnabled(*this);
}

std::pair<String, bool> NetworkStorageSession::cookiesForDOM(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies includeSecureCookies) const
{
    return cookieStorage().cookiesForDOM(*this, firstParty, sameSiteInfo, url, frameID, pageID, includeSecureCookies);
}

void NetworkStorageSession::setCookies(const Vector<Cookie>&, const URL&, const URL&)
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::setCookie(const Cookie&)
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::deleteCookie(const Cookie&)
{
    // FIXME: Implement for WebKit to use.
}

void NetworkStorageSession::deleteCookie(const URL& url, const String& cookie) const
{
    cookieStorage().deleteCookie(*this, url, cookie);
}

void NetworkStorageSession::deleteAllCookies()
{
    cookieStorage().deleteAllCookies(*this);
}

void NetworkStorageSession::deleteAllCookiesModifiedSince(WallTime since)
{
    cookieStorage().deleteAllCookiesModifiedSince(*this, since);
}

void NetworkStorageSession::deleteCookiesForHostnames(const Vector<String>& cookieHostNames)
{
    cookieStorage().deleteCookiesForHostnames(*this, cookieHostNames);
}

Vector<Cookie> NetworkStorageSession::getAllCookies()
{
    // FIXME: Implement for WebKit to use.
    return { };
}

void NetworkStorageSession::getHostnamesWithCookies(HashSet<String>& hostnames)
{
    cookieStorage().getHostnamesWithCookies(*this, hostnames);
}

Vector<Cookie> NetworkStorageSession::getCookies(const URL&)
{
    // FIXME: Implement for WebKit to use.
    return { };
}

bool NetworkStorageSession::getRawCookies(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, Vector<Cookie>& rawCookies) const
{
    return cookieStorage().getRawCookies(*this, firstParty, sameSiteInfo, url, frameID, pageID, rawCookies);
}

void NetworkStorageSession::flushCookieStore()
{
    // FIXME: Implement for WebKit to use.
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies includeSecureCookies) const
{
    return cookieStorage().cookieRequestHeaderFieldValue(*this, firstParty, sameSiteInfo, url, frameID, pageID, includeSecureCookies);
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const CookieRequestHeaderFieldProxy& headerFieldProxy) const
{
    return cookieStorage().cookieRequestHeaderFieldValue(*this, headerFieldProxy.firstParty, headerFieldProxy.sameSiteInfo, headerFieldProxy.url, headerFieldProxy.frameID, headerFieldProxy.pageID, headerFieldProxy.includeSecureCookies);
}

} // namespace WebCore

#endif // USE(CURL)
