/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2013 Company 100, Inc. All rights reserved.
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
#include "CompositingCoordinator.h"

#if USE(COORDINATED_GRAPHICS)

#include <WebCore/DOMWindow.h>
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/InspectorController.h>
#include <WebCore/NicosiaPaintingEngine.h>
#include <WebCore/Page.h>
#include <wtf/MemoryPressureHandler.h>
#include <wtf/SetForScope.h>

#if USE(GLIB_EVENT_LOOP)
#include <wtf/glib/RunLoopSourcePriority.h>
#endif

namespace WebKit {
using namespace WebCore;

CompositingCoordinator::CompositingCoordinator(Page* page, CompositingCoordinator::Client& client)
    : m_page(page)
    , m_client(client)
    , m_paintingEngine(Nicosia::PaintingEngine::create())
{
    m_nicosia.scene = Nicosia::Scene::create();
    m_state.nicosia.scene = m_nicosia.scene;
}

CompositingCoordinator::~CompositingCoordinator()
{
    m_isDestructing = true;

    purgeBackingStores();

    for (auto& registeredLayer : m_registeredLayers.values())
        registeredLayer->setCoordinator(nullptr);
}

void CompositingCoordinator::invalidate()
{
    m_rootLayer = nullptr;
    purgeBackingStores();
}

void CompositingCoordinator::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    if (m_rootCompositingLayer == graphicsLayer)
        return;

    if (m_rootCompositingLayer)
        m_rootCompositingLayer->removeFromParent();

    m_rootCompositingLayer = graphicsLayer;
    if (m_rootCompositingLayer)
        m_rootLayer->addChildAtIndex(m_rootCompositingLayer, 0);
}

void CompositingCoordinator::setViewOverlayRootLayer(GraphicsLayer* graphicsLayer)
{
    if (m_overlayCompositingLayer == graphicsLayer)
        return;

    if (m_overlayCompositingLayer)
        m_overlayCompositingLayer->removeFromParent();

    m_overlayCompositingLayer = graphicsLayer;
    if (m_overlayCompositingLayer)
        m_rootLayer->addChild(m_overlayCompositingLayer);
}

void CompositingCoordinator::sizeDidChange(const IntSize& newSize)
{
    m_rootLayer->setSize(newSize);
    notifyFlushRequired(m_rootLayer.get());
}

bool CompositingCoordinator::flushPendingLayerChanges()
{
    SetForScope<bool> protector(m_isFlushingLayerChanges, true);

    initializeRootCompositingLayerIfNeeded();

    m_rootLayer->flushCompositingStateForThisLayerOnly();
    m_client.didFlushRootLayer(m_visibleContentsRect);

    if (m_overlayCompositingLayer)
        m_overlayCompositingLayer->flushCompositingState(FloatRect(FloatPoint(), m_rootLayer->size()));

    bool didSync = m_page->mainFrame().view()->flushCompositingStateIncludingSubframes();

    auto& coordinatedLayer = downcast<CoordinatedGraphicsLayer>(*m_rootLayer);
    coordinatedLayer.updateContentBuffersIncludingSubLayers();
    coordinatedLayer.syncPendingStateChangesIncludingSubLayers();

    flushPendingImageBackingChanges();

    if (m_shouldSyncFrame) {
        didSync = true;

        m_state.nicosia.scene->accessState(
            [this](Nicosia::Scene::State& state)
            {
                ++state.id;
                state.layers = m_nicosia.state.layers;
                state.rootLayer = m_nicosia.state.rootLayer;
            });

        m_client.commitSceneState(m_state);

        clearPendingStateChanges();
        m_shouldSyncFrame = false;
    }

    return didSync;
}

double CompositingCoordinator::timestamp() const
{
    auto* document = m_page->mainFrame().document();
    if (!document)
        return 0;
    return document->domWindow() ? document->domWindow()->nowTimestamp() : document->monotonicTimestamp();
}

void CompositingCoordinator::syncDisplayState()
{
    m_page->mainFrame().view()->updateLayoutAndStyleIfNeededRecursive();
}

double CompositingCoordinator::nextAnimationServiceTime() const
{
    // According to the requestAnimationFrame spec, rAF callbacks should not be faster than 60FPS.
    static const double MinimalTimeoutForAnimations = 1. / 60.;
    return std::max<double>(0., MinimalTimeoutForAnimations - timestamp() + m_lastAnimationServiceTime);
}

void CompositingCoordinator::clearPendingStateChanges()
{
    m_state.layersToCreate.clear();
    m_state.layersToUpdate.clear();
    m_state.layersToRemove.clear();

    m_state.imagesToCreate.clear();
    m_state.imagesToRemove.clear();
    m_state.imagesToUpdate.clear();
    m_state.imagesToClear.clear();
}

void CompositingCoordinator::initializeRootCompositingLayerIfNeeded()
{
    if (m_didInitializeRootCompositingLayer)
        return;

    auto& rootLayer = downcast<CoordinatedGraphicsLayer>(*m_rootLayer);
    m_nicosia.state.rootLayer = rootLayer.compositionLayer();
    m_state.rootCompositingLayer = rootLayer.id();
    m_didInitializeRootCompositingLayer = true;
    m_shouldSyncFrame = true;
}

void CompositingCoordinator::createRootLayer(const IntSize& size)
{
    ASSERT(!m_rootLayer);
    // Create a root layer.
    m_rootLayer = GraphicsLayer::create(this, *this);
#ifndef NDEBUG
    m_rootLayer->setName("CompositingCoordinator root layer");
#endif
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setSize(size);
}

void CompositingCoordinator::syncLayerState(CoordinatedLayerID id, CoordinatedGraphicsLayerState& state)
{
    m_shouldSyncFrame = true;
    m_state.layersToUpdate.append(std::make_pair(id, state));
}

Ref<CoordinatedImageBacking> CompositingCoordinator::createImageBackingIfNeeded(Image& image)
{
    CoordinatedImageBackingID imageID = CoordinatedImageBacking::getCoordinatedImageBackingID(image);
    auto addResult = m_imageBackings.ensure(imageID, [this, &image] {
        return CoordinatedImageBacking::create(*this, image);
    });
    return *addResult.iterator->value;
}

void CompositingCoordinator::createImageBacking(CoordinatedImageBackingID imageID)
{
    m_state.imagesToCreate.append(imageID);
}

void CompositingCoordinator::updateImageBacking(CoordinatedImageBackingID imageID, RefPtr<Nicosia::Buffer>&& buffer)
{
    m_shouldSyncFrame = true;
    m_state.imagesToUpdate.append(std::make_pair(imageID, WTFMove(buffer)));
}

void CompositingCoordinator::clearImageBackingContents(CoordinatedImageBackingID imageID)
{
    m_shouldSyncFrame = true;
    m_state.imagesToClear.append(imageID);
}

void CompositingCoordinator::removeImageBacking(CoordinatedImageBackingID imageID)
{
    if (m_isPurging)
        return;

    ASSERT(m_imageBackings.contains(imageID));
    m_imageBackings.remove(imageID);

    m_state.imagesToRemove.append(imageID);

    size_t imageIDPosition = m_state.imagesToClear.find(imageID);
    if (imageIDPosition != notFound)
        m_state.imagesToClear.remove(imageIDPosition);
}

void CompositingCoordinator::flushPendingImageBackingChanges()
{
    for (auto& imageBacking : m_imageBackings.values())
        imageBacking->update();
}

void CompositingCoordinator::notifyFlushRequired(const GraphicsLayer*)
{
    if (!m_isDestructing && !isFlushingLayerChanges())
        m_client.notifyFlushRequired();
}

float CompositingCoordinator::deviceScaleFactor() const
{
    return m_page->deviceScaleFactor();
}

float CompositingCoordinator::pageScaleFactor() const
{
    return m_page->pageScaleFactor();
}

std::unique_ptr<GraphicsLayer> CompositingCoordinator::createGraphicsLayer(GraphicsLayer::Type layerType, GraphicsLayerClient& client)
{
    CoordinatedGraphicsLayer* layer = new CoordinatedGraphicsLayer(layerType, client);
    layer->setCoordinator(this);
    m_nicosia.state.layers.add(layer->compositionLayer());
    m_registeredLayers.add(layer->id(), layer);
    m_state.layersToCreate.append(layer->id());
    layer->setNeedsVisibleRectAdjustment();
    notifyFlushRequired(layer);
    return std::unique_ptr<GraphicsLayer>(layer);
}

FloatRect CompositingCoordinator::visibleContentsRect() const
{
    return m_visibleContentsRect;
}

void CompositingCoordinator::setVisibleContentsRect(const FloatRect& rect)
{
    bool contentsRectDidChange = rect != m_visibleContentsRect;
    if (contentsRectDidChange) {
        m_visibleContentsRect = rect;

        for (auto& registeredLayer : m_registeredLayers.values())
            registeredLayer->setNeedsVisibleRectAdjustment();
    }

    FrameView* view = m_page->mainFrame().view();
    if (view->useFixedLayout() && contentsRectDidChange) {
        // Round the rect instead of enclosing it to make sure that its size stays
        // the same while panning. This can have nasty effects on layout.
        view->setFixedVisibleContentRect(roundedIntRect(rect));
    }
}

void CompositingCoordinator::deviceOrPageScaleFactorChanged()
{
    m_rootLayer->deviceOrPageScaleFactorChanged();
}

void CompositingCoordinator::detachLayer(CoordinatedGraphicsLayer* layer)
{
    if (m_isPurging)
        return;

    m_nicosia.state.layers.remove(layer->compositionLayer());
    m_registeredLayers.remove(layer->id());

    size_t index = m_state.layersToCreate.find(layer->id());
    if (index != notFound) {
        m_state.layersToCreate.remove(index);
        return;
    }

    m_state.layersToRemove.append(layer->id());
    notifyFlushRequired(layer);
}

void CompositingCoordinator::attachLayer(CoordinatedGraphicsLayer* layer)
{
    layer->setCoordinator(this);
    m_nicosia.state.layers.add(layer->compositionLayer());
    m_registeredLayers.add(layer->id(), layer);
    m_state.layersToCreate.append(layer->id());
    layer->setNeedsVisibleRectAdjustment();
    notifyFlushRequired(layer);
}

void CompositingCoordinator::renderNextFrame()
{
}

void CompositingCoordinator::purgeBackingStores()
{
    SetForScope<bool> purgingToggle(m_isPurging, true);

    for (auto& registeredLayer : m_registeredLayers.values())
        registeredLayer->purgeBackingStores();

    m_imageBackings.clear();
}

Nicosia::PaintingEngine& CompositingCoordinator::paintingEngine()
{
    return *m_paintingEngine;
}

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS)
