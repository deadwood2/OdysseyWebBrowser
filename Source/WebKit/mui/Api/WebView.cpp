/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebView.h"

#include "BALBase.h"
#include <wtf/bal/PtrAndFlags.h>
#include "DefaultPolicyDelegate.h"
#include "DOMCoreClasses.h"
#include "JSActionDelegate.h"
#include "WebBackForwardList.h"
#include "WebBackForwardList_p.h"
#include "WebBindingJSDelegate.h"
#include "WebChromeClient.h"
#include "WebContextMenuClient.h"
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "WebDesktopNotificationsDelegate.h"
#endif
#include "WebDatabaseManager.h"
#include "WebDatabaseProvider.h"
#if ENABLE(DEVICE_ORIENTATION)
#include "WebDeviceOrientationClient.h"
#include "WebDeviceMotionClient.h"
#endif
#include "WebDocumentLoader.h"
#include "WebDownloadDelegate.h"
#include "WebDragClient.h"
#include "WebDragData.h"
#include "WebDragData_p.h"
#include "WebEditingDelegate.h"
#include "WebEditorClient.h"
#include "WebFrame.h"
#include "WebFrameLoaderClient.h"
#include "WebFrameLoadDelegate.h"
#include "WebGeolocationClient.h"
#include "WebGeolocationPosition.h"
#include "WebGeolocationProvider.h"
#include "WebHistoryDelegate.h"
#include "WebHitTestResults.h"
#include "WebHistoryItem.h"
#include "WebHistoryItem_p.h"
#if ENABLE(ICONDATABASE)
#include "WebIconDatabase.h"
#endif
#include "WebKitVersion.h"
#include "WebInspector.h"
#include "WebInspectorClient.h"
#include "WebMutableURLRequest.h"
#include "WebNotificationDelegate.h"
#include "WebPreferences.h"
#include "WebProgressTrackerClient.h"
#include "WebResourceLoadDelegate.h"
#include "WebScriptWorld.h"
#include "WebStorageNamespaceProvider.h"
#include "WebViewGroup.h"
#include "WebViewPrivate.h"
#include "WebVisitedLinkStore.h"
#include "WebWidgetEngineDelegate.h"
#include "WebWindow.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
#include <ApplicationCacheStorage.h>
#endif
#if HAVE(ACCESSIBILITY)
#include <AXObjectCache.h>
#endif
#include <BackForwardController.h>
#include <Chrome.h>
#include <ContextMenu.h>
#include <ContextMenuController.h>
#include <CrossOriginPreflightResultCache.h>
#include <Cursor.h>
#include <Database.h>
#include <DatabaseManager.h>
#include <DragController.h>
#include <DragData.h>
#include <Editor.h>
#include <EventHandler.h>
#include <EventNames.h>
#include <FileSystem.h>
#include <FocusController.h>
#include <FrameLoader.h>
#include <FrameTree.h>
#include <FrameView.h>
#include <Frame.h>
#include <GeolocationController.h>
#include <GeolocationError.h>
#include <GraphicsContext.h>
#include <HistoryController.h>
#include <HistoryItem.h>
#include <HitTestResult.h>
#include <IntPoint.h>
#include <IntRect.h>
#include <KeyboardEvent.h>
#include <Logging.h>
#include <MemoryCache.h>
#include <MIMETypeRegistry.h>
#include <NotImplemented.h>
#include <ObserverData.h>
#include <ObserverServiceData.h>
#include <Page.h>
#include <PageConfiguration.h>
#include <PageCache.h>
#include <PageGroup.h>
#include <PlatformKeyboardEvent.h>
#include <PlatformMouseEvent.h>
#include <PlatformWheelEvent.h>
#include <PluginDatabase.h>
#include <PluginData.h>
#include <PluginView.h>
#include <ProgressTracker.h>
#include <RenderTheme.h>
#include <RenderWidget.h>
#include <ResourceHandle.h>
#include <ResourceHandleClient.h>
#include <SchemeRegistry.h>
#include <ScriptController.h> 
#include <ScrollbarTheme.h>
#include <SecurityOrigin.h>
#include <SecurityPolicy.h>
#include <Settings.h>
#include <SubframeLoader.h>
#include <TypingCommand.h>
#include <UserContentController.h>
#include <WindowsKeyboardCodes.h>

#include <JSCell.h>
#include <JSLock.h>
#include <JSValue.h>

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <InitializeThreading.h>

#include <wtf/unicode/icu/EncodingICU.h>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>

#include "owb-config.h"
#include "FileIOLinux.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/gadgetclass.h>
#include "gui.h"
#include "utils.h"
#undef set
#undef get
#undef String
#undef PageGroup

using namespace WebCore;
using std::min;
using std::max;

static char* globalUserAgent = getenv("OWB_USER_AGENT");

#define HOST_NAME_BUFFER_LENGTH 2048

WebView* kit(Page* page)
{
    if (!page)
        return 0;

    if (page->chrome().client().isEmptyChromeClient()) 
        return 0;

    return (WebView *) static_cast<WebChromeClient&>(page->chrome().client()).webView();   
}

class WebViewObserver : public ObserverData {
public:
    WebViewObserver(WebView* webview) : m_webView(webview) {};
    ~WebViewObserver() {m_webView = 0;}

    virtual void observe(const WTF::String &topic, const WTF::String &data, void *userData);
private:
    WebView *m_webView;
};

void WebViewObserver::observe(const WTF::String &topic, const WTF::String &data, void *userData)
{
#if ENABLE(ICONDATABASE)
    if (topic == WebIconDatabase::iconDatabaseDidAddIconNotification())
	m_webView->notifyDidAddIcon();
#endif
    if (topic == WebPreferences::webPreferencesChangedNotification())
        m_webView->notifyPreferencesChanged(static_cast<WebPreferences*>(userData));

    if (topic == "PopupMenuHide")
        m_webView->popupMenuHide();
    if (topic == "PopupMenuShow")
        m_webView->popupMenuShow(userData);
}


class PreferencesChangedOrRemovedObserver : public ObserverData {
public:
    static PreferencesChangedOrRemovedObserver* sharedInstance();

private:
    PreferencesChangedOrRemovedObserver() {}
    virtual ~PreferencesChangedOrRemovedObserver() {}

public:
    virtual void observe(const WTF::String &topic, const WTF::String &data, void *userData);

private:
    void notifyPreferencesChanged(WebCacheModel);
    void notifyPreferencesRemoved(WebCacheModel);
};

PreferencesChangedOrRemovedObserver* PreferencesChangedOrRemovedObserver::sharedInstance()
{
    static PreferencesChangedOrRemovedObserver shared;
    return &shared;
}

void PreferencesChangedOrRemovedObserver::observe(const WTF::String &topic, const WTF::String &data, void *userData)
{
    WebPreferences* preferences = static_cast<WebPreferences*>(userData);
    if (!preferences)
        return;

    WebCacheModel cacheModel = preferences->cacheModel();

    if (topic == WebPreferences::webPreferencesChangedNotification())
        notifyPreferencesChanged(cacheModel);

    if (topic == WebPreferences::webPreferencesRemovedNotification())
        notifyPreferencesRemoved(cacheModel);
}

void PreferencesChangedOrRemovedObserver::notifyPreferencesChanged(WebCacheModel cacheModel)
{
    if (!WebView::didSetCacheModel() || cacheModel > WebView::cacheModel())
        WebView::setCacheModel(cacheModel);
    else if (cacheModel < WebView::cacheModel()) {
        WebCacheModel sharedPreferencesCacheModel = WebPreferences::sharedStandardPreferences()->cacheModel();
        WebView::setCacheModel(sharedPreferencesCacheModel);
    }
}

void PreferencesChangedOrRemovedObserver::notifyPreferencesRemoved(WebCacheModel cacheModel)
{
    if (cacheModel == WebView::cacheModel()) {
        WebCacheModel sharedPreferencesCacheModel = WebPreferences::sharedStandardPreferences()->cacheModel();
        WebView::setCacheModel(sharedPreferencesCacheModel);
    }
}

static const int maxToolTipWidth = 250;

static const int delayBeforeDeletingBackingStoreMsec = 5000;

static void initializeStaticObservers();

static void updateSharedSettingsFromPreferencesIfNeeded(WebPreferences*);

static bool continuousSpellCheckingEnabled;
static bool grammarCheckingEnabled;

static bool s_didSetCacheModel;
static WebCacheModel s_cacheModel = WebCacheModelDocumentViewer;

enum {
    UpdateActiveStateTimer = 1,
    DeleteBackingStoreTimer = 2,
};

bool WebView::s_allowSiteSpecificHacks = false;

#if ENABLE(OFFLINE_WEB_APPLICATIONS)
static void WebKitSetApplicationCachePathIfNecessary()
{
    static bool initialized = false;
    if (initialized)
        return;

	WTF::String path = WebCore::pathByAppendingComponent("PROGDIR:conf", "ApplicationCache");

    if (!path.isNull())
        cacheStorage().setCacheDirectory(path);

    initialized = true;
}
#endif

WebView::WebView()
	: m_viewWindow(0)
    , m_mainFrame(0)
	, m_page(0)
    , m_inspectorClient(0)
    , m_policyDelegate(0)
    , m_downloadDelegate(0)
    , m_webNotificationDelegate(0)
    , m_webFrameLoadDelegate(0)
    , m_jsActionDelegate(0)
    , m_webEditingDelegate(0)
    , m_webResourceLoadDelegate(0)
    , m_webBindingJSDelegate(0)
    , m_webWidgetEngineDelegate(0)
    , m_historyDelegate(0)
    , m_geolocationProvider(0)
    , m_preferences(0)
    , m_userAgentOverridden(false)
    , m_useBackForwardList(true)
    , m_zoomMultiplier(1.0f)
	, m_zoomsTextOnly(false)
    , m_mouseActivated(false)
    // , m_dragData(0)
    // , m_currentCharacterCode(0)
    , m_isBeingDestroyed(false)
    , m_paintCount(0)
    , m_hasSpellCheckerDocumentTag(false)
    , m_smartInsertDeleteEnabled(false)
    , m_selectTrailingWhitespaceEnabled(false)
    , m_didClose(false)
    , m_hasCustomDropTarget(false)
    , m_inIMEComposition(0)
    , m_toolTipHwnd(0)
    , m_deleteBackingStoreTimerActive(false)
    , m_transparent(false)
    , m_isInitialized(false)
    , m_topLevelParent(0)
    , d(new WebViewPrivate(this))
    , m_webViewObserver(new WebViewObserver(this))
    , m_webInspector(0)
    , m_toolbarsVisible(true)
    , m_statusbarVisible(true)
    , m_menubarVisible(true)
    , m_locationbarVisible(true)
    , m_inputState(false)
    , m_dragTargetDispatch(false)
    , m_dragIdentity(0)
    , m_dropEffect(DropEffectDefault)
    , m_operationsAllowed(WebDragOperationNone)
    , m_dragOperation(WebDragOperationNone)
    , m_currentDragData(0)
{
    d->clearDirtyRegion();

	globalUserAgent = getenv("OWB_USER_AGENT");
    setScheduledScrollOffset(IntPoint(0, 0));

    initializeStaticObservers();

    WebCore::ObserverServiceData::createObserverService()->registerObserver("PopupMenuShow", m_webViewObserver);
    WebCore::ObserverServiceData::createObserverService()->registerObserver("PopupMenuHide", m_webViewObserver);

    WebPreferences* sharedPreferences = WebPreferences::sharedStandardPreferences();
    sharedPreferences->setContinuousSpellCheckingEnabled(getv(app, MA_OWBApp_ContinuousSpellChecking) != 0);
    continuousSpellCheckingEnabled = sharedPreferences->continuousSpellCheckingEnabled();
    grammarCheckingEnabled = sharedPreferences->grammarCheckingEnabled();
    sharedPreferences->willAddToWebView();
    m_preferences = sharedPreferences;
    m_webViewGroup = WebViewGroup::getOrCreate(String(), m_preferences->localStorageDatabasePath());
    m_webViewGroup->addWebView(this);

    m_inspectorClient = new WebInspectorClient(this);

    WebFrameLoaderClient * pageWebFrameLoaderClient = new WebFrameLoaderClient();
    WebProgressTrackerClient * pageProgressTrackerClient = new WebProgressTrackerClient();

    PageConfiguration configuration;
    configuration.chromeClient = new WebChromeClient(this);
    configuration.contextMenuClient = new WebContextMenuClient(this);
    configuration.editorClient = new WebEditorClient(this);
    configuration.dragClient = new WebDragClient(this);
    configuration.inspectorClient = m_inspectorClient;
    configuration.loaderClientForMainFrame = pageWebFrameLoaderClient;
    configuration.databaseProvider = &WebDatabaseProvider::singleton();
    configuration.storageNamespaceProvider = &m_webViewGroup->storageNamespaceProvider();
    configuration.progressTrackerClient = pageProgressTrackerClient;
    configuration.userContentController = &m_webViewGroup->userContentController();
    configuration.visitedLinkStore = &WebVisitedLinkStore::singleton();


    m_page = new Page(configuration);
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    provideNotification(m_page, new WebDesktopNotificationsDelegate(this));
#endif
#if ENABLE(GEOLOCATION)
    provideGeolocationTo(m_page, new WebGeolocationClient());
#endif
#if ENABLE(DEVICE_ORIENTATION)
    provideDeviceMotionTo(m_page, new WebDeviceMotionClient());
    provideDeviceOrientationTo(m_page, new WebDeviceOrientationClient());
#endif

    m_page->settings().setFTPDirectoryTemplatePath("PROGDIR:resource/FTPDirectoryTemplate.html");

    WebFrame* webFrame = WebFrame::createInstance(); 
    webFrame->initWithWebView(this, m_page); 
    m_mainFrame = webFrame;
    // webFrame is now owned by WebFrameLoaderClient and will be destroyed in
    // chain Page->MainFrame->FrameLoader->WebFrameLoaderClient
    pageWebFrameLoaderClient->setWebFrame(webFrame);

    pageProgressTrackerClient->setWebFrame(webFrame);
}

WebView::~WebView()
{
    close();
    
    m_webViewGroup->removeWebView(this);

    if (m_preferences)
        if (m_preferences != WebPreferences::sharedStandardPreferences())
            delete m_preferences;
    if (m_policyDelegate)
        m_policyDelegate = 0;
    if (m_downloadDelegate)
        m_downloadDelegate = 0;
    if (m_webNotificationDelegate)
        m_webNotificationDelegate = 0;
    if (m_webFrameLoadDelegate)
        m_webFrameLoadDelegate = 0;
    if (m_jsActionDelegate)
        m_jsActionDelegate = 0;
    if (m_historyDelegate)
        m_historyDelegate = 0;
    if (m_mainFrame)
        delete m_mainFrame;
    if (d)
        delete d;
    if (m_webViewObserver)
        delete m_webViewObserver;
    m_children.clear();
}

WebView* WebView::createInstance()
{
    return new WebView();
}

void initializeStaticObservers()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;

    WebCore::ObserverServiceData::createObserverService()->registerObserver(WebPreferences::webPreferencesChangedNotification(), PreferencesChangedOrRemovedObserver::sharedInstance());
    WebCore::ObserverServiceData::createObserverService()->registerObserver(WebPreferences::webPreferencesRemovedNotification(), PreferencesChangedOrRemovedObserver::sharedInstance());

}

void WebView::setCacheModel(WebCacheModel cacheModel)
{
    if (s_didSetCacheModel && cacheModel == s_cacheModel)
        return;

    //TODO : Calcul the next values
    unsigned long long memSize = 256;
    //unsigned long long diskFreeSize = 4096;
    
    memSize = AvailMem(MEMF_TOTAL) / (1024 * 1024);

    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    auto deadDecodedDataDeletionInterval = std::chrono::seconds { 0 };

    unsigned pageCacheCapacity = 0;

    switch (cacheModel) {
    case WebCacheModelDocumentViewer: {
        // Page cache capacity (in pages)
        pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1536)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;
		else if (memSize >= 256)
			cacheTotalCapacity = 8 * 1024 * 1024;
		else if (memSize >= 128)
			cacheTotalCapacity = 4 * 1024 * 1024;

        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;

        break;
    }
    case WebCacheModelDocumentBrowser: {
        // Page cache capacity (in pages)
        if (memSize >= 1024)
            pageCacheCapacity = 3;
        else if (memSize >= 512)
            pageCacheCapacity = 2;
        else if (memSize >= 256)
            pageCacheCapacity = 1;
        else
            pageCacheCapacity = 0;

        // Object cache capacities (in bytes)
        if (memSize >= 2048)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1536)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 32 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 16 * 1024 * 1024;
		else if (memSize >= 256)
			cacheTotalCapacity = 8 * 1024 * 1024;
		else if (memSize >= 128)
			cacheTotalCapacity = 4 * 1024 * 1024;

        cacheMinDeadCapacity = cacheTotalCapacity / 8;
        cacheMaxDeadCapacity = cacheTotalCapacity / 4;

        break;
    }
    case WebCacheModelPrimaryWebBrowser: {
        // Page cache capacity (in pages)
        // (Research indicates that value / page drops substantially after 3 pages.)
        if (memSize >= 2048)
            pageCacheCapacity = 5;
        else if (memSize >= 1024)
            pageCacheCapacity = 4;
        else if (memSize >= 512)
            pageCacheCapacity = 3;
        else if (memSize >= 256)
            pageCacheCapacity = 2;
        else
            pageCacheCapacity = 1;

        // Object cache capacities (in bytes)
        // (Testing indicates that value / MB depends heavily on content and
        // browsing pattern. Even growth above 128MB can have substantial 
        // value / MB for some content / browsing patterns.)
        if (memSize >= 2048)
            cacheTotalCapacity = 128 * 1024 * 1024;
        else if (memSize >= 1536)
            cacheTotalCapacity = 96 * 1024 * 1024;
        else if (memSize >= 1024)
            cacheTotalCapacity = 64 * 1024 * 1024;
        else if (memSize >= 512)
            cacheTotalCapacity = 32 * 1024 * 1024;
		else if (memSize >= 256)
			cacheTotalCapacity = 16 * 1024 * 1024;
		else if (memSize >= 128)
			cacheTotalCapacity = 8 * 1024 * 1024;

		cacheTotalCapacity*=4;

        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;

        // This code is here to avoid a PLT regression. We can remove it if we
        // can prove that the overall system gain would justify the regression.
        cacheMaxDeadCapacity = max(24u, cacheMaxDeadCapacity);

        deadDecodedDataDeletionInterval = std::chrono::seconds { 60 };

        break;
    }
    default:
        ASSERT_NOT_REACHED();
    };

    WebCore::MemoryCache::singleton().setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    WebCore::MemoryCache::singleton().setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    WebCore::PageCache::singleton().setMaxSize(pageCacheCapacity);

    s_didSetCacheModel = true;
    s_cacheModel = cacheModel;
    return;
}

WebCacheModel WebView::cacheModel()
{
    return s_cacheModel;
}

bool WebView::didSetCacheModel()
{
    return s_didSetCacheModel;
}

void WebView::close()
{
    if (m_didClose)
        return;

    m_didClose = true;

    // Purge page cache
    // The easiest way to do that is to disable page cache
    // The preferences can be null.
    if (m_preferences && m_preferences->usesPageCache())
        m_page->settings().setUsesPageCache(false);
    
    if (!WebCore::MemoryCache::singleton().disabled()) {
        WebCore::MemoryCache::singleton().setDisabled(true);
        WebCore::MemoryCache::singleton().setDisabled(false);

#if ENABLE(STORAGE)
        // Empty the application cache.
        WebCore::cacheStorage().empty();
#endif

        // Empty the Cross-Origin Preflight cache
        WebCore::CrossOriginPreflightResultCache::singleton().empty();
    }

    if (m_page)
        m_page->mainFrame().loader().detachFromParent(); 

    m_inspectorClient = 0;

    delete m_page;
    m_page = 0;

#if ENABLE(ICONDATABASE)
    registerForIconNotification(false);
#endif
    WebCore::ObserverServiceData::createObserverService()->removeObserver(WebPreferences::webPreferencesChangedNotification(), m_webViewObserver);
    WebCore::ObserverServiceData::createObserverService()->removeObserver("PopupMenuShow", m_webViewObserver);
    WebCore::ObserverServiceData::createObserverService()->removeObserver("PopupMenuHide", m_webViewObserver);
    const String& identifier = m_preferences ? m_preferences->identifier() : String();
    if (identifier != String())
        WebPreferences::removeReferenceForIdentifier(identifier.utf8().data());

    WebPreferences* preferences = m_preferences;
    if (preferences)
        preferences->didRemoveFromWebView();

    deleteBackingStore();
}

void WebView::repaint(const WebCore::IntRect& windowRect, bool contentChanged, bool immediate, bool repaintContentOnly)
{
    //kprintf("WebView::repaint([%d %d %d %d], %d, %d , %d\n", windowRect.x(), windowRect.y(), windowRect.width(), windowRect.height(), contentChanged, immediate, repaintContentOnly);
    d->repaint(windowRect, contentChanged, immediate, repaintContentOnly);
}

void WebView::deleteBackingStore()
{
    //kprintf("WebView::deleteBackingStore\n");
    if (m_deleteBackingStoreTimerActive) {
        m_deleteBackingStoreTimerActive = false;
    }
    //m_backingStoreBitmap.clear();
    d->clearDirtyRegion();
}

bool WebView::ensureBackingStore()
{
    return false;
}

void WebView::addToDirtyRegion(const BalRectangle& dirtyRect)
{
    d->addToDirtyRegion(dirtyRect);
}

BalRectangle WebView::dirtyRegion()
{
    return d->dirtyRegion();
}

void WebView::clearDirtyRegion()
{
    d->clearDirtyRegion();
}

void WebView::scrollBackingStore(FrameView* frameView, int dx, int dy, const BalRectangle& scrollViewRect, const BalRectangle& clipRect)
{
    //kprintf("WebView::scrollBackingStore\n");
    d->scrollBackingStore(frameView, dx, dy, scrollViewRect, clipRect);
}

void WebView::updateBackingStore(FrameView* frameView, bool backingStoreCompletelyDirty)
{
    //kprintf("WebView::updateBackingStore(completelyDirty %d\n", backingStoreCompletelyDirty);
    //frameView->updateBackingStore();
}

void WebView::selectionChanged() 
{
    Frame* targetFrame = &(m_page->focusController().focusedOrMainFrame());
    if (!targetFrame || !targetFrame->editor().hasComposition())
        return;
    
    if (targetFrame->editor().ignoreCompositionSelectionChange())
        return;

    unsigned start;
    unsigned end;
    if (!targetFrame->editor().getCompositionSelection(start, end))
        targetFrame->editor().cancelComposition();
}


BalRectangle WebView::onExpose(BalEventExpose ev)
{
    BalRectangle rect = d->onExpose(ev);
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->onExpose(ev, rect);
    return rect;
}

bool WebView::onKeyDown(BalEventKey ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onKeyDown(ev))
            return true;
    return d->onKeyDown(ev);
}

bool WebView::onKeyUp(BalEventKey ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onKeyUp(ev))
            return true;
    return d->onKeyUp(ev);
}

bool WebView::onMouseMotion(BalEventMotion ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onMouseMotion(ev))
            return true;
    return d->onMouseMotion(ev);
}

bool WebView::onMouseButtonDown(BalEventButton ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onMouseButtonDown(ev))
            return true;
    return d->onMouseButtonDown(ev);
}

bool WebView::onMouseButtonUp(BalEventButton ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onMouseButtonUp(ev))
            return true;
    return d->onMouseButtonUp(ev);
}

bool WebView::onScroll(BalEventScroll ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onScroll(ev))
            return true;
    return d->onScroll(ev);
}

bool WebView::onResize(BalResizeEvent ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onResize(ev))
            return true;
    d->onResize(ev);
    return true;
}

bool WebView::onQuit(BalQuitEvent ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onQuit(ev))
            return false;
    d->onQuit(ev);
    return true;
}

void WebView::onUserEvent(BalUserEvent ev)
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        if(m_children[i]->onUserEvent(ev))
            return;
    d->onUserEvent(ev);
}

bool WebView::defaultActionOnFocusedNode(BalEventKey event)
{
    Frame* frame = &(page()->focusController().focusedOrMainFrame());
    if (!frame)
        return false;
	WebCore::Node* focusedNode = frame->document()->focusedElement();
    if (!focusedNode)
        return false;

    PlatformKeyboardEvent keyboardEvent(&event);
    if (keyboardEvent.type() == PlatformKeyboardEvent::KeyDown) {
        // FIXME: We should tweak the parameters instead of hard coding them.
        keyboardEvent.disambiguateKeyDownEvent(PlatformKeyboardEvent::RawKeyDown, false);
    }
    RefPtr<KeyboardEvent> keyEvent = KeyboardEvent::create(keyboardEvent, frame->document()->defaultView());
    keyEvent->setTarget(focusedNode);
    focusedNode->defaultEventHandler(keyEvent.get());
    
    return (keyEvent->defaultHandled() || keyEvent->defaultPrevented());
}

void WebView::paint()
{
    /*Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return;
    FrameView* frameView = coreFrame->view();
    frameView->paint();*/
}

BalRectangle WebView::frameRect()
{
    return d->frameRect();
}

void WebView::closeWindowSoon()
{
	//closeWindow();
	d->closeWindowSoon();
}

void WebView::closeWindow()
{
    BalWidget *widget = m_viewWindow;
	if(widget)
	{
		DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_OWBApp_RemoveBrowser, widget->browser);
	}
}

bool WebView::canHandleRequest(const WebCore::ResourceRequest& request)
{
	if (request.url().protocolIs("about"))
	{
	    return true;
	}
	else if (request.url().protocolIs("topsites"))
	{
	    return true;
	}
	else if (request.url().protocolIs("mailto"))
	{
	    DoMethod(app, MM_OWBApp_MailTo, request.url().string().latin1().data());
	    return false;
	}

    return true;
}

void WebView::reloadFromOrigin()
{
    if (!m_mainFrame)
        return;

    m_mainFrame->reloadFromOrigin();
}

void WebView::setCookieEnabled(bool enable)
{
    if (!m_page)
        return;

    m_page->settings().setCookieEnabled(enable);
}

bool WebView::cookieEnabled()
{
    if (!m_page)
        return false;

    return m_page->settings().cookieEnabled();
}

Page* WebView::page()
{
    return m_page;
}

static const unsigned CtrlKey = 1 << 0;
static const unsigned AltKey = 1 << 1;
static const unsigned ShiftKey = 1 << 2;
static const unsigned AmigaKey = 1 << 4;


struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static const KeyDownEntry keyDownEntries[] = {
    { VK_LEFT,   0,                  "MoveLeft"                                    },
    { VK_LEFT,   ShiftKey,           "MoveLeftAndModifySelection"                  },
    { VK_LEFT,   CtrlKey,            "MoveWordLeft"                                },
    { VK_LEFT,   CtrlKey | ShiftKey, "MoveWordLeftAndModifySelection"              },
    { VK_RIGHT,  0,                  "MoveRight"                                   },
    { VK_RIGHT,  ShiftKey,           "MoveRightAndModifySelection"                 },
    { VK_RIGHT,  CtrlKey,            "MoveWordRight"                               },
    { VK_RIGHT,  CtrlKey | ShiftKey, "MoveWordRightAndModifySelection"             },
    { VK_UP,     0,                  "MoveUp"                                      },
    { VK_UP,     ShiftKey,           "MoveUpAndModifySelection"                    },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection"                },
    { VK_PRIOR,  0,                  "MovePageUp"                                  },
    { VK_DOWN,   0,                  "MoveDown"                                    },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection"                  },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection"              },
    { VK_NEXT,   0,                  "MovePageDown"                                },
    { VK_HOME,   0,                  "MoveToBeginningOfLine"                       },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection"     },
    { VK_HOME,   CtrlKey,            "MoveToBeginningOfDocument"                   },
    { VK_HOME,   CtrlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },

    { VK_END,    0,                  "MoveToEndOfLine"                             },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection"           },
    { VK_END,    CtrlKey,            "MoveToEndOfDocument"                         },
    { VK_END,    CtrlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection"       },

    { VK_BACK,   0,                  "DeleteBackward"                              },
	{ VK_BACK,   ShiftKey,           /*"DeleteBackward"*/"DeleteToBeginningOfLine" },
    { VK_DELETE, 0,                  "DeleteForward"                               },
    { VK_BACK,   CtrlKey,            "DeleteWordBackward"                          },
    { VK_DELETE, CtrlKey,            "DeleteWordForward"                           },
	{ VK_DELETE, ShiftKey,           "DeleteToEndOfLine"                           },
    
    { 'B',       CtrlKey,            "ToggleBold"                                  },
    { 'I',       CtrlKey,            "ToggleItalic"                                },

    { VK_ESCAPE, 0,                  "Cancel"                                      },
    { VK_OEM_PERIOD, CtrlKey,        "Cancel"                                      },
    { VK_TAB,    0,                  "InsertTab"                                   },
    { VK_TAB,    ShiftKey,           "InsertBacktab"                               },
    { VK_RETURN, 0,                  "InsertNewline"                               },
    { VK_RETURN, CtrlKey,            "InsertNewline"                               },
    { VK_RETURN, AltKey,             "InsertNewline"                               },
    { VK_RETURN, ShiftKey,           "InsertNewline"                               },
    { VK_RETURN, AltKey | ShiftKey,  "InsertNewline"                               },

    // It's not quite clear whether clipboard shortcuts and Undo/Redo should be handled
    // in the application or in WebKit. We chose WebKit.
    { 'C',       CtrlKey,            "Copy"                                        },
    { 'V',       CtrlKey,            "Paste"                                       },
    { 'X',       CtrlKey,            "Cut"                                         },
    { 'A',       CtrlKey,            "SelectAll"                                   },
    { 'C',       AmigaKey,           "Copy"                                        },
    { 'V',       AmigaKey,           "Paste"                                       },
    { 'X',       AmigaKey,           "Cut"                                         },
    { 'A',       AmigaKey,           "SelectAll"                                   },
    { VK_INSERT, CtrlKey,            "Copy"                                        },
    { VK_DELETE, ShiftKey,           "Cut"                                         },
    { VK_INSERT, ShiftKey,           "Paste"                                       },
    { 'Z',       CtrlKey,            "Undo"                                        },
    { 'Z',       CtrlKey | ShiftKey, "Redo"                                        },
    { 'Z',       AmigaKey,           "Undo"                                        },
    { 'Z',       AmigaKey | ShiftKey,"Redo"                                        },
    { 9999999,   0,                  ""                                            },
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',   0,                  "InsertTab"                                   },
    { '\t',   ShiftKey,           "InsertBacktab"                               },
    { '\r',   0,                  "InsertNewline"                               },
    { '\r',   CtrlKey,            "InsertNewline"                               },
    { '\r',   AltKey,             "InsertNewline"                               },
    { '\r',   ShiftKey,           "InsertNewline"                               },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               },
    { 9999999,   0,               ""                                         },
};

const char* WebView::interpretKeyEvent(const KeyboardEvent* evt)
{
    ASSERT(evt->type() == eventNames().keydownEvent || evt->type() == eventNames().keypressEvent);

    static HashMap<int, const char*>* keyDownCommandsMap = 0;
    static HashMap<int, const char*>* keyPressCommandsMap = 0;

    if (!keyDownCommandsMap) {
        keyDownCommandsMap = new HashMap<int, const char*>;
        keyPressCommandsMap = new HashMap<int, const char*>;

        int i = 0;
        while(keyDownEntries[i].virtualKey != 9999999) {
            keyDownCommandsMap->set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);
            i++;
        }

        i = 0;
        while(keyPressEntries[i].charCode != 9999999) {
            keyPressCommandsMap->set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
            i++;
        }
    }

    unsigned modifiers = 0;
    if (evt->shiftKey())
        modifiers |= ShiftKey;
    if (evt->altKey())
        modifiers |= AltKey;
    if (evt->ctrlKey())
        modifiers |= CtrlKey;
    if (evt->metaKey())
        modifiers |= AmigaKey;

    if (evt->type() == eventNames().keydownEvent) {
        int mapKey = modifiers << 16 | evt->keyCode();
        return mapKey ? keyDownCommandsMap->get(mapKey) : 0;
    }

    int mapKey = modifiers << 16 | evt->charCode();
    return mapKey ? keyPressCommandsMap->get(mapKey) : 0;
}

bool WebView::handleEditingKeyboardEvent(KeyboardEvent* evt)
{
    WebCore::Node* node = evt->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document().frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* keyEvent = evt->keyEvent();
    if (!keyEvent)  // do not treat this as text input if it's a system key event
        return false;

    Editor::Command command = frame->editor().command(interpretKeyEvent(evt));

    if (keyEvent->type() == PlatformKeyboardEvent::RawKeyDown) {
        // WebKit doesn't have enough information about mode to decide how commands that just insert text if executed via Editor should be treated,
        // so we leave it upon WebCore to either handle them immediately (e.g. Tab that changes focus) or let a keypress event be generated
        // (e.g. Tab that inserts a Tab character, or Enter).
        return !command.isTextInsertion() && command.execute(evt);
    }

     if (command.execute(evt))
        return true;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (evt->charCode() < ' ')
        return false;

    return frame->editor().insertText(evt->keyEvent()->text(), evt);
}

void WebView::setShouldInvertColors(bool shouldInvertColors)
{
}

bool WebView::developerExtrasEnabled() const
{
    return getv(app, MA_OWBApp_EnableInspector);

    //if (m_preferences->developerExtrasDisabledByOverride())
    //    return false;
}

const string& WebView::userAgentForURL(const string& url)
{
    if (m_userAgentOverridden)
	{
        return m_userAgentCustom;
	}
	else
	{
	    // Env var overrides everything (is it right?)
		if (globalUserAgent)
			m_userAgentStandard = globalUserAgent;
	    else
			m_userAgentStandard = (char *) getv(app, MA_OWBApp_UserAgent);
	 
	    return m_userAgentStandard;
	}
}

bool WebView::canShowMIMEType(const char* mimeType)
{
    String type = String(mimeType).lower();
    Frame* coreFrame = core(m_mainFrame);
    bool allowPlugins = coreFrame && coreFrame->loader().subframeLoader().allowPlugins(NotAboutToInstantiatePlugin);
    
	bool canShow = type.isEmpty()
	|| MIMETypeRegistry::isSupportedImageMIMEType(type)
	|| MIMETypeRegistry::isSupportedNonImageMIMEType(type)
	|| MIMETypeRegistry::isSupportedMediaMIMEType(type);
    
    if (!canShow && m_page) {
	canShow = (m_page->pluginData().supportsWebVisibleMimeType(type, PluginData::AllPlugins) && allowPlugins)
	|| m_page->pluginData().supportsWebVisibleMimeType(type, PluginData::OnlyApplicationPlugins);
    }     
    
    //kprintf("WebView::canShowMIMEType(%s): %s\n", type.utf8().data(), canShow ? "yes" : "no");
    
    return canShow;
}

bool WebView::canShowMIMETypeAsHTML(const char* /*mimeType*/)
{
    return true;
}

void WebView::setMIMETypesShownAsHTML(const char* /*mimeTypes*/, int /*cMimeTypes*/)
{
}

void WebView::createWindow(BalRectangle frame)
{
    m_viewWindow = d->createWindow(frame);
}

void WebView::initWithFrame(BalRectangle& frame, const char* frameName, const char* groupName)
{
    if (m_isInitialized)
        return;

    m_isInitialized = true;

    if (!m_viewWindow)
        m_viewWindow = d->createWindow(frame);
    else
        d->setFrameRect(frame);

    RefPtr<Frame> coreFrame = core(m_mainFrame);
    m_page->mainFrame().tree().setName(frameName);
    m_page->mainFrame().init(); 
    setGroupName(groupName);
    WebCore::FrameView* frameView = coreFrame->view();
    d->initWithFrameView(frameView);

    initializeToolTipWindow();
    windowAncestryDidChange();

    static bool didOneTimeInitialization;

    if (!didOneTimeInitialization)
    {
        //WebCore::initializeLoggingChannelsIfNecessary();
        //WebPlatformStrategies::initialize();   
		WebKitInitializeWebDatabasesIfNecessary();
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
		WebKitSetApplicationCachePathIfNecessary();
#endif
		didOneTimeInitialization = true;
    }

    WebCore::ObserverServiceData::createObserverService()->registerObserver(WebPreferences::webPreferencesChangedNotification(), m_webViewObserver);
    
    m_preferences->postPreferencesChangesNotification();

    setSmartInsertDeleteEnabled(true);
    WebVisitedLinkStore::setShouldTrackVisitedLinks(true);

    m_page->focusController().setFocusedFrame(&(m_page->mainFrame()));
    m_page->focusController().setActive(true); 
    m_page->focusController().setFocused(true);
    m_page->settings().setMinimumDOMTimerInterval(0.004);

    Frame* mainFrame = &(m_page->mainFrame());
    Frame* focusedFrame = &(m_page->focusController().focusedOrMainFrame());
    mainFrame->selection().setFocused(mainFrame == focusedFrame);
}

void WebView::initializeToolTipWindow()
{
}

void WebView::setToolTip(const char* toolTip)
{
    if (toolTip == m_toolTip)
        return;

    m_toolTip = toolTip;

    BalWidget *widget = m_viewWindow;

	if(widget)
	{
		char *converted = utf8_to_local(toolTip);

		if(converted)
		{
			SetAttrs(widget->browser, MA_OWBBrowser_ToolTipText, converted, TAG_DONE);
			free(converted);
		}
	}
}

#if ENABLE(ICONDATABASE)
void WebView::notifyDidAddIcon()
{
    dispatchDidReceiveIconFromWebFrame(m_mainFrame);
}

void WebView::registerForIconNotification(bool listen)
{
    if (listen)
		WebCore::ObserverServiceData::createObserverService()->registerObserver(WebIconDatabase::iconDatabaseDidAddIconNotification(), m_webViewObserver);
    else
		WebCore::ObserverServiceData::createObserverService()->removeObserver(WebIconDatabase::iconDatabaseDidAddIconNotification(), m_webViewObserver);
}

void WebView::dispatchDidReceiveIconFromWebFrame(WebFrame* frame)
{
    registerForIconNotification(false);

    if(getv(app, MA_OWBApp_ShowFavIcons))
	{
		BalWidget *widget = m_viewWindow;
		if(widget)
		{
			SetAttrs(widget->browser, MA_FavIcon_PageURL, frame->url(), TAG_DONE);
		}
	}
}
#endif

void WebView::setDownloadDelegate(TransferSharedPtr<WebDownloadDelegate> d)
{
    m_downloadDelegate = d;
}

TransferSharedPtr<WebDownloadDelegate> WebView::downloadDelegate()
{
    return m_downloadDelegate;
}

void WebView::setPolicyDelegate(TransferSharedPtr<WebPolicyDelegate> d)
{
    m_policyDelegate = d;
}

TransferSharedPtr<WebPolicyDelegate> WebView::policyDelegate()
{
    return m_policyDelegate;
}

void WebView::setWebNotificationDelegate(TransferSharedPtr<WebNotificationDelegate> notificationDelegate)
{
    m_webNotificationDelegate = notificationDelegate;
}

TransferSharedPtr<WebNotificationDelegate> WebView::webNotificationDelegate()
{
    return m_webNotificationDelegate;
}

void WebView::setWebFrameLoadDelegate(TransferSharedPtr<WebFrameLoadDelegate> webFrameLoadDelegate)
{
    m_webFrameLoadDelegate = webFrameLoadDelegate;

}

TransferSharedPtr<WebFrameLoadDelegate> WebView::webFrameLoadDelegate()
{
    return m_webFrameLoadDelegate;
}

void WebView::setJSActionDelegate(TransferSharedPtr<JSActionDelegate> newJSActionDelegate)
{
    m_jsActionDelegate = newJSActionDelegate;
}

TransferSharedPtr<JSActionDelegate> WebView::jsActionDelegate()
{
    return m_jsActionDelegate;
}

void WebView::setWebEditingDelegate(TransferSharedPtr<WebEditingDelegate> editing)
{
    m_webEditingDelegate = editing;
}

TransferSharedPtr<WebEditingDelegate> WebView::webEditingDelegate()
{
    return m_webEditingDelegate;
}

void WebView::setWebResourceLoadDelegate(TransferSharedPtr<WebResourceLoadDelegate> resourceLoad)
{
    m_webResourceLoadDelegate = resourceLoad;
}

TransferSharedPtr<WebResourceLoadDelegate> WebView::webResourceLoadDelegate()
{
    return m_webResourceLoadDelegate;
}

void WebView::setWebBindingJSDelegate(TransferSharedPtr<WebBindingJSDelegate> webBindingJSDelegate)
{
    m_webBindingJSDelegate = webBindingJSDelegate;
}

TransferSharedPtr<WebBindingJSDelegate> WebView::webBindingJSDelegate()
{
    return m_webBindingJSDelegate;
}

void WebView::setWebWidgetEngineDelegate(TransferSharedPtr<WebWidgetEngineDelegate> webWidgetEngineDelegate)
{
    m_webWidgetEngineDelegate = webWidgetEngineDelegate;
}

TransferSharedPtr<WebWidgetEngineDelegate> WebView::webWidgetEngineDelegate()
{
    return m_webWidgetEngineDelegate;
}

void WebView::setGeolocationProvider(WebGeolocationProvider* locationProvider)
{
    m_geolocationProvider = locationProvider;
}

WebGeolocationProvider* WebView::geolocationProvider()
{
    return m_geolocationProvider;
}

#if ENABLE(GEOLOCATION)
void WebView::geolocationDidChangePosition(WebGeolocationPosition* position)
{
    if (!m_page)
        return;
    GeolocationController::from(m_page)->positionChanged(core(position));
}

void WebView::geolocationDidFailWithError(WebError* error)
{
    if (!m_page)
        return;
    if (!error)
        return;

    const char* description = error->localizedDescription();
    if (!description)
        return;

    RefPtr<GeolocationError> geolocationError = GeolocationError::create(GeolocationError::PositionUnavailable, description);
    GeolocationController::from(m_page)->errorOccurred(geolocationError.get());
}
#endif 

/*void WebView::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const char* scheme)
{
    SecurityPolicy::setDomainRelaxationForbiddenForURLScheme(forbidden, String(scheme, SysStringLen(scheme)));
}

void WebView::registerURLSchemeAsSecure(const char* scheme)
{
	SchemeRegistry::registerURLSchemeAsSecure(toString(scheme));
}*/

WebFrame* WebView::mainFrame()
{
    return m_mainFrame;
}

WebFrame* WebView::focusedFrame()
{
    Frame* f = m_page->focusController().focusedFrame();
    if (!f)
        return 0;

    WebFrame* webFrame = kit(f);
    return webFrame;
}
WebBackForwardList* WebView::backForwardList()
{
    WebBackForwardListPrivate *p = new WebBackForwardListPrivate(static_cast<WebCore::BackForwardList*>(m_page->backForward().client()));
    return WebBackForwardList::createInstance(p);
}

void WebView::setMaintainsBackForwardList(bool flag)
{
    m_useBackForwardList = !!flag;
}

bool WebView::goBack()
{
    bool hasGoneBack= m_page->backForward().goBack();
    d->repaintAfterNavigationIfNeeded();
    return hasGoneBack;
}

bool WebView::goForward()
{
    bool hasGoneForward = m_page->backForward().goForward();
    d->repaintAfterNavigationIfNeeded();
    return hasGoneForward;
}

void WebView::goBackOrForward(int steps)
{
    m_page->backForward().goBackOrForward(steps);
}

bool WebView::goToBackForwardItem(WebHistoryItem* item)
{
    WebHistoryItemPrivate *priv = item->getPrivateItem(); 
    m_page->goToItem(*priv->m_historyItem, FrameLoadType::IndexedBackForward);
    return true;
}

bool WebView::setTextSizeMultiplier(float multiplier)
{
    return setZoomMultiplier(multiplier, true);
}

bool WebView::setPageSizeMultiplier(float multiplier)
{
    return setZoomMultiplier(multiplier, false);
}

bool WebView::setZoomMultiplier(float multiplier, bool isTextOnly)
{
    // Check if we need to round the value.
    bool hasRounded = false;
    if (multiplier < cMinimumZoomMultiplier) {
        multiplier = cMinimumZoomMultiplier;
        hasRounded = true;
    } else if (multiplier > cMaximumZoomMultiplier) {
        multiplier = cMaximumZoomMultiplier;
        hasRounded = true;
    }

    m_zoomMultiplier = multiplier;
    m_zoomsTextOnly = isTextOnly;
    Frame* coreFrame = core(m_mainFrame);
	if (coreFrame && coreFrame->document()) {
        if (m_zoomsTextOnly)
            coreFrame->setPageAndTextZoomFactors(1, multiplier);
		else
			coreFrame->setPageAndTextZoomFactors(multiplier, 1);
	}

    return !hasRounded;
}

float WebView::textSizeMultiplier()
{
    return zoomMultiplier(true);
}

float WebView::pageSizeMultiplier()
{
    return zoomMultiplier(false);
}

float WebView::zoomMultiplier(bool isTextOnly)
{
	if (isTextOnly != m_zoomsTextOnly)
        return 1.0f;
    return m_zoomMultiplier;
}

bool WebView::zoomIn(bool isTextOnly)
{
    return setZoomMultiplier(zoomMultiplier(isTextOnly) * cZoomMultiplierRatio, isTextOnly);
}

bool WebView::zoomOut(bool isTextOnly)
{
    return setZoomMultiplier(zoomMultiplier(isTextOnly) / cZoomMultiplierRatio, isTextOnly);
}

bool WebView::zoomPageIn()
{
    return zoomIn(false);
}

bool WebView::zoomPageOut()
{
    return zoomOut(false);
}

bool WebView::canResetZoom(bool isTextOnly)
{
    return zoomMultiplier(isTextOnly) != 1.0f;
}

bool WebView::canResetPageZoom()
{
    return canResetZoom(false);
}

void WebView::resetPageZoom()
{
    return resetZoom(false);
}

void WebView::resetZoom(bool isTextOnly)
{
    if (!canResetZoom(isTextOnly))
        return ;
    setZoomMultiplier(1.0f, isTextOnly);
}

bool WebView::makeTextLarger()
{
    return zoomIn(m_zoomsTextOnly);
}

bool WebView::makeTextSmaller()
{
    return zoomOut(m_zoomsTextOnly);
}

bool WebView::canMakeTextStandardSize()
{
    // Since we always reset text zoom and page zoom together, this should continue to return an answer about text zoom even if its not enabled.
    return canResetZoom(true);
}

void WebView::makeTextStandardSize()
{
    resetZoom(true);
}



void WebView::setApplicationNameForUserAgent(const string& applicationName)
{
    m_applicationName = applicationName;
    m_userAgentStandard = "";
}

const string& WebView::applicationNameForUserAgent()
{
    return m_applicationName;
}

void WebView::setCustomUserAgent(const string& userAgentString)
{
    m_userAgentOverridden = userAgentString != string("");
    m_userAgentCustom = userAgentString;
}

const string& WebView::customUserAgent()
{
    return m_userAgentCustom;
}

bool WebView::supportsTextEncoding()
{
    return true;
}

void WebView::setCustomTextEncodingName(const char* encodingName)
{
    if (!m_mainFrame)
        return ;

    String oldEncoding = customTextEncodingName();

    if (oldEncoding != encodingName) {
        if (Frame* coreFrame = core(m_mainFrame))
            coreFrame->loader().reloadWithOverrideEncoding(encodingName);
    }
}

const char* WebView::customTextEncodingName()
{
    WebDataSource* dataSource;

    if (!m_mainFrame)
        return NULL;

    dataSource = m_mainFrame->provisionalDataSource();
    if (!dataSource) {
        dataSource = m_mainFrame->dataSource();
        if (!dataSource)
            return NULL;
    }

    return dataSource->documentLoader()->overrideEncoding().utf8().data();
}

void WebView::setMediaStyle(const char* /*media*/)
{
}

const char* WebView::mediaStyle()
{
    return NULL;
}

const char* WebView::stringByEvaluatingJavaScriptFromString(const char* script)
{
    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return NULL; 

    JSC::JSValue scriptExecutionResult = coreFrame->script().executeScript(script, false).jsValue();
    if(!scriptExecutionResult)
        return NULL;
    else if (scriptExecutionResult.isString()) {
        JSC::ExecState* exec = coreFrame->script().globalObject(mainThreadNormalWorld())->globalExec();
        JSC::JSLockHolder lock(exec);
        return scriptExecutionResult.getString(exec).ascii().data();
    }
    return NULL;
}

void WebView::executeScript(const char* script)
{
    if (!script)
        return;

    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return ;

    coreFrame->script().executeScript(String::fromUTF8(script), true);
}

WebScriptObject* WebView::windowScriptObject()
{
    return 0;
}

void WebView::setPreferences(WebPreferences* prefs)
{
    if (!prefs)
        prefs = WebPreferences::sharedStandardPreferences();

    if (m_preferences == prefs)
        return ;

    prefs->willAddToWebView();

    WebPreferences *oldPrefs = m_preferences;

    WebCore::ObserverServiceData::createObserverService()->removeObserver(WebPreferences::webPreferencesChangedNotification(), m_webViewObserver);

    String identifier = oldPrefs->identifier();
    oldPrefs->didRemoveFromWebView();
    oldPrefs = 0; // Make sure we release the reference, since WebPreferences::removeReferenceForIdentifier will check for last reference to WebPreferences
    
    WebPreferences::removeReferenceForIdentifier(identifier.utf8().data());

    m_preferences = prefs;

    WebCore::ObserverServiceData::createObserverService()->registerObserver(WebPreferences::webPreferencesChangedNotification(), m_webViewObserver);

    m_preferences->postPreferencesChangesNotification();
}

WebPreferences* WebView::preferences()
{
    return m_preferences;
}

void WebView::setPreferencesIdentifier(const char* /*anIdentifier*/)
{
}

const char* WebView::preferencesIdentifier()
{
    return "";
}

void WebView::updateActiveStateSoon() const
{
//    SetTimer(m_viewWindow, UpdateActiveStateTimer, 0, 0);
}

void WebView::deleteBackingStoreSoon()
{
    m_deleteBackingStoreTimerActive = true;
//    SetTimer(m_viewWindow, DeleteBackingStoreTimer, delayBeforeDeletingBackingStoreMsec, 0);
}

void WebView::cancelDeleteBackingStoreSoon()
{
    if (!m_deleteBackingStoreTimerActive)
        return;
    m_deleteBackingStoreTimerActive = false;
//    KillTimer(m_viewWindow, DeleteBackingStoreTimer);
}

bool WebView::searchFor(const char* str, bool forward, bool caseFlag, bool wrapFlag)
{
    if (!m_page)
        return false;
    FindOptions opts;
    if (wrapFlag) opts |= WrapAround;
    if (!caseFlag) opts |= CaseInsensitive;
    if (!forward)opts |= Backwards;

	String searchString = String::fromUTF8(str);
	return m_page->findString(searchString, opts);
}

bool WebView::active()
{
    BalWidget *widget = m_viewWindow;
	if(widget)
	{
		return (bool) getv(widget->browser, MA_OWBBrowser_Active);
	}
	else
	{
		return true;
	}
}

void WebView::setFocus()
{
    FocusController* focusController = &(m_page->focusController());
    focusController->setFocused(true);
    if(!focusController->focusedFrame())
        focusController->setFocusedFrame(&(m_page->mainFrame()));
}

bool WebView::focused() const
{
    return m_page->focusController().isFocused();
}

void WebView::clearFocus()
{
    m_page->focusController().setFocused(false);
}

void WebView::updateActiveState()
{
    FocusController* focusController = &(m_page->focusController());
    focusController->setActive(active());
    focusController->setFocused(focused());
}

void WebView::updateFocusedAndActiveState()
{
    m_page->focusController().setActive(active());

    bool active = m_page->focusController().isActive();

    if(active)
    {
	FocusController *focusController = &(m_page->focusController());
	Frame *frame = focusController->focusedFrame();
	Frame* mainFrame = &(m_page->mainFrame());
	if (frame)
	    focusController->setFocused(true);
	else if(mainFrame)
	    focusController->setFocusedFrame(mainFrame);
    }
    else
    {
	FocusController *focusController = &(m_page->focusController());
	focusController->setFocused(false);
    }
}

String buffer(const char* url)
{
    String path;
    if (!url) {
        path = OWB_DATA;
        path.append("owbrc");
    } else
        path = url;

	OWBFile *configFile = new OWBFile(path);
    if (!configFile)
        return String();
    if (configFile->open('r') == -1) {
        delete configFile;
        return String();
    }
    String buffer(configFile->read(configFile->getSize()));
    configFile->close();
    delete configFile;

    return buffer;
}

void WebView::parseConfigFile(const char* url)
{
    int width = 0, height = 0;
    String fileBuffer = buffer(url);
    while (!fileBuffer.isEmpty()) {
		size_t eol = fileBuffer.find("\n");
		size_t delimiter = fileBuffer.find("=");

        String keyword = fileBuffer.substring(0, delimiter).stripWhiteSpace();
        String key = fileBuffer.substring(delimiter +  1, eol - delimiter).stripWhiteSpace();

        if (keyword == "width")
            width = key.toInt();
        if (keyword == "height")
            height = key.toInt();

        //Remove processed line from the buffer
        String truncatedBuffer = fileBuffer.substring(eol + 1, fileBuffer.length() - eol - 1);
        fileBuffer = truncatedBuffer;
    }

    if (width > 0 && height > 0)
        d->setFrameRect(IntRect(0, 0, width, height));
}


void WebView::executeCoreCommandByName(const char* name, const char* value)
{
    m_page->focusController().focusedOrMainFrame().editor().command(name).execute(value);
}

bool WebView::commandEnabled(const char* commandName)
{
    if (!commandName)
        return false;

    Editor::Command command = m_page->focusController().focusedOrMainFrame().editor().command(commandName);
    return command.isEnabled();
}

unsigned int WebView::markAllMatchesForText(const char* str, bool caseSensitive, bool highlight, unsigned int limit)
{
    if (!m_page)
        return 0;

    return m_page->markAllMatchesForText(str, caseSensitive ? TextCaseSensitive : TextCaseInsensitive, highlight, limit);
}

void WebView::unmarkAllTextMatches()
{
    if (!m_page)
        return ;

    m_page->unmarkAllTextMatches();
}

BalRectangle WebView::selectionRect()
{
    WebCore::Frame* frame = &(m_page->focusController().focusedOrMainFrame());

    if (frame) {
	IntRect ir = enclosingIntRect(frame->selection().selectionBounds());
        ir = frame->view()->convertToContainingWindow(ir);
        ir.move(-frame->view()->scrollOffset().width(), -frame->view()->scrollOffset().height());
        return ir;
    }

    return IntRect();
}

void WebView::setGroupName(const char* groupName)
{
    m_page->setGroupName(groupName);
}
    
const char* WebView::groupName()
{
    // We need to duplicate the translated string.
    CString groupName = m_page->groupName().utf8();
    return strdup(groupName.data());
}
    
double WebView::estimatedProgress()
{
    return m_page->progress().estimatedProgress();
}
    
bool WebView::isLoading()
{
    WebDataSource* dataSource = m_mainFrame->dataSource();

    bool isLoading = false;
    if (dataSource)
        isLoading = dataSource->isLoading();

    if (isLoading)
        return true;

    WebDataSource* provisionalDataSource = m_mainFrame->provisionalDataSource();
    if (provisionalDataSource)
        isLoading = provisionalDataSource->isLoading();
    return isLoading;
}

WebHitTestResults* WebView::elementAtPoint(BalPoint& point)
{
    Frame* frame = core(m_mainFrame);
    if (!frame)
        return 0;

    IntPoint webCorePoint(point);
    HitTestResult result = HitTestResult(webCorePoint);
    if (frame->contentRenderer())
        result = frame->eventHandler().hitTestResultAtPoint(webCorePoint, false);
    return WebHitTestResults::createInstance(result);
}

const char* WebView::selectedText()
{
    Frame* focusedFrame = (m_page) ? &(m_page->focusController().focusedOrMainFrame()) : 0;
    if (!focusedFrame)
        return NULL;

	return strdup(focusedFrame->editor().selectedText().utf8().data());
}

void WebView::centerSelectionInVisibleArea()
{
    Frame* coreFrame = core(m_mainFrame);
    if (!coreFrame)
        return ;

	coreFrame->selection().revealSelection(ScrollAlignment::alignCenterAlways);
}

WebDragOperation WebView::dragEnter(WebDragData* webDragData, int identity,
                                    const BalPoint& clientPoint, const BalPoint& screenPoint, 
                                    WebDragOperation operationsAllowed)
{
    ASSERT(!m_currentDragData);

    m_currentDragData = webDragData;
    m_dragIdentity = identity;
    m_operationsAllowed = operationsAllowed;

    DragData dragData(
        m_currentDragData->platformDragData()->m_dragDataRef,
        clientPoint,
        screenPoint,
        static_cast<DragOperation>(operationsAllowed));

    m_dropEffect = DropEffectDefault;
    m_dragTargetDispatch = true;
    DragOperation effect = m_page->dragController().dragEntered(dragData);
    // Mask the operation against the drag source's allowed operations.
    if ((effect & dragData.draggingSourceOperationMask()) != effect)
        effect = DragOperationNone;
    m_dragTargetDispatch = false;

    if (m_dropEffect != DropEffectDefault) {
        m_dragOperation = (m_dropEffect != DropEffectNone) ? WebDragOperationCopy
                                                           : WebDragOperationNone;
    } else
        m_dragOperation = static_cast<WebDragOperation>(effect);
    return m_dragOperation;
}

WebDragOperation WebView::dragOver(const BalPoint& clientPoint, const BalPoint& screenPoint, WebDragOperation operationsAllowed)
{
    ASSERT(m_currentDragData);

    m_operationsAllowed = operationsAllowed;
    DragData dragData(
	m_currentDragData->platformDragData()->m_dragDataRef,
        clientPoint,
        screenPoint,
        static_cast<DragOperation>(operationsAllowed));

    m_dropEffect = DropEffectDefault;
    m_dragTargetDispatch = true;
    DragOperation effect = m_page->dragController().dragUpdated(dragData);
    // Mask the operation against the drag source's allowed operations.
    if ((effect & dragData.draggingSourceOperationMask()) != effect)
        effect = DragOperationNone;
    m_dragTargetDispatch = false;

    if (m_dropEffect != DropEffectDefault) {
        m_dragOperation = (m_dropEffect != DropEffectNone) ? WebDragOperationCopy
                                                           : WebDragOperationNone;
    } else
        m_dragOperation = static_cast<WebDragOperation>(effect);
    return m_dragOperation;
}

void WebView::dragTargetDragLeave()
{
    ASSERT(m_currentDragData);

    DragData dragData(
        m_currentDragData->platformDragData()->m_dragDataRef,
        IntPoint(),
        IntPoint(),
        static_cast<DragOperation>(m_operationsAllowed));

    m_dragTargetDispatch = true;
    m_page->dragController().dragExited(dragData);
    m_dragTargetDispatch = false;

    m_currentDragData = 0;
    m_dropEffect = DropEffectDefault;
    m_dragOperation = WebDragOperationNone;
    m_dragIdentity = 0;
}

void WebView::dragTargetDrop(const BalPoint& clientPoint,
                                 const BalPoint& screenPoint)
{
    ASSERT(m_currentDragData);

    // If this webview transitions from the "drop accepting" state to the "not
    // accepting" state, then our IPC message reply indicating that may be in-
    // flight, or else delayed by javascript processing in this webview.  If a
    // drop happens before our IPC reply has reached the browser process, then
    // the browser forwards the drop to this webview.  So only allow a drop to
    // proceed if our webview m_dragOperation state is not DragOperationNone.

    if (m_dragOperation == WebDragOperationNone) { // IPC RACE CONDITION: do not allow this drop.
        dragTargetDragLeave();
        return;
    }

    DragData dragData(
        m_currentDragData->platformDragData()->m_dragDataRef,
        clientPoint,
        screenPoint,
        static_cast<DragOperation>(m_operationsAllowed));

    m_dragTargetDispatch = true;
    m_page->dragController().performDragOperation(dragData);
    m_dragTargetDispatch = false;

    m_currentDragData = 0;
    m_dropEffect = DropEffectDefault;
    m_dragOperation = WebDragOperationNone;
    m_dragIdentity = 0;
}

int WebView::dragIdentity()
{
    if (m_dragTargetDispatch)
        return m_dragIdentity;
    return 0;
}

void WebView::dragSourceEndedAt(const BalPoint& clientPoint, const BalPoint& screenPoint, WebDragOperation operation)
{
    PlatformMouseEvent pme(clientPoint,
                           screenPoint,
                           LeftButton, PlatformEvent::MouseMoved, 0, false, false, false,
                           false, 0, WebCore::ForceAtClick);
    m_page->mainFrame().eventHandler().dragSourceEndedAt(pme,
        static_cast<DragOperation>(operation));
}

void WebView::dragSourceSystemDragEnded()
{
    m_page->dragController().dragEnded();
}

void WebView::moveDragCaretToPoint(BalPoint& /*point*/)
{
}

void WebView::removeDragCaret()
{
}

void WebView::setDrawsBackground(bool /*drawsBackground*/)
{
}

bool WebView::drawsBackground()
{
    return false;
}

void WebView::setMainFrameURL(const char* /*urlString*/)
{
}

const char* WebView::mainFrameURL()
{
    WebDataSource* dataSource(m_mainFrame->provisionalDataSource());
    if (!dataSource) {
        dataSource = m_mainFrame->dataSource();
        if (!dataSource)
            return 0;
    }

    WebMutableURLRequest* request(dataSource->request());
    if (!request)
        return 0;

    return request->_URL();
}

DOMDocument* WebView::mainFrameDocument()
{
    return m_mainFrame->domDocument();
}

const char* WebView::mainFrameTitle()
{
    return m_mainFrame->name();
}

void WebView::registerURLSchemeAsLocal(const char* scheme)
{
    SchemeRegistry::registerURLSchemeAsLocal(scheme);
}

void WebView::setSmartInsertDeleteEnabled(bool flag)
{
    m_smartInsertDeleteEnabled = !!flag;
    if (m_smartInsertDeleteEnabled)
        setSelectTrailingWhitespaceEnabled(false);
}
    
bool WebView::smartInsertDeleteEnabled()
{
    return m_smartInsertDeleteEnabled ? true : false;
}

void WebView::setSelectTrailingWhitespaceEnabled(bool flag)
{
    m_selectTrailingWhitespaceEnabled = !!flag;
    if (m_selectTrailingWhitespaceEnabled)
        setSmartInsertDeleteEnabled(false);
}

bool WebView::isSelectTrailingWhitespaceEnabled()
{
    return m_selectTrailingWhitespaceEnabled;
}

void WebView::setContinuousSpellCheckingEnabled(bool flag)
{
    if (continuousSpellCheckingEnabled != !!flag) {
        continuousSpellCheckingEnabled = !!flag;
        WebPreferences* prefs = preferences();
        if (prefs)
            prefs->setContinuousSpellCheckingEnabled(flag);
		SetAttrs(app, MA_OWBApp_ContinuousSpellChecking, flag ? 1 : 0, TAG_DONE);
    }
    
    bool spellCheckingEnabled = isContinuousSpellCheckingEnabled();
    if (!spellCheckingEnabled)
        m_mainFrame->unmarkAllMisspellings();
    /*else
        preflightSpellChecker();*/
}

bool WebView::isContinuousSpellCheckingEnabled()
{
    return (continuousSpellCheckingEnabled && continuousCheckingAllowed()) ? true : false;
}

int WebView::spellCheckerDocumentTag()
{
    // we just use this as a flag to indicate that we've spell checked the document
    // and need to close the spell checker out when the view closes.
    m_hasSpellCheckerDocumentTag = true;
    return 0;
}

bool WebView::continuousCheckingAllowed()
{
    static bool allowContinuousSpellChecking = true;
    static bool readAllowContinuousSpellCheckingDefault = false;
    if (!readAllowContinuousSpellCheckingDefault) {
        WebPreferences* prefs = preferences();
        if (prefs) {
            bool allowed = prefs->allowContinuousSpellChecking();
            allowContinuousSpellChecking = !!allowed;
        }
        readAllowContinuousSpellCheckingDefault = true;
    }
    return allowContinuousSpellChecking;
}

bool WebView::hasSelectedRange()
{
    return m_page->mainFrame().selection().isRange();
}

bool WebView::cutEnabled()
{
    Editor &editor = m_page->focusController().focusedOrMainFrame().editor();
    return editor.canCut() || editor.canDHTMLCut();
}

bool WebView::copyEnabled()
{
    Editor &editor = m_page->focusController().focusedOrMainFrame().editor();
    return editor.canCopy() || editor.canDHTMLCopy();
}

bool WebView::pasteEnabled()
{
    Editor &editor = m_page->focusController().focusedOrMainFrame().editor();
    return editor.canPaste() || editor.canDHTMLPaste();
}

bool WebView::deleteEnabled()
{
    return m_page->focusController().focusedOrMainFrame().editor().canDelete();
}

bool WebView::editingEnabled()
{
    return m_page->focusController().focusedOrMainFrame().editor().canEdit();
}

bool WebView::isGrammarCheckingEnabled()
{
    return grammarCheckingEnabled ? true : false;
}

void WebView::setGrammarCheckingEnabled(bool enabled)
{
    if (grammarCheckingEnabled == !!enabled)
        return ;

    grammarCheckingEnabled = !!enabled;
    WebPreferences* prefs = preferences();
    if (prefs)
        prefs->setGrammarCheckingEnabled(enabled);

    // We call _preflightSpellChecker when turning continuous spell checking on, but we don't need to do that here
    // because grammar checking only occurs on code paths that already preflight spell checking appropriately.

    bool grammarEnabled = isGrammarCheckingEnabled();
    if (!grammarEnabled)
        m_mainFrame->unmarkAllBadGrammar();
}

void WebView::replaceSelectionWithText(const char* text)
{
    Position start = m_page->mainFrame().selection().selection().start();
    m_page->focusController().focusedOrMainFrame().editor().insertText(text, 0);
    m_page->mainFrame().selection().setBase(start);
}
    
void WebView::replaceSelectionWithMarkupString(const char* /*markupString*/)
{
}
    
void WebView::replaceSelectionWithArchive(WebArchive* /*archive*/)
{
}
    
void WebView::deleteSelection()
{
    Editor &editor = m_page->focusController().focusedOrMainFrame().editor();
    editor.deleteSelectionWithSmartDelete(editor.canSmartCopyOrDelete());
}

void WebView::clearSelection()
{
    m_page->focusController().focusedOrMainFrame().selection().clear();
}

void WebView::clearMainFrameName()
{
    m_page->mainFrame().tree().clearName();
}

void WebView::copy()
{
    m_page->focusController().focusedOrMainFrame().editor().command("Copy").execute();
}

void WebView::cut()
{
    m_page->focusController().focusedOrMainFrame().editor().command("Cut").execute();
}

void WebView::paste()
{
    m_page->focusController().focusedOrMainFrame().editor().command("Paste").execute();
}

void WebView::copyURL(const char* url)
{
    m_page->focusController().focusedOrMainFrame().editor().copyURL(URL(ParsedURLString, url), "");
}


void WebView::copyFont()
{
}
    
void WebView::pasteFont()
{
}
    
void WebView::delete_()
{
    m_page->focusController().focusedOrMainFrame().editor().command("Delete").execute();
}
    
void WebView::pasteAsPlainText()
{
}

void WebView::pasteAsRichText()
{
}
    
void WebView::changeFont()
{
}
    
void WebView::changeAttributes()
{
}
    
void WebView::changeDocumentBackgroundColor()
{
}
    
void WebView::changeColor()
{
}
    
void WebView::alignCenter()
{
}
    
void WebView::alignJustified()
{
}
    
void WebView::alignLeft()
{
}
    
void WebView::alignRight()
{
}
    
void WebView::checkSpelling()
{
    core(m_mainFrame)->editor().advanceToNextMisspelling();
}
    
void WebView::showGuessPanel()
{
    core(m_mainFrame)->editor().advanceToNextMisspelling(true);
    //m_editingDelegate->showSpellingUI(TRUE);
}
    
void WebView::performFindPanelAction()
{
}
    
void WebView::startSpeaking()
{
}
    
void WebView::stopSpeaking()
{
}

void WebView::notifyPreferencesChanged(WebPreferences* preferences)
{
    ASSERT(preferences == m_preferences);

    BalWidget *widget = m_viewWindow;
    String str;
    int size;
    bool enabled;
    bool applyDefault = false;

    Settings* settings = &m_page->settings();

    str = preferences->cursiveFontFamily();
    settings->setCursiveFontFamily(str);

    size = preferences->defaultFixedFontSize();
    settings->setDefaultFixedFontSize(size);

    size = preferences->defaultFontSize();
    settings->setDefaultFontSize(size);

    str = preferences->defaultTextEncodingName();
    settings->setDefaultTextEncodingName(str);

    str = preferences->fantasyFontFamily();
    settings->setFantasyFontFamily(str);

    str = preferences->fixedFontFamily();
    settings->setFixedFontFamily(str);

#if ENABLE(VIDEO_TRACK)
    enabled = preferences->shouldDisplaySubtitles();
    settings->setShouldDisplaySubtitles(enabled);
    
    enabled = preferences->shouldDisplayCaptions();
    settings->setShouldDisplayCaptions(enabled);

    enabled = preferences->shouldDisplayTextDescriptions();
    settings->setShouldDisplayTextDescriptions(enabled);
#endif

    enabled = preferences->isJavaEnabled();
    settings->setJavaEnabled(enabled);

    enabled = preferences->isXSSAuditorEnabled();
    settings->setXSSAuditorEnabled(!!enabled);

    enabled = preferences->javaScriptCanOpenWindowsAutomatically();
    settings->setJavaScriptCanOpenWindowsAutomatically(enabled);

    size = preferences->minimumFontSize();
    settings->setMinimumFontSize(size);

    size = preferences->minimumLogicalFontSize();
    settings->setMinimumLogicalFontSize(size);

    enabled = preferences->arePlugInsEnabled();
    settings->setPluginsEnabled(!!enabled);

    str = preferences->sansSerifFontFamily();
    settings->setSansSerifFontFamily(str);

    str = preferences->serifFontFamily();
    settings->setSerifFontFamily(str);

    str = preferences->standardFontFamily();
    settings->setStandardFontFamily(str);

	/* Following ones need special care because they can be overridden by URLPrefs or temporary menu settings. */
	applyDefault = widget == NULL || (DoMethod(app, MM_URLPrefsGroup_MatchesURL, getv(widget->browser, MA_OWBBrowser_URL)) == 0 && getv(widget->browser, MA_OWBBrowser_JavaScriptEnabled) == JAVASCRIPT_DEFAULT);

	if(applyDefault)
	{
	    enabled = preferences->isJavaScriptEnabled();
		settings->setScriptEnabled(enabled);
	}

//	applyDefault = widget == NULL || getv(widget->browser, MA_OWBBrowser_PrivateBrowsing) == FALSE;
//
//	if(applyDefault)
//	{
//	    enabled = preferences->privateBrowsingEnabled();
//	    settings->setPrivateBrowsingEnabled(!!enabled);
//	}

	applyDefault = widget == NULL || (DoMethod(app, MM_URLPrefsGroup_MatchesURL, getv(widget->browser, MA_OWBBrowser_URL)) == 0 && getv(widget->browser, MA_OWBBrowser_LoadImagesAutomatically) == IMAGES_DEFAULT);

	if(applyDefault)
	{
	    enabled = preferences->loadsImagesAutomatically();
	    settings->setLoadsImagesAutomatically(!!enabled);
	}

	applyDefault = widget == NULL || (DoMethod(app, MM_URLPrefsGroup_MatchesURL, getv(widget->browser, MA_OWBBrowser_URL)) == 0 && getv(widget->browser, MA_OWBBrowser_PluginsEnabled) == PLUGINS_DEFAULT);

	if(applyDefault)
	{
	    enabled = preferences->arePlugInsEnabled();
		settings->setPluginsEnabled(!!enabled);
	}

	applyDefault = widget == NULL || (DoMethod(app, MM_URLPrefsGroup_MatchesURL, getv(widget->browser, MA_OWBBrowser_URL)) == 0);

	if(applyDefault)
	{
		enabled = preferences->localStorageEnabled();
		settings->setLocalStorageEnabled(!!enabled);
	}

	/* */

	enabled = preferences->areImagesEnabled();
	settings->setImagesEnabled(!!enabled);

	//    enabled = preferences->isCSSRegionsEnabled();
	//    settings->setCSSRegionsEnabled(!!enabled);

    enabled = preferences->userStyleSheetEnabled();
    if (enabled) {
        str = preferences->userStyleSheetLocation();
        settings->setUserStyleSheetLocation(URL(ParsedURLString, str));
    } else {
        settings->setUserStyleSheetLocation(URL());
    }

    enabled = preferences->shouldPrintBackgrounds();
    settings->setShouldPrintBackgrounds(!!enabled);

    enabled = preferences->textAreasAreResizable();
    settings->setTextAreasAreResizable(!!enabled);

    WebKitEditableLinkBehavior behavior = preferences->editableLinkBehavior();
    settings->setEditableLinkBehavior((EditableLinkBehavior)behavior);

    enabled = preferences->requestAnimationFrameEnabled();
    settings->setRequestAnimationFrameEnabled(!!enabled);

    enabled = preferences->usesPageCache();
    settings->setUsesPageCache(!!enabled);

    enabled = preferences->isDOMPasteAllowed();
    settings->setDOMPasteAllowed(!!enabled);

    enabled = preferences->zoomsTextOnly();
    if (m_zoomsTextOnly != !!enabled)
        setZoomMultiplier(m_zoomMultiplier, enabled);

    settings->setShowsURLsInToolTips(true);
    settings->setForceFTPDirectoryListings(false);
    settings->setDeveloperExtrasEnabled(developerExtrasEnabled());
    settings->setNeedsSiteSpecificQuirks(s_allowSiteSpecificHacks);

    FontSmoothingType smoothingType = preferences->fontSmoothing();
    settings->setFontRenderingMode(smoothingType != FontSmoothingTypeWindows ? NormalRenderingMode : AlternateRenderingMode);

    enabled = preferences->authorAndUserStylesEnabled();
    settings->setAuthorAndUserStylesEnabled(enabled);

    enabled = preferences->offlineWebApplicationCacheEnabled();
    settings->setOfflineWebApplicationCacheEnabled(enabled);

    enabled = preferences->databasesEnabled();
    DatabaseManager::singleton().setIsAvailable(enabled);

    enabled = preferences->experimentalNotificationsEnabled();
    settings->setExperimentalNotificationsEnabled(enabled);

    enabled = preferences->isWebSecurityEnabled();
    settings->setWebSecurityEnabled(enabled);

    enabled = preferences->allowUniversalAccessFromFileURLs();
    settings->setAllowUniversalAccessFromFileURLs(!!enabled);

    enabled = preferences->allowFileAccessFromFileURLs();
    settings->setAllowFileAccessFromFileURLs(!!enabled);

    enabled = preferences->javaScriptCanAccessClipboard();
    settings->setJavaScriptCanAccessClipboard(!!enabled);

    enabled = preferences->isXSSAuditorEnabled();
    settings->setXSSAuditorEnabled(!!enabled);

    enabled = preferences->isFrameFlatteningEnabled();
    settings->setFrameFlatteningEnabled(enabled);
    
    enabled = preferences->webGLEnabled();
    settings->setWebGLEnabled(enabled);

    enabled = preferences->acceleratedCompositingEnabled();
    settings->setAcceleratedCompositingEnabled(enabled);

    enabled = preferences->showDebugBorders();
    settings->setShowDebugBorders(enabled);

    enabled = preferences->showRepaintCounter();
    settings->setShowRepaintCounter(enabled);

    str = preferences->localStorageDatabasePath();
    settings->setLocalStorageDatabasePath(str);

	/*
    enabled = preferences->localStorageEnabled();
    settings->setLocalStorageEnabled(enabled);
	*/

    enabled = preferences->DNSPrefetchingEnabled();
    settings->setDNSPrefetchingEnabled(enabled);

    //enabled = preferences->memoryInfoEnabled();
    //settings->setMemoryInfoEnabled(enabled);

    enabled = preferences->hyperlinkAuditingEnabled();
    settings->setHyperlinkAuditingEnabled(enabled);

    enabled = preferences->shouldInvertColors();
    setShouldInvertColors(enabled);

    /* int limit = preferences->memoryLimit();
    WTF::setMemoryLimit(limit); */

    enabled = preferences->allowScriptsToCloseWindows();
    settings->setAllowScriptsToCloseWindows(enabled);
    
    settings->setSpatialNavigationEnabled(preferences->spatialNavigationEnabled());

#if ENABLE(FULLSCREEN_API)
	settings->setFullScreenEnabled(false);
#endif

    updateSharedSettingsFromPreferencesIfNeeded(preferences);
}

void updateSharedSettingsFromPreferencesIfNeeded(WebPreferences* preferences)
{
    if (preferences != WebPreferences::sharedStandardPreferences())
        return ;
}

BalWidget* WebView::viewWindow()
{
    return m_viewWindow;
}

void WebView::setViewWindow(BalWidget* view)
{
    m_viewWindow = view;
    d->setViewWindow(view);
}

BalPoint WebView::scrollOffset()
{
    if (m_page) {
        IntSize offsetIntSize = m_page->mainFrame().view()->scrollOffset();
        return IntPoint(offsetIntSize.width(), offsetIntSize.height());
    } else
        return IntPoint();
}

void WebView::scrollBy(BalPoint offset)
{
    if (m_page) {
        IntPoint p(offset);
        m_page->mainFrame().view()->scrollBy(IntSize(p.x(), p.y()));
#if OS(AROS)
        BalWidget *widget = m_viewWindow;

        if(widget)
        {
            DoMethod(widget->browser, MM_OWBBrowser_Autofill_HidePopup);
        }
#endif
    }
}

BalPoint WebView::scheduledScrollOffset()
{
	return m_scheduledScrollOffset;
}

void WebView::setScheduledScrollOffset(BalPoint offset)
{
	m_scheduledScrollOffset = offset;
}

void WebView::scrollWithDirection(::ScrollDirection direction)
{
    switch (direction) {
    case Up:
        m_page->mainFrame().view()->scrollBy(IntSize(0, (int)-Scrollbar::pixelsPerLineStep()));
        break;
    case Down:
        m_page->mainFrame().view()->scrollBy(IntSize(0, (int)Scrollbar::pixelsPerLineStep()));
        break;
    case Left:
        m_page->mainFrame().view()->scrollBy(IntSize((int)-Scrollbar::pixelsPerLineStep(), 0));
        break;
    case Right:
        m_page->mainFrame().view()->scrollBy(IntSize((int)Scrollbar::pixelsPerLineStep(), 0));
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

BalRectangle WebView::visibleContentRect()
{
    FloatRect visibleContent = m_page->mainFrame().view()->visibleContentRect();
    return IntRect((int)visibleContent.x(), (int)visibleContent.y(), (int)visibleContent.maxX(), (int)visibleContent.maxY());
}


bool WebView::canHandleRequest(WebMutableURLRequest *request)
{
    return !!canHandleRequest(request->resourceRequest());
}

void WebView::clearFocusNode()
{
    if (m_page)
        m_page->focusController().setFocusedElement(0, 0);
}

void WebView::setInitialFocus(bool forward)
{
    if (m_page) {
        Frame* frame = &(m_page->focusController().focusedOrMainFrame());
        frame->document()->setFocusedElement(0);
        m_page->focusController().setInitialFocus(forward ? FocusDirectionForward : FocusDirectionBackward, 0);
    }
}

void WebView::setTabKeyCyclesThroughElements(bool cycles)
{
    if (m_page)
        m_page->setTabKeyCyclesThroughElements(!!cycles);
}

bool WebView::tabKeyCyclesThroughElements()
{
    return m_page && m_page->tabKeyCyclesThroughElements() ? true : false;
}

void WebView::setAllowSiteSpecificHacks(bool allow)
{
    s_allowSiteSpecificHacks = !!allow;
    // FIXME: This sets a global so it needs to call notifyPreferencesChanged
    // on all WebView objects (not just itself).
}

void WebView::addAdditionalPluginDirectory(const char* directory)
{
    PluginDatabase::installedPlugins()->addExtraPluginDirectory(directory);
}

void WebView::loadBackForwardListFromOtherView(WebView* otherView)
{
    if (!m_page)
        return ;
    
    // It turns out the right combination of behavior is done with the back/forward load
    // type.  (See behavior matrix at the top of WebFramePrivate.)  So we copy all the items
    // in the back forward list, and go to the current one.
	BackForwardList* backForwardList = static_cast<WebCore::BackForwardList*>(m_page->backForward().client());
    ASSERT(!backForwardList->currentItem()); // destination list should be empty

	BackForwardClient* otherBackForwardList = static_cast<WebCore::BackForwardList*>(otherView->m_page->backForward().client());
    if (!otherView->m_page->backForward().currentItem())
        return ; // empty back forward list, bail
    
    HistoryItem* newItemToGoTo = 0;

    int lastItemIndex = otherBackForwardList->forwardListCount();
    for (int i = -otherBackForwardList->backListCount(); i <= lastItemIndex; ++i) {
        if (!i) {
            // If this item is showing , save away its current scroll and form state,
            // since that might have changed since loading and it is normally not saved
            // until we leave that page.
            otherView->m_page->mainFrame().loader().history().saveDocumentAndScrollState();
        }
        Ref<HistoryItem> newItem = otherBackForwardList->itemAtIndex(i)->copy();
        if (!i) 
            newItemToGoTo = newItem.ptr();
        backForwardList->addItem(WTF::move(newItem));
    }
    
    ASSERT(newItemToGoTo);
    m_page->goToItem(*newItemToGoTo, FrameLoadType::IndexedBackForward);
}

void WebView::clearUndoRedoOperations()
{
    if (m_page)
	m_page->focusController().focusedOrMainFrame().editor().clearUndoRedoOperations();
}

bool WebView::shouldClose()
{
    if (m_page)
	return m_page->mainFrame().loader().shouldClose() ? true : false;
    return true;
}

void WebView::setProhibitsMainFrameScrolling(bool b)
{
    if (!m_page)
        return ;

    m_page->mainFrame().view()->setProhibitsScrolling(b);
}

void WebView::setShouldApplyMacFontAscentHack(bool b)
{
    //SimpleFontData::setShouldApplyMacAscentHack(b);
}


void WebView::windowAncestryDidChange()
{
    updateActiveState();
}

/*void WebView::paintDocumentRectToContext(IntRect rect, PlatformGraphicsContext *pgc)
{
    Frame* frame = m_page->mainFrame();
    if (!frame)
        return ;

    FrameView* view = frame->view();
    if (!view)
        return ;

    // We can't paint with a layout still pending.
    view->layoutIfNeededRecursive();

    GraphicsContext gc(pgc);
    gc.save();
    int width = rect.width();
    int height = rect.height();
    FloatRect dirtyRect;
    dirtyRect.setWidth(width);
    dirtyRect.setHeight(height);
    gc.clip(dirtyRect);
    gc.translate(-rect.x(), -rect.y());
    view->paint(&gc, rect);
    gc.restore();
}*/

/*WebCore::Image* WebView::backingStore()
{
    WebCore::Image* hBitmap = m_backingStoreBitmap.get();
    return hBitmap;
}*/

void WebView::setTransparent(bool transparent)
{
    if (m_transparent == !!transparent)
        return;

    m_transparent = transparent;
    if (m_mainFrame)
        m_mainFrame->updateBackground();
}

bool WebView::transparent()
{
    return m_transparent;
}

void WebView::setMediaVolume(float volume)                                                        
{                                                                                                                      
    if (!m_page)                                                                                                       
        return ; 
                                                                                                                
    m_page->setMediaVolume(volume);                                                                                    
}                                                                                                                      
                                                                                                                       
float WebView::mediaVolume()
{                                                                                                                      
    if (!m_page)                                                                                                       
        return 0.0;
                                                                                                                       
    return m_page->mediaVolume();                                                                                   
}  

void WebView::setMemoryCacheDelegateCallsEnabled(bool enabled)
{
    m_page->setMemoryCacheClientCallsEnabled(enabled);
}

void WebView::setDefersCallbacks(bool defersCallbacks)
{
    if (!m_page)
        return;

    m_page->setDefersLoading(defersCallbacks);
}

bool WebView::defersCallbacks()
{
    if (!m_page)
        return false;

    return m_page->defersLoading();
}

void WebView::popupMenuHide()
{
    d->popupMenuHide();
}

void WebView::popupMenuShow(void* userData)
{
    d->popupMenuShow(userData);
}

void WebView::fireWebKitTimerEvents()
{
    d->fireWebKitTimerEvents();
}

void WebView::fireWebKitThreadEvents()
{
    d->fireWebKitThreadEvents();
}


void WebView::setJavaScriptURLsAreAllowed(bool areAllowed)
{
  //    m_page->setJavaScriptURLsAreAllowed(areAllowed);
}

void WebView::resize(BalRectangle r)
{
    d->resize(r);
}


void WebView::move(BalPoint lastPos, BalPoint newPos)
{
    d->move(lastPos, newPos);
}

unsigned WebView::backgroundColor()
{
    if (!m_mainFrame)
        return 0;
    Frame* coreFrame = core(m_mainFrame);
    return coreFrame->view()->baseBackgroundColor().rgb();
}

void WebView::allowLocalLoadsForAll()
{
    SecurityPolicy::setLocalLoadPolicy(SecurityPolicy::AllowLocalLoadsForAll);
}

WebInspector* WebView::inspector() {
	if (!m_webInspector)
		m_webInspector = WebInspector::createInstance(this, m_inspectorClient);
    return m_webInspector;
}


Page* core(WebView* webView)
{
    Page* page = 0;

    page = webView->page();

    return page;
}

bool WebView::invalidateBackingStore(const WebCore::IntRect* rect)
{
    return false;
}

void WebView::addOriginAccessWhitelistEntry(const char* sourceOrigin, const char* destinationProtocol, const char* destinationHost, bool allowDestinationSubDomains) const
{
    SecurityPolicy::addOriginAccessWhitelistEntry(SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubDomains);
}

void WebView::removeOriginAccessWhitelistEntry(const char* sourceOrigin, const char* destinationProtocol, const char* destinationHost, bool allowDestinationSubdomains) const
{
    SecurityPolicy::removeOriginAccessWhitelistEntry(SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void WebView::resetOriginAccessWhitelists() const
{
    SecurityPolicy::resetOriginAccessWhitelists();
}

void WebView::setHistoryDelegate(TransferSharedPtr<WebHistoryDelegate> historyDelegate)
{
    m_historyDelegate = historyDelegate;
}

TransferSharedPtr<WebHistoryDelegate> WebView::historyDelegate() const
{
    return m_historyDelegate;
}

void WebView::addVisitedLinks(const char** visitedURLs, unsigned visitedURLCount)
{
    auto& visitedLinkStore = WebVisitedLinkStore::singleton();

    for (unsigned i = 0; i < visitedURLCount; ++i)
    {
        const char * url = visitedURLs[i];
        unsigned length = strlen(url);
        visitedLinkStore.addVisitedLink(String(url, length));
    }
}

void WebView::stopLoading(bool stop)
{
    m_isStopped = stop;
    if (stop)
        core(mainFrame())->loader().stopLoading(UnloadEventPolicyUnloadAndPageHide);
}

void WebView::enterFullscreenForNode(WebCore::Node* element)
{
    BalWidget *widget = m_viewWindow;

	if(widget)
	{
		DoMethod(widget->browser, MM_OWBBrowser_VideoEnterFullPage, element, TRUE);
	}
}

void WebView::exitFullscreen()
{
    BalWidget *widget = m_viewWindow;

	if(widget)
	{
		DoMethod(widget->browser, MM_OWBBrowser_VideoEnterFullPage, NULL, FALSE);
	}
}

#if ENABLE(FULLSCREEN_API)
void WebView::enterFullScreenForElement(WebCore::Element* element)
{
    BalWidget *widget = m_viewWindow;

    if(widget)
	{
	    DoMethod(widget->browser, MM_OWBBrowser_VideoEnterFullPage, element, TRUE);
	}
}

void WebView::exitFullScreenForElement(WebCore::Element* element)
{
    BalWidget *widget = m_viewWindow;

    if(widget)
	{
	    DoMethod(widget->browser, MM_OWBBrowser_VideoEnterFullPage, NULL, FALSE);
	}
}
#endif

const char* WebView::inspectorSettings() const
{
    return m_inspectorSettings.c_str();
}

void WebView::setInspectorSettings(const char* settings)
{
    m_inspectorSettings = settings;
}


const char* WebView::encodeHostName(const char* source)
{
    UChar destinationBuffer[HOST_NAME_BUFFER_LENGTH];

    String src = String::fromUTF8(source);
    int length = src.length();

	if (src.find("%") != notFound)
        src = decodeURLEscapeSequences(src);

    auto upconvertedCharacters = StringView(src).upconvertedCharacters();
    const UChar* sourceBuffer = upconvertedCharacters;

    bool error = false;
    int32_t numCharactersConverted = WTF::Unicode::IDNToASCII(sourceBuffer, length, destinationBuffer, HOST_NAME_BUFFER_LENGTH, &error);

    if (error)
        return source;
    if (numCharactersConverted == length && memcmp(sourceBuffer, destinationBuffer, length * sizeof(UChar)) == 0) {
        return source;
    }
   
    if (!numCharactersConverted)
        return source;

    String result(destinationBuffer, numCharactersConverted);
    return strdup(result.utf8().data()); 
}

const char* WebView::decodeHostName(const char* source)
{
    UChar destinationBuffer[HOST_NAME_BUFFER_LENGTH];

    String src = String::fromUTF8(source);


    auto upconvertedCharacters = StringView(src).upconvertedCharacters();
    const UChar* sourceBuffer = upconvertedCharacters;
    int length = src.length();

    bool error = false;
    int32_t numCharactersConverted = WTF::Unicode::IDNToUnicode(sourceBuffer, length, destinationBuffer, HOST_NAME_BUFFER_LENGTH, &error);

    if (error)
        return source;

    if (numCharactersConverted == length && memcmp(sourceBuffer, destinationBuffer, length * sizeof(UChar)) == 0) {
        return source;
    }
    if (!WTF::Unicode::allCharactersInIDNScriptWhiteList(destinationBuffer, numCharactersConverted)) {
        return source; 
    }

    if (!numCharactersConverted)
        return source;

    String result(destinationBuffer, numCharactersConverted);
    return strdup(result.utf8().data()); 
}

void WebView::addChildren(WebWindow* webWindow)
{
    if (webWindow)
        m_children.push_back(webWindow);
}

void WebView::removeChildren(WebWindow* webWindow)
{
    vector<WebWindow*>::iterator it = m_children.begin();
    for(; it != m_children.end(); ++it) {
        if ((*it) == webWindow) {
            m_children.erase(it);
            break;
        }
    }
}

void WebView::sendExposeEvent(BalRectangle rect)
{
    addToDirtyRegion(rect);
    d->sendExposeEvent(rect);
}

bool WebView::screenshot(int &requested_width, int& requested_height, void *imageData)
{
	return d->screenshot(requested_width, requested_height, (Vector<char> *) imageData);
}

bool WebView::screenshot(char* path)
{
	String p = String(path);
	return d->screenshot(p);
}
