/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#pragma once

#include "WKPage.h"
#include "WebEvent.h"
#include "WebHitTestResultData.h"
#include "WebPageProxy.h"
#include <WebCore/FloatRect.h>
#include <wtf/CompletionHandler.h>

#if PLATFORM(IOS)
OBJC_CLASS NSArray;
OBJC_CLASS _WKActivatedElementInfo;
OBJC_CLASS UIViewController;
#endif

namespace WebCore {
class ResourceRequest;
struct SecurityOriginData;
struct WindowFeatures;
}

namespace WebKit {
class NativeWebKeyboardEvent;
class NativeWebWheelEvent;
class NotificationPermissionRequest;
class UserMediaPermissionRequestProxy;
class WebColorPickerResultListenerProxy;
class WebFrameProxy;
class WebOpenPanelResultListenerProxy;
class WebPageProxy;
struct NavigationActionData;

#if ENABLE(MEDIA_SESSION)
class WebMediaSessionMetadata;
#endif
}

namespace API {

class Data;
class Dictionary;
class Object;
class OpenPanelParameters;
class SecurityOrigin;

class UIClient {
public:
    virtual ~UIClient() { }

    virtual void createNewPage(WebKit::WebPageProxy&, Ref<FrameInfo>&&, WebCore::ResourceRequest&&, WebCore::WindowFeatures&&, WebKit::NavigationActionData&&, CompletionHandler<void(RefPtr<WebKit::WebPageProxy>&&)>&& completionHandler) { completionHandler(nullptr); }
    virtual void showPage(WebKit::WebPageProxy*) { }
    virtual void fullscreenMayReturnToInline(WebKit::WebPageProxy*) { }
    virtual void didEnterFullscreen(WebKit::WebPageProxy*) { }
    virtual void didExitFullscreen(WebKit::WebPageProxy*) { }
    virtual void hasVideoInPictureInPictureDidChange(WebKit::WebPageProxy*, bool) { }
    virtual void close(WebKit::WebPageProxy*) { }

    virtual void takeFocus(WebKit::WebPageProxy*, WKFocusDirection) { }
    virtual void focus(WebKit::WebPageProxy*) { }
    virtual void unfocus(WebKit::WebPageProxy*) { }

    virtual void runJavaScriptAlert(WebKit::WebPageProxy*, const WTF::String&, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, Function<void ()>&& completionHandler) { completionHandler(); }
    virtual void runJavaScriptConfirm(WebKit::WebPageProxy*, const WTF::String&, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, Function<void (bool)>&& completionHandler) { completionHandler(false); }
    virtual void runJavaScriptPrompt(WebKit::WebPageProxy*, const WTF::String&, const WTF::String&, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, Function<void (const WTF::String&)>&& completionHandler) { completionHandler(WTF::String()); }

    virtual void setStatusText(WebKit::WebPageProxy*, const WTF::String&) { }
    virtual void mouseDidMoveOverElement(WebKit::WebPageProxy&, const WebKit::WebHitTestResultData&, WebKit::WebEvent::Modifiers, Object*) { }
#if ENABLE(NETSCAPE_PLUGIN_API)
    virtual void unavailablePluginButtonClicked(WebKit::WebPageProxy&, WKPluginUnavailabilityReason, Dictionary&) { }
#endif // ENABLE(NETSCAPE_PLUGIN_API)

    virtual void didNotHandleKeyEvent(WebKit::WebPageProxy*, const WebKit::NativeWebKeyboardEvent&) { }
    virtual void didNotHandleWheelEvent(WebKit::WebPageProxy*, const WebKit::NativeWebWheelEvent&) { }

    virtual void toolbarsAreVisible(WebKit::WebPageProxy&, Function<void(bool)>&& completionHandler) { completionHandler(true); }
    virtual void setToolbarsAreVisible(WebKit::WebPageProxy&, bool) { }
    virtual void menuBarIsVisible(WebKit::WebPageProxy&, Function<void(bool)>&& completionHandler) { completionHandler(true); }
    virtual void setMenuBarIsVisible(WebKit::WebPageProxy&, bool) { }
    virtual void statusBarIsVisible(WebKit::WebPageProxy&, Function<void(bool)>&& completionHandler) { completionHandler(true); }
    virtual void setStatusBarIsVisible(WebKit::WebPageProxy&, bool) { }
    virtual void setIsResizable(WebKit::WebPageProxy&, bool) { }

    virtual void setWindowFrame(WebKit::WebPageProxy&, const WebCore::FloatRect&) { }
    virtual void windowFrame(WebKit::WebPageProxy&, Function<void(WebCore::FloatRect)>&& completionHandler) { completionHandler({ }); }

    virtual bool canRunBeforeUnloadConfirmPanel() const { return false; }
    virtual void runBeforeUnloadConfirmPanel(WebKit::WebPageProxy*, const WTF::String&, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, Function<void (bool)>&& completionHandler) { completionHandler(true); }

    virtual void pageDidScroll(WebKit::WebPageProxy*) { }

    virtual void exceededDatabaseQuota(WebKit::WebPageProxy*, WebKit::WebFrameProxy*, SecurityOrigin*, const WTF::String&, const WTF::String&, unsigned long long currentQuota, unsigned long long, unsigned long long, unsigned long long, Function<void (unsigned long long)>&& completionHandler)
    {
        completionHandler(currentQuota);
    }

    virtual void reachedApplicationCacheOriginQuota(WebKit::WebPageProxy*, const WebCore::SecurityOrigin&, uint64_t currentQuota, uint64_t, Function<void (unsigned long long)>&& completionHandler)
    {
        completionHandler(currentQuota);
    }

    virtual bool runOpenPanel(WebKit::WebPageProxy*, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, OpenPanelParameters*, WebKit::WebOpenPanelResultListenerProxy*) { return false; }
    virtual void decidePolicyForGeolocationPermissionRequest(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, SecurityOrigin&, Function<void(bool)>&) { }
    virtual bool decidePolicyForUserMediaPermissionRequest(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, SecurityOrigin&, SecurityOrigin&, WebKit::UserMediaPermissionRequestProxy&) { return false; }
    virtual bool checkUserMediaPermissionForOrigin(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, SecurityOrigin&, SecurityOrigin&, WebKit::UserMediaPermissionCheckProxy&) { return false; }
    virtual void decidePolicyForNotificationPermissionRequest(WebKit::WebPageProxy&, SecurityOrigin&, Function<void(bool)>&& completionHandler) { completionHandler(false); }

    // Printing.
    virtual float headerHeight(WebKit::WebPageProxy&, WebKit::WebFrameProxy&) { return 0; }
    virtual float footerHeight(WebKit::WebPageProxy&, WebKit::WebFrameProxy&) { return 0; }
    virtual void drawHeader(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, WebCore::FloatRect&&) { }
    virtual void drawFooter(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, WebCore::FloatRect&&) { }
    virtual void printFrame(WebKit::WebPageProxy&, WebKit::WebFrameProxy&) { }

    virtual bool canRunModal() const { return false; }
    virtual void runModal(WebKit::WebPageProxy&) { }

    virtual void saveDataToFileInDownloadsFolder(WebKit::WebPageProxy*, const WTF::String&, const WTF::String&, const WebCore::URL&, Data&) { }

    virtual void pinnedStateDidChange(WebKit::WebPageProxy&) { }

    virtual void isPlayingMediaDidChange(WebKit::WebPageProxy&) { }
    virtual void mediaCaptureStateDidChange(WebCore::MediaProducer::MediaStateFlags) { }
    virtual void handleAutoplayEvent(WebKit::WebPageProxy&, WebCore::AutoplayEvent, OptionSet<WebCore::AutoplayEventFlags>) { }

#if ENABLE(MEDIA_SESSION)
    virtual void mediaSessionMetadataDidChange(WebKit::WebPageProxy&, WebKit::WebMediaSessionMetadata*) { }
#endif

#if PLATFORM(IOS)
#if HAVE(APP_LINKS)
    virtual bool shouldIncludeAppLinkActionsForElement(_WKActivatedElementInfo *) { return true; }
#endif
    virtual RetainPtr<NSArray> actionsForElement(_WKActivatedElementInfo *, RetainPtr<NSArray> defaultActions) { return defaultActions; }
    virtual void didNotHandleTapAsClick(const WebCore::IntPoint&) { }
    virtual UIViewController *presentingViewController() { return nullptr; }
#endif
#if PLATFORM(COCOA)
    virtual NSDictionary *dataDetectionContext() { return nullptr; }
#endif

#if ENABLE(POINTER_LOCK)
    virtual void requestPointerLock(WebKit::WebPageProxy*) { }
    virtual void didLosePointerLock(WebKit::WebPageProxy*) { }
#endif

    virtual void didClickAutoFillButton(WebKit::WebPageProxy&, Object*) { }

    virtual void imageOrMediaDocumentSizeChanged(const WebCore::IntSize&) { }

    virtual void didExceedBackgroundResourceLimitWhileInForeground(WebKit::WebPageProxy&, WKResourceLimit) { }
};

} // namespace API
