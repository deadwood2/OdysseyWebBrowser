/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#pragma once

#include "APIObject.h"
#include "CacheModel.h"
#include "WebsiteDataStore.h"
#include <wtf/ProcessID.h>
#include <wtf/Ref.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace API {

#if PLATFORM(COCOA) && !PLATFORM(IOS_FAMILY_SIMULATOR)
#define DEFAULT_CAPTURE_DISPLAY_IN_UI_PROCESS true
#else
#define DEFAULT_CAPTURE_DISPLAY_IN_UI_PROCESS false
#endif

class ProcessPoolConfiguration final : public ObjectImpl<Object::Type::ProcessPoolConfiguration> {
public:
    static Ref<ProcessPoolConfiguration> create();
    static Ref<ProcessPoolConfiguration> createWithLegacyOptions();
    static Ref<ProcessPoolConfiguration> createWithWebsiteDataStoreConfiguration(const WebKit::WebsiteDataStoreConfiguration&);

    explicit ProcessPoolConfiguration();
    virtual ~ProcessPoolConfiguration();
    
    Ref<ProcessPoolConfiguration> copy();

    bool shouldHaveLegacyDataStore() const { return m_shouldHaveLegacyDataStore; }
    void setShouldHaveLegacyDataStore(bool shouldHaveLegacyDataStore) { m_shouldHaveLegacyDataStore = shouldHaveLegacyDataStore; }

    bool usesSingleWebProcess() const { return m_usesSingleWebProcess; }
    void setUsesSingleWebProcess(bool enabled) { m_usesSingleWebProcess = enabled; }

    uint32_t downloadMonitorSpeedMultiplier() const { return m_downloadMonitorSpeedMultiplier; }
    void setDownloadMonitorSpeedMultiplier(uint32_t multiplier) { m_downloadMonitorSpeedMultiplier = multiplier; }
    
    bool isAutomaticProcessWarmingEnabled() const
    {
        return m_isAutomaticProcessWarmingEnabledByClient.valueOr(m_clientWouldBenefitFromAutomaticProcessPrewarming);
    }

    bool wasAutomaticProcessWarmingSetByClient() const { return !!m_isAutomaticProcessWarmingEnabledByClient; }
    void setIsAutomaticProcessWarmingEnabled(bool value) { m_isAutomaticProcessWarmingEnabledByClient = value; }

    void setUsesWebProcessCache(bool value) { m_usesWebProcessCache = value; }
    bool usesWebProcessCache() const { return m_usesWebProcessCache; }

    bool clientWouldBenefitFromAutomaticProcessPrewarming() const { return m_clientWouldBenefitFromAutomaticProcessPrewarming; }
    void setClientWouldBenefitFromAutomaticProcessPrewarming(bool value) { m_clientWouldBenefitFromAutomaticProcessPrewarming = value; }

    bool diskCacheSpeculativeValidationEnabled() const { return m_diskCacheSpeculativeValidationEnabled; }
    void setDiskCacheSpeculativeValidationEnabled(bool enabled) { m_diskCacheSpeculativeValidationEnabled = enabled; }

    WebKit::CacheModel cacheModel() const { return m_cacheModel; }
    void setCacheModel(WebKit::CacheModel cacheModel) { m_cacheModel = cacheModel; }

    const WTF::String& applicationCacheDirectory() const { return m_applicationCacheDirectory; }
    void setApplicationCacheDirectory(const WTF::String& applicationCacheDirectory) { m_applicationCacheDirectory = applicationCacheDirectory; }

    const WTF::String& applicationCacheFlatFileSubdirectoryName() const { return m_applicationCacheFlatFileSubdirectoryName; }

    const WTF::String& diskCacheDirectory() const { return m_diskCacheDirectory; }
    void setDiskCacheDirectory(const WTF::String& diskCacheDirectory) { m_diskCacheDirectory = diskCacheDirectory; }

    const WTF::String& mediaCacheDirectory() const { return m_mediaCacheDirectory; }
    void setMediaCacheDirectory(const WTF::String& mediaCacheDirectory) { m_mediaCacheDirectory = mediaCacheDirectory; }
    
    const WTF::String& indexedDBDatabaseDirectory() const { return m_indexedDBDatabaseDirectory; }
    void setIndexedDBDatabaseDirectory(const WTF::String& indexedDBDatabaseDirectory) { m_indexedDBDatabaseDirectory = indexedDBDatabaseDirectory; }

    const WTF::String& injectedBundlePath() const { return m_injectedBundlePath; }
    void setInjectedBundlePath(const WTF::String& injectedBundlePath) { m_injectedBundlePath = injectedBundlePath; }

    const WTF::String& localStorageDirectory() const { return m_localStorageDirectory; }
    void setLocalStorageDirectory(const WTF::String& localStorageDirectory) { m_localStorageDirectory = localStorageDirectory; }

    const WTF::String& deviceIdHashSaltsStorageDirectory() const { return m_deviceIdHashSaltsStorageDirectory; }
    void setDeviceIdHashSaltsStorageDirectory(const WTF::String& directory) { m_deviceIdHashSaltsStorageDirectory = directory; }

    const WTF::String& webSQLDatabaseDirectory() const { return m_webSQLDatabaseDirectory; }
    void setWebSQLDatabaseDirectory(const WTF::String& webSQLDatabaseDirectory) { m_webSQLDatabaseDirectory = webSQLDatabaseDirectory; }

    const WTF::String& mediaKeysStorageDirectory() const { return m_mediaKeysStorageDirectory; }
    void setMediaKeysStorageDirectory(const WTF::String& mediaKeysStorageDirectory) { m_mediaKeysStorageDirectory = mediaKeysStorageDirectory; }

    const WTF::String& resourceLoadStatisticsDirectory() const { return m_resourceLoadStatisticsDirectory; }
    void setResourceLoadStatisticsDirectory(const WTF::String& resourceLoadStatisticsDirectory) { m_resourceLoadStatisticsDirectory = resourceLoadStatisticsDirectory; }

    const Vector<WTF::String>& cachePartitionedURLSchemes() { return m_cachePartitionedURLSchemes; }
    void setCachePartitionedURLSchemes(Vector<WTF::String>&& cachePartitionedURLSchemes) { m_cachePartitionedURLSchemes = WTFMove(cachePartitionedURLSchemes); }

    const Vector<WTF::String>& alwaysRevalidatedURLSchemes() { return m_alwaysRevalidatedURLSchemes; }
    void setAlwaysRevalidatedURLSchemes(Vector<WTF::String>&& alwaysRevalidatedURLSchemes) { m_alwaysRevalidatedURLSchemes = WTFMove(alwaysRevalidatedURLSchemes); }

    const Vector<WTF::CString>& additionalReadAccessAllowedPaths() { return m_additionalReadAccessAllowedPaths; }
    void setAdditionalReadAccessAllowedPaths(Vector<WTF::CString>&& additionalReadAccessAllowedPaths) { m_additionalReadAccessAllowedPaths = additionalReadAccessAllowedPaths; }

    bool fullySynchronousModeIsAllowedForTesting() const { return m_fullySynchronousModeIsAllowedForTesting; }
    void setFullySynchronousModeIsAllowedForTesting(bool allowed) { m_fullySynchronousModeIsAllowedForTesting = allowed; }

    bool ignoreSynchronousMessagingTimeoutsForTesting() const { return m_ignoreSynchronousMessagingTimeoutsForTesting; }
    void setIgnoreSynchronousMessagingTimeoutsForTesting(bool allowed) { m_ignoreSynchronousMessagingTimeoutsForTesting = allowed; }

    bool attrStyleEnabled() const { return m_attrStyleEnabled; }
    void setAttrStyleEnabled(bool enabled) { m_attrStyleEnabled = enabled; }

    const Vector<WTF::String>& overrideLanguages() const { return m_overrideLanguages; }
    void setOverrideLanguages(Vector<WTF::String>&& languages) { m_overrideLanguages = WTFMove(languages); }
    
    bool alwaysRunsAtBackgroundPriority() const { return m_alwaysRunsAtBackgroundPriority; }
    void setAlwaysRunsAtBackgroundPriority(bool alwaysRunsAtBackgroundPriority) { m_alwaysRunsAtBackgroundPriority = alwaysRunsAtBackgroundPriority; }

    bool shouldTakeUIBackgroundAssertion() const { return m_shouldTakeUIBackgroundAssertion; }
    void setShouldTakeUIBackgroundAssertion(bool shouldTakeUIBackgroundAssertion) { m_shouldTakeUIBackgroundAssertion = shouldTakeUIBackgroundAssertion; }

    bool shouldCaptureAudioInUIProcess() const { return m_shouldCaptureAudioInUIProcess; }
    void setShouldCaptureAudioInUIProcess(bool shouldCaptureAudioInUIProcess) { m_shouldCaptureAudioInUIProcess = shouldCaptureAudioInUIProcess; }

    bool shouldCaptureVideoInUIProcess() const { return m_shouldCaptureVideoInUIProcess; }
    void setShouldCaptureVideoInUIProcess(bool shouldCaptureVideoInUIProcess) { m_shouldCaptureVideoInUIProcess = shouldCaptureVideoInUIProcess; }

    bool shouldCaptureDisplayInUIProcess() const { return m_shouldCaptureDisplayInUIProcess; }
    void setShouldCaptureDisplayInUIProcess(bool shouldCaptureDisplayInUIProcess) { m_shouldCaptureDisplayInUIProcess = shouldCaptureDisplayInUIProcess; }

    bool isJITEnabled() const { return m_isJITEnabled; }
    void setJITEnabled(bool enabled) { m_isJITEnabled = enabled; }
    
#if PLATFORM(IOS_FAMILY)
    const WTF::String& ctDataConnectionServiceType() const { return m_ctDataConnectionServiceType; }
    void setCTDataConnectionServiceType(const WTF::String& ctDataConnectionServiceType) { m_ctDataConnectionServiceType = ctDataConnectionServiceType; }
#endif

    ProcessID presentingApplicationPID() const { return m_presentingApplicationPID; }
    void setPresentingApplicationPID(ProcessID pid) { m_presentingApplicationPID = pid; }

    bool processSwapsOnNavigation() const
    {
        return m_processSwapsOnNavigationFromClient.valueOr(m_processSwapsOnNavigationFromExperimentalFeatures);
    }
    void setProcessSwapsOnNavigation(bool swaps) { m_processSwapsOnNavigationFromClient = swaps; }
    void setProcessSwapsOnNavigationFromExperimentalFeatures(bool swaps) { m_processSwapsOnNavigationFromExperimentalFeatures = swaps; }

    bool alwaysKeepAndReuseSwappedProcesses() const { return m_alwaysKeepAndReuseSwappedProcesses; }
    void setAlwaysKeepAndReuseSwappedProcesses(bool keepAndReuse) { m_alwaysKeepAndReuseSwappedProcesses = keepAndReuse; }

    bool processSwapsOnWindowOpenWithOpener() const { return m_processSwapsOnWindowOpenWithOpener; }
    void setProcessSwapsOnWindowOpenWithOpener(bool swaps) { m_processSwapsOnWindowOpenWithOpener = swaps; }

    const WTF::String& customWebContentServiceBundleIdentifier() const { return m_customWebContentServiceBundleIdentifier; }
    void setCustomWebContentServiceBundleIdentifier(const WTF::String& customWebContentServiceBundleIdentifier) { m_customWebContentServiceBundleIdentifier = customWebContentServiceBundleIdentifier; }

    const WTF::String& hstsStorageDirectory() const { return m_hstsStorageDirectory; }
    void setHSTSStorageDirectory(WTF::String&& directory) { m_hstsStorageDirectory = WTFMove(directory); }
    
#if PLATFORM(COCOA)
    bool suppressesConnectionTerminationOnSystemChange() const { return m_suppressesConnectionTerminationOnSystemChange; }
    void setSuppressesConnectionTerminationOnSystemChange(bool suppressesConnectionTerminationOnSystemChange) { m_suppressesConnectionTerminationOnSystemChange = suppressesConnectionTerminationOnSystemChange; }
#endif

private:
    bool m_shouldHaveLegacyDataStore { false };

    bool m_diskCacheSpeculativeValidationEnabled { false };
    WebKit::CacheModel m_cacheModel { WebKit::CacheModel::PrimaryWebBrowser };

    WTF::String m_applicationCacheDirectory;
    WTF::String m_applicationCacheFlatFileSubdirectoryName;
    WTF::String m_diskCacheDirectory;
    WTF::String m_mediaCacheDirectory;
    WTF::String m_indexedDBDatabaseDirectory;
    WTF::String m_injectedBundlePath;
    WTF::String m_localStorageDirectory;
    WTF::String m_deviceIdHashSaltsStorageDirectory;
    WTF::String m_webSQLDatabaseDirectory;
    WTF::String m_mediaKeysStorageDirectory;
    WTF::String m_resourceLoadStatisticsDirectory;
    Vector<WTF::String> m_cachePartitionedURLSchemes;
    Vector<WTF::String> m_alwaysRevalidatedURLSchemes;
    Vector<WTF::CString> m_additionalReadAccessAllowedPaths;
    bool m_fullySynchronousModeIsAllowedForTesting { false };
    bool m_ignoreSynchronousMessagingTimeoutsForTesting { false };
    bool m_attrStyleEnabled { false };
    Vector<WTF::String> m_overrideLanguages;
    bool m_alwaysRunsAtBackgroundPriority { false };
    bool m_shouldTakeUIBackgroundAssertion { true };
    bool m_shouldCaptureAudioInUIProcess { false };
    bool m_shouldCaptureVideoInUIProcess { false };
    bool m_shouldCaptureDisplayInUIProcess { DEFAULT_CAPTURE_DISPLAY_IN_UI_PROCESS };
    ProcessID m_presentingApplicationPID { getCurrentProcessID() };
    Optional<bool> m_processSwapsOnNavigationFromClient;
    bool m_processSwapsOnNavigationFromExperimentalFeatures { false };
    bool m_alwaysKeepAndReuseSwappedProcesses { false };
    bool m_processSwapsOnWindowOpenWithOpener { false };
    Optional<bool> m_isAutomaticProcessWarmingEnabledByClient;
    bool m_usesWebProcessCache { false };
    bool m_clientWouldBenefitFromAutomaticProcessPrewarming { false };
    WTF::String m_customWebContentServiceBundleIdentifier;
    bool m_isJITEnabled { true };
    bool m_usesSingleWebProcess { false };
    uint32_t m_downloadMonitorSpeedMultiplier { 1 };
    WTF::String m_hstsStorageDirectory;

#if PLATFORM(IOS_FAMILY)
    WTF::String m_ctDataConnectionServiceType;
#endif

#if PLATFORM(COCOA)
    bool m_suppressesConnectionTerminationOnSystemChange { false };
#endif
};

} // namespace API
