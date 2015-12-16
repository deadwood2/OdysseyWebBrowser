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
        MUIA_List_Format, "MIW=-1 MAW=-2 BAR,",
		MUIA_List_Title, TRUE,
		MUIA_ContextMenu, TRUE,
		MUIA_CycleChain, TRUE,
		TAG_MORE, INITTAGS
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
	ResourceHandle* job = (ResourceHandle*) msg->entry;

	if (job)
	{
		STATIC char buf1[2048];
		STATIC char buf2[256];
		ResourceHandleInternal* d = job->getInternal();

		snprintf(buf1, sizeof(buf1), "%s", (!job->firstRequest().isNull() && !job->firstRequest().url().isEmpty()) ? job->firstRequest().url().string().utf8().data() : "");

		switch(d->m_state)
		{
			case STATUS_CONNECTING:
			{
				snprintf(buf2, sizeof(buf2), "%s", GSI(MSG_NETWORKWINDOW_STATUS_CONNECTING));
				break;
			}

			case STATUS_WAITING_DATA:
			{
				snprintf(buf2, sizeof(buf2), "%s", GSI(MSG_NETWORKWINDOW_STATUS_WAITING_FOR_DATA));
				break;
			}

			case STATUS_RECEIVING_DATA:
			{
				char received[128], total[128];
				format_size(received, sizeof(received), d->m_received);
				format_size(total, sizeof(total), d->m_totalSize);
				snprintf(buf2, sizeof(buf2), GSI(MSG_NETWORKWINDOW_STATUS_RECEIVING_DATA), received, total);
				break;
			}

			case STATUS_SENDING_DATA:
			{
				char sent[128], total[128];
				format_size(sent, sizeof(sent), d->m_bodyDataSent);
				format_size(total, sizeof(total), d->m_bodySize);
				snprintf(buf2, sizeof(buf2), GSI(MSG_NETWORKWINDOW_STATUS_SENDING_DATA), sent, total);
				break;
			}
		}

		msg->array[0] = buf1;
		msg->array[1] = buf2;
	}
	else
	{
		msg->array[0] = GSI(MSG_NETWORKWINDOW_URL);
		msg->array[1] = GSI(MSG_NETWORKWINDOW_STATE);
	}

	return TRUE;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUI_List_TestPos_Result res;
	ResourceHandle* e;

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
					MUIA_Family_Child, MenuObjectT(GSI(MSG_NETWORKWINDOW_URL)),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, GSI(MSG_CONSOLELIST_COPY), // XXX: use own msg id
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
	ResourceHandle *e;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ULONG *)&e);

	if(e)
	{
		ULONG udata = muiUserData(msg->item);

		switch(udata)
		{
			case POPMENU_OPEN_COPY:
			{
				copyTextToClipboard((char *) ((!e->firstRequest().isNull() && !e->firstRequest().url().isEmpty()) ? e->firstRequest().url().string().utf8().data() : ""), false);
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

DECSUBCLASS_NC(MUIC_List, networklistclass)
