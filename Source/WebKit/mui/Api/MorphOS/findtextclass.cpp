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

#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"

struct Data
{
	Object *pop_search;
	Object *bt_prev;
	Object *bt_next;
	Object *ch_case;
	Object *ch_showallmatches;
	Object *txt_notfound;

	Object *obj_target;
};

STATIC VOID doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			/*
			case MUIA_Text_Contents:
				#warning is it needed?
				break;
			*/
			case MA_FindText_Target:
				data->obj_target = (Object *)tag->ti_Data;
				break;
			
			case MA_FindText_Active:
				if(tag->ti_Data)
				{
					set(data->pop_search, MA_PopString_ActivateString, TRUE);
					DoMethod((Object *) getv(data->pop_search, MUIA_Popstring_String), MUIM_Textinput_DoMarkAll);
				}
				break;
		}
	}
}

DEFNEW
{
	Object *bt_close = NULL, *bt_prev =  NULL, *bt_next = NULL;
	Object *pop_search = NULL, *ch_case = NULL, *ch_showallmatches = NULL, *txt_notfound = NULL;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		TAG_MORE, INITTAGS);

	if (obj)
	{
		GETDATA;
		ULONG closable       = GetTagData(MA_FindText_Closable,    TRUE, INITTAGS);
		ULONG buttons        = GetTagData(MA_FindText_ShowButtons, TRUE, INITTAGS);
		ULONG casecheck      = GetTagData(MA_FindText_ShowCaseSensitive, TRUE, INITTAGS);
		ULONG showtext       = GetTagData(MA_FindText_ShowText,    TRUE, INITTAGS);
		ULONG showallmatches = GetTagData(MA_FindText_ShowAllMatches, FALSE, INITTAGS);

		DoMethod(obj, MUIM_Group_InitChange);

		if(closable)
		{
			bt_close = ImageObject,
					   MUIA_InputMode, MUIV_InputMode_RelVerify,
					   MUIA_Image_Spec, MUII_Close,
					   MUIA_CycleChain, 1,
					   End;
			DoMethod(obj, MUIM_Group_AddTail, bt_close);
		}

		pop_search = (Object *) NewObject(getpopstringclass(), NULL, TAG_DONE);
		DoMethod(obj, MUIM_Group_AddTail, pop_search);

		if(buttons)
		{
			bt_prev = (Object *)MakeButton(GSI(MSG_FINDTEXT_PREV));
			DoMethod(obj, MUIM_Group_AddTail, bt_prev);
		}

		bt_next = (Object *)MakeButton(GSI(MSG_FINDTEXT_NEXT));
		DoMethod(obj, MUIM_Group_AddTail, bt_next);

		if(casecheck)
		{
			ch_case = (Object *)MakeCheck(GSI(MSG_FINDTEXT_CASE_SENSITIVE), FALSE);
			DoMethod(obj, MUIM_Group_AddTail, ch_case);
            DoMethod(obj, MUIM_Group_AddTail, MakeLabel(GSI(MSG_FINDTEXT_CASE_SENSITIVE)));
		}

		if(showallmatches)
		{
			ch_showallmatches = (Object *)MakeCheck(GSI(MSG_FINDTEXT_MARK_ALL_MATCHES), FALSE);
			DoMethod(obj, MUIM_Group_AddTail, ch_showallmatches);
            DoMethod(obj, MUIM_Group_AddTail, MakeLabel(GSI(MSG_FINDTEXT_MARK_ALL_MATCHES)));
		}

		if(showtext)
		{
			DoMethod(obj, MUIM_Group_AddTail, MakeRect());

	        txt_notfound = (Object *) TextObject,
							MUIA_Text_Contents, "",
							MUIA_Text_PreParse, "\338",
							MUIA_FixWidthTxt, GSI(MSG_FINDTEXT_TEXT_NOT_FOUND),
							End;

			DoMethod(obj, MUIM_Group_AddTail, txt_notfound);
		}

		DoMethod(obj, MUIM_Group_ExitChange);

		data->pop_search        = pop_search;
		data->bt_prev           = bt_prev;
		data->bt_next           = bt_next;
		data->ch_case           = ch_case;
		data->ch_showallmatches = ch_showallmatches;
		data->txt_notfound      = txt_notfound;

		data->obj_target = (Object *)GetTagData(MA_FindText_Target, NULL, INITTAGS);

		if(bt_prev) set(bt_prev, MUIA_Weight, 0);
		if(bt_next)	set(bt_next, MUIA_Weight, 0);

		DoMethod((Object *) getv(pop_search, MUIA_Popstring_String), MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 4, MM_FindText_Find, NULL, 0, FALSE);
		DoMethod((Object *) getv(pop_search, MUIA_Popstring_String), MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 4, MM_FindText_Find, NULL, 0, TRUE);
		DoMethod((Object *) getv(pop_search, MUIA_Popstring_String), MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, pop_search, 3, MUIM_Set, MA_PopString_ActivateString, TRUE);
		
		if(ch_showallmatches) DoMethod(ch_showallmatches, MUIM_Notify, MUIA_Selected, MUIV_EveryTime, obj, 4, MM_FindText_Find, NULL, MV_FindText_MarkOnly, FALSE);
		if(bt_prev)  DoMethod(bt_prev,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 4, MM_FindText_Find, NULL, MV_FindText_Previous, TRUE);
		if(bt_next)  DoMethod(bt_next,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 4, MM_FindText_Find, NULL, 0, TRUE);
		if(bt_close) DoMethod(bt_close, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MUIM_Set, MUIA_ShowMe, FALSE);
	}

	return (ULONG)obj;
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
			return GetAttr(MUIA_String_Contents, data->pop_search, (ULONGPTR)msg->opg_Storage);
	}

	return DOSUPER;
}

DEFMMETHOD(Show)
{
	GETDATA;
	ULONG rc;

	if ((rc = DOSUPER))
	{
		set(data->pop_search, MA_PopString_ActivateString, TRUE);
	}

	return rc;
}

DEFSMETHOD(FindText_DisableButtons)
{
	GETDATA;
	if(data->bt_prev)
	{
		set(data->bt_prev, MUIA_Disabled, msg->prev);
	}

	if(data->bt_next)
	{
		set(data->bt_next, MUIA_Disabled, msg->next);
	}

	return 0;
}

DEFSMETHOD(FindText_Find)
{
	GETDATA;

	ULONG flags = msg->flags;
	STRPTR string = NULL;

	if(data->txt_notfound)
	{
		set(data->txt_notfound, MUIA_Text_Contents, "");
	}

	if(data->ch_case) 
	{
		flags |= getv(data->ch_case, MUIA_Selected) ? MV_FindText_CaseSensitive : 0;
	}

	if(data->ch_showallmatches)
	{
		flags |= getv(data->ch_showallmatches, MUIA_Selected) ? MV_FindText_ShowAllMatches : 0;
	}

	string = (STRPTR)getv(data->pop_search, MUIA_String_Contents);

	if(string && *string && data->obj_target)
	{
		ULONG res;
		if(msg->validate)
		{
			DoMethod(data->pop_search, MM_PopString_Insert, string);
		}

		res = DoMethod(data->obj_target, MM_Search, string, flags);

		if(data->txt_notfound)
		{
			set(data->txt_notfound, MUIA_Text_Contents, res ? (STRPTR) "" : GSI(MSG_FINDTEXT_TEXT_NOT_FOUND) );
		}
	}
	return 0;
}

DEFMMETHOD(DragQuery)
{
	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if (type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_Tab ||
		type == MV_OWB_ObjectType_Bookmark || type == MV_OWB_ObjectType_QuickLink || type == MV_OWB_ObjectType_URL)
	{
		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	GETDATA;
	char *url = (char *) getv(msg->obj, MA_OWB_URL);

	if(url)
	{
		set(_win(obj), MUIA_Window_Activate, TRUE);
		set(data->pop_search, MUIA_String_Contents, url);
	}

	return DOSUPER;
}

DEFMMETHOD(DragFinish)
{
	GETDATA;
	set(_win(obj), MUIA_Window_ActiveObject, data->pop_search);
	return DOSUPER;
}


BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(Show)
DECSMETHOD(FindText_DisableButtons)
DECSMETHOD(FindText_Find)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(DragFinish)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, findtextclass)
