/*
 * Copyright (C) 2010-2019 Apple Inc. All rights reserved.
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
#include "AutoCorrectionCallback.h"
#include "Connection.h"
#include "ContextMenuContextData.h"
#include "DownloadID.h"
#include "DragControllerAction.h"
#include "EditingRange.h"
#include "EditorState.h"
#include "FocusedElementInformation.h"
#include "GeolocationPermissionRequestManagerProxy.h"
#include "HiddenPageThrottlingAutoIncreasesCounter.h"
#include "LayerTreeContext.h"
#include "MessageSender.h"
#include "NotificationPermissionRequestManagerProxy.h"
#include "PageLoadState.h"
#include "ProcessTerminationReason.h"
#include "ProcessThrottler.h"
#include "SandboxExtension.h"
#include "ShareSheetCallbackID.h"
#include "ShareableBitmap.h"
#include "SuspendedPageProxy.h"
#include "SystemPreviewController.h"
#include "UserMediaPermissionRequestManagerProxy.h"
#include "VisibleContentRectUpdateInfo.h"
#include "VisibleWebPageCounter.h"
#include "WKBase.h"
#include "WKPagePrivate.h"
#include "WebColorPicker.h"
#include "WebContextMenuItemData.h"
#include "WebCoreArgumentCoders.h"
#include "WebDataListSuggestionsDropdown.h"
#include "WebFrameProxy.h"
#include "WebPageCreationParameters.h"
#include "WebPageDiagnosticLoggingClient.h"
#include "WebPageInjectedBundleClient.h"
#include "WebPaymentCoordinatorProxy.h"
#include "WebPreferences.h"
#include "WebPageProxyMessages.h"
#include "WebPopupMenuProxy.h"
#include "WebProcessLifetimeTracker.h"
#include "WebUndoStepID.h"
#include "WebsitePoliciesData.h"
#include <WebCore/ActivityState.h>
#include <WebCore/AutoplayEvent.h>
#include <WebCore/Color.h>
#include <WebCore/DragActions.h>
#include <WebCore/EventTrackingRegions.h>
#include <WebCore/FontAttributes.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/FrameView.h> // FIXME: Move LayoutViewportConstraint to its own file and stop including this.
#include <WebCore/InputMode.h>
#include <WebCore/LayoutPoint.h>
#include <WebCore/LayoutSize.h>
#include <WebCore/MediaPlaybackTargetContext.h>
#include <WebCore/MediaProducer.h>
#include <WebCore/PlatformEvent.h>
#include <WebCore/PlatformScreen.h>
#include <WebCore/PointerID.h>
#include <WebCore/ScrollTypes.h>
#include <WebCore/SearchPopupMenu.h>
#include <WebCore/TextChecking.h>
#include <WebCore/TextGranularity.h>
#include <WebCore/UserInterfaceLayoutDirection.h>
#include <memory>
#include <wtf/CompletionHandler.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/MonotonicTime.h>
#include <wtf/Optional.h>
#include <wtf/ProcessID.h>
#include <wtf/Ref.h>
#include <wtf/RefPtr.h>
#include <wtf/RunLoop.h>
#include <wtf/URL.h>
#include <wtf/UniqueRef.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(IOS_FAMILY)
#include "WebAutocorrectionContext.h"
#endif

OBJC_CLASS NSView;
OBJC_CLASS _WKRemoteObjectRegistry;

#if ENABLE(DRAG_SUPPORT)
#include <WebCore/DragActions.h>
#endif

#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#endif

#if PLATFORM(COCOA)
#include "DynamicViewportSizeUpdate.h"
#include "RemoteLayerTreeNode.h"
#include "TouchBarMenuData.h"
#include "TouchBarMenuItemData.h"
#endif

#if PLATFORM(GTK)
#include "ArgumentCodersGtk.h"
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
#include <WebCore/MediaPlaybackTargetPicker.h>
#include <WebCore/WebMediaSessionManagerClient.h>
#endif

#if ENABLE(MEDIA_SESSION)
namespace WebCore {
class MediaSessionMetadata;
}
#endif

namespace API {
class Attachment;
class ContextMenuClient;
class FindClient;
class FindMatchesClient;
class FormClient;
class FullscreenClient;
class HistoryClient;
class IconLoadingClient;
class LoaderClient;
class Navigation;
class NavigationClient;
class PolicyClient;
class UIClient;
class URLRequest;
}

namespace IPC {
class Decoder;
class Connection;
}

namespace WebCore {
class AuthenticationChallenge;
class CertificateInfo;
class Cursor;
class DragData;
class FloatRect;
class FontAttributeChanges;
class FontChanges;
class GraphicsLayer;
class IntSize;
class ProtectionSpace;
class RunLoopObserver;
class SharedBuffer;
class TextIndicator;
class ValidationBubble;

enum SelectionDirection : uint8_t;

enum class AutoplayEvent : uint8_t;
enum class LockBackForwardList : bool;
enum class HasInsecureContent : bool;
enum class NotificationDirection : uint8_t;
enum class ShouldSample : bool;
enum class ShouldTreatAsContinuingLoad : bool;
enum class WritingDirection : uint8_t;

struct ApplicationManifest;
struct BackForwardItemIdentifier;
struct DataListSuggestionInformation;
struct DictionaryPopupInfo;
struct ExceptionDetails;
struct FileChooserSettings;
struct GlobalWindowIdentifier;
struct MediaStreamRequest;
struct PrewarmInformation;
struct SecurityOriginData;
struct ShareData;
struct TextAlternativeWithRange;
struct TextCheckingResult;
struct ViewportAttributes;
struct WindowFeatures;

template <typename> class RectEdges;
using FloatBoxExtent = RectEdges<float>;
}

#if PLATFORM(GTK)
typedef GtkWidget* PlatformWidget;
#endif

#if PLATFORM(WPE)
struct wpe_view_backend;
#endif

#if PLATFORM(GTK) || PLATFORM(WPE)
typedef struct OpaqueJSContext* JSGlobalContextRef;
#endif

#if PLATFORM(WIN)
typedef HWND PlatformWidget;
#endif

namespace WebKit {
class DrawingAreaProxy;
class EditableImageController;
class NativeWebGestureEvent;
class NativeWebKeyboardEvent;
class NativeWebMouseEvent;
class NativeWebWheelEvent;
class PageClient;
class ProvisionalPageProxy;
class RemoteLayerTreeHost;
class RemoteLayerTreeScrollingPerformanceData;
class RemoteLayerTreeTransaction;
class RemoteScrollingCoordinatorProxy;
class SecKeyProxyStore;
class SharedBufferDataReference;
class UserData;
class ViewSnapshot;
class VisitedLinkStore;
class WebBackForwardList;
class WebBackForwardListItem;
class WebContextMenuProxy;
class WebEditCommandProxy;
class WebFullScreenManagerProxy;
class PlaybackSessionManagerProxy;
class WebNavigationState;
class VideoFullscreenManagerProxy;
class WebAuthenticatorCoordinatorProxy;
class WebKeyboardEvent;
class WebURLSchemeHandler;
class WebMouseEvent;
class WebOpenPanelResultListenerProxy;
class WebPageDebuggable;
class WebPageGroup;
class WebPageInspectorController;
class WebProcessProxy;
class WebUserContentControllerProxy;
class WebWheelEvent;
class WebsiteDataStore;
class GamepadData;

struct AttributedString;
struct ColorSpaceData;
struct EditingRange;
struct EditorState;
struct FrameInfoData;
struct InteractionInformationRequest;
struct LoadParameters;
struct PlatformPopupMenuData;
struct PrintInfo;
struct WebPopupItem;
struct URLSchemeTaskParameters;

enum class ProcessSwapRequestedByClient;
enum class UndoOrRedo : bool;

#if USE(QUICK_LOOK)
class QuickLookDocumentData;
#endif

typedef GenericCallback<uint64_t> UnsignedCallback;
typedef GenericCallback<EditingRange> EditingRangeCallback;
typedef GenericCallback<const String&> StringCallback;
typedef GenericCallback<API::SerializedScriptValue*, bool, const WebCore::ExceptionDetails&> ScriptValueCallback;
typedef GenericCallback<const WebCore::FontAttributes&> FontAttributesCallback;

#if PLATFORM(GTK)
typedef GenericCallback<API::Error*> PrintFinishedCallback;
#endif

#if ENABLE(TOUCH_EVENTS)
struct QueuedTouchEvents {
    QueuedTouchEvents(const NativeWebTouchEvent& event)
        : forwardedEvent(event)
    {
    }
    NativeWebTouchEvent forwardedEvent;
    Vector<NativeWebTouchEvent> deferredTouchEvents;
};
#endif

typedef GenericCallback<const String&, bool, int32_t> ValidateCommandCallback;
typedef GenericCallback<const WebCore::IntRect&, const EditingRange&> RectForCharacterRangeCallback;

#if ENABLE(APPLICATION_MANIFEST)
typedef GenericCallback<const Optional<WebCore::ApplicationManifest>&> ApplicationManifestCallback;
#endif

#if PLATFORM(MAC)
typedef GenericCallback<const AttributedString&, const EditingRange&> AttributedStringForCharacterRangeCallback;
typedef GenericCallback<const String&, double, bool> FontAtSelectionCallback;
#endif

#if PLATFORM(IOS_FAMILY)
typedef GenericCallback<const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t> GestureCallback;
typedef GenericCallback<const WebCore::IntPoint&, uint32_t, uint32_t> TouchesCallback;
typedef GenericCallback<const Vector<WebCore::SelectionRect>&> SelectionRectsCallback;
typedef GenericCallback<const FocusedElementInformation&> FocusedElementInformationCallback;
struct ElementDidFocusArguments {
    WTF_MAKE_FAST_ALLOCATED;
public:
    FocusedElementInformation information;
    bool userIsInteracting;
    bool blurPreviousNode;
    bool changingActivityState;
    RefPtr<API::Object> userData;
};

using DrawToPDFCallback = GenericCallback<const IPC::DataReference&>;
#endif

#if PLATFORM(COCOA)
typedef GenericCallback<const WTF::MachSendRight&> MachSendRightCallback;
typedef GenericCallback<bool, bool, String, double, double, uint64_t> NowPlayingInfoCallback;
#endif

using SpellDocumentTag = int64_t;

class WebPageProxy : public API::ObjectImpl<API::Object::Type::Page>
#if ENABLE(INPUT_TYPE_COLOR)
    , public WebColorPicker::Client
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , public WebCore::WebMediaSessionManagerClient
#endif
    , public WebPopupMenuProxy::Client
    , public IPC::MessageReceiver
    , public IPC::MessageSender
    , public CanMakeWeakPtr<WebPageProxy> {
public:
    static Ref<WebPageProxy> create(PageClient&, WebProcessProxy&, uint64_t pageID, Ref<API::PageConfiguration>&&);
    virtual ~WebPageProxy();

    const API::PageConfiguration& configuration() const;

    uint64_t pageID() const { return m_pageID; }

    PAL::SessionID sessionID() const;

    WebFrameProxy* mainFrame() const { return m_mainFrame.get(); }
    WebFrameProxy* focusedFrame() const { return m_focusedFrame.get(); }
    WebFrameProxy* frameSetLargestFrame() const { return m_frameSetLargestFrame.get(); }

    DrawingAreaProxy* drawingArea() const { return m_drawingArea.get(); }
    DrawingAreaProxy* provisionalDrawingArea() const;

    WebNavigationState& navigationState() { return *m_navigationState.get(); }

    WebsiteDataStore& websiteDataStore() { return m_websiteDataStore; }
    void changeWebsiteDataStore(WebsiteDataStore&);

    void addPreviouslyVisitedPath(const String&);

#if ENABLE(DATA_DETECTION)
    NSArray *dataDetectionResults() { return m_dataDetectionResults.get(); }
    void detectDataInAllFrames(WebCore::DataDetectorTypes, CompletionHandler<void(const DataDetectionResult&)>&&);
    void removeDataDetectedLinks(CompletionHandler<void(const DataDetectionResult&)>&&);
#endif
        
#if ENABLE(ASYNC_SCROLLING) && PLATFORM(COCOA)
    RemoteScrollingCoordinatorProxy* scrollingCoordinatorProxy() const { return m_scrollingCoordinatorProxy.get(); }
#endif

    WebBackForwardList& backForwardList() { return m_backForwardList; }

    bool addsVisitedLinks() const { return m_addsVisitedLinks; }
    void setAddsVisitedLinks(bool addsVisitedLinks) { m_addsVisitedLinks = addsVisitedLinks; }
    VisitedLinkStore& visitedLinkStore() { return m_visitedLinkStore; }

    void exitFullscreenImmediately();
    void fullscreenMayReturnToInline();
    void didEnterFullscreen();
    void didExitFullscreen();

    WebInspectorProxy* inspector() const;

    void didChangeInspectorFrontendCount(unsigned count) { m_inspectorFrontendCount = count; }
    unsigned inspectorFrontendCount() const { return m_inspectorFrontendCount; }
    bool hasInspectorFrontend() const { return m_inspectorFrontendCount > 0; }

    bool isControlledByAutomation() const { return m_controlledByAutomation; }
    void setControlledByAutomation(bool);

    WebPageInspectorController& inspectorController() { return *m_inspectorController; }

#if PLATFORM(IOS_FAMILY)
    void showInspectorIndication();
    void hideInspectorIndication();
#endif

    void createInspectorTarget(const String& targetId, Inspector::InspectorTargetType);
    void destroyInspectorTarget(const String& targetId);
    void sendMessageToInspectorFrontend(const String& targetId, const String& message);

#if ENABLE(REMOTE_INSPECTOR)
    void setIndicating(bool);
    bool allowsRemoteInspection() const;
    void setAllowsRemoteInspection(bool);
    String remoteInspectionNameOverride() const;
    void setRemoteInspectionNameOverride(const String&);
    void remoteInspectorInformationDidChange();
#endif

#if ENABLE(FULLSCREEN_API)
    WebFullScreenManagerProxy* fullScreenManager();

    API::FullscreenClient& fullscreenClient() const { return *m_fullscreenClient; }
    void setFullscreenClient(std::unique_ptr<API::FullscreenClient>&&);
#endif
#if (PLATFORM(IOS_FAMILY) && HAVE(AVKIT)) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))
    PlaybackSessionManagerProxy* playbackSessionManager();
    VideoFullscreenManagerProxy* videoFullscreenManager();
#endif

#if PLATFORM(IOS_FAMILY)
    bool allowsMediaDocumentInlinePlayback() const;
    void setAllowsMediaDocumentInlinePlayback(bool);
#endif

#if USE(SYSTEM_PREVIEW)
    SystemPreviewController* systemPreviewController() { return m_systemPreviewController.get(); }
#endif

#if ENABLE(CONTEXT_MENUS)
    API::ContextMenuClient& contextMenuClient() { return *m_contextMenuClient; }
    void setContextMenuClient(std::unique_ptr<API::ContextMenuClient>&&);
#endif
    API::FindClient& findClient() { return *m_findClient; }
    void setFindClient(std::unique_ptr<API::FindClient>&&);
    API::FindMatchesClient& findMatchesClient() { return *m_findMatchesClient; }
    void setFindMatchesClient(std::unique_ptr<API::FindMatchesClient>&&);
    API::DiagnosticLoggingClient* diagnosticLoggingClient() { return m_diagnosticLoggingClient.get(); }
    void setDiagnosticLoggingClient(std::unique_ptr<API::DiagnosticLoggingClient>&&);
    void setFormClient(std::unique_ptr<API::FormClient>&&);
    void setNavigationClient(UniqueRef<API::NavigationClient>&&);
    void setHistoryClient(UniqueRef<API::HistoryClient>&&);
    void setLoaderClient(std::unique_ptr<API::LoaderClient>&&);
    void setPolicyClient(std::unique_ptr<API::PolicyClient>&&);
    void setInjectedBundleClient(const WKPageInjectedBundleClientBase*);
    WebPageInjectedBundleClient* injectedBundleClient() { return m_injectedBundleClient.get(); }

    API::UIClient& uiClient() { return *m_uiClient; }
    void setUIClient(std::unique_ptr<API::UIClient>&&);

    API::IconLoadingClient& iconLoadingClient() { return *m_iconLoadingClient; }
    void setIconLoadingClient(std::unique_ptr<API::IconLoadingClient>&&);

    void initializeWebPage();
    void setDrawingArea(std::unique_ptr<DrawingAreaProxy>&&);

    WeakPtr<SecKeyProxyStore> secKeyProxyStore(const WebCore::AuthenticationChallenge&);
        
    void close();
    bool tryClose();
    bool isClosed() const { return m_isClosed; }

    void setOpenedByDOM() { m_openedByDOM = true; }
    bool openedByDOM() const { return m_openedByDOM; }

    bool hasCommittedAnyProvisionalLoads() const { return m_hasCommittedAnyProvisionalLoads; }

    void setIsUsingHighPerformanceWebGL(bool value) { m_isUsingHighPerformanceWebGL = value; }
    bool isUsingHighPerformanceWebGL() const { return m_isUsingHighPerformanceWebGL; }

    void didExceedInactiveMemoryLimitWhileActive();
    void didExceedBackgroundCPULimitWhileInForeground();

    void closePage(bool stopResponsivenessTimer);

    void addPlatformLoadParameters(LoadParameters&);
    RefPtr<API::Navigation> loadRequest(WebCore::ResourceRequest&&, WebCore::ShouldOpenExternalURLsPolicy = WebCore::ShouldOpenExternalURLsPolicy::ShouldAllowExternalSchemes, API::Object* userData = nullptr);
    RefPtr<API::Navigation> loadFile(const String& fileURL, const String& resourceDirectoryURL, API::Object* userData = nullptr);
    RefPtr<API::Navigation> loadData(const IPC::DataReference&, const String& MIMEType, const String& encoding, const String& baseURL, API::Object* userData = nullptr);
    void loadAlternateHTML(const IPC::DataReference&, const String& encoding, const URL& baseURL, const URL& unreachableURL, API::Object* userData = nullptr);
    void loadWebArchiveData(API::Data*, API::Object* userData = nullptr);
    void navigateToPDFLinkWithSimulatedClick(const String& url, WebCore::IntPoint documentPoint, WebCore::IntPoint screenPoint);

    void simulateDeviceOrientationChange(double alpha, double beta, double gamma);

    void stopLoading();
    RefPtr<API::Navigation> reload(OptionSet<WebCore::ReloadOption>);

    RefPtr<API::Navigation> goForward();
    RefPtr<API::Navigation> goBack();

    RefPtr<API::Navigation> goToBackForwardItem(WebBackForwardListItem&);
    void tryRestoreScrollPosition();
    void didChangeBackForwardList(WebBackForwardListItem* addedItem, Vector<Ref<WebBackForwardListItem>>&& removed);
    void willGoToBackForwardListItem(const WebCore::BackForwardItemIdentifier&, bool inPageCache);

    bool shouldKeepCurrentBackForwardListItemInList(WebBackForwardListItem&);

    bool willHandleHorizontalScrollEvents() const;

    void updateWebsitePolicies(WebsitePoliciesData&&);

    bool canShowMIMEType(const String& mimeType);

    String currentURL() const;

    float topContentInset() const { return m_topContentInset; }
    void setTopContentInset(float);

    WebCore::Color underlayColor() const { return m_underlayColor; }
    void setUnderlayColor(const WebCore::Color&);

    // At this time, m_pageExtendedBackgroundColor can be set via pageExtendedBackgroundColorDidChange() which is a message
    // from the UIProcess, or by didCommitLayerTree(). When PLATFORM(MAC) adopts UI side compositing, we should get rid of
    // the message entirely.
    WebCore::Color pageExtendedBackgroundColor() const { return m_pageExtendedBackgroundColor; }

    void viewWillStartLiveResize();
    void viewWillEndLiveResize();

    void setInitialFocus(bool forward, bool isKeyboardEventValid, const WebKeyboardEvent&, WTF::Function<void (CallbackBase::Error)>&&);
    
    void clearSelection();
    void restoreSelectionInFocusedEditableElement();

    PageClient& pageClient() const;

    void setViewNeedsDisplay(const WebCore::Region&);
    void requestScroll(const WebCore::FloatPoint& scrollPosition, const WebCore::IntPoint& scrollOrigin, bool isProgrammaticScroll);
    
    WebCore::FloatPoint viewScrollPosition() const;

    void setDelegatesScrolling(bool delegatesScrolling) { m_delegatesScrolling = delegatesScrolling; }
    bool delegatesScrolling() const { return m_delegatesScrolling; }

    enum class ActivityStateChangeDispatchMode { Deferrable, Immediate };
    void activityStateDidChange(OptionSet<WebCore::ActivityState::Flag> mayHaveChanged, bool wantsSynchronousReply = false, ActivityStateChangeDispatchMode = ActivityStateChangeDispatchMode::Deferrable);
    bool isInWindow() const { return m_activityState.contains(WebCore::ActivityState::IsInWindow); }
    void waitForDidUpdateActivityState(ActivityStateChangeID);
    void didUpdateActivityState() { m_waitingForDidUpdateActivityState = false; }

    void layerHostingModeDidChange();

    WebCore::IntSize viewSize() const;
    bool isViewVisible() const { return m_activityState.contains(WebCore::ActivityState::IsVisible); }
    bool isViewFocused() const { return m_activityState.contains(WebCore::ActivityState::IsFocused); }
    bool isViewWindowActive() const { return m_activityState.contains(WebCore::ActivityState::WindowIsActive); }

    void addMIMETypeWithCustomContentProvider(const String& mimeType);

    void executeEditCommand(const String& commandName, const String& argument = String());
    void validateCommand(const String& commandName, WTF::Function<void (const String&, bool, int32_t, CallbackBase::Error)>&&);

    const EditorState& editorState() const { return m_editorState; }
    bool canDelete() const { return hasSelectedRange() && isContentEditable(); }
    bool hasSelectedRange() const { return m_editorState.selectionIsRange; }
    bool isContentEditable() const { return m_editorState.isContentEditable; }

    void increaseListLevel();
    void decreaseListLevel();
    void changeListType();

    void setBaseWritingDirection(WebCore::WritingDirection);

    Optional<WebCore::FontAttributes> cachedFontAttributesAtSelectionStart() const { return m_cachedFontAttributesAtSelectionStart; }

#if PLATFORM(COCOA)
    const TouchBarMenuData& touchBarMenuData() const { return m_touchBarMenuData; }
#endif

    bool maintainsInactiveSelection() const;
    void setMaintainsInactiveSelection(bool);
    void setEditable(bool);
    bool isEditable() const { return m_isEditable; }

    void activateMediaStreamCaptureInPage();
    bool isMediaStreamCaptureMuted() const { return m_mutedState & WebCore::MediaProducer::CaptureDevicesAreMuted; }
    void setMediaStreamCaptureMuted(bool);
    void executeEditCommand(const String& commandName, const String& argument, WTF::Function<void(CallbackBase::Error)>&&);
        
    void requestFontAttributesAtSelectionStart(Function<void(const WebCore::FontAttributes&, CallbackBase::Error)>&&);
    void fontAttributesCallback(const WebCore::FontAttributes&, CallbackID);

#if PLATFORM(IOS_FAMILY)
    double displayedContentScale() const { return m_lastVisibleContentRectUpdate.scale(); }
    const WebCore::FloatRect& exposedContentRect() const { return m_lastVisibleContentRectUpdate.exposedContentRect(); }
    const WebCore::FloatRect& unobscuredContentRect() const { return m_lastVisibleContentRectUpdate.unobscuredContentRect(); }
    bool inStableState() const { return m_lastVisibleContentRectUpdate.inStableState(); }
    const WebCore::FloatRect& unobscuredContentRectRespectingInputViewBounds() const { return m_lastVisibleContentRectUpdate.unobscuredContentRectRespectingInputViewBounds(); }
    // When visual viewports are enabled, this is the layout viewport rect.
    const WebCore::FloatRect& customFixedPositionRect() const { return m_lastVisibleContentRectUpdate.customFixedPositionRect(); }

    void updateVisibleContentRects(const VisibleContentRectUpdateInfo&);
    void resendLastVisibleContentRects();

    WebCore::FloatRect computeCustomFixedPositionRect(const WebCore::FloatRect& unobscuredContentRect, const WebCore::FloatRect& unobscuredContentRectRespectingInputViewBounds, const WebCore::FloatRect& currentCustomFixedPositionRect, double displayedContentScale, WebCore::FrameView::LayoutViewportConstraint = WebCore::FrameView::LayoutViewportConstraint::Unconstrained, bool visualViewportEnabled = false) const;

    void scrollingNodeScrollViewWillStartPanGesture();
    void scrollingNodeScrollViewDidScroll();
    void scrollingNodeScrollWillStartScroll();
    void scrollingNodeScrollDidEndScroll();

    void dynamicViewportSizeUpdate(const WebCore::FloatSize& viewLayoutSize, const WebCore::FloatSize& maximumUnobscuredSize, const WebCore::FloatRect& targetExposedContentRect, const WebCore::FloatRect& targetUnobscuredRect, const WebCore::FloatRect& targetUnobscuredRectInScrollViewCoordinates, const WebCore::FloatBoxExtent& unobscuredSafeAreaInsets, double targetScale, int32_t deviceOrientation, DynamicViewportSizeUpdateID);

    void setViewportConfigurationViewLayoutSize(const WebCore::FloatSize&, double scaleFactor, double minimumEffectiveDeviceWidth);
    void setMaximumUnobscuredSize(const WebCore::FloatSize&);
    void setDeviceOrientation(int32_t);
    int32_t deviceOrientation() const { return m_deviceOrientation; }
    void setOverrideViewportArguments(const Optional<WebCore::ViewportArguments>&);
    void willCommitLayerTree(uint64_t transactionID);

    void selectWithGesture(const WebCore::IntPoint, WebCore::TextGranularity, uint32_t gestureType, uint32_t gestureState, bool isInteractingWithFocusedElement, WTF::Function<void(const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t, CallbackBase::Error)>&&);
    void updateSelectionWithTouches(const WebCore::IntPoint, uint32_t touches, bool baseIsStart, WTF::Function<void (const WebCore::IntPoint&, uint32_t, uint32_t, CallbackBase::Error)>&&);
    void selectWithTwoTouches(const WebCore::IntPoint from, const WebCore::IntPoint to, uint32_t gestureType, uint32_t gestureState, WTF::Function<void (const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t, CallbackBase::Error)>&&);
    void extendSelection(WebCore::TextGranularity);
    void selectWordBackward();
    void moveSelectionByOffset(int32_t offset, WTF::Function<void (CallbackBase::Error)>&&);
    void selectTextWithGranularityAtPoint(const WebCore::IntPoint, WebCore::TextGranularity, bool isInteractingWithFocusedElement, WTF::Function<void(CallbackBase::Error)>&&);
    void selectPositionAtPoint(const WebCore::IntPoint, bool isInteractingWithFocusedElement, WTF::Function<void(CallbackBase::Error)>&&);
    void selectPositionAtBoundaryWithDirection(const WebCore::IntPoint, WebCore::TextGranularity, WebCore::SelectionDirection, bool isInteractingWithFocusedElement, WTF::Function<void(CallbackBase::Error)>&&);
    void moveSelectionAtBoundaryWithDirection(WebCore::TextGranularity, WebCore::SelectionDirection, WTF::Function<void(CallbackBase::Error)>&&);
    void beginSelectionInDirection(WebCore::SelectionDirection, WTF::Function<void (uint64_t, CallbackBase::Error)>&&);
    void updateSelectionWithExtentPoint(const WebCore::IntPoint, bool isInteractingWithFocusedElement, WTF::Function<void(uint64_t, CallbackBase::Error)>&&);
    void updateSelectionWithExtentPointAndBoundary(const WebCore::IntPoint, WebCore::TextGranularity, bool isInteractingWithFocusedElement, WTF::Function<void(uint64_t, CallbackBase::Error)>&&);
    void requestAutocorrectionData(const String& textForAutocorrection, WTF::Function<void (const Vector<WebCore::FloatRect>&, const String&, double, uint64_t, CallbackBase::Error)>&&);
    void applyAutocorrection(const String& correction, const String& originalText, WTF::Function<void (const String&, CallbackBase::Error)>&&);
    bool applyAutocorrection(const String& correction, const String& originalText);
    void requestAutocorrectionContext(Function<void(const WebAutocorrectionContext&, CallbackBase::Error)>&&);
    WebAutocorrectionContext autocorrectionContextSync();
    void requestDictationContext(WTF::Function<void (const String&, const String&, const String&, CallbackBase::Error)>&&);
    void replaceDictatedText(const String& oldText, const String& newText);
    void replaceSelectedText(const String& oldText, const String& newText);
    void didReceivePositionInformation(const InteractionInformationAtPosition&);
    void requestPositionInformation(const InteractionInformationRequest&);
    void startInteractionWithElementAtPosition(const WebCore::IntPoint&);
    void stopInteraction();
    void performActionOnElement(uint32_t action);
    void saveImageToLibrary(const SharedMemory::Handle& imageHandle, uint64_t imageSize);
    void focusNextFocusedElement(bool isForward, WTF::Function<void (CallbackBase::Error)>&& = [] (auto) { });
    void setFocusedElementValue(const String&);
    void setFocusedElementValueAsNumber(double);
    void setFocusedElementSelectedIndex(uint32_t index, bool allowMultipleSelection = false);
    void applicationDidEnterBackground();
    void applicationDidFinishSnapshottingAfterEnteringBackground();
    void applicationWillEnterForeground();
    void applicationWillResignActive();
    void applicationDidBecomeActive();
    void commitPotentialTapFailed();
    void didNotHandleTapAsClick(const WebCore::IntPoint&);
    void didCompleteSyntheticClick();
    void disableDoubleTapGesturesDuringTapIfNecessary(uint64_t requestID);
    void contentSizeCategoryDidChange(const String& contentSizeCategory);
    void getSelectionContext(WTF::Function<void(const String&, const String&, const String&, CallbackBase::Error)>&&);
    void handleTwoFingerTapAtPoint(const WebCore::IntPoint&, OptionSet<WebKit::WebEvent::Modifier>, uint64_t requestID);
    void handleStylusSingleTapAtPoint(const WebCore::IntPoint&, uint64_t requestID);
    void setForceAlwaysUserScalable(bool);
    bool forceAlwaysUserScalable() const { return m_forceAlwaysUserScalable; }
    double layoutSizeScaleFactor() const { return m_viewportConfigurationLayoutSizeScaleFactor; }
    void setIsScrollingOrZooming(bool);
    void requestRectsForGranularityWithSelectionOffset(WebCore::TextGranularity, uint32_t offset, WTF::Function<void(const Vector<WebCore::SelectionRect>&, CallbackBase::Error)>&&);
    void requestRectsAtSelectionOffsetWithText(int32_t offset, const String&, WTF::Function<void(const Vector<WebCore::SelectionRect>&, CallbackBase::Error)>&&);
    void autofillLoginCredentials(const String& username, const String& password);
    void storeSelectionForAccessibility(bool);
    void startAutoscrollAtPosition(const WebCore::FloatPoint& positionInWindow);
    void cancelAutoscroll();
    void hardwareKeyboardAvailabilityChanged();
    bool isScrollingOrZooming() const { return m_isScrollingOrZooming; }
#if ENABLE(DATA_INTERACTION)
    void didHandleDragStartRequest(bool started);
    void didHandleAdditionalDragItemsRequest(bool added);
    void requestDragStart(const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition);
    void requestAdditionalItemsForDragSession(const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition);
    void didConcludeEditDrag(Optional<WebCore::TextIndicatorData>);
#endif
#endif
#if ENABLE(DATA_DETECTION)
    void setDataDetectionResult(const DataDetectionResult&);
#endif
    void didCommitLayerTree(const WebKit::RemoteLayerTreeTransaction&);
    void layerTreeCommitComplete();

    bool updateLayoutViewportParameters(const WebKit::RemoteLayerTreeTransaction&);

#if PLATFORM(GTK)
    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    void confirmComposition(const String& compositionString, int64_t selectionStart, int64_t selectionLength);
    void cancelComposition();
#endif

#if PLATFORM(GTK)
    void setInputMethodState(bool enabled);
#endif

#if PLATFORM (GTK) && HAVE(GTK_GESTURES)
    void getCenterForZoomGesture(const WebCore::IntPoint& centerInViewCoordinates, WebCore::IntPoint& center);
#endif

#if PLATFORM(COCOA)
    void windowAndViewFramesChanged(const WebCore::FloatRect& viewFrameInWindowCoordinates, const WebCore::FloatPoint& accessibilityViewCoordinates);
    void setMainFrameIsScrollable(bool);

    void sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput);
    bool shouldDelayWindowOrderingForEvent(const WebMouseEvent&);
    bool acceptsFirstMouse(int eventNumber, const WebMouseEvent&);

    void setRemoteLayerTreeRootNode(RemoteLayerTreeNode*);
    CALayer *acceleratedCompositingRootLayer() const;

    void setTextAsync(const String&);
    void insertTextAsync(const String& text, const EditingRange& replacementRange, bool registerUndoGroup = false, EditingRangeIsRelativeTo = EditingRangeIsRelativeTo::EditableRoot, bool suppressSelectionUpdate = false);
    void getMarkedRangeAsync(WTF::Function<void (EditingRange, CallbackBase::Error)>&&);
    void getSelectedRangeAsync(WTF::Function<void (EditingRange, CallbackBase::Error)>&&);
    void characterIndexForPointAsync(const WebCore::IntPoint&, WTF::Function<void (uint64_t, CallbackBase::Error)>&&);
    void firstRectForCharacterRangeAsync(const EditingRange&, WTF::Function<void (const WebCore::IntRect&, const EditingRange&, CallbackBase::Error)>&&);
    void setCompositionAsync(const String& text, const Vector<WebCore::CompositionUnderline>& underlines, const EditingRange& selectionRange, const EditingRange& replacementRange);
    void confirmCompositionAsync();

    void setScrollPerformanceDataCollectionEnabled(bool);
    bool scrollPerformanceDataCollectionEnabled() const { return m_scrollPerformanceDataCollectionEnabled; }
    RemoteLayerTreeScrollingPerformanceData* scrollingPerformanceData() { return m_scrollingPerformanceData.get(); }
#endif // PLATFORM(COCOA)

    void changeFontAttributes(WebCore::FontAttributeChanges&&);
    void changeFont(WebCore::FontChanges&&);

#if PLATFORM(MAC)
    void insertDictatedTextAsync(const String& text, const EditingRange& replacementRange, const Vector<WebCore::TextAlternativeWithRange>& dictationAlternatives, bool registerUndoGroup);
    void attributedSubstringForCharacterRangeAsync(const EditingRange&, WTF::Function<void (const AttributedString&, const EditingRange&, CallbackBase::Error)>&&);
    void fontAtSelection(WTF::Function<void (const String&, double, bool, CallbackBase::Error)>&&);

    void startWindowDrag();
    NSWindow *platformWindow();
    void rootViewToWindow(const WebCore::IntRect& viewRect, WebCore::IntRect& windowRect);

#if WK_API_ENABLED
    NSView *inspectorAttachmentView();
    _WKRemoteObjectRegistry *remoteObjectRegistry();
#endif

    void intrinsicContentSizeDidChange(const WebCore::IntSize& intrinsicContentSize);
    CGRect boundsOfLayerInLayerBackedWindowCoordinates(CALayer *) const;
#endif // PLATFORM(MAC)

#if PLATFORM(GTK)
    PlatformWidget viewWidget();
    bool makeGLContextCurrent();
#endif

    const Optional<WebCore::Color>& backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const Optional<WebCore::Color>&);

#if PLATFORM(WIN)
    PlatformWidget viewWidget();
#endif
#if PLATFORM(WPE)
    struct wpe_view_backend* viewBackend();
#endif

    bool isProcessingMouseEvents() const;
    void processNextQueuedMouseEvent();
    void handleMouseEvent(const NativeWebMouseEvent&);

    void handleWheelEvent(const NativeWebWheelEvent&);

    bool isProcessingKeyboardEvents() const;
    void handleKeyboardEvent(const NativeWebKeyboardEvent&);

#if ENABLE(MAC_GESTURE_EVENTS)
    void handleGestureEvent(const NativeWebGestureEvent&);
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
    void handleTouchEventSynchronously(NativeWebTouchEvent&);
    void handleTouchEventAsynchronously(const NativeWebTouchEvent&);

#elif ENABLE(TOUCH_EVENTS)
    void handleTouchEvent(const NativeWebTouchEvent&);
#endif

#if ENABLE(POINTER_EVENTS)
    void cancelPointer(WebCore::PointerID, const WebCore::IntPoint&);
#endif

    void scrollBy(WebCore::ScrollDirection, WebCore::ScrollGranularity);
    void centerSelectionInVisibleArea();

    const String& toolTip() const { return m_toolTip; }

    const String& userAgent() const { return m_userAgent; }
    void setApplicationNameForUserAgent(const String&);
    const String& applicationNameForUserAgent() const { return m_applicationNameForUserAgent; }
    void setCustomUserAgent(const String&);
    const String& customUserAgent() const { return m_customUserAgent; }
    static String standardUserAgent(const String& applicationName = String());

    bool supportsTextEncoding() const;
    void setCustomTextEncodingName(const String&);
    String customTextEncodingName() const { return m_customTextEncodingName; }

    bool areActiveDOMObjectsAndAnimationsSuspended() const { return m_isPageSuspended; }
    void resumeActiveDOMObjectsAndAnimations();
    void suspendActiveDOMObjectsAndAnimations();

    double estimatedProgress() const;

    SessionState sessionState(WTF::Function<bool (WebBackForwardListItem&)>&& = nullptr) const;
    RefPtr<API::Navigation> restoreFromSessionState(SessionState, bool navigate);

    bool supportsTextZoom() const;
    double textZoomFactor() const { return m_textZoomFactor; }
    void setTextZoomFactor(double);
    double pageZoomFactor() const;
    void setPageZoomFactor(double);
    void setPageAndTextZoomFactors(double pageZoomFactor, double textZoomFactor);

    void scalePage(double scale, const WebCore::IntPoint& origin);
    void scalePageInViewCoordinates(double scale, const WebCore::IntPoint& centerInViewCoordinates);
    double pageScaleFactor() const;
    double viewScaleFactor() const { return m_viewScaleFactor; }
    void scaleView(double scale);
    void setShouldScaleViewToFitDocument(bool);
    
    float deviceScaleFactor() const;
    void setIntrinsicDeviceScaleFactor(float);
    void setCustomDeviceScaleFactor(float);
    void windowScreenDidChange(WebCore::PlatformDisplayID);
    void accessibilitySettingsDidChange();
#if ENABLE(ACCESSIBILITY_EVENTS)
    void updateAccessibilityEventsEnabled(bool);
#endif

    void setUseFixedLayout(bool);
    void setFixedLayoutSize(const WebCore::IntSize&);
    bool useFixedLayout() const { return m_useFixedLayout; };
    const WebCore::IntSize& fixedLayoutSize() const { return m_fixedLayoutSize; };

    void setAlwaysShowsHorizontalScroller(bool);
    void setAlwaysShowsVerticalScroller(bool);
    bool alwaysShowsHorizontalScroller() const { return m_alwaysShowsHorizontalScroller; }
    bool alwaysShowsVerticalScroller() const { return m_alwaysShowsVerticalScroller; }

    void listenForLayoutMilestones(OptionSet<WebCore::LayoutMilestone>);

    bool hasHorizontalScrollbar() const { return m_mainFrameHasHorizontalScrollbar; }
    bool hasVerticalScrollbar() const { return m_mainFrameHasVerticalScrollbar; }

    void setSuppressScrollbarAnimations(bool);
    bool areScrollbarAnimationsSuppressed() const { return m_suppressScrollbarAnimations; }

    bool isPinnedToLeftSide() const { return m_mainFrameIsPinnedToLeftSide; }
    bool isPinnedToRightSide() const { return m_mainFrameIsPinnedToRightSide; }
    bool isPinnedToTopSide() const { return m_mainFrameIsPinnedToTopSide; }
    bool isPinnedToBottomSide() const { return m_mainFrameIsPinnedToBottomSide; }

    bool rubberBandsAtLeft() const;
    void setRubberBandsAtLeft(bool);
    bool rubberBandsAtRight() const;
    void setRubberBandsAtRight(bool);
    bool rubberBandsAtTop() const;
    void setRubberBandsAtTop(bool);
    bool rubberBandsAtBottom() const;
    void setRubberBandsAtBottom(bool);

    void setShouldUseImplicitRubberBandControl(bool shouldUseImplicitRubberBandControl) { m_shouldUseImplicitRubberBandControl = shouldUseImplicitRubberBandControl; }
    bool shouldUseImplicitRubberBandControl() const { return m_shouldUseImplicitRubberBandControl; }
        
    void setEnableVerticalRubberBanding(bool);
    bool verticalRubberBandingIsEnabled() const;
    void setEnableHorizontalRubberBanding(bool);
    bool horizontalRubberBandingIsEnabled() const;
        
    void setBackgroundExtendsBeyondPage(bool);
    bool backgroundExtendsBeyondPage() const;

    void setPaginationMode(WebCore::Pagination::Mode);
    WebCore::Pagination::Mode paginationMode() const { return m_paginationMode; }
    void setPaginationBehavesLikeColumns(bool);
    bool paginationBehavesLikeColumns() const { return m_paginationBehavesLikeColumns; }
    void setPageLength(double);
    double pageLength() const { return m_pageLength; }
    void setGapBetweenPages(double);
    double gapBetweenPages() const { return m_gapBetweenPages; }
    void setPaginationLineGridEnabled(bool);
    bool paginationLineGridEnabled() const { return m_paginationLineGridEnabled; }
    unsigned pageCount() const { return m_pageCount; }

    void isJITEnabled(CompletionHandler<void(bool)>&&);

#if PLATFORM(MAC)
    void setUseSystemAppearance(bool);
    bool useSystemAppearance() const { return m_useSystemAppearance; }
#endif

    void effectiveAppearanceDidChange();
    bool useDarkAppearance() const;

#if PLATFORM(COCOA)
    // Called by the web process through a message.
    void registerWebProcessAccessibilityToken(const IPC::DataReference&);
    // Called by the UI process when it is ready to send its tokens to the web process.
    void registerUIProcessAccessibilityTokens(const IPC::DataReference& elemenToken, const IPC::DataReference& windowToken);
    bool readSelectionFromPasteboard(const String& pasteboardName);
    String stringSelectionForPasteboard();
    RefPtr<WebCore::SharedBuffer> dataSelectionForPasteboard(const String& pasteboardType);
    void makeFirstResponder();
    void assistiveTechnologyMakeFirstResponder();

    ColorSpaceData colorSpace();
#endif

#if ENABLE(SERVICE_CONTROLS)
    void replaceSelectionWithPasteboardData(const Vector<String>& types, const IPC::DataReference&);
#endif

    void pageScaleFactorDidChange(double);
    void pluginScaleFactorDidChange(double);
    void pluginZoomFactorDidChange(double);

    // Find.
    void findString(const String&, FindOptions, unsigned maxMatchCount);
    void findStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void getImageForFindMatch(int32_t matchIndex);
    void selectFindMatch(int32_t matchIndex);
    void didGetImageForFindMatch(const ShareableBitmap::Handle& contentImageHandle, uint32_t matchIndex);
    void hideFindUI();
    void countStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void replaceMatches(Vector<uint32_t>&& matchIndices, const String& replacementText, bool selectionOnly, Function<void(uint64_t, CallbackBase::Error)>&&);
    void didCountStringMatches(const String&, uint32_t matchCount);
    void setTextIndicator(const WebCore::TextIndicatorData&, uint64_t /* WebCore::TextIndicatorWindowLifetime */ lifetime = 0 /* Permanent */);
    void setTextIndicatorAnimationProgress(float);
    void clearTextIndicator();
    void didFindString(const String&, const Vector<WebCore::IntRect>&, uint32_t matchCount, int32_t matchIndex, bool didWrapAround);
    void didFailToFindString(const String&);
    void didFindStringMatches(const String&, const Vector<Vector<WebCore::IntRect>>& matchRects, int32_t firstIndexAfterSelection);

    void getContentsAsString(WTF::Function<void (const String&, CallbackBase::Error)>&&);
    void getBytecodeProfile(WTF::Function<void (const String&, CallbackBase::Error)>&&);
    void getSamplingProfilerOutput(WTF::Function<void (const String&, CallbackBase::Error)>&&);

#if ENABLE(MHTML)
    void getContentsAsMHTMLData(Function<void (API::Data*, CallbackBase::Error)>&&);
#endif
    void getMainResourceDataOfFrame(WebFrameProxy*, Function<void (API::Data*, CallbackBase::Error)>&&);
    void getResourceDataFromFrame(WebFrameProxy*, API::URL*, Function<void (API::Data*, CallbackBase::Error)>&&);
    void getRenderTreeExternalRepresentation(WTF::Function<void (const String&, CallbackBase::Error)>&&);
    void getSelectionOrContentsAsString(WTF::Function<void (const String&, CallbackBase::Error)>&&);
    void getSelectionAsWebArchiveData(Function<void (API::Data*, CallbackBase::Error)>&&);
    void getSourceForFrame(WebFrameProxy*, WTF::Function<void (const String&, CallbackBase::Error)>&&);
    void getWebArchiveOfFrame(WebFrameProxy*, Function<void (API::Data*, CallbackBase::Error)>&&);
    void runJavaScriptInMainFrame(const String&, bool, WTF::Function<void (API::SerializedScriptValue*, bool hadException, const WebCore::ExceptionDetails&, CallbackBase::Error)>&& callbackFunction);
    void runJavaScriptInMainFrameScriptWorld(const String&, bool, const String& worldName, WTF::Function<void(API::SerializedScriptValue*, bool hadException, const WebCore::ExceptionDetails&, CallbackBase::Error)>&& callbackFunction);
    void forceRepaint(RefPtr<VoidCallback>&&);

    float headerHeight(WebFrameProxy&);
    float footerHeight(WebFrameProxy&);
    void drawHeader(WebFrameProxy&, WebCore::FloatRect&&);
    void drawFooter(WebFrameProxy&, WebCore::FloatRect&&);

#if PLATFORM(COCOA)
    // Dictionary.
    void performDictionaryLookupAtLocation(const WebCore::FloatPoint&);
    void performDictionaryLookupOfCurrentSelection();
#endif

    class PolicyDecisionSender;
    enum class WillContinueLoadInNewProcess : bool { No, Yes };
    void receivedPolicyDecision(WebCore::PolicyAction, API::Navigation*, Optional<WebsitePoliciesData>&&, Ref<PolicyDecisionSender>&&, WillContinueLoadInNewProcess = WillContinueLoadInNewProcess::No);
    void receivedNavigationPolicyDecision(WebCore::PolicyAction, API::Navigation*, ProcessSwapRequestedByClient, WebFrameProxy&, API::WebsitePolicies*, Ref<PolicyDecisionSender>&&);

    void backForwardRemovedItem(const WebCore::BackForwardItemIdentifier&);

#if ENABLE(DRAG_SUPPORT)    
    // Drag and drop support.
    void dragEntered(WebCore::DragData&, const String& dragStorageName = String());
    void dragUpdated(WebCore::DragData&, const String& dragStorageName = String());
    void dragExited(WebCore::DragData&, const String& dragStorageName = String());
    void performDragOperation(WebCore::DragData&, const String& dragStorageName, SandboxExtension::Handle&&, SandboxExtension::HandleArray&&);
    void didPerformDragOperation(bool handled);

    void didPerformDragControllerAction(uint64_t dragOperation, WebCore::DragHandlingMethod, bool mouseIsOverFileInput, unsigned numberOfItemsToBeAccepted, const WebCore::IntRect& insertionRect, const WebCore::IntRect& editableElementRect);
    void dragEnded(const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition, uint64_t operation);
    void didStartDrag();
    void dragCancelled();
    void setDragCaretRect(const WebCore::IntRect&);
#if PLATFORM(COCOA)
    void startDrag(const WebCore::DragItem&, const ShareableBitmap::Handle& dragImageHandle);
    void setPromisedDataForImage(const String& pasteboardName, const SharedMemory::Handle& imageHandle, uint64_t imageSize, const String& filename, const String& extension,
                         const String& title, const String& url, const String& visibleURL, const SharedMemory::Handle& archiveHandle, uint64_t archiveSize);
#endif
#if PLATFORM(GTK)
    void startDrag(WebSelectionData&&, uint64_t dragOperation, const ShareableBitmap::Handle& dragImage);
#endif
#endif

    void processDidBecomeUnresponsive();
    void processDidBecomeResponsive();
    void processDidTerminate(ProcessTerminationReason);
    void provisionalProcessDidTerminate();
    void dispatchProcessDidTerminate(ProcessTerminationReason);
    void willChangeProcessIsResponsive();
    void didChangeProcessIsResponsive();

#if PLATFORM(IOS_FAMILY)
    void processWillBecomeSuspended();
    void processWillBecomeForeground();
#endif

    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&);

    void addEditCommand(WebEditCommandProxy&);
    void removeEditCommand(WebEditCommandProxy&);
    void registerEditCommand(Ref<WebEditCommandProxy>&&, UndoOrRedo);

    bool canUndo();
    bool canRedo();

#if PLATFORM(COCOA)
    void registerKeypressCommandName(const String& name) { m_knownKeypressCommandNames.add(name); }
    bool isValidKeypressCommandName(const String& name) const { return m_knownKeypressCommandNames.contains(name); }
#endif

    WebProcessProxy& process() { return m_process; }
    ProcessID processIdentifier() const;

    WebPreferences& preferences() { return m_preferences; }
    void setPreferences(WebPreferences&);

    WebPageGroup& pageGroup() { return m_pageGroup; }

    bool isValid() const;

#if ENABLE(DRAG_SUPPORT)
    WebCore::DragOperation currentDragOperation() const { return m_currentDragOperation; }
    WebCore::DragHandlingMethod currentDragHandlingMethod() const { return m_currentDragHandlingMethod; }
    bool currentDragIsOverFileInput() const { return m_currentDragIsOverFileInput; }
    unsigned currentDragNumberOfFilesToBeAccepted() const { return m_currentDragNumberOfFilesToBeAccepted; }
    WebCore::IntRect currentDragCaretRect() const { return m_currentDragCaretRect; }
    WebCore::IntRect currentDragCaretEditableElementRect() const { return m_currentDragCaretEditableElementRect; }
    void resetCurrentDragInformation();
    void didEndDragging();
#endif

    void preferencesDidChange();

#if ENABLE(CONTEXT_MENUS)
    // Called by the WebContextMenuProxy.
    void contextMenuItemSelected(const WebContextMenuItemData&);
    void handleContextMenuKeyEvent();
#endif

    // Called by the WebOpenPanelResultListenerProxy.
#if PLATFORM(IOS_FAMILY)
    void didChooseFilesForOpenPanelWithDisplayStringAndIcon(const Vector<String>&, const String& displayString, const API::Data* iconData);
#endif
    void didChooseFilesForOpenPanel(const Vector<String>&);
    void didCancelForOpenPanel();

    WebPageCreationParameters creationParameters(WebProcessProxy&);

    void handleDownloadRequest(DownloadProxy*);

    void advanceToNextMisspelling(bool startBeforeSelection);
    void changeSpellingToWord(const String& word);
#if USE(APPKIT)
    void uppercaseWord();
    void lowercaseWord();
    void capitalizeWord();
#endif

#if PLATFORM(COCOA)
    bool isSmartInsertDeleteEnabled() const { return m_isSmartInsertDeleteEnabled; }
    void setSmartInsertDeleteEnabled(bool);
#endif

    void setCanRunModal(bool);
    bool canRunModal();

    void beginPrinting(WebFrameProxy*, const PrintInfo&);
    void endPrinting();
    void computePagesForPrinting(WebFrameProxy*, const PrintInfo&, Ref<ComputedPagesCallback>&&);
#if PLATFORM(COCOA)
    void drawRectToImage(WebFrameProxy*, const PrintInfo&, const WebCore::IntRect&, const WebCore::IntSize&, Ref<ImageCallback>&&);
    void drawPagesToPDF(WebFrameProxy*, const PrintInfo&, uint32_t first, uint32_t count, Ref<DataCallback>&&);
#if PLATFORM(IOS_FAMILY)
    uint32_t computePagesForPrintingAndDrawToPDF(uint64_t frameID, const PrintInfo&, DrawToPDFCallback::CallbackFunction&&);
    void drawToPDFCallback(const IPC::DataReference& pdfData, WebKit::CallbackID);
#endif
#elif PLATFORM(GTK)
    void drawPagesForPrinting(WebFrameProxy*, const PrintInfo&, Ref<PrintFinishedCallback>&&);
#endif

    PageLoadState& pageLoadState() { return m_pageLoadState; }

#if PLATFORM(COCOA)
    void handleAlternativeTextUIResult(const String& result);
#endif

    void saveDataToFileInDownloadsFolder(String&& suggestedFilename, String&& mimeType, URL&& originatingURL, API::Data&);
    void savePDFToFileInDownloadsFolder(String&& suggestedFilename, URL&& originatingURL, const IPC::DataReference&);
#if PLATFORM(COCOA)
    void savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size, const String& pdfUUID);
    void savePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const IPC::DataReference&, const String& pdfUUID);
    void openPDFFromTemporaryFolderWithNativeApplication(const String& pdfUUID);
#endif

#if ENABLE(PDFKIT_PLUGIN)
    void showPDFContextMenu(const WebKit::PDFContextMenu&, Optional<int32_t>& selectedIndex);
#endif
    WebCore::IntRect visibleScrollerThumbRect() const { return m_visibleScrollerThumbRect; }

    uint64_t renderTreeSize() const { return m_renderTreeSize; }

    void printMainFrame();
    
    void setMediaVolume(float);
    void setMuted(WebCore::MediaProducer::MutedStateFlags);
    void setMayStartMediaWhenInWindow(bool);
    bool mayStartMediaWhenInWindow() const { return m_mayStartMediaWhenInWindow; }
    void setMediaCaptureEnabled(bool);
    bool mediaCaptureEnabled() const { return m_mediaCaptureEnabled; }
    void stopMediaCapture();

    void stopAllMediaPlayback();
    void suspendAllMediaPlayback();
    void resumeAllMediaPlayback();

#if ENABLE(MEDIA_SESSION)
    bool hasMediaSessionWithActiveMediaElements() const { return m_hasMediaSessionWithActiveMediaElements; }
    void handleMediaEvent(WebCore::MediaEventType);
    void setVolumeOfMediaElement(double, uint64_t);
#endif
        
#if ENABLE(POINTER_LOCK)
    void didAllowPointerLock();
    void didDenyPointerLock();
#endif

    // WebPopupMenuProxy::Client
    NativeWebMouseEvent* currentlyProcessedMouseDownEvent() override;

    void setSuppressVisibilityUpdates(bool flag);
    bool suppressVisibilityUpdates() { return m_suppressVisibilityUpdates; }

#if PLATFORM(IOS_FAMILY)
    void willStartUserTriggeredZooming();

    void potentialTapAtPosition(const WebCore::FloatPoint&, uint64_t& requestID);
    void commitPotentialTap(OptionSet<WebKit::WebEvent::Modifier>, uint64_t layerTreeTransactionIdAtLastTouchStart);
    void cancelPotentialTap();
    void tapHighlightAtPosition(const WebCore::FloatPoint&, uint64_t& requestID);
    void handleTap(const WebCore::FloatPoint&, OptionSet<WebKit::WebEvent::Modifier>, uint64_t layerTreeTransactionIdAtLastTouchStart);

    void inspectorNodeSearchMovedToPosition(const WebCore::FloatPoint&);
    void inspectorNodeSearchEndedAtPosition(const WebCore::FloatPoint&);

    void blurFocusedElement();
#endif

    void postMessageToInjectedBundle(const String& messageName, API::Object* messageBody);

#if ENABLE(INPUT_TYPE_COLOR)
    void setColorPickerColor(const WebCore::Color&);
    void endColorPicker();
#endif

    WebCore::IntSize viewLayoutSize() const { return m_viewLayoutSize; }
    void setViewLayoutSize(const WebCore::IntSize&);

    bool autoSizingShouldExpandToViewHeight() const { return m_autoSizingShouldExpandToViewHeight; }
    void setAutoSizingShouldExpandToViewHeight(bool);

    void setViewportSizeForCSSViewportUnits(const WebCore::IntSize&);
    WebCore::IntSize viewportSizeForCSSViewportUnits() const { return m_viewportSizeForCSSViewportUnits.valueOr(WebCore::IntSize()); }

    void didReceiveAuthenticationChallengeProxy(uint64_t frameID, Ref<AuthenticationChallengeProxy>&&);

    SpellDocumentTag spellDocumentTag();

    void didFinishCheckingText(uint64_t requestID, const Vector<WebCore::TextCheckingResult>&);
    void didCancelCheckingText(uint64_t requestID);

    void connectionWillOpen(IPC::Connection&);
    void webProcessWillShutDown();

    void processDidFinishLaunching();

    void didSaveToPageCache();
        
    void setScrollPinningBehavior(WebCore::ScrollPinningBehavior);
    WebCore::ScrollPinningBehavior scrollPinningBehavior() const { return m_scrollPinningBehavior; }

    void setOverlayScrollbarStyle(Optional<WebCore::ScrollbarOverlayStyle>);
    Optional<WebCore::ScrollbarOverlayStyle> overlayScrollbarStyle() const { return m_scrollbarOverlayStyle; }

    // When the state of the window changes such that the WebPage needs immediate update, the UIProcess sends a new
    // ActivityStateChangeID to the WebProcess through the SetActivityState message. The UIProcess will wait till it
    // receives a CommitLayerTree which has an ActivityStateChangeID equal to or greater than the one it sent.
    ActivityStateChangeID takeNextActivityStateChangeID() { return ++m_currentActivityStateChangeID; }

    bool shouldRecordNavigationSnapshots() const { return m_shouldRecordNavigationSnapshots; }
    void setShouldRecordNavigationSnapshots(bool shouldRecordSnapshots) { m_shouldRecordNavigationSnapshots = shouldRecordSnapshots; }
    void recordAutomaticNavigationSnapshot();
    void suppressNextAutomaticNavigationSnapshot() { m_shouldSuppressNextAutomaticNavigationSnapshot = true; }
    void recordNavigationSnapshot(WebBackForwardListItem&);
    void requestFocusedElementInformation(Function<void(const FocusedElementInformation&, CallbackBase::Error)>&&);

#if PLATFORM(COCOA) || PLATFORM(GTK)
    RefPtr<ViewSnapshot> takeViewSnapshot();
#endif

#if ENABLE(WEB_CRYPTO)
    void wrapCryptoKey(const Vector<uint8_t>&, bool& succeeded, Vector<uint8_t>&);
    void unwrapCryptoKey(const Vector<uint8_t>&, bool& succeeded, Vector<uint8_t>&);
#endif

    void signedPublicKeyAndChallengeString(unsigned keySizeIndex, const String& challengeString, const URL&, String& result);

    void takeSnapshot(WebCore::IntRect, WebCore::IntSize bitmapSize, SnapshotOptions, WTF::Function<void (const ShareableBitmap::Handle&, CallbackBase::Error)>&&);

    void navigationGestureDidBegin();
    void navigationGestureWillEnd(bool willNavigate, WebBackForwardListItem&);
    void navigationGestureDidEnd(bool willNavigate, WebBackForwardListItem&);
    void navigationGestureDidEnd();
    void navigationGestureSnapshotWasRemoved();
    void willRecordNavigationSnapshot(WebBackForwardListItem&);

    bool isShowingNavigationGestureSnapshot() const { return m_isShowingNavigationGestureSnapshot; }

    bool isPlayingAudio() const { return !!(m_mediaState & WebCore::MediaProducer::IsPlayingAudio); }
    void isPlayingMediaDidChange(WebCore::MediaProducer::MediaStateFlags, uint64_t);
    void updatePlayingMediaDidChange(WebCore::MediaProducer::MediaStateFlags);
    bool hasActiveAudioStream() const { return m_mediaState & WebCore::MediaProducer::HasActiveAudioCaptureDevice; }
    bool hasActiveVideoStream() const { return m_mediaState & WebCore::MediaProducer::HasActiveVideoCaptureDevice; }
    WebCore::MediaProducer::MediaStateFlags mediaStateFlags() const { return m_mediaState; }
    void handleAutoplayEvent(WebCore::AutoplayEvent, OptionSet<WebCore::AutoplayEventFlags>);

    void videoControlsManagerDidChange();
    bool hasActiveVideoForControlsManager() const;
    void requestControlledElementID() const;
    void handleControlledElementIDResponse(const String&) const;
    bool isPlayingVideoInEnhancedFullscreen() const;

#if PLATFORM(COCOA)
    void requestActiveNowPlayingSessionInfo(Ref<NowPlayingInfoCallback>&&);
    void nowPlayingInfoCallback(bool, bool, const String&, double, double, uint64_t, CallbackID);
#endif

#if ENABLE(MEDIA_SESSION)
    void hasMediaSessionWithActiveMediaElementsDidChange(bool);
    void mediaSessionMetadataDidChange(const WebCore::MediaSessionMetadata&);
    void focusedContentMediaElementDidChange(uint64_t);
#endif

#if PLATFORM(MAC)
    API::HitTestResult* lastMouseMoveHitTestResult() const { return m_lastMouseMoveHitTestResult.get(); }
    void performImmediateActionHitTestAtLocation(WebCore::FloatPoint);

    void immediateActionDidUpdate();
    void immediateActionDidCancel();
    void immediateActionDidComplete();

    NSObject *immediateActionAnimationControllerForHitTestResult(RefPtr<API::HitTestResult>, uint64_t, RefPtr<API::Object>);

    void installActivityStateChangeCompletionHandler(WTF::Function<void ()>&&);

    void handleAcceptedCandidate(WebCore::TextCheckingResult);
    void didHandleAcceptedCandidate();

    void setHeaderBannerHeightForTesting(int);
    void setFooterBannerHeightForTesting(int);
#endif

#if USE(UNIFIED_TEXT_CHECKING)
    void checkTextOfParagraph(const String& text, OptionSet<WebCore::TextCheckingType> checkingTypes, int32_t insertionPoint, Vector<WebCore::TextCheckingResult>& results);
#endif
    void getGuessesForWord(const String& word, const String& context, int32_t insertionPoint, Vector<String>& guesses);

    void setShouldDispatchFakeMouseMoveEvents(bool);

    // Diagnostic messages logging.
    void logDiagnosticMessage(const String& message, const String& description, WebCore::ShouldSample);
    void logDiagnosticMessageWithResult(const String& message, const String& description, uint32_t result, WebCore::ShouldSample);
    void logDiagnosticMessageWithValue(const String& message, const String& description, double value, unsigned significantFigures, WebCore::ShouldSample);
    void logDiagnosticMessageWithEnhancedPrivacy(const String& message, const String& description, WebCore::ShouldSample);

    // Performance logging.
    void logScrollingEvent(uint32_t eventType, MonotonicTime, uint64_t);

    // Form validation messages.
    void showValidationMessage(const WebCore::IntRect& anchorClientRect, const String& message);
    void hideValidationMessage();
#if PLATFORM(COCOA)
    WebCore::ValidationBubble* validationBubble() const { return m_validationBubble.get(); } // For testing.
#endif

#if PLATFORM(IOS_FAMILY)
    void setIsKeyboardAnimatingIn(bool isKeyboardAnimatingIn) { m_isKeyboardAnimatingIn = isKeyboardAnimatingIn; }
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    void addPlaybackTargetPickerClient(uint64_t);
    void removePlaybackTargetPickerClient(uint64_t);
    void showPlaybackTargetPicker(uint64_t, const WebCore::FloatRect&, bool hasVideo);
    void playbackTargetPickerClientStateDidChange(uint64_t, WebCore::MediaProducer::MediaStateFlags);
    void setMockMediaPlaybackTargetPickerEnabled(bool);
    void setMockMediaPlaybackTargetPickerState(const String&, WebCore::MediaPlaybackTargetContext::State);

    // WebMediaSessionManagerClient
    void setPlaybackTarget(uint64_t, Ref<WebCore::MediaPlaybackTarget>&&) override;
    void externalOutputDeviceAvailableDidChange(uint64_t, bool) override;
    void setShouldPlayToPlaybackTarget(uint64_t, bool) override;
#endif

    void didChangeBackgroundColor();
    void didLayoutForCustomContentProvider();

    // For testing
    void clearWheelEventTestTrigger();
    void callAfterNextPresentationUpdate(WTF::Function<void (CallbackBase::Error)>&&);

    void didReachLayoutMilestone(OptionSet<WebCore::LayoutMilestone>);

    void didRestoreScrollPosition();

    void getLoadDecisionForIcon(const WebCore::LinkIcon&, WebKit::CallbackID);
    void finishedLoadingIcon(WebKit::CallbackID, const IPC::DataReference&);

    void setFocus(bool focused);
    void setWindowFrame(const WebCore::FloatRect&);
    void getWindowFrame(Messages::WebPageProxy::GetWindowFrame::DelayedReply&&);
    void getWindowFrameWithCallback(Function<void(WebCore::FloatRect)>&&);

    bool isResourceCachingDisabled() const { return m_isResourceCachingDisabled; }
    void setResourceCachingDisabled(bool);

    WebCore::UserInterfaceLayoutDirection userInterfaceLayoutDirection();
    void setUserInterfaceLayoutDirection(WebCore::UserInterfaceLayoutDirection);

    bool hasHadSelectionChangesFromUserInteraction() const { return m_hasHadSelectionChangesFromUserInteraction; }
    bool needsHiddenContentEditableQuirk() const { return m_needsHiddenContentEditableQuirk; }
    bool needsPlainTextQuirk() const { return m_needsPlainTextQuirk; }

    bool isAlwaysOnLoggingAllowed() const;

#if ENABLE(GAMEPAD)
    void gamepadActivity(const Vector<GamepadData>&, bool shouldMakeGamepadsVisible);
#endif

    void isLoadingChanged() { activityStateDidChange(WebCore::ActivityState::IsLoading); }

    void clearUserMediaState();

    void setShouldSkipWaitingForPaintAfterNextViewDidMoveToWindow(bool shouldSkip) { m_shouldSkipWaitingForPaintAfterNextViewDidMoveToWindow = shouldSkip; }

    void setURLSchemeHandlerForScheme(Ref<WebURLSchemeHandler>&&, const String& scheme);
    WebURLSchemeHandler* urlSchemeHandlerForScheme(const String& scheme);

#if PLATFORM(COCOA)
    void createSandboxExtensionsIfNeeded(const Vector<String>& files, SandboxExtension::Handle& fileReadHandle, SandboxExtension::HandleArray& fileUploadHandles);
#endif
    void editorStateChanged(const EditorState&);

#if PLATFORM(COCOA)
    void touchBarMenuDataRemoved();
    void touchBarMenuDataChanged(const TouchBarMenuData&);
    void touchBarMenuItemDataAdded(const TouchBarMenuItemData&);
    void touchBarMenuItemDataRemoved(const TouchBarMenuItemData&);
#endif

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void requestStorageAccessConfirm(const String& subFrameHost, const String& topFrameHost, uint64_t frameID, CompletionHandler<void(bool)>&&);
#endif

    static WebPageProxy* nonEphemeralWebPageProxy();

#if ENABLE(ATTACHMENT_ELEMENT)
    RefPtr<API::Attachment> attachmentForIdentifier(const String& identifier) const;
    void insertAttachment(Ref<API::Attachment>&&, Function<void(CallbackBase::Error)>&&);
    void updateAttachmentAttributes(const API::Attachment&, Function<void(CallbackBase::Error)>&&);
    void serializedAttachmentDataForIdentifiers(const Vector<String>&, Vector<WebCore::SerializedAttachmentData>&);
    void registerAttachmentIdentifier(const String&);
    void didInvalidateDataForAttachment(API::Attachment&);

    enum class ShouldUpdateAttachmentAttributes : bool { No, Yes };
    ShouldUpdateAttachmentAttributes willUpdateAttachmentAttributes(const API::Attachment&);
#endif

#if ENABLE(APPLICATION_MANIFEST)
    void getApplicationManifest(Function<void(const Optional<WebCore::ApplicationManifest>&, CallbackBase::Error)>&&);
#endif

    WebPreferencesStore preferencesStore() const;

    void setDefersLoadingForTesting(bool);

    bool isPageOpenedByDOMShowingInitialEmptyDocument() const;

    WebCore::IntRect syncRootViewToScreen(const WebCore::IntRect& viewRect);

#if ENABLE(DATALIST_ELEMENT)
    void didSelectOption(const String&);
    void didCloseSuggestions();
#endif

    void updateCurrentModifierState();

#if HAVE(PENCILKIT)
    EditableImageController& editableImageController() { return *m_editableImageController; }
#endif

    ProvisionalPageProxy* provisionalPageProxy() const { return m_provisionalPage.get(); }
    void commitProvisionalPage(uint64_t frameID, uint64_t navigationID, const String& mimeType, bool frameHasCustomContentProvider, uint32_t frameLoadType, const WebCore::CertificateInfo&, bool containsPluginDocument, Optional<WebCore::HasInsecureContent> forcedHasInsecureContent, const UserData&);

    // Logic shared between the WebPageProxy and the ProvisionalPageProxy.
    void didStartProvisionalLoadForFrameShared(Ref<WebProcessProxy>&&, uint64_t frameID, uint64_t navigationID, URL&&, URL&& unreachableURL, const UserData&);
    void didFailProvisionalLoadForFrameShared(Ref<WebProcessProxy>&&, uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, uint64_t navigationID, const String& provisionalURL, const WebCore::ResourceError&, const UserData&);
    void didReceiveServerRedirectForProvisionalLoadForFrameShared(Ref<WebProcessProxy>&&, uint64_t frameID, uint64_t navigationID, WebCore::ResourceRequest&&, const UserData&);
    void didPerformClientRedirectShared(Ref<WebProcessProxy>&&, const String& sourceURLString, const String& destinationURLString, uint64_t frameID);
    void didNavigateWithNavigationDataShared(Ref<WebProcessProxy>&&, const WebNavigationDataStore&, uint64_t frameID);
    void didChangeProvisionalURLForFrameShared(Ref<WebProcessProxy>&&, uint64_t frameID, uint64_t navigationID, URL&&);
    void decidePolicyForNavigationActionAsyncShared(Ref<WebProcessProxy>&&, uint64_t frameID, WebCore::SecurityOriginData&& frameSecurityOrigin, WebCore::PolicyCheckIdentifier,
        uint64_t navigationID, NavigationActionData&&, FrameInfoData&&, uint64_t originatingPageID, const WebCore::ResourceRequest& originalRequest, WebCore::ResourceRequest&&,
        IPC::FormDataReference&& requestBody, WebCore::ResourceResponse&& redirectResponse, const UserData&, uint64_t listenerID);
    void decidePolicyForResponseShared(Ref<WebProcessProxy>&&, uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, WebCore::PolicyCheckIdentifier,
        uint64_t navigationID, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, bool canShowMIMEType, uint64_t listenerID, const UserData&);
    void startURLSchemeTaskShared(Ref<WebProcessProxy>&&, URLSchemeTaskParameters&&);
    void loadDataWithNavigationShared(Ref<WebProcessProxy>&&, API::Navigation&, const IPC::DataReference&, const String& MIMEType, const String& encoding, const String& baseURL, API::Object* userData, WebCore::ShouldTreatAsContinuingLoad, Optional<WebsitePoliciesData>&& = WTF::nullopt);
    void loadRequestWithNavigationShared(Ref<WebProcessProxy>&&, API::Navigation&, WebCore::ResourceRequest&&, WebCore::ShouldOpenExternalURLsPolicy, API::Object* userData, WebCore::ShouldTreatAsContinuingLoad, Optional<WebsitePoliciesData>&& = WTF::nullopt);
    void backForwardGoToItemShared(Ref<WebProcessProxy>&&, const WebCore::BackForwardItemIdentifier&, SandboxExtension::Handle&);

    // IPC::MessageReceiver
    // Implemented in generated WebPageProxyMessageReceiver.cpp
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;

private:
    WebPageProxy(PageClient&, WebProcessProxy&, uint64_t pageID, Ref<API::PageConfiguration>&&);
    void platformInitialize();

    void notifyProcessPoolToPrewarm();

    RefPtr<API::Navigation> goToBackForwardItem(WebBackForwardListItem&, WebCore::FrameLoadType);

    void updateActivityState(OptionSet<WebCore::ActivityState::Flag> flagsToUpdate = WebCore::ActivityState::allFlags());
    void updateThrottleState();
    void updateHiddenPageThrottlingAutoIncreases();

    bool suspendCurrentPageIfPossible(API::Navigation&, Optional<uint64_t> mainFrameID, ProcessSwapRequestedByClient);

    enum class ResetStateReason {
        PageInvalidated,
        WebProcessExited,
        NavigationSwap,
    };
    void resetState(ResetStateReason);
    void resetStateAfterProcessExited(ProcessTerminationReason);

    void setUserAgent(String&&);

    // IPC::MessageSender
    bool sendMessage(std::unique_ptr<IPC::Encoder>, OptionSet<IPC::SendOption>) override;
    IPC::Connection* messageSenderConnection() const override;
    uint64_t messageSenderDestinationID() const override;

    // WebPopupMenuProxy::Client
    void valueChangedForPopupMenu(WebPopupMenuProxy*, int32_t newSelectedIndex) override;
    void setTextFromItemForPopupMenu(WebPopupMenuProxy*, int32_t index) override;
#if PLATFORM(GTK)
    void failedToShowPopupMenu() override;
#endif

#if ENABLE(POINTER_LOCK)
    void requestPointerLock();
    void requestPointerUnlock();
#endif

    void didCreateMainFrame(uint64_t frameID);
    void didCreateSubframe(uint64_t frameID);
    void didCreateWindow(uint64_t frameID, WebCore::GlobalWindowIdentifier&&);

    void didStartProvisionalLoadForFrame(uint64_t frameID, uint64_t navigationID, URL&&, URL&& unreachableURL, const UserData&);
    void didReceiveServerRedirectForProvisionalLoadForFrame(uint64_t frameID, uint64_t navigationID, WebCore::ResourceRequest&&, const UserData&);
    void willPerformClientRedirectForFrame(uint64_t frameID, const String& url, double delay, WebCore::LockBackForwardList);
    void didCancelClientRedirectForFrame(uint64_t frameID);
    void didChangeProvisionalURLForFrame(uint64_t frameID, uint64_t navigationID, URL&&);
    void didFailProvisionalLoadForFrame(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, uint64_t navigationID, const String& provisionalURL, const WebCore::ResourceError&, const UserData&);
    void didCommitLoadForFrame(uint64_t frameID, uint64_t navigationID, const String& mimeType, bool frameHasCustomContentProvider, uint32_t frameLoadType, const WebCore::CertificateInfo&, bool containsPluginDocument, Optional<WebCore::HasInsecureContent> forcedHasInsecureContent, const UserData&);
    void didFinishDocumentLoadForFrame(uint64_t frameID, uint64_t navigationID, const UserData&);
    void didFinishLoadForFrame(uint64_t frameID, uint64_t navigationID, const UserData&);
    void didFailLoadForFrame(uint64_t frameID, uint64_t navigationID, const WebCore::ResourceError&, const UserData&);
    void didSameDocumentNavigationForFrame(uint64_t frameID, uint64_t navigationID, uint32_t sameDocumentNavigationType, URL&&, const UserData&);
    void didChangeMainDocument(uint64_t frameID);

    void didReceiveTitleForFrame(uint64_t frameID, const String&, const UserData&);
    void didFirstLayoutForFrame(uint64_t frameID, const UserData&);
    void didFirstVisuallyNonEmptyLayoutForFrame(uint64_t frameID, const UserData&);
    void didDisplayInsecureContentForFrame(uint64_t frameID, const UserData&);
    void didRunInsecureContentForFrame(uint64_t frameID, const UserData&);
    void didDetectXSSForFrame(uint64_t frameID, const UserData&);
    void mainFramePluginHandlesPageScaleGestureDidChange(bool);
    void frameDidBecomeFrameSet(uint64_t frameID, bool);
    void didStartProgress();
    void didChangeProgress(double);
    void didFinishProgress();
    void setNetworkRequestsInProgress(bool);

    void hasInsecureContent(WebCore::HasInsecureContent&);

    void didDestroyNavigation(uint64_t navigationID);

    void decidePolicyForNavigationAction(Ref<WebProcessProxy>&&, WebFrameProxy&, WebCore::SecurityOriginData&&, uint64_t navigationID, NavigationActionData&&,
        FrameInfoData&&, uint64_t originatingPageID, const WebCore::ResourceRequest& originalRequest, WebCore::ResourceRequest&&, IPC::FormDataReference&& requestBody,
        WebCore::ResourceResponse&& redirectResponse, const UserData&, Ref<PolicyDecisionSender>&&);
    void decidePolicyForNavigationActionAsync(uint64_t frameID, WebCore::SecurityOriginData&&, WebCore::PolicyCheckIdentifier, uint64_t navigationID, NavigationActionData&&,
        FrameInfoData&&, uint64_t originatingPageID, const WebCore::ResourceRequest& originalRequest, WebCore::ResourceRequest&&, IPC::FormDataReference&& requestBody,
        WebCore::ResourceResponse&& redirectResponse, const UserData&, uint64_t listenerID);
    void decidePolicyForNavigationActionSync(uint64_t frameID, bool isMainFrame, WebCore::SecurityOriginData&&, WebCore::PolicyCheckIdentifier, uint64_t navigationID, NavigationActionData&&,
        FrameInfoData&&, uint64_t originatingPageID, const WebCore::ResourceRequest& originalRequest, WebCore::ResourceRequest&&, IPC::FormDataReference&& requestBody,
        WebCore::ResourceResponse&& redirectResponse, const UserData&, Messages::WebPageProxy::DecidePolicyForNavigationActionSync::DelayedReply&&);
    void decidePolicyForNewWindowAction(uint64_t frameID, const WebCore::SecurityOriginData&, WebCore::PolicyCheckIdentifier, NavigationActionData&&,
        WebCore::ResourceRequest&&, const String& frameName, uint64_t listenerID, const UserData&);
    void decidePolicyForResponse(uint64_t frameID, const WebCore::SecurityOriginData&, WebCore::PolicyCheckIdentifier, uint64_t navigationID,
        const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, bool canShowMIMEType, uint64_t listenerID, const UserData&);
    void unableToImplementPolicy(uint64_t frameID, const WebCore::ResourceError&, const UserData&);
    void beginSafeBrowsingCheck(const URL&, bool, WebFramePolicyListenerProxy&);

    void willSubmitForm(uint64_t frameID, uint64_t sourceFrameID, const Vector<std::pair<String, String>>& textFieldValues, uint64_t listenerID, const UserData&);
        
    void contentRuleListNotification(URL&&, Vector<String>&& identifiers, Vector<String>&& notifications);

    // History client
    void didNavigateWithNavigationData(const WebNavigationDataStore&, uint64_t frameID);
    void didPerformClientRedirect(const String& sourceURLString, const String& destinationURLString, uint64_t frameID);
    void didPerformServerRedirect(const String& sourceURLString, const String& destinationURLString, uint64_t frameID);
    void didUpdateHistoryTitle(const String& title, const String& url, uint64_t frameID);

    // UI client
    void createNewPage(const FrameInfoData&, uint64_t originatingPageID, WebCore::ResourceRequest&&, WebCore::WindowFeatures&&, NavigationActionData&&, Messages::WebPageProxy::CreateNewPage::DelayedReply&&);
    void showPage();
    void runJavaScriptAlert(uint64_t frameID, const WebCore::SecurityOriginData&, const String&, Messages::WebPageProxy::RunJavaScriptAlert::DelayedReply&&);
    void runJavaScriptConfirm(uint64_t frameID, const WebCore::SecurityOriginData&, const String&, Messages::WebPageProxy::RunJavaScriptConfirm::DelayedReply&&);
    void runJavaScriptPrompt(uint64_t frameID, const WebCore::SecurityOriginData&, const String&, const String&, Messages::WebPageProxy::RunJavaScriptPrompt::DelayedReply&&);
    void setStatusText(const String&);
    void mouseDidMoveOverElement(WebHitTestResultData&&, uint32_t modifiers, UserData&&);

#if ENABLE(NETSCAPE_PLUGIN_API)
    void unavailablePluginButtonClicked(uint32_t opaquePluginUnavailabilityReason, const String& mimeType, const String& pluginURLString, const String& pluginsPageURLString, const String& frameURLString, const String& pageURLString);
#endif // ENABLE(NETSCAPE_PLUGIN_API)
#if ENABLE(WEBGL)
    void webGLPolicyForURL(URL&&, Messages::WebPageProxy::WebGLPolicyForURL::DelayedReply&&);
    void resolveWebGLPolicyForURL(URL&&, Messages::WebPageProxy::ResolveWebGLPolicyForURL::DelayedReply&&);
#endif // ENABLE(WEBGL)
    void setToolbarsAreVisible(bool toolbarsAreVisible);
    void getToolbarsAreVisible(Messages::WebPageProxy::GetToolbarsAreVisible::DelayedReply&&);
    void setMenuBarIsVisible(bool menuBarIsVisible);
    void getMenuBarIsVisible(Messages::WebPageProxy::GetMenuBarIsVisible::DelayedReply&&);
    void setStatusBarIsVisible(bool statusBarIsVisible);
    void getStatusBarIsVisible(Messages::WebPageProxy::GetStatusBarIsVisible::DelayedReply&&);
    void getIsViewVisible(bool&);
    void setIsResizable(bool isResizable);
    void screenToRootView(const WebCore::IntPoint& screenPoint, Messages::WebPageProxy::ScreenToRootView::DelayedReply&&);
    void rootViewToScreen(const WebCore::IntRect& viewRect, Messages::WebPageProxy::RootViewToScreen::DelayedReply&&);
#if PLATFORM(IOS_FAMILY)
    void accessibilityScreenToRootView(const WebCore::IntPoint& screenPoint, WebCore::IntPoint& windowPoint);
    void rootViewToAccessibilityScreen(const WebCore::IntRect& viewRect, WebCore::IntRect& result);
#endif
    void runBeforeUnloadConfirmPanel(uint64_t frameID, const WebCore::SecurityOriginData&, const String& message, Messages::WebPageProxy::RunBeforeUnloadConfirmPanel::DelayedReply&&);
    void didChangeViewportProperties(const WebCore::ViewportAttributes&);
    void pageDidScroll();
    void runOpenPanel(uint64_t frameID, const WebCore::SecurityOriginData&, const WebCore::FileChooserSettings&);
    void showShareSheet(const WebCore::ShareDataWithParsedURL&, ShareSheetCallbackID);
    void printFrame(uint64_t frameID);
    void exceededDatabaseQuota(uint64_t frameID, const String& originIdentifier, const String& databaseName, const String& displayName, uint64_t currentQuota, uint64_t currentOriginUsage, uint64_t currentDatabaseUsage, uint64_t expectedUsage, Messages::WebPageProxy::ExceededDatabaseQuota::DelayedReply&&);
    void reachedApplicationCacheOriginQuota(const String& originIdentifier, uint64_t currentQuota, uint64_t totalBytesNeeded, Messages::WebPageProxy::ReachedApplicationCacheOriginQuota::DelayedReply&&);
    void requestGeolocationPermissionForFrame(uint64_t geolocationID, uint64_t frameID, String originIdentifier);

#if ENABLE(MEDIA_STREAM)
    UserMediaPermissionRequestManagerProxy& userMediaPermissionRequestManager();
#endif
    void requestUserMediaPermissionForFrame(uint64_t userMediaID, uint64_t frameID, const WebCore::SecurityOriginData& userMediaDocumentOriginIdentifier, const WebCore::SecurityOriginData& topLevelDocumentOriginIdentifier, WebCore::MediaStreamRequest&&);
    void enumerateMediaDevicesForFrame(uint64_t userMediaID, uint64_t frameID, const WebCore::SecurityOriginData& userMediaDocumentOriginData, const WebCore::SecurityOriginData& topLevelDocumentOriginData);
    void beginMonitoringCaptureDevices();

    void runModal();
    void notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect&);
    void recommendedScrollbarStyleDidChange(int32_t newStyle);
    void didChangeScrollbarsForMainFrame(bool hasHorizontalScrollbar, bool hasVerticalScrollbar);
    void didChangeScrollOffsetPinningForMainFrame(bool pinnedToLeftSide, bool pinnedToRightSide, bool pinnedToTopSide, bool pinnedToBottomSide);
    void didChangePageCount(unsigned);
    void pageExtendedBackgroundColorDidChange(const WebCore::Color&);
#if ENABLE(NETSCAPE_PLUGIN_API)
    void didFailToInitializePlugin(const String& mimeType, const String& frameURLString, const String& pageURLString);
    void didBlockInsecurePluginVersion(const String& mimeType, const String& pluginURLString, const String& frameURLString, const String& pageURLString, bool replacementObscured);
#endif // ENABLE(NETSCAPE_PLUGIN_API)
    void setCanShortCircuitHorizontalWheelEvents(bool canShortCircuitHorizontalWheelEvents) { m_canShortCircuitHorizontalWheelEvents = canShortCircuitHorizontalWheelEvents; }

    void reattachToWebProcess();
    void swapToWebProcess(Ref<WebProcessProxy>&&, std::unique_ptr<DrawingAreaProxy>&&, RefPtr<WebFrameProxy>&& mainFrame);
    void didFailToSuspendAfterProcessSwap();
    void didSuspendAfterProcessSwap();

    enum class ShouldInitializeWebPage { No, Yes };
    void finishAttachingToWebProcess(ShouldInitializeWebPage = ShouldInitializeWebPage::Yes);

    RefPtr<API::Navigation> reattachToWebProcessForReload();
    RefPtr<API::Navigation> reattachToWebProcessWithItem(WebBackForwardListItem&);

    void requestNotificationPermission(uint64_t notificationID, const String& originString);
    void showNotification(const String& title, const String& body, const String& iconURL, const String& tag, const String& lang, WebCore::NotificationDirection, const String& originString, uint64_t notificationID);
    void cancelNotification(uint64_t notificationID);
    void clearNotifications(const Vector<uint64_t>& notificationIDs);
    void didDestroyNotification(uint64_t notificationID);

    void didChangeContentSize(const WebCore::IntSize&);

#if ENABLE(INPUT_TYPE_COLOR)
    void showColorPicker(const WebCore::Color& initialColor, const WebCore::IntRect&, Vector<WebCore::Color>&&);
    void didChooseColor(const WebCore::Color&) override;
    void didEndColorPicker() override;
#endif

#if ENABLE(DATALIST_ELEMENT)
    void showDataListSuggestions(WebCore::DataListSuggestionInformation&&);
    void handleKeydownInDataList(const String&);
    void endDataListSuggestions();
#endif

    void closeOverlayedViews();

    void compositionWasCanceled();
    void setHasHadSelectionChangesFromUserInteraction(bool);
    void setNeedsHiddenContentEditableQuirk(bool);
    void setNeedsPlainTextQuirk(bool);

    // Back/Forward list management
    void backForwardAddItem(BackForwardListItemState&&);
    void backForwardGoToItem(const WebCore::BackForwardItemIdentifier&, SandboxExtension::Handle&);
    void backForwardItemAtIndex(int32_t index, Optional<WebCore::BackForwardItemIdentifier>&);
    void backForwardBackListCount(uint32_t& count);
    void backForwardForwardListCount(uint32_t& count);
    void backForwardClear();

    // Undo management
    void registerEditCommandForUndo(WebUndoStepID commandID, const String& label);
    void registerInsertionUndoGrouping();
    void clearAllEditCommands();
    void canUndoRedo(UndoOrRedo, bool& result);
    void executeUndoRedo(UndoOrRedo);

    // Keyboard handling
#if PLATFORM(COCOA)
    void executeSavedCommandBySelector(const String& selector, bool& handled);
#endif

#if PLATFORM(GTK)
    void getEditorCommandsForKeyEvent(const AtomicString&, Vector<String>&);
    void bindAccessibilityTree(const String&);
#endif

    // Popup Menu.
    void showPopupMenu(const WebCore::IntRect& rect, uint64_t textDirection, const Vector<WebPopupItem>& items, int32_t selectedIndex, const PlatformPopupMenuData&);
    void hidePopupMenu();

#if ENABLE(CONTEXT_MENUS)
    void showContextMenu(ContextMenuContextData&&, const UserData&);
#endif

#if ENABLE(TELEPHONE_NUMBER_DETECTION)
#if PLATFORM(MAC)
    void showTelephoneNumberMenu(const String& telephoneNumber, const WebCore::IntPoint&);
#endif
#endif

    // Search popup results
    void saveRecentSearches(const String&, const Vector<WebCore::RecentSearch>&);
    void loadRecentSearches(const String&, Vector<WebCore::RecentSearch>&);

#if PLATFORM(COCOA)
    // Speech.
    void getIsSpeaking(bool&);
    void speak(const String&);
    void stopSpeaking();

    // Spotlight.
    void searchWithSpotlight(const String&);
        
    void searchTheWeb(const String&);

    // Dictionary.
    void didPerformDictionaryLookup(const WebCore::DictionaryPopupInfo&);
#endif

#if PLATFORM(MAC)
    bool appleMailPaginationQuirkEnabled();
#endif

#if PLATFORM(MAC)
    // FIXME: Need to support iOS too, but there is no isAppleMail for iOS.
    bool appleMailLinesClampEnabled();
#endif

    // Spelling and grammar.
    void checkSpellingOfString(const String& text, int32_t& misspellingLocation, int32_t& misspellingLength);
    void checkGrammarOfString(const String& text, Vector<WebCore::GrammarDetail>&, int32_t& badGrammarLocation, int32_t& badGrammarLength);
    void spellingUIIsShowing(bool&);
    void updateSpellingUIWithMisspelledWord(const String& misspelledWord);
    void updateSpellingUIWithGrammarString(const String& badGrammarPhrase, const WebCore::GrammarDetail&);
    void learnWord(const String& word);
    void ignoreWord(const String& word);
    void requestCheckingOfString(uint64_t requestID, const WebCore::TextCheckingRequestData&, int32_t insertionPoint);

    void takeFocus(uint32_t direction);
    void setToolTip(const String&);
    void setCursor(const WebCore::Cursor&);
    void setCursorHiddenUntilMouseMoves(bool);

    void didReceiveEvent(uint32_t opaqueType, bool handled);

    void voidCallback(CallbackID);
    void dataCallback(const IPC::DataReference&, CallbackID);
    void imageCallback(const ShareableBitmap::Handle&, CallbackID);
    void stringCallback(const String&, CallbackID);
    void invalidateStringCallback(CallbackID);
    void scriptValueCallback(const IPC::DataReference&, bool hadException, const WebCore::ExceptionDetails&, CallbackID);
    void computedPagesCallback(const Vector<WebCore::IntRect>&, double totalScaleFactorForPrinting, CallbackID);
    void validateCommandCallback(const String&, bool, int, CallbackID);
    void unsignedCallback(uint64_t, CallbackID);
    void editingRangeCallback(const EditingRange&, CallbackID);
#if ENABLE(APPLICATION_MANIFEST)
    void applicationManifestCallback(const Optional<WebCore::ApplicationManifest>&, CallbackID);
#endif
#if PLATFORM(COCOA)
    void machSendRightCallback(const WTF::MachSendRight&, CallbackID);
#endif
    void rectForCharacterRangeCallback(const WebCore::IntRect&, const EditingRange&, CallbackID);
#if PLATFORM(MAC)
    void attributedStringForCharacterRangeCallback(const AttributedString&, const EditingRange&, CallbackID);
    void fontAtSelectionCallback(const String&, double, bool, CallbackID);
#endif
#if PLATFORM(IOS_FAMILY)
    void gestureCallback(const WebCore::IntPoint&, uint32_t gestureType, uint32_t gestureState, uint32_t flags, CallbackID);
    void touchesCallback(const WebCore::IntPoint&, uint32_t touches, uint32_t flags, CallbackID);
    void autocorrectionDataCallback(const Vector<WebCore::FloatRect>&, const String& fontName, float fontSize, uint64_t fontTraits, CallbackID);
    void autocorrectionContextCallback(const WebAutocorrectionContext&, CallbackID);
    void selectionContextCallback(const String& selectedText, const String& beforeText, const String& afterText, CallbackID);
    void interpretKeyEvent(const EditorState&, bool isCharEvent, bool& handled);
    void showPlaybackTargetPicker(bool hasVideo, const WebCore::IntRect& elementRect, WebCore::RouteSharingPolicy, const String&);
    void selectionRectsCallback(const Vector<WebCore::SelectionRect>&, CallbackID);

    void updateStringForFind(const String&);
#endif
#if PLATFORM(GTK)
    void printFinishedCallback(const WebCore::ResourceError&, CallbackID);
#endif

    void focusedFrameChanged(uint64_t frameID);
    void frameSetLargestFrameChanged(uint64_t frameID);

    void didFinishLoadingDataForCustomContentProvider(const String& suggestedFilename, const IPC::DataReference&);

#if PLATFORM(COCOA)
    void pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus);
    void setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, uint64_t complexTextInputState);
#endif

    bool maybeInitializeSandboxExtensionHandle(WebProcessProxy&, const URL&, SandboxExtension::Handle&);

#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    void toggleSmartInsertDelete();
    void toggleAutomaticQuoteSubstitution();
    void toggleAutomaticLinkDetection();
    void toggleAutomaticDashSubstitution();
    void toggleAutomaticTextReplacement();
#endif

#if PLATFORM(MAC)
    void substitutionsPanelIsShowing(bool&);
    void showCorrectionPanel(int32_t panelType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings);
    void dismissCorrectionPanel(int32_t reason);
    void dismissCorrectionPanelSoon(int32_t reason, String& result);
    void recordAutocorrectionResponse(int32_t responseType, const String& replacedString, const String& replacementString);

#if USE(DICTATION_ALTERNATIVES)
    void showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext);
    void removeDictationAlternatives(uint64_t dictationContext);
    void dictationAlternatives(uint64_t dictationContext, Vector<String>& result);
#endif

    void setEditableElementIsFocused(bool);
#endif // PLATFORM(MAC)

#if PLATFORM(IOS_FAMILY)
    WebCore::FloatSize screenSize();
    WebCore::FloatSize availableScreenSize();
    WebCore::FloatSize overrideScreenSize();
    float textAutosizingWidth();

    void couldNotRestorePageState();
    void restorePageState(Optional<WebCore::FloatPoint> scrollPosition, const WebCore::FloatPoint& scrollOrigin, const WebCore::FloatBoxExtent& obscuredInsetsOnSave, double scale);
    void restorePageCenterAndScale(Optional<WebCore::FloatPoint>, double scale);

    void didGetTapHighlightGeometries(uint64_t requestID, const WebCore::Color& color, const Vector<WebCore::FloatQuad>& geometries, const WebCore::IntSize& topLeftRadius, const WebCore::IntSize& topRightRadius, const WebCore::IntSize& bottomLeftRadius, const WebCore::IntSize& bottomRightRadius, bool nodeHasBuiltInClickHandling);

    void elementDidFocus(const FocusedElementInformation&, bool userIsInteracting, bool blurPreviousNode, bool changingActivityState, const UserData&);
    void elementDidBlur();
    void focusedElementDidChangeInputMode(WebCore::InputMode);
    void didReceiveEditorStateUpdateAfterFocus();

    void showInspectorHighlight(const WebCore::Highlight&);
    void hideInspectorHighlight();

    void enableInspectorNodeSearch();
    void disableInspectorNodeSearch();
    void focusedElementInformationCallback(const FocusedElementInformation&, CallbackID);
#endif // PLATFORM(IOS_FAMILY)

#if ENABLE(DATA_DETECTION)
    RetainPtr<NSArray> m_dataDetectionResults;
#endif

    void clearLoadDependentCallbacks();

    void performDragControllerAction(DragControllerAction, WebCore::DragData&, const String& dragStorageName, SandboxExtension::Handle&&, SandboxExtension::HandleArray&&);

    void updateBackingStoreDiscardableState();

    void setRenderTreeSize(uint64_t treeSize) { m_renderTreeSize = treeSize; }

#if PLATFORM(X11)
    void createPluginContainer(uint64_t& windowID);
    void windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID);
    void windowedPluginVisibilityDidChange(bool isVisible, uint64_t windowID);
#endif

    void processNextQueuedWheelEvent();
    void sendWheelEvent(const WebWheelEvent&);
    bool shouldProcessWheelEventNow(const WebWheelEvent&) const;

#if ENABLE(TOUCH_EVENTS)
    void updateTouchEventTracking(const WebTouchEvent&);
    WebCore::TrackingType touchEventTrackingType(const WebTouchEvent&) const;
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    void findPlugin(const String& mimeType, uint32_t processType, const String& urlString, const String& frameURLString, const String& pageURLString, bool allowOnlyApplicationPlugins, Messages::WebPageProxy::FindPlugin::DelayedReply&&);
#endif

#if USE(QUICK_LOOK)
    void didStartLoadForQuickLookDocumentInMainFrame(const String& fileName, const String& uti);
    void didFinishLoadForQuickLookDocumentInMainFrame(const QuickLookDocumentData&);
    void didRequestPasswordForQuickLookDocumentInMainFrame(const String& fileName);
#endif

#if ENABLE(CONTENT_FILTERING)
    void contentFilterDidBlockLoadForFrame(const WebCore::ContentFilterUnblockHandler&, uint64_t frameID);
#endif

    void tryReloadAfterProcessTermination();
    void resetRecentCrashCountSoon();
    void resetRecentCrashCount();

    API::DiagnosticLoggingClient* effectiveDiagnosticLoggingClient(WebCore::ShouldSample);

    void dispatchActivityStateChange();
    void viewDidLeaveWindow();
    void viewDidEnterWindow();

#if PLATFORM(MAC)
    void didPerformImmediateActionHitTest(const WebHitTestResultData&, bool contentPreventsDefault, const UserData&);
#endif

    void useFixedLayoutDidChange(bool useFixedLayout) { m_useFixedLayout = useFixedLayout; }
    void fixedLayoutSizeDidChange(WebCore::IntSize fixedLayoutSize) { m_fixedLayoutSize = fixedLayoutSize; }

    void imageOrMediaDocumentSizeChanged(const WebCore::IntSize&);
#if ENABLE(VIDEO)
#if USE(GSTREAMER)
    void requestInstallMissingMediaPlugins(const String& details, const String& description);
#endif
#endif

    void startURLSchemeTask(URLSchemeTaskParameters&&);
    void stopURLSchemeTask(uint64_t handlerIdentifier, uint64_t taskIdentifier);
    void loadSynchronousURLSchemeTask(URLSchemeTaskParameters&&, Messages::WebPageProxy::LoadSynchronousURLSchemeTask::DelayedReply&&);

    bool checkURLReceivedFromCurrentOrPreviousWebProcess(WebProcessProxy&, const String&);
    bool checkURLReceivedFromCurrentOrPreviousWebProcess(WebProcessProxy&, const URL&);
    void willAcquireUniversalFileReadSandboxExtension(WebProcessProxy&);

    void handleAutoFillButtonClick(const UserData&);

    void didResignInputElementStrongPasswordAppearance(const UserData&);

    void finishInitializingWebPageAfterProcessLaunch();

    void handleMessage(IPC::Connection&, const String& messageName, const UserData& messageBody);
    void handleSynchronousMessage(IPC::Connection&, const String& messageName, const UserData& messageBody, UserData& returnUserData);

    void viewIsBecomingVisible();

    void stopAllURLSchemeTasks();

    void clearInspectorTargets();
    void createInspectorTargets();

#if ENABLE(ATTACHMENT_ELEMENT)
    void registerAttachmentIdentifierFromData(const String&, const String& contentType, const String& preferredFileName, const IPC::DataReference&);
    void registerAttachmentIdentifierFromFilePath(const String&, const String& contentType, const String& filePath);
    void registerAttachmentsFromSerializedData(Vector<WebCore::SerializedAttachmentData>&&);
    void cloneAttachmentData(const String& fromIdentifier, const String& toIdentifier);

    void platformRegisterAttachment(Ref<API::Attachment>&&, const String& preferredFileName, const IPC::DataReference&);
    void platformRegisterAttachment(Ref<API::Attachment>&&, const String& filePath);
    void platformCloneAttachment(Ref<API::Attachment>&& fromAttachment, Ref<API::Attachment>&& toAttachment);

    void didInsertAttachmentWithIdentifier(const String& identifier, const String& source, bool hasEnclosingImage);
    void didRemoveAttachmentWithIdentifier(const String& identifier);
    void didRemoveAttachment(API::Attachment&);
    Ref<API::Attachment> ensureAttachment(const String& identifier);
    void invalidateAllAttachments();
#endif

    void reportPageLoadResult(const WebCore::ResourceError& = { });

    void continueNavigationInNewProcess(API::Navigation&, std::unique_ptr<SuspendedPageProxy>&&, Ref<WebProcessProxy>&&, ProcessSwapRequestedByClient, Optional<WebsitePoliciesData>&&);

    void setNeedsFontAttributes(bool);
    void updateFontAttributesAfterEditorStateChange();

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void logFrameNavigation(const WebFrameProxy&, const URL& pageURL, const WebCore::ResourceRequest&, const URL& redirectURL);
#endif

    WeakPtr<PageClient> m_pageClient;
    Ref<API::PageConfiguration> m_configuration;

    std::unique_ptr<API::LoaderClient> m_loaderClient;
    std::unique_ptr<API::PolicyClient> m_policyClient;
    UniqueRef<API::NavigationClient> m_navigationClient;
    UniqueRef<API::HistoryClient> m_historyClient;
    std::unique_ptr<API::IconLoadingClient> m_iconLoadingClient;
    std::unique_ptr<API::FormClient> m_formClient;
    std::unique_ptr<API::UIClient> m_uiClient;
    std::unique_ptr<API::FindClient> m_findClient;
    std::unique_ptr<API::FindMatchesClient> m_findMatchesClient;
    std::unique_ptr<API::DiagnosticLoggingClient> m_diagnosticLoggingClient;
#if ENABLE(CONTEXT_MENUS)
    std::unique_ptr<API::ContextMenuClient> m_contextMenuClient;
#endif
    std::unique_ptr<WebPageInjectedBundleClient> m_injectedBundleClient;

    std::unique_ptr<WebNavigationState> m_navigationState;
    String m_failingProvisionalLoadURL;
    bool m_isLoadingAlternateHTMLStringForFailingProvisionalLoad { false };

    std::unique_ptr<DrawingAreaProxy> m_drawingArea;
#if PLATFORM(COCOA)
    std::unique_ptr<RemoteLayerTreeHost> m_frozenRemoteLayerTreeHost;
#if ENABLE(ASYNC_SCROLLING)
    std::unique_ptr<RemoteScrollingCoordinatorProxy> m_scrollingCoordinatorProxy;
#endif
#endif
    Ref<WebProcessProxy> m_process;
    Ref<WebPageGroup> m_pageGroup;
    Ref<WebPreferences> m_preferences;

    WebProcessLifetimeTracker m_webProcessLifetimeTracker { *this };

    Ref<WebUserContentControllerProxy> m_userContentController;
    Ref<VisitedLinkStore> m_visitedLinkStore;
    Ref<WebsiteDataStore> m_websiteDataStore;

    RefPtr<WebFrameProxy> m_mainFrame;

    RefPtr<WebFrameProxy> m_focusedFrame;
    RefPtr<WebFrameProxy> m_frameSetLargestFrame;

    String m_userAgent;
    String m_applicationNameForUserAgent;
    String m_customUserAgent;
    String m_customTextEncodingName;
    String m_overrideContentSecurityPolicy;

    bool m_treatsSHA1CertificatesAsInsecure { true };

    RefPtr<WebInspectorProxy> m_inspector;

#if ENABLE(FULLSCREEN_API)
    std::unique_ptr<WebFullScreenManagerProxy> m_fullScreenManager;
    std::unique_ptr<API::FullscreenClient> m_fullscreenClient;
#endif

#if (PLATFORM(IOS_FAMILY) && HAVE(AVKIT)) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))
    RefPtr<PlaybackSessionManagerProxy> m_playbackSessionManager;
    RefPtr<VideoFullscreenManagerProxy> m_videoFullscreenManager;
#endif

#if PLATFORM(IOS_FAMILY)
    VisibleContentRectUpdateInfo m_lastVisibleContentRectUpdate;
    uint64_t m_firstLayerTreeTransactionIdAfterDidCommitLoad { 0 };
    int32_t m_deviceOrientation { 0 };
    bool m_hasReceivedLayerTreeTransactionAfterDidCommitLoad { true };
    bool m_hasNetworkRequestsOnSuspended { false };
    bool m_isKeyboardAnimatingIn { false };
    bool m_isScrollingOrZooming { false };
#endif
        
#if PLATFORM(MAC)
    bool m_useSystemAppearance { false };
#endif

#if ENABLE(APPLE_PAY)
    std::unique_ptr<WebPaymentCoordinatorProxy> m_paymentCoordinator;
#endif

#if USE(SYSTEM_PREVIEW)
    std::unique_ptr<SystemPreviewController> m_systemPreviewController;
#endif

#if ENABLE(WEB_AUTHN)
    std::unique_ptr<WebAuthenticatorCoordinatorProxy> m_credentialsMessenger;
#endif

    CallbackMap m_callbacks;
    HashSet<CallbackID> m_loadDependentStringCallbackIDs;

    HashSet<WebEditCommandProxy*> m_editCommandSet;

#if PLATFORM(COCOA)
    HashSet<String> m_knownKeypressCommandNames;
#endif

    RefPtr<WebPopupMenuProxy> m_activePopupMenu;
#if ENABLE(CONTEXT_MENUS)
    RefPtr<WebContextMenuProxy> m_activeContextMenu;
    ContextMenuContextData m_activeContextMenuContextData;
#endif
    RefPtr<API::HitTestResult> m_lastMouseMoveHitTestResult;

    RefPtr<WebOpenPanelResultListenerProxy> m_openPanelResultListener;
    GeolocationPermissionRequestManagerProxy m_geolocationPermissionRequestManager;
    NotificationPermissionRequestManagerProxy m_notificationPermissionRequestManager;

#if ENABLE(MEDIA_STREAM)
    std::unique_ptr<UserMediaPermissionRequestManagerProxy> m_userMediaPermissionRequestManager;
#endif

    OptionSet<WebCore::ActivityState::Flag> m_activityState;
    bool m_viewWasEverInWindow { false };
#if PLATFORM(IOS_FAMILY)
    bool m_allowsMediaDocumentInlinePlayback { false };
    bool m_alwaysRunsAtForegroundPriority { false };
    ProcessThrottler::ForegroundActivityToken m_activityToken;
#endif
    bool m_initialCapitalizationEnabled { false };
    Optional<double> m_cpuLimit;
    Ref<WebBackForwardList> m_backForwardList;
        
    bool m_maintainsInactiveSelection { false };

    bool m_waitsForPaintAfterViewDidMoveToWindow { false };
    bool m_shouldSkipWaitingForPaintAfterNextViewDidMoveToWindow { false };

    String m_toolTip;

    EditorState m_editorState;
    bool m_isEditable { false };

#if PLATFORM(COCOA)
    TouchBarMenuData m_touchBarMenuData;
#endif

    double m_textZoomFactor { 1 };
    double m_pageZoomFactor { 1 };
    double m_pageScaleFactor { 1 };
    double m_pluginZoomFactor { 1 };
    double m_pluginScaleFactor { 1 };
    double m_viewScaleFactor { 1 };
    float m_intrinsicDeviceScaleFactor { 1 };
    Optional<float> m_customDeviceScaleFactor;
    float m_topContentInset { 0 };

    LayerHostingMode m_layerHostingMode { LayerHostingMode::InProcess };

    WebCore::Color m_underlayColor;
    WebCore::Color m_pageExtendedBackgroundColor;

    bool m_useFixedLayout { false };
    WebCore::IntSize m_fixedLayoutSize;

    bool m_alwaysShowsHorizontalScroller { false };
    bool m_alwaysShowsVerticalScroller { false };

    OptionSet<WebCore::LayoutMilestone> m_observedLayoutMilestones;

    bool m_suppressScrollbarAnimations { false };

    WebCore::Pagination::Mode m_paginationMode { WebCore::Pagination::Unpaginated };
    bool m_paginationBehavesLikeColumns { false };
    double m_pageLength { 0 };
    double m_gapBetweenPages { 0 };
    bool m_paginationLineGridEnabled { false };
        
    // If the process backing the web page is alive and kicking.
    bool m_isValid { true };

    // Whether WebPageProxy::close() has been called on this page.
    bool m_isClosed { false };

    // Whether it can run modal child web pages.
    bool m_canRunModal { false };

    bool m_needsToFinishInitializingWebPageAfterProcessLaunch { false };

    bool m_isInPrintingMode { false };
    bool m_isPerformingDOMPrintOperation { false };

    WebCore::ResourceRequest m_decidePolicyForResponseRequest;
    bool m_shouldSuppressAppLinksInNextNavigationPolicyDecision { false };

    Deque<NativeWebMouseEvent> m_mouseEventQueue;
    Deque<NativeWebKeyboardEvent> m_keyEventQueue;
    Deque<NativeWebWheelEvent> m_wheelEventQueue;
    Deque<std::unique_ptr<Vector<NativeWebWheelEvent>>> m_currentlyProcessedWheelEvents;
#if ENABLE(MAC_GESTURE_EVENTS)
    Deque<NativeWebGestureEvent> m_gestureEventQueue;
#endif

#if ENABLE(TOUCH_EVENTS)
    struct TouchEventTracking {
        WebCore::TrackingType touchForceChangedTracking { WebCore::TrackingType::NotTracking };
        WebCore::TrackingType touchStartTracking { WebCore::TrackingType::NotTracking };
        WebCore::TrackingType touchMoveTracking { WebCore::TrackingType::NotTracking };
        WebCore::TrackingType touchEndTracking { WebCore::TrackingType::NotTracking };

        bool isTrackingAnything() const
        {
            return touchForceChangedTracking != WebCore::TrackingType::NotTracking
                || touchStartTracking != WebCore::TrackingType::NotTracking
                || touchMoveTracking != WebCore::TrackingType::NotTracking
                || touchEndTracking != WebCore::TrackingType::NotTracking;
        }

        void reset()
        {
            touchForceChangedTracking = WebCore::TrackingType::NotTracking;
            touchStartTracking = WebCore::TrackingType::NotTracking;
            touchMoveTracking = WebCore::TrackingType::NotTracking;
            touchEndTracking = WebCore::TrackingType::NotTracking;
        }
    };
    TouchEventTracking m_touchAndPointerEventTracking;
#endif
#if ENABLE(TOUCH_EVENTS) && !ENABLE(IOS_TOUCH_EVENTS)
    Deque<QueuedTouchEvents> m_touchEventQueue;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    RefPtr<WebColorPicker> m_colorPicker;
#endif
#if ENABLE(DATALIST_ELEMENT)
    RefPtr<WebDataListSuggestionsDropdown> m_dataListSuggestionsDropdown;
#endif
#if PLATFORM(COCOA)
    RefPtr<WebCore::ValidationBubble> m_validationBubble;
#endif

    const uint64_t m_pageID;

    bool m_isPageSuspended { false };
    bool m_addsVisitedLinks { true };

    bool m_controlledByAutomation { false };

    unsigned m_inspectorFrontendCount { 0 };

#if PLATFORM(COCOA)
    bool m_isSmartInsertDeleteEnabled { false };
#endif

    Optional<WebCore::Color> m_backgroundColor;

    unsigned m_pendingLearnOrIgnoreWordMessageCount { 0 };

    bool m_mainFrameHasCustomContentProvider { false };

#if ENABLE(DRAG_SUPPORT)
    // Current drag destination details are delivered as an asynchronous response,
    // so we preserve them to be used when the next dragging delegate call is made.
    WebCore::DragOperation m_currentDragOperation { WebCore::DragOperationNone };
    WebCore::DragHandlingMethod m_currentDragHandlingMethod { WebCore::DragHandlingMethod::None };
    bool m_currentDragIsOverFileInput { false };
    unsigned m_currentDragNumberOfFilesToBeAccepted { 0 };
    WebCore::IntRect m_currentDragCaretRect;
    WebCore::IntRect m_currentDragCaretEditableElementRect;
#endif

    PageLoadState m_pageLoadState;
    
    bool m_delegatesScrolling { false };

    bool m_mainFrameHasHorizontalScrollbar { false };
    bool m_mainFrameHasVerticalScrollbar { false };

    // Whether horizontal wheel events can be handled directly for swiping purposes.
    bool m_canShortCircuitHorizontalWheelEvents { true };

    bool m_mainFrameIsPinnedToLeftSide { true };
    bool m_mainFrameIsPinnedToRightSide { true };
    bool m_mainFrameIsPinnedToTopSide { true };
    bool m_mainFrameIsPinnedToBottomSide { true };

    bool m_shouldUseImplicitRubberBandControl { false };
    bool m_rubberBandsAtLeft { true };
    bool m_rubberBandsAtRight { true };
    bool m_rubberBandsAtTop { true };
    bool m_rubberBandsAtBottom { true };
        
    bool m_enableVerticalRubberBanding { true };
    bool m_enableHorizontalRubberBanding { true };

    bool m_backgroundExtendsBeyondPage { true };

    bool m_shouldRecordNavigationSnapshots { false };
    bool m_isShowingNavigationGestureSnapshot { false };

    bool m_mainFramePluginHandlesPageScaleGesture { false };

    unsigned m_pageCount { 0 };

    WebCore::IntRect m_visibleScrollerThumbRect;

    uint64_t m_renderTreeSize { 0 };
    uint64_t m_sessionRestorationRenderTreeSize { 0 };
    bool m_hitRenderTreeSizeThreshold { false };

    bool m_suppressVisibilityUpdates { false };
    bool m_autoSizingShouldExpandToViewHeight { false };
    WebCore::IntSize m_viewLayoutSize;
    Optional<WebCore::IntSize> m_viewportSizeForCSSViewportUnits;

    // Visual viewports
    WebCore::LayoutSize m_baseLayoutViewportSize;
    WebCore::LayoutPoint m_minStableLayoutViewportOrigin;
    WebCore::LayoutPoint m_maxStableLayoutViewportOrigin;

    float m_mediaVolume { 1 };
    WebCore::MediaProducer::MutedStateFlags m_mutedState { WebCore::MediaProducer::NoneMuted };
    bool m_mayStartMediaWhenInWindow { true };
    bool m_mediaCaptureEnabled { true };

    bool m_waitingForDidUpdateActivityState { false };

    bool m_shouldScaleViewToFitDocument { false };
    bool m_shouldSuppressNextAutomaticNavigationSnapshot { false };

#if PLATFORM(COCOA)
    HashMap<String, String> m_temporaryPDFFiles;
    std::unique_ptr<WebCore::RunLoopObserver> m_activityStateChangeDispatcher;

    std::unique_ptr<RemoteLayerTreeScrollingPerformanceData> m_scrollingPerformanceData;
    bool m_scrollPerformanceDataCollectionEnabled { false };
#endif
    UserObservablePageCounter::Token m_pageIsUserObservableCount;
    ProcessSuppressionDisabledToken m_preventProcessSuppressionCount;
    HiddenPageThrottlingAutoIncreasesCounter::Token m_hiddenPageDOMTimerThrottlingAutoIncreasesCount;
    VisibleWebPageToken m_visiblePageToken;
        
    WebCore::ScrollPinningBehavior m_scrollPinningBehavior { WebCore::DoNotPin };
    Optional<WebCore::ScrollbarOverlayStyle> m_scrollbarOverlayStyle;

    uint64_t m_navigationID { 0 };
    ActivityStateChangeID m_currentActivityStateChangeID { ActivityStateChangeAsynchronous };

    WebPreferencesStore::ValueMap m_configurationPreferenceValues;
    OptionSet<WebCore::ActivityState::Flag> m_potentiallyChangedActivityStateFlags;
    bool m_activityStateChangeWantsSynchronousReply { false };
    Vector<CallbackID> m_nextActivityStateChangeCallbacks;

    WebCore::MediaProducer::MediaStateFlags m_mediaState { WebCore::MediaProducer::IsNotPlaying };

    bool m_isResourceCachingDisabled { false };

    bool m_hasHadSelectionChangesFromUserInteraction { false };
    bool m_needsHiddenContentEditableQuirk { false };
    bool m_needsPlainTextQuirk { false };

#if ENABLE(MEDIA_SESSION)
    bool m_hasMediaSessionWithActiveMediaElements { false };
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    bool m_requiresTargetMonitoring { false };
#endif

#if PLATFORM(IOS_FAMILY)
    std::unique_ptr<ElementDidFocusArguments> m_deferredElementDidFocusArguments;
    bool m_waitingForPostLayoutEditorStateUpdateAfterFocusingElement { false };
    bool m_forceAlwaysUserScalable { false };
    WebCore::FloatSize m_viewportConfigurationViewLayoutSize;
    double m_viewportConfigurationLayoutSizeScaleFactor { 1 };
    WebCore::FloatSize m_maximumUnobscuredSize;
#endif

    Optional<WebCore::FontAttributes> m_cachedFontAttributesAtSelectionStart;

#if ENABLE(POINTER_LOCK)
    bool m_isPointerLockPending { false };
    bool m_isPointerLocked { false };
#endif

    bool m_isUsingHighPerformanceWebGL { false };
    bool m_openedByDOM { false };
    bool m_hasCommittedAnyProvisionalLoads { false };

    HashMap<String, Ref<WebURLSchemeHandler>> m_urlSchemeHandlersByScheme;
    HashMap<uint64_t, Ref<WebURLSchemeHandler>> m_urlSchemeHandlersByIdentifier;

#if ENABLE(ATTACHMENT_ELEMENT)
    HashMap<String, Ref<API::Attachment>> m_attachmentIdentifierToAttachmentMap;
#endif

    const std::unique_ptr<WebPageInspectorController> m_inspectorController;
#if ENABLE(REMOTE_INSPECTOR)
    std::unique_ptr<WebPageDebuggable> m_inspectorDebuggable;
#endif

    Optional<SpellDocumentTag> m_spellDocumentTag;

    Optional<MonotonicTime> m_pageLoadStart;
    HashSet<String> m_previouslyVisitedPaths;

    RunLoop::Timer<WebPageProxy> m_resetRecentCrashCountTimer;
    unsigned m_recentCrashCount { 0 };

    bool m_needsFontAttributes { false };
    bool m_mayHaveUniversalFileReadSandboxExtension { false };

    std::unique_ptr<ProvisionalPageProxy> m_provisionalPage;

#if HAVE(PENCILKIT)
    std::unique_ptr<EditableImageController> m_editableImageController;
#endif
};

} // namespace WebKit
