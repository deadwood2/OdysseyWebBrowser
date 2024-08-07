/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#if ENABLE(DRAG_SUPPORT) && PLATFORM(IOS)

#import "UIKitSPI.h"
#import <WebCore/DragActions.h>
#import <WebCore/DragData.h>
#import <WebCore/TextIndicator.h>
#import <WebCore/URL.h>
#import <WebCore/WebItemProviderPasteboard.h>
#import <wtf/BlockPtr.h>
#import <wtf/RetainPtr.h>
#import <wtf/Vector.h>

namespace WebCore {
struct DragItem;
}

namespace WebKit {

struct DragSourceState {
    WebCore::DragSourceAction action { WebCore::DragSourceActionNone };
    CGPoint adjustedOrigin { CGPointZero };
    CGRect dragPreviewFrameInRootViewCoordinates { CGRectZero };
    RetainPtr<UIImage> image;
    std::optional<WebCore::TextIndicatorData> indicatorData;
    String linkTitle;
    WebCore::URL linkURL;
    bool possiblyNeedsDragPreviewUpdate { true };

    NSInteger itemIdentifier { 0 };
};

class DragDropInteractionState {
public:
    bool anyActiveDragSourceIs(WebCore::DragSourceAction) const;

    // These helper methods are unique to UIDragInteraction.
    void prepareForDragSession(id <UIDragSession>, dispatch_block_t completionHandler);
    void dragSessionWillBegin();
    void stageDragItem(const WebCore::DragItem&, UIImage *);
    bool hasStagedDragSource() const;
    const DragSourceState& stagedDragSource() const { return m_stagedDragSource.value(); }
    enum class DidBecomeActive { No, Yes };
    void clearStagedDragSource(DidBecomeActive = DidBecomeActive::No);
    UITargetedDragPreview *previewForDragItem(UIDragItem *, UIView *contentView, UIView *previewContainer) const;
    void dragSessionWillDelaySetDownAnimation(dispatch_block_t completion);
    bool shouldRequestAdditionalItemForDragSession(id <UIDragSession>) const;
    void dragSessionWillRequestAdditionalItem(void (^completion)(NSArray <UIDragItem *> *));

    // These helper methods are unique to UIDropInteraction.
    void dropSessionDidEnterOrUpdate(id <UIDropSession>, const WebCore::DragData&);
    void dropSessionDidExit() { m_dropSession = nil; }
    void dropSessionWillPerformDrop() { m_isPerformingDrop = true; }

    // This is invoked when both drag and drop interactions are no longer active.
    void dragAndDropSessionsDidEnd();

    CGPoint adjustedPositionForDragEnd() const { return m_adjustedPositionForDragEnd; }
    bool didBeginDragging() const { return m_didBeginDragging; }
    bool isPerformingDrop() const { return m_isPerformingDrop; }
    id<UIDragSession> dragSession() const { return m_dragSession.get(); }
    id<UIDropSession> dropSession() const { return m_dropSession.get(); }
    BlockPtr<void()> takeDragStartCompletionBlock() { return WTFMove(m_dragStartCompletionBlock); }
    BlockPtr<void()> takeDragCancelSetDownBlock() { return WTFMove(m_dragCancelSetDownBlock); }
    BlockPtr<void(NSArray<UIDragItem *> *)> takeAddDragItemCompletionBlock() { return WTFMove(m_addDragItemCompletionBlock); }

private:
    void updatePreviewsForActiveDragSources();
    std::optional<DragSourceState> activeDragSourceForItem(UIDragItem *) const;

    CGPoint m_lastGlobalPosition { CGPointZero };
    CGPoint m_adjustedPositionForDragEnd { CGPointZero };
    bool m_didBeginDragging { false };
    bool m_isPerformingDrop { false };
    RetainPtr<id <UIDragSession>> m_dragSession;
    RetainPtr<id <UIDropSession>> m_dropSession;
    BlockPtr<void()> m_dragStartCompletionBlock;
    BlockPtr<void()> m_dragCancelSetDownBlock;
    BlockPtr<void(NSArray<UIDragItem *> *)> m_addDragItemCompletionBlock;

    std::optional<DragSourceState> m_stagedDragSource;
    Vector<DragSourceState> m_activeDragSources;
};

} // namespace WebKit

#endif // ENABLE(DRAG_SUPPORT) && PLATFORM(IOS)
