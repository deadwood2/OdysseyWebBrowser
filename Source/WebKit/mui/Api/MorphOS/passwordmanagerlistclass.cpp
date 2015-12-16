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
#include "BALBase.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>
#include <devices/timer.h>
#include <utility/date.h>

#include "gui.h"
#include "utils.h"
#include "ExtCredential.h"

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
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Format, "BAR,BAR,BAR, H BAR,",
		MUIA_List_Title, TRUE,
		MUIA_ContextMenu, TRUE,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;
		data->cmenu = NULL;
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

DEFMMETHOD(List_Construct)
{
	// Built by caller
	return (ULONG)msg->entry;
}

DEFMMETHOD(List_Destruct)
{
	struct credential_entry *entry = (struct credential_entry *) msg->entry;

	if(entry)
	{
		free(entry->host);
		free(entry->realm);
		free(entry->username);
		free(entry->password);
		free(entry->username_field);
		free(entry->password_field);

		free(entry);
	}

	return TRUE;
}

DEFMMETHOD(List_Display)
{
#define MAX_HOST_WIDTH 64
	struct credential_entry *e = (struct credential_entry *) msg->entry;

	if (e)
	{
		static char shost[MAX_HOST_WIDTH + 1];
		stccpy(shost, e->host, sizeof(shost));
		if(strlen(shost) == MAX_HOST_WIDTH)
		{
			shost[MAX_HOST_WIDTH - 1] = '.';
			shost[MAX_HOST_WIDTH - 2] = '.';
			shost[MAX_HOST_WIDTH - 3] = '.';
		}

		msg->array[0] = shost;
		msg->array[1] = e->type == CREDENTIAL_TYPE_AUTH ? e->realm : (char *) GSI(MSG_PASSWORDMANAGERLIST_FORM);
		msg->array[2] = e->username;
		msg->array[3] = e->password;
		msg->array[4] = e->flags & CREDENTIAL_FLAG_BLACKLISTED ? (char *) GSI(MSG_PASSWORDMANAGERLIST_FILTER_IGNORE) : (char *) GSI(MSG_PASSWORDMANAGERLIST_FILTER_REMEMBER);
	}
	else
	{
		msg->array[0] = GSI(MSG_PASSWORDMANAGERLIST_HOST);
		msg->array[1] = GSI(MSG_PASSWORDMANAGERLIST_REALM);
		msg->array[2] = GSI(MSG_PASSWORDMANAGERLIST_USERNAME);
		msg->array[3] = GSI(MSG_PASSWORDMANAGERLIST_PASSWORD);
		msg->array[4] = GSI(MSG_PASSWORDMANAGERLIST_FILTER);
	}

	return TRUE;
}

DEFMMETHOD(List_Compare)
{
	struct credential_entry *e1 = (struct credential_entry *) msg->entry1;
	struct credential_entry *e2 = (struct credential_entry *) msg->entry2;

	return strcmp(e1->host, e2->host);
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUI_List_TestPos_Result res;
	struct credential_entry *e;

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	if (DoMethod(obj, MUIM_List_TestPos, msg->mx, msg->my, &res) && (res.entry != -1))
	{
		DoMethod(obj, MUIM_List_GetEntry, res.entry, (ULONG *)&e);

		if(e)
		{
			data->cmenu = MenustripObject,
					MUIA_Family_Child, MenuObjectT(GSI(MSG_PASSWORDMANAGERLIST_HOST)),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLIST_OPEN_URL), // XXX: Don't use history msgid, laziness
						MUIA_UserData, POPMENU_OPEN_URL,
	                    End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLIST_OPEN_NEW_TAB), // XXX: Don't use history msgid, laziness
						MUIA_UserData, POPMENU_OPEN_URL_IN_NEW_TAB,
						End,
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_HISTORYLIST_OPEN_NEW_WINDOW), // XXX: Don't use history msgid, laziness
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
	struct credential_entry *e = NULL;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&e);

	if(e)
	{
		ULONG udata = muiUserData(msg->item);

		switch(udata)
		{
			case POPMENU_OPEN_URL:
			{
                Object *window = (Object *) getv(app, MA_OWBApp_ActiveWindow);
				DoMethod(window, MM_OWBWindow_LoadURL, e->host, NULL);
			}
			break;

			case POPMENU_OPEN_URL_IN_NEW_TAB:
			{
				DoMethod(app, MM_OWBApp_AddBrowser, NULL, e->host, FALSE, NULL, FALSE, FALSE, TRUE);
			}
			break;

			case POPMENU_OPEN_URL_IN_NEW_WINDOW:
			{
				DoMethod(app, MM_OWBApp_AddWindow, e->host, FALSE, NULL, FALSE, NULL, FALSE);
			}
			break;
		}
	}
	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, passwordmanagerlistclass)
