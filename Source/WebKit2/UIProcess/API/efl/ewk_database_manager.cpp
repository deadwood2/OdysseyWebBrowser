/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ewk_database_manager.h"

#include "APIWebsiteDataStore.h"
#include "GenericCallback.h"
#include "WKAPICast.h"
#include "WKArray.h"
#include "ewk_database_manager_private.h"
#include "ewk_security_origin_private.h"
#include <WebCore/DatabaseTracker.h>
#include <WebCore/OriginLock.h>
#include <WebCore/SecurityOrigin.h>

using namespace WebKit;

typedef GenericCallback<API::Array*> ArrayCallback;

EwkDatabaseManager::EwkDatabaseManager()
{
}

void EwkDatabaseManager::getDatabaseOrigins(Ewk_Database_Manager_Get_Database_Origins_Function callback, void* context) const
{
    if (!callback)
        return;

    RefPtr<ArrayCallback> arrayCallback = ArrayCallback::create(toGenericCallbackFunction(context, callback));

    Vector<RefPtr<WebCore::SecurityOrigin>> origins;
    Vector<RefPtr<API::Object>> securityOrigins;

    WebCore::DatabaseTracker::trackerWithDatabasePath(API::WebsiteDataStore::defaultWebSQLDatabaseDirectory())->origins(origins);
    securityOrigins.reserveInitialCapacity(origins.size());

    for (const auto& originIdentifier : origins)
        securityOrigins.uncheckedAppend(API::SecurityOrigin::create(*originIdentifier));

    arrayCallback->performCallbackWithReturnValue(API::Array::create(WTFMove(securityOrigins)).ptr());
}

Eina_List* EwkDatabaseManager::createOriginList(WKArrayRef origins) const
{
    Eina_List* originList = 0;
    const size_t length = WKArrayGetSize(origins);

    for (size_t i = 0; i < length; ++i) {
        WKSecurityOriginRef wkOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        RefPtr<EwkSecurityOrigin> origin = m_wrapperCache.get(wkOriginRef);
        if (!origin) {
            origin = EwkSecurityOrigin::create(wkOriginRef);
            m_wrapperCache.set(wkOriginRef, origin);
        }
        originList = eina_list_append(originList, origin.leakRef());
    }

    return originList;
}

struct EwkDatabaseOriginsAsyncData {
    const Ewk_Database_Manager* manager;
    Ewk_Database_Origins_Async_Get_Cb callback;
    void* userData;

    EwkDatabaseOriginsAsyncData(const Ewk_Database_Manager* manager, Ewk_Database_Origins_Async_Get_Cb callback, void* userData)
        : manager(manager)
        , callback(callback)
        , userData(userData)
    { }
};

static void getDatabaseOriginsCallback(WKArrayRef origins, WKErrorRef, void* context)
{
    auto webDatabaseContext = std::unique_ptr<EwkDatabaseOriginsAsyncData>(static_cast<EwkDatabaseOriginsAsyncData*>(context));
    Eina_List* originList = webDatabaseContext->manager->createOriginList(origins);
    webDatabaseContext->callback(originList, webDatabaseContext->userData);
}

Eina_Bool ewk_database_manager_origins_async_get(const Ewk_Database_Manager* ewkDatabaseManager, Ewk_Database_Origins_Async_Get_Cb callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkDatabaseManager, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    EwkDatabaseOriginsAsyncData* context = new EwkDatabaseOriginsAsyncData(ewkDatabaseManager, callback, userData);
    ewkDatabaseManager->getDatabaseOrigins(getDatabaseOriginsCallback, context);

    return true;
}
