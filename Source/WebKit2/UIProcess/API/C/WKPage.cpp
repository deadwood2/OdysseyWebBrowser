/*
 * Copyright (C) 2010, 2015 Apple Inc. All rights reserved.
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
#include "WKPage.h"
#include "WKPagePrivate.h"

#include "APIArray.h"
#include "APIContextMenuClient.h"
#include "APIData.h"
#include "APIDictionary.h"
#include "APIFindClient.h"
#include "APIFindMatchesClient.h"
#include "APIFrameHandle.h"
#include "APIFrameInfo.h"
#include "APIGeometry.h"
#include "APIHitTestResult.h"
#include "APILoaderClient.h"
#include "APINavigationAction.h"
#include "APINavigationClient.h"
#include "APINavigationResponse.h"
#include "APIOpenPanelParameters.h"
#include "APIPageConfiguration.h"
#include "APIPolicyClient.h"
#include "APISessionState.h"
#include "APIUIClient.h"
#include "APIWindowFeatures.h"
#include "AuthenticationChallengeProxy.h"
#include "LegacySessionStateCoding.h"
#include "Logging.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebWheelEvent.h"
#include "NavigationActionData.h"
#include "PluginInformation.h"
#include "PrintInfo.h"
#include "WKAPICast.h"
#include "WKPagePolicyClientInternal.h"
#include "WKPageRenderingProgressEventsInternal.h"
#include "WKPluginInformation.h"
#include "WebBackForwardList.h"
#include "WebFormClient.h"
#include "WebImage.h"
#include "WebInspectorProxy.h"
#include "WebOpenPanelResultListenerProxy.h"
#include "WebPageGroup.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebProcessPool.h"
#include "WebProcessProxy.h"
#include "WebProtectionSpace.h"
#include <WebCore/Page.h>
#include <WebCore/SecurityOriginData.h>
#include <WebCore/WindowFeatures.h>

#ifdef __BLOCKS__
#include <Block.h>
#endif

#if ENABLE(CONTEXT_MENUS)
#include "WebContextMenuItem.h"
#endif

#if ENABLE(VIBRATION)
#include "WebVibrationProxy.h"
#endif

#if ENABLE(MEDIA_SESSION)
#include "WebMediaSessionMetadata.h"
#include <WebCore/MediaSessionEvents.h>
#endif

using namespace WebCore;
using namespace WebKit;

namespace API {
template<> struct ClientTraits<WKPageLoaderClientBase> {
    typedef std::tuple<WKPageLoaderClientV0, WKPageLoaderClientV1, WKPageLoaderClientV2, WKPageLoaderClientV3, WKPageLoaderClientV4, WKPageLoaderClientV5, WKPageLoaderClientV6> Versions;
};

template<> struct ClientTraits<WKPageNavigationClientBase> {
    typedef std::tuple<WKPageNavigationClientV0> Versions;
};

template<> struct ClientTraits<WKPagePolicyClientBase> {
    typedef std::tuple<WKPagePolicyClientV0, WKPagePolicyClientV1, WKPagePolicyClientInternal> Versions;
};

template<> struct ClientTraits<WKPageUIClientBase> {
    typedef std::tuple<WKPageUIClientV0, WKPageUIClientV1, WKPageUIClientV2, WKPageUIClientV3, WKPageUIClientV4, WKPageUIClientV5, WKPageUIClientV6, WKPageUIClientV7> Versions;
};

#if ENABLE(CONTEXT_MENUS)
template<> struct ClientTraits<WKPageContextMenuClientBase> {
    typedef std::tuple<WKPageContextMenuClientV0, WKPageContextMenuClientV1, WKPageContextMenuClientV2, WKPageContextMenuClientV3> Versions;
};
#endif

template<> struct ClientTraits<WKPageFindClientBase> {
    typedef std::tuple<WKPageFindClientV0> Versions;
};

template<> struct ClientTraits<WKPageFindMatchesClientBase> {
    typedef std::tuple<WKPageFindMatchesClientV0> Versions;
};

}

WKTypeID WKPageGetTypeID()
{
    return toAPI(WebPageProxy::APIType);
}

WKContextRef WKPageGetContext(WKPageRef pageRef)
{
    return toAPI(&toImpl(pageRef)->process().processPool());
}

WKPageGroupRef WKPageGetPageGroup(WKPageRef pageRef)
{
    return toAPI(&toImpl(pageRef)->pageGroup());
}

WKPageConfigurationRef WKPageCopyPageConfiguration(WKPageRef pageRef)
{
    return toAPI(&toImpl(pageRef)->configuration().copy().leakRef());
}

void WKPageLoadURL(WKPageRef pageRef, WKURLRef URLRef)
{
    toImpl(pageRef)->loadRequest(URL(URL(), toWTFString(URLRef)));
}

void WKPageLoadURLWithShouldOpenExternalURLsPolicy(WKPageRef pageRef, WKURLRef URLRef, bool shouldOpenExternalURLs)
{
    ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicy = shouldOpenExternalURLs ? ShouldOpenExternalURLsPolicy::ShouldAllow : ShouldOpenExternalURLsPolicy::ShouldNotAllow;
    toImpl(pageRef)->loadRequest(URL(URL(), toWTFString(URLRef)), shouldOpenExternalURLsPolicy);
}

void WKPageLoadURLWithUserData(WKPageRef pageRef, WKURLRef URLRef, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadRequest(URL(URL(), toWTFString(URLRef)), ShouldOpenExternalURLsPolicy::ShouldNotAllow, toImpl(userDataRef));
}

void WKPageLoadURLRequest(WKPageRef pageRef, WKURLRequestRef urlRequestRef)
{
    toImpl(pageRef)->loadRequest(toImpl(urlRequestRef)->resourceRequest());
}

void WKPageLoadURLRequestWithUserData(WKPageRef pageRef, WKURLRequestRef urlRequestRef, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadRequest(toImpl(urlRequestRef)->resourceRequest(), ShouldOpenExternalURLsPolicy::ShouldNotAllow, toImpl(userDataRef));
}

void WKPageLoadFile(WKPageRef pageRef, WKURLRef fileURL, WKURLRef resourceDirectoryURL)
{
    toImpl(pageRef)->loadFile(toWTFString(fileURL), toWTFString(resourceDirectoryURL));
}

void WKPageLoadFileWithUserData(WKPageRef pageRef, WKURLRef fileURL, WKURLRef resourceDirectoryURL, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadFile(toWTFString(fileURL), toWTFString(resourceDirectoryURL), toImpl(userDataRef));
}

void WKPageLoadData(WKPageRef pageRef, WKDataRef dataRef, WKStringRef MIMETypeRef, WKStringRef encodingRef, WKURLRef baseURLRef)
{
    toImpl(pageRef)->loadData(toImpl(dataRef), toWTFString(MIMETypeRef), toWTFString(encodingRef), toWTFString(baseURLRef));
}

void WKPageLoadDataWithUserData(WKPageRef pageRef, WKDataRef dataRef, WKStringRef MIMETypeRef, WKStringRef encodingRef, WKURLRef baseURLRef, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadData(toImpl(dataRef), toWTFString(MIMETypeRef), toWTFString(encodingRef), toWTFString(baseURLRef), toImpl(userDataRef));
}

void WKPageLoadHTMLString(WKPageRef pageRef, WKStringRef htmlStringRef, WKURLRef baseURLRef)
{
    toImpl(pageRef)->loadHTMLString(toWTFString(htmlStringRef), toWTFString(baseURLRef));
}

void WKPageLoadHTMLStringWithUserData(WKPageRef pageRef, WKStringRef htmlStringRef, WKURLRef baseURLRef, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadHTMLString(toWTFString(htmlStringRef), toWTFString(baseURLRef), toImpl(userDataRef));
}

void WKPageLoadAlternateHTMLString(WKPageRef pageRef, WKStringRef htmlStringRef, WKURLRef baseURLRef, WKURLRef unreachableURLRef)
{
    toImpl(pageRef)->loadAlternateHTMLString(toWTFString(htmlStringRef), toWTFString(baseURLRef), toWTFString(unreachableURLRef));
}

void WKPageLoadAlternateHTMLStringWithUserData(WKPageRef pageRef, WKStringRef htmlStringRef, WKURLRef baseURLRef, WKURLRef unreachableURLRef, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadAlternateHTMLString(toWTFString(htmlStringRef), toWTFString(baseURLRef), toWTFString(unreachableURLRef), toImpl(userDataRef));
}

void WKPageLoadPlainTextString(WKPageRef pageRef, WKStringRef plainTextStringRef)
{
    toImpl(pageRef)->loadPlainTextString(toWTFString(plainTextStringRef));    
}

void WKPageLoadPlainTextStringWithUserData(WKPageRef pageRef, WKStringRef plainTextStringRef, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadPlainTextString(toWTFString(plainTextStringRef), toImpl(userDataRef));    
}

void WKPageLoadWebArchiveData(WKPageRef pageRef, WKDataRef webArchiveDataRef)
{
    toImpl(pageRef)->loadWebArchiveData(toImpl(webArchiveDataRef));
}

void WKPageLoadWebArchiveDataWithUserData(WKPageRef pageRef, WKDataRef webArchiveDataRef, WKTypeRef userDataRef)
{
    toImpl(pageRef)->loadWebArchiveData(toImpl(webArchiveDataRef), toImpl(userDataRef));
}

void WKPageStopLoading(WKPageRef pageRef)
{
    toImpl(pageRef)->stopLoading();
}

void WKPageReload(WKPageRef pageRef)
{
    const bool reloadFromOrigin = false;
    const bool contentBlockersEnabled = true;
    toImpl(pageRef)->reload(reloadFromOrigin, contentBlockersEnabled);
}

void WKPageReloadWithoutContentBlockers(WKPageRef pageRef)
{
    const bool reloadFromOrigin = false;
    const bool contentBlockersEnabled = false;
    toImpl(pageRef)->reload(reloadFromOrigin, contentBlockersEnabled);
}

void WKPageReloadFromOrigin(WKPageRef pageRef)
{
    const bool reloadFromOrigin = true;
    const bool contentBlockersEnabled = true;
    toImpl(pageRef)->reload(reloadFromOrigin, contentBlockersEnabled);
}

bool WKPageTryClose(WKPageRef pageRef)
{
    return toImpl(pageRef)->tryClose();
}

void WKPageClose(WKPageRef pageRef)
{
    toImpl(pageRef)->close();
}

bool WKPageIsClosed(WKPageRef pageRef)
{
    return toImpl(pageRef)->isClosed();
}

void WKPageGoForward(WKPageRef pageRef)
{
    toImpl(pageRef)->goForward();
}

bool WKPageCanGoForward(WKPageRef pageRef)
{
    return toImpl(pageRef)->backForwardList().forwardItem();
}

void WKPageGoBack(WKPageRef pageRef)
{
    toImpl(pageRef)->goBack();
}

bool WKPageCanGoBack(WKPageRef pageRef)
{
    return toImpl(pageRef)->backForwardList().backItem();
}

void WKPageGoToBackForwardListItem(WKPageRef pageRef, WKBackForwardListItemRef itemRef)
{
    toImpl(pageRef)->goToBackForwardItem(toImpl(itemRef));
}

void WKPageTryRestoreScrollPosition(WKPageRef pageRef)
{
    toImpl(pageRef)->tryRestoreScrollPosition();
}

WKBackForwardListRef WKPageGetBackForwardList(WKPageRef pageRef)
{
    return toAPI(&toImpl(pageRef)->backForwardList());
}

bool WKPageWillHandleHorizontalScrollEvents(WKPageRef pageRef)
{
    return toImpl(pageRef)->willHandleHorizontalScrollEvents();
}

WKStringRef WKPageCopyTitle(WKPageRef pageRef)
{
    return toCopiedAPI(toImpl(pageRef)->pageLoadState().title());
}

WKFrameRef WKPageGetMainFrame(WKPageRef pageRef)
{
    return toAPI(toImpl(pageRef)->mainFrame());
}

WKFrameRef WKPageGetFocusedFrame(WKPageRef pageRef)
{
    return toAPI(toImpl(pageRef)->focusedFrame());
}

WKFrameRef WKPageGetFrameSetLargestFrame(WKPageRef pageRef)
{
    return toAPI(toImpl(pageRef)->frameSetLargestFrame());
}

uint64_t WKPageGetRenderTreeSize(WKPageRef page)
{
    return toImpl(page)->renderTreeSize();
}

WKInspectorRef WKPageGetInspector(WKPageRef pageRef)
{
    return toAPI(toImpl(pageRef)->inspector());
}

WKVibrationRef WKPageGetVibration(WKPageRef page)
{
#if ENABLE(VIBRATION)
    return toAPI(toImpl(page)->vibration());
#else
    UNUSED_PARAM(page);
    return 0;
#endif
}

double WKPageGetEstimatedProgress(WKPageRef pageRef)
{
    return toImpl(pageRef)->estimatedProgress();
}

WKStringRef WKPageCopyUserAgent(WKPageRef pageRef)
{
    return toCopiedAPI(toImpl(pageRef)->userAgent());
}

WKStringRef WKPageCopyApplicationNameForUserAgent(WKPageRef pageRef)
{
    return toCopiedAPI(toImpl(pageRef)->applicationNameForUserAgent());
}

void WKPageSetApplicationNameForUserAgent(WKPageRef pageRef, WKStringRef applicationNameRef)
{
    toImpl(pageRef)->setApplicationNameForUserAgent(toWTFString(applicationNameRef));
}

WKStringRef WKPageCopyCustomUserAgent(WKPageRef pageRef)
{
    return toCopiedAPI(toImpl(pageRef)->customUserAgent());
}

void WKPageSetCustomUserAgent(WKPageRef pageRef, WKStringRef userAgentRef)
{
    toImpl(pageRef)->setCustomUserAgent(toWTFString(userAgentRef));
}

void WKPageSetUserContentExtensionsEnabled(WKPageRef pageRef, bool enabled)
{
    // FIXME: Remove this function once it is no longer used.
}

bool WKPageSupportsTextEncoding(WKPageRef pageRef)
{
    return toImpl(pageRef)->supportsTextEncoding();
}

WKStringRef WKPageCopyCustomTextEncodingName(WKPageRef pageRef)
{
    return toCopiedAPI(toImpl(pageRef)->customTextEncodingName());
}

void WKPageSetCustomTextEncodingName(WKPageRef pageRef, WKStringRef encodingNameRef)
{
    toImpl(pageRef)->setCustomTextEncodingName(toWTFString(encodingNameRef));
}

void WKPageTerminate(WKPageRef pageRef)
{
    toImpl(pageRef)->terminateProcess();
}

WKStringRef WKPageGetSessionHistoryURLValueType()
{
    static API::String& sessionHistoryURLValueType = API::String::create("SessionHistoryURL").leakRef();
    return toAPI(&sessionHistoryURLValueType);
}

WKStringRef WKPageGetSessionBackForwardListItemValueType()
{
    static API::String& sessionBackForwardListValueType = API::String::create("SessionBackForwardListItem").leakRef();
    return toAPI(&sessionBackForwardListValueType);
}

WKTypeRef WKPageCopySessionState(WKPageRef pageRef, void* context, WKPageSessionStateFilterCallback filter)
{
    // FIXME: This is a hack to make sure we return a WKDataRef to maintain compatibility with older versions of Safari.
    bool shouldReturnData = !(reinterpret_cast<uintptr_t>(context) & 1);
    context = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(context) & ~1);

    auto sessionState = toImpl(pageRef)->sessionState([pageRef, context, filter](WebBackForwardListItem& item) {
        if (filter) {
            if (!filter(pageRef, WKPageGetSessionBackForwardListItemValueType(), toAPI(&item), context))
                return false;

            if (!filter(pageRef, WKPageGetSessionHistoryURLValueType(), toURLRef(item.originalURL().impl()), context))
                return false;
        }

        return true;
    });

    if (shouldReturnData)
        return toAPI(encodeLegacySessionState(sessionState).leakRef());

    return toAPI(&API::SessionState::create(WTFMove(sessionState)).leakRef());
}

static void restoreFromSessionState(WKPageRef pageRef, WKTypeRef sessionStateRef, bool navigate)
{
    SessionState sessionState;

    // FIXME: This is for backwards compatibility with Safari. Remove it once Safari no longer depends on it.
    if (toImpl(sessionStateRef)->type() == API::Object::Type::Data) {
        if (!decodeLegacySessionState(toImpl(static_cast<WKDataRef>(sessionStateRef))->bytes(), toImpl(static_cast<WKDataRef>(sessionStateRef))->size(), sessionState))
            return;
    } else {
        ASSERT(toImpl(sessionStateRef)->type() == API::Object::Type::SessionState);

        sessionState = toImpl(static_cast<WKSessionStateRef>(sessionStateRef))->sessionState();
    }

    toImpl(pageRef)->restoreFromSessionState(WTFMove(sessionState), navigate);
}

void WKPageRestoreFromSessionState(WKPageRef pageRef, WKTypeRef sessionStateRef)
{
    restoreFromSessionState(pageRef, sessionStateRef, true);
}

void WKPageRestoreFromSessionStateWithoutNavigation(WKPageRef pageRef, WKTypeRef sessionStateRef)
{
    restoreFromSessionState(pageRef, sessionStateRef, false);
}

double WKPageGetTextZoomFactor(WKPageRef pageRef)
{
    return toImpl(pageRef)->textZoomFactor();
}

double WKPageGetBackingScaleFactor(WKPageRef pageRef)
{
    return toImpl(pageRef)->deviceScaleFactor();
}

void WKPageSetCustomBackingScaleFactor(WKPageRef pageRef, double customScaleFactor)
{
    toImpl(pageRef)->setCustomDeviceScaleFactor(customScaleFactor);
}

bool WKPageSupportsTextZoom(WKPageRef pageRef)
{
    return toImpl(pageRef)->supportsTextZoom();
}

void WKPageSetTextZoomFactor(WKPageRef pageRef, double zoomFactor)
{
    toImpl(pageRef)->setTextZoomFactor(zoomFactor);
}

double WKPageGetPageZoomFactor(WKPageRef pageRef)
{
    return toImpl(pageRef)->pageZoomFactor();
}

void WKPageSetPageZoomFactor(WKPageRef pageRef, double zoomFactor)
{
    toImpl(pageRef)->setPageZoomFactor(zoomFactor);
}

void WKPageSetPageAndTextZoomFactors(WKPageRef pageRef, double pageZoomFactor, double textZoomFactor)
{
    toImpl(pageRef)->setPageAndTextZoomFactors(pageZoomFactor, textZoomFactor);
}

void WKPageSetScaleFactor(WKPageRef pageRef, double scale, WKPoint origin)
{
    toImpl(pageRef)->scalePage(scale, toIntPoint(origin));
}

double WKPageGetScaleFactor(WKPageRef pageRef)
{
    return toImpl(pageRef)->pageScaleFactor();
}

void WKPageSetUseFixedLayout(WKPageRef pageRef, bool fixed)
{
    toImpl(pageRef)->setUseFixedLayout(fixed);
}

void WKPageSetFixedLayoutSize(WKPageRef pageRef, WKSize size)
{
    toImpl(pageRef)->setFixedLayoutSize(toIntSize(size));
}

bool WKPageUseFixedLayout(WKPageRef pageRef)
{
    return toImpl(pageRef)->useFixedLayout();
}

WKSize WKPageFixedLayoutSize(WKPageRef pageRef)
{
    return toAPI(toImpl(pageRef)->fixedLayoutSize());
}

void WKPageListenForLayoutMilestones(WKPageRef pageRef, WKLayoutMilestones milestones)
{
    toImpl(pageRef)->listenForLayoutMilestones(toLayoutMilestones(milestones));
}

bool WKPageHasHorizontalScrollbar(WKPageRef pageRef)
{
    return toImpl(pageRef)->hasHorizontalScrollbar();
}

bool WKPageHasVerticalScrollbar(WKPageRef pageRef)
{
    return toImpl(pageRef)->hasVerticalScrollbar();
}

void WKPageSetSuppressScrollbarAnimations(WKPageRef pageRef, bool suppressAnimations)
{
    toImpl(pageRef)->setSuppressScrollbarAnimations(suppressAnimations);
}

bool WKPageAreScrollbarAnimationsSuppressed(WKPageRef pageRef)
{
    return toImpl(pageRef)->areScrollbarAnimationsSuppressed();
}

bool WKPageIsPinnedToLeftSide(WKPageRef pageRef)
{
    return toImpl(pageRef)->isPinnedToLeftSide();
}

bool WKPageIsPinnedToRightSide(WKPageRef pageRef)
{
    return toImpl(pageRef)->isPinnedToRightSide();
}

bool WKPageIsPinnedToTopSide(WKPageRef pageRef)
{
    return toImpl(pageRef)->isPinnedToTopSide();
}

bool WKPageIsPinnedToBottomSide(WKPageRef pageRef)
{
    return toImpl(pageRef)->isPinnedToBottomSide();
}

bool WKPageRubberBandsAtLeft(WKPageRef pageRef)
{
    return toImpl(pageRef)->rubberBandsAtLeft();
}

void WKPageSetRubberBandsAtLeft(WKPageRef pageRef, bool rubberBandsAtLeft)
{
    toImpl(pageRef)->setRubberBandsAtLeft(rubberBandsAtLeft);
}

bool WKPageRubberBandsAtRight(WKPageRef pageRef)
{
    return toImpl(pageRef)->rubberBandsAtRight();
}

void WKPageSetRubberBandsAtRight(WKPageRef pageRef, bool rubberBandsAtRight)
{
    toImpl(pageRef)->setRubberBandsAtRight(rubberBandsAtRight);
}

bool WKPageRubberBandsAtTop(WKPageRef pageRef)
{
    return toImpl(pageRef)->rubberBandsAtTop();
}

void WKPageSetRubberBandsAtTop(WKPageRef pageRef, bool rubberBandsAtTop)
{
    toImpl(pageRef)->setRubberBandsAtTop(rubberBandsAtTop);
}

bool WKPageRubberBandsAtBottom(WKPageRef pageRef)
{
    return toImpl(pageRef)->rubberBandsAtBottom();
}

void WKPageSetRubberBandsAtBottom(WKPageRef pageRef, bool rubberBandsAtBottom)
{
    toImpl(pageRef)->setRubberBandsAtBottom(rubberBandsAtBottom);
}

bool WKPageVerticalRubberBandingIsEnabled(WKPageRef pageRef)
{
    return toImpl(pageRef)->verticalRubberBandingIsEnabled();
}

void WKPageSetEnableVerticalRubberBanding(WKPageRef pageRef, bool enableVerticalRubberBanding)
{
    toImpl(pageRef)->setEnableVerticalRubberBanding(enableVerticalRubberBanding);
}

bool WKPageHorizontalRubberBandingIsEnabled(WKPageRef pageRef)
{
    return toImpl(pageRef)->horizontalRubberBandingIsEnabled();
}

void WKPageSetEnableHorizontalRubberBanding(WKPageRef pageRef, bool enableHorizontalRubberBanding)
{
    toImpl(pageRef)->setEnableHorizontalRubberBanding(enableHorizontalRubberBanding);
}

void WKPageSetBackgroundExtendsBeyondPage(WKPageRef pageRef, bool backgroundExtendsBeyondPage)
{
    toImpl(pageRef)->setBackgroundExtendsBeyondPage(backgroundExtendsBeyondPage);
}

bool WKPageBackgroundExtendsBeyondPage(WKPageRef pageRef)
{
    return toImpl(pageRef)->backgroundExtendsBeyondPage();
}

void WKPageSetPaginationMode(WKPageRef pageRef, WKPaginationMode paginationMode)
{
    Pagination::Mode mode;
    switch (paginationMode) {
    case kWKPaginationModeUnpaginated:
        mode = Pagination::Unpaginated;
        break;
    case kWKPaginationModeLeftToRight:
        mode = Pagination::LeftToRightPaginated;
        break;
    case kWKPaginationModeRightToLeft:
        mode = Pagination::RightToLeftPaginated;
        break;
    case kWKPaginationModeTopToBottom:
        mode = Pagination::TopToBottomPaginated;
        break;
    case kWKPaginationModeBottomToTop:
        mode = Pagination::BottomToTopPaginated;
        break;
    default:
        return;
    }
    toImpl(pageRef)->setPaginationMode(mode);
}

WKPaginationMode WKPageGetPaginationMode(WKPageRef pageRef)
{
    switch (toImpl(pageRef)->paginationMode()) {
    case Pagination::Unpaginated:
        return kWKPaginationModeUnpaginated;
    case Pagination::LeftToRightPaginated:
        return kWKPaginationModeLeftToRight;
    case Pagination::RightToLeftPaginated:
        return kWKPaginationModeRightToLeft;
    case Pagination::TopToBottomPaginated:
        return kWKPaginationModeTopToBottom;
    case Pagination::BottomToTopPaginated:
        return kWKPaginationModeBottomToTop;
    }

    ASSERT_NOT_REACHED();
    return kWKPaginationModeUnpaginated;
}

void WKPageSetPaginationBehavesLikeColumns(WKPageRef pageRef, bool behavesLikeColumns)
{
    toImpl(pageRef)->setPaginationBehavesLikeColumns(behavesLikeColumns);
}

bool WKPageGetPaginationBehavesLikeColumns(WKPageRef pageRef)
{
    return toImpl(pageRef)->paginationBehavesLikeColumns();
}

void WKPageSetPageLength(WKPageRef pageRef, double pageLength)
{
    toImpl(pageRef)->setPageLength(pageLength);
}

double WKPageGetPageLength(WKPageRef pageRef)
{
    return toImpl(pageRef)->pageLength();
}

void WKPageSetGapBetweenPages(WKPageRef pageRef, double gap)
{
    toImpl(pageRef)->setGapBetweenPages(gap);
}

double WKPageGetGapBetweenPages(WKPageRef pageRef)
{
    return toImpl(pageRef)->gapBetweenPages();
}

void WKPageSetPaginationLineGridEnabled(WKPageRef pageRef, bool lineGridEnabled)
{
    toImpl(pageRef)->setPaginationLineGridEnabled(lineGridEnabled);
}

bool WKPageGetPaginationLineGridEnabled(WKPageRef pageRef)
{
    return toImpl(pageRef)->paginationLineGridEnabled();
}

unsigned WKPageGetPageCount(WKPageRef pageRef)
{
    return toImpl(pageRef)->pageCount();
}

bool WKPageCanDelete(WKPageRef pageRef)
{
    return toImpl(pageRef)->canDelete();
}

bool WKPageHasSelectedRange(WKPageRef pageRef)
{
    return toImpl(pageRef)->hasSelectedRange();
}

bool WKPageIsContentEditable(WKPageRef pageRef)
{
    return toImpl(pageRef)->isContentEditable();
}

void WKPageSetMaintainsInactiveSelection(WKPageRef pageRef, bool newValue)
{
    return toImpl(pageRef)->setMaintainsInactiveSelection(newValue);
}

void WKPageCenterSelectionInVisibleArea(WKPageRef pageRef)
{
    return toImpl(pageRef)->centerSelectionInVisibleArea();
}

void WKPageFindStringMatches(WKPageRef pageRef, WKStringRef string, WKFindOptions options, unsigned maxMatchCount)
{
    toImpl(pageRef)->findStringMatches(toImpl(string)->string(), toFindOptions(options), maxMatchCount);
}

void WKPageGetImageForFindMatch(WKPageRef pageRef, int32_t matchIndex)
{
    toImpl(pageRef)->getImageForFindMatch(matchIndex);
}

void WKPageSelectFindMatch(WKPageRef pageRef, int32_t matchIndex)
{
    toImpl(pageRef)->selectFindMatch(matchIndex);
}

void WKPageFindString(WKPageRef pageRef, WKStringRef string, WKFindOptions options, unsigned maxMatchCount)
{
    toImpl(pageRef)->findString(toImpl(string)->string(), toFindOptions(options), maxMatchCount);
}

void WKPageHideFindUI(WKPageRef pageRef)
{
    toImpl(pageRef)->hideFindUI();
}

void WKPageCountStringMatches(WKPageRef pageRef, WKStringRef string, WKFindOptions options, unsigned maxMatchCount)
{
    toImpl(pageRef)->countStringMatches(toImpl(string)->string(), toFindOptions(options), maxMatchCount);
}

void WKPageSetPageContextMenuClient(WKPageRef pageRef, const WKPageContextMenuClientBase* wkClient)
{
#if ENABLE(CONTEXT_MENUS)
    class ContextMenuClient final : public API::Client<WKPageContextMenuClientBase>, public API::ContextMenuClient {
    public:
        explicit ContextMenuClient(const WKPageContextMenuClientBase* client)
        {
            initialize(client);
        }

    private:
        bool getContextMenuFromProposedMenu(WebPageProxy& page, const Vector<RefPtr<WebKit::WebContextMenuItem>>& proposedMenuVector, Vector<RefPtr<WebKit::WebContextMenuItem>>& customMenu, const WebHitTestResultData& hitTestResultData, API::Object* userData) override
        {
            if (!m_client.getContextMenuFromProposedMenu && !m_client.getContextMenuFromProposedMenu_deprecatedForUseWithV0)
                return false;

            if (m_client.base.version >= 2 && !m_client.getContextMenuFromProposedMenu)
                return false;

            Vector<RefPtr<API::Object>> proposedMenuItems;
            proposedMenuItems.reserveInitialCapacity(proposedMenuVector.size());

            for (const auto& menuItem : proposedMenuVector)
                proposedMenuItems.uncheckedAppend(menuItem);

            WKArrayRef newMenu = nullptr;
            if (m_client.base.version >= 2) {
                RefPtr<API::HitTestResult> webHitTestResult = API::HitTestResult::create(hitTestResultData);
                m_client.getContextMenuFromProposedMenu(toAPI(&page), toAPI(API::Array::create(WTFMove(proposedMenuItems)).ptr()), &newMenu, toAPI(webHitTestResult.get()), toAPI(userData), m_client.base.clientInfo);
            } else
                m_client.getContextMenuFromProposedMenu_deprecatedForUseWithV0(toAPI(&page), toAPI(API::Array::create(WTFMove(proposedMenuItems)).ptr()), &newMenu, toAPI(userData), m_client.base.clientInfo);

            RefPtr<API::Array> array = adoptRef(toImpl(newMenu));

            customMenu.clear();

            size_t newSize = array ? array->size() : 0;
            for (size_t i = 0; i < newSize; ++i) {
                WebContextMenuItem* item = array->at<WebContextMenuItem>(i);
                if (!item) {
                    LOG(ContextMenu, "New menu entry at index %i is not a WebContextMenuItem", (int)i);
                    continue;
                }

                customMenu.append(item);
            }

            return true;
        }

        void customContextMenuItemSelected(WebPageProxy& page, const WebContextMenuItemData& itemData) override
        {
            if (!m_client.customContextMenuItemSelected)
                return;

            m_client.customContextMenuItemSelected(toAPI(&page), toAPI(WebContextMenuItem::create(itemData).ptr()), m_client.base.clientInfo);
        }

        bool showContextMenu(WebPageProxy& page, const WebCore::IntPoint& menuLocation, const Vector<RefPtr<WebContextMenuItem>>& menuItemsVector) override
        {
            if (!m_client.showContextMenu)
                return false;

            Vector<RefPtr<API::Object>> menuItems;
            menuItems.reserveInitialCapacity(menuItemsVector.size());

            for (const auto& menuItem : menuItemsVector)
                menuItems.uncheckedAppend(menuItem);

            m_client.showContextMenu(toAPI(&page), toAPI(menuLocation), toAPI(API::Array::create(WTFMove(menuItems)).ptr()), m_client.base.clientInfo);

            return true;
        }

        bool hideContextMenu(WebPageProxy& page) override
        {
            if (!m_client.hideContextMenu)
                return false;

            m_client.hideContextMenu(toAPI(&page), m_client.base.clientInfo);

            return true;
        }
    };

    toImpl(pageRef)->setContextMenuClient(std::make_unique<ContextMenuClient>(wkClient));
#else
    UNUSED_PARAM(pageRef);
    UNUSED_PARAM(wkClient);
#endif
}

void WKPageSetPageDiagnosticLoggingClient(WKPageRef pageRef, const WKPageDiagnosticLoggingClientBase* wkClient)
{
    toImpl(pageRef)->setDiagnosticLoggingClient(std::make_unique<WebPageDiagnosticLoggingClient>(wkClient));
}

void WKPageSetPageFindClient(WKPageRef pageRef, const WKPageFindClientBase* wkClient)
{
    class FindClient : public API::Client<WKPageFindClientBase>, public API::FindClient {
    public:
        explicit FindClient(const WKPageFindClientBase* client)
        {
            initialize(client);
        }

    private:
        void didFindString(WebPageProxy* page, const String& string, const Vector<WebCore::IntRect>&, uint32_t matchCount, int32_t) override
        {
            if (!m_client.didFindString)
                return;
            
            m_client.didFindString(toAPI(page), toAPI(string.impl()), matchCount, m_client.base.clientInfo);
        }

        void didFailToFindString(WebPageProxy* page, const String& string) override
        {
            if (!m_client.didFailToFindString)
                return;
            
            m_client.didFailToFindString(toAPI(page), toAPI(string.impl()), m_client.base.clientInfo);
        }

        void didCountStringMatches(WebPageProxy* page, const String& string, uint32_t matchCount) override
        {
            if (!m_client.didCountStringMatches)
                return;

            m_client.didCountStringMatches(toAPI(page), toAPI(string.impl()), matchCount, m_client.base.clientInfo);
        }
    };

    toImpl(pageRef)->setFindClient(std::make_unique<FindClient>(wkClient));
}

void WKPageSetPageFindMatchesClient(WKPageRef pageRef, const WKPageFindMatchesClientBase* wkClient)
{
    class FindMatchesClient : public API::Client<WKPageFindMatchesClientBase>, public API::FindMatchesClient {
    public:
        explicit FindMatchesClient(const WKPageFindMatchesClientBase* client)
        {
            initialize(client);
        }

    private:
        void didFindStringMatches(WebPageProxy* page, const String& string, const Vector<Vector<WebCore::IntRect>>& matchRects, int32_t index) override
        {
            if (!m_client.didFindStringMatches)
                return;

            Vector<RefPtr<API::Object>> matches;
            matches.reserveInitialCapacity(matchRects.size());

            for (const auto& rects : matchRects) {
                Vector<RefPtr<API::Object>> apiRects;
                apiRects.reserveInitialCapacity(rects.size());

                for (const auto& rect : rects)
                    apiRects.uncheckedAppend(API::Rect::create(toAPI(rect)));

                matches.uncheckedAppend(API::Array::create(WTFMove(apiRects)));
            }

            m_client.didFindStringMatches(toAPI(page), toAPI(string.impl()), toAPI(API::Array::create(WTFMove(matches)).ptr()), index, m_client.base.clientInfo);
        }

        void didGetImageForMatchResult(WebPageProxy* page, WebImage* image, int32_t index) override
        {
            if (!m_client.didGetImageForMatchResult)
                return;

            m_client.didGetImageForMatchResult(toAPI(page), toAPI(image), index, m_client.base.clientInfo);
        }
    };

    toImpl(pageRef)->setFindMatchesClient(std::make_unique<FindMatchesClient>(wkClient));
}

void WKPageSetPageInjectedBundleClient(WKPageRef pageRef, const WKPageInjectedBundleClientBase* wkClient)
{
    toImpl(pageRef)->setInjectedBundleClient(wkClient);
}

void WKPageSetPageFormClient(WKPageRef pageRef, const WKPageFormClientBase* wkClient)
{
    toImpl(pageRef)->setFormClient(std::make_unique<WebFormClient>(wkClient));
}

void WKPageSetPageLoaderClient(WKPageRef pageRef, const WKPageLoaderClientBase* wkClient)
{
    class LoaderClient : public API::Client<WKPageLoaderClientBase>, public API::LoaderClient {
    public:
        explicit LoaderClient(const WKPageLoaderClientBase* client)
        {
            initialize(client);
        }

    private:
        void didStartProvisionalLoadForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, API::Object* userData) override
        {
            if (!m_client.didStartProvisionalLoadForFrame)
                return;

            m_client.didStartProvisionalLoadForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didReceiveServerRedirectForProvisionalLoadForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, API::Object* userData) override
        {
            if (!m_client.didReceiveServerRedirectForProvisionalLoadForFrame)
                return;

            m_client.didReceiveServerRedirectForProvisionalLoadForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didFailProvisionalLoadWithErrorForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, const ResourceError& error, API::Object* userData) override
        {
            if (!m_client.didFailProvisionalLoadWithErrorForFrame)
                return;

            m_client.didFailProvisionalLoadWithErrorForFrame(toAPI(&page), toAPI(&frame), toAPI(error), toAPI(userData), m_client.base.clientInfo);
        }

        void didCommitLoadForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, API::Object* userData) override
        {
            if (!m_client.didCommitLoadForFrame)
                return;

            m_client.didCommitLoadForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didFinishDocumentLoadForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, API::Object* userData) override
        {
            if (!m_client.didFinishDocumentLoadForFrame)
                return;

            m_client.didFinishDocumentLoadForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didFinishLoadForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, API::Object* userData) override
        {
            if (!m_client.didFinishLoadForFrame)
                return;

            m_client.didFinishLoadForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didFailLoadWithErrorForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, const ResourceError& error, API::Object* userData) override
        {
            if (!m_client.didFailLoadWithErrorForFrame)
                return;

            m_client.didFailLoadWithErrorForFrame(toAPI(&page), toAPI(&frame), toAPI(error), toAPI(userData), m_client.base.clientInfo);
        }

        void didSameDocumentNavigationForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Navigation*, SameDocumentNavigationType type, API::Object* userData) override
        {
            if (!m_client.didSameDocumentNavigationForFrame)
                return;

            m_client.didSameDocumentNavigationForFrame(toAPI(&page), toAPI(&frame), toAPI(type), toAPI(userData), m_client.base.clientInfo);
        }

        void didReceiveTitleForFrame(WebPageProxy& page, const String& title, WebFrameProxy& frame, API::Object* userData) override
        {
            if (!m_client.didReceiveTitleForFrame)
                return;

            m_client.didReceiveTitleForFrame(toAPI(&page), toAPI(title.impl()), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didFirstLayoutForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Object* userData) override
        {
            if (!m_client.didFirstLayoutForFrame)
                return;

            m_client.didFirstLayoutForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didFirstVisuallyNonEmptyLayoutForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Object* userData) override
        {
            if (!m_client.didFirstVisuallyNonEmptyLayoutForFrame)
                return;

            m_client.didFirstVisuallyNonEmptyLayoutForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didReachLayoutMilestone(WebPageProxy& page, LayoutMilestones milestones) override
        {
            if (!m_client.didLayout)
                return;

            m_client.didLayout(toAPI(&page), toWKLayoutMilestones(milestones), nullptr, m_client.base.clientInfo);
        }

        void didDisplayInsecureContentForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Object* userData) override
        {
            if (!m_client.didDisplayInsecureContentForFrame)
                return;

            m_client.didDisplayInsecureContentForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didRunInsecureContentForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Object* userData) override
        {
            if (!m_client.didRunInsecureContentForFrame)
                return;

            m_client.didRunInsecureContentForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        void didDetectXSSForFrame(WebPageProxy& page, WebFrameProxy& frame, API::Object* userData) override
        {
            if (!m_client.didDetectXSSForFrame)
                return;

            m_client.didDetectXSSForFrame(toAPI(&page), toAPI(&frame), toAPI(userData), m_client.base.clientInfo);
        }

        bool canAuthenticateAgainstProtectionSpaceInFrame(WebPageProxy& page, WebFrameProxy& frame, WebProtectionSpace* protectionSpace) override
        {
            if (!m_client.canAuthenticateAgainstProtectionSpaceInFrame)
                return false;

            return m_client.canAuthenticateAgainstProtectionSpaceInFrame(toAPI(&page), toAPI(&frame), toAPI(protectionSpace), m_client.base.clientInfo);
        }

        void didReceiveAuthenticationChallengeInFrame(WebPageProxy& page, WebFrameProxy& frame, AuthenticationChallengeProxy* authenticationChallenge) override
        {
            if (!m_client.didReceiveAuthenticationChallengeInFrame)
                return;

            m_client.didReceiveAuthenticationChallengeInFrame(toAPI(&page), toAPI(&frame), toAPI(authenticationChallenge), m_client.base.clientInfo);
        }

        void didStartProgress(WebPageProxy& page) override
        {
            if (!m_client.didStartProgress)
                return;

            m_client.didStartProgress(toAPI(&page), m_client.base.clientInfo);
        }

        void didChangeProgress(WebPageProxy& page) override
        {
            if (!m_client.didChangeProgress)
                return;

            m_client.didChangeProgress(toAPI(&page), m_client.base.clientInfo);
        }

        void didFinishProgress(WebPageProxy& page) override
        {
            if (!m_client.didFinishProgress)
                return;

            m_client.didFinishProgress(toAPI(&page), m_client.base.clientInfo);
        }

        void processDidBecomeUnresponsive(WebPageProxy& page) override
        {
            if (!m_client.processDidBecomeUnresponsive)
                return;

            m_client.processDidBecomeUnresponsive(toAPI(&page), m_client.base.clientInfo);
        }

        void processDidBecomeResponsive(WebPageProxy& page) override
        {
            if (!m_client.processDidBecomeResponsive)
                return;

            m_client.processDidBecomeResponsive(toAPI(&page), m_client.base.clientInfo);
        }

        void processDidCrash(WebPageProxy& page) override
        {
            if (!m_client.processDidCrash)
                return;

            m_client.processDidCrash(toAPI(&page), m_client.base.clientInfo);
        }

        void didChangeBackForwardList(WebPageProxy& page, WebBackForwardListItem* addedItem, Vector<RefPtr<WebBackForwardListItem>> removedItems) override
        {
            if (!m_client.didChangeBackForwardList)
                return;

            RefPtr<API::Array> removedItemsArray;
            if (!removedItems.isEmpty()) {
                Vector<RefPtr<API::Object>> removedItemsVector;
                removedItemsVector.reserveInitialCapacity(removedItems.size());
                for (auto& removedItem : removedItems)
                    removedItemsVector.append(WTFMove(removedItem));

                removedItemsArray = API::Array::create(WTFMove(removedItemsVector));
            }

            m_client.didChangeBackForwardList(toAPI(&page), toAPI(addedItem), toAPI(removedItemsArray.get()), m_client.base.clientInfo);
        }

        bool shouldKeepCurrentBackForwardListItemInList(WebKit::WebPageProxy& page, WebKit::WebBackForwardListItem* item) override
        {
            if (!m_client.shouldKeepCurrentBackForwardListItemInList)
                return true;

            return m_client.shouldKeepCurrentBackForwardListItemInList(toAPI(&page), toAPI(item), m_client.base.clientInfo);
        }

        void willGoToBackForwardListItem(WebPageProxy& page, WebBackForwardListItem* item, API::Object* userData) override
        {
            if (m_client.willGoToBackForwardListItem)
                m_client.willGoToBackForwardListItem(toAPI(&page), toAPI(item), toAPI(userData), m_client.base.clientInfo);
        }

        RefPtr<API::Data> webCryptoMasterKey(WebPageProxy& page) override
        {
            return page.process().processPool().client().copyWebCryptoMasterKey(&page.process().processPool());
        }

        void navigationGestureDidBegin(WebPageProxy& page) override
        {
            if (m_client.navigationGestureDidBegin)
                m_client.navigationGestureDidBegin(toAPI(&page), m_client.base.clientInfo);
        }

        void navigationGestureWillEnd(WebPageProxy& page, bool willNavigate, WebBackForwardListItem& item) override
        {
            if (m_client.navigationGestureWillEnd)
                m_client.navigationGestureWillEnd(toAPI(&page), willNavigate, toAPI(&item), m_client.base.clientInfo);
        }

        void navigationGestureDidEnd(WebPageProxy& page, bool willNavigate, WebBackForwardListItem& item) override
        {
            if (m_client.navigationGestureDidEnd)
                m_client.navigationGestureDidEnd(toAPI(&page), willNavigate, toAPI(&item), m_client.base.clientInfo);
        }

#if ENABLE(NETSCAPE_PLUGIN_API)
        void didFailToInitializePlugin(WebPageProxy& page, API::Dictionary* pluginInformation) override
        {
            if (m_client.didFailToInitializePlugin_deprecatedForUseWithV0)
                m_client.didFailToInitializePlugin_deprecatedForUseWithV0(toAPI(&page), toAPI(pluginInformation->get<API::String>(pluginInformationMIMETypeKey())), m_client.base.clientInfo);

            if (m_client.pluginDidFail_deprecatedForUseWithV1)
                m_client.pluginDidFail_deprecatedForUseWithV1(toAPI(&page), kWKErrorCodeCannotLoadPlugIn, toAPI(pluginInformation->get<API::String>(pluginInformationMIMETypeKey())), 0, 0, m_client.base.clientInfo);

            if (m_client.pluginDidFail)
                m_client.pluginDidFail(toAPI(&page), kWKErrorCodeCannotLoadPlugIn, toAPI(pluginInformation), m_client.base.clientInfo);
        }

        void didBlockInsecurePluginVersion(WebPageProxy& page, API::Dictionary* pluginInformation) override
        {
            if (m_client.pluginDidFail_deprecatedForUseWithV1)
                m_client.pluginDidFail_deprecatedForUseWithV1(toAPI(&page), kWKErrorCodeInsecurePlugInVersion, toAPI(pluginInformation->get<API::String>(pluginInformationMIMETypeKey())), toAPI(pluginInformation->get<API::String>(pluginInformationBundleIdentifierKey())), toAPI(pluginInformation->get<API::String>(pluginInformationBundleVersionKey())), m_client.base.clientInfo);

            if (m_client.pluginDidFail)
                m_client.pluginDidFail(toAPI(&page), kWKErrorCodeInsecurePlugInVersion, toAPI(pluginInformation), m_client.base.clientInfo);
        }

        PluginModuleLoadPolicy pluginLoadPolicy(WebPageProxy& page, PluginModuleLoadPolicy currentPluginLoadPolicy, API::Dictionary* pluginInformation, String& unavailabilityDescription) override
        {
            WKStringRef unavailabilityDescriptionOut = 0;
            PluginModuleLoadPolicy loadPolicy = currentPluginLoadPolicy;

            if (m_client.pluginLoadPolicy_deprecatedForUseWithV2)
                loadPolicy = toPluginModuleLoadPolicy(m_client.pluginLoadPolicy_deprecatedForUseWithV2(toAPI(&page), toWKPluginLoadPolicy(currentPluginLoadPolicy), toAPI(pluginInformation), m_client.base.clientInfo));
            else if (m_client.pluginLoadPolicy)
                loadPolicy = toPluginModuleLoadPolicy(m_client.pluginLoadPolicy(toAPI(&page), toWKPluginLoadPolicy(currentPluginLoadPolicy), toAPI(pluginInformation), &unavailabilityDescriptionOut, m_client.base.clientInfo));

            if (unavailabilityDescriptionOut) {
                RefPtr<API::String> webUnavailabilityDescription = adoptRef(toImpl(unavailabilityDescriptionOut));
                unavailabilityDescription = webUnavailabilityDescription->string();
            }
            
            return loadPolicy;
        }
#endif // ENABLE(NETSCAPE_PLUGIN_API)

#if ENABLE(WEBGL)
        WebCore::WebGLLoadPolicy webGLLoadPolicy(WebPageProxy& page, const String& url) const override
        {
            WebCore::WebGLLoadPolicy loadPolicy = WebGLAllowCreation;

            if (m_client.webGLLoadPolicy)
                loadPolicy = toWebGLLoadPolicy(m_client.webGLLoadPolicy(toAPI(&page), toAPI(url.impl()), m_client.base.clientInfo));

            return loadPolicy;
        }

        WebCore::WebGLLoadPolicy resolveWebGLLoadPolicy(WebPageProxy& page, const String& url) const override
        {
            WebCore::WebGLLoadPolicy loadPolicy = WebGLAllowCreation;

            if (m_client.resolveWebGLLoadPolicy)
                loadPolicy = toWebGLLoadPolicy(m_client.resolveWebGLLoadPolicy(toAPI(&page), toAPI(url.impl()), m_client.base.clientInfo));

            return loadPolicy;
        }

#endif // ENABLE(WEBGL)
    };

    WebPageProxy* webPageProxy = toImpl(pageRef);

    auto loaderClient = std::make_unique<LoaderClient>(wkClient);

    // It would be nice to get rid of this code and transition all clients to using didLayout instead of
    // didFirstLayoutInFrame and didFirstVisuallyNonEmptyLayoutInFrame. In the meantime, this is required
    // for backwards compatibility.
    WebCore::LayoutMilestones milestones = 0;
    if (loaderClient->client().didFirstLayoutForFrame)
        milestones |= WebCore::DidFirstLayout;
    if (loaderClient->client().didFirstVisuallyNonEmptyLayoutForFrame)
        milestones |= WebCore::DidFirstVisuallyNonEmptyLayout;

    if (milestones)
        webPageProxy->process().send(Messages::WebPage::ListenForLayoutMilestones(milestones), webPageProxy->pageID());

    webPageProxy->setLoaderClient(WTFMove(loaderClient));
}

void WKPageSetPagePolicyClient(WKPageRef pageRef, const WKPagePolicyClientBase* wkClient)
{
    class PolicyClient : public API::Client<WKPagePolicyClientBase>, public API::PolicyClient {
    public:
        explicit PolicyClient(const WKPagePolicyClientBase* client)
        {
            initialize(client);
        }

    private:
        void decidePolicyForNavigationAction(WebPageProxy& page, WebFrameProxy* frame, const NavigationActionData& navigationActionData, WebFrameProxy* originatingFrame, const WebCore::ResourceRequest& originalResourceRequest, const WebCore::ResourceRequest& resourceRequest, Ref<WebFramePolicyListenerProxy>&& listener, API::Object* userData) override
        {
            if (!m_client.decidePolicyForNavigationAction_deprecatedForUseWithV0 && !m_client.decidePolicyForNavigationAction_deprecatedForUseWithV1 && !m_client.decidePolicyForNavigationAction) {
                listener->use();
                return;
            }

            Ref<API::URLRequest> originalRequest = API::URLRequest::create(originalResourceRequest);
            Ref<API::URLRequest> request = API::URLRequest::create(resourceRequest);

            if (m_client.decidePolicyForNavigationAction_deprecatedForUseWithV0)
                m_client.decidePolicyForNavigationAction_deprecatedForUseWithV0(toAPI(&page), toAPI(frame), toAPI(navigationActionData.navigationType), toAPI(navigationActionData.modifiers), toAPI(navigationActionData.mouseButton), toAPI(request.ptr()), toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
            else if (m_client.decidePolicyForNavigationAction_deprecatedForUseWithV1)
                m_client.decidePolicyForNavigationAction_deprecatedForUseWithV1(toAPI(&page), toAPI(frame), toAPI(navigationActionData.navigationType), toAPI(navigationActionData.modifiers), toAPI(navigationActionData.mouseButton), toAPI(originatingFrame), toAPI(request.ptr()), toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
            else
                m_client.decidePolicyForNavigationAction(toAPI(&page), toAPI(frame), toAPI(navigationActionData.navigationType), toAPI(navigationActionData.modifiers), toAPI(navigationActionData.mouseButton), toAPI(originatingFrame), toAPI(originalRequest.ptr()), toAPI(request.ptr()), toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
        }

        void decidePolicyForNewWindowAction(WebPageProxy& page, WebFrameProxy& frame, const NavigationActionData& navigationActionData, const ResourceRequest& resourceRequest, const String& frameName, Ref<WebFramePolicyListenerProxy>&& listener, API::Object* userData) override
        {
            if (!m_client.decidePolicyForNewWindowAction) {
                listener->use();
                return;
            }

            Ref<API::URLRequest> request = API::URLRequest::create(resourceRequest);

            m_client.decidePolicyForNewWindowAction(toAPI(&page), toAPI(&frame), toAPI(navigationActionData.navigationType), toAPI(navigationActionData.modifiers), toAPI(navigationActionData.mouseButton), toAPI(request.ptr()), toAPI(frameName.impl()), toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
        }

        void decidePolicyForResponse(WebPageProxy& page, WebFrameProxy& frame, const ResourceResponse& resourceResponse, const ResourceRequest& resourceRequest, bool canShowMIMEType, Ref<WebFramePolicyListenerProxy>&& listener, API::Object* userData) override
        {
            if (!m_client.decidePolicyForResponse_deprecatedForUseWithV0 && !m_client.decidePolicyForResponse) {
                listener->use();
                return;
            }

            Ref<API::URLResponse> response = API::URLResponse::create(resourceResponse);
            Ref<API::URLRequest> request = API::URLRequest::create(resourceRequest);

            if (m_client.decidePolicyForResponse_deprecatedForUseWithV0)
                m_client.decidePolicyForResponse_deprecatedForUseWithV0(toAPI(&page), toAPI(&frame), toAPI(response.ptr()), toAPI(request.ptr()), toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
            else
                m_client.decidePolicyForResponse(toAPI(&page), toAPI(&frame), toAPI(response.ptr()), toAPI(request.ptr()), canShowMIMEType, toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
        }

        void unableToImplementPolicy(WebPageProxy& page, WebFrameProxy& frame, const ResourceError& error, API::Object* userData) override
        {
            if (!m_client.unableToImplementPolicy)
                return;
            
            m_client.unableToImplementPolicy(toAPI(&page), toAPI(&frame), toAPI(error), toAPI(userData), m_client.base.clientInfo);
        }
    };

    toImpl(pageRef)->setPolicyClient(std::make_unique<PolicyClient>(wkClient));
}

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED <= 101000
static void fixUpBotchedPageUIClient(WKPageRef pageRef, const WKPageUIClientBase& wkClient)
{
    struct BotchedWKPageUIClientV4 {
        WKPageUIClientBase                                                  base;

        // Version 0.
        WKPageCreateNewPageCallback_deprecatedForUseWithV0                  createNewPage_deprecatedForUseWithV0;
        WKPageUIClientCallback                                              showPage;
        WKPageUIClientCallback                                              close;
        WKPageTakeFocusCallback                                             takeFocus;
        WKPageFocusCallback                                                 focus;
        WKPageUnfocusCallback                                               unfocus;
        WKPageRunJavaScriptAlertCallback_deprecatedForUseWithV0             runJavaScriptAlert_deprecatedForUseWithV0;
        WKPageRunJavaScriptConfirmCallback_deprecatedForUseWithV0           runJavaScriptConfirm_deprecatedForUseWithV0;
        WKPageRunJavaScriptPromptCallback_deprecatedForUseWithV0            runJavaScriptPrompt_deprecatedForUseWithV0;
        WKPageSetStatusTextCallback                                         setStatusText;
        WKPageMouseDidMoveOverElementCallback_deprecatedForUseWithV0        mouseDidMoveOverElement_deprecatedForUseWithV0;
        WKPageMissingPluginButtonClickedCallback_deprecatedForUseWithV0     missingPluginButtonClicked_deprecatedForUseWithV0;
        WKPageDidNotHandleKeyEventCallback                                  didNotHandleKeyEvent;
        WKPageDidNotHandleWheelEventCallback                                didNotHandleWheelEvent;
        WKPageGetToolbarsAreVisibleCallback                                 toolbarsAreVisible;
        WKPageSetToolbarsAreVisibleCallback                                 setToolbarsAreVisible;
        WKPageGetMenuBarIsVisibleCallback                                   menuBarIsVisible;
        WKPageSetMenuBarIsVisibleCallback                                   setMenuBarIsVisible;
        WKPageGetStatusBarIsVisibleCallback                                 statusBarIsVisible;
        WKPageSetStatusBarIsVisibleCallback                                 setStatusBarIsVisible;
        WKPageGetIsResizableCallback                                        isResizable;
        WKPageSetIsResizableCallback                                        setIsResizable;
        WKPageGetWindowFrameCallback                                        getWindowFrame;
        WKPageSetWindowFrameCallback                                        setWindowFrame;
        WKPageRunBeforeUnloadConfirmPanelCallback_deprecatedForUseWithV6    runBeforeUnloadConfirmPanel;
        WKPageUIClientCallback                                              didDraw;
        WKPageUIClientCallback                                              pageDidScroll;
        WKPageExceededDatabaseQuotaCallback                                 exceededDatabaseQuota;
        WKPageRunOpenPanelCallback                                          runOpenPanel;
        WKPageDecidePolicyForGeolocationPermissionRequestCallback           decidePolicyForGeolocationPermissionRequest;
        WKPageHeaderHeightCallback                                          headerHeight;
        WKPageFooterHeightCallback                                          footerHeight;
        WKPageDrawHeaderCallback                                            drawHeader;
        WKPageDrawFooterCallback                                            drawFooter;
        WKPagePrintFrameCallback                                            printFrame;
        WKPageUIClientCallback                                              runModal;
        void*                                                               unused1; // Used to be didCompleteRubberBandForMainFrame
        WKPageSaveDataToFileInDownloadsFolderCallback                       saveDataToFileInDownloadsFolder;
        void*                                                               shouldInterruptJavaScript_unavailable;

        // Version 1.
        WKPageCreateNewPageCallback_deprecatedForUseWithV1                  createNewPage;
        WKPageMouseDidMoveOverElementCallback                               mouseDidMoveOverElement;
        WKPageDecidePolicyForNotificationPermissionRequestCallback          decidePolicyForNotificationPermissionRequest;
        WKPageUnavailablePluginButtonClickedCallback_deprecatedForUseWithV1 unavailablePluginButtonClicked_deprecatedForUseWithV1;

        // Version 2.
        WKPageShowColorPickerCallback                                       showColorPicker;
        WKPageHideColorPickerCallback                                       hideColorPicker;
        WKPageUnavailablePluginButtonClickedCallback                        unavailablePluginButtonClicked;

        // Version 3.
        WKPagePinnedStateDidChangeCallback                                  pinnedStateDidChange;

        // Version 4.
        WKPageRunJavaScriptAlertCallback_deprecatedForUseWithV5             runJavaScriptAlert;
        WKPageRunJavaScriptConfirmCallback_deprecatedForUseWithV5           runJavaScriptConfirm;
        WKPageRunJavaScriptPromptCallback_deprecatedForUseWithV5            runJavaScriptPrompt;
    };

    const auto& botchedPageUIClient = reinterpret_cast<const BotchedWKPageUIClientV4&>(wkClient);

    WKPageUIClientV5 fixedPageUIClient = {
        { 5, botchedPageUIClient.base.clientInfo },
        botchedPageUIClient.createNewPage_deprecatedForUseWithV0,
        botchedPageUIClient.showPage,
        botchedPageUIClient.close,
        botchedPageUIClient.takeFocus,
        botchedPageUIClient.focus,
        botchedPageUIClient.unfocus,
        botchedPageUIClient.runJavaScriptAlert_deprecatedForUseWithV0,
        botchedPageUIClient.runJavaScriptConfirm_deprecatedForUseWithV0,
        botchedPageUIClient.runJavaScriptPrompt_deprecatedForUseWithV0,
        botchedPageUIClient.setStatusText,
        botchedPageUIClient.mouseDidMoveOverElement_deprecatedForUseWithV0,
        botchedPageUIClient.missingPluginButtonClicked_deprecatedForUseWithV0,
        botchedPageUIClient.didNotHandleKeyEvent,
        botchedPageUIClient.didNotHandleWheelEvent,
        botchedPageUIClient.toolbarsAreVisible,
        botchedPageUIClient.setToolbarsAreVisible,
        botchedPageUIClient.menuBarIsVisible,
        botchedPageUIClient.setMenuBarIsVisible,
        botchedPageUIClient.statusBarIsVisible,
        botchedPageUIClient.setStatusBarIsVisible,
        botchedPageUIClient.isResizable,
        botchedPageUIClient.setIsResizable,
        botchedPageUIClient.getWindowFrame,
        botchedPageUIClient.setWindowFrame,
        botchedPageUIClient.runBeforeUnloadConfirmPanel,
        botchedPageUIClient.didDraw,
        botchedPageUIClient.pageDidScroll,
        botchedPageUIClient.exceededDatabaseQuota,
        botchedPageUIClient.runOpenPanel,
        botchedPageUIClient.decidePolicyForGeolocationPermissionRequest,
        botchedPageUIClient.headerHeight,
        botchedPageUIClient.footerHeight,
        botchedPageUIClient.drawHeader,
        botchedPageUIClient.drawFooter,
        botchedPageUIClient.printFrame,
        botchedPageUIClient.runModal,
        botchedPageUIClient.unused1,
        botchedPageUIClient.saveDataToFileInDownloadsFolder,
        botchedPageUIClient.shouldInterruptJavaScript_unavailable,
        botchedPageUIClient.createNewPage,
        botchedPageUIClient.mouseDidMoveOverElement,
        botchedPageUIClient.decidePolicyForNotificationPermissionRequest,
        botchedPageUIClient.unavailablePluginButtonClicked_deprecatedForUseWithV1,
        botchedPageUIClient.showColorPicker,
        botchedPageUIClient.hideColorPicker,
        botchedPageUIClient.unavailablePluginButtonClicked,
        botchedPageUIClient.pinnedStateDidChange,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        botchedPageUIClient.runJavaScriptAlert,
        botchedPageUIClient.runJavaScriptConfirm,
        botchedPageUIClient.runJavaScriptPrompt,
        nullptr,
    };

    WKPageSetPageUIClient(pageRef, &fixedPageUIClient.base);
}
#endif

namespace WebKit {

class RunBeforeUnloadConfirmPanelResultListener : public API::ObjectImpl<API::Object::Type::RunBeforeUnloadConfirmPanelResultListener> {
public:
    static PassRefPtr<RunBeforeUnloadConfirmPanelResultListener> create(std::function<void (bool)>&& completionHandler)
    {
        return adoptRef(new RunBeforeUnloadConfirmPanelResultListener(WTFMove(completionHandler)));
    }

    virtual ~RunBeforeUnloadConfirmPanelResultListener()
    {
    }

    void call(bool result)
    {
        m_completionHandler(result);
    }

private:
    explicit RunBeforeUnloadConfirmPanelResultListener(std::function<void (bool)>&& completionHandler)
        : m_completionHandler(WTFMove(completionHandler))
    {
    }

    std::function<void (bool)> m_completionHandler;
};

class RunJavaScriptAlertResultListener : public API::ObjectImpl<API::Object::Type::RunJavaScriptAlertResultListener> {
public:
    static PassRefPtr<RunJavaScriptAlertResultListener> create(std::function<void ()>&& completionHandler)
    {
        return adoptRef(new RunJavaScriptAlertResultListener(WTFMove(completionHandler)));
    }

    virtual ~RunJavaScriptAlertResultListener()
    {
    }

    void call()
    {
        m_completionHandler();
    }

private:
    explicit RunJavaScriptAlertResultListener(std::function<void ()>&& completionHandler)
        : m_completionHandler(WTFMove(completionHandler))
    {
    }
    
    std::function<void ()> m_completionHandler;
};

class RunJavaScriptConfirmResultListener : public API::ObjectImpl<API::Object::Type::RunJavaScriptConfirmResultListener> {
public:
    static PassRefPtr<RunJavaScriptConfirmResultListener> create(std::function<void (bool)>&& completionHandler)
    {
        return adoptRef(new RunJavaScriptConfirmResultListener(WTFMove(completionHandler)));
    }

    virtual ~RunJavaScriptConfirmResultListener()
    {
    }

    void call(bool result)
    {
        m_completionHandler(result);
    }

private:
    explicit RunJavaScriptConfirmResultListener(std::function<void (bool)>&& completionHandler)
        : m_completionHandler(WTFMove(completionHandler))
    {
    }

    std::function<void (bool)> m_completionHandler;
};

class RunJavaScriptPromptResultListener : public API::ObjectImpl<API::Object::Type::RunJavaScriptPromptResultListener> {
public:
    static PassRefPtr<RunJavaScriptPromptResultListener> create(std::function<void (const String&)>&& completionHandler)
    {
        return adoptRef(new RunJavaScriptPromptResultListener(WTFMove(completionHandler)));
    }

    virtual ~RunJavaScriptPromptResultListener()
    {
    }

    void call(const String& result)
    {
        m_completionHandler(result);
    }

private:
    explicit RunJavaScriptPromptResultListener(std::function<void (const String&)>&& completionHandler)
        : m_completionHandler(WTFMove(completionHandler))
    {
    }

    std::function<void (const String&)> m_completionHandler;
};

WK_ADD_API_MAPPING(WKPageRunBeforeUnloadConfirmPanelResultListenerRef, RunBeforeUnloadConfirmPanelResultListener)
WK_ADD_API_MAPPING(WKPageRunJavaScriptAlertResultListenerRef, RunJavaScriptAlertResultListener)
WK_ADD_API_MAPPING(WKPageRunJavaScriptConfirmResultListenerRef, RunJavaScriptConfirmResultListener)
WK_ADD_API_MAPPING(WKPageRunJavaScriptPromptResultListenerRef, RunJavaScriptPromptResultListener)

}

WKTypeID WKPageRunBeforeUnloadConfirmPanelResultListenerGetTypeID()
{
    return toAPI(RunBeforeUnloadConfirmPanelResultListener::APIType);
}

void WKPageRunBeforeUnloadConfirmPanelResultListenerCall(WKPageRunBeforeUnloadConfirmPanelResultListenerRef listener, bool result)
{
    toImpl(listener)->call(result);
}

WKTypeID WKPageRunJavaScriptAlertResultListenerGetTypeID()
{
    return toAPI(RunJavaScriptAlertResultListener::APIType);
}

void WKPageRunJavaScriptAlertResultListenerCall(WKPageRunJavaScriptAlertResultListenerRef listener)
{
    toImpl(listener)->call();
}

WKTypeID WKPageRunJavaScriptConfirmResultListenerGetTypeID()
{
    return toAPI(RunJavaScriptConfirmResultListener::APIType);
}

void WKPageRunJavaScriptConfirmResultListenerCall(WKPageRunJavaScriptConfirmResultListenerRef listener, bool result)
{
    toImpl(listener)->call(result);
}

WKTypeID WKPageRunJavaScriptPromptResultListenerGetTypeID()
{
    return toAPI(RunJavaScriptPromptResultListener::APIType);
}

void WKPageRunJavaScriptPromptResultListenerCall(WKPageRunJavaScriptPromptResultListenerRef listener, WKStringRef result)
{
    toImpl(listener)->call(toWTFString(result));
}

void WKPageSetPageUIClient(WKPageRef pageRef, const WKPageUIClientBase* wkClient)
{
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED <= 101000
    if (wkClient && wkClient->version == 4) {
        fixUpBotchedPageUIClient(pageRef, *wkClient);
        return;
    }
#endif

    class UIClient : public API::Client<WKPageUIClientBase>, public API::UIClient {
    public:
        explicit UIClient(const WKPageUIClientBase* client)
        {
            initialize(client);
        }

    private:
        PassRefPtr<WebPageProxy> createNewPage(WebPageProxy* page, WebFrameProxy* initiatingFrame, const SecurityOriginData& securityOriginData, const ResourceRequest& resourceRequest, const WindowFeatures& windowFeatures, const NavigationActionData& navigationActionData) override
        {
            if (m_client.createNewPage) {
                auto configuration = page->configuration().copy();
                configuration->setRelatedPage(page);

                auto sourceFrameInfo = API::FrameInfo::create(*initiatingFrame, securityOriginData.securityOrigin());

                auto userInitiatedActivity = page->process().userInitiatedActivity(navigationActionData.userGestureTokenIdentifier);
                bool shouldOpenAppLinks = !hostsAreEqual(WebCore::URL(WebCore::ParsedURLString, initiatingFrame->url()), resourceRequest.url());
                auto apiNavigationAction = API::NavigationAction::create(navigationActionData, sourceFrameInfo.ptr(), nullptr, resourceRequest, WebCore::URL(), shouldOpenAppLinks, userInitiatedActivity);

                auto apiWindowFeatures = API::WindowFeatures::create(windowFeatures);

                return adoptRef(toImpl(m_client.createNewPage(toAPI(page), toAPI(configuration.ptr()), toAPI(apiNavigationAction.ptr()), toAPI(apiWindowFeatures.ptr()), m_client.base.clientInfo)));
            }
        
            if (m_client.createNewPage_deprecatedForUseWithV1 || m_client.createNewPage_deprecatedForUseWithV0) {
                API::Dictionary::MapType map;
                if (windowFeatures.x)
                    map.set("x", API::Double::create(*windowFeatures.x));
                if (windowFeatures.y)
                    map.set("y", API::Double::create(*windowFeatures.y));
                if (windowFeatures.width)
                    map.set("width", API::Double::create(*windowFeatures.width));
                if (windowFeatures.height)
                    map.set("height", API::Double::create(*windowFeatures.height));
                map.set("menuBarVisible", API::Boolean::create(windowFeatures.menuBarVisible));
                map.set("statusBarVisible", API::Boolean::create(windowFeatures.statusBarVisible));
                map.set("toolBarVisible", API::Boolean::create(windowFeatures.toolBarVisible));
                map.set("locationBarVisible", API::Boolean::create(windowFeatures.locationBarVisible));
                map.set("scrollbarsVisible", API::Boolean::create(windowFeatures.scrollbarsVisible));
                map.set("resizable", API::Boolean::create(windowFeatures.resizable));
                map.set("fullscreen", API::Boolean::create(windowFeatures.fullscreen));
                map.set("dialog", API::Boolean::create(windowFeatures.dialog));
                Ref<API::Dictionary> featuresMap = API::Dictionary::create(WTFMove(map));

                if (m_client.createNewPage_deprecatedForUseWithV1) {
                    Ref<API::URLRequest> request = API::URLRequest::create(resourceRequest);
                    return adoptRef(toImpl(m_client.createNewPage_deprecatedForUseWithV1(toAPI(page), toAPI(request.ptr()), toAPI(featuresMap.ptr()), toAPI(navigationActionData.modifiers), toAPI(navigationActionData.mouseButton), m_client.base.clientInfo)));
                }
    
                ASSERT(m_client.createNewPage_deprecatedForUseWithV0);
                return adoptRef(toImpl(m_client.createNewPage_deprecatedForUseWithV0(toAPI(page), toAPI(featuresMap.ptr()), toAPI(navigationActionData.modifiers), toAPI(navigationActionData.mouseButton), m_client.base.clientInfo)));
            }

            return nullptr;
        }

        void showPage(WebPageProxy* page) override
        {
            if (!m_client.showPage)
                return;

            m_client.showPage(toAPI(page), m_client.base.clientInfo);
        }

        void fullscreenMayReturnToInline(WebPageProxy* page) override
        {
            if (!m_client.fullscreenMayReturnToInline)
                return;

            m_client.fullscreenMayReturnToInline(toAPI(page), m_client.base.clientInfo);
        }

        void close(WebPageProxy* page) override
        {
            if (!m_client.close)
                return;

            m_client.close(toAPI(page), m_client.base.clientInfo);
        }

        void takeFocus(WebPageProxy* page, WKFocusDirection direction) override
        {
            if (!m_client.takeFocus)
                return;

            m_client.takeFocus(toAPI(page), direction, m_client.base.clientInfo);
        }

        void focus(WebPageProxy* page) override
        {
            if (!m_client.focus)
                return;

            m_client.focus(toAPI(page), m_client.base.clientInfo);
        }

        void unfocus(WebPageProxy* page) override
        {
            if (!m_client.unfocus)
                return;

            m_client.unfocus(toAPI(page), m_client.base.clientInfo);
        }

        void runJavaScriptAlert(WebPageProxy* page, const String& message, WebFrameProxy* frame, const SecurityOriginData& securityOriginData, std::function<void ()> completionHandler) override
        {
            if (m_client.runJavaScriptAlert) {
                RefPtr<RunJavaScriptAlertResultListener> listener = RunJavaScriptAlertResultListener::create(WTFMove(completionHandler));
                RefPtr<API::SecurityOrigin> securityOrigin = API::SecurityOrigin::create(securityOriginData.protocol, securityOriginData.host, securityOriginData.port);
                m_client.runJavaScriptAlert(toAPI(page), toAPI(message.impl()), toAPI(frame), toAPI(securityOrigin.get()), toAPI(listener.get()), m_client.base.clientInfo);
                return;
            }

            if (m_client.runJavaScriptAlert_deprecatedForUseWithV5) {
                RefPtr<API::SecurityOrigin> securityOrigin = API::SecurityOrigin::create(securityOriginData.protocol, securityOriginData.host, securityOriginData.port);
                m_client.runJavaScriptAlert_deprecatedForUseWithV5(toAPI(page), toAPI(message.impl()), toAPI(frame), toAPI(securityOrigin.get()), m_client.base.clientInfo);
                completionHandler();
                return;
            }
            
            if (m_client.runJavaScriptAlert_deprecatedForUseWithV0) {
                m_client.runJavaScriptAlert_deprecatedForUseWithV0(toAPI(page), toAPI(message.impl()), toAPI(frame), m_client.base.clientInfo);
                completionHandler();
                return;
            }


            completionHandler();
        }

        void runJavaScriptConfirm(WebPageProxy* page, const String& message, WebFrameProxy* frame, const SecurityOriginData& securityOriginData, std::function<void (bool)> completionHandler) override
        {
            if (m_client.runJavaScriptConfirm) {
                RefPtr<RunJavaScriptConfirmResultListener> listener = RunJavaScriptConfirmResultListener::create(WTFMove(completionHandler));
                RefPtr<API::SecurityOrigin> securityOrigin = API::SecurityOrigin::create(securityOriginData.protocol, securityOriginData.host, securityOriginData.port);
                m_client.runJavaScriptConfirm(toAPI(page), toAPI(message.impl()), toAPI(frame), toAPI(securityOrigin.get()), toAPI(listener.get()), m_client.base.clientInfo);
                return;
            }

            if (m_client.runJavaScriptConfirm_deprecatedForUseWithV5) {
                RefPtr<API::SecurityOrigin> securityOrigin = API::SecurityOrigin::create(securityOriginData.protocol, securityOriginData.host, securityOriginData.port);
                bool result = m_client.runJavaScriptConfirm_deprecatedForUseWithV5(toAPI(page), toAPI(message.impl()), toAPI(frame), toAPI(securityOrigin.get()), m_client.base.clientInfo);
                
                completionHandler(result);
                return;
            }
            
            if (m_client.runJavaScriptConfirm_deprecatedForUseWithV0) {
                bool result = m_client.runJavaScriptConfirm_deprecatedForUseWithV0(toAPI(page), toAPI(message.impl()), toAPI(frame), m_client.base.clientInfo);

                completionHandler(result);
                return;
            }
            
            completionHandler(false);
        }

        void runJavaScriptPrompt(WebPageProxy* page, const String& message, const String& defaultValue, WebFrameProxy* frame, const SecurityOriginData& securityOriginData, std::function<void (const String&)> completionHandler) override
        {
            if (m_client.runJavaScriptPrompt) {
                RefPtr<RunJavaScriptPromptResultListener> listener = RunJavaScriptPromptResultListener::create(WTFMove(completionHandler));
                RefPtr<API::SecurityOrigin> securityOrigin = API::SecurityOrigin::create(securityOriginData.protocol, securityOriginData.host, securityOriginData.port);
                m_client.runJavaScriptPrompt(toAPI(page), toAPI(message.impl()), toAPI(defaultValue.impl()), toAPI(frame), toAPI(securityOrigin.get()), toAPI(listener.get()), m_client.base.clientInfo);
                return;
            }

            if (m_client.runJavaScriptPrompt_deprecatedForUseWithV5) {
                RefPtr<API::SecurityOrigin> securityOrigin = API::SecurityOrigin::create(securityOriginData.protocol, securityOriginData.host, securityOriginData.port);
                RefPtr<API::String> string = adoptRef(toImpl(m_client.runJavaScriptPrompt_deprecatedForUseWithV5(toAPI(page), toAPI(message.impl()), toAPI(defaultValue.impl()), toAPI(frame), toAPI(securityOrigin.get()), m_client.base.clientInfo)));
                
                if (string)
                    completionHandler(string->string());
                else
                    completionHandler(String());
                return;
            }
            
            if (m_client.runJavaScriptPrompt_deprecatedForUseWithV0) {
                RefPtr<API::String> string = adoptRef(toImpl(m_client.runJavaScriptPrompt_deprecatedForUseWithV0(toAPI(page), toAPI(message.impl()), toAPI(defaultValue.impl()), toAPI(frame), m_client.base.clientInfo)));
                
                if (string)
                    completionHandler(string->string());
                else
                    completionHandler(String());
                return;
            }

            completionHandler(String());
        }

        void setStatusText(WebPageProxy* page, const String& text) override
        {
            if (!m_client.setStatusText)
                return;

            m_client.setStatusText(toAPI(page), toAPI(text.impl()), m_client.base.clientInfo);
        }

        void mouseDidMoveOverElement(WebPageProxy* page, const WebHitTestResultData& data, WebEvent::Modifiers modifiers, API::Object* userData) override
        {
            if (!m_client.mouseDidMoveOverElement && !m_client.mouseDidMoveOverElement_deprecatedForUseWithV0)
                return;

            if (m_client.base.version > 0 && !m_client.mouseDidMoveOverElement)
                return;

            if (!m_client.base.version) {
                m_client.mouseDidMoveOverElement_deprecatedForUseWithV0(toAPI(page), toAPI(modifiers), toAPI(userData), m_client.base.clientInfo);
                return;
            }

            RefPtr<API::HitTestResult> webHitTestResult = API::HitTestResult::create(data);
            m_client.mouseDidMoveOverElement(toAPI(page), toAPI(webHitTestResult.get()), toAPI(modifiers), toAPI(userData), m_client.base.clientInfo);
        }

#if ENABLE(NETSCAPE_PLUGIN_API)
        void unavailablePluginButtonClicked(WebPageProxy* page, WKPluginUnavailabilityReason pluginUnavailabilityReason, API::Dictionary* pluginInformation) override
        {
            if (pluginUnavailabilityReason == kWKPluginUnavailabilityReasonPluginMissing) {
                if (m_client.missingPluginButtonClicked_deprecatedForUseWithV0)
                    m_client.missingPluginButtonClicked_deprecatedForUseWithV0(
                        toAPI(page),
                        toAPI(pluginInformation->get<API::String>(pluginInformationMIMETypeKey())),
                        toAPI(pluginInformation->get<API::String>(pluginInformationPluginURLKey())),
                        toAPI(pluginInformation->get<API::String>(pluginInformationPluginspageAttributeURLKey())),
                        m_client.base.clientInfo);
            }

            if (m_client.unavailablePluginButtonClicked_deprecatedForUseWithV1)
                m_client.unavailablePluginButtonClicked_deprecatedForUseWithV1(
                    toAPI(page),
                    pluginUnavailabilityReason,
                    toAPI(pluginInformation->get<API::String>(pluginInformationMIMETypeKey())),
                    toAPI(pluginInformation->get<API::String>(pluginInformationPluginURLKey())),
                    toAPI(pluginInformation->get<API::String>(pluginInformationPluginspageAttributeURLKey())),
                    m_client.base.clientInfo);

            if (m_client.unavailablePluginButtonClicked)
                m_client.unavailablePluginButtonClicked(
                    toAPI(page),
                    pluginUnavailabilityReason,
                    toAPI(pluginInformation),
                    m_client.base.clientInfo);
        }
#endif // ENABLE(NETSCAPE_PLUGIN_API)

        bool implementsDidNotHandleKeyEvent() const override
        {
            return m_client.didNotHandleKeyEvent;
        }

        void didNotHandleKeyEvent(WebPageProxy* page, const NativeWebKeyboardEvent& event) override
        {
            if (!m_client.didNotHandleKeyEvent)
                return;
            m_client.didNotHandleKeyEvent(toAPI(page), event.nativeEvent(), m_client.base.clientInfo);
        }

        bool implementsDidNotHandleWheelEvent() const override
        {
            return m_client.didNotHandleWheelEvent;
        }

        void didNotHandleWheelEvent(WebPageProxy* page, const NativeWebWheelEvent& event) override
        {
            if (!m_client.didNotHandleWheelEvent)
                return;
            m_client.didNotHandleWheelEvent(toAPI(page), event.nativeEvent(), m_client.base.clientInfo);
        }

        bool toolbarsAreVisible(WebPageProxy* page) override
        {
            if (!m_client.toolbarsAreVisible)
                return true;
            return m_client.toolbarsAreVisible(toAPI(page), m_client.base.clientInfo);
        }

        void setToolbarsAreVisible(WebPageProxy* page, bool visible) override
        {
            if (!m_client.setToolbarsAreVisible)
                return;
            m_client.setToolbarsAreVisible(toAPI(page), visible, m_client.base.clientInfo);
        }

        bool menuBarIsVisible(WebPageProxy* page) override
        {
            if (!m_client.menuBarIsVisible)
                return true;
            return m_client.menuBarIsVisible(toAPI(page), m_client.base.clientInfo);
        }

        void setMenuBarIsVisible(WebPageProxy* page, bool visible) override
        {
            if (!m_client.setMenuBarIsVisible)
                return;
            m_client.setMenuBarIsVisible(toAPI(page), visible, m_client.base.clientInfo);
        }

        bool statusBarIsVisible(WebPageProxy* page) override
        {
            if (!m_client.statusBarIsVisible)
                return true;
            return m_client.statusBarIsVisible(toAPI(page), m_client.base.clientInfo);
        }

        void setStatusBarIsVisible(WebPageProxy* page, bool visible) override
        {
            if (!m_client.setStatusBarIsVisible)
                return;
            m_client.setStatusBarIsVisible(toAPI(page), visible, m_client.base.clientInfo);
        }

        bool isResizable(WebPageProxy* page) override
        {
            if (!m_client.isResizable)
                return true;
            return m_client.isResizable(toAPI(page), m_client.base.clientInfo);
        }

        void setIsResizable(WebPageProxy* page, bool resizable) override
        {
            if (!m_client.setIsResizable)
                return;
            m_client.setIsResizable(toAPI(page), resizable, m_client.base.clientInfo);
        }

        void setWindowFrame(WebPageProxy* page, const FloatRect& frame) override
        {
            if (!m_client.setWindowFrame)
                return;

            m_client.setWindowFrame(toAPI(page), toAPI(frame), m_client.base.clientInfo);
        }

        FloatRect windowFrame(WebPageProxy* page) override
        {
            if (!m_client.getWindowFrame)
                return FloatRect();

            return toFloatRect(m_client.getWindowFrame(toAPI(page), m_client.base.clientInfo));
        }

        bool canRunBeforeUnloadConfirmPanel() const override
        {
            return m_client.runBeforeUnloadConfirmPanel_deprecatedForUseWithV6 || m_client.runBeforeUnloadConfirmPanel;
        }

        void runBeforeUnloadConfirmPanel(WebKit::WebPageProxy* page, const WTF::String& message, WebKit::WebFrameProxy* frame, std::function<void (bool)> completionHandler) override
        {
            if (m_client.runBeforeUnloadConfirmPanel) {
                RefPtr<RunBeforeUnloadConfirmPanelResultListener> listener = RunBeforeUnloadConfirmPanelResultListener::create(WTFMove(completionHandler));
                m_client.runBeforeUnloadConfirmPanel(toAPI(page), toAPI(message.impl()), toAPI(frame), toAPI(listener.get()), m_client.base.clientInfo);
                return;
            }

            if (m_client.runBeforeUnloadConfirmPanel_deprecatedForUseWithV6) {
                bool result = m_client.runBeforeUnloadConfirmPanel_deprecatedForUseWithV6(toAPI(page), toAPI(message.impl()), toAPI(frame), m_client.base.clientInfo);
                completionHandler(result);
                return;
            }

            completionHandler(true);
        }

        void pageDidScroll(WebPageProxy* page) override
        {
            if (!m_client.pageDidScroll)
                return;

            m_client.pageDidScroll(toAPI(page), m_client.base.clientInfo);
        }

        void exceededDatabaseQuota(WebPageProxy* page, WebFrameProxy* frame, API::SecurityOrigin* origin, const String& databaseName, const String& databaseDisplayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, std::function<void (unsigned long long)> completionHandler) override
        {
            if (!m_client.exceededDatabaseQuota) {
                completionHandler(currentQuota);
                return;
            }

            completionHandler(m_client.exceededDatabaseQuota(toAPI(page), toAPI(frame), toAPI(origin), toAPI(databaseName.impl()), toAPI(databaseDisplayName.impl()), currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage, m_client.base.clientInfo));
        }

        bool runOpenPanel(WebPageProxy* page, WebFrameProxy* frame, const WebCore::SecurityOriginData&, API::OpenPanelParameters* parameters, WebOpenPanelResultListenerProxy* listener) override
        {
            if (!m_client.runOpenPanel)
                return false;

            m_client.runOpenPanel(toAPI(page), toAPI(frame), toAPI(parameters), toAPI(listener), m_client.base.clientInfo);
            return true;
        }

        bool decidePolicyForGeolocationPermissionRequest(WebPageProxy* page, WebFrameProxy* frame, API::SecurityOrigin* origin, GeolocationPermissionRequestProxy* permissionRequest) override
        {
            if (!m_client.decidePolicyForGeolocationPermissionRequest)
                return false;

            m_client.decidePolicyForGeolocationPermissionRequest(toAPI(page), toAPI(frame), toAPI(origin), toAPI(permissionRequest), m_client.base.clientInfo);
            return true;
        }

        bool decidePolicyForUserMediaPermissionRequest(WebPageProxy& page, WebFrameProxy& frame, API::SecurityOrigin& userMediaDocumentOrigin, API::SecurityOrigin& topLevelDocumentOrigin, UserMediaPermissionRequestProxy& permissionRequest) override
        {
            if (!m_client.decidePolicyForUserMediaPermissionRequest)
                return false;

            m_client.decidePolicyForUserMediaPermissionRequest(toAPI(&page), toAPI(&frame), toAPI(&userMediaDocumentOrigin), toAPI(&topLevelDocumentOrigin), toAPI(&permissionRequest), m_client.base.clientInfo);
            return true;
        }

        bool checkUserMediaPermissionForOrigin(WebPageProxy& page, WebFrameProxy& frame, API::SecurityOrigin& userMediaDocumentOrigin, API::SecurityOrigin& topLevelDocumentOrigin, UserMediaPermissionCheckProxy& request) override
        {
            if (!m_client.checkUserMediaPermissionForOrigin)
                return false;

            m_client.checkUserMediaPermissionForOrigin(toAPI(&page), toAPI(&frame), toAPI(&userMediaDocumentOrigin), toAPI(&topLevelDocumentOrigin), toAPI(&request), m_client.base.clientInfo);
            return true;
        }
        
        bool decidePolicyForNotificationPermissionRequest(WebPageProxy* page, API::SecurityOrigin* origin, NotificationPermissionRequest* permissionRequest) override
        {
            if (!m_client.decidePolicyForNotificationPermissionRequest)
                return false;

            m_client.decidePolicyForNotificationPermissionRequest(toAPI(page), toAPI(origin), toAPI(permissionRequest), m_client.base.clientInfo);
            return true;
        }

        // Printing.
        float headerHeight(WebPageProxy* page, WebFrameProxy* frame) override
        {
            if (!m_client.headerHeight)
                return 0;

            return m_client.headerHeight(toAPI(page), toAPI(frame), m_client.base.clientInfo);
        }

        float footerHeight(WebPageProxy* page, WebFrameProxy* frame) override
        {
            if (!m_client.footerHeight)
                return 0;

            return m_client.footerHeight(toAPI(page), toAPI(frame), m_client.base.clientInfo);
        }

        void drawHeader(WebPageProxy* page, WebFrameProxy* frame, const WebCore::FloatRect& rect) override
        {
            if (!m_client.drawHeader)
                return;

            m_client.drawHeader(toAPI(page), toAPI(frame), toAPI(rect), m_client.base.clientInfo);
        }

        void drawFooter(WebPageProxy* page, WebFrameProxy* frame, const WebCore::FloatRect& rect) override
        {
            if (!m_client.drawFooter)
                return;

            m_client.drawFooter(toAPI(page), toAPI(frame), toAPI(rect), m_client.base.clientInfo);
        }

        void printFrame(WebPageProxy* page, WebFrameProxy* frame) override
        {
            if (!m_client.printFrame)
                return;

            m_client.printFrame(toAPI(page), toAPI(frame), m_client.base.clientInfo);
        }

        bool canRunModal() const override
        {
            return m_client.runModal;
        }

        void runModal(WebPageProxy* page) override
        {
            if (!m_client.runModal)
                return;

            m_client.runModal(toAPI(page), m_client.base.clientInfo);
        }

        void saveDataToFileInDownloadsFolder(WebPageProxy* page, const String& suggestedFilename, const String& mimeType, const String& originatingURLString, API::Data* data) override
        {
            if (!m_client.saveDataToFileInDownloadsFolder)
                return;

            m_client.saveDataToFileInDownloadsFolder(toAPI(page), toAPI(suggestedFilename.impl()), toAPI(mimeType.impl()), toURLRef(originatingURLString.impl()), toAPI(data), m_client.base.clientInfo);
        }

        void pinnedStateDidChange(WebPageProxy& page) override
        {
            if (!m_client.pinnedStateDidChange)
                return;

            m_client.pinnedStateDidChange(toAPI(&page), m_client.base.clientInfo);
        }

        void isPlayingAudioDidChange(WebPageProxy& page) override
        {
            if (!m_client.isPlayingAudioDidChange)
                return;

            m_client.isPlayingAudioDidChange(toAPI(&page), m_client.base.clientInfo);
        }

        void didClickAutoFillButton(WebPageProxy& page, API::Object* userInfo) override
        {
            if (!m_client.didClickAutoFillButton)
                return;

            m_client.didClickAutoFillButton(toAPI(&page), toAPI(userInfo), m_client.base.clientInfo);
        }

#if ENABLE(MEDIA_SESSION)
        void mediaSessionMetadataDidChange(WebPageProxy& page, WebMediaSessionMetadata* metadata) override
        {
            if (!m_client.mediaSessionMetadataDidChange)
                return;

            m_client.mediaSessionMetadataDidChange(toAPI(&page), toAPI(metadata), m_client.base.clientInfo);
        }
#endif
    };

    toImpl(pageRef)->setUIClient(std::make_unique<UIClient>(wkClient));
}

void WKPageSetPageNavigationClient(WKPageRef pageRef, const WKPageNavigationClientBase* wkClient)
{
    class NavigationClient : public API::Client<WKPageNavigationClientBase>, public API::NavigationClient {
    public:
        explicit NavigationClient(const WKPageNavigationClientBase* client)
        {
            initialize(client);
        }

    private:
        void decidePolicyForNavigationAction(WebPageProxy& page, API::NavigationAction& navigationAction, Ref<WebKit::WebFramePolicyListenerProxy>&& listener, API::Object* userData) override
        {
            if (!m_client.decidePolicyForNavigationAction)
                return;
            m_client.decidePolicyForNavigationAction(toAPI(&page), toAPI(&navigationAction), toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
        }

        void decidePolicyForNavigationResponse(WebPageProxy& page, API::NavigationResponse& navigationResponse, Ref<WebKit::WebFramePolicyListenerProxy>&& listener, API::Object* userData) override
        {
            if (!m_client.decidePolicyForNavigationResponse)
                return;
            m_client.decidePolicyForNavigationResponse(toAPI(&page), toAPI(&navigationResponse), toAPI(listener.ptr()), toAPI(userData), m_client.base.clientInfo);
        }

        void didStartProvisionalNavigation(WebPageProxy& page, API::Navigation* navigation, API::Object* userData) override
        {
            if (!m_client.didStartProvisionalNavigation)
                return;
            m_client.didStartProvisionalNavigation(toAPI(&page), toAPI(navigation), toAPI(userData), m_client.base.clientInfo);
        }

        void didReceiveServerRedirectForProvisionalNavigation(WebPageProxy& page, API::Navigation* navigation, API::Object* userData) override
        {
            if (!m_client.didReceiveServerRedirectForProvisionalNavigation)
                return;
            m_client.didReceiveServerRedirectForProvisionalNavigation(toAPI(&page), toAPI(navigation), toAPI(userData), m_client.base.clientInfo);
        }

        void didFailProvisionalNavigationWithError(WebPageProxy& page, WebFrameProxy&, API::Navigation* navigation, const WebCore::ResourceError& error, API::Object* userData) override
        {
            if (!m_client.didFailProvisionalNavigation)
                return;
            m_client.didFailProvisionalNavigation(toAPI(&page), toAPI(navigation), toAPI(error), toAPI(userData), m_client.base.clientInfo);
        }

        void didCommitNavigation(WebPageProxy& page, API::Navigation* navigation, API::Object* userData) override
        {
            if (!m_client.didCommitNavigation)
                return;
            m_client.didCommitNavigation(toAPI(&page), toAPI(navigation), toAPI(userData), m_client.base.clientInfo);
        }

        void didFinishNavigation(WebPageProxy& page, API::Navigation* navigation, API::Object* userData) override
        {
            if (!m_client.didFinishNavigation)
                return;
            m_client.didFinishNavigation(toAPI(&page), toAPI(navigation), toAPI(userData), m_client.base.clientInfo);
        }

        void didFailNavigationWithError(WebPageProxy& page, WebFrameProxy&, API::Navigation* navigation, const WebCore::ResourceError& error, API::Object* userData) override
        {
            if (!m_client.didFailNavigation)
                return;
            m_client.didFailNavigation(toAPI(&page), toAPI(navigation), toAPI(error), toAPI(userData), m_client.base.clientInfo);
        }

        void didFailProvisionalLoadInSubframeWithError(WebPageProxy& page, WebFrameProxy& subframe, const WebCore::SecurityOriginData& securityOriginData, API::Navigation* navigation, const WebCore::ResourceError& error, API::Object* userData) override
        {
            if (!m_client.didFailProvisionalLoadInSubframe)
                return;
            m_client.didFailProvisionalLoadInSubframe(toAPI(&page), toAPI(navigation), toAPI(API::FrameInfo::create(subframe, securityOriginData.securityOrigin()).ptr()), toAPI(error), toAPI(userData), m_client.base.clientInfo);
        }

        void didFinishDocumentLoad(WebPageProxy& page, API::Navigation* navigation, API::Object* userData) override
        {
            if (!m_client.didFinishDocumentLoad)
                return;
            m_client.didFinishDocumentLoad(toAPI(&page), toAPI(navigation), toAPI(userData), m_client.base.clientInfo);
        }

        void didSameDocumentNavigation(WebPageProxy& page, API::Navigation* navigation, WebKit::SameDocumentNavigationType navigationType, API::Object* userData) override
        {
            if (!m_client.didSameDocumentNavigation)
                return;
            m_client.didSameDocumentNavigation(toAPI(&page), toAPI(navigation), toAPI(navigationType), toAPI(userData), m_client.base.clientInfo);
        }
        
        void renderingProgressDidChange(WebPageProxy& page, WebCore::LayoutMilestones milestones) override
        {
            if (!m_client.renderingProgressDidChange)
                return;
            m_client.renderingProgressDidChange(toAPI(&page), pageRenderingProgressEvents(milestones), nullptr, m_client.base.clientInfo);
        }
        
        bool canAuthenticateAgainstProtectionSpace(WebPageProxy& page, WebProtectionSpace* protectionSpace) override
        {
            if (!m_client.canAuthenticateAgainstProtectionSpace)
                return false;
            return m_client.canAuthenticateAgainstProtectionSpace(toAPI(&page), toAPI(protectionSpace), m_client.base.clientInfo);
        }
        
        void didReceiveAuthenticationChallenge(WebPageProxy& page, AuthenticationChallengeProxy* authenticationChallenge) override
        {
            if (!m_client.didReceiveAuthenticationChallenge)
                return;
            m_client.didReceiveAuthenticationChallenge(toAPI(&page), toAPI(authenticationChallenge), m_client.base.clientInfo);
        }

        void processDidCrash(WebPageProxy& page) override
        {
            if (!m_client.webProcessDidCrash)
                return;
            m_client.webProcessDidCrash(toAPI(&page), m_client.base.clientInfo);
        }

        RefPtr<API::Data> webCryptoMasterKey(WebPageProxy& page) override
        {
            if (!m_client.copyWebCryptoMasterKey)
                return nullptr;
            return adoptRef(toImpl(m_client.copyWebCryptoMasterKey(toAPI(&page), m_client.base.clientInfo)));
        }

        void didBeginNavigationGesture(WebPageProxy& page) override
        {
            if (!m_client.didBeginNavigationGesture)
                return;
            m_client.didBeginNavigationGesture(toAPI(&page), m_client.base.clientInfo);
        }

        void didEndNavigationGesture(WebPageProxy& page, bool willNavigate, WebKit::WebBackForwardListItem& item) override
        {
            if (!m_client.didEndNavigationGesture)
                return;
            m_client.didEndNavigationGesture(toAPI(&page), willNavigate ? toAPI(&item) : nullptr, m_client.base.clientInfo);
        }

        void willEndNavigationGesture(WebPageProxy& page, bool willNavigate, WebKit::WebBackForwardListItem& item) override
        {
            if (!m_client.willEndNavigationGesture)
                return;
            m_client.willEndNavigationGesture(toAPI(&page), willNavigate ? toAPI(&item) : nullptr, m_client.base.clientInfo);
        }

        void didRemoveNavigationGestureSnapshot(WebPageProxy& page) override
        {
            if (!m_client.didRemoveNavigationGestureSnapshot)
                return;
            m_client.didRemoveNavigationGestureSnapshot(toAPI(&page), m_client.base.clientInfo);
        }
        
#if ENABLE(NETSCAPE_PLUGIN_API)
        PluginModuleLoadPolicy decidePolicyForPluginLoad(WebPageProxy& page, PluginModuleLoadPolicy currentPluginLoadPolicy, API::Dictionary* pluginInformation, String& unavailabilityDescription) override
        {
            WKStringRef unavailabilityDescriptionOut = 0;
            PluginModuleLoadPolicy loadPolicy = currentPluginLoadPolicy;
            
            if (m_client.decidePolicyForPluginLoad)
                loadPolicy = toPluginModuleLoadPolicy(m_client.decidePolicyForPluginLoad(toAPI(&page), toWKPluginLoadPolicy(currentPluginLoadPolicy), toAPI(pluginInformation), &unavailabilityDescriptionOut, m_client.base.clientInfo));
            
            if (unavailabilityDescriptionOut) {
                RefPtr<API::String> webUnavailabilityDescription = adoptRef(toImpl(unavailabilityDescriptionOut));
                unavailabilityDescription = webUnavailabilityDescription->string();
            }
            
            return loadPolicy;
        }
#endif
    };

    WebPageProxy* webPageProxy = toImpl(pageRef);

    auto navigationClient = std::make_unique<NavigationClient>(wkClient);
    webPageProxy->setNavigationClient(WTFMove(navigationClient));
}

void WKPageSetSession(WKPageRef pageRef, WKSessionRef session)
{
    toImpl(pageRef)->setSessionID(toImpl(session)->getID());
}

void WKPageRunJavaScriptInMainFrame(WKPageRef pageRef, WKStringRef scriptRef, void* context, WKPageRunJavaScriptFunction callback)
{
    toImpl(pageRef)->runJavaScriptInMainFrame(toImpl(scriptRef)->string(), [context, callback](API::SerializedScriptValue* returnValue, bool, const WebCore::ExceptionDetails&, CallbackBase::Error error) {
        callback(toAPI(returnValue), (error != CallbackBase::Error::None) ? toAPI(API::Error::create().ptr()) : 0, context);
    });
}

#ifdef __BLOCKS__
static void callRunJavaScriptBlockAndRelease(WKSerializedScriptValueRef resultValue, WKErrorRef error, void* context)
{
    WKPageRunJavaScriptBlock block = (WKPageRunJavaScriptBlock)context;
    block(resultValue, error);
    Block_release(block);
}

void WKPageRunJavaScriptInMainFrame_b(WKPageRef pageRef, WKStringRef scriptRef, WKPageRunJavaScriptBlock block)
{
    WKPageRunJavaScriptInMainFrame(pageRef, scriptRef, Block_copy(block), callRunJavaScriptBlockAndRelease);
}
#endif

static std::function<void (const String&, WebKit::CallbackBase::Error)> toGenericCallbackFunction(void* context, void (*callback)(WKStringRef, WKErrorRef, void*))
{
    return [context, callback](const String& returnValue, WebKit::CallbackBase::Error error) {
        callback(toAPI(API::String::create(returnValue).ptr()), error != WebKit::CallbackBase::Error::None ? toAPI(API::Error::create().ptr()) : 0, context);
    };
}

void WKPageRenderTreeExternalRepresentation(WKPageRef pageRef, void* context, WKPageRenderTreeExternalRepresentationFunction callback)
{
    toImpl(pageRef)->getRenderTreeExternalRepresentation(toGenericCallbackFunction(context, callback));
}

void WKPageGetSourceForFrame(WKPageRef pageRef, WKFrameRef frameRef, void* context, WKPageGetSourceForFrameFunction callback)
{
    toImpl(pageRef)->getSourceForFrame(toImpl(frameRef), toGenericCallbackFunction(context, callback));
}

void WKPageGetContentsAsString(WKPageRef pageRef, void* context, WKPageGetContentsAsStringFunction callback)
{
    toImpl(pageRef)->getContentsAsString(toGenericCallbackFunction(context, callback));
}

void WKPageGetBytecodeProfile(WKPageRef pageRef, void* context, WKPageGetBytecodeProfileFunction callback)
{
    toImpl(pageRef)->getBytecodeProfile(toGenericCallbackFunction(context, callback));
}

void WKPageIsWebProcessResponsive(WKPageRef pageRef, void* context, WKPageIsWebProcessResponsiveFunction callback)
{
    toImpl(pageRef)->isWebProcessResponsive([context, callback](bool isWebProcessResponsive) {
        callback(isWebProcessResponsive, context);
    });
}

void WKPageGetSelectionAsWebArchiveData(WKPageRef pageRef, void* context, WKPageGetSelectionAsWebArchiveDataFunction callback)
{
    toImpl(pageRef)->getSelectionAsWebArchiveData(toGenericCallbackFunction(context, callback));
}

void WKPageGetContentsAsMHTMLData(WKPageRef pageRef, void* context, WKPageGetContentsAsMHTMLDataFunction callback)
{
#if ENABLE(MHTML)
    toImpl(pageRef)->getContentsAsMHTMLData(toGenericCallbackFunction(context, callback));
#else
    UNUSED_PARAM(pageRef);
    UNUSED_PARAM(context);
    UNUSED_PARAM(callback);
#endif
}

void WKPageForceRepaint(WKPageRef pageRef, void* context, WKPageForceRepaintFunction callback)
{
    toImpl(pageRef)->forceRepaint(VoidCallback::create([context, callback](WebKit::CallbackBase::Error error) {
        callback(error == WebKit::CallbackBase::Error::None ? nullptr : toAPI(API::Error::create().ptr()), context);
    }));
}

WK_EXPORT WKURLRef WKPageCopyPendingAPIRequestURL(WKPageRef pageRef)
{
    const String& pendingAPIRequestURL = toImpl(pageRef)->pageLoadState().pendingAPIRequestURL();

    if (pendingAPIRequestURL.isNull())
        return nullptr;

    return toCopiedURLAPI(pendingAPIRequestURL);
}

WKURLRef WKPageCopyActiveURL(WKPageRef pageRef)
{
    return toCopiedURLAPI(toImpl(pageRef)->pageLoadState().activeURL());
}

WKURLRef WKPageCopyProvisionalURL(WKPageRef pageRef)
{
    return toCopiedURLAPI(toImpl(pageRef)->pageLoadState().provisionalURL());
}

WKURLRef WKPageCopyCommittedURL(WKPageRef pageRef)
{
    return toCopiedURLAPI(toImpl(pageRef)->pageLoadState().url());
}

WKStringRef WKPageCopyStandardUserAgentWithApplicationName(WKStringRef applicationName)
{
    return toCopiedAPI(WebPageProxy::standardUserAgent(toImpl(applicationName)->string()));
}

void WKPageValidateCommand(WKPageRef pageRef, WKStringRef command, void* context, WKPageValidateCommandCallback callback)
{
    toImpl(pageRef)->validateCommand(toImpl(command)->string(), [context, callback](const String& commandName, bool isEnabled, int32_t state, WebKit::CallbackBase::Error error) {
        callback(toAPI(API::String::create(commandName).ptr()), isEnabled, state, error != WebKit::CallbackBase::Error::None ? toAPI(API::Error::create().ptr()) : 0, context);
    });
}

void WKPageExecuteCommand(WKPageRef pageRef, WKStringRef command)
{
    toImpl(pageRef)->executeEditCommand(toImpl(command)->string());
}

#if PLATFORM(COCOA)
static PrintInfo printInfoFromWKPrintInfo(const WKPrintInfo& printInfo)
{
    PrintInfo result;
    result.pageSetupScaleFactor = printInfo.pageSetupScaleFactor;
    result.availablePaperWidth = printInfo.availablePaperWidth;
    result.availablePaperHeight = printInfo.availablePaperHeight;
    return result;
}

void WKPageComputePagesForPrinting(WKPageRef page, WKFrameRef frame, WKPrintInfo printInfo, WKPageComputePagesForPrintingFunction callback, void* context)
{
    toImpl(page)->computePagesForPrinting(toImpl(frame), printInfoFromWKPrintInfo(printInfo), ComputedPagesCallback::create([context, callback](const Vector<WebCore::IntRect>& rects, double scaleFactor, WebKit::CallbackBase::Error error) {
        Vector<WKRect> wkRects(rects.size());
        for (size_t i = 0; i < rects.size(); ++i)
            wkRects[i] = toAPI(rects[i]);
        callback(wkRects.data(), wkRects.size(), scaleFactor, error != WebKit::CallbackBase::Error::None ? toAPI(API::Error::create().ptr()) : 0, context);
    }));
}

void WKPageBeginPrinting(WKPageRef page, WKFrameRef frame, WKPrintInfo printInfo)
{
    toImpl(page)->beginPrinting(toImpl(frame), printInfoFromWKPrintInfo(printInfo));
}

void WKPageDrawPagesToPDF(WKPageRef page, WKFrameRef frame, WKPrintInfo printInfo, uint32_t first, uint32_t count, WKPageDrawToPDFFunction callback, void* context)
{
    toImpl(page)->drawPagesToPDF(toImpl(frame), printInfoFromWKPrintInfo(printInfo), first, count, DataCallback::create(toGenericCallbackFunction(context, callback)));
}

void WKPageEndPrinting(WKPageRef page)
{
    toImpl(page)->endPrinting();
}
#endif

bool WKPageGetIsControlledByAutomation(WKPageRef page)
{
    return toImpl(page)->isControlledByAutomation();
}

void WKPageSetControlledByAutomation(WKPageRef page, bool controlled)
{
    toImpl(page)->setControlledByAutomation(controlled);
}

bool WKPageGetAllowsRemoteInspection(WKPageRef page)
{
#if ENABLE(REMOTE_INSPECTOR)
    return toImpl(page)->allowsRemoteInspection();
#else
    UNUSED_PARAM(page);
    return false;
#endif    
}

void WKPageSetAllowsRemoteInspection(WKPageRef page, bool allow)
{
#if ENABLE(REMOTE_INSPECTOR)
    toImpl(page)->setAllowsRemoteInspection(allow);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(allow);
#endif
}

void WKPageSetMediaVolume(WKPageRef page, float volume)
{
    toImpl(page)->setMediaVolume(volume);    
}

void WKPageSetMuted(WKPageRef page, bool muted)
{
    toImpl(page)->setMuted(muted);
}

bool WKPageHasMediaSessionWithActiveMediaElements(WKPageRef page)
{
#if ENABLE(MEDIA_SESSION)
    return toImpl(page)->hasMediaSessionWithActiveMediaElements();
#else
    UNUSED_PARAM(page);
    return false;
#endif
}

void WKPageHandleMediaEvent(WKPageRef page, WKMediaEventType wkEventType)
{
#if ENABLE(MEDIA_SESSION)
    MediaEventType eventType;

    switch (wkEventType) {
    case kWKMediaEventTypePlayPause:
        eventType = MediaEventType::PlayPause;
        break;
    case kWKMediaEventTypeTrackNext:
        eventType = MediaEventType::TrackNext;
        break;
    case kWKMediaEventTypeTrackPrevious:
        eventType = MediaEventType::TrackPrevious;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    toImpl(page)->handleMediaEvent(eventType);
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(wkEventType);
#endif
}

void WKPagePostMessageToInjectedBundle(WKPageRef pageRef, WKStringRef messageNameRef, WKTypeRef messageBodyRef)
{
    toImpl(pageRef)->postMessageToInjectedBundle(toImpl(messageNameRef)->string(), toImpl(messageBodyRef));
}

WKArrayRef WKPageCopyRelatedPages(WKPageRef pageRef)
{
    Vector<RefPtr<API::Object>> relatedPages;

    for (auto& page : toImpl(pageRef)->process().pages()) {
        if (page != toImpl(pageRef))
            relatedPages.append(page);
    }

    return toAPI(&API::Array::create(WTFMove(relatedPages)).leakRef());
}

WKFrameRef WKPageLookUpFrameFromHandle(WKPageRef pageRef, WKFrameHandleRef handleRef)
{
    auto page = toImpl(pageRef);
    auto frame = page->process().webFrame(toImpl(handleRef)->frameID());
    if (!frame || frame->page() != page)
        return nullptr;

    return toAPI(frame);
}

void WKPageSetMayStartMediaWhenInWindow(WKPageRef pageRef, bool mayStartMedia)
{
    toImpl(pageRef)->setMayStartMediaWhenInWindow(mayStartMedia);
}


void WKPageSelectContextMenuItem(WKPageRef page, WKContextMenuItemRef item)
{
#if ENABLE(CONTEXT_MENUS)
    toImpl(page)->contextMenuItemSelected((toImpl(item)->data()));
#else
    UNUSED_PARAM(page);
    UNUSED_PARAM(item);
#endif
}

WKScrollPinningBehavior WKPageGetScrollPinningBehavior(WKPageRef page)
{
    ScrollPinningBehavior pinning = toImpl(page)->scrollPinningBehavior();
    
    switch (pinning) {
    case WebCore::ScrollPinningBehavior::DoNotPin:
        return kWKScrollPinningBehaviorDoNotPin;
    case WebCore::ScrollPinningBehavior::PinToTop:
        return kWKScrollPinningBehaviorPinToTop;
    case WebCore::ScrollPinningBehavior::PinToBottom:
        return kWKScrollPinningBehaviorPinToBottom;
    }
    
    ASSERT_NOT_REACHED();
    return kWKScrollPinningBehaviorDoNotPin;
}

void WKPageSetScrollPinningBehavior(WKPageRef page, WKScrollPinningBehavior pinning)
{
    ScrollPinningBehavior corePinning = ScrollPinningBehavior::DoNotPin;

    switch (pinning) {
    case kWKScrollPinningBehaviorDoNotPin:
        corePinning = ScrollPinningBehavior::DoNotPin;
        break;
    case kWKScrollPinningBehaviorPinToTop:
        corePinning = ScrollPinningBehavior::PinToTop;
        break;
    case kWKScrollPinningBehaviorPinToBottom:
        corePinning = ScrollPinningBehavior::PinToBottom;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    
    toImpl(page)->setScrollPinningBehavior(corePinning);
}

bool WKPageGetAddsVisitedLinks(WKPageRef page)
{
    return toImpl(page)->addsVisitedLinks();
}

void WKPageSetAddsVisitedLinks(WKPageRef page, bool addsVisitedLinks)
{
    toImpl(page)->setAddsVisitedLinks(addsVisitedLinks);
}

bool WKPageIsPlayingAudio(WKPageRef page)
{
    return toImpl(page)->isPlayingAudio();
}

WKMediaState WKPageGetMediaState(WKPageRef page)
{
    WebCore::MediaProducer::MediaStateFlags coreState = toImpl(page)->mediaStateFlags();
    WKMediaState state = kWKMediaIsNotPlaying;

    if (coreState & WebCore::MediaProducer::IsPlayingAudio)
        state |= kWKMediaIsPlayingAudio;
    if (coreState & WebCore::MediaProducer::IsPlayingVideo)
        state |= kWKMediaIsPlayingVideo;
    if (coreState & WebCore::MediaProducer::HasActiveMediaCaptureDevice)
        state |= kWKMediaHasActiveCaptureDevice;

    return state;
}

void WKPageClearWheelEventTestTrigger(WKPageRef pageRef)
{
    toImpl(pageRef)->clearWheelEventTestTrigger();
}

void WKPageCallAfterNextPresentationUpdate(WKPageRef pageRef, void* context, WKPagePostPresentationUpdateFunction callback)
{
    toImpl(pageRef)->callAfterNextPresentationUpdate([context, callback](WebKit::CallbackBase::Error error) {
        callback(error != WebKit::CallbackBase::Error::None ? toAPI(API::Error::create().ptr()) : 0, context);
    });
}

bool WKPageGetResourceCachingDisabled(WKPageRef page)
{
    return toImpl(page)->isResourceCachingDisabled();
}

void WKPageSetResourceCachingDisabled(WKPageRef page, bool disabled)
{
    toImpl(page)->setResourceCachingDisabled(disabled);
}

void WKPageSetIgnoresViewportScaleLimits(WKPageRef page, bool ignoresViewportScaleLimits)
{
#if PLATFORM(IOS)
    toImpl(page)->setForceAlwaysUserScalable(ignoresViewportScaleLimits);
#endif
}

pid_t WKPageGetProcessIdentifier(WKPageRef page)
{
    return toImpl(page)->processIdentifier();
}

#if ENABLE(NETSCAPE_PLUGIN_API)

// -- DEPRECATED --

WKStringRef WKPageGetPluginInformationBundleIdentifierKey()
{
    return WKPluginInformationBundleIdentifierKey();
}

WKStringRef WKPageGetPluginInformationBundleVersionKey()
{
    return WKPluginInformationBundleVersionKey();
}

WKStringRef WKPageGetPluginInformationDisplayNameKey()
{
    return WKPluginInformationDisplayNameKey();
}

WKStringRef WKPageGetPluginInformationFrameURLKey()
{
    return WKPluginInformationFrameURLKey();
}

WKStringRef WKPageGetPluginInformationMIMETypeKey()
{
    return WKPluginInformationMIMETypeKey();
}

WKStringRef WKPageGetPluginInformationPageURLKey()
{
    return WKPluginInformationPageURLKey();
}

WKStringRef WKPageGetPluginInformationPluginspageAttributeURLKey()
{
    return WKPluginInformationPluginspageAttributeURLKey();
}

WKStringRef WKPageGetPluginInformationPluginURLKey()
{
    return WKPluginInformationPluginURLKey();
}

// -- DEPRECATED --

#endif // ENABLE(NETSCAPE_PLUGIN_API)
