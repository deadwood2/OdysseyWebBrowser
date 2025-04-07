/*
 * Copyright (C) 2012-2019 Apple Inc. All rights reserved.
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

#include "APIWebsiteDataStore.h"
#include "AuxiliaryProcessProxy.h"
#if ENABLE(LEGACY_CUSTOM_PROTOCOL_MANAGER)
#include "LegacyCustomProtocolManagerProxy.h"
#endif
#include "NetworkProcessProxyMessages.h"
#include "ProcessLauncher.h"
#include "ProcessThrottler.h"
#include "ProcessThrottlerClient.h"
#include "UserContentControllerIdentifier.h"
#include "WebProcessProxyMessages.h"
#include <WebCore/CrossSiteNavigationDataTransfer.h>
#include <WebCore/RegistrableDomain.h>
#include <memory>
#include <wtf/Deque.h>

namespace PAL {
class SessionID;
}

namespace WebCore {
class AuthenticationChallenge;
class ProtectionSpace;
class ResourceRequest;
enum class ShouldSample : bool;
enum class StorageAccessPromptWasShown : bool;
enum class StorageAccessWasGranted : bool;
class SecurityOrigin;
struct SecurityOriginData;
}

namespace WebKit {

class DownloadProxy;
class DownloadProxyMap;
class WebProcessPool;
enum class ShouldGrandfatherStatistics : bool;
enum class StorageAccessStatus : uint8_t;
enum class WebsiteDataFetchOption;
enum class WebsiteDataType;
struct NetworkProcessCreationParameters;
class WebUserContentControllerProxy;
struct WebsiteData;

class NetworkProcessProxy final : public AuxiliaryProcessProxy, private ProcessThrottlerClient, public CanMakeWeakPtr<NetworkProcessProxy> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    using RegistrableDomain = WebCore::RegistrableDomain;
    using TopFrameDomain = WebCore::RegistrableDomain;
    using SubFrameDomain = WebCore::RegistrableDomain;
    using SubResourceDomain = WebCore::RegistrableDomain;
    using RedirectDomain = WebCore::RegistrableDomain;
    using RedirectedFromDomain = WebCore::RegistrableDomain;
    using RedirectedToDomain = WebCore::RegistrableDomain;
    using NavigatedFromDomain = WebCore::RegistrableDomain;
    using NavigatedToDomain = WebCore::RegistrableDomain;
    using DomainInNeedOfStorageAccess = WebCore::RegistrableDomain;
    using OpenerDomain = WebCore::RegistrableDomain;

    explicit NetworkProcessProxy(WebProcessPool&);
    ~NetworkProcessProxy();

    void getNetworkProcessConnection(WebProcessProxy&, Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply&&);

    DownloadProxy& createDownloadProxy(const WebCore::ResourceRequest&);

    void fetchWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, OptionSet<WebsiteDataFetchOption>, CompletionHandler<void(WebsiteData)>&&);
    void deleteWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, WallTime modifiedSince, CompletionHandler<void()>&& completionHandler);
    void deleteWebsiteDataForOrigins(PAL::SessionID, OptionSet<WebKit::WebsiteDataType>, const Vector<WebCore::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, const Vector<String>& HSTSCacheHostNames, CompletionHandler<void()>&&);

    void getLocalStorageDetails(PAL::SessionID, CompletionHandler<void(Vector<LocalStorageDatabaseTracker::OriginDetails>&&)>&&);

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void clearPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void clearUserInteraction(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void dumpResourceLoadStatistics(PAL::SessionID, CompletionHandler<void(String)>&&);
    void updatePrevalentDomainsToBlockCookiesFor(PAL::SessionID, const Vector<RegistrableDomain>&, CompletionHandler<void()>&&);
    void hasHadUserInteraction(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void hasLocalStorage(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isGrandfathered(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isRegisteredAsRedirectingTo(PAL::SessionID, const RedirectedFromDomain&, const RedirectedToDomain&, CompletionHandler<void(bool)>&&);
    void isRegisteredAsSubFrameUnder(PAL::SessionID, const SubFrameDomain&, const TopFrameDomain&, CompletionHandler<void(bool)>&&);
    void isRegisteredAsSubresourceUnder(PAL::SessionID, const SubResourceDomain&, const TopFrameDomain&, CompletionHandler<void(bool)>&&);
    void isVeryPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void logUserInteraction(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void scheduleStatisticsAndDataRecordsProcessing(PAL::SessionID, CompletionHandler<void()>&&);
    void setLastSeen(PAL::SessionID, const RegistrableDomain&, Seconds, CompletionHandler<void()>&&);
    void setAgeCapForClientSideCookies(PAL::SessionID, Optional<Seconds>, CompletionHandler<void()>&&);
    void setCacheMaxAgeCap(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setGrandfathered(PAL::SessionID, const RegistrableDomain&, bool isGrandfathered, CompletionHandler<void()>&&);
    void setNotifyPagesWhenDataRecordsWereScanned(PAL::SessionID, bool, CompletionHandler<void()>&&);
    void setIsRunningResourceLoadStatisticsTest(PAL::SessionID, bool, CompletionHandler<void()>&&);
    void setNotifyPagesWhenTelemetryWasCaptured(PAL::SessionID, bool, CompletionHandler<void()>&&);
    void setSubframeUnderTopFrameDomain(PAL::SessionID, const SubFrameDomain&, const TopFrameDomain&, CompletionHandler<void()>&&);
    void setSubresourceUnderTopFrameDomain(PAL::SessionID, const SubResourceDomain&, const TopFrameDomain&, CompletionHandler<void()>&&);
    void setSubresourceUniqueRedirectTo(PAL::SessionID, const SubResourceDomain&, const RedirectedToDomain&, CompletionHandler<void()>&&);
    void setSubresourceUniqueRedirectFrom(PAL::SessionID, const SubResourceDomain&, const RedirectedFromDomain&, CompletionHandler<void()>&&);
    void setTimeToLiveUserInteraction(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setTopFrameUniqueRedirectTo(PAL::SessionID, const TopFrameDomain&, const RedirectedToDomain&, CompletionHandler<void()>&&);
    void setTopFrameUniqueRedirectFrom(PAL::SessionID, const TopFrameDomain&, const RedirectedFromDomain&, CompletionHandler<void()>&&);
    void setPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setPrevalentResourceForDebugMode(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setVeryPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void getAllStorageAccessEntries(PAL::SessionID, CompletionHandler<void(Vector<String> domains)>&&);
    void requestStorageAccessConfirm(WebCore::PageIdentifier, WebCore::FrameIdentifier, const SubFrameDomain&, const TopFrameDomain&, CompletionHandler<void(bool)>&&);
    void resetParametersToDefaultValues(PAL::SessionID, CompletionHandler<void()>&&);
    void scheduleClearInMemoryAndPersistent(PAL::SessionID, ShouldGrandfatherStatistics, CompletionHandler<void()>&&);
    void scheduleClearInMemoryAndPersistent(PAL::SessionID, Optional<WallTime> modifiedSince, ShouldGrandfatherStatistics, CompletionHandler<void()>&&);
    void scheduleCookieBlockingUpdate(PAL::SessionID, CompletionHandler<void()>&&);
    void submitTelemetry(PAL::SessionID, CompletionHandler<void()>&&);
    void setCacheMaxAgeCapForPrevalentResources(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setGrandfatheringTime(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setMaxStatisticsEntries(PAL::SessionID, size_t maximumEntryCount, CompletionHandler<void()>&&);
    void setMinimumTimeBetweenDataRecordsRemoval(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setPruneEntriesDownTo(PAL::SessionID, size_t pruneTargetCount, CompletionHandler<void()>&&);
    void setResourceLoadStatisticsDebugMode(PAL::SessionID, bool debugMode, CompletionHandler<void()>&&);
    void setShouldClassifyResourcesBeforeDataRecordsRemoval(PAL::SessionID, bool, CompletionHandler<void()>&&);
    void resetCacheMaxAgeCapForPrevalentResources(PAL::SessionID, CompletionHandler<void()>&&);
    void didCommitCrossSiteLoadWithDataTransfer(PAL::SessionID, const NavigatedFromDomain&, const NavigatedToDomain&, OptionSet<WebCore::CrossSiteNavigationDataTransfer::Flag>, WebCore::PageIdentifier);
    void didCommitCrossSiteLoadWithDataTransferFromPrevalentResource(WebCore::PageIdentifier);
    void setCrossSiteLoadWithLinkDecorationForTesting(PAL::SessionID, const NavigatedFromDomain&, const NavigatedToDomain&, CompletionHandler<void()>&&);
    void resetCrossSiteLoadsWithLinkDecorationForTesting(PAL::SessionID, CompletionHandler<void()>&&);
    void deleteCookiesForTesting(PAL::SessionID, const RegistrableDomain&, bool includeHttpOnlyCookies, CompletionHandler<void()>&&);
    void deleteWebsiteDataInUIProcessForRegistrableDomains(PAL::SessionID, OptionSet<WebsiteDataType>, OptionSet<WebsiteDataFetchOption>, Vector<RegistrableDomain>, CompletionHandler<void(HashSet<WebCore::RegistrableDomain>&&)>&&);
    void hasIsolatedSession(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
#endif

    void processReadyToSuspend();
    
    void sendProcessDidTransitionToForeground();
    void sendProcessDidTransitionToBackground();
    void synthesizeAppIsBackground(bool background);

    void setIsHoldingLockedFiles(bool);

    void syncAllCookies();
    void didSyncAllCookies();

    void testProcessIncomingSyncMessagesWhenWaitingForSyncReply(WebCore::PageIdentifier, Messages::NetworkProcessProxy::TestProcessIncomingSyncMessagesWhenWaitingForSyncReply::DelayedReply&&);

    ProcessThrottler& throttler() { return m_throttler; }
    WebProcessPool& processPool() { return m_processPool; }

#if ENABLE(CONTENT_EXTENSIONS)
    void didDestroyWebUserContentControllerProxy(WebUserContentControllerProxy&);
#endif

    void addSession(Ref<WebsiteDataStore>&&);
    void removeSession(PAL::SessionID);
    
    void takeUploadAssertion();
    void clearUploadAssertion();
    
#if ENABLE(INDEXED_DATABASE)
    void createSymLinkForFileUpgrade(const String& indexedDatabaseDirectory);
#endif

    // ProcessThrottlerClient
    void sendProcessWillSuspendImminently() final;
    void sendProcessDidResume() final;
    
    void sendProcessWillSuspendImminentlyForTesting();

private:
    // AuxiliaryProcessProxy
    void getLaunchOptions(ProcessLauncher::LaunchOptions&) override;
    void connectionWillOpen(IPC::Connection&) override;
    void processWillShutDown(IPC::Connection&) override;

    void networkProcessCrashed();
    void clearCallbackStates();

    // ProcessThrottlerClient
    void sendPrepareToSuspend() final;
    void sendCancelPrepareToSuspend() final;
    void didSetAssertionState(AssertionState) final;

    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;
    void didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference messageReceiverName, IPC::StringReference messageName) override;
    void didReceiveSyncNetworkProcessProxyMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);

    // Message handlers
    void didReceiveNetworkProcessProxyMessage(IPC::Connection&, IPC::Decoder&);
    void didCreateNetworkConnectionToWebProcess(const IPC::Attachment&);
    void didReceiveAuthenticationChallenge(PAL::SessionID, WebCore::PageIdentifier, WebCore::FrameIdentifier, WebCore::AuthenticationChallenge&&, uint64_t challengeID);
    void didFetchWebsiteData(uint64_t callbackID, const WebsiteData&);
    void didDeleteWebsiteData(uint64_t callbackID);
    void didDeleteWebsiteDataForOrigins(uint64_t callbackID);
    void logDiagnosticMessage(WebCore::PageIdentifier, const String& message, const String& description, WebCore::ShouldSample);
    void logDiagnosticMessageWithResult(WebCore::PageIdentifier, const String& message, const String& description, uint32_t result, WebCore::ShouldSample);
    void logDiagnosticMessageWithValue(WebCore::PageIdentifier, const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample);
    void logGlobalDiagnosticMessageWithValue(const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample);
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void logTestingEvent(PAL::SessionID, const String& event);
    void notifyResourceLoadStatisticsProcessed();
    void notifyWebsiteDataDeletionForRegistrableDomainsFinished();
    void notifyWebsiteDataScanForRegistrableDomainsFinished();
    void notifyResourceLoadStatisticsTelemetryFinished(unsigned totalPrevalentResources, unsigned totalPrevalentResourcesWithUserInteraction, unsigned top3SubframeUnderTopFrameOrigins);
#endif
    void retrieveCacheStorageParameters(PAL::SessionID);

#if ENABLE(CONTENT_EXTENSIONS)
    void contentExtensionRules(UserContentControllerIdentifier);
#endif

#if ENABLE(SANDBOX_EXTENSIONS)
    void getSandboxExtensionsForBlobFiles(const Vector<String>& paths, Messages::NetworkProcessProxy::GetSandboxExtensionsForBlobFiles::AsyncReply&&);
#endif

#if ENABLE(SERVICE_WORKER)
    void establishWorkerContextConnectionToNetworkProcess(WebCore::RegistrableDomain&&);
    void establishWorkerContextConnectionToNetworkProcessForExplicitSession(WebCore::RegistrableDomain&&, PAL::SessionID);
#endif

    void requestStorageSpace(PAL::SessionID, const WebCore::ClientOrigin&, uint64_t quota, uint64_t currentSize, uint64_t spaceRequired, CompletionHandler<void(Optional<uint64_t> quota)>&&);

    WebsiteDataStore* websiteDataStoreFromSessionID(PAL::SessionID);

    // ProcessLauncher::Client
    void didFinishLaunching(ProcessLauncher*, IPC::Connection::Identifier) override;

    WebProcessPool& m_processPool;
    
    unsigned m_numPendingConnectionRequests;
    Deque<std::pair<WeakPtr<WebProcessProxy>, Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>> m_pendingConnectionReplies;

    HashMap<uint64_t, CompletionHandler<void(WebsiteData)>> m_pendingFetchWebsiteDataCallbacks;
    HashMap<uint64_t, CompletionHandler<void()>> m_pendingDeleteWebsiteDataCallbacks;
    HashMap<uint64_t, CompletionHandler<void()>> m_pendingDeleteWebsiteDataForOriginsCallbacks;

    std::unique_ptr<DownloadProxyMap> m_downloadProxyMap;
#if ENABLE(LEGACY_CUSTOM_PROTOCOL_MANAGER)
    LegacyCustomProtocolManagerProxy m_customProtocolManagerProxy;
#endif
    ProcessThrottler m_throttler;
    ProcessThrottler::BackgroundActivityToken m_tokenForHoldingLockedFiles;
    ProcessThrottler::BackgroundActivityToken m_syncAllCookiesToken;
    
    unsigned m_syncAllCookiesCounter { 0 };

#if ENABLE(CONTENT_EXTENSIONS)
    HashSet<WebUserContentControllerProxy*> m_webUserContentControllerProxies;
#endif

    HashMap<PAL::SessionID, RefPtr<WebsiteDataStore>> m_websiteDataStores;
    
    std::unique_ptr<ProcessAssertion> m_uploadAssertion;
};

} // namespace WebKit
