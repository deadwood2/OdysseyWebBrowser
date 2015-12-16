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

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <devices/rawkeycodes.h>

#include "gui.h"

#define SORTABLE_TABS 1

struct Data
{
	ULONG added;
	struct MUI_EventHandlerNode ehnode;
};

DEFMMETHOD(Title_Close)
{
	if (msg->tito)
	{
		Object *browser = (Object *) getv(msg->tito, MUIA_UserData);
		DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveBrowser, browser);
	}

	return 0;
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_CycleChain, 1,
#if SORTABLE_TABS
		MUIA_Title_Sortable, TRUE,
//		  MUIA_Title_Newable, TRUE,
#endif
		MUIA_Group_SameWidth, TRUE,
		TAG_MORE, INITTAGS
	);

	if(obj)
	{
		GETDATA;
		data->added = FALSE;
	}

	return ((ULONG)obj);
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
					case RAWKEY_PAGEUP:
					case RAWKEY_PAGEDOWN:
						if (imsg->Qualifier  & (IEQUALIFIER_CONTROL | IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))
						{
							set((APTR)getv(obj, MUIA_Parent), MUIA_Group_ActivePage, imsg->Code == RAWKEY_PAGEUP ? MUIV_Group_ActivePage_Prev : MUIV_Group_ActivePage_Next);
							rc = MUI_EventHandlerRC_Eat;
						}
						break;

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

									if (imsg->Qualifier & (IEQUALIFIER_CONTROL | IEQUALIFIER_RCOMMAND | IEQUALIFIER_LCOMMAND))
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

#if SORTABLE_TABS

DEFMMETHOD(DragQuery)
{
	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if (obj != msg->obj && (type == MV_OWB_ObjectType_Tab) /*&& _win(obj) == _win(msg->obj)*/)
	{
		ULONG value = 0;

		if(GetAttr(MUIA_Title_Sortable, obj, (ULONGPTR) &value))
		{
			return (MUIV_DragQuery_Accept);
		}
	}
	return DOSUPER;
}

DEFMMETHOD(DragDrop)
{
	LONG type = getv(msg->obj, MA_OWB_ObjectType);
	ULONG rc = 0;

	// Drop from another window tab -> override and add to end.
	if (type == MV_OWB_ObjectType_Tab && _win(obj) != _win(msg->obj))
	{
		DoMethod(_win(msg->obj), MM_OWBWindow_DetachBrowser, (Object *) getv(msg->obj, MA_OWB_Browser), _win(obj));
	}
	else
	{
		rc = DOSUPER;
	}

	return rc;
}

DEFMMETHOD(Title_New)
{
	return DoMethod(_win(obj), MM_OWBWindow_MenuAction, MNA_NEW_PAGE);
}
DEFMMETHOD(Title_CreateNewButton)
{
	return (ULONG) NewObject(gettoolbutton_newtabclass(), NULL, TAG_DONE);
}

#endif

BEGINMTABLE
DECNEW
DECMMETHOD(HandleEvent)
DECMMETHOD(Hide)
DECMMETHOD(Show)
DECMMETHOD(Title_Close)
#if SORTABLE_TABS
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(Title_New)
DECMMETHOD(Title_CreateNewButton)
#endif
ENDMTABLE

DECSUBCLASS_NC(MUIC_Title, titleclass)
