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

using namespace WebCore;

struct Data
{
	Object *lv_messages;
};

DEFNEW
{
	Object *lv_messages;
	Object *clear;

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Window_ID, MAKE_ID('W','C','O','N'),
			MUIA_Window_Title, GSI(MSG_CONSOLEWINDOW_MESSAGES),
			MUIA_Window_NoMenus, TRUE,
			WindowContents, VGroup,
				Child, VGroup,
					Child, lv_messages = (Object *) NewObject(getconsolelistclass(), NULL,TAG_DONE),
					Child, HGroup,
						Child, clear = (Object *) MakeButton(GSI(MSG_CONSOLEWINDOW_CLEAR)),
					End,
				End,
			End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->lv_messages = lv_messages;

		DoMethod(obj,   MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
		DoMethod(clear, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ConsoleWindow_Clear);
	}

	return (IPTR)obj;
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_OWB_WindowType:
		{
			*msg->opg_Storage = (ULONG) MV_OWB_Window_Console;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFSMETHOD(ConsoleWindow_Add)
{
	struct console_entry *e = (struct console_entry *) malloc(sizeof(*e));

	if(e)
	{
		GETDATA;
		struct timeval timeval;

		GetSysTime(&timeval);
		Amiga2Date(timeval.tv_secs, &e->clockdata);

		stccpy(e->message, msg->message, sizeof(e->message));

		DoMethod(data->lv_messages, MUIM_List_InsertSingle, e);
		set(data->lv_messages, MUIA_List_Active, MUIV_List_Active_Bottom);
	}

	return 0;
}


DEFTMETHOD(ConsoleWindow_Clear)
{
	GETDATA;

	set(data->lv_messages, MUIA_List_Quiet, TRUE);

	DoMethod(data->lv_messages, MUIM_List_Clear);

	set(data->lv_messages, MUIA_List_Quiet, FALSE);

	return 0;
}

BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(ConsoleWindow_Add)
DECTMETHOD(ConsoleWindow_Clear)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, consolewindowclass)
