/*
 * Copyright 2010 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 * Copyright 2009 Polymere
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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mui/Listtree_mcc.h>
#include <clib/macros.h>

#include "gui.h"
#include "bookmarkgroupclass.h"

#define D(x)

#define LOC(a,b) b

struct Data
{
	struct treedata *node;
	STRPTR title;
	APTR   button;
	APTR   icon;
	ULONG  mode;
	APTR   cMenu; 
	char   cMenu_Title[32];
	APTR   pMenu;
};

static void build_content(Object *obj, struct Data *data)
{
	if (data->mode & MV_QuickLinkGroup_Mode_Button)
	{
		data->button = TextObject,
			MUIA_Font, MUIV_Font_Tiny,
			MUIA_Text_Contents, data->title,
			MUIA_Text_PreParse, "\033c",
			End;

		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_QuickLink)
		{
			data->icon = NewObject(getfaviconclass(), NULL, MA_FavIcon_Folder, data->node->flags & NODEFLAG_GROUP, MA_FavIcon_PageURL, data->node->address, MUIA_Weight, 0, TAG_DONE);
			DoMethod((Object *) data->icon, MUIM_Notify, MA_FavIcon_NeedRedraw, MUIV_EveryTime, obj, 1, MM_QuickLinkButtonGroup_Redraw);
		}
		else
		{
			data->icon = NULL;
		}

		set(obj, MUIA_Frame, MUIV_Frame_Button);
		set(obj, MUIA_FrameDynamic, FALSE);
		set(obj, MUIA_FrameVisible, TRUE);
		set(obj, MUIA_Background, MUII_ButtonBack);
	}
	else
	{
		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_QuickLink)
		{
			data->icon = NewObject(getfaviconclass(), NULL, MA_FavIcon_Folder, data->node->flags & NODEFLAG_GROUP, MA_FavIcon_PageURL, data->node->address, MUIA_Weight, 0, TAG_DONE);
			DoMethod((Object *) data->icon, MUIM_Notify, MA_FavIcon_NeedRedraw, MUIV_EveryTime, obj, 1, MM_QuickLinkButtonGroup_Redraw);
		}
		else
		{
			data->icon = ImageObject,
				NoFrame,
				MUIA_Image_Spec,     MUII_Network,
				MUIA_Image_FreeVert, TRUE,
				MUIA_ShowSelState,   TRUE,
				End;			
		}

		data->button = TextObject,
			MUIA_Font, MUIV_Font_Tiny,
			MUIA_Text_Contents, data->title,
			End;

		set(obj, MUIA_Frame, MUIV_Frame_Button);
		set(obj, MUIA_FrameDynamic, TRUE);
		set(obj, MUIA_FrameVisible, FALSE);
		set(obj, MUIA_Background, "");
	}
}

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_QuickLinkGroup_Mode:
			if ((data->mode & MV_QuickLinkGroup_Mode_Button) != (tag->ti_Data & MV_QuickLinkGroup_Mode_Button))
			{
				// Type change
				Object *OldIcon = (Object *) data->icon;
				Object *OldButton = (Object *) data->button;
				data->mode = tag->ti_Data;
				build_content(obj, data);
				if (data->button)
				{
					// Remove old objects from group
					DoMethod(obj, MUIM_Group_InitChange);

					if (OldButton)
					{
						DoMethod(obj, MUIM_Group_Remove, OldButton);
						DoMethod(OldButton, OM_DISPOSE);
					}
					if (OldIcon)
					{
						DoMethod(obj, MUIM_Group_Remove, OldIcon);
						DoMethod(OldIcon, OM_DISPOSE);
					}
					
					if (data->icon) DoMethod(obj, MUIM_Group_AddTail, data->icon);
					DoMethod(obj, MUIM_Group_AddTail, data->button);
					DoMethod(obj, MUIM_Group_ExitChange);
				}
				else
				{
					// Something fail so restore old one's
					data->icon = OldIcon;
					data->button = OldButton;
				}
				
			}
			data->mode = tag->ti_Data;
			break;
	}
	NEXTTAG
}

DEFNEW
{
	struct Data tmp = {NULL,NULL,NULL,NULL,0,NULL,"",NULL}, *data;
	struct TagItem *tags,*tag;
	STRPTR title;

	for (tags=((struct opSet *)msg)->ops_AttrList;(tag=NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
			case MA_QuickLinkGroup_Mode:
				tmp.mode = tag->ti_Data;
				break;
			case MA_QuickLinkButtonGroup_Node:
				tmp.node = (struct treedata *)tag->ti_Data;
				break;
        }
	}

	if (!tmp.node) return (NULL);

	if ((tmp.node->alias) && (*tmp.node->alias!='\0'))
	{
		title = tmp.node->alias;
	}
	else if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_QuickLink) // allow empty labels in that case
	{
		title = "";
	}
	else
	{
		title = GSI(MSG_QUICKLINKBUTTONGROUP_NO_NAME);
	}

	tmp.title = (STRPTR)malloc(strlen(title)+1);

	if (!tmp.title) return (NULL);

	strcpy(tmp.title,title);

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,
		MUIA_CycleChain, 1,
		MUIA_InputMode, MUIV_InputMode_RelVerify,  
		MUIA_UserData, tmp.node,
		MUIA_ShortHelp, TRUE,
        MUIA_ContextMenu, TRUE,
		MUIA_Draggable, tmp.node->flags & NODEFLAG_GROUP ? FALSE : TRUE,
		End;

	if(obj)
	{
		build_content(obj, &tmp);

		if(tmp.icon)
		{
			DoMethod(obj, OM_ADDMEMBER, tmp.icon);
		}

		DoMethod(obj, OM_ADDMEMBER, tmp.button);

		data = (struct Data *) INST_DATA(cl, obj);
		data->node = tmp.node;
		data->title = tmp.title;
		data->button = tmp.button;
		data->icon = tmp.icon;
		data->cMenu = tmp.cMenu;
		data->mode = tmp.mode;
		data->cMenu_Title[0] = '\0';
		//kprintf("QuickLinkButtonGroup End\n");

		if(data->node->flags & NODEFLAG_GROUP)
		{
			DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_QuickLinkButtonGroup_BuildMenu);
		}
		else
		{
			DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 3, MM_OWBWindow_LoadURL, data->node->address, NULL);
		}

	}
	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;
	free(data->title);

	if (data->cMenu)
		MUI_DisposeObject((Object *)data->cMenu);

	if (data->pMenu)
		MUI_DisposeObject((Object *)data->pMenu);

	return (DOSUPER);
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
		case MA_OWB_ObjectType:
		{
			*msg->opg_Storage = MV_OWB_ObjectType_QuickLink;
		}
		return TRUE;

		case MA_OWB_URL:
		{
			*msg->opg_Storage = (ULONG) data->node->address;
		}
		return TRUE;

		case MA_QuickLinkGroup_Mode:
		{
			*msg->opg_Storage = (ULONG) data->mode;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFTMETHOD(QuickLinkButtonGroup_Update)
{
	GETDATA;
	STRPTR title;

	if ((data->node->alias) && (*data->node->alias!='\0'))
	{
		title = data->node->alias;
	}
	else if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_QuickLink) // allow empty labels in that case
	{
		title = "";
	}
	else
	{
		title = GSI(MSG_QUICKLINKBUTTONGROUP_NO_NAME);
	}

	//XXX: remove/add button if needed

	free(data->title);
	data->title = (STRPTR) malloc(strlen(title)+1);

	// Use up-to-date address ptr
	DoMethod(obj, MUIM_KillNotify, MUIA_Pressed);
    DoMethod(obj, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 3, MM_OWBWindow_LoadURL, data->node->address, NULL);

	if (data->title)
	{
		strcpy(data->title, title);
		set(data->button, MUIA_Text_Contents, data->title);
	}
	return 0;
}

struct popupmenu_entry
{
	struct MinNode n;
	int  type;
	char *address;
	char *title;
	APTR icon;
};

static ULONG build_menu(Object *menu, Object *lt, struct MUIS_Listtree_TreeNode *list, struct MinList *menuentry_list)
{
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *node;
	ULONG  pos = 0;
	Object *item, *submenu;
	ULONG count = 0;

	for(pos=0; ; pos++)
	{
		if( (tn=(struct MUIS_Listtree_TreeNode *)DoMethod(lt, MUIM_Listtree_GetEntry, list, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
		{
			node = (struct treedata *)tn->tn_User;

			if(node)
			{
				if(node->flags & NODEFLAG_GROUP)
				{
					submenu = (Object *)NewObject(getmenuclass(), NULL, MUIA_Menu_Title, strdup(node->title ? node->title : ""), TAG_DONE);

					if(submenu)
					{
						// Add menu
						DoMethod(menu, MUIM_Family_AddTail, submenu);
						count += build_menu(submenu, lt, tn, menuentry_list);
					}
				}
				else
				{
					if(node->flags & NODEFLAG_BAR)
					{
						item = (Object *)NewObject(getmenuitemclass(), NULL,
												   MUIA_Menuitem_Title,          NM_BARLABEL,
												   MUIA_UserData,            NULL,
												   MA_MenuItem_FreeUserData, FALSE,
												   TAG_DONE);
						if(item)
						{
							// Add separator
							DoMethod(menu, MUIM_Family_AddTail, item);
						}

						count++;
					}
					else
					{
						struct popupmenu_entry *entry = (struct popupmenu_entry *) malloc(sizeof(*entry));

						if(entry)
						{
							//char label[128];
							WTF::String truncatedTitle = node->title ? node->title : "";

							if(truncatedTitle.length() > 64)
							{
								truncatedTitle.truncate(64);
								truncatedTitle.append("...");
							}

							/*
							if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_QuickLink)
							{
								entry->icon = NewObject(getfaviconclass(), NULL,
								  MA_FavIcon_Folder, node->address == NULL,
								  MA_FavIcon_PageURL, node->address,
								  MUIA_Weight, 0,
								  TAG_DONE);

								snprintf(label, sizeof(label), "\033O[%08lx] %s", (ULONG) entry->icon, truncatedTitle.latin1().data());
							}
							else
							{
	                            entry->icon = NULL;
								snprintf(label, sizeof(label), "%s", truncatedTitle.latin1().data());
							}
							*/

							entry->type    = POPMENU_OPEN_CURRENT;
							entry->icon    = NULL;
							entry->address = strdup(node->address ? node->address : "");
							entry->title   = strdup(truncatedTitle.latin1().data());

							ADDTAIL(menuentry_list, entry);

							item = (Object *)NewObject(getmenuitemclass(), NULL,
								MUIA_Menuitem_Title,      strdup(entry->title),
								MUIA_UserData,            entry,
								MA_MenuItem_FreeUserData, FALSE,
								TAG_DONE);

							if(item)
							{
								// Add link
								DoMethod(menu, MUIM_Family_AddTail, item);
							}

							/*
							if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_QuickLink)
							{
								if(entry->icon && menuobj)
								{
									DoMethod((Object *) entry->icon, MUIM_Notify, MA_FavIcon_NeedRedraw, MUIV_EveryTime, obj, 2, MM_QuickLinkButtonGroup_RedrawMenuItem, entry->icon);
								}
							}
							*/

							count++;
						}
					}
				}
			}
		}
		else
		{
			break;
		}
	}

	return count;
}

DEFTMETHOD(QuickLinkButtonGroup_BuildMenu)
{
	GETDATA;
	Object *menulistgroup;

	if (data->pMenu)
	{
		MUI_DisposeObject((Object *) data->pMenu);
		data->pMenu = NULL;
	}

	data->pMenu = (Object *) MenustripObject,
							 Child, menulistgroup = (Object *)NewObject(getmenuclass(), NULL, MUIA_Menu_Title, strdup(data->title), TAG_DONE),
							 End;

	if(data->pMenu)
	{
		if(data->node->treenode)
		{
			ULONG rc;
			ULONG count = 0;
			APTR n, m;
            struct MinList menuentry_list;

			// Will include all menu entries (including submenus items)
            NEWLIST(&menuentry_list);

			count = build_menu(menulistgroup, (Object *) data->node->tree, (struct MUIS_Listtree_TreeNode *) data->node->treenode, &menuentry_list);

			// Finish it...
			if(count)
			{
				Object *item;

				struct popupmenu_entry *entry = (struct popupmenu_entry *) malloc(sizeof(*entry));

				if(entry)
				{
					char label[64];
					snprintf(label, sizeof(label), "%s", GSI(MSG_QUICKLINKBUTTONGROUP_OPEN_ALL));

					entry->type = POPMENU_OPEN_ALL;
					entry->icon = NULL;
					entry->address = NULL;
					entry->title = strdup(label);

					item = (Object *)NewObject(getmenuitemclass(), NULL,
						MUIA_Menuitem_Title,      NM_BARLABEL,
						MUIA_UserData,            NULL,
						MA_MenuItem_FreeUserData, FALSE,
						TAG_DONE);

					if(item)
					{
						DoMethod(menulistgroup, MUIM_Family_AddTail, item);
					}

					item = (Object *)NewObject(getmenuitemclass(), NULL,
						MUIA_Menuitem_Title,      strdup(entry->title),
						MUIA_UserData,            entry,
                        MA_MenuItem_FreeUserData, FALSE,
						TAG_DONE);

					if(item)
					{
						DoMethod(menulistgroup, MUIM_Family_AddTail, item);
					} 
				}
			}

			rc = DoMethod((Object *) data->pMenu, MUIM_Menustrip_Popup, obj, 0, _left(obj), _top(obj));

			if(rc)
			{
				struct popupmenu_entry *entry = (struct popupmenu_entry *) rc;

				if(entry->type == POPMENU_OPEN_CURRENT)
				{
					DoMethod(_win(obj), MM_OWBWindow_LoadURL, entry->address, NULL);
				}
				else if(entry->type == POPMENU_OPEN_ALL)
				{
					ITERATELISTSAFE(n, m, &menuentry_list)
					{
						struct popupmenu_entry *e = (struct popupmenu_entry *) n;
						if(e->address)
						{
							DoMethod(app, MM_OWBApp_AddBrowser, NULL, e->address, FALSE, NULL, TRUE, FALSE, TRUE);
						}
					}
				}
			}

			MUI_DisposeObject((Object *) data->pMenu);
			data->pMenu = NULL;

			ITERATELISTSAFE(n, m, &menuentry_list)
			{
				struct popupmenu_entry *entry = (struct popupmenu_entry *) n;
				
				REMOVE(n);
				if(entry->icon)
					MUI_DisposeObject((Object *) entry->icon);
				free(entry->address);
				free(entry->title);
				free(n);
			}
		}
	}

	return 0;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	if (data->cMenu)
	{
		MUI_DisposeObject((Object *)data->cMenu);
		data->cMenu = NULL;
	}

	snprintf(data->cMenu_Title, sizeof(data->cMenu_Title)-1, "%s", data->node->title);

	if ( strlen(data->node->title)>(sizeof(data->cMenu_Title)-1) )
	{
		ULONG a=sizeof(data->cMenu_Title)-2;
		data->cMenu_Title[a--] = '.';
		data->cMenu_Title[a--] = '.';
		data->cMenu_Title[a--] = '.';
	}

	data->cMenu=MenustripObject,
		MUIA_Family_Child, MenuObjectT( data->cMenu_Title ),
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_QUICKLINKBUTTONGROUP_OPEN),
				MUIA_UserData, POPMENU_OPEN_CURRENT,
				End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_QUICKLINKBUTTONGROUP_OPEN_NEW_TAB),
				MUIA_UserData, POPMENU_OPEN_NEWTAB,
				End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_QUICKLINKBUTTONGROUP_OPEN_NEW_WINDOW),
				MUIA_UserData, POPMENU_OPEN_NEWWIN,
				End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title,  NM_BARLABEL,
				End,
			MUIA_Family_Child, MenuitemObject,
				MUIA_Menuitem_Title, GSI(MSG_QUICKLINKBUTTONGROUP_REMOVE),
				MUIA_UserData, POPMENU_REMOVE,
				End,
			End,
		End;
	
	return (ULONG)data->cMenu;
}

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;
	ULONG udata = muiUserData(msg->item);

	switch (udata)
    {
		case POPMENU_OPEN_CURRENT:
		{
			Object *window = (Object *) getv(app, MA_OWBApp_ActiveWindow);
			if(window && data->node->address)
			{
				DoMethod(window, MM_OWBWindow_LoadURL, data->node->address, NULL);
			}

			break;
		}
		case POPMENU_OPEN_NEWTAB:
			if(data->node->address)
			{
				DoMethod(app, MM_OWBApp_AddBrowser, NULL, data->node->address, FALSE, NULL, FALSE, FALSE, TRUE);
			}
			break;
		case POPMENU_OPEN_NEWWIN:
			if(data->node->address)
			{
				DoMethod(app, MM_OWBApp_AddWindow, data->node->address, FALSE, NULL, FALSE, NULL, FALSE);
			}
			break;
		case POPMENU_REMOVE:
			DoMethod(app, MM_Bookmarkgroup_RemoveQuickLink, data->node);
			break;
		default:
			;
    }
    return (ULONG)NULL;
}

DEFMMETHOD(CreateShortHelp)
{
	GETDATA;  
	STRPTR help;
	ULONG title_len = (data->node->address) ? strlen(data->node->title) : 0;
	ULONG address_len = (data->node->address) ? strlen(data->node->address) : 0;

	if ((address_len) && (title_len))
	{
		help = (STRPTR)malloc(address_len+title_len+1);
		if (help)
		{
			sprintf(help, "%s\n%s", data->node->title, data->node->address);
			return ((ULONG)help);
		}
	}
	return (0);
}

DEFMMETHOD(DeleteShortHelp)
{
	free(msg->help);
    return 0;
}

DEFTMETHOD(QuickLinkButtonGroup_Redraw)
{
	GETDATA;

	if(muiRenderInfo(obj))
	{
		D(kprintf("QuickLinkButtonGroup_Redraw <%s>\n", data->node->address));
		MUI_Redraw(obj, MADF_DRAWOBJECT);

		if(data->icon)
		{
			MUI_Redraw((Object *) data->icon, MADF_DRAWOBJECT);
		}
	}

	return 0;
}

DEFSMETHOD(QuickLinkButtonGroup_RedrawMenuItem)
{
	GETDATA;

	if(data->pMenu)
	{
		D(kprintf("QuickLinkButtonGroup_RedrawMenu\n"));
		MUI_Redraw((Object *) msg->obj, MADF_DRAWOBJECT);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECTMETHOD(QuickLinkButtonGroup_Update)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECMMETHOD(CreateShortHelp)
DECMMETHOD(DeleteShortHelp)
DECTMETHOD(QuickLinkButtonGroup_Redraw)
DECSMETHOD(QuickLinkButtonGroup_RedrawMenuItem)
DECTMETHOD(QuickLinkButtonGroup_BuildMenu)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, quicklinkbuttongroupclass)

