/*
 * Copyright (C) 2006-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebChromeClient.h"

//#include "WebElementPropertyBag.h"
#include "WebFrame.h"
//#include "WebHistory.h"
//#include "WebMutableURLRequest.h"
//#include "WebDesktopNotificationsDelegate.h"
//#include "WebSecurityOrigin.h"
#include "WebProcess.h"
#include "WebPage.h"
#include <WebCore/ContextMenu.h>
#include <WebCore/Cursor.h>
#include <WebCore/FileChooser.h>
#include <WebCore/FileIconLoader.h>
#include <WebCore/FloatRect.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/FrameView.h>
//#include <WebCore/FullScreenController.h>
//#include <WebCore/FullscreenManager.h>
#include <WebCore/GraphicsLayer.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLVideoElement.h>
#include <WebCore/Icon.h>
//#include <WebCore/LocalWindowsContext.h>
#include <WebCore/LocalizedStrings.h>
#include <WebCore/NavigationAction.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/WindowFeatures.h>
#include <WebCore/ApplicationCacheStorage.h>
#include "PopupMenu.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"

using namespace WebCore;

namespace WebKit {

// When you call GetOpenFileName, if the size of the buffer is too small,
// MSDN says that the first two bytes of the buffer contain the required size for the file selection, in bytes or characters
// So we can assume the required size can't be more than the maximum value for a short.
static const size_t maxFilePathsListSize = USHRT_MAX;

WebChromeClient::WebChromeClient(WebKit::WebPage& webPage)
    : m_webPage(webPage)
{
}

void WebChromeClient::chromeDestroyed()
{
    delete this;
}

void WebChromeClient::setWindowRect(const FloatRect& r)
{
	notImplemented();
}

FloatRect WebChromeClient::windowRect()
{
    return { FloatPoint(0.f, 0.f), FloatSize(m_webPage.size()) };
}

FloatRect WebChromeClient::pageRect()
{
	notImplemented();
	return windowRect();
}

void WebChromeClient::focus()
{
	notImplemented();
    // Normally this would happen on a timer, but JS might need to know this earlier, so we'll update here.
//    m_webPage.updateActiveState();
}

void WebChromeClient::unfocus()
{
	notImplemented();
    // Normally this would happen on a timer, but JS might need to know this earlier, so we'll update here.
//    m_webPage.updateActiveState();
}

bool WebChromeClient::canTakeFocus(FocusDirection direction)
{
	notImplemented();
	return true;
}

void WebChromeClient::takeFocus(FocusDirection direction)
{
	notImplemented();
	if (FocusDirection::Backward == direction)
	{
		if (m_webPage._fActivatePrevious)
			m_webPage._fActivatePrevious();
	}
	else
	{
		if (m_webPage._fActivateNext)
			m_webPage._fActivateNext();
	}
}

void WebChromeClient::focusedElementChanged(Element* element)
{
	m_webPage.setFocusedElement(element);
}

void WebChromeClient::focusedFrameChanged(Frame*)
{
}

Page* WebChromeClient::createWindow(Frame& frame, const WindowFeatures& features, const NavigationAction& navigationAction)
{
	if (!m_webPage._fCanOpenWindow || !m_webPage._fCanOpenWindow(navigationAction.url().string(), features))
		return nullptr;
	
	return m_webPage._fDoOpenWindow();
}

void WebChromeClient::show()
{
	notImplemented();
}

bool WebChromeClient::canRunModal()
{
	notImplemented();
	return false;
}

void WebChromeClient::runModal()
{
	notImplemented();
}

void WebChromeClient::setToolbarsVisible(bool visible)
{
	notImplemented();
}

bool WebChromeClient::toolbarsVisible()
{
	notImplemented();
	return false;
}

void WebChromeClient::setStatusbarVisible(bool visible)
{
	notImplemented();
}

bool WebChromeClient::statusbarVisible()
{
	notImplemented();
	return false;
}

void WebChromeClient::setScrollbarsVisible(bool b)
{
	m_webPage.setAllowsScrolling(b);
}

bool WebChromeClient::scrollbarsVisible()
{
	return m_webPage.allowsScrolling();
}

void WebChromeClient::setMenubarVisible(bool visible)
{
	notImplemented();
}

bool WebChromeClient::menubarVisible()
{
	notImplemented();
	return true;
}

void WebChromeClient::setResizable(bool resizable)
{
	notImplemented();
}

#if 0
static BOOL messageIsError(MessageLevel level)
{
    return level == MessageLevel::Error;
}
#endif

void WebChromeClient::addMessageToConsole(MessageSource source, MessageLevel level, const String& message, unsigned lineNumber, unsigned columnNumber, const String& url)
{
	if (m_webPage._fConsole)
		m_webPage._fConsole(url, message, int(level), lineNumber, columnNumber);
}

bool WebChromeClient::canRunBeforeUnloadConfirmPanel()
{
	notImplemented();
    return false;
}

bool WebChromeClient::runBeforeUnloadConfirmPanel(const String& message, Frame& frame)
{
	notImplemented();
	return true;
}

void WebChromeClient::closeWindowSoon()
{
    // We need to remove the parent WebPage from WebPageSets here, before it actually
    // closes, to make sure that JavaScript code that executes before it closes
    // can't find it. Otherwise, window.open will select a closed WebPage instead of
    // opening a new one <rdar://problem/3572585>.

    // We also need to stop the load to prevent further parsing or JavaScript execution
    // after the window has torn down <rdar://problem/4161660>.
  
    // FIXME: This code assumes that the UI delegate will respond to a webPageClose
    // message by actually closing the WebPage. Safari guarantees this behavior, but other apps might not.
    // This approach is an inherent limitation of not making a close execute immediately
    // after a call to window.close.
	notImplemented();

#if 0
    m_webPage.setGroupName(0);
    m_webPage.stopLoading(0);
    m_webPage.closeWindowSoon();
#endif
}

void WebChromeClient::runJavaScriptAlert(Frame&, const String& message)
{
	if (m_webPage._fAlert)
		m_webPage._fAlert(message);
}

bool WebChromeClient::runJavaScriptConfirm(Frame&, const String& message)
{
	if (m_webPage._fConfirm)
		return m_webPage._fConfirm(message);
	return false;
}

bool WebChromeClient::runJavaScriptPrompt(Frame&, const String& message, const String& defaultValue, String& result)
{
	if (m_webPage._fPrompt)
		return m_webPage._fPrompt(message, defaultValue, result);
	return false;
}

void WebChromeClient::setStatusbarText(const String& statusText)
{
	notImplemented();
}

KeyboardUIMode WebChromeClient::keyboardUIMode()
{
	bool enabled = false;
    return enabled ? KeyboardAccessTabsToLinks : KeyboardAccessDefault;
}

void WebChromeClient::invalidateRootView(const IntRect& windowRect)
{
    ASSERT(core(m_webPage.topLevelFrame()));
    m_webPage.repaint(windowRect);
}

void WebChromeClient::invalidateContentsAndRootView(const IntRect& windowRect)
{
    ASSERT(core(m_webPage.topLevelFrame()));
    m_webPage.repaint(windowRect);
}

void WebChromeClient::invalidateContentsForSlowScroll(const IntRect& windowRect)
{
    ASSERT(core(m_webPage.topLevelFrame()));
    m_webPage.repaint(windowRect);
}

void WebChromeClient::scroll(const IntSize& delta, const IntRect& scrollViewRect, const IntRect& clipRect)
{
//dprintf("scroll by %d.%d rect %d %d %d %d cr %d %d %d %d\n", delta.width(), delta.height(),
//	scrollViewRect.x(), scrollViewRect.y(), scrollViewRect.width(), scrollViewRect.height(),
//	clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
	m_webPage.internalScroll(delta.width(), delta.height());
}

IntPoint WebChromeClient::accessibilityScreenToRootView(const WebCore::IntPoint& point) const
{
    return screenToRootView(point);
}

IntRect WebChromeClient::rootViewToAccessibilityScreen(const WebCore::IntRect& rect) const
{
    return rootViewToScreen(rect);
}

IntRect WebChromeClient::rootViewToScreen(const IntRect& rect) const
{
	return IntRect();
}

IntPoint WebChromeClient::screenToRootView(const IntPoint& point) const
{
	return IntPoint();
}

PlatformPageClient WebChromeClient::platformPageClient() const
{
	notImplemented();
	return 0;
}

void WebChromeClient::contentsSizeChanged(Frame& frame, const IntSize& size) const
{
//    dprintf("%s: to %dx%d\n", __PRETTY_FUNCTION__, size.width(), size.height());
    m_webPage.frameSizeChanged(frame, size.width(), size.height());
}

void WebChromeClient::intrinsicContentsSizeChanged(const IntSize& size) const
{
//    dprintf("%s: to %dx%d\n", __PRETTY_FUNCTION__, size.width(), size.height());
}

void WebChromeClient::mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags, const WTF::String& toolTip, WebCore::TextDirection)
{
	notImplemented();
}

bool WebChromeClient::shouldUnavailablePluginMessageBeButton(RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason) const
{
	return false;
}

void WebChromeClient::unavailablePluginButtonClicked(Element& element, RenderEmbeddedObject::PluginUnavailabilityReason pluginUnavailabilityReason) const
{
	notImplemented();
}

void WebChromeClient::print(WebCore::Frame&, const WebCore::StringWithDirection&)
{
    if (m_webPage._fPrint)
        m_webPage._fPrint();
}

void WebChromeClient::exceededDatabaseQuota(Frame& frame, const String& databaseIdentifier, DatabaseDetails)
{
	notImplemented();
}

void WebChromeClient::reachedMaxAppCacheSize(int64_t spaceNeeded)
{
    // FIXME: Free some space.
    notImplemented();
}

void WebChromeClient::reachedApplicationCacheOriginQuota(SecurityOrigin&, int64_t)
{
    notImplemented();
}

void WebChromeClient::runOpenPanel(Frame&, FileChooser& fileChooser)
{
	if (m_webPage._fFile)
		m_webPage._fFile(fileChooser);
}

void WebChromeClient::loadIconForFiles(const Vector<WTF::String>& filenames, WebCore::FileIconLoader& loader)
{
    loader.iconLoaded(Icon::createIconForFiles(filenames));
}

RefPtr<Icon> WebChromeClient::createIconForFiles(const Vector<String>& filenames)
{
    return Icon::createIconForFiles(filenames);
}

void WebChromeClient::didFinishLoadingImageForElement(WebCore::HTMLImageElement&)
{
}

void WebChromeClient::setCursor(const Cursor& cursor)
{
	m_webPage.setCursor(int(cursor.platformCursor()));
}

void WebChromeClient::setCursorHiddenUntilMouseMoves(bool)
{
    notImplemented();
}

void WebChromeClient::attachRootGraphicsLayer(Frame&, GraphicsLayer* graphicsLayer)
{
notImplemented();
//    m_webPage.setRootChildLayer(graphicsLayer);
}

void WebChromeClient::attachViewOverlayGraphicsLayer(GraphicsLayer*)
{
notImplemented();
    // FIXME: If we want view-relative page overlays in Legacy WebKit on Windows, this would be the place to hook them up.
}

void WebChromeClient::triggerRenderingUpdate()
{
	m_webPage.flushCompositing();
}

bool WebChromeClient::selectItemWritingDirectionIsNatural()
{
    return false;
}

bool WebChromeClient::selectItemAlignmentFollowsMenuWritingDirection()
{
    return true;
}

RefPtr<PopupMenu> WebChromeClient::createPopupMenu(PopupMenuClient& client) const
{
	return adoptRef(new PopupMenuMorphOS(&client, &m_webPage));
}

RefPtr<SearchPopupMenu> WebChromeClient::createSearchPopupMenu(PopupMenuClient& client) const
{
    return adoptRef(new SearchPopupMenuMorphOS(&client, &m_webPage));
}

#if ENABLE(FULLSCREEN_API)

bool WebChromeClient::supportsFullScreenForElement(const Element& element, bool requestingKeyboardAccess)
{
	if (requestingKeyboardAccess)
		return false;
	return true;
}

void WebChromeClient::enterFullScreenForElement(Element& element)
{
    m_webPage.setFullscreenElement(&element);
}

void WebChromeClient::exitFullScreenForElement(Element* element)
{
    m_webPage.setFullscreenElement(nullptr);
}

#endif

bool WebChromeClient::supportsVideoFullscreen(WebCore::HTMLMediaElementEnums::VideoFullscreenMode)
{
	return true;
}

bool WebChromeClient::shouldUseTiledBackingForFrameView(const FrameView& frameView) const
{
    return false;
}

#if ENABLE(VIDEO)

void WebChromeClient::enterVideoFullscreenForVideoElement(HTMLVideoElement& videoElement, HTMLMediaElementEnums::VideoFullscreenMode, bool)
{
//	dprintf("%s\n", __PRETTY_FUNCTION__);
//    m_webView->enterVideoFullscreenForVideoElement(videoElement);
}

void WebChromeClient::exitVideoFullscreenForVideoElement(HTMLVideoElement& videoElement, WTF::CompletionHandler<void(bool)>&& completionHandler)
{
 //   m_webView->exitVideoFullscreenForVideoElement(videoElement);
	completionHandler(true);
 }

void WebChromeClient::setUpPlaybackControlsManager(WebCore::HTMLMediaElement&)
{
// dprintf("%s:\n", __PRETTY_FUNCTION__);
}

void WebChromeClient::clearPlaybackControlsManager()
{
// dprintf("%s:\n", __PRETTY_FUNCTION__);
}
 #endif

}

