/*
 * Copyright (C) 2010-2018 Apple Inc. All rights reserved.
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
#if ENABLE(MEDIA_STREAM)
#include "MediaDeviceSandboxExtensions.h"
#endif
#include "PluginProcessConnectionManager.h"
#include "ResourceCachesToClear.h"
#include "SandboxExtension.h"
#include "StorageAreaIdentifier.h"
#include "TextCheckerState.h"
#include "ViewUpdateDispatcher.h"
#include "WebInspectorInterruptDispatcher.h"
#include "WebProcessCreationParameters.h"
#include "WebSQLiteDatabaseTracker.h"
#include "WebSocketChannelManager.h"
#include <WebCore/ActivityState.h>
#include <WebCore/FrameIdentifier.h>
#include <WebCore/PageIdentifier.h>
#include <WebCore/RegistrableDomain.h>
#if PLATFORM(MAC)
#include <WebCore/ScreenProperties.h>
#endif
#include <WebCore/Timer.h>
#include <pal/HysteresisActivity.h>
#include <pal/SessionID.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefCounter.h>
#include <wtf/text/AtomString.h>
#include <wtf/text/AtomStringHash.h>

#if PLATFORM(COCOA)
#include <dispatch/dispatch.h>
#include <wtf/MachSendRight.h>
#endif

#if PLATFORM(IOS_FAMILY)
#include "ProcessTaskStateObserver.h"
#endif

#if PLATFORM(WAYLAND) && USE(WPE_RENDERER)
#include <WebCore/PlatformDisplayLibWPE.h>
#endif

namespace API {
class Object;
}

namespace PAL {
class SessionID;
}

namespace WebCore {
class ApplicationCacheStorage;
class CPUMonitor;
class CertificateInfo;
class PageGroup;
class ResourceRequest;
class UserGestureToken;
struct MessagePortIdentifier;
struct MessageWithMessagePorts;
struct MockMediaDevice;
struct PluginInfo;
struct PrewarmInformation;
struct SecurityOriginData;

#if ENABLE(SERVICE_WORKER)
struct ServiceWorkerContextData;
#endif
}

namespace WebKit {

class EventDispatcher;
class GamepadData;
class InjectedBundle;
class LibWebRTCNetwork;
class NetworkProcessConnection;
class ObjCObjectGraph;
class StorageAreaMap;
class UserData;
class WaylandCompositorDisplay;
class WebAutomationSessionProxy;
class WebCacheStorageProvider;
class WebConnectionToUIProcess;
class WebFrame;
class WebLoaderStrategy;
class WebPage;
class WebPageGroupProxy;
struct WebProcessCreationParameters;
struct WebProcessDataStoreParameters;
class WebProcessSupplement;
enum class WebsiteDataType;
struct WebPageCreationParameters;
struct WebPageGroupData;
struct WebPreferencesStore;
struct WebsiteData;
struct WebsiteDataStoreParameters;

#if PLATFORM(IOS_FAMILY)
class LayerHostingContext;
#endif

class WebProcess
    : public AuxiliaryProcess
#if PLATFORM(IOS_FAMILY)
    , ProcessTaskStateObserver::Client
#endif
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static WebProcess& singleton();
    static constexpr ProcessType processType = ProcessType::WebContent;

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

    WebConnectionToUIProcess* webConnectionToUIProcess() const { return m_webConnection.get(); }

    WebPage* webPage(WebCore::PageIdentifier) const;
    void createWebPage(WebCore::PageIdentifier, WebPageCreationParameters&&);
    void removeWebPage(PAL::SessionID, WebCore::PageIdentifier);
    WebPage* focusedWebPage() const;

    InjectedBundle* injectedBundle() const { return m_injectedBundle.get(); }

#if PLATFORM(COCOA)
    const WTF::MachSendRight& compositingRenderServerPort() const { return m_compositingRenderServerPort; }
#endif

    bool shouldPlugInAutoStartFromOrigin(WebPage&, const String& pageOrigin, const String& pluginOrigin, const String& mimeType);
    void plugInDidStartFromOrigin(const String& pageOrigin, const String& pluginOrigin, const String& mimeType, PAL::SessionID);
    void plugInDidReceiveUserInteraction(const String& pageOrigin, const String& pluginOrigin, const String& mimeType, PAL::SessionID);
    void setPluginLoadClientPolicy(uint8_t policy, const String& host, const String& bundleIdentifier, const String& versionString);
    void resetPluginLoadClientPolicies(const HashMap<String, HashMap<String, HashMap<String, uint8_t>>>&);
    void clearPluginClientPolicies();
    void refreshPlugins();

    bool fullKeyboardAccessEnabled() const { return m_fullKeyboardAccessEnabled; }

    WebFrame* webFrame(WebCore::FrameIdentifier) const;
    void addWebFrame(WebCore::FrameIdentifier, WebFrame*);
    void removeWebFrame(WebCore::FrameIdentifier);

    WebPageGroupProxy* webPageGroup(WebCore::PageGroup*);
    WebPageGroupProxy* webPageGroup(uint64_t pageGroupID);
    WebPageGroupProxy* webPageGroup(const WebPageGroupData&);

    uint64_t userGestureTokenIdentifier(RefPtr<WebCore::UserGestureToken>);
    void userGestureTokenDestroyed(WebCore::UserGestureToken&);
    
    const TextCheckerState& textCheckerState() const { return m_textCheckerState; }
    void setTextCheckerState(const TextCheckerState&);

    void clearResourceCaches(ResourceCachesToClear = AllResourceCaches);
    
#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginProcessConnectionManager& pluginProcessConnectionManager();
#endif

    EventDispatcher& eventDispatcher() { return m_eventDispatcher.get(); }

    NetworkProcessConnection& ensureNetworkProcessConnection();
    void networkProcessConnectionClosed(NetworkProcessConnection*);
    NetworkProcessConnection* existingNetworkProcessConnection() { return m_networkProcessConnection.get(); }
    WebLoaderStrategy& webLoaderStrategy();

    LibWebRTCNetwork& libWebRTCNetwork();

    void setCacheModel(CacheModel);

    void ensureLegacyPrivateBrowsingSessionInNetworkProcess();

    void pageDidEnterWindow(WebCore::PageIdentifier);
    void pageWillLeaveWindow(WebCore::PageIdentifier);

    void nonVisibleProcessCleanupTimerFired();

    void registerStorageAreaMap(StorageAreaMap&);
    void unregisterStorageAreaMap(StorageAreaMap&);
    StorageAreaMap* storageAreaMap(StorageAreaIdentifier) const;

#if PLATFORM(COCOA)
    RetainPtr<CFDataRef> sourceApplicationAuditData() const;
    void destroyRenderingResources();
#endif

    const String& uiProcessBundleIdentifier() const { return m_uiProcessBundleIdentifier; }

    void updateActivePages();
    void getActivePagesOriginsForTesting(CompletionHandler<void(Vector<String>&&)>&&);
    void pageActivityStateDidChange(WebCore::PageIdentifier, OptionSet<WebCore::ActivityState::Flag> changed);

    void setHiddenPageDOMTimerThrottlingIncreaseLimit(int milliseconds);

    void processWillSuspendImminently();
    void prepareToSuspend();
    void cancelPrepareToSuspend();
    void processDidResume();

    void sendPrewarmInformation(const URL&);

    void isJITEnabled(CompletionHandler<void(bool)>&&);

#if PLATFORM(IOS_FAMILY)
    void resetAllGeolocationPermissions();
#endif

#if PLATFORM(WAYLAND)
    WaylandCompositorDisplay* waylandCompositorDisplay() const { return m_waylandCompositorDisplay.get(); }
#endif

    RefPtr<API::Object> transformHandlesToObjects(API::Object*);
    static RefPtr<API::Object> transformObjectsToHandles(API::Object*);

#if PLATFORM(COCOA)
    RefPtr<ObjCObjectGraph> transformHandlesToObjects(ObjCObjectGraph&);
    static RefPtr<ObjCObjectGraph> transformObjectsToHandles(ObjCObjectGraph&);
#endif

#if ENABLE(SERVICE_CONTROLS)
    bool hasImageServices() const { return m_hasImageServices; }
    bool hasSelectionServices() const { return m_hasSelectionServices; }
    bool hasRichContentServices() const { return m_hasRichContentServices; }
#endif

    WebCore::ApplicationCacheStorage& applicationCacheStorage() { return *m_applicationCacheStorage; }

    void prefetchDNS(const String&);

    WebAutomationSessionProxy* automationSessionProxy() { return m_automationSessionProxy.get(); }

    WebCacheStorageProvider& cacheStorageProvider() { return m_cacheStorageProvider.get(); }
    WebSocketChannelManager& webSocketChannelManager() { return m_webSocketChannelManager; }

#if PLATFORM(IOS_FAMILY)
    void accessibilityProcessSuspendedNotification(bool);
    
    void unblockAccessibilityServer(const SandboxExtension::Handle&);
#endif

#if PLATFORM(IOS)
    float backlightLevel() const { return m_backlightLevel; }
#endif

#if PLATFORM(COCOA)
    void setMediaMIMETypes(const Vector<String>);
#endif

    bool areAllPagesThrottleable() const;

private:
    WebProcess();
    ~WebProcess();

    void initializeWebProcess(WebProcessCreationParameters&&);
    void platformInitializeWebProcess(WebProcessCreationParameters&);
    void setWebsiteDataStoreParameters(WebProcessDataStoreParameters&&);
    void platformSetWebsiteDataStoreParameters(WebProcessDataStoreParameters&&);

    void prewarmGlobally();
    void prewarmWithDomainInformation(const WebCore::PrewarmInformation&);

#if USE(OS_STATE)
    void registerWithStateDumper();
#endif

    void markAllLayersVolatile(WTF::Function<void(bool)>&& completionHandler);
    void cancelMarkAllLayersVolatile();

    void freezeAllLayerTrees();
    void unfreezeAllLayerTrees();

    void processSuspensionCleanupTimerFired();

    void platformTerminate();

    void setHasSuspendedPageProxy(bool);
    void setIsInProcessCache(bool);
    void markIsNoLongerPrewarmed();

    void registerURLSchemeAsEmptyDocument(const String&);
    void registerURLSchemeAsSecure(const String&) const;
    void registerURLSchemeAsBypassingContentSecurityPolicy(const String&) const;
    void setDomainRelaxationForbiddenForURLScheme(const String&) const;
    void registerURLSchemeAsLocal(const String&) const;
    void registerURLSchemeAsNoAccess(const String&) const;
    void registerURLSchemeAsDisplayIsolated(const String&) const;
    void registerURLSchemeAsCORSEnabled(const String&) const;
    void registerURLSchemeAsAlwaysRevalidated(const String&) const;
    void registerURLSchemeAsCachePartitioned(const String&) const;
    void registerURLSchemeAsCanDisplayOnlyIfCanRequest(const String&) const;

    void setDefaultRequestTimeoutInterval(double);
    void setAlwaysUsesComplexTextCodePath(bool);
    void setShouldUseFontSmoothing(bool);
    void setResourceLoadStatisticsEnabled(bool);
    void clearResourceLoadStatistics();
    void userPreferredLanguagesChanged(const Vector<String>&) const;
    void fullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled);

    bool isPlugInAutoStartOriginHash(unsigned plugInOriginHash, PAL::SessionID);
    void didAddPlugInAutoStartOriginHash(unsigned plugInOriginHash, WallTime expirationTime, PAL::SessionID);
    void resetPlugInAutoStartOriginDefaultHashes(const HashMap<unsigned, WallTime>& hashes);
    void resetPlugInAutoStartOriginHashes(const HashMap<PAL::SessionID, HashMap<unsigned, WallTime>>& hashes);

    void platformSetCacheModel(CacheModel);

    void setEnhancedAccessibility(bool);
    
    void startMemorySampler(SandboxExtension::Handle&&, const String&, const double);
    void stopMemorySampler();
    
    void getWebCoreStatistics(uint64_t callbackID);
    void garbageCollectJavaScriptObjects();
    void setJavaScriptGarbageCollectorTimerEnabled(bool flag);

    void mainThreadPing();
    void backgroundResponsivenessPing();

    void didTakeAllMessagesForPort(Vector<WebCore::MessageWithMessagePorts>&& messages, uint64_t messageCallbackIdentifier, uint64_t messageBatchIdentifier);
    void checkProcessLocalPortForActivity(const WebCore::MessagePortIdentifier&, uint64_t callbackIdentifier);
    void didCheckRemotePortForActivity(uint64_t callbackIdentifier, bool hasActivity);
    void messagesAvailableForPort(const WebCore::MessagePortIdentifier&);

#if ENABLE(GAMEPAD)
    void setInitialGamepads(const Vector<GamepadData>&);
    void gamepadConnected(const GamepadData&);
    void gamepadDisconnected(unsigned index);
#endif

#if ENABLE(SERVICE_WORKER)
    void establishWorkerContextConnectionToNetworkProcess(uint64_t pageGroupID, WebCore::PageIdentifier, const WebPreferencesStore&, PAL::SessionID);
    void registerServiceWorkerClients();
#endif

    void releasePageCache();

    void fetchWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, CompletionHandler<void(WebsiteData&&)>&&);
    void deleteWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, WallTime modifiedSince, CompletionHandler<void()>&&);
    void deleteWebsiteDataForOrigins(PAL::SessionID, OptionSet<WebsiteDataType>, const Vector<WebCore::SecurityOriginData>& origins, CompletionHandler<void()>&&);

    void setMemoryCacheDisabled(bool);

#if ENABLE(SERVICE_CONTROLS)
    void setEnabledServices(bool hasImageServices, bool hasSelectionServices, bool hasRichContentServices);
#endif

    void handleInjectedBundleMessage(const String& messageName, const UserData& messageBody);
    void setInjectedBundleParameter(const String& key, const IPC::DataReference&);
    void setInjectedBundleParameters(const IPC::DataReference&);

    enum class ShouldAcknowledgeWhenReadyToSuspend { No, Yes };
    void actualPrepareToSuspend(ShouldAcknowledgeWhenReadyToSuspend);

    bool hasPageRequiringPageCacheWhileSuspended() const;
    bool areAllPagesSuspended() const;

    void ensureAutomationSessionProxy(const String& sessionIdentifier);
    void destroyAutomationSessionProxy();

    void logDiagnosticMessageForNetworkProcessCrash();
    bool hasVisibleWebPage() const;
    void updateCPULimit();
    enum class CPUMonitorUpdateReason { LimitHasChanged, VisibilityHasChanged };
    void updateCPUMonitorState(CPUMonitorUpdateReason);

    // AuxiliaryProcess
    void initializeProcess(const AuxiliaryProcessInitializationParameters&) override;
    void initializeProcessName(const AuxiliaryProcessInitializationParameters&) override;
    void initializeSandbox(const AuxiliaryProcessInitializationParameters&, SandboxInitializationParameters&) override;
    void initializeConnection(IPC::Connection*) override;
    bool shouldTerminate() override;
    void terminate() override;

#if USE(APPKIT)
    void stopRunLoop() override;
#endif

#if ENABLE(MEDIA_STREAM)
    void addMockMediaDevice(const WebCore::MockMediaDevice&);
    void clearMockMediaDevices();
    void removeMockMediaDevice(const String& persistentId);
    void resetMockMediaDevices();
#if ENABLE(SANDBOX_EXTENSIONS)
    void grantUserMediaDeviceSandboxExtensions(MediaDeviceSandboxExtensions&&);
    void revokeUserMediaDeviceSandboxExtensions(const Vector<String>&);
#endif

#endif

    void platformInitializeProcess(const AuxiliaryProcessInitializationParameters&);

    // IPC::Connection::Client
    friend class WebConnectionToUIProcess;
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;

    // Implemented in generated WebProcessMessageReceiver.cpp
    void didReceiveWebProcessMessage(IPC::Connection&, IPC::Decoder&);

#if PLATFORM(MAC)
    void setScreenProperties(const WebCore::ScreenProperties&);
#if ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    void scrollerStylePreferenceChanged(bool useOverlayScrollbars);
    void displayConfigurationChanged(CGDirectDisplayID, CGDisplayChangeSummaryFlags);
    void displayWasRefreshed(CGDirectDisplayID);
#endif
#endif

#if PLATFORM(COCOA)
    void updateProcessName();
#endif

#if PLATFORM(IOS)
    void backlightLevelDidChange(float backlightLevel);
#endif

#if PLATFORM(IOS_FAMILY)
    void processTaskStateDidChange(ProcessTaskStateObserver::TaskState) final;
    bool shouldFreezeOnSuspension() const;
    void updateFreezerStatus();
#endif

#if ENABLE(VIDEO)
    void suspendAllMediaBuffering();
    void resumeAllMediaBuffering();
#endif

    void clearCurrentModifierStateForTesting();

    RefPtr<WebConnectionToUIProcess> m_webConnection;

    HashMap<WebCore::PageIdentifier, RefPtr<WebPage>> m_pageMap;
    HashMap<uint64_t, RefPtr<WebPageGroupProxy>> m_pageGroupMap;
    RefPtr<InjectedBundle> m_injectedBundle;

    Ref<EventDispatcher> m_eventDispatcher;
#if PLATFORM(IOS_FAMILY)
    RefPtr<ViewUpdateDispatcher> m_viewUpdateDispatcher;
#endif
    RefPtr<WebInspectorInterruptDispatcher> m_webInspectorInterruptDispatcher;

    HashMap<PAL::SessionID, HashMap<unsigned, WallTime>> m_plugInAutoStartOriginHashes;
    HashSet<String> m_plugInAutoStartOrigins;

    bool m_hasSetCacheModel { false };
    CacheModel m_cacheModel { CacheModel::DocumentViewer };

#if PLATFORM(COCOA)
    WTF::MachSendRight m_compositingRenderServerPort;
#endif

    bool m_fullKeyboardAccessEnabled { false };

    HashMap<WebCore::FrameIdentifier, WebFrame*> m_frameMap;

    typedef HashMap<const char*, std::unique_ptr<WebProcessSupplement>, PtrHash<const char*>> WebProcessSupplementMap;
    WebProcessSupplementMap m_supplements;

    TextCheckerState m_textCheckerState;

    String m_uiProcessBundleIdentifier;
    RefPtr<NetworkProcessConnection> m_networkProcessConnection;
    WebLoaderStrategy& m_webLoaderStrategy;

    Ref<WebCacheStorageProvider> m_cacheStorageProvider;
    WebSocketChannelManager m_webSocketChannelManager;

    std::unique_ptr<LibWebRTCNetwork> m_libWebRTCNetwork;

    HashSet<String> m_dnsPrefetchedHosts;
    PAL::HysteresisActivity m_dnsPrefetchHystereris;

    std::unique_ptr<WebAutomationSessionProxy> m_automationSessionProxy;

#if ENABLE(NETSCAPE_PLUGIN_API)
    RefPtr<PluginProcessConnectionManager> m_pluginProcessConnectionManager;
#endif

#if ENABLE(SERVICE_CONTROLS)
    bool m_hasImageServices { false };
    bool m_hasSelectionServices { false };
    bool m_hasRichContentServices { false };
#endif

    bool m_processIsSuspended { false };

    HashSet<WebCore::PageIdentifier> m_pagesInWindows;
    WebCore::Timer m_nonVisibleProcessCleanupTimer;

    RefPtr<WebCore::ApplicationCacheStorage> m_applicationCacheStorage;

#if PLATFORM(IOS_FAMILY)
    WebSQLiteDatabaseTracker m_webSQLiteDatabaseTracker;
    ProcessTaskStateObserver m_taskStateObserver { *this };
#endif

    enum PageMarkingLayersAsVolatileCounterType { };
    using PageMarkingLayersAsVolatileCounter = RefCounter<PageMarkingLayersAsVolatileCounterType>;
    std::unique_ptr<PageMarkingLayersAsVolatileCounter> m_pageMarkingLayersAsVolatileCounter;
    unsigned m_countOfPagesFailingToMarkVolatile { 0 };

    bool m_suppressMemoryPressureHandler { false };
#if PLATFORM(MAC)
    std::unique_ptr<WebCore::CPUMonitor> m_cpuMonitor;
    Optional<double> m_cpuLimit;

    String m_uiProcessName;
    WebCore::RegistrableDomain m_registrableDomain;
#endif

#if PLATFORM(COCOA)
    enum class ProcessType { Inspector, ServiceWorker, PrewarmedWebContent, CachedWebContent, WebContent };
    ProcessType m_processType { ProcessType::WebContent };
#endif

    HashMap<WebCore::UserGestureToken *, uint64_t> m_userGestureTokens;

#if PLATFORM(WAYLAND)
    std::unique_ptr<WaylandCompositorDisplay> m_waylandCompositorDisplay;
#endif

#if PLATFORM(WAYLAND) && USE(WPE_RENDERER)
    std::unique_ptr<WebCore::PlatformDisplayLibWPE> m_wpeDisplay;
#endif

    bool m_hasSuspendedPageProxy { false };
    bool m_isSuspending { false };

#if ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS)
    HashMap<String, RefPtr<SandboxExtension>> m_mediaCaptureSandboxExtensions;
#endif

#if PLATFORM(IOS)
    float m_backlightLevel { 0 };
#endif

    HashMap<StorageAreaIdentifier, StorageAreaMap*> m_storageAreaMaps;
};

} // namespace WebKit
