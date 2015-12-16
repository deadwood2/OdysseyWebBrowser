#ifndef __GUI_H__
#define __GUI_H__

#include <exec/types.h>
#include <proto/utility.h>
#include <utility/date.h>
#include <clib/debug_protos.h>

#include "WebKitVersion.h"
#include "classes.h"
#include "methodstack.h"
#include "mui.h"
#include "owb_cat.h"

#define xstringify(s) stringify(s)
#define stringify(s) #s

/* Constants */

#define THREAD_NAME              "OWB Thread"
#define APPLICATION_DESCRIPTION  "WebKit-based browser."
#define APPLICATION_BASE         "OWB"
#define APPLICATION_ICON         "PROGDIR:OWB"
#if !defined(__AROS__)
#define APPLICATION_ENV_PREFS    "ENV:MUI/OWB.config"
#define APPLICATION_ENVARC_PREFS "ENVARC:MUI/OWB.config"
#else
#define APPLICATION_ENV_PREFS    "ENV:Zune/OWB.config"
#define APPLICATION_ENVARC_PREFS "ENVARC:Zune/OWB.config"
#endif

#define DEFAULT_PAGE_STRING "<html><head><title>Origyn Web Browser</title><head><body><table width=\"100%%\" height=\"100%%\"><tr align=\"center\"><td align=\"center\"><h1>Origyn Web Browser</h1><h4><a href=\"http://fabportnawak.free.fr/owb/\">MorphOS version " VERSION "</a></h4></table></body></html>"
#define ERROR_PAGE_STRING   "<html><head><title>Origyn Web Browser Error</title><head><body>Unable to open %s.<br>Error %d: %s</body></html>"

#define DEFAULT_WINDOW_NAME GSI(MSG_OWBAPP_DEFAULT_WINDOW_NAME)
#define DEFAULT_PAGE_NAME   GSI(MSG_OWBAPP_DEFAULT_PAGE_NAME)

#define BOOKMARK_PATH       "PROGDIR:bookmarks.html"
#define CONF_DIRECTORY	    "PROGDIR:Conf/"
#define SESSION_DIRECTORY   "PROGDIR:Conf/sessions/"
#define SESSION_PATH        "PROGDIR:Conf/sessions/session.prefs"
#define SCRIPT_DIRECTORY    "PROGDIR:Scripts/"

#define OWB_URL		    "http://fabportnawak.free.fr/owb/"
#define OWB_PLUGINS_URL     OWB_URL "plugins/"

enum
{
	MNA_NEW_WINDOW = 1000,
	MNA_NEW_PAGE,
	MNA_OPEN_LOCAL_FILE,
	MNA_OPEN_URL,
	MNA_OPEN_SESSION,
	MNA_SAVE_SESSION,
	MNA_SAVE_AS_TEXT,
	MNA_SAVE_AS_SOURCE,
	MNA_SAVE_AS_PDF,
	MNA_SAVE_AS_PS,
	MNA_PRINT,
	MNA_ABOUT,
	MNA_CLOSE_PAGE,
	MNA_CLOSE_WINDOW,
	MNA_COPY,
	MNA_COPY_LOCAL,
	MNA_CUT,
	MNA_PASTE,
	MNA_PASTE_TO_URL,
	MNA_SELECT_ALL,
	MNA_UNDO,
	MNA_REDO,
	MNA_FIND,
	MNA_ZOOM_IN,
	MNA_ZOOM_OUT,
	MNA_ZOOM_RESET,
	MNA_FULLSCREEN,
	MNA_BACK,
	MNA_FORWARD,
	MNA_HOME,
	MNA_STOP,
	MNA_RELOAD,
	MNA_SOURCE,
	MNA_NAVIGATION,
	MNA_QUICKLINKS,
	MNA_LOCATION,
	MNA_STATUS,
	MNA_NO_PANEL,
	MNA_HISTORY_MENU,
	MNA_HISTORY_PANEL,
	MNA_HISTORY_RECENTENTRIES,
	MNA_HISTORY_CLOSEDVIEWS,
	MNA_BOOKMARK_PANEL,
	MNA_BOOKMARKS_MENU,
	MNA_ADD_BOOKMARK,
	MNA_DOWNLOADS_WINDOW,
	MNA_BOOKMARKS_WINDOW,
	MNA_BOOKMARKS_STOP,
	MNA_NETWORK_WINDOW,
	MNA_CONSOLE_WINDOW,
	MNA_PASSWORDMANAGER_WINDOW,
	MNA_COOKIEMANAGER_WINDOW,
	MNA_BLOCKMANAGER_WINDOW,
	MNA_SEARCHMANAGER_WINDOW,
	MNA_SCRIPTMANAGER_WINDOW,
	MNA_URLSETTINGS_WINDOW,
	MNA_OWB_SETTINGS,
	MNA_PRIVATE_BROWSING,
	MNA_MUI_SETTINGS,
	MNA_QUIT,

	/* Per tab settings */
	MNA_SETTINGS_ENABLE_JAVASCRIPT,
	MNA_SETTINGS_JAVASCRIPT_ENABLED,
	MNA_SETTINGS_JAVASCRIPT_DISABLED,
	MNA_SETTINGS_JAVASCRIPT_DEFAULT,

	MNA_SETTINGS_ENABLE_IMAGES,
	MNA_SETTINGS_IMAGES_ENABLED,
	MNA_SETTINGS_IMAGES_DISABLED,
	MNA_SETTINGS_IMAGES_DEFAULT,

	MNA_SETTINGS_ENABLE_ANIMATIONS,
	MNA_SETTINGS_ANIMATIONS_ENABLED,
	MNA_SETTINGS_ANIMATIONS_DISABLED,
	MNA_SETTINGS_ANIMATIONS_DEFAULT,

	MNA_SETTINGS_ENABLE_PLUGINS,
	MNA_SETTINGS_PLUGINS_ENABLED,
	MNA_SETTINGS_PLUGINS_DISABLED,
	MNA_SETTINGS_PLUGINS_DEFAULT,

	MNA_SETTINGS_SPOOF_AS,
	MNA_SETTINGS_SPOOF_AS_DEFAULT,

	MNA_DUMMY,
};

enum
{
	ACTION_AMIGADOS,
	ACTION_REXX,
	ACTION_INTERNAL,
};

typedef enum
{
	MIMETYPE_ACTION_INTERNAL,
	MIMETYPE_ACTION_EXTERNAL,
	MIMETYPE_ACTION_DOWNLOAD,
	MIMETYPE_ACTION_ASK,
	MIMETYPE_ACTION_STREAM,
	MIMETYPE_ACTION_PIPE,
	MIMETYPE_ACTION_PLUGIN,
	MIMETYPE_ACTION_IGNORE
} mimetype_action_t;

/* Exported structures */

/*
struct viewnode
{
  struct MinNode node;
  ULONG num;
  Object   *app;
  Object   *window; 
  Object   *browser; 

  WebView *webView;
  _cairo_surface *surface;
  _cairo *cr;

  bool expose;
};
*/

struct windownode
{
	struct MinNode node;
	ULONG num;
	Object *window;
};

struct downloadnode
{
	struct MinNode node;
	APTR  webdownload;
	char* filename;
	char* path;
	char* url;
	char* originurl;
	char* mimetype;
	QUAD size;
	QUAD done;
	QUAD prevdone;
	LONG state;
	ULONG speed;
	double starttime;
	double lastupdatetime;
	double remainingtime;
	struct ClockData eta;
	char status[64];
	APTR gaugeobj;
	APTR gaugeimg;
	APTR iconobj;
	APTR iconimg;
};

struct contextmenunode
{
	struct MinNode node;
	int actionId;
	int category;
	char *label;
	int commandType;
	char *commandString;
};

struct mimetypenode
{
	struct MinNode node;
	char *mimetype;
	char *extensions;
	mimetype_action_t action;
	char *viewer;
	char *parameters;
	char *description;
	int	builtin;
};

/* browser settings */

enum { PLUGINS_DISABLED, PLUGINS_ENABLED, PLUGINS_DEFAULT };
enum { JAVASCRIPT_DISABLED, JAVASCRIPT_ENABLED, JAVASCRIPT_DEFAULT };
enum { IMAGES_DISABLED, IMAGES_ENABLED, IMAGES_DEFAULT };
enum { ANIMATIONS_DISABLED, ANIMATIONS_ENABLED, ANIMATIONS_DEFAULT };
enum { COOKIES_REJECT, COOKIES_ACCEPT, COOKIES_ASK, COOKIES_DEFAULT };
enum { BLOCK_DISABLED, BLOCK_ENABLED, BLOCK_DEFAULT };
enum { PRIVATE_BROWSING_DISABLED, PRIVATE_BROWSING_ENABLED, PRIVATE_BROWSING_DEFAULT };
enum { USERAGENT_DEFAULT = -1 };

struct browsersettings
{
	ULONG javascript;
	ULONG images;
	ULONG plugins;
	ULONG animations;
	LONG useragent;
	ULONG cookiepolicy;
	char *cookiefilter;
	ULONG privatebrowsing;
	ULONG blocking;
	ULONG localstorage;
};

struct urlsettingnode
{
	struct MinNode node;
	char  *urlpattern;
	struct browsersettings settings;
};

struct familynode
{
	struct MinNode node;
	char family[0];
};

struct console_entry
{
	struct ClockData clockdata;
	char message[2048];
};

struct credential_entry
{
	char *host;
	char *realm;
	char *username;
	char *password;
	int  type;
	int  flags;
	char *username_field;
	char *password_field;
};

#define COOKIEFLAG_DOMAIN 1
#define COOKIEFLAG_COOKIE 2

struct cookie_entry
{
	ULONG flags;
	char *name;
	char *value;
	char *domain;
	char *protocol;
	char *path;
	double expiry;
	ULONG secure;
	ULONG http_only;
	ULONG session;
};

struct block_entry
{
	char *rule;
	int type;
	void *ptr;
};

struct search_entry
{
	char *label;
	char *request;
	char *shortcut;
};

struct history_entry
{
	APTR webhistoryitem;
	APTR faviconobj;
	APTR faviconimg;
	ULONG isbookmark;
	APTR bookmarkobj;
	APTR bookmarkimg;
};

/* menu entries */
enum { MENUTYPE_BOOKMARK, MENUTYPE_CLOSEDVIEW, MENUTYPE_HISTORY, MENUTYPE_SPOOF };

struct menu_entry
{
	int type;
	int index;
	char data[0];
};

/* exported globals */
extern struct MinList window_list;
extern struct MinList view_list;
extern struct MinList download_list;
extern struct MinList contextmenu_list;
extern struct MinList mimetype_list;
extern struct MinList urlsetting_list;
extern struct MinList family_list;

extern struct NewMenu MenuData[];
extern Object *app;

/* exported functions */
ULONG is_morphos2(void);

struct NotifyRequest *dosnotify_start(CONST_STRPTR name);
void dosnotify_stop(struct NotifyRequest *nr);

Object *create_application(char *url);
Object *create_window(char *url, ULONG isframe, APTR sourceview, APTR windowfeatures, ULONG privatebrowsing);
Object *create_browser(char *url, ULONG isframe, Object *title, APTR sourceview, Object *window, ULONG privatebrowsing);
Object *create_toolbutton(CONST_STRPTR text, CONST_STRPTR image, ULONG type);
Object *create_historybutton(CONST_STRPTR text, CONST_STRPTR image, ULONG type, ULONG buttontype);

struct downloadnode* download_create(char *url, char *filename, char *path, char *originurl);
void download_delete(struct downloadnode *dl);

struct contextmenunode*	contextmenu_create(int actionId, int category, char *label, int commandType, char *commandString);
void contextmenu_delete(struct contextmenunode *cn);

struct mimetypenode* mimetype_create(char *mimetype, char *extensions, mimetype_action_t action, char *viewer, char *parameters, int builtin);
void mimetype_delete(struct mimetypenode *mn);

/* quit flags */
#if defined(__cplusplus)
extern "C"
{
#endif
	void setIsQuitting(int);
	int isQuitting(void);
	void setIsSafeToQuit(int);
	int isSafeToQuit(void);
#if defined(__cplusplus)
}
#endif

/* undefine them, since they can clash with some c++ classes/methods */
#undef String
#undef get

/* Benchmark helpers */

#if defined(__cplusplus)
#define BENCHMARK 1

#ifdef BENCHMARK
#define BENCHMARK_DECLARE static const bool localBenchmark = getenv("OWB_BENCHMARK");
#define BENCHMARK_INIT double startBenchmark = 0, diffBenchmark = 0;
#define BENCHMARK_RESET if(localBenchmark) startBenchmark = WTF::currentTime();
#define BENCHMARK_EVALUATE if(localBenchmark) diffBenchmark = WTF::currentTime() - startBenchmark;
#define BENCHMARK_EXPRESSION(x) if(localBenchmark) x
#else
#define BENCHMARK_DECLARE
#define BENCHMARK_INIT 
#define BENCHMARK_RESET
#define BENCHMARK_EVALUATE
#define BENCHMARK_EXPRESSION(x)
#endif
#endif

#endif




