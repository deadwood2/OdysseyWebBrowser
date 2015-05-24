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

#ifndef WebKitTypes_h
#define WebKitTypes_h

#include <WebKitDefines.h>

#if PLATFORM(GTK)

#include <gtk/gtk.h>

typedef GtkWidget BalWidget;
typedef cairo_surface_t BalSurface;
typedef struct _BalEventExpose{} BalEventExpose;
typedef struct _BalResizeEvent{} BalResizeEvent;
typedef struct _BalQuitEvent{} BalQuitEvent;
typedef struct _BalUserEvent{} BalUserEvent;
typedef GdkEventKey BalEventKey;
typedef GdkEventButton BalEventButton;
typedef GdkEventMotion BalEventMotion;
typedef GdkEventScroll BalEventScroll;

typedef GdkPoint BalPoint;
typedef GdkRectangle BalRectangle;

#elif PLATFORM(MUI)

#include <cairo.h>
#include <intuition/classusr.h>

class WebView;
struct _cairo;
struct _cairo_surface;
struct MinNode;

struct viewnode
{
  struct MinNode node;
  ULONG num;
  Object   *app;
  Object   *window; /* window object containing the browser */
  Object   *browser; /* browser object */

  WebView *webView;
  _cairo_surface *surface;
  _cairo *cr;

  bool expose;
};

typedef struct viewnode BalWidget;
typedef _cairo_surface BalSurface;

struct MorphOSResizeEvent
{
  int w, h;
};

typedef int BalEventExpose;
typedef struct MorphOSResizeEvent BalResizeEvent;
typedef int BalQuitEvent;
typedef int BalUserEvent;
typedef struct IntuiMessage BalEventKey;
typedef struct IntuiMessage BalEventButton;
typedef struct IntuiMessage BalEventMotion;
typedef struct IntuiMessage BalEventScroll;

typedef struct _BalPoint{
  int x;
  int y;
} BalPoint;

typedef struct _BalRectangle{
  int x, y;
  int w, h;
} BalRectangle;

#elif OS(AMIGAOS4)

#include <cairo.h>

struct Window;
struct Gadget;
struct Hook;
struct AppIcon;
struct Node;
struct List;
class WebView;

struct AmigaOWBWindow
{
    AmigaOWBWindow* next;
    Window* window;
    WebView* webView;
    int offsetx, offsety;
    int left, top, width, height;
    int webViewWidth, webViewHeight;
    _cairo* cr;
    _cairo_surface* surface;
    void *img_back, *img_forward, *img_stop,
         *img_search, *img_home, *img_reload,
         *img_iconify, *img_bookmark, *img_bookmarkadd;
    Gadget *gad_toolbar, *gad_vbar, *gad_hbar,
           *gad_url, *gad_fuelgauge, *gad_stop,
           *gad_back, *gad_forward, *gad_iconify,
           *gad_search, *gad_status, *gad_webview,
           *gad_statuspage, *gad_hlayout, *gad_bookmark,
           *gad_bookmarkadd, *gad_clicktab, *gad_vlayout;
    Hook* backfill_hook;
    char title[256];
    char url[2000];
    char search[500];
    char statusBarText[256];
    char toolTipText[256];
    char statusToolTipText[512];
    AppIcon* appicon;
    void* curentCursor;
    unsigned int fuelGaugeArgs[2];
    const char* arexxPortName;
    void* bookmark;
    unsigned long* page;
    List* clickTabList;
    Node* clickTabNode;
    bool expose;
};

typedef struct AmigaOWBWindow BalWidget;
typedef _cairo_surface BalSurface;

struct AmigaOWBResizeEvent
{
    int w, h;
};

typedef int BalEventExpose;
typedef struct AmigaOWBResizeEvent BalResizeEvent;
typedef int BalQuitEvent;
typedef int BalUserEvent;
typedef struct IntuiMessage BalEventKey;
typedef struct IntuiMessage BalEventButton;
typedef struct IntuiMessage BalEventMotion;
typedef struct IntuiMessage BalEventScroll;

typedef struct _BalPoint{
    int x;
    int y;
} BalPoint;
typedef struct _BalRectangle{
    int x, y;
    int w, h;
} BalRectangle;

#elif PLATFORM(QT)

#include <QWidget>
#include <QRect>
#include <QPoint>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QEvent>

typedef QWidget BalWidget;
typedef QPixmap BalSurface;
typedef QPaintEvent BalEventExpose;
typedef QResizeEvent BalResizeEvent;
typedef QEvent BalQuitEvent;
typedef struct _BalUserEvent{} BalUserEvent;
typedef QKeyEvent BalEventKey;
typedef QMouseEvent BalEventButton;
typedef QMouseEvent BalEventMotion;
typedef QWheelEvent BalEventScroll;

typedef QPoint BalPoint;
typedef QRect BalRectangle;

#elif PLATFORM(SDL) || PLATFORM(SDLCAIRO)

#include <SDL.h>

typedef SDL_Surface BalWidget;
#if PLATFORM(SDL)
typedef SDL_Surface BalSurface;
#else
#include <cairo.h>
typedef cairo_surface_t BalSurface;
#endif

typedef struct SDL_ExposeEvent BalEventExpose;
typedef struct SDL_ResizeEvent BalResizeEvent;
typedef struct SDL_QuitEvent BalQuitEvent;
typedef struct SDL_UserEvent BalUserEvent;
typedef struct SDL_KeyboardEvent BalEventKey;
typedef struct SDL_MouseButtonEvent BalEventButton;
typedef struct SDL_MouseMotionEvent BalEventMotion;
typedef struct SDL_MouseButtonEvent BalEventScroll;

typedef struct _BalPoint{
    int x;
    int y;
} BalPoint;
typedef SDL_Rect BalRectangle;

#define SDLUserEventCode_Timer         0x0000CAFE
#define SDLUserEventCode_Thread        0x00CAFE00

#elif PLATFORM()
#include ""

#endif

typedef enum WebCacheModel {
    WebCacheModelDocumentViewer = 0,
    WebCacheModelDocumentBrowser = 1,
    WebCacheModelPrimaryWebBrowser = 2
} WebCacheModel;

#endif
