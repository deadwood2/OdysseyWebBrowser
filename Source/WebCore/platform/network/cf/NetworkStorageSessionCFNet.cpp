/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ProcessID.h>

#if PLATFORM(COCOA)
#include "PublicSuffix.h"
#include "ResourceRequest.h"
#include "WebCoreSystemInterface.h"
#else
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

// FIXME: This file is mostly Cocoa code, not CFNetwork code. This code should be moved.

namespace WebCore {

static bool cookieStoragePartitioningEnabled;

NetworkStorageSession::NetworkStorageSession(SessionID sessionID, RetainPtr<CFURLStorageSessionRef> platformSession)
    : m_sessionID(sessionID)
    , m_platformSession(platformSession)
{
}

static std::unique_ptr<NetworkStorageSession>& defaultNetworkStorageSession()
{
    ASSERT(isMainThread());
    static NeverDestroyed<std::unique_ptr<NetworkStorageSession>> session;
    return session;
}

void NetworkStorageSession::switchToNewTestingSession()
{
    // Session name should be short enough for shared memory region name to be under the limit, otehrwise sandbox rules won't work (see <rdar://problem/13642852>).
    String sessionName = String::format("WebKit Test-%u", static_cast<uint32_t>(getCurrentProcessID()));
#if PLATFORM(COCOA)
    defaultNetworkStorageSession() = std::make_unique<NetworkStorageSession>(SessionID::defaultSessionID(), adoptCF(wkCreatePrivateStorageSession(sessionName.createCFString().get())));
#else
    defaultNetworkStorageSession() = std::make_unique<NetworkStorageSession>(SessionID::defaultSessionID(), adoptCF(wkCreatePrivateStorageSession(sessionName.createCFString().get(), defaultNetworkStorageSession()->platformSession())));
#endif
}

NetworkStorageSession& NetworkStorageSession::defaultStorageSession()
{
    if (!defaultNetworkStorageSession())
        defaultNetworkStorageSession() = std::make_unique<NetworkStorageSession>(SessionID::defaultSessionID(), nullptr);
    return *defaultNetworkStorageSession();
}

void NetworkStorageSession::ensurePrivateBrowsingSession(SessionID sessionID, const String& identifierBase)
{
    if (globalSessionMap().contains(sessionID))
        return;
    RetainPtr<CFStringRef> cfIdentifier = String(identifierBase + ".PrivateBrowsing").createCFString();

#if PLATFORM(COCOA)
    auto session = std::make_unique<NetworkStorageSession>(sessionID, adoptCF(wkCreatePrivateStorageSession(cfIdentifier.get())));
#else
    auto session = std::make_unique<NetworkStorageSession>(sessionID, adoptCF(wkCreatePrivateStorageSession(cfIdentifier.get(), defaultNetworkStorageSession()->platformSession())));
#endif

    globalSessionMap().add(sessionID, WTFMove(session));
}

RetainPtr<CFHTTPCookieStorageRef> NetworkStorageSession::cookieStorage() const
{
    if (m_platformSession)
        return adoptCF(_CFURLStorageSessionCopyCookieStorage(kCFAllocatorDefault, m_platformSession.get()));

#if USE(CFURLCONNECTION)
    return _CFHTTPCookieStorageGetDefault(kCFAllocatorDefault);
#else
    // When using NSURLConnection, we also use its shared cookie storage.
    return nullptr;
#endif
}

void NetworkStorageSession::setCookieStoragePartitioningEnabled(bool enabled)
{
    cookieStoragePartitioningEnabled = enabled;
}

#if PLATFORM(COCOA)

String cookieStoragePartition(const ResourceRequest& request)
{
    return cookieStoragePartition(request.firstPartyForCookies(), request.url());
}

static inline bool hostIsInDomain(StringView host, StringView domain)
{
    if (!host.endsWithIgnoringASCIICase(domain))
        return false;

    ASSERT(host.length() >= domain.length());
    unsigned suffixOffset = host.length() - domain.length();
    return suffixOffset == 0 || host[suffixOffset - 1] == '.';
}

String cookieStoragePartition(const URL& firstPartyForCookies, const URL& resource)
{
    if (!cookieStoragePartitioningEnabled)
        return emptyString();

    String firstPartyDomain = firstPartyForCookies.host();
#if ENABLE(PUBLIC_SUFFIX_LIST)
    firstPartyDomain = topPrivatelyControlledDomain(firstPartyDomain);
#endif
    
    return hostIsInDomain(resource.host(), firstPartyDomain) ? emptyString() : firstPartyDomain;
}

#endif

}
