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

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <stdio.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

struct Data
{
	Object *txt_realm;
	Object *st_user;
	Object *st_password;
	Object *ch_save;
	ULONG done;
	ULONG validate;
};

DEFNEW
{
	Object *st_user, *st_password;
	Object *bt_login, *bt_cancel;
	Object *ch_save;
	STRPTR realm, host, suggested_login, suggested_password;
	char message[1024];

	host  = (STRPTR) GetTagData(MA_LoginWindow_Host, NULL, msg->ops_AttrList);
	realm = (STRPTR) GetTagData(MA_LoginWindow_Realm, NULL, msg->ops_AttrList);
	suggested_login    = (STRPTR) GetTagData(MA_LoginWindow_Username, NULL, msg->ops_AttrList);
	suggested_password = (STRPTR) GetTagData(MA_LoginWindow_Password, NULL, msg->ops_AttrList);

	snprintf(message, sizeof(message), GSI(MSG_LOGINWINDOW_MESSAGE), realm, host);

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Window_ID, MAKE_ID('W','A','U','T'),
			MUIA_Window_TopEdge , MUIV_Window_TopEdge_Centered,
			MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered,
			MUIA_Window_Title, GSI(MSG_LOGINWINDOW_TITLE),
			MUIA_Window_NoMenus, TRUE,
			WindowContents, VGroup,
				MUIA_Background, MUII_RequesterBack,
				Child, VGroup,

                    Child, TextObject,
		                    TextFrame,
		                    MUIA_InnerBottom, 8,
		                    MUIA_InnerLeft, 8,
		                    MUIA_InnerRight, 8,
		                    MUIA_InnerTop, 8,
		                    MUIA_Background, MUII_TextBack,
		                    MUIA_Text_SetMax, TRUE,
							MUIA_Text_Contents, message,
		                    End,

                    Child, VSpace(2),

					Child, ColGroup(2),
						Child, MakeLabel(GSI(MSG_LOGINWINDOW_USERNAME)),
						Child, st_user = (Object *) MakeString(suggested_login ? suggested_login : "", FALSE),
						Child, MakeLabel(GSI(MSG_LOGINWINDOW_PASSWORD)),
						Child, st_password = (Object *) MakeString(suggested_password ? suggested_password : "", TRUE),
						Child, HSpace(0),
						Child, ColGroup(3),
									Child, ch_save = (Object *) MakeCheck(GSI(MSG_LOGINWINDOW_SAVE_CREDENTIALS), FALSE),
									Child, LLabel(GSI(MSG_LOGINWINDOW_SAVE_CREDENTIALS)),
									Child, HSpace(0),
								End,
						End,
					//Child, MakeHBar(),
					Child, VSpace(2),

					Child, HGroup,
						Child, bt_login = (Object *) MakeButton(GSI(MSG_LOGINWINDOW_LOGIN)),
						Child, RectangleObject, End,
						Child, bt_cancel = (Object *) MakeButton(GSI(MSG_LOGINWINDOW_CANCEL)),
					End,
				End,
			End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->st_user     = st_user;
		data->st_password = st_password;
		data->ch_save     = ch_save;
		data->done     = FALSE;
		data->validate = FALSE;

		set(obj, MUIA_Window_ActiveObject, data->st_user);

		if(suggested_login && *suggested_login)
		{
			set(ch_save, MUIA_Selected, TRUE);
		}

		DoMethod(obj,       MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 2, MM_LoginWindow_Login, FALSE);
		DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_LoginWindow_Login, FALSE);
		DoMethod(bt_login,  MUIM_Notify, MUIA_Pressed, FALSE, obj, 2, MM_LoginWindow_Login, TRUE);
	}

	return (IPTR)obj;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_OWB_WindowType:
		{
			*msg->opg_Storage = (ULONG) MV_OWB_Window_Auth;
		}
		return TRUE;

		case MA_LoginWindow_Username:
		{
			*msg->opg_Storage = (ULONG) getv(data->st_user, MUIA_String_Contents);
		}
		return TRUE;

		case MA_LoginWindow_Password:
		{
			*msg->opg_Storage = (ULONG) getv(data->st_password, MUIA_String_Contents);
		}
		return TRUE;

		case MA_LoginWindow_SaveAuthentication:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_save, MUIA_Selected);
		}
		return TRUE;
	}

	return DOSUPER;
}

/* FIXME: As the caller (webkit) expects answer, it's blocking
		  -> make the main loop complete to handle webkit events
*/
DEFTMETHOD(LoginWindow_Open)
{
	GETDATA;
	ULONG sigs;

	while(!data->done)
	{
		DoMethod(app, MUIM_Application_NewInput, &sigs);
		if (sigs)
		{
			sigs=Wait(sigs|SIGBREAKF_CTRL_C);
			if (sigs & SIGBREAKF_CTRL_C)
			{
				break;
			}
		}
	}

	return data->validate;
}


DEFSMETHOD(LoginWindow_Login)
{
	GETDATA;

	data->done = TRUE;
	data->validate = msg->validate;

	return 0;
}

BEGINMTABLE
DECNEW
DECGET
DECTMETHOD(LoginWindow_Open)
DECSMETHOD(LoginWindow_Login)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, loginwindowclass)
