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

#include <ChromeClient.h>
#include <FocusDirection.h>
#include <GraphicsContext.h>
#include <ScrollTypes.h>
#include <wtf/Forward.h>

#include <clib/debug_protos.h>

class WebDesktopNotificationsDelegate;
class WebView;

namespace WebCore {
    class Geolocation;
	class NotificationClient;
    class GraphicsLayer;
    class Frame;
}

class WebChromeClient : public WebCore::ChromeClient {
public:
    WebChromeClient(WebView*);
    ~WebChromeClient();

    virtual void chromeDestroyed();

    virtual void setWindowRect(const WebCore::FloatRect&);
    virtual WebCore::FloatRect windowRect();
    
    virtual WebCore::FloatRect pageRect();
    
    virtual void focus();
    virtual void unfocus();

    virtual bool canTakeFocus(WebCore::FocusDirection);
    virtual void takeFocus(WebCore::FocusDirection);

    virtual void focusedElementChanged(WebCore::Element*);
    virtual void focusedFrameChanged(WebCore::Frame*);

    virtual WebCore::Page* createWindow(WebCore::Frame*, const WebCore::FrameLoadRequest&, const WebCore::WindowFeatures&, const WebCore::NavigationAction&);
    virtual void show();

    virtual bool canRunModal();
    virtual void runModal();

    virtual void setToolbarsVisible(bool);
    virtual bool toolbarsVisible();
    
    virtual void setStatusbarVisible(bool);
    virtual bool statusbarVisible();
    
    virtual void setScrollbarsVisible(bool);
    virtual bool scrollbarsVisible();
    
    virtual void setMenubarVisible(bool);
    virtual bool menubarVisible();

    virtual void setResizable(bool);

    virtual void addMessageToConsole(MessageSource source, MessageLevel level, const WTF::String& message, unsigned lineNumber, unsigned columnNumber, const WTF::String& sourceID);

    virtual bool canRunBeforeUnloadConfirmPanel();
    virtual bool runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame* frame);

    virtual void closeWindowSoon();

    virtual void runJavaScriptAlert(WebCore::Frame*, const WTF::String&);
    virtual bool runJavaScriptConfirm(WebCore::Frame*, const WTF::String&);
    virtual bool runJavaScriptPrompt(WebCore::Frame*, const WTF::String& message, const WTF::String& defaultValue, WTF::String& result);
    virtual void setStatusbarText(const WTF::String&);
    virtual bool shouldInterruptJavaScript();
    virtual WebCore::KeyboardUIMode keyboardUIMode();

    virtual bool tabsToLinks() const;
    virtual WebCore::IntRect windowResizerRect() const;

    virtual void invalidateRootView(const WebCore::IntRect&);
    virtual void invalidateContentsAndRootView(const WebCore::IntRect&);
    virtual void invalidateContentsForSlowScroll(const WebCore::IntRect&);
    virtual void scroll(const WebCore::IntSize& scrollDelta, const WebCore::IntRect& rectToScroll, const WebCore::IntRect& clipRect);
    
    virtual WebCore::IntPoint screenToRootView(const WebCore::IntPoint& p) const ;
    virtual WebCore::IntRect rootViewToScreen(const WebCore::IntRect& r) const;
    virtual PlatformPageClient platformPageClient() const;
    virtual void contentsSizeChanged(WebCore::Frame*, const WebCore::IntSize&) const;

    virtual void scrollbarsModeDidChange() const;
    virtual void mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags);
    virtual bool shouldUnavailablePluginMessageBeButton(WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const;
    virtual void unavailablePluginButtonClicked(WebCore::Element*, WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const;

    virtual void setToolTip(const WTF::String&, WebCore::TextDirection);

    virtual void print(WebCore::Frame*);

    virtual void exceededDatabaseQuota(WebCore::Frame*, const WTF::String&, WebCore::DatabaseDetails);

    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded);
    virtual void reachedApplicationCacheOriginQuota(WebCore::SecurityOrigin*, int64_t totalSpaceNeeded);

	/*
#if ENABLE(CONTEXT_MENUS)
	virtual void showContextMenu() { }
#endif
	*/
#if ENABLE(INPUT_TYPE_COLOR)
    virtual std::unique_ptr<WebCore::ColorChooser> createColorChooser(WebCore::ColorChooserClient*, const WebCore::Color&);
#endif

#if ENABLE(INPUT_TYPE_DATE)
    virtual PassRefPtr<WebCore::DateTimeChooser> openDateTimeChooser(WebCore::DateTimeChooserClient*, const WebCore::DateTimeChooserParameters&);
#endif 

    virtual void runOpenPanel(WebCore::Frame*, PassRefPtr<WebCore::FileChooser>);
    virtual void loadIconForFiles(const Vector<WTF::String>&, WebCore::FileIconLoader*);

    virtual void setCursor(const WebCore::Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool);
    virtual void setLastSetCursorToCurrentCursor();

    virtual void formStateDidChange(const WebCore::Node*);

    // Pass 0 as the GraphicsLayer to detatch the root layer.
    virtual void attachRootGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*);
    // Sets a flag to specify that the next time content is drawn to the window,
    // the changes appear on the screen in synchrony with updates to GraphicsLayers.
    virtual void setNeedsOneShotDrawingSynchronization();
    // Sets a flag to specify that the view needs to be updated, so we need
    // to do an eager layout before the drawing.
    virtual void scheduleCompositingLayerFlush();

    virtual void scrollRectIntoView(const WebCore::IntRect&) const {}
    
#if ENABLE(VIDEO)
    virtual bool supportsFullscreenForNode(const WebCore::Node*);
    virtual void enterFullscreenForNode(WebCore::Node*);
    virtual void exitFullscreenForNode(WebCore::Node*);
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    virtual WebCore::NotificationClient* notificationPresenter() const { return reinterpret_cast<WebCore::NotificationClient*>(m_notificationsDelegate.get()); }
#endif

    virtual bool selectItemWritingDirectionIsNatural();
    virtual bool selectItemAlignmentFollowsMenuWritingDirection();
    virtual bool hasOpenedPopup() const;
    virtual RefPtr<WebCore::PopupMenu> createPopupMenu(WebCore::PopupMenuClient*) const;
    virtual RefPtr<WebCore::SearchPopupMenu> createSearchPopupMenu(WebCore::PopupMenuClient*) const;

#if ENABLE(FULLSCREEN_API)
    virtual bool supportsFullScreenForElement(const WebCore::Element*, bool withKeyboard); 
    virtual void enterFullScreenForElement(WebCore::Element*); 
    virtual void exitFullScreenForElement(WebCore::Element*); 
    virtual void fullScreenRendererChanged(WebCore::RenderBox*);
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
    virtual void scheduleAnimation() {}
#endif

    virtual bool shouldRubberBandInDirection(WebCore::ScrollDirection) const { return true; } 
    virtual void numWheelEventHandlersChanged(unsigned) { }
    virtual void wheelEventHandlersChanged(bool) { }

     WebView* webView() { return m_webView; }

     virtual void AXStartFrameLoad(); 
     virtual void AXFinishFrameLoad(); 
     virtual void attachViewOverlayGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*) { };

private:
    WebView* m_webView;
    RefPtr<WebCore::Element> m_fullScreenElement;
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    std::unique_ptr<WebDesktopNotificationsDelegate> m_notificationsDelegate;
#endif
};
