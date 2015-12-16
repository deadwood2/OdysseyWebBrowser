/*
 * Copyright 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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
#include "GraphicsContext.h"
#include "WebIconDatabase.h"
#include "BitmapImage.h"
#include <wtf/text/CString.h>
#include "URL.h"

#include <cairo.h>
#include <string.h>
#include <stdio.h>

#define SYSTEM_PRIVATE 1

#include <cybergraphx/cybergraphics.h>
#include <proto/alib.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>
#include <clib/debug_protos.h>

#include "gui.h"

#define D(x)

#define FAVICON_WIDTH 16
#define FAVICON_HEIGHT 16

#define FACTOR (data->scalefactor + 1)

using namespace WebCore;

struct Data
{
	ULONG need_redraw;
	ULONG isfolder;
	STRPTR url;
	cairo_surface_t *surface;
#if OS(AROS)
    APTR bgra;
#endif
	cairo_t *cr;
	ULONG scalefactor;
	ULONG retain;
};

STATIC VOID doset(struct Data *data, APTR obj, struct TagItem *taglist)
{
	struct TagItem *tag, *tstate;

	tstate = taglist;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		ULONG tag_data = tag->ti_Data;

		switch (tag->ti_Tag)
		{
			case MA_FavIcon_Retain:
			{
				data->retain = tag_data;
			}
			break;

			case MA_FavIcon_Folder:
			{
				data->isfolder = tag_data;
			}
			break;

			case MA_FavIcon_NeedRedraw:
			{
				D(kprintf("Setting MA_FavIcon_NeedRedraw for <%s>\n", data->url, tag_data));
				data->need_redraw = tag_data;
			}
			break;

			case MA_FavIcon_PageURL:
			{
				WebIconDatabase* sharedWebIconDatabase = WebIconDatabase::sharedWebIconDatabase();
					
				if(sharedWebIconDatabase)
				{
					ULONG sameURL = data->url && tag_data && strcmp((char *) tag_data, data->url) == 0;

					if(!sameURL)
					{
						char *tmp = (STRPTR) strdup(tag_data ? (char *)tag_data : (char *) "");

						if(data->url)
						{
							free(data->url);
						}

						data->url = tmp;
					}

					if(data->cr)
					{
						cairo_destroy(data->cr);
						data->cr = NULL;
					}

					if(data->surface)
					{
						cairo_surface_destroy(data->surface);
#if OS(AROS)
						if (data->bgra)
						{
						    ARGB2BGRAFREE(data->bgra);
						    data->bgra = NULL;
						}
#endif
					}
					data->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, FAVICON_WIDTH*FACTOR, FAVICON_HEIGHT*FACTOR);

					if(cairo_surface_status(data->surface) == CAIRO_STATUS_SUCCESS)
					{
						data->cr = cairo_create(data->surface);
					}

					if(data->cr)
					{
						WebCore::BitmapImage* icon = 0;

						static RefPtr<WebCore::Image> defaultIcon = 0;
						if (!defaultIcon)
							defaultIcon = WebCore::Image::loadPlatformResource("urlIcon");

						static RefPtr<WebCore::Image> defaultFolderIcon = 0;
						if (!defaultFolderIcon)
							defaultFolderIcon = WebCore::Image::loadPlatformResource("folderIcon");

						static RefPtr<WebCore::Image> defaultScriptIcon = 0;
						if (!defaultScriptIcon)
							defaultScriptIcon = WebCore::Image::loadPlatformResource("scriptIcon");

						static RefPtr<WebCore::Image> topsitesIcon = 0;
						if (!topsitesIcon)
							topsitesIcon = WebCore::Image::loadPlatformResource("topsitesIcon");

						static RefPtr<WebCore::Image> aboutIcon = 0;
						if (!aboutIcon)
							aboutIcon = WebCore::Image::loadPlatformResource("aboutIcon");

						if(data->isfolder)
						{
							icon = (WebCore::BitmapImage *) defaultFolderIcon.get();
						}
						else if(URL(ParsedURLString, data->url).protocolIs("javascript"))
						{
							icon = (WebCore::BitmapImage *) defaultScriptIcon.get();
						}
						else if(URL(ParsedURLString, data->url).string().startsWith("topsites"))
						{
							icon = (WebCore::BitmapImage *) topsitesIcon.get();
						}
						else if(URL(ParsedURLString, data->url).protocolIs("about"))
						{
							icon = (WebCore::BitmapImage *) aboutIcon.get();
						}
						else
						{
							sharedWebIconDatabase->retainIconForURL(data->url);
							icon = (WebCore::BitmapImage*) sharedWebIconDatabase->iconForURL((const char *) data->url, IntSize(FAVICON_WIDTH*FACTOR, FAVICON_HEIGHT*FACTOR), false);
						}

						if(!icon)
						{
							icon = (WebCore::BitmapImage*) defaultIcon.get();
						}

						if(icon)
						{
						        cairo_surface_t * icon_surface = icon->nativeImageForCurrentFrame().leakRef();

							if(!icon_surface)
							{
								icon = (WebCore::BitmapImage*) defaultIcon.get();

								if(icon)
								{
								  icon_surface = icon->nativeImageForCurrentFrame().leakRef();;
								}
							}
									
							if(icon_surface)
							{
								cairo_save(data->cr);

								cairo_set_source_rgba(data->cr, 0, 0, 0, 0);
								cairo_rectangle(data->cr, 0, 0, FAVICON_WIDTH*FACTOR, FAVICON_HEIGHT*FACTOR);
								cairo_fill(data->cr);

								cairo_scale(data->cr, ((double) FAVICON_WIDTH*FACTOR)/((double) icon->width()),
											      	  ((double) FAVICON_HEIGHT*FACTOR)/((double) icon->height()));

								cairo_set_source_surface(data->cr, icon_surface, 0, 0);
								cairo_paint(data->cr);

								cairo_restore(data->cr);

#if OS(AROS)
								data->bgra = ARGB2BGRA(cairo_image_surface_get_data(data->surface),
								        cairo_image_surface_get_stride(data->surface),
								        cairo_image_surface_get_height(data->surface));

#endif
										
								set(obj, MA_FavIcon_NeedRedraw, TRUE);
							}
						}
					}
				}
			}
			break;
		}
	}
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_FillArea, FALSE,
		InnerSpacing(0,0),
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;

		data->url = NULL;
		data->surface = NULL;
		data->cr = NULL;
		data->need_redraw = FALSE;
		data->retain = (ULONG) GetTagData(MA_FavIcon_Retain, TRUE, msg->ops_AttrList);
		data->scalefactor = (ULONG) GetTagData(MA_FavIcon_ScaleFactor, MV_FavIcon_Scale_Normal, msg->ops_AttrList);

		doset(data, obj, INITTAGS);

		DoMethod(app, MUIM_Notify, MA_OWBApp_DidReceiveFavIcon, MUIV_EveryTime, obj, 2, MM_FavIcon_DidReceiveFavIcon, MUIV_TriggerValue);
		DoMethod(app, MUIM_Notify, MA_OWBApp_FavIconImportComplete, MUIV_EveryTime, obj, 1, MM_FavIcon_Update);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;

	DoMethod(app, MUIM_KillNotifyObj, MA_OWBApp_DidReceiveFavIcon, obj);
	DoMethod(app, MUIM_KillNotifyObj, MA_OWBApp_FavIconImportComplete, obj);

	if(data->cr)
	{
		cairo_destroy(data->cr);
		data->cr = NULL;
	}

	if(data->surface)
	{
		cairo_surface_destroy(data->surface);
		data->surface = NULL;
	}

	if(data->url)
	{
		free(data->url);
		data->url = NULL;
	}

#if OS(AROS)
    if(data->bgra)
    {
        ARGB2BGRAFREE(data->bgra);
        data->bgra = NULL;
    }
#endif

	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_FavIcon_NeedRedraw:
			*msg->opg_Storage = data->need_redraw;
			return TRUE;
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(data, obj, msg->ops_AttrList);
	return DOSUPER;
}

DEFMMETHOD(AskMinMax)
{
	GETDATA;
	DOSUPER;

	msg->MinMaxInfo->MinWidth  += FAVICON_WIDTH*FACTOR;
	msg->MinMaxInfo->MinHeight += FAVICON_HEIGHT*FACTOR;
	msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
	msg->MinMaxInfo->DefWidth  += FAVICON_WIDTH*FACTOR;
	msg->MinMaxInfo->DefHeight += FAVICON_HEIGHT*FACTOR;

	return 0;
}

DEFMMETHOD(Draw)
{
	DOSUPER;

	if (msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))
	{
		GETDATA;
		ULONG mleft, mtop, mwidth, mheight;
		unsigned int stride;
		unsigned char *src;

		mleft   = _mleft(obj);
		mtop    = _mtop(obj);
		mwidth  = _mwidth(obj);
		mheight = _mheight(obj);

		DoMethod(obj, MUIM_DrawBackground, mleft, mtop, mwidth, mheight, 0, 0, 0);

		if(data->url && cairo_surface_status(data->surface) == CAIRO_STATUS_SUCCESS)
		{
			stride = cairo_image_surface_get_stride(data->surface);
#if OS(AROS)
			src = (unsigned char *)data->bgra;
#else
			src	= cairo_image_surface_get_data(data->surface);
#endif

			if(src)
			{
				WritePixelArrayAlpha((APTR) src, 0, 0, stride, _rp(obj), mleft, mtop, FAVICON_WIDTH*FACTOR, FAVICON_HEIGHT*FACTOR, 0xffffffff);
			}
		}
	}

	return 0;
}

DEFSMETHOD(FavIcon_DidReceiveFavIcon)
{
	GETDATA;

	if(msg->url)
	{
		if(data->url && !strcmp(msg->url, data->url))
		{
			set(obj, MA_FavIcon_PageURL, data->url);
		}
	}

	return 0;
}

DEFTMETHOD(FavIcon_Update)
{
	GETDATA;
	set(obj, MA_FavIcon_PageURL, data->url);
	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECSMETHOD(FavIcon_DidReceiveFavIcon)
DECTMETHOD(FavIcon_Update)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, faviconclass)
