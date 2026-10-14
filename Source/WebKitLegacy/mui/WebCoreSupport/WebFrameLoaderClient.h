/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebFrameLoaderClient_h
#define WebFrameLoaderClient_h

#include <WebCore/LocalFrameLoaderClient.h>
#include <WebCore/ResourceLoaderIdentifier.h>
#include <WebCore/ProgressTrackerClient.h>

namespace WebCore {
    class PluginManualLoader;
    class PluginView;
}

class WebFrame;
class WebFramePolicyListener;
class WebHistory;

class WebFrameLoaderClient final : public WebCore::LocalFrameLoaderClient {
public:
    WebFrameLoaderClient(WebFrame* = nullptr);
    ~WebFrameLoaderClient();

    void setWebFrame(WebFrame* webFrame) { m_webFrame = webFrame; }
    WebFrame* webFrame() const { return m_webFrame; }

    void dispatchDidFailToStartPlugin(const WebCore::PluginView&) const;

    bool hasWebView() const override;

    Ref<WebCore::FrameNetworkingContext> createNetworkingContext() override;

    void makeRepresentation(WebCore::DocumentLoader*) override;
    void forceLayoutForNonHTML() override;

    void setCopiesOnScroll() override;

    void detachedFromParent2() override;
    void detachedFromParent3() override;

    void convertMainResourceLoadToDownload(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&) override;
    void assignIdentifierToInitialRequest(WebCore::ResourceLoaderIdentifier identifier, WebCore::DocumentLoader*, const WebCore::ResourceRequest&) override;

    void dispatchWillSendRequest(WebCore::DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse) override;
    bool shouldUseCredentialStorage(WebCore::DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier) override;
    void dispatchDidReceiveAuthenticationChallenge(WebCore::DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, const WebCore::AuthenticationChallenge&) override;
    void dispatchDidReceiveResponse(WebCore::DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, const WebCore::ResourceResponse&) override;
    void dispatchDidReceiveContentLength(WebCore::DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, int dataLength) override;
    void dispatchDidFinishLoading(WebCore::DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier) override;
    void dispatchDidFailLoading(WebCore::DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, const WebCore::ResourceError&) override;

    void dispatchDidDispatchOnloadEvents() override;
    void dispatchDidReceiveServerRedirectForProvisionalLoad() override;
    void dispatchDidCancelClientRedirect() override;
    void dispatchWillPerformClientRedirect(const URL&, double interval, WallTime fireDate, WebCore::LockBackForwardList) override;
    void dispatchDidChangeLocationWithinPage() override;
    void dispatchDidPushStateWithinPage() override;
    void dispatchDidReplaceStateWithinPage() override;
    void dispatchDidPopStateWithinPage() override;
    void dispatchWillClose() override;
    void dispatchDidStartProvisionalLoad() override;
    void dispatchDidReceiveTitle(const WebCore::StringWithDirection&) override;
    void dispatchDidCommitLoad(std::optional<WebCore::HasInsecureContent>, std::optional<WebCore::UsedLegacyTLS>, std::optional<WebCore::WasPrivateRelayed>) override;
    void dispatchDidFailProvisionalLoad(const WebCore::ResourceError&, WebCore::WillContinueLoading, WebCore::WillInternallyHandleFailure) override;
    void dispatchDidFailLoad(const WebCore::ResourceError&) override;
    void dispatchDidFinishDocumentLoad() override;
    void dispatchDidFinishLoad() override;
    void dispatchDidReachLayoutMilestone(OptionSet<WebCore::LayoutMilestone>) override;

    void dispatchDecidePolicyForResponse(const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, const String& downloadAttribute, WebCore::FramePolicyFunction&&) override;
    void dispatchDecidePolicyForNewWindowAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, WebCore::FormState*, const WTF::String& frameName, std::optional<WebCore::HitTestResult>&&, WebCore::FramePolicyFunction&&) override;
    void dispatchDecidePolicyForNavigationAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse, WebCore::FormState*, const String& clientRedirectSourceForHistory, std::optional<WebCore::NavigationIdentifier>, std::optional<WebCore::HitTestResult>&&, bool hasOpener, WebCore::SandboxFlags, WebCore::PolicyDecisionMode, WebCore::FramePolicyFunction&&) override;
    void broadcastMainFrameURLChangeToOtherProcesses(const URL&) override { }
    void dispatchLoadEventToOwnerElementInAnotherProcess() override { }
    void cancelPolicyCheck() override;

    void dispatchUnableToImplementPolicy(const WebCore::ResourceError&) override;

    void dispatchWillSendSubmitEvent(Ref<WebCore::FormState>&&) override;
    void dispatchWillSubmitForm(WebCore::FormState&, CompletionHandler<void()>&&) override;

    void revertToProvisionalState(WebCore::DocumentLoader*) override;
    bool dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int length) override;

    WebCore::LocalFrame* dispatchCreatePage(const WebCore::NavigationAction&, WebCore::NewFrameOpenerPolicy) override;
    void dispatchShow() override;

    void setMainDocumentError(WebCore::DocumentLoader*, const WebCore::ResourceError&) override;
    void setMainFrameDocumentReady(bool) override;

    void startDownload(const WebCore::ResourceRequest&, const String& suggestedName = String(), WebCore::FromDownloadAttribute = WebCore::FromDownloadAttribute::No) override;

    void committedLoad(WebCore::DocumentLoader*, const WebCore::SharedBuffer&) override;
    void finishedLoading(WebCore::DocumentLoader*) override;

    void willChangeTitle(WebCore::DocumentLoader*) override;
    void didChangeTitle(WebCore::DocumentLoader*) override;

    void willReplaceMultipartContent() override { }
    void didReplaceMultipartContent() override { }

    void updateGlobalHistory() override;
    void updateGlobalHistoryRedirectLinks() override;
    bool shouldGoToHistoryItem(WebCore::HistoryItem&) const override;

    void didDisplayInsecureContent() override;
    void didRunInsecureContent(WebCore::SecurityOrigin&) override;

    WebCore::ResourceError cancelledError(const WebCore::ResourceRequest&) const override;
    WebCore::ResourceError blockedError(const WebCore::ResourceRequest&) const  override;
    WebCore::ResourceError blockedByContentBlockerError(const WebCore::ResourceRequest&) const  override;
    WebCore::ResourceError cannotShowURLError(const WebCore::ResourceRequest&) const  override;
    WebCore::ResourceError interruptedForPolicyChangeError(const WebCore::ResourceRequest&) const  override;
    WebCore::ResourceError cannotShowMIMETypeError(const WebCore::ResourceResponse&) const  override;
    WebCore::ResourceError fileDoesNotExistError(const WebCore::ResourceResponse&) const  override;
    WebCore::ResourceError pluginWillHandleLoadError(const WebCore::ResourceResponse&) const  override;
    WebCore::ResourceError httpsUpgradeRedirectLoopError(const WebCore::ResourceRequest&) const override;
    WebCore::ResourceError httpNavigationWithHTTPSOnlyError(const WebCore::ResourceRequest&) const override;
    void loadStorageAccessQuirksIfNeeded() override { }

    bool shouldFallBack(const WebCore::ResourceError&) const override;

    WTF::String userAgent(const URL&) const override;

    Ref<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&) override;
    void updateCachedDocumentLoader(WebCore::DocumentLoader&) override { }

    void setTitle(const WebCore::StringWithDirection&, const URL&) override;

    void savePlatformDataToCachedFrame(WebCore::CachedFrame*) override;
    void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*) override;
    void transitionToCommittedForNewPage(InitializingIframe) override;

    bool canHandleRequest(const WebCore::ResourceRequest&) const override;
    bool canShowMIMEType(const WTF::String& MIMEType) const override;
    bool canShowMIMETypeAsHTML(const WTF::String& MIMEType) const override;
    bool representationExistsForURLScheme(StringView URLScheme) const override;
    WTF::String generatedMIMETypeForURLScheme(StringView URLScheme) const override;

    void frameLoadCompleted() override;
    void saveViewStateToItem(WebCore::HistoryItem&) override;
    void restoreViewState() override;
    void provisionalLoadStarted() override;
    void didFinishLoad() override;
    void prepareForDataSourceReplacement() override;

    void didRestoreFromBackForwardCache() override;

    bool canCachePage() const override;

    RefPtr<WebCore::LocalFrame> createFrame(const AtomString& name, WebCore::HTMLFrameOwnerElement&) override;
    RefPtr<WebCore::Widget> createPlugin(WebCore::HTMLPlugInElement&, const URL&, const Vector<AtomString>&, const Vector<AtomString>&, const WTF::String&, bool loadManually) override;
    void redirectDataToPlugin(WebCore::Widget&) override;

    WebCore::ObjectContentType objectContentType(const URL&, const WTF::String& mimeType) override;
    AtomString overrideMediaType() const override;

    void dispatchDidClearWindowObjectInWorld(WebCore::DOMWrapperWorld&) override;

    WebFramePolicyListener* setUpPolicyListener(WebCore::FramePolicyFunction&&);
    void receivedPolicyDecision(WebCore::PolicyAction);

#if USE(CURL_OPENSSL)
    virtual void didReceiveSSLSecurityExtension(const WebCore::ResourceRequest&, const char*);
#endif

    void prefetchDNS(const String&) override;
    void sendH2Ping(const URL&, CompletionHandler<void(Expected<Seconds, WebCore::ResourceError>&&)>&&) final;

private:
    WebHistory* webHistory() const;
    
    WebFrame* m_webFrame;

    int m_time;

    class WebFramePolicyListenerPrivate;
    std::unique_ptr<WebFramePolicyListenerPrivate> m_policyListenerPrivate;
};

#endif // WebFrameLoaderClient_h
