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

#if defined(__AROS__) /* Variadic + C++ local object problem */
#define NO_INLINE_STDARG
#endif

#include "cairo.h"

/* OWB */
#include "config.h"
#include "GraphicsContext.h"
#include <Api/WebFrame.h>
#include <Api/WebView.h>
#include <wtf/text/CString.h>
#include "Page.h"
#include "DOMImplementation.h"
#include "Document.h"
#include "Editor.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "GCController.h"
#include "ProgressTracker.h"
#include "SharedTimer.h"
#include <wtf/CurrentTime.h>
#include "SubstituteData.h"
#include "Timer.h"
#include <wtf/MainThread.h>
#include "MIMETypeRegistry.h"
#include "WebBackForwardList.h"
#include "WebDataSource.h"
#include "WebPreferences.h"
#include "WebDownload.h"
#include "WebDownloadPrivate.h"
#include "WebFramePolicyListener.h"
#include "DownloadDelegateMorphOS.h"
#include "CookieManager.h"
#include "ResourceHandleManager.h"
#include "ResourceHandle.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include "ResourceLoader.h"
#include "URL.h"
#include "FileIOLinux.h"
#include "HTMLCollection.h"
#include "HTMLInputElement.h"
#include "ContextMenu.h"
#include "ContextMenuController.h"
#include "PluginDatabase.h"
#if ENABLE(ICONDATABASE)
#include "IconDatabase.h"
#include "WebIconDatabase.h"
#endif
#include "WebHistory.h"
#include "WebHistoryItem.h"
#include "WebMutableURLRequest.h"
#include "WebPasswordFormData.h"
#include "WebPlatformStrategies.h"
#include "WindowFeatures.h"
#include <runtime/InitializeThreading.h>
#include "JSDOMWindow.h"
#include "JSDOMWindowBase.h"

/* Posix */
#include <unistd.h>
#include <cstdio>
#include <string.h>

/* System */
#include <clib/macros.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <libraries/gadtools.h>
#include <libraries/openurl.h>
#include <mui/Aboutbox_mcc.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>

#include <proto/openurl.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/dos.h>
#include <proto/asl.h>

/* Local */
#include "gui.h"
#include "utils.h"
#include "clipboard.h"
#include "owbcommand.h"
#include "asl.h"
#include "ExtCredential.h"
#include "versioninfo.h"

#define D(x)
#undef String

/******************************************************************
 * owbappclass
 *****************************************************************/

using namespace WebCore;

extern struct Library * OpenURLBase;
extern char * _ProgramName;

namespace WebCore
{
    extern bool ad_block_enabled;
    extern void freeLeakedMediaObjects();
}

Object *app;

struct MinList window_list;
struct MinList view_list;
struct MinList download_list;
struct MinList contextmenu_list;
struct MinList mimetype_list;
struct MinList urlsetting_list;
struct MinList family_list;

static const CONST_STRPTR classlist[] = {
	"Lamp.mcc",
	NULL
};

static CONST TEXT credits[] =
	"\033b%p\033n\n"
	"\tFabien Coeurjoly\n"
	"\tKrzysztof Smiechowicz\n"
	"\tSandLabs\n"
	"\tWebKit\n\n"

	"\033b%T\033n\n"
	"\tChristophe Delorme\n\t\t\033ifor his transfer animations\033n\n"
	"\tIlkka Lehtoranta\n\t\t\033ifor his NetSurf GUI classes\033n\n"
	"\tFrederic Rignault\n\t\t\033ifor his bookmark class\033n\n"
	"\tChristian Rosentreter\n\t\t\033ifor ShowGirls icon\033n\n"
	"\tAndré Siegel\n\t\t\033ifor his OWB icon\033n\n\n"

	"\033b%l\033n\n"
	"\tStefan Blixth\n"
	"\tGergely Boros\n"
	"\tJaime Cagigal\n"
	"\tSamir Hawamdeh\n"
	"\tMarcin Kornas\n"
	"\tAnbjørn Myren\n"
	"\tMickaël Pernot\n"
	"\tChristian Rosentreter\n"
	"\tHarry Sintonen\n"
	"\tRoman Brychta\n\n"

	"\033b%W\033n\n"
	"\thttp://fabportnawak.free.fr/owb/";

struct AppSettings
{
	ULONG panelweight;
	ULONG showbookmarkpanel;
	ULONG showhistorypanel;
	ULONG addbookmarkstomenu;
	ULONG continuousspellchecking;
	ULONG privatebrowsing;
};

struct BrowserState
{
	String url;
	IntPoint offset;
};

struct WindowState
{
	int id;
	Vector<BrowserState> browserstates;
};

struct Data
{
	struct DiskObject *diskobject;
	Object *menustrip;

	Object *aboutwin;
	Object *dlwin;
	Object *prefswin;
	Object *bookmarkwin;
	Object *networkwin;
	Object *consolewin;
	Object *passwordmanagerwin;
	Object *cookiemanagerwin;
	Object *blockmanagerwin;
	Object *searchmanagerwin;
	Object *scriptmanagerwin;
	Object *urlprefswin;
	Object *printerwin;

	/* Settings (somewhat mirroring prefswindowclass, sucky) */
	ULONG newpagepolicy;
	ULONG newpageposition;
	ULONG popuppolicy;
	ULONG showbuttonframe;
	ULONG enablepointers;
	ULONG showsearchbar;
	ULONG showvalidationbuttons;
	ULONG showseparators;
	ULONG enabletabtransferanim;
	ULONG toolbuttontype;
	ULONG quicklinklook;
	ULONG quicklinklayout;
	ULONG quicklinkrows;
	ULONG quicklinkposition;
	ULONG urlcompletiontype;
	ULONG errormode;
	ULONG showfavicons;
	ULONG middlebuttonpolicy;

	ULONG inspector;

	ULONG downloadautoclose;
	ULONG savedownloads;
	ULONG downloadstartautomatically;

	ULONG savesession;
	ULONG deletesessionatexit;
	ULONG sessionrestoremode;
	/*
	ULONG temporarycookiespolicy;
	ULONG persistantcookiespolicy;
	*/
	ULONG savecookies;
	ULONG cookiespolicy;
	ULONG enablelocalstorage;
	ULONG savehistory;
	ULONG historyitems;
	ULONG historyage;
	ULONG savecredentials;
	ULONG formautofill;

	ULONG use_partial_content;
	ULONG use_webm;
	ULONG use_flv;
	ULONG use_ogg;
	ULONG loopfilter_mode;

	char homepage[512];
	char startpage[512];
	char newtabpage[512];
	char download_dir[1024];
	char current_dir[1024];
	char useragent[512];

	ULONG ignoreSSLErrors;
	char certificate_path[512];

	/* Temp stuff */
	char dummy[64];

	/* Persistant settings saved automatically when quitting */
	struct AppSettings appsettings; 

	/* App state */
	Object *lastwin; // Last activated window
	ULONG privatebrowsing_clients;
	LONG downloads_in_progress;
	ULONG isquitting;

	/* Notifications */
	STRPTR favicon_url;
	ULONG  favicon_import_complete;
	ULONG  history_changed;
	ULONG  bookmarks_changed;
};

/* Menu */

#define TICK (CHECKIT|MENUTOGGLE)
#define DIS  NM_ITEMDISABLED
#define MENU(x) (STRPTR)MSG_MENU_##x

struct NewMenu MenuData[] =
{
/* Project */
	{ NM_TITLE, MENU(PROJECT)         , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(NEW_WINDOW)      , (STRPTR) "N", 0, 0, (APTR)MNA_NEW_WINDOW },
	{ NM_ITEM , MENU(NEW_TAB)         , (STRPTR) "T", 0, 0, (APTR)MNA_NEW_PAGE },
	{ NM_ITEM , MENU(OPEN_LOCAL_FILE) , (STRPTR) "O", 0, 0, (APTR)MNA_OPEN_LOCAL_FILE },
	{ NM_ITEM , MENU(OPEN_URL)        , (STRPTR) "L", 0, 0, (APTR)MNA_OPEN_URL },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(CLOSE_WINDOW)    , (STRPTR) "Y", 0, 0, (APTR)MNA_CLOSE_WINDOW },
	{ NM_ITEM , MENU(CLOSE_TAB)       , (STRPTR) "K", 0, 0, (APTR)MNA_CLOSE_PAGE },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(OPEN_SESSION)    , 0, 0, 0, (APTR)MNA_OPEN_SESSION },
	{ NM_ITEM , MENU(SAVE_SESSION)    , 0, 0, 0, (APTR)MNA_SAVE_SESSION },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
//	  { NM_ITEM , "Save As"           , 0, 0, 0, NULL },
//	  { NM_SUB  , "Text..."           , 0, 0, 0, (APTR)MNA_SAVE_AS_TEXT },
	{ NM_ITEM , MENU(SAVE_AS_SOURCE)  , 0, 0, 0, (APTR)MNA_SAVE_AS_SOURCE },
	{ NM_ITEM , MENU(SAVE_AS_PDF)     , 0, 0, 0, (APTR)MNA_SAVE_AS_PDF },
  	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(PRINT)           , (STRPTR) "P", 0, 0, (APTR)MNA_PRINT },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(ABOUT)           , 0, 0, 0, (APTR)MNA_ABOUT },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(QUIT)            , (STRPTR) "Q", 0, 0, (APTR)MNA_QUIT },

/* Edit */
	{ NM_TITLE, MENU(EDIT)            , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(CUT)             , 0, 0, 0, (APTR)MNA_CUT },
	{ NM_ITEM , MENU(COPY)            , 0, 0, 0, (APTR)MNA_COPY },
	{ NM_ITEM , MENU(PASTE)           , 0, 0, 0, (APTR)MNA_PASTE },
	{ NM_ITEM , MENU(PASTE_TO_URL)    , (STRPTR) "G", 0, 0, (APTR)MNA_PASTE_TO_URL },
	{ NM_ITEM , MENU(SELECT_ALL)      , 0, 0, 0, (APTR)MNA_SELECT_ALL },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(FIND)            , (STRPTR) "F", 0, 0, (APTR)MNA_FIND },

/* View */
	{ NM_TITLE, MENU(VIEW)            , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(NAVIGATION)      , 0, CHECKIT|MENUTOGGLE, 0, (APTR)MNA_NAVIGATION },
	{ NM_ITEM , MENU(QUICKLINKS)      , 0, CHECKIT|MENUTOGGLE, 0, (APTR)MNA_QUICKLINKS },
	{ NM_ITEM , MENU(LOCATION)        , 0, CHECKIT|MENUTOGGLE, 0, (APTR)MNA_LOCATION },
	{ NM_ITEM , MENU(STATUS)          , 0, CHECKIT|MENUTOGGLE, 0, (APTR)MNA_STATUS },
	{ NM_ITEM , MENU(FULLSCREEN)      , 0, CHECKIT|MENUTOGGLE, 0, (APTR)MNA_FULLSCREEN },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(PANELS)          , 0, 0, 0, NULL },
	{ NM_SUB  , MENU(NO_PANEL)        , 0, CHECKIT|MENUTOGGLE, 2 | 4, (APTR)MNA_NO_PANEL },
	{ NM_SUB  , MENU(BOOKMARK_PANEL)  , (STRPTR) "M", CHECKIT|MENUTOGGLE, 1 | 4, (APTR)MNA_BOOKMARK_PANEL },
	{ NM_SUB  , MENU(HISTORY_PANEL)   , (STRPTR) "H", CHECKIT|MENUTOGGLE, 1 | 2, (APTR)MNA_HISTORY_PANEL },

	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(BACK)            , (STRPTR) "B", 0, 0, (APTR)MNA_BACK },
	{ NM_ITEM , MENU(FORWARD)         , (STRPTR) "W", 0, 0, (APTR)MNA_FORWARD },
//	  { NM_ITEM , MENU(HOME)            , (STRPTR) "H", 0, 0, (APTR)MNA_HOME },
	{ NM_ITEM , MENU(STOP)            , (STRPTR) "S", 0, 0, (APTR)MNA_STOP },
	{ NM_ITEM , MENU(RELOAD)          , (STRPTR) "R", 0, 0, (APTR)MNA_RELOAD },

	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(ZOOM_IN)         , (STRPTR) "+", 0, 0, (APTR)MNA_ZOOM_IN },
	{ NM_ITEM , MENU(ZOOM_RESET)      , (STRPTR) "0", 0, 0, (APTR)MNA_ZOOM_RESET },
	{ NM_ITEM , MENU(ZOOM_OUT)        , (STRPTR) "-", 0, 0, (APTR)MNA_ZOOM_OUT },
	{ NM_ITEM , NM_BARLABEL           , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(SOURCE)          , (STRPTR) "U", 0, 0, (APTR)MNA_SOURCE },

/* History */
	{ NM_TITLE, MENU(HISTORY)         , 0, 0, 0, (APTR)MNA_HISTORY_MENU },
	{ NM_ITEM , MENU(HISTORY_CLOSEDVIEWS), 0, 0, 0, (APTR)MNA_HISTORY_CLOSEDVIEWS },
	{ NM_ITEM , NM_BARLABEL            , 0, 0, 0, (APTR)MNA_HISTORY_RECENTENTRIES },

/* Bookmarks */
	{ NM_TITLE, MENU(BOOKMARKS)       , 0, 0, 0, (APTR)MNA_BOOKMARKS_MENU },

/* Windows */
	{ NM_TITLE, MENU(WINDOWS)         , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(DOWNLOADS)       , (STRPTR) "D", 0, 0, (APTR)MNA_DOWNLOADS_WINDOW },
	{ NM_ITEM , MENU(NETWORK)         , 0, 0, 0, (APTR)MNA_NETWORK_WINDOW },
	{ NM_ITEM , MENU(PASSWORDS)       , 0, 0, 0, (APTR)MNA_PASSWORDMANAGER_WINDOW },
	{ NM_ITEM , MENU(COOKIES)         , 0, 0, 0, (APTR)MNA_COOKIEMANAGER_WINDOW },
	{ NM_ITEM , MENU(BLOCK)           , 0, 0, 0, (APTR)MNA_BLOCKMANAGER_WINDOW },
	{ NM_ITEM , MENU(SEARCH_ENGINES)  , 0, 0, 0, (APTR)MNA_SEARCHMANAGER_WINDOW },
	{ NM_ITEM , MENU(SCRIPT)          , 0, 0, 0, (APTR)MNA_SCRIPTMANAGER_WINDOW },
	{ NM_ITEM , MENU(URLPREFS)        , 0, 0, 0, (APTR)MNA_URLSETTINGS_WINDOW },
	{ NM_ITEM , MENU(MESSAGES)        , 0, 0, 0, (APTR)MNA_CONSOLE_WINDOW },

/* Settings */
	{ NM_TITLE, MENU(SETTINGS)          , 0, 0, 0, NULL },

	{ NM_ITEM , MENU(ENABLE_IMAGES)     , 0, 0, 0, (APTR)MNA_SETTINGS_ENABLE_IMAGES },
	{ NM_SUB  , MENU(ENABLE)            , 0, CHECKIT|MENUTOGGLE, 2 | 4, (APTR)MNA_SETTINGS_IMAGES_ENABLED },
	{ NM_SUB  , MENU(DISABLE)           , 0, CHECKIT|MENUTOGGLE, 1 | 4, (APTR)MNA_SETTINGS_IMAGES_DISABLED },
	{ NM_SUB  , MENU(DEFAULT)           , 0, CHECKIT|MENUTOGGLE, 1 | 2, (APTR)MNA_SETTINGS_IMAGES_DEFAULT },
/*
	{ NM_ITEM , MENU(ENABLE_ANIMATIONS) , 0, 0, 0, (APTR)MNA_SETTINGS_ENABLE_ANIMATIONS },
	{ NM_SUB  , MENU(ENABLE)            , 0, CHECKIT|MENUTOGGLE, 2 | 4, (APTR)MNA_SETTINGS_ANIMATIONS_ENABLED },
	{ NM_SUB  , MENU(DISABLE)           , 0, CHECKIT|MENUTOGGLE, 1 | 4, (APTR)MNA_SETTINGS_ANIMATIONS_DISABLED },
	{ NM_SUB  , MENU(DEFAULT)           , 0, CHECKIT|MENUTOGGLE, 1 | 2, (APTR)MNA_SETTINGS_ANIMATIONS_DEFAULT },
*/
	{ NM_ITEM , MENU(ENABLE_JAVASCRIPT) , 0, 0, 0, (APTR)MNA_SETTINGS_ENABLE_JAVASCRIPT },
	{ NM_SUB  , MENU(ENABLE)            , 0, CHECKIT|MENUTOGGLE, 2 | 4, (APTR)MNA_SETTINGS_JAVASCRIPT_ENABLED },
	{ NM_SUB  , MENU(DISABLE)           , 0, CHECKIT|MENUTOGGLE, 1 | 4, (APTR)MNA_SETTINGS_JAVASCRIPT_DISABLED },
	{ NM_SUB  , MENU(DEFAULT)           , 0, CHECKIT|MENUTOGGLE, 1 | 2, (APTR)MNA_SETTINGS_JAVASCRIPT_DEFAULT },

	{ NM_ITEM , MENU(ENABLE_PLUGINS)    , 0, 0, 0, (APTR)MNA_SETTINGS_ENABLE_PLUGINS },
	{ NM_SUB  , MENU(ENABLE)            , 0, CHECKIT|MENUTOGGLE, 2 | 4, (APTR)MNA_SETTINGS_PLUGINS_ENABLED },
	{ NM_SUB  , MENU(DISABLE)           , 0, CHECKIT|MENUTOGGLE, 1 | 4, (APTR)MNA_SETTINGS_PLUGINS_DISABLED },
	{ NM_SUB  , MENU(DEFAULT)           , 0, CHECKIT|MENUTOGGLE, 1 | 2, (APTR)MNA_SETTINGS_PLUGINS_DEFAULT },

	{ NM_ITEM , MENU(SPOOF_AS)          , 0, 0, 0, (APTR)MNA_SETTINGS_SPOOF_AS },

	{ NM_ITEM , MENU(PRIVATE_BROWSING)  , 0, CHECKIT|MENUTOGGLE, 0, (APTR)MNA_PRIVATE_BROWSING },

	{ NM_ITEM , NM_BARLABEL             , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(PREFERENCES)       , 0, 0, 0, (APTR)MNA_OWB_SETTINGS },
	{ NM_ITEM , NM_BARLABEL             , 0, 0, 0, NULL },
	{ NM_ITEM , MENU(MUI)               , 0, 0, 0, (APTR)MNA_MUI_SETTINGS },
	{ NM_END  , NULL                    , 0, 0, 0, NULL }
};

ULONG menus_init(void)
{
	struct NewMenu *nmp = MenuData;

	while (nmp->nm_Type)
	{
		if (nmp->nm_Label != NM_BARLABEL)
		{
			nmp->nm_Label = (STRPTR)GSI((ULONG)nmp->nm_Label);
		}
		nmp++;
	}

	return (TRUE);
}

/* Rexx interface */

enum
{
	REXX_PRINT,
	REXX_ABOUT,
	REXX_COPY,
	REXX_CLEARSELECTED,
	REXX_SNAPSHOT,
	REXX_WINDOWTOFRONT,
	REXX_WINDOWTOBACK,
	REXX_ACTIVATEWINDOW,
	REXX_SCREENTOFRONT,
	REXX_SCREENTOBACK,
	REXX_OPEN,
	REXX_RELOAD,
	REXX_BACK,
	REXX_FORWARD,
	REXX_SAVEURL,
	REXX_ADDBOOKMARK,
	REXX_GETURL,
	REXX_GETSELECTED,
	REXX_FULLSCREEN,
	REXX_GETTITLE,
	REXX_STATUS
};

#if OS(MORPHOS)
#define REXXHOOK(name, param)	static const struct Hook name = { { NULL, NULL }, (HOOKFUNC)&RexxEmul, NULL, (APTR)(param) }
static LONG Rexx(void);
static const struct EmulLibEntry RexxEmul = { TRAP_LIB, 0, (void (*)())&Rexx };
#endif
#if OS(AROS)
AROS_UFP3
(
    LONG, Rexx,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, params, A1)
);
#define REXXHOOK(name, param)   static const struct Hook name = { { NULL, NULL }, NULL, NULL, (APTR)(param) };
#endif
REXXHOOK(RexxHookA, REXX_PRINT);
REXXHOOK(RexxHookB, REXX_ABOUT);
REXXHOOK(RexxHookC, REXX_COPY);
REXXHOOK(RexxHookD, REXX_CLEARSELECTED);
REXXHOOK(RexxHookE, REXX_SNAPSHOT);
REXXHOOK(RexxHookF, REXX_WINDOWTOFRONT);
REXXHOOK(RexxHookG, REXX_WINDOWTOBACK);
REXXHOOK(RexxHookH, REXX_ACTIVATEWINDOW);
REXXHOOK(RexxHookI, REXX_SCREENTOFRONT);
REXXHOOK(RexxHookJ, REXX_SCREENTOBACK);
REXXHOOK(RexxHookK, REXX_OPEN);
REXXHOOK(RexxHookL, REXX_RELOAD);
REXXHOOK(RexxHookM, REXX_BACK);
REXXHOOK(RexxHookN, REXX_FORWARD);
REXXHOOK(RexxHookO, REXX_SAVEURL);
REXXHOOK(RexxHookP, REXX_ADDBOOKMARK);
REXXHOOK(RexxHookQ, REXX_GETURL);
REXXHOOK(RexxHookR, REXX_GETSELECTED);
REXXHOOK(RexxHookS, REXX_FULLSCREEN);
REXXHOOK(RexxHookT, REXX_GETTITLE);
REXXHOOK(RexxHookU, REXX_STATUS);

static const struct MUI_Command rexxcommands[] =
{
	{ "PRINT"         , NULL    , 0, (struct Hook *)&RexxHookA, { 0 } },
	{ "ABOUT"         , NULL    , 0, (struct Hook *)&RexxHookB, { 0 } },
	{ "COPY"          , NULL    , 0, (struct Hook *)&RexxHookC, { 0 } },
	{ "CLEARSELECTED" , NULL    , 0, (struct Hook *)&RexxHookD, { 0 } },
	{ "SNAPSHOT"      , NULL    , 0, (struct Hook *)&RexxHookE, { 0 } },
	{ "WINDOWTOFRONT" , NULL    , 0, (struct Hook *)&RexxHookF, { 0 } },
	{ "WINDOWTOBACK"  , NULL    , 0, (struct Hook *)&RexxHookG, { 0 } },
	{ "ACTIVATEWINDOW", NULL    , 0, (struct Hook *)&RexxHookH, { 0 } },
	{ "SCREENTOFRONT" , NULL    , 0, (struct Hook *)&RexxHookI, { 0 } },
	{ "SCREENTOBACK"  , NULL    , 0, (struct Hook *)&RexxHookJ, { 0 } },
	{ "OPEN"          , "NAME/K,NEWPAGE/S,BACKGROUND/S,SOURCE/S,FULLSCREEN/S", 5, (struct Hook *)&RexxHookK, { 0 } },
	{ "RELOAD"        , NULL    , 0, (struct Hook *)&RexxHookL, { 0 } },
	{ "BACK"          , NULL    , 0, (struct Hook *)&RexxHookM, { 0 } },
	{ "FORWARD"       , NULL    , 0, (struct Hook *)&RexxHookN, { 0 } },
	{ "SAVEURL"       , "URL/A,AS/K", 2, (struct Hook *)&RexxHookO, { 0 } },
	{ "ADDBOOKMARK"   , "TITLE/A,URL/A,ALIAS/K,MENU/S,QUICKLINK/S", 5, (struct Hook *)&RexxHookP, { 0 } },
	{ "GETURL"        , NULL    , 0, (struct Hook *)&RexxHookQ, { 0 } },
	{ "SELECTEDTEXT"  , NULL    , 0, (struct Hook *)&RexxHookR, { 0 } },
	{ "FULLSCREEN"    , "MODE/K", 1, (struct Hook *)&RexxHookS, { 0 } },
	{ "GETTITLE"      , NULL    , 0, (struct Hook *)&RexxHookT, { 0 } },
	{ "STATUS"        , NULL	, 0, (struct Hook *)&RexxHookU, { 0 } },
	{ NULL            , NULL    , 0, NULL, { 0 } }
};

#if OS(MORPHOS)
static LONG Rexx(void)
{
	struct Data *data;
	struct Hook *h = (struct Hook *)REG_A0;
	Object * obj = (Object *)REG_A2;
	IPTR *params = (IPTR *)REG_A1;

	data = (struct Data *) INST_DATA(OCLASS(obj), obj);
#endif
#if OS(AROS)
AROS_UFH3
(
    LONG, Rexx,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, params, A1)
)
{
    AROS_USERFUNC_INIT
#endif

	Object *window = (Object *) getv(obj, MA_OWBApp_ActiveWindow);

	if ((ULONG)h->h_Data == REXX_ABOUT)
	{
		DoMethod(obj, MM_OWBApp_About);
	}
	else if (window)
	{
		switch ((ULONG)h->h_Data)
		{
			case REXX_PRINT:
				DoMethod(window, MM_OWBWindow_Print);
				break;

			case REXX_COPY:
				//
				break;

			case REXX_CLEARSELECTED:
				//
				break;

			case REXX_GETSELECTED:
				{
	                Object *browser = (Object *) getv(window, MA_OWBWindow_ActiveBrowser);

					if(browser)
					{
						BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);

						if(widget)
						{
							char *selected = (char *) widget->webView->selectedText();

							if(selected)
							{
								char *converted = utf8_to_local(selected);
								if(converted)
								{
									set(app, MUIA_Application_RexxString, converted);
									free(converted);
								}

								free(selected);
							}
						}
					}
				}

				break;

			case REXX_SNAPSHOT:
				DoMethod(window, MUIM_Window_Snapshot);
				break;

			case REXX_WINDOWTOFRONT:
				DoMethod(window, MUIM_Window_ToFront);
				break;

			case REXX_WINDOWTOBACK:
				DoMethod(window, MUIM_Window_ToBack);
				break;

			case REXX_ACTIVATEWINDOW:
				set(window, MUIA_Window_Activate, TRUE);
				break;

			case REXX_SCREENTOFRONT:
				DoMethod(window, MUIM_Window_ScreenToFront);
				break;

			case REXX_SCREENTOBACK:
				DoMethod(window, MUIM_Window_ScreenToBack);
				break;

			case REXX_OPEN:
				{
					CONST_STRPTR file = (CONST_STRPTR)*params;
					ULONG newpage = (ULONG) params[1];
					ULONG background = (ULONG) params[2];
					ULONG source = (ULONG) params[3];
					ULONG fullscreen = (ULONG) params[4];

					if(source)
					{
						Object *browser = (Object *) getv(window, MA_OWBWindow_ActiveBrowser);

						if(browser)
						{
							BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);

							if(widget)
							{
								WebView *sourceview = widget->webView;
								DoMethod(app, MM_OWBApp_AddWindow, NULL, FALSE, sourceview, FALSE, NULL, FALSE);
							}
						}
					}
					else
					{
						if (newpage)
						{
							DoMethod(app, MM_OWBApp_AddPage, file, FALSE, background, NULL, NULL, FALSE, TRUE);
						}
						else
						{
							DoMethod(window, MM_OWBWindow_LoadURL, file, NULL);
						}

						if(fullscreen)
						{
							DoMethod(window, MM_OWBWindow_FullScreen, MV_OWBWindow_FullScreen_On);
						}
					}
				}
				break;

			case REXX_RELOAD:
				DoMethod(window, MM_OWBWindow_Reload, NULL);
				break;

			case REXX_BACK:
				DoMethod(window, MM_OWBWindow_Back);
				break;

			case REXX_FORWARD:
				DoMethod(window, MM_OWBWindow_Forward);
				break;

			case REXX_SAVEURL:
				{
					CONST_STRPTR url = (CONST_STRPTR)*params;
					CONST_STRPTR fullpath = (CONST_STRPTR) params[1];
					Object *browser = (Object *) getv(window, MA_OWBWindow_ActiveBrowser);

					if(browser)
					{
						BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);

						if(widget)
						{
							TransferSharedPtr<WebDownloadDelegate> downloadDelegate = widget->webView->downloadDelegate();

						    if(downloadDelegate)
						    {
								URL u(ParsedURLString, url);
								WebDownload* download = WebDownload::createInstance(u, downloadDelegate);
								
								if(download)
								{								 
									if(fullpath)
									{
										download->setDestination(fullpath, true, false);
									}
									download->start(true);
								}
						    }							 
						}
					}
				}
				break;

			case REXX_GETURL:
				{
                    Object *browser = (Object *) getv(window, MA_OWBWindow_ActiveBrowser);

					if(browser)
					{
						set(app, MUIA_Application_RexxString, getv(browser, MA_OWBBrowser_URL));
					}
				}
				break;

			case REXX_GETTITLE:
				{
                    Object *browser = (Object *) getv(window, MA_OWBWindow_ActiveBrowser);

					if(browser)
					{
						set(app, MUIA_Application_RexxString, getv(browser, MA_OWBBrowser_Title));
					}
				}
				break;

			case REXX_ADDBOOKMARK:
				{
					CONST_STRPTR title   = (CONST_STRPTR)*params;
					CONST_STRPTR address = (CONST_STRPTR) params[1];
					CONST_STRPTR alias   = (CONST_STRPTR) params[2];
					ULONG menu = (ULONG) params[3];
					ULONG quicklink = (ULONG) params[4];

					DoMethod(app, MM_Bookmarkgroup_AddLink, title, alias, address, TRUE, menu, quicklink);
				}
				break;

			case REXX_FULLSCREEN:
				{
					String arg   = String((CONST_STRPTR)*params);
					ULONG mode;
					if(arg == "ON")
						mode = MV_OWBWindow_FullScreen_On;
					else if(arg == "OFF")
						mode = MV_OWBWindow_FullScreen_Off;
					else
						mode = MV_OWBWindow_FullScreen_Toggle;

					DoMethod(window, MM_OWBWindow_FullScreen, mode);

					break;
				}
		}
	}

	return 0;
#if OS(AROS)
	AROS_USERFUNC_EXIT
#endif
}

/**************************************************/

String GetFilePart(URL url)
{
	String path = url.string().substring(url.pathAfterLastSlash());
	size_t questionPos = path.find('?');
	size_t hashPos = path.find('#');
	unsigned pathEnd;

	if (hashPos != notFound && (questionPos == notFound || questionPos > hashPos))
		pathEnd = hashPos;
	else if (questionPos != notFound)
		pathEnd = questionPos;
	else
		pathEnd = path.length();

	path = path.left(pathEnd);

	return path;
}

String GetURLWithoutArgument(String url)
{
	size_t questionPos = url.find('?');
	size_t hashPos = url.find('#');
	unsigned pathEnd;

	if (hashPos != notFound && (questionPos == notFound || questionPos > hashPos))
		pathEnd = hashPos;
	else if (questionPos != notFound)
		pathEnd = questionPos;
	else
		pathEnd = url.length();

	url	= url.left(pathEnd);

	return url;
}

/**************************************************/

String BufferFile(const char* url)
{
	String path = url;

	OWBFile *configFile = new OWBFile(path);
    if (!configFile)
        return String();
	if (configFile->open('r') == -1)
	{
        delete configFile;
        return String();
    }
	char *buffer = configFile->read(configFile->getSize());
	String fileBuffer = buffer;
	delete [] buffer;
    configFile->close();
    delete configFile;

	return fileBuffer;
}

void InitConfig(AppSettings *settings)
{
	settings->panelweight = 30;
	settings->showbookmarkpanel	= FALSE;
	settings->showhistorypanel = FALSE;
	settings->addbookmarkstomenu = TRUE;
	settings->continuousspellchecking = FALSE;
	settings->privatebrowsing = FALSE;
}

void ParseConfigFile(const char* url, AppSettings *settings)
{
	String fileBuffer = BufferFile(url);

	InitConfig(settings);

	while (!fileBuffer.isEmpty())
	{
		size_t eol = fileBuffer.find("\n");
		size_t delimiter = fileBuffer.find("=");

		if(eol == notFound)
			break;

        String keyword = fileBuffer.substring(0, delimiter).stripWhiteSpace();
        String key = fileBuffer.substring(delimiter +  1, eol - delimiter).stripWhiteSpace();

		if (keyword == "panelweight")
			settings->panelweight = key.toInt();

		if (keyword == "showbookmarkpanel")
			settings->showbookmarkpanel	= key.toInt();

		if (keyword == "showhistorypanel")
			settings->showhistorypanel = key.toInt();

		if (keyword == "addbookmarkstomenu")
			settings->addbookmarkstomenu = key.toInt();

		if (keyword == "continuousspellchecking")
			settings->continuousspellchecking = key.toInt();
		/*
		if (keyword == "privatebrowsing")
			settings->privatebrowsing = key.toInt(); */

		// add other options

        //Remove processed line from the buffer
        String truncatedBuffer = fileBuffer.substring(eol + 1, fileBuffer.length() - eol - 1);
        fileBuffer = truncatedBuffer;
    }
}

void WriteConfigFile(const char* url, AppSettings *settings)
{
	OWBFile *configFile = new OWBFile(url);
    if (!configFile)
		return;

	if (configFile->open('w') == -1)
	{
        delete configFile;
		return;
    }

	configFile->write(String::format("panelweight=%lu\n", (unsigned long)settings->panelweight));
	configFile->write(String::format("showbookmarkpanel=%lu\n", (unsigned long)settings->showbookmarkpanel));
	configFile->write(String::format("showhistorypanel=%lu\n", (unsigned long)settings->showhistorypanel));
	configFile->write(String::format("addbookmarkstomenu=%lu\n", (unsigned long)settings->addbookmarkstomenu));
	configFile->write(String::format("continuousspellchecking=%lu\n", (unsigned long)settings->continuousspellchecking));
	//configFile->write(String::format("privatebrowsing=%lu\n", (unsigned long)settings->privatebrowsing));
	
	configFile->close();
    delete configFile;
}

DEFNEW
{
	Object *prefswin, *menustrip;
	Object *splashwindow;
	struct DiskObject *diskobject;

	NEWLIST(&window_list);
	NEWLIST(&view_list);
	NEWLIST(&download_list);
	NEWLIST(&contextmenu_list);
	NEWLIST(&mimetype_list);
	NEWLIST(&urlsetting_list);
	NEWLIST(&family_list);

	installClipboardMonitor();

	menus_init();

	// Initialize JS (still needed?)
	JSC::initializeThreading();
	PlatformStrategiesMorphOS::initialize();
	//gcController().setJavaScriptGarbageCollectorTimerEnabled(true);

	obj = (Object *) DoSuperNew(cl, obj,
			MUIA_Application_Title      , "Odyssey Web Browser",
			MUIA_Application_Version    , "$VER: Odyssey Web Browser " VERSION " (" DATE ")",
			MUIA_Application_Copyright  , "©\n2009-2014 Fabien Coeurjoly\n2014-2015 Krzysztof Smiechowicz",
			MUIA_Application_Author     , "Fabien Coeurjoly",
			MUIA_Application_Description, APPLICATION_DESCRIPTION,
			MUIA_Application_UsedClasses, classlist,
			MUIA_Application_Base       , APPLICATION_BASE,
			MUIA_Application_DiskObject , diskobject = GetDiskObject(_ProgramName),
			MUIA_Application_Commands   , &rexxcommands,
			MUIA_Application_Menustrip  , menustrip = MUI_MakeObject(MUIO_MenustripNM, MenuData, MUIO_MenustripNM_CommandKeyCheck),
			SubWindow, prefswin = (Object *) NewObject(getprefswindowclass(), NULL, TAG_DONE),
			TAG_MORE, INITTAGS);

	if(obj)
	{
		GETDATA;

		// Set it as soon as possible since it might be referenced in several places
		app = obj;

		// Try to initialize most static objects crap here with that utf8 call (sigh, what a lame hack). Needed, because doing it in a thread could cause a crash. (still true?)
		String dummy = "dummyéè";
		stccpy(data->dummy, dummy.utf8().data(), sizeof(data->dummy));

		// Builtin defaults
		stccpy(data->homepage,     "", sizeof(data->homepage));
		stccpy(data->startpage,    "about:", sizeof(data->startpage));
		stccpy(data->newtabpage,   "topsites://", sizeof(data->newtabpage));
		stccpy(data->download_dir, "RAM:", sizeof(data->download_dir));
		stccpy(data->current_dir,  "", sizeof(data->current_dir));
		data->newpagepolicy = MV_OWBApp_NewPagePolicy_Tab;
		data->popuppolicy   = MV_OWBApp_NewPagePolicy_Tab;

		data->diskobject = diskobject;
		data->menustrip  = menustrip;
		data->prefswin   = prefswin;

		data->lastwin = NULL;
		data->downloads_in_progress = 0;
		data->privatebrowsing_clients = 0;

		/* Splash Window */
		/* XXX: Should fail with app creation if fontconfig encountered a fatal error */
		splashwindow = (Object *) NewObject(getsplashwindowclass(), NULL, TAG_DONE);

		if(splashwindow)
		{
			DoMethod(obj, OM_ADDMEMBER, splashwindow);
			DoMethod(splashwindow, MM_SplashWindow_Open);
			DoMethod(obj, OM_REMMEMBER, splashwindow);
			MUI_DisposeObject(splashwindow);
		}
		else
		{
			CoerceMethod(cl, obj, OM_DISPOSE);
			return NULL;
		}

		/* Load settings */
		DoMethod(obj, MM_OWBApp_PrefsLoad);

		/* Create additional windows */
		data->dlwin              = (Object *) NewObject(getdownloadwindowclass(), NULL, TAG_DONE);
		data->bookmarkwin        = (Object *) NewObject(getbookmarkwindowclass(), NULL, TAG_DONE);
		data->networkwin         = (Object *) NewObject(getnetworkwindowclass(), NULL, TAG_DONE);
		data->consolewin         = (Object *) NewObject(getconsolewindowclass(), NULL, TAG_DONE);
		data->passwordmanagerwin = (Object *) NewObject(getpasswordmanagerwindowclass(), NULL, TAG_DONE);
		data->cookiemanagerwin   = (Object *) NewObject(getcookiemanagerwindowclass(), NULL, TAG_DONE);
		data->blockmanagerwin    = (Object *) NewObject(getblockmanagerwindowclass(), NULL, TAG_DONE);
		data->searchmanagerwin   = (Object *) NewObject(getsearchmanagerwindowclass(), NULL, TAG_DONE);
		data->scriptmanagerwin   = (Object *) NewObject(getscriptmanagerwindowclass(), NULL, TAG_DONE);
		data->urlprefswin        = (Object *) NewObject(geturlprefswindowclass(), NULL, TAG_DONE);
		data->printerwin         = (Object *) NewObject(getprinterwindowclass(), NULL, TAG_DONE);

		DoMethod(obj, OM_ADDMEMBER, data->dlwin);
		DoMethod(obj, OM_ADDMEMBER, data->bookmarkwin);
		DoMethod(obj, OM_ADDMEMBER, data->networkwin);
		DoMethod(obj, OM_ADDMEMBER, data->consolewin);
		DoMethod(obj, OM_ADDMEMBER, data->passwordmanagerwin);
		DoMethod(obj, OM_ADDMEMBER, data->cookiemanagerwin);
		DoMethod(obj, OM_ADDMEMBER, data->blockmanagerwin);
		DoMethod(obj, OM_ADDMEMBER, data->searchmanagerwin);
		DoMethod(obj, OM_ADDMEMBER, data->scriptmanagerwin);
		DoMethod(obj, OM_ADDMEMBER, data->urlprefswin);
		DoMethod(obj, OM_ADDMEMBER, data->printerwin);

		/* Load bookmarks */
		DoMethod(obj, MM_OWBApp_LoadBookmarks);

		/* Load history */
		WebHistory::sharedHistory();

		/* Load filters */
		DoMethod(data->blockmanagerwin, MM_BlockManagerGroup_Load);

		/* Load Search engines */
		DoMethod(data->searchmanagerwin, MM_SearchManagerGroup_Load);

		/* Load user scripts */
		DoMethod(data->scriptmanagerwin, MM_ScriptManagerGroup_Load);

		/* Load URL settings */
		DoMethod(data->urlprefswin, MM_URLPrefsGroup_Load);

		/* Restore session / create the first browser window */
		ULONG restoremode = getv(app, MA_OWBApp_SessionRestoreMode);

		if(restoremode == MV_OWBApp_SessionRestore_Off || DoMethod(app, MM_OWBApp_RestoreSession, restoremode == MV_OWBApp_SessionRestore_Ask ? TRUE : FALSE, FALSE) == FALSE)
		{
			char *starturl = (char *) GetTagData(MA_OWBBrowser_URL, NULL, msg->ops_AttrList);
			DoMethod(app, MM_OWBApp_AddWindow, (starturl && starturl[0]) ? starturl : data->startpage, FALSE, NULL, TRUE, NULL, FALSE);
		}
		else
		{
			char *starturl = (char *) GetTagData(MA_OWBBrowser_URL, NULL, msg->ops_AttrList);
			if(starturl && starturl[0])
			{
				DoMethod(app, MM_OWBApp_AddBrowser, NULL, starturl, FALSE, NULL, FALSE, FALSE, TRUE);
			}
		}

		// Notifications on app
		DoMethod(app, MUIM_Notify, MA_OWBApp_BookmarksChanged, MUIV_EveryTime, obj, 1, MM_OWBApp_BookmarksChanged);
	}

	return (ULONG)obj;
}

void FreeWebKitLeakedObjects()
{
  static int freed = FALSE;

  if(!freed)
  {
	/* Seems webkit leaks all plugins, sigh, let's do it ourselves */
	PluginDatabase * sharedPluginDatabase = PluginDatabase::installedPlugins();
	Vector<PluginPackage *> plugins = sharedPluginDatabase->plugins();
	for(size_t i = 0; i < plugins.size(); i++)
	{
		plugins[i]->unload();
	}

#if ENABLE(VIDEO)
	/* Media instances might be leaked as well... */
	WebCore::freeLeakedMediaObjects();
#endif

	/* Yup, built as an indestructible singleton, sigh. ;) */
	cookieManager().destroy();

	/* More to come? :) */
	freed = TRUE;
  }
}

DEFDISP
{
	GETDATA;
	APTR n, m;

	ITERATELISTSAFE(n, m, &window_list)
	{
		struct windownode *wn = (struct windownode *) n;

		set(wn->window, MUIA_Window_Open, FALSE);
		DoMethod(app, OM_REMMEMBER, wn->window);
		MUI_DisposeObject(wn->window);
	}

	methodstack_cleanup_flush();

	/* Free prefs objects */
	ITERATELISTSAFE(n, m, &family_list)
	{
		REMOVE(n);
		free(n);
	}

	FreeDiskObject(data->diskobject);

	/* Write persistant config file */
	WriteConfigFile("PROGDIR:Conf/misc.prefs", &data->appsettings);

	if(getv(app, MA_OWBApp_DeleteSessionAtExit))
	{
		DeleteFile(SESSION_PATH);
	}

	/* Free shared instances that really need to be freed */
	
	WebIconDatabase *sharedWebIconDatabase = WebIconDatabase::sharedWebIconDatabase();
	if(sharedWebIconDatabase)
	{
		delete sharedWebIconDatabase;
	}

	/* Free resource manager, since it doesn't seem freed anywhere else */
	ResourceHandleManager *sharedResourceHandleManager = ResourceHandleManager::sharedInstance();

	if(sharedResourceHandleManager)
	{
		delete sharedResourceHandleManager;
	}

	FreeWebKitLeakedObjects();

	removeClipboardMonitor();

	//kprintf("OWBApp: Ok, calling supermethod\n");

	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
/* Settings */
		case MA_OWBApp_DownloadDirectory:
		{
			*msg->opg_Storage = (ULONG) data->download_dir;
		}
		return TRUE;

		case MA_OWBApp_DownloadAutoClose:
		{
			*msg->opg_Storage = (ULONG) data->downloadautoclose;
		}
		return TRUE;

		case MA_OWBApp_DownloadSave:
		{
			*msg->opg_Storage = (ULONG) data->savedownloads;
		}
		return TRUE;

		case MA_OWBApp_DownloadStartAutomatically:
		{
			*msg->opg_Storage = (ULONG) data->downloadstartautomatically;
		}
		return TRUE;

		case MA_OWBApp_DefaultURL:
		{
			*msg->opg_Storage = (ULONG) data->homepage;
		}
		return TRUE;

		case MA_OWBApp_StartPage:
		{
			*msg->opg_Storage = (ULONG) data->startpage;
		}
		return TRUE;

		case MA_OWBApp_NewTabPage:
		{
			*msg->opg_Storage = (ULONG) data->newtabpage;
		}
		return TRUE;

		case MA_OWBApp_CLIDevice:
		{
			*msg->opg_Storage = (ULONG) getv(data->prefswin, MA_OWBApp_CLIDevice);
		}
		return TRUE;

		case MA_OWBApp_ShowValidationButtons:
		{
			*msg->opg_Storage = (ULONG) data->showvalidationbuttons;
		}
		return TRUE;

		case MA_OWBApp_ShowSeparators:
		{
			*msg->opg_Storage = (ULONG) data->showseparators;
		}
		return TRUE;

		case MA_OWBApp_EnablePointers:
		{
			*msg->opg_Storage = (ULONG) data->enablepointers;
		}
		return TRUE;

		case MA_OWBApp_ShowButtonFrame:
		{
			*msg->opg_Storage = (ULONG) data->showbuttonframe;
		}
		return TRUE;

		case MA_OWBApp_ShowSearchBar:
		{
			*msg->opg_Storage = (ULONG) data->showsearchbar;
		}
		return TRUE;

		case MA_OWBApp_EnableTabTransferAnim:
		{
			*msg->opg_Storage = (ULONG) data->enabletabtransferanim;
		}
		return TRUE;

		case MA_OWBApp_NewPagePolicy:
		{
			*msg->opg_Storage = (ULONG) data->newpagepolicy;
		}
		return TRUE;

		case MA_OWBApp_NewPagePosition:
		{
			*msg->opg_Storage = (ULONG) data->newpageposition;
		}
		return TRUE;

		case MA_OWBApp_PopupPolicy:
		{
			*msg->opg_Storage = (ULONG) data->popuppolicy;
		}
		return TRUE;

		case MA_OWBApp_ToolButtonType:
		{
			*msg->opg_Storage = (ULONG) data->toolbuttontype;
		}
		return TRUE;

		case MA_OWBApp_URLCompletionType:
		{
			*msg->opg_Storage = (ULONG) data->urlcompletiontype;
		}
		return TRUE;

		case MA_OWBApp_ShouldShowCloseRequester:
		{
			ULONG ret;
			ULONG policy = getv(data->prefswin, MA_OWBApp_CloseRequester);
			switch(policy)
			{
				default:
				case MV_OWBApp_CloseRequester_Always:
					ret = TRUE;
					break;

				case MV_OWBApp_CloseRequester_Never:
					ret = FALSE;
					break;

				case MV_OWBApp_CloseRequester_IfSeveralTabs:
				{
					APTR n;
					ULONG count = 0;

					ret = FALSE;

					ITERATELIST(n, &view_list)
					{
						count++;
					}

					if(count > 1)
					{
						ret = TRUE;
					}

					break;
				}
			}

			*msg->opg_Storage = (ULONG) ret;
		}
		return TRUE;

		case MA_OWBApp_ShouldAnimate:
		{
			ULONG ret = getv(data->prefswin, MA_OWBApp_EnableAnimation);

			#if 0
			ULONG ret = FALSE;
			ULONG policy = getv(data->prefswin, MA_OWBApp_AnimationPolicy);

			// Animation are allowed
			if(policy)
			{
				ret = TRUE;

				// Animations are allowed, only if window is active
				if(policy == 2)
				{
					Object *window = (Object *) getv(app, MA_OWBApp_ActiveWindow);

					ret &= getv(window, MUIA_Window_Activate);
				}
			}
			#endif
			*msg->opg_Storage = (ULONG) ret;
		}
		return TRUE;

		case MA_OWBApp_ErrorMode:
		{
			*msg->opg_Storage = (ULONG) data->errormode;
		}
		return TRUE;

		case MA_OWBApp_MiddleButtonPolicy:
		{
			*msg->opg_Storage = (ULONG) data->middlebuttonpolicy;
		}
		return TRUE;

		case MA_OWBApp_ShowFavIcons:
		{
			*msg->opg_Storage = (ULONG) data->showfavicons;
		}
		return TRUE;

		case MA_OWBApp_DidReceiveFavIcon:
		{
			*msg->opg_Storage = (ULONG) data->favicon_url;
		}
		return TRUE;


		case MA_OWBApp_FavIconImportComplete:
		{
			*msg->opg_Storage = (ULONG) data->favicon_import_complete;
		}
		return TRUE;

		case MA_OWBApp_QuickLinkLook:
		{
			*msg->opg_Storage = (ULONG) data->quicklinklook;
		}
		return TRUE;

		case MA_OWBApp_QuickLinkLayout:
		{
			*msg->opg_Storage = (ULONG) data->quicklinklayout;
		}
		return TRUE;

		case MA_OWBApp_QuickLinkRows:
		{
			*msg->opg_Storage = (ULONG) data->quicklinkrows;
		}
		return TRUE;

		case MA_OWBApp_QuickLinkPosition:
		{
			*msg->opg_Storage = (ULONG) data->quicklinkposition;
		}
		return TRUE;

		case MA_OWBApp_SaveSession:
		{
			*msg->opg_Storage = (ULONG) data->savesession;
		}
		return TRUE;

		case MA_OWBApp_DeleteSessionAtExit:
		{
			*msg->opg_Storage = (ULONG) data->deletesessionatexit;
		}
		return TRUE;

		case MA_OWBApp_SessionRestoreMode:
		{
			*msg->opg_Storage = (ULONG) data->sessionrestoremode;
		}
		return TRUE;

		case MA_OWBApp_SaveCookies:
		{
			*msg->opg_Storage = (ULONG) data->savecookies;
		}
		return TRUE;

		case MA_OWBApp_CookiesPolicy:
		{
			*msg->opg_Storage = (ULONG) data->cookiespolicy;
		}
		return TRUE;

/*
		case MA_OWBApp_PersistantCookiesPolicy:
		{
			*msg->opg_Storage = (ULONG) data->temporarycookiespolicy;
		}
		return TRUE;

		case MA_OWBApp_TemporaryCookiesPolicy:
		{
			*msg->opg_Storage = (ULONG) data->persistantcookiespolicy;
		}
		return TRUE;
*/

		case MA_OWBApp_SaveFormCredentials:
		{
			*msg->opg_Storage = (ULONG) data->savecredentials;
		}
		return TRUE;

		case MA_OWBApp_EnableFormAutofill:
		{
			*msg->opg_Storage = (ULONG) data->formautofill;
		}
		return TRUE;

		case MA_OWBApp_SaveHistory:
		{
			*msg->opg_Storage = (ULONG) data->savehistory;
		}
		return TRUE;

		case MA_OWBApp_HistoryItemLimit:
		{
			*msg->opg_Storage = (ULONG) data->historyitems;
		}
		return TRUE;

		case MA_OWBApp_HistoryAgeInDaysLimit:
		{
			*msg->opg_Storage = (ULONG) data->historyage;
		}
		return TRUE;

		case MA_OWBApp_UserAgent:
		{
			*msg->opg_Storage = (ULONG) data->useragent;
		}
		return TRUE;

		case MA_OWBApp_IgnoreSSLErrors:
		{
			*msg->opg_Storage = (ULONG) data->ignoreSSLErrors;
		}
		return TRUE;

		case MA_OWBApp_CertificatePath:
		{
			*msg->opg_Storage = (ULONG) data->certificate_path;
		}
		return TRUE;

		case MA_OWBApp_EnableInspector:
		{
			*msg->opg_Storage = (ULONG) data->inspector;
		}
		return TRUE;

		case MA_OWBApp_EnableVP8:
		{
			*msg->opg_Storage = (ULONG) data->use_webm;
		}
		return TRUE;

		case MA_OWBApp_EnableFLV:
		{
			*msg->opg_Storage = (ULONG) data->use_flv;
		}
		return TRUE;

		case MA_OWBApp_EnableOgg:
		{
			*msg->opg_Storage = (ULONG) data->use_ogg;
		}
		return TRUE;

		case MA_OWBApp_EnablePartialContent:
		{
			*msg->opg_Storage = (ULONG) data->use_partial_content;
		}
		return TRUE;

		case MA_OWBApp_LoopFilterMode:
		{
			*msg->opg_Storage = (ULONG) data->loopfilter_mode;
		}
		return TRUE;

/* Sticky Settings */

		case MA_OWBApp_ShowBookmarkPanel:
		{
			*msg->opg_Storage = (ULONG) data->appsettings.showbookmarkpanel;
		}
		return TRUE;

		case MA_OWBApp_ShowHistoryPanel:
		{
			*msg->opg_Storage = (ULONG) data->appsettings.showhistorypanel;
		}
		return TRUE;

		case MA_OWBApp_PanelWeight:
		{
			*msg->opg_Storage = (ULONG) data->appsettings.panelweight;
		}
		return TRUE;

		case MA_OWBApp_Bookmark_AddToMenu:
		{
			*msg->opg_Storage = (ULONG) data->appsettings.addbookmarkstomenu;
		}
		return TRUE;

		case MA_OWBApp_ContinuousSpellChecking:
		{
			*msg->opg_Storage = (ULONG) data->appsettings.continuousspellchecking;
		}
		return TRUE;

		case MA_OWBApp_PrivateBrowsing:
		{
			*msg->opg_Storage = (ULONG) data->appsettings.privatebrowsing;
		}
		return TRUE;

/* Generic */

		case MA_OWBApp_CurrentDirectory:
		{
			*msg->opg_Storage = (ULONG) data->current_dir;
		}
		return TRUE;

		case MA_OWBApp_ActiveBrowser:
		{
			Object *browser = NULL;
			Object *window = (Object *) getv(app, MA_OWBApp_ActiveWindow);

			if(window)
			{
				browser = (Object *) getv(window, MA_OWBWindow_ActiveBrowser);
			}

			*msg->opg_Storage = (ULONG) browser;
		}
		return TRUE;

		case MA_OWBApp_ActiveWindow:
		{
			Object *window = NULL;
			Object *firstwindow = NULL;

			/* Get the target window */
			FORCHILD(obj, MUIA_Application_WindowList)
			{
				if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
				{
					if(!firstwindow)
					{
						firstwindow = (Object *) child;
					}

					if (getv(child, MUIA_Window_Activate) || child == data->lastwin)
					{
						window = (Object *) child;
						break;
					}
				}
			}
			NEXTCHILD

			if(!window)
			{
				window = firstwindow;
			}

			*msg->opg_Storage = (ULONG) window;
		}
		return TRUE;

		case MA_OWBApp_BookmarkWindow:
		{
			*msg->opg_Storage = (ULONG) data->bookmarkwin;
		}
		return TRUE;

		case MA_OWBApp_DownloadWindow:
		{
			*msg->opg_Storage = (ULONG) data->dlwin;
		}
		return TRUE;

		case MA_OWBApp_SearchManagerWindow:
		{
			*msg->opg_Storage = (ULONG) data->searchmanagerwin;
		}
		return TRUE;

		case MA_OWBApp_ScriptManagerWindow:
		{
			*msg->opg_Storage = (ULONG) data->scriptmanagerwin;
		}
		return TRUE;

		case MA_OWBApp_PrinterWindow:
		{
			*msg->opg_Storage = (ULONG) data->printerwin;
		}
		return TRUE;

		case MA_OWBApp_DownloadsInProgress:
		{
			*msg->opg_Storage = (ULONG) data->downloads_in_progress;
		}
		return TRUE;

		case MA_OWBApp_HistoryChanged:
		{
			*msg->opg_Storage = (ULONG) data->history_changed;
		}
		return TRUE;

		case MA_OWBApp_BookmarksChanged:
		{
			*msg->opg_Storage = (ULONG) data->bookmarks_changed;
		}
		return TRUE;

		case MA_OWBApp_PrivateBrowsingClients:
		{
			*msg->opg_Storage = (ULONG) data->privatebrowsing_clients;
		}
		return TRUE;
	}


	return DOSUPER;
}

static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	struct TagItem *tag, *tstate;

	tstate = tags;

	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		ULONG tdata = tag->ti_Data;

		switch (tag->ti_Tag)
		{
			case MA_OWBApp_CurrentDirectory:
				stccpy(data->current_dir, (char *) tdata, sizeof(data->current_dir));
				break;

			case MA_OWBApp_LastActiveWindow:
				data->lastwin = (Object *) tdata;
				break;

			case MA_OWBApp_DownloadsInProgress:
				if((LONG) tdata >= 0)
					data->downloads_in_progress = tdata;
				else
					data->downloads_in_progress = 0;
				break;

			case MA_OWBApp_PrivateBrowsingClients:
				if((LONG) tdata >= 0)
					data->privatebrowsing_clients = tdata;
				else
					data->privatebrowsing_clients = 0;
				break;

/* Sticky Settings */
			case MA_OWBApp_ShowBookmarkPanel:
				data->appsettings.showbookmarkpanel = tdata;
				break;

			case MA_OWBApp_ShowHistoryPanel:
				data->appsettings.showhistorypanel = tdata;
				break;

			case MA_OWBApp_PanelWeight:
				data->appsettings.panelweight = tdata;
				break;

			case MA_OWBApp_Bookmark_AddToMenu:
				data->appsettings.addbookmarkstomenu = tdata;
				break;

			case MA_OWBApp_ContinuousSpellChecking:
				data->appsettings.continuousspellchecking = tdata;
				break;


/* Notifications flags */
			case MA_OWBApp_DidReceiveFavIcon:
				data->favicon_url = (STRPTR) tdata;
				break;

			case MA_OWBApp_FavIconImportComplete:
				data->favicon_import_complete = tdata;
				break;

			case MA_OWBApp_HistoryChanged:
				data->history_changed = tdata;
				break;

			case MA_OWBApp_BookmarksChanged:
				data->bookmarks_changed = tdata;
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

DEFSMETHOD(OWBApp_DisposeObject)
{
	if(msg->obj)
	{
		MUI_DisposeObject((Object *)msg->obj);
	}

	return (TRUE);
}

DEFSMETHOD(OWBApp_DisposeWindow)
{
	if(msg->obj)
	{
		DoMethod(obj, OM_REMMEMBER, msg->obj);
		MUI_DisposeObject((Object *)msg->obj);
	}

	return (TRUE);
}

DEFTMETHOD(OWBApp_About)
{
	GETDATA;

	if (!data->aboutwin)
	{
		data->aboutwin = AboutboxObject,
							MUIA_Aboutbox_Credits, credits,
							MUIA_Aboutbox_Build, REVISION,
							End;

		if (data->aboutwin)
		{
			DoMethod(obj, OM_ADDMEMBER, data->aboutwin);
		}
	}

	if (data->aboutwin)
	{
		set(data->aboutwin, MUIA_Window_Open, TRUE);
	}

	return 0;
}

DEFTMETHOD(OWBApp_Quit)
{
	if(getv(app, MA_OWBApp_DownloadsInProgress) > 0)
	{
		if(MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_REQUESTER_YES_NO), GSI(MSG_OWBWINDOW_PENDING_DOWNLOADS), NULL))
		{
			DoMethod((Object *) getv(app, MA_OWBApp_DownloadWindow), MM_Download_Cancel, TRUE);
		}
		else
		{
			return 0;
		}
	}
	else if(getv(app, MA_OWBApp_ShouldShowCloseRequester))
	{
		if(!MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_REQUESTER_YES_NO), GSI(MSG_OWBWINDOW_QUIT_CONFIRMATION), NULL))
			return 0;
	}

	DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	return 0;
}

DEFSMETHOD(OWBApp_OpenWindow)
{
	GETDATA;
	Object *win = NULL;

	switch (msg->window_id)
	{
		case MV_OWB_Window_Settings:
			win = data->prefswin;
			break;

		case MV_OWB_Window_Downloads:
			if(getv(data->dlwin, MUIA_Window_Open) && !msg->activate) return 0;
			win = data->dlwin;
			break;

		case MV_OWB_Window_Bookmarks:
			win = data->bookmarkwin;
			break;

		case MV_OWB_Window_Network:
			win = data->networkwin;
			break;

		case MV_OWB_Window_Console:
			win = data->consolewin;
			break;

		case MV_OWB_Window_PasswordManager:
			win = data->passwordmanagerwin;
			break;

		case MV_OWB_Window_CookieManager:
			win = data->cookiemanagerwin;
			break;

		case MV_OWB_Window_BlockManager:
			win = data->blockmanagerwin;
			break;

		case MV_OWB_Window_SearchManager:
			win = data->searchmanagerwin;
			break;

		case MV_OWB_Window_ScriptManager:
			win = data->scriptmanagerwin;
			break;

		case MV_OWB_Window_Printer:
			win = data->printerwin;
			break;

		case MV_OWB_Window_URLPrefs:
			win = data->urlprefswin;
			break;
	}

	return set(win, MUIA_Window_Open, TRUE);
}

/* Adds a new tab or window depending on setting */
DEFSMETHOD(OWBApp_AddPage)
{
	GETDATA;
	BalWidget *widget = NULL;

	if(data->newpagepolicy == MV_OWBApp_NewPagePolicy_Window)
	{
		widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddWindow, msg->url, msg->isframe, NULL, FALSE, msg->windowfeatures, msg->privatebrowsing);
	}
	else if(data->newpagepolicy == MV_OWBApp_NewPagePolicy_Tab)
	{
		widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddBrowser, msg->window, msg->url, msg->isframe, NULL, msg->donotactivate, msg->privatebrowsing, msg->addtoend);
	}

	return (ULONG) widget;
}

/* Add a new window */
DEFSMETHOD(OWBApp_AddWindow)
{
	GETDATA;

	WindowFeatures *features = (WindowFeatures *) msg->windowfeatures;
	BalWidget *widget = NULL;

	Object *window = (Object *) create_window((char *) msg->url, msg->isframe, msg->sourceview, msg->windowfeatures, msg->privatebrowsing);

	if(window)
	{
		Object *browser;

		DoMethod(app, OM_ADDMEMBER, window);

		browser	= (Object *) getv(window, MA_OWBWindow_ActiveBrowser);

		if(browser)
		{
			widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
		}

		set(window, MUIA_Window_Open, TRUE);

		// XXX: move this feature stuff to OWBWindow constructor

		if(features)
		{
			set(window, MA_OWBWindow_ShowNavigationGroup, features->toolBarVisible);
			set(window, MA_OWBWindow_ShowQuickLinkGroup, features->toolBarVisible);
			set(window, MA_OWBWindow_ShowAddressBarGroup, features->locationBarVisible);
			set(window, MA_OWBWindow_ShowStatusGroup, features->statusBarVisible);

			if((features->xSet || features->ySet || features->widthSet || features->heightSet))
			{
				ULONG id = getv(window, MUIA_Window_ID);

				set(window, MUIA_Window_ID, 0);

				SetAttrs(window,
						 features->xSet      ? MUIA_Window_LeftEdge : TAG_IGNORE, (LONG) features->x,
						 features->ySet      ? MUIA_Window_TopEdge  : TAG_IGNORE, (LONG) features->y,
						 features->widthSet  ? MUIA_Window_Width    : TAG_IGNORE, (LONG) features->width,
						 features->heightSet ? MUIA_Window_Height   : TAG_IGNORE, (LONG) features->height,
						 TAG_DONE);

				set(window, MUIA_Window_ID, id);
			}

			// XXX: handle, resizable, fullscreen, scrollbars attributes
		}

		// Only focus URL if needed (i.e manual new window)
		if(msg->urlfocus)
		{
			set(window, MA_AddressBarGroup_Active, TRUE);
		}


		if(!features || features->locationBarVisible)
		{
			set(window, MA_OWBWindow_ShowSearchGroup, data->showsearchbar);
		}
	}

	return (ULONG) widget;
}

/* Remove a window */
DEFSMETHOD(OWBApp_RemoveWindow)
{
	Object *window = msg->window;

	set(window, MUIA_Window_Open, FALSE);

	DoMethod(app, OM_REMMEMBER, window);

	MUI_DisposeObject(window);

	DoMethod(app, MM_OWBApp_SaveSession, FALSE);

	return 0;
}

/* Create a new browser */
DEFSMETHOD(OWBApp_AddBrowser)
{
	BalWidget *widget = NULL;
	Object *window = msg->window;

	if(!window)
	{
		window = (Object *) getv(app, MA_OWBApp_ActiveWindow);
	}

	if(window)
	{
		widget = (BalWidget *) DoMethod(window, MM_OWBWindow_AddBrowser, msg->url, msg->isframe, msg->sourceview, msg->donotactivate, msg->privatebrowsing, msg->addtoend);
	}

	return (ULONG) widget;
}

/* Remove a browser */
DEFSMETHOD(OWBApp_RemoveBrowser)
{
	if(msg->browser && !getv(msg->browser, MA_OWBBrowser_ForbidEvents))
	{
		DoMethod(_win(msg->browser), MM_OWBWindow_RemoveBrowser, msg->browser);
		DoMethod(app, MM_OWBApp_SaveSession, FALSE);
	}
	return 0;
}

/* Refresh each active browser */
DEFTMETHOD(OWBApp_Expose)
{
	APTR n, m;

	ITERATELISTSAFE(n, m, &window_list)
	{
		struct windownode *node = (struct windownode *) n;

		if(getv((Object *) node->window, MUIA_Window_Open))
		{
			Object *browser = (Object *) getv(node->window, MA_OWBWindow_ActiveBrowser);

			if(browser)
			{
				BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
				if(widget && widget->expose)
				{
					DoMethod(browser, MM_OWBBrowser_UpdateScrollers);

					widget->expose = false;
					BalEventExpose ev = 0;
					widget->webView->onExpose(ev);
				}
			}

			browser = (Object *) getv(node->window, MA_OWBWindow_ActiveWebInspector);

			if(browser)
			{
				BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
				if(widget && widget->expose)
				{
					DoMethod(browser, MM_OWBBrowser_UpdateScrollers);

					widget->expose = false;
					BalEventExpose ev = 0;
					widget->webView->onExpose(ev);
				}
			}
		}
	}
	return 0;
}

/* Run webkit events for each active browser */
DEFTMETHOD(OWBApp_WebKitEvents)
{
	APTR n, m;

	ITERATELISTSAFE(n, m, &window_list)
	{
		struct windownode *node = (struct windownode *) n;
		Object *browser = (Object *) getv(node->window, MA_OWBWindow_ActiveBrowser);

		if(browser && !getv(browser, MA_OWBBrowser_ForbidEvents))
		{
			BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
			if(widget)
			{
				widget->webView->fireWebKitThreadEvents();
				widget->webView->fireWebKitTimerEvents();
			}
		}

		browser = (Object *) getv(node->window, MA_OWBWindow_ActiveWebInspector);

		if(browser && !getv(browser, MA_OWBBrowser_ForbidEvents))
		{
			BalWidget *widget = (BalWidget *) getv(browser, MA_OWBBrowser_Widget);
			if(widget)
			{
				widget->webView->fireWebKitThreadEvents();
				widget->webView->fireWebKitTimerEvents();
			}
		}
	}
	return 0;
}

/* Download Management */

DEFSMETHOD(OWBApp_Download)
{
	GETDATA;
	URL url(ParsedURLString, msg->url);
	WebDownload * webDownload = (WebDownload *) msg->webdownload;
	WebDownloadPrivate *priv = NULL;
	struct downloadnode *dl = NULL;
	String lastComponent;
	STRPTR localFilename = NULL;
	bool quiet = false;

	if(webDownload && (priv = webDownload->getWebDownloadPrivate()) && priv->quiet)
	{
		quiet = true;
	}

	lastComponent = decodeURLEscapeSequences(msg->suggestedname ? String(msg->suggestedname) : GetFilePart(url));
	lastComponent.replace(":", "");
	lastComponent.replace("/", ""); // XXX: any other to filter?
	localFilename = utf8_to_local(lastComponent.utf8().data());

	if(localFilename)
	{
		if(quiet)
		{
			/* if destination path is set, enforce it */
			if(!priv->destinationPath.isEmpty())
			{
				char path[512];
				char file[512];

				stccpy(path, priv->destinationPath.latin1().data(), sizeof(path));
				stccpy(file, FilePart(path), sizeof(file));
				path[FilePart(path) - path] = 0;

				dl = download_create(msg->url, file, path, (char *)(priv ? priv->originURL.utf8().data() : ""));
			}
			else
			{
				dl = download_create(msg->url, localFilename, data->download_dir, (char *)(priv ? priv->originURL.utf8().data() : ""));
			}
		}
		else
		{
			struct FileRequester *filereq = (struct FileRequester *)MUI_AllocAslRequest(ASL_FileRequest, NULL);

			if (filereq)
			{
				set(app, MUIA_Application_Sleep, TRUE);

				if (MUI_AslRequestTags(filereq,
					ASLFR_TitleText, "Save As...",
					ASLFR_DoSaveMode, TRUE,
					ASLFR_InitialFile, localFilename,
					ASLFR_InitialDrawer, data->current_dir[0]? data->current_dir : data->download_dir,
					TAG_DONE))
				{
					dl = download_create(msg->url, filereq->fr_File, filereq->fr_Drawer, (char *)(priv ? priv->originURL.utf8().data() : ""));
					
					if (dl)
					{
						set(app, MA_OWBApp_CurrentDirectory, filereq->fr_Drawer);
					}
				}

				set(app, MUIA_Application_Sleep, FALSE);

				MUI_FreeAslRequest(filereq);
			}
		}

		free(localFilename);
	}

	if(dl)
	{
		DoMethod(data->dlwin, MUIM_List_InsertSingle, dl, MUIV_List_Insert_Bottom);
	}

	return (ULONG)dl;
}

DEFSMETHOD(OWBApp_DownloadUpdate)
{
	GETDATA;
	return DoMethod(data->dlwin, MUIM_List_Redraw, MUIV_List_Redraw_Entry, msg->entry);
}

DEFSMETHOD(OWBApp_DownloadDone)
{
	GETDATA;
	return DoMethod(data->dlwin, MM_Download_Done, msg->entry);
}

DEFSMETHOD(OWBApp_DownloadError)
{
	GETDATA;
	return DoMethod(data->dlwin, MM_Download_Error, msg->entry, msg->error);
}

DEFSMETHOD(OWBApp_DownloadCancelled)
{
	GETDATA;
	return DoMethod(data->dlwin, MM_Download_Cancelled, msg->entry);
}

/* Bookmarks/Quicklinks management */

DEFSMETHOD(Bookmarkgroup_AddLink)
{
	GETDATA;
	return DoMethodA(data->bookmarkwin, (_Msg_*) msg);
}

DEFSMETHOD(Bookmarkgroup_RemoveQuickLink)
{
	GETDATA;
	return DoMethodA(data->bookmarkwin, (_Msg_*) msg);
}

DEFTMETHOD(OWBApp_LoadBookmarks)
{
	GETDATA;

	if (DoMethod(data->bookmarkwin, MM_Bookmarkgroup_LoadHtml, BOOKMARK_PATH) == MV_Bookmark_ImportHtml_WrongType)
	{
		MUI_RequestA(obj, NULL, 0, GSI(MSG_REQUESTER_ERROR_TITLE), GSI(MSG_REQUESTER_OK), GSI(MSG_REQUESTER_BOOKMARK_IMPORT_ERROR), NULL);
	}

	DoMethod(data->bookmarkwin, MM_Bookmarkgroup_Update);

	return 0;
}

DEFTMETHOD(OWBApp_BookmarksChanged)
{
	DoMethod(app, MUIM_Application_PushMethod,
			 app, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE,
			 MM_OWBApp_UpdateBookmarkPanels);

	return 0;
}

/* Prefs management */

void prefs_update(Object *obj, struct Data *data)
{
	WebPreferences* sharedPreferences = WebPreferences::sharedStandardPreferences();

	/* Directories */
	stccpy(data->download_dir, (char *) getv(data->prefswin, MA_OWBApp_DownloadDirectory), sizeof(data->download_dir));
	stccpy(data->homepage, (char *) getv(data->prefswin, MA_OWBApp_DefaultURL), sizeof(data->homepage));
	stccpy(data->startpage, (char *) getv(data->prefswin, MA_OWBApp_StartPage), sizeof(data->startpage));
	stccpy(data->newtabpage, (char *) getv(data->prefswin, MA_OWBApp_NewTabPage), sizeof(data->newtabpage));

	data->downloadautoclose = getv(data->prefswin, MA_OWBApp_DownloadAutoClose);
	data->savedownloads = getv(data->prefswin, MA_OWBApp_DownloadSave);
	data->downloadstartautomatically = getv(data->prefswin, MA_OWBApp_DownloadStartAutomatically);
	data->enablepointers = getv(data->prefswin, MA_OWBApp_EnablePointers);
	data->showbuttonframe = getv(data->prefswin, MA_OWBApp_ShowButtonFrame);
	data->showsearchbar = getv(data->prefswin, MA_OWBApp_ShowSearchBar);
	data->showvalidationbuttons = getv(data->prefswin, MA_OWBApp_ShowValidationButtons);
	data->showseparators = getv(data->prefswin, MA_OWBApp_ShowSeparators);
	data->enabletabtransferanim = getv(data->prefswin, MA_OWBApp_EnableTabTransferAnim);
	data->newpagepolicy = getv(data->prefswin, MA_OWBApp_NewPagePolicy);
	data->newpageposition = getv(data->prefswin, MA_OWBApp_NewPagePosition);	
	data->popuppolicy = getv(data->prefswin, MA_OWBApp_PopupPolicy);
	data->toolbuttontype = getv(data->prefswin, MA_OWBApp_ToolButtonType);
	data->urlcompletiontype = getv(data->prefswin, MA_OWBApp_URLCompletionType);
	data->quicklinklook = getv(data->prefswin, MA_OWBApp_QuickLinkLook);
	data->quicklinklayout = getv(data->prefswin, MA_OWBApp_QuickLinkLayout);
	data->quicklinkrows = getv(data->prefswin, MA_OWBApp_QuickLinkRows);
	data->quicklinkposition = getv(data->prefswin, MA_OWBApp_QuickLinkPosition);
	data->errormode = getv(data->prefswin, MA_OWBApp_ErrorMode);
	data->middlebuttonpolicy = getv(data->prefswin, MA_OWBApp_MiddleButtonPolicy);
	data->showfavicons = getv(data->prefswin, MA_OWBApp_ShowFavIcons);

	data->inspector = getv(data->prefswin, MA_OWBApp_EnableInspector);

	/* Needed in ResourceHandleManager::sharedInstance() */
	stccpy(data->certificate_path, (char *) getv(data->prefswin, MA_OWBApp_CertificatePath), sizeof(data->certificate_path));

	int activeconnections = (int) getv(data->prefswin, MA_OWBApp_ActiveConnections);
	ResourceHandleManager::setMaxConnections(activeconnections);
	stccpy(data->useragent, (char *) getv(data->prefswin, MA_OWBApp_UserAgent), sizeof(data->useragent));
	ResourceHandleManager *sharedResourceHandleManager = ResourceHandleManager::sharedInstance();
	if(getv(data->prefswin, MA_OWBApp_ProxyEnabled))
		sharedResourceHandleManager->setProxyInfo(
											String((char *)getv(data->prefswin, MA_OWBApp_ProxyHost)),
											(unsigned long) getv(data->prefswin, MA_OWBApp_ProxyPort),
											(ResourceHandleManager::ProxyType) getv(data->prefswin, MA_OWBApp_ProxyType),
											String((char *)getv(data->prefswin, MA_OWBApp_ProxyUsername)),
											String((char *)getv(data->prefswin, MA_OWBApp_ProxyPassword)) );
	else
		sharedResourceHandleManager->setProxyInfo();

	data->savesession = getv(data->prefswin, MA_OWBApp_SaveSession);
	data->deletesessionatexit = getv(data->prefswin, MA_OWBApp_DeleteSessionAtExit);
	data->sessionrestoremode = getv(data->prefswin, MA_OWBApp_SessionRestoreMode);

	data->savecookies = getv(data->prefswin, MA_OWBApp_SaveCookies);
	/*
	data->temporarycookiespolicy = getv(data->prefswin, MA_OWBApp_TemporaryCookiesPolicy);
	data->persistantcookiespolicy = getv(data->prefswin, MA_OWBApp_PersistantCookiesPolicy);
	*/
	data->cookiespolicy = getv(data->prefswin, MA_OWBApp_CookiesPolicy);
	data->enablelocalstorage = getv(data->prefswin, MA_OWBApp_EnableLocalStorage);
	data->savehistory = getv(data->prefswin, MA_OWBApp_SaveHistory);
	data->historyitems = getv(data->prefswin, MA_OWBApp_HistoryItemLimit);
	data->historyage = getv(data->prefswin, MA_OWBApp_HistoryAgeInDaysLimit);
	data->savecredentials = getv(data->prefswin, MA_OWBApp_SaveFormCredentials);
	data->formautofill = getv(data->prefswin, MA_OWBApp_EnableFormAutofill);

	data->ignoreSSLErrors = getv(data->prefswin, MA_OWBApp_IgnoreSSLErrors);

	data->use_webm = getv(data->prefswin, MA_OWBApp_EnableVP8);
	data->use_flv = getv(data->prefswin, MA_OWBApp_EnableFLV);
	data->use_ogg = getv(data->prefswin, MA_OWBApp_EnableOgg);
	MIMETypeRegistry::reinitializeSupportedMediaMIMETypes();
	data->use_partial_content = getv(data->prefswin, MA_OWBApp_EnablePartialContent);
	data->loopfilter_mode = getv(data->prefswin, MA_OWBApp_LoopFilterMode);

	/* Update windows prefs */
	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			if(getv(child, MUIA_Window_Open))
			{
				set(child, MA_OWBWindow_ShowSearchGroup, data->showsearchbar);
				set(child, MA_OWBApp_ToolButtonType, data->toolbuttontype);
				set(child, MA_OWBWindow_ShowSeparators, data->showseparators);
			}

			set((Object *) getv(child, MA_OWBWindow_SearchGroup), MA_SearchBarGroup_SearchButton, data->showvalidationbuttons);
			set((Object *) getv(child, MA_OWBWindow_AddressBarGroup), MA_AddressBarGroup_GoButton, data->showvalidationbuttons);
			set((Object *) getv(child, MA_OWBWindow_NetworkLedsGroup), MA_NetworkLedsGroup_Count, activeconnections);
			SetAttrs((Object *) getv(child, MA_OWBWindow_FastLinkGroup),
					   MA_QuickLinkGroup_Mode, data->quicklinklook | data->quicklinklayout,
					   MA_QuickLinkGroup_Row, data->quicklinkrows,
					   TAG_DONE);
		}
	}
	NEXTCHILD

	/* Content */
	sharedPreferences->setJavaScriptEnabled((bool) getv(data->prefswin, MA_OWBApp_EnableJavaScript));
	sharedPreferences->setJavaScriptCanOpenWindowsAutomatically((bool) getv(data->prefswin, MA_OWBApp_AllowJavaScriptNewWindow));
	sharedPreferences->setAllowsAnimatedImages((bool) getv(data->prefswin, MA_OWBApp_EnableAnimation));
//	  sharedPreferences->setAllowsAnimatedImages((bool) getv(data->prefswin, MA_OWBApp_AnimationPolicy) != 0);
//	  sharedPreferences->setAllowAnimatedImageLooping((bool) getv(data->prefswin, MA_OWBApp_EnableAnimationLoop));
	sharedPreferences->setUsesPageCache((bool) getv(data->prefswin, MA_OWBApp_EnablePageCache));
	sharedPreferences->setCacheModel((WebCacheModel) getv(data->prefswin, MA_OWBApp_CacheModel));
	sharedPreferences->setLoadsImagesAutomatically((bool) getv(data->prefswin, MA_OWBApp_LoadImagesAutomatically));
	sharedPreferences->setShouldPrintBackgrounds((bool) getv(data->prefswin, MA_OWBApp_ShouldPrintBackgrounds));
	sharedPreferences->setPlugInsEnabled((bool) getv(data->prefswin, MA_OWBApp_EnablePlugins));
	sharedPreferences->setIconDatabaseEnabled(data->showfavicons);
	iconDatabase().setEnabled(data->showfavicons);
	
	WebCore::ad_block_enabled = (bool) getv(data->prefswin, MA_OWBApp_EnableContentBlocking); // Hack

	/* Fonts */
	sharedPreferences->setDefaultFontSize((int) getv(data->prefswin, MA_OWBApp_DefaultFontSize));
	sharedPreferences->setDefaultFixedFontSize((int) getv(data->prefswin, MA_OWBApp_DefaultFixedFontSize));
	sharedPreferences->setMinimumFontSize((int) getv(data->prefswin, MA_OWBApp_MinimumFontSize));
	sharedPreferences->setMinimumLogicalFontSize((int) getv(data->prefswin, MA_OWBApp_MinimumLogicalFontSize));
	sharedPreferences->setDefaultTextEncodingName((char *) getv(data->prefswin, MA_OWBApp_TextEncoding));
	sharedPreferences->setFontSmoothing((FontSmoothingType) getv(data->prefswin, MA_OWBApp_SmoothingType));

	sharedPreferences->setStandardFontFamily((char *) getv(data->prefswin, MA_OWBApp_StandardFontFamily));
	sharedPreferences->setFixedFontFamily((char *) getv(data->prefswin, MA_OWBApp_FixedFontFamily));
	sharedPreferences->setSerifFontFamily((char *) getv(data->prefswin, MA_OWBApp_SerifFontFamily));
	sharedPreferences->setSansSerifFontFamily((char *) getv(data->prefswin, MA_OWBApp_SansSerifFontFamily));
	sharedPreferences->setCursiveFontFamily((char *) getv(data->prefswin, MA_OWBApp_CursiveFontFamily));
	sharedPreferences->setFantasyFontFamily((char *) getv(data->prefswin, MA_OWBApp_FantasyFontFamily));

	/* history */
	sharedPreferences->setHistoryItemLimit(data->historyitems);
	sharedPreferences->setHistoryAgeInDaysLimit(data->historyage);

	/* security */
	sharedPreferences->setLocalStorageEnabled((bool) getv(data->prefswin, MA_OWBApp_EnableLocalStorage));

	sharedPreferences->postPreferencesChangesNotification();
}

DEFSMETHOD(OWBApp_PrefsLoad)
{
	GETDATA;

	DoMethod(obj, MUIM_Application_Load, APPLICATION_ENVARC_PREFS);

	prefs_update(obj, data);

	DoMethod(data->prefswin, MM_PrefsWindow_Fill);

	// Persistant config file (saved at each exit)
	ParseConfigFile("PROGDIR:Conf/misc.prefs", &data->appsettings);

	return 0;
}

DEFSMETHOD(OWBApp_PrefsSave)
{
	GETDATA;

	if (msg->SaveENVARC)
	{
		DoMethod(obj, MUIM_Application_Save, APPLICATION_ENVARC_PREFS);
	}
	DoMethod(obj, MUIM_Application_Save, APPLICATION_ENV_PREFS);

	prefs_update(obj, data);

	return set(data->prefswin, MUIA_Window_Open, FALSE);
}

DEFTMETHOD(OWBApp_PrefsCancel)
{
	GETDATA;

	DoMethod(obj, MUIM_Application_Load, APPLICATION_ENV_PREFS);

	prefs_update(obj, data);

	return set(data->prefswin, MUIA_Window_Open, FALSE);
}

/* Network activity forwarders */

DEFSMETHOD(Network_AddJob)
{
	GETDATA;

	FORCHILD(app, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			DoMethod((Object *)getv(child, MA_OWBWindow_NetworkLedsGroup), MM_Network_AddJob, msg->job);
		}
	}
	NEXTCHILD

	DoMethod(data->networkwin, MM_Network_AddJob, msg->job);

	return 0;
}

DEFSMETHOD(Network_RemoveJob)
{
	GETDATA;

	FORCHILD(app, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			DoMethod((Object *) getv(child, MA_OWBWindow_NetworkLedsGroup), MM_Network_RemoveJob, msg->job);
		}
	}
	NEXTCHILD

	DoMethod(data->networkwin, MM_Network_RemoveJob, msg->job);

	return 0;
}

DEFSMETHOD(Network_UpdateJob)
{
	GETDATA;

	FORCHILD(app, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			DoMethod((Object *)getv(child, MA_OWBWindow_NetworkLedsGroup), MM_Network_UpdateJob, msg->job);
		}
	}
	NEXTCHILD

	DoMethod(data->networkwin, MM_Network_UpdateJob, msg->job);

	return 0;
}

/* Authentication */

DEFSMETHOD(OWBApp_Login)
{
	GETDATA;
	ULONG ret = FALSE;
	String suggestedUsername, suggestedPassword;
	String host = (char *) msg->host;
	STRPTR realm = utf8_to_local((char *) msg->realm); 

	ExtCredential *credential = (ExtCredential *) DoMethod(data->passwordmanagerwin, MM_PasswordManagerGroup_Get, &host);

	if(credential  && credential->type() == CREDENTIAL_TYPE_AUTH)
	{
		suggestedUsername = credential->user();
		suggestedPassword = credential->password();
	}

	Object *loginwin = (Object *) NewObject(getloginwindowclass(), NULL,
			  MA_LoginWindow_Host, msg->host,
			  MA_LoginWindow_Realm, realm,
			  MA_LoginWindow_Username, suggestedUsername.utf8().data(),
			  MA_LoginWindow_Password, suggestedPassword.utf8().data(),
			  TAG_DONE);

	if(loginwin)
	{
		DoMethod(app, OM_ADDMEMBER, loginwin);

		set(loginwin, MUIA_Window_Open, TRUE);

		if(DoMethod(loginwin, MM_LoginWindow_Open))
		{
			*msg->username = (TEXT *) strdup((char *) getv(loginwin, MA_LoginWindow_Username));
			*msg->password = (TEXT *) strdup((char *) getv(loginwin, MA_LoginWindow_Password));
			*msg->persistence = getv(loginwin, MA_LoginWindow_SaveAuthentication) ? CredentialPersistencePermanent : CredentialPersistenceForSession;
			ret = TRUE;
		}

		DoMethod(app, OM_REMMEMBER, loginwin);
		set(loginwin, MUIA_Window_Open, FALSE);
		MUI_DisposeObject(loginwin);

		free(realm);
	}

	return ret;
}

DEFSMETHOD(OWBApp_SetCredential)
{
	GETDATA;
	Credential *credential = (Credential *) msg->credential;
	ExtCredential extCredential(credential->user(), credential->password(), credential->persistence(), *((String *) msg->realm), 0);
	DoMethod(data->passwordmanagerwin, MM_PasswordManagerGroup_Insert, msg->host, &extCredential);

	return 0;
}

DEFSMETHOD(OWBApp_SetFormState)
{
	GETDATA;
	String *host = (String *) msg->host;
	Document *doc = (Document *) msg->doc;
	String strippedURL = stripURL(URL(ParsedURLString, *host)).string();

	D(kprintf("SetFormState for URL <%s>\nStripped <%s>\n", host->latin1().data(), strippedURL.latin1().data()));

	ExtCredential *credential = (ExtCredential *) DoMethod(data->passwordmanagerwin, MM_PasswordManagerGroup_Get, &strippedURL);

	D(kprintf("Found Form credential? %p\n", credential));

	if (credential && credential->type() == CREDENTIAL_TYPE_FORM && credential->flags() != CREDENTIAL_FLAG_BLACKLISTED)
	{
		D(kprintf("user <%s> password <%s>\n", credential->user().utf8().data(), credential->password().utf8().data()));

		// Iterate through all forms
		RefPtr<HTMLCollection> forms = doc->forms();

		for (size_t i = 0; i < forms->length(); ++i)
		{
			WebCore::Node* node = forms->item(i);

		    if (node && node->isHTMLElement())
			{
				WebPasswordFormData webFormPasswordData(static_cast<HTMLFormElement*>(node));

				if (webFormPasswordData.isValid())
				{
					ExtCredential extCredential(webFormPasswordData.userNameValue,
												webFormPasswordData.passwordValue,
												CredentialPersistencePermanent,
												webFormPasswordData.userNameElement,
												webFormPasswordData.passwordElement,
												0);
					if(webFormPasswordData.userNameElementElement)
					{
						webFormPasswordData.userNameElementElement->setValue(credential->user());
						webFormPasswordData.userNameElementElement->setAutoFilled();
					}

					if(webFormPasswordData.passwordElementElement)
					{
						webFormPasswordData.passwordElementElement->setValue(credential->password());
						webFormPasswordData.passwordElementElement->setAutoFilled();
					}

					if(webFormPasswordData.oldPasswordElementElement)
					{
						webFormPasswordData.oldPasswordElementElement->setValue(credential->password());
						webFormPasswordData.oldPasswordElementElement->setAutoFilled();
					}
				}
			}
		}
	}

	return 0;
}

DEFSMETHOD(OWBApp_SaveFormState)
{
	GETDATA;

	WebView *webView = (WebView *) msg->webView;

	if(!msg->webView || !msg->request)
		return 0;

	Frame* coreFrame = &webView->page()->focusController().focusedOrMainFrame();

	if (coreFrame && coreFrame->document())
	{
		// Iterate through all forms
		RefPtr<HTMLCollection> forms = coreFrame->document()->forms();

		for (size_t i = 0; i < forms->length(); ++i)
		{
			WebCore::Node* node = forms->item(i);

		    if (node && node->isHTMLElement())
			{
				WebPasswordFormData webFormPasswordData(static_cast<HTMLFormElement*>(node));

				if (webFormPasswordData.isValid())
				{
					D(kprintf("Valid formdata found for <%s>\n", webFormPasswordData.origin.string().latin1().data()));

					if(getv(app, MA_OWBApp_SaveFormCredentials) && DoMethod(data->passwordmanagerwin, MM_PasswordManagerGroup_Get, &(webFormPasswordData.origin.string())) == NULL)
					{
						ExtCredential extCredential(webFormPasswordData.userNameValue,
													webFormPasswordData.passwordValue,
													CredentialPersistencePermanent,
													webFormPasswordData.userNameElement,
													webFormPasswordData.passwordElement,
													0);

						D(kprintf("Credentials ElementNames [%s : %s] ElementValues [%s : %s]\n", webFormPasswordData.userNameElement.latin1().data(), webFormPasswordData.userNameValue.latin1().data(), webFormPasswordData.passwordElement.latin1().data(), webFormPasswordData.passwordValue.latin1().data()));

						if (!extCredential.isEmpty())
						{
							String message = String::format(GSI(MSG_OWBAPP_SAVE_CREDENTIALS), extCredential.user().utf8().data());
							char *converted_message = utf8_to_local(message.utf8().data());

							if (converted_message)
							{
								ULONG ret = MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_REQUESTER_YES_NO_NEVER), converted_message, NULL);

								if (ret == 0) // Never
								{
									extCredential.clear();
									extCredential.setFlags(CREDENTIAL_FLAG_BLACKLISTED);
									DoMethod(data->passwordmanagerwin, MM_PasswordManagerGroup_Insert, &(webFormPasswordData.origin.string()), &extCredential);
								}
								else if (ret == 1) // Yes
								{
									DoMethod(data->passwordmanagerwin, MM_PasswordManagerGroup_Insert, &(webFormPasswordData.origin.string()), &extCredential);
								}

								free(converted_message);
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

/* MailTo */

DEFSMETHOD(OWBApp_MailTo)
{
	if(OpenURLBase)
	{
		struct TagItem tagitem;
		tagitem.ti_Data = 0;
		tagitem.ti_Tag  = TAG_END;

		URL_OpenA((char *) msg->url, &tagitem);
	}

	return 0;
}

/* History Management */

DEFSMETHOD(History_Insert)
{
	WebHistoryItem *item = (WebHistoryItem *) msg->item;

	FORCHILD(app, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			Object *historypanelgroup = (Object *) getv(child, MA_OWBWindow_HistoryPanelGroup);

			if(historypanelgroup)
			{
				DoMethod(historypanelgroup, MM_History_Insert, item);
			}

			DoMethod((Object *) child, MM_OWBWindow_AddHistoryItem, item);

			DoMethod((Object *) getv(child, MA_OWBWindow_AddressBarGroup), MM_History_Insert, item);

		}
	}
	NEXTCHILD

	return 0;
}

DEFSMETHOD(History_Remove)
{
	WebHistoryItem *item = (WebHistoryItem *) msg->item;

	FORCHILD(app, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			Object *historypanelgroup = (Object *) getv(child, MA_OWBWindow_HistoryPanelGroup);

			if(historypanelgroup)
			{
				DoMethod(historypanelgroup, MM_History_Remove, item);
			}

			DoMethod((Object *) getv(child, MA_OWBWindow_AddressBarGroup), MM_History_Remove, item);
		}
	}
	NEXTCHILD

	return 0;
}

DEFSMETHOD(History_ContainsURL)
{
	ULONG found = FALSE;
    WebHistory* history = WebHistory::sharedHistory();
	std::vector<WebHistoryItem *> historyList = *(history->historyList());

	for(unsigned int i = 0; i < historyList.size(); i++)
	{
		WebHistoryItem *webHistoryItem = historyList[i];

		if(webHistoryItem)
		{
			char *wurl = (char *) webHistoryItem->URLString();
			if(!strcmp(wurl, (char *) msg->url))
			{
				found = TRUE;
				break;
			}
			free(wurl);
		}
	}

	return found;
}

/* Misc */

DEFSMETHOD(OWBApp_AddConsoleMessage)
{
	GETDATA;
	DoMethod(data->consolewin, MM_ConsoleWindow_Add, msg->message);
	return 0;
}

DEFTMETHOD(OWBApp_UpdateBookmarkPanels)
{
	/* Update windows */
	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if(getv(child, MA_OWB_WindowType) == MV_OWB_Window_Browser)
		{
			if(getv(child, MA_OWBWindow_ShowBookmarkPanel))
			{
				DoMethod((Object *) child, MM_OWBWindow_UpdateBookmarkPanel);
			}
		}
	}
	NEXTCHILD

	return 0;
}

/* User Contextmenu Handling */

DEFSMETHOD(OWBApp_BuildUserMenu)
{
	ContextMenu *menu = (ContextMenu *) msg->menu;
	ContextMenuController *controller = (ContextMenuController *) msg->controller;

	if(menu)
	{
		APTR n;
		ULONG count = 0;

		ITERATELIST(n, &contextmenu_list)
		{
			struct contextmenunode *cn = (struct contextmenunode *) n;
            if(cn->category == msg->category)
			{
				count++;
			}
		}

		if(count)
		{
			ContextMenuItem separator(SeparatorType, ContextMenuItemTagNoAction, String());
			controller->appendItem(separator, menu);

			ITERATELIST(n, &contextmenu_list)
			{
	            struct contextmenunode *cn = (struct contextmenunode *) n;

				if(cn->category == msg->category)
				{
					ContextMenuItem item(ActionType, (ContextMenuAction) cn->actionId, String(cn->label));
					controller->appendItem(item, menu);
				}
			}
		}
	}

	return 0;
}

DEFSMETHOD(OWBApp_SelectUserMenu)
{
	ContextMenuController *menucontroller = (ContextMenuController *) msg->menucontroller;
	ContextMenuItem *item = (ContextMenuItem *) msg->item;

	if(menucontroller)
	{
		HitTestResult result = menucontroller->hitTestResult();
		Frame* frame = result.innerNonSharedNode()->document().frame();
	    if (!frame)
			return 0;

		if(item)
		{
			APTR n;
			ITERATELIST(n, &contextmenu_list)
			{
                struct contextmenunode *cn = (struct contextmenunode *) n;

				if(cn->actionId == (int) item->action())
				{
					String command = cn->commandString;
					command.replace("%u", frame->document()->url().string());
					command.replace("%l", result.absoluteLinkURL().string());
					command.replace("%i", result.absoluteImageURL().string());
					command.replace("%p", (char *)getv(app, MUIA_Application_Base));

					OWBCommand cmd(command, ACTION_AMIGADOS);
					cmd.execute();

					break;
				}
			}
		}
	}

	return 0;
}

DEFSMETHOD(OWBApp_CanShowMediaMimeType)
{
	APTR n;
	ULONG ret = FALSE;

	ITERATELIST(n, &mimetype_list)
	{
		struct mimetypenode *mn = (struct mimetypenode *) n;

		/* Exact match first, family match then */
		if((strstr(msg->mimetype, "video/") || strstr(msg->mimetype, "audio/")) && !stricmp(mn->mimetype, msg->mimetype))
		{
			ret = TRUE;
			break;
		}
	}

	return ret;
}

DEFSMETHOD(OWBApp_RequestPolicyForMimeType)
{
	WebMutableURLRequest *webrequest = (WebMutableURLRequest *) msg->request;
	ResourceRequest request = webrequest->resourceRequest();
	ResourceResponse *response = (ResourceResponse *) msg->response;
	WebView* webView = (WebView*) msg->webview;
	WebFramePolicyListener* listener = (WebFramePolicyListener*) msg->listener;

	URL url = response->url();
	String urlstring = url.string();
	String mimetype = response->mimeType();
	String generatedpath = String::format("T:OWB_TempFile_%ld", (unsigned long)time(NULL));
	String path;

	APTR n;
	struct mimetypenode *matching_mn = NULL;
	

	if(!response->suggestedFilename().isEmpty())
	{
	    path = response->suggestedFilename();
	}
	else
	{
	    path = GetFilePart(response->url());
	    path =  decodeURLEscapeSequences(path);
	}

	//kprintf("OWBApp_RequestPolicyForMimeType mimetype <%s> url <%s> attachment %d path <%s>\n", mimetype.utf8().data(), urlstring.utf8().data(), response->isAttachment(), path.latin1().data());
	//kprintf("IsSupportedMediaMIMEType(%s): %d\n", mimetype.utf8().data(), MIMETypeRegistry::isSupportedMediaMIMEType(mimetype));
	//kprintf("WebView::canShowMIMEType(%s): %d\n", mimetype.utf8().data(), webView->canShowMIMEType(mimetype.utf8().data()));

/* Honour attachment attribute */
	if(response->isAttachment())
	{
		listener->download();
		return FALSE;
	}

/* Check configured types first */

	/* Check if there's an exact mimetype match first */
	ITERATELIST(n, &mimetype_list)
	{
		struct mimetypenode *mn = (struct mimetypenode *) n;

		/* Exact match first, family match then */
		if(equalIgnoringCase(mimetype, mn->mimetype))
		{
			matching_mn = mn;
			break;
		}
	}

	//kprintf("1 matching_mn %p <%s>\n", matching_mn, matching_mn?matching_mn->mimetype:"");

	/* Search again with family match */
	if(!matching_mn)
	{
		Vector<String> typeparts;

		mimetype.split("/", true, typeparts);

		if(typeparts.size() == 2)
		{
			String family = typeparts[0];
			family.append("/*");

			ITERATELIST(n, &mimetype_list)
			{
				struct mimetypenode *mn = (struct mimetypenode *) n;

				/* Exact match or family match */
				if(equalIgnoringCase(family, mn->mimetype))
				{
					matching_mn = mn;
					break;
				}
			}
		}
	}

	//kprintf("2 matching_mn %p <%s>\n", matching_mn, matching_mn?matching_mn->mimetype:"");

	/* Extension check in text/plain case, to handle bad text/html mimetype responses */
	if((matching_mn && (equalIgnoringCase(String(matching_mn->mimetype), "text/html") ||
						equalIgnoringCase(String(matching_mn->mimetype), "text/plain") ||
						equalIgnoringCase(String(matching_mn->mimetype), "application/octet-stream")
					   )
	   ) ||
	   url.protocolIs("ftp") ||
	   matching_mn == NULL)
	{
	    int pos = path.reverseFind('.');

		//kprintf("looking for extension now...\n");

		if (pos >= 0)
		{
			bool found = false;
	        String extension = path.substring(pos + 1);

			//kprintf("extension %s\n", extension.utf8().data());

			ITERATELIST(n, &mimetype_list)
			{
				struct mimetypenode *mn = (struct mimetypenode *) n;
				String extensions = mn->extensions;
				Vector<String> listExtensions;
				extensions.split(" ", true, listExtensions);

				for(unsigned int i = 0; i < listExtensions.size(); i++)
				{
					//kprintf("checking extension %s (mimetype %s)\n", listExtensions[i].utf8().data(), mn->mimetype);

					if(equalIgnoringCase(listExtensions[i], extension))
					{
						//kprintf("found !\n");
						matching_mn = mn;
						found = true;
						break;
					}
				}

				if(found)
					break;
			}
		}	 
	}

	//kprintf("3 matching_mn %p <%s>\n", matching_mn, matching_mn?matching_mn->mimetype:"");

	if(matching_mn)
	{
		mimetype_action_t action = MIMETYPE_ACTION_IGNORE;
		struct mimetypenode *mn = matching_mn;

		if(mn->action == MIMETYPE_ACTION_ASK)
		{
			String truncatedpath = truncate(path, 64);
			String message = String::format(GSI(MSG_OWBAPP_MIMETYPE_REQUEST_ACTION), truncatedpath.latin1().data(), matching_mn->mimetype);
			char *cmessage = strdup(message.latin1().data());

			if(cmessage)
			{
				ULONG ret = MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_OWBAPP_MIMETYPE_ACTIONS), cmessage, NULL);
				
				switch(ret)
				{
					case 1:
						action = MIMETYPE_ACTION_INTERNAL;
						break;

					case 2:
						action = MIMETYPE_ACTION_EXTERNAL;
						break;

					case 3:
						action = MIMETYPE_ACTION_DOWNLOAD;
						break;

					default:
					case 0:
						action = MIMETYPE_ACTION_IGNORE;
						break;
				}

				free(cmessage);
			}
		}
		else
		{
			action = mn->action;
		}

		switch(action)
		{
			case MIMETYPE_ACTION_INTERNAL:
			{
				if(webView->canShowMIMEType(mimetype.utf8().data()))
				{
					listener->use();
				}
				else
				{
					listener->download();
				}
				break;
			}

			case MIMETYPE_ACTION_EXTERNAL:
			{
				bool processafterdownload;
				String command = mn->viewer;
				
				command.append(" ");
				command.append(mn->parameters);

				processafterdownload = command.find("%f") != notFound;

				command.replace("%l", urlstring);
				command.replace("%f", generatedpath);
				command.replace("%p", (char *)getv(app, MUIA_Application_Base));

				if(processafterdownload) /* %f in command means file must be downloaded first, and command processed after completion */
				{
					TransferSharedPtr<WebDownloadDelegate> downloadDelegate = webView->downloadDelegate();

					listener->ignore();

					if(downloadDelegate)
					{
						WebDownload* download = WebDownload::createInstance(url, downloadDelegate);
						if(download)
						{
							char *cgeneratedpath = strdup(generatedpath.latin1().data());
							char *ccommand = strdup(command.latin1().data());

							download->setDestination(cgeneratedpath, true, false);
							download->setCommandUponCompletion(ccommand);
							download->start(true);

							free(cgeneratedpath);
							free(ccommand);
						}
					}
				}
				else
				{
					OWBCommand cmd(command, ACTION_AMIGADOS);
					
					listener->ignore();
					cmd.execute();
				}
				break;
			}

			case MIMETYPE_ACTION_DOWNLOAD:
			{
				listener->download();
				break;
			}

			case MIMETYPE_ACTION_ASK:
			{
				break;
			}

			case MIMETYPE_ACTION_STREAM:
			{
				listener->ignore();
				break;
			}

			case MIMETYPE_ACTION_PIPE:
			{
				listener->ignore();
				break;
			}

			case MIMETYPE_ACTION_IGNORE:
			{
				listener->ignore();
				break;
			}

			case MIMETYPE_ACTION_PLUGIN:
			{
			    listener->ignore();
			}
		}
		
		return TRUE;
	}

	//kprintf("Nothing found in mimetypes, fallback to default behaviour (canShow: %d)\n", webView->canShowMIMEType(type));

/* Fall back to default behaviour */

	/* hardcoded check for FTP scheme to handle most common binary formats */
	if(webView->canShowMIMEType(mimetype.utf8().data()) && url.protocolIs("ftp") && name_match(path.latin1().data(), "#?.(zip|gz|bz2|tar|exe|elf|rar|avi|mpg|lha|lzx|pdf)"))
	{
        String truncatedpath = truncate(path, 64);
		String message = String::format(GSI(MSG_OWBAPP_MIMETYPE_REQUEST_ACTION), truncatedpath.latin1().data(), mimetype.utf8().data());
		char *cmessage = strdup(message.latin1().data());

		if(cmessage)
		{
			ULONG ret = MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_OWBAPP_MIMETYPE_ACTIONS_FALLBACK), cmessage, NULL);
			
			switch(ret)
			{
				default:
				case 1:
					listener->use();
					break;

				case 2:
	                listener->download();
					break;

				case 0:
					listener->ignore();
					break;
			}

			free(cmessage);
		}
	}
	else
	{
		if(webView->canShowMIMEType(mimetype.utf8().data()))
		{
			listener->use();
		}
		else
		{
            listener->download();
		}
	}

	return TRUE;
}

DEFSMETHOD(OWBApp_RestoreSession)
{
	ULONG rc = FALSE;

	String sessionfile = SESSION_PATH;

	if(msg->selectfile)
	{
	    struct TagItem tags[] =
	    {
            { ASLFR_TitleText, (STACKIPTR) GSI(MSG_OWBAPP_OPEN_SESSION) },
            { ASLFR_InitialPattern, (STACKIPTR) "#?" },
            { ASLFR_InitialDrawer, (STACKIPTR) SESSION_DIRECTORY },
            {TAG_DONE, TAG_DONE}
	    };

		char *file = asl_run(SESSION_DIRECTORY, (struct TagItem *) &tags, FALSE);

		if(file)
		{
			sessionfile = file;
			FreeVecTaskPooled(file);
		}	 
		else
		{
			return 0;
		}
	}

	OWBFile f(sessionfile);

	if(f.open('r') != -1)
	{
		if(msg->ask)
		{
			if(MUI_RequestA(app, NULL, 0, GSI(MSG_REQUESTER_NORMAL_TITLE), GSI(MSG_OWBAPP_RESTORE_SESSION_CHOICES), GSI(MSG_OWBAPP_RESTORE_SESSION), NULL) == 0)
			{
				f.close();
				DeleteFile(SESSION_PATH);
				return FALSE;
			}
		}

		char *fileContent = f.read(f.getSize());

		if(fileContent)
		{
			ULONG i;
			String input = fileContent;
			Vector<String> entries;
			static Vector<WindowState> windowstates;
			windowstates.clear();

			input.split("\n", true, entries);

			for(i = 0; i < entries.size(); i++)
			{
				Vector<String> attributes;
				entries[i].split(" ", true, attributes);

				if(attributes.size() == 4)
				{
					bool found = false;
					ULONG j;
					int id = attributes[0].toInt();
					BrowserState browserstate = { attributes[1], IntPoint(attributes[2].toInt(), attributes[3].toInt()) };

					for(j = 0; j < windowstates.size(); j++)
					{
						if(windowstates[j].id == id)
						{
							found = true;
							break;
						}
					}

					if(found)
					{
						windowstates[j].browserstates.append(browserstate);
					}
					else
					{
						WindowState windowstate;
						windowstate.id = id;
						windowstate.browserstates.append(browserstate);

						windowstates.append(windowstate);
					}
				}
			}

			delete [] fileContent;

			for(i = 0; i < windowstates.size(); i++)
			{
				ULONG j;
				Object *window = NULL;

				for(j = 0; j < windowstates[i].browserstates.size(); j++)
				{
					BalWidget *widget = NULL;

					if(j == 0)
					{
						widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddWindow, windowstates[i].browserstates[j].url.utf8().data(), FALSE, NULL, FALSE, NULL, FALSE);
						if(widget)
							window = widget->window;
					}
					else
					{
						widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddBrowser, window, windowstates[i].browserstates[j].url.utf8().data(), FALSE, NULL, TRUE, FALSE, TRUE);
					}

					if(widget)
					{
						BalPoint p = { windowstates[i].browserstates[j].offset.x(), windowstates[i].browserstates[j].offset.y() };
						widget->webView->setScheduledScrollOffset(p);
					}
				}

				rc = TRUE;
			}
		}

		f.close();
	}

	return rc;
}

DEFSMETHOD(OWBApp_SaveSession)
{
	//kprintf("OWBApp_SaveSession\n");

	String sessionfile = SESSION_PATH;

	if(!msg->selectfile)
	{
		if(!getv(app, MA_OWBApp_SaveSession))
		{
			return 0;
		}
	}
	else
	{
		if(msg->selectfile)
		{
			struct TagItem tags[] =
			{
                { ASLFR_TitleText, (STACKIPTR) GSI(MSG_OWBAPP_SAVE_SESSION) },
                { ASLFR_InitialPattern, (STACKIPTR) "#?" },
                { ASLFR_InitialDrawer, (STACKIPTR) SESSION_DIRECTORY },
                { ASLFR_DoSaveMode, (STACKIPTR) TRUE },
                { TAG_DONE, TAG_DONE }
			};

			char *file = asl_run(SESSION_DIRECTORY, (struct TagItem *) &tags, FALSE);

			if(file)
			{
				sessionfile = file;
				FreeVecTaskPooled(file);
			}
			else
			{
				return 0;
			}
		}
	}

	OWBFile f(sessionfile);

	if(f.open('w') != -1)
	{
		APTR n, m;
		ULONG i = 0;
		String output = "";

		ITERATELIST(n, &window_list)
		{
			struct windownode *wn = (struct windownode *) n;

			ITERATELIST(m, &view_list)
			{
				BalWidget *vn = (BalWidget *) m;

				if(vn->window == wn->window)
				{
					// Honour the private browsing flag
					if(!getv(vn->browser, MA_OWBBrowser_PrivateBrowsing))
					{
					    // core(vn->webView->mainFrame())->view()->visibleContentRect().y()
					  output.append(String::format("%lu %s %d %d\n", (unsigned long)i, (char *) getv(vn->browser, MA_OWBBrowser_URL), vn->webView->scrollOffset().x, vn->webView->scrollOffset().y));
					}
				}
			}

			i++;
		}

		f.write(output);
		f.close();
	}

	return 0;
}

DEFMMETHOD(Application_ReturnID)
{
	GETDATA;

	if(msg->retid == (unsigned int) MUIV_Application_ReturnID_Quit)
	{
		data->isquitting = TRUE;
	}

	return DOSUPER;
}

DEFSMETHOD(CookieManagerGroup_DidInsert)
{
	GETDATA;
	if(data->cookiemanagerwin && getv(data->cookiemanagerwin, MUIA_Window_Open))
	    return DoMethodA(data->cookiemanagerwin, (_Msg_*) msg);
	else
	    return 0;
}

DEFSMETHOD(CookieManagerGroup_DidRemove)
{
	GETDATA;
	if(data->cookiemanagerwin && getv(data->cookiemanagerwin, MUIA_Window_Open))
  	    return DoMethodA(data->cookiemanagerwin, (_Msg_*) msg);
	else
	    return 0;
}

DEFSMETHOD(BlockManagerGroup_DidInsert)
{
	GETDATA;
	return DoMethodA(data->blockmanagerwin, (_Msg_*) msg);
}

DEFTMETHOD(OWBApp_EraseFavicons)
{
    WebIconDatabase *sharedWebIconDatabase = WebIconDatabase::sharedWebIconDatabase();
	if(sharedWebIconDatabase)
	{
		sharedWebIconDatabase->removeAllIcons();
	}
	return 0;
}

DEFSMETHOD(URLPrefsGroup_ApplySettingsForURL)
{
	GETDATA;
	DoMethod(data->urlprefswin, MM_URLPrefsGroup_ApplySettingsForURL, msg->url, msg->webView);
	return 0;
}

DEFSMETHOD(URLPrefsGroup_UserAgentForURL)
{
	GETDATA;
	DoMethod(data->urlprefswin, MM_URLPrefsGroup_UserAgentForURL, msg->url, msg->webView);
	return 0;
}

DEFSMETHOD(URLPrefsGroup_MatchesURL)
{
	GETDATA;
	return DoMethod(data->urlprefswin, MM_URLPrefsGroup_MatchesURL, msg->url);
}

DEFSMETHOD(URLPrefsGroup_CookiePolicyForURLAndName)
{
	GETDATA;
	return DoMethod(data->urlprefswin, MM_URLPrefsGroup_CookiePolicyForURLAndName, msg->url, msg->name);
}

DEFMMETHOD(DragDrop)
{
	if(getv(msg->obj, MA_OWB_ObjectType) == MV_OWB_ObjectType_Tab)
	{
		Object *sourcewin = _win(msg->obj);
		ULONG detachit = !(msg->x > (LONG) getv(sourcewin, MUIA_Window_LeftEdge) && msg->x < (LONG) getv(sourcewin, MUIA_Window_LeftEdge) + (LONG) getv(sourcewin, MUIA_Window_Width)
					   && msg->y > (LONG) getv(sourcewin, MUIA_Window_TopEdge) && msg->y < (LONG) getv(sourcewin, MUIA_Window_TopEdge) + (LONG) getv(sourcewin, MUIA_Window_Height));

		if(detachit)
		{
			DoMethod(_win(msg->obj), MM_OWBWindow_DetachBrowser, (Object *) getv(msg->obj, MA_OWB_Browser), NULL);
		}
	}

	return 0;
}

DEFTMETHOD(OWBApp_ResetVM)
{
    Object *window = (Object *) getv(obj, MA_OWBApp_ActiveWindow);
    APTR n, m;

    kprintf("OWBApp_ResetVM\n");
    kprintf("Closing tabs\n");

    ITERATELISTSAFE(n, m, &view_list)
    {
	struct viewnode *vn = (struct viewnode *) n;
	DoMethod(app, MM_OWBApp_RemoveBrowser, vn->browser);
    }

    kprintf("Resetting VM\n");
    //JSDOMWindowBase::commonVM()->cleanup();
    //WebCore::resetVM();

    kprintf("Adding empty tab\n");
    DoMethod(window, MM_OWBWindow_MenuAction, MNA_NEW_PAGE);      

    kprintf("Alive?\n");

    return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECSMETHOD(OWBApp_DisposeObject)
DECSMETHOD(OWBApp_DisposeWindow)
DECSMETHOD(OWBApp_OpenWindow)
DECTMETHOD(OWBApp_About)
DECTMETHOD(OWBApp_Quit)
DECSMETHOD(OWBApp_AddPage)
DECSMETHOD(OWBApp_AddWindow)
DECSMETHOD(OWBApp_RemoveWindow)
DECSMETHOD(OWBApp_AddBrowser)
DECSMETHOD(OWBApp_RemoveBrowser)
DECTMETHOD(OWBApp_Expose)
DECTMETHOD(OWBApp_WebKitEvents)
DECSMETHOD(OWBApp_Download)
DECSMETHOD(OWBApp_DownloadUpdate)
DECSMETHOD(OWBApp_DownloadDone)
DECSMETHOD(OWBApp_DownloadError)
DECSMETHOD(OWBApp_DownloadCancelled)
DECTMETHOD(OWBApp_LoadBookmarks)
DECTMETHOD(OWBApp_BookmarksChanged)
DECSMETHOD(Bookmarkgroup_AddLink)
DECSMETHOD(Bookmarkgroup_RemoveQuickLink)
DECSMETHOD(OWBApp_PrefsLoad)
DECSMETHOD(OWBApp_PrefsSave)
DECTMETHOD(OWBApp_PrefsCancel)
DECSMETHOD(Network_AddJob)
DECSMETHOD(Network_RemoveJob)
DECSMETHOD(Network_UpdateJob)
DECSMETHOD(OWBApp_Login)
DECSMETHOD(OWBApp_SetCredential)
DECSMETHOD(OWBApp_SetFormState)
DECSMETHOD(OWBApp_SaveFormState)
DECSMETHOD(OWBApp_MailTo)
DECSMETHOD(History_Insert)
DECSMETHOD(History_Remove)
DECSMETHOD(History_ContainsURL)
DECSMETHOD(OWBApp_AddConsoleMessage)
DECTMETHOD(OWBApp_UpdateBookmarkPanels)
DECSMETHOD(OWBApp_BuildUserMenu)
DECSMETHOD(OWBApp_SelectUserMenu)
DECSMETHOD(OWBApp_CanShowMediaMimeType)
DECSMETHOD(OWBApp_RequestPolicyForMimeType)
DECSMETHOD(OWBApp_RestoreSession)
DECSMETHOD(OWBApp_SaveSession)
DECMMETHOD(Application_ReturnID)
DECSMETHOD(CookieManagerGroup_DidInsert)
DECSMETHOD(CookieManagerGroup_DidRemove)
DECSMETHOD(BlockManagerGroup_DidInsert)
DECTMETHOD(OWBApp_EraseFavicons)
DECSMETHOD(URLPrefsGroup_ApplySettingsForURL)
DECSMETHOD(URLPrefsGroup_UserAgentForURL)
DECSMETHOD(URLPrefsGroup_MatchesURL)
DECSMETHOD(URLPrefsGroup_CookiePolicyForURLAndName)
DECMMETHOD(DragDrop)
DECTMETHOD(OWBApp_ResetVM)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Application, owbappclass)
