/*
 * Copyright (c) 2016 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScrollAnimatorGtk.h"

#include "ScrollAnimationKinetic.h"
#include "ScrollAnimationSmooth.h"
#include "ScrollableArea.h"
#include "ScrollbarTheme.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

static const Seconds overflowScrollbarsAnimationDuration { 1_s };
static const Seconds overflowScrollbarsAnimationHideDelay { 2_s };
static const Seconds scrollCaptureThreshold { 150_ms };

std::unique_ptr<ScrollAnimator> ScrollAnimator::create(ScrollableArea& scrollableArea)
{
    return std::make_unique<ScrollAnimatorGtk>(scrollableArea);
}

ScrollAnimatorGtk::ScrollAnimatorGtk(ScrollableArea& scrollableArea)
    : ScrollAnimator(scrollableArea)
    , m_overlayScrollbarAnimationTimer(*this, &ScrollAnimatorGtk::overlayScrollbarAnimationTimerFired)
{
    m_kineticAnimation = std::make_unique<ScrollAnimationKinetic>(m_scrollableArea, [this](FloatPoint&& position) {
#if ENABLE(SMOOTH_SCROLLING)
        if (m_smoothAnimation)
            m_smoothAnimation->setCurrentPosition(position);
#endif
        updatePosition(WTFMove(position));
    });

#if ENABLE(SMOOTH_SCROLLING)
    if (scrollableArea.scrollAnimatorEnabled())
        ensureSmoothScrollingAnimation();
#endif
}

ScrollAnimatorGtk::~ScrollAnimatorGtk()
{
}

#if ENABLE(SMOOTH_SCROLLING)
void ScrollAnimatorGtk::ensureSmoothScrollingAnimation()
{
    if (m_smoothAnimation)
        return;

    m_smoothAnimation = std::make_unique<ScrollAnimationSmooth>(m_scrollableArea, m_currentPosition, [this](FloatPoint&& position) {
        updatePosition(WTFMove(position));
    });
}
#endif

#if ENABLE(SMOOTH_SCROLLING)
bool ScrollAnimatorGtk::scroll(ScrollbarOrientation orientation, ScrollGranularity granularity, float step, float multiplier)
{
    if (!m_scrollableArea.scrollAnimatorEnabled() || granularity == ScrollByPrecisePixel)
        return ScrollAnimator::scroll(orientation, granularity, step, multiplier);

    ensureSmoothScrollingAnimation();
    return m_smoothAnimation->scroll(orientation, granularity, step, multiplier);
}
#endif

void ScrollAnimatorGtk::scrollToOffsetWithoutAnimation(const FloatPoint& offset)
{
    FloatPoint position = ScrollableArea::scrollPositionFromOffset(offset, toFloatSize(m_scrollableArea.scrollOrigin()));
    m_kineticAnimation->stop();
    m_scrollHistory.clear();

#if ENABLE(SMOOTH_SCROLLING)
    if (m_smoothAnimation)
        m_smoothAnimation->setCurrentPosition(position);
#endif

    updatePosition(WTFMove(position));
}

FloatPoint ScrollAnimatorGtk::computeVelocity()
{
    if (m_scrollHistory.isEmpty())
        return { };

    double first = m_scrollHistory[0].timestamp();
    double last = m_scrollHistory.rbegin()->timestamp();

    if (last == first)
        return { };

    FloatPoint accumDelta;
    for (const auto& scrollEvent : m_scrollHistory)
        accumDelta += FloatPoint(scrollEvent.deltaX(), scrollEvent.deltaY());

    m_scrollHistory.clear();

    return FloatPoint(accumDelta.x() * -1000 / (last - first), accumDelta.y() * -1000 / (last - first));
}

bool ScrollAnimatorGtk::handleWheelEvent(const PlatformWheelEvent& event)
{
    m_kineticAnimation->stop();

    m_scrollHistory.removeAllMatching([&event] (PlatformWheelEvent& otherEvent) -> bool {
        return Seconds::fromMilliseconds(event.timestamp() - otherEvent.timestamp()) > scrollCaptureThreshold;
    });

    if (event.isEndOfNonMomentumScroll()) {
        // We don't need to add the event to the history as its delta will be (0, 0).
        static_cast<ScrollAnimationKinetic*>(m_kineticAnimation.get())->start(m_currentPosition, computeVelocity(), m_scrollableArea.horizontalScrollbar(), m_scrollableArea.verticalScrollbar());
        return true;
    }
    if (event.isTransitioningToMomentumScroll()) {
        m_scrollHistory.clear();
        static_cast<ScrollAnimationKinetic*>(m_kineticAnimation.get())->start(m_currentPosition, event.swipeVelocity(), m_scrollableArea.horizontalScrollbar(), m_scrollableArea.verticalScrollbar());
        return true;
    }

    m_scrollHistory.append(event);

    return ScrollAnimator::handleWheelEvent(event);
}

void ScrollAnimatorGtk::willEndLiveResize()
{
    m_kineticAnimation->updateVisibleLengths();

#if ENABLE(SMOOTH_SCROLLING)
    if (m_smoothAnimation)
        m_smoothAnimation->updateVisibleLengths();
#endif
}

void ScrollAnimatorGtk::updatePosition(FloatPoint&& position)
{
    FloatSize delta = position - m_currentPosition;
    m_currentPosition = WTFMove(position);
    notifyPositionChanged(delta);
}

void ScrollAnimatorGtk::didAddVerticalScrollbar(Scrollbar* scrollbar)
{
    m_kineticAnimation->updateVisibleLengths();

#if ENABLE(SMOOTH_SCROLLING)
    if (m_smoothAnimation)
        m_smoothAnimation->updateVisibleLengths();
#endif
    if (!scrollbar->isOverlayScrollbar())
        return;
    m_verticalOverlayScrollbar = scrollbar;
    if (!m_horizontalOverlayScrollbar)
        m_overlayScrollbarAnimationCurrent = 1;
    m_verticalOverlayScrollbar->setOpacity(m_overlayScrollbarAnimationCurrent);
    hideOverlayScrollbars();
}

void ScrollAnimatorGtk::didAddHorizontalScrollbar(Scrollbar* scrollbar)
{
    m_kineticAnimation->updateVisibleLengths();

#if ENABLE(SMOOTH_SCROLLING)
    if (m_smoothAnimation)
        m_smoothAnimation->updateVisibleLengths();
#endif
    if (!scrollbar->isOverlayScrollbar())
        return;
    m_horizontalOverlayScrollbar = scrollbar;
    if (!m_verticalOverlayScrollbar)
        m_overlayScrollbarAnimationCurrent = 1;
    m_horizontalOverlayScrollbar->setOpacity(m_overlayScrollbarAnimationCurrent);
    hideOverlayScrollbars();
}

void ScrollAnimatorGtk::willRemoveVerticalScrollbar(Scrollbar* scrollbar)
{
    if (m_verticalOverlayScrollbar != scrollbar)
        return;
    m_verticalOverlayScrollbar = nullptr;
    if (!m_horizontalOverlayScrollbar)
        m_overlayScrollbarAnimationCurrent = 0;
}

void ScrollAnimatorGtk::willRemoveHorizontalScrollbar(Scrollbar* scrollbar)
{
    if (m_horizontalOverlayScrollbar != scrollbar)
        return;
    m_horizontalOverlayScrollbar = nullptr;
    if (!m_verticalOverlayScrollbar)
        m_overlayScrollbarAnimationCurrent = 0;
}

void ScrollAnimatorGtk::updateOverlayScrollbarsOpacity()
{
    if (m_verticalOverlayScrollbar && m_overlayScrollbarAnimationCurrent != m_verticalOverlayScrollbar->opacity()) {
        m_verticalOverlayScrollbar->setOpacity(m_overlayScrollbarAnimationCurrent);
        if (m_verticalOverlayScrollbar->hoveredPart() == NoPart)
            m_verticalOverlayScrollbar->invalidate();
    }

    if (m_horizontalOverlayScrollbar && m_overlayScrollbarAnimationCurrent != m_horizontalOverlayScrollbar->opacity()) {
        m_horizontalOverlayScrollbar->setOpacity(m_overlayScrollbarAnimationCurrent);
        if (m_horizontalOverlayScrollbar->hoveredPart() == NoPart)
            m_horizontalOverlayScrollbar->invalidate();
    }
}

static inline double easeOutCubic(double t)
{
    double p = t - 1;
    return p * p * p + 1;
}

void ScrollAnimatorGtk::overlayScrollbarAnimationTimerFired()
{
    if (!m_horizontalOverlayScrollbar && !m_verticalOverlayScrollbar)
        return;
    if (m_overlayScrollbarsLocked)
        return;

    MonotonicTime currentTime = MonotonicTime::now();
    double progress = 1;
    if (currentTime < m_overlayScrollbarAnimationEndTime)
        progress = (currentTime - m_overlayScrollbarAnimationStartTime).value() / (m_overlayScrollbarAnimationEndTime - m_overlayScrollbarAnimationStartTime).value();
    progress = m_overlayScrollbarAnimationSource + (easeOutCubic(progress) * (m_overlayScrollbarAnimationTarget - m_overlayScrollbarAnimationSource));
    if (progress != m_overlayScrollbarAnimationCurrent) {
        m_overlayScrollbarAnimationCurrent = progress;
        updateOverlayScrollbarsOpacity();
    }

    if (m_overlayScrollbarAnimationCurrent != m_overlayScrollbarAnimationTarget) {
        static const double frameRate = 60;
        static const Seconds tickTime = 1_s / frameRate;
        static const Seconds minimumTimerInterval = 1_ms;
        Seconds deltaToNextFrame = std::max(tickTime - (MonotonicTime::now() - currentTime), minimumTimerInterval);
        m_overlayScrollbarAnimationTimer.startOneShot(deltaToNextFrame);
    } else
        hideOverlayScrollbars();
}

void ScrollAnimatorGtk::showOverlayScrollbars()
{
    if (m_overlayScrollbarsLocked)
        return;

    if (m_overlayScrollbarAnimationTimer.isActive() && m_overlayScrollbarAnimationTarget == 1)
        return;
    m_overlayScrollbarAnimationTimer.stop();

    if (!m_horizontalOverlayScrollbar && !m_verticalOverlayScrollbar)
        return;

    m_overlayScrollbarAnimationSource = m_overlayScrollbarAnimationCurrent;
    m_overlayScrollbarAnimationTarget = 1;
    if (m_overlayScrollbarAnimationTarget != m_overlayScrollbarAnimationCurrent) {
        m_overlayScrollbarAnimationStartTime = MonotonicTime::now();
        m_overlayScrollbarAnimationEndTime = m_overlayScrollbarAnimationStartTime + overflowScrollbarsAnimationDuration;
        m_overlayScrollbarAnimationTimer.startOneShot(0_s);
    } else
        hideOverlayScrollbars();
}

void ScrollAnimatorGtk::hideOverlayScrollbars()
{
    if (m_overlayScrollbarAnimationTimer.isActive() && !m_overlayScrollbarAnimationTarget)
        return;
    m_overlayScrollbarAnimationTimer.stop();

    if (!m_horizontalOverlayScrollbar && !m_verticalOverlayScrollbar)
        return;

    m_overlayScrollbarAnimationSource = m_overlayScrollbarAnimationCurrent;
    m_overlayScrollbarAnimationTarget = 0;
    if (m_overlayScrollbarAnimationTarget == m_overlayScrollbarAnimationCurrent)
        return;
    m_overlayScrollbarAnimationStartTime = MonotonicTime::now() + overflowScrollbarsAnimationHideDelay;
    m_overlayScrollbarAnimationEndTime = m_overlayScrollbarAnimationStartTime + overflowScrollbarsAnimationDuration + overflowScrollbarsAnimationHideDelay;
    m_overlayScrollbarAnimationTimer.startOneShot(overflowScrollbarsAnimationHideDelay);
}

void ScrollAnimatorGtk::mouseEnteredContentArea()
{
    showOverlayScrollbars();
}

void ScrollAnimatorGtk::mouseExitedContentArea()
{
    hideOverlayScrollbars();
}

void ScrollAnimatorGtk::mouseMovedInContentArea()
{
    showOverlayScrollbars();
}

void ScrollAnimatorGtk::contentAreaDidShow()
{
    showOverlayScrollbars();
}

void ScrollAnimatorGtk::contentAreaDidHide()
{
    if (m_overlayScrollbarsLocked)
        return;
    m_overlayScrollbarAnimationTimer.stop();
    if (m_overlayScrollbarAnimationCurrent) {
        m_overlayScrollbarAnimationCurrent = 0;
        updateOverlayScrollbarsOpacity();
    }
}

void ScrollAnimatorGtk::notifyContentAreaScrolled(const FloatSize&)
{
    showOverlayScrollbars();
}

void ScrollAnimatorGtk::lockOverlayScrollbarStateToHidden(bool shouldLockState)
{
    if (m_overlayScrollbarsLocked == shouldLockState)
        return;
    m_overlayScrollbarsLocked = shouldLockState;

    if (!m_horizontalOverlayScrollbar && !m_verticalOverlayScrollbar)
        return;

    if (m_overlayScrollbarsLocked) {
        m_overlayScrollbarAnimationTimer.stop();
        if (m_horizontalOverlayScrollbar)
            m_horizontalOverlayScrollbar->setOpacity(0);
        if (m_verticalOverlayScrollbar)
            m_verticalOverlayScrollbar->setOpacity(0);
    } else {
        if (m_overlayScrollbarAnimationCurrent == 1)
            updateOverlayScrollbarsOpacity();
        else
            showOverlayScrollbars();
    }
}

} // namespace WebCore

