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
#include <wtf/Vector.h>
#include "FileIOLinux.h"
#include "Page.h"
#include "UserContentController.h"
#include "UserContentURLPattern.h"
#include "UserScript.h"
#include "WebScriptWorld.h"
#include "WebView.h"
#include "JSDOMWindow.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <proto/asl.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include <clib/macros.h>

#include "gui.h"
#include "asl.h"
#include "ScriptEntry.h"

#define D(x)

using namespace WebCore;

struct Data
{
	Object *ch_script_enable;
	Object *bt_script_add;
	Object *bt_script_remove;
	Object *lv_scripts;
	Object *lv_whitelist;
	Object *st_whitelist_url;
	Object *lv_blacklist;
	Object *st_blacklist_url;
	Object *bt_whitelist_add;
	Object *bt_whitelist_remove;
	Object *bt_blacklist_add;
	Object *bt_blacklist_remove;
	Object *gr_script;
	Object *txt_title;
	Object *txt_path;
	Object *txt_description;
};

static Vector<ScriptEntry *> scripts_list;

static bool parse_script(ScriptEntry *entry)
{
	bool res = false;
	OWBFile f(entry->path);

	if(f.open('r') != -1)
	{
		char *buffer = f.read(f.getSize());

		if(buffer)
		{
			String fileContent = buffer;
			delete [] buffer;
			Vector<String> lines;
			fileContent.split("\n", true, lines);
			bool inMetaData = false;

			for(size_t i = 0; i < lines.size(); i++)
			{    
				if(!inMetaData)
				{
					if(lines[i].find("==UserScript==") != notFound)
					{
						inMetaData = true;
					}
				}
				else
				{
					size_t pos;

					if(lines[i].find("==/UserScript==") != notFound)
					{
						inMetaData = false;
					}
					else if((pos = lines[i].find("@name")) != notFound && (lines[i].find("@namespace") == notFound)) // Additional space/tab hack to avoid matching @namespace
					{
						entry->title = lines[i].substring(pos + 1 + strlen("@name")).stripWhiteSpace();
					}
					else if((pos = lines[i].find("@description")) != notFound)
					{
						if(entry->description.length() == 0) // Just the first line (we should use multiline)
						{
							entry->description = lines[i].substring(pos + 1 + strlen("@decription")).stripWhiteSpace();
						}
					}
					else if((pos = lines[i].find("@include")) != notFound)
					{
						entry->whitelist.append(lines[i].substring(pos + 1 + strlen("@include")).stripWhiteSpace());
					}
					else if((pos = lines[i].find("@exclude")) != notFound)
					{
						entry->blacklist.append(lines[i].substring(pos + 1 + strlen("@exclude")).stripWhiteSpace());
					}
				}
			}

			if(entry->title.isEmpty()) entry->title = "No name";
			if(entry->description.isEmpty()) entry->description = "No description";

			res = true;
		}

		f.close();
	}

	return res;
}

static void load_scripts(Object *obj, struct Data *data)
{
	OWBFile *scriptFile = new OWBFile("PROGDIR:Conf/userscripts.prefs");

	if (!scriptFile)
		return;

	if (scriptFile->open('r') == -1)
	{
		delete scriptFile;
		return;
    }

	char *buffer = scriptFile->read(scriptFile->getSize());
	String fileBuffer = buffer;
	delete [] buffer;
	scriptFile->close();
	delete scriptFile;

	Vector<String> scripts;
	fileBuffer.split("\n", true, scripts);
	for(size_t i = 0; i < scripts.size(); i++)
	{
		Vector<String> scriptAttributes;
		scripts[i].split("\1", true, scriptAttributes);

		if(scriptAttributes.size() == 4)
		{
			ScriptEntry *script = new ScriptEntry;

			if(script)
			{
				script->path    = scriptAttributes[0];
				script->enabled = scriptAttributes[1] == "1";

				if(parse_script(script))
				{
					if(scriptAttributes[2].length())
					{
						scriptAttributes[2].split("\2", true, script->whitelist);
					}

					if(scriptAttributes[3].length())
					{
						scriptAttributes[3].split("\2", true, script->blacklist);
					}

					scripts_list.append(script);
					DoMethod(data->lv_scripts, MUIM_List_InsertSingle, script, MUIV_List_Insert_Bottom);
				}
			}
		}
	}
}

void save_scripts()
{
	OWBFile *scriptFile = new OWBFile("PROGDIR:Conf/userscripts.prefs");
	if(!scriptFile)
		return;

	if (scriptFile->open('w') == -1)
	{
		delete scriptFile;
		return;
    }

	for(size_t i = 0; i < scripts_list.size(); i++)
	{
		String whitelisthosts = "", blacklisthosts = "";
		const char *enabled = scripts_list[i]->enabled ? "1" : "0";

		for(size_t j = 0; j < scripts_list[i]->whitelist.size(); j++)
		{
		        whitelisthosts.append(scripts_list[i]->whitelist[j]);

			if(j < scripts_list[i]->whitelist.size() - 1)
			{
			        whitelisthosts.append("\2");
			}
		}

		for(size_t j = 0; j < scripts_list[i]->blacklist.size(); j++)
		{
		        blacklisthosts.append(scripts_list[i]->blacklist[j]);

			if(j < scripts_list[i]->blacklist.size() - 1)
			{
			        blacklisthosts.append("\2");
			}
		}

		scriptFile->write(String::format("%s\1%s\1%s\1%s\n", scripts_list[i]->path.latin1().data(), enabled, whitelisthosts.latin1().data(), blacklisthosts.latin1().data()));
	}

	scriptFile->close();
	delete scriptFile;
}

DEFNEW
{
	Object *lv_scripts, *lv_whitelist, *lv_blacklist, *gr_script, *txt_title, *txt_description, *txt_path, *bt_script_add, *bt_script_remove, *ch_script_enable,
		   *bt_whitelist_add, *bt_whitelist_remove, *st_whitelist_url, *bt_blacklist_add, *bt_blacklist_remove, *st_blacklist_url;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Group_Horiz, TRUE,

		Child, VGroup, MUIA_Weight, 30,
			Child, lv_scripts = (Object *) NewObject(getscriptmanagerlistclass(), NULL, TAG_DONE),
			Child, HGroup,
				Child, ch_script_enable = (Object *) MakeCheck(GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_ENABLE), FALSE),
				Child, LLabel(GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_ENABLE)),
				Child, bt_script_add    = (Object *) MakeButton(GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_ADD)),
				Child, bt_script_remove = (Object *) MakeButton(GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_REMOVE)),
				End,
			End,

		Child, gr_script = VGroup, MUIA_Weight, 70,
			Child, ColGroup(2),
				Child, Label(GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_NAME)),
				Child, txt_title = TextObject,
									MUIA_Text_Contents, "",
									MUIA_Text_SetMin, FALSE,
									End,

				Child, Label(GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_PATH)),
				Child, txt_path = TextObject,
									MUIA_Text_Contents, "",
									MUIA_Text_SetMin, FALSE,
									End,

				Child, Label(GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_DESCRIPTION)),
				Child, txt_description = TextObject,
									MUIA_Text_Contents, "",
									MUIA_Text_SetMin, FALSE,
									End,
				End,

			Child, HGroup,
				GroupFrameT(GSI(MSG_SCRIPTMANAGERGROUP_WHITELIST)),
				Child, VGroup, MUIA_Weight, 100,
					Child, lv_whitelist = (Object *) NewObject(getscriptmanagerhostlistclass(), NULL, TAG_DONE),
					Child, st_whitelist_url = (Object *) MakeString("", FALSE),
					End,
				Child, VGroup, MUIA_Weight, 1,
					Child, bt_whitelist_add    = (Object *) MakeButton(GSI(MSG_SCRIPTMANAGERGROUP_WHITELIST_ADD)),
					Child, bt_whitelist_remove = (Object *) MakeButton(GSI(MSG_SCRIPTMANAGERGROUP_WHITELIST_REMOVE)),
					Child, VSpace(0),
					End,

				End,

			Child, HGroup,
				GroupFrameT(GSI(MSG_SCRIPTMANAGERGROUP_BLACKLIST)),
				Child, VGroup, MUIA_Weight, 100,
					Child, lv_blacklist = (Object *) NewObject(getscriptmanagerhostlistclass(), NULL, TAG_DONE),
					Child, st_blacklist_url = (Object *) MakeString("", FALSE),
					End,
				Child, VGroup, MUIA_Weight, 1,
					Child, bt_blacklist_add    = (Object *) MakeButton(GSI(MSG_SCRIPTMANAGERGROUP_BLACKLIST_ADD)),
					Child, bt_blacklist_remove = (Object *) MakeButton(GSI(MSG_SCRIPTMANAGERGROUP_BLACKLIST_REMOVE)),
					Child, VSpace(0),
					End,

				End,

			End,

		TAG_MORE, INITTAGS
		);

	if (obj)
	{
		GETDATA;

		data->lv_scripts = lv_scripts;
		data->bt_script_add    = bt_script_add;
		data->bt_script_remove = bt_script_remove;
		data->ch_script_enable = ch_script_enable;
		data->gr_script = gr_script;
		data->txt_title = txt_title;
		data->txt_description = txt_description;
		data->txt_path = txt_path;
		data->lv_whitelist = lv_whitelist;
		data->lv_blacklist = lv_blacklist;
		data->st_whitelist_url = st_whitelist_url;
		data->st_blacklist_url = st_blacklist_url;
		data->bt_whitelist_add = bt_whitelist_add;
		data->bt_whitelist_remove = bt_whitelist_remove;
		data->bt_blacklist_add = bt_blacklist_add;
		data->bt_blacklist_remove = bt_blacklist_remove;

		set(gr_script, MUIA_Disabled, TRUE);
		set(bt_script_remove, MUIA_Disabled, TRUE);

		DoMethod(bt_script_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ScriptManagerGroup_Add);
		DoMethod(bt_script_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ScriptManagerGroup_Remove);
		DoMethod(lv_scripts,       MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 2, MM_ScriptManagerGroup_Update, MV_ScriptManagerGroup_Update_Script | MV_ScriptManagerGroup_Update_WhiteList | MV_ScriptManagerGroup_Update_BlackList);
		DoMethod(lv_whitelist,     MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 2, MM_ScriptManagerGroup_Update, MV_ScriptManagerGroup_Update_WhiteList);
		DoMethod(lv_blacklist,     MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 2, MM_ScriptManagerGroup_Update, MV_ScriptManagerGroup_Update_BlackList);

		DoMethod(bt_whitelist_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ScriptManagerGroup_WhiteList_Add);
		DoMethod(bt_whitelist_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ScriptManagerGroup_WhiteList_Remove);

		DoMethod(bt_blacklist_add,    MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ScriptManagerGroup_BlackList_Add);
		DoMethod(bt_blacklist_remove, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_ScriptManagerGroup_BlackList_Remove);

		DoMethod(ch_script_enable, MUIM_Notify, MUIA_Selected,        MUIV_EveryTime, obj, 1, MM_ScriptManagerGroup_Change);
		DoMethod(st_whitelist_url, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_ScriptManagerGroup_Change);
		DoMethod(st_blacklist_url, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, obj, 1, MM_ScriptManagerGroup_Change);
	}

	return ((IPTR)obj);
}

DEFDISP
{
	for(size_t i = 0; i < scripts_list.size(); i++)
	{
		delete scripts_list[i];
	}

	scripts_list.clear();

	return DOSUPER;
}


DEFTMETHOD(ScriptManagerGroup_Load)
{
	GETDATA;

	load_scripts(obj, data);

	return 0;
}

DEFTMETHOD(ScriptManagerGroup_Add)
{
	GETDATA;

	APTR tags[] = { (APTR) ASLFR_TitleText, (APTR) GSI(MSG_SCRIPTMANAGERGROUP_SCRIPT_SELECT),
					(APTR) ASLFR_InitialPattern, (APTR) "#?.js",
					(APTR) ASLFR_InitialDrawer, (APTR) SCRIPT_DIRECTORY,
					TAG_DONE };

	char *file = asl_run(SCRIPT_DIRECTORY, (struct TagItem *) &tags, FALSE);

	if(file)
	{
		ScriptEntry *script = new ScriptEntry;
		
		if(script)
		{
			script->path = file;
			script->enabled = true;

			if(parse_script(script))
			{
				scripts_list.append(script);

				DoMethod(data->lv_scripts, MUIM_List_InsertSingle, script, MUIV_List_Insert_Bottom);
				set(data->lv_scripts, MUIA_List_Active, MUIV_List_Active_Bottom);

				save_scripts();
			}
		}

		FreeVecTaskPooled(file);
	}

	return 0;
}

DEFTMETHOD(ScriptManagerGroup_Remove)
{
	GETDATA;
	ScriptEntry *script;

	DoMethod(data->lv_scripts, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ScriptEntry **) &script);

	if(script)
	{
		DoMethod(data->lv_scripts, MUIM_List_Remove, MUIV_List_Remove_Active);

		for(size_t i = 0; i < scripts_list.size(); i++)
		{
			if(scripts_list[i] == script)
			{
				scripts_list.remove(i);
				delete script;
				break;
			}
		}

		save_scripts();
	}

	return 0;
}

DEFSMETHOD(ScriptManagerGroup_Update)
{
	GETDATA;
	ScriptEntry *script;

	DoMethod(data->lv_scripts, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ScriptEntry **) &script);

	if(script)
	{
		if(msg->which & MV_ScriptManagerGroup_Update_Script)
		{
			set(data->bt_script_remove, MUIA_Disabled, FALSE);
			set(data->ch_script_enable, MUIA_Disabled, FALSE);
			set(data->gr_script,        MUIA_Disabled, FALSE);


			set(data->ch_script_enable, MUIA_Selected, script->enabled);
			set(data->txt_title,        MUIA_Text_Contents, script->title.latin1().data());
			set(data->txt_path,         MUIA_Text_Contents, script->path.latin1().data());
			set(data->txt_description,  MUIA_Text_Contents, script->description.latin1().data());
			nnset(data->st_whitelist_url, MUIA_String_Contents, "");
			nnset(data->st_blacklist_url, MUIA_String_Contents, "");

			// white list
			DoMethod(data->lv_whitelist, MUIM_List_Clear);

			for(size_t i = 0; i < script->whitelist.size(); i++)
			{
				String *host = new String(script->whitelist[i]);
				DoMethod(data->lv_whitelist, MUIM_List_InsertSingle, host, MUIV_List_Insert_Bottom);
			}

			// black list
			DoMethod(data->lv_blacklist, MUIM_List_Clear);

			for(size_t i = 0; i < script->blacklist.size(); i++)
			{
				String *host = new String(script->blacklist[i]);
				DoMethod(data->lv_blacklist, MUIM_List_InsertSingle, host, MUIV_List_Insert_Bottom);
			}

			set(data->bt_script_remove, MUIA_Disabled, getv(data->lv_scripts, MUIA_List_Entries) == 0);
		}

		if(msg->which & MV_ScriptManagerGroup_Update_WhiteList)
		{
			String *host;

			DoMethod(data->lv_whitelist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (String **) &host);

			if(host)
			{
				nnset(data->st_whitelist_url, MUIA_String_Contents, host->latin1().data());
			}

			set(data->bt_whitelist_remove, MUIA_Disabled, getv(data->lv_whitelist, MUIA_List_Entries) == 0);
		}

		if(msg->which & MV_ScriptManagerGroup_Update_BlackList)
		{
			String *host;

			DoMethod(data->lv_blacklist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (String **) &host);

			if(host)
			{
				nnset(data->st_blacklist_url, MUIA_String_Contents, host->latin1().data());
			}

			set(data->bt_blacklist_remove, MUIA_Disabled, getv(data->lv_blacklist, MUIA_List_Entries) == 0);
		}
	}
	else
	{
		set(data->gr_script, MUIA_Disabled, TRUE);
		
		set(data->txt_title,        MUIA_Text_Contents, "");
		set(data->txt_path,         MUIA_Text_Contents, "");
		set(data->txt_description,  MUIA_Text_Contents, "");

		set(data->bt_script_remove, MUIA_Disabled, TRUE);
		set(data->ch_script_enable, MUIA_Disabled, TRUE);
		
		DoMethod(data->lv_whitelist, MUIM_List_Clear);
		DoMethod(data->lv_blacklist, MUIM_List_Clear);
		nnset(data->st_whitelist_url, MUIA_String_Contents, "");
		nnset(data->st_blacklist_url, MUIA_String_Contents, "");
	}

	return 0;
}

DEFTMETHOD(ScriptManagerGroup_Change)
{
	GETDATA;
	ScriptEntry	*script;

	DoMethod(data->lv_scripts, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ScriptEntry **) &script);

	if(script)
	{
		String *host;
		char *newhost;
		
		// whitelist
		newhost	= (char *) getv(data->st_whitelist_url, MUIA_String_Contents);
		DoMethod(data->lv_whitelist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (String **) &host);

		if(host && *host != newhost)
		{
			int i = 0;

			*host = newhost;
			DoMethod(data->lv_whitelist, MUIM_List_Redraw, MUIV_List_Redraw_Active);

			script->whitelist.clear();

			do
			{
				DoMethod(data->lv_whitelist, MUIM_List_GetEntry, i, (String **) &host);

				if(host)
				{
					script->whitelist.append(*host);
				}

				i++;
			}
			while (host);
		}

		// blacklist
		newhost	= (char *) getv(data->st_blacklist_url, MUIA_String_Contents);
		DoMethod(data->lv_blacklist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (String **) &host);

		if(host && *host != newhost)
		{
			int i = 0;

			*host = newhost;
			DoMethod(data->lv_blacklist, MUIM_List_Redraw, MUIV_List_Redraw_Active);

			script->blacklist.clear();

			do
			{
				DoMethod(data->lv_blacklist, MUIM_List_GetEntry, i, (String **) &host);

				if(host)
				{
					script->blacklist.append(*host);
				}

				i++;
			}
			while (host);
		}

		script->enabled = getv(data->ch_script_enable, MUIA_Selected) != 0;

		DoMethod(data->lv_scripts, MUIM_List_Redraw, MUIV_List_Redraw_Active);

		save_scripts();
	}

	return 0;
}

DEFTMETHOD(ScriptManagerGroup_WhiteList_Add)
{
	GETDATA;
	ScriptEntry	*script;
	DoMethod(data->lv_scripts, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ScriptEntry **) &script);

	if(script)
	{
		String *host = new String("http://");
		script->whitelist.append(*host);
		DoMethod(data->lv_whitelist, MUIM_List_InsertSingle, host, MUIV_List_Insert_Bottom);
		set(data->lv_whitelist, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(_win(obj), MUIA_Window_ActiveObject, data->st_whitelist_url);
	}

	return 0;
}

DEFTMETHOD(ScriptManagerGroup_WhiteList_Remove)
{
	GETDATA;
	ScriptEntry	*script;
	DoMethod(data->lv_scripts, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ScriptEntry **) &script);

	if(script)
	{
		String *host;
		DoMethod(data->lv_whitelist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (String **) &host);

		if(host)
		{
			int i = 0;

            DoMethod(data->lv_whitelist, MUIM_List_Remove, MUIV_List_Remove_Active);

			script->whitelist.clear();

			do
			{
				DoMethod(data->lv_whitelist, MUIM_List_GetEntry, i, (String **) &host);

				if(host)
				{
					script->whitelist.append(*host);
				}

				i++;
			}
			while (host);

			save_scripts();
		}
	}

    return 0;
}

DEFTMETHOD(ScriptManagerGroup_BlackList_Add)
{
	GETDATA;
	ScriptEntry	*script;
	DoMethod(data->lv_scripts, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ScriptEntry **) &script);

	if(script)
	{
		String *host = new String("http://");
		script->blacklist.append(*host);
		DoMethod(data->lv_blacklist, MUIM_List_InsertSingle, host, MUIV_List_Insert_Bottom);
		set(data->lv_blacklist, MUIA_List_Active, MUIV_List_Active_Bottom);
		set(_win(obj), MUIA_Window_ActiveObject, data->st_blacklist_url);
	}

    return 0;
}

DEFTMETHOD(ScriptManagerGroup_BlackList_Remove)
{
	GETDATA;
	ScriptEntry	*script;
	DoMethod(data->lv_scripts, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (ScriptEntry **) &script);

	if(script)
	{
		String *host;
		DoMethod(data->lv_blacklist, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, (String **) &host);

		if(host)
		{
			int i = 0;

            DoMethod(data->lv_blacklist, MUIM_List_Remove, MUIV_List_Remove_Active);

			script->blacklist.clear();

			do
			{
				DoMethod(data->lv_blacklist, MUIM_List_GetEntry, i, (String **) &host);

				if(host)
				{
					script->blacklist.append(*host);
				}

				i++;
			}
			while (host);

			save_scripts();
		}
	}
    return 0;
}

DEFSMETHOD(ScriptManagerGroup_InjectScripts)
{
	WebView *webView = (WebView *) msg->webView;

	if(!webView) return 0;

	Page * page = webView->page();

	if(page)
	{
		BalWidget *widget = webView->viewWindow();

		if(widget)
		{
			for(size_t i = 0; i < scripts_list.size(); i++)
			{
				if(scripts_list[i]->enabled)
				{
					OWBFile f(scripts_list[i]->path);

					if(f.open('r') != -1)
					{
						char *fileContent = f.read(f.getSize());

						if(fileContent)
						{
							Object *browser = widget->browser;
							char *url = (char *) getv(browser, MA_OWB_URL);

							auto userScript = std::make_unique<UserScript>(
															String::fromUTF8(fileContent),
															URL(URL(), String(url)), // XXX: will be empty most of the time...
															Vector<String>(scripts_list[i]->whitelist),
															Vector<String>(scripts_list[i]->blacklist),
															InjectAtDocumentEnd,
															InjectInAllFrames);
                            page->userContentController()->addUserScript(mainThreadNormalWorld(), std::move(userScript));

							delete [] fileContent;
						}

						f.close();
					}
				}
			}
		}
	}

	return 0;
}

DEFSMETHOD(ScriptManagerGroup_ScriptsForURL)
{
	Vector<ScriptEntry *> *matchingscripts = new Vector<ScriptEntry *>;

	for(size_t i = 0; i < scripts_list.size(); i++)
	{
		if(scripts_list[i]->enabled)
		{
			if(UserContentURLPattern::matchesPatterns(URL(ParsedURLString, String(msg->url)), scripts_list[i]->whitelist, scripts_list[i]->blacklist))
			{
				matchingscripts->append(scripts_list[i]);
			}
		}
	}

	return (ULONG) matchingscripts;
}

BEGINMTABLE
DECNEW
DECDISP
DECTMETHOD(ScriptManagerGroup_Load)
DECTMETHOD(ScriptManagerGroup_Add)
DECTMETHOD(ScriptManagerGroup_Remove)
DECTMETHOD(ScriptManagerGroup_WhiteList_Add)
DECTMETHOD(ScriptManagerGroup_WhiteList_Remove)
DECTMETHOD(ScriptManagerGroup_BlackList_Add)
DECTMETHOD(ScriptManagerGroup_BlackList_Remove)
DECSMETHOD(ScriptManagerGroup_Update)
DECTMETHOD(ScriptManagerGroup_Change)
DECSMETHOD(ScriptManagerGroup_InjectScripts)
DECSMETHOD(ScriptManagerGroup_ScriptsForURL)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, scriptmanagergroupclass)
