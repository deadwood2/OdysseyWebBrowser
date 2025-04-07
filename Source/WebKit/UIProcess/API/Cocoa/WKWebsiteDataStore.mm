/*
 * Copyright (C) 2014-2019 Apple Inc. All rights reserved.
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

#import "APIString.h"
#import "AuthenticationChallengeDispositionCocoa.h"
#import "CompletionHandlerCallChecker.h"
#import "WKHTTPCookieStoreInternal.h"
#import "WKNSArray.h"
#import "WKNSURLAuthenticationChallenge.h"
#import "WKWebViewInternal.h"
#import "WKWebsiteDataRecordInternal.h"
#import "WebPageProxy.h"
#import "WebResourceLoadStatisticsStore.h"
#import "WebResourceLoadStatisticsTelemetry.h"
#import "WebsiteDataFetchOption.h"
#import "_WKWebsiteDataStoreConfiguration.h"
#import "_WKWebsiteDataStoreDelegate.h"
#import <WebCore/Credential.h>
#import <WebKit/ServiceWorkerProcessProxy.h>
#import <wtf/BlockPtr.h>
#import <wtf/URL.h>
#import <wtf/WeakObjCPtr.h>

class WebsiteDataStoreClient final : public WebKit::WebsiteDataStoreClient {
public:
    explicit WebsiteDataStoreClient(id <_WKWebsiteDataStoreDelegate> delegate)
        : m_delegate(delegate)
        , m_hasRequestStorageSpaceSelector([m_delegate.get() respondsToSelector:@selector(requestStorageSpace: frameOrigin: quota: currentSize: spaceRequired: decisionHandler:)])
        , m_hasAuthenticationChallengeSelector([m_delegate.get() respondsToSelector:@selector(didReceiveAuthenticationChallenge: completionHandler:)])
    {
    }

private:
    void requestStorageSpace(const WebCore::SecurityOriginData& topOrigin, const WebCore::SecurityOriginData& frameOrigin, uint64_t quota, uint64_t currentSize, uint64_t spaceRequired, CompletionHandler<void(Optional<uint64_t>)>&& completionHandler) final
    {
        if (!m_hasRequestStorageSpaceSelector || !m_delegate) {
            completionHandler({ });
            return;
        }

        auto checker = WebKit::CompletionHandlerCallChecker::create(m_delegate.getAutoreleased(), @selector(requestStorageSpace: frameOrigin: quota: currentSize: spaceRequired: decisionHandler:));
        auto decisionHandler = makeBlockPtr([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)](unsigned long long quota) mutable {
            if (checker->completionHandlerHasBeenCalled())
                return;
            checker->didCallCompletionHandler();
            completionHandler(quota);
        });

        URL mainFrameURL { URL(), topOrigin.toString() };
        URL frameURL { URL(), frameOrigin.toString() };

        [m_delegate.getAutoreleased() requestStorageSpace:mainFrameURL frameOrigin:frameURL quota:quota currentSize:currentSize spaceRequired:spaceRequired decisionHandler:decisionHandler.get()];
    }

    void didReceiveAuthenticationChallenge(Ref<WebKit::AuthenticationChallengeProxy>&& challenge) final
    {
        if (!m_hasAuthenticationChallengeSelector || !m_delegate) {
            challenge->listener().completeChallenge(WebKit::AuthenticationChallengeDisposition::PerformDefaultHandling);
            return;
        }

        auto nsURLChallenge = wrapper(challenge);
        auto checker = WebKit::CompletionHandlerCallChecker::create(m_delegate.getAutoreleased(), @selector(didReceiveAuthenticationChallenge: completionHandler:));
        auto completionHandler = makeBlockPtr([challenge = WTFMove(challenge), checker = WTFMove(checker)](NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential) mutable {
            if (checker->completionHandlerHasBeenCalled())
                return;
            checker->didCallCompletionHandler();
            challenge->listener().completeChallenge(WebKit::toAuthenticationChallengeDisposition(disposition), WebCore::Credential(credential));
        });

        [m_delegate.getAutoreleased() didReceiveAuthenticationChallenge:nsURLChallenge completionHandler:completionHandler.get()];
    }

    WeakObjCPtr<id <_WKWebsiteDataStoreDelegate> > m_delegate;
    bool m_hasRequestStorageSpaceSelector { false };
    bool m_hasAuthenticationChallengeSelector { false };
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
        auto *privateTypes = @[_WKWebsiteDataTypeHSTSCache, _WKWebsiteDataTypeMediaKeys, _WKWebsiteDataTypeSearchFieldRecentSearches, _WKWebsiteDataTypeResourceLoadStatistics, _WKWebsiteDataTypeCredentials, _WKWebsiteDataTypeAdClickAttributions
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

    auto config = configuration.isPersistent ? API::WebsiteDataStore::defaultDataStoreConfiguration() : WebKit::WebsiteDataStoreConfiguration::create();

    RELEASE_ASSERT(config->isPersistent() == configuration.isPersistent);

    if (configuration.isPersistent) {
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
        if (configuration.networkCacheDirectory)
            config->setNetworkCacheDirectory(configuration.networkCacheDirectory.path);
        if (configuration.deviceIdHashSaltsStorageDirectory)
            config->setDeviceIdHashSaltsStorageDirectory(configuration.deviceIdHashSaltsStorageDirectory.path);
        if (configuration.applicationCacheDirectory)
            config->setApplicationCacheDirectory(configuration.applicationCacheDirectory.path);
        if (configuration.applicationCacheFlatFileSubdirectoryName)
            config->setApplicationCacheFlatFileSubdirectoryName(configuration.applicationCacheFlatFileSubdirectoryName);
        if (configuration.mediaCacheDirectory)
            config->setMediaCacheDirectory(configuration.mediaCacheDirectory.path);
        if (configuration.mediaKeysStorageDirectory)
            config->setMediaKeysStorageDirectory(configuration.mediaKeysStorageDirectory.path);
    } else {
        RELEASE_ASSERT(!configuration._webStorageDirectory);
        RELEASE_ASSERT(!configuration._webSQLDatabaseDirectory);
        RELEASE_ASSERT(!configuration._indexedDBDatabaseDirectory);
        RELEASE_ASSERT(!configuration._cookieStorageFile);
        RELEASE_ASSERT(!configuration._resourceLoadStatisticsDirectory);
        RELEASE_ASSERT(!configuration._cacheStorageDirectory);
        RELEASE_ASSERT(!configuration._serviceWorkerRegistrationDirectory);
        RELEASE_ASSERT(!configuration.networkCacheDirectory);
        RELEASE_ASSERT(!configuration.deviceIdHashSaltsStorageDirectory);
        RELEASE_ASSERT(!configuration.applicationCacheDirectory);
        RELEASE_ASSERT(!configuration.mediaCacheDirectory);
        RELEASE_ASSERT(!configuration.mediaKeysStorageDirectory);
    }

    if (configuration.sourceApplicationBundleIdentifier)
        config->setSourceApplicationBundleIdentifier(configuration.sourceApplicationBundleIdentifier);
    if (configuration.sourceApplicationSecondaryIdentifier)
        config->setSourceApplicationSecondaryIdentifier(configuration.sourceApplicationSecondaryIdentifier);
    if (configuration.httpProxy)
        config->setHTTPProxy(configuration.httpProxy);
    if (configuration.httpsProxy)
        config->setHTTPSProxy(configuration.httpsProxy);
    config->setDeviceManagementRestrictionsEnabled(configuration.deviceManagementRestrictionsEnabled);
    config->setAllLoadsBlockedByDeviceManagementRestrictionsForTesting(configuration.allLoadsBlockedByDeviceManagementRestrictionsForTesting);

    auto sessionID = configuration.isPersistent ? PAL::SessionID::generatePersistentSessionID() : PAL::SessionID::generateEphemeralSessionID();

    API::Object::constructInWrapper<API::WebsiteDataStore>(self, WTFMove(config), sessionID);

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

- (NSUInteger)_perOriginStorageQuota
{
    return _websiteDataStore->websiteDataStore().perOriginStorageQuota();
}

- (void)_setPerOriginStorageQuota:(NSUInteger)size
{
    _websiteDataStore->websiteDataStore().setPerOriginStorageQuota(size);
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

- (NSString *)_sourceApplicationBundleIdentifier
{
    return _websiteDataStore->websiteDataStore().sourceApplicationBundleIdentifier();
}

- (void)_setSourceApplicationBundleIdentifier:(NSString *)identifier
{
    if (!_websiteDataStore->websiteDataStore().setSourceApplicationBundleIdentifier(identifier))
        [NSException raise:NSGenericException format:@"_setSourceApplicationBundleIdentifier cannot be called after networking has begun"];
}

- (NSString *)_sourceApplicationSecondaryIdentifier
{
    return _websiteDataStore->websiteDataStore().sourceApplicationSecondaryIdentifier();
}

- (void)_setSourceApplicationSecondaryIdentifier:(NSString *)identifier
{
    if (!_websiteDataStore->websiteDataStore().setSourceApplicationSecondaryIdentifier(identifier))
        [NSException raise:NSGenericException format:@"_setSourceApplicationSecondaryIdentifier cannot be called after networking has begun"];
}

- (void)_setAllowsTLSFallback:(BOOL)allows
{
    if (!_websiteDataStore->websiteDataStore().setAllowsTLSFallback(allows))
        [NSException raise:NSGenericException format:@"_setAllowsTLSFallback cannot be called after networking has begun"];
}

- (BOOL)_allowsTLSFallback
{
    return _websiteDataStore->websiteDataStore().allowsTLSFallback();
}

- (NSDictionary *)_proxyConfiguration
{
    return (__bridge NSDictionary *)_websiteDataStore->websiteDataStore().proxyConfiguration();
}

- (NSURL *)_indexedDBDatabaseDirectory
{
    return [NSURL fileURLWithPath:_websiteDataStore->indexedDBDatabaseDirectory() isDirectory:YES];
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
        _websiteDataStore->websiteDataStore().setStatisticsTestingCallback([callback = makeBlockPtr(callback), self](const String& event) {
            callback(self, event);
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

- (void)_scheduleCookieBlockingUpdate:(void (^)(void))completionHandler
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    _websiteDataStore->websiteDataStore().scheduleCookieBlockingUpdate([completionHandler = makeBlockPtr(completionHandler)]() {
        completionHandler();
    });
#else
    completionHandler();
#endif
}

- (void)_setPrevalentDomain:(NSURL *)domain completionHandler:(void (^)(void))completionHandler
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    _websiteDataStore->websiteDataStore().setPrevalentResource(URL(domain), [completionHandler = makeBlockPtr(completionHandler)]() {
        completionHandler();
    });
#else
    completionHandler();
#endif
}

- (void)_getIsPrevalentDomain:(NSURL *)domain completionHandler:(void (^)(BOOL))completionHandler
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    _websiteDataStore->websiteDataStore().isPrevalentResource(URL(domain), [completionHandler = makeBlockPtr(completionHandler)](bool enabled) {
        completionHandler(enabled);
    });
#else
    completionHandler(NO);
#endif
}

- (void)_clearPrevalentDomain:(NSURL *)domain completionHandler:(void (^)(void))completionHandler
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    _websiteDataStore->websiteDataStore().clearPrevalentResource(URL(domain), [completionHandler = makeBlockPtr(completionHandler)]() {
        completionHandler();
    });
#else
    completionHandler();
#endif
}

- (void)_processStatisticsAndDataRecords:(void (^)(void))completionHandler
{
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    _websiteDataStore->websiteDataStore().scheduleStatisticsAndDataRecordsProcessing([completionHandler = makeBlockPtr(completionHandler)]() {
        completionHandler();
    });
#else
    completionHandler();
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
