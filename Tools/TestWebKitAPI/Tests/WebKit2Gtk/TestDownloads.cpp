/*
 * Copyright (C) 2012, 2014 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <string.h>
#include <webkit2/webkit2.h>
#include <wtf/Vector.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>

class DownloadTest: public Test {
public:
    MAKE_GLIB_TEST_FIXTURE(DownloadTest);

    enum DownloadEvent {
        Started,
        ReceivedResponse,
        CreatedDestination,
        ReceivedData,
        Failed,
        Finished
    };

    static void receivedResponseCallback(WebKitDownload* download, GParamSpec*, DownloadTest* test)
    {
        g_assert(webkit_download_get_response(download));
        test->receivedResponse(download);
    }

    static void createdDestinationCallback(WebKitDownload* download, const gchar* destination, DownloadTest* test)
    {
        g_assert(webkit_download_get_destination(download));
        g_assert_cmpstr(webkit_download_get_destination(download), ==, destination);
        GRefPtr<GFile> file = adoptGRef(g_file_new_for_uri(destination));
        g_assert(g_file_query_exists(file.get(), nullptr));
        test->createdDestination(download, destination);
    }

    static void receivedDataCallback(WebKitDownload* download, guint64 dataLength, DownloadTest* test)
    {
        test->receivedData(download, dataLength);
    }

    static void finishedCallback(WebKitDownload* download, DownloadTest* test)
    {
        test->finished(download);
    }

    static void failedCallback(WebKitDownload* download, GError* error, DownloadTest* test)
    {
        g_assert(error);

        const char* destinationURI = webkit_download_get_destination(download);
        if (destinationURI) {
            GUniquePtr<char> tempFileURI(g_strconcat(destinationURI, ".wkdownload", nullptr));
            GRefPtr<GFile> tempFile = adoptGRef(g_file_new_for_uri(tempFileURI.get()));
            g_assert(!g_file_query_exists(tempFile.get(), nullptr));
        }

        test->failed(download, error);
    }

    static gboolean decideDestinationCallback(WebKitDownload* download, const gchar* suggestedFilename, DownloadTest* test)
    {
        g_assert(suggestedFilename);
        test->decideDestination(download, suggestedFilename);
        return TRUE;
    }

    static void downloadStartedCallback(WebKitWebContext* context, WebKitDownload* download, DownloadTest* test)
    {
        g_assert(webkit_download_get_request(download));
        test->started(download);
        g_signal_connect(download, "notify::response", G_CALLBACK(receivedResponseCallback), test);
        g_signal_connect(download, "created-destination", G_CALLBACK(createdDestinationCallback), test);
        g_signal_connect(download, "received-data", G_CALLBACK(receivedDataCallback), test);
        g_signal_connect(download, "finished", G_CALLBACK(finishedCallback), test);
        g_signal_connect(download, "failed", G_CALLBACK(failedCallback), test);
        g_signal_connect(download, "decide-destination", G_CALLBACK(decideDestinationCallback), test);
    }

    DownloadTest()
        : m_mainLoop(g_main_loop_new(nullptr, TRUE))
        , m_downloadSize(0)
        , m_allowOverwrite(false)
    {
        g_signal_connect(m_webContext.get(), "download-started", G_CALLBACK(downloadStartedCallback), this);
    }

    ~DownloadTest()
    {
        g_signal_handlers_disconnect_matched(m_webContext.get(), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        g_main_loop_unref(m_mainLoop);
    }

    virtual void started(WebKitDownload* download)
    {
        m_downloadSize = 0;
        m_downloadEvents.clear();
        m_downloadEvents.append(Started);
    }

    virtual void receivedResponse(WebKitDownload* download)
    {
        m_downloadEvents.append(ReceivedResponse);
    }

    virtual void createdDestination(WebKitDownload* download, const char* destination)
    {
        m_downloadEvents.append(CreatedDestination);
    }

    virtual void receivedData(WebKitDownload* download, guint64 dataLength)
    {
        m_downloadSize += dataLength;
        if (!m_downloadEvents.contains(ReceivedData))
            m_downloadEvents.append(ReceivedData);
    }

    virtual void finished(WebKitDownload* download)
    {
        g_assert_cmpuint(m_downloadSize, ==, webkit_download_get_received_data_length(download));
        m_downloadEvents.append(Finished);
        g_main_loop_quit(m_mainLoop);
    }

    virtual void failed(WebKitDownload* download, GError* error)
    {
        m_downloadEvents.append(Failed);
    }

    virtual void decideDestination(WebKitDownload* download, const gchar* suggestedFilename)
    {
        GUniquePtr<char> destination(g_build_filename(Test::dataDirectory(), suggestedFilename, nullptr));
        GUniquePtr<char> destinationURI(g_filename_to_uri(destination.get(), 0, 0));
        webkit_download_set_destination(download, destinationURI.get());
    }

    WebKitDownload* downloadURIAndWaitUntilFinishes(const CString& requestURI)
    {
        WebKitDownload* download = webkit_web_context_download_uri(m_webContext.get(), requestURI.data());
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download));

        g_assert(!webkit_download_get_allow_overwrite(download));
        webkit_download_set_allow_overwrite(download, m_allowOverwrite);
        g_assert(webkit_download_get_allow_overwrite(download) == m_allowOverwrite);

        WebKitURIRequest* request = webkit_download_get_request(download);
        g_assert(request);
        ASSERT_CMP_CSTRING(webkit_uri_request_get_uri(request), ==, requestURI);

        g_main_loop_run(m_mainLoop);

        return download;
    }

    void checkDestinationAndDeleteFile(WebKitDownload* download, const char* expectedName)
    {
        if (!webkit_download_get_destination(download))
            return;
        GRefPtr<GFile> destFile = adoptGRef(g_file_new_for_uri(webkit_download_get_destination(download)));
        GUniquePtr<char> destBasename(g_file_get_basename(destFile.get()));
        g_assert_cmpstr(destBasename.get(), ==, expectedName);

        g_file_delete(destFile.get(), 0, 0);
    }

    GMainLoop* m_mainLoop;
    Vector<DownloadEvent> m_downloadEvents;
    guint64 m_downloadSize;
    bool m_allowOverwrite;
};

static GRefPtr<WebKitDownload> downloadLocalFileSuccessfully(DownloadTest* test, const char* filename)
{
    GUniquePtr<char> sourcePath(g_build_filename(Test::getResourcesDir().data(), filename, nullptr));
    GRefPtr<GFile> source = adoptGRef(g_file_new_for_path(sourcePath.get()));
    GRefPtr<GFileInfo> sourceInfo = adoptGRef(g_file_query_info(source.get(), G_FILE_ATTRIBUTE_STANDARD_SIZE, static_cast<GFileQueryInfoFlags>(0), 0, 0));
    GUniquePtr<char> sourceURI(g_file_get_uri(source.get()));
    GRefPtr<WebKitDownload> download = adoptGRef(test->downloadURIAndWaitUntilFinishes(sourceURI.get()));
    g_assert(!webkit_download_get_web_view(download.get()));

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 5);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::CreatedDestination);
    g_assert_cmpint(events[3], ==, DownloadTest::ReceivedData);
    g_assert_cmpint(events[4], ==, DownloadTest::Finished);

    WebKitURIRequest* request = webkit_download_get_request(download.get());
    g_assert(request);
    g_assert_cmpstr(webkit_uri_request_get_uri(request), ==, sourceURI.get());

    g_assert_cmpint(test->m_downloadSize, ==, g_file_info_get_size(sourceInfo.get()));
    g_assert(webkit_download_get_destination(download.get()));
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), ==, 1);

    return download;
}

static void testDownloadLocalFile(DownloadTest* test, gconstpointer)
{
    static const char* filename = "test.pdf";
    GRefPtr<WebKitDownload> download = downloadLocalFileSuccessfully(test, filename);
    test->checkDestinationAndDeleteFile(download.get(), filename);
}

static void createFileAtDestination(const char* filename)
{
    GUniquePtr<char> path(g_build_filename(Test::dataDirectory(), filename, nullptr));
    GRefPtr<GFile> file = adoptGRef(g_file_new_for_path(path.get()));
    GUniqueOutPtr<GError> error;
    g_file_create(file.get(), G_FILE_CREATE_NONE, nullptr, &error.outPtr());
    g_assert(!error);
    g_assert(g_file_query_exists(file.get(), nullptr));
}

static void testDownloadOverwriteDestinationAllowed(DownloadTest* test, gconstpointer)
{
    static const char* filename = "test.pdf";
    createFileAtDestination(filename);

    test->m_allowOverwrite = true;
    GRefPtr<WebKitDownload> download = downloadLocalFileSuccessfully(test, filename);
    test->checkDestinationAndDeleteFile(download.get(), filename);
}

class DownloadErrorTest: public DownloadTest {
public:
    MAKE_GLIB_TEST_FIXTURE(DownloadErrorTest);

    enum ExpectedError {
        NetworkError,
        DownloadCancelled,
        InvalidDestination,
        DestinationExists
    };

    DownloadErrorTest()
        : m_expectedError(NetworkError)
    {
    }

    void receivedResponse(WebKitDownload* download)
    {
        DownloadTest::receivedResponse(download);
    }

    void createdDestination(WebKitDownload* download, const char* destination)
    {
        if (m_expectedError == DownloadCancelled)
            webkit_download_cancel(download);
        else
            g_assert_not_reached();
    }

    void failed(WebKitDownload* download, GError* error)
    {
        g_assert(g_error_matches(error, WEBKIT_DOWNLOAD_ERROR, expectedErrorToWebKitDownloadError(m_expectedError)));
        DownloadTest::failed(download, error);
    }

    void decideDestination(WebKitDownload* download, const gchar* suggestedFilename)
    {
        if (m_expectedError != InvalidDestination) {
            DownloadTest::decideDestination(download, suggestedFilename);
            return;
        }
        webkit_download_set_destination(download, "file:///foo/bar");
    }

    static WebKitDownloadError expectedErrorToWebKitDownloadError(ExpectedError expected)
    {
        switch (expected) {
        case NetworkError:
            return WEBKIT_DOWNLOAD_ERROR_NETWORK;
        case DownloadCancelled:
            return WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER;
        case InvalidDestination:
            return WEBKIT_DOWNLOAD_ERROR_DESTINATION;
        case DestinationExists:
            return WEBKIT_DOWNLOAD_ERROR_DESTINATION;
        default:
            g_assert_not_reached();
        }
    }

    ExpectedError m_expectedError;
};

static void testDownloadOverwriteDestinationDisallowed(DownloadErrorTest* test, gconstpointer)
{
    static const char* filename = "test.pdf";
    createFileAtDestination(filename);

    test->m_expectedError = DownloadErrorTest::DestinationExists;
    GUniquePtr<char> sourcePath(g_build_filename(Test::getResourcesDir().data(), filename, nullptr));
    GRefPtr<GFile> source = adoptGRef(g_file_new_for_path(sourcePath.get()));
    GUniquePtr<char> sourceURI(g_file_get_uri(source.get()));
    GRefPtr<WebKitDownload> download = adoptGRef(test->downloadURIAndWaitUntilFinishes(sourceURI.get()));
    g_assert(!webkit_download_get_web_view(download.get()));

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), ==, 0);

    test->checkDestinationAndDeleteFile(download.get(), filename);
}

static void testDownloadLocalFileError(DownloadErrorTest* test, gconstpointer)
{
    test->m_expectedError = DownloadErrorTest::NetworkError;
    GRefPtr<WebKitDownload> download = adoptGRef(test->downloadURIAndWaitUntilFinishes("file:///foo/bar"));
    g_assert(!webkit_download_get_web_view(download.get()));

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 3);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::Failed);
    g_assert_cmpint(events[2], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);

    test->m_expectedError = DownloadErrorTest::InvalidDestination;
    GUniquePtr<char> path(g_build_filename(Test::getResourcesDir().data(), "test.pdf", nullptr));
    GRefPtr<GFile> file = adoptGRef(g_file_new_for_path(path.get()));
    GUniquePtr<char> uri(g_file_get_uri(file.get()));
    download = adoptGRef(test->downloadURIAndWaitUntilFinishes(uri.get()));
    g_assert(!webkit_download_get_web_view(download.get()));

    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    test->checkDestinationAndDeleteFile(download.get(), "bar");

    test->m_expectedError = DownloadErrorTest::DownloadCancelled;
    download = adoptGRef(test->downloadURIAndWaitUntilFinishes(uri.get()));
    g_assert(!webkit_download_get_web_view(download.get()));

    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    test->checkDestinationAndDeleteFile(download.get(), "test.pdf");
}

static WebKitTestServer* kServer;
static const char* kServerSuggestedFilename = "webkit-downloaded-file";

static void addContentDispositionHTTPHeaderToResponse(SoupMessage* message)
{
    GUniquePtr<char> contentDisposition(g_strdup_printf("filename=%s", kServerSuggestedFilename));
    soup_message_headers_append(message->response_headers, "Content-Disposition", contentDisposition.get());
}

static gboolean writeNextChunkIdle(SoupMessage* message)
{
    soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, "chunk", 5);
    return FALSE;
}

static void writeNextChunk(SoupMessage* message)
{
    g_timeout_add(100, reinterpret_cast<GSourceFunc>(writeNextChunkIdle), message);
}

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);

    if (g_str_equal(path, "/cancel-after-destination")) {
        // Use an infinite message to make sure it's cancelled before it finishes.
        soup_message_headers_set_encoding(message->response_headers, SOUP_ENCODING_CHUNKED);
        addContentDispositionHTTPHeaderToResponse(message);
        g_signal_connect(message, "wrote_headers", G_CALLBACK(writeNextChunk), nullptr);
        g_signal_connect(message, "wrote_chunk", G_CALLBACK(writeNextChunk), nullptr);
        return;
    }

    GUniquePtr<char> filePath(g_build_filename(Test::getResourcesDir().data(), path, nullptr));
    char* contents;
    gsize contentsLength;
    if (!g_file_get_contents(filePath.get(), &contents, &contentsLength, 0)) {
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
        soup_message_body_complete(message->response_body);
        return;
    }

    addContentDispositionHTTPHeaderToResponse(message);
    soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, contentsLength);

    soup_message_body_complete(message->response_body);
}

static void testDownloadRemoteFile(DownloadTest* test, gconstpointer)
{
    GRefPtr<WebKitDownload> download = adoptGRef(test->downloadURIAndWaitUntilFinishes(kServer->getURIForPath("/test.pdf")));
    g_assert(!webkit_download_get_web_view(download.get()));

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 5);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::CreatedDestination);
    g_assert_cmpint(events[3], ==, DownloadTest::ReceivedData);
    g_assert_cmpint(events[4], ==, DownloadTest::Finished);
    events.clear();

    WebKitURIRequest* request = webkit_download_get_request(download.get());
    g_assert(request);
    ASSERT_CMP_CSTRING(webkit_uri_request_get_uri(request), ==, kServer->getURIForPath("/test.pdf"));

    g_assert(webkit_download_get_destination(download.get()));
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), ==, 1);
    test->checkDestinationAndDeleteFile(download.get(), kServerSuggestedFilename);
}

static void testDownloadRemoteFileError(DownloadErrorTest* test, gconstpointer)
{
    test->m_expectedError = DownloadErrorTest::NetworkError;
    GRefPtr<WebKitDownload> download = adoptGRef(test->downloadURIAndWaitUntilFinishes(kServer->getURIForPath("/foo")));
    g_assert(!webkit_download_get_web_view(download.get()));

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    WebKitURIResponse* response = webkit_download_get_response(download.get());
    g_assert_cmpuint(webkit_uri_response_get_status_code(response), ==, 404);
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);

    test->m_expectedError = DownloadErrorTest::InvalidDestination;
    download = adoptGRef(test->downloadURIAndWaitUntilFinishes(kServer->getURIForPath("/test.pdf")));
    g_assert(!webkit_download_get_web_view(download.get()));

    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    test->checkDestinationAndDeleteFile(download.get(), "bar");

    test->m_expectedError = DownloadErrorTest::DownloadCancelled;
    download = adoptGRef(test->downloadURIAndWaitUntilFinishes(kServer->getURIForPath("/cancel-after-destination")));
    g_assert(!webkit_download_get_web_view(download.get()));

    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    // Check the intermediate file is deleted when the download is cancelled.
    GUniquePtr<char> intermediateURI(g_strdup_printf("%s.wkdownload", webkit_download_get_destination(download.get())));
    GRefPtr<GFile> intermediateFile = adoptGRef(g_file_new_for_uri(intermediateURI.get()));
    g_assert(!g_file_query_exists(intermediateFile.get(), nullptr));
}

class WebViewDownloadTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(WebViewDownloadTest);

    static void downloadStartedCallback(WebKitWebContext* context, WebKitDownload* download, WebViewDownloadTest* test)
    {
        test->m_download = download;
        test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download));
        test->quitMainLoop();
    }

    WebViewDownloadTest()
    {
        g_signal_connect(webkit_web_view_get_context(m_webView), "download-started", G_CALLBACK(downloadStartedCallback), this);
    }

    ~WebViewDownloadTest()
    {
        g_signal_handlers_disconnect_matched(webkit_web_view_get_context(m_webView), G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void waitUntilDownloadStarted()
    {
        m_download = 0;
        g_main_loop_run(m_mainLoop);
        g_assert(m_download.get());
    }

    static gboolean downloadDecideDestinationCallback(WebKitDownload* download, const gchar* suggestedFilename, WebViewDownloadTest* test)
    {
        GUniquePtr<char> destination(g_build_filename(Test::dataDirectory(), suggestedFilename, nullptr));
        GUniquePtr<char> destinationURI(g_filename_to_uri(destination.get(), 0, 0));
        webkit_download_set_destination(download, destinationURI.get());
        return TRUE;
    }

    static void downloadFinishedCallback(WebKitDownload* download, WebViewDownloadTest* test)
    {
        test->quitMainLoop();
    }

    void waitUntilDownloadFinished()
    {
        g_signal_connect(m_download.get(), "decide-destination", G_CALLBACK(downloadDecideDestinationCallback), this);
        g_signal_connect(m_download.get(), "finished", G_CALLBACK(downloadFinishedCallback), this);
        g_main_loop_run(m_mainLoop);
    }

    GRefPtr<WebKitDownload> m_download;
};

static void testWebViewDownloadURI(WebViewDownloadTest* test, gconstpointer)
{
    GRefPtr<WebKitDownload> download = adoptGRef(webkit_web_view_download_uri(test->m_webView, kServer->getURIForPath("/test.pdf").data()));
    test->waitUntilDownloadStarted();
    g_assert(test->m_webView == webkit_download_get_web_view(download.get()));
    test->waitUntilDownloadFinished();

    GRefPtr<GFile> downloadFile = adoptGRef(g_file_new_for_uri(webkit_download_get_destination(download.get())));
    GRefPtr<GFileInfo> downloadFileInfo = adoptGRef(g_file_query_info(downloadFile.get(), G_FILE_ATTRIBUTE_STANDARD_SIZE, static_cast<GFileQueryInfoFlags>(0), 0, 0));
    g_assert_cmpint(g_file_info_get_size(downloadFileInfo.get()), >, 0);
    g_file_delete(downloadFile.get(), 0, 0);
}

class PolicyResponseDownloadTest: public WebViewDownloadTest {
public:
    MAKE_GLIB_TEST_FIXTURE(PolicyResponseDownloadTest);

    static gboolean decidePolicyCallback(WebKitWebView* webView, WebKitPolicyDecision* decision, WebKitPolicyDecisionType type, PolicyResponseDownloadTest* test)
    {
        if (type != WEBKIT_POLICY_DECISION_TYPE_RESPONSE)
            return FALSE;

        webkit_policy_decision_download(decision);
        return TRUE;
    }

    PolicyResponseDownloadTest()
    {
        g_signal_connect(m_webView, "decide-policy", G_CALLBACK(decidePolicyCallback), this);
    }

    ~PolicyResponseDownloadTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    void cancelDownloadAndWaitUntilFinished()
    {
        webkit_download_cancel(m_download.get());
        waitUntilDownloadFinished();
        m_download = 0;
    }
};

static void testPolicyResponseDownload(PolicyResponseDownloadTest* test, gconstpointer)
{
    // Test that a download started by the the policy checker contains the web view.
    CString requestURI = kServer->getURIForPath("/test.pdf").data();
    test->loadURI(requestURI.data());
    test->waitUntilDownloadStarted();

    WebKitURIRequest* request = webkit_download_get_request(test->m_download.get());
    g_assert(request);
    ASSERT_CMP_CSTRING(webkit_uri_request_get_uri(request), ==, requestURI);

    g_assert(test->m_webView == webkit_download_get_web_view(test->m_download.get()));
    test->cancelDownloadAndWaitUntilFinished();
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    DownloadTest::add("Downloads", "local-file", testDownloadLocalFile);
    DownloadTest::add("Downloads", "overwrite-destination-allowed", testDownloadOverwriteDestinationAllowed);
    DownloadErrorTest::add("Downloads", "overwrite-destination-disallowed", testDownloadOverwriteDestinationDisallowed);
    DownloadErrorTest::add("Downloads", "local-file-error", testDownloadLocalFileError);
    DownloadTest::add("Downloads", "remote-file", testDownloadRemoteFile);
    DownloadErrorTest::add("Downloads", "remote-file-error", testDownloadRemoteFileError);
    WebViewDownloadTest::add("WebKitWebView", "download-uri", testWebViewDownloadURI);
    PolicyResponseDownloadTest::add("Downloads", "policy-decision-download", testPolicyResponseDownload);
}

void afterAll()
{
    delete kServer;
}
