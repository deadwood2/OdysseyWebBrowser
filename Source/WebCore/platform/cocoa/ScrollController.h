/*
 * Copyright (C) 2011, 2014-2015 Apple Inc. All rights reserved.
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

#ifndef ScrollController_h
#define ScrollController_h

#if ENABLE(RUBBER_BANDING)

#include "FloatPoint.h"
#include "FloatSize.h"
#include "ScrollTypes.h"
#include <wtf/Noncopyable.h>
#include <wtf/RunLoop.h>

#if ENABLE(CSS_SCROLL_SNAP)
#include "ScrollSnapAnimatorState.h"
#endif

namespace WebCore {

class PlatformWheelEvent;
class ScrollableArea;
class WheelEventTestTrigger;

class ScrollControllerClient {
protected:
    virtual ~ScrollControllerClient() { }

public:
    virtual bool allowsHorizontalStretching(const PlatformWheelEvent&) = 0;
    virtual bool allowsVerticalStretching(const PlatformWheelEvent&) = 0;
    virtual IntSize stretchAmount() = 0;
    virtual bool pinnedInDirection(const FloatSize&) = 0;
    virtual bool canScrollHorizontally() = 0;
    virtual bool canScrollVertically() = 0;
    virtual bool shouldRubberBandInDirection(ScrollDirection) = 0;

    // Return the absolute scroll position, not relative to the scroll origin.
    virtual WebCore::IntPoint absoluteScrollPosition() = 0;

    virtual void immediateScrollBy(const FloatSize&) = 0;
    virtual void immediateScrollByWithoutContentEdgeConstraints(const FloatSize&) = 0;
    virtual void startSnapRubberbandTimer()
    {
        // Override to perform client-specific snap start logic
    }

    virtual void stopSnapRubberbandTimer()
    {
        // Override to perform client-specific snap end logic
    }
    
    // If the current scroll position is within the overhang area, this function will cause
    // the page to scroll to the nearest boundary point.
    virtual void adjustScrollPositionToBoundsIfNecessary() = 0;

    virtual WheelEventTestTrigger* testTrigger() const { return nullptr; }

#if ENABLE(CSS_SCROLL_SNAP) && PLATFORM(MAC)
    virtual LayoutUnit scrollOffsetOnAxis(ScrollEventAxis) const = 0;
    virtual void immediateScrollOnAxis(ScrollEventAxis, float delta) = 0;
    virtual void startScrollSnapTimer(ScrollEventAxis)
    {
        // Override to perform client-specific scroll snap point start logic
    }

    virtual void stopScrollSnapTimer(ScrollEventAxis)
    {
        // Override to perform client-specific scroll snap point end logic
    }

    virtual float pageScaleFactor() const
    {
        return 1.0f;
    }
#endif
};

class ScrollController {
    WTF_MAKE_NONCOPYABLE(ScrollController);

public:
    explicit ScrollController(ScrollControllerClient&);

    bool handleWheelEvent(const PlatformWheelEvent&);

    bool isRubberBandInProgress() const;

#if ENABLE(CSS_SCROLL_SNAP) && PLATFORM(MAC)
    bool processWheelEventForScrollSnap(const PlatformWheelEvent&);
    void updateScrollAnimatorsAndTimers(const ScrollableArea&);
    void updateScrollSnapPoints(ScrollEventAxis, const Vector<LayoutUnit>&);
#endif

private:
    void startSnapRubberbandTimer();
    void stopSnapRubberbandTimer();
    void snapRubberBand();
    void snapRubberBandTimerFired();

    bool shouldRubberBandInHorizontalDirection(const PlatformWheelEvent&);

#if ENABLE(CSS_SCROLL_SNAP) && PLATFORM(MAC)
    void horizontalScrollSnapTimerFired();
    void verticalScrollSnapTimerFired();
    void startScrollSnapTimer(ScrollEventAxis);
    void stopScrollSnapTimer(ScrollEventAxis);

    LayoutUnit scrollOffsetOnAxis(ScrollEventAxis) const;
    void processWheelEventForScrollSnapOnAxis(ScrollEventAxis, const PlatformWheelEvent&);
    bool shouldOverrideWheelEvent(ScrollEventAxis, const PlatformWheelEvent&) const;

    void beginScrollSnapAnimation(ScrollEventAxis, ScrollSnapState);
    void scrollSnapAnimationUpdate(ScrollEventAxis);
    void endScrollSnapAnimation(ScrollEventAxis, ScrollSnapState);

    void initializeGlideParameters(ScrollEventAxis, bool);
    float computeSnapDelta(ScrollEventAxis) const;
    float computeGlideDelta(ScrollEventAxis) const;

    ScrollSnapAnimatorState& scrollSnapPointState(ScrollEventAxis);
    const ScrollSnapAnimatorState& scrollSnapPointState(ScrollEventAxis) const;
#endif

    ScrollControllerClient& m_client;
    
    CFTimeInterval m_lastMomentumScrollTimestamp;
    FloatSize m_overflowScrollDelta;
    FloatSize m_stretchScrollForce;
    FloatSize m_momentumVelocity;

    // Rubber band state.
    CFTimeInterval m_startTime;
    FloatSize m_startStretch;
    FloatPoint m_origOrigin;
    FloatSize m_origVelocity;
    RunLoop::Timer<ScrollController> m_snapRubberbandTimer;

#if ENABLE(CSS_SCROLL_SNAP) && PLATFORM(MAC)
    // FIXME: Find a way to consolidate both timers into one variable.
    std::unique_ptr<ScrollSnapAnimatorState> m_horizontalScrollSnapState;
    std::unique_ptr<ScrollSnapAnimatorState> m_verticalScrollSnapState;
    RunLoop::Timer<ScrollController> m_horizontalScrollSnapTimer;
    RunLoop::Timer<ScrollController> m_verticalScrollSnapTimer;
#endif

    bool m_inScrollGesture;
    bool m_momentumScrollInProgress;
    bool m_ignoreMomentumScrolls;
    bool m_snapRubberbandTimerIsActive;
};

} // namespace WebCore

#endif // ENABLE(RUBBER_BANDING)

#endif // ScrollController_h
