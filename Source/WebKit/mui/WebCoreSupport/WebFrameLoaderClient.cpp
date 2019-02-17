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

#include "config.h"
#include "WebFrameLoaderClient.h"

#include "DefaultPolicyDelegate.h"
#include "DOMCoreClasses.h"
#include "FileIOLinux.h"
#include "NotImplemented.h"
#include "WebNavigationAction.h"
#include "WebCachedFramePlatformData.h"
#include "WebChromeClient.h"
#include "WebDocumentLoader.h"
#include "WebDownload.h"
#include "WebDownloadDelegate.h"
#include "WebError.h"
#include "WebFrame.h"
#include "WebFrameLoadDelegate.h"
#include "WebFrameNetworkingContext.h"
#include "WebFramePolicyListener.h"
#include "WebHistory.h"
#include "WebHistoryDelegate.h"
#include "WebHistoryItem.h"
#include "WebHistoryItem_p.h"
#include "WebMutableURLRequest.h"
#include "WebNavigationData.h"
#include "WebNotificationDelegate.h"
#include "WebPreferences.h"
#include "WebResourceLoadDelegate.h"
#include "WebSecurityOrigin.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLResponse.h"
#include "WebView.h"

#include <CachedFrame.h>
#include <CredentialStorage.h>
#include <DocumentLoader.h>
#include <FormState.h>
#include <Frame.h>
#include <FrameLoader.h>
#include <FrameTree.h>
#include <FrameView.h>
#include <HTMLAppletElement.h>
#include <HTMLFormElement.h>
#include <HTMLFrameElement.h>
#include <HTMLFrameOwnerElement.h>
#include <HTMLNames.h>
#include <HTMLObjectElement.h>
#include <HTMLPlugInElement.h>
#include <HistoryItem.h>
#include <Logging.h>
#include <MIMETypeRegistry.h>
#include <Page.h>
#include <PluginDatabase.h>
#include <PluginPackage.h>
#include <PluginView.h>
#include <ResourceHandle.h>
#include <ResourceHandleInternal.h>
#include <ResourceLoader.h> 
#include <ScriptController.h>
#include <Settings.h>
#include <APICast.h>

#include "AutofillManager.h"
#include "gui.h"
#include "utils.h"
#include <proto/dos.h>
#include <proto/intuition.h>
#include <clib/debug_protos.h>
#undef get
#undef String

#include <cstdio>

using namespace WebCore;
using namespace HTMLNames;

static WebDataSource* getWebDataSource(DocumentLoader* loader)
{
    return loader ? static_cast<WebDocumentLoader*>(loader)->dataSource() : 0;
}

WebFrameLoaderClient::WebFrameLoaderClient(WebFrame* webFrame)
    : m_webFrame(webFrame)
    , m_pluginView(0)
    , m_hasSentResponseToPlugin(false)
    , m_policyFunction(0)
    , m_policyListener(0)
{
    ASSERT_ARG(webFrame, webFrame);
}

WebFrameLoaderClient::~WebFrameLoaderClient()
{
    if (m_webFrame)
        m_webFrame = 0;
    if (m_pluginView)
        delete m_pluginView;
    if (m_policyListener)
        delete m_policyListener;
}

bool WebFrameLoaderClient::hasWebView() const
{
    return m_webFrame->webView();
}

void WebFrameLoaderClient::forceLayout()
{
    Frame* frame = core(m_webFrame);
    if (!frame)
        return;

    if (frame->document() && frame->document()->inPageCache())
        return;

    FrameView* view = frame->view();
    if (!view)
        return;

    view->setNeedsLayout();
    view->forceLayout(true);
}

void WebFrameLoaderClient::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader* loader, const ResourceRequest& request)
{
    WebView* webView = m_webFrame->webView();
    SharedPtr<WebResourceLoadDelegate> resourceLoadDelegate = webView->webResourceLoadDelegate();
    if (!resourceLoadDelegate)
        return;

    WebMutableURLRequest* webURLRequest = WebMutableURLRequest::createInstance(request);
    resourceLoadDelegate->identifierForInitialRequest(webView, webURLRequest, getWebDataSource(loader), identifier);
    delete webURLRequest;
}

void WebFrameLoaderClient::dispatchDidReceiveAuthenticationChallenge(DocumentLoader* loader, unsigned long identifier, const AuthenticationChallenge& challenge)
{
    ASSERT(challenge.authenticationClient());

    Credential storedCredential = CredentialStorage::defaultCredentialStorage().get(challenge.protectionSpace());
    if(!storedCredential.isEmpty())
    {
	challenge.authenticationClient()->receivedCredential(challenge, storedCredential);
	return;
    }

    char *username = NULL, *password = NULL;
    String host = challenge.protectionSpace().host();
    String realm = challenge.protectionSpace().realm();
    CredentialPersistence persistence = CredentialPersistenceNone;
    
    if(DoMethod(app, MM_OWBApp_Login, host.utf8().data(), realm.utf8().data(), &username, &password, &persistence))
    {
	Credential credential = Credential(username, password, persistence);
	free(username);
	free(password);
	challenge.authenticationClient()->receivedCredential(challenge, credential);
    }
    else
	challenge.authenticationClient()->receivedRequestToContinueWithoutCredential(challenge);
}

void WebFrameLoaderClient::dispatchDidLayout(LayoutMilestones milestones)
{
    SharedPtr<WebFrameLoadDelegate> frameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (frameLoadDelegate)
    {
        if (milestones & DidFirstLayout)
            frameLoadDelegate->didFirstLayoutInFrame(m_webFrame);

	if (milestones & DidFirstVisuallyNonEmptyLayout)
	    frameLoadDelegate->didFirstVisuallyNonEmptyLayoutInFrame(m_webFrame);
    }
}


void WebFrameLoaderClient::dispatchDidCancelAuthenticationChallenge(DocumentLoader* loader, unsigned long identifier, const AuthenticationChallenge& challenge)
{
	/*
    SharedPtr<WebResourceLoadDelegate> resourceLoadDelegate = webView->webResourceLoadDelegate();
    if (!resourceLoadDelegate)
        return;

    WebURLAuthenticationChallenge* webChallenge = WebURLAuthenticationChallenge::createInstance(challenge);
    resourceLoadDelegate->didCancelAuthenticationChallenge(webView, identifier, webChallenge, getWebDataSource(loader));
    delete webChallenge;
	*/
}

bool WebFrameLoaderClient::shouldUseCredentialStorage(DocumentLoader* loader, unsigned long identifier)
{
    /*WebView* webView = m_webFrame->webView();
    SharedPtr<WebResourceLoadDelegate> resourceLoadDelegate = webView->webResourceLoadDelegate();
    if (!resourceLoadDelegate)
        return true;

    BOOL shouldUse;
    if (SUCCEEDED(resourceLoadDelegatePrivate->shouldUseCredentialStorage(webView, identifier, getWebDataSource(loader), &shouldUse)))
        return shouldUse;*/

    return true;
}

void WebFrameLoaderClient::dispatchWillSendRequest(DocumentLoader* loader, unsigned long identifier, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
}

void WebFrameLoaderClient::dispatchDidReceiveResponse(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& response)
{
	//WebView* webView = m_webFrame->webView();

	//kprintf("dispatchDidReceiveResponse:: loader %p webView %p\n", loader, webView);
	/*
    SharedPtr<WebResourceLoadDelegate> resourceLoadDelegate = webView->webResourceLoadDelegate();
    if (!resourceLoadDelegate)
        return;

    WebURLResponse* webURLResponse = WebURLResponse::createInstance(response);
    resourceLoadDelegate->didReceiveResponse(webView, identifier, webURLResponse, getWebDataSource(loader));
    delete webURLResponse;
	*/
}

void WebFrameLoaderClient::dispatchDidReceiveContentLength(DocumentLoader* loader, unsigned long identifier, int length)
{
	//WebView* webView = m_webFrame->webView();

	//kprintf("dispatchDidReceiveContentLength:: loader %p webView %p\n", loader, webView);
	/*
    SharedPtr<WebResourceLoadDelegate> resourceLoadDelegate = webView->webResourceLoadDelegate();
    if (!resourceLoadDelegate)
        return;

    resourceLoadDelegate->didReceiveContentLength(webView, identifier, length, getWebDataSource(loader));
	*/
}

void WebFrameLoaderClient::dispatchDidFinishLoading(DocumentLoader* loader, unsigned long identifier)
{
	//WebView* webView = m_webFrame->webView();

	//kprintf("dispatchDidFinishLoading:: loader %p webView %p\n", loader, webView);
	/*
    SharedPtr<WebResourceLoadDelegate> resourceLoadDelegate = webView->webResourceLoadDelegate();
    if (!resourceLoadDelegate)
        return;

    resourceLoadDelegate->didFinishLoadingFromDataSource(webView, identifier, getWebDataSource(loader));
	*/
}

void WebFrameLoaderClient::dispatchDidFailLoading(DocumentLoader* loader, unsigned long identifier, const ResourceError& error)
{
	//WebView* webView = m_webFrame->webView();

	//kprintf("dispatchDidFailLoading:: loader %p webView %p\n", loader, webView);
	/*
	SharedPtr<WebResourceLoadDelegate> resourceLoadDelegate = webView->webResourceLoadDelegate();
	kprintf("dispatchDidFailLoading:: resourceLoadDelegate %p\n", resourceLoadDelegate);
    if (!resourceLoadDelegate)
        return;

    WebError* webError = WebError::createInstance(error);
    resourceLoadDelegate->didFailLoadingWithError(webView, identifier, webError, getWebDataSource(loader));
    delete webError;
	*/
}

bool WebFrameLoaderClient::shouldCacheResponse(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& response, const unsigned char* data, const unsigned long long length)
{
    return false;
}

void WebFrameLoaderClient::dispatchDidHandleOnloadEvents()
{
    SharedPtr<WebFrameLoadDelegate> frameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (frameLoadDelegate)
        frameLoadDelegate->didHandleOnloadEventsForFrame(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidReceiveServerRedirectForProvisionalLoad()
{
    SharedPtr<WebFrameLoadDelegate> frameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (frameLoadDelegate)
        frameLoadDelegate->didReceiveServerRedirectForProvisionalLoadForFrame(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidCancelClientRedirect()
{
    SharedPtr<WebFrameLoadDelegate> frameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (frameLoadDelegate)
        frameLoadDelegate->didCancelClientRedirectForFrame(m_webFrame);
}

void WebFrameLoaderClient::dispatchWillPerformClientRedirect(const URL& url, double delay, double fireDate)
{
    SharedPtr<WebFrameLoadDelegate> frameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (frameLoadDelegate)
        frameLoadDelegate->willPerformClientRedirectToURL(m_webFrame, url.string().utf8().data(), delay, fireDate);
}

void WebFrameLoaderClient::dispatchDidChangeLocationWithinPage()
{
    SharedPtr<WebFrameLoadDelegate> frameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (frameLoadDelegate)
        frameLoadDelegate->didChangeLocationWithinPageForFrame(m_webFrame);
}
void WebFrameLoaderClient::dispatchDidPushStateWithinPage()
{
    balNotImplemented();
}

void WebFrameLoaderClient::dispatchDidReplaceStateWithinPage()
{
    balNotImplemented();
}

void WebFrameLoaderClient::dispatchDidPopStateWithinPage()
{
    balNotImplemented();
}

void WebFrameLoaderClient::dispatchWillClose()
{
    SharedPtr<WebFrameLoadDelegate> frameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (frameLoadDelegate)
        frameLoadDelegate->willCloseFrame(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidReceiveIcon()
{
#if ENABLE(ICONDATABASE)
    m_webFrame->webView()->dispatchDidReceiveIconFromWebFrame(m_webFrame);
#endif
}

void WebFrameLoaderClient::dispatchDidStartProvisionalLoad()
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate)
        webFrameLoadDelegate->didStartProvisionalLoad(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidReceiveTitle(const WebCore::StringWithDirection& title)
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate)
	webFrameLoadDelegate->titleChange(m_webFrame, title.string().utf8().data());
}

void WebFrameLoaderClient::dispatchDidChangeIcons(WebCore::IconType type)
{
    if (type != WebCore::Favicon)
        return;

    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate)
        webFrameLoadDelegate->didChangeIcons(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidCommitLoad()
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate)
        webFrameLoadDelegate->didCommitLoad(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidFinishDocumentLoad()
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate)
        webFrameLoadDelegate->didFinishDocumentLoadForFrame(m_webFrame);
}

void WebFrameLoaderClient::dispatchDidFinishLoad()
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate)
        webFrameLoadDelegate->didFinishLoad(m_webFrame);
}

Frame* WebFrameLoaderClient::dispatchCreatePage(const WebCore::NavigationAction&)
{
    BalWidget *widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddPage, NULL, FALSE, FALSE, NULL, NULL, getv(m_webFrame->webView()->viewWindow()->browser, MA_OWBBrowser_PrivateBrowsing), FALSE);

    if(widget && widget->webView)
    {
        WebFrame *mainFrame = widget->webView->mainFrame();
        if (mainFrame && mainFrame->impl())
            return mainFrame->impl();
    }

    return 0;
}

void WebFrameLoaderClient::dispatchShow()
{
    /*WebView* webView = m_webFrame->webView();
    COMPtr<IWebUIDelegate> ui;
    if (SUCCEEDED(webView->uiDelegate(&ui)))
        ui->webViewShow(webView);*/
}

void WebFrameLoaderClient::setMainDocumentError(DocumentLoader*, const ResourceError& error)
{
    if (!m_pluginView)
        return;

    if (m_pluginView->status() == PluginStatusLoadedSuccessfully) //
        m_pluginView->didFail(error);
    m_pluginView = 0;
    m_hasSentResponseToPlugin = false;
}

void WebFrameLoaderClient::committedLoad(DocumentLoader* loader, const char* data, int length)
{
    //	  const String& textEncoding = loader->response().textEncodingName();

    if (!m_pluginView)
	loader->commitData(data, length);

    // If the document is a stand-alone media document, now is the right time to cancel the WebKit load.
    // FIXME: This code should be shared across all ports. <http://webkit.org/b/48762>.
    Frame* coreFrame = core(m_webFrame);
    if (coreFrame->document()->isMediaDocument())
	loader->cancelMainResourceLoad(pluginWillHandleLoadError(loader->response()));
    
    if (!m_pluginView || m_pluginView->status() != PluginStatusLoadedSuccessfully) //
        return;

    if (!m_hasSentResponseToPlugin) {
	m_pluginView->didReceiveResponse(loader->response());
        // didReceiveResponse sets up a new stream to the plug-in. on a full-page plug-in, a failure in
        // setting up this stream can cause the main document load to be cancelled, setting m_pluginView
        // to null
        if (!m_pluginView)
            return;
        m_hasSentResponseToPlugin = true;
    }
    m_pluginView->didReceiveData(data, length);
}

void WebFrameLoaderClient::finishedLoading(DocumentLoader*)
{
    //committedLoad(loader, 0, 0);
	
    if (!m_pluginView) {
        return;
    }

    if (m_pluginView->status() == PluginStatusLoadedSuccessfully) //
        m_pluginView->didFinishLoading();
    m_pluginView = 0;
    m_hasSentResponseToPlugin = false;
}

void WebFrameLoaderClient::updateGlobalHistory()
{
    DocumentLoader* loader = core(m_webFrame)->loader().documentLoader();

    WebView* webView = m_webFrame->webView();
    SharedPtr<WebHistoryDelegate> historyDelegate = webView->historyDelegate();
    if (historyDelegate) {
        String url(loader->urlForHistory().string());
        String title(loader->title().string());
        String redirectSource(loader->clientRedirectSourceForHistory());
        WebURLResponse * urlResponse = WebURLResponse::createInstance(loader->response());
        WebMutableURLRequest * urlRequest = WebMutableURLRequest::createInstance(loader->originalRequestCopy());
        WebNavigationData * navigationData = WebNavigationData::createInstance(url.utf8().data(), title.utf8().data(), urlRequest, urlResponse, loader->substituteData().isValid(), redirectSource.utf8().data());

        historyDelegate->didNavigateWithNavigationData(webView, navigationData, m_webFrame);
        return;
    }


    WebHistory* history = WebHistory::sharedHistory();
    if (!history)
        return;

    history->visitedURL(strdup(loader->urlForHistory().string().utf8().data()), strdup(loader->title().string().utf8().data()), strdup(loader->originalRequestCopy().httpMethod().utf8().data()), loader->urlForHistoryReflectsFailure());
}

void WebFrameLoaderClient::updateGlobalHistoryRedirectLinks()
{
    WebView* webView = m_webFrame->webView();
    SharedPtr<WebHistoryDelegate> historyDelegate = webView->historyDelegate();

    WebHistory* history = WebHistory::sharedHistory();

    DocumentLoader* loader = core(m_webFrame)->loader().documentLoader();
    ASSERT(loader->unreachableURL().isEmpty());

    if (!loader->clientRedirectSourceForHistory().isNull()) {
        if (historyDelegate) {
            String sourceURL(loader->clientRedirectSourceForHistory());
            String destinationURL(loader->clientRedirectDestinationForHistory());
            historyDelegate->didPerformClientRedirectFromURL(webView, sourceURL.utf8().data(), destinationURL.utf8().data(), m_webFrame);
        } else {
            if (history) {
                if (WebHistoryItem* webHistoryItem = history->itemForURLString(strdup(loader->clientRedirectSourceForHistory().utf8().data())))
                    webHistoryItem->getPrivateItem()->m_historyItem.get()->addRedirectURL(loader->clientRedirectDestinationForHistory());
            }
        }
    }

    if (!loader->serverRedirectSourceForHistory().isNull()) {
        if (historyDelegate) {
            String sourceURL(loader->serverRedirectSourceForHistory());
            String destinationURL(loader->serverRedirectDestinationForHistory());
            historyDelegate->didPerformServerRedirectFromURL(webView, sourceURL.utf8().data(), destinationURL.utf8().data(), m_webFrame);
        } else {
            if (history) {
                if (WebHistoryItem *webHistoryItem = history->itemForURLString(strdup(loader->serverRedirectSourceForHistory().utf8().data())))
                    webHistoryItem->getPrivateItem()->m_historyItem.get()->addRedirectURL(loader->serverRedirectDestinationForHistory());
            }
        }
    }
}

bool WebFrameLoaderClient::shouldGoToHistoryItem(HistoryItem *item) const
{
    // FIXME: This is a very simple implementation. More sophisticated
    // implementation would delegate the decision to a PolicyDelegate.
    // See mac implementation for example.
    return item != 0;
}

bool WebFrameLoaderClient::shouldStopLoadingForHistoryItem(HistoryItem*) const 
{ 
    return true;
}

void WebFrameLoaderClient::dispatchDidAddBackForwardItem(HistoryItem*) const
{
    balNotImplemented();
}

void WebFrameLoaderClient::dispatchDidRemoveBackForwardItem(HistoryItem*) const
{
    balNotImplemented();
}

void WebFrameLoaderClient::dispatchDidChangeBackForwardIndex() const
{
    balNotImplemented();
}

void WebFrameLoaderClient::didDisplayInsecureContent()
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate) {
        webFrameLoadDelegate->didDisplayInsecureContent(m_webFrame);
    }
}

void WebFrameLoaderClient::didRunInsecureContent(SecurityOrigin* origin, const URL& insecureURL)
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate) {
        WebSecurityOrigin* webSecurityOrigin = WebSecurityOrigin::createInstance(origin);
        webFrameLoadDelegate->didRunInsecureContent(m_webFrame, webSecurityOrigin);
    }
}

void WebFrameLoaderClient::didDetectXSS(const URL&, bool)
{
    // FIXME: propagate call into the private delegate
}

Ref<DocumentLoader> WebFrameLoaderClient::createDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
{
    Ref<WebDocumentLoader> loader = WebDocumentLoader::create(request, substituteData);

    WebDataSource* dataSource = WebDataSource::createInstance(loader.ptr());
    loader->setDataSource(dataSource);
    return WTF::move(loader);
}

void WebFrameLoaderClient::updateCachedDocumentLoader(WebCore::DocumentLoader&)
{
}

void WebFrameLoaderClient::setTitle(const WebCore::StringWithDirection& title, const URL& url)
{
    WebView* webView = m_webFrame->webView();
    SharedPtr<WebHistoryDelegate> historyDelegate = webView->historyDelegate();
    if (historyDelegate) {
        historyDelegate->updateHistoryTitle(webView, title.string().utf8().data(), url.string().utf8().data());
        return;
    }
    bool privateBrowsingEnabled = false; 

    BalWidget* widget = m_webFrame->webView()->viewWindow();
    if(widget)
    {
	// We don't use the global preference private setting
	privateBrowsingEnabled = getv(widget->browser, MA_OWBBrowser_PrivateBrowsing) != FALSE;
    }

    if (privateBrowsingEnabled)
        return;

    // update title in global history
    WebHistory* history = webHistory();
    if (!history)
        return;

    WebHistoryItem* item = history->itemForURL(strdup(url.string().utf8().data()));
    if (!item)
        return;

    item->setTitle(title.string().utf8().data());

    // Not sure calling twice visitedURL is particularly smart, but else, title isn't set in our history
//#warning "find something better, really, calling visitedURL is not optimal at all"
    DocumentLoader* loader = core(m_webFrame)->loader().documentLoader();
    history->visitedURL(strdup(loader->urlForHistory().string().utf8().data()), strdup(loader->title().string().utf8().data()), strdup(loader->originalRequestCopy().httpMethod().utf8().data()), loader->urlForHistoryReflectsFailure());
}

void WebFrameLoaderClient::savePlatformDataToCachedFrame(CachedFrame* cachedFrame)
{
    Frame* coreFrame = core(m_webFrame);
    if (!coreFrame)
        return;

    ASSERT(coreFrame->loader().documentLoader() == cachedFrame->documentLoader());

    cachedFrame->setCachedFramePlatformData(std::unique_ptr<CachedFramePlatformData>(new WebCachedFramePlatformData(getWebDataSource(coreFrame->loader().documentLoader()))));
}

void WebFrameLoaderClient::transitionToCommittedFromCachedFrame(CachedFrame*)
{
}

void WebFrameLoaderClient::transitionToCommittedForNewPage()
{
    WebView* view = m_webFrame->webView();
    BalRectangle rect = view->frameRect();
    bool transparent = view->transparent();
    Color backgroundColor = transparent ? Color::transparent : Color::white;
    core(m_webFrame)->createView(IntRect(rect).size(), backgroundColor, transparent);
}

void WebFrameLoaderClient::didSaveToPageCache()
{
}

void WebFrameLoaderClient::didRestoreFromPageCache()
{
}

void WebFrameLoaderClient::dispatchDidBecomeFrameset(bool)
{
}

bool WebFrameLoaderClient::canCachePage() const
{
    return true;
}

RefPtr<Frame> WebFrameLoaderClient::createFrame(const URL& url, const String& name, HTMLFrameOwnerElement* ownerElement, const String& referrer, bool /*allowsScrolling*/, int /*marginWidth*/, int /*marginHeight*/)
{
    RefPtr<Frame> result = createFrame(url, name, ownerElement, referrer);
    if (!result)
        return 0;

    return result;
}

PassRefPtr<Frame> WebFrameLoaderClient::createFrame(const URL& url, const String& name, HTMLFrameOwnerElement* ownerElement, const String& referrer)
{
    if (url.string().isEmpty())
        return 0;

    Frame* coreFrame = core(m_webFrame);
    ASSERT(coreFrame);

    WebFrame* webFrame = WebFrame::createInstance();

    RefPtr<Frame> childFrame = webFrame->createSubframeWithOwnerElement(m_webFrame->webView(), coreFrame->page(), ownerElement);

    childFrame->tree().setName(name);
    coreFrame->tree().appendChild(childFrame);
    childFrame->init();

    // The creation of the frame may have run arbitrary JavaScript that removed it from the page already.
    if (!childFrame->page()) {
        delete webFrame;
        return 0;
    }

    childFrame->loader().loadURLIntoChildFrame(url, referrer, childFrame.get());

    // The frame's onload handler may have removed it from the document.
    if (!childFrame->tree().parent()) {
        delete webFrame;
        return 0;
    }

    return childFrame.release();
}

RefPtr<Widget> WebFrameLoaderClient::createPlugin(const IntSize& pluginSize, HTMLPlugInElement* element, const URL& url, const Vector<String>& paramNames, const Vector<String>& paramValues, const String& mimeType, bool loadManually)
{
    Frame* frame = core(m_webFrame);
    RefPtr<PluginView> pluginView = PluginView::create(frame, pluginSize, element, url, paramNames, paramValues, mimeType, loadManually);

    if (pluginView->status() == PluginStatusLoadedSuccessfully)
        return pluginView;

    return pluginView;
}

void WebFrameLoaderClient::redirectDataToPlugin(Widget* pluginWidget)
{
    // Ideally, this function shouldn't be necessary, see <rdar://problem/4852889>

    m_pluginView = static_cast<PluginView*>(pluginWidget);
}

WebHistory* WebFrameLoaderClient::webHistory() const
{
    if (m_webFrame != m_webFrame->webView()->topLevelFrame())
        return 0;

    return WebHistory::sharedHistory();
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
    notImplemented();
}

void WebFrameLoaderClient::detachedFromParent3()
{
    notImplemented();
}

void WebFrameLoaderClient::cancelPolicyCheck()
{
    if (m_policyListener) {
        m_policyListener->invalidate();
        m_policyListener = 0;
    }

    m_policyFunction = 0;
}

void WebFrameLoaderClient::dispatchWillSubmitForm(PassRefPtr<FormState> formState, FramePolicyFunction function)
{
    Object * browser = m_webFrame->webView()->viewWindow()->browser;
    if(!getv(browser, MA_OWBBrowser_PrivateBrowsing))
    {
        DoMethod(browser, MM_OWBBrowser_Autofill_SaveTextFields, formState->form());
    }

    // XXX: save credentials here as well instead of doing it in the defaultpolicydelegate, since it's a bit clumsy

    function(PolicyUse);
}

void WebFrameLoaderClient::dispatchWillSendSubmitEvent(PassRefPtr<WebCore::FormState> formState)
{
    Object * browser = m_webFrame->webView()->viewWindow()->browser;
    if(!getv(browser, MA_OWBBrowser_PrivateBrowsing))
    {
        DoMethod(browser, MM_OWBBrowser_Autofill_SaveTextFields, formState->form());
    }
}

void WebFrameLoaderClient::revertToProvisionalState(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::setMainFrameDocumentReady(bool)
{
    notImplemented();
}

void WebFrameLoaderClient::willChangeTitle(DocumentLoader*)
{
    notImplemented();
}

void WebFrameLoaderClient::didChangeTitle(DocumentLoader*)
{
    notImplemented();
}

bool WebFrameLoaderClient::canHandleRequest(const ResourceRequest& request) const
{
    return WebView::canHandleRequest(request);
}

bool WebFrameLoaderClient::canShowMIMEType(const String& type) const
{
	String ltype = type.lower();
	bool show =	ltype.isEmpty() ||
	  MIMETypeRegistry::isSupportedImageMIMEType(ltype) || 
	  MIMETypeRegistry::isSupportedNonImageMIMEType(ltype) ||
	  MIMETypeRegistry::isSupportedMediaMIMEType(ltype) ||
	  PluginDatabase::installedPlugins()->isMIMETypeRegistered(ltype);

	//kprintf("WebView::canShowMIMEType(%s): %s\n", ltype.utf8().data(), show ? "yes" : "no");

	return show;
}

bool WebFrameLoaderClient::canShowMIMETypeAsHTML(const WTF::String& MIMEType) const
{
	//kprintf("canShowMIMETypeAsHTML(%s)\n", MIMEType.latin1().data());
	return true;
}

bool WebFrameLoaderClient::representationExistsForURLScheme(const String& /*URLScheme*/) const
{
    notImplemented();
    return false;
}

String WebFrameLoaderClient::generatedMIMETypeForURLScheme(const String& /*URLScheme*/) const
{
    notImplemented();
    ASSERT_NOT_REACHED();
    return String();
}

void WebFrameLoaderClient::frameLoadCompleted()
{
}

void WebFrameLoaderClient::restoreViewState()
{
}

void WebFrameLoaderClient::provisionalLoadStarted()
{
    notImplemented();
}

void WebFrameLoaderClient::didFinishLoad()
{
    notImplemented();
}

void WebFrameLoaderClient::prepareForDataSourceReplacement()
{
    notImplemented();
}

String WebFrameLoaderClient::userAgent(const URL& url)
{
    return m_webFrame->webView()->userAgentForURL(url.string().utf8().data()).c_str();
}

void WebFrameLoaderClient::transitionToCommittedFromCachedPage(CachedPage*)
{
}

void WebFrameLoaderClient::saveViewStateToItem(HistoryItem*)
{
}

// FIXME: We need to have better names for the 7 next *Error methods and have a localized description for each.
ResourceError WebFrameLoaderClient::cancelledError(const ResourceRequest& request)
{
    return ResourceError(String(WebURLErrorDomain), WebURLErrorCancelled, request.url().string(), String());
}

ResourceError WebFrameLoaderClient::blockedError(const ResourceRequest& request)
{
    return ResourceError(String(WebKitErrorDomain), WebKitErrorCannotUseRestrictedPort, request.url().string(), String());
}

ResourceError WebFrameLoaderClient::cannotShowURLError(const ResourceRequest& request)
{
    return ResourceError(String(WebKitCannotShowURL), WebURLErrorBadURL, request.url().string(), String());
}

ResourceError WebFrameLoaderClient::interruptedForPolicyChangeError(const ResourceRequest& request)
{
    return ResourceError(String(WebKitErrorDomain), WebKitErrorFrameLoadInterruptedByPolicyChange, request.url().string(), String());
}

ResourceError WebFrameLoaderClient::cannotShowMIMETypeError(const ResourceResponse& response)
{
    return ResourceError(String(WebKitErrorMIMETypeKey), WebKitErrorCannotShowMIMEType, response.url().string(), String());
}

ResourceError WebFrameLoaderClient::fileDoesNotExistError(const ResourceResponse& response)
{
    return ResourceError(String(WebKitFileDoesNotExist), WebURLErrorFileDoesNotExist, response.url().string(), String());
}

ResourceError WebFrameLoaderClient::pluginWillHandleLoadError(const ResourceResponse& response)
{
    return ResourceError(String(WebKitErrorDomain), WebKitErrorPlugInWillHandleLoad, response.url().string(), String());
}


bool WebFrameLoaderClient::shouldFallBack(const ResourceError& error)
{
    return error.errorCode() != WebURLErrorCancelled;
}

WebFramePolicyListener* WebFrameLoaderClient::setUpPolicyListener(FramePolicyFunction function)
{
    // FIXME: <rdar://5634381> We need to support multiple active policy listeners.

    if (m_policyListener)
	{
        m_policyListener->invalidate();
		/*
		m_policyListener = 0;
		delete m_policyListener;
		*/
	}

    m_policyListener = WebFramePolicyListener::createInstance(m_webFrame);
    m_policyFunction = function;

    return m_policyListener;
}

void WebFrameLoaderClient::receivedPolicyDecision(PolicyAction action)
{
    ASSERT(m_policyListener);
    ASSERT(m_policyFunction);

    FramePolicyFunction function = m_policyFunction;

    m_policyListener = 0;
    m_policyFunction = 0;

    function(action);
}

void WebFrameLoaderClient::dispatchDecidePolicyForResponse(const WebCore::ResourceResponse& response, const ResourceRequest& request, FramePolicyFunction function)
{
    SharedPtr<WebPolicyDelegate> policyDelegate = m_webFrame->webView()->policyDelegate();
    if (!policyDelegate)
        policyDelegate = DefaultPolicyDelegate::sharedInstance();

    WebMutableURLRequest* urlRequest = WebMutableURLRequest::createInstance(request);

    if (policyDelegate->decidePolicyForMIMEType(m_webFrame->webView(), response, urlRequest, m_webFrame, setUpPolicyListener(function))) {
        delete urlRequest;
        return;
    }
}

void WebFrameLoaderClient::dispatchDecidePolicyForNewWindowAction(const NavigationAction& action, const ResourceRequest& request, PassRefPtr<FormState> formState, const String& frameName, FramePolicyFunction function)
{
    SharedPtr<WebPolicyDelegate> policyDelegate = m_webFrame->webView()->policyDelegate();
    if (!policyDelegate)
        policyDelegate = DefaultPolicyDelegate::sharedInstance();

    WebMutableURLRequest* urlRequest =  WebMutableURLRequest::createInstance(request);
    WebNavigationAction* actionInformation = WebNavigationAction::createInstance(&action, formState ? formState->form() : 0, m_webFrame);

    //policyDelegate->decidePolicyForNewWindowAction(d->webView, actionInformation, urlRequest, frameName, setUpPolicyListener(function));

    function(PolicyUse);
    delete urlRequest;
    delete actionInformation;
}

void WebFrameLoaderClient::dispatchDecidePolicyForNavigationAction(const NavigationAction& action, const ResourceRequest& request, PassRefPtr<FormState> formState, FramePolicyFunction function)
{
    SharedPtr<WebPolicyDelegate> policyDelegate = m_webFrame->webView()->policyDelegate();
    if (!policyDelegate) {
        policyDelegate = DefaultPolicyDelegate::sharedInstance();
        m_webFrame->webView()->setPolicyDelegate(policyDelegate);
    }

    WebMutableURLRequest* urlRequest =  WebMutableURLRequest::createInstance(request);
    WebNavigationAction* actionInformation = WebNavigationAction::createInstance(&action, formState ? formState->form() : 0, m_webFrame);

    policyDelegate->decidePolicyForNavigationAction(m_webFrame->webView(), actionInformation, urlRequest, m_webFrame, setUpPolicyListener(function));

    delete urlRequest;
    delete actionInformation;
}

void WebFrameLoaderClient::dispatchUnableToImplementPolicy(const ResourceError& error)
{
    SharedPtr<WebPolicyDelegate> policyDelegate = m_webFrame->webView()->policyDelegate();
    if (!policyDelegate)
        policyDelegate = DefaultPolicyDelegate::sharedInstance();

    WebError* webError = WebError::createInstance(error);
    policyDelegate->unableToImplementPolicyWithError(m_webFrame->webView(), webError, m_webFrame);

    delete webError;
}

void WebFrameLoaderClient::convertMainResourceLoadToDownload(DocumentLoader* documentLoader, const ResourceRequest& request, const ResourceResponse& response)
{
    SharedPtr<WebDownloadDelegate> downloadDelegate;
    WebView* webView = m_webFrame->webView();
    downloadDelegate = webView->downloadDelegate();
    if(downloadDelegate)
    {
        WebDownload* download = WebDownload::createInstance(documentLoader->mainResourceLoader()->handle(), &request, &response, downloadDelegate);
        download->start();
    }
}

bool WebFrameLoaderClient::dispatchDidLoadResourceFromMemoryCache(DocumentLoader*, const ResourceRequest&, const ResourceResponse&, int /*length*/)
{
    notImplemented();
    return false;
}

void WebFrameLoaderClient::dispatchDidFailProvisionalLoad(const ResourceError& error)
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate) {
        WebError* err = WebError::createInstance(error);
        webFrameLoadDelegate->didFailProvisionalLoad(m_webFrame, err);
    }
}

void WebFrameLoaderClient::dispatchDidFailLoad(const ResourceError& error)
{
    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate) {
        WebError* err = WebError::createInstance(error);
        webFrameLoadDelegate->didFailLoad(m_webFrame, err);
    }
}

void WebFrameLoaderClient::startDownload(const ResourceRequest&, const String& /* suggestedName */)
{
    notImplemented();
}

PassRefPtr<Widget> WebFrameLoaderClient::createJavaAppletWidget(const IntSize& pluginSize, HTMLAppletElement* element, const URL& /*baseURL*/, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    RefPtr<PluginView> pluginView = PluginView::create(core(m_webFrame), pluginSize, element, URL(), paramNames, paramValues, "application/x-java-applet", false);

    // Check if the plugin can be loaded successfully
    if (pluginView->plugin() && pluginView->plugin()->load())
        return pluginView;

    return pluginView;
}

ObjectContentType WebFrameLoaderClient::objectContentType(const URL& url, const String& mimeTypeIn, bool shouldPreferPlugInsForImages)
{
    String mimeType = mimeTypeIn;

    if (mimeType.isEmpty())
        mimeType = mimeTypeFromURL(url);

    if (mimeType.isEmpty()) {
        String decodedPath = decodeURLEscapeSequences(url.path());
        mimeType = PluginDatabase::installedPlugins()->MIMETypeForExtension(decodedPath.substring(decodedPath.reverseFind('.') + 1));
    }

    if (mimeType.isEmpty())
        return ObjectContentFrame; // Go ahead and hope that we can display the content.

    bool plugInSupportsMIMEType = PluginDatabase::installedPlugins()->isMIMETypeRegistered(mimeType);

    if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
        return shouldPreferPlugInsForImages && plugInSupportsMIMEType ? WebCore::ObjectContentNetscapePlugin : WebCore::ObjectContentImage;

    if (plugInSupportsMIMEType)
        return WebCore::ObjectContentNetscapePlugin;

    if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
        return WebCore::ObjectContentFrame;

    return WebCore::ObjectContentNone;
}


String WebFrameLoaderClient::overrideMediaType() const
{
    notImplemented();
    return String();
}

void WebFrameLoaderClient::dispatchDidClearWindowObjectInWorld(DOMWrapperWorld& world)
{
    Frame* coreFrame = core(m_webFrame);
    ASSERT(coreFrame);

    Settings* settings = &(coreFrame->settings());
    if (!settings || !settings->isScriptEnabled())
        return;
 
    JSGlobalContextRef context = toGlobalRef(coreFrame->script().globalObject(world)->globalExec());
    JSObjectRef windowObject = toRef(coreFrame->script().globalObject(world));
    ASSERT(windowObject);

    SharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate = m_webFrame->webView()->webFrameLoadDelegate();
    if (webFrameLoadDelegate)
        webFrameLoadDelegate->windowObjectClearNotification(m_webFrame, (void*)context, (void*)windowObject);
}

void WebFrameLoaderClient::documentElementAvailable()
{
}

void WebFrameLoaderClient::didPerformFirstNavigation() const
{
    WebPreferences* preferences = m_webFrame->webView()->preferences();
    if(!preferences)
        return;

    bool automaticallyDetectsCacheModel = preferences->automaticallyDetectsCacheModel();

    WebCacheModel cacheModel = preferences->cacheModel();

    if (automaticallyDetectsCacheModel && cacheModel < WebCacheModelDocumentBrowser)
        preferences->setCacheModel(WebCacheModelDocumentBrowser);
}

void WebFrameLoaderClient::frameLoaderDestroyed()
{
    // The FrameLoader going away is equivalent to the Frame going away,
    // so we now need to clear our frame pointer.
    // FrameLoaderClient own WebFrame.

    delete m_webFrame;
    delete this;
}

void WebFrameLoaderClient::registerForIconNotification(bool listen)
{
#if ENABLE(ICONDATABASE)
	m_webFrame->webView()->registerForIconNotification(listen);
#endif
}

void WebFrameLoaderClient::makeRepresentation(DocumentLoader*)
{
    notImplemented();
}

bool WebFrameLoaderClient::shouldAlwaysUsePluginDocument(const String& mimeType) const
{
    return false;
}

#if USE(CURL_OPENSSL)
void WebFrameLoaderClient::didReceiveSSLSecurityExtension(const ResourceRequest& request, const char* securityExtension)
{
    // FIXME: This sanity check is failing because m_webFrame->url() is returning null, so disable it for now.
    //ASSERT(request.url().string() == m_webFrame->url());
    UNUSED_PARAM(request); // For release build.
}
#endif

bool WebFrameLoaderClient::allowPlugins(bool enabledPerSettings)
{
	BalWidget* widget = m_webFrame->webView()->viewWindow();

	if(widget)
	{
		ULONG value = getv(widget->browser, MA_OWBBrowser_PluginsEnabled);

		switch(value)
		{
			default:
			case PLUGINS_DEFAULT:
				return enabledPerSettings;

			case PLUGINS_DISABLED:
				return false;

			case PLUGINS_ENABLED:
				return true;
		}
	}

	return enabledPerSettings;
}

PassRefPtr<FrameNetworkingContext> WebFrameLoaderClient::createNetworkingContext()
{
    return WebFrameNetworkingContext::create(core(m_webFrame), userAgent(core(m_webFrame)->document()->url()));
}

