/*
 * Copyright 2010 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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
#include "FileIOLinux.h"
#include "Page.h"
#include <yarr/RegularExpression.h>
#include "Settings.h"
#include "CookieManager.h"
#include "Api/WebView.h"
#include "Api/WebPreferences.h"

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "gui.h"
#include "utils.h"

#ifndef get
#define get(obj,attr,store) GetAttr(attr,obj,(ULONG *)store)
#endif

extern CONST_STRPTR * get_user_agent_strings();
extern CONST_STRPTR * get_user_agent_labels();

// Reuse prefswindow cookie labels
#define LABEL(x) (STRPTR)MSG_PREFSWINDOW_##x
STATIC CONST CONST_STRPTR cookiepolicies[] =
{
	LABEL(COOKIE_ACCEPT),
	LABEL(COOKIE_REJECT),
//	  LABEL(COOKIE_ASK),
	NULL
};

using namespace WebCore;

struct urlsettingnode* urlsetting_create(char *urlpattern, ULONG javascript, ULONG images, ULONG plugins, ULONG useragent, ULONG cookiepolicy, char *cookiefilter, ULONG localstorage)
{
	struct urlsettingnode *un = (struct urlsettingnode *) malloc(sizeof(*un));

	if(un)
	{
		un->urlpattern          = strdup(urlpattern);
		un->settings.javascript = javascript;
		un->settings.images     = images;
		un->settings.plugins    = plugins;
		//un->settings.animations = animations;
		un->settings.useragent  = useragent;
		un->settings.cookiepolicy = cookiepolicy;
		un->settings.cookiefilter = strdup(cookiefilter);
		un->settings.localstorage = localstorage;

		ADDTAIL(&urlsetting_list, un);
	}

	return un;
}

void urlsetting_delete(struct urlsettingnode *un)
{
	REMOVE(un);
	free(un->settings.cookiefilter);
	free(un->urlpattern);
	free(un);
}

static void cycles_init(void)
{
	APTR arrays[] = { (APTR) cookiepolicies,
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

struct Data
{
	Object *lv_url;
	Object *bt_add;
	Object *bt_remove;
	Object *gr_settings;

	Object *st_url;
	Object *ch_enable_javascript;
	Object *ch_load_images;
	Object *ch_load_plugins;
//	  Object *ch_play_animations;
	Object *cy_user_agent;
	Object *cy_cookie_policy;
	Object *st_cookie_filter;
	Object *ch_localstorage;
};

DEFNEW
{
	Object *lv_url, *bt_add, *bt_remove, *st_url, *gr_settings,
		   *ch_enable_javascript, *ch_load_images, *ch_load_plugins,
			*cy_cookie_policy, *st_cookie_filter, /**ch_play_animations, */*cy_user_agent, *ch_localstorage;

	cycles_init();

	obj = (Object *) DoSuperNew(cl, obj,
				Child, lv_url = (Object *) NewObject(geturlprefslistclass(), NULL, TAG_DONE),
				Child, HGroup,
						Child, bt_add = (Object *) MakeButton(GSI(MSG_URLPREFSGROUP_ADD)),
						Child, bt_remove = (Object *) MakeButton(GSI(MSG_URLPREFSGROUP_REMOVE)),
						End,

				Child, gr_settings = VGroup,

					Child, st_url = (Object *) MakeString("", FALSE),

					Child, ColGroup(3),
								Child, ch_enable_javascript = (Object *) MakeCheck(GSI(MSG_URLPREFSGROUP_JAVASCRIPT), TRUE),
								Child, LLabel(GSI(MSG_URLPREFSGROUP_JAVASCRIPT)),
								Child, HSpace(0),

								Child, ch_load_images = (Object *) MakeCheck(GSI(MSG_URLPREFSGROUP_LOAD_IMAGES), TRUE),
								Child, LLabel(GSI(MSG_URLPREFSGROUP_LOAD_IMAGES)),
								Child, HSpace(0),

								Child, ch_load_plugins = (Object *) MakeCheck(GSI(MSG_URLPREFSGROUP_PLUGINS), TRUE),
								Child, LLabel(GSI(MSG_URLPREFSGROUP_PLUGINS)),
								Child, HSpace(0),

								Child, ch_localstorage = (Object *) MakeCheck(GSI(MSG_PREFSWINDOW_PRIVACY_ENABLE_LOCAL_STORAGE), TRUE),
								Child, LLabel(GSI(MSG_PREFSWINDOW_PRIVACY_ENABLE_LOCAL_STORAGE)),
								Child, HSpace(0),
								End,

					Child, ColGroup(3),
								Child, MakeLabel(GSI(MSG_URLPREFSGROUP_SPOOF)),
								Child, cy_user_agent = (Object *) MakeCycle(GSI(MSG_URLPREFSGROUP_SPOOF), get_user_agent_labels()),
								Child, HSpace(0),
							    End,

					Child, ColGroup(4),
								Child, MakeLabel(GSI(MSG_URLPREFSGROUP_COOKIE_POLICY)),
								Child, cy_cookie_policy = (Object *) MakeCycle(GSI(MSG_URLPREFSGROUP_COOKIE_POLICY), cookiepolicies),
								Child, MakeLabel(GSI(MSG_URLPREFSGROUP_COOKIE_FILTER)),
								Child, st_cookie_filter = (Object *) MakeString(".*", FALSE),
							    End,

					End,
			TAG_MORE, msg->ops_AttrList);

	if (obj)
	{
		GETDATA;

		data->bt_remove = bt_remove;
		data->bt_add = bt_add;
		data->lv_url = lv_url;
		data->st_url = st_url;
		data->gr_settings = gr_settings;
		data->ch_enable_javascript = ch_enable_javascript;
		data->ch_load_images       = ch_load_images;
		data->ch_load_plugins      = ch_load_plugins;
		data->cy_user_agent        = cy_user_agent;
		data->cy_cookie_policy     = cy_cookie_policy;
		data->st_cookie_filter     = st_cookie_filter;
		data->ch_localstorage      = ch_localstorage;

		set(bt_remove,   MUIA_Disabled, TRUE);
		set(gr_settings, MUIA_Disabled, TRUE);

		DoMethod(bt_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_URLPrefsGroup_Add);
		DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_URLPrefsGroup_Remove);

		DoMethod(lv_url,    MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Refresh);

		DoMethod(st_url,               MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
		DoMethod(ch_enable_javascript, MUIM_Notify, MUIA_Selected,        MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
		DoMethod(ch_load_images,       MUIM_Notify, MUIA_Selected,        MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
		DoMethod(ch_load_plugins,      MUIM_Notify, MUIA_Selected,        MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
		DoMethod(cy_user_agent,        MUIM_Notify, MUIA_Cycle_Active,    MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
		DoMethod(cy_cookie_policy,     MUIM_Notify, MUIA_Cycle_Active,    MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
		DoMethod(st_cookie_filter,     MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
		DoMethod(ch_localstorage,      MUIM_Notify, MUIA_Selected,        MUIV_EveryTime, obj, 1, MM_URLPrefsGroup_Change);
	}

	return (IPTR)obj;
}

DEFDISP
{
	GETDATA;
	APTR n, m;

	DoMethod(data->lv_url, MUIM_List_Clear);

	ITERATELISTSAFE(n, m, &urlsetting_list)
	{
		urlsetting_delete((struct urlsettingnode *) n);
	}

	return DOSUPER;
}

DEFTMETHOD(URLPrefsGroup_Refresh)
{
	GETDATA;
	struct urlsettingnode *un;

	DoMethod(data->lv_url, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct urlsettingnode *) &un);

	if(un)
	{
		set(data->bt_remove,   MUIA_Disabled, FALSE);
		set(data->gr_settings, MUIA_Disabled, FALSE);

		nnset(data->st_url, MUIA_String_Contents, un->urlpattern);
		nnset(data->ch_enable_javascript, MUIA_Selected, un->settings.javascript);
		nnset(data->ch_load_images, MUIA_Selected, un->settings.images);
		nnset(data->ch_load_plugins, MUIA_Selected, un->settings.plugins);
		nnset(data->cy_user_agent, MUIA_Cycle_Active, un->settings.useragent);
		nnset(data->cy_cookie_policy, MUIA_Cycle_Active, un->settings.cookiepolicy);
		set(data->st_cookie_filter, MUIA_Disabled, un->settings.cookiepolicy  == 0);
		nnset(data->st_cookie_filter, MUIA_String_Contents, un->settings.cookiefilter);
		nnset(data->ch_localstorage, MUIA_Selected, un->settings.localstorage);
	}
	else
	{
		set(data->bt_remove,   MUIA_Disabled, TRUE);
		set(data->gr_settings, MUIA_Disabled, TRUE);
	}

	return 0;
}

DEFTMETHOD(URLPrefsGroup_Change)
{
	GETDATA;
	struct urlsettingnode *un;

	DoMethod(data->lv_url, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct urlsettingnode *) &un);

	if(un)
	{
		free(un->urlpattern);
		un->urlpattern = strdup((char *) getv(data->st_url, MUIA_String_Contents));

		free(un->settings.cookiefilter);
		un->settings.cookiefilter = strdup((char *) getv(data->st_cookie_filter, MUIA_String_Contents));
		
		un->settings.javascript      = getv(data->ch_enable_javascript, MUIA_Selected);
		un->settings.images          = getv(data->ch_load_images, MUIA_Selected);
		un->settings.plugins         = getv(data->ch_load_plugins, MUIA_Selected);
		un->settings.useragent       = (int) getv(data->cy_user_agent, MUIA_Cycle_Active);
		un->settings.cookiepolicy    = (int) getv(data->cy_cookie_policy, MUIA_Cycle_Active);
		set(data->st_cookie_filter, MUIA_Disabled, un->settings.cookiepolicy  == 0);
		un->settings.localstorage    = getv(data->ch_localstorage, MUIA_Selected);

		DoMethod(data->lv_url, MUIM_List_Redraw, MUIV_List_Redraw_Entry, un);

		DoMethod(obj, MM_URLPrefsGroup_Save);
	}

	return 0;
}

DEFTMETHOD(URLPrefsGroup_Add)
{
	GETDATA;
	struct urlsettingnode *un;

	un = urlsetting_create("http://", TRUE, TRUE, TRUE, 0, 0, ".*", TRUE); // XXX: Should use default settings

	if(un)
	{
		DoMethod(data->lv_url, MUIM_List_InsertSingle, un, MUIV_List_Insert_Bottom);
		set(data->lv_url, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(_win(obj), MUIA_Window_ActiveObject, data->st_url);

		DoMethod(obj, MM_URLPrefsGroup_Save);
	}

	return 0;
}

DEFTMETHOD(URLPrefsGroup_Remove)
{
	GETDATA;
	struct urlsettingnode *un;
	DoMethod(data->lv_url, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (struct urlsettingnode *) &un);

	if(un)
	{
		DoMethod(data->lv_url, MUIM_List_Remove, MUIV_List_Remove_Active);
		urlsetting_delete(un);

		DoMethod(obj, MM_URLPrefsGroup_Save);
	}

	return 0;
}

DEFTMETHOD(URLPrefsGroup_Load)
{
	GETDATA;

	NEWLIST(&urlsetting_list);

	OWBFile *urlFile = new OWBFile("PROGDIR:Conf/url.prefs");

	if (!urlFile)
		return 0;

	if (urlFile->open('r') == -1)
	{
		delete urlFile;
		return 0;
    }

	char *buffer = urlFile->read(urlFile->getSize());
	String fileBuffer = buffer;
	delete [] buffer;
	urlFile->close();
	delete urlFile;

	Vector<String> urlsettings;
	fileBuffer.split("\n", true, urlsettings);

	set(data->lv_url, MUIA_List_Quiet, TRUE);

	for(size_t i = 0; i < urlsettings.size(); i++)
	{
		Vector<String> urlsettingsAttributes;
		urlsettings[i].split("\1", true, urlsettingsAttributes);

		if(urlsettingsAttributes.size() >= 5)
		{
			ULONG cookiepolicy = 0;
			String cookiefilter = ".*";
			ULONG localstorage = TRUE;

			if(urlsettingsAttributes.size() >= 7) // We added cookies later, hence this additional check
			{
				cookiepolicy = atoi(urlsettingsAttributes[5].latin1().data());
				cookiefilter = urlsettingsAttributes[6];
			}

			if(urlsettingsAttributes.size() >= 8) // We added local storage, hence this additional check
			{
				localstorage = urlsettingsAttributes[7] != "0";
			}

			urlsettingnode *un = urlsetting_create((char *) urlsettingsAttributes[0].latin1().data(),
												   urlsettingsAttributes[1] != "0",
												   urlsettingsAttributes[2] != "0",
												   urlsettingsAttributes[3] != "0",
												   atoi(urlsettingsAttributes[4].latin1().data()),
												   cookiepolicy,
												   (char *) cookiefilter.latin1().data(),
												   localstorage);

			if(un)
			{
				DoMethod(data->lv_url, MUIM_List_InsertSingle, un, MUIV_List_Insert_Bottom);
			}
		}
	}

	set(data->lv_url, MUIA_List_Quiet, FALSE);

	return 0;
}

DEFTMETHOD(URLPrefsGroup_Save)
{
	GETDATA;
	int i = 0;
	struct urlsettingnode * un;

	OWBFile *urlFile = new OWBFile("PROGDIR:Conf/url.prefs");
	if(!urlFile)
		return 0;

	if (urlFile->open('w') == -1)
	{
		delete urlFile;
		return 0;
    }

	do
	{
		DoMethod(data->lv_url, MUIM_List_GetEntry, i, (struct urlsettingnode *) &un);

		if (un)
		{
			urlFile->write(String::format("%s\1%lu\1%lu\1%lu\1%lu\1%lu\1%s\1%lu\n",
						   un->urlpattern,
						   (unsigned long)un->settings.javascript,
						   (unsigned long)un->settings.images,
						   (unsigned long)un->settings.plugins,
						   (unsigned long)un->settings.useragent,
						   (unsigned long)un->settings.cookiepolicy,
						   un->settings.cookiefilter,
						   (unsigned long)un->settings.localstorage));
		}

		i++;
	}
	while (un);

	urlFile->close();
	delete urlFile;

	return 0;
}

DEFSMETHOD(URLPrefsGroup_ApplySettingsForURL)
{
	APTR n, m;
	WebView *webView = (WebView *) msg->webView;
	BalWidget *widget = webView->viewWindow();

	//kprintf("ApplySettingForURL <%s>\n", msg->url);

	if(webView && widget)
	{
		bool enabled;

		// We must reset to default settings first for non-overridden settings

		// JavaScript
		if(getv(widget->browser, MA_OWBBrowser_JavaScriptEnabled) == JAVASCRIPT_DEFAULT)
		{
			enabled = webView->preferences()->isJavaScriptEnabled();
			webView->page()->settings().setScriptEnabled(enabled);
		}

		// Images
		if(getv(widget->browser, MA_OWBBrowser_LoadImagesAutomatically) == IMAGES_DEFAULT)
		{
			enabled = webView->preferences()->loadsImagesAutomatically();
            webView->page()->settings().setLoadsImagesAutomatically(enabled);
		}

		// Plugins
		if(getv(widget->browser, MA_OWBBrowser_PluginsEnabled) == PLUGINS_DEFAULT)
		{
			enabled = webView->preferences()->arePlugInsEnabled();
            webView->page()->settings().setPluginsEnabled(enabled);
		}

		// User agent
		if((int) getv(widget->browser, MA_OWBBrowser_UserAgent) == USERAGENT_DEFAULT)
		{
			webView->setCustomUserAgent("");
		}

		enabled = webView->preferences()->localStorageEnabled();
		webView->page()->settings().setLocalStorageEnabled(enabled);

		// Search for URL settings
		ITERATELISTSAFE(n, m, &urlsetting_list)
		{
			struct urlsettingnode *un = (struct urlsettingnode *) n;

			JSC::Yarr::RegularExpression re(un->urlpattern, TextCaseInsensitive);

			if(re.match(msg->url) >=0)
			{
				//kprintf("ApplySettingForURL pattern <%s> matches\n", un->urlpattern);

				// Apply URL settings
				webView->page()->settings().setScriptEnabled(un->settings.javascript != FALSE);
				webView->page()->settings().setLoadsImagesAutomatically(un->settings.images != FALSE);
				webView->page()->settings().setPluginsEnabled(un->settings.plugins != FALSE);
				webView->page()->settings().setLocalStorageEnabled(un->settings.localstorage != FALSE);
				webView->setCustomUserAgent(get_user_agent_strings()[un->settings.useragent]);
				break;
			}
		}

		// But still honour overridden settings if they're not set to default settings

		// JavaScript
		if(getv(widget->browser, MA_OWBBrowser_JavaScriptEnabled) != JAVASCRIPT_DEFAULT)
		{
			webView->page()->settings().setScriptEnabled(getv(widget->browser, MA_OWBBrowser_JavaScriptEnabled) == JAVASCRIPT_ENABLED);
		}

		// Images
		if(getv(widget->browser, MA_OWBBrowser_LoadImagesAutomatically) != IMAGES_DEFAULT)
		{
            webView->page()->settings().setLoadsImagesAutomatically(getv(widget->browser, MA_OWBBrowser_LoadImagesAutomatically) == IMAGES_ENABLED);
		}

		// Plugins
		if(getv(widget->browser, MA_OWBBrowser_PluginsEnabled) != PLUGINS_DEFAULT)
		{
            webView->page()->settings().setPluginsEnabled(getv(widget->browser, MA_OWBBrowser_PluginsEnabled) == PLUGINS_ENABLED);
		}

		// UserAgent
		if((int) getv(widget->browser, MA_OWBBrowser_UserAgent) != USERAGENT_DEFAULT)
		{
			int index = getv(widget->browser, MA_OWBBrowser_UserAgent);
			webView->setCustomUserAgent(get_user_agent_strings()[index]);
		}
	}

	return 0;
}

DEFSMETHOD(URLPrefsGroup_MatchesURL)
{
	ULONG match = FALSE;
	APTR n, m;

	ITERATELISTSAFE(n, m, &urlsetting_list)
	{
		struct urlsettingnode *un = (struct urlsettingnode *) n;

		JSC::Yarr::RegularExpression re(un->urlpattern, TextCaseInsensitive);

		if(re.match(msg->url) >=0)
		{
			match = TRUE;
			break;
		}
	}

	return match;
}

DEFSMETHOD(URLPrefsGroup_UserAgentForURL)
{
	APTR n, m;

	ITERATELISTSAFE(n, m, &urlsetting_list)
	{
		struct urlsettingnode *un = (struct urlsettingnode *) n;

		JSC::Yarr::RegularExpression re(un->urlpattern, TextCaseInsensitive);

		if(re.match(msg->url) >=0)
		{
			return (ULONG) get_user_agent_strings()[un->settings.useragent];
		}
	}

	return NULL;
}

DEFSMETHOD(URLPrefsGroup_CookiePolicyForURLAndName)
{
	APTR n, m;
	CookieStorageAcceptPolicy policy = CookieStorageAcceptPolicyAlways;

	switch(getv(app, MA_OWBApp_CookiesPolicy))
	{
		default:
		case 0:
			policy = CookieStorageAcceptPolicyAlways;
			break;
		case 1:
			policy = CookieStorageAcceptPolicyNever;
			break;
	}

	ITERATELISTSAFE(n, m, &urlsetting_list)
	{
		struct urlsettingnode *un = (struct urlsettingnode *) n;

		JSC::Yarr::RegularExpression re_url(un->urlpattern, TextCaseInsensitive);

		if(re_url.match(msg->url) >= 0)
		{
			JSC::Yarr::RegularExpression re_name(un->settings.cookiefilter, TextCaseInsensitive);

			if(re_name.match(msg->name) >= 0)
			{
				switch(un->settings.cookiepolicy)
				{
					default:
					case 0:
						policy = CookieStorageAcceptPolicyAlways;
						break;
					case 1:
						policy = CookieStorageAcceptPolicyNever;
						break;
				}
			}
			break;
		}
	}

	return (ULONG) policy;
}

BEGINMTABLE
DECNEW
DECDISP
DECTMETHOD(URLPrefsGroup_Load)
DECTMETHOD(URLPrefsGroup_Save)
DECTMETHOD(URLPrefsGroup_Add)
DECTMETHOD(URLPrefsGroup_Remove)
DECTMETHOD(URLPrefsGroup_Change)
DECTMETHOD(URLPrefsGroup_Refresh)
DECSMETHOD(URLPrefsGroup_ApplySettingsForURL)
DECSMETHOD(URLPrefsGroup_MatchesURL)
DECSMETHOD(URLPrefsGroup_UserAgentForURL)
DECSMETHOD(URLPrefsGroup_CookiePolicyForURLAndName)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, urlprefsgroupclass)
