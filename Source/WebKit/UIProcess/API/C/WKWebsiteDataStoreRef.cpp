/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "WKWebsiteDataStoreRef.h"

#include "APIArray.h"
#include "APIWebsiteDataStore.h"
#include "WKAPICast.h"
#include "WKSecurityOriginRef.h"
#include "WebResourceLoadStatisticsStore.h"
#include "WebResourceLoadStatisticsTelemetry.h"
#include "WebsiteData.h"
#include "WebsiteDataFetchOption.h"
#include "WebsiteDataRecord.h"
#include "WebsiteDataType.h"
#include <WebCore/URL.h>

WKTypeID WKWebsiteDataStoreGetTypeID()
{
    return WebKit::toAPI(API::WebsiteDataStore::APIType);
}

WKWebsiteDataStoreRef WKWebsiteDataStoreGetDefaultDataStore()
{
    return WebKit::toAPI(API::WebsiteDataStore::defaultDataStore().ptr());
}

WKWebsiteDataStoreRef WKWebsiteDataStoreCreateNonPersistentDataStore()
{
    return WebKit::toAPI(&API::WebsiteDataStore::createNonPersistentDataStore().leakRef());
}

void WKWebsiteDataStoreSetResourceLoadStatisticsEnabled(WKWebsiteDataStoreRef dataStoreRef, bool enable)
{
    WebKit::toImpl(dataStoreRef)->setResourceLoadStatisticsEnabled(enable);
}

bool WKWebsiteDataStoreGetResourceLoadStatisticsEnabled(WKWebsiteDataStoreRef dataStoreRef)
{
    return WebKit::toImpl(dataStoreRef)->resourceLoadStatisticsEnabled();
}

void WKWebsiteDataStoreSetResourceLoadStatisticsDebugMode(WKWebsiteDataStoreRef dataStoreRef, bool enable)
{
    WebKit::toImpl(dataStoreRef)->setResourceLoadStatisticsDebugMode(enable);
}

void WKWebsiteDataStoreSetStatisticsLastSeen(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, double seconds)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setLastSeen(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), Seconds { seconds });
}

void WKWebsiteDataStoreSetStatisticsPrevalentResource(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    if (value)
        store->setPrevalentResource(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()));
    else
        store->clearPrevalentResource(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()));
}

void WKWebsiteDataStoreIsStatisticsPrevalentResource(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, void* context, WKWebsiteDataStoreIsStatisticsPrevalentResourceFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        callback(false, context);
        return;
    }

    store->isPrevalentResource(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), [context, callback](bool isPrevalentResource) {
        callback(isPrevalentResource, context);
    });
}

void WKWebsiteDataStoreIsStatisticsRegisteredAsSubFrameUnder(WKWebsiteDataStoreRef dataStoreRef, WKStringRef subFrameHost, WKStringRef topFrameHost, void* context, WKWebsiteDataStoreIsStatisticsRegisteredAsSubFrameUnderFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        callback(false, context);
        return;
    }

    store->isRegisteredAsSubFrameUnder(WebCore::URL(WebCore::URL(), WebKit::toImpl(subFrameHost)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(topFrameHost)->string()), [context, callback](bool isRegisteredAsSubFrameUnder) {
        callback(isRegisteredAsSubFrameUnder, context);
    });
}

void WKWebsiteDataStoreIsStatisticsRegisteredAsRedirectingTo(WKWebsiteDataStoreRef dataStoreRef, WKStringRef hostRedirectedFrom, WKStringRef hostRedirectedTo, void* context, WKWebsiteDataStoreIsStatisticsRegisteredAsRedirectingToFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        callback(false, context);
        return;
    }

    store->isRegisteredAsRedirectingTo(WebCore::URL(WebCore::URL(), WebKit::toImpl(hostRedirectedFrom)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(hostRedirectedTo)->string()), [context, callback](bool isRegisteredAsRedirectingTo) {
        callback(isRegisteredAsRedirectingTo, context);
    });
}

void WKWebsiteDataStoreSetStatisticsHasHadUserInteraction(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    if (value)
        store->logUserInteraction(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()));
    else
        store->clearUserInteraction(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()));
}

void WKWebsiteDataStoreSetStatisticsHasHadNonRecentUserInteraction(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;
    
    store->logNonRecentUserInteraction(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()));
}

void WKWebsiteDataStoreIsStatisticsHasHadUserInteraction(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, void* context, WKWebsiteDataStoreIsStatisticsHasHadUserInteractionFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        callback(false, context);
        return;
    }

    store->hasHadUserInteraction(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), [context, callback](bool hasHadUserInteraction) {
        callback(hasHadUserInteraction, context);
    });
}

void WKWebsiteDataStoreSetStatisticsGrandfathered(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setGrandfathered(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), value);
}

void WKWebsiteDataStoreIsStatisticsGrandfathered(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, void* context, WKWebsiteDataStoreIsStatisticsGrandfatheredFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store) {
        callback(false, context);
        return;
    }

    store->hasHadUserInteraction(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), [context, callback](bool isGrandfathered) {
        callback(isGrandfathered, context);
    });
}

void WKWebsiteDataStoreSetStatisticsSubframeUnderTopFrameOrigin(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef topFrameHost)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setSubframeUnderTopFrameOrigin(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(topFrameHost)->string()));
}

void WKWebsiteDataStoreSetStatisticsSubresourceUnderTopFrameOrigin(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef topFrameHost)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setSubresourceUnderTopFrameOrigin(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(topFrameHost)->string()));
}

void WKWebsiteDataStoreSetStatisticsSubresourceUniqueRedirectTo(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedTo)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setSubresourceUniqueRedirectTo(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(hostRedirectedTo)->string()));
}

void WKWebsiteDataStoreSetStatisticsSubresourceUniqueRedirectFrom(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedFrom)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;
    
    store->setSubresourceUniqueRedirectFrom(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(hostRedirectedFrom)->string()));
}

void WKWebsiteDataStoreSetStatisticsTopFrameUniqueRedirectTo(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedTo)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;
    
    store->setTopFrameUniqueRedirectTo(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(hostRedirectedTo)->string()));
}

void WKWebsiteDataStoreSetStatisticsTopFrameUniqueRedirectFrom(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedFrom)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;
    
    store->setTopFrameUniqueRedirectFrom(WebCore::URL(WebCore::URL(), WebKit::toImpl(host)->string()), WebCore::URL(WebCore::URL(), WebKit::toImpl(hostRedirectedFrom)->string()));
}

void WKWebsiteDataStoreSetStatisticsTimeToLiveUserInteraction(WKWebsiteDataStoreRef dataStoreRef, double seconds)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setTimeToLiveUserInteraction(Seconds { seconds });
}

void WKWebsiteDataStoreSetStatisticsTimeToLiveCookiePartitionFree(WKWebsiteDataStoreRef dataStoreRef, double seconds)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setTimeToLiveCookiePartitionFree(Seconds { seconds });
}

void WKWebsiteDataStoreStatisticsProcessStatisticsAndDataRecords(WKWebsiteDataStoreRef dataStoreRef)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleStatisticsAndDataRecordsProcessing();
}

void WKWebsiteDataStoreStatisticsUpdateCookiePartitioning(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreStatisticsUpdateCookiePartitioningFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleCookiePartitioningUpdate([context, callback]() {
        callback(context);
    });
}

void WKWebsiteDataStoreSetStatisticsShouldPartitionCookiesForHost(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value, void* context, WKWebsiteDataStoreSetStatisticsShouldPartitionCookiesForHostFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    if (value)
        store->scheduleCookiePartitioningUpdateForDomains({ WebKit::toImpl(host)->string() }, { }, { }, WebKit::ShouldClearFirst::No, [context, callback]() {
            callback(context);
        });
    else
        store->scheduleClearPartitioningStateForDomains({ WebKit::toImpl(host)->string() }, [context, callback]() {
            callback(context);
        });
}

void WKWebsiteDataStoreStatisticsSubmitTelemetry(WKWebsiteDataStoreRef dataStoreRef)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->submitTelemetry();
}

void WKWebsiteDataStoreSetStatisticsNotifyPagesWhenDataRecordsWereScanned(WKWebsiteDataStoreRef dataStoreRef, bool value)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setNotifyPagesWhenDataRecordsWereScanned(value);
}

void WKWebsiteDataStoreSetStatisticsShouldClassifyResourcesBeforeDataRecordsRemoval(WKWebsiteDataStoreRef dataStoreRef, bool value)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setShouldClassifyResourcesBeforeDataRecordsRemoval(value);
}

void WKWebsiteDataStoreSetStatisticsNotifyPagesWhenTelemetryWasCaptured(WKWebsiteDataStoreRef, bool value)
{
    WebKit::WebResourceLoadStatisticsTelemetry::setNotifyPagesWhenTelemetryWasCaptured(value);
}

void WKWebsiteDataStoreSetStatisticsMinimumTimeBetweenDataRecordsRemoval(WKWebsiteDataStoreRef dataStoreRef, double seconds)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setMinimumTimeBetweenDataRecordsRemoval(Seconds { seconds });
}

void WKWebsiteDataStoreSetStatisticsGrandfatheringTime(WKWebsiteDataStoreRef dataStoreRef, double seconds)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setGrandfatheringTime(Seconds {seconds });
}

void WKWebsiteDataStoreSetStatisticsMaxStatisticsEntries(WKWebsiteDataStoreRef dataStoreRef, unsigned entries)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setMaxStatisticsEntries(entries);
}

void WKWebsiteDataStoreSetStatisticsPruneEntriesDownTo(WKWebsiteDataStoreRef dataStoreRef, unsigned entries)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->setPruneEntriesDownTo(entries);
}

void WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStore(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleClearInMemoryAndPersistent(WebKit::WebResourceLoadStatisticsStore::ShouldGrandfather::Yes, [context, callback]() {
        callback(context);
    });
}

void WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreModifiedSinceHours(WKWebsiteDataStoreRef dataStoreRef, unsigned hours, void* context, WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreModifiedSinceHoursFunction callback)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->scheduleClearInMemoryAndPersistent(WallTime::now() - Seconds::fromHours(hours), WebKit::WebResourceLoadStatisticsStore::ShouldGrandfather::Yes, [context, callback]() {
        callback(context);
    });
}

void WKWebsiteDataStoreStatisticsClearThroughWebsiteDataRemoval(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreStatisticsClearThroughWebsiteDataRemovalFunction callback)
{
    OptionSet<WebKit::WebsiteDataType> dataTypes = WebKit::WebsiteDataType::ResourceLoadStatistics;
    WebKit::toImpl(dataStoreRef)->websiteDataStore().removeData(dataTypes, WallTime::fromRawSeconds(0), [context, callback] {
        callback(context);
    });
}

void WKWebsiteDataStoreStatisticsResetToConsistentState(WKWebsiteDataStoreRef dataStoreRef)
{
    auto* store = WebKit::toImpl(dataStoreRef)->websiteDataStore().resourceLoadStatistics();
    if (!store)
        return;

    store->resetParametersToDefaultValues();
    store->scheduleClearInMemory();
}

void WKWebsiteDataStoreRemoveAllFetchCaches(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreRemoveFetchCacheRemovalFunction callback)
{
    OptionSet<WebKit::WebsiteDataType> dataTypes = WebKit::WebsiteDataType::DOMCache;
    WebKit::toImpl(dataStoreRef)->websiteDataStore().removeData(dataTypes, -WallTime::infinity(), [context, callback] {
        callback(context);
    });
}

void WKWebsiteDataStoreRemoveFetchCacheForOrigin(WKWebsiteDataStoreRef dataStoreRef, WKSecurityOriginRef origin, void* context, WKWebsiteDataStoreRemoveFetchCacheRemovalFunction callback)
{
    WebKit::WebsiteDataRecord dataRecord;
    dataRecord.add(WebKit::WebsiteDataType::DOMCache, WebCore::SecurityOriginData::fromSecurityOrigin(WebKit::toImpl(origin)->securityOrigin()));
    Vector<WebKit::WebsiteDataRecord> dataRecords = { WTFMove(dataRecord) };

    OptionSet<WebKit::WebsiteDataType> dataTypes = WebKit::WebsiteDataType::DOMCache;
    WebKit::toImpl(dataStoreRef)->websiteDataStore().removeData(dataTypes, dataRecords, [context, callback] {
        callback(context);
    });
}

void WKWebsiteDataStoreRemoveAllIndexedDatabases(WKWebsiteDataStoreRef dataStoreRef)
{
    OptionSet<WebKit::WebsiteDataType> dataTypes = WebKit::WebsiteDataType::IndexedDBDatabases;
    WebKit::toImpl(dataStoreRef)->websiteDataStore().removeData(dataTypes, -WallTime::infinity(), [] { });
}

void WKWebsiteDataStoreRemoveAllServiceWorkerRegistrations(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreRemoveAllServiceWorkerRegistrationsCallback callback)
{
#if ENABLE(SERVICE_WORKER)
    OptionSet<WebKit::WebsiteDataType> dataTypes = WebKit::WebsiteDataType::ServiceWorkerRegistrations;
    WebKit::toImpl(dataStoreRef)->websiteDataStore().removeData(dataTypes, -WallTime::infinity(), [context, callback] {
        callback(context);
    });
#else
    UNUSED_PARAM(dataStoreRef);
#endif
}

void WKWebsiteDataStoreGetFetchCacheOrigins(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreGetFetchCacheOriginsFunction callback)
{
    WebKit::toImpl(dataStoreRef)->websiteDataStore().fetchData(WebKit::WebsiteDataType::DOMCache, { }, [context, callback] (auto dataRecords) {
        Vector<RefPtr<API::Object>> securityOrigins;
        for (const auto& dataRecord : dataRecords) {
            for (const auto& origin : dataRecord.origins)
                securityOrigins.append(API::SecurityOrigin::create(origin.securityOrigin()));
        }
        callback(WebKit::toAPI(API::Array::create(WTFMove(securityOrigins)).ptr()), context);
    });
}

void WKWebsiteDataStoreGetFetchCacheSizeForOrigin(WKWebsiteDataStoreRef dataStoreRef, WKStringRef origin, void* context, WKWebsiteDataStoreGetFetchCacheSizeForOriginFunction callback)
{
    OptionSet<WebKit::WebsiteDataFetchOption> fetchOptions = WebKit::WebsiteDataFetchOption::ComputeSizes;

    WebKit::toImpl(dataStoreRef)->websiteDataStore().fetchData(WebKit::WebsiteDataType::DOMCache, fetchOptions, [origin, context, callback] (auto dataRecords) {
        auto originData = WebCore::SecurityOriginData::fromSecurityOrigin(WebCore::SecurityOrigin::createFromString(WebKit::toImpl(origin)->string()));
        for (auto& dataRecord : dataRecords) {
            for (const auto& recordOrigin : dataRecord.origins) {
                if (originData == recordOrigin) {
                    callback(dataRecord.size ? dataRecord.size->totalSize : 0, context);
                    return;
                }

            }
        }
        callback(0, context);
    });
}
