/*
 * Copyright 2010 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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
#include "FileIOLinux.h"

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clib/macros.h>

#include "gui.h"

#define D(x)

using namespace WebCore;

STATIC CONST CONST_STRPTR labels[] =
{
	"Google",
	"Aminet",
	"MorphOS",
	"YouTube",
	NULL
};

STATIC CONST CONST_STRPTR shortcuts[] =
{
	"g",
	"a",
	"m",
	"y",
	NULL
};

STATIC CONST CONST_STRPTR requests[] =
{
	"http://www.google.com/search?q=%s&ie=UTF-8&oe=UTF-8",
	"http://www.aminet.net/search.php?query=%s",
	"http://morphos-files.net/find.php?find=%s",
	"http://www.youtube.com/results?search_query=%s",
	NULL
};

struct Data
{
	ULONG changed;
	Vector<String> labels;
	Vector<String> requests;
	Vector<String> shortcuts;

	Object *bt_remove;
	Object *lv_entries;
	Object *st_label;
	Object *st_request;
	Object *st_shortcut;
};

static void load_searchengines(Object *obj, struct Data *data)
{
	Vector<String> lines;
	OWBFile *searchFile = new OWBFile("PROGDIR:Conf/searchengines.prefs");

    if (!searchFile)
		return;

	if (searchFile->open('r') == -1)
	{
		delete searchFile;
		return;
    }

	char *buffer = searchFile->read(searchFile->getSize());
	String fileBuffer = buffer;
	delete [] buffer;
    searchFile->close();
	delete searchFile;

	fileBuffer.split("\n", true, lines);

	set(data->lv_entries, MUIA_List_Quiet, TRUE);

	for(size_t i = 0; i < lines.size(); i++)
	{
		Vector<String> searchAttributes;
		lines[i].split("\1", true, searchAttributes);

		if(searchAttributes.size() >= 2) // 1.10 introduced a 3rd field
		{
			struct search_entry *se = (struct search_entry *) malloc(sizeof(*se));

			if (se)
			{
				se->label   = strdup(searchAttributes[0].latin1().data());
				se->request = strdup(searchAttributes[1].latin1().data());

				if(searchAttributes.size() == 3)
				{
					se->shortcut = strdup(searchAttributes[2].latin1().data());
				}
				else
				{
					se->shortcut = strdup("");
				}

				DoMethod(data->lv_entries, MUIM_List_InsertSingle, se, MUIV_List_Insert_Bottom);
			}
		}
	}

	set(data->lv_entries, MUIA_List_Quiet, FALSE);
}

void save_searchengines(Object *obj, struct Data *data)
{
	struct search_entry *se;
	ULONG i = 0;
	OWBFile *searchFile = new OWBFile("PROGDIR:Conf/searchengines.prefs");
	if(!searchFile)
		return;

	if (searchFile->open('w') == -1)
	{
		delete searchFile;
		return;
    }

	do
	{
		DoMethod(data->lv_entries, MUIM_List_GetEntry, i, (struct search_entry *) &se);

		if (se)
		{
			searchFile->write(String::format("%s\1%s\1%s\n", se->label, se->request, se->shortcut));
		}

		i++;
	}
	while (se);

	searchFile->close();
	delete searchFile;
}

static void	doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MA_SearchManagerGroup_Changed:
			{
				data->changed = (ULONG) tag->ti_Data;
				break;
			}
		}
	}
}

DEFNEW
{
	Object *lv_entries, *st_label, *st_request, *st_shortcut, *bt_add, *bt_remove;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, lv_entries = (Object *) NewObject(getsearchmanagerlistclass(), NULL, TAG_DONE),

		Child, HGroup,
			Child, bt_add = (Object *) MakeButton(GSI(MSG_SEARCHMANAGERGROUP_ADD)),
			Child, bt_remove = (Object *) MakeButton(GSI(MSG_SEARCHMANAGERGROUP_REMOVE)),
		End,

		Child, ColGroup(2), 
			Child, MakeLabel(GSI(MSG_SEARCHMANAGERGROUP_TITLE)),
			Child, st_label = (Object *) MakeString("", FALSE),
			Child, MakeLabel(GSI(MSG_SEARCHMANAGERGROUP_LINK)),
			Child, st_request = (Object *) MakeString("", FALSE),
			Child, MakeLabel(GSI(MSG_SEARCHMANAGERGROUP_SHORTCUT)),
			Child, st_shortcut = (Object *) MakeString("", FALSE),
		End,

		TAG_MORE, INITTAGS
		);

	if (obj)
	{
		GETDATA;

		data->lv_entries  = lv_entries;
		data->st_request  = st_request;
		data->st_label    = st_label;
		data->st_shortcut = st_shortcut;
		data->bt_remove   = bt_remove;

		set(st_label,    MUIA_Disabled, TRUE);
		set(st_request,  MUIA_Disabled, TRUE);
		set(st_shortcut, MUIA_Disabled, TRUE);
		set(bt_remove,   MUIA_Disabled, TRUE);

		DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_SearchManagerGroup_Remove);
		DoMethod(bt_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_SearchManagerGroup_Add);
		DoMethod(lv_entries,  MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MM_SearchManagerGroup_Update);

		DoMethod(st_request,  MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_SearchManagerGroup_Change);
		DoMethod(st_label,    MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_SearchManagerGroup_Change);
		DoMethod(st_shortcut, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_SearchManagerGroup_Change);
	}

	return ((ULONG)obj);
}

DEFDISP
{
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
		case MA_SearchManagerGroup_Changed:
			*msg->opg_Storage = data->changed;
			return TRUE;

		// XXX: what about using some structure/class for all of them? :)
		case MA_SearchManagerGroup_Labels:
		{
			struct search_entry *se;
			ULONG i = 0;
			ULONG count = getv(data->lv_entries, MUIA_List_Entries);

			data->labels.clear();

			if(count > 0)
			{
				do
				{
					DoMethod(data->lv_entries, MUIM_List_GetEntry, i, (struct search_entry *) &se);

					if (se)
					{
						data->labels.append(String(se->label));
						i++;
					}
				}
				while(se);
			}
			else
			{
				for(i = 0; labels[i]; i++)
				{
					data->labels.append(String(labels[i]));
				}
			}

			*msg->opg_Storage = (ULONG)&data->labels;
			return TRUE;
		}

		case MA_SearchManagerGroup_Requests:
		{
			struct search_entry *se;
			ULONG i = 0;
			ULONG count = getv(data->lv_entries, MUIA_List_Entries);

			data->requests.clear();

			if(count > 0)
			{
				do
				{
					DoMethod(data->lv_entries, MUIM_List_GetEntry, i, (struct search_entry *) &se);

					if (se)
					{
						data->requests.append(String(se->request));
						i++;
					}
				}
				while(se);
			}
			else
			{
				for(i = 0; requests[i]; i++)
				{
					data->requests.append(String(requests[i]));
				}
			}

			*msg->opg_Storage = (ULONG)&data->requests;
			return TRUE;
		}

		case MA_SearchManagerGroup_Shortcuts:
		{
			struct search_entry *se;
			ULONG i = 0;
			ULONG count = getv(data->lv_entries, MUIA_List_Entries);

			data->shortcuts.clear();

			if(count > 0)
			{
				do
				{
					DoMethod(data->lv_entries, MUIM_List_GetEntry, i, (struct search_entry *) &se);

					if (se)
					{
						data->shortcuts.append(String(se->shortcut));
						i++;
					}
				}
				while(se);
			}
			else
			{
				for(i = 0; shortcuts[i]; i++)
				{
					data->shortcuts.append(String(shortcuts[i]));
				}
			}

			*msg->opg_Storage = (ULONG)&data->shortcuts;
			return TRUE;
		}
	}

	return DOSUPER;
}

DEFTMETHOD(SearchManagerGroup_Load)
{
	GETDATA;

	load_searchengines(obj, data);

	// Use defaults if there's nothing in prefs
	if(getv(data->lv_entries, MUIA_List_Entries) == 0)
	{
		ULONG i = 0;
		char **ptr = (char **) labels;

		while(*ptr)
		{
			struct search_entry *se = (struct search_entry *) malloc(sizeof(*se));

			if (se)
			{
				se->label    = strdup(labels[i]);
				se->request  = strdup(requests[i]);
				se->shortcut = strdup(shortcuts[i]);

				DoMethod(data->lv_entries, MUIM_List_InsertSingle, se, MUIV_List_Insert_Bottom);
			}		 

			i++;
			ptr++;
		} 
	}

	set(obj, MA_SearchManagerGroup_Changed, TRUE);

	return 0;
}

DEFTMETHOD(SearchManagerGroup_Add)
{
	GETDATA;
	struct search_entry *entry = (struct search_entry *) malloc(sizeof(*entry));

	if(entry)
	{
		entry->label    = strdup("");
		entry->request  = strdup("");
		entry->shortcut = strdup("");

		DoMethod(data->lv_entries, MUIM_List_InsertSingle, entry, MUIV_List_Insert_Bottom);
		set(data->lv_entries, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(_win(obj), MUIA_Window_ActiveObject, data->st_label);

		save_searchengines(obj, data);

		set(obj, MA_SearchManagerGroup_Changed, TRUE);
	}

	return 0;
}

DEFTMETHOD(SearchManagerGroup_Remove)
{
	GETDATA;
	struct search_entry *entry;

	DoMethod(data->lv_entries, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct search_entry *) &entry);

	if(entry)
	{
		DoMethod(data->lv_entries, MUIM_List_Remove, MUIV_List_Remove_Active);

		save_searchengines(obj, data);

		// Don't allow empty list
		if(getv(data->lv_entries, MUIA_List_Entries) == 0)
		{
			DoMethod(obj, MM_SearchManagerGroup_Load);
			save_searchengines(obj, data);
		}

		set(obj, MA_SearchManagerGroup_Changed, TRUE);
	}

	return 0;
}

DEFTMETHOD(SearchManagerGroup_Update)
{
	GETDATA;
	struct search_entry *entry;

	DoMethod(data->lv_entries, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct search_entry *) &entry);

	if(entry)
	{
		set(data->bt_remove,   MUIA_Disabled, FALSE);
		set(data->st_label,    MUIA_Disabled, FALSE);
		set(data->st_request,  MUIA_Disabled, FALSE);
		set(data->st_shortcut, MUIA_Disabled, FALSE);
		nnset(data->st_label,    MUIA_String_Contents, entry->label);
		nnset(data->st_request,  MUIA_String_Contents, entry->request);
		nnset(data->st_shortcut, MUIA_String_Contents, entry->shortcut);
	}
	else
	{
		set(data->bt_remove,   MUIA_Disabled, TRUE);
		set(data->st_label,    MUIA_Disabled, TRUE);
		set(data->st_request,  MUIA_Disabled, TRUE);
		set(data->st_shortcut, MUIA_Disabled, TRUE);
	}

	return 0;
}

DEFTMETHOD(SearchManagerGroup_Change)
{
	GETDATA;
	struct search_entry *entry;

	DoMethod(data->lv_entries, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct search_entry *) &entry);

	if(entry)
	{
		free(entry->label);
		free(entry->request);
		entry->label    = strdup((char *) getv(data->st_label,    MUIA_String_Contents));
		entry->request  = strdup((char *) getv(data->st_request,  MUIA_String_Contents));
		entry->shortcut = strdup((char *) getv(data->st_shortcut, MUIA_String_Contents));

		DoMethod(data->lv_entries,  MUIM_List_Redraw, MUIV_List_Redraw_Entry, entry);

		save_searchengines(obj, data);

		set(obj, MA_SearchManagerGroup_Changed, TRUE);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECTMETHOD(SearchManagerGroup_Load)
DECTMETHOD(SearchManagerGroup_Add)
DECTMETHOD(SearchManagerGroup_Remove)
DECTMETHOD(SearchManagerGroup_Update)
DECTMETHOD(SearchManagerGroup_Change)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, searchmanagergroupclass)
