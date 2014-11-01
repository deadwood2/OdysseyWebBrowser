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
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include "Document.h"

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clib/macros.h>

#include "gui.h"

#define D(x)

using namespace WebCore;

namespace WebCore
{
	extern bool ad_block_enabled;
	extern void loadCache();
	extern void flushCache();
	extern bool writeCache();
	extern void *addCacheEntry(String rule, int type);
	extern void updateCacheEntry(String rule, int type, void *ptr);
	extern void removeCacheEntry(void *ptr, int type);
}


#define LABEL(x) (STRPTR)MSG_BLOCKMANAGERGROUP_##x

STATIC CONST CONST_STRPTR filtertypes[] =
{
	LABEL(DENY),
	LABEL(ALLOW),
	NULL
};

static void cycles_init(void)
{
	APTR arrays[] = { (APTR) filtertypes, NULL };

	APTR *ptr = arrays;

	while(*ptr)
	{
		STRPTR *current = (STRPTR *)*ptr;
		while(*current)
		{
			*current = (STRPTR)GSI((ULONG)*current);
			current++;
		}
		ptr++;
	}
}


struct Data
{
	Object *lv_rules;
	Object *st_rule;
	Object *cy_type;
	Object *bt_remove;
	ULONG loaded;
};

DEFNEW
{
	Object *lv_rules, *st_rule, *cy_type, *bt_add, *bt_remove;

	cycles_init();

	obj = (Object *) DoSuperNew(cl, obj,
		Child, lv_rules = (Object *) NewObject(getblockmanagerlistclass(), NULL, TAG_DONE),

		Child, HGroup,
			Child, bt_add = (Object *) MakeButton(GSI(MSG_BLOCKMANAGERGROUP_ADD)),
			Child, bt_remove = (Object *) MakeButton(GSI(MSG_BLOCKMANAGERGROUP_REMOVE)),
		End,

		Child, ColGroup(2), 
			Child, MakeLabel(GSI(MSG_BLOCKMANAGERGROUP_RULE)),
			Child, st_rule = (Object *) MakeString("",FALSE),
			Child, MakeLabel(GSI(MSG_BLOCKMANAGERGROUP_TYPE)),
			Child, cy_type = (Object *) MakeCycle(GSI(MSG_BLOCKMANAGERGROUP_TYPE), filtertypes),
		End,

		TAG_MORE, INITTAGS
		);

	if (obj)
	{
		GETDATA;

		data->bt_remove = bt_remove;
		data->lv_rules = lv_rules;
		data->st_rule = st_rule;
		data->cy_type = cy_type;

		set(st_rule, MUIA_Disabled, TRUE);
		set(cy_type, MUIA_Disabled, TRUE);
		set(bt_remove,  MUIA_Disabled, TRUE);

		DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_BlockManagerGroup_Remove);
		DoMethod(bt_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_BlockManagerGroup_Add);
		DoMethod(lv_rules,  MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MM_BlockManagerGroup_Update);

		DoMethod(st_rule, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_BlockManagerGroup_Change);
		DoMethod(cy_type, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 1, MM_BlockManagerGroup_Change);
	}

	return ((ULONG)obj);
}

DEFDISP
{
	return DOSUPER;
}


DEFTMETHOD(BlockManagerGroup_Load)
{
	GETDATA;

	if(!data->loaded)
	{
		data->loaded = TRUE;
		WebCore::loadCache();
	}
	return 0;
}

DEFSMETHOD(BlockManagerGroup_DidInsert)
{
	GETDATA;
	struct block_entry *entry = (struct block_entry *) malloc(sizeof(*entry));

	if(entry)
	{
		entry->rule = strdup(msg->rule);
		entry->type = msg->type;
		entry->ptr = msg->ptr;

		DoMethod(data->lv_rules, MUIM_List_InsertSingle, entry, MUIV_List_Insert_Bottom);
	}

	return 0;
}

DEFTMETHOD(BlockManagerGroup_Add)
{
	GETDATA;
	struct block_entry *entry = (struct block_entry *) malloc(sizeof(*entry));

	if(entry)
	{
		entry->rule = strdup("");
		entry->type = 1;
		entry->ptr = WebCore::addCacheEntry(entry->rule, entry->type);
		WebCore::writeCache();
		WebCore::flushCache();

		DoMethod(data->lv_rules, MUIM_List_InsertSingle, entry, MUIV_List_Insert_Bottom);
		set(data->lv_rules, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(_win(obj), MUIA_Window_ActiveObject, data->st_rule);
	}

	return 0;
}

DEFTMETHOD(BlockManagerGroup_Remove)
{
	GETDATA;
	struct block_entry *entry;

	DoMethod(data->lv_rules, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct block_entry *) &entry);

	if(entry)
	{
		WebCore::removeCacheEntry(entry->ptr, entry->type);
		WebCore::writeCache();
		WebCore::flushCache();
		DoMethod(data->lv_rules, MUIM_List_Remove, MUIV_List_Remove_Active);
	}

	return 0;
}

DEFTMETHOD(BlockManagerGroup_Update)
{
	GETDATA;
	struct block_entry *entry;

	DoMethod(data->lv_rules, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct block_entry *) &entry);

	if(entry)
	{
		set(data->st_rule,   MUIA_Disabled, FALSE);
		set(data->cy_type,   MUIA_Disabled, FALSE);
		set(data->bt_remove, MUIA_Disabled, FALSE);
		nnset(data->cy_type, MUIA_Cycle_Active, entry->type);
		nnset(data->st_rule, MUIA_String_Contents, entry->rule);
	}
	else
	{
		set(data->st_rule,   MUIA_Disabled, TRUE);
		set(data->cy_type,   MUIA_Disabled, TRUE);
		set(data->bt_remove, MUIA_Disabled, TRUE);
	}

	return 0;
}

DEFTMETHOD(BlockManagerGroup_Change)
{
	GETDATA;
	struct block_entry *entry;

	DoMethod(data->lv_rules, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct block_entry *) &entry);

	if(entry)
	{
		free(entry->rule);
		entry->rule = strdup((char *) getv(data->st_rule, MUIA_String_Contents));

		if(entry->type != (int) getv(data->cy_type, MUIA_Cycle_Active))
		{
			WebCore::removeCacheEntry(entry->ptr, entry->type);
			entry->type = getv(data->cy_type, MUIA_Cycle_Active);
			entry->ptr = WebCore::addCacheEntry(entry->rule, entry->type);
		}
		else
		{
			WebCore::updateCacheEntry(entry->rule, entry->type, entry->ptr);
		}

		WebCore::writeCache();
		WebCore::flushCache();

		DoMethod(data->lv_rules,  MUIM_List_Redraw, MUIV_List_Redraw_Entry, entry);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECTMETHOD(BlockManagerGroup_Load)
DECSMETHOD(BlockManagerGroup_DidInsert)
DECTMETHOD(BlockManagerGroup_Add)
DECTMETHOD(BlockManagerGroup_Remove)
DECTMETHOD(BlockManagerGroup_Update)
DECTMETHOD(BlockManagerGroup_Change)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, blockmanagergroupclass)
