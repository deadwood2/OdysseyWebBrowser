/*
 * Copyright (C) 2010-2018 Apple Inc. All rights reserved.
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

#include "WebKit.h"
#include "WebFrameLoaderClient.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebDocumentLoader.h"
#include "WebFrameNetworkingContext.h"

#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSObject.h>
#include <WebCore/CachedFrame.h>
#include <WebCore/CertificateInfo.h>
#include <WebCore/Chrome.h>
#include <WebCore/DOMWrapperWorld.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/FormState.h>
#include <WebCore/LocalFrame.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/LocalFrameView.h>
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HistoryController.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/MIMETypeRegistry.h>
#include <WebCore/MouseEvent.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/PluginData.h>
#include <WebCore/PluginDocument.h>
#include <WebCore/PolicyChecker.h>
#include <WebCore/ProgressTracker.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ScriptController.h>
#include <WebCore/SecurityOriginData.h>
#include <WebCore/Settings.h>
#include <WebCore/SubframeLoader.h>
#include <WebCore/UIEventWithKeyState.h>
#include <WebCore/Widget.h>
#include <WebCore/WindowFeatures.h>
#include <WebCore/CertificateInfo.h>
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/AuthenticationClient.h>
#include <WebCore/BitmapImage.h>
#include <WebCore/FullscreenManager.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ProcessID.h>
#include <wtf/ProcessPrivilege.h>
#include <wtf/HexNumber.h>
#include "../../WTF/wtf/morphos/MD5.h"

#define D(x) 

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"

namespace WebKit {
using namespace WebCore;

WebFrameLoaderClient::WebFrameLoaderClient(Ref<WebFrame>&& frame)
    : m_frame(WTFMove(frame))
    , m_didCompletePageTransition(false)
    , m_frameCameFromPageCache(false)
{
	D(dprintf("%s: this %p\n", __PRETTY_FUNCTION__, this));
}

WebFrameLoaderClient::~WebFrameLoaderClient()
{
	D(dprintf("%s: this %p\n", __PRETTY_FUNCTION__, this));
	m_frame->invalidate();
}

#if ENABLE(RESOURCE_LOAD_STATISTICS)
void WebFrameLoaderClient::setHasFrameSpecificStorageAccess(FrameSpecificStorageAccessIdentifier&& frameSpecificStorageAccessIdentifier )
{
    ASSERT(!m_frameSpecificStorageAccessIdentifier);

    m_frameSpecificStorageAccessIdentifier = WTFMove(frameSpecificStorageAccessIdentifier);
}
#endif

bool WebFrameLoaderClient::hasHTMLView() const
{
    return true;
}

bool WebFrameLoaderClient::hasWebView() const
{
    return m_frame->page();
}

void WebFrameLoaderClient::makeRepresentation(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::forceLayoutForNonHTML()
{
    notImplemented();
}

void WebFrameLoaderClient::setCopiesOnScroll()
{
    notImplemented();
}

void WebFrameLoaderClient::detachedFromParent2()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    notImplemented();

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (m_frameSpecificStorageAccessIdentifier) {
        WebProcess::singleton().ensureNetworkProcessConnection().connection().send(Messages::NetworkConnectionToWebProcess::RemoveStorageAccessForFrame(
            m_frameSpecificStorageAccessIdentifier->sessionID, m_frameSpecificStorageAccessIdentifier->frameID, m_frameSpecificStorageAccessIdentifier->pageID), 0);
        m_frameSpecificStorageAccessIdentifier = WTF::nullopt;
    }
#endif
}

void WebFrameLoaderClient::detachedFromParent3()
{
    notImplemented();
}

void WebFrameLoaderClient::assignIdentifierToInitialRequest(WebCore::ResourceLoaderIdentifier identifier, DocumentLoader* loader, const ResourceRequest& request)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

//    bool pageIsProvisionallyLoading = false;
//    if (FrameLoader* frameLoader = loader ? loader->frameLoader() : nullptr)
//        pageIsProvisionallyLoading = frameLoader->provisionalDocumentLoader() == loader;

//    webPage->injectedBundleResourceLoadClient().didInitiateLoadForResource(*webPage, *m_frame, identifier, request, pageIsProvisionallyLoading);
    webPage->addResourceRequest(identifier, request);
}

void WebFrameLoaderClient::dispatchWillSendRequest(DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
}

bool WebFrameLoaderClient::shouldUseCredentialStorage(DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier)
{
    return true;
}

void WebFrameLoaderClient::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, const AuthenticationChallenge& challenge)
{
    WebPage* webPage = m_frame->page();
    if (webPage && webPage->_fAuthChallenge)
    {
		if (webPage->_fAuthChallenge(challenge))
			return;
	}
//	dprintf("challenge at %s\n", challenge.failureResponse().url().string().utf8().data());
	challenge.authenticationClient()->receivedCancellation(challenge);
}

void WebFrameLoaderClient::dispatchDidReceiveResponse(DocumentLoader* loader, WebCore::ResourceLoaderIdentifier identifier, const ResourceResponse& response)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    D(dprintf("%s: url %s attach %d sr %d ismoved %d\n", __PRETTY_FUNCTION__, response.url().string().utf8().data(), response.isAttachment(), response.isRedirection(), response.isMovedPermanently()));

	bool hadAuth =loader->request().httpHeaderField(HTTPHeaderName::Authorization).length() > 0;

	if (webPage->_fDidReceiveResponse)
		webPage->_fDidReceiveResponse(response, hadAuth);
}

void WebFrameLoaderClient::dispatchDidReceiveContentLength(DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier, int dataLength)
{
}

void WebFrameLoaderClient::dispatchDidFinishLoading(DocumentLoader*, WebCore::ResourceLoaderIdentifier identifier)
{
    notImplemented();
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    D(dprintf("%s: \n", __PRETTY_FUNCTION__));

    webPage->removeResourceRequest(identifier);
}

void WebFrameLoaderClient::dispatchDidFailLoading(DocumentLoader*loader, WebCore::ResourceLoaderIdentifier identifier, const ResourceError& error)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    webPage->removeResourceRequest(identifier);
}

bool WebFrameLoaderClient::dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int /*length*/)
{
    notImplemented();
    return false;
}

void WebFrameLoaderClient::dispatchDidDispatchOnloadEvents()
{
    notImplemented();
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    // Notify the bundle client.
//    webPage->injectedBundleLoaderClient().didHandleOnloadEventsForFrame(*webPage, *m_frame);
}

void WebFrameLoaderClient::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
}

void WebFrameLoaderClient::dispatchDidChangeProvisionalURL()
{
	// do not send url updates from here!
}

void WebFrameLoaderClient::dispatchDidCancelClientRedirect()
{
    D(dprintf("%s: \n", __PRETTY_FUNCTION__));
}

void WebFrameLoaderClient::dispatchWillPerformClientRedirect(const URL& url, double interval, WallTime fireDate, LockBackForwardList lockBackForwardList)
{
}

void WebFrameLoaderClient::dispatchDidChangeLocationWithinPage()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));

	if (m_frame->isMainFrame() && webPage->_fChangedURL)
	{
		webPage->_fChangedURL(m_frame->coreFrame()->document()->url().string());
	}
	webPage->clearAutofillElements();
//    auto navigationID = static_cast<WebDocumentLoader&>(*m_frame->coreFrame()->loader().documentLoader()).navigationID();
}

void WebFrameLoaderClient::dispatchDidChangeMainDocument()
{
	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));

    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
	webPage->clearAutofillElements();

	m_didSendFormEvent = false;

/* NO! it's the old url there
	if (m_frame->isMainFrame() && webPage->_fChangedURL && m_frame->coreFrame() && m_frame->coreFrame()->document())
	{
		webPage->_fChangedURL(m_frame->coreFrame()->document()->url().string());
	}
*/
}

void WebFrameLoaderClient::dispatchWillChangeDocument(const URL& currentUrl, const URL& newUrl)
{
	D(dprintf("%s: mainfr %d '%s' > '%s'\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), currentUrl.string().utf8().data(), newUrl.string().utf8().data()));
// would change url for direct clicks into downloadable content
#if 0
	WebPage* webPage = m_frame->page();
    if (webPage)
	{
		if (m_frame->isMainFrame() && webPage->_fChangedURL)
		{
			webPage->_fChangedURL(newUrl.string());
		}
	}
#endif
}

void WebFrameLoaderClient::dispatchDidPushStateWithinPage()
{
	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));
	WebPage* webPage = m_frame->page();
    if (webPage)
	{
		if (m_frame->isMainFrame() && webPage->_fChangedURL)
		{
			webPage->_fChangedURL(m_frame->coreFrame()->document()->url().string());
		}
	}
}

void WebFrameLoaderClient::dispatchDidReplaceStateWithinPage()
{
	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));
	WebPage* webPage = m_frame->page();
    if (webPage)
	{
		if (m_frame->isMainFrame() && webPage->_fChangedURL)
		{
			webPage->_fChangedURL(m_frame->coreFrame()->document()->url().string());
		}
	}
}

void WebFrameLoaderClient::dispatchDidPopStateWithinPage()
{
	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));
	WebPage* webPage = m_frame->page();
    if (webPage)
	{
		if (m_frame->isMainFrame() && webPage->_fChangedURL)
		{
			webPage->_fChangedURL(m_frame->coreFrame()->document()->url().string());
		}
	}
}

void WebFrameLoaderClient::dispatchWillClose()
{
    notImplemented();
}

void WebFrameLoaderClient::dispatchDidExplicitOpen(const URL&, const String& /* mimeType */)
{
    notImplemented();

    auto* webPage = m_frame->page();
    if (!webPage)
        return;
}

void WebFrameLoaderClient::dispatchDidStartProvisionalLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

	D(dprintf("%s: frame ID %llu\n", __PRETTY_FUNCTION__, m_frame->frameID()));

#if ENABLE(FULLSCREEN_API)
    auto* document = m_frame->coreFrame()->document();
    if (document && document->fullscreenManager().fullscreenElement())
		webPage->exitFullscreen();
#endif
}

static constexpr unsigned maxTitleLength = 1000; // Closest power of 10 above the W3C recommendation for Title length.

void WebFrameLoaderClient::dispatchDidReceiveTitle(const StringWithDirection& title)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    auto truncatedTitle = truncateFromEnd(title, maxTitleLength);

	if (m_frame->isMainFrame() && webPage->_fChangedTitle)
		webPage->_fChangedTitle(truncatedTitle.string);

#if 0
    RefPtr<API::Object> userData;

    // Notify the bundle client.
    // FIXME: Use direction of title.
    webPage->injectedBundleLoaderClient().didReceiveTitleForFrame(*webPage, truncatedTitle.string, *m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidReceiveTitleForFrame(m_frame->frameID(), truncatedTitle.string, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get())));
#endif
}

void WebFrameLoaderClient::dispatchDidCommitLoad(std::optional<HasInsecureContent> hasInsecureContent, std::optional<WebCore::UsedLegacyTLS>, std::optional<WebCore::WasPrivateRelayed>)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    WebDocumentLoader& documentLoader = static_cast<WebDocumentLoader&>(*m_frame->coreFrame()->loader().documentLoader());

    D(dprintf("%s: hasInsecure %d '%s' main %d\n", __PRETTY_FUNCTION__, hasInsecureContent && hasInsecureContent.value() == HasInsecureContent::Yes, documentLoader.url().string().utf8().data(), m_frame->isMainFrame()));

	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));

    webPage->didCommitLoad(m_frame);
    m_didSendFormEvent = false;

	if (m_frame->isMainFrame() && webPage->_fChangedURL)
	{
		webPage->_fChangedURL(documentLoader.url().string());
	}
	
	if ((hasInsecureContent && hasInsecureContent.value() == HasInsecureContent::Yes) && m_frame->isMainFrame() && webPage->_fDidLoadInsecureContent)
	{
		webPage->_fDidLoadInsecureContent();
	}

	webPage->clearAutofillElements();

#if 0
    WebDocumentLoader& documentLoader = static_cast<WebDocumentLoader&>(*m_frame->coreFrame()->loader().documentLoader());
    RefPtr<API::Object> userData;

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didCommitLoadForFrame(*webPage, *m_frame, userData);

    webPage->sandboxExtensionTracker().didCommitProvisionalLoad(m_frame);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidCommitLoadForFrame(m_frame->frameID(), documentLoader.navigationID(), documentLoader.response().mimeType(), m_frameHasCustomContentProvider, static_cast<uint32_t>(m_frame->coreFrame()->loader().loadType()), valueOrCompute(documentLoader.response().certificateInfo(), [] { return CertificateInfo(); }), m_frame->coreFrame()->document()->isPluginDocument(), hasInsecureContent, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get())));
#endif
}

void WebFrameLoaderClient::dispatchDidFailProvisionalLoad(const ResourceError& error, WillContinueLoading willContinueLoading, WebCore::WillInternallyHandleFailure)
{
}

void WebFrameLoaderClient::dispatchDidFailLoad(const ResourceError& error)
{
}

void WebFrameLoaderClient::dispatchDidFinishDocumentLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    D(dprintf("%s: frame ID %llu\n", __PRETTY_FUNCTION__, m_frame->frameID()));
	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));

#if 0
    RefPtr<API::Object> userData;

    auto navigationID = static_cast<WebDocumentLoader&>(*m_frame->coreFrame()->loader().documentLoader()).navigationID();

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didFinishDocumentLoadForFrame(*webPage, *m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidFinishDocumentLoadForFrame(m_frame->frameID(), navigationID, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get())));
#endif

    webPage->didFinishDocumentLoad(m_frame);
}

void WebFrameLoaderClient::dispatchDidFinishLoad()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    D(dprintf("%s: frame ID %llu\n", __PRETTY_FUNCTION__, m_frame->frameID()));
	D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));

#if 0
    RefPtr<API::Object> userData;

    auto navigationID = static_cast<WebDocumentLoader&>(*m_frame->coreFrame()->loader().documentLoader()).navigationID();

    // Notify the bundle client.
    webPage->injectedBundleLoaderClient().didFinishLoadForFrame(*webPage, *m_frame, userData);

    // Notify the UIProcess.
    webPage->send(Messages::WebPageProxy::DidFinishLoadForFrame(m_frame->frameID(), navigationID, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get())));
#endif

    webPage->didFinishLoad(m_frame);
}

#if 0
void WebFrameLoaderClient::forcePageTransitionIfNeeded()
{
    if (m_didCompletePageTransition)
        return;

    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->didCompletePageTransition();
    m_didCompletePageTransition = true;
}
#endif

void WebFrameLoaderClient::dispatchDidReachLayoutMilestone(OptionSet<WebCore::LayoutMilestone> milestones)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

#if 0
    // Send this after DidFirstLayout-specific calls since some clients expect to get those messages first.
    webPage->dispatchDidReachLayoutMilestone(milestones);
#endif

    if (milestones & WebCore::LayoutMilestone::DidFirstVisuallyNonEmptyLayout) {
        if (m_frame->isMainFrame() && !m_didCompletePageTransition && !webPage->corePage()->settings().suppressesIncrementalRendering()) {
            RELEASE_LOG(Layout, "%p - WebFrameLoaderClient::dispatchDidReachLayoutMilestone: dispatching didCompletePageTransition, page = %p", this, webPage);
            webPage->didCompletePageTransition();
            m_didCompletePageTransition = true;
        }
    }
}

void WebFrameLoaderClient::dispatchDidLayout()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
#if 0
    if (m_frame == m_frame->page()->mainWebFrame()) {
        webPage->mainFrameDidLayout();
    }
#endif
}

LocalFrame* WebFrameLoaderClient::dispatchCreatePage(const NavigationAction& navigationAction, WebCore::NewFrameOpenerPolicy)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return nullptr;

    // Just call through to the chrome client.
    auto newPage = webPage->corePage()->chrome().createWindow(*m_frame->coreFrame(), { }, navigationAction);
    if (!newPage)
        return nullptr;
	
    return downcast<LocalFrame>(&newPage->mainFrame());
}

void WebFrameLoaderClient::dispatchShow()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

//    webPage->show();
}

void WebFrameLoaderClient::dispatchDecidePolicyForResponse(const ResourceResponse& response, const ResourceRequest& request, const String& downloadAttribute, WebCore::FramePolicyFunction&& function)
{
    WebPage* webPage = m_frame->page();

	D(dprintf("%s: '%s' isattach %d protocol %s shouldRedirect %d isMoved %d ir %d code %d\n", __PRETTY_FUNCTION__, request.url().string().utf8().data(), response.isAttachment(), request.url().protocol().toString().utf8().data(), response.isRedirection(), response.isMovedPermanently(), response.isRedirection(), response.httpStatusCode()));

    if (!webPage) {
    	D(dprintf("%s: ignore!\n", __PRETTY_FUNCTION__));
        function(WebCore::PolicyAction::Ignore);
        return;
    }

    if (!request.url().string()) {
        function(WebCore::PolicyAction::Use);
        return;
    }

	// undisplayable mime AND this is a top navigation - meaning the url the user clicked on or typed in
	if (response.isAttachment() || (!canShowMIMEType(response.mimeType()) && !response.mimeType().isNull() && request.isTopSite()))
	{
    	D(dprintf("%s: ask to download...\n", __PRETTY_FUNCTION__));
		// should we download this??
		if (webPage->_fDownloadAsk)
		{
			webPage->_fDownloadAsk(response, request, downloadAttribute, std::move(function));
			return;
		}
    	D(dprintf("%s: ignore post download\n", __PRETTY_FUNCTION__));
        function(PolicyAction::Ignore);
		return;
	}

   	D(dprintf("%s: use!\n", __PRETTY_FUNCTION__));
    function(PolicyAction::Use);
}

void WebFrameLoaderClient::dispatchDecidePolicyForNewWindowAction(const NavigationAction& navigationAction, const ResourceRequest& request,
    FormState* formState, const String& frameName, std::optional<WebCore::HitTestResult>&&, FramePolicyFunction&& function)
{
    WebPage* webPage = m_frame->page();
    if (!webPage) {
        function(PolicyAction::Ignore);
        return;
    }

	if (webPage && webPage->_fShouldNavigateToURL)
	{
		if (!webPage->_fShouldNavigateToURL(request.url(), true, true))
		{
            function(PolicyAction::Ignore);
			return;
		}
	}

    function(PolicyAction::Use);
}

void WebFrameLoaderClient::applyToDocumentLoader(WebsitePoliciesData&& websitePolicies)
{
    auto* coreFrame = m_frame->coreFrame();
    if (!coreFrame)
        return;

	notImplemented();
#if 0
    WebDocumentLoader* documentLoader = static_cast<WebDocumentLoader*>(coreFrame->loader().policyDocumentLoader());
    if (!documentLoader)
        documentLoader = static_cast<WebDocumentLoader*>(coreFrame->loader().provisionalDocumentLoader());
    if (!documentLoader)
        documentLoader = static_cast<WebDocumentLoader*>(coreFrame->loader().documentLoader());
    if (!documentLoader)
        return;

    WebsitePoliciesData::applyToDocumentLoader(WTFMove(websitePolicies), *documentLoader);
#endif
}

void WebFrameLoaderClient::dispatchDecidePolicyForNavigationAction(const WebCore::NavigationAction& navigationAction, const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& redirectResponse, WebCore::FormState* formState, const String&, std::optional<WebCore::NavigationIdentifier> navigationIdentifier, std::optional<WebCore::HitTestResult>&&, bool, WebCore::SandboxFlags, WebCore::PolicyDecisionMode, WebCore::FramePolicyFunction&& function)
{
    WebPage* webPage = m_frame->page();
    if (!webPage) {
        function(PolicyAction::Ignore);
        return;
    }

	D(dprintf("%s: %s, is download? %s empty %d protocol %s attach %d canshowmime %d topsite %d isred %d status %d\n", __PRETTY_FUNCTION__, request.url().string().utf8().data(), navigationAction.downloadAttribute().string().utf8().data(), request.isEmpty(), request.url().protocol().toString().utf8().data(), redirectResponse.isAttachment(), canShowMIMEType(redirectResponse.mimeType()), request.isTopSite(), redirectResponse.isRedirection(), redirectResponse.httpStatusCode()));

    if (webPage && webPage->_fShouldNavigateToURL)
    {
        if (!webPage->_fShouldNavigateToURL(request.url(), false, request.isTopSite()))
        {
            D(dprintf("%s: cancelled by handler\n", __PRETTY_FUNCTION__));
            function(PolicyAction::Ignore);
            return;
        }
    }

    notImplemented();
    LOG(Loading, "WebProcess %i - dispatchDecidePolicyForNavigationAction to request url %s", getCurrentProcessID(), request.url().string().utf8().data());

    // Always ignore requests with empty URLs. 
    if (request.isEmpty()) {
        function(PolicyAction::Ignore);
        return;
    }

	String protocol = request.url().protocol().toString();

	if (redirectResponse.isAttachment() || navigationAction.downloadAttribute().length())
	{
		// should we download this??
		if (webPage->_fDownloadAsk)
		{
			webPage->_fDownloadAsk(redirectResponse, request, navigationAction.downloadAttribute().string(), std::move(function));
			return;
		}
    	D(dprintf("%s: ignore post download\n", __PRETTY_FUNCTION__));
        function(PolicyAction::Ignore);
		return;
	}

    function(PolicyAction::Use);
}

void WebFrameLoaderClient::cancelPolicyCheck()
{
	notImplemented();
//    m_frame->invalidatePolicyListener();
}

void WebFrameLoaderClient::broadcastMainFrameURLChangeToOtherProcesses(const URL&)
{
// todo!
}

void WebFrameLoaderClient::dispatchUnableToImplementPolicy(const ResourceError& error)
{
}

void WebFrameLoaderClient::dispatchWillSendSubmitEvent(Ref<FormState>&& formState)
{
    WebPage* webPage = m_frame->page();
    if (!webPage) {
        return;
    }

	if (!m_didSendFormEvent && webPage->hasAutofillElements() && webPage->_fStoreAutofill)
	{
		WTF::String user, password;
		webPage->getAutofillElements(user, password);
		if (!password.isEmpty())
		{
			webPage->_fStoreAutofill(user, password);
		}
		m_didSendFormEvent = true;
	}
}

void WebFrameLoaderClient::dispatchWillSubmitForm(FormState& formState, CompletionHandler<void()>&& completionHandler)
{
    WebPage* webPage = m_frame->page();
    if (!webPage) {
        completionHandler();
        return;
    }

D(dprintf("%s: \n", __PRETTY_FUNCTION__));


#if 0
	auto& values = formState.textFieldValues();
	for (auto const &e : values)
	{
		dprintf("xx %s - %s\n", e.first.utf8().data(), e.second.utf8().data());
	}
#endif

	if (!m_didSendFormEvent && webPage->hasAutofillElements() && webPage->_fStoreAutofill)
	{
		WTF::String user, password;
		webPage->getAutofillElements(user, password);
		if (!password.isEmpty())
		{
			webPage->_fStoreAutofill(user, password);
		}
		m_didSendFormEvent = true;
	}

    completionHandler();
}

void WebFrameLoaderClient::revertToProvisionalState(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::setMainDocumentError(DocumentLoader *documentLoader, const ResourceError& error)
{
    WebPage* webPage = m_frame->page();
    if (webPage && m_frame->isMainFrame())
    {
		D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));

		if (webPage->_fChangedURL && documentLoader && error.type() != WebCore::ResourceErrorBase::Type::Cancellation && error.type() != WebCore::ResourceErrorBase::Type::Null)
		{
			WTF::String url = documentLoader->url().string();
			D(dprintf("%s: change url to %s\n", __PRETTY_FUNCTION__, url.utf8().data()));
			webPage->_fChangedURL(url);
		}

		webPage->didFailLoad(error);

		// happens if we click on download link and hit download... bit of a hack here
		if (error.type() == WebCore::ResourceErrorBase::Type::Null)
		{
			setMainFrameDocumentReady(true);
		}
		
		m_didSendFormEvent = false;
	}
}

void WebFrameLoaderClient::setMainFrameDocumentReady(bool ready)
{
	if (m_mainDocumentReady != ready)
	{
		D(dprintf("%s: mainfr %d %s\n", __PRETTY_FUNCTION__, m_frame->isMainFrame(), m_frame->coreFrame() && m_frame->coreFrame()->document() ? m_frame->coreFrame()->document()->url().string().utf8().data() : "?"));

   		WebPage* webPage = m_frame->page();
		m_mainDocumentReady = ready;
		if (webPage)
		{
			if (m_mainDocumentReady && webPage->_fDidStopLoading)
				webPage->_fDidStopLoading();
			else if (!m_mainDocumentReady && webPage->_fDidStartLoading)
				webPage->_fDidStartLoading();

/* // b0rks on errors and breaks error handling
			if (webPage->_fChangedURL && m_frame->isMainFrame())
			{
				webPage->_fChangedURL(m_frame->coreFrame()->document()->url().string());
			}
*/
		}
	}
}

void WebFrameLoaderClient::startDownload(const ResourceRequest& request, const String& suggestedName, FromDownloadAttribute)
{
	D(dprintf("%s: '%s'\n", __PRETTY_FUNCTION__, request.url().string().utf8().data()));
	m_frame->startDownload(request, suggestedName);
}

void WebFrameLoaderClient::willChangeTitle(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::didChangeTitle(DocumentLoader*)
{
    notImplemented();
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
}

void WebFrameLoaderClient::didRestoreFromBackForwardCache()
{
D(dprintf("%s: '%s' \n", __PRETTY_FUNCTION__, m_frame->coreFrame()->document()->url().string().utf8().data()));
	m_frameCameFromPageCache = true;
}

void WebFrameLoaderClient::willReplaceMultipartContent()
{
	notImplemented();
#if 0
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
    webPage->willReplaceMultipartContent(*m_frame);
#endif
}

void WebFrameLoaderClient::didReplaceMultipartContent()
{
	notImplemented();
#if 0
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
    webPage->didReplaceMultipartContent(*m_frame);
#endif
}

void WebFrameLoaderClient::committedLoad(DocumentLoader* loader, const WebCore::SharedBuffer& data)
{
//	notImplemented();

    loader->commitData(data);

    // If the document is a stand-alone media document, now is the right time to cancel the WebKit load.
    // FIXME: This code should be shared across all ports. <http://webkit.org/b/48762>.
    if (m_frame->coreFrame()->document()->isMediaDocument())
        loader->cancelMainResourceLoad(pluginWillHandleLoadError(loader->response()));
}

void WebFrameLoaderClient::finishedLoading(DocumentLoader* loader)
{
	notImplemented();
}

void WebFrameLoaderClient::updateGlobalHistory()
{
	notImplemented();

#if 0
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    DocumentLoader* loader = m_frame->coreFrame()->loader().documentLoader();

    WebNavigationDataStore data;
    data.url = loader->url().string();
    // FIXME: Use direction of title.
    data.title = loader->title().string;
    data.originalRequest = loader->originalRequestCopy();
    data.response = loader->response();

    webPage->send(Messages::WebPageProxy::DidNavigateWithNavigationData(data, m_frame->frameID()));
#endif
}

void WebFrameLoaderClient::updateGlobalHistoryRedirectLinks()
{
	notImplemented();
#if 0
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    DocumentLoader* loader = m_frame->coreFrame()->loader().documentLoader();
    ASSERT(loader->unreachableURL().isEmpty());

    // Client redirect
    if (!loader->clientRedirectSourceForHistory().isNull()) {
        webPage->send(Messages::WebPageProxy::DidPerformClientRedirect(
            loader->clientRedirectSourceForHistory(), loader->clientRedirectDestinationForHistory(), m_frame->frameID()));
    }

    // Server redirect
    if (!loader->serverRedirectSourceForHistory().isNull()) {
        webPage->send(Messages::WebPageProxy::DidPerformServerRedirect(
            loader->serverRedirectSourceForHistory(), loader->serverRedirectDestinationForHistory(), m_frame->frameID()));
    }
#endif
}

bool WebFrameLoaderClient::shouldGoToHistoryItem(HistoryItem& item) const
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return false;
    return true;
}

void WebFrameLoaderClient::didDisplayInsecureContent()
{
	D(dprintf("%s: !!!\n", __PRETTY_FUNCTION__));
    WebPage* webPage = m_frame->page();
    if (webPage)
		webPage->_fDidLoadInsecureContent();
}


void WebFrameLoaderClient::didRunInsecureContent(SecurityOrigin&)
{
	D(dprintf("%s: !!!\n", __PRETTY_FUNCTION__));
    WebPage* webPage = m_frame->page();
    if (webPage)
		webPage->_fDidLoadInsecureContent();
}

/* void WebFrameLoaderClient::didDetectXSS(const URL&, bool)
{
	D(dprintf("%s: !!!\n", __PRETTY_FUNCTION__));
    WebPage* webPage = m_frame->page();
    if (webPage)
		webPage->_fDidLoadInsecureContent();
} */

ResourceError WebFrameLoaderClient::cancelledError(const ResourceRequest& request) const
{
	notImplemented();
    return ResourceError();//WebKit::cancelledError(request);
}

ResourceError WebFrameLoaderClient::httpNavigationWithHTTPSOnlyError(const ResourceRequest&) const
{
    return { };
}

ResourceError WebFrameLoaderClient::blockedError(const ResourceRequest& request) const
{
	notImplemented();
    return ResourceError();//WebKit::blockedError(request);
}

ResourceError WebFrameLoaderClient::blockedByContentBlockerError(const ResourceRequest& request) const
{
	notImplemented();
    return ResourceError();//WebKit::blockedByContentBlockerError(request);
}

ResourceError WebFrameLoaderClient::cannotShowURLError(const ResourceRequest& request) const
{
	notImplemented();
    return ResourceError();//WebKit::cannotShowURLError(request);
}

ResourceError WebFrameLoaderClient::interruptedForPolicyChangeError(const ResourceRequest& request) const
{
	notImplemented();
    return ResourceError();//WebKit::interruptedForPolicyChangeError(request);
}

#if ENABLE(CONTENT_FILTERING)
ResourceError WebFrameLoaderClient::blockedByContentFilterError(const ResourceRequest& request) const
{
    return ResourceError();//WebKit::blockedByContentFilterError(request);
}
#endif

ResourceError WebFrameLoaderClient::cannotShowMIMETypeError(const ResourceResponse& response) const
{
	notImplemented();
    return ResourceError();//WebKit::cannotShowMIMETypeError(response);
}

ResourceError WebFrameLoaderClient::fileDoesNotExistError(const ResourceResponse& response) const
{
	notImplemented();
    return ResourceError();//WebKit::fileDoesNotExistError(response);
}

ResourceError WebFrameLoaderClient::pluginWillHandleLoadError(const ResourceResponse& response) const
{
    return ResourceError();//WebKit::pluginWillHandleLoadError(response);
}

WebCore::ResourceError WebFrameLoaderClient::httpsUpgradeRedirectLoopError(const WebCore::ResourceRequest&) const
{
    return ResourceError();
}

bool WebFrameLoaderClient::shouldFallBack(const ResourceError& error) const
{
    static NeverDestroyed<const ResourceError> cancelledError(this->cancelledError(ResourceRequest()));
    static NeverDestroyed<const ResourceError> pluginWillHandleLoadError(this->pluginWillHandleLoadError(ResourceResponse()));

    if (error.errorCode() == cancelledError.get().errorCode() && error.domain() == cancelledError.get().domain())
        return false;

    if (error.errorCode() == pluginWillHandleLoadError.get().errorCode() && error.domain() == pluginWillHandleLoadError.get().domain())
        return false;

    return true;
}

bool WebFrameLoaderClient::canHandleRequest(const ResourceRequest& request) const
{
    WebPage* webPage = m_frame->page();

	if (webPage && webPage->_fCanHandleRequest)
	{
		if (!webPage->_fCanHandleRequest(request))
			return false;
	}

    return true;
}

bool WebFrameLoaderClient::canShowMIMEType(const String& mimeType) const
{
    bool canShow = mimeType.isNull()
        || MIMETypeRegistry::isSupportedImageMIMEType(mimeType)
        || MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType);
//        || MIMETypeRegistry::isSupportedMediaMIMEType(mimeType); // player can't play stuff directly atm

    D(dprintf("%s: %s %d\n", __PRETTY_FUNCTION__, mimeType.utf8().data(), canShow));

    return canShow;
}

bool WebFrameLoaderClient::canShowMIMETypeAsHTML(const String& mimeType) const
{
    return MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType);
}

bool WebFrameLoaderClient::representationExistsForURLScheme(WTF::StringView /*URLScheme*/) const
{
    D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    notImplemented();
    return false;
}

String WebFrameLoaderClient::generatedMIMETypeForURLScheme(WTF::StringView /*URLScheme*/) const
{
    notImplemented();
    return String();
}

void WebFrameLoaderClient::frameLoadCompleted()
{
    // Note: Can be called multiple times.
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    if (m_frame->isMainFrame() && !m_didCompletePageTransition) {
        webPage->didCompletePageTransition();
        m_didCompletePageTransition = true;
    }
}

void WebFrameLoaderClient::saveViewStateToItem(HistoryItem& historyItem)
{
}

void WebFrameLoaderClient::restoreViewState()
{
    // Inform the UI process of the scale factor.
  //  double scaleFactor = m_frame->coreFrame()->loader().history().currentItem()->pageScaleFactor();

    // A scale factor of 0 means the history item has the default scale factor, thus we do not need to update it.
//    if (scaleFactor)
//        m_frame->page()->send(Messages::WebPageProxy::PageScaleFactorDidChange(scaleFactor));

	notImplemented();
#if 0
    // FIXME: This should not be necessary. WebCore should be correctly invalidating
    // the view on restores from the back/forward cache.
    if (m_frame->page() && m_frame == m_frame->page()->mainWebFrame())
        m_frame->page()->drawingArea()->setNeedsDisplay();
#endif
}

void WebFrameLoaderClient::provisionalLoadStarted()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    if (m_frame->isMainFrame()) {
        webPage->didStartPageTransition();
        m_didCompletePageTransition = false;
    }
}

void WebFrameLoaderClient::didFinishLoad()
{
}

void WebFrameLoaderClient::prepareForDataSourceReplacement()
{
    notImplemented();
}

Ref<DocumentLoader> WebFrameLoaderClient::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    return m_frame->page()->createDocumentLoader(*m_frame->coreFrame(), request, substituteData);
}

void WebFrameLoaderClient::updateCachedDocumentLoader(WebCore::DocumentLoader& loader)
{
    m_frame->page()->updateCachedDocumentLoader(static_cast<WebDocumentLoader&>(loader), *m_frame->coreFrame());
}

void WebFrameLoaderClient::setTitle(const StringWithDirection& title, const URL& url)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;
	
	// this is not an actual titlebar change it seems
	notImplemented();
}

String WebFrameLoaderClient::userAgent(const URL& url) const
{
    auto* webPage = m_frame->page();

    if (webPage && webPage->_fUserAgentForURL)
    {
        auto protocol = url.protocol();
        
        if (protocol == "about"_s || protocol == "blob"_s || protocol == "data"_s)
        {
            // return default and don't pass it to the client...
        }
        else
        {
            WTF::String urlBase = url.truncatedForUseAsBase().string();
            WTF::String out = webPage->_fUserAgentForURL(urlBase);
            return out;
        }
	}

 	return String("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15"_s);
}

String WebFrameLoaderClient::overrideContentSecurityPolicy() const
{
    return String();
}

void WebFrameLoaderClient::savePlatformDataToCachedFrame(CachedFrame* cachedFrame)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

//    HasInsecureContent hasInsecureContent;
//    if (webPage->sendSync(Messages::WebPageProxy::HasInsecureContent(), Messages::WebPageProxy::HasInsecureContent::Reply(hasInsecureContent)))
//    cachedFrame->setHasInsecureContent(hasInsecureContent);
}

void WebFrameLoaderClient::transitionToCommittedFromCachedFrame(CachedFrame*)
{
//    const ResourceResponse& response = m_frame->coreFrame()->loader().documentLoader()->response();
    m_frameCameFromPageCache = true;
}

void WebFrameLoaderClient::transitionToCommittedForNewPage(InitializingIframe)
{
    WebPage* webPage = m_frame->page();

    bool isMainFrame = m_frame->isMainFrame();
    bool shouldUseFixedLayout = false;//isMainFrame;// && webPage->useFixedLayout();
    bool shouldDisableScrolling = !isMainFrame; //isMainFrame && !webPage->mainFrameIsScrollable();
    bool shouldHideScrollbars = shouldDisableScrolling || isMainFrame;
    auto psize = webPage->size();
    IntRect fixedVisibleContentRect = webPage->bounds();

    // force rendering tree destruction now... macOS seems to do just that
    auto* coreFrame = m_frame->coreFrame();
    if (isMainFrame && coreFrame->view())
        coreFrame->view()->setParentVisible(false);
    coreFrame->setView(nullptr);

    // const ResourceResponse& response = m_frame->coreFrame()->loader().documentLoader()->response();
    m_frameCameFromPageCache = false;

    ScrollbarMode defaultScrollbarMode = shouldHideScrollbars ? ScrollbarMode::AlwaysOff : ScrollbarMode::Auto;

    ScrollbarMode horizontalScrollbarMode = webPage->alwaysShowsHorizontalScroller() ? ScrollbarMode::AlwaysOn : defaultScrollbarMode;
    ScrollbarMode verticalScrollbarMode = webPage->alwaysShowsVerticalScroller() ? ScrollbarMode::AlwaysOn : defaultScrollbarMode;

    bool horizontalLock = shouldHideScrollbars || webPage->alwaysShowsHorizontalScroller();
    bool verticalLock = shouldHideScrollbars || webPage->alwaysShowsVerticalScroller();

//dprintf("%s: rect size %d %d, fixedl %d mainf %d %d %d vsmode %d\n", __PRETTY_FUNCTION__, fixedVisibleContentRect.width(),
//fixedVisibleContentRect.height(),shouldUseFixedLayout, isMainFrame, horizontalLock, verticalLock, int(verticalScrollbarMode));

    m_frame->coreFrame()->createView(psize, webPage->backgroundColor(),
        psize, fixedVisibleContentRect, shouldUseFixedLayout,
        horizontalScrollbarMode, horizontalLock, verticalScrollbarMode, verticalLock);

if (isMainFrame)
{
//m_frame->coreFrame()->view()->enableAutoSizeMode(true, { psize.width(), psize.height() });
m_frame->coreFrame()->view()->resize(psize.width(), psize.height());
m_frame->coreFrame()->view()->setVisualUpdatesAllowedByClient(true);
//m_frame->coreFrame()->view()->setViewportSizeForCSSViewportUnits(psize);
}
// TODO:
#if 0
    if (int viewLayoutWidth = webPage->viewLayoutSize().width()) {
        int viewLayoutHeight = std::max(webPage->viewLayoutSize().height(), 1);
        m_frame->coreFrame()->view()->enableAutoSizeMode(true, { viewLayoutWidth, viewLayoutHeight });

        if (webPage->autoSizingShouldExpandToViewHeight())
            m_frame->coreFrame()->view()->setAutoSizeFixedMinimumHeight(webPage->size().height());
    }

    if (auto viewportSizeForViewportUnits = webPage->viewportSizeForCSSViewportUnits())
        m_frame->coreFrame()->view()->setViewportSizeForCSSViewportUnits(*viewportSizeForViewportUnits);
    m_frame->coreFrame()->view()->setProhibitsScrolling(shouldDisableScrolling);
    m_frame->coreFrame()->view()->setVisualUpdatesAllowedByClient(!webPage->shouldExtendIncrementalRenderingSuppression());

    if (webPage->scrollPinningBehavior() != DoNotPin)
        m_frame->coreFrame()->view()->setScrollPinningBehavior(webPage->scrollPinningBehavior());
#endif
}

bool WebFrameLoaderClient::canCachePage() const
{
    return true;
}

void WebFrameLoaderClient::convertMainResourceLoadToDownload(DocumentLoader *documentLoader, const ResourceRequest& request, const ResourceResponse& response)
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    m_frame->convertMainResourceLoadToDownload(documentLoader, request, response);
}

RefPtr<LocalFrame> WebFrameLoaderClient::createFrame(const WTF::AtomString& name, HTMLFrameOwnerElement&ownerElement)
{
    auto* webPage = m_frame->page();

    auto subframe = WebFrame::createSubframe(webPage, name, &ownerElement);
    auto* coreSubframe = subframe->coreFrame();
    if (!coreSubframe)
        return nullptr;

    // The creation of the frame may have run arbitrary JavaScript that removed it from the page already.
    if (!coreSubframe->page())
        return nullptr;

#if 0
    m_frame->coreFrame()->loader().loadURLIntoChildFrame(url, referrer, coreSubframe);

    // The frame's onload handler may have removed it from the document.
    if (!subframe->coreFrame())
        return nullptr;
    ASSERT(subframe->coreFrame() == coreSubframe);
    if (!coreSubframe->tree().parent())
        return nullptr;
#endif

    return coreSubframe;
}

ObjectContentType WebFrameLoaderClient::objectContentType(const URL& url, const String& mimeTypeIn)
{
    // FIXME: This should eventually be merged with WebCore::FrameLoader::defaultObjectContentType.

    String mimeType = mimeTypeIn;
    if (mimeType.isEmpty()) {
        String path = url.path().toString();
        auto dotPosition = path.reverseFind('.');
        if (dotPosition == notFound)
            return ObjectContentType::Frame;
        String extension = path.substring(dotPosition + 1).convertToASCIILowercase();

        // Try to guess the MIME type from the extension.
        mimeType = MIMETypeRegistry::mimeTypeForExtension(extension);
    }

	D(dprintf("%s: mime %s image %d nonimage %d\n", __PRETTY_FUNCTION__, mimeType.utf8().data(), MIMETypeRegistry::isSupportedImageMIMEType(mimeType),MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType)));

    if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return ObjectContentType::Image;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return ObjectContentType::Frame;

    return ObjectContentType::None;
}

AtomString WebFrameLoaderClient::overrideMediaType() const
{
    notImplemented();
    return WTF::nullAtom();
}

void WebFrameLoaderClient::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld& world)
{
}

void WebFrameLoaderClient::dispatchGlobalObjectAvailable(DOMWrapperWorld& world)
{
}

void WebFrameLoaderClient::willInjectUserScript(DOMWrapperWorld& world)
{
}

void WebFrameLoaderClient::dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(WebCore::DOMWindowExtension* extension)
{
}

void WebFrameLoaderClient::dispatchDidReconnectDOMWindowExtensionToGlobalObject(WebCore::DOMWindowExtension* extension)
{
}

void WebFrameLoaderClient::dispatchWillDestroyGlobalObjectForDOMWindowExtension(WebCore::DOMWindowExtension* extension)
{
}

void WebFrameLoaderClient::didChangeScrollOffset()
{
}

bool WebFrameLoaderClient::allowScript(bool enabledPerSettings)
{
    if (!enabledPerSettings)
        return false;

	return true;
}

bool WebFrameLoaderClient::shouldForceUniversalAccessFromLocalURL(const URL& url)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return false;
notImplemented();
	return true;
}

Ref<FrameNetworkingContext> WebFrameLoaderClient::createNetworkingContext()
{
    return WebFrameNetworkingContext::create(m_frame->coreFrame());
}

#if ENABLE(CONTENT_FILTERING)

void WebFrameLoaderClient::contentFilterDidBlockLoad(WebCore::ContentFilterUnblockHandler unblockHandler)
{
    if (!unblockHandler.needsUIProcess()) {
        m_frame->coreFrame()->loader().policyChecker().setContentFilterUnblockHandler(WTFMove(unblockHandler));
        return;
    }

    if (WebPage* webPage { m_frame->page() })
        webPage->send(Messages::WebPageProxy::ContentFilterDidBlockLoadForFrame(unblockHandler, m_frame->frameID()));
}

#endif

void WebFrameLoaderClient::prefetchDNS(const String& hostname)
{
	notImplemented();
//    WebProcess::singleton().prefetchDNS(hostname);
}

void WebFrameLoaderClient::sendH2Ping(const URL& url, CompletionHandler<void(Expected<Seconds, WebCore::ResourceError>&&)>&&completionHandler)
{
	completionHandler(makeUnexpected(WebCore::internalError(url)));
}

void WebFrameLoaderClient::didRestoreScrollPosition()
{
}

String generateFileNameForIcon(const WTF::String &inHost)
{
    CString url(inHost.utf8());

    MD5 md5;
    md5.addBytes(reinterpret_cast<const uint8_t*>(url.data()), url.length());

    MD5::Digest sum;
    md5.checksum(sum);
    uint8_t* rawdata = sum.data();

    StringBuilder baseNameBuilder;
    for (size_t i = 0; i < MD5::hashSize; i++)
        baseNameBuilder.append(WTF::hex(rawdata[i], WTF::Lowercase));
    return makeString("PROGDIR:Cache/FavIcons/"_s, baseNameBuilder.toString());
}

void WebFrameLoaderClient::getLoadDecisionForIcons(const Vector<std::pair<WebCore::LinkIcon&, uint64_t>>& icons)
{
	WebPage* webPage = m_frame->page();
	auto* documentLoader = m_frame->coreFrame()->loader().documentLoader();
	const String fileName(generateFileNameForIcon(documentLoader->url().host().toString()));

	if (webPage && webPage->_fFavIconLoad && !webPage->_fFavIconLoad(documentLoader->url()))
	{
		for (auto& icon : icons)
			documentLoader->didGetLoadDecisionForIcon(false, icon.second, [](FragmentedSharedBuffer*data){});
		return;
	}

	{
		RefPtr<WebCore::SharedBuffer> buffer = WebCore::SharedBuffer::createWithContentsOfFile(fileName);
		if (buffer.get())
		{
			finishedLoadingIcon(buffer.get());

			for (auto& icon : icons)
				documentLoader->didGetLoadDecisionForIcon(false, icon.second, [](FragmentedSharedBuffer*data){});
			
			return;
		}
	}

	for (auto& icon : icons)
	{
		if (icon.first.type == WebCore::LinkIconType::Favicon)
		{
			documentLoader->didGetLoadDecisionForIcon(true, icon.second, [this](FragmentedSharedBuffer*data) {
				finishedLoadingIcon(data);
			});
		}
		else
		{
			documentLoader->didGetLoadDecisionForIcon(false, icon.second, [](FragmentedSharedBuffer*data){});
		}
	}
}

void WebFrameLoaderClient::finishedLoadingIcon(WebCore::FragmentedSharedBuffer* fragmentedData)
{
    RefPtr<SharedBuffer> data;
    if (fragmentedData)
        data = fragmentedData->makeContiguous();
    else
        return;

	auto* documentLoader = m_frame->coreFrame()->loader().documentLoader();
	if (!!data && data->size() > 0)
	{
		const String fileName(generateFileNameForIcon(documentLoader->url().host().toString()));
		WTF::FileSystemImpl::PlatformFileHandle file = WTF::FileSystemImpl::openFile(fileName, WTF::FileSystemImpl::FileOpenMode::Truncate);
		if (file != WTF::FileSystemImpl::invalidPlatformFileHandle)
		{
			if (int(data->size()) != WTF::FileSystemImpl::writeToFile(file, data->span()))
			{
				WTF::FileSystemImpl::closeFile(file);
				WTF::FileSystemImpl::deleteFile(fileName);
			}
			else
			{
				WTF::FileSystemImpl::closeFile(file);
			}
		}
	}

	if (!!data && data->size())
	{
		WebPage* webPage = m_frame->page();
		if (webPage && webPage->_fFavIconLoaded)
			webPage->_fFavIconLoaded(WTFMove(data), documentLoader->url());
	}
}

#if ENABLE(APPLICATION_MANIFEST)
void WebFrameLoaderClient::finishedLoadingApplicationManifest(uint64_t, const std::optional<WebCore::ApplicationManifest>&)
{
	notImplemented();
}
#endif

} // namespace WebKit
