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

#include "AuxiliaryProcess.h"
#include "CacheModel.h"
#include "DownloadManager.h"
#include "LocalStorageDatabaseTracker.h"
#include "NetworkContentRuleListManager.h"
#include "NetworkHTTPSUpgradeChecker.h"
#include "SandboxExtension.h"
#include "WebResourceLoadStatisticsStore.h"
#include "WebsiteData.h"
#include <WebCore/AdClickAttribution.h>
#include <WebCore/ClientOrigin.h>
#include <WebCore/CrossSiteNavigationDataTransfer.h>
#include <WebCore/DiagnosticLoggingClient.h>
#include <WebCore/FetchIdentifier.h>
#include <WebCore/IDBKeyData.h>
#include <WebCore/IDBServer.h>
#include <WebCore/PageIdentifier.h>
#include <WebCore/RegistrableDomain.h>
#include <WebCore/ServiceWorkerIdentifier.h>
#include <WebCore/ServiceWorkerTypes.h>
#include <memory>
#include <wtf/CrossThreadTask.h>
#include <wtf/Function.h>
#include <wtf/HashSet.h>
#include <wtf/MemoryPressureHandler.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RetainPtr.h>
#include <wtf/WeakPtr.h>

#if PLATFORM(IOS_FAMILY)
#include "WebSQLiteDatabaseTracker.h"
#endif

#if PLATFORM(COCOA)
typedef struct OpaqueCFHTTPCookieStorage*  CFHTTPCookieStorageRef;
#endif

namespace IPC {
class FormDataReference;
}

namespace PAL {
class SessionID;
}

namespace WebCore {
class CertificateInfo;
class CurlProxySettings;
class DownloadID;
class ProtectionSpace;
class StorageQuotaManager;
class NetworkStorageSession;
class ResourceError;
class SWServer;
enum class IncludeHttpOnlyCookies : bool;
enum class StoredCredentialsPolicy : uint8_t;
enum class StorageAccessPromptWasShown : bool;
enum class StorageAccessWasGranted : bool;
struct ClientOrigin;
struct MessageWithMessagePorts;
struct SecurityOriginData;
struct SoupNetworkProxySettings;
struct ServiceWorkerClientIdentifier;
}

namespace WebKit {

class AuthenticationManager;
class NetworkConnectionToWebProcess;
class NetworkProcessSupplement;
class NetworkProximityManager;
class NetworkResourceLoader;
class StorageManagerSet;
class WebSWServerConnection;
class WebSWServerToContextConnection;
enum class ShouldGrandfatherStatistics : bool;
enum class StorageAccessStatus : uint8_t;
enum class WebsiteDataFetchOption;
enum class WebsiteDataType;
struct NetworkProcessCreationParameters;
struct WebsiteDataStoreParameters;

#if ENABLE(SERVICE_WORKER)
class WebSWOriginStore;
#endif

namespace NetworkCache {
enum class CacheOption : uint8_t;
}

namespace CacheStorage {
class Engine;
}

class NetworkProcess : public AuxiliaryProcess, private DownloadManager::Client, public ThreadSafeRefCounted<NetworkProcess>
#if ENABLE(INDEXED_DATABASE)
    , public WebCore::IDBServer::IDBBackingStoreTemporaryFileHandler
#endif
    , public CanMakeWeakPtr<NetworkProcess>
{
    WTF_MAKE_NONCOPYABLE(NetworkProcess);
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

    NetworkProcess(AuxiliaryProcessInitializationParameters&&);
    ~NetworkProcess();
    static constexpr ProcessType processType = ProcessType::Network;

    template <typename T>
    T* supplement()
    {
        return static_cast<T*>(m_supplements.get(T::supplementName()));
    }

    template <typename T>
    void addSupplement()
    {
        m_supplements.add(T::supplementName(), makeUnique<T>(*this));
    }

    void removeNetworkConnectionToWebProcess(NetworkConnectionToWebProcess&);

    AuthenticationManager& authenticationManager();
    DownloadManager& downloadManager();

    void setSession(PAL::SessionID, std::unique_ptr<NetworkSession>&&);
    NetworkSession* networkSession(PAL::SessionID) const final;
    NetworkSession* networkSessionByConnection(IPC::Connection&) const;
    void destroySession(PAL::SessionID);

    void forEachNetworkSession(const Function<void(NetworkSession&)>&);

    void forEachNetworkStorageSession(const Function<void(WebCore::NetworkStorageSession&)>&);
    WebCore::NetworkStorageSession* storageSession(const PAL::SessionID&) const;
    WebCore::NetworkStorageSession& defaultStorageSession() const;
    void switchToNewTestingSession();
#if PLATFORM(COCOA)
    void ensureSession(const PAL::SessionID&, const String& identifier, RetainPtr<CFHTTPCookieStorageRef>&&);
#else
    void ensureSession(const PAL::SessionID&, const String& identifier);
#endif

    bool canHandleHTTPSServerTrustEvaluation() const { return m_canHandleHTTPSServerTrustEvaluation; }

    void processWillSuspendImminently();
    void processWillSuspendImminentlyForTestingSync(CompletionHandler<void()>&&);
    void prepareToSuspend();
    void cancelPrepareToSuspend();
    void processDidResume();
    void resume();

    // Diagnostic messages logging.
    void logDiagnosticMessage(WebCore::PageIdentifier, const String& message, const String& description, WebCore::ShouldSample);
    void logDiagnosticMessageWithResult(WebCore::PageIdentifier, const String& message, const String& description, WebCore::DiagnosticLoggingResultType, WebCore::ShouldSample);
    void logDiagnosticMessageWithValue(WebCore::PageIdentifier, const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample);

#if PLATFORM(COCOA)
    RetainPtr<CFDataRef> sourceApplicationAuditData() const;
    bool suppressesConnectionTerminationOnSystemChange() const { return m_suppressesConnectionTerminationOnSystemChange; }
#endif
#if PLATFORM(COCOA) || USE(SOUP)
    void getHostNamesWithHSTSCache(WebCore::NetworkStorageSession&, HashSet<String>&);
    void deleteHSTSCacheForHostNames(WebCore::NetworkStorageSession&, const Vector<String>&);
    void clearHSTSCache(WebCore::NetworkStorageSession&, WallTime modifiedSince);
#endif

    void findPendingDownloadLocation(NetworkDataTask&, ResponseCompletionHandler&&, const WebCore::ResourceResponse&);

    void prefetchDNS(const String&);

    void addWebsiteDataStore(WebsiteDataStoreParameters&&);

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void clearPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void clearUserInteraction(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void deleteWebsiteDataForRegistrableDomains(PAL::SessionID, OptionSet<WebsiteDataType>, Vector<std::pair<RegistrableDomain, WebsiteDataToRemove>>&&, bool shouldNotifyPage, CompletionHandler<void(const HashSet<RegistrableDomain>&)>&&);
    void deleteCookiesForTesting(PAL::SessionID, RegistrableDomain, bool includeHttpOnlyCookies, CompletionHandler<void()>&&);
    void dumpResourceLoadStatistics(PAL::SessionID, CompletionHandler<void(String)>&&);
    void updatePrevalentDomainsToBlockCookiesFor(PAL::SessionID, const Vector<RegistrableDomain>& domainsToBlock, CompletionHandler<void()>&&);
    void isGrandfathered(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isVeryPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void setAgeCapForClientSideCookies(PAL::SessionID, Optional<Seconds>, CompletionHandler<void()>&&);
    void isRegisteredAsRedirectingTo(PAL::SessionID, const RedirectedFromDomain&, const RedirectedToDomain&, CompletionHandler<void(bool)>&&);
    void isRegisteredAsSubFrameUnder(PAL::SessionID, const SubFrameDomain&, const TopFrameDomain&, CompletionHandler<void(bool)>&&);
    void isRegisteredAsSubresourceUnder(PAL::SessionID, const SubResourceDomain&, const TopFrameDomain&, CompletionHandler<void(bool)>&&);
    void setGrandfathered(PAL::SessionID, const RegistrableDomain&, bool isGrandfathered, CompletionHandler<void()>&&);
    void setMaxStatisticsEntries(PAL::SessionID, uint64_t maximumEntryCount, CompletionHandler<void()>&&);
    void setPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setPrevalentResourceForDebugMode(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setVeryPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setPruneEntriesDownTo(PAL::SessionID, uint64_t pruneTargetCount, CompletionHandler<void()>&&);
    void hadUserInteraction(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void hasLocalStorage(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void getAllStorageAccessEntries(PAL::SessionID, CompletionHandler<void(Vector<String> domains)>&&);
    void logFrameNavigation(PAL::SessionID, const NavigatedToDomain&, const TopFrameDomain&, const NavigatedFromDomain&, bool isRedirect, bool isMainFrame);
    void logUserInteraction(PAL::SessionID, const TopFrameDomain&, CompletionHandler<void()>&&);
    void removePrevalentDomains(PAL::SessionID, const Vector<RegistrableDomain>&);
    void resetCacheMaxAgeCapForPrevalentResources(PAL::SessionID, CompletionHandler<void()>&&);
    void resetParametersToDefaultValues(PAL::SessionID, CompletionHandler<void()>&&);
    void scheduleClearInMemoryAndPersistent(PAL::SessionID, Optional<WallTime> modifiedSince, ShouldGrandfatherStatistics, CompletionHandler<void()>&&);
    void scheduleCookieBlockingUpdate(PAL::SessionID, CompletionHandler<void()>&&);
    void scheduleStatisticsAndDataRecordsProcessing(PAL::SessionID, CompletionHandler<void()>&&);
    void submitTelemetry(PAL::SessionID, CompletionHandler<void()>&&);
    void setCacheMaxAgeCapForPrevalentResources(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setGrandfatheringTime(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setLastSeen(PAL::SessionID, const RegistrableDomain&, Seconds, CompletionHandler<void()>&&);
    void setMinimumTimeBetweenDataRecordsRemoval(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setNotifyPagesWhenDataRecordsWereScanned(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setIsRunningResourceLoadStatisticsTest(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setNotifyPagesWhenTelemetryWasCaptured(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setResourceLoadStatisticsEnabled(bool);
    void setResourceLoadStatisticsLogTestingEvent(bool);
    void setResourceLoadStatisticsDebugMode(PAL::SessionID, bool debugMode, CompletionHandler<void()>&&d);
    void setShouldClassifyResourcesBeforeDataRecordsRemoval(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setSubframeUnderTopFrameDomain(PAL::SessionID, const SubFrameDomain&, const TopFrameDomain&, CompletionHandler<void()>&&);
    void setSubresourceUnderTopFrameDomain(PAL::SessionID, const SubResourceDomain&, const TopFrameDomain&, CompletionHandler<void()>&&);
    void setSubresourceUniqueRedirectTo(PAL::SessionID, const SubResourceDomain&, const RedirectedToDomain&, CompletionHandler<void()>&&);
    void setSubresourceUniqueRedirectFrom(PAL::SessionID, const SubResourceDomain&, const RedirectedFromDomain&, CompletionHandler<void()>&&);
    void setTimeToLiveUserInteraction(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setTopFrameUniqueRedirectTo(PAL::SessionID, const TopFrameDomain&, const RedirectedToDomain&, CompletionHandler<void()>&&);
    void setTopFrameUniqueRedirectFrom(PAL::SessionID, const TopFrameDomain&, const RedirectedFromDomain&, CompletionHandler<void()>&&);
    void registrableDomainsWithWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, bool shouldNotifyPage, CompletionHandler<void(HashSet<RegistrableDomain>&&)>&&);
    void didCommitCrossSiteLoadWithDataTransfer(PAL::SessionID, const RegistrableDomain& fromDomain, const RegistrableDomain& toDomain, OptionSet<WebCore::CrossSiteNavigationDataTransfer::Flag>, WebCore::PageIdentifier);
    void setCrossSiteLoadWithLinkDecorationForTesting(PAL::SessionID, const RegistrableDomain& fromDomain, const RegistrableDomain& toDomain, CompletionHandler<void()>&&);
    void resetCrossSiteLoadsWithLinkDecorationForTesting(PAL::SessionID, CompletionHandler<void()>&&);
    void hasIsolatedSession(PAL::SessionID, const WebCore::RegistrableDomain&, CompletionHandler<void(bool)>&&) const;
#endif

    using CacheStorageRootPathCallback = CompletionHandler<void(String&&)>;
    void cacheStorageRootPath(PAL::SessionID, CacheStorageRootPathCallback&&);

    void preconnectTo(const URL&, WebCore::StoredCredentialsPolicy);

    void setSessionIsControlledByAutomation(PAL::SessionID, bool);
    bool sessionIsControlledByAutomation(PAL::SessionID) const;

    void connectionToWebProcessClosed(IPC::Connection&);
    void getLocalStorageOriginDetails(PAL::SessionID, CompletionHandler<void(Vector<LocalStorageDatabaseTracker::OriginDetails>&&)>&&);

#if ENABLE(CONTENT_EXTENSIONS)
    NetworkContentRuleListManager& networkContentRuleListManager() { return m_networkContentRuleListManager; }
#endif

#if ENABLE(INDEXED_DATABASE)
    WebCore::IDBServer::IDBServer& idbServer(PAL::SessionID);
    // WebCore::IDBServer::IDBBackingStoreFileHandler.
    void accessToTemporaryFileComplete(const String& path) final;
#endif

    void syncLocalStorage(CompletionHandler<void()>&&);
    void clearLegacyPrivateBrowsingLocalStorage();

    void updateQuotaBasedOnSpaceUsageForTesting(PAL::SessionID, const WebCore::ClientOrigin&);

#if ENABLE(SANDBOX_EXTENSIONS)
    void getSandboxExtensionsForBlobFiles(const Vector<String>& filenames, CompletionHandler<void(SandboxExtension::HandleArray&&)>&&);
#endif

    void didReceiveNetworkProcessMessage(IPC::Connection&, IPC::Decoder&);

#if ENABLE(SERVICE_WORKER)
    WebSWServerToContextConnection* serverToContextConnectionForRegistrableDomain(const WebCore::RegistrableDomain&);
    void createServerToContextConnection(const WebCore::RegistrableDomain&, Optional<PAL::SessionID>);
    
    WebCore::SWServer& swServerForSession(PAL::SessionID);
    void registerSWServerConnection(WebSWServerConnection&);
    void unregisterSWServerConnection(WebSWServerConnection&);
    
    void swContextConnectionMayNoLongerBeNeeded(WebSWServerToContextConnection&);
    
    WebSWServerToContextConnection* connectionToContextProcessFromIPCConnection(IPC::Connection&);
    void connectionToContextProcessWasClosed(Ref<WebSWServerToContextConnection>&&);
#endif

#if PLATFORM(IOS_FAMILY)
    bool parentProcessHasServiceWorkerEntitlement() const;
#else
    bool parentProcessHasServiceWorkerEntitlement() const { return true; }
#endif

#if PLATFORM(COCOA)
    NetworkHTTPSUpgradeChecker& networkHTTPSUpgradeChecker();
#endif

    const String& uiProcessBundleIdentifier() const { return m_uiProcessBundleIdentifier; }

    void ref() const override { ThreadSafeRefCounted<NetworkProcess>::ref(); }
    void deref() const override { ThreadSafeRefCounted<NetworkProcess>::deref(); }

    CacheStorage::Engine* findCacheEngine(const PAL::SessionID&);
    CacheStorage::Engine& ensureCacheEngine(const PAL::SessionID&, Function<Ref<CacheStorage::Engine>()>&&);
    void removeCacheEngine(const PAL::SessionID&);
    void requestStorageSpace(PAL::SessionID, const WebCore::ClientOrigin&, uint64_t quota, uint64_t currentSize, uint64_t spaceRequired, CompletionHandler<void(Optional<uint64_t>)>&&);

    WebCore::BlobRegistryImpl* blobRegistry(NetworkConnectionToWebProcess&);

    void storeAdClickAttribution(PAL::SessionID, WebCore::AdClickAttribution&&);
    void dumpAdClickAttribution(PAL::SessionID, CompletionHandler<void(String)>&&);
    void clearAdClickAttribution(PAL::SessionID, CompletionHandler<void()>&&);
    void setAdClickAttributionOverrideTimerForTesting(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setAdClickAttributionConversionURLForTesting(PAL::SessionID, URL&&, CompletionHandler<void()>&&);
    void markAdClickAttributionsAsExpiredForTesting(PAL::SessionID, CompletionHandler<void()>&&);

    WebCore::StorageQuotaManager& storageQuotaManager(PAL::SessionID, const WebCore::ClientOrigin&);

    void addKeptAliveLoad(Ref<NetworkResourceLoader>&&);
    void removeKeptAliveLoad(NetworkResourceLoader&);

    const String& diskCacheDirectory() const { return m_diskCacheDirectory; }
    const OptionSet<NetworkCache::CacheOption>& cacheOptions() const { return m_cacheOptions; }
    
private:
    void platformInitializeNetworkProcess(const NetworkProcessCreationParameters&);
    std::unique_ptr<WebCore::NetworkStorageSession> platformCreateDefaultStorageSession() const;

    void terminate() override;
    void platformTerminate();

    void lowMemoryHandler(Critical);
    
    void processDidTransitionToForeground();
    void processDidTransitionToBackground();
    void platformProcessDidTransitionToForeground();
    void platformProcessDidTransitionToBackground();

    enum class ShouldAcknowledgeWhenReadyToSuspend { No, Yes };
    void actualPrepareToSuspend(ShouldAcknowledgeWhenReadyToSuspend);
    void platformPrepareToSuspend(CompletionHandler<void()>&&);
    void platformProcessDidResume();

    // AuxiliaryProcess
    void initializeProcess(const AuxiliaryProcessInitializationParameters&) override;
    void initializeProcessName(const AuxiliaryProcessInitializationParameters&) override;
    void initializeSandbox(const AuxiliaryProcessInitializationParameters&, SandboxInitializationParameters&) override;
    void initializeConnection(IPC::Connection*) override;
    bool shouldTerminate() override;

    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;

    // DownloadManager::Client
    void didCreateDownload() override;
    void didDestroyDownload() override;
    IPC::Connection* downloadProxyConnection() override;
    IPC::Connection* parentProcessConnectionForDownloads() override { return parentProcessConnection(); }
    AuthenticationManager& downloadsAuthenticationManager() override;
    void pendingDownloadCanceled(DownloadID) override;
    uint32_t downloadMonitorSpeedMultiplier() const override { return m_downloadMonitorSpeedMultiplier; }

    // Message Handlers
    void didReceiveSyncNetworkProcessMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);
    void initializeNetworkProcess(NetworkProcessCreationParameters&&);
    void createNetworkConnectionToWebProcess(bool isServiceWorkerProcess, WebCore::RegistrableDomain&&);

    void fetchWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, OptionSet<WebsiteDataFetchOption>, uint64_t callbackID);
    void deleteWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, WallTime modifiedSince, uint64_t callbackID);
    void deleteWebsiteDataForOrigins(PAL::SessionID, OptionSet<WebsiteDataType>, const Vector<WebCore::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, const Vector<String>& HSTSCacheHostnames, uint64_t callbackID);

    void clearCachedCredentials();

    void setCacheStorageParameters(PAL::SessionID, String&& cacheStorageDirectory, SandboxExtension::Handle&&);
    void initializeQuotaUsers(WebCore::StorageQuotaManager&, PAL::SessionID, const WebCore::ClientOrigin&);

    // FIXME: This should take a session ID so we can identify which disk cache to delete.
    void clearDiskCache(WallTime modifiedSince, CompletionHandler<void()>&&);

    void downloadRequest(PAL::SessionID, DownloadID, const WebCore::ResourceRequest&, const String& suggestedFilename);
    void resumeDownload(PAL::SessionID, DownloadID, const IPC::DataReference& resumeData, const String& path, SandboxExtension::Handle&&);
    void cancelDownload(DownloadID);
#if PLATFORM(COCOA)
    void publishDownloadProgress(DownloadID, const URL&, SandboxExtension::Handle&&);
#endif
    void continueWillSendRequest(DownloadID, WebCore::ResourceRequest&&);
    void continueDecidePendingDownloadDestination(DownloadID, String destination, SandboxExtension::Handle&&, bool allowOverwrite);
    void applicationDidEnterBackground();
    void applicationWillEnterForeground();

    void setCacheModel(CacheModel);
    void allowSpecificHTTPSCertificateForHost(const WebCore::CertificateInfo&, const String& host);
    void setCanHandleHTTPSServerTrustEvaluation(bool);
    void getNetworkProcessStatistics(uint64_t callbackID);
    void clearCacheForAllOrigins(uint32_t cachesToClear);
    void setAllowsAnySSLCertificateForWebSocket(bool, CompletionHandler<void()>&&);
    
    void syncAllCookies();
    void didSyncAllCookies();

#if USE(SOUP)
    void setIgnoreTLSErrors(bool);
    void userPreferredLanguagesChanged(const Vector<String>&);
    void setNetworkProxySettings(const WebCore::SoupNetworkProxySettings&);
#endif

#if USE(CURL)
    void setNetworkProxySettings(PAL::SessionID, WebCore::CurlProxySettings&&);
#endif

#if PLATFORM(MAC)
    static void setSharedHTTPCookieStorage(const Vector<uint8_t>& identifier);
#endif

    void platformSyncAllCookies(CompletionHandler<void()>&&);
    
    void registerURLSchemeAsSecure(const String&) const;
    void registerURLSchemeAsBypassingContentSecurityPolicy(const String&) const;
    void registerURLSchemeAsLocal(const String&) const;
    void registerURLSchemeAsNoAccess(const String&) const;
    void registerURLSchemeAsDisplayIsolated(const String&) const;
    void registerURLSchemeAsCORSEnabled(const String&) const;
    void registerURLSchemeAsCanDisplayOnlyIfCanRequest(const String&) const;

#if ENABLE(INDEXED_DATABASE)
    void addIndexedDatabaseSession(PAL::SessionID, String&, SandboxExtension::Handle&);
    void collectIndexedDatabaseOriginsForVersion(const String&, HashSet<WebCore::SecurityOriginData>&);
    HashSet<WebCore::SecurityOriginData> indexedDatabaseOrigins(const String& path);
    Ref<WebCore::IDBServer::IDBServer> createIDBServer(PAL::SessionID);
#endif

#if ENABLE(SERVICE_WORKER)
    void didCreateWorkerContextProcessConnection(const IPC::Attachment&);
    
    void postMessageToServiceWorkerClient(const WebCore::ServiceWorkerClientIdentifier& destinationIdentifier, WebCore::MessageWithMessagePorts&&, WebCore::ServiceWorkerIdentifier sourceIdentifier, const String& sourceOrigin);
    void postMessageToServiceWorker(WebCore::ServiceWorkerIdentifier destination, WebCore::MessageWithMessagePorts&&, const WebCore::ServiceWorkerOrClientIdentifier& source, WebCore::SWServerConnectionIdentifier);
    
    void disableServiceWorkerProcessTerminationDelay();
    
    WebSWOriginStore* existingSWOriginStoreForSession(PAL::SessionID) const;
    bool needsServerToContextConnectionForRegistrableDomain(const WebCore::RegistrableDomain&) const;

    void addServiceWorkerSession(PAL::SessionID, String& serviceWorkerRegistrationDirectory, const SandboxExtension::Handle&);
#endif

    void postStorageTask(CrossThreadTask&&);
    // For execution on work queue thread only.
    void performNextStorageTask();
    void ensurePathExists(const String& path);

    void clearStorageQuota(PAL::SessionID);
    void initializeStorageQuota(const WebsiteDataStoreParameters&);

    // Connections to WebProcesses.
    Vector<Ref<NetworkConnectionToWebProcess>> m_webProcessConnections;

    String m_diskCacheDirectory;
    bool m_hasSetCacheModel { false };
    CacheModel m_cacheModel { CacheModel::DocumentViewer };
    bool m_suppressMemoryPressureHandler { false };
    bool m_diskCacheIsDisabledForTesting { false };
    bool m_canHandleHTTPSServerTrustEvaluation { true };
    String m_uiProcessBundleIdentifier;
    DownloadManager m_downloadManager;

    HashMap<PAL::SessionID, Ref<CacheStorage::Engine>> m_cacheEngines;

    typedef HashMap<const char*, std::unique_ptr<NetworkProcessSupplement>, PtrHash<const char*>> NetworkProcessSupplementMap;
    NetworkProcessSupplementMap m_supplements;

    HashSet<PAL::SessionID> m_sessionsControlledByAutomation;
    HashMap<PAL::SessionID, Vector<CacheStorageRootPathCallback>> m_cacheStorageParametersCallbacks;

    HashMap<PAL::SessionID, std::unique_ptr<NetworkSession>> m_networkSessions;
    HashMap<PAL::SessionID, std::unique_ptr<WebCore::NetworkStorageSession>> m_networkStorageSessions;
    mutable std::unique_ptr<WebCore::NetworkStorageSession> m_defaultNetworkStorageSession;

    RefPtr<StorageManagerSet> m_storageManagerSet;

#if PLATFORM(COCOA)
    void platformInitializeNetworkProcessCocoa(const NetworkProcessCreationParameters&);
    void setStorageAccessAPIEnabled(bool);

    // FIXME: We'd like to be able to do this without the #ifdef, but WorkQueue + BinarySemaphore isn't good enough since
    // multiple requests to clear the cache can come in before previous requests complete, and we need to wait for all of them.
    // In the future using WorkQueue and a counting semaphore would work, as would WorkQueue supporting the libdispatch concept of "work groups".
    dispatch_group_t m_clearCacheDispatchGroup { nullptr };

    bool m_suppressesConnectionTerminationOnSystemChange { false };
#endif

#if ENABLE(CONTENT_EXTENSIONS)
    NetworkContentRuleListManager m_networkContentRuleListManager;
#endif

#if PLATFORM(IOS_FAMILY)
    WebSQLiteDatabaseTracker m_webSQLiteDatabaseTracker;
#endif

    Ref<WorkQueue> m_storageTaskQueue { WorkQueue::create("com.apple.WebKit.StorageTask") };

#if ENABLE(INDEXED_DATABASE)
    HashMap<PAL::SessionID, String> m_idbDatabasePaths;
    HashMap<PAL::SessionID, RefPtr<WebCore::IDBServer::IDBServer>> m_idbServers;
#endif

    Deque<CrossThreadTask> m_storageTasks;
    Lock m_storageTaskMutex;
    
#if ENABLE(SERVICE_WORKER)
    HashMap<WebCore::RegistrableDomain, RefPtr<WebSWServerToContextConnection>> m_serverToContextConnections;
    bool m_waitingForServerToContextProcessConnection { false };
    bool m_shouldDisableServiceWorkerProcessTerminationDelay { false };
    HashMap<PAL::SessionID, String> m_swDatabasePaths;
    HashMap<PAL::SessionID, std::unique_ptr<WebCore::SWServer>> m_swServers;
    HashMap<WebCore::SWServerConnectionIdentifier, WeakPtr<WebSWServerConnection>> m_swServerConnections;
#endif

#if PLATFORM(COCOA)
    std::unique_ptr<NetworkHTTPSUpgradeChecker> m_networkHTTPSUpgradeChecker;
#endif

    class StorageQuotaManagers {
    public:
        uint64_t defaultQuota(const WebCore::ClientOrigin& origin) const { return origin.topOrigin == origin.clientOrigin ? m_defaultQuota : m_defaultThirdPartyQuota; }
        void setDefaultQuotas(uint64_t defaultQuota, uint64_t defaultThirdPartyQuota)
        {
            m_defaultQuota = defaultQuota;
            m_defaultThirdPartyQuota = defaultThirdPartyQuota;
        }

        HashMap<WebCore::ClientOrigin, std::unique_ptr<WebCore::StorageQuotaManager>>& managersPerOrigin() { return m_managersPerOrigin; }

    private:
        uint64_t m_defaultQuota { WebCore::StorageQuotaManager::defaultQuota() };
        uint64_t m_defaultThirdPartyQuota { WebCore::StorageQuotaManager::defaultThirdPartyQuota() };
        HashMap<WebCore::ClientOrigin, std::unique_ptr<WebCore::StorageQuotaManager>> m_managersPerOrigin;
    };
    HashMap<PAL::SessionID, StorageQuotaManagers> m_storageQuotaManagers;
    uint32_t m_downloadMonitorSpeedMultiplier { 1 };

    HashMap<IPC::Connection::UniqueID, PAL::SessionID> m_sessionByConnection;

    OptionSet<NetworkCache::CacheOption> m_cacheOptions;
};

} // namespace WebKit
