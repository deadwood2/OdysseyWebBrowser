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

#ifndef UIClient_h
#define UIClient_h

#import "WKFoundation.h"

#if WK_API_ENABLED

#import "APIContextMenuClient.h"
#import "APIUIClient.h"
#import "WeakObjCPtr.h"
#import <wtf/RetainPtr.h>

@class _WKActivatedElementInfo;
@class WKWebView;
@protocol WKUIDelegate;

namespace WebKit {

class UIDelegate {
public:
    explicit UIDelegate(WKWebView *);
    ~UIDelegate();

#if ENABLE(CONTEXT_MENUS)
    std::unique_ptr<API::ContextMenuClient> createContextMenuClient();
#endif
    std::unique_ptr<API::UIClient> createUIClient();

    RetainPtr<id <WKUIDelegate> > delegate();
    void setDelegate(id <WKUIDelegate>);

private:
#if ENABLE(CONTEXT_MENUS)
    class ContextMenuClient : public API::ContextMenuClient {
    public:
        explicit ContextMenuClient(UIDelegate&);
        ~ContextMenuClient();

    private:
        // API::ContextMenuClient
        RetainPtr<NSMenu> menuFromProposedMenu(WebKit::WebPageProxy&, NSMenu *, const WebKit::WebHitTestResultData&, API::Object*) override;

        UIDelegate& m_uiDelegate;
    };
#endif

    class UIClient : public API::UIClient {
    public:
        explicit UIClient(UIDelegate&);
        ~UIClient();

    private:
        // API::UIClient
        PassRefPtr<WebKit::WebPageProxy> createNewPage(WebKit::WebPageProxy*, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, const WebCore::ResourceRequest&, const WebCore::WindowFeatures&, const WebKit::NavigationActionData&) override;
        void close(WebKit::WebPageProxy*) override;
        void fullscreenMayReturnToInline(WebKit::WebPageProxy*) override;
        void didEnterFullscreen(WebKit::WebPageProxy*) override;
        void didExitFullscreen(WebKit::WebPageProxy*) override;
        void runJavaScriptAlert(WebKit::WebPageProxy*, const WTF::String&, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, std::function<void ()> completionHandler) override;
        void runJavaScriptConfirm(WebKit::WebPageProxy*, const WTF::String&, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, std::function<void (bool)> completionHandler) override;
        void runJavaScriptPrompt(WebKit::WebPageProxy*, const WTF::String&, const WTF::String&, WebKit::WebFrameProxy*, const WebCore::SecurityOriginData&, std::function<void (const WTF::String&)> completionHandler) override;
        void exceededDatabaseQuota(WebPageProxy*, WebFrameProxy*, API::SecurityOrigin*, const WTF::String& databaseName, const WTF::String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentUsage, unsigned long long expectedUsage, std::function<void (unsigned long long)>) override;
        void reachedApplicationCacheOriginQuota(WebPageProxy*, const WebCore::SecurityOrigin&, uint64_t currentQuota, uint64_t totalBytesNeeded, std::function<void (unsigned long long)> completionHandler) override;
#if PLATFORM(MAC)
        bool runOpenPanel(WebPageProxy*, WebFrameProxy*, const WebCore::SecurityOriginData&, API::OpenPanelParameters*, WebOpenPanelResultListenerProxy*) override;
#endif
        void printFrame(WebKit::WebPageProxy*, WebKit::WebFrameProxy*) override;
#if PLATFORM(IOS)
#if HAVE(APP_LINKS)
        bool shouldIncludeAppLinkActionsForElement(_WKActivatedElementInfo *) override;
#endif
        RetainPtr<NSArray> actionsForElement(_WKActivatedElementInfo *, RetainPtr<NSArray> defaultActions) override;
        void didNotHandleTapAsClick(const WebCore::IntPoint&) override;
        UIViewController *presentingViewController() override;
#endif // PLATFORM(IOS)

        NSDictionary *dataDetectionContext() override;

        void imageOrMediaDocumentSizeChanged(const WebCore::IntSize&) override;

        UIDelegate& m_uiDelegate;
    };

    WKWebView *m_webView;
    WeakObjCPtr<id <WKUIDelegate> > m_delegate;

    struct {
        bool webViewCreateWebViewWithConfigurationForNavigationActionWindowFeatures : 1;
        bool webViewRunJavaScriptAlertPanelWithMessageInitiatedByFrameCompletionHandler : 1;
        bool webViewRunJavaScriptConfirmPanelWithMessageInitiatedByFrameCompletionHandler : 1;
        bool webViewRunJavaScriptTextInputPanelWithPromptDefaultTextInitiatedByFrameCompletionHandler : 1;
#if PLATFORM(MAC)
        bool webViewRunOpenPanelWithParametersInitiatedByFrameCompletionHandler : 1;
#endif
        bool webViewDecideDatabaseQuotaForSecurityOriginCurrentQuotaCurrentOriginUsageCurrentDatabaseUsageExpectedUsageDecisionHandler : 1;
        bool webViewDecideWebApplicationCacheQuotaForSecurityOriginCurrentQuotaTotalBytesNeeded : 1;
        bool webViewPrintFrame : 1;
        bool webViewDidClose : 1;
        bool webViewClose : 1;
        bool webViewFullscreenMayReturnToInline : 1;
        bool webViewDidEnterFullscreen : 1;
        bool webViewDidExitFullscreen : 1;
#if PLATFORM(IOS)
#if HAVE(APP_LINKS)
        bool webViewShouldIncludeAppLinkActionsForElement : 1;
#endif
        bool webViewActionsForElementDefaultActions : 1;
        bool webViewDidNotHandleTapAsClickAtPoint : 1;
        bool presentingViewControllerForWebView : 1;
#endif
        bool dataDetectionContextForWebView : 1;
        bool webViewImageOrMediaDocumentSizeChanged : 1;

#if ENABLE(CONTEXT_MENUS)
        bool webViewContextMenuForElement : 1;
        bool webViewContextMenuForElementUserInfo : 1;
#endif
    } m_delegateMethods;
};

} // namespace WebKit

#endif // WK_API_ENABLED

#endif // UIClient_h
