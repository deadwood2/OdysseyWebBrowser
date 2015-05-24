/*
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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

#include <string.h>

#include <devices/rawkeycodes.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "config.h"
#include "gui.h"

struct Data
{
	Object * str;
	Object * pop;
	Object * lv_entries;

	ULONG added;
	struct MUI_EventHandlerNode ehnode;
};

MUI_HOOK(popclose, APTR list, APTR str)
{
	STRPTR s;

	DoMethod((Object *) list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &s);
	nnset((Object *) str, MUIA_String_Contents, s);
	set(_win((Object *) str), MUIA_Window_ActiveObject, (Object *) str);

	return 0;
}

MUI_HOOK(popopen, APTR pop, APTR win)
{
	Object *list = (Object *)getv((Object *)pop , MUIA_Listview_List);

	SetAttrs((Object *) win, MUIA_Window_DefaultObject, list, MUIA_Window_ActiveObject, list, TAG_DONE);
	set(list, MUIA_List_Active, 0);

	return (TRUE);
}

STATIC VOID doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{   /*
			case MUIA_Text_Contents:
				#warning is it needed?
				break;
			*/
			case MA_PopString_ActivateString:
				if(tag->ti_Data)
				{
#if !OS(AROS) // This cause a hang when opening bookmarks/history side panel
					set((Object *) _win(obj), MUIA_Window_ActiveObject, data->str);
#endif
				}
				break;
		}
	}
}

DEFNEW
{
	Object *str;
	Object *pop, *bt_pop, *lv_entries;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		Child, pop = PopobjectObject,
			MUIA_Popstring_String, str = StringObject,
				StringFrame,
				//MUIA_Textinput_RemainActive, TRUE,
				//MUIA_Textinput_Format, MUIV_Textinput_Format_Center,
				MUIA_Textinput_ResetMarkOnCursor, TRUE,
				MUIA_CycleChain, 1,
				MUIA_String_MaxLen, 2048,
			End,
			MUIA_Popstring_Button, bt_pop = PopButton(MUII_PopUp),
			MUIA_Popobject_Object, lv_entries = ListviewObject,
				MUIA_Listview_List, ListObject,
					InputListFrame,
					MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
					MUIA_List_DestructHook, MUIV_List_DestructHook_String,
					End,
				End,
			MUIA_Popobject_ObjStrHook, &popclose_hook,
			MUIA_Popobject_WindowHook, &popopen_hook,
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
		data->ehnode.ehn_Priority = 1;
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

		set(bt_pop, MUIA_CycleChain, 1);

		DoMethod(data->lv_entries, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, obj, 2, MUIM_Popstring_Close, TRUE);
		//DoMethod(str, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 3, MUIM_Set, MUIA_String_Contents, MUIV_TriggerValue);
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
		case MUIA_Popobject_Object:
            *msg->opg_Storage = (ULONG) data->lv_entries;
            return TRUE;

		case MUIA_Popstring_String:
			*msg->opg_Storage = (ULONG) data->str;
			return TRUE;

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
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode );
		data->added = FALSE;
	}
	return DOSUPER;
}

DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg;

	if ((imsg = msg->imsg))
	{
		int MouseX, MouseY;

		MouseX = imsg->MouseX;
		MouseY = imsg->MouseY;

        if (_isinobject(obj, MouseX, MouseY))
		{
			if (imsg->Class == IDCMP_RAWKEY)
			{
				GETDATA;
				Object *active;

				switch (imsg->Code)
				{
					case RAWKEY_DOWN + 0x80:
					case RAWKEY_NM_WHEEL_DOWN + 0x80:
						active = (Object *)getv(_win(obj) , MUIA_Window_ActiveObject);

						if	(active == data->str)
						{
							if (getv(data->lv_entries, MUIA_List_Entries))
							{
								DoMethod(data->pop, MUIM_Popstring_Open);
								return MUI_EventHandlerRC_Eat;
							}
						}
						break;
				}
			}
		}
	}

	return 0;

}

DEFSMETHOD(PopString_Insert)
{
	STRPTR s, x;
	ULONG i;
	GETDATA;

	s = (STRPTR) msg->txt;

	/*
	 * Find if the current entry is already there and remove it
	 * if so.
	 */

	for (i = 0; ; i++)
	{
		DoMethod(data->lv_entries, MUIM_List_GetEntry, i, &x);
						
		if (!x)
			break;

		if (!(stricmp(s, x))) /* XXX: we should strip spaces too.. */
		{
			DoMethod(data->lv_entries, MUIM_List_Remove, i);
			break;
		}
	}

#if 0
	while (getv(data->lv_entries, MUIA_List_Entries) >= data->max_items)
	{
		DoMethod(data->lv_entries, MUIM_List_Remove, MUIV_List_Remove_Last);
	}
#endif

	return DoMethod(data->lv_entries, MUIM_List_InsertSingle, s, MUIV_List_Insert_Top);
}

BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(HandleEvent)
DECSMETHOD(PopString_Insert)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, popstringclass)
