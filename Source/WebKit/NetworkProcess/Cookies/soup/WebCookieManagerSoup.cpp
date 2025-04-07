/*
 * Copyright (C) 2011 Samsung Electronics.
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
#include "WebCookieManager.h"

#include "NetworkProcess.h"
#include "NetworkSessionSoup.h"
#include "SoupCookiePersistentStorageType.h"
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/SoupNetworkSession.h>
#include <libsoup/soup.h>

namespace WebKit {
using namespace WebCore;

void WebCookieManager::platformSetHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicy policy)
{
    SoupCookieJarAcceptPolicy soupPolicy = SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;
    switch (policy) {
    case HTTPCookieAcceptPolicy::AlwaysAccept:
        soupPolicy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
        break;
    case HTTPCookieAcceptPolicy::Never:
        soupPolicy = SOUP_COOKIE_JAR_ACCEPT_NEVER;
        break;
    case HTTPCookieAcceptPolicy::OnlyFromMainDocumentDomain:
        soupPolicy = SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;
        break;
    case HTTPCookieAcceptPolicy::ExclusivelyFromMainDocumentDomain:
        ASSERT_NOT_REACHED();
        break;
    }

    m_process.forEachNetworkStorageSession([soupPolicy] (const auto& session) {
        soup_cookie_jar_set_accept_policy(session.cookieStorage(), soupPolicy);
    });
}

HTTPCookieAcceptPolicy WebCookieManager::platformGetHTTPCookieAcceptPolicy()
{
    switch (soup_cookie_jar_get_accept_policy(m_process.defaultStorageSession().cookieStorage())) {
    case SOUP_COOKIE_JAR_ACCEPT_ALWAYS:
        return HTTPCookieAcceptPolicy::AlwaysAccept;
    case SOUP_COOKIE_JAR_ACCEPT_NEVER:
        return HTTPCookieAcceptPolicy::Never;
    case SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY:
        return HTTPCookieAcceptPolicy::OnlyFromMainDocumentDomain;
    }

    ASSERT_NOT_REACHED();
    return HTTPCookieAcceptPolicy::OnlyFromMainDocumentDomain;
}

void WebCookieManager::setCookiePersistentStorage(PAL::SessionID sessionID, const String& storagePath, SoupCookiePersistentStorageType storageType)
{
    if (auto* networkSession = m_process.networkSession(sessionID))
        static_cast<NetworkSessionSoup&>(*networkSession).setCookiePersistentStorage(storagePath, storageType);
}

} // namespace WebKit
