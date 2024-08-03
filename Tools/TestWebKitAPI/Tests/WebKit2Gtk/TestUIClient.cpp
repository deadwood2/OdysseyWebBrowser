/*
 * Copyright (C) 2011 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebViewTest.h"
#include <wtf/HashSet.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/text/StringHash.h>

static const char* kAlertDialogMessage = "WebKitGTK+ alert dialog message";
static const char* kConfirmDialogMessage = "WebKitGTK+ confirm dialog message";
static const char* kPromptDialogMessage = "WebKitGTK+ prompt dialog message";
static const char* kPromptDialogReturnedText = "WebKitGTK+ prompt dialog returned text";
static const char* kBeforeUnloadConfirmDialogMessage = "WebKitGTK+ beforeunload dialog message";

class UIClientTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(UIClientTest);

    enum WebViewEvents {
        Create,
        ReadyToShow,
        RunAsModal,
        Close
    };

    class WindowProperties {
    public:
        WindowProperties()
            : m_isNull(true)
            , m_toolbarVisible(true)
            , m_statusbarVisible(true)
            , m_scrollbarsVisible(true)
            , m_menubarVisible(true)
            , m_locationbarVisible(true)
            , m_resizable(true)
            , m_fullscreen(false)
        {
            memset(&m_geometry, 0, sizeof(GdkRectangle));
        }

        WindowProperties(WebKitWindowProperties* windowProperties)
            : m_isNull(false)
            , m_toolbarVisible(webkit_window_properties_get_toolbar_visible(windowProperties))
            , m_statusbarVisible(webkit_window_properties_get_statusbar_visible(windowProperties))
            , m_scrollbarsVisible(webkit_window_properties_get_scrollbars_visible(windowProperties))
            , m_menubarVisible(webkit_window_properties_get_menubar_visible(windowProperties))
            , m_locationbarVisible(webkit_window_properties_get_locationbar_visible(windowProperties))
            , m_resizable(webkit_window_properties_get_resizable(windowProperties))
            , m_fullscreen(webkit_window_properties_get_fullscreen(windowProperties))
        {
            webkit_window_properties_get_geometry(windowProperties, &m_geometry);
        }

        WindowProperties(GdkRectangle* geometry, bool toolbarVisible, bool statusbarVisible, bool scrollbarsVisible, bool menubarVisible, bool locationbarVisible, bool resizable, bool fullscreen)
            : m_isNull(false)
            , m_geometry(*geometry)
            , m_toolbarVisible(toolbarVisible)
            , m_statusbarVisible(statusbarVisible)
            , m_scrollbarsVisible(scrollbarsVisible)
            , m_menubarVisible(menubarVisible)
            , m_locationbarVisible(locationbarVisible)
            , m_resizable(resizable)
            , m_fullscreen(fullscreen)
        {
        }

        bool isNull() const { return m_isNull; }

        void assertEqual(const WindowProperties& other) const
        {
            g_assert_cmpint(m_geometry.x, ==, other.m_geometry.x);
            g_assert_cmpint(m_geometry.y, ==, other.m_geometry.y);
            g_assert_cmpint(m_geometry.width, ==, other.m_geometry.width);
            g_assert_cmpint(m_geometry.height, ==, other.m_geometry.height);
            g_assert_cmpint(static_cast<int>(m_toolbarVisible), ==, static_cast<int>(other.m_toolbarVisible));
            g_assert_cmpint(static_cast<int>(m_statusbarVisible), ==, static_cast<int>(other.m_statusbarVisible));
            g_assert_cmpint(static_cast<int>(m_scrollbarsVisible), ==, static_cast<int>(other.m_scrollbarsVisible));
            g_assert_cmpint(static_cast<int>(m_menubarVisible), ==, static_cast<int>(other.m_menubarVisible));
            g_assert_cmpint(static_cast<int>(m_locationbarVisible), ==, static_cast<int>(other.m_locationbarVisible));
            g_assert_cmpint(static_cast<int>(m_resizable), ==, static_cast<int>(other.m_resizable));
            g_assert_cmpint(static_cast<int>(m_fullscreen), ==, static_cast<int>(other.m_fullscreen));
        }

    private:
        bool m_isNull;

        GdkRectangle m_geometry;

        bool m_toolbarVisible;
        bool m_statusbarVisible;
        bool m_scrollbarsVisible;
        bool m_menubarVisible;
        bool m_locationbarVisible;

        bool m_resizable;
        bool m_fullscreen;
    };

    static void windowPropertiesNotifyCallback(GObject*, GParamSpec* paramSpec, UIClientTest* test)
    {
        test->m_windowPropertiesChanged.add(g_param_spec_get_name(paramSpec));
    }

    static GtkWidget* viewCreateCallback(WebKitWebView* webView, WebKitNavigationAction* navigation, UIClientTest* test)
    {
        return test->viewCreate(webView, navigation);
    }

    static void viewReadyToShowCallback(WebKitWebView* webView, UIClientTest* test)
    {
        test->viewReadyToShow(webView);
    }

    static void viewCloseCallback(WebKitWebView* webView, UIClientTest* test)
    {
        test->viewClose(webView);
    }

    void scriptAlert(WebKitScriptDialog* dialog)
    {
        switch (m_scriptDialogType) {
        case WEBKIT_SCRIPT_DIALOG_ALERT:
            g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kAlertDialogMessage);
            break;
        case WEBKIT_SCRIPT_DIALOG_CONFIRM:
            g_assert(m_scriptDialogConfirmed);
            g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, "confirmed");

            break;
        case WEBKIT_SCRIPT_DIALOG_PROMPT:
            g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kPromptDialogReturnedText);
            break;
        case WEBKIT_SCRIPT_DIALOG_BEFORE_UNLOAD_CONFIRM:
            g_assert_not_reached();
            break;
        }

        g_main_loop_quit(m_mainLoop);
    }

    void scriptConfirm(WebKitScriptDialog* dialog)
    {
        g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kConfirmDialogMessage);
        m_scriptDialogConfirmed = !m_scriptDialogConfirmed;
        webkit_script_dialog_confirm_set_confirmed(dialog, m_scriptDialogConfirmed);
    }

    void scriptPrompt(WebKitScriptDialog* dialog)
    {
        g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kPromptDialogMessage);
        g_assert_cmpstr(webkit_script_dialog_prompt_get_default_text(dialog), ==, "default");
        webkit_script_dialog_prompt_set_text(dialog, kPromptDialogReturnedText);
    }

    void scriptBeforeUnloadConfirm(WebKitScriptDialog* dialog)
    {
        g_assert_cmpstr(webkit_script_dialog_get_message(dialog), ==, kBeforeUnloadConfirmDialogMessage);
        m_scriptDialogConfirmed = true;
        webkit_script_dialog_confirm_set_confirmed(dialog, m_scriptDialogConfirmed);
    }

    static gboolean scriptDialog(WebKitWebView*, WebKitScriptDialog* dialog, UIClientTest* test)
    {
        switch (webkit_script_dialog_get_dialog_type(dialog)) {
        case WEBKIT_SCRIPT_DIALOG_ALERT:
            test->scriptAlert(dialog);
            break;
        case WEBKIT_SCRIPT_DIALOG_CONFIRM:
            test->scriptConfirm(dialog);
            break;
        case WEBKIT_SCRIPT_DIALOG_PROMPT:
            test->scriptPrompt(dialog);
            break;
        case WEBKIT_SCRIPT_DIALOG_BEFORE_UNLOAD_CONFIRM:
            test->scriptBeforeUnloadConfirm(dialog);
            break;
        }

        return TRUE;
    }

    static void mouseTargetChanged(WebKitWebView*, WebKitHitTestResult* hitTestResult, guint modifiers, UIClientTest* test)
    {
        g_assert(WEBKIT_IS_HIT_TEST_RESULT(hitTestResult));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(hitTestResult));

        test->m_mouseTargetHitTestResult = hitTestResult;
        test->m_mouseTargetModifiers = modifiers;
        g_main_loop_quit(test->m_mainLoop);
    }

    static gboolean permissionRequested(WebKitWebView*, WebKitPermissionRequest* request, UIClientTest* test)
    {
        g_assert(WEBKIT_IS_PERMISSION_REQUEST(request));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));

        if (test->m_verifyMediaTypes && WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(request)) {
            WebKitUserMediaPermissionRequest* userMediaRequest = WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request);
            g_assert(webkit_user_media_permission_is_for_audio_device(userMediaRequest) == test->m_expectedAudioMedia);
            g_assert(webkit_user_media_permission_is_for_video_device(userMediaRequest) == test->m_expectedVideoMedia);
        }

        if (test->m_allowPermissionRequests)
            webkit_permission_request_allow(request);
        else
            webkit_permission_request_deny(request);

        return TRUE;
    }

    static void permissionResultMessageReceivedCallback(WebKitUserContentManager* userContentManager, WebKitJavascriptResult* javascriptResult, UIClientTest* test)
    {
        test->m_permissionResult.reset(WebViewTest::javascriptResultToCString(javascriptResult));
        g_main_loop_quit(test->m_mainLoop);
    }

    UIClientTest()
        : m_scriptDialogType(WEBKIT_SCRIPT_DIALOG_ALERT)
        , m_scriptDialogConfirmed(true)
        , m_allowPermissionRequests(false)
        , m_verifyMediaTypes(false)
        , m_expectedAudioMedia(false)
        , m_expectedVideoMedia(false)
        , m_mouseTargetModifiers(0)
    {
        webkit_settings_set_javascript_can_open_windows_automatically(webkit_web_view_get_settings(m_webView), TRUE);
        g_signal_connect(m_webView, "create", G_CALLBACK(viewCreateCallback), this);
        g_signal_connect(m_webView, "script-dialog", G_CALLBACK(scriptDialog), this);
        g_signal_connect(m_webView, "mouse-target-changed", G_CALLBACK(mouseTargetChanged), this);
        g_signal_connect(m_webView, "permission-request", G_CALLBACK(permissionRequested), this);
        webkit_user_content_manager_register_script_message_handler(m_userContentManager.get(), "permission");
        g_signal_connect(m_userContentManager.get(), "script-message-received::permission", G_CALLBACK(permissionResultMessageReceivedCallback), this);
    }

    ~UIClientTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        g_signal_handlers_disconnect_matched(m_userContentManager.get(), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        webkit_user_content_manager_unregister_script_message_handler(m_userContentManager.get(), "permission");
    }

    static void tryWebViewCloseCallback(UIClientTest* test)
    {
        g_main_loop_quit(test->m_mainLoop);
    }

    void tryCloseAndWaitUntilClosed()
    {
        gulong handler = g_signal_connect_swapped(m_webView, "close", G_CALLBACK(tryWebViewCloseCallback), this);
        // Use an idle because webkit_web_view_try_close can emit the close signal in the
        // current run loop iteration.
        g_idle_add([](gpointer data) -> gboolean {
            webkit_web_view_try_close(WEBKIT_WEB_VIEW(data));
            return G_SOURCE_REMOVE;
        }, m_webView);
        g_main_loop_run(m_mainLoop);
        g_signal_handler_disconnect(m_webView, handler);
    }

    void waitUntilMainLoopFinishes()
    {
        g_main_loop_run(m_mainLoop);
    }

    const char* waitUntilPermissionResultMessageReceived()
    {
        m_permissionResult = nullptr;
        g_main_loop_run(m_mainLoop);
        return m_permissionResult.get();
    }

    void setExpectedWindowProperties(const WindowProperties& windowProperties)
    {
        m_windowProperties = windowProperties;
    }

    WebKitHitTestResult* moveMouseAndWaitUntilMouseTargetChanged(int x, int y, unsigned mouseModifiers = 0)
    {
        mouseMoveTo(x, y, mouseModifiers);
        g_main_loop_run(m_mainLoop);
        return m_mouseTargetHitTestResult.get();
    }

    virtual GtkWidget* viewCreate(WebKitWebView* webView, WebKitNavigationAction* navigation)
    {
        g_assert(webView == m_webView);
        g_assert(navigation);

        GtkWidget* newWebView = webkit_web_view_new_with_context(webkit_web_view_get_context(webView));
        g_object_ref_sink(newWebView);

        m_webViewEvents.append(Create);

        WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(newWebView));
        g_assert(windowProperties);
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(windowProperties));
        m_windowPropertiesChanged.clear();

        g_signal_connect(windowProperties, "notify", G_CALLBACK(windowPropertiesNotifyCallback), this);
        g_signal_connect(newWebView, "ready-to-show", G_CALLBACK(viewReadyToShowCallback), this);
        g_signal_connect(newWebView, "close", G_CALLBACK(viewCloseCallback), this);

        return newWebView;
    }

    virtual void viewReadyToShow(WebKitWebView* webView)
    {
        g_assert(webView != m_webView);

        WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(webView);
        g_assert(windowProperties);
        if (!m_windowProperties.isNull())
            WindowProperties(windowProperties).assertEqual(m_windowProperties);

        m_webViewEvents.append(ReadyToShow);
    }

    virtual void viewClose(WebKitWebView* webView)
    {
        g_assert(webView != m_webView);

        m_webViewEvents.append(Close);
        g_object_unref(webView);

        g_main_loop_quit(m_mainLoop);
    }

    Vector<WebViewEvents> m_webViewEvents;
    WebKitScriptDialogType m_scriptDialogType;
    bool m_scriptDialogConfirmed;
    bool m_allowPermissionRequests;
    gboolean m_verifyMediaTypes;
    gboolean m_expectedAudioMedia;
    gboolean m_expectedVideoMedia;
    WindowProperties m_windowProperties;
    HashSet<WTF::String> m_windowPropertiesChanged;
    GRefPtr<WebKitHitTestResult> m_mouseTargetHitTestResult;
    unsigned m_mouseTargetModifiers;
    GUniquePtr<char> m_permissionResult;
};

static void testWebViewCreateReadyClose(UIClientTest* test, gconstpointer)
{
    test->loadHtml("<html><body onLoad=\"window.open().close();\"></html>", 0);
    test->waitUntilMainLoopFinishes();

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 3);
    g_assert_cmpint(events[0], ==, UIClientTest::Create);
    g_assert_cmpint(events[1], ==, UIClientTest::ReadyToShow);
    g_assert_cmpint(events[2], ==, UIClientTest::Close);
}

class CreateNavigationDataTest: public UIClientTest {
public:
    MAKE_GLIB_TEST_FIXTURE(CreateNavigationDataTest);

    CreateNavigationDataTest()
        : m_navigation(nullptr)
    {
    }

    ~CreateNavigationDataTest()
    {
        clearNavigation();
    }

    void clearNavigation()
    {
        if (m_navigation)
            webkit_navigation_action_free(m_navigation);
        m_navigation = nullptr;
    }

    GtkWidget* viewCreate(WebKitWebView* webView, WebKitNavigationAction* navigation)
    {
        g_assert(navigation);
        g_assert(!m_navigation);
        m_navigation = webkit_navigation_action_copy(navigation);
        g_main_loop_quit(m_mainLoop);
        return nullptr;
    }

    void loadHTML(const char* html)
    {
        clearNavigation();
        WebViewTest::loadHtml(html, nullptr);
    }

    void clickAndWaitUntilMainLoopFinishes(int x, int y)
    {
        clearNavigation();
        clickMouseButton(x, y, 1);
        g_main_loop_run(m_mainLoop);
    }

    WebKitNavigationAction* m_navigation;
};

static void testWebViewCreateNavigationData(CreateNavigationDataTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    test->loadHTML(
        "<html><body>"
        "<input style=\"position:absolute; left:0; top:0; margin:0; padding:0\" type=\"button\" value=\"click to show a popup\" onclick=\"window.open('data:foo');\"/>"
        "<a style=\"position:absolute; left:20; top:20;\" href=\"data:bar\" target=\"_blank\">popup link</a>"
        "</body></html>");
    test->waitUntilLoadFinished();

    // Click on a button.
    test->clickAndWaitUntilMainLoopFinishes(5, 5);
    g_assert_cmpstr(webkit_uri_request_get_uri(webkit_navigation_action_get_request(test->m_navigation)), ==, "data:foo");
    g_assert_cmpuint(webkit_navigation_action_get_navigation_type(test->m_navigation), ==, WEBKIT_NAVIGATION_TYPE_OTHER);
    // FIXME: This should be button 1.
    g_assert_cmpuint(webkit_navigation_action_get_mouse_button(test->m_navigation), ==, 0);
    g_assert_cmpuint(webkit_navigation_action_get_modifiers(test->m_navigation), ==, 0);
    g_assert(webkit_navigation_action_is_user_gesture(test->m_navigation));

    // Click on a link.
    test->clickAndWaitUntilMainLoopFinishes(21, 21);
    g_assert_cmpstr(webkit_uri_request_get_uri(webkit_navigation_action_get_request(test->m_navigation)), ==, "data:bar");
    g_assert_cmpuint(webkit_navigation_action_get_navigation_type(test->m_navigation), ==, WEBKIT_NAVIGATION_TYPE_LINK_CLICKED);
    g_assert_cmpuint(webkit_navigation_action_get_mouse_button(test->m_navigation), ==, 1);
    g_assert_cmpuint(webkit_navigation_action_get_modifiers(test->m_navigation), ==, 0);
    g_assert(webkit_navigation_action_is_user_gesture(test->m_navigation));

    // No user interaction.
    test->loadHTML("<html><body onLoad=\"window.open();\"></html>");
    test->waitUntilMainLoopFinishes();

    g_assert_cmpstr(webkit_uri_request_get_uri(webkit_navigation_action_get_request(test->m_navigation)), ==, "");
    g_assert_cmpuint(webkit_navigation_action_get_navigation_type(test->m_navigation), ==, WEBKIT_NAVIGATION_TYPE_OTHER);
    g_assert_cmpuint(webkit_navigation_action_get_mouse_button(test->m_navigation), ==, 0);
    g_assert_cmpuint(webkit_navigation_action_get_modifiers(test->m_navigation), ==, 0);
    g_assert(!webkit_navigation_action_is_user_gesture(test->m_navigation));
}

static gboolean checkMimeTypeForFilter(GtkFileFilter* filter, const gchar* mimeType)
{
    GtkFileFilterInfo filterInfo;
    filterInfo.contains = GTK_FILE_FILTER_MIME_TYPE;
    filterInfo.mime_type = mimeType;
    return gtk_file_filter_filter(filter, &filterInfo);
}

class ModalDialogsTest: public UIClientTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ModalDialogsTest);

    static void dialogRunAsModalCallback(WebKitWebView* webView, ModalDialogsTest* test)
    {
        g_assert(webView != test->m_webView);
        test->m_webViewEvents.append(RunAsModal);
    }

    GtkWidget* viewCreate(WebKitWebView* webView, WebKitNavigationAction* navigation)
    {
        g_assert(webView == m_webView);

        GtkWidget* newWebView = UIClientTest::viewCreate(webView, navigation);
        g_signal_connect(newWebView, "run-as-modal", G_CALLBACK(dialogRunAsModalCallback), this);
        return newWebView;
    }

    void viewReadyToShow(WebKitWebView* webView)
    {
        g_assert(webView != m_webView);
        m_webViewEvents.append(ReadyToShow);
    }
};

static void testWebViewAllowModalDialogs(ModalDialogsTest* test, gconstpointer)
{
    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    webkit_settings_set_allow_modal_dialogs(settings, TRUE);

    test->loadHtml("<html><body onload=\"window.showModalDialog('data:text/html,<html><body/><script>window.close();</script></html>')\"></body></html>", 0);
    test->waitUntilMainLoopFinishes();

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, UIClientTest::Create);
    g_assert_cmpint(events[1], ==, UIClientTest::ReadyToShow);
    g_assert_cmpint(events[2], ==, UIClientTest::RunAsModal);
    g_assert_cmpint(events[3], ==, UIClientTest::Close);
}

static void testWebViewDisallowModalDialogs(ModalDialogsTest* test, gconstpointer)
{
    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    webkit_settings_set_allow_modal_dialogs(settings, FALSE);

    test->loadHtml("<html><body onload=\"window.showModalDialog('data:text/html,<html><body/><script>window.close();</script></html>')\"></body></html>", 0);
    // We need to use a timeout here because the viewClose() function
    // won't ever be called as the dialog won't be created.
    test->wait(1);

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 0);
}

static void testWebViewJavaScriptDialogs(UIClientTest* test, gconstpointer)
{
    static const char* htmlOnLoadFormat = "<html><body onLoad=\"%s\"></body></html>";
    static const char* jsAlertFormat = "alert('%s')";
    static const char* jsConfirmFormat = "do { confirmed = confirm('%s'); } while (!confirmed); alert('confirmed');";
    static const char* jsPromptFormat = "alert(prompt('%s', 'default'));";
    static const char* htmlOnBeforeUnloadFormat =
        "<html><body onbeforeunload=\"return beforeUnloadHandler();\"><script>function beforeUnloadHandler() { return \"%s\"; }</script></body></html>";

    test->m_scriptDialogType = WEBKIT_SCRIPT_DIALOG_ALERT;
    GUniquePtr<char> alertDialogMessage(g_strdup_printf(jsAlertFormat, kAlertDialogMessage));
    GUniquePtr<char> alertHTML(g_strdup_printf(htmlOnLoadFormat, alertDialogMessage.get()));
    test->loadHtml(alertHTML.get(), 0);
    test->waitUntilMainLoopFinishes();
    webkit_web_view_stop_loading(test->m_webView);
    test->waitUntilLoadFinished();

    test->m_scriptDialogType = WEBKIT_SCRIPT_DIALOG_CONFIRM;
    GUniquePtr<char> confirmDialogMessage(g_strdup_printf(jsConfirmFormat, kConfirmDialogMessage));
    GUniquePtr<char> confirmHTML(g_strdup_printf(htmlOnLoadFormat, confirmDialogMessage.get()));
    test->loadHtml(confirmHTML.get(), 0);
    test->waitUntilMainLoopFinishes();
    webkit_web_view_stop_loading(test->m_webView);
    test->waitUntilLoadFinished();

    test->m_scriptDialogType = WEBKIT_SCRIPT_DIALOG_PROMPT;
    GUniquePtr<char> promptDialogMessage(g_strdup_printf(jsPromptFormat, kPromptDialogMessage));
    GUniquePtr<char> promptHTML(g_strdup_printf(htmlOnLoadFormat, promptDialogMessage.get()));
    test->loadHtml(promptHTML.get(), 0);
    test->waitUntilMainLoopFinishes();
    webkit_web_view_stop_loading(test->m_webView);
    test->waitUntilLoadFinished();

    test->m_scriptDialogType = WEBKIT_SCRIPT_DIALOG_BEFORE_UNLOAD_CONFIRM;
    GUniquePtr<char> beforeUnloadDialogHTML(g_strdup_printf(htmlOnBeforeUnloadFormat, kBeforeUnloadConfirmDialogMessage));
    test->loadHtml(beforeUnloadDialogHTML.get(), nullptr);
    test->waitUntilLoadFinished();

    // Reload should trigger onbeforeunload.
#if 0
    // FIXME: reloading HTML data doesn't emit finished load event.
    // See https://bugs.webkit.org/show_bug.cgi?id=139089.
    test->m_scriptDialogConfirmed = false;
    webkit_web_view_reload(test->m_webView);
    test->waitUntilLoadFinished();
    g_assert(test->m_scriptDialogConfirmed);
#endif

    // Navigation should trigger onbeforeunload.
    test->m_scriptDialogConfirmed = false;
    test->loadHtml("<html></html>", nullptr);
    test->waitUntilLoadFinished();
    g_assert(test->m_scriptDialogConfirmed);

    // Try close should trigger onbeforeunload.
    test->m_scriptDialogConfirmed = false;
    test->loadHtml(beforeUnloadDialogHTML.get(), nullptr);
    test->waitUntilLoadFinished();
    test->tryCloseAndWaitUntilClosed();
    g_assert(test->m_scriptDialogConfirmed);

    // Try close on a page with no unload handlers should not trigger onbeforeunload,
    // but should actually close the page.
    test->m_scriptDialogConfirmed = false;
    test->loadHtml("<html><body></body></html>", nullptr);
    test->waitUntilLoadFinished();
    // We got a onbeforeunload of the previous page.
    g_assert(test->m_scriptDialogConfirmed);
    test->m_scriptDialogConfirmed = false;
    test->tryCloseAndWaitUntilClosed();
    g_assert(!test->m_scriptDialogConfirmed);
}

static void testWebViewWindowProperties(UIClientTest* test, gconstpointer)
{
    static const char* windowProrpertiesString = "left=100,top=150,width=400,height=400,location=no,menubar=no,status=no,toolbar=no,scrollbars=no";
    GdkRectangle geometry = { 100, 150, 400, 400 };
    test->setExpectedWindowProperties(UIClientTest::WindowProperties(&geometry, false, false, false, false, false, true, false));

    GUniquePtr<char> htmlString(g_strdup_printf("<html><body onLoad=\"window.open('', '', '%s').close();\"></body></html>", windowProrpertiesString));
    test->loadHtml(htmlString.get(), 0);
    test->waitUntilMainLoopFinishes();

    static const char* propertiesChanged[] = {
        "geometry", "locationbar-visible", "menubar-visible", "statusbar-visible", "toolbar-visible", "scrollbars-visible"
    };
    for (size_t i = 0; i < G_N_ELEMENTS(propertiesChanged); ++i)
        g_assert(test->m_windowPropertiesChanged.contains(propertiesChanged[i]));

    Vector<UIClientTest::WebViewEvents>& events = test->m_webViewEvents;
    g_assert_cmpint(events.size(), ==, 3);
    g_assert_cmpint(events[0], ==, UIClientTest::Create);
    g_assert_cmpint(events[1], ==, UIClientTest::ReadyToShow);
    g_assert_cmpint(events[2], ==, UIClientTest::Close);
}

static void testWebViewMouseTarget(UIClientTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);

    const char* linksHoveredHTML =
        "<html><head>"
        " <script>"
        "    window.onload = function () {"
        "      window.getSelection().removeAllRanges();"
        "      var select_range = document.createRange();"
        "      select_range.selectNodeContents(document.getElementById('text_to_select'));"
        "      window.getSelection().addRange(select_range);"
        "    }"
        " </script>"
        "</head><body>"
        " <a style='position:absolute; left:1; top:1' href='http://www.webkitgtk.org' title='WebKitGTK+ Title'>WebKitGTK+ Website</a>"
        " <img style='position:absolute; left:1; top:10' src='0xdeadbeef' width=5 height=5></img>"
        " <a style='position:absolute; left:1; top:20' href='http://www.webkitgtk.org/logo' title='WebKitGTK+ Logo'><img src='0xdeadbeef' width=5 height=5></img></a>"
        " <input style='position:absolute; left:1; top:30' size='10'></input>"
        " <video style='position:absolute; left:1; top:100' width='300' height='300' controls='controls' preload='none'><source src='movie.ogg' type='video/ogg' /></video>"
        " <p style='position:absolute; left:1; top:120' id='text_to_select'>Lorem ipsum.</p>"
        "</body></html>";

    test->loadHtml(linksHoveredHTML, "file:///");
    test->waitUntilLoadFinished();

    // Move over link.
    WebKitHitTestResult* hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 1);
    g_assert(webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_link_uri(hitTestResult), ==, "http://www.webkitgtk.org/");
    g_assert_cmpstr(webkit_hit_test_result_get_link_title(hitTestResult), ==, "WebKitGTK+ Title");
    g_assert_cmpstr(webkit_hit_test_result_get_link_label(hitTestResult), ==, "WebKitGTK+ Website");
    g_assert(!test->m_mouseTargetModifiers);

    // Move out of the link.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(0, 0);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

    // Move over image with GDK_CONTROL_MASK.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 10, GDK_CONTROL_MASK);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_image_uri(hitTestResult), ==, "file:///0xdeadbeef");
    g_assert(test->m_mouseTargetModifiers & GDK_CONTROL_MASK);

    // Move over image link.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 20);
    g_assert(webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_link_uri(hitTestResult), ==, "http://www.webkitgtk.org/logo");
    g_assert_cmpstr(webkit_hit_test_result_get_image_uri(hitTestResult), ==, "file:///0xdeadbeef");
    g_assert_cmpstr(webkit_hit_test_result_get_link_title(hitTestResult), ==, "WebKitGTK+ Logo");
    g_assert(!webkit_hit_test_result_get_link_label(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

    // Move over media.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(1, 100);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert_cmpstr(webkit_hit_test_result_get_media_uri(hitTestResult), ==, "file:///movie.ogg");
    g_assert(!test->m_mouseTargetModifiers);

    // Mover over input.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(5, 35);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

    // Move over scrollbar.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(gtk_widget_get_allocated_width(GTK_WIDGET(test->m_webView)) - 4, 5);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

    // Move over selection.
    hitTestResult = test->moveMouseAndWaitUntilMouseTargetChanged(2, 145);
    g_assert(!webkit_hit_test_result_context_is_link(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_image(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_media(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_editable(hitTestResult));
    g_assert(!webkit_hit_test_result_context_is_scrollbar(hitTestResult));
    g_assert(webkit_hit_test_result_context_is_selection(hitTestResult));
    g_assert(!test->m_mouseTargetModifiers);

}

static void testWebViewGeolocationPermissionRequests(UIClientTest* test, gconstpointer)
{
    // Some versions of geoclue give a runtime warning because it tries
    // to register the error quark twice. See https://bugs.webkit.org/show_bug.cgi?id=89858.
    // Make warnings non-fatal for this test to make it pass.
    test->removeLogFatalFlag(G_LOG_LEVEL_WARNING);
    test->showInWindowAndWaitUntilMapped();
    static const char* geolocationRequestHTML =
        "<html>"
        "  <script>"
        "  function runTest()"
        "  {"
        "    navigator.geolocation.getCurrentPosition(function(p) { window.webkit.messageHandlers.permission.postMessage('OK'); },"
        "                                             function(e) { window.webkit.messageHandlers.permission.postMessage(e.code.toString()); });"
        "  }"
        "  </script>"
        "  <body onload='runTest();'></body>"
        "</html>";

    // Geolocation is not allowed from insecure connections like HTTP,
    // POSITION_UNAVAILABLE ('2') is returned in that case without even
    // asking the API layer.
    test->m_allowPermissionRequests = false;
    test->loadHtml(geolocationRequestHTML, "http://foo.com/bar");
    const gchar* result = test->waitUntilPermissionResultMessageReceived();
    g_assert_cmpstr(result, ==, "2");

    // Test denying a permission request. PERMISSION_DENIED ('1') is
    // returned in this case.
    test->m_allowPermissionRequests = false;
    test->loadHtml(geolocationRequestHTML, "https://foo.com/bar");
    result = test->waitUntilPermissionResultMessageReceived();
    g_assert_cmpstr(result, ==, "1");

    // Test allowing a permission request. Result should be different
    // to PERMISSION_DENIED ('1').
    test->m_allowPermissionRequests = true;
    test->loadHtml(geolocationRequestHTML, "https://foo.com/bar");
    result = test->waitUntilPermissionResultMessageReceived();
    g_assert_cmpstr(result, !=, "1");
    test->addLogFatalFlag(G_LOG_LEVEL_WARNING);
}

#if ENABLE(MEDIA_STREAM)
static void testWebViewUserMediaPermissionRequests(UIClientTest* test, gconstpointer)
{
    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    gboolean enabled = webkit_settings_get_enable_media_stream(settings);
    webkit_settings_set_enable_media_stream(settings, TRUE);

    test->showInWindowAndWaitUntilMapped();
    static const char* userMediaRequestHTML =
        "<html>"
        "  <script>"
        "  function runTest()"
        "  {"
        "    navigator.webkitGetUserMedia({audio: true, video: true},"
        "                                 function(s) { document.title = \"OK\" },"
        "                                 function(e) { document.title = e.name });"
        "  }"
        "  </script>"
        "  <body onload='runTest();'></body>"
        "</html>";

    test->m_verifyMediaTypes = TRUE;
    test->m_expectedAudioMedia = TRUE;
    test->m_expectedVideoMedia = TRUE;

    // Test denying a permission request.
    test->m_allowPermissionRequests = false;
    test->loadHtml(userMediaRequestHTML, nullptr);
    test->waitUntilTitleChangedTo("PermissionDeniedError");

    // Test allowing a permission request.
    test->m_allowPermissionRequests = true;
    test->loadHtml(userMediaRequestHTML, nullptr);
    test->waitUntilTitleChangedTo("OK");

    webkit_settings_set_enable_media_stream(settings, enabled);
}

static void testWebViewAudioOnlyUserMediaPermissionRequests(UIClientTest* test, gconstpointer)
{
    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    gboolean enabled = webkit_settings_get_enable_media_stream(settings);
    webkit_settings_set_enable_media_stream(settings, TRUE);

    test->showInWindowAndWaitUntilMapped();
    static const char* userMediaRequestHTML =
        "<html>"
        "  <script>"
        "  function runTest()"
        "  {"
        "    navigator.webkitGetUserMedia({audio: true, video: false},"
        "                                 function(s) { document.title = \"OK\" },"
        "                                 function(e) { document.title = e.name });"
        "  }"
        "  </script>"
        "  <body onload='runTest();'></body>"
        "</html>";

    test->m_verifyMediaTypes = TRUE;
    test->m_expectedAudioMedia = TRUE;
    test->m_expectedVideoMedia = FALSE;

    // Test denying a permission request.
    test->m_allowPermissionRequests = false;
    test->loadHtml(userMediaRequestHTML, nullptr);
    test->waitUntilTitleChangedTo("PermissionDeniedError");

    webkit_settings_set_enable_media_stream(settings, enabled);
}
#endif // ENABLE(MEDIA_STREAM)

class FileChooserTest: public UIClientTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FileChooserTest);

    FileChooserTest()
    {
        g_signal_connect(m_webView, "run-file-chooser", G_CALLBACK(runFileChooserCallback), this);
    }

    static gboolean runFileChooserCallback(WebKitWebView*, WebKitFileChooserRequest* request, FileChooserTest* test)
    {
        test->runFileChooser(request);
        return TRUE;
    }

    void runFileChooser(WebKitFileChooserRequest* request)
    {
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));
        m_fileChooserRequest = request;
        g_main_loop_quit(m_mainLoop);
    }

    WebKitFileChooserRequest* clickMouseButtonAndWaitForFileChooserRequest(int x, int y)
    {
        clickMouseButton(x, y);
        g_main_loop_run(m_mainLoop);
        return m_fileChooserRequest.get();
    }

private:
    GRefPtr<WebKitFileChooserRequest> m_fileChooserRequest;
};

static void testWebViewFileChooserRequest(FileChooserTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();
    static const char* fileChooserHTMLFormat = "<html><body><input style='position:absolute;left:0;top:0;margin:0;padding:0' type='file' %s/></body></html>";

    // Multiple selections not allowed, no MIME filtering.
    GUniquePtr<char> simpleFileUploadHTML(g_strdup_printf(fileChooserHTMLFormat, ""));
    test->loadHtml(simpleFileUploadHTML.get(), 0);
    test->waitUntilLoadFinished();
    WebKitFileChooserRequest* fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    g_assert(!webkit_file_chooser_request_get_select_multiple(fileChooserRequest));

    const gchar* const* mimeTypes = webkit_file_chooser_request_get_mime_types(fileChooserRequest);
    g_assert(!mimeTypes);
    GtkFileFilter* filter = webkit_file_chooser_request_get_mime_types_filter(fileChooserRequest);
    g_assert(!filter);
    const gchar* const* selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(!selectedFiles);
    webkit_file_chooser_request_cancel(fileChooserRequest);

    // Multiple selections allowed, no MIME filtering, some pre-selected files.
    GUniquePtr<char> multipleSelectionFileUploadHTML(g_strdup_printf(fileChooserHTMLFormat, "multiple"));
    test->loadHtml(multipleSelectionFileUploadHTML.get(), 0);
    test->waitUntilLoadFinished();
    fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    g_assert(webkit_file_chooser_request_get_select_multiple(fileChooserRequest));

    mimeTypes = webkit_file_chooser_request_get_mime_types(fileChooserRequest);
    g_assert(!mimeTypes);
    filter = webkit_file_chooser_request_get_mime_types_filter(fileChooserRequest);
    g_assert(!filter);
    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(!selectedFiles);

    // Select some files.
    const gchar* filesToSelect[4] = { "/foo", "/foo/bar", "/foo/bar/baz", 0 };
    webkit_file_chooser_request_select_files(fileChooserRequest, filesToSelect);

    // Check the files that have been just selected.
    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(selectedFiles);
    g_assert_cmpstr(selectedFiles[0], ==, "/foo");
    g_assert_cmpstr(selectedFiles[1], ==, "/foo/bar");
    g_assert_cmpstr(selectedFiles[2], ==, "/foo/bar/baz");
    g_assert(!selectedFiles[3]);

    // Perform another request to check if the list of files selected
    // in the previous step appears now as part of the new request.
    fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(selectedFiles);
    g_assert_cmpstr(selectedFiles[0], ==, "/foo");
    g_assert_cmpstr(selectedFiles[1], ==, "/foo/bar");
    g_assert_cmpstr(selectedFiles[2], ==, "/foo/bar/baz");
    g_assert(!selectedFiles[3]);
    webkit_file_chooser_request_cancel(fileChooserRequest);

    // Multiple selections not allowed, only accept images, audio and video files..
    GUniquePtr<char> mimeFilteredFileUploadHTML(g_strdup_printf(fileChooserHTMLFormat, "accept='audio/*,video/*,image/*'"));
    test->loadHtml(mimeFilteredFileUploadHTML.get(), 0);
    test->waitUntilLoadFinished();
    fileChooserRequest = test->clickMouseButtonAndWaitForFileChooserRequest(5, 5);
    g_assert(!webkit_file_chooser_request_get_select_multiple(fileChooserRequest));

    mimeTypes = webkit_file_chooser_request_get_mime_types(fileChooserRequest);
    g_assert(mimeTypes);
    g_assert_cmpstr(mimeTypes[0], ==, "audio/*");
    g_assert_cmpstr(mimeTypes[1], ==, "video/*");
    g_assert_cmpstr(mimeTypes[2], ==, "image/*");
    g_assert(!mimeTypes[3]);

    filter = webkit_file_chooser_request_get_mime_types_filter(fileChooserRequest);
    g_assert(GTK_IS_FILE_FILTER(filter));
    g_assert(checkMimeTypeForFilter(filter, "audio/*"));
    g_assert(checkMimeTypeForFilter(filter, "video/*"));
    g_assert(checkMimeTypeForFilter(filter, "image/*"));

    selectedFiles = webkit_file_chooser_request_get_selected_files(fileChooserRequest);
    g_assert(!selectedFiles);
    webkit_file_chooser_request_cancel(fileChooserRequest);
}

class ColorChooserTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(ColorChooserTest);

    static gboolean runColorChooserCallback(WebKitWebView*, WebKitColorChooserRequest* request, ColorChooserTest* test)
    {
        test->runColorChooser(request);
        return TRUE;
    }

    static void requestFinishedCallback(WebKitColorChooserRequest* request, ColorChooserTest* test)
    {
        g_assert(test->m_request.get() == request);
        test->m_request = nullptr;
        if (g_main_loop_is_running(test->m_mainLoop))
            g_main_loop_quit(test->m_mainLoop);
    }

    ColorChooserTest()
    {
        g_signal_connect(m_webView, "run-color-chooser", G_CALLBACK(runColorChooserCallback), this);
    }

    void runColorChooser(WebKitColorChooserRequest* request)
    {
        g_assert(WEBKIT_IS_COLOR_CHOOSER_REQUEST(request));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));
        m_request = request;
        g_signal_connect(request, "finished", G_CALLBACK(requestFinishedCallback), this);
        g_main_loop_quit(m_mainLoop);
    }

    void finishRequest()
    {
        g_assert(m_request.get());
        webkit_color_chooser_request_finish(m_request.get());
        g_assert(!m_request);
    }

    void cancelRequest()
    {
        g_assert(m_request.get());
        webkit_color_chooser_request_cancel(m_request.get());
        g_assert(!m_request);
    }

    WebKitColorChooserRequest* clickMouseButtonAndWaitForColorChooserRequest(int x, int y)
    {
        clickMouseButton(x, y);
        g_main_loop_run(m_mainLoop);
        g_assert(m_request.get());
        return m_request.get();
    }

private:
    GRefPtr<WebKitColorChooserRequest> m_request;
};

static void testWebViewColorChooserRequest(ColorChooserTest* test, gconstpointer)
{
    static const char* colorChooserHTMLFormat = "<html><body><input style='position:absolute;left:1;top:1;margin:0;padding:0;width:45;height:25' type='color' %s/></body></html>";
    test->showInWindowAndWaitUntilMapped();

    GUniquePtr<char> defaultColorHTML(g_strdup_printf(colorChooserHTMLFormat, ""));
    test->loadHtml(defaultColorHTML.get(), nullptr);
    test->waitUntilLoadFinished();
    WebKitColorChooserRequest* request = test->clickMouseButtonAndWaitForColorChooserRequest(5, 5);

    // Default color is black (#000000).
    GdkRGBA rgba1;
    GdkRGBA rgba2 = { 0., 0., 0., 1. };
    webkit_color_chooser_request_get_rgba(request, &rgba1);
    g_assert(gdk_rgba_equal(&rgba1, &rgba2));

    // Set a different color.
    rgba2.green = 1;
    webkit_color_chooser_request_set_rgba(request, &rgba2);
    webkit_color_chooser_request_get_rgba(request, &rgba1);
    g_assert(gdk_rgba_equal(&rgba1, &rgba2));

    GdkRectangle rect;
    webkit_color_chooser_request_get_element_rectangle(request, &rect);
    g_assert_cmpint(rect.x, == , 1);
    g_assert_cmpint(rect.y, == , 1);
    g_assert_cmpint(rect.width, == , 45);
    g_assert_cmpint(rect.height, == , 25);

    test->finishRequest();

    // Use an initial color.
    GUniquePtr<char> initialColorHTML(g_strdup_printf(colorChooserHTMLFormat, "value='#FF00FF'"));
    test->loadHtml(initialColorHTML.get(), nullptr);
    test->waitUntilLoadFinished();
    request = test->clickMouseButtonAndWaitForColorChooserRequest(5, 5);

    webkit_color_chooser_request_get_rgba(request, &rgba1);
    GdkRGBA rgba3 = { 1., 0., 1., 1. };
    g_assert(gdk_rgba_equal(&rgba1, &rgba3));

    test->cancelRequest();
}

void beforeAll()
{
    UIClientTest::add("WebKitWebView", "create-ready-close", testWebViewCreateReadyClose);
    CreateNavigationDataTest::add("WebKitWebView", "create-navigation-data", testWebViewCreateNavigationData);
    ModalDialogsTest::add("WebKitWebView", "allow-modal-dialogs", testWebViewAllowModalDialogs);
    ModalDialogsTest::add("WebKitWebView", "disallow-modal-dialogs", testWebViewDisallowModalDialogs);
    UIClientTest::add("WebKitWebView", "javascript-dialogs", testWebViewJavaScriptDialogs);
    UIClientTest::add("WebKitWebView", "window-properties", testWebViewWindowProperties);
    UIClientTest::add("WebKitWebView", "mouse-target", testWebViewMouseTarget);
    UIClientTest::add("WebKitWebView", "geolocation-permission-requests", testWebViewGeolocationPermissionRequests);
#if ENABLE(MEDIA_STREAM)
    UIClientTest::add("WebKitWebView", "usermedia-permission-requests", testWebViewUserMediaPermissionRequests);
    UIClientTest::add("WebKitWebView", "audio-usermedia-permission-request", testWebViewAudioOnlyUserMediaPermissionRequests);
#endif
    FileChooserTest::add("WebKitWebView", "file-chooser-request", testWebViewFileChooserRequest);
    ColorChooserTest::add("WebKitWebView", "color-chooser-request", testWebViewColorChooserRequest);
}

void afterAll()
{
}
