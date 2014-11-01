/*
 * Copyright 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 * Copyright 2009 Ilkka Lehtoranta <ilkleht@isoveli.org>
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
#include <string.h>
#include <cairo.h>

#define SYSTEM_PRIVATE 1

#include <cybergraphx/cybergraphics.h>
#include <proto/alib.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>

#include "gui.h"
#include "throbber.h"

struct Data
{
	cairo_surface_t *surface;
#if OS(AROS)
    APTR bgra;
#endif
	ULONG imagecount;
	ULONG imagewidth;
	ULONG imageheight;
	UWORD imagenum;
	UBYTE animate;
	UBYTE is_shown;
	UBYTE added;
	struct MUI_InputHandlerNode ihnode;
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
			case MA_TransferAnim_Animate:
				if (tag_data && data->animate == 0)
				{
					data->animate = 1;

					if (data->is_shown && !data->added)
					{
						DoMethod(_app(obj), MUIM_Application_AddInputHandler, (ULONG)&data->ihnode);
						data->added = TRUE;
					}

				}
				else if (!tag_data && data->animate)
				{
					data->animate = 0;

					if (data->is_shown && data->added)
					{
						DoMethod(_app(obj), MUIM_Application_RemInputHandler, (ULONG)&data->ihnode);
						data->added = FALSE;
					}

					data->imagenum = 0;
					MUI_Redraw((Object *)obj, MADF_DRAWUPDATE);
				}
				break;
		}
	}
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_FillArea, FALSE,
		MUIA_Frame   , getv(app, MA_OWBApp_ShowButtonFrame) ? MUIV_Frame_ImageButton : MUIV_Frame_None,
		MUIA_Weight  , 0,
		MUIA_DoubleBuffer, TRUE,
		TAG_MORE     , (IPTR)msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Flags = MUIIHNF_TIMER;
		data->ihnode.ihn_Millis = 30;
		data->ihnode.ihn_Method = MM_TransferAnim_Run;

		data->surface = cairo_image_surface_create_from_png ("PROGDIR:resource/transferanim.png");

		if(cairo_surface_status(data->surface) == CAIRO_STATUS_SUCCESS)
		{
			if(cairo_image_surface_get_width(data->surface) > 0 && cairo_image_surface_get_height(data->surface) > 0)
			{
				data->imagecount  = cairo_image_surface_get_width(data->surface) / cairo_image_surface_get_height(data->surface);
				data->imageheight = cairo_image_surface_get_height(data->surface);
				data->imagewidth  = data->imageheight;
#if OS(AROS)
                data->bgra = ARGB2BGRA(cairo_image_surface_get_data(data->surface),
                        cairo_image_surface_get_stride(data->surface),data->imageheight);
#endif
			}
			else
			{
				cairo_surface_destroy (data->surface);
				data->surface = NULL;
			}
		}
		else
		{
			cairo_surface_destroy (data->surface);
            data->surface = NULL;
		}

		if(!data->surface)
		{
			data->imagecount  = THROBBER_IMAGES;
			data->imageheight = THROBBER_HEIGHT;
			data->imagewidth  =	THROBBER_WIDTH / THROBBER_IMAGES;
		}

		doset(data, obj, msg->ops_AttrList);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;

	if(data->surface)
	{
		cairo_surface_destroy (data->surface);
	}

#if OS(AROS)
    if(data->bgra)
    {
        ARGB2BGRAFREE(data->bgra);
    }
#endif

	return DOSUPER;
}

DEFMMETHOD(AskMinMax)
{
	GETDATA;

	DOSUPER;

	msg->MinMaxInfo->MinWidth  += data->imagewidth;
	msg->MinMaxInfo->MinHeight += data->imageheight;
	msg->MinMaxInfo->MaxWidth  += MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
	msg->MinMaxInfo->DefWidth  += data->imagewidth;
	msg->MinMaxInfo->DefHeight += data->imageheight;

	return 0;
}

DEFMMETHOD(Show)
{
	BOOL rc = DOSUPER;

	if (rc)
	{
		GETDATA;

		data->is_shown = 1;

		if (data->animate && !data->added)
		{
			DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->ihnode);
			data->added = TRUE;
		}
	}

	return rc;
}

DEFMMETHOD(Hide)
{
	GETDATA;

	data->is_shown = 0;

	if (data->animate && data->added)
	{
		DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->ihnode);
		data->added = FALSE;
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(data, obj, msg->ops_AttrList);
	return DOSUPER;
}

DEFTMETHOD(TransferAnim_Run)
{
	GETDATA;

	data->imagenum++;

	if (data->imagenum >= data->imagecount)
		data->imagenum = 0;

	MUI_Redraw(obj, MADF_DRAWUPDATE);
	return 0;
}

DEFMMETHOD(Draw)
{
	DOSUPER;

	if (msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))
	{
		GETDATA;
		ULONG mleft, mtop, mwidth, mheight;
		ULONG stride = 0;
		char *src = NULL;

		mleft   = _mleft(obj);
		mtop    = _mtop(obj);
		mwidth  = _mwidth(obj);
		mheight = _mheight(obj);

		DoMethod(obj, MUIM_DrawBackground, mleft, mtop, mwidth, mheight, 0, 0, 0);

		if(data->surface)
		{
#if OS(AROS)
            src = (char *) data->bgra;
#else
            src = (char *) cairo_image_surface_get_data(data->surface);
#endif
			stride = cairo_image_surface_get_stride(data->surface);
		}

		// Fall back to builtin animation
		if(!src || !stride)
		{
			src = (char *) &Throbber;
			stride = THROBBER_WIDTH * sizeof(ULONG);
		}

		WritePixelArrayAlpha((APTR) src, data->imagenum * data->imagewidth, 0, stride, _rp(obj), mleft + (mwidth - data->imagewidth) / 2, mtop + (mheight - data->imageheight) / 2, data->imagewidth, data->imageheight, 0xffffffff); //data->animate ? 0xffffffff : 0x4fffffff);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECMMETHOD(Hide)
DECMMETHOD(Show)
DECTMETHOD(TransferAnim_Run)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, transferanimclass)
