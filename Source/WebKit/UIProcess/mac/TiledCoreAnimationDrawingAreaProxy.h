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

#pragma once

#if !PLATFORM(IOS)

#include "DrawingAreaProxy.h"

namespace WebKit {

class TiledCoreAnimationDrawingAreaProxy : public DrawingAreaProxy {
public:
    explicit TiledCoreAnimationDrawingAreaProxy(WebPageProxy&);
    virtual ~TiledCoreAnimationDrawingAreaProxy();

private:
    // DrawingAreaProxy
    void deviceScaleFactorDidChange() override;
    void sizeDidChange() override;
    void colorSpaceDidChange() override;
    void minimumLayoutSizeDidChange() override;

    void enterAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&) override;
    void exitAcceleratedCompositingMode(uint64_t backingStoreStateID, const UpdateInfo&) override;
    void updateAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&) override;

    void adjustTransientZoom(double scale, WebCore::FloatPoint origin) override;
    void commitTransientZoom(double scale, WebCore::FloatPoint origin) override;

    void waitForDidUpdateActivityState() override;
    void dispatchAfterEnsuringDrawing(WTF::Function<void (CallbackBase::Error)>&&) override;
    void dispatchPresentationCallbacksAfterFlushingLayers(const Vector<CallbackID>&) final;

    void willSendUpdateGeometry() override;

    WebCore::MachSendRight createFence() override;

    // Message handlers.
    void didUpdateGeometry() override;
    void intrinsicContentSizeDidChange(const WebCore::IntSize&) override;

    void sendUpdateGeometry();

    // Whether we're waiting for a DidUpdateGeometry message from the web process.
    bool m_isWaitingForDidUpdateGeometry;

    // The last size we sent to the web process.
    WebCore::IntSize m_lastSentSize;

    // The last minimum layout size we sent to the web process.
    WebCore::IntSize m_lastSentMinimumLayoutSize;

    CallbackMap m_callbacks;
};

} // namespace WebKit

SPECIALIZE_TYPE_TRAITS_DRAWING_AREA_PROXY(TiledCoreAnimationDrawingAreaProxy, DrawingAreaTypeTiledCoreAnimation)

#endif // !PLATFORM(IOS)
