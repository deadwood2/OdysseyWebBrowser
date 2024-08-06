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

#ifndef WebWindow_H
#define WebWindow_H
  
#include "WebKitTypes.h"

class WebView;
namespace WebCore {
    class GraphicsContext;
}

class WEBKIT_OWB_API WebWindow
{
public:
    WebWindow(bool modal, WebView* webView, BalRectangle rect);
    virtual ~WebWindow();

    virtual bool onExpose(BalEventExpose event, BalRectangle&);
    virtual bool onKeyDown(BalEventKey event);
    virtual bool onKeyUp(BalEventKey event);
    virtual bool onMouseMotion(BalEventMotion event);
    virtual bool onMouseButtonDown(BalEventButton event);
    virtual bool onMouseButtonUp(BalEventButton event);
    virtual bool onScroll(BalEventScroll event);
    virtual bool onResize(BalResizeEvent event);
    virtual bool onQuit(BalQuitEvent);
    virtual bool onUserEvent(BalUserEvent);
    virtual bool onIdle();
    void show();
    void hide();

    BalRectangle mainWindowRect();
protected:
    void setMainWindow();
    void createPlatformWindow();
    virtual void paint(BalRectangle) = 0;
    void paintDecoration(BalRectangle);
    WebCore::GraphicsContext* createContext();
    void releaseContext(WebCore::GraphicsContext* ctx);
    void updateRect(BalRectangle src, BalRectangle dest);
    void runMainLoop();
    void stopMainLoop();

    BalSurface* m_surface;
    BalWidget* m_mainSurface;
    BalRectangle m_rect;
private:
    bool m_modal;
    bool m_visible;
    WebView *m_webView;
};

#endif

