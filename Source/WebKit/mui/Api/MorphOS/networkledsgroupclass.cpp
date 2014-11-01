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

/* OWB */
#include "config.h"
#include "GraphicsContext.h"
#include <Api/WebFrame.h>
#include <Api/WebView.h>
#include <wtf/text/CString.h>
#include "Page.h"
#include "DOMImplementation.h"
#include "Editor.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "Timer.h"
#include <wtf/MainThread.h>
#include "ResourceHandleManager.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"
#include <FrameLoader.h>
#include <unistd.h>
#include <cstdio>

#include <clib/macros.h>
#include <mui/Lamp_mcc.h>
#include "gui.h"
#undef String

/******************************************************************
 * networkledsgroupclass
 *****************************************************************/

using namespace WebCore;

struct LedState
{
	APTR job;
	Object *led;
};

struct Data
{
	int count;
	struct LedState *leds;
};

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_NetworkLedsGroup_Count:
		{
			int i;

			DoMethod((Object *)obj, MUIM_Group_InitChange);

			if(data->leds)
			{
				for(i = 0; i < data->count; i++)
				{
					if(data->leds[i].led)
					{
						DoMethod((Object *)obj, OM_REMMEMBER, data->leds[i].led);
						MUI_DisposeObject(data->leds[i].led);
					}
				}

				free(data->leds);
			}

			data->count = (int) tag->ti_Data;
			data->leds = (struct LedState *) malloc(data->count*sizeof(struct LedState));

			for(i = 0; i < data->count; i++)
			{
				Object *led = LampObject,
								MUIA_Lamp_Type, MUIV_Lamp_Type_Big,
								MUIA_Lamp_ColorType, MUIV_Lamp_ColorType_Color,
								MUIA_Lamp_Color, MUIV_Lamp_Color_Off,
								End;

				data->leds[i].led = led;
				data->leds[i].job = NULL;

				DoMethod((Object *)obj, OM_ADDMEMBER, data->leds[i].led);
			}

			DoMethod((Object *)obj, MUIM_Group_ExitChange);
		}
		break;
	}
	NEXTTAG
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_Group_Rows, 2,
			MUIA_Group_Spacing, 1,
		    TAG_MORE, INITTAGS);

	if (obj)
	{
		set(obj, MA_NetworkLedsGroup_Count, ResourceHandleManager::maxConnections());

		DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 2, MM_OWBApp_OpenWindow, MV_OWB_Window_Network);

		return (ULONG)obj;
	}

	return(0);
}

DEFDISP
{
	GETDATA;

	free(data->leds);

	return DOSUPER;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return DOSUPER;
}


DEFSMETHOD(Network_AddJob)
{
	int i = 0;
	GETDATA;

	while(i < data->count && data->leds[i].job)
	{
		i++;
	}

	if(i < data->count)
	{
		set(data->leds[i].led, MUIA_Lamp_Color, MUIV_Lamp_Color_Connecting);
		data->leds[i].job = msg->job;
	}

	return 0;
}

DEFSMETHOD(Network_UpdateJob)
{
	int i = 0;
	ResourceHandle *job = NULL;
	GETDATA;

	while(i < data->count)
	{
		if(msg->job == data->leds[i].job)
		{
			job = (ResourceHandle *) msg->job;
			break;
		}
		i++;
	}

	if(job)
	{
		ULONG color;
		ResourceHandleInternal *d = (ResourceHandleInternal*) job->getInternal();

		switch(d->m_state)
		{
			default:
			case STATUS_CONNECTING:
				color = MUIV_Lamp_Color_Connecting;
				break;
			case STATUS_WAITING_DATA:
				color = MUIV_Lamp_Color_Processing;
				break;
			case STATUS_RECEIVING_DATA:
				color = MUIV_Lamp_Color_ReceivingData;
				break;
			case STATUS_SENDING_DATA:
				color = MUIV_Lamp_Color_SendingData;
				break;

		}

		set(data->leds[i].led, MUIA_Lamp_Color, color);
		data->leds[i].job = job;
	}

	return 0;
}

DEFSMETHOD(Network_RemoveJob)
{
	int i = 0;
	GETDATA;

	while(i < data->count && data->leds[i].job != msg->job)
	{
		i++;
	}

	if(i < data->count)
	{
		set(data->leds[i].led, MUIA_Lamp_Color, MUIV_Lamp_Color_Off);
		data->leds[i].job = NULL;
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECSMETHOD(Network_AddJob)
DECSMETHOD(Network_UpdateJob)
DECSMETHOD(Network_RemoveJob)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, networkledsgroupclass)
