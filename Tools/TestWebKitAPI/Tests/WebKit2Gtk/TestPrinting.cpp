/*
 * Copyright (C) 2012 Igalia S.L.
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
#include <glib/gstdio.h>
#include <wtf/glib/GRefPtr.h>

#ifdef HAVE_GTK_UNIX_PRINTING
#include <gtk/gtkunixprint.h>
#endif

static void testPrintOperationPrintSettings(WebViewTest* test, gconstpointer)
{
    GRefPtr<WebKitPrintOperation> printOperation = adoptGRef(webkit_print_operation_new(test->m_webView));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(printOperation.get()));

    g_assert(!webkit_print_operation_get_print_settings(printOperation.get()));
    g_assert(!webkit_print_operation_get_page_setup(printOperation.get()));

    GRefPtr<GtkPrintSettings> printSettings = adoptGRef(gtk_print_settings_new());
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(printSettings.get()));

    GRefPtr<GtkPageSetup> pageSetup = adoptGRef(gtk_page_setup_new());
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(pageSetup.get()));

    webkit_print_operation_set_print_settings(printOperation.get(), printSettings.get());
    webkit_print_operation_set_page_setup(printOperation.get(), pageSetup.get());

    g_assert(webkit_print_operation_get_print_settings(printOperation.get()) == printSettings.get());
    g_assert(webkit_print_operation_get_page_setup(printOperation.get()) == pageSetup.get());
}

static gboolean webViewPrintCallback(WebKitWebView* webView, WebKitPrintOperation* printOperation, WebViewTest* test)
{
    g_assert(webView == test->m_webView);

    g_assert(WEBKIT_IS_PRINT_OPERATION(printOperation));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(printOperation));

    g_assert(!webkit_print_operation_get_print_settings(printOperation));
    g_assert(!webkit_print_operation_get_page_setup(printOperation));

    g_main_loop_quit(test->m_mainLoop);

    return TRUE;
}

static void testWebViewPrint(WebViewTest* test, gconstpointer)
{
    g_signal_connect(test->m_webView, "print", G_CALLBACK(webViewPrintCallback), test);
    test->loadHtml("<html><body onLoad=\"print();\">WebKitGTK+ printing test</body></html>", 0);
    g_main_loop_run(test->m_mainLoop);
}

#ifdef HAVE_GTK_UNIX_PRINTING
static gboolean testPrintOperationPrintPrinter(GtkPrinter* printer, gpointer userData)
{
    if (strcmp(gtk_printer_get_name(printer), "Print to File"))
        return FALSE;

    GtkPrinter** foundPrinter = static_cast<GtkPrinter**>(userData);
    *foundPrinter = static_cast<GtkPrinter*>(g_object_ref(printer));
    return TRUE;
}

static GtkPrinter* findPrintToFilePrinter()
{
    GtkPrinter* printer = 0;
    gtk_enumerate_printers(testPrintOperationPrintPrinter, &printer, 0, TRUE);
    return printer;
}

class PrintTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(PrintTest);

    static void printFinishedCallback(WebKitPrintOperation*, PrintTest* test)
    {
        g_main_loop_quit(test->m_mainLoop);
    }

    static void printFailedCallback(WebKitPrintOperation*, GError* error, PrintTest* test)
    {
        g_assert(test->m_expectedError);
        g_assert(error);
        g_assert(g_error_matches(error, WEBKIT_PRINT_ERROR, test->m_expectedError));
    }

    PrintTest()
        : m_expectedError(0)
    {
        m_printOperation = adoptGRef(webkit_print_operation_new(m_webView));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_printOperation.get()));
        g_signal_connect(m_printOperation.get(), "finished", G_CALLBACK(printFinishedCallback), this);
        g_signal_connect(m_printOperation.get(), "failed", G_CALLBACK(printFailedCallback), this);
    }

    void waitUntilPrintFinished()
    {
        g_main_loop_run(m_mainLoop);
    }

    GRefPtr<WebKitPrintOperation> m_printOperation;
    unsigned m_expectedError;
};

static void testPrintOperationPrint(PrintTest* test, gconstpointer)
{
    test->loadHtml("<html><body>WebKitGTK+ printing test</body></html>", 0);
    test->waitUntilLoadFinished();

    GRefPtr<GtkPrinter> printer = adoptGRef(findPrintToFilePrinter());
    if (!printer) {
        g_message("%s", "Cannot test WebKitPrintOperation/print: no suitable printer found");
        return;
    }

    GUniquePtr<char> outputFilename(g_build_filename(Test::dataDirectory(), "webkit-print.pdf", nullptr));
    GRefPtr<GFile> outputFile = adoptGRef(g_file_new_for_path(outputFilename.get()));
    GUniquePtr<char> outputURI(g_file_get_uri(outputFile.get()));

    GRefPtr<GtkPrintSettings> printSettings = adoptGRef(gtk_print_settings_new());
    gtk_print_settings_set_printer(printSettings.get(), gtk_printer_get_name(printer.get()));
    gtk_print_settings_set(printSettings.get(), GTK_PRINT_SETTINGS_OUTPUT_URI, outputURI.get());

    webkit_print_operation_set_print_settings(test->m_printOperation.get(), printSettings.get());
    webkit_print_operation_print(test->m_printOperation.get());
    test->waitUntilPrintFinished();

    GRefPtr<GFileInfo> fileInfo = adoptGRef(g_file_query_info(outputFile.get(),
        G_FILE_ATTRIBUTE_STANDARD_SIZE "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
        static_cast<GFileQueryInfoFlags>(0), 0, 0));
    g_assert(fileInfo.get());
    g_assert_cmpint(g_file_info_get_size(fileInfo.get()), >, 0);
    g_assert_cmpstr(g_file_info_get_content_type(fileInfo.get()), ==, "application/pdf");

    g_file_delete(outputFile.get(), 0, 0);
}

static void testPrintOperationErrors(PrintTest* test, gconstpointer)
{
    test->loadHtml("<html><body>WebKitGTK+ printing errors test</body></html>", 0);
    test->waitUntilLoadFinished();

    GRefPtr<GtkPrinter> printer = adoptGRef(findPrintToFilePrinter());
    if (!printer) {
        g_message("%s", "Cannot test WebKitPrintOperation/print: no suitable printer found");
        return;
    }

    // General Error: invalid filename.
    test->m_expectedError = WEBKIT_PRINT_ERROR_GENERAL;
    GRefPtr<GtkPrintSettings> printSettings = adoptGRef(gtk_print_settings_new());
    gtk_print_settings_set_printer(printSettings.get(), gtk_printer_get_name(printer.get()));
    gtk_print_settings_set(printSettings.get(), GTK_PRINT_SETTINGS_OUTPUT_URI, "file:///foo/bar");
    webkit_print_operation_set_print_settings(test->m_printOperation.get(), printSettings.get());
    webkit_print_operation_print(test->m_printOperation.get());
    test->waitUntilPrintFinished();

    // Printer not found error.
    test->m_expectedError = WEBKIT_PRINT_ERROR_PRINTER_NOT_FOUND;
    gtk_print_settings_set_printer(printSettings.get(), "The fake WebKit printer");
    webkit_print_operation_print(test->m_printOperation.get());
    test->waitUntilPrintFinished();

    // No pages to print: print even pages for a single page document.
    test->m_expectedError = WEBKIT_PRINT_ERROR_INVALID_PAGE_RANGE;
    gtk_print_settings_set_printer(printSettings.get(), gtk_printer_get_name(printer.get()));
    gtk_print_settings_set_page_set(printSettings.get(), GTK_PAGE_SET_EVEN);
    webkit_print_operation_print(test->m_printOperation.get());
    test->waitUntilPrintFinished();
}

class CloseAfterPrintTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(CloseAfterPrintTest);

    static GtkWidget* webViewCreate(WebKitWebView* webView, WebKitNavigationAction*, CloseAfterPrintTest* test)
    {
        return test->createWebView();
    }

    static gboolean webViewPrint(WebKitWebView* webView, WebKitPrintOperation* printOperation, CloseAfterPrintTest* test)
    {
        test->print(printOperation);
        return TRUE;
    }

    static void printOperationFinished(WebKitPrintOperation* printOperation, CloseAfterPrintTest* test)
    {
        test->printFinished();
    }

    static void webViewClosed(WebKitWebView* webView, CloseAfterPrintTest* test)
    {
        gtk_widget_destroy(GTK_WIDGET(webView));
        test->m_webViewClosed = true;
        if (test->m_printFinished)
            g_main_loop_quit(test->m_mainLoop);
    }

    CloseAfterPrintTest()
        : m_webViewClosed(false)
        , m_printFinished(false)
    {
        webkit_settings_set_javascript_can_open_windows_automatically(webkit_web_view_get_settings(m_webView), TRUE);
        g_signal_connect(m_webView, "create", G_CALLBACK(webViewCreate), this);
    }

    GtkWidget* createWebView()
    {
        GtkWidget* newWebView = webkit_web_view_new_with_context(m_webContext.get());
        g_object_ref_sink(newWebView);

        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(newWebView));
        g_signal_connect(newWebView, "print", G_CALLBACK(webViewPrint), this);
        g_signal_connect(newWebView, "close", G_CALLBACK(webViewClosed), this);
        return newWebView;
    }

    void print(WebKitPrintOperation* printOperation)
    {
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(printOperation));

        GRefPtr<GtkPrinter> printer = adoptGRef(findPrintToFilePrinter());
        if (!printer) {
            g_message("%s", "Cannot test WebKitPrintOperation/print: no suitable printer found");
            return;
        }

        GUniquePtr<char> outputFilename(g_build_filename(Test::dataDirectory(), "webkit-close-after-print.pdf", nullptr));
        m_outputFile = adoptGRef(g_file_new_for_path(outputFilename.get()));
        GUniquePtr<char> outputURI(g_file_get_uri(m_outputFile.get()));

        GRefPtr<GtkPrintSettings> printSettings = adoptGRef(gtk_print_settings_new());
        gtk_print_settings_set_printer(printSettings.get(), gtk_printer_get_name(printer.get()));
        gtk_print_settings_set(printSettings.get(), GTK_PRINT_SETTINGS_OUTPUT_URI, outputURI.get());
        webkit_print_operation_set_print_settings(printOperation, printSettings.get());

        m_printOperation = printOperation;
        g_signal_connect(m_printOperation.get(), "finished", G_CALLBACK(printOperationFinished), this);
        webkit_print_operation_print(m_printOperation.get());
    }

    void printFinished()
    {
        m_printFinished = true;
        m_printOperation = nullptr;
        g_assert(m_outputFile);
        g_file_delete(m_outputFile.get(), 0, 0);
        m_outputFile = nullptr;
        if (m_webViewClosed)
            g_main_loop_quit(m_mainLoop);
    }

    void waitUntilPrintFinishedAndViewClosed()
    {
        g_main_loop_run(m_mainLoop);
    }

    GRefPtr<WebKitPrintOperation> m_printOperation;
    GRefPtr<GFile> m_outputFile;
    bool m_webViewClosed;
    bool m_printFinished;
};

static void testPrintOperationCloseAfterPrint(CloseAfterPrintTest* test, gconstpointer)
{
    test->loadHtml("<html><body onLoad=\"w = window.open();w.print();w.close();\"></body></html>", 0);
    test->waitUntilPrintFinishedAndViewClosed();
}
#endif // HAVE_GTK_UNIX_PRINTING

void beforeAll()
{
    WebViewTest::add("WebKitPrintOperation", "printing-settings", testPrintOperationPrintSettings);
    WebViewTest::add("WebKitWebView", "print", testWebViewPrint);
#ifdef HAVE_GTK_UNIX_PRINTING
    PrintTest::add("WebKitPrintOperation", "print", testPrintOperationPrint);
    PrintTest::add("WebKitPrintOperation", "print-errors", testPrintOperationErrors);
    CloseAfterPrintTest::add("WebKitPrintOperation", "close-after-print", testPrintOperationCloseAfterPrint);
#endif
}

void afterAll()
{
}
