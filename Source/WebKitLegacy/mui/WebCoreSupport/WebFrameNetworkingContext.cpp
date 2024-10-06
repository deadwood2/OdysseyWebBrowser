/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#include "WebFrameNetworkingContext.h"

#include "FrameLoaderClient.h"
#include "NetworkStorageSessionMap.h"
#include "Page.h"
#include <WebCore/FrameLoader.h>

#include <wtf/NeverDestroyed.h>

using namespace WebCore;

NetworkStorageSession* WebFrameNetworkingContext::storageSession() const
{
    if (frame() && frame()->page()->usesEphemeralSession())
        return NetworkStorageSessionMap::storageSession(PAL::SessionID::legacyPrivateSessionID());

    return &NetworkStorageSessionMap::defaultStorageSession();
}

String WebFrameNetworkingContext::userAgent() const
{
    return m_userAgent;
}

String WebFrameNetworkingContext::referrer() const
{
    return frame()->loader().referrer();
}

static String& privateBrowsingStorageSessionIdentifierBase() 
{
    ASSERT(isMainThread()); 
    static NeverDestroyed<String> base;
    return base; 
}

void WebFrameNetworkingContext::setPrivateBrowsingStorageSessionIdentifierBase(const String& identifier) 
{
    privateBrowsingStorageSessionIdentifierBase() = identifier;
}

WebCore::NetworkStorageSession& WebFrameNetworkingContext::ensurePrivateBrowsingSession()
{
    NetworkStorageSessionMap::ensureSession(PAL::SessionID::legacyPrivateSessionID());

    return *NetworkStorageSessionMap::storageSession(PAL::SessionID::legacyPrivateSessionID());
}

void WebFrameNetworkingContext::destroyPrivateBrowsingSession()
{

}
