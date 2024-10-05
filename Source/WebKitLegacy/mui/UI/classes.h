#ifndef __CLASSES_H__
#define __CLASSES_H__

#include "include/macros/vapor.h"

#include <proto/alib.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <utility/tagitem.h>

#include <intuition/intuition.h>
#include <proto/intuition.h>

#include <libraries/gadtools.h>
#if defined(__AROS__)
#define MUI_OBSOLETE
#endif
#include <libraries/mui.h>
#include <proto/muimaster.h>

#define DEFCLASS(s) ULONG create_##s##class(void); \
    struct IClass *get##s##class(void); \
    APTR get##s##classroot(void); \
    void delete_##s##class(void)

ULONG classes_init(void);
void  classes_cleanup(void);

/************************************************************/

/* Classes */
DEFCLASS(owbapp);
DEFCLASS(owbbrowser);
DEFCLASS(owbgroup);
DEFCLASS(owbwindow);

DEFCLASS(navigationgroup);
DEFCLASS(addressbargroup);
DEFCLASS(searchbargroup);
DEFCLASS(findtext);
DEFCLASS(downloadwindow);
DEFCLASS(downloadgroup);
DEFCLASS(downloadlist);
DEFCLASS(prefswindow);
DEFCLASS(toolbutton);
DEFCLASS(transferanim);
DEFCLASS(tabtransferanim);
DEFCLASS(popstring);
DEFCLASS(historypopstring);
//DEFCLASS(fontfamilypopstring);
DEFCLASS(historylist);
DEFCLASS(title);
DEFCLASS(titlelabel);
DEFCLASS(menu);
DEFCLASS(menuitem);
DEFCLASS(bookmarkwindow);
DEFCLASS(bookmarkgroup);
DEFCLASS(bookmarklisttree);
DEFCLASS(linklist);
DEFCLASS(quicklinkgroup);
DEFCLASS(quicklinkbuttongroup);
DEFCLASS(quicklinkparentgroup);
DEFCLASS(historybutton);
DEFCLASS(networkwindow);
DEFCLASS(networklist);
DEFCLASS(networkledsgroup);
DEFCLASS(splashwindow);
DEFCLASS(loginwindow);
DEFCLASS(consolewindow);
DEFCLASS(consolelist);
DEFCLASS(bookmarkpanelgroup);
DEFCLASS(contextmenugroup);
DEFCLASS(contextmenulist);
DEFCLASS(mimetypegroup);
DEFCLASS(mimetypelist);
DEFCLASS(choosetitlegroup);
DEFCLASS(toolbutton_newtab);
DEFCLASS(toolbutton_addbookmark);
DEFCLASS(toolbutton_bookmarks);
DEFCLASS(urlstring);
DEFCLASS(favicon);
DEFCLASS(icon);
//DEFCLASS(historywindow);
DEFCLASS(historypanelgroup);
DEFCLASS(historylisttree);
DEFCLASS(passwordmanagerwindow);
DEFCLASS(passwordmanagergroup);
DEFCLASS(passwordmanagerlist);
DEFCLASS(cookiemanagerwindow);
DEFCLASS(cookiemanagergroup);
DEFCLASS(cookiemanagerlisttree);
DEFCLASS(blockmanagerwindow);
DEFCLASS(blockmanagergroup);
DEFCLASS(blockmanagerlist);
DEFCLASS(searchmanagerwindow);
DEFCLASS(searchmanagergroup);
DEFCLASS(searchmanagerlist);
DEFCLASS(scriptmanagerwindow);
DEFCLASS(scriptmanagergroup);
DEFCLASS(scriptmanagerlist);
DEFCLASS(scriptmanagerhostlist);
DEFCLASS(urlprefswindow);
DEFCLASS(urlprefsgroup);
DEFCLASS(urlprefslist);
DEFCLASS(mediacontrolsgroup);
DEFCLASS(seekslider);
DEFCLASS(volumeslider);
DEFCLASS(spacer);
DEFCLASS(suggestlist);
DEFCLASS(suggestpopstring);
DEFCLASS(printerwindow);
DEFCLASS(autofillpopup);
DEFCLASS(autofillpopuplist);
DEFCLASS(colorchooserpopup);
DEFCLASS(datetimechooserpopup);

#define MTAGBASE (TAG_USER|((0xDEADL<<16)+0))

/* Attributes */
enum {
    MA_dummy = (int)(MTAGBASE),

    /* Generic (forwarded to right object) */
    MA_OWB_WindowType,
    MA_OWB_ObjectType,
    MA_OWB_URL,
    MA_OWB_Title,
    MA_OWB_Browser,

    MM_Search,

    /* OWBApp */
    /* - OWBApp Settings*/
    MA_OWBApp_DefaultURL,
    MA_OWBApp_StartPage,
    MA_OWBApp_NewTabPage,
    MA_OWBApp_CLIDevice,

    MA_OWBApp_ShowButtonFrame,
    MA_OWBApp_EnablePointers,
    MA_OWBApp_ShowSearchBar,
    MA_OWBApp_ShowSeparators,
    MA_OWBApp_ShowValidationButtons,
    MA_OWBApp_EnableTabTransferAnim,
    MA_OWBApp_NewPagePolicy,
    MA_OWBApp_PopupPolicy,
    MA_OWBApp_ToolButtonType,
    MA_OWBApp_URLCompletionType,
    MA_OWBApp_NewPageButton,
    MA_OWBApp_CloseRequester,
    MA_OWBApp_ErrorMode,
    MA_OWBApp_ShowFavIcons,
    MA_OWBApp_MiddleButtonPolicy,
    MA_OWBApp_NewPagePosition,

    MA_OWBApp_DefaultFontSize,
    MA_OWBApp_DefaultFixedFontSize,
    MA_OWBApp_MinimumFontSize,
    MA_OWBApp_MinimumLogicalFontSize,
    MA_OWBApp_SansSerifFontFamily,
    MA_OWBApp_SerifFontFamily,
    MA_OWBApp_StandardFontFamily,
    MA_OWBApp_CursiveFontFamily,
    MA_OWBApp_FantasyFontFamily,
    MA_OWBApp_FixedFontFamily,
    MA_OWBApp_TextEncoding,
    MA_OWBApp_SmoothingType,

    MA_OWBApp_ActiveConnections,
    MA_OWBApp_UserAgent,

    MA_OWBApp_ProxyEnabled,
    MA_OWBApp_ProxyType,
    MA_OWBApp_ProxyHost,
    MA_OWBApp_ProxyPort,
    MA_OWBApp_ProxyUsername,
    MA_OWBApp_ProxyPassword,

/*
    MA_OWBApp_PersistantCookiesPolicy,
    MA_OWBApp_TemporaryCookiesPolicy,
*/
    MA_OWBApp_SaveCookies,
    MA_OWBApp_CookiesPolicy,
    MA_OWBApp_EnableLocalStorage,
    MA_OWBApp_SaveFormCredentials,
    MA_OWBApp_EnableFormAutofill,
    MA_OWBApp_SaveSession,
    MA_OWBApp_DeleteSessionAtExit,
    MA_OWBApp_SessionRestoreMode,
    MA_OWBApp_SaveHistory,
    MA_OWBApp_HistoryItemLimit,
    MA_OWBApp_HistoryAgeInDaysLimit,

    MA_OWBApp_IgnoreSSLErrors,
    MA_OWBApp_CertificatePath,

    MA_OWBApp_DownloadDirectory,
    MA_OWBApp_DownloadAutoClose,
    MA_OWBApp_DownloadSave,
    MA_OWBApp_DownloadStartAutomatically,

    MA_OWBApp_EnablePageCache,
    MA_OWBApp_CacheModel,
    MA_OWBApp_EnableJavaScript,
    MA_OWBApp_AllowJavaScriptNewWindow,
    MA_OWBApp_LoadImagesAutomatically,
    MA_OWBApp_PrintBackgrounds,
    MA_OWBApp_EnableAnimation,
    MA_OWBApp_EnableAnimationLoop,
    MA_OWBApp_ShouldPrintBackgrounds,
    MA_OWBApp_EnableContentBlocking,
    MA_OWBApp_EnableInspector,
    MA_OWBApp_EnablePlugins,
    //MA_OWBApp_AnimationPolicy,

    MA_OWBApp_EnableVP8,
    MA_OWBApp_EnableFLV,
    MA_OWBApp_EnableOgg,
    MA_OWBApp_EnableMP4,
    MA_OWBApp_EnablePartialContent,
    MA_OWBApp_LoopFilterMode,

    MA_OWBApp_QuickLinkLook,
    MA_OWBApp_QuickLinkLayout,
    MA_OWBApp_QuickLinkRows,
    MA_OWBApp_QuickLinkColums,
    MA_OWBApp_QuickLinkPosition,

    MA_OWBApp_CurrentDirectory,
    MA_OWBApp_DownloadsInProgress,
    MA_OWBApp_LastActiveWindow,
    MA_OWBApp_ShouldShowCloseRequester,
    MA_OWBApp_ShouldAnimate,
    MA_OWBApp_PrivateBrowsingClients,

    /* - OWBApp Sticky Settings */
    MA_OWBApp_ShowBookmarkPanel,
    MA_OWBApp_ShowHistoryPanel,
    MA_OWBApp_PanelWeight,
    MA_OWBApp_Bookmark_AddToMenu,
    MA_OWBApp_ContinuousSpellChecking,
    MA_OWBApp_PrivateBrowsing,

    /* - OWBApp Misc */
    MA_OWBApp_ActiveWindow,
    MA_OWBApp_ActiveBrowser,
    MA_OWBApp_FullScreen,
    MA_OWBApp_DidReceiveFavIcon,
    MA_OWBApp_FavIconImportComplete,
    MA_OWBApp_HistoryChanged,
    MA_OWBApp_BookmarksChanged,
    MA_OWBApp_BookmarkWindow,
    MA_OWBApp_DownloadWindow,
    MA_OWBApp_SearchManagerWindow,
    MA_OWBApp_ScriptManagerWindow,
    MA_OWBApp_PrinterWindow,

    /* - OWBApp Methods */
    MM_OWBApp_DisposeObject,
    MM_OWBApp_DisposeWindow,
    MM_OWBApp_PrefsLoad,
    MM_OWBApp_PrefsSave,
    MM_OWBApp_PrefsCancel,
    MM_OWBApp_OpenWindow,
    MM_OWBApp_About,
    MM_OWBApp_Quit,
    MM_OWBApp_AddPage, /* Decides if new tab or window shall be opened */
    MM_OWBApp_AddWindow,
    MM_OWBApp_RemoveWindow,
    MM_OWBApp_AddBrowser,
    MM_OWBApp_RemoveBrowser,
    MM_OWBApp_WebKitEvents,
    MM_OWBApp_Expose,

    MM_OWBApp_Download,
    MM_OWBApp_DownloadDone,
    MM_OWBApp_DownloadError,
    MM_OWBApp_DownloadCancelled,
    MM_OWBApp_DownloadUpdate,

    MM_OWBApp_RestoreSession,
    MM_OWBApp_SaveSession,
    MM_OWBApp_Login,
    MM_OWBApp_SetCredential,
    MM_OWBApp_MailTo,
    MM_OWBApp_SetFormState,
    MM_OWBApp_SaveFormState,

    MM_OWBApp_BookmarksChanged,
    MM_OWBApp_LoadBookmarks,
    MM_OWBApp_AddConsoleMessage,
    MM_OWBApp_UpdateBookmarkPanels,
    MM_OWBApp_BuildUserMenu,
    MM_OWBApp_SelectUserMenu,
    MM_OWBApp_ClearContextMenu,
    MM_OWBApp_LoadContextMenu,
    MM_OWBApp_SaveContextMenu,

    MM_OWBApp_CanShowMediaMimeType,
    MM_OWBApp_RequestPolicyForMimeType,
    MM_OWBApp_ProcessResourceClientAction,

    MM_OWBApp_EraseFavicons,
    MM_OWBApp_ResetVM,

    /* OWBWindow */
    MA_OWBWindow_ActiveBrowser,
    MA_OWBWindow_ActiveWebInspector,
    MA_OWBWindow_AddressBarGroup,
    MA_OWBWindow_FastLinkGroup,
    MA_OWBWindow_FastLinkParentGroup,
    MA_OWBWindow_NetworkLedsGroupBROKEN,
    MA_OWBWindow_SearchGroup,
    MA_OWBWindow_NavigationGroup,
    MA_OWBWindow_BookmarkPanelGroup,
    MA_OWBWindow_HistoryPanelGroup,

    MA_OWBWindow_InitialBrowser,
    MA_OWBWindow_Features,
    MA_OWBWindow_ShowBookmarkPanel,
    MA_OWBWindow_ShowHistoryPanel,
    MA_OWBWindow_ShowAddressBarGroup,
    MA_OWBWindow_ShowSearchGroup,
    MA_OWBWindow_ShowQuickLinkGroup,
    MA_OWBWindow_ShowNavigationGroup,
    MA_OWBWindow_ShowStatusGroup,
    MA_OWBWindow_ShowSeparators,

    MM_OWBWindow_Close,
    MM_OWBWindow_OpenLocalFile,
    MM_OWBWindow_SaveAsSource,
    MM_OWBWindow_SaveAsPDF,
    MM_OWBWindow_Print,

    MM_OWBWindow_LoadURL,
    MM_OWBWindow_Back,
    MM_OWBWindow_Forward,
    MM_OWBWindow_Stop,
    MM_OWBWindow_Home,
    MM_OWBWindow_Reload,
    MM_OWBWindow_Find,

    MM_OWBWindow_JavaScriptPrompt,
    MM_OWBWindow_AutoComplete,

    MM_OWBWindow_UpdateTitle,
    MM_OWBWindow_UpdateState,
    MM_OWBWindow_UpdateZone,
    MM_OWBWindow_UpdateSecurity,
    MM_OWBWindow_UpdatePrivateBrowsing,
    MM_OWBWindow_UpdateUserScript,
    MM_OWBWindow_UpdateProgress,
    MM_OWBWindow_UpdateStatus,
    MM_OWBWindow_UpdateURL,
    MM_OWBWindow_UpdateNavigation,
    MM_OWBWindow_UpdateMenu,
    MM_OWBWindow_BuildSpoofMenu,
    MM_OWBWindow_FullScreen,

    MM_OWBWindow_AddBrowser,
    MM_OWBWindow_RemoveBrowser,
    MM_OWBWindow_DetachBrowser,
    MM_OWBWindow_TransferBrowser,
    MM_OWBWindow_ActivePage,
    MM_OWBWindow_InspectPage,
    MM_OWBWindow_CreateInspector,
    MM_OWBWindow_DestroyInspector,
    MM_OWBWindow_MenuAction,
    MM_OWBWindow_InsertBookmark,
    MM_OWBWindow_InsertBookmarkAskTitle,
    MM_OWBWindow_UpdateBookmarkPanel,
    MM_OWBWindow_UpdatePanelGroup,
    MM_OWBWindow_AddClosedView,
    MM_OWBWindow_AddHistoryItem,
    MM_OWBWindow_RemoveHistoryPanel,
    MM_OWBWindow_RemoveBookmarkPanel,

    /* OWBGroup */
    MA_OWBGroup_Browser,
    MA_OWBGroup_MediaControlsGroup,
    MA_OWBGroup_InspectorGroup,

    /* OWBBrowser */
    MA_OWBBrowser_Widget,
    MA_OWBBrowser_URL,
    MA_OWBBrowser_EditedURL,
    MA_OWBBrowser_IsFrame,
    MA_OWBBrowser_Title,
    MA_OWBBrowser_Window,
    MA_OWBBrowser_StatusText,
    MA_OWBBrowser_ToolTipText,
    MA_OWBBrowser_Loading,
    MA_OWBBrowser_LoadingProgress,
    MA_OWBBrowser_State,
    MA_OWBBrowser_TitleObj,
    MA_OWBBrowser_Pointer,
    MA_OWBBrowser_BackAvailable,
    MA_OWBBrowser_ForwardAvailable,
    MA_OWBBrowser_ReloadAvailable,
    MA_OWBBrowser_StopAvailable,
    MA_OWBBrowser_SourceView,
    MA_OWBBrowser_Active,
    MA_OWBBrowser_DragURL,
    MA_OWBBrowser_DragImage,
    MA_OWBBrowser_DragData,
    MA_OWBBrowser_DragOperation,
    MA_OWBBrowser_Security,
    MA_OWBBrowser_Zone,
    MA_OWBBrowser_VideoElement,
    MA_OWBBrowser_VideoDecoder,
    MA_OWBBrowser_ParentBrowser, // Browser it was opened from
    MA_OWBBrowser_ParentGroup,   // OWBGroup the browser belongs to
    MA_OWBBrowser_ForbidEvents,  // Locking events (to print safely in another thread)

    MA_OWBBrowser_VBar,
    MA_OWBBrowser_VBarGroup,
    MA_OWBBrowser_HBar,
    MA_OWBBrowser_HBarGroup,

    MA_OWBBrowser_VTotalPixel,
    MA_OWBBrowser_VTopPixel,
    MA_OWBBrowser_VVisiblePixel,

    MA_OWBBrowser_HTotalPixel,
    MA_OWBBrowser_HTopPixel,
    MA_OWBBrowser_HVisiblePixel,

    MM_OWBBrowser_ReturnFocus,
    MM_OWBBrowser_Expose,
    MM_OWBBrowser_Update,
    MM_OWBBrowser_Scroll,
    MM_OWBBrowser_PopupMenu,
    MM_OWBBrowser_Autofill_ShowPopup,
    MM_OWBBrowser_Autofill_HidePopup,
    MM_OWBBrowser_Autofill_DidSelect,
    MM_OWBBrowser_Autofill_SaveTextFields,
    MM_OWBBrowser_Autofill_DidChangeInTextField,
    MM_OWBBrowser_Autofill_HandleNavigationEvent,
    MM_OWBBrowser_Print,
    MM_OWBBrowser_FocusChanged,
    MM_OWBBrowser_DidCommitLoad,
    MM_OWBBrowser_DidStartProvisionalLoad,
    MM_OWBBrowser_WillCloseFrame,
    MM_OWBBrowser_VideoEnterFullPage,
    MM_OWBBrowser_VideoBlit,
    MM_OWBBrowser_SetScrollOffset,
    MM_OWBBrowser_UpdateScrollers,
    MM_OWBBrowser_UpdateNavigation,
    MM_OWBBrowser_AutoScroll_Perform,
    MM_OWBBrowser_ColorChooser_ShowPopup,
    MM_OWBBrowser_ColorChooser_HidePopup,
    MM_OWBBrowser_DateTimeChooser_ShowPopup,
    MM_OWBBrowser_DateTimeChooser_HidePopup,

    /* Per browser setting */
    MA_OWBBrowser_PrivateBrowsing,
    MA_OWBBrowser_ContentBlocking,
    MA_OWBBrowser_PluginsEnabled,
    MA_OWBBrowser_JavaScriptEnabled,
    MA_OWBBrowser_LoadImagesAutomatically,
    MA_OWBBrowser_PlayAnimations,
    MA_OWBBrowser_UserAgent,
    // ...

    /* Navigation */
    MA_Navigation_BackForwardList,
    MA_Navigation_BackEnabled,
    MA_Navigation_ForwardEnabled,
    MA_Navigation_ReloadEnabled,
    MA_Navigation_StopEnabled,

    /* Address bar */
    MA_AddressBarGroup_Active,
    MA_AddressBarGroup_GoButton,
    MA_AddressBarGroup_PopString,

    MM_AddressBarGroup_LoadHistory,
    MM_AddressBarGroup_Mark,
    MM_AddressBarGroup_CompleteString,

    /* Search bar */
    MA_SearchBarGroup_SearchButton,

    MM_SearchBarGroup_Update,
    MM_SearchBarGroup_LoadURLFromShortcut,

    /* Download window */
    MM_Download_Done,
    MM_Download_Error,
    MM_Download_Cancelled,
    MM_Download_RemoveEntry,
    MM_Download_Cancel,
    MM_Download_Retry,
    MM_Download_ComputeSpeed,
    MM_Download_HilightPage,
    MM_Download_UnhilightPage,

    /* Download list */
    MA_DownloadList_Type,

    /* ToolButton */
    MA_ToolButton_Image,
    MA_ToolButton_Text,
    MA_ToolButton_Type,
    MA_ToolButton_Frame,
    MA_ToolButton_Background,
    MA_ToolButton_Pressed,
    MA_ToolButton_Disabled,

    /* ToolButton_AddBookmark */
    MA_ToolButton_AddBookmark_IsBookmark,

    /* Transfer anim */
    MM_TransferAnim_Run,
    MA_TransferAnim_Animate,    /* ISG */

    /* FindText */
    MA_FindText_Active,
    MA_FindText_Target,
    MA_FindText_Closable,
    MA_FindText_ShowCaseSensitive,
    MA_FindText_ShowButtons,
    MA_FindText_ShowText,
    MA_FindText_ShowAllMatches,
    MM_FindText_DisableButtons,
    MM_FindText_Find,

    /* PopString */
    MM_PopString_Insert,
    MA_PopString_ActivateString,

    /* HistoryList */
    MA_HistoryList_Opened,
    MA_HistoryList_Complete,
    MM_HistoryList_SelectChange,
    MM_HistoryList_Redraw,

    /* FontFamilyPopString */
    MM_FontFamilyPopString_InsertFamily,

    /* Bookmark */
    MA_Bookmarkgroup_Title,
    MA_Bookmarkgroup_Alias,
    MA_Bookmarkgroup_Address,
    MA_Bookmarkgroup_InMenu,
    MA_Bookmarkgroup_Bar,
    MA_Bookmarkgroup_QuickLink,
    MA_Bookmarkgroup_BookmarkObject,
    MA_Bookmarkgroup_BookmarkList,

    MM_Bookmarkgroup_Update,
    MM_Bookmarkgroup_AddLink,
    MM_Bookmarkgroup_AddGroup,
    MM_Bookmarkgroup_Remove,
    MM_Bookmarkgroup_Open,
    MM_Bookmarkgroup_LoadHtml,
    MM_Bookmarkgroup_SaveHtml,
    MM_Bookmarkgroup_BuildNotification,
    MM_Bookmarkgroup_KillNotification,
    MM_Bookmarkgroup_BuildMenu,
    MM_Bookmarkgroup_UpdateMenu,
    MM_Bookmarkgroup_AddQuickLink,
    MM_Bookmarkgroup_ReadQLOrder,
    MM_Bookmarkgroup_RemoveQuickLink,
    MM_Bookmarkgroup_AddSeparator,
    MM_Bookmarkgroup_AutoSave,
    MM_Bookmarkgroup_DosNotify,
    MM_Bookmarkgroup_RegisterQLGroup,
    MM_Bookmarkgroup_UnRegisterQLGroup,
    MM_Bookmarkgroup_ContainsURL,
    MM_Bookmarkgroup_RetainFavicons,

    /* QuickLinkGroup */
    MA_QuickLinkGroup_Data,
    MA_QuickLinkGroup_Row,
    MA_QuickLinkGroup_Mode,
    MA_QuickLinkGroup_Buttons,
    MA_QuickLinkGroup_MinW,
    MA_QuickLinkGroup_MinH,
    MA_QuickLinkGroup_MaxW,
    MA_QuickLinkGroup_MaxH,
    MA_QuickLinkGroup_Parent,
    MM_QuickLinkGroup_Add,
    MM_QuickLinkGroup_Remove,
    MM_QuickLinkGroup_Update,
    MM_QuickLinkGroup_InitChange,
    MM_QuickLinkGroup_ExitChange,

    /* QuickLinkButtonGroup */
    MA_QuickLinkButtonGroup_Node,
    MM_QuickLinkButtonGroup_Update,
    MM_QuickLinkButtonGroup_Redraw,
    MM_QuickLinkButtonGroup_RedrawMenuItem,
    MM_QuickLinkButtonGroup_BuildMenu,

    /* QuickLinkParentGroup */
    MA_QuickLinkParentGroup_Hide,
    MA_QuickLinkParentGroup_QLGroup,

    /* HistoryButton */
    MA_HistoryButton_List,
    MA_HistoryButton_Type,

    /* NetworkLedsGroup */
    MA_NetworkLedsGroup_Count,
    MM_Network_AddJob,
    MM_Network_UpdateJob,
    MM_Network_RemoveJob,

    /* NetworkWindow */
    MM_NetworkWindow_Cancel,

    /* LoginWindow */
    MA_LoginWindow_Host,
    MA_LoginWindow_Realm,
    MA_LoginWindow_Username,
    MA_LoginWindow_Password,
    MA_LoginWindow_SaveAuthentication,
    
    MM_LoginWindow_Open,
    MM_LoginWindow_Login,

    /* Splash Window */
    MM_SplashWindow_Update,
    MM_SplashWindow_Open,

    /* Console Window */
    MM_ConsoleWindow_Add,
    MM_ConsoleWindow_Clear,

    /* Prefs Window */
    MM_PrefsWindow_Fill,

    /* ContextMenuGroup */
    MM_ContextMenuGroup_Add,
    MM_ContextMenuGroup_Remove,
    MM_ContextMenuGroup_Refresh,
    MM_ContextMenuGroup_Change,
    MM_ContextMenuGroup_AddPlaceholder,

    /* MimeTypeGroup */
    MM_MimeTypeGroup_Add,
    MM_MimeTypeGroup_Remove,
    MM_MimeTypeGroup_Refresh,
    MM_MimeTypeGroup_Change,
    MM_MimeTypeGroup_AddPlaceholder,

    /* ChooseTitleGroup */
    MA_ChooseTitleGroup_QuickLink,

    MM_ChooseTitleGroup_Add,
    MM_ChooseTitleGroup_Cancel,

    /* TitleLabel */
    MM_Title_Redraw,

    /* MenuItem */
    MA_MenuItem_FreeUserData,

    /* FavIcon */
    MA_FavIcon_PageURL,
    MA_FavIcon_NeedRedraw,
    MA_FavIcon_ScaleFactor,
    MA_FavIcon_Folder,
    MA_FavIcon_Retain,
    
    MM_FavIcon_DidReceiveFavIcon,
    MM_FavIcon_Update,

    /* Icon */
    MA_Icon_Path,
    MA_Icon_ScaleFactor,

    /* History */
    MM_History_Load,
    MM_History_Clear,
    MM_History_Insert,
    MM_History_Remove,
    MM_History_Update,
    MM_History_ContainsURL,

    /* Password Manager */
    MM_PasswordManagerGroup_Load,
    MM_PasswordManagerGroup_Clear,
    MM_PasswordManagerGroup_Insert,
    MM_PasswordManagerGroup_Remove,
    MM_PasswordManagerGroup_Get,

    /* Cookie Manager */
    MM_CookieManagerGroup_Load,
    MM_CookieManagerGroup_Clear,
    MM_CookieManagerGroup_DidInsert,
    MM_CookieManagerGroup_DidRemove,
    MM_CookieManagerGroup_Remove,
    MM_CookieManagerGroup_DisplayProperties,

    /* Block Manager */
    MM_BlockManagerGroup_Load,
    MM_BlockManagerGroup_DidInsert,
    MM_BlockManagerGroup_Add,
    MM_BlockManagerGroup_Remove,
    MM_BlockManagerGroup_Update,
    MM_BlockManagerGroup_Change,

    /* Search Manager */
    MA_SearchManagerWindow_Group,

    MA_SearchManagerGroup_Changed,
    MA_SearchManagerGroup_Labels,
    MA_SearchManagerGroup_Requests,
    MA_SearchManagerGroup_Shortcuts,

    MM_SearchManagerGroup_Load,
    MM_SearchManagerGroup_Add,
    MM_SearchManagerGroup_Remove,
    MM_SearchManagerGroup_Update,
    MM_SearchManagerGroup_Change,

    /* Script Manager */
    MM_ScriptManagerGroup_Load,
    MM_ScriptManagerGroup_Add,
    MM_ScriptManagerGroup_Remove,
    MM_ScriptManagerGroup_Update,
    MM_ScriptManagerGroup_Change,
    MM_ScriptManagerGroup_WhiteList_Add,
    MM_ScriptManagerGroup_WhiteList_Remove,
    MM_ScriptManagerGroup_BlackList_Add,
    MM_ScriptManagerGroup_BlackList_Remove,
    MM_ScriptManagerGroup_InjectScripts,
    MM_ScriptManagerGroup_ScriptsForURL,

    /* URL prefs */

    MM_URLPrefsGroup_Load,
    MM_URLPrefsGroup_Save,
    MM_URLPrefsGroup_Add,
    MM_URLPrefsGroup_Remove,
    MM_URLPrefsGroup_Change,
    MM_URLPrefsGroup_Refresh,
    MM_URLPrefsGroup_ApplySettingsForURL,
    MM_URLPrefsGroup_MatchesURL,
    MM_URLPrefsGroup_UserAgentForURL,
    MM_URLPrefsGroup_CookiePolicyForURLAndName,

    /* MediaControlsGroup */
    MA_MediaControlsGroup_Update,
    MA_MediaControlsGroup_Browser,

    MM_MediaControlsGroup_PlayPause,
    MM_MediaControlsGroup_Mute,
    MM_MediaControlsGroup_FullScreen,
    MM_MediaControlsGroup_Seek,
    MM_MediaControlsGroup_Volume,
    MM_MediaControlsGroup_Update,

    /* SuggestList */
    MA_SuggestList_Opened,
    MM_SuggestList_SelectChange,

    /* SuggestPopString */
    MM_SuggestPopString_Insert,
    MM_SuggestPopString_Initiate,
    MM_SuggestPopString_Abort,

    /* PrinterWindow */
    MM_PrinterWindow_PrintDocument,
    MM_PrinterWindow_Start,
    MM_PrinterWindow_Stop,
    MM_PrinterWindow_Done,
    MM_PrinterWindow_Close,
    MM_PrinterWindow_StatusUpdate,
    MM_PrinterWindow_PrinterPrefs,
    MM_PrinterWindow_UpdateParameters,

    /* AutofillPopup */
    MA_AutofillPopup_Source,
    MA_AutofillPopup_Suggestions,
    MA_AutofillPopup_Rect,

    MM_AutofillPopup_DidSelect,
    MM_AutofillPopup_Update,
    MM_AutofillPopup_HandleNavigationEvent,

    /* ColorChooserPopup */
    MA_ColorChooserPopup_Source,
    MA_ColorChooserPopup_Controller,
    MA_ColorChooserPopup_InitialColor,

    MM_ColorChooserPopup_DidSelect,

    /* DateTimeChooserPopup */
    MA_DateTimeChooserPopup_Source,
    MA_DateTimeChooserPopup_Controller,

    MM_DateTimeChooserPopup_DidSelect,

    MA_dummyend
};

enum
{
    /* Plugin Interface, don't change that offset, and reflect any addition to plugin sdk. */
    MM_Plugin_RenderRastPort = (TAG_USER|0xDEAD8000) + 0x0001, //
    MM_Plugin_GetWidth,
    MM_Plugin_GetHeight,
    MM_Plugin_GetTop,
    MM_Plugin_GetLeft,
    MM_Plugin_GetSurface, //
    MM_Plugin_IsBrowserActive, //
    MM_Plugin_AddIDCMPHandler, //
    MM_Plugin_RemoveIDCMPHandler, //
    MM_Plugin_AddTimeOut, //
    MM_Plugin_RemoveTimeOut, //
    MM_Plugin_GetBitMapAttr,
    MM_Plugin_SetMousePointer,
    MM_Plugin_Message, //
};

/* Method Structures */

/* OWBApp */

struct MP_Search {
    STACKED ULONG MethodID;
    STACKED STRPTR string;
    STACKED ULONG flags;
};

struct MP_OWBApp_DisposeObject {
    STACKED ULONG MethodID;
    STACKED APTR obj;
};

struct MP_OWBApp_DisposeWindow {
    STACKED ULONG MethodID;
    STACKED APTR obj;
};

struct MP_OWBApp_PrefsSave {
    STACKED ULONG MethodID;
    STACKED ULONG SaveENVARC;
};

struct MP_OWBApp_OpenWindow {
    STACKED ULONG MethodID;
    STACKED ULONG window_id;
    STACKED ULONG activate;
};

struct MP_OWBApp_AddPage {
    STACKED ULONG MethodID;
    STACKED TEXT* url;
    STACKED ULONG isframe;
    STACKED ULONG donotactivate;
    STACKED APTR  windowfeatures;
    STACKED Object *window;
    STACKED ULONG privatebrowsing;
    STACKED ULONG addtoend;
};

struct MP_OWBApp_AddWindow {
    STACKED ULONG MethodID;
    STACKED TEXT* url;
    STACKED ULONG isframe;
    STACKED APTR  sourceview;
    STACKED ULONG urlfocus;
    STACKED APTR  windowfeatures;
    STACKED ULONG privatebrowsing;
};

struct MP_OWBApp_RemoveWindow {
    STACKED ULONG MethodID;
    STACKED Object *window;
};

struct MP_OWBApp_AddBrowser {
    STACKED ULONG MethodID;
    STACKED Object* window;
    STACKED TEXT* url;
    STACKED ULONG isframe;
    STACKED APTR  sourceview;
    STACKED ULONG donotactivate;
    STACKED ULONG privatebrowsing;
    STACKED ULONG addtoend;
};

struct MP_OWBApp_RemoveBrowser {
    STACKED ULONG MethodID;
    STACKED Object* browser;
};

struct MP_OWBApp_Download {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
    STACKED STRPTR suggestedname;
    STACKED APTR webdownload;
};

struct MP_OWBApp_DownloadDone {
    STACKED ULONG MethodID;
    STACKED APTR entry;
};

struct MP_OWBApp_DownloadError {
    STACKED ULONG MethodID;
    STACKED APTR entry;
    STACKED char *error;
};

struct MP_OWBApp_DownloadCancelled {
    STACKED ULONG MethodID;
    STACKED APTR entry;
};

struct MP_OWBApp_DownloadUpdate {
    STACKED ULONG MethodID;
    STACKED APTR entry;
};

struct MP_OWBApp_Login {
    STACKED ULONG MethodID;
    STACKED TEXT *host;
    STACKED TEXT *realm;
    STACKED TEXT **username;
    STACKED TEXT **password;
    STACKED ULONG *persistence;
};

struct MP_OWBApp_SetCredential {
    STACKED ULONG MethodID;
    STACKED APTR host;
    STACKED APTR realm;
    STACKED APTR credential;
};

struct MP_OWBApp_SetFormState {
    STACKED ULONG MethodID;
    STACKED APTR host;
    STACKED APTR doc;
};

struct MP_OWBApp_SaveFormState {
    STACKED ULONG MethodID;
    STACKED APTR webView;
    STACKED APTR request;
};

struct MP_OWBApp_MailTo {
    STACKED ULONG MethodID;
    STACKED TEXT *url;
};

struct MP_OWBApp_AddConsoleMessage {
    STACKED ULONG MethodID;
    STACKED STRPTR message;
};

struct MP_OWBApp_BuildUserMenu
{
    STACKED ULONG MethodID;
    STACKED int  category;
    STACKED APTR controller;
    STACKED APTR menu;
};

struct MP_OWBApp_SelectUserMenu
{
    STACKED ULONG MethodID;
    STACKED APTR item;
    STACKED APTR menucontroller;
};

struct MP_OWBApp_CanShowMediaMimeType
{
    STACKED ULONG MethodID;
    STACKED STRPTR mimetype;
};

struct MP_OWBApp_RequestPolicyForMimeType
{
    STACKED ULONG MethodID;
    STACKED APTR response;
    STACKED APTR request;
    STACKED APTR webview;
    STACKED APTR webframe;
    STACKED APTR listener;
};

struct MP_OWBApp_ProcessResourceClientAction {
    STACKED ULONG MethodID;
    STACKED ULONG action;
    STACKED APTR client;
    STACKED APTR job;
    STACKED APTR response;
    STACKED APTR data;
    STACKED int    totalsize;
    STACKED APTR redirectedrequest;
    STACKED APTR resourceerror;
    STACKED APTR authenticationchallenge;
};

struct MP_OWBApp_RestoreSession {
    STACKED ULONG MethodID;
    STACKED ULONG ask;
    STACKED ULONG selectfile;
};

struct MP_OWBApp_SaveSession {
    STACKED ULONG MethodID;
    STACKED ULONG selectfile;
};

/* OWBWindow */
struct MP_OWBWindow_AddBrowser {
    STACKED ULONG MethodID;
    STACKED TEXT* url;
    STACKED ULONG isframe;
    STACKED APTR  sourceview;
    STACKED ULONG donotactivate;
    STACKED ULONG privatebrowsing;
    STACKED ULONG addtoend;
};

struct MP_OWBWindow_RemoveBrowser {
    STACKED ULONG MethodID;
    STACKED Object* browser;
};

struct MP_OWBWindow_DetachBrowser {
    STACKED ULONG MethodID;
    STACKED Object* browser;
    STACKED Object* window;
};

struct MP_OWBWindow_TransferBrowser {
    STACKED ULONG MethodID;
    STACKED Object* browser;
};

struct MP_OWBWindow_ActivePage {
    STACKED ULONG MethodID;
    STACKED ULONG pagenum;
};

struct MP_OWBWindow_CreateInspector {
    STACKED ULONG MethodID;
    STACKED Object* browser;
};

struct MP_OWBWindow_DestroyInspector {
    STACKED ULONG MethodID;
    STACKED Object* browser;
};

struct MP_OWBWindow_LoadURL {
    STACKED ULONG MethodID;
    STACKED TEXT* url;
    STACKED APTR browser;
};

struct MP_OWBWindow_Reload {
    STACKED ULONG MethodID;
    STACKED Object* browser;
};

struct MP_OWBWindow_MenuAction {
    STACKED ULONG MethodID;
    STACKED IPTR  action;
};

struct MP_OWBWindow_UpdateTitle {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED TEXT* title;
};

struct MP_OWBWindow_UpdateStatus {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED TEXT *status;
};

struct MP_OWBWindow_UpdateURL {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED TEXT *url;
};

struct MP_OWBWindow_UpdateNavigation {
    STACKED ULONG MethodID;
    STACKED APTR browser;
};

struct MP_OWBWindow_UpdateProgress {
    STACKED ULONG MethodID;
    STACKED APTR browser;
};

struct MP_OWBWindow_UpdateState {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED ULONG state;
};

struct MP_OWBWindow_UpdateZone {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED ULONG zone;
};

struct MP_OWBWindow_UpdateSecurity {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED ULONG security;
};

struct MP_OWBWindow_UpdatePrivateBrowsing {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED ULONG enabled;
};

struct MP_OWBWindow_UpdateUserScript {
    STACKED ULONG MethodID;
    STACKED APTR browser;
    STACKED STRPTR url;
};

struct MP_OWBWindow_UpdateMenu {
    STACKED ULONG MethodID;
    STACKED APTR browser;
};

struct MP_OWBWindow_AddClosedView {
    STACKED ULONG MethodID;
    STACKED APTR browser;
};

struct MP_OWBWindow_AddHistoryItem {
    STACKED ULONG MethodID;
    STACKED APTR item;
};

struct MP_OWBWindow_Find {
    STACKED ULONG MethodID;
    STACKED STRPTR string;
    STACKED ULONG flags;
};

struct MP_OWBWindow_InsertBookmarkAskTitle {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
    STACKED STRPTR title;
    STACKED ULONG quicklink;
};

struct MP_OWBWindow_JavaScriptPrompt {
    STACKED ULONG MethodID;
    STACKED STRPTR message;
    STACKED STRPTR defaultvalue;
};

struct MP_OWBWindow_AutoComplete {
    STACKED ULONG MethodID;
    STACKED LONG len;
    STACKED LONG prevlen;
};

struct MP_OWBWindow_FullScreen {
    STACKED ULONG MethodID;
    STACKED ULONG fullscreen;
};

/* OWBBrowser */

struct MP_OWBBrowser_Expose {
    STACKED ULONG MethodID;
    STACKED ULONG updatecontrols;
};

struct MP_OWBBrowser_Update {
    STACKED ULONG MethodID;
    STACKED APTR rect;
    STACKED ULONG sync;
};

struct MP_OWBBrowser_Scroll {
    STACKED ULONG MethodID;
    STACKED int dx;
    STACKED int dy;
    STACKED APTR rect;
};

struct MP_OWBBrowser_DidStartProvisionalLoad {
    STACKED ULONG MethodID;
    STACKED APTR webframe;
};

struct MP_OWBBrowser_DidCommitLoad {
    STACKED ULONG MethodID;
    STACKED APTR webframe;
};

struct MP_OWBBrowser_WillCloseFrame {
    STACKED ULONG MethodID;
    STACKED APTR webframe;
};

struct MP_OWBBrowser_PopupMenu {
    STACKED ULONG MethodID;
    STACKED APTR popupinfo;
};

struct MP_OWBBrowser_Autofill_ShowPopup {
    STACKED ULONG MethodID;
    STACKED APTR suggestions;
    STACKED APTR rect;
};

struct MP_OWBBrowser_Autofill_HidePopup {
    STACKED ULONG MethodID;
};

struct MP_OWBBrowser_Autofill_DidSelect {
    STACKED ULONG MethodID;
    STACKED APTR value;
};

struct MP_OWBBrowser_Autofill_SaveTextFields {
    STACKED ULONG MethodID;
    STACKED APTR form;
};

struct MP_OWBBrowser_Autofill_DidChangeInTextField {
    STACKED ULONG MethodID;
    STACKED APTR element;
};

struct MP_OWBBrowser_Autofill_HandleNavigationEvent {
    STACKED ULONG MethodID;
    STACKED APTR event;
};

struct MP_OWBBrowser_ColorChooser_ShowPopup {
    STACKED ULONG MethodID;
    STACKED APTR client;
    STACKED APTR color;
};

struct MP_OWBBrowser_ColorChooser_HidePopup {
    STACKED ULONG MethodID;
};

struct MP_OWBBrowser_DateTimeChooser_ShowPopup {
    STACKED ULONG MethodID;
    STACKED APTR client;
};

struct MP_OWBBrowser_DateTimeChooser_HidePopup {
    STACKED ULONG MethodID;
};

struct MP_OWBBrowser_Print {
    STACKED ULONG MethodID;
    STACKED STRPTR file;
    STACKED ULONG headerheight;
    STACKED ULONG footerheight;
    STACKED APTR scalefactor; // ptr to double
    STACKED ULONG mode;
};

struct MP_OWBBrowser_SetScrollOffset {
    STACKED ULONG MethodID;
    STACKED LONG x;
    STACKED LONG y;
};

struct MP_OWBBrowser_VideoEnterFullPage {
    STACKED ULONG MethodID;
    STACKED APTR element;
    STACKED ULONG fullscreen;
};

struct MP_OWBBrowser_VideoBlit {
    STACKED ULONG MethodID;
    STACKED unsigned char **src;
    STACKED int *stride;
    STACKED int    width;
    STACKED int height;
};

/* AddressBarGroup */

struct MP_AddressBarGroup_Mark {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
};

struct MP_AddressBarGroup_CompleteString {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
    STACKED ULONG cursorpos;
    STACKED ULONG markstart;
    STACKED ULONG markend;
};

/* SearchBarGroup */

struct MP_SearchBarGroup_LoadURLFromShortcut {
    STACKED ULONG MethodID;
    STACKED STRPTR shortcut;
    STACKED STRPTR string;
};

/* Download */
struct MP_Download_Done {
    STACKED ULONG MethodID;
    STACKED APTR entry;
};

struct MP_Download_Error {
    STACKED ULONG MethodID;
    STACKED APTR entry;
    STACKED char *error;
};

struct MP_Download_Cancelled {
    STACKED ULONG MethodID;
    STACKED APTR entry;
};

struct MP_Download_RemoveEntry {
    STACKED ULONG MethodID;
    STACKED APTR listview;
    STACKED LONG all;
};

struct MP_Download_Cancel {
    STACKED ULONG MethodID;
    STACKED LONG all;
};

struct MP_Download_Retry {
    STACKED ULONG MethodID;
    STACKED APTR listview;
};

struct MP_Download_HilightPage {
    STACKED ULONG MethodID;
    STACKED ULONG pagenum;
};

struct MP_Download_UnhilightPage {
    STACKED ULONG MethodID;
    STACKED ULONG pagenum;
};

/* FindText */
struct MP_FindText_DisableButtons {
    STACKED ULONG MethodID;
    STACKED LONG prev;
    STACKED LONG next;
};

struct MP_FindText_Find {
    STACKED ULONG MethodID;
    STACKED STRPTR string;
    STACKED ULONG flags;
    STACKED ULONG validate;
};

/* PopString */
struct MP_PopString_Insert {
    STACKED ULONG MethodID;
    STACKED TEXT* txt;
};

struct MP_FontFamilyPopString_InsertFamily {
    STACKED ULONG MethodID;
    STACKED TEXT* txt;
};

/* HistoryList */
struct MP_HistoryList_Redraw {
    STACKED ULONG MethodID;
    STACKED APTR entry;
};

/* Bookmark */
struct MP_Bookmarkgroup_Update  {
    STACKED ULONG MethodID;
    STACKED ULONG from;
};

struct MP_Bookmarkgroup_AddLink {
    STACKED ULONG MethodID;
    STACKED TEXT* title;
    STACKED TEXT* alias;
    STACKED TEXT* address;
    STACKED ULONG external;
    STACKED ULONG menu;
    STACKED ULONG quicklink;
};

struct MP_Bookmarkgroup_AddGroup {
    STACKED ULONG MethodID;
    STACKED TEXT* title;
};

struct MP_Bookmarkgroup_LoadHtml {
    STACKED LONG MethodID;
    STACKED TEXT* file;
};

struct MP_Bookmarkgroup_SaveHtml {
    STACKED LONG MethodID;
    STACKED TEXT* file;
    STACKED ULONG type;
};

struct MP_Bookmarkgroup_BuildMenu {
    STACKED LONG MethodID;
    STACKED Object *menu;
};

struct MP_Bookmarkgroup_RegisterQLGroup {
    STACKED LONG MethodID;
    STACKED Object *group;
    STACKED Object *parent;
};

struct MP_Bookmarkgroup_UnRegisterQLGroup {
    STACKED LONG MethodID;
    STACKED Object *group;
};

struct MP_Bookmarkgroup_AddQuickLink {
    STACKED LONG MethodID;
    STACKED struct treedata *node;
    STACKED LONG    pos;
};

struct MP_Bookmarkgroup_RemoveQuickLink {
    STACKED LONG MethodID;
    STACKED struct treedata *node;
};

struct MP_Bookmarkgroup_ContainsURL {
    STACKED LONG MethodID;
    STACKED void *url;
};

/* QuickLinkButtonGroup */
struct MP_QuickLinkButtonGroup_RedrawMenuItem {
    STACKED LONG MethodID;
    STACKED APTR obj;
};

/* QuickLinkGroup */
struct MP_QuickLinkGroup_Add {
    STACKED LONG MethodID;
    STACKED struct treedata *td;
};

struct MP_QuickLinkGroup_Remove {
    STACKED LONG MethodID;
    STACKED struct treedata *td;
};

struct MP_QuickLinkGroup_Update {
    STACKED LONG MethodID;
    STACKED struct treedata *td;
};

struct MP_QuickLinkGroup_InitChange {
    STACKED LONG MethodID;
    STACKED ULONG mode;
};

struct MP_QuickLinkGroup_ExitChange {
    STACKED LONG MethodID;
    STACKED ULONG mode;
};

/* NetworkLedsGroup */
struct MP_Network_AddJob {
    STACKED LONG MethodID;
    STACKED APTR job;
};

struct MP_Network_UpdateJob {
    STACKED LONG MethodID;
    STACKED APTR job;
};

struct MP_Network_RemoveJob {
    STACKED LONG MethodID;
    STACKED APTR job;
};

/* NetworkWindow */
struct MP_NetworkWindow_Cancel {
    STACKED LONG MethodID;
    STACKED LONG all;
};

/* LoginWindow */
struct MP_LoginWindow_Login {
    STACKED LONG MethodID;
    STACKED ULONG validate;
};

/* SplashWindow */
struct MP_SplashWindow_Update {
    STACKED LONG MethodID;
    STACKED int current;
    STACKED int total;
    STACKED TEXT* file;
};

/* ConsoleWindow */
struct MP_ConsoleWindow_Add {
    STACKED ULONG MethodID;
    STACKED STRPTR message;
};

/* FavIcon */
struct MP_FavIcon_DidReceiveFavIcon {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
};

/* History */
struct MP_History_Insert {
    STACKED ULONG MethodID;
    STACKED APTR item;
};

struct MP_History_Remove {
    STACKED ULONG MethodID;
    STACKED APTR item;
};

struct MP_History_ContainsURL {
    STACKED LONG MethodID;
    STACKED void *url;
};

/* Password Manager */
struct MP_PasswordManagerGroup_Insert {
    STACKED ULONG MethodID;
    STACKED APTR host;
    STACKED APTR credential;
};

struct MP_PasswordManagerGroup_Remove {
    STACKED ULONG MethodID;
    STACKED APTR host;
};

struct MP_PasswordManagerGroup_Get {
    STACKED ULONG MethodID;
    STACKED APTR host;
};

/* Cookie Manager */
struct MP_CookieManagerGroup_DidInsert {
    STACKED ULONG MethodID;
    STACKED APTR cookie;
};

struct MP_CookieManagerGroup_DidRemove {
    STACKED ULONG MethodID;
    STACKED APTR cookie;
};

struct MP_CookieManagerGroup_Remove {
    STACKED ULONG MethodID;
};

/* Block Manager */
struct MP_BlockManagerGroup_DidInsert {
    STACKED ULONG MethodID;
    STACKED STRPTR rule;
    STACKED int type;
    STACKED APTR ptr;
};

/* Script Manager */
struct MP_ScriptManagerGroup_InjectScripts {
    STACKED ULONG MethodID;
    STACKED APTR webView;
};

struct MP_ScriptManagerGroup_ScriptsForURL {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
};

struct MP_ScriptManagerGroup_Update {
    STACKED ULONG MethodID;
    STACKED ULONG which;
};

/* URLPrefsGroup */
struct MP_URLPrefsGroup_ApplySettingsForURL {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
    STACKED APTR webView;
};

struct MP_URLPrefsGroup_MatchesURL {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
};

struct MP_URLPrefsGroup_UserAgentForURL {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
    STACKED APTR webView;
};

struct MP_URLPrefsGroup_CookiePolicyForURLAndName {
    STACKED ULONG MethodID;
    STACKED STRPTR url;
    STACKED STRPTR name;
};

/* SuggestPopString */
struct MP_SuggestPopString_Insert {
    STACKED ULONG MethodID;
    STACKED APTR item;
};

/* PrinterWindow */
struct MP_PrinterWindow_PrintDocument {
    STACKED ULONG MethodID;
    STACKED APTR frame;
};

/* AutofillPopup */
struct MP_AutofillPopup_Update {
    STACKED ULONG MethodID;
    STACKED void *suggestions;
};

struct MP_AutofillPopup_DidSelect {
    STACKED ULONG MethodID;
    STACKED LONG idx;
    STACKED ULONG close;
};

struct MP_AutofillPopup_HandleNavigationEvent {
    STACKED ULONG MethodID;
    STACKED LONG event;
};

/* ColorChooserPopup */
struct MP_ColorChooserPopup_DidSelect {
    STACKED ULONG MethodID;
    STACKED ULONG close;
};

/* DateTimeChooserPopup */
struct MP_DateTimeChooserPopup_DidSelect {
    STACKED ULONG MethodID;
    STACKED ULONG close;
};

/* Plugins */

struct MP_Plugin_RenderRastPort {
    STACKED ULONG MethodID;
    STACKED void* rect;
    STACKED void* windowrect;
    STACKED APTR src;
    STACKED ULONG stride;
};

struct MP_Plugin_AddTimeOut {
    STACKED ULONG MethodID;
    STACKED LONG delay;
    STACKED APTR timeoutfunc;
    STACKED APTR instance;
};

struct MP_Plugin_RemoveTimeOut {
    STACKED ULONG MethodID;
    STACKED APTR timer;
};

struct MP_Plugin_AddIDCMPHandler {
    STACKED ULONG MethodID;
    STACKED APTR  handlerfunc;
    STACKED APTR  instance;
};

struct MP_Plugin_RemoveIDCMPHandler {
    STACKED ULONG MethodID;
    STACKED APTR instance;
};

struct MP_Plugin_Message {
    STACKED ULONG MethodID;
    STACKED STRPTR message;
};

/* Variables */

/* App policy*/
#define MV_OWBApp_NewPagePolicy_Tab    0
#define MV_OWBApp_NewPagePolicy_Window 1

#define MV_OWBApp_NewPagePosition_Last         0
#define MV_OWBApp_NewPagePosition_After_Active 1

#define MV_OWBApp_CookiePolicy_Accept 0
#define MV_OWBApp_CookiePolicy_Reject 1
#define MV_OWBApp_CookiePolicy_Ask    2

#define MV_OWBApp_URLCompletionType_None         0
#define MV_OWBApp_URLCompletionType_Autocomplete 1
#define MV_OWBApp_URLCompletionType_Popup        2

#define MV_OWBApp_CloseRequester_Always        0
#define MV_OWBApp_CloseRequester_Never         1
#define MV_OWBApp_CloseRequester_IfSeveralTabs 2

#define MV_OWBApp_BuildUserMenu_Link  0
#define MV_OWBApp_BuildUserMenu_Image 1
#define MV_OWBApp_BuildUserMenu_Page  2

#define MV_OWBApp_ErrorMode_Requester  0
#define MV_OWBApp_ErrorMode_Page       1

#define MV_OWBApp_ShowFavIcons_Tab           1
#define MV_OWBApp_ShowFavIcons_Bookmark      2
#define MV_OWBApp_ShowFavIcons_History       4
#define MV_OWBApp_ShowFavIcons_HistoryPanel  8
#define MV_OWBApp_ShowFavIcons_QuickLink     16

#define MV_OWBApp_MiddleButtonPolicy_NewBackgroundTab 0
#define MV_OWBApp_MiddleButtonPolicy_NewTab           1
#define MV_OWBApp_MiddleButtonPolicy_NewWindow        2

#define MV_OWBApp_SessionRestore_Off 0
#define MV_OWBApp_SessionRestore_Ask 1
#define MV_OWBApp_SessionRestore_On  2

#define MV_OWBApp_LoopFilterMode_SkipNone    0
#define MV_OWBApp_LoopFilterMode_SkipNonRef  1
#define MV_OWBApp_LoopFilterMode_SkipNonKey  2
#define MV_OWBApp_LoopFilterMode_SkipAll     3

/* Browser */
#define MV_OWBBrowser_State_Ready      0
#define MV_OWBBrowser_State_Connecting 1
#define MV_OWBBrowser_State_Loading    2
#define MV_OWBBrowser_State_Error      3

#define MV_OWBBrowser_Zone_Local      0
#define MV_OWBBrowser_Zone_Internet   1

#define MV_OWBBrowser_Security_None      0
#define MV_OWBBrowser_Security_Secure    1
#define MV_OWBBrowser_Security_NotSecure 2

/* Window */
#define MV_OWBWindow_FullScreen_Off      0
#define MV_OWBWindow_FullScreen_On       1
#define MV_OWBWindow_FullScreen_Toggle   2

/* Window types */
#define MV_OWB_Window_Browser         0
#define MV_OWB_Window_Settings        1
#define MV_OWB_Window_Downloads       2
#define MV_OWB_Window_Bookmarks       3
#define MV_OWB_Window_Network         4
#define MV_OWB_Window_Auth            5
#define MV_OWB_Window_Console         6
#define MV_OWB_Window_History         7
#define MV_OWB_Window_PasswordManager 8
#define MV_OWB_Window_CookieManager   9
#define MV_OWB_Window_BlockManager   10
#define MV_OWB_Window_SearchManager  11
#define MV_OWB_Window_ScriptManager  12
#define MV_OWB_Window_URLPrefs       13
#define MV_OWB_Window_Printer        14

/* Object types */
#define MV_OWB_ObjectType_Browser   0
#define MV_OWB_ObjectType_QuickLink 1
#define MV_OWB_ObjectType_Bookmark  2
#define MV_OWB_ObjectType_Tab       3
#define MV_OWB_ObjectType_URL       4

/* DownloadList */
#define MV_DownloadList_Type_InProgress 0
#define MV_DownloadList_Type_Finished   1
#define MV_DownloadList_Type_Failed     2

/* Bookmark */
#define MV_Bookmark_SaveHtml_IBrowse 1
#define MV_Bookmark_SaveHtml_OWB     2
#define MV_Bookmark_ImportHtml_OK        1
#define MV_Bookmark_ImportHtml_NoFile    0
#define MV_Bookmark_ImportHtml_WrongType 2

#define MV_BookmarkGroup_Update_Tree      1
#define MV_BookmarkGroup_Update_QuickLink 2
#define MV_BookmarkGroup_Update_All       3

#define MV_QuickLinkGroup_Mode_Button 1
#define MV_QuickLinkGroup_Mode_Prop   2
#define MV_QuickLinkGroup_Mode_Horiz  0
#define MV_QuickLinkGroup_Mode_Vert   4
#define MV_QuickLinkGroup_Mode_Col    8

#define MV_QuickLinkGroup_Change_Add 1
#define MV_QuickLinkGroup_Change_Remove 2
#define MV_QuickLinkGroup_Change_Redraw 4

/* FindText */
#define MV_FindText_CaseSensitive  (1 << 0)
#define MV_FindText_Previous       (1 << 1)
#define MV_FindText_ShowAllMatches (1 << 2)
#define MV_FindText_MarkOnly       (1 << 3)

/* History Button */
#define MV_HistoryButton_Type_Backward 0
#define MV_HistoryButton_Type_Forward  1

/* Picture Button */
#define MV_ToolButton_Type_Icon        0
#define MV_ToolButton_Type_Text        1
#define MV_ToolButton_Type_IconAndText 2

#define MV_ToolButton_Frame_None       0
#define MV_ToolButton_Frame_Button     1

#define MV_ToolButton_Background_Parent 0
#define MV_ToolButton_Background_Button 1

/* Favicon */
#define    MV_FavIcon_Scale_Normal 0
#define MV_FavIcon_Scale_Double 1

/* Icon */
#define    MV_Icon_Scale_Normal 0
#define MV_Icon_Scale_Double 1

/* Script Manager */
#define    MV_ScriptManagerGroup_Update_Script    1
#define MV_ScriptManagerGroup_Update_WhiteList 2
#define MV_ScriptManagerGroup_Update_BlackList 4

/* AutofillPopup */
#define MV_AutofillPopup_HandleNavigationEvent_Down     1
#define MV_AutofillPopup_HandleNavigationEvent_Up       2
#define MV_AutofillPopup_HandleNavigationEvent_Accept   3
#define MV_AutofillPopup_HandleNavigationEvent_Close    4

#endif




