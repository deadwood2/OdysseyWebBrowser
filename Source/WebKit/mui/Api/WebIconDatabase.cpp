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

#if ENABLE(ICONDATABASE)
#include "WebIconDatabase.h"

#include "WebPreferences.h"

#include <BitmapImage.h>
#include <wtf/text/CString.h>
#include <IconDatabaseClient.h>
#include <FileSystem.h>
#include <IconDatabase.h>
#include <Image.h>
#include <wtf/text/WTFString.h>
#include <wtf/MainThread.h>
#include "ObserverServiceData.h"

#include "gui.h"
#undef set
#undef get
#undef String
#undef PageGroup
#include <clib/debug_protos.h>
#define D(x)

using namespace WebCore;
using namespace WTF;
using namespace std;

class WebIconDatabaseClient : public IconDatabaseClient {
public:
    WebIconDatabaseClient(WebIconDatabase* icon)
    : m_webIconDatabase(icon)
    {}

    virtual ~WebIconDatabaseClient() { m_webIconDatabase = 0; }
    virtual bool performImport() { return true; }
    virtual void didRemoveAllIcons() { m_webIconDatabase->didRemoveAllIcons(); }
    virtual void didImportIconURLForPageURL(const WTF::String& pageURL) { m_webIconDatabase->didImportIconURLForPageURL(strdup(pageURL.utf8().data())); }
	virtual void didImportIconDataForPageURL(const WTF::String& pageURL) { m_webIconDatabase->didImportIconDataForPageURL(strdup(pageURL.utf8().data())); }
	virtual void didChangeIconForPageURL(const WTF::String& pageURL) { m_webIconDatabase->didChangeIconForPageURL(strdup(pageURL.utf8().data())); }
	virtual void didFinishURLImport() { m_webIconDatabase->didFinishURLImport(); }
    virtual void didFinishURLIconImport() { m_webIconDatabase->didFinishURLIconImport(); }

    Mutex m_notificationMutex;
private:
    WebIconDatabase* m_webIconDatabase;
};


WebIconDatabase* WebIconDatabase::m_sharedWebIconDatabase = 0;

WebIconDatabase::WebIconDatabase()
: m_deliveryRequested(false)
, m_webIconDatabaseClient(new WebIconDatabaseClient(this))
{
}

WebIconDatabase::~WebIconDatabase()
{
	iconDatabase().close();
}

void WebIconDatabase::init()
{
    WebPreferences* standardPrefs = WebPreferences::sharedStandardPreferences();
    bool enabled = standardPrefs->iconDatabaseEnabled();
//     if (!enabled) {
//         LOG_ERROR("Unable to get icon database enabled preference");
//     }
    iconDatabase().setEnabled(!!enabled);

    startUpIconDatabase();
}

void WebIconDatabase::startUpIconDatabase()
{
    WebPreferences* standardPrefs = WebPreferences::sharedStandardPreferences();
	iconDatabase().setClient(m_webIconDatabaseClient);

    String databasePath = standardPrefs->iconDatabaseLocation();

    if (databasePath.isEmpty())
        databasePath = homeDirectoryPath();

    bool ret = iconDatabase().open(databasePath, WebCore::IconDatabase::defaultDatabaseFilename());
    if (!ret)
        LOG_ERROR("Failed to open icon database path");
}

void WebIconDatabase::shutDownIconDatabase()
{
}

WebIconDatabase* WebIconDatabase::createInstance()
{
    WebIconDatabase* instance = new WebIconDatabase();
    return instance;
}

WebIconDatabase* WebIconDatabase::sharedWebIconDatabase()
{
    if (m_sharedWebIconDatabase) {
        return m_sharedWebIconDatabase;
    }
    m_sharedWebIconDatabase = createInstance();
    m_sharedWebIconDatabase->init();
    return m_sharedWebIconDatabase;
}

WebIconDatabase *WebIconDatabase::sharedIconDatabase()
{
    return sharedWebIconDatabase();
}

WebCore::Image *WebIconDatabase::iconForURL(const char* url, IntSize size, bool /*cache*/)
{
    IntSize intSize(size);

	WebCore::Image* icon = iconDatabase().synchronousIconForPageURL(url, intSize);
    return icon;
}

void WebIconDatabase::retainIconForURL(const char* url)
{
    iconDatabase().retainIconForPageURL(url);
}

void WebIconDatabase::releaseIconForURL(const char* url)
{
    iconDatabase().releaseIconForPageURL(url);
}

void WebIconDatabase::removeAllIcons()
{
    iconDatabase().removeAllIcons();
}

void WebIconDatabase::delayDatabaseCleanup()
{
    IconDatabase::delayDatabaseCleanup();
}

void WebIconDatabase::allowDatabaseCleanup()
{
    IconDatabase::allowDatabaseCleanup();
}

const char* WebIconDatabase::iconURLForURL(const char* url)
{
    return strdup(iconDatabase().synchronousIconURLForPageURL(url).utf8().data());
}


BalSurface* WebIconDatabase::getOrCreateSharedBitmap(BalPoint s)
{
    BalSurface* result = m_sharedIconMap[s];
    if (result)
        return result;
    RefPtr<BitmapImage> image = BitmapImage::create();
    IntPoint p(s);
    IntSize size(p.x(), p.y());
    image->setContainerSize(size);
    cairo_surface_t *surf = image->nativeImageForCurrentFrame().leakRef();
    result = surf;
    m_sharedIconMap[s] = result;
    return result;
}

BalSurface* WebIconDatabase::getOrCreateDefaultIconBitmap(BalPoint s)
{
    BalSurface* result = m_defaultIconMap[s];
    if (result)
        return result;

    RefPtr<BitmapImage> image = BitmapImage::create();
    IntPoint p(s);
    IntSize size(p.x(), p.y());
    image->setContainerSize(size);
    
    cairo_surface_t *surf = image->nativeImageForCurrentFrame().leakRef();
    result = surf;
    m_defaultIconMap[s] = result;
    return result;
}

bool WebIconDatabase::isEnabled()
{
    return iconDatabase().isEnabled();
}

void WebIconDatabase::setEnabled(bool flag)
{
    bool currentlyEnabled = isEnabled();
    if (currentlyEnabled && !flag) {
        iconDatabase().setEnabled(false);
        shutDownIconDatabase();
    } else if (!currentlyEnabled && flag) {
        iconDatabase().setEnabled(true);
        startUpIconDatabase();
    }
}

bool WebIconDatabase::isPrivateBrowsingEnabled()
{
	return iconDatabase().isPrivateBrowsingEnabled();
}

void WebIconDatabase::setPrivateBrowsingEnabled(bool flag)
{
	iconDatabase().setPrivateBrowsingEnabled(flag);
}

bool WebIconDatabase::performImport()
{
    // Windows doesn't do any old-style database importing.
    return true;
}         

void WebIconDatabase::didRemoveAllIcons()
{
    // Queueing the empty string is a special way of saying "this queued notification is the didRemoveAllIcons notification"
    MutexLocker locker(m_webIconDatabaseClient->m_notificationMutex);
    m_notificationQueue.push_back("");
    scheduleNotificationDelivery();
}

void WebIconDatabase::didImportIconURLForPageURL(const char* pageURL)
{   
	D(kprintf("WebIconDatabase::didImportIconURLForPageURL(%s)\n", pageURL));

    MutexLocker locker(m_webIconDatabaseClient->m_notificationMutex);
    m_notificationQueue.push_back(String(pageURL));
    scheduleNotificationDelivery();

    free((char *)pageURL); // Since it's allocated by caller (sigh)
}

void WebIconDatabase::didImportIconDataForPageURL(const char* pageURL)
{
	D(kprintf("WebIconDatabase::didImportIconDataForPageURL(%s)\n", pageURL));
    // WebKit1 only has a single "icon did change" notification.
    didImportIconURLForPageURL(pageURL);
}

void WebIconDatabase::didChangeIconForPageURL(const char* pageURL)
{
	D(kprintf("WebIconDatabase::didChangeIconForPageURL(%s)\n", pageURL));
    // WebKit1 only has a single "icon did change" notification.
    didImportIconURLForPageURL(pageURL);
}

void WebIconDatabase::didFinishURLImport()
{
	didFinishURLIconImport();
}  

void WebIconDatabase::scheduleNotificationDelivery()
{
    // Caller of this method must hold the m_notificationQueue lock
    ASSERT(!m_webIconDatabaseClient->m_notificationMutex.tryLock());

    if (!m_deliveryRequested) {
        m_deliveryRequested = true;
        callOnMainThread(deliverNotifications, 0);
    }
}

const char* WebIconDatabase::iconDatabaseDidAddIconNotification()
{
    static const char* didAddIconName = WebIconDatabaseDidAddIconNotification;
    return didAddIconName;
}

const char* WebIconDatabase::iconDatabaseNotificationUserInfoURLKey()
{
    static const char* iconUserInfoURLKey = WebIconNotificationUserInfoURLKey;
    return iconUserInfoURLKey;
}

const char* WebIconDatabase::iconDatabaseDidRemoveAllIconsNotification()
{
    static const char* didRemoveAllIconsName = WebIconDatabaseDidRemoveAllIconsNotification;
    return didRemoveAllIconsName;
}

static void postDidRemoveAllIconsNotification(WebIconDatabase* iconDB)
{
    WebCore::ObserverServiceData::createObserverService()->notifyObserver(WebIconDatabase::iconDatabaseDidRemoveAllIconsNotification(), "", iconDB);
}

static void postDidAddIconNotification(String pageURL, WebIconDatabase* iconDB)
{
	SetAttrs(app, MA_OWBApp_DidReceiveFavIcon, pageURL.utf8().data(), TAG_DONE);
	D(kprintf("postDidAddIconNotification(%s)\n", pageURL.utf8().data()));
    WebCore::ObserverServiceData::createObserverService()->notifyObserver(WebIconDatabase::iconDatabaseDidAddIconNotification(), pageURL, iconDB);
}

void WebIconDatabase::deliverNotifications(void*)
{
    ASSERT(m_sharedWebIconDatabase);
    if (!m_sharedWebIconDatabase)
        return;

    ASSERT(m_sharedWebIconDatabase->m_deliveryRequested);

	vector<WTF::String> queue;
    {
        MutexLocker locker(m_sharedWebIconDatabase->m_webIconDatabaseClient->m_notificationMutex);
        queue.swap(m_sharedWebIconDatabase->m_notificationQueue);
        m_sharedWebIconDatabase->m_deliveryRequested = false;
    }

    for (unsigned i = 0; i < queue.size(); ++i) {
        if (!queue[i])
            postDidRemoveAllIconsNotification(m_sharedWebIconDatabase);
        else
            postDidAddIconNotification(queue[i], m_sharedWebIconDatabase);
    }
}

bool operator<(BalPoint p1, BalPoint p2)
{
    IntPoint point1(p1);
    IntPoint point2(p2);
    return point1 < point2;
}

void WebIconDatabase::didFinishURLIconImport()
{
	D(kprintf("WebIconDatabase::didFinishURLIconImport()\n"));
	SetAttrs(app, MA_OWBApp_FavIconImportComplete, TRUE, TAG_DONE);
	//allowDatabaseCleanup();
}

#endif
