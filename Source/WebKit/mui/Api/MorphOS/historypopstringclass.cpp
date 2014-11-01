/*
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
 * Copyright 2009 Ilkka Lehtoranta <ilkleht@isoveli.org>
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

#include <string.h>
#include <devices/rawkeycodes.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"

using namespace WebCore;

struct Data
{
	Object *str;
	Object *pop;
	Object *lv_entries;

	ULONG added;
	struct MUI_EventHandlerNode ehnode;
};

MUI_HOOK(history_popclose, APTR list, APTR str)
{
	struct history_entry *s = NULL;

	DoMethod((Object *) list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &s);
	if(s)
	{
		WebHistoryItem *witem = (WebHistoryItem *) s->webhistoryitem;
		char *wurl = (char *) witem->URLString();
		nnset((Object *) str, MUIA_String_Contents, wurl);
		set(_win((Object *) str), MUIA_Window_ActiveObject, (Object *) str);
		DoMethod(_win((Object *) str), MM_OWBWindow_LoadURL, wurl, NULL);
		free(wurl);
	}

	return 0;
}

MUI_HOOK(history_popopen, APTR pop, APTR win)
{
	Object *list = (Object *)getv((Object *)pop, MUIA_Listview_List);

	SetAttrs((Object *) win, MUIA_Window_DefaultObject, list, MUIA_Window_ActiveObject, list, TAG_DONE);
	set(list, MUIA_List_Active, MUIV_List_Active_Off/*0*/);

	return (TRUE);
}

STATIC VOID doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MA_PopString_ActivateString:
				if(tag->ti_Data)
				{
					set((Object *) _win(obj), MUIA_Window_ActiveObject, data->str);
				}
				break;
		}
	}
}

DEFNEW
{
	Object *str, *pop, *bt_pop, *lv_entries;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		Child, pop = PopobjectObject,
			MUIA_Popstring_String, str = (Object *) NewObject(geturlstringclass(), NULL, TAG_DONE),
			MUIA_Popstring_Button, bt_pop = PopButton(MUII_PopUp),
			MUIA_Popobject_Object, lv_entries =  (Object *) NewObject(gethistorylistclass(), NULL, TAG_DONE),
			MUIA_Popobject_ObjStrHook, &history_popclose_hook,
			MUIA_Popobject_WindowHook, &history_popopen_hook,
		End,
		TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->added = FALSE;

		data->str = str;
		data->pop = pop;
		data->lv_entries = lv_entries;

		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_RAWKEY;
		data->ehnode.ehn_Priority = 5;
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

		set(bt_pop, MUIA_CycleChain, 1);

		set(lv_entries, MUIA_Popstring_String, str);
		set(lv_entries, MUIA_Popobject_Object, obj);

		DoMethod(lv_entries, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, data->lv_entries, 1, MM_HistoryList_SelectChange);
		DoMethod(lv_entries, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, obj, 2, MUIM_Popstring_Close, TRUE);
	}

	return (IPTR)obj;
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
		case MA_OWB_ObjectType:
			*msg->opg_Storage = MV_OWB_ObjectType_URL;
			return TRUE;

		case MUIA_Popobject_Object:
            *msg->opg_Storage = (ULONG) data->lv_entries;
            return TRUE;

		case MUIA_Popstring_String:
			*msg->opg_Storage = (ULONG) data->str;
			return TRUE;

		case MA_OWB_URL:
		case MUIA_String_Contents:
			return GetAttr(MUIA_String_Contents, data->str, (ULONGPTR)msg->opg_Storage);
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
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);
		data->added = FALSE;
	}
	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	return DOSUPER;
}

DEFMMETHOD(Cleanup)
{
	return DOSUPER;
}

DEFMMETHOD(HandleEvent)
{
	GETDATA;
	ULONG rc = 0;
	struct IntuiMessage *imsg;

	if(msg->muikey > MUIKEY_NONE)
	{
		switch(msg->muikey)
		{
			case MUIKEY_WINDOW_CLOSE:
			{
				Object *active = (Object *) getv(_win(obj), MUIA_Window_ActiveObject);

				if(active == data->str)
				{
					DoMethod(obj, MUIM_Popstring_Close, FALSE);
					rc = MUI_EventHandlerRC_Eat;
				}
				break;
			}
		}
	}
	else if((imsg = msg->imsg))
	{
		if(imsg->Class == IDCMP_RAWKEY)
		{
			Object *active = (Object *) getv(_win(obj), MUIA_Window_ActiveObject);

			if(active == data->str && !(imsg->Code & IECODE_UP_PREFIX))
			{
				switch(imsg->Code & ~IECODE_UP_PREFIX)
				{
					case RAWKEY_ESCAPE:
						DoMethod(obj, MUIM_Popstring_Close, FALSE);
	                    rc = MUI_EventHandlerRC_Eat;
						break;

					case RAWKEY_RETURN:
						DoMethod(obj, MUIM_Popstring_Close, TRUE);
	                    rc = MUI_EventHandlerRC_Eat;
						break;

					case RAWKEY_UP:
					//case RAWKEY_NM_WHEEL_UP:
						set(data->lv_entries, MUIA_List_Active, MUIV_List_Active_Up);
						rc = MUI_EventHandlerRC_Eat;
						break;

					case RAWKEY_DOWN:
					//case RAWKEY_NM_WHEEL_DOWN:
						DoMethod(obj, MUIM_Popstring_Open);
						set(_win(obj), MUIA_Window_Activate, TRUE);
						set(_win(obj), MUIA_Window_ActiveObject, data->str);
						set(data->lv_entries, MUIA_List_Active, MUIV_List_Active_Down);
	                    rc = MUI_EventHandlerRC_Eat;
						break;
				}
			}
		}
	}

	return rc;

}

DEFSMETHOD(History_Insert)
{
	GETDATA;
	// Allocated by list constructor
	DoMethod(data->lv_entries, MUIM_List_InsertSingle, msg->item, MUIV_List_Insert_Top);

	return 0;
}

DEFSMETHOD(History_Remove)
{
	GETDATA;
	WebHistoryItem *s;
	struct history_entry *x;
	ULONG i;

	s = (WebHistoryItem *) msg->item;

	for (i = 0; ; i++)
	{
		DoMethod(data->lv_entries, MUIM_List_GetEntry, i, &x);

		if (!x)
			break;

		if (s == x->webhistoryitem)
		{
			DoMethod(data->lv_entries, MUIM_List_Remove, i);
			// Freed by list destructor
			break;
		}
	}

	return 0;
}

DEFMMETHOD(Popstring_Open)
{
	GETDATA;

	// Avoid closing popup when it's told to be opened again. :)
	if(!getv(data->lv_entries, MA_HistoryList_Opened))
	{
		return DOSUPER;
	}
	else
	{	
		return 0;
	}
}

DEFMMETHOD(Popstring_Close)
{
	return DOSUPER;
}

BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECSMETHOD(History_Insert)
DECSMETHOD(History_Remove)
DECMMETHOD(Popstring_Open)
DECMMETHOD(Popstring_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, historypopstringclass)
