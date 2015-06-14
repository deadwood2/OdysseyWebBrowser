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

#include <stdio.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>
#include <devices/timer.h>
#include <utility/date.h>

#include "gui.h"
#include "clipboard.h"

using namespace WebCore;

enum {
	POPMENU_OPEN_COPY = 1,
};

struct Data
{
	Object *cmenu;
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Format,      "BAR, MIW=-1 MAW=-2",
        MUIA_List_AutoVisible, TRUE,
		MUIA_List_Title,       TRUE,
		MUIA_ContextMenu,      TRUE,
		TAG_MORE,              INITTAGS
	);

	if (obj)
	{
		GETDATA;
		data->cmenu=NULL;
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
	return (ULONG)msg->entry;
}

DEFMMETHOD(List_Destruct)
{
	return TRUE;
}

DEFMMETHOD(List_Display)
{
	struct console_entry *e = (struct console_entry *) msg->entry;

	if (e)
	{
		STATIC char buf1[10];
		STATIC char buf2[2048];
		snprintf(buf1, sizeof(buf1), "%.2d:%.2d:%.2d", e->clockdata.hour, e->clockdata.min, e->clockdata.sec);
		snprintf(buf2, sizeof(buf2), "%s", e->message);

		msg->array[0] = buf1;
		msg->array[1] = buf2;
	}
	else
	{
		msg->array[0] = GSI(MSG_CONSOLELIST_TIME);
		msg->array[1] = GSI(MSG_CONSOLELIST_MESSAGE);
	}

	return TRUE;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUI_List_TestPos_Result res;
	struct console_entry *ce;

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
		data->cmenu = NULL;
	}

	if (DoMethod(obj, MUIM_List_TestPos, msg->mx, msg->my, &res) && (res.entry != -1))
	{
		DoMethod(obj, MUIM_List_GetEntry, res.entry, (ULONG *)&ce);

		if(ce)
		{
			data->cmenu = MenustripObject,
					MUIA_Family_Child, MenuObjectT(GSI(MSG_CONSOLELIST_MESSAGE)),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_CONSOLELIST_COPY),
						MUIA_UserData, POPMENU_OPEN_COPY,
	                    End,
	                End,
	            End;
		}
	}
	return (ULONG)data->cmenu;
}

DEFMMETHOD(ContextMenuChoice)
{
	struct console_entry *ce;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&ce);

	if(ce)
	{
		ULONG udata = muiUserData(msg->item);

		switch(udata)
		{
			case POPMENU_OPEN_COPY:
			{
				copyTextToClipboard(ce->message, false);
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
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, consolelistclass)
