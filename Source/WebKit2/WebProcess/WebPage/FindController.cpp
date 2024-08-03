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
#include "FindController.h"

#include "DrawingArea.h"
#include "PluginView.h"
#include "ShareableBitmap.h"
#include "WKPage.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/DocumentMarkerController.h>
#include <WebCore/FloatQuad.h>
#include <WebCore/FocusController.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/MainFrame.h>
#include <WebCore/Page.h>
#include <WebCore/PageOverlayController.h>
#include <WebCore/PlatformMouseEvent.h>
#include <WebCore/PluginDocument.h>

#if PLATFORM(COCOA)
#include <WebCore/TextIndicatorWindow.h>
#endif

using namespace WebCore;

namespace WebKit {

static WebCore::FindOptions core(FindOptions options)
{
    return (options & FindOptionsCaseInsensitive ? CaseInsensitive : 0)
        | (options & FindOptionsAtWordStarts ? AtWordStarts : 0)
        | (options & FindOptionsTreatMedialCapitalAsWordStart ? TreatMedialCapitalAsWordStart : 0)
        | (options & FindOptionsBackwards ? Backwards : 0)
        | (options & FindOptionsWrapAround ? WrapAround : 0);
}

FindController::FindController(WebPage* webPage)
    : m_webPage(webPage)
    , m_findPageOverlay(nullptr)
    , m_isShowingFindIndicator(false)
    , m_foundStringMatchIndex(-1)
{
}

FindController::~FindController()
{
}

static PluginView* pluginViewForFrame(Frame* frame)
{
    if (!frame->document()->isPluginDocument())
        return 0;

    PluginDocument* pluginDocument = static_cast<PluginDocument*>(frame->document());
    return static_cast<PluginView*>(pluginDocument->pluginWidget());
}

void FindController::countStringMatches(const String& string, FindOptions options, unsigned maxMatchCount)
{
    if (maxMatchCount == std::numeric_limits<unsigned>::max())
        --maxMatchCount;
    
    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());
    
    unsigned matchCount;

    if (pluginView)
        matchCount = pluginView->countFindMatches(string, core(options), maxMatchCount + 1);
    else {
        matchCount = m_webPage->corePage()->countFindMatches(string, core(options), maxMatchCount + 1);
        m_webPage->corePage()->unmarkAllTextMatches();
    }

    if (matchCount > maxMatchCount)
        matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
    
    m_webPage->send(Messages::WebPageProxy::DidCountStringMatches(string, matchCount));
}

static Frame* frameWithSelection(Page* page)
{
    for (Frame* frame = &page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (frame->selection().isRange())
            return frame;
    }

    return 0;
}

void FindController::updateFindUIAfterPageScroll(bool found, const String& string, FindOptions options, unsigned maxMatchCount)
{
    Frame* selectedFrame = frameWithSelection(m_webPage->corePage());
    
    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());

    bool shouldShowOverlay = false;

    if (!found) {
        if (!pluginView)
            m_webPage->corePage()->unmarkAllTextMatches();

        if (selectedFrame)
            selectedFrame->selection().clear();

        hideFindIndicator();
        didFailToFindString();

        m_webPage->send(Messages::WebPageProxy::DidFailToFindString(string));
    } else {
        shouldShowOverlay = options & FindOptionsShowOverlay;
        bool shouldShowHighlight = options & FindOptionsShowHighlight;
        bool shouldDetermineMatchIndex = options & FindOptionsDetermineMatchIndex;
        unsigned matchCount = 1;

        if (shouldDetermineMatchIndex) {
            if (pluginView)
                matchCount = pluginView->countFindMatches(string, core(options), maxMatchCount + 1);
            else
                matchCount = m_webPage->corePage()->countFindMatches(string, core(options), maxMatchCount + 1);
        }

        if (shouldShowOverlay || shouldShowHighlight) {
            if (maxMatchCount == std::numeric_limits<unsigned>::max())
                --maxMatchCount;

            if (pluginView) {
                if (!shouldDetermineMatchIndex)
                    matchCount = pluginView->countFindMatches(string, core(options), maxMatchCount + 1);
                shouldShowOverlay = false;
            } else {
                m_webPage->corePage()->unmarkAllTextMatches();
                matchCount = m_webPage->corePage()->markAllMatchesForText(string, core(options), shouldShowHighlight, maxMatchCount + 1);
            }

            // If we have a large number of matches, we don't want to take the time to paint the overlay.
            if (matchCount > maxMatchCount) {
                shouldShowOverlay = false;
                matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
            }
        }
        if (matchCount == static_cast<unsigned>(kWKMoreThanMaximumMatchCount))
            m_foundStringMatchIndex = -1;
        else {
            if (m_foundStringMatchIndex < 0)
                m_foundStringMatchIndex += matchCount;
            if (m_foundStringMatchIndex >= (int) matchCount)
                m_foundStringMatchIndex -= matchCount;
        }

        m_findMatches.clear();
        Vector<IntRect> matchRects;
        if (auto range = m_webPage->corePage()->selection().firstRange()) {
            range->absoluteTextRects(matchRects);
            m_findMatches.append(range);
        }

        m_webPage->send(Messages::WebPageProxy::DidFindString(string, matchRects, matchCount, m_foundStringMatchIndex));

        if (!(options & FindOptionsShowFindIndicator) || !selectedFrame || !updateFindIndicator(*selectedFrame, shouldShowOverlay))
            hideFindIndicator();
    }

    if (!shouldShowOverlay) {
        if (m_findPageOverlay)
            m_webPage->mainFrame()->pageOverlayController().uninstallPageOverlay(m_findPageOverlay, PageOverlay::FadeMode::Fade);
    } else {
        if (!m_findPageOverlay) {
            auto findPageOverlay = PageOverlay::create(*this, PageOverlay::OverlayType::Document);
            m_findPageOverlay = findPageOverlay.ptr();
            m_webPage->mainFrame()->pageOverlayController().installPageOverlay(WTFMove(findPageOverlay), PageOverlay::FadeMode::Fade);
        }
        m_findPageOverlay->setNeedsDisplay();
    }
}

void FindController::findString(const String& string, FindOptions options, unsigned maxMatchCount)
{
    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());

    WebCore::FindOptions coreOptions = core(options);

    // iOS will reveal the selection through a different mechanism, and
    // we need to avoid sending the non-painted selection change to the UI process
    // so that it does not clear the selection out from under us.
#if PLATFORM(IOS)
    coreOptions = static_cast<FindOptions>(coreOptions | DoNotRevealSelection);
#endif

    willFindString();

    bool foundStringStartsAfterSelection = false;
    if (!pluginView) {
        if (Frame* selectedFrame = frameWithSelection(m_webPage->corePage())) {
            FrameSelection& fs = selectedFrame->selection();
            if (fs.selectionBounds().isEmpty()) {
                m_findMatches.clear();
                int indexForSelection;
                m_webPage->corePage()->findStringMatchingRanges(string, coreOptions, maxMatchCount, m_findMatches, indexForSelection);
                m_foundStringMatchIndex = indexForSelection;
                foundStringStartsAfterSelection = true;
            }
        }
    }

    m_findMatches.clear();

    bool found;
    if (pluginView)
        found = pluginView->findString(string, coreOptions, maxMatchCount);
    else
        found = m_webPage->corePage()->findString(string, coreOptions);

    if (found) {
        didFindString();

        if (!foundStringStartsAfterSelection) {
            if (options & FindOptionsBackwards)
                m_foundStringMatchIndex--;
            else
                m_foundStringMatchIndex++;
        }
    }

    RefPtr<WebPage> protectedWebPage = m_webPage;
    m_webPage->drawingArea()->dispatchAfterEnsuringUpdatedScrollPosition([protectedWebPage, found, string, options, maxMatchCount] () {
        protectedWebPage->findController().updateFindUIAfterPageScroll(found, string, options, maxMatchCount);
    });
}

void FindController::findStringMatches(const String& string, FindOptions options, unsigned maxMatchCount)
{
    m_findMatches.clear();
    int indexForSelection;

    m_webPage->corePage()->findStringMatchingRanges(string, core(options), maxMatchCount, m_findMatches, indexForSelection);

    Vector<Vector<IntRect>> matchRects;
    for (size_t i = 0; i < m_findMatches.size(); ++i) {
        Vector<IntRect> rects;
        m_findMatches[i]->absoluteTextRects(rects);
        matchRects.append(WTFMove(rects));
    }

    m_webPage->send(Messages::WebPageProxy::DidFindStringMatches(string, matchRects, indexForSelection));
}

void FindController::getImageForFindMatch(uint32_t matchIndex)
{
    if (matchIndex >= m_findMatches.size())
        return;
    Frame* frame = m_findMatches[matchIndex]->startContainer().document().frame();
    if (!frame)
        return;

    VisibleSelection oldSelection = frame->selection().selection();
    frame->selection().setSelection(VisibleSelection(*m_findMatches[matchIndex]));

    RefPtr<ShareableBitmap> selectionSnapshot = WebFrame::fromCoreFrame(*frame)->createSelectionSnapshot();

    frame->selection().setSelection(oldSelection);

    if (!selectionSnapshot)
        return;

    ShareableBitmap::Handle handle;
    selectionSnapshot->createHandle(handle);

    if (handle.isNull())
        return;

    m_webPage->send(Messages::WebPageProxy::DidGetImageForFindMatch(handle, matchIndex));
}

void FindController::selectFindMatch(uint32_t matchIndex)
{
    if (matchIndex >= m_findMatches.size())
        return;
    Frame* frame = m_findMatches[matchIndex]->startContainer().document().frame();
    if (!frame)
        return;
    frame->selection().setSelection(VisibleSelection(*m_findMatches[matchIndex]));
}

void FindController::hideFindUI()
{
    m_findMatches.clear();
    if (m_findPageOverlay)
        m_webPage->mainFrame()->pageOverlayController().uninstallPageOverlay(m_findPageOverlay, PageOverlay::FadeMode::Fade);

    PluginView* pluginView = pluginViewForFrame(m_webPage->mainFrame());
    
    if (pluginView)
        pluginView->findString(emptyString(), 0, 0);
    else
        m_webPage->corePage()->unmarkAllTextMatches();
    
    hideFindIndicator();
}

#if !PLATFORM(IOS)
bool FindController::updateFindIndicator(Frame& selectedFrame, bool isShowingOverlay, bool shouldAnimate)
{
    RefPtr<TextIndicator> indicator = TextIndicator::createWithSelectionInFrame(selectedFrame, TextIndicatorOptionIncludeMarginIfRangeMatchesSelection, shouldAnimate ? TextIndicatorPresentationTransition::Bounce : TextIndicatorPresentationTransition::None);
    if (!indicator)
        return false;

    m_findIndicatorRect = enclosingIntRect(indicator->selectionRectInRootViewCoordinates());
#if PLATFORM(COCOA)
    m_webPage->send(Messages::WebPageProxy::SetTextIndicator(indicator->data(), static_cast<uint64_t>(isShowingOverlay ? TextIndicatorWindowLifetime::Permanent : TextIndicatorWindowLifetime::Temporary)));
#endif
    m_isShowingFindIndicator = true;

    return true;
}

void FindController::hideFindIndicator()
{
    if (!m_isShowingFindIndicator)
        return;

    m_webPage->send(Messages::WebPageProxy::ClearTextIndicator());
    m_isShowingFindIndicator = false;
    m_foundStringMatchIndex = -1;
    didHideFindIndicator();
}

void FindController::willFindString()
{
}

void FindController::didFindString()
{
}

void FindController::didFailToFindString()
{
}

void FindController::didHideFindIndicator()
{
}
#endif

void FindController::showFindIndicatorInSelection()
{
    Frame& selectedFrame = m_webPage->corePage()->focusController().focusedOrMainFrame();
    updateFindIndicator(selectedFrame, false);
}

void FindController::deviceScaleFactorDidChange()
{
    ASSERT(isShowingOverlay());

    Frame* selectedFrame = frameWithSelection(m_webPage->corePage());
    if (!selectedFrame)
        return;

    updateFindIndicator(*selectedFrame, true, false);
}

void FindController::redraw()
{
    if (!m_isShowingFindIndicator)
        return;

    Frame* selectedFrame = frameWithSelection(m_webPage->corePage());
    if (!selectedFrame)
        return;

    updateFindIndicator(*selectedFrame, isShowingOverlay(), false);
}

Vector<IntRect> FindController::rectsForTextMatchesInRect(IntRect clipRect)
{
    Vector<IntRect> rects;

    FrameView* mainFrameView = m_webPage->corePage()->mainFrame().view();

    for (Frame* frame = &m_webPage->corePage()->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        Document* document = frame->document();
        if (!document)
            continue;

        for (FloatRect rect : document->markers().renderedRectsForMarkers(DocumentMarker::TextMatch)) {
            if (!frame->isMainFrame())
                rect = mainFrameView->windowToContents(frame->view()->contentsToWindow(enclosingIntRect(rect)));
            rect.intersect(clipRect);

            if (rect.isEmpty())
                continue;

            rects.append(rect);
        }
    }

    return rects;
}

void FindController::willMoveToPage(PageOverlay&, Page* page)
{
    if (page)
        return;

    ASSERT(m_findPageOverlay);
    m_findPageOverlay = 0;
}
    
void FindController::didMoveToPage(PageOverlay&, Page*)
{
}

const float shadowOffsetX = 0;
const float shadowOffsetY = 0;
const float shadowBlurRadius = 1;
const float shadowColorAlpha = 0.5;

void FindController::drawRect(PageOverlay&, GraphicsContext& graphicsContext, const IntRect& dirtyRect)
{
    const int borderWidth = 1;

    Color overlayBackgroundColor(0.1f, 0.1f, 0.1f, 0.25f);

    IntRect borderInflatedDirtyRect = dirtyRect;
    borderInflatedDirtyRect.inflate(borderWidth);
    Vector<IntRect> rects = rectsForTextMatchesInRect(borderInflatedDirtyRect);

    // Draw the background.
    graphicsContext.fillRect(dirtyRect, overlayBackgroundColor);

    {
        GraphicsContextStateSaver stateSaver(graphicsContext);

        graphicsContext.setShadow(FloatSize(shadowOffsetX, shadowOffsetY), shadowBlurRadius, Color(0.0f, 0.0f, 0.0f, shadowColorAlpha));
        graphicsContext.setFillColor(Color::white);

        // Draw white frames around the holes.
        for (auto& rect : rects) {
            IntRect whiteFrameRect = rect;
            whiteFrameRect.inflate(borderWidth);
            graphicsContext.fillRect(whiteFrameRect);
        }
    }

    // Clear out the holes.
    for (auto& rect : rects)
        graphicsContext.clearRect(rect);

    if (!m_isShowingFindIndicator)
        return;

    if (Frame* selectedFrame = frameWithSelection(m_webPage->corePage())) {
        IntRect findIndicatorRect = selectedFrame->view()->contentsToRootView(enclosingIntRect(selectedFrame->selection().selectionBounds()));

        if (findIndicatorRect != m_findIndicatorRect)
            hideFindIndicator();
    }
}

bool FindController::mouseEvent(PageOverlay&, const PlatformMouseEvent& mouseEvent)
{
    if (mouseEvent.type() == PlatformEvent::MousePressed)
        hideFindUI();

    return false;
}

void FindController::didInvalidateDocumentMarkerRects()
{
    if (m_findPageOverlay)
        m_findPageOverlay->setNeedsDisplay();
}

} // namespace WebKit
