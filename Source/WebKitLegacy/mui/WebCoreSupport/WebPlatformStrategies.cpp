/*
    Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
    Copyright (C) 2008 Trolltech ASA
    Copyright (C) 2008 Collabora Ltd. All rights reserved.
    Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2011 Samsung Electronics
    Copyright (C) 2012 Intel Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#include "WebPlatformStrategies.h"

#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformCookieJar.h"
#include "PluginDatabase.h"
#include "PluginPackage.h"
#include "SubresourceLoader.h"

#include <wtf/NeverDestroyed.h>

using namespace WebCore;

void WebPlatformStrategies::initialize()
{
    static NeverDestroyed<WebPlatformStrategies> platformStrategies;

    setPlatformStrategies(&platformStrategies.get());
}

WebPlatformStrategies::WebPlatformStrategies()
{
}

CookiesStrategy* WebPlatformStrategies::createCookiesStrategy()
{
    return this;
}

LoaderStrategy* WebPlatformStrategies::createLoaderStrategy()
{
    return this;
}

PasteboardStrategy* WebPlatformStrategies::createPasteboardStrategy()
{
    notImplemented();
    return 0;
}

BlobRegistry* WebPlatformStrategies::createBlobRegistry()
{
    return nullptr;
}

// CookiesStrategy
std::pair<String, bool> WebPlatformStrategies::cookiesForDOM(const WebCore::NetworkStorageSession& session, const WebCore::URL& firstParty, const WebCore::URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, WebCore::IncludeSecureCookies includeSecureCookies)
{
    return WebCore::cookiesForDOM(session, firstParty, url, frameID, pageID, includeSecureCookies);
}

void WebPlatformStrategies::setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, const String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, url, frameID, pageID, cookieString);
}

bool WebPlatformStrategies::cookiesEnabled(const NetworkStorageSession& session)
{
    return WebCore::cookiesEnabled(session);
}

std::pair<String, bool> WebPlatformStrategies::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& firstParty, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, WebCore::IncludeSecureCookies includeSecureCookies)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url, frameID, pageID, includeSecureCookies);
}

bool WebPlatformStrategies::getRawCookies(const NetworkStorageSession& session, const URL& firstParty, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, url, frameID, pageID, rawCookies);
}

void WebPlatformStrategies::deleteCookie(const NetworkStorageSession& session, const URL& url, const String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}

void WebPlatformStrategies::loadResource(WebCore::Frame&, WebCore::CachedResource&, WebCore::ResourceRequest&&, const WebCore::ResourceLoaderOptions&, CompletionHandler<void(RefPtr<WebCore::SubresourceLoader>&&)>&&)
{
}

void  WebPlatformStrategies::loadResourceSynchronously(WebCore::NetworkingContext*, unsigned long identifier, const WebCore::ResourceRequest&, WebCore::StoredCredentialsPolicy, WebCore::ClientCredentialPolicy, WebCore::ResourceError&, WebCore::ResourceResponse&, Vector<char>& data)
{
}

void  WebPlatformStrategies::remove(ResourceLoader*)
{
}
void  WebPlatformStrategies::setDefersLoading(ResourceLoader*, bool)
{
}
void  WebPlatformStrategies::crossOriginRedirectReceived(ResourceLoader*, const URL& redirectURL)
{
}

void  WebPlatformStrategies::servePendingRequests(ResourceLoadPriority minimumPriority)
{
}
void  WebPlatformStrategies::suspendPendingRequests()
{
}
void  WebPlatformStrategies::resumePendingRequests()
{
}

std::pair<String, bool>  WebPlatformStrategies::cookieRequestHeaderFieldValue(PAL::SessionID, const URL& firstParty, const URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies)
{
    return std::pair<String, bool>(String(), false);
}

void WebPlatformStrategies::addCookie(const NetworkStorageSession&, const URL&, const Cookie&)
{
}

void WebPlatformStrategies::storeDerivedDataToCache(const SHA1::Digest& bodyKey, const String& type, const String& partition, WebCore::SharedBuffer&)
{
}

void WebPlatformStrategies::setCaptureExtraNetworkLoadMetricsEnabled(bool)
{
}

void WebPlatformStrategies::preconnectTo(WebCore::NetworkingContext&, const WebCore::URL&, WebCore::StoredCredentialsPolicy, PreconnectCompletionHandler&&)
{
}

void WebPlatformStrategies::startPingLoad(WebCore::Frame&, WebCore::ResourceRequest&, const WebCore::HTTPHeaderMap& originalRequestHeaders, const WebCore::FetchOptions&, PingLoadCompletionHandler&&)
{
}

