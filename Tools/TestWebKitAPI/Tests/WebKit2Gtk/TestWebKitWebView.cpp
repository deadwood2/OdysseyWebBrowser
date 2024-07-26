/*
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2014 Collabora Ltd.
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
#include "WebKitTestServer.h"
#include "WebViewTest.h"
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/JSValueRef.h>
#include <glib/gstdio.h>
#include <wtf/glib/GRefPtr.h>

class IsPlayingAudioWebViewTest : public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(IsPlayingAudioWebViewTest);

    static void isPlayingAudioChanged(GObject*, GParamSpec*, IsPlayingAudioWebViewTest* test)
    {
        g_signal_handlers_disconnect_by_func(test->m_webView, reinterpret_cast<void*>(isPlayingAudioChanged), test);
        g_main_loop_quit(test->m_mainLoop);
    }

    void waitUntilIsPlayingAudioChanged()
    {
        g_signal_connect(m_webView, "notify::is-playing-audio", G_CALLBACK(isPlayingAudioChanged), this);
        g_main_loop_run(m_mainLoop);
    }
};

static WebKitTestServer* gServer;

static void testWebViewWebContext(WebViewTest* test, gconstpointer)
{
    g_assert(webkit_web_view_get_context(test->m_webView) == test->m_webContext.get());
    g_assert(webkit_web_context_get_default() != test->m_webContext.get());

    // Check that a web view created with g_object_new has the default context.
    GRefPtr<WebKitWebView> webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW, nullptr));
    g_assert(webkit_web_view_get_context(webView.get()) == webkit_web_context_get_default());

    // Check that a web view created with a related view has the related view context.
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new_with_related_view(test->m_webView));
    g_assert(webkit_web_view_get_context(webView.get()) == test->m_webContext.get());

    // Check that a web context given as construct parameter is ignored if a related view is also provided.
    webView = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
        "web-context", webkit_web_context_get_default(), "related-view", test->m_webView, nullptr));
    g_assert(webkit_web_view_get_context(webView.get()) == test->m_webContext.get());
}

static void testWebViewWebContextLifetime(WebViewTest* test, gconstpointer)
{
    WebKitWebContext* webContext = webkit_web_context_new();
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webContext));

    GtkWidget* webView = webkit_web_view_new_with_context(webContext);
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webView));

    g_object_ref_sink(webView);
    g_object_unref(webContext);

    // Check that the web view still has a valid context.
    WebKitWebContext* tmpContext = webkit_web_view_get_context(WEBKIT_WEB_VIEW(webView));
    g_assert_true(WEBKIT_IS_WEB_CONTEXT(tmpContext));
    g_object_unref(webView);

    WebKitWebContext* webContext2 = webkit_web_context_new();
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webContext2));

    GtkWidget* webView2 = webkit_web_view_new_with_context(webContext2);
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webView2));

    g_object_ref_sink(webView2);
    g_object_unref(webView2);

    // Check that the context is still valid.
    g_assert_true(WEBKIT_IS_WEB_CONTEXT(webContext2));
    g_object_unref(webContext2);
}

static void testWebViewCustomCharset(WebViewTest* test, gconstpointer)
{
    g_assert(!webkit_web_view_get_custom_charset(test->m_webView));
    webkit_web_view_set_custom_charset(test->m_webView, "utf8");
    g_assert_cmpstr(webkit_web_view_get_custom_charset(test->m_webView), ==, "utf8");
    // Go back to the default charset.
    webkit_web_view_set_custom_charset(test->m_webView, 0);
    g_assert(!webkit_web_view_get_custom_charset(test->m_webView));
}

static void testWebViewSettings(WebViewTest* test, gconstpointer)
{
    WebKitSettings* defaultSettings = webkit_web_view_get_settings(test->m_webView);
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(defaultSettings));
    g_assert(defaultSettings);
    g_assert(webkit_settings_get_enable_javascript(defaultSettings));

    GRefPtr<WebKitSettings> newSettings = adoptGRef(webkit_settings_new());
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(newSettings.get()));
    g_object_set(G_OBJECT(newSettings.get()), "enable-javascript", FALSE, NULL);
    webkit_web_view_set_settings(test->m_webView, newSettings.get());

    WebKitSettings* settings = webkit_web_view_get_settings(test->m_webView);
    g_assert(settings != defaultSettings);
    g_assert(!webkit_settings_get_enable_javascript(settings));

    GRefPtr<GtkWidget> webView2 = webkit_web_view_new();
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webView2.get()));
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView2.get()), settings);
    g_assert(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView2.get())) == settings);

    GRefPtr<WebKitSettings> newSettings2 = adoptGRef(webkit_settings_new());
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(newSettings2.get()));
    webkit_web_view_set_settings(WEBKIT_WEB_VIEW(webView2.get()), newSettings2.get());
    settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView2.get()));
    g_assert(settings == newSettings2.get());
    g_assert(webkit_settings_get_enable_javascript(settings));

    GRefPtr<GtkWidget> webView3 = webkit_web_view_new_with_settings(newSettings2.get());
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(webView3.get()));
    g_assert(webkit_web_view_get_settings(WEBKIT_WEB_VIEW(webView3.get())) == newSettings2.get());
}

static void testWebViewZoomLevel(WebViewTest* test, gconstpointer)
{
    g_assert_cmpfloat(webkit_web_view_get_zoom_level(test->m_webView), ==, 1);
    webkit_web_view_set_zoom_level(test->m_webView, 2.5);
    g_assert_cmpfloat(webkit_web_view_get_zoom_level(test->m_webView), ==, 2.5);

    webkit_settings_set_zoom_text_only(webkit_web_view_get_settings(test->m_webView), TRUE);
    // The zoom level shouldn't change when zoom-text-only setting changes.
    g_assert_cmpfloat(webkit_web_view_get_zoom_level(test->m_webView), ==, 2.5);
}

static void testWebViewRunJavaScript(WebViewTest* test, gconstpointer)
{
    static const char* html = "<html><body><a id='WebKitLink' href='http://www.webkitgtk.org/' title='WebKitGTK+ Title'>WebKitGTK+ Website</a></body></html>";
    test->loadHtml(html, 0);
    test->waitUntilLoadFinished();

    GUniqueOutPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.getElementById('WebKitLink').title;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    GUniquePtr<char> valueString(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "WebKitGTK+ Title");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.getElementById('WebKitLink').href;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.reset(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "http://www.webkitgtk.org/");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("window.document.getElementById('WebKitLink').textContent", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.reset(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "WebKitGTK+ Website");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = 25;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert_cmpfloat(WebViewTest::javascriptResultToNumber(javascriptResult), ==, 25);

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = 2.5;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert_cmpfloat(WebViewTest::javascriptResultToNumber(javascriptResult), ==, 2.5);

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = true", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultToBoolean(javascriptResult));

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = false", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(!WebViewTest::javascriptResultToBoolean(javascriptResult));

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("a = null", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultIsNull(javascriptResult));

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("function Foo() { a = 25; } Foo();", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultIsUndefined(javascriptResult));

    javascriptResult = test->runJavaScriptFromGResourceAndWaitUntilFinished("/org/webkit/webkit2gtk/tests/link-title.js", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.reset(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "WebKitGTK+ Title");

    javascriptResult = test->runJavaScriptFromGResourceAndWaitUntilFinished("/wrong/path/to/resource.js", &error.outPtr());
    g_assert(!javascriptResult);
    g_assert_error(error.get(), G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND);

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("foo();", &error.outPtr());
    g_assert(!javascriptResult);
    g_assert_error(error.get(), WEBKIT_JAVASCRIPT_ERROR, WEBKIT_JAVASCRIPT_ERROR_SCRIPT_FAILED);
}

class FullScreenClientTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FullScreenClientTest);

    enum FullScreenEvent {
        None,
        Enter,
        Leave
    };

    static gboolean viewEnterFullScreenCallback(WebKitWebView*, FullScreenClientTest* test)
    {
        test->m_event = Enter;
        g_main_loop_quit(test->m_mainLoop);
        return FALSE;
    }

    static gboolean viewLeaveFullScreenCallback(WebKitWebView*, FullScreenClientTest* test)
    {
        test->m_event = Leave;
        g_main_loop_quit(test->m_mainLoop);
        return FALSE;
    }

    FullScreenClientTest()
        : m_event(None)
    {
        webkit_settings_set_enable_fullscreen(webkit_web_view_get_settings(m_webView), TRUE);
        g_signal_connect(m_webView, "enter-fullscreen", G_CALLBACK(viewEnterFullScreenCallback), this);
        g_signal_connect(m_webView, "leave-fullscreen", G_CALLBACK(viewLeaveFullScreenCallback), this);
    }

    ~FullScreenClientTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void requestFullScreenAndWaitUntilEnteredFullScreen()
    {
        m_event = None;
        webkit_web_view_run_javascript(m_webView, "document.documentElement.webkitRequestFullScreen();", 0, 0, 0);
        g_main_loop_run(m_mainLoop);
    }

    static gboolean leaveFullScreenIdle(FullScreenClientTest* test)
    {
        test->keyStroke(GDK_KEY_Escape);
        return FALSE;
    }

    void leaveFullScreenAndWaitUntilLeftFullScreen()
    {
        m_event = None;
        g_idle_add(reinterpret_cast<GSourceFunc>(leaveFullScreenIdle), this);
        g_main_loop_run(m_mainLoop);
    }

    FullScreenEvent m_event;
};

static void testWebViewFullScreen(FullScreenClientTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();
    test->loadHtml("<html><body>FullScreen test</body></html>", 0);
    test->waitUntilLoadFinished();
    test->requestFullScreenAndWaitUntilEnteredFullScreen();
    g_assert_cmpint(test->m_event, ==, FullScreenClientTest::Enter);
    test->leaveFullScreenAndWaitUntilLeftFullScreen();
    g_assert_cmpint(test->m_event, ==, FullScreenClientTest::Leave);
}

static void testWebViewCanShowMIMEType(WebViewTest* test, gconstpointer)
{
    // Supported MIME types.
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "text/html"));
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "text/plain"));
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "image/jpeg"));

    // Unsupported MIME types.
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "text/vcard"));
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "application/zip"));
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "application/octet-stream"));

    // Plugins are only supported when enabled.
    webkit_web_context_set_additional_plugins_directory(webkit_web_view_get_context(test->m_webView), WEBKIT_TEST_PLUGIN_DIR);
    g_assert(webkit_web_view_can_show_mime_type(test->m_webView, "application/x-webkit-test-netscape"));
    webkit_settings_set_enable_plugins(webkit_web_view_get_settings(test->m_webView), FALSE);
    g_assert(!webkit_web_view_can_show_mime_type(test->m_webView, "application/x-webkit-test-netscape"));
}

class FormClientTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FormClientTest);

    static void submitFormCallback(WebKitWebView*, WebKitFormSubmissionRequest* request, FormClientTest* test)
    {
        test->submitForm(request);
    }

    FormClientTest()
        : m_submitPositionX(0)
        , m_submitPositionY(0)
    {
        g_signal_connect(m_webView, "submit-form", G_CALLBACK(submitFormCallback), this);
    }

    ~FormClientTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void submitForm(WebKitFormSubmissionRequest* request)
    {
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));
        m_request = request;
        webkit_form_submission_request_submit(request);
        quitMainLoop();
    }

    GHashTable* waitUntilFormSubmittedAndGetTextFields()
    {
        g_main_loop_run(m_mainLoop);
        return webkit_form_submission_request_get_text_fields(m_request.get());
    }

    static gboolean doClickIdleCallback(FormClientTest* test)
    {
        test->clickMouseButton(test->m_submitPositionX, test->m_submitPositionY, 1);
        return FALSE;
    }

    void submitFormAtPosition(int x, int y)
    {
        m_submitPositionX = x;
        m_submitPositionY = y;
        g_idle_add(reinterpret_cast<GSourceFunc>(doClickIdleCallback), this);
    }

    int m_submitPositionX;
    int m_submitPositionY;
    GRefPtr<WebKitFormSubmissionRequest> m_request;
};

static void testWebViewSubmitForm(FormClientTest* test, gconstpointer)
{
    test->showInWindowAndWaitUntilMapped();

    const char* formHTML =
        "<html><body>"
        " <form action='#'>"
        "  <input type='text' name='text1' value='value1'>"
        "  <input type='text' name='text2' value='value2'>"
        "  <input type='password' name='password' value='secret'>"
        "  <textarea cols='5' rows='5' name='textarea'>Text</textarea>"
        "  <input type='hidden' name='hidden1' value='hidden1'>"
        "  <input type='submit' value='Submit' style='position:absolute; left:1; top:1' size='10'>"
        " </form>"
        "</body></html>";

    test->loadHtml(formHTML, "file:///");
    test->waitUntilLoadFinished();

    test->submitFormAtPosition(5, 5);
    GHashTable* values = test->waitUntilFormSubmittedAndGetTextFields();
    g_assert(values);
    g_assert_cmpuint(g_hash_table_size(values), ==, 3);
    g_assert_cmpstr(static_cast<char*>(g_hash_table_lookup(values, "text1")), ==, "value1");
    g_assert_cmpstr(static_cast<char*>(g_hash_table_lookup(values, "text2")), ==, "value2");
    g_assert_cmpstr(static_cast<char*>(g_hash_table_lookup(values, "password")), ==, "secret");
}

class SaveWebViewTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(SaveWebViewTest);

    SaveWebViewTest()
        : m_tempDirectory(g_dir_make_tmp("WebKit2SaveViewTest-XXXXXX", 0))
    {
    }

    ~SaveWebViewTest()
    {
        if (G_IS_FILE(m_file.get()))
            g_file_delete(m_file.get(), 0, 0);

        if (G_IS_INPUT_STREAM(m_inputStream.get()))
            g_input_stream_close(m_inputStream.get(), 0, 0);

        if (m_tempDirectory)
            g_rmdir(m_tempDirectory.get());
    }

    static void webViewSavedToStreamCallback(GObject* object, GAsyncResult* result, SaveWebViewTest* test)
    {
        GUniqueOutPtr<GError> error;
        test->m_inputStream = adoptGRef(webkit_web_view_save_finish(test->m_webView, result, &error.outPtr()));
        g_assert(G_IS_INPUT_STREAM(test->m_inputStream.get()));
        g_assert(!error);

        test->quitMainLoop();
    }

    static void webViewSavedToFileCallback(GObject* object, GAsyncResult* result, SaveWebViewTest* test)
    {
        GUniqueOutPtr<GError> error;
        g_assert(webkit_web_view_save_to_file_finish(test->m_webView, result, &error.outPtr()));
        g_assert(!error);

        test->quitMainLoop();
    }

    void saveAndWaitForStream()
    {
        webkit_web_view_save(m_webView, WEBKIT_SAVE_MODE_MHTML, 0, reinterpret_cast<GAsyncReadyCallback>(webViewSavedToStreamCallback), this);
        g_main_loop_run(m_mainLoop);
    }

    void saveAndWaitForFile()
    {
        m_saveDestinationFilePath.reset(g_build_filename(m_tempDirectory.get(), "testWebViewSaveResult.mht", NULL));
        m_file = adoptGRef(g_file_new_for_path(m_saveDestinationFilePath.get()));
        webkit_web_view_save_to_file(m_webView, m_file.get(), WEBKIT_SAVE_MODE_MHTML, 0, reinterpret_cast<GAsyncReadyCallback>(webViewSavedToFileCallback), this);
        g_main_loop_run(m_mainLoop);
    }

    GUniquePtr<char> m_tempDirectory;
    GUniquePtr<char> m_saveDestinationFilePath;
    GRefPtr<GInputStream> m_inputStream;
    GRefPtr<GFile> m_file;
};

static void testWebViewSave(SaveWebViewTest* test, gconstpointer)
{
    test->loadHtml("<html>"
        "<body>"
        "  <p>A paragraph with plain text</p>"
        "  <p>"
        "    A red box: <img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3AYWDTMVwnSZnwAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAAAFklEQVQI12P8z8DAwMDAxMDAwMDAAAANHQEDK+mmyAAAAABJRU5ErkJggg=='></br>"
        "    A blue box: <img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3AYWDTMvBHhALQAAAB1pVFh0Q29tbWVudAAAAAAAQ3JlYXRlZCB3aXRoIEdJTVBkLmUHAAAAFklEQVQI12Nk4PnPwMDAxMDAwMDAAAALrwEPPIs1pgAAAABJRU5ErkJggg=='>"
        "  </p>"
        "</body>"
        "</html>", 0);
    test->waitUntilLoadFinished();

    // Write to a file and to an input stream.
    test->saveAndWaitForFile();
    test->saveAndWaitForStream();

    // We should have exactly the same amount of bytes in the file
    // than those coming from the GInputStream. We don't compare the
    // strings read since the 'Date' field and the boundaries will be
    // different on each case. MHTML functionality will be tested by
    // Layout tests, so checking the amount of bytes is enough.
    GUniqueOutPtr<GError> error;
    gchar buffer[512] = { 0 };
    gssize readBytes = 0;
    gssize totalBytesFromStream = 0;
    while ((readBytes = g_input_stream_read(test->m_inputStream.get(), &buffer, 512, 0, &error.outPtr()))) {
        g_assert(!error);
        totalBytesFromStream += readBytes;
    }

    // Check that the file exists and that it contains the same amount of bytes.
    GRefPtr<GFileInfo> fileInfo = adoptGRef(g_file_query_info(test->m_file.get(), G_FILE_ATTRIBUTE_STANDARD_SIZE, static_cast<GFileQueryInfoFlags>(0), 0, 0));
    g_assert_cmpint(g_file_info_get_size(fileInfo.get()), ==, totalBytesFromStream);
}

// To test page visibility API. Currently only 'visible', 'hidden' and 'prerender' states are implemented fully in WebCore.
// See also http://www.w3.org/TR/2011/WD-page-visibility-20110602/ and https://developers.google.com/chrome/whitepapers/pagevisibility
static void testWebViewPageVisibility(WebViewTest* test, gconstpointer)
{
    test->loadHtml("<html><title></title>"
        "<body><p>Test Web Page Visibility</p>"
        "<script>"
        "document.addEventListener(\"visibilitychange\", onVisibilityChange, false);"
        "function onVisibilityChange() {"
        "    document.title = document.visibilityState;"
        "}"
        "</script>"
        "</body></html>",
        0);

    // Wait until the page is loaded. Initial visibility should be 'prerender'.
    test->waitUntilLoadFinished();

    GUniqueOutPtr<GError> error;
    WebKitJavascriptResult* javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.visibilityState;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    GUniquePtr<char> valueString(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "prerender");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.hidden;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultToBoolean(javascriptResult));

    // Show the page. The visibility should be updated to 'visible'.
    test->showInWindow();
    test->waitUntilTitleChanged();

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.visibilityState;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.reset(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "visible");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.hidden;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(!WebViewTest::javascriptResultToBoolean(javascriptResult));

    // Hide the page. The visibility should be updated to 'hidden'.
    gtk_widget_hide(GTK_WIDGET(test->m_webView));
    test->waitUntilTitleChanged();

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.visibilityState;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    valueString.reset(WebViewTest::javascriptResultToCString(javascriptResult));
    g_assert_cmpstr(valueString.get(), ==, "hidden");

    javascriptResult = test->runJavaScriptAndWaitUntilFinished("document.hidden;", &error.outPtr());
    g_assert(javascriptResult);
    g_assert(!error.get());
    g_assert(WebViewTest::javascriptResultToBoolean(javascriptResult));
}

class SnapshotWebViewTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(SnapshotWebViewTest);

    static void onSnapshotCancelledReady(WebKitWebView* web_view, GAsyncResult* res, SnapshotWebViewTest* test)
    {
        GUniqueOutPtr<GError> error;
        test->m_surface = webkit_web_view_get_snapshot_finish(web_view, res, &error.outPtr());
        g_assert(!test->m_surface);
        g_assert_error(error.get(), G_IO_ERROR, G_IO_ERROR_CANCELLED);
        test->quitMainLoop();
    }

    gboolean getSnapshotAndCancel()
    {
        if (m_surface)
            cairo_surface_destroy(m_surface);
        m_surface = 0;
        GRefPtr<GCancellable> cancellable = adoptGRef(g_cancellable_new());
        webkit_web_view_get_snapshot(m_webView, WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE, cancellable.get(), reinterpret_cast<GAsyncReadyCallback>(onSnapshotCancelledReady), this);
        g_cancellable_cancel(cancellable.get());
        g_main_loop_run(m_mainLoop);

        return true;
    }

};

static void testWebViewSnapshot(SnapshotWebViewTest* test, gconstpointer)
{
    test->loadHtml("<html><head><style>html { width: 200px; height: 100px; } ::-webkit-scrollbar { display: none; }</style></head><body><p>Whatever</p></body></html>", nullptr);
    test->waitUntilLoadFinished();

    // WEBKIT_SNAPSHOT_REGION_VISIBLE returns a null surface when the view is not visible.
    cairo_surface_t* surface1 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE);
    g_assert(!surface1);

    // WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT works even if the window is not visible.
    surface1 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT, WEBKIT_SNAPSHOT_OPTIONS_NONE);
    g_assert(surface1);
    g_assert_cmpuint(cairo_surface_get_type(surface1), ==, CAIRO_SURFACE_TYPE_IMAGE);
    g_assert_cmpint(cairo_image_surface_get_width(surface1), ==, 200);
    g_assert_cmpint(cairo_image_surface_get_height(surface1), ==, 100);

    // Show the WebView in a popup widow of 50x50 and try again with WEBKIT_SNAPSHOT_REGION_VISIBLE.
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_POPUP, 50, 50);
    surface1 = cairo_surface_reference(test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE));
    g_assert(surface1);
    g_assert_cmpuint(cairo_surface_get_type(surface1), ==, CAIRO_SURFACE_TYPE_IMAGE);
    g_assert_cmpint(cairo_image_surface_get_width(surface1), ==, 50);
    g_assert_cmpint(cairo_image_surface_get_height(surface1), ==, 50);

    // Select all text in the WebView, request a snapshot ignoring selection.
    test->selectAll();
    cairo_surface_t* surface2 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_NONE);
    g_assert(surface2);
    g_assert(Test::cairoSurfacesEqual(surface1, surface2));

    // Request a new snapshot, including the selection this time. The size should be the same but the result
    // must be different to the one previously obtained.
    surface2 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_INCLUDE_SELECTION_HIGHLIGHTING);
    g_assert_cmpuint(cairo_surface_get_type(surface2), ==, CAIRO_SURFACE_TYPE_IMAGE);
    g_assert_cmpint(cairo_image_surface_get_width(surface1), ==, cairo_image_surface_get_width(surface2));
    g_assert_cmpint(cairo_image_surface_get_height(surface1), ==, cairo_image_surface_get_height(surface2));
    g_assert(!Test::cairoSurfacesEqual(surface1, surface2));

    // Get a snpashot with a transparent background, the result must be different.
    surface2 = test->getSnapshotAndWaitUntilReady(WEBKIT_SNAPSHOT_REGION_VISIBLE, WEBKIT_SNAPSHOT_OPTIONS_TRANSPARENT_BACKGROUND);
    g_assert_cmpuint(cairo_surface_get_type(surface2), ==, CAIRO_SURFACE_TYPE_IMAGE);
    g_assert_cmpint(cairo_image_surface_get_width(surface1), ==, cairo_image_surface_get_width(surface2));
    g_assert_cmpint(cairo_image_surface_get_height(surface1), ==, cairo_image_surface_get_height(surface2));
    g_assert(!Test::cairoSurfacesEqual(surface1, surface2));
    cairo_surface_destroy(surface1);

    // Test that cancellation works.
    g_assert(test->getSnapshotAndCancel());
}

class NotificationWebViewTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(NotificationWebViewTest);

    enum NotificationEvent {
        None,
        Permission,
        Shown,
        Closed,
        OnClosed,
    };

    static gboolean permissionRequestCallback(WebKitWebView*, WebKitPermissionRequest *request, NotificationWebViewTest* test)
    {
        g_assert(WEBKIT_IS_NOTIFICATION_PERMISSION_REQUEST(request));
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(request));

        test->m_event = Permission;

        webkit_permission_request_allow(request);

        g_main_loop_quit(test->m_mainLoop);

        return TRUE;
    }

    static gboolean notificationClosedCallback(WebKitNotification* notification, NotificationWebViewTest* test)
    {
        g_assert(test->m_notification == notification);
        test->m_notification = nullptr;
        test->m_event = Closed;
        if (g_main_loop_is_running(test->m_mainLoop))
            g_main_loop_quit(test->m_mainLoop);
        return TRUE;
    }

    static gboolean showNotificationCallback(WebKitWebView*, WebKitNotification* notification, NotificationWebViewTest* test)
    {
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(notification));
        test->m_notification = notification;
        g_signal_connect(notification, "closed", G_CALLBACK(notificationClosedCallback), test);
        test->m_event = Shown;
        g_main_loop_quit(test->m_mainLoop);
        return TRUE;
    }

    static void notificationsMessageReceivedCallback(WebKitUserContentManager* userContentManager, WebKitJavascriptResult*, NotificationWebViewTest* test)
    {
        test->m_event = OnClosed;
        g_main_loop_quit(test->m_mainLoop);
    }

    NotificationWebViewTest()
        : WebViewTest(webkit_user_content_manager_new())
        , m_notification(nullptr)
        , m_event(None)
    {
        g_signal_connect(m_webView, "permission-request", G_CALLBACK(permissionRequestCallback), this);
        g_signal_connect(m_webView, "show-notification", G_CALLBACK(showNotificationCallback), this);
        WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(m_webView);
        webkit_user_content_manager_register_script_message_handler(manager, "notifications");
        g_signal_connect(manager, "script-message-received::notifications", G_CALLBACK(notificationsMessageReceivedCallback), this);
    }

    ~NotificationWebViewTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(m_webView);
        g_signal_handlers_disconnect_matched(manager, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        webkit_user_content_manager_unregister_script_message_handler(manager, "notifications");
    }

   void requestPermissionAndWaitUntilGiven()
    {
        m_event = None;
        webkit_web_view_run_javascript(m_webView, "Notification.requestPermission();", nullptr, nullptr, nullptr);
        g_main_loop_run(m_mainLoop);
    }

    void requestNotificationAndWaitUntilShown(const char* title, const char* body)
    {
        m_event = None;

        GUniquePtr<char> jscode(g_strdup_printf("n = new Notification('%s', { body: '%s'});", title, body));
        webkit_web_view_run_javascript(m_webView, jscode.get(), nullptr, nullptr, nullptr);

        g_main_loop_run(m_mainLoop);
    }

    void closeNotificationAndWaitUntilClosed()
    {
        m_event = None;
        webkit_web_view_run_javascript(m_webView, "n.close()", nullptr, nullptr, nullptr);
        g_main_loop_run(m_mainLoop);
    }

    void closeNotificationAndWaitUntilOnClosed()
    {
        g_assert(m_notification);
        m_event = None;
        runJavaScriptAndWaitUntilFinished("n.onclose = function() { window.webkit.messageHandlers.notifications.postMessage('closed'); }", nullptr);
        webkit_notification_close(m_notification);
        g_assert(m_event == Closed);
        g_main_loop_run(m_mainLoop);
    }

    NotificationEvent m_event;
    WebKitNotification* m_notification;
};

static void testWebViewNotification(NotificationWebViewTest* test, gconstpointer)
{
    // Notifications don't work with local or special schemes.
    test->loadURI(gServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();

    test->requestPermissionAndWaitUntilGiven();
    g_assert(test->m_event == NotificationWebViewTest::Permission);

    static const char* title = "This is a notification";
    static const char* body = "This is the body.";
    test->requestNotificationAndWaitUntilShown(title, body);

    g_assert(test->m_event == NotificationWebViewTest::Shown);
    g_assert(test->m_notification);
    g_assert_cmpstr(webkit_notification_get_title(test->m_notification), ==, title);
    g_assert_cmpstr(webkit_notification_get_body(test->m_notification), ==, body);

    test->closeNotificationAndWaitUntilClosed();
    g_assert(test->m_event == NotificationWebViewTest::Closed);

    test->requestNotificationAndWaitUntilShown(title, body);
    g_assert(test->m_event == NotificationWebViewTest::Shown);

    test->closeNotificationAndWaitUntilOnClosed();
    g_assert(test->m_event == NotificationWebViewTest::OnClosed);

    test->requestNotificationAndWaitUntilShown(title, body);
    g_assert(test->m_event == NotificationWebViewTest::Shown);

    test->loadURI(gServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();
    g_assert(test->m_event == NotificationWebViewTest::Closed);
}

static void testWebViewIsPlayingAudio(IsPlayingAudioWebViewTest* test, gconstpointer)
{
    // The web view must be realized for the video to start playback and
    // trigger changes in WebKitWebView::is-playing-audio.
    test->showInWindowAndWaitUntilMapped(GTK_WINDOW_TOPLEVEL);

    // Initially, web views should always report no audio being played.
    g_assert(!webkit_web_view_is_playing_audio(test->m_webView));

    GUniquePtr<char> resourcePath(g_build_filename(Test::getResourcesDir(Test::WebKit2Resources).data(), "file-with-video.html", nullptr));
    GUniquePtr<char> resourceURL(g_filename_to_uri(resourcePath.get(), nullptr, nullptr));
    webkit_web_view_load_uri(test->m_webView, resourceURL.get());
    test->waitUntilLoadFinished();
    g_assert(!webkit_web_view_is_playing_audio(test->m_webView));

    webkit_web_view_run_javascript(test->m_webView, "playVideo();", nullptr, nullptr, nullptr);
    test->waitUntilIsPlayingAudioChanged();
    g_assert(webkit_web_view_is_playing_audio(test->m_webView));

    // Pause the video, and check again.
    webkit_web_view_run_javascript(test->m_webView, "document.getElementById('test-video').pause();", nullptr, nullptr, nullptr);
    test->waitUntilIsPlayingAudioChanged();
    g_assert(!webkit_web_view_is_playing_audio(test->m_webView));
}

static void testWebViewBackgroundColor(WebViewTest* test, gconstpointer)
{
    // White is the default background.
    GdkRGBA rgba;
    webkit_web_view_get_background_color(test->m_webView, &rgba);
    g_assert_cmpfloat(rgba.red, ==, 1);
    g_assert_cmpfloat(rgba.green, ==, 1);
    g_assert_cmpfloat(rgba.blue, ==, 1);
    g_assert_cmpfloat(rgba.alpha, ==, 1);

    // Set a different (semi-transparent red).
    rgba.red = 1;
    rgba.green = 0;
    rgba.blue = 0;
    rgba.alpha = 0.5;
    webkit_web_view_set_background_color(test->m_webView, &rgba);
    g_assert_cmpfloat(rgba.red, ==, 1);
    g_assert_cmpfloat(rgba.green, ==, 0);
    g_assert_cmpfloat(rgba.blue, ==, 0);
    g_assert_cmpfloat(rgba.alpha, ==, 0.5);

    // The actual rendering can't be tested using unit tests, use
    // MiniBrowser --bg-color="<color-value>" for manually testing this API.
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_equal(path, "/")) {
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_complete(message->response_body);
    } else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
}

void beforeAll()
{
    gServer = new WebKitTestServer();
    gServer->run(serverCallback);

    WebViewTest::add("WebKitWebView", "web-context", testWebViewWebContext);
    WebViewTest::add("WebKitWebView", "web-context-lifetime", testWebViewWebContextLifetime);
    WebViewTest::add("WebKitWebView", "custom-charset", testWebViewCustomCharset);
    WebViewTest::add("WebKitWebView", "settings", testWebViewSettings);
    WebViewTest::add("WebKitWebView", "zoom-level", testWebViewZoomLevel);
    WebViewTest::add("WebKitWebView", "run-javascript", testWebViewRunJavaScript);
    FullScreenClientTest::add("WebKitWebView", "fullscreen", testWebViewFullScreen);
    WebViewTest::add("WebKitWebView", "can-show-mime-type", testWebViewCanShowMIMEType);
    FormClientTest::add("WebKitWebView", "submit-form", testWebViewSubmitForm);
    SaveWebViewTest::add("WebKitWebView", "save", testWebViewSave);
    SnapshotWebViewTest::add("WebKitWebView", "snapshot", testWebViewSnapshot);
    WebViewTest::add("WebKitWebView", "page-visibility", testWebViewPageVisibility);
    NotificationWebViewTest::add("WebKitWebView", "notification", testWebViewNotification);
    IsPlayingAudioWebViewTest::add("WebKitWebView", "is-playing-audio", testWebViewIsPlayingAudio);
    WebViewTest::add("WebKitWebView", "background-color", testWebViewBackgroundColor);
}

void afterAll()
{
}
