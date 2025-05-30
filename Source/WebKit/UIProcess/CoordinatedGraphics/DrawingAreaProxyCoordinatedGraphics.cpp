/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2016-2019 Igalia S.L.
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
#include "DrawingAreaProxyCoordinatedGraphics.h"

#include "DrawingAreaMessages.h"
#include "DrawingAreaProxyMessages.h"
#include "LayerTreeContext.h"
#include "UpdateInfo.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include "WebProcessProxy.h"
#include <WebCore/Region.h>

#if PLATFORM(GTK)
#include <gtk/gtk.h>
#endif

#if PLATFORM(WAYLAND)
#include "WaylandCompositor.h"
#include <WebCore/PlatformDisplay.h>
#endif

#if USE(GLIB_EVENT_LOOP)
#include <wtf/glib/RunLoopSourcePriority.h>
#endif

namespace WebKit {
using namespace WebCore;

DrawingAreaProxyCoordinatedGraphics::DrawingAreaProxyCoordinatedGraphics(WebPageProxy& webPageProxy, WebProcessProxy& process)
    : DrawingAreaProxy(DrawingAreaTypeCoordinatedGraphics, webPageProxy, process)
#if !PLATFORM(WPE)
    , m_discardBackingStoreTimer(RunLoop::current(), this, &DrawingAreaProxyCoordinatedGraphics::discardBackingStore)
#endif
{
#if USE(GLIB_EVENT_LOOP) && !PLATFORM(WPE)
    m_discardBackingStoreTimer.setPriority(RunLoopSourcePriority::ReleaseUnusedResourcesTimer);
#endif
}

DrawingAreaProxyCoordinatedGraphics::~DrawingAreaProxyCoordinatedGraphics()
{
    // Make sure to exit accelerated compositing mode.
    if (isInAcceleratedCompositingMode())
        exitAcceleratedCompositingMode();
}

#if !PLATFORM(WPE)
void DrawingAreaProxyCoordinatedGraphics::paint(BackingStore::PlatformGraphicsContext context, const IntRect& rect, Region& unpaintedRegion)
{
    unpaintedRegion = rect;

    if (isInAcceleratedCompositingMode())
        return;

    ASSERT(m_currentBackingStoreStateID <= m_nextBackingStoreStateID);
    if (m_currentBackingStoreStateID < m_nextBackingStoreStateID) {
        // Tell the web process to do a full backing store update now, in case we previously told
        // it about our next state but didn't request an immediate update.
        sendUpdateBackingStoreState(RespondImmediately);

        // If we haven't yet received our first bits from the WebProcess then don't paint anything.
        if (!m_hasReceivedFirstUpdate)
            return;

        if (m_isWaitingForDidUpdateBackingStoreState) {
            // Wait for a DidUpdateBackingStoreState message that contains the new bits before we paint
            // what's currently in the backing store.
            waitForAndDispatchDidUpdateBackingStoreState();
        }

        // Dispatching DidUpdateBackingStoreState (either beneath sendUpdateBackingStoreState or
        // beneath waitForAndDispatchDidUpdateBackingStoreState) could destroy our backing store or
        // change the compositing mode.
        if (!m_backingStore || isInAcceleratedCompositingMode())
            return;
    } else {
        ASSERT(!m_isWaitingForDidUpdateBackingStoreState);
        if (!m_backingStore) {
            // The view has asked us to paint before the web process has painted anything. There's
            // nothing we can do.
            return;
        }
    }

    m_backingStore->paint(context, rect);
    unpaintedRegion.subtract(IntRect(IntPoint(), m_backingStore->size()));

    discardBackingStoreSoon();
}
#endif

void DrawingAreaProxyCoordinatedGraphics::sizeDidChange()
{
    backingStoreStateDidChange(RespondImmediately);
}

void DrawingAreaProxyCoordinatedGraphics::deviceScaleFactorDidChange()
{
    backingStoreStateDidChange(RespondImmediately);
}

void DrawingAreaProxyCoordinatedGraphics::waitForBackingStoreUpdateOnNextPaint()
{
    m_hasReceivedFirstUpdate = true;
}

void DrawingAreaProxyCoordinatedGraphics::setBackingStoreIsDiscardable(bool isBackingStoreDiscardable)
{
#if !PLATFORM(WPE)
    if (m_isBackingStoreDiscardable == isBackingStoreDiscardable)
        return;

    m_isBackingStoreDiscardable = isBackingStoreDiscardable;
    if (m_isBackingStoreDiscardable)
        discardBackingStoreSoon();
    else
        m_discardBackingStoreTimer.stop();
#endif
}

void DrawingAreaProxyCoordinatedGraphics::update(uint64_t backingStoreStateID, const UpdateInfo& updateInfo)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_currentBackingStoreStateID);
    if (backingStoreStateID < m_currentBackingStoreStateID)
        return;

    // FIXME: Handle the case where the view is hidden.

#if !PLATFORM(WPE)
    incorporateUpdate(updateInfo);
#endif
    process().send(Messages::DrawingArea::DidUpdate(), m_webPageProxy.pageID());
}

void DrawingAreaProxyCoordinatedGraphics::didUpdateBackingStoreState(uint64_t backingStoreStateID, const UpdateInfo& updateInfo, const LayerTreeContext& layerTreeContext)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_nextBackingStoreStateID);
    ASSERT_ARG(backingStoreStateID, backingStoreStateID > m_currentBackingStoreStateID);
    m_currentBackingStoreStateID = backingStoreStateID;

    m_isWaitingForDidUpdateBackingStoreState = false;

    // Stop the responsiveness timer that was started in sendUpdateBackingStoreState.
    process().responsivenessTimer().stop();

    if (layerTreeContext != m_layerTreeContext) {
        if (layerTreeContext.isEmpty() && !m_layerTreeContext.isEmpty()) {
            exitAcceleratedCompositingMode();
            ASSERT(m_layerTreeContext.isEmpty());
        } else if (!layerTreeContext.isEmpty() && m_layerTreeContext.isEmpty()) {
            enterAcceleratedCompositingMode(layerTreeContext);
            ASSERT(layerTreeContext == m_layerTreeContext);
        } else {
            updateAcceleratedCompositingMode(layerTreeContext);
            ASSERT(layerTreeContext == m_layerTreeContext);
        }
    }

    if (m_nextBackingStoreStateID != m_currentBackingStoreStateID)
        sendUpdateBackingStoreState(RespondImmediately);
    else {
        m_hasReceivedFirstUpdate = true;

#if USE(TEXTURE_MAPPER_GL) && PLATFORM(GTK) && PLATFORM(X11) && !USE(REDIRECTED_XCOMPOSITE_WINDOW)
        if (m_pendingNativeSurfaceHandleForCompositing) {
            setNativeSurfaceHandleForCompositing(m_pendingNativeSurfaceHandleForCompositing);
            m_pendingNativeSurfaceHandleForCompositing = 0;
        }
#endif
    }

#if !PLATFORM(WPE)
    if (isInAcceleratedCompositingMode()) {
        ASSERT(!m_backingStore);
        return;
    }

    // If we have a backing store the right size, reuse it.
    if (m_backingStore && (m_backingStore->size() != updateInfo.viewSize || m_backingStore->deviceScaleFactor() != updateInfo.deviceScaleFactor))
        m_backingStore = nullptr;
    incorporateUpdate(updateInfo);
#endif
}

void DrawingAreaProxyCoordinatedGraphics::enterAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext& layerTreeContext)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_currentBackingStoreStateID);
    if (backingStoreStateID < m_currentBackingStoreStateID)
        return;

    enterAcceleratedCompositingMode(layerTreeContext);
}

void DrawingAreaProxyCoordinatedGraphics::exitAcceleratedCompositingMode(uint64_t backingStoreStateID, const UpdateInfo& updateInfo)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_currentBackingStoreStateID);
    if (backingStoreStateID < m_currentBackingStoreStateID)
        return;

    exitAcceleratedCompositingMode();
#if !PLATFORM(WPE)
    incorporateUpdate(updateInfo);
#endif
}

void DrawingAreaProxyCoordinatedGraphics::updateAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext& layerTreeContext)
{
    ASSERT_ARG(backingStoreStateID, backingStoreStateID <= m_currentBackingStoreStateID);
    if (backingStoreStateID < m_currentBackingStoreStateID)
        return;

    updateAcceleratedCompositingMode(layerTreeContext);
}

#if !PLATFORM(WPE)
void DrawingAreaProxyCoordinatedGraphics::incorporateUpdate(const UpdateInfo& updateInfo)
{
    ASSERT(!isInAcceleratedCompositingMode());

    if (updateInfo.updateRectBounds.isEmpty())
        return;

    if (!m_backingStore)
        m_backingStore = std::make_unique<BackingStore>(updateInfo.viewSize, updateInfo.deviceScaleFactor, m_webPageProxy);

    m_backingStore->incorporateUpdate(updateInfo);

    Region damageRegion;
    if (updateInfo.scrollRect.isEmpty()) {
        for (const auto& rect : updateInfo.updateRects)
            damageRegion.unite(rect);
    } else
        damageRegion = IntRect(IntPoint(), m_webPageProxy.viewSize());
    m_webPageProxy.setViewNeedsDisplay(damageRegion);
}
#endif

bool DrawingAreaProxyCoordinatedGraphics::alwaysUseCompositing() const
{
    return m_webPageProxy.preferences().acceleratedCompositingEnabled() && m_webPageProxy.preferences().forceCompositingMode();
}

void DrawingAreaProxyCoordinatedGraphics::enterAcceleratedCompositingMode(const LayerTreeContext& layerTreeContext)
{
    ASSERT(!isInAcceleratedCompositingMode());
#if !PLATFORM(WPE)
    m_backingStore = nullptr;
#endif
    m_layerTreeContext = layerTreeContext;
    m_webPageProxy.enterAcceleratedCompositingMode(layerTreeContext);
}

void DrawingAreaProxyCoordinatedGraphics::exitAcceleratedCompositingMode()
{
    ASSERT(isInAcceleratedCompositingMode());

    m_layerTreeContext = { };
    m_webPageProxy.exitAcceleratedCompositingMode();
}

void DrawingAreaProxyCoordinatedGraphics::updateAcceleratedCompositingMode(const LayerTreeContext& layerTreeContext)
{
    ASSERT(isInAcceleratedCompositingMode());

    m_layerTreeContext = layerTreeContext;
    m_webPageProxy.updateAcceleratedCompositingMode(layerTreeContext);
}

void DrawingAreaProxyCoordinatedGraphics::backingStoreStateDidChange(RespondImmediatelyOrNot respondImmediatelyOrNot)
{
    ++m_nextBackingStoreStateID;
    sendUpdateBackingStoreState(respondImmediatelyOrNot);
}

void DrawingAreaProxyCoordinatedGraphics::sendUpdateBackingStoreState(RespondImmediatelyOrNot respondImmediatelyOrNot)
{
    ASSERT(m_currentBackingStoreStateID < m_nextBackingStoreStateID);

    if (!m_webPageProxy.isValid())
        return;

    if (m_isWaitingForDidUpdateBackingStoreState)
        return;

    if (m_webPageProxy.viewSize().isEmpty() && !m_webPageProxy.useFixedLayout())
        return;

    m_isWaitingForDidUpdateBackingStoreState = respondImmediatelyOrNot == RespondImmediately;

    process().send(Messages::DrawingArea::UpdateBackingStoreState(m_nextBackingStoreStateID, respondImmediatelyOrNot == RespondImmediately, m_webPageProxy.deviceScaleFactor(), m_size, m_scrollOffset), m_webPageProxy.pageID());

    m_scrollOffset = IntSize();

    if (m_isWaitingForDidUpdateBackingStoreState) {
        // Start the responsiveness timer. We will stop it when we hear back from the WebProcess
        // in didUpdateBackingStoreState.
        process().responsivenessTimer().start();
    }

    if (m_isWaitingForDidUpdateBackingStoreState && !m_layerTreeContext.isEmpty()) {
        // Wait for the DidUpdateBackingStoreState message. Normally we do this in DrawingAreaProxyCoordinatedGraphics::paint, but that
        // function is never called when in accelerated compositing mode.
        waitForAndDispatchDidUpdateBackingStoreState();
    }
}

void DrawingAreaProxyCoordinatedGraphics::waitForAndDispatchDidUpdateBackingStoreState()
{
    ASSERT(m_isWaitingForDidUpdateBackingStoreState);

    if (!m_webPageProxy.isValid())
        return;
    if (process().state() == WebProcessProxy::State::Launching)
        return;
    if (!m_webPageProxy.isViewVisible())
        return;
#if PLATFORM(WAYLAND) && USE(EGL)
    // Never block the UI process in Wayland when waiting for DidUpdateBackingStoreState after a resize,
    // because the nested compositor needs to handle the web process requests that happens while resizing.
    if (PlatformDisplay::sharedDisplay().type() == PlatformDisplay::Type::Wayland && isInAcceleratedCompositingMode())
        return;
#endif

    // FIXME: waitForAndDispatchImmediately will always return the oldest DidUpdateBackingStoreState message that
    // hasn't yet been processed. But it might be better to skip ahead to some other DidUpdateBackingStoreState
    // message, if multiple DidUpdateBackingStoreState messages are waiting to be processed. For instance, we could
    // choose the most recent one, or the one that is closest to our current size.

    // The timeout, in seconds, we use when waiting for a DidUpdateBackingStoreState message when we're asked to paint.
    process().connection()->waitForAndDispatchImmediately<Messages::DrawingAreaProxy::DidUpdateBackingStoreState>(m_webPageProxy.pageID(), Seconds::fromMilliseconds(500));
}

#if !PLATFORM(WPE)
void DrawingAreaProxyCoordinatedGraphics::discardBackingStoreSoon()
{
    if (!m_backingStore || !m_isBackingStoreDiscardable || m_discardBackingStoreTimer.isActive())
        return;

    // We'll wait this many seconds after the last paint before throwing away our backing store to save memory.
    // FIXME: It would be smarter to make this delay based on how expensive painting is. See <http://webkit.org/b/55733>.
    static const Seconds discardBackingStoreDelay = 2_s;

    m_discardBackingStoreTimer.startOneShot(discardBackingStoreDelay);
}

void DrawingAreaProxyCoordinatedGraphics::discardBackingStore()
{
    if (!m_backingStore)
        return;
    m_backingStore = nullptr;
    backingStoreStateDidChange(DoNotRespondImmediately);
}
#endif

#if USE(TEXTURE_MAPPER_GL) && PLATFORM(GTK) && PLATFORM(X11) && !USE(REDIRECTED_XCOMPOSITE_WINDOW)
void DrawingAreaProxyCoordinatedGraphics::setNativeSurfaceHandleForCompositing(uint64_t handle)
{
    if (!m_hasReceivedFirstUpdate) {
        m_pendingNativeSurfaceHandleForCompositing = handle;
        return;
    }
    process().send(Messages::DrawingArea::SetNativeSurfaceHandleForCompositing(handle), m_webPageProxy.pageID(), IPC::SendOption::DispatchMessageEvenWhenWaitingForSyncReply);
}

void DrawingAreaProxyCoordinatedGraphics::destroyNativeSurfaceHandleForCompositing()
{
    if (m_pendingNativeSurfaceHandleForCompositing) {
        m_pendingNativeSurfaceHandleForCompositing = 0;
        return;
    }
    bool handled;
    process().sendSync(Messages::DrawingArea::DestroyNativeSurfaceHandleForCompositing(), Messages::DrawingArea::DestroyNativeSurfaceHandleForCompositing::Reply(handled), m_webPageProxy.pageID());
}
#endif

DrawingAreaProxyCoordinatedGraphics::DrawingMonitor::DrawingMonitor(WebPageProxy& webPage)
    : m_timer(RunLoop::main(), this, &DrawingMonitor::stop)
#if PLATFORM(GTK)
    , m_webPage(webPage)
#endif
{
#if USE(GLIB_EVENT_LOOP)
#if PLATFORM(GTK)
    // Give redraws more priority.
    m_timer.setPriority(GDK_PRIORITY_REDRAW - 10);
#else
    m_timer.setPriority(RunLoopSourcePriority::RunLoopDispatcher);
#endif
#endif
}

DrawingAreaProxyCoordinatedGraphics::DrawingMonitor::~DrawingMonitor()
{
    m_callback = nullptr;
    stop();
}

int DrawingAreaProxyCoordinatedGraphics::DrawingMonitor::webViewDrawCallback(DrawingAreaProxyCoordinatedGraphics::DrawingMonitor* monitor)
{
    monitor->didDraw();
    return FALSE;
}

void DrawingAreaProxyCoordinatedGraphics::DrawingMonitor::start(WTF::Function<void(CallbackBase::Error)>&& callback)
{
    m_startTime = MonotonicTime::now();
    m_callback = WTFMove(callback);
#if PLATFORM(GTK)
    g_signal_connect_swapped(m_webPage.viewWidget(), "draw", reinterpret_cast<GCallback>(webViewDrawCallback), this);
    m_timer.startOneShot(1_s);
#else
    m_timer.startOneShot(0_s);
#endif
}

void DrawingAreaProxyCoordinatedGraphics::DrawingMonitor::stop()
{
    m_timer.stop();
#if PLATFORM(GTK)
    g_signal_handlers_disconnect_by_func(m_webPage.viewWidget(), reinterpret_cast<gpointer>(webViewDrawCallback), this);
#endif
    m_startTime = MonotonicTime();
    if (m_callback) {
        m_callback(CallbackBase::Error::None);
        m_callback = nullptr;
    }
}

void DrawingAreaProxyCoordinatedGraphics::DrawingMonitor::didDraw()
{
    // We wait up to 1 second for draw events. If there are several draw events queued quickly,
    // we want to wait until all of them have been processed, so after receiving a draw, we wait
    // up to 100ms for the next one or stop.
    if (MonotonicTime::now() - m_startTime > 1_s)
        stop();
    else
        m_timer.startOneShot(100_ms);
}

void DrawingAreaProxyCoordinatedGraphics::dispatchAfterEnsuringDrawing(WTF::Function<void(CallbackBase::Error)>&& callbackFunction)
{
    if (!m_webPageProxy.isValid()) {
        callbackFunction(CallbackBase::Error::OwnerWasInvalidated);
        return;
    }

    if (!m_drawingMonitor)
        m_drawingMonitor = std::make_unique<DrawingAreaProxyCoordinatedGraphics::DrawingMonitor>(m_webPageProxy);
    m_drawingMonitor->start(WTFMove(callbackFunction));
}

} // namespace WebKit
