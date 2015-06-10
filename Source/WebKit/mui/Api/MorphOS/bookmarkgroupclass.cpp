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

#include "config.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include "URL.h"
#include "IconDatabase.h"

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gui.h"
#include "bookmarkgroupclass.h"

#ifndef get
#define get(obj,attr,store) GetAttr(attr,obj,(ULONGPTR)store)
#endif

#define D(x)

/* public */
#include <mui/Listtree_mcc.h>
#include <proto/dos.h>
#include <clib/macros.h>

using namespace WebCore;

/* private */

struct qlgroupnode
{
	struct MinNode node;
	Object *obj;
};

struct Data {
	Object *lv_bookmark;
	Object *lt_bookmark;
	Object *bt_addgroup;
	Object *bt_addlink;
	Object *bt_remove;
	Object *st_title;
	Object *st_alias;
	Object *st_address;
	Object *ch_showmenu;
	Object *bt_separator;
	Object *ch_quicklink;
	Object *lt_linklist;
	Object *bt_removeql;
	ULONG	save_in_progress;
	struct NotifyRequest *notifyreq;
	struct MinList qlgroups;
	Object *win;
	Object *findgroup;
	ULONG nofocuschange;
};

#define LOC(a,b) (b)

/*
result=parsevar("Text: a parser! 150,50", "%s: %s! %d,%.", &s1, &s2, &n)
-> result=4 s1="Text" s2="a parser" n=150
%s for store a string
%d for an integer (limit atoi)
%. for ignore this part but increment the count result
Warning: the parser string MUST start with %
*/
int parsevarpool(STRPTR var,char *temp,...)
{
    va_list pp;
    char    key[20],p[4096];
    int     l,lkey,pkey,type,lp,next,n=0;
	int    *arg_int;
	STRPTR *arg_string;
	
	va_start(pp,temp);
	//PF("## parse var %s with %s = ",var,temp);
    while (*temp)
    {
        if (*temp == '%')
        {
            switch (*++temp)
            {
                case 'd' : { type=1; break; }
                case 's' : { type=2; break; }
                case '.' : { type=-1; break; }
                default  : type=0;
            }
            lkey=0;
            while (*++temp)
            {
                if (*temp=='%') break;
                key[lkey++]=*temp;
            }
            key[lkey]='\0';
			//PF(" >Type %ld Key %s\n",type,&key[0]);
            lp=0;
            pkey=0;
            next=0;
			//PF("*var=%ld pkey=%ld lkey=%ld\n",*var,pkey,lkey);
            while (next==0)
            {
                p[lp++]=*var;
                if (*var==key[pkey]) pkey++;
                else pkey=0;
                if (*var!=0) var++;
				//else next=1;

				//PF("    *var=%ld pkey=%ld lkey=%ld\n",*var,pkey,lkey);
				if ((pkey==lkey && lkey>0) || *var==0)
                {
					if (pkey==lkey && lkey>0) p[lp-lkey]='\0';
					else p[lp]='\0';
					//D(kprintf("Found Key, string is %s\n",&p[0]));
                    switch (type)
                    {
                        case -1 :
							//PF("Ignored...\n");
                            n++;
                            break;
                        case 1 :
							l=0;
							l=atoi(&p[0]);
							
							arg_int=NULL;
                            arg_int = va_arg(pp, int *);
							//D(kprintf("Valeur int: %ld arg is %08lx\n", l, arg_int));
							if (arg_int)
							{	
								*arg_int=l;
								n++;
							}
                            break;
                        case 2 :
							//PF("Chaine:\n'%s'\n\n",&p[0]);
							arg_string=va_arg(pp, STRPTR *);
							// if (*arg_string!=NULL) free(*arg_string);
							// NEVER FREE something you don't allow yourself
							*arg_string= (STRPTR) AllocVecTaskPooled(strlen(&p[0])+1);
                            strcpy(*arg_string,&p[0]);
                            n++;
							//PF("AllocVecPooled Addr: %08lx Len: %ld\n",*arg_string,strlen(&p[0])+1);
							break;
                    }
					if (*var==0)
					{
						//D(kprintf("end if string parse var return %ld\n",n));
						return n;
					}
					else next=1;
                }
            }
        }
        else
        {
			//PUTS(" >Bad parse string need %% at fist caracters.");
            temp++;
        }
    }
    va_end(pp);
	//D(kprintf("parse var return %ld\n",n));
    //if (strlen(temp)) PF("#### Parsevar end with unparse \"%s\"\n",temp);
    return(n);
}
 

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
				current=(struct treedata *)active->tn_User; 	
				if (current->title) FreeVecTaskPooled(current->title);
				current->title=(STRPTR)AllocVecTaskPooled(strlen((STRPTR)tag->ti_Data)+1);
				strcpy(current->title, (STRPTR)tag->ti_Data);
				DoMethod(data->lt_bookmark, MUIM_Listtree_Rename, active, current->title, NULL);
				line=DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, active, 0L);
				DoMethod(data->lt_bookmark, MUIM_List_Redraw, line, NULL);
				DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);
 
			}
			break;
		case MA_Bookmarkgroup_Alias:
			current=NULL;
			D(kprintf("Set Alias '%s'\n", tag->ti_Data));
			if (DoMethod(data->lt_linklist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &current) && (current))
			{
				APTR n;

				if (current->alias) FreeVecTaskPooled(current->alias);
				current->alias=(STRPTR)AllocVecTaskPooled(strlen((STRPTR)tag->ti_Data)+1);
				if (current->alias)	strcpy(current->alias, (STRPTR)tag->ti_Data);

				ITERATELIST(n, &data->qlgroups)
				{
					struct qlgroupnode * qn = (struct qlgroupnode *) n;

					if(qn->obj) DoMethod(app, MUIM_Application_PushMethod,
						qn->obj, 2 | MUIV_PushMethod_Delay(300) | MUIF_PUSHMETHOD_SINGLE,
						MM_QuickLinkGroup_Update, current);
				}
				
				DoMethod(data->lt_linklist, MUIM_List_Redraw, MUIV_List_Redraw_Active);
				DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);
			}
			break;
		case MA_Bookmarkgroup_Address:
			active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
			if(active)
			{
				APTR n;

				current=(struct treedata *)active->tn_User; 	
				if (current->address) FreeVecTaskPooled(current->address);
				current->address=(STRPTR)AllocVecTaskPooled(strlen((STRPTR)tag->ti_Data)+1);
				if (current->address) strcpy(current->address, (STRPTR)tag->ti_Data);
				line=DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, active, 0L);
				DoMethod(data->lt_bookmark, MUIM_List_Redraw, line, NULL);

				// Also update quicklinks, since they use this node address ptr directly...
				ITERATELIST(n, &data->qlgroups)
				{
					struct qlgroupnode * qn = (struct qlgroupnode *) n;

					if(qn->obj) DoMethod(app, MUIM_Application_PushMethod,
						qn->obj, 2 | MUIV_PushMethod_Delay(300) | MUIF_PUSHMETHOD_SINGLE,
						MM_QuickLinkGroup_Update, current);
				}

				DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);
			}
			break;
		case MA_Bookmarkgroup_InMenu:
			active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
			if(active)
			{
				current=(struct treedata *)active->tn_User;   
				D(kprintf( "InMenu before is %08lx.\n",(ULONG)current->flags));
				if (tag->ti_Data) current->flags = (current->flags & ~NODEFLAG_INMENU) | NODEFLAG_INMENU;
				else current->flags = current->flags & ~NODEFLAG_INMENU;
				D(kprintf( "InMenu after is %08lx.\n",(ULONG)current->flags));
				line=DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, active, 0L);
				DoMethod(data->lt_bookmark, MUIM_List_Redraw, line, NULL);
				DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);

                DoMethod(obj, MM_Bookmarkgroup_UpdateMenu);
			}
			break;
		case MA_Bookmarkgroup_QuickLink:
			current=NULL;
			active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
			if(active)
			{
				current=(struct treedata *)active->tn_User;
				current->treenode = active;
				D(kprintf( "QLink before is %08lx.\n",(ULONG)current->flags));
				if (tag->ti_Data!=(current->flags & NODEFLAG_QUICKLINK))
				{
					if (tag->ti_Data!=0)
					{
						DoMethod(obj, MM_Bookmarkgroup_AddQuickLink, current, tag->ti_Data);
					}
					else
					{
						DoMethod(obj, MM_Bookmarkgroup_RemoveQuickLink, current);
					}
				}
				D(kprintf( "QLink after is %08lx.\n",(ULONG)current->flags));
			}
			break;
	}
	NEXTTAG
}

DEFNEW
{
	struct Data tmp;

	obj = (Object *) DoSuperNew(cl, obj,
		Child, HGroup,
			Child, VGroup,
				Child, tmp.findgroup = (Object *) NewObject(getfindtextclass(), NULL,
												MUIA_ShowMe, TRUE,
												MA_FindText_Closable, FALSE,
												MA_FindText_ShowButtons, FALSE,
												MA_FindText_ShowCaseSensitive, FALSE,
												MA_FindText_ShowText, FALSE,
												TAG_DONE),
				Child, tmp.lv_bookmark = ListviewObject,
					MUIA_CycleChain, TRUE,
					MUIA_Listview_DragType, 2,
					MUIA_Listview_List, tmp.lt_bookmark = (Object *) NewObject(getbookmarklisttreeclass(), NULL,
						End,
					End,
				Child, HGroup,
					Child, tmp.bt_addgroup  = SimpleButton(GSI(MSG_BOOKMARKGROUP_ADD_GROUP)),
					Child, tmp.bt_addlink   = SimpleButton(GSI(MSG_BOOKMARKGROUP_ADD_LINK)),
					Child, tmp.bt_separator = SimpleButton(GSI(MSG_BOOKMARKGROUP_ADD_SEPARATOR)),
					Child, tmp.bt_remove    = SimpleButton(GSI(MSG_BOOKMARKGROUP_REMOVE)),
					End,
				Child, MUI_NewObject(MUIC_Rectangle,
					MUIA_Rectangle_HBar, TRUE,
					MUIA_FixHeight,      6,
					End,
				Child, HGroup,
					Child, RowGroup(2),
						Child, Label(GSI(MSG_BOOKMARKGROUP_TITLE)),
						Child, tmp.st_title=StringObject,
							StringFrame,
							MUIA_String_MaxLen,      128,
							MUIA_String_Contents,    "",
							MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain,         TRUE,
							End,
						Child, Label(GSI(MSG_BOOKMARKGROUP_ADDRESS)),
						Child, tmp.st_address=StringObject,
							StringFrame,
							MUIA_String_MaxLen,      4096,
							MUIA_String_Contents,    "",
							MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain,         TRUE,
							End,
						End,
					Child, MUI_NewObject(MUIC_Rectangle,
						MUIA_Rectangle_VBar, TRUE,
						MUIA_FixWidth, 6,
						End,
					Child, RowGroup(5),
						Child, VSpace(0), 
						Child, VSpace(0), 
						Child, tmp.ch_showmenu=CheckMark(FALSE),  
						Child, LLabel(GSI(MSG_BOOKMARKGROUP_SHOW_IN_MENU)),
						Child, VSpace(0),
						Child, VSpace(0),
						Child, tmp.ch_quicklink=CheckMark(FALSE),
						Child, LLabel(GSI(MSG_BOOKMARKGROUP_QUICKLINK)),
						Child, VSpace(0), 
						Child, VSpace(0), 
						End,
					End,
				End,
			Child, BalanceObject,
				MUIA_ObjectID, MAKE_ID('B','M','B','A'),
				End,
			Child, VGroup,
				MUIA_HorizWeight, 30,
				Child, tmp.lt_linklist = (Object *) NewObject(getlinklistclass(), NULL,
					End,
				Child, tmp.st_alias=StringObject,
							StringFrame,
							MUIA_String_MaxLen,      32,
							MUIA_String_Contents,    "",
							MUIA_String_AdvanceOnCR, TRUE,
							MUIA_CycleChain,         TRUE,
							End,
				Child, tmp.bt_removeql=SimpleButton(GSI(MSG_BOOKMARKGROUP_REMOVE)),
				End,
			End,
		TAG_MORE, INITTAGS
		);

	if (!obj)
	{
		return (NULL);
	}

	GETDATA;

	data->save_in_progress = FALSE;
	data->lv_bookmark   = tmp.lv_bookmark;
	data->lt_bookmark   = tmp.lt_bookmark;
	data->bt_addgroup   = tmp.bt_addgroup;
	data->bt_addlink    = tmp.bt_addlink;
	data->bt_remove     = tmp.bt_remove;
	data->st_title      = tmp.st_title;
	data->st_address    = tmp.st_address;
	data->st_alias      = tmp.st_alias;
	data->ch_showmenu   = tmp.ch_showmenu;
	data->bt_separator  = tmp.bt_separator;
	data->ch_quicklink  = tmp.ch_quicklink;
	data->lt_linklist   = tmp.lt_linklist;
	data->bt_removeql   = tmp.bt_removeql;
	data->win           = NULL;
	data->findgroup     = tmp.findgroup;
	data->nofocuschange = FALSE;

	NEWLIST(&data->qlgroups);

	set(data->bt_addgroup , MUIA_CycleChain, TRUE);
	set(data->bt_addlink  , MUIA_CycleChain, TRUE);
	set(data->bt_remove   , MUIA_CycleChain, TRUE);
	set(data->ch_showmenu , MUIA_CycleChain, TRUE);
	set(data->bt_separator, MUIA_CycleChain, TRUE);
	set(data->ch_quicklink, MUIA_CycleChain, TRUE);
	set(data->bt_removeql , MUIA_CycleChain, TRUE);

	set(data->lt_bookmark, MA_Bookmarkgroup_BookmarkObject, obj);
	
	set(data->lt_linklist, MA_Bookmarkgroup_BookmarkObject, obj);
	set(data->lt_linklist, MA_Bookmarkgroup_BookmarkList, data->lt_bookmark);
	
	set(data->findgroup  , MA_FindText_Target, obj);

	DoMethod(obj, MM_Bookmarkgroup_Update, MV_BookmarkGroup_Update_All); // Init fiels correctly

	DoMethod(data->bt_addgroup, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2,
		MM_Bookmarkgroup_AddGroup, ""
	);
	DoMethod(data->bt_addlink, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 7,
		MM_Bookmarkgroup_AddLink, "", "", "http://", FALSE, TRUE, FALSE
	);
	DoMethod(data->bt_separator, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1,
		MM_Bookmarkgroup_AddSeparator
	);
	DoMethod(data->bt_remove, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1,
		MM_Bookmarkgroup_Remove
	);
	DoMethod(data->bt_removeql, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2,
		MM_Bookmarkgroup_RemoveQuickLink, NULL
	);
	
	DoMethod(data->lt_bookmark, MUIM_Notify, MUIA_Listtree_DoubleClick, MUIV_EveryTime,
		obj, 1,
		MM_Bookmarkgroup_Open
	);
	DoMethod(data->lt_bookmark, MUIM_Notify, MUIA_Listtree_Quiet, TRUE,
		obj, 1,
		MM_Bookmarkgroup_KillNotification
	);
	DoMethod(data->lt_bookmark, MUIM_Notify, MUIA_Listtree_Quiet, FALSE,
		obj, 1,
		MM_Bookmarkgroup_BuildNotification
	);
 
	DoMethod(data->ch_showmenu, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
		obj, 3,
		MUIM_Set, MA_Bookmarkgroup_InMenu, MUIV_TriggerValue
	);

	DoMethod(data->ch_quicklink, MUIM_Notify, MUIA_Selected, TRUE,
		obj, 3,
		MUIM_Set, MA_Bookmarkgroup_QuickLink, -1
	);

	DoMethod(data->ch_quicklink, MUIM_Notify, MUIA_Selected, FALSE,
		obj, 3,
		MUIM_Set, MA_Bookmarkgroup_QuickLink, 0
	);
 

	DoMethod(obj, MM_Bookmarkgroup_BuildNotification);

	//data->notifyreq=dosnotify_start(BOOKMARK_PATH) ;

	return ((ULONG)obj);
}

DEFDISP
{
	//GETDATA;
	D(kprintf("OWB Bookmark test app: disposing\n"));
	//dosnotify_stop(data->notifyreq);
	DoMethod(obj, MM_Bookmarkgroup_SaveHtml, BOOKMARK_PATH, MV_Bookmark_SaveHtml_OWB);
	D(kprintf("OWB Bookmark test app: ok, calling supermethod\n"));
	return DOSUPER;
}

DEFSET
{
	GETDATA;
	doset(obj, data, msg->ops_AttrList);
	return DOSUPER;
}

/*
DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Bookmarkgroup_Reorder:
		{
			*msg->opg_Storage = TRUE;
			return (TRUE);
		}
	}
	return (DOSUPER);
}
*/

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
		/*DoMethod(data->lt_bookmark, MUIM_Notify, MUIA_Listtree_Active, MUIV_EveryTime,
			_win(obj), 3,
			MUIM_Set, MUIA_Window_ActiveObject, data->st_title);
		
		DoMethod(data->lt_linklist, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
			_win(obj), 3,
			MUIM_Set, MUIA_Window_ActiveObject, data->st_alias);*/
        MUI_RequestIDCMP(obj,IDCMP_RAWKEY);
		if(getv(app, MA_OWBApp_ShowFavIcons) & MV_OWBApp_ShowFavIcons_Bookmark)
			handle_favicons(cl, obj, MUIV_Listtree_GetEntry_ListNode_Root, true);
	}
	return rc;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;
	/*if (_win(obj))
	{
		DoMethod(data->lt_bookmark, MUIM_KillNotifyObj, MUIA_Listtree_Active, _win(obj));
		DoMethod(data->lt_linklist, MUIM_KillNotifyObj, MUIA_List_Active, _win(obj));
	}*/
    MUI_RejectIDCMP(obj,IDCMP_RAWKEY);
	handle_favicons(cl, obj, MUIV_Listtree_GetEntry_ListNode_Root, false);
	data->win=NULL;
	return DOSUPER;
}

DEFSMETHOD(Bookmarkgroup_Update)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *active = NULL;
	struct treedata *current;
	D(kprintf("Bookmarkgroup_Update %ld\n", msg->from));
	if (msg->from & MV_BookmarkGroup_Update_Tree)
	{
		active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
		if(active)
		{
			current=(struct treedata *)active->tn_User;

			if (current->title) nnset(data->st_title, MUIA_String_Contents, current->title);
			else nnset(data->st_title, MUIA_String_Contents, "");

			if(current->flags & NODEFLAG_BAR)
			{
				// Bar separator
				set(data->bt_remove,    MUIA_Disabled, FALSE);
				set(data->st_title,     MUIA_Disabled, TRUE );
				set(data->st_address,   MUIA_Disabled, TRUE );
				set(data->ch_showmenu,  MUIA_Disabled, FALSE);
				set(data->ch_quicklink, MUIA_Disabled, TRUE);

				nnset(data->st_address, MUIA_String_Contents, "");
				nnset(data->st_title, 	MUIA_String_Contents, "");
				if (current->flags & NODEFLAG_INMENU) nnset(data->ch_showmenu, MUIA_Selected, TRUE);
				else nnset(data->ch_showmenu, MUIA_Selected, FALSE);
			}
			else if(current->flags & NODEFLAG_GROUP)
			{
				// Group is active
				set(data->bt_remove,    MUIA_Disabled, FALSE);
				set(data->st_title,     MUIA_Disabled, FALSE);
				set(data->st_address,   MUIA_Disabled, TRUE );
				set(data->ch_showmenu,  MUIA_Disabled, FALSE);
				set(data->ch_quicklink, MUIA_Disabled, FALSE);

				if (current->title) nnset(data->st_title, MUIA_String_Contents, current->title);
				else nnset(data->st_title, MUIA_String_Contents, "");
				nnset(data->st_address, MUIA_String_Contents, "");

                if (current->flags & NODEFLAG_QUICKLINK) nnset(data->ch_quicklink, MUIA_Selected, TRUE);
                else nnset(data->ch_quicklink, MUIA_Selected, FALSE);
				if (current->flags & NODEFLAG_INMENU) nnset(data->ch_showmenu, MUIA_Selected, TRUE);
				else nnset(data->ch_showmenu, MUIA_Selected, FALSE);
				if ((data->win) && !(msg->from & MV_BookmarkGroup_Update_QuickLink) && !data->nofocuschange) DoMethod(data->win, MUIM_Set, MUIA_Window_ActiveObject, data->st_title);
			}
			else
			{
				// Link is active
				DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);
				set(data->bt_remove,    MUIA_Disabled, FALSE);
				set(data->st_title,     MUIA_Disabled, FALSE);
				set(data->st_address,   MUIA_Disabled, FALSE);
				set(data->ch_showmenu,  MUIA_Disabled, FALSE);
				set(data->ch_quicklink, MUIA_Disabled, FALSE);

				if (current->title) nnset(data->st_title, MUIA_String_Contents, current->title);
				else nnset(data->st_title, MUIA_String_Contents, "");
				
				if (current->address) nnset(data->st_address, MUIA_String_Contents, current->address);
				else nnset(data->st_address, MUIA_String_Contents, "");

				if (current->flags & NODEFLAG_INMENU) nnset(data->ch_showmenu, MUIA_Selected, TRUE);
				else nnset(data->ch_showmenu, MUIA_Selected, FALSE);
				
				if (current->flags & NODEFLAG_QUICKLINK) nnset(data->ch_quicklink, MUIA_Selected, TRUE);
				else nnset(data->ch_quicklink, MUIA_Selected, FALSE);
				if ((data->win) && !(msg->from & MV_BookmarkGroup_Update_QuickLink) && !data->nofocuschange) DoMethod(data->win, MUIM_Set, MUIA_Window_ActiveObject, data->st_title);
			}
		}
		else
		{
			// No entry selected
			set(data->bt_remove,    MUIA_Disabled, TRUE);
			set(data->st_title,     MUIA_Disabled, TRUE);
			set(data->st_address,   MUIA_Disabled, TRUE);
			set(data->ch_showmenu,  MUIA_Disabled, TRUE);
			set(data->ch_quicklink, MUIA_Disabled, TRUE); 	
			nnset(data->st_address, MUIA_String_Contents, "");
			nnset(data->st_title, 	MUIA_String_Contents, "");
		}
	}
	// Quick list update
	if (msg->from & MV_BookmarkGroup_Update_QuickLink)
	{
		current=NULL;
		if (DoMethod(data->lt_linklist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &current) && (current))
		{
			// Active entry
			set(data->st_alias,     MUIA_Disabled, FALSE); 	
			set(data->bt_removeql,  MUIA_Disabled, FALSE); 	
			if (current->alias)
			{
				D(kprintf("Alias is '%s'\n", current->alias));
				nnset(data->st_alias, MUIA_String_Contents, current->alias);
			}
			else
			{
				D(kprintf("Alias is null\n"));
				nnset(data->st_alias, MUIA_String_Contents, "");
			}
			if ((data->win) && !(msg->from & MV_BookmarkGroup_Update_Tree)) DoMethod(data->win,	MUIM_Set, MUIA_Window_ActiveObject, data->st_alias);
		}
		else
		{
			set(data->st_alias,     MUIA_Disabled, TRUE); 	  
			set(data->bt_removeql,  MUIA_Disabled, TRUE);
			nnset(data->st_alias,   MUIA_String_Contents, ""); 	
		}
	}
	return (0);
}

DEFSMETHOD(Bookmarkgroup_AddGroup)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *entry = NULL, *active = NULL, *parent = NULL;
	struct treedata newnode;
	struct treedata *node = NULL;
	ULONG line;

	newnode.flags = NODEFLAG_GROUP;
	if(getv(app, MA_OWBApp_Bookmark_AddToMenu)) newnode.flags |= NODEFLAG_INMENU;
	newnode.alias   = "";
	newnode.address = "";
	newnode.icon    = NULL;
	newnode.iconimg = NULL;
	newnode.tree    = data->lt_bookmark;

	if (!msg->title) newnode.title="";
	else newnode.title=(char *) msg->title;

	D(kprintf("AddGroup title %s\n",msg->title));

	active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);

	// Disable notifications
	set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE);

	if(active)
	{
		node = (struct treedata *)active->tn_User;

		if (node->flags & NODEFLAG_GROUP)
		{
			// Add link in current group
			entry = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
				msg->title,
				&newnode,
				active,
				MUIV_Listtree_Insert_PrevNode_Tail,
                TNF_LIST
			);

		}
		else
		{
			parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);
			entry = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
				msg->title,
				&newnode,
				parent ? parent : MUIV_Listtree_Insert_ListNode_Root,
				active,
                TNF_LIST
				);
		}
	}
	else
	{
		// Add link at root
		entry = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
			msg->title,
			&newnode,
			MUIV_Listtree_Insert_ListNode_Root,
			MUIV_Listtree_Insert_PrevNode_Tail,
            TNF_LIST
		);
	}

	D(kprintf("entry is %08lx.\n",(ULONG)entry));
	
	set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);
	
	if (entry)
	{
		// make line active for edit
		line = DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, entry, 0L);
		set(data->lt_bookmark, MUIA_List_Active, line);
        DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);
	}
	return (0);
}

DEFSMETHOD(Bookmarkgroup_AddLink)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *active = NULL, *entry = NULL, *parent = NULL;
	struct treedata newnode, *node;
	ULONG line;
	String title = "";
	String alias = "";
	String address = "";
	char *ctitle, *calias, *caddress;

	if(msg->title)   title = String(msg->title).replace("\"", "");
	if(msg->alias)   alias =  String(msg->alias).replace("\"", "");
	if(msg->address) address = String(msg->address).replace("\"", ""); //encodeWithURLEscapeSequences(msg->address);

	newnode.flags    = NODEFLAG_LINK;
	newnode.title    = (char*) strdup(title.latin1().data());
	newnode.alias    = (char*) strdup(alias.latin1().data());
	newnode.address  = (char*) strdup(address.latin1().data());
	newnode.ql_order = 0;
	newnode.icon     = NULL;
	newnode.iconimg  = NULL;
	newnode.tree     = data->lt_bookmark;

	ctitle   = newnode.title;
	caddress = newnode.address;
	calias   = newnode.alias;
	
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
		node=(struct treedata *)active->tn_User;
		
		if (node->flags & NODEFLAG_GROUP)
		{
			// Add link in current group
			entry= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
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
			entry= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
				msg->title,
				&newnode,
				parent ? parent : MUIV_Listtree_Insert_ListNode_Root,
				active,
				0
				);
		}
	}
	else
	{
		// Add link at root
		entry= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
			msg->title,
			&newnode,
			MUIV_Listtree_Insert_ListNode_Root,
			MUIV_Listtree_Insert_PrevNode_Tail,
			0
		);
	}

	free(ctitle);
	free(calias);
	free(caddress);

	D(kprintf( "entry is %08lx.\n",(ULONG)entry));
	set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);	
	if (entry)
	{
		// make line active for edit
		line = DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, entry, 0L);
		set(data->lt_bookmark, MUIA_List_Active, line);
		set(data->ch_showmenu, MUIA_Selected, msg->menu);
		if(msg->quicklink)
			set(data->ch_quicklink, MUIA_Selected, TRUE);

		DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);

		return ((ULONG)entry->tn_User);
	}
	return (0);
}

DEFTMETHOD(Bookmarkgroup_AddSeparator)
{
	struct MUIS_Listtree_TreeNode *active,*entry=NULL,*parent;
	struct treedata newnode,*node;
	LONG line = -1;
	GETDATA;

	newnode.flags=NODEFLAG_BAR;
	newnode.title=NULL;
	newnode.alias=NULL;
	newnode.address=NULL;
	newnode.ql_order=0;
	newnode.icon    = NULL;
	newnode.iconimg = NULL;
	newnode.tree    = data->lt_bookmark;
	
	D(kprintf("AddSeparator\n"));
	
	active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
	
	D(kprintf("Active is %08lx.\n",(ULONG)active));
	
	// Disable notifications
	set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE); 	 
	
	if(active)
	{
		node=(struct treedata *)active->tn_User;
		
		if (node->flags & NODEFLAG_GROUP)
		{
			// Add link in current group
			entry= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
				"",
				&newnode,
				active,
				MUIV_Listtree_Insert_PrevNode_Tail,
				0
			);
   
		}
		else
		{
			parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);
			entry= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
				"",
				&newnode,
				parent ? parent : MUIV_Listtree_Insert_ListNode_Root,
				active,
				0
				);
		}
	}
	else
	{
		// Add separator at root
		entry = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
			"",
			&newnode,
			MUIV_Listtree_Insert_ListNode_Root,
			MUIV_Listtree_Insert_PrevNode_Tail,
			0
		);
	}

	D(kprintf( "entry is %08lx.\n",(ULONG)entry));
	set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);	
	if (entry)
	{
		// make line active for edit
		line = DoMethod(data->lt_bookmark, MUIM_Listtree_GetNr, entry, 0L);
		set(data->lt_bookmark, MUIA_List_Active, line);
		DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);
	}
	return (0);
}

DEFTMETHOD(Bookmarkgroup_Remove)
{
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

		td=(struct treedata *)tn->tn_User;
		if ((td) && (td->flags & NODEFLAG_QUICKLINK))
		{
			// Remove quick link
			DoMethod(obj, MM_Bookmarkgroup_RemoveQuickLink, td);
		}
	}

	set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE); 	
	DoMethod(data->lt_bookmark, MUIM_Listtree_Remove, NULL, MUIV_Listtree_Remove_TreeNode_Active, 0);
	set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);
	DoMethod(obj, MM_Bookmarkgroup_Update, MV_BookmarkGroup_Update_All);
    DoMethod(app, MUIM_Application_PushMethod,
					obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
					MM_Bookmarkgroup_AutoSave);
	return (0);
}

DEFTMETHOD(Bookmarkgroup_ReadQLOrder)
{
	// Read the current sort order of quick link
	GETDATA;
	struct treedata *td;
	ULONG count,max=0,change=FALSE;
	D(kprintf("Bookmarkgroup ReadQLOrder\n"));
	get(data->lt_linklist, MUIA_List_Entries, &max);
	
	for (count=0;count<max;count++)
	{
		if (DoMethod(data->lt_linklist, MUIM_List_GetEntry, count, &td))
		{
			if (td->ql_order!=count+1) change=true;
			td->ql_order=count+1;
		}
	}
	if (change)
	{
		APTR n;

		ITERATELIST(n, &data->qlgroups)
		{
			struct qlgroupnode *qn = (struct qlgroupnode *) n;

			if(qn->obj)
			{
				DoMethod(qn->obj, MM_QuickLinkGroup_InitChange, MV_QuickLinkGroup_Change_Redraw);
				DoMethod(qn->obj, MM_QuickLinkGroup_ExitChange, MV_QuickLinkGroup_Change_Redraw);
			}
		}
	}
	else
	{
		D(kprintf("ReadQLOrder no change\n"));
	}
	return (0);
}

DEFSMETHOD(Bookmarkgroup_LoadHtml)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *newentry = NULL, *group = NULL;
	struct treedata newnode;
	char   line[8192];
	STRPTR ptr[8]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	BPTR  html;
	ULONG found,type=0,order,showalias=TRUE;
	LONG  p,q;
	html=Open((char *) msg->file, MODE_OLDFILE);
	if (html)
	{
		FGets(html, line, sizeof(line)-1); // Version string
		if (strstr(line, "<!-- OWB MorphOS Bookmark V1 -->"))
		{
			type=MV_Bookmark_SaveHtml_OWB;
			//printf("Load bookmark in OWB format for file %s\n", msg->file);
		}
		if (strstr(line, "<!-- IBrowse Hotlist V2 -->"))
		{
			type=MV_Bookmark_SaveHtml_IBrowse;
			//printf("Load bookmark in IBrowse format for file %s\n", msg->file);
		}

		if (type==0)
		{
			Close(html);
			printf("Unknow type of bookmark. Only IBrowse 2.4 and OWB MorphOS supported.\n");
			return (MV_Bookmark_ImportHtml_WrongType);
		}

		while ( FGets(html, line, sizeof(line)-1) )
		{
			found=FALSE;
			p=parsevarpool(line, "%.<LI><A HREF=\"%s\"%s>%s</A>", &ptr[0],&ptr[1],&ptr[2]);
			p--;
			if (p==3)
			{
				q=0;
				D(kprintf( "Parse Link: '%s'\n", ptr[2]));

				if (strcmp(ptr[2],"----------------------------------------")==0)
				{
					newnode.flags=NODEFLAG_BAR;
					newnode.address=NULL;
					newnode.title="";
					newnode.alias=NULL;
					newnode.ql_order=0;
					newnode.showalias=TRUE;
					newnode.icon    = NULL;
					newnode.iconimg = NULL;
					newnode.tree    = data->lt_bookmark;
				}
				else
				{
					newnode.flags=NODEFLAG_LINK;
					newnode.address=ptr[0];
					newnode.title=ptr[2];
					newnode.ql_order=0;  
					newnode.showalias=TRUE;
					newnode.icon    = NULL;
					newnode.iconimg = NULL;
					newnode.tree    = data->lt_bookmark;
				}
				
				newnode.alias=NULL;
				
				q=parsevarpool(ptr[1], "%.IBSHORTCUT=\"%s\"", &ptr[3]);
				if (q==2)
				{
					// Alias
					D(kprintf( "    Alias: '%s'\n", ptr[3]));
					newnode.alias=ptr[3];
					p++;
				}
				else //if (type==MV_Bookmark_SaveHtml_OWB)
				{
					// Try OWB Quick Link
					q=parsevarpool(ptr[1], "%.OWBQUICKLINK=\"%s\"", &ptr[3]);
					if (q==2)
					{
						D(kprintf( "QuickLink: '%s'\n", ptr[3]));
						newnode.alias=ptr[3];
						p++;
						newnode.flags|=NODEFLAG_QUICKLINK;

						q=parsevarpool(ptr[1], "%.OWBQLORDER=\"%d\"", &order);
						if (q==2)
						{
							newnode.ql_order=order;
							D(kprintf("Order: %ld\n",order));
						}
						else
						{
							D(kprintf("Fail get order '%s'\n", ptr[1]));
						}

						q=parsevarpool(ptr[1], "%.OWBQLSHOWALIAS=\"%d\"", &showalias);
						if (q==2)
						{
							newnode.showalias=showalias;
							D(kprintf("Show Alias: %ld\n",showalias));
						}
						else
						{
							D(kprintf("Fail get show alias '%s'\n", ptr[1]));
						}
					}
					else
					{
						newnode.alias=NULL;
					}
				}

				q=parsevarpool(ptr[1], "%.IBHLFLAGS=\"%s\"", &ptr[4]);
				if (q==2)
				{
					D(kprintf( "Flags: '%s'\n", ptr[4]));
					FreeVecTaskPooled(ptr[4]); ptr[4]=NULL;
				}
				else
				{
					newnode.flags |= NODEFLAG_INMENU;
				}

				// Retain it for icondatabase
				if(newnode.address)
				{
					iconDatabase().retainIconForPageURL(String(newnode.address));
				}

				// Add entry
				newentry= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
					newnode.title,
					&newnode,
					group ? group : MUIV_Listtree_Insert_ListNode_Root,
					MUIV_Listtree_Insert_PrevNode_Tail,
					0
					);

				found=TRUE;
				if ( (newentry) && (newnode.flags & NODEFLAG_QUICKLINK) )
				{
					// It is a quick link add to second list
					DoMethod(data->lt_linklist, MUIM_List_InsertSingle,
						newentry->tn_User,
						MUIV_List_Insert_Sorted
						);
				}
			}
			if (p>0) { for(p--;p>=0;p--) { FreeVecTaskPooled(ptr[p]); ptr[p]=NULL; } p=0; }  
			if (!found)
			{
				if (p>0) { for(p--;p>=0;p--) { FreeVecTaskPooled(ptr[p]); ptr[p]=NULL; } p=0; }
				p=parsevarpool(line, "%.<LI%s><B>%s</B><UL>", &ptr[0],&ptr[1]);
				p--;
				if (p==2)
				{
					D(kprintf( "Parse Group: '%s'\n", ptr[1]));
					newnode.flags=NODEFLAG_GROUP;
					newnode.title=ptr[1];
					newnode.alias=NULL;
					newnode.address=NULL;
					newnode.ql_order=0;  
					newnode.showalias=TRUE;
					newnode.icon    = NULL;
					newnode.iconimg = NULL;
					newnode.tree    = data->lt_bookmark;

					q=parsevarpool(ptr[0], "%.IBHLFLAGS=\"%s\"", &ptr[4]);
					if (q==2)
					{
						D(kprintf( "Flags: '%s'\n", ptr[4]));
						FreeVecTaskPooled(ptr[4]); ptr[4]=NULL;
					}
					else
					{
						newnode.flags |= NODEFLAG_INMENU;
					}

					// Try OWB Quick Link
					q=parsevarpool(ptr[0], "%.OWBQUICKLINK=\"%s\"", &ptr[4]);
					if (q==2)
					{
						D(kprintf( "QuickLink: '%s'\n", ptr[4]));
						newnode.alias=ptr[4];
						p++;
						newnode.flags|=NODEFLAG_QUICKLINK;

						q=parsevarpool(ptr[0], "%.OWBQLORDER=\"%d\"", &order);
						if (q==2)
						{
							newnode.ql_order=order;
							D(kprintf("Order: %ld\n",order));
						}
						else
						{
							D(kprintf("Fail get order '%s'\n", ptr[0]));
						}

						q=parsevarpool(ptr[1], "%.OWBQLSHOWALIAS=\"%d\"", &showalias);
						if (q==2)
						{
							newnode.showalias=showalias;
							D(kprintf("Show Alias: %ld\n",showalias));
						}
						else
						{
							D(kprintf("Fail get show alias '%s'\n", ptr[1]));
						}
					}
					else
					{
						newnode.alias=NULL;
					}

					// Add group
					group= (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_Insert,
						newnode.title,
						&newnode,
						group ? group : MUIV_Listtree_Insert_ListNode_Root,
						MUIV_Listtree_Insert_PrevNode_Tail,
						TNF_LIST
						);

					found=TRUE;

					if ( (group) && (newnode.flags & NODEFLAG_QUICKLINK) )
					{
						// It is a quick link add to second list
						DoMethod(data->lt_linklist, MUIM_List_InsertSingle,
							group->tn_User,
							MUIV_List_Insert_Sorted
							);
					}
				}
			}
			if (p>0) { for(p--;p>=0;p--) { FreeVecTaskPooled(ptr[p]); ptr[p]=NULL; } p=0; }  
			if (!found)
			{
				if (strstr(line, "</UL>"))
				{
					D(kprintf( "Parse End Group\n"));
					if(group != NULL)
					{
						group = (MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, group, MUIV_Listtree_GetEntry_Position_Parent, 0);
					}
					found=TRUE;
				}
			}
			if (p>0) { for(p--;p>=0;p--) { FreeVecTaskPooled(ptr[p]); ptr[p]=NULL; } p=0; }
		}
		Close(html);
		return (MV_Bookmark_ImportHtml_OK);
	}
	return (MV_Bookmark_ImportHtml_NoFile);
}

static void save_list(BPTR html, Object *lt, struct MUIS_Listtree_TreeNode *list, ULONG type)
{
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *node;   
	ULONG  pos=0;
	char  line[8192],link[128];
	
	for(pos=0; ; pos++)
	{
		if( (tn=(struct MUIS_Listtree_TreeNode *)DoMethod(lt, MUIM_Listtree_GetEntry, list, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
		{
			node=(struct treedata *)tn->tn_User;
			if (node)
			{
				STRPTR title,quicklink,alias,flags,address;

				String wtitle = "";
				String walias = "";
				String waddress = "";

				if(node->title)   title = node->title; else title = "";
				if(node->alias)   alias = node->alias; else alias = "";
				if(node->address) address = node->address; else address = "";

				// A couple safety checks
				if(node->title)
				{
					wtitle = String(title);
					wtitle.replace("\"", "");
					wtitle.replace("<", "");
					wtitle.replace(">", "");
				}
				
				if(node->alias)
				{
					walias = String(alias);
					walias.replace("\"", "");
					walias.replace("<", "");
					walias.replace(">", "");
				}

				if(node->address)
				{
					waddress = String(address);
					waddress.replace("\"", "");
					waddress.replace("<", "");
					waddress.replace(">", "");
				}

				title   = strdup(wtitle.latin1().data());
				alias   = strdup(walias.latin1().data());
				address	= strdup(waddress.latin1().data());

				if(tn->tn_Flags & TNF_LIST)
				{
					if (node->flags & NODEFLAG_INMENU) flags="";
					else flags=" IBHLFLAGS=\"NOMENU\"";

					if ( (node->alias) && (node->flags & NODEFLAG_QUICKLINK) && type==MV_Bookmark_SaveHtml_OWB )
					{
						snprintf(link, sizeof(link), " OWBQUICKLINK=\"%s\" OWBQLORDER=\"%ld\" OWBQLSHOWALIAS=\"%ld\"%s", alias, (unsigned long)node->ql_order, (unsigned long)node->showalias, flags );
						flags=link;
					}

					// <LI IBHLFLAGS="NOMENU"><B>Forums Voitures</B><UL>
					snprintf(line, sizeof(line), "<LI%s><B>%s</B><UL>\n", flags, title);
					FPuts(html, line);
					save_list(html, lt, tn, type);
					FPuts(html, "</UL>\n");
				}
				else
				{
					if (node->flags & NODEFLAG_BAR)
					{
						// <LI><A HREF="" IBHLFLAGS="NOMENU">----------------------------------------</A>
						if (node->flags & NODEFLAG_INMENU) FPuts(html, "<LI><A HREF=\"\">----------------------------------------</A>\n");
						else FPuts(html, "<LI><A HREF=\"\" IBHLFLAGS=\"NOMENU\">----------------------------------------</A>\n");
					}
					else
					{
						if ( (node->alias) && (node->flags & NODEFLAG_QUICKLINK) && type==MV_Bookmark_SaveHtml_OWB )
						{
							snprintf(link, sizeof(link), " OWBQUICKLINK=\"%s\" OWBQLORDER=\"%ld\" OWBQLSHOWALIAS=\"%ld\"", alias, (unsigned long)node->ql_order, (unsigned long)node->showalias );
							quicklink=link;
						}
						else if (node->alias)
						{
							snprintf(link, sizeof(link), " IBSHORTCUT=\"%s\"", alias );
							quicklink=link;
						}
						else quicklink="";

						if (node->flags & NODEFLAG_INMENU) flags="";
						else flags=" IBHLFLAGS=\"NOMENU\"";

						// <LI><A HREF="http://publications.gbdirect.co.uk/c_book/" IBSHORTCUT="CBook" IBHLFLAGS="NOMENU">The C Book</A>
						snprintf(line, sizeof(line), "<LI><A HREF=\"%s\"%s%s>%s</A>\n", address, quicklink, flags, title);
						FPuts(html, line); 	
					}
				}

				free(title);
				free(alias);
				free(address);
			}
		}
		else
			break;
	}
}

DEFSMETHOD(Bookmarkgroup_SaveHtml)
{
	GETDATA;
	BPTR  html;
	DoMethod(obj, MM_Bookmarkgroup_ReadQLOrder);
	html=Open((char *) msg->file, MODE_NEWFILE);
	if (html)
	{
		if (msg->type==MV_Bookmark_SaveHtml_OWB)
		{
			D(kprintf("Save OWB bookmark in %s\n", msg->file));
			FPuts(html,"<!-- OWB MorphOS Bookmark V1 -->\n");
			FPuts(html,"<HTML>\n");
			FPuts(html,"<HEAD><TITLE>OWB MorphOS Bookmark</TITLE></HEAD>\n");
			FPuts(html,"<BODY>\n");
			FPuts(html,"<H2><P ALIGN=CENTER>OWB Bookmark</P></H2>\n");
			FPuts(html,"<HR>\n");
			FPuts(html,"<UL>\n");
			save_list(html, data->lt_bookmark, MUIV_Listtree_GetEntry_ListNode_Root, msg->type);
			FPuts(html,"</UL>\n");
			FPuts(html,"<HR>\n");
			FPuts(html,"<P align=center><font size=-1>This hotlist has been created using OWB 1.23.</font></P></BODY>\n");
			FPuts(html,"</HTML>\n");
			Close(html);
		}
		else if (msg->type==MV_Bookmark_SaveHtml_IBrowse)
		{
			printf("Save IBrowse bookmark in %s\n", msg->file);
			FPuts(html,"<!-- IBrowse Hotlist V2 -->\n");
			FPuts(html,"<HTML>\n");
			FPuts(html,"<HEAD><TITLE>IBrowse Hotlist</TITLE></HEAD>\n");
			FPuts(html,"<BODY>\n");
			FPuts(html,"<H2><P ALIGN=CENTER>IBrowse Hotlist</P></H2>\n");
			FPuts(html,"<HR>\n");
			FPuts(html,"<UL>\n");
			save_list(html, data->lt_bookmark, MUIV_Listtree_GetEntry_ListNode_Root, msg->type);
			FPuts(html,"</UL>\n");
			FPuts(html,"<HR>\n");
			FPuts(html,"<P align=center><font size=-1>This hotlist has been created using <A HREF=\"http://www.ibrowse-dev.net/\">IBrowse 2.4beta</A>.</font></P></BODY>\n");
			FPuts(html,"</HTML>\n");
			Close(html);
		}
	}

	return 0;
}

DEFTMETHOD(Bookmarkgroup_AutoSave)
{
	GETDATA;
	if (data->save_in_progress==FALSE)
	{
		data->save_in_progress=TRUE;
		//dosnotify_stop(data->notifyreq);
		//data->notifyreq=NULL;
		DoMethod(obj, MM_Bookmarkgroup_SaveHtml, BOOKMARK_PATH, MV_Bookmark_SaveHtml_OWB);
		//data->notifyreq=dosnotify_start(BOOKMARK_PATH) ;
		data->save_in_progress=FALSE;
		DoMethod(obj, MM_Bookmarkgroup_UpdateMenu);
		
		set(app, MA_OWBApp_BookmarksChanged, TRUE);
	}
	else
	{
		D(kprintf("Dual call to auto save\n"));
	}
	return (0);
}

DEFTMETHOD(Bookmarkgroup_BuildNotification)
{
	GETDATA;
	
	DoMethod(data->lt_bookmark, MUIM_Notify, MUIA_Listtree_Active, MUIV_EveryTime,
		obj, 2,
		MM_Bookmarkgroup_Update, MV_BookmarkGroup_Update_Tree);
	
	DoMethod(data->lt_linklist, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		obj, 2,
		MM_Bookmarkgroup_Update, MV_BookmarkGroup_Update_QuickLink);
 
	DoMethod(data->st_title, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		obj, 3,
		MUIM_Set, MA_Bookmarkgroup_Title, MUIV_TriggerValue);
	DoMethod(data->st_alias, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		obj, 3,
		MUIM_Set, MA_Bookmarkgroup_Alias, MUIV_TriggerValue);
	DoMethod(data->st_address, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
		obj, 3,
		MUIM_Set, MA_Bookmarkgroup_Address, MUIV_TriggerValue);
	return (0);
}

DEFTMETHOD(Bookmarkgroup_KillNotification)
{
	GETDATA;
	DoMethod(data->lt_bookmark, MUIM_KillNotifyObj, MUIA_Listtree_Active, obj);
	DoMethod(data->lt_linklist, MUIM_KillNotifyObj, MUIA_List_Active,     obj);
	DoMethod(data->st_title,    MUIM_KillNotifyObj, MUIA_String_Contents, obj);
	DoMethod(data->st_alias,    MUIM_KillNotifyObj, MUIA_String_Contents, obj);
	DoMethod(data->st_address,  MUIM_KillNotifyObj, MUIA_String_Contents, obj);
	return (0);
}

BOOL MyStrCpyOnly(STRPTR *d, STRPTR s)
{
    if (!s)
    {
		D(kprintf("## Source is NULL!\n"));
        return(FALSE);
    }
    *d=(char *)malloc(strlen(s)+1);
    if (*d==NULL)
    {
		D(kprintf("## No memory for Destination!\n"));
        return(FALSE);
    }
    strcpy(*d,s);
    return(TRUE);
}

static void build_menu(Object *menu, Object *lt, struct MUIS_Listtree_TreeNode *list)
{
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *node;   
	ULONG  pos=0;
	STRPTR title,address;
	Object *item, *submenu;

	for(pos=0; ; pos++)
	{
		if( (tn=(struct MUIS_Listtree_TreeNode *)DoMethod(lt, MUIM_Listtree_GetEntry, list, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
		{
			node = (struct treedata *)tn->tn_User;
			if(node)
			{
				if(tn->tn_Flags & TNF_LIST)
				{
					if(node->flags & NODEFLAG_INMENU)
					{
						if (node->title) MyStrCpyOnly(&title, node->title);
						else MyStrCpyOnly(&title,"");

						submenu = (Object *)NewObject(getmenuclass(), NULL, MUIA_Menu_Title, title, TAG_DONE);

						if(submenu)
						{
							D(kprintf("Add sub menu\n"));
							DoMethod(menu, MUIM_Family_AddTail, submenu);
							build_menu(submenu, lt, tn);
						}
					}
					else
					{
						build_menu(menu, lt, tn);
					}
				}
				else
				{
					if(node->flags & NODEFLAG_INMENU)
					{
						if(node->flags & NODEFLAG_BAR)
						{
							// Add separator
							D(kprintf("Add separator\n"));
							item = MenuitemObject,
			                    MUIA_Menuitem_Title,  NM_BARLABEL,
								MUIA_UserData, NULL,
								End;

							if(item)
							{
								DoMethod(menu, MUIM_Family_AddTail, item);
							}
						}
						else
						{
							WTF::String truncatedTitle;
							struct menu_entry *menu_entry;

							if (node->title) MyStrCpyOnly(&title,node->title);
							else MyStrCpyOnly(&title,"");
							
							if (node->address) MyStrCpyOnly(&address,node->address);
							else MyStrCpyOnly(&address,"");
							
							D(kprintf("Add link\n"));

							// truncate title
							truncatedTitle = title;
							if(truncatedTitle.length() > 64)
							{
								truncatedTitle.truncate(64);
								truncatedTitle.append("...");
							}

							menu_entry = (struct menu_entry *) malloc(sizeof(*menu_entry) + strlen(address) + 1);

							if(menu_entry)
							{
								menu_entry->type = MENUTYPE_BOOKMARK;
								stccpy(menu_entry->data, address, strlen(address) + 1);

								item = (Object *)NewObject(getmenuitemclass(), NULL,
									MUIA_Menuitem_Title, strdup(truncatedTitle.latin1().data()),
									MUIA_UserData,       menu_entry,
									End;

								if(item)
								{
									DoMethod(menu, MUIM_Family_AddTail, item);
								}
							}

							free(title);
							free(address);
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
}
 
DEFSMETHOD(Bookmarkgroup_BuildMenu)
{
	GETDATA;
	Object *item;
	Object *menu = msg->menu;

	// Add bookmark menu to the menu object
	DoMethod(menu, MUIM_Menustrip_InitChange);

	item = (Object *)MenuitemObject,
								MUIA_Menuitem_Title, GSI(MSG_MENU_ADD_BOOKMARK),
								MUIA_Menuitem_Shortcut, "A",
								MUIA_UserData, MNA_ADD_BOOKMARK,
								End;
	if(item)
	{
		DoMethod(menu, MUIM_Family_AddTail, item);
	}

	item = (Object *)MenuitemObject,
								MUIA_Menuitem_Title, GSI(MSG_MENU_MANAGE_BOOKMARKS),
								MUIA_Menuitem_Shortcut, "M",
								MUIA_UserData, MNA_BOOKMARKS_WINDOW,
								End;

	if(item)
	{
		DoMethod(menu, MUIM_Family_AddTail, item);
	}

	item = (Object *)MenuitemObject,
								MUIA_Menuitem_Title,  NM_BARLABEL,
								MUIA_UserData, MNA_BOOKMARKS_STOP,
								End;
	if(item)
	{
		DoMethod(menu, MUIM_Family_AddTail, item);
	}

	build_menu(menu, data->lt_bookmark, MUIV_Listtree_GetEntry_ListNode_Root);

	DoMethod(menu, MUIM_Menustrip_ExitChange);

	return (0);
}

#ifndef MUIM_Family_GetChild
#define MUIM_Family_GetChild                0x8042c556 /* V20 */
struct  MUIP_Family_GetChild                { ULONG MethodID; LONG nr; Object *ref; };
#endif

DEFTMETHOD(Bookmarkgroup_UpdateMenu)
{
	GETDATA;
	D(kprintf("Bookmark UpdateMenu\n"));

	FORCHILD(app, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			Object *menu = (Object *) DoMethod((Object *) getv(child, MUIA_Window_Menustrip), MUIM_FindUData, MNA_BOOKMARKS_MENU);

			if(menu)
			{
				ULONG id,found=FALSE;
				Object *child2;

				DoMethod(menu, MUIM_Menustrip_InitChange);
				// Remove all items after 3 fist entries
				while( (child2=(Object *)DoMethod(menu, MUIM_Family_GetChild, MUIV_Family_GetChild_Last, NULL)) )
				{
					if (get(child2, MUIA_UserData, &id))
					{
						if (id==MNA_BOOKMARKS_STOP)
						{
							found=TRUE;
							break;
						}
						if (!found)
						{
							DoMethod(menu, OM_REMMEMBER, child2);
							DoMethod(child2, OM_DISPOSE);
						}
					}
				}

				// Rebuild the menu
				build_menu(menu, data->lt_bookmark, MUIV_Listtree_GetEntry_ListNode_Root);
				DoMethod(menu, MUIM_Menustrip_ExitChange);
			}
		}
	}
	NEXTCHILD

	return (0);
}

DEFSMETHOD(Bookmarkgroup_AddQuickLink)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *node=NULL;

	D(kprintf("\nBookmarkGroup AddQuickLink in pos %ld\n",msg->pos));
	if (msg->node)
	{
		// Use the one provided
		node=msg->node;
	}
	else
	{
		// Get current in bookmark list
		if( (tn=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0)) )
		{
			node=(struct treedata *)tn->tn_User;
			node->treenode = tn;
		}
	}

	if ((node) && (node->flags & NODEFLAG_NOTATTACHED))
	{
		// Link from other add to bookmark
		D(kprintf("External node, add it to bookmark\n"));
		node=(struct treedata *)DoMethod(obj, MM_Bookmarkgroup_AddLink, node->title, node->alias, node->address, TRUE, TRUE, FALSE);
	}

	if (node)
	{
		// Set flag quicklink
		char temp[32];
		int size=sizeof(temp)-1;
		LONG line;
		APTR n;

		node->flags|=NODEFLAG_QUICKLINK;

		if ( !node->alias || *(node->alias)=='\0' )
		{
			// No alias name copy from Title
			snprintf(temp, size, "%s", (node->title) ? (node->title) : "(Anonymous)");
			if ((int)strlen(temp)>(size-2))
			{
				temp[size-2]='.';
				temp[size-3]='.';
				temp[size-4]='.';
			}
			D(kprintf("Alias new name '%s'\n",temp));

			if (node->alias) FreeVecTaskPooled(node->alias);
			node->alias=(STRPTR)AllocVecTaskPooled(strlen(temp)+1);
			if (node->alias) strcpy(node->alias, temp);
		}

		if (msg->pos==-1) line=MUIV_List_Insert_Bottom;
		else line=msg->pos-1;

		line=DoMethod(data->lt_linklist, MUIM_List_InsertSingle,
				node,
				line
				);

		DoMethod(obj, MM_Bookmarkgroup_ReadQLOrder);

		// Add to quicklink group
		ITERATELIST(n, &data->qlgroups)
		{
			struct qlgroupnode *qn = (struct qlgroupnode *) n;
			if (qn->obj)
			{
				DoMethod(qn->obj, MM_QuickLinkGroup_InitChange, MV_QuickLinkGroup_Change_Add);
				D(kprintf("Add node to qlgroup %08lx\n", qn->obj));
				DoMethod(qn->obj, MM_QuickLinkGroup_Add, node);
				DoMethod(qn->obj, MM_QuickLinkGroup_ExitChange, MV_QuickLinkGroup_Change_Add);
			}
		}
		DoMethod(obj, MM_Bookmarkgroup_Update, MV_BookmarkGroup_Update_All);
		set(data->lt_linklist, MUIA_List_Active, line);
		DoMethod(app, MUIM_Application_PushMethod,
			obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
			MM_Bookmarkgroup_AutoSave);
	}
	D(kprintf("End AddQuickLink\n"));
	return (0);
}

DEFSMETHOD(Bookmarkgroup_RemoveQuickLink)
{
	GETDATA;
	struct treedata *node=NULL;
	LONG pos=0,max=0;

	if (msg->node)
	{
		// Found pos in linklist
		get(data->lt_linklist, MUIA_List_Entries, &max);
		for (pos=0;pos<max;pos++)
		{
			if (DoMethod(data->lt_linklist, MUIM_List_GetEntry, pos, &node))
			{
				if (msg->node==node) break;
				node=NULL;
			}
		}
	}
	else
	{
		// Get active item in linklist
		if (get(data->lt_linklist, MUIA_List_Active, &pos))
		{
			DoMethod(data->lt_linklist, MUIM_List_GetEntry, pos, &node);
		}
	}

	if (node)
	{
		APTR n;
		D(kprintf("Remove QuickLink %ld\n", pos));
		node->flags = node->flags & ~NODEFLAG_QUICKLINK;

		DoMethod(data->lt_linklist, MUIM_List_Remove, pos);
		DoMethod(obj, MM_Bookmarkgroup_ReadQLOrder);

		ITERATELIST(n, &data->qlgroups)
		{
			struct qlgroupnode *qn = (struct qlgroupnode *) n;
			// Remove from quicklink
			if (qn->obj)
			{
				DoMethod(qn->obj, MM_QuickLinkGroup_InitChange, MV_QuickLinkGroup_Change_Remove);
				DoMethod(qn->obj, MM_QuickLinkGroup_Remove, node);
				DoMethod(qn->obj, MM_QuickLinkGroup_ExitChange, MV_QuickLinkGroup_Change_Remove);
			}
		}

		DoMethod(obj, MM_Bookmarkgroup_Update, MV_BookmarkGroup_Update_All);
        DoMethod(app, MUIM_Application_PushMethod,
				obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
				MM_Bookmarkgroup_AutoSave);
		D(kprintf("Remove QuickLink End\n"));
	}
	return (0);
}

DEFTMETHOD(Bookmarkgroup_DosNotify)
{
	GETDATA;
	// Start reload
	D(kprintf("DosNotification on bookmarks.html\n"));
	// get active for reset after ?
	set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE);
	set(data->lt_linklist, MUIA_List_Quiet, TRUE);
	DoMethod(data->lt_linklist, MUIM_List_Clear);
	DoMethod(data->lt_bookmark, MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root, MUIV_Listtree_Remove_TreeNode_All, 0);
	DoMethod(obj, MM_Bookmarkgroup_LoadHtml, BOOKMARK_PATH);
	set(data->lt_linklist, MUIA_List_Quiet, FALSE);
	set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);
	return (0);
}

DEFSMETHOD(Bookmarkgroup_RegisterQLGroup)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *node;
	ULONG  pos=0;

	struct qlgroupnode * qn = (struct qlgroupnode *) malloc(sizeof(*qn));

	if(qn)
	{
		qn->obj = msg->group;
		ADDTAIL(&data->qlgroups, (APTR) qn);

		DoMethod(obj, MM_Bookmarkgroup_ReadQLOrder);

		// Define parent group of quicklinkgroup
		set(msg->group, MA_QuickLinkGroup_Parent, msg->parent);

		DoMethod(qn->obj, MM_QuickLinkGroup_InitChange, MV_QuickLinkGroup_Change_Add);

		D(kprintf("RegisterQLGroup Build QuickLink\n"));
		for(pos=0; ; pos++)
		{
			if( (tn=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, MUIV_Listtree_GetEntry_ListNode_Root, pos, 0)) )
			{
				node=(struct treedata *)tn->tn_User;
				node->treenode = tn;
				if ( (node) && (node->flags & NODEFLAG_QUICKLINK) )
				{
					D(kprintf("Add link\n"));
					DoMethod(msg->group, MM_QuickLinkGroup_Add, node);
				}
			}
			else break;
		}
		D(kprintf("End\n"));
		DoMethod(qn->obj, MM_QuickLinkGroup_ExitChange, MV_QuickLinkGroup_Change_Add);
	}
	return (0);
}

DEFSMETHOD(Bookmarkgroup_UnRegisterQLGroup)
{
	GETDATA;
	struct treedata *td;
	ULONG count,max=0;
	APTR n = NULL;
	Object *group = NULL;

	ITERATELIST(n, &data->qlgroups)
	{
		struct qlgroupnode *qn = (struct qlgroupnode *) n;
		if(qn->obj == msg->group)
		{
			group = qn->obj;
			break;
		}
	}

	if(group)
	{
		get(data->lt_linklist, MUIA_List_Entries, &max);
		DoMethod(group, MM_QuickLinkGroup_InitChange, MV_QuickLinkGroup_Change_Remove);
		for (count=0;count<max;count++)
		{
			if (DoMethod(data->lt_linklist, MUIM_List_GetEntry, count, &td))
			{
				DoMethod(group, MM_QuickLinkGroup_Remove, td);
			}
		}
		DoMethod(group, MM_QuickLinkGroup_ExitChange, MV_QuickLinkGroup_Change_Remove);
		REMOVE(n);
		free(n);
	}
	return (0);
}

DEFMMETHOD(HandleInput)
{
	if (msg->imsg)
	{
		switch (msg->imsg->Class)
		{
			case IDCMP_RAWKEY:
				{
					// Wheel mouse only on position
					/*
					if (_isinobject(msg->imsg->MouseX,msg->imsg->MouseY))
					{
						if (msg->imsg->Code==0x7a)
						{
							// +1
							set(obj, MUIA_TaskType_Current, MUIV_TaskType_Current_Next);
						}
						if (msg->imsg->Code==0x7b)
						{
							// -1
							set(obj, MUIA_TaskType_Current, MUIV_TaskType_Current_Prev);
						}
						PF("IDCMP_RAWKEY: %08lx\n",msg->imsg->Code);
					}*/

					// Cursor key only if string active
					GETDATA;
					Object *active=NULL;
					LONG pos=0;

					get(data->win, MUIA_Window_ActiveObject, &active);
					D(kprintf("Handle %08lx Obj %08lx\n", msg->imsg->Code, active));
					if (active==data->st_alias)
					{
						if (msg->imsg->Code==0x4d)
						{
							// +1
							get(data->lt_linklist, MUIA_List_Active, &pos);
							pos++;
							set(data->lt_linklist, MUIA_List_Active, pos);
						}
						if (msg->imsg->Code==0x4c)
						{
							// -1
							get(data->lt_linklist, MUIA_List_Active, &pos);
							if (pos>0)
							{
								pos--;
								set(data->lt_linklist, MUIA_List_Active, pos);
							}
						}
					}
					if (active==data->st_title || active==NULL)
					{
						struct MUIS_Listtree_TreeNode *active,*parent;
						active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
						if (msg->imsg->Code==0x4d)
						{
							// +1
							if ( active->tn_Flags & TNF_OPEN )
							{
								active=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, pos, MUIV_Listtree_GetEntry_Flags_SameLevel);
								if (active) set(data->lt_bookmark, MUIA_Listtree_Active, active);
							}
							else
							{
								active=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Next, MUIV_Listtree_GetEntry_Flags_Visible);
								if (active) set(data->lt_bookmark, MUIA_Listtree_Active, active);
								else
								{
									active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
									parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);
									if (parent)
									{
										active=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, parent, MUIV_Listtree_GetEntry_Position_Next, MUIV_Listtree_GetEntry_Flags_Visible);
										if (active) set(data->lt_bookmark, MUIA_Listtree_Active, active);
									}
								}
							}
						}
						if (msg->imsg->Code==0x4c)
						{
							// -1
							set(data->lt_bookmark, MUIA_Listtree_Quiet, TRUE);
							active=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Previous, MUIV_Listtree_GetEntry_Flags_Visible);
							if (active)
							{
								set(data->lt_bookmark, MUIA_Listtree_Active, active);
								if ( active->tn_Flags & TNF_OPEN )
								{
									active=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Tail, MUIV_Listtree_GetEntry_Flags_SameLevel);
									if (active) set(data->lt_bookmark, MUIA_Listtree_Active, active);
								}
							}
							else
							{
								active = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
								parent = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_bookmark, MUIM_Listtree_GetEntry, active, MUIV_Listtree_GetEntry_Position_Parent, 0);
								if (parent)
								{
									set(data->lt_bookmark, MUIA_Listtree_Active, parent);
								}
							}
							set(data->lt_bookmark, MUIA_Listtree_Quiet, FALSE);
						}
					}

				}
			break;
		}
	}

	return(DoSuperMethodA(cl,obj,(Msg)msg)); /*FR* Added (Msg) */
}

DEFSMETHOD(Search)
{
	GETDATA;
	data->nofocuschange = TRUE;
	DoMethodA(data->lt_bookmark, (_Msg_*)msg);
	data->nofocuschange = FALSE;

	return 0;
}

static ULONG contains_entry(struct IClass *cl, Object *obj, struct MUIS_Listtree_TreeNode *list, char *url)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *tn;
	struct treedata *node;
	ULONG  pos=0;
	ULONG found = FALSE;
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
					found = contains_entry(cl, obj, tn, url);
				}
				else
				{
					if(node->address)
					{
						URL kurl = URL(ParsedURLString, url);
						URL kbookmark = URL(ParsedURLString, node->address);

						if(kurl == kbookmark)
						{
							found = TRUE;
						}
					}
				}

				if(found)
					break;
			}
		}
		else
			break;
	}

	return found;
}

DEFSMETHOD(Bookmarkgroup_ContainsURL)
{
	return contains_entry(cl, obj, MUIV_Listtree_GetEntry_ListNode_Root, (char *) msg->url);
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
	LONG type = getv(msg->obj, MA_OWB_ObjectType);
	STRPTR url = (STRPTR) getv(msg->obj, MA_OWB_URL);
	STRPTR title = NULL;

	if(type == MV_OWB_ObjectType_Browser || type == MV_OWB_ObjectType_Tab)
	{
		title = (STRPTR) getv(msg->obj, MA_OWB_Title);
	}

	if(url)
	{
		DoMethod(_app(obj), MM_Bookmarkgroup_AddLink, title ? title : GSI(MSG_BOOKMARKGROUP_UNTITLED), NULL, url, TRUE, TRUE, FALSE);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
//DECGET
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleInput)
DECSMETHOD(Bookmarkgroup_Update)
DECSMETHOD(Bookmarkgroup_AddGroup)
DECSMETHOD(Bookmarkgroup_AddLink)
DECTMETHOD(Bookmarkgroup_AddSeparator) 	
DECTMETHOD(Bookmarkgroup_Remove)
DECSMETHOD(Bookmarkgroup_LoadHtml)
DECSMETHOD(Bookmarkgroup_SaveHtml)
DECTMETHOD(Bookmarkgroup_BuildNotification)
DECTMETHOD(Bookmarkgroup_KillNotification)
DECSMETHOD(Bookmarkgroup_BuildMenu)
DECTMETHOD(Bookmarkgroup_UpdateMenu)
DECSMETHOD(Bookmarkgroup_AddQuickLink)
DECTMETHOD(Bookmarkgroup_ReadQLOrder)
DECSMETHOD(Bookmarkgroup_RemoveQuickLink)
DECTMETHOD(Bookmarkgroup_AutoSave)
DECTMETHOD(Bookmarkgroup_DosNotify)
DECSMETHOD(Bookmarkgroup_RegisterQLGroup)
DECSMETHOD(Bookmarkgroup_UnRegisterQLGroup)
DECSMETHOD(Search)
DECSMETHOD(Bookmarkgroup_ContainsURL)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, bookmarkgroupclass)
