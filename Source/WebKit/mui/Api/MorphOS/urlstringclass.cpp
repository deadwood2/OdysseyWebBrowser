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
#include <wtf/text/CString.h>
#include "WebHistory.h"
#include "WebHistoryItem.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <clib/debug_protos.h>

#include "gui.h"

using namespace WebCore;

enum {
	POPMENU_PASTE_AND_GO = 1,
};

struct Data
{
	Object *menu;
/*
	ULONG added;
	struct MUI_EventHandlerNode ehnode;
*/
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
			StringFrame,
			MUIA_Textinput_ResetMarkOnCursor, TRUE,
			MUIA_CycleChain, 1,
			MUIA_String_MaxLen, 16384,
			TAG_MORE, INITTAGS,
			TAG_DONE);

	return (ULONG)obj;
}
/*
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

DEFMMETHOD(Show)
{
	ULONG rc = DOSUPER;

	if (rc)
	{
		GETDATA;

		if(!data->added)
		{
			data->ehnode.ehn_Object = obj;
			data->ehnode.ehn_Class  = cl;
			data->ehnode.ehn_Priority = -1;
			data->ehnode.ehn_Flags  = MUI_EHF_GUIMODE;
			data->ehnode.ehn_Events = IDCMP_RAWKEY;

			DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
			data->added = TRUE;
		}
	}

	return rc;
}

DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg = msg->imsg;
	ULONG rc = 0;

	if (imsg)
	{
		switch (imsg->Class)
		{
			case IDCMP_RAWKEY:
				switch (imsg->Code)
				{
					default:
						{
							struct InputEvent ie;
							TEXT buffer[4];

							ie.ie_Class        = IECLASS_RAWKEY;
							ie.ie_SubClass     = 0;
							ie.ie_Code         = imsg->Code;
							ie.ie_Qualifier    = 0;
							ie.ie_EventAddress = NULL;

							if (MapRawKey(&ie, (STRPTR) buffer, 4, NULL) == 1)
							{
								LONG page = buffer[0] - '0';

								if (page >= 0 && page <= 9)
								{
									page--;

									if (page == -1)
										page += 10;

									if (imsg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
										page += 10;

									if (imsg->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
										page += 10;

									if (imsg->Qualifier & IEQUALIFIER_CONTROL)
										page += 10;

									if (imsg->Qualifier & (IEQUALIFIER_RCOMMAND | IEQUALIFIER_LCOMMAND))
									{
										set((APTR)getv(obj, MUIA_Parent), MUIA_Group_ActivePage, page);
										rc = MUI_EventHandlerRC_Eat;
									}
								}
							}
						}
						break;
				}
				break;
		}
	}

	return rc;
}
*/

/*
DEFMMETHOD(ContextMenuAdd)
{
	GETDATA;

	data->menu = MenuObject, MUIA_Menu_Title, "OWB URL", End;

	if(data->menu)
	{

		DoMethod(data->menu, OM_ADDMEMBER, MenuitemObject, MUIA_UserData, POPMENU_PASTE_AND_GO, MUIA_Menuitem_Title, "Paste and Go", End);
	}

	DoMethod(msg->menustrip, OM_ADDMEMBER, data->menu);

	return DOSUPER;
}

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;
	ULONG rc = 0;

	if (data->menu && DoMethod(data->menu, MUIM_FindObject, msg->item))
	{
		switch(muiUserData(msg->item))
		{
			case POPMENU_PASTE_AND_GO:
			{
                String url = pasteFromClipboard();
				DoMethod(_win(obj), MM_OWBWindow_LoadURL, (char *) url.utf8().data(), NULL);
				break;
			}
			default:
				;
		}
	}
	else
		rc = DOSUPER;

	data->menu = NULL;

	return rc;
}
*/

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
	char *url = (char *) getv(msg->obj, MA_OWB_URL);

	if(url)
	{
		set(_win(obj), MUIA_Window_Activate, TRUE);
		set(obj, MUIA_Text_Contents, url);
	}

	return DOSUPER;
}

DEFMMETHOD(DragFinish)
{
	set(_win(obj), MUIA_Window_ActiveObject, obj);
	return DOSUPER;
}


BEGINMTABLE
DECNEW
/*
DECMMETHOD(HandleEvent)
DECMMETHOD(Hide)
DECMMETHOD(Show)
*/
/*
DECMMETHOD(ContextMenuAdd)
DECMMETHOD(ContextMenuChoice)
*/
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(DragFinish)
ENDMTABLE

DECSUBCLASS_NC(MUIC_String, urlstringclass)
