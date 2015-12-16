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

#include "config.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include "URL.h"

#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"
#include "utils.h"

#define CY_SEARCH_ID MAKE_ID('C','S','R','H')

using namespace WebCore;

struct Data
{
	Object *pop_search;
	Object *cy_search;
	Object *bt_search;
	Object *gr_search;

	char **labels;
	char **requests;
	char **shortcuts;

	String url;
};

static void	doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MA_SearchBarGroup_SearchButton:
			{
				if(tag->ti_Data)
				{
					if(!data->bt_search)
					{
						data->bt_search = (Object *) NewObject(gettoolbuttonclass(), NULL,
																MA_ToolButton_Text, NULL,
																MA_ToolButton_Image, "PROGDIR:resource/search.png",
																MA_ToolButton_Type, MV_ToolButton_Type_Icon,
																MA_ToolButton_Frame, /*getv(app, MA_OWBApp_ShowButtonFrame)*/0 ? MV_ToolButton_Frame_Button : MV_ToolButton_Frame_None,
																MA_ToolButton_Background, /*getv(app, MA_OWBApp_ShowButtonFrame)*/0 ? MV_ToolButton_Background_Button :MV_ToolButton_Background_Parent,
																TAG_DONE);
						if(data->bt_search)
						{
							DoMethod((Object *) obj, MUIM_Group_InitChange);
							DoMethod((Object *) obj, MUIM_Group_AddTail, data->bt_search);
							DoMethod((Object *) obj, MUIM_Group_ExitChange);

							DoMethod(data->bt_search, MUIM_Notify, MA_ToolButton_Pressed, FALSE, obj, 3, MM_OWBWindow_LoadURL, NULL, NULL);
						}
					}
				}
				else
				{
					if(data->bt_search)
					{
						DoMethod((Object *) obj, MUIM_Group_InitChange);
						DoMethod((Object *) obj, MUIM_Group_Remove, data->bt_search);
						DoMethod((Object *) obj, MUIM_Group_ExitChange);

						MUI_DisposeObject(data->bt_search);
						data->bt_search = NULL;
					}
				}
				break;
			}
		}
	}
}

DEFNEW
{
	char **labels = NULL;
	Object *pop_search, *cy_search;
	Object *searchwindow = (Object *) getv(app, MA_OWBApp_SearchManagerWindow);
	Object *searchgroup = NULL;

	if(searchwindow)
	{
		searchgroup = (Object *) getv(searchwindow, MA_SearchManagerWindow_Group);

		if(searchgroup)
		{
			Vector<String> *slabels = (Vector<String> *) getv(searchgroup, MA_SearchManagerGroup_Labels);
			labels = (char **) malloc((slabels->size() + 1)*sizeof(char *));

			if(labels)
			{
				size_t i;
				for(i = 0; i < slabels->size(); i++)
				{
					labels[i] = strdup((*slabels)[i].latin1().data());
				}
				labels[i] = NULL;
			}
		}
	}

	if(labels == NULL)
	{
		return NULL;
	}

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		MUIA_Weight, 25,
		Child, cy_search = (Object *) MakePrefsCycle(NULL, labels, CY_SEARCH_ID),
		Child, pop_search = (Object *) NewObject(getsuggestpopstringclass(), NULL, TAG_DONE),
		TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->gr_search  = searchgroup;
		data->pop_search = pop_search;
		data->bt_search  = NULL;
		data->cy_search  = cy_search;
		data->labels     = labels;
		data->requests   = NULL;
		data->shortcuts  = NULL;

        doset(obj, data, INITTAGS);

		DoMethod(obj, MUIM_MultiSet, MUIA_Weight, 0, cy_search, NULL);

		DoMethod(searchgroup, MUIM_Notify, MA_SearchManagerGroup_Changed, MUIV_EveryTime, obj, 1, MM_SearchBarGroup_Update);
		DoMethod((Object *) getv(pop_search, MUIA_Popstring_String), MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, obj, 3, MM_OWBWindow_LoadURL, NULL, NULL);
	}

	return (ULONG)obj;
}

DEFDISP
{
	GETDATA;
	char **ptr;

	if(data->gr_search)
	{
		DoMethod(data->gr_search, MUIM_KillNotifyObj, MA_SearchManagerGroup_Changed, obj);
	}

	ptr	= data->requests;

	if(ptr)
	{
		while(*ptr)
		{
			free(*ptr);
			ptr++;
		}
		free(data->requests);
	}

	ptr = data->labels;

	if(ptr)
	{
		while(*ptr)
		{
			free(*ptr);
			ptr++;
		}
		free(data->labels);
	}

	ptr = data->shortcuts;

	if(ptr)
	{
		while(*ptr)
		{
			free(*ptr);
			ptr++;
		}
		free(data->shortcuts);
	}

	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MUIA_String_Contents:
			return GetAttr(MUIA_String_Contents, data->pop_search, (ULONGPTR)msg->opg_Storage);

		case MA_SearchBarGroup_SearchButton:
			*msg->opg_Storage = data->bt_search != NULL;
			return TRUE;
	}

	return DOSUPER;
}

DEFTMETHOD(SearchBarGroup_Update)
{
	GETDATA;

	if(data->gr_search)
	{
		/* */
		Vector<String> *slabels = (Vector<String> *) getv(data->gr_search, MA_SearchManagerGroup_Labels);
		char **ptr = data->labels;

		if(ptr)
		{
			while(*ptr)
			{
				free(*ptr);
				ptr++;
			}
			free(data->labels);
		}
		
		data->labels = (char **) malloc((slabels->size() + 1)*sizeof(char *));

		if(data->labels)
		{
			size_t i;
			for(i = 0; i < slabels->size(); i++)
			{
				data->labels[i] = strdup((*slabels)[i].latin1().data());
			}
			data->labels[i] = NULL;
		}
		/* */

		if(data->labels)
		{
	        DoMethod(obj, MUIM_Group_InitChange);
			DoMethod(obj, MUIM_Group_Remove, data->cy_search);
			set(data->cy_search, MUIA_Cycle_Entries, data->labels);
			set(data->cy_search, MUIA_Cycle_Active, 0);
			DoMethod(obj, MUIM_Group_AddHead, data->cy_search);
			set(data->cy_search, MUIA_CycleChain, 1);
	        DoMethod(obj, MUIM_Group_ExitChange);
		}
	}

	return 0;
}

DEFSMETHOD(OWBWindow_LoadURL)
{
	GETDATA;

	STRPTR str = strdup((STRPTR)getv(data->pop_search, MUIA_String_Contents));
	ULONG idx  = getv(data->cy_search, MUIA_Cycle_Active);

	DoMethod(data->pop_search, MM_SuggestPopString_Abort);

	if(str)
	{
		if(data->gr_search)
		{
			/* not really very smart to rebuild that each time :) */
			Vector<String> *srequests = (Vector<String> *) getv(data->gr_search, MA_SearchManagerGroup_Requests);
			char **ptr = data->requests;

			if(ptr)
			{
				while(*ptr)
				{
					free(*ptr);
					ptr++;
				}
				free(data->requests);
			}

			data->requests = (char **) malloc((srequests->size() + 1)*sizeof(char *));

			if(data->requests)
			{
				size_t i;
				for(i = 0; i < srequests->size(); i++)
				{
					data->requests[i] = strdup((*srequests)[i].latin1().data());
				}
				data->requests[i] = NULL;
			}
			/* */

			if(data->requests)
			{
				STRPTR fmt = (STRPTR)data->requests[idx];
				STRPTR converted = local_to_utf8(str);

				if(converted)
				{
					String searchString = String::fromUTF8(converted).stripWhiteSpace();
					searchString.stripWhiteSpace();
					String encoded = encodeWithURLEscapeSequences(searchString);
					encoded.replace("+", "%2B");
				    encoded.replace("%20", "+");
					encoded.replace("&", "%26");
					data->url = String::format(fmt, encoded.utf8().data());

					DoMethod(_win(obj), MM_OWBWindow_LoadURL, data->url.utf8().data(), NULL);

					free(converted);
				}
			}
		}

		free(str);
	}

	return 0;
}

DEFSMETHOD(SearchBarGroup_LoadURLFromShortcut)
{
	GETDATA;
	ULONG res = FALSE;

	if(data->gr_search)
	{
		Vector<String> *sshortcuts = (Vector<String> *) getv(data->gr_search, MA_SearchManagerGroup_Shortcuts);

		for(size_t i = 0; i < sshortcuts->size(); i++)
		{
			if((*sshortcuts)[i] == msg->shortcut)
			{
                Vector<String> *srequests = (Vector<String> *) getv(data->gr_search, MA_SearchManagerGroup_Requests);

				STRPTR fmt = strdup((*srequests)[i].latin1().data());
				STRPTR converted = local_to_utf8(msg->string);

				if(converted && fmt)
				{
					String searchString = String::fromUTF8(converted).stripWhiteSpace();
					String encoded = encodeWithURLEscapeSequences(searchString);
					encoded.replace("+", "%2B");
				    encoded.replace("%20", "+");
					encoded.replace("&", "%26");
					data->url = String::format(fmt, encoded.utf8().data());

					DoMethod(_win(obj), MM_OWBWindow_LoadURL, data->url.utf8().data(), NULL);

					res = TRUE;
				}

				free(fmt);
				free(converted);
			}
		}
	}

	return res;
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
	GETDATA;
	char *url = (char *) getv(msg->obj, MA_OWB_URL);

	if(url)
	{
		set(_win(obj), MUIA_Window_Activate, TRUE);
		set(data->pop_search, MUIA_String_Contents, url);
	}

	return DOSUPER;
}

DEFMMETHOD(DragFinish)
{
	GETDATA;
	set(_win(obj), MUIA_Window_ActiveObject, data->pop_search);
	return DOSUPER;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECSMETHOD(OWBWindow_LoadURL)
DECSMETHOD(SearchBarGroup_LoadURLFromShortcut)
DECTMETHOD(SearchBarGroup_Update)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(DragFinish)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, searchbargroupclass)
