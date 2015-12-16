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

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gui.h"
#include "bookmarkgroupclass.h"

#define D(x)

/* public */
#include <mui/Listtree_mcc.h>
#include <proto/dos.h>
#include <clib/macros.h>


/* private */

struct Data {
	Object *lv_bookmark;
	Object *lt_bookmark;
	Object *win;
	Object *findgroup;
};

#define LOC(a,b) (b)

extern int parsevarpool(STRPTR var,char *temp,...);

static void doset(Object *obj, struct Data *data, struct TagItem *tags)
{
	struct MUIS_Listtree_TreeNode *active;
	struct treedata *current;
	LONG  line=0;

	FORTAG(tags)
	{
		case MA_Bookmarkgroup_Title:
			active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
			if(active)
			{
				current = (struct treedata *)active->tn_User;

				if (current->title) FreeVecTaskPooled(current->title);
				current->title = (STRPTR)AllocVecTaskPooled(strlen((STRPTR)tag->ti_Data)+1);
				strcpy(current->title, (STRPTR)tag->ti_Data);

				DoMethod(data->lt_bookmark, MUIM_Listtree_Rename, active, current->title, NULL);
				line = DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, active, 0L);
				DoMethod(data->lt_bookmark, MUIM_List_Redraw, line, NULL);
			}
			break;

		case MA_Bookmarkgroup_Alias:
			break;

		case MA_Bookmarkgroup_Address:
			active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
			if(active)
			{
				current = (struct treedata *)active->tn_User;

				if (current->address) FreeVecTaskPooled(current->address);
				current->address = (STRPTR)AllocVecTaskPooled(strlen((STRPTR)tag->ti_Data)+1);
				if (current->address) strcpy(current->address, (STRPTR)tag->ti_Data);

				line = DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, active, 0L);
				DoMethod(data->lt_bookmark, MUIM_List_Redraw, line, NULL);
			}
			break;

		case MA_Bookmarkgroup_InMenu:
			active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
			if(active)
			{
				current = (struct treedata *)active->tn_User;
				D(kprintf( "InMenu before is %08lx.\n",(ULONG)current->flags));
				if (tag->ti_Data) current->flags = (current->flags & ~NODEFLAG_INMENU) | NODEFLAG_INMENU;
				else current->flags = current->flags & ~NODEFLAG_INMENU;
				D(kprintf( "InMenu after is %08lx.\n",(ULONG)current->flags));
				line = DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, active, 0L);
				DoMethod(data->lt_bookmark, MUIM_List_Redraw, line, NULL);
			}
			break;

		case MA_Bookmarkgroup_QuickLink:
			break;
	}
	NEXTTAG
}

DEFNEW
{
	struct Data tmp;
	Object *closebutton, *findgroup;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, HGroup,
				Child, closebutton = ImageObject,
                        MUIA_Frame, MUIV_Frame_ImageButton,
						MUIA_CustomBackfill, TRUE,
						MUIA_InputMode, MUIV_InputMode_RelVerify,
						MUIA_Image_Spec, MUII_Close,
						End,
				Child, TextObject,
						MUIA_Text_SetMax, FALSE,
						MUIA_Text_SetMin, FALSE,
						MUIA_Text_Contents, GSI(MSG_BOOKMARKLISTTREE_BOOKMARK),
						End,
				End,
		Child, tmp.lv_bookmark = ListviewObject,
					MUIA_CycleChain, TRUE,
					MUIA_Listview_List, tmp.lt_bookmark = (Object *) NewObject(getbookmarklisttreeclass(), NULL, TAG_DONE),
					End,

		Child, findgroup = (Object *) NewObject(getfindtextclass(), NULL,
												MUIA_ShowMe, TRUE,
												MA_FindText_Closable, TRUE,
												MA_FindText_ShowButtons, FALSE,
												MA_FindText_ShowCaseSensitive, FALSE,
												MA_FindText_ShowText, FALSE,
												TAG_DONE),

		TAG_MORE, INITTAGS
		);

	if (!obj)
	{
		return (NULL);
	}

	GETDATA;
	data->lv_bookmark = tmp.lv_bookmark;
	data->lt_bookmark = tmp.lt_bookmark;
	data->win = NULL;
	data->findgroup = findgroup;

	set(data->findgroup, MA_FindText_Target, data->lt_bookmark);

	DoMethod(closebutton, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Window, 1, MM_OWBWindow_RemoveBookmarkPanel);

	DoMethod(data->lt_bookmark, MUIM_Notify, MUIA_Listtree_DoubleClick, MUIV_EveryTime, obj, 1, MM_Bookmarkgroup_Open);

	DoMethod(obj, MM_Bookmarkgroup_LoadHtml, BOOKMARK_PATH);

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

static void handle_favicons(struct IClass *cl, Object *obj, struct MUIS_Listtree_TreeNode *list, bool create)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *node;
	ULONG  pos=0;
	Object *lt = data->lt_bookmark;

	for(pos=0; ; pos++)
	{
		if( (tn = (struct MUIS_Listtree_TreeNode *) DoMethod(lt, MUIM_Listtree_GetEntry, list, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
		{
			node = (struct treedata *) tn->tn_User;

			if(node)
			{
				if(tn->tn_Flags & TNF_LIST)
				{
					handle_favicons(cl, obj, tn, create);
				}
				if(node->icon && muiRenderInfo(lt))
				{
					if(create)
					{
						node->iconimg = (APTR) DoMethod((Object *) lt, MUIM_List_CreateImage, node->icon, 0);
					}
					else
					{
						if(node->iconimg)
						{
							DoMethod((Object *) lt, MUIM_List_DeleteImage, node->iconimg);
							node->iconimg = NULL;
						}
					}
				}
			}
		}
		else
			break;
	}
}

DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;
	GETDATA;

	if (rc && _win(obj))
	{
		data->win=_win(obj);
		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark)
			handle_favicons(cl, obj, MUIV_Listtree_GetEntry_ListNode_Root, true);
	}
	return rc;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;
	handle_favicons(cl, obj, MUIV_Listtree_GetEntry_ListNode_Root, false);
	data->win=NULL;
	return DOSUPER;
}

DEFSMETHOD(Bookmarkgroup_LoadHtml)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *group = NULL;
	struct treedata newnode;
	char   line[8192];
	STRPTR ptr[8]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	BPTR  html;
	ULONG found,type = 0,order;
	LONG  p,q;

	DoMethod(data->lt_bookmark, MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root, MUIV_Listtree_Remove_TreeNode_All, 0);

	html = Open((char *) msg->file, MODE_OLDFILE);
	if (html)
	{
		FGets(html, line, sizeof(line)-1); // Version string
		if (strstr(line, "<!-- OWB MorphOS Bookmark V1 -->"))
		{
			type = MV_Bookmark_SaveHtml_OWB;
			//printf("Load bookmark in OWB format for file %s\n", msg->file);
		}
		if (strstr(line, "<!-- IBrowse Hotlist V2 -->"))
		{
			type = MV_Bookmark_SaveHtml_IBrowse;
			//printf("Load bookmark in IBrowse format for file %s\n", msg->file);
		}

		if (type == 0)
		{
			Close(html);
			printf("Unknow type of bookmark. Only IBrowse 2.4 and OWB MorphOS supported.\n");
			return (MV_Bookmark_ImportHtml_WrongType);
		}

		set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE);

		while ( FGets(html, line, sizeof(line)-1) )
		{
			found=FALSE;
			p=parsevarpool(line, "%.<LI><A HREF=\"%s\"%s>%s</A>", &ptr[0],&ptr[1],&ptr[2]);
			p--;
			if (p == 3)
			{
				q=0;
				D(kprintf( "Parse Link: '%s'\n", ptr[2]));

				if (strcmp(ptr[2],"----------------------------------------")==0)
				{
					newnode.flags    = NODEFLAG_BAR;
					newnode.address  = NULL;
					newnode.title    = "";
					newnode.alias    = NULL;
					newnode.ql_order = 0;
					newnode.icon     = NULL;
					newnode.iconimg  = NULL;
					newnode.tree     = data->lt_bookmark;
				}
				else
				{
					newnode.flags    = NODEFLAG_LINK;
					newnode.address  = ptr[0];
					newnode.title    = ptr[2];
					newnode.ql_order = 0;
					newnode.icon     = NULL;
					newnode.iconimg  = NULL;
					newnode.tree     = data->lt_bookmark;
				}
				
				newnode.alias = NULL;
				
				q = parsevarpool(ptr[1], "%.IBSHORTCUT=\"%s\"", &ptr[3]);
				if (q == 2)
				{
					// Alias
					D(kprintf( "    Alias: '%s'\n", ptr[3]));
					newnode.alias = ptr[3];
					p++;
				}
				else //if (type==MV_Bookmark_SaveHtml_OWB)
				{
					// Try OWB Quick Link
					q = parsevarpool(ptr[1], "%.OWBQUICKLINK=\"%s\"", &ptr[3]);
					if (q == 2)
					{
						D(kprintf( "QuickLink: '%s'\n", ptr[3]));
						newnode.alias = ptr[3];
						p++;
						newnode.flags |= NODEFLAG_QUICKLINK;

						q = parsevarpool(ptr[1], "%.OWBQLORDER=\"%d\"", &order);
						if (q == 2)
						{
							newnode.ql_order = order;
							D(kprintf("Order: %ld\n",order));
						}
						else
						{
							D(kprintf("Fail get order '%s'\n", ptr[1]));
						}
					}
					else newnode.alias = NULL;
				}

				q = parsevarpool(ptr[1], "%.IBHLFLAGS=\"%s\"", &ptr[4]);
				if (q == 2)
				{
					D(kprintf( "Flags: '%s'\n", ptr[4]));
					FreeVecTaskPooled(ptr[4]); ptr[4] = NULL;
				}
				else newnode.flags |= NODEFLAG_INMENU;

				// Add entry
				DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
					newnode.title,
					&newnode,
					group ? group : MUIV_Listtree_Insert_ListNode_Root,
					MUIV_Listtree_Insert_PrevNode_Tail,
					0
					);
				found = TRUE;
			}
			if (p > 0) { for(p--; p>=0; p--) { FreeVecTaskPooled(ptr[p]); ptr[p] = NULL; } p = 0; }
			if (!found)
			{
				if (p>0) { for(p--; p>=0; p--) { FreeVecTaskPooled(ptr[p]); ptr[p] = NULL; } p = 0; }
				p = parsevarpool(line, "%.<LI%s><B>%s</B><UL>", &ptr[0], &ptr[1]);
				p--;
				if (p == 2)
				{
					D(kprintf( "Parse Group: '%s'\n", ptr[1]));
					newnode.flags    = NODEFLAG_GROUP;
					newnode.title    = ptr[1];
					newnode.alias    = NULL;
					newnode.address  = NULL;
					newnode.ql_order = 0;
					newnode.icon     = NULL;
					newnode.iconimg  = NULL;
					newnode.tree     = data->lt_bookmark;

					q = parsevarpool(ptr[0], "%.IBHLFLAGS=\"%s\"", &ptr[4]);
					if (q == 2)
					{
						D(kprintf( "Flags: '%s'\n", ptr[4]));
						FreeVecTaskPooled(ptr[4]); ptr[4] = NULL;
					}
					else newnode.flags |= NODEFLAG_INMENU;
 
					if (group)
					{
						D(kprintf("Only one level of group supported, add group at root\n"));
					}

					// Add group
					group= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
						newnode.title,
						&newnode,
						group ? group : MUIV_Listtree_Insert_ListNode_Root,
						MUIV_Listtree_Insert_PrevNode_Tail,
						TNF_LIST
						);

					found = TRUE;
				}
			}
			if (p > 0) { for(p--; p>=0; p--) { FreeVecTaskPooled(ptr[p]); ptr[p] = NULL; } p = 0; }
			if (!found)
			{
				if (strstr(line, "</UL>"))
				{
					D(kprintf( "Parse End Group\n"));
					if(group != NULL)
					{
						group = (MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, group, MUIV_Listtree_GetEntry_Position_Parent, 0);
					}
					found = TRUE;
				}
			}
			if (p>0) { for(p--; p>=0; p--) { FreeVecTaskPooled(ptr[p]); ptr[p] = NULL; } p = 0; }
		}

		set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);
		
		Close(html);

		return (MV_Bookmark_ImportHtml_OK);
	}
	return (MV_Bookmark_ImportHtml_NoFile);
}

DEFSMETHOD(Bookmarkgroup_AddLink)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *active = NULL, *parent = NULL;
	struct treedata newnode,*node;

	newnode.flags    = NODEFLAG_LINK;
	newnode.title    = (char*) msg->title;
	newnode.alias    = (char*) msg->alias;
	newnode.address  = (char*) msg->address;
	newnode.ql_order = 0;
	newnode.icon     = NULL;
	newnode.iconimg  = NULL;
	newnode.tree     = data->lt_bookmark;

	D(kprintf("AddLink title %s %08lx %08lx\n",msg->title, (ULONG)msg->alias, (ULONG)msg->address));

	if (!msg->external)
	{
		active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
		D(kprintf("Active is %08lx.\n",(ULONG)active));
	}

	// Disable notifications
	set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE);

	if(active)
	{
		node = (struct treedata *)active->tn_User;

		if (node->flags & NODEFLAG_GROUP)
		{
			// Add link in current group
			DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
				msg->title,
				&newnode,
				active,
				MUIV_Listtree_Insert_PrevNode_Tail,
				0
			);

		}
		else
		{
			parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);
			if (parent)
			{
				DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
					msg->title,
					&newnode,
					parent,
					active,
					0
					);
			}
			else
			{
				DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
					msg->title,
					&newnode,
					MUIV_Listtree_Insert_ListNode_Root,
					active,
					0
					);
			}
		}
	}
	else
	{
		// Add link at root
		DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
			msg->title,
			&newnode,
			MUIV_Listtree_Insert_ListNode_Root,
			MUIV_Listtree_Insert_PrevNode_Tail,
			0
		);
	}

	set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);

	return (0);
}

DEFTMETHOD(Bookmarkgroup_Remove)
{
	/*
	struct MUIS_Listtree_TreeNode *tn,*tn2;
	struct treedata *td;
	LONG pos;
	GETDATA;
	
	tn=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
	if (tn)
	{
		// Check if it is a group
		if(tn->tn_Flags & TNF_LIST)
		{
			for(pos=0; ; pos++)
			{
				if( (tn2=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, tn, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
				{
					td=(struct treedata *)tn2->tn_User;
					if ((td) && (td->flags & NODEFLAG_QUICKLINK))
					{
						DoMethod(obj, MM_Bookmarkgroup_RemoveQuickLink, td);
					}
				}
				else break;
			}
		}
	}

	set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE);
	DoMethod(data->lt_bookmark, MUIM_Listtree_Remove, NULL, MUIV_Listtree_Remove_TreeNode_Active, 0);
	set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);
	*/
	return (0);
}

DEFMMETHOD(DragQuery)
{
	LONG type = getv(msg->obj, MA_OWB_ObjectType);

	if (type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_Tab || type == MV_OWB_ObjectType_URL)
	{
		return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}

DEFMMETHOD(DragDrop)
{
	STRPTR url   = (STRPTR) strdup((STRPTR) getv(msg->obj, MA_OWB_URL));
	STRPTR title = NULL;
	LONG type    = getv(msg->obj, MA_OWB_ObjectType);

	if(type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_Tab)
	{
		char *tmp = (STRPTR) getv(msg->obj, MA_OWB_Title);
		if(tmp)
		{
			title = (STRPTR) strdup(tmp);
		}
	}

	if(url)
	{
		DoMethod(app, MUIM_Application_PushMethod, _win(obj), 4, MM_OWBWindow_InsertBookmarkAskTitle, url, title, FALSE);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECSMETHOD(Bookmarkgroup_LoadHtml)
DECSMETHOD(Bookmarkgroup_AddLink)
DECTMETHOD(Bookmarkgroup_Remove)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, bookmarkpanelgroupclass)
