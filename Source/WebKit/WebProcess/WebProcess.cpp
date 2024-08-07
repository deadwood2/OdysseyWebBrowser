/*
 * Copyright (C) 2009-2017 Apple Inc. All rights reserved.
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
#include "WebProcess.h"

#include "APIFrameHandle.h"
#include "APIPageGroupHandle.h"
#include "APIPageHandle.h"
#include "AuthenticationManager.h"
#include "ChildProcessMessages.h"
#include "DrawingArea.h"
#include "EventDispatcher.h"
#include "InjectedBundle.h"
#include "LibWebRTCNetwork.h"
#include "Logging.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkProcessConnection.h"
#include "NetworkSession.h"
#include "NetworkSessionCreationParameters.h"
#include "PluginProcessConnectionManager.h"
#include "SessionTracker.h"
#include "StatisticsData.h"
#include "StorageProcessMessages.h"
#include "UserData.h"
#include "WebAutomationSessionProxy.h"
#include "WebCacheStorageProvider.h"
#include "WebConnectionToUIProcess.h"
#include "WebCookieManager.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebFrameNetworkingContext.h"
#include "WebGamepadProvider.h"
#include "WebGeolocationManager.h"
#include "WebLoaderStrategy.h"
#include "WebMediaKeyStorageManager.h"
#include "WebMemorySampler.h"
#include "WebMessagePortChannelProvider.h"
#include "WebPage.h"
#include "WebPageGroupProxy.h"
#include "WebPlatformStrategies.h"
#include "WebPluginInfoProvider.h"
#include "WebProcessCreationParameters.h"
#include "WebProcessMessages.h"
#include "WebProcessPoolMessages.h"
#include "WebProcessProxyMessages.h"
#include "WebResourceLoadStatisticsStoreMessages.h"
#include "WebSWContextManagerConnection.h"
#include "WebSWContextManagerConnectionMessages.h"
#include "WebServiceWorkerProvider.h"
#include "WebSocketStream.h"
#include "WebToStorageProcessConnection.h"
#include "WebsiteData.h"
#include "WebsiteDataStoreParameters.h"
#include "WebsiteDataType.h"
#include <JavaScriptCore/JSLock.h>
#include <JavaScriptCore/MemoryStatistics.h>
#include <JavaScriptCore/WasmFaultSignalHandler.h>
#include <WebCore/AXObjectCache.h>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/CPUMonitor.h>
#include <WebCore/CommonVM.h>
#include <WebCore/CrossOriginPreflightResultCache.h>
#include <WebCore/DNS.h>
#include <WebCore/DOMWindow.h>
#include <WebCore/DatabaseManager.h>
#include <WebCore/DatabaseTracker.h>
#include <WebCore/DeprecatedGlobalSettings.h>
#include <WebCore/DiagnosticLoggingClient.h>
#include <WebCore/DiagnosticLoggingKeys.h>
#include <WebCore/FontCache.h>
#include <WebCore/FontCascade.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/GCController.h>
#include <WebCore/GlyphPage.h>
#include <WebCore/HTMLMediaElement.h>
#include <WebCore/JSDOMWindow.h>
#include <WebCore/MainFrame.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/MemoryRelease.h>
#include <WebCore/MessagePort.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/Page.h>
#include <WebCore/PageCache.h>
#include <WebCore/PageGroup.h>
#include <WebCore/PlatformMediaSessionManager.h>
#include <WebCore/ResourceLoadObserver.h>
#include <WebCore/ResourceLoadStatistics.h>
#include <WebCore/RuntimeApplicationChecks.h>
#include <WebCore/SchemeRegistry.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/ServiceWorkerContextData.h>
#include <WebCore/Settings.h>
#include <WebCore/URLParser.h>
#include <WebCore/UserGestureIndicator.h>
#include <unistd.h>
#include <wtf/CurrentTime.h>
#include <wtf/Language.h>
#include <wtf/RunLoop.h>
#include <wtf/text/StringHash.h>

#if PLATFORM(WAYLAND)
#include "WaylandCompositorDisplay.h"
#endif

#if PLATFORM(COCOA)
#include "ObjCObjectGraph.h"
#include "UserMediaCaptureManager.h"
#endif

#if ENABLE(SEC_ITEM_SHIM)
#include "SecItemShim.h"
#endif

#if ENABLE(NOTIFICATIONS)
#include "WebNotificationManager.h"
#endif

#if ENABLE(REMOTE_INSPECTOR)
#include <JavaScriptCore/RemoteInspector.h>
#endif

using namespace JSC;
using namespace WebCore;

// This should be less than plugInAutoStartExpirationTimeThreshold in PlugInAutoStartProvider.
static const Seconds plugInAutoStartExpirationTimeUpdateThreshold { 29 * 24 * 60 * 60 };

// This should be greater than tileRevalidationTimeout in TileController.
static const Seconds nonVisibleProcessCleanupDelay { 10_s };

namespace WebKit {

WebProcess& WebProcess::singleton()
{
    static WebProcess& process = *new WebProcess;
    return process;
}

WebProcess::WebProcess()
    : m_eventDispatcher(EventDispatcher::create())
#if PLATFORM(IOS)
    , m_viewUpdateDispatcher(ViewUpdateDispatcher::create())
#endif
    , m_webInspectorInterruptDispatcher(WebInspectorInterruptDispatcher::create())
    , m_webLoaderStrategy(*new WebLoaderStrategy)
    , m_cacheStorageProvider(WebCacheStorageProvider::create())
    , m_dnsPrefetchHystereris([this](PAL::HysteresisState state) { if (state == PAL::HysteresisState::Stopped) m_dnsPrefetchedHosts.clear(); })
#if ENABLE(NETSCAPE_PLUGIN_API)
    , m_pluginProcessConnectionManager(PluginProcessConnectionManager::create())
#endif
    , m_nonVisibleProcessCleanupTimer(*this, &WebProcess::nonVisibleProcessCleanupTimerFired)
#if PLATFORM(IOS)
    , m_webSQLiteDatabaseTracker(*this)
#endif
{
    // Initialize our platform strategies.
    WebPlatformStrategies::initialize();

    // FIXME: This should moved to where WebProcess::initialize is called,
    // so that ports have a chance to customize, and ifdefs in this file are
    // limited.
    addSupplement<WebGeolocationManager>();
    addSupplement<WebCookieManager>();

#if ENABLE(NOTIFICATIONS)
    addSupplement<WebNotificationManager>();
#endif

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    addSupplement<WebMediaKeyStorageManager>();
#endif

#if PLATFORM(COCOA) && ENABLE(MEDIA_STREAM)
    addSupplement<UserMediaCaptureManager>();
#endif

    m_plugInAutoStartOriginHashes.add(PAL::SessionID::defaultSessionID(), HashMap<unsigned, WallTime>());

    ResourceLoadObserver::shared().setNotificationCallback([this] (Vector<ResourceLoadStatistics>&& statistics) {
        ASSERT(!statistics.isEmpty());
        parentProcessConnection()->send(Messages::WebResourceLoadStatisticsStore::ResourceLoadStatisticsUpdated(WTFMove(statistics)), 0);
    });

    Gigacage::disableDisablingPrimitiveGigacageIfShouldBeEnabled();
}

WebProcess::~WebProcess()
{
}

void WebProcess::initializeProcess(const ChildProcessInitializationParameters& parameters)
{
    MessagePortChannelProvider::setSharedProvider(WebMessagePortChannelProvider::singleton());
    
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101400
    // This call is needed when the WebProcess is not running the NSApplication event loop.
    // Otherwise, calling enableSandboxStyleFileQuarantine() will fail.
    launchServicesCheckIn();
#endif
    platformInitializeProcess(parameters);
}

void WebProcess::initializeConnection(IPC::Connection* connection)
{
    ChildProcess::initializeConnection(connection);

#if !PLATFORM(GTK) && !PLATFORM(WPE)
    connection->setShouldExitOnSyncMessageSendFailure(true);
#endif

#if HAVE(QOS_CLASSES)
    connection->setShouldBoostMainThreadOnSyncMessage(true);
#endif

    m_eventDispatcher->initializeConnection(connection);
#if PLATFORM(IOS)
    m_viewUpdateDispatcher->initializeConnection(connection);
#endif // PLATFORM(IOS)
    m_webInspectorInterruptDispatcher->initializeConnection(connection);

#if ENABLE(NETSCAPE_PLUGIN_API)
    m_pluginProcessConnectionManager->initializeConnection(connection);
#endif

    for (auto& supplement : m_supplements.values())
        supplement->initializeConnection(connection);

    m_webConnection = WebConnectionToUIProcess::create(this);

    // In order to ensure that the asynchronous messages that are used for notifying the UI process
    // about when WebFrame objects come and go are always delivered before the synchronous policy messages,
    // use this flag to force synchronous messages to be treated as asynchronous messages in the UI process
    // unless when doing so would lead to a deadlock.
    connection->setOnlySendMessagesAsDispatchWhenWaitingForSyncReplyWhenProcessingSuchAMessage(true);
}

void WebProcess::initializeWebProcess(WebProcessCreationParameters&& parameters)
{    
    ASSERT(m_pageMap.isEmpty());

    WebCore::setPresentingApplicationPID(parameters.presentingApplicationPID);

#if OS(LINUX)
    if (parameters.memoryPressureMonitorHandle.fileDescriptor() != -1)
        MemoryPressureHandler::singleton().setMemoryPressureMonitorHandle(parameters.memoryPressureMonitorHandle.releaseFileDescriptor());
    MemoryPressureHandler::ReliefLogger::setLoggingEnabled(parameters.shouldEnableMemoryPressureReliefLogging);
#endif

    platformInitializeWebProcess(WTFMove(parameters));

    // Match the QoS of the UIProcess and the scrolling thread but use a slightly lower priority.
    WTF::Thread::setCurrentThreadIsUserInteractive(-1);

    m_suppressMemoryPressureHandler = parameters.shouldSuppressMemoryPressureHandler;
    if (!m_suppressMemoryPressureHandler) {
        auto& memoryPressureHandler = MemoryPressureHandler::singleton();
        memoryPressureHandler.setLowMemoryHandler([] (Critical critical, Synchronous synchronous) {
            WebCore::releaseMemory(critical, synchronous);
        });
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101200) || PLATFORM(GTK) || PLATFORM(WPE)
        memoryPressureHandler.setShouldUsePeriodicMemoryMonitor(true);
        memoryPressureHandler.setMemoryKillCallback([this] () {
            WebCore::logMemoryStatisticsAtTimeOfDeath();
            if (MemoryPressureHandler::singleton().processState() == WebsamProcessState::Active)
                parentProcessConnection()->send(Messages::WebProcessProxy::DidExceedActiveMemoryLimit(), 0);
            else
                parentProcessConnection()->send(Messages::WebProcessProxy::DidExceedInactiveMemoryLimit(), 0);
        });
        memoryPressureHandler.setDidExceedInactiveLimitWhileActiveCallback([this] () {
            parentProcessConnection()->send(Messages::WebProcessProxy::DidExceedInactiveMemoryLimitWhileActive(), 0);
        });
#endif
        memoryPressureHandler.setMemoryPressureStatusChangedCallback([this](bool isUnderMemoryPressure) {
            if (parentProcessConnection())
                parentProcessConnection()->send(Messages::WebProcessProxy::MemoryPressureStatusChanged(isUnderMemoryPressure), 0);
        });
        memoryPressureHandler.install();
    }

    for (size_t i = 0, size = parameters.additionalSandboxExtensionHandles.size(); i < size; ++i)
        SandboxExtension::consumePermanently(parameters.additionalSandboxExtensionHandles[i]);

    if (!parameters.injectedBundlePath.isEmpty())
        m_injectedBundle = InjectedBundle::create(parameters, transformHandlesToObjects(parameters.initializationUserData.object()).get());

    for (auto& supplement : m_supplements.values())
        supplement->initialize(parameters);

    auto& databaseManager = DatabaseManager::singleton();
    databaseManager.initialize(parameters.webSQLDatabaseDirectory);

    // FIXME: This should be constructed per data store, not per process.
    m_applicationCacheStorage = ApplicationCacheStorage::create(parameters.applicationCacheDirectory, parameters.applicationCacheFlatFileSubdirectoryName);
#if PLATFORM(IOS)
    m_applicationCacheStorage->setDefaultOriginQuota(25ULL * 1024 * 1024);
#endif

#if ENABLE(VIDEO)
    if (!parameters.mediaCacheDirectory.isEmpty())
        WebCore::HTMLMediaElement::setMediaCacheDirectory(parameters.mediaCacheDirectory);
#endif

    setCacheModel(static_cast<uint32_t>(parameters.cacheModel));

    if (!parameters.languages.isEmpty())
        overrideUserPreferredLanguages(parameters.languages);

    m_textCheckerState = parameters.textCheckerState;

    m_fullKeyboardAccessEnabled = parameters.fullKeyboardAccessEnabled;

    for (auto& scheme : parameters.urlSchemesRegisteredAsEmptyDocument)
        registerURLSchemeAsEmptyDocument(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsSecure)
        registerURLSchemeAsSecure(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsBypassingContentSecurityPolicy)
        registerURLSchemeAsBypassingContentSecurityPolicy(scheme);

    for (auto& scheme : parameters.urlSchemesForWhichDomainRelaxationIsForbidden)
        setDomainRelaxationForbiddenForURLScheme(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsLocal)
        registerURLSchemeAsLocal(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsNoAccess)
        registerURLSchemeAsNoAccess(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsDisplayIsolated)
        registerURLSchemeAsDisplayIsolated(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsCORSEnabled)
        registerURLSchemeAsCORSEnabled(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsAlwaysRevalidated)
        registerURLSchemeAsAlwaysRevalidated(scheme);

    for (auto& scheme : parameters.urlSchemesRegisteredAsCachePartitioned)
        registerURLSchemeAsCachePartitioned(scheme);

    for (auto& scheme : parameters.urlSchemesServiceWorkersCanHandle)
        registerURLSchemeServiceWorkersCanHandle(scheme);

    setDefaultRequestTimeoutInterval(parameters.defaultRequestTimeoutInterval);

    setResourceLoadStatisticsEnabled(parameters.resourceLoadStatisticsEnabled);

    if (parameters.shouldAlwaysUseComplexTextCodePath)
        setAlwaysUsesComplexTextCodePath(true);

    if (parameters.shouldUseFontSmoothing)
        setShouldUseFontSmoothing(true);

    if (parameters.shouldUseTestingNetworkSession)
        NetworkStorageSession::switchToNewTestingSession();

    ensureNetworkProcessConnection();

    setTerminationTimeout(parameters.terminationTimeout);

    resetPlugInAutoStartOriginHashes(parameters.plugInAutoStartOriginHashes);
    for (auto& origin : parameters.plugInAutoStartOrigins)
        m_plugInAutoStartOrigins.add(origin);

    setMemoryCacheDisabled(parameters.memoryCacheDisabled);

#if ENABLE(SERVICE_CONTROLS)
    setEnabledServices(parameters.hasImageServices, parameters.hasSelectionServices, parameters.hasRichContentServices);
#endif

#if ENABLE(REMOTE_INSPECTOR) && PLATFORM(COCOA)
    audit_token_t auditToken;
    if (parentProcessConnection()->getAuditToken(auditToken)) {
        RetainPtr<CFDataRef> auditData = adoptCF(CFDataCreate(nullptr, (const UInt8*)&auditToken, sizeof(auditToken)));
        Inspector::RemoteInspector::singleton().setParentProcessInformation(WebCore::presentingApplicationPID(), auditData);
    }
#endif

#if ENABLE(NETSCAPE_PLUGIN_API) && PLATFORM(MAC)
    resetPluginLoadClientPolicies(parameters.pluginLoadClientPolicies);
#endif

#if ENABLE(GAMEPAD)
    GamepadProvider::singleton().setSharedProvider(WebGamepadProvider::singleton());
#endif

#if ENABLE(SERVICE_WORKER)
    auto& serviceWorkerProvider = WebServiceWorkerProvider::singleton();
    serviceWorkerProvider.setHasRegisteredServiceWorkers(parameters.hasRegisteredServiceWorkers);
    ServiceWorkerProvider::setSharedProvider(serviceWorkerProvider);
#endif

#if ENABLE(WEBASSEMBLY)
    JSC::Wasm::enableFastMemory();
#endif

#if HAVE(CFNETWORK_STORAGE_PARTITIONING) && !RELEASE_LOG_DISABLED
    ResourceLoadObserver::shared().setShouldLogUserInteraction(parameters.shouldLogUserInteraction);
#endif

    RELEASE_LOG(Process, "%p - WebProcess::initializeWebProcess: Presenting process = %d", this, WebCore::presentingApplicationPID());
}

void WebProcess::registerURLSchemeAsEmptyDocument(const String& urlScheme)
{
    SchemeRegistry::registerURLSchemeAsEmptyDocument(urlScheme);
}

void WebProcess::registerURLSchemeAsSecure(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsSecure(urlScheme);
}

void WebProcess::registerURLSchemeAsBypassingContentSecurityPolicy(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsBypassingContentSecurityPolicy(urlScheme);
}

void WebProcess::setDomainRelaxationForbiddenForURLScheme(const String& urlScheme) const
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(true, urlScheme);
}

void WebProcess::registerURLSchemeAsLocal(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsLocal(urlScheme);
}

void WebProcess::registerURLSchemeAsNoAccess(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsNoAccess(urlScheme);
}

void WebProcess::registerURLSchemeAsDisplayIsolated(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsDisplayIsolated(urlScheme);
}

void WebProcess::registerURLSchemeAsCORSEnabled(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsCORSEnabled(urlScheme);
}

void WebProcess::registerURLSchemeAsAlwaysRevalidated(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsAlwaysRevalidated(urlScheme);
}

void WebProcess::registerURLSchemeAsCachePartitioned(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsCachePartitioned(urlScheme);
}

void WebProcess::setDefaultRequestTimeoutInterval(double timeoutInterval)
{
    ResourceRequest::setDefaultTimeoutInterval(timeoutInterval);
}

void WebProcess::setAlwaysUsesComplexTextCodePath(bool alwaysUseComplexText)
{
    WebCore::FontCascade::setCodePath(alwaysUseComplexText ? WebCore::FontCascade::Complex : WebCore::FontCascade::Auto);
}

void WebProcess::setShouldUseFontSmoothing(bool useFontSmoothing)
{
    WebCore::FontCascade::setShouldUseSmoothing(useFontSmoothing);
}

void WebProcess::userPreferredLanguagesChanged(const Vector<String>& languages) const
{
    overrideUserPreferredLanguages(languages);
}

void WebProcess::fullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled)
{
    m_fullKeyboardAccessEnabled = fullKeyboardAccessEnabled;
}

void WebProcess::addWebsiteDataStore(WebsiteDataStoreParameters&& parameters)
{
    WebFrameNetworkingContext::ensureWebsiteDataStoreSession(WTFMove(parameters));
}

void WebProcess::destroySession(PAL::SessionID sessionID)
{
    SessionTracker::destroySession(sessionID);
}

void WebProcess::ensureLegacyPrivateBrowsingSessionInNetworkProcess()
{
    ensureNetworkProcessConnection().connection().send(Messages::NetworkConnectionToWebProcess::EnsureLegacyPrivateBrowsingSession(), 0);
}

#if ENABLE(NETSCAPE_PLUGIN_API)
PluginProcessConnectionManager& WebProcess::pluginProcessConnectionManager()
{
    return *m_pluginProcessConnectionManager;
}
#endif

void WebProcess::setCacheModel(uint32_t cm)
{
    CacheModel cacheModel = static_cast<CacheModel>(cm);

    if (m_hasSetCacheModel && (cacheModel == m_cacheModel))
        return;

    m_hasSetCacheModel = true;
    m_cacheModel = cacheModel;

    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    Seconds deadDecodedDataDeletionInterval;
    unsigned pageCacheSize = 0;
    calculateMemoryCacheSizes(cacheModel, cacheTotalCapacity, cacheMinDeadCapacity, cacheMaxDeadCapacity, deadDecodedDataDeletionInterval, pageCacheSize);

    auto& memoryCache = MemoryCache::singleton();
    memoryCache.setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    memoryCache.setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    PageCache::singleton().setMaxSize(pageCacheSize);

    platformSetCacheModel(cacheModel);
}

void WebProcess::clearCachedCredentials()
{
    NetworkStorageSession::defaultStorageSession().credentialStorage().clearCredentials();
}

WebPage* WebProcess::focusedWebPage() const
{    
    for (auto& page : m_pageMap.values()) {
        if (page->windowAndWebPageAreFocused())
            return page.get();
    }
    return 0;
}
    
WebPage* WebProcess::webPage(uint64_t pageID) const
{
    return m_pageMap.get(pageID);
}

void WebProcess::createWebPage(uint64_t pageID, WebPageCreationParameters&& parameters)
{
    // It is necessary to check for page existence here since during a window.open() (or targeted
    // link) the WebPage gets created both in the synchronous handler and through the normal way. 
    HashMap<uint64_t, RefPtr<WebPage>>::AddResult result = m_pageMap.add(pageID, nullptr);
    if (result.isNewEntry) {
        ASSERT(!result.iterator->value);
        result.iterator->value = WebPage::create(pageID, WTFMove(parameters));

        // Balanced by an enableTermination in removeWebPage.
        disableTermination();
        updateCPULimit();
    } else
        result.iterator->value->reinitializeWebPage(WTFMove(parameters));

    ASSERT(result.iterator->value);
}

void WebProcess::removeWebPage(uint64_t pageID)
{
    ASSERT(m_pageMap.contains(pageID));

    pageWillLeaveWindow(pageID);
    m_pageMap.remove(pageID);

    enableTermination();
    updateCPULimit();
}

bool WebProcess::shouldTerminate()
{
    ASSERT(m_pageMap.isEmpty());

    // FIXME: the ShouldTerminate message should also send termination parameters, such as any session cookies that need to be preserved.
    bool shouldTerminate = false;
    if (parentProcessConnection()->sendSync(Messages::WebProcessProxy::ShouldTerminate(), Messages::WebProcessProxy::ShouldTerminate::Reply(shouldTerminate), 0)
        && !shouldTerminate)
        return false;

    return true;
}

void WebProcess::terminate()
{
#ifndef NDEBUG
    GCController::singleton().garbageCollectNow();
    FontCache::singleton().invalidate();
    MemoryCache::singleton().setDisabled(true);
#endif

    m_webConnection->invalidate();
    m_webConnection = nullptr;

    platformTerminate();

    ChildProcess::terminate();
}

void WebProcess::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (messageReceiverMap().dispatchSyncMessage(connection, decoder, replyEncoder))
        return;

    didReceiveSyncWebProcessMessage(connection, decoder, replyEncoder);
}

void WebProcess::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (messageReceiverMap().dispatchMessage(connection, decoder))
        return;

    if (decoder.messageReceiverName() == Messages::WebProcess::messageReceiverName()) {
        didReceiveWebProcessMessage(connection, decoder);
        return;
    }

    if (decoder.messageReceiverName() == Messages::ChildProcess::messageReceiverName()) {
        ChildProcess::didReceiveMessage(connection, decoder);
        return;
    }

#if ENABLE(SERVICE_WORKER)
    // FIXME: Remove?
    if (decoder.messageReceiverName() == Messages::WebSWContextManagerConnection::messageReceiverName()) {
        ASSERT(SWContextManager::singleton().connection());
        if (auto* contextManagerConnection = SWContextManager::singleton().connection())
            static_cast<WebSWContextManagerConnection&>(*contextManagerConnection).didReceiveMessage(connection, decoder);
        return;
    }
#endif

    LOG_ERROR("Unhandled web process message '%s:%s'", decoder.messageReceiverName().toString().data(), decoder.messageName().toString().data());
}

void WebProcess::didClose(IPC::Connection&)
{
#if !defined(NDEBUG) || PLATFORM(GTK) || PLATFORM(WPE)
    for (auto& page : copyToVector(m_pageMap.values()))
        page->close();
#endif

#ifndef NDEBUG
    GCController::singleton().garbageCollectSoon();
    FontCache::singleton().invalidate();
    MemoryCache::singleton().setDisabled(true);
#endif

#if ENABLE(VIDEO)
    // FIXME(146657): This explicit media stop command should not be necessary
    if (auto* platformMediaSessionManager = PlatformMediaSessionManager::sharedManagerIfExists())
        platformMediaSessionManager->stopAllMediaPlaybackForProcess();
#endif

    // The UI process closed this connection, shut down.
    stopRunLoop();
}

WebFrame* WebProcess::webFrame(uint64_t frameID) const
{
    return m_frameMap.get(frameID);
}

void WebProcess::addWebFrame(uint64_t frameID, WebFrame* frame)
{
    m_frameMap.set(frameID, frame);
}

void WebProcess::removeWebFrame(uint64_t frameID)
{
    m_frameMap.remove(frameID);

    // We can end up here after our connection has closed when WebCore's frame life-support timer
    // fires when the application is shutting down. There's no need (and no way) to update the UI
    // process in this case.
    if (!parentProcessConnection())
        return;

    parentProcessConnection()->send(Messages::WebProcessProxy::DidDestroyFrame(frameID), 0);
}

WebPageGroupProxy* WebProcess::webPageGroup(PageGroup* pageGroup)
{
    for (auto& page : m_pageGroupMap.values()) {
        if (page->corePageGroup() == pageGroup)
            return page.get();
    }

    return 0;
}

WebPageGroupProxy* WebProcess::webPageGroup(uint64_t pageGroupID)
{
    return m_pageGroupMap.get(pageGroupID);
}

WebPageGroupProxy* WebProcess::webPageGroup(const WebPageGroupData& pageGroupData)
{
    auto result = m_pageGroupMap.add(pageGroupData.pageGroupID, nullptr);
    if (result.isNewEntry) {
        ASSERT(!result.iterator->value);
        result.iterator->value = WebPageGroupProxy::create(pageGroupData);
    }

    return result.iterator->value.get();
}

static uint64_t nextUserGestureTokenIdentifier()
{
    static uint64_t identifier = 1;
    return identifier++;
}

uint64_t WebProcess::userGestureTokenIdentifier(RefPtr<UserGestureToken> token)
{
    if (!token || !token->processingUserGesture())
        return 0;

    auto result = m_userGestureTokens.ensure(token.get(), [] { return nextUserGestureTokenIdentifier(); });
    if (result.isNewEntry) {
        result.iterator->key->addDestructionObserver([] (UserGestureToken& tokenBeingDestroyed) {
            WebProcess::singleton().userGestureTokenDestroyed(tokenBeingDestroyed);
        });
    }
    
    return result.iterator->value;
}

void WebProcess::userGestureTokenDestroyed(UserGestureToken& token)
{
    auto identifier = m_userGestureTokens.take(&token);
    parentProcessConnection()->send(Messages::WebProcessProxy::DidDestroyUserGestureToken(identifier), 0);
}

void WebProcess::clearResourceCaches(ResourceCachesToClear resourceCachesToClear)
{
    // Toggling the cache model like this forces the cache to evict all its in-memory resources.
    // FIXME: We need a better way to do this.
    CacheModel cacheModel = m_cacheModel;
    setCacheModel(CacheModelDocumentViewer);
    setCacheModel(cacheModel);

    MemoryCache::singleton().evictResources();

    // Empty the cross-origin preflight cache.
    CrossOriginPreflightResultCache::singleton().empty();
}

static inline void addCaseFoldedCharacters(StringHasher& hasher, const String& string)
{
    if (string.isEmpty())
        return;
    if (string.is8Bit()) {
        hasher.addCharacters<LChar, ASCIICaseInsensitiveHash::FoldCase<LChar>>(string.characters8(), string.length());
        return;
    }
    hasher.addCharacters<UChar, ASCIICaseInsensitiveHash::FoldCase<UChar>>(string.characters16(), string.length());
}

static unsigned hashForPlugInOrigin(const String& pageOrigin, const String& pluginOrigin, const String& mimeType)
{
    // We want to avoid concatenating the strings and then taking the hash, since that could lead to an expensive conversion.
    // We also want to avoid using the hash() function in StringImpl or ASCIICaseInsensitiveHash because that masks out bits for the use of flags.
    StringHasher hasher;
    addCaseFoldedCharacters(hasher, pageOrigin);
    hasher.addCharacter(0);
    addCaseFoldedCharacters(hasher, pluginOrigin);
    hasher.addCharacter(0);
    addCaseFoldedCharacters(hasher, mimeType);
    return hasher.hash();
}

bool WebProcess::isPlugInAutoStartOriginHash(unsigned plugInOriginHash, PAL::SessionID sessionID)
{
    HashMap<PAL::SessionID, HashMap<unsigned, WallTime>>::const_iterator sessionIterator = m_plugInAutoStartOriginHashes.find(sessionID);
    HashMap<unsigned, WallTime>::const_iterator it;
    bool contains = false;

    if (sessionIterator != m_plugInAutoStartOriginHashes.end()) {
        it = sessionIterator->value.find(plugInOriginHash);
        contains = it != sessionIterator->value.end();
    }
    if (!contains) {
        sessionIterator = m_plugInAutoStartOriginHashes.find(PAL::SessionID::defaultSessionID());
        it = sessionIterator->value.find(plugInOriginHash);
        if (it == sessionIterator->value.end())
            return false;
    }
    return WallTime::now() < it->value;
}

bool WebProcess::shouldPlugInAutoStartFromOrigin(WebPage& webPage, const String& pageOrigin, const String& pluginOrigin, const String& mimeType)
{
    if (!pluginOrigin.isEmpty() && m_plugInAutoStartOrigins.contains(pluginOrigin))
        return true;

#ifdef ENABLE_PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC
    // The plugin wasn't in the general whitelist, so check if it similar to the primary plugin for the page (if we've found one).
    if (webPage.matchesPrimaryPlugIn(pageOrigin, pluginOrigin, mimeType))
        return true;
#else
    UNUSED_PARAM(webPage);
#endif

    // Lastly check against the more explicit hash list.
    return isPlugInAutoStartOriginHash(hashForPlugInOrigin(pageOrigin, pluginOrigin, mimeType), webPage.sessionID());
}

void WebProcess::plugInDidStartFromOrigin(const String& pageOrigin, const String& pluginOrigin, const String& mimeType, PAL::SessionID sessionID)
{
    if (pageOrigin.isEmpty()) {
        LOG(Plugins, "Not adding empty page origin");
        return;
    }

    unsigned plugInOriginHash = hashForPlugInOrigin(pageOrigin, pluginOrigin, mimeType);
    if (isPlugInAutoStartOriginHash(plugInOriginHash, sessionID)) {
        LOG(Plugins, "Hash %x already exists as auto-start origin (request for %s)", plugInOriginHash, pageOrigin.utf8().data());
        return;
    }

    // We might attempt to start another plugin before the didAddPlugInAutoStartOrigin message
    // comes back from the parent process. Temporarily add this hash to the list with a thirty
    // second timeout. That way, even if the parent decides not to add it, we'll only be
    // incorrect for a little while.
    m_plugInAutoStartOriginHashes.add(sessionID, HashMap<unsigned, WallTime>()).iterator->value.set(plugInOriginHash, WallTime::now() + 30_s * 1000);

    parentProcessConnection()->send(Messages::WebProcessPool::AddPlugInAutoStartOriginHash(pageOrigin, plugInOriginHash, sessionID), 0);
}

void WebProcess::didAddPlugInAutoStartOriginHash(unsigned plugInOriginHash, WallTime expirationTime, PAL::SessionID sessionID)
{
    // When called, some web process (which also might be this one) added the origin for auto-starting,
    // or received user interaction.
    // Set the bit to avoid having redundantly call into the UI process upon user interaction.
    m_plugInAutoStartOriginHashes.add(sessionID, HashMap<unsigned, WallTime>()).iterator->value.set(plugInOriginHash, expirationTime);
}

void WebProcess::resetPlugInAutoStartOriginDefaultHashes(const HashMap<unsigned, WallTime>& hashes)
{
    m_plugInAutoStartOriginHashes.clear();
    m_plugInAutoStartOriginHashes.add(PAL::SessionID::defaultSessionID(), HashMap<unsigned, WallTime>()).iterator->value.swap(const_cast<HashMap<unsigned, WallTime>&>(hashes));
}

void WebProcess::resetPlugInAutoStartOriginHashes(const HashMap<PAL::SessionID, HashMap<unsigned, WallTime>>& hashes)
{
    m_plugInAutoStartOriginHashes.swap(const_cast<HashMap<PAL::SessionID, HashMap<unsigned, WallTime>>&>(hashes));
}

void WebProcess::plugInDidReceiveUserInteraction(const String& pageOrigin, const String& pluginOrigin, const String& mimeType, PAL::SessionID sessionID)
{
    if (pageOrigin.isEmpty())
        return;

    unsigned plugInOriginHash = hashForPlugInOrigin(pageOrigin, pluginOrigin, mimeType);
    if (!plugInOriginHash)
        return;

    HashMap<PAL::SessionID, HashMap<unsigned, WallTime>>::const_iterator sessionIterator = m_plugInAutoStartOriginHashes.find(sessionID);
    HashMap<unsigned, WallTime>::const_iterator it;
    bool contains = false;
    if (sessionIterator != m_plugInAutoStartOriginHashes.end()) {
        it = sessionIterator->value.find(plugInOriginHash);
        contains = it != sessionIterator->value.end();
    }
    if (!contains) {
        sessionIterator = m_plugInAutoStartOriginHashes.find(PAL::SessionID::defaultSessionID());
        it = sessionIterator->value.find(plugInOriginHash);
        if (it == sessionIterator->value.end())
            return;
    }

    if (it->value - WallTime::now() > plugInAutoStartExpirationTimeUpdateThreshold)
        return;

    parentProcessConnection()->send(Messages::WebProcessPool::PlugInDidReceiveUserInteraction(plugInOriginHash, sessionID), 0);
}

void WebProcess::setPluginLoadClientPolicy(uint8_t policy, const String& host, const String& bundleIdentifier, const String& versionString)
{
#if ENABLE(NETSCAPE_PLUGIN_API) && PLATFORM(MAC)
    WebPluginInfoProvider::singleton().setPluginLoadClientPolicy(static_cast<PluginLoadClientPolicy>(policy), host, bundleIdentifier, versionString);
#endif
}

void WebProcess::resetPluginLoadClientPolicies(const HashMap<WTF::String, HashMap<WTF::String, HashMap<WTF::String, uint8_t>>>& pluginLoadClientPolicies)
{
#if ENABLE(NETSCAPE_PLUGIN_API) && PLATFORM(MAC)
    clearPluginClientPolicies();

    for (auto& hostPair : pluginLoadClientPolicies) {
        for (auto& bundleIdentifierPair : hostPair.value) {
            for (auto& versionPair : bundleIdentifierPair.value)
                WebPluginInfoProvider::singleton().setPluginLoadClientPolicy(static_cast<PluginLoadClientPolicy>(versionPair.value), hostPair.key, bundleIdentifierPair.key, versionPair.key);
        }
    }
#endif
}

void WebProcess::clearPluginClientPolicies()
{
#if ENABLE(NETSCAPE_PLUGIN_API) && PLATFORM(MAC)
    WebPluginInfoProvider::singleton().clearPluginClientPolicies();
#endif
}

void WebProcess::refreshPlugins()
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    WebPluginInfoProvider::singleton().refresh(false);
#endif
}

static void fromCountedSetToHashMap(TypeCountSet* countedSet, HashMap<String, uint64_t>& map)
{
    TypeCountSet::const_iterator end = countedSet->end();
    for (TypeCountSet::const_iterator it = countedSet->begin(); it != end; ++it)
        map.set(it->key, it->value);
}

static void getWebCoreMemoryCacheStatistics(Vector<HashMap<String, uint64_t>>& result)
{
    String imagesString(ASCIILiteral("Images"));
    String cssString(ASCIILiteral("CSS"));
    String xslString(ASCIILiteral("XSL"));
    String javaScriptString(ASCIILiteral("JavaScript"));
    
    MemoryCache::Statistics memoryCacheStatistics = MemoryCache::singleton().getStatistics();
    
    HashMap<String, uint64_t> counts;
    counts.set(imagesString, memoryCacheStatistics.images.count);
    counts.set(cssString, memoryCacheStatistics.cssStyleSheets.count);
    counts.set(xslString, memoryCacheStatistics.xslStyleSheets.count);
    counts.set(javaScriptString, memoryCacheStatistics.scripts.count);
    result.append(counts);
    
    HashMap<String, uint64_t> sizes;
    sizes.set(imagesString, memoryCacheStatistics.images.size);
    sizes.set(cssString, memoryCacheStatistics.cssStyleSheets.size);
    sizes.set(xslString, memoryCacheStatistics.xslStyleSheets.size);
    sizes.set(javaScriptString, memoryCacheStatistics.scripts.size);
    result.append(sizes);
    
    HashMap<String, uint64_t> liveSizes;
    liveSizes.set(imagesString, memoryCacheStatistics.images.liveSize);
    liveSizes.set(cssString, memoryCacheStatistics.cssStyleSheets.liveSize);
    liveSizes.set(xslString, memoryCacheStatistics.xslStyleSheets.liveSize);
    liveSizes.set(javaScriptString, memoryCacheStatistics.scripts.liveSize);
    result.append(liveSizes);
    
    HashMap<String, uint64_t> decodedSizes;
    decodedSizes.set(imagesString, memoryCacheStatistics.images.decodedSize);
    decodedSizes.set(cssString, memoryCacheStatistics.cssStyleSheets.decodedSize);
    decodedSizes.set(xslString, memoryCacheStatistics.xslStyleSheets.decodedSize);
    decodedSizes.set(javaScriptString, memoryCacheStatistics.scripts.decodedSize);
    result.append(decodedSizes);
}

void WebProcess::getWebCoreStatistics(uint64_t callbackID)
{
    StatisticsData data;
    
    // Gather JavaScript statistics.
    {
        JSLockHolder lock(commonVM());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptObjectsCount"), commonVM().heap.objectCount());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptGlobalObjectsCount"), commonVM().heap.globalObjectCount());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptProtectedObjectsCount"), commonVM().heap.protectedObjectCount());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptProtectedGlobalObjectsCount"), commonVM().heap.protectedGlobalObjectCount());
        
        std::unique_ptr<TypeCountSet> protectedObjectTypeCounts(commonVM().heap.protectedObjectTypeCounts());
        fromCountedSetToHashMap(protectedObjectTypeCounts.get(), data.javaScriptProtectedObjectTypeCounts);
        
        std::unique_ptr<TypeCountSet> objectTypeCounts(commonVM().heap.objectTypeCounts());
        fromCountedSetToHashMap(objectTypeCounts.get(), data.javaScriptObjectTypeCounts);
        
        uint64_t javaScriptHeapSize = commonVM().heap.size();
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptHeapSize"), javaScriptHeapSize);
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptFreeSize"), commonVM().heap.capacity() - javaScriptHeapSize);
    }

    WTF::FastMallocStatistics fastMallocStatistics = WTF::fastMallocStatistics();
    data.statisticsNumbers.set(ASCIILiteral("FastMallocReservedVMBytes"), fastMallocStatistics.reservedVMBytes);
    data.statisticsNumbers.set(ASCIILiteral("FastMallocCommittedVMBytes"), fastMallocStatistics.committedVMBytes);
    data.statisticsNumbers.set(ASCIILiteral("FastMallocFreeListBytes"), fastMallocStatistics.freeListBytes);
    
    // Gather font statistics.
    auto& fontCache = FontCache::singleton();
    data.statisticsNumbers.set(ASCIILiteral("CachedFontDataCount"), fontCache.fontCount());
    data.statisticsNumbers.set(ASCIILiteral("CachedFontDataInactiveCount"), fontCache.inactiveFontCount());
    
    // Gather glyph page statistics.
    data.statisticsNumbers.set(ASCIILiteral("GlyphPageCount"), GlyphPage::count());
    
    // Get WebCore memory cache statistics
    getWebCoreMemoryCacheStatistics(data.webCoreCacheStatistics);
    
    parentProcessConnection()->send(Messages::WebProcessPool::DidGetStatistics(data, callbackID), 0);
}

void WebProcess::garbageCollectJavaScriptObjects()
{
    GCController::singleton().garbageCollectNow();
}

void WebProcess::mainThreadPing()
{
    parentProcessConnection()->send(Messages::WebProcessProxy::DidReceiveMainThreadPing(), 0);
}

void WebProcess::backgroundResponsivenessPing()
{
    parentProcessConnection()->send(Messages::WebProcessProxy::DidReceiveBackgroundResponsivenessPing(), 0);
}

void WebProcess::syncIPCMessageWhileWaitingForSyncReplyForTesting()
{
}

void WebProcess::didTakeAllMessagesForPort(Vector<MessageWithMessagePorts>&& messages, uint64_t messageCallbackIdentifier, uint64_t messageBatchIdentifier)
{
    WebMessagePortChannelProvider::singleton().didTakeAllMessagesForPort(WTFMove(messages), messageCallbackIdentifier, messageBatchIdentifier);
}

void WebProcess::checkProcessLocalPortForActivity(const MessagePortIdentifier& port, uint64_t callbackIdentifier)
{
    WebMessagePortChannelProvider::singleton().checkProcessLocalPortForActivity(port, callbackIdentifier);
}

void WebProcess::didCheckRemotePortForActivity(uint64_t callbackIdentifier, bool hasActivity)
{
    WebMessagePortChannelProvider::singleton().didCheckRemotePortForActivity(callbackIdentifier, hasActivity);
}

void WebProcess::messagesAvailableForPort(const MessagePortIdentifier& identifier)
{
    MessagePort::notifyMessageAvailable(identifier);
}

#if ENABLE(GAMEPAD)

void WebProcess::setInitialGamepads(const Vector<WebKit::GamepadData>& gamepadDatas)
{
    WebGamepadProvider::singleton().setInitialGamepads(gamepadDatas);
}

void WebProcess::gamepadConnected(const GamepadData& gamepadData)
{
    WebGamepadProvider::singleton().gamepadConnected(gamepadData);
}

void WebProcess::gamepadDisconnected(unsigned index)
{
    WebGamepadProvider::singleton().gamepadDisconnected(index);
}

#endif

void WebProcess::setJavaScriptGarbageCollectorTimerEnabled(bool flag)
{
    GCController::singleton().setJavaScriptGarbageCollectorTimerEnabled(flag);
}

void WebProcess::handleInjectedBundleMessage(const String& messageName, const UserData& messageBody)
{
    InjectedBundle* injectedBundle = WebProcess::singleton().injectedBundle();
    if (!injectedBundle)
        return;

    injectedBundle->didReceiveMessage(messageName, transformHandlesToObjects(messageBody.object()).get());
}

void WebProcess::setInjectedBundleParameter(const String& key, const IPC::DataReference& value)
{
    InjectedBundle* injectedBundle = WebProcess::singleton().injectedBundle();
    if (!injectedBundle)
        return;

    injectedBundle->setBundleParameter(key, value);
}

void WebProcess::setInjectedBundleParameters(const IPC::DataReference& value)
{
    InjectedBundle* injectedBundle = WebProcess::singleton().injectedBundle();
    if (!injectedBundle)
        return;

    injectedBundle->setBundleParameters(value);
}

NetworkProcessConnection& WebProcess::ensureNetworkProcessConnection()
{
    // If we've lost our connection to the network process (e.g. it crashed) try to re-establish it.
    if (!m_networkProcessConnection) {
        IPC::Attachment encodedConnectionIdentifier;

        if (!parentProcessConnection()->sendSync(Messages::WebProcessProxy::GetNetworkProcessConnection(), Messages::WebProcessProxy::GetNetworkProcessConnection::Reply(encodedConnectionIdentifier), 0, Seconds::infinity(), IPC::SendSyncOption::DoNotProcessIncomingMessagesWhenWaitingForSyncReply)) {
#if PLATFORM(GTK) || PLATFORM(WPE)
            // GTK+ and WPE ports don't exit on send sync message failure.
            // In this particular case, the network process can be terminated by the UI process while the
            // Web process is still initializing, so we always want to exit instead of crashing. This can
            // happen when the WebView is created and then destroyed quickly.
            // See https://bugs.webkit.org/show_bug.cgi?id=183348.
            exit(0);
#else
            CRASH();
#endif
        }

#if USE(UNIX_DOMAIN_SOCKETS)
        IPC::Connection::Identifier connectionIdentifier = encodedConnectionIdentifier.releaseFileDescriptor();
#elif OS(DARWIN)
        IPC::Connection::Identifier connectionIdentifier(encodedConnectionIdentifier.port());
#else
        ASSERT_NOT_REACHED();
#endif
        if (IPC::Connection::identifierIsNull(connectionIdentifier))
            CRASH();
        m_networkProcessConnection = NetworkProcessConnection::create(connectionIdentifier);
    }
    
    return *m_networkProcessConnection;
}

void WebProcess::logDiagnosticMessageForNetworkProcessCrash()
{
    WebCore::Page* page = nullptr;

    if (auto* webPage = focusedWebPage())
        page = webPage->corePage();

    if (!page) {
        for (auto& webPage : m_pageMap.values()) {
            if (auto* corePage = webPage->corePage()) {
                page = corePage;
                break;
            }
        }
    }

    if (page)
        page->diagnosticLoggingClient().logDiagnosticMessage(WebCore::DiagnosticLoggingKeys::internalErrorKey(), WebCore::DiagnosticLoggingKeys::networkProcessCrashedKey(), WebCore::ShouldSample::No);
}

void WebProcess::networkProcessConnectionClosed(NetworkProcessConnection* connection)
{
    ASSERT(m_networkProcessConnection);
    ASSERT_UNUSED(connection, m_networkProcessConnection == connection);

    m_networkProcessConnection = nullptr;

    logDiagnosticMessageForNetworkProcessCrash();

    m_webLoaderStrategy.networkProcessCrashed();
    WebSocketStream::networkProcessCrashed();

    for (auto& page : m_pageMap.values())
        page->stopAllURLSchemeTasks();
}

WebLoaderStrategy& WebProcess::webLoaderStrategy()
{
    return m_webLoaderStrategy;
}

void WebProcess::webToStorageProcessConnectionClosed(WebToStorageProcessConnection* connection)
{
    ASSERT(m_webToStorageProcessConnection);
    ASSERT(m_webToStorageProcessConnection == connection);

    m_webToStorageProcessConnection = nullptr;
}

WebToStorageProcessConnection& WebProcess::ensureWebToStorageProcessConnection(PAL::SessionID initialSessionID)
{
    if (!m_webToStorageProcessConnection) {
        IPC::Attachment encodedConnectionIdentifier;

        if (!parentProcessConnection()->sendSync(Messages::WebProcessProxy::GetStorageProcessConnection(initialSessionID), Messages::WebProcessProxy::GetStorageProcessConnection::Reply(encodedConnectionIdentifier), 0)) {
#if PLATFORM(GTK) || PLATFORM(WPE)
            // GTK+ and WPE ports don't exit on send sync message failure.
            // In this particular case, the storage process can be terminated by the UI process while the
            // connection is being done, so we always want to exit instead of crashing.
            // See https://bugs.webkit.org/show_bug.cgi?id=183348.
            exit(0);
#else
            CRASH();
#endif
        }

#if USE(UNIX_DOMAIN_SOCKETS)
        IPC::Connection::Identifier connectionIdentifier = encodedConnectionIdentifier.releaseFileDescriptor();
#elif OS(DARWIN)
        IPC::Connection::Identifier connectionIdentifier(encodedConnectionIdentifier.port());
#elif OS(WINDOWS)
        IPC::Connection::Identifier connectionIdentifier(encodedConnectionIdentifier.handle());
#else
        ASSERT_NOT_REACHED();
#endif
        if (IPC::Connection::identifierIsNull(connectionIdentifier))
            CRASH();
        m_webToStorageProcessConnection = WebToStorageProcessConnection::create(connectionIdentifier);

    }
    return *m_webToStorageProcessConnection;
}

void WebProcess::setEnhancedAccessibility(bool flag)
{
    WebCore::AXObjectCache::setEnhancedUserInterfaceAccessibility(flag);
}
    
void WebProcess::startMemorySampler(SandboxExtension::Handle&& sampleLogFileHandle, const String& sampleLogFilePath, const double interval)
{
#if ENABLE(MEMORY_SAMPLER)    
    WebMemorySampler::singleton()->start(WTFMove(sampleLogFileHandle), sampleLogFilePath, interval);
#else
    UNUSED_PARAM(sampleLogFileHandle);
    UNUSED_PARAM(sampleLogFilePath);
    UNUSED_PARAM(interval);
#endif
}
    
void WebProcess::stopMemorySampler()
{
#if ENABLE(MEMORY_SAMPLER)
    WebMemorySampler::singleton()->stop();
#endif
}

void WebProcess::setTextCheckerState(const TextCheckerState& textCheckerState)
{
    bool continuousSpellCheckingTurnedOff = !textCheckerState.isContinuousSpellCheckingEnabled && m_textCheckerState.isContinuousSpellCheckingEnabled;
    bool grammarCheckingTurnedOff = !textCheckerState.isGrammarCheckingEnabled && m_textCheckerState.isGrammarCheckingEnabled;

    m_textCheckerState = textCheckerState;

    if (!continuousSpellCheckingTurnedOff && !grammarCheckingTurnedOff)
        return;

    for (auto& page : m_pageMap.values()) {
        if (continuousSpellCheckingTurnedOff)
            page->unmarkAllMisspellings();
        if (grammarCheckingTurnedOff)
            page->unmarkAllBadGrammar();
    }
}

void WebProcess::releasePageCache()
{
    PageCache::singleton().pruneToSizeNow(0, PruningReason::MemoryPressure);
}

void WebProcess::fetchWebsiteData(PAL::SessionID sessionID, OptionSet<WebsiteDataType> websiteDataTypes, WebsiteData& websiteData)
{
    if (websiteDataTypes.contains(WebsiteDataType::MemoryCache)) {
        for (auto& origin : MemoryCache::singleton().originsWithCache(sessionID))
            websiteData.entries.append(WebsiteData::Entry { SecurityOriginData::fromSecurityOrigin(*origin), WebsiteDataType::MemoryCache, 0 });
    }

    if (websiteDataTypes.contains(WebsiteDataType::Credentials)) {
        if (NetworkStorageSession::storageSession(sessionID))
            websiteData.originsWithCredentials = NetworkStorageSession::storageSession(sessionID)->credentialStorage().originsWithCredentials();
    }
}

void WebProcess::deleteWebsiteData(PAL::SessionID sessionID, OptionSet<WebsiteDataType> websiteDataTypes, WallTime modifiedSince)
{
    UNUSED_PARAM(modifiedSince);

    if (websiteDataTypes.contains(WebsiteDataType::MemoryCache)) {
        PageCache::singleton().pruneToSizeNow(0, PruningReason::None);
        MemoryCache::singleton().evictResources(sessionID);

        CrossOriginPreflightResultCache::singleton().empty();
    }

    if (websiteDataTypes.contains(WebsiteDataType::Credentials)) {
        if (WebCore::NetworkStorageSession::storageSession(sessionID))
            NetworkStorageSession::storageSession(sessionID)->credentialStorage().clearCredentials();
    }
}

void WebProcess::deleteWebsiteDataForOrigins(PAL::SessionID sessionID, OptionSet<WebsiteDataType> websiteDataTypes, const Vector<WebCore::SecurityOriginData>& originDatas)
{
    if (websiteDataTypes.contains(WebsiteDataType::MemoryCache)) {
        HashSet<RefPtr<SecurityOrigin>> origins;
        for (auto& originData : originDatas)
            origins.add(originData.securityOrigin());

        MemoryCache::singleton().removeResourcesWithOrigins(sessionID, origins);
    }
}

void WebProcess::setHiddenPageDOMTimerThrottlingIncreaseLimit(int milliseconds)
{
    for (auto& page : m_pageMap.values())
        page->setHiddenPageDOMTimerThrottlingIncreaseLimit(Seconds::fromMilliseconds(milliseconds));
}

#if !PLATFORM(COCOA)
void WebProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}

void WebProcess::initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&)
{
}

void WebProcess::platformInitializeProcess(const ChildProcessInitializationParameters&)
{
}

void WebProcess::updateActivePages()
{
}

void WebProcess::updateCPULimit()
{
}

void WebProcess::updateCPUMonitorState(CPUMonitorUpdateReason)
{
}

#endif

void WebProcess::pageActivityStateDidChange(uint64_t, WebCore::ActivityState::Flags changed)
{
    if (changed & WebCore::ActivityState::IsVisible)
        updateCPUMonitorState(CPUMonitorUpdateReason::VisibilityHasChanged);
}

#if PLATFORM(IOS)
void WebProcess::resetAllGeolocationPermissions()
{
    for (auto& page : m_pageMap.values()) {
        if (Frame* mainFrame = page->mainFrame())
            mainFrame->resetAllGeolocationPermission();
    }
}
#endif

void WebProcess::actualPrepareToSuspend(ShouldAcknowledgeWhenReadyToSuspend shouldAcknowledgeWhenReadyToSuspend)
{
    if (!m_suppressMemoryPressureHandler)
        MemoryPressureHandler::singleton().releaseMemory(Critical::Yes, Synchronous::Yes);

    setAllLayerTreeStatesFrozen(true);
    
#if PLATFORM(COCOA)
    destroyRenderingResources();
#endif

#if PLATFORM(IOS)
    accessibilityProcessSuspendedNotification(true);
#endif

    markAllLayersVolatile([this, shouldAcknowledgeWhenReadyToSuspend] {
        RELEASE_LOG(ProcessSuspension, "%p - WebProcess::markAllLayersVolatile() Successfuly marked all layers as volatile", this);

        if (shouldAcknowledgeWhenReadyToSuspend == ShouldAcknowledgeWhenReadyToSuspend::Yes) {
            RELEASE_LOG(ProcessSuspension, "%p - WebProcess::actualPrepareToSuspend() Sending ProcessReadyToSuspend IPC message", this);
            parentProcessConnection()->send(Messages::WebProcessProxy::ProcessReadyToSuspend(), 0);
        }
    });
}

void WebProcess::processWillSuspendImminently(bool& handled)
{
    if (parentProcessConnection()->inSendSync()) {
        // Avoid reentrency bugs such as rdar://problem/21605505 by just bailing
        // if we get an incoming ProcessWillSuspendImminently message when waiting for a
        // reply to a sync message.
        // FIXME: ProcessWillSuspendImminently should not be a sync message.
        return;
    }

    RELEASE_LOG(ProcessSuspension, "%p - WebProcess::processWillSuspendImminently()", this);
    DatabaseTracker::singleton().closeAllDatabases(CurrentQueryBehavior::Interrupt);
    actualPrepareToSuspend(ShouldAcknowledgeWhenReadyToSuspend::No);
    handled = true;
}

void WebProcess::prepareToSuspend()
{
    RELEASE_LOG(ProcessSuspension, "%p - WebProcess::prepareToSuspend()", this);
    actualPrepareToSuspend(ShouldAcknowledgeWhenReadyToSuspend::Yes);
}

void WebProcess::cancelPrepareToSuspend()
{
    RELEASE_LOG(ProcessSuspension, "%p - WebProcess::cancelPrepareToSuspend()", this);
    setAllLayerTreeStatesFrozen(false);

    // If we've already finished cleaning up and sent ProcessReadyToSuspend, we
    // shouldn't send DidCancelProcessSuspension; the UI process strictly expects one or the other.
    if (!m_pageMarkingLayersAsVolatileCounter)
        return;

    cancelMarkAllLayersVolatile();

    RELEASE_LOG(ProcessSuspension, "%p - WebProcess::cancelPrepareToSuspend() Sending DidCancelProcessSuspension IPC message", this);
    parentProcessConnection()->send(Messages::WebProcessProxy::DidCancelProcessSuspension(), 0);
}

void WebProcess::markAllLayersVolatile(WTF::Function<void()>&& completionHandler)
{
    RELEASE_LOG(ProcessSuspension, "%p - WebProcess::markAllLayersVolatile()", this);
    ASSERT(!m_pageMarkingLayersAsVolatileCounter);
    m_pageMarkingLayersAsVolatileCounter = std::make_unique<PageMarkingLayersAsVolatileCounter>([this, completionHandler = WTFMove(completionHandler)] (RefCounterEvent) {
        if (m_pageMarkingLayersAsVolatileCounter->value())
            return;

        completionHandler();
        m_pageMarkingLayersAsVolatileCounter = nullptr;
    });
    auto token = m_pageMarkingLayersAsVolatileCounter->count();
    for (auto& page : m_pageMap.values())
        page->markLayersVolatile([token] { });
}

void WebProcess::cancelMarkAllLayersVolatile()
{
    if (!m_pageMarkingLayersAsVolatileCounter)
        return;

    m_pageMarkingLayersAsVolatileCounter = nullptr;
    for (auto& page : m_pageMap.values())
        page->cancelMarkLayersVolatile();
}

void WebProcess::setAllLayerTreeStatesFrozen(bool frozen)
{
    for (auto& page : m_pageMap.values())
        page->setLayerTreeStateIsFrozen(frozen);
}
    
void WebProcess::processDidResume()
{
    RELEASE_LOG(ProcessSuspension, "%p - WebProcess::processDidResume()", this);

    cancelMarkAllLayersVolatile();
    setAllLayerTreeStatesFrozen(false);
    
#if PLATFORM(IOS)
    accessibilityProcessSuspendedNotification(false);
#endif
}

void WebProcess::pageDidEnterWindow(uint64_t pageID)
{
    m_pagesInWindows.add(pageID);
    m_nonVisibleProcessCleanupTimer.stop();
}

void WebProcess::pageWillLeaveWindow(uint64_t pageID)
{
    m_pagesInWindows.remove(pageID);

    if (m_pagesInWindows.isEmpty() && !m_nonVisibleProcessCleanupTimer.isActive())
        m_nonVisibleProcessCleanupTimer.startOneShot(nonVisibleProcessCleanupDelay);
}
    
void WebProcess::nonVisibleProcessCleanupTimerFired()
{
    ASSERT(m_pagesInWindows.isEmpty());
    if (!m_pagesInWindows.isEmpty())
        return;

#if PLATFORM(COCOA)
    destroyRenderingResources();
#endif
}

void WebProcess::setResourceLoadStatisticsEnabled(bool enabled)
{
    WebCore::DeprecatedGlobalSettings::setResourceLoadStatisticsEnabled(enabled);
}

void WebProcess::clearResourceLoadStatistics()
{
    ResourceLoadObserver::shared().clearState();
}

RefPtr<API::Object> WebProcess::transformHandlesToObjects(API::Object* object)
{
    struct Transformer final : UserData::Transformer {
        Transformer(WebProcess& webProcess)
            : m_webProcess(webProcess)
        {
        }

        bool shouldTransformObject(const API::Object& object) const override
        {
            switch (object.type()) {
            case API::Object::Type::FrameHandle:
                return static_cast<const API::FrameHandle&>(object).isAutoconverting();

            case API::Object::Type::PageHandle:
                return static_cast<const API::PageHandle&>(object).isAutoconverting();

            case API::Object::Type::PageGroupHandle:
#if PLATFORM(COCOA)
            case API::Object::Type::ObjCObjectGraph:
#endif
                return true;

            default:
                return false;
            }
        }

        RefPtr<API::Object> transformObject(API::Object& object) const override
        {
            switch (object.type()) {
            case API::Object::Type::FrameHandle:
                return m_webProcess.webFrame(static_cast<const API::FrameHandle&>(object).frameID());

            case API::Object::Type::PageGroupHandle:
                return m_webProcess.webPageGroup(static_cast<const API::PageGroupHandle&>(object).webPageGroupData());

            case API::Object::Type::PageHandle:
                return m_webProcess.webPage(static_cast<const API::PageHandle&>(object).pageID());

#if PLATFORM(COCOA)
            case API::Object::Type::ObjCObjectGraph:
                return m_webProcess.transformHandlesToObjects(static_cast<ObjCObjectGraph&>(object));
#endif
            default:
                return &object;
            }
        }

        WebProcess& m_webProcess;
    };

    return UserData::transform(object, Transformer(*this));
}

RefPtr<API::Object> WebProcess::transformObjectsToHandles(API::Object* object)
{
    struct Transformer final : UserData::Transformer {
        bool shouldTransformObject(const API::Object& object) const override
        {
            switch (object.type()) {
            case API::Object::Type::BundleFrame:
            case API::Object::Type::BundlePage:
            case API::Object::Type::BundlePageGroup:
#if PLATFORM(COCOA)
            case API::Object::Type::ObjCObjectGraph:
#endif
                return true;

            default:
                return false;
            }
        }

        RefPtr<API::Object> transformObject(API::Object& object) const override
        {
            switch (object.type()) {
            case API::Object::Type::BundleFrame:
                return API::FrameHandle::createAutoconverting(static_cast<const WebFrame&>(object).frameID());

            case API::Object::Type::BundlePage:
                return API::PageHandle::createAutoconverting(static_cast<const WebPage&>(object).pageID());

            case API::Object::Type::BundlePageGroup: {
                WebPageGroupData pageGroupData;
                pageGroupData.pageGroupID = static_cast<const WebPageGroupProxy&>(object).pageGroupID();

                return API::PageGroupHandle::create(WTFMove(pageGroupData));
            }

#if PLATFORM(COCOA)
            case API::Object::Type::ObjCObjectGraph:
                return transformObjectsToHandles(static_cast<ObjCObjectGraph&>(object));
#endif

            default:
                return &object;
            }
        }
    };

    return UserData::transform(object, Transformer());
}

void WebProcess::setMemoryCacheDisabled(bool disabled)
{
    auto& memoryCache = MemoryCache::singleton();
    if (memoryCache.disabled() != disabled)
        memoryCache.setDisabled(disabled);
}

#if ENABLE(SERVICE_CONTROLS)
void WebProcess::setEnabledServices(bool hasImageServices, bool hasSelectionServices, bool hasRichContentServices)
{
    m_hasImageServices = hasImageServices;
    m_hasSelectionServices = hasSelectionServices;
    m_hasRichContentServices = hasRichContentServices;
}
#endif

void WebProcess::ensureAutomationSessionProxy(const String& sessionIdentifier)
{
    m_automationSessionProxy = std::make_unique<WebAutomationSessionProxy>(sessionIdentifier);
}

void WebProcess::destroyAutomationSessionProxy()
{
    m_automationSessionProxy = nullptr;
}

void WebProcess::prefetchDNS(const String& hostname)
{
    if (hostname.isEmpty())
        return;

    if (m_dnsPrefetchedHosts.add(hostname).isNewEntry)
        ensureNetworkProcessConnection().connection().send(Messages::NetworkConnectionToWebProcess::PrefetchDNS(hostname), 0);
    // The DNS prefetched hosts cache is only to avoid asking for the same hosts too many times
    // in a very short period of time, producing a lot of IPC traffic. So we clear this cache after
    // some time of no DNS requests.
    m_dnsPrefetchHystereris.impulse();
}

bool WebProcess::hasVisibleWebPage() const
{
    for (auto& page : m_pageMap.values()) {
        if (page->isVisible())
            return true;
    }
    return false;
}

#if USE(LIBWEBRTC)
LibWebRTCNetwork& WebProcess::libWebRTCNetwork()
{
    if (!m_libWebRTCNetwork)
        m_libWebRTCNetwork = std::make_unique<LibWebRTCNetwork>();
    return *m_libWebRTCNetwork;
}
#endif

#if ENABLE(SERVICE_WORKER)
void WebProcess::establishWorkerContextConnectionToStorageProcess(uint64_t pageGroupID, uint64_t pageID, const WebPreferencesStore& store, PAL::SessionID initialSessionID)
{
    // We are in the Service Worker context process and the call below establishes our connection to the Storage Process
    // by calling webToStorageProcessConnection. SWContextManager needs to use the same underlying IPC::Connection as the
    // WebToStorageProcessConnection for synchronization purposes.
    auto& ipcConnection = ensureWebToStorageProcessConnection(initialSessionID).connection();
    SWContextManager::singleton().setConnection(std::make_unique<WebSWContextManagerConnection>(ipcConnection, pageGroupID, pageID, store));
}

void WebProcess::registerServiceWorkerClients(PAL::SessionID sessionID)
{
    ServiceWorkerProvider::singleton().registerServiceWorkerClients(sessionID);
}

#endif

} // namespace WebKit
