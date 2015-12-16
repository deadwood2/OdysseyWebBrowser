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

#define ICON_WIDTH 22
#define ICON_HEIGHT 22

#define FACTOR (data->scalefactor + 1)

using namespace WebCore;

struct Data
{
	STRPTR path;
	cairo_surface_t *surface;
	cairo_t *cr;
	ULONG scalefactor;
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
			case MA_Icon_Path:
			{
				ULONG samePath = data->path && tag_data && strcmp((char *) tag_data, data->path) == 0;

				if(!samePath)
				{
					char *tmp = (STRPTR) strdup(tag_data ? (char *)tag_data : (char *) "");

					if(data->path)
					{
						free(data->path);
					}

					data->path = tmp;
				}

				if(data->cr)
				{
					cairo_destroy(data->cr);
					data->cr = NULL;
				}

				if(data->surface)
				{
					cairo_surface_destroy(data->surface);
				}
				data->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ICON_WIDTH*FACTOR, ICON_HEIGHT*FACTOR);

				if(cairo_surface_status(data->surface) == CAIRO_STATUS_SUCCESS)
				{
					data->cr = cairo_create(data->surface);
				}

				if(data->cr)
				{
					static RefPtr<WebCore::Image> defaultIcon = 0;
					if (!defaultIcon)
						defaultIcon = WebCore::Image::loadPlatformResource("defaultIcon");
											
					RefPtr<WebCore::Image> image = WebCore::Image::loadPlatformResource(data->path);
					WebCore::BitmapImage* icon = (WebCore::BitmapImage*) image.get();

					if(icon)
					{
						cairo_surface_t * icon_surface = icon->nativeImageForCurrentFrame().leakRef();

						if(!icon_surface)
						{
							icon = (WebCore::BitmapImage*) defaultIcon.get();

							if(icon)
							{
							  icon_surface = icon->nativeImageForCurrentFrame().leakRef();
							}
						}
								
						if(icon_surface)
						{
							cairo_save(data->cr);

							cairo_set_source_rgba(data->cr, 0, 0, 0, 0);
							cairo_rectangle(data->cr, 0, 0, ICON_WIDTH*FACTOR, ICON_HEIGHT*FACTOR);
							cairo_fill(data->cr);

							cairo_scale(data->cr, ((double) ICON_WIDTH*FACTOR)/((double) icon->width()),
												  ((double) ICON_HEIGHT*FACTOR)/((double) icon->height()));

							cairo_set_source_surface(data->cr, icon_surface, 0, 0);
							cairo_paint(data->cr);

							cairo_restore(data->cr);
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

		data->path = NULL;
		data->surface = NULL;
		data->cr = NULL;
		data->scalefactor = (ULONG) GetTagData(MA_Icon_ScaleFactor, MV_Icon_Scale_Normal, msg->ops_AttrList);

		doset(data, obj, INITTAGS);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;

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

	if(data->path)
	{
		free(data->path);
		data->path = NULL;
	}

	return DOSUPER;
}

DEFGET
{
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

	msg->MinMaxInfo->MinWidth  += ICON_WIDTH*FACTOR;
	msg->MinMaxInfo->MinHeight += ICON_HEIGHT*FACTOR;
	msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
	msg->MinMaxInfo->DefWidth  += ICON_WIDTH*FACTOR;
	msg->MinMaxInfo->DefHeight += ICON_HEIGHT*FACTOR;

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

		if(data->path && cairo_surface_status(data->surface) == CAIRO_STATUS_SUCCESS)
		{
			stride = cairo_image_surface_get_stride(data->surface);
			src	= cairo_image_surface_get_data(data->surface);

			if(src)
			{
				WritePixelArrayAlpha((APTR) src, 0, 0, stride, _rp(obj), mleft, mtop, ICON_WIDTH*FACTOR, ICON_HEIGHT*FACTOR, 0xffffffff);
			}
		}
	}

	return 0;
}


BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, iconclass)
