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
#include <Api/WebFrame.h>
#include <Api/WebView.h>

#include <clib/macros.h>
#include <proto/dos.h>
#include <clib/debug_protos.h>

#include "gui.h"

#define D(x)

/******************************************************************
 * titlelabelclass
 *****************************************************************/

enum {
	POPMENU_NEW = 1,
	POPMENU_RELOAD,
	POPMENU_RELOAD_ALL,
	POPMENU_CLOSE,
	POPMENU_CLOSE_ALL_OTHERS
};

struct Data
{
	Object *closebutton;
	Object *label;
	Object *transferanim;
	Object *favicon;
	Object *cmenu;
	Object *pagegroup;
};

STATIC VOID doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MUIA_Text_Contents:
				set(data->label, MUIA_Text_Contents, tag->ti_Data);
				break;

			case MA_TransferAnim_Animate:
				set(data->pagegroup, MUIA_Group_ActivePage, tag->ti_Data != 0 ? 1 : 0);
				set(data->transferanim, MA_TransferAnim_Animate, tag->ti_Data);
				break;

			case MA_FavIcon_PageURL:
				set(data->pagegroup, MUIA_Group_ActivePage, 0);
				set(data->favicon, MA_FavIcon_PageURL, tag->ti_Data);
				break;
		}
	}
}

DEFNEW
{
	Object *closebutton;
	Object *label;
	Object *transferanim;
	Object *favicon;
	Object *pagegroup;

	obj = (Object *) DoSuperNew(cl, obj,
        MUIA_Group_Horiz, TRUE,
		MUIA_ContextMenu, TRUE,
		MUIA_Draggable,   TRUE,
		MUIA_Dropable,    TRUE,

		Child, closebutton = ImageObject,
                        MUIA_Frame, MUIV_Frame_ImageButton,
						MUIA_CustomBackfill, TRUE,
						MUIA_InputMode, MUIV_InputMode_RelVerify,
						MUIA_Image_Spec, MUII_Close,
						End,
		Child, label = TextObject,
						MUIA_Text_SetMax, FALSE,
						MUIA_Text_SetMin, FALSE,
						MUIA_Text_PreParse, "\033-",
						MUIA_Text_Shorten, MUIV_Text_Shorten_Cutoff,
						MUIA_Text_Contents, DEFAULT_PAGE_NAME,
						End,
		Child, pagegroup = HGroup,
			MUIA_Group_PageMode, TRUE,
			Child, VGroup, Child, favicon = (getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Tab) ? (Object *) NewObject(getfaviconclass(), NULL, TAG_DONE) : RectangleObject, End, End,
			Child, VGroup, Child, transferanim = (Object *) NewObject(gettabtransferanimclass(), NULL, TAG_DONE), End,
			End,

		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;
		data->cmenu = NULL;
		data->closebutton = closebutton;
		data->label = label;
		data->transferanim = transferanim;
		data->favicon = favicon;
		data->pagegroup = pagegroup;

		DoMethod(data->closebutton, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_Title_Close);

		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Tab)
		{
			DoMethod(favicon, MUIM_Notify, MA_FavIcon_NeedRedraw, MUIV_EveryTime, obj, 1, MM_Title_Redraw);
		}
	}
	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;
	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
	}
	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_OWB_ObjectType:
		{
			*msg->opg_Storage = MV_OWB_ObjectType_Tab;
		}
		return TRUE;

		case MA_OWB_URL:
		{
			Object *browser = (Object *) muiUserData(obj);
			*msg->opg_Storage = (ULONG) getv(browser, MA_OWBBrowser_URL);
		}
		return TRUE;

		case MA_OWB_Title:
		{
			Object *browser = (Object *) muiUserData(obj);
			*msg->opg_Storage = (ULONG) getv(browser, MA_OWBBrowser_Title);
		}
		return TRUE;

		case MA_OWB_Browser:
		{
			Object *browser = (Object *) muiUserData(obj);
			*msg->opg_Storage = (ULONG) browser;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MinWidth += 32;

	return 0;
}

DEFMMETHOD(Title_Close)
{
	Object *browser = (Object *) getv(obj, MUIA_UserData);

	if(browser)
	{
		DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveBrowser, browser);
	}

	return 0;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	data->cmenu = MenustripObject,
			MUIA_Family_Child, MenuObjectT(GSI(MSG_TITLELABEL_BROWSER)),
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_TITLELABEL_NEW_TAB),
				MUIA_UserData, POPMENU_NEW,
                End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, NM_BARLABEL,
				MUIA_UserData, NULL,
                End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_TITLELABEL_RELOAD),
				MUIA_UserData, POPMENU_RELOAD,
                End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_TITLELABEL_RELOAD_ALL_TABS),
				MUIA_UserData, POPMENU_RELOAD_ALL,
                End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, NM_BARLABEL,
				MUIA_UserData, NULL,
                End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_TITLELABEL_CLOSE),
				MUIA_UserData, POPMENU_CLOSE,
				End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_TITLELABEL_CLOSE_ALL_OTHER_TABS),
				MUIA_UserData, POPMENU_CLOSE_ALL_OTHERS,
				End,
            End,
        End;

	return (ULONG)data->cmenu;
}

DEFMMETHOD(ContextMenuChoice)
{
	Object *browser = (Object *) muiUserData(obj);
	ULONG udata = muiUserData(msg->item);

	switch(udata)
	{
		case POPMENU_NEW:
		{
			DoMethod(_win(obj), MM_OWBWindow_MenuAction, MNA_NEW_PAGE);
		}
		break;

		case POPMENU_RELOAD:
		{
			DoMethod(_win(obj), MM_OWBWindow_Reload, browser);
		}
		break;

		case POPMENU_RELOAD_ALL:
		{
			APTR n;

			ITERATELIST(n, &view_list)
			{
				struct viewnode *vn = (struct viewnode *) n;

				if(vn->window == _win(obj))
				{
					DoMethod(_win(obj), MM_OWBWindow_Reload, vn->browser);
				}
			}
		}
		break;

		case POPMENU_CLOSE:
		{
            DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveBrowser, browser);
		}
		break;

		case POPMENU_CLOSE_ALL_OTHERS:
		{
			APTR n;
			ITERATELIST(n, &view_list)
			{
				struct viewnode *vn = (struct viewnode *) n;

				if(vn->browser != browser && vn->window == _win(obj))
				{
					DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveBrowser, vn->browser);
				}
			}
		}
		break;
	}

	return 0;
}

DEFMMETHOD(DragQuery)
{
	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if (obj != msg->obj &&
	(type == MV_OWB_ObjectType_QuickLink || type == MV_OWB_ObjectType_Browser
	 || type == MV_OWB_ObjectType_Bookmark || type == MV_OWB_ObjectType_Tab || type == MV_OWB_ObjectType_URL))
	{
		// Sortable tabs handling
		if(type == MV_OWB_ObjectType_Tab)
		{
			ULONG value = 0;

			if(GetAttr(MUIA_Title_Sortable, _parent(msg->obj), (ULONGPTR) &value))
			{
				if(value)
				{
					return (MUIV_DragQuery_Refuse);
				}
			}
		}
		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	STRPTR url = (STRPTR) getv(msg->obj, MA_OWB_URL);

	if(url)
	{
		Object *browser = (Object *) muiUserData(obj);

		if(browser)
		{
			DoMethod(_win(obj), MM_OWBWindow_LoadURL, url, browser);
		}
	}

	return 0;
}

DEFTMETHOD(Title_Redraw)
{
	GETDATA;

	if(muiRenderInfo(obj))
	{
		D(kprintf("Title_Redraw <%s>\n", (STRPTR) getv(browser, MA_OWBBrowser_URL)));

		MUI_Redraw(obj, MADF_DRAWOBJECT);

		if(data->favicon)
		{
			MUI_Redraw(data->favicon, MADF_DRAWOBJECT);
		}
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECMMETHOD(Title_Close)
DECMMETHOD(AskMinMax)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECTMETHOD(Title_Redraw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, titlelabelclass)

