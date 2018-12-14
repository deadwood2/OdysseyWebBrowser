/*
 * Copyright 2009-2010 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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

#define USE_MORPHOS_SURFACE 0

/* WebKit */
#include "config.h"
#include "GraphicsContext.h"
#include "BackForwardController.h"
#include "ContextMenu.h"
#include "ContextMenuController.h"
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>
#include "SharedBuffer.h"
#include "DocumentLoader.h"
#include "EventHandler.h"
#include "DocumentLoader.h"
#include "DOMImplementation.h"
#include "DragController.h"
#include "DragData.h"
#include "Editor.h"
#include "FileIOLinux.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "FrameView.h"
#include "HostWindow.h"
#include <wtf/MainThread.h>
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PlatformKeyboardEvent.h"
#include "KeyboardEvent.h"
#include "WindowsKeyboardCodes.h"
#include "PopupMenuMorphOS.h"
#include "PrintContext.h"
#include "ProgressTracker.h"
#include "RenderView.h"
#include "SharedTimer.h"
#include "Settings.h"
#include "SubstituteData.h"
#include "Timer.h"
#include "WebBackForwardList.h"
#include "WebDataSource.h"
#include "WebPreferences.h"
#include "WebIconDatabase.h"
#include "AutofillManager.h"
#include "TopSitesManager.h"

#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#include "MediaPlayer.h"
#endif

/* OWB */
#include <Api/WebFrame.h>
#include <Api/WebView.h>

/* Posix */
#include <unistd.h>
#include <math.h>
#include <cstdio>

/* Cairo */
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#if USE_MORPHOS_SURFACE
#include <proto/cairo.h>
#endif

/* Video */
#if ENABLE(VIDEO)
extern "C"
{
	#include "acinerella.h"
	extern struct Library *CGXVideoBase;
}
#endif

/* System */
#define SYSTEM_PRIVATE

#include <cybergraphx/cybergraphics.h>
#if ENABLE_VIDEO
#include <cybergraphx/cgxvideo.h>
#endif
#include <intuition/pointerclass.h>
#include <graphics/rpattr.h>
#include <devices/rawkeycodes.h>
#include <devices/inputevent.h>
#include <proto/cybergraphics.h>
#if ENABLE(VIDEO) && !OS(AROS)
#include <proto/cgxvideo.h>
#endif
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/keymap.h>
#include <clib/macros.h>
#include <Calltips_mcc.h>

#if OS(AROS)
#undef SetRPAttrs
#endif

#define min(a,b) ((a)<(b) ? (a) : (b))

/* Local */
#include "OWBPrintContext.h"
#include "gui.h"
#include "utils.h"

#define D(x)

extern CONST_STRPTR * get_user_agent_strings();

static const bool lowVideoMemoryProfile = getenv("OWB_LOW_VIDEO_MEMORY_PROFILE") != NULL;

/******************************************************************
 * owbbrowserclass
 *****************************************************************/

using namespace WebCore;

/* Plugin helpers */

#if OS(MORPHOS)
#include <emul/emulinterface.h>
#include <emul/emulregs.h>

static __inline IPTR _CFCALL(void (*func)(void))
{
	const UWORD *funcptr = (const UWORD *) func;
	IPTR res;
	if (funcptr[0] >= TRAP_AREA_START) REG_A7 -= sizeof(IPTR);
	res = MyEmulHandle->EmulCallDirect68k((void *) func);
	if (funcptr[0] >= TRAP_AREA_START) REG_A7 += sizeof(IPTR);
	return res;
}

static __inline IPTR _CALLFUNC1(void (*func)(void), IPTR arg1)
{
	IPTR res, *sptr;
	REG_A7 -= 1 * sizeof(IPTR);
	sptr = (IPTR *) REG_A7;
	sptr[0] = arg1;
	res = _CFCALL(func);
	REG_A7 += 1 * sizeof(IPTR);
	return res;
}
#define CALLFUNC1(f,a1) _CALLFUNC1((void (*)(void))(f),(IPTR)(a1))

static __inline IPTR _CALLFUNC2(void (*func)(void), IPTR arg1, IPTR arg2)
{
	IPTR res, *sptr;
	REG_A7 -= 2 * sizeof(IPTR);
	sptr = (IPTR *) REG_A7;
	sptr[0] = arg1;
	sptr[1] = arg2;
	res = _CFCALL(func);
	REG_A7 += 2 * sizeof(IPTR);
	return res;
}
#define CALLFUNC2(f,a1,a2) _CALLFUNC2((void (*)(void))(f),(IPTR)(a1),(IPTR)(a2))
#endif

/**/

Object *create_browser(char * url, ULONG is_frame, Object *title, APTR sourceview, Object *window, ULONG privatebrowsing)
{
	return (Object *) NewObject(getowbbrowserclass(), NULL,
						MA_OWBBrowser_URL, (ULONG) url,
						MA_OWBBrowser_IsFrame, (ULONG) is_frame,
						MA_OWBBrowser_TitleObj, (ULONG) title,
						MA_OWBBrowser_SourceView, (ULONG) sourceview,
						MA_OWBBrowser_Window, (ULONG) window,
						MA_OWBBrowser_PrivateBrowsing, (ULONG) privatebrowsing,
						TAG_DONE);
}

/**/

static void autoscroll_add(Object *obj, struct Data *data, IntuiMessage *im);
static void autoscroll_remove(Object *obj, struct Data *data);

enum
{
	DRAW_UPDATE,
	DRAW_SCROLL,
	DRAW_PLUGIN,
};

struct EventHandlerNode
{
	struct MinNode n;
	APTR instance;
	APTR handlerfunc;
};

static APTR dataObject; // Move that

struct Data
{
	/* area coords */
	int width;
	int height;
	int left;
	int top;

	/* draw mode */
	int draw_mode;

#if !USE_MORPHOS_SURFACE
	/* offscreen bitmap */
	struct RastPort rp_offscreen;
#endif
	ULONG dirty;

	/* scroll info */
	int dx;
	int dy;
	IntRect scrollrect;
	IntRect pendingscrollrect;

	/* update rect */
	int update_x;
	int update_y;
	int update_width;
	int update_height;

	/* mouse */
	ULONG pointertype;
	ULONG mouse_inside;
	IntPoint last_position;

	/* middle mouse scrolling */
	ULONG is_scrolling;
	ULONG scrolling_mode;
	IntPoint autoscroll_position;
	IntSize autoscroll_delta;
	ULONG autoscroll_added;
	double lastMMBClickTime;

	/* page */
	char *url;
	char *editedurl;
	char title[512]; // XXX: remove these size limits
	char status[512];
	char tooltip[512];
	char *dragurl;
	ULONG is_frame;
	APTR source_view;
	APTR parent_browser;

	ULONG forbid_events;
	ULONG is_active;
	ULONG back_available;
	ULONG forward_available;
	ULONG stop_available;
	ULONG reload_available;
	ULONG loading;
	ULONG loadprogress;
	ULONG state;
	ULONG zone;
	ULONG security;
	struct browsersettings settings;

#if ENABLE(VIDEO)
	/* media  */
	VLayerHandle     *video_handle;
	HTMLMediaElement *video_element; // XXX: we can have several media instances per browser but this one is the vlayer video element (there can be only one at once).
	ULONG video_fullscreen;
	ULONG video_mode;
	ULONG video_x_offset;
	ULONG video_y_offset;
	ULONG video_colorkey;
	double video_lastclick;
#endif

#if OS(MORPHOS)
	/* popup Menu */
	Object *popmenu;
#endif
#if OS(AROS)
	struct Hook itemActivatedHook;
	struct Hook cancelledHook;
	struct Hook hideHook;
#endif

	/* Autofill */
	Object *autofillCalltip;
	AutofillManager *autofillManager;

	/* ColorChooser */
	Object *colorchooserCalltip;

	/* DateTimeChooser */
	Object *datetimechooserCalltip;

	/* context Menu */
	Object *contextmenu;
	ContextMenuController *menucontroller;

	/* title object linked to this browser */
	Object *titleobj;

	/* ref. to view, allocated from this object */
	BalWidget *view;

	/* Drag data */
	/*
	APTR dragdata;
	*/
	APTR dragimage;
	LONG dragoperation;
	IntPoint last_drag_position;

	/* scrollers elements */
	Object *hbar;
	Object *vbar;
	Object *hbargroup;
	Object *vbargroup;

	/* event handler */
	struct MUI_EventHandlerNode ehnode;
	ULONG added;

	/* input handler */
	struct MUI_InputHandlerNode ihnode;

	/* plugin */
	struct MinList eventhandlerlist;
	unsigned char *plugin_src;
	unsigned int plugin_stride;
	int plugin_update_x;
	int plugin_update_y;
	int plugin_update_width;
	int plugin_update_height;
};

DEFNEW
{
	BalWidget *view = (BalWidget *) malloc(sizeof(BalWidget));
	WebView *webView;

	if(!view)
	{
		return 0;
	}

	webView = WebView::createInstance();

	if(!webView)
	{
		free(view);
		return 0;
	}

	memset(view, 0, sizeof(BalWidget));

	obj = (Object *) DoSuperNew(cl, obj,
		InnerSpacing(0,0),
		MUIA_Font,        MUIV_Font_Inherit,
		MUIA_ContextMenu, TRUE,
		MUIA_CycleChain,  TRUE,
		MUIA_FillArea,    FALSE,
		MUIA_Dropable,    TRUE,
		MUIA_ShortHelp,	  FALSE,
		TAG_MORE, INITTAGS
	);

	if(obj)
	{
		char *url = (char *) GetTagData(MA_OWBBrowser_URL, NULL, msg->ops_AttrList);
		GETDATA;

		data->added = FALSE;

		data->loading = FALSE;
		data->loadprogress = 0;

		data->is_active = 0;

		data->back_available    = FALSE;
		data->forward_available = FALSE;
		data->reload_available  = TRUE;
		data->stop_available    = FALSE;

		data->source_view = (APTR)     GetTagData(MA_OWBBrowser_SourceView, NULL, msg->ops_AttrList);
		data->is_frame    = (ULONG)    GetTagData(MA_OWBBrowser_IsFrame,    NULL, msg->ops_AttrList);
		data->titleobj   = (Object *) GetTagData(MA_OWBBrowser_TitleObj,   NULL, msg->ops_AttrList);
		set(data->titleobj, MUIA_UserData, obj);
		DoMethod(obj, MUIM_Notify, MA_OWBBrowser_Title, MUIV_EveryTime, data->titleobj, 3, MUIM_Set, MUIA_Text_Contents, MUIV_TriggerValue);

		data->view = view;
		data->view->webView = webView;
		data->view->browser = obj;

#if !OS(AROS) // This causes crash later on
		data->view->app     = _app(obj);
#endif

		data->view->window  = (Object *) GetTagData(MA_OWBBrowser_Window, NULL, msg->ops_AttrList);

		/* Browser default settings (could be set at init from opener) */
		data->settings.javascript      = JAVASCRIPT_DEFAULT;
		data->settings.plugins         = PLUGINS_DEFAULT;
		data->settings.images          = IMAGES_DEFAULT;
		data->settings.animations      = ANIMATIONS_DEFAULT;
		data->settings.privatebrowsing = 0; // PRIVATE_BROWSING_DEFAULT;
		data->settings.blocking		   = BLOCK_DEFAULT;
		data->settings.useragent       = USERAGENT_DEFAULT;
		//data->settings.cookiepolicy    = COOKIES_ACCEPT;
		//data->settings.cookiefilter    = strdup("*");

#if !USE_MORPHOS_SURFACE
		InitRastPort(&data->rp_offscreen);
		data->dirty = TRUE;
#endif

		if(data->url)
		{
			free(data->url);
		}

		if(url)
		{
			data->url = strdup(url);
		}
		else
		{
			data->url = strdup(""); // not really needed, but whatever
		}

		data->editedurl = strdup(data->url);

		data->dragurl = strdup("");

		data->width  = 160;
		data->height = 120;

#if 0//USE_MORPHOS_SURFACE // We can use an image surface, this one isn't used anyway
		struct Window *window = (struct Window *) getv(data->view->window, MUIA_Window);
		struct RastPort *rp   = window->RPort;
		data->view->surface = cairo_morphos_surface_create_from_bitmap(CAIRO_CONTENT_COLOR_ALPHA, data->width, data->height, rp->BitMap);
#else
		data->view->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, data->width, data->height);
#endif
		if(cairo_surface_status(data->view->surface) == CAIRO_STATUS_SUCCESS)
		{
			data->view->cr = cairo_create(data->view->surface);
		}

		MorphOSResizeEvent re = {data->width, data->height};

		BalRectangle clientRect = { 0, 0, data->width, data->height };

		data->view->webView->initWithFrame(clientRect, data->is_frame ? data->url : "", "");
		data->view->webView->setViewWindow(data->view);
		data->view->webView->onResize(re);

		/* Passed attributes */
		set(obj, MA_OWBBrowser_PrivateBrowsing, (ULONG) GetTagData(MA_OWBBrowser_PrivateBrowsing, FALSE, msg->ops_AttrList));

		//kprintf("OWBBrowser: loading url <%s> is_frame: %d sourceview %p\n", data->url, data->is_frame, data->source_view);

		if(data->source_view)
		{
	        Frame* frame = core(data->view->webView->mainFrame());

			if (frame)
			{
				BalWidget *widget = ((WebView *) data->source_view)->viewWindow();

				if(widget)
				{
					Object *browser = widget->browser;
					String mimeType = frame->loader().documentLoader()->responseMIMEType();
					URL baseURL(ParsedURLString, (char *) getv(browser, MA_OWBBrowser_URL));
		            URL failingURL(ParsedURLString, "");
		            ResourceRequest request(baseURL);
					
					Frame* sourceFrame = core(((WebView *) data->source_view)->mainFrame());
					RefPtr<SharedBuffer> dataSource = sourceFrame->loader().documentLoader() ? sourceFrame->loader().documentLoader()->mainResourceData() : 0;

		            if (!mimeType)
					{
		                mimeType = "text/html";
					}

					if(dataSource)
					{
						ResourceResponse response(URL(), mimeType, dataSource->size(), frame->loader().documentLoader()->overrideEncoding());
						SubstituteData substituteData(dataSource, failingURL, response, SubstituteData::SessionHistoryVisibility::Hidden);

						FrameLoadRequest frameLoadRequest(frame, request, ShouldOpenExternalURLsPolicy::ShouldNotAllow, substituteData);
						frame->loader().load(frameLoadRequest);

						set(obj, MA_OWBBrowser_URL, (char *) getv(browser, MA_OWBBrowser_URL));
						set(obj, MA_OWBBrowser_Title, (char *) getv(browser, MA_OWBBrowser_Title));
					}
				}
	        }
		}
		else
		{
			if(!data->is_frame)
			{
#if 0			
				if(data->view->window)
					DoMethod(app, MUIM_Application_PushMethod, data->view->window, 3, MM_OWBWindow_LoadURL, data->url, obj);
#else
				// XXX: we should reuse OWBWindow_LoadURL method (but we can't at that point, since it's not attached to window yet)
				if(!strcmp("about:", data->url))
				{
					OWBFile f("PROGDIR:resource/about.html");

					if(f.open('r') != -1)
					{
						char *fileContent = f.read(f.getSize());

						if(fileContent)
						{
							String content = fileContent;

							data->view->webView->mainFrame()->loadHTMLString(content.utf8().data(), data->url);

							delete [] fileContent;
						}

						f.close();
					}
				}
				else if(!strcmp("topsites://", data->url))
				{
					TopSitesManager::getInstance().generateTemplate(data->view->webView, data->url);
				}
				else if(data->url[0])
				{
					data->view->webView->mainFrame()->loadURL(data->url);
				}
#endif
			}
		}

		data->autofillManager = new AutofillManager(obj);

		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Flags = MUIIHNF_TIMER;
		data->ihnode.ihn_Millis = 20;
		data->ihnode.ihn_Method = MM_OWBBrowser_AutoScroll_Perform;

		NEWLIST(&data->eventhandlerlist);

		ADDTAIL(&view_list, data->view);

		DoMethod((Object *) getv(app, MA_OWBApp_ScriptManagerWindow), MM_ScriptManagerGroup_InjectScripts, webView);
	}
	else
	{
		if(webView) delete webView;
		if(view) free(view);
	}

	return (ULONG) obj;
}

DEFDISPOSE
{
	GETDATA;
	APTR n, m;

	// XXX: Sigh, this is really sucky :)
	if ((APTR) getv((Object *) getv(app, MA_OWBApp_PrinterWindow), MA_OWB_Browser) == (APTR) obj)
	{
		DoMethod((Object *) getv(app, MA_OWBApp_PrinterWindow), MM_PrinterWindow_Close);
	}

#if OS(MORPHOS)
	if (data->popmenu)
	{
		MUI_DisposeObject(data->popmenu);
		data->popmenu = NULL;
	}
#endif
#if OS(AROS)
#endif

	if (data->contextmenu)
	{
		MUI_DisposeObject(data->contextmenu);
		data->contextmenu = NULL;
	}

	if (data->autofillCalltip)
	{
		DoMethod(obj, MM_OWBBrowser_Autofill_HidePopup);
	}

	if(data->autofillManager)
	{
		delete data->autofillManager;
		data->autofillManager = NULL;
	}

	if (data->colorchooserCalltip)
	{
		DoMethod(obj, MM_OWBBrowser_ColorChooser_HidePopup);
	}

	if (data->datetimechooserCalltip)
	{
		DoMethod(obj, MM_OWBBrowser_DateTimeChooser_HidePopup);
	}

	/* webview */
	data->view->browser = NULL;
	delete data->view->webView;
	data->view->webView = NULL;

	/* cairo objects */
	if (data->view->cr)
	{
		cairo_destroy(data->view->cr);
	}

	if (data->view->surface)
	{
		cairo_surface_destroy(data->view->surface);
	}

	/* view node */
	REMOVE(data->view);
	free(data->view);

	/* There shouldn't be any, anyway */
	ITERATELISTSAFE(n, m, &data->eventhandlerlist)
	{
		REMOVE(n);
		free(n);
	}

	free(data->url);
	free(data->editedurl);
	free(data->dragurl);

	return DOSUPER;
}

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_OWBBrowser_URL:
		{
			if(data->url)
			{
				free(data->url);
			}
			data->url = strdup((char *) tag->ti_Data);

			//set(obj, MA_OWBBrowser_EditedURL, data->url);
		}
		break;

		case MA_OWBBrowser_EditedURL:
		{
			if(data->editedurl)
			{
				free(data->editedurl);
			}
			data->editedurl = strdup((char *) tag->ti_Data);
		}
		break;

		case MA_OWBBrowser_Title:
		{
			stccpy(data->title, (char *) tag->ti_Data, sizeof(data->title));
		}
		break;

		case MA_OWBBrowser_TitleObj:
		{
			data->titleobj = (Object *) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_Loading:
		{
			if(data->loading != tag->ti_Data)
			{
				data->loading = tag->ti_Data;
				set(data->titleobj, MA_TransferAnim_Animate, data->loading);

				if((getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Tab) && data->loading == FALSE)
				{
					set(obj, MA_FavIcon_PageURL, data->url);
				}
			}
		}
		break;

		case MA_OWBBrowser_State:
		{
			data->state = (ULONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_Zone:
		{
			data->zone = (ULONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_Security:
		{
			data->security = (ULONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_LoadingProgress:
		{
			data->loadprogress = (ULONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_StatusText:
		{
			stccpy(data->status, (char *) tag->ti_Data, sizeof(data->status));
		}
		break;

		case MA_OWBBrowser_ToolTipText:
		{
			stccpy(data->tooltip, (char *) tag->ti_Data, sizeof(data->tooltip));
		}
		break;

		case MA_OWBBrowser_Pointer:
		{
			if(is_morphos2() && getv(app, MA_OWBApp_EnablePointers))
			{
				if (data->pointertype != tag->ti_Data)
				{
					data->pointertype = tag->ti_Data;

					if (muiRenderInfo(obj))
						set(_window(obj), WA_PointerType, tag->ti_Data);
				}
			}
		}
		break;

		case MA_OWBBrowser_Active:
		{
			data->is_active = (int) tag->ti_Data;
			if(_win(obj))
			{
				if(data->is_active)
				{
				   Object *active = (Object *) getv(_win(obj), MUIA_Window_ActiveObject);

				   if(active != obj && active != MUIV_Window_ActiveObject_None)
				   {
					   set(_win(obj), MUIA_Window_ActiveObject, obj);
					   set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
				   }
			   }
			   else
			   {
					Object *active = (Object *) getv(_win(obj), MUIA_Window_ActiveObject);
					if(active == obj)
					{
					   set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
					}
			   }
			}
			data->view->webView->updateFocusedAndActiveState();
		}
		break;

		case MA_OWBBrowser_ForbidEvents:
		{
			data->forbid_events = (ULONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_ParentBrowser:
		{
			data->parent_browser = (APTR) tag->ti_Data;
		}
		break;

		case MA_FavIcon_PageURL:
		{
			set(data->titleobj, MA_FavIcon_PageURL, tag->ti_Data);
		}
		break;

		case MA_OWBBrowser_PrivateBrowsing:
		{
			bool enable = tag->ti_Data != FALSE;
			ULONG clientcount = getv(app, MA_OWBApp_PrivateBrowsingClients);

			data->settings.privatebrowsing = (ULONG) tag->ti_Data;

			// Set private browsing flag for current page
			//data->view->webView->page()->settings().setPrivateBrowsingEnabled(enable);

			// Update client counter
			set(app, MA_OWBApp_PrivateBrowsingClients, enable ? ++clientcount : --clientcount);

			// Only restore icondatabase normal behaviour if private browsing is not used anymore (privatebrowsing unfortunately global to icondatabase)
			WebIconDatabase::sharedWebIconDatabase()->setPrivateBrowsingEnabled(getv(app, MA_OWBApp_PrivateBrowsingClients) > 0);
		}
		break;

		case MA_OWBBrowser_ContentBlocking:
		{
			data->settings.blocking = (ULONG) tag->ti_Data;

			switch((ULONG) tag->ti_Data)
			{
				case BLOCK_ENABLED:
				{
					
				}
				break;

				case BLOCK_DISABLED:
				{

				}
				break;

				case BLOCK_DEFAULT:
				{

				}
				break;
			}
		}
		break;

		case MA_OWBBrowser_PluginsEnabled:
		{
			data->settings.plugins = (ULONG) tag->ti_Data;
			// Queried by WebFrameLoaderClient::allowPlugins()
		}
		break;

		case MA_OWBBrowser_LoadImagesAutomatically:
		{
			data->settings.images = (ULONG) tag->ti_Data;

			switch((ULONG) tag->ti_Data)
			{
				case IMAGES_ENABLED:
				{
					data->view->webView->page()->settings().setLoadsImagesAutomatically(true);
				}
				break;

				case IMAGES_DISABLED:
				{
					data->view->webView->page()->settings().setLoadsImagesAutomatically(false);
				}
				break;

				case IMAGES_DEFAULT:
				{
					data->view->webView->page()->settings().setLoadsImagesAutomatically(WebPreferences::sharedStandardPreferences()->loadsImagesAutomatically());
				}
				break;
			}
		}
		break;

		case MA_OWBBrowser_PlayAnimations:
		{
			data->settings.animations = (ULONG) tag->ti_Data;

			// Implement in webkit first...:)

			switch((ULONG) tag->ti_Data)
			{
				case ANIMATIONS_ENABLED:
				{
					//data->view->webView->page()->settings()->setAllowsAnimatedImages(true);
				}
				break;

				case ANIMATIONS_DISABLED:
				{
					//data->view->webView->page()->settings()->setAllowsAnimatedImages(false);
				}
				break;

				case ANIMATIONS_DEFAULT:
				{
					//data->view->webView->page()->settings()->setAllowsAnimatedImages(WebPreferences::sharedStandardPreferences()->allowsAnimatedImages());
				}
				break;
			}
			break;
		}
		break;

		case MA_OWBBrowser_JavaScriptEnabled:
		{
			data->settings.javascript = (ULONG) tag->ti_Data;

			switch((ULONG) tag->ti_Data)
			{
				case JAVASCRIPT_ENABLED:
				{
					data->view->webView->page()->settings().setScriptEnabled(true);
				}
				break;

				case JAVASCRIPT_DISABLED:
				{
					data->view->webView->page()->settings().setScriptEnabled(false);
				}
				break;

				case JAVASCRIPT_DEFAULT:
				{
					data->view->webView->page()->settings().setScriptEnabled(WebPreferences::sharedStandardPreferences()->isJavaScriptEnabled());
				}
				break;
			}
		}
		break;

		case MA_OWBBrowser_UserAgent:
		{
			data->settings.useragent = (ULONG) tag->ti_Data;
			char *useragent = (char *) (data->settings.useragent != USERAGENT_DEFAULT ? get_user_agent_strings()[data->settings.useragent] : "");
			data->view->webView->setCustomUserAgent(useragent);
		}
		break;

		/* DragnDrop */
		case MA_OWBBrowser_DragURL:
		case MA_OWB_URL:
		{
		        free(data->dragurl);
			data->dragurl = strdup((char *) tag->ti_Data);
		}
		break;

		case MA_OWBBrowser_DragData:
		{
			dataObject = (APTR) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_DragOperation:
		{
			data->dragoperation = (LONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_DragImage:
		{
			data->dragimage = (APTR) tag->ti_Data;
		}
		break;
		
		/* Navigation */
		case MA_OWBBrowser_BackAvailable:
		{
			data->back_available = (ULONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_ForwardAvailable:
		{
			data->forward_available = (ULONG) tag->ti_Data; 
		}
		break;

		case MA_OWBBrowser_ReloadAvailable:
		{
			data->reload_available = (ULONG) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_StopAvailable:
		{
			data->stop_available = (ULONG) tag->ti_Data;
		}
		break;

		/* Scrollers */
		case MA_OWBBrowser_VBar:
		{
			data->vbar = (Object *) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_VBarGroup:
		{
			data->vbargroup = (Object *) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_HBar:
		{
			data->hbar = (Object *) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_HBarGroup:
		{
			data->hbargroup = (Object *) tag->ti_Data;
		}
		break;

		case MA_OWBBrowser_VTopPixel:
		{
		}
		break;

		case MA_OWBBrowser_VTotalPixel:
		{
		}
		break;

		case MA_OWBBrowser_VVisiblePixel:
		{
		}
		break;

		case MA_OWBBrowser_HTopPixel:
		{
		}
		break;

		case MA_OWBBrowser_HTotalPixel:
		{
		}
		break;

		case MA_OWBBrowser_HVisiblePixel:
		{
		}
		break;
	}
	NEXTTAG
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_OWB_ObjectType:
		{
			*msg->opg_Storage = MV_OWB_ObjectType_Browser;
		}
		return TRUE;

		case MA_OWBBrowser_BackAvailable:
		{
			*msg->opg_Storage = (ULONG) data->back_available;
		}
		return TRUE;

		case MA_OWBBrowser_ForwardAvailable:
		{
			*msg->opg_Storage = (ULONG) data->forward_available;
		}
		return TRUE;

		case MA_OWBBrowser_ReloadAvailable:
		{
			*msg->opg_Storage = (ULONG) data->reload_available;
		}
		return TRUE;

		case MA_OWBBrowser_StopAvailable:
		{
			*msg->opg_Storage = (ULONG) data->stop_available;
		}
		return TRUE;

		case MA_OWBBrowser_Widget:
		{
			*msg->opg_Storage = (ULONG) data->view;
		}
		return TRUE;

		case MA_OWBBrowser_URL:
		{
			*msg->opg_Storage = (ULONG) data->url;
		}
		return TRUE;

		case MA_OWBBrowser_EditedURL:
		{
			*msg->opg_Storage = (ULONG) data->editedurl;
		}
		return TRUE;

		case MA_OWBBrowser_IsFrame:
		{
			*msg->opg_Storage = (ULONG) data->is_frame;
		}
		return TRUE;

		case MA_OWBBrowser_SourceView:
		{
			*msg->opg_Storage = (ULONG) data->source_view;
		}
		return TRUE;

		case MA_OWBBrowser_Title:
		{
			*msg->opg_Storage = (ULONG) data->title;
		}
		return TRUE;

		case MA_OWBBrowser_Loading:
		{
			*msg->opg_Storage = (ULONG) data->loading;
		}
		return TRUE;

		case MA_OWBBrowser_State:
		{
			*msg->opg_Storage = (ULONG) data->state;
		}
		return TRUE;

		case MA_OWBBrowser_Zone:
		{
			*msg->opg_Storage = (ULONG) data->zone;
		}
		return TRUE;

		case MA_OWBBrowser_Security:
		{
			*msg->opg_Storage = (ULONG) data->security;
		}
		return TRUE;

		case MA_OWBBrowser_LoadingProgress:
		{
			*msg->opg_Storage = (ULONG) data->loadprogress;
		}
		return TRUE;

		case MA_OWBBrowser_StatusText:
		{
			*msg->opg_Storage = (ULONG) data->status;
		}
		return TRUE;

		case MA_OWBBrowser_ToolTipText:
		{
			*msg->opg_Storage = (ULONG) data->tooltip;
		}
		return TRUE;

		case MA_OWBBrowser_TitleObj:
		{
			*msg->opg_Storage = (ULONG) data->titleobj;
		}
		return TRUE;

		case MA_OWBBrowser_Active:
		{
			*msg->opg_Storage = (ULONG) data->is_active;
		}
		return TRUE;

		case MA_OWBBrowser_ForbidEvents:
		{
			*msg->opg_Storage = (ULONG) data->forbid_events;
		}
		return TRUE;

		case MA_OWBBrowser_ParentBrowser:
		{
			*msg->opg_Storage = (ULONG) data->parent_browser;
		}
		break;

		case MA_OWBBrowser_PluginsEnabled:
		{
			*msg->opg_Storage = (ULONG) data->settings.plugins;
		}
		return TRUE;

		case MA_OWBBrowser_PrivateBrowsing:
		{
			*msg->opg_Storage = (ULONG) data->settings.privatebrowsing;
		}
		return TRUE;

		case MA_OWBBrowser_ContentBlocking:
		{
			*msg->opg_Storage = (ULONG) data->settings.blocking;
		}
		return TRUE;

		case MA_OWBBrowser_LoadImagesAutomatically:
		{
			*msg->opg_Storage = (ULONG) data->settings.images;
		}
		return TRUE;

		case MA_OWBBrowser_PlayAnimations:
		{
			*msg->opg_Storage = (ULONG) data->settings.animations;
		}
		return TRUE;

		case MA_OWBBrowser_JavaScriptEnabled:
		{
			*msg->opg_Storage = (ULONG) data->settings.javascript;
		}
		return TRUE;

		case MA_OWBBrowser_UserAgent:
		{
			*msg->opg_Storage = (ULONG) data->settings.useragent;
		}
		return TRUE;

		case MA_OWBBrowser_VideoElement:
		{
#if ENABLE(VIDEO)
			*msg->opg_Storage = (ULONG) data->video_element;
#else
            *msg->opg_Storage = (ULONG) 0;
#endif
		}
		return TRUE;

		/* Drag Stuff */
		case MA_OWBBrowser_DragURL:
		case MA_OWB_URL:
		{
			*msg->opg_Storage = (ULONG) data->dragurl;
		}
		return TRUE;

		case MA_OWBBrowser_DragData:
		{
			*msg->opg_Storage = (ULONG) dataObject;
		}
		return TRUE;

		case MA_OWBBrowser_DragOperation:
		{
			*msg->opg_Storage = (ULONG) data->dragoperation;
		}
		return TRUE;

		case MA_OWBBrowser_DragImage:
		{
			*msg->opg_Storage = (ULONG) data->dragimage;
		}
		return TRUE;

		/* Scrollers */
		case MA_OWBBrowser_VBar:
		{
			*msg->opg_Storage = (ULONG) data->vbar;
		}
		return TRUE;

		case MA_OWBBrowser_VBarGroup:
		{
			*msg->opg_Storage = (ULONG) data->vbargroup;
		}
		return TRUE;

		case MA_OWBBrowser_HBar:
		{
			*msg->opg_Storage = (ULONG) data->hbar;
		}
		return TRUE;

		case MA_OWBBrowser_HBarGroup:
		{
			*msg->opg_Storage = (ULONG) data->hbargroup;
		}
		return TRUE;
		
		case MA_OWBBrowser_VTopPixel:
		{
			*msg->opg_Storage = (ULONG) core(data->view->webView->mainFrame())->view()->visibleContentRect().y();
		}
		return TRUE;

		case MA_OWBBrowser_VTotalPixel:
		{
			*msg->opg_Storage = (ULONG) core(data->view->webView->mainFrame())->view()->contentsHeight();
		}
		return TRUE;

		case MA_OWBBrowser_VVisiblePixel:
		{
			*msg->opg_Storage = (ULONG) core(data->view->webView->mainFrame())->view()->visibleHeight();
		}
		return TRUE;

		case MA_OWBBrowser_HTopPixel:
		{
			*msg->opg_Storage = (ULONG) core(data->view->webView->mainFrame())->view()->visibleContentRect().x();
		}
		return TRUE;

		case MA_OWBBrowser_HTotalPixel:
		{
			*msg->opg_Storage = (ULONG) core(data->view->webView->mainFrame())->view()->contentsWidth();
		}
		return TRUE;

		case MA_OWBBrowser_HVisiblePixel:
		{
			*msg->opg_Storage = (ULONG) core(data->view->webView->mainFrame())->view()->visibleWidth(); 
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MinWidth  += 160;
	msg->MinMaxInfo->MinHeight += 120;
	msg->MinMaxInfo->DefWidth  += 800;
	msg->MinMaxInfo->DefHeight += 600;
	msg->MinMaxInfo->MaxWidth  = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

	return(0);
}

DEFMMETHOD(Show)
{
	ULONG rc;
	GETDATA;

	struct BitMap *obm = _rp(obj)->BitMap;
	_cairo_surface *newsurface;
	_cairo *newcr = NULL;

	rc = DOSUPER;

	D(kprintf("[OWBBrowser] Show: %p size: %dx%d at (%d,%d) for widget %p\n", obj, _mwidth(obj), _mheight(obj), _mleft(obj), _mtop(obj), data->view));

	LONG oldwidth = data->width;
	LONG oldheight = data->height;

	data->width  = _mwidth(obj);
	data->height = _mheight(obj);
	data->left   = _mleft(obj);
	data->top    = _mtop(obj);

	data->view->app     = _app(obj);
	data->view->window  = _win(obj);

#if !USE_MORPHOS_SURFACE
	if (data->rp_offscreen.BitMap)
	{
		FreeBitMap(data->rp_offscreen.BitMap);
		data->rp_offscreen.BitMap = NULL;
	}

	if (!data->rp_offscreen.BitMap)
	{
		ULONG flags = BMF_MINPLANES | BMF_CLEAR;
		if(!lowVideoMemoryProfile)
		{
			flags |= BMF_DISPLAYABLE;
		}

		data->rp_offscreen.BitMap = AllocBitMap(data->width, data->height, 32, flags, obm);
	}
#endif

	// Window size changed
	if(oldwidth != data->width || oldheight != data->height)
	{
		D(kprintf("[OWBBrowser] Resizing\n"));

#if USE_MORPHOS_SURFACE
		struct Window *window = (struct Window *) getv(data->view->window, MUIA_Window);
		struct RastPort *rp   = window->RPort;
		newsurface = cairo_morphos_surface_create_from_bitmap(CAIRO_CONTENT_COLOR_ALPHA, data->width, data->height, rp->BitMap);
#else
		newsurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, data->width, data->height);
#endif
		if (newsurface && cairo_surface_status(newsurface) == CAIRO_STATUS_SUCCESS)
		{
	        newcr = cairo_create(newsurface);

			if (newcr == NULL)
			{
	            cairo_surface_destroy(newsurface);
			}
	    }

		if (newcr)
		{
			if(data->view->cr)
			{
				cairo_destroy(data->view->cr);
			}

			if(data->view->surface)
			{
				cairo_surface_destroy(data->view->surface);
			}

			data->view->cr = newcr;
			data->view->surface = newsurface;

			MorphOSResizeEvent re = {data->width, data->height};
			data->view->webView->onResize(re);
	    }

#if ENABLE(VIDEO) && !OS(AROS)
		// Video: recompute vlayer offset whenever window size changes
		if (data->video_element)
		{
			struct Window *window = (struct Window *) getv(_win(obj), MUIA_Window);

			if(window && data->video_handle)
			{
				FloatSize size = data->video_element->player()->naturalSize();
				//kprintf("naturalsize %dx%d\n", size.width(), size.height());

				if ( ( (float) size.width() / (float) size.height()) < ( (float) _mwidth(obj) / (float) _mheight(obj)) )
				{
					// Width is too big
					data->video_y_offset = 0;
					data->video_x_offset = _mwidth(obj) - (ULONG) (_mheight(obj) * ( (float) size.width() / (float) size.height()));
					data->video_x_offset /= 2;
				}
				else
				{
					// Height too big
					data->video_y_offset = _mheight(obj) - (ULONG) (_mwidth(obj) * ( (float) size.height() / (float) size.width()));
					data->video_y_offset /= 2;
					data->video_x_offset = 0;
				}

				SetVLayerAttrTags(data->video_handle,
								  VOA_LeftIndent,   _mleft(obj) - window->BorderLeft + data->video_x_offset,
								  VOA_RightIndent,  window->Width - window->BorderRight - 1 - _mright(obj) + data->video_x_offset,
								  VOA_TopIndent,    _mtop(obj) - window->BorderTop + data->video_y_offset,
								  VOA_BottomIndent, window->Height - window->BorderBottom -1 - _mbottom(obj) + data->video_y_offset,
								  TAG_DONE);
			}
		}
#endif
	}

	// Ask redraw
	data->view->webView->addToDirtyRegion(IntRect(0, 0, data->width, data->height));
	data->dirty = TRUE;
	DoMethod(obj, MM_OWBBrowser_Expose, FALSE);

	if(data->autofillCalltip)
	{
		DoMethod(data->autofillCalltip, MUIM_Calltips_ParentShow);
	}

	if(data->colorchooserCalltip)
	{
		DoMethod(data->colorchooserCalltip, MUIM_Calltips_ParentShow);
	}

	if(data->datetimechooserCalltip)
	{
		DoMethod(data->datetimechooserCalltip, MUIM_Calltips_ParentShow);
	}

	return rc;
}

DEFMMETHOD(Hide)
{
	GETDATA;

#if !USE_MORPHOS_SURFACE
	if (data->rp_offscreen.BitMap)
	{
		FreeBitMap(data->rp_offscreen.BitMap);
		data->rp_offscreen.BitMap = NULL;
	}
#endif

	if(data->autofillCalltip)
	{
		DoMethod(data->autofillCalltip, MUIM_Calltips_ParentHide);
	}

		if(data->colorchooserCalltip)
		{
			DoMethod(data->colorchooserCalltip, MUIM_Calltips_ParentHide);
		}

		if(data->datetimechooserCalltip)
		{
			DoMethod(data->datetimechooserCalltip, MUIM_Calltips_ParentHide);
		}

	return DOSUPER;
}

DEFSMETHOD(OWBBrowser_Expose)
{
	GETDATA;

	BalEventExpose ev = 0;
	data->view->webView->onExpose(ev);

	if(msg->updatecontrols)
	{
		DoMethod(obj, MM_OWBBrowser_UpdateScrollers);
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_Update)
{
	GETDATA;

	//double start = currentTime();

	unsigned int stride;
	unsigned char *src;
	IntRect *rect = (IntRect *)msg->rect;

	data->update_x = rect->x();
	data->update_y = rect->y();
	data->update_width = rect->width();
	data->update_height = rect->height();

	if(data->update_x < 0)
	{
		data->update_width += data->update_x;
		data->update_x = 0;
	}

	if(data->update_y < 0)
	{
		data->update_height += data->update_y;
		data->update_y = 0;
	}

	if (data->update_width > _mwidth(obj) - data->update_x)
		data->update_width = _mwidth(obj) - data->update_x;

	if (data->update_height > _mheight(obj) - data->update_y)
		data->update_height = _mheight(obj) - data->update_y;

	// Synchronize changes
	if(msg->sync)
	{
		data->draw_mode = DRAW_UPDATE;
		MUI_Redraw(obj, MADF_DRAWUPDATE);
	}
	// Render in offscreen bitmap
	else
	{
		if(data->view->surface == NULL || cairo_surface_status(data->view->surface) != CAIRO_STATUS_SUCCESS || data->view->cr == NULL)
			return 0;

		if (data->update_width <= 0 || data->update_height <= 0)
			return 0;

		stride = cairo_image_surface_get_stride(data->view->surface);
		src	   = cairo_image_surface_get_data(data->view->surface);

		if(src && data->rp_offscreen.BitMap)
		{
			WritePixelArray(src, data->update_x, data->update_y, stride, &data->rp_offscreen, data->update_x, data->update_y, data->update_width, data->update_height, NATIVE_ARGB);
		}
	}

	//kprintf("[update] %f ms\n", (currentTime() - start)*1000);

	return 0;
}

DEFSMETHOD(OWBBrowser_Scroll)
{
	GETDATA;

	//double start = currentTime();

	data->scrollrect = *((IntRect *)msg->rect);
	data->dx = msg->dx;
	data->dy = msg->dy;

	if(data->rp_offscreen.BitMap)
	{
		ScrollRaster(&data->rp_offscreen, data->dx, data->dy,
						  data->scrollrect.x(),
						  data->scrollrect.y(),
						  data->scrollrect.width() + data->scrollrect.x() - 1,
						  data->scrollrect.height() + data->scrollrect.y() - 1);
	}

    data->pendingscrollrect = data->scrollrect;

	//kprintf("[scroll] %f ms\n", (currentTime() - start)*1000);

	return 0;
}

DEFMMETHOD(Draw)
{
	GETDATA;

	DOSUPER;

	if(!muiRenderInfo(obj)) return 0;

#if ENABLE(VIDEO)
	if(data->video_fullscreen) return 0; // Custom backfill
#endif

	if (msg->flags & MADF_DRAWUPDATE)
	{
		D(kprintf("[OWBBrowser] Drawupdate\n"));

		if(data->draw_mode == DRAW_UPDATE)
		{
#if USE_MORPHOS_SURFACE
			// FIXME: not up-to-date with current render method anymore	(pending scroll, sync)
			BltBitMapRastPort(cairo_morphos_surface_get_bitmap(data->view->surface), data->update_x, data->update_y, rp, data->update_x + _mleft(obj), data->update_y + _mtop(obj), data->update_width, data->update_height, 0xCC);
#else
			if(!data->pendingscrollrect.isEmpty())
			{
				// Optimize that, thank you
                BltBitMapRastPort(data->rp_offscreen.BitMap, 0, 0, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 0xC0);
                data->pendingscrollrect = IntRect(0, 0, 0, 0);
			}
			else
			{
				BltBitMapRastPort(data->rp_offscreen.BitMap, data->update_x, data->update_y, _rp(obj), _mleft(obj) + data->update_x, _mtop(obj) + data->update_y, data->update_width, data->update_height, 0xC0);
			}
#endif
		}
		else if(data->draw_mode == DRAW_PLUGIN)
		{
			unsigned int stride;
			unsigned char *src;

			if(!data->plugin_src || !data->plugin_stride)
				return 0;

			if (data->update_width <= 0 || data->update_height <= 0)
				return 0;

			stride = data->plugin_stride;
			src	= data->plugin_src;

			D(kprintf("draw plugin: %dx%d at (%d, %d) src 0x%p stride %lu\n", data->update_width, data->update_height, data->update_x, data->update_y, src, stride));

#if USE_MORPHOS_SURFACE
			// XXX: implement
#else
			if(src && data->rp_offscreen.BitMap)
			{
				WritePixelArrayAlpha(src, data->plugin_update_x, data->plugin_update_y, stride, &data->rp_offscreen, data->update_x, data->update_y, data->update_width, data->update_height, 0xffffffff);
				//WritePixelArray(src, data->plugin_update_x, data->plugin_update_y, stride, &data->rp_offscreen, data->update_x, data->update_y, data->update_width, data->update_height, NATIVE_ARGB);
				BltBitMapRastPort(data->rp_offscreen.BitMap, data->update_x, data->update_y, _rp(obj), _mleft(obj) + data->update_x, _mtop(obj) + data->update_y, data->update_width, data->update_height, 0xC0);
			}
#endif
		}
		else if(data->draw_mode == DRAW_SCROLL)
		{
			// nothing anymore, since it's sync'ed in draw_update
		}
	}
	else if(msg->flags & MADF_DRAWOBJECT)
	{
		D(kprintf("[OWBBrowser] Drawobject\n"));

#if USE_MORPHOS_SURFACE
		BltBitMapRastPort(cairo_morphos_surface_get_bitmap(data->view->surface), 0, 0, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 0xCC);
#else
		if(data->rp_offscreen.BitMap)
		{
			if(data->dirty)
			{
				unsigned int stride;
				unsigned char *src;

				if(data->view->surface == NULL || cairo_surface_status(data->view->surface) != CAIRO_STATUS_SUCCESS || data->view->cr == NULL)
					return 0;

				if (data->update_width <= 0 || data->update_height <= 0)
					return 0;

				stride = cairo_image_surface_get_stride(data->view->surface);
				src	   = cairo_image_surface_get_data(data->view->surface);

				if(src)
				{
					WritePixelArray(src, 0, 0, stride, &data->rp_offscreen, 0, 0, data->width, data->height, NATIVE_ARGB);
				}

				data->dirty = FALSE;
			}

			BltBitMapRastPort(data->rp_offscreen.BitMap, 0, 0, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 0xC0);
		}
#endif
	}

	return 0;
}

DEFMMETHOD(Setup)
{
	GETDATA;

	if (!DOSUPER)
	{
		return (0);
	}

	if(data->autofillCalltip)
	{
		DoMethod(data->autofillCalltip, MUIM_Calltips_ParentSetup);
	}

	if(data->colorchooserCalltip)
	{
		DoMethod(data->colorchooserCalltip, MUIM_Calltips_ParentSetup);
	}

	if(data->datetimechooserCalltip)
	{
		DoMethod(data->datetimechooserCalltip, MUIM_Calltips_ParentSetup);
	}

	if(!data->added)
	{
		data->ehnode.ehn_Object = obj;
		data->ehnode.ehn_Class = cl;
		data->ehnode.ehn_Events =  IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_MOUSEMOVE | IDCMP_CHANGEWINDOW;
		data->ehnode.ehn_Priority = 1;
		data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);

		data->added = TRUE;
	}

	return TRUE;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

#if ENABLE(VIDEO)
	if (data->video_element)
	{
		data->video_element->exitFullscreen();
		//DoMethod(obj, MM_OWBBrowser_VideoEnterFullPage, NULL, FALSE);
	}
#endif

	autoscroll_remove(obj, data);

	if(data->added)
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
		data->added = FALSE;
	}

#if !USE_MORPHOS_SURFACE
	if (data->rp_offscreen.BitMap)
	{
		FreeBitMap(data->rp_offscreen.BitMap);
		data->rp_offscreen.BitMap = NULL;
	}
#endif

	if(data->autofillCalltip)
	{
		DoMethod(data->autofillCalltip, MUIM_Calltips_ParentCleanup);
	}

	if(data->colorchooserCalltip)
	{
		DoMethod(data->colorchooserCalltip, MUIM_Calltips_ParentCleanup);
	}

	if(data->datetimechooserCalltip)
	{
		DoMethod(data->datetimechooserCalltip, MUIM_Calltips_ParentCleanup);
	}

	return DOSUPER;
}

DEFTMETHOD(OWBBrowser_AutoScroll_Perform)
{
	GETDATA;

	Frame* frame = &(data->view->webView->page()->focusController().focusedOrMainFrame());
	if(frame)
	{
		FrameView *view = frame->view();
		if(view)
		{
			view->scrollBy(data->autoscroll_delta);
		}
	}

	return 0;
}

static void autoscroll_add(Object *obj, struct Data *data, IntuiMessage *im)
{
	if(!data->is_scrolling)
	{
		data->is_scrolling   = 1;
		data->scrolling_mode = 2;
		data->autoscroll_position = IntPoint(im->MouseX, im->MouseY);
		data->autoscroll_delta = IntSize(0, 0);
		if(!data->autoscroll_added)
		{
			DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->ihnode);
			data->autoscroll_added = TRUE;
		}
#if 0 // AROS: this is causing a crash
	    set(_window(obj), WA_PointerType, POINTERTYPE_MOVE);
#endif
	}
}

static void autoscroll_remove(Object *obj, struct Data *data)
{
	if(data->is_scrolling)
	{
		data->is_scrolling   = 0;
		data->scrolling_mode = 0;
		if (data->autoscroll_added)
		{
			DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->ihnode);
			data->autoscroll_added = FALSE;
		}
#if 0 // AROS: this is causing a crash
		set(_window(obj), WA_PointerType, data->pointertype);
#endif
	}
}

DEFMMETHOD(HandleEvent)
{
	ULONG rc = 0;

	GETDATA;

	if (msg->imsg)
	{
		ULONG mouse_inside = 0;
		ULONG Class;
		UWORD Code;
		UWORD Qualifier;
		int MouseX, MouseY;
		IntSize Delta;

		IntuiMessage im = *msg->imsg;
		im.MouseX -= _mleft(obj);
		im.MouseY -= _mtop(obj);

		Class     = msg->imsg->Class;
		Code      = msg->imsg->Code;
		Qualifier = msg->imsg->Qualifier;
		MouseX    = msg->imsg->MouseX;
		MouseY    = msg->imsg->MouseY;
		Delta     = IntSize(im.MouseX - data->last_position.x(), im.MouseY - data->last_position.y());

		data->last_position = IntPoint(im.MouseX, im.MouseY);

		/* XXX: move to a dedicated method */
		/* Plugin forward */
		if(Class == IDCMP_MOUSEBUTTONS || Class == IDCMP_MOUSEMOVE || Class == IDCMP_RAWKEY)
		{
			// Check if it's a plugin area before sending, maybe...
			APTR n, m;
			ULONG eaten = FALSE;

			ITERATELISTSAFE(n, m, &data->eventhandlerlist)
			{
				struct EventHandlerNode *ehn = (struct EventHandlerNode *) n;

				if(ehn->instance && ehn->handlerfunc)
				{
#if OS(MORPHOS)
					eaten = CALLFUNC2(ehn->handlerfunc,
							  ehn->instance,
							  (APTR) msg->imsg);
#endif
#if OS(AROS)
					eaten = 1;
#endif
				}

				if(eaten)
				{
					return MUI_EventHandlerRC_Eat;
				}
			}
		}

#if ENABLE(VIDEO)
		/* XXX: move to a dedicated method */
		/* FullScreen Video mode events */
		if(data->video_fullscreen && data->video_element && _isinobject(obj, MouseX, MouseY))
		{
            mouse_inside = 0;

			switch(Class)
			{
				case IDCMP_CHANGEWINDOW:
				{
					if(data->autofillCalltip)
						DoMethod(data->autofillCalltip, MUIM_Calltips_ParentWindowArranged);

					if(data->colorchooserCalltip)
						DoMethod(data->colorchooserCalltip, MUIM_Calltips_ParentWindowArranged);
						
					if(data->datetimechooserCalltip)
						DoMethod(data->datetimechooserCalltip, MUIM_Calltips_ParentWindowArranged);
						
					break;
				}

	            case IDCMP_MOUSEBUTTONS:
				{
					switch(Code & ~IECODE_UP_PREFIX)
					{
						case IECODE_LBUTTON:
						{
							if(!(Code & IECODE_UP_PREFIX))
							{
								double now = currentTime();
								if(DoubleClick((ULONG) data->video_lastclick, (ULONG) ((data->video_lastclick - floor(data->video_lastclick)) * 1000000), (ULONG) now, (ULONG) ((now - floor(now)) * 1000000)))
								{
		                            data->video_element->exitFullscreen();
								}

								data->video_lastclick = currentTime();
							}
							break;
						}

						case IECODE_MBUTTON:
						{
							if(!(Code & IECODE_UP_PREFIX))
							{
								Object *mediacontrolsgroup = (Object *) getv(_parent(_parent(obj)), MA_OWBGroup_MediaControlsGroup);
								if(mediacontrolsgroup)
								{
									ULONG shown = getv(mediacontrolsgroup, MUIA_ShowMe);

									if(shown)
									{
										set(mediacontrolsgroup, MUIA_ShowMe, FALSE);
										set(mediacontrolsgroup, MA_MediaControlsGroup_Browser, NULL);
									}
									else
									{
										set(mediacontrolsgroup, MA_MediaControlsGroup_Browser, obj);
										set(mediacontrolsgroup, MUIA_ShowMe, TRUE);
									}
								}							 
							}
							break;
						}
					}
					break;
				}

				case IDCMP_RAWKEY:
				{
					float delta = 0.f;

					if(!(Code & IECODE_UP_PREFIX))
					{
						switch(Code)
						{
                            case RAWKEY_NM_WHEEL_UP:
							case RAWKEY_RIGHT:
								delta = 10.f;
								break;
							
							case RAWKEY_NM_WHEEL_DOWN:
							case RAWKEY_LEFT:
								delta = -10.f;
								break;
							
							case RAWKEY_UP:
								delta = 60.f;
								break;
							
							case RAWKEY_DOWN:
								delta = -60.f;
								break;

							case RAWKEY_SPACE:
								if(data->video_element->paused())
								{
									data->video_element->play();
								}
								else
								{
									data->video_element->pause();
								}
								break;

							case RAWKEY_ESCAPE:
								data->video_element->exitFullscreen();
								break;

							case RAWKEY_F11:
								DoMethod(_win(obj), MM_OWBWindow_FullScreen, MV_OWBWindow_FullScreen_Toggle);
								break;
						}

						if(delta != 0.f)
						{
							data->video_element->setCurrentTime(data->video_element->currentTime() + delta);
						}
					}
					break;
				}

				default:
					;
			}

			if(mouse_inside != data->mouse_inside)
			{
				data->mouse_inside = mouse_inside;

				if(is_morphos2() && getv(app, MA_OWBApp_EnablePointers))
				{
					set(_window(obj), WA_PointerType, mouse_inside ? data->pointertype : POINTERTYPE_NORMAL);
				}
			}
		
			return MUI_EventHandlerRC_Eat;
		}
#endif

		switch(Class)
		{
			case IDCMP_CHANGEWINDOW:
			{
				if(data->autofillCalltip)
					DoMethod(data->autofillCalltip, MUIM_Calltips_ParentWindowArranged);

				if(data->colorchooserCalltip)
					DoMethod(data->colorchooserCalltip, MUIM_Calltips_ParentWindowArranged);
					
				if(data->datetimechooserCalltip)
					DoMethod(data->datetimechooserCalltip, MUIM_Calltips_ParentWindowArranged);					
					
				break;
			}

            case IDCMP_MOUSEBUTTONS:
			{
				mouse_inside = 0;

				if (_isinobject(obj, MouseX, MouseY))
                {
					mouse_inside = 1;

                    switch (Code & ~IECODE_UP_PREFIX)
                    {
						case IECODE_LBUTTON:
							if (!(Code & IECODE_UP_PREFIX))
							{
								/* Fake mouse move event when taking focus is necessary to fill some EventHandler fields */
								set(obj, MA_OWBBrowser_Active, TRUE);
								//data->view->webView->onMouseMotion(im);
							}

							if (Code & IECODE_UP_PREFIX)
							{
								data->view->webView->onMouseButtonUp(im);
							}
	                 	    else
							{
								data->view->webView->onMouseButtonDown(im);
							}
							break;

						case IECODE_RBUTTON:
							if (Code & IECODE_UP_PREFIX)
							{
								data->view->webView->onMouseButtonUp(im);
							}
	                 	    else
							{
								data->view->webView->onMouseButtonDown(im);
							}
							break;

						case IECODE_MBUTTON:
							if (Code & IECODE_UP_PREFIX)
							{
								// XXX: I find this a bit complicated... Add some function to hide this
								bool was_scrolling = data->is_scrolling;

								// If we are in first scrolling mode, abort it
								if(data->is_scrolling && data->scrolling_mode == 1)
								{
									data->is_scrolling   = 0;
									data->scrolling_mode = 0;
								}
								
								// If we are in second scrolling, abort it
								if(data->is_scrolling && data->scrolling_mode == 2)
								{
                                    autoscroll_remove(obj, data);
								}
								else if(!data->is_scrolling)
								{
									// If we were in first scrolling mode
									if(was_scrolling)
									{
										// And that we doubleclicked, switch to second scrolling mode
										if((currentTime() - data->lastMMBClickTime) < 0.5)
										{
                                            autoscroll_add(obj, data, &im);										   
										}
										// Else don't trigger the button up, it's not desired, especially over a link
									}
									// Pass the event (link, whatever)
									else
									{
										data->view->webView->onMouseButtonUp(im);
									}
								}

								data->lastMMBClickTime = currentTime();
							}
							else
							{
								if(!data->is_scrolling)
								{
									if(!data->view->webView->onMouseButtonDown(im))
									{
										data->is_scrolling   = 1;
										data->scrolling_mode = 1;
									}
								}
							}

	                 	    break;
                    }
                }
				else
				{
					// Abort scrolling when mouse is outside
					if(data->is_scrolling && data->scrolling_mode == 1)
					{
						data->is_scrolling   = 0;
						data->scrolling_mode = 0;
					}

					if(data->is_active)
		    	    {
						if(Code & IECODE_UP_PREFIX)
						{
							data->view->webView->onMouseButtonUp(im);
						}

						switch(Code & ~IECODE_UP_PREFIX)
	                    {
							case IECODE_LBUTTON:
								set(obj, MA_OWBBrowser_Active, FALSE);
								break;
							default:
								;
						}
		    	    }
				}
			}
			break;

	        case IDCMP_MOUSEMOVE:
			{
				if(data->is_scrolling)
				{
					// Act depending on scrolling mode
					if(data->scrolling_mode == 1)
					{
					  Frame* frame = &(data->view->webView->page()->focusController().focusedOrMainFrame());
						if(frame)
						{
							FrameView *view = frame->view();
							if(view)
							{
#if OS(AROS)
							    DoMethod(obj, MM_OWBBrowser_Autofill_HidePopup);
#endif
								view->scrollBy(Delta);
							}
						}
					}
					else if(data->scrolling_mode == 2)
					{
						data->autoscroll_delta = IntSize(im.MouseX - data->autoscroll_position.x(), im.MouseY - data->autoscroll_position.y());
					}
				}
				else
				{
		            mouse_inside = 0;
					if(_isinobject(obj, MouseX, MouseY) || data->is_active)
					{
						data->view->webView->onMouseMotion(im);
						mouse_inside = 1;
					}
				}
	        }
	        break;

            case IDCMP_RAWKEY:
			{
				bool forwardtoWebview = false;

				switch(Code)
				{
					case RAWKEY_TAB:
					{
						Object *active = (Object *) getv(_win(obj), MUIA_Window_ActiveObject);
						if(active == obj && data->is_active == 0)
					    {
							/* WebView Object got activated in the event chain */
							data->view->webView->clearFocusNode();
                            data->view->webView->setInitialFocus(true);
							data->is_active = TRUE;
							data->view->webView->updateFocusedAndActiveState();
							rc = MUI_EventHandlerRC_Eat;
							break;
					    }
						else if(active != obj && data->is_active == 0)
					    {
							/* Ignore tab events if object is not active */
							break;
					    }

						/* Override active object if needed, so toggling between page
						 * elements won't interfere with MUI object cycle */
						if(data->is_active)
						{
							set(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_None);
							forwardtoWebview = true;
                            rc = MUI_EventHandlerRC_Eat;
						}
						
						break;
					}

					/* Scroll event, do it here, and don't pass to webview */
					case RAWKEY_NM_WHEEL_UP:
					case RAWKEY_NM_WHEEL_DOWN:
					case RAWKEY_NM_WHEEL_LEFT:
					case RAWKEY_NM_WHEEL_RIGHT:
					{
						if(!(Qualifier & IEQUALIFIER_CONTROL) && _isinobject(obj, MouseX, MouseY))
						{
#if OS(AROS)
						    DoMethod(obj, MM_OWBBrowser_Autofill_HidePopup);
#endif
							data->view->webView->onScroll(im);
						}
						else
						{
							forwardtoWebview = true;
                            rc = MUI_EventHandlerRC_Eat;
						}

						break;
					}

					/* These keys when used with qualifiers are used to control title navigation,
					 * so don't pass them to webview */
					case RAWKEY_KP_0:
					case RAWKEY_KP_1:
					case RAWKEY_KP_2:
					case RAWKEY_KP_3:
					case RAWKEY_KP_4:
					case RAWKEY_KP_5:
					case RAWKEY_KP_6:
					case RAWKEY_KP_7:
					case RAWKEY_KP_8:
					case RAWKEY_KP_9:
					case RAWKEY_PAGEUP:
					case RAWKEY_PAGEDOWN:
					{
						if(!(Qualifier & (IEQUALIFIER_CONTROL | IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)))
						{
							forwardtoWebview = true;
                            rc = MUI_EventHandlerRC_Eat;
						}

						break;
					}

					default:
					{
                        forwardtoWebview = true;

						if(data->is_active)
						{
							rc = MUI_EventHandlerRC_Eat;
						}
						break;
					}
				}

				if(forwardtoWebview)
				{
					if (Code & IECODE_UP_PREFIX)
						data->view->webView->onKeyUp(im);
                    else
						data->view->webView->onKeyDown(im);
				}
            }
            break;

			default:
				;
	    }

		// Set mouse pointer if needed (ignore that when scrolling)
		if(!data->is_scrolling)
		{
			mouse_inside = _isinobject(obj, MouseX, MouseY);

			if (data->mouse_inside != mouse_inside)
			{
				data->mouse_inside = mouse_inside;
				if(is_morphos2() && getv(app, MA_OWBApp_EnablePointers))
				{
					set(_window(obj), WA_PointerType, mouse_inside ? data->pointertype : POINTERTYPE_NORMAL);
				}
			}
		}
	}

	return rc;
}

#define MADF_KNOWSACTIVE       (1<< 7) /* private */

DEFMMETHOD(GoActive)
{
	/*
	GETDATA;
	kprintf("GoActive\n");
	data->is_active = TRUE;
	data->view->webView->updateFocusedAndActiveState();
	_flags(obj) &= ~MADF_KNOWSACTIVE;
	*/
	return DOSUPER;
}

DEFMMETHOD(GoInactive)
{
	/*
	GETDATA;
	kprintf("GoInactive\n");
	data->is_active = FALSE;
	data->view->webView->updateFocusedAndActiveState();
	*/
	return DOSUPER;
}

DEFTMETHOD(OWBBrowser_FocusChanged)
{
	/*
	GETDATA;
	data->is_active = ((Object *) getv(_win(obj), MUIA_Window_ActiveObject)) == obj;
	kprintf("FocusChanged: activeobject %d\n", ((Object *) getv(_win(obj), MUIA_Window_ActiveObject)) == obj);
	*/

	return 0;
}

DEFSMETHOD(OWBBrowser_DidCommitLoad)
{
	return 0;
}

DEFSMETHOD(OWBBrowser_DidStartProvisionalLoad)
{
	/*
	DoMethod(obj, MM_OWBBrowser_ColorChooser_HidePopup);
	DoMethod(obj, MM_OWBBrowser_DateTimeChooser_HidePopup);
	*/

	return 0;
}

DEFSMETHOD(OWBBrowser_WillCloseFrame)
{
	DoMethod(obj, MM_OWBBrowser_ColorChooser_HidePopup);
	DoMethod(obj, MM_OWBBrowser_DateTimeChooser_HidePopup);

	// Add anything that might need cleanup 

	return 0;
}

#if OS(AROS)
static void cancelled(struct Hook *hook, Object *popupwin, APTR *dummy)
{
    set(popupwin, MUIA_Window_Open, FALSE);
}

static void closed(struct Hook *hook, Object *popupwin, APTR *params)
{
    PopupMenuMorphOS *that = (PopupMenuMorphOS *)params[0];
    int itemCount = (int)params[1];
    char ** items = (char **)params[2];
    that->client()->popupDidHide();

    DoMethod(_app(popupwin), OM_REMMEMBER, popupwin);

    for (int i = 0; i < itemCount; ++i)
        free(items[i]);
    free(items);

    MUI_DisposeObject(popupwin);
}

static void itemActivated(struct Hook *hook, Object *list, PopupMenuMorphOS **that)
{
    IPTR activeItem; Object * popupwin;
    GetAttr(MUIA_List_Active, list, (IPTR*) &activeItem);
    D(bug("setting active item to %d\n", activeItem));
    (*that)->client()->setTextFromItem(activeItem);
    (*that)->client()->valueChanged(activeItem);
    GetAttr(MUIA_WindowObject, list, (IPTR*)&popupwin);

    /* It has to be this way because List object crashes if we close its parent window
     * inside a notification call for this object */
    DoMethod(_app(popupwin), MUIM_Application_PushMethod, popupwin, 3, MUIM_Set, MUIA_Window_Open, (IPTR) FALSE);
}

#endif

DEFSMETHOD(OWBBrowser_PopupMenu)
{
#if OS(MORPHOS)
    GETDATA;

    Object *menulistgroup;
    
    PopupMenuMorphOS *pop = static_cast<PopupMenuMorphOS *>(msg->popupinfo);

    if(!pop || !pop->client()) return 0;

    //RefPtr<PopupMenuMorphOS> guard(pop);

    BalWidget* widget = pop->client()->hostWindow()->platformPageClient();
    if (!widget)
	return 0;

    if (widget != data->view)
	return 0;

    int itemCount = pop->client()->listSize();
    if (itemCount > 25*25)
	itemCount = 25*25;

    if (data->popmenu)
    {
	MUI_DisposeObject(data->popmenu);
	data->popmenu = NULL;
    }

    data->popmenu = (Object *) MenustripObject,
	Child, menulistgroup = MenuObject, MUIA_Menu_Title, "",	End,
	End;

    if(data->popmenu)
    {
	ULONG rc;
	int i;
	
	char *labels[itemCount];
	
	if(itemCount <= 25)
	{
	    for (i = 0; i < itemCount; ++i)
	    {
		char *text = strdup((char *) pop->client()->itemText(i).utf8().data());
		char *prefix = (char *) (pop->client()->itemIsSelected(i) ? "\033b" : "");
		char *tmp = utf8_to_local(text);
		free(text);

		if(tmp)
		{
		    labels[i] = (char *) malloc(strlen(tmp) + strlen(prefix) + 1);
		    
		    if(labels[i])
		    {
                        sprintf(labels[i], "%s%s", prefix, tmp);
			
			DoMethod(menulistgroup, OM_ADDMEMBER,
				 MenuitemObject,
				 MUIA_UserData, i+1,
				 MUIA_Menuitem_Title, pop->client()->itemIsSeparator(i) ? NM_BARLABEL : labels[i],
				 MUIA_Menuitem_Enabled, pop->client()->itemIsEnabled(i),
				 End);
		    }
		    
		    free(tmp);
		}
		else
		{
		    labels[i] = NULL;
		}
	    }
	}
	else
	{
	    for (int i = 0; i < (itemCount + 24) / 25; i++)
	    {
		Object *menu = (Object *) NewObject(getmenuclass(), NULL, MUIA_Menu_Title, NULL, End;
						    
		if(menu)
		{
		    int j;
		    for (j = 0; i * 25 + j < itemCount && j < 25 ; j++)
		    {
			char *text = strdup((char *) pop->client()->itemText(i*25+j).utf8().data());
			char *prefix = (char *) (pop->client()->itemIsSelected(i*25+j) ? "\033b" : "");
			char *tmp = utf8_to_local(text);
			free(text);
			
			if(tmp)
			{
			    labels[i*25+j] = (char *) malloc(strlen(tmp) + strlen(prefix) + 1);

			    if(labels[i*25+j])
			    {
				sprintf(labels[i*25+j], "%s%s", prefix, tmp);
				
				DoMethod(menu, OM_ADDMEMBER,
					 MenuitemObject,
					 MUIA_UserData, i*25+j+1,
					 MUIA_Menuitem_Title, pop->client()->itemIsSeparator(i*25+j) ? NM_BARLABEL : labels[i*25+j],
					 MUIA_Menuitem_Enabled, pop->client()->itemIsEnabled(i*25+j),
					 End);
			    }

			    free(tmp);
			}
			else
			{
			    labels[i*25+j] = NULL;
			}
		    }
		    
		    String menutitle = pop->client()->itemText(i * 25);
		    
		    if (j > 0)
		    {
			menutitle.append(" ... ");
			menutitle.append(pop->client()->itemText(i * 25 + j - 1));
		    }

		    set(menu, MUIA_Menu_Title, utf8_to_local(menutitle.utf8().data()));

		    DoMethod(menulistgroup, OM_ADDMEMBER, menu);
		}
	    }
	}
	    
        rc = DoMethod(data->popmenu, MUIM_Menustrip_Popup, obj, 0, _mleft(obj) + pop->windowRect().x(), _mtop(obj) + pop->windowRect().y());

	if(rc)
	{
	    if(pop->client()) pop->client()->setTextFromItem(rc-1);
	    if(pop->client()) pop->client()->valueChanged(rc-1);
	}

	for (i = 0; i < itemCount; ++i)
	{
	    free(labels[i]);
	}

	if(pop->client())
	{
	    pop->client()->popupDidHide();
	}

	MUI_DisposeObject(data->popmenu);
	data->popmenu = NULL;
    }
#endif
#if OS(AROS)
    GETDATA;

    PopupMenuMorphOS *pop = static_cast<PopupMenuMorphOS *>(msg->popupinfo);

    if(!pop || !pop->client()) return 0;

    RefPtr<PopupMenuMorphOS> guard(pop);

    BalWidget* widget = pop->client()->hostWindow()->platformPageClient();
    if (!widget)
        return 0;

    if (widget != data->view)
        return 0;

    int itemCount = pop->client()->listSize();

    char ** items = (char**) malloc(sizeof(char*) * (itemCount + 1));
    memset(items, 0, sizeof(char*) * (itemCount + 1));
    for (int i = 0; i < itemCount; ++i)
    {
        String text = pop->client()->itemText(i);

        if(text.isEmpty())
            items[i] = strdup("");
        else
        {
            char *textChars = strdup(text.utf8().data());

            items[i] = utf8_to_local(textChars);

            free(textChars);
        }
    }

    Object *list, *popupwin, *listview;

    popupwin = WindowObject,
        MUIA_Window_Borderless, (IPTR) TRUE,
        MUIA_Window_DragBar, (IPTR) FALSE,
        MUIA_Window_CloseGadget, (IPTR) FALSE,
        MUIA_Window_DepthGadget, (IPTR) FALSE,
        MUIA_Window_SizeGadget, (IPTR) FALSE,
        WindowContents,
            listview = ListviewObject,
                MUIA_Listview_List,
                    list = ListObject,
                        MUIA_List_SourceArray, items,
                    End,
            End,
        End;

    data->itemActivatedHook.h_Entry = (APTR) HookEntry;
    data->itemActivatedHook.h_SubEntry = (APTR) itemActivated;

    data->cancelledHook.h_Entry = (APTR) HookEntry;
    data->cancelledHook.h_SubEntry = (APTR) cancelled;

    data->hideHook.h_Entry = (APTR) HookEntry;
    data->hideHook.h_SubEntry = (APTR) (HOOKFUNC) closed;

    DoMethod(list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
        list, 3,
        MUIM_CallHook, &data->itemActivatedHook, (IPTR) pop);

    DoMethod(popupwin, MUIM_Notify, MUIA_Window_Activate, FALSE,
        popupwin, 2,
        MUIM_CallHook, &data->cancelledHook);

    DoMethod(popupwin, MUIM_Notify, MUIA_Window_Open, FALSE,
        popupwin, 5,
        MUIM_CallHook, &data->hideHook, (IPTR) pop, (IPTR)itemCount, (IPTR)items);

    /* Add popup window to application */
    DoMethod(_app(obj), OM_ADDMEMBER, popupwin);

    IntRect rScreenCoords(pop->windowRect());

    struct MUI_MinMax MinMaxInfo;
    DoMethod(listview, MUIM_AskMinMax, &MinMaxInfo);

    rScreenCoords.setHeight(MinMaxInfo.DefHeight);
    rScreenCoords.setWidth(max(rScreenCoords.width() + 13, (int) MinMaxInfo.DefWidth));

    /* Get offsets to compute absolute position of new window on the screen */
    IPTR winOffsetX, winOffsetY;
    struct Window *parentWindow;
    GetAttr(MUIA_Window, obj, (IPTR*) &parentWindow);
    winOffsetX = parentWindow->LeftEdge;
    winOffsetY = parentWindow->TopEdge;

    SetAttrs(popupwin,
        MUIA_Window_LeftEdge, (IPTR) _left(obj) + winOffsetX + rScreenCoords.x(),
        MUIA_Window_TopEdge, (IPTR) _top(obj) + winOffsetY + rScreenCoords.y(),
        MUIA_Window_Width, (IPTR) rScreenCoords.width(),
        TAG_DONE
    );

    DoMethod(_app(popupwin), MUIM_Application_PushMethod, popupwin, 3, MUIM_Set, MUIA_Window_Open, (IPTR) TRUE);
#endif
    
    return 0;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	if (data->contextmenu)
	{
		MUI_DisposeObject(data->contextmenu);
		data->contextmenu = NULL;
		data->menucontroller = NULL;
	}

	Page* page = core(data->view->webView);
    page->contextMenuController().clearContextMenu();
    Frame* focusedFrame = &(page->focusController().focusedOrMainFrame());

    if (!focusedFrame->view())
		return DOSUPER;

	IntuiMessage event;
	event.MouseX = msg->mx - _mleft(obj);
	event.MouseY = msg->my - _mtop(obj);
	event.Class = IDCMP_MOUSEBUTTONS;
	event.Code  = IECODE_RBUTTON;

	PlatformMouseEvent pevent = PlatformMouseEvent(&event);

	bool handledEvent = focusedFrame->eventHandler().sendContextMenuEvent(pevent);
    if (!handledEvent)
		return 0;

    ContextMenu* coreMenu = page->contextMenuController().contextMenu();
    if (!coreMenu)
		return 0;

	data->contextmenu = (Object *) MenustripObject,
									Child, (Object *) coreMenu->platformDescription(),
								    End;

	data->menucontroller = &page->contextMenuController();

	return (ULONG) data->contextmenu;
}

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;

	if (msg->item && data->menucontroller)
	{
		ContextMenuItem contextItem(msg->item);
		data->menucontroller->contextMenuItemSelected(&contextItem);
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_Autofill_ShowPopup)
{
	GETDATA;

	Vector<String> *suggestions = (Vector<String> *) msg->suggestions;
	size_t count = suggestions->size();

	if(count > 0)
	{
		if(!data->autofillCalltip)
		{
			data->autofillCalltip = (Object *) NewObject(getautofillpopupclass(), NULL,
												MA_AutofillPopup_Source, obj,
												MA_AutofillPopup_Rect, msg->rect,
												MA_AutofillPopup_Suggestions, suggestions,
												TAG_DONE);

			if(data->autofillCalltip)
			{
				DoMethod(app, OM_ADDMEMBER, data->autofillCalltip);
				set(data->autofillCalltip, MUIA_Window_Open, TRUE);
			}
		}
		else
		{
			DoMethod(data->autofillCalltip, MM_AutofillPopup_Update, suggestions);
		}
	}
	// If there's no candidates, just hide
	else
	{
		DoMethod(obj, MM_OWBBrowser_Autofill_HidePopup);
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_Autofill_HidePopup)
{
	GETDATA;

	if(data->autofillCalltip)
	{
		set(data->autofillCalltip, MUIA_Window_Open, FALSE);
		DoMethod(app, OM_REMMEMBER, data->autofillCalltip);
		MUI_DisposeObject(data->autofillCalltip);
		data->autofillCalltip = NULL;
	}
 
	return 0;
}

DEFSMETHOD(OWBBrowser_Autofill_DidSelect)
{
	GETDATA;

	String *value = (String *) msg->value;
	data->autofillManager->autofillTextField(*value);

	return 0;
}

DEFSMETHOD(OWBBrowser_Autofill_SaveTextFields)
{
	GETDATA;

	if(getv(app, MA_OWBApp_EnableFormAutofill))
	{
		data->autofillManager->saveTextFields((HTMLFormElement *)msg->form);
	}
	return 0;
}

DEFSMETHOD(OWBBrowser_Autofill_DidChangeInTextField)
{
	GETDATA;

	if(getv(app, MA_OWBApp_EnableFormAutofill))
	{
		data->autofillManager->didChangeInTextField((HTMLInputElement*) msg->element);
	}
	return 0;
}

DEFSMETHOD(OWBBrowser_Autofill_HandleNavigationEvent)
{
	GETDATA;
	ULONG handled = FALSE;

	if(getv(app, MA_OWBApp_EnableFormAutofill))
	{
		KeyboardEvent *event = (KeyboardEvent *) msg->event;
		const PlatformKeyboardEvent* keyEvent = event->keyEvent();
		if (keyEvent)
		{
			if (keyEvent->type() == PlatformKeyboardEvent::RawKeyDown)
			{
				int virtualKey = keyEvent->windowsVirtualKeyCode();

				if(data->autofillCalltip)
				{
					LONG action = 0;
					switch(virtualKey)
					{
						case VK_UP:
							action = MV_AutofillPopup_HandleNavigationEvent_Up;
							break;

						//case VK_TAB:
						case VK_DOWN:
							action = MV_AutofillPopup_HandleNavigationEvent_Down;
							break;					  

						case VK_RETURN:
							action = MV_AutofillPopup_HandleNavigationEvent_Accept;
							break;

						case VK_ESCAPE:
							action = MV_AutofillPopup_HandleNavigationEvent_Close;
							break;
					}

					handled = DoMethod(data->autofillCalltip, MM_AutofillPopup_HandleNavigationEvent, action);
				}
			}
		}
	}

	return handled;
}

DEFSMETHOD(OWBBrowser_ColorChooser_ShowPopup)
{
	GETDATA;

	DoMethod(obj, MM_OWBBrowser_ColorChooser_HidePopup);

	data->colorchooserCalltip = (Object *) NewObject(getcolorchooserpopupclass(), NULL,
										MA_ColorChooserPopup_Source, obj,
										MA_ColorChooserPopup_Controller, msg->client,
										MA_ColorChooserPopup_InitialColor, msg->color,
										TAG_DONE);

	if(data->colorchooserCalltip)
	{
		DoMethod(app, OM_ADDMEMBER, data->colorchooserCalltip);
		set(data->colorchooserCalltip, MUIA_Window_Open, TRUE);
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_ColorChooser_HidePopup)
{
	GETDATA;

	if(data->colorchooserCalltip)
	{
		set(data->colorchooserCalltip, MUIA_Window_Open, FALSE);
		DoMethod(app, OM_REMMEMBER, data->colorchooserCalltip);
		MUI_DisposeObject(data->colorchooserCalltip);
		data->colorchooserCalltip = NULL;
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_DateTimeChooser_ShowPopup)
{
	GETDATA;

	DoMethod(obj, MM_OWBBrowser_DateTimeChooser_HidePopup);

	data->datetimechooserCalltip = (Object *) NewObject(getdatetimechooserpopupclass(), NULL,
										MA_DateTimeChooserPopup_Source, obj,
										MA_DateTimeChooserPopup_Controller, msg->client,
										TAG_DONE);

	if(data->datetimechooserCalltip)
	{
		DoMethod(app, OM_ADDMEMBER, data->datetimechooserCalltip);
		set(data->datetimechooserCalltip, MUIA_Window_Open, TRUE);
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_DateTimeChooser_HidePopup)
{
	GETDATA;

	if(data->datetimechooserCalltip)
	{
		set(data->datetimechooserCalltip, MUIA_Window_Open, FALSE);
		DoMethod(app, OM_REMMEMBER, data->datetimechooserCalltip);
		MUI_DisposeObject(data->datetimechooserCalltip);
		data->datetimechooserCalltip = NULL;
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_SetScrollOffset)
{
	GETDATA;

	int x = (int) msg->x;
	int y = (int) msg->y;

	if(x == -1)
	{
		x = data->view->webView->scrollOffset().x;
	}

	if(y == -1)
	{
		y = data->view->webView->scrollOffset().y;
	}

	IntPoint diff(x - data->view->webView->scrollOffset().x, y - data->view->webView->scrollOffset().y);

	if (diff.y() || diff.x())
	{
		data->view->webView->scrollBy(diff);

		BalEventExpose ev = 0;
		data->view->webView->onExpose(ev);

		DoMethod(obj, MM_OWBBrowser_UpdateScrollers); // XXX: needed?
    }
	
	return 0;
}

DEFTMETHOD(OWBBrowser_UpdateScrollers)
{
	GETDATA;

#if ENABLE(VIDEO)
	if(data->video_fullscreen) return 0; // Custom backfill
#endif

	/* XXX: Optimize that */

	set(obj, MA_OWBBrowser_VTotalPixel, core(data->view->webView->mainFrame())->view()->contentsHeight());
	set(obj, MA_OWBBrowser_VVisiblePixel, core(data->view->webView->mainFrame())->view()->visibleHeight());
	set(obj, MA_OWBBrowser_VTopPixel, core(data->view->webView->mainFrame())->view()->visibleContentRect().y());

	set(obj, MA_OWBBrowser_HTotalPixel, core(data->view->webView->mainFrame())->view()->contentsWidth());
	set(obj, MA_OWBBrowser_HVisiblePixel, core(data->view->webView->mainFrame())->view()->visibleWidth());
	set(obj, MA_OWBBrowser_HTopPixel, core(data->view->webView->mainFrame())->view()->visibleContentRect().x());

	bool show_vbar = getv(obj, MA_OWBBrowser_VTotalPixel) != getv(obj, MA_OWBBrowser_VVisiblePixel);
	if((getv(data->vbar, MUIA_ShowMe) && !show_vbar) || (!getv(data->vbar, MUIA_ShowMe) && show_vbar))
	{	 
	  set(data->vbar, MUIA_ShowMe, show_vbar);
	}

	bool show_hbar = getv(obj, MA_OWBBrowser_HTotalPixel) != getv(obj, MA_OWBBrowser_HVisiblePixel);
	if((getv(data->hbargroup, MUIA_ShowMe) && !show_hbar) || (!getv(data->hbargroup, MUIA_ShowMe) && show_hbar) )
	{
		set(data->hbargroup, MUIA_ShowMe, show_hbar);
	}

	return 0;
}

DEFTMETHOD(OWBBrowser_UpdateNavigation)
{
	GETDATA;

	set(obj, MA_OWBBrowser_BackAvailable, core(data->view->webView)->backForward().canGoBackOrForward(-1));
	set(obj, MA_OWBBrowser_ForwardAvailable, core(data->view->webView)->backForward().canGoBackOrForward(1));
	set(obj, MA_OWBBrowser_StopAvailable, data->view->webView->isLoading());
	set(obj, MA_OWBBrowser_ReloadAvailable, TRUE);

	return 0;
}

DEFTMETHOD(OWBBrowser_ReturnFocus)
{
	GETDATA;

	DoMethod(_win(obj), MUIA_Window_ActiveObject, MUIV_Window_ActiveObject_Next);
	data->is_active = 0;
	data->view->webView->updateFocusedAndActiveState();

	return 0;
}

DEFSMETHOD(OWBBrowser_Print)
{
	GETDATA;
	Frame* frame = core(data->view->webView->mainFrame());

	if(frame)
	{
		// Surface size in points (1pt = 1/72 inch)
		float surfaceWidth = 8.27*72;
		float surfaceHeight = 11.73*72;

		// A4 Papersize in pixels
		float pageWidth = surfaceWidth;
		float pageHeight = surfaceHeight;

		FloatRect printRect = FloatRect(0, 0, pageWidth, pageHeight);

		// Margins
		float headerHeight = 0;
		float footerHeight = 0;

		float userscale = 1.0;

		OWBPrintContext printContext(frame);

		printContext.begin(pageWidth, pageHeight);

		printContext.computeAutomaticScaleFactor(FloatSize(pageWidth, pageHeight));

		if(getenv("OWB_PDF_SCALEFACTOR"))
		{
			userscale = strtod(getenv("OWB_PDF_SCALEFACTOR"), NULL);
		}

		printContext.computePageRects(printRect, headerHeight, footerHeight, userscale, pageHeight);

		_cairo_surface *surface = cairo_pdf_surface_create(msg->file, surfaceWidth, surfaceHeight);

		if(surface)
		{
			_cairo *cr = cairo_create(surface);

			if(cr)
			{
				if (frame->contentRenderer())
				{
					GraphicsContext ctx(cr);

					for(int i = 0; i < printContext.pageCount(); i++)
					{
						float shrinkfactor = printContext.getPageShrink(i);

						printContext.spoolPage(ctx, i, shrinkfactor);
						cairo_show_page(cr);
					}
				}

				cairo_destroy(cr);
			}

			cairo_surface_destroy(surface);
		}

		printContext.end();
	}

	return 0;
}

/* DragnDrop  */

static void fix_scanline(struct RastPort *rp, int y, int width, int minalpha)
{
	unsigned char buff[width * 4]; /* this will never be bigger than DRAGSIZE * 4 */
	int i;

	ReadPixelArray(buff, 0, 0, 0, rp, 0, y, width, 1, RECTFMT_RGBA);

	for(i=0; i<width; i++)
	{
		int a = buff[i * 4 + 0]; /* use red component */
		if (a < minalpha)
			a = minalpha;
		buff[i * 4 + 3] = a;
	}

	WritePixelArray(buff, 0, 0, 0, rp, 0, y, width, 1, RECTFMT_RGBA);
}

DEFMMETHOD(CreateDragImage)
{
	GETDATA;

	struct MUI_DragImage *di = (struct MUI_DragImage *) calloc(1, sizeof(*di));

	if(di)
	{
		Object *parent = _parent(obj);
		struct RastPort rp;
		LONG width;
		LONG height;
		STRPTR url;

		if (parent == NULL)
			return NULL;

		url = (STRPTR) getv(obj, MA_OWBBrowser_DragURL);

		// If we have a real dragimage, use it
		if(data->dragimage)
		{
			unsigned int stride = cairo_image_surface_get_stride((cairo_surface_t *)data->dragimage);
			unsigned char *src  = cairo_image_surface_get_data((cairo_surface_t *)data->dragimage);
			IntSize size = dragImageSize(data->dragimage);
			width  = size.width();
			height = size.height();

			if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) >= 24)
			{
				di->bm = AllocBitMap(width, height, 32, BMF_DISPLAYABLE | BMF_MINPLANES | BMF_CLEAR, _rp(obj)->BitMap);
			}
			else
			{
				di->bm = AllocBitMap(width, height, 32, BMF_CLEAR, NULL);
			}

			di->width  = width;
			di->height = height;
			di->touchx = di->width / 2;
			di->touchy = di->height / 2;
			di->flags  = MUIF_DRAGIMAGE_SOURCEALPHA;
#if !OS(AROS)
			di->mask   = NULL;
#endif

			InitRastPort(&rp);
			rp.BitMap = di->bm;
			WritePixelArray(src, 0,0, stride, &rp, 0, 0, width, height, RECTFMT_ARGB);
		}
		// Else, generate some text (XXX: consider more relevant data using MA_OWBBrowser_DragData)
#if 0
		else
		{
			struct TextFont *font = _font(obj);
			char buffer[128];
			size_t len;
			int i;

            if(url[0] == '\0')
			{
				url = "Empty";
			}

			stccpy(buffer, url, sizeof(buffer));

			len = strlen(buffer);
			width  = TextLength(_rp(obj), buffer, len) + 4;
			height = font->tf_YSize + 2;

			if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) >= 24)
			{
				di->bm = AllocBitMap(width, height, 32, BMF_DISPLAYABLE | BMF_MINPLANES | BMF_CLEAR, _rp(obj)->BitMap);
			}
			else
			{
				di->bm = AllocBitMap(width, height, 32, BMF_CLEAR, NULL);
			}

			di->width  = width;
			di->height = height;
			di->touchx = di->width / 2;
			di->touchy = di->height / 2;
			di->flags  = MUIF_DRAGIMAGE_SOURCEALPHA;
			di->mask   = NULL;

			InitRastPort(&rp);
			rp.BitMap = di->bm;
			SetRPAttrs(&rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, 0x7f000000, TAG_DONE);
			RectFill(&rp, 0, 0, di->width, di->height);

			SetFont(&rp, font);
			SetRPAttrs(&rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, 0xffffffff, RPTAG_BgColor, 0x7f000000, TAG_DONE);
			Move(&rp, 2, di->height - font->tf_YSize + font->tf_Baseline - 1);
			Text(&rp, buffer, len);
			/* ttf rendering messes alpha channel, so fix it */
			for(i = di->height - font->tf_YSize - 1; i < di->height; i++)
				fix_scanline(&rp, i, di->width, 0xbf);
		}
#else
		else
		{
			struct TextFont *font = _font(obj);
			Vector<String> lines;
			char *converted = NULL;
			String buffer;
			size_t maxindex = 0;
			size_t maxlen = 0;
			size_t i, j;

			if(!url || url[0] == '\0')
			{
				url = "Empty";
			}

			converted = local_to_utf8(url);

			if(converted)
			{
				buffer = String::fromUTF8(converted);
				free(converted);
			}

			if(buffer.find('\n') != notFound)
			{
				buffer = buffer.substring(0, 512);
			}

			buffer.split("\n", true, lines);

			for(i = 0; i < lines.size(); i++)
			{
				lines[i] = truncate(lines[i], 64);

				if(lines[i].length() > maxlen)
				{
					maxlen = lines[i].length();
					maxindex = i;
				}
			}

			converted = utf8_to_local(lines[maxindex].utf8().data());

			if(converted)
			{
				width  = TextLength(_rp(obj), converted, maxlen) + 4;
				height = (font->tf_YSize + 2 ) * lines.size() ;

				free(converted);

				if (GetBitMapAttr(_rp(obj)->BitMap, BMA_DEPTH) >= 24)
				{
					di->bm = AllocBitMap(width, height, 32, BMF_DISPLAYABLE | BMF_MINPLANES | BMF_CLEAR, _rp(obj)->BitMap);
				}
				else
				{
					di->bm = AllocBitMap(width, height, 32, BMF_CLEAR, NULL);
				}

				di->width  = width;
				di->height = height;
				di->touchx = di->width / 2;
				di->touchy = di->height / 2;
				di->flags  = MUIF_DRAGIMAGE_SOURCEALPHA;
#if !OS(AROS)
				di->mask   = NULL;
#endif

				InitRastPort(&rp);
				rp.BitMap = di->bm;
#if OS(AROS)
				{
				    struct TagItem tags [] =
				    {
				        { RPTAG_PenMode, FALSE },
				        { RPTAG_FgColor, 0x7f000000 },
				        { TAG_DONE, TAG_DONE }
				    };
				    SetRPAttrsA(&rp, tags);
				}
#else
				SetRPAttrs(&rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, 0x7f000000, TAG_DONE);
#endif
				RectFill(&rp, 0, 0, di->width, di->height);

				SetFont(&rp, font);
#if OS(AROS)
				{
				    struct TagItem tags [] =
				    {
				        { RPTAG_PenMode, FALSE },
				        { RPTAG_FgColor, 0xffffffff },
				        { RPTAG_BgColor, 0x7f000000 },
				        { TAG_DONE, TAG_DONE }
				    };
				    SetRPAttrsA(&rp, tags);
				}
#else
				SetRPAttrs(&rp, RPTAG_PenMode, FALSE, RPTAG_FgColor, 0xffffffff, RPTAG_BgColor, 0x7f000000, TAG_DONE);
#endif
				
				for(i = 0; i < lines.size(); i++)
				{
					Move(&rp, 2, di->height - (font->tf_YSize + 1) * (lines.size() - i) + font->tf_Baseline);
					
					converted = utf8_to_local(lines[i].utf8().data());

					if(converted)
					{
						Text(&rp, converted, lines[i].length());
						free(converted);

						/* ttf rendering messes alpha channel, so fix it */
						for(j = di->height - (font->tf_YSize + 1) * (lines.size() - i); j < di->height - (font->tf_YSize + 1) * (lines.size() - (i + 1)) ; j++)
							fix_scanline(&rp, j, di->width, 0xbf);
					}
				}
			}
		}
#endif
	}

	return (ULONG)di;
}

DEFMMETHOD(DeleteDragImage)
{
	if (msg->di)
	{
		FreeBitMap(msg->di->bm);
		free(msg->di);
	}
	return 0;
}

DEFMMETHOD(DragQuery)
{
	GETDATA;

	D(kprintf("DragQuery from %p for %p\n", msg->obj, obj));

	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if (data->titleobj != msg->obj && (type == MV_OWB_ObjectType_QuickLink || type == MV_OWB_ObjectType_Browser
	 || type == MV_OWB_ObjectType_Bookmark || type == MV_OWB_ObjectType_Tab || type == MV_OWB_ObjectType_URL))
	{
		if(type == MV_OWB_ObjectType_Browser)
		{
			D(kprintf("Source object is a browser dataObject %p\n", dataObject));
			if(obj != msg->obj)
			{
				dataObject = (APTR) getv(msg->obj, MA_OWBBrowser_DragData);
				data->dragoperation = (LONG) getv(msg->obj, MA_OWBBrowser_DragOperation);
			}
		}
		else
		{
			dataObject = 0;
		}

		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragBegin)
{
	GETDATA;

	D(kprintf("DragBegin (%d %d)\n", data->last_position.x(), data->last_position.y()));

	if(dataObject)
	{
		IntPoint position(data->last_position);
		DragData dragData((DataObjectMorphOS *) dataObject, position, position, (DragOperation) data->dragoperation);
		D(kprintf("dragEntered\n"));
		/*DragOperation operation = */core(data->view->webView)->dragController().dragEntered(dragData);
	}

	return 0;
}

DEFMMETHOD(DragReport)
{
	GETDATA;

	//D(kprintf("DragReport (%d %d)\n", msg->x - _mleft(obj), msg->y - _mtop(obj)));

	if(!msg->update)
	{
		return MUIV_DragReport_Refresh;
	}

	if(dataObject)
	{
		IntPoint position(msg->x - _mleft(obj), msg->y - _mtop(obj));
		data->last_drag_position = position;
		DragData dragData((DataObjectMorphOS *) dataObject, position, position, (DragOperation) data->dragoperation);
		//D(kprintf("dragUpdated\n"));
		/*DragOperation operation = */core(data->view->webView)->dragController().dragUpdated(dragData);
	}

	return MUIV_DragReport_Continue;
}

DEFMMETHOD(DragFinish)
{
	D(kprintf("DragFinish (%d %d) dropfollows %d\n", data->last_drag_position.x(), data->last_drag_position.y(), msg->dropfollows));

	if(dataObject)
	{
#if !OS(AROS)
		// We leave the object without drop
		if(!msg->dropfollows)
		{
			IntPoint position(data->last_drag_position);
			DragData dragData((DataObjectMorphOS *) dataObject, position, position, (DragOperation) data->dragoperation);
			D(kprintf("dragExited\n"));
			core(data->view->webView)->dragController().dragExited(&dragData);
		}
#endif

		// XXX: how to detect there wasn't any drop at all?
	}

	return DOSUPER;
}

DEFMMETHOD(DragDrop)
{
	GETDATA;

	D(kprintf("DragDrop drop (%d %d) last_drag_position (%d %d)\n", msg->x - _mleft(obj), msg->y - _mtop(obj), data->last_drag_position.x(), data->last_drag_position.y())); // Wrong pos, why?

	if(dataObject)
	{
		//IntPoint position(msg->x - _left(obj), msg->y - _mtop(obj));
		IntPoint position(data->last_drag_position.x(), data->last_drag_position.y());

		// Indicate where drop ended
		Frame* frame = &(core(data->view->webView)->focusController().focusedOrMainFrame());
		if (frame)
		{
			IntuiMessage event;
			event.MouseX = position.x();
			event.MouseY = position.y();
			event.Class = IDCMP_MOUSEBUTTONS;
			event.Code  = IECODE_LBUTTON | IECODE_UP_PREFIX;

			PlatformMouseEvent pevent = PlatformMouseEvent(&event);
			D(kprintf("dragSourceEndedAt(%d %d)\n", position.x(), position.y()));
			frame->eventHandler().dragSourceEndedAt(pevent, (DragOperation) data->dragoperation);
		}

		// Perform the drag
		data->last_drag_position = position;
		DragData dragData((DataObjectMorphOS *) dataObject, position, position, (DragOperation) data->dragoperation);
		D(kprintf("performDrag\n"));
		core(data->view->webView)->dragController().performDragOperation(dragData);
		//D(kprintf("dragEnded\n"));
		//core(data->view->webView)->dragController()->dragEnded();
		//dataObject = 0;
	}
	else /* Drop from other elements */
	{
		LONG type = getv(msg->obj, MA_OWB_ObjectType);

		if(type == MV_OWB_ObjectType_Tab && _win(obj) != _win(msg->obj)) // Drop from a tab of another window
		{
			DoMethod(_win(msg->obj), MM_OWBWindow_DetachBrowser, (Object *) getv(msg->obj, MA_OWB_Browser), _win(obj));
		}
		else
		{
			STRPTR url = (STRPTR) getv(msg->obj, MA_OWB_URL);

			if(url)
			{
				DoMethod(_win(obj), MM_OWBWindow_LoadURL, url, NULL);
			}
		}
	}

	return TRUE;
}

DEFMMETHOD(CreateShortHelp)
{
	GETDATA;
	
    Frame* frame = core(data->view->webView->mainFrame());

    if (!frame)
		return 0;

	FrameView* v = frame->view();

	if(v)
	{
		IntPoint viewportPos = v->windowToContents(data->last_position);
		IntPoint point = IntPoint(viewportPos.x(), viewportPos.y());
		HitTestResult result(point);

		result = frame->eventHandler().hitTestResultAtPoint(point, false);
		
		if(!result.altDisplayString().isEmpty())
			return (ULONG) utf8_to_local(result.altDisplayString().utf8().data());
	}
	
	return 0;
}

DEFMMETHOD(CheckShortHelp)
{
	return FALSE;
}

DEFMMETHOD(DeleteShortHelp)
{
    free(msg->help);
    return 0;
}

/* Video stuff */

#if ENABLE(VIDEO)

static inline void _AndRectRect(struct Rectangle *rect, const struct Rectangle *limit)
{
	rect->MinX = max(rect->MinX, limit->MinX);
	rect->MinY = max(rect->MinY, limit->MinY);
	rect->MaxX = min(rect->MaxX, limit->MaxX);
	rect->MaxY = min(rect->MaxY, limit->MaxY);
}

static inline LONG IsValidRect(const struct Rectangle *rect)
{
	return rect->MinX <= rect->MaxX && rect->MinY <= rect->MaxY;
}

DEFMMETHOD(Backfill)
{
	GETDATA;
	WORD left = msg->left, top = msg->top, right = msg->right, bottom = msg->bottom;
	struct Rectangle b1, b2, k;
	struct Rectangle bounds = { left, top, right, bottom };

//	  kprintf("backfill %d %d %d %d x_offset %d y_offset %d\n", left, top, right, bottom, mygui->x_offset, mygui->y_offset);

	/* key rect */
	k.MinX = left + data->video_x_offset;
	k.MinY = top + data->video_y_offset;
	k.MaxX = right - data->video_x_offset;
	k.MaxY = bottom - data->video_y_offset;

	_AndRectRect(&k, &bounds);

	if (data->video_x_offset || data->video_y_offset)
	{
		if (data->video_x_offset)
		{
			/* left rect */
			b1.MinX = left;
			b1.MinY = top;
			b1.MaxX = left + data->video_x_offset;
			b1.MaxY = bottom;

			/* right rect */
			b2.MinX = right - data->video_x_offset;
			b2.MinY = top;
			b2.MaxX = right;
			b2.MaxY = bottom;
		}
		else if (data->video_y_offset)
		{
			/* top rect */
			b1.MinX = left;
			b1.MinY = top;
			b1.MaxX = right;
			b1.MaxY = top + data->video_y_offset;

			/* bottom rect */
			b2.MinX = left;
			b2.MinY = bottom - data->video_y_offset;
			b2.MaxX = right;
			b2.MaxY = bottom;
		}

		_AndRectRect(&b1, &bounds);
		_AndRectRect(&b2, &bounds);
	}

	/* draw rects, if visible */
	if (data->video_x_offset || data->video_y_offset)
	{
		if (IsValidRect(&b1))
		{
			FillPixelArray(_rp(obj), b1.MinX, b1.MinY,
			               b1.MaxX - b1.MinX + 1, b1.MaxY - b1.MinY + 1,
			               0x00000000);
		}

		if (IsValidRect(&b2))
		{
			FillPixelArray(_rp(obj), b2.MinX, b2.MinY,
			               b2.MaxX - b2.MinX + 1, b2.MaxY - b2.MinY + 1,
			               0x00000000);
		}
	}

	if (IsValidRect(&k))
	{
		FillPixelArray(_rp(obj), k.MinX, k.MinY,
		               k.MaxX - k.MinX + 1, k.MaxY - k.MinY + 1,
					   data->video_colorkey);
	}

	return (TRUE);
}

/*
DEFSMETHOD(OWBBrowser_VideoEnterFullWindow)
{
	GETDATA;

	if(msg->enable)
	{
		struct Window *window = (struct Window *) getv(_win(obj), MUIA_Window);

		if(window)
		{
			SetVLayerAttrTags(data->video_handle,
				   VOA_LeftIndent,   window->BorderLeft + data->video_x_offset,
				   VOA_RightIndent,  window->Width - window->BorderRight - 1 + data->video_x_offset,
			  	   VOA_TopIndent,    window->BorderTop + data->video_y_offset,
				   VOA_BottomIndent, window->Height - window->BorderBottom -1 + data->video_y_offset,
				   TAG_DONE);
		}
	}
	else
	{
	
	}

	return 0;
}
*/

DEFSMETHOD(OWBBrowser_VideoEnterFullPage)
{
	GETDATA;

	Element *e = (Element *) msg->element;
	HTMLMediaElement *element = (HTMLMediaElement *) msg->element;

	if(e && !e->isMediaElement()) return 0;

	if(data->video_element == element && data->video_fullscreen == msg->fullscreen) return 0;

	if(element)
	{
#if !OS(AROS)
		if(CGXVideoBase)
		{
			struct Window *window = (struct Window *) getv(_win(obj), MUIA_Window);

			if(window)
			{
				FloatSize size = element->player()->naturalSize();
				//kprintf("naturalsize %dx%d\n", size.width(), size.height());

				ULONG vlayer_width  = size.width() & -8;
				ULONG vlayer_height = size.height() & -2;

				data->video_mode = SRCFMT_YCbCr420;

				data->video_handle = CreateVLayerHandleTags(window->WScreen,
										VOA_SrcType,      data->video_mode,
										VOA_UseColorKey,  TRUE,
										VOA_UseBackfill,  FALSE,
										VOA_SrcWidth,     vlayer_width,
										VOA_SrcHeight,    vlayer_height,
										VOA_DoubleBuffer, TRUE,
									    TAG_DONE);

				if(!data->video_handle)
				{
					DoMethod(app, MM_OWBApp_AddConsoleMessage, "[MediaPlayer] Couldn't create planar overlay layer, trying chunky instead");

					data->video_mode = SRCFMT_YCbCr16;

					data->video_handle = CreateVLayerHandleTags(window->WScreen,
											VOA_SrcType,      data->video_mode,
											VOA_UseColorKey,  TRUE,
											VOA_UseBackfill,  FALSE,
											VOA_SrcWidth,     vlayer_width,
											VOA_SrcHeight,    vlayer_height,
											VOA_DoubleBuffer, TRUE,
										    TAG_DONE);
				}

				if(data->video_handle)
				{
					if ( ( (float) size.width() / (float) size.height()) < ( (float) _mwidth(obj) / (float) _mheight(obj)) )
					{
						// Width is too big
						data->video_y_offset = 0;
						data->video_x_offset = _mwidth(obj) - (ULONG) (_mheight(obj) * ( (float) size.width() / (float) size.height()));
						data->video_x_offset /= 2;
					}
					else
					{
						// Height too big
						data->video_y_offset = _mheight(obj) - (ULONG) (_mwidth(obj) * ( (float) size.height() / (float) size.width()));
						data->video_y_offset /= 2;
						data->video_x_offset = 0;
					}

					if(0 == AttachVLayerTags(data->video_handle, window,
											  VOA_LeftIndent,   _mleft(obj) - window->BorderLeft + data->video_x_offset,
											  VOA_RightIndent,  window->Width - window->BorderRight - 1 - _mright(obj) + data->video_x_offset,
										 	  VOA_TopIndent,    _mtop(obj) - window->BorderTop + data->video_y_offset,
									    	  VOA_BottomIndent, window->Height - window->BorderBottom -1 - _mbottom(obj) + data->video_y_offset,
								              TAG_DONE))
					{
						data->video_fullscreen = msg->fullscreen;
						data->video_element    = element;
						data->video_colorkey   = GetVLayerAttr(data->video_handle, VOA_ColorKey);

						enable_blanker(_screen(obj), FALSE);

						// Change backfill behaviour
						set(obj, MUIA_FillArea,       TRUE);
						set(obj, MUIA_CustomBackfill, TRUE);
						DoMethod(obj, MUIM_Backfill, _mleft(obj), _mtop(obj), _mright(obj), _mbottom(obj), 0, 0, 0);

						if(data->video_mode == SRCFMT_YCbCr16)
						{
							element->player()->setOutputPixelFormat(AC_OUTPUT_YUV422);
						}
						else if(data->video_mode == SRCFMT_YCbCr420)
						{                            
							element->player()->setOutputPixelFormat(AC_OUTPUT_YUV420P);
						}

						set(data->hbargroup, MUIA_ShowMe, FALSE);
						set(data->vbar, MUIA_ShowMe, FALSE);

						Object *mediacontrolsgroup = (Object *) getv(_parent(_parent(obj)), MA_OWBGroup_MediaControlsGroup);
						if(mediacontrolsgroup)
						{
							set(mediacontrolsgroup, MA_MediaControlsGroup_Browser, obj);
							set(mediacontrolsgroup, MUIA_ShowMe, TRUE);
						}
					}
					else
					{
						DeleteVLayerHandle(data->video_handle);
						data->video_handle = NULL;
					}			 
				}
				else
				{
					DoMethod(app, MM_OWBApp_AddConsoleMessage, "[MediaPlayer] Couldn't create overlay layer");
				}
			}	 
		}
#endif
	}
	else
	{
		Object *mediacontrolsgroup = (Object *) getv(_parent(_parent(obj)), MA_OWBGroup_MediaControlsGroup);
		if(mediacontrolsgroup)
		{
			set(mediacontrolsgroup, MUIA_ShowMe, FALSE);
			set(mediacontrolsgroup, MA_MediaControlsGroup_Browser, NULL);
		}

		// Switch back to RGBA mode
		if(data->video_element)
		{
			data->video_element->player()->setOutputPixelFormat(AC_OUTPUT_RGBA32);
		}

		// Restore fill behaviour
		set(obj, MUIA_FillArea,       FALSE);
		set(obj, MUIA_CustomBackfill, FALSE);

		data->video_element    = NULL;
		data->video_fullscreen = FALSE;

#if !OS(AROS)
		// Destroy vlayer
		if (CGXVideoBase)
		{
			if (data->video_handle)
			{
				DetachVLayer(data->video_handle);
				DeleteVLayerHandle(data->video_handle);
				data->video_handle = NULL;
			}
		}
#endif

		// Redraw the page
		data->view->webView->addToDirtyRegion(IntRect(0, 0, data->width, data->height));
		data->dirty = TRUE;
		DoMethod(obj, MM_OWBBrowser_Expose, FALSE);

		enable_blanker(_screen(obj), TRUE);
	}

	return 0;
}

DEFSMETHOD(OWBBrowser_VideoBlit)
{
#if !OS(AROS)
	GETDATA;

	//kprintf("blitoverlay %d %d %d\n", msg->width, msg->height, msg->linesize);

	if(data->video_handle && msg->src && msg->stride && LockVLayer(data->video_handle))
	{
		int w = msg->width & -8;
		int h = msg->height & -2;
		int x = 0;
		int y = 0;

		switch(data->video_mode)
		{
			case SRCFMT_YCbCr16:
			{
				UBYTE *dYUV;
				UBYTE *sYUV;
				ULONG dtYUV, stYUV;

				sYUV = msg->src[0];
				stYUV = msg->stride[0];

				if(!sYUV || !stYUV)
					break;

				dtYUV = GetVLayerAttr(data->video_handle, VOA_Modulo);
				dYUV = (UBYTE *)GetVLayerAttr(data->video_handle, VOA_BaseAddress);
				dYUV += (y * dtYUV) + x;

				if (stYUV == dtYUV && w == msg->width)
				{
					CopyMem(sYUV, dYUV, dtYUV * h);
				}
				else do
				{
					CopyMem(sYUV, dYUV, w);
					dYUV += dtYUV;
					sYUV += stYUV;
				} while (--h > 0);
			
				break;
			}

			case SRCFMT_YCbCr420:
			{
				UBYTE *pY, *pCb, *pCr;
				UBYTE *sY, *sCb, *sCr;
				ULONG ptY, stY, ptCb, stCb, ptCr, stCr;
				ULONG w2 = w >> 1;

				h &= -2;
				w &= -2;
				sY   = msg->src[0];
				sCb  = msg->src[1];
				sCr  = msg->src[2];
				stY  = msg->stride[0];
				stCb = msg->stride[1];
				stCr = msg->stride[2];

				if(!(sY && sCb && sCr && stY && stCb && stCr))
					break;

				ptY = GetVLayerAttr(data->video_handle, VOA_Modulo) >> 1;
				ptCr = ptCb = ptY >> 1;
				pY = (UBYTE *)GetVLayerAttr(data->video_handle, VOA_BaseAddress);
				pCb = pY + (ptY * msg->height);
				pCr = pCb + ((ptCb * msg->height) >> 1);
				pY += (y * ptY) + x;
				pCb += ((y * ptCb) >> 1) + (x >> 1);
				pCr += ((y * ptCr) >> 1) + (x >> 1);

				if (stY == ptY && w == msg->width)
				{
					CopyMem(sY, pY, ptY * h);
					CopyMem(sCb, pCb, (ptCb * h) >> 1);
					CopyMem(sCr, pCr, (ptCr * h) >> 1);
				}
				else do
				{
					CopyMem(sY, pY, w);

					pY += ptY;
					sY += stY;

					CopyMem(sY, pY, w);
					CopyMem(sCb, pCb, w2);
					CopyMem(sCr, pCr, w2);

					sY += stY;
					sCb += stCb;
					sCr += stCr;

					pY += ptY;
					pCb += ptCb;
					pCr += ptCr;

					h -= 2;
				} while (h > 0);

				break;
			}
		}

		UnlockVLayer(data->video_handle);
        SwapVLayerBuffer(data->video_handle);
	}
#endif

	return 0;
}

#endif

/*****************************************************************************/

/* Plugin Methods */

typedef struct
{
	long x;
	long y;
	long width;
	long height;
} PluginRect;

// This is really a workaround to render faster, because everything should go through WebKit compositing
// It has some sideeffects for pages that want to paint over plugin area
DEFSMETHOD(Plugin_RenderRastPort)
{
	if(msg->src == NULL && msg->stride == NULL)
	{
		PluginRect *npwindowrect = (PluginRect *) msg->windowrect;
		IntRect windowrect(npwindowrect->x, npwindowrect->y, npwindowrect->width, npwindowrect->height);

		PluginRect *nprect = (PluginRect *) msg->rect;
		IntRect rect(nprect->x + npwindowrect->x, nprect->y + npwindowrect->y, nprect->width, nprect->height);

		D(kprintf("RenderRastPort rect [%d, %d, %d, %d] windowrect [%d, %d, %d, %d] src 0x%p stride %lu\n",
				 nprect->x, nprect->y, nprect->width, nprect->height,
				 npwindowrect->x, npwindowrect->y, npwindowrect->width, npwindowrect->height,
				 msg->src, msg->stride));

		return DoMethod(obj, MM_OWBBrowser_Update, &rect);
	}
	else
	{
		GETDATA;

		PluginRect *nprect = (PluginRect *) msg->rect;
		IntRect rect(nprect->x, nprect->y, nprect->width, nprect->height);

		PluginRect *npwindowrect = (PluginRect *) msg->windowrect;
		IntRect windowrect(npwindowrect->x, npwindowrect->y, npwindowrect->width, npwindowrect->height);

		D(kprintf("RenderRastPort rect [%d, %d, %d, %d] windowrect [%d, %d, %d, %d] src 0x%p stride %lu\n",
				 nprect->x, nprect->y, nprect->width, nprect->height,
				 npwindowrect->x, npwindowrect->y, npwindowrect->width, npwindowrect->height,
				 msg->src, msg->stride));

		data->update_x = windowrect.x() + rect.x();
		data->update_y = windowrect.y() + rect.y();
		data->update_width = rect.width();
		data->update_height = rect.height();

		data->plugin_src    = (unsigned char *) msg->src;
		data->plugin_stride = (unsigned int) msg->stride;
		data->plugin_update_x = rect.x();
		data->plugin_update_y = rect.y();
		data->plugin_update_width = rect.width();
		data->plugin_update_height = rect.height();

		if(data->update_x < 0)
		{
			data->update_width += data->update_x;
			data->plugin_update_x -= data->update_x;
			data->update_x = 0;
		}

		if(data->update_y < 0)
		{
			data->update_height += data->update_y;
			data->plugin_update_y -= data->update_y;
			data->update_y = 0;
		}

		if (data->update_width > _mwidth(obj) - data->update_x)
			data->update_width = _mwidth(obj) - data->update_x;

		if (data->update_height > _mheight(obj) - data->update_y)
			data->update_height = _mheight(obj) - data->update_y;

		data->draw_mode = DRAW_PLUGIN;
		MUI_Redraw(obj, MADF_DRAWUPDATE);

		return 0;
	}
}

DEFTMETHOD(Plugin_GetTop)
{
	LONG ret = - 1;
	if(muiRenderInfo(obj) && _win(obj))
	{
		GETDATA;
		ret = core(data->view->webView->mainFrame())->view()->visibleContentRect().y();
	}

	return ret;
}

DEFTMETHOD(Plugin_GetLeft)
{
	LONG ret = - 1;
	if(muiRenderInfo(obj) && _win(obj))
	{
		GETDATA;
		ret = core(data->view->webView->mainFrame())->view()->visibleContentRect().x();
	}

	return ret;
}

DEFTMETHOD(Plugin_GetWidth)
{
	LONG ret = - 1;
	if(muiRenderInfo(obj) && _win(obj))
	{
		GETDATA;
		ret = core(data->view->webView->mainFrame())->view()->visibleWidth();
	}

	return ret;
}

DEFTMETHOD(Plugin_GetHeight)
{
	LONG ret = - 1;
	if(muiRenderInfo(obj) && _win(obj))
	{
		GETDATA;
		ret = core(data->view->webView->mainFrame())->view()->visibleHeight();
	}

	return ret;
}

DEFSMETHOD(Plugin_GetSurface)
{
	GETDATA;
	return (ULONG) data->view->cr;
}

DEFTMETHOD(Plugin_IsBrowserActive)
{
	ULONG ret = FALSE;

	D(kprintf("IsBrowserActive\n"));

	if(muiRenderInfo(obj) && _win(obj) && getv(app, MA_OWBApp_ShouldAnimate))
	{
		if(obj == (Object *) getv(_win(obj), MA_OWBWindow_ActiveBrowser))
		{
			ret = TRUE;
		}
	}

	return ret;
}

DEFSMETHOD(Plugin_AddIDCMPHandler)
{
	GETDATA;
	struct EventHandlerNode *n;

	D(kprintf("Plugin_AddIDCMPHandler %p %p\n", msg->instance, msg->handlerfunc));

	n = (struct EventHandlerNode *) malloc(sizeof(*n));

	if(n)
	{
		n->instance = msg->instance;
		n->handlerfunc = msg->handlerfunc;

		ADDTAIL(&data->eventhandlerlist, n);
	}

	return 0;
}

DEFSMETHOD(Plugin_RemoveIDCMPHandler)
{
	GETDATA;
	APTR n, m;

	D(kprintf("Plugin_RemoveIDCMPHandler %p\n", msg->instance));

	ITERATELISTSAFE(n, m, &data->eventhandlerlist)
	{
		struct EventHandlerNode *ehn = (struct EventHandlerNode *) n;

		if(ehn->instance == msg->instance)
		{
			REMOVE(n);
			free(n);
			break;
		}
	}

	return 0;
}

class PluginTimer
{
public:
	Object *m_obj;
	APTR m_instance;
	APTR m_timeoutfunc;

	Timer m_pluginTimer;
	void start(unsigned long ms)
	{
		if(!m_pluginTimer.isActive())
			m_pluginTimer.startOneShot((double)(1.0*ms)/1000.0);
	}

	void stop()
	{
		if(m_pluginTimer.isActive())
			m_pluginTimer.stop();
	}

	void pluginFired()
	{
#if !OS(AROS)
		CALLFUNC2(m_timeoutfunc, m_instance, this);
#endif
	}

	PluginTimer(Object *obj, APTR instance, APTR timeoutfunc)
		: m_obj(obj),
		  m_instance(instance),
		  m_timeoutfunc(timeoutfunc),
		  m_pluginTimer(*this, &PluginTimer::pluginFired)
	{
	}
};

DEFSMETHOD(Plugin_AddTimeOut)
{
	PluginTimer *timer = new PluginTimer(obj, msg->instance, msg->timeoutfunc);

	if(timer)
	{
		timer->start(msg->delay);
	}

	return (ULONG) timer;
}

DEFSMETHOD(Plugin_RemoveTimeOut)
{
	if(msg->timer)
	{
		PluginTimer *timer = (PluginTimer *) msg->timer;
		timer->stop();
		delete timer;
	}

	return 0;
}

DEFSMETHOD(Plugin_Message)
{
	return DoMethod(app, MM_OWBApp_AddConsoleMessage, msg->message);
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECMMETHOD(GoActive)
DECMMETHOD(GoInactive)
DECTMETHOD(OWBBrowser_FocusChanged)
DECSMETHOD(OWBBrowser_DidCommitLoad)
DECSMETHOD(OWBBrowser_DidStartProvisionalLoad)
DECSMETHOD(OWBBrowser_WillCloseFrame)
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECTMETHOD(OWBBrowser_ReturnFocus)
DECSMETHOD(OWBBrowser_Expose)
DECSMETHOD(OWBBrowser_Update)
DECSMETHOD(OWBBrowser_Scroll)
DECSMETHOD(OWBBrowser_PopupMenu)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECSMETHOD(OWBBrowser_Autofill_ShowPopup)
DECSMETHOD(OWBBrowser_Autofill_HidePopup)
DECSMETHOD(OWBBrowser_Autofill_DidSelect)
DECSMETHOD(OWBBrowser_Autofill_SaveTextFields)
DECSMETHOD(OWBBrowser_Autofill_DidChangeInTextField)
DECSMETHOD(OWBBrowser_Autofill_HandleNavigationEvent)
DECSMETHOD(OWBBrowser_ColorChooser_ShowPopup)
DECSMETHOD(OWBBrowser_ColorChooser_HidePopup)
DECSMETHOD(OWBBrowser_DateTimeChooser_ShowPopup)
DECSMETHOD(OWBBrowser_DateTimeChooser_HidePopup)
DECSMETHOD(OWBBrowser_SetScrollOffset)
DECTMETHOD(OWBBrowser_UpdateScrollers)
DECTMETHOD(OWBBrowser_UpdateNavigation)
DECSMETHOD(OWBBrowser_Print)
DECTMETHOD(OWBBrowser_AutoScroll_Perform)
DECMMETHOD(CreateDragImage)
DECMMETHOD(DeleteDragImage)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(DragReport)
DECMMETHOD(DragBegin)
DECMMETHOD(DragFinish)
DECMMETHOD(CreateShortHelp)
DECMMETHOD(CheckShortHelp)
DECMMETHOD(DeleteShortHelp)
#if ENABLE(VIDEO)
DECSMETHOD(OWBBrowser_VideoEnterFullPage)
DECSMETHOD(OWBBrowser_VideoBlit)
DECMMETHOD(Backfill)
#endif

/* Plugin Methods */
DECSMETHOD(Plugin_RenderRastPort)
DECTMETHOD(Plugin_GetTop)
DECTMETHOD(Plugin_GetLeft)
DECTMETHOD(Plugin_GetWidth)
DECTMETHOD(Plugin_GetHeight)
DECSMETHOD(Plugin_GetSurface)
DECTMETHOD(Plugin_IsBrowserActive)
DECSMETHOD(Plugin_AddIDCMPHandler)
DECSMETHOD(Plugin_RemoveIDCMPHandler)
DECSMETHOD(Plugin_AddTimeOut)
DECSMETHOD(Plugin_RemoveTimeOut)
DECSMETHOD(Plugin_Message)

ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, owbbrowserclass)

