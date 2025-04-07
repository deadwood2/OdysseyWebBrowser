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

#include "config.h"
#include "NetworkProcessProxy.h"

#include "APIContentRuleList.h"
#include "AuthenticationChallengeProxy.h"
#include "DownloadProxyMessages.h"
#if ENABLE(LEGACY_CUSTOM_PROTOCOL_MANAGER)
#include "LegacyCustomProtocolManagerProxyMessages.h"
#endif
#include "Logging.h"
#include "NetworkContentRuleListManagerMessages.h"
#include "NetworkProcessCreationParameters.h"
#include "NetworkProcessMessages.h"
#include "SandboxExtension.h"
#if HAVE(SEC_KEY_PROXY)
#include "SecKeyProxyStore.h"
#endif
#include "ShouldGrandfatherStatistics.h"
#include "StorageAccessStatus.h"
#include "WebCompiledContentRuleList.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebProcessMessages.h"
#include "WebProcessPool.h"
#include "WebProcessProxy.h"
#include "WebResourceLoadStatisticsStore.h"
#include "WebUserContentControllerProxy.h"
#include "WebsiteData.h"
#include "WebsiteDataStoreClient.h"
#include <WebCore/ClientOrigin.h>
#include <WebCore/RegistrableDomain.h>
#include <wtf/CompletionHandler.h>

#if ENABLE(SEC_ITEM_SHIM)
#include "SecItemShimProxy.h"
#endif

#if PLATFORM(IOS_FAMILY)
#include <wtf/spi/darwin/XPCSPI.h>
#endif

#define MESSAGE_CHECK(assertion) MESSAGE_CHECK_BASE(assertion, connection())

namespace WebKit {
using namespace WebCore;

static uint64_t generateCallbackID()
{
    static uint64_t callbackID;

    return ++callbackID;
}

NetworkProcessProxy::NetworkProcessProxy(WebProcessPool& processPool)
    : AuxiliaryProcessProxy(processPool.alwaysRunsAtBackgroundPriority())
    , m_processPool(processPool)
    , m_numPendingConnectionRequests(0)
#if ENABLE(LEGACY_CUSTOM_PROTOCOL_MANAGER)
    , m_customProtocolManagerProxy(*this)
#endif
    , m_throttler(*this, processPool.shouldTakeUIBackgroundAssertion())
{
    connect();

    if (auto* websiteDataStore = m_processPool.websiteDataStore())
        m_websiteDataStores.set(websiteDataStore->websiteDataStore().sessionID(), makeRef(websiteDataStore->websiteDataStore()));
}

NetworkProcessProxy::~NetworkProcessProxy()
{
    ASSERT(m_pendingFetchWebsiteDataCallbacks.isEmpty());
    ASSERT(m_pendingDeleteWebsiteDataCallbacks.isEmpty());
    ASSERT(m_pendingDeleteWebsiteDataForOriginsCallbacks.isEmpty());
#if ENABLE(CONTENT_EXTENSIONS)
    for (auto* proxy : m_webUserContentControllerProxies)
        proxy->removeNetworkProcess(*this);
#endif

    if (m_downloadProxyMap)
        m_downloadProxyMap->invalidate();

    for (auto& reply : m_pendingConnectionReplies)
        reply.second({ });
}

void NetworkProcessProxy::getLaunchOptions(ProcessLauncher::LaunchOptions& launchOptions)
{
    launchOptions.processType = ProcessLauncher::ProcessType::Network;
    AuxiliaryProcessProxy::getLaunchOptions(launchOptions);

    if (processPool().shouldMakeNextNetworkProcessLaunchFailForTesting()) {
        processPool().setShouldMakeNextNetworkProcessLaunchFailForTesting(false);
        launchOptions.shouldMakeProcessLaunchFailForTesting = true;
    }
}

void NetworkProcessProxy::connectionWillOpen(IPC::Connection& connection)
{
#if ENABLE(SEC_ITEM_SHIM)
    SecItemShimProxy::singleton().initializeConnection(connection);
#else
    UNUSED_PARAM(connection);
#endif
}

void NetworkProcessProxy::processWillShutDown(IPC::Connection& connection)
{
    ASSERT_UNUSED(connection, this->connection() == &connection);
}

void NetworkProcessProxy::getNetworkProcessConnection(WebProcessProxy& webProcessProxy, Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply&& reply)
{
    m_pendingConnectionReplies.append(std::make_pair(makeWeakPtr(webProcessProxy), WTFMove(reply)));

    if (state() == State::Launching) {
        m_numPendingConnectionRequests++;
        return;
    }

    bool isServiceWorkerProcess = false;
    RegistrableDomain registrableDomain;
#if ENABLE(SERVICE_WORKER)
    if (is<ServiceWorkerProcessProxy>(webProcessProxy)) {
        isServiceWorkerProcess = true;
        registrableDomain = downcast<ServiceWorkerProcessProxy>(webProcessProxy).registrableDomain();
    }
#endif

    connection()->send(Messages::NetworkProcess::CreateNetworkConnectionToWebProcess(isServiceWorkerProcess, registrableDomain), 0, IPC::SendOption::DispatchMessageEvenWhenWaitingForSyncReply);
}

void NetworkProcessProxy::synthesizeAppIsBackground(bool background)
{
    if (m_downloadProxyMap) {
        if (background)
            m_downloadProxyMap->applicationDidEnterBackground();
        else
            m_downloadProxyMap->applicationWillEnterForeground();
    }
}

DownloadProxy& NetworkProcessProxy::createDownloadProxy(const ResourceRequest& resourceRequest)
{
    if (!m_downloadProxyMap)
        m_downloadProxyMap = makeUnique<DownloadProxyMap>(*this);

    return m_downloadProxyMap->createDownloadProxy(m_processPool, resourceRequest);
}

void NetworkProcessProxy::fetchWebsiteData(PAL::SessionID sessionID, OptionSet<WebsiteDataType> dataTypes, OptionSet<WebsiteDataFetchOption> fetchOptions, CompletionHandler<void (WebsiteData)>&& completionHandler)
{
    ASSERT(canSendMessage());

    uint64_t callbackID = generateCallbackID();
    RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is taking a background assertion because the Network process is fetching Website data", this);

    m_pendingFetchWebsiteDataCallbacks.add(callbackID, [this, token = throttler().backgroundActivityToken(), completionHandler = WTFMove(completionHandler), sessionID] (WebsiteData websiteData) mutable {
#if RELEASE_LOG_DISABLED
        UNUSED_PARAM(this);
        UNUSED_PARAM(sessionID);
#endif
        completionHandler(WTFMove(websiteData));
        RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is releasing a background assertion because the Network process is done fetching Website data", this);
    });

    send(Messages::NetworkProcess::FetchWebsiteData(sessionID, dataTypes, fetchOptions, callbackID), 0);
}

void NetworkProcessProxy::deleteWebsiteData(PAL::SessionID sessionID, OptionSet<WebsiteDataType> dataTypes, WallTime modifiedSince, CompletionHandler<void ()>&& completionHandler)
{
    auto callbackID = generateCallbackID();
    RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is taking a background assertion because the Network process is deleting Website data", this);

    m_pendingDeleteWebsiteDataCallbacks.add(callbackID, [this, token = throttler().backgroundActivityToken(), completionHandler = WTFMove(completionHandler), sessionID] () mutable {
#if RELEASE_LOG_DISABLED
        UNUSED_PARAM(this);
        UNUSED_PARAM(sessionID);
#endif
        completionHandler();
        RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is releasing a background assertion because the Network process is done deleting Website data", this);
    });
    send(Messages::NetworkProcess::DeleteWebsiteData(sessionID, dataTypes, modifiedSince, callbackID), 0);
}

void NetworkProcessProxy::deleteWebsiteDataForOrigins(PAL::SessionID sessionID, OptionSet<WebsiteDataType> dataTypes, const Vector<WebCore::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, const Vector<String>& HSTSCacheHostNames, CompletionHandler<void()>&& completionHandler)
{
    ASSERT(canSendMessage());

    uint64_t callbackID = generateCallbackID();
    RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is taking a background assertion because the Network process is deleting Website data for several origins", this);

    m_pendingDeleteWebsiteDataForOriginsCallbacks.add(callbackID, [this, token = throttler().backgroundActivityToken(), completionHandler = WTFMove(completionHandler), sessionID] () mutable {
#if RELEASE_LOG_DISABLED
        UNUSED_PARAM(this);
        UNUSED_PARAM(sessionID);
#endif
        completionHandler();
        RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), ProcessSuspension, "%p - NetworkProcessProxy is releasing a background assertion because the Network process is done deleting Website data for several origins", this);
    });

    send(Messages::NetworkProcess::DeleteWebsiteDataForOrigins(sessionID, dataTypes, origins, cookieHostNames, HSTSCacheHostNames, callbackID), 0);
}

void NetworkProcessProxy::networkProcessCrashed()
{
    clearCallbackStates();

    Vector<std::pair<RefPtr<WebProcessProxy>, Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>> pendingReplies;
    pendingReplies.reserveInitialCapacity(m_pendingConnectionReplies.size());
    for (auto& reply : m_pendingConnectionReplies) {
        if (reply.first)
            pendingReplies.append(std::make_pair(makeRefPtr(reply.first.get()), WTFMove(reply.second)));
        else
            reply.second({ });
    }
    m_pendingConnectionReplies.clear();

    // Tell the network process manager to forget about this network process proxy. This will cause us to be deleted.
    m_processPool.networkProcessCrashed(*this, WTFMove(pendingReplies));
}

void NetworkProcessProxy::clearCallbackStates()
{
    while (!m_pendingFetchWebsiteDataCallbacks.isEmpty())
        m_pendingFetchWebsiteDataCallbacks.take(m_pendingFetchWebsiteDataCallbacks.begin()->key)(WebsiteData { });

    while (!m_pendingDeleteWebsiteDataCallbacks.isEmpty())
        m_pendingDeleteWebsiteDataCallbacks.take(m_pendingDeleteWebsiteDataCallbacks.begin()->key)();

    while (!m_pendingDeleteWebsiteDataForOriginsCallbacks.isEmpty())
        m_pendingDeleteWebsiteDataForOriginsCallbacks.take(m_pendingDeleteWebsiteDataForOriginsCallbacks.begin()->key)();
}

void NetworkProcessProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (dispatchMessage(connection, decoder))
        return;

    if (m_processPool.dispatchMessage(connection, decoder))
        return;

    didReceiveNetworkProcessProxyMessage(connection, decoder);
}

void NetworkProcessProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (dispatchSyncMessage(connection, decoder, replyEncoder))
        return;

    didReceiveSyncNetworkProcessProxyMessage(connection, decoder, replyEncoder);
}

void NetworkProcessProxy::didClose(IPC::Connection&)
{
    auto protectedProcessPool = makeRef(m_processPool);

    if (m_downloadProxyMap)
        m_downloadProxyMap->invalidate();
#if ENABLE(LEGACY_CUSTOM_PROTOCOL_MANAGER)
    m_customProtocolManagerProxy.invalidate();
#endif

    m_tokenForHoldingLockedFiles = nullptr;
    
    m_syncAllCookiesToken = nullptr;
    m_syncAllCookiesCounter = 0;

    // This will cause us to be deleted.
    networkProcessCrashed();
}

void NetworkProcessProxy::didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference, IPC::StringReference)
{
}

void NetworkProcessProxy::didCreateNetworkConnectionToWebProcess(const IPC::Attachment& connectionIdentifier)
{
    ASSERT(!m_pendingConnectionReplies.isEmpty());

    // Grab the first pending connection reply.
    auto reply = m_pendingConnectionReplies.takeFirst().second;

#if USE(UNIX_DOMAIN_SOCKETS) || OS(WINDOWS)
    reply(connectionIdentifier);
#elif OS(DARWIN)
    MESSAGE_CHECK(MACH_PORT_VALID(connectionIdentifier.port()));
    reply(IPC::Attachment(connectionIdentifier.port(), MACH_MSG_TYPE_MOVE_SEND));
#else
    notImplemented();
#endif
}

void NetworkProcessProxy::didReceiveAuthenticationChallenge(PAL::SessionID sessionID, PageIdentifier pageID, FrameIdentifier frameID, WebCore::AuthenticationChallenge&& coreChallenge, uint64_t challengeID)
{
#if HAVE(SEC_KEY_PROXY)
    WeakPtr<SecKeyProxyStore> secKeyProxyStore;
    if (coreChallenge.protectionSpace().authenticationScheme() == ProtectionSpaceAuthenticationSchemeClientCertificateRequested) {
        if (auto* store = websiteDataStoreFromSessionID(sessionID)) {
            auto newSecKeyProxyStore = SecKeyProxyStore::create();
            secKeyProxyStore = makeWeakPtr(newSecKeyProxyStore.get());
            store->addSecKeyProxyStore(WTFMove(newSecKeyProxyStore));
        }
    }
    auto authenticationChallenge = AuthenticationChallengeProxy::create(WTFMove(coreChallenge), challengeID, makeRef(*connection()), WTFMove(secKeyProxyStore));
#else
    auto authenticationChallenge = AuthenticationChallengeProxy::create(WTFMove(coreChallenge), challengeID, makeRef(*connection()), nullptr);
#endif

    WebPageProxy* page = nullptr;
    if (pageID && !m_processPool.isServiceWorkerPageID(pageID))
        page = WebProcessProxy::webPage(pageID);

    if (!page) {
        auto* store = websiteDataStoreFromSessionID(sessionID);
        if (!store || coreChallenge.protectionSpace().authenticationScheme() != ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested) {
            authenticationChallenge->listener().completeChallenge(AuthenticationChallengeDisposition::PerformDefaultHandling);
            return;
        }
        store->client().didReceiveAuthenticationChallenge(WTFMove(authenticationChallenge));
        return;
    }

    page->didReceiveAuthenticationChallengeProxy(frameID, WTFMove(authenticationChallenge));
}

void NetworkProcessProxy::didFetchWebsiteData(uint64_t callbackID, const WebsiteData& websiteData)
{
    auto callback = m_pendingFetchWebsiteDataCallbacks.take(callbackID);
    callback(websiteData);
}

void NetworkProcessProxy::didDeleteWebsiteData(uint64_t callbackID)
{
    auto callback = m_pendingDeleteWebsiteDataCallbacks.take(callbackID);
    callback();
}

void NetworkProcessProxy::didDeleteWebsiteDataForOrigins(uint64_t callbackID)
{
    auto callback = m_pendingDeleteWebsiteDataForOriginsCallbacks.take(callbackID);
    callback();
}

void NetworkProcessProxy::didFinishLaunching(ProcessLauncher* launcher, IPC::Connection::Identifier connectionIdentifier)
{
    AuxiliaryProcessProxy::didFinishLaunching(launcher, connectionIdentifier);

    if (!IPC::Connection::identifierIsValid(connectionIdentifier)) {
        networkProcessCrashed();
        return;
    }

    for (unsigned i = 0; i < m_numPendingConnectionRequests; ++i)
        connection()->send(Messages::NetworkProcess::CreateNetworkConnectionToWebProcess(false, { }), 0);
    
    m_numPendingConnectionRequests = 0;

#if PLATFORM(COCOA)
    if (m_processPool.processSuppressionEnabled())
        setProcessSuppressionEnabled(true);
#endif
    
#if PLATFORM(IOS_FAMILY)
    if (xpc_connection_t connection = this->connection()->xpcConnection())
        m_throttler.didConnectToProcess(xpc_connection_get_pid(connection));
#endif
}

void NetworkProcessProxy::logDiagnosticMessage(PageIdentifier pageID, const String& message, const String& description, WebCore::ShouldSample shouldSample)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    // FIXME: We do this null-check because by the time the decision to log is made, the page may be gone. We should refactor to avoid this,
    // but for now we simply drop the message in the rare case this happens.
    if (!page)
        return;

    page->logDiagnosticMessage(message, description, shouldSample);
}

void NetworkProcessProxy::logDiagnosticMessageWithResult(PageIdentifier pageID, const String& message, const String& description, uint32_t result, WebCore::ShouldSample shouldSample)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    // FIXME: We do this null-check because by the time the decision to log is made, the page may be gone. We should refactor to avoid this,
    // but for now we simply drop the message in the rare case this happens.
    if (!page)
        return;

    page->logDiagnosticMessageWithResult(message, description, result, shouldSample);
}

void NetworkProcessProxy::logDiagnosticMessageWithValue(PageIdentifier pageID, const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample shouldSample)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    // FIXME: We do this null-check because by the time the decision to log is made, the page may be gone. We should refactor to avoid this,
    // but for now we simply drop the message in the rare case this happens.
    if (!page)
        return;

    page->logDiagnosticMessageWithValue(message, description, value, significantFigures, shouldSample);
}

void NetworkProcessProxy::logGlobalDiagnosticMessageWithValue(const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample shouldSample)
{
    if (auto* page = WebPageProxy::nonEphemeralWebPageProxy())
        page->logDiagnosticMessageWithValue(message, description, value, significantFigures, shouldSample);
}

#if ENABLE(RESOURCE_LOAD_STATISTICS)
void NetworkProcessProxy::dumpResourceLoadStatistics(PAL::SessionID sessionID, CompletionHandler<void(String)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler({ });
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::DumpResourceLoadStatistics(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::updatePrevalentDomainsToBlockCookiesFor(PAL::SessionID sessionID, const Vector<RegistrableDomain>& domainsToBlock, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    Vector<RegistrableDomain> registrableDomainsToBlock;
    registrableDomainsToBlock.reserveInitialCapacity(domainsToBlock.size());
    for (auto& domain : domainsToBlock)
        registrableDomainsToBlock.uncheckedAppend(domain);

    sendWithAsyncReply(Messages::NetworkProcess::UpdatePrevalentDomainsToBlockCookiesFor(sessionID, registrableDomainsToBlock), WTFMove(completionHandler));
}

void NetworkProcessProxy::isPrevalentResource(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::IsPrevalentResource(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::isVeryPrevalentResource(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::IsVeryPrevalentResource(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setPrevalentResource(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::SetPrevalentResource(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setPrevalentResourceForDebugMode(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::SetPrevalentResourceForDebugMode(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setVeryPrevalentResource(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::SetVeryPrevalentResource(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setLastSeen(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, Seconds lastSeen, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetLastSeen(sessionID, resourceDomain, lastSeen), WTFMove(completionHandler));
}

void NetworkProcessProxy::clearPrevalentResource(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::ClearPrevalentResource(sessionID, resourceDomain), WTFMove(completionHandler));
}
    
void NetworkProcessProxy::scheduleCookieBlockingUpdate(PAL::SessionID sessionID, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::ScheduleCookieBlockingUpdate(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::scheduleClearInMemoryAndPersistent(PAL::SessionID sessionID, Optional<WallTime> modifiedSince, ShouldGrandfatherStatistics shouldGrandfather, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::ScheduleClearInMemoryAndPersistent(sessionID, modifiedSince, shouldGrandfather), WTFMove(completionHandler));
}

void NetworkProcessProxy::scheduleStatisticsAndDataRecordsProcessing(PAL::SessionID sessionID, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::ScheduleStatisticsAndDataRecordsProcessing(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::logUserInteraction(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::LogUserInteraction(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::hasHadUserInteraction(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::HadUserInteraction(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::clearUserInteraction(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::ClearUserInteraction(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::hasLocalStorage(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::HasLocalStorage(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setAgeCapForClientSideCookies(PAL::SessionID sessionID, Optional<Seconds> seconds, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetAgeCapForClientSideCookies(sessionID, seconds), WTFMove(completionHandler));
}

void NetworkProcessProxy::setTimeToLiveUserInteraction(PAL::SessionID sessionID, Seconds seconds, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetTimeToLiveUserInteraction(sessionID, seconds), WTFMove(completionHandler));
}

void NetworkProcessProxy::setNotifyPagesWhenTelemetryWasCaptured(PAL::SessionID sessionID, bool value, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetNotifyPagesWhenTelemetryWasCaptured(sessionID, value), WTFMove(completionHandler));
}

void NetworkProcessProxy::setNotifyPagesWhenDataRecordsWereScanned(PAL::SessionID sessionID, bool value, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetNotifyPagesWhenDataRecordsWereScanned(sessionID, value), WTFMove(completionHandler));
}

void NetworkProcessProxy::setIsRunningResourceLoadStatisticsTest(PAL::SessionID sessionID, bool value, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetIsRunningResourceLoadStatisticsTest(sessionID, value), WTFMove(completionHandler));
}

void NetworkProcessProxy::setSubframeUnderTopFrameDomain(PAL::SessionID sessionID, const RegistrableDomain& subFrameDomain, const RegistrableDomain& topFrameDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetSubframeUnderTopFrameDomain(sessionID, subFrameDomain, topFrameDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::isRegisteredAsRedirectingTo(PAL::SessionID sessionID, const RegistrableDomain& domainRedirectedFrom, const RegistrableDomain& domainRedirectedTo, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::IsRegisteredAsRedirectingTo(sessionID, domainRedirectedFrom, domainRedirectedTo), WTFMove(completionHandler));
}

void NetworkProcessProxy::isRegisteredAsSubFrameUnder(PAL::SessionID sessionID, const RegistrableDomain& subFrameDomain, const RegistrableDomain& topFrameDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::IsRegisteredAsSubFrameUnder(sessionID, subFrameDomain, topFrameDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setSubresourceUnderTopFrameDomain(PAL::SessionID sessionID, const RegistrableDomain& subresourceDomain, const RegistrableDomain& topFrameDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetSubresourceUnderTopFrameDomain(sessionID, subresourceDomain, topFrameDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::isRegisteredAsSubresourceUnder(PAL::SessionID sessionID, const RegistrableDomain& subresourceDomain, const RegistrableDomain& topFrameDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::IsRegisteredAsSubresourceUnder(sessionID, subresourceDomain, topFrameDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setSubresourceUniqueRedirectTo(PAL::SessionID sessionID, const RegistrableDomain& subresourceDomain, const RegistrableDomain& domainRedirectedTo, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetSubresourceUniqueRedirectTo(sessionID, subresourceDomain, domainRedirectedTo), WTFMove(completionHandler));
}

void NetworkProcessProxy::setSubresourceUniqueRedirectFrom(PAL::SessionID sessionID, const RegistrableDomain& subresourceDomain, const RegistrableDomain& domainRedirectedFrom, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetSubresourceUniqueRedirectFrom(sessionID, subresourceDomain, domainRedirectedFrom), WTFMove(completionHandler));
}

void NetworkProcessProxy::setTopFrameUniqueRedirectTo(PAL::SessionID sessionID, const RegistrableDomain& topFrameDomain, const RegistrableDomain& domainRedirectedTo, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetTopFrameUniqueRedirectTo(sessionID, topFrameDomain, domainRedirectedTo), WTFMove(completionHandler));
}

void NetworkProcessProxy::setTopFrameUniqueRedirectFrom(PAL::SessionID sessionID, const RegistrableDomain& topFrameDomain, const RegistrableDomain& domainRedirectedFrom, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetTopFrameUniqueRedirectFrom(sessionID, topFrameDomain, domainRedirectedFrom), WTFMove(completionHandler));
}

void NetworkProcessProxy::isGrandfathered(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::IsGrandfathered(sessionID, resourceDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::setGrandfathered(PAL::SessionID sessionID, const RegistrableDomain& resourceDomain, bool isGrandfathered, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetGrandfathered(sessionID, resourceDomain, isGrandfathered), WTFMove(completionHandler));
}

void NetworkProcessProxy::requestStorageAccessConfirm(PageIdentifier pageID, FrameIdentifier frameID, const RegistrableDomain& subFrameDomain, const RegistrableDomain& topFrameDomain, CompletionHandler<void(bool)>&& completionHandler)
{
    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    if (!page) {
        completionHandler(false);
        return;
    }
    
    page->requestStorageAccessConfirm(subFrameDomain, topFrameDomain, frameID, WTFMove(completionHandler));
}

void NetworkProcessProxy::getAllStorageAccessEntries(PAL::SessionID sessionID, CompletionHandler<void(Vector<String> domains)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler({ });
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::GetAllStorageAccessEntries(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::setCacheMaxAgeCapForPrevalentResources(PAL::SessionID sessionID, Seconds seconds, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::SetCacheMaxAgeCapForPrevalentResources(sessionID, seconds), WTFMove(completionHandler));
}

void NetworkProcessProxy::setCacheMaxAgeCap(PAL::SessionID sessionID, Seconds seconds, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetCacheMaxAgeCapForPrevalentResources(sessionID, seconds), WTFMove(completionHandler));
}

void NetworkProcessProxy::setGrandfatheringTime(PAL::SessionID sessionID, Seconds seconds, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetGrandfatheringTime(sessionID, seconds), WTFMove(completionHandler));
}

void NetworkProcessProxy::setMaxStatisticsEntries(PAL::SessionID sessionID, size_t maximumEntryCount, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::SetMaxStatisticsEntries(sessionID, maximumEntryCount), WTFMove(completionHandler));
}

void NetworkProcessProxy::setMinimumTimeBetweenDataRecordsRemoval(PAL::SessionID sessionID, Seconds seconds, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetMinimumTimeBetweenDataRecordsRemoval(sessionID, seconds), WTFMove(completionHandler));
}

void NetworkProcessProxy::setPruneEntriesDownTo(PAL::SessionID sessionID, size_t pruneTargetCount, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::SetPruneEntriesDownTo(sessionID, pruneTargetCount), WTFMove(completionHandler));
}

void NetworkProcessProxy::setShouldClassifyResourcesBeforeDataRecordsRemoval(PAL::SessionID sessionID, bool value, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetShouldClassifyResourcesBeforeDataRecordsRemoval(sessionID, value), WTFMove(completionHandler));
}

void NetworkProcessProxy::setResourceLoadStatisticsDebugMode(PAL::SessionID sessionID, bool debugMode, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetResourceLoadStatisticsDebugMode(sessionID, debugMode), WTFMove(completionHandler));
}

void NetworkProcessProxy::resetCacheMaxAgeCapForPrevalentResources(PAL::SessionID sessionID, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::ResetCacheMaxAgeCapForPrevalentResources(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::resetParametersToDefaultValues(PAL::SessionID sessionID, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::ResetParametersToDefaultValues(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::submitTelemetry(PAL::SessionID sessionID, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SubmitTelemetry(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::scheduleClearInMemoryAndPersistent(PAL::SessionID sessionID, ShouldGrandfatherStatistics shouldGrandfather, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::ScheduleClearInMemoryAndPersistent(sessionID, { }, shouldGrandfather), WTFMove(completionHandler));
}

void NetworkProcessProxy::logTestingEvent(PAL::SessionID sessionID, const String& event)
{
    if (auto* websiteDataStore = websiteDataStoreFromSessionID(sessionID))
        websiteDataStore->logTestingEvent(event);
}

void NetworkProcessProxy::notifyResourceLoadStatisticsProcessed()
{
    WebProcessProxy::notifyPageStatisticsAndDataRecordsProcessed();
}

void NetworkProcessProxy::notifyWebsiteDataDeletionForRegistrableDomainsFinished()
{
    WebProcessProxy::notifyWebsiteDataDeletionForRegistrableDomainsFinished();
}

void NetworkProcessProxy::notifyWebsiteDataScanForRegistrableDomainsFinished()
{
    WebProcessProxy::notifyWebsiteDataScanForRegistrableDomainsFinished();
}

void NetworkProcessProxy::notifyResourceLoadStatisticsTelemetryFinished(unsigned totalPrevalentResources, unsigned totalPrevalentResourcesWithUserInteraction, unsigned top3SubframeUnderTopFrameOrigins)
{
    API::Dictionary::MapType messageBody;
    messageBody.set("TotalPrevalentResources"_s, API::UInt64::create(totalPrevalentResources));
    messageBody.set("TotalPrevalentResourcesWithUserInteraction"_s, API::UInt64::create(totalPrevalentResourcesWithUserInteraction));
    messageBody.set("Top3SubframeUnderTopFrameOrigins"_s, API::UInt64::create(top3SubframeUnderTopFrameOrigins));

    WebProcessProxy::notifyPageStatisticsTelemetryFinished(API::Dictionary::create(messageBody).ptr());
}

void NetworkProcessProxy::didCommitCrossSiteLoadWithDataTransfer(PAL::SessionID sessionID, const RegistrableDomain& fromDomain, const RegistrableDomain& toDomain, OptionSet<WebCore::CrossSiteNavigationDataTransfer::Flag> navigationDataTransfer, PageIdentifier pageID)
{
    if (!canSendMessage())
        return;

    send(Messages::NetworkProcess::DidCommitCrossSiteLoadWithDataTransfer(sessionID, fromDomain, toDomain, navigationDataTransfer, pageID), 0);
}

void NetworkProcessProxy::didCommitCrossSiteLoadWithDataTransferFromPrevalentResource(PageIdentifier pageID)
{
    if (!canSendMessage())
        return;

    WebPageProxy* page = WebProcessProxy::webPage(pageID);
    if (!page)
        return;

    page->didCommitCrossSiteLoadWithDataTransferFromPrevalentResource();
}

void NetworkProcessProxy::setCrossSiteLoadWithLinkDecorationForTesting(PAL::SessionID sessionID, const RegistrableDomain& fromDomain, const RegistrableDomain& toDomain, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::SetCrossSiteLoadWithLinkDecorationForTesting(sessionID, fromDomain, toDomain), WTFMove(completionHandler));
}

void NetworkProcessProxy::resetCrossSiteLoadsWithLinkDecorationForTesting(PAL::SessionID sessionID, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::ResetCrossSiteLoadsWithLinkDecorationForTesting(sessionID), WTFMove(completionHandler));
}

void NetworkProcessProxy::deleteCookiesForTesting(PAL::SessionID sessionID, const RegistrableDomain& domain, bool includeHttpOnlyCookies, CompletionHandler<void()>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler();
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::DeleteCookiesForTesting(sessionID, domain, includeHttpOnlyCookies), WTFMove(completionHandler));
}

void NetworkProcessProxy::deleteWebsiteDataInUIProcessForRegistrableDomains(PAL::SessionID sessionID, OptionSet<WebsiteDataType> dataTypes, OptionSet<WebsiteDataFetchOption> fetchOptions, Vector<RegistrableDomain> domains, CompletionHandler<void(HashSet<WebCore::RegistrableDomain>&&)>&& completionHandler)
{
    auto* websiteDataStore = websiteDataStoreFromSessionID(sessionID);
    if (!websiteDataStore || dataTypes.isEmpty() || domains.isEmpty()) {
        completionHandler({ });
        return;
    }

    websiteDataStore->fetchDataForRegistrableDomains(dataTypes, fetchOptions, domains, [dataTypes, websiteDataStore = makeRef(*websiteDataStore), completionHandler = WTFMove(completionHandler)] (Vector<WebsiteDataRecord>&& matchingDataRecords, HashSet<WebCore::RegistrableDomain>&& domainsWithMatchingDataRecords) mutable {
        websiteDataStore->removeData(dataTypes, WTFMove(matchingDataRecords), [domainsWithMatchingDataRecords = WTFMove(domainsWithMatchingDataRecords), completionHandler = WTFMove(completionHandler)] () mutable {
            completionHandler(WTFMove(domainsWithMatchingDataRecords));
        });
    });
}

void NetworkProcessProxy::hasIsolatedSession(PAL::SessionID sessionID, const RegistrableDomain& domain, CompletionHandler<void(bool)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler(false);
        return;
    }
    
    sendWithAsyncReply(Messages::NetworkProcess::HasIsolatedSession(sessionID, domain), WTFMove(completionHandler));
}

#endif // ENABLE(RESOURCE_LOAD_STATISTICS)

void NetworkProcessProxy::sendProcessWillSuspendImminently()
{
    if (canSendMessage())
        send(Messages::NetworkProcess::ProcessWillSuspendImminently(), 0);
}

void NetworkProcessProxy::sendProcessWillSuspendImminentlyForTesting()
{
    if (canSendMessage())
        sendSync(Messages::NetworkProcess::ProcessWillSuspendImminentlyForTestingSync(), Messages::NetworkProcess::ProcessWillSuspendImminentlyForTestingSync::Reply(), 0);
}
    
void NetworkProcessProxy::sendPrepareToSuspend()
{
    if (canSendMessage())
        send(Messages::NetworkProcess::PrepareToSuspend(), 0);
}

void NetworkProcessProxy::sendCancelPrepareToSuspend()
{
    if (canSendMessage())
        send(Messages::NetworkProcess::CancelPrepareToSuspend(), 0);
}

void NetworkProcessProxy::sendProcessDidResume()
{
    if (canSendMessage())
        send(Messages::NetworkProcess::ProcessDidResume(), 0);
}

void NetworkProcessProxy::processReadyToSuspend()
{
    m_throttler.processReadyToSuspend();
}

void NetworkProcessProxy::didSetAssertionState(AssertionState)
{
}
    
void NetworkProcessProxy::setIsHoldingLockedFiles(bool isHoldingLockedFiles)
{
    if (!isHoldingLockedFiles) {
        RELEASE_LOG(ProcessSuspension, "UIProcess is releasing a background assertion because the Network process is no longer holding locked files");
        m_tokenForHoldingLockedFiles = nullptr;
        return;
    }
    if (!m_tokenForHoldingLockedFiles) {
        RELEASE_LOG(ProcessSuspension, "UIProcess is taking a background assertion because the Network process is holding locked files");
        m_tokenForHoldingLockedFiles = m_throttler.backgroundActivityToken();
    }
}

void NetworkProcessProxy::syncAllCookies()
{
    send(Messages::NetworkProcess::SyncAllCookies(), 0);
    
    ++m_syncAllCookiesCounter;
    if (m_syncAllCookiesToken)
        return;
    
    RELEASE_LOG(ProcessSuspension, "%p - NetworkProcessProxy is taking a background assertion because the Network process is syncing cookies", this);
    m_syncAllCookiesToken = throttler().backgroundActivityToken();
}
    
void NetworkProcessProxy::didSyncAllCookies()
{
    ASSERT(m_syncAllCookiesCounter);

    --m_syncAllCookiesCounter;
    if (!m_syncAllCookiesCounter) {
        RELEASE_LOG(ProcessSuspension, "%p - NetworkProcessProxy is releasing a background assertion because the Network process is done syncing cookies", this);
        m_syncAllCookiesToken = nullptr;
    }
}

void NetworkProcessProxy::addSession(Ref<WebsiteDataStore>&& store)
{
    if (canSendMessage())
        send(Messages::NetworkProcess::AddWebsiteDataStore { store->parameters() }, 0);
    auto sessionID = store->sessionID();
    if (!sessionID.isEphemeral()) {
#if ENABLE(INDEXED_DATABASE)
        createSymLinkForFileUpgrade(store->resolvedIndexedDatabaseDirectory());
#endif
        m_websiteDataStores.set(sessionID, WTFMove(store));
    }
}

void NetworkProcessProxy::removeSession(PAL::SessionID sessionID)
{
    if (canSendMessage())
        send(Messages::NetworkProcess::DestroySession { sessionID }, 0);
    if (!sessionID.isEphemeral())
        m_websiteDataStores.remove(sessionID);
}

WebsiteDataStore* NetworkProcessProxy::websiteDataStoreFromSessionID(PAL::SessionID sessionID)
{
    auto iterator = m_websiteDataStores.find(sessionID);
    if (iterator != m_websiteDataStores.end())
        return iterator->value.get();

    if (auto* websiteDataStore = m_processPool.websiteDataStore()) {
        if (sessionID == websiteDataStore->websiteDataStore().sessionID())
            return &websiteDataStore->websiteDataStore();
    }

    if (sessionID != PAL::SessionID::defaultSessionID())
        return nullptr;

    return &API::WebsiteDataStore::defaultDataStore()->websiteDataStore();
}

void NetworkProcessProxy::retrieveCacheStorageParameters(PAL::SessionID sessionID)
{
    auto* store = websiteDataStoreFromSessionID(sessionID);

    if (!store) {
        RELEASE_LOG_ERROR(CacheStorage, "%p - NetworkProcessProxy is unable to retrieve CacheStorage parameters from the given session ID %" PRIu64, this, sessionID.toUInt64());
        send(Messages::NetworkProcess::SetCacheStorageParameters { sessionID, { }, { } }, 0);
        return;
    }

    auto& cacheStorageDirectory = store->cacheStorageDirectory();
    SandboxExtension::Handle cacheStorageDirectoryExtensionHandle;
    if (!cacheStorageDirectory.isEmpty())
        SandboxExtension::createHandleForReadWriteDirectory(cacheStorageDirectory, cacheStorageDirectoryExtensionHandle);

    send(Messages::NetworkProcess::SetCacheStorageParameters { sessionID, cacheStorageDirectory, cacheStorageDirectoryExtensionHandle }, 0);
}

#if ENABLE(CONTENT_EXTENSIONS)
void NetworkProcessProxy::contentExtensionRules(UserContentControllerIdentifier identifier)
{
    if (auto* webUserContentControllerProxy = WebUserContentControllerProxy::get(identifier)) {
        m_webUserContentControllerProxies.add(webUserContentControllerProxy);
        webUserContentControllerProxy->addNetworkProcess(*this);

        auto rules = WTF::map(webUserContentControllerProxy->contentExtensionRules(), [](auto&& keyValue) -> std::pair<String, WebCompiledContentRuleListData> {
            return std::make_pair(keyValue.value->name(), keyValue.value->compiledRuleList().data());
        });
        send(Messages::NetworkContentRuleListManager::AddContentRuleLists { identifier, rules }, 0);
        return;
    }
    send(Messages::NetworkContentRuleListManager::AddContentRuleLists { identifier, { } }, 0);
}

void NetworkProcessProxy::didDestroyWebUserContentControllerProxy(WebUserContentControllerProxy& proxy)
{
    send(Messages::NetworkContentRuleListManager::Remove { proxy.identifier() }, 0);
    m_webUserContentControllerProxies.remove(&proxy);
}
#endif

void NetworkProcessProxy::sendProcessDidTransitionToForeground()
{
    send(Messages::NetworkProcess::ProcessDidTransitionToForeground(), 0);
}

void NetworkProcessProxy::sendProcessDidTransitionToBackground()
{
    send(Messages::NetworkProcess::ProcessDidTransitionToBackground(), 0);
}

#if ENABLE(SANDBOX_EXTENSIONS)
void NetworkProcessProxy::getSandboxExtensionsForBlobFiles(const Vector<String>& paths, Messages::NetworkProcessProxy::GetSandboxExtensionsForBlobFiles::AsyncReply&& reply)
{
    SandboxExtension::HandleArray extensions;
    extensions.allocate(paths.size());
    for (size_t i = 0; i < paths.size(); ++i) {
        // ReadWrite is required for creating hard links, which is something that might be done with these extensions.
        SandboxExtension::createHandle(paths[i], SandboxExtension::Type::ReadWrite, extensions[i]);
    }
    reply(WTFMove(extensions));
}
#endif

#if ENABLE(SERVICE_WORKER)
void NetworkProcessProxy::establishWorkerContextConnectionToNetworkProcess(RegistrableDomain&& registrableDomain)
{
    m_processPool.establishWorkerContextConnectionToNetworkProcess(*this, WTFMove(registrableDomain), WTF::nullopt);
}

void NetworkProcessProxy::establishWorkerContextConnectionToNetworkProcessForExplicitSession(RegistrableDomain&& registrableDomain, PAL::SessionID sessionID)
{
    m_processPool.establishWorkerContextConnectionToNetworkProcess(*this, WTFMove(registrableDomain), sessionID);
}
#endif

void NetworkProcessProxy::requestStorageSpace(PAL::SessionID sessionID, const WebCore::ClientOrigin& origin, uint64_t currentQuota, uint64_t currentSize, uint64_t spaceRequired, CompletionHandler<void(Optional<uint64_t> quota)>&& completionHandler)
{
    auto* store = websiteDataStoreFromSessionID(sessionID);

    if (!store) {
        completionHandler({ });
        return;
    }

    store->client().requestStorageSpace(origin.topOrigin, origin.clientOrigin, currentQuota, currentSize, spaceRequired, [sessionID, origin, currentQuota, currentSize, spaceRequired, completionHandler = WTFMove(completionHandler)](auto quota) mutable {
        if (quota) {
            completionHandler(quota);
            return;
        }

        if (origin.topOrigin != origin.clientOrigin) {
            completionHandler({ });
            return;
        }

        WebPageProxy::forMostVisibleWebPageIfAny(sessionID, origin.topOrigin, [completionHandler = WTFMove(completionHandler), origin, currentQuota, currentSize, spaceRequired](auto* page) mutable {
            if (!page) {
                completionHandler({ });
                return;
            }
            String name = makeString(FileSystem::encodeForFileName(origin.topOrigin.host), " content");
            page->requestStorageSpace(page->mainFrame()->frameID(), origin.topOrigin.databaseIdentifier(), name, name, currentQuota, currentSize, currentSize, spaceRequired, [completionHandler = WTFMove(completionHandler)](auto quota) mutable {
                completionHandler(quota);
            });
        });
    });
}

void NetworkProcessProxy::takeUploadAssertion()
{
    ASSERT(!m_uploadAssertion);
    m_uploadAssertion = makeUnique<ProcessAssertion>(processIdentifier(), "WebKit uploads"_s, AssertionState::UnboundedNetworking);
}

void NetworkProcessProxy::clearUploadAssertion()
{
    ASSERT(m_uploadAssertion);
    m_uploadAssertion = nullptr;
}

void NetworkProcessProxy::testProcessIncomingSyncMessagesWhenWaitingForSyncReply(WebCore::PageIdentifier webPageID, Messages::NetworkProcessProxy::TestProcessIncomingSyncMessagesWhenWaitingForSyncReply::DelayedReply&& reply)
{
    auto* page = WebProcessProxy::webPage(webPageID);
    if (!page)
        return reply(false);

    bool handled = false;
    if (!page->sendSync(Messages::WebPage::TestProcessIncomingSyncMessagesWhenWaitingForSyncReply(), Messages::WebPage::TestProcessIncomingSyncMessagesWhenWaitingForSyncReply::Reply(handled), Seconds::infinity(), IPC::SendSyncOption::ForceDispatchWhenDestinationIsWaitingForUnboundedSyncReply))
        return reply(false);
    reply(handled);
}

#if ENABLE(INDEXED_DATABASE)
void NetworkProcessProxy::createSymLinkForFileUpgrade(const String& indexedDatabaseDirectory)
{
    if (indexedDatabaseDirectory.isEmpty())
        return;

    String oldVersionDirectory = FileSystem::pathByAppendingComponent(indexedDatabaseDirectory, "v0");
    FileSystem::deleteEmptyDirectory(oldVersionDirectory);
    if (!FileSystem::fileExists(oldVersionDirectory))
        FileSystem::createSymbolicLink(indexedDatabaseDirectory, oldVersionDirectory);
}
#endif

void NetworkProcessProxy::getLocalStorageDetails(PAL::SessionID sessionID, CompletionHandler<void(Vector<LocalStorageDatabaseTracker::OriginDetails>&&)>&& completionHandler)
{
    if (!canSendMessage()) {
        completionHandler({ });
        return;
    }

    sendWithAsyncReply(Messages::NetworkProcess::GetLocalStorageOriginDetails(sessionID), WTFMove(completionHandler));
}

} // namespace WebKit

#undef MESSAGE_CHECK
#undef MESSAGE_CHECK_URL
