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
#include "CookieManager.h"
#include "CookieMap.h"
#include "ParsedCookie.h"
#include "URL.h"

#include <proto/intuition.h>
#include <proto/utility.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <mui/Listtree_mcc.h>

#include <clib/macros.h>

#include "gui.h"

#define D(x)

using namespace WebCore;

struct Data
{
	Object *findgroup;
	Object *lt_cookies;
	Object *txt_domain;
	Object *txt_name;
	Object *txt_value;
	Object *txt_path;
	Object *txt_expiration;
	Object *txt_type;
};

DEFNEW
{
	Object *lt_cookies, *bt_remove, *bt_clear, *txt_name, *txt_value, *txt_domain, *txt_path, *txt_expiration, *txt_type, *findgroup;

	obj = (Object *) DoSuperNew(cl, obj,

		Child, findgroup = (Object *) NewObject(getfindtextclass(), NULL,
												MUIA_ShowMe, TRUE,
												MA_FindText_Closable, FALSE,
												MA_FindText_ShowButtons, FALSE,
												MA_FindText_ShowCaseSensitive, FALSE,
												MA_FindText_ShowText, FALSE,
												TAG_DONE),

		Child, lt_cookies = (Object *) NewObject(getcookiemanagerlisttreeclass(), NULL, TAG_DONE),

		Child, ColGroup(2), GroupFrame,
			MUIA_Background, MUII_GroupBack,
			Child, Label(GSI(MSG_COOKIEMANAGERGROUP_NAME)),
			Child, txt_name = TextObject,
				MUIA_Text_Contents, "",
				MUIA_Text_SetMin, FALSE,
			End,
			Child, Label(GSI(MSG_COOKIEMANAGERGROUP_VALUE)),
			Child, txt_value = TextObject,
				MUIA_Text_Contents, "",
				MUIA_Text_SetMin, FALSE,
			End,
			Child, Label(GSI(MSG_COOKIEMANAGERGROUP_DOMAIN)),
			Child, txt_domain = TextObject,
				MUIA_Text_Contents, "",
				MUIA_Text_SetMin, FALSE,
			End,
			Child, Label(GSI(MSG_COOKIEMANAGERGROUP_PATH)),
			Child, txt_path = TextObject,
				MUIA_Text_Contents, "",
				MUIA_Text_SetMin, FALSE,
			End,
			Child, Label(GSI(MSG_COOKIEMANAGERGROUP_TYPE)),
			Child, txt_type = TextObject,
				MUIA_Text_Contents, "",
				MUIA_Text_SetMin, FALSE,
			End,
			Child, Label(GSI(MSG_COOKIEMANAGERGROUP_EXPIRATION)),
			Child, txt_expiration = TextObject,
				MUIA_Text_Contents, "",
				MUIA_Text_SetMin, FALSE,
			End,
		End,

		Child, HGroup,
			Child, bt_remove = (Object *) MakeButton(GSI(MSG_COOKIEMANAGERGROUP_REMOVE)),
			Child, bt_clear  = (Object *) MakeButton(GSI(MSG_COOKIEMANAGERGROUP_REMOVE_ALL)),
		End,
		TAG_MORE, INITTAGS
		);

	if (obj)
	{
		GETDATA;

		data->lt_cookies = lt_cookies;
		data->txt_domain = txt_domain;
		data->txt_name = txt_name;
		data->txt_value = txt_value;
		data->txt_path = txt_path;
		data->txt_expiration = txt_expiration;
		data->txt_type = txt_type;
		data->findgroup = findgroup;

		set(data->findgroup, MA_FindText_Target, data->lt_cookies);

		DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_CookieManagerGroup_Remove);
		DoMethod(bt_clear, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_CookieManagerGroup_Clear);
		DoMethod(lt_cookies, MUIM_Notify, MUIA_Listtree_Active, MUIV_EveryTime, obj, 1, MM_CookieManagerGroup_DisplayProperties);

		//DoMethod(obj, MM_CookieManagerGroup_Load);
	}

	return ((ULONG)obj);
}

DEFDISP
{
	return DOSUPER;
}

DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;

	if(rc)
	{
		DoMethod(obj, MM_CookieManagerGroup_Load);
	}
	return rc;

}

DEFMMETHOD(Cleanup)
{
	return DOSUPER;
}

DEFTMETHOD(CookieManagerGroup_Load)
{
	GETDATA;

	DoMethod(data->lt_cookies, MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root, MUIV_Listtree_Remove_TreeNode_All, 0);

	D(kprintf("Loading cookies\n"));

	HashMap<String, CookieMap*>& manager_map = cookieManager().getCookieMap();

	Vector<ParsedCookie*> cookies;
	for (HashMap<String, CookieMap*>::iterator it = manager_map.begin(); it != manager_map.end(); ++it)
        it->value->getAllChildCookies(&cookies);

	for (size_t i = 0; i < cookies.size(); ++i)
	{
		ParsedCookie* cookie = cookies[i];

		struct MUIS_Listtree_TreeNode* group = NULL;
        struct cookie_entry node;
		char *domain = strdup(cookie->domain().utf8().data());

		group = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_cookies, MUIM_Listtree_FindName,
			MUIV_Listtree_FindName_ListNode_Root,
			domain, MUIV_Listtree_FindName_Flags_SameLevel
		);

		// Domain is not inserted yet, do it
		if(group == NULL)
		{
			node.flags = COOKIEFLAG_DOMAIN;
			node.name = NULL;
			node.value = NULL;
			node.domain = strdup(domain);
			node.protocol = NULL;
			node.path = NULL;
			node.expiry = 0;
			node.secure = FALSE;
			node.http_only = FALSE;

			group = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_cookies, MUIM_Listtree_Insert,
					node.domain,
					&node,
					MUIV_Listtree_Insert_ListNode_Root,
					MUIV_Listtree_Insert_PrevNode_Sorted,
					TNF_LIST
			);
		}

		node.flags = COOKIEFLAG_COOKIE;
		node.name = strdup(cookie->name().utf8().data());
		node.value = strdup(cookie->value().utf8().data());
		node.domain = strdup(domain);
		node.protocol = strdup(cookie->protocol().utf8().data());
		node.path = strdup(cookie->path().utf8().data());
		node.expiry = cookie->expiry();
		node.secure = cookie->isSecure();
		node.http_only = cookie->isHttpOnly();
		node.session = cookie->isSession();

		D(kprintf("protocol <%s>\n", node.protocol));
		D(kprintf("\tdomain <%s>\n", node.domain));
		D(kprintf("\tname <%s>\n", node.name));
		D(kprintf("\tpath <%s>\n", node.path));

		/*
		newentry = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_cookies, MUIM_Listtree_FindName,
			group,
			node.name, MUIV_Listtree_FindName_Flags_SameLevel
		);

		if(newentry && !strcmp(node.path, newentry.path) && !strcmp(node.protocol, newentry.protocol))
		{
		
		}
		*/

		DoMethod(data->lt_cookies, MUIM_Listtree_Insert,
			node.name,
			&node,
			group,
			MUIV_Listtree_Insert_PrevNode_Sorted,
			0
			);

		free(domain);
	}

	return 0;
}

DEFTMETHOD(CookieManagerGroup_Clear)
{
	GETDATA;

	/* Remove from list */
	DoMethod(data->lt_cookies, MUIM_Listtree_Remove, MUIV_Listtree_Remove_ListNode_Root, MUIV_Listtree_Remove_TreeNode_All, 0);

	/* Remove from map(s) and database */
	cookieManager().removeAllCookies(RemoveFromBackingStore);

	return 0;
}

DEFSMETHOD(CookieManagerGroup_DidInsert)
{
#if 0
	GETDATA;
	int pos;
	struct MUIS_Listtree_TreeNode *tn = NULL;
	ParsedCookie* cookie = (ParsedCookie *) msg->cookie;

	D(kprintf("didInsert <%s><%s>\n", cookie->domain().utf8().data(), cookie->name().utf8().data()));

	for(pos=0; ; pos++)
	{
		if( (tn=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_cookies, MUIM_Listtree_GetEntry, MUIV_Listtree_GetEntry_ListNode_Root, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
		{
			struct cookie_entry *node = (struct cookie_entry *)tn->tn_User;
			if(node && node->flags == COOKIEFLAG_DOMAIN && cookie->domain() == node->domain)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if(!tn)
	{
		struct cookie_entry node;

		node.flags = COOKIEFLAG_DOMAIN;
		node.name = NULL;
		node.value = NULL;
		node.domain = strdup(cookie->domain().utf8().data());
		node.path = NULL;
		node.expiry = 0;
		node.secure = FALSE;
		node.http_only = FALSE;
		node.session = FALSE;

		tn = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_cookies, MUIM_Listtree_Insert,
				node.domain,
				&node,
				MUIV_Listtree_Insert_ListNode_Root,
				MUIV_Listtree_Insert_PrevNode_Sorted,
				TNF_LIST
			);
	
	}

	if(tn)
	{
		struct MUIS_Listtree_TreeNode *newentry = NULL;
		struct cookie_entry node;

		node.flags = COOKIEFLAG_COOKIE;
		node.name = strdup(cookie->name().utf8().data());
		node.value = strdup(cookie->value().utf8().data());
		node.domain = strdup(cookie->domain().utf8().data());
		node.path = strdup(cookie->path().utf8().data());
		node.expiry = cookie->expiry();
		node.secure = cookie->isSecure();
		node.http_only = cookie->isHttpOnly();
		node.session = cookie->isSession();

		newentry = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_cookies, MUIM_Listtree_Insert,
			node.name,
			&node,
			tn,
			MUIV_Listtree_Insert_PrevNode_Sorted,
			0
			);
	}

#endif
	return 0;
}

DEFSMETHOD(CookieManagerGroup_DidRemove)
{
#if 0
	GETDATA;
	int pos;
	struct MUIS_Listtree_TreeNode *tn = NULL, *tn2 = NULL;
	ParsedCookie *cookie = (ParsedCookie *) msg->cookie;

	D(kprintf("didRemove <%s><%s>\n", cookie->domain().utf8().data(), cookie->name().utf8().data()));

	/* find the domain node */
	for(pos=0; ; pos++)
	{
		if( (tn=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_cookies, MUIM_Listtree_GetEntry, MUIV_Listtree_GetEntry_ListNode_Root, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
		{
			struct cookie_entry *node = (struct cookie_entry *)tn->tn_User;
			if(node && node->flags == COOKIEFLAG_DOMAIN && cookie->domain() == node->domain)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	/* find the cookie node */
	if(tn)
	{
		for(pos=0; ; pos++)
		{
			if( (tn2=(struct MUIS_Listtree_TreeNode *)DoMethod(data->lt_cookies, MUIM_Listtree_GetEntry, tn, pos, MUIV_Listtree_GetEntry_Flags_SameLevel)) )
			{
				struct cookie_entry *node = (struct cookie_entry *)tn2->tn_User;

				if(node && node->flags == COOKIEFLAG_COOKIE && cookie->name() == node->name && cookie->path() == node->path)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}	 
	}

	/* remove it */
	if(tn2)
	{
		DoMethod(data->lt_cookies, MUIM_Listtree_Remove, NULL, tn2, 0);
	}
#endif

	return 0;
}

DEFTMETHOD(CookieManagerGroup_Remove)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *tn;

	tn = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_cookies, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
	if(tn)
	{
		struct cookie_entry *entry = (struct cookie_entry *) tn->tn_User;

		if(entry && entry->flags == COOKIEFLAG_COOKIE)
		{
			String cookieURL = String(entry->protocol);
			String domain = String(entry->domain);
			cookieURL.append("://");
			if(domain.startsWith(".", false))
				domain = domain.substring(1);
			cookieURL.append(domain);
			cookieURL.append(entry->path);
			URL url(ParsedURLString, cookieURL);
			cookieManager().removeCookieWithName(url, entry->name);
			DoMethod(data->lt_cookies, MUIM_Listtree_Remove, NULL, MUIV_Listtree_Remove_TreeNode_Active, 0);
		}
		else if(entry && entry->flags == COOKIEFLAG_DOMAIN)
		{
			cookieManager().removeCookiesFromDomain("http", entry->domain);
			cookieManager().removeCookiesFromDomain("https", entry->domain);
            DoMethod(data->lt_cookies, MUIM_Listtree_Remove, NULL, MUIV_Listtree_Remove_TreeNode_Active, 0);
		}
	}

	return 0;
}

DEFTMETHOD(CookieManagerGroup_DisplayProperties)
{
	GETDATA;
	struct MUIS_Listtree_TreeNode *tn;

	tn = (struct MUIS_Listtree_TreeNode *) DoMethod(data->lt_cookies, MUIM_Listtree_GetEntry, NULL, MUIV_Listtree_GetEntry_Position_Active, 0);
	if(tn)
	{
		struct cookie_entry *entry = (struct cookie_entry *) tn->tn_User;

		if(entry && entry->flags == COOKIEFLAG_COOKIE)
		{
			char type[256];
			char expiration[256];
			time_t expiry = (time_t) entry->expiry;
			snprintf(type, sizeof(type), "%s%s%s", entry->session ? "Session" : "Permanent", entry->http_only ? " | HTTP Only" : " ", entry->secure ? " | Secure" : "");
			snprintf(expiration, sizeof(expiration), "%s", asctime(localtime(&expiry)));
			expiration[strlen(expiration) - 1] = 0;

			set(data->txt_name, MUIA_Text_Contents, entry->name);
			set(data->txt_value, MUIA_Text_Contents, entry->value);
			set(data->txt_domain, MUIA_Text_Contents, entry->domain);
			set(data->txt_path, MUIA_Text_Contents, entry->path);
			set(data->txt_type, MUIA_Text_Contents, type);
			set(data->txt_expiration, MUIA_Text_Contents, expiration);
		}
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECTMETHOD(CookieManagerGroup_Load)
DECTMETHOD(CookieManagerGroup_Clear)
DECSMETHOD(CookieManagerGroup_DidInsert)
DECSMETHOD(CookieManagerGroup_DidRemove)
DECTMETHOD(CookieManagerGroup_Remove)
DECTMETHOD(CookieManagerGroup_DisplayProperties)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, cookiemanagergroupclass)
