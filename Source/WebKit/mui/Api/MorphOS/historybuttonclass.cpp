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
#include "GraphicsContext.h"
#include <Api/WebFrame.h>
#include <Api/WebView.h>
#include <wtf/text/CString.h>
#include "Page.h"
#include "HistoryItem.h"
#include "DOMImplementation.h"
#include "Editor.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "Timer.h"
#include <wtf/MainThread.h>
#include "WebHistoryItem.h"
#include "WebBackForwardList.h"
#include <FrameLoader.h>
#include <unistd.h>
#include <cstdio>

#include <clib/macros.h>

#include "gui.h"
#include "utils.h"
#undef String

/******************************************************************
 * historybuttonclass
 *****************************************************************/

Object * create_historybutton(CONST_STRPTR text, CONST_STRPTR imagepath, ULONG type, ULONG buttontype)
{
	return (Object *) NewObject(gethistorybuttonclass(), NULL,
								MA_ToolButton_Text, text,
								MA_ToolButton_Image, imagepath,
								MA_ToolButton_Type, buttontype,
								MA_ToolButton_Frame, getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Frame_Button : MV_ToolButton_Frame_None,
								MA_ToolButton_Background, getv(app, MA_OWBApp_ShowButtonFrame) ? MV_ToolButton_Background_Button :MV_ToolButton_Background_Parent,
								MA_HistoryButton_Type, type,
								MUIA_CycleChain, 1,
								TAG_DONE);
}

using namespace WebCore;

struct Data
{
	ULONG type;
	WebBackForwardList* backforwardlist;
	Object *contextmenu;
};


static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_HistoryButton_List:
		{
			data->backforwardlist = (WebBackForwardList *) tag->ti_Data;
		}
		break;

		case MA_HistoryButton_Type:
		{
			data->type = tag->ti_Data;
		}
		break;
	}
	NEXTTAG
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
    		MUIA_ContextMenu, 1,
		    TAG_MORE, INITTAGS);

	if (obj)
	{
		GETDATA;

		doset(obj, data, INITTAGS);

		return (ULONG)obj;
	}

	return(0);
}

DEFDISP
{
	GETDATA;

	if (data->contextmenu)
	{
		MUI_DisposeObject(data->contextmenu);
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_HistoryButton_List:
		{
			*msg->opg_Storage = (ULONG) data->backforwardlist;
		}
		return TRUE;
	}

	return DOSUPER;
}


DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;
	Object *menu = NULL;

	if(getv(obj, MA_ToolButton_Disabled))
	{
		return 0;
	}

	if (data->contextmenu)
	{
		MUI_DisposeObject(data->contextmenu);
		data->contextmenu = NULL;
	}

	if(!data->backforwardlist)
	{
		return 0;
	}

	data->contextmenu = (Object *) MenustripObject,
									Child, menu = (Object *) NewObject(getmenuclass(), NULL, MUIA_Menu_Title, strdup(GSI(MSG_HISTORYBUTTON_MENU_TITLE)), TAG_DONE),
								    End;

	if(data->contextmenu)
	{
		int firstItemIndex, lastItemIndex;
		int di = 1;

		if(data->type == MV_HistoryButton_Type_Backward)
		{
			firstItemIndex = 0;
			lastItemIndex = -data->backforwardlist->backListCount();
			di = -1;

		}
		else if(data->type == MV_HistoryButton_Type_Forward)
		{
			firstItemIndex = 0;
            lastItemIndex = data->backforwardlist->forwardListCount();
			di = 1;
		}
		else
		{
			return (ULONG) data->contextmenu;
		}

		for (int i = firstItemIndex ; (data->type == MV_HistoryButton_Type_Backward) ? i >= lastItemIndex : i <= lastItemIndex ; i+=di)
		{
			char *title = (char *) data->backforwardlist->itemAtIndex(i)->title();
			char *tmp = title ? utf8_to_local(title) : strdup(GSI(MSG_HISTORYBUTTON_NO_TITLE));

			if(tmp)
			{
				char *prefix = (char *) (i == firstItemIndex ? "\033b" : "");
				char *label = (char *) malloc(strlen(tmp) + strlen(prefix) + 1);
				
				if(label)
				{
					sprintf(label, "%s%s", prefix, tmp);

					Object *item = (Object *) NewObject(getmenuitemclass(), NULL,
													    MUIA_Menuitem_Title, (ULONG) label,
											            MUIA_UserData, (ULONG) data->backforwardlist->itemAtIndex(i),
														MA_MenuItem_FreeUserData, FALSE, /* Don't free it, thx */
														TAG_DONE);
					if(item)
					{
						DoMethod(menu, OM_ADDMEMBER, item);
					}
				}
			}

			free(tmp);
			free(title);
	    }
	}

	return (ULONG) data->contextmenu;
}

DEFMMETHOD(ContextMenuChoice)
{
	if(msg->item)
	{
		Object *browser = (Object *) getv(_win(obj), MA_OWBWindow_ActiveBrowser);

		if(browser)
		{
			BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
			widget->webView->clearMainFrameName();
			widget->webView->goToBackForwardItem((WebHistoryItem *) getv(msg->item, MUIA_UserData));
			widget->expose = true;
		}
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
ENDMTABLE

DECSUBCLASSPTR_NC(toolbuttonclass, historybuttonclass)
