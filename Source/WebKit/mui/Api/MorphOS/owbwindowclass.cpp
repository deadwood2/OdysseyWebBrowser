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

/* OWB */
#include "config.h"
#include <Api/WebFrame.h>
#include <Api/WebView.h>
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>
#include "DOMImplementation.h"
#include "Editor.h"
#include "FileIOLinux.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "InspectorController.h"
#include <wtf/MainThread.h>
#include "Page.h"
#include "ProgressTracker.h"
#include "Settings.h"
#include "Scrollbar.h"
#include "SharedBuffer.h"
#include "SharedTimer.h"
#include "SubstituteData.h"
#include "Timer.h"
#include "WebDataSource.h"
#include "WebHistory.h"
#include "WebHistoryItem.h"
#include "WebHistoryItem_p.h"
#include "GraphicsContext.h"
#include "WebBackForwardList.h"
#include "WebPreferences.h"
#include "WindowFeatures.h"
#include "FrameLoader.h"
#include "ScriptEntry.h"
#include "TopSitesManager.h"

#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#endif

#include <unistd.h>
#include <cstdio>
#include <cairo.h>

#include <clib/macros.h>
#include <proto/iffparse.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <datatypes/textclass.h>
#include <workbench/workbench.h>

#include "gui.h"
#include "utils.h"
#include "clipboard.h"
#include "asl.h"
#include "versioninfo.h"

/******************************************************************
 * owbwindowclass
 *****************************************************************/

extern ULONG get_user_agent_count();
extern CONST_STRPTR * get_user_agent_labels();
extern CONST_STRPTR * get_user_agent_strings();

using namespace WebCore;

struct historymenuitemnode
{
	struct MinNode node;
	STRPTR title;
	STRPTR url;
	int x;
	int y;
	Object *obj;
};

struct Data
{
	struct windownode *node;
	char windowtitle[512];
    char progressbuffer[512];

	ULONG lastpagenum;
	Object *active_browser;
	Object *active_webinspector;

	/* Objects */
	Object *maingroup;
	Object *navigationgroup;
	Object *urlgroup;
	Object *addressbargroup;
	Object *urlbalance;
	Object *searchgroup;
	Object *fastlinkgroup;
	Object *fastlinkparentgroup;
	Object *pagegroup;
	Object *pagetitles;
	Object *findgroup;
	Object *statusgroup;
	Object *zoneimage;
	Object *secureimage;
	Object *privatebrowsingimage;
	Object *userscriptimage;
	Object *statusbar;
	Object *progressgauge;
	Object *progressgroup;
	Object *networkledsgroup;
	Object *inspectorbutton;
	Object *panelgroup;
	Object *panelbalance;
	Object *bookmarkpanelgroup;
	Object *historypanelgroup;
	Object *navigationseparator;
	Object *fastlinkseparator;

	/* Completion objects */
	ThreadIdentifier completion_thread;
	ULONG abort_completion;
	ULONG completion_mode_popup;
	ULONG completion_mode_string;
	ULONG completion_prevlen;
	String url_to_complete;
	Object *lv_completion;

	/* Position */
	ULONG maingroup_innerleft;
	ULONG maingroup_innerright;
	ULONG maingroup_innertop;
	ULONG maingroup_innerbottom;

	/* History */
	struct MinList history_closed_list;
	struct MinList history_list;
	
	/* Misc */
	STRPTR userscriptimage_shorthelp;

	ULONG fullscreen;
	ULONG panelshow;
	ULONG paneltype;
};

Object *create_window(char * url, ULONG isframe, APTR sourceview, APTR features, ULONG privatebrowsing)
{
	return (Object *) NewObject(getowbwindowclass(), NULL,
						MA_OWBBrowser_URL, (ULONG) url,
						MA_OWBBrowser_IsFrame, (ULONG) isframe,
                        MA_OWBBrowser_SourceView, (ULONG) sourceview,
						MA_OWBWindow_Features, (ULONG) features,
						MA_OWBBrowser_PrivateBrowsing, privatebrowsing,
						TAG_DONE);
}

static void copyWebViewSelectionToClipboard(WebView* webView, bool html, bool local)
{
    Frame* frame = &(core(webView)->focusController().focusedOrMainFrame());
    if (!frame)
        return;

    String selectedText;
    if (html) {
       FrameSelection *selection = &frame->selection();
       if (selection)
           selectedText = selection->toNormalizedRange()->toHTML();
    }
    else
	   selectedText = frame->editor().selectedText();

    CString text = selectedText.utf8();
    const char *data = text.data();
    size_t len = text.length();
	char *converted_data = local ? utf8_to_local(data) : strdup(data);
	size_t converted_len = local ? strlen(converted_data) : len;

	if(converted_data && converted_len)
	{
		copyTextToClipboard(converted_data, local ? false : true);

		free(converted_data);
	}
}

static  void setup_browser_notifications(Object *obj, struct Data *data, Object *browser)
{
	//kprintf("OWBWindow: setup notifications for browser %p\n", browser);

	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_Loading, MUIV_EveryTime, data->progressgroup,   3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_Loading, MUIV_EveryTime, data->navigationgroup, 3, MUIM_Set, MA_TransferAnim_Animate, MUIV_TriggerValue);

	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_LoadingProgress,  MUIV_EveryTime, obj, 2, MM_OWBWindow_UpdateProgress, browser);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_StatusText,       MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdateStatus,   browser, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_ToolTipText,      MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdateStatus,   browser, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_URL,              MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdateURL,      browser, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_Title,            MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdateTitle,    browser, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_State,            MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdateState,    browser, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_Zone,             MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdateZone,     browser, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_Security,         MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdateSecurity, browser, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_PrivateBrowsing,  MUIV_EveryTime, obj, 3, MM_OWBWindow_UpdatePrivateBrowsing, browser, MUIV_TriggerValue);

	// We could pass the actual attribute for more efficiency there
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_BackAvailable,    MUIV_EveryTime, obj, 2, MM_OWBWindow_UpdateNavigation, browser);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_ForwardAvailable, MUIV_EveryTime, obj, 2, MM_OWBWindow_UpdateNavigation, browser);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_ReloadAvailable,  MUIV_EveryTime, obj, 2, MM_OWBWindow_UpdateNavigation, browser);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_StopAvailable,    MUIV_EveryTime, obj, 2, MM_OWBWindow_UpdateNavigation, browser);

	/*
	Object *urlstring = (Object *) getv((Object *) getv(data->addressbargroup, MA_AddressBarGroup_PopString), MUIA_Popstring_String);
	kprintf("set notification urlstring %p browser %p\n", urlstring, browser);
	DoMethod(urlstring, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime, browser, 3, MUIM_Set, MA_OWBBrowser_EditedURL, MUIV_TriggerValue);
	*/
	//DoMethod(obj, MUIM_Notify, MUIA_Window_ActiveObject, MUIV_EveryTime, browser, 1, MM_OWBBrowser_FocusChanged);
}

static void cleanup_browser_notifications(Object *obj, struct Data *data, Object *browser)
{
	//DoMethod(obj, MUIM_KillNotify, MUIA_Window_ActiveObject);

	//kprintf("OWBWindow: cleanup notifications for browser %p\n", browser);
	/*
	Object *urlstring = (Object *) getv((Object *) getv(data->addressbargroup, MA_AddressBarGroup_PopString), MUIA_Popstring_String);
	kprintf("clear notification urlstring %p browser %p\n", urlstring, browser);
	DoMethod(urlstring, MUIM_KillNotifyObj, MUIA_String_Contents, browser);
	*/
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_Loading);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_Loading);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_Loading);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_LoadingProgress);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_State);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_Zone);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_Security);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_PrivateBrowsing);

	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_StatusText);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_ToolTipText);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_URL);
	DoMethod(browser, MUIM_KillNotifyObj, MA_OWBBrowser_Title, obj); // Don't kill the other notifications the browser have
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_BackAvailable);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_ForwardAvailable);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_ReloadAvailable);
	DoMethod(browser, MUIM_KillNotify, MA_OWBBrowser_StopAvailable);
}

static void setup_scrollbars_notifications(Object *browser, Object *vbar, Object *hbar, Object *vbargroup, Object *hbargroup)
{
	set(browser, MA_OWBBrowser_VBar, vbar);
	set(browser, MA_OWBBrowser_HBar, hbar);
	set(browser, MA_OWBBrowser_VBarGroup, vbargroup);
	set(browser, MA_OWBBrowser_HBarGroup, hbargroup);

	/* Scrollers */
	DoMethod(vbar, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, browser, 3, MM_OWBBrowser_SetScrollOffset, (LONG) -1, (LONG) MUIV_TriggerValue);
	//DoMethod(vbar, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, app, 6, MUIM_Application_PushMethod, browser, 3 | MUIV_PushMethod_Delay(1) | MUIF_PUSHMETHOD_SINGLE, MM_OWBBrowser_SetScrollOffset, (LONG) -1, (LONG) MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_VTotalPixel  , MUIV_EveryTime, vbar, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_VVisiblePixel, MUIV_EveryTime, vbar, 3, MUIM_NoNotifySet, MUIA_Prop_Visible, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_VTopPixel    , MUIV_EveryTime, vbar, 3, MUIM_NoNotifySet, MUIA_Prop_First  , MUIV_TriggerValue);

	DoMethod(hbar, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, browser, 3, MM_OWBBrowser_SetScrollOffset, (LONG) MUIV_TriggerValue, (LONG) -1);
	//DoMethod(hbar, MUIM_Notify, MUIA_Prop_First, MUIV_EveryTime, app, 6, MUIM_Application_PushMethod, browser, 3 | MUIV_PushMethod_Delay(1) | MUIF_PUSHMETHOD_SINGLE, MM_OWBBrowser_SetScrollOffset, (LONG) MUIV_TriggerValue, (LONG) -1);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_HTotalPixel  , MUIV_EveryTime, hbar, 3, MUIM_NoNotifySet, MUIA_Prop_Entries, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_HVisiblePixel, MUIV_EveryTime, hbar, 3, MUIM_NoNotifySet, MUIA_Prop_Visible, MUIV_TriggerValue);
	DoMethod(browser, MUIM_Notify, MA_OWBBrowser_HTopPixel    , MUIV_EveryTime, hbar, 3, MUIM_NoNotifySet, MUIA_Prop_First  , MUIV_TriggerValue);

}

#if OS(MORPHOS)
/* Handle appwindow messages */
static LONG AppMsgFunc(void)
{
	struct AppMessage **x = (struct AppMessage **) REG_A1;
	Object *window = (Object *) REG_A2;
	struct WBArg *ap;
	struct AppMessage *amsg = *x;
	int i;
	char buf[256];

	for(ap = amsg->am_ArgList, i = 0; i < amsg->am_NumArgs; i++,ap++)
	{
		if(NameFromLock(ap->wa_Lock, buf, sizeof(buf)))
		{
			char url[2048];
			AddPart((STRPTR) buf, (STRPTR) ap->wa_Name, sizeof(buf));
			snprintf(url, sizeof(url), "file:///%s", buf);

			DoMethod(window, MM_OWBWindow_AddBrowser, url, FALSE, NULL, TRUE, FALSE, TRUE);
		}
	}

	return(0);
}

static struct EmulLibEntry AppMsgHookGate = { TRAP_LIB, 0, (void (*)(void))AppMsgFunc };
static struct Hook AppMsgHook = { {0, 0}, (HOOKFUNC)&AppMsgHookGate, NULL, NULL };
#endif
#if OS(AROS)
static struct Hook AppMsgHook = { {0, 0}, NULL, NULL, NULL };
#endif

DEFNEW
{
	Object *maingroup, *navigationgroup, *urlgroup, *searchgroup, *addressbargroup, *urlbalance, *fastlinkgroup, *fastlinkparentgroup, *findgroup, *statusbar, *statusgroup, *progressgauge, *progressgroup;
	Object *panelgroup, *panelbalance;
	Object *pagegroup, *pagetitles;
	Object *zoneimage, *secureimage, *privatebrowsingimage, *userscriptimage;
	Object *networkledsgroup;
	Object *menustrip;
	//APTR n, m;
	char id = 'W'; // Sigh, let's stay compatible with previous release, now the mistake is made :)
	Object *initialbrowser = (Object *) GetTagData(MA_OWBWindow_InitialBrowser, NULL, msg->ops_AttrList);
	struct windownode *node = (struct windownode *) malloc(sizeof(struct windownode));

	ULONG showseparators = getv(app, MA_OWBApp_ShowSeparators);

	if(!node)
		return 0;

	obj = (Object *) DoSuperNew(cl, obj,
		MUIA_Window_ScreenTitle, "Odyssey Web Browser " VERSION " (" DATE ")",
		MUIA_Window_Title, DEFAULT_WINDOW_NAME,
		MUIA_Window_ID, MAKE_ID('O','W','B', id),
		MUIA_Window_Menustrip, menustrip = (Object *) MUI_MakeObject(MUIO_MenustripNM, MenuData, 0),
        MUIA_Window_AppWindow, TRUE,
		WindowContents, maingroup = VGroup,
			Child, navigationgroup     = VGroup,
											Child, (Object *) NewObject(getnavigationgroupclass(), NULL, TAG_DONE),
											End,

			Child, fastlinkparentgroup = VGroup,
											Child, (Object *) NewObject(getquicklinkparentgroupclass(), NULL,
															            MA_QuickLinkParentGroup_QLGroup,
															            fastlinkgroup = (Object *) NewObject(getquicklinkgroupclass(), NULL,
																								             MA_QuickLinkGroup_Row, getv(app, MA_OWBApp_QuickLinkRows),
																								             MA_QuickLinkGroup_Mode, getv(app, MA_OWBApp_QuickLinkLayout) | getv(app, MA_OWBApp_QuickLinkLook),
																								             TAG_DONE),
															            TAG_DONE),
											End,

			Child, urlgroup = HGroup,
				Child, addressbargroup = (Object *) NewObject(getaddressbargroupclass(), NULL,
													          MA_AddressBarGroup_GoButton, getv(app, MA_OWBApp_ShowValidationButtons),
													          TAG_DONE),
				Child, urlbalance      = BalanceObject, MUIA_ObjectID, MAKE_ID('O','U','B','A'),
														MUIA_Balance_Quiet, TRUE,
														End,
				Child, searchgroup     = (Object *) NewObject(getsearchbargroupclass(), NULL,
															  MA_SearchBarGroup_SearchButton, getv(app, MA_OWBApp_ShowValidationButtons),
				                                              TAG_DONE),
				End,

			Child, HGroup,
				Child, panelgroup   = HGroup, MUIA_Weight, 20, MUIA_ShowMe, FALSE, End,
				Child, panelbalance = BalanceObject, MUIA_ObjectID, MAKE_ID('O','P','B','A'), MUIA_ShowMe, FALSE, End,
				Child, pagegroup    = VGroup,
										MUIA_Background, MUII_RegisterBack,
										MUIA_Frame, MUIV_Frame_Register,
										MUIA_Group_PageMode, TRUE,
										InnerSpacing(0, 0),
										GroupSpacing(0),
										Child, pagetitles = (Object *) NewObject(gettitleclass(), NULL, TAG_DONE),
									    End,
				End,

			Child, findgroup = (Object *) NewObject(getfindtextclass(), NULL,
													MUIA_ShowMe, FALSE,
													MA_FindText_ShowAllMatches, TRUE,
													TAG_DONE),

			Child, statusgroup = HGroup,

					Child, zoneimage = (Object *) MUI_NewObject(MUIC_Dtpic,
																MUIA_Dtpic_Name, "PROGDIR:resource/zone_local.png",
																TAG_DONE),
					
					Child, MakeVBar(),

					Child, secureimage = (Object *) MUI_NewObject(MUIC_Dtpic,
																  MUIA_Dtpic_Alpha, 0,
																  MUIA_Dtpic_Name, "PROGDIR:resource/zone_notsecure.png",
																  TAG_DONE),
					Child, MakeVBar(),

					Child, privatebrowsingimage = (Object *) MUI_NewObject(MUIC_Dtpic,
																  MUIA_Dtpic_Alpha, 0,
#if !OS(AROS)
																  MUIA_Dtpic_Name, "PROGDIR:resource/private_browsing.png",
#else
                                                                  MUIA_Dtpic_Name, "PROGDIR:resource/private_browsing_a.png",
#endif
																  TAG_DONE),
					Child, MakeVBar(),

					Child, userscriptimage = (Object *) MUI_NewObject(MUIC_Dtpic,
																  MUIA_Dtpic_Alpha, 0,
																  MUIA_Dtpic_Name, "PROGDIR:resource/scriptIcon.png",
																  TAG_DONE),
					Child, MakeVBar(),

					Child, statusbar = TextObject,
										MUIA_Text_SetMax, FALSE,
										MUIA_Text_SetMin, FALSE,
										MUIA_Weight, 80,
                                        MUIA_Text_PreParse, "\033-",
										End,

					Child, progressgroup = HGroup,
						MUIA_Weight, 20,
						MUIA_Group_PageMode, TRUE,
						Child,
							VGroup, Child, RectangleObject, End, End,
						Child,
							VGroup, Child, progressgauge = GaugeObject,
								                GaugeFrame,
												MUIA_Gauge_Horiz, TRUE,
												MUIA_FixHeightTxt, "/",
												MUIA_Gauge_InfoText, "",
												MUIA_Gauge_Current, 0,
												MUIA_Gauge_Max, 100,
												End,
							End,
						End,

					Child, networkledsgroup = (Object *) NewObject(getnetworkledsgroupclass(), NULL, TAG_DONE),

					End,
			End,
		    TAG_MORE, INITTAGS);

	if (obj)
	{
		GETDATA;

		NEWLIST(&data->history_closed_list);
		NEWLIST(&data->history_list);

		node->window = obj;
		data->node = node;
		data->userscriptimage_shorthelp = NULL;

		data->maingroup       = maingroup;
		data->navigationgroup = navigationgroup;
		data->urlgroup        = urlgroup;
		data->searchgroup     = searchgroup;
		data->urlbalance      = urlbalance;
		data->addressbargroup = addressbargroup;
		data->fastlinkgroup   = fastlinkgroup;
		data->fastlinkparentgroup = fastlinkparentgroup;
		data->pagegroup       = pagegroup;
		data->pagetitles      = pagetitles;
		data->findgroup       = findgroup;
		data->statusgroup     = statusgroup;
		data->statusbar       = statusbar;
		data->zoneimage       = zoneimage;
		data->secureimage     = secureimage;
		data->privatebrowsingimage = privatebrowsingimage;
		data->userscriptimage = userscriptimage;
		data->progressgauge   = progressgauge;
		data->progressgroup   = progressgroup;
		data->networkledsgroup = networkledsgroup;
		data->panelgroup       = panelgroup;
		data->panelbalance     = panelbalance;

		set(obj, MA_OWBWindow_ShowSeparators, showseparators);

		/* Create the first browser object */
		if(initialbrowser)
		{
			DoMethod(obj, MM_OWBWindow_TransferBrowser, initialbrowser);
		}
		else
		{
			DoMethod(obj, MM_OWBWindow_AddBrowser,
					 (char *) GetTagData(MA_OWBBrowser_URL, NULL, msg->ops_AttrList),
					 (ULONG)  GetTagData(MA_OWBBrowser_IsFrame, NULL, msg->ops_AttrList),
					 (APTR)   GetTagData(MA_OWBBrowser_SourceView, NULL, msg->ops_AttrList),
					 FALSE,
					 (ULONG)  GetTagData(MA_OWBBrowser_PrivateBrowsing, FALSE, msg->ops_AttrList),
					 TRUE);
		}

		/* Notifications */
		DoMethod(obj, MUIM_Notify, MUIA_AppMessage, MUIV_EveryTime, obj, 3, MUIM_CallHook, &AppMsgHook, MUIV_TriggerValue);
		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, obj, 1, MM_OWBWindow_Close);
		DoMethod(obj, MUIM_Notify, MUIA_Window_MenuAction, MUIV_EveryTime, obj, 2, MM_OWBWindow_MenuAction, MUIV_TriggerValue);
		DoMethod(data->pagegroup, MUIM_Notify, MUIA_Group_ActivePage, MUIV_EveryTime, obj, 2, MM_OWBWindow_ActivePage, MUIV_TriggerValue);
		DoMethod(obj, MUIM_Notify, MUIA_Window_Activate, TRUE, MUIV_Notify_Application, 3, MUIM_Set, MA_OWBApp_LastActiveWindow, obj);

		/* Menus */
		DoMethod(obj, MM_OWBWindow_BuildSpoofMenu);

		DoMethod(menustrip, MUIM_SetUData, MNA_LOCATION,   MUIA_Menuitem_Checked, TRUE); // These ones shouldn't be hardcoded, and should be loaded/saved
		DoMethod(menustrip, MUIM_SetUData, MNA_QUICKLINKS, MUIA_Menuitem_Checked, TRUE);
		DoMethod(menustrip, MUIM_SetUData, MNA_NAVIGATION, MUIA_Menuitem_Checked, TRUE);
		DoMethod(menustrip, MUIM_SetUData, MNA_STATUS,     MUIA_Menuitem_Checked, TRUE);

		/* Initialization */
		set(data->pagegroup, MUIA_Group_ActivePage, 0);
		DoMethod(obj, MM_OWBWindow_UpdatePanelGroup);
		DoMethod((Object *) getv(app, MA_OWBApp_BookmarkWindow), MM_Bookmarkgroup_RegisterQLGroup, fastlinkgroup, fastlinkparentgroup);
		DoMethod((Object *) getv(app, MA_OWBApp_BookmarkWindow), MM_Bookmarkgroup_BuildMenu, DoMethod(menustrip, MUIM_FindUData, MNA_BOOKMARKS_MENU));

		ADDTAIL(&window_list, data->node);

		return (ULONG)obj;
	}
	else
	{
		free(node);
	}

	return 0;
}

DEFDISP
{
	GETDATA;
	APTR n, m;

	//kprintf("OWBWindow: disposing window %p\n", obj);

	if(data->completion_thread)
	{
		data->abort_completion = TRUE;
		waitForThreadCompletion(data->completion_thread);
		data->completion_thread = NULL;
	}

	DoMethod((Object *) getv(app, MA_OWBApp_BookmarkWindow), MM_Bookmarkgroup_UnRegisterQLGroup, data->fastlinkgroup);

	ITERATELISTSAFE(n, m, &data->history_closed_list)
	{
		struct historymenuitemnode *mn = (struct historymenuitemnode *) n;
		REMOVE(mn);
		free(mn->url);
		free(mn->title);
		free(mn);
	}

	ITERATELISTSAFE(n, m, &data->history_list)
	{
		struct historymenuitemnode *mn = (struct historymenuitemnode *) n;
		REMOVE(mn);
		free(mn->url);
		free(mn->title);
		free(mn);
	}

	/* Is it really necessary? */
	if(data->active_browser)
	{
		set(data->active_browser, MA_OWBBrowser_Loading, FALSE);
		cleanup_browser_notifications(obj, data, data->active_browser);
	}

	REMOVE(data->node);
	free(data->node);

	free(data->userscriptimage_shorthelp);

	//kprintf("OWBWindow: ok, calling supermethod\n");

	return DOSUPER;
}

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case MA_OWBWindow_ActiveWebInspector:
			{
				data->active_webinspector = (Object *) tag->ti_Data;
				break;
			}

			case MA_OWBWindow_ShowSeparators:
			{
				if(tag->ti_Data)
				{
					if(!data->navigationseparator)
					{
						data->navigationseparator = MUI_MakeObject(MUIO_HBar, NULL);
						set(data->navigationseparator, MUIA_Weight, 0);

						DoMethod(data->navigationgroup, MUIM_Group_InitChange);
						DoMethod(data->navigationgroup, MUIM_Group_AddTail, data->navigationseparator);
						DoMethod(data->navigationgroup, MUIM_Group_ExitChange);
					}

					if(!data->fastlinkseparator)
					{
						data->fastlinkseparator = MUI_MakeObject(MUIO_HBar, NULL);
						set(data->fastlinkseparator, MUIA_Weight, 0);

						DoMethod(data->fastlinkparentgroup, MUIM_Group_InitChange);
						DoMethod(data->fastlinkparentgroup, MUIM_Group_AddTail, data->fastlinkseparator);
						DoMethod(data->fastlinkparentgroup, MUIM_Group_ExitChange);
					}
				}
				else
				{
					if(data->navigationseparator)
					{
						DoMethod(data->navigationgroup, MUIM_Group_InitChange);
						DoMethod(data->navigationgroup, MUIM_Group_Remove, data->navigationseparator);
						DoMethod(data->navigationgroup, MUIM_Group_ExitChange);
						MUI_DisposeObject(data->navigationseparator);
						data->navigationseparator = NULL;
					}

					if(data->fastlinkseparator)
					{
						DoMethod(data->fastlinkparentgroup, MUIM_Group_InitChange);
						DoMethod(data->fastlinkparentgroup, MUIM_Group_Remove, data->fastlinkseparator);
						DoMethod(data->fastlinkparentgroup, MUIM_Group_ExitChange);
						MUI_DisposeObject(data->fastlinkseparator);
						data->fastlinkseparator = NULL;
					}
				}

				break;
			}

			case MA_OWBWindow_ShowSearchGroup:
			{
				set(data->urlbalance,  MUIA_ShowMe, tag->ti_Data);
				set(data->searchgroup, MUIA_ShowMe, tag->ti_Data);
				break;
			}

			case MA_OWBWindow_ShowAddressBarGroup:
			{
				set(data->urlgroup, MUIA_ShowMe, tag->ti_Data);
				DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
						 MUIM_SetUData, MNA_LOCATION, MUIA_Menuitem_Checked, tag->ti_Data);
				break;
			}

			case MA_OWBWindow_ShowQuickLinkGroup:
			{
				set(data->fastlinkparentgroup, MUIA_ShowMe, tag->ti_Data);
				DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
						 MUIM_SetUData, MNA_QUICKLINKS, MUIA_Menuitem_Checked, tag->ti_Data);
				break;
			}

			case MA_OWBWindow_ShowNavigationGroup:
			{
				set(data->navigationgroup, MUIA_ShowMe, tag->ti_Data);
				DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
						 MUIM_SetUData, MNA_NAVIGATION, MUIA_Menuitem_Checked, tag->ti_Data);
				break;
			}

			case MA_OWBWindow_ShowStatusGroup:
			{
				set(data->statusgroup, MUIA_ShowMe, tag->ti_Data);
				DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
						 MUIM_SetUData, MNA_STATUS, MUIA_Menuitem_Checked, tag->ti_Data);
				break;
			}

			case MA_OWBApp_ToolButtonType:
			{
				set(data->navigationgroup, MA_OWBApp_ToolButtonType, tag->ti_Data);
				DoMethod(data->active_browser, MM_OWBBrowser_UpdateNavigation);
				break;
			}

			case MA_AddressBarGroup_Active:
			{
				set(data->addressbargroup, MA_AddressBarGroup_Active, tag->ti_Data);
				break;
			}

			/* Rather sucky handling, do something smarter */
			case MA_OWBWindow_ShowBookmarkPanel:
			{
				ULONG show = tag->ti_Data;

				data->historypanelgroup = NULL;

				DoMethod(_parent(data->panelgroup), MUIM_Group_InitChange);

				DoMethod(data->panelgroup, MUIM_Group_InitChange);

				FORCHILD(data->panelgroup, MUIA_Group_ChildList)
				{
					DoMethod(data->panelgroup, OM_REMMEMBER, child);
					MUI_DisposeObject((Object *) child);
				}
				NEXTCHILD

				if(show)
				{
					data->bookmarkpanelgroup = (Object *) NewObject(getbookmarkpanelgroupclass(), NULL, TAG_DONE);
					DoMethod(data->panelgroup, OM_ADDMEMBER, data->bookmarkpanelgroup);
					set(data->panelbalance, MUIA_ShowMe, TRUE);
				}
				else
				{
					data->bookmarkpanelgroup = NULL;
					set(data->panelbalance, MUIA_ShowMe, FALSE);
				}

				DoMethod(data->panelgroup, MUIM_Group_ExitChange);

				set(data->panelgroup, MUIA_ShowMe, show);

				DoMethod(_parent(data->panelgroup), MUIM_Group_ExitChange);

				set(app, MA_OWBApp_ShowBookmarkPanel, show);
				set(app, MA_OWBApp_ShowHistoryPanel, FALSE);

				break;
			}

			case MA_OWBWindow_ShowHistoryPanel:
			{
				ULONG show = tag->ti_Data;

				data->bookmarkpanelgroup = NULL;

				DoMethod(_parent(data->panelgroup), MUIM_Group_InitChange);

				DoMethod(data->panelgroup, MUIM_Group_InitChange);

				FORCHILD(data->panelgroup, MUIA_Group_ChildList)
				{
					DoMethod(data->panelgroup, OM_REMMEMBER, child);
					MUI_DisposeObject((Object *) child);
				}
				NEXTCHILD

				if(show)
				{
					data->historypanelgroup = (Object *) NewObject(gethistorypanelgroupclass(), NULL, TAG_DONE);
					DoMethod(data->panelgroup, OM_ADDMEMBER, data->historypanelgroup);
					set(data->panelbalance, MUIA_ShowMe, TRUE);
				}
				else
				{
					data->historypanelgroup = NULL;
					set(data->panelbalance, MUIA_ShowMe, FALSE);
				}

				DoMethod(data->panelgroup, MUIM_Group_ExitChange);

				set(data->panelgroup, MUIA_ShowMe, show);

				DoMethod(_parent(data->panelgroup), MUIM_Group_ExitChange);

				set(app, MA_OWBApp_ShowHistoryPanel, show);
				set(app, MA_OWBApp_ShowBookmarkPanel, FALSE);

				break;
			}

			case MUIA_Window_Open:
			{
				if(tag->ti_Data == FALSE)
				{
#if ENABLE(VIDEO)
					// disable fullpage mode (overlay) to be on the safe side
					FORCHILD(data->pagegroup, MUIA_Group_ChildList)
					{
						if (data->pagetitles == child)
							continue;

						Object *browser = (Object *) getv((Object *)child, MA_OWBGroup_Browser);

						HTMLMediaElement *element = (HTMLMediaElement *) getv(browser, MA_OWBBrowser_VideoElement);
						if(element)
						{
                            element->exitFullscreen();
							break;
						}
					}
					NEXTCHILD
#endif
				}
			}
			break;
		}
	}
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
		case MA_OWBWindow_ActiveBrowser:
		{
			*msg->opg_Storage = (ULONG) data->active_browser;
		}
		return TRUE;

		case MA_OWBWindow_ActiveWebInspector:
		{
			*msg->opg_Storage = (ULONG) data->active_webinspector;
		}
		return TRUE;

		case MA_OWBWindow_FastLinkGroup:
		{
			*msg->opg_Storage = (ULONG) data->fastlinkgroup;
		}
		return TRUE;

		case MA_OWBWindow_FastLinkParentGroup:
		{
			*msg->opg_Storage = (ULONG) data->fastlinkparentgroup;
		}
		return TRUE;

		case MA_OWBWindow_NavigationGroup:
		{
			*msg->opg_Storage = (ULONG) data->navigationgroup;
		}
		return TRUE;

		case MA_OWBWindow_AddressBarGroup:
		{
			*msg->opg_Storage = (ULONG) data->addressbargroup;
		}
		return TRUE;

		case MA_OWBWindow_NetworkLedsGroup:
		{
			*msg->opg_Storage = (ULONG) data->networkledsgroup;
		}
		return TRUE;

		case MA_OWBWindow_SearchGroup:
		{
			*msg->opg_Storage = (ULONG) data->searchgroup;
		}
		return TRUE;

		case MA_OWB_WindowType:
		{
			*msg->opg_Storage = (ULONG) MV_OWB_Window_Browser;
		}
		return TRUE;

		case MA_OWBWindow_ShowBookmarkPanel:
		{
			*msg->opg_Storage = (ULONG) (data->bookmarkpanelgroup != NULL);
		}
		return TRUE;

		case MA_OWBWindow_ShowHistoryPanel:
		{
			*msg->opg_Storage = (ULONG) (data->historypanelgroup != NULL);
		}
		return TRUE;

		case MA_OWBWindow_BookmarkPanelGroup:
		{
			*msg->opg_Storage = (ULONG) data->bookmarkpanelgroup;
		}
		return TRUE;

		case MA_OWBWindow_HistoryPanelGroup:
		{
			*msg->opg_Storage = (ULONG) data->historypanelgroup;
		}
		return TRUE;
	}

	return DOSUPER;
}

DEFSMETHOD(OWBWindow_MenuAction)
{
	GETDATA;

	switch (msg->action)
	{
		case MNA_ABOUT:
		{
			DoMethod(app, MM_OWBApp_About);
		}
		break;

		case MNA_QUIT:
		{
			DoMethod(app, MM_OWBApp_Quit);
		}
		break;

		case MNA_OWB_SETTINGS:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_Settings);
		}
		break;

		case MNA_MUI_SETTINGS:
		{
			DoMethod(app, MUIM_Application_OpenConfigWindow, 0, NULL);
		}
		break;
		
		case MNA_OPEN_LOCAL_FILE:
		{
			DoMethod(obj, MM_OWBWindow_OpenLocalFile);
		}
		break;

		case MNA_OPEN_URL:
		{
			set(data->addressbargroup, MA_AddressBarGroup_Active, TRUE);
			break;
		}

		case MNA_SAVE_AS_SOURCE:
		{
			DoMethod(obj, MM_OWBWindow_SaveAsSource);
		}
		break;

		case MNA_SAVE_AS_PDF:
		{
			DoMethod(obj, MM_OWBWindow_SaveAsPDF);
		}
		break;

		case MNA_PRINT:
		{
			DoMethod(obj, MM_OWBWindow_Print);
		}
		break;

		case MNA_OPEN_SESSION:
		{
			DoMethod(app, MM_OWBApp_RestoreSession, FALSE, TRUE);
		}
		break;

		case MNA_SAVE_SESSION:
		{
            DoMethod(app, MM_OWBApp_SaveSession, TRUE);
		}
		break;

		case MNA_NEW_PAGE:
		{
			DoMethod(app, MM_OWBApp_AddBrowser, obj, (STRPTR) getv(app, MA_OWBApp_NewTabPage), FALSE, NULL, FALSE, FALSE, TRUE);
			set(data->addressbargroup, MA_AddressBarGroup_Active, TRUE);
		}
		break;

		case MNA_CLOSE_PAGE:
		{
			DoMethod(app, MM_OWBApp_RemoveBrowser, data->active_browser);
		}
		break;

		case MNA_NEW_WINDOW:
		{
			DoMethod(app, MM_OWBApp_AddWindow, (STRPTR) getv(app, MA_OWBApp_NewTabPage), FALSE, FALSE, TRUE, NULL, FALSE);
		}
		break;

		case MNA_CLOSE_WINDOW:
		{
			DoMethod(obj, MM_OWBWindow_Close);
		}
		break;

		case MNA_BACK:
		{
			DoMethod(obj, MM_OWBWindow_Back);
		}
		break;

		case MNA_FORWARD:
		{
			DoMethod(obj, MM_OWBWindow_Forward);
		}
		break;

		case MNA_HOME:
		{
			DoMethod(obj, MM_OWBWindow_Home);
		}
		break;

		case MNA_STOP:
		{
			DoMethod(obj, MM_OWBWindow_Stop);
		}
		break;

		case MNA_RELOAD:
		{
			DoMethod(obj, MM_OWBWindow_Reload, NULL);
		}
		break;

		case MNA_CUT:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			widget->webView->cut();
		}
		break;

		case MNA_COPY:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			copyWebViewSelectionToClipboard(widget->webView, false, false);
		}
		break;

		case MNA_COPY_LOCAL:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			copyWebViewSelectionToClipboard(widget->webView, false, true);
		}
		break;

		case MNA_PASTE:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			widget->webView->paste();
		}
		break;

		case MNA_PASTE_TO_URL:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			if(widget)
			{
				String url = pasteFromClipboard();
				DoMethod(widget->window, MM_OWBWindow_LoadURL, url.utf8().data(), NULL);
			}
		}
		break;

		case MNA_SELECT_ALL:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			widget->webView->executeCoreCommandByName("SelectAll", "");
		}
		break;

		case MNA_FIND:
		{
			set(data->findgroup, MA_FindText_Target, obj);
			set(data->findgroup, MUIA_ShowMe, TRUE);			
			set(data->findgroup, MA_FindText_Active, TRUE);
		}
		break;

		case MNA_ZOOM_IN:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			widget->webView->zoomPageIn();
		}
		break;

		case MNA_ZOOM_OUT:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			widget->webView->zoomPageOut();
		}
		break;

		case MNA_ZOOM_RESET:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			if (widget->webView->canResetPageZoom())
				widget->webView->resetPageZoom();
		}
		break;

		case MNA_SOURCE:
		{
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

			if(widget)
			{
				WebView *sourceview = widget->webView;
				DoMethod(app, MM_OWBApp_AddWindow, NULL, FALSE, sourceview, FALSE, NULL, getv(widget->browser, MA_OWBBrowser_PrivateBrowsing));
			}
		}
		break;

		case MNA_NAVIGATION:
		{
			ULONG show;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_NAVIGATION, MUIA_Menuitem_Checked, &show);
			set(obj, MA_OWBWindow_ShowNavigationGroup, show);
			break;
		}

		case MNA_QUICKLINKS:
		{
			ULONG show;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_QUICKLINKS, MUIA_Menuitem_Checked, &show);
			set(obj, MA_OWBWindow_ShowQuickLinkGroup, show);
			break;
		}

		case MNA_LOCATION:
		{
			ULONG show;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_LOCATION, MUIA_Menuitem_Checked, &show);
			set(obj, MA_OWBWindow_ShowAddressBarGroup, show);
			break;
		}

		case MNA_STATUS:
		{
			ULONG show;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_STATUS, MUIA_Menuitem_Checked, &show);
			set(obj, MA_OWBWindow_ShowStatusGroup, show);
			break;
		}

		case MNA_FULLSCREEN:
		{
			ULONG fullscreen;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_STATUS, MUIA_Menuitem_Checked, &fullscreen);
			DoMethod(obj, MM_OWBWindow_FullScreen, fullscreen ? MV_OWBWindow_FullScreen_On : MV_OWBWindow_FullScreen_Off);
			break;
		}

		case MNA_NO_PANEL:
		{
			ULONG show;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_NO_PANEL, MUIA_Menuitem_Checked, &show);
			set(obj, MA_OWBWindow_ShowBookmarkPanel, FALSE);
			set(obj, MA_OWBWindow_ShowHistoryPanel, FALSE);
            DoMethod(obj, MM_OWBWindow_UpdatePanelGroup);

			break;
		}

		case MNA_BOOKMARK_PANEL:
		{
			ULONG show;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_BOOKMARK_PANEL, MUIA_Menuitem_Checked, &show);
			set(obj, MA_OWBWindow_ShowBookmarkPanel, show);
            DoMethod(obj, MM_OWBWindow_UpdatePanelGroup);

			break;
		}

		case MNA_HISTORY_PANEL:
		{
			ULONG show;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_HISTORY_PANEL, MUIA_Menuitem_Checked, &show);
			set(obj, MA_OWBWindow_ShowHistoryPanel, show);
            DoMethod(obj, MM_OWBWindow_UpdatePanelGroup);

			break;
		}

		case MNA_DOWNLOADS_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_Downloads, TRUE);
		}
		break;

		case MNA_NETWORK_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_Network);
		}
		break;

		case MNA_CONSOLE_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_Console);
		}
		break;

		case MNA_PASSWORDMANAGER_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_PasswordManager);
		}
		break;

		case MNA_COOKIEMANAGER_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_CookieManager);
		}
		break;

		case MNA_BLOCKMANAGER_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_BlockManager);
		}
		break;

		case MNA_SEARCHMANAGER_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_SearchManager);
		}
		break;

		case MNA_SCRIPTMANAGER_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_ScriptManager);
		}
		break;

		case MNA_URLSETTINGS_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_URLPrefs);
		}
		break;

		case MNA_BOOKMARKS_WINDOW:
		{
			DoMethod(app, MM_OWBApp_OpenWindow, MV_OWB_Window_Bookmarks);
		}
		break;

		case MNA_ADD_BOOKMARK:
		{
			DoMethod(obj, MM_OWBWindow_InsertBookmark);
		}
		break;

		case MNA_PRIVATE_BROWSING:
		{
			ULONG enabled;
			BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
			
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, msg->action, MUIA_Menuitem_Checked, &enabled);

			set(widget->browser, MA_OWBBrowser_PrivateBrowsing, enabled);
		}
		break;

		/* Per-tab settings */
		case MNA_SETTINGS_JAVASCRIPT_ENABLED:
		case MNA_SETTINGS_JAVASCRIPT_DISABLED:
		case MNA_SETTINGS_JAVASCRIPT_DEFAULT:
		{
			ULONG enabled;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, msg->action, MUIA_Menuitem_Checked, &enabled);

			if(enabled)
			{
				ULONG value;
				BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

				switch(msg->action)
				{
					case MNA_SETTINGS_JAVASCRIPT_ENABLED:
						value = JAVASCRIPT_ENABLED;
						break;
					case MNA_SETTINGS_JAVASCRIPT_DISABLED:
						value = JAVASCRIPT_DISABLED;
						break;
					default:
					case MNA_SETTINGS_JAVASCRIPT_DEFAULT:
						value = JAVASCRIPT_DEFAULT;
						break;
				}

				set(widget->browser, MA_OWBBrowser_JavaScriptEnabled, value);
			}
		}
		break;

		case MNA_SETTINGS_IMAGES_ENABLED:
		case MNA_SETTINGS_IMAGES_DISABLED:
		case MNA_SETTINGS_IMAGES_DEFAULT:
		{
			ULONG enabled;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, msg->action, MUIA_Menuitem_Checked, &enabled);

			if(enabled)
			{
				ULONG value;
				BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

				switch(msg->action)
				{
					case MNA_SETTINGS_IMAGES_ENABLED:
						value = IMAGES_ENABLED;
						break;
					case MNA_SETTINGS_IMAGES_DISABLED:
						value = IMAGES_DISABLED;
						break;
					default:
					case MNA_SETTINGS_IMAGES_DEFAULT:
						value = IMAGES_DEFAULT;
						break;
				}

				set(widget->browser, MA_OWBBrowser_LoadImagesAutomatically, value);
			}
		}
		break;

		case MNA_SETTINGS_ANIMATIONS_ENABLED:
		case MNA_SETTINGS_ANIMATIONS_DISABLED:
		case MNA_SETTINGS_ANIMATIONS_DEFAULT:
		{
			ULONG enabled;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, msg->action, MUIA_Menuitem_Checked, &enabled);

			if(enabled)
			{
				ULONG value;
				BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

				switch(msg->action)
				{
					case MNA_SETTINGS_ANIMATIONS_ENABLED:
						value = ANIMATIONS_ENABLED;
						break;
					case MNA_SETTINGS_ANIMATIONS_DISABLED:
						value = ANIMATIONS_DISABLED;
						break;
					default:
					case MNA_SETTINGS_ANIMATIONS_DEFAULT:
						value = ANIMATIONS_DEFAULT;
						break;
				}

				set(widget->browser, MA_OWBBrowser_PlayAnimations, value);
			}
		}
		break;

		case MNA_SETTINGS_PLUGINS_ENABLED:
		case MNA_SETTINGS_PLUGINS_DISABLED:
		case MNA_SETTINGS_PLUGINS_DEFAULT:
		{
			ULONG enabled;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, msg->action, MUIA_Menuitem_Checked, &enabled);

			if(enabled)
			{
				ULONG value;
				BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

				switch(msg->action)
				{
					case MNA_SETTINGS_PLUGINS_ENABLED:
						value = PLUGINS_ENABLED;
						break;
					case MNA_SETTINGS_PLUGINS_DISABLED:
						value = PLUGINS_DISABLED;
						break;
					default:
					case MNA_SETTINGS_PLUGINS_DEFAULT:
						value = PLUGINS_DEFAULT;
						break;
				}

				set(widget->browser, MA_OWBBrowser_PluginsEnabled, value);
			}
		}
		break;

		case MNA_SETTINGS_SPOOF_AS_DEFAULT:
		{
			ULONG enabled;
			DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, MNA_SETTINGS_SPOOF_AS_DEFAULT, MUIA_Menuitem_Checked, &enabled);

			if(enabled)
			{
				BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
				set(widget->browser, MA_OWBBrowser_UserAgent, USERAGENT_DEFAULT);
			}
		}
		break;

		/* Dynamic entries: bookmark/history/spoof */
		default:
		{
			if(msg->action > MNA_DUMMY) 
			{
				struct menu_entry *entry = (struct menu_entry *) msg->action;

				if(entry)
				{
					switch(entry->type)
					{
						// Just go to the URL
						case MENUTYPE_HISTORY:
						case MENUTYPE_BOOKMARK:
							DoMethod(obj, MM_OWBWindow_LoadURL, (STRPTR) entry->data, NULL);
							break;				  
						
						// Create a new browser for the URL
						case MENUTYPE_CLOSEDVIEW:
							DoMethod(app, MM_OWBApp_AddBrowser, obj, (STRPTR) entry->data, FALSE, NULL, FALSE, FALSE, TRUE);
							break;

						// Set the new user agent for the webview
						case MENUTYPE_SPOOF:
						{
							ULONG enabled;
							DoMethod((Object *)getv(obj, MUIA_Window_Menustrip), MUIM_GetUData, entry, MUIA_Menuitem_Checked, &enabled);

							if(enabled)
							{
		                        BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);
								set(widget->browser, MA_OWBBrowser_UserAgent, entry->index);
							}
						}
						break;

						default:
							;
					}
				}
			}
		}
	}

	return 0;
}

/* Navigation */
DEFSMETHOD(OWBWindow_LoadURL)
{
	GETDATA;

	BalWidget *widget;
	Object *browser = msg->browser ? (Object *) msg->browser : (Object *) data->active_browser;

	if (!browser)
	{
		return 0;
	}

	widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);

	if (widget && msg->url && *(msg->url))
	{
		Object *urlString = (Object *) getv(data->addressbargroup, MUIA_Popstring_String);
		String url = String((const char *) msg->url).stripWhiteSpace();
		URL kurl(ParsedURLString, url);
		size_t spacepos = url.find(' ');

		// Handle possible shortcuts
		if(spacepos != notFound)
		{
			ULONG res = FALSE;
			String shortcut = url.left(spacepos);
			String text = url.right(url.length() - (spacepos + 1));
			char *converted_shortcut = utf8_to_local(shortcut.utf8().data());
			char *converted_text     = utf8_to_local(text.utf8().data());

			if(converted_shortcut && converted_text)
			{
				res = DoMethod(data->searchgroup, MM_SearchBarGroup_LoadURLFromShortcut, converted_shortcut, converted_text);
			}

			free(converted_shortcut);
			free(converted_text);

			// If shortcut was handled, we stop here, since LoadURL will be called from there
			if(res)
			{
				return 0;
			}
		}

		// Close history and restore its global state (a bit heavy to reload whole list...)
		DoMethod((Object *) getv(data->addressbargroup, MA_AddressBarGroup_PopString), MUIM_Popstring_Close, FALSE);

		if(getv(app, MA_OWBApp_URLCompletionType) & MV_OWBApp_URLCompletionType_Popup)
		{
			DoMethod(data->addressbargroup, MM_AddressBarGroup_LoadHistory);
		}

		// Reset url marked text
		if(urlString)
		{
			nnset(urlString, MUIA_Textinput_MarkStart, 0);
			nnset(urlString, MUIA_Textinput_MarkEnd, - 1);
		}

		// Clear temporary frame names (really needed?)
		widget->webView->clearMainFrameName();

		// Handle protocols (any better place?)
		if(kurl.protocolIs("about"))
		{
			// XXX: duplicate from owbbrowser about: handling...
			OWBFile f("PROGDIR:resource/about.html");

			if(f.open('r') != -1)
			{
				char *fileContent = f.read(f.getSize());

				if(fileContent)
				{
					String content = fileContent;

					widget->webView->mainFrame()->loadHTMLString(content.utf8().data(), "about:");

					delete [] fileContent;
				}

				f.close();
			}
		}
		else if(kurl.string().startsWith("topsites"))
		{
			TopSitesManager::getInstance().generateTemplate(widget->webView, kurl.string());
		}
		else if(kurl.protocolIs("javascript"))
		{
			size_t scriptpos = kurl.string().find(':');
			if(scriptpos != notFound)
			{
				String script = kurl.string().substring(scriptpos+1);
				script = decodeURLEscapeSequences(script);
				widget->webView->executeScript(script.utf8().data());
			}
		}
		else if(kurl.protocolIs("data"))
		{
			widget->webView->mainFrame()->loadURL(url.utf8().data());
		}
		// Google for sentences or words
		else if(kurl.string().find(':') == notFound && (kurl.string().find(' ') != notFound || kurl.string().find('.') == notFound))
		{
			// Implement a LoadURLFromDefaultSearchEngine method in searchbargroup or so
			char *converted = local_to_utf8((char *) msg->url);
			if(converted)
			{
                String searchString = String::fromUTF8(converted);
				free(converted);
				searchString.stripWhiteSpace();
			    searchString = encodeWithURLEscapeSequences(searchString);
				searchString.replace("+", "%2B");
				searchString.replace("%20", "+");
				searchString.replace("&", "%26");
				widget->webView->mainFrame()->loadURL(String::format("http://www.google.com/search?ie=UTF-8&oe=UTF-8&sourceid=navclient&gfns=1&q=%s", searchString.utf8().data()).utf8().data());
			}
		}
		// Normal URL (at last)
		else
		{
			size_t pos = kurl.string().find(':');
			if(pos == notFound || pos > 5)
			{
				widget->webView->mainFrame()->loadURL(String("http://" + url).utf8().data());
			}
			else
			{
				widget->webView->mainFrame()->loadURL(url.utf8().data());
			}	  
		}
    }

	return 0;
}

DEFTMETHOD(OWBWindow_Back)
{
	GETDATA;

	BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

	widget->webView->clearMainFrameName();
	widget->webView->goBack();
	widget->expose = true;

	return 0;
}

DEFTMETHOD(OWBWindow_Forward)
{
	GETDATA;

	BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

	widget->webView->clearMainFrameName();
	widget->webView->goForward();
	widget->expose = true;

	return 0;
}

DEFTMETHOD(OWBWindow_Stop)
{
	GETDATA;

	BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

	core(widget->webView->mainFrame())->loader().stopAllLoaders();
	widget->expose = true;

	return 0;
}

DEFTMETHOD(OWBWindow_Home)
{
	DoMethod(obj, MM_OWBWindow_LoadURL, (STRPTR) getv(app, MA_OWBApp_DefaultURL), NULL);
	return 0;
}

DEFSMETHOD(OWBWindow_Reload)
{
	GETDATA;

	BalWidget *widget = (BalWidget *) getv(msg->browser?:data->active_browser, MA_OWBBrowser_Widget);

	widget->webView->clearMainFrameName();
	widget->webView->mainFrame()->reload();
	widget->expose = true;

	return 0;
}

DEFTMETHOD(OWBWindow_Close)
{
	GETDATA;

	if(data->pagetitles)
	{
		APTR n;
		ULONG count = 0;

		ITERATELIST(n, &window_list)
		{
			count++;
		}

		if(count > 1 && getv(data->pagetitles, MUIA_Group_ChildCount) > 1) // Is the pagetitle count check needed?
		{
			if (!MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_REQUESTER_NO_YES), GSI(MSG_OWBWINDOW_CLOSE_ALL_PAGES_CONFIRMATION), NULL))
				return 0;
		}
		else if(count == 1)
		{
			DoMethod(app, MM_OWBApp_Quit);
			return 0;
		}

		DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveWindow, obj);
	}

	return 0;
}

DEFTMETHOD(OWBWindow_OpenLocalFile)
{
	struct TagItem tags[] =
	{
        { ASLFR_TitleText, (STACKIPTR) GSI(MSG_OWBWINDOW_OPEN_FILE) },
        { ASLFR_InitialPattern, (STACKIPTR) "#?" },
        { TAG_DONE, TAG_DONE }
	};

	char *file = asl_run((char *)getv(app, MA_OWBApp_CurrentDirectory), (struct TagItem *) &tags, TRUE);

	if(file)
	{
		ULONG size = strlen(file) + strlen("file:///") + 1;
		char *uri = (char *) malloc(size);

		if(uri)
		{
			stccpy(uri, "file:///", size);
			strncat(uri, file, size);

			DoMethod(obj, MM_OWBWindow_LoadURL, uri, NULL);
			free(uri);
		}
		FreeVecTaskPooled(file);
	}
	return 0;
}

DEFTMETHOD(OWBWindow_SaveAsSource)
{
	GETDATA;
    BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

	if(widget)
	{
		WebDataSource* dataSource = widget->webView->mainFrame()->dataSource();

		if(dataSource && !dataSource->isLoading())
		{
			char buffer[100];
			URL url(ParsedURLString, (char *)getv(data->active_browser, MA_OWBBrowser_URL));
			char *name = strdup(decodeURLEscapeSequences(url.lastPathComponent()).latin1().data());

			if(name)
			{
				if(!*name)
				{
					snprintf(buffer, sizeof(buffer), "%s.html", (char *)getv(data->active_browser, MA_OWBBrowser_Title));
				}
				else
				{
					if(!strstr(name, ".php") && !strstr(name, ".html") && !strstr(name, ".asp") && !strstr(name, ".htm"))
					{
						snprintf(buffer, sizeof(buffer), "%s.html", name);
					}
					else
					{
						stccpy(buffer, name, sizeof(buffer));
					}
				}

                struct TagItem tags[] =
                {
                    { ASLFR_TitleText, (STACKIPTR) GSI(MSG_OWBWINDOW_SAVE_AS) },
                    { ASLFR_InitialPattern, (STACKIPTR) "#?" },
                    { ASLFR_InitialFile, (STACKIPTR) buffer },
                    { ASLFR_DoSaveMode, (STACKIPTR) TRUE },
                    { TAG_DONE, TAG_DONE }
                };

				char *file = asl_run((char *)getv(app, MA_OWBApp_CurrentDirectory), (struct TagItem *) &tags, TRUE);

				if(file)
				{
					FILE *f = fopen(file, "w");

					if(f)
					{
						fwrite(dataSource->data()->data(), dataSource->data()->size(), 1, f);
						fclose(f);
					}

					FreeVecTaskPooled(file);
				}

				free(name);
			}
		}
	}

	return 0;
}

DEFTMETHOD(OWBWindow_SaveAsPDF)
{
	GETDATA;
	char buffer[100];
	URL url(ParsedURLString, (char *)getv(data->active_browser, MA_OWBBrowser_URL));
	char *name = strdup(decodeURLEscapeSequences(url.lastPathComponent()).latin1().data());

	if(name)
	{
		if(!*name)
		{
			snprintf(buffer, sizeof(buffer), "%s.pdf", (char *)getv(data->active_browser, MA_OWBBrowser_Title));
		}
		else
		{
			snprintf(buffer, sizeof(buffer), "%s.pdf", name);
		}

        struct TagItem tags[] =
        {
            { ASLFR_TitleText, (STACKIPTR)GSI(MSG_OWBWINDOW_SAVE_AS) },
            { ASLFR_InitialPattern, (STACKIPTR)"#?.pdf" },
            { ASLFR_InitialFile, (STACKIPTR)buffer },
            { ASLFR_DoSaveMode, (STACKIPTR)TRUE },
            { TAG_DONE, TAG_DONE }
        };

		char *file = asl_run((char *)getv(app, MA_OWBApp_CurrentDirectory), (struct TagItem *) &tags, TRUE);

		if(file)
		{
			DoMethod(data->active_browser, MM_OWBBrowser_Print, file);

			FreeVecTaskPooled(file);
		}

		free(name);
	}

	return 0;
}

DEFTMETHOD(OWBWindow_Print)
{
	GETDATA;

    BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

	if(widget)
	{
		DoMethod((Object *) getv(app, MA_OWBApp_PrinterWindow), MM_PrinterWindow_PrintDocument, core(widget->webView->mainFrame()));
	}

	return 0;
}

// Just some helpers
static Object * get_group_from_browser(Object *browser)
{
	return (Object *) _parent(_parent(browser));
}

static LONG get_tab_position(struct Data *data, Object *browser)
{
	LONG idx = 0;

	FORCHILD(data->pagegroup, MUIA_Group_ChildList )
	{
		if (data->pagetitles == child)
			continue;

		if((Object *)child == get_group_from_browser(browser))
		{
			break;
		}
		idx++;
	}
	NEXTCHILD

	return idx;
}

static Object* get_browser_to_insert_after(struct Data *data, Object *browser)
{
	Object *result_browser = NULL;
	Object *parent_browser = (Object *) getv(browser, MA_OWBBrowser_ParentBrowser);	
	
	// Check all browsers having the same parents as this browser, and return the last of them	
	if(parent_browser)
	{
		FORCHILD(data->pagegroup, MUIA_Group_ChildList)
		{
			if (data->pagetitles == child)
				continue;

			Object *iterated_browser = (Object *) getv(child, MA_OWBGroup_Browser);
			
			if(iterated_browser)
			{
				// Found the parent
				if(iterated_browser == parent_browser)
				{
					result_browser = iterated_browser;
				}
				// Found a tab having the same parent
				else
				{
					Object *parent_of_iterated_browser = (Object *) getv(iterated_browser, MA_OWBBrowser_ParentBrowser);
					if(parent_of_iterated_browser == parent_browser)
					{
						result_browser = iterated_browser;
					}
				}
			}
		}
		NEXTCHILD
	}
	
	// If none can be found, return last browser
	if(!result_browser)
	{
		Object *group = (Object *) DoMethod(data->pagegroup, MUIM_Family_GetChild, MUIV_Family_GetChild_Last);
		if(group)
		{
			result_browser = (Object *) getv(group, MA_OWBGroup_Browser);
		}
	}

	return result_browser;
}

static void invalidate_parent(struct Data *data, Object *browser)
{
	// Check all browsers and invalidate parentbrowser for the ones having this browser as parent

	FORCHILD(data->pagegroup, MUIA_Group_ChildList)
	{
		if (data->pagetitles == child)
			continue;

		Object *iterated_browser = (Object *) getv(child, MA_OWBGroup_Browser);
		
		if(iterated_browser)
		{
			Object *parent_of_iterated_browser = (Object *) getv(iterated_browser, MA_OWBBrowser_ParentBrowser);
			if(parent_of_iterated_browser == browser)
			{
				set(iterated_browser, MA_OWBBrowser_ParentBrowser, NULL);
			}
		}
	}
	NEXTCHILD
}

DEFSMETHOD(OWBWindow_AddBrowser)
{
	GETDATA;

	//ULONG add_after_active = FALSE;
	ULONG add_after_active = !msg->addtoend && data->active_browser != NULL && (getv(app, MA_OWBApp_NewPagePosition) == MV_OWBApp_NewPagePosition_After_Active);

	BalWidget *widget = NULL;

	Object *title = (Object *)NewObject(gettitlelabelclass(), NULL,
								MUIA_UserData, NULL,
								TAG_DONE);

	if(title)
	{
		Object *hbar, *vbar, *hbargroup, *vbargroup, *mediacontrolsgroup, *inspectorgroup = 0;
		Object *group;
		Object *browser = create_browser((char *)msg->url, msg->isframe, title, msg->sourceview, obj, msg->privatebrowsing);

		group = (Object *) NewObject(getowbgroupclass(), NULL,
					Child,
						vbargroup = HGroup,
							InnerSpacing(0, 0),
							GroupSpacing(0),
							Child, browser,
							Child, vbar = ScrollbarObject, MUIA_Group_Horiz, FALSE, End,
							End,
					Child,
						hbargroup = HGroup,
							InnerSpacing(0, 0),
							GroupSpacing(0),
							Child, hbar = ScrollbarObject, MUIA_Group_Horiz, TRUE, End,
							End,
					Child,
						mediacontrolsgroup = (Object *) NewObject(getmediacontrolsgroupclass(), NULL, MUIA_ShowMe, FALSE, TAG_DONE),
					
					Child,
						inspectorgroup = GroupObject, MUIA_ShowMe, FALSE, End,
					

				TAG_DONE);

		if(group)
		{
			set(group, MA_OWBGroup_Browser, browser);
			set(group, MA_OWBGroup_MediaControlsGroup, mediacontrolsgroup);
			set(group, MA_OWBGroup_InspectorGroup, inspectorgroup);

			setup_scrollbars_notifications(browser, vbar, hbar, vbargroup, hbargroup);

			DoMethod(_parent(data->pagegroup), MUIM_Group_InitChange);
			DoMethod(data->pagegroup, MUIM_Group_InitChange);
			DoMethod(data->pagetitles, MUIM_Group_InitChange);

			if(add_after_active)
			{
				set(browser, MA_OWBBrowser_ParentBrowser, data->active_browser);

				//DoMethod(data->pagetitles, MUIM_Group_Insert, title, getv(data->active_browser, MA_OWBBrowser_TitleObj));
				//DoMethod(data->pagegroup, MUIM_Group_Insert, group, get_group_from_browser(data->active_browser));

				// TODO: loop over browsers, and insert after the last one having active_browser as parent
				Object *browser_to_insert_after = get_browser_to_insert_after(data, browser);
				DoMethod(data->pagetitles, MUIM_Group_Insert, title, getv(browser_to_insert_after, MA_OWBBrowser_TitleObj));
				DoMethod(data->pagegroup, MUIM_Group_Insert, group, get_group_from_browser(browser_to_insert_after));				
			}
			else
			{
				set(browser, MA_OWBBrowser_ParentBrowser, NULL);

				DoMethod(data->pagetitles, MUIM_Group_AddTail, title);
				DoMethod(data->pagegroup, MUIM_Group_AddTail, group);
			}

			DoMethod(data->pagetitles, MUIM_Group_ExitChange);
			DoMethod(data->pagegroup, MUIM_Group_ExitChange);
			DoMethod(_parent(data->pagegroup), MUIM_Group_ExitChange);

			if(msg->donotactivate == FALSE)
			{
				//set(data->pagegroup, MUIA_Group_ActivePage, add_after_active ? MUIV_Group_ActivePage_Next : MUIV_Group_ActivePage_Last);

				// TODO: activate the idx resulting from the insertion
				set(data->pagegroup, MUIA_Group_ActivePage, add_after_active ? get_tab_position(data, browser) : MUIV_Group_ActivePage_Last);
			}

            widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
		}
	}

	return (ULONG) widget;
}

DEFSMETHOD(OWBWindow_RemoveBrowser)
{
	GETDATA;
	ULONG count = getv(data->pagetitles, MUIA_Group_ChildCount);

	if(!msg->browser)
	{
		return 0;
	}

	// Honour private browsing and don't record closed view in that mode
	if(!getv(msg->browser, MA_OWBBrowser_PrivateBrowsing))
	{
		DoMethod(obj, MM_OWBWindow_AddClosedView, msg->browser);
	}

	char* resetvm = getenv("OWB_RESETVM");
	/*
	if(!resetvm)
	{
	    if(count <= 1)
	    {
		DoMethod(obj, MM_OWBWindow_AddBrowser, "", FALSE, NULL, TRUE, FALSE, TRUE);
	    }
	}
	*/

	if (!resetvm && count <= 1)
	{
	    DoMethod(obj, MM_OWBWindow_Close);
	}
	else
	{
		Object *group =	get_group_from_browser(msg->browser);
		Object *title = (Object *) getv(msg->browser, MA_OWBBrowser_TitleObj);
		ULONG idx = get_tab_position(data, msg->browser);

		set(msg->browser, MA_OWBBrowser_Loading, FALSE);
		cleanup_browser_notifications(obj, data, msg->browser);

		DoMethod(_parent(data->pagegroup), MUIM_Group_InitChange);

		DoMethod(data->pagegroup, MUIM_Group_InitChange);

		DoMethod(data->pagegroup, OM_REMMEMBER, group);

		DoMethod(data->pagetitles, MUIM_Group_InitChange);
		DoMethod(data->pagetitles, OM_REMMEMBER, title);

		MUI_DisposeObject(title);
		MUI_DisposeObject(group);

		data->active_browser = NULL;

		// TODO: Iterate browsers and clear ParentBrowser for the one having msg->browser as parent
		invalidate_parent(data, msg->browser);

		if(idx < data->lastpagenum)
			idx = data->lastpagenum - 1;
		else
			idx = data->lastpagenum;

		set(data->pagegroup, MUIA_Group_ActivePage, std::min(idx, count - 2));

		DoMethod(data->pagetitles, MUIM_Group_ExitChange);
		DoMethod(data->pagegroup, MUIM_Group_ExitChange);
		DoMethod(_parent(data->pagegroup), MUIM_Group_ExitChange);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_DetachBrowser)
{
	GETDATA;
	ULONG count = getv(data->pagetitles, MUIA_Group_ChildCount);

	if(!msg->browser)
	{
		return 0;
	}

	// Avoid detaching a singletab window to a new one
	if ((count > 0 && msg->window) || (count > 1))
	{
		Object *group =	get_group_from_browser(msg->browser);
		Object *title = (Object *) getv(msg->browser, MA_OWBBrowser_TitleObj);
		ULONG idx = get_tab_position(data, msg->browser);

		set(msg->browser, MA_OWBBrowser_Loading, FALSE);
		cleanup_browser_notifications(obj, data, msg->browser);

		DoMethod(_parent(data->pagegroup), MUIM_Group_InitChange);

		DoMethod(data->pagegroup, MUIM_Group_InitChange);

		DoMethod(data->pagegroup, OM_REMMEMBER, group);

		DoMethod(data->pagetitles, MUIM_Group_InitChange);
		DoMethod(data->pagetitles, OM_REMMEMBER, title);

		data->active_browser = NULL;

		// TODO: Iterate browsers and clear ParentBrowser for the one having msg->browser as parent
		invalidate_parent(data, msg->browser);

		if(count > 1)
		{
			if(idx < data->lastpagenum)
				idx = data->lastpagenum - 1;
			else
				idx = data->lastpagenum;

			set(data->pagegroup, MUIA_Group_ActivePage, std::min(idx, count - 2));
		}

		DoMethod(data->pagetitles, MUIM_Group_ExitChange);
		DoMethod(data->pagegroup, MUIM_Group_ExitChange);
		DoMethod(_parent(data->pagegroup), MUIM_Group_ExitChange);

        // It was the last browser, so close the window
		if(count == 1)
		{
			DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveWindow, obj);
		}

		// Create a new window with the existing browser, or transfer it to the target window
		if(!msg->window)
		{
			Object *window = (Object *) NewObject(getowbwindowclass(), NULL,
										MA_OWBWindow_InitialBrowser, (ULONG) msg->browser,
										TAG_DONE);

			if(window)
			{
                DoMethod(app, OM_ADDMEMBER, window);
                set(window, MUIA_Window_Open, TRUE);
			}
		
		}
		else
		{
			DoMethod(msg->window, MM_OWBWindow_TransferBrowser, msg->browser);
		}
	}

	return 0;
}

DEFSMETHOD(OWBWindow_TransferBrowser)
{
	GETDATA;

	Object *browser = (Object *) msg->browser;

	if(browser)
	{
		Object *group = get_group_from_browser(browser);
		Object *title = (Object *) getv(browser, MA_OWBBrowser_TitleObj);

		DoMethod(_parent(data->pagegroup), MUIM_Group_InitChange);
		DoMethod(data->pagegroup, MUIM_Group_InitChange);
		DoMethod(data->pagetitles, MUIM_Group_InitChange);

		set(browser, MA_OWBBrowser_ParentBrowser, NULL);

		DoMethod(data->pagetitles, MUIM_Group_AddTail, title);
		DoMethod(data->pagegroup, MUIM_Group_AddTail, group);

		DoMethod(data->pagetitles, MUIM_Group_ExitChange);
		DoMethod(data->pagegroup, MUIM_Group_ExitChange);
		DoMethod(_parent(data->pagegroup), MUIM_Group_ExitChange);

		set(data->pagegroup, MUIA_Group_ActivePage, MUIV_Group_ActivePage_Last);
	}

	return 0;
}

DEFTMETHOD(OWBWindow_InspectPage)
{
	GETDATA;
	BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

	if(widget)
	{
		if (Page* page = widget->webView->page())
		{
			page->inspectorController().show();
		}
	}

	return 0;
}

DEFSMETHOD(OWBWindow_DestroyInspector)
{
	Object *inspectedbrowser = msg->browser;
	Object *parentgroup = get_group_from_browser(inspectedbrowser);
	Object *inspectorgroup = (Object *) getv(parentgroup, MA_OWBGroup_InspectorGroup);
	Object *group = (Object *) DoMethod(inspectorgroup, MUIM_Family_GetChild, 0);

	DoMethod(inspectorgroup, MUIM_Group_InitChange);
	DoMethod(inspectorgroup, OM_REMMEMBER, group);
	DoMethod(inspectorgroup, MUIM_Group_ExitChange);

	MUI_DisposeObject(group);

	return 0;
}

DEFSMETHOD(OWBWindow_CreateInspector)
{
	Object *inspectedbrowser = msg->browser;
	BalWidget *widget = NULL;

	Object *hbar, *vbar, *hbargroup, *vbargroup;
	Object *group;
	Object *browser = create_browser((char *)"file:///PROGDIR:webinspector/inspector.html", FALSE, NULL, NULL, obj, FALSE);

	if(browser)
	{
		group = (Object *) NewObject(getowbgroupclass(), NULL,
					Child,
						vbargroup = HGroup,
							InnerSpacing(0, 0),
							GroupSpacing(0),
							Child, browser,
							Child, vbar = ScrollbarObject, MUIA_Group_Horiz, FALSE, End,
							End,
					Child,
						hbargroup = HGroup,
							InnerSpacing(0, 0),
							GroupSpacing(0),
							Child, hbar = ScrollbarObject, MUIA_Group_Horiz, TRUE, End,
							End,
				TAG_DONE);

		if(group)
		{
			Object *parentgroup = get_group_from_browser(inspectedbrowser);
			Object *inspectorgroup = (Object *) getv(parentgroup, MA_OWBGroup_InspectorGroup);

			set(inspectorgroup, MUIA_ShowMe, TRUE);

			set(group, MA_OWBGroup_Browser, browser);
			set(obj, MA_OWBWindow_ActiveWebInspector, browser);

			setup_scrollbars_notifications(browser, vbar, hbar, vbargroup, hbargroup);

			DoMethod(inspectorgroup, MUIM_Group_InitChange);
			DoMethod(inspectorgroup, MUIM_Group_AddTail, group);
			DoMethod(inspectorgroup, MUIM_Group_ExitChange);


	        widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
		}
	}

	return (ULONG) widget;
	
	//return 0;
}

DEFSMETHOD(OWBWindow_ActivePage)
{
	GETDATA;
	ULONG idx = 0;

	data->lastpagenum = msg->pagenum;

	//kprintf("OWBWindow: active page %d\n", msg->pagenum);

	FORCHILD(data->pagegroup, MUIA_Group_ChildList)
	{
		if (data->pagetitles == child)
			continue;

		if (msg->pagenum == idx)
		{
			Object *old = data->active_browser;

			// No need to react on ourselves (see if it doesn't cause regressions)
			if (old == (Object *) getv((Object *)child, MA_OWBGroup_Browser))
			{
				return 0;
			}

			if (old)
			{
				cleanup_browser_notifications(obj, data, old);
			}

			data->active_browser = (Object *) getv((Object *)child, MA_OWBGroup_Browser);

			//kprintf("OWBWindow: active browser = %p\n", data->active_browser);
			//kprintf("OWBWindow: loading state: %d\n", getv(data->active_browser, MA_OWBBrowser_Loading));

			setup_browser_notifications(obj, data, data->active_browser);

			DoMethod(obj, MM_OWBWindow_UpdateTitle,           data->active_browser, getv(data->active_browser, MA_OWBBrowser_Title));
			DoMethod(obj, MM_OWBWindow_UpdateURL,             data->active_browser, getv(data->active_browser, MA_OWBBrowser_URL));
			//DoMethod(obj, MM_OWBWindow_UpdateURL,           data->active_browser, getv(data->active_browser, MA_OWBBrowser_EditedURL));
			DoMethod(obj, MM_OWBWindow_UpdateZone,            data->active_browser, getv(data->active_browser, MA_OWBBrowser_Zone));
			DoMethod(obj, MM_OWBWindow_UpdateSecurity,        data->active_browser, getv(data->active_browser, MA_OWBBrowser_Security));
			DoMethod(obj, MM_OWBWindow_UpdatePrivateBrowsing, data->active_browser, getv(data->active_browser, MA_OWBBrowser_PrivateBrowsing));
			DoMethod(obj, MM_OWBWindow_UpdateStatus,          data->active_browser, getv(data->active_browser, MA_OWBBrowser_StatusText));
			DoMethod(obj, MM_OWBWindow_UpdateStatus,          data->active_browser, getv(data->active_browser, MA_OWBBrowser_ToolTipText));
			DoMethod(obj, MM_OWBWindow_UpdateNavigation,      data->active_browser);
			DoMethod(obj, MM_OWBWindow_UpdateMenu,            data->active_browser);

			SetAttrs(data->navigationgroup,
					 MA_TransferAnim_Animate, getv(data->active_browser, MA_OWBBrowser_Loading),
				     MA_Navigation_BackForwardList, (ULONG) ((WebView *) ((BalWidget *)getv(data->active_browser, MA_OWBBrowser_Widget))->webView)->backForwardList(),
			         TAG_DONE);

			SetAttrs(data->progressgroup, MUIA_Group_ActivePage, getv(data->active_browser, MA_OWBBrowser_Loading) ? 1 : 0, TAG_DONE);

			//SetAttrs(data->active_browser, MA_OWBBrowser_Active, TRUE, TAG_DONE);
			break;
		}

		idx++;
	}
	NEXTCHILD

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateTitle)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		snprintf(data->windowtitle, sizeof(data->windowtitle), "OWB: %s", msg->title);
		set(obj, MUIA_Window_Title, data->windowtitle);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateNavigation)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		SetAttrs(data->navigationgroup,
				 MA_Navigation_BackEnabled,     getv(data->active_browser, MA_OWBBrowser_BackAvailable),
				 MA_Navigation_ForwardEnabled,  getv(data->active_browser, MA_OWBBrowser_ForwardAvailable),
				 MA_Navigation_ReloadEnabled,   getv(data->active_browser, MA_OWBBrowser_ReloadAvailable),
				 MA_Navigation_StopEnabled,     getv(data->active_browser, MA_OWBBrowser_StopAvailable),
		         TAG_DONE);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateURL)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		// Don't trigger a notify there
		nnset(data->addressbargroup, MUIA_String_Contents, msg->url);
		DoMethod(data->addressbargroup, MM_AddressBarGroup_Mark, msg->url);
		DoMethod(obj, MM_OWBWindow_UpdateUserScript, msg->browser, msg->url);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateStatus)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		set(data->statusbar, MUIA_Text_Contents, msg->status);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateProgress)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		BalWidget *widget = (BalWidget *) getv(msg->browser, MA_OWBBrowser_Widget);

		if(widget)
		{
			char received[128], total[128];

			format_size(received, sizeof(received), (QUAD) (widget->webView->page()->progress().totalBytesReceived()));
			format_size(total,    sizeof(total),    (QUAD) (widget->webView->page()->progress().totalPageAndResourceBytesToLoad()));
			snprintf(data->progressbuffer, sizeof(data->progressbuffer), GSI(MSG_OWBWINDOW_READ_PROGRESS), received, total);

			SetAttrs(data->progressgauge,
					 MUIA_Gauge_InfoText, data->progressbuffer,
					 MUIA_Gauge_Current, getv(msg->browser, MA_OWBBrowser_LoadingProgress),
					 TAG_DONE);
		}
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateState)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		char *state = NULL;

		switch(msg->state)
		{
			default:
			case MV_OWBBrowser_State_Ready:
				state = GSI(MSG_OWBWINDOW_STATE_READY);
				break;
			case MV_OWBBrowser_State_Loading:
				state =	GSI(MSG_OWBWINDOW_STATE_RECEIVING_DATA);
				break;
			case MV_OWBBrowser_State_Error:
				state =	GSI(MSG_OWBWINDOW_STATE_ERROR);
				break;
			case MV_OWBBrowser_State_Connecting:
				state =	GSI(MSG_OWBWINDOW_STATE_SENDING_REQUEST);
				break;
		}

		set(data->statusbar, MUIA_Text_Contents, state);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateZone)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		char *image;

		switch(msg->zone)
		{
			default:
			case MV_OWBBrowser_Zone_Local:
				image = "PROGDIR:resource/zone_local.png";
				break;
			case MV_OWBBrowser_Zone_Internet:
				image = "PROGDIR:resource/zone_internet.png";
				break;
		}

		set(data->zoneimage, MUIA_Dtpic_Name, image);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateSecurity)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		char *image;
		ULONG alpha = 0xFF;

		switch(msg->security)
		{
			default:
			case MV_OWBBrowser_Security_None:
				alpha = 0;
				image = "PROGDIR:resource/zone_notsecure.png"; // Just use it as fully transparent placeholder, so that layout is done
				break;
			case MV_OWBBrowser_Security_Secure:
				image = "PROGDIR:resource/zone_secure.png";
				break;
			case MV_OWBBrowser_Security_NotSecure:
				image = "PROGDIR:resource/zone_notsecure.png";
				break;
		}

		set(data->secureimage, MUIA_Dtpic_Alpha, alpha);
		set(data->secureimage, MUIA_Dtpic_Name, image);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdatePrivateBrowsing)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
#if !OS(AROS)
		char *image = "PROGDIR:resource/private_browsing.png";;
#else
        char *image = "PROGDIR:resource/private_browsing_a.png";;
#endif
		ULONG alpha = msg->enabled ?  0xFF : 0;

		set(data->privatebrowsingimage, MUIA_Dtpic_Alpha, alpha);
		set(data->privatebrowsingimage, MUIA_Dtpic_Name, image);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateUserScript)
{
	GETDATA;

	if(msg->browser == data->active_browser)
	{
		Vector<ScriptEntry *> *scripts = (Vector<ScriptEntry *> *) DoMethod((Object *) getv(app, MA_OWBApp_ScriptManagerWindow), MM_ScriptManagerGroup_ScriptsForURL, msg->url);	

		String shorthelp = "";
		
		if(scripts->size())
		{
			shorthelp = String(GSI(MSG_OWBWINDOW_RUNNING_USERSCRIPTS));
		}
		
		for(size_t i = 0; i < scripts->size(); i++)
		{
			shorthelp = shorthelp +  String(" - ") + (*scripts)[i]->title;
			
			if(i < scripts->size() - 1)
				shorthelp = shorthelp + String("\n");
		}
		
		free(data->userscriptimage_shorthelp);
		data->userscriptimage_shorthelp = utf8_to_local(shorthelp.utf8().data());
		set(data->userscriptimage, MUIA_ShortHelp, data->userscriptimage_shorthelp);

		char *image = "PROGDIR:resource/userscript.png";
		ULONG alpha = scripts->size() ?  0xFF : 0;

		set(data->userscriptimage, MUIA_Dtpic_Alpha, alpha);
		set(data->userscriptimage, MUIA_Dtpic_Name, image);
		
		delete scripts;
	}

	return 0;
}

DEFTMETHOD(OWBWindow_BuildSpoofMenu)
{
	ULONG i = 0;
	Object *item;
	Object *strip = (Object *) getv(obj, MUIA_Window_Menustrip);
	Object *menu  = (Object *) DoMethod(strip, MUIM_FindUData, MNA_SETTINGS_SPOOF_AS);
	CONST_STRPTR * labels = get_user_agent_labels();
	CONST_STRPTR * agents = get_user_agent_strings();

	DoMethod(menu, MUIM_Menustrip_InitChange);

	while(*labels)
	{
		ULONG bits = 0;
		ULONG count = get_user_agent_count() + 1; /* +1 for default entry */
        struct menu_entry *menu_entry;

		while(count--)
		{
			if(count != i)
			{
				bits |= 1 << count;
			}
		}

		menu_entry = (struct menu_entry *) malloc(sizeof(*menu_entry) + strlen(*agents) + 1);

		if(menu_entry)
		{
			menu_entry->type = MENUTYPE_SPOOF;
			menu_entry->index = i,
			stccpy(menu_entry->data, *agents, strlen(*agents) + 1); // XXX: We should just use index here

			item = (Object *) NewObject(getmenuitemclass(), NULL,
				MUIA_Menuitem_Title, strdup(*labels),
				MUIA_UserData,       menu_entry,
				MUIA_Menuitem_Checkit, TRUE,
				MUIA_Menuitem_Checked, FALSE,
				End;

			if(item)
			{
				set(item, MUIA_Menuitem_Exclude, bits);
				DoMethod(menu, MUIM_Family_AddTail, item);
			}
		}

		labels++;
		agents++;
		i++;
	}

	item = (Object *) NewObject(getmenuitemclass(), NULL,
		MUIA_Menuitem_Title, strdup(GSI(MSG_MENU_DEFAULT)),
		MUIA_UserData, MNA_SETTINGS_SPOOF_AS_DEFAULT,
		MA_MenuItem_FreeUserData, FALSE, // Don't free that one, thanks. :)
		MUIA_Menuitem_Checkit, TRUE,
		MUIA_Menuitem_Checked, TRUE,
		End;

	if(item)
	{
		ULONG bits = 0;
		ULONG count = get_user_agent_count() + 1 - 1; /* +1 for default entry, -1 to not exclude us */

		while(count--)
		{
			bits |= 1 << count;
		}

        set(item, MUIA_Menuitem_Exclude, bits);

		DoMethod(menu, MUIM_Family_AddTail, item);
	}

	DoMethod(menu, MUIM_Menustrip_ExitChange);

	return 0;
}

DEFSMETHOD(OWBWindow_UpdateMenu)
{
	BalWidget *widget = (BalWidget *) getv(msg->browser, MA_OWBBrowser_Widget);

	if(widget)
	{
		Object *strip = (Object *) getv(obj, MUIA_Window_Menustrip);
		Object *menu = NULL;
		ULONG userdata = 0;

		DoMethod(strip, MUIM_Menustrip_InitChange);

		// Private browsing

		DoMethod(strip, MUIM_SetUData, MNA_PRIVATE_BROWSING, MUIA_Menuitem_Checked, getv(widget->browser, MA_OWBBrowser_PrivateBrowsing));

		// Plugin settings

		switch(getv(widget->browser, MA_OWBBrowser_PluginsEnabled))
		{
			default:
			case PLUGINS_DEFAULT:
				userdata = MNA_SETTINGS_PLUGINS_DEFAULT;
				break;
			case PLUGINS_ENABLED:
				userdata = MNA_SETTINGS_PLUGINS_ENABLED;
				break;
			case PLUGINS_DISABLED:
				userdata = MNA_SETTINGS_PLUGINS_DISABLED;
				break;
		}

		DoMethod(strip, MUIM_SetUData, userdata, MUIA_Menuitem_Checked, TRUE);

		// Image settings

		switch(getv(widget->browser, MA_OWBBrowser_LoadImagesAutomatically))
		{
			default:
			case IMAGES_DEFAULT:
				userdata = MNA_SETTINGS_IMAGES_DEFAULT;
				break;
			case IMAGES_ENABLED:
				userdata = MNA_SETTINGS_IMAGES_ENABLED;
				break;
			case IMAGES_DISABLED:
				userdata = MNA_SETTINGS_IMAGES_DISABLED;
				break;
		}

		DoMethod(strip, MUIM_SetUData, userdata, MUIA_Menuitem_Checked, TRUE);

		/*
		// Animation settings

		switch(getv(widget->browser, MA_OWBBrowser_PlayAnimations))
		{
			default:
			case ANIMATIONS_DEFAULT:
				userdata = MNA_SETTINGS_ANIMATIONS_DEFAULT;
				break;
			case ANIMATIONS_ENABLED:
				userdata = MNA_SETTINGS_ANIMATIONS_ENABLED;
				break;
			case ANIMATIONS_DISABLED:
				userdata = MNA_SETTINGS_ANIMATIONS_DISABLED;
				break;
		}

		DoMethod(strip, MUIM_SetUData, userdata, MUIA_Menuitem_Checked, TRUE);
		*/

		// Javascript settings

		switch(getv(widget->browser, MA_OWBBrowser_JavaScriptEnabled))
		{
			default:
			case JAVASCRIPT_DEFAULT:
				userdata = MNA_SETTINGS_JAVASCRIPT_DEFAULT;
				break;
			case JAVASCRIPT_ENABLED:
				userdata = MNA_SETTINGS_JAVASCRIPT_ENABLED;
				break;
			case JAVASCRIPT_DISABLED:
				userdata = MNA_SETTINGS_JAVASCRIPT_DISABLED;
				break;
		}

		DoMethod(strip, MUIM_SetUData, userdata, MUIA_Menuitem_Checked, TRUE);

		// User agent settings

		ULONG found = FALSE;
		int index = getv(widget->browser, MA_OWBBrowser_UserAgent);
		String useragent = (index != USERAGENT_DEFAULT) ? get_user_agent_strings()[index] : "";
		menu = (Object *) DoMethod(strip, MUIM_FindUData, MNA_SETTINGS_SPOOF_AS);

		FORCHILD(menu, MUIA_Family_List)
		{
			struct menu_entry *menu_entry = (struct menu_entry *) muiUserData(child);

			if(menu_entry && ((LONG) menu_entry > MNA_DUMMY) && menu_entry->type == MENUTYPE_SPOOF)
			{
				if(useragent == String((char *) menu_entry->data))
				{
					DoMethod(strip, MUIM_SetUData, menu_entry, MUIA_Menuitem_Checked, TRUE);
					found = TRUE;
					break;
				}
			}
		}
		NEXTCHILD;

		if(!found)
		{
			DoMethod(strip, MUIM_SetUData, MNA_SETTINGS_SPOOF_AS_DEFAULT, MUIA_Menuitem_Checked, TRUE);
		}

		DoMethod(strip, MUIM_Menustrip_ExitChange);
	}

	return 0;
}

DEFSMETHOD(Search)
{
	GETDATA;

	BalWidget *widget = (BalWidget *) getv(data->active_browser, MA_OWBBrowser_Widget);

	if(widget)
	{
		char *converted = local_to_utf8(msg->string);
		if(converted)
		{
			bool res = false;
            String searchString = String::fromUTF8(converted);

			if(msg->flags & MV_FindText_MarkOnly)
			{
				res = widget->webView->page()->markAllMatchesForText(searchString, (msg->flags & MV_FindText_CaseSensitive) ? TextCaseSensitive : TextCaseInsensitive, msg->flags & MV_FindText_ShowAllMatches, 512);
				
				if((msg->flags & MV_FindText_ShowAllMatches) == 0)
				{
					widget->webView->page()->unmarkAllTextMatches();
				}
			}
			else
			{
				//widget->webView->page()->markAllMatchesForText(searchString, (msg->flags & MV_FindText_CaseSensitive) ? TextCaseSensitive : TextCaseInsensitive, msg->flags & MV_FindText_ShowAllMatches, 512);
			    FindOptions opts = WrapAround;
			    if (!(msg->flags & MV_FindText_CaseSensitive)) opts |= CaseInsensitive;
			    if (msg->flags & MV_FindText_Previous) opts |= Backwards;

				res = widget->webView->page()->findString(searchString, opts);
			}

			free(converted);
			return res;
		}
	}

	return 0;
}

DEFSMETHOD(FindText_DisableButtons)
{
	GETDATA;
	return DoMethodA(data->findgroup, (_Msg_*) msg);
}

DEFTMETHOD(OWBWindow_InsertBookmark)
{
	GETDATA;
	STRPTR url, title;

	url = (STRPTR)getv(data->active_browser, MA_OWBBrowser_URL);
	title = (STRPTR)getv(data->active_browser, MA_OWBBrowser_Title);

	return DoMethod(_app(obj), MM_Bookmarkgroup_AddLink, title, NULL, url, TRUE, getv(app, MA_OWBApp_Bookmark_AddToMenu), FALSE);
}

DEFSMETHOD(OWBWindow_InsertBookmarkAskTitle)
{
	// url and title freed by choosetitlegroupclass, because this method is pushed, most of the time.
	Object *req_wnd;
	char *url = msg->url;
	char *title = msg->title;

    req_wnd = (Object*) WindowObject,
		MUIA_Window_Title, GSI(MSG_OWBWINDOW_CHOOSE_TITLE),
		MUIA_Window_ID, MAKE_ID('O','A','Q','L'),
		MUIA_Window_RefWindow, obj,
        MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered,
        MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered,
        MUIA_Window_CloseGadget, FALSE,
		MUIA_Window_NoMenus, TRUE,
		WindowContents,
			NewObject(getchoosetitlegroupclass(), NULL,
			          MA_OWB_URL, url,
					  MA_OWB_Title, title,
					  MA_ChooseTitleGroup_QuickLink, msg->quicklink,
					  TAG_DONE),
        End;

	if(req_wnd)
	{
		DoMethod(app, OM_ADDMEMBER, req_wnd);
		set(req_wnd, MUIA_Window_Open, TRUE);
	}

	return 0;
}

/* FIXME: As the caller (webkit) expects answer, it's blocking
		  -> make the main loop complete to handle webkit events
*/
DEFSMETHOD(OWBWindow_JavaScriptPrompt)
{
    Object *req_wnd, *bt_cancel, *bt_ok, *str;
    STRPTR answer = NULL;

    req_wnd = (Object*) WindowObject,
		MUIA_Window_Title, GSI(MSG_OWBWINDOW_JAVASCRIPT_PROMPT),
		MUIA_Window_RefWindow, obj,
        MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Centered,
        MUIA_Window_TopEdge, MUIV_Window_TopEdge_Centered,
        MUIA_Window_CloseGadget, FALSE,
        MUIA_Window_SizeGadget, FALSE,
		MUIA_Window_NoMenus, TRUE,
        WindowContents, VGroup,
            MUIA_Background, MUII_RequesterBack,
            Child, HGroup,
                Child, TextObject,
                    TextFrame,
                    MUIA_InnerBottom, 8,
                    MUIA_InnerLeft, 8,
                    MUIA_InnerRight, 8,
                    MUIA_InnerTop, 8,
                    MUIA_Background, MUII_TextBack,
                    MUIA_Text_SetMax, TRUE,
					MUIA_Text_Contents, msg->message,
                    End,
                End,
			Child, VSpace(2),
			Child, str = (Object*) StringObject,
                MUIA_Frame, MUIV_Frame_String,
				MUIA_String_Contents, msg->defaultvalue,
				End,
			Child, VSpace(2),
            Child, HGroup,
				Child, bt_ok = (Object*) MakeButton(GSI(MSG_OWBWINDOW_JAVASCRIPT_OK)),
                Child, HSpace(0),
				Child, bt_cancel = (Object*) MakeButton(GSI(MSG_OWBWINDOW_JAVASCRIPT_CANCEL)),
                End,
            End,
        End;

    if (!req_wnd)
        return NULL;

	DoMethod(_app(obj), OM_ADDMEMBER, req_wnd);

    DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		_app(obj), 2, MUIM_Application_ReturnID, 1);

    DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
		_app(obj), 2, MUIM_Application_ReturnID, 2);

	set(req_wnd, MUIA_Window_ActiveObject, /*bt_ok*/str);
    set(req_wnd, MUIA_Window_Open, TRUE);

    LONG result = -1;

	if (getv(req_wnd, MUIA_Window_Open))
    {
        ULONG sigs = 0;

        while (result == -1)
        {
			ULONG ret = DoMethod(_app(obj), MUIM_Application_NewInput, &sigs);
            if(ret == 1 || ret == 2)
            {
                result = ret;
                break;
            }

            if (sigs)
                sigs = Wait(sigs);
        }
    }

    if (result == 2)
    {
		STRPTR value = (STRPTR) getv(str, MUIA_String_Contents);
        if(value)
			answer = strdup(value);
    }

    set(req_wnd, MUIA_Window_Open, FALSE);
	DoMethod(_app(obj), OM_REMMEMBER, req_wnd);
    MUI_DisposeObject(req_wnd);

	return (ULONG) answer;
}

/* AutoCompletion */

class matching_entry
{
public:
	ULONG cursorstart;
	WebHistoryItem *entry;

	matching_entry(WebHistoryItem *e, ULONG start)
		: cursorstart(start)
		, entry(e)
	{
	}

	friend bool operator<(const matching_entry &e1, const matching_entry &e2);
};

bool operator<(const matching_entry &e1, const matching_entry &e2)
{
	if(e1.cursorstart < e2.cursorstart)
	{
		return true;
	}
	else if(e1.cursorstart > e2.cursorstart)
	{
		return false;
	}
	else
	{
		std::string s1 = e1.entry->getPrivateItem()->m_historyItem->urlString().utf8().data();
		std::string s2 = e2.entry->getPrivateItem()->m_historyItem->urlString().utf8().data();
		return s1 < s2;
	}
}

struct completion_thread_arg
{
	struct IClass *cl;
	Object *obj;
};

static void completion_thread_start(void* p)
{
	struct completion_thread_arg * arg = (struct completion_thread_arg *) p;
	struct IClass *cl = arg->cl;
	Object * obj = arg->obj;
	GETDATA;

	String prefixes[] = { String(""), String("http://"), String("http://www."), String("https://"), String("https://www.") };

	WebHistory* history = WebHistory::sharedHistory();
	std::vector<WebHistoryItem *> *historyList;
	std::vector<matching_entry> matching_list;

	STRPTR url         = strdup(data->url_to_complete.utf8().data());
	ULONG len          = strlen(url);
	ULONG maxEntries   = 50;
	ULONG countEntries = 0;

	free(p);

	historyList = history->historyList();

	for(size_t i = historyList->size(); i > 0 && (countEntries < maxEntries); i--)
	{
		if(data->abort_completion)
			break;

		WebHistoryItem *webHistoryItem = (*historyList)[i-1];

		if(webHistoryItem)
		{
			char *item = (char *) webHistoryItem->URLString();

			if(item)
			{
				String historyURL = item;
				int offset = -1;
				bool match = false;

				webHistoryItem->setFragment(url);

				// In String completion mode, only match the url that start with what we type
				if(data->completion_mode_string)
				{
					for(unsigned int j = 0; j < sizeof(prefixes)/sizeof(String); j++)
					{
						String urlToCompare = prefixes[j];
						urlToCompare.append(url);
						match = historyURL.startsWith(urlToCompare, false);

						if(match)
						{
							len = urlToCompare.length();
							offset = 0;
							break;
						}
					}
				}
				// Otherwise, match anything
				else if(data->completion_mode_popup)
				{
					offset = historyURL.find(url, 0, false);
					match = offset != -1;
				}

				if(historyURL.length() >= len && match)
				{
					matching_entry entry(webHistoryItem, len + offset);
					matching_list.push_back(entry);

					countEntries++;
				}

				free(item);
			}
		}
	}

	if(matching_list.size())
	{
		std::sort(matching_list.begin(), matching_list.end());

		/* Popup Mode */
		if(data->completion_mode_popup)
		{
			//Object *lv_entries, *pop_string, *pop_object;

			methodstack_push(data->lv_completion, 3, MUIM_Set, MA_HistoryList_Complete, TRUE);
			methodstack_push(data->lv_completion, 3, MUIM_Set, MUIA_List_Quiet, TRUE);
			//methodstack_push(data->lv_completion, 1, MUIM_List_Clear);

			for(unsigned int i = 0; i < matching_list.size() && !data->abort_completion; i++)
			{
				methodstack_push(data->lv_completion, 3, MUIM_List_InsertSingle, matching_list[i].entry, MUIV_List_Insert_Bottom);
			}

			methodstack_push(data->lv_completion, 3, MUIM_Set, MUIA_List_Quiet, FALSE);
		}

		/* String Mode */
		if(data->completion_mode_string)
		{
			/* Only autocomplete if new text is longer than previous text (to allow deletion easily */
			if(len >= 1 && len > data->completion_prevlen)
			{
				char *item = (char *) matching_list[0].entry->URLString();

				if(item)
				{
					ULONG markstart = matching_list[0].cursorstart;
					ULONG markend = strlen(item) - 1;
					ULONG cursorpos = markend;

					methodstack_push(data->addressbargroup, 5, MM_AddressBarGroup_CompleteString, strdup(item), cursorpos, markstart, markend);
					free(item);
				}
			}
		}
	}

	free(url);
}

DEFSMETHOD(OWBWindow_AutoComplete)
{
	GETDATA;

	STRPTR url = (STRPTR)getv(data->addressbargroup, MUIA_String_Contents);

	if(url && *url)
	{
		data->url_to_complete = String(url);
		data->completion_prevlen = msg->prevlen;
		data->completion_mode_string = getv(app, MA_OWBApp_URLCompletionType) & MV_OWBApp_URLCompletionType_Autocomplete;
		data->completion_mode_popup  = getv(app, MA_OWBApp_URLCompletionType) & MV_OWBApp_URLCompletionType_Popup;

		if(!data->lv_completion)
			data->lv_completion = (Object *) getv(data->addressbargroup, MUIA_Popobject_Object);

		if(data->completion_mode_popup)
		{
			/* Open history listview and focus to string again */
			DoMethod(data->lv_completion, MUIM_List_Clear);
			DoMethod((Object *) getv(data->addressbargroup, MA_AddressBarGroup_PopString), MUIM_Popstring_Open);
			set(obj, MUIA_Window_Activate, TRUE);
			set(obj, MUIA_Window_ActiveObject, (Object *) getv(data->addressbargroup, MUIA_Popstring_String));
		}

		if(data->completion_thread)
		{
			data->abort_completion = TRUE;
			waitForThreadCompletion(data->completion_thread);
			data->abort_completion = FALSE;
		}

		struct completion_thread_arg * arg = (struct completion_thread_arg *) malloc(sizeof(struct completion_thread_arg));
		if(arg)
		{
			arg->cl = cl;
			arg->obj = obj;

			data->completion_thread = createThread(completion_thread_start, arg, "[OWB] Completion Thread");
		}
	}
	else
	{
		DoMethod((Object *) getv(data->addressbargroup, MA_AddressBarGroup_PopString), MUIM_Popstring_Close);
	}

	return 0;
}

DEFTMETHOD(OWBWindow_UpdateBookmarkPanel)
{
	GETDATA;
	DoMethod(data->bookmarkpanelgroup, MM_Bookmarkgroup_LoadHtml, BOOKMARK_PATH);
	return 0;
}

DEFTMETHOD(OWBWindow_UpdatePanelGroup)
{
	GETDATA;

	set(data->panelgroup, MUIA_Weight, getv(app, MA_OWBApp_PanelWeight));

	if(getv(app, MA_OWBApp_ShowBookmarkPanel))
	{
		set(obj, MA_OWBWindow_ShowBookmarkPanel, TRUE);
	}
	else if(getv(app, MA_OWBApp_ShowHistoryPanel))
	{
		set(obj, MA_OWBWindow_ShowHistoryPanel, TRUE);
	}

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
				 MUIM_SetUData, MNA_NO_PANEL, MUIA_Menuitem_Checked, getv(app, MA_OWBApp_ShowBookmarkPanel) == FALSE && getv(app, MA_OWBApp_ShowHistoryPanel) == FALSE);

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
			 MUIM_SetUData, MNA_BOOKMARK_PANEL, MUIA_Menuitem_Checked, getv(app, MA_OWBApp_ShowBookmarkPanel));

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
			 MUIM_SetUData, MNA_HISTORY_PANEL, MUIA_Menuitem_Checked, getv(app, MA_OWBApp_ShowHistoryPanel));

	return 0;
}


DEFTMETHOD(OWBWindow_RemoveBookmarkPanel)
{
	DoMethod(app, MUIM_Application_PushMethod, obj, 3, MUIM_Set, MA_OWBWindow_ShowBookmarkPanel, FALSE);

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
			 MUIM_SetUData, MNA_BOOKMARK_PANEL, MUIA_Menuitem_Checked, FALSE);

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
			 MUIM_SetUData, MNA_NO_PANEL, MUIA_Menuitem_Checked, TRUE);
	return 0;
}

DEFTMETHOD(OWBWindow_RemoveHistoryPanel)
{
	DoMethod(app, MUIM_Application_PushMethod, obj, 3, MUIM_Set, MA_OWBWindow_ShowHistoryPanel, FALSE);

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
			 MUIM_SetUData, MNA_HISTORY_PANEL, MUIA_Menuitem_Checked, FALSE);

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip),
			 MUIM_SetUData, MNA_NO_PANEL, MUIA_Menuitem_Checked, TRUE);
	return 0;
}

DEFSMETHOD(OWBWindow_AddClosedView)
{
	GETDATA;
#define MAX_CLOSED_VIEWS_COUNT 20

	Object *strip = (Object *) getv(obj, MUIA_Window_Menustrip);
	Object *menu = (Object *) DoMethod(strip, MUIM_FindUData, MNA_HISTORY_CLOSEDVIEWS);

	if(menu)
	{
		Object *item;
		String truncatedTitle;
		struct menu_entry *menu_entry;
		STRPTR title = (STRPTR) getv(msg->browser, MA_OWBBrowser_Title); // Already converted to local encoding
		STRPTR url   = (STRPTR) getv(msg->browser, MA_OWBBrowser_URL);

		if(!url || *url == '\0')
		{
			// blank page, don't record
			return 0;
		}

		if(title && *title == '\0')
		{
			// blank title, use URL
			title = url;
		}

		// truncate title
		truncatedTitle = title;
		if(truncatedTitle.length() > 64)
		{
			truncatedTitle.truncate(64);
			truncatedTitle.append("...");
		}

		menu_entry = (struct menu_entry *) malloc(sizeof(*menu_entry) + strlen(url) + 1);

		if(menu_entry)
		{
			menu_entry->type = MENUTYPE_CLOSEDVIEW;
			stccpy(menu_entry->data, url, strlen(url) + 1);

			item = (Object *) NewObject(getmenuitemclass(), NULL,
				MUIA_Menuitem_Title, strdup(truncatedTitle.latin1().data()),
				MUIA_UserData,       menu_entry,
				End;

			if(item)
			{
				struct historymenuitemnode *cvn = (struct historymenuitemnode *) malloc(sizeof(*cvn));

				if(cvn)
				{
					APTR n, m;
					ULONG count = 0;

					cvn->title = strdup(title);
					cvn->url = strdup(url);
					cvn->x = 0;
					cvn->y = 0;
					cvn->obj = item;

					ITERATELISTSAFE(n, m, &data->history_closed_list)
					{
						count++;
					}

					if(count == MAX_CLOSED_VIEWS_COUNT)
					{
						struct historymenuitemnode *rcvn = (struct historymenuitemnode *) REMTAIL(&data->history_closed_list);
						DoMethod(menu, MUIM_Menustrip_InitChange);
						DoMethod(menu, MUIM_Family_Remove, rcvn->obj);
						MUI_DisposeObject(rcvn->obj);
						free(rcvn->title);
						free(rcvn->url);
						free(rcvn);
						DoMethod(menu, MUIM_Menustrip_ExitChange);
					}

					ADDHEAD(&data->history_closed_list, cvn);

					DoMethod(menu, MUIM_Menustrip_InitChange);
					DoMethod(menu, MUIM_Family_AddHead, item);
					DoMethod(menu, MUIM_Menustrip_ExitChange);
				}
			}
		}
	}

	return 0;
}

DEFSMETHOD(OWBWindow_AddHistoryItem)
{
#define MAX_HISTORY_ENTRIES_COUNT 20
	GETDATA;
	WebHistoryItem *s = (WebHistoryItem *) msg->item;
	Object *strip = (Object *) getv(obj, MUIA_Window_Menustrip);
	Object *menu = (Object *) DoMethod(strip, MUIM_FindUData, MNA_HISTORY_MENU);
	Object *previtem = (Object *) DoMethod(strip, MUIM_FindUData, MNA_HISTORY_RECENTENTRIES);

	if(menu)
	{
		Object *item;
		String truncatedTitle;
		struct menu_entry *menu_entry;
		char *title = (char *) s->title();
		char *url   = (char *) s->URLString();
		char *titleptr = title;

		if(!title || *title == '\0')
		{
			titleptr = url;
		}

		// truncate title
		truncatedTitle = String::fromUTF8(titleptr);
		if(truncatedTitle.length() > 64)
		{
			truncatedTitle.truncate(64);
			truncatedTitle.append("...");
		}

		menu_entry = (struct menu_entry *) malloc(sizeof(*menu_entry) + strlen(url) + 1);

		if(menu_entry )
		{
			menu_entry->type = MENUTYPE_HISTORY;
			stccpy(menu_entry->data, url, strlen(url) + 1);

			item = (Object *) NewObject(getmenuitemclass(), NULL,
				MUIA_Menuitem_Title, utf8_to_local(truncatedTitle.utf8().data()),
				MUIA_UserData,       menu_entry,
				End;

			if(item)
			{
				struct historymenuitemnode *cvn = (struct historymenuitemnode *) malloc(sizeof(*cvn));

				if(cvn)
				{
					APTR n, m;
					ULONG count = 0;

					cvn->title = strdup(titleptr);
					cvn->url = strdup(url);
					cvn->x = 0;
					cvn->y = 0;
					cvn->obj = item;

					/* Remove duplicates */

					ITERATELISTSAFE(n, m, &data->history_list)
					{
						struct historymenuitemnode *mn = (struct historymenuitemnode *) n;

						if(!strcmp(mn->url, cvn->url))
						{
							DoMethod(menu, MUIM_Menustrip_InitChange);
							DoMethod(menu, MUIM_Family_Remove, mn->obj);
							REMOVE(mn);
							MUI_DisposeObject(mn->obj);
							free(mn->title);
							free(mn->url);
							free(mn);
							DoMethod(menu, MUIM_Menustrip_ExitChange);					  
						}
						else
						{
							count++;
						}
					}

					if(count == MAX_HISTORY_ENTRIES_COUNT)
					{
						struct historymenuitemnode *rcvn = (struct historymenuitemnode *) REMTAIL(&data->history_list);
						DoMethod(menu, MUIM_Menustrip_InitChange);
						DoMethod(menu, MUIM_Family_Remove, rcvn->obj);
						MUI_DisposeObject(rcvn->obj);
						free(rcvn->title);
						free(rcvn->url);
						free(rcvn);
						DoMethod(menu, MUIM_Menustrip_ExitChange);
					}

					ADDHEAD(&data->history_list, cvn);

					DoMethod(menu, MUIM_Menustrip_InitChange);
					DoMethod(menu, MUIM_Family_Insert, item, previtem);
					DoMethod(menu, MUIM_Menustrip_ExitChange);
				}
			}
		}

		free(title);
		free(url);
	}

	return 0;
}

DEFSMETHOD(OWBWindow_FullScreen)
{
	GETDATA;

	ULONG gofullscreen;

	if(msg->fullscreen == MV_OWBWindow_FullScreen_Toggle)
	{
		if(data->fullscreen == FALSE)
			gofullscreen = TRUE;
		else
			gofullscreen = FALSE;
	}
	else
	{
		gofullscreen = msg->fullscreen;
	}

	if(gofullscreen)
    {
		// Backup data before switching

#if ENABLE(VIDEO)
		// If a media element was in fullscreen mode, save it
		HTMLMediaElement *element = (HTMLMediaElement *) getv(data->active_browser, MA_OWBBrowser_VideoElement);
#endif

    	// Save maingroup spacing to set it when being back from fullscreen mode
		data->maingroup_innerleft   = getv(data->maingroup, MUIA_InnerLeft);
		data->maingroup_innerright  = getv(data->maingroup, MUIA_InnerRight);
		data->maingroup_innertop    = getv(data->maingroup, MUIA_InnerTop);
		data->maingroup_innerbottom = getv(data->maingroup, MUIA_InnerBottom);

		// Save panel state
		data->panelshow = getv(obj, MA_OWBWindow_ShowHistoryPanel) || getv(obj, MA_OWBWindow_ShowBookmarkPanel);
		if(getv(obj, MA_OWBWindow_ShowHistoryPanel))
		{
			data->paneltype = 1;
		}
		else if(getv(obj, MA_OWBWindow_ShowBookmarkPanel))
		{
			data->paneltype = 2;
		}
		else
		{
			data->paneltype = 0;
		}

		set(obj, MUIA_Window_Open, FALSE);

        set(obj, MUIA_Window_Title, NULL);
		set(obj, MUIA_Window_ID, NULL);
        set(obj, MUIA_Window_Borderless, TRUE);
        set(obj, MUIA_Window_DragBar, FALSE);
        set(obj, MUIA_Window_CloseGadget, FALSE);
        set(obj, MUIA_Window_DepthGadget, FALSE);
        set(obj, MUIA_Window_Width, MUIV_Window_Width_Visible(100));
        set(obj, MUIA_Window_Height, MUIV_Window_Height_Visible(100));

		set(data->maingroup, MUIA_InnerLeft,   0);
		set(data->maingroup, MUIA_InnerRight,  0);
		set(data->maingroup, MUIA_InnerTop,    4); // Keep a small space to access pulldownmenu
		set(data->maingroup, MUIA_InnerBottom, 0);

        set(obj, MUIA_Window_Open, TRUE);

		// Only do this after window is opened
		DoMethod(data->maingroup, MUIM_Group_InitChange);
		set(obj, MA_OWBWindow_ShowAddressBarGroup, FALSE);
		set(obj, MA_OWBWindow_ShowQuickLinkGroup, FALSE);
		set(obj, MA_OWBWindow_ShowNavigationGroup, FALSE);
		set(obj, MA_OWBWindow_ShowStatusGroup, FALSE);

		set(obj, MA_OWBWindow_ShowHistoryPanel,  FALSE);
		set(obj, MA_OWBWindow_ShowBookmarkPanel, FALSE);
        DoMethod(obj, MM_OWBWindow_UpdatePanelGroup);

		DoMethod(data->maingroup, MUIM_Group_ExitChange);

#if ENABLE(VIDEO)
		if(element)
		{
			element->enterFullscreen();
		}
#endif

		data->fullscreen = TRUE;
    }
	else
    {
#if ENABLE(VIDEO)
		// If a media element was in fullscreen mode, save it
		HTMLMediaElement *element = (HTMLMediaElement *) getv(data->active_browser, MA_OWBBrowser_VideoElement);
#endif

        set(obj, MUIA_Window_Open, FALSE);

		set(obj, MUIA_Window_Title, data->windowtitle);
		set(obj, MUIA_Window_ID, MAKE_ID('O','W','B', 'W')); // hmhm, update that if we change the id someday
        set(obj, MUIA_Window_Borderless, FALSE);
        set(obj, MUIA_Window_DragBar, TRUE);
        set(obj, MUIA_Window_CloseGadget, TRUE);
        set(obj, MUIA_Window_DepthGadget, TRUE);
        set(obj, MUIA_Window_Width, MUIV_Window_Width_Default);
        set(obj, MUIA_Window_Height, MUIV_Window_Height_Default);

		set(data->maingroup, MUIA_InnerLeft,   data->maingroup_innerleft);
		set(data->maingroup, MUIA_InnerRight,  data->maingroup_innerright);
		set(data->maingroup, MUIA_InnerTop,    data->maingroup_innertop);
		set(data->maingroup, MUIA_InnerBottom, data->maingroup_innerbottom);

        set(obj, MUIA_Window_Open, TRUE);

		// Only do this after window is opened
		DoMethod(data->maingroup, MUIM_Group_InitChange);
		set(obj, MA_OWBWindow_ShowAddressBarGroup, TRUE);
		set(obj, MA_OWBWindow_ShowQuickLinkGroup, TRUE);
		set(obj, MA_OWBWindow_ShowNavigationGroup, TRUE);
		set(obj, MA_OWBWindow_ShowStatusGroup, TRUE);

		if(data->paneltype == 1)
		{
			set(obj, MA_OWBWindow_ShowHistoryPanel, TRUE);
		}
		else if(data->paneltype == 2)
		{
			set(obj, MA_OWBWindow_ShowBookmarkPanel, TRUE);
		}
		else
		{
			set(obj, MA_OWBWindow_ShowHistoryPanel,  FALSE);
			set(obj, MA_OWBWindow_ShowBookmarkPanel, FALSE);		
		}
		DoMethod(obj, MM_OWBWindow_UpdatePanelGroup);

		DoMethod(data->maingroup, MUIM_Group_ExitChange);

#if ENABLE(VIDEO)
		if(element)
		{
			element->enterFullscreen();
		}
#endif

		data->fullscreen = FALSE;
    }

	DoMethod((Object *) getv(obj, MUIA_Window_Menustrip), MUIM_SetUData, MNA_FULLSCREEN, MUIA_Menuitem_Checked, data->fullscreen);

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECSMETHOD(OWBWindow_MenuAction)
DECSMETHOD(OWBWindow_LoadURL)
DECTMETHOD(OWBWindow_Back)
DECTMETHOD(OWBWindow_Forward)
DECTMETHOD(OWBWindow_Stop)
DECTMETHOD(OWBWindow_Home)
DECSMETHOD(OWBWindow_Reload)
DECSMETHOD(OWBWindow_AddBrowser)
DECSMETHOD(OWBWindow_RemoveBrowser)
DECSMETHOD(OWBWindow_DetachBrowser)
DECSMETHOD(OWBWindow_TransferBrowser)
DECSMETHOD(OWBWindow_ActivePage)
DECTMETHOD(OWBWindow_InspectPage)
DECSMETHOD(OWBWindow_CreateInspector)
DECSMETHOD(OWBWindow_DestroyInspector)
DECTMETHOD(OWBWindow_Close)
DECTMETHOD(OWBWindow_OpenLocalFile)
DECTMETHOD(OWBWindow_SaveAsSource)
DECTMETHOD(OWBWindow_SaveAsPDF)
DECTMETHOD(OWBWindow_Print)
DECSMETHOD(OWBWindow_UpdateTitle)
DECSMETHOD(OWBWindow_UpdateStatus)
DECSMETHOD(OWBWindow_UpdateURL)
DECSMETHOD(OWBWindow_UpdateNavigation)
DECSMETHOD(OWBWindow_UpdateProgress)
DECSMETHOD(OWBWindow_UpdateState)
DECSMETHOD(OWBWindow_UpdateSecurity)
DECSMETHOD(OWBWindow_UpdateZone)
DECSMETHOD(OWBWindow_UpdatePrivateBrowsing)
DECSMETHOD(OWBWindow_UpdateUserScript)
DECSMETHOD(OWBWindow_UpdateMenu)
DECTMETHOD(OWBWindow_BuildSpoofMenu)
DECSMETHOD(FindText_DisableButtons)
DECSMETHOD(Search)
DECSMETHOD(OWBWindow_JavaScriptPrompt)
DECTMETHOD(OWBWindow_InsertBookmark)
DECSMETHOD(OWBWindow_InsertBookmarkAskTitle)
DECSMETHOD(OWBWindow_AutoComplete)
DECTMETHOD(OWBWindow_UpdateBookmarkPanel)
DECTMETHOD(OWBWindow_UpdatePanelGroup)
DECTMETHOD(OWBWindow_RemoveBookmarkPanel)
DECTMETHOD(OWBWindow_RemoveHistoryPanel)
DECSMETHOD(OWBWindow_AddClosedView)
DECSMETHOD(OWBWindow_AddHistoryItem)
DECSMETHOD(OWBWindow_FullScreen)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, owbwindowclass)
