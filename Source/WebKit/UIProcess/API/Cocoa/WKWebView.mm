/*
 * Copyright (C) 2014-2018 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WKWebViewInternal.h"

#if WK_API_ENABLED

#import "APIFormClient.h"
#import "APIPageConfiguration.h"
#import "APISerializedScriptValue.h"
#import "CompletionHandlerCallChecker.h"
#import "DiagnosticLoggingClient.h"
#import "FindClient.h"
#import "FullscreenClient.h"
#import "IconLoadingDelegate.h"
#import "LegacySessionStateCoding.h"
#import "Logging.h"
#import "NavigationState.h"
#import "ObjCObjectGraph.h"
#import "RemoteLayerTreeScrollingPerformanceData.h"
#import "RemoteLayerTreeTransaction.h"
#import "RemoteObjectRegistry.h"
#import "RemoteObjectRegistryMessages.h"
#import "UIDelegate.h"
#import "VersionChecks.h"
#import "ViewGestureController.h"
#import "ViewSnapshotStore.h"
#import "WKBackForwardListInternal.h"
#import "WKBackForwardListItemInternal.h"
#import "WKBrowsingContextHandleInternal.h"
#import "WKDragDestinationAction.h"
#import "WKErrorInternal.h"
#import "WKHistoryDelegatePrivate.h"
#import "WKLayoutMode.h"
#import "WKNSData.h"
#import "WKNSURLExtras.h"
#import "WKNavigationDelegate.h"
#import "WKNavigationInternal.h"
#import "WKPreferencesInternal.h"
#import "WKProcessPoolInternal.h"
#import "WKSharedAPICast.h"
#import "WKSnapshotConfiguration.h"
#import "WKUIDelegate.h"
#import "WKUIDelegatePrivate.h"
#import "WKUserContentControllerInternal.h"
#import "WKWebViewConfigurationInternal.h"
#import "WKWebViewContentProvider.h"
#import "WKWebsiteDataStoreInternal.h"
#import "WebBackForwardList.h"
#import "WebCertificateInfo.h"
#import "WebFullScreenManagerProxy.h"
#import "WebPageGroup.h"
#import "WebPageProxy.h"
#import "WebPreferencesKeys.h"
#import "WebProcessPool.h"
#import "WebProcessProxy.h"
#import "WebURLSchemeHandlerCocoa.h"
#import "WebViewImpl.h"
#import "_WKActivatedElementInfoInternal.h"
#import "_WKDiagnosticLoggingDelegate.h"
#import "_WKFindDelegate.h"
#import "_WKFrameHandleInternal.h"
#import "_WKFullscreenDelegate.h"
#import "_WKHitTestResultInternal.h"
#import "_WKInputDelegate.h"
#import "_WKRemoteObjectRegistryInternal.h"
#import "_WKSessionStateInternal.h"
#import "_WKVisitedLinkStoreInternal.h"
#import "_WKWebsitePoliciesInternal.h"
#import <WebCore/AttachmentTypes.h>
#import <WebCore/GraphicsContextCG.h>
#import <WebCore/IOSurface.h>
#import <WebCore/JSDOMBinding.h>
#import <WebCore/JSDOMExceptionHandling.h>
#import <WebCore/PlatformScreen.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/SQLiteDatabaseTracker.h>
#import <WebCore/SchemeRegistry.h>
#import <WebCore/Settings.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/ValidationBubble.h>
#import <WebCore/ViewportArguments.h>
#import <WebCore/WritingMode.h>
#import <pal/spi/cocoa/NSKeyedArchiverSPI.h>
#import <pal/spi/mac/NSTextFinderSPI.h>
#import <wtf/BlockPtr.h>
#import <wtf/HashMap.h>
#import <wtf/MathExtras.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/Optional.h>
#import <wtf/RetainPtr.h>
#import <wtf/SetForScope.h>
#import <wtf/UUID.h>
#import <wtf/spi/darwin/dyldSPI.h>
#import <wtf/text/TextStream.h>

#if ENABLE(APPLICATION_MANIFEST)
#import "_WKApplicationManifestInternal.h"
#endif

#if ENABLE(DATA_DETECTION)
#import "WKDataDetectorTypesInternal.h"
#endif

#if PLATFORM(IOS)
#import "InteractionInformationAtPosition.h"
#import "InteractionInformationRequest.h"
#import "ProcessThrottler.h"
#import "RemoteLayerTreeDrawingAreaProxy.h"
#import "RemoteScrollingCoordinatorProxy.h"
#import "UIKitSPI.h"
#import "VideoFullscreenManagerProxy.h"
#import "WKContentViewInteraction.h"
#import "WKPDFView.h"
#import "WKPasswordView.h"
#import "WKScrollView.h"
#import "WKWebViewContentProviderRegistry.h"
#import "_WKWebViewPrintFormatter.h"
#import <UIKit/UIApplication.h>
#import <WebCore/FrameLoaderTypes.h>
#import <WebCore/InspectorOverlay.h>
#import <WebCore/ScrollableArea.h>
#import <WebCore/WebBackgroundTaskController.h>
#import <WebCore/WebSQLiteDatabaseTrackerClient.h>
#import <pal/spi/cg/CoreGraphicsSPI.h>
#import <pal/spi/cocoa/QuartzCoreSPI.h>

#if __has_include(<AccessibilitySupport.h>)
#include <AccessibilitySupport.h>
#else
extern "C" {
CFStringRef kAXSAllowForceWebScalingEnabledNotification;
}
#endif

#define RELEASE_LOG_IF_ALLOWED(...) RELEASE_LOG_IF(_page && _page->isAlwaysOnLoggingAllowed(), ViewState, __VA_ARGS__)

@interface UIScrollView (UIScrollViewInternal)
- (void)_adjustForAutomaticKeyboardInfo:(NSDictionary*)info animated:(BOOL)animated lastAdjustment:(CGFloat*)lastAdjustment;
- (BOOL)_isScrollingToTop;
- (CGPoint)_animatedTargetOffset;
@end

@interface UIPeripheralHost(UIKitInternal)
- (CGFloat)getVerticalOverlapForView:(UIView *)view usingKeyboardInfo:(NSDictionary *)info;
@end

@interface UIView (UIViewInternal)
- (UIViewController *)_viewControllerForAncestor;
@end

@interface UIWindow (UIWindowInternal)
- (BOOL)_isHostedInAnotherProcess;
@end

@interface UIViewController (UIViewControllerInternal)
- (UIViewController *)_rootAncestorViewController;
- (UIViewController *)_viewControllerForSupportedInterfaceOrientations;
@end

enum class DynamicViewportUpdateMode {
    NotResizing,
    ResizingWithAnimation,
    ResizingWithDocumentHidden,
};

#endif // PLATFORM(IOS)

#if PLATFORM(IOS)
static const uint32_t firstSDKVersionWithLinkPreviewEnabledByDefault = 0xA0000;
#endif

#if PLATFORM(MAC)
#import "WKTextFinderClient.h"
#import "WKViewInternal.h"
#import <WebCore/ColorMac.h>

@interface WKWebView () <WebViewImplDelegate, NSTextInputClient>
@end

#if HAVE(TOUCH_BAR)
@interface WKWebView () <NSTouchBarProvider>
@end
#endif // HAVE(TOUCH_BAR)
#endif // PLATFORM(MAC)

static HashMap<WebKit::WebPageProxy*, WKWebView *>& pageToViewMap()
{
    static NeverDestroyed<HashMap<WebKit::WebPageProxy*, WKWebView *>> map;
    return map;
}

WKWebView* fromWebPageProxy(WebKit::WebPageProxy& page)
{
    return pageToViewMap().get(&page);
}

#if PLATFORM(MAC)
static _WKOverlayScrollbarStyle toAPIScrollbarStyle(std::optional<WebCore::ScrollbarOverlayStyle> coreScrollbarStyle)
{
    if (!coreScrollbarStyle)
        return _WKOverlayScrollbarStyleAutomatic;
    
    switch (coreScrollbarStyle.value()) {
    case WebCore::ScrollbarOverlayStyleDark:
        return _WKOverlayScrollbarStyleDark;
    case WebCore::ScrollbarOverlayStyleLight:
        return _WKOverlayScrollbarStyleLight;
    case WebCore::ScrollbarOverlayStyleDefault:
        return _WKOverlayScrollbarStyleDefault;
    }
    ASSERT_NOT_REACHED();
    return _WKOverlayScrollbarStyleAutomatic;
}

static std::optional<WebCore::ScrollbarOverlayStyle> toCoreScrollbarStyle(_WKOverlayScrollbarStyle scrollbarStyle)
{
    switch (scrollbarStyle) {
    case _WKOverlayScrollbarStyleDark:
        return WebCore::ScrollbarOverlayStyleDark;
    case _WKOverlayScrollbarStyleLight:
        return WebCore::ScrollbarOverlayStyleLight;
    case _WKOverlayScrollbarStyleDefault:
        return WebCore::ScrollbarOverlayStyleDefault;
    case _WKOverlayScrollbarStyleAutomatic:
        break;
    }
    return std::nullopt;
}
#endif

@implementation WKWebView {
    std::unique_ptr<WebKit::NavigationState> _navigationState;
    std::unique_ptr<WebKit::UIDelegate> _uiDelegate;
    std::unique_ptr<WebKit::IconLoadingDelegate> _iconLoadingDelegate;

    _WKRenderingProgressEvents _observedRenderingProgressEvents;

    WebKit::WeakObjCPtr<id <_WKInputDelegate>> _inputDelegate;

#if PLATFORM(IOS)
    RetainPtr<_WKRemoteObjectRegistry> _remoteObjectRegistry;

    RetainPtr<WKScrollView> _scrollView;
    RetainPtr<WKContentView> _contentView;

#if ENABLE(FULLSCREEN_API)
    RetainPtr<WKFullScreenWindowController> _fullScreenWindowController;
#endif

    BOOL _overridesMinimumLayoutSize;
    CGSize _minimumLayoutSizeOverride;
    std::optional<WebCore::FloatSize> _lastSentMinimumLayoutSize;
    BOOL _overridesMaximumUnobscuredSize;
    CGSize _maximumUnobscuredSizeOverride;
    std::optional<WebCore::FloatSize> _lastSentMaximumUnobscuredSize;
    CGRect _inputViewBounds;
    CGFloat _viewportMetaTagWidth;
    BOOL _viewportMetaTagWidthWasExplicit;
    BOOL _viewportMetaTagCameFromImageDocument;
    CGFloat _initialScaleFactor;
    BOOL _fastClickingIsDisabled;

    BOOL _allowsLinkPreview;

    UIEdgeInsets _obscuredInsets;
    BOOL _haveSetObscuredInsets;
    BOOL _isChangingObscuredInsetsInteractively;

    UIEdgeInsets _unobscuredSafeAreaInsets;
    BOOL _haveSetUnobscuredSafeAreaInsets;
    BOOL _avoidsUnsafeArea;
    UIRectEdge _obscuredInsetEdgesAffectedBySafeArea;

    UIInterfaceOrientation _interfaceOrientationOverride;
    BOOL _overridesInterfaceOrientation;
    std::optional<int32_t> _lastSentDeviceOrientation;

    BOOL _allowsViewportShrinkToFit;

    BOOL _hasCommittedLoadForMainFrame;
    BOOL _needsResetViewStateAfterCommitLoadForMainFrame;
    uint64_t _firstPaintAfterCommitLoadTransactionID;
    DynamicViewportUpdateMode _dynamicViewportUpdateMode;
    CATransform3D _resizeAnimationTransformAdjustments;
    std::optional<uint64_t> _resizeAnimationTransformTransactionID;
    RetainPtr<UIView> _resizeAnimationView;
    CGFloat _lastAdjustmentForScroller;
    std::optional<CGRect> _frozenVisibleContentRect;
    std::optional<CGRect> _frozenUnobscuredContentRect;

    BOOL _commitDidRestoreScrollPosition;
    std::optional<WebCore::FloatPoint> _scrollOffsetToRestore;
    WebCore::FloatBoxExtent _obscuredInsetsWhenSaved;

    std::optional<WebCore::FloatPoint> _unobscuredCenterToRestore;
    std::optional<uint64_t> _firstTransactionIDAfterPageRestore;
    double _scaleToRestore;

    std::unique_ptr<WebKit::ViewGestureController> _gestureController;
    BOOL _allowsBackForwardNavigationGestures;

    RetainPtr<UIView <WKWebViewContentProvider>> _customContentView;
    RetainPtr<UIView> _customContentFixedOverlayView;

    RetainPtr<NSTimer> _enclosingScrollViewScrollTimer;
    BOOL _didScrollSinceLastTimerFire;

    WebCore::Color _scrollViewBackgroundColor;

    // This value tracks the current adjustment added to the bottom inset due to the keyboard sliding out from the bottom
    // when computing obscured content insets. This is used when updating the visible content rects where we should not
    // include this adjustment.
    CGFloat _totalScrollViewBottomInsetAdjustmentForKeyboard;
    BOOL _currentlyAdjustingScrollViewInsetsForKeyboard;

    BOOL _delayUpdateVisibleContentRects;
    BOOL _hadDelayedUpdateVisibleContentRects;

    int _activeAnimatedResizeCount;

    Vector<WTF::Function<void ()>> _snapshotsDeferredDuringResize;
    RetainPtr<NSMutableArray> _stableStatePresentationUpdateCallbacks;

    RetainPtr<WKPasswordView> _passwordView;

    BOOL _hasScheduledVisibleRectUpdate;
    BOOL _visibleContentRectUpdateScheduledFromScrollViewInStableState;
    Vector<BlockPtr<void ()>> _visibleContentRectUpdateCallbacks;
#endif
#if PLATFORM(MAC)
    std::unique_ptr<WebKit::WebViewImpl> _impl;
    RetainPtr<WKTextFinderClient> _textFinderClient;
#endif

#if PLATFORM(IOS)
    _WKDragInteractionPolicy _dragInteractionPolicy;
#endif
}

- (instancetype)initWithFrame:(CGRect)frame
{
    return [self initWithFrame:frame configuration:adoptNS([[WKWebViewConfiguration alloc] init]).get()];
}

- (BOOL)_isValid
{
    return _page && _page->isValid();
}

#if PLATFORM(IOS)
static int32_t deviceOrientationForUIInterfaceOrientation(UIInterfaceOrientation orientation)
{
    switch (orientation) {
    case UIInterfaceOrientationUnknown:
    case UIInterfaceOrientationPortrait:
        return 0;
    case UIInterfaceOrientationPortraitUpsideDown:
        return 180;
    case UIInterfaceOrientationLandscapeLeft:
        return -90;
    case UIInterfaceOrientationLandscapeRight:
        return 90;
    }
}

static int32_t deviceOrientation()
{
    return deviceOrientationForUIInterfaceOrientation([[UIApplication sharedApplication] statusBarOrientation]);
}

- (BOOL)_isShowingVideoPictureInPicture
{
#if !HAVE(AVKIT)
    return false;
#else
    if (!_page || !_page->videoFullscreenManager())
        return false;

    return _page->videoFullscreenManager()->hasMode(WebCore::HTMLMediaElementEnums::VideoFullscreenModePictureInPicture);
#endif
}

- (BOOL)_mayAutomaticallyShowVideoPictureInPicture
{
#if !HAVE(AVKIT)
    return false;
#else
    if (!_page || !_page->videoFullscreenManager())
        return false;

    return _page->videoFullscreenManager()->mayAutomaticallyShowVideoPictureInPicture();
#endif
}

static bool shouldAllowPictureInPictureMediaPlayback()
{
    static bool shouldAllowPictureInPictureMediaPlayback = dyld_get_program_sdk_version() >= DYLD_IOS_VERSION_9_0;
    return shouldAllowPictureInPictureMediaPlayback;
}

static bool shouldAllowSettingAnyXHRHeaderFromFileURLs()
{
    static bool shouldAllowSettingAnyXHRHeaderFromFileURLs = WebCore::IOSApplication::isCardiogram() && !linkedOnOrAfter(WebKit::SDKVersion::FirstThatDisallowsSettingAnyXHRHeaderFromFileURLs);
    return shouldAllowSettingAnyXHRHeaderFromFileURLs;
}

#endif

static bool shouldRequireUserGestureToLoadVideo()
{
#if PLATFORM(IOS)
    static bool shouldRequireUserGestureToLoadVideo = dyld_get_program_sdk_version() >= DYLD_IOS_VERSION_10_0;
    return shouldRequireUserGestureToLoadVideo;
#else
    return false;
#endif
}

#if PLATFORM(MAC)
static uint32_t convertUserInterfaceDirectionPolicy(WKUserInterfaceDirectionPolicy policy)
{
    switch (policy) {
    case WKUserInterfaceDirectionPolicyContent:
        return static_cast<uint32_t>(WebCore::UserInterfaceDirectionPolicy::Content);
    case WKUserInterfaceDirectionPolicySystem:
        return static_cast<uint32_t>(WebCore::UserInterfaceDirectionPolicy::System);
    }
    return static_cast<uint32_t>(WebCore::UserInterfaceDirectionPolicy::Content);
}

static uint32_t convertSystemLayoutDirection(NSUserInterfaceLayoutDirection direction)
{
    switch (direction) {
    case NSUserInterfaceLayoutDirectionLeftToRight:
        return static_cast<uint32_t>(WebCore::UserInterfaceLayoutDirection::LTR);
    case NSUserInterfaceLayoutDirectionRightToLeft:
        return static_cast<uint32_t>(WebCore::UserInterfaceLayoutDirection::RTL);
    }
    return static_cast<uint32_t>(WebCore::UserInterfaceLayoutDirection::LTR);
}
#endif

static void validate(WKWebViewConfiguration *configuration)
{
    if (!configuration.processPool)
        [NSException raise:NSInvalidArgumentException format:@"configuration.processPool is nil"];
    
    if (!configuration.preferences)
        [NSException raise:NSInvalidArgumentException format:@"configuration.preferences is nil"];
    
    if (!configuration.userContentController)
        [NSException raise:NSInvalidArgumentException format:@"configuration.userContentController is nil"];
    
    if (!configuration.websiteDataStore)
        [NSException raise:NSInvalidArgumentException format:@"configuration.websiteDataStore is nil"];
    
    if (!configuration._visitedLinkStore)
        [NSException raise:NSInvalidArgumentException format:@"configuration._visitedLinkStore is nil"];
    
#if PLATFORM(IOS)
    if (!configuration._contentProviderRegistry)
        [NSException raise:NSInvalidArgumentException format:@"configuration._contentProviderRegistry is nil"];
#endif
}

- (void)_initializeWithConfiguration:(WKWebViewConfiguration *)configuration
{
    if (!configuration)
        [NSException raise:NSInvalidArgumentException format:@"Configuration cannot be nil"];

    _configuration = adoptNS([configuration copy]);

    if (WKWebView *relatedWebView = [_configuration _relatedWebView]) {
        WKProcessPool *processPool = [_configuration processPool];
        WKProcessPool *relatedWebViewProcessPool = [relatedWebView->_configuration processPool];
        if (processPool && processPool != relatedWebViewProcessPool)
            [NSException raise:NSInvalidArgumentException format:@"Related web view %@ has process pool %@ but configuration specifies a different process pool %@", relatedWebView, relatedWebViewProcessPool, configuration.processPool];

        [_configuration setProcessPool:relatedWebViewProcessPool];
    }

    validate(_configuration.get());

    WebKit::WebProcessPool& processPool = *[_configuration processPool]->_processPool;
    processPool.setResourceLoadStatisticsEnabled(configuration.websiteDataStore._resourceLoadStatisticsEnabled);

    auto pageConfiguration = API::PageConfiguration::create();

    pageConfiguration->setProcessPool(&processPool);
    pageConfiguration->setPreferences([_configuration preferences]->_preferences.get());
    if (WKWebView *relatedWebView = [_configuration _relatedWebView])
        pageConfiguration->setRelatedPage(relatedWebView->_page.get());

    pageConfiguration->setUserContentController([_configuration userContentController]->_userContentControllerProxy.get());
    pageConfiguration->setVisitedLinkStore([_configuration _visitedLinkStore]->_visitedLinkStore.get());
    pageConfiguration->setWebsiteDataStore([_configuration websiteDataStore]->_websiteDataStore.get());
    pageConfiguration->setTreatsSHA1SignedCertificatesAsInsecure([_configuration _treatsSHA1SignedCertificatesAsInsecure]);

    if (NSString *overrideContentSecurityPolicy = configuration._overrideContentSecurityPolicy)
        pageConfiguration->setOverrideContentSecurityPolicy(overrideContentSecurityPolicy);

#if PLATFORM(MAC)
    if (auto pageGroup = WebKit::toImpl([configuration _pageGroup])) {
        pageConfiguration->setPageGroup(pageGroup);
        pageConfiguration->setUserContentController(&pageGroup->userContentController());
    } else
#endif
    {
        NSString *groupIdentifier = configuration._groupIdentifier;
        if (groupIdentifier.length)
            pageConfiguration->setPageGroup(WebKit::WebPageGroup::create(configuration._groupIdentifier).ptr());
    }

    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::suppressesIncrementalRenderingKey(), WebKit::WebPreferencesStore::Value(!![_configuration suppressesIncrementalRendering]));

    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::shouldRespectImageOrientationKey(), WebKit::WebPreferencesStore::Value(!![_configuration _respectsImageOrientation]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::shouldPrintBackgroundsKey(), WebKit::WebPreferencesStore::Value(!![_configuration _printsBackgrounds]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::incrementalRenderingSuppressionTimeoutKey(), WebKit::WebPreferencesStore::Value([_configuration _incrementalRenderingSuppressionTimeout]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::javaScriptMarkupEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _allowsJavaScriptMarkup]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::shouldConvertPositionStyleOnCopyKey(), WebKit::WebPreferencesStore::Value(!![_configuration _convertsPositionStyleOnCopy]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::httpEquivEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _allowsMetaRefresh]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::allowUniversalAccessFromFileURLsKey(), WebKit::WebPreferencesStore::Value(!![_configuration _allowUniversalAccessFromFileURLs]));
    pageConfiguration->setInitialCapitalizationEnabled([_configuration _initialCapitalizationEnabled]);
    pageConfiguration->setWaitsForPaintAfterViewDidMoveToWindow([_configuration _waitsForPaintAfterViewDidMoveToWindow]);
    pageConfiguration->setControlledByAutomation([_configuration _isControlledByAutomation]);

#if ENABLE(APPLICATION_MANIFEST)
    pageConfiguration->setApplicationManifest([_configuration _applicationManifest] ? [configuration _applicationManifest]->_applicationManifest.get() : nullptr);
#endif

#if PLATFORM(MAC)
    if (auto cpuLimit = [_configuration _cpuLimit])
        pageConfiguration->setCPULimit(cpuLimit);
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::showsURLsInToolTipsEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _showsURLsInToolTips]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::serviceControlsEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _serviceControlsEnabled]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::imageControlsEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _imageControlsEnabled]));

    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::userInterfaceDirectionPolicyKey(), WebKit::WebPreferencesStore::Value(convertUserInterfaceDirectionPolicy([_configuration userInterfaceDirectionPolicy])));
    // We are in the View's initialization routine, so our client hasn't had time to set our user interface direction.
    // Therefore, according to the docs[1], "this property contains the value reported by the app's userInterfaceLayoutDirection property."
    // [1] http://developer.apple.com/library/mac/documentation/Cocoa/Reference/ApplicationKit/Classes/NSView_Class/index.html#//apple_ref/doc/uid/20000014-SW222
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::systemLayoutDirectionKey(), WebKit::WebPreferencesStore::Value(convertSystemLayoutDirection(self.userInterfaceLayoutDirection)));
#endif

#if PLATFORM(IOS)
    pageConfiguration->setAlwaysRunsAtForegroundPriority([_configuration _alwaysRunsAtForegroundPriority]);

    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::allowsInlineMediaPlaybackKey(), WebKit::WebPreferencesStore::Value(!![_configuration allowsInlineMediaPlayback]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::allowsInlineMediaPlaybackAfterFullscreenKey(), WebKit::WebPreferencesStore::Value(!![_configuration _allowsInlineMediaPlaybackAfterFullscreen]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::inlineMediaPlaybackRequiresPlaysInlineAttributeKey(), WebKit::WebPreferencesStore::Value(!![_configuration _inlineMediaPlaybackRequiresPlaysInlineAttribute]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::allowsPictureInPictureMediaPlaybackKey(), WebKit::WebPreferencesStore::Value(!![_configuration allowsPictureInPictureMediaPlayback] && shouldAllowPictureInPictureMediaPlayback()));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::userInterfaceDirectionPolicyKey(), WebKit::WebPreferencesStore::Value(static_cast<uint32_t>(WebCore::UserInterfaceDirectionPolicy::Content)));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::systemLayoutDirectionKey(), WebKit::WebPreferencesStore::Value(static_cast<uint32_t>(WebCore::LTR)));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::allowSettingAnyXHRHeaderFromFileURLsKey(), WebKit::WebPreferencesStore::Value(shouldAllowSettingAnyXHRHeaderFromFileURLs()));
#endif

    WKAudiovisualMediaTypes mediaTypesRequiringUserGesture = [_configuration mediaTypesRequiringUserActionForPlayback];
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::requiresUserGestureForVideoPlaybackKey(), WebKit::WebPreferencesStore::Value((mediaTypesRequiringUserGesture & WKAudiovisualMediaTypeVideo) == WKAudiovisualMediaTypeVideo));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::requiresUserGestureForAudioPlaybackKey(), WebKit::WebPreferencesStore::Value(((mediaTypesRequiringUserGesture & WKAudiovisualMediaTypeAudio) == WKAudiovisualMediaTypeAudio)));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::requiresUserGestureToLoadVideoKey(), WebKit::WebPreferencesStore::Value(shouldRequireUserGestureToLoadVideo()));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::mainContentUserGestureOverrideEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _mainContentUserGestureOverrideEnabled]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::invisibleAutoplayNotPermittedKey(), WebKit::WebPreferencesStore::Value(!![_configuration _invisibleAutoplayNotPermitted]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::mediaDataLoadsAutomaticallyKey(), WebKit::WebPreferencesStore::Value(!![_configuration _mediaDataLoadsAutomatically]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::attachmentElementEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _attachmentElementEnabled]));

#if ENABLE(DATA_DETECTION) && PLATFORM(IOS)
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::dataDetectorTypesKey(), WebKit::WebPreferencesStore::Value(static_cast<uint32_t>(fromWKDataDetectorTypes([_configuration dataDetectorTypes]))));
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::allowsAirPlayForMediaPlaybackKey(), WebKit::WebPreferencesStore::Value(!![_configuration allowsAirPlayForMediaPlayback]));
#endif

#if ENABLE(APPLE_PAY)
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::applePayEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _applePayEnabled]));
#endif

    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::needsStorageAccessFromFileURLsQuirkKey(), WebKit::WebPreferencesStore::Value(!![_configuration _needsStorageAccessFromFileURLsQuirk]));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::mediaContentTypesRequiringHardwareSupportKey(), WebKit::WebPreferencesStore::Value(String([_configuration _mediaContentTypesRequiringHardwareSupport])));
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::allowMediaContentTypesRequiringHardwareSupportAsFallbackKey(), WebKit::WebPreferencesStore::Value(!![_configuration _allowMediaContentTypesRequiringHardwareSupportAsFallback]));

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    pageConfiguration->preferenceValues().set(WebKit::WebPreferencesKey::legacyEncryptedMediaAPIEnabledKey(), WebKit::WebPreferencesStore::Value(!![_configuration _legacyEncryptedMediaAPIEnabled]));
#endif

#if PLATFORM(IOS)
    CGRect bounds = self.bounds;
    _scrollView = adoptNS([[WKScrollView alloc] initWithFrame:bounds]);
    [_scrollView setInternalDelegate:self];
    [_scrollView setBouncesZoom:YES];

    _avoidsUnsafeArea = YES;
    [self _updateScrollViewInsetAdjustmentBehavior];

    [self addSubview:_scrollView.get()];

    static uint32_t programSDKVersion = dyld_get_program_sdk_version();
    _allowsLinkPreview = programSDKVersion >= firstSDKVersionWithLinkPreviewEnabledByDefault;

    _contentView = adoptNS([[WKContentView alloc] initWithFrame:bounds processPool:processPool configuration:WTFMove(pageConfiguration) webView:self]);

    _page = [_contentView page];
    [self _dispatchSetDeviceOrientation:deviceOrientation()];
    _page->setDrawsBackground(self.opaque);

    [_contentView layer].anchorPoint = CGPointZero;
    [_contentView setFrame:bounds];
    [_scrollView addSubview:_contentView.get()];
    [_scrollView addSubview:[_contentView unscaledView]];
    [self _updateScrollViewBackground];
    _obscuredInsetEdgesAffectedBySafeArea = UIRectEdgeTop | UIRectEdgeLeft | UIRectEdgeRight;

    _viewportMetaTagWidth = WebCore::ViewportArguments::ValueAuto;
    _initialScaleFactor = 1;
    _fastClickingIsDisabled = [[NSUserDefaults standardUserDefaults] boolForKey:@"WebKitFastClickingDisabled"];

    [self _frameOrBoundsChanged];

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(_keyboardWillChangeFrame:) name:UIKeyboardWillChangeFrameNotification object:nil];
    [center addObserver:self selector:@selector(_keyboardDidChangeFrame:) name:UIKeyboardDidChangeFrameNotification object:nil];
    [center addObserver:self selector:@selector(_keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
    [center addObserver:self selector:@selector(_keyboardDidShow:) name:UIKeyboardDidShowNotification object:nil];
    [center addObserver:self selector:@selector(_keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
    [center addObserver:self selector:@selector(_windowDidRotate:) name:UIWindowDidRotateNotification object:nil];
    [center addObserver:self selector:@selector(_contentSizeCategoryDidChange:) name:UIContentSizeCategoryDidChangeNotification object:nil];
    _page->contentSizeCategoryDidChange([self _contentSizeCategory]);

    [center addObserver:self selector:@selector(_accessibilitySettingsDidChange:) name:UIAccessibilityGrayscaleStatusDidChangeNotification object:nil];
    [center addObserver:self selector:@selector(_accessibilitySettingsDidChange:) name:UIAccessibilityInvertColorsStatusDidChangeNotification object:nil];
    [center addObserver:self selector:@selector(_accessibilitySettingsDidChange:) name:UIAccessibilityReduceMotionStatusDidChangeNotification object:nil];

    [[_configuration _contentProviderRegistry] addPage:*_page];
    _page->setForceAlwaysUserScalable([_configuration ignoresViewportScaleLimits]);
#endif

#if PLATFORM(MAC)
    _impl = std::make_unique<WebKit::WebViewImpl>(self, self, processPool, WTFMove(pageConfiguration));
    _page = &_impl->page();

    _impl->setAutomaticallyAdjustsContentInsets(true);
    _impl->setRequiresUserActionForEditingControlsManager([configuration _requiresUserActionForEditingControlsManager]);
#endif

    _page->setBackgroundExtendsBeyondPage(true);

    if (NSString *applicationNameForUserAgent = configuration.applicationNameForUserAgent)
        _page->setApplicationNameForUserAgent(applicationNameForUserAgent);

    _navigationState = std::make_unique<WebKit::NavigationState>(self);
    _page->setNavigationClient(_navigationState->createNavigationClient());

    _uiDelegate = std::make_unique<WebKit::UIDelegate>(self);
    _page->setFindClient(std::make_unique<WebKit::FindClient>(self));
    _page->setDiagnosticLoggingClient(std::make_unique<WebKit::DiagnosticLoggingClient>(self));

    _iconLoadingDelegate = std::make_unique<WebKit::IconLoadingDelegate>(self);

#if PLATFORM(IOS)
    [self _setUpSQLiteDatabaseTrackerClient];
#endif

    auto *handlers = _configuration.get()._urlSchemeHandlers;
    for (NSString *key in handlers)
        _page->setURLSchemeHandlerForScheme(WebKit::WebURLSchemeHandlerCocoa::create(handlers[key]), key);

    pageToViewMap().add(_page.get(), self);

#if PLATFORM(IOS)
    _dragInteractionPolicy = _WKDragInteractionPolicyDefault;
#endif
}

- (void)_setUpSQLiteDatabaseTrackerClient
{
#if PLATFORM(IOS)
    WebBackgroundTaskController *controller = [WebBackgroundTaskController sharedController];
    if (controller.backgroundTaskStartBlock)
        return;

    controller.backgroundTaskStartBlock = ^NSUInteger (void (^expirationHandler)())
    {
        return [[UIApplication sharedApplication] beginBackgroundTaskWithName:@"com.apple.WebKit.DatabaseActivity" expirationHandler:expirationHandler];
    };
    controller.backgroundTaskEndBlock = ^(UIBackgroundTaskIdentifier taskIdentifier)
    {
        [[UIApplication sharedApplication] endBackgroundTask:taskIdentifier];
    };
    controller.invalidBackgroundTaskIdentifier = UIBackgroundTaskInvalid;

    WebCore::SQLiteDatabaseTracker::setClient(&WebCore::WebSQLiteDatabaseTrackerClient::sharedWebSQLiteDatabaseTrackerClient());
#endif
}

- (instancetype)initWithFrame:(CGRect)frame configuration:(WKWebViewConfiguration *)configuration
{
    if (!(self = [super initWithFrame:frame]))
        return nil;

    [self _initializeWithConfiguration:configuration];

    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    if (!(self = [super initWithCoder:coder]))
        return nil;

    WKWebViewConfiguration *configuration = decodeObjectOfClassForKeyFromCoder([WKWebViewConfiguration class], @"configuration", coder);
    [self _initializeWithConfiguration:configuration];

    self.allowsBackForwardNavigationGestures = [coder decodeBoolForKey:@"allowsBackForwardNavigationGestures"];
    self.customUserAgent = decodeObjectOfClassForKeyFromCoder([NSString class], @"customUserAgent", coder);
    self.allowsLinkPreview = [coder decodeBoolForKey:@"allowsLinkPreview"];

#if PLATFORM(MAC)
    self.allowsMagnification = [coder decodeBoolForKey:@"allowsMagnification"];
    self.magnification = [coder decodeDoubleForKey:@"magnification"];
#endif

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [super encodeWithCoder:coder];

    [coder encodeObject:_configuration.get() forKey:@"configuration"];

    [coder encodeBool:self.allowsBackForwardNavigationGestures forKey:@"allowsBackForwardNavigationGestures"];
    [coder encodeObject:self.customUserAgent forKey:@"customUserAgent"];
    [coder encodeBool:self.allowsLinkPreview forKey:@"allowsLinkPreview"];

#if PLATFORM(MAC)
    [coder encodeBool:self.allowsMagnification forKey:@"allowsMagnification"];
    [coder encodeDouble:self.magnification forKey:@"magnification"];
#endif
}

- (void)dealloc
{
#if PLATFORM(MAC)
    [_textFinderClient willDestroyView:self];
#endif

#if PLATFORM(IOS)
    [_contentView _webViewDestroyed];

    if (_remoteObjectRegistry)
        _page->process().processPool().removeMessageReceiver(Messages::RemoteObjectRegistry::messageReceiverName(), _page->pageID());
#endif

    _page->close();

#if PLATFORM(IOS)
    [_remoteObjectRegistry _invalidate];
    [[_configuration _contentProviderRegistry] removePage:*_page];
    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDarwinNotifyCenter(), (__bridge const void *)(self), nullptr, nullptr);
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [_scrollView setInternalDelegate:nil];
#endif

    pageToViewMap().remove(_page.get());

    [super dealloc];
}

- (WKWebViewConfiguration *)configuration
{
    return [[_configuration copy] autorelease];
}

- (WKBackForwardList *)backForwardList
{
    return wrapper(_page->backForwardList());
}

- (id <WKNavigationDelegate>)navigationDelegate
{
    return _navigationState->navigationDelegate().autorelease();
}

- (void)setNavigationDelegate:(id <WKNavigationDelegate>)navigationDelegate
{
    _page->setNavigationClient(_navigationState->createNavigationClient());
    _navigationState->setNavigationDelegate(navigationDelegate);
}

- (id <WKUIDelegate>)UIDelegate
{
    return _uiDelegate->delegate().autorelease();
}

- (void)setUIDelegate:(id<WKUIDelegate>)UIDelegate
{
    _uiDelegate->setDelegate(UIDelegate);
#if ENABLE(CONTEXT_MENUS)
    _page->setContextMenuClient(_uiDelegate->createContextMenuClient());
#endif
    _page->setUIClient(_uiDelegate->createUIClient());
}

- (id <_WKIconLoadingDelegate>)_iconLoadingDelegate
{
    return _iconLoadingDelegate->delegate().autorelease();
}

- (void)_setIconLoadingDelegate:(id<_WKIconLoadingDelegate>)iconLoadingDelegate
{
    _page->setIconLoadingClient(_iconLoadingDelegate->createIconLoadingClient());
    _iconLoadingDelegate->setDelegate(iconLoadingDelegate);
}

- (WKNavigation *)loadRequest:(NSURLRequest *)request
{
    auto navigation = _page->loadRequest(request);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)loadFileURL:(NSURL *)URL allowingReadAccessToURL:(NSURL *)readAccessURL
{
    if (![URL isFileURL])
        [NSException raise:NSInvalidArgumentException format:@"%@ is not a file URL", URL];

    if (![readAccessURL isFileURL])
        [NSException raise:NSInvalidArgumentException format:@"%@ is not a file URL", readAccessURL];

    auto navigation = _page->loadFile([URL _web_originalDataAsWTFString], [readAccessURL _web_originalDataAsWTFString]);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)loadHTMLString:(NSString *)string baseURL:(NSURL *)baseURL
{
    NSData *data = [string dataUsingEncoding:NSUTF8StringEncoding];

    return [self loadData:data MIMEType:@"text/html" characterEncodingName:@"UTF-8" baseURL:baseURL];
}

- (WKNavigation *)loadData:(NSData *)data MIMEType:(NSString *)MIMEType characterEncodingName:(NSString *)characterEncodingName baseURL:(NSURL *)baseURL
{
    auto navigation = _page->loadData(API::Data::createWithoutCopying(data).ptr(), MIMEType, characterEncodingName, baseURL.absoluteString);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)goToBackForwardListItem:(WKBackForwardListItem *)item
{
    auto navigation = _page->goToBackForwardItem(&item._item);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (NSString *)title
{
    return _page->pageLoadState().title();
}

- (NSURL *)URL
{
    return [NSURL _web_URLWithWTFString:_page->pageLoadState().activeURL()];
}

- (BOOL)isLoading
{
    return _page->pageLoadState().isLoading();
}

- (double)estimatedProgress
{
    return _page->pageLoadState().estimatedProgress();
}

- (BOOL)hasOnlySecureContent
{
    return _page->pageLoadState().hasOnlySecureContent();
}

- (SecTrustRef)serverTrust
{
#if HAVE(SEC_TRUST_SERIALIZATION)
    auto certificateInfo = _page->pageLoadState().certificateInfo();
    if (!certificateInfo)
        return nil;

    return certificateInfo->certificateInfo().trust();
#else
    return nil;
#endif
}

- (BOOL)canGoBack
{
    return _page->pageLoadState().canGoBack();
}

- (BOOL)canGoForward
{
    return _page->pageLoadState().canGoForward();
}

- (WKNavigation *)goBack
{
    auto navigation = _page->goBack();
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)goForward
{
    auto navigation = _page->goForward();
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)reload
{
    OptionSet<WebCore::ReloadOption> reloadOptions;
    if (linkedOnOrAfter(WebKit::SDKVersion::FirstWithExpiredOnlyReloadBehavior))
        reloadOptions |= WebCore::ReloadOption::ExpiredOnly;

    auto navigation = _page->reload(reloadOptions);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)reloadFromOrigin
{
    auto navigation = _page->reload(WebCore::ReloadOption::FromOrigin);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (void)stopLoading
{
    _page->stopLoading();
}

static WKErrorCode callbackErrorCode(WebKit::CallbackBase::Error error)
{
    switch (error) {
    case WebKit::CallbackBase::Error::None:
        ASSERT_NOT_REACHED();
        return WKErrorUnknown;

    case WebKit::CallbackBase::Error::Unknown:
        return WKErrorUnknown;

    case WebKit::CallbackBase::Error::ProcessExited:
        return WKErrorWebContentProcessTerminated;

    case WebKit::CallbackBase::Error::OwnerWasInvalidated:
        return WKErrorWebViewInvalidated;
    }
}

- (void)evaluateJavaScript:(NSString *)javaScriptString completionHandler:(void (^)(id, NSError *))completionHandler
{
    [self _evaluateJavaScript:javaScriptString forceUserGesture:YES completionHandler:completionHandler];
}

- (void)_evaluateJavaScript:(NSString *)javaScriptString forceUserGesture:(BOOL)forceUserGesture completionHandler:(void (^)(id, NSError *))completionHandler
{
    auto handler = adoptNS([completionHandler copy]);

    _page->runJavaScriptInMainFrame(javaScriptString, forceUserGesture, [handler](API::SerializedScriptValue* serializedScriptValue, bool hadException, const WebCore::ExceptionDetails& details, WebKit::ScriptValueCallback::Error errorCode) {
        if (!handler)
            return;

        if (errorCode != WebKit::ScriptValueCallback::Error::None) {
            auto error = createNSError(callbackErrorCode(errorCode));
            if (errorCode == WebKit::ScriptValueCallback::Error::OwnerWasInvalidated) {
                // The OwnerWasInvalidated callback is synchronous. We don't want to call the block from within it
                // because that can trigger re-entrancy bugs in WebKit.
                // FIXME: It would be even better if GenericCallback did this for us.
                dispatch_async(dispatch_get_main_queue(), [handler, error] {
                    auto rawHandler = (void (^)(id, NSError *))handler.get();
                    rawHandler(nil, error.get());
                });
                return;
            }

            auto rawHandler = (void (^)(id, NSError *))handler.get();
            rawHandler(nil, error.get());
            return;
        }

        auto rawHandler = (void (^)(id, NSError *))handler.get();
        if (hadException) {
            ASSERT(!serializedScriptValue);

            RetainPtr<NSMutableDictionary> userInfo = adoptNS([[NSMutableDictionary alloc] init]);

            [userInfo setObject:localizedDescriptionForErrorCode(WKErrorJavaScriptExceptionOccurred) forKey:NSLocalizedDescriptionKey];
            [userInfo setObject:static_cast<NSString *>(details.message) forKey:_WKJavaScriptExceptionMessageErrorKey];
            [userInfo setObject:@(details.lineNumber) forKey:_WKJavaScriptExceptionLineNumberErrorKey];
            [userInfo setObject:@(details.columnNumber) forKey:_WKJavaScriptExceptionColumnNumberErrorKey];

            if (!details.sourceURL.isEmpty())
                [userInfo setObject:[NSURL _web_URLWithWTFString:details.sourceURL] forKey:_WKJavaScriptExceptionSourceURLErrorKey];

            rawHandler(nil, adoptNS([[NSError alloc] initWithDomain:WKErrorDomain code:WKErrorJavaScriptExceptionOccurred userInfo:userInfo.get()]).get());
            return;
        }

        if (!serializedScriptValue) {
            rawHandler(nil, createNSError(WKErrorJavaScriptResultTypeIsUnsupported).get());
            return;
        }

        id body = API::SerializedScriptValue::deserialize(serializedScriptValue->internalRepresentation(), 0);
        rawHandler(body, nil);
    });
}

#if PLATFORM(MAC)
- (void)takeSnapshotWithConfiguration:(WKSnapshotConfiguration *)snapshotConfiguration completionHandler:(void(^)(NSImage *, NSError *))completionHandler
{
    CGRect rectInViewCoordinates = snapshotConfiguration && !CGRectIsNull(snapshotConfiguration.rect) ? snapshotConfiguration.rect : self.bounds;
    CGFloat snapshotWidth;
    if (snapshotConfiguration)
        snapshotWidth = snapshotConfiguration.snapshotWidth.doubleValue ?: rectInViewCoordinates.size.width;
    else
        snapshotWidth = self.bounds.size.width;

    auto handler = makeBlockPtr(completionHandler);
    CGFloat imageScale = snapshotWidth / rectInViewCoordinates.size.width;
    CGFloat imageHeight = imageScale * rectInViewCoordinates.size.height;

    // Need to scale by device scale factor or the image will be distorted.
    CGFloat deviceScale = _page->deviceScaleFactor();
    WebCore::IntSize bitmapSize(snapshotWidth, imageHeight);
    bitmapSize.scale(deviceScale, deviceScale);

    // Software snapshot will not capture elements rendered with hardware acceleration (WebGL, video, etc).
    _page->takeSnapshot(WebCore::enclosingIntRect(rectInViewCoordinates), bitmapSize, WebKit::SnapshotOptionsInViewCoordinates, [handler, snapshotWidth, imageHeight](const WebKit::ShareableBitmap::Handle& imageHandle, WebKit::CallbackBase::Error errorCode) {
        if (errorCode != WebKit::ScriptValueCallback::Error::None) {
            auto error = createNSError(callbackErrorCode(errorCode));
            handler(nil, error.get());
            return;
        }

        RefPtr<WebKit::ShareableBitmap> bitmap = WebKit::ShareableBitmap::create(imageHandle, WebKit::SharedMemory::Protection::ReadOnly);
        RetainPtr<CGImageRef> cgImage = bitmap ? bitmap->makeCGImage() : nullptr;
        RetainPtr<NSImage> nsImage = adoptNS([[NSImage alloc] initWithCGImage:cgImage.get() size:NSMakeSize(snapshotWidth, imageHeight)]);
        handler(nsImage.get(), nil);
    });
}

#elif PLATFORM(IOS)
- (void)takeSnapshotWithConfiguration:(WKSnapshotConfiguration *)snapshotConfiguration completionHandler:(void(^)(UIImage *, NSError *))completionHandler
{
    CGRect rectInViewCoordinates = snapshotConfiguration && !CGRectIsNull(snapshotConfiguration.rect) ? snapshotConfiguration.rect : self.bounds;
    CGFloat snapshotWidth;
    if (snapshotConfiguration)
        snapshotWidth = snapshotConfiguration.snapshotWidth.doubleValue ?: rectInViewCoordinates.size.width;
    else
        snapshotWidth = self.bounds.size.width;

    auto handler = makeBlockPtr(completionHandler);
    CGFloat deviceScale = _page->deviceScaleFactor();

    [self _snapshotRect:rectInViewCoordinates intoImageOfWidth:(snapshotWidth * deviceScale) completionHandler:^(CGImageRef snapshotImage) {
        RetainPtr<NSError> error;
        RetainPtr<UIImage> uiImage;

        if (!snapshotImage)
            error = createNSError(WKErrorUnknown);
        else
            uiImage = adoptNS([[UIImage alloc] initWithCGImage:snapshotImage scale:deviceScale orientation:UIImageOrientationUp]);

        handler(uiImage.get(), error.get());
    }];
}
#endif

- (NSString *)customUserAgent
{
    return _page->customUserAgent();
}

- (void)setCustomUserAgent:(NSString *)customUserAgent
{
    _page->setCustomUserAgent(customUserAgent);
}

- (WKPageRef)_pageForTesting
{
    return toAPI(_page.get());
}

- (WebKit::WebPageProxy *)_page
{
    return _page.get();
}

- (BOOL)allowsLinkPreview
{
#if PLATFORM(MAC)
    return _impl->allowsLinkPreview();
#elif PLATFORM(IOS)
    return _allowsLinkPreview;
#endif
}

- (void)setAllowsLinkPreview:(BOOL)allowsLinkPreview
{
#if PLATFORM(MAC)
    _impl->setAllowsLinkPreview(allowsLinkPreview);
    return;
#elif PLATFORM(IOS)
    if (_allowsLinkPreview == allowsLinkPreview)
        return;

    _allowsLinkPreview = allowsLinkPreview;

#if HAVE(LINK_PREVIEW)
    if (_allowsLinkPreview)
        [_contentView _registerPreview];
    else
        [_contentView _unregisterPreview];
#endif // HAVE(LINK_PREVIEW)
#endif // PLATFORM(IOS)
}

- (CGSize)_viewportSizeForCSSViewportUnits
{
    return _page->viewportSizeForCSSViewportUnits();
}

- (void)_setViewportSizeForCSSViewportUnits:(CGSize)viewportSize
{
    auto viewportSizeForViewportUnits = WebCore::IntSize(viewportSize);
    if (viewportSizeForViewportUnits.isEmpty())
        [NSException raise:NSInvalidArgumentException format:@"Viewport size should not be empty"];

    _page->setViewportSizeForCSSViewportUnits(viewportSizeForViewportUnits);
}

static NSTextAlignment nsTextAlignment(WebKit::TextAlignment alignment)
{
    switch (alignment) {
    case WebKit::NoAlignment:
        return NSTextAlignmentNatural;
    case WebKit::LeftAlignment:
        return NSTextAlignmentLeft;
    case WebKit::RightAlignment:
        return NSTextAlignmentRight;
    case WebKit::CenterAlignment:
        return NSTextAlignmentCenter;
    case WebKit::JustifiedAlignment:
        return NSTextAlignmentJustified;
    }
    ASSERT_NOT_REACHED();
    return NSTextAlignmentNatural;
}

static NSDictionary *dictionaryRepresentationForEditorState(const WebKit::EditorState& state)
{
    if (state.isMissingPostLayoutData)
        return @{ @"post-layout-data" : @NO };

    auto& postLayoutData = state.postLayoutData();
    return @{
        @"post-layout-data" : @YES,
        @"bold": postLayoutData.typingAttributes & WebKit::AttributeBold ? @YES : @NO,
        @"italic": postLayoutData.typingAttributes & WebKit::AttributeItalics ? @YES : @NO,
        @"underline": postLayoutData.typingAttributes & WebKit::AttributeUnderline ? @YES : @NO,
        @"text-alignment": @(nsTextAlignment(static_cast<WebKit::TextAlignment>(postLayoutData.textAlignment))),
        @"text-color": (NSString *)postLayoutData.textColor.cssText()
    };
}

- (void)_didChangeEditorState
{
    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)self.UIDelegate;
    if ([uiDelegate respondsToSelector:@selector(_webView:editorStateDidChange:)])
        [uiDelegate _webView:self editorStateDidChange:dictionaryRepresentationForEditorState(_page->editorState())];
}

#if ENABLE(ATTACHMENT_ELEMENT)

- (void)_didInsertAttachment:(NSString *)identifier withSource:(NSString *)source
{
    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)self.UIDelegate;
    if ([uiDelegate respondsToSelector:@selector(_webView:didInsertAttachment:withSource:)])
        [uiDelegate _webView:self didInsertAttachment:[wrapper(API::Attachment::create(identifier, *_page).leakRef()) autorelease] withSource:source];
    else if ([uiDelegate respondsToSelector:@selector(_webView:didInsertAttachment:)])
        [uiDelegate _webView:self didInsertAttachment:[wrapper(API::Attachment::create(identifier, *_page).leakRef()) autorelease]];
}

- (void)_didRemoveAttachment:(NSString *)identifier
{
    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)self.UIDelegate;
    if ([uiDelegate respondsToSelector:@selector(_webView:didRemoveAttachment:)])
        [uiDelegate _webView:self didRemoveAttachment:[wrapper(API::Attachment::create(identifier, *_page).leakRef()) autorelease]];
}

#endif // ENABLE(ATTACHMENT_ELEMENT)

#pragma mark iOS-specific methods

#if PLATFORM(IOS)

- (_WKDragInteractionPolicy)_dragInteractionPolicy
{
    return _dragInteractionPolicy;
}

- (void)_setDragInteractionPolicy:(_WKDragInteractionPolicy)policy
{
    if (_dragInteractionPolicy == policy)
        return;

    _dragInteractionPolicy = policy;
#if ENABLE(DRAG_SUPPORT)
    [_contentView _didChangeDragInteractionPolicy];
#endif
}

- (void)_populateArchivedSubviews:(NSMutableSet *)encodedViews
{
    [super _populateArchivedSubviews:encodedViews];

    if (_scrollView)
        [encodedViews removeObject:_scrollView.get()];
    if (_customContentFixedOverlayView)
        [encodedViews removeObject:_customContentFixedOverlayView.get()];
}

- (BOOL)_isBackground
{
    if ([self _isDisplayingPDF])
        return [(WKPDFView *)_customContentView isBackground];

    return [_contentView isBackground];
}

- (void)setFrame:(CGRect)frame
{
    CGRect oldFrame = self.frame;
    [super setFrame:frame];

    if (!CGSizeEqualToSize(oldFrame.size, frame.size))
        [self _frameOrBoundsChanged];
}

- (void)setBounds:(CGRect)bounds
{
    CGRect oldBounds = self.bounds;
    [super setBounds:bounds];
    [_customContentFixedOverlayView setFrame:self.bounds];

    if (!CGSizeEqualToSize(oldBounds.size, bounds.size))
        [self _frameOrBoundsChanged];
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    [self _frameOrBoundsChanged];
}

- (UIScrollView *)scrollView
{
    return _scrollView.get();
}

- (WKBrowsingContextController *)browsingContextController
{
    return [_contentView browsingContextController];
}

- (BOOL)becomeFirstResponder
{
    UIView *currentContentView = self._currentContentView;
    if (currentContentView == _contentView && [_contentView superview])
        return [_contentView becomeFirstResponderForWebView] || [super becomeFirstResponder];

    return [currentContentView becomeFirstResponder] || [super becomeFirstResponder];
}

- (BOOL)canBecomeFirstResponder
{
    if (self._currentContentView == _contentView)
        return [_contentView canBecomeFirstResponderForWebView];

    return YES;
}

- (BOOL)resignFirstResponder
{
    if ([_contentView isFirstResponder])
        return [_contentView resignFirstResponderForWebView];

    return [super resignFirstResponder];
}

#define FORWARD_ACTION_TO_WKCONTENTVIEW(_action) \
    - (void)_action:(id)sender \
    { \
        if (self.usesStandardContentView) \
            [_contentView _action ## ForWebView:sender]; \
    }

FOR_EACH_WKCONTENTVIEW_ACTION(FORWARD_ACTION_TO_WKCONTENTVIEW)

#undef FORWARD_ACTION_TO_WKCONTENTVIEW

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
    #define FORWARD_CANPERFORMACTION_TO_WKCONTENTVIEW(_action) \
        if (action == @selector(_action:)) \
            return self.usesStandardContentView && [_contentView canPerformActionForWebView:action withSender:sender];

    FOR_EACH_WKCONTENTVIEW_ACTION(FORWARD_CANPERFORMACTION_TO_WKCONTENTVIEW)

    #undef FORWARD_CANPERFORMACTION_TO_WKCONTENTVIEW

    return [super canPerformAction:action withSender:sender];
}

- (id)targetForAction:(SEL)action withSender:(id)sender
{
    #define FORWARD_TARGETFORACTION_TO_WKCONTENTVIEW(_action) \
        if (action == @selector(_action:) && self.usesStandardContentView) \
            return [_contentView targetForActionForWebView:action withSender:sender];

    FOR_EACH_WKCONTENTVIEW_ACTION(FORWARD_TARGETFORACTION_TO_WKCONTENTVIEW)

    #undef FORWARD_TARGETFORACTION_TO_WKCONTENTVIEW

    return [super targetForAction:action withSender:sender];
}

static inline CGFloat floorToDevicePixel(CGFloat input, float deviceScaleFactor)
{
    return CGFloor(input * deviceScaleFactor) / deviceScaleFactor;
}

static inline bool pointsEqualInDevicePixels(CGPoint a, CGPoint b, float deviceScaleFactor)
{
    return fabs(a.x * deviceScaleFactor - b.x * deviceScaleFactor) < std::numeric_limits<float>::epsilon()
        && fabs(a.y * deviceScaleFactor - b.y * deviceScaleFactor) < std::numeric_limits<float>::epsilon();
}

static CGSize roundScrollViewContentSize(const WebKit::WebPageProxy& page, CGSize contentSize)
{
    float deviceScaleFactor = page.deviceScaleFactor();
    return CGSizeMake(floorToDevicePixel(contentSize.width, deviceScaleFactor), floorToDevicePixel(contentSize.height, deviceScaleFactor));
}

- (UIView *)_currentContentView
{
    return _customContentView ? _customContentView.get() : _contentView.get();
}

- (WKWebViewContentProviderRegistry *)_contentProviderRegistry
{
    return [_configuration _contentProviderRegistry];
}

- (WKSelectionGranularity)_selectionGranularity
{
    return [_configuration selectionGranularity];
}

- (void)_setHasCustomContentView:(BOOL)pageHasCustomContentView loadedMIMEType:(const WTF::String&)mimeType
{
    if (pageHasCustomContentView) {
        [_customContentView removeFromSuperview];
        [_customContentFixedOverlayView removeFromSuperview];

        Class representationClass = [[_configuration _contentProviderRegistry] providerForMIMEType:mimeType];
        ASSERT(representationClass);
        _customContentView = adoptNS([[representationClass alloc] web_initWithFrame:self.bounds webView:self]);
        _customContentFixedOverlayView = adoptNS([[UIView alloc] initWithFrame:self.bounds]);
        [_customContentFixedOverlayView layer].name = @"CustomContentFixedOverlay";
        [_customContentFixedOverlayView setUserInteractionEnabled:NO];

        [[_contentView unscaledView] removeFromSuperview];
        [_contentView removeFromSuperview];
        [_scrollView addSubview:_customContentView.get()];
        [self addSubview:_customContentFixedOverlayView.get()];

        [_customContentView web_setMinimumSize:self.bounds.size];
        [_customContentView web_setFixedOverlayView:_customContentFixedOverlayView.get()];

        _scrollViewBackgroundColor = WebCore::Color();
        [_scrollView setContentOffset:[self _adjustedContentOffset:CGPointZero]];

        [self _setAvoidsUnsafeArea:NO];
    } else if (_customContentView) {
        [_customContentView removeFromSuperview];
        _customContentView = nullptr;

        [_customContentFixedOverlayView removeFromSuperview];
        _customContentFixedOverlayView = nullptr;

        [_scrollView addSubview:_contentView.get()];
        [_scrollView addSubview:[_contentView unscaledView]];
        [_scrollView setContentSize:roundScrollViewContentSize(*_page, [_contentView frame].size)];

        [_customContentFixedOverlayView setFrame:self.bounds];
        [self addSubview:_customContentFixedOverlayView.get()];
    }

    if (self.isFirstResponder) {
        UIView *currentContentView = self._currentContentView;
        if (currentContentView == _contentView ? [_contentView canBecomeFirstResponderForWebView] : currentContentView.canBecomeFirstResponder)
            [currentContentView becomeFirstResponder];
    }
}

- (void)_didFinishLoadingDataForCustomContentProviderWithSuggestedFilename:(const String&)suggestedFilename data:(NSData *)data
{
    ASSERT(_customContentView);
    [_customContentView web_setContentProviderData:data suggestedFilename:suggestedFilename];

    // FIXME: It may make more sense for custom content providers to invoke this when they're ready,
    // because there's no guarantee that all custom content providers will lay out synchronously.
    _page->didLayoutForCustomContentProvider();
}

- (void)_willInvokeUIScrollViewDelegateCallback
{
    _delayUpdateVisibleContentRects = YES;
}

- (void)_didInvokeUIScrollViewDelegateCallback
{
    _delayUpdateVisibleContentRects = NO;
    if (_hadDelayedUpdateVisibleContentRects) {
        _hadDelayedUpdateVisibleContentRects = NO;
        [self _scheduleVisibleContentRectUpdate];
    }
}

static CGFloat contentZoomScale(WKWebView *webView)
{
    CGFloat scale = webView._currentContentView.layer.affineTransform.a;
    ASSERT(scale == [webView->_scrollView zoomScale]);
    return scale;
}

static WebCore::Color baseScrollViewBackgroundColor(WKWebView *webView)
{
    if (webView->_customContentView)
        return [webView->_customContentView backgroundColor].CGColor;

    if (webView->_gestureController) {
        WebCore::Color color = webView->_gestureController->backgroundColorForCurrentSnapshot();
        if (color.isValid())
            return color;
    }

    return webView->_page->pageExtendedBackgroundColor();
}

static WebCore::Color scrollViewBackgroundColor(WKWebView *webView)
{
    if (!webView.opaque)
        return WebCore::Color::transparent;

    WebCore::Color color = baseScrollViewBackgroundColor(webView);

    if (!color.isValid())
        color = WebCore::Color::white;

    CGFloat zoomScale = contentZoomScale(webView);
    CGFloat minimumZoomScale = [webView->_scrollView minimumZoomScale];
    if (zoomScale < minimumZoomScale) {
        CGFloat slope = 12;
        CGFloat opacity = std::max<CGFloat>(1 - slope * (minimumZoomScale - zoomScale), 0);
        color = WebCore::colorWithOverrideAlpha(color.rgb(), opacity);
    }

    return color;
}

- (void)_updateScrollViewBackground
{
    WebCore::Color color = scrollViewBackgroundColor(self);

    if (_scrollViewBackgroundColor == color)
        return;

    _scrollViewBackgroundColor = color;

    auto uiBackgroundColor = adoptNS([[UIColor alloc] initWithCGColor:cachedCGColor(color)]);
    [_scrollView setBackgroundColor:uiBackgroundColor.get()];

    // Update the indicator style based on the lightness/darkness of the background color.
    double hue, saturation, lightness;
    color.getHSL(hue, saturation, lightness);
    if (lightness <= .5 && color.isVisible())
        [_scrollView setIndicatorStyle:UIScrollViewIndicatorStyleWhite];
    else
        [_scrollView setIndicatorStyle:UIScrollViewIndicatorStyleDefault];
}

- (CGPoint)_adjustedContentOffset:(CGPoint)point
{
    CGPoint result = point;
    UIEdgeInsets contentInset = [self _computedContentInset];

    result.x -= contentInset.left;
    result.y -= contentInset.top;

    return result;
}

- (UIRectEdge)_effectiveObscuredInsetEdgesAffectedBySafeArea
{
    if (![self usesStandardContentView])
        return UIRectEdgeAll;
    return _obscuredInsetEdgesAffectedBySafeArea;
}

- (UIEdgeInsets)_computedContentInset
{
    if (_haveSetObscuredInsets)
        return _obscuredInsets;

    UIEdgeInsets insets = [_scrollView contentInset];

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
    if (self._safeAreaShouldAffectObscuredInsets)
        insets = UIEdgeInsetsAdd(insets, self._scrollViewSystemContentInset, self._effectiveObscuredInsetEdgesAffectedBySafeArea);
#endif

    return insets;
}

- (UIEdgeInsets)_computedUnobscuredSafeAreaInset
{
    if (_haveSetUnobscuredSafeAreaInsets)
        return _unobscuredSafeAreaInsets;

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
    if (!self._safeAreaShouldAffectObscuredInsets)
        return self.safeAreaInsets;
#endif

    return UIEdgeInsetsZero;
}

- (void)_processDidExit
{
    [self _hidePasswordView];
    if (!_customContentView && _dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing) {
        NSUInteger indexOfResizeAnimationView = [[_scrollView subviews] indexOfObject:_resizeAnimationView.get()];
        [_scrollView insertSubview:_contentView.get() atIndex:indexOfResizeAnimationView];
        [_scrollView insertSubview:[_contentView unscaledView] atIndex:indexOfResizeAnimationView + 1];
        [_resizeAnimationView removeFromSuperview];
        _resizeAnimationView = nil;

        _resizeAnimationTransformAdjustments = CATransform3DIdentity;
    }
    [_contentView setFrame:self.bounds];
    [_scrollView setBackgroundColor:[UIColor whiteColor]];
    [_scrollView setContentOffset:[self _adjustedContentOffset:CGPointZero]];
    [_scrollView setZoomScale:1];

    _viewportMetaTagWidth = WebCore::ViewportArguments::ValueAuto;
    _initialScaleFactor = 1;
    _hasCommittedLoadForMainFrame = NO;
    _needsResetViewStateAfterCommitLoadForMainFrame = NO;
    _dynamicViewportUpdateMode = DynamicViewportUpdateMode::NotResizing;
    [_contentView setHidden:NO];
    _scrollOffsetToRestore = std::nullopt;
    _unobscuredCenterToRestore = std::nullopt;
    _scrollViewBackgroundColor = WebCore::Color();
    _delayUpdateVisibleContentRects = NO;
    _hadDelayedUpdateVisibleContentRects = NO;
    _lastSentMinimumLayoutSize = std::nullopt;
    _lastSentMaximumUnobscuredSize = std::nullopt;
    _lastSentDeviceOrientation = std::nullopt;

    _frozenVisibleContentRect = std::nullopt;
    _frozenUnobscuredContentRect = std::nullopt;

    _firstPaintAfterCommitLoadTransactionID = 0;
    _firstTransactionIDAfterPageRestore = std::nullopt;
    _resizeAnimationTransformTransactionID = std::nullopt;

    _hasScheduledVisibleRectUpdate = NO;
    _commitDidRestoreScrollPosition = NO;

    _avoidsUnsafeArea = YES;
}

- (void)_didCommitLoadForMainFrame
{
    _firstPaintAfterCommitLoadTransactionID = downcast<WebKit::RemoteLayerTreeDrawingAreaProxy>(*_page->drawingArea()).nextLayerTreeTransactionID();

    _hasCommittedLoadForMainFrame = YES;
    _needsResetViewStateAfterCommitLoadForMainFrame = YES;

    [_scrollView _stopScrollingAndZoomingAnimations];
}

static CGPoint contentOffsetBoundedInValidRange(UIScrollView *scrollView, CGPoint contentOffset)
{
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
    UIEdgeInsets contentInsets = scrollView.adjustedContentInset;
#else
    UIEdgeInsets contentInsets = scrollView.contentInset;
#endif

    CGSize contentSize = scrollView.contentSize;
    CGSize scrollViewSize = scrollView.bounds.size;

    CGPoint minimumContentOffset = CGPointMake(-contentInsets.left, -contentInsets.top);
    CGPoint maximumContentOffset = CGPointMake(std::max(minimumContentOffset.x, contentSize.width + contentInsets.right - scrollViewSize.width), std::max(minimumContentOffset.y, contentSize.height + contentInsets.bottom - scrollViewSize.height));

    return CGPointMake(std::max(std::min(contentOffset.x, maximumContentOffset.x), minimumContentOffset.x), std::max(std::min(contentOffset.y, maximumContentOffset.y), minimumContentOffset.y));
}

static void changeContentOffsetBoundedInValidRange(UIScrollView *scrollView, WebCore::FloatPoint contentOffset)
{
    scrollView.contentOffset = contentOffsetBoundedInValidRange(scrollView, contentOffset);
}

- (WebCore::FloatRect)visibleRectInViewCoordinates
{
    WebCore::FloatRect bounds = self.bounds;
    bounds.moveBy([_scrollView contentOffset]);
    WebCore::FloatRect contentViewBounds = [_contentView bounds];
    bounds.intersect(contentViewBounds);
    return bounds;
}

// WebCore stores the page scale factor as float instead of double. When we get a scale from WebCore,
// we need to ignore differences that are within a small rounding error on floats.
static inline bool areEssentiallyEqualAsFloat(float a, float b)
{
    return WTF::areEssentiallyEqual(a, b);
}

- (void)_didCommitLayerTree:(const WebKit::RemoteLayerTreeTransaction&)layerTreeTransaction
{
    if (![self usesStandardContentView])
        return;

    LOG_WITH_STREAM(VisibleRects, stream << "-[WKWebView " << _page->pageID() << " _didCommitLayerTree:] transactionID " <<  layerTreeTransaction.transactionID() << " _dynamicViewportUpdateMode " << (int)_dynamicViewportUpdateMode);

    bool needUpdateVisibleContentRects = _page->updateLayoutViewportParameters(layerTreeTransaction);

    if (_dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing) {
        if (_resizeAnimationTransformTransactionID && layerTreeTransaction.transactionID() >= _resizeAnimationTransformTransactionID.value()) {
            _resizeAnimationTransformTransactionID = std::nullopt;
            [_resizeAnimationView layer].sublayerTransform = _resizeAnimationTransformAdjustments;
            if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::ResizingWithDocumentHidden) {
                [_contentView setHidden:NO];
                [self _endAnimatedResize];
            }
        }
        return;
    }

    if (_activeAnimatedResizeCount)
        RELEASE_LOG_IF_ALLOWED("%p -[WKWebView _didCommitLayerTree:] - %d animated resizes in flight", self, _activeAnimatedResizeCount);

    CGSize newContentSize = roundScrollViewContentSize(*_page, [_contentView frame].size);
    [_scrollView _setContentSizePreservingContentOffsetDuringRubberband:newContentSize];

    [_scrollView setMinimumZoomScale:layerTreeTransaction.minimumScaleFactor()];
    [_scrollView setMaximumZoomScale:layerTreeTransaction.maximumScaleFactor()];
    [_scrollView setZoomEnabled:layerTreeTransaction.allowsUserScaling()];
    if (!layerTreeTransaction.scaleWasSetByUIProcess() && ![_scrollView isZooming] && ![_scrollView isZoomBouncing] && ![_scrollView _isAnimatingZoom] && [_scrollView zoomScale] != layerTreeTransaction.pageScaleFactor()) {
        LOG_WITH_STREAM(VisibleRects, stream << " updating scroll view with pageScaleFactor " << layerTreeTransaction.pageScaleFactor());
        [_scrollView setZoomScale:layerTreeTransaction.pageScaleFactor()];
    }

    _viewportMetaTagWidth = layerTreeTransaction.viewportMetaTagWidth();
    _viewportMetaTagWidthWasExplicit = layerTreeTransaction.viewportMetaTagWidthWasExplicit();
    _viewportMetaTagCameFromImageDocument = layerTreeTransaction.viewportMetaTagCameFromImageDocument();
    _initialScaleFactor = layerTreeTransaction.initialScaleFactor();
    if (_page->inStableState() && layerTreeTransaction.isInStableState() && [_stableStatePresentationUpdateCallbacks count]) {
        for (dispatch_block_t action in _stableStatePresentationUpdateCallbacks.get())
            action();

        [_stableStatePresentationUpdateCallbacks removeAllObjects];
        _stableStatePresentationUpdateCallbacks = nil;
    }

    if (![_contentView _mayDisableDoubleTapGesturesDuringSingleTap])
        [_contentView _setDoubleTapGesturesEnabled:self._allowsDoubleTapGestures];

    [self _updateScrollViewBackground];
    [self _setAvoidsUnsafeArea:layerTreeTransaction.avoidsUnsafeArea()];

    if (_gestureController)
        _gestureController->setRenderTreeSize(layerTreeTransaction.renderTreeSize());

    if (_needsResetViewStateAfterCommitLoadForMainFrame && layerTreeTransaction.transactionID() >= _firstPaintAfterCommitLoadTransactionID) {
        _needsResetViewStateAfterCommitLoadForMainFrame = NO;
        [_scrollView setContentOffset:[self _adjustedContentOffset:CGPointZero]];
        if (_observedRenderingProgressEvents & _WKRenderingProgressEventFirstPaint)
            _navigationState->didFirstPaint();

        needUpdateVisibleContentRects = true;
    }

    if (_firstTransactionIDAfterPageRestore && layerTreeTransaction.transactionID() >= _firstTransactionIDAfterPageRestore.value()) {
        if (_scrollOffsetToRestore) {
            WebCore::FloatPoint scaledScrollOffset = _scrollOffsetToRestore.value();
            _scrollOffsetToRestore = std::nullopt;

            if (areEssentiallyEqualAsFloat(contentZoomScale(self), _scaleToRestore)) {
                scaledScrollOffset.scale(_scaleToRestore);
                WebCore::FloatPoint contentOffsetInScrollViewCoordinates = scaledScrollOffset - WebCore::FloatSize(_obscuredInsetsWhenSaved.left(), _obscuredInsetsWhenSaved.top());

                changeContentOffsetBoundedInValidRange(_scrollView.get(), contentOffsetInScrollViewCoordinates);
                _commitDidRestoreScrollPosition = YES;
            }

            needUpdateVisibleContentRects = true;
        }

        if (_unobscuredCenterToRestore) {
            WebCore::FloatPoint unobscuredCenterToRestore = _unobscuredCenterToRestore.value();
            _unobscuredCenterToRestore = std::nullopt;

            if (areEssentiallyEqualAsFloat(contentZoomScale(self), _scaleToRestore)) {
                CGRect unobscuredRect = UIEdgeInsetsInsetRect(self.bounds, _obscuredInsets);
                WebCore::FloatSize unobscuredContentSizeAtNewScale = WebCore::FloatSize(unobscuredRect.size) / _scaleToRestore;
                WebCore::FloatPoint topLeftInDocumentCoordinates = unobscuredCenterToRestore - unobscuredContentSizeAtNewScale / 2;

                topLeftInDocumentCoordinates.scale(_scaleToRestore);
                topLeftInDocumentCoordinates.moveBy(WebCore::FloatPoint(-_obscuredInsets.left, -_obscuredInsets.top));

                changeContentOffsetBoundedInValidRange(_scrollView.get(), topLeftInDocumentCoordinates);
            }

            needUpdateVisibleContentRects = true;
        }

        if (_gestureController)
            _gestureController->didRestoreScrollPosition();

        _firstTransactionIDAfterPageRestore = std::nullopt;
    }

    if (needUpdateVisibleContentRects)
        [self _scheduleVisibleContentRectUpdate];

    if (WebKit::RemoteLayerTreeScrollingPerformanceData* scrollPerfData = _page->scrollingPerformanceData())
        scrollPerfData->didCommitLayerTree([self visibleRectInViewCoordinates]);
}

- (void)_layerTreeCommitComplete
{
    _commitDidRestoreScrollPosition = NO;
}

- (void)_dynamicViewportUpdateChangedTargetToScale:(double)newScale position:(CGPoint)newScrollPosition nextValidLayerTreeTransactionID:(uint64_t)nextValidLayerTreeTransactionID
{
    LOG_WITH_STREAM(VisibleRects, stream << "-[WKWebView " << _page->pageID() << " _dynamicViewportUpdateChangedTargetToScale:] " <<  newScale << " _dynamicViewportUpdateMode " << (int)_dynamicViewportUpdateMode);

    if (_dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing) {
        CGFloat animatingScaleTarget = [[_resizeAnimationView layer] transform].m11;
        double currentTargetScale = animatingScaleTarget * [[_contentView layer] transform].m11;
        double scale = newScale / currentTargetScale;
        _resizeAnimationTransformAdjustments = CATransform3DMakeScale(scale, scale, 1);

        CGPoint newContentOffset = [self _adjustedContentOffset:CGPointMake(newScrollPosition.x * newScale, newScrollPosition.y * newScale)];
        CGPoint currentContentOffset = [_scrollView contentOffset];

        _resizeAnimationTransformAdjustments.m41 = (currentContentOffset.x - newContentOffset.x) / animatingScaleTarget;
        _resizeAnimationTransformAdjustments.m42 = (currentContentOffset.y - newContentOffset.y) / animatingScaleTarget;
        _resizeAnimationTransformTransactionID = nextValidLayerTreeTransactionID;
    }
}

- (void)_couldNotRestorePageState
{
    // The gestureController may be waiting for the scroll position to be restored
    // in order to remove the swipe snapshot. Since the scroll position could not be
    // restored, tell the gestureController it was restored so that it no longer waits
    // for it.
    if (_gestureController)
        _gestureController->didRestoreScrollPosition();
}

- (void)_restorePageScrollPosition:(std::optional<WebCore::FloatPoint>)scrollPosition scrollOrigin:(WebCore::FloatPoint)scrollOrigin previousObscuredInset:(WebCore::FloatBoxExtent)obscuredInsets scale:(double)scale
{
    if (_dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing)
        return;

    if (![self usesStandardContentView])
        return;

    _firstTransactionIDAfterPageRestore = downcast<WebKit::RemoteLayerTreeDrawingAreaProxy>(*_page->drawingArea()).nextLayerTreeTransactionID();
    if (scrollPosition)
        _scrollOffsetToRestore = WebCore::ScrollableArea::scrollOffsetFromPosition(WebCore::FloatPoint(scrollPosition.value()), WebCore::toFloatSize(scrollOrigin));
    else
        _scrollOffsetToRestore = std::nullopt;

    _obscuredInsetsWhenSaved = obscuredInsets;
    _scaleToRestore = scale;
}

- (void)_restorePageStateToUnobscuredCenter:(std::optional<WebCore::FloatPoint>)center scale:(double)scale
{
    if (_dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing)
        return;

    if (![self usesStandardContentView])
        return;

    _firstTransactionIDAfterPageRestore = downcast<WebKit::RemoteLayerTreeDrawingAreaProxy>(*_page->drawingArea()).nextLayerTreeTransactionID();
    _unobscuredCenterToRestore = center.value();

    _scaleToRestore = scale;
}

- (RefPtr<WebKit::ViewSnapshot>)_takeViewSnapshot
{
    float deviceScale = WebCore::screenScaleFactor();
    WebCore::FloatSize snapshotSize(self.bounds.size);
    snapshotSize.scale(deviceScale);

    CATransform3D transform = CATransform3DMakeScale(deviceScale, deviceScale, 1);

#if USE(IOSURFACE)
    WebCore::IOSurface::Format snapshotFormat = WebCore::screenSupportsExtendedColor() ? WebCore::IOSurface::Format::RGB10 : WebCore::IOSurface::Format::RGBA;
    auto surface = WebCore::IOSurface::create(WebCore::expandedIntSize(snapshotSize), WebCore::sRGBColorSpaceRef(), snapshotFormat);
    if (!surface)
        return nullptr;
    CARenderServerRenderLayerWithTransform(MACH_PORT_NULL, self.layer.context.contextId, reinterpret_cast<uint64_t>(self.layer), surface->surface(), 0, 0, &transform);

    WebCore::IOSurface::Format compressedFormat = WebCore::IOSurface::Format::YUV422;
    if (WebCore::IOSurface::allowConversionFromFormatToFormat(snapshotFormat, compressedFormat)) {
        RefPtr<WebKit::ViewSnapshot> viewSnapshot = WebKit::ViewSnapshot::create(nullptr);
        WebCore::IOSurface::convertToFormat(WTFMove(surface), WebCore::IOSurface::Format::YUV422, [viewSnapshot](std::unique_ptr<WebCore::IOSurface> convertedSurface) {
            if (convertedSurface)
                viewSnapshot->setSurface(WTFMove(convertedSurface));
        });

        return viewSnapshot;
    }

    return WebKit::ViewSnapshot::create(WTFMove(surface));
#else
    uint32_t slotID = [WebKit::ViewSnapshotStore::snapshottingContext() createImageSlot:snapshotSize hasAlpha:YES];

    if (!slotID)
        return nullptr;

    CARenderServerCaptureLayerWithTransform(MACH_PORT_NULL, self.layer.context.contextId, (uint64_t)self.layer, slotID, 0, 0, &transform);
    WebCore::IntSize imageSize = WebCore::expandedIntSize(WebCore::FloatSize(snapshotSize));
    return WebKit::ViewSnapshot::create(slotID, imageSize, (imageSize.area() * 4).unsafeGet());
#endif
}

- (void)_zoomToPoint:(WebCore::FloatPoint)point atScale:(double)scale animated:(BOOL)animated
{
    CFTimeInterval duration = 0;
    CGFloat zoomScale = contentZoomScale(self);

    if (animated) {
        const double maximumZoomDuration = 0.4;
        const double minimumZoomDuration = 0.1;
        const double zoomDurationFactor = 0.3;

        duration = std::min(fabs(log(zoomScale) - log(scale)) * zoomDurationFactor + minimumZoomDuration, maximumZoomDuration);
    }

    if (scale != zoomScale)
        _page->willStartUserTriggeredZooming();

    LOG_WITH_STREAM(VisibleRects, stream << "_zoomToPoint:" << point << " scale: " << scale << " duration:" << duration);

    [_scrollView _zoomToCenter:point scale:scale duration:duration];
}

- (void)_zoomToRect:(WebCore::FloatRect)targetRect atScale:(double)scale origin:(WebCore::FloatPoint)origin animated:(BOOL)animated
{
    // FIXME: Some of this could be shared with _scrollToRect.
    const double visibleRectScaleChange = contentZoomScale(self) / scale;
    const WebCore::FloatRect visibleRect([self convertRect:self.bounds toView:self._currentContentView]);
    const WebCore::FloatRect unobscuredRect([self _contentRectForUserInteraction]);

    const WebCore::FloatSize topLeftObscuredInsetAfterZoom((unobscuredRect.minXMinYCorner() - visibleRect.minXMinYCorner()) * visibleRectScaleChange);
    const WebCore::FloatSize bottomRightObscuredInsetAfterZoom((visibleRect.maxXMaxYCorner() - unobscuredRect.maxXMaxYCorner()) * visibleRectScaleChange);

    const WebCore::FloatSize unobscuredRectSizeAfterZoom(unobscuredRect.size() * visibleRectScaleChange);

    // Center to the target rect.
    WebCore::FloatPoint unobscuredRectLocationAfterZoom = targetRect.location() - (unobscuredRectSizeAfterZoom - targetRect.size()) * 0.5;

    // Center to the tap point instead in case the target rect won't fit in a direction.
    if (targetRect.width() > unobscuredRectSizeAfterZoom.width())
        unobscuredRectLocationAfterZoom.setX(origin.x() - unobscuredRectSizeAfterZoom.width() / 2);
    if (targetRect.height() > unobscuredRectSizeAfterZoom.height())
        unobscuredRectLocationAfterZoom.setY(origin.y() - unobscuredRectSizeAfterZoom.height() / 2);

    // We have computed where we want the unobscured rect to be. Now adjust for the obscuring insets.
    WebCore::FloatRect visibleRectAfterZoom(unobscuredRectLocationAfterZoom, unobscuredRectSizeAfterZoom);
    visibleRectAfterZoom.move(-topLeftObscuredInsetAfterZoom);
    visibleRectAfterZoom.expand(topLeftObscuredInsetAfterZoom + bottomRightObscuredInsetAfterZoom);

    [self _zoomToPoint:visibleRectAfterZoom.center() atScale:scale animated:animated];
}

static WebCore::FloatPoint constrainContentOffset(WebCore::FloatPoint contentOffset, WebCore::FloatSize contentSize, WebCore::FloatSize unobscuredContentSize)
{
    WebCore::FloatSize maximumContentOffset = contentSize - unobscuredContentSize;
    return contentOffset.constrainedBetween(WebCore::FloatPoint(), WebCore::FloatPoint(maximumContentOffset));
}

- (void)_scrollToContentScrollPosition:(WebCore::FloatPoint)scrollPosition scrollOrigin:(WebCore::IntPoint)scrollOrigin
{
    if (_commitDidRestoreScrollPosition || _dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing)
        return;

    WebCore::FloatPoint contentOffset = WebCore::ScrollableArea::scrollOffsetFromPosition(scrollPosition, toFloatSize(scrollOrigin));

    WebCore::FloatPoint scaledOffset = contentOffset;
    CGFloat zoomScale = contentZoomScale(self);
    scaledOffset.scale(zoomScale);

    CGPoint contentOffsetInScrollViewCoordinates = [self _adjustedContentOffset:scaledOffset];
    contentOffsetInScrollViewCoordinates = contentOffsetBoundedInValidRange(_scrollView.get(), contentOffsetInScrollViewCoordinates);

    [_scrollView _stopScrollingAndZoomingAnimations];

    if (!CGPointEqualToPoint(contentOffsetInScrollViewCoordinates, [_scrollView contentOffset]))
        [_scrollView setContentOffset:contentOffsetInScrollViewCoordinates];
    else {
        // If we haven't changed anything, there would not be any VisibleContentRect update sent to the content.
        // The WebProcess would keep the invalid contentOffset as its scroll position.
        // To synchronize the WebProcess with what is on screen, we send the VisibleContentRect again.
        _page->resendLastVisibleContentRects();
    }
}

- (BOOL)_scrollToRect:(WebCore::FloatRect)targetRect origin:(WebCore::FloatPoint)origin minimumScrollDistance:(float)minimumScrollDistance
{
    WebCore::FloatRect unobscuredContentRect([self _contentRectForUserInteraction]);
    WebCore::FloatPoint unobscuredContentOffset = unobscuredContentRect.location();
    WebCore::FloatSize contentSize([self._currentContentView bounds].size);

    // Center the target rect in the scroll view.
    // If the target doesn't fit in the scroll view, center on the gesture location instead.
    WebCore::FloatPoint newUnobscuredContentOffset;
    if (targetRect.width() <= unobscuredContentRect.width())
        newUnobscuredContentOffset.setX(targetRect.x() - (unobscuredContentRect.width() - targetRect.width()) / 2);
    else
        newUnobscuredContentOffset.setX(origin.x() - unobscuredContentRect.width() / 2);
    if (targetRect.height() <= unobscuredContentRect.height())
        newUnobscuredContentOffset.setY(targetRect.y() - (unobscuredContentRect.height() - targetRect.height()) / 2);
    else
        newUnobscuredContentOffset.setY(origin.y() - unobscuredContentRect.height() / 2);
    newUnobscuredContentOffset = constrainContentOffset(newUnobscuredContentOffset, contentSize, unobscuredContentRect.size());

    if (unobscuredContentOffset == newUnobscuredContentOffset) {
        if (targetRect.width() > unobscuredContentRect.width())
            newUnobscuredContentOffset.setX(origin.x() - unobscuredContentRect.width() / 2);
        if (targetRect.height() > unobscuredContentRect.height())
            newUnobscuredContentOffset.setY(origin.y() - unobscuredContentRect.height() / 2);
        newUnobscuredContentOffset = constrainContentOffset(newUnobscuredContentOffset, contentSize, unobscuredContentRect.size());
    }

    WebCore::FloatSize scrollViewOffsetDelta = newUnobscuredContentOffset - unobscuredContentOffset;
    scrollViewOffsetDelta.scale(contentZoomScale(self));

    float scrollDistance = scrollViewOffsetDelta.diagonalLength();
    if (scrollDistance < minimumScrollDistance)
        return false;

    [_contentView willStartZoomOrScroll];

    LOG_WITH_STREAM(VisibleRects, stream << "_scrollToRect: scrolling to " << [_scrollView contentOffset] + scrollViewOffsetDelta);

    [_scrollView setContentOffset:([_scrollView contentOffset] + scrollViewOffsetDelta) animated:YES];
    return true;
}

- (void)_scrollByContentOffset:(WebCore::FloatPoint)contentOffsetDelta
{
    WebCore::FloatPoint scaledOffsetDelta = contentOffsetDelta;
    CGFloat zoomScale = contentZoomScale(self);
    scaledOffsetDelta.scale(zoomScale);

    CGPoint currentOffset = [_scrollView _isAnimatingScroll] ? [_scrollView _animatedTargetOffset] : [_scrollView contentOffset];
    CGPoint boundedOffset = contentOffsetBoundedInValidRange(_scrollView.get(), currentOffset + scaledOffsetDelta);

    if (CGPointEqualToPoint(boundedOffset, currentOffset))
        return;
    [_contentView willStartZoomOrScroll];

    LOG_WITH_STREAM(VisibleRects, stream << "_scrollByContentOffset: scrolling to " << WebCore::FloatPoint(boundedOffset));

    [_scrollView setContentOffset:boundedOffset animated:YES];
}

- (void)_zoomOutWithOrigin:(WebCore::FloatPoint)origin animated:(BOOL)animated
{
    [self _zoomToPoint:origin atScale:[_scrollView minimumZoomScale] animated:animated];
}

- (void)_zoomToInitialScaleWithOrigin:(WebCore::FloatPoint)origin animated:(BOOL)animated
{
    ASSERT(_initialScaleFactor > 0);
    [self _zoomToPoint:origin atScale:_initialScaleFactor animated:animated];
}

// focusedElementRect and selectionRect are both in document coordinates.
- (void)_zoomToFocusRect:(WebCore::FloatRect)focusedElementRectInDocumentCoordinates selectionRect:(WebCore::FloatRect)selectionRectInDocumentCoordinates insideFixed:(BOOL)insideFixed
    fontSize:(float)fontSize minimumScale:(double)minimumScale maximumScale:(double)maximumScale allowScaling:(BOOL)allowScaling forceScroll:(BOOL)forceScroll
{
    LOG_WITH_STREAM(VisibleRects, stream << "_zoomToFocusRect:" << focusedElementRectInDocumentCoordinates << " selectionRect:" << selectionRectInDocumentCoordinates);
    UNUSED_PARAM(insideFixed);

    const double WKWebViewStandardFontSize = 16;
    const double kMinimumHeightToShowContentAboveKeyboard = 106;
    const CFTimeInterval UIWebFormAnimationDuration = 0.25;
    const double CaretOffsetFromWindowEdge = 20;

    // Zoom around the element's bounding frame. We use a "standard" size to determine the proper frame.
    double scale = allowScaling ? std::min(std::max(WKWebViewStandardFontSize / fontSize, minimumScale), maximumScale) : contentZoomScale(self);
    CGFloat documentWidth = [_contentView bounds].size.width;
    scale = CGRound(documentWidth * scale) / documentWidth;

    UIWindow *window = [_scrollView window];

    WebCore::FloatRect focusedElementRectInNewScale = focusedElementRectInDocumentCoordinates;
    focusedElementRectInNewScale.scale(scale);
    focusedElementRectInNewScale.moveBy([_contentView frame].origin);

    // Find the portion of the view that is visible on the screen.
    UIViewController *topViewController = [[[_scrollView _viewControllerForAncestor] _rootAncestorViewController] _viewControllerForSupportedInterfaceOrientations];
    UIView *fullScreenView = topViewController.view;
    if (!fullScreenView)
        fullScreenView = window;

    CGRect unobscuredScrollViewRectInWebViewCoordinates = UIEdgeInsetsInsetRect([self bounds], _obscuredInsets);
    CGRect visibleScrollViewBoundsInWebViewCoordinates = CGRectIntersection(unobscuredScrollViewRectInWebViewCoordinates, [fullScreenView convertRect:[fullScreenView bounds] toView:self]);
    CGRect formAssistantFrameInWebViewCoordinates = [window convertRect:_inputViewBounds toView:self];
    CGRect intersectionBetweenScrollViewAndFormAssistant = CGRectIntersection(visibleScrollViewBoundsInWebViewCoordinates, formAssistantFrameInWebViewCoordinates);
    CGSize visibleSize = visibleScrollViewBoundsInWebViewCoordinates.size;

    CGFloat visibleOffsetFromTop = 0;
    if (!CGRectIsEmpty(intersectionBetweenScrollViewAndFormAssistant)) {
        CGFloat heightVisibleAboveFormAssistant = CGRectGetMinY(intersectionBetweenScrollViewAndFormAssistant) - CGRectGetMinY(visibleScrollViewBoundsInWebViewCoordinates);
        CGFloat heightVisibleBelowFormAssistant = CGRectGetMaxY(visibleScrollViewBoundsInWebViewCoordinates) - CGRectGetMaxY(intersectionBetweenScrollViewAndFormAssistant);

        if (heightVisibleAboveFormAssistant >= kMinimumHeightToShowContentAboveKeyboard || heightVisibleBelowFormAssistant < heightVisibleAboveFormAssistant)
            visibleSize.height = heightVisibleAboveFormAssistant;
        else {
            visibleSize.height = heightVisibleBelowFormAssistant;
            visibleOffsetFromTop = CGRectGetMaxY(intersectionBetweenScrollViewAndFormAssistant) - CGRectGetMinY(visibleScrollViewBoundsInWebViewCoordinates);
        }
    }

    BOOL selectionRectIsNotNull = !selectionRectInDocumentCoordinates.isZero();
    if (!forceScroll) {
        CGRect currentlyVisibleRegionInWebViewCoordinates;
        currentlyVisibleRegionInWebViewCoordinates.origin = unobscuredScrollViewRectInWebViewCoordinates.origin;
        currentlyVisibleRegionInWebViewCoordinates.origin.y += visibleOffsetFromTop;
        currentlyVisibleRegionInWebViewCoordinates.size = visibleSize;

        // Don't bother scrolling if the entire node is already visible, whether or not we got a selectionRect.
        if (CGRectContainsRect(currentlyVisibleRegionInWebViewCoordinates, [self convertRect:focusedElementRectInDocumentCoordinates fromView:_contentView.get()]))
            return;

        // Don't bother scrolling if we have a valid selectionRect and it is already visible.
        if (selectionRectIsNotNull && CGRectContainsRect(currentlyVisibleRegionInWebViewCoordinates, [self convertRect:selectionRectInDocumentCoordinates fromView:_contentView.get()]))
            return;
    }

    // We want to zoom to the left/top corner of the DOM node, with as much spacing on all sides as we
    // can get based on the visible area after zooming (workingFrame).  The spacing in either dimension is half the
    // difference between the size of the DOM node and the size of the visible frame.
    CGFloat horizontalSpaceInWebViewCoordinates = std::max((visibleSize.width - focusedElementRectInNewScale.width()) / 2.0, 0.0);
    CGFloat verticalSpaceInWebViewCoordinates = std::max((visibleSize.height - focusedElementRectInNewScale.height()) / 2.0, 0.0);

    CGPoint topLeft;
    topLeft.x = focusedElementRectInNewScale.x() - horizontalSpaceInWebViewCoordinates;
    topLeft.y = focusedElementRectInNewScale.y() - verticalSpaceInWebViewCoordinates - visibleOffsetFromTop;

    CGFloat minimumAllowableHorizontalOffsetInWebViewCoordinates = -INFINITY;
    CGFloat minimumAllowableVerticalOffsetInWebViewCoordinates = -INFINITY;
    if (selectionRectIsNotNull) {
        WebCore::FloatRect selectionRectInNewScale = selectionRectInDocumentCoordinates;
        selectionRectInNewScale.scale(scale);
        selectionRectInNewScale.moveBy([_contentView frame].origin);
        minimumAllowableHorizontalOffsetInWebViewCoordinates = CGRectGetMaxX(selectionRectInNewScale) + CaretOffsetFromWindowEdge - visibleSize.width;
        minimumAllowableVerticalOffsetInWebViewCoordinates = CGRectGetMaxY(selectionRectInNewScale) + CaretOffsetFromWindowEdge - visibleSize.height - visibleOffsetFromTop;
    }

    WebCore::FloatRect documentBoundsInNewScale = [_contentView bounds];
    documentBoundsInNewScale.scale(scale);
    documentBoundsInNewScale.moveBy([_contentView frame].origin);

    // Constrain the left edge in document coordinates so that:
    //  - it isn't so small that the scrollVisibleRect isn't visible on the screen
    //  - it isn't so great that the document's right edge is less than the right edge of the screen
    if (selectionRectIsNotNull && topLeft.x < minimumAllowableHorizontalOffsetInWebViewCoordinates)
        topLeft.x = minimumAllowableHorizontalOffsetInWebViewCoordinates;
    else {
        CGFloat maximumAllowableHorizontalOffset = CGRectGetMaxX(documentBoundsInNewScale) - visibleSize.width;
        if (topLeft.x > maximumAllowableHorizontalOffset)
            topLeft.x = maximumAllowableHorizontalOffset;
    }

    // Constrain the top edge in document coordinates so that:
    //  - it isn't so small that the scrollVisibleRect isn't visible on the screen
    //  - it isn't so great that the document's bottom edge is higher than the top of the form assistant
    if (selectionRectIsNotNull && topLeft.y < minimumAllowableVerticalOffsetInWebViewCoordinates)
        topLeft.y = minimumAllowableVerticalOffsetInWebViewCoordinates;
    else {
        CGFloat maximumAllowableVerticalOffset = CGRectGetMaxY(documentBoundsInNewScale) - visibleSize.height;
        if (topLeft.y > maximumAllowableVerticalOffset)
            topLeft.y = maximumAllowableVerticalOffset;
    }

    WebCore::FloatPoint newCenter = CGPointMake(topLeft.x + unobscuredScrollViewRectInWebViewCoordinates.size.width / 2.0, topLeft.y + unobscuredScrollViewRectInWebViewCoordinates.size.height / 2.0);

    if (scale != contentZoomScale(self))
        _page->willStartUserTriggeredZooming();

    LOG_WITH_STREAM(VisibleRects, stream << "_zoomToFocusRect: zooming to " << newCenter << " scale:" << scale);

    // The newCenter has been computed in the new scale, but _zoomToCenter expected the center to be in the original scale.
    newCenter.scale(1 / scale);
    [_scrollView _zoomToCenter:newCenter
                        scale:scale
                     duration:UIWebFormAnimationDuration
                        force:YES];
}

- (CGFloat)_targetContentZoomScaleForRect:(const WebCore::FloatRect&)targetRect currentScale:(double)currentScale fitEntireRect:(BOOL)fitEntireRect minimumScale:(double)minimumScale maximumScale:(double)maximumScale
{
    WebCore::FloatSize unobscuredContentSize([self _contentRectForUserInteraction].size);
    double horizontalScale = unobscuredContentSize.width() * currentScale / targetRect.width();
    double verticalScale = unobscuredContentSize.height() * currentScale / targetRect.height();

    horizontalScale = std::min(std::max(horizontalScale, minimumScale), maximumScale);
    verticalScale = std::min(std::max(verticalScale, minimumScale), maximumScale);

    return fitEntireRect ? std::min(horizontalScale, verticalScale) : horizontalScale;
}

- (BOOL)_zoomToRect:(WebCore::FloatRect)targetRect withOrigin:(WebCore::FloatPoint)origin fitEntireRect:(BOOL)fitEntireRect minimumScale:(double)minimumScale maximumScale:(double)maximumScale minimumScrollDistance:(float)minimumScrollDistance
{
    const float maximumScaleFactorDeltaForPanScroll = 0.02;

    double currentScale = contentZoomScale(self);
    double targetScale = [self _targetContentZoomScaleForRect:targetRect currentScale:currentScale fitEntireRect:fitEntireRect minimumScale:minimumScale maximumScale:maximumScale];

    if (fabs(targetScale - currentScale) < maximumScaleFactorDeltaForPanScroll) {
        if ([self _scrollToRect:targetRect origin:origin minimumScrollDistance:minimumScrollDistance])
            return true;
    } else if (targetScale != currentScale) {
        [self _zoomToRect:targetRect atScale:targetScale origin:origin animated:YES];
        return true;
    }

    return false;
}

- (void)didMoveToWindow
{
    _page->activityStateDidChange(WebCore::ActivityState::AllFlags);
}

- (void)setOpaque:(BOOL)opaque
{
    BOOL oldOpaque = self.opaque;

    [super setOpaque:opaque];
    [_contentView setOpaque:opaque];

    if (oldOpaque == opaque)
        return;

    if (!_page)
        return;

    _page->setDrawsBackground(opaque);
    [self _updateScrollViewBackground];
}

- (void)setBackgroundColor:(UIColor *)backgroundColor
{
    [super setBackgroundColor:backgroundColor];
    [_contentView setBackgroundColor:backgroundColor];
}

- (BOOL)_allowsDoubleTapGestures
{
    if (_fastClickingIsDisabled)
        return YES;

    // If the page is not user scalable, we don't allow double tap gestures.
    if (![_scrollView isZoomEnabled] || [_scrollView minimumZoomScale] >= [_scrollView maximumZoomScale])
        return NO;

    // If the viewport width was not explicit, we allow double tap gestures.
    if (!_viewportMetaTagWidthWasExplicit || _viewportMetaTagCameFromImageDocument)
        return YES;

    // If the page set a viewport width that wasn't the device width, then it was
    // scaled and thus will probably need to zoom.
    if (_viewportMetaTagWidth != WebCore::ViewportArguments::ValueDeviceWidth)
        return YES;

    // At this point, we have a page that asked for width = device-width. However,
    // if the content's width and height were large, we might have had to shrink it.
    // Since we'll enable double tap zoom whenever we're not at the actual
    // initial scale, this simply becomes a test of the current scale against 1.
    return !areEssentiallyEqualAsFloat(contentZoomScale(self), 1);
}

#pragma mark - UIScrollViewDelegate

- (BOOL)usesStandardContentView
{
    return !_customContentView && !_passwordView;
}

- (CGSize)scrollView:(UIScrollView*)scrollView contentSizeForZoomScale:(CGFloat)scale withProposedSize:(CGSize)proposedSize
{
    return roundScrollViewContentSize(*_page, proposedSize);
}

- (UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView
{
    ASSERT(_scrollView == scrollView);

    if (_customContentView)
        return _customContentView.get();

    return _contentView.get();
}

- (void)scrollViewWillBeginZooming:(UIScrollView *)scrollView withView:(UIView *)view
{
    if (![self usesStandardContentView])
        return;

    if (scrollView.pinchGestureRecognizer.state == UIGestureRecognizerStateBegan) {
        _page->willStartUserTriggeredZooming();
        [_contentView scrollViewWillStartPanOrPinchGesture];
    }
    [_contentView willStartZoomOrScroll];
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView
{
    if (![self usesStandardContentView])
        return;

    if (scrollView.panGestureRecognizer.state == UIGestureRecognizerStateBegan)
        [_contentView scrollViewWillStartPanOrPinchGesture];

    [_contentView willStartZoomOrScroll];
#if ENABLE(CSS_SCROLL_SNAP) && ENABLE(ASYNC_SCROLLING)
    // FIXME: We will want to detect whether snapping will occur before beginning to drag. See WebPageProxy::didCommitLayerTree.
    WebKit::RemoteScrollingCoordinatorProxy* coordinator = _page->scrollingCoordinatorProxy();
    ASSERT(scrollView == _scrollView.get());
    CGFloat scrollDecelerationFactor = (coordinator && coordinator->shouldSetScrollViewDecelerationRateFast()) ? UIScrollViewDecelerationRateFast : UIScrollViewDecelerationRateNormal;
    scrollView.horizontalScrollDecelerationFactor = scrollDecelerationFactor;
    scrollView.verticalScrollDecelerationFactor = scrollDecelerationFactor;
#endif
}

- (void)_didFinishScrolling
{
    if (![self usesStandardContentView])
        return;

    [self _scheduleVisibleContentRectUpdate];
    [_contentView didFinishScrolling];
}

- (void)scrollViewWillEndDragging:(UIScrollView *)scrollView withVelocity:(CGPoint)velocity targetContentOffset:(inout CGPoint *)targetContentOffset
{
    // Work around <rdar://problem/16374753> by avoiding deceleration while
    // zooming. We'll animate to the right place once the zoom finishes.
    if ([scrollView isZooming])
        *targetContentOffset = [scrollView contentOffset];
#if ENABLE(CSS_SCROLL_SNAP) && ENABLE(ASYNC_SCROLLING)
    if (WebKit::RemoteScrollingCoordinatorProxy* coordinator = _page->scrollingCoordinatorProxy()) {
        // FIXME: Here, I'm finding the maximum horizontal/vertical scroll offsets. There's probably a better way to do this.
        CGSize maxScrollOffsets = CGSizeMake(scrollView.contentSize.width - scrollView.bounds.size.width, scrollView.contentSize.height - scrollView.bounds.size.height);

        CGRect fullViewRect = self.bounds;

        UIEdgeInsets contentInset;

        id<WKUIDelegatePrivate> uiDelegatePrivate = static_cast<id <WKUIDelegatePrivate>>([self UIDelegate]);
        if ([uiDelegatePrivate respondsToSelector:@selector(_webView:finalObscuredInsetsForScrollView:withVelocity:targetContentOffset:)])
            contentInset = [uiDelegatePrivate _webView:self finalObscuredInsetsForScrollView:scrollView withVelocity:velocity targetContentOffset:targetContentOffset];
        else
            contentInset = [self _computedContentInset];

        CGRect unobscuredRect = UIEdgeInsetsInsetRect(fullViewRect, contentInset);

        coordinator->adjustTargetContentOffsetForSnapping(maxScrollOffsets, velocity, unobscuredRect.origin.y, targetContentOffset);
    }
#endif
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate
{
    // If we're decelerating, scroll offset will be updated when scrollViewDidFinishDecelerating: is called.
    if (!decelerate)
        [self _didFinishScrolling];
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView
{
    [self _didFinishScrolling];
}

- (void)scrollViewDidScrollToTop:(UIScrollView *)scrollView
{
    [self _didFinishScrolling];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    if (![self usesStandardContentView])
        [_customContentView scrollViewDidScroll:(UIScrollView *)scrollView];

    [self _scheduleVisibleContentRectUpdateAfterScrollInView:scrollView];

    if (WebKit::RemoteLayerTreeScrollingPerformanceData* scrollPerfData = _page->scrollingPerformanceData())
        scrollPerfData->didScroll([self visibleRectInViewCoordinates]);
}

- (void)scrollViewDidZoom:(UIScrollView *)scrollView
{
    [self _updateScrollViewBackground];
    [self _scheduleVisibleContentRectUpdateAfterScrollInView:scrollView];
}

- (void)scrollViewDidEndZooming:(UIScrollView *)scrollView withView:(UIView *)view atScale:(CGFloat)scale
{
    ASSERT(scrollView == _scrollView);
    // FIXME: remove when rdar://problem/36065495 is fixed.
    // When rotating with two fingers down, UIScrollView can set a bogus content view position.
    // "Center" is top left because we set the anchorPoint to 0,0.
    [_contentView setCenter:self.bounds.origin];

    [self _scheduleVisibleContentRectUpdateAfterScrollInView:scrollView];
    [_contentView didZoomToScale:scale];
}

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView
{
    [self _didFinishScrolling];
}

- (void)_scrollViewDidInterruptDecelerating:(UIScrollView *)scrollView
{
    if (![self usesStandardContentView])
        return;

    [_contentView didInterruptScrolling];
    [self _scheduleVisibleContentRectUpdateAfterScrollInView:scrollView];
}

- (UIView *)_enclosingViewForExposedRectComputation
{
    return [self _scroller];
}

- (CGRect)_visibleRectInEnclosingView:(UIView *)enclosingView
{
    if (!enclosingView)
        return self.bounds;

    CGRect exposedRect = [enclosingView convertRect:enclosingView.bounds toView:self];
    return CGRectIntersectsRect(exposedRect, self.bounds) ? CGRectIntersection(exposedRect, self.bounds) : CGRectZero;
}

- (CGRect)_visibleContentRect
{
    if (_frozenVisibleContentRect)
        return _frozenVisibleContentRect.value();

    CGRect visibleRectInContentCoordinates = [self convertRect:self.bounds toView:_contentView.get()];

    if (UIView *enclosingView = [self _enclosingViewForExposedRectComputation]) {
        CGRect viewVisibleRect = [self _visibleRectInEnclosingView:enclosingView];
        CGRect viewVisibleContentRect = [self convertRect:viewVisibleRect toView:_contentView.get()];
        visibleRectInContentCoordinates = CGRectIntersection(visibleRectInContentCoordinates, viewVisibleContentRect);
    }

    return visibleRectInContentCoordinates;
}

// Called when some ancestor UIScrollView scrolls.
- (void)_didScroll
{
    [self _scheduleVisibleContentRectUpdateAfterScrollInView:[self _scroller]];

    const NSTimeInterval ScrollingEndedTimerInterval = 0.032;
    if (!_enclosingScrollViewScrollTimer) {
        _enclosingScrollViewScrollTimer = adoptNS([[NSTimer alloc] initWithFireDate:[NSDate dateWithTimeIntervalSinceNow:ScrollingEndedTimerInterval]
            interval:0 target:self selector:@selector(_enclosingScrollerScrollingEnded:) userInfo:nil repeats:YES]);
        [[NSRunLoop mainRunLoop] addTimer:_enclosingScrollViewScrollTimer.get() forMode:NSDefaultRunLoopMode];
    }
    _didScrollSinceLastTimerFire = YES;
}

- (void)_enclosingScrollerScrollingEnded:(NSTimer *)timer
{
    if (_didScrollSinceLastTimerFire) {
        _didScrollSinceLastTimerFire = NO;
        return;
    }

    [self _scheduleVisibleContentRectUpdate];
    [_enclosingScrollViewScrollTimer invalidate];
    _enclosingScrollViewScrollTimer = nil;
}

- (UIEdgeInsets)_scrollViewSystemContentInset
{
    // It's not safe to access the scroll view's safeAreaInsets or _systemContentInset from
    // inside our layoutSubviews implementation, because they aren't updated until afterwards.
    // Instead, depend on the fact that the UIScrollView and WKWebView are in the same coordinate
    // space, and map the WKWebView's own insets into the scroll view manually.
    return UIEdgeInsetsAdd([_scrollView _contentScrollInset], self.safeAreaInsets, [_scrollView _edgesApplyingSafeAreaInsetsToContentInset]);
}

static WebCore::FloatSize activeMinimumLayoutSize(WKWebView *webView, const CGRect& bounds)
{
    if (webView->_overridesMinimumLayoutSize)
        return WebCore::FloatSize(webView->_minimumLayoutSizeOverride);

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
    return WebCore::FloatSize(UIEdgeInsetsInsetRect(CGRectMake(0, 0, bounds.size.width, bounds.size.height), webView._scrollViewSystemContentInset).size);
#else
    return WebCore::FloatSize(bounds.size);
#endif
}

- (void)_dispatchSetMinimumLayoutSize:(WebCore::FloatSize)minimumLayoutSize
{
    if (_lastSentMinimumLayoutSize && CGSizeEqualToSize(_lastSentMinimumLayoutSize.value(), minimumLayoutSize))
        return;

    LOG_WITH_STREAM(VisibleRects, stream << "-[WKWebView " << _page->pageID() << " _dispatchSetMinimumLayoutSize:] " << minimumLayoutSize << " contentZoomScale " << contentZoomScale(self));
    _page->setViewportConfigurationMinimumLayoutSize(minimumLayoutSize);
    _lastSentMinimumLayoutSize = minimumLayoutSize;
}

- (void)_dispatchSetMaximumUnobscuredSize:(WebCore::FloatSize)maximumUnobscuredSize
{
    if (_lastSentMaximumUnobscuredSize && CGSizeEqualToSize(_lastSentMaximumUnobscuredSize.value(), maximumUnobscuredSize))
        return;

    _page->setMaximumUnobscuredSize(maximumUnobscuredSize);
    _lastSentMaximumUnobscuredSize = maximumUnobscuredSize;
}

- (void)_dispatchSetDeviceOrientation:(int32_t)deviceOrientation
{
    if (_lastSentDeviceOrientation && _lastSentDeviceOrientation.value() == deviceOrientation)
        return;

    _page->setDeviceOrientation(deviceOrientation);
    _lastSentDeviceOrientation = deviceOrientation;
}

- (void)_frameOrBoundsChanged
{
    CGRect bounds = self.bounds;
    [_scrollView setFrame:bounds];

    if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::NotResizing) {
        if (!_overridesMinimumLayoutSize)
            [self _dispatchSetMinimumLayoutSize:activeMinimumLayoutSize(self, self.bounds)];
        if (!_overridesMaximumUnobscuredSize)
            [self _dispatchSetMaximumUnobscuredSize:WebCore::FloatSize(bounds.size)];

        BOOL sizeChanged = NO;
        if (auto drawingArea = _page->drawingArea())
            sizeChanged = drawingArea->setSize(WebCore::IntSize(bounds.size));

        if (sizeChanged & [self usesStandardContentView])
            [_contentView setSizeChangedSinceLastVisibleContentRectUpdate:YES];
    }

    [_customContentView web_setMinimumSize:bounds.size];
    [self _scheduleVisibleContentRectUpdate];
}

// Unobscured content rect where the user can interact. When the keyboard is up, this should be the area above or below the keyboard, wherever there is enough space.
- (CGRect)_contentRectForUserInteraction
{
    // FIXME: handle split keyboard.
    UIEdgeInsets obscuredInsets = _obscuredInsets;
    obscuredInsets.bottom = std::max(_obscuredInsets.bottom, _inputViewBounds.size.height);
    CGRect unobscuredRect = UIEdgeInsetsInsetRect(self.bounds, obscuredInsets);
    return [self convertRect:unobscuredRect toView:self._currentContentView];
}

// Ideally UIScrollView would expose this for us: <rdar://problem/21394567>.
- (BOOL)_scrollViewIsRubberBanding
{
    float deviceScaleFactor = _page->deviceScaleFactor();

    CGPoint contentOffset = [_scrollView contentOffset];
    CGPoint boundedOffset = contentOffsetBoundedInValidRange(_scrollView.get(), contentOffset);
    return !pointsEqualInDevicePixels(contentOffset, boundedOffset, deviceScaleFactor);
}

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
- (void)safeAreaInsetsDidChange
{
    [super safeAreaInsetsDidChange];

    [self _scheduleVisibleContentRectUpdate];
}
#endif

- (void)_scheduleVisibleContentRectUpdate
{
    // For visible rect updates not associated with a specific UIScrollView, just consider our own scroller.
    [self _scheduleVisibleContentRectUpdateAfterScrollInView:_scrollView.get()];
}

- (BOOL)_scrollViewIsInStableState:(UIScrollView *)scrollView
{
    BOOL isStableState = !([scrollView isDragging] || [scrollView isDecelerating] || [scrollView isZooming] || [scrollView _isAnimatingZoom] || [scrollView _isScrollingToTop]);

    if (isStableState && scrollView == _scrollView.get())
        isStableState = !_isChangingObscuredInsetsInteractively;

    if (isStableState && scrollView == _scrollView.get())
        isStableState = ![self _scrollViewIsRubberBanding];

    if (isStableState)
        isStableState = !scrollView._isInterruptingDeceleration;

    if (NSNumber *stableOverride = self._stableStateOverride)
        isStableState = stableOverride.boolValue;

    return isStableState;
}

- (void)_addUpdateVisibleContentRectPreCommitHandler
{
    auto retainedSelf = retainPtr(self);
    [CATransaction addCommitHandler:[retainedSelf] {
        WKWebView *webView = retainedSelf.get();
        if (![webView _isValid])
            return;
        [webView _updateVisibleContentRects];
        webView->_hasScheduledVisibleRectUpdate = NO;
    } forPhase:kCATransactionPhasePreCommit];
}

- (void)_scheduleVisibleContentRectUpdateAfterScrollInView:(UIScrollView *)scrollView
{
    _visibleContentRectUpdateScheduledFromScrollViewInStableState = [self _scrollViewIsInStableState:scrollView];

    if (_hasScheduledVisibleRectUpdate)
        return;

    _hasScheduledVisibleRectUpdate = YES;

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
    CATransactionPhase transactionPhase = [CATransaction currentPhase];
    if (transactionPhase == kCATransactionPhaseNull || transactionPhase == kCATransactionPhasePreLayout) {
        [self _addUpdateVisibleContentRectPreCommitHandler];
        return;
    }
#endif

    dispatch_async(dispatch_get_main_queue(), [retainedSelf = retainPtr(self)] {
        WKWebView *webView = retainedSelf.get();
        if (![webView _isValid])
            return;
        [webView _addUpdateVisibleContentRectPreCommitHandler];
    });
}

static bool scrollViewCanScroll(UIScrollView *scrollView)
{
    if (!scrollView)
        return NO;

    UIEdgeInsets contentInset = scrollView.contentInset;
    CGSize contentSize = scrollView.contentSize;
    CGSize boundsSize = scrollView.bounds.size;

    return (contentSize.width + contentInset.left + contentInset.right) > boundsSize.width
        || (contentSize.height + contentInset.top + contentInset.bottom) > boundsSize.height;
}

- (CGRect)_contentBoundsExtendedForRubberbandingWithScale:(CGFloat)scaleFactor
{
    CGPoint contentOffset = [_scrollView contentOffset];
    CGPoint boundedOffset = contentOffsetBoundedInValidRange(_scrollView.get(), contentOffset);

    CGFloat horiontalRubberbandAmountInContentCoordinates = (contentOffset.x - boundedOffset.x) / scaleFactor;
    CGFloat verticalRubberbandAmountInContentCoordinates = (contentOffset.y - boundedOffset.y) / scaleFactor;

    CGRect extendedBounds = [_contentView bounds];

    if (horiontalRubberbandAmountInContentCoordinates < 0) {
        extendedBounds.origin.x += horiontalRubberbandAmountInContentCoordinates;
        extendedBounds.size.width -= horiontalRubberbandAmountInContentCoordinates;
    } else if (horiontalRubberbandAmountInContentCoordinates > 0)
        extendedBounds.size.width += horiontalRubberbandAmountInContentCoordinates;

    if (verticalRubberbandAmountInContentCoordinates < 0) {
        extendedBounds.origin.y += verticalRubberbandAmountInContentCoordinates;
        extendedBounds.size.height -= verticalRubberbandAmountInContentCoordinates;
    } else if (verticalRubberbandAmountInContentCoordinates > 0)
        extendedBounds.size.height += verticalRubberbandAmountInContentCoordinates;

    return extendedBounds;
}

- (void)_updateVisibleContentRects
{
    BOOL inStableState = _visibleContentRectUpdateScheduledFromScrollViewInStableState;

    if (![self usesStandardContentView]) {
        [_passwordView setFrame:self.bounds];
        [_customContentView web_computedContentInsetDidChange];
        return;
    }

    if (_delayUpdateVisibleContentRects) {
        _hadDelayedUpdateVisibleContentRects = YES;
        return;
    }

    if (_dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing
        || (_needsResetViewStateAfterCommitLoadForMainFrame && ![_contentView sizeChangedSinceLastVisibleContentRectUpdate])
        || [_scrollView isZoomBouncing]
        || _currentlyAdjustingScrollViewInsetsForKeyboard)
        return;

    CGRect fullViewRect = self.bounds;
    CGRect visibleRectInContentCoordinates = [self _visibleContentRect];

    UIEdgeInsets computedContentInsetUnadjustedForKeyboard = [self _computedContentInset];
    if (!_haveSetObscuredInsets)
        computedContentInsetUnadjustedForKeyboard.bottom -= _totalScrollViewBottomInsetAdjustmentForKeyboard;

    CGFloat scaleFactor = contentZoomScale(self);

    CGRect unobscuredRect = UIEdgeInsetsInsetRect(fullViewRect, computedContentInsetUnadjustedForKeyboard);
    CGRect unobscuredRectInContentCoordinates = _frozenUnobscuredContentRect ? _frozenUnobscuredContentRect.value() : [self convertRect:unobscuredRect toView:_contentView.get()];
    unobscuredRectInContentCoordinates = CGRectIntersection(unobscuredRectInContentCoordinates, [self _contentBoundsExtendedForRubberbandingWithScale:scaleFactor]);

#if ENABLE(CSS_SCROLL_SNAP) && ENABLE(ASYNC_SCROLLING)
    if (inStableState) {
        WebKit::RemoteScrollingCoordinatorProxy* coordinator = _page->scrollingCoordinatorProxy();
        if (coordinator && coordinator->hasActiveSnapPoint()) {
            CGRect unobscuredRect = UIEdgeInsetsInsetRect(fullViewRect, computedContentInsetUnadjustedForKeyboard);

            CGPoint currentPoint = [_scrollView contentOffset];
            CGPoint activePoint = coordinator->nearestActiveContentInsetAdjustedSnapPoint(unobscuredRect.origin.y, currentPoint);

            if (!CGPointEqualToPoint(activePoint, currentPoint)) {
                RetainPtr<WKScrollView> strongScrollView = _scrollView;
                dispatch_async(dispatch_get_main_queue(), [strongScrollView, activePoint] {
                    [strongScrollView setContentOffset:activePoint animated:NO];
                });
            }
        }
    }
#endif

    [_contentView didUpdateVisibleRect:visibleRectInContentCoordinates
        unobscuredRect:unobscuredRectInContentCoordinates
        unobscuredRectInScrollViewCoordinates:unobscuredRect
        obscuredInsets:_obscuredInsets
        unobscuredSafeAreaInsets:[self _computedUnobscuredSafeAreaInset]
        inputViewBounds:_inputViewBounds
        scale:scaleFactor minimumScale:[_scrollView minimumZoomScale]
        inStableState:inStableState
        isChangingObscuredInsetsInteractively:_isChangingObscuredInsetsInteractively
        enclosedInScrollableAncestorView:scrollViewCanScroll([self _scroller])];

    while (!_visibleContentRectUpdateCallbacks.isEmpty()) {
        auto callback = _visibleContentRectUpdateCallbacks.takeLast();
        callback();
    }
}

- (void)_didFinishLoadForMainFrame
{
    if (_gestureController)
        _gestureController->didFinishLoadForMainFrame();
}

- (void)_didFailLoadForMainFrame
{
    if (_gestureController)
        _gestureController->didFailLoadForMainFrame();
}

- (void)_didSameDocumentNavigationForMainFrame:(WebKit::SameDocumentNavigationType)navigationType
{
    [_customContentView web_didSameDocumentNavigation:toAPI(navigationType)];

    if (_gestureController)
        _gestureController->didSameDocumentNavigationForMainFrame(navigationType);
}

- (void)_keyboardChangedWithInfo:(NSDictionary *)keyboardInfo adjustScrollView:(BOOL)adjustScrollView
{
    NSValue *endFrameValue = [keyboardInfo objectForKey:UIKeyboardFrameEndUserInfoKey];
    if (!endFrameValue)
        return;

    // The keyboard rect is always in screen coordinates. In the view services case the window does not
    // have the interface orientation rotation transformation; its host does. So, it makes no sense to
    // clip the keyboard rect against its screen.
    if ([[self window] _isHostedInAnotherProcess])
        _inputViewBounds = [self.window convertRect:[endFrameValue CGRectValue] fromWindow:nil];
    else
        _inputViewBounds = [self.window convertRect:CGRectIntersection([endFrameValue CGRectValue], self.window.screen.bounds) fromWindow:nil];

    if (adjustScrollView) {
        CGFloat bottomInsetBeforeAdjustment = [_scrollView contentInset].bottom;
        SetForScope<BOOL> insetAdjustmentGuard(_currentlyAdjustingScrollViewInsetsForKeyboard, YES);
        [_scrollView _adjustForAutomaticKeyboardInfo:keyboardInfo animated:YES lastAdjustment:&_lastAdjustmentForScroller];
        CGFloat bottomInsetAfterAdjustment = [_scrollView contentInset].bottom;
        if (bottomInsetBeforeAdjustment != bottomInsetAfterAdjustment)
            _totalScrollViewBottomInsetAdjustmentForKeyboard += bottomInsetAfterAdjustment - bottomInsetBeforeAdjustment;
    }

    [self _scheduleVisibleContentRectUpdate];
}

- (BOOL)_shouldUpdateKeyboardWithInfo:(NSDictionary *)keyboardInfo
{
    if ([_contentView isAssistingNode])
        return YES;

    NSNumber *isLocalKeyboard = [keyboardInfo valueForKey:UIKeyboardIsLocalUserInfoKey];
    return isLocalKeyboard && !isLocalKeyboard.boolValue;
}

- (void)_keyboardWillChangeFrame:(NSNotification *)notification
{
    if ([self _shouldUpdateKeyboardWithInfo:notification.userInfo])
        [self _keyboardChangedWithInfo:notification.userInfo adjustScrollView:YES];
}

- (void)_keyboardDidChangeFrame:(NSNotification *)notification
{
    [self _keyboardChangedWithInfo:notification.userInfo adjustScrollView:NO];
}

- (void)_keyboardWillShow:(NSNotification *)notification
{
    if ([self _shouldUpdateKeyboardWithInfo:notification.userInfo])
        [self _keyboardChangedWithInfo:notification.userInfo adjustScrollView:YES];

    _page->setIsKeyboardAnimatingIn(true);
}

- (void)_keyboardDidShow:(NSNotification *)notification
{
    _page->setIsKeyboardAnimatingIn(false);
}

- (void)_keyboardWillHide:(NSNotification *)notification
{
    // Ignore keyboard will hide notifications sent during rotation. They're just there for
    // backwards compatibility reasons and processing the will hide notification would
    // temporarily screw up the unobscured view area.
    if ([[UIPeripheralHost sharedInstance] rotationState])
        return;

    [self _keyboardChangedWithInfo:notification.userInfo adjustScrollView:YES];
}

- (void)_windowDidRotate:(NSNotification *)notification
{
    if (!_overridesInterfaceOrientation)
        [self _dispatchSetDeviceOrientation:deviceOrientation()];
}

- (void)_contentSizeCategoryDidChange:(NSNotification *)notification
{
    _page->contentSizeCategoryDidChange([self _contentSizeCategory]);
}

- (NSString *)_contentSizeCategory
{
    return [[UIApplication sharedApplication] preferredContentSizeCategory];
}

- (void)_accessibilitySettingsDidChange:(NSNotification *)notification
{
    _page->accessibilitySettingsDidChange();
}

- (void)setAllowsBackForwardNavigationGestures:(BOOL)allowsBackForwardNavigationGestures
{
    if (_allowsBackForwardNavigationGestures == allowsBackForwardNavigationGestures)
        return;

    _allowsBackForwardNavigationGestures = allowsBackForwardNavigationGestures;

    if (allowsBackForwardNavigationGestures) {
        if (!_gestureController) {
            _gestureController = std::make_unique<WebKit::ViewGestureController>(*_page);
            _gestureController->installSwipeHandler(self, [self scrollView]);
            if (WKWebView *alternateWebView = [_configuration _alternateWebViewForNavigationGestures])
                _gestureController->setAlternateBackForwardListSourcePage(alternateWebView->_page.get());
        }
    } else
        _gestureController = nullptr;

    _page->setShouldRecordNavigationSnapshots(allowsBackForwardNavigationGestures);
}

- (BOOL)allowsBackForwardNavigationGestures
{
    return _allowsBackForwardNavigationGestures;
}

- (BOOL)_isNavigationSwipeGestureRecognizer:(UIGestureRecognizer *)recognizer
{
    if (!_gestureController)
        return NO;
    return _gestureController->isNavigationSwipeGestureRecognizer(recognizer);
}

- (void)_navigationGestureDidBegin
{
    // During a back/forward swipe, there's a view interposed between this view and the content view that has
    // an offset and animation on it, which results in computing incorrect rectangles. Work around by using
    // frozen rects during swipes.
    CGRect fullViewRect = self.bounds;
    CGRect unobscuredRect = UIEdgeInsetsInsetRect(fullViewRect, [self _computedContentInset]);

    _frozenVisibleContentRect = [self convertRect:fullViewRect toView:_contentView.get()];
    _frozenUnobscuredContentRect = [self convertRect:unobscuredRect toView:_contentView.get()];

    LOG_WITH_STREAM(VisibleRects, stream << "_navigationGestureDidBegin: freezing visibleContentRect " << WebCore::FloatRect(_frozenVisibleContentRect.value()) << " UnobscuredContentRect " << WebCore::FloatRect(_frozenUnobscuredContentRect.value()));
}

- (void)_navigationGestureDidEnd
{
    _frozenVisibleContentRect = std::nullopt;
    _frozenUnobscuredContentRect = std::nullopt;
}

- (void)_showPasswordViewWithDocumentName:(NSString *)documentName passwordHandler:(void (^)(NSString *))passwordHandler
{
    ASSERT(!_passwordView);
    _passwordView = adoptNS([[WKPasswordView alloc] initWithFrame:self.bounds documentName:documentName]);
    [_passwordView setUserDidEnterPassword:passwordHandler];
    [_passwordView showInScrollView:_scrollView.get()];
    self._currentContentView.hidden = YES;
}

- (void)_hidePasswordView
{
    if (!_passwordView)
        return;

    self._currentContentView.hidden = NO;
    [_passwordView hide];
    _passwordView = nil;
}

- (WKPasswordView *)_passwordView
{
    return _passwordView.get();
}

- (void)_updateScrollViewInsetAdjustmentBehavior
{
#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000
    if (![_scrollView _contentInsetAdjustmentBehaviorWasExternallyOverridden])
        [_scrollView _setContentInsetAdjustmentBehaviorInternal:self._safeAreaShouldAffectObscuredInsets ? UIScrollViewContentInsetAdjustmentAlways : UIScrollViewContentInsetAdjustmentNever];
#endif
}

- (void)_setAvoidsUnsafeArea:(BOOL)avoidsUnsafeArea
{
    if (_avoidsUnsafeArea == avoidsUnsafeArea)
        return;

    _avoidsUnsafeArea = avoidsUnsafeArea;

    [self _updateScrollViewInsetAdjustmentBehavior];
    [self _scheduleVisibleContentRectUpdate];

    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)[self UIDelegate];
    if ([uiDelegate respondsToSelector:@selector(_webView:didChangeSafeAreaShouldAffectObscuredInsets:)])
        [uiDelegate _webView:self didChangeSafeAreaShouldAffectObscuredInsets:avoidsUnsafeArea];
}

#endif // PLATFORM(IOS)

#pragma mark OS X-specific methods

#if PLATFORM(MAC)

- (BOOL)acceptsFirstResponder
{
    return _impl->acceptsFirstResponder();
}

- (BOOL)becomeFirstResponder
{
    return _impl->becomeFirstResponder();
}

- (BOOL)resignFirstResponder
{
    return _impl->resignFirstResponder();
}

- (void)viewWillStartLiveResize
{
    _impl->viewWillStartLiveResize();
}

- (void)viewDidEndLiveResize
{
    _impl->viewDidEndLiveResize();
}

- (BOOL)isFlipped
{
    return YES;
}

- (NSSize)intrinsicContentSize
{
    return NSSizeFromCGSize(_impl->intrinsicContentSize());
}

- (void)prepareContentInRect:(NSRect)rect
{
    _impl->prepareContentInRect(NSRectToCGRect(rect));
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];
    _impl->setFrameSize(NSSizeToCGSize(size));
}

- (void)renewGState
{
    _impl->renewGState();
    [super renewGState];
}

#define WEBCORE_COMMAND(command) - (void)command:(id)sender { _impl->executeEditCommandForSelector(_cmd); }

WEBCORE_COMMAND(alignCenter)
WEBCORE_COMMAND(alignJustified)
WEBCORE_COMMAND(alignLeft)
WEBCORE_COMMAND(alignRight)
WEBCORE_COMMAND(copy)
WEBCORE_COMMAND(cut)
WEBCORE_COMMAND(delete)
WEBCORE_COMMAND(deleteBackward)
WEBCORE_COMMAND(deleteBackwardByDecomposingPreviousCharacter)
WEBCORE_COMMAND(deleteForward)
WEBCORE_COMMAND(deleteToBeginningOfLine)
WEBCORE_COMMAND(deleteToBeginningOfParagraph)
WEBCORE_COMMAND(deleteToEndOfLine)
WEBCORE_COMMAND(deleteToEndOfParagraph)
WEBCORE_COMMAND(deleteToMark)
WEBCORE_COMMAND(deleteWordBackward)
WEBCORE_COMMAND(deleteWordForward)
WEBCORE_COMMAND(ignoreSpelling)
WEBCORE_COMMAND(indent)
WEBCORE_COMMAND(insertBacktab)
WEBCORE_COMMAND(insertLineBreak)
WEBCORE_COMMAND(insertNewline)
WEBCORE_COMMAND(insertNewlineIgnoringFieldEditor)
WEBCORE_COMMAND(insertParagraphSeparator)
WEBCORE_COMMAND(insertTab)
WEBCORE_COMMAND(insertTabIgnoringFieldEditor)
WEBCORE_COMMAND(makeTextWritingDirectionLeftToRight)
WEBCORE_COMMAND(makeTextWritingDirectionNatural)
WEBCORE_COMMAND(makeTextWritingDirectionRightToLeft)
WEBCORE_COMMAND(moveBackward)
WEBCORE_COMMAND(moveBackwardAndModifySelection)
WEBCORE_COMMAND(moveDown)
WEBCORE_COMMAND(moveDownAndModifySelection)
WEBCORE_COMMAND(moveForward)
WEBCORE_COMMAND(moveForwardAndModifySelection)
WEBCORE_COMMAND(moveLeft)
WEBCORE_COMMAND(moveLeftAndModifySelection)
WEBCORE_COMMAND(moveParagraphBackwardAndModifySelection)
WEBCORE_COMMAND(moveParagraphForwardAndModifySelection)
WEBCORE_COMMAND(moveRight)
WEBCORE_COMMAND(moveRightAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfDocument)
WEBCORE_COMMAND(moveToBeginningOfDocumentAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfLine)
WEBCORE_COMMAND(moveToBeginningOfLineAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfParagraph)
WEBCORE_COMMAND(moveToBeginningOfParagraphAndModifySelection)
WEBCORE_COMMAND(moveToBeginningOfSentence)
WEBCORE_COMMAND(moveToBeginningOfSentenceAndModifySelection)
WEBCORE_COMMAND(moveToEndOfDocument)
WEBCORE_COMMAND(moveToEndOfDocumentAndModifySelection)
WEBCORE_COMMAND(moveToEndOfLine)
WEBCORE_COMMAND(moveToEndOfLineAndModifySelection)
WEBCORE_COMMAND(moveToEndOfParagraph)
WEBCORE_COMMAND(moveToEndOfParagraphAndModifySelection)
WEBCORE_COMMAND(moveToEndOfSentence)
WEBCORE_COMMAND(moveToEndOfSentenceAndModifySelection)
WEBCORE_COMMAND(moveToLeftEndOfLine)
WEBCORE_COMMAND(moveToLeftEndOfLineAndModifySelection)
WEBCORE_COMMAND(moveToRightEndOfLine)
WEBCORE_COMMAND(moveToRightEndOfLineAndModifySelection)
WEBCORE_COMMAND(moveUp)
WEBCORE_COMMAND(moveUpAndModifySelection)
WEBCORE_COMMAND(moveWordBackward)
WEBCORE_COMMAND(moveWordBackwardAndModifySelection)
WEBCORE_COMMAND(moveWordForward)
WEBCORE_COMMAND(moveWordForwardAndModifySelection)
WEBCORE_COMMAND(moveWordLeft)
WEBCORE_COMMAND(moveWordLeftAndModifySelection)
WEBCORE_COMMAND(moveWordRight)
WEBCORE_COMMAND(moveWordRightAndModifySelection)
WEBCORE_COMMAND(outdent)
WEBCORE_COMMAND(pageDown)
WEBCORE_COMMAND(pageDownAndModifySelection)
WEBCORE_COMMAND(pageUp)
WEBCORE_COMMAND(pageUpAndModifySelection)
WEBCORE_COMMAND(paste)
WEBCORE_COMMAND(pasteAsPlainText)
WEBCORE_COMMAND(scrollPageDown)
WEBCORE_COMMAND(scrollPageUp)
WEBCORE_COMMAND(scrollLineDown)
WEBCORE_COMMAND(scrollLineUp)
WEBCORE_COMMAND(scrollToBeginningOfDocument)
WEBCORE_COMMAND(scrollToEndOfDocument)
WEBCORE_COMMAND(selectAll)
WEBCORE_COMMAND(selectLine)
WEBCORE_COMMAND(selectParagraph)
WEBCORE_COMMAND(selectSentence)
WEBCORE_COMMAND(selectToMark)
WEBCORE_COMMAND(selectWord)
WEBCORE_COMMAND(setMark)
WEBCORE_COMMAND(subscript)
WEBCORE_COMMAND(superscript)
WEBCORE_COMMAND(swapWithMark)
WEBCORE_COMMAND(takeFindStringFromSelection)
WEBCORE_COMMAND(transpose)
WEBCORE_COMMAND(underline)
WEBCORE_COMMAND(unscript)
WEBCORE_COMMAND(yank)
WEBCORE_COMMAND(yankAndSelect)

#undef WEBCORE_COMMAND

- (BOOL)writeSelectionToPasteboard:(NSPasteboard *)pasteboard types:(NSArray *)types
{
    return _impl->writeSelectionToPasteboard(pasteboard, types);
}

- (void)centerSelectionInVisibleArea:(id)sender
{
    _impl->centerSelectionInVisibleArea();
}

- (id)validRequestorForSendType:(NSString *)sendType returnType:(NSString *)returnType
{
    return _impl->validRequestorForSendAndReturnTypes(sendType, returnType);
}

- (BOOL)readSelectionFromPasteboard:(NSPasteboard *)pasteboard
{
    return _impl->readSelectionFromPasteboard(pasteboard);
}

- (void)changeFont:(id)sender
{
    _impl->changeFontFromFontPanel();
}

- (IBAction)startSpeaking:(id)sender
{
    _impl->startSpeaking();
}

- (IBAction)stopSpeaking:(id)sender
{
    _impl->stopSpeaking(sender);
}

- (IBAction)showGuessPanel:(id)sender
{
    _impl->showGuessPanel(sender);
}

- (IBAction)checkSpelling:(id)sender
{
    _impl->checkSpelling();
}

- (void)changeSpelling:(id)sender
{
    _impl->changeSpelling(sender);
}

- (IBAction)toggleContinuousSpellChecking:(id)sender
{
    _impl->toggleContinuousSpellChecking();
}

- (BOOL)isGrammarCheckingEnabled
{
    return _impl->isGrammarCheckingEnabled();
}

- (void)setGrammarCheckingEnabled:(BOOL)flag
{
    _impl->setGrammarCheckingEnabled(flag);
}

- (IBAction)toggleGrammarChecking:(id)sender
{
    _impl->toggleGrammarChecking();
}

- (IBAction)toggleAutomaticSpellingCorrection:(id)sender
{
    _impl->toggleAutomaticSpellingCorrection();
}

- (void)orderFrontSubstitutionsPanel:(id)sender
{
    _impl->orderFrontSubstitutionsPanel(sender);
}

- (IBAction)toggleSmartInsertDelete:(id)sender
{
    _impl->toggleSmartInsertDelete();
}

- (BOOL)isAutomaticQuoteSubstitutionEnabled
{
    return _impl->isAutomaticQuoteSubstitutionEnabled();
}

- (void)setAutomaticQuoteSubstitutionEnabled:(BOOL)flag
{
    _impl->setAutomaticQuoteSubstitutionEnabled(flag);
}

- (void)toggleAutomaticQuoteSubstitution:(id)sender
{
    _impl->toggleAutomaticQuoteSubstitution();
}

- (BOOL)isAutomaticDashSubstitutionEnabled
{
    return _impl->isAutomaticDashSubstitutionEnabled();
}

- (void)setAutomaticDashSubstitutionEnabled:(BOOL)flag
{
    _impl->setAutomaticDashSubstitutionEnabled(flag);
}

- (void)toggleAutomaticDashSubstitution:(id)sender
{
    _impl->toggleAutomaticDashSubstitution();
}

- (BOOL)isAutomaticLinkDetectionEnabled
{
    return _impl->isAutomaticLinkDetectionEnabled();
}

- (void)setAutomaticLinkDetectionEnabled:(BOOL)flag
{
    _impl->setAutomaticLinkDetectionEnabled(flag);
}

- (void)toggleAutomaticLinkDetection:(id)sender
{
    _impl->toggleAutomaticLinkDetection();
}

- (BOOL)isAutomaticTextReplacementEnabled
{
    return _impl->isAutomaticTextReplacementEnabled();
}

- (void)setAutomaticTextReplacementEnabled:(BOOL)flag
{
    _impl->setAutomaticTextReplacementEnabled(flag);
}

- (void)toggleAutomaticTextReplacement:(id)sender
{
    _impl->toggleAutomaticTextReplacement();
}

- (void)uppercaseWord:(id)sender
{
    _impl->uppercaseWord();
}

- (void)lowercaseWord:(id)sender
{
    _impl->lowercaseWord();
}

- (void)capitalizeWord:(id)sender
{
    _impl->capitalizeWord();
}

- (BOOL)_wantsKeyDownForEvent:(NSEvent *)event
{
    return _impl->wantsKeyDownForEvent(event);
}

- (void)scrollWheel:(NSEvent *)event
{
    _impl->scrollWheel(event);
}

- (void)swipeWithEvent:(NSEvent *)event
{
    _impl->swipeWithEvent(event);
}

- (void)mouseMoved:(NSEvent *)event
{
    _impl->mouseMoved(event);
}

- (void)mouseDown:(NSEvent *)event
{
    _impl->mouseDown(event);
}

- (void)mouseUp:(NSEvent *)event
{
    _impl->mouseUp(event);
}

- (void)mouseDragged:(NSEvent *)event
{
    _impl->mouseDragged(event);
}

- (void)mouseEntered:(NSEvent *)event
{
    _impl->mouseEntered(event);
}

- (void)mouseExited:(NSEvent *)event
{
    _impl->mouseExited(event);
}

- (void)otherMouseDown:(NSEvent *)event
{
    _impl->otherMouseDown(event);
}

- (void)otherMouseDragged:(NSEvent *)event
{
    _impl->otherMouseDragged(event);
}

- (void)otherMouseUp:(NSEvent *)event
{
    _impl->otherMouseUp(event);
}

- (void)rightMouseDown:(NSEvent *)event
{
    _impl->rightMouseDown(event);
}

- (void)rightMouseDragged:(NSEvent *)event
{
    _impl->rightMouseDragged(event);
}

- (void)rightMouseUp:(NSEvent *)event
{
    _impl->rightMouseUp(event);
}

- (void)pressureChangeWithEvent:(NSEvent *)event
{
    _impl->pressureChangeWithEvent(event);
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
    return _impl->acceptsFirstMouse(event);
}

- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent *)event
{
    return _impl->shouldDelayWindowOrderingForEvent(event);
}

- (void)doCommandBySelector:(SEL)selector
{
    _impl->doCommandBySelector(selector);
}

- (void)insertText:(id)string
{
    _impl->insertText(string);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
    _impl->insertText(string, replacementRange);
}

- (NSTextInputContext *)inputContext
{
    if (!_impl)
        return nil;
    return _impl->inputContext();
}

- (BOOL)performKeyEquivalent:(NSEvent *)event
{
    return _impl->performKeyEquivalent(event);
}

- (void)keyUp:(NSEvent *)theEvent
{
    _impl->keyUp(theEvent);
}

- (void)keyDown:(NSEvent *)theEvent
{
    _impl->keyDown(theEvent);
}

- (void)flagsChanged:(NSEvent *)theEvent
{
    _impl->flagsChanged(theEvent);
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)newSelectedRange replacementRange:(NSRange)replacementRange
{
    _impl->setMarkedText(string, newSelectedRange, replacementRange);
}

- (void)unmarkText
{
    _impl->unmarkText();
}

- (NSRange)selectedRange
{
    return _impl->selectedRange();
}

- (BOOL)hasMarkedText
{
    return _impl->hasMarkedText();
}

- (NSRange)markedRange
{
    return _impl->markedRange();
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)nsRange actualRange:(NSRangePointer)actualRange
{
    return _impl->attributedSubstringForProposedRange(nsRange, actualRange);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)thePoint
{
    return _impl->characterIndexForPoint(thePoint);
}

- (NSRect)firstRectForCharacterRange:(NSRange)theRange actualRange:(NSRangePointer)actualRange
{
    return _impl->firstRectForCharacterRange(theRange, actualRange);
}

- (void)selectedRangeWithCompletionHandler:(void(^)(NSRange selectedRange))completionHandlerPtr
{
    _impl->selectedRangeWithCompletionHandler(completionHandlerPtr);
}

- (void)markedRangeWithCompletionHandler:(void(^)(NSRange markedRange))completionHandlerPtr
{
    _impl->markedRangeWithCompletionHandler(completionHandlerPtr);
}

- (void)hasMarkedTextWithCompletionHandler:(void(^)(BOOL hasMarkedText))completionHandlerPtr
{
    _impl->hasMarkedTextWithCompletionHandler(completionHandlerPtr);
}

- (void)attributedSubstringForProposedRange:(NSRange)nsRange completionHandler:(void(^)(NSAttributedString *attrString, NSRange actualRange))completionHandlerPtr
{
    _impl->attributedSubstringForProposedRange(nsRange, completionHandlerPtr);
}

- (void)firstRectForCharacterRange:(NSRange)theRange completionHandler:(void(^)(NSRect firstRect, NSRange actualRange))completionHandlerPtr
{
    _impl->firstRectForCharacterRange(theRange, completionHandlerPtr);
}

- (void)characterIndexForPoint:(NSPoint)thePoint completionHandler:(void(^)(NSUInteger))completionHandlerPtr
{
    _impl->characterIndexForPoint(thePoint, completionHandlerPtr);
}

- (NSArray *)validAttributesForMarkedText
{
    return _impl->validAttributesForMarkedText();
}

#if ENABLE(DRAG_SUPPORT)
- (void)draggedImage:(NSImage *)image endedAt:(NSPoint)endPoint operation:(NSDragOperation)operation
{
    _impl->draggedImage(image, NSPointToCGPoint(endPoint), operation);
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)draggingInfo
{
    return _impl->draggingEntered(draggingInfo);
}

- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)draggingInfo
{
    return _impl->draggingUpdated(draggingInfo);
}

- (void)draggingExited:(id <NSDraggingInfo>)draggingInfo
{
    _impl->draggingExited(draggingInfo);
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    return _impl->prepareForDragOperation(draggingInfo);
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)draggingInfo
{
    return _impl->performDragOperation(draggingInfo);
}

- (NSView *)_hitTest:(NSPoint *)point dragTypes:(NSSet *)types
{
    return _impl->hitTestForDragTypes(NSPointToCGPoint(*point), types);
}
#endif // ENABLE(DRAG_SUPPORT)

- (BOOL)_windowResizeMouseLocationIsInVisibleScrollerThumb:(NSPoint)point
{
    return _impl->windowResizeMouseLocationIsInVisibleScrollerThumb(NSPointToCGPoint(point));
}

- (void)viewWillMoveToWindow:(NSWindow *)window
{
    _impl->viewWillMoveToWindow(window);
}

- (void)viewDidMoveToWindow
{
    _impl->viewDidMoveToWindow();
}

- (void)drawRect:(NSRect)rect
{
    _impl->drawRect(NSRectToCGRect(rect));
}

- (BOOL)isOpaque
{
    return _impl->isOpaque();
}

- (BOOL)mouseDownCanMoveWindow
{
    return WebKit::WebViewImpl::mouseDownCanMoveWindow();
}

- (void)viewDidHide
{
    _impl->viewDidHide();
}

- (void)viewDidUnhide
{
    _impl->viewDidUnhide();
}

- (void)viewDidChangeBackingProperties
{
    _impl->viewDidChangeBackingProperties();
}

- (void)_activeSpaceDidChange:(NSNotification *)notification
{
    _impl->activeSpaceDidChange();
}

- (id)accessibilityFocusedUIElement
{
    return _impl->accessibilityFocusedUIElement();
}

- (BOOL)accessibilityIsIgnored
{
    return _impl->accessibilityIsIgnored();
}

- (id)accessibilityHitTest:(NSPoint)point
{
    return _impl->accessibilityHitTest(NSPointToCGPoint(point));
}

- (id)accessibilityAttributeValue:(NSString *)attribute
{
    return _impl->accessibilityAttributeValue(attribute);
}

- (NSView *)hitTest:(NSPoint)point
{
    if (!_impl)
        return [super hitTest:point];
    return _impl->hitTest(NSPointToCGPoint(point));
}

- (NSInteger)conversationIdentifier
{
    return (NSInteger)self;
}

- (void)quickLookWithEvent:(NSEvent *)event
{
    _impl->quickLookWithEvent(event);
}

- (NSTrackingRectTag)addTrackingRect:(NSRect)rect owner:(id)owner userData:(void *)data assumeInside:(BOOL)assumeInside
{
    return _impl->addTrackingRect(NSRectToCGRect(rect), owner, data, assumeInside);
}

- (NSTrackingRectTag)_addTrackingRect:(NSRect)rect owner:(id)owner userData:(void *)data assumeInside:(BOOL)assumeInside useTrackingNum:(int)tag
{
    return _impl->addTrackingRectWithTrackingNum(NSRectToCGRect(rect), owner, data, assumeInside, tag);
}

- (void)_addTrackingRects:(NSRect *)rects owner:(id)owner userDataList:(void **)userDataList assumeInsideList:(BOOL *)assumeInsideList trackingNums:(NSTrackingRectTag *)trackingNums count:(int)count
{
    CGRect *cgRects = (CGRect *)calloc(1, sizeof(CGRect));
    for (int i = 0; i < count; i++)
        cgRects[i] = NSRectToCGRect(rects[i]);
    _impl->addTrackingRectsWithTrackingNums(cgRects, owner, userDataList, assumeInsideList, trackingNums, count);
    free(cgRects);
}

- (void)removeTrackingRect:(NSTrackingRectTag)tag
{
    if (!_impl)
        return;
    _impl->removeTrackingRect(tag);
}

- (void)_removeTrackingRects:(NSTrackingRectTag *)tags count:(int)count
{
    if (!_impl)
        return;
    _impl->removeTrackingRects(tags, count);
}

- (NSString *)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void *)data
{
    return _impl->stringForToolTip(tag);
}

- (void)pasteboardChangedOwner:(NSPasteboard *)pasteboard
{
    _impl->pasteboardChangedOwner(pasteboard);
}

- (void)pasteboard:(NSPasteboard *)pasteboard provideDataForType:(NSString *)type
{
    _impl->provideDataForPasteboard(pasteboard, type);
}

- (NSArray *)namesOfPromisedFilesDroppedAtDestination:(NSURL *)dropDestination
{
    return _impl->namesOfPromisedFilesDroppedAtDestination(dropDestination);
}

- (BOOL)wantsUpdateLayer
{
    return WebKit::WebViewImpl::wantsUpdateLayer();
}

- (void)updateLayer
{
    _impl->updateLayer();
}

- (void)setAllowsBackForwardNavigationGestures:(BOOL)allowsBackForwardNavigationGestures
{
    _impl->setAllowsBackForwardNavigationGestures(allowsBackForwardNavigationGestures);
}

- (BOOL)allowsBackForwardNavigationGestures
{
    return _impl->allowsBackForwardNavigationGestures();
}

- (void)smartMagnifyWithEvent:(NSEvent *)event
{
    _impl->smartMagnifyWithEvent(event);
}

- (void)setMagnification:(double)magnification centeredAtPoint:(NSPoint)point
{
    _impl->setMagnification(magnification, NSPointToCGPoint(point));
}

- (void)setMagnification:(double)magnification
{
    _impl->setMagnification(magnification);
}

- (double)magnification
{
    return _impl->magnification();
}

- (void)setAllowsMagnification:(BOOL)allowsMagnification
{
    _impl->setAllowsMagnification(allowsMagnification);
}

- (BOOL)allowsMagnification
{
    return _impl->allowsMagnification();
}

- (void)magnifyWithEvent:(NSEvent *)event
{
    _impl->magnifyWithEvent(event);
}

#if ENABLE(MAC_GESTURE_EVENTS)
- (void)rotateWithEvent:(NSEvent *)event
{
    _impl->rotateWithEvent(event);
}
#endif

- (WKTextFinderClient *)_ensureTextFinderClient
{
    if (!_textFinderClient)
        _textFinderClient = adoptNS([[WKTextFinderClient alloc] initWithPage:*_page view:self]);
    return _textFinderClient.get();
}

- (void)findMatchesForString:(NSString *)targetString relativeToMatch:(id <NSTextFinderAsynchronousDocumentFindMatch>)relativeMatch findOptions:(NSTextFinderAsynchronousDocumentFindOptions)findOptions maxResults:(NSUInteger)maxResults resultCollector:(void (^)(NSArray *matches, BOOL didWrap))resultCollector
{
    [[self _ensureTextFinderClient] findMatchesForString:targetString relativeToMatch:relativeMatch findOptions:findOptions maxResults:maxResults resultCollector:resultCollector];
}

- (NSView *)documentContainerView
{
    return self;
}

- (void)getSelectedText:(void (^)(NSString *selectedTextString))completionHandler
{
    [[self _ensureTextFinderClient] getSelectedText:completionHandler];
}

- (void)selectFindMatch:(id <NSTextFinderAsynchronousDocumentFindMatch>)findMatch completionHandler:(void (^)(void))completionHandler
{
    [[self _ensureTextFinderClient] selectFindMatch:findMatch completionHandler:completionHandler];
}

- (NSTextInputContext *)_web_superInputContext
{
    return [super inputContext];
}

- (void)_web_superQuickLookWithEvent:(NSEvent *)event
{
    [super quickLookWithEvent:event];
}

- (void)_web_superSwipeWithEvent:(NSEvent *)event
{
    [super swipeWithEvent:event];
}

- (void)_web_superMagnifyWithEvent:(NSEvent *)event
{
    [super magnifyWithEvent:event];
}

- (void)_web_superSmartMagnifyWithEvent:(NSEvent *)event
{
    [super smartMagnifyWithEvent:event];
}

- (void)_web_superRemoveTrackingRect:(NSTrackingRectTag)tag
{
    [super removeTrackingRect:tag];
}

- (id)_web_superAccessibilityAttributeValue:(NSString *)attribute
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return [super accessibilityAttributeValue:attribute];
#pragma clang diagnostic pop
}

- (void)_web_superDoCommandBySelector:(SEL)selector
{
    [super doCommandBySelector:selector];
}

- (BOOL)_web_superPerformKeyEquivalent:(NSEvent *)event
{
    return [super performKeyEquivalent:event];
}

- (void)_web_superKeyDown:(NSEvent *)event
{
    [super keyDown:event];
}

- (NSView *)_web_superHitTest:(NSPoint)point
{
    return [super hitTest:point];
}

- (id)_web_immediateActionAnimationControllerForHitTestResultInternal:(API::HitTestResult*)hitTestResult withType:(uint32_t)type userData:(API::Object*)userData
{
    id<NSSecureCoding> data = userData ? static_cast<id<NSSecureCoding>>(userData->wrapper()) : nil;
    return [self _immediateActionAnimationControllerForHitTestResult:wrapper(*hitTestResult) withType:(_WKImmediateActionType)type userData:data];
}

- (void)_web_prepareForImmediateActionAnimation
{
    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)[self UIDelegate];
    if ([uiDelegate respondsToSelector:@selector(_prepareForImmediateActionAnimationForWebView:)])
        [uiDelegate _prepareForImmediateActionAnimationForWebView:self];
    else
        [self _prepareForImmediateActionAnimation];
}

- (void)_web_cancelImmediateActionAnimation
{
    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)[self UIDelegate];
    if ([uiDelegate respondsToSelector:@selector(_cancelImmediateActionAnimationForWebView:)])
        [uiDelegate _cancelImmediateActionAnimationForWebView:self];
    else
        [self _cancelImmediateActionAnimation];
}

- (void)_web_completeImmediateActionAnimation
{
    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)[self UIDelegate];
    if ([uiDelegate respondsToSelector:@selector(_completeImmediateActionAnimationForWebView:)])
        [uiDelegate _completeImmediateActionAnimationForWebView:self];
    else
        [self _completeImmediateActionAnimation];
}

- (void)_web_didChangeContentSize:(NSSize)newSize
{
}

#if ENABLE(DRAG_SUPPORT)

- (WKDragDestinationAction)_web_dragDestinationActionForDraggingInfo:(id <NSDraggingInfo>)draggingInfo
{
    id <WKUIDelegatePrivate> uiDelegate = (id <WKUIDelegatePrivate>)[self UIDelegate];
    if ([uiDelegate respondsToSelector:@selector(_webView:dragDestinationActionMaskForDraggingInfo:)])
        return [uiDelegate _webView:self dragDestinationActionMaskForDraggingInfo:draggingInfo];

    if (!linkedOnOrAfter(WebKit::SDKVersion::FirstWithDropToNavigateDisallowedByDefault))
        return WKDragDestinationActionAny;

    return WKDragDestinationActionAny & ~WKDragDestinationActionLoad;
}

#endif

- (void)_web_dismissContentRelativeChildWindows
{
    _impl->dismissContentRelativeChildWindowsFromViewOnly();
}

- (void)_web_dismissContentRelativeChildWindowsWithAnimation:(BOOL)withAnimation
{
    _impl->dismissContentRelativeChildWindowsWithAnimationFromViewOnly(withAnimation);
}

- (void)_web_editorStateDidChange
{
    [self _didChangeEditorState];
}

- (void)_web_gestureEventWasNotHandledByWebCore:(NSEvent *)event
{
    [self _gestureEventWasNotHandledByWebCore:event];
}

#endif // PLATFORM(MAC)

#if HAVE(TOUCH_BAR)

@dynamic touchBar;

- (NSTouchBar *)makeTouchBar
{
    return _impl->makeTouchBar();
}

- (NSCandidateListTouchBarItem *)candidateListTouchBarItem
{
    return _impl->candidateListTouchBarItem();
}

- (void)_web_didAddMediaControlsManager:(id)controlsManager
{
    [self _addMediaPlaybackControlsView:controlsManager];
}

- (void)_web_didRemoveMediaControlsManager
{
    [self _removeMediaPlaybackControlsView];
}

- (void)_interactWithMediaControlsForTesting
{
    [self _setWantsMediaPlaybackControlsView:YES];
    [self makeTouchBar];
}

#endif // HAVE(TOUCH_BAR)

- (id <WKURLSchemeHandler>)urlSchemeHandlerForURLScheme:(NSString *)urlScheme
{
    auto* handler = static_cast<WebKit::WebURLSchemeHandlerCocoa*>(_page->urlSchemeHandlerForScheme(urlScheme));
    return handler ? handler->apiHandler() : nil;
}

+ (BOOL)handlesURLScheme:(NSString *)urlScheme
{
    return WebCore::SchemeRegistry::isBuiltinScheme(urlScheme);
}

@end

@implementation WKWebView (WKPrivate)

- (BOOL)_isEditable
{
    return _page->isEditable();
}

- (void)_setEditable:(BOOL)editable
{
    _page->setEditable(editable);
#if PLATFORM(MAC)
    if (editable)
        _impl->didBecomeEditable();
#endif
}

- (_WKRemoteObjectRegistry *)_remoteObjectRegistry
{
#if PLATFORM(MAC)
    return _impl->remoteObjectRegistry();
#else
    if (!_remoteObjectRegistry) {
        _remoteObjectRegistry = adoptNS([[_WKRemoteObjectRegistry alloc] _initWithMessageSender:*_page]);
        _page->process().processPool().addMessageReceiver(Messages::RemoteObjectRegistry::messageReceiverName(), _page->pageID(), [_remoteObjectRegistry remoteObjectRegistry]);
    }

    return _remoteObjectRegistry.get();
#endif
}

- (WKBrowsingContextHandle *)_handle
{
    return [[[WKBrowsingContextHandle alloc] _initWithPageID:_page->pageID()] autorelease];
}

- (_WKRenderingProgressEvents)_observedRenderingProgressEvents
{
    return _observedRenderingProgressEvents;
}

- (id <WKHistoryDelegatePrivate>)_historyDelegate
{
    return _navigationState->historyDelegate().autorelease();
}

- (void)_setHistoryDelegate:(id <WKHistoryDelegatePrivate>)historyDelegate
{
    _page->setHistoryClient(_navigationState->createHistoryClient());
    _navigationState->setHistoryDelegate(historyDelegate);
}

- (NSURL *)_unreachableURL
{
    return [NSURL _web_URLWithWTFString:_page->pageLoadState().unreachableURL()];
}

- (void)_loadAlternateHTMLString:(NSString *)string baseURL:(NSURL *)baseURL forUnreachableURL:(NSURL *)unreachableURL
{
    _page->loadAlternateHTMLString(string, baseURL, unreachableURL);
}

- (WKNavigation *)_loadData:(NSData *)data MIMEType:(NSString *)MIMEType characterEncodingName:(NSString *)characterEncodingName baseURL:(NSURL *)baseURL userData:(id)userData
{
    auto navigation = _page->loadData(API::Data::createWithoutCopying(data).ptr(), MIMEType, characterEncodingName, baseURL.absoluteString, WebKit::ObjCObjectGraph::create(userData).ptr());
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)_loadRequest:(NSURLRequest *)request shouldOpenExternalURLs:(BOOL)shouldOpenExternalURLs
{
    auto navigation = _page->loadRequest(request, shouldOpenExternalURLs ? WebCore::ShouldOpenExternalURLsPolicy::ShouldAllow : WebCore::ShouldOpenExternalURLsPolicy::ShouldNotAllow);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (NSArray *)_certificateChain
{
    if (WebKit::WebFrameProxy* mainFrame = _page->mainFrame())
        return mainFrame->certificateInfo() ? (NSArray *)mainFrame->certificateInfo()->certificateInfo().certificateChain() : nil;

    return nil;
}

- (NSURL *)_committedURL
{
    return [NSURL _web_URLWithWTFString:_page->pageLoadState().url()];
}

- (NSString *)_MIMEType
{
    if (_page->mainFrame())
        return _page->mainFrame()->mimeType();

    return nil;
}

- (NSString *)_userAgent
{
    return _page->userAgent();
}

- (NSString *)_applicationNameForUserAgent
{
    return _page->applicationNameForUserAgent();
}

- (void)_setApplicationNameForUserAgent:(NSString *)applicationNameForUserAgent
{
    _page->setApplicationNameForUserAgent(applicationNameForUserAgent);
}

- (NSString *)_customUserAgent
{
    return self.customUserAgent;
}

- (void)_setCustomUserAgent:(NSString *)customUserAgent
{
    self.customUserAgent = customUserAgent;
}

- (void)_setUserContentExtensionsEnabled:(BOOL)userContentExtensionsEnabled
{
    // This is kept for binary compatibility with iOS 9.
}

- (BOOL)_userContentExtensionsEnabled
{
    // This is kept for binary compatibility with iOS 9.
    return true;
}

- (pid_t)_webProcessIdentifier
{
    if (![self _isValid])
        return 0;

    return _page->processIdentifier();
}

- (void)_killWebContentProcess
{
    if (![self _isValid])
        return;

    _page->process().terminate();
}

- (WKNavigation *)_reloadWithoutContentBlockers
{
    auto navigation = _page->reload(WebCore::ReloadOption::DisableContentBlockers);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (WKNavigation *)_reloadExpiredOnly
{
    auto navigation = _page->reload(WebCore::ReloadOption::ExpiredOnly);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (void)_killWebContentProcessAndResetState
{
    _page->process().requestTermination(WebKit::ProcessTerminationReason::RequestedByClient);
}

#if PLATFORM(MAC)
- (void)_setShouldSuppressFirstResponderChanges:(BOOL)shouldSuppress
{
    _impl->setShouldSuppressFirstResponderChanges(shouldSuppress);
}
#endif

#if PLATFORM(IOS)
static WebCore::FloatSize activeMaximumUnobscuredSize(WKWebView *webView, const CGRect& bounds)
{
    return WebCore::FloatSize(webView->_overridesMaximumUnobscuredSize ? webView->_maximumUnobscuredSizeOverride : bounds.size);
}

static int32_t activeOrientation(WKWebView *webView)
{
    return webView->_overridesInterfaceOrientation ? deviceOrientationForUIInterfaceOrientation(webView->_interfaceOrientationOverride) : webView->_page->deviceOrientation();
}

- (void (^)(void))_retainActiveFocusedState
{
    ++_activeFocusedStateRetainCount;

    // FIXME: Use something like CompletionHandlerCallChecker to ensure that the returned block is called before it's released.
    return [[[self] {
        --_activeFocusedStateRetainCount;
    } copy] autorelease];
}

- (void)_becomeFirstResponderWithSelectionMovingForward:(BOOL)selectingForward completionHandler:(void (^)(BOOL didBecomeFirstResponder))completionHandler
{
    typeof(completionHandler) completionHandlerCopy = nil;
    if (completionHandler)
        completionHandlerCopy = Block_copy(completionHandler);

    [_contentView _becomeFirstResponderWithSelectionMovingForward:selectingForward completionHandler:[completionHandlerCopy](BOOL didBecomeFirstResponder) {
        if (!completionHandlerCopy)
            return;

        completionHandlerCopy(didBecomeFirstResponder);
        Block_release(completionHandlerCopy);
    }];
}

- (id)_snapshotLayerContentsForBackForwardListItem:(WKBackForwardListItem *)item
{
    if (_page->backForwardList().currentItem() == &item._item)
        _page->recordNavigationSnapshot(*_page->backForwardList().currentItem());

    if (auto* viewSnapshot = item._item.snapshot())
        return viewSnapshot->asLayerContents();

    return nil;
}

- (NSArray *)_dataDetectionResults
{
#if ENABLE(DATA_DETECTION)
    return [_contentView _dataDetectionResults];
#else
    return nil;
#endif
}

- (void)_accessibilityRetrieveSpeakSelectionContent
{
    [_contentView accessibilityRetrieveSpeakSelectionContent];
}

// This method is for subclasses to override.
// Currently it's only in TestRunnerWKWebView.
- (void)_accessibilityDidGetSpeakSelectionContent:(NSString *)content
{
}

#endif

- (NSData *)_sessionStateData
{
    WebKit::SessionState sessionState = _page->sessionState();

    // FIXME: This should not use the legacy session state encoder.
    return [wrapper(*WebKit::encodeLegacySessionState(sessionState).leakRef()) autorelease];
}

- (_WKSessionState *)_sessionState
{
    return adoptNS([[_WKSessionState alloc] _initWithSessionState:_page->sessionState()]).autorelease();
}

- (_WKSessionState *)_sessionStateWithFilter:(BOOL (^)(WKBackForwardListItem *item))filter
{
    WebKit::SessionState sessionState = _page->sessionState([filter](WebKit::WebBackForwardListItem& item) {
        if (!filter)
            return true;

        return (bool)filter(wrapper(item));
    });

    return adoptNS([[_WKSessionState alloc] _initWithSessionState:sessionState]).autorelease();
}

- (void)_restoreFromSessionStateData:(NSData *)sessionStateData
{
    // FIXME: This should not use the legacy session state decoder.
    WebKit::SessionState sessionState;
    if (!WebKit::decodeLegacySessionState(static_cast<const uint8_t*>(sessionStateData.bytes), sessionStateData.length, sessionState))
        return;

    _page->restoreFromSessionState(WTFMove(sessionState), true);
}

- (WKNavigation *)_restoreSessionState:(_WKSessionState *)sessionState andNavigate:(BOOL)navigate
{
    auto navigation = _page->restoreFromSessionState(sessionState->_sessionState, navigate);
    if (!navigation)
        return nil;

    return [wrapper(*navigation.leakRef()) autorelease];
}

- (void)_close
{
    _page->close();
}

- (_WKAttachment *)_insertAttachmentWithFilename:(NSString *)filename contentType:(NSString *)contentType data:(NSData *)data options:(_WKAttachmentDisplayOptions *)options completion:(void(^)(BOOL success))completionHandler
{
#if ENABLE(ATTACHMENT_ELEMENT)
    auto identifier = createCanonicalUUIDString();

    auto coreOptions = options ? options.coreDisplayOptions : WebCore::AttachmentDisplayOptions { };
    auto buffer = WebCore::SharedBuffer::create(data);
    _page->insertAttachment(identifier, coreOptions, filename, contentType.length ? std::optional<String> { contentType } : std::nullopt, buffer.get(), [capturedHandler = makeBlockPtr(completionHandler), capturedBuffer = buffer.copyRef()] (WebKit::CallbackBase::Error error) {
        if (capturedHandler)
            capturedHandler(error == WebKit::CallbackBase::Error::None);
    });

    return [wrapper(API::Attachment::create(identifier, *_page).leakRef()) autorelease];
#else
    return nil;
#endif
}

- (void)_evaluateJavaScriptWithoutUserGesture:(NSString *)javaScriptString completionHandler:(void (^)(id, NSError *))completionHandler
{
    [self _evaluateJavaScript:javaScriptString forceUserGesture:NO completionHandler:completionHandler];
}

- (void)_updateWebsitePolicies:(_WKWebsitePolicies *)websitePolicies
{
    auto data = websitePolicies->_websitePolicies->data();
    if (data.websiteDataStoreParameters)
        [NSException raise:NSInvalidArgumentException format:@"Updating WKWebsiteDataStore is only supported during decidePolicyForNavigationAction."];
    _page->updateWebsitePolicies(WTFMove(data));
}

- (BOOL)_allowsRemoteInspection
{
#if ENABLE(REMOTE_INSPECTOR)
    return _page->allowsRemoteInspection();
#else
    return NO;
#endif
}

- (void)_setAllowsRemoteInspection:(BOOL)allow
{
#if ENABLE(REMOTE_INSPECTOR)
    _page->setAllowsRemoteInspection(allow);
#endif
}

- (NSString *)_remoteInspectionNameOverride
{
#if ENABLE(REMOTE_INSPECTOR)
    return _page->remoteInspectionNameOverride();
#else
    return nil;
#endif
}

- (void)_setRemoteInspectionNameOverride:(NSString *)name
{
#if ENABLE(REMOTE_INSPECTOR)
    _page->setRemoteInspectionNameOverride(name);
#endif
}

- (BOOL)_addsVisitedLinks
{
    return _page->addsVisitedLinks();
}

- (void)_setAddsVisitedLinks:(BOOL)addsVisitedLinks
{
    _page->setAddsVisitedLinks(addsVisitedLinks);
}

- (BOOL)_networkRequestsInProgress
{
    return _page->pageLoadState().networkRequestsInProgress();
}

static inline WebCore::LayoutMilestones layoutMilestones(_WKRenderingProgressEvents events)
{
    WebCore::LayoutMilestones milestones = 0;

    if (events & _WKRenderingProgressEventFirstLayout)
        milestones |= WebCore::DidFirstLayout;

    if (events & _WKRenderingProgressEventFirstVisuallyNonEmptyLayout)
        milestones |= WebCore::DidFirstVisuallyNonEmptyLayout;

    if (events & _WKRenderingProgressEventFirstPaintWithSignificantArea)
        milestones |= WebCore::DidHitRelevantRepaintedObjectsAreaThreshold;

    if (events & _WKRenderingProgressEventReachedSessionRestorationRenderTreeSizeThreshold)
        milestones |= WebCore::ReachedSessionRestorationRenderTreeSizeThreshold;

    if (events & _WKRenderingProgressEventFirstLayoutAfterSuppressedIncrementalRendering)
        milestones |= WebCore::DidFirstLayoutAfterSuppressedIncrementalRendering;

    if (events & _WKRenderingProgressEventFirstPaintAfterSuppressedIncrementalRendering)
        milestones |= WebCore::DidFirstPaintAfterSuppressedIncrementalRendering;

    return milestones;
}

- (void)_setObservedRenderingProgressEvents:(_WKRenderingProgressEvents)observedRenderingProgressEvents
{
    _observedRenderingProgressEvents = observedRenderingProgressEvents;
    _page->listenForLayoutMilestones(layoutMilestones(observedRenderingProgressEvents));
}

- (void)_getMainResourceDataWithCompletionHandler:(void (^)(NSData *, NSError *))completionHandler
{
    auto handler = adoptNS([completionHandler copy]);

    _page->getMainResourceDataOfFrame(_page->mainFrame(), [handler](API::Data* data, WebKit::CallbackBase::Error error) {
        void (^completionHandlerBlock)(NSData *, NSError *) = (void (^)(NSData *, NSError *))handler.get();
        if (error != WebKit::CallbackBase::Error::None) {
            // FIXME: Pipe a proper error in from the WebPageProxy.
            RetainPtr<NSError> error = adoptNS([[NSError alloc] init]);
            completionHandlerBlock(nil, error.get());
        } else
            completionHandlerBlock(wrapper(*data), nil);
    });
}

- (void)_getWebArchiveDataWithCompletionHandler:(void (^)(NSData *, NSError *))completionHandler
{
    auto handler = adoptNS([completionHandler copy]);

    _page->getWebArchiveOfFrame(_page->mainFrame(), [handler](API::Data* data, WebKit::CallbackBase::Error error) {
        void (^completionHandlerBlock)(NSData *, NSError *) = (void (^)(NSData *, NSError *))handler.get();
        if (error != WebKit::CallbackBase::Error::None) {
            // FIXME: Pipe a proper error in from the WebPageProxy.
            RetainPtr<NSError> error = adoptNS([[NSError alloc] init]);
            completionHandlerBlock(nil, error.get());
        } else
            completionHandlerBlock(wrapper(*data), nil);
    });
}

- (void)_getContentsAsStringWithCompletionHandler:(void (^)(NSString *, NSError *))completionHandler
{
    auto handler = makeBlockPtr(completionHandler);

    _page->getContentsAsString([handler](String string, WebKit::CallbackBase::Error error) {
        if (error != WebKit::CallbackBase::Error::None) {
            // FIXME: Pipe a proper error in from the WebPageProxy.
            RetainPtr<NSError> error = adoptNS([[NSError alloc] init]);
            handler(nil, error.get());
        } else
            handler(string, nil);
    });
}

- (void)_getApplicationManifestWithCompletionHandler:(void (^)(_WKApplicationManifest *))completionHandler
{
#if ENABLE(APPLICATION_MANIFEST)
    _page->getApplicationManifest([completionHandler = makeBlockPtr(completionHandler)](const std::optional<WebCore::ApplicationManifest>& manifest, WebKit::CallbackBase::Error error) {
        UNUSED(error);
        if (completionHandler) {
            if (manifest) {
                auto apiManifest = API::ApplicationManifest::create(*manifest);
                completionHandler(wrapper(apiManifest));
            } else
                completionHandler(nil);
        }
    });
#else
    if (completionHandler)
        completionHandler(nil);
#endif
}

- (_WKPaginationMode)_paginationMode
{
    switch (_page->paginationMode()) {
    case WebCore::Pagination::Unpaginated:
        return _WKPaginationModeUnpaginated;
    case WebCore::Pagination::LeftToRightPaginated:
        return _WKPaginationModeLeftToRight;
    case WebCore::Pagination::RightToLeftPaginated:
        return _WKPaginationModeRightToLeft;
    case WebCore::Pagination::TopToBottomPaginated:
        return _WKPaginationModeTopToBottom;
    case WebCore::Pagination::BottomToTopPaginated:
        return _WKPaginationModeBottomToTop;
    }

    ASSERT_NOT_REACHED();
    return _WKPaginationModeUnpaginated;
}

- (void)_setPaginationMode:(_WKPaginationMode)paginationMode
{
    WebCore::Pagination::Mode mode;
    switch (paginationMode) {
    case _WKPaginationModeUnpaginated:
        mode = WebCore::Pagination::Unpaginated;
        break;
    case _WKPaginationModeLeftToRight:
        mode = WebCore::Pagination::LeftToRightPaginated;
        break;
    case _WKPaginationModeRightToLeft:
        mode = WebCore::Pagination::RightToLeftPaginated;
        break;
    case _WKPaginationModeTopToBottom:
        mode = WebCore::Pagination::TopToBottomPaginated;
        break;
    case _WKPaginationModeBottomToTop:
        mode = WebCore::Pagination::BottomToTopPaginated;
        break;
    default:
        return;
    }

    _page->setPaginationMode(mode);
}

- (BOOL)_paginationBehavesLikeColumns
{
    return _page->paginationBehavesLikeColumns();
}

- (void)_setPaginationBehavesLikeColumns:(BOOL)behavesLikeColumns
{
    _page->setPaginationBehavesLikeColumns(behavesLikeColumns);
}

- (CGFloat)_pageLength
{
    return _page->pageLength();
}

- (void)_setPageLength:(CGFloat)pageLength
{
    _page->setPageLength(pageLength);
}

- (CGFloat)_gapBetweenPages
{
    return _page->gapBetweenPages();
}

- (void)_setGapBetweenPages:(CGFloat)gapBetweenPages
{
    _page->setGapBetweenPages(gapBetweenPages);
}

- (BOOL)_paginationLineGridEnabled
{
    return _page->paginationLineGridEnabled();
}

- (void)_setPaginationLineGridEnabled:(BOOL)lineGridEnabled
{
    _page->setPaginationLineGridEnabled(lineGridEnabled);
}

- (NSUInteger)_pageCount
{
    return _page->pageCount();
}

- (BOOL)_supportsTextZoom
{
    return _page->supportsTextZoom();
}

- (double)_textZoomFactor
{
    return _page->textZoomFactor();
}

- (void)_setTextZoomFactor:(double)zoomFactor
{
    _page->setTextZoomFactor(zoomFactor);
}

- (double)_pageZoomFactor
{
    return _page->pageZoomFactor();
}

- (void)_setPageZoomFactor:(double)zoomFactor
{
    _page->setPageZoomFactor(zoomFactor);
}

- (id <_WKDiagnosticLoggingDelegate>)_diagnosticLoggingDelegate
{
    auto* diagnosticLoggingClient = _page->diagnosticLoggingClient();
    if (!diagnosticLoggingClient)
        return nil;

    return [static_cast<WebKit::DiagnosticLoggingClient&>(*diagnosticLoggingClient).delegate().leakRef() autorelease];
}

- (void)_setDiagnosticLoggingDelegate:(id<_WKDiagnosticLoggingDelegate>)diagnosticLoggingDelegate
{
    auto* diagnosticLoggingClient = _page->diagnosticLoggingClient();
    if (!diagnosticLoggingClient)
        return;

    static_cast<WebKit::DiagnosticLoggingClient&>(*diagnosticLoggingClient).setDelegate(diagnosticLoggingDelegate);
}

- (id <_WKFindDelegate>)_findDelegate
{
    return [static_cast<WebKit::FindClient&>(_page->findClient()).delegate().leakRef() autorelease];
}

- (void)_setFindDelegate:(id<_WKFindDelegate>)findDelegate
{
    static_cast<WebKit::FindClient&>(_page->findClient()).setDelegate(findDelegate);
}

static inline WebKit::FindOptions toFindOptions(_WKFindOptions wkFindOptions)
{
    unsigned findOptions = 0;

    if (wkFindOptions & _WKFindOptionsCaseInsensitive)
        findOptions |= WebKit::FindOptionsCaseInsensitive;
    if (wkFindOptions & _WKFindOptionsAtWordStarts)
        findOptions |= WebKit::FindOptionsAtWordStarts;
    if (wkFindOptions & _WKFindOptionsTreatMedialCapitalAsWordStart)
        findOptions |= WebKit::FindOptionsTreatMedialCapitalAsWordStart;
    if (wkFindOptions & _WKFindOptionsBackwards)
        findOptions |= WebKit::FindOptionsBackwards;
    if (wkFindOptions & _WKFindOptionsWrapAround)
        findOptions |= WebKit::FindOptionsWrapAround;
    if (wkFindOptions & _WKFindOptionsShowOverlay)
        findOptions |= WebKit::FindOptionsShowOverlay;
    if (wkFindOptions & _WKFindOptionsShowFindIndicator)
        findOptions |= WebKit::FindOptionsShowFindIndicator;
    if (wkFindOptions & _WKFindOptionsShowHighlight)
        findOptions |= WebKit::FindOptionsShowHighlight;
    if (wkFindOptions & _WKFindOptionsDetermineMatchIndex)
        findOptions |= WebKit::FindOptionsDetermineMatchIndex;

    return static_cast<WebKit::FindOptions>(findOptions);
}

- (void)_countStringMatches:(NSString *)string options:(_WKFindOptions)options maxCount:(NSUInteger)maxCount
{
#if PLATFORM(IOS)
    if (_customContentView) {
        [_customContentView web_countStringMatches:string options:options maxCount:maxCount];
        return;
    }
#endif
    _page->countStringMatches(string, toFindOptions(options), maxCount);
}

- (void)_findString:(NSString *)string options:(_WKFindOptions)options maxCount:(NSUInteger)maxCount
{
#if PLATFORM(IOS)
    if (_customContentView) {
        [_customContentView web_findString:string options:options maxCount:maxCount];
        return;
    }
#endif
    _page->findString(string, toFindOptions(options), maxCount);
}

- (void)_hideFindUI
{
#if PLATFORM(IOS)
    if (_customContentView) {
        [_customContentView web_hideFindUI];
        return;
    }
#endif
    _page->hideFindUI();
}

- (void)_saveBackForwardSnapshotForItem:(WKBackForwardListItem *)item
{
    _page->recordNavigationSnapshot(item._item);
}

- (id <_WKInputDelegate>)_inputDelegate
{
    return _inputDelegate.getAutoreleased();
}

- (void)_setInputDelegate:(id <_WKInputDelegate>)inputDelegate
{
    _inputDelegate = inputDelegate;

    class FormClient : public API::FormClient {
    public:
        explicit FormClient(WKWebView *webView)
            : m_webView(webView)
        {
        }

        virtual ~FormClient() { }

        void willSubmitForm(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, WebKit::WebFrameProxy& sourceFrame, const Vector<std::pair<WTF::String, WTF::String>>& textFieldValues, API::Object* userData, WTF::Function<void(void)>&& completionHandler) override
        {
            if (userData && userData->type() != API::Object::Type::Data) {
                ASSERT(!userData || userData->type() == API::Object::Type::Data);
                m_webView->_page->process().connection()->markCurrentlyDispatchedMessageAsInvalid();
                completionHandler();
                return;
            }

            auto inputDelegate = m_webView->_inputDelegate.get();

            if (![inputDelegate respondsToSelector:@selector(_webView:willSubmitFormValues:userObject:submissionHandler:)]) {
                completionHandler();
                return;
            }

            auto valueMap = adoptNS([[NSMutableDictionary alloc] initWithCapacity:textFieldValues.size()]);
            for (const auto& pair : textFieldValues)
                [valueMap setObject:pair.second forKey:pair.first];

            NSObject <NSSecureCoding> *userObject = nil;
            if (API::Data* data = static_cast<API::Data*>(userData)) {
                auto nsData = adoptNS([[NSData alloc] initWithBytesNoCopy:const_cast<void*>(static_cast<const void*>(data->bytes())) length:data->size() freeWhenDone:NO]);
                auto unarchiver = secureUnarchiverFromData(nsData.get());
                @try {
                    userObject = [unarchiver decodeObjectOfClass:[NSObject class] forKey:@"userObject"];
                } @catch (NSException *exception) {
                    LOG_ERROR("Failed to decode user data: %@", exception);
                }
            }

            auto checker = WebKit::CompletionHandlerCallChecker::create(inputDelegate.get(), @selector(_webView:willSubmitFormValues:userObject:submissionHandler:));
            [inputDelegate _webView:m_webView willSubmitFormValues:valueMap.get() userObject:userObject submissionHandler:BlockPtr<void(void)>::fromCallable([completionHandler = WTFMove(completionHandler), checker = WTFMove(checker)] {
                if (checker->completionHandlerHasBeenCalled())
                    return;
                checker->didCallCompletionHandler();
                completionHandler();
            }).get()];
        }

    private:
        WKWebView *m_webView;
    };

    if (inputDelegate)
        _page->setFormClient(std::make_unique<FormClient>(self));
    else
        _page->setFormClient(nullptr);
}

- (BOOL)_isDisplayingStandaloneImageDocument
{
    if (auto* mainFrame = _page->mainFrame())
        return mainFrame->isDisplayingStandaloneImageDocument();
    return NO;
}

- (BOOL)_isDisplayingStandaloneMediaDocument
{
    if (auto* mainFrame = _page->mainFrame())
        return mainFrame->isDisplayingStandaloneMediaDocument();
    return NO;
}

- (BOOL)_isPlayingAudio
{
    return _page->isPlayingAudio();
}

- (BOOL)_isShowingNavigationGestureSnapshot
{
    return _page->isShowingNavigationGestureSnapshot();
}

- (_WKLayoutMode)_layoutMode
{
#if PLATFORM(MAC)
    switch (_impl->layoutMode()) {
    case kWKLayoutModeFixedSize:
        return _WKLayoutModeFixedSize;
    case kWKLayoutModeDynamicSizeComputedFromViewScale:
        return _WKLayoutModeDynamicSizeComputedFromViewScale;
    case kWKLayoutModeDynamicSizeComputedFromMinimumDocumentSize:
        return _WKLayoutModeDynamicSizeComputedFromMinimumDocumentSize;
    case kWKLayoutModeViewSize:
    default:
        return _WKLayoutModeViewSize;
    }
#else
    return _page->useFixedLayout() ? _WKLayoutModeFixedSize : _WKLayoutModeViewSize;
#endif
}

- (void)_setLayoutMode:(_WKLayoutMode)layoutMode
{
#if PLATFORM(MAC)
    WKLayoutMode wkViewLayoutMode;
    switch (layoutMode) {
    case _WKLayoutModeFixedSize:
        wkViewLayoutMode = kWKLayoutModeFixedSize;
        break;
    case _WKLayoutModeDynamicSizeComputedFromViewScale:
        wkViewLayoutMode = kWKLayoutModeDynamicSizeComputedFromViewScale;
        break;
    case _WKLayoutModeDynamicSizeComputedFromMinimumDocumentSize:
        wkViewLayoutMode = kWKLayoutModeDynamicSizeComputedFromMinimumDocumentSize;
        break;
    case _WKLayoutModeViewSize:
    default:
        wkViewLayoutMode = kWKLayoutModeViewSize;
        break;
    }
    _impl->setLayoutMode(wkViewLayoutMode);
#else
    _page->setUseFixedLayout(layoutMode == _WKLayoutModeFixedSize || layoutMode == _WKLayoutModeDynamicSizeComputedFromViewScale);
#endif
}

- (CGSize)_fixedLayoutSize
{
    return _page->fixedLayoutSize();
}

- (void)_setFixedLayoutSize:(CGSize)fixedLayoutSize
{
    _page->setFixedLayoutSize(WebCore::expandedIntSize(WebCore::FloatSize(fixedLayoutSize)));
}

- (void)_setBackgroundExtendsBeyondPage:(BOOL)backgroundExtends
{
    _page->setBackgroundExtendsBeyondPage(backgroundExtends);
}

- (BOOL)_backgroundExtendsBeyondPage
{
    return _page->backgroundExtendsBeyondPage();
}

- (CGFloat)_viewScale
{
    return _page->viewScaleFactor();
}

- (void)_setViewScale:(CGFloat)viewScale
{
#if PLATFORM(MAC)
    _impl->setViewScale(viewScale);
#else
    if (viewScale <= 0 || isnan(viewScale) || isinf(viewScale))
        [NSException raise:NSInvalidArgumentException format:@"View scale should be a positive number"];

    _page->scaleView(viewScale);
#endif
}

#pragma mark scrollperf methods

- (void)_setScrollPerformanceDataCollectionEnabled:(BOOL)enabled
{
    _page->setScrollPerformanceDataCollectionEnabled(enabled);
}

- (BOOL)_scrollPerformanceDataCollectionEnabled
{
    return _page->scrollPerformanceDataCollectionEnabled();
}

- (NSArray *)_scrollPerformanceData
{
#if PLATFORM(IOS)
    if (WebKit::RemoteLayerTreeScrollingPerformanceData* scrollPerfData = _page->scrollingPerformanceData())
        return scrollPerfData->data();
#endif
    return nil;
}

#pragma mark media playback restrictions

- (BOOL)_allowsMediaDocumentInlinePlayback
{
#if PLATFORM(IOS)
    return _page->allowsMediaDocumentInlinePlayback();
#else
    return NO;
#endif
}

- (void)_setAllowsMediaDocumentInlinePlayback:(BOOL)flag
{
#if PLATFORM(IOS)
    _page->setAllowsMediaDocumentInlinePlayback(flag);
#endif
}

- (BOOL)_webProcessIsResponsive
{
    return _page->process().isResponsive();
}

- (void)_setFullscreenDelegate:(id<_WKFullscreenDelegate>)delegate
{
#if ENABLE(FULLSCREEN_API)
    if (is<WebKit::FullscreenClient>(_page->fullscreenClient()))
        downcast<WebKit::FullscreenClient>(_page->fullscreenClient()).setDelegate(delegate);
#endif
}

- (id<_WKFullscreenDelegate>)_fullscreenDelegate
{
#if ENABLE(FULLSCREEN_API)
    if (is<WebKit::FullscreenClient>(_page->fullscreenClient()))
        return downcast<WebKit::FullscreenClient>(_page->fullscreenClient()).delegate().autorelease();
#endif
    return nil;
}

- (BOOL)_isInFullscreen
{
#if ENABLE(FULLSCREEN_API)
    return _page->fullScreenManager() && _page->fullScreenManager()->isFullScreen();
#else
    return false;
#endif
}

- (void)_muteMediaCapture
{
    _page->setMediaStreamCaptureMuted(true);
}

- (void)_setMediaCaptureEnabled:(BOOL)enabled
{
    _page->setMediaCaptureEnabled(enabled);
}

- (BOOL)_mediaCaptureEnabled
{
    return _page->mediaCaptureEnabled();
}

- (void)_setPageMuted:(_WKMediaMutedState)mutedState
{
    WebCore::MediaProducer::MutedStateFlags coreState = WebCore::MediaProducer::NoneMuted;

    if (mutedState & _WKMediaAudioMuted)
        coreState |= WebCore::MediaProducer::AudioIsMuted;
    if (mutedState & _WKMediaCaptureDevicesMuted)
        coreState |= WebCore::MediaProducer::CaptureDevicesAreMuted;

    _page->setMuted(coreState);
}

#pragma mark iOS-specific methods

#if PLATFORM(IOS)

#if ENABLE(FULLSCREEN_API)
- (void)removeFromSuperview
{
    [super removeFromSuperview];

    if ([_fullScreenWindowController isFullScreen])
        [_fullScreenWindowController webViewDidRemoveFromSuperviewWhileInFullscreen];
}
#endif

- (CGSize)_minimumLayoutSizeOverride
{
    ASSERT(_overridesMinimumLayoutSize);
    return _minimumLayoutSizeOverride;
}

- (void)_setMinimumLayoutSizeOverride:(CGSize)minimumLayoutSizeOverride
{
    _overridesMinimumLayoutSize = YES;
    _minimumLayoutSizeOverride = minimumLayoutSizeOverride;

    if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::NotResizing)
        [self _dispatchSetMinimumLayoutSize:WebCore::FloatSize(minimumLayoutSizeOverride)];

}

- (UIEdgeInsets)_obscuredInsets
{
    return _obscuredInsets;
}

- (void)_setObscuredInsets:(UIEdgeInsets)obscuredInsets
{
    ASSERT(obscuredInsets.top >= 0);
    ASSERT(obscuredInsets.left >= 0);
    ASSERT(obscuredInsets.bottom >= 0);
    ASSERT(obscuredInsets.right >= 0);

    _haveSetObscuredInsets = YES;

    if (UIEdgeInsetsEqualToEdgeInsets(_obscuredInsets, obscuredInsets))
        return;

    _obscuredInsets = obscuredInsets;

    [self _scheduleVisibleContentRectUpdate];
}

- (UIRectEdge)_obscuredInsetEdgesAffectedBySafeArea
{
    return _obscuredInsetEdgesAffectedBySafeArea;
}

- (void)_setObscuredInsetEdgesAffectedBySafeArea:(UIRectEdge)edges
{
    if (edges == _obscuredInsetEdgesAffectedBySafeArea)
        return;

    _obscuredInsetEdgesAffectedBySafeArea = edges;

    [self _scheduleVisibleContentRectUpdate];
}

- (UIEdgeInsets)_unobscuredSafeAreaInsets
{
    return _unobscuredSafeAreaInsets;
}

- (void)_setUnobscuredSafeAreaInsets:(UIEdgeInsets)unobscuredSafeAreaInsets
{
    ASSERT(unobscuredSafeAreaInsets.top >= 0);
    ASSERT(unobscuredSafeAreaInsets.left >= 0);
    ASSERT(unobscuredSafeAreaInsets.bottom >= 0);
    ASSERT(unobscuredSafeAreaInsets.right >= 0);

    _haveSetUnobscuredSafeAreaInsets = YES;

    if (UIEdgeInsetsEqualToEdgeInsets(_unobscuredSafeAreaInsets, unobscuredSafeAreaInsets))
        return;

    _unobscuredSafeAreaInsets = unobscuredSafeAreaInsets;

    [self _scheduleVisibleContentRectUpdate];
}

- (BOOL)_safeAreaShouldAffectObscuredInsets
{
    if (![self usesStandardContentView])
        return NO;
    return _avoidsUnsafeArea;
}

- (void)_setInterfaceOrientationOverride:(UIInterfaceOrientation)interfaceOrientation
{
    _overridesInterfaceOrientation = YES;
    _interfaceOrientationOverride = interfaceOrientation;

    if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::NotResizing)
        [self _dispatchSetDeviceOrientation:deviceOrientationForUIInterfaceOrientation(_interfaceOrientationOverride)];
}

- (UIInterfaceOrientation)_interfaceOrientationOverride
{
    ASSERT(_overridesInterfaceOrientation);
    return _interfaceOrientationOverride;
}

- (void)_clearInterfaceOrientationOverride
{
    _overridesInterfaceOrientation = NO;
    _interfaceOrientationOverride = UIInterfaceOrientationPortrait;
}

- (CGSize)_maximumUnobscuredSizeOverride
{
    ASSERT(_overridesMaximumUnobscuredSize);
    return _maximumUnobscuredSizeOverride;
}

- (void)_setMaximumUnobscuredSizeOverride:(CGSize)size
{
    ASSERT(size.width <= self.bounds.size.width && size.height <= self.bounds.size.height);
    _overridesMaximumUnobscuredSize = YES;
    _maximumUnobscuredSizeOverride = size;

    if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::NotResizing)
        [self _dispatchSetMaximumUnobscuredSize:WebCore::FloatSize(size)];
}

- (void)_setAllowsViewportShrinkToFit:(BOOL)allowShrinkToFit
{
    _allowsViewportShrinkToFit = allowShrinkToFit;
}

- (BOOL)_allowsViewportShrinkToFit
{
    return _allowsViewportShrinkToFit;
}

- (void)_beginInteractiveObscuredInsetsChange
{
    ASSERT(!_isChangingObscuredInsetsInteractively);
    _isChangingObscuredInsetsInteractively = YES;
}

- (void)_endInteractiveObscuredInsetsChange
{
    ASSERT(_isChangingObscuredInsetsInteractively);
    _isChangingObscuredInsetsInteractively = NO;
    [self _scheduleVisibleContentRectUpdate];
}

- (void)_hideContentUntilNextUpdate
{
    if (auto* area = _page->drawingArea())
        area->hideContentUntilAnyUpdate();
}

- (void)_beginAnimatedResizeWithUpdates:(void (^)(void))updateBlock
{
    CGRect oldBounds = self.bounds;
    WebCore::FloatRect oldUnobscuredContentRect = _page->unobscuredContentRect();

    if (![self usesStandardContentView] || !_hasCommittedLoadForMainFrame || CGRectIsEmpty(oldBounds) || oldUnobscuredContentRect.isEmpty()) {
        updateBlock();
        return;
    }

    _dynamicViewportUpdateMode = DynamicViewportUpdateMode::ResizingWithAnimation;

    WebCore::FloatSize oldMinimumLayoutSize = activeMinimumLayoutSize(self, oldBounds);
    WebCore::FloatSize oldMaximumUnobscuredSize = activeMaximumUnobscuredSize(self, oldBounds);
    int32_t oldOrientation = activeOrientation(self);
    UIEdgeInsets oldObscuredInsets = _obscuredInsets;

    updateBlock();

    CGRect newBounds = self.bounds;
    WebCore::FloatSize newMinimumLayoutSize = activeMinimumLayoutSize(self, newBounds);
    WebCore::FloatSize newMaximumUnobscuredSize = activeMaximumUnobscuredSize(self, newBounds);
    int32_t newOrientation = activeOrientation(self);
    UIEdgeInsets newObscuredInsets = _obscuredInsets;
    CGRect futureUnobscuredRectInSelfCoordinates = UIEdgeInsetsInsetRect(newBounds, _obscuredInsets);
    CGRect contentViewBounds = [_contentView bounds];

    ASSERT_WITH_MESSAGE(!(_overridesMinimumLayoutSize && newMinimumLayoutSize.isEmpty()), "Clients controlling the layout size should maintain a valid layout size to minimize layouts.");
    if (CGRectIsEmpty(newBounds) || newMinimumLayoutSize.isEmpty() || CGRectIsEmpty(futureUnobscuredRectInSelfCoordinates) || CGRectIsEmpty(contentViewBounds)) {
        _dynamicViewportUpdateMode = DynamicViewportUpdateMode::NotResizing;
        [self _frameOrBoundsChanged];
        if (_overridesMinimumLayoutSize)
            [self _dispatchSetMinimumLayoutSize:WebCore::FloatSize(newMinimumLayoutSize)];
        if (_overridesMaximumUnobscuredSize)
            [self _dispatchSetMaximumUnobscuredSize:WebCore::FloatSize(newMaximumUnobscuredSize)];
        if (_overridesInterfaceOrientation)
            [self _dispatchSetDeviceOrientation:newOrientation];

        [self _scheduleVisibleContentRectUpdate];
        return;
    }

    if (CGRectEqualToRect(oldBounds, newBounds)
        && oldMinimumLayoutSize == newMinimumLayoutSize
        && oldMaximumUnobscuredSize == newMaximumUnobscuredSize
        && oldOrientation == newOrientation
        && UIEdgeInsetsEqualToEdgeInsets(oldObscuredInsets, newObscuredInsets)) {
        _dynamicViewportUpdateMode = DynamicViewportUpdateMode::NotResizing;
        [self _scheduleVisibleContentRectUpdate];
        return;
    }

    ++_activeAnimatedResizeCount;
    _resizeAnimationTransformAdjustments = CATransform3DIdentity;

    NSUInteger indexOfContentView = [[_scrollView subviews] indexOfObject:_contentView.get()];
    _resizeAnimationView = adoptNS([[UIView alloc] init]);
    [_resizeAnimationView layer].name = @"ResizeAnimation";
    [_scrollView insertSubview:_resizeAnimationView.get() atIndex:indexOfContentView];
    [_resizeAnimationView addSubview:_contentView.get()];
    [_resizeAnimationView addSubview:[_contentView unscaledView]];

    CGSize contentSizeInContentViewCoordinates = contentViewBounds.size;
    [_scrollView setMinimumZoomScale:std::min(newMinimumLayoutSize.width() / contentSizeInContentViewCoordinates.width, [_scrollView minimumZoomScale])];
    [_scrollView setMaximumZoomScale:std::max(newMinimumLayoutSize.width() / contentSizeInContentViewCoordinates.width, [_scrollView maximumZoomScale])];

    // Compute the new scale to keep the current content width in the scrollview.
    CGFloat oldWebViewWidthInContentViewCoordinates = oldUnobscuredContentRect.width();
    CGFloat visibleContentViewWidthInContentCoordinates = std::min(contentSizeInContentViewCoordinates.width, oldWebViewWidthInContentViewCoordinates);
    CGFloat targetScale = newMinimumLayoutSize.width() / visibleContentViewWidthInContentCoordinates;
    CGFloat resizeAnimationViewAnimationScale = targetScale / contentZoomScale(self);
    [_resizeAnimationView setTransform:CGAffineTransformMakeScale(resizeAnimationViewAnimationScale, resizeAnimationViewAnimationScale)];

    // Compute a new position to keep the content centered.
    CGPoint originalContentCenter = oldUnobscuredContentRect.center();
    CGPoint originalContentCenterInSelfCoordinates = [self convertPoint:originalContentCenter fromView:_contentView.get()];
    CGPoint futureUnobscuredRectCenterInSelfCoordinates = CGPointMake(futureUnobscuredRectInSelfCoordinates.origin.x + futureUnobscuredRectInSelfCoordinates.size.width / 2, futureUnobscuredRectInSelfCoordinates.origin.y + futureUnobscuredRectInSelfCoordinates.size.height / 2);

    CGPoint originalContentOffset = [_scrollView contentOffset];
    CGPoint contentOffset = originalContentOffset;
    contentOffset.x += (originalContentCenterInSelfCoordinates.x - futureUnobscuredRectCenterInSelfCoordinates.x);
    contentOffset.y += (originalContentCenterInSelfCoordinates.y - futureUnobscuredRectCenterInSelfCoordinates.y);

    // Limit the new offset within the scrollview, we do not want to rubber band programmatically.
    CGSize futureContentSizeInSelfCoordinates = CGSizeMake(contentSizeInContentViewCoordinates.width * targetScale, contentSizeInContentViewCoordinates.height * targetScale);
    CGFloat maxHorizontalOffset = futureContentSizeInSelfCoordinates.width - newBounds.size.width + _obscuredInsets.right;
    contentOffset.x = std::min(contentOffset.x, maxHorizontalOffset);
    CGFloat maxVerticalOffset = futureContentSizeInSelfCoordinates.height - newBounds.size.height + _obscuredInsets.bottom;
    contentOffset.y = std::min(contentOffset.y, maxVerticalOffset);

    contentOffset.x = std::max(contentOffset.x, -_obscuredInsets.left);
    contentOffset.y = std::max(contentOffset.y, -_obscuredInsets.top);

    // Make the top/bottom edges "sticky" within 1 pixel.
    if (oldUnobscuredContentRect.maxY() > contentSizeInContentViewCoordinates.height - 1)
        contentOffset.y = maxVerticalOffset;
    if (oldUnobscuredContentRect.y() < 1)
        contentOffset.y = -_obscuredInsets.top;

    // FIXME: if we have content centered after double tap to zoom, we should also try to keep that rect in view.
    [_scrollView setContentSize:roundScrollViewContentSize(*_page, futureContentSizeInSelfCoordinates)];
    [_scrollView setContentOffset:contentOffset];

    CGRect visibleRectInContentCoordinates = [self convertRect:newBounds toView:_contentView.get()];
    CGRect unobscuredRectInContentCoordinates = [self convertRect:futureUnobscuredRectInSelfCoordinates toView:_contentView.get()];

    UIEdgeInsets unobscuredSafeAreaInsets = [self _computedUnobscuredSafeAreaInset];
    WebCore::FloatBoxExtent unobscuredSafeAreaInsetsExtent(unobscuredSafeAreaInsets.top, unobscuredSafeAreaInsets.right, unobscuredSafeAreaInsets.bottom, unobscuredSafeAreaInsets.left);

    _lastSentMinimumLayoutSize = newMinimumLayoutSize;
    _lastSentMaximumUnobscuredSize = newMaximumUnobscuredSize;
    _lastSentDeviceOrientation = newOrientation;

    _page->dynamicViewportSizeUpdate(newMinimumLayoutSize, newMaximumUnobscuredSize, visibleRectInContentCoordinates, unobscuredRectInContentCoordinates, futureUnobscuredRectInSelfCoordinates, unobscuredSafeAreaInsetsExtent, targetScale, newOrientation);
    if (WebKit::DrawingAreaProxy* drawingArea = _page->drawingArea())
        drawingArea->setSize(WebCore::IntSize(newBounds.size));
}

- (void)_endAnimatedResize
{
    LOG_WITH_STREAM(VisibleRects, stream << "-[WKWebView " << _page->pageID() << " _endAnimatedResize:] " << " _dynamicViewportUpdateMode " << (int)_dynamicViewportUpdateMode);

    if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::NotResizing)
        return;

    _page->synchronizeDynamicViewportUpdate();

    // synchronizeDynamicViewportUpdate() may cause this function to re-enter via _didCommitLayerTree, so check _dynamicViewportUpdateMode again.
    if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::NotResizing)
        return;

    if (!_resizeAnimationView) {
        // Paranoia. If _resizeAnimationView is null we'll end up setting a zero scale on the content view.
        RELEASE_LOG_IF_ALLOWED("%p -[WKWebView _endAnimatedResize:] - _resizeAnimationView is nil", self);
        _dynamicViewportUpdateMode = DynamicViewportUpdateMode::NotResizing;
        [_contentView setHidden:NO];
        return;
    }

    --_activeAnimatedResizeCount;
    NSUInteger indexOfResizeAnimationView = [[_scrollView subviews] indexOfObject:_resizeAnimationView.get()];
    [_scrollView insertSubview:_contentView.get() atIndex:indexOfResizeAnimationView];
    [_scrollView insertSubview:[_contentView unscaledView] atIndex:indexOfResizeAnimationView + 1];

    CALayer *contentViewLayer = [_contentView layer];
    CGFloat adjustmentScale = _resizeAnimationTransformAdjustments.m11;
    contentViewLayer.sublayerTransform = CATransform3DIdentity;

    CGFloat animatingScaleTarget = [[_resizeAnimationView layer] transform].m11;
    CALayer *contentLayer = [_contentView layer];
    CATransform3D contentLayerTransform = contentLayer.transform;
    CGFloat currentScale = [[_resizeAnimationView layer] transform].m11 * contentLayerTransform.m11;

    // We cannot use [UIScrollView setZoomScale:] directly because the UIScrollView delegate would get a callback with
    // an invalid contentOffset. The real content offset is only set below.
    // Since there is no public API for setting both the zoomScale and the contentOffset, we set the zoomScale manually
    // on the zoom layer and then only change the contentOffset.
    CGFloat adjustedScale = adjustmentScale * currentScale;
    contentLayerTransform.m11 = adjustedScale;
    contentLayerTransform.m22 = adjustedScale;
    contentLayer.transform = contentLayerTransform;

    CGPoint currentScrollOffset = [_scrollView contentOffset];
    double horizontalScrollAdjustement = _resizeAnimationTransformAdjustments.m41 * animatingScaleTarget;
    double verticalScrollAdjustment = _resizeAnimationTransformAdjustments.m42 * animatingScaleTarget;

    [_scrollView setContentSize:roundScrollViewContentSize(*_page, [_contentView frame].size)];
    [_scrollView setContentOffset:CGPointMake(currentScrollOffset.x - horizontalScrollAdjustement, currentScrollOffset.y - verticalScrollAdjustment)];

    [_resizeAnimationView removeFromSuperview];
    _resizeAnimationView = nil;
    _resizeAnimationTransformAdjustments = CATransform3DIdentity;

    _dynamicViewportUpdateMode = DynamicViewportUpdateMode::NotResizing;
    [_contentView setHidden:NO];
    [self _scheduleVisibleContentRectUpdate];

    CGRect newBounds = self.bounds;
    WebCore::FloatSize newMinimumLayoutSize = activeMinimumLayoutSize(self, newBounds);
    WebCore::FloatSize newMaximumUnobscuredSize = activeMaximumUnobscuredSize(self, newBounds);
    int32_t newOrientation = activeOrientation(self);

    if (!_lastSentMinimumLayoutSize || newMinimumLayoutSize != _lastSentMinimumLayoutSize.value())
        [self _dispatchSetMinimumLayoutSize:WebCore::FloatSize(newMinimumLayoutSize)];
    if (!_lastSentMaximumUnobscuredSize || newMaximumUnobscuredSize != _lastSentMaximumUnobscuredSize.value())
        [self _dispatchSetMaximumUnobscuredSize:WebCore::FloatSize(newMaximumUnobscuredSize)];
    if (!_lastSentDeviceOrientation || newOrientation != _lastSentDeviceOrientation.value())
        [self _dispatchSetDeviceOrientation:newOrientation];

    while (!_snapshotsDeferredDuringResize.isEmpty())
        _snapshotsDeferredDuringResize.takeLast()();
}

- (void)_resizeWhileHidingContentWithUpdates:(void (^)(void))updateBlock
{
    LOG_WITH_STREAM(VisibleRects, stream << "-[WKWebView " << _page->pageID() << " _resizeWhileHidingContentWithUpdates:]");
    [self _beginAnimatedResizeWithUpdates:updateBlock];
    if (_dynamicViewportUpdateMode == DynamicViewportUpdateMode::ResizingWithAnimation) {
        [_contentView setHidden:YES];
        _dynamicViewportUpdateMode = DynamicViewportUpdateMode::ResizingWithDocumentHidden;
    }
}

- (void)_setOverlaidAccessoryViewsInset:(CGSize)inset
{
    [_customContentView web_setOverlaidAccessoryViewsInset:inset];
}

- (void)_snapshotRect:(CGRect)rectInViewCoordinates intoImageOfWidth:(CGFloat)imageWidth completionHandler:(void(^)(CGImageRef))completionHandler
{
    if (_dynamicViewportUpdateMode != DynamicViewportUpdateMode::NotResizing) {
        // Defer snapshotting until after the current resize completes.
        void (^copiedCompletionHandler)(CGImageRef) = [completionHandler copy];
        RetainPtr<WKWebView> retainedSelf = self;
        _snapshotsDeferredDuringResize.append([retainedSelf, rectInViewCoordinates, imageWidth, copiedCompletionHandler] {
            [retainedSelf _snapshotRect:rectInViewCoordinates intoImageOfWidth:imageWidth completionHandler:copiedCompletionHandler];
            [copiedCompletionHandler release];
        });
        return;
    }

    CGRect snapshotRectInContentCoordinates = [self convertRect:rectInViewCoordinates toView:self._currentContentView];
    CGFloat imageScale = imageWidth / snapshotRectInContentCoordinates.size.width;
    CGFloat imageHeight = imageScale * snapshotRectInContentCoordinates.size.height;
    CGSize imageSize = CGSizeMake(imageWidth, imageHeight);

#if USE(IOSURFACE)
    // If we are parented and thus won't incur a significant penalty from paging in tiles, snapshot the view hierarchy directly.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    if (CADisplay *display = self.window.screen._display) {
#pragma clang diagnostic pop
        auto surface = WebCore::IOSurface::create(WebCore::expandedIntSize(WebCore::FloatSize(imageSize)), WebCore::sRGBColorSpaceRef());
        if (!surface) {
            completionHandler(nullptr);
            return;
        }
        CGFloat imageScaleInViewCoordinates = imageWidth / rectInViewCoordinates.size.width;
        CATransform3D transform = CATransform3DMakeScale(imageScaleInViewCoordinates, imageScaleInViewCoordinates, 1);
        transform = CATransform3DTranslate(transform, -rectInViewCoordinates.origin.x, -rectInViewCoordinates.origin.y, 0);
        CARenderServerRenderDisplayLayerWithTransformAndTimeOffset(MACH_PORT_NULL, (CFStringRef)display.name, self.layer.context.contextId, reinterpret_cast<uint64_t>(self.layer), surface->surface(), 0, 0, &transform, 0);
        completionHandler(WebCore::IOSurface::sinkIntoImage(WTFMove(surface)).get());
        return;
    }
#endif

    if (_customContentView) {
        UIGraphicsBeginImageContextWithOptions(imageSize, YES, 1);

        UIView *customContentView = _customContentView.get();
        [customContentView.backgroundColor set];
        UIRectFill(CGRectMake(0, 0, imageWidth, imageHeight));

        CGContextRef context = UIGraphicsGetCurrentContext();
        CGContextTranslateCTM(context, -snapshotRectInContentCoordinates.origin.x * imageScale, -snapshotRectInContentCoordinates.origin.y * imageScale);
        CGContextScaleCTM(context, imageScale, imageScale);
        [customContentView.layer renderInContext:context];

        completionHandler([UIGraphicsGetImageFromCurrentImageContext() CGImage]);

        UIGraphicsEndImageContext();
        return;
    }

    void(^copiedCompletionHandler)(CGImageRef) = [completionHandler copy];
    _page->takeSnapshot(WebCore::enclosingIntRect(snapshotRectInContentCoordinates), WebCore::expandedIntSize(WebCore::FloatSize(imageSize)), WebKit::SnapshotOptionsExcludeDeviceScaleFactor, [=](const WebKit::ShareableBitmap::Handle& imageHandle, WebKit::CallbackBase::Error) {
        if (imageHandle.isNull()) {
            copiedCompletionHandler(nullptr);
            [copiedCompletionHandler release];
            return;
        }

        RefPtr<WebKit::ShareableBitmap> bitmap = WebKit::ShareableBitmap::create(imageHandle, WebKit::SharedMemory::Protection::ReadOnly);

        if (!bitmap) {
            copiedCompletionHandler(nullptr);
            [copiedCompletionHandler release];
            return;
        }

        RetainPtr<CGImageRef> cgImage;
        cgImage = bitmap->makeCGImage();
        copiedCompletionHandler(cgImage.get());
        [copiedCompletionHandler release];
    });
}

- (void)_overrideLayoutParametersWithMinimumLayoutSize:(CGSize)minimumLayoutSize minimumLayoutSizeForMinimalUI:(CGSize)minimumLayoutSizeForMinimalUI maximumUnobscuredSizeOverride:(CGSize)maximumUnobscuredSizeOverride
{
    UNUSED_PARAM(minimumLayoutSizeForMinimalUI);
    [self _overrideLayoutParametersWithMinimumLayoutSize:minimumLayoutSize maximumUnobscuredSizeOverride:maximumUnobscuredSizeOverride];
}

- (void)_overrideLayoutParametersWithMinimumLayoutSize:(CGSize)minimumLayoutSize maximumUnobscuredSizeOverride:(CGSize)maximumUnobscuredSizeOverride
{
    [self _setMinimumLayoutSizeOverride:minimumLayoutSize];
    [self _setMaximumUnobscuredSizeOverride:maximumUnobscuredSizeOverride];
}

- (void)_clearOverrideLayoutParameters
{
    _overridesMinimumLayoutSize = NO;
    _minimumLayoutSizeOverride = CGSizeZero;

    _overridesMaximumUnobscuredSize = NO;
    _maximumUnobscuredSizeOverride = CGSizeZero;
}

- (UIView *)_viewForFindUI
{
    return [self viewForZoomingInScrollView:[self scrollView]];
}

- (BOOL)_isDisplayingPDF
{
    return [_customContentView isKindOfClass:[WKPDFView class]];
}

- (NSData *)_dataForDisplayedPDF
{
    if (![self _isDisplayingPDF])
        return nil;
    CGPDFDocumentRef pdfDocument = [(WKPDFView *)_customContentView pdfDocument];
    return [(NSData *)CGDataProviderCopyData(CGPDFDocumentGetDataProvider(pdfDocument)) autorelease];
}

- (NSString *)_suggestedFilenameForDisplayedPDF
{
    if (![self _isDisplayingPDF])
        return nil;
    return [(WKPDFView *)_customContentView.get() suggestedFilename];
}

- (_WKWebViewPrintFormatter *)_webViewPrintFormatter
{
    UIViewPrintFormatter *viewPrintFormatter = self.viewPrintFormatter;
    ASSERT([viewPrintFormatter isKindOfClass:[_WKWebViewPrintFormatter class]]);
    return (_WKWebViewPrintFormatter *)viewPrintFormatter;
}

static WebCore::UserInterfaceLayoutDirection toUserInterfaceLayoutDirection(UISemanticContentAttribute contentAttribute)
{
    auto direction = [UIView userInterfaceLayoutDirectionForSemanticContentAttribute:contentAttribute];
    switch (direction) {
    case UIUserInterfaceLayoutDirectionLeftToRight:
        return WebCore::UserInterfaceLayoutDirection::LTR;
    case UIUserInterfaceLayoutDirectionRightToLeft:
        return WebCore::UserInterfaceLayoutDirection::RTL;
    }

    ASSERT_NOT_REACHED();
    return WebCore::UserInterfaceLayoutDirection::LTR;
}

- (void)setSemanticContentAttribute:(UISemanticContentAttribute)contentAttribute
{
    [super setSemanticContentAttribute:contentAttribute];

    _page->setUserInterfaceLayoutDirection(toUserInterfaceLayoutDirection(contentAttribute));
}

#else // #if PLATFORM(IOS)

#pragma mark - OS X-specific methods

- (BOOL)_drawsBackground
{
    return _impl->drawsBackground();
}

- (void)_setDrawsBackground:(BOOL)drawsBackground
{
    _impl->setDrawsBackground(drawsBackground);
}

- (void)_setDrawsTransparentBackground:(BOOL)drawsTransparentBackground
{
    static BOOL hasLoggedDeprecationWarning;
    if (!hasLoggedDeprecationWarning) {
        // See bug 155550 for details.
        NSLog(@"-[WKWebView _setDrawsTransparentBackground:] is deprecated and should not be used.");
        hasLoggedDeprecationWarning = YES;
    }
    [self _setDrawsBackground:!drawsTransparentBackground];
}

- (NSView *)_inspectorAttachmentView
{
    return _impl->inspectorAttachmentView();
}

- (void)_setInspectorAttachmentView:(NSView *)newView
{
    _impl->setInspectorAttachmentView(newView);
}

- (void)_setOverlayScrollbarStyle:(_WKOverlayScrollbarStyle)scrollbarStyle
{
    _impl->setOverlayScrollbarStyle(toCoreScrollbarStyle(scrollbarStyle));
}

- (_WKOverlayScrollbarStyle)_overlayScrollbarStyle
{
    return toAPIScrollbarStyle(_impl->overlayScrollbarStyle());
}

- (BOOL)_windowOcclusionDetectionEnabled
{
    return _impl->windowOcclusionDetectionEnabled();
}

- (void)_setWindowOcclusionDetectionEnabled:(BOOL)enabled
{
    _impl->setWindowOcclusionDetectionEnabled(enabled);
}

- (void)_setOverrideDeviceScaleFactor:(CGFloat)deviceScaleFactor
{
    _impl->setOverrideDeviceScaleFactor(deviceScaleFactor);
}

- (CGFloat)_overrideDeviceScaleFactor
{
    return _impl->overrideDeviceScaleFactor();
}

- (void)_setTopContentInset:(CGFloat)contentInset
{
    return _impl->setTopContentInset(contentInset);
}

- (CGFloat)_topContentInset
{
    return _impl->topContentInset();
}

- (NSColor *)_pageExtendedBackgroundColor
{
    return _impl->pageExtendedBackgroundColor();
}

- (_WKRectEdge)_pinnedState
{
    return _impl->pinnedState();
}

- (_WKRectEdge)_rubberBandingEnabled
{
    return _impl->rubberBandingEnabled();
}

- (void)_setRubberBandingEnabled:(_WKRectEdge)state
{
    _impl->setRubberBandingEnabled(state);
}

- (id)_immediateActionAnimationControllerForHitTestResult:(_WKHitTestResult *)hitTestResult withType:(_WKImmediateActionType)type userData:(id<NSSecureCoding>)userData
{
    return nil;
}

- (void)_setAutomaticallyAdjustsContentInsets:(BOOL)automaticallyAdjustsContentInsets
{
    _impl->setAutomaticallyAdjustsContentInsets(automaticallyAdjustsContentInsets);
}

- (BOOL)_automaticallyAdjustsContentInsets
{
    return _impl->automaticallyAdjustsContentInsets();
}

- (CGFloat)_minimumLayoutWidth
{
    return _page->minimumLayoutSize().width();
}

- (void)_setMinimumLayoutWidth:(CGFloat)width
{
    BOOL expandsToFit = width > 0;

    _page->setMinimumLayoutSize(WebCore::IntSize(width, 0));
    _page->setMainFrameIsScrollable(!expandsToFit);

    _impl->setClipsToVisibleRect(expandsToFit);
}

- (BOOL)_shouldExpandContentToViewHeightForAutoLayout
{
    return _impl->shouldExpandToViewHeightForAutoLayout();
}

- (void)_setShouldExpandContentToViewHeightForAutoLayout:(BOOL)shouldExpand
{
    return _impl->setShouldExpandToViewHeightForAutoLayout(shouldExpand);
}

- (BOOL)_alwaysShowsHorizontalScroller
{
    return _page->alwaysShowsHorizontalScroller();
}

- (void)_setAlwaysShowsHorizontalScroller:(BOOL)alwaysShowsHorizontalScroller
{
    _page->setAlwaysShowsHorizontalScroller(alwaysShowsHorizontalScroller);
}

- (BOOL)_alwaysShowsVerticalScroller
{
    return _page->alwaysShowsVerticalScroller();
}

- (void)_setAlwaysShowsVerticalScroller:(BOOL)alwaysShowsVerticalScroller
{
    _page->setAlwaysShowsVerticalScroller(alwaysShowsVerticalScroller);
}

- (NSPrintOperation *)_printOperationWithPrintInfo:(NSPrintInfo *)printInfo
{
    if (auto webFrameProxy = _page->mainFrame())
        return _impl->printOperationWithPrintInfo(printInfo, *webFrameProxy);
    return nil;
}

- (NSPrintOperation *)_printOperationWithPrintInfo:(NSPrintInfo *)printInfo forFrame:(_WKFrameHandle *)frameHandle
{
    if (auto* webFrameProxy = _page->process().webFrame(frameHandle._frameID))
        return _impl->printOperationWithPrintInfo(printInfo, *webFrameProxy);
    return nil;
}

- (void)setUserInterfaceLayoutDirection:(NSUserInterfaceLayoutDirection)userInterfaceLayoutDirection
{
    [super setUserInterfaceLayoutDirection:userInterfaceLayoutDirection];

    _impl->setUserInterfaceLayoutDirection(userInterfaceLayoutDirection);
}

- (BOOL)_wantsMediaPlaybackControlsView
{
#if HAVE(TOUCH_BAR)
    return _impl->clientWantsMediaPlaybackControlsView();
#else
    return NO;
#endif
}

- (void)_setWantsMediaPlaybackControlsView:(BOOL)wantsMediaPlaybackControlsView
{
#if HAVE(TOUCH_BAR)
    _impl->setClientWantsMediaPlaybackControlsView(wantsMediaPlaybackControlsView);
#endif
}

- (id)_mediaPlaybackControlsView
{
#if HAVE(TOUCH_BAR)
    return _impl->clientWantsMediaPlaybackControlsView() ? _impl->mediaPlaybackControlsView() : nil;
#else
    return nil;
#endif
}

// This method is for subclasses to override.
- (void)_addMediaPlaybackControlsView:(id)mediaPlaybackControlsView
{
}

// This method is for subclasses to override.
- (void)_removeMediaPlaybackControlsView
{
}

- (void)_prepareForMoveToWindow:(NSWindow *)targetWindow completionHandler:(void(^)(void))completionHandler
{
    auto completionHandlerCopy = makeBlockPtr(completionHandler);
    _impl->prepareForMoveToWindow(targetWindow, [completionHandlerCopy] {
        completionHandlerCopy();
    });
}

- (void)_setThumbnailView:(_WKThumbnailView *)thumbnailView
{
    _impl->setThumbnailView(thumbnailView);
}

- (_WKThumbnailView *)_thumbnailView
{
    if (!_impl)
        return nil;
    return _impl->thumbnailView();
}

- (void)_setIgnoresAllEvents:(BOOL)ignoresAllEvents
{
    _impl->setIgnoresAllEvents(ignoresAllEvents);
}

- (BOOL)_ignoresAllEvents
{
    return _impl->ignoresAllEvents();
}

#endif

@end


@implementation WKWebView (WKTesting)

- (NSDictionary *)_contentsOfUserInterfaceItem:(NSString *)userInterfaceItem
{
    if ([userInterfaceItem isEqualToString:@"validationBubble"]) {
        auto* validationBubble = _page->validationBubble();
        String message = validationBubble ? validationBubble->message() : emptyString();
        double fontSize = validationBubble ? validationBubble->fontSize() : 0;
        return @{ userInterfaceItem: @{ @"message": (NSString *)message, @"fontSize": [NSNumber numberWithDouble:fontSize] } };
    }

#if PLATFORM(IOS)
    return [_contentView _contentsOfUserInterfaceItem:(NSString *)userInterfaceItem];
#else
    return nil;
#endif
}

#if PLATFORM(IOS)
- (void)_requestActivatedElementAtPosition:(CGPoint)position completionBlock:(void (^)(_WKActivatedElementInfo *))block
{
    auto infoRequest = WebKit::InteractionInformationRequest(WebCore::roundedIntPoint(position));
    infoRequest.includeSnapshot = true;

    [_contentView doAfterPositionInformationUpdate:[capturedBlock = makeBlockPtr(block)] (WebKit::InteractionInformationAtPosition information) {
        capturedBlock([_WKActivatedElementInfo activatedElementInfoWithInteractionInformationAtPosition:information]);
    } forRequest:infoRequest];
}

- (void)_accessibilityRetrieveRectsAtSelectionOffset:(NSInteger)offset withText:(NSString *)text completionHandler:(void (^)(NSArray<NSValue *> *rects))completionHandler
{
    [_contentView _accessibilityRetrieveRectsAtSelectionOffset:offset withText:text completionHandler:[capturedCompletionHandler = makeBlockPtr(completionHandler)] (const Vector<WebCore::SelectionRect>& selectionRects) {
        if (!capturedCompletionHandler)
            return;
        auto rectValues = adoptNS([[NSMutableArray alloc] initWithCapacity:selectionRects.size()]);
        for (auto& selectionRect : selectionRects)
            [rectValues addObject:[NSValue valueWithCGRect:selectionRect.rect()]];
        capturedCompletionHandler(rectValues.get());
    }];
}

- (void)_accessibilityStoreSelection
{
    [_contentView _accessibilityStoreSelection];
}

- (void)_accessibilityClearSelection
{
    [_contentView _accessibilityClearSelection];
}

- (CGRect)_contentVisibleRect
{
    return [self convertRect:[self bounds] toView:self._currentContentView];
}

- (CGPoint)_convertPointFromContentsToView:(CGPoint)point
{
    return [self convertPoint:point fromView:self._currentContentView];
}

- (CGPoint)_convertPointFromViewToContents:(CGPoint)point
{
    return [self convertPoint:point toView:self._currentContentView];
}

- (void)keyboardAccessoryBarNext
{
    [_contentView accessoryTab:YES];
}

- (void)keyboardAccessoryBarPrevious
{
    [_contentView accessoryTab:NO];
}

- (void)applyAutocorrection:(NSString *)newString toString:(NSString *)oldString withCompletionHandler:(void (^)(void))completionHandler
{
    [_contentView applyAutocorrection:newString toString:oldString withCompletionHandler:[capturedCompletionHandler = makeBlockPtr(completionHandler)] (UIWKAutocorrectionRects *rects) {
        capturedCompletionHandler();
    }];
}

- (void)dismissFormAccessoryView
{
    [_contentView accessoryDone];
}

- (void)selectFormAccessoryPickerRow:(int)rowIndex
{
    [_contentView selectFormAccessoryPickerRow:rowIndex];
}

- (void)didStartFormControlInteraction
{
    // For subclasses to override.
}

- (void)didEndFormControlInteraction
{
    // For subclasses to override.
}

- (void)_didShowForcePressPreview
{
    // For subclasses to override.
}

- (void)_didDismissForcePressPreview
{
    // For subclasses to override.
}

- (CGRect)_uiTextCaretRect
{
    // Force the selection view to update if needed.
    [_contentView _updateChangedSelection];

    return [[_contentView valueForKeyPath:@"interactionAssistant.selectionView.selection.caretRect"] CGRectValue];
}

- (CGRect)_inputViewBounds
{
    return _inputViewBounds;
}

- (NSArray<NSValue *> *)_uiTextSelectionRects
{
    // Force the selection view to update if needed.
    [_contentView _updateChangedSelection];

    return [_contentView _uiTextSelectionRects];
}

- (NSString *)_scrollingTreeAsText
{
    WebKit::RemoteScrollingCoordinatorProxy* coordinator = _page->scrollingCoordinatorProxy();
    if (!coordinator)
        return @"";

    return coordinator->scrollingTreeAsText();
}

- (NSNumber *)_stableStateOverride
{
    // For subclasses to override.
    return nil;
}

- (void)_doAfterNextStablePresentationUpdate:(dispatch_block_t)updateBlock
{
    if (![self usesStandardContentView]) {
        dispatch_async(dispatch_get_main_queue(), updateBlock);
        return;
    }

    auto updateBlockCopy = makeBlockPtr(updateBlock);

    if (_stableStatePresentationUpdateCallbacks)
        [_stableStatePresentationUpdateCallbacks addObject:updateBlockCopy.get()];
    else {
        _stableStatePresentationUpdateCallbacks = adoptNS([[NSMutableArray alloc] initWithObjects:updateBlockCopy.get(), nil]);
        [self _firePresentationUpdateForPendingStableStatePresentationCallbacks];
    }
}

- (void)_firePresentationUpdateForPendingStableStatePresentationCallbacks
{
    RetainPtr<WKWebView> strongSelf = self;
    [self _doAfterNextPresentationUpdate:[strongSelf] {
        dispatch_async(dispatch_get_main_queue(), [strongSelf] {
            if ([strongSelf->_stableStatePresentationUpdateCallbacks count])
                [strongSelf _firePresentationUpdateForPendingStableStatePresentationCallbacks];
        });
    }];
}

- (NSDictionary *)_propertiesOfLayerWithID:(unsigned long long)layerID
{
    CALayer* layer = asLayer(downcast<WebKit::RemoteLayerTreeDrawingAreaProxy>(*_page->drawingArea()).layerWithIDForTesting(layerID));
    if (!layer)
        return nil;

    return @{
        @"bounds" : @{
            @"x" : @(layer.bounds.origin.x),
            @"y" : @(layer.bounds.origin.x),
            @"width" : @(layer.bounds.size.width),
            @"height" : @(layer.bounds.size.height),

        },
        @"position" : @{
            @"x" : @(layer.position.x),
            @"y" : @(layer.position.y),
        },
        @"zPosition" : @(layer.zPosition),
        @"anchorPoint" : @{
            @"x" : @(layer.anchorPoint.x),
            @"y" : @(layer.anchorPoint.y),
        },
        @"anchorPointZ" : @(layer.anchorPointZ),
        @"transform" : @{
            @"m11" : @(layer.transform.m11),
            @"m12" : @(layer.transform.m12),
            @"m13" : @(layer.transform.m13),
            @"m14" : @(layer.transform.m14),

            @"m21" : @(layer.transform.m21),
            @"m22" : @(layer.transform.m22),
            @"m23" : @(layer.transform.m23),
            @"m24" : @(layer.transform.m24),

            @"m31" : @(layer.transform.m31),
            @"m32" : @(layer.transform.m32),
            @"m33" : @(layer.transform.m33),
            @"m34" : @(layer.transform.m34),

            @"m41" : @(layer.transform.m41),
            @"m42" : @(layer.transform.m42),
            @"m43" : @(layer.transform.m43),
            @"m44" : @(layer.transform.m44),
        },
        @"sublayerTransform" : @{
            @"m11" : @(layer.sublayerTransform.m11),
            @"m12" : @(layer.sublayerTransform.m12),
            @"m13" : @(layer.sublayerTransform.m13),
            @"m14" : @(layer.sublayerTransform.m14),

            @"m21" : @(layer.sublayerTransform.m21),
            @"m22" : @(layer.sublayerTransform.m22),
            @"m23" : @(layer.sublayerTransform.m23),
            @"m24" : @(layer.sublayerTransform.m24),

            @"m31" : @(layer.sublayerTransform.m31),
            @"m32" : @(layer.sublayerTransform.m32),
            @"m33" : @(layer.sublayerTransform.m33),
            @"m34" : @(layer.sublayerTransform.m34),

            @"m41" : @(layer.sublayerTransform.m41),
            @"m42" : @(layer.sublayerTransform.m42),
            @"m43" : @(layer.sublayerTransform.m43),
            @"m44" : @(layer.sublayerTransform.m44),
        },

        @"hidden" : @(layer.hidden),
        @"doubleSided" : @(layer.doubleSided),
        @"masksToBounds" : @(layer.masksToBounds),
        @"contentsScale" : @(layer.contentsScale),
        @"rasterizationScale" : @(layer.rasterizationScale),
        @"opaque" : @(layer.opaque),
        @"opacity" : @(layer.opacity),
    };
}

#endif // PLATFORM(IOS)

#if PLATFORM(MAC)
- (WKPageRef)_pageRefForTransitionToWKWebView
{
    return toAPI(_page.get());
}

- (void)_dismissContentRelativeChildWindows
{
    _impl->dismissContentRelativeChildWindowsFromViewOnly();
}

- (void)_setFrame:(NSRect)rect andScrollBy:(NSSize)offset
{
    _impl->setFrameAndScrollBy(NSRectToCGRect(rect), NSSizeToCGSize(offset));
}

- (void)_setTotalHeightOfBanners:(CGFloat)totalHeightOfBanners
{
    _impl->setTotalHeightOfBanners(totalHeightOfBanners);
}

- (CGFloat)_totalHeightOfBanners
{
    return _impl->totalHeightOfBanners();
}

- (void)_beginDeferringViewInWindowChanges
{
    _impl->beginDeferringViewInWindowChanges();
}

- (void)_endDeferringViewInWindowChanges
{
    _impl->endDeferringViewInWindowChanges();
}

- (void)_endDeferringViewInWindowChangesSync
{
    _impl->endDeferringViewInWindowChangesSync();
}

- (void)_gestureEventWasNotHandledByWebCore:(NSEvent *)event
{
    _impl->gestureEventWasNotHandledByWebCoreFromViewOnly(event);
}

- (void)_setIgnoresNonWheelEvents:(BOOL)ignoresNonWheelEvents
{
    _impl->setIgnoresNonWheelEvents(ignoresNonWheelEvents);
}

- (BOOL)_ignoresNonWheelEvents
{
    return _impl->ignoresNonWheelEvents();
}

- (void)_setCustomSwipeViews:(NSArray *)customSwipeViews
{
    _impl->setCustomSwipeViews(customSwipeViews);
}

- (void)_setCustomSwipeViewsTopContentInset:(float)topContentInset
{
    _impl->setCustomSwipeViewsTopContentInset(topContentInset);
}

- (BOOL)_tryToSwipeWithEvent:(NSEvent *)event ignoringPinnedState:(BOOL)ignoringPinnedState
{
    return _impl->tryToSwipeWithEvent(event, ignoringPinnedState);
}

- (void)_setDidMoveSwipeSnapshotCallback:(void(^)(CGRect))callback
{
    _impl->setDidMoveSwipeSnapshotCallback(callback);
}

- (NSView *)_fullScreenPlaceholderView
{
    return _impl->fullScreenPlaceholderView();
}

- (NSWindow *)_fullScreenWindow
{
    return _impl->fullScreenWindow();
}

- (void)_disableFrameSizeUpdates
{
    _impl->disableFrameSizeUpdates();
}

- (void)_enableFrameSizeUpdates
{
    _impl->enableFrameSizeUpdates();
}

- (void)_prepareForImmediateActionAnimation
{
}

- (void)_cancelImmediateActionAnimation
{
}

- (void)_completeImmediateActionAnimation
{
}

- (BOOL)_canChangeFrameLayout:(_WKFrameHandle *)frameHandle
{
    if (auto* webFrameProxy = _page->process().webFrame(frameHandle._frameID))
        return _impl->canChangeFrameLayout(*webFrameProxy);
    return false;
}

- (NSColor *)_underlayColor
{
    return _impl->underlayColor();
}

- (void)_setUnderlayColor:(NSColor *)underlayColor
{
    _impl->setUnderlayColor(underlayColor);
}

- (BOOL)_hasActiveVideoForControlsManager
{
    return _page && _page->hasActiveVideoForControlsManager();
}

- (void)_requestControlledElementID
{
    if (_page)
        _page->requestControlledElementID();
}

- (void)_handleControlledElementIDResponse:(NSString *)identifier
{
    // Overridden by subclasses.
}

- (void)_handleAcceptedCandidate:(NSTextCheckingResult *)candidate
{
    _impl->handleAcceptedCandidate(candidate);
}

- (void)_didHandleAcceptedCandidate
{
    // Overridden by subclasses.
}

- (void)_didUpdateCandidateListVisibility:(BOOL)visible
{
    // Overridden by subclasses.
}

- (void)_forceRequestCandidates
{
    _impl->forceRequestCandidatesForTesting();
}

- (BOOL)_shouldRequestCandidates
{
    return _impl->shouldRequestCandidates();
}

- (void)_insertText:(id)string replacementRange:(NSRange)replacementRange
{
    [self insertText:string replacementRange:replacementRange];
}

- (NSRect)_candidateRect
{
    return _page->editorState().postLayoutData().selectionClipRect;
}

- (void)_setHeaderBannerHeight:(int)height
{
    _page->setHeaderBannerHeightForTesting(height);
}

- (void)_setFooterBannerHeight:(int)height
{
    _page->setFooterBannerHeightForTesting(height);
}

#endif // PLATFORM(MAC)

- (void)_requestActiveNowPlayingSessionInfo:(void(^)(BOOL, BOOL, NSString*, double, double, NSInteger))callback
{
    if (!_page) {
        callback(NO, NO, @"", std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(), 0);
        return;
    }

    auto handler = makeBlockPtr(callback);
    auto localCallback = WebKit::NowPlayingInfoCallback::create([handler](bool active, bool registeredAsNowPlayingApplication, String title, double duration, double elapsedTime, uint64_t uniqueIdentifier, WebKit::CallbackBase::Error) {
        handler(active, registeredAsNowPlayingApplication, title, duration, elapsedTime, uniqueIdentifier);
    });

    _page->requestActiveNowPlayingSessionInfo(WTFMove(localCallback));
}

- (void)_setPageScale:(CGFloat)scale withOrigin:(CGPoint)origin
{
    _page->scalePage(scale, WebCore::roundedIntPoint(origin));
}

- (CGFloat)_pageScale
{
    return _page->pageScaleFactor();
}

// Execute the supplied block after the next transaction from the WebProcess.
- (void)_doAfterNextPresentationUpdate:(void (^)(void))updateBlock
{
#if PLATFORM(IOS)
    if (![self usesStandardContentView]) {
        dispatch_async(dispatch_get_main_queue(), updateBlock);
        return;
    }
#endif

    auto updateBlockCopy = makeBlockPtr(updateBlock);

    _page->callAfterNextPresentationUpdate([updateBlockCopy](WebKit::CallbackBase::Error error) {
        if (updateBlockCopy)
            updateBlockCopy();
    });
}

- (void)_doAfterNextPresentationUpdateWithoutWaitingForPainting:(void (^)(void))updateBlock
{
#if PLATFORM(IOS)
    if (![self usesStandardContentView]) {
        dispatch_async(dispatch_get_main_queue(), updateBlock);
        return;
    }
#endif

    _page->setShouldSkipWaitingForPaintAfterNextViewDidMoveToWindow(true);
    [self _doAfterNextPresentationUpdate:updateBlock];
}

- (void)_doAfterNextVisibleContentRectUpdate:(void (^)(void))updateBlock
{
#if PLATFORM(IOS)
    _visibleContentRectUpdateCallbacks.append(makeBlockPtr(updateBlock));
    [self _scheduleVisibleContentRectUpdate];
#else
    dispatch_async(dispatch_get_main_queue(), updateBlock);
#endif
}

- (void)_disableBackForwardSnapshotVolatilityForTesting
{
    WebKit::ViewSnapshotStore::singleton().setDisableSnapshotVolatilityForTesting(true);
}

- (void)_executeEditCommand:(NSString *)command argument:(NSString *)argument completion:(void (^)(BOOL))completion
{
    _page->executeEditCommand(command, argument, [capturedCompletionBlock = makeBlockPtr(completion)](WebKit::CallbackBase::Error error) {
        if (capturedCompletionBlock)
            capturedCompletionBlock(error == WebKit::CallbackBase::Error::None);
    });
}

#if PLATFORM(IOS)

- (CGRect)_dragCaretRect
{
#if ENABLE(DRAG_SUPPORT)
    return _page->currentDragCaretRect();
#else
    return CGRectZero;
#endif
}

- (void)_simulateLongPressActionAtLocation:(CGPoint)location
{
    [_contentView _simulateLongPressActionAtLocation:location];
}

#endif // PLATFORM(IOS)

- (BOOL)_beginBackSwipeForTesting
{
#if PLATFORM(MAC)
    return _impl->beginBackSwipeForTesting();
#else
    if (!_gestureController)
        return NO;
    return _gestureController->beginSimulatedSwipeInDirectionForTesting(WebKit::ViewGestureController::SwipeDirection::Back);
#endif
}

- (BOOL)_completeBackSwipeForTesting
{
#if PLATFORM(MAC)
    return _impl->completeBackSwipeForTesting();
#else
    if (!_gestureController)
        return NO;
    return _gestureController->completeSimulatedSwipeInDirectionForTesting(WebKit::ViewGestureController::SwipeDirection::Back);
#endif
}

@end


#if ENABLE(FULLSCREEN_API) && PLATFORM(IOS)

@implementation WKWebView (FullScreenAPI)

- (BOOL)hasFullScreenWindowController
{
    return !!_fullScreenWindowController;
}

- (WKFullScreenWindowController *)fullScreenWindowController
{
    if (!_fullScreenWindowController)
        _fullScreenWindowController = adoptNS([[WKFullScreenWindowController alloc] initWithWebView:self]);

    return _fullScreenWindowController.get();
}

- (void)closeFullScreenWindowController
{
    if (!_fullScreenWindowController)
        return;

    [_fullScreenWindowController close];
    _fullScreenWindowController = nullptr;
}

- (WebCoreFullScreenPlaceholderView *)fullScreenPlaceholderView
{
    if ([_fullScreenWindowController isFullScreen])
        return [_fullScreenWindowController webViewPlaceholder];
    return nil;
}

@end
#endif // ENABLE(FULLSCREEN_API) && PLATFORM(IOS)

#if PLATFORM(MAC)

@implementation WKWebView (WKIBActions)

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
    SEL action = item.action;

    if (action == @selector(goBack:))
        return !!_page->backForwardList().backItem();

    if (action == @selector(goForward:))
        return !!_page->backForwardList().forwardItem();

    if (action == @selector(stopLoading:)) {
        // FIXME: Return no if we're stopped.
        return YES;
    }

    if (action == @selector(reload:) || action == @selector(reloadFromOrigin:)) {
        // FIXME: Return no if we're loading.
        return YES;
    }

    return _impl->validateUserInterfaceItem(item);
}

- (IBAction)goBack:(id)sender
{
    [self goBack];
}

- (IBAction)goForward:(id)sender
{
    [self goForward];
}

- (IBAction)reload:(id)sender
{
    [self reload];
}

- (IBAction)reloadFromOrigin:(id)sender
{
    [self reloadFromOrigin];
}

- (IBAction)stopLoading:(id)sender
{
    _page->stopLoading();
}

@end

#endif // PLATFORM(MAC)

#if PLATFORM(IOS)
@implementation WKWebView (_WKWebViewPrintFormatter)

- (Class)_printFormatterClass
{
    return [_WKWebViewPrintFormatter class];
}

- (id <_WKWebViewPrintProvider>)_printProvider
{
    id contentView = self._currentContentView;
    if ([contentView conformsToProtocol:@protocol(_WKWebViewPrintProvider)])
        return contentView;
    return nil;
}

@end
#endif

@implementation WKWebView (WKDeprecated)

- (NSArray *)certificateChain
{
    auto certificateInfo = _page->pageLoadState().certificateInfo();
    if (!certificateInfo)
        return @[ ];

    return (NSArray *)certificateInfo->certificateInfo().certificateChain() ?: @[ ];
}

@end

@implementation WKWebView (WKBinaryCompatibilityWithIOS10)

- (id <_WKInputDelegate>)_formDelegate
{
    return self._inputDelegate;
}

- (void)_setFormDelegate:(id <_WKInputDelegate>)formDelegate
{
    self._inputDelegate = formDelegate;
}

@end

#endif // WK_API_ENABLED
