
/*
 * Copyright (C) 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 * Copyright (C) 2009 Pleyo.  All rights reserved.
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
#include "WebViewPrivate.h"
#include "Chrome.h"
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>
#include "Document.h"
#include "DownloadDelegateMorphOS.h"
#include "Editor.h"
#include "EventHandler.h"
#include "FileIOLinux.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InspectorController.h"
#include "IntRect.h"
#include <wtf/MainThread.h>
#include "MemoryCache.h"
#include "MemoryPressureHandler.h"
#include "MouseEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "Node.h"
#include "Page.h"
#include "PageGroup.h"
#include "PopupMenu.h"
#include "ProgressTracker.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "Scrollbar.h"
#include "Settings.h"
#include "SharedTimer.h"
#include "TopSitesManager.h"
#include "WebDocumentLoader.h"
#include "WebError.h"
#include "WebFrame.h"
#include "WebIconDatabase.h"
#include "WebMutableURLRequest.h"
#include "WebScriptWorld.h"
#include "WebView.h"
#include "WindowFeatures.h"

#include "JSDOMWindow.h"

#include "owb-config.h"
#include "cairo.h"

/* MorphOS */
#include "gui.h"
#include "utils.h"
#include "asl.h"
#include <clib/debug_protos.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <proto/keymap.h>
#include <proto/asl.h>
#include <devices/rawkeycodes.h>
#include <devices/inputevent.h>

#define D(x)

#undef PageGroup

#if OS(MORPHOS)
namespace WTF
{
    extern int memory_consumption;
}
#endif

using namespace WebCore;

static const bool renderBenchmark = getenv("OWB_BENCHMARK");

/* MorphOSWebNotificationDelegate */

MorphOSWebNotificationDelegate::MorphOSWebNotificationDelegate()
{
}

MorphOSWebNotificationDelegate::~MorphOSWebNotificationDelegate()
{
}

void MorphOSWebNotificationDelegate::startLoadNotification(WebFrame* webFrame)
{
}

void MorphOSWebNotificationDelegate::progressNotification(WebFrame* webFrame)
{
    WebView* webView = webFrame->webView();
    if (webView->mainFrame() != webFrame)
        return;

	BalWidget* widget = webView->viewWindow();

	if (widget && widget->browser)
	{
		ULONG progress = int(webView->estimatedProgress() * 100);
		SetAttrs(widget->browser, MA_OWBBrowser_LoadingProgress, progress, TAG_DONE);
	}
}

void MorphOSWebNotificationDelegate::finishedLoadNotification(WebFrame* webFrame)
{
}

/* MorphOSJSActionDelegate */

void MorphOSJSActionDelegate::windowObjectClearNotification(WebFrame*, void*, void*)
{
//	printf("JS windowObjectClearNotification\n");
}

void MorphOSJSActionDelegate::consoleMessage(WebFrame*, int line, int column, const char* message)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "[JavaScript:%d:%d] %s", line, column, message);
	DoMethod(app, MM_OWBApp_AddConsoleMessage, buffer);
}

bool MorphOSJSActionDelegate::jsAlert(WebFrame* webFrame, const char* message)
{
	WebView* webView = webFrame->webView();
	BalWidget* widget = webView->viewWindow();

	if(widget)
	{
		char *converted = utf8_to_local(message);

		if(converted)
		{
			bool ret = MUI_RequestA(app, widget->window, 0, GSI(MSG_WEBVIEW_JSACTION_TITLE), GSI(MSG_REQUESTER_OK), (char *) converted, NULL) != 0;
			free(converted);
			return ret;
		}
	}

	return false;
}

bool MorphOSJSActionDelegate::jsConfirm(WebFrame* webFrame, const char* message)
{
	WebView* webView = webFrame->webView();
	BalWidget* widget = webView->viewWindow();

	if(widget)
	{
		char *converted = utf8_to_local(message);

		if(converted)
		{
			bool  ret;
			ret = MUI_RequestA(app, widget->window, 0, GSI(MSG_WEBVIEW_JSACTION_TITLE), GSI(MSG_REQUESTER_YES_NO), (char *) converted, NULL) != 0;
			free(converted);
			return ret;
		}
	}

	return false;
}

bool MorphOSJSActionDelegate::jsPrompt(WebFrame* webFrame, const char* message, const char* defaultValue, char** value)
{
    WebView* webView = webFrame->webView();
	BalWidget* widget = webView->viewWindow();

	if (widget)
	{
		char *converted = utf8_to_local(message);

		if(converted)
		{
			*value = (char *) DoMethod(widget->window, MM_OWBWindow_JavaScriptPrompt, converted, defaultValue);
			free(converted);
			return (*value != NULL);
		}
	}

	return false;
}

void MorphOSJSActionDelegate::exceededDatabaseQuota(WebFrame *frame, WebSecurityOrigin *origin, const char* databaseIdentifier)
{
}

MorphOSJSActionDelegate::MorphOSJSActionDelegate()
{
}

MorphOSJSActionDelegate::~MorphOSJSActionDelegate()
{
}

/* MorphOSWebFrameDelegate */

void MorphOSWebFrameDelegate::windowObjectClearNotification(WebFrame*, void*, void*)
{
	D(kprintf("windowObjectClearNotification\n"));
}

void MorphOSWebFrameDelegate::didReceiveServerRedirectForProvisionalLoadForFrame(WebFrame*)
{
	D(kprintf("didReceiveServerRedirectForProvisionalLoadForFrame\n"));
}

void MorphOSWebFrameDelegate::didCancelClientRedirectForFrame(WebFrame*)
{
	D(kprintf("didCancelClientRedirectForFrame\n"));
}

void MorphOSWebFrameDelegate::willPerformClientRedirectToURL(WebFrame*, const char*, double, double)
{
	D(kprintf("willPerformClientRedirectToURL\n"));
}

void MorphOSWebFrameDelegate::didStartProvisionalLoad(WebFrame* webFrame)
{
    WebView* webView = webFrame->webView();

	D(kprintf("didStartProvisionalLoad\n"));

    if (webView->mainFrame() != webFrame)
        return;

	BalWidget* widget = webView->viewWindow();

	if (widget && widget->browser)
	{
		char *url = (char*) webFrame->url();
		URL u(ParsedURLString, url);

		DoMethod(app, MM_URLPrefsGroup_ApplySettingsForURL, url, widget->webView);

		DoMethod(widget->browser, MM_OWBBrowser_DidStartProvisionalLoad, webFrame);

#if ENABLE(VIDEO)
		// Better ensure that we quit fullpage mode before loading something new
		DoMethod(widget->browser, MM_OWBBrowser_VideoEnterFullPage, NULL, FALSE);
#endif

		SetAttrs(widget->browser,
		         MA_OWBBrowser_Loading, TRUE,
				 MA_OWBBrowser_State, MV_OWBBrowser_State_Connecting,
				 MA_OWBBrowser_Zone, u.protocolIs("file") ? MV_OWBBrowser_Zone_Local : MV_OWBBrowser_Zone_Internet,
				 MA_OWBBrowser_Security, u.protocolIs("https") ? MV_OWBBrowser_Security_Secure : MV_OWBBrowser_Security_None,
		         TAG_DONE);

		if(widget->window && ((Object *)getv(widget->window, MA_OWBWindow_ActiveBrowser)) == widget->browser)
		{
			set(widget->browser, MA_OWBBrowser_Active, TRUE);
		}

		updateTitle(widget->browser, GSI(MSG_WEBVIEW_PROVISIONAL_TITLE), false);
		updateURL(widget->browser, url);
		updateNavigation(widget->browser);
		free(url);
	}
}

void MorphOSWebFrameDelegate::didCommitLoad(WebFrame* webFrame)
{
    WebView* webView = webFrame->webView();

	D(kprintf("didCommitLoad\n"));

    if (webView->mainFrame() != webFrame)
        return;

	BalWidget* widget = webView->viewWindow();

	if (widget && widget->browser)
	{
		char *url = (char*) webFrame->url();
		URL u(ParsedURLString, url);

		DoMethod(widget->browser, MM_OWBBrowser_DidCommitLoad, webFrame);

		DoMethod(app, MM_URLPrefsGroup_ApplySettingsForURL, url, widget->webView);

		SetAttrs(widget->browser,
		         MA_OWBBrowser_Loading, TRUE,
				 MA_OWBBrowser_State, MV_OWBBrowser_State_Loading,
				 MA_OWBBrowser_Zone, u.protocolIs("file") ? MV_OWBBrowser_Zone_Local : MV_OWBBrowser_Zone_Internet,
				 MA_OWBBrowser_Security, u.protocolIs("https") ? MV_OWBBrowser_Security_Secure : MV_OWBBrowser_Security_None,
		         TAG_DONE);

		updateURL(widget->browser, url);
		updateTitle(widget->browser, FilePart(url), false);
		updateNavigation(widget->browser);
		free(url);
	}
}

void MorphOSWebFrameDelegate::didFinishLoad(WebFrame* webFrame)
{
	WebView* webView = webFrame->webView();
	BalWidget* widget = webView->viewWindow();
    WebFrame* mainFrame = webView->mainFrame();

	D(kprintf("didFinishLoad\n"));

    if (mainFrame != webFrame)
        return;

	if(webView->scheduledScrollOffset() != IntPoint(0, 0))
	{
		webView->scrollBy(webView->scheduledScrollOffset());
		webView->setScheduledScrollOffset(IntPoint(0, 0));
	}

	if (widget && widget->browser)
	{
		char *url = (char *) mainFrame->url();
        URL u(ParsedURLString, url);
		Frame* coreFrame = core(mainFrame);
		String title = coreFrame->loader().documentLoader()->title().string();

		SetAttrs(widget->browser,
		         MA_OWBBrowser_Loading, FALSE,
				 MA_OWBBrowser_State, MV_OWBBrowser_State_Ready,
				 TAG_DONE);

		if(title.isEmpty())
		{
			if(u.protocolIs("file"))
			{
				updateTitle(widget->browser, FilePart(url), false);
			}
			else if(u.protocolIs("ftp"))
			{
				updateTitle(widget->browser, url, false);
			}
			else if(u.protocolIs("topsites"))
			{
				updateTitle(widget->browser, "Top Sites", false);
			}
		}

		updateNavigation(widget->browser);

		String wurl = coreFrame->loader().documentLoader()->url();
		DoMethod(app, MM_OWBApp_SetFormState, &wurl, coreFrame->document());

		// Don't record private browsing views
		if(!getv(widget->browser, MA_OWBBrowser_PrivateBrowsing))
		{
			DoMethod(app, MM_OWBApp_SaveSession, FALSE);
			TopSitesManager::getInstance().update(webView, u, title);
		}
		
		free(url);		
	}
}

void MorphOSWebFrameDelegate::didFailProvisionalLoad(WebFrame* webFrame, WebError* error)
{
	D(kprintf("didFailProvisionalLoad\n"));

	didFailLoad(webFrame, error);
}

void MorphOSWebFrameDelegate::didFailLoad(WebFrame* webFrame, WebError* error)
{
    WebView* webView = webFrame->webView();
    BalWidget* widget = webView->viewWindow();
    WebFrame* mainFrame = webView->mainFrame();
    // XXX: Is it right to filter it that way?
    bool handleAsError = !error->resourceError().isCancellation() && !error->isPolicyChangeError() && error->code() != 204 /*&& error->code() != 23*/;

    D(kprintf("didFailLoad code %d frame %p mainframe %p\n", error->code(), webFrame, mainFrame));

    if (mainFrame != webFrame)
        return;

	if (widget && widget->browser)
	{
		SetAttrs(widget->browser,
		         MA_OWBBrowser_Loading, FALSE,
				 MA_OWBBrowser_State, handleAsError ? MV_OWBBrowser_State_Error : MV_OWBBrowser_State_Ready,
				 TAG_DONE);
		
		updateNavigation(widget->browser);
	}

	if(error->resourceError().sslErrors())
	{
		//kprintf("SSL Error %d\n", error->resourceError().sslErrors());
	}

	if(handleAsError)
	{
		OWBFile f("PROGDIR:resource/error.html");

		if(getv(app, MA_OWBApp_ErrorMode) == MV_OWBApp_ErrorMode_Page && f.open('r') != -1)
		{
			char *fileContent = f.read(f.getSize());

			if(fileContent)
			{
				String message = String::format(GSI(MSG_WEBVIEW_ERROR_MSG_HTML),
										   error->code(),
										   error->resourceError().localizedDescription().utf8().data());

				String content = String::format(fileContent, error->resourceError().failingURL().utf8().data(), message.utf8().data());

				webFrame->loadHTMLString(content.utf8().data(), error->resourceError().failingURL().utf8().data());

				delete [] fileContent;
			}

			f.close();
		}
		else
		{
			char errorstring[512];
			String shorterror = error->resourceError().failingURL();
			shorterror.truncate(128);
			ULONG len = strescape(shorterror.latin1().data(), NULL);
			char failingURL[len+1];
			strescape(shorterror.latin1().data(), failingURL);

			snprintf(errorstring, sizeof(errorstring), GSI(MSG_WEBVIEW_ERROR_MSG), failingURL, error->code(), error->resourceError().localizedDescription().latin1().data());

			MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_ERROR_TITLE), GSI(MSG_REQUESTER_OK), errorstring, NULL);
		}
	}
}

void MorphOSWebFrameDelegate::didFinishDocumentLoadForFrame(WebFrame* webFrame)
{
	WebView* webView = webFrame->webView();
	
	D(kprintf("didFinishDocumentLoadForFrame\n"));

	BalWidget* widget = webView->viewWindow();

	if (widget && widget->browser)
	{
		updateNavigation(widget->browser);
	}
}

void MorphOSWebFrameDelegate::willCloseFrame(WebFrame* webFrame)
{
	WebView* webView = webFrame->webView();
	
	D(kprintf("willCloseFrame\n"));

	BalWidget* widget = webView->viewWindow();

	if (widget && widget->browser)
	{
		DoMethod(widget->browser, MM_OWBBrowser_WillCloseFrame, webFrame);
	}
}

void MorphOSWebFrameDelegate::didHandleOnloadEventsForFrame(WebFrame*)
{
	D(kprintf("didHandleOnloadEventsForFrame\n"));
}

void MorphOSWebFrameDelegate::didChangeLocationWithinPageForFrame(WebFrame*)
{
	D(kprintf("didChangeLocationWithinPageForFrame\n"));
}

void MorphOSWebFrameDelegate::didFirstLayoutInFrame(WebFrame*)
{
	D(kprintf("dispatchDidFirstLayout\n"));
}

void MorphOSWebFrameDelegate::didFirstVisuallyNonEmptyLayoutInFrame(WebFrame*)
{
	D(kprintf("dispatchDidFirstVisuallyNonEmptyLayout\n"));
}

void MorphOSWebFrameDelegate::titleChange(WebFrame* webFrame, const char* title)
{
	WebView* webView = webFrame->webView();

	D(kprintf("titleChange\n"));
	
	if (webView->mainFrame() != webFrame)
        return;

	BalWidget* widget = webView->viewWindow();
	if (widget && widget->browser)
	{
		updateTitle(widget->browser, (char *)title);
	}
}

void MorphOSWebFrameDelegate::dispatchNotEnoughMemory(WebFrame*)
{
	kprintf("dispatchNotEnoughMemory\n");
}

void MorphOSWebFrameDelegate::didDisplayInsecureContent(WebFrame* webFrame)
{
    WebView* webView = webFrame->webView();

	D(kprintf("didDisplayInsecureContent\n"));

    if (webView->mainFrame() != webFrame)
        return;

	BalWidget* widget = webView->viewWindow();

	if (widget && widget->browser)
	{
		SetAttrs(widget->browser,
				 MA_OWBBrowser_Security, MV_OWBBrowser_Security_NotSecure,
		         TAG_DONE);
	}
}

void MorphOSWebFrameDelegate::didRunInsecureContent(WebFrame* webFrame, WebSecurityOrigin*)
{
    WebView* webView = webFrame->webView();

	D(kprintf("didRunInsecureContent\n"));

    if (webView->mainFrame() != webFrame)
        return;

	BalWidget* widget = webView->viewWindow();

	if (widget && widget->browser)
	{
		SetAttrs(widget->browser,
				 MA_OWBBrowser_Security, MV_OWBBrowser_Security_NotSecure,
		         TAG_DONE);
	}
}

void MorphOSWebFrameDelegate::didChangeIcons(WebFrame* webFrame)
{
	D(kprintf("didChangeIcons\n"));
}

void MorphOSWebFrameDelegate::updateURL(void *browser, char *url)
{
	updateNavigation(browser);

	char *converted_url = utf8_to_local(url);

	if(converted_url)
	{
		SetAttrs((Object *)browser,
				 MA_OWBBrowser_URL, converted_url,
				 TAG_DONE);

		free(converted_url);
	}
}

void MorphOSWebFrameDelegate::updateTitle(void *browser, char *title, bool isUTF8)
{
	char * converted_title = title;

	if(isUTF8)
	{
		converted_title = utf8_to_local(title);
	}

	if(converted_title)
	{
		SetAttrs((Object *)browser,
				 MA_OWBBrowser_Title, converted_title,
				 TAG_DONE);
	}

	if(isUTF8 && converted_title)
	{
		free(converted_title);
	}
}

void MorphOSWebFrameDelegate::updateNavigation(void *browser)
{
    DoMethod((Object *)browser, MM_OWBBrowser_UpdateNavigation);
}

MorphOSWebFrameDelegate::MorphOSWebFrameDelegate()
{
}

MorphOSWebFrameDelegate::~MorphOSWebFrameDelegate()
{
}

/* MorphOSResourceLoadDelegate */

MorphOSResourceLoadDelegate::MorphOSResourceLoadDelegate()
{
}

MorphOSResourceLoadDelegate::~MorphOSResourceLoadDelegate()
{
}

void MorphOSResourceLoadDelegate::identifierForInitialRequest(WebView* webView, WebMutableURLRequest* request, WebDataSource* dataSource, unsigned long identifier)
{
	const char *url = request->_URL();
	kprintf("MorphOSResourceLoadDelegate::identifierForInitialRequest [%d - %s ]\n", identifier, url);
	free((char *)url);
}

WebMutableURLRequest* MorphOSResourceLoadDelegate::willSendRequest(WebView* webView, unsigned long identifier, WebMutableURLRequest* request, WebURLResponse* redirectResponse, WebDataSource* dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::willSendRequest(%d)\n", identifier);
    return request;
}

void MorphOSResourceLoadDelegate::didFinishLoadingFromDataSource(WebView* webView, unsigned long identifier, WebDataSource* dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::didFinishLoadingFromDataSource(%d)\n", identifier);
}

void MorphOSResourceLoadDelegate::didFailLoadingWithError(WebView* webView, unsigned long identifier, WebError* error, WebDataSource* dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::didFailLoadingWithError(%d)\n", identifier);
}

void MorphOSResourceLoadDelegate::didReceiveResponse(WebView *webView, unsigned long identifier, WebURLResponse *response, WebDataSource *dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::didReceiveResponse(%d)\n", identifier);
}

void MorphOSResourceLoadDelegate::didReceiveContentLength(WebView *webView, unsigned long identifier, unsigned length, WebDataSource *dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::didReceiveContentLength(%d)\n", identifier);
}

void MorphOSResourceLoadDelegate::didReceiveAuthenticationChallenge(WebView *webView, unsigned long identifier, WebURLAuthenticationChallenge *challenge, WebDataSource *dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::didReceiveAuthenticationChallenge(%d)\n", identifier);
}

void MorphOSResourceLoadDelegate::didCancelAuthenticationChallenge(WebView *webView, unsigned long identifier, WebURLAuthenticationChallenge *challenge, WebDataSource *dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::didCancelAuthenticationChallenge(%d)\n", identifier);
}

void MorphOSResourceLoadDelegate::plugInFailedWithError(WebView *webView, WebError *error, WebDataSource *dataSource)
{
	kprintf("MorphOSResourceLoadDelegate::plugInFailedWithError()\n");
}

/******/

WebViewPrivate::WebViewPrivate(WebView *webView)
    : m_webView(webView)
    , isInitialized(false)
	, m_closeWindowTimer(*this, &WebViewPrivate::closeWindowTimerFired)
{
	webView->setWebNotificationDelegate(MorphOSWebNotificationDelegate::createInstance());
	webView->setJSActionDelegate(MorphOSJSActionDelegate::createInstance());
	webView->setWebFrameLoadDelegate(MorphOSWebFrameDelegate::createInstance());
	webView->setDownloadDelegate(DownloadDelegateMorphOS::createInstance());
//	  webView->setPolicyDelegate(PolicyDelegateMorphOS::createInstance());
//	  webView->setWebResourceLoadDelegate(MorphOSResourceLoadDelegate::createInstance());
}

void WebViewPrivate::fireWebKitTimerEvents()
{
    fireTimerIfNeeded();
}


void WebViewPrivate::fireWebKitThreadEvents()
{
	WTF::dispatchFunctionsFromMainThread();
}

BalRectangle WebViewPrivate::onExpose(BalEventExpose event)
{
    Frame* frame = core(m_webView->mainFrame());
    if (!frame)
		return IntRect();

    if(!isInitialized) {
        isInitialized = true;
        frame->view()->resize(m_rect.width(), m_rect.height());
		frame->view()->forceLayout();
        frame->view()->adjustViewSize();
    }

    BalWidget* widget = m_webView->viewWindow();
    if (!widget->window)
		return IntRect();

	volatile double start = 0, layout = 0, paint = 0, blit = 0, inspector = 0; //
	if(renderBenchmark) start = currentTime(); //

    GraphicsContext ctx(widget->cr);
	IntRect rect(m_webView->dirtyRegion());

	//kprintf("WebViewPrivate::onExpose(%d,%d,%d,%d)\n", rect.x(), rect.y(), rect.width(), rect.height());

	if (frame->contentRenderer() && frame->view() && !rect.isEmpty() && !getv(widget->browser, MA_OWBBrowser_VideoElement))
	{
		bool coalesce = shouldCoalesce(rect);

		if(renderBenchmark) { kprintf("dirtyRegion [%d %d %d %d] rects %d coalesce %d\n", rect.x(), rect.y(), rect.width(), rect.height(), m_dirtyRegions.size(), coalesce); } //

		if(coalesce)
		{
			if(renderBenchmark) { kprintf("*** Coalescing rects\n"); } //

			clearDirtyRegion();

			frame->view()->updateLayoutAndStyleIfNeededRecursive();

			if(renderBenchmark)	{ layout = currentTime() - start; start = currentTime(); kprintf("Painting [%d %d %d %d]\n", rect.x(), rect.y(), rect.width(), rect.height()); } //

			ctx.save();
			ctx.clip(rect);
			frame->view()->paint(&ctx, rect);
			ctx.restore();

			if(renderBenchmark)	{ paint = currentTime() - start; start = currentTime(); kprintf("Painting inspector [%d %d %d %d]\n", rect.x(), rect.y(), rect.width(), rect.height()); } //

			updateView(widget, rect, false);

			if(renderBenchmark)	{ blit = currentTime() - start; } //
		}
		else
		{
			if(renderBenchmark) { kprintf("*** Not Coalescing rects\n"); } //

			Vector<IntRect> dirtyRegions = m_dirtyRegions; // urg
			clearDirtyRegion();

			frame->view()->updateLayoutAndStyleIfNeededRecursive();

			if(renderBenchmark)	{ layout = currentTime() - start; start = currentTime(); } //

			for(size_t i = 0; i < dirtyRegions.size(); i++)
			{
				if(renderBenchmark) { kprintf("Painting [%d %d %d %d]\n", dirtyRegions[i].x(), dirtyRegions[i].y(), dirtyRegions[i].width(), dirtyRegions[i].height()); }
				ctx.save();
				ctx.clip(dirtyRegions[i]);
				frame->view()->paint(&ctx, dirtyRegions[i]);
				ctx.restore();
/*
				ctx.save();
				ctx.clip(dirtyRegions[i]);
				frame->page()->inspectorController()->drawHighlight(ctx);
				ctx.restore();
*/
				if(renderBenchmark)	{ paint += currentTime() - start; start = currentTime(); kprintf("Blitting [%d %d %d %d]\n", dirtyRegions[i].x(), dirtyRegions[i].y(), dirtyRegions[i].width(), dirtyRegions[i].height()); } //

				updateView(widget, dirtyRegions[i], false);

				if(renderBenchmark)	{ blit += currentTime() - start; start = currentTime(); } //
			}
		}

		updateView(widget, rect, true);

		if(renderBenchmark) blit += currentTime() - start; //
    }

    if(renderBenchmark)
	{
		kprintf("WebViewPrivate::onExpose(%d,%d,%d,%d)\n  Layout: %f ms\n  Paint: %f ms\n  Inspector: %f ms\n  Blit: %f ms\n->Total: %f ms\n\n",
			rect.x(), rect.y(), rect.width(), rect.height(),
			layout*1000,
			paint*1000,
			inspector*1000,
			blit*1000,
			(layout + paint + blit + inspector)*1000
			);
	}

	return rect;
}

static cairo_status_t writeFunction(void* output, const unsigned char* data, unsigned int length)
{
    if (!reinterpret_cast<Vector<unsigned char>*>(output)->tryAppend(data, length))
        return CAIRO_STATUS_WRITE_ERROR;
    return CAIRO_STATUS_SUCCESS;
}

bool WebViewPrivate::screenshot(int &requested_width, int& requested_height, Vector<char> *imageData)
{
    Frame* frame = core(m_webView->mainFrame());
	if (frame && frame->view())
	{
		int view_width = m_rect.width();
		int view_height = m_rect.height();
		double scale_ratio = (double) (1.0 * requested_width) / view_width;
		
		requested_height = view_height * scale_ratio;

		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, requested_width, requested_height);

		if(surface)
		{
			cairo_t *cr = cairo_create(surface);

			if(cr)
			{
				if (frame->contentRenderer())
				{
					GraphicsContext ctx(cr);

					IntRect rect(m_rect);

					frame->view()->updateLayoutAndStyleIfNeededRecursive();
					
					ctx.save();
					ctx.scale(FloatSize(scale_ratio, scale_ratio));
					frame->view()->paintContents(&ctx, rect);
					ctx.restore();

					cairo_surface_write_to_png_stream(surface, writeFunction, imageData);

					return true;
				}

				cairo_destroy(cr);
			}

			cairo_surface_destroy(surface);
		}
	}
		
	return false;
}

bool WebViewPrivate::screenshot(String& path)
{
    Frame* frame = core(m_webView->mainFrame());
	if (frame && frame->view())
	{
		int view_width = m_rect.width();
		int view_height = m_rect.height();
		
		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, view_width, view_height);

		if(surface)
		{
			cairo_t *cr = cairo_create(surface);

			if(cr)
			{
				if (frame->contentRenderer())
				{
					GraphicsContext ctx(cr);

					IntRect rect(m_rect);

					frame->view()->updateLayoutAndStyleIfNeededRecursive();
					
					ctx.save();
					frame->view()->paintContents(&ctx, rect);
					ctx.restore();					
				    
					cairo_surface_write_to_png(surface, path.utf8().data());

					return true;
				}

				cairo_destroy(cr);
			}

			cairo_surface_destroy(surface);
		}
	}
		
	return false;
}

bool WebViewPrivate::onKeyDown(BalEventKey event)
{
	bool handled = false;
	Frame* frame = &(m_webView->page()->focusController().focusedOrMainFrame());
    if (!frame)
		return false;
    PlatformKeyboardEvent keyboardEvent(&event);

    if (frame->eventHandler().keyEvent(keyboardEvent))
		return true;

	if (IDCMP_RAWKEY == event.Class)
	{
        FrameView* view = frame->view();
		WebCore::ScrollDirection direction;
	    ScrollGranularity granularity;

		if(event.Code == RAWKEY_LEFT && (event.Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT)))
		{
			DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Back);
			return true;
		}

		if(event.Code == RAWKEY_RIGHT && (event.Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT)))
		{
            DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Forward);
			return true;
		}

		handled = true;

		switch (event.Code)
		{
			default:
				handled = false;
				break;

			case RAWKEY_DOWN:
	            direction = ScrollDown;

				if(event.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
				{
					granularity = ScrollByPage;
				}
				else
				{
                    granularity = ScrollByLine;
				}
                break;

			case RAWKEY_UP:				   
				direction = ScrollUp;

				if(event.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
				{
					granularity = ScrollByPage;
				}
				else
				{
					granularity = ScrollByLine;
				}
				break;

			case RAWKEY_RIGHT:
				direction = ScrollRight;

				if(event.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
				{
					granularity = ScrollByPage;
				}
				else
				{
                    granularity = ScrollByLine;
				}
                break;

			case RAWKEY_LEFT:
				direction = ScrollLeft;

				if(event.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
				{
					granularity = ScrollByPage;
				}
				else
				{
                    granularity = ScrollByLine;
				}
                break;

			case RAWKEY_SPACE:
				granularity = ScrollByPage;

				if(event.Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
				{
					direction = ScrollUp;
				}
				else
				{
                    direction = ScrollDown;
				}
				break;

	        case RAWKEY_PAGEDOWN:
				granularity = ScrollByPage;
				direction = ScrollDown;
                break;

			case RAWKEY_BACKSPACE:
	        case RAWKEY_PAGEUP:
				granularity = ScrollByPage;
				direction = ScrollUp;
                break;

	        case RAWKEY_HOME:
	            granularity = ScrollByDocument;
	            direction = ScrollUp;
                break;

	        case RAWKEY_END:
	            granularity = ScrollByDocument;
	            direction = ScrollDown;
                break;
		}

		if(handled)
		{
			int step = 0;
			int deltaX = 0, deltaY = 0;
			int total = 0, visible = 0;

			switch(direction)
			{
				case ScrollDown:
					visible = view->visibleHeight();
					total   = view->contentsHeight();
					deltaY  = 1;
					break;
				case ScrollUp:
					visible = view->visibleHeight();
					total   = view->contentsHeight();
					deltaY  = -1;
					break;
				case ScrollRight:
					visible = view->visibleWidth();
					total   = view->contentsWidth();
					deltaX  = 1;
					break;
				case ScrollLeft:
					visible = view->visibleWidth();
					total   = view->contentsWidth();
					deltaX  = -1;
					break;
			}

			switch(granularity)
			{
				case ScrollByPage:
					step = max(max<int>(visible * Scrollbar::minFractionToStepWhenPaging(), visible - Scrollbar::maxOverlapBetweenPages()), 1);
					break;
				case ScrollByLine:
					step = Scrollbar::pixelsPerLineStep();
					break;
				case ScrollByDocument:
					step = total;
					break;
				case ScrollByPixel:
				case ScrollByPrecisePixel:
					step = 1;
					break;
			}

			if (deltaX) deltaX *= step;
			if (deltaY) deltaY *= step;
#if OS(AROS)
            DoMethod(m_webView->viewWindow()->browser, MM_OWBBrowser_Autofill_HidePopup);
#endif
			view->scrollBy(IntSize(deltaX, deltaY));
			return handled;
		}

		handled = true;

		// Other keys
		switch (event.Code)
		{
			case RAWKEY_NM_WHEEL_UP:
			{
				if(event.Qualifier & IEQUALIFIER_CONTROL)
				{
                    m_webView->zoomPageIn();
				}
                break;
			}
			
			case RAWKEY_NM_WHEEL_DOWN:
			{
				if(event.Qualifier & IEQUALIFIER_CONTROL)
				{
					m_webView->zoomPageOut();
				}
                break;
			}

			case RAWKEY_F1:
			{
				DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Back);
                break;
	        }

	        case RAWKEY_F2:
	        {
				DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Forward);
                break;
	        }

	        case RAWKEY_F3:
			{
				m_webView->zoomPageIn();
                break;
			}

	        case RAWKEY_F4:
			{
				m_webView->zoomPageOut();
                break;
			}

			case RAWKEY_F5:
			{
				DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Reload, NULL);
				break;
			}

			case RAWKEY_F6:
			{
				DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Stop);
				break;
			}

			case RAWKEY_F8:
			{
				// Testing key
				//m_webView->page()->settings()->setUserStyleSheetLocation(URL(ParsedURLString, "file:///resource/rss.css"));
				m_webView->screenshot("ram:owb_screenshot.png");

				break;
			}

			case RAWKEY_F9:
			{
				DoMethod(app, MM_OWBApp_RestoreSession, FALSE, TRUE);
				break;
			}

			case RAWKEY_F10:
			{
				DoMethod(app, MM_OWBApp_SaveSession, TRUE);
				break;
			}

			case RAWKEY_F11:
			{
				DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_FullScreen, MV_OWBWindow_FullScreen_Toggle);
				break;
			}

			case RAWKEY_F12:
			{
				MemoryCache::Statistics stats = MemoryCache::singleton().getStatistics();

				kprintf("Statistics about cache:\n");
				kprintf("\timages: count=%d - size=%d - liveSize=%d - decodedSize=%d\n", stats.images.count, stats.images.size, stats.images.liveSize, stats.images.decodedSize);
				kprintf("\tcssStyleSheets: count=%d - size=%d - liveSize=%d - decodedSize=%d\n", stats.cssStyleSheets.count, stats.cssStyleSheets.size, stats.cssStyleSheets.liveSize, stats.cssStyleSheets.decodedSize);
				kprintf("\tscripts: count=%d - size=%d - liveSize=%d - decodedSize=%d\n", stats.scripts.count, stats.scripts.size, stats.scripts.liveSize, stats.scripts.decodedSize);
				kprintf("\tfonts: count=%d - size=%d - liveSize=%d - decodedSize=%d\n", stats.fonts.count, stats.fonts.size, stats.fonts.liveSize, stats.fonts.decodedSize);

				kprintf("\nStatistics about JavaScript Heap:\n");


#if OS(MORPHOS)
				kprintf("\tmemory allocated by allocator: %d\n", WTF::memory_consumption);
#endif
				kprintf("\theap: used %d - total %d\n", JSDOMWindow::commonVM().heap.size(), JSDOMWindow::commonVM().heap.capacity());

				kprintf("\nPruning caches and running Garbage collector.\n");
				
				MemoryPressureHandler::singleton().releaseMemory(Critical::Yes, Synchronous::Yes);

				if(getenv("OWB_RESETVM"))
				{
				    DoMethod(app, MUIM_Application_PushMethod, app, 1, MM_OWBApp_ResetVM);
				}

				kprintf("Done\n\n\n");

				break;
			}

			default:
			{
				struct InputEvent ie;
				char c;

				ie.ie_NextEvent = NULL;
				ie.ie_Class = IECLASS_RAWKEY;
				ie.ie_SubClass = 0;
				ie.ie_Code = event.Code & ~IECODE_UP_PREFIX;
				ie.ie_Qualifier = 0;
				ie.ie_EventAddress = NULL;

				if (MapRawKey(&ie, (char *)&c, 1, NULL) == 1)
				{
					if (event.Qualifier & (IEQUALIFIER_CONTROL | AMIGAKEYS))
					{
						switch(c)
						{
					        case 'c':
								frame->editor().command("Copy").execute();
								break;
					        case 'x':
								frame->editor().command("Cut").execute();
								break;
							case 'v':
								frame->editor().command("Paste").execute();
					            break;
							case 'r':
								DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Reload, NULL);
								break;
							case 's':
								DoMethod(m_webView->viewWindow()->window, MM_OWBWindow_Stop);
								break;
							default:
								handled = false;
								break;
						}
					}
				}
			}
        }
    }

	return handled;
}

bool WebViewPrivate::onKeyUp(BalEventKey event)
{
    Frame* frame = &(m_webView->page()->focusController().focusedOrMainFrame());
    if (!frame)
		return false;
    PlatformKeyboardEvent keyboardEvent(&event);

    if (frame->eventHandler().keyEvent(keyboardEvent))
		return true;

	return false;
}

bool WebViewPrivate::onMouseMotion(BalEventMotion event)
{
    Frame* frame = core(m_webView->mainFrame());
    if (!frame)
		return false;
	return frame->eventHandler().mouseMoved(PlatformMouseEvent(&event));
}

bool WebViewPrivate::onMouseButtonDown(BalEventButton event)
{
    Frame* frame = core(m_webView->mainFrame());
	bool handled = false;

    if (!frame)
		return handled;

	PlatformMouseEvent mouseEvent = PlatformMouseEvent(&event);

	// If an url is mmb clicked, consider the event wasn't handled.
	if(mouseEvent.type() == PlatformEvent::MousePressed && mouseEvent.button() == MiddleButton)
	{
		FrameView* v = frame->view();

		if(v)
		{
			IntPoint viewportPos = v->windowToContents(mouseEvent.position());
			IntPoint point = IntPoint(viewportPos.x(), viewportPos.y());
		    HitTestResult result(point);

			result = frame->eventHandler().hitTestResultAtPoint(point, false);

			if(result.innerNonSharedNode() && !result.absoluteLinkURL().string().isEmpty())
			{
				handled = true;
			}
		}

		return handled;
	}

	handled = frame->eventHandler().handleMousePressEvent(PlatformMouseEvent(mouseEvent));
	return handled;
}

bool WebViewPrivate::onMouseButtonUp(BalEventButton event)
{
	Frame* frame = core(m_webView->mainFrame());
	bool handled = false;

	if(!frame)
		return handled;

	PlatformMouseEvent mouseEvent = PlatformMouseEvent(&event);

	/* XXX: Highly hacky IMO, not the right place for that */
	if(mouseEvent.type() == PlatformEvent::MouseReleased && mouseEvent.button() == MiddleButton)
	{
		FrameView* v = frame->view();
		if (!v) return handled;

		IntPoint viewportPos = v->windowToContents(mouseEvent.position());
		IntPoint point = IntPoint(viewportPos.x(), viewportPos.y());
	    HitTestResult result(point);

		result = frame->eventHandler().hitTestResultAtPoint(point, false);

		if (!result.innerNonSharedNode()) return handled;

		URL urlToLoad = result.absoluteLinkURL();

		if(!urlToLoad.string().isEmpty())
		{
			if(Page* oldPage = frame->page())
			{
		        WindowFeatures features;

				switch(getv(app, MA_OWBApp_MiddleButtonPolicy))
				{
					case MV_OWBApp_MiddleButtonPolicy_NewBackgroundTab:
						features.newPagePolicy = NEWPAGE_POLICY_TAB;
						features.donotactivate = true;
						break;
					case MV_OWBApp_MiddleButtonPolicy_NewTab:
						features.newPagePolicy = NEWPAGE_POLICY_TAB;
						features.donotactivate = false;
						break;
					case MV_OWBApp_MiddleButtonPolicy_NewWindow:
						features.newPagePolicy = NEWPAGE_POLICY_WINDOW;
						features.donotactivate = false;
						break;
				}

				if (Page* newPage = oldPage->chrome().createWindow(
				        frame,
				        FrameLoadRequest(frame, ResourceRequest(urlToLoad, frame->loader().outgoingReferrer()), ShouldOpenExternalURLsPolicy::ShouldNotAllow),
				        features, NavigationAction()))
		            newPage->chrome().show();
		    }

			handled = true;
		}

		return handled;
	}

	handled = frame->eventHandler().handleMouseReleaseEvent(mouseEvent);
	return handled;
}

bool WebViewPrivate::onScroll(BalEventScroll event)
{
    Frame* frame = core(m_webView->mainFrame());
    if (!frame)
		return false;
    PlatformWheelEvent wheelEvent(&event);
	return frame->eventHandler().handleWheelEvent(wheelEvent);
}

void WebViewPrivate::onResize(BalResizeEvent event)
{
    Frame* frame = core(m_webView->mainFrame());
    if (!frame)
        return;
    m_rect.setWidth(event.w);
    m_rect.setHeight(event.h);
    frame->view()->resize(event.w, event.h);
	frame->view()->forceLayout();
    frame->view()->adjustViewSize();
}

void WebViewPrivate::onQuit(BalQuitEvent)
{
}

void WebViewPrivate::onUserEvent(BalUserEvent)
{
}

void WebViewPrivate::popupMenuHide()
{
}


void WebViewPrivate::popupMenuShow(void *popupInfo)
{
	BalWidget *widget = m_webView->viewWindow();

	if(widget)
	{
		DoMethod(widget->browser, MM_OWBBrowser_PopupMenu, popupInfo);
	}
}

void WebViewPrivate::updateView(BalWidget *widget, IntRect rect, bool sync)
{
	// kprintf("updateview(%p (%p),(%d,%d,%d,%d))\n", widget, widget?widget->window:NULL, rect.x(), rect.y(), rect.width(), rect.height());

    if (!widget || !widget->window || rect.isEmpty())
        return;

	rect.intersect(m_rect);

	DoMethod(widget->browser, MM_OWBBrowser_Update, &rect, sync);
}

void WebViewPrivate::sendExposeEvent(IntRect)
{
    if (m_webView->viewWindow())
        m_webView->viewWindow()->expose = true;

	// Signal it, to make it happen a bit faster ?
}

void WebViewPrivate::repaint(const WebCore::IntRect& windowRect, bool contentChanged, bool immediate, bool repaintContentOnly)
{
	// kprintf("WebViewPrivate::repaint([%d %d %d %d], %d, %d , %d\n", windowRect.x(), windowRect.y(), windowRect.width(), windowRect.height(), contentChanged, immediate, repaintContentOnly);

    if (windowRect.isEmpty())
        return;
    IntRect rect = windowRect;
    rect.intersect(m_rect);

	// kprintf("repaint: contentChanged %d immediate %d repaintContentOnly %d\n", contentChanged, immediate, repaintContentOnly);

    if (rect.isEmpty())
        return;

    if (contentChanged) {
        m_webView->addToDirtyRegion(rect);
#if 0
        Frame* focusedFrame = m_webView->page()->focusController()->focusedFrame();
        if (focusedFrame) {
            Scrollbar* hBar = focusedFrame->view()->horizontalScrollbar();
            Scrollbar* vBar = focusedFrame->view()->verticalScrollbar();

            // TODO : caculate the scroll delta and test this.
            //if (dx && hBar)
            if (hBar)
                m_webView->addToDirtyRegion(IntRect(focusedFrame->view()->windowClipRect().x() + hBar->x(), focusedFrame->view()->windowClipRect().y() + hBar->y(), hBar->width(), hBar->height()));
            //if (dy && vBar)
            if (vBar)
                m_webView->addToDirtyRegion(IntRect(focusedFrame->view()->windowClipRect().x() + vBar->x(), focusedFrame->view()->windowClipRect().y() + vBar->y(), vBar->width(), vBar->height()));
        }
#endif
    }
    if (!repaintContentOnly)
	{
        sendExposeEvent(rect);
	}
	if (immediate)
	{
        if (repaintContentOnly)
		{
            m_webView->updateBackingStore(core(m_webView->topLevelFrame())->view());
		}
        else
		{
            sendExposeEvent(rect);
		}
    }
}

void WebViewPrivate::scrollBackingStore(WebCore::FrameView* view, int dx, int dy, const WebCore::IntRect& scrollViewRect, const WebCore::IntRect& clipRect)
{
	double start = 0;
	if(renderBenchmark)
	{
		start = currentTime();
		kprintf("WebViewPrivate::scrollBackingStore(%d, %d, scrollViewRect[%d, %d, %d, %d], clipRect[%d, %d, %d, %d])\n", dx, dy,
								scrollViewRect.x(), scrollViewRect.y(), scrollViewRect.width(), scrollViewRect.height(),
								clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
		kprintf("  dirtyRegion [%d, %d, %d, %d]\n", m_webView->dirtyRegion().x, m_webView->dirtyRegion().y, m_webView->dirtyRegion().w, m_webView->dirtyRegion().h);
	}

	m_backingStoreDirtyRegion.move(dx, dy);
	for(size_t i = 0; i < m_dirtyRegions.size(); i++)
	{
		m_dirtyRegions[i].move(dx, dy);
	}

    BalWidget* widget = m_webView->viewWindow();
    if (!widget || !widget->window)
        return;

    IntRect updateRect = clipRect;
    updateRect.intersect(scrollViewRect);
    
    int x = updateRect.x();
    int y = updateRect.y();
    int width = updateRect.width();
    int height = updateRect.height();
    int dirtyX = 0, dirtyY = 0, dirtyW = 0, dirtyH = 0;

    dx = -dx;
    dy = -dy;

    if (dy == 0 && dx < 0 && -dx < width) {
        dirtyX = x;
        dirtyY = y;
        dirtyW = -dx;
        dirtyH = height;
    }
    else if (dy == 0 && dx > 0 && dx < width) {
        dirtyX = x + width - dx;
        dirtyY = y;
        dirtyW = dx;
        dirtyH = height;
    }
    else if (dx == 0 && dy < 0 && -dy < height) {
        dirtyX = x;
        dirtyY = y;
        dirtyW = width;
        dirtyH = -dy;
    }
    else if (dx == 0 && dy > 0 && dy < height) {
        dirtyX = x;
        dirtyY = y + height - dy;
        dirtyW = width;
        dirtyH = dy;
    }

	//kprintf("new dirtyRegion [%d, %d, %d, %d]\n", dirtyX, dirtyY, dirtyW, dirtyH);

	if (dirtyX || dirtyY || dirtyW || dirtyH)
	{
		WebCore::IntRect r = WebCore::IntRect(x, y, width, height);

		if(renderBenchmark) { kprintf("  Scroll d=[%d %d] r=[%d, %d, %d, %d]\n", dx, dy, r.x(), r.y(), r.width(), r.height()); } //

		DoMethod(widget->browser, MM_OWBBrowser_Scroll, dx, dy, &r);
        m_webView->addToDirtyRegion(IntRect(dirtyX, dirtyY, dirtyW, dirtyH));
    }
    else
	{
        m_webView->addToDirtyRegion(updateRect);
	}

	// Sync scrollers here (less lag feeling that way, since the rest isn't synchron)
	DoMethod(widget->browser, MM_OWBBrowser_UpdateScrollers);

	//kprintf("new dirtyRegion [%d, %d, %d, %d]\n", m_webView->dirtyRegion().x, m_webView->dirtyRegion().y, m_webView->dirtyRegion().w, m_webView->dirtyRegion().h);

	sendExposeEvent(updateRect); // only processed at next expose event, potential lag
	//onExpose(0);               // immediate, but scrolling sideffect with fixed elements

   if(renderBenchmark) { kprintf("WebViewPrivate::scrollBackingStore()\n  Scroll: %f ms\n", currentTime() - start); } //
}

/* Implement these properly */

void WebViewPrivate::repaintAfterNavigationIfNeeded()
{
	m_webView->addToDirtyRegion(IntRect(0, 0, m_rect.width(), m_rect.height()));
	onExpose(0);
}

void WebViewPrivate::resize(BalRectangle r)
{
}

void WebViewPrivate::move(BalPoint lastPos, BalPoint newPos)
{
}

void WebViewPrivate::closeWindowSoon()
{
    m_closeWindowTimer.startOneShot(0);
}

void WebViewPrivate::closeWindowTimerFired()
{
    closeWindow();
}

void WebViewPrivate::closeWindow()
{
	BalWidget* widget = m_webView->viewWindow();
	if(widget)
	{
		DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveBrowser, widget->browser);
	}
}
