/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef TiledCoreAnimationDrawingArea_h
#define TiledCoreAnimationDrawingArea_h

#if !PLATFORM(IOS)

#include "DrawingArea.h"
#include "LayerTreeContext.h"
#include <WebCore/FloatRect.h>
#include <WebCore/LayerFlushScheduler.h>
#include <WebCore/LayerFlushSchedulerClient.h>
#include <WebCore/TransformationMatrix.h>
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
#include <wtf/RunLoop.h>

OBJC_CLASS CALayer;

namespace WebCore {
class FrameView;
class PlatformCALayer;
class TiledBacking;
}

namespace WebKit {

class LayerHostingContext;

class TiledCoreAnimationDrawingArea : public DrawingArea, WebCore::LayerFlushSchedulerClient {
public:
    TiledCoreAnimationDrawingArea(WebPage&, const WebPageCreationParameters&);
    virtual ~TiledCoreAnimationDrawingArea();

private:
    // DrawingArea
    void setNeedsDisplay() override;
    void setNeedsDisplayInRect(const WebCore::IntRect&) override;
    void scroll(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollDelta) override;

    void forceRepaint() override;
    bool forceRepaintAsync(CallbackID) override;
    void setLayerTreeStateIsFrozen(bool) override;
    bool layerTreeStateIsFrozen() const override;
    void setRootCompositingLayer(WebCore::GraphicsLayer*) override;
    void scheduleCompositingLayerFlush() override;
    void scheduleCompositingLayerFlushImmediately() override;

    void updatePreferences(const WebPreferencesStore&) override;
    void mainFrameContentSizeChanged(const WebCore::IntSize&) override;

    void setViewExposedRect(std::optional<WebCore::FloatRect>) override;
    std::optional<WebCore::FloatRect> viewExposedRect() const override { return m_scrolledViewExposedRect; }

    bool supportsAsyncScrolling() override { return true; }

    void dispatchAfterEnsuringUpdatedScrollPosition(WTF::Function<void ()>&&) override;

    bool shouldUseTiledBackingForFrameView(const WebCore::FrameView&) override;

    void activityStateDidChange(WebCore::ActivityState::Flags changed, bool wantsDidUpdateActivityState, const Vector<CallbackID>&) override;
    void didUpdateActivityStateTimerFired();

    void attachViewOverlayGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*) override;

    bool dispatchDidReachLayoutMilestone(WebCore::LayoutMilestones) override;

    // WebCore::LayerFlushSchedulerClient
    bool flushLayers() override;

    // Message handlers.
    void updateGeometry(const WebCore::IntSize& viewSize, const WebCore::IntSize& layerPosition, bool flushSynchronously, const WebCore::MachSendRight& fencePort) override;
    void setDeviceScaleFactor(float) override;
    void suspendPainting();
    void resumePainting();
    void setLayerHostingMode(LayerHostingMode) override;
    void setColorSpace(const ColorSpaceData&) override;
    void addFence(const WebCore::MachSendRight&) override;

    void setShouldScaleViewToFitDocument(bool) override;

    void adjustTransientZoom(double scale, WebCore::FloatPoint origin) override;
    void commitTransientZoom(double scale, WebCore::FloatPoint origin) override;
    void applyTransientZoomToPage(double scale, WebCore::FloatPoint origin);
    WebCore::PlatformCALayer* layerForTransientZoom() const;
    WebCore::PlatformCALayer* shadowLayerForTransientZoom() const;

    void applyTransientZoomToLayers(double scale, WebCore::FloatPoint origin);

    void updateLayerHostingContext();

    void setRootCompositingLayer(CALayer *);
    void updateRootLayers();

    WebCore::TiledBacking* mainFrameTiledBacking() const;
    void updateDebugInfoLayer(bool showLayer);

    void updateIntrinsicContentSizeIfNeeded();
    void updateScrolledExposedRect();
    void scaleViewToFitDocumentIfNeeded();

    void sendPendingNewlyReachedLayoutMilestones();

    bool m_layerTreeStateIsFrozen;
    WebCore::LayerFlushScheduler m_layerFlushScheduler;

    std::unique_ptr<LayerHostingContext> m_layerHostingContext;

    RetainPtr<CALayer> m_hostingLayer;
    RetainPtr<CALayer> m_rootLayer;
    RetainPtr<CALayer> m_debugInfoLayer;

    RetainPtr<CALayer> m_pendingRootLayer;

    bool m_isPaintingSuspended;

    std::optional<WebCore::FloatRect> m_viewExposedRect;
    std::optional<WebCore::FloatRect> m_scrolledViewExposedRect;

    WebCore::IntSize m_lastSentIntrinsicContentSize;
    bool m_inUpdateGeometry;

    double m_transientZoomScale;
    WebCore::FloatPoint m_transientZoomOrigin;

    WebCore::TransformationMatrix m_transform;

    RunLoop::Timer<TiledCoreAnimationDrawingArea> m_sendDidUpdateActivityStateTimer;
    Vector<CallbackID> m_nextActivityStateChangeCallbackIDs;
    bool m_wantsDidUpdateActivityState;

    WebCore::GraphicsLayer* m_viewOverlayRootLayer;

    bool m_shouldScaleViewToFitDocument { false };
    bool m_isScalingViewToFitDocument { false };
    WebCore::IntSize m_lastViewSizeForScaleToFit;
    WebCore::IntSize m_lastDocumentSizeForScaleToFit;

    WebCore::LayoutMilestones m_pendingNewlyReachedLayoutMilestones { 0 };
};

} // namespace WebKit

SPECIALIZE_TYPE_TRAITS_DRAWING_AREA(TiledCoreAnimationDrawingArea, DrawingAreaTypeTiledCoreAnimation)

#endif // !PLATFORM(IOS)

#endif // TiledCoreAnimationDrawingArea_h
