/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#include "RenderLayerCompositor.h"

#include "CSSAnimationController.h"
#include "CSSPropertyNames.h"
#include "CanvasRenderingContext.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "DocumentTimeline.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsLayer.h"
#include "HTMLCanvasElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "InspectorInstrumentation.h"
#include "Logging.h"
#include "NodeList.h"
#include "Page.h"
#include "PageOverlayController.h"
#include "RenderEmbeddedObject.h"
#include "RenderFragmentedFlow.h"
#include "RenderFullScreen.h"
#include "RenderGeometryMap.h"
#include "RenderIFrame.h"
#include "RenderLayerBacking.h"
#include "RenderReplica.h"
#include "RenderVideo.h"
#include "RenderView.h"
#include "RuntimeEnabledFeatures.h"
#include "ScrollingConstraints.h"
#include "ScrollingCoordinator.h"
#include "Settings.h"
#include "TiledBacking.h"
#include "TransformState.h"
#include <wtf/HexNumber.h>
#include <wtf/MemoryPressureHandler.h>
#include <wtf/SetForScope.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenateNumbers.h>
#include <wtf/text/TextStream.h>

#if PLATFORM(IOS_FAMILY)
#include "LegacyTileCache.h"
#include "RenderScrollbar.h"
#endif

#if PLATFORM(MAC)
#include "LocalDefaultSystemAppearance.h"
#endif

#if ENABLE(TREE_DEBUGGING)
#include "RenderTreeAsText.h"
#endif

#if ENABLE(3D_TRANSFORMS)
// This symbol is used to determine from a script whether 3D rendering is enabled (via 'nm').
WEBCORE_EXPORT bool WebCoreHas3DRendering = true;
#endif

#if !PLATFORM(MAC) && !PLATFORM(IOS_FAMILY)
#define USE_COMPOSITING_FOR_SMALL_CANVASES 1
#endif

namespace WebCore {

#if !USE(COMPOSITING_FOR_SMALL_CANVASES)
static const int canvasAreaThresholdRequiringCompositing = 50 * 100;
#endif
// During page loading delay layer flushes up to this many seconds to allow them coalesce, reducing workload.
#if PLATFORM(IOS_FAMILY)
static const Seconds throttledLayerFlushInitialDelay { 500_ms };
static const Seconds throttledLayerFlushDelay { 1.5_s };
#else
static const Seconds throttledLayerFlushInitialDelay { 500_ms };
static const Seconds throttledLayerFlushDelay { 500_ms };
#endif

using namespace HTMLNames;

struct ScrollingTreeState {
    Optional<ScrollingNodeID> parentNodeID;
    size_t nextChildIndex { 0 };
};

class OverlapMapContainer {
public:
    void add(const LayoutRect& bounds)
    {
        m_layerRects.append(bounds);
        m_boundingBox.unite(bounds);
    }

    bool overlapsLayers(const LayoutRect& bounds) const
    {
        // Checking with the bounding box will quickly reject cases when
        // layers are created for lists of items going in one direction and
        // never overlap with each other.
        if (!bounds.intersects(m_boundingBox))
            return false;
        for (const auto& layerRect : m_layerRects) {
            if (layerRect.intersects(bounds))
                return true;
        }
        return false;
    }

    void unite(const OverlapMapContainer& otherContainer)
    {
        m_layerRects.appendVector(otherContainer.m_layerRects);
        m_boundingBox.unite(otherContainer.m_boundingBox);
    }
private:
    Vector<LayoutRect> m_layerRects;
    LayoutRect m_boundingBox;
};

class RenderLayerCompositor::OverlapMap {
    WTF_MAKE_NONCOPYABLE(OverlapMap);
public:
    OverlapMap()
        : m_geometryMap(UseTransforms)
    {
        // Begin assuming the root layer will be composited so that there is
        // something on the stack. The root layer should also never get an
        // popCompositingContainer call.
        pushCompositingContainer();
    }

    void add(const LayoutRect& bounds)
    {
        // Layers do not contribute to overlap immediately--instead, they will
        // contribute to overlap as soon as their composited ancestor has been
        // recursively processed and popped off the stack.
        ASSERT(m_overlapStack.size() >= 2);
        m_overlapStack[m_overlapStack.size() - 2].add(bounds);
        m_isEmpty = false;
    }

    bool overlapsLayers(const LayoutRect& bounds) const
    {
        return m_overlapStack.last().overlapsLayers(bounds);
    }

    bool isEmpty() const
    {
        return m_isEmpty;
    }

    void pushCompositingContainer()
    {
        m_overlapStack.append(OverlapMapContainer());
    }

    void popCompositingContainer()
    {
        m_overlapStack[m_overlapStack.size() - 2].unite(m_overlapStack.last());
        m_overlapStack.removeLast();
    }

    const RenderGeometryMap& geometryMap() const { return m_geometryMap; }
    RenderGeometryMap& geometryMap() { return m_geometryMap; }

private:
    struct RectList {
        Vector<LayoutRect> rects;
        LayoutRect boundingRect;
        
        void append(const LayoutRect& rect)
        {
            rects.append(rect);
            boundingRect.unite(rect);
        }

        void append(const RectList& rectList)
        {
            rects.appendVector(rectList.rects);
            boundingRect.unite(rectList.boundingRect);
        }
        
        bool intersects(const LayoutRect& rect) const
        {
            if (!rects.size() || !boundingRect.intersects(rect))
                return false;

            for (const auto& currentRect : rects) {
                if (currentRect.intersects(rect))
                    return true;
            }
            return false;
        }
    };

    Vector<OverlapMapContainer> m_overlapStack;
    RenderGeometryMap m_geometryMap;
    bool m_isEmpty { true };
};

struct RenderLayerCompositor::CompositingState {
    CompositingState(RenderLayer* compAncestor, bool testOverlap = true)
        : compositingAncestor(compAncestor)
        , testingOverlap(testOverlap)
    {
    }
    
    CompositingState(const CompositingState& other)
        : compositingAncestor(other.compositingAncestor)
        , subtreeIsCompositing(other.subtreeIsCompositing)
        , testingOverlap(other.testingOverlap)
        , fullPaintOrderTraversalRequired(other.fullPaintOrderTraversalRequired)
        , descendantsRequireCompositingUpdate(other.descendantsRequireCompositingUpdate)
        , ancestorHasTransformAnimation(other.ancestorHasTransformAnimation)
#if ENABLE(CSS_COMPOSITING)
        , hasNotIsolatedCompositedBlendingDescendants(other.hasNotIsolatedCompositedBlendingDescendants)
#endif
#if ENABLE(TREE_DEBUGGING)
        , depth(other.depth + 1)
#endif
    {
    }
    
    RenderLayer* compositingAncestor;
    bool subtreeIsCompositing { false };
    bool testingOverlap { true };
    bool fullPaintOrderTraversalRequired { false };
    bool descendantsRequireCompositingUpdate { false };
    bool ancestorHasTransformAnimation { false };
#if ENABLE(CSS_COMPOSITING)
    bool hasNotIsolatedCompositedBlendingDescendants { false };
#endif
#if ENABLE(TREE_DEBUGGING)
    int depth { 0 };
#endif
};

struct RenderLayerCompositor::OverlapExtent {
    LayoutRect bounds;
    bool extentComputed { false };
    bool hasTransformAnimation { false };
    bool animationCausesExtentUncertainty { false };

    bool knownToBeHaveExtentUncertainty() const { return extentComputed && animationCausesExtentUncertainty; }
};

#if !LOG_DISABLED
static inline bool compositingLogEnabled()
{
    return LogCompositing.state == WTFLogChannelOn;
}
#endif

RenderLayerCompositor::RenderLayerCompositor(RenderView& renderView)
    : m_renderView(renderView)
    , m_updateCompositingLayersTimer(*this, &RenderLayerCompositor::updateCompositingLayersTimerFired)
    , m_layerFlushTimer(*this, &RenderLayerCompositor::layerFlushTimerFired)
{
#if PLATFORM(IOS_FAMILY)
    if (m_renderView.frameView().platformWidget())
        m_legacyScrollingLayerCoordinator = std::make_unique<LegacyWebKitScrollingLayerCoordinator>(page().chrome().client(), isMainFrameCompositor());
#endif
}

RenderLayerCompositor::~RenderLayerCompositor()
{
    // Take care that the owned GraphicsLayers are deleted first as their destructors may call back here.
    m_clipLayer = nullptr;
    m_scrolledContentsLayer = nullptr;
    ASSERT(m_rootLayerAttachment == RootLayerUnattached);
}

void RenderLayerCompositor::enableCompositingMode(bool enable /* = true */)
{
    if (enable != m_compositing) {
        m_compositing = enable;
        
        if (m_compositing) {
            ensureRootLayer();
            notifyIFramesOfCompositingChange();
        } else
            destroyRootLayer();
        
        
        m_renderView.layer()->setNeedsPostLayoutCompositingUpdate();
    }
}

void RenderLayerCompositor::cacheAcceleratedCompositingFlags()
{
    auto& settings = m_renderView.settings();
    bool hasAcceleratedCompositing = settings.acceleratedCompositingEnabled();

    // We allow the chrome to override the settings, in case the page is rendered
    // on a chrome that doesn't allow accelerated compositing.
    if (hasAcceleratedCompositing) {
        m_compositingTriggers = page().chrome().client().allowedCompositingTriggers();
        hasAcceleratedCompositing = m_compositingTriggers;
    }

    bool showDebugBorders = settings.showDebugBorders();
    bool showRepaintCounter = settings.showRepaintCounter();
    bool acceleratedDrawingEnabled = settings.acceleratedDrawingEnabled();
    bool displayListDrawingEnabled = settings.displayListDrawingEnabled();

    // forceCompositingMode for subframes can only be computed after layout.
    bool forceCompositingMode = m_forceCompositingMode;
    if (isMainFrameCompositor())
        forceCompositingMode = m_renderView.settings().forceCompositingMode() && hasAcceleratedCompositing; 
    
    if (hasAcceleratedCompositing != m_hasAcceleratedCompositing || showDebugBorders != m_showDebugBorders || showRepaintCounter != m_showRepaintCounter || forceCompositingMode != m_forceCompositingMode) {
        if (auto* rootLayer = m_renderView.layer()) {
            rootLayer->setNeedsCompositingConfigurationUpdate();
            rootLayer->setDescendantsNeedUpdateBackingAndHierarchyTraversal();
        }
    }

    bool debugBordersChanged = m_showDebugBorders != showDebugBorders;
    m_hasAcceleratedCompositing = hasAcceleratedCompositing;
    m_forceCompositingMode = forceCompositingMode;
    m_showDebugBorders = showDebugBorders;
    m_showRepaintCounter = showRepaintCounter;
    m_acceleratedDrawingEnabled = acceleratedDrawingEnabled;
    m_displayListDrawingEnabled = displayListDrawingEnabled;
    
    if (debugBordersChanged) {
        if (m_layerForHorizontalScrollbar)
            m_layerForHorizontalScrollbar->setShowDebugBorder(m_showDebugBorders);

        if (m_layerForVerticalScrollbar)
            m_layerForVerticalScrollbar->setShowDebugBorder(m_showDebugBorders);

        if (m_layerForScrollCorner)
            m_layerForScrollCorner->setShowDebugBorder(m_showDebugBorders);
    }
    
    if (updateCompositingPolicy())
        rootRenderLayer().setDescendantsNeedCompositingRequirementsTraversal();
}

void RenderLayerCompositor::cacheAcceleratedCompositingFlagsAfterLayout()
{
    cacheAcceleratedCompositingFlags();

    if (isMainFrameCompositor())
        return;

    RequiresCompositingData queryData;
    bool forceCompositingMode = m_hasAcceleratedCompositing && m_renderView.settings().forceCompositingMode() && requiresCompositingForScrollableFrame(queryData);
    if (forceCompositingMode != m_forceCompositingMode) {
        m_forceCompositingMode = forceCompositingMode;
        rootRenderLayer().setDescendantsNeedCompositingRequirementsTraversal();
    }
}

bool RenderLayerCompositor::updateCompositingPolicy()
{
    if (!usesCompositing())
        return false;

    auto currentPolicy = m_compositingPolicy;
    if (page().compositingPolicyOverride()) {
        m_compositingPolicy = page().compositingPolicyOverride().value();
        return m_compositingPolicy != currentPolicy;
    }
    
    auto memoryPolicy = MemoryPressureHandler::currentMemoryUsagePolicy();
    m_compositingPolicy = memoryPolicy == WTF::MemoryUsagePolicy::Unrestricted ? CompositingPolicy::Normal : CompositingPolicy::Conservative;
    return m_compositingPolicy != currentPolicy;
}

bool RenderLayerCompositor::canRender3DTransforms() const
{
    return hasAcceleratedCompositing() && (m_compositingTriggers & ChromeClient::ThreeDTransformTrigger);
}

void RenderLayerCompositor::willRecalcStyle()
{
    cacheAcceleratedCompositingFlags();
}

bool RenderLayerCompositor::didRecalcStyleWithNoPendingLayout()
{
    return updateCompositingLayers(CompositingUpdateType::AfterStyleChange);
}

void RenderLayerCompositor::customPositionForVisibleRectComputation(const GraphicsLayer* graphicsLayer, FloatPoint& position) const
{
    if (graphicsLayer != m_scrolledContentsLayer.get())
        return;

    FloatPoint scrollPosition = -position;

    if (m_renderView.frameView().scrollBehaviorForFixedElements() == StickToDocumentBounds)
        scrollPosition = m_renderView.frameView().constrainScrollPositionForOverhang(roundedIntPoint(scrollPosition));

    position = -scrollPosition;
}

void RenderLayerCompositor::notifyFlushRequired(const GraphicsLayer* layer)
{
    scheduleLayerFlush(layer->canThrottleLayerFlush());
}

void RenderLayerCompositor::scheduleLayerFlushNow()
{
    m_hasPendingLayerFlush = false;
    page().chrome().client().scheduleCompositingLayerFlush();
}

void RenderLayerCompositor::scheduleLayerFlush(bool canThrottle)
{
    ASSERT(!m_flushingLayers);

    if (canThrottle)
        startInitialLayerFlushTimerIfNeeded();

    if (canThrottle && isThrottlingLayerFlushes()) {
        m_hasPendingLayerFlush = true;
        return;
    }
    scheduleLayerFlushNow();
}

FloatRect RenderLayerCompositor::visibleRectForLayerFlushing() const
{
    const FrameView& frameView = m_renderView.frameView();
#if PLATFORM(IOS_FAMILY)
    return frameView.exposedContentRect();
#else
    // Having a m_scrolledContentsLayer indicates that we're doing scrolling via GraphicsLayers.
    FloatRect visibleRect = m_scrolledContentsLayer ? FloatRect({ }, frameView.sizeForVisibleContent()) : frameView.visibleContentRect();

    if (frameView.viewExposedRect())
        visibleRect.intersect(frameView.viewExposedRect().value());

    return visibleRect;
#endif
}

void RenderLayerCompositor::flushPendingLayerChanges(bool isFlushRoot)
{
    // FrameView::flushCompositingStateIncludingSubframes() flushes each subframe,
    // but GraphicsLayer::flushCompositingState() will cross frame boundaries
    // if the GraphicsLayers are connected (the RootLayerAttachedViaEnclosingFrame case).
    // As long as we're not the root of the flush, we can bail.
    if (!isFlushRoot && rootLayerAttachment() == RootLayerAttachedViaEnclosingFrame)
        return;
    
    if (rootLayerAttachment() == RootLayerUnattached) {
#if PLATFORM(IOS_FAMILY)
        startLayerFlushTimerIfNeeded();
#endif
        m_shouldFlushOnReattach = true;
        return;
    }

    auto& frameView = m_renderView.frameView();
    AnimationUpdateBlock animationUpdateBlock(&frameView.frame().animation());

    ASSERT(!m_flushingLayers);
    {
        SetForScope<bool> flushingLayersScope(m_flushingLayers, true);

        if (auto* rootLayer = rootGraphicsLayer()) {
            FloatRect visibleRect = visibleRectForLayerFlushing();
            LOG_WITH_STREAM(Compositing,  stream << "\nRenderLayerCompositor " << this << " flushPendingLayerChanges (is root " << isFlushRoot << ") visible rect " << visibleRect);
            rootLayer->flushCompositingState(visibleRect);
        }
        
        ASSERT(m_flushingLayers);
    }

#if PLATFORM(IOS_FAMILY)
    updateScrollCoordinatedLayersAfterFlushIncludingSubframes();

    if (isFlushRoot)
        page().chrome().client().didFlushCompositingLayers();
#endif

    ++m_layerFlushCount;
    startLayerFlushTimerIfNeeded();
}

#if PLATFORM(IOS_FAMILY)
void RenderLayerCompositor::updateScrollCoordinatedLayersAfterFlushIncludingSubframes()
{
    updateScrollCoordinatedLayersAfterFlush();

    auto& frame = m_renderView.frameView().frame();
    for (Frame* subframe = frame.tree().firstChild(); subframe; subframe = subframe->tree().traverseNext(&frame)) {
        auto* view = subframe->contentRenderer();
        if (!view)
            continue;

        view->compositor().updateScrollCoordinatedLayersAfterFlush();
    }
}

void RenderLayerCompositor::updateScrollCoordinatedLayersAfterFlush()
{
    if (m_legacyScrollingLayerCoordinator) {
        m_legacyScrollingLayerCoordinator->registerAllViewportConstrainedLayers(*this);
        m_legacyScrollingLayerCoordinator->registerScrollingLayersNeedingUpdate();
    }
}
#endif

void RenderLayerCompositor::didChangePlatformLayerForLayer(RenderLayer& layer, const GraphicsLayer*)
{
#if PLATFORM(IOS_FAMILY)
    if (m_legacyScrollingLayerCoordinator)
        m_legacyScrollingLayerCoordinator->didChangePlatformLayerForLayer(layer);
#endif

    auto* scrollingCoordinator = this->scrollingCoordinator();
    if (!scrollingCoordinator)
        return;

    auto* backing = layer.backing();
    if (auto nodeID = backing->scrollingNodeIDForRole(ScrollCoordinationRole::ViewportConstrained))
        scrollingCoordinator->setNodeLayers(nodeID, { backing->graphicsLayer() });

    if (auto nodeID = backing->scrollingNodeIDForRole(ScrollCoordinationRole::Scrolling)) {
        // FIXME: would be nice to not have to special-case the root.
        ScrollingCoordinator::NodeLayers nodeLayers;
        if (layer.isRenderViewLayer())
            nodeLayers = { nullptr, scrollContainerLayer(), scrolledContentsLayer(), fixedRootBackgroundLayer(), clipLayer(), rootContentsLayer() };
        else
            nodeLayers = { layer.backing()->graphicsLayer(), backing->scrollContainerLayer(), backing->scrolledContentsLayer() };

        scrollingCoordinator->setNodeLayers(nodeID, nodeLayers);
    }

    if (auto nodeID = backing->scrollingNodeIDForRole(ScrollCoordinationRole::FrameHosting))
        scrollingCoordinator->setNodeLayers(nodeID, { backing->graphicsLayer() });
}

void RenderLayerCompositor::didPaintBacking(RenderLayerBacking*)
{
    auto& frameView = m_renderView.frameView();
    frameView.setLastPaintTime(MonotonicTime::now());
    if (frameView.milestonesPendingPaint())
        frameView.firePaintRelatedMilestonesIfNeeded();
}

void RenderLayerCompositor::didChangeVisibleRect()
{
    auto* rootLayer = rootGraphicsLayer();
    if (!rootLayer)
        return;

    FloatRect visibleRect = visibleRectForLayerFlushing();
    bool requiresFlush = rootLayer->visibleRectChangeRequiresFlush(visibleRect);
    LOG_WITH_STREAM(Compositing, stream << "RenderLayerCompositor::didChangeVisibleRect " << visibleRect << " requiresFlush " << requiresFlush);
    if (requiresFlush)
        scheduleLayerFlushNow();
}

void RenderLayerCompositor::notifyFlushBeforeDisplayRefresh(const GraphicsLayer*)
{
    if (!m_layerUpdater) {
        PlatformDisplayID displayID = page().chrome().displayID();
        m_layerUpdater = std::make_unique<GraphicsLayerUpdater>(*this, displayID);
    }
    
    m_layerUpdater->scheduleUpdate();
}

void RenderLayerCompositor::flushLayersSoon(GraphicsLayerUpdater&)
{
    scheduleLayerFlush(true);
}

void RenderLayerCompositor::layerTiledBackingUsageChanged(const GraphicsLayer* graphicsLayer, bool usingTiledBacking)
{
    if (usingTiledBacking) {
        ++m_layersWithTiledBackingCount;
        graphicsLayer->tiledBacking()->setIsInWindow(page().isInWindow());
    } else {
        ASSERT(m_layersWithTiledBackingCount > 0);
        --m_layersWithTiledBackingCount;
    }
}

void RenderLayerCompositor::scheduleCompositingLayerUpdate()
{
    if (!m_updateCompositingLayersTimer.isActive())
        m_updateCompositingLayersTimer.startOneShot(0_s);
}

void RenderLayerCompositor::updateCompositingLayersTimerFired()
{
    updateCompositingLayers(CompositingUpdateType::AfterLayout);
}

void RenderLayerCompositor::cancelCompositingLayerUpdate()
{
    m_updateCompositingLayersTimer.stop();
}

static Optional<ScrollingNodeID> frameHostingNodeForFrame(Frame& frame)
{
    if (!frame.document() || !frame.view())
        return { };

    // Find the frame's enclosing layer in our render tree.
    auto* ownerElement = frame.document()->ownerElement();
    if (!ownerElement)
        return { };

    auto* frameRenderer = ownerElement->renderer();
    if (!frameRenderer || !is<RenderWidget>(frameRenderer))
        return { };

    auto& widgetRenderer = downcast<RenderWidget>(*frameRenderer);
    if (!widgetRenderer.hasLayer() || !widgetRenderer.layer()->isComposited()) {
        LOG(Scrolling, "frameHostingNodeForFrame: frame renderer has no layer or is not composited.");
        return { };
    }

    if (auto frameHostingNodeID = widgetRenderer.layer()->backing()->scrollingNodeIDForRole(ScrollCoordinationRole::FrameHosting))
        return frameHostingNodeID;

    return { };
}

// Returns true on a successful update.
bool RenderLayerCompositor::updateCompositingLayers(CompositingUpdateType updateType, RenderLayer* updateRoot)
{
    LOG_WITH_STREAM(Compositing, stream << "RenderLayerCompositor " << this << " updateCompositingLayers " << updateType << " contentLayersCount " << m_contentLayersCount);

#if ENABLE(TREE_DEBUGGING)
    if (compositingLogEnabled())
        showPaintOrderTree(m_renderView.layer());
#endif

    if (updateType == CompositingUpdateType::AfterStyleChange || updateType == CompositingUpdateType::AfterLayout)
        cacheAcceleratedCompositingFlagsAfterLayout(); // Some flags (e.g. forceCompositingMode) depend on layout.

    m_updateCompositingLayersTimer.stop();

    ASSERT(m_renderView.document().pageCacheState() == Document::NotInPageCache);
    
    // Compositing layers will be updated in Document::setVisualUpdatesAllowed(bool) if suppressed here.
    if (!m_renderView.document().visualUpdatesAllowed())
        return false;

    // Avoid updating the layers with old values. Compositing layers will be updated after the layout is finished.
    // This happens when m_updateCompositingLayersTimer fires before layout is updated.
    if (m_renderView.needsLayout()) {
        LOG_WITH_STREAM(Compositing, stream << "RenderLayerCompositor " << this << " updateCompositingLayers " << updateType << " - m_renderView.needsLayout, bailing ");
        return false;
    }

    if (!m_compositing && (m_forceCompositingMode || (isMainFrameCompositor() && page().pageOverlayController().overlayCount())))
        enableCompositingMode(true);

    bool isPageScroll = !updateRoot || updateRoot == &rootRenderLayer();
    updateRoot = &rootRenderLayer();

    if (updateType == CompositingUpdateType::OnScroll || updateType == CompositingUpdateType::OnCompositedScroll) {
        // We only get here if we didn't scroll on the scrolling thread, so this update needs to re-position viewport-constrained layers.
        if (m_renderView.settings().acceleratedCompositingForFixedPositionEnabled() && isPageScroll) {
            if (auto* viewportConstrainedObjects = m_renderView.frameView().viewportConstrainedObjects()) {
                for (auto* renderer : *viewportConstrainedObjects) {
                    if (auto* layer = renderer->layer())
                        layer->setNeedsCompositingGeometryUpdate();
                }
            }
        }

        // Scrolling can affect overlap. FIXME: avoid for page scrolling.
        updateRoot->setDescendantsNeedCompositingRequirementsTraversal();
    }

    if (!updateRoot->hasDescendantNeedingCompositingRequirementsTraversal() && !m_compositing) {
        LOG_WITH_STREAM(Compositing, stream << " no compositing work to do");
        return true;
    }

    if (!updateRoot->needsAnyCompositingTraversal()) {
        LOG_WITH_STREAM(Compositing, stream << " updateRoot has no dirty child and doesn't need update");
        return true;
    }

    ++m_compositingUpdateCount;

    AnimationUpdateBlock animationUpdateBlock(&m_renderView.frameView().frame().animation());

    SetForScope<bool> postLayoutChange(m_inPostLayoutUpdate, true);

#if !LOG_DISABLED
    MonotonicTime startTime;
    if (compositingLogEnabled()) {
        ++m_rootLayerUpdateCount;
        startTime = MonotonicTime::now();
    }

    if (compositingLogEnabled()) {
        m_obligateCompositedLayerCount = 0;
        m_secondaryCompositedLayerCount = 0;
        m_obligatoryBackingStoreBytes = 0;
        m_secondaryBackingStoreBytes = 0;

        auto& frame = m_renderView.frameView().frame();
        bool isMainFrame = isMainFrameCompositor();
        LOG_WITH_STREAM(Compositing, stream << "\nUpdate " << m_rootLayerUpdateCount << " of " << (isMainFrame ? "main frame" : frame.tree().uniqueName().string().utf8().data()) << " - compositing policy is " << m_compositingPolicy);
    }
#endif

    // FIXME: optimize root-only update.
    if (updateRoot->hasDescendantNeedingCompositingRequirementsTraversal() || updateRoot->needsCompositingRequirementsTraversal()) {
        CompositingState compositingState(updateRoot);
        OverlapMap overlapMap;

        bool descendantHas3DTransform = false;
        computeCompositingRequirements(nullptr, rootRenderLayer(), overlapMap, compositingState, descendantHas3DTransform);
    }

    LOG(Compositing, "\nRenderLayerCompositor::updateCompositingLayers - mid");
#if ENABLE(TREE_DEBUGGING)
    if (compositingLogEnabled())
        showPaintOrderTree(m_renderView.layer());
#endif

    if (updateRoot->hasDescendantNeedingUpdateBackingOrHierarchyTraversal() || updateRoot->needsUpdateBackingOrHierarchyTraversal()) {
        ScrollingTreeState scrollingTreeState = { 0, 0 };
        if (!m_renderView.frame().isMainFrame())
            scrollingTreeState.parentNodeID = frameHostingNodeForFrame(m_renderView.frame());

        Vector<Ref<GraphicsLayer>> childList;
        updateBackingAndHierarchy(*updateRoot, childList, scrollingTreeState);

        // Host the document layer in the RenderView's root layer.
        appendDocumentOverlayLayers(childList);
        // Even when childList is empty, don't drop out of compositing mode if there are
        // composited layers that we didn't hit in our traversal (e.g. because of visibility:hidden).
        if (childList.isEmpty() && !needsCompositingForContentOrOverlays())
            destroyRootLayer();
        else if (m_rootContentsLayer)
            m_rootContentsLayer->setChildren(WTFMove(childList));
    }

#if !LOG_DISABLED
    if (compositingLogEnabled()) {
        MonotonicTime endTime = MonotonicTime::now();
        LOG(Compositing, "Total layers   primary   secondary   obligatory backing (KB)   secondary backing(KB)   total backing (KB)  update time (ms)\n");

        LOG(Compositing, "%8d %11d %9d %20.2f %22.2f %22.2f %18.2f\n",
            m_obligateCompositedLayerCount + m_secondaryCompositedLayerCount, m_obligateCompositedLayerCount,
            m_secondaryCompositedLayerCount, m_obligatoryBackingStoreBytes / 1024, m_secondaryBackingStoreBytes / 1024, (m_obligatoryBackingStoreBytes + m_secondaryBackingStoreBytes) / 1024, (endTime - startTime).milliseconds());
    }
#endif

    // FIXME: Only do if dirty.
    updateRootLayerPosition();

#if ENABLE(TREE_DEBUGGING)
    if (compositingLogEnabled()) {
        LOG(Compositing, "RenderLayerCompositor::updateCompositingLayers - post");
        showPaintOrderTree(m_renderView.layer());
        LOG(Compositing, "RenderLayerCompositor::updateCompositingLayers - GraphicsLayers post, contentLayersCount %d", m_contentLayersCount);
        showGraphicsLayerTree(m_rootContentsLayer.get());
    }
#endif

    InspectorInstrumentation::layerTreeDidChange(&page());

    return true;
}

void RenderLayerCompositor::computeCompositingRequirements(RenderLayer* ancestorLayer, RenderLayer& layer, OverlapMap& overlapMap, CompositingState& compositingState, bool& descendantHas3DTransform)
{
    if (!layer.hasDescendantNeedingCompositingRequirementsTraversal() && !layer.needsCompositingRequirementsTraversal() && !compositingState.fullPaintOrderTraversalRequired && !compositingState.descendantsRequireCompositingUpdate) {
        traverseUnchangedSubtree(ancestorLayer, layer, overlapMap, compositingState, descendantHas3DTransform);
        return;
    }

#if ENABLE(TREE_DEBUGGING)
    LOG(Compositing, "%*p computeCompositingRequirements", 12 + compositingState.depth * 2, &layer);
#endif

    // FIXME: maybe we can avoid updating all remaining layers in paint order.
    compositingState.fullPaintOrderTraversalRequired |= layer.needsCompositingRequirementsTraversal();
    compositingState.descendantsRequireCompositingUpdate |= layer.descendantsNeedCompositingRequirementsTraversal();

    layer.updateDescendantDependentFlags();
    layer.updateLayerListsIfNeeded();

    layer.setHasCompositingDescendant(false);

    // We updated compositing for direct reasons in layerStyleChanged(). Here, check for compositing that can only be evaluated after layout.
    RequiresCompositingData queryData;
    bool willBeComposited = layer.isComposited();
    if (layer.needsPostLayoutCompositingUpdate() || compositingState.fullPaintOrderTraversalRequired || compositingState.descendantsRequireCompositingUpdate) {
        layer.setIndirectCompositingReason(RenderLayer::IndirectCompositingReason::None);
        willBeComposited = needsToBeComposited(layer, queryData);
    }

    compositingState.fullPaintOrderTraversalRequired |= layer.subsequentLayersNeedCompositingRequirementsTraversal();

    OverlapExtent layerExtent;
    // Use the fact that we're composited as a hint to check for an animating transform.
    // FIXME: Maybe needsToBeComposited() should return a bitmask of reasons, to avoid the need to recompute things.
    if (willBeComposited && !layer.isRenderViewLayer())
        layerExtent.hasTransformAnimation = isRunningTransformAnimation(layer.renderer());

    bool respectTransforms = !layerExtent.hasTransformAnimation;
    overlapMap.geometryMap().pushMappingsToAncestor(&layer, ancestorLayer, respectTransforms);

    RenderLayer::IndirectCompositingReason compositingReason = compositingState.subtreeIsCompositing ? RenderLayer::IndirectCompositingReason::Stacking : RenderLayer::IndirectCompositingReason::None;

    // If we know for sure the layer is going to be composited, don't bother looking it up in the overlap map
    if (!willBeComposited && !overlapMap.isEmpty() && compositingState.testingOverlap) {
        computeExtent(overlapMap, layer, layerExtent);
        // If we're testing for overlap, we only need to composite if we overlap something that is already composited.
        compositingReason = overlapMap.overlapsLayers(layerExtent.bounds) ? RenderLayer::IndirectCompositingReason::Overlap : RenderLayer::IndirectCompositingReason::None;
    }

#if ENABLE(VIDEO)
    // Video is special. It's the only RenderLayer type that can both have
    // RenderLayer children and whose children can't use its backing to render
    // into. These children (the controls) always need to be promoted into their
    // own layers to draw on top of the accelerated video.
    if (compositingState.compositingAncestor && compositingState.compositingAncestor->renderer().isVideo())
        compositingReason = RenderLayer::IndirectCompositingReason::Overlap;
#endif

    if (compositingReason != RenderLayer::IndirectCompositingReason::None)
        layer.setIndirectCompositingReason(compositingReason);

    // Check if the computed indirect reason will force the layer to become composited.
    if (!willBeComposited && layer.mustCompositeForIndirectReasons() && canBeComposited(layer))
        willBeComposited = true;

    // The children of this layer don't need to composite, unless there is
    // a compositing layer among them, so start by inheriting the compositing
    // ancestor with subtreeIsCompositing set to false.
    CompositingState childState(compositingState);
    childState.subtreeIsCompositing = false;
#if ENABLE(CSS_COMPOSITING)
    childState.hasNotIsolatedCompositedBlendingDescendants = false;
#endif

    if (willBeComposited) {
        // Tell the parent it has compositing descendants.
        compositingState.subtreeIsCompositing = true;
        // This layer now acts as the ancestor for kids.
        childState.compositingAncestor = &layer;

        overlapMap.pushCompositingContainer();
        // This layer is going to be composited, so children can safely ignore the fact that there's an
        // animation running behind this layer, meaning they can rely on the overlap map testing again.
        childState.testingOverlap = true;

        computeExtent(overlapMap, layer, layerExtent);
        childState.ancestorHasTransformAnimation |= layerExtent.hasTransformAnimation;
        // Too hard to compute animated bounds if both us and some ancestor is animating transform.
        layerExtent.animationCausesExtentUncertainty |= layerExtent.hasTransformAnimation && compositingState.ancestorHasTransformAnimation;
    }

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    bool anyDescendantHas3DTransform = false;

    for (auto* childLayer : layer.negativeZOrderLayers()) {
        computeCompositingRequirements(&layer, *childLayer, overlapMap, childState, anyDescendantHas3DTransform);

        // If we have to make a layer for this child, make one now so we can have a contents layer
        // (since we need to ensure that the -ve z-order child renders underneath our contents).
        if (!willBeComposited && childState.subtreeIsCompositing) {
            // make layer compositing
            layer.setIndirectCompositingReason(RenderLayer::IndirectCompositingReason::BackgroundLayer);
            childState.compositingAncestor = &layer;
            overlapMap.pushCompositingContainer();
            // This layer is going to be composited, so children can safely ignore the fact that there's an
            // animation running behind this layer, meaning they can rely on the overlap map testing again
            childState.testingOverlap = true;
            willBeComposited = true;
        }
    }
    
    for (auto* childLayer : layer.normalFlowLayers())
        computeCompositingRequirements(&layer, *childLayer, overlapMap, childState, anyDescendantHas3DTransform);

    for (auto* childLayer : layer.positiveZOrderLayers())
        computeCompositingRequirements(&layer, *childLayer, overlapMap, childState, anyDescendantHas3DTransform);

    // If we just entered compositing mode, the root will have become composited (as long as accelerated compositing is enabled).
    if (layer.isRenderViewLayer()) {
        if (usesCompositing() && m_hasAcceleratedCompositing)
            willBeComposited = true;
    }
    
    // All layers (even ones that aren't being composited) need to get added to
    // the overlap map. Layers that do not composite will draw into their
    // compositing ancestor's backing, and so are still considered for overlap.
    // FIXME: When layerExtent has taken animation bounds into account, we also know that the bounds
    // include descendants, so we don't need to add them all to the overlap map.
    if (childState.compositingAncestor && !childState.compositingAncestor->isRenderViewLayer())
        addToOverlapMap(overlapMap, layer, layerExtent);

#if ENABLE(CSS_COMPOSITING)
    layer.setHasNotIsolatedCompositedBlendingDescendants(childState.hasNotIsolatedCompositedBlendingDescendants);
    ASSERT(!layer.hasNotIsolatedCompositedBlendingDescendants() || layer.hasNotIsolatedBlendingDescendants());
#endif
    // Now check for reasons to become composited that depend on the state of descendant layers.
    RenderLayer::IndirectCompositingReason indirectCompositingReason;
    if (!willBeComposited && canBeComposited(layer)
        && requiresCompositingForIndirectReason(layer.renderer(), childState.subtreeIsCompositing, anyDescendantHas3DTransform, indirectCompositingReason)) {
        layer.setIndirectCompositingReason(indirectCompositingReason);
        childState.compositingAncestor = &layer;
        overlapMap.pushCompositingContainer();
        addToOverlapMapRecursive(overlapMap, layer);
        willBeComposited = true;
    }
    
    if (layer.reflectionLayer()) {
        // FIXME: Shouldn't we call computeCompositingRequirements to handle a reflection overlapping with another renderer?
        layer.reflectionLayer()->setIndirectCompositingReason(willBeComposited ? RenderLayer::IndirectCompositingReason::Stacking : RenderLayer::IndirectCompositingReason::None);
    }

    // Subsequent layers in the parent stacking context also need to composite.
    compositingState.subtreeIsCompositing |= childState.subtreeIsCompositing;
    compositingState.fullPaintOrderTraversalRequired |= childState.fullPaintOrderTraversalRequired;

    // Set the flag to say that this layer has compositing children.
    layer.setHasCompositingDescendant(childState.subtreeIsCompositing);

    // setHasCompositingDescendant() may have changed the answer to needsToBeComposited() when clipping, so test that again.
    bool isCompositedClippingLayer = canBeComposited(layer) && clipsCompositingDescendants(layer);

    // Turn overlap testing off for later layers if it's already off, or if we have an animating transform.
    // Note that if the layer clips its descendants, there's no reason to propagate the child animation to the parent layers. That's because
    // we know for sure the animation is contained inside the clipping rectangle, which is already added to the overlap map.
    if ((!childState.testingOverlap && !isCompositedClippingLayer) || layerExtent.knownToBeHaveExtentUncertainty())
        compositingState.testingOverlap = false;
    
    if (isCompositedClippingLayer) {
        if (!willBeComposited) {
            childState.compositingAncestor = &layer;
            overlapMap.pushCompositingContainer();
            addToOverlapMapRecursive(overlapMap, layer);
            willBeComposited = true;
        }
    }

#if ENABLE(CSS_COMPOSITING)
    if ((willBeComposited && layer.hasBlendMode())
        || (layer.hasNotIsolatedCompositedBlendingDescendants() && !layer.isolatesCompositedBlending()))
        compositingState.hasNotIsolatedCompositedBlendingDescendants = true;
#endif

    if (childState.compositingAncestor == &layer && !layer.isRenderViewLayer())
        overlapMap.popCompositingContainer();

    // If we're back at the root, and no other layers need to be composited, and the root layer itself doesn't need
    // to be composited, then we can drop out of compositing mode altogether. However, don't drop out of compositing mode
    // if there are composited layers that we didn't hit in our traversal (e.g. because of visibility:hidden).
    RequiresCompositingData rootLayerQueryData;
    if (layer.isRenderViewLayer() && !childState.subtreeIsCompositing && !requiresCompositingLayer(layer, rootLayerQueryData) && !m_forceCompositingMode && !needsCompositingForContentOrOverlays()) {
        // Don't drop out of compositing on iOS, because we may flash. See <rdar://problem/8348337>.
#if !PLATFORM(IOS_FAMILY)
        enableCompositingMode(false);
        willBeComposited = false;
#endif
    }
    
    ASSERT(willBeComposited == needsToBeComposited(layer, queryData));

    // Create or destroy backing here. However, we can't update geometry because layers above us may become composited
    // during post-order traversal (e.g. for clipping).
    if (updateBacking(layer, queryData, CompositingChangeRepaintNow, willBeComposited ? BackingRequired::Yes : BackingRequired::No)) {
        layer.setNeedsCompositingLayerConnection();
        // Child layers need to get a geometry update to recompute their position.
        layer.setChildrenNeedCompositingGeometryUpdate();
        // The composited bounds of enclosing layers depends on which descendants are composited, so they need a geometry update.
        layer.setNeedsCompositingGeometryUpdateOnAncestors();
    }

    if (layer.reflectionLayer() && updateLayerCompositingState(*layer.reflectionLayer(), queryData, CompositingChangeRepaintNow))
        layer.setNeedsCompositingLayerConnection();

    descendantHas3DTransform |= anyDescendantHas3DTransform || layer.has3DTransform();
    
    // FIXME: clarify needsCompositingPaintOrderChildrenUpdate. If a composited layer gets a new ancestor, it needs geometry computations.
    if (layer.needsCompositingPaintOrderChildrenUpdate()) {
        layer.setChildrenNeedCompositingGeometryUpdate();
        layer.setNeedsCompositingLayerConnection();
    }

#if ENABLE(TREE_DEBUGGING)
    LOG(Compositing, "%*p computeCompositingRequirements - willBeComposited %d", 12 + compositingState.depth * 2, &layer, willBeComposited);
#endif

    layer.clearCompositingRequirementsTraversalState();
    
    overlapMap.geometryMap().popMappingsToAncestor(ancestorLayer);
}

// We have to traverse unchanged layers to fill in the overlap map.
void RenderLayerCompositor::traverseUnchangedSubtree(RenderLayer* ancestorLayer, RenderLayer& layer, OverlapMap& overlapMap, CompositingState& compositingState, bool& descendantHas3DTransform)
{
    ASSERT(!compositingState.fullPaintOrderTraversalRequired);
    ASSERT(!layer.hasDescendantNeedingCompositingRequirementsTraversal());
    ASSERT(!layer.needsCompositingRequirementsTraversal());

#if ENABLE(TREE_DEBUGGING)
    LOG(Compositing, "%*p traverseUnchangedSubtree", 12 + compositingState.depth * 2, &layer);
#endif

    layer.updateDescendantDependentFlags();
    layer.updateLayerListsIfNeeded();

    bool layerIsComposited = layer.isComposited();

    OverlapExtent layerExtent;
    if (layerIsComposited && !layer.isRenderViewLayer())
        layerExtent.hasTransformAnimation = isRunningTransformAnimation(layer.renderer());

    bool respectTransforms = !layerExtent.hasTransformAnimation;
    overlapMap.geometryMap().pushMappingsToAncestor(&layer, ancestorLayer, respectTransforms);

    // If we know for sure the layer is going to be composited, don't bother looking it up in the overlap map
    if (!layerIsComposited && !overlapMap.isEmpty() && compositingState.testingOverlap)
        computeExtent(overlapMap, layer, layerExtent);

    CompositingState childState(compositingState);
    childState.subtreeIsCompositing = false;
#if ENABLE(CSS_COMPOSITING)
    childState.hasNotIsolatedCompositedBlendingDescendants = false;
#endif

    if (layerIsComposited) {
        // Tell the parent it has compositing descendants.
        compositingState.subtreeIsCompositing = true;
        // This layer now acts as the ancestor for kids.
        childState.compositingAncestor = &layer;

        overlapMap.pushCompositingContainer();
        // This layer is going to be composited, so children can safely ignore the fact that there's an
        // animation running behind this layer, meaning they can rely on the overlap map testing again.
        childState.testingOverlap = true;

        computeExtent(overlapMap, layer, layerExtent);
        childState.ancestorHasTransformAnimation |= layerExtent.hasTransformAnimation;
        // Too hard to compute animated bounds if both us and some ancestor is animating transform.
        layerExtent.animationCausesExtentUncertainty |= layerExtent.hasTransformAnimation && compositingState.ancestorHasTransformAnimation;
    }

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    bool anyDescendantHas3DTransform = false;

    for (auto* childLayer : layer.negativeZOrderLayers()) {
        traverseUnchangedSubtree(&layer, *childLayer, overlapMap, childState, anyDescendantHas3DTransform);
        if (childState.subtreeIsCompositing)
            ASSERT(layerIsComposited);
    }
    
    for (auto* childLayer : layer.normalFlowLayers())
        traverseUnchangedSubtree(&layer, *childLayer, overlapMap, childState, anyDescendantHas3DTransform);

    for (auto* childLayer : layer.positiveZOrderLayers())
        traverseUnchangedSubtree(&layer, *childLayer, overlapMap, childState, anyDescendantHas3DTransform);

    // All layers (even ones that aren't being composited) need to get added to
    // the overlap map. Layers that do not composite will draw into their
    // compositing ancestor's backing, and so are still considered for overlap.
    // FIXME: When layerExtent has taken animation bounds into account, we also know that the bounds
    // include descendants, so we don't need to add them all to the overlap map.
    if (childState.compositingAncestor && !childState.compositingAncestor->isRenderViewLayer())
        addToOverlapMap(overlapMap, layer, layerExtent);

    // Subsequent layers in the parent stacking context also need to composite.
    if (childState.subtreeIsCompositing)
        compositingState.subtreeIsCompositing = true;

    // Set the flag to say that this layer has compositing children.
    ASSERT(layer.hasCompositingDescendant() == childState.subtreeIsCompositing);

    // setHasCompositingDescendant() may have changed the answer to needsToBeComposited() when clipping, so test that again.
    bool isCompositedClippingLayer = canBeComposited(layer) && clipsCompositingDescendants(layer);

    // Turn overlap testing off for later layers if it's already off, or if we have an animating transform.
    // Note that if the layer clips its descendants, there's no reason to propagate the child animation to the parent layers. That's because
    // we know for sure the animation is contained inside the clipping rectangle, which is already added to the overlap map.
    if ((!childState.testingOverlap && !isCompositedClippingLayer) || layerExtent.knownToBeHaveExtentUncertainty())
        compositingState.testingOverlap = false;
    
    if (isCompositedClippingLayer)
        ASSERT(layerIsComposited);

#if ENABLE(CSS_COMPOSITING)
    if ((layerIsComposited && layer.hasBlendMode())
        || (layer.hasNotIsolatedCompositedBlendingDescendants() && !layer.isolatesCompositedBlending()))
        compositingState.hasNotIsolatedCompositedBlendingDescendants = true;
#endif

    if (childState.compositingAncestor == &layer && !layer.isRenderViewLayer())
        overlapMap.popCompositingContainer();

    descendantHas3DTransform |= anyDescendantHas3DTransform || layer.has3DTransform();

    ASSERT(!layer.needsCompositingRequirementsTraversal());

    overlapMap.geometryMap().popMappingsToAncestor(ancestorLayer);
}

void RenderLayerCompositor::updateBackingAndHierarchy(RenderLayer& layer, Vector<Ref<GraphicsLayer>>& childLayersOfEnclosingLayer, ScrollingTreeState& scrollingTreeState, OptionSet<UpdateLevel> updateLevel, int depth)
{
    layer.updateDescendantDependentFlags();
    layer.updateLayerListsIfNeeded();

    bool layerNeedsUpdate = !updateLevel.isEmpty();
    if (layer.descendantsNeedUpdateBackingAndHierarchyTraversal())
        updateLevel.add(UpdateLevel::AllDescendants);

    ScrollingTreeState stateForDescendants = scrollingTreeState;

    auto* layerBacking = layer.backing();
    if (layerBacking) {
        updateLevel.remove(UpdateLevel::CompositedChildren);

        // We updated the composited bounds in RenderLayerBacking::updateAfterLayout(), but it may have changed
        // based on which descendants are now composited.
        if (layerBacking->updateCompositedBounds()) {
            layer.setNeedsCompositingGeometryUpdate();
            // Our geometry can affect descendants.
            updateLevel.add(UpdateLevel::CompositedChildren);
        }
        
        if (layerNeedsUpdate || layer.needsCompositingConfigurationUpdate()) {
            if (layerBacking->updateConfiguration()) {
                layerNeedsUpdate = true; // We also need to update geometry.
                layer.setNeedsCompositingLayerConnection();
            }

            layerBacking->updateDebugIndicators(m_showDebugBorders, m_showRepaintCounter);
        }
        
        OptionSet<ScrollingNodeChangeFlags> scrollingNodeChanges = { ScrollingNodeChangeFlags::Layer };
        if (layerNeedsUpdate || layer.needsCompositingGeometryUpdate() || layer.needsScrollingTreeUpdate()) {
            layerBacking->updateGeometry();
            scrollingNodeChanges.add(ScrollingNodeChangeFlags::LayerGeometry);
        }

        if (auto* reflection = layer.reflectionLayer()) {
            if (auto* reflectionBacking = reflection->backing()) {
                reflectionBacking->updateCompositedBounds();
                reflectionBacking->updateGeometry();
                reflectionBacking->updateAfterDescendants();
            }
        }

        if (!layer.parent())
            updateRootLayerPosition();

        // FIXME: do based on dirty flags. Need to do this for changes of geometry, configuration and hierarchy.
        // Need to be careful to do the right thing when a scroll-coordinated layer loses a scroll-coordinated ancestor.
        stateForDescendants.parentNodeID = updateScrollCoordinationForLayer(layer, scrollingTreeState, layerBacking->coordinatedScrollingRoles(), scrollingNodeChanges);
        stateForDescendants.nextChildIndex = 0;

#if !LOG_DISABLED
        logLayerInfo(layer, "updateBackingAndHierarchy", depth);
#else
        UNUSED_PARAM(depth);
#endif
    }

    if (layer.childrenNeedCompositingGeometryUpdate())
        updateLevel.add(UpdateLevel::CompositedChildren);

    // If this layer has backing, then we are collecting its children, otherwise appending
    // to the compositing child list of an enclosing layer.
    Vector<Ref<GraphicsLayer>> layerChildren;
    auto& childList = layerBacking ? layerChildren : childLayersOfEnclosingLayer;

    bool requireDescendantTraversal = layer.hasDescendantNeedingUpdateBackingOrHierarchyTraversal()
        || (layer.hasCompositingDescendant() && (!layerBacking || layer.needsCompositingLayerConnection() || !updateLevel.isEmpty()));

    bool requiresChildRebuild = layerBacking && layer.needsCompositingLayerConnection() && !layer.hasCompositingDescendant();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif
    
    auto appendForegroundLayerIfNecessary = [&] () {
        // If a negative z-order child is compositing, we get a foreground layer which needs to get parented.
        if (layer.negativeZOrderLayers().size()) {
            if (layerBacking && layerBacking->foregroundLayer())
                childList.append(*layerBacking->foregroundLayer());
        }
    };

    if (requireDescendantTraversal) {
        for (auto* renderLayer : layer.negativeZOrderLayers())
            updateBackingAndHierarchy(*renderLayer, childList, stateForDescendants, updateLevel, depth + 1);

        appendForegroundLayerIfNecessary();

        for (auto* renderLayer : layer.normalFlowLayers())
            updateBackingAndHierarchy(*renderLayer, childList, stateForDescendants, updateLevel, depth + 1);
        
        for (auto* renderLayer : layer.positiveZOrderLayers())
            updateBackingAndHierarchy(*renderLayer, childList, stateForDescendants, updateLevel, depth + 1);
    } else if (requiresChildRebuild)
        appendForegroundLayerIfNecessary();

    if (layerBacking) {
        if (requireDescendantTraversal || requiresChildRebuild) {
            bool parented = false;
            if (is<RenderWidget>(layer.renderer()))
                parented = parentFrameContentLayers(downcast<RenderWidget>(layer.renderer()));

            if (!parented)
                layerBacking->parentForSublayers()->setChildren(WTFMove(layerChildren));

            // If the layer has a clipping layer the overflow controls layers will be siblings of the clipping layer.
            // Otherwise, the overflow control layers are normal children.
            if (!layerBacking->hasClippingLayer() && !layerBacking->hasScrollingLayer()) {
                if (auto* overflowControlLayer = layerBacking->layerForHorizontalScrollbar())
                    layerBacking->parentForSublayers()->addChild(*overflowControlLayer);

                if (auto* overflowControlLayer = layerBacking->layerForVerticalScrollbar())
                    layerBacking->parentForSublayers()->addChild(*overflowControlLayer);

                if (auto* overflowControlLayer = layerBacking->layerForScrollCorner())
                    layerBacking->parentForSublayers()->addChild(*overflowControlLayer);
            }
        }

        childLayersOfEnclosingLayer.append(*layerBacking->childForSuperlayers());

        layerBacking->updateAfterDescendants();
    }
    
    layer.clearUpdateBackingOrHierarchyTraversalState();
}

void RenderLayerCompositor::appendDocumentOverlayLayers(Vector<Ref<GraphicsLayer>>& childList)
{
    if (!isMainFrameCompositor() || !m_compositing)
        return;

    if (!page().pageOverlayController().hasDocumentOverlays())
        return;

    Ref<GraphicsLayer> overlayHost = page().pageOverlayController().layerWithDocumentOverlays();
    childList.append(WTFMove(overlayHost));
}

bool RenderLayerCompositor::needsCompositingForContentOrOverlays() const
{
    return m_contentLayersCount + page().pageOverlayController().overlayCount();
}

void RenderLayerCompositor::layerBecameComposited(const RenderLayer& layer)
{
    if (&layer != m_renderView.layer())
        ++m_contentLayersCount;
}

void RenderLayerCompositor::layerBecameNonComposited(const RenderLayer& layer)
{
    // Inform the inspector that the given RenderLayer was destroyed.
    // FIXME: "destroyed" is a misnomer.
    InspectorInstrumentation::renderLayerDestroyed(&page(), layer);

    if (&layer != m_renderView.layer()) {
        ASSERT(m_contentLayersCount > 0);
        --m_contentLayersCount;
    }
}

#if !LOG_DISABLED
void RenderLayerCompositor::logLayerInfo(const RenderLayer& layer, const char* phase, int depth)
{
    if (!compositingLogEnabled())
        return;

    auto* backing = layer.backing();
    RequiresCompositingData queryData;
    if (requiresCompositingLayer(layer, queryData) || layer.isRenderViewLayer()) {
        ++m_obligateCompositedLayerCount;
        m_obligatoryBackingStoreBytes += backing->backingStoreMemoryEstimate();
    } else {
        ++m_secondaryCompositedLayerCount;
        m_secondaryBackingStoreBytes += backing->backingStoreMemoryEstimate();
    }

    LayoutRect absoluteBounds = backing->compositedBounds();
    absoluteBounds.move(layer.offsetFromAncestor(m_renderView.layer()));
    
    StringBuilder logString;
    logString.append(makeString(pad(' ', 12 + depth * 2, hex(reinterpret_cast<uintptr_t>(&layer))), " id ", backing->graphicsLayer()->primaryLayerID(), " (", FormattedNumber::fixedWidth(absoluteBounds.x().toFloat(), 3), ',', FormattedNumber::fixedWidth(absoluteBounds.y().toFloat(), 3), '-', FormattedNumber::fixedWidth(absoluteBounds.maxX().toFloat(), 3), ',', FormattedNumber::fixedWidth(absoluteBounds.maxY().toFloat(), 3), ") ", FormattedNumber::fixedWidth(backing->backingStoreMemoryEstimate() / 1024, 2), "KB"));

    if (!layer.renderer().style().hasAutoZIndex())
        logString.append(makeString(" z-index: ", layer.renderer().style().zIndex()));

    logString.appendLiteral(" (");
    logString.append(logReasonsForCompositing(layer));
    logString.appendLiteral(") ");

    if (backing->graphicsLayer()->contentsOpaque() || backing->paintsIntoCompositedAncestor() || backing->foregroundLayer() || backing->backgroundLayer()) {
        logString.append('[');
        bool prependSpace = false;
        if (backing->graphicsLayer()->contentsOpaque()) {
            logString.appendLiteral("opaque");
            prependSpace = true;
        }

        if (backing->paintsIntoCompositedAncestor()) {
            if (prependSpace)
                logString.appendLiteral(", ");
            logString.appendLiteral("paints into ancestor");
            prependSpace = true;
        }

        if (backing->foregroundLayer() || backing->backgroundLayer()) {
            if (prependSpace)
                logString.appendLiteral(", ");
            if (backing->foregroundLayer() && backing->backgroundLayer()) {
                logString.appendLiteral("+foreground+background");
                prependSpace = true;
            } else if (backing->foregroundLayer()) {
                logString.appendLiteral("+foreground");
                prependSpace = true;
            } else {
                logString.appendLiteral("+background");
                prependSpace = true;
            }
        }
        
        if (backing->paintsSubpixelAntialiasedText()) {
            if (prependSpace)
                logString.appendLiteral(", ");
            logString.appendLiteral("texty");
        }

        logString.appendLiteral("] ");
    }

    logString.append(layer.name());

    logString.appendLiteral(" - ");
    logString.append(phase);

    LOG(Compositing, "%s", logString.toString().utf8().data());
}
#endif

static bool clippingChanged(const RenderStyle& oldStyle, const RenderStyle& newStyle)
{
    return oldStyle.overflowX() != newStyle.overflowX() || oldStyle.overflowY() != newStyle.overflowY()
        || oldStyle.hasClip() != newStyle.hasClip() || oldStyle.clip() != newStyle.clip();
}

static bool styleAffectsLayerGeometry(const RenderStyle& style)
{
    return style.hasClip() || style.clipPath() || style.hasBorderRadius();
}

void RenderLayerCompositor::layerStyleChanged(StyleDifference diff, RenderLayer& layer, const RenderStyle* oldStyle)
{
    if (diff == StyleDifference::Equal)
        return;

    // Create or destroy backing here so that code that runs during layout can reliably use isComposited() (though this
    // is only true for layers composited for direct reasons).
    // Also, it allows us to avoid a tree walk in updateCompositingLayers() when no layer changed its compositing state.
    RequiresCompositingData queryData;
    queryData.layoutUpToDate = LayoutUpToDate::No;
    
    bool layerChanged = updateBacking(layer, queryData, CompositingChangeRepaintNow);
    if (layerChanged) {
        layer.setChildrenNeedCompositingGeometryUpdate();
        layer.setNeedsCompositingLayerConnection();
        layer.setSubsequentLayersNeedCompositingRequirementsTraversal();
        // Ancestor layers that composited for indirect reasons (things listed in styleChangeMayAffectIndirectCompositingReasons()) need to get updated.
        // This could be optimized by only setting this flag on layers with the relevant styles.
        layer.setNeedsPostLayoutCompositingUpdateOnAncestors();
    }
    
    if (queryData.reevaluateAfterLayout)
        layer.setNeedsPostLayoutCompositingUpdate();

    if (diff >= StyleDifference::LayoutPositionedMovementOnly && hasContentCompositingLayers()) {
        layer.setNeedsPostLayoutCompositingUpdate();
        layer.setNeedsCompositingGeometryUpdate();
    }

    auto* backing = layer.backing();
    if (!backing)
        return;

    backing->updateConfigurationAfterStyleChange();

    const auto& newStyle = layer.renderer().style();

    if (diff >= StyleDifference::Repaint) {
        // Visibility change may affect geometry of the enclosing composited layer.
        if (oldStyle && oldStyle->visibility() != newStyle.visibility())
            layer.setNeedsCompositingGeometryUpdate();
        
        // We'll get a diff of Repaint when things like clip-path change; these might affect layer or inner-layer geometry.
        if (layer.isComposited() && oldStyle) {
            if (styleAffectsLayerGeometry(*oldStyle) || styleAffectsLayerGeometry(newStyle))
                layer.setNeedsCompositingGeometryUpdate();
        }
    }

    // This is necessary to get iframe layers hooked up in response to scheduleInvalidateStyleAndLayerComposition().
    if (diff == StyleDifference::RecompositeLayer && layer.isComposited() && is<RenderWidget>(layer.renderer()))
        layer.setNeedsCompositingConfigurationUpdate();

    if (diff >= StyleDifference::RecompositeLayer && oldStyle) {
        if (oldStyle->transform() != newStyle.transform()) {
            // FIXME: transform changes really need to trigger layout. See RenderElement::adjustStyleDifference().
            layer.setNeedsPostLayoutCompositingUpdate();
            layer.setNeedsCompositingGeometryUpdate();
        }
    }

    if (diff >= StyleDifference::Layout) {
        // FIXME: only set flags here if we know we have a composited descendant, but we might not know at this point.
        if (oldStyle && clippingChanged(*oldStyle, newStyle)) {
            if (layer.isStackingContext()) {
                layer.setNeedsPostLayoutCompositingUpdate(); // Layer needs to become composited if it has composited descendants.
                layer.setNeedsCompositingConfigurationUpdate(); // If already composited, layer needs to create/destroy clipping layer.
            } else {
                // Descendant (in containing block order) compositing layers need to re-evaluate their clipping,
                // but they might be siblings in z-order so go up to our stacking context.
                if (auto* stackingContext = layer.stackingContext())
                    stackingContext->setDescendantsNeedUpdateBackingAndHierarchyTraversal();
            }
        }

        // These properties trigger compositing if some descendant is composited.
        if (oldStyle && styleChangeMayAffectIndirectCompositingReasons(*oldStyle, newStyle))
            layer.setNeedsPostLayoutCompositingUpdate();

        layer.setNeedsCompositingGeometryUpdate();
    }
}

bool RenderLayerCompositor::needsCompositingUpdateForStyleChangeOnNonCompositedLayer(RenderLayer& layer, const RenderStyle* oldStyle) const
{
    // Needed for scroll bars.
    if (layer.isRenderViewLayer())
        return true;

    if (!oldStyle)
        return false;

    const RenderStyle& newStyle = layer.renderer().style();
    // Visibility change may affect geometry of the enclosing composited layer.
    if (oldStyle->visibility() != newStyle.visibility())
        return true;

    // We don't have any direct reasons for this style change to affect layer composition. Test if it might affect things indirectly.
    if (styleChangeMayAffectIndirectCompositingReasons(*oldStyle, newStyle))
        return true;

    return false;
}

bool RenderLayerCompositor::canCompositeClipPath(const RenderLayer& layer)
{
    ASSERT(layer.isComposited());
    ASSERT(layer.renderer().style().clipPath());

    if (layer.renderer().hasMask())
        return false;

    auto& clipPath = *layer.renderer().style().clipPath();
    return (clipPath.type() != ClipPathOperation::Shape || clipPath.type() == ClipPathOperation::Shape) && GraphicsLayer::supportsLayerType(GraphicsLayer::Type::Shape);
}

// FIXME: remove and never ask questions about reflection layers.
static RenderLayerModelObject& rendererForCompositingTests(const RenderLayer& layer)
{
    auto* renderer = &layer.renderer();

    // The compositing state of a reflection should match that of its reflected layer.
    if (layer.isReflection())
        renderer = downcast<RenderLayerModelObject>(renderer->parent()); // The RenderReplica's parent is the object being reflected.

    return *renderer;
}

void RenderLayerCompositor::updateRootContentLayerClipping()
{
    m_rootContentsLayer->setMasksToBounds(!m_renderView.settings().backgroundShouldExtendBeyondPage());
}

bool RenderLayerCompositor::updateBacking(RenderLayer& layer, RequiresCompositingData& queryData, CompositingChangeRepaint shouldRepaint, BackingRequired backingRequired)
{
    bool layerChanged = false;
    if (backingRequired == BackingRequired::Unknown)
        backingRequired = needsToBeComposited(layer, queryData) ? BackingRequired::Yes : BackingRequired::No;
    else {
        // Need to fetch viewportConstrainedNotCompositedReason, but without doing all the work that needsToBeComposited does.
        requiresCompositingForPosition(rendererForCompositingTests(layer), layer, queryData);
    }

    if (backingRequired == BackingRequired::Yes) {
        enableCompositingMode();
        
        if (!layer.backing()) {
            // If we need to repaint, do so before making backing
            if (shouldRepaint == CompositingChangeRepaintNow)
                repaintOnCompositingChange(layer);

            layer.ensureBacking();

            if (layer.isRenderViewLayer() && useCoordinatedScrollingForLayer(layer)) {
                auto& frameView = m_renderView.frameView();
                if (auto* scrollingCoordinator = this->scrollingCoordinator())
                    scrollingCoordinator->frameViewRootLayerDidChange(frameView);
#if ENABLE(RUBBER_BANDING)
                updateLayerForHeader(frameView.headerHeight());
                updateLayerForFooter(frameView.footerHeight());
#endif
                updateRootContentLayerClipping();

                if (auto* tiledBacking = layer.backing()->tiledBacking())
                    tiledBacking->setTopContentInset(frameView.topContentInset());
            }

            // This layer and all of its descendants have cached repaints rects that are relative to
            // the repaint container, so change when compositing changes; we need to update them here.
            if (layer.parent())
                layer.computeRepaintRectsIncludingDescendants();
            
            layer.setNeedsCompositingGeometryUpdate();
            layer.setNeedsCompositingConfigurationUpdate();
            layer.setNeedsCompositingPaintOrderChildrenUpdate();

            layerChanged = true;
        }
    } else {
        if (layer.backing()) {
            // If we're removing backing on a reflection, clear the source GraphicsLayer's pointer to
            // its replica GraphicsLayer. In practice this should never happen because reflectee and reflection 
            // are both either composited, or not composited.
            if (layer.isReflection()) {
                auto* sourceLayer = downcast<RenderLayerModelObject>(*layer.renderer().parent()).layer();
                if (auto* backing = sourceLayer->backing()) {
                    ASSERT(backing->graphicsLayer()->replicaLayer() == layer.backing()->graphicsLayer());
                    backing->graphicsLayer()->setReplicatedByLayer(nullptr);
                }
            }

            layer.clearBacking();
            layerChanged = true;

            // This layer and all of its descendants have cached repaints rects that are relative to
            // the repaint container, so change when compositing changes; we need to update them here.
            layer.computeRepaintRectsIncludingDescendants();

            // If we need to repaint, do so now that we've removed the backing
            if (shouldRepaint == CompositingChangeRepaintNow)
                repaintOnCompositingChange(layer);
        }
    }
    
#if ENABLE(VIDEO)
    if (layerChanged && is<RenderVideo>(layer.renderer())) {
        // If it's a video, give the media player a chance to hook up to the layer.
        downcast<RenderVideo>(layer.renderer()).acceleratedRenderingStateChanged();
    }
#endif

    if (layerChanged && is<RenderWidget>(layer.renderer())) {
        auto* innerCompositor = frameContentsCompositor(downcast<RenderWidget>(layer.renderer()));
        if (innerCompositor && innerCompositor->usesCompositing())
            innerCompositor->updateRootLayerAttachment();
    }
    
    if (layerChanged)
        layer.clearClipRectsIncludingDescendants(PaintingClipRects);

    // If a fixed position layer gained/lost a backing or the reason not compositing it changed,
    // the scrolling coordinator needs to recalculate whether it can do fast scrolling.
    if (layer.renderer().isFixedPositioned()) {
        if (layer.viewportConstrainedNotCompositedReason() != queryData.nonCompositedForPositionReason) {
            layer.setViewportConstrainedNotCompositedReason(queryData.nonCompositedForPositionReason);
            layerChanged = true;
        }
        if (layerChanged) {
            if (auto* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewFixedObjectsDidChange(m_renderView.frameView());
        }
    } else
        layer.setViewportConstrainedNotCompositedReason(RenderLayer::NoNotCompositedReason);
    
    if (layer.backing())
        layer.backing()->updateDebugIndicators(m_showDebugBorders, m_showRepaintCounter);

    return layerChanged;
}

bool RenderLayerCompositor::updateLayerCompositingState(RenderLayer& layer, RequiresCompositingData& queryData, CompositingChangeRepaint shouldRepaint)
{
    bool layerChanged = updateBacking(layer, queryData, shouldRepaint);

    // See if we need content or clipping layers. Methods called here should assume
    // that the compositing state of descendant layers has not been updated yet.
    if (layer.backing() && layer.backing()->updateConfiguration())
        layerChanged = true;

    return layerChanged;
}

void RenderLayerCompositor::repaintOnCompositingChange(RenderLayer& layer)
{
    // If the renderer is not attached yet, no need to repaint.
    if (&layer.renderer() != &m_renderView && !layer.renderer().parent())
        return;

    auto* repaintContainer = layer.renderer().containerForRepaint();
    if (!repaintContainer)
        repaintContainer = &m_renderView;

    layer.repaintIncludingNonCompositingDescendants(repaintContainer);
    if (repaintContainer == &m_renderView) {
        // The contents of this layer may be moving between the window
        // and a GraphicsLayer, so we need to make sure the window system
        // synchronizes those changes on the screen.
        m_renderView.frameView().setNeedsOneShotDrawingSynchronization();
    }
}

// This method assumes that layout is up-to-date, unlike repaintOnCompositingChange().
void RenderLayerCompositor::repaintInCompositedAncestor(RenderLayer& layer, const LayoutRect& rect)
{
    auto* compositedAncestor = layer.enclosingCompositingLayerForRepaint(ExcludeSelf);
    if (!compositedAncestor)
        return;

    ASSERT(compositedAncestor->backing());
    LayoutRect repaintRect = rect;
    repaintRect.move(layer.offsetFromAncestor(compositedAncestor));
    compositedAncestor->setBackingNeedsRepaintInRect(repaintRect);

    // The contents of this layer may be moving from a GraphicsLayer to the window,
    // so we need to make sure the window system synchronizes those changes on the screen.
    if (compositedAncestor->isRenderViewLayer())
        m_renderView.frameView().setNeedsOneShotDrawingSynchronization();
}

// FIXME: remove.
void RenderLayerCompositor::layerWasAdded(RenderLayer&, RenderLayer&)
{
}

void RenderLayerCompositor::layerWillBeRemoved(RenderLayer& parent, RenderLayer& child)
{
    if (!child.isComposited() || parent.renderer().renderTreeBeingDestroyed())
        return;

    repaintInCompositedAncestor(child, child.backing()->compositedBounds()); // FIXME: do via dirty bits?

    child.setNeedsCompositingLayerConnection();
}

RenderLayer* RenderLayerCompositor::enclosingNonStackingClippingLayer(const RenderLayer& layer) const
{
    for (auto* parent = layer.parent(); parent; parent = parent->parent()) {
        if (parent->isStackingContext())
            return nullptr;
        if (parent->renderer().hasClipOrOverflowClip())
            return parent;
    }
    return nullptr;
}

void RenderLayerCompositor::computeExtent(const OverlapMap& overlapMap, const RenderLayer& layer, OverlapExtent& extent) const
{
    if (extent.extentComputed)
        return;

    LayoutRect layerBounds;
    if (extent.hasTransformAnimation)
        extent.animationCausesExtentUncertainty = !layer.getOverlapBoundsIncludingChildrenAccountingForTransformAnimations(layerBounds);
    else
        layerBounds = layer.overlapBounds();
    
    // In the animating transform case, we avoid double-accounting for the transform because
    // we told pushMappingsToAncestor() to ignore transforms earlier.
    extent.bounds = enclosingLayoutRect(overlapMap.geometryMap().absoluteRect(layerBounds));

    // Empty rects never intersect, but we need them to for the purposes of overlap testing.
    if (extent.bounds.isEmpty())
        extent.bounds.setSize(LayoutSize(1, 1));


    RenderLayerModelObject& renderer = layer.renderer();
    if (renderer.isFixedPositioned() && renderer.container() == &m_renderView) {
        // Because fixed elements get moved around without re-computing overlap, we have to compute an overlap
        // rect that covers all the locations that the fixed element could move to.
        // FIXME: need to handle sticky too.
        extent.bounds = m_renderView.frameView().fixedScrollableAreaBoundsInflatedForScrolling(extent.bounds);
    }

    extent.extentComputed = true;
}

void RenderLayerCompositor::addToOverlapMap(OverlapMap& overlapMap, const RenderLayer& layer, OverlapExtent& extent)
{
    if (layer.isRenderViewLayer())
        return;

    computeExtent(overlapMap, layer, extent);

    LayoutRect clipRect = layer.backgroundClipRect(RenderLayer::ClipRectsContext(&rootRenderLayer(), AbsoluteClipRects)).rect(); // FIXME: Incorrect for CSS regions.

    // On iOS, pageScaleFactor() is not applied by RenderView, so we should not scale here.
    if (!m_renderView.settings().delegatesPageScaling())
        clipRect.scale(pageScaleFactor());
    clipRect.intersect(extent.bounds);
    overlapMap.add(clipRect);
}

void RenderLayerCompositor::addToOverlapMapRecursive(OverlapMap& overlapMap, const RenderLayer& layer, const RenderLayer* ancestorLayer)
{
    if (!canBeComposited(layer))
        return;

    // A null ancestorLayer is an indication that 'layer' has already been pushed.
    if (ancestorLayer)
        overlapMap.geometryMap().pushMappingsToAncestor(&layer, ancestorLayer);
    
    OverlapExtent layerExtent;
    addToOverlapMap(overlapMap, layer, layerExtent);

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(const_cast<RenderLayer&>(layer));
#endif

    for (auto* renderLayer : layer.negativeZOrderLayers())
        addToOverlapMapRecursive(overlapMap, *renderLayer, &layer);

    for (auto* renderLayer : layer.normalFlowLayers())
        addToOverlapMapRecursive(overlapMap, *renderLayer, &layer);

    for (auto* renderLayer : layer.positiveZOrderLayers())
        addToOverlapMapRecursive(overlapMap, *renderLayer, &layer);
    
    if (ancestorLayer)
        overlapMap.geometryMap().popMappingsToAncestor(ancestorLayer);
}

#if ENABLE(VIDEO)
bool RenderLayerCompositor::canAccelerateVideoRendering(RenderVideo& video) const
{
    if (!m_hasAcceleratedCompositing)
        return false;

    return video.supportsAcceleratedRendering();
}
#endif

void RenderLayerCompositor::frameViewDidChangeLocation(const IntPoint& contentsOffset)
{
    if (m_overflowControlsHostLayer)
        m_overflowControlsHostLayer->setPosition(contentsOffset);
}

void RenderLayerCompositor::frameViewDidChangeSize()
{
    if (auto* layer = m_renderView.layer())
        layer->setNeedsCompositingGeometryUpdate();

    if (m_scrolledContentsLayer) {
        updateScrollLayerClipping();
        frameViewDidScroll();
        updateOverflowControlsLayers();

#if ENABLE(RUBBER_BANDING)
        if (m_layerForOverhangAreas) {
            auto& frameView = m_renderView.frameView();
            m_layerForOverhangAreas->setSize(frameView.frameRect().size());
            m_layerForOverhangAreas->setPosition(FloatPoint(0, frameView.topContentInset()));
        }
#endif
    }
}

bool RenderLayerCompositor::hasCoordinatedScrolling() const
{
    auto* scrollingCoordinator = this->scrollingCoordinator();
    return scrollingCoordinator && scrollingCoordinator->coordinatesScrollingForFrameView(m_renderView.frameView());
}

void RenderLayerCompositor::updateScrollLayerPosition()
{
    ASSERT(!hasCoordinatedScrolling());
    ASSERT(m_scrolledContentsLayer);

    auto& frameView = m_renderView.frameView();
    IntPoint scrollPosition = frameView.scrollPosition();

    m_scrolledContentsLayer->setPosition(FloatPoint(-scrollPosition.x(), -scrollPosition.y()));

    if (auto* fixedBackgroundLayer = fixedRootBackgroundLayer())
        fixedBackgroundLayer->setPosition(frameView.scrollPositionForFixedPosition());
}

void RenderLayerCompositor::updateScrollLayerClipping()
{
    auto* layerForClipping = this->layerForClipping();
    if (!layerForClipping)
        return;

    layerForClipping->setSize(m_renderView.frameView().sizeForVisibleContent());
    layerForClipping->setPosition(positionForClipLayer());
}

FloatPoint RenderLayerCompositor::positionForClipLayer() const
{
    auto& frameView = m_renderView.frameView();

    return FloatPoint(
        frameView.shouldPlaceBlockDirectionScrollbarOnLeft() ? frameView.horizontalScrollbarIntrusion() : 0,
        FrameView::yPositionForInsetClipLayer(frameView.scrollPosition(), frameView.topContentInset()));
}

void RenderLayerCompositor::frameViewDidScroll()
{
    if (!m_scrolledContentsLayer)
        return;

    // If there's a scrolling coordinator that manages scrolling for this frame view,
    // it will also manage updating the scroll layer position.
    if (hasCoordinatedScrolling()) {
        // We have to schedule a flush in order for the main TiledBacking to update its tile coverage.
        scheduleLayerFlushNow();
        return;
    }

    updateScrollLayerPosition();
}

void RenderLayerCompositor::frameViewDidAddOrRemoveScrollbars()
{
    updateOverflowControlsLayers();
}

void RenderLayerCompositor::frameViewDidLayout()
{
    if (auto* renderViewBacking = m_renderView.layer()->backing())
        renderViewBacking->adjustTiledBackingCoverage();
}

void RenderLayerCompositor::rootLayerConfigurationChanged()
{
    auto* renderViewBacking = m_renderView.layer()->backing();
    if (renderViewBacking && renderViewBacking->isFrameLayerWithTiledBacking()) {
        m_renderView.layer()->setNeedsCompositingConfigurationUpdate();
        scheduleCompositingLayerUpdate();
    }
}

String RenderLayerCompositor::layerTreeAsText(LayerTreeFlags flags)
{
    updateCompositingLayers(CompositingUpdateType::AfterLayout);

    if (!m_rootContentsLayer)
        return String();

    flushPendingLayerChanges(true);

    LayerTreeAsTextBehavior layerTreeBehavior = LayerTreeAsTextBehaviorNormal;
    if (flags & LayerTreeFlagsIncludeDebugInfo)
        layerTreeBehavior |= LayerTreeAsTextDebug;
    if (flags & LayerTreeFlagsIncludeVisibleRects)
        layerTreeBehavior |= LayerTreeAsTextIncludeVisibleRects;
    if (flags & LayerTreeFlagsIncludeTileCaches)
        layerTreeBehavior |= LayerTreeAsTextIncludeTileCaches;
    if (flags & LayerTreeFlagsIncludeRepaintRects)
        layerTreeBehavior |= LayerTreeAsTextIncludeRepaintRects;
    if (flags & LayerTreeFlagsIncludePaintingPhases)
        layerTreeBehavior |= LayerTreeAsTextIncludePaintingPhases;
    if (flags & LayerTreeFlagsIncludeContentLayers)
        layerTreeBehavior |= LayerTreeAsTextIncludeContentLayers;
    if (flags & LayerTreeFlagsIncludeAcceleratesDrawing)
        layerTreeBehavior |= LayerTreeAsTextIncludeAcceleratesDrawing;
    if (flags & LayerTreeFlagsIncludeBackingStoreAttached)
        layerTreeBehavior |= LayerTreeAsTextIncludeBackingStoreAttached;
    if (flags & LayerTreeFlagsIncludeRootLayerProperties)
        layerTreeBehavior |= LayerTreeAsTextIncludeRootLayerProperties;

    // We skip dumping the scroll and clip layers to keep layerTreeAsText output
    // similar between platforms.
    String layerTreeText = m_rootContentsLayer->layerTreeAsText(layerTreeBehavior);

    // Dump an empty layer tree only if the only composited layer is the main frame's tiled backing,
    // so that tests expecting us to drop out of accelerated compositing when there are no layers succeed.
    if (!hasContentCompositingLayers() && documentUsesTiledBacking() && !(layerTreeBehavior & LayerTreeAsTextIncludeTileCaches) && !(layerTreeBehavior & LayerTreeAsTextIncludeRootLayerProperties))
        layerTreeText = emptyString();

    // The true root layer is not included in the dump, so if we want to report
    // its repaint rects, they must be included here.
    if (flags & LayerTreeFlagsIncludeRepaintRects)
        return m_renderView.frameView().trackedRepaintRectsAsText() + layerTreeText;

    return layerTreeText;
}

static RenderView* frameContentsRenderView(RenderWidget& renderer)
{
    if (auto* contentDocument = renderer.frameOwnerElement().contentDocument())
        return contentDocument->renderView();

    return nullptr;
}

RenderLayerCompositor* RenderLayerCompositor::frameContentsCompositor(RenderWidget& renderer)
{
    if (auto* view = frameContentsRenderView(renderer))
        return &view->compositor();

    return nullptr;
}

bool RenderLayerCompositor::parentFrameContentLayers(RenderWidget& renderer)
{
    auto* innerCompositor = frameContentsCompositor(renderer);
    if (!innerCompositor || !innerCompositor->usesCompositing() || innerCompositor->rootLayerAttachment() != RootLayerAttachedViaEnclosingFrame)
        return false;
    
    auto* layer = renderer.layer();
    if (!layer->isComposited())
        return false;

    auto* backing = layer->backing();
    auto* hostingLayer = backing->parentForSublayers();
    auto* rootLayer = innerCompositor->rootGraphicsLayer();
    if (hostingLayer->children().size() != 1 || hostingLayer->children()[0].ptr() != rootLayer) {
        hostingLayer->removeAllChildren();
        hostingLayer->addChild(*rootLayer);
    }

    if (auto frameHostingNodeID = backing->scrollingNodeIDForRole(ScrollCoordinationRole::FrameHosting)) {
        auto* contentsRenderView = frameContentsRenderView(renderer);
        if (auto frameRootScrollingNodeID = contentsRenderView->frameView().scrollingNodeID()) {
            if (auto* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->insertNode(ScrollingNodeType::Subframe, frameRootScrollingNodeID, frameHostingNodeID, 0);
        }
    }

    // FIXME: Why always return true and not just when the layers changed?
    return true;
}

void RenderLayerCompositor::repaintCompositedLayers()
{
    recursiveRepaintLayer(rootRenderLayer());
}

void RenderLayerCompositor::recursiveRepaintLayer(RenderLayer& layer)
{
    layer.updateLayerListsIfNeeded();

    // FIXME: This method does not work correctly with transforms.
    if (layer.isComposited() && !layer.backing()->paintsIntoCompositedAncestor())
        layer.setBackingNeedsRepaint();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(layer);
#endif

    if (layer.hasCompositingDescendant()) {
        for (auto* renderLayer : layer.negativeZOrderLayers())
            recursiveRepaintLayer(*renderLayer);

        for (auto* renderLayer : layer.positiveZOrderLayers())
            recursiveRepaintLayer(*renderLayer);
    }

    for (auto* renderLayer : layer.normalFlowLayers())
        recursiveRepaintLayer(*renderLayer);
}

RenderLayer& RenderLayerCompositor::rootRenderLayer() const
{
    return *m_renderView.layer();
}

GraphicsLayer* RenderLayerCompositor::rootGraphicsLayer() const
{
    if (m_overflowControlsHostLayer)
        return m_overflowControlsHostLayer.get();
    return m_rootContentsLayer.get();
}

void RenderLayerCompositor::setIsInWindow(bool isInWindow)
{
    LOG(Compositing, "RenderLayerCompositor %p setIsInWindow %d", this, isInWindow);

    if (!usesCompositing())
        return;

    if (auto* rootLayer = rootGraphicsLayer()) {
        GraphicsLayer::traverse(*rootLayer, [isInWindow](GraphicsLayer& layer) {
            layer.setIsInWindow(isInWindow);
        });
    }

    if (isInWindow) {
        if (m_rootLayerAttachment != RootLayerUnattached)
            return;

        RootLayerAttachment attachment = isMainFrameCompositor() ? RootLayerAttachedViaChromeClient : RootLayerAttachedViaEnclosingFrame;
        attachRootLayer(attachment);
#if PLATFORM(IOS_FAMILY)
        if (m_legacyScrollingLayerCoordinator) {
            m_legacyScrollingLayerCoordinator->registerAllViewportConstrainedLayers(*this);
            m_legacyScrollingLayerCoordinator->registerAllScrollingLayers();
        }
#endif
    } else {
        if (m_rootLayerAttachment == RootLayerUnattached)
            return;

        detachRootLayer();
#if PLATFORM(IOS_FAMILY)
        if (m_legacyScrollingLayerCoordinator) {
            m_legacyScrollingLayerCoordinator->unregisterAllViewportConstrainedLayers();
            m_legacyScrollingLayerCoordinator->unregisterAllScrollingLayers();
        }
#endif
    }
}

void RenderLayerCompositor::clearBackingForLayerIncludingDescendants(RenderLayer& layer)
{
    if (layer.isComposited())
        layer.clearBacking();

    for (auto* childLayer = layer.firstChild(); childLayer; childLayer = childLayer->nextSibling())
        clearBackingForLayerIncludingDescendants(*childLayer);
}

void RenderLayerCompositor::clearBackingForAllLayers()
{
    clearBackingForLayerIncludingDescendants(*m_renderView.layer());
}

void RenderLayerCompositor::updateRootLayerPosition()
{
    if (m_rootContentsLayer) {
        m_rootContentsLayer->setSize(m_renderView.frameView().contentsSize());
        m_rootContentsLayer->setPosition(m_renderView.frameView().positionForRootContentLayer());
        m_rootContentsLayer->setAnchorPoint(FloatPoint3D());
    }

    updateScrollLayerClipping();

#if ENABLE(RUBBER_BANDING)
    if (m_contentShadowLayer && m_rootContentsLayer) {
        m_contentShadowLayer->setPosition(m_rootContentsLayer->position());
        m_contentShadowLayer->setSize(m_rootContentsLayer->size());
    }

    updateLayerForTopOverhangArea(m_layerForTopOverhangArea != nullptr);
    updateLayerForBottomOverhangArea(m_layerForBottomOverhangArea != nullptr);
    updateLayerForHeader(m_layerForHeader != nullptr);
    updateLayerForFooter(m_layerForFooter != nullptr);
#endif
}

bool RenderLayerCompositor::has3DContent() const
{
    return layerHas3DContent(rootRenderLayer());
}

bool RenderLayerCompositor::needsToBeComposited(const RenderLayer& layer, RequiresCompositingData& queryData) const
{
    if (!canBeComposited(layer))
        return false;

    return requiresCompositingLayer(layer, queryData) || layer.mustCompositeForIndirectReasons() || (usesCompositing() && layer.isRenderViewLayer());
}

// Note: this specifies whether the RL needs a compositing layer for intrinsic reasons.
// Use needsToBeComposited() to determine if a RL actually needs a compositing layer.
// FIXME: is clipsCompositingDescendants() an intrinsic reason?
bool RenderLayerCompositor::requiresCompositingLayer(const RenderLayer& layer, RequiresCompositingData& queryData) const
{
    auto& renderer = rendererForCompositingTests(layer);

    // The root layer always has a compositing layer, but it may not have backing.
    return requiresCompositingForTransform(renderer)
        || requiresCompositingForAnimation(renderer)
        || clipsCompositingDescendants(*renderer.layer())
        || requiresCompositingForPosition(renderer, *renderer.layer(), queryData)
        || requiresCompositingForCanvas(renderer)
        || requiresCompositingForFilters(renderer)
        || requiresCompositingForWillChange(renderer)
        || requiresCompositingForBackfaceVisibility(renderer)
        || requiresCompositingForVideo(renderer)
        || requiresCompositingForFrame(renderer, queryData)
        || requiresCompositingForPlugin(renderer, queryData)
        || requiresCompositingForEditableImage(renderer)
        || requiresCompositingForOverflowScrolling(*renderer.layer(), queryData);
}

bool RenderLayerCompositor::canBeComposited(const RenderLayer& layer) const
{
    if (m_hasAcceleratedCompositing && layer.isSelfPaintingLayer()) {
        if (!layer.isInsideFragmentedFlow())
            return true;

        // CSS Regions flow threads do not need to be composited as we use composited RenderFragmentContainers
        // to render the background of the RenderFragmentedFlow.
        if (layer.isRenderFragmentedFlow())
            return false;

        return true;
    }
    return false;
}

#if ENABLE(FULLSCREEN_API)
enum class FullScreenDescendant { Yes, No, NotApplicable };
static FullScreenDescendant isDescendantOfFullScreenLayer(const RenderLayer& layer)
{
    auto& document = layer.renderer().document();

    if (!document.webkitIsFullScreen() || !document.fullScreenRenderer())
        return FullScreenDescendant::NotApplicable;

    auto* fullScreenLayer = document.fullScreenRenderer()->layer();
    if (!fullScreenLayer) {
        ASSERT_NOT_REACHED();
        return FullScreenDescendant::NotApplicable;
    }

    return layer.isDescendantOf(*fullScreenLayer) ? FullScreenDescendant::Yes : FullScreenDescendant::No;
}
#endif

bool RenderLayerCompositor::requiresOwnBackingStore(const RenderLayer& layer, const RenderLayer* compositingAncestorLayer, const LayoutRect& layerCompositedBoundsInAncestor, const LayoutRect& ancestorCompositedBounds) const
{
    auto& renderer = layer.renderer();

    if (compositingAncestorLayer
        && !(compositingAncestorLayer->backing()->graphicsLayer()->drawsContent()
            || compositingAncestorLayer->backing()->paintsIntoWindow()
            || compositingAncestorLayer->backing()->paintsIntoCompositedAncestor()))
        return true;

    RequiresCompositingData queryData;
    if (layer.isRenderViewLayer()
        || layer.transform() // note: excludes perspective and transformStyle3D.
        || requiresCompositingForAnimation(renderer)
        || requiresCompositingForPosition(renderer, layer, queryData)
        || requiresCompositingForCanvas(renderer)
        || requiresCompositingForFilters(renderer)
        || requiresCompositingForWillChange(renderer)
        || requiresCompositingForBackfaceVisibility(renderer)
        || requiresCompositingForVideo(renderer)
        || requiresCompositingForFrame(renderer, queryData)
        || requiresCompositingForPlugin(renderer, queryData)
        || requiresCompositingForEditableImage(renderer)
        || requiresCompositingForOverflowScrolling(layer, queryData)
        || needsContentsCompositingLayer(layer)
        || renderer.isTransparent()
        || renderer.hasMask()
        || renderer.hasReflection()
        || renderer.hasFilter()
        || renderer.hasBackdropFilter())
        return true;

    if (layer.mustCompositeForIndirectReasons()) {
        RenderLayer::IndirectCompositingReason reason = layer.indirectCompositingReason();
        return reason == RenderLayer::IndirectCompositingReason::Overlap
            || reason == RenderLayer::IndirectCompositingReason::Stacking
            || reason == RenderLayer::IndirectCompositingReason::BackgroundLayer
            || reason == RenderLayer::IndirectCompositingReason::GraphicalEffect
            || reason == RenderLayer::IndirectCompositingReason::Preserve3D; // preserve-3d has to create backing store to ensure that 3d-transformed elements intersect.
    }

    if (!ancestorCompositedBounds.contains(layerCompositedBoundsInAncestor))
        return true;

    return false;
}

OptionSet<CompositingReason> RenderLayerCompositor::reasonsForCompositing(const RenderLayer& layer) const
{
    OptionSet<CompositingReason> reasons;

    if (!layer.isComposited())
        return reasons;

    RequiresCompositingData queryData;

    auto& renderer = rendererForCompositingTests(layer);

    if (requiresCompositingForTransform(renderer))
        reasons.add(CompositingReason::Transform3D);

    if (requiresCompositingForVideo(renderer))
        reasons.add(CompositingReason::Video);
    else if (requiresCompositingForCanvas(renderer))
        reasons.add(CompositingReason::Canvas);
    else if (requiresCompositingForPlugin(renderer, queryData))
        reasons.add(CompositingReason::Plugin);
    else if (requiresCompositingForFrame(renderer, queryData))
        reasons.add(CompositingReason::IFrame);
    else if (requiresCompositingForEditableImage(renderer))
        reasons.add(CompositingReason::EmbeddedView);

    if ((canRender3DTransforms() && renderer.style().backfaceVisibility() == BackfaceVisibility::Hidden))
        reasons.add(CompositingReason::BackfaceVisibilityHidden);

    if (clipsCompositingDescendants(*renderer.layer()))
        reasons.add(CompositingReason::ClipsCompositingDescendants);

    if (requiresCompositingForAnimation(renderer))
        reasons.add(CompositingReason::Animation);

    if (requiresCompositingForFilters(renderer))
        reasons.add(CompositingReason::Filters);

    if (requiresCompositingForWillChange(renderer))
        reasons.add(CompositingReason::WillChange);

    if (requiresCompositingForPosition(renderer, *renderer.layer(), queryData))
        reasons.add(renderer.isFixedPositioned() ? CompositingReason::PositionFixed : CompositingReason::PositionSticky);

    if (requiresCompositingForOverflowScrolling(*renderer.layer(), queryData))
        reasons.add(CompositingReason::OverflowScrollingTouch);

    switch (renderer.layer()->indirectCompositingReason()) {
    case RenderLayer::IndirectCompositingReason::None:
        break;
    case RenderLayer::IndirectCompositingReason::Stacking:
        reasons.add(CompositingReason::Stacking);
        break;
    case RenderLayer::IndirectCompositingReason::Overlap:
        reasons.add(CompositingReason::Overlap);
        break;
    case RenderLayer::IndirectCompositingReason::BackgroundLayer:
        reasons.add(CompositingReason::NegativeZIndexChildren);
        break;
    case RenderLayer::IndirectCompositingReason::GraphicalEffect:
        if (renderer.hasTransform())
            reasons.add(CompositingReason::TransformWithCompositedDescendants);

        if (renderer.isTransparent())
            reasons.add(CompositingReason::OpacityWithCompositedDescendants);

        if (renderer.hasMask())
            reasons.add(CompositingReason::MaskWithCompositedDescendants);

        if (renderer.hasReflection())
            reasons.add(CompositingReason::ReflectionWithCompositedDescendants);

        if (renderer.hasFilter() || renderer.hasBackdropFilter())
            reasons.add(CompositingReason::FilterWithCompositedDescendants);

#if ENABLE(CSS_COMPOSITING)
        if (layer.isolatesCompositedBlending())
            reasons.add(CompositingReason::IsolatesCompositedBlendingDescendants);

        if (layer.hasBlendMode())
            reasons.add(CompositingReason::BlendingWithCompositedDescendants);
#endif
        break;
    case RenderLayer::IndirectCompositingReason::Perspective:
        reasons.add(CompositingReason::Perspective);
        break;
    case RenderLayer::IndirectCompositingReason::Preserve3D:
        reasons.add(CompositingReason::Preserve3D);
        break;
    }

    if (usesCompositing() && renderer.layer()->isRenderViewLayer())
        reasons.add(CompositingReason::Root);

    return reasons;
}

#if !LOG_DISABLED
const char* RenderLayerCompositor::logReasonsForCompositing(const RenderLayer& layer)
{
    OptionSet<CompositingReason> reasons = reasonsForCompositing(layer);

    if (reasons & CompositingReason::Transform3D)
        return "3D transform";

    if (reasons & CompositingReason::Video)
        return "video";

    if (reasons & CompositingReason::Canvas)
        return "canvas";

    if (reasons & CompositingReason::Plugin)
        return "plugin";

    if (reasons & CompositingReason::IFrame)
        return "iframe";

    if (reasons & CompositingReason::BackfaceVisibilityHidden)
        return "backface-visibility: hidden";

    if (reasons & CompositingReason::ClipsCompositingDescendants)
        return "clips compositing descendants";

    if (reasons & CompositingReason::Animation)
        return "animation";

    if (reasons & CompositingReason::Filters)
        return "filters";

    if (reasons & CompositingReason::PositionFixed)
        return "position: fixed";

    if (reasons & CompositingReason::PositionSticky)
        return "position: sticky";

    if (reasons & CompositingReason::OverflowScrollingTouch)
        return "-webkit-overflow-scrolling: touch";

    if (reasons & CompositingReason::Stacking)
        return "stacking";

    if (reasons & CompositingReason::Overlap)
        return "overlap";

    if (reasons & CompositingReason::NegativeZIndexChildren)
        return "negative z-index children";

    if (reasons & CompositingReason::TransformWithCompositedDescendants)
        return "transform with composited descendants";

    if (reasons & CompositingReason::OpacityWithCompositedDescendants)
        return "opacity with composited descendants";

    if (reasons & CompositingReason::MaskWithCompositedDescendants)
        return "mask with composited descendants";

    if (reasons & CompositingReason::ReflectionWithCompositedDescendants)
        return "reflection with composited descendants";

    if (reasons & CompositingReason::FilterWithCompositedDescendants)
        return "filter with composited descendants";

#if ENABLE(CSS_COMPOSITING)
    if (reasons & CompositingReason::BlendingWithCompositedDescendants)
        return "blending with composited descendants";

    if (reasons & CompositingReason::IsolatesCompositedBlendingDescendants)
        return "isolates composited blending descendants";
#endif

    if (reasons & CompositingReason::Perspective)
        return "perspective";

    if (reasons & CompositingReason::Preserve3D)
        return "preserve-3d";

    if (reasons & CompositingReason::Root)
        return "root";

    return "";
}
#endif

// Return true if the given layer has some ancestor in the RenderLayer hierarchy that clips,
// up to the enclosing compositing ancestor. This is required because compositing layers are parented
// according to the z-order hierarchy, yet clipping goes down the renderer hierarchy.
// Thus, a RenderLayer can be clipped by a RenderLayer that is an ancestor in the renderer hierarchy,
// but a sibling in the z-order hierarchy.
// FIXME: can we do this without a tree walk?
bool RenderLayerCompositor::clippedByAncestor(RenderLayer& layer) const
{
    ASSERT(layer.isComposited());
    if (!layer.parent())
        return false;

    // On first pass in WK1, the root may not have become composited yet.
    auto* compositingAncestor = layer.ancestorCompositingLayer();
    if (!compositingAncestor)
        return false;

    // If the compositingAncestor clips, that will be taken care of by clipsCompositingDescendants(),
    // so we only care about clipping between its first child that is our ancestor (the computeClipRoot),
    // and layer. The exception is when the compositingAncestor isolates composited blending children,
    // in this case it is not allowed to clipsCompositingDescendants() and each of its children
    // will be clippedByAncestor()s, including the compositingAncestor.
    auto* computeClipRoot = compositingAncestor;
    if (!compositingAncestor->isolatesCompositedBlending()) {
        computeClipRoot = nullptr;
        auto* parent = &layer;
        while (parent) {
            auto* next = parent->parent();
            if (next == compositingAncestor) {
                computeClipRoot = parent;
                break;
            }
            parent = next;
        }

        if (!computeClipRoot || computeClipRoot == &layer)
            return false;
    }

    return !layer.backgroundClipRect(RenderLayer::ClipRectsContext(computeClipRoot, TemporaryClipRects)).isInfinite(); // FIXME: Incorrect for CSS regions.
}

// Return true if the given layer is a stacking context and has compositing child
// layers that it needs to clip. In this case we insert a clipping GraphicsLayer
// into the hierarchy between this layer and its children in the z-order hierarchy.
bool RenderLayerCompositor::clipsCompositingDescendants(const RenderLayer& layer) const
{
    return layer.hasCompositingDescendant() && layer.renderer().hasClipOrOverflowClip() && !layer.isolatesCompositedBlending();
}

bool RenderLayerCompositor::requiresCompositingForAnimation(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::AnimationTrigger))
        return false;

    if (auto* element = renderer.element()) {
        if (auto* timeline = element->document().existingTimeline()) {
            if (timeline->runningAnimationsForElementAreAllAccelerated(*element))
                return true;
        }
    }

    if (RuntimeEnabledFeatures::sharedFeatures().webAnimationsCSSIntegrationEnabled())
        return false;

    auto& animController = renderer.animation();
    return (animController.isRunningAnimationOnRenderer(renderer, CSSPropertyOpacity)
        && (usesCompositing() || (m_compositingTriggers & ChromeClient::AnimatedOpacityTrigger)))
        || animController.isRunningAnimationOnRenderer(renderer, CSSPropertyFilter)
#if ENABLE(FILTERS_LEVEL_2)
        || animController.isRunningAnimationOnRenderer(renderer, CSSPropertyWebkitBackdropFilter)
#endif
        || animController.isRunningAnimationOnRenderer(renderer, CSSPropertyTransform);
}

bool RenderLayerCompositor::requiresCompositingForTransform(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::ThreeDTransformTrigger))
        return false;

    // Note that we ask the renderer if it has a transform, because the style may have transforms,
    // but the renderer may be an inline that doesn't suppport them.
    if (!renderer.hasTransform())
        return false;
    
    switch (m_compositingPolicy) {
    case CompositingPolicy::Normal:
        return renderer.style().transform().has3DOperation();
    case CompositingPolicy::Conservative:
        // Continue to allow pages to avoid the very slow software filter path.
        if (renderer.style().transform().has3DOperation() && renderer.hasFilter())
            return true;
        return renderer.style().transform().isRepresentableIn2D() ? false : true;
    }
    return false;
}

bool RenderLayerCompositor::requiresCompositingForBackfaceVisibility(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::ThreeDTransformTrigger))
        return false;

    if (renderer.style().backfaceVisibility() != BackfaceVisibility::Hidden)
        return false;

    if (renderer.layer()->has3DTransformedAncestor())
        return true;
    
    // FIXME: workaround for webkit.org/b/132801
    auto* stackingContext = renderer.layer()->stackingContext();
    if (stackingContext && stackingContext->renderer().style().transformStyle3D() == TransformStyle3D::Preserve3D)
        return true;

    return false;
}

bool RenderLayerCompositor::requiresCompositingForVideo(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::VideoTrigger))
        return false;

#if ENABLE(VIDEO)
    if (!is<RenderVideo>(renderer))
        return false;

    auto& video = downcast<RenderVideo>(renderer);
    if ((video.requiresImmediateCompositing() || video.shouldDisplayVideo()) && canAccelerateVideoRendering(video))
        return true;
#else
    UNUSED_PARAM(renderer);
#endif
    return false;
}

bool RenderLayerCompositor::requiresCompositingForCanvas(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::CanvasTrigger))
        return false;

    if (!renderer.isCanvas())
        return false;

    bool isCanvasLargeEnoughToForceCompositing = true;
#if !USE(COMPOSITING_FOR_SMALL_CANVASES)
    auto* canvas = downcast<HTMLCanvasElement>(renderer.element());
    auto canvasArea = canvas->size().area<RecordOverflow>();
    isCanvasLargeEnoughToForceCompositing = !canvasArea.hasOverflowed() && canvasArea.unsafeGet() >= canvasAreaThresholdRequiringCompositing;
#endif

    CanvasCompositingStrategy compositingStrategy = canvasCompositingStrategy(renderer);
    if (compositingStrategy == CanvasAsLayerContents)
        return true;

    if (m_compositingPolicy == CompositingPolicy::Normal)
        return compositingStrategy == CanvasPaintedToLayer && isCanvasLargeEnoughToForceCompositing;

    return false;
}

bool RenderLayerCompositor::requiresCompositingForFilters(RenderLayerModelObject& renderer) const
{
#if ENABLE(FILTERS_LEVEL_2)
    if (renderer.hasBackdropFilter())
        return true;
#endif

    if (!(m_compositingTriggers & ChromeClient::FilterTrigger))
        return false;

    return renderer.hasFilter();
}

bool RenderLayerCompositor::requiresCompositingForWillChange(RenderLayerModelObject& renderer) const
{
    if (!renderer.style().willChange() || !renderer.style().willChange()->canTriggerCompositing())
        return false;

#if ENABLE(FULLSCREEN_API)
    // FIXME: does this require layout?
    if (renderer.layer() && isDescendantOfFullScreenLayer(*renderer.layer()) == FullScreenDescendant::No)
        return false;
#endif

    if (m_compositingPolicy == CompositingPolicy::Conservative)
        return false;

    if (is<RenderBox>(renderer))
        return true;

    return renderer.style().willChange()->canTriggerCompositingOnInline();
}

bool RenderLayerCompositor::requiresCompositingForPlugin(RenderLayerModelObject& renderer, RequiresCompositingData& queryData) const
{
    if (!(m_compositingTriggers & ChromeClient::PluginTrigger))
        return false;

    bool isCompositedPlugin = is<RenderEmbeddedObject>(renderer) && downcast<RenderEmbeddedObject>(renderer).allowsAcceleratedCompositing();
    if (!isCompositedPlugin)
        return false;

    auto& pluginRenderer = downcast<RenderWidget>(renderer);
    if (pluginRenderer.style().visibility() != Visibility::Visible)
        return false;

    // If we can't reliably know the size of the plugin yet, don't change compositing state.
    if (queryData.layoutUpToDate == LayoutUpToDate::No) {
        queryData.reevaluateAfterLayout = true;
        return pluginRenderer.isComposited();
    }

    // Don't go into compositing mode if height or width are zero, or size is 1x1.
    IntRect contentBox = snappedIntRect(pluginRenderer.contentBoxRect());
    return (contentBox.height() * contentBox.width() > 1);
}
    
bool RenderLayerCompositor::requiresCompositingForEditableImage(RenderLayerModelObject& renderer) const
{
    if (!renderer.isRenderImage())
        return false;

    auto& image = downcast<RenderImage>(renderer);
    if (!image.isEditableImage())
        return false;

    return true;
}

bool RenderLayerCompositor::requiresCompositingForFrame(RenderLayerModelObject& renderer, RequiresCompositingData& queryData) const
{
    if (!is<RenderWidget>(renderer))
        return false;

    auto& frameRenderer = downcast<RenderWidget>(renderer);
    if (frameRenderer.style().visibility() != Visibility::Visible)
        return false;

    if (!frameRenderer.requiresAcceleratedCompositing())
        return false;

    if (queryData.layoutUpToDate == LayoutUpToDate::No) {
        queryData.reevaluateAfterLayout = true;
        return frameRenderer.isComposited();
    }

    // Don't go into compositing mode if height or width are zero.
    return !snappedIntRect(frameRenderer.contentBoxRect()).isEmpty();
}

bool RenderLayerCompositor::requiresCompositingForScrollableFrame(RequiresCompositingData& queryData) const
{
    if (isMainFrameCompositor())
        return false;

#if PLATFORM(MAC) || PLATFORM(IOS_FAMILY)
    if (!m_renderView.settings().asyncFrameScrollingEnabled())
        return false;
#endif

    if (!(m_compositingTriggers & ChromeClient::ScrollableNonMainFrameTrigger))
        return false;

    if (queryData.layoutUpToDate == LayoutUpToDate::No) {
        queryData.reevaluateAfterLayout = true;
        return m_renderView.isComposited();
    }

    return m_renderView.frameView().isScrollable();
}

bool RenderLayerCompositor::requiresCompositingForPosition(RenderLayerModelObject& renderer, const RenderLayer& layer, RequiresCompositingData& queryData) const
{
    // position:fixed elements that create their own stacking context (e.g. have an explicit z-index,
    // opacity, transform) can get their own composited layer. A stacking context is required otherwise
    // z-index and clipping will be broken.
    if (!renderer.isPositioned())
        return false;
    
#if ENABLE(FULLSCREEN_API)
    if (isDescendantOfFullScreenLayer(layer) == FullScreenDescendant::No)
        return false;
#endif

    auto position = renderer.style().position();
    bool isFixed = renderer.isOutOfFlowPositioned() && position == PositionType::Fixed;
    if (isFixed && !layer.isStackingContext())
        return false;
    
    bool isSticky = renderer.isInFlowPositioned() && position == PositionType::Sticky;
    if (!isFixed && !isSticky)
        return false;

    // FIXME: acceleratedCompositingForFixedPositionEnabled should probably be renamed acceleratedCompositingForViewportConstrainedPositionEnabled().
    if (!m_renderView.settings().acceleratedCompositingForFixedPositionEnabled())
        return false;

    if (isSticky)
        return isAsyncScrollableStickyLayer(layer);

    if (queryData.layoutUpToDate == LayoutUpToDate::No) {
        queryData.reevaluateAfterLayout = true;
        return layer.isComposited();
    }

    auto container = renderer.container();
    ASSERT(container);

    // Don't promote fixed position elements that are descendants of a non-view container, e.g. transformed elements.
    // They will stay fixed wrt the container rather than the enclosing frame.j
    if (container != &m_renderView) {
        queryData.nonCompositedForPositionReason = RenderLayer::NotCompositedForNonViewContainer;
        return false;
    }

    bool paintsContent = layer.isVisuallyNonEmpty() || layer.hasVisibleDescendant();
    if (!paintsContent) {
        queryData.nonCompositedForPositionReason = RenderLayer::NotCompositedForNoVisibleContent;
        return false;
    }

    bool intersectsViewport = fixedLayerIntersectsViewport(layer);
    if (!intersectsViewport) {
        queryData.nonCompositedForPositionReason = RenderLayer::NotCompositedForBoundsOutOfView;
        LOG_WITH_STREAM(Compositing, stream << "Layer " << &layer << " is outside the viewport");
        return false;
    }

    return true;
}

bool RenderLayerCompositor::requiresCompositingForOverflowScrolling(const RenderLayer& layer, RequiresCompositingData& queryData) const
{
    if (!layer.canUseCompositedScrolling())
        return false;

    if (queryData.layoutUpToDate == LayoutUpToDate::No) {
        queryData.reevaluateAfterLayout = true;
        return layer.isComposited();
    }

    return layer.hasCompositedScrollableOverflow();
}

// FIXME: why doesn't this handle the clipping cases?
bool RenderLayerCompositor::requiresCompositingForIndirectReason(RenderLayerModelObject& renderer, bool hasCompositedDescendants, bool has3DTransformedDescendants, RenderLayer::IndirectCompositingReason& reason) const
{
    auto& layer = *downcast<RenderBoxModelObject>(renderer).layer();

    // When a layer has composited descendants, some effects, like 2d transforms, filters, masks etc must be implemented
    // via compositing so that they also apply to those composited descendants.
    if (hasCompositedDescendants && (layer.isolatesCompositedBlending() || layer.transform() || renderer.createsGroup() || renderer.hasReflection())) {
        reason = RenderLayer::IndirectCompositingReason::GraphicalEffect;
        return true;
    }

    // A layer with preserve-3d or perspective only needs to be composited if there are descendant layers that
    // will be affected by the preserve-3d or perspective.
    if (has3DTransformedDescendants) {
        if (renderer.style().transformStyle3D() == TransformStyle3D::Preserve3D) {
            reason = RenderLayer::IndirectCompositingReason::Preserve3D;
            return true;
        }
    
        if (renderer.style().hasPerspective()) {
            reason = RenderLayer::IndirectCompositingReason::Perspective;
            return true;
        }
    }

    reason = RenderLayer::IndirectCompositingReason::None;
    return false;
}

bool RenderLayerCompositor::styleChangeMayAffectIndirectCompositingReasons(const RenderStyle& oldStyle, const RenderStyle& newStyle)
{
    if (RenderElement::createsGroupForStyle(newStyle) != RenderElement::createsGroupForStyle(oldStyle))
        return true;
    if (newStyle.isolation() != oldStyle.isolation())
        return true;
    if (newStyle.hasTransform() != oldStyle.hasTransform())
        return true;
    if (newStyle.boxReflect() != oldStyle.boxReflect())
        return true;
    if (newStyle.transformStyle3D() != oldStyle.transformStyle3D())
        return true;
    if (newStyle.hasPerspective() != oldStyle.hasPerspective())
        return true;

    return false;
}

bool RenderLayerCompositor::isAsyncScrollableStickyLayer(const RenderLayer& layer, const RenderLayer** enclosingAcceleratedOverflowLayer) const
{
    ASSERT(layer.renderer().isStickilyPositioned());

    auto* enclosingOverflowLayer = layer.enclosingOverflowClipLayer(ExcludeSelf);

#if PLATFORM(IOS_FAMILY)
    if (enclosingOverflowLayer && enclosingOverflowLayer->hasCompositedScrollableOverflow()) {
        if (enclosingAcceleratedOverflowLayer)
            *enclosingAcceleratedOverflowLayer = enclosingOverflowLayer;
        return true;
    }
#else
    UNUSED_PARAM(enclosingAcceleratedOverflowLayer);
#endif
    // If the layer is inside normal overflow, it's not async-scrollable.
    if (enclosingOverflowLayer)
        return false;

    // No overflow ancestor, so see if the frame supports async scrolling.
    if (hasCoordinatedScrolling())
        return true;

#if PLATFORM(IOS_FAMILY)
    // iOS WK1 has fixed/sticky support in the main frame via WebFixedPositionContent.
    return isMainFrameCompositor();
#else
    return false;
#endif
}

bool RenderLayerCompositor::isViewportConstrainedFixedOrStickyLayer(const RenderLayer& layer) const
{
    if (layer.renderer().isStickilyPositioned())
        return isAsyncScrollableStickyLayer(layer);

    if (layer.renderer().style().position() != PositionType::Fixed)
        return false;

    // FIXME: Handle fixed inside of a transform, which should not behave as fixed.
    for (auto* stackingContext = layer.stackingContext(); stackingContext; stackingContext = stackingContext->stackingContext()) {
        if (stackingContext->isComposited() && stackingContext->renderer().isFixedPositioned())
            return false;
    }

    return true;
}

bool RenderLayerCompositor::fixedLayerIntersectsViewport(const RenderLayer& layer) const
{
    ASSERT(layer.renderer().style().position() == PositionType::Fixed);

    // Fixed position elements that are invisible in the current view don't get their own layer.
    // FIXME: We shouldn't have to check useFixedLayout() here; one of the viewport rects needs to give the correct answer.
    LayoutRect viewBounds;
    if (m_renderView.frameView().useFixedLayout())
        viewBounds = m_renderView.unscaledDocumentRect();
    else
        viewBounds = m_renderView.frameView().rectForFixedPositionLayout();

    LayoutRect layerBounds = layer.calculateLayerBounds(&layer, LayoutSize(), { RenderLayer::UseLocalClipRectIfPossible, RenderLayer::IncludeFilterOutsets, RenderLayer::UseFragmentBoxesExcludingCompositing,
        RenderLayer::ExcludeHiddenDescendants, RenderLayer::DontConstrainForMask, RenderLayer::IncludeCompositedDescendants });
    // Map to m_renderView to ignore page scale.
    FloatRect absoluteBounds = layer.renderer().localToContainerQuad(FloatRect(layerBounds), &m_renderView).boundingBox();
    return viewBounds.intersects(enclosingIntRect(absoluteBounds));
}

bool RenderLayerCompositor::useCoordinatedScrollingForLayer(const RenderLayer& layer) const
{
    if (layer.isRenderViewLayer() && hasCoordinatedScrolling())
        return true;

    if (auto* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->coordinatesScrollingForOverflowLayer(layer);

    return false;
}

bool RenderLayerCompositor::isLayerForIFrameWithScrollCoordinatedContents(const RenderLayer& layer) const
{
    if (!is<RenderWidget>(layer.renderer()))
        return false;

    auto* contentDocument = downcast<RenderWidget>(layer.renderer()).frameOwnerElement().contentDocument();
    if (!contentDocument)
        return false;

    auto* view = contentDocument->renderView();
    if (!view)
        return false;

    if (auto* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->coordinatesScrollingForFrameView(view->frameView());

    return false;
}

bool RenderLayerCompositor::isRunningTransformAnimation(RenderLayerModelObject& renderer) const
{
    if (!(m_compositingTriggers & ChromeClient::AnimationTrigger))
        return false;

    if (RuntimeEnabledFeatures::sharedFeatures().webAnimationsCSSIntegrationEnabled()) {
        if (auto* element = renderer.element()) {
            if (auto* timeline = element->document().existingTimeline())
                return timeline->isRunningAnimationOnRenderer(renderer, CSSPropertyTransform);
        }
        return false;
    }
    return renderer.animation().isRunningAnimationOnRenderer(renderer, CSSPropertyTransform);
}

// If an element has negative z-index children, those children render in front of the 
// layer background, so we need an extra 'contents' layer for the foreground of the layer object.
bool RenderLayerCompositor::needsContentsCompositingLayer(const RenderLayer& layer) const
{
    return layer.hasNegativeZOrderLayers();
}

bool RenderLayerCompositor::requiresScrollLayer(RootLayerAttachment attachment) const
{
    auto& frameView = m_renderView.frameView();

    // This applies when the application UI handles scrolling, in which case RenderLayerCompositor doesn't need to manage it.
    if (frameView.delegatesScrolling() && isMainFrameCompositor())
        return false;

    // We need to handle our own scrolling if we're:
    return !m_renderView.frameView().platformWidget() // viewless (i.e. non-Mac, or Mac in WebKit2)
        || attachment == RootLayerAttachedViaEnclosingFrame; // a composited frame on Mac
}

void paintScrollbar(Scrollbar* scrollbar, GraphicsContext& context, const IntRect& clip)
{
    if (!scrollbar)
        return;

    context.save();
    const IntRect& scrollbarRect = scrollbar->frameRect();
    context.translate(-scrollbarRect.location());
    IntRect transformedClip = clip;
    transformedClip.moveBy(scrollbarRect.location());
    scrollbar->paint(context, transformedClip);
    context.restore();
}

void RenderLayerCompositor::paintContents(const GraphicsLayer* graphicsLayer, GraphicsContext& context, GraphicsLayerPaintingPhase, const FloatRect& clip, GraphicsLayerPaintBehavior)
{
#if PLATFORM(MAC)
    LocalDefaultSystemAppearance localAppearance(m_renderView.useDarkAppearance());
#endif

    IntRect pixelSnappedRectForIntegralPositionedItems = snappedIntRect(LayoutRect(clip));
    if (graphicsLayer == layerForHorizontalScrollbar())
        paintScrollbar(m_renderView.frameView().horizontalScrollbar(), context, pixelSnappedRectForIntegralPositionedItems);
    else if (graphicsLayer == layerForVerticalScrollbar())
        paintScrollbar(m_renderView.frameView().verticalScrollbar(), context, pixelSnappedRectForIntegralPositionedItems);
    else if (graphicsLayer == layerForScrollCorner()) {
        const IntRect& scrollCorner = m_renderView.frameView().scrollCornerRect();
        context.save();
        context.translate(-scrollCorner.location());
        IntRect transformedClip = pixelSnappedRectForIntegralPositionedItems;
        transformedClip.moveBy(scrollCorner.location());
        m_renderView.frameView().paintScrollCorner(context, transformedClip);
        context.restore();
    }
}

bool RenderLayerCompositor::supportsFixedRootBackgroundCompositing() const
{
    auto* renderViewBacking = m_renderView.layer()->backing();
    return renderViewBacking && renderViewBacking->isFrameLayerWithTiledBacking();
}

bool RenderLayerCompositor::needsFixedRootBackgroundLayer(const RenderLayer& layer) const
{
    if (!layer.isRenderViewLayer())
        return false;

    if (m_renderView.settings().fixedBackgroundsPaintRelativeToDocument())
        return false;

    return supportsFixedRootBackgroundCompositing() && m_renderView.rootBackgroundIsEntirelyFixed();
}

GraphicsLayer* RenderLayerCompositor::fixedRootBackgroundLayer() const
{
    // Get the fixed root background from the RenderView layer's backing.
    auto* viewLayer = m_renderView.layer();
    if (!viewLayer)
        return nullptr;

    if (viewLayer->isComposited() && viewLayer->backing()->backgroundLayerPaintsFixedRootBackground())
        return viewLayer->backing()->backgroundLayer();

    return nullptr;
}

void RenderLayerCompositor::resetTrackedRepaintRects()
{
    if (auto* rootLayer = rootGraphicsLayer()) {
        GraphicsLayer::traverse(*rootLayer, [](GraphicsLayer& layer) {
            layer.resetTrackedRepaints();
        });
    }
}

float RenderLayerCompositor::deviceScaleFactor() const
{
    return m_renderView.document().deviceScaleFactor();
}

float RenderLayerCompositor::pageScaleFactor() const
{
    return page().pageScaleFactor();
}

float RenderLayerCompositor::zoomedOutPageScaleFactor() const
{
    return page().zoomedOutPageScaleFactor();
}

float RenderLayerCompositor::contentsScaleMultiplierForNewTiles(const GraphicsLayer*) const
{
#if PLATFORM(IOS_FAMILY)
    LegacyTileCache* tileCache = nullptr;
    if (auto* frameView = page().mainFrame().view())
        tileCache = frameView->legacyTileCache();

    if (!tileCache)
        return 1;

    return tileCache->tileControllerShouldUseLowScaleTiles() ? 0.125 : 1;
#else
    return 1;
#endif
}

bool RenderLayerCompositor::documentUsesTiledBacking() const
{
    auto* layer = m_renderView.layer();
    if (!layer)
        return false;

    auto* backing = layer->backing();
    if (!backing)
        return false;

    return backing->isFrameLayerWithTiledBacking();
}

bool RenderLayerCompositor::isMainFrameCompositor() const
{
    return m_renderView.frameView().frame().isMainFrame();
}

bool RenderLayerCompositor::shouldCompositeOverflowControls() const
{
    auto& frameView = m_renderView.frameView();

    if (!frameView.managesScrollbars())
        return false;

    if (documentUsesTiledBacking())
        return true;

    if (m_overflowControlsHostLayer && isMainFrameCompositor())
        return true;

#if !USE(COORDINATED_GRAPHICS)
    if (!frameView.hasOverlayScrollbars())
        return false;
#endif

    return true;
}

bool RenderLayerCompositor::requiresHorizontalScrollbarLayer() const
{
    return shouldCompositeOverflowControls() && m_renderView.frameView().horizontalScrollbar();
}

bool RenderLayerCompositor::requiresVerticalScrollbarLayer() const
{
    return shouldCompositeOverflowControls() && m_renderView.frameView().verticalScrollbar();
}

bool RenderLayerCompositor::requiresScrollCornerLayer() const
{
    return shouldCompositeOverflowControls() && m_renderView.frameView().isScrollCornerVisible();
}

#if ENABLE(RUBBER_BANDING)
bool RenderLayerCompositor::requiresOverhangAreasLayer() const
{
    if (!isMainFrameCompositor())
        return false;

    // We do want a layer if we're using tiled drawing and can scroll.
    if (documentUsesTiledBacking() && m_renderView.frameView().hasOpaqueBackground() && !m_renderView.frameView().prohibitsScrolling())
        return true;

    return false;
}

bool RenderLayerCompositor::requiresContentShadowLayer() const
{
    if (!isMainFrameCompositor())
        return false;

#if PLATFORM(COCOA)
    if (viewHasTransparentBackground())
        return false;

    // If the background is going to extend, then it doesn't make sense to have a shadow layer.
    if (m_renderView.settings().backgroundShouldExtendBeyondPage())
        return false;

    // On Mac, we want a content shadow layer if we're using tiled drawing and can scroll.
    if (documentUsesTiledBacking() && !m_renderView.frameView().prohibitsScrolling())
        return true;
#endif

    return false;
}

GraphicsLayer* RenderLayerCompositor::updateLayerForTopOverhangArea(bool wantsLayer)
{
    if (!isMainFrameCompositor())
        return nullptr;

    if (!wantsLayer) {
        GraphicsLayer::unparentAndClear(m_layerForTopOverhangArea);
        return nullptr;
    }

    if (!m_layerForTopOverhangArea) {
        m_layerForTopOverhangArea = GraphicsLayer::create(graphicsLayerFactory(), *this);
        m_layerForTopOverhangArea->setName("top overhang");
        m_scrolledContentsLayer->addChildBelow(*m_layerForTopOverhangArea, m_rootContentsLayer.get());
    }

    return m_layerForTopOverhangArea.get();
}

GraphicsLayer* RenderLayerCompositor::updateLayerForBottomOverhangArea(bool wantsLayer)
{
    if (!isMainFrameCompositor())
        return nullptr;

    if (!wantsLayer) {
        GraphicsLayer::unparentAndClear(m_layerForBottomOverhangArea);
        return nullptr;
    }

    if (!m_layerForBottomOverhangArea) {
        m_layerForBottomOverhangArea = GraphicsLayer::create(graphicsLayerFactory(), *this);
        m_layerForBottomOverhangArea->setName("bottom overhang");
        m_scrolledContentsLayer->addChildBelow(*m_layerForBottomOverhangArea, m_rootContentsLayer.get());
    }

    m_layerForBottomOverhangArea->setPosition(FloatPoint(0, m_rootContentsLayer->size().height() + m_renderView.frameView().headerHeight()
        + m_renderView.frameView().footerHeight() + m_renderView.frameView().topContentInset()));
    return m_layerForBottomOverhangArea.get();
}

GraphicsLayer* RenderLayerCompositor::updateLayerForHeader(bool wantsLayer)
{
    if (!isMainFrameCompositor())
        return nullptr;

    if (!wantsLayer) {
        if (m_layerForHeader) {
            GraphicsLayer::unparentAndClear(m_layerForHeader);

            // The ScrollingTree knows about the header layer, and the position of the root layer is affected
            // by the header layer, so if we remove the header, we need to tell the scrolling tree.
            if (auto* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewRootLayerDidChange(m_renderView.frameView());
        }
        return nullptr;
    }

    if (!m_layerForHeader) {
        m_layerForHeader = GraphicsLayer::create(graphicsLayerFactory(), *this);
        m_layerForHeader->setName("header");
        m_scrolledContentsLayer->addChildAbove(*m_layerForHeader, m_rootContentsLayer.get());
        m_renderView.frameView().addPaintPendingMilestones(DidFirstFlushForHeaderLayer);
    }

    m_layerForHeader->setPosition(FloatPoint(0,
        FrameView::yPositionForHeaderLayer(m_renderView.frameView().scrollPosition(), m_renderView.frameView().topContentInset())));
    m_layerForHeader->setAnchorPoint(FloatPoint3D());
    m_layerForHeader->setSize(FloatSize(m_renderView.frameView().visibleWidth(), m_renderView.frameView().headerHeight()));

    if (auto* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->frameViewRootLayerDidChange(m_renderView.frameView());

    page().chrome().client().didAddHeaderLayer(*m_layerForHeader);

    return m_layerForHeader.get();
}

GraphicsLayer* RenderLayerCompositor::updateLayerForFooter(bool wantsLayer)
{
    if (!isMainFrameCompositor())
        return nullptr;

    if (!wantsLayer) {
        if (m_layerForFooter) {
            GraphicsLayer::unparentAndClear(m_layerForFooter);

            // The ScrollingTree knows about the footer layer, and the total scrollable size is affected
            // by the footer layer, so if we remove the footer, we need to tell the scrolling tree.
            if (auto* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->frameViewRootLayerDidChange(m_renderView.frameView());
        }
        return nullptr;
    }

    if (!m_layerForFooter) {
        m_layerForFooter = GraphicsLayer::create(graphicsLayerFactory(), *this);
        m_layerForFooter->setName("footer");
        m_scrolledContentsLayer->addChildAbove(*m_layerForFooter, m_rootContentsLayer.get());
    }

    float totalContentHeight = m_rootContentsLayer->size().height() + m_renderView.frameView().headerHeight() + m_renderView.frameView().footerHeight();
    m_layerForFooter->setPosition(FloatPoint(0, FrameView::yPositionForFooterLayer(m_renderView.frameView().scrollPosition(),
        m_renderView.frameView().topContentInset(), totalContentHeight, m_renderView.frameView().footerHeight())));
    m_layerForFooter->setAnchorPoint(FloatPoint3D());
    m_layerForFooter->setSize(FloatSize(m_renderView.frameView().visibleWidth(), m_renderView.frameView().footerHeight()));

    if (auto* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->frameViewRootLayerDidChange(m_renderView.frameView());

    page().chrome().client().didAddFooterLayer(*m_layerForFooter);

    return m_layerForFooter.get();
}

#endif

bool RenderLayerCompositor::viewHasTransparentBackground(Color* backgroundColor) const
{
    if (m_renderView.frameView().isTransparent()) {
        if (backgroundColor)
            *backgroundColor = Color(); // Return an invalid color.
        return true;
    }

    Color documentBackgroundColor = m_renderView.frameView().documentBackgroundColor();
    if (!documentBackgroundColor.isValid())
        documentBackgroundColor = m_renderView.frameView().baseBackgroundColor();

    ASSERT(documentBackgroundColor.isValid());

    if (backgroundColor)
        *backgroundColor = documentBackgroundColor;

    return !documentBackgroundColor.isOpaque();
}

// We can't rely on getting layerStyleChanged() for a style change that affects the root background, because the style change may
// be on the body which has no RenderLayer.
void RenderLayerCompositor::rootOrBodyStyleChanged(RenderElement& renderer, const RenderStyle* oldStyle)
{
    if (!usesCompositing())
        return;

    Color oldBackgroundColor;
    if (oldStyle)
        oldBackgroundColor = oldStyle->visitedDependentColorWithColorFilter(CSSPropertyBackgroundColor);

    if (oldBackgroundColor != renderer.style().visitedDependentColorWithColorFilter(CSSPropertyBackgroundColor))
        rootBackgroundColorOrTransparencyChanged();

    bool hadFixedBackground = oldStyle && oldStyle->hasEntirelyFixedBackground();
    if (hadFixedBackground != renderer.style().hasEntirelyFixedBackground())
        rootLayerConfigurationChanged();
}

void RenderLayerCompositor::rootBackgroundColorOrTransparencyChanged()
{
    if (!usesCompositing())
        return;

    Color backgroundColor;
    bool isTransparent = viewHasTransparentBackground(&backgroundColor);
    
    Color extendedBackgroundColor = m_renderView.settings().backgroundShouldExtendBeyondPage() ? backgroundColor : Color();
    
    bool transparencyChanged = m_viewBackgroundIsTransparent != isTransparent;
    bool backgroundColorChanged = m_viewBackgroundColor != backgroundColor;
    bool extendedBackgroundColorChanged = m_rootExtendedBackgroundColor != extendedBackgroundColor;

    if (!transparencyChanged && !backgroundColorChanged && !extendedBackgroundColorChanged)
        return;

    LOG(Compositing, "RenderLayerCompositor %p rootBackgroundColorOrTransparencyChanged. isTransparent=%d", this, isTransparent);

    m_viewBackgroundIsTransparent = isTransparent;
    m_viewBackgroundColor = backgroundColor;
    m_rootExtendedBackgroundColor = extendedBackgroundColor;
    
    if (extendedBackgroundColorChanged) {
        page().chrome().client().pageExtendedBackgroundColorDidChange(m_rootExtendedBackgroundColor);
        
#if ENABLE(RUBBER_BANDING)
        if (m_layerForOverhangAreas) {
            m_layerForOverhangAreas->setBackgroundColor(m_rootExtendedBackgroundColor);

            if (!m_rootExtendedBackgroundColor.isValid())
                m_layerForOverhangAreas->setCustomAppearance(GraphicsLayer::CustomAppearance::ScrollingOverhang);
        }
#endif
    }
    
    rootLayerConfigurationChanged();
}

void RenderLayerCompositor::updateOverflowControlsLayers()
{
#if ENABLE(RUBBER_BANDING)
    if (requiresOverhangAreasLayer()) {
        if (!m_layerForOverhangAreas) {
            m_layerForOverhangAreas = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_layerForOverhangAreas->setName("overhang areas");
            m_layerForOverhangAreas->setDrawsContent(false);

            float topContentInset = m_renderView.frameView().topContentInset();
            IntSize overhangAreaSize = m_renderView.frameView().frameRect().size();
            overhangAreaSize.setHeight(overhangAreaSize.height() - topContentInset);
            m_layerForOverhangAreas->setSize(overhangAreaSize);
            m_layerForOverhangAreas->setPosition(FloatPoint(0, topContentInset));
            m_layerForOverhangAreas->setAnchorPoint(FloatPoint3D());

            if (m_renderView.settings().backgroundShouldExtendBeyondPage())
                m_layerForOverhangAreas->setBackgroundColor(m_renderView.frameView().documentBackgroundColor());
            else
                m_layerForOverhangAreas->setCustomAppearance(GraphicsLayer::CustomAppearance::ScrollingOverhang);

            // We want the overhang areas layer to be positioned below the frame contents,
            // so insert it below the clip layer.
            m_overflowControlsHostLayer->addChildBelow(*m_layerForOverhangAreas, layerForClipping());
        }
    } else
        GraphicsLayer::unparentAndClear(m_layerForOverhangAreas);

    if (requiresContentShadowLayer()) {
        if (!m_contentShadowLayer) {
            m_contentShadowLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_contentShadowLayer->setName("content shadow");
            m_contentShadowLayer->setSize(m_rootContentsLayer->size());
            m_contentShadowLayer->setPosition(m_rootContentsLayer->position());
            m_contentShadowLayer->setAnchorPoint(FloatPoint3D());
            m_contentShadowLayer->setCustomAppearance(GraphicsLayer::CustomAppearance::ScrollingShadow);

            m_scrolledContentsLayer->addChildBelow(*m_contentShadowLayer, m_rootContentsLayer.get());
        }
    } else
        GraphicsLayer::unparentAndClear(m_contentShadowLayer);
#endif

    if (requiresHorizontalScrollbarLayer()) {
        if (!m_layerForHorizontalScrollbar) {
            m_layerForHorizontalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_layerForHorizontalScrollbar->setCanDetachBackingStore(false);
            m_layerForHorizontalScrollbar->setShowDebugBorder(m_showDebugBorders);
            m_layerForHorizontalScrollbar->setName("horizontal scrollbar container");
#if PLATFORM(COCOA) && USE(CA)
            m_layerForHorizontalScrollbar->setAcceleratesDrawing(acceleratedDrawingEnabled());
#endif
            m_overflowControlsHostLayer->addChild(*m_layerForHorizontalScrollbar);

            if (auto* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), HorizontalScrollbar);
        }
    } else if (m_layerForHorizontalScrollbar) {
        GraphicsLayer::unparentAndClear(m_layerForHorizontalScrollbar);

        if (auto* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), HorizontalScrollbar);
    }

    if (requiresVerticalScrollbarLayer()) {
        if (!m_layerForVerticalScrollbar) {
            m_layerForVerticalScrollbar = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_layerForVerticalScrollbar->setCanDetachBackingStore(false);
            m_layerForVerticalScrollbar->setShowDebugBorder(m_showDebugBorders);
            m_layerForVerticalScrollbar->setName("vertical scrollbar container");
#if PLATFORM(COCOA) && USE(CA)
            m_layerForVerticalScrollbar->setAcceleratesDrawing(acceleratedDrawingEnabled());
#endif
            m_overflowControlsHostLayer->addChild(*m_layerForVerticalScrollbar);

            if (auto* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), VerticalScrollbar);
        }
    } else if (m_layerForVerticalScrollbar) {
        GraphicsLayer::unparentAndClear(m_layerForVerticalScrollbar);

        if (auto* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), VerticalScrollbar);
    }

    if (requiresScrollCornerLayer()) {
        if (!m_layerForScrollCorner) {
            m_layerForScrollCorner = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_layerForScrollCorner->setCanDetachBackingStore(false);
            m_layerForScrollCorner->setShowDebugBorder(m_showDebugBorders);
            m_layerForScrollCorner->setName("scroll corner");
#if PLATFORM(COCOA) && USE(CA)
            m_layerForScrollCorner->setAcceleratesDrawing(acceleratedDrawingEnabled());
#endif
            m_overflowControlsHostLayer->addChild(*m_layerForScrollCorner);
        }
    } else
        GraphicsLayer::unparentAndClear(m_layerForScrollCorner);

    m_renderView.frameView().positionScrollbarLayers();
}

void RenderLayerCompositor::ensureRootLayer()
{
    RootLayerAttachment expectedAttachment = isMainFrameCompositor() ? RootLayerAttachedViaChromeClient : RootLayerAttachedViaEnclosingFrame;
    if (expectedAttachment == m_rootLayerAttachment)
         return;

    if (!m_rootContentsLayer) {
        m_rootContentsLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
        m_rootContentsLayer->setName("content root");
        IntRect overflowRect = snappedIntRect(m_renderView.layoutOverflowRect());
        m_rootContentsLayer->setSize(FloatSize(overflowRect.maxX(), overflowRect.maxY()));
        m_rootContentsLayer->setPosition(FloatPoint());

#if PLATFORM(IOS_FAMILY)
        // Page scale is applied above this on iOS, so we'll just say that our root layer applies it.
        auto& frame = m_renderView.frameView().frame();
        if (frame.isMainFrame())
            m_rootContentsLayer->setAppliesPageScale();
#endif

        // Need to clip to prevent transformed content showing outside this frame
        updateRootContentLayerClipping();
    }

    if (requiresScrollLayer(expectedAttachment)) {
        if (!m_overflowControlsHostLayer) {
            ASSERT(!m_scrolledContentsLayer);
            ASSERT(!m_clipLayer);

            // Create a layer to host the clipping layer and the overflow controls layers.
            m_overflowControlsHostLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_overflowControlsHostLayer->setName("overflow controls host");

            m_scrolledContentsLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
            m_scrolledContentsLayer->setName("scrolled contents");
            m_scrolledContentsLayer->setAnchorPoint({ });

#if PLATFORM(IOS_FAMILY)
            if (m_renderView.settings().asyncFrameScrollingEnabled()) {
                m_scrollContainerLayer = GraphicsLayer::create(graphicsLayerFactory(), *this, GraphicsLayer::Type::ScrollContainer);

                m_scrollContainerLayer->setName("scroll container");
                m_scrollContainerLayer->setMasksToBounds(true);
                m_scrollContainerLayer->setAnchorPoint({ });

                m_scrollContainerLayer->addChild(*m_scrolledContentsLayer);
                m_overflowControlsHostLayer->addChild(*m_scrollContainerLayer);
            }
#endif
            if (!m_scrollContainerLayer) {
                m_clipLayer = GraphicsLayer::create(graphicsLayerFactory(), *this);
                m_clipLayer->setName("frame clipping");
                m_clipLayer->setMasksToBounds(true);
                m_clipLayer->setAnchorPoint({ });

                m_clipLayer->addChild(*m_scrolledContentsLayer);
                m_overflowControlsHostLayer->addChild(*m_clipLayer);
            }

            m_scrolledContentsLayer->addChild(*m_rootContentsLayer);

            updateScrollLayerClipping();
            updateOverflowControlsLayers();

            if (hasCoordinatedScrolling())
                scheduleLayerFlush(true);
            else
                updateScrollLayerPosition();
        }
    } else {
        if (m_overflowControlsHostLayer) {
            GraphicsLayer::unparentAndClear(m_overflowControlsHostLayer);
            GraphicsLayer::unparentAndClear(m_clipLayer);
            GraphicsLayer::unparentAndClear(m_scrollContainerLayer);
            GraphicsLayer::unparentAndClear(m_scrolledContentsLayer);
        }
    }

    // Check to see if we have to change the attachment
    if (m_rootLayerAttachment != RootLayerUnattached)
        detachRootLayer();

    attachRootLayer(expectedAttachment);
}

void RenderLayerCompositor::destroyRootLayer()
{
    if (!m_rootContentsLayer)
        return;

    detachRootLayer();

#if ENABLE(RUBBER_BANDING)
    GraphicsLayer::unparentAndClear(m_layerForOverhangAreas);
#endif

    if (m_layerForHorizontalScrollbar) {
        GraphicsLayer::unparentAndClear(m_layerForHorizontalScrollbar);
        if (auto* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), HorizontalScrollbar);
        if (auto* horizontalScrollbar = m_renderView.frameView().verticalScrollbar())
            m_renderView.frameView().invalidateScrollbar(*horizontalScrollbar, IntRect(IntPoint(0, 0), horizontalScrollbar->frameRect().size()));
    }

    if (m_layerForVerticalScrollbar) {
        GraphicsLayer::unparentAndClear(m_layerForVerticalScrollbar);
        if (auto* scrollingCoordinator = this->scrollingCoordinator())
            scrollingCoordinator->scrollableAreaScrollbarLayerDidChange(m_renderView.frameView(), VerticalScrollbar);
        if (auto* verticalScrollbar = m_renderView.frameView().verticalScrollbar())
            m_renderView.frameView().invalidateScrollbar(*verticalScrollbar, IntRect(IntPoint(0, 0), verticalScrollbar->frameRect().size()));
    }

    if (m_layerForScrollCorner) {
        GraphicsLayer::unparentAndClear(m_layerForScrollCorner);
        m_renderView.frameView().invalidateScrollCorner(m_renderView.frameView().scrollCornerRect());
    }

    if (m_overflowControlsHostLayer) {
        GraphicsLayer::unparentAndClear(m_overflowControlsHostLayer);
        GraphicsLayer::unparentAndClear(m_clipLayer);
        GraphicsLayer::unparentAndClear(m_scrollContainerLayer);
        GraphicsLayer::unparentAndClear(m_scrolledContentsLayer);
    }
    ASSERT(!m_scrolledContentsLayer);
    GraphicsLayer::unparentAndClear(m_rootContentsLayer);

    m_layerUpdater = nullptr;
}

void RenderLayerCompositor::attachRootLayer(RootLayerAttachment attachment)
{
    if (!m_rootContentsLayer)
        return;

    LOG(Compositing, "RenderLayerCompositor %p attachRootLayer %d", this, attachment);

    switch (attachment) {
        case RootLayerUnattached:
            ASSERT_NOT_REACHED();
            break;
        case RootLayerAttachedViaChromeClient: {
            auto& frame = m_renderView.frameView().frame();
            page().chrome().client().attachRootGraphicsLayer(frame, rootGraphicsLayer());
            break;
        }
        case RootLayerAttachedViaEnclosingFrame: {
            // The layer will get hooked up via RenderLayerBacking::updateConfiguration()
            // for the frame's renderer in the parent document.
            if (auto* ownerElement = m_renderView.document().ownerElement())
                ownerElement->scheduleInvalidateStyleAndLayerComposition();
            break;
        }
    }

    m_rootLayerAttachment = attachment;
    rootLayerAttachmentChanged();
    
    if (m_shouldFlushOnReattach) {
        scheduleLayerFlushNow();
        m_shouldFlushOnReattach = false;
    }
}

void RenderLayerCompositor::detachRootLayer()
{
    if (!m_rootContentsLayer || m_rootLayerAttachment == RootLayerUnattached)
        return;

    switch (m_rootLayerAttachment) {
    case RootLayerAttachedViaEnclosingFrame: {
        // The layer will get unhooked up via RenderLayerBacking::updateConfiguration()
        // for the frame's renderer in the parent document.
        if (m_overflowControlsHostLayer)
            m_overflowControlsHostLayer->removeFromParent();
        else
            m_rootContentsLayer->removeFromParent();

        if (auto* ownerElement = m_renderView.document().ownerElement())
            ownerElement->scheduleInvalidateStyleAndLayerComposition();

        if (auto frameRootScrollingNodeID = m_renderView.frameView().scrollingNodeID()) {
            if (auto* scrollingCoordinator = this->scrollingCoordinator())
                scrollingCoordinator->unparentNode(frameRootScrollingNodeID);
        }
        break;
    }
    case RootLayerAttachedViaChromeClient: {
        auto& frame = m_renderView.frameView().frame();
        page().chrome().client().attachRootGraphicsLayer(frame, nullptr);
    }
    break;
    case RootLayerUnattached:
        break;
    }

    m_rootLayerAttachment = RootLayerUnattached;
    rootLayerAttachmentChanged();
}

void RenderLayerCompositor::updateRootLayerAttachment()
{
    ensureRootLayer();
}

void RenderLayerCompositor::rootLayerAttachmentChanged()
{
    // The document-relative page overlay layer (which is pinned to the main frame's layer tree)
    // is moved between different RenderLayerCompositors' layer trees, and needs to be
    // reattached whenever we swap in a new RenderLayerCompositor.
    if (m_rootLayerAttachment == RootLayerUnattached)
        return;

    auto& frame = m_renderView.frameView().frame();

    // The attachment can affect whether the RenderView layer's paintsIntoWindow() behavior,
    // so call updateDrawsContent() to update that.
    auto* layer = m_renderView.layer();
    if (auto* backing = layer ? layer->backing() : nullptr)
        backing->updateDrawsContent();

    if (!frame.isMainFrame())
        return;

    Ref<GraphicsLayer> overlayHost = page().pageOverlayController().layerWithDocumentOverlays();
    m_rootContentsLayer->addChild(WTFMove(overlayHost));
}

void RenderLayerCompositor::notifyIFramesOfCompositingChange()
{
    // Compositing affects the answer to RenderIFrame::requiresAcceleratedCompositing(), so
    // we need to schedule a style recalc in our parent document.
    if (auto* ownerElement = m_renderView.document().ownerElement())
        ownerElement->scheduleInvalidateStyleAndLayerComposition();
}

bool RenderLayerCompositor::layerHas3DContent(const RenderLayer& layer) const
{
    const RenderStyle& style = layer.renderer().style();

    if (style.transformStyle3D() == TransformStyle3D::Preserve3D || style.hasPerspective() || style.transform().has3DOperation())
        return true;

    const_cast<RenderLayer&>(layer).updateLayerListsIfNeeded();

#if !ASSERT_DISABLED
    LayerListMutationDetector mutationChecker(const_cast<RenderLayer&>(layer));
#endif

    for (auto* renderLayer : layer.negativeZOrderLayers()) {
        if (layerHas3DContent(*renderLayer))
            return true;
    }

    for (auto* renderLayer : layer.positiveZOrderLayers()) {
        if (layerHas3DContent(*renderLayer))
            return true;
    }

    for (auto* renderLayer : layer.normalFlowLayers()) {
        if (layerHas3DContent(*renderLayer))
            return true;
    }

    return false;
}

void RenderLayerCompositor::deviceOrPageScaleFactorChanged()
{
    // Page scale will only be applied at to the RenderView and sublayers, but the device scale factor
    // needs to be applied at the level of rootGraphicsLayer().
    if (auto* rootLayer = rootGraphicsLayer())
        rootLayer->noteDeviceOrPageScaleFactorChangedIncludingDescendants();
}

void RenderLayerCompositor::removeFromScrollCoordinatedLayers(RenderLayer& layer)
{
#if PLATFORM(IOS_FAMILY)
    if (m_legacyScrollingLayerCoordinator)
        m_legacyScrollingLayerCoordinator->removeLayer(layer);
#endif

    detachScrollCoordinatedLayer(layer, { ScrollCoordinationRole::Scrolling, ScrollCoordinationRole::ViewportConstrained, ScrollCoordinationRole::FrameHosting });
}

FixedPositionViewportConstraints RenderLayerCompositor::computeFixedViewportConstraints(RenderLayer& layer) const
{
    ASSERT(layer.isComposited());

    auto* graphicsLayer = layer.backing()->graphicsLayer();

    FixedPositionViewportConstraints constraints;
    constraints.setLayerPositionAtLastLayout(graphicsLayer->position());
    constraints.setViewportRectAtLastLayout(m_renderView.frameView().rectForFixedPositionLayout());
    constraints.setAlignmentOffset(graphicsLayer->pixelAlignmentOffset());

    const RenderStyle& style = layer.renderer().style();
    if (!style.left().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeLeft);

    if (!style.right().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeRight);

    if (!style.top().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeTop);

    if (!style.bottom().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeBottom);

    // If left and right are auto, use left.
    if (style.left().isAuto() && style.right().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeLeft);

    // If top and bottom are auto, use top.
    if (style.top().isAuto() && style.bottom().isAuto())
        constraints.addAnchorEdge(ViewportConstraints::AnchorEdgeTop);
        
    return constraints;
}

StickyPositionViewportConstraints RenderLayerCompositor::computeStickyViewportConstraints(RenderLayer& layer) const
{
    ASSERT(layer.isComposited());
#if !PLATFORM(IOS_FAMILY)
    // We should never get here for stickies constrained by an enclosing clipping layer.
    // FIXME: Why does this assertion fail on iOS?
    ASSERT(!layer.enclosingOverflowClipLayer(ExcludeSelf));
#endif

    auto& renderer = downcast<RenderBoxModelObject>(layer.renderer());

    StickyPositionViewportConstraints constraints;
    renderer.computeStickyPositionConstraints(constraints, renderer.constrainingRectForStickyPosition());

    auto* graphicsLayer = layer.backing()->graphicsLayer();
    constraints.setLayerPositionAtLastLayout(graphicsLayer->position());
    constraints.setStickyOffsetAtLastLayout(renderer.stickyPositionOffset());
    constraints.setAlignmentOffset(graphicsLayer->pixelAlignmentOffset());

    return constraints;
}

static inline ScrollCoordinationRole scrollCoordinationRoleForNodeType(ScrollingNodeType nodeType)
{
    switch (nodeType) {
    case ScrollingNodeType::MainFrame:
    case ScrollingNodeType::Subframe:
    case ScrollingNodeType::Overflow:
        return ScrollCoordinationRole::Scrolling;
    case ScrollingNodeType::FrameHosting:
        return ScrollCoordinationRole::FrameHosting;
    case ScrollingNodeType::Fixed:
    case ScrollingNodeType::Sticky:
        return ScrollCoordinationRole::ViewportConstrained;
    }
    ASSERT_NOT_REACHED();
    return ScrollCoordinationRole::Scrolling;
}

ScrollingNodeID RenderLayerCompositor::attachScrollingNode(RenderLayer& layer, ScrollingNodeType nodeType, ScrollingTreeState& treeState)
{
    auto* scrollingCoordinator = this->scrollingCoordinator();
    auto* backing = layer.backing();
    // Crash logs suggest that backing can be null here, but we don't know how: rdar://problem/18545452.
    ASSERT(backing);
    if (!backing)
        return 0;

    ASSERT(treeState.parentNodeID || nodeType == ScrollingNodeType::Subframe);
    ASSERT_IMPLIES(nodeType == ScrollingNodeType::MainFrame, !treeState.parentNodeID.value());

    ScrollCoordinationRole role = scrollCoordinationRoleForNodeType(nodeType);
    ScrollingNodeID nodeID = backing->scrollingNodeIDForRole(role);
    if (!nodeID)
        nodeID = scrollingCoordinator->uniqueScrollingNodeID();

    LOG_WITH_STREAM(Scrolling, stream << "RenderLayerCompositor " << this << " attachScrollingNode " << nodeID << " (layer " << backing->graphicsLayer()->primaryLayerID() << ") type " << nodeType << " parent " << treeState.parentNodeID.valueOr(0));

    if (nodeType == ScrollingNodeType::Subframe && !treeState.parentNodeID)
        nodeID = scrollingCoordinator->createNode(nodeType, nodeID);
    else {
        auto newNodeID = scrollingCoordinator->insertNode(nodeType, nodeID, treeState.parentNodeID.valueOr(0), treeState.nextChildIndex);
        if (newNodeID != nodeID) {
            // We'll get a new nodeID if the type changed (and not if the node is new).
            scrollingCoordinator->unparentChildrenAndDestroyNode(nodeID);
            m_scrollingNodeToLayerMap.remove(nodeID);
        }
        nodeID = newNodeID;
    }

    ASSERT(nodeID);
    if (!nodeID)
        return 0;
    
    backing->setScrollingNodeIDForRole(nodeID, role);
    m_scrollingNodeToLayerMap.add(nodeID, &layer);
    
    ++treeState.nextChildIndex;
    return nodeID;
}

void RenderLayerCompositor::detachScrollCoordinatedLayerWithRole(RenderLayer& layer, ScrollingCoordinator& scrollingCoordinator, ScrollCoordinationRole role)
{
    auto nodeID = layer.backing()->scrollingNodeIDForRole(role);
    if (!nodeID)
        return;

    auto childNodes = scrollingCoordinator.childrenOfNode(nodeID);
    for (auto childNodeID : childNodes) {
        // FIXME: The child might be in a child frame. Need to do something that crosses frame boundaries.
        if (auto* layer = m_scrollingNodeToLayerMap.get(childNodeID))
            layer->setNeedsScrollingTreeUpdate();
    }

    m_scrollingNodeToLayerMap.remove(nodeID);
}

void RenderLayerCompositor::detachScrollCoordinatedLayer(RenderLayer& layer, OptionSet<ScrollCoordinationRole> roles)
{
    auto* backing = layer.backing();
    if (!backing)
        return;

    auto* scrollingCoordinator = this->scrollingCoordinator();

    if (roles.contains(ScrollCoordinationRole::Scrolling))
        detachScrollCoordinatedLayerWithRole(layer, *scrollingCoordinator, ScrollCoordinationRole::Scrolling);

    if (roles.contains(ScrollCoordinationRole::FrameHosting))
        detachScrollCoordinatedLayerWithRole(layer, *scrollingCoordinator, ScrollCoordinationRole::FrameHosting);

    if (roles.contains(ScrollCoordinationRole::ViewportConstrained))
        detachScrollCoordinatedLayerWithRole(layer, *scrollingCoordinator, ScrollCoordinationRole::ViewportConstrained);

    backing->detachFromScrollingCoordinator(roles);
}

ScrollingNodeID RenderLayerCompositor::updateScrollCoordinationForLayer(RenderLayer& layer, ScrollingTreeState& treeState, OptionSet<ScrollCoordinationRole> roles, OptionSet<ScrollingNodeChangeFlags> changes)
{
    bool isViewportConstrained = roles.contains(ScrollCoordinationRole::ViewportConstrained);
#if PLATFORM(IOS_FAMILY)
    if (m_legacyScrollingLayerCoordinator) {
        if (isViewportConstrained)
            m_legacyScrollingLayerCoordinator->addViewportConstrainedLayer(layer);
        else
            m_legacyScrollingLayerCoordinator->removeViewportConstrainedLayer(layer);
    }
#endif

    // GraphicsLayers need to know whether they are viewport-constrained.
    layer.backing()->setIsScrollCoordinatedWithViewportConstrainedRole(isViewportConstrained);

    if (!hasCoordinatedScrolling()) {
        // If this frame isn't coordinated, it cannot contain any scrolling tree nodes.
        return 0;
    }

    auto newNodeID = treeState.parentNodeID.valueOr(0);

    ScrollingTreeState viewportConstrainedChildTreeState;
    ScrollingTreeState* currentTreeState = &treeState;

    // If a node plays both roles, fixed/sticky is always the ancestor node of scrolling/frame hosting.
    if (roles.contains(ScrollCoordinationRole::ViewportConstrained)) {
        newNodeID = updateScrollingNodeForViewportConstrainedRole(layer, *currentTreeState, changes);
        // ViewportConstrained nodes are the parent of same-layer scrolling nodes, so adjust the ScrollingTreeState.
        viewportConstrainedChildTreeState.parentNodeID = newNodeID;
        currentTreeState = &viewportConstrainedChildTreeState;
    } else
        detachScrollCoordinatedLayer(layer, ScrollCoordinationRole::ViewportConstrained);

    if (roles.contains(ScrollCoordinationRole::Scrolling))
        newNodeID = updateScrollingNodeForScrollingRole(layer, *currentTreeState, changes);
    else
        detachScrollCoordinatedLayer(layer, ScrollCoordinationRole::Scrolling);

    if (roles.contains(ScrollCoordinationRole::FrameHosting))
        newNodeID = updateScrollingNodeForFrameHostingRole(layer, *currentTreeState, changes);
    else
        detachScrollCoordinatedLayer(layer, ScrollCoordinationRole::FrameHosting);

    return newNodeID;
}

ScrollingNodeID RenderLayerCompositor::updateScrollingNodeForViewportConstrainedRole(RenderLayer& layer, ScrollingTreeState& treeState, OptionSet<ScrollingNodeChangeFlags> changes)
{
    auto* scrollingCoordinator = this->scrollingCoordinator();

    auto nodeType = ScrollingNodeType::Fixed;
    if (layer.renderer().style().position() == PositionType::Sticky)
        nodeType = ScrollingNodeType::Sticky;
    else
        ASSERT(layer.renderer().isFixedPositioned());

    auto newNodeID = attachScrollingNode(layer, nodeType, treeState);
    if (!newNodeID) {
        ASSERT_NOT_REACHED();
        return treeState.parentNodeID.valueOr(0);
    }

    LOG_WITH_STREAM(Compositing, stream << "Registering ViewportConstrained " << nodeType << " node " << newNodeID << " (layer " << layer.backing()->graphicsLayer()->primaryLayerID() << ") as child of " << treeState.parentNodeID.valueOr(0));

    if (changes & ScrollingNodeChangeFlags::Layer)
        scrollingCoordinator->setNodeLayers(newNodeID, { layer.backing()->graphicsLayer() });

    if (changes & ScrollingNodeChangeFlags::LayerGeometry) {
        switch (nodeType) {
        case ScrollingNodeType::Fixed:
            scrollingCoordinator->setViewportConstraintedNodeGeometry(newNodeID, computeFixedViewportConstraints(layer));
            break;
        case ScrollingNodeType::Sticky:
            scrollingCoordinator->setViewportConstraintedNodeGeometry(newNodeID, computeStickyViewportConstraints(layer));
            break;
        case ScrollingNodeType::MainFrame:
        case ScrollingNodeType::Subframe:
        case ScrollingNodeType::FrameHosting:
        case ScrollingNodeType::Overflow:
            break;
        }
    }

    return newNodeID;
}

void RenderLayerCompositor::computeFrameScrollingGeometry(ScrollingCoordinator::ScrollingGeometry& scrollingGeometry) const
{
    auto& frameView = m_renderView.frameView();

    if (m_renderView.frame().isMainFrame())
        scrollingGeometry.parentRelativeScrollableRect = frameView.frameRect();
    else
        scrollingGeometry.parentRelativeScrollableRect = LayoutRect({ }, LayoutSize(frameView.size()));

    scrollingGeometry.scrollOrigin = frameView.scrollOrigin();
    scrollingGeometry.scrollableAreaSize = frameView.visibleContentRect().size();
    scrollingGeometry.contentSize = frameView.totalContentsSize();
    scrollingGeometry.reachableContentSize = frameView.totalContentsSize();
#if ENABLE(CSS_SCROLL_SNAP)
    frameView.updateSnapOffsets();
    updateScrollSnapPropertiesWithFrameView(frameView);
#endif
}

void RenderLayerCompositor::computeFrameHostingGeometry(const RenderLayer& layer, const RenderLayer* ancestorLayer, ScrollingCoordinator::ScrollingGeometry& scrollingGeometry) const
{
    // FIXME: ancestorLayer needs to be always non-null, so should become a reference.
    if (ancestorLayer) {
        LayoutRect scrollableRect;
        if (is<RenderBox>(layer.renderer()))
            scrollableRect = downcast<RenderBox>(layer.renderer()).paddingBoxRect();

        auto offset = layer.convertToLayerCoords(ancestorLayer, scrollableRect.location()); // FIXME: broken for columns.
        scrollableRect.setLocation(offset);
        scrollingGeometry.parentRelativeScrollableRect = scrollableRect;
    }
}

void RenderLayerCompositor::computeOverflowScrollingGeometry(const RenderLayer& layer, const RenderLayer* ancestorLayer, ScrollingCoordinator::ScrollingGeometry& scrollingGeometry) const
{
    // FIXME: ancestorLayer needs to be always non-null, so should become a reference.
    if (ancestorLayer) {
        LayoutRect scrollableRect;
        if (is<RenderBox>(layer.renderer()))
            scrollableRect = downcast<RenderBox>(layer.renderer()).paddingBoxRect();

        auto offset = layer.convertToLayerCoords(ancestorLayer, scrollableRect.location()); // FIXME: broken for columns.
        scrollableRect.setLocation(offset);
        scrollingGeometry.parentRelativeScrollableRect = scrollableRect;
    }

    scrollingGeometry.scrollOrigin = layer.scrollOrigin();
    scrollingGeometry.scrollPosition = layer.scrollPosition();
    scrollingGeometry.scrollableAreaSize = layer.visibleSize();
    scrollingGeometry.contentSize = layer.contentsSize();
    scrollingGeometry.reachableContentSize = layer.scrollableContentsSize();
#if ENABLE(CSS_SCROLL_SNAP)
    if (auto* offsets = layer.horizontalSnapOffsets())
        scrollingGeometry.horizontalSnapOffsets = *offsets;
    if (auto* offsets = layer.verticalSnapOffsets())
        scrollingGeometry.verticalSnapOffsets = *offsets;
    if (auto* ranges = layer.horizontalSnapOffsetRanges())
        scrollingGeometry.horizontalSnapOffsetRanges = *ranges;
    if (auto* ranges = layer.verticalSnapOffsetRanges())
        scrollingGeometry.verticalSnapOffsetRanges = *ranges;
    scrollingGeometry.currentHorizontalSnapPointIndex = layer.currentHorizontalSnapPointIndex();
    scrollingGeometry.currentVerticalSnapPointIndex = layer.currentVerticalSnapPointIndex();
#endif
}

ScrollingNodeID RenderLayerCompositor::updateScrollingNodeForScrollingRole(RenderLayer& layer, ScrollingTreeState& treeState, OptionSet<ScrollingNodeChangeFlags> changes)
{
    auto* scrollingCoordinator = this->scrollingCoordinator();

    ScrollingNodeID newNodeID = 0;

    if (layer.isRenderViewLayer()) {
        FrameView& frameView = m_renderView.frameView();
        ASSERT_UNUSED(frameView, scrollingCoordinator->coordinatesScrollingForFrameView(frameView));

        newNodeID = attachScrollingNode(*m_renderView.layer(), m_renderView.frame().isMainFrame() ? ScrollingNodeType::MainFrame : ScrollingNodeType::Subframe, treeState);

        if (!newNodeID) {
            ASSERT_NOT_REACHED();
            return treeState.parentNodeID.valueOr(0);
        }

        if (changes & ScrollingNodeChangeFlags::Layer)
            scrollingCoordinator->setNodeLayers(newNodeID, { nullptr, scrollContainerLayer(), scrolledContentsLayer(), fixedRootBackgroundLayer(), clipLayer(), rootContentsLayer() });

        if (changes & ScrollingNodeChangeFlags::LayerGeometry) {
            ScrollingCoordinator::ScrollingGeometry scrollingGeometry;
            computeFrameScrollingGeometry(scrollingGeometry);
            scrollingCoordinator->setScrollingNodeGeometry(newNodeID, scrollingGeometry);
        }
    } else {
        newNodeID = attachScrollingNode(layer, ScrollingNodeType::Overflow, treeState);
        if (!newNodeID) {
            ASSERT_NOT_REACHED();
            return treeState.parentNodeID.valueOr(0);
        }
        
        if (changes & ScrollingNodeChangeFlags::Layer)
            scrollingCoordinator->setNodeLayers(newNodeID, { layer.backing()->graphicsLayer(), layer.backing()->scrollContainerLayer(), layer.backing()->scrolledContentsLayer() });

        if (changes & ScrollingNodeChangeFlags::LayerGeometry && treeState.parentNodeID) {
            RenderLayer* scrollingAncestorLayer = m_scrollingNodeToLayerMap.get(treeState.parentNodeID.value());
            ScrollingCoordinator::ScrollingGeometry scrollingGeometry;
            computeOverflowScrollingGeometry(layer, scrollingAncestorLayer, scrollingGeometry);
            scrollingCoordinator->setScrollingNodeGeometry(newNodeID, scrollingGeometry);
        }
    }

    return newNodeID;
}

ScrollingNodeID RenderLayerCompositor::updateScrollingNodeForFrameHostingRole(RenderLayer& layer, ScrollingTreeState& treeState, OptionSet<ScrollingNodeChangeFlags> changes)
{
    auto* scrollingCoordinator = this->scrollingCoordinator();

    auto newNodeID = attachScrollingNode(layer, ScrollingNodeType::FrameHosting, treeState);
    if (!newNodeID) {
        ASSERT_NOT_REACHED();
        return treeState.parentNodeID.valueOr(0);
    }

    if (changes & ScrollingNodeChangeFlags::Layer)
        scrollingCoordinator->setNodeLayers(newNodeID, { layer.backing()->graphicsLayer() });

    if (changes & ScrollingNodeChangeFlags::LayerGeometry && treeState.parentNodeID) {
        RenderLayer* scrollingAncestorLayer = m_scrollingNodeToLayerMap.get(treeState.parentNodeID.value());
        ScrollingCoordinator::ScrollingGeometry scrollingGeometry;
        computeFrameHostingGeometry(layer, scrollingAncestorLayer, scrollingGeometry);
        scrollingCoordinator->setScrollingNodeGeometry(newNodeID, scrollingGeometry);
    }

    return newNodeID;
}

ScrollableArea* RenderLayerCompositor::scrollableAreaForScrollLayerID(ScrollingNodeID nodeID) const
{
    if (!nodeID)
        return nullptr;

    return m_scrollingNodeToLayerMap.get(nodeID);
}

void RenderLayerCompositor::willRemoveScrollingLayerWithBacking(RenderLayer& layer, RenderLayerBacking& backing)
{
    if (scrollingCoordinator())
        return;

#if PLATFORM(IOS_FAMILY)
    ASSERT(m_renderView.document().pageCacheState() == Document::NotInPageCache);
    if (m_legacyScrollingLayerCoordinator)
        m_legacyScrollingLayerCoordinator->removeScrollingLayer(layer, backing);
#else
    UNUSED_PARAM(layer);
    UNUSED_PARAM(backing);
#endif
}

// FIXME: This should really be called from the updateBackingAndHierarchy.
void RenderLayerCompositor::didAddScrollingLayer(RenderLayer& layer)
{
    if (scrollingCoordinator())
        return;

#if PLATFORM(IOS_FAMILY)
    ASSERT(m_renderView.document().pageCacheState() == Document::NotInPageCache);
    if (m_legacyScrollingLayerCoordinator)
        m_legacyScrollingLayerCoordinator->addScrollingLayer(layer);
#else
    UNUSED_PARAM(layer);
#endif
}

void RenderLayerCompositor::windowScreenDidChange(PlatformDisplayID displayID)
{
    if (m_layerUpdater)
        m_layerUpdater->screenDidChange(displayID);
}

ScrollingCoordinator* RenderLayerCompositor::scrollingCoordinator() const
{
    return page().scrollingCoordinator();
}

GraphicsLayerFactory* RenderLayerCompositor::graphicsLayerFactory() const
{
    return page().chrome().client().graphicsLayerFactory();
}

void RenderLayerCompositor::setLayerFlushThrottlingEnabled(bool enabled)
{
    m_layerFlushThrottlingEnabled = enabled;
    if (m_layerFlushThrottlingEnabled)
        return;
    m_layerFlushTimer.stop();
    if (!m_hasPendingLayerFlush)
        return;
    scheduleLayerFlushNow();
}

void RenderLayerCompositor::disableLayerFlushThrottlingTemporarilyForInteraction()
{
    if (m_layerFlushThrottlingTemporarilyDisabledForInteraction)
        return;
    m_layerFlushThrottlingTemporarilyDisabledForInteraction = true;
}

bool RenderLayerCompositor::isThrottlingLayerFlushes() const
{
    if (!m_layerFlushThrottlingEnabled)
        return false;
    if (!m_layerFlushTimer.isActive())
        return false;
    if (m_layerFlushThrottlingTemporarilyDisabledForInteraction)
        return false;
    return true;
}

void RenderLayerCompositor::startLayerFlushTimerIfNeeded()
{
    m_layerFlushThrottlingTemporarilyDisabledForInteraction = false;
    m_layerFlushTimer.stop();
    if (!m_layerFlushThrottlingEnabled)
        return;
    m_layerFlushTimer.startOneShot(throttledLayerFlushDelay);
}

void RenderLayerCompositor::startInitialLayerFlushTimerIfNeeded()
{
    if (!m_layerFlushThrottlingEnabled)
        return;
    if (m_layerFlushTimer.isActive())
        return;
    m_layerFlushTimer.startOneShot(throttledLayerFlushInitialDelay);
}

void RenderLayerCompositor::layerFlushTimerFired()
{
    if (!m_hasPendingLayerFlush)
        return;
    scheduleLayerFlushNow();
}

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
RefPtr<DisplayRefreshMonitor> RenderLayerCompositor::createDisplayRefreshMonitor(PlatformDisplayID displayID) const
{
    if (auto monitor = page().chrome().client().createDisplayRefreshMonitor(displayID))
        return monitor;

    return DisplayRefreshMonitor::createDefaultDisplayRefreshMonitor(displayID);
}
#endif

#if ENABLE(CSS_SCROLL_SNAP)
void RenderLayerCompositor::updateScrollSnapPropertiesWithFrameView(const FrameView& frameView) const
{
    if (auto* coordinator = scrollingCoordinator())
        coordinator->updateScrollSnapPropertiesWithFrameView(frameView);
}
#endif

Page& RenderLayerCompositor::page() const
{
    return m_renderView.page();
}

TextStream& operator<<(TextStream& ts, CompositingUpdateType updateType)
{
    switch (updateType) {
    case CompositingUpdateType::AfterStyleChange: ts << "after style change"; break;
    case CompositingUpdateType::AfterLayout: ts << "after layout"; break;
    case CompositingUpdateType::OnScroll: ts << "on scroll"; break;
    case CompositingUpdateType::OnCompositedScroll: ts << "on composited scroll"; break;
    }
    return ts;
}

TextStream& operator<<(TextStream& ts, CompositingPolicy compositingPolicy)
{
    switch (compositingPolicy) {
    case CompositingPolicy::Normal: ts << "normal"; break;
    case CompositingPolicy::Conservative: ts << "conservative"; break;
    }
    return ts;
}

#if PLATFORM(IOS_FAMILY)
typedef HashMap<PlatformLayer*, std::unique_ptr<ViewportConstraints>> LayerMap;
typedef HashMap<PlatformLayer*, PlatformLayer*> StickyContainerMap;

void LegacyWebKitScrollingLayerCoordinator::registerAllViewportConstrainedLayers(RenderLayerCompositor& compositor)
{
    if (!m_coordinateViewportConstrainedLayers)
        return;

    LayerMap layerMap;
    StickyContainerMap stickyContainerMap;

    for (auto* layer : m_viewportConstrainedLayers) {
        ASSERT(layer->isComposited());

        std::unique_ptr<ViewportConstraints> constraints;
        if (layer->renderer().isStickilyPositioned()) {
            constraints = std::make_unique<StickyPositionViewportConstraints>(compositor.computeStickyViewportConstraints(*layer));
            const RenderLayer* enclosingTouchScrollableLayer = nullptr;
            if (compositor.isAsyncScrollableStickyLayer(*layer, &enclosingTouchScrollableLayer) && enclosingTouchScrollableLayer) {
                ASSERT(enclosingTouchScrollableLayer->isComposited());
                // what
                stickyContainerMap.add(layer->backing()->graphicsLayer()->platformLayer(), enclosingTouchScrollableLayer->backing()->scrollContainerLayer()->platformLayer());
            }
        } else if (layer->renderer().isFixedPositioned())
            constraints = std::make_unique<FixedPositionViewportConstraints>(compositor.computeFixedViewportConstraints(*layer));
        else
            continue;

        layerMap.add(layer->backing()->graphicsLayer()->platformLayer(), WTFMove(constraints));
    }
    
    m_chromeClient.updateViewportConstrainedLayers(layerMap, stickyContainerMap);
}

void LegacyWebKitScrollingLayerCoordinator::unregisterAllViewportConstrainedLayers()
{
    if (!m_coordinateViewportConstrainedLayers)
        return;

    LayerMap layerMap;
    m_chromeClient.updateViewportConstrainedLayers(layerMap, { });
}

static bool scrollbarHasDisplayNone(Scrollbar* scrollbar)
{
    if (!scrollbar || !scrollbar->isCustomScrollbar())
        return false;

    std::unique_ptr<RenderStyle> scrollbarStyle = static_cast<RenderScrollbar*>(scrollbar)->getScrollbarPseudoStyle(ScrollbarBGPart, PseudoId::Scrollbar);
    return scrollbarStyle && scrollbarStyle->display() == DisplayType::None;
}

void LegacyWebKitScrollingLayerCoordinator::updateScrollingLayer(RenderLayer& layer)
{
    auto* backing = layer.backing();
    ASSERT(backing);

    bool allowHorizontalScrollbar = !scrollbarHasDisplayNone(layer.horizontalScrollbar());
    bool allowVerticalScrollbar = !scrollbarHasDisplayNone(layer.verticalScrollbar());
    m_chromeClient.addOrUpdateScrollingLayer(layer.renderer().element(), backing->scrollContainerLayer()->platformLayer(), backing->scrolledContentsLayer()->platformLayer(),
        layer.scrollableContentsSize(), allowHorizontalScrollbar, allowVerticalScrollbar);
}

void LegacyWebKitScrollingLayerCoordinator::registerAllScrollingLayers()
{
    for (auto* layer : m_scrollingLayers)
        updateScrollingLayer(*layer);
}

void LegacyWebKitScrollingLayerCoordinator::registerScrollingLayersNeedingUpdate()
{
    for (auto* layer : m_scrollingLayersNeedingUpdate)
        updateScrollingLayer(*layer);
    
    m_scrollingLayersNeedingUpdate.clear();
}

void LegacyWebKitScrollingLayerCoordinator::unregisterAllScrollingLayers()
{
    for (auto* layer : m_scrollingLayers) {
        auto* backing = layer->backing();
        ASSERT(backing);
        m_chromeClient.removeScrollingLayer(layer->renderer().element(), backing->scrollContainerLayer()->platformLayer(), backing->scrolledContentsLayer()->platformLayer());
    }
}

void LegacyWebKitScrollingLayerCoordinator::addScrollingLayer(RenderLayer& layer)
{
    m_scrollingLayers.add(&layer);
    m_scrollingLayersNeedingUpdate.add(&layer);
}

void LegacyWebKitScrollingLayerCoordinator::removeScrollingLayer(RenderLayer& layer, RenderLayerBacking& backing)
{
    m_scrollingLayersNeedingUpdate.remove(&layer);
    if (m_scrollingLayers.remove(&layer)) {
        auto* scrollContainerLayer = backing.scrollContainerLayer()->platformLayer();
        auto* scrolledContentsLayer = backing.scrolledContentsLayer()->platformLayer();
        m_chromeClient.removeScrollingLayer(layer.renderer().element(), scrollContainerLayer, scrolledContentsLayer);
    }
}

void LegacyWebKitScrollingLayerCoordinator::removeLayer(RenderLayer& layer)
{
    removeScrollingLayer(layer, *layer.backing());

    // We'll put the new set of layers to the client via registerAllViewportConstrainedLayers() at flush time.
    m_viewportConstrainedLayers.remove(&layer);
}

void LegacyWebKitScrollingLayerCoordinator::addViewportConstrainedLayer(RenderLayer& layer)
{
    m_viewportConstrainedLayers.add(&layer);
}

void LegacyWebKitScrollingLayerCoordinator::removeViewportConstrainedLayer(RenderLayer& layer)
{
    m_viewportConstrainedLayers.remove(&layer);
}

void LegacyWebKitScrollingLayerCoordinator::didChangePlatformLayerForLayer(RenderLayer& layer)
{
    if (m_scrollingLayers.contains(&layer))
        m_scrollingLayersNeedingUpdate.add(&layer);
}

#endif

} // namespace WebCore

#if ENABLE(TREE_DEBUGGING)
void showGraphicsLayerTreeForCompositor(WebCore::RenderLayerCompositor& compositor)
{
    showGraphicsLayerTree(compositor.rootGraphicsLayer());
}
#endif
