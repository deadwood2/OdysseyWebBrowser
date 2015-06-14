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
#include "ParsedCookie.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>
#include <devices/timer.h>
#include <utility/date.h>
#include <mui/Listtree_mcc.h>

#include <stdio.h>

#include "gui.h"
#include "utils.h"

#define _treenode(_o) ((struct MUIS_Listtree_TreeNode *)(_o))

enum
{
	COOKIE_POPMENU_OPEN = 1,
	COOKIE_POPMENU_OPEN_ALL,
	COOKIE_POPMENU_CLOSE,
	COOKIE_POPMENU_CLOSE_ALL,
};

using namespace WebCore;

struct Data
{
	Object *cMenu;
	struct MUIS_Listtree_TreeNode *drop;
};

MUI_HOOK(cookiemanagerlisttree_constructfunc, APTR pool, APTR t)
{
	struct cookie_entry * entry;
	struct cookie_entry * tempentry = (struct cookie_entry *) t;

	if ((entry = (struct cookie_entry *) malloc(sizeof(struct cookie_entry))))
	{
		entry->flags = tempentry->flags;
		entry->name = tempentry->name;
		entry->value = tempentry->value;
		entry->domain = tempentry->domain;
		entry->protocol = tempentry->protocol;
		entry->path = tempentry->path;
		entry->expiry = tempentry->expiry;
		entry->secure = tempentry->secure;
		entry->http_only = tempentry->http_only;
		entry->session = tempentry->session;
	}

	return (ULONG) (entry);
}

MUI_HOOK(cookiemanagerlisttree_destructfunc, APTR pool, APTR t)
{
	struct cookie_entry * entry = (struct cookie_entry *) t;

	if(entry)
	{
		free(entry->name);
		free(entry->value);
		free(entry->domain);
		free(entry->protocol);
		free(entry->path);

		free(entry);
	}
	return (0);
}

MUI_HOOK(cookiemanagerlisttree_displayfunc, APTR a, APTR t)
{
	struct MUIS_Listtree_TreeNode * tn = (struct MUIS_Listtree_TreeNode *) t;
	STRPTR *array = (STRPTR *) a;

	if (tn)
	{
		struct cookie_entry * entry = ((struct cookie_entry *)tn->tn_User);
#if 0
		if (entry->flags & COOKIEFLAG_DOMAIN)
		{
			array[0] = entry->domain/*+1*/;
			array[1] = "";
			array[2] = "";
		}
		else if(entry->flags & COOKIEFLAG_COOKIE)
		{
			array[0] = "";
			array[1] = entry->name;
			array[2] = entry->value;
		}
#endif
		if (entry->flags & COOKIEFLAG_DOMAIN)
		{
			array[0] = entry->domain/*+1*/;
			array[1] = "";
		}
		else if(entry->flags & COOKIEFLAG_COOKIE)
		{
			array[0] = entry->name;
			array[1] = entry->value;
		}
	}
	else
	{
#if 0
		array[0] = GSI(MSG_COOKIEMANAGERLIST_DOMAIN);
		array[1] = GSI(MSG_COOKIEMANAGERLIST_NAME);
		array[2] = GSI(MSG_COOKIEMANAGERLIST_VALUE);
#endif
		array[0] = GSI(MSG_COOKIEMANAGERLIST_NAME);
		array[1] = GSI(MSG_COOKIEMANAGERLIST_VALUE);
	}

	return (0);
}

MUI_HOOK(cookiemanagerlisttree_sortfunc, APTR e1, APTR e2)
{
	struct MUIS_Listtree_TreeNode * tn1 = (struct MUIS_Listtree_TreeNode *) e1;
	struct MUIS_Listtree_TreeNode * tn2 = (struct MUIS_Listtree_TreeNode *) e2;

	if (tn1 && tn2)
	{
		struct cookie_entry * entry1 = ((struct cookie_entry *)tn1->tn_User);
		struct cookie_entry * entry2 = ((struct cookie_entry *)tn2->tn_User);

		if(entry1->flags & COOKIEFLAG_DOMAIN && entry2->flags == COOKIEFLAG_DOMAIN)
		{
			return strcmp(entry1->domain, entry2->domain);
		}
		else if(entry1->flags & COOKIEFLAG_COOKIE && entry2->flags == COOKIEFLAG_COOKIE)
		{
			return strcmp(entry1->name, entry2->name);
		}
	}

	return (0);
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_ContextMenu,            TRUE,
#if 0
		MUIA_Listtree_Format,        "BAR,BAR,",
#endif
		MUIA_Listtree_Format,        "BAR,",
		MUIA_Listtree_Title,         TRUE,
		MUIA_List_Title,             TRUE, /* needed */
		MUIA_Listtree_DisplayHook,   &cookiemanagerlisttree_displayfunc_hook,
		MUIA_Listtree_ConstructHook, &cookiemanagerlisttree_constructfunc_hook,
		MUIA_Listtree_DestructHook,  &cookiemanagerlisttree_destructfunc_hook,
		MUIA_Listtree_SortHook,      &cookiemanagerlisttree_sortfunc_hook, /*MUIV_Listtree_SortHook_LeavesMixed,*/

		TAG_MORE, INITTAGS
	);

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	if (data->cMenu)
	{
		MUI_DisposeObject(data->cMenu);
	}
	return DOSUPER;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUIS_Listtree_TestPos_Result res={ NULL, 0, 0, 0 };
	struct MUIS_Listtree_TreeNode *tn;
	//struct cookie_entry *entry;
	Object *item;

	if (data->cMenu)
	{
		MUI_DisposeObject(data->cMenu);
		data->cMenu = NULL;
	}

	if (DoMethod(obj, MUIM_Listtree_TestPos, msg->mx, msg->my, &res) && (res.tpr_TreeNode))
	{
		tn = (struct MUIS_Listtree_TreeNode *) res.tpr_TreeNode;
		//entry = ((struct cookie_entry *) tn->tn_User);
		
		data->drop = tn;

		if (tn->tn_Flags & TNF_LIST)
		{
			data->cMenu = MenustripObject,
				MUIA_Family_Child, MenuObjectT(GSI(MSG_COOKIEMANAGERLIST_COOKIES)),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, (tn->tn_Flags & TNF_OPEN) ? GSI(MSG_COOKIEMANAGERLIST_FOLD_DOMAIN) : GSI(MSG_COOKIEMANAGERLIST_UNFOLD_DOMAIN),
						MUIA_UserData, (tn->tn_Flags & TNF_OPEN) ? COOKIE_POPMENU_CLOSE : COOKIE_POPMENU_OPEN ,
	                    End,
	                End,
	            End;
		}
	}
	else
	{
		data->cMenu = MenustripObject,
			MUIA_Family_Child, MenuObjectT(GSI(MSG_COOKIEMANAGERLIST_COOKIES)),
				End,
			End;
	}

	if (data->cMenu)
	{
		item = MenuitemObject,
			MUIA_Menuitem_Title, GSI(MSG_COOKIEMANAGERLIST_UNFOLD_ALL_DOMAINS),
			MUIA_UserData, COOKIE_POPMENU_OPEN_ALL,
			End;
		if (item) DoMethod(data->cMenu, MUIM_Family_AddTail, item);

		item = MenuitemObject,
			MUIA_Menuitem_Title, GSI(MSG_COOKIEMANAGERLIST_FOLD_ALL_DOMAINS),
			MUIA_UserData, COOKIE_POPMENU_CLOSE_ALL,
			End;
		if (item) DoMethod(data->cMenu, MUIM_Family_AddTail, item);

	}
	return (ULONG)data->cMenu;
}

/***********************************************************************/

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;
	ULONG udata = muiUserData(msg->item);
	//struct cookie_entry *entry;

	switch(udata)
    {
		case COOKIE_POPMENU_OPEN:
			if (data->drop)	DoMethod(obj, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root, data->drop, 0);
			break;

		case COOKIE_POPMENU_OPEN_ALL:
			if (data->drop) DoMethod(obj, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root, MUIV_Listtree_Open_TreeNode_All, 0);
			break;

		case COOKIE_POPMENU_CLOSE:
			if (data->drop)
			{
				if (!(data->drop->tn_Flags & TNF_LIST))
				{
					struct MUIS_Listtree_TreeNode *parent=(struct MUIS_Listtree_TreeNode *) DoMethod(obj, MUIM_Listtree_GetEntry, data->drop, MUIV_Listtree_GetEntry_Position_Parent, 0);
					if (parent) DoMethod(obj, MUIM_Listtree_Close, MUIV_Listtree_Close_ListNode_Root, parent, 0);
				}
				else
				{
					DoMethod(obj, MUIM_Listtree_Close, MUIV_Listtree_Close_ListNode_Root, data->drop, 0);
				}
			}
			break;

		case COOKIE_POPMENU_CLOSE_ALL:
			if (data->drop) DoMethod(obj, MUIM_Listtree_Close, MUIV_Listtree_Close_ListNode_Root, MUIV_Listtree_Close_TreeNode_All, 0);
			break;

		default:
			;
    }

	data->drop=NULL;

	return 0;
}

/*
 * Iterate through tree. dosearch informs if we should start matching entries. Used when starting search
 * from nofe different than root. sucky? oh well:)
 */

static APTR tree_iteratefind(Object *lt, struct MUIS_Listtree_TreeNode *list, STRPTR pattern, APTR startnode, ULONG *dosearch)
{
	struct MUIS_Listtree_TreeNode *tn;
	UWORD pos=0;

	for(pos=0; ; pos++)
	{
		tn = (struct MUIS_Listtree_TreeNode *)DoMethod(lt, MUIM_Listtree_GetEntry, list, pos, MUIV_Listtree_GetEntry_Flags_SameLevel);

		if ( tn != NULL )
		{
			if ( tn->tn_Flags & TNF_LIST )
			{
				if ( name_match (tn->tn_Name, pattern ))
				{
					return tn;
				}
				else
				{
					APTR ftn = tree_iteratefind(lt, tn, pattern, startnode, dosearch);

					if ( ftn != NULL )
						return ftn;
				}
			}
			else
			{
				if ( *dosearch )
				{
					struct cookie_entry *e = ((struct cookie_entry *)tn->tn_User);
					if ( name_match( tn->tn_Name, pattern ) || ( e != NULL && e->value != NULL && name_match( e->value, pattern ) ) )
						return tn;
				}
			}

			if ( tn == startnode )
				*dosearch = TRUE;

		}
		else
		{
			break;
		}
	}

	return NULL;
}

static APTR tree_find(Object *lt, STRPTR pattern, APTR startnode)
{
	ULONG dofind = startnode != NULL ? FALSE : TRUE;
	return tree_iteratefind(lt, MUIV_Listtree_GetEntry_ListNode_Root, pattern, startnode, &dofind);
}

DEFSMETHOD(Search)
{
	struct MUIS_Listtree_TreeNode *tn = NULL;
	STRPTR pattern;

	if ( msg->string == NULL || *msg->string == 0 )
	{
		return FALSE;
	}

	/*
	 * Search for pattern in mimetype name and description.
	 */

	pattern = (STRPTR) malloc( strlen( msg->string ) + 5 );
	if ( pattern != NULL )
	{
		/*
		 * When searching for next one, fetch selected one first.
		 */

		if ( (msg->flags & MV_FindText_Previous) == 0)
		{
			tn = (struct MUIS_Listtree_TreeNode *)DoMethod(obj, MUIM_Listtree_GetEntry, MUIV_Listtree_GetEntry_ListNode_Active, MUIV_Listtree_GetEntry_Position_Active, 0);
		}

		sprintf( pattern, "#?%s#?", msg->string );

		tn = _treenode( tree_find( obj, pattern, tn ) );

		if(tn == NULL)
		{
			tn = _treenode( tree_find( obj, pattern, NULL ) );
		}

		/*
		 * Open node if needed and select.
		 */

		if ( tn != NULL )
		{
			DoMethod( obj, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Parent, tn, 0 );
			SetAttrs( obj, MUIA_Listtree_Active, tn, TAG_DONE );
		}

		free( pattern );
	}

	return tn != NULL ? TRUE : FALSE;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECSMETHOD(Search)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Listtree, cookiemanagerlisttreeclass)
