/*
 * Copyright 2012 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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
#include "GraphicsContext.h"

#include <Calltips_mcc.h>
#include <clib/debug_protos.h>
#include "IntRect.h"

#include "gui.h"

#define D(x)

using namespace WebCore;

/******************************************************************
 * autofillpopupclass
 *****************************************************************/

struct Data
{
	Object *lv_suggestions;
	Object *source;
};

DEFNEW
{
	Object *lv_suggestions;
	Object *source              = (Object *) GetTagData(MA_AutofillPopup_Source, NULL, msg->ops_AttrList);
	IntRect *rect               = (IntRect *) GetTagData(MA_AutofillPopup_Rect, NULL, msg->ops_AttrList);
	Vector<String> *suggestions = (Vector<String> *) GetTagData(MA_AutofillPopup_Suggestions, NULL, msg->ops_AttrList);
	struct Rect32 r = { _mleft(source) + rect->x(), _mtop(source) + rect->y() + rect->height(), _mleft(source) + rect->maxX(), _mtop(source) + rect->y() + rect->height() + 100 };

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_RootObject,
			VGroup, MUIA_FixWidth, rect->width(),
				Child, lv_suggestions = (Object *) NewObject(getautofillpopuplistclass(), NULL, TAG_DONE),
				End,
		MUIA_Calltips_Source, source,
		MUIA_Calltips_Rectangle, &r,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;
		data->lv_suggestions = lv_suggestions;
		data->source = source;

		for(size_t i = 0; i < suggestions->size(); i++)
		{
			DoMethod(data->lv_suggestions, MUIM_List_InsertSingle, &((*suggestions)[i]), MUIV_List_Insert_Bottom);
		}

		DoMethod(data->lv_suggestions, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 3, MM_AutofillPopup_DidSelect, MUIV_TriggerValue, FALSE);
		DoMethod(data->lv_suggestions, MUIM_Notify, MUIA_List_DoubleClick, MUIV_EveryTime, obj, 3, MM_AutofillPopup_DidSelect, -1, TRUE);
	}
	return ((ULONG)obj);
}

DEFDISP
{
	return DOSUPER;
}

DEFSMETHOD(AutofillPopup_Update)
{
	GETDATA;

	Vector<String> *suggestions = (Vector<String> *) msg->suggestions;

	DoMethod(data->lv_suggestions, MUIM_List_Clear);

	for(size_t i = 0; i < suggestions->size(); i++)
	{
		DoMethod(data->lv_suggestions, MUIM_List_InsertSingle, &((*suggestions)[i]), MUIV_List_Insert_Bottom);
	}

	return 0;
}

DEFSMETHOD(AutofillPopup_DidSelect)
{
	GETDATA;

	String *value;

	if(msg->idx == -1)
	{
		msg->idx = getv(data->lv_suggestions, MUIA_List_Active);
	}

	DoMethod(data->lv_suggestions, MUIM_List_GetEntry, msg->idx, &value);

	if(value)
	{
		DoMethod(data->source, MM_OWBBrowser_Autofill_DidSelect, value);
	}

	if(msg->close)
	{
		DoMethod(app, MUIM_Application_PushMethod, data->source, 1, MM_OWBBrowser_Autofill_HidePopup);
	}

	return 0;
}

DEFSMETHOD(AutofillPopup_HandleNavigationEvent)
{
	GETDATA;
	ULONG handled = FALSE;

	if(getv(obj, MUIA_Window_Open))
	{
		switch(msg->event)
		{
			case MV_AutofillPopup_HandleNavigationEvent_Down:
				set(data->lv_suggestions, MUIA_List_Active, MUIV_List_Active_Down);
				handled = true;
				break;

			case MV_AutofillPopup_HandleNavigationEvent_Up:
				set(data->lv_suggestions, MUIA_List_Active, MUIV_List_Active_Up);
				handled = true;
				break;

			case MV_AutofillPopup_HandleNavigationEvent_Accept:
				DoMethod(obj, MM_AutofillPopup_DidSelect, -1, TRUE);
				handled = true;
				break;

			case MV_AutofillPopup_HandleNavigationEvent_Close:
				DoMethod(app, MUIM_Application_PushMethod, data->source, 1, MM_OWBBrowser_Autofill_HidePopup);
				handled = true;
				break;
		}
	}

	return handled;
}

BEGINMTABLE
DECNEW
DECDISP
DECSMETHOD(AutofillPopup_Update)
DECSMETHOD(AutofillPopup_DidSelect)
DECSMETHOD(AutofillPopup_HandleNavigationEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Calltips, autofillpopupclass)

