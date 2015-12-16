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

#include "gui.h"
#include "utils.h"

using namespace WebCore;

struct Data
{
	Object *bt_ok;
	Object *bt_cancel;
	Object *str;
	Object *ch_menu;
	char *url;
	char *title;
	ULONG quicklink;
};

DEFNEW
{
	Object *bt_ok, *bt_cancel, *str, *ch_menu;

	char *title = (char *) GetTagData(MA_OWB_Title, NULL, msg->ops_AttrList);

	obj = (Object *) DoSuperNew(cl, obj,
        MUIA_Background, MUII_RequesterBack,
		Child, ColGroup(2),
			Child, MakeLabel(GSI(MSG_CHOOSETITLEGROUP_TITLE)),
			Child, str = (Object *) MakeString(title ? title : GSI(MSG_CHOOSETITLEGROUP_NO_TITLE), FALSE),
			Child, HSpace(1),
			Child, ColGroup(3),
						Child, ch_menu = (Object *) MakeCheck(GSI(MSG_CHOOSETITLEGROUP_MENU), getv(app, MA_OWBApp_Bookmark_AddToMenu)),
						Child, LLabel(GSI(MSG_CHOOSETITLEGROUP_MENU)),
						Child, HSpace(0),
				   End,
			End,
		Child, VSpace(2),

        Child, HGroup,
			Child, bt_ok = (Object*) MakeButton(GSI(MSG_CHOOSETITLEGROUP_ADD)),
            Child, HSpace(0),
			Child, bt_cancel = (Object*) MakeButton(GSI(MSG_CHOOSETITLEGROUP_CANCEL)),
            End,
		TAG_MORE, INITTAGS,
		TAG_DONE
	);

	if (obj)
	{
		GETDATA;

		data->bt_ok = bt_ok;
		data->bt_cancel = bt_cancel;
		data->str = str;
		data->ch_menu = ch_menu;
		data->title = title;
		data->url = (char *) GetTagData(MA_OWB_URL, NULL, msg->ops_AttrList),
		data->quicklink = (ULONG) GetTagData(MA_ChooseTitleGroup_QuickLink, 0, msg->ops_AttrList),

		set(data->str, MUIA_CycleChain, 1);

		DoMethod(data->bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
				 obj, 1, MM_ChooseTitleGroup_Add);

		DoMethod(data->bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
				 obj, 1, MM_ChooseTitleGroup_Cancel);

		DoMethod(data->str, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
				 obj, 1, MM_ChooseTitleGroup_Add);
	}

	return (ULONG)obj;
}

DEFDISP
{
	GETDATA;
	free(data->url);
	free(data->title);
	return DOSUPER;
}

DEFTMETHOD(ChooseTitleGroup_Add)
{
	GETDATA;

	STRPTR title = (STRPTR) getv(data->str, MUIA_String_Contents);

	if(title)
	{
		ULONG addtomenu = getv(data->ch_menu, MUIA_Selected);
		set(app, MA_OWBApp_Bookmark_AddToMenu, addtomenu);

		DoMethod(app, MM_Bookmarkgroup_AddLink, title, NULL, data->url, TRUE, addtomenu, data->quicklink);
	}

	DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_DisposeWindow, _win(obj));

	return 0;
}

DEFTMETHOD(ChooseTitleGroup_Cancel)
{
	DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_DisposeWindow, _win(obj));

	return 0;
}

DEFMMETHOD(Show)
{
	GETDATA;

	ULONG rc = DOSUPER;

	set(_win(obj), MUIA_Window_ActiveObject, data->str);

	return rc;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Show)
DECTMETHOD(ChooseTitleGroup_Add)
DECTMETHOD(ChooseTitleGroup_Cancel)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, choosetitlegroupclass)
