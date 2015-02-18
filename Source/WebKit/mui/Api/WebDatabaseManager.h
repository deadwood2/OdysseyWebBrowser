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

#ifndef WebDatabaseManager_h
#define WebDatabaseManager_h


/**
 *  @file  WebDatabaseManager.h
 *  WebDatabaseManager description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */
#include "WebKitTypes.h"
#include <vector>

namespace WebCore {
    class DatabaseDetails;
}

class WebDatabaseTracker;
class WebSecurityOrigin;

#define WebDatabaseDisplayNameKey "WebDatabaseDisplayNameKey"
#define WebDatabaseExpectedSizeKey "WebDatabaseExpectedSizeKey"
#define WebDatabaseUsageKey "WebDatabaseUsageKey"

#define WebDatabaseDidModifyOriginNotification "WebDatabaseDidModifyOriginNotification"
#define WebDatabaseDidModifyDatabaseNotification "WebDatabaseDidModifyDatabaseNotification"
#define WebDatabaseNameKey "WebDatabaseNameKey"

class WEBKIT_OWB_API WebDatabaseDetails {
protected:
    friend class WebDatabaseManager;
    static WebDatabaseDetails* createInstance(const char* name, const char* displayName, unsigned long long expectedUsage, unsigned long long currentUsage);

public:
    ~WebDatabaseDetails();

    const char* name() const { return m_name; }
    const char* displayName() const { return m_displayName; }
    unsigned long long expectedUsage() const { return m_expectedUsage; }
    unsigned long long currentUsage() const { return m_currentUsage; }

private:
    WebDatabaseDetails(const char* name, const char* displayName, unsigned long long expectedUsage, unsigned long long currentUsage)
    : m_name(name)
    , m_displayName(displayName)
    , m_expectedUsage(expectedUsage)
    , m_currentUsage(currentUsage)
    {}

    const char* m_name;
    const char* m_displayName;
    unsigned long long m_expectedUsage;
    unsigned long long m_currentUsage;
};


class WEBKIT_OWB_API WebDatabaseManager {
public:

    /**
     * create a new instance of WebDatabaseManager
     * @param[out]: WebDatabaseManager
     * @code
     * WebDatabaseManager *d = WebDatabaseManager::createInstance();
     * @endcode
     */
    static WebDatabaseManager* createInstance();


    /**
     * WebDatabaseManager destructor
     */
    virtual ~WebDatabaseManager();

    /**
     * get shared WebDatabaseManager
     * @param[out]: WebDatabaseManager
     * @code
     * WebDatabaseManager *d = WebDatabaseManager::sharedWebDatabaseManager();
     * @endcode
     */
    virtual WebDatabaseManager* sharedWebDatabaseManager();

    /**
     * get origins
     * @param[out]: security origin
     * @code
     * WTF::Vector<RefPtr<WebCore::SecurityOrigin> > s = d->origins();
     * @endcode
     */
    virtual std::vector<WebSecurityOrigin*> origins();

    /**
     * get databases with origin
     * @param[in]: WebSecurityOrigin
     * @param[out]: String
     * @code
     * WTF::Vector<WebCore::String> s = d->databasesWithOrigin(o);
     * @endcode
     */
    virtual std::vector<const char*> databasesWithOrigin(WebSecurityOrigin* origin);

    /**
     * get details for database
     * @param[in]: databaseName
     * @param[in]: origin
     * @param[out]: DatabaseDetails
     * @code
     * WebCore::DatabaseDetails dd = d->detailsForDatabase(dn, o);
     * @endcode
     */
    virtual WebDatabaseDetails* detailsForDatabase(const char* databaseName, WebSecurityOrigin* origin);

    /**
     * delete all databases
     */
    virtual void deleteAllDatabases();

    /**
     * delete web security origin
     * @param[in]: WebSecurityOrigin
     */
    virtual void deleteOrigin(WebSecurityOrigin* origin);

    /**
     * set quota
     * @parma[in]: origin
     * @param[in]: quota
     */
    virtual void setQuota(const char* origin, unsigned long long quota);

    /**
     * delete Database 
     * @param[in]: database name
     * @param[in]: WebSecurityOrigin
     */
    virtual void deleteDatabase(const char* databaseName, WebSecurityOrigin* origin);


    /**
     * dispatch did modify origin
     * @param[in]: SecurityOrigin
     */
    virtual void dispatchDidModifyOrigin(WebSecurityOrigin*);

    /**
     * dispatch did modify database
     * @param[in]: SecurityOrigin
     * @param[in]: database name
     * @param[out]: description
     * @code
     * @endcode
     */
    virtual void dispatchDidModifyDatabase(WebSecurityOrigin*, const char* databaseName);

    WebDatabaseTracker* webDatabaseTracker() { return m_webDatabaseTracker; }

private:

    /**
     * WebDatabaseManager default constructor
     */
    WebDatabaseManager();

    WebDatabaseTracker* m_webDatabaseTracker;
};


    /**
     *  WebKit set WebDatabases path if necessary
     */
void WebKitInitializeWebDatabasesIfNecessary();
#endif
