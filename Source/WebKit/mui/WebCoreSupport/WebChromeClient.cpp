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
#include "WebChromeClient.h"
#include "ColorChooserController.h"
#include "DateTimeChooserController.h"
#include "ColorChooser.h"
#include "DateTimeChooser.h"
#include "FileChooser.h"
#include "FileIconLoader.h"
#include "JSActionDelegate.h"
#include "WebDesktopNotificationsDelegate.h"
#include "WebHitTestResults.h"
#include "WebFrame.h"
#include "WebFrameLoadDelegate.h"
#include "WebHistory.h"
#include "WebHistoryDelegate.h"
#include "WebMutableURLRequest.h"
#include "WebPreferences.h"
#include "WebSecurityOrigin.h"
#include "WebView.h"
#include "WebViewWindow.h"

#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <ContextMenu.h>
#include <FileChooser.h>
#include <FloatRect.h>
#include <Frame.h>
#include <FrameLoadRequest.h>
#include <FrameView.h>
#include <HTMLNames.h>
#include <Icon.h>
#include <IntRect.h>
#include <NavigationAction.h> 
#include <NotImplemented.h>
#include <Page.h>
#include <SecurityOrigin.h>
#include <PopupMenuMorphOS.h>
#include <SearchPopupMenuMorphOS.h>
#include <WindowFeatures.h>
#include "gui.h"
#include "utils.h"
#include <proto/intuition.h>
#include <clib/debug_protos.h>
#include <proto/asl.h>
#include "asl.h"
#undef get
#undef String

#include <cstdio>

using namespace WebCore;

WebChromeClient::WebChromeClient(WebView* webView)
    : m_webView(webView)
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    , m_notificationsDelegate(std::make_unique<WebDesktopNotificationsDelegate>(webView))
#endif
{
}

WebChromeClient::~WebChromeClient()
{
}

void WebChromeClient::chromeDestroyed()
{
    delete this;
}

void WebChromeClient::setWindowRect(const FloatRect& r)
{
}

FloatRect WebChromeClient::windowRect()
{
    return FloatRect();
}

FloatRect WebChromeClient::pageRect()
{
    IntRect p(m_webView->frameRect());
    return FloatRect(p);
}

void WebChromeClient::focus()
{
    // FIXME: Win uses its IWebUIDelegate here.
    // Win should call this method from the delegate (which we do not have). So match what Qt does here and just update our focus,
    // which is needed by WebCore.
    m_webView->setFocus();
}

void WebChromeClient::unfocus()
{
    // FIXME: Win uses its IWebUIDelegate here.
    // See comment in focus().
    m_webView->clearFocus();
}

bool WebChromeClient::canTakeFocus(FocusDirection direction)
{
    // FIXME: Win uses its IWebUIDelegate here.
    return true;
}

void WebChromeClient::takeFocus(FocusDirection direction)
{
    BalWidget *widget = m_webView->viewWindow();
    if(widget) 
		DoMethod(widget->browser, MM_OWBBrowser_ReturnFocus);
}

void WebChromeClient::focusedElementChanged(Element*) 
{
}

void WebChromeClient::focusedFrameChanged(WebCore::Frame*)
{
}

Page* WebChromeClient::createWindow(Frame*, const FrameLoadRequest& frameLoadRequest, const WindowFeatures& features, const WebCore::NavigationAction&)
{
	if (features.dialog)
	{
		kprintf("%s: features.dialog not implemented on MorphOS.\n", __PRETTY_FUNCTION__);
		return 0;
	}

	//kprintf("WebChromeClient::createWindow(url: <%s> framename: <%s> empty: %d)\n", frameLoadRequest.resourceRequest().url().string().utf8().data(), frameLoadRequest.frameName().utf8().data(),  frameLoadRequest.isEmpty());

	ULONG frame = frameLoadRequest.isEmpty();
	ULONG privatebrowsing = getv(m_webView->viewWindow()->browser, MA_OWBBrowser_PrivateBrowsing);
	BalWidget *widget = NULL;

	if(features.newPagePolicy == NEWPAGE_POLICY_WINDOW)
	{
	    widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddWindow, frame ? frameLoadRequest.frameName().utf8().data() : frameLoadRequest.resourceRequest().url().string().utf8().data(), frame, NULL, FALSE, &features, privatebrowsing);
	}
	else if(features.newPagePolicy == NEWPAGE_POLICY_TAB)
	{
	    widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddBrowser, NULL, frame ? frameLoadRequest.frameName().utf8().data() : frameLoadRequest.resourceRequest().url().string().utf8().data(), frame, NULL, features.donotactivate? TRUE : FALSE, privatebrowsing, FALSE);
	}
	else /* User settings are considered for new page */
	{
		ULONG popuppolicy = getv(app, MA_OWBApp_PopupPolicy);

		if(popuppolicy == MV_OWBApp_NewPagePolicy_Window)
		{
		    widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddWindow, frame ? frameLoadRequest.frameName().utf8().data() : frameLoadRequest.resourceRequest().url().string().utf8().data(), frame, NULL, FALSE, &features, privatebrowsing);
		}
		else if(popuppolicy == MV_OWBApp_NewPagePolicy_Tab)
		{
		    widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddBrowser, NULL, frame ? frameLoadRequest.frameName().utf8().data() : frameLoadRequest.resourceRequest().url().string().utf8().data(), frame, NULL, features.donotactivate? TRUE : FALSE, privatebrowsing, FALSE);
		}
	}

	if(widget)
	{
		return core(widget->webView);
	}

    return 0;
}

void WebChromeClient::show()
{
}

bool WebChromeClient::canRunModal()
{
    return false;
}

void WebChromeClient::runModal()
{
}

void WebChromeClient::setToolbarsVisible(bool visible)
{
    m_webView->setToolbarsVisible(visible);
}

bool WebChromeClient::toolbarsVisible()
{
    return m_webView->toolbarsVisible();
}

void WebChromeClient::setStatusbarVisible(bool visible)
{
    m_webView->setStatusbarVisible(visible);
}

bool WebChromeClient::statusbarVisible()
{
    return m_webView->statusbarVisible();
}

void WebChromeClient::setScrollbarsVisible(bool b)
{
    WebFrame* webFrame = m_webView->topLevelFrame();
    if (webFrame)
        webFrame->setAllowsScrolling(b);
}

bool WebChromeClient::scrollbarsVisible()
{
    WebFrame* webFrame = m_webView->topLevelFrame();
    bool b = false;
    if (webFrame)
        b = webFrame->allowsScrolling();

    return !!b;
}

void WebChromeClient::setMenubarVisible(bool visible)
{
    m_webView->setMenubarVisible(visible);
}

bool WebChromeClient::menubarVisible()
{
    return m_webView->menubarVisible();
}

void WebChromeClient::setResizable(bool resizable)
{
}

void WebChromeClient::addMessageToConsole(MessageSource source, MessageLevel level, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID)
{
    SharedPtr<JSActionDelegate> jsActionDelegate = m_webView->jsActionDelegate();
    if (jsActionDelegate)
        jsActionDelegate->consoleMessage(m_webView->mainFrame(), lineNumber, columnNumber, message.utf8().data());
}

bool WebChromeClient::canRunBeforeUnloadConfirmPanel()
{
    return false;
}

bool WebChromeClient::runBeforeUnloadConfirmPanel(const String& message, Frame* frame)
{
    kprintf("runBeforeUnloadConfirmPanel: %s\n", message.utf8().data());
    return false;
}

void WebChromeClient::closeWindowSoon()
{
    // We need to remove the parent WebView from WebViewSets here, before it actually
    // closes, to make sure that JavaScript code that executes before it closes
    // can't find it. Otherwise, window.open will select a closed WebView instead of 
    // opening a new one <rdar://problem/3572585>.

    // We also need to stop the load to prevent further parsing or JavaScript execution
    // after the window has torn down <rdar://problem/4161660>.
  
    // FIXME: This code assumes that the UI delegate will respond to a webViewClose
    // message by actually closing the WebView. Safari guarantees this behavior, but other apps might not.
    // This approach is an inherent limitation of not making a close execute immediately
    // after a call to window.close.

    m_webView->setGroupName("");
    m_webView->mainFrame()->stopLoading();
    m_webView->closeWindowSoon();
}

void WebChromeClient::runJavaScriptAlert(Frame* frame, const String& message)
{
    SharedPtr<JSActionDelegate> jsActionDelegate = m_webView->jsActionDelegate();
    if (jsActionDelegate)
        jsActionDelegate->jsAlert(m_webView->mainFrame(), message.utf8().data());
}

bool WebChromeClient::runJavaScriptConfirm(Frame *frame, const String& message)
{
    SharedPtr<JSActionDelegate> jsActionDelegate = m_webView->jsActionDelegate();
    if (jsActionDelegate)
        return jsActionDelegate->jsConfirm(m_webView->mainFrame(), message.utf8().data());
    return false;
}

bool WebChromeClient::runJavaScriptPrompt(Frame *frame, const String& message, const String& defaultValue, String& result)
{
    char* value = 0;
    SharedPtr<JSActionDelegate> jsActionDelegate = m_webView->jsActionDelegate();
    if (jsActionDelegate)
    {
		if(jsActionDelegate->jsPrompt(m_webView->mainFrame(), message.utf8().data(), defaultValue.utf8().data(), &value))
		{
			result = value;
			free(value);
			return true;
		}
    }
    return false;
}

void WebChromeClient::setStatusbarText(const String& statusText)
{
    BalWidget *widget = m_webView->viewWindow();

	if(widget)
	{
		CString statusUT8 = statusText.utf8();
		char *converted_status = utf8_to_local(statusUT8.data());

		SetAttrs(widget->browser, MA_OWBBrowser_StatusText, converted_status, TAG_DONE);

		free(converted_status);
	}
}

bool WebChromeClient::shouldInterruptJavaScript()
{
	BalWidget* widget = m_webView->viewWindow();

	if(widget)
	{
		return MUI_RequestA(app, widget->window, 0, GSI(MSG_WEBVIEW_JSACTION_TITLE), GSI(MSG_REQUESTER_YES_NO), GSI(MSG_WEBVIEW_INTERRUPT_SCRIPT), NULL);
	}
    return false;
}

KeyboardUIMode WebChromeClient::keyboardUIMode()
{
    WebPreferences* preferences = m_webView->preferences();
    return preferences->tabsToLinks() ? KeyboardAccessTabsToLinks : KeyboardAccessDefault;
}

bool WebChromeClient::tabsToLinks() const
{
    WebPreferences* preferences = m_webView->preferences();
    return preferences->tabsToLinks();
}

IntRect WebChromeClient::windowResizerRect() const
{
    return IntRect();
}

#if ENABLE(REGISTER_PROTOCOL_HANDLER)
void WebChromeClient::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title)
{
}
#endif

void WebChromeClient::invalidateRootView(const IntRect& windowRect)
{
    ASSERT(core(m_webView->topLevelFrame()));
    //kprintf("WebChromeClient::invalidateRootView([%d, %d, %d, %d], immediate %d)\n", windowRect.x(), windowRect.y(), windowRect.width(), windowRect.height(), immediate);
    m_webView->repaint(windowRect, false /*contentChanged*/, true /*immediate*/, false /*repaintContentOnly*/);
}

void WebChromeClient::invalidateContentsAndRootView(const IntRect& windowRect)
{
    ASSERT(core(m_webView->topLevelFrame()));
    //kprintf("WebChromeClient::invalidateContentsAndRootView([%d, %d, %d, %d], immediate %d)\n", windowRect.x(), windowRect.y(), windowRect.width(), windowRect.height(), immediate);
    m_webView->repaint(windowRect, true /*contentChanged*/, true /*immediate*/, false /*repaintContentOnly*/);
}

void WebChromeClient::invalidateContentsForSlowScroll(const IntRect& windowRect)
{
    ASSERT(core(m_webView->topLevelFrame()));
    //kprintf("WebChromeClient::invalidateContentsForSlowScroll([%d, %d, %d, %d], immediate %d)\n", windowRect.x(), windowRect.y(), windowRect.width(), windowRect.height(), immediate);
    m_webView->repaint(windowRect, true /*contentChanged*/, true /*immediate*/, true /*repaintContentOnly*/);
}

void WebChromeClient::scroll(const IntSize& delta, const IntRect& scrollViewRect, const IntRect& clipRect)
{
    ASSERT(core(m_webView->topLevelFrame()));
    //kprintf("WebChromeClient::scroll(delta[%d, %d], scrollViewRect[%d, %d, %d, %d], clipRect[%d, %d, %d, %d])\n", delta.width(), delta.height(), scrollViewRect.x(), scrollViewRect.y(), scrollViewRect.width(), scrollViewRect.height(), clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
    m_webView->scrollBackingStore(core(m_webView->topLevelFrame())->view(), delta.width(), delta.height(), scrollViewRect, clipRect);
}

IntRect WebChromeClient::rootViewToScreen(const IntRect& rect) const
{
    return rect;
}

PlatformPageClient WebChromeClient::platformPageClient() const
{
    return m_webView->viewWindow();
} 

void WebChromeClient::contentsSizeChanged(Frame*, const IntSize&) const
{
}

void WebChromeClient::mouseDidMoveOverElement(const HitTestResult& result, unsigned modifierFlags)
{

}

bool WebChromeClient::shouldUnavailablePluginMessageBeButton(RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason) const
{
    if (pluginUnavailabilityReason != RenderEmbeddedObject::PluginMissing) 
        return false;
    return true;
}

void WebChromeClient::unavailablePluginButtonClicked(Element* element, RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason) const
{
	BalWidget *widget = m_webView->viewWindow();
	if(widget)
		DoMethod(widget->window, MM_OWBWindow_LoadURL, "http://fabportnawak.free.fr/owb/plugins/"); // Just a test
}

void WebChromeClient::setToolTip(const String& toolTip, TextDirection)
{
    m_webView->setToolTip(toolTip.utf8().data());
}

void WebChromeClient::print(Frame* frame)
{
	DoMethod((Object *) getv(app, MA_OWBApp_PrinterWindow), MM_PrinterWindow_PrintDocument, frame);
}

void WebChromeClient::exceededDatabaseQuota(Frame* frame, const String& databaseIdentifier, DatabaseDetails)
{
    WebSecurityOrigin *origin = WebSecurityOrigin::createInstance(frame->document()->securityOrigin());
    SharedPtr<JSActionDelegate> jsActionDelegate = m_webView->jsActionDelegate();
    if (jsActionDelegate)
        jsActionDelegate->exceededDatabaseQuota(m_webView->mainFrame(), origin, databaseIdentifier.utf8().data());
    else {
        const unsigned long long defaultQuota = 5 * 1024 * 1024; // 5 megabytes should hopefully be enough to test storage support.
        origin->setQuota(defaultQuota);
    }
    delete origin;
}

#if ENABLE(DASHBOARD_SUPPORT)
void WebChromeClient::dashboardRegionsChanged()
{
    // This option is not supported so use it at your own risk.
    ASSERT_NOT_REACHED();
}
#endif

#include "ApplicationCacheStorage.h"
void WebChromeClient::reachedMaxAppCacheSize(int64_t spaceNeeded)
{
    // FIXME: Free some space.
    notImplemented();
}

void WebChromeClient::reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t)
{
	notImplemented();
}

#if ENABLE(INPUT_TYPE_COLOR)
std::unique_ptr<ColorChooser> WebChromeClient::createColorChooser(ColorChooserClient* chooserClient, const Color&)
{
    std::unique_ptr<ColorChooserController> controller = std::unique_ptr<ColorChooserController>(new ColorChooserController(this, chooserClient));
    controller->openUI();
    return std::move(controller);
}
#endif

#if ENABLE(INPUT_TYPE_DATE)
PassRefPtr<DateTimeChooser> WebChromeClient::openDateTimeChooser(DateTimeChooserClient* chooserClient, const DateTimeChooserParameters& parameters)
{
    /*
    kprintf("WebChromeClient::openDateTimeChooser\n");

    RefPtr<DateTimeChooserController> controller = adoptRef(new DateTimeChooserController(this, chooserClient, parameters));
    controller->openUI();
    return controller.release();    
    */

    PassRefPtr<DateTimeChooser> ret;
    return ret;
}
#endif

void WebChromeClient::runOpenPanel(Frame*, PassRefPtr<FileChooser> prpFileChooser)
{
    RefPtr<FileChooser> chooser = prpFileChooser;
    bool multiFile = chooser->settings().allowsMultipleFiles;

    if(multiFile)
    {
		APTR tags[] = { (APTR) ASLFR_TitleText, (APTR) GSI(MSG_WEBVIEW_UPLOAD_FILES),
						(APTR) ASLFR_InitialPattern, (APTR) "#?",
						TAG_DONE };

		ULONG count;
		char ** files = NULL;
	    Vector<String> names;

		count = asl_run_multiple((char *)getv(app, MA_OWBApp_CurrentDirectory), (struct TagItem *) &tags, &files, TRUE);

		if(files)
		{
			ULONG i;
			for(i = 0; i < count; i++)
			{
				//kprintf("file = %s\n", files[i]);
				names.append(String(files[i]));
			}

			chooser->chooseFiles(names);
			asl_free(count, files);
		}
	}
	else
	{
		APTR tags[] = { (APTR) ASLFR_TitleText, (APTR) GSI(MSG_WEBVIEW_UPLOAD_FILE),
						(APTR) ASLFR_InitialPattern, (APTR) "#?",
						TAG_DONE };

		char *file = asl_run((char *)getv(app, MA_OWBApp_CurrentDirectory), (struct TagItem *) &tags, TRUE);

		if(file)
		{
			//kprintf("file = %s\n", file);
			chooser->chooseFile(String(file));
			FreeVecTaskPooled(file);
		}
	}
}

void WebChromeClient::loadIconForFiles(const Vector<WTF::String>& filenames, WebCore::FileIconLoader* loader)   
{
   loader->notifyFinished(Icon::createIconForFiles(filenames));
}

void WebChromeClient::setCursor(const Cursor& cursor)
{
	//FIXME: implement me!
	/*
	HCURSOR platformCursor = cursor.platformCursor()->nativeCursor();
	if (!platformCursor)
		return;
	if (COMPtr<IWebUIDelegate> delegate = uiDelegate()) {
		COMPtr<IWebUIDelegatePrivate> delegatePrivate(Query, delegate);
		if (delegatePrivate) {
			if (SUCCEEDED(delegatePrivate->webViewSetCursor(m_webView, reinterpret_cast<OLE_HANDLE>(platformCursor))))
				return;
		}
	}

	m_webView->setLastCursor(platformCursor);
	::SetCursor(platformCursor);
	return;
	*/
}

void WebChromeClient::setCursorHiddenUntilMouseMoves(bool)
{
	notImplemented();
}


void WebChromeClient::setLastSetCursorToCurrentCursor()
{
	//m_webView->setLastCursor(::GetCursor());
}

void WebChromeClient::formStateDidChange(const WebCore::Node* node) 
{ 
}

IntPoint WebChromeClient::screenToRootView(const WebCore::IntPoint& p) const 
{
    return p;
}

void WebChromeClient::scrollbarsModeDidChange() const
{
    //FIXME: implement me!
    notImplemented();
}

void WebChromeClient::scheduleCompositingLayerFlush()
{
}

void WebChromeClient::attachRootGraphicsLayer(WebCore::Frame* frame, WebCore::GraphicsLayer* layer)
{
}

void WebChromeClient::setNeedsOneShotDrawingSynchronization()
{
}

#if ENABLE(VIDEO)

bool WebChromeClient::supportsFullscreenForNode(const WebCore::Node* node)
{
    return node->hasTagName(HTMLNames::videoTag);
}

void WebChromeClient::enterFullscreenForNode(WebCore::Node* node)
{
    m_webView->enterFullscreenForNode(node);
}

void WebChromeClient::exitFullscreenForNode(WebCore::Node*)
{
    m_webView->exitFullscreen();
}

#endif

#if ENABLE(FULLSCREEN_API)

bool WebChromeClient::supportsFullScreenForElement(const WebCore::Element* element, bool)
{
#if ENABLE(VIDEO)
	//kprintf("supportsFullScreenForElement %d %d\n", element && element->isMediaElement(), element && element->isMediaElement());
    return element && element->isMediaElement();
#else
    return false;
#endif
}

void WebChromeClient::enterFullScreenForElement(WebCore::Element* element)
{
	//kprintf("enterFullScreenForElement\n");
    element->document().webkitWillEnterFullScreenForElement(element);
    m_webView->enterFullScreenForElement(element);
    element->document().webkitDidEnterFullScreenForElement(element);
    m_fullScreenElement = element;
}

void WebChromeClient::exitFullScreenForElement(WebCore::Element*)
{
	//kprintf("exitFullScreenForElement\n");

    // The element passed into this function is not reliable, i.e. it could
    // be null. In addition the parameter may be disappearing in the future.
    // So we use the reference to the element we saved above.
    ASSERT(m_fullScreenElement);
    m_fullScreenElement->document().webkitWillExitFullScreenForElement(m_fullScreenElement.get());
    m_webView->exitFullScreenForElement(m_fullScreenElement.get());
    m_fullScreenElement->document().webkitDidExitFullScreenForElement(m_fullScreenElement.get());
    m_fullScreenElement = nullptr;
}

void WebChromeClient::fullScreenRendererChanged(RenderBox*)
{
    //m_webView->adjustFullScreenElementDimensionsIfNeeded();
}

#endif

bool WebChromeClient::selectItemWritingDirectionIsNatural()
{
    return true;
}

bool WebChromeClient::selectItemAlignmentFollowsMenuWritingDirection()
{
    return false;
}

bool WebChromeClient::hasOpenedPopup() const
{
	return false;
}

RefPtr<PopupMenu> WebChromeClient::createPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new PopupMenuMorphOS(client));
}

RefPtr<SearchPopupMenu> WebChromeClient::createSearchPopupMenu(PopupMenuClient* client) const
{
    return adoptRef(new SearchPopupMenuMorphOS(client));
}

void WebChromeClient::AXStartFrameLoad()
{
}

void WebChromeClient::AXFinishFrameLoad()
{
}         
