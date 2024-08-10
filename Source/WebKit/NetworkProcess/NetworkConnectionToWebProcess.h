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

#include "BlockingResponseMap.h"
#include "CacheStorageEngineConnection.h"
#include "Connection.h"
#include "DownloadID.h"
#include "NetworkActivityTracker.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkMDNSRegister.h"
#include "NetworkRTCProvider.h"
#include <WebCore/NetworkLoadInformation.h>
#include <WebCore/ResourceLoadPriority.h>
#include <wtf/RefCounted.h>

namespace WebCore {
class BlobDataFileReference;
class HTTPHeaderMap;
class ResourceError;
class ResourceRequest;
struct SameSiteInfo;

enum class IncludeSecureCookies;
}

namespace WebKit {

class NetworkConnectionToWebProcess;
class NetworkLoadParameters;
class NetworkResourceLoader;
class NetworkSocketStream;
class SyncNetworkResourceLoader;
typedef uint64_t ResourceLoadIdentifier;

namespace NetworkCache {
struct DataKey;
}

class NetworkConnectionToWebProcess : public RefCounted<NetworkConnectionToWebProcess>, IPC::Connection::Client {
public:
    static Ref<NetworkConnectionToWebProcess> create(IPC::Connection::Identifier);
    virtual ~NetworkConnectionToWebProcess();

    IPC::Connection& connection() { return m_connection.get(); }

    void didCleanupResourceLoader(NetworkResourceLoader&);
    void setOnLineState(bool);

    bool captureExtraNetworkLoadMetricsEnabled() const { return m_captureExtraNetworkLoadMetricsEnabled; }

    RefPtr<WebCore::BlobDataFileReference> getBlobDataFileReferenceForPath(const String& path);

    void cleanupForSuspension(Function<void()>&&);
    void endSuspension();

    void getNetworkLoadInformationRequest(ResourceLoadIdentifier identifier, WebCore::ResourceRequest& request)
    {
        request = m_networkLoadInformationByID.get(identifier).request;
    }

    void getNetworkLoadInformationResponse(ResourceLoadIdentifier identifier, WebCore::ResourceResponse& response)
    {
        response = m_networkLoadInformationByID.get(identifier).response;
    }

    void getNetworkLoadIntermediateInformation(ResourceLoadIdentifier identifier, Vector<WebCore::NetworkTransactionInformation>& information)
    {
        information = m_networkLoadInformationByID.get(identifier).transactions;
    }

    void takeNetworkLoadInformationMetrics(ResourceLoadIdentifier identifier, WebCore::NetworkLoadMetrics& metrics)
    {
        metrics = m_networkLoadInformationByID.take(identifier).metrics;
    }

    void addNetworkLoadInformation(ResourceLoadIdentifier identifier, WebCore::NetworkLoadInformation&& information)
    {
        ASSERT(!m_networkLoadInformationByID.contains(identifier));
        m_networkLoadInformationByID.add(identifier, WTFMove(information));
    }

    void addNetworkLoadInformationMetrics(ResourceLoadIdentifier identifier, const WebCore::NetworkLoadMetrics& metrics)
    {
        ASSERT(m_networkLoadInformationByID.contains(identifier));
        m_networkLoadInformationByID.ensure(identifier, [] {
            return WebCore::NetworkLoadInformation { };
        }).iterator->value.metrics = metrics;
    }

    void removeNetworkLoadInformation(ResourceLoadIdentifier identifier)
    {
        m_networkLoadInformationByID.remove(identifier);
    }

    std::optional<NetworkActivityTracker> startTrackingResourceLoad(uint64_t pageID, ResourceLoadIdentifier resourceID, bool isMainResource, const PAL::SessionID&);
    void stopTrackingResourceLoad(ResourceLoadIdentifier resourceID, NetworkActivityTracker::CompletionCode);

private:
    NetworkConnectionToWebProcess(IPC::Connection::Identifier);

    void didFinishPreconnection(uint64_t preconnectionIdentifier, const WebCore::ResourceError&);

    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;
    void didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference messageReceiverName, IPC::StringReference messageName) override;

    // Message handlers.
    void didReceiveNetworkConnectionToWebProcessMessage(IPC::Connection&, IPC::Decoder&);
    void didReceiveSyncNetworkConnectionToWebProcessMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);

    void scheduleResourceLoad(NetworkResourceLoadParameters&&);
    void performSynchronousLoad(NetworkResourceLoadParameters&&, Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad::DelayedReply&&);
    void loadPing(NetworkResourceLoadParameters&&);
    void prefetchDNS(const String&);
    void preconnectTo(uint64_t preconnectionIdentifier, NetworkResourceLoadParameters&&);

    void removeLoadIdentifier(ResourceLoadIdentifier);
    void pageLoadCompleted(uint64_t webPageID);
    void setDefersLoading(ResourceLoadIdentifier, bool);
    void crossOriginRedirectReceived(ResourceLoadIdentifier, const WebCore::URL& redirectURL);
    void startDownload(PAL::SessionID, DownloadID, const WebCore::ResourceRequest&, const String& suggestedName = { });
    void convertMainResourceLoadToDownload(PAL::SessionID, uint64_t mainResourceLoadIdentifier, DownloadID, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);

    void cookiesForDOM(PAL::SessionID, const WebCore::URL& firstParty, const WebCore::SameSiteInfo&, const WebCore::URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, WebCore::IncludeSecureCookies, String& cookieString, bool& secureCookiesAccessed);
    void setCookiesFromDOM(PAL::SessionID, const WebCore::URL& firstParty, const WebCore::SameSiteInfo&, const WebCore::URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, const String&);
    void cookiesEnabled(PAL::SessionID, bool& result);
    void cookieRequestHeaderFieldValue(PAL::SessionID, const WebCore::URL& firstParty, const WebCore::SameSiteInfo&, const WebCore::URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, WebCore::IncludeSecureCookies, String& cookieString, bool& secureCookiesAccessed);
    void getRawCookies(PAL::SessionID, const WebCore::URL& firstParty, const WebCore::SameSiteInfo&, const WebCore::URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, Vector<WebCore::Cookie>&);
    void deleteCookie(PAL::SessionID, const WebCore::URL&, const String& cookieName);

    void registerFileBlobURL(const WebCore::URL&, const String& path, SandboxExtension::Handle&&, const String& contentType);
    void registerBlobURL(const WebCore::URL&, Vector<WebCore::BlobPart>&&, const String& contentType);
    void registerBlobURLFromURL(const WebCore::URL&, const WebCore::URL& srcURL, bool shouldBypassConnectionCheck);
    void preregisterSandboxExtensionsForOptionallyFileBackedBlob(const Vector<String>& fileBackedPath, SandboxExtension::HandleArray&&);
    void registerBlobURLOptionallyFileBacked(const WebCore::URL&, const WebCore::URL& srcURL, const String& fileBackedPath, const String& contentType);
    void registerBlobURLForSlice(const WebCore::URL&, const WebCore::URL& srcURL, int64_t start, int64_t end);
    void blobSize(const WebCore::URL&, uint64_t& resultSize);
    void unregisterBlobURL(const WebCore::URL&);
    void writeBlobsToTemporaryFiles(const Vector<String>& blobURLs, uint64_t requestIdentifier);

    void storeDerivedDataToCache(const WebKit::NetworkCache::DataKey&, const IPC::DataReference&);

    void setCaptureExtraNetworkLoadMetricsEnabled(bool);

    void createSocketStream(WebCore::URL&&, PAL::SessionID, String cachePartition, uint64_t);
    void destroySocketStream(uint64_t);
    
    void ensureLegacyPrivateBrowsingSession();

#if USE(LIBWEBRTC)
    NetworkRTCProvider& rtcProvider();
#endif
#if ENABLE(WEB_RTC)
    NetworkMDNSRegister& mdnsRegister() { return m_mdnsRegister; }
#endif

    CacheStorageEngineConnection& cacheStorageConnection();

    void removeStorageAccessForFrame(PAL::SessionID, uint64_t frameID, uint64_t pageID);
    void removeStorageAccessForAllFramesOnPage(PAL::SessionID, uint64_t pageID);

    void addOriginAccessWhitelistEntry(const String& sourceOrigin, const String& destinationProtocol, const String& destinationHost, bool allowDestinationSubdomains);
    void removeOriginAccessWhitelistEntry(const String& sourceOrigin, const String& destinationProtocol, const String& destinationHost, bool allowDestinationSubdomains);
    void resetOriginAccessWhitelists();

    struct ResourceNetworkActivityTracker {
        ResourceNetworkActivityTracker() = default;
        ResourceNetworkActivityTracker(const ResourceNetworkActivityTracker&) = default;
        ResourceNetworkActivityTracker(ResourceNetworkActivityTracker&&) = default;
        ResourceNetworkActivityTracker(uint64_t pageID)
            : pageID { pageID }
            , isRootActivity { true }
            , networkActivity { NetworkActivityTracker::Label::LoadPage }
        {
        }

        ResourceNetworkActivityTracker(uint64_t pageID, ResourceLoadIdentifier resourceID)
            : pageID { pageID }
            , resourceID { resourceID }
            , networkActivity { NetworkActivityTracker::Label::LoadResource }
        {
        }

        uint64_t pageID { 0 };
        ResourceLoadIdentifier resourceID { 0 };
        bool isRootActivity { false };
        NetworkActivityTracker networkActivity;
    };

    void stopAllNetworkActivityTracking();
    void stopAllNetworkActivityTrackingForPage(uint64_t pageID);
    size_t findRootNetworkActivity(uint64_t pageID);
    size_t findNetworkActivityTracker(ResourceLoadIdentifier resourceID);

    Ref<IPC::Connection> m_connection;

    HashMap<uint64_t, RefPtr<NetworkSocketStream>> m_networkSocketStreams;
    HashMap<ResourceLoadIdentifier, Ref<NetworkResourceLoader>> m_networkResourceLoaders;
    HashMap<String, RefPtr<WebCore::BlobDataFileReference>> m_blobDataFileReferences;
    Vector<ResourceNetworkActivityTracker> m_networkActivityTrackers;

    HashMap<ResourceLoadIdentifier, WebCore::NetworkLoadInformation> m_networkLoadInformationByID;


#if USE(LIBWEBRTC)
    RefPtr<NetworkRTCProvider> m_rtcProvider;
#endif
#if ENABLE(WEB_RTC)
    NetworkMDNSRegister m_mdnsRegister;
#endif

    bool m_captureExtraNetworkLoadMetricsEnabled { false };

    RefPtr<CacheStorageEngineConnection> m_cacheStorageConnection;
};

} // namespace WebKit
