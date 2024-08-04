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

using namespace WebCore;

void WebPlatformStrategies::initialize()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(WebPlatformStrategies, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
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
String WebPlatformStrategies::cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    return WebCore::cookiesForDOM(session, firstParty, url);
}

void WebPlatformStrategies::setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, const String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, url, cookieString);
}

bool WebPlatformStrategies::cookiesEnabled(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    return WebCore::cookiesEnabled(session, firstParty, url);
}

String WebPlatformStrategies::cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, url);
}

bool WebPlatformStrategies::getRawCookies(const NetworkStorageSession& session, const URL& firstParty, const URL& url, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, url, rawCookies);
}

void WebPlatformStrategies::deleteCookie(const NetworkStorageSession& session, const URL& url, const String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}

RefPtr<SubresourceLoader> WebPlatformStrategies::loadResource(Frame&, CachedResource&, const ResourceRequest&, const ResourceLoaderOptions&)
{
    return nullptr;
}

void  WebPlatformStrategies::loadResourceSynchronously(NetworkingContext*, unsigned long identifier, const ResourceRequest&, StoredCredentials, ClientCredentialPolicy, ResourceError&, ResourceResponse&, Vector<char>& data)
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

void  WebPlatformStrategies::createPingHandle(NetworkingContext*, ResourceRequest&, bool shouldUseCredentialStorage, bool shouldFollowRedirects)
{
}

String WebPlatformStrategies::cookieRequestHeaderFieldValue(SessionID, const URL& firstParty, const URL&)
{
    return String();
}

void WebPlatformStrategies::addCookie(const NetworkStorageSession&, const URL&, const Cookie&)
{
}


