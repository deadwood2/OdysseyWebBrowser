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
#include <wtf/text/CString.h>
#include "WebHistory.h"
#include "WebHistoryItem.h"

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mui/Listtree_mcc.h>
#include <clib/macros.h>

#include "gui.h"

#define D(x)

struct Data
{
	Object *lv_history;
	Object *findgroup;
};

DEFNEW
{
	Object *lv_history;
	Object *closebutton, *findgroup;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, HGroup,
				Child, closebutton = ImageObject,
                        MUIA_Frame, MUIV_Frame_ImageButton,
						MUIA_CustomBackfill, TRUE,
						MUIA_InputMode, MUIV_InputMode_RelVerify,
						MUIA_Image_Spec, MUII_Close,
						End,
				Child, TextObject,
						MUIA_Text_SetMax, FALSE,
						MUIA_Text_SetMin, FALSE,
						MUIA_Text_Contents, GSI(MSG_HISTORYPANELGROUP_HISTORY),
						End,
				End,
		Child, lv_history = (Object *) NewObject(gethistorylisttreeclass(), NULL, TAG_DONE),
		Child, findgroup  = (Object *) NewObject(getfindtextclass(), NULL,
												 MUIA_ShowMe, TRUE,
												 MA_FindText_Closable, TRUE,
												 MA_FindText_ShowButtons, FALSE,
											 	 MA_FindText_ShowCaseSensitive, FALSE,
												 MA_FindText_ShowText, FALSE,
											 	 TAG_DONE),
		TAG_MORE, INITTAGS
		);

	if (obj)
	{
		GETDATA;
		data->lv_history = lv_history;
		data->findgroup  = findgroup;

		set(data->findgroup, MA_FindText_Target, data->lv_history);

		DoMethod(closebutton, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_RemoveHistoryPanel);
	}

	return ((ULONG)obj);
}

DEFDISP
{
	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;

	if(rc)
	{
		DoMethod(obj, MM_History_Load);
	}
	return rc;

}

DEFMMETHOD(Cleanup)
{
	DoMethod(obj, MM_History_Clear);
	return DOSUPER;
}

DEFTMETHOD(History_Load)
{
	GETDATA;
	WebHistory* history = WebHistory::sharedHistory();
	std::vector<WebHistoryItem *> historyList = *(history->historyList());

	set(data->lv_history, MUIA_List_Quiet, TRUE);

	DoMethod(data->lv_history, MUIM_List_Clear);

	for(unsigned int i = 0; i < historyList.size(); i++)
	{
		WebHistoryItem *webHistoryItem = historyList[i];

		if(webHistoryItem)
		{
			DoMethod(obj, MM_History_Insert, webHistoryItem);
		}
	}

	set(data->lv_history, MUIA_List_Quiet, FALSE);

	return 0;
}

DEFTMETHOD(History_Clear)
{
	GETDATA;
	WebHistory* history = WebHistory::sharedHistory();
	std::vector<WebHistoryItem *> historyList = *(history->historyList());

	set(data->lv_history, MUIA_List_Quiet, TRUE);

	for(unsigned int i = 0; i < historyList.size(); i++)
	{
		WebHistoryItem *webHistoryItem = historyList[i];

		if(webHistoryItem)
		{
			DoMethod(obj, MM_History_Remove, webHistoryItem);
		}
	}

	set(data->lv_history, MUIA_List_Quiet, FALSE);

	return 0;
}

DEFSMETHOD(History_Insert)
{
	GETDATA;
	WebHistoryItem *s = (WebHistoryItem *) msg->item;
	struct history_entry *hitem = (struct history_entry *) malloc(sizeof(*hitem));

	if(hitem)
	{
		hitem->webhistoryitem = s;

		hitem->faviconobj = NULL;
		hitem->faviconimg = NULL;

		DoMethod(data->lv_history, MUIM_List_InsertSingle, hitem, MUIV_List_Insert_Top);
	}

	return 0;
}

DEFSMETHOD(History_Remove)
{
	GETDATA;
	WebHistoryItem *s;
	struct history_entry *x;
	ULONG i;

	s = (WebHistoryItem *) msg->item;

	for (i = 0; ; i++)
	{
		DoMethod(data->lv_history, MUIM_List_GetEntry, i, &x);

		if (!x)
			break;

		if (s == x->webhistoryitem)
		{
			DoMethod(data->lv_history, MUIM_List_Remove, i);

			if(x->faviconobj)
			{
				MUI_DisposeObject((Object *) x->faviconobj);
			}
			free(x);

			break;
		}
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECTMETHOD(History_Load)
DECTMETHOD(History_Clear)
DECSMETHOD(History_Insert)
DECSMETHOD(History_Remove)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, historypanelgroupclass)
