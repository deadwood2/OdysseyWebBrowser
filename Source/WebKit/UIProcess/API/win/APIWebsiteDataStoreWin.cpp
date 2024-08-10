/*
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "APIWebsiteDataStore.h"

#include <WebCore/FileSystem.h>

namespace API {

String WebsiteDataStore::defaultApplicationCacheDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "ApplicationCache");
}

String WebsiteDataStore::defaultCacheStorageDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "CacheStorage");
}

String WebsiteDataStore::defaultNetworkCacheDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "NetworkCache");
}

String WebsiteDataStore::defaultIndexedDBDatabaseDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "IndexedDB");
}

String WebsiteDataStore::defaultServiceWorkerRegistrationDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "ServiceWorkers");
}

String WebsiteDataStore::defaultLocalStorageDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "LocalStorage");
}

String WebsiteDataStore::defaultMediaKeysStorageDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "MediaKeyStorage");
}

String WebsiteDataStore::defaultWebSQLDatabaseDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "WebSQL");
}

String WebsiteDataStore::defaultResourceLoadStatisticsDirectory()
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), "ResourceLoadStatistics");
}

String WebsiteDataStore::cacheDirectoryFileSystemRepresentation(const String& directoryName)
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), directoryName);
}

String WebsiteDataStore::websiteDataDirectoryFileSystemRepresentation(const String& directoryName)
{
    return WebCore::FileSystem::pathByAppendingComponent(WebCore::FileSystem::localUserSpecificStorageDirectory(), directoryName);
}

WebKit::WebsiteDataStore::Configuration WebsiteDataStore::defaultDataStoreConfiguration()
{
    WebKit::WebsiteDataStore::Configuration configuration;

    configuration.applicationCacheDirectory = defaultApplicationCacheDirectory();
    configuration.networkCacheDirectory = defaultNetworkCacheDirectory();
    configuration.webSQLDatabaseDirectory = defaultWebSQLDatabaseDirectory();
    configuration.localStorageDirectory = defaultLocalStorageDirectory();
    configuration.mediaKeysStorageDirectory = defaultMediaKeysStorageDirectory();
    configuration.resourceLoadStatisticsDirectory = defaultResourceLoadStatisticsDirectory();

    return configuration;
}

} // namespace API
