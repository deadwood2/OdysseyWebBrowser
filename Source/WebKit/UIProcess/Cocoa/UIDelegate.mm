/*
 * Copyright (C) 2014-2016 Apple Inc. All rights reserved.
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

#import "config.h"
#import "UIDelegate.h"

#if WK_API_ENABLED

#import "APIFrameInfo.h"
#import "CompletionHandlerCallChecker.h"
#import "NavigationActionData.h"
#import "UserMediaPermissionCheckProxy.h"
#import "UserMediaPermissionRequestProxy.h"
#import "WKFrameInfoInternal.h"
#import "WKNavigationActionInternal.h"
#import "WKOpenPanelParametersInternal.h"
#import "WKSecurityOriginInternal.h"
#import "WKUIDelegatePrivate.h"
#import "WKWebViewConfigurationInternal.h"
#import "WKWebViewInternal.h"
#import "WKWindowFeaturesInternal.h"
#import "WebOpenPanelResultListenerProxy.h"
#import "WebProcessProxy.h"
#import "_WKContextMenuElementInfo.h"
#import "_WKFrameHandleInternal.h"
#import <WebCore/SecurityOriginData.h>
#import <WebCore/URL.h>
#import <wtf/BlockPtr.h>

#if PLATFORM(IOS)
#import <AVFoundation/AVCaptureDevice.h>
#import <AVFoundation/AVMediaFormat.h>
#import <wtf/SoftLinking.h>

SOFT_LINK_FRAMEWORK(AVFoundation);
SOFT_LINK_CLASS(AVFoundation, AVCaptureDevice);
SOFT_LINK_CONSTANT(AVFoundation, AVMediaTypeAudio, NSString *);
SOFT_LINK_CONSTANT(AVFoundation, AVMediaTypeVideo, NSString *);
#endif

namespace WebKit {

UIDelegate::UIDelegate(WKWebView *webView)
    : m_webView(webView)
{
}

UIDelegate::~UIDelegate()
{
}

#if ENABLE(CONTEXT_MENUS)
std::unique_ptr<API::ContextMenuClient> UIDelegate::createContextMenuClient()
{
    return std::make_unique<ContextMenuClient>(*this);
}
#endif

std::unique_ptr<API::UIClient> UIDelegate::createUIClient()
{
    return std::make_unique<UIClient>(*this);
}

RetainPtr<id <WKUIDelegate> > UIDelegate::delegate()
{
    return m_delegate.get();
}

void UIDelegate::setDelegate(id <WKUIDelegate> delegate)
{
    m_delegate = delegate;

    m_delegateMethods.webViewCreateWebViewWithConfigurationForNavigationActionWindowFeatures = [delegate respondsToSelector:@selector(webView:createWebViewWithConfiguration:forNavigationAction:windowFeatures:)];
    m_delegateMethods.webViewCreateWebViewWithConfigurationForNavigationActionWindowFeaturesAsync = [delegate respondsToSelector:@selector(_webView:createWebViewWithConfiguration:forNavigationAction:windowFeatures:completionHandler:)];
    m_delegateMethods.webViewRunJavaScriptAlertPanelWithMessageInitiatedByFrameCompletionHandler = [delegate respondsToSelector:@selector(webView:runJavaScriptAlertPanelWithMessage:initiatedByFrame:completionHandler:)];
    m_delegateMethods.webViewRunJavaScriptConfirmPanelWithMessageInitiatedByFrameCompletionHandler = [delegate respondsToSelector:@selector(webView:runJavaScriptConfirmPanelWithMessage:initiatedByFrame:completionHandler:)];
    m_delegateMethods.webViewRunJavaScriptTextInputPanelWithPromptDefaultTextInitiatedByFrameCompletionHandler = [delegate respondsToSelector:@selector(webView:runJavaScriptTextInputPanelWithPrompt:defaultText:initiatedByFrame:completionHandler:)];
    m_delegateMethods.webViewRunBeforeUnloadConfirmPanelWithMessageInitiatedByFrameCompletionHandler = [delegate respondsToSelector:@selector(_webView:runBeforeUnloadConfirmPanelWithMessage:initiatedByFrame:completionHandler:)];

#if PLATFORM(MAC)
    m_delegateMethods.webViewRunOpenPanelWithParametersInitiatedByFrameCompletionHandler = [delegate respondsToSelector:@selector(webView:runOpenPanelWithParameters:initiatedByFrame:completionHandler:)];
#endif

    m_delegateMethods.webViewDecideDatabaseQuotaForSecurityOriginCurrentQuotaCurrentOriginUsageCurrentDatabaseUsageExpectedUsageDecisionHandler = [delegate respondsToSelector:@selector(_webView:decideDatabaseQuotaForSecurityOrigin:currentQuota:currentOriginUsage:currentDatabaseUsage:expectedUsage:decisionHandler:)];
    m_delegateMethods.webViewDecideWebApplicationCacheQuotaForSecurityOriginCurrentQuotaTotalBytesNeeded = [delegate respondsToSelector:@selector(_webView:decideWebApplicationCacheQuotaForSecurityOrigin:currentQuota:totalBytesNeeded:decisionHandler:)];
    m_delegateMethods.webViewPrintFrame = [delegate respondsToSelector:@selector(_webView:printFrame:)];
    m_delegateMethods.webViewDidClose = [delegate respondsToSelector:@selector(webViewDidClose:)];
    m_delegateMethods.webViewClose = [delegate respondsToSelector:@selector(_webViewClose:)];
    m_delegateMethods.webViewFullscreenMayReturnToInline = [delegate respondsToSelector:@selector(_webViewFullscreenMayReturnToInline:)];
    m_delegateMethods.webViewDidEnterFullscreen = [delegate respondsToSelector:@selector(_webViewDidEnterFullscreen:)];
    m_delegateMethods.webViewDidExitFullscreen = [delegate respondsToSelector:@selector(_webViewDidExitFullscreen:)];
#if PLATFORM(IOS)
#if HAVE(APP_LINKS)
    m_delegateMethods.webViewShouldIncludeAppLinkActionsForElement = [delegate respondsToSelector:@selector(_webView:shouldIncludeAppLinkActionsForElement:)];
#endif
    m_delegateMethods.webViewActionsForElementDefaultActions = [delegate respondsToSelector:@selector(_webView:actionsForElement:defaultActions:)];
    m_delegateMethods.webViewDidNotHandleTapAsClickAtPoint = [delegate respondsToSelector:@selector(_webView:didNotHandleTapAsClickAtPoint:)];
    m_delegateMethods.presentingViewControllerForWebView = [delegate respondsToSelector:@selector(_presentingViewControllerForWebView:)];
#endif
    m_delegateMethods.webViewRequestUserMediaAuthorizationForDevicesURLMainFrameURLDecisionHandler = [delegate respondsToSelector:@selector(_webView:requestUserMediaAuthorizationForDevices:url:mainFrameURL:decisionHandler:)];
    m_delegateMethods.webViewCheckUserMediaPermissionForURLMainFrameURLFrameIdentifierDecisionHandler = [delegate respondsToSelector:@selector(_webView:checkUserMediaPermissionForURL:mainFrameURL:frameIdentifier:decisionHandler:)];
    m_delegateMethods.webViewMediaCaptureStateDidChange = [delegate respondsToSelector:@selector(_webView:mediaCaptureStateDidChange:)];
    m_delegateMethods.dataDetectionContextForWebView = [delegate respondsToSelector:@selector(_dataDetectionContextForWebView:)];
    m_delegateMethods.webViewImageOrMediaDocumentSizeChanged = [delegate respondsToSelector:@selector(_webView:imageOrMediaDocumentSizeChanged:)];

#if ENABLE(POINTER_LOCK)
    m_delegateMethods.webViewRequestPointerLock = [delegate respondsToSelector:@selector(_webViewRequestPointerLock:)];
    m_delegateMethods.webViewDidLosePointerLock = [delegate respondsToSelector:@selector(_webViewDidLosePointerLock:)];
#endif
#if ENABLE(CONTEXT_MENUS)
    m_delegateMethods.webViewContextMenuForElement = [delegate respondsToSelector:@selector(_webView:contextMenu:forElement:)];
    m_delegateMethods.webViewContextMenuForElementUserInfo = [delegate respondsToSelector:@selector(_webView:contextMenu:forElement:userInfo:)];
#endif
    
    m_delegateMethods.webViewHasVideoInPictureInPictureDidChange = [delegate respondsToSelector:@selector(_webView:hasVideoInPictureInPictureDidChange:)];
}

#if ENABLE(CONTEXT_MENUS)
UIDelegate::ContextMenuClient::ContextMenuClient(UIDelegate& uiDelegate)
    : m_uiDelegate(uiDelegate)
{
}

UIDelegate::ContextMenuClient::~ContextMenuClient()
{
}

RetainPtr<NSMenu> UIDelegate::ContextMenuClient::menuFromProposedMenu(WebKit::WebPageProxy&, NSMenu *menu, const WebKit::WebHitTestResultData&, API::Object* userInfo)
{
    if (!m_uiDelegate.m_delegateMethods.webViewContextMenuForElement && !m_uiDelegate.m_delegateMethods.webViewContextMenuForElementUserInfo)
        return menu;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return menu;

    auto contextMenuElementInfo = adoptNS([[_WKContextMenuElementInfo alloc] init]);

    if (m_uiDelegate.m_delegateMethods.webViewContextMenuForElement)
        return [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView contextMenu:menu forElement:contextMenuElementInfo.get()];

    return [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView contextMenu:menu forElement:contextMenuElementInfo.get() userInfo:static_cast<id <NSSecureCoding>>(userInfo->wrapper())];
}
#endif

UIDelegate::UIClient::UIClient(UIDelegate& uiDelegate)
    : m_uiDelegate(uiDelegate)
{
}

UIDelegate::UIClient::~UIClient()
{
}

RefPtr<WebKit::WebPageProxy> UIDelegate::UIClient::createNewPageCommon(WebKit::WebPageProxy* page, API::FrameInfo& sourceFrameInfo, WebCore::ResourceRequest&& request, const WebCore::WindowFeatures& windowFeatures, WebKit::NavigationActionData&& navigationActionData, WTF::Function<void(RefPtr<WebKit::WebPageProxy>&&)>&& completionHandler)
{
    auto delegate = m_uiDelegate.m_delegate.get();
    ASSERT(delegate);

    auto configuration = adoptNS([m_uiDelegate.m_webView->_configuration copy]);
    [configuration _setRelatedWebView:m_uiDelegate.m_webView];

    auto userInitiatedActivity = page->process().userInitiatedActivity(navigationActionData.userGestureTokenIdentifier);
    bool shouldOpenAppLinks = !hostsAreEqual(sourceFrameInfo.request().url(), request.url());
    auto apiNavigationAction = API::NavigationAction::create(WTFMove(navigationActionData), &sourceFrameInfo, nullptr, WTFMove(request), WebCore::URL(), shouldOpenAppLinks, WTFMove(userInitiatedActivity));

    auto apiWindowFeatures = API::WindowFeatures::create(windowFeatures);

    if (completionHandler) {
        RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(_webView:createWebViewWithConfiguration:forNavigationAction:windowFeatures:completionHandler:));

        [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView createWebViewWithConfiguration:configuration.get() forNavigationAction:wrapper(apiNavigationAction) windowFeatures:wrapper(apiWindowFeatures) completionHandler:BlockPtr<void (WKWebView *)>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker), relatedWebView = RetainPtr<WKWebView>(m_uiDelegate.m_webView)](WKWebView *webView) {
            if (checker->completionHandlerHasBeenCalled())
                return;
            checker->didCallCompletionHandler();

            if (!webView) {
                completionHandler(nullptr);
                return;
            }

            if ([webView->_configuration _relatedWebView] != relatedWebView.get())
                [NSException raise:NSInternalInconsistencyException format:@"Returned WKWebView was not created with the given configuration."];

            completionHandler(webView->_page.get());
        }).get()];

        return nullptr;
    }

    RetainPtr<WKWebView> webView = [delegate webView:m_uiDelegate.m_webView createWebViewWithConfiguration:configuration.get() forNavigationAction:wrapper(apiNavigationAction) windowFeatures:wrapper(apiWindowFeatures)];

    if (!webView)
        return nullptr;

    if ([webView->_configuration _relatedWebView] != m_uiDelegate.m_webView)
        [NSException raise:NSInternalInconsistencyException format:@"Returned WKWebView was not created with the given configuration."];

    return webView->_page.get();
}

RefPtr<WebPageProxy> UIDelegate::UIClient::createNewPage(WebPageProxy* page, API::FrameInfo& originatingFrameInfo, WebCore::ResourceRequest&& request, const WebCore::WindowFeatures& windowFeatures, NavigationActionData&& navigationActionData)
{
    if (!m_uiDelegate.m_delegateMethods.webViewCreateWebViewWithConfigurationForNavigationActionWindowFeatures)
        return nullptr;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return nullptr;

    return createNewPageCommon(page, originatingFrameInfo, WTFMove(request), windowFeatures, WTFMove(navigationActionData), nullptr);
}

bool UIDelegate::UIClient::canCreateNewPageAsync()
{
    return m_uiDelegate.m_delegateMethods.webViewCreateWebViewWithConfigurationForNavigationActionWindowFeaturesAsync
        && m_uiDelegate.m_delegate.get();
}

void UIDelegate::UIClient::createNewPageAsync(WebPageProxy* page, API::FrameInfo& originatingFrameInfo, WebCore::ResourceRequest&& request, const WebCore::WindowFeatures& windowFeatures, NavigationActionData&& navigationActionData, WTF::Function<void(RefPtr<WebPageProxy>&&)>&& completionHandler)
{
    ASSERT(canCreateNewPageAsync());
    ASSERT(m_uiDelegate.m_delegateMethods.webViewCreateWebViewWithConfigurationForNavigationActionWindowFeaturesAsync);

    auto delegate = m_uiDelegate.m_delegate.get();
    ASSERT(delegate);

    createNewPageCommon(page, originatingFrameInfo, WTFMove(request), windowFeatures, WTFMove(navigationActionData), WTFMove(completionHandler));
}

void UIDelegate::UIClient::runJavaScriptAlert(WebKit::WebPageProxy*, const WTF::String& message, WebKit::WebFrameProxy* webFrameProxy, const WebCore::SecurityOriginData& securityOriginData, Function<void ()>&& completionHandler)
{
    if (!m_uiDelegate.m_delegateMethods.webViewRunJavaScriptAlertPanelWithMessageInitiatedByFrameCompletionHandler) {
        completionHandler();
        return;
    }

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate) {
        completionHandler();
        return;
    }

    RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(webView:runJavaScriptAlertPanelWithMessage:initiatedByFrame:completionHandler:));
    [delegate webView:m_uiDelegate.m_webView runJavaScriptAlertPanelWithMessage:message initiatedByFrame:wrapper(API::FrameInfo::create(*webFrameProxy, securityOriginData.securityOrigin())) completionHandler:BlockPtr<void ()>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)] {
        if (checker->completionHandlerHasBeenCalled())
            return;
        completionHandler();
        checker->didCallCompletionHandler();
    }).get()];
}

void UIDelegate::UIClient::runJavaScriptConfirm(WebKit::WebPageProxy*, const WTF::String& message, WebKit::WebFrameProxy* webFrameProxy, const WebCore::SecurityOriginData& securityOriginData, Function<void (bool)>&& completionHandler)
{
    if (!m_uiDelegate.m_delegateMethods.webViewRunJavaScriptConfirmPanelWithMessageInitiatedByFrameCompletionHandler) {
        completionHandler(false);
        return;
    }

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate) {
        completionHandler(false);
        return;
    }

    RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(webView:runJavaScriptConfirmPanelWithMessage:initiatedByFrame:completionHandler:));
    [delegate webView:m_uiDelegate.m_webView runJavaScriptConfirmPanelWithMessage:message initiatedByFrame:wrapper(API::FrameInfo::create(*webFrameProxy, securityOriginData.securityOrigin())) completionHandler:BlockPtr<void (BOOL)>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)](BOOL result) {
        if (checker->completionHandlerHasBeenCalled())
            return;
        completionHandler(result);
        checker->didCallCompletionHandler();
    }).get()];
}

void UIDelegate::UIClient::runJavaScriptPrompt(WebKit::WebPageProxy*, const WTF::String& message, const WTF::String& defaultValue, WebKit::WebFrameProxy* webFrameProxy, const WebCore::SecurityOriginData& securityOriginData, Function<void (const WTF::String&)>&& completionHandler)
{
    if (!m_uiDelegate.m_delegateMethods.webViewRunJavaScriptTextInputPanelWithPromptDefaultTextInitiatedByFrameCompletionHandler) {
        completionHandler(String());
        return;
    }

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate) {
        completionHandler(String());
        return;
    }

    RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(webView:runJavaScriptTextInputPanelWithPrompt:defaultText:initiatedByFrame:completionHandler:));
    [delegate webView:m_uiDelegate.m_webView runJavaScriptTextInputPanelWithPrompt:message defaultText:defaultValue initiatedByFrame:wrapper(API::FrameInfo::create(*webFrameProxy, securityOriginData.securityOrigin())) completionHandler:BlockPtr<void (NSString *)>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)](NSString *result) {
        if (checker->completionHandlerHasBeenCalled())
            return;
        completionHandler(result);
        checker->didCallCompletionHandler();
    }).get()];
}

bool UIDelegate::UIClient::canRunBeforeUnloadConfirmPanel() const
{
    return m_uiDelegate.m_delegateMethods.webViewRunBeforeUnloadConfirmPanelWithMessageInitiatedByFrameCompletionHandler;
}

void UIDelegate::UIClient::runBeforeUnloadConfirmPanel(WebKit::WebPageProxy*, const WTF::String& message, WebKit::WebFrameProxy* webFrameProxy, const WebCore::SecurityOriginData& securityOriginData, Function<void (bool)>&& completionHandler)
{
    if (!m_uiDelegate.m_delegateMethods.webViewRunBeforeUnloadConfirmPanelWithMessageInitiatedByFrameCompletionHandler) {
        completionHandler(false);
        return;
    }

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate) {
        completionHandler(false);
        return;
    }

    RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(_webView:runBeforeUnloadConfirmPanelWithMessage:initiatedByFrame:completionHandler:));
    [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView runBeforeUnloadConfirmPanelWithMessage:message initiatedByFrame:wrapper(API::FrameInfo::create(*webFrameProxy, securityOriginData.securityOrigin())) completionHandler:BlockPtr<void (BOOL)>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)](BOOL result) {
        if (checker->completionHandlerHasBeenCalled())
            return;
        completionHandler(result);
        checker->didCallCompletionHandler();
    }).get()];
}

void UIDelegate::UIClient::exceededDatabaseQuota(WebPageProxy*, WebFrameProxy*, API::SecurityOrigin* securityOrigin, const WTF::String& databaseName, const WTF::String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentUsage, unsigned long long expectedUsage, Function<void (unsigned long long)>&& completionHandler)
{
    if (!m_uiDelegate.m_delegateMethods.webViewDecideDatabaseQuotaForSecurityOriginCurrentQuotaCurrentOriginUsageCurrentDatabaseUsageExpectedUsageDecisionHandler) {

        // Use 50 MB as the default database quota.
        unsigned long long defaultPerOriginDatabaseQuota = 50 * 1024 * 1024;

        completionHandler(defaultPerOriginDatabaseQuota);
        return;
    }

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate) {
        completionHandler(currentQuota);
        return;
    }

    ASSERT(securityOrigin);
    RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(_webView:decideDatabaseQuotaForSecurityOrigin:currentQuota:currentOriginUsage:currentDatabaseUsage:expectedUsage:decisionHandler:));
    [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView decideDatabaseQuotaForSecurityOrigin:wrapper(*securityOrigin) currentQuota:currentQuota currentOriginUsage:currentOriginUsage currentDatabaseUsage:currentUsage expectedUsage:expectedUsage decisionHandler:BlockPtr<void (unsigned long long newQuota)>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)](unsigned long long newQuota) {
        if (checker->completionHandlerHasBeenCalled())
            return;
        checker->didCallCompletionHandler();
        completionHandler(newQuota);
    }).get()];
}

#if PLATFORM(MAC)
bool UIDelegate::UIClient::runOpenPanel(WebPageProxy*, WebFrameProxy* webFrameProxy, const WebCore::SecurityOriginData& securityOriginData, API::OpenPanelParameters* openPanelParameters, WebOpenPanelResultListenerProxy* listener)
{
    if (!m_uiDelegate.m_delegateMethods.webViewRunOpenPanelWithParametersInitiatedByFrameCompletionHandler)
        return false;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return false;

    auto frame = API::FrameInfo::create(*webFrameProxy, securityOriginData.securityOrigin());
    RefPtr<WebOpenPanelResultListenerProxy> resultListener = listener;

    RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(webView:runOpenPanelWithParameters:initiatedByFrame:completionHandler:));

    [delegate webView:m_uiDelegate.m_webView runOpenPanelWithParameters:wrapper(*openPanelParameters) initiatedByFrame:wrapper(frame) completionHandler:[checker, resultListener](NSArray *URLs) {
        if (checker->completionHandlerHasBeenCalled())
            return;
        checker->didCallCompletionHandler();

        if (!URLs) {
            resultListener->cancel();
            return;
        }

        Vector<String> filenames;
        for (NSURL *url in URLs)
            filenames.append(url.path);

        resultListener->chooseFiles(filenames);
    }];

    return true;
}
#endif

static void requestUserMediaAuthorizationForDevices(const WebKit::WebFrameProxy& frame, WebKit::UserMediaPermissionRequestProxy& request, id <WKUIDelegatePrivate> delegate, WKWebView& webView)
{
    auto decisionHandler = BlockPtr<void(BOOL)>::fromCallable([protectedRequest = makeRef(request)](BOOL authorized) {
        if (!authorized) {
            protectedRequest->deny(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied);
            return;
        }
        const String& videoDeviceUID = protectedRequest->requiresVideo() ? protectedRequest->videoDeviceUIDs().first() : String();
        const String& audioDeviceUID = protectedRequest->requiresAudio() ? protectedRequest->audioDeviceUIDs().first() : String();
        protectedRequest->allow(audioDeviceUID, videoDeviceUID);
    });

    const WebFrameProxy* mainFrame = frame.page()->mainFrame();
    WebCore::URL requestFrameURL(WebCore::URL(), frame.url());
    WebCore::URL mainFrameURL(WebCore::URL(), mainFrame->url());

    _WKCaptureDevices devices = 0;
    if (request.requiresAudio())
        devices |= _WKCaptureDeviceMicrophone;
    if (request.requiresVideo())
        devices |= _WKCaptureDeviceCamera;

    auto protectedWebView = RetainPtr<WKWebView>(&webView);
    [delegate _webView:protectedWebView.get() requestUserMediaAuthorizationForDevices:devices url:requestFrameURL mainFrameURL:mainFrameURL decisionHandler:decisionHandler.get()];
}

bool UIDelegate::UIClient::decidePolicyForUserMediaPermissionRequest(WebKit::WebPageProxy& page, WebKit::WebFrameProxy& frame, API::SecurityOrigin& userMediaOrigin, API::SecurityOrigin& topLevelOrigin, WebKit::UserMediaPermissionRequestProxy& request)
{
    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate || !m_uiDelegate.m_delegateMethods.webViewRequestUserMediaAuthorizationForDevicesURLMainFrameURLDecisionHandler) {
        request.deny(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::UserMediaDisabled);
        return true;
    }

    bool requiresAudio = request.requiresAudio();
    bool requiresVideo = request.requiresVideo();
    if (!requiresAudio && !requiresVideo) {
        request.deny(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::NoConstraints);
        return true;
    }

#if PLATFORM(IOS)
    auto requestCameraAuthorization = BlockPtr<void()>::fromCallable([this, &frame, protectedRequest = makeRef(request), webView = RetainPtr<WKWebView>(m_uiDelegate.m_webView)]() {

        if (!protectedRequest->requiresVideo()) {
            requestUserMediaAuthorizationForDevices(frame, protectedRequest, (id <WKUIDelegatePrivate>)m_uiDelegate.m_delegate.get(), *webView.get());
            return;
        }
        AVAuthorizationStatus cameraAuthorizationStatus = [getAVCaptureDeviceClass() authorizationStatusForMediaType:getAVMediaTypeVideo()];
        switch (cameraAuthorizationStatus) {
        case AVAuthorizationStatusAuthorized:
            requestUserMediaAuthorizationForDevices(frame, protectedRequest, (id <WKUIDelegatePrivate>)m_uiDelegate.m_delegate.get(), *webView.get());
            break;
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
            protectedRequest->deny(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied);
            return;
        case AVAuthorizationStatusNotDetermined:
            auto decisionHandler = BlockPtr<void(BOOL)>::fromCallable([this, &frame, protectedRequest = makeRef(protectedRequest.get()), webView = RetainPtr<WKWebView>(m_uiDelegate.m_webView)](BOOL authorized) {
                if (!authorized) {
                    protectedRequest->deny(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied);
                    return;
                }
                requestUserMediaAuthorizationForDevices(frame, protectedRequest, (id <WKUIDelegatePrivate>)m_uiDelegate.m_delegate.get(), *webView.get());
            });

            [getAVCaptureDeviceClass() requestAccessForMediaType:getAVMediaTypeVideo() completionHandler:decisionHandler.get()];
            break;
        }
    });

    if (requiresAudio) {
        AVAuthorizationStatus microphoneAuthorizationStatus = [getAVCaptureDeviceClass() authorizationStatusForMediaType:getAVMediaTypeAudio()];
        switch (microphoneAuthorizationStatus) {
        case AVAuthorizationStatusAuthorized:
            requestCameraAuthorization();
            break;
        case AVAuthorizationStatusDenied:
        case AVAuthorizationStatusRestricted:
            request.deny(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied);
            return true;
        case AVAuthorizationStatusNotDetermined:
            auto decisionHandler = BlockPtr<void(BOOL)>::fromCallable([protectedRequest = makeRef(request), requestCameraAuthorization](BOOL authorized) {
                if (!authorized) {
                    protectedRequest->deny(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied);
                    return;
                }
                requestCameraAuthorization();
            });

            [getAVCaptureDeviceClass() requestAccessForMediaType:getAVMediaTypeAudio() completionHandler:decisionHandler.get()];
            break;
        }
    } else
        requestCameraAuthorization();
#else
    requestUserMediaAuthorizationForDevices(frame, request, (id <WKUIDelegatePrivate>)m_uiDelegate.m_delegate.get(), *m_uiDelegate.m_webView);
#endif

    return true;
}

bool UIDelegate::UIClient::checkUserMediaPermissionForOrigin(WebKit::WebPageProxy& page, WebKit::WebFrameProxy& frame, API::SecurityOrigin& userMediaOrigin, API::SecurityOrigin& topLevelOrigin, WebKit::UserMediaPermissionCheckProxy& request)
{
    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate || !m_uiDelegate.m_delegateMethods.webViewCheckUserMediaPermissionForURLMainFrameURLFrameIdentifierDecisionHandler) {
        request.setUserMediaAccessInfo(String(), false);
        return true;
    }

    WKWebView *webView = m_uiDelegate.m_webView;
    const WebFrameProxy* mainFrame = frame.page()->mainFrame();
    WebCore::URL requestFrameURL(WebCore::URL(), frame.url());
    WebCore::URL mainFrameURL(WebCore::URL(), mainFrame->url());

    auto decisionHandler = BlockPtr<void(NSString *, BOOL)>::fromCallable([protectedRequest = makeRef(request)](NSString *salt, BOOL authorized) {
        protectedRequest->setUserMediaAccessInfo(String(salt), authorized);
    });

    [(id <WKUIDelegatePrivate>)delegate _webView:webView checkUserMediaPermissionForURL:requestFrameURL mainFrameURL:mainFrameURL frameIdentifier:frame.frameID() decisionHandler:decisionHandler.get()];

    return true;
}

void UIDelegate::UIClient::mediaCaptureStateDidChange(WebCore::MediaProducer::MediaStateFlags state)
{
    WKWebView *webView = m_uiDelegate.m_webView;
    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate || !m_uiDelegate.m_delegateMethods.webViewMediaCaptureStateDidChange)
        return;

    _WKMediaCaptureState mediaCaptureState = _WKMediaCaptureStateNone;
    if (state & WebCore::MediaProducer::HasActiveAudioCaptureDevice)
        mediaCaptureState |= _WKMediaCaptureStateActiveMicrophone;
    if (state & WebCore::MediaProducer::HasActiveVideoCaptureDevice)
        mediaCaptureState |= _WKMediaCaptureStateActiveCamera;
    if (state & WebCore::MediaProducer::HasMutedAudioCaptureDevice)
        mediaCaptureState |= _WKMediaCaptureStateMutedMicrophone;
    if (state & WebCore::MediaProducer::HasMutedVideoCaptureDevice)
        mediaCaptureState |= _WKMediaCaptureStateMutedCamera;

    [(id <WKUIDelegatePrivate>)delegate _webView:webView mediaCaptureStateDidChange:mediaCaptureState];
}

void UIDelegate::UIClient::reachedApplicationCacheOriginQuota(WebPageProxy*, const WebCore::SecurityOrigin& securityOrigin, uint64_t currentQuota, uint64_t totalBytesNeeded, Function<void (unsigned long long)>&& completionHandler)
{
    if (!m_uiDelegate.m_delegateMethods.webViewDecideWebApplicationCacheQuotaForSecurityOriginCurrentQuotaTotalBytesNeeded) {
        completionHandler(currentQuota);
        return;
    }

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate) {
        completionHandler(currentQuota);
        return;
    }

    RefPtr<CompletionHandlerCallChecker> checker = CompletionHandlerCallChecker::create(delegate.get(), @selector(_webView:decideWebApplicationCacheQuotaForSecurityOrigin:currentQuota:totalBytesNeeded:decisionHandler:));
    RefPtr<API::SecurityOrigin> apiOrigin = API::SecurityOrigin::create(securityOrigin);
    
    [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView decideWebApplicationCacheQuotaForSecurityOrigin:wrapper(*apiOrigin) currentQuota:currentQuota totalBytesNeeded:totalBytesNeeded decisionHandler:BlockPtr<void (unsigned long long)>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)](unsigned long long newQuota) {
        if (checker->completionHandlerHasBeenCalled())
            return;
        checker->didCallCompletionHandler();
        completionHandler(newQuota);
    }).get()];
}

void UIDelegate::UIClient::printFrame(WebKit::WebPageProxy*, WebKit::WebFrameProxy* webFrameProxy)
{
    ASSERT_ARG(webFrameProxy, webFrameProxy);

    if (!m_uiDelegate.m_delegateMethods.webViewPrintFrame)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView printFrame:wrapper(API::FrameHandle::create(webFrameProxy->frameID()))];
}

void UIDelegate::UIClient::close(WebKit::WebPageProxy*)
{
    if (m_uiDelegate.m_delegateMethods.webViewClose) {
        auto delegate = m_uiDelegate.m_delegate.get();
        if (!delegate)
            return;

        [(id <WKUIDelegatePrivate>)delegate _webViewClose:m_uiDelegate.m_webView];
        return;
    }

    if (!m_uiDelegate.m_delegateMethods.webViewDidClose)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [delegate webViewDidClose:m_uiDelegate.m_webView];
}

void UIDelegate::UIClient::fullscreenMayReturnToInline(WebKit::WebPageProxy*)
{
    if (!m_uiDelegate.m_delegateMethods.webViewFullscreenMayReturnToInline)
        return;
    
    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;
    
    [(id <WKUIDelegatePrivate>)delegate _webViewFullscreenMayReturnToInline:m_uiDelegate.m_webView];
}

void UIDelegate::UIClient::didEnterFullscreen(WebKit::WebPageProxy*)
{
    if (!m_uiDelegate.m_delegateMethods.webViewDidEnterFullscreen)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [(id <WKUIDelegatePrivate>)delegate _webViewDidEnterFullscreen:m_uiDelegate.m_webView];
}

void UIDelegate::UIClient::didExitFullscreen(WebKit::WebPageProxy*)
{
    if (!m_uiDelegate.m_delegateMethods.webViewDidExitFullscreen)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [(id <WKUIDelegatePrivate>)delegate _webViewDidExitFullscreen:m_uiDelegate.m_webView];
}
    
#if PLATFORM(IOS)
#if HAVE(APP_LINKS)
bool UIDelegate::UIClient::shouldIncludeAppLinkActionsForElement(_WKActivatedElementInfo *elementInfo)
{
    if (!m_uiDelegate.m_delegateMethods.webViewShouldIncludeAppLinkActionsForElement)
        return true;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return true;

    return [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView shouldIncludeAppLinkActionsForElement:elementInfo];
}
#endif

RetainPtr<NSArray> UIDelegate::UIClient::actionsForElement(_WKActivatedElementInfo *elementInfo, RetainPtr<NSArray> defaultActions)
{
    if (!m_uiDelegate.m_delegateMethods.webViewActionsForElementDefaultActions)
        return defaultActions;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return defaultActions;

    return [(id <WKUIDelegatePrivate>)delegate _webView:m_uiDelegate.m_webView actionsForElement:elementInfo defaultActions:defaultActions.get()];
}

void UIDelegate::UIClient::didNotHandleTapAsClick(const WebCore::IntPoint& point)
{
    if (!m_uiDelegate.m_delegateMethods.webViewDidNotHandleTapAsClickAtPoint)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [static_cast<id <WKUIDelegatePrivate>>(delegate) _webView:m_uiDelegate.m_webView didNotHandleTapAsClickAtPoint:point];
}

UIViewController *UIDelegate::UIClient::presentingViewController()
{
    if (!m_uiDelegate.m_delegateMethods.presentingViewControllerForWebView)
        return nullptr;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return nullptr;

    return [static_cast<id <WKUIDelegatePrivate>>(delegate) _presentingViewControllerForWebView:m_uiDelegate.m_webView];
}

#endif

NSDictionary *UIDelegate::UIClient::dataDetectionContext()
{
    if (!m_uiDelegate.m_delegateMethods.dataDetectionContextForWebView)
        return nullptr;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return nullptr;

    return [static_cast<id <WKUIDelegatePrivate>>(delegate) _dataDetectionContextForWebView:m_uiDelegate.m_webView];
}

#if ENABLE(POINTER_LOCK)

void UIDelegate::UIClient::requestPointerLock(WebKit::WebPageProxy*)
{
    if (!m_uiDelegate.m_delegateMethods.webViewRequestPointerLock)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [static_cast<id <WKUIDelegatePrivate>>(delegate) _webViewRequestPointerLock:m_uiDelegate.m_webView];
}

void UIDelegate::UIClient::didLosePointerLock(WebKit::WebPageProxy*)
{
    if (!m_uiDelegate.m_delegateMethods.webViewDidLosePointerLock)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [static_cast<id <WKUIDelegatePrivate>>(delegate) _webViewDidLosePointerLock:m_uiDelegate.m_webView];
}

#endif
    
void UIDelegate::UIClient::hasVideoInPictureInPictureDidChange(WebKit::WebPageProxy*, bool hasVideoInPictureInPicture)
{
    if (!m_uiDelegate.m_delegateMethods.webViewHasVideoInPictureInPictureDidChange)
        return;
    
    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;
    
    [static_cast<id <WKUIDelegatePrivate>>(delegate) _webView:m_uiDelegate.m_webView hasVideoInPictureInPictureDidChange:hasVideoInPictureInPicture];
}

void UIDelegate::UIClient::imageOrMediaDocumentSizeChanged(const WebCore::IntSize& newSize)
{
    if (!m_uiDelegate.m_delegateMethods.webViewImageOrMediaDocumentSizeChanged)
        return;

    auto delegate = m_uiDelegate.m_delegate.get();
    if (!delegate)
        return;

    [static_cast<id <WKUIDelegatePrivate>>(delegate) _webView:m_uiDelegate.m_webView imageOrMediaDocumentSizeChanged:newSize];
}

} // namespace WebKit

#endif // WK_API_ENABLED
