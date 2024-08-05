/*
 * Copyright (C) 2014 Igalia S.L.
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
#include "PageClientImpl.h"

#include "AcceleratedDrawingAreaProxy.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "ScrollGestureController.h"
#include "WPEView.h"
#include "WebContextMenuProxy.h"
#include <WebCore/ActivityState.h>
#include <WebCore/NotImplemented.h>

namespace WebKit {

PageClientImpl::PageClientImpl(WKWPE::View& view)
    : m_view(view)
    , m_scrollGestureController(std::make_unique<ScrollGestureController>())
{
}

PageClientImpl::~PageClientImpl() = default;

std::unique_ptr<DrawingAreaProxy> PageClientImpl::createDrawingAreaProxy()
{
    return std::make_unique<AcceleratedDrawingAreaProxy>(m_view.page());
}

void PageClientImpl::setViewNeedsDisplay(const WebCore::Region&)
{
}

void PageClientImpl::requestScroll(const WebCore::FloatPoint&, const WebCore::IntPoint&, bool)
{
}

WebCore::FloatPoint PageClientImpl::viewScrollPosition()
{
    return { };
}

WebCore::IntSize PageClientImpl::viewSize()
{
    return m_view.size();
}

bool PageClientImpl::isViewWindowActive()
{
    return m_view.viewState() & WebCore::ActivityState::WindowIsActive;
}

bool PageClientImpl::isViewFocused()
{
    return m_view.viewState() & WebCore::ActivityState::IsFocused;
}

bool PageClientImpl::isViewVisible()
{
    return m_view.viewState() & WebCore::ActivityState::IsVisible;
}

bool PageClientImpl::isViewInWindow()
{
    return m_view.viewState() & WebCore::ActivityState::IsInWindow;
}

void PageClientImpl::processDidExit()
{
}

void PageClientImpl::didRelaunchProcess()
{
}

void PageClientImpl::pageClosed()
{
}

void PageClientImpl::preferencesDidChange()
{
}

void PageClientImpl::toolTipChanged(const String&, const String&)
{
}

void PageClientImpl::didCommitLoadForMainFrame(const String&, bool)
{
}

void PageClientImpl::handleDownloadRequest(DownloadProxy* download)
{
    ASSERT(download);
    m_view.handleDownloadRequest(*download);
}

void PageClientImpl::didChangeContentSize(const WebCore::IntSize&)
{
}

void PageClientImpl::setCursor(const WebCore::Cursor&)
{
}

void PageClientImpl::setCursorHiddenUntilMouseMoves(bool)
{
}

void PageClientImpl::didChangeViewportProperties(const WebCore::ViewportAttributes&)
{
}

void PageClientImpl::registerEditCommand(Ref<WebEditCommandProxy>&&, WebPageProxy::UndoOrRedo)
{
}

void PageClientImpl::clearAllEditCommands()
{
}

bool PageClientImpl::canUndoRedo(WebPageProxy::UndoOrRedo)
{
    return false;
}

void PageClientImpl::executeUndoRedo(WebPageProxy::UndoOrRedo)
{
}

WebCore::FloatRect PageClientImpl::convertToDeviceSpace(const WebCore::FloatRect& rect)
{
    return rect;
}

WebCore::FloatRect PageClientImpl::convertToUserSpace(const WebCore::FloatRect& rect)
{
    return rect;
}

WebCore::IntPoint PageClientImpl::screenToRootView(const WebCore::IntPoint& point)
{
    return point;
}

WebCore::IntRect PageClientImpl::rootViewToScreen(const WebCore::IntRect& rect)
{
    return rect;
}

void PageClientImpl::doneWithKeyEvent(const NativeWebKeyboardEvent&, bool)
{
}

void PageClientImpl::doneWithTouchEvent(const NativeWebTouchEvent& touchEvent, bool wasEventHandled)
{
    if (wasEventHandled)
        return;

    const struct wpe_input_touch_event_raw* touchPoint = touchEvent.nativeFallbackTouchPoint();
    if (touchPoint->type == wpe_input_touch_event_type_null)
        return;

    auto& page = m_view.page();

    if (m_scrollGestureController->handleEvent(touchPoint)) {
        struct wpe_input_axis_event* axisEvent = m_scrollGestureController->axisEvent();
        if (axisEvent->type != wpe_input_axis_event_type_null)
            page.handleWheelEvent(WebKit::NativeWebWheelEvent(axisEvent, m_view.page().deviceScaleFactor()));
        return;
    }

    struct wpe_input_pointer_event pointerEvent {
        wpe_input_pointer_event_type_null, touchPoint->time,
        touchPoint->x, touchPoint->y,
        1, 0
    };

    switch (touchPoint->type) {
    case wpe_input_touch_event_type_down:
        pointerEvent.type = wpe_input_pointer_event_type_button;
        pointerEvent.state = 1;
        break;
    case wpe_input_touch_event_type_motion:
        pointerEvent.type = wpe_input_pointer_event_type_motion;
        pointerEvent.state = 1;
        break;
    case wpe_input_touch_event_type_up:
        pointerEvent.type = wpe_input_pointer_event_type_button;
        pointerEvent.state = 0;
        break;
    case wpe_input_pointer_event_type_null:
        ASSERT_NOT_REACHED();
        return;
    }

    page.handleMouseEvent(NativeWebMouseEvent(&pointerEvent, page.deviceScaleFactor()));
}

void PageClientImpl::wheelEventWasNotHandledByWebCore(const NativeWebWheelEvent&)
{
}

RefPtr<WebPopupMenuProxy> PageClientImpl::createPopupMenuProxy(WebPageProxy&)
{
    return nullptr;
}

#if ENABLE(CONTEXT_MENUS)
RefPtr<WebContextMenuProxy> PageClientImpl::createContextMenuProxy(WebPageProxy&, const ContextMenuContextData&, const UserData&)
{
    return nullptr;
}
#endif

void PageClientImpl::enterAcceleratedCompositingMode(const LayerTreeContext&)
{
}

void PageClientImpl::exitAcceleratedCompositingMode()
{
}

void PageClientImpl::updateAcceleratedCompositingMode(const LayerTreeContext&)
{
}

void PageClientImpl::didFinishLoadingDataForCustomContentProvider(const String&, const IPC::DataReference&)
{
}

void PageClientImpl::navigationGestureDidBegin()
{
}

void PageClientImpl::navigationGestureWillEnd(bool, WebBackForwardListItem&)
{
}

void PageClientImpl::navigationGestureDidEnd(bool, WebBackForwardListItem&)
{
}

void PageClientImpl::navigationGestureDidEnd()
{
}

void PageClientImpl::willRecordNavigationSnapshot(WebBackForwardListItem&)
{
}

void PageClientImpl::didRemoveNavigationGestureSnapshot()
{
}

void PageClientImpl::didFirstVisuallyNonEmptyLayoutForMainFrame()
{
}

void PageClientImpl::didFinishLoadForMainFrame()
{
}

void PageClientImpl::didFailLoadForMainFrame()
{
}

void PageClientImpl::didSameDocumentNavigationForMainFrame(SameDocumentNavigationType)
{
}

void PageClientImpl::didChangeBackgroundColor()
{
}

void PageClientImpl::refView()
{
}

void PageClientImpl::derefView()
{
}

#if ENABLE(VIDEO)
bool PageClientImpl::decidePolicyForInstallMissingMediaPluginsPermissionRequest(InstallMissingMediaPluginsPermissionRequest&)
{
    return false;
}
#endif

void PageClientImpl::didRestoreScrollPosition()
{
}

WebCore::UserInterfaceLayoutDirection PageClientImpl::userInterfaceLayoutDirection()
{
    return WebCore::UserInterfaceLayoutDirection::LTR;
}

JSGlobalContextRef PageClientImpl::javascriptGlobalContext()
{
    return m_view.javascriptGlobalContext();
}

} // namespace WebKit
