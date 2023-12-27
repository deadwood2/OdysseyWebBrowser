/*
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef TestMain_h
#define TestMain_h

#include <cairo.h>
#include <glib-object.h>
#include <webkit2/webkit2.h>
#include <wtf/HashSet.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>

#define MAKE_GLIB_TEST_FIXTURE(ClassName) \
    static void setUp(ClassName* fixture, gconstpointer data) \
    { \
        new (fixture) ClassName; \
    } \
    static void tearDown(ClassName* fixture, gconstpointer data) \
    { \
        fixture->~ClassName(); \
    } \
    static void add(const char* suiteName, const char* testName, void (*testFunc)(ClassName*, const void*)) \
    { \
        GUniquePtr<gchar> testPath(g_strdup_printf("/webkit2/%s/%s", suiteName, testName)); \
        g_test_add(testPath.get(), ClassName, 0, ClassName::setUp, testFunc, ClassName::tearDown); \
    }

#define ASSERT_CMP_CSTRING(s1, cmp, s2) \
    do {                                                                 \
        CString __s1 = (s1);                                             \
        CString __s2 = (s2);                                             \
        if (g_strcmp0(__s1.data(), __s2.data()) cmp 0) ;                 \
        else                                                             \
            g_assertion_message_cmpstr(G_LOG_DOMAIN, __FILE__, __LINE__, \
                G_STRFUNC, #s1 " " #cmp " " #s2, __s1.data(), #cmp, __s2.data()); \
    } while (0)


class Test {
public:
    MAKE_GLIB_TEST_FIXTURE(Test);

    static const char* dataDirectory();

    static void initializeWebExtensionsCallback(WebKitWebContext* context, Test* test)
    {
        test->initializeWebExtensions();
    }

    Test()
    {
        GUniquePtr<char> localStorageDirectory(g_build_filename(dataDirectory(), "local-storage", nullptr));
        GUniquePtr<char> indexedDBDirectory(g_build_filename(dataDirectory(), "indexeddb", nullptr));
        GUniquePtr<char> diskCacheDirectory(g_build_filename(dataDirectory(), "disk-cache", nullptr));
        GUniquePtr<char> applicationCacheDirectory(g_build_filename(dataDirectory(), "appcache", nullptr));
        GUniquePtr<char> webSQLDirectory(g_build_filename(dataDirectory(), "websql", nullptr));
        GRefPtr<WebKitWebsiteDataManager> websiteDataManager = adoptGRef(webkit_website_data_manager_new(
            "local-storage-directory", localStorageDirectory.get(), "indexeddb-directory", indexedDBDirectory.get(),
            "disk-cache-directory", diskCacheDirectory.get(), "offline-application-cache-directory", applicationCacheDirectory.get(),
            "websql-directory", webSQLDirectory.get(), nullptr));

        m_webContext = adoptGRef(webkit_web_context_new_with_website_data_manager(websiteDataManager.get()));
        g_signal_connect(m_webContext.get(), "initialize-web-extensions", G_CALLBACK(initializeWebExtensionsCallback), this);
    }

    ~Test()
    {
        g_signal_handlers_disconnect_matched(m_webContext.get(), G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
        m_webContext = nullptr;
        if (m_watchedObjects.isEmpty())
            return;

        g_print("Leaked objects:");
        HashSet<GObject*>::const_iterator end = m_watchedObjects.end();
        for (HashSet<GObject*>::const_iterator it = m_watchedObjects.begin(); it != end; ++it)
            g_print(" %s(%p)", g_type_name_from_instance(reinterpret_cast<GTypeInstance*>(*it)), *it);
        g_print("\n");

        g_assert(m_watchedObjects.isEmpty());
    }

    virtual void initializeWebExtensions()
    {
        webkit_web_context_set_web_extensions_directory(m_webContext.get(), WEBKIT_TEST_WEB_EXTENSIONS_DIR);
        webkit_web_context_set_web_extensions_initialization_user_data(m_webContext.get(), g_variant_new_uint32(++s_webExtensionID));
    }

    static void objectFinalized(Test* test, GObject* finalizedObject)
    {
        test->m_watchedObjects.remove(finalizedObject);
    }

    void assertObjectIsDeletedWhenTestFinishes(GObject* object)
    {
        m_watchedObjects.add(object);
        g_object_weak_ref(object, reinterpret_cast<GWeakNotify>(objectFinalized), this);
    }


    enum ResourcesDir {
        WebKit2GTKResources,
        WebKit2Resources,
    };

    static CString getResourcesDir(ResourcesDir resourcesDir = WebKit2GTKResources)
    {
        switch (resourcesDir) {
        case WebKit2GTKResources: {
            GUniquePtr<char> resourcesDir(g_build_filename(WEBKIT_SRC_DIR, "Tools", "TestWebKitAPI", "Tests", "WebKit2Gtk", "resources", nullptr));
            return resourcesDir.get();
        }
        case WebKit2Resources: {
            GUniquePtr<char> resourcesDir(g_build_filename(WEBKIT_SRC_DIR, "Tools", "TestWebKitAPI", "Tests", "WebKit2", nullptr));
            return resourcesDir.get();
        }
        }
    }

    void addLogFatalFlag(unsigned flag)
    {
        unsigned fatalMask = g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK));
        fatalMask |= flag;
        g_log_set_always_fatal(static_cast<GLogLevelFlags>(fatalMask));
    }

    void removeLogFatalFlag(unsigned flag)
    {
        unsigned fatalMask = g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK));
        fatalMask &= ~flag;
        g_log_set_always_fatal(static_cast<GLogLevelFlags>(fatalMask));
    }

    static bool cairoSurfacesEqual(cairo_surface_t* s1, cairo_surface_t* s2)
    {
        return (cairo_image_surface_get_format(s1) == cairo_image_surface_get_format(s2)
            && cairo_image_surface_get_width(s1) == cairo_image_surface_get_width(s2)
            && cairo_image_surface_get_height(s1) == cairo_image_surface_get_height(s2)
            && cairo_image_surface_get_stride(s1) == cairo_image_surface_get_stride(s2)
            && !memcmp(const_cast<const void*>(reinterpret_cast<void*>(cairo_image_surface_get_data(s1))),
                const_cast<const void*>(reinterpret_cast<void*>(cairo_image_surface_get_data(s2))),
                cairo_image_surface_get_height(s1)*cairo_image_surface_get_stride(s1)));
    }

    HashSet<GObject*> m_watchedObjects;
    GRefPtr<WebKitWebContext> m_webContext;
    static uint32_t s_webExtensionID;
};

#endif // TestMain_h
