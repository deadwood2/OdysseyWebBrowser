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

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"

struct Data
{
	Object *bookmarkgroup;
};

DEFNEW
{
	Object *bookmarkgroup;

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Window_ID, MAKE_ID('O','B','M','S'),
			MUIA_Window_Title, GSI(MSG_BOOKMARKWINDOW_BOOKMARKS),
			MUIA_Window_NoMenus, TRUE,
			WindowContents, VGroup,
				Child, bookmarkgroup = (Object *) NewObject(getbookmarkgroupclass(), NULL, End,
				End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->bookmarkgroup = bookmarkgroup;

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
	}

	return (ULONG)obj;
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_OWB_WindowType:
		{
			*msg->opg_Storage = (ULONG) MV_OWB_Window_Bookmarks;
		}
		return TRUE;
	}

	return DOSUPER;
}

/* Forwarders */
DEFSMETHOD(Bookmarkgroup_AddLink)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

DEFSMETHOD(Bookmarkgroup_Update)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

DEFSMETHOD(Bookmarkgroup_LoadHtml)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

DEFSMETHOD(Bookmarkgroup_BuildMenu)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

DEFSMETHOD(Bookmarkgroup_RegisterQLGroup)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

DEFSMETHOD(Bookmarkgroup_UnRegisterQLGroup)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

DEFSMETHOD(Bookmarkgroup_RemoveQuickLink)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

DEFSMETHOD(Bookmarkgroup_ContainsURL)
{
	GETDATA;
	return DoMethodA(data->bookmarkgroup, (_Msg_*)msg);
}

BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(Bookmarkgroup_Update)
DECSMETHOD(Bookmarkgroup_LoadHtml)
DECSMETHOD(Bookmarkgroup_AddLink)
DECSMETHOD(Bookmarkgroup_BuildMenu)
DECSMETHOD(Bookmarkgroup_RegisterQLGroup)
DECSMETHOD(Bookmarkgroup_UnRegisterQLGroup)
DECSMETHOD(Bookmarkgroup_RemoveQuickLink)
DECSMETHOD(Bookmarkgroup_ContainsURL)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window,bookmarkwindowclass)
