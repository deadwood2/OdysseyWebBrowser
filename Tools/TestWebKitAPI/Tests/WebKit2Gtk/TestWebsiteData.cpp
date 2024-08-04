/*
 * Copyright (C) 2017 Igalia S.L.
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

static WebKitTestServer* kServer;

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_equal(path, "/empty")) {
        const char* emptyHTML = "<html><body></body></html>";
        soup_message_headers_replace(message->response_headers, "Set-Cookie", "foo=bar; Max-Age=60");
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, emptyHTML, strlen(emptyHTML));
        soup_message_body_complete(message->response_body);
        soup_message_set_status(message, SOUP_STATUS_OK);
    } else if (g_str_equal(path, "/appcache")) {
        const char* appcacheHTML = "<html manifest=appcache.manifest><body></body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, appcacheHTML, strlen(appcacheHTML));
        soup_message_body_complete(message->response_body);
        soup_message_set_status(message, SOUP_STATUS_OK);
    } else if (g_str_equal(path, "/appcache.manifest")) {
        const char* appcacheManifest = "CACHE MANIFEST\nCACHE:\nappcache/foo.txt\n";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, appcacheManifest, strlen(appcacheManifest));
        soup_message_body_complete(message->response_body);
        soup_message_set_status(message, SOUP_STATUS_OK);
    } else if (g_str_equal(path, "/appcache/foo.txt")) {
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, "foo", 3);
        soup_message_body_complete(message->response_body);
        soup_message_set_status(message, SOUP_STATUS_OK);
    } else if (g_str_equal(path, "/sessionstorage")) {
        const char* sessionStorageHTML = "<html><body onload=\"sessionStorage.foo = 'bar';\"></body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, sessionStorageHTML, strlen(sessionStorageHTML));
        soup_message_body_complete(message->response_body);
        soup_message_set_status(message, SOUP_STATUS_OK);
    } else if (g_str_equal(path, "/localstorage")) {
        const char* localStorageHTML = "<html><body onload=\"localStorage.foo = 'bar';\"></body></html>";
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, localStorageHTML, strlen(localStorageHTML));
        soup_message_body_complete(message->response_body);
        soup_message_set_status(message, SOUP_STATUS_OK);
    } else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
}

class WebsiteDataTest : public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(WebsiteDataTest);


    WebsiteDataTest()
        : m_manager(webkit_web_context_get_website_data_manager(webkit_web_view_get_context(m_webView)))
    {
        g_assert(WEBKIT_IS_WEBSITE_DATA_MANAGER(m_manager));
        assertObjectIsDeletedWhenTestFinishes(G_OBJECT(m_manager));
        // WebsiteDataStore creates a new WebProcessPool when used before any secondary process has been created.
        // Ensure we have a web process by always loading about:blank here.
        loadURI("about:blank");
        waitUntilLoadFinished();
    }

    ~WebsiteDataTest()
    {
        g_list_free_full(m_dataList, reinterpret_cast<GDestroyNotify>(webkit_website_data_unref));
    }

    GList* fetch(WebKitWebsiteDataTypes types)
    {
        if (m_dataList) {
            g_list_free_full(m_dataList, reinterpret_cast<GDestroyNotify>(webkit_website_data_unref));
            m_dataList = nullptr;
        }
        webkit_website_data_manager_fetch(m_manager, types, nullptr, [](GObject*, GAsyncResult* result, gpointer userData) {
            WebsiteDataTest* test = static_cast<WebsiteDataTest*>(userData);
            test->m_dataList = webkit_website_data_manager_fetch_finish(test->m_manager, result, nullptr);
            test->quitMainLoop();
        }, this);
        g_main_loop_run(m_mainLoop);
        return m_dataList;
    }

    void remove(WebKitWebsiteDataTypes types, GList* dataList)
    {
        webkit_website_data_manager_remove(m_manager, types, dataList, nullptr, [](GObject*, GAsyncResult* result, gpointer userData) {
            WebsiteDataTest* test = static_cast<WebsiteDataTest*>(userData);
            g_assert(webkit_website_data_manager_remove_finish(test->m_manager, result, nullptr));
            test->quitMainLoop();
        }, this);
        g_main_loop_run(m_mainLoop);
    }

    void clear(WebKitWebsiteDataTypes types, GTimeSpan timeSpan)
    {
        webkit_website_data_manager_clear(m_manager, types, timeSpan, nullptr, [](GObject*, GAsyncResult* result, gpointer userData) {
            WebsiteDataTest* test = static_cast<WebsiteDataTest*>(userData);
            g_assert(webkit_website_data_manager_clear_finish(test->m_manager, result, nullptr));
            test->quitMainLoop();
        }, this);
        g_main_loop_run(m_mainLoop);
    }

    WebKitWebsiteDataManager* m_manager;
    GList* m_dataList { nullptr };
};

static void testWebsiteDataConfiguration(WebsiteDataTest* test, gconstpointer)
{
    // Base directories are not used by TestMain.
    g_assert(!webkit_website_data_manager_get_base_data_directory(test->m_manager));
    g_assert(!webkit_website_data_manager_get_base_cache_directory(test->m_manager));

    GUniquePtr<char> localStorageDirectory(g_build_filename(Test::dataDirectory(), "local-storage", nullptr));
    g_assert_cmpstr(localStorageDirectory.get(), ==, webkit_website_data_manager_get_local_storage_directory(test->m_manager));
    g_assert(g_file_test(localStorageDirectory.get(), G_FILE_TEST_IS_DIR));

    test->loadURI(kServer->getURIForPath("/empty").data());
    test->waitUntilLoadFinished();
    test->runJavaScriptAndWaitUntilFinished("window.indexedDB.open('TestDatabase');", nullptr);
    GUniquePtr<char> indexedDBDirectory(g_build_filename(Test::dataDirectory(), "indexeddb", nullptr));
    g_assert_cmpstr(indexedDBDirectory.get(), ==, webkit_website_data_manager_get_indexeddb_directory(test->m_manager));
    g_assert(g_file_test(indexedDBDirectory.get(), G_FILE_TEST_IS_DIR));

    GUniquePtr<char> webSQLDirectory(g_build_filename(Test::dataDirectory(), "websql", nullptr));
    g_assert_cmpstr(webSQLDirectory.get(), ==, webkit_website_data_manager_get_websql_directory(test->m_manager));
    test->runJavaScriptAndWaitUntilFinished("db = openDatabase(\"TestDatabase\", \"1.0\", \"TestDatabase\", 1);", nullptr);
    g_assert(g_file_test(webSQLDirectory.get(), G_FILE_TEST_IS_DIR));

    test->loadURI(kServer->getURIForPath("/appcache").data());
    test->waitUntilLoadFinished();
    GUniquePtr<char> applicationCacheDirectory(g_build_filename(Test::dataDirectory(), "appcache", nullptr));
    g_assert_cmpstr(applicationCacheDirectory.get(), ==, webkit_website_data_manager_get_offline_application_cache_directory(test->m_manager));
    GUniquePtr<char> applicationCacheDatabase(g_build_filename(applicationCacheDirectory.get(), "ApplicationCache.db", nullptr));
    unsigned triesCount = 4;
    while (!g_file_test(applicationCacheDatabase.get(), G_FILE_TEST_IS_REGULAR) && --triesCount)
        test->wait(0.25);
    g_assert(triesCount);

    GUniquePtr<char> diskCacheDirectory(g_build_filename(Test::dataDirectory(), "disk-cache", nullptr));
    g_assert_cmpstr(diskCacheDirectory.get(), ==, webkit_website_data_manager_get_disk_cache_directory(test->m_manager));
    g_assert(g_file_test(diskCacheDirectory.get(), G_FILE_TEST_IS_DIR));

    // Clear all persistent caches, since the data dir is common to all test cases.
    static const WebKitWebsiteDataTypes persistentCaches = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_DISK_CACHE | WEBKIT_WEBSITE_DATA_LOCAL_STORAGE
        | WEBKIT_WEBSITE_DATA_WEBSQL_DATABASES | WEBKIT_WEBSITE_DATA_INDEXEDDB_DATABASES | WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE);
    test->clear(persistentCaches, 0);
    g_assert(!test->fetch(persistentCaches));

    // The default context should have a different manager with different configuration.
    WebKitWebsiteDataManager* defaultManager = webkit_web_context_get_website_data_manager(webkit_web_context_get_default());
    g_assert(WEBKIT_IS_WEBSITE_DATA_MANAGER(defaultManager));
    g_assert(test->m_manager != defaultManager);
    g_assert_cmpstr(webkit_website_data_manager_get_local_storage_directory(test->m_manager), !=, webkit_website_data_manager_get_local_storage_directory(defaultManager));
    g_assert_cmpstr(webkit_website_data_manager_get_indexeddb_directory(test->m_manager), !=, webkit_website_data_manager_get_indexeddb_directory(defaultManager));
    g_assert_cmpstr(webkit_website_data_manager_get_disk_cache_directory(test->m_manager), !=, webkit_website_data_manager_get_disk_cache_directory(defaultManager));
    g_assert_cmpstr(webkit_website_data_manager_get_offline_application_cache_directory(test->m_manager), !=, webkit_website_data_manager_get_offline_application_cache_directory(defaultManager));
    g_assert_cmpstr(webkit_website_data_manager_get_websql_directory(test->m_manager), !=, webkit_website_data_manager_get_websql_directory(defaultManager));

    // Using Test::dataDirectory() we get the default configuration but for a differrent prefix.
    GRefPtr<WebKitWebsiteDataManager> baseDataManager = adoptGRef(webkit_website_data_manager_new("base-data-directory", Test::dataDirectory(), "base-cache-directory", Test::dataDirectory(), nullptr));
    g_assert(WEBKIT_IS_WEBSITE_DATA_MANAGER(baseDataManager.get()));

    localStorageDirectory.reset(g_build_filename(Test::dataDirectory(), "localstorage", nullptr));
    g_assert_cmpstr(webkit_website_data_manager_get_local_storage_directory(baseDataManager.get()), ==, localStorageDirectory.get());

    indexedDBDirectory.reset(g_build_filename(Test::dataDirectory(), "databases", "indexeddb", nullptr));
    g_assert_cmpstr(webkit_website_data_manager_get_indexeddb_directory(baseDataManager.get()), ==, indexedDBDirectory.get());

    applicationCacheDirectory.reset(g_build_filename(Test::dataDirectory(), "applications", nullptr));
    g_assert_cmpstr(webkit_website_data_manager_get_offline_application_cache_directory(baseDataManager.get()), ==, applicationCacheDirectory.get());

    webSQLDirectory.reset(g_build_filename(Test::dataDirectory(), "databases", nullptr));
    g_assert_cmpstr(webkit_website_data_manager_get_websql_directory(baseDataManager.get()), ==, webSQLDirectory.get());

    g_assert_cmpstr(webkit_website_data_manager_get_disk_cache_directory(baseDataManager.get()), ==, Test::dataDirectory());

    // Any specific configuration provided takes precedence over base dirs.
    indexedDBDirectory.reset(g_build_filename(Test::dataDirectory(), "mycustomindexeddb", nullptr));
    applicationCacheDirectory.reset(g_build_filename(Test::dataDirectory(), "mycustomappcache", nullptr));
    baseDataManager = adoptGRef(webkit_website_data_manager_new("base-data-directory", Test::dataDirectory(), "base-cache-directory", Test::dataDirectory(),
        "indexeddb-directory", indexedDBDirectory.get(), "offline-application-cache-directory", applicationCacheDirectory.get(), nullptr));
    g_assert_cmpstr(webkit_website_data_manager_get_indexeddb_directory(baseDataManager.get()), ==, indexedDBDirectory.get());
    g_assert_cmpstr(webkit_website_data_manager_get_offline_application_cache_directory(baseDataManager.get()), ==, applicationCacheDirectory.get());
    // The result should be the same as previous manager.
    g_assert_cmpstr(webkit_website_data_manager_get_local_storage_directory(baseDataManager.get()), ==, localStorageDirectory.get());
    g_assert_cmpstr(webkit_website_data_manager_get_websql_directory(baseDataManager.get()), ==, webSQLDirectory.get());
    g_assert_cmpstr(webkit_website_data_manager_get_disk_cache_directory(baseDataManager.get()), ==, Test::dataDirectory());
}

static void ephemeralViewloadChanged(WebKitWebView* webView, WebKitLoadEvent loadEvent, WebViewTest* test)
{
    if (loadEvent != WEBKIT_LOAD_FINISHED)
        return;
    g_signal_handlers_disconnect_by_func(webView, reinterpret_cast<void*>(ephemeralViewloadChanged), test);
    test->quitMainLoop();
}

static void testWebsiteDataEphemeral(WebViewTest* test, gconstpointer)
{
    GRefPtr<WebKitWebsiteDataManager> manager = adoptGRef(webkit_website_data_manager_new_ephemeral());
    g_assert(webkit_website_data_manager_is_ephemeral(manager.get()));
    g_assert(!webkit_website_data_manager_get_base_data_directory(manager.get()));
    g_assert(!webkit_website_data_manager_get_base_cache_directory(manager.get()));
    g_assert(!webkit_website_data_manager_get_local_storage_directory(manager.get()));
    g_assert(!webkit_website_data_manager_get_disk_cache_directory(manager.get()));
    g_assert(!webkit_website_data_manager_get_offline_application_cache_directory(manager.get()));
    g_assert(!webkit_website_data_manager_get_indexeddb_directory(manager.get()));
    g_assert(!webkit_website_data_manager_get_websql_directory(manager.get()));

    // Configuration is ignored when is-ephemeral is used.
    manager = adoptGRef(WEBKIT_WEBSITE_DATA_MANAGER(g_object_new(WEBKIT_TYPE_WEBSITE_DATA_MANAGER, "base-data-directory", Test::dataDirectory(), "is-ephemeral", TRUE, nullptr)));
    g_assert(webkit_website_data_manager_is_ephemeral(manager.get()));
    g_assert(!webkit_website_data_manager_get_base_data_directory(manager.get()));

    // Non persistent data can be queried in an ephemeral manager.
    GRefPtr<WebKitWebContext> webContext = adoptGRef(webkit_web_context_new_with_website_data_manager(manager.get()));
    g_assert(webkit_web_context_is_ephemeral(webContext.get()));
    GRefPtr<WebKitWebView> webView = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(webContext.get()));
    g_assert(webkit_web_view_is_ephemeral(webView.get()));
    g_assert(webkit_web_view_get_website_data_manager(webView.get()) == manager.get());

    g_signal_connect(webView.get(), "load-changed", G_CALLBACK(ephemeralViewloadChanged), test);
    webkit_web_view_load_uri(webView.get(), kServer->getURIForPath("/empty").data());
    g_main_loop_run(test->m_mainLoop);

    webkit_website_data_manager_fetch(manager.get(), WEBKIT_WEBSITE_DATA_MEMORY_CACHE, nullptr, [](GObject* manager, GAsyncResult* result, gpointer userData) {
        auto* test = static_cast<WebViewTest*>(userData);
        GList* dataList = webkit_website_data_manager_fetch_finish(WEBKIT_WEBSITE_DATA_MANAGER(manager), result, nullptr);
        g_assert(dataList);
        g_assert_cmpuint(g_list_length(dataList), ==, 1);
        WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(dataList->data);
        g_assert(data);
        WebKitSecurityOrigin* origin = webkit_security_origin_new_for_uri(kServer->getURIForPath("/").data());
        g_assert_cmpstr(webkit_website_data_get_name(data), ==, webkit_security_origin_get_host(origin));
        webkit_security_origin_unref(origin);
        g_list_free_full(dataList, reinterpret_cast<GDestroyNotify>(webkit_website_data_unref));
        test->quitMainLoop();
    }, test);
    g_main_loop_run(test->m_mainLoop);
}

static void testWebsiteDataCache(WebsiteDataTest* test, gconstpointer)
{
    static const WebKitWebsiteDataTypes cacheTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_DISK_CACHE);
    GList* dataList = test->fetch(cacheTypes);
    g_assert(!dataList);

    test->loadURI(kServer->getURIForPath("/empty").data());
    test->waitUntilLoadFinished();

    // Disk cache delays the storing of initial resources for 1 second to avoid
    // affecting early page load. So, wait 1 second here to make sure resources
    // have already been stored.
    test->wait(1);

    dataList = test->fetch(cacheTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    WebKitSecurityOrigin* origin = webkit_security_origin_new_for_uri(kServer->getURIForPath("/").data());
    g_assert_cmpstr(webkit_website_data_get_name(data), ==, webkit_security_origin_get_host(origin));
    webkit_security_origin_unref(origin);
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, cacheTypes);
    // Memory cache size is unknown.
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_MEMORY_CACHE), ==, 0);
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_DISK_CACHE), >, 0);

    // Try again but only getting disk cache.
    dataList = test->fetch(WEBKIT_WEBSITE_DATA_DISK_CACHE);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    g_assert(webkit_website_data_get_types(data) & WEBKIT_WEBSITE_DATA_DISK_CACHE);
    g_assert(!(webkit_website_data_get_types(data) & WEBKIT_WEBSITE_DATA_MEMORY_CACHE));

    GUniquePtr<char> fileURL(g_strdup_printf("file://%s/simple.html", Test::getResourcesDir(Test::WebKit2Resources).data()));
    test->loadURI(fileURL.get());
    test->waitUntilLoadFinished();

    fileURL.reset(g_strdup_printf("file://%s/simple2.html", Test::getResourcesDir(Test::WebKit2Resources).data()));
    test->loadURI(fileURL.get());
    test->waitUntilLoadFinished();

    // Local files are grouped.
    dataList = test->fetch(cacheTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 2);
    GList* itemList = g_list_find_custom(dataList, nullptr, [](gconstpointer item, gconstpointer) -> int {
        WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(const_cast<gpointer>(item));
        return g_strcmp0(webkit_website_data_get_name(data), "Local files");
    });
    g_assert(itemList);
    data = static_cast<WebKitWebsiteData*>(itemList->data);
    g_assert(data);
    g_assert(webkit_website_data_get_types(data) & WEBKIT_WEBSITE_DATA_MEMORY_CACHE);
    // Local files are never stored in disk cache.
    g_assert(!(webkit_website_data_get_types(data) & WEBKIT_WEBSITE_DATA_DISK_CACHE));

    // Clear data modified since the last microsecond should not clear anything.
    // Use disk-cache because memory cache ignores the modified since.
    test->clear(WEBKIT_WEBSITE_DATA_DISK_CACHE, 1);
    dataList = test->fetch(cacheTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 2);

    // Remove memory cache only for local files.
    itemList = g_list_find_custom(dataList, nullptr, [](gconstpointer item, gconstpointer) -> int {
        WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(const_cast<gpointer>(item));
        return g_strcmp0(webkit_website_data_get_name(data), "Local files");
    });
    g_assert(itemList);
    GList removeList = { itemList->data, nullptr, nullptr };
    test->remove(WEBKIT_WEBSITE_DATA_MEMORY_CACHE, &removeList);
    dataList = test->fetch(cacheTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(webkit_website_data_get_types(data) & WEBKIT_WEBSITE_DATA_DISK_CACHE);

    // Clear all.
    test->clear(cacheTypes, 0);
    dataList = test->fetch(cacheTypes);
    g_assert(!dataList);
}

static void testWebsiteDataStorage(WebsiteDataTest* test, gconstpointer)
{
    static const WebKitWebsiteDataTypes storageTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_SESSION_STORAGE | WEBKIT_WEBSITE_DATA_LOCAL_STORAGE);
    GList* dataList = test->fetch(storageTypes);
    g_assert(!dataList);

    test->loadURI(kServer->getURIForPath("/sessionstorage").data());
    test->waitUntilLoadFinished();

    test->loadURI(kServer->getURIForPath("/localstorage").data());
    test->waitUntilLoadFinished();

    // Local storage uses a 1 second timer to update the database.
    test->wait(1);

    dataList = test->fetch(storageTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    WebKitSecurityOrigin* origin = webkit_security_origin_new_for_uri(kServer->getURIForPath("/").data());
    g_assert_cmpstr(webkit_website_data_get_name(data), ==, webkit_security_origin_get_host(origin));
    webkit_security_origin_unref(origin);
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, storageTypes);
    // Storage sizes are unknown.
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_SESSION_STORAGE), ==, 0);
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_LOCAL_STORAGE), ==, 0);

    // Get also cached data, and clear it.
    static const WebKitWebsiteDataTypes cacheAndStorageTypes = static_cast<WebKitWebsiteDataTypes>(storageTypes | WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_DISK_CACHE);
    dataList = test->fetch(cacheAndStorageTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, cacheAndStorageTypes);
    test->clear(static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_DISK_CACHE), 0);

    // Get all types again, but only storage is retrieved now.
    dataList = test->fetch(cacheAndStorageTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, storageTypes);

    // Remove the session storage.
    GList removeList = { data, nullptr, nullptr };
    test->remove(WEBKIT_WEBSITE_DATA_SESSION_STORAGE, &removeList);
    dataList = test->fetch(storageTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(!(webkit_website_data_get_types(data) & WEBKIT_WEBSITE_DATA_SESSION_STORAGE));
    g_assert(webkit_website_data_get_types(data) & WEBKIT_WEBSITE_DATA_LOCAL_STORAGE);

    // Clear all.
    test->clear(cacheAndStorageTypes, 0);
    dataList = test->fetch(cacheAndStorageTypes);
    g_assert(!dataList);
}

static void testWebsiteDataDatabases(WebsiteDataTest* test, gconstpointer)
{
    static const WebKitWebsiteDataTypes databaseTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_WEBSQL_DATABASES | WEBKIT_WEBSITE_DATA_INDEXEDDB_DATABASES);
    GList* dataList = test->fetch(databaseTypes);
    g_assert(!dataList);

    test->loadURI(kServer->getURIForPath("/empty").data());
    test->waitUntilLoadFinished();
    test->runJavaScriptAndWaitUntilFinished("window.indexedDB.open('TestDatabase');", nullptr);

    dataList = test->fetch(databaseTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    WebKitSecurityOrigin* origin = webkit_security_origin_new_for_uri(kServer->getURIForPath("/").data());
    g_assert_cmpstr(webkit_website_data_get_name(data), ==, webkit_security_origin_get_host(origin));
    webkit_security_origin_unref(origin);
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, WEBKIT_WEBSITE_DATA_INDEXEDDB_DATABASES);
    // Database sizes are unknown.
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_INDEXEDDB_DATABASES), ==, 0);

    test->runJavaScriptAndWaitUntilFinished("db = openDatabase(\"TestDatabase\", \"1.0\", \"TestDatabase\", 1);", nullptr);
    dataList = test->fetch(databaseTypes);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, databaseTypes);
    // Database sizes are unknown.
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_INDEXEDDB_DATABASES), ==, 0);
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_WEBSQL_DATABASES), ==, 0);

    // Remove all databases at once.
    GList removeList = { data, nullptr, nullptr };
    test->remove(databaseTypes, &removeList);
    dataList = test->fetch(databaseTypes);
    g_assert(!dataList);

    // Clear all.
    static const WebKitWebsiteDataTypes cacheAndDatabaseTypes = static_cast<WebKitWebsiteDataTypes>(databaseTypes | WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_DISK_CACHE);
    test->clear(cacheAndDatabaseTypes, 0);
    dataList = test->fetch(cacheAndDatabaseTypes);
    g_assert(!dataList);
}

static void testWebsiteDataAppcache(WebsiteDataTest* test, gconstpointer)
{
    GList* dataList = test->fetch(WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE);
    g_assert(!dataList);

    test->loadURI(kServer->getURIForPath("/appcache").data());
    test->waitUntilLoadFinished();

    test->wait(1);
    dataList = test->fetch(WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    WebKitSecurityOrigin* origin = webkit_security_origin_new_for_uri(kServer->getURIForPath("/").data());
    g_assert_cmpstr(webkit_website_data_get_name(data), ==, webkit_security_origin_get_host(origin));
    webkit_security_origin_unref(origin);
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE);
    // Appcache size is unknown.
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE), ==, 0);

    GList removeList = { data, nullptr, nullptr };
    test->remove(WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE, &removeList);
    dataList = test->fetch(WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE);
    g_assert(!dataList);

    // Clear all.
    static const WebKitWebsiteDataTypes cacheAndAppcacheTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE | WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_DISK_CACHE);
    test->clear(cacheAndAppcacheTypes, 0);
    dataList = test->fetch(cacheAndAppcacheTypes);
    g_assert(!dataList);
}

static void testWebsiteDataCookies(WebsiteDataTest* test, gconstpointer)
{
    GList* dataList = test->fetch(WEBKIT_WEBSITE_DATA_COOKIES);
    g_assert(!dataList);

    test->loadURI(kServer->getURIForPath("/empty").data());
    test->waitUntilLoadFinished();

    dataList = test->fetch(WEBKIT_WEBSITE_DATA_COOKIES);
    g_assert(dataList);
    g_assert_cmpuint(g_list_length(dataList), ==, 1);
    WebKitWebsiteData* data = static_cast<WebKitWebsiteData*>(dataList->data);
    g_assert(data);
    g_assert_cmpstr(webkit_website_data_get_name(data), ==, "127.0.0.1");
    g_assert_cmpuint(webkit_website_data_get_types(data), ==, WEBKIT_WEBSITE_DATA_COOKIES);
    // Cookies size is unknown.
    g_assert_cmpuint(webkit_website_data_get_size(data, WEBKIT_WEBSITE_DATA_COOKIES), ==, 0);

    GList removeList = { data, nullptr, nullptr };
    test->remove(WEBKIT_WEBSITE_DATA_COOKIES, &removeList);
    dataList = test->fetch(WEBKIT_WEBSITE_DATA_COOKIES);
    g_assert(!dataList);

    // Clear all.
    static const WebKitWebsiteDataTypes cacheAndCookieTypes = static_cast<WebKitWebsiteDataTypes>(WEBKIT_WEBSITE_DATA_COOKIES | WEBKIT_WEBSITE_DATA_MEMORY_CACHE | WEBKIT_WEBSITE_DATA_DISK_CACHE);
    test->clear(cacheAndCookieTypes, 0);
    dataList = test->fetch(cacheAndCookieTypes);
    g_assert(!dataList);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    WebsiteDataTest::add("WebKitWebsiteData", "configuration", testWebsiteDataConfiguration);
    WebViewTest::add("WebKitWebsiteData", "ephemeral", testWebsiteDataEphemeral);
    WebsiteDataTest::add("WebKitWebsiteData", "cache", testWebsiteDataCache);
    WebsiteDataTest::add("WebKitWebsiteData", "storage", testWebsiteDataStorage);
    WebsiteDataTest::add("WebKitWebsiteData", "databases", testWebsiteDataDatabases);
    WebsiteDataTest::add("WebKitWebsiteData", "appcache", testWebsiteDataAppcache);
    WebsiteDataTest::add("WebKitWebsiteData", "cookies", testWebsiteDataCookies);
}

void afterAll()
{
    delete kServer;
}
