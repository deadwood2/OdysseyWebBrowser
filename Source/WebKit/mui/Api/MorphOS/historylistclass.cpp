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
#include "WebHistory.h"
#include "WebHistoryItem.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <devices/rawkeycodes.h>
#include <proto/intuition.h>
#include <clib/macros.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

enum
{
	POPMENU_OPEN_URL = 1,
	POPMENU_OPEN_URL_IN_NEW_TAB,
	POPMENU_OPEN_URL_IN_NEW_WINDOW
};

struct Data
{
	Object *str;
	Object *pop;
	Object *cmenu;

	ULONG complete;
	ULONG opened;
	ULONG added;
	struct MUI_EventHandlerNode ehnode;
};

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MUIA_Popstring_String:
			data->str = (Object *) tag->ti_Data;
			break;
		case MUIA_Popobject_Object:
			data->pop = (Object *) tag->ti_Data;
			break;
		case MA_HistoryList_Complete:
			data->complete = tag->ti_Data;
			break;
	}
	NEXTTAG
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_MinLineHeight, 36,
		MUIA_List_Format, (getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_History) ? (ULONG) "," : (ULONG) "",
		MUIA_List_Title, FALSE,
		MUIA_ContextMenu, TRUE,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;
		data->cmenu = NULL;

		data->added = FALSE;

		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS;
		data->ehnode.ehn_Priority = 1;
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

		doset(obj, data, msg->ops_AttrList);
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

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_HistoryList_Opened:
			*msg->opg_Storage = (ULONG) data->opened;
            return TRUE;
		case MA_HistoryList_Complete:
			*msg->opg_Storage = (ULONG) data->complete;
            return TRUE;
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

DEFMMETHOD(Show)
{
	ULONG rc;
	GETDATA;

	data->opened = TRUE;

	// Update list format here, quite convenient (but it doesn't quite work :))
	//set(obj, MUIA_List_Format, (getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_History) ? "," : "");


	if ((rc = DOSUPER))
	{
		if(!data->added)
		{
			DoMethod( _win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
			data->added = TRUE;
		}
	}

	return rc;
}

DEFMMETHOD(Hide)
{
	GETDATA;
	ULONG i;

	data->opened = FALSE;

	if(data->added)
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode );
		data->added = FALSE;
	}

	/* Delete Favicon Images if needed */
	for (i = 0; ; i++)
	{
		struct history_entry *x;
		DoMethod(obj, MUIM_List_GetEntry, i, &x);

		if(x)
		{
			if(x->faviconimg)
			{
				DoMethod(obj, MUIM_List_DeleteImage, x->faviconimg);
				x->faviconimg = NULL;
			}
		}
		else
		{
			break;
		}
	}

	return DOSUPER;
}

DEFMMETHOD(HandleEvent)
{
	GETDATA;
	struct IntuiMessage *imsg;

	if((imsg = msg->imsg))
	{
		if(imsg->Class == IDCMP_RAWKEY)
		{
			switch(imsg->Code & ~IECODE_UP_PREFIX)
			{
				case RAWKEY_UP:
				case RAWKEY_DOWN:
				case RAWKEY_NM_WHEEL_UP:
				case RAWKEY_NM_WHEEL_DOWN:
					//return MUI_EventHandlerRC_Eat;
					break;
				default:
					set(_win(data->str), MUIA_Window_Activate, TRUE);
					set(_win(data->str), MUIA_Window_ActiveObject, (Object *) data->str);
					break;
			}
		}
		else if(imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			switch (imsg->Code & ~IECODE_UP_PREFIX)
			{
				case IECODE_MBUTTON:
				{
					if (!(imsg->Code & IECODE_UP_PREFIX))
					{
						struct MUI_List_TestPos_Result res;
						struct history_entry *item = NULL;

						if (_isinobject(obj, imsg->MouseX, imsg->MouseY) && DoMethod(obj, MUIM_List_TestPos, imsg->MouseX, imsg->MouseY, &res) && (res.entry != -1))
						{
							DoMethod(obj, MUIM_List_GetEntry, res.entry, (ULONG *)&item);

							if(item)
							{
	                            WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;
								char *wurl = (char *) witem->URLString();
								nnset(obj, MUIA_List_Active, res.entry);
								DoMethod(data->pop, MUIM_Popstring_Close, FALSE);
								DoMethod(app, MM_OWBApp_AddBrowser, NULL, wurl, FALSE, NULL, FALSE, FALSE, TRUE);
								free(wurl);
							}
						}
					}
             	    break;
				}

				default: ;
			}
		}
	}

	return 0;

}

DEFMMETHOD(List_Construct)
{
	WebHistoryItem *item = (WebHistoryItem *) msg->entry;
	struct history_entry *hitem = (struct history_entry *) malloc(sizeof(*hitem));

	if(hitem)
	{
		hitem->webhistoryitem = item;
		hitem->faviconobj = NULL;
		hitem->faviconimg = NULL;
	}
	return (ULONG)hitem;
}

DEFMMETHOD(List_Destruct)
{
	struct history_entry *entry = (struct history_entry *) msg->entry;

	if(entry->faviconimg && muiRenderInfo(obj))
	{
		DoMethod(obj, MUIM_List_DeleteImage, entry->faviconimg);
		entry->faviconimg = NULL;
	}

	if(entry->faviconobj)
	{
		MUI_DisposeObject((Object *) entry->faviconobj);
		entry->faviconobj = NULL;
	}

	free(entry);

	return TRUE;
}

DEFMMETHOD(List_Display)
{
#define LF "\n\33P[80------]"
#define MAX_URL_WIDTH   2048
#define MAX_LABEL_WIDTH 128
	struct history_entry *item = (struct history_entry *) msg->entry;

	if(item)
	{
		static char slabel[MAX_LABEL_WIDTH + MAX_URL_WIDTH + 64];
		static char stitle[MAX_LABEL_WIDTH + 1];
		static char surl[MAX_URL_WIDTH + 1];
		WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;
		char *url = (char *) witem->URLString();
		char *title = (char *) witem->title();

		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_History)
		{
			if(!item->faviconobj)
			{
				item->faviconobj = NewObject(getfaviconclass(), NULL,
											 MA_FavIcon_PageURL, url,
											 MA_FavIcon_ScaleFactor, MV_FavIcon_Scale_Normal,
											 MUIA_Weight, 0,
											 TAG_DONE);
				
				DoMethod((Object *) item->faviconobj, MUIM_Notify, MA_FavIcon_NeedRedraw, MUIV_EveryTime, obj, 2, MM_HistoryList_Redraw, item);
			}

			if(!item->faviconimg)
			{
				item->faviconimg = (APTR) DoMethod(obj, MUIM_List_CreateImage, item->faviconobj, 0);
			}
		}

		if(getv(obj, MA_HistoryList_Complete) && url)
		{
			String fragment = witem->fragment();
			String decoratedURL = url;
			decoratedURL.replace(fragment, "\033b\033u\33P[ff------]" + fragment + "\033n\33P[80------]");
			stccpy(surl, decoratedURL.latin1().data(), sizeof(surl));
		}
		else
		{
			stccpy(surl, url ? url : GSI(MSG_HISTORYLIST_NO_URL), sizeof(surl));
		}

		if(title && title[0])
		{
			char *converted_title = utf8_to_local(title);
			if(converted_title)
			{
				stccpy(stitle, converted_title, sizeof(stitle));
				free(converted_title);
			}
		}
		else
		{
			stccpy(stitle, GSI(MSG_HISTORYLIST_NO_TITLE), sizeof(stitle));
		}

		if(strlen(surl) == MAX_URL_WIDTH)
		{
			surl[MAX_URL_WIDTH - 1] = '.';
			surl[MAX_URL_WIDTH - 2] = '.';
			surl[MAX_URL_WIDTH - 3] = '.';
		}

		if(strlen(stitle) == MAX_LABEL_WIDTH)
		{
			stitle[MAX_LABEL_WIDTH - 1] = '.';
			stitle[MAX_LABEL_WIDTH - 2] = '.';
			stitle[MAX_LABEL_WIDTH - 3] = '.';
		}

		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_History)
		{
			static char sicon[128];
			snprintf(sicon, sizeof(sicon), "\033O[%08lx]", (unsigned long) item->faviconimg);
			snprintf(slabel, sizeof(slabel), " %s%s %s", stitle, LF, surl);

			msg->array[0] = (char *) sicon;
			msg->array[1] = (char *) slabel;
		}
		else
		{
			snprintf(slabel, sizeof(slabel), "%s%s%s", stitle, LF, surl);
			msg->array[0] = (char *) slabel;
		}

		if( (ULONG)msg->array[-1] % 2 )
		{
#if !OS(AROS)
			/* This code overrides internal data structures and causes a crash on AROS */
			msg->array[-9] = (STRPTR) 10;
#endif
		}

		free(url);
		free(title);
	}

	return TRUE;
}

DEFSMETHOD(HistoryList_Redraw)
{
	if(muiRenderInfo(obj))
	{
		DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_Entry, msg->entry);
	}

	return 0;
}

DEFTMETHOD(HistoryList_SelectChange)
{
	GETDATA;
	struct history_entry *item = NULL;

	if(muiRenderInfo(obj))
	{
		DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &item);
		if (item)
		{
			WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;
			char *wurl = (char *) witem->URLString();
			//ULONG len = strlen(wurl);
			nnset(data->str, MUIA_String_Contents, wurl);
			free(wurl);
			/*
			nnset(data->str, MUIA_Textinput_MarkStart, 0);
			nnset(data->str, MUIA_Textinput_MarkEnd, len - 1);
			*/
		}
	}
	return 0;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUI_List_TestPos_Result res;
	struct history_entry *item = NULL;

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	if (DoMethod(obj, MUIM_List_TestPos, msg->mx, msg->my, &res) && (res.entry != -1))
	{
		DoMethod(obj, MUIM_List_GetEntry, res.entry, (ULONG *)&item);

		if(item)
		{
			data->cmenu = MenustripObject,
					MUIA_Family_Child, MenuObjectT(GSI(MSG_HISTORYLIST_HISTORY)),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLIST_OPEN_URL),
						MUIA_UserData, POPMENU_OPEN_URL,
	                    End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLIST_OPEN_NEW_TAB),
						MUIA_UserData, POPMENU_OPEN_URL_IN_NEW_TAB,
						End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLIST_OPEN_NEW_WINDOW),
						MUIA_UserData, POPMENU_OPEN_URL_IN_NEW_WINDOW,
						End,
	                End,
	            End;
		}
	}
	return (ULONG)data->cmenu;
}

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;
	struct history_entry *item = NULL;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&item);

	if(item)
	{
		WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;
		ULONG udata = muiUserData(msg->item);

		switch(udata)
		{
			case POPMENU_OPEN_URL:
			{
                Object *window = (Object *) getv(app, MA_OWBApp_ActiveWindow);
				char *wurl = (char *) witem->URLString();
				DoMethod(data->pop, MUIM_Popstring_Close, FALSE);
				DoMethod(window, MM_OWBWindow_LoadURL, wurl, NULL);
				free(wurl);
			}
			break;

			case POPMENU_OPEN_URL_IN_NEW_TAB:
			{
				char *wurl = (char *) witem->URLString();
				DoMethod(data->pop, MUIM_Popstring_Close, FALSE);
				DoMethod(app, MM_OWBApp_AddBrowser, NULL, wurl, FALSE, NULL, FALSE, FALSE, TRUE);
				free(wurl);
			}
			break;

			case POPMENU_OPEN_URL_IN_NEW_WINDOW:
			{
				char *wurl = (char *) witem->URLString();
				DoMethod(data->pop, MUIM_Popstring_Close, FALSE);
				DoMethod(app, MM_OWBApp_AddWindow, wurl, FALSE, NULL, FALSE, NULL, FALSE);
				free(wurl);
			}
			break;
		}
	}
	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECTMETHOD(HistoryList_SelectChange)
DECSMETHOD(HistoryList_Redraw)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(HandleEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, historylistclass)
