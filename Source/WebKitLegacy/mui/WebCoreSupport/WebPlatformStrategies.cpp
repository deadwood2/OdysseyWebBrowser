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
#include "WebFrameNetworkingContext.h"

#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "PluginDatabase.h"
#include "PluginPackage.h"
#include "SubresourceLoader.h"
#include <WebCore/NetworkStorageSession.h>

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

void WebPlatformStrategies::loadResource(WebCore::Frame&, WebCore::CachedResource&, WebCore::ResourceRequest&&, const WebCore::ResourceLoaderOptions&, CompletionHandler<void(RefPtr<WebCore::SubresourceLoader>&&)>&&)
{
}

void  WebPlatformStrategies::loadResourceSynchronously(WebCore::FrameLoader&, unsigned long identifier, const WebCore::ResourceRequest&, WebCore::ClientCredentialPolicy, const WebCore::FetchOptions&, const WebCore::HTTPHeaderMap&, WebCore::ResourceError&, WebCore::ResourceResponse&, Vector<char>& data)
{
}

void  WebPlatformStrategies::remove(ResourceLoader*)
{
}
void WebPlatformStrategies::setDefersLoading(WebCore::ResourceLoader&, bool)
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


void WebPlatformStrategies::setCaptureExtraNetworkLoadMetricsEnabled(bool)
{
}

void WebPlatformStrategies::preconnectTo(WebCore::FrameLoader&, const URL&, WebCore::StoredCredentialsPolicy, PreconnectCompletionHandler&&)
{
}

void WebPlatformStrategies::startPingLoad(WebCore::Frame&, WebCore::ResourceRequest&, const WebCore::HTTPHeaderMap& originalRequestHeaders, const WebCore::FetchOptions&, WebCore::ContentSecurityPolicyImposition, PingLoadCompletionHandler&&)
{
}

void WebPlatformStrategies::pageLoadCompleted(uint64_t webPageID)
{
}
bool WebPlatformStrategies::isOnLine() const
{
    return true;
}
void WebPlatformStrategies::addOnlineStateChangeListener(Function<void(bool)>&&)
{
}

