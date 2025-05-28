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
#include <WebCore/Frame.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/FrameView.h>
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

std::optional<PageIdentifier> WebFrameLoaderClient::pageID() const
{
    if (m_frame->page())
        return m_frame->page()->pageID();

    return std::nullopt;
}

std::optional<WebCore::FrameIdentifier> WebFrameLoaderClient::frameID() const
{
    return WebCore::FrameIdentifier(m_frame->frameID());
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

void WebFrameLoaderClient::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader* loader, const ResourceRequest& request)
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

void WebFrameLoaderClient::dispatchWillSendRequest(DocumentLoader*, unsigned long identifier, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
}

bool WebFrameLoaderClient::shouldUseCredentialStorage(DocumentLoader*, unsigned long identifier)
{
    return true;
}

void WebFrameLoaderClient::dispatchDidReceiveAuthenticationChallenge(DocumentLoader*, unsigned long, const AuthenticationChallenge& challenge)
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

void WebFrameLoaderClient::dispatchDidReceiveResponse(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& response)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

	bool hadAuth =loader->request().httpHeaderField(HTTPHeaderName::Authorization).length() > 0;

	if (webPage->_fDidReceiveResponse)
		webPage->_fDidReceiveResponse(response, hadAuth);
}

void WebFrameLoaderClient::dispatchDidReceiveContentLength(DocumentLoader*, unsigned long identifier, int dataLength)
{
}

void WebFrameLoaderClient::dispatchDidFinishLoading(DocumentLoader*, unsigned long identifier)
{
    notImplemented();
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

    webPage->removeResourceRequest(identifier);
}

void WebFrameLoaderClient::dispatchDidFailLoading(DocumentLoader*loader, unsigned long identifier, const ResourceError& error)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

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
    Element* documentElement = m_frame->coreFrame()->document()->documentElement();
    if (documentElement && documentElement->containsFullScreenElement())
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

void WebFrameLoaderClient::dispatchDidCommitLoad(std::optional<HasInsecureContent> hasInsecureContent, std::optional<WebCore::UsedLegacyTLS>)
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

void WebFrameLoaderClient::dispatchDidFailProvisionalLoad(const ResourceError& error, WillContinueLoading willContinueLoading)
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

    if (milestones & DidFirstVisuallyNonEmptyLayout) {
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

Frame* WebFrameLoaderClient::dispatchCreatePage(const NavigationAction& navigationAction, WebCore::NewFrameOpenerPolicy)
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return nullptr;

    // Just call through to the chrome client.
    Page* newPage = webPage->corePage()->chrome().createWindow(*m_frame->coreFrame(), { }, navigationAction);
    if (!newPage)
        return nullptr;
	
    return &newPage->mainFrame();
}

void WebFrameLoaderClient::dispatchShow()
{
    WebPage* webPage = m_frame->page();
    if (!webPage)
        return;

//    webPage->show();
}

void WebFrameLoaderClient::dispatchDecidePolicyForResponse(const ResourceResponse& response, const ResourceRequest& request, WebCore::PolicyCheckIdentifier identifier, const String& downloadAttribute, WebCore::BrowsingContextGroupSwitchDecision decision, WebCore::FramePolicyFunction&& function)
{
    WebPage* webPage = m_frame->page();

	D(dprintf("%s: '%s' isattach %d\n", __PRETTY_FUNCTION__, request.url().string().utf8().data(), response.isAttachment()));

    if (!webPage) {
    	D(dprintf("%s: ignore!\n", __PRETTY_FUNCTION__));
        function(PolicyAction::Ignore, identifier);
        return;
    }

/* ?? not sure this makes any sense?
    if (!request.url().string()) {
    	D(dprintf("%s: use!\n", __PRETTY_FUNCTION__));
        function(PolicyAction::Use, identifier);
        return;
    } */

	// undisplayable mime AND this is a top navigation - meaning the url the user clicked on or typed in
	if (response.isAttachment() || (!canShowMIMEType(response.mimeType()) && request.isTopSite()))
	{
		// should we download this??
		if (webPage->_fDownloadAsk)
		{
			webPage->_fDownloadAsk(response, request, identifier, downloadAttribute, std::move(function));
			return;
		}
    	D(dprintf("%s: ignore post download\n", __PRETTY_FUNCTION__));
		function(PolicyAction::Ignore, identifier);
	}

   	D(dprintf("%s: use!\n", __PRETTY_FUNCTION__));
	function(PolicyAction::Use, identifier);
}

void WebFrameLoaderClient::dispatchDecidePolicyForNewWindowAction(const NavigationAction& navigationAction, const ResourceRequest& request,
    FormState* formState, const String& frameName, WebCore::PolicyCheckIdentifier identifier, FramePolicyFunction&& function)
{
    WebPage* webPage = m_frame->page();
    if (!webPage) {
        function(PolicyAction::Ignore, identifier);
        return;
    }

	if (webPage && webPage->_fShouldNavigateToURL)
	{
		if (!webPage->_fShouldNavigateToURL(request.url(), true))
		{
        	function(PolicyAction::Ignore, identifier);
			return;
		}
	}

	notImplemented();
	function(PolicyAction::Use, identifier);
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

void WebFrameLoaderClient::dispatchDecidePolicyForNavigationAction(const NavigationAction& navigationAction, const ResourceRequest& request, const ResourceResponse& redirectResponse,
    FormState* formState, PolicyDecisionMode policyDecisionMode, WebCore::PolicyCheckIdentifier requestIdentifier, FramePolicyFunction&& function)
{
    WebPage* webPage = m_frame->page();
    if (!webPage) {
        function(PolicyAction::Ignore, requestIdentifier);
        return;
    }

       if (webPage && webPage->_fShouldNavigateToURL)
       {
               if (!webPage->_fShouldNavigateToURL(request.url(), false))
               {
               function(PolicyAction::Ignore, requestIdentifier);
                       return;
               }
       }

    notImplemented();
    LOG(Loading, "WebProcess %i - dispatchDecidePolicyForNavigationAction to request url %s", getCurrentProcessID(), request.url().string().utf8().data());

    // Always ignore requests with empty URLs. 
    if (request.isEmpty()) {
        function(PolicyAction::Ignore, requestIdentifier);
        return;
    }

	function(PolicyAction::Use, requestIdentifier);

#if 0
    RefPtr<API::Object> userData;

    Ref<InjectedBundleNavigationAction> action = InjectedBundleNavigationAction::create(m_frame, navigationAction, formState);

    // Notify the bundle client.
    WKBundlePagePolicyAction policy = webPage->injectedBundlePolicyClient().decidePolicyForNavigationAction(webPage, m_frame, action.ptr(), request, userData);
    if (policy == WKBundlePagePolicyActionUse) {
        function(PolicyAction::Use, requestIdentifier);
        return;
    }

    uint64_t listenerID = m_frame->setUpPolicyListener(requestIdentifier, WTFMove(function), WebFrame::ForNavigationAction::Yes);

    ASSERT(navigationAction.requester());
    auto requester = navigationAction.requester().value();

    FrameInfoData originatingFrameInfoData;
    originatingFrameInfoData.isMainFrame = navigationAction.initiatedByMainFrame() == InitiatedByMainFrame::Yes;
    originatingFrameInfoData.request = ResourceRequest { requester.url() };
    originatingFrameInfoData.securityOrigin = requester.securityOrigin().data();
    if (requester.frameID() && WebProcess::singleton().webFrame(requester.frameID()))
        originatingFrameInfoData.frameID = requester.frameID();

    std::optional<PageIdentifier> originatingPageID;
    if (requester.pageID() && WebProcess::singleton().webPage(requester.pageID()))
        originatingPageID = requester.pageID();

    NavigationActionData navigationActionData;
    navigationActionData.navigationType = action->navigationType();
    navigationActionData.modifiers = action->modifiers();
    navigationActionData.mouseButton = action->mouseButton();
    navigationActionData.syntheticClickType = action->syntheticClickType();
    navigationActionData.clickLocationInRootViewCoordinates = action->clickLocationInRootViewCoordinates();
    navigationActionData.userGestureTokenIdentifier = WebProcess::singleton().userGestureTokenIdentifier(navigationAction.userGestureToken());
    navigationActionData.canHandleRequest = webPage->canHandleRequest(request);
    navigationActionData.shouldOpenExternalURLsPolicy = navigationAction.shouldOpenExternalURLsPolicy();
    navigationActionData.downloadAttribute = navigationAction.downloadAttribute();
    navigationActionData.isRedirect = !redirectResponse.isNull();
    navigationActionData.treatAsSameOriginNavigation = navigationAction.treatAsSameOriginNavigation();
    navigationActionData.hasOpenedFrames = navigationAction.hasOpenedFrames();
    navigationActionData.openedByDOMWithOpener = navigationAction.openedByDOMWithOpener();
    if (auto& requester = navigationAction.requester())
        navigationActionData.requesterOrigin = requester->securityOrigin().data();
    navigationActionData.targetBackForwardItemIdentifier = navigationAction.targetBackForwardItemIdentifier();
    navigationActionData.sourceBackForwardItemIdentifier = navigationAction.sourceBackForwardItemIdentifier();
    navigationActionData.lockHistory = navigationAction.lockHistory();
    navigationActionData.lockBackForwardList = navigationAction.lockBackForwardList();
    navigationActionData.adClickAttribution = navigationAction.adClickAttribution();

    WebCore::Frame* coreFrame = m_frame->coreFrame();
    if (!coreFrame)
        return function(PolicyAction::Ignore, requestIdentifier);
    WebDocumentLoader* documentLoader = static_cast<WebDocumentLoader*>(coreFrame->loader().policyDocumentLoader());
    if (!documentLoader) {
        // FIXME: When we receive a redirect after the navigation policy has been decided for the initial request,
        // the provisional load's DocumentLoader needs to receive navigation policy decisions. We need a better model for this state.
        documentLoader = static_cast<WebDocumentLoader*>(coreFrame->loader().provisionalDocumentLoader());
    }
    if (!documentLoader)
        documentLoader = static_cast<WebDocumentLoader*>(coreFrame->loader().documentLoader());

    navigationActionData.clientRedirectSourceForHistory = documentLoader->clientRedirectSourceForHistory();

    // Notify the UIProcess.
    Ref<WebFrame> protect(m_frame);

    if (policyDecisionMode == PolicyDecisionMode::Synchronous) {
        uint64_t newNavigationID;
        WebCore::PolicyCheckIdentifier responseIdentifier;
        PolicyAction policyAction;
        DownloadID downloadID;
        std::optional<WebsitePoliciesData> websitePolicies;

        if (!webPage->sendSync(Messages::WebPageProxy::DecidePolicyForNavigationActionSync(m_frame->frameID(), m_frame->isMainFrame(), SecurityOriginData::fromFrame(coreFrame),
            requestIdentifier, documentLoader->navigationID(), navigationActionData, originatingFrameInfoData, originatingPageID, navigationAction.resourceRequest(), request,
            IPC::FormDataReference { request.httpBody() }, redirectResponse, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get())),
            Messages::WebPageProxy::DecidePolicyForNavigationActionSync::Reply(responseIdentifier, policyAction, newNavigationID, downloadID, websitePolicies))) {
            m_frame->didReceivePolicyDecision(listenerID, requestIdentifier, PolicyAction::Ignore, 0, { }, { });
            return;
        }

        m_frame->didReceivePolicyDecision(listenerID, responseIdentifier, policyAction, 0, downloadID, { });
        return;
    }

    ASSERT(policyDecisionMode == PolicyDecisionMode::Asynchronous);
    if (!webPage->send(Messages::WebPageProxy::DecidePolicyForNavigationActionAsync(m_frame->frameID(), SecurityOriginData::fromFrame(coreFrame),
        requestIdentifier, documentLoader->navigationID(), navigationActionData, originatingFrameInfoData, originatingPageID, navigationAction.resourceRequest(), request,
        IPC::FormDataReference { request.httpBody() }, redirectResponse, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get()), listenerID)))
        m_frame->didReceivePolicyDecision(listenerID, requestIdentifier, PolicyAction::Ignore, 0, { }, { });
#endif
}

void WebFrameLoaderClient::cancelPolicyCheck()
{
	notImplemented();
//    m_frame->invalidatePolicyListener();
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

	notImplemented();
	completionHandler();
#if 0
    auto& form = formState.form();

    auto* sourceCoreFrame = formState.sourceDocument().frame();
    if (!sourceCoreFrame)
        return completionHandler();
    auto* sourceFrame = WebFrame::fromCoreFrame(*sourceCoreFrame);
    if (!sourceFrame)
        return completionHandler();

    auto& values = formState.textFieldValues();

    RefPtr<API::Object> userData;
    webPage->injectedBundleFormClient().willSubmitForm(webPage, &form, m_frame, sourceFrame, values, userData);

    uint64_t listenerID = m_frame->setUpWillSubmitFormListener(WTFMove(completionHandler));

    webPage->send(Messages::WebPageProxy::WillSubmitForm(m_frame->frameID(), sourceFrame->frameID(), values, listenerID, UserData(WebProcess::singleton().transformObjectsToHandles(userData.get()).get())));
#endif
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

void WebFrameLoaderClient::startDownload(const ResourceRequest& request, const String& suggestedName)
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

void WebFrameLoaderClient::committedLoad(DocumentLoader* loader, const uint8_t* data, int length)
{
	notImplemented();

    loader->commitData(data, length);

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


void WebFrameLoaderClient::didRunInsecureContent(SecurityOrigin&, const URL&)
{
	D(dprintf("%s: !!!\n", __PRETTY_FUNCTION__));
    WebPage* webPage = m_frame->page();
    if (webPage)
		webPage->_fDidLoadInsecureContent();
}

void WebFrameLoaderClient::didDetectXSS(const URL&, bool)
{
	D(dprintf("%s: !!!\n", __PRETTY_FUNCTION__));
    WebPage* webPage = m_frame->page();
    if (webPage)
		webPage->_fDidLoadInsecureContent();
}

ResourceError WebFrameLoaderClient::cancelledError(const ResourceRequest& request) const
{
	notImplemented();
    return ResourceError();//WebKit::cancelledError(request);
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
    bool canShow = MIMETypeRegistry::isSupportedImageMIMEType(mimeType)
        || MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType)
        || MIMETypeRegistry::isSupportedMediaMIMEType(mimeType);
// dprintf("%s: %s %d\n", __PRETTY_FUNCTION__, mimeType.utf8().data(), canShow);

    return canShow;
}

bool WebFrameLoaderClient::canShowMIMETypeAsHTML(const String& mimeType) const
{
    return MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType);
}

bool WebFrameLoaderClient::representationExistsForURLScheme(const String& /*URLScheme*/) const
{
    notImplemented();
    return false;
}

String WebFrameLoaderClient::generatedMIMETypeForURLScheme(const String& /*URLScheme*/) const
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
    	WTF::String urlBase = url.truncatedForUseAsBase().string();
    	WTF::String out = webPage->_fUserAgentForURL(urlBase);
    	return out;
	}

 	// return String("Mozilla/5.0 (MorphOS; PowerPC 3_14) WebKitty/605.1.15 (KHTML, like Gecko)");

	// Chrome
	// return String("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.100 Safari/537.36");

 	return String("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.1 Safari/605.1.15");
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

void WebFrameLoaderClient::transitionToCommittedForNewPage()
{
    WebPage* webPage = m_frame->page();

    bool isMainFrame = m_frame->isMainFrame();
    bool shouldUseFixedLayout = false;//isMainFrame;// && webPage->useFixedLayout();
    bool shouldDisableScrolling = !isMainFrame; //isMainFrame && !webPage->mainFrameIsScrollable();
    bool shouldHideScrollbars = shouldDisableScrolling || isMainFrame;
    auto psize = webPage->size();
    IntRect fixedVisibleContentRect = webPage->bounds();

    // const ResourceResponse& response = m_frame->coreFrame()->loader().documentLoader()->response();
    m_frameCameFromPageCache = false;

    ScrollbarMode defaultScrollbarMode = shouldHideScrollbars ? ScrollbarAlwaysOff : ScrollbarAuto;

    ScrollbarMode horizontalScrollbarMode = webPage->alwaysShowsHorizontalScroller() ? ScrollbarAlwaysOn : defaultScrollbarMode;
    ScrollbarMode verticalScrollbarMode = webPage->alwaysShowsVerticalScroller() ? ScrollbarAlwaysOn : defaultScrollbarMode;

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

RefPtr<Frame> WebFrameLoaderClient::createFrame(const String& name, HTMLFrameOwnerElement&ownerElement)
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

RefPtr<Widget> WebFrameLoaderClient::createPlugin(const IntSize&, HTMLPlugInElement& pluginElement, const URL& url, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually)
{
    UNUSED_PARAM(pluginElement);
    return nullptr;
}

void WebFrameLoaderClient::redirectDataToPlugin(Widget& pluginWidget)
{
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

String WebFrameLoaderClient::overrideMediaType() const
{
    notImplemented();
    return String();
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
    ASSERT(!hasProcessPrivilege(ProcessPrivilege::CanAccessRawCookies));
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
        String out("PROGDIR:Cache/FavIcons/");
    out.append(baseNameBuilder.toString());
        return out;
}

void WebFrameLoaderClient::getLoadDecisionForIcons(const Vector<std::pair<WebCore::LinkIcon&, uint64_t>>& icons)
{
	WebPage* webPage = m_frame->page();
	auto* documentLoader = m_frame->coreFrame()->loader().documentLoader();
	const String fileName(generateFileNameForIcon(documentLoader->url().host().toString()));

	if (webPage && webPage->_fFavIconLoad && !webPage->_fFavIconLoad(documentLoader->url()))
	{
		for (auto& icon : icons)
			documentLoader->didGetLoadDecisionForIcon(false, icon.second, [](SharedBuffer*data){});
		return;
	}

	{
		RefPtr<WebCore::SharedBuffer> buffer = WebCore::SharedBuffer::createWithContentsOfFile(fileName);
		if (buffer.get())
		{
			finishedLoadingIcon(buffer.get());

			for (auto& icon : icons)
				documentLoader->didGetLoadDecisionForIcon(false, icon.second, [](SharedBuffer*data){});
			
			return;
		}
	}

	for (auto& icon : icons)
	{
		if (icon.first.type == WebCore::LinkIconType::Favicon)
		{
			documentLoader->didGetLoadDecisionForIcon(true, icon.second, [this](SharedBuffer*data) {
				finishedLoadingIcon(data);
			});
		}
		else
		{
			documentLoader->didGetLoadDecisionForIcon(false, icon.second, [](SharedBuffer*data){});
		}
	}
}

void WebFrameLoaderClient::finishedLoadingIcon(WebCore::SharedBuffer* data)
{
	auto* documentLoader = m_frame->coreFrame()->loader().documentLoader();
	if (data != nullptr && data->size() > 0)
	{
		const String fileName(generateFileNameForIcon(documentLoader->url().host().toString()));
		WTF::FileSystemImpl::PlatformFileHandle file = WTF::FileSystemImpl::openFile(fileName, WTF::FileSystemImpl::FileOpenMode::Write);
		if (file != WTF::FileSystemImpl::invalidPlatformFileHandle)
		{
			if (int(data->size()) != WTF::FileSystemImpl::writeToFile(file, data->data(), data->size()))
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

	if (data && data->size())
	{
		WebPage* webPage = m_frame->page();
		if (webPage && webPage->_fFavIconLoaded)
			webPage->_fFavIconLoaded(data, documentLoader->url());
	}
}

void WebFrameLoaderClient::didCreateWindow(DOMWindow& window)
{
}

#if ENABLE(APPLICATION_MANIFEST)
void WebFrameLoaderClient::finishedLoadingApplicationManifest(uint64_t, const std::optional<WebCore::ApplicationManifest>&)
{
	notImplemented();
}
#endif

} // namespace WebKit
