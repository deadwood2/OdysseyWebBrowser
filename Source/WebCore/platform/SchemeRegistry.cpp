/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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
 *
 */
#include "config.h"
#include "SchemeRegistry.h"
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>

#if USE(QUICK_LOOK)
#include "QuickLook.h"
#endif

namespace WebCore {

static URLSchemesMap& localURLSchemes()
{
    static NeverDestroyed<URLSchemesMap> localSchemes;

    if (localSchemes.get().isEmpty()) {
        localSchemes.get().add("file");
#if PLATFORM(COCOA)
        localSchemes.get().add("applewebdata");
#endif
    }

    return localSchemes;
}

static URLSchemesMap& displayIsolatedURLSchemes()
{
    static NeverDestroyed<URLSchemesMap> displayIsolatedSchemes;
    return displayIsolatedSchemes;
}

static URLSchemesMap& secureSchemes()
{
    static NeverDestroyed<URLSchemesMap> secureSchemes;

    if (secureSchemes.get().isEmpty()) {
        secureSchemes.get().add("https");
        secureSchemes.get().add("about");
        secureSchemes.get().add("data");
        secureSchemes.get().add("wss");
#if USE(QUICK_LOOK)
        secureSchemes.get().add(QLPreviewProtocol());
#endif
#if PLATFORM(GTK)
        secureSchemes.get().add("resource");
#endif
    }

    return secureSchemes;
}

static URLSchemesMap& schemesWithUniqueOrigins()
{
    static NeverDestroyed<URLSchemesMap> schemesWithUniqueOrigins;

    if (schemesWithUniqueOrigins.get().isEmpty()) {
        schemesWithUniqueOrigins.get().add("about");
        schemesWithUniqueOrigins.get().add("javascript");
        // This is a willful violation of HTML5.
        // See https://bugs.webkit.org/show_bug.cgi?id=11885
        schemesWithUniqueOrigins.get().add("data");
    }

    return schemesWithUniqueOrigins;
}

static URLSchemesMap& emptyDocumentSchemes()
{
    static NeverDestroyed<URLSchemesMap> emptyDocumentSchemes;

    if (emptyDocumentSchemes.get().isEmpty())
        emptyDocumentSchemes.get().add("about");

    return emptyDocumentSchemes;
}

static HashSet<String, ASCIICaseInsensitiveHash>& schemesForbiddenFromDomainRelaxation()
{
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> schemes;
    return schemes;
}

static URLSchemesMap& canDisplayOnlyIfCanRequestSchemes()
{
    static NeverDestroyed<URLSchemesMap> canDisplayOnlyIfCanRequestSchemes;

    if (canDisplayOnlyIfCanRequestSchemes.get().isEmpty()) {
        canDisplayOnlyIfCanRequestSchemes.get().add("blob");
    }

    return canDisplayOnlyIfCanRequestSchemes;
}

static URLSchemesMap& notAllowingJavascriptURLsSchemes()
{
    static NeverDestroyed<URLSchemesMap> notAllowingJavascriptURLsSchemes;
    return notAllowingJavascriptURLsSchemes;
}

void SchemeRegistry::registerURLSchemeAsLocal(const String& scheme)
{
    localURLSchemes().add(scheme);
}

void SchemeRegistry::removeURLSchemeRegisteredAsLocal(const String& scheme)
{
    if (equalLettersIgnoringASCIICase(scheme, "file"))
        return;
#if PLATFORM(COCOA)
    if (equalLettersIgnoringASCIICase(scheme, "applewebdata"))
        return;
#endif
    localURLSchemes().remove(scheme);
}

const URLSchemesMap& SchemeRegistry::localSchemes()
{
    return localURLSchemes();
}

static URLSchemesMap& schemesAllowingLocalStorageAccessInPrivateBrowsing()
{
    static NeverDestroyed<URLSchemesMap> schemesAllowingLocalStorageAccessInPrivateBrowsing;
    return schemesAllowingLocalStorageAccessInPrivateBrowsing;
}

static URLSchemesMap& schemesAllowingDatabaseAccessInPrivateBrowsing()
{
    static NeverDestroyed<URLSchemesMap> schemesAllowingDatabaseAccessInPrivateBrowsing;
    return schemesAllowingDatabaseAccessInPrivateBrowsing;
}

static URLSchemesMap& CORSEnabledSchemes()
{
    // FIXME: http://bugs.webkit.org/show_bug.cgi?id=77160
    static NeverDestroyed<URLSchemesMap> CORSEnabledSchemes;

    if (CORSEnabledSchemes.get().isEmpty()) {
        CORSEnabledSchemes.get().add("http");
        CORSEnabledSchemes.get().add("https");
    }

    return CORSEnabledSchemes;
}

static URLSchemesMap& ContentSecurityPolicyBypassingSchemes()
{
    static NeverDestroyed<URLSchemesMap> schemes;
    return schemes;
}

#if ENABLE(CACHE_PARTITIONING)
static URLSchemesMap& cachePartitioningSchemes()
{
    static NeverDestroyed<URLSchemesMap> schemes;
    return schemes;
}
#endif

static URLSchemesMap& alwaysRevalidatedSchemes()
{
    static NeverDestroyed<URLSchemesMap> schemes;
    return schemes;
}

bool SchemeRegistry::shouldTreatURLSchemeAsLocal(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return localURLSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsNoAccess(const String& scheme)
{
    schemesWithUniqueOrigins().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsNoAccess(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesWithUniqueOrigins().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsDisplayIsolated(const String& scheme)
{
    displayIsolatedURLSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsDisplayIsolated(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return displayIsolatedURLSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsSecure(const String& scheme)
{
    secureSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsSecure(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return secureSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsEmptyDocument(const String& scheme)
{
    emptyDocumentSchemes().add(scheme);
}

bool SchemeRegistry::shouldLoadURLSchemeAsEmptyDocument(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return emptyDocumentSchemes().contains(scheme);
}

void SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const String& scheme)
{
    if (scheme.isEmpty())
        return;

    if (forbidden)
        schemesForbiddenFromDomainRelaxation().add(scheme);
    else
        schemesForbiddenFromDomainRelaxation().remove(scheme);
}

bool SchemeRegistry::isDomainRelaxationForbiddenForURLScheme(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesForbiddenFromDomainRelaxation().contains(scheme);
}

bool SchemeRegistry::canDisplayOnlyIfCanRequest(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return canDisplayOnlyIfCanRequestSchemes().contains(scheme);
}

void SchemeRegistry::registerAsCanDisplayOnlyIfCanRequest(const String& scheme)
{
    canDisplayOnlyIfCanRequestSchemes().add(scheme);
}

void SchemeRegistry::registerURLSchemeAsNotAllowingJavascriptURLs(const String& scheme)
{
    notAllowingJavascriptURLsSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsNotAllowingJavascriptURLs(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return notAllowingJavascriptURLsSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(const String& scheme)
{
    schemesAllowingLocalStorageAccessInPrivateBrowsing().add(scheme);
}

bool SchemeRegistry::allowsLocalStorageAccessInPrivateBrowsing(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesAllowingLocalStorageAccessInPrivateBrowsing().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(const String& scheme)
{
    schemesAllowingDatabaseAccessInPrivateBrowsing().add(scheme);
}

bool SchemeRegistry::allowsDatabaseAccessInPrivateBrowsing(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return schemesAllowingDatabaseAccessInPrivateBrowsing().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsCORSEnabled(const String& scheme)
{
    CORSEnabledSchemes().add(scheme);
}

bool SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return CORSEnabledSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsBypassingContentSecurityPolicy(const String& scheme)
{
    ContentSecurityPolicyBypassingSchemes().add(scheme);
}

void SchemeRegistry::removeURLSchemeRegisteredAsBypassingContentSecurityPolicy(const String& scheme)
{
    ContentSecurityPolicyBypassingSchemes().remove(scheme);
}

bool SchemeRegistry::schemeShouldBypassContentSecurityPolicy(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return ContentSecurityPolicyBypassingSchemes().contains(scheme);
}

void SchemeRegistry::registerURLSchemeAsAlwaysRevalidated(const String& scheme)
{
    alwaysRevalidatedSchemes().add(scheme);
}

bool SchemeRegistry::shouldAlwaysRevalidateURLScheme(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return alwaysRevalidatedSchemes().contains(scheme);
}

#if ENABLE(CACHE_PARTITIONING)
void SchemeRegistry::registerURLSchemeAsCachePartitioned(const String& scheme)
{
    cachePartitioningSchemes().add(scheme);
}

bool SchemeRegistry::shouldPartitionCacheForURLScheme(const String& scheme)
{
    if (scheme.isEmpty())
        return false;
    return cachePartitioningSchemes().contains(scheme);
}
#endif

} // namespace WebCore
