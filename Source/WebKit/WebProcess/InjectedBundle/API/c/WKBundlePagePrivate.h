/*
 * Copyright (C) 2010, 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef WKBundlePagePrivate_h
#define WKBundlePagePrivate_h

#include <WebKit/WKBase.h>
#include <WebKit/WKDeprecated.h>
#include <WebKit/WKEvent.h>
#include <WebKit/WKGeometry.h>
#include <WebKit/WKUserContentInjectedFrames.h>
#include <WebKit/WKUserScriptInjectionTime.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT void WKBundlePageStopLoading(WKBundlePageRef page);
WK_EXPORT void WKBundlePageSetDefersLoading(WKBundlePageRef page, bool defersLoading) WK_C_API_DEPRECATED;
WK_EXPORT bool WKBundlePageIsEditingCommandEnabled(WKBundlePageRef page, WKStringRef commandName);
WK_EXPORT void WKBundlePageClearMainFrameName(WKBundlePageRef page);
WK_EXPORT void WKBundlePageClose(WKBundlePageRef page);
WK_EXPORT WKStringRef WKBundlePageCopyRenderTreeExternalRepresentation(WKBundlePageRef page);
WK_EXPORT WKStringRef WKBundlePageCopyRenderTreeExternalRepresentationForPrinting(WKBundlePageRef page);
WK_EXPORT void WKBundlePageExecuteEditingCommand(WKBundlePageRef page, WKStringRef commandName, WKStringRef argument);

WK_EXPORT double WKBundlePageGetTextZoomFactor(WKBundlePageRef page);
WK_EXPORT void WKBundlePageSetTextZoomFactor(WKBundlePageRef page, double zoomFactor);
WK_EXPORT double WKBundlePageGetPageZoomFactor(WKBundlePageRef page);
WK_EXPORT void WKBundlePageSetPageZoomFactor(WKBundlePageRef page, double zoomFactor);

WK_EXPORT void WKBundlePageSetScaleAtOrigin(WKBundlePageRef page, double scale, WKPoint origin);

WK_EXPORT void WKBundlePageForceRepaint(WKBundlePageRef page);

WK_EXPORT void WKBundlePageSimulateMouseDown(WKBundlePageRef page, int button, WKPoint position, int clickCount, WKEventModifiers modifiers, double time);
WK_EXPORT void WKBundlePageSimulateMouseUp(WKBundlePageRef page, int button, WKPoint position, int clickCount, WKEventModifiers modifiers, double time);
WK_EXPORT void WKBundlePageSimulateMouseMotion(WKBundlePageRef page, WKPoint position, double time);

WK_EXPORT uint64_t WKBundlePageGetRenderTreeSize(WKBundlePageRef page);

WK_EXPORT WKRenderObjectRef WKBundlePageCopyRenderTree(WKBundlePageRef page);
WK_EXPORT WKRenderLayerRef WKBundlePageCopyRenderLayerTree(WKBundlePageRef page);

// FIXME: This function is only still here to keep open source Mac builds building. It doesn't do anything anymore!
// We should remove it as soon as we can.
WK_EXPORT void WKBundlePageSetPaintedObjectsCounterThreshold(WKBundlePageRef page, uint64_t threshold);

WK_EXPORT void WKBundlePageSetTracksRepaints(WKBundlePageRef page, bool trackRepaints);
WK_EXPORT bool WKBundlePageIsTrackingRepaints(WKBundlePageRef page);
WK_EXPORT void WKBundlePageResetTrackedRepaints(WKBundlePageRef page);
WK_EXPORT WKArrayRef WKBundlePageCopyTrackedRepaintRects(WKBundlePageRef page);

WK_EXPORT void WKBundlePageSetComposition(WKBundlePageRef page, WKStringRef text, int from, int length, bool suppressUnderline);
WK_EXPORT bool WKBundlePageHasComposition(WKBundlePageRef page);
WK_EXPORT void WKBundlePageConfirmComposition(WKBundlePageRef page);
WK_EXPORT void WKBundlePageConfirmCompositionWithText(WKBundlePageRef page, WKStringRef text);

WK_EXPORT void WKBundlePageSetUseDarkAppearance(WKBundlePageRef page, bool useDarkAppearance);
WK_EXPORT bool WKBundlePageIsUsingDarkAppearance(WKBundlePageRef page);

WK_EXPORT bool WKBundlePageCanShowMIMEType(WKBundlePageRef, WKStringRef mimeType);

WK_EXPORT void* WKAccessibilityRootObject(WKBundlePageRef);
WK_EXPORT void* WKAccessibilityFocusedObject(WKBundlePageRef);

WK_EXPORT void WKAccessibilityEnableEnhancedAccessibility(bool);
WK_EXPORT bool WKAccessibilityEnhancedAccessibilityEnabled();

WK_EXPORT void WKBundlePageClickMenuItem(WKBundlePageRef, WKContextMenuItemRef);
WK_EXPORT WKArrayRef WKBundlePageCopyContextMenuItems(WKBundlePageRef);
WK_EXPORT WKArrayRef WKBundlePageCopyContextMenuAtPointInWindow(WKBundlePageRef, WKPoint);

WK_EXPORT void WKBundlePageInsertNewlineInQuotedContent(WKBundlePageRef page);

// This only works if the SuppressesIncrementalRendering preference is set as well.
typedef unsigned WKRenderingSuppressionToken;
WK_EXPORT WKRenderingSuppressionToken WKBundlePageExtendIncrementalRenderingSuppression(WKBundlePageRef);
WK_EXPORT void WKBundlePageStopExtendingIncrementalRenderingSuppression(WKBundlePageRef, WKRenderingSuppressionToken);

// UserContent API (compatible with Modern API, for WKTR)
WK_EXPORT void WKBundlePageAddUserScript(WKBundlePageRef page, WKStringRef source, _WKUserScriptInjectionTime injectionTime, WKUserContentInjectedFrames injectedFrames);
WK_EXPORT void WKBundlePageAddUserStyleSheet(WKBundlePageRef page, WKStringRef source, WKUserContentInjectedFrames injectedFrames);
WK_EXPORT void WKBundlePageRemoveAllUserContent(WKBundlePageRef page);

// Application Cache API, for WKTR.
WK_EXPORT void WKBundlePageClearApplicationCache(WKBundlePageRef page);
WK_EXPORT void WKBundlePageClearApplicationCacheForOrigin(WKBundlePageRef page, WKStringRef origin);
WK_EXPORT void WKBundlePageSetAppCacheMaximumSize(WKBundlePageRef page, uint64_t size);
WK_EXPORT uint64_t WKBundlePageGetAppCacheUsageForOrigin(WKBundlePageRef page, WKStringRef origin);
WK_EXPORT void WKBundlePageSetApplicationCacheOriginQuota(WKBundlePageRef page, WKStringRef origin, uint64_t bytes);
WK_EXPORT void WKBundlePageResetApplicationCacheOriginQuota(WKBundlePageRef page, WKStringRef origin);
WK_EXPORT WKArrayRef WKBundlePageCopyOriginsWithApplicationCache(WKBundlePageRef page);

enum {
    kWKEventThrottlingBehaviorResponsive = 0,
    kWKEventThrottlingBehaviorUnresponsive
};

typedef uint32_t WKEventThrottlingBehavior;

// Passing null in the second parameter clears the override.
WK_EXPORT void WKBundlePageSetEventThrottlingBehaviorOverride(WKBundlePageRef, WKEventThrottlingBehavior*);

enum {
    kWKCompositingPolicyNormal = 0,
    kWKCompositingPolicyConservative
};

typedef uint32_t WKCompositingPolicy;

// Passing null in the second parameter clears the override.
WK_EXPORT void WKBundlePageSetCompositingPolicyOverride(WKBundlePageRef, WKCompositingPolicy*);

#if TARGET_OS_IPHONE
WK_EXPORT void WKBundlePageSetUseTestingViewportConfiguration(WKBundlePageRef, bool);
#endif

#ifdef __cplusplus
}
#endif

#endif /* WKBundlePagePrivate_h */
