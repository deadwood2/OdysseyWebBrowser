/*
 * Copyright (C) 2014 Igalia S.L.
 * Copyright (C) 2016-2018 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "WebUserMediaClient.h"

#if ENABLE(MEDIA_STREAM)

#include <WebCore/UserMediaController.h>
#include <WebCore/UserMediaRequest.h>
#include "WebPage.h"

namespace WebKit {
using namespace WebCore;

WebUserMediaClient::WebUserMediaClient(WebPage& page)
    : m_page(page)
{
}

void WebUserMediaClient::pageDestroyed()
{
    delete this;
}

void WebUserMediaClient::requestUserMediaAccess(UserMediaRequest& request)
{
// dprintf("%s\n", __PRETTY_FUNCTION__);
}

void WebUserMediaClient::cancelUserMediaAccessRequest(UserMediaRequest& request)
{
// dprintf("%s\n", __PRETTY_FUNCTION__);
}

void WebUserMediaClient::enumerateMediaDevices(Document& document, CompletionHandler<void(const Vector<CaptureDevice>&, const String&)>&& completionHandler)
{
// dprintf("%s\n", __PRETTY_FUNCTION__);
}

WebUserMediaClient::DeviceChangeObserverToken WebUserMediaClient::addDeviceChangeObserver(WTF::Function<void()>&& observer)
{
    return { };
}

void WebUserMediaClient::removeDeviceChangeObserver(DeviceChangeObserverToken token)
{
}

} // namespace WebKit;

#endif // MEDIA_STREAM
