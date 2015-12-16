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
#include "SuggestEntry.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <devices/rawkeycodes.h>
#include <proto/intuition.h>
#include <clib/macros.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

struct Data
{
	Object *str;
	Object *pop;

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
	}
	NEXTTAG
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Format, "",
		MUIA_List_Title, FALSE,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;

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
	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_SuggestList_Opened:
			*msg->opg_Storage = (ULONG) data->opened;
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

	data->opened = FALSE;

	if(data->added)
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode );
		data->added = FALSE;
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
	}

	return 0;

}

DEFMMETHOD(List_Construct)
{
	return (ULONG) msg->entry;
}

DEFMMETHOD(List_Destruct)
{
	SuggestEntry *entry = (SuggestEntry *) msg->entry;
	delete entry;
	return TRUE;
}

DEFMMETHOD(List_Display)
{
	SuggestEntry *item = (SuggestEntry *) msg->entry;

	if(item)
	{
		static char label[256];
//		static char occurrences[32];
		char *suggestion = utf8_to_local(item->suggestion().utf8().data());

		if(suggestion)
		{
			snprintf(label, sizeof(label), "%s", suggestion);
//			  snprintf(occurrences, sizeof(occurrences), "  \33P[80------](%d)", item->occurrences());
			free(suggestion);
		}

		msg->array[0] = (char *) label;
//		  msg->array[1] = (char *) occurrences;
	}

	return TRUE;
}

DEFMMETHOD(List_Compare)
{
	SuggestEntry *e1 = (SuggestEntry *)msg->entry1;
	SuggestEntry *e2 = (SuggestEntry *)msg->entry2;


	if(e1->occurrences() < e2->occurrences())
	{
		return 1;
	}
	else if(e1->occurrences() > e2->occurrences())
	{
		return -1;
	}
	else
	{
		return -(WTF::codePointCompare(e1->suggestion(), e2->suggestion()));
	}
}

DEFTMETHOD(SuggestList_SelectChange)
{
	GETDATA;
	SuggestEntry *item = NULL;

	if(muiRenderInfo(obj))
	{
		DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &item);
		if (item)
		{
			char *suggestion = utf8_to_local(item->suggestion().utf8().data());
			if(suggestion)
			{
				nnset(data->str, MUIA_String_Contents, suggestion);
				free(suggestion);
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
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
DECTMETHOD(SuggestList_SelectChange)
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(HandleEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, suggestlistclass)
