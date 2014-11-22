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
#include "GraphicsContext.h"
#include "ContextMenu.h"
#include <wtf/text/CString.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"
#include "utils.h"

#ifndef get
#define get(obj,attr,store) GetAttr(attr,obj,(ULONGPTR)store)
#endif

using namespace WebCore;

struct contextmenunode* contextmenu_create(int actionId, int category, char *label, int commandType, char *commandString)
{
	struct contextmenunode *cn = (struct contextmenunode *) malloc(sizeof(*cn));

	if(cn)
	{
		cn->actionId      = actionId;
		cn->category      = category;
		cn->label         = strdup(label);
		cn->commandType   = commandType;
		cn->commandString = strdup(commandString);

		ADDTAIL(&contextmenu_list, cn);
	}

	return cn;
}

void contextmenu_delete(struct contextmenunode *cn)
{
	REMOVE(cn);
	free(cn->label);
	free(cn->commandString);
	free(cn);
}

#define LABEL(x) (STRPTR)MSG_CONTEXTMENUGROUP_##x

STATIC CONST CONST_STRPTR categories[] =
{
	LABEL(LINK),
	LABEL(IMAGE),
	LABEL(PAGE),
	NULL
};

STATIC CONST CONST_STRPTR actiontypes[] =
{
	"AmigaDOS",
	"ARexx",
	"Internal",
	NULL
};

STATIC CONST CONST_STRPTR placeholders[] = {"%l", "%i", "%u", "%p", NULL};
STATIC CONST CONST_STRPTR placeholders_desc[] = {"%l - link URL", "%i - image URL", "%u - page URL", "%p - OWB REXX port", NULL};

static void cycles_init(void)
{
	APTR arrays[] = { (APTR) categories, /*(APTR) actiontypes,*/ NULL };


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
	Object *lv_contextmenu;
	Object *bt_add;
	Object *bt_remove;
	Object *cy_category;
	Object *st_label;
	Object *cy_commandtype;
	Object *st_commandstring;
	Object *pop_commandstring;
	Object *lv_commandstring;
};

DEFNEW
{
	Object *lv_contextmenu, *bt_add, *bt_remove, *cy_category, /**cy_commandtype,*/ *st_label, *st_commandstring, *pop_commandstring, *lv_commandstring;

	cycles_init();

	obj = (Object *) DoSuperNew(cl, obj,
				MUIA_ObjectID, MAKE_ID('S','M','0','0'),
				Child, lv_contextmenu = (Object *) NewObject(getcontextmenulistclass(), NULL, TAG_DONE),
				Child, HGroup,
						Child, bt_add = (Object *) MakeButton(GSI(MSG_CONTEXTMENUGROUP_ADD)),
						Child, bt_remove = (Object *) MakeButton(GSI(MSG_CONTEXTMENUGROUP_REMOVE)),
						End,
				Child, ColGroup(2),
					Child, MakeLabel(GSI(MSG_CONTEXTMENUGROUP_CATEGORY)),
					Child, HGroup,
							Child, cy_category = (Object *) MakeCycle(GSI(MSG_CONTEXTMENUGROUP_CATEGORY), categories),
							Child, MakeLabel(GSI(MSG_CONTEXTMENUGROUP_LABEL)),
							Child, st_label = (Object *) MakeString("", FALSE),
							End,
					Child, MakeLabel(GSI(MSG_CONTEXTMENUGROUP_ACTION)),
					Child, HGroup,
							//Child, cy_commandtype = (Object *) MakeCycle("Action:", actiontypes),
							Child, pop_commandstring = PopobjectObject,
									MUIA_Popstring_String, st_commandstring = StringObject,
										StringFrame,
										MUIA_CycleChain, 1,
										MUIA_String_MaxLen, 512,
										End,
									MUIA_Popstring_Button, PopButton(MUII_PopUp),
										MUIA_Popobject_Object, lv_commandstring = ListObject,
										InputListFrame,
										MUIA_List_SourceArray, placeholders_desc,
										End,
									End,
							End,
					End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->bt_remove = bt_remove;
		data->bt_add = bt_add;
		data->lv_contextmenu = lv_contextmenu;
		data->cy_category = cy_category;
		data->st_label = st_label;
		//data->cy_commandtype = cy_commandtype;
		data->st_commandstring = st_commandstring;
		data->lv_commandstring = lv_commandstring;
		data->pop_commandstring = pop_commandstring;

		set(bt_remove,   MUIA_Disabled, TRUE);
		set(cy_category, MUIA_Disabled, TRUE);
		//set(cy_commandtype, MUIA_Disabled, TRUE);
		set(st_label, MUIA_Disabled, TRUE);
		set(pop_commandstring, MUIA_Disabled, TRUE);

		DoMethod(bt_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ContextMenuGroup_Add);
		DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ContextMenuGroup_Remove);

		DoMethod(lv_contextmenu, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MM_ContextMenuGroup_Refresh);

		DoMethod(cy_category,      MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 1, MM_ContextMenuGroup_Change);
		//DoMethod(cy_commandtype,   MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, obj, 1, MM_ContextMenuGroup_Change);
		DoMethod(st_label,         MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_ContextMenuGroup_Change);
		DoMethod(st_commandstring, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_ContextMenuGroup_Change);

		DoMethod(lv_commandstring, MUIM_Notify, MUIA_List_DoubleClick, TRUE, pop_commandstring, 2, MUIM_Popstring_Close, TRUE);
		DoMethod(lv_commandstring, MUIM_Notify, MUIA_List_DoubleClick, TRUE, obj, 1, MM_ContextMenuGroup_AddPlaceholder);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;
	APTR n, m;

	DoMethod(data->lv_contextmenu, MUIM_List_Clear);

	ITERATELISTSAFE(n, m, &contextmenu_list)
	{
		contextmenu_delete((contextmenunode *) n);
	}

	return DOSUPER;
}

DEFTMETHOD(ContextMenuGroup_AddPlaceholder)
{
    GETDATA;

    STRPTR oldpat;
	char newpat[512];
    ULONG clicked, pos;

	DoMethod(data->pop_commandstring, MUIM_Popstring_Close, TRUE);

	if (get(data->st_commandstring, MUIA_String_Contents, &oldpat))
    {
		get(data->st_commandstring, MUIA_String_BufferPos, &pos);
		get(data->lv_commandstring, MUIA_List_Active, &clicked);

		if (clicked < sizeof(placeholders))
		{
			newpat[pos] = '\0';
			strncpy(newpat, oldpat, pos);
			strcat(newpat, placeholders[clicked]);
			strcat(newpat, oldpat + pos);
			set(data->st_commandstring, MUIA_String_Contents, newpat);
		}
	}

	return (0);
}

DEFTMETHOD(ContextMenuGroup_Refresh)
{
	GETDATA;
	struct contextmenunode *cn;

	DoMethod(data->lv_contextmenu, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct contextmenunode *) &cn);

	if(cn)
	{
		set(data->bt_remove,   MUIA_Disabled, FALSE);
		set(data->cy_category, MUIA_Disabled, FALSE);
		//set(data->cy_commandtype, MUIA_Disabled, FALSE);
		set(data->st_label, MUIA_Disabled, FALSE);
		set(data->pop_commandstring, MUIA_Disabled, FALSE);

		nnset(data->cy_category, MUIA_Cycle_Active, cn->category);
		//nnset(data->cy_commandtype, MUIA_Cycle_Active, cn->commandType);
		nnset(data->st_label, MUIA_String_Contents, cn->label);
		nnset(data->st_commandstring, MUIA_String_Contents, cn->commandString);
	}
	else
	{
		set(data->bt_remove,   MUIA_Disabled, TRUE);
		set(data->cy_category, MUIA_Disabled, TRUE);
		//set(data->cy_commandtype, MUIA_Disabled, TRUE);
		set(data->st_label, MUIA_Disabled, TRUE);
		set(data->pop_commandstring, MUIA_Disabled, TRUE);
	}

	return 0;
}

DEFTMETHOD(ContextMenuGroup_Change)
{
	GETDATA;
	struct contextmenunode *cn;

	DoMethod(data->lv_contextmenu, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct contextmenunode *) &cn);

	if(cn)
	{
		cn->category = (int) getv(data->cy_category, MUIA_Cycle_Active);
		//cn->commandType = (int) getv(data->cy_commandtype, MUIA_Cycle_Active);
		free(cn->label);
		cn->label = strdup((char *) getv(data->st_label, MUIA_String_Contents));
		free(cn->commandString);
		cn->commandString = strdup((char *) getv(data->st_commandstring, MUIA_String_Contents));

		DoMethod(data->lv_contextmenu,  MUIM_List_Redraw, MUIV_List_Redraw_Entry, cn);
	}

	return 0;
}

DEFTMETHOD(ContextMenuGroup_Add)
{
	GETDATA;
	APTR n;
	int	maxId = ContextMenuItemBaseApplicationTag;
	struct contextmenunode *cn;

	ITERATELIST(n, &contextmenu_list)
	{
        cn = (struct contextmenunode *) n;
		if(cn->actionId > maxId)
			maxId = cn->actionId;
	}
	
	maxId++;

	cn = contextmenu_create(maxId,
					   MV_OWBApp_BuildUserMenu_Link,
					   GSI(MSG_CONTEXTMENUGROUP_DEFAULT_LABEL),
					   ACTION_AMIGADOS,
					   "");

	if(cn)
	{
		DoMethod(data->lv_contextmenu, MUIM_List_InsertSingle, cn, MUIV_List_Insert_Bottom);
		set(data->lv_contextmenu, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(_win(obj), MUIA_Window_ActiveObject, data->st_label);
	}

	return 0;
}

DEFTMETHOD(ContextMenuGroup_Remove)
{
	GETDATA;
	struct contextmenunode *cn;
	DoMethod(data->lv_contextmenu, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct contextmenunode *) &cn);

	if(cn)
	{
		DoMethod(data->lv_contextmenu, MUIM_List_Remove, MUIV_List_Remove_Active);
		contextmenu_delete(cn);
	}

	return 0;
}

DEFMMETHOD(Import)
{
	GETDATA;
	ULONG id;
	char *val;
	APTR n, m;

	DoMethod(data->lv_contextmenu, MUIM_List_Clear);

	ITERATELISTSAFE(n, m, &contextmenu_list)
	{
		contextmenu_delete((contextmenunode *) n);
	}

	NEWLIST(&contextmenu_list);

	if((id=(muiNotifyData(obj)->mnd_ObjectID)))
	{
		if ((val = (char *)DoMethod(msg->dataspace, MUIM_Dataspace_Find, id)))
		{
			ULONG i;
			String input = val;
			Vector<String> menuItems;
			input.split("\n", true, menuItems);
			for(i = 0; i < menuItems.size(); i++)
			{
				Vector<String> menuAttributes;
				menuItems[i].split("\1", true, menuAttributes);

				if(menuAttributes.size() == 4)
				{
					struct contextmenunode *cn;
					cn = contextmenu_create(ContextMenuItemBaseApplicationTag + i,
									   menuAttributes[0].toInt(),
									   (char *) menuAttributes[1].latin1().data(),
									   menuAttributes[2].toInt(),
									   (char *) menuAttributes[3].latin1().data());

					if(cn)
					{
						DoMethod(data->lv_contextmenu, MUIM_List_InsertSingle, cn, MUIV_List_Insert_Bottom);
					}
				}
			}
		}
	}

	return 0;
}

DEFMMETHOD(Export)
{
	ULONG id;

	if((id=(muiNotifyData(obj)->mnd_ObjectID)))
	{
		APTR n;
		String output = "";

		ITERATELIST(n, &contextmenu_list)
		{
			struct contextmenunode *cn = (struct contextmenunode *) n;
			output.append(String::format("%d\1%s\1%d\1%s\n", cn->category, cn->label, cn->commandType, cn->commandString));
		}

		DoMethod(msg->dataspace, MUIM_Dataspace_Add, output.latin1().data(), output.length()+1, id);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Import)
DECMMETHOD(Export)
DECTMETHOD(ContextMenuGroup_Add)
DECTMETHOD(ContextMenuGroup_Remove)
DECTMETHOD(ContextMenuGroup_Change)
DECTMETHOD(ContextMenuGroup_Refresh)
DECTMETHOD(ContextMenuGroup_AddPlaceholder)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, contextmenugroupclass)
