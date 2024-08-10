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

#import "APIString.h"
#import "CompletionHandlerCallChecker.h"
#import "WKHTTPCookieStoreInternal.h"
#import "WKNSArray.h"
#import "WKWebViewInternal.h"
#import "WKWebsiteDataRecordInternal.h"
#import "WebPageProxy.h"
#import "WebResourceLoadStatisticsStore.h"
#import "WebResourceLoadStatisticsTelemetry.h"
#import "WebsiteDataFetchOption.h"
#import "_WKWebsiteDataStoreConfiguration.h"
#import "_WKWebsiteDataStoreDelegate.h"
#import <WebKit/ServiceWorkerProcessProxy.h>
#import <wtf/BlockPtr.h>
#import <wtf/URL.h>
#import <wtf/WeakObjCPtr.h>

class WebsiteDataStoreClient : public WebKit::WebsiteDataStoreClient {
public:
    explicit WebsiteDataStoreClient(id <_WKWebsiteDataStoreDelegate> delegate)
        : m_delegate(delegate)
        , m_hasRequestCacheStorageSpaceSelector([m_delegate.get() respondsToSelector:@selector(requestCacheStorageSpace: frameOrigin: quota: currentSize: spaceRequired: decisionHandler:)])
    {
    }

private:
    void requestCacheStorageSpace(const WebCore::SecurityOriginData& topOrigin, const WebCore::SecurityOriginData& frameOrigin, uint64_t quota, uint64_t currentSize, uint64_t spaceRequired, CompletionHandler<void(Optional<uint64_t>)>&& completionHandler) final
    {
        if (!m_hasRequestCacheStorageSpaceSelector || !m_delegate) {
            completionHandler({ });
            return;
        }

        auto checker = WebKit::CompletionHandlerCallChecker::create(m_delegate.getAutoreleased(), @selector(requestCacheStorageSpace: frameOrigin: quota: currentSize: spaceRequired: decisionHandler:));
        auto decisionHandler = makeBlockPtr([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)](unsigned long long quota) mutable {
            if (checker->completionHandlerHasBeenCalled())
                return;
            checker->didCallCompletionHandler();
            completionHandler(quota);
        });

        URL mainFrameURL { URL(), topOrigin.toString() };
        URL frameURL { URL(), frameOrigin.toString() };

        [m_delegate.getAutoreleased() requestCacheStorageSpace:mainFrameURL frameOrigin:frameURL quota:quota currentSize:currentSize spaceRequired:spaceRequired decisionHandler:decisionHandler.get()];
    }

    WeakObjCPtr<id <_WKWebsiteDataStoreDelegate> > m_delegate;
    bool m_hasRequestCacheStorageSpaceSelector { false };
};

@implementation WKWebsiteDataStore

+ (WKWebsiteDataStore *)defaultDataStore
{
    return wrapper(API::WebsiteDataStore::defaultDataStore());
}

+ (WKWebsiteDataStore *)nonPersistentDataStore
{
    return wrapper(API::WebsiteDataStore::createNonPersistentDataStore());
}

- (void)dealloc
{
    _websiteDataStore->API::WebsiteDataStore::~WebsiteDataStore();

    [super dealloc];
}

+ (BOOL)supportsSecureCoding
{
    return YES;
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
        allWebsiteDataTypes = [[NSSet alloc] initWithArray:@[ WKWebsiteDataTypeDiskCache, WKWebsiteDataTypeFetchCache, WKWebsiteDataTypeMemoryCache, WKWebsiteDataTypeOfflineWebApplicationCache, WKWebsiteDataTypeCookies, WKWebsiteDataTypeSessionStorage, WKWebsiteDataTypeLocalStorage, WKWebsiteDataTypeIndexedDBDatabases, WKWebsiteDataTypeServiceWorkerRegistrations, WKWebsiteDataTypeWebSQLDatabases ]];
    });

    return allWebsiteDataTypes;
}

- (WKHTTPCookieStore *)httpCookieStore
{
    return wrapper(_websiteDataStore->httpCookieStore());
}

static WallTime toSystemClockTime(NSDate *date)
{
    ASSERT(date);
    return WallTime::fromRawSeconds(date.timeIntervalSince1970);
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

+ (void)_deleteDefaultDataStoreForTesting
{
    return API::WebsiteDataStore::deleteDefaultDataStoreForTesting();
}

- (instancetype)_initWithConfiguration:(_WKWebsiteDataStoreConfiguration *)configuration
{
    if (!(self = [super init]))
        return nil;

    auto config = API::WebsiteDataStore::defaultDataStoreConfiguration();

    if (configuration._webStorageDirectory)
        config->setLocalStorageDirectory(configuration._webStorageDirectory.path);
    if (configuration._webSQLDatabaseDirectory)
        config->setWebSQLDatabaseDirectory(configuration._webSQLDatabaseDirectory.path);
    if (configuration._indexedDBDatabaseDirectory)
        config->setIndexedDBDatabaseDirectory(configuration._indexedDBDatabaseDirectory.path);
    if (configuration._cookieStorageFile)
        config->setCookieStorageFile(configuration._cookieStorageFile.path);
    if (configuration._resourceLoadStatisticsDirectory)
        config->setResourceLoadStatisticsDirectory(configuration._resourceLoadStatisticsDirectory.path);
    if (configuration._cacheStorageDirectory)
        config->setCacheStorageDirectory(configuration._cacheStorageDirectory.path);
    if (configuration._serviceWorkerRegistrationDirectory)
        config->setServiceWorkerRegistrationDirectory(configuration._serviceWorkerRegistrationDirectory.path);
    if (configuration.sourceApplicationBundleIdentifier)
        config->setSourceApplicationBundleIdentifier(configuration.sourceApplicationBundleIdentifier);
    if (configuration.sourceApplicationSecondaryIdentifier)
        config->setSourceApplicationSecondaryIdentifier(configuration.sourceApplicationSecondaryIdentifier);
    if (configuration.httpProxy)
        config->setHTTPProxy(configuration.httpProxy);
    if (configuration.httpsProxy)
        config->setHTTPSProxy(configuration.httpsProxy);

    API::Object::constructInWrapper<API::WebsiteDataStore>(self, WTFMove(config), PAL::SessionID::generatePersistentSessionID());

    return self;
}

- (void)_fetchDataRecordsOfTypes:(NSSet<NSString *> *)dataTypes withOptions:(_WKWebsiteDataStoreFetchOptions)options completionHandler:(void (^)(NSArray<WKWebsiteDataRecord *> *))completionHandler
{
    auto completionHandlerCopy = makeBlockPtr(completionHandler);

    OptionSet<WebKit::WebsiteDataFetchOption> fetchOptions;
    if (options & _WKWebsiteDataStoreFetchOptionComputeSizes)
        fetchOptions.add(WebKit::WebsiteDataFetchOption::ComputeSizes);

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

- (BOOL)_resourceLoadStatisticsDebugMode
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    return _websiteDataStore->websiteDataStore().resourceLoadStatisticsDebugMode();
#else
    return NO;
#endif
}

- (void)_setResourceLoadStatisticsDebugMode:(BOOL)enabled
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    _websiteDataStore->websiteDataStore().setResourceLoadStatisticsDebugMode(enabled);
#else
    UNUSED_PARAM(enabled);
#endif
}

- (NSUInteger)_cacheStoragePerOriginQuota
{
    return _websiteDataStore->websiteDataStore().cacheStoragePerOriginQuota();
}

- (void)_setCacheStoragePerOriginQuota:(NSUInteger)size
{
    _websiteDataStore->websiteDataStore().setCacheStoragePerOriginQuota(size);
}

- (NSString *)_cacheStorageDirectory
{
    return _websiteDataStore->websiteDataStore().cacheStorageDirectory();
}

- (void)_setCacheStorageDirectory:(NSString *)directory
{
    _websiteDataStore->websiteDataStore().setCacheStorageDirectory(directory);
}

- (NSString *)_serviceWorkerRegistrationDirectory
{
    return _websiteDataStore->websiteDataStore().serviceWorkerRegistrationDirectory();
}

- (void)_setServiceWorkerRegistrationDirectory:(NSString *)directory
{
    _websiteDataStore->websiteDataStore().setServiceWorkerRegistrationDirectory(directory);
}

- (void)_setBoundInterfaceIdentifier:(NSString *)identifier
{
    _websiteDataStore->websiteDataStore().setBoundInterfaceIdentifier(identifier);
}

- (NSString *)_boundInterfaceIdentifier
{
    return _websiteDataStore->websiteDataStore().boundInterfaceIdentifier();
}

- (void)_setAllowsCellularAccess:(BOOL)allows
{
    _websiteDataStore->websiteDataStore().setAllowsCellularAccess(allows ? WebKit::AllowsCellularAccess::Yes : WebKit::AllowsCellularAccess::No);
}

- (BOOL)_allowsCellularAccess
{
    return _websiteDataStore->websiteDataStore().allowsCellularAccess() == WebKit::AllowsCellularAccess::Yes;
}

- (void)_setProxyConfiguration:(NSDictionary *)configuration
{
    _websiteDataStore->websiteDataStore().setProxyConfiguration((__bridge CFDictionaryRef)configuration);
}

- (NSDictionary *)_proxyConfiguration
{
    return (__bridge NSDictionary *)_websiteDataStore->websiteDataStore().proxyConfiguration();
}

- (void)_resourceLoadStatisticsSetShouldSubmitTelemetry:(BOOL)value
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    auto* store = _websiteDataStore->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setShouldSubmitTelemetry(value);
#endif
}

- (void)_setResourceLoadStatisticsTestingCallback:(void (^)(WKWebsiteDataStore *, NSString *))callback
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (!_websiteDataStore->isPersistent())
        return;

    if (callback) {
        _websiteDataStore->websiteDataStore().enableResourceLoadStatisticsAndSetTestingCallback([callback = makeBlockPtr(callback), self](const String& event) {
            callback(self, (NSString *)event);
        });
        return;
    }

    _websiteDataStore->websiteDataStore().setStatisticsTestingCallback(nullptr);
#endif
}

+ (void)_allowWebsiteDataRecordsForAllOrigins
{
    WebKit::WebsiteDataStore::allowWebsiteDataRecordsForAllOrigins();
}

- (void)_getAllStorageAccessEntriesFor:(WKWebView *)webView completionHandler:(void (^)(NSArray<NSString *> *domains))completionHandler
{
    if (!webView) {
        completionHandler({ });
        return;
    }

    auto* webPageProxy = [webView _page];
    if (!webPageProxy) {
        completionHandler({ });
        return;
    }

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    _websiteDataStore->websiteDataStore().getAllStorageAccessEntries(webPageProxy->pageID(), [completionHandler = makeBlockPtr(completionHandler)](auto domains) {
        Vector<RefPtr<API::Object>> apiDomains;
        apiDomains.reserveInitialCapacity(domains.size());
        for (auto& domain : domains)
            apiDomains.uncheckedAppend(API::String::create(domain));
        completionHandler(wrapper(API::Array::create(WTFMove(apiDomains))));
    });
#else
    completionHandler({ });
#endif
}

- (bool)_hasRegisteredServiceWorker
{
    return WebKit::ServiceWorkerProcessProxy::hasRegisteredServiceWorkers(_websiteDataStore->websiteDataStore().serviceWorkerRegistrationDirectory());
}

- (id <_WKWebsiteDataStoreDelegate>)_delegate
{
    return _delegate.get();
}

- (void)set_delegate:(id <_WKWebsiteDataStoreDelegate>)delegate
{
    _delegate = delegate;
    _websiteDataStore->websiteDataStore().setClient(makeUniqueRef<WebsiteDataStoreClient>(delegate));
}

@end

#endif // WK_API_ENABLED
