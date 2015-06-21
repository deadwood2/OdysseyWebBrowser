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
	Object *cmenu;
	ULONG added;
	struct MUI_EventHandlerNode ehnode;
};

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MUIA_List_DoubleClick:
		{
			struct history_entry *item = NULL;
			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &item);
			if (item)
			{
				WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;
				if(witem)
				{
					char *wurl = (char *) witem->URLString();
					DoMethod(_win(obj), MM_OWBWindow_LoadURL, wurl, NULL);
					free(wurl);
				}
			}
		}
		break;
	}
	NEXTTAG
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Format, "MIW=-1 MAW=-2, H",
		MUIA_List_MinLineHeight, (getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_HistoryPanel) ? 18 : TAG_IGNORE,
		MUIA_List_Title,  TRUE,
		MUIA_ContextMenu, TRUE,
		MUIA_CycleChain, TRUE,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;
		data->cmenu = NULL;

		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_MOUSEBUTTONS;
		data->ehnode.ehn_Priority = 1;
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

		doset(obj, data, msg->ops_AttrList);

		//DoMethod(app, MUIM_Notify, MA_OWBApp_HistoryChanged, MUIV_EveryTime, obj, 1, MM_History_Update);
		//DoMethod(app, MUIM_Notify, MA_OWBApp_DidReceiveFavIcon, MUIV_EveryTime, obj, 1, MM_History_Update);
	}
	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	//DoMethod(app, MUIM_KillNotifyObj, MA_OWBApp_DidReceiveFavIcon, obj);

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
	}
	return DOSUPER;
}

DEFMMETHOD(Show)
{
	ULONG rc;
	GETDATA;

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

	if(data->added)
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode );
		data->added = FALSE;
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

DEFMMETHOD(List_Construct)
{
	struct history_entry *entry = (struct history_entry *) msg->entry;

	if(entry && entry->faviconobj && muiRenderInfo(obj))
	{
		entry->faviconimg = (APTR) DoMethod(obj, MUIM_List_CreateImage, entry->faviconobj, 0);
	}

	return (ULONG)msg->entry;
}

DEFMMETHOD(List_Destruct)
{
	struct history_entry *entry = (struct history_entry *) msg->entry;

	if(entry && entry->faviconobj && muiRenderInfo(obj))
	{
		DoMethod(obj, MUIM_List_DeleteImage, entry->faviconimg);
		entry->faviconimg = NULL;
	}
	return TRUE;
}

DEFMMETHOD(List_Display)
{
#define MAX_URL_WIDTH   2048
#define MAX_LABEL_WIDTH 64
	struct history_entry *item = (struct history_entry *) msg->entry;

	if(item)
	{
		static char stitle[MAX_LABEL_WIDTH + 1];
		static char surl[MAX_URL_WIDTH + 1];
		WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;
		char *title = (char *) witem->title();
		char *url   = (char *) witem->URLString();

		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_HistoryPanel)
		{
			if(!item->faviconobj)
			{
				item->faviconobj = NewObject(getfaviconclass(), NULL,
											 MA_FavIcon_PageURL, url,
											 MUIA_Weight, 0,
											 TAG_DONE);

				if(item->faviconobj)
				{
					item->faviconimg = (APTR) DoMethod(obj, MUIM_List_CreateImage, item->faviconobj, 0);
					DoMethod((Object *) item->faviconobj, MUIM_Notify, MA_FavIcon_NeedRedraw, MUIV_EveryTime, obj, 3, MUIM_List_Redraw, MUIV_List_Redraw_Entry, item);
				}
			}
		}

		if(title && title[0])
		{
			char *converted_title = utf8_to_local(title);
			if(converted_title)
			{
				if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_HistoryPanel)
				{
					snprintf(stitle, sizeof(stitle), "\033O[%08lx] %s", (unsigned long) item->faviconimg, converted_title);
                    stitle[MAX_LABEL_WIDTH] = '\0';
				}
				else
				{
					stccpy(stitle, converted_title, sizeof(stitle));
				}
				free(converted_title);
			}
		}
		else
		{
			if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_HistoryPanel)
			{
				snprintf(stitle, sizeof(stitle), "\033O[%08lx] %s", (unsigned long) item->faviconimg, url ? url : "no title");
                stitle[MAX_LABEL_WIDTH] = '\0';
			}
			else
			{
				stccpy(stitle, url ? url : GSI(MSG_HISTORYLISTTREE_NO_TITLE), sizeof(stitle));
			}
		}

		stccpy(surl, url ? url : GSI(MSG_HISTORYLISTTREE_NO_URL), sizeof(surl));

		if(strlen(stitle) == MAX_LABEL_WIDTH)
		{
			stitle[MAX_LABEL_WIDTH - 1] = '.';
			stitle[MAX_LABEL_WIDTH - 2] = '.';
			stitle[MAX_LABEL_WIDTH - 3] = '.';
		}

		if(strlen(surl) == MAX_URL_WIDTH)
		{
			surl[MAX_URL_WIDTH - 1] = '.';
			surl[MAX_URL_WIDTH - 2] = '.';
			surl[MAX_URL_WIDTH - 3] = '.';
		}

		msg->array[0] = (char *) stitle;
		msg->array[1] = (char *) surl;

		if( (ULONG)msg->array[-1] % 2 )
		{
#if !OS(AROS)
			/* This code overrides internal data structures and causes a crash on AROS */
			msg->array[-9] = (STRPTR) 10;
#endif
		}

		free(title);
		free(url);
	}
	else
	{
		msg->array[0] = GSI(MSG_HISTORYLISTTREE_TITLE);
		msg->array[1] = GSI(MSG_HISTORYLISTTREE_ADDRESS);
	}

	return TRUE;
}

DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg;

	if((imsg = msg->imsg))
	{
		if(imsg->Class == IDCMP_MOUSEBUTTONS)
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
                                set(obj, MUIA_List_Active, res.entry);
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

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUI_List_TestPos_Result res;
	struct history_entry *item;

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
					MUIA_Family_Child, MenuObjectT(GSI(MSG_HISTORYLISTTREE_HISTORY)),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLISTTREE_OPEN_URL),
						MUIA_UserData, POPMENU_OPEN_URL,
	                    End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLISTTREE_OPEN_NEW_TAB),
						MUIA_UserData, POPMENU_OPEN_URL_IN_NEW_TAB,
						End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLISTTREE_OPEN_NEW_WINDOW),
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
	struct history_entry *item;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&item);

	if(item)
	{
		WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;
		ULONG udata = muiUserData(msg->item);

		switch(udata)
		{
			case POPMENU_OPEN_URL:
			{
				char *wurl = (char *) witem->URLString();
				DoMethod(_win(obj), MM_OWBWindow_LoadURL, wurl, NULL);
				free(wurl);
			}
			break;

			case POPMENU_OPEN_URL_IN_NEW_TAB:
			{
				char *wurl = (char *) witem->URLString();
				DoMethod(app, MM_OWBApp_AddBrowser, NULL, wurl, FALSE, NULL, FALSE, FALSE, TRUE);
				free(wurl);
			}
			break;

			case POPMENU_OPEN_URL_IN_NEW_WINDOW:
			{
				char *wurl = (char *) witem->URLString();
				DoMethod(app, MM_OWBApp_AddWindow, wurl, FALSE, NULL, FALSE, NULL, FALSE);
				free(wurl);
			}
			break;
		}		 
	}
	return 0;
}

DEFTMETHOD(History_Update)
{
	if(muiRenderInfo(obj))
	{
		DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
	}
	return 0;
}

DEFSMETHOD(Search)
{
	struct history_entry *item, *active;
	STRPTR pattern;
	ULONG found = FALSE;
	ULONG pos = 0;
	ULONG dosearch = FALSE;

	if(msg->string == NULL || *msg->string == 0)
	{
		return FALSE;
	}

	pattern = (STRPTR) malloc(strlen(msg->string) + 5);
	if(pattern)
	{
		sprintf(pattern, "#?%s#?", msg->string);

		DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &active);

restart:
		for(pos = 0; ; pos++)
		{
			DoMethod(obj, MUIM_List_GetEntry, pos, &item);

			if(item)
			{
				if(!active || item == active)
				{
					dosearch = TRUE;
				}

				if(dosearch && item != active)
				{
					WebHistoryItem *witem = (WebHistoryItem *) item->webhistoryitem;

					if(witem)
					{
						char *url = (char *) witem->URLString();
						char *title = (char *) witem->title();
						
						if((title && title[0] && name_match(title, pattern)) || (url && url[0] && name_match(url, pattern)))
						{
							found = TRUE;
						}
			
						free(url);
						free(title);
					}
				}
			}
			else
			{
				// Restart search from start, if we reached last entry
				if(active)
				{
					active = NULL;
					goto restart;
				}

				break;
			}

			if(found)
			{
				SetAttrs(obj, MUIA_List_Active, pos, TAG_DONE);
				break;
			}
		}

		free(pattern);
	}

	return found;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECSET
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(HandleEvent)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECTMETHOD(History_Update)
DECSMETHOD(Search)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, historylisttreeclass)
