/*
 * Copyright (C) 2012-2017 Apple Inc. All rights reserved.
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

#include "CacheModel.h"
#include "ChildProcess.h"
#include "DownloadManager.h"
#include "MessageReceiverMap.h"
#include <WebCore/DiagnosticLoggingClient.h>
#include <memory>
#include <pal/SessionID.h>
#include <wtf/Forward.h>
#include <wtf/Function.h>
#include <wtf/HashSet.h>
#include <wtf/MemoryPressureHandler.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RetainPtr.h>
#include <wtf/WeakPtr.h>

#if PLATFORM(IOS)
#include "WebSQLiteDatabaseTracker.h"
#endif

namespace PAL {
class SessionID;
}

namespace WebCore {
class DownloadID;
class CertificateInfo;
class NetworkStorageSession;
class ProtectionSpace;
class SecurityOrigin;
struct SecurityOriginData;
struct SoupNetworkProxySettings;
enum class StoredCredentialsPolicy;
class URL;
}

namespace WebKit {
class AuthenticationManager;
#if ENABLE(SERVER_PRECONNECT)
class PreconnectTask;
#endif
class NetworkConnectionToWebProcess;
class NetworkProcessSupplement;
class NetworkResourceLoader;
enum class WebsiteDataFetchOption;
enum class WebsiteDataType;
struct NetworkProcessCreationParameters;
struct WebsiteDataStoreParameters;

namespace NetworkCache {
class Cache;
}

class NetworkProcess : public ChildProcess, private DownloadManager::Client {
    WTF_MAKE_NONCOPYABLE(NetworkProcess);
    friend NeverDestroyed<NetworkProcess>;
    friend NeverDestroyed<DownloadManager>;
public:
    static NetworkProcess& singleton();

    template <typename T>
    T* supplement()
    {
        return static_cast<T*>(m_supplements.get(T::supplementName()));
    }

    template <typename T>
    void addSupplement()
    {
        m_supplements.add(T::supplementName(), std::make_unique<T>(*this));
    }

    void removeNetworkConnectionToWebProcess(NetworkConnectionToWebProcess*);

    AuthenticationManager& authenticationManager();
    DownloadManager& downloadManager();

    NetworkCache::Cache* cache() { return m_cache.get(); }

    bool canHandleHTTPSServerTrustEvaluation() const { return m_canHandleHTTPSServerTrustEvaluation; }

    void processWillSuspendImminently(bool& handled);
    void prepareToSuspend();
    void cancelPrepareToSuspend();
    void processDidResume();

    // Diagnostic messages logging.
    void logDiagnosticMessage(uint64_t webPageID, const String& message, const String& description, WebCore::ShouldSample);
    void logDiagnosticMessageWithResult(uint64_t webPageID, const String& message, const String& description, WebCore::DiagnosticLoggingResultType, WebCore::ShouldSample);
    void logDiagnosticMessageWithValue(uint64_t webPageID, const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample);

#if PLATFORM(COCOA)
    RetainPtr<CFDataRef> sourceApplicationAuditData() const;
    void clearHSTSCache(WebCore::NetworkStorageSession&, WallTime modifiedSince);
#endif

    void findPendingDownloadLocation(NetworkDataTask&, ResponseCompletionHandler&&, const WebCore::ResourceResponse&);

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    void canAuthenticateAgainstProtectionSpace(NetworkResourceLoader&, const WebCore::ProtectionSpace&);
#if ENABLE(SERVER_PRECONNECT)
    void canAuthenticateAgainstProtectionSpace(PreconnectTask&, const WebCore::ProtectionSpace&);
#endif
#endif

    void prefetchDNS(const String&);

    void addWebsiteDataStore(WebsiteDataStoreParameters&&);

    void grantSandboxExtensionsToStorageProcessForBlobs(const Vector<String>& filenames, Function<void ()>&& completionHandler);

#if HAVE(CFNETWORK_STORAGE_PARTITIONING)
    void updatePrevalentDomainsToPartitionOrBlockCookies(PAL::SessionID, const Vector<String>& domainsToPartition, const Vector<String>& domainsToBlock, const Vector<String>& domainsToNeitherPartitionNorBlock, bool shouldClearFirst);
    void hasStorageAccessForFrame(PAL::SessionID, const String& resourceDomain, const String& firstPartyDomain, uint64_t frameID, uint64_t pageID, uint64_t contextId);
    void getAllStorageAccessEntries(PAL::SessionID, uint64_t contextId);
    void grantStorageAccessForFrame(PAL::SessionID, const String& resourceDomain, const String& firstPartyDomain, uint64_t frameID, uint64_t pageID, uint64_t contextId);
    void removePrevalentDomains(PAL::SessionID, const Vector<String>& domains);
#endif

    Seconds loadThrottleLatency() const { return m_loadThrottleLatency; }
    String cacheStorageDirectory(PAL::SessionID) const;
    uint64_t cacheStoragePerOriginQuota() const;

    void preconnectTo(const WebCore::URL&, WebCore::StoredCredentialsPolicy);

#if HAVE(CFNETWORK_STORAGE_PARTITIONING) && !RELEASE_LOG_DISABLED
    bool shouldLogCookieInformation() const { return m_logCookieInformation; }
#endif

    void setSessionIsControlledByAutomation(PAL::SessionID, bool);
    bool sessionIsControlledByAutomation(PAL::SessionID) const;

private:
    NetworkProcess();
    ~NetworkProcess();

    void platformInitializeNetworkProcess(const NetworkProcessCreationParameters&);

    void terminate() override;
    void platformTerminate();

    void lowMemoryHandler(Critical);

    enum class ShouldAcknowledgeWhenReadyToSuspend { No, Yes };
    void actualPrepareToSuspend(ShouldAcknowledgeWhenReadyToSuspend);

    // ChildProcess
    void initializeProcess(const ChildProcessInitializationParameters&) override;
    void initializeProcessName(const ChildProcessInitializationParameters&) override;
    void initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&) override;
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
    AuthenticationManager& downloadsAuthenticationManager() override;
    void pendingDownloadCanceled(DownloadID) override;

    // Message Handlers
    void didReceiveNetworkProcessMessage(IPC::Connection&, IPC::Decoder&);
    void didReceiveSyncNetworkProcessMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);
    void initializeNetworkProcess(NetworkProcessCreationParameters&&);
    void createNetworkConnectionToWebProcess();
    void destroySession(PAL::SessionID);

    void fetchWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, OptionSet<WebsiteDataFetchOption>, uint64_t callbackID);
    void deleteWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, WallTime modifiedSince, uint64_t callbackID);
    void deleteWebsiteDataForOrigins(PAL::SessionID, OptionSet<WebsiteDataType>, const Vector<WebCore::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, uint64_t callbackID);

    void clearCachedCredentials();

    // FIXME: This should take a session ID so we can identify which disk cache to delete.
    void clearDiskCache(WallTime modifiedSince, Function<void ()>&& completionHandler);

    void downloadRequest(PAL::SessionID, DownloadID, const WebCore::ResourceRequest&, const String& suggestedFilename);
    void resumeDownload(PAL::SessionID, DownloadID, const IPC::DataReference& resumeData, const String& path, SandboxExtension::Handle&&);
    void cancelDownload(DownloadID);
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    void continueCanAuthenticateAgainstProtectionSpace(uint64_t resourceLoadIdentifier, bool canAuthenticate);
#endif
    void continueWillSendRequest(DownloadID, WebCore::ResourceRequest&&);
    void continueDecidePendingDownloadDestination(DownloadID, String destination, SandboxExtension::Handle&&, bool allowOverwrite);

    void setCacheModel(uint32_t);
    void allowSpecificHTTPSCertificateForHost(const WebCore::CertificateInfo&, const String& host);
    void setCanHandleHTTPSServerTrustEvaluation(bool);
    void getNetworkProcessStatistics(uint64_t callbackID);
    void clearCacheForAllOrigins(uint32_t cachesToClear);
    void setAllowsAnySSLCertificateForWebSocket(bool);
    void syncAllCookies();

    void didGrantSandboxExtensionsToStorageProcessForBlobs(uint64_t requestID);

    void writeBlobToFilePath(const WebCore::URL&, const String& path, SandboxExtension::Handle&&, uint64_t requestID);

#if USE(SOUP)
    void setIgnoreTLSErrors(bool);
    void userPreferredLanguagesChanged(const Vector<String>&);
    void setNetworkProxySettings(const WebCore::SoupNetworkProxySettings&);
#endif

    // Platform Helpers
    void platformSetURLCacheSize(unsigned urlCacheMemoryCapacity, uint64_t urlCacheDiskCapacity);

    // Connections to WebProcesses.
    Vector<RefPtr<NetworkConnectionToWebProcess>> m_webProcessConnections;

    String m_cacheStorageDirectory;
    uint64_t m_cacheStoragePerOriginQuota { 0 };
    String m_diskCacheDirectory;
    bool m_hasSetCacheModel;
    CacheModel m_cacheModel;
    int64_t m_diskCacheSizeOverride { -1 };
    bool m_suppressMemoryPressureHandler { false };
    bool m_diskCacheIsDisabledForTesting;
    bool m_canHandleHTTPSServerTrustEvaluation;
    Seconds m_loadThrottleLatency;
#if HAVE(CFNETWORK_STORAGE_PARTITIONING) && !RELEASE_LOG_DISABLED
    bool m_logCookieInformation { false };
#endif

    RefPtr<NetworkCache::Cache> m_cache;

    typedef HashMap<const char*, std::unique_ptr<NetworkProcessSupplement>, PtrHash<const char*>> NetworkProcessSupplementMap;
    NetworkProcessSupplementMap m_supplements;

    HashMap<uint64_t, Function<void ()>> m_sandboxExtensionForBlobsCompletionHandlers;
    HashMap<uint64_t, Ref<NetworkResourceLoader>> m_waitingNetworkResourceLoaders;
#if ENABLE(SERVER_PRECONNECT)
    HashMap<uint64_t, WeakPtr<PreconnectTask>> m_waitingPreconnectTasks;
#endif
    HashSet<PAL::SessionID> m_sessionsControlledByAutomation;

#if PLATFORM(COCOA)
    void platformInitializeNetworkProcessCocoa(const NetworkProcessCreationParameters&);
    void setCookieStoragePartitioningEnabled(bool);
    void setStorageAccessAPIEnabled(bool);

    // FIXME: We'd like to be able to do this without the #ifdef, but WorkQueue + BinarySemaphore isn't good enough since
    // multiple requests to clear the cache can come in before previous requests complete, and we need to wait for all of them.
    // In the future using WorkQueue and a counting semaphore would work, as would WorkQueue supporting the libdispatch concept of "work groups".
    dispatch_group_t m_clearCacheDispatchGroup;
#endif

#if PLATFORM(IOS)
    WebSQLiteDatabaseTracker m_webSQLiteDatabaseTracker;
#endif
};

} // namespace WebKit
