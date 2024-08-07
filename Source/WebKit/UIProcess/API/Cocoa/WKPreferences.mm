/*
 * Copyright (C) 2014-2017 Apple Inc. All rights reserved.
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
#import "WKPreferencesInternal.h"

#if WK_API_ENABLED

#import "APIArray.h"
#import "WKNSArray.h"
#import "WebPreferences.h"
#import "_WKExperimentalFeature.h"
#import "_WKExperimentalFeatureInternal.h"
#import <WebCore/SecurityOrigin.h>
#import <WebCore/Settings.h>
#import <wtf/RetainPtr.h>

@implementation WKPreferences

- (instancetype)init
{
    if (!(self = [super init]))
        return nil;

    API::Object::constructInWrapper<WebKit::WebPreferences>(self, String(), "WebKit", "WebKitDebug");
    return self;
}

- (void)dealloc
{
    _preferences->~WebPreferences();

    [super dealloc];
}

+ (BOOL)supportsSecureCoding
{
    return YES;
}

// FIXME: We currently only encode/decode API preferences. We should consider whether we should
// encode/decode SPI preferences as well.

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeDouble:self.minimumFontSize forKey:@"minimumFontSize"];
    [coder encodeBool:self.javaScriptEnabled forKey:@"javaScriptEnabled"];
    [coder encodeBool:self.javaScriptCanOpenWindowsAutomatically forKey:@"javaScriptCanOpenWindowsAutomatically"];

#if PLATFORM(MAC)
    [coder encodeBool:self.javaEnabled forKey:@"javaEnabled"];
    [coder encodeBool:self.plugInsEnabled forKey:@"plugInsEnabled"];
    [coder encodeBool:self.tabFocusesLinks forKey:@"tabFocusesLinks"];
#endif
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    if (!(self = [self init]))
        return nil;

    self.minimumFontSize = [coder decodeDoubleForKey:@"minimumFontSize"];
    self.javaScriptEnabled = [coder decodeBoolForKey:@"javaScriptEnabled"];
    self.javaScriptCanOpenWindowsAutomatically = [coder decodeBoolForKey:@"javaScriptCanOpenWindowsAutomatically"];

#if PLATFORM(MAC)
    self.javaEnabled = [coder decodeBoolForKey:@"javaEnabled"];
    self.plugInsEnabled = [coder decodeBoolForKey:@"plugInsEnabled"];
    self.tabFocusesLinks = [coder decodeBoolForKey:@"tabFocusesLinks"];
#endif

    return self;
}

- (id)copyWithZone:(NSZone *)zone
{
    return wrapper(_preferences->copy().leakRef());
}

- (CGFloat)minimumFontSize
{
    return _preferences->minimumFontSize();
}

- (void)setMinimumFontSize:(CGFloat)minimumFontSize
{
    _preferences->setMinimumFontSize(minimumFontSize);
}

- (BOOL)javaScriptEnabled
{
    return _preferences->javaScriptEnabled();
}

- (void)setJavaScriptEnabled:(BOOL)javaScriptEnabled
{
    _preferences->setJavaScriptEnabled(javaScriptEnabled);
}

- (BOOL)javaScriptCanOpenWindowsAutomatically
{
    return _preferences->javaScriptCanOpenWindowsAutomatically();
}

- (void)setJavaScriptCanOpenWindowsAutomatically:(BOOL)javaScriptCanOpenWindowsAutomatically
{
    _preferences->setJavaScriptCanOpenWindowsAutomatically(javaScriptCanOpenWindowsAutomatically);
}

#pragma mark OS X-specific methods

#if PLATFORM(MAC)

- (BOOL)javaEnabled
{
    return _preferences->javaEnabled();
}

- (void)setJavaEnabled:(BOOL)javaEnabled
{
    _preferences->setJavaEnabled(javaEnabled);
}

- (BOOL)plugInsEnabled
{
    return _preferences->pluginsEnabled();
}

- (void)setPlugInsEnabled:(BOOL)plugInsEnabled
{
    _preferences->setPluginsEnabled(plugInsEnabled);
}

- (BOOL)tabFocusesLinks
{
    return _preferences->tabsToLinks();
}

- (void)setTabFocusesLinks:(BOOL)tabFocusesLinks
{
    _preferences->setTabsToLinks(tabFocusesLinks);
}

#endif

#pragma mark WKObject protocol implementation

- (API::Object&)_apiObject
{
    return *_preferences;
}

@end

@implementation WKPreferences (WKPrivate)

- (BOOL)_telephoneNumberDetectionIsEnabled
{
    return _preferences->telephoneNumberParsingEnabled();
}

- (void)_setTelephoneNumberDetectionIsEnabled:(BOOL)telephoneNumberDetectionIsEnabled
{
    _preferences->setTelephoneNumberParsingEnabled(telephoneNumberDetectionIsEnabled);
}

static WebCore::SecurityOrigin::StorageBlockingPolicy toStorageBlockingPolicy(_WKStorageBlockingPolicy policy)
{
    switch (policy) {
    case _WKStorageBlockingPolicyAllowAll:
        return WebCore::SecurityOrigin::AllowAllStorage;
    case _WKStorageBlockingPolicyBlockThirdParty:
        return WebCore::SecurityOrigin::BlockThirdPartyStorage;
    case _WKStorageBlockingPolicyBlockAll:
        return WebCore::SecurityOrigin::BlockAllStorage;
    }

    ASSERT_NOT_REACHED();
    return WebCore::SecurityOrigin::AllowAllStorage;
}

static _WKStorageBlockingPolicy toAPI(WebCore::SecurityOrigin::StorageBlockingPolicy policy)
{
    switch (policy) {
    case WebCore::SecurityOrigin::AllowAllStorage:
        return _WKStorageBlockingPolicyAllowAll;
    case WebCore::SecurityOrigin::BlockThirdPartyStorage:
        return _WKStorageBlockingPolicyBlockThirdParty;
    case WebCore::SecurityOrigin::BlockAllStorage:
        return _WKStorageBlockingPolicyBlockAll;
    }

    ASSERT_NOT_REACHED();
    return _WKStorageBlockingPolicyAllowAll;
}

- (_WKStorageBlockingPolicy)_storageBlockingPolicy
{
    return toAPI(static_cast<WebCore::SecurityOrigin::StorageBlockingPolicy>(_preferences->storageBlockingPolicy()));
}

- (void)_setStorageBlockingPolicy:(_WKStorageBlockingPolicy)policy
{
    _preferences->setStorageBlockingPolicy(toStorageBlockingPolicy(policy));
}

- (BOOL)_offlineApplicationCacheIsEnabled
{
    return _preferences->offlineWebApplicationCacheEnabled();
}

- (void)_setOfflineApplicationCacheIsEnabled:(BOOL)offlineApplicationCacheIsEnabled
{
    _preferences->setOfflineWebApplicationCacheEnabled(offlineApplicationCacheIsEnabled);
}

- (BOOL)_fullScreenEnabled
{
    return _preferences->fullScreenEnabled();
}

- (void)_setFullScreenEnabled:(BOOL)fullScreenEnabled
{
    _preferences->setFullScreenEnabled(fullScreenEnabled);
}

- (BOOL)_allowsPictureInPictureMediaPlayback
{
    return _preferences->allowsPictureInPictureMediaPlayback();
}

- (void)_setAllowsPictureInPictureMediaPlayback:(BOOL)allowed
{
    _preferences->setAllowsPictureInPictureMediaPlayback(allowed);
}

- (BOOL)_compositingBordersVisible
{
    return _preferences->compositingBordersVisible();
}

- (void)_setCompositingBordersVisible:(BOOL)compositingBordersVisible
{
    _preferences->setCompositingBordersVisible(compositingBordersVisible);
}

- (BOOL)_compositingRepaintCountersVisible
{
    return _preferences->compositingRepaintCountersVisible();
}

- (void)_setCompositingRepaintCountersVisible:(BOOL)repaintCountersVisible
{
    _preferences->setCompositingRepaintCountersVisible(repaintCountersVisible);
}

- (BOOL)_tiledScrollingIndicatorVisible
{
    return _preferences->tiledScrollingIndicatorVisible();
}

- (void)_setTiledScrollingIndicatorVisible:(BOOL)tiledScrollingIndicatorVisible
{
    _preferences->setTiledScrollingIndicatorVisible(tiledScrollingIndicatorVisible);
}

- (BOOL)_resourceUsageOverlayVisible
{
    return _preferences->resourceUsageOverlayVisible();
}

- (void)_setResourceUsageOverlayVisible:(BOOL)resourceUsageOverlayVisible
{
    _preferences->setResourceUsageOverlayVisible(resourceUsageOverlayVisible);
}

- (_WKDebugOverlayRegions)_visibleDebugOverlayRegions
{
    return _preferences->visibleDebugOverlayRegions();
}

- (void)_setVisibleDebugOverlayRegions:(_WKDebugOverlayRegions)regionFlags
{
    _preferences->setVisibleDebugOverlayRegions(regionFlags);
}

- (BOOL)_simpleLineLayoutEnabled
{
    return _preferences->simpleLineLayoutEnabled();
}

- (void)_setSimpleLineLayoutEnabled:(BOOL)simpleLineLayoutEnabled
{
    _preferences->setSimpleLineLayoutEnabled(simpleLineLayoutEnabled);
}

- (BOOL)_simpleLineLayoutDebugBordersEnabled
{
    return _preferences->simpleLineLayoutDebugBordersEnabled();
}

- (void)_setSimpleLineLayoutDebugBordersEnabled:(BOOL)simpleLineLayoutDebugBordersEnabled
{
    _preferences->setSimpleLineLayoutDebugBordersEnabled(simpleLineLayoutDebugBordersEnabled);
}

- (BOOL)_acceleratedDrawingEnabled
{
    return _preferences->acceleratedDrawingEnabled();
}

- (void)_setAcceleratedDrawingEnabled:(BOOL)acceleratedDrawingEnabled
{
    _preferences->setAcceleratedDrawingEnabled(acceleratedDrawingEnabled);
}

- (BOOL)_displayListDrawingEnabled
{
    return _preferences->displayListDrawingEnabled();
}

- (void)_setDisplayListDrawingEnabled:(BOOL)displayListDrawingEnabled
{
    _preferences->setDisplayListDrawingEnabled(displayListDrawingEnabled);
}

- (BOOL)_visualViewportEnabled
{
    return _preferences->visualViewportEnabled();
}

- (void)_setVisualViewportEnabled:(BOOL)_visualViewportEnabled
{
    _preferences->setVisualViewportEnabled(_visualViewportEnabled);
}

- (BOOL)_largeImageAsyncDecodingEnabled
{
    return _preferences->largeImageAsyncDecodingEnabled();
}

- (void)_setLargeImageAsyncDecodingEnabled:(BOOL)_largeImageAsyncDecodingEnabled
{
    _preferences->setLargeImageAsyncDecodingEnabled(_largeImageAsyncDecodingEnabled);
}

- (BOOL)_animatedImageAsyncDecodingEnabled
{
    return _preferences->animatedImageAsyncDecodingEnabled();
}

- (void)_setAnimatedImageAsyncDecodingEnabled:(BOOL)_animatedImageAsyncDecodingEnabled
{
    _preferences->setAnimatedImageAsyncDecodingEnabled(_animatedImageAsyncDecodingEnabled);
}

- (BOOL)_textAutosizingEnabled
{
    return _preferences->textAutosizingEnabled();
}

- (void)_setTextAutosizingEnabled:(BOOL)enabled
{
    _preferences->setTextAutosizingEnabled(enabled);
}

- (BOOL)_subpixelAntialiasedLayerTextEnabled
{
    return _preferences->subpixelAntialiasedLayerTextEnabled();
}

- (void)_setSubpixelAntialiasedLayerTextEnabled:(BOOL)enabled
{
    _preferences->setSubpixelAntialiasedLayerTextEnabled(enabled);
}

- (BOOL)_developerExtrasEnabled
{
    return _preferences->developerExtrasEnabled();
}

- (void)_setDeveloperExtrasEnabled:(BOOL)developerExtrasEnabled
{
    _preferences->setDeveloperExtrasEnabled(developerExtrasEnabled);
}

- (BOOL)_logsPageMessagesToSystemConsoleEnabled
{
    return _preferences->logsPageMessagesToSystemConsoleEnabled();
}

- (void)_setLogsPageMessagesToSystemConsoleEnabled:(BOOL)logsPageMessagesToSystemConsoleEnabled
{
    _preferences->setLogsPageMessagesToSystemConsoleEnabled(logsPageMessagesToSystemConsoleEnabled);
}

- (BOOL)_hiddenPageDOMTimerThrottlingEnabled
{
    return _preferences->hiddenPageDOMTimerThrottlingEnabled();
}

- (void)_setHiddenPageDOMTimerThrottlingEnabled:(BOOL)hiddenPageDOMTimerThrottlingEnabled
{
    _preferences->setHiddenPageDOMTimerThrottlingEnabled(hiddenPageDOMTimerThrottlingEnabled);
}

- (BOOL)_hiddenPageDOMTimerThrottlingAutoIncreases
{
    return _preferences->hiddenPageDOMTimerThrottlingAutoIncreases();
}

- (void)_setHiddenPageDOMTimerThrottlingAutoIncreases:(BOOL)hiddenPageDOMTimerThrottlingAutoIncreases
{
    _preferences->setHiddenPageDOMTimerThrottlingAutoIncreases(hiddenPageDOMTimerThrottlingAutoIncreases);
}

- (BOOL)_pageVisibilityBasedProcessSuppressionEnabled
{
    return _preferences->pageVisibilityBasedProcessSuppressionEnabled();
}

- (void)_setPageVisibilityBasedProcessSuppressionEnabled:(BOOL)pageVisibilityBasedProcessSuppressionEnabled
{
    _preferences->setPageVisibilityBasedProcessSuppressionEnabled(pageVisibilityBasedProcessSuppressionEnabled);
}

- (BOOL)_allowFileAccessFromFileURLs
{
    return _preferences->allowFileAccessFromFileURLs();
}

- (void)_setAllowFileAccessFromFileURLs:(BOOL)allowFileAccessFromFileURLs
{
    _preferences->setAllowFileAccessFromFileURLs(allowFileAccessFromFileURLs);
}

- (_WKJavaScriptRuntimeFlags)_javaScriptRuntimeFlags
{
    return _preferences->javaScriptRuntimeFlags();
}

- (void)_setJavaScriptRuntimeFlags:(_WKJavaScriptRuntimeFlags)javaScriptRuntimeFlags
{
    _preferences->setJavaScriptRuntimeFlags(javaScriptRuntimeFlags);
}

- (BOOL)_isStandalone
{
    return _preferences->standalone();
}

- (void)_setStandalone:(BOOL)standalone
{
    _preferences->setStandalone(standalone);
}

- (BOOL)_diagnosticLoggingEnabled
{
    return _preferences->diagnosticLoggingEnabled();
}

- (void)_setDiagnosticLoggingEnabled:(BOOL)diagnosticLoggingEnabled
{
    _preferences->setDiagnosticLoggingEnabled(diagnosticLoggingEnabled);
}

- (NSUInteger)_defaultFontSize
{
    return _preferences->defaultFontSize();
}

- (void)_setDefaultFontSize:(NSUInteger)defaultFontSize
{
    _preferences->setDefaultFontSize(defaultFontSize);
}

- (NSUInteger)_defaultFixedPitchFontSize
{
    return _preferences->defaultFixedFontSize();
}

- (void)_setDefaultFixedPitchFontSize:(NSUInteger)defaultFixedPitchFontSize
{
    _preferences->setDefaultFixedFontSize(defaultFixedPitchFontSize);
}

- (NSString *)_fixedPitchFontFamily
{
    return _preferences->fixedFontFamily();
}

- (void)_setFixedPitchFontFamily:(NSString *)fixedPitchFontFamily
{
    _preferences->setFixedFontFamily(fixedPitchFontFamily);
}

+ (NSArray<_WKExperimentalFeature *> *)_experimentalFeatures
{
    auto features = WebKit::WebPreferences::experimentalFeatures();
    return [wrapper(API::Array::create(WTFMove(features)).leakRef()) autorelease];
}

- (BOOL)_isEnabledForFeature:(_WKExperimentalFeature *)feature
{
    return _preferences->isEnabledForFeature(*feature->_experimentalFeature);
}

- (void)_setEnabled:(BOOL)value forFeature:(_WKExperimentalFeature *)feature
{
    _preferences->setEnabledForFeature(value, *feature->_experimentalFeature);
}

- (BOOL)_applePayCapabilityDisclosureAllowed
{
#if ENABLE(APPLE_PAY)
    return _preferences->applePayCapabilityDisclosureAllowed();
#else
    return NO;
#endif
}

- (void)_setApplePayCapabilityDisclosureAllowed:(BOOL)applePayCapabilityDisclosureAllowed
{
#if ENABLE(APPLE_PAY)
    _preferences->setApplePayCapabilityDisclosureAllowed(applePayCapabilityDisclosureAllowed);
#endif
}

- (BOOL)_shouldSuppressKeyboardInputDuringProvisionalNavigation
{
    return _preferences->shouldSuppressTextInputFromEditingDuringProvisionalNavigation();
}

- (void)_setShouldSuppressKeyboardInputDuringProvisionalNavigation:(BOOL)shouldSuppress
{
    _preferences->setShouldSuppressTextInputFromEditingDuringProvisionalNavigation(shouldSuppress);
}

- (BOOL)_loadsImagesAutomatically
{
    return _preferences->loadsImagesAutomatically();
}

- (void)_setLoadsImagesAutomatically:(BOOL)loadsImagesAutomatically
{
    _preferences->setLoadsImagesAutomatically(loadsImagesAutomatically);
}

- (BOOL)_peerConnectionEnabled
{
    return _preferences->peerConnectionEnabled();
}

- (void)_setPeerConnectionEnabled:(BOOL)enabled
{
    _preferences->setPeerConnectionEnabled(enabled);
}

- (BOOL)_mediaDevicesEnabled
{
    return _preferences->mediaDevicesEnabled();
}

- (void)_setMediaDevicesEnabled:(BOOL)enabled
{
    _preferences->setMediaDevicesEnabled(enabled);
}

- (BOOL)_screenCaptureEnabled
{
    return _preferences->screenCaptureEnabled();
}

- (void)_setScreenCaptureEnabled:(BOOL)enabled
{
    _preferences->setScreenCaptureEnabled(enabled);
}

- (BOOL)_mockCaptureDevicesEnabled
{
    return _preferences->mockCaptureDevicesEnabled();
}

- (void)_setMockCaptureDevicesEnabled:(BOOL)enabled
{
    _preferences->setMockCaptureDevicesEnabled(enabled);
}

- (BOOL)_mockCaptureDevicesPromptEnabled
{
    return _preferences->mockCaptureDevicesPromptEnabled();
}

- (void)_setMockCaptureDevicesPromptEnabled:(BOOL)enabled
{
    _preferences->setMockCaptureDevicesPromptEnabled(enabled);
}

- (BOOL)_mediaCaptureRequiresSecureConnection
{
    return _preferences->mediaCaptureRequiresSecureConnection();
}

- (void)_setMediaCaptureRequiresSecureConnection:(BOOL)requiresSecureConnection
{
    _preferences->setMediaCaptureRequiresSecureConnection(requiresSecureConnection);
}

- (double)_inactiveMediaCaptureSteamRepromptIntervalInMinutes
{
    return _preferences->inactiveMediaCaptureSteamRepromptIntervalInMinutes();
}

- (void)_setInactiveMediaCaptureSteamRepromptIntervalInMinutes:(double)interval
{
    _preferences->setInactiveMediaCaptureSteamRepromptIntervalInMinutes(interval);
}

- (BOOL)_enumeratingAllNetworkInterfacesEnabled
{
    return _preferences->enumeratingAllNetworkInterfacesEnabled();
}

- (void)_setEnumeratingAllNetworkInterfacesEnabled:(BOOL)enabled
{
    _preferences->setEnumeratingAllNetworkInterfacesEnabled(enabled);
}

- (BOOL)_iceCandidateFilteringEnabled
{
    return _preferences->iceCandidateFilteringEnabled();
}

- (void)_setICECandidateFilteringEnabled:(BOOL)enabled
{
    _preferences->setICECandidateFilteringEnabled(enabled);
}

- (BOOL)_webRTCLegacyAPIEnabled
{
    return _preferences->webRTCLegacyAPIEnabled();
}

- (void)_setWebRTCLegacyAPIEnabled:(BOOL)enabled
{
    _preferences->setWebRTCLegacyAPIEnabled(enabled);
}

- (void)_setJavaScriptCanAccessClipboard:(BOOL)javaScriptCanAccessClipboard
{
    _preferences->setJavaScriptCanAccessClipboard(javaScriptCanAccessClipboard);
}

- (BOOL)_shouldAllowUserInstalledFonts
{
    return _preferences->shouldAllowUserInstalledFonts();
}

- (void)_setShouldAllowUserInstalledFonts:(BOOL)_shouldAllowUserInstalledFonts
{
    _preferences->setShouldAllowUserInstalledFonts(_shouldAllowUserInstalledFonts);
}

static _WKEditableLinkBehavior toAPI(WebCore::EditableLinkBehavior behavior)
{
    switch (behavior) {
    case WebCore::EditableLinkDefaultBehavior:
        return _WKEditableLinkBehaviorDefault;
    case WebCore::EditableLinkAlwaysLive:
        return _WKEditableLinkBehaviorAlwaysLive;
    case WebCore::EditableLinkOnlyLiveWithShiftKey:
        return _WKEditableLinkBehaviorOnlyLiveWithShiftKey;
    case WebCore::EditableLinkLiveWhenNotFocused:
        return _WKEditableLinkBehaviorLiveWhenNotFocused;
    case WebCore::EditableLinkNeverLive:
        return _WKEditableLinkBehaviorNeverLive;
    }
    
    ASSERT_NOT_REACHED();
    return _WKEditableLinkBehaviorNeverLive;
}

static WebCore::EditableLinkBehavior toEditableLinkBehavior(_WKEditableLinkBehavior wkBehavior)
{
    switch (wkBehavior) {
    case _WKEditableLinkBehaviorDefault:
        return WebCore::EditableLinkDefaultBehavior;
    case _WKEditableLinkBehaviorAlwaysLive:
        return WebCore::EditableLinkAlwaysLive;
    case _WKEditableLinkBehaviorOnlyLiveWithShiftKey:
        return WebCore::EditableLinkOnlyLiveWithShiftKey;
    case _WKEditableLinkBehaviorLiveWhenNotFocused:
        return WebCore::EditableLinkLiveWhenNotFocused;
    case _WKEditableLinkBehaviorNeverLive:
        return WebCore::EditableLinkNeverLive;
    }
    
    ASSERT_NOT_REACHED();
    return WebCore::EditableLinkNeverLive;
}

- (_WKEditableLinkBehavior)_editableLinkBehavior
{
    return toAPI(static_cast<WebCore::EditableLinkBehavior>(_preferences->editableLinkBehavior()));
}

- (void)_setEditableLinkBehavior:(_WKEditableLinkBehavior)editableLinkBehavior
{
    _preferences->setEditableLinkBehavior(toEditableLinkBehavior(editableLinkBehavior));
}

#if PLATFORM(MAC)
- (void)_setJavaEnabledForLocalFiles:(BOOL)enabled
{
    _preferences->setJavaEnabledForLocalFiles(enabled);
}

- (BOOL)_javaEnabledForLocalFiles
{
    return _preferences->javaEnabledForLocalFiles();
}

- (void)_setCanvasUsesAcceleratedDrawing:(BOOL)enabled
{
    _preferences->setCanvasUsesAcceleratedDrawing(enabled);
}

- (BOOL)_canvasUsesAcceleratedDrawing
{
    return _preferences->canvasUsesAcceleratedDrawing();
}

- (void)_setAcceleratedCompositingEnabled:(BOOL)enabled
{
    _preferences->setAcceleratedCompositingEnabled(enabled);
}

- (BOOL)_acceleratedCompositingEnabled
{
    return _preferences->acceleratedCompositingEnabled();
}

- (void)_setDefaultTextEncodingName:(NSString *)name
{
    _preferences->setDefaultTextEncodingName(name);
}

- (NSString *)_defaultTextEncodingName
{
    return _preferences->defaultTextEncodingName();
}

- (void)_setNeedsSiteSpecificQuirks:(BOOL)enabled
{
    _preferences->setNeedsSiteSpecificQuirks(enabled);
}

- (BOOL)_needsSiteSpecificQuirks
{
    return _preferences->needsSiteSpecificQuirks();
}

- (void)_setAuthorAndUserStylesEnabled:(BOOL)enabled
{
    _preferences->setAuthorAndUserStylesEnabled(enabled);
}

- (BOOL)_authorAndUserStylesEnabled
{
    return _preferences->authorAndUserStylesEnabled();
}

- (void)_setDOMTimersThrottlingEnabled:(BOOL)enabled
{
    _preferences->setDOMTimersThrottlingEnabled(enabled);
}

- (BOOL)_domTimersThrottlingEnabled
{
    return _preferences->domTimersThrottlingEnabled();
}

- (void)_setWebArchiveDebugModeEnabled:(BOOL)enabled
{
    _preferences->setWebArchiveDebugModeEnabled(enabled);
}

- (BOOL)_webArchiveDebugModeEnabled
{
    return _preferences->webArchiveDebugModeEnabled();
}

- (void)_setLocalFileContentSniffingEnabled:(BOOL)enabled
{
    _preferences->setLocalFileContentSniffingEnabled(enabled);
}

- (BOOL)_localFileContentSniffingEnabled
{
    return _preferences->localFileContentSniffingEnabled();
}

- (void)_setUsesPageCache:(BOOL)enabled
{
    _preferences->setUsesPageCache(enabled);
}

- (BOOL)_usesPageCache
{
    return _preferences->usesPageCache();
}

- (void)_setPageCacheSupportsPlugins:(BOOL)enabled
{
    _preferences->setPageCacheSupportsPlugins(enabled);
}

- (BOOL)_pageCacheSupportsPlugins
{
    return _preferences->pageCacheSupportsPlugins();
}

- (void)_setShouldPrintBackgrounds:(BOOL)enabled
{
    _preferences->setShouldPrintBackgrounds(enabled);
}

- (BOOL)_shouldPrintBackgrounds
{
    return _preferences->shouldPrintBackgrounds();
}

- (void)_setWebSecurityEnabled:(BOOL)enabled
{
    _preferences->setWebSecurityEnabled(enabled);
}

- (BOOL)_webSecurityEnabled
{
    return _preferences->webSecurityEnabled();
}

- (void)_setUniversalAccessFromFileURLsAllowed:(BOOL)enabled
{
    _preferences->setAllowUniversalAccessFromFileURLs(enabled);
}

- (BOOL)_universalAccessFromFileURLsAllowed
{
    return _preferences->allowUniversalAccessFromFileURLs();
}

- (void)_setAVFoundationEnabled:(BOOL)enabled
{
    _preferences->setAVFoundationEnabled(enabled);
}

- (BOOL)_avFoundationEnabled
{
    return _preferences->isAVFoundationEnabled();
}

- (void)_setSuppressesIncrementalRendering:(BOOL)enabled
{
    _preferences->setSuppressesIncrementalRendering(enabled);
}

- (BOOL)_suppressesIncrementalRendering
{
    return _preferences->suppressesIncrementalRendering();
}

- (void)_setAsynchronousPluginInitializationEnabled:(BOOL)enabled
{
    _preferences->setAsynchronousPluginInitializationEnabled(enabled);
}

- (BOOL)_asynchronousPluginInitializationEnabled
{
    return _preferences->asynchronousPluginInitializationEnabled();
}

- (void)_setArtificialPluginInitializationDelayEnabled:(BOOL)enabled
{
    _preferences->setArtificialPluginInitializationDelayEnabled(enabled);
}

- (BOOL)_artificialPluginInitializationDelayEnabled
{
    return _preferences->artificialPluginInitializationDelayEnabled();
}

- (void)_setCookieEnabled:(BOOL)enabled
{
    _preferences->setCookieEnabled(enabled);
}

- (BOOL)_cookieEnabled
{
    return _preferences->cookieEnabled();
}

- (void)_setPlugInSnapshottingEnabled:(BOOL)enabled
{
    _preferences->setPlugInSnapshottingEnabled(enabled);
}

- (BOOL)_plugInSnapshottingEnabled
{
    return _preferences->plugInSnapshottingEnabled();
}

- (void)_setSubpixelCSSOMElementMetricsEnabled:(BOOL)enabled
{
    _preferences->setSubpixelCSSOMElementMetricsEnabled(enabled);
}

- (BOOL)_subpixelCSSOMElementMetricsEnabled
{
    return _preferences->subpixelCSSOMElementMetricsEnabled();
}

- (void)_setMediaSourceEnabled:(BOOL)enabled
{
    _preferences->setMediaSourceEnabled(enabled);
}

- (BOOL)_mediaSourceEnabled
{
    return _preferences->mediaSourceEnabled();
}

- (void)_setViewGestureDebuggingEnabled:(BOOL)enabled
{
    _preferences->setViewGestureDebuggingEnabled(enabled);
}

- (BOOL)_viewGestureDebuggingEnabled
{
    return _preferences->viewGestureDebuggingEnabled();
}

- (void)_setCSSAnimationTriggersEnabled:(BOOL)enabled
{
    _preferences->setCSSAnimationTriggersEnabled(enabled);
}

- (BOOL)_cssAnimationTriggersEnabled
{
    return _preferences->cssAnimationTriggersEnabled();
}

- (void)_setStandardFontFamily:(NSString *)family
{
    _preferences->setStandardFontFamily(family);
}

- (NSString *)_standardFontFamily
{
    return _preferences->standardFontFamily();
}

- (void)_setNotificationsEnabled:(BOOL)enabled
{
    _preferences->setNotificationsEnabled(enabled);
}

- (BOOL)_notificationsEnabled
{
    return _preferences->notificationsEnabled();
}

- (void)_setBackspaceKeyNavigationEnabled:(BOOL)enabled
{
    _preferences->setBackspaceKeyNavigationEnabled(enabled);
}

- (BOOL)_backspaceKeyNavigationEnabled
{
    return _preferences->backspaceKeyNavigationEnabled();
}

- (void)_setWebGLEnabled:(BOOL)enabled
{
    _preferences->setWebGLEnabled(enabled);
}

- (BOOL)_webGLEnabled
{
    return _preferences->webGLEnabled();
}
#endif // PLATFORM(MAC)

- (BOOL)_javaScriptCanAccessClipboard
{
    return _preferences->javaScriptCanAccessClipboard();
}

- (void)_setDOMPasteAllowed:(BOOL)domPasteAllowed
{
    _preferences->setDOMPasteAllowed(domPasteAllowed);
}

- (BOOL)_domPasteAllowed
{
    return _preferences->domPasteAllowed();
}

@end

#endif // WK_API_ENABLED
