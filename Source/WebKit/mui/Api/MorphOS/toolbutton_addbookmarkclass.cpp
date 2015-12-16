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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <clib/debug_protos.h>

#include "gui.h"


struct Data
{
};

static void doset(Object * obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_ToolButton_AddBookmark_IsBookmark:
		{
			ULONG isbookmark = tag->ti_Data;
			set(obj, MA_ToolButton_Image, isbookmark ? "PROGDIR:resource/addbookmark_added.png" : "PROGDIR:resource/addbookmark_notadded.png");
		}
		break;

	}
	NEXTTAG
}

DEFNEW
{
	ULONG type       = getv(app, MA_OWBApp_ToolButtonType) == MV_ToolButton_Type_Text ? MV_ToolButton_Type_Text : MV_ToolButton_Type_Icon;
	ULONG frame      = type == MV_ToolButton_Type_Text ? MV_ToolButton_Frame_Button : (getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Frame_Button : MV_ToolButton_Frame_None);
	ULONG background = type == MV_ToolButton_Type_Text ? MV_ToolButton_Background_Button : (getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Background_Button : MV_ToolButton_Background_Parent);

	obj = (Object *) DoSuperNew(cl, obj,
			MA_ToolButton_Text, GSI(MSG_TOOLBUTTON_ADDBOOKMARK_ADD),
			MA_ToolButton_Image, "PROGDIR:resource/addbookmark_notadded.png",
			MA_ToolButton_Type,       type,
			MA_ToolButton_Frame,      frame,
			MA_ToolButton_Background, background,
            MUIA_CycleChain, 1,
			TAG_MORE, INITTAGS,
			TAG_DONE);

	return (ULONG)obj;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);
	return DOSUPER;
}

DEFMMETHOD(DragQuery)
{
	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if (type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_Tab || type == MV_OWB_ObjectType_URL)
	{
		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	char *url = strdup((char *) getv(msg->obj, MA_OWB_URL));

	if(url)
	{
		LONG type = getv(msg->obj, MA_OWB_ObjectType);

		if(type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_URL)
		{
			// Means we drag a link, so we ask for title
			DoMethod(app, MUIM_Application_PushMethod, _win(obj), 4, MM_OWBWindow_InsertBookmarkAskTitle, strdup(url), NULL, FALSE);
		}
		else if(type == MV_OWB_ObjectType_Tab)
		{
			// The title can be retrieved from tab
			char *title = (char *) getv(msg->obj, MA_OWB_Title);
			DoMethod(app, MM_Bookmarkgroup_AddLink, title ? title : GSI(MSG_TOOLBUTTON_ADDBOOKMARK_NO_TITLE), NULL, url, TRUE, TRUE, FALSE);
		}

		free(url);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECSET
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASSPTR_NC(toolbuttonclass, toolbutton_addbookmarkclass)
