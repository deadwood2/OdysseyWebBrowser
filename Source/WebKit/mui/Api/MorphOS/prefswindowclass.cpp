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
#include "WebPreferences.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <strings.h>
#include <stdlib.h>

#include <curl/curl.h>
#include "gui.h"
#include "versioninfo.h"

#include <exec/resident.h>

#define LABEL(x) (STRPTR)MSG_PREFSWINDOW_##x

STATIC CONST CONST_STRPTR prefslist[] =
{
	LABEL(CATEGORY_GENERAL),
	LABEL(CATEGORY_INTERFACE),
	LABEL(CATEGORY_CONTEXTUAL_MENUS),
	LABEL(CATEGORY_MIMETYPES),
	LABEL(CATEGORY_CONTENT),
	LABEL(CATEGORY_FONTS),
	LABEL(CATEGORY_DOWNLOADS),
	LABEL(CATEGORY_NETWORK),
	LABEL(CATEGORY_PRIVACY),
	LABEL(CATEGORY_SECURITY),
	LABEL(CATEGORY_MEDIA),
	NULL
};

STATIC CONST CONST_STRPTR cachemodels[] =
{
	LABEL(CACHE_DOCUMENT_VIEWER),
	LABEL(CACHE_DOCUMENT_BROWSER),
	LABEL(CACHE_PRIMARY_BROWSER),
	NULL
};

STATIC CONST CONST_STRPTR textencodings[] =
{
	"ISO-8859-1",
	"ISO-8859-2",
	"ISO-8859-3",
	"ISO-8859-4",
	"ISO-8859-5",
	"ISO-8859-6",
	"ISO-8859-7",
	"ISO-8859-8",
	"ISO-8859-9",
	"ISO-8859-10",
	"ISO-8859-13",
	"ISO-8859-14",
	"ISO-8859-15",
	"UTF-8",
	"BIG5",
	"SHIFT-JIS",
	"EUC-JP",
	"ISO-2022-JP",
	"ISO-2022-JP-1",
	"CP1250",
	"CP1251",
	"CP1252",
	"CP1253",
	"CP1254",
	"CP1255",
	"CP1256",
	"CP1257",
	"CP1258",
	"CP874",
	"KOI8-R",
	"KOI8-RU",
	"UCS-2",
	"UCS-2BE",
	"UCS-2LE",
	NULL
};

STATIC CONST CONST_STRPTR smoothingmethods[] =
{
	LABEL(SMOOTH_NORMAL),
	LABEL(SMOOTH_ALTERNATIVE),
	NULL,
};

STATIC CONST CONST_STRPTR newpagepolicies[] =
{
	LABEL(NEWPAGE_NEWTAB),
	LABEL(NEWPAGE_NEWWINDOW),
	NULL
};

STATIC CONST CONST_STRPTR newpagepositions[] =
{
	LABEL(NEWPAGE_LAST),
	LABEL(NEWPAGE_AFTERACTIVE),
	NULL
};

STATIC CONST CONST_STRPTR quicklinklooks[] =
{
	LABEL(QUICKLINK_BUTTON),
	LABEL(QUICKLINK_LINK),
	NULL
};

STATIC CONST CONST_STRPTR quicklinklayout[] =
{
	LABEL(QUICKLINK_DYNAMIC),
	LABEL(QUICKLINK_FIXED),
	LABEL(QUICKLINK_BESTFIT),
	NULL
};

STATIC CONST CONST_STRPTR toolbuttontypes[] =
{
	LABEL(NAVIGATION_ICON),
	LABEL(NAVIGATION_TEXT),
	LABEL(NAVIGATION_ICON_AND_TEXT),
	NULL
};

STATIC CONST CONST_STRPTR cookiepolicies[] =
{
	LABEL(COOKIE_ACCEPT),
	LABEL(COOKIE_REJECT),
//	  LABEL(COOKIE_ASK),
	NULL
};

STATIC CONST CONST_STRPTR completiontypes[] =
{
	LABEL(COMPLETION_NONE),
	LABEL(COMPLETION_STRING),
	LABEL(COMPLETION_POPUP),
	LABEL(COMPLETION_STRING_AND_POPUP),
	NULL
};

STATIC CONST CONST_STRPTR closerequesterpolicies[] =
{
	LABEL(CLOSE_REQUESTER_ALWAYS),
	LABEL(CLOSE_REQUESTER_NEVER),
	LABEL(CLOSE_REQUESTER_IF_MORE_THAN_ONE),
	NULL
};

STATIC CONST CONST_STRPTR errorreportingmodes[] =
{
	LABEL(ERROR_REQUESTER),
	LABEL(ERROR_HTML),
	NULL,
};

STATIC CONST CONST_STRPTR animationpolicies[] =
{
	LABEL(ANIMATE_NEVER),
	LABEL(ANIMATE_ALWAYS),
	LABEL(ANIMATE_ACTIVE_WINDOW),
	NULL
};

STATIC CONST CONST_STRPTR middlebuttonpolicies[] =
{
	LABEL(INTERFACE_MIDDLEBUTTON_BEHAVIOUR_NEW_BACKGROUND_TAB),
	LABEL(INTERFACE_MIDDLEBUTTON_BEHAVIOUR_NEW_TAB),
	LABEL(INTERFACE_MIDDLEBUTTON_BEHAVIOUR_NEW_WINDOW),
	NULL
};

STATIC CONST CONST_STRPTR restoresessionmodes[] =
{
	LABEL(PRIVACY_SESSION_RESTORATION_OFF),
	LABEL(PRIVACY_SESSION_RESTORATION_ASK),
	LABEL(PRIVACY_SESSION_RESTORATION_ON),
	NULL
};

STATIC CONST CONST_STRPTR loopfiltermodes[] =
{
	LABEL(MEDIA_LOOPFILTER_SKIP_NONE),
	LABEL(MEDIA_LOOPFILTER_SKIP_NONREF),
	LABEL(MEDIA_LOOPFILTER_SKIP_NONKEY),
	LABEL(MEDIA_LOOPFILTER_SKIP_ALL),
	NULL
};

STATIC CONST CONST_STRPTR proxy_types[] =
{
	"HTTP",
	"SOCKS 4",
	"SOCKS 4A",
	"SOCKS 5",
	"SOCKS 5 Hostname",
	NULL
};

STATIC CONST int proxy_values[] = 
{
    CURLPROXY_HTTP,
    CURLPROXY_SOCKS4,
    CURLPROXY_SOCKS4A,
    CURLPROXY_SOCKS5,
    CURLPROXY_SOCKS5_HOSTNAME,
    NULL,
};

struct useragent_pair
{
	STRPTR label;
	STRPTR string;
};

/* Keep the two arrays below in sync */
STATIC CONST CONST_STRPTR useragents_labels[] =
{
	"Odyssey Web Browser",

	"Firefox 38 (Linux)",
	"Firefox 3.6 (Windows)",

	"Internet Explorer 10",
	"Internet Explorer 8",
	"Internet Explorer 6",

	"Opera 12 (Windows)",

	"Safari 6.0.2 (Mac)",
	"Chrome 42 (Linux)",

	"IPhone",
	"IPad",

	NULL
};

#if OS(AROS)
#define OSHEADER "i686; AROS"
#elif OS(MORPHOS)
#define OSHEADER "Macintosh; PowerPC MorphOS %u.%u"
#endif
const char verfmt[] = "Mozilla/5.0 (" OSHEADER "; Odyssey Web Browser; rv:" VERSION ") AppleWebKit/" WEBKITVER " (KHTML, like Gecko) OWB/" VERSION " Safari/" WEBKITVER;
STATIC char odysseyuseragent[sizeof(verfmt) + 2 * (10 - 2)];

void init_useragent()
{
#if OS(AROS)
    strcpy(odysseyuseragent, verfmt);
#elif OS(MORPHOS)
	ULONG version = 0, revision = 0;
	struct Resident *res = FindResident("MorphOS");
	if (res)
	{
		version = res->rt_Version;
		if (res->rt_Flags & RTF_EXTENDED)
		{
			revision = res->rt_Revision;
		}
		else
		{
			char *p = index((char *)res->rt_IdString, '.');
			if (p) revision = strtoul(p + 1, NULL, 10);
		}
	}

	sprintf(odysseyuseragent, verfmt, version, revision);
#endif
}

STATIC CONST STRPTR useragents_strings[] =
{
	odysseyuseragent,

	"Mozilla/5.0 (X11; Ubuntu; Linux i686; rv:38.0) Gecko/20100101 Firefox/38.0",
	"Mozilla/5.0 (Windows NT 6.1; rv:1.9.2) Gecko/20100101 Firefox/3.6",

	"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.1; WOW64; Trident/6.0)",
	"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; .NET CLR 2.0.50727; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)",
	"Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; .NET CLR 1.1.4322)",

	"Opera/9.80 (Windows NT 5.1; U; en) Presto/2.12.388 Version/12.14",

	"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_8_2) AppleWebKit/536.26.17 (KHTML, like Gecko) Version/6.0.2 Safari/536.26.17",
	"Mozilla/5.0 (X11; Linux i686) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/42.0.2311.152 Safari/537.36",

	"Mozilla/5.0 (iPhone; CPU iPhone OS 6_0 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10A5376e Safari/8536.25",
	"Mozilla/5.0 (iPad; U; CPU OS 6_1 like Mac OS X; en-us) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10B141 Safari/8536.25",

	NULL
};

ULONG get_user_agent_count()
{
	return sizeof(useragents_labels)/sizeof(CONST_STRPTR) - 1 ;
}

CONST_STRPTR * get_user_agent_labels()
{
	return (CONST_STRPTR *) useragents_labels;
}

CONST_STRPTR * get_user_agent_strings()
{
	return (CONST_STRPTR *) useragents_strings;
}

static void cycles_init(void)
{
	APTR arrays[] = { (APTR) prefslist, (APTR) cachemodels, (APTR) smoothingmethods, (APTR) newpagepolicies, (APTR) newpagepositions,
					  (APTR) quicklinklooks, (APTR) quicklinklayout, (APTR) toolbuttontypes, (APTR) cookiepolicies,
					  (APTR) completiontypes, (APTR) closerequesterpolicies, (APTR) errorreportingmodes, (APTR) animationpolicies, (APTR) middlebuttonpolicies,
					  (APTR) restoresessionmodes, (APTR) loopfiltermodes,
					   NULL };

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

MUI_HOOK(familypopclose, APTR list, APTR str)
{
	STRPTR s;

	DoMethod((Object *) list, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &s);
	set((Object *) str, MUIA_String_Contents, s);
	set(_win((Object *) str), MUIA_Window_ActiveObject, (Object *) str);

	return 0;
}

MUI_HOOK(familypopopen, APTR pop, APTR win)
{
	Object *list = (Object *)getv((Object *)pop , MUIA_Listview_List);

	SetAttrs((Object *) win, MUIA_Window_DefaultObject, list, MUIA_Window_ActiveObject, list, TAG_DONE);
	set(list, MUIA_List_Active, 0);

	return (TRUE);
}

#define FontFamilyPopString(name, def, id) pop_##name = MUI_NewObject(MUIC_Popobject,\
			MUIA_Popstring_String, str_##name = StringObject,\
														  StringFrame,\
														  MUIA_String_Contents, def,\
														  MUIA_ObjectID, id,\
														  End,\
			MUIA_Popstring_Button, PopButton(MUII_PopUp),\
			MUIA_Popobject_Object, lv_##name = MUI_NewObject(MUIC_List, TAG_DONE),\
			MUIA_Popobject_ObjStrHook, &familypopclose_hook,\
			MUIA_Popobject_WindowHook, &familypopopen_hook,\
			MUIA_CycleChain, 1,\
			End

struct Data
{
	/**/
	Object *str_dldir;
	Object *ch_downloadautoclose;
	Object *ch_savedownloads;
	Object *ch_downloadstartautomatically;

	/**/
	Object *str_homepage;
	Object *str_startpage;
	Object *str_newtabpage;
	Object *str_clidevice;

	/**/
	Object *ch_enablejavascript;
	Object *ch_allowjavascriptnewwindow;
	Object *ch_enableanimation;
	Object *ch_enableanimationloop;
	Object *ch_enablepagecache;
	Object *cy_cachemodel;
	Object *ch_enableimages;
	Object *ch_shouldprintbackground;
	Object *ch_contentblocking;
	Object *ch_enableplugins;
	Object *ch_enableinspector;
	//Object *cy_animationpolicy;

	/**/
	Object *ch_showbuttonframe;
	Object *ch_enablepointers;
	Object *ch_showtabtransferanim;
	Object *ch_showsearch;
	Object *ch_showvalidationbuttons;
	Object *ch_showseparators;
	Object *cy_newpagepolicy;
	Object *cy_popuppolicy;
	Object *cy_newpageposition;
	Object *cy_toolbuttontype;
	Object *cy_completiontype;
	Object *ch_newpagebutton;
	Object *cy_closerequester;
	Object *cy_errorreporting;
	Object *cy_middlebuttonpolicy;
	Object *ch_favicon_tab;
	Object *ch_favicon_bookmark;
	Object *ch_favicon_history;
	Object *ch_favicon_historypanel;
	Object *ch_favicon_quicklink;

	Object *cy_quicklinklook;
	Object *cy_quicklinklayout;
	Object *sl_quicklinkrows;

	/**/
	Object *gr_contextmenus;

	/**/
	Object *gr_mimetypes;

	/**/
	Object *str_defaultfontsize;
	Object *str_defaultfixedfontsize;
	Object *str_minimumfontsize;
	Object *str_minimumlogicalfontsize;
	Object *cy_textencoding;
	Object *cy_fontsmoothing;

	Object *pop_sansseriffamily;
	Object *str_sansseriffamily;
	Object *lv_sansseriffamily;

	Object *pop_seriffamily;
	Object *str_seriffamily;
	Object *lv_seriffamily;

	Object *pop_standardfamily;
	Object *str_standardfamily;
	Object *lv_standardfamily;

	Object *pop_cursivefamily;
	Object *str_cursivefamily;
	Object *lv_cursivefamily;

	Object *pop_fantasyfamily;
	Object *str_fantasyfamily;
	Object *lv_fantasyfamily;

	Object *pop_fixedfamily;
	Object *str_fixedfamily;
	Object *lv_fixedfamily;

	/**/
	Object *cy_useragent;
	Object *sl_connections;
	Object *cy_proxytype;
	Object *ch_proxyenabled;
	Object *str_proxyhost;
	Object *str_proxyport;
	Object *str_proxyusername;
	Object *str_proxypassword;

	/**/
	Object *ch_savecookies;
	/*
	Object *cy_temporarycookiespolicy;
	Object *cy_persistantcookiespolicy;
	*/
	Object *cy_cookiespolicy;

	Object *ch_savehistory;
	Object *sl_historyitems;
	Object *sl_historymaxage;
	Object *ch_savesession;
	Object *ch_deletesessionatexit;
	Object *cy_sessionrestore;
	Object *ch_savecredentials;
	Object *ch_formautofill;
	Object *ch_enablelocalstorage;

	/**/
	Object *ch_ignoresslerrors;
	Object *str_certificatepath;

	/**/
	Object *cy_loopfilter;
	Object *ch_partialcontent;
	Object *ch_webm;
	Object *ch_flv;
	Object *ch_ogg;
	Object *ch_mp4;
};

DEFNEW
{
	Object *prefs_save, *prefs_use, *prefs_cancel, *lv_prefs, *gp_prefs;

	Object *str_homepage, *str_startpage, *str_newtabpage, *str_clidevice;
	Object *str_dldir, *ch_downloadautoclose, *ch_savedownloads, *ch_downloadstartautomatically;

	Object *str_defaultfontsize, *str_defaultfixedfontsize, *str_minimumfontsize, *str_minimumlogicalfontsize, *cy_textencoding, *cy_fontsmoothing;
	Object *pop_sansseriffamily, *pop_seriffamily, *pop_standardfamily, *pop_cursivefamily, *pop_fantasyfamily, *pop_fixedfamily;
	Object *str_sansseriffamily, *str_seriffamily, *str_standardfamily, *str_cursivefamily, *str_fantasyfamily, *str_fixedfamily;
	Object *lv_sansseriffamily, *lv_seriffamily, *lv_standardfamily, *lv_cursivefamily, *lv_fantasyfamily, *lv_fixedfamily;

	Object *ch_enablejavascript, *ch_allowjavascriptnewwindow, *ch_enableanimation, /* *ch_enableanimationloop,*/ *ch_enablepagecache, *cy_cachemodel, *ch_enableimages, *ch_shouldprintbackground, *ch_contentblocking, *ch_enableinspector, *ch_enableplugins/*, *cy_animationpolicy*/;

	Object *ch_showbuttonframe, *ch_enablepointers, *ch_showtabtransferanim, *ch_showsearch, *cy_newpagepolicy, *cy_popuppolicy, *cy_newpageposition, *cy_toolbuttontype, *cy_completiontype, *cy_closerequester, *cy_errorreporting, *cy_middlebuttonpolicy, *ch_showvalidationbuttons, *ch_showseparators;
	Object *ch_favicon_tab, *ch_favicon_bookmark, *ch_favicon_history, *ch_favicon_historypanel, *ch_favicon_quicklink;
	Object *cy_quicklinklook, *sl_quicklinkrows, *cy_quicklinklayout;

	Object *gr_contextmenus;

	Object *gr_mimetypes;

	Object *sl_connections, *cy_useragent;
	Object *ch_proxyenabled, *cy_proxytype, *str_proxyhost, *str_proxyport, *str_proxyusername, *str_proxypassword;

	Object /**cy_temporarycookiespolicy, *cy_persistantcookiespolicy,*/*cy_cookiespolicy, *ch_savecookies, *ch_savehistory, *sl_historyitems, *sl_historymaxage, *ch_savesession, *ch_deletesessionatexit, *cy_sessionrestore, *ch_savecredentials, *ch_formautofill, *ch_enablelocalstorage;

	Object *ch_ignoresslerrors, *str_certificatepath;

	Object *cy_loopfilter, *ch_partialcontent, *ch_webm, *ch_flv, *ch_ogg, *ch_mp4;

	cycles_init();

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Window_ID, MAKE_ID('W','P','R','E'),
			MUIA_Window_Title, GSI(MSG_PREFSWINDOW_TITLE),
			MUIA_Window_NoMenus, TRUE,
			WindowContents, VGroup,
				Child, HGroup,
					Child, lv_prefs = ListviewObject,
						MUIA_Listview_List, ListObject,
							InputListFrame,
							MUIA_List_AdjustWidth, TRUE,
							MUIA_List_SourceArray, prefslist,
							MUIA_List_Active, 0,
						End,
					End,
					Child, VGroup,
						Child, gp_prefs = VGroup,
							MUIA_Group_PageMode, TRUE,
							MUIA_Frame, MUIV_Frame_Page,
							MUIA_Background, MUII_PageBack,

						// General
							Child, VGroup,
								Child, HVSpace,
								Child, ColGroup(2),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_GENERAL_HOME_PAGE)),
									Child, str_homepage = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_GENERAL_HOME_PAGE), "about:", 1024, MAKE_ID('S','H','P','U')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_GENERAL_START_PAGE)),
									Child, str_startpage = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_GENERAL_START_PAGE), "about:", 1024, MAKE_ID('S','S','P','U')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_GENERAL_NEW_PAGE)),
									Child, str_newtabpage = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_GENERAL_NEW_PAGE), "topsites://", 1024, MAKE_ID('S','N','T','U')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_GENERAL_OUTPUT_WINDOW)),
									Child, str_clidevice = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_GENERAL_OUTPUT_WINDOW), "CON:0/0/640/240/OWB Output Window/CLOSE/AUTO/WAIT", 1024, MAKE_ID('S','O','C','W')),
								End,
								Child, HVSpace,
							End,

						// Interface
							Child, VGroup,
								Child, HVSpace,
								Child, VGroup,
									GroupFrameT(GSI(MSG_PREFSWINDOW_INTERFACE_GENERAL)),
									Child, ColGroup(2),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_NEW_PAGE_POLICY)),
										Child, cy_newpagepolicy = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_NEW_PAGE_POLICY), newpagepolicies, MAKE_ID('S','I','N','P')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_POPUP_POLICY)),
										Child, cy_popuppolicy = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_POPUP_POLICY), newpagepolicies, MAKE_ID('S','I','P','P')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_NEW_PAGE_POSITION)),
										Child, cy_newpageposition = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_NEW_PAGE_POSITION), newpagepositions, MAKE_ID('S','I','T','P')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_COMPLETION_MODE)),
										Child, cy_completiontype = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_COMPLETION_MODE), completiontypes, MAKE_ID('S','I','C','T')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_CLOSE_REQUESTER)),
										Child, cy_closerequester = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_CLOSE_REQUESTER), closerequesterpolicies, MAKE_ID('S','I','C','R')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_ERROR_REPORTING)),
										Child, cy_errorreporting = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_ERROR_REPORTING), errorreportingmodes, MAKE_ID('S','I','E','R')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_MIDDLEBUTTON_BEHAVIOUR)),
										Child, cy_middlebuttonpolicy = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_MIDDLEBUTTON_BEHAVIOUR), middlebuttonpolicies, MAKE_ID('S','I','M','B')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_NAVIGATION_BUTTON_TYPE)),
										Child, cy_toolbuttontype = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_NAVIGATION_BUTTON_TYPE), toolbuttontypes, MAKE_ID('S','I','B','T')),
									End,
									Child, ColGroup(3),
										Child, ch_showbuttonframe = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_BUTTON_FRAME), TRUE, MAKE_ID('S','I','B','F')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_BUTTON_FRAME)),
										Child, HSpace(0),

										Child, ch_showvalidationbuttons = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_VALIDATION_BUTTONS), FALSE, MAKE_ID('S','I','S','V')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_VALIDATION_BUTTONS)),
										Child, HSpace(0),

										Child, ch_showseparators = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_SEPARATORS), FALSE, MAKE_ID('S','I','S','S')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_SEPARATORS)),
										Child, HSpace(0),

										Child, ch_showsearch = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_SEARCH_BAR), TRUE, MAKE_ID('S','I','S','B')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_SEARCH_BAR)),
										Child, HSpace(0),

										Child, ch_showtabtransferanim = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_TAB_TRANSFERANIM), TRUE, MAKE_ID('S','I','T','T')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_TAB_TRANSFERANIM)),
										Child, HSpace(0),

										Child, ch_enablepointers = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_CONTEXTUAL_POINTERS), TRUE, MAKE_ID('S','I','C','P')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_CONTEXTUAL_POINTERS)),
										Child, HSpace(0),
									End,
								End,

								Child, HSpace(0),

								Child, ColGroup(6),
									GroupFrameT(GSI(MSG_PREFSWINDOW_INTERFACE_FAVICONS)),
									
									Child, ch_favicon_tab = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_TABS), TRUE, MAKE_ID('S','I','F','I')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_TABS)),
									Child, ch_favicon_bookmark = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_BOOKMARKS), TRUE, MAKE_ID('S','I','F','0')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_BOOKMARKS)),
									Child, ch_favicon_history = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_HISTORY), TRUE, MAKE_ID('S','I','F','1')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_HISTORY)),
									
									Child, ch_favicon_historypanel = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_HISTORY_PANEL), TRUE, MAKE_ID('S','I','F','2')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_HISTORY_PANEL)),
									Child, ch_favicon_quicklink = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_QUICKLINKS), TRUE, MAKE_ID('S','I','F','3')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_INTERFACE_SHOW_FAVICONS_QUICKLINKS)),
									Child, HSpace(0),
									Child, HSpace(0),

								End,

								Child, HSpace(0),

								Child, ColGroup(2),
									GroupFrameT(GSI(MSG_PREFSWINDOW_INTERFACE_QUICK_LINKS)),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_QUICK_LINK_LOOK)),
									Child, cy_quicklinklook = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_QUICK_LINK_LOOK), quicklinklooks, MAKE_ID('S','I','Q','A')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_QUICK_LINK_LAYOUT)),
									Child, cy_quicklinklayout = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_INTERFACE_QUICK_LINK_LAYOUT), quicklinklayout, MAKE_ID('S','I','Q','L')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_INTERFACE_MAXIMUM_ROWS)),
									Child, sl_quicklinkrows = (Object *) MakePrefsSlider(GSI(MSG_PREFSWINDOW_INTERFACE_MAXIMUM_ROWS), 1, 8, 2, MAKE_ID('S', 'I','Q','R')),
								End,
								Child, HVSpace,
							End,

						// ContextMenus
							Child, gr_contextmenus = (Object *) NewObject(getcontextmenugroupclass(), NULL, TAG_DONE),

						// Mimetypes
							Child, gr_mimetypes = (Object *) NewObject(getmimetypegroupclass(), NULL, TAG_DONE),

						// Content
							Child, VGroup,

								Child, HVSpace,
							
								Child, ColGroup(3),

									Child, HSpace(0),
									Child, ColGroup(2),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_CONTENT_CACHE_MODEL)),
										Child, cy_cachemodel = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_CONTENT_CACHE_MODEL), cachemodels, MAKE_ID('S','C','C','M')),
										/*
										Child, MakeLabel("Play _Animations:"),
										Child, cy_animationpolicy = (Object *) MakePrefsCycle("Play _Animations:", animationpolicies, MAKE_ID('S','C','P','A')),*/
									    End,
									Child, HSpace(0),

									Child, HSpace(0),
									Child, ColGroup(2),
										Child, ch_enablepagecache = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_PAGE_CACHE), TRUE, MAKE_ID('S','C','P','C')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_PAGE_CACHE)),

										Child, ch_enablejavascript = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_JAVASCRIPT), TRUE, MAKE_ID('S','C','E','J')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_JAVASCRIPT)),

										Child, ch_allowjavascriptnewwindow = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_ALLOW_NEW_WINDOW), TRUE, MAKE_ID('S','C','A','W')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_ALLOW_NEW_WINDOW)),

										Child, ch_enableanimation = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_PLAY_ANIMATIONS), TRUE, MAKE_ID('S','C','P','A')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_PLAY_ANIMATIONS)),
										/*
										Child, ch_enableanimationloop = (Object *) MakePrefsCheck("Play animations in loop:", TRUE, MAKE_ID('S','C','P','L')),
										Child, LLabel("Play Animations in Loop:"),
										*/
										Child, ch_enableimages = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_LOAD_IMAGES), TRUE, MAKE_ID('S','C','L','I')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_LOAD_IMAGES)),

										Child, ch_shouldprintbackground = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_PRINT_BACKGROUNDS), TRUE, MAKE_ID('S','C','P','B')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_PRINT_BACKGROUNDS)),

										Child, ch_contentblocking = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_BLOCKING), FALSE, MAKE_ID('S','C','B','C')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_BLOCKING)),

										Child, ch_enableplugins = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_PLUGINS), TRUE, MAKE_ID('S','C','E','P')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_PLUGINS)),

										Child, ch_enableinspector = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_CONTENT_INSPECTOR), TRUE, MAKE_ID('S','C','E','W')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_CONTENT_INSPECTOR)),

									End,
									Child, HSpace(0),

									End,

								Child, HVSpace,

							End,

						// Fonts
							Child, VGroup,

								Child, HVSpace,

								Child, ColGroup(2),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_DEFAULT_SIZE)),
									Child, str_defaultfontsize = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_FONTS_DEFAULT_SIZE), "16", 1024, MAKE_ID('S','F','S','1')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_DEFAULT_MONOSPACED_SIZE)),
									Child, str_defaultfixedfontsize = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_FONTS_DEFAULT_MONOSPACED_SIZE), "13", 1024, MAKE_ID('S','F','S','2')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_MINIMUM_SIZE)),
									Child, str_minimumfontsize = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_FONTS_MINIMUM_SIZE), "1", 1024, MAKE_ID('S','F','S','3')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_MINIMUM_LOGICAL_SIZE)),
									Child, str_minimumlogicalfontsize = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_FONTS_MINIMUM_LOGICAL_SIZE), "9", 1024, MAKE_ID('S','F','S','4')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_SMOOTHING_METHOD)),
									Child, cy_fontsmoothing = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_FONTS_SMOOTHING_METHOD), smoothingmethods, MAKE_ID('S','F','S','M')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_ENCODING)),
									Child, cy_textencoding = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_FONTS_ENCODING), textencodings, MAKE_ID('S','T','E','1')),

									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_STANDARD_FAMILY)),
									Child, FontFamilyPopString(standardfamily, "Times New Roman", MAKE_ID('S','F','F','1')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_MONOSPACED_FAMILY)),
									Child, FontFamilyPopString(fixedfamily, "Courier New", MAKE_ID('S','F','F','2')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_SERIF_FAMILY)),
									Child, FontFamilyPopString(seriffamily, "Times New Roman", MAKE_ID('S','F','F','3')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_SANS_SERIF_FAMILY)),
									Child, FontFamilyPopString(sansseriffamily, "Arial", MAKE_ID('S','F','F','4')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_CURSIVE_FAMILY)),
									Child, FontFamilyPopString(cursivefamily, "Comic Sans MS", MAKE_ID('S','F','F','5')),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_FONTS_FANTASY_FAMILY)),
									Child, FontFamilyPopString(fantasyfamily, "Comic Sans MS", MAKE_ID('S','F','F','6')),
								End,

								Child, HVSpace,

							End,

						// Downloads
							Child, VGroup,

								Child, HVSpace,

								Child, HGroup,
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_DOWNLOADS_PATH)),
									Child, str_dldir = (Object *) MakeDirString(GSI(MSG_PREFSWINDOW_DOWNLOADS_PATH), "RAM:", MAKE_ID('S', 'D','L','S')),
								End,
								Child, ColGroup(3),
									Child, ch_downloadautoclose = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_DOWNLOADS_AUTOCLOSE), FALSE, MAKE_ID('S','D','A','C')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_DOWNLOADS_AUTOCLOSE)),
									Child, HSpace(0),
									Child, ch_savedownloads = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_DOWNLOADS_SAVEDOWNLOADS), TRUE, MAKE_ID('S','D','S','D')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_DOWNLOADS_SAVEDOWNLOADS)),
									Child, HSpace(0),
									Child, ch_downloadstartautomatically = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_DOWNLOADS_START_AUTOMATICALLY), FALSE, MAKE_ID('S','D','S','A')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_DOWNLOADS_START_AUTOMATICALLY)),
									Child, HSpace(0),
								End,

								Child, HVSpace,

							End,

						// Network
							Child, VGroup,

								Child, HVSpace,

								Child, VGroup,
									Child, ColGroup(2),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_NETWORK_SPOOF)),
										Child, cy_useragent = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_NETWORK_SPOOF), useragents_labels, MAKE_ID('S','N','U','A')),
									End,

                                    Child, ColGroup(2),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_NETWORK_ACTIVE_CONNECTIONS)),
										Child, sl_connections = (Object *) MakePrefsSlider(GSI(MSG_PREFSWINDOW_NETWORK_ACTIVE_CONNECTIONS), 2, 32, 16, MAKE_ID('S', 'N','E','C')),
									End,
								End,

								Child, VGroup,
									GroupFrameT(GSI(MSG_PREFSWINDOW_NETWORK_PROXY)),

									Child, ColGroup(3),
										Child, ch_proxyenabled = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_USE), FALSE, MAKE_ID('S','N','P','E')),
										Child, LLabel(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_USE)),
										Child, HSpace(0),
									End,

                                    Child, ColGroup(2),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_TYPE)),
										Child, cy_proxytype = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_TYPE), proxy_types, MAKE_ID('S','N','P','T')),
									End,

									Child, ColGroup(4),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_HOST)),
										Child, str_proxyhost = (Object *) MakePrefsStringWithWeight(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_HOST), "", 256, 80, MAKE_ID('S','N','P','H')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_PORT)),
										Child, str_proxyport = (Object *) MakePrefsStringWithWeight(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_PORT), "", 6, 20, MAKE_ID('S','N','P','P')),
									End,

									Child, ColGroup(4),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_USERNAME)),
										Child, str_proxyusername = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_USERNAME), "", 128, MAKE_ID('S','N','P','U')),
										Child, MakeLabel(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_PASSWORD)),
										Child, str_proxypassword = (Object *) MakePrefsString(GSI(MSG_PREFSWINDOW_NETWORK_PROXY_PASSWORD), "", 128, MAKE_ID('S','N','P','V')),
									End,

								End,

								Child, HVSpace,

							End,

						// Privacy
							Child, VGroup,

								Child, HVSpace,

								Child, ColGroup(2),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_PRIVACY_SESSION_RESTORATION_MODE)),
									Child, cy_sessionrestore = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_PRIVACY_SESSION_RESTORATION_MODE), restoresessionmodes, MAKE_ID('S','P','R','M')),
								End,
									
								Child, ColGroup(3),
									Child, ch_savesession = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_SESSION), TRUE, MAKE_ID('S','P','S','S')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_SESSION)),
									Child, HSpace(0),
								End,

								Child, ColGroup(3),
									Child, ch_deletesessionatexit = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_PRIVACY_DELETE_SESSION_AT_EXIT), FALSE, MAKE_ID('S','P','D','S')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_DELETE_SESSION_AT_EXIT)),
									Child, HSpace(0),
								End,

								Child, ColGroup(3),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_PRIVACY_COOKIES_POLICY)),
									Child, cy_cookiespolicy = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_PRIVACY_COOKIES_POLICY), cookiepolicies, MAKE_ID('S','P','C','P')),
								End,

								Child, ColGroup(3),
									Child, ch_savecookies = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_COOKIES), TRUE, MAKE_ID('S','P','S','C')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_COOKIES)),
									Child, HSpace(0),
								End,

								/*
								Child, MakeLabel("_Temporary Cookies:"),
								Child, cy_temporarycookiespolicy = (Object *) MakePrefsCycle("_Temporary Cookies:", cookiepolicies, MAKE_ID('S','N','T','C')),
								Child, HSpace(0),
								Child, MakeLabel("_Persistant Cookies:"),
								Child, cy_persistantcookiespolicy = (Object *) MakePrefsCycle("_Persistant Cookies:", cookiepolicies, MAKE_ID('S','N','P','C')),
								Child, HSpace(0),
								*/

								Child, ColGroup(3),
									Child, ch_enablelocalstorage = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_PRIVACY_ENABLE_LOCAL_STORAGE), TRUE, MAKE_ID('S','P','E','L')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_ENABLE_LOCAL_STORAGE)),
									Child, HSpace(0),
								End,

								Child, ColGroup(3),
									Child, ch_savecredentials = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_FORM_CREDENTIALS), TRUE, MAKE_ID('S','P','S','F')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_FORM_CREDENTIALS)),
									Child, HSpace(0),
								End,

								Child, ColGroup(3),
									Child, ch_formautofill = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_PRIVACY_ENABLE_FORM_AUTOFILL), TRUE, MAKE_ID('S','P','A','F')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_ENABLE_FORM_AUTOFILL)),
									Child, HSpace(0),
								End,

								Child, ColGroup(3),
									Child, ch_savehistory = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_HISTORY), TRUE, MAKE_ID('S','P','S','H')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_SAVE_HISTORY)),
									Child, HSpace(0),
								End,

								Child, ColGroup(2),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_PRIVACY_HISTORY_ENTRIES)),
									Child, sl_historyitems = (Object *) MakePrefsSlider(GSI(MSG_PREFSWINDOW_PRIVACY_HISTORY_ENTRIES), 10, 10000, 1000, MAKE_ID('S', 'P','H','M')),

									Child, MakeLabel(GSI(MSG_PREFSWINDOW_PRIVACY_HISTORY_AGE)),
									Child, sl_historymaxage = (Object *) MakePrefsSlider(GSI(MSG_PREFSWINDOW_PRIVACY_HISTORY_AGE), 1, 365, 60, MAKE_ID('S', 'P','H','A')),
								End,

								Child, HVSpace,

							End,

						// Security
							Child, VGroup,

								Child, HVSpace,

								Child, HGroup,
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_SECURITY_CERTIFICATE_PATH)),
									Child, str_certificatepath = (Object *) MakeFileString(GSI(MSG_PREFSWINDOW_SECURITY_CERTIFICATE_PATH), "PROGDIR:curl-ca-bundle.crt", MAKE_ID('S','S','C','P')),
								End,
								Child, ColGroup(3),
									Child, ch_ignoresslerrors = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_SECURITY_IGNORE_SSL_ERRORS), FALSE, MAKE_ID('S','S','I','E')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_SECURITY_IGNORE_SSL_ERRORS)),
									Child, HSpace(0),
								End,

								Child, HVSpace,

							End,

						// Media
							Child, VGroup,

								Child, HVSpace,

								Child, ColGroup(2),
									Child, MakeLabel(GSI(MSG_PREFSWINDOW_MEDIA_LOOPFILTER_MODE)),
									Child, cy_loopfilter = (Object *) MakePrefsCycle(GSI(MSG_PREFSWINDOW_MEDIA_LOOPFILTER_MODE), loopfiltermodes, MAKE_ID('S','M','L','F')),
								End,

								Child, ColGroup(3),
									Child, ch_partialcontent = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_MEDIA_ENABLE_PARTIAL_CONTENT), TRUE, MAKE_ID('S','M','P','C')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_MEDIA_ENABLE_PARTIAL_CONTENT)),
									Child, HSpace(0),
								End,

								Child, ColGroup(3),
									Child, ch_mp4 = (Object *) MakePrefsCheck("_MP4 (H.264) support", TRUE, MAKE_ID('S','M','M','4')),
									Child, LLabel("_MP4 (H.264) support"),
									Child, HSpace(0),
								End,
								Child, ColGroup(3),
									Child, ch_webm = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_MEDIA_USE_WEBM), FALSE, MAKE_ID('S','M','V','8')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_MEDIA_USE_WEBM)),
									Child, HSpace(0),
								End,
								Child, ColGroup(3),
									Child, ch_flv = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_MEDIA_USE_FLV), FALSE, MAKE_ID('S','M','F','L')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_MEDIA_USE_FLV)),
									Child, HSpace(0),
								End,
								Child, ColGroup(3),
									Child, ch_ogg = (Object *) MakePrefsCheck(GSI(MSG_PREFSWINDOW_MEDIA_USE_OGG), FALSE, MAKE_ID('S','M','O','G')),
									Child, LLabel(GSI(MSG_PREFSWINDOW_MEDIA_USE_OGG)),
									Child, HSpace(0),
								End,

								Child, HVSpace,

							End,

						End,
					End,
				End,

				Child, HGroup,
					Child, prefs_save = (Object *) MakeButton(GSI(MSG_PREFSWINDOW_SAVE)),
					Child, RectangleObject, End,
					Child, prefs_use = (Object *) MakeButton(GSI(MSG_PREFSWINDOW_USE)),
					Child, RectangleObject, End,
					Child, prefs_cancel = (Object *) MakeButton(GSI(MSG_PREFSWINDOW_CANCEL)),
				End,
			End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->str_homepage = str_homepage;
		data->str_startpage = str_startpage;
		data->str_newtabpage = str_newtabpage;
		data->str_clidevice = str_clidevice;

		data->str_dldir    = str_dldir;
		data->ch_downloadautoclose = ch_downloadautoclose;
		data->ch_savedownloads = ch_savedownloads;
		data->ch_downloadstartautomatically = ch_downloadstartautomatically;

		data->str_defaultfontsize = str_defaultfontsize;
		data->str_defaultfixedfontsize = str_defaultfixedfontsize;
		data->str_minimumfontsize = str_minimumfontsize;
		data->str_minimumlogicalfontsize = str_minimumlogicalfontsize;
		data->cy_textencoding = cy_textencoding;
		data->cy_fontsmoothing = cy_fontsmoothing;

		data->pop_sansseriffamily = pop_sansseriffamily;
		data->str_sansseriffamily = str_sansseriffamily;
		data->lv_sansseriffamily = lv_sansseriffamily;

		data->pop_seriffamily = pop_seriffamily;
		data->str_seriffamily = str_seriffamily;
		data->lv_seriffamily = lv_seriffamily;

		data->pop_standardfamily = pop_standardfamily;
		data->str_standardfamily = str_standardfamily;
		data->lv_standardfamily = lv_standardfamily;

		data->pop_cursivefamily = pop_cursivefamily;
		data->str_cursivefamily = str_cursivefamily;
		data->lv_cursivefamily = lv_cursivefamily;

		data->pop_fantasyfamily = pop_fantasyfamily;
		data->str_fantasyfamily = str_fantasyfamily;
		data->lv_fantasyfamily = lv_fantasyfamily;

		data->pop_fixedfamily = pop_fixedfamily;
		data->str_fixedfamily = str_fixedfamily;
		data->lv_fixedfamily = lv_fixedfamily;

		data->ch_enablejavascript = ch_enablejavascript;
		data->ch_allowjavascriptnewwindow = ch_allowjavascriptnewwindow;
		data->ch_enableanimation = ch_enableanimation;
		//data->ch_enableanimationloop = ch_enableanimationloop;
		//data->cy_animationpolicy = cy_animationpolicy;
		data->ch_enablepagecache = ch_enablepagecache;
		data->cy_cachemodel = cy_cachemodel;
		data->ch_enableimages = ch_enableimages;
		data->ch_shouldprintbackground = ch_shouldprintbackground;
		data->ch_contentblocking = ch_contentblocking;
		data->ch_enableplugins = ch_enableplugins;
		data->ch_enableinspector = ch_enableinspector;

		data->ch_showbuttonframe = ch_showbuttonframe;
		data->ch_showseparators = ch_showseparators;
		data->ch_enablepointers = ch_enablepointers;
		data->ch_showtabtransferanim = ch_showtabtransferanim;
		data->ch_showsearch = ch_showsearch;
		data->ch_showvalidationbuttons = ch_showvalidationbuttons;
		data->cy_newpagepolicy = cy_newpagepolicy;
		data->cy_popuppolicy = cy_popuppolicy;
		data->cy_newpageposition = cy_newpageposition;
		data->cy_completiontype = cy_completiontype;
		data->cy_toolbuttontype = cy_toolbuttontype;
		data->cy_closerequester = cy_closerequester;
		data->cy_errorreporting = cy_errorreporting;
		data->cy_middlebuttonpolicy = cy_middlebuttonpolicy;
		data->ch_favicon_tab = ch_favicon_tab;
		data->ch_favicon_bookmark = ch_favicon_bookmark;
		data->ch_favicon_history = ch_favicon_history;
		data->ch_favicon_historypanel = ch_favicon_historypanel;
		data->ch_favicon_quicklink = ch_favicon_quicklink;

		data->gr_contextmenus = gr_contextmenus;

		data->gr_mimetypes = gr_mimetypes;

		data->cy_quicklinklook = cy_quicklinklook;
		data->cy_quicklinklayout = cy_quicklinklayout;
		data->sl_quicklinkrows = sl_quicklinkrows;

		data->sl_connections = sl_connections;
		data->cy_useragent = cy_useragent;
		data->ch_proxyenabled = ch_proxyenabled;
		data->cy_proxytype = cy_proxytype;
		data->str_proxyhost = str_proxyhost;
		data->str_proxyport = str_proxyport;
		data->str_proxyusername = str_proxyusername;
		data->str_proxypassword = str_proxypassword;

		/*
		data->cy_temporarycookiespolicy = cy_temporarycookiespolicy;
		data->cy_persistantcookiespolicy = cy_persistantcookiespolicy;
		*/
		data->ch_savecookies = ch_savecookies;
		data->cy_cookiespolicy = cy_cookiespolicy;
		data->ch_enablelocalstorage = ch_enablelocalstorage;
		data->ch_savehistory = ch_savehistory;
		data->sl_historyitems = sl_historyitems;
		data->sl_historymaxage = sl_historymaxage;
		data->ch_savesession = ch_savesession;
		data->ch_deletesessionatexit = ch_deletesessionatexit;
		data->cy_sessionrestore = cy_sessionrestore;
		data->ch_savecredentials = ch_savecredentials;
		data->ch_formautofill = ch_formautofill;

		data->str_certificatepath = str_certificatepath;
		data->ch_ignoresslerrors = ch_ignoresslerrors;

		data->cy_loopfilter = cy_loopfilter;
		data->ch_partialcontent = ch_partialcontent;
		data->ch_webm = ch_webm;
		data->ch_flv  = ch_flv;
		data->ch_ogg  = ch_ogg;
		data->ch_mp4  = ch_mp4;

		/* Set defaults */
		set(data->cy_completiontype, MUIA_Cycle_Active, 2);

		/* Font families notifications */
		DoMethod(data->lv_standardfamily, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->pop_standardfamily, 2, MUIM_Popstring_Close, TRUE);
		DoMethod(data->lv_fixedfamily, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->pop_fixedfamily, 2, MUIM_Popstring_Close, TRUE);
		DoMethod(data->lv_seriffamily, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->pop_seriffamily, 2, MUIM_Popstring_Close, TRUE);
		DoMethod(data->lv_sansseriffamily, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->pop_sansseriffamily, 2, MUIM_Popstring_Close, TRUE);
		DoMethod(data->lv_cursivefamily, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->pop_cursivefamily, 2, MUIM_Popstring_Close, TRUE);
		DoMethod(data->lv_fantasyfamily, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE, data->pop_fantasyfamily, 2, MUIM_Popstring_Close, TRUE);

		/* Prefs actions */

		/* Generic prefs notifications */
		DoMethod(lv_prefs, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, gp_prefs, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application, 1, MM_OWBApp_PrefsCancel);
		DoMethod(prefs_save, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 2, MM_OWBApp_PrefsSave, TRUE);
		DoMethod(prefs_use, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 2, MM_OWBApp_PrefsSave, FALSE);
		DoMethod(prefs_cancel, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 1, MM_OWBApp_PrefsCancel);
	}

	return (IPTR)obj;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_OWB_WindowType:
		{
			*msg->opg_Storage = (ULONG) MV_OWB_Window_Settings;
		}
		return TRUE;

		case MA_OWBApp_DefaultURL:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_homepage, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_StartPage:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_startpage, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_NewTabPage:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_newtabpage, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_CLIDevice:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_clidevice, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_ShowButtonFrame:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_showbuttonframe, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnablePointers:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enablepointers, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_ShowSearchBar:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_showsearch, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_ShowValidationButtons:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_showvalidationbuttons, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_ShowSeparators:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_showseparators, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableTabTransferAnim:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_showtabtransferanim, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_NewPagePolicy:
		{
			*msg->opg_Storage = (ULONG) (getv(data->cy_newpagepolicy, MUIA_Cycle_Active) == 0) ? MV_OWBApp_NewPagePolicy_Tab : MV_OWBApp_NewPagePolicy_Window;
		}
		return TRUE;

		case MA_OWBApp_PopupPolicy:
		{
			*msg->opg_Storage = (ULONG) (getv(data->cy_popuppolicy, MUIA_Cycle_Active) == 0) ? MV_OWBApp_NewPagePolicy_Tab : MV_OWBApp_NewPagePolicy_Window;
		}
		return TRUE;

		case MA_OWBApp_NewPagePosition:
		{
			*msg->opg_Storage = (ULONG) (getv(data->cy_newpageposition, MUIA_Cycle_Active) == 0) ? MV_OWBApp_NewPagePosition_Last : MV_OWBApp_NewPagePosition_After_Active;
		}
		return TRUE;

		case MA_OWBApp_ToolButtonType:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_toolbuttontype, MUIA_Cycle_Active);
		}
		return TRUE;

		case MA_OWBApp_URLCompletionType:
		{
			switch(getv(data->cy_completiontype, MUIA_Cycle_Active))
			{
				default:
				case 0:
					*msg->opg_Storage = MV_OWBApp_URLCompletionType_None;
					break;
				case 1:
					*msg->opg_Storage = MV_OWBApp_URLCompletionType_Autocomplete;
					break;
				case 2:
					*msg->opg_Storage = MV_OWBApp_URLCompletionType_Popup;
					break;
				case 3:
					*msg->opg_Storage = MV_OWBApp_URLCompletionType_Autocomplete | MV_OWBApp_URLCompletionType_Popup;
					break;
			}
		}
		return TRUE;

		case MA_OWBApp_CloseRequester:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_closerequester, MUIA_Cycle_Active);
		}
		return TRUE;

		case MA_OWBApp_ErrorMode:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_errorreporting, MUIA_Cycle_Active) ? MV_OWBApp_ErrorMode_Page : MV_OWBApp_ErrorMode_Requester;
		}
		return TRUE;

		case MA_OWBApp_MiddleButtonPolicy:
		{
			switch(getv(data->cy_middlebuttonpolicy, MUIA_Cycle_Active))
			{
				default:
				case 0:
					*msg->opg_Storage = MV_OWBApp_MiddleButtonPolicy_NewBackgroundTab;
					break;
				case 1:
					*msg->opg_Storage = MV_OWBApp_MiddleButtonPolicy_NewTab;
					break;
				case 2:
					*msg->opg_Storage = MV_OWBApp_MiddleButtonPolicy_NewWindow;
					break;
			}
		}
		return TRUE;

		case MA_OWBApp_ShowFavIcons:
		{
			ULONG value = 0;
			value |= getv(data->ch_favicon_tab, MUIA_Selected) ? MV_OWBApp_ShowFavIcons_Tab : 0;
			value |= getv(data->ch_favicon_bookmark, MUIA_Selected) ? MV_OWBApp_ShowFavIcons_Bookmark : 0;
			value |= getv(data->ch_favicon_history, MUIA_Selected) ? MV_OWBApp_ShowFavIcons_History : 0;
			value |= getv(data->ch_favicon_historypanel, MUIA_Selected) ? MV_OWBApp_ShowFavIcons_HistoryPanel : 0;
			value |= getv(data->ch_favicon_quicklink, MUIA_Selected) ? MV_OWBApp_ShowFavIcons_QuickLink : 0;

			*msg->opg_Storage = value;
		}
		return TRUE;

		case MA_OWBApp_QuickLinkLook:
		{
			switch(getv(data->cy_quicklinklook, MUIA_Cycle_Active))
			{
				default:
				case 0:
					*msg->opg_Storage = MV_QuickLinkGroup_Mode_Button;
					break;
				case 1:
					*msg->opg_Storage = 0;
					break;
			}
		}
		return TRUE;

		case MA_OWBApp_QuickLinkRows:
		{
			*msg->opg_Storage = (ULONG) getv(data->sl_quicklinkrows, MUIA_Slider_Level);
		}
		return TRUE;

		case MA_OWBApp_QuickLinkLayout:
		{
			switch(getv(data->cy_quicklinklayout, MUIA_Cycle_Active))
			{
				default:
				case 0:
                    *msg->opg_Storage = MV_QuickLinkGroup_Mode_Horiz;
					break;
				case 1:
					*msg->opg_Storage = MV_QuickLinkGroup_Mode_Col;
					break;
				case 2:
					*msg->opg_Storage = MV_QuickLinkGroup_Mode_Prop;
					break;	  
			}
		}
		return TRUE;

		case MA_OWBApp_QuickLinkPosition:
		{
			*msg->opg_Storage = 0;
		}
		return TRUE;

		case MA_OWBApp_EnablePageCache:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enablepagecache, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_CacheModel:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_cachemodel, MUIA_Cycle_Active);
		}
		return TRUE;

		case MA_OWBApp_EnableJavaScript:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enablejavascript, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_AllowJavaScriptNewWindow:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_allowjavascriptnewwindow, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableAnimation:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enableanimation, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableAnimationLoop:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enableanimationloop, MUIA_Selected);
		}
		return TRUE;
		/*
		case MA_OWBApp_AnimationPolicy:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_animationpolicy, MUIA_Cycle_Active);
		}
		return TRUE;
		*/
		case MA_OWBApp_LoadImagesAutomatically:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enableimages, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_ShouldPrintBackgrounds:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_shouldprintbackground, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableContentBlocking:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_contentblocking, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnablePlugins:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enableplugins, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableInspector:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enableinspector, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_DownloadDirectory:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_dldir, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_DownloadAutoClose:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_downloadautoclose, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_DownloadSave:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_savedownloads, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_DownloadStartAutomatically:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_downloadstartautomatically, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_DefaultFontSize:
		{
			*msg->opg_Storage = (ULONG) atoi((char *) getv(data->str_defaultfontsize, MUIA_String_Contents));
		}
		return TRUE;

		case MA_OWBApp_DefaultFixedFontSize:
		{
			*msg->opg_Storage = (ULONG) atoi((char *) getv(data->str_defaultfixedfontsize, MUIA_String_Contents));
		}
		return TRUE;

		case MA_OWBApp_MinimumFontSize:
		{
			*msg->opg_Storage = (ULONG) atoi((char *) getv(data->str_minimumfontsize, MUIA_String_Contents));
		}
		return TRUE;

		case MA_OWBApp_MinimumLogicalFontSize:
		{
			*msg->opg_Storage = (ULONG) atoi((char *) getv(data->str_minimumlogicalfontsize, MUIA_String_Contents));
		}
		return TRUE;

		case MA_OWBApp_TextEncoding:
		{
			ULONG i = getv(data->cy_textencoding, MUIA_Cycle_Active);
			*msg->opg_Storage = (ULONG) textencodings[i];
		}
		return TRUE;

		case MA_OWBApp_SmoothingType:
		{
			*msg->opg_Storage = getv(data->cy_fontsmoothing, MUIA_Cycle_Active) == 0 ? (ULONG) FontSmoothingTypeMedium : (ULONG) FontSmoothingTypeWindows;
		}
		return TRUE;

		case MA_OWBApp_SansSerifFontFamily:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_sansseriffamily, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_SerifFontFamily:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_seriffamily, MUIA_String_Contents);
		}
		return TRUE;
		
		case MA_OWBApp_StandardFontFamily:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_standardfamily, MUIA_String_Contents);
		}
		return TRUE;
		
		case MA_OWBApp_CursiveFontFamily:
		{			 
			*msg->opg_Storage = (ULONG) getv(data->str_cursivefamily, MUIA_String_Contents);
		}
		return TRUE;
		
		case MA_OWBApp_FantasyFontFamily:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_fantasyfamily, MUIA_String_Contents);
		}
		return TRUE;
		
		case MA_OWBApp_FixedFontFamily:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_fixedfamily, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_ActiveConnections:
		{
			*msg->opg_Storage = (ULONG) getv(data->sl_connections,  MUIA_Slider_Level);
		}
		return TRUE;

		case MA_OWBApp_UserAgent:
		{
			*msg->opg_Storage = (ULONG) useragents_strings[getv(data->cy_useragent, MUIA_Cycle_Active)];
		}
		return TRUE;

		case MA_OWBApp_ProxyEnabled:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_proxyenabled, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_ProxyType:
		{
			*msg->opg_Storage = (ULONG) proxy_values[getv(data->cy_proxytype, MUIA_Cycle_Active)];
		}
		return TRUE;

		case MA_OWBApp_ProxyHost:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_proxyhost, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_ProxyPort:
		{
			*msg->opg_Storage = (ULONG) atoi((char *) getv(data->str_proxyport, MUIA_String_Contents));
		}
		return TRUE;

		case MA_OWBApp_ProxyUsername:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_proxyusername, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_ProxyPassword:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_proxypassword, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_SaveSession:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_savesession, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_DeleteSessionAtExit:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_deletesessionatexit, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_SessionRestoreMode:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_sessionrestore, MUIA_Cycle_Active);
		}
		return TRUE;

		/*
		case MA_OWBApp_PersistantCookiesPolicy:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_persistantcookiespolicy, MUIA_Cycle_Active);
		}
		return TRUE;

		case MA_OWBApp_TemporaryCookiesPolicy:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_temporarycookiespolicy, MUIA_Cycle_Active);
		}
		return TRUE;
		*/

		case MA_OWBApp_SaveCookies:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_savecookies, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_CookiesPolicy:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_cookiespolicy, MUIA_Cycle_Active);
		}
		return TRUE;

		case MA_OWBApp_EnableLocalStorage:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_enablelocalstorage, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_SaveHistory:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_savehistory, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_HistoryItemLimit:
		{
			*msg->opg_Storage = (ULONG) getv(data->sl_historyitems, MUIA_Slider_Level);
		}
		return TRUE;

		case MA_OWBApp_HistoryAgeInDaysLimit:
		{
			*msg->opg_Storage = (ULONG) getv(data->sl_historymaxage, MUIA_Slider_Level);
		}
		return TRUE;

		case MA_OWBApp_SaveFormCredentials:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_savecredentials, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableFormAutofill:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_formautofill, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_CertificatePath:
		{
			*msg->opg_Storage = (ULONG) getv(data->str_certificatepath, MUIA_String_Contents);
		}
		return TRUE;

		case MA_OWBApp_IgnoreSSLErrors:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_ignoresslerrors, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_LoopFilterMode:
		{
			*msg->opg_Storage = (ULONG) getv(data->cy_loopfilter, MUIA_Cycle_Active);
		}
		return TRUE;

		case MA_OWBApp_EnableVP8:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_webm, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableFLV:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_flv, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableOgg:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_ogg, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnableMP4:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_mp4, MUIA_Selected);
		}
		return TRUE;

		case MA_OWBApp_EnablePartialContent:
		{
			*msg->opg_Storage = (ULONG) getv(data->ch_partialcontent, MUIA_Selected);
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFTMETHOD(PrefsWindow_Fill)
{
	GETDATA;
	APTR n;

	/* Fill pop strings */
	ITERATELIST(n, &family_list)
	{
		struct familynode *fn = (struct familynode *)n;

		DoMethod(data->lv_seriffamily, MUIM_List_InsertSingle, fn->family, MUIV_List_Insert_Sorted);
		DoMethod(data->lv_sansseriffamily, MUIM_List_InsertSingle, fn->family, MUIV_List_Insert_Sorted);
		DoMethod(data->lv_standardfamily, MUIM_List_InsertSingle, fn->family, MUIV_List_Insert_Sorted);
		DoMethod(data->lv_cursivefamily, MUIM_List_InsertSingle, fn->family, MUIV_List_Insert_Sorted);
		DoMethod(data->lv_fantasyfamily, MUIM_List_InsertSingle, fn->family, MUIV_List_Insert_Sorted);
		DoMethod(data->lv_fixedfamily, MUIM_List_InsertSingle, fn->family, MUIV_List_Insert_Sorted);
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECGET
DECTMETHOD(PrefsWindow_Fill)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, prefswindowclass)
