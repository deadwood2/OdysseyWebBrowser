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

#ifndef WKWebsiteDataStoreRef_h
#define WKWebsiteDataStoreRef_h

#include <WebKit/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKWebsiteDataStoreGetTypeID();

WK_EXPORT WKWebsiteDataStoreRef WKWebsiteDataStoreGetDefaultDataStore();
WK_EXPORT WKWebsiteDataStoreRef WKWebsiteDataStoreCreateNonPersistentDataStore();

WK_EXPORT bool WKWebsiteDataStoreGetResourceLoadStatisticsEnabled(WKWebsiteDataStoreRef dataStoreRef);
WK_EXPORT void WKWebsiteDataStoreSetResourceLoadStatisticsEnabled(WKWebsiteDataStoreRef dataStoreRef, bool enable);
WK_EXPORT void WKWebsiteDataStoreSetResourceLoadStatisticsDebugMode(WKWebsiteDataStoreRef dataStoreRef, bool enable);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsLastSeen(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, double seconds);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsPrevalentResource(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value);
typedef void (*WKWebsiteDataStoreIsStatisticsPrevalentResourceFunction)(bool isPrevalentResource, void* functionContext);
WK_EXPORT void WKWebsiteDataStoreIsStatisticsPrevalentResource(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, void* context, WKWebsiteDataStoreIsStatisticsPrevalentResourceFunction callback);
typedef void (*WKWebsiteDataStoreIsStatisticsRegisteredAsSubFrameUnderFunction)(bool isRegisteredAsSubFrameUnder, void* functionContext);
WK_EXPORT void WKWebsiteDataStoreIsStatisticsRegisteredAsSubFrameUnder(WKWebsiteDataStoreRef dataStoreRef, WKStringRef subFrameHost, WKStringRef topFrameHost, void* context, WKWebsiteDataStoreIsStatisticsRegisteredAsSubFrameUnderFunction callback);
typedef void (*WKWebsiteDataStoreIsStatisticsRegisteredAsRedirectingToFunction)(bool isRegisteredAsRedirectingTo, void* functionContext);
WK_EXPORT void WKWebsiteDataStoreIsStatisticsRegisteredAsRedirectingTo(WKWebsiteDataStoreRef dataStoreRef, WKStringRef hostRedirectedFrom, WKStringRef hostRedirectedTo, void* context, WKWebsiteDataStoreIsStatisticsRegisteredAsRedirectingToFunction callback);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsHasHadUserInteraction(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsHasHadNonRecentUserInteraction(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host);
typedef void (*WKWebsiteDataStoreIsStatisticsHasHadUserInteractionFunction)(bool hasHadUserInteraction, void* functionContext);
WK_EXPORT void WKWebsiteDataStoreIsStatisticsHasHadUserInteraction(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, void* context, WKWebsiteDataStoreIsStatisticsHasHadUserInteractionFunction callback);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsGrandfathered(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value);
typedef void (*WKWebsiteDataStoreIsStatisticsGrandfatheredFunction)(bool isGrandfathered, void* functionContext);
WK_EXPORT void WKWebsiteDataStoreIsStatisticsGrandfathered(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, void* context, WKWebsiteDataStoreIsStatisticsGrandfatheredFunction callback);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsSubframeUnderTopFrameOrigin(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef topFrameHost);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsSubresourceUnderTopFrameOrigin(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef topFrameHost);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsSubresourceUniqueRedirectTo(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedTo);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsSubresourceUniqueRedirectFrom(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedFrom);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsTopFrameUniqueRedirectTo(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedTo);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsTopFrameUniqueRedirectFrom(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, WKStringRef hostRedirectedFrom);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsTimeToLiveUserInteraction(WKWebsiteDataStoreRef dataStoreRef, double seconds);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsTimeToLiveCookiePartitionFree(WKWebsiteDataStoreRef dataStoreRef, double seconds);
WK_EXPORT void WKWebsiteDataStoreStatisticsProcessStatisticsAndDataRecords(WKWebsiteDataStoreRef dataStoreRef);
typedef void (*WKWebsiteDataStoreStatisticsUpdateCookiePartitioningFunction)(void* functionContext);
WK_EXPORT void WKWebsiteDataStoreStatisticsUpdateCookiePartitioning(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreStatisticsUpdateCookiePartitioningFunction callback);
typedef void (*WKWebsiteDataStoreSetStatisticsShouldPartitionCookiesForHostFunction)(void* functionContext);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsShouldPartitionCookiesForHost(WKWebsiteDataStoreRef dataStoreRef, WKStringRef host, bool value, void* context, WKWebsiteDataStoreSetStatisticsShouldPartitionCookiesForHostFunction callback);
WK_EXPORT void WKWebsiteDataStoreStatisticsSubmitTelemetry(WKWebsiteDataStoreRef dataStoreRef);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsNotifyPagesWhenDataRecordsWereScanned(WKWebsiteDataStoreRef dataStoreRef, bool value);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsShouldClassifyResourcesBeforeDataRecordsRemoval(WKWebsiteDataStoreRef dataStoreRef, bool value);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsNotifyPagesWhenTelemetryWasCaptured(WKWebsiteDataStoreRef dataStoreRef, bool value);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsMinimumTimeBetweenDataRecordsRemoval(WKWebsiteDataStoreRef dataStoreRef, double seconds);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsGrandfatheringTime(WKWebsiteDataStoreRef dataStoreRef, double seconds);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsMaxStatisticsEntries(WKWebsiteDataStoreRef dataStoreRef, unsigned entries);
WK_EXPORT void WKWebsiteDataStoreSetStatisticsPruneEntriesDownTo(WKWebsiteDataStoreRef dataStoreRef, unsigned entries);
typedef void (*WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreFunction)(void* functionContext);
WK_EXPORT void WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStore(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreFunction callback);
typedef void (*WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreModifiedSinceHoursFunction)(void* functionContext);
WK_EXPORT void WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreModifiedSinceHours(WKWebsiteDataStoreRef dataStoreRef, unsigned hours, void* context, WKWebsiteDataStoreStatisticsClearInMemoryAndPersistentStoreModifiedSinceHoursFunction callback);
typedef void (*WKWebsiteDataStoreStatisticsClearThroughWebsiteDataRemovalFunction)(void* functionContext);
WK_EXPORT void WKWebsiteDataStoreStatisticsClearThroughWebsiteDataRemoval(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreStatisticsClearThroughWebsiteDataRemovalFunction callback);
WK_EXPORT void WKWebsiteDataStoreStatisticsResetToConsistentState(WKWebsiteDataStoreRef dataStoreRef);

typedef void (*WKWebsiteDataStoreRemoveFetchCacheRemovalFunction)(void* functionContext);
WK_EXPORT void WKWebsiteDataStoreRemoveFetchCacheForOrigin(WKWebsiteDataStoreRef dataStoreRef, WKSecurityOriginRef origin, void* context, WKWebsiteDataStoreRemoveFetchCacheRemovalFunction callback);
WK_EXPORT void WKWebsiteDataStoreRemoveAllFetchCaches(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreRemoveFetchCacheRemovalFunction callback);

typedef void (*WKWebsiteDataStoreRemoveAllServiceWorkerRegistrationsCallback)(void* functionContext);
WK_EXPORT void WKWebsiteDataStoreRemoveAllServiceWorkerRegistrations(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreRemoveAllServiceWorkerRegistrationsCallback callback);

WK_EXPORT void WKWebsiteDataStoreRemoveAllIndexedDatabases(WKWebsiteDataStoreRef dataStoreRef);

typedef void (*WKWebsiteDataStoreGetFetchCacheOriginsFunction)(WKArrayRef, void*);
WK_EXPORT void WKWebsiteDataStoreGetFetchCacheOrigins(WKWebsiteDataStoreRef dataStoreRef, void* context, WKWebsiteDataStoreGetFetchCacheOriginsFunction function);

typedef void (*WKWebsiteDataStoreGetFetchCacheSizeForOriginFunction)(uint64_t, void*);
WK_EXPORT void WKWebsiteDataStoreGetFetchCacheSizeForOrigin(WKWebsiteDataStoreRef dataStoreRef, WKStringRef origin, void* context, WKWebsiteDataStoreGetFetchCacheSizeForOriginFunction function);

#ifdef __cplusplus
}
#endif

#endif /* WKWebsiteDataStoreRef_h */
