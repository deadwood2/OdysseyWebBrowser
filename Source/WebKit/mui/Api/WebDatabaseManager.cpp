/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebDatabaseManager.h"

#include "WebSecurityOrigin.h"

#include <wtf/MainThread.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <DatabaseManager.h>
#include <DatabaseManagerClient.h>
#include <FileSystem.h>
#include <SecurityOrigin.h>
#include "ObserverServiceData.h"

using namespace WebCore;
using namespace std;


class WebDatabaseTracker : public WebCore::DatabaseManagerClient {
public:
    WebDatabaseTracker(WebDatabaseManager* manager) : m_webDatabaseManager(manager) {}
    ~WebDatabaseTracker() { m_webDatabaseManager = 0; }

    void dispatchDidModifyOrigin(SecurityOrigin* origin)
    {
        WebSecurityOrigin* securityOrigin = WebSecurityOrigin::createInstance(origin);
        WebCore::ObserverServiceData::createObserverService()->notifyObserver(WebDatabaseDidModifyOriginNotification, "", securityOrigin);

        delete securityOrigin;
    }

    void dispatchDidModifyDatabase(SecurityOrigin* origin, const String& databaseName)
    {
        WebSecurityOrigin* securityOrigin = WebSecurityOrigin::createInstance(origin);

        WebCore::ObserverServiceData::createObserverService()->notifyObserver(WebDatabaseDidModifyDatabaseNotification, "", securityOrigin);

        delete securityOrigin;
    }

private:
    WebDatabaseManager* m_webDatabaseManager;
};

WebDatabaseDetails* WebDatabaseDetails::createInstance(const char* name, const char* displayName, unsigned long long expectedUsage, unsigned long long currentUsage)
{
    WebDatabaseDetails* webDatabaseDetails = new WebDatabaseDetails(name, displayName, expectedUsage, currentUsage);
    return webDatabaseDetails;
}

WebDatabaseDetails::~WebDatabaseDetails()
{
    if (m_name)
        delete m_name;
    if (m_displayName)
        delete m_displayName;
}

static WebDatabaseManager* s_sharedWebDatabaseManager;

WebDatabaseManager* WebDatabaseManager::createInstance()
{
    WebDatabaseManager* manager = new WebDatabaseManager();
    return manager;
}

WebDatabaseManager::WebDatabaseManager()
: m_webDatabaseTracker(new WebDatabaseTracker(this))
{
}

WebDatabaseManager::~WebDatabaseManager()
{
    delete m_webDatabaseTracker;
}

WebDatabaseManager* WebDatabaseManager::sharedWebDatabaseManager()
{
    if (!s_sharedWebDatabaseManager) {
        s_sharedWebDatabaseManager = WebDatabaseManager::createInstance();
	DatabaseManager::singleton().setClient(s_sharedWebDatabaseManager->webDatabaseTracker());
    }

    return s_sharedWebDatabaseManager;
}

vector<WebSecurityOrigin*> WebDatabaseManager::origins()
{
    vector<WebSecurityOrigin*> ori;
    if (this != s_sharedWebDatabaseManager)
        return ori;

    Vector<RefPtr<SecurityOrigin> > origins;
    DatabaseManager::singleton().origins(origins);
    for(size_t i = 0; i < origins.size(); i++)
        ori.push_back(WebSecurityOrigin::createInstance(origins[i].get()));

    return ori;
}

vector<const char*> WebDatabaseManager::databasesWithOrigin(WebSecurityOrigin* origin)
{
    vector<const char*> databaseN;
    if (this != s_sharedWebDatabaseManager)
        return databaseN;

    WebSecurityOrigin *webSecurityOrigin = WebSecurityOrigin::createInstance(origin->securityOrigin());
    if (!webSecurityOrigin)
        return databaseN;

    Vector<String> databaseNames;
    DatabaseManager::singleton().databaseNamesForOrigin(webSecurityOrigin->securityOrigin(), databaseNames);

    delete webSecurityOrigin;

    for(size_t i = 0; i < databaseNames.size(); i++)
        databaseN.push_back(strdup(databaseNames[i].utf8().data()));

    return databaseN;
}

WebDatabaseDetails* WebDatabaseManager::detailsForDatabase(const char* databaseName, WebSecurityOrigin* origin)
{
    if (this != s_sharedWebDatabaseManager)
        return 0;

    WebSecurityOrigin* webSecurityOrigin = WebSecurityOrigin::createInstance(origin->securityOrigin());
    if (!webSecurityOrigin)
        return 0;

    DatabaseDetails d = DatabaseManager::singleton().detailsForNameAndOrigin(databaseName, webSecurityOrigin->securityOrigin());
    WebDatabaseDetails *details = WebDatabaseDetails::createInstance(strdup(d.name().utf8().data()), strdup(d.displayName().utf8().data()), d.expectedUsage(), d.currentUsage());

    delete webSecurityOrigin;

    return details;
}

void WebDatabaseManager::deleteAllDatabases()
{
    DatabaseManager::singleton().deleteAllDatabases();
}

void WebDatabaseManager::deleteOrigin(WebSecurityOrigin* origin)
{
    if (this != s_sharedWebDatabaseManager)
        return;

    WebSecurityOrigin* webSecurityOrigin = WebSecurityOrigin::createInstance(origin->securityOrigin());
    if (!webSecurityOrigin)
        return;

    DatabaseManager::singleton().deleteOrigin(webSecurityOrigin->securityOrigin());

    delete webSecurityOrigin;
}

void WebDatabaseManager::setQuota(const char* origin, unsigned long long quota)
{
    if (!origin)
        return;

    if (this != s_sharedWebDatabaseManager)
        return;

    DatabaseManager::singleton().setQuota(SecurityOrigin::createFromString(origin).ptr(), quota);
}

void WebDatabaseManager::deleteDatabase(const char* databaseName, WebSecurityOrigin* origin)
{
    if (this != s_sharedWebDatabaseManager)
        return;

    WebSecurityOrigin* webSecurityOrigin = WebSecurityOrigin::createInstance(origin->securityOrigin());
    if (!webSecurityOrigin)
        return;

    DatabaseManager::singleton().deleteDatabase(webSecurityOrigin->securityOrigin(), databaseName);

    delete webSecurityOrigin;
}
/*
class DidModifyOriginData : public Noncopyable {
public:
    static void dispatchToMainThread(WebDatabaseManager* databaseManager, SecurityOrigin* origin)
    {
        DidModifyOriginData* context = new DidModifyOriginData(databaseManager, origin->threadsafeCopy());
        callOnMainThread(&DidModifyOriginData::dispatchDidModifyOriginOnMainThread, context);
    }

private:
    DidModifyOriginData(WebDatabaseManager* databaseManager, PassRefPtr<SecurityOrigin> origin)
        : databaseManager(databaseManager)
        , origin(origin)
    {
    }

    static void dispatchDidModifyOriginOnMainThread(void* context)
    {
        ASSERT(isMainThread());
        DidModifyOriginData* info = static_cast<DidModifyOriginData*>(context);
        info->databaseManager->dispatchDidModifyOrigin(info->origin.get());
        delete info;
    }

    WebDatabaseManager* databaseManager;
    RefPtr<SecurityOrigin> origin;
};
*/
void WebDatabaseManager::dispatchDidModifyOrigin(WebSecurityOrigin* origin)
{  /*
   if (!isMainThread()) {
       DidModifyOriginData::dispatchToMainThread(this, origin);
       return;
   }
	*/
    m_webDatabaseTracker->dispatchDidModifyOrigin(origin->securityOrigin());
}

void WebDatabaseManager::dispatchDidModifyDatabase(WebSecurityOrigin* origin, const char* databaseName)
{   /*
    if (!isMainThread()) {
        DidModifyOriginData::dispatchToMainThread(this, origin);
        return;
    }
	*/
    m_webDatabaseTracker->dispatchDidModifyDatabase(origin->securityOrigin(), databaseName);
}

void WebKitInitializeWebDatabasesIfNecessary()
{
    static bool initialized = false;
    if (initialized)
        return;

    WTF::String databasesDirectory = WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), "Databases");
    WebCore::DatabaseManager::singleton().initialize(databasesDirectory);

    initialized = true;
}
