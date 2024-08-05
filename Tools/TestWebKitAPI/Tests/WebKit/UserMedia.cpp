/*
 * Copyright (C) 2014 Igalia S.L
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#if WK_HAVE_C_SPI

#if ENABLE(MEDIA_STREAM)
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "Test.h"
#include <WebKit/WKPagePrivate.h>
#include <WebKit/WKPreferencesRef.h>
#include <WebKit/WKPreferencesRefPrivate.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKUserMediaPermissionCheck.h>
#include <string.h>
#include <vector>

namespace TestWebKitAPI {

static bool wasPrompted;

static void decidePolicyForUserMediaPermissionRequestCallBack(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKSecurityOriginRef, WKUserMediaPermissionRequestRef permissionRequest, const void* /* clientInfo */)
{
    WKRetainPtr<WKArrayRef> audioDeviceUIDs = WKUserMediaPermissionRequestAudioDeviceUIDs(permissionRequest);
    WKRetainPtr<WKArrayRef> videoDeviceUIDs = WKUserMediaPermissionRequestVideoDeviceUIDs(permissionRequest);

    if (WKArrayGetSize(videoDeviceUIDs.get()) || WKArrayGetSize(audioDeviceUIDs.get())) {
        WKRetainPtr<WKStringRef> videoDeviceUID;
        if (WKArrayGetSize(videoDeviceUIDs.get()))
            videoDeviceUID = reinterpret_cast<WKStringRef>(WKArrayGetItemAtIndex(videoDeviceUIDs.get(), 0));
        else
            videoDeviceUID = WKStringCreateWithUTF8CString("");

        WKRetainPtr<WKStringRef> audioDeviceUID;
        if (WKArrayGetSize(audioDeviceUIDs.get()))
            audioDeviceUID = reinterpret_cast<WKStringRef>(WKArrayGetItemAtIndex(audioDeviceUIDs.get(), 0));
        else
            audioDeviceUID = WKStringCreateWithUTF8CString("");

        WKUserMediaPermissionRequestAllow(permissionRequest, audioDeviceUID.get(), videoDeviceUID.get());
    }

    wasPrompted = true;
}

static void checkUserMediaPermissionCallback(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKSecurityOriginRef, WKUserMediaPermissionCheckRef permissionRequest, const void*)
{
    WKUserMediaPermissionCheckSetUserMediaAccessInfo(permissionRequest, WKStringCreateWithUTF8CString("0x123456789"), false);
}

TEST(WebKit2, UserMediaBasic)
{
    auto context = adoptWK(WKContextCreate());

    WKRetainPtr<WKPageGroupRef> pageGroup(AdoptWK, WKPageGroupCreateWithIdentifier(Util::toWK("GetUserMedia").get()));
    WKPreferencesRef preferences = WKPageGroupGetPreferences(pageGroup.get());
    WKPreferencesSetMediaDevicesEnabled(preferences, true);
    WKPreferencesSetFileAccessFromFileURLsAllowed(preferences, true);
    WKPreferencesSetMediaCaptureRequiresSecureConnection(preferences, false);
    WKPreferencesSetMockCaptureDevicesEnabled(preferences, true);

    WKPageUIClientV6 uiClient;
    memset(&uiClient, 0, sizeof(uiClient));
    uiClient.base.version = 6;
    uiClient.decidePolicyForUserMediaPermissionRequest = decidePolicyForUserMediaPermissionRequestCallBack;
    uiClient.checkUserMediaPermissionForOrigin = checkUserMediaPermissionCallback;
    
    PlatformWebView webView(context.get(), pageGroup.get());
    WKPageSetPageUIClient(webView.page(), &uiClient.base);

    wasPrompted = false;
    auto url = adoptWK(Util::createURLForResource("getUserMedia", "html"));
    ASSERT(url.get());

    WKPageLoadURL(webView.page(), url.get());

    Util::run(&wasPrompted);
}

} // namespace TestWebKitAPI

#endif // ENABLE(MEDIA_STREAM)

#endif // WK_HAVE_C_SPI

