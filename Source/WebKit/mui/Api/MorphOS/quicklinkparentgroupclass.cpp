/*
 * Copyright 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 * Copyright 2009 Polymere
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

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "gui.h"
#include "bookmarkgroupclass.h"

#ifndef get
#define get(obj,attr,store) GetAttr(attr,obj,(ULONGPTR)store)
#endif

//#define D(x)

/* private */

struct Data {
	ULONG hidden;
	Object *qlgroup;
};

extern Object *app;

#define D(x)
#define LOC(a,b) (b)

static ULONG LayoutFonc( struct Hook *hook, Object *obj, struct MUI_LayoutMsg *lm )
{
	//struct Data *data=NULL;
	
	switch (lm->lm_Type)
	{
		case MUILM_MINMAX:
		{
			ULONG min_w=0,min_h=0,mode=0,max_w=0,max_h=0;
			Object *cstate = (Object *)lm->lm_Children->mlh_Head;
			Object *child;
			if ( (child=(Object *)NextObject(&cstate)) )
			{
				if (get(child, MA_QuickLinkGroup_Mode, &mode))
				{
					get(child, MA_QuickLinkGroup_Mode, &mode);
					get(child, MA_QuickLinkGroup_MinW, &min_w);
					get(child, MA_QuickLinkGroup_MinH, &min_h);
					get(child, MA_QuickLinkGroup_MaxW, &max_w);
					get(child, MA_QuickLinkGroup_MaxH, &max_h);
					D(kprintf("QL Parent Group: mode=%ld buts=%ld min_w=%ld min_h=%ld\n",mode,buts,min_w,min_h));
					if (mode & MV_QuickLinkGroup_Mode_Vert)
					{
						lm->lm_MinMax.MinWidth  = max_w;
						lm->lm_MinMax.MinHeight = min_h;
						lm->lm_MinMax.DefWidth  = max_w;
#if OS(AROS)
						lm->lm_MinMax.DefHeight = min_h;
#else
						lm->lm_MinMax.DefHeight = max_h;
#endif
						lm->lm_MinMax.MaxWidth  = max_w;
						lm->lm_MinMax.MaxHeight = max_h;
					}
					else
					{
						lm->lm_MinMax.MinWidth  = min_w;
						lm->lm_MinMax.MinHeight = max_h;
#if OS(AROS)
						lm->lm_MinMax.DefWidth  = min_w;
#else
						lm->lm_MinMax.DefWidth  = max_w;
#endif
						lm->lm_MinMax.DefHeight = max_h;
						lm->lm_MinMax.MaxWidth  = max_w;
						lm->lm_MinMax.MaxHeight = max_h;
					}
				}
				else
				{
					D(kprintf("QL Parent Group: HIDED\n"));
					if (_maxheight(child)==1)
					{
						// HSpace()
						lm->lm_MinMax.MinWidth  = 1;
						lm->lm_MinMax.MinHeight = 1;
						lm->lm_MinMax.DefWidth  = MUI_MAXMAX;
						lm->lm_MinMax.DefHeight = 1;
						lm->lm_MinMax.MaxWidth  = MUI_MAXMAX;
						lm->lm_MinMax.MaxHeight = 1;
					}
					else
					{
						// VSpace()
						lm->lm_MinMax.MinWidth  = 1;
						lm->lm_MinMax.MinHeight = 1;
						lm->lm_MinMax.DefWidth  = 1;
						lm->lm_MinMax.DefHeight = MUI_MAXMAX;
						lm->lm_MinMax.MaxWidth  = 1;
						lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
					}
				}
			}
			return(0);
		}

		case MUILM_LAYOUT:
		{
			APTR cstate = (APTR)lm->lm_Children->mlh_Head;
			Object *child;
			ULONG w,h,mode;
			if ( (child=(Object *)NextObject(&cstate)) )
			{
				if (get(child, MA_QuickLinkGroup_Mode, &mode))
				{
					w=_maxwidth(child);
					h=_maxheight(child);
					if (w>(ULONG)lm->lm_Layout.Width) w=lm->lm_Layout.Width;
					if (h>(ULONG)lm->lm_Layout.Height) h=lm->lm_Layout.Height;
					D(kprintf("QL Parent Group: w=%ld h=%ld\n",w,h));
					if (!MUI_Layout(child,0,0,w,h,0))
					{
						return(FALSE);
					}
				}
				else
				{
					// Space
					if (_maxheight(obj)==1)
					{
						// Layout HSpace()
						if (!MUI_Layout(child,0,0,lm->lm_Layout.Width,1,0))
						{
							return(FALSE);
						}
					}
					else
					{
						// Layout VSpace()
						if (!MUI_Layout(child,0,0,1,lm->lm_Layout.Height,0))
						{
							return(FALSE);
						}
					}
				}
			}
			return (TRUE);
		}
	}
	return((ULONG)MUILM_UNKNOWN);
}

static struct Hook LayoutFonc_hook = {
    {NULL, NULL},
	(APTR) HookEntry,
	(APTR) LayoutFonc,
	NULL
};

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_QuickLinkParentGroup_QLGroup:      //Init only
			if (!data->qlgroup)
			{
				ULONG mode=0;
				Object *child;
				data->qlgroup=(Object *)tag->ti_Data;
				get(data->qlgroup, MA_QuickLinkGroup_Mode, &mode);
				if (mode & MV_QuickLinkGroup_Mode_Vert)
				{
					// Vertical mode add 1 pixel band
					child=(Object *)HSpace(1);
					if(child) DoMethod(obj, MUIM_Group_AddTail, child);
				}
				else
				{
					// Horiz mode add 1 pixel band
					child=(Object *)VSpace(1);
					if(child) DoMethod(obj, MUIM_Group_AddTail, child);
				}
				data->hidden=TRUE;
			}
			break;
		case MA_QuickLinkParentGroup_Hide:
			if (data->hidden!=tag->ti_Data)
			{
				if (data->hidden)
				{
					// Hidden
					// Remove space then add group
					Object *button;
					struct MinList *l;
					APTR cstate;
					D(kprintf("QuickLinkParentGroup_Hide FALSE\n"));
					get(obj, MUIA_Group_ChildList, &l);
					cstate=l->mlh_Head;
					button=(Object *)NextObject(&cstate);
					//DoMethod(obj, MUIM_Group_InitChange);
					if (button)
					{
						D(kprintf("QuickLinkParentGroup_Hide Remove space\n"));
						DoMethod(obj, OM_REMMEMBER, button);
						DoMethod(button, OM_DISPOSE);
					}
					D(kprintf("QuickLinkParentGroup_Hide Add QLGroup %08lx\n", data->qlgroup));
					DoMethod(obj, OM_ADDMEMBER, data->qlgroup);
					//DoMethod(obj, MUIM_Group_ExitChange);
					data->hidden=FALSE;
				}
				else
				{
					// Group showed, remove it and add little space
					ULONG mode=0;
					APTR child;
					struct MinList *l;
					APTR cstate;

					D(kprintf("QuickLinkParentGroup_Hide TRUE\n"));
					get(obj, MUIA_Group_ChildList, &l);
					cstate=l->mlh_Head;
					data->qlgroup=(Object *)NextObject(&cstate);
					//DoMethod(obj, MUIM_Group_InitChange);
					D(kprintf("QuickLinkParentGroup_Hide Remove QLGroup %08lx\n", data->qlgroup));
					if (data->qlgroup)
					{
						DoMethod(obj, OM_REMMEMBER, data->qlgroup);
					}

					get(data->qlgroup, MA_QuickLinkGroup_Mode, &mode);
					if (mode & MV_QuickLinkGroup_Mode_Vert)
					{
						// Vertical mode add 1 pixel band
						D(kprintf("QuickLinkParentGroup_Hide Add HSpace\n"));
						child=(Object *)HSpace(1);
						if(child) DoMethod(obj, MUIM_Group_AddTail, child);
					}
					else
					{
						// Horiz mode add 1 pixel band
						D(kprintf("QuickLinkParentGroup_Hide Add VSpace\n"));
						child=(Object *)VSpace(1);
						if(child) DoMethod(obj, MUIM_Group_AddTail, child);
					}
					//DoMethod(obj, MUIM_Group_ExitChange);
					data->hidden=TRUE;
				}
			}
			break;
	}
	NEXTTAG
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_LayoutHook, &LayoutFonc_hook,
		//MUIA_Weight, 800,
		TAG_MORE, INITTAGS,
		End;
	if (obj)
	{
		GETDATA;
		data->qlgroup=NULL;
		data->hidden=FALSE;
		doset(obj, data, msg->ops_AttrList);
	}
	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;
	D(kprintf("QuickLinkParentGroup: disposing\n"));
	if (data->hidden)
	{
		D(kprintf("Hidden mode dispose QLGroup"));
		DoMethod(data->qlgroup, OM_DISPOSE);
	}
	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

DEFMMETHOD(DragQuery)
{
	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if (type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_Tab || type == MV_OWB_ObjectType_URL)
	{
		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	STRPTR url = strdup((STRPTR) getv(msg->obj, MA_OWB_URL));
	STRPTR title = NULL;
	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if(type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_Tab)
	{
		char *tmp = (STRPTR) getv(msg->obj, MA_OWB_Title);
		if(tmp)
		{
			title = (STRPTR) strdup(tmp);
		}
	}

	if(url)
	{
		DoMethod(app, MUIM_Application_PushMethod, _win(obj), 4, MM_OWBWindow_InsertBookmarkAskTitle, url, title, TRUE);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, quicklinkparentgroupclass)
