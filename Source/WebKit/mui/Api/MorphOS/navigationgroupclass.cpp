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

#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"

struct Data
{
	Object *bgroup;
	Object *sgroup;
	Object *back;
	Object *forward;
	Object *reload;
	Object *stop;
	Object *home;
	Object *inspector;
};

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		ULONG tdata = tag->ti_Data;

		switch (tag->ti_Tag)
		{
			case MA_OWBApp_ToolButtonType:
			{
				APTR backforwardlist = 0;
				
				if(data->back)
				{
					backforwardlist	= (APTR) getv(data->back, MA_HistoryButton_List);
				}

				DoMethod(data->bgroup, MUIM_Group_InitChange);

				FORCHILD(data->bgroup, MUIA_Group_ChildList)
				{
					DoMethod(data->bgroup, OM_REMMEMBER, child);
					MUI_DisposeObject((Object *) child);
				}
				NEXTCHILD

				data->back    = create_historybutton(GSI(MSG_NAVIGATIONGROUP_BACK),    "PROGDIR:resource/back.png", MV_HistoryButton_Type_Backward, tdata);
				data->forward = create_historybutton(GSI(MSG_NAVIGATIONGROUP_FORWARD), "PROGDIR:resource/forward.png", MV_HistoryButton_Type_Forward, tdata);
				data->reload  = create_toolbutton(GSI(MSG_NAVIGATIONGROUP_RELOAD),     "PROGDIR:resource/reload.png", tdata);
				data->stop    = create_toolbutton(GSI(MSG_NAVIGATIONGROUP_STOP),       "PROGDIR:resource/stop.png", tdata);
				data->home    = create_toolbutton(GSI(MSG_NAVIGATIONGROUP_HOME),       "PROGDIR:resource/home.png", tdata);
				if(getv(app, MA_OWBApp_EnableInspector))
					data->inspector	= create_toolbutton(GSI(MSG_NAVIGATIONGROUP_INSPECT), "PROGDIR:resource/inspectorIcon.png", tdata);

				DoMethod(data->bgroup, OM_ADDMEMBER, data->back);
				DoMethod(data->bgroup, OM_ADDMEMBER, data->forward);
				DoMethod(data->bgroup, OM_ADDMEMBER, data->reload);
				DoMethod(data->bgroup, OM_ADDMEMBER, data->stop);
				DoMethod(data->bgroup, OM_ADDMEMBER, data->home);

				if(getv(app, MA_OWBApp_EnableInspector))
					DoMethod(data->bgroup, OM_ADDMEMBER, data->inspector);

				DoMethod(data->bgroup, MUIM_Group_ExitChange);

				DoMethod((Object *) obj, MUIM_MultiSet, MA_ToolButton_Disabled, TRUE, data->back, data->forward, data->reload, data->stop, NULL);

				DoMethod(data->back,    	MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_Back);
				DoMethod(data->forward, 	MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_Forward);
				DoMethod(data->reload,  	MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 2, MM_OWBWindow_Reload, NULL);
				DoMethod(data->stop,    	MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_Stop);
				DoMethod(data->home,    	MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_Home);
				if(getv(app, MA_OWBApp_EnableInspector))
					DoMethod(data->inspector, MUIM_Notify, MA_ToolButton_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_InspectPage);

				if(backforwardlist)
				{
					set(obj, MA_Navigation_BackForwardList, backforwardlist);
				}

				break;
			}

			case MA_Navigation_BackEnabled:
				set(data->back, MA_ToolButton_Disabled, !tdata);
				break;

			case MA_Navigation_ForwardEnabled:
				set(data->forward, MA_ToolButton_Disabled, !tdata);
				break;

			case MA_Navigation_ReloadEnabled:
				set(data->reload, MA_ToolButton_Disabled, !tdata);
				break;

			case MA_Navigation_StopEnabled:
				set(data->stop, MA_ToolButton_Disabled, !tdata);
				break;

			case MA_Navigation_BackForwardList:
				set(data->back,    MA_HistoryButton_List, tdata);
				set(data->forward, MA_HistoryButton_List, tdata);
				break;
		}
	}
}

DEFNEW
{
	Object *bgroup, *sgroup;
	ULONG type = getv(app, MA_OWBApp_ToolButtonType);

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Group_Horiz, TRUE,
			Child, bgroup = HGroup, End,
			Child, sgroup = RectangleObject, End,
			Child, NewObject(gettransferanimclass(), NULL, TAG_DONE),
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->bgroup = bgroup;
		data->sgroup = sgroup;
		
		set(obj, MA_OWBApp_ToolButtonType, type);
	}

	return (ULONG)obj;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

BEGINMTABLE
DECNEW
DECSET
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, navigationgroupclass)
