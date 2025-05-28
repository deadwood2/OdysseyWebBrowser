/*
 * Copyright (C) 2007-2008, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebDatabaseManager.h"
#include <WebCore/DatabaseManager.h>
#include <WebCore/DatabaseTracker.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityOriginData.h>
#include <wtf/FileSystem.h>
#include <wtf/MainThread.h>

#if ENABLE(INDEXED_DATABASE)
#include "WebDatabaseProvider.h"
#endif

using namespace WebCore;

// WebDatabaseManager --------------------------------------------------------------
WebDatabaseManager* WebDatabaseManager::createInstance()
{
    WebDatabaseManager* manager = new WebDatabaseManager();
    return manager;
}

WebDatabaseManager::WebDatabaseManager()
{
}

WebDatabaseManager::~WebDatabaseManager()
{
}

class DidModifyOriginData {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(DidModifyOriginData);
public:
    static void dispatchToMainThread(WebDatabaseManager* databaseManager, const SecurityOriginData& origin)
    {
        DidModifyOriginData* context = new DidModifyOriginData(databaseManager, origin.isolatedCopy());
        callOnMainThread([context] {
            dispatchDidModifyOriginOnMainThread(context);
        });
    }

private:
    DidModifyOriginData(WebDatabaseManager* databaseManager, const SecurityOriginData& origin)
        : databaseManager(databaseManager)
        , origin(origin)
    {
    }

    static void dispatchDidModifyOriginOnMainThread(void* context)
    {
        ASSERT(isMainThread());
        DidModifyOriginData* info = static_cast<DidModifyOriginData*>(context);
        info->databaseManager->dispatchDidModifyOrigin(info->origin);
        delete info;
    }

    WebDatabaseManager* databaseManager;
    SecurityOriginData origin;
};

void WebDatabaseManager::dispatchDidModifyOrigin(const SecurityOriginData& origin)
{
#if 0
    if (!isMainThread()) {
        DidModifyOriginData::dispatchToMainThread(this, origin);
        return;
    }

    static BSTR databaseDidModifyOriginName = SysAllocString(WebDatabaseDidModifyOriginNotification);
    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();

    COMPtr<WebSecurityOrigin> securityOrigin(AdoptCOM, WebSecurityOrigin::createInstance(origin.securityOrigin().ptr()));
    notifyCenter->postNotificationName(databaseDidModifyOriginName, securityOrigin.get(), 0);
#endif
}

void WebDatabaseManager::dispatchDidModifyDatabase(const SecurityOriginData& origin, const String& databaseName)
{
#if 0
    if (!isMainThread()) {
        DidModifyOriginData::dispatchToMainThread(this, origin);
        return;
    }

    static BSTR databaseDidModifyOriginName = SysAllocString(WebDatabaseDidModifyDatabaseNotification);
    IWebNotificationCenter* notifyCenter = WebNotificationCenter::defaultCenterInternal();

    COMPtr<WebSecurityOrigin> securityOrigin(AdoptCOM, WebSecurityOrigin::createInstance(origin.securityOrigin().ptr()));

    HashMap<String, String> userInfo;
    userInfo.set(WebDatabaseNameKey, databaseName);
    COMPtr<IPropertyBag> userInfoBag(AdoptCOM, COMPropertyBag<String>::adopt(userInfo));

    notifyCenter->postNotificationName(databaseDidModifyOriginName, securityOrigin.get(), userInfoBag.get());
#endif
}

static WTF::String databasesDirectory()
{
	return "PROGDIR:Cache/Databases/";
}

void WebKitInitializeWebDatabasesIfNecessary()
{
    static bool initialized = false;
    if (initialized)
        return;

    WebCore::DatabaseManager::singleton().initialize(databasesDirectory());
    initialized = true;
}
