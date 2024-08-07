/*
 * Copyright (C) 2010, 2015-2016 Apple Inc. All rights reserved.
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
#include "InjectedBundleRangeHandle.h"

#include "InjectedBundleNodeHandle.h"
#include "ShareableBitmap.h"
#include "WebImage.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/HeapInlines.h>
#include <WebCore/Document.h>
#include <WebCore/FloatRect.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameSelection.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/IntRect.h>
#include <WebCore/JSRange.h>
#include <WebCore/Page.h>
#include <WebCore/Range.h>
#include <WebCore/VisibleSelection.h>
#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>

using namespace WebCore;

namespace WebKit {

typedef HashMap<Range*, InjectedBundleRangeHandle*> DOMHandleCache;

static DOMHandleCache& domHandleCache()
{
    static NeverDestroyed<DOMHandleCache> cache;
    return cache;
}

RefPtr<InjectedBundleRangeHandle> InjectedBundleRangeHandle::getOrCreate(JSContextRef context, JSObjectRef object)
{
    Range* range = JSRange::toWrapped(toJS(context)->vm(), toJS(object));
    return getOrCreate(range);
}

RefPtr<InjectedBundleRangeHandle> InjectedBundleRangeHandle::getOrCreate(Range* range)
{
    if (!range)
        return nullptr;

    DOMHandleCache::AddResult result = domHandleCache().add(range, nullptr);
    if (!result.isNewEntry)
        return result.iterator->value;

    auto rangeHandle = InjectedBundleRangeHandle::create(*range);
    result.iterator->value = rangeHandle.ptr();
    return WTFMove(rangeHandle);
}

Ref<InjectedBundleRangeHandle> InjectedBundleRangeHandle::create(Range& range)
{
    return adoptRef(*new InjectedBundleRangeHandle(range));
}

InjectedBundleRangeHandle::InjectedBundleRangeHandle(Range& range)
    : m_range(range)
{
}

InjectedBundleRangeHandle::~InjectedBundleRangeHandle()
{
    domHandleCache().remove(m_range.ptr());
}

Range& InjectedBundleRangeHandle::coreRange() const
{
    return m_range.get();
}

Ref<InjectedBundleNodeHandle> InjectedBundleRangeHandle::document()
{
    return InjectedBundleNodeHandle::getOrCreate(m_range->ownerDocument());
}

WebCore::IntRect InjectedBundleRangeHandle::boundingRectInWindowCoordinates() const
{
    FloatRect boundingRect = m_range->absoluteBoundingRect();
    Frame* frame = m_range->ownerDocument().frame();
    return frame->view()->contentsToWindow(enclosingIntRect(boundingRect));
}

RefPtr<WebImage> InjectedBundleRangeHandle::renderedImage(SnapshotOptions options)
{
    Document& ownerDocument = m_range->ownerDocument();
    Frame* frame = ownerDocument.frame();
    if (!frame)
        return nullptr;

    FrameView* frameView = frame->view();
    if (!frameView)
        return nullptr;

    Ref<Frame> protector(*frame);

    VisibleSelection oldSelection = frame->selection().selection();
    frame->selection().setSelection(VisibleSelection(m_range));

    float scaleFactor = (options & SnapshotOptionsExcludeDeviceScaleFactor) ? 1 : frame->page()->deviceScaleFactor();
    IntRect paintRect = enclosingIntRect(m_range->absoluteBoundingRect());
    IntSize backingStoreSize = paintRect.size();
    backingStoreSize.scale(scaleFactor);

    RefPtr<ShareableBitmap> backingStore = ShareableBitmap::createShareable(backingStoreSize, { });
    if (!backingStore)
        return nullptr;

    auto graphicsContext = backingStore->createGraphicsContext();
    graphicsContext->scale(scaleFactor);

    paintRect.move(frameView->frameRect().x(), frameView->frameRect().y());
    paintRect.moveBy(-frameView->scrollPosition());

    graphicsContext->translate(-paintRect.location());

    PaintBehavior oldPaintBehavior = frameView->paintBehavior();
    PaintBehavior paintBehavior = oldPaintBehavior | PaintBehaviorSelectionOnly | PaintBehaviorFlattenCompositingLayers | PaintBehaviorSnapshotting;
    if (options & SnapshotOptionsForceBlackText)
        paintBehavior |= PaintBehaviorForceBlackText;
    if (options & SnapshotOptionsForceWhiteText)
        paintBehavior |= PaintBehaviorForceWhiteText;

    frameView->setPaintBehavior(paintBehavior);
    ownerDocument.updateLayout();

    frameView->paint(*graphicsContext, paintRect);
    frameView->setPaintBehavior(oldPaintBehavior);

    frame->selection().setSelection(oldSelection);

    return WebImage::create(backingStore.releaseNonNull());
}

String InjectedBundleRangeHandle::text() const
{
    return m_range->text();
}

} // namespace WebKit
