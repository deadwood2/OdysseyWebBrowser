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
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"
#include "ResourceHandleManager.h"
#include "ResourceRequest.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"
#include "utils.h"

using namespace WebCore;

struct Data
{
	Object *lv_transfers;
};

DEFNEW
{
	Object *lv_transfers;
	Object *cancel, *cancel_all;

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Window_ID, MAKE_ID('W','T','R','A'),
			MUIA_Window_Title, GSI(MSG_NETWORKWINDOW_TITLE),
			MUIA_Window_NoMenus, TRUE,
			WindowContents, VGroup,
				Child, VGroup,
					Child, lv_transfers = (Object *) NewObject(getnetworklistclass(), NULL,TAG_DONE),
					Child, HGroup,
						Child, cancel = (Object *) MakeButton(GSI(MSG_NETWORKWINDOW_ABORT)),
						Child, cancel_all = (Object *) MakeButton(GSI(MSG_NETWORKWINDOW_ABORT_ALL)),
					End,
				End,
			End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->lv_transfers = lv_transfers;

		DoMethod(obj,        MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 3, MUIM_Set, MUIA_Window_Open, FALSE);
		DoMethod(cancel,     MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MM_NetworkWindow_Cancel, 0);
		DoMethod(cancel_all, MUIM_Notify, MUIA_Pressed, FALSE, obj, 3, MM_NetworkWindow_Cancel, 1);
	}

	return (IPTR)obj;
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_OWB_WindowType:
		{
			*msg->opg_Storage = (ULONG) MV_OWB_Window_Network;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFMMETHOD(List_Redraw)
{
	GETDATA;
	return DoMethodA(data->lv_transfers, (_Msg_*)msg);
}

DEFSMETHOD(Network_AddJob)
{
	GETDATA;
	DoMethod(data->lv_transfers, MUIM_List_InsertSingle, msg->job);
	return 0;
}

DEFSMETHOD(Network_RemoveJob)
{
	GETDATA;
	ResourceHandle *job = NULL;
	ULONG i = 0;

	do
	{
		DoMethod(data->lv_transfers, MUIM_List_GetEntry, i, (ResourceHandle **) &job);

		if ((APTR) job == msg->job)
		{
			DoMethod(data->lv_transfers, MUIM_List_Remove, i);
			break;
		}

		i++;
	}
	while (job);

	return 0;
}

DEFSMETHOD(Network_UpdateJob)
{
	GETDATA;
   
	DoMethod(data->lv_transfers, MUIM_List_Redraw, MUIV_List_Redraw_Entry, msg->job);

	return 0;
}

DEFSMETHOD(NetworkWindow_Cancel)
{
	GETDATA;

	ResourceHandleManager *sharedResourceHandleManager = ResourceHandleManager::sharedInstance();
	
	if (msg->all)
	{
		ResourceHandle *job = NULL;

		do
		{
			DoMethod(data->lv_transfers, MUIM_List_GetEntry, 0, (ResourceHandle **) &job);

			if (job)
			{
				DoMethod(data->lv_transfers, MUIM_List_Remove, MUIV_List_Remove_First);
				sharedResourceHandleManager->cancel(job);
			}
		}
		while (job);
	}
	else
	{
		ResourceHandle *job = NULL;
		DoMethod(data->lv_transfers, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ResourceHandle **) &job);

		if(job)
		{
			DoMethod(data->lv_transfers, MUIM_List_Remove, MUIV_List_Remove_Active);
			sharedResourceHandleManager->cancel(job);
		}
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(Network_AddJob)
DECSMETHOD(Network_UpdateJob)
DECSMETHOD(Network_RemoveJob)
DECSMETHOD(NetworkWindow_Cancel)
DECMMETHOD(List_Redraw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, networkwindowclass)
