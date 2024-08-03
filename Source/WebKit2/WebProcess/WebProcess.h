/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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

#ifndef WebProcess_h
#define WebProcess_h

#include "CacheModel.h"
#include "ChildProcess.h"
#include "DrawingArea.h"
#include "PluginProcessConnectionManager.h"
#include "ResourceCachesToClear.h"
#include "SandboxExtension.h"
#include "SharedMemory.h"
#include "TextCheckerState.h"
#include "ViewUpdateDispatcher.h"
#include "VisitedLinkTable.h"
#include <WebCore/HysteresisActivity.h>
#include <WebCore/ResourceLoadStatisticsStore.h>
#include <WebCore/SessionID.h>
#include <WebCore/Timer.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/AtomicStringHash.h>

#if PLATFORM(COCOA)
#include <dispatch/dispatch.h>
#include <WebCore/MachSendRight.h>
#endif

#if PLATFORM(IOS)
#include "WebSQLiteDatabaseTracker.h"
#endif

namespace API {
class Object;
}

namespace WebCore {
class ApplicationCacheStorage;
class CertificateInfo;
class PageGroup;
class ResourceRequest;
class SessionID;
class UserGestureToken;
struct PluginInfo;
struct SecurityOriginData;
}

namespace WebKit {

class EventDispatcher;
class InjectedBundle;
class NetworkProcessConnection;
class ObjCObjectGraph;
class UserData;
class WebAutomationSessionProxy;
class WebConnectionToUIProcess;
class WebFrame;
class WebIconDatabaseProxy;
class WebLoaderStrategy;
class WebPage;
class WebPageGroupProxy;
class WebProcessSupplement;
enum class WebsiteDataType;
class GamepadData;
struct WebPageCreationParameters;
struct WebPageGroupData;
struct WebPreferencesStore;
struct WebProcessCreationParameters;

#if ENABLE(DATABASE_PROCESS)
class WebToDatabaseProcessConnection;
#endif

class WebProcess : public ChildProcess {
public:
    static WebProcess& singleton();

    template <typename T>
    T* supplement()
    {
        return static_cast<T*>(m_supplements.get(T::supplementName()));
    }

    template <typename T>
    void addSupplement()
    {
        m_supplements.add(T::supplementName(), std::make_unique<T>(this));
    }

    WebConnectionToUIProcess* webConnectionToUIProcess() const { return m_webConnection.get(); }

    WebPage* webPage(uint64_t pageID) const;
    void createWebPage(uint64_t pageID, const WebPageCreationParameters&);
    void removeWebPage(uint64_t pageID);
    WebPage* focusedWebPage() const;

    InjectedBundle* injectedBundle() const { return m_injectedBundle.get(); }

#if PLATFORM(COCOA)
    const WebCore::MachSendRight& compositingRenderServerPort() const { return m_compositingRenderServerPort; }
#endif

    bool shouldPlugInAutoStartFromOrigin(WebPage&, const String& pageOrigin, const String& pluginOrigin, const String& mimeType);
    void plugInDidStartFromOrigin(const String& pageOrigin, const String& pluginOrigin, const String& mimeType, WebCore::SessionID);
    void plugInDidReceiveUserInteraction(const String& pageOrigin, const String& pluginOrigin, const String& mimeType, WebCore::SessionID);
    void setPluginLoadClientPolicy(uint8_t policy, const String& host, const String& bundleIdentifier, const String& versionString);
    void clearPluginClientPolicies();

    bool fullKeyboardAccessEnabled() const { return m_fullKeyboardAccessEnabled; }

    WebFrame* webFrame(uint64_t) const;
    void addWebFrame(uint64_t, WebFrame*);
    void removeWebFrame(uint64_t);

    WebPageGroupProxy* webPageGroup(WebCore::PageGroup*);
    WebPageGroupProxy* webPageGroup(uint64_t pageGroupID);
    WebPageGroupProxy* webPageGroup(const WebPageGroupData&);

    uint64_t userGestureTokenIdentifier(RefPtr<WebCore::UserGestureToken>);
    void userGestureTokenDestroyed(WebCore::UserGestureToken&);

#if PLATFORM(COCOA)
    pid_t presenterApplicationPid() const { return m_presenterApplicationPid; }
#endif
    
    const TextCheckerState& textCheckerState() const { return m_textCheckerState; }
    void setTextCheckerState(const TextCheckerState&);

    void clearResourceCaches(ResourceCachesToClear = AllResourceCaches);
    
#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginProcessConnectionManager& pluginProcessConnectionManager();
#endif

    EventDispatcher& eventDispatcher() { return *m_eventDispatcher; }

    NetworkProcessConnection& networkConnection();
    void networkProcessConnectionClosed(NetworkProcessConnection*);
    WebLoaderStrategy& webLoaderStrategy();

#if ENABLE(DATABASE_PROCESS)
    void webToDatabaseProcessConnectionClosed(WebToDatabaseProcessConnection*);
    WebToDatabaseProcessConnection* webToDatabaseProcessConnection();
#endif

    void setCacheModel(uint32_t);

    void ensurePrivateBrowsingSession(WebCore::SessionID);
    void destroyPrivateBrowsingSession(WebCore::SessionID);
    void ensureLegacyPrivateBrowsingSessionInNetworkProcess();

    void pageDidEnterWindow(uint64_t pageID);
    void pageWillLeaveWindow(uint64_t pageID);

    void nonVisibleProcessCleanupTimerFired();
    void statisticsChangedTimerFired();

#if PLATFORM(COCOA)
    RetainPtr<CFDataRef> sourceApplicationAuditData() const;
    void destroyRenderingResources();
#endif

    void updateActivePages();

    void setHiddenPageTimerThrottlingIncreaseLimit(int milliseconds);

    void processWillSuspendImminently(bool& handled);
    void prepareToSuspend();
    void cancelPrepareToSuspend();
    void processDidResume();

#if PLATFORM(IOS)
    void resetAllGeolocationPermissions();
#endif

#if PLATFORM(WAYLAND)
    String waylandCompositorDisplayName() const { return m_waylandCompositorDisplayName; }
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

private:
    WebProcess();
    ~WebProcess();

    void initializeWebProcess(WebProcessCreationParameters&&);
    void platformInitializeWebProcess(WebProcessCreationParameters&&);

#if USE(OS_STATE)
    void registerWithStateDumper();
#endif

    void markAllLayersVolatile(std::function<void()> completionHandler);
    void cancelMarkAllLayersVolatile();
    void setAllLayerTreeStatesFrozen(bool);
    void processSuspensionCleanupTimerFired();

    void clearCachedCredentials();

    void platformTerminate();
    void registerURLSchemeAsEmptyDocument(const String&);
    void registerURLSchemeAsSecure(const String&) const;
    void registerURLSchemeAsBypassingContentSecurityPolicy(const String&) const;
    void setDomainRelaxationForbiddenForURLScheme(const String&) const;
    void registerURLSchemeAsLocal(const String&) const;
    void registerURLSchemeAsNoAccess(const String&) const;
    void registerURLSchemeAsDisplayIsolated(const String&) const;
    void registerURLSchemeAsCORSEnabled(const String&) const;
    void registerURLSchemeAsAlwaysRevalidated(const String&) const;
#if ENABLE(CACHE_PARTITIONING)
    void registerURLSchemeAsCachePartitioned(const String&) const;
#endif
    void setDefaultRequestTimeoutInterval(double);
    void setAlwaysUsesComplexTextCodePath(bool);
    void setShouldUseFontSmoothing(bool);
    void setResourceLoadStatisticsEnabled(bool);
    void userPreferredLanguagesChanged(const Vector<String>&) const;
    void fullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled);

    bool isPlugInAutoStartOriginHash(unsigned plugInOriginHash, WebCore::SessionID);
    void didAddPlugInAutoStartOriginHash(unsigned plugInOriginHash, double expirationTime, WebCore::SessionID);
    void resetPlugInAutoStartOriginDefaultHashes(const HashMap<unsigned, double>& hashes);
    void resetPlugInAutoStartOriginHashes(const HashMap<WebCore::SessionID, HashMap<unsigned, double>>& hashes);

    void platformSetCacheModel(CacheModel);

    void setEnhancedAccessibility(bool);
    
    void startMemorySampler(const SandboxExtension::Handle&, const String&, const double);
    void stopMemorySampler();
    
    void getWebCoreStatistics(uint64_t callbackID);
    void garbageCollectJavaScriptObjects();
    void setJavaScriptGarbageCollectorTimerEnabled(bool flag);

    void mainThreadPing();

#if ENABLE(GAMEPAD)
    void setInitialGamepads(const Vector<GamepadData>&);
    void gamepadConnected(const GamepadData&);
    void gamepadDisconnected(unsigned index);
#endif

    void releasePageCache();

    void fetchWebsiteData(WebCore::SessionID, OptionSet<WebsiteDataType>, uint64_t callbackID);
    void deleteWebsiteData(WebCore::SessionID, OptionSet<WebsiteDataType>, std::chrono::system_clock::time_point modifiedSince, uint64_t callbackID);
    void deleteWebsiteDataForOrigins(WebCore::SessionID, OptionSet<WebsiteDataType>, const Vector<WebCore::SecurityOriginData>& origins, uint64_t callbackID);

    void setMemoryCacheDisabled(bool);

#if ENABLE(SERVICE_CONTROLS)
    void setEnabledServices(bool hasImageServices, bool hasSelectionServices, bool hasRichContentServices);
#endif

    void handleInjectedBundleMessage(const String& messageName, const UserData& messageBody);
    void setInjectedBundleParameter(const String& key, const IPC::DataReference&);
    void setInjectedBundleParameters(const IPC::DataReference&);

    enum class ShouldAcknowledgeWhenReadyToSuspend { No, Yes };
    void actualPrepareToSuspend(ShouldAcknowledgeWhenReadyToSuspend);

    void ensureAutomationSessionProxy(const String& sessionIdentifier);
    void destroyAutomationSessionProxy();

    // ChildProcess
    void initializeProcess(const ChildProcessInitializationParameters&) override;
    void initializeProcessName(const ChildProcessInitializationParameters&) override;
    void initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&) override;
    void initializeConnection(IPC::Connection*) override;
    bool shouldTerminate() override;
    void terminate() override;

#if USE(APPKIT)
    void stopRunLoop() override;
#endif

    void platformInitializeProcess(const ChildProcessInitializationParameters&);

    // IPC::Connection::Client
    friend class WebConnectionToUIProcess;
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;
    void didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference messageReceiverName, IPC::StringReference messageName) override;

    // Implemented in generated WebProcessMessageReceiver.cpp
    void didReceiveWebProcessMessage(IPC::Connection&, IPC::Decoder&);
    void didReceiveSyncWebProcessMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);

    RefPtr<WebConnectionToUIProcess> m_webConnection;

    HashMap<uint64_t, RefPtr<WebPage>> m_pageMap;
    HashMap<uint64_t, RefPtr<WebPageGroupProxy>> m_pageGroupMap;
    RefPtr<InjectedBundle> m_injectedBundle;

    RefPtr<EventDispatcher> m_eventDispatcher;
#if PLATFORM(IOS)
    RefPtr<ViewUpdateDispatcher> m_viewUpdateDispatcher;
#endif

    bool m_inDidClose;

    HashMap<WebCore::SessionID, HashMap<unsigned, double>> m_plugInAutoStartOriginHashes;
    HashSet<String> m_plugInAutoStartOrigins;

    bool m_hasSetCacheModel;
    CacheModel m_cacheModel;

#if PLATFORM(COCOA)
    WebCore::MachSendRight m_compositingRenderServerPort;
    pid_t m_presenterApplicationPid;
#endif

    bool m_fullKeyboardAccessEnabled;

    HashMap<uint64_t, WebFrame*> m_frameMap;

    typedef HashMap<const char*, std::unique_ptr<WebProcessSupplement>, PtrHash<const char*>> WebProcessSupplementMap;
    WebProcessSupplementMap m_supplements;

    TextCheckerState m_textCheckerState;

    WebIconDatabaseProxy& m_iconDatabaseProxy;

    void ensureNetworkProcessConnection();
    RefPtr<NetworkProcessConnection> m_networkProcessConnection;
    WebLoaderStrategy& m_webLoaderStrategy;
    HashSet<String> m_dnsPrefetchedHosts;
    WebCore::HysteresisActivity m_dnsPrefetchHystereris;

    std::unique_ptr<WebAutomationSessionProxy> m_automationSessionProxy;

#if ENABLE(DATABASE_PROCESS)
    void ensureWebToDatabaseProcessConnection();
    RefPtr<WebToDatabaseProcessConnection> m_webToDatabaseProcessConnection;
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    RefPtr<PluginProcessConnectionManager> m_pluginProcessConnectionManager;
#endif

#if ENABLE(SERVICE_CONTROLS)
    bool m_hasImageServices;
    bool m_hasSelectionServices;
    bool m_hasRichContentServices;
#endif

    HashSet<uint64_t> m_pagesInWindows;
    WebCore::Timer m_nonVisibleProcessCleanupTimer;
    WebCore::Timer m_statisticsChangedTimer;

    RefPtr<WebCore::ApplicationCacheStorage> m_applicationCacheStorage;

#if PLATFORM(IOS)
    WebSQLiteDatabaseTracker m_webSQLiteDatabaseTracker;
#endif

    Ref<WebCore::ResourceLoadStatisticsStore> m_resourceLoadStatisticsStorage;

    unsigned m_pagesMarkingLayersAsVolatile { 0 };
    bool m_suppressMemoryPressureHandler { false };

    HashMap<WebCore::UserGestureToken *, uint64_t> m_userGestureTokens;

#if PLATFORM(WAYLAND)
    String m_waylandCompositorDisplayName;
#endif
};

} // namespace WebKit

#endif // WebProcess_h
