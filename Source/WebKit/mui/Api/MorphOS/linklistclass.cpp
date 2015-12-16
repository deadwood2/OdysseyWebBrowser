/*
 * Copyright 2009 Polymere
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

#include "gui.h"

/* public */
#include <stdio.h>
#include <string.h>
#include <mui/Listtree_mcc.h>
#include <stdlib.h>
#include <clib/macros.h>


/* private */
#include "bookmarkgroupclass.h"

#define LOC(a,b) (b)

struct Data
{
	LONG    Drop;
	LONG	Drop_Pos;
	LONG    Drop_Offset;
	Object *cMenu;
	char    cMenu_Title[30];
	Object *bookmark_obj;
	Object *bookmark_list;
};

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	struct treedata *td=NULL;
	FORTAG(tags)
	{
		case MUIA_List_DoubleClick:
			DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &td);
			if (td)
			{
				//kprintf("Double click on %08lx\n", td);
				// Open link
				//kprintf("URL: %s\n", (td->alias) ? td->alias : (STRPTR)"(NULL)" ); // address can be NULL
			}
			break;
		case MA_Bookmarkgroup_BookmarkObject:
			data->bookmark_obj=(Object *)tag->ti_Data;
			break;
		case MA_Bookmarkgroup_BookmarkList:
			data->bookmark_list=(Object *)tag->ti_Data;
			break;
	}
	NEXTTAG
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_Format,            "BAR,",
		MUIA_List_MinLineHeight,     (getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark) ? 18 : TAG_IGNORE,
		MUIA_List_Title,             TRUE,
		MUIA_List_DragSortable,      TRUE,
		MUIA_List_DragType,          1,
		MUIA_List_ShowDropMarks,     TRUE,
		MUIA_ShortHelp,              TRUE,
		//MUIA_ContextMenu,            TRUE,
		MUIA_CycleChain,             TRUE,
	End;

	if (obj)
	{
		GETDATA;
		data->cMenu=NULL;
		data->Drop=NULL;
	}
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

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
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
	if (msg->entry)
	{
		struct treedata *td = ((struct treedata *)msg->entry);

		if (td->buffer2)
		{
			FreeVecTaskPooled(td->buffer2);
			td->buffer2 = NULL;
		}

		if((td->flags & NODEFLAG_LINK || td->flags & NODEFLAG_GROUP) && (getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark))
		{
			if(!td->icon)
			{
				td->icon  = NewObject(getfaviconclass(), NULL,
									  MA_FavIcon_Folder, td->flags & NODEFLAG_GROUP,
									  MA_FavIcon_PageURL, td->address,
									  MUIA_Weight, 0,
									  TAG_DONE);

				if(td->icon)
				{
					td->iconimg	= (APTR) DoMethod((Object *) td->tree, MUIM_List_CreateImage, td->icon, 0);
				}
			}
		}

		if((getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark) && td->iconimg)
		{
			STRPTR alias = NULL;

			if (!td->alias || *td->alias=='\0')
				alias = "\0";
			else
				alias = td->alias;

			td->buffer2 = (STRPTR) AllocVecTaskPooled(strlen(alias)+1+16);
			if (td->buffer2 )
				sprintf(td->buffer2, " \033O[%08lx] %s", (unsigned long) td->iconimg, alias);
		}
		else
		{
			STRPTR alias = NULL;

			if (!td->alias || *td->alias=='\0')
				alias = GSI(MSG_LINKLIST_NO_NAME);
			else
				alias = td->alias;

			td->buffer2 = (STRPTR) AllocVecTaskPooled(strlen(alias)+1);
			if (td->buffer2)
				sprintf(td->buffer2, "%s", alias);
		}

		msg->array[0] = td->buffer2;
		msg->array[1] = td->address;

		if( (ULONG)msg->array[-1] % 2 )
		{
			msg->array[-9] = (STRPTR) 10;
		}
	}
	else
	{
		msg->array[0] = GSI(MSG_LINKLIST_QUICK_LINK);
		msg->array[1] = GSI(MSG_BOOKMARKLISTTREE_ADDRESS);
	}
	return TRUE;
}

/***********************************************************************/

DEFMMETHOD(List_Compare)
{
	//struct obj_data *data = INST_DATA(cl,obj);
	struct treedata *td1=(struct treedata *)msg->entry1;
	struct treedata *td2=(struct treedata *)msg->entry2;

	if (td2->ql_order==0 && td1->ql_order!=0) return (ULONG)-1;
	if (td1->ql_order==0 && td2->ql_order!=0) return 1;

	if (td1->ql_order>td2->ql_order) return 1;
	if (td1->ql_order<td2->ql_order) return (ULONG)-1;

	if (!td1->alias) return (ULONG)-1;
	if (!td2->alias) return 1;

	return stricmp(td1->alias,td2->alias);
}

DEFMMETHOD(DragQuery)
{
	struct MUIS_Listtree_TreeNode *tn=NULL;
	struct treedata *td;  
	GETDATA;
	
	if( (data->bookmark_list) && msg->obj==data->bookmark_list)
	{
		//kprintf("From Bookmark\n");
		tn=(struct MUIS_Listtree_TreeNode *)DoMethod(msg->obj, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
		if (tn)
		{
			td=(struct treedata *)tn->tn_User;
			//kprintf("Flags: %08lx\n",td->flags);
			if ( (td->flags & NODEFLAG_LINK) && !(td->flags & NODEFLAG_QUICKLINK) && !(td->flags & NODEFLAG_BAR) ) return MUIV_DragQuery_Accept;
		}
	}
	if( msg->obj==obj )
	{
		//kprintf("From ourself\n");
		return MUIV_DragQuery_Accept;
	}

	return MUIV_DragQuery_Refuse;
}

DEFMMETHOD(DragReport)
{
	GETDATA;   
	struct MUI_List_TestPos_Result r={ 0, 0, 0, 0, 0 };

	if( (data->bookmark_list) && msg->obj==data->bookmark_list)
	{
		DoMethod(obj, MUIM_List_TestPos, msg->x, msg->y, &r);
		if ( data->Drop!=r.entry || data->Drop_Pos!=r.flags || data->Drop_Offset!=r.yoffset )
		{
			if (!msg->update) return MUIV_DragReport_Refresh;
			data->Drop=r.entry;
			data->Drop_Pos=r.flags;
			data->Drop_Offset=r.yoffset;
			//kprintf("DropEntry is %ld pos %ld yoffset %ld\n", data->Drop, data->Drop_Pos, r.yoffset);
		}
	}
	return DOSUPER;
}

DEFMMETHOD(DragDrop)
{
	GETDATA;
	LONG   rc;
	
	if (msg->obj==data->bookmark_list)
	{
		// From Bookmark
		//kprintf("Drop from Bookmark\n");
		rc=data->Drop+1;
		if (data->Drop_Pos==1) rc=1;
		if (data->Drop_Pos==2) rc=-1;
		if (data->Drop_Pos==0 && data->Drop_Offset>0) rc++;
		set(data->bookmark_obj, MA_Bookmarkgroup_QuickLink, rc);
		return 0;
	}
	//kprintf("Drop sort\n");
	rc=DOSUPER;
	DoMethod(app, MUIM_Application_PushMethod,
		data->bookmark_obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
		MM_Bookmarkgroup_AutoSave);
	return rc;
}

DEFMMETHOD(CreateShortHelp)
{
	STRPTR help;
	struct treedata *td=NULL;
	LONG title_len,address_len,pos=-1;

	DoMethod(obj, MUIM_List_TestPos, msg->mx, msg->my, &pos);
	if (pos!=-1)
	{
		DoMethod(obj, MUIM_List_GetEntry, pos, &td);
		if (td)
		{
			title_len=(td->address) ? strlen(td->title) : 0;
			address_len=(td->address) ? strlen(td->address) : 0;
	 
			if ((address_len) && (title_len))
			{
				help=(STRPTR)malloc(address_len+title_len+1);
				if (help)
				{
					sprintf(help, "%s\n%s", td->title, td->address);
					return ((ULONG)help);
				}
			}
		}
	}
	return (0);
}

DEFMMETHOD(DeleteShortHelp)
{
	free(msg->help);
    return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
DECMMETHOD(DragQuery)
DECMMETHOD(DragReport)
DECMMETHOD(DragDrop)
DECMMETHOD(CreateShortHelp)
DECMMETHOD(DeleteShortHelp)
//DECMMETHOD(ContextMenuBuild)
//DECMMETHOD(ContextMenuChoice)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, linklistclass)
