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

#include <string.h>
#include <stdio.h>
#include <clib/debug_protos.h>

#include "gui.h"


struct Data
{
};

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
			MA_ToolButton_Text,  GSI(MSG_TOOLBUTTON_NEWTAB_LABEL),
			MA_ToolButton_Image, "PROGDIR:resource/newtab.png",
			MA_ToolButton_Type,  MV_ToolButton_Type_Icon,
			MA_ToolButton_Frame, getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Frame_Button : MV_ToolButton_Frame_None,
			MA_ToolButton_Background, getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Background_Button :MV_ToolButton_Background_Parent,
			TAG_MORE, INITTAGS,
			TAG_DONE);

	return (ULONG)obj;
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
	char *url = (char *) getv(msg->obj, MA_OWB_URL);

	if(url)
	{
		DoMethod(app, MM_OWBApp_AddPage, url, FALSE, TRUE, NULL, _win(obj), FALSE, TRUE);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASSPTR_NC(toolbuttonclass, toolbutton_newtabclass)
