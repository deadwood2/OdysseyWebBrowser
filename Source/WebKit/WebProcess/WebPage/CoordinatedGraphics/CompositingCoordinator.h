/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef CompositingCoordinator_h
#define CompositingCoordinator_h

#if USE(COORDINATED_GRAPHICS)

#include <WebCore/CoordinatedGraphicsLayer.h>
#include <WebCore/CoordinatedGraphicsState.h>
#include <WebCore/CoordinatedImageBacking.h>
#include <WebCore/FloatPoint.h>
#include <WebCore/GraphicsLayerClient.h>
#include <WebCore/GraphicsLayerFactory.h>
#include <WebCore/IntRect.h>
#include <WebCore/NicosiaBuffer.h>
#include <WebCore/UpdateAtlas.h>

namespace Nicosia {
class PaintingEngine;
}

namespace WebCore {
class GraphicsContext;
class GraphicsLayer;
class Page;
}

namespace WebKit {

class CompositingCoordinator final : public WebCore::GraphicsLayerClient
    , public WebCore::CoordinatedGraphicsLayerClient
    , public WebCore::CoordinatedImageBacking::Client
    , public WebCore::UpdateAtlas::Client
    , public WebCore::GraphicsLayerFactory {
    WTF_MAKE_NONCOPYABLE(CompositingCoordinator);
public:
    class Client {
    public:
        virtual void didFlushRootLayer(const WebCore::FloatRect& visibleContentRect) = 0;
        virtual void notifyFlushRequired() = 0;
        virtual void commitSceneState(const WebCore::CoordinatedGraphicsState&) = 0;
        virtual void releaseUpdateAtlases(const Vector<uint32_t>&) = 0;
    };

    CompositingCoordinator(WebCore::Page*, CompositingCoordinator::Client&);
    virtual ~CompositingCoordinator();

    void invalidate();
    void clearUpdateAtlases();

    void setRootCompositingLayer(WebCore::GraphicsLayer*);
    void setViewOverlayRootLayer(WebCore::GraphicsLayer*);
    void sizeDidChange(const WebCore::IntSize&);
    void deviceOrPageScaleFactorChanged();

    void setVisibleContentsRect(const WebCore::FloatRect&, const WebCore::FloatPoint&);
    void renderNextFrame();
    void commitScrollOffset(uint32_t layerID, const WebCore::IntSize& offset);

    void createRootLayer(const WebCore::IntSize&);
    WebCore::GraphicsLayer* rootLayer() const { return m_rootLayer.get(); }
    WebCore::GraphicsLayer* rootCompositingLayer() const { return m_rootCompositingLayer; }
    WebCore::CoordinatedGraphicsLayer* mainContentsLayer();

    void forceFrameSync() { m_shouldSyncFrame = true; }

    bool flushPendingLayerChanges();
    WebCore::CoordinatedGraphicsState& state() { return m_state; }

    void syncDisplayState();

    double nextAnimationServiceTime() const;

private:
    enum ReleaseAtlasPolicy {
        ReleaseInactive,
        ReleaseUnused
    };

    // GraphicsLayerClient
    void notifyFlushRequired(const WebCore::GraphicsLayer*) override;
    float deviceScaleFactor() const override;
    float pageScaleFactor() const override;

    // CoordinatedImageBacking::Client
    void createImageBacking(WebCore::CoordinatedImageBackingID) override;
    void updateImageBacking(WebCore::CoordinatedImageBackingID, RefPtr<Nicosia::Buffer>&&) override;
    void clearImageBackingContents(WebCore::CoordinatedImageBackingID) override;
    void removeImageBacking(WebCore::CoordinatedImageBackingID) override;

    // CoordinatedGraphicsLayerClient
    bool isFlushingLayerChanges() const override { return m_isFlushingLayerChanges; }
    WebCore::FloatRect visibleContentsRect() const override;
    Ref<WebCore::CoordinatedImageBacking> createImageBackingIfNeeded(WebCore::Image&) override;
    void detachLayer(WebCore::CoordinatedGraphicsLayer*) override;
    void attachLayer(WebCore::CoordinatedGraphicsLayer*) override;
    Ref<Nicosia::Buffer> getCoordinatedBuffer(const WebCore::IntSize&, Nicosia::Buffer::Flags, uint32_t&, WebCore::IntRect&) override;
    Nicosia::PaintingEngine& paintingEngine() override;
    void syncLayerState(WebCore::CoordinatedLayerID, WebCore::CoordinatedGraphicsLayerState&) override;

    // UpdateAtlas::Client
    void createUpdateAtlas(WebCore::UpdateAtlas::ID, Ref<Nicosia::Buffer>&&) override;
    void removeUpdateAtlas(WebCore::UpdateAtlas::ID) override;

    // GraphicsLayerFactory
    std::unique_ptr<WebCore::GraphicsLayer> createGraphicsLayer(WebCore::GraphicsLayer::Type, WebCore::GraphicsLayerClient&) override;

    void initializeRootCompositingLayerIfNeeded();
    void flushPendingImageBackingChanges();
    void clearPendingStateChanges();

    void purgeBackingStores();

    void scheduleReleaseInactiveAtlases();
    void releaseInactiveAtlasesTimerFired();
    void releaseAtlases(ReleaseAtlasPolicy);

    double timestamp() const;

    WebCore::Page* m_page;
    CompositingCoordinator::Client& m_client;

    std::unique_ptr<WebCore::GraphicsLayer> m_rootLayer;
    WebCore::GraphicsLayer* m_rootCompositingLayer { nullptr };
    WebCore::GraphicsLayer* m_overlayCompositingLayer { nullptr };

    WebCore::CoordinatedGraphicsState m_state;

    HashMap<WebCore::CoordinatedLayerID, WebCore::CoordinatedGraphicsLayer*> m_registeredLayers;
    HashMap<WebCore::CoordinatedImageBackingID, RefPtr<WebCore::CoordinatedImageBacking>> m_imageBackings;

    std::unique_ptr<Nicosia::PaintingEngine> m_paintingEngine;
    Vector<std::unique_ptr<WebCore::UpdateAtlas>> m_updateAtlases;
    Vector<uint32_t> m_atlasesToRemove;

    // We don't send the messages related to releasing resources to renderer during purging, because renderer already had removed all resources.
    bool m_isDestructing { false };
    bool m_isPurging { false };
    bool m_isFlushingLayerChanges { false };
    bool m_shouldSyncFrame { false };
    bool m_didInitializeRootCompositingLayer { false };

    WebCore::FloatRect m_visibleContentsRect;
    RunLoop::Timer<CompositingCoordinator> m_releaseInactiveAtlasesTimer;

    double m_lastAnimationServiceTime { 0 };
};

}

#endif // namespace WebKit

#endif // CompositingCoordinator_h
