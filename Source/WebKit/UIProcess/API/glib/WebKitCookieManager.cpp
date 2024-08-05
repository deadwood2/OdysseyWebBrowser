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
#include "WebKitCookieManager.h"

#include "SoupCookiePersistentStorageType.h"
#include "WebCookieManagerProxy.h"
#include "WebKitCookieManagerPrivate.h"
#include "WebKitEnumTypes.h"
#include "WebKitWebsiteDataManagerPrivate.h"
#include "WebKitWebsiteDataPrivate.h"
#include "WebsiteDataRecord.h"
#include <WebCore/SessionID.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/WTFGType.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitCookieManager
 * @Short_description: Defines how to handle cookies in a #WebKitWebContext
 * @Title: WebKitCookieManager
 *
 * The WebKitCookieManager defines how to set up and handle cookies.
 * You can get it from a #WebKitWebsiteDataManager with
 * webkit_website_data_manager_get_cookie_manager(), and use it to set where to
 * store cookies with webkit_cookie_manager_set_persistent_storage(),
 * or to set the acceptance policy, with webkit_cookie_manager_get_accept_policy().
 */

enum {
    CHANGED,

    LAST_SIGNAL
};

struct _WebKitCookieManagerPrivate {
    ~_WebKitCookieManagerPrivate()
    {
        auto sessionID = webkitWebsiteDataManagerGetDataStore(dataManager).websiteDataStore().sessionID();
        for (auto* processPool : webkitWebsiteDataManagerGetProcessPools(dataManager))
            processPool->supplement<WebCookieManagerProxy>()->setCookieObserverCallback(sessionID, nullptr);
    }

    WebKitWebsiteDataManager* dataManager;
};

static guint signals[LAST_SIGNAL] = { 0, };

WEBKIT_DEFINE_TYPE(WebKitCookieManager, webkit_cookie_manager, G_TYPE_OBJECT)

static inline SoupCookiePersistentStorageType toSoupCookiePersistentStorageType(WebKitCookiePersistentStorage kitStorage)
{
    switch (kitStorage) {
    case WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT:
        return SoupCookiePersistentStorageText;
    case WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE:
        return SoupCookiePersistentStorageSQLite;
    default:
        ASSERT_NOT_REACHED();
        return SoupCookiePersistentStorageText;
    }
}

static inline WebKitCookieAcceptPolicy toWebKitCookieAcceptPolicy(HTTPCookieAcceptPolicy httpPolicy)
{
    switch (httpPolicy) {
    case HTTPCookieAcceptPolicyAlways:
        return WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS;
    case HTTPCookieAcceptPolicyNever:
        return WEBKIT_COOKIE_POLICY_ACCEPT_NEVER;
    case HTTPCookieAcceptPolicyOnlyFromMainDocumentDomain:
        return WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY;
    default:
        ASSERT_NOT_REACHED();
        return WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS;
    }
}

static inline HTTPCookieAcceptPolicy toHTTPCookieAcceptPolicy(WebKitCookieAcceptPolicy kitPolicy)
{
    switch (kitPolicy) {
    case WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS:
        return HTTPCookieAcceptPolicyAlways;
    case WEBKIT_COOKIE_POLICY_ACCEPT_NEVER:
        return HTTPCookieAcceptPolicyNever;
    case WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY:
        return HTTPCookieAcceptPolicyOnlyFromMainDocumentDomain;
    default:
        ASSERT_NOT_REACHED();
        return HTTPCookieAcceptPolicyAlways;
    }
}

static void webkit_cookie_manager_class_init(WebKitCookieManagerClass* findClass)
{
    GObjectClass* gObjectClass = G_OBJECT_CLASS(findClass);

    /**
     * WebKitCookieManager::changed:
     * @cookie_manager: the #WebKitCookieManager
     *
     * This signal is emitted when cookies are added, removed or modified.
     */
    signals[CHANGED] =
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(gObjectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);
}

WebKitCookieManager* webkitCookieManagerCreate(WebKitWebsiteDataManager* dataManager)
{
    WebKitCookieManager* manager = WEBKIT_COOKIE_MANAGER(g_object_new(WEBKIT_TYPE_COOKIE_MANAGER, nullptr));
    manager->priv->dataManager = dataManager;
    auto sessionID = webkitWebsiteDataManagerGetDataStore(manager->priv->dataManager).websiteDataStore().sessionID();
    for (auto* processPool : webkitWebsiteDataManagerGetProcessPools(manager->priv->dataManager)) {
        processPool->supplement<WebCookieManagerProxy>()->setCookieObserverCallback(sessionID, [manager] {
            g_signal_emit(manager, signals[CHANGED], 0);
        });
    }
    return manager;
}

/**
 * webkit_cookie_manager_set_persistent_storage:
 * @cookie_manager: a #WebKitCookieManager
 * @filename: the filename to read to/write from
 * @storage: a #WebKitCookiePersistentStorage
 *
 * Set the @filename where non-session cookies are stored persistently using
 * @storage as the format to read/write the cookies.
 * Cookies are initially read from @filename to create an initial set of cookies.
 * Then, non-session cookies will be written to @filename when the WebKitCookieManager::changed
 * signal is emitted.
 * By default, @cookie_manager doesn't store the cookies persistently, so you need to call this
 * method to keep cookies saved across sessions.
 *
 * This method should never be called on a #WebKitCookieManager associated to an ephemeral #WebKitWebsiteDataManager.
 */
void webkit_cookie_manager_set_persistent_storage(WebKitCookieManager* manager, const char* filename, WebKitCookiePersistentStorage storage)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));
    g_return_if_fail(filename);
    g_return_if_fail(!webkit_website_data_manager_is_ephemeral(manager->priv->dataManager));

    for (auto* processPool : webkitWebsiteDataManagerGetProcessPools(manager->priv->dataManager))
        processPool->supplement<WebCookieManagerProxy>()->setCookiePersistentStorage(String::fromUTF8(filename), toSoupCookiePersistentStorageType(storage));
}

/**
 * webkit_cookie_manager_set_accept_policy:
 * @cookie_manager: a #WebKitCookieManager
 * @policy: a #WebKitCookieAcceptPolicy
 *
 * Set the cookie acceptance policy of @cookie_manager as @policy.
 */
void webkit_cookie_manager_set_accept_policy(WebKitCookieManager* manager, WebKitCookieAcceptPolicy policy)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    for (auto* processPool : webkitWebsiteDataManagerGetProcessPools(manager->priv->dataManager))
        processPool->supplement<WebCookieManagerProxy>()->setHTTPCookieAcceptPolicy(WebCore::SessionID::defaultSessionID(), toHTTPCookieAcceptPolicy(policy), [](CallbackBase::Error){});
}

/**
 * webkit_cookie_manager_get_accept_policy:
 * @cookie_manager: a #WebKitCookieManager
 * @cancellable: (allow-none): a #GCancellable or %NULL to ignore
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously get the cookie acceptance policy of @cookie_manager.
 *
 * When the operation is finished, @callback will be called. You can then call
 * webkit_cookie_manager_get_accept_policy_finish() to get the result of the operation.
 */
void webkit_cookie_manager_get_accept_policy(WebKitCookieManager* manager, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    GRefPtr<GTask> task = adoptGRef(g_task_new(manager, cancellable, callback, userData));

    // The policy is the same in all process pools having the same session ID, so just ask any.
    const auto& processPools = webkitWebsiteDataManagerGetProcessPools(manager->priv->dataManager);
    if (processPools.isEmpty()) {
        g_task_return_int(task.get(), WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);
        return;
    }

    processPools[0]->supplement<WebCookieManagerProxy>()->getHTTPCookieAcceptPolicy(WebCore::SessionID::defaultSessionID(), [task = WTFMove(task)](HTTPCookieAcceptPolicy policy, CallbackBase::Error) {
        g_task_return_int(task.get(), toWebKitCookieAcceptPolicy(policy));
    });
}

/**
 * webkit_cookie_manager_get_accept_policy_finish:
 * @cookie_manager: a #WebKitCookieManager
 * @result: a #GAsyncResult
 * @error: return location for error or %NULL to ignore
 *
 * Finish an asynchronous operation started with webkit_cookie_manager_get_accept_policy().
 *
 * Returns: the cookie acceptance policy of @cookie_manager as a #WebKitCookieAcceptPolicy.
 */
WebKitCookieAcceptPolicy webkit_cookie_manager_get_accept_policy_finish(WebKitCookieManager* manager, GAsyncResult* result, GError** error)
{
    g_return_val_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager), WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);
    g_return_val_if_fail(g_task_is_valid(result, manager), WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);

    gssize returnValue = g_task_propagate_int(G_TASK(result), error);
    return returnValue == -1 ? WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY : static_cast<WebKitCookieAcceptPolicy>(returnValue);
}

/**
 * webkit_cookie_manager_get_domains_with_cookies:
 * @cookie_manager: a #WebKitCookieManager
 * @cancellable: (allow-none): a #GCancellable or %NULL to ignore
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously get the list of domains for which @cookie_manager contains cookies.
 *
 * When the operation is finished, @callback will be called. You can then call
 * webkit_cookie_manager_get_domains_with_cookies_finish() to get the result of the operation.
 *
 * Deprecated: 2.16: Use webkit_website_data_manager_fetch() instead.
 */
void webkit_cookie_manager_get_domains_with_cookies(WebKitCookieManager* manager, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    GTask* task = g_task_new(manager, cancellable, callback, userData);
    webkit_website_data_manager_fetch(manager->priv->dataManager, WEBKIT_WEBSITE_DATA_COOKIES, cancellable, [](GObject* object, GAsyncResult* result, gpointer userData) {
        GRefPtr<GTask> task = adoptGRef(G_TASK(userData));
        GError* error = nullptr;
        GUniquePtr<GList> dataList(webkit_website_data_manager_fetch_finish(WEBKIT_WEBSITE_DATA_MANAGER(object), result, &error));
        if (error) {
            g_task_return_error(task.get(), error);
            return;
        }

        GPtrArray* domains = g_ptr_array_sized_new(g_list_length(dataList.get()));
        for (GList* item = dataList.get(); item; item = g_list_next(item)) {
            auto* data = static_cast<WebKitWebsiteData*>(item->data);
            g_ptr_array_add(domains, g_strdup(webkit_website_data_get_name(data)));
            webkit_website_data_unref(data);
        }
        g_ptr_array_add(domains, nullptr);
        g_task_return_pointer(task.get(), g_ptr_array_free(domains, FALSE), reinterpret_cast<GDestroyNotify>(g_strfreev));
    }, task);
}

/**
 * webkit_cookie_manager_get_domains_with_cookies_finish:
 * @cookie_manager: a #WebKitCookieManager
 * @result: a #GAsyncResult
 * @error: return location for error or %NULL to ignore
 *
 * Finish an asynchronous operation started with webkit_cookie_manager_get_domains_with_cookies().
 * The return value is a %NULL terminated list of strings which should
 * be released with g_strfreev().
 *
 * Returns: (transfer full) (array zero-terminated=1): A %NULL terminated array of domain names
 *    or %NULL in case of error.
 *
 * Deprecated: 2.16: Use webkit_website_data_manager_fetch_finish() instead.
 */
gchar** webkit_cookie_manager_get_domains_with_cookies_finish(WebKitCookieManager* manager, GAsyncResult* result, GError** error)
{
    g_return_val_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager), nullptr);
    g_return_val_if_fail(g_task_is_valid(result, manager), nullptr);

    return reinterpret_cast<char**>(g_task_propagate_pointer(G_TASK(result), error));
}

/**
 * webkit_cookie_manager_delete_cookies_for_domain:
 * @cookie_manager: a #WebKitCookieManager
 * @domain: a domain name
 *
 * Remove all cookies of @cookie_manager for the given @domain.
 *
 * Deprecated: 2.16: Use webkit_website_data_manager_remove() instead.
 */
void webkit_cookie_manager_delete_cookies_for_domain(WebKitCookieManager* manager, const gchar* domain)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));
    g_return_if_fail(domain);

    WebsiteDataRecord record;
    record.addCookieHostName(String::fromUTF8(domain));
    auto* data = webkitWebsiteDataCreate(WTFMove(record));
    GList dataList = { data, nullptr, nullptr };
    webkit_website_data_manager_remove(manager->priv->dataManager, WEBKIT_WEBSITE_DATA_COOKIES, &dataList, nullptr, nullptr, nullptr);
    webkit_website_data_unref(data);
}

/**
 * webkit_cookie_manager_delete_all_cookies:
 * @cookie_manager: a #WebKitCookieManager
 *
 * Delete all cookies of @cookie_manager
 *
 * Deprecated: 2.16: Use webkit_website_data_manager_clear() instead.
 */
void webkit_cookie_manager_delete_all_cookies(WebKitCookieManager* manager)
{
    g_return_if_fail(WEBKIT_IS_COOKIE_MANAGER(manager));

    webkit_website_data_manager_clear(manager->priv->dataManager, WEBKIT_WEBSITE_DATA_COOKIES, 0, nullptr, nullptr, nullptr);
}
