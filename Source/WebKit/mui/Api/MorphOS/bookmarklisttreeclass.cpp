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

#include <config.h>

/* public */
#include <mui/Listtree_mcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* private */
#include "gui.h"
#undef set
#include "utils.h"
#include "bookmarkgroupclass.h"

#define LOC(a,b) (b)

struct Data
{
	APTR activetype;
	struct MUIS_Listtree_TreeNode *drop;
	ULONG drop_flags;
	LONG  old_entry;
	struct MUIS_Listtree_TreeNode *dd_active;
	ULONG  dd_type;
	struct MUIS_Listtree_TreeNode *drop_node;
	Object *cMenu;
	char    cMenu_Title[30];
	Object *group_obj;

	ULONG added;
	struct MUI_EventHandlerNode ehnode;
};

#define _treenode(_o) ((struct MUIS_Listtree_TreeNode *)(_o))

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	struct treedata * td; 
	FORTAG(tags)
	{
		case MUIA_List_DoubleClick:
			data->drop= (struct MUIS_Listtree_TreeNode *) DoMethod(obj, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
			if ( (data->drop) && !(data->drop->tn_Flags & TNF_LIST))
			{
				Object *window = NULL;
				//kprintf("Double click open new tab or current ?\n", data->drop);
				// Open link
				td=((struct treedata *)data->drop->tn_User);
				//kprintf("URL: %s\n", (td->address) ? td->address : (STRPTR)"(NULL)" ); // address can be NULL

				window = (Object *) getv(app, MA_OWBApp_ActiveWindow);
				if(window)
				{
					DoMethod(window, MM_OWBWindow_LoadURL, td->address, NULL);
				}
			}
			break;
		case MA_Bookmarkgroup_BookmarkObject:
			data->group_obj=(Object *)tag->ti_Data;
			break;
	}
	NEXTTAG
}

MUI_HOOK(bookmarklisttree_constructfunc, APTR pool, APTR t)
{
	struct treedata * td;
	struct treedata * tempnode = (struct treedata *) t;

	//kprintf( "Listtree construct hook title %08lx alias %08lx address %08lx.\n", (ULONG)tempnode->title, (ULONG)tempnode->alias, (ULONG)tempnode->address);
	if ((td = (struct treedata *) AllocPooled(pool, sizeof(struct treedata))))
	{
		td->flags    = tempnode->flags;
		td->title    = NULL;
		td->alias    = NULL;
		td->address  = NULL;
		td->buffer1  = NULL;
		td->buffer2  = NULL;
		td->ql_order = tempnode->ql_order;
		td->icon = tempnode->icon;
		td->iconimg = tempnode->iconimg;
		td->tree = tempnode->tree;

		if (tempnode->title)
		{
			td->title = (STRPTR) AllocVecTaskPooled(strlen(tempnode->title)+1);
			if (td->title) strcpy(td->title, tempnode->title);

		}
		if (tempnode->alias)
		{
			td->alias = (STRPTR) AllocVecTaskPooled(strlen(tempnode->alias)+1);
			if (td->alias) strcpy(td->alias, tempnode->alias);
		}
		if (tempnode->address)
		{
			td->address = (STRPTR) AllocVecTaskPooled(strlen(tempnode->address)+1);
			if (td->address) strcpy(td->address, tempnode->address);
		}
	}
	//kprintf( "End of construct\n");
	return (ULONG) (td);
}

MUI_HOOK(bookmarklisttree_destructfunc, APTR pool, APTR t)
{
	struct treedata * td = (struct treedata *) t;

	if (td)
	{
		if (td->title)   FreeVecTaskPooled(td->title);
		if (td->alias)   FreeVecTaskPooled(td->alias);
		if (td->address) FreeVecTaskPooled(td->address);
		if (td->buffer1) FreeVecTaskPooled(td->buffer1);
		if (td->buffer2) FreeVecTaskPooled(td->buffer2);

		if(td->tree && muiRenderInfo(td->tree))
		{
			DoMethod((Object *) td->tree, MUIM_List_DeleteImage, td->iconimg);
		}

		if(td->icon)
		{
			MUI_DisposeObject((Object *) td->icon);
		}

		FreePooled(pool, td, sizeof(struct treedata));
	}
	return (0);
}

MUI_HOOK(bookmarklisttree_displayfunc, APTR a, APTR t)
{
	struct MUIS_Listtree_TreeNode * tn = (struct MUIS_Listtree_TreeNode *) t;
	STRPTR *array = (STRPTR *) a;

	if (tn)
	{
		struct treedata * td = ((struct treedata *)tn->tn_User);

		if (td->flags & NODEFLAG_BAR)
		{
			// A bar
			array[0] = GSI(MSG_BOOKMARKLISTTREE_SEPARATOR);
			array[1] = "";
			//array[2] = "";

			/*

			td->icon  = MUI_MakeObject(MUIO_HBar, NULL);

			if(td->icon)
			{
				SetAttrs(td->icon, MUIA_FixWidth, 600, TAG_DONE);
				td->iconimg	= (APTR) DoMethod((Object *) td->tree, MUIM_List_CreateImage, td->icon, 0);
			}

			if (td->buffer1)
			{
				FreeVecTaskPooled(td->buffer1);
				td->buffer1=NULL;
			}
			td->buffer1=(STRPTR) AllocVecTaskPooled(strlen(td->title)+4+16);
			if (td->buffer1) sprintf(td->buffer1, " \033O[%08lx]", (unsigned long) td->iconimg);

			if (td->buffer1) array[0]=td->buffer1;
			else array[0] = GSI(MSG_BOOKMARKLISTTREE_SEPARATOR);
			array[1] = "";  */
		}
		else
		{
			STRPTR title=td->title;

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
						DoMethod((Object *) td->icon, MUIM_Notify, MA_FavIcon_NeedRedraw, MUIV_EveryTime, (Object *) td->tree, 3, MUIM_List_Redraw, MUIV_List_Redraw_Entry, t);
					}
				}
			}

			if (!td->title || *td->title=='\0') title=GSI(MSG_BOOKMARKLISTTREE_NO_TITLE);
			if (td->buffer1)
			{
				FreeVecTaskPooled(td->buffer1);
				td->buffer1=NULL;
			}

			if (td->flags & NODEFLAG_INMENU)
			{
				// add bold
				if((getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark) && td->iconimg)
				{
					td->buffer1=(STRPTR) AllocVecTaskPooled(strlen(td->title)+4+16);
					if (td->buffer1) sprintf(td->buffer1, " \033b\033O[%08lx] %s", (unsigned long) td->iconimg, td->title);
				}
				else
				{
					td->buffer1=(STRPTR) AllocVecTaskPooled(strlen(td->title)+4);
					if (td->buffer1) sprintf(td->buffer1, " \033b%s", td->title);
				}
			}
			else
			{
				// simple
				if((getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark) && td->iconimg)
				{
					td->buffer1=(STRPTR) AllocVecTaskPooled(strlen(title)+1+16);
					if (td->buffer1) sprintf(td->buffer1, " \033O[%08lx] %s", (unsigned long) td->iconimg, title);
				}
				else
				{
					td->buffer1=(STRPTR) AllocVecTaskPooled(strlen(title)+1);
					if (td->buffer1) sprintf(td->buffer1, " %s", title);
				}
			}
			if (td->buffer1) array[0]=td->buffer1;
			else array[0] = (STRPTR)"## Alloc error";

			array[1] = (td->address) ? td->address : (STRPTR)"";
		}

		if( (ULONG)array[ -1 ] % 2 )
		{
			array[ -9 ] = (STRPTR) 10;
		}
	}
	else
	{
		array[ 0 ] = GSI(MSG_BOOKMARKLISTTREE_TITLE);
		array[ 1 ] = GSI(MSG_BOOKMARKLISTTREE_ADDRESS);
	}

	return (0);
}

DEFNEW
{
	obj = (Object *) DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_Listtree_DisplayHook,   &bookmarklisttree_displayfunc_hook,
		MUIA_Listtree_ConstructHook, &bookmarklisttree_constructfunc_hook,
		MUIA_Listtree_DestructHook,  &bookmarklisttree_destructfunc_hook,
		MUIA_Listtree_Format,        "BAR,",
		MUIA_Listtree_Title,         TRUE, /* XXX: has no effect -> report to stuntzi (MUI4 bug) */
		MUIA_List_Title,             TRUE, /*      this worksaround the above problem ;)         */
		MUIA_Listtree_DragDropSort,  TRUE,
		MUIA_List_DragSortable,      TRUE,
		MUIA_ContextMenu,            TRUE,
		MUIA_List_MinLineHeight,     (getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark) ? 18 : TAG_IGNORE,
	End;

	if (obj)
	{
		GETDATA;
		data->activetype=NULL;
		data->drop=NULL;
		data->drop_flags=0;
		data->old_entry=0;
		data->cMenu=NULL;

		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_MOUSEBUTTONS;
		data->ehnode.ehn_Priority = 1;
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

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

DEFMMETHOD(Show)
{
	ULONG rc;
	GETDATA;

	if ((rc = DOSUPER))
	{
		if(!data->added)
		{
			DoMethod( _win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
			data->added = TRUE;
		}
	}

	return rc;
}

DEFMMETHOD(Hide)
{
	GETDATA;

	if(data->added)
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode );
		data->added = FALSE;
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
	switch (msg->opg_AttrID)
	{
		case MA_OWB_ObjectType:
		{
			*msg->opg_Storage = MV_OWB_ObjectType_Bookmark;
		}
		return TRUE;

		case MA_OWB_URL:
		{
			char *url = NULL;
			struct MUIS_Listtree_TreeNode *active = (struct MUIS_Listtree_TreeNode *) DoMethod(obj, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);

			if(active)
			{
				struct treedata * current = (struct treedata *)active->tn_User;

				url = current->address;
			}

			*msg->opg_Storage = (ULONG) url;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFMMETHOD(DragReport)
{
	struct MUIS_Listtree_TestPos_Result r={ NULL, 0, 0, 0 };
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *td;

	GETDATA;
	
	if(msg->obj == obj)
	{
		/* Autoscroll list, when mouse is in upper or lower area */
		if(_isinobject(obj, msg->x, msg->y))
		{
			if( msg->y + 10 > _top(obj) + _height(obj))
			{
				if(!msg->update)
				{
					return MUIV_DragReport_Refresh;
				}

				SetAttrs(obj, MUIA_List_TopPixel, getv(obj, MUIA_List_TopPixel) + 10, TAG_DONE);
				return (MUIV_DragReport_Continue);
			}
			else if(_top(obj) + 20 > msg->y)
			{
				if(getv(obj, MUIA_List_TopPixel) >= 10)
				{
					if(!msg->update)
					{
						return MUIV_DragReport_Refresh;
					}

					SetAttrs(obj, MUIA_List_TopPixel, getv(obj, MUIA_List_TopPixel) - 10, TAG_DONE);
					return (MUIV_DragReport_Continue);
				}
			}
		}
		else
		{
			return MUIV_DragReport_Continue;
		}

		// Sort mode
		if (DoMethod(obj, MUIM_Listtree_TestPos, msg->x, msg->y, &r) && (r.tpr_TreeNode) )
		{
			if (r.tpr_TreeNode != data->drop_node || (r.tpr_Flags != data->drop_flags) )
			{
				// Entry under mouse change need refresh
				if(!msg->update) return(MUIV_DragReport_Refresh);

				data->drop_node = (struct MUIS_Listtree_TreeNode *) r.tpr_TreeNode;
				data->drop_flags = r.tpr_Flags;

				tn=(struct MUIS_Listtree_TreeNode *)r.tpr_TreeNode;
				if (tn)
				{
					td=(struct treedata *)tn->tn_User;
					if (td)
					{
						if ( data->drop_flags==MUIV_Listtree_TestPos_Result_Flags_Onto && !(data->drop_node->tn_Flags & TNF_LIST) )
						{
							// Don't allow
							DoMethod(obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, MUIV_Listtree_SetDropMark_Values_None);
							data->drop=NULL;
							//kprintf("###### Lock\n");
							return MUIV_DragReport_Lock;
						}
						else
						{
							//kprintf("###### Draw\n");
							data->drop = tn;
							DoMethod(obj, MUIM_Listtree_SetDropMark, r.tpr_ListEntry, r.tpr_Flags);
						}
					}
					else
					{//kprintf("###### DragReport: no td\n");
					}
				}
				else
				{//kprintf("###### DragReport: not en entry\n");
				}
			}
		}
		else
		{
			//kprintf("###### DragReport: not en entry\n");
			//put under
		}
	}

	return MUIV_DragReport_Continue;
}

DEFMMETHOD(DragBegin)
{
	GETDATA;
	data->dd_active = (struct MUIS_Listtree_TreeNode *) DoMethod(obj, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
	if (data->dd_active->tn_Flags & TNF_LIST) data->dd_type=NODEFLAG_GROUP;
	else data->dd_type=NODEFLAG_LINK;
	return DOSUPER;
}

DEFMMETHOD(DragFinish)
{
	LONG rc;
	GETDATA;
	data->dd_active=NULL;
	rc=DOSUPER;
	DoMethod(app, MUIM_Application_PushMethod,
		data->group_obj, 1,
		MM_Bookmarkgroup_AutoSave);
	return rc;
}

DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg;

	if((imsg = msg->imsg))
	{
		if(imsg->Class == IDCMP_MOUSEBUTTONS)
		{
			switch (imsg->Code & ~IECODE_UP_PREFIX)
			{
				case IECODE_MBUTTON:
				{
					if (!(imsg->Code & IECODE_UP_PREFIX))
					{
						struct MUIS_Listtree_TestPos_Result res={ NULL, 0, 0, 0 };

						if (_isinobject(obj, imsg->MouseX, imsg->MouseY) && DoMethod(obj, MUIM_Listtree_TestPos, imsg->MouseX, imsg->MouseY, &res) && (res.tpr_TreeNode))
						{
							struct MUIS_Listtree_TreeNode *tn = (struct MUIS_Listtree_TreeNode *)res.tpr_TreeNode;
							struct treedata *td = (struct treedata *)tn->tn_User;
							SetAttrs(obj, MUIA_Listtree_Active, tn, TAG_DONE);
							DoMethod(app, MM_OWBApp_AddBrowser, NULL, td->address, FALSE, NULL, FALSE, FALSE, TRUE);
						}
					}
             	    break;
				}

				default: ;
			}
		}
	}

	return 0;

}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	struct MUIS_Listtree_TestPos_Result res={ NULL, 0, 0, 0 };
	struct MUIS_Listtree_TreeNode *tn,*parent;
	struct treedata *td;
	Object *item;

	//kprintf("Bookmark Context menu build\n");

	if (data->cMenu)
	{
		MUI_DisposeObject(data->cMenu);
		data->cMenu = NULL;
	}

	if (DoMethod(obj, MUIM_Listtree_TestPos, msg->mx, msg->my, &res) && (res.tpr_TreeNode))
	{
		// on an entry
		tn=(struct MUIS_Listtree_TreeNode *)res.tpr_TreeNode;
		td=((struct treedata *)tn->tn_User);

		data->drop=tn;

		snprintf(data->cMenu_Title, sizeof(data->cMenu_Title)-1, "%s", td->title);

		if ( strlen(td->title)>(sizeof(data->cMenu_Title)-1) )
		{
			ULONG a=sizeof(data->cMenu_Title)-2;
			data->cMenu_Title[a--]='.';
			data->cMenu_Title[a--]='.';
			data->cMenu_Title[a--]='.';
		}

		if (tn->tn_Flags & TNF_LIST)
		{
			// Over a group
			data->cMenu=MenustripObject,
				MUIA_Family_Child, MenuObjectT( data->cMenu_Title ),
					MUIA_Family_Child, MenuitemObject,
						MUIA_Menuitem_Title, (tn->tn_Flags & TNF_OPEN) ? GSI(MSG_BOOKMARKLISTTREE_FOLD_GROUP_BOLD) : GSI(MSG_BOOKMARKLISTTREE_UNFOLD_GROUP_BOLD),
						MUIA_UserData, (tn->tn_Flags & TNF_OPEN) ? POPMENU_CLOSE : POPMENU_OPEN ,
	                    End,
					/*MUIA_Family_Child, MenuitemObject,
	                    MUIA_Menuitem_Title,  NM_BARLABEL,
						End,*/
	                End,
	            End;
		}
		else
		{
			// Over a link
			if (td->flags & NODEFLAG_BAR)
			{
				// Separator
				data->cMenu=MenustripObject,
					MUIA_Family_Child, MenuObjectT(GSI(MSG_BOOKMARKLISTTREE_SEPARATOR_LABEL)),
						End,
					End;
			}
			else
			{
				// Normal item
				data->cMenu=MenustripObject,
					MUIA_Family_Child, MenuObjectT( data->cMenu_Title ),
						MUIA_Family_Child, MenuitemObject,
							MUIA_Menuitem_Title, GSI(MSG_BOOKMARKLISTTREE_OPEN),
							MUIA_UserData, POPMENU_OPEN_CURRENT,
		                    End,
						MUIA_Family_Child, MenuitemObject,
							MUIA_Menuitem_Title, GSI(MSG_BOOKMARKLISTTREE_OPEN_NEW_TAB),
							MUIA_UserData, POPMENU_OPEN_NEWTAB,
		                    End,
						MUIA_Family_Child, MenuitemObject,
							MUIA_Menuitem_Title, GSI(MSG_BOOKMARKLISTTREE_OPEN_NEW_WINDOW),
							MUIA_UserData, POPMENU_OPEN_NEWWIN,
		                    End,
						/*MUIA_Family_Child, MenuitemObject,
		                    MUIA_Menuitem_Title,  NM_BARLABEL,
							End,*/
		                End,
		            End;
			}
			// check it parent group
			parent=(struct MUIS_Listtree_TreeNode *) DoMethod(obj, MUIM_Listtree_GetEntry, tn, MUIV_Listtree_GetEntry_Position_Parent, 0);
			if ( (parent) && (data->cMenu) )
			{
				item=MenuitemObject,
					MUIA_Menuitem_Title, (parent->tn_Flags & TNF_OPEN) ? GSI(MSG_BOOKMARKLISTTREE_FOLD_GROUP) : GSI(MSG_BOOKMARKLISTTREE_UNFOLD_GROUP),
					MUIA_UserData, (parent->tn_Flags & TNF_OPEN) ? POPMENU_CLOSE : POPMENU_OPEN ,
					End;
				if (item) DoMethod(data->cMenu, MUIM_Family_AddTail, item);   
			}
		}
	}
	else
	{
		// Not under a line menu
		data->cMenu=MenustripObject,
			MUIA_Family_Child, MenuObjectT(GSI(MSG_BOOKMARKLISTTREE_BOOKMARK)),
				End,
			End;
	}

	if (data->cMenu)
	{
		// Add global
		item=MenuitemObject,
			MUIA_Menuitem_Title, GSI(MSG_BOOKMARKLISTTREE_UNFOLD_ALL_GROUPS),
			MUIA_UserData, POPMENU_OPEN_ALL,
			End;
		if (item) DoMethod(data->cMenu, MUIM_Family_AddTail, item);

		item=MenuitemObject,
			MUIA_Menuitem_Title, GSI(MSG_BOOKMARKLISTTREE_FOLD_ALL_GROUPS),
			MUIA_UserData, POPMENU_CLOSE_ALL,
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
	struct treedata *td;
 
	//kprintf("Bookmark menu trig: %lx\n",udata);
	switch (udata)
    {
		case POPMENU_OPEN_CURRENT:
			//kprintf("Open link in current view\n");
			if (data->drop)	
			{
				Object *window;

				td=((struct treedata *)data->drop->tn_User);
				//kprintf("URL: %s\n", (td->address) ? td->address : (STRPTR)"(NULL)" );

				window = (Object *) getv(app, MA_OWBApp_ActiveWindow);
				if(window)
				{
					DoMethod(window, MM_OWBWindow_LoadURL, td->address, NULL);
				}
			}
			break;
		case POPMENU_OPEN_NEWTAB:
			//kprintf("Open link in new tab\n");
			if (data->drop)	
			{
				td=((struct treedata *)data->drop->tn_User);
				//kprintf("URL: %s\n", (td->address) ? td->address : (STRPTR)"(NULL)" );
				DoMethod(app, MM_OWBApp_AddBrowser, NULL, td->address, FALSE, NULL, FALSE, FALSE, TRUE);
			}
			break;
		case POPMENU_OPEN_NEWWIN:
			//kprintf("Open link in new window\n");
			if (data->drop)
			{
				td=((struct treedata *)data->drop->tn_User);
				//kprintf("URL: %s\n", (td->address) ? td->address : (STRPTR)"(NULL)" );
				DoMethod(app, MM_OWBApp_AddWindow, td->address, FALSE, NULL, FALSE, NULL, FALSE);
			}
			break;
		case POPMENU_OPEN:
			//kprintf("Unfold group\n");
			if (data->drop)	DoMethod(obj, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root, data->drop, 0);
			break;
		case POPMENU_OPEN_ALL:
			//kprintf("Unfold all group\n");
			if (data->drop) DoMethod(obj, MUIM_Listtree_Open, MUIV_Listtree_Open_ListNode_Root, MUIV_Listtree_Open_TreeNode_All, 0);
			break;
		case POPMENU_CLOSE:
			//kprintf("Fold group\n");
			if (data->drop)
			{
				if (!(data->drop->tn_Flags & TNF_LIST))
				{
					struct MUIS_Listtree_TreeNode *parent=(struct MUIS_Listtree_TreeNode *) DoMethod(obj, MUIM_Listtree_GetEntry, data->drop, MUIV_Listtree_GetEntry_Position_Parent, 0);
					//kprintf("Parent %08lx\n",parent);
					if (parent) DoMethod(obj, MUIM_Listtree_Close, MUIV_Listtree_Close_ListNode_Root, parent, 0);
				}
				else
				{
					DoMethod(obj, MUIM_Listtree_Close, MUIV_Listtree_Close_ListNode_Root, data->drop, 0);
				}
			}
			break;
		case POPMENU_CLOSE_ALL:
			//kprintf("Fold all group\n");
			if (data->drop) DoMethod(obj, MUIM_Listtree_Close, MUIV_Listtree_Close_ListNode_Root, MUIV_Listtree_Close_TreeNode_All, 0);
			break;
		default:
			;
			//kprintf("Bad Context menu return\n");
    }
	data->drop=NULL;
    return (ULONG)NULL;
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
			{   /*
				if ( name_match (tn->tn_Name, pattern ))
				{
					return tn;
				}
				else */
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
					struct treedata *td = ((struct treedata *)tn->tn_User);
					if ( name_match( tn->tn_Name, pattern ) || ( td != NULL && td->address != NULL && name_match( td->address, pattern ) ) )
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
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECSET
DECGET
DECMMETHOD(DragBegin)
DECMMETHOD(DragFinish)
DECMMETHOD(DragReport)
DECMMETHOD(HandleEvent)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECSMETHOD(Search)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Listtree, bookmarklisttreeclass)
