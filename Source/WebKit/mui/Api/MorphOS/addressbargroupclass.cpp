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
#include <wtf/text/CString.h>
#include "WebHistory.h"
#include "WebHistoryItem.h"

#include <string.h>
#include <proto/intuition.h>
#include <clib/macros.h>

#include "gui.h"

using namespace WebCore;

struct Data
{
	Object *pop_path;
	Object *bt_go;
	Object *bt_add;
	Object *gr_pop;
	LONG prevlen;
};

static void loadhistory(struct Data *data)
{
	WebHistory* history = WebHistory::sharedHistory();
	std::vector<WebHistoryItem *> historyList = *(history->historyList());

	set((Object *) getv(data->pop_path, MUIA_Popobject_Object), MA_HistoryList_Complete, FALSE);
	set((Object *) getv(data->pop_path, MUIA_Popobject_Object), MUIA_List_Quiet, TRUE);

	DoMethod((Object *) getv(data->pop_path, MUIA_Popobject_Object), MUIM_List_Clear);

	for(unsigned int i = 0; i < historyList.size(); i++)
	{
		WebHistoryItem *webHistoryItem = historyList[i];

		if(webHistoryItem)
		{
			DoMethod(data->pop_path, MM_History_Insert, webHistoryItem);
		}
	}

	set((Object *) getv(data->pop_path, MUIA_Popobject_Object), MUIA_List_Quiet, FALSE);
}

static void	doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MUIA_String_Contents:
			{
				nnset(data->pop_path, MUIA_String_Contents, (char *) tag->ti_Data);
				break;
			}

			case MA_AddressBarGroup_Active:
			{
				if(tag->ti_Data)
				{
					set((Object *) _win(obj), MUIA_Window_ActiveObject, (Object *) getv(data->pop_path, MUIA_Popstring_String));
				}
				break;
			}

			case MA_AddressBarGroup_GoButton:
			{
				if(tag->ti_Data)
				{
					if(!data->bt_go)
					{
						data->bt_go = (Object *) NewObject(gettoolbuttonclass(), NULL,
																MA_ToolButton_Text, NULL,
																MA_ToolButton_Image, "PROGDIR:resource/go.png",
																MA_ToolButton_Type, MV_ToolButton_Type_Icon,
																MA_ToolButton_Frame, /*getv(app, MA_OWBApp_ShowButtonFrame)*/0 ? MV_ToolButton_Frame_Button : MV_ToolButton_Frame_None,
																MA_ToolButton_Background, /*getv(app, MA_OWBApp_ShowButtonFrame)*/0 ? MV_ToolButton_Background_Button :MV_ToolButton_Background_Parent,
																TAG_DONE);
						if(data->bt_go)
						{
							DoMethod((Object *) obj, MUIM_Group_InitChange);
							DoMethod((Object *) obj, MUIM_Group_Insert, data->bt_go, data->gr_pop);
							DoMethod((Object *) obj, MUIM_Group_ExitChange);

							DoMethod(data->bt_go, MUIM_Notify, MA_ToolButton_Pressed, FALSE, obj, 3, MM_OWBWindow_LoadURL, NULL, NULL);
						}
					}
				}
				else
				{
					if(data->bt_go)
					{
						DoMethod((Object *) obj, MUIM_Group_InitChange);
						DoMethod((Object *) obj, MUIM_Group_Remove, data->bt_go);
						DoMethod((Object *) obj, MUIM_Group_ExitChange);

						MUI_DisposeObject(data->bt_go);
						data->bt_go = NULL;
					}				 
				}
				break;
			}
		}
	}
}

DEFNEW
{
	Object *pop_path, *bt_bookmarks, *bt_add, *bt_addtab, *gr_pop;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		Child, bt_addtab = (Object *) NewObject(gettoolbutton_newtabclass(), NULL, End,
		Child, gr_pop = HGroup,
			MUIA_Draggable, TRUE,
			//Child, Label(GSI(MSG_ADDRESSBARGROUP_URL)),
			Child, pop_path = (Object *) NewObject(gethistorypopstringclass(), NULL, End,
			End,
		Child, MakeVBar(),
		Child, bt_add       = (Object *)  NewObject(gettoolbutton_addbookmarkclass(), NULL, End,
		Child, bt_bookmarks = (Object *)  NewObject(gettoolbutton_bookmarksclass(), NULL, End,
		TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->pop_path = pop_path;
		data->bt_go = NULL;
		data->bt_add = bt_add;
		data->gr_pop = gr_pop;
		data->prevlen = -1;

		loadhistory(data);

        doset(obj, data, INITTAGS);

        DoMethod((Object *) getv(pop_path, MUIA_Popstring_String),   MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 3, MM_OWBWindow_LoadURL, NULL, NULL);
		DoMethod(bt_bookmarks, MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Application, 2, MM_OWBApp_OpenWindow, MV_OWB_Window_Bookmarks);
		DoMethod(bt_add,       MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_InsertBookmark);
		DoMethod(bt_addtab,    MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 2, MM_OWBWindow_MenuAction, MNA_NEW_PAGE);

		DoMethod(app, MUIM_Notify, MA_OWBApp_BookmarksChanged, MUIV_EveryTime, obj, 2, MM_AddressBarGroup_Mark, NULL);

		DoMethod((Object *) getv(pop_path, MUIA_Popstring_String), MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 3, MM_OWBWindow_AutoComplete, 0, 0);
	}

	return (IPTR)obj;
}

DEFDISP
{
	DoMethod(app, MUIM_KillNotifyObj, MA_OWBApp_BookmarksChanged, obj);

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
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MUIA_String_Contents:
  		        return DoMethodA(data->pop_path, (_Msg_*) msg);

		case MA_AddressBarGroup_PopString:
			*msg->opg_Storage = (ULONG) data->pop_path;
			return TRUE;

		case MUIA_Popstring_String:
			*msg->opg_Storage = getv(data->pop_path, MUIA_Popstring_String);
			return TRUE;

		case MUIA_Popobject_Object:
			*msg->opg_Storage = getv(data->pop_path, MUIA_Popobject_Object);
			return TRUE;

		case MA_AddressBarGroup_GoButton:
			*msg->opg_Storage = data->bt_go != NULL;
			return TRUE;
	}

	return DOSUPER;
}

DEFMMETHOD(List_InsertSingle)
{
	GETDATA;
	return DoMethodA(data->pop_path, (_Msg_*) msg);
}

DEFSMETHOD(OWBWindow_LoadURL)
{
	if (muiRenderInfo(obj))
	{
		GETDATA;
		msg->url = (TEXT *)getv(data->pop_path, MUIA_String_Contents);
		DoMethodA(_win(obj), (_Msg_*) msg);
	}

	return 0;
}

DEFTMETHOD(AddressBarGroup_LoadHistory)
{
	GETDATA;
	loadhistory(data);
	return 0;
}

DEFSMETHOD(History_Insert)
{
	GETDATA;
	return DoMethod(data->pop_path, MM_History_Insert, msg->item);
}

DEFSMETHOD(History_Remove)
{
	GETDATA;
	return DoMethod(data->pop_path, MM_History_Remove, msg->item);
}

DEFSMETHOD(OWBWindow_AutoComplete)
{
	if(getv(app, MA_OWBApp_URLCompletionType) != MV_OWBApp_URLCompletionType_None)
	{
		if (muiRenderInfo(obj))
		{
			GETDATA;
			STRPTR text = (STRPTR) getv(data->pop_path, MUIA_String_Contents);
			
			if(text)
			{
				LONG len = strlen(text);

				DoMethod(_win(obj), MM_OWBWindow_AutoComplete, len, data->prevlen);

	            text = (STRPTR) getv(data->pop_path, MUIA_String_Contents);
				data->prevlen = strlen(text);
			}
		}
	}

	return 0;
}

DEFSMETHOD(AddressBarGroup_Mark)
{
	GETDATA;
	STRPTR url = msg->url ? msg->url : (STRPTR) getv(data->pop_path, MUIA_String_Contents);
	ULONG isbookmark = DoMethod((Object *) getv(app, MA_OWBApp_BookmarkWindow), MM_Bookmarkgroup_ContainsURL, url);
	set(data->bt_add, MA_ToolButton_AddBookmark_IsBookmark, isbookmark);

	return 0;
}

DEFSMETHOD(AddressBarGroup_CompleteString)
{
	Object *urlString = (Object *) getv(obj, MUIA_Popstring_String);

	nnset(urlString, MUIA_String_Contents, msg->url);
	nnset(urlString, MUIA_Textinput_CursorPos, msg->cursorpos);
	nnset(urlString, MUIA_Textinput_MarkStart, msg->markstart);
	nnset(urlString, MUIA_Textinput_MarkEnd, msg->markend);

	free(msg->url); // Allocated by caller (pushed), freed here.

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECMMETHOD(List_InsertSingle)
DECSMETHOD(OWBWindow_LoadURL)
DECTMETHOD(AddressBarGroup_LoadHistory)
DECSMETHOD(History_Insert)
DECSMETHOD(History_Remove)
DECSMETHOD(OWBWindow_AutoComplete)
DECSMETHOD(AddressBarGroup_Mark)
DECSMETHOD(AddressBarGroup_CompleteString)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, addressbargroupclass)
