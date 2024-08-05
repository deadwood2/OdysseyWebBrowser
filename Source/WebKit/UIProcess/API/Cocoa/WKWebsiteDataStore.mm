/*
 * Copyright (C) 2014-2017 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WKWebsiteDataStoreInternal.h"

#if WK_API_ENABLED

#import "WKHTTPCookieStoreInternal.h"
#import "WKNSArray.h"
#import "WKWebsiteDataRecordInternal.h"
#import "WebResourceLoadStatisticsStore.h"
#import "WebResourceLoadStatisticsTelemetry.h"
#import "WebsiteDataFetchOption.h"
#import "_WKWebsiteDataStoreConfiguration.h"
#import <WebCore/URL.h>
#import <wtf/BlockPtr.h>

using namespace WebCore;

@implementation WKWebsiteDataStore

+ (WKWebsiteDataStore *)defaultDataStore
{
    return WebKit::wrapper(API::WebsiteDataStore::defaultDataStore().get());
}

+ (WKWebsiteDataStore *)nonPersistentDataStore
{
    return [WebKit::wrapper(API::WebsiteDataStore::createNonPersistentDataStore().leakRef()) autorelease];
}

- (void)dealloc
{
    _websiteDataStore->API::WebsiteDataStore::~WebsiteDataStore();

    [super dealloc];
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    if (!(self = [super init]))
        return nil;

    RetainPtr<WKWebsiteDataStore> dataStore;
    if ([coder decodeBoolForKey:@"isDefaultDataStore"])
        dataStore = [WKWebsiteDataStore defaultDataStore];
    else
        dataStore = [WKWebsiteDataStore nonPersistentDataStore];

    [self release];

    return dataStore.leakRef();
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    if (self == [WKWebsiteDataStore defaultDataStore]) {
        [coder encodeBool:YES forKey:@"isDefaultDataStore"];
        return;
    }

    ASSERT(!self.persistent);
}

- (BOOL)isPersistent
{
    return _websiteDataStore->isPersistent();
}

+ (NSSet *)allWebsiteDataTypes
{
    static dispatch_once_t onceToken;
    static NSSet *allWebsiteDataTypes;
    dispatch_once(&onceToken, ^{
        allWebsiteDataTypes = [[NSSet alloc] initWithArray:@[ WKWebsiteDataTypeDiskCache, WKWebsiteDataTypeMemoryCache, WKWebsiteDataTypeOfflineWebApplicationCache, WKWebsiteDataTypeCookies, WKWebsiteDataTypeSessionStorage, WKWebsiteDataTypeLocalStorage, WKWebsiteDataTypeIndexedDBDatabases, WKWebsiteDataTypeWebSQLDatabases ]];
    });

    return allWebsiteDataTypes;
}

- (WKHTTPCookieStore *)httpCookieStore
{
    return WebKit::wrapper(_websiteDataStore->httpCookieStore());
}

static std::chrono::system_clock::time_point toSystemClockTime(NSDate *date)
{
    ASSERT(date);
    using namespace std::chrono;

    return system_clock::time_point(duration_cast<system_clock::duration>(duration<double>(date.timeIntervalSince1970)));
}

- (void)fetchDataRecordsOfTypes:(NSSet *)dataTypes completionHandler:(void (^)(NSArray<WKWebsiteDataRecord *> *))completionHandler
{
    [self _fetchDataRecordsOfTypes:dataTypes withOptions:0 completionHandler:completionHandler];
}

- (void)removeDataOfTypes:(NSSet *)dataTypes modifiedSince:(NSDate *)date completionHandler:(void (^)(void))completionHandler
{
    auto completionHandlerCopy = makeBlockPtr(completionHandler);
    _websiteDataStore->websiteDataStore().removeData(WebKit::toWebsiteDataTypes(dataTypes), toSystemClockTime(date ? date : [NSDate distantPast]), [completionHandlerCopy] {
        completionHandlerCopy();
    });
}

static Vector<WebKit::WebsiteDataRecord> toWebsiteDataRecords(NSArray *dataRecords)
{
    Vector<WebKit::WebsiteDataRecord> result;

    for (WKWebsiteDataRecord *dataRecord in dataRecords)
        result.append(dataRecord->_websiteDataRecord->websiteDataRecord());

    return result;
}

- (void)removeDataOfTypes:(NSSet *)dataTypes forDataRecords:(NSArray *)dataRecords completionHandler:(void (^)(void))completionHandler
{
    auto completionHandlerCopy = makeBlockPtr(completionHandler);

    _websiteDataStore->websiteDataStore().removeData(WebKit::toWebsiteDataTypes(dataTypes), toWebsiteDataRecords(dataRecords), [completionHandlerCopy] {
        completionHandlerCopy();
    });
}

#pragma mark WKObject protocol implementation

- (API::Object&)_apiObject
{
    return *_websiteDataStore;
}

@end

@implementation WKWebsiteDataStore (WKPrivate)

+ (NSSet<NSString *> *)_allWebsiteDataTypesIncludingPrivate
{
    static dispatch_once_t onceToken;
    static NSSet *allWebsiteDataTypes;
    dispatch_once(&onceToken, ^ {
        auto *privateTypes = @[_WKWebsiteDataTypeHSTSCache, _WKWebsiteDataTypeMediaKeys, _WKWebsiteDataTypeSearchFieldRecentSearches, _WKWebsiteDataTypeResourceLoadStatistics, _WKWebsiteDataTypeCredentials
#if !TARGET_OS_IPHONE
        , _WKWebsiteDataTypePlugInData
#endif
        ];

        allWebsiteDataTypes = [[[self allWebsiteDataTypes] setByAddingObjectsFromArray:privateTypes] retain];
    });

    return allWebsiteDataTypes;
}

+ (BOOL)_defaultDataStoreExists
{
    return API::WebsiteDataStore::defaultDataStoreExists();
}

- (instancetype)_initWithConfiguration:(_WKWebsiteDataStoreConfiguration *)configuration
{
    if (!(self = [super init]))
        return nil;

    auto config = API::WebsiteDataStore::defaultDataStoreConfiguration();

    if (configuration._webStorageDirectory)
        config.localStorageDirectory = configuration._webStorageDirectory.path;
    if (configuration._webSQLDatabaseDirectory)
        config.webSQLDatabaseDirectory = configuration._webSQLDatabaseDirectory.path;
    if (configuration._indexedDBDatabaseDirectory)
        config.indexedDBDatabaseDirectory = configuration._indexedDBDatabaseDirectory.path;
    if (configuration._cookieStorageFile)
        config.cookieStorageFile = configuration._cookieStorageFile.path;

    API::Object::constructInWrapper<API::WebsiteDataStore>(self, config, WebCore::SessionID::generatePersistentSessionID());

    return self;
}

- (void)_fetchDataRecordsOfTypes:(NSSet<NSString *> *)dataTypes withOptions:(_WKWebsiteDataStoreFetchOptions)options completionHandler:(void (^)(NSArray<WKWebsiteDataRecord *> *))completionHandler
{
    auto completionHandlerCopy = makeBlockPtr(completionHandler);

    OptionSet<WebKit::WebsiteDataFetchOption> fetchOptions;
    if (options & _WKWebsiteDataStoreFetchOptionComputeSizes)
        fetchOptions |= WebKit::WebsiteDataFetchOption::ComputeSizes;

    _websiteDataStore->websiteDataStore().fetchData(WebKit::toWebsiteDataTypes(dataTypes), fetchOptions, [completionHandlerCopy = WTFMove(completionHandlerCopy)](auto websiteDataRecords) {
        Vector<RefPtr<API::Object>> elements;
        elements.reserveInitialCapacity(websiteDataRecords.size());

        for (auto& websiteDataRecord : websiteDataRecords)
            elements.uncheckedAppend(API::WebsiteDataRecord::create(WTFMove(websiteDataRecord)));

        completionHandlerCopy(wrapper(API::Array::create(WTFMove(elements))));
    });
}

- (BOOL)_resourceLoadStatisticsEnabled
{
    return _websiteDataStore->websiteDataStore().resourceLoadStatisticsEnabled();
}

- (void)_setResourceLoadStatisticsEnabled:(BOOL)enabled
{
    _websiteDataStore->websiteDataStore().setResourceLoadStatisticsEnabled(enabled);
}

- (void)_resourceLoadStatisticsSetLastSeen:(double)seconds forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;
    
    store->setLastSeen(URL(URL(), host), Seconds { seconds });
}

- (void)_resourceLoadStatisticsSetIsPrevalentResource:(BOOL)value forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    if (value)
        store->setPrevalentResource(URL(URL(), host));
    else
        store->clearPrevalentResource(URL(URL(), host));
}

- (void)_resourceLoadStatisticsIsPrevalentResource:(NSString *)host completionHandler:(void (^)(BOOL))completionHandler
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        completionHandler(NO);
        return;
    }

    auto completionHandlerCopy = makeBlockPtr(completionHandler);
    store->isPrevalentResource(URL(URL(), host), [completionHandlerCopy](bool isPrevalentResource) {
        completionHandlerCopy(isPrevalentResource);
    });
}

- (void)_resourceLoadStatisticsSetHadUserInteraction:(BOOL)value forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    if (value)
        store->logUserInteraction(URL(URL(), host));
    else
        store->clearUserInteraction(URL(URL(), host));
}

- (void)_resourceLoadStatisticsHadUserInteraction:(NSString *)host completionHandler:(void (^)(BOOL))completionHandler
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        completionHandler(NO);
        return;
    }

    auto completionHandlerCopy = makeBlockPtr(completionHandler);
    store->hasHadUserInteraction(URL(URL(), host), [completionHandlerCopy](bool hasHadUserInteraction) {
        completionHandlerCopy(hasHadUserInteraction);
    });
}

- (void)_resourceLoadStatisticsSetIsGrandfathered:(BOOL)value forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setGrandfathered(URL(URL(), host), value);
}

- (void)_resourceLoadStatisticsIsGrandfathered:(NSString *)host completionHandler:(void (^)(BOOL))completionHandler
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        completionHandler(NO);
        return;
    }

    auto completionHandlerCopy = makeBlockPtr(completionHandler);
    store->isGrandfathered(URL(URL(), host), [completionHandlerCopy](bool isGrandfathered) {
        completionHandlerCopy(isGrandfathered);
    });
}

- (void)_resourceLoadStatisticsSetSubframeUnderTopFrameOrigin:(NSString *)topFrameHostName forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setSubframeUnderTopFrameOrigin(URL(URL(), host), URL(URL(), topFrameHostName));
}

- (void)_resourceLoadStatisticsSetSubresourceUnderTopFrameOrigin:(NSString *)topFrameHostName forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setSubresourceUnderTopFrameOrigin(URL(URL(), host), URL(URL(), topFrameHostName));
}

- (void)_resourceLoadStatisticsSetSubresourceUniqueRedirectTo:(NSString *)hostNameRedirectedTo forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setSubresourceUniqueRedirectTo(URL(URL(), host), URL(URL(), hostNameRedirectedTo));
}

- (void)_resourceLoadStatisticsSetTimeToLiveUserInteraction:(double)seconds
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setTimeToLiveUserInteraction(Seconds { seconds });
}

- (void)_resourceLoadStatisticsSetTimeToLiveCookiePartitionFree:(double)seconds
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setTimeToLiveCookiePartitionFree(Seconds { seconds });
}

- (void)_resourceLoadStatisticsSetMinimumTimeBetweenDataRecordsRemoval:(double)seconds
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setMinimumTimeBetweenDataRecordsRemoval(Seconds { seconds });
}

- (void)_resourceLoadStatisticsSetGrandfatheringTime:(double)seconds
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setGrandfatheringTime(Seconds {seconds });
}

- (void)_resourceLoadStatisticsSetMaxStatisticsEntries:(size_t)entries
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setMaxStatisticsEntries(entries);
}

- (void)_resourceLoadStatisticsSetPruneEntriesDownTo:(size_t)entries
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setPruneEntriesDownTo(entries);
}

- (void)_resourceLoadStatisticsProcessStatisticsAndDataRecords
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleStatisticsAndDataRecordsProcessing();
}

- (void)_resourceLoadStatisticsUpdateCookiePartitioning
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleCookiePartitioningUpdate();
}

- (void)_resourceLoadStatisticsSetShouldPartitionCookies:(BOOL)value forHost:(NSString *)host
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    if (value)
        store->scheduleCookiePartitioningUpdateForDomains({ }, { host }, WebKit::ShouldClearFirst::No);
    else
        store->scheduleCookiePartitioningUpdateForDomains({ host }, { }, WebKit::ShouldClearFirst::No);
}

- (void)_resourceLoadStatisticsSubmitTelemetry
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->submitTelemetry();
}

- (void)_resourceLoadStatisticsSetNotifyPagesWhenDataRecordsWereScanned:(BOOL)value
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setNotifyPagesWhenDataRecordsWereScanned(value);
}

- (void)_resourceLoadStatisticsSetShouldClassifyResourcesBeforeDataRecordsRemoval:(BOOL)value
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setShouldClassifyResourcesBeforeDataRecordsRemoval(value);
}

- (void)_resourceLoadStatisticsSetNotifyPagesWhenTelemetryWasCaptured:(BOOL)value
{
    WebKit::WebResourceLoadStatisticsTelemetry::setNotifyPagesWhenTelemetryWasCaptured(value);
}

- (void)_resourceLoadStatisticsSetShouldSubmitTelemetry:(BOOL)value
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setShouldSubmitTelemetry(value);
}

- (void)_resourceLoadStatisticsClearInMemoryAndPersistentStore
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleClearInMemoryAndPersistent(WebKit::WebResourceLoadStatisticsStore::ShouldGrandfather::Yes);
}

- (void)_resourceLoadStatisticsClearInMemoryAndPersistentStoreModifiedSinceHours:(unsigned)hours
{
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleClearInMemoryAndPersistent(std::chrono::system_clock::now() - std::chrono::hours(hours), WebKit::WebResourceLoadStatisticsStore::ShouldGrandfather::Yes);
}

- (void)_resourceLoadStatisticsResetToConsistentState
{
    WebKit::WebResourceLoadStatisticsTelemetry::setNotifyPagesWhenTelemetryWasCaptured(false);

    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->resetParametersToDefaultValues();
    store->scheduleClearInMemory();
}

- (void)_setResourceLoadStatisticsTestingCallback:(void (^)(WKWebsiteDataStore *, NSString *))callback
{
    if (callback) {
        _websiteDataStore->websiteDataStore().enableResourceLoadStatisticsAndSetTestingCallback([callback = makeBlockPtr(callback), self](const String& event) {
            callback(self, (NSString *)event);
        });
        return;
    }

    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setStatisticsTestingCallback(nullptr);
}

@end

#endif // WK_API_ENABLED
