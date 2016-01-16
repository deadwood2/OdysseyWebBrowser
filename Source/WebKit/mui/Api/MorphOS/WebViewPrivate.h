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
#ifndef WebViewPrivate_H
#define WebViewPrivate_H

#include "WebView.h"
#include "IntRect.h"
#include <wtf/Vector.h>
#include "FrameView.h"
#include "MainFrame.h"
#include "BALBase.h"
#include "cairo.h"
#include "WebNotificationDelegate.h"
#include "WebResourceLoadDelegate.h"
#include "JSActionDelegate.h"
#include "WebFrameLoadDelegate.h"
#include <wtf/text/WTFString.h>

class MorphOSWebNotificationDelegate : public WebNotificationDelegate
{
public:
    static TransferSharedPtr<MorphOSWebNotificationDelegate> createInstance()
    {
        return new MorphOSWebNotificationDelegate();
    }

    MorphOSWebNotificationDelegate();
    ~MorphOSWebNotificationDelegate();

    virtual void startLoadNotification(WebFrame* webFrame);
    virtual void progressNotification(WebFrame* webFrame);
    virtual void finishedLoadNotification(WebFrame* webFrame);
};

class MorphOSJSActionDelegate : public JSActionDelegate
{
public:
    static TransferSharedPtr<MorphOSJSActionDelegate> createInstance()
    {
	return new MorphOSJSActionDelegate();
    }

    MorphOSJSActionDelegate();
    ~MorphOSJSActionDelegate();
	
    virtual void windowObjectClearNotification(WebFrame*, void*, void*);
    virtual void consoleMessage(WebFrame*, int, int, const char*);
    virtual bool jsAlert(WebFrame*, const char*);
    virtual bool jsConfirm(WebFrame*, const char *);
    virtual bool jsPrompt(WebFrame*, const char*, const char*, char**);
    virtual void exceededDatabaseQuota(WebFrame *frame, WebSecurityOrigin *origin, const char* databaseIdentifier);
};

class MorphOSWebFrameDelegate : public WebFrameLoadDelegate
{
public:
    static TransferSharedPtr<MorphOSWebFrameDelegate> createInstance()
    {
	return new MorphOSWebFrameDelegate();
    }

    MorphOSWebFrameDelegate();
    ~MorphOSWebFrameDelegate();
    
    virtual void windowObjectClearNotification(WebFrame*, void*, void*);
    virtual void titleChange(WebFrame* webFrame, const char *title);
    virtual void didStartProvisionalLoad(WebFrame* frame);
    virtual void didReceiveServerRedirectForProvisionalLoadForFrame(WebFrame*);
    virtual void didCancelClientRedirectForFrame(WebFrame*);
    virtual void willPerformClientRedirectToURL(WebFrame*, const char*, double, double);
    virtual void didCommitLoad(WebFrame*);
    virtual void didFinishLoad(WebFrame*);
    virtual void didFailProvisionalLoad(WebFrame*, WebError*);
    virtual void didFailLoad(WebFrame*, WebError*);
    virtual void didFinishDocumentLoadForFrame(WebFrame*);
    virtual void didHandleOnloadEventsForFrame(WebFrame*);
    virtual void didChangeLocationWithinPageForFrame(WebFrame*);
    virtual void willCloseFrame(WebFrame*);
    virtual void didFirstLayoutInFrame(WebFrame*);
    virtual void didFirstVisuallyNonEmptyLayoutInFrame(WebFrame*);
    virtual void dispatchNotEnoughMemory(WebFrame*);
    virtual void didDisplayInsecureContent(WebFrame*);
    virtual void didRunInsecureContent(WebFrame*, WebSecurityOrigin*);
    virtual void didChangeIcons(WebFrame*);
	
private:
    void updateURL(void *browser, char *url);
    void updateTitle(void *browser, char *title, bool isUTF8 = true);
    void updateNavigation(void *browser);
};

class MorphOSResourceLoadDelegate : public WebResourceLoadDelegate
{
public:
    static TransferSharedPtr<MorphOSResourceLoadDelegate> createInstance()
    {
	return new MorphOSResourceLoadDelegate();
    }
    
    MorphOSResourceLoadDelegate();
    ~MorphOSResourceLoadDelegate();
    
    virtual void identifierForInitialRequest(WebView *webView, WebMutableURLRequest *request, WebDataSource *dataSource, unsigned long identifier);
    virtual WebMutableURLRequest* willSendRequest(WebView *webView, unsigned long identifier, WebMutableURLRequest *request, WebURLResponse *redirectResponse, WebDataSource *dataSource);
    virtual void didReceiveAuthenticationChallenge(WebView *webView, unsigned long identifier, WebURLAuthenticationChallenge *challenge, WebDataSource *dataSource);
    virtual void didCancelAuthenticationChallenge(WebView *webView, unsigned long identifier, WebURLAuthenticationChallenge *challenge, WebDataSource *dataSource);
    virtual void didReceiveResponse(WebView *webView, unsigned long identifier, WebURLResponse *response, WebDataSource *dataSource);
    virtual void didReceiveContentLength(WebView *webView, unsigned long identifier, unsigned length, WebDataSource *dataSource);
    virtual void didFinishLoadingFromDataSource(WebView *webView, unsigned long identifier, WebDataSource *dataSource);
    virtual void didFailLoadingWithError(WebView *webView, unsigned long identifier, WebError *error, WebDataSource *dataSource);
    virtual void plugInFailedWithError(WebView *webView, WebError *error, WebDataSource *dataSource);
};

class WebViewPrivate {
public:
    WebViewPrivate(WebView *webView);
    ~WebViewPrivate() 
    {
    }

    void show()
    {
    }
    
    void hide()
    {
    }

    void setFrameRect(WebCore::IntRect r)
    {
        m_rect = r;
    }

    WebCore::IntRect frameRect()
    { 
        return m_rect; 
    }
    
    BalWidget *createWindow(BalRectangle r)
    {
        WebCore::IntRect rect(r.x, r.y, r.w, r.h);
        if(rect != m_rect)
            m_rect = rect;
    

        return 0;
    }

    void initWithFrameView(WebCore::FrameView *frameView)
    {
    }

    bool shouldCoalesce(WebCore::IntRect rect)
    {
	const size_t cRectThreshold = 10;
	const float cWastedSpaceThreshold = 0.75f;
	size_t count = m_dirtyRegions.size();
	bool useUnionedRect = (count <= 1) || (count > cRectThreshold);
	if (!useUnionedRect)
	{
	    // Attempt to guess whether or not we should use the unioned rect or the individual rects.
	    // We do this by computing the percentage of "wasted space" in the union.  If that wasted space
	    // is too large, then we will do individual rect painting instead.
	    float unionPixels = (rect.width() * rect.height());
	    float singlePixels = 0;
	    for (size_t i = 0; i < count; ++i)
		singlePixels += m_dirtyRegions[i].width() * m_dirtyRegions[i].height();
	    float wastedSpace = 1 - (singlePixels / unionPixels);
	    if (wastedSpace <= cWastedSpaceThreshold)
		useUnionedRect = true;
	}
	return useUnionedRect;
    }
    
    void clearDirtyRegion()
    {
	m_dirtyRegions.clear();
        m_backingStoreDirtyRegion.setX(0);
        m_backingStoreDirtyRegion.setY(0);
        m_backingStoreDirtyRegion.setWidth(0);
        m_backingStoreDirtyRegion.setHeight(0);
    }

    BalRectangle dirtyRegion()
    {
        BalRectangle rect = {m_backingStoreDirtyRegion.x(), m_backingStoreDirtyRegion.y(), m_backingStoreDirtyRegion.width(), m_backingStoreDirtyRegion.height()};
        return rect;
    }

    void addToDirtyRegion(const BalRectangle& dirtyRect)
    {
        m_backingStoreDirtyRegion.unite(dirtyRect);
	if(m_dirtyRegions.size() > 10)
	{
	    m_dirtyRegions.clear();
	    m_dirtyRegions.append(m_backingStoreDirtyRegion);
	}
	else
	{
	    m_dirtyRegions.append(dirtyRect);
	}
    }

    BalRectangle onExpose(BalEventExpose event);
    bool onKeyDown(BalEventKey event);
    bool onKeyUp(BalEventKey event);
    bool onMouseMotion(BalEventMotion event);
    bool onMouseButtonDown(BalEventButton event);
    bool onMouseButtonUp(BalEventButton event);
    bool onScroll(BalEventScroll event);
    void onResize(BalResizeEvent event);
    void onQuit(BalQuitEvent);
    void onUserEvent(BalUserEvent);
    void popupMenuHide();
    void popupMenuShow(void *popupInfo);

    void sendExposeEvent(WebCore::IntRect);
    
    void repaint(const WebCore::IntRect&, bool contentChanged, bool immediate = false, bool repaintContentOnly = false);
    
    void scrollBackingStore(WebCore::FrameView*, int dx, int dy, const WebCore::IntRect& scrollViewRect, const WebCore::IntRect& clipRect);
    
    void fireWebKitTimerEvents();
    void fireWebKitThreadEvents();
    
    void resize(BalRectangle);
    void move(BalPoint, BalPoint);
    
    void repaintAfterNavigationIfNeeded();
    
    void closeWindowSoon();
    
    void setViewWindow(BalWidget*) {}
    
    bool screenshot(int &requested_width, int& requested_height, WTF::Vector<char> *imageData);
    bool screenshot(WTF::String& path);
    
    void requestMemoryRelease();

 private:
    void updateView(BalWidget *widget, WebCore::IntRect rect, bool sync);
    void closeWindowTimerFired();
    void closeWindow();
    
    WebCore::IntRect m_rect;
    WebView *m_webView;
    bool isInitialized;
    
    WTF::Vector<WebCore::IntRect> m_dirtyRegions;
    WebCore::IntPoint m_backingStoreSize;
    WebCore::IntRect m_backingStoreDirtyRegion;

    WebCore::Timer m_closeWindowTimer;

};


#endif
