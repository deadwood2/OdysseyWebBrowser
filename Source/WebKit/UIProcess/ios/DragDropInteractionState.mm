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

#include "config.h"
#include "DragDropInteractionState.h"

#if ENABLE(DRAG_SUPPORT) && PLATFORM(IOS)

#import <WebCore/DragItem.h>
#import <WebCore/Image.h>

using namespace WebCore;
using namespace WebKit;

namespace WebKit {

static UIDragItem *dragItemMatchingIdentifier(id <UIDragSession> session, NSInteger identifier)
{
    for (UIDragItem *item in session.items) {
        id context = item.privateLocalContext;
        if ([context isKindOfClass:[NSNumber class]] && [context integerValue] == identifier)
            return item;
    }
    return nil;
}

static UITargetedDragPreview *createTargetedDragPreview(UIImage *image, UIView *rootView, UIView *previewContainer, const FloatRect& frameInRootViewCoordinates, const Vector<FloatRect>& clippingRectsInFrameCoordinates, UIColor *backgroundColor)
{
    if (frameInRootViewCoordinates.isEmpty() || !image)
        return nullptr;

    NSMutableArray *clippingRectValuesInFrameCoordinates = [NSMutableArray arrayWithCapacity:clippingRectsInFrameCoordinates.size()];

    FloatRect frameInContainerCoordinates = [rootView convertRect:frameInRootViewCoordinates toView:previewContainer];
    if (frameInContainerCoordinates.isEmpty())
        return nullptr;

    FloatSize scalingRatio = frameInContainerCoordinates.size() / frameInRootViewCoordinates.size();
    for (auto rect : clippingRectsInFrameCoordinates) {
        rect.scale(scalingRatio);
        [clippingRectValuesInFrameCoordinates addObject:[NSValue valueWithCGRect:rect]];
    }

    auto imageView = adoptNS([[UIImageView alloc] initWithImage:image]);
    [imageView setFrame:frameInContainerCoordinates];

    RetainPtr<UIDragPreviewParameters> parameters;
    if (clippingRectValuesInFrameCoordinates.count)
        parameters = adoptNS([[UIDragPreviewParameters alloc] initWithTextLineRects:clippingRectValuesInFrameCoordinates]);
    else
        parameters = adoptNS([[UIDragPreviewParameters alloc] init]);

    if (backgroundColor)
        [parameters setBackgroundColor:backgroundColor];

    CGPoint centerInContainerCoordinates = { CGRectGetMidX(frameInContainerCoordinates), CGRectGetMidY(frameInContainerCoordinates) };
    auto target = adoptNS([[UIDragPreviewTarget alloc] initWithContainer:previewContainer center:centerInContainerCoordinates]);
    auto dragPreview = adoptNS([[UITargetedDragPreview alloc] initWithView:imageView.get() parameters:parameters.get() target:target.get()]);
    return dragPreview.autorelease();
}

static RetainPtr<UIImage> uiImageForImage(Image* image)
{
    if (!image)
        return nullptr;

    auto cgImage = image->nativeImage();
    if (!cgImage)
        return nullptr;

    return adoptNS([[UIImage alloc] initWithCGImage:cgImage.get()]);
}

static bool shouldUseDragImageToCreatePreviewForDragSource(const DragSourceState& source)
{
    if (!source.image)
        return false;

    return source.action & (DragSourceActionDHTML | DragSourceActionImage);
}

static bool shouldUseTextIndicatorToCreatePreviewForDragSource(const DragSourceState& source)
{
    if (!source.indicatorData)
        return false;

    if (source.action & (DragSourceActionLink | DragSourceActionSelection))
        return true;

#if ENABLE(ATTACHMENT_ELEMENT)
    if (source.action & DragSourceActionAttachment)
        return true;
#endif

    return false;
}

std::optional<DragSourceState> DragDropInteractionState::activeDragSourceForItem(UIDragItem *item) const
{
    if (![item.privateLocalContext isKindOfClass:[NSNumber class]])
        return std::nullopt;

    auto identifier = [(NSNumber *)item.privateLocalContext integerValue];
    for (auto& source : m_activeDragSources) {
        if (source.itemIdentifier == identifier)
            return source;
    }
    return std::nullopt;
}

bool DragDropInteractionState::anyActiveDragSourceIs(WebCore::DragSourceAction action) const
{
    for (auto& source : m_activeDragSources) {
        if (source.action & action)
            return true;
    }
    return false;
}

void DragDropInteractionState::prepareForDragSession(id <UIDragSession> session, dispatch_block_t completionHandler)
{
    m_dragSession = session;
    m_dragStartCompletionBlock = completionHandler;
}

void DragDropInteractionState::dragSessionWillBegin()
{
    m_didBeginDragging = true;
    updatePreviewsForActiveDragSources();
}

UITargetedDragPreview *DragDropInteractionState::previewForDragItem(UIDragItem *item, UIView *contentView, UIView *previewContainer) const
{
    auto foundSource = activeDragSourceForItem(item);
    if (!foundSource)
        return nil;

    auto& source = foundSource.value();
    if (shouldUseDragImageToCreatePreviewForDragSource(source))
        return createTargetedDragPreview(source.image.get(), contentView, previewContainer, source.dragPreviewFrameInRootViewCoordinates, { }, nil);

    if (shouldUseTextIndicatorToCreatePreviewForDragSource(source)) {
        auto indicator = source.indicatorData.value();
        auto textIndicatorImage = uiImageForImage(indicator.contentImage.get());
        return createTargetedDragPreview(textIndicatorImage.get(), contentView, previewContainer, indicator.textBoundingRectInRootViewCoordinates, indicator.textRectsInBoundingRectCoordinates, [UIColor colorWithCGColor:cachedCGColor(indicator.estimatedBackgroundColor)]);
    }

    return nil;
}

void DragDropInteractionState::dragSessionWillDelaySetDownAnimation(dispatch_block_t completion)
{
    m_dragCancelSetDownBlock = completion;
}

bool DragDropInteractionState::shouldRequestAdditionalItemForDragSession(id <UIDragSession> session) const
{
    return m_dragSession == session && !m_addDragItemCompletionBlock && !m_dragStartCompletionBlock;
}

void DragDropInteractionState::dragSessionWillRequestAdditionalItem(void (^completion)(NSArray <UIDragItem *> *))
{
    clearStagedDragSource();
    m_addDragItemCompletionBlock = completion;
}

void DragDropInteractionState::dropSessionDidEnterOrUpdate(id <UIDropSession> session, const DragData& dragData)
{
    m_dropSession = session;
    m_lastGlobalPosition = dragData.globalPosition();
}

void DragDropInteractionState::stageDragItem(const DragItem& item, UIImage *dragImage)
{
    static NSInteger currentDragSourceItemIdentifier = 0;

    m_adjustedPositionForDragEnd = item.eventPositionInContentCoordinates;
    m_stagedDragSource = {{
        static_cast<DragSourceAction>(item.sourceAction),
        item.eventPositionInContentCoordinates,
        item.dragPreviewFrameInRootViewCoordinates,
        dragImage,
        item.image.indicatorData(),
        item.title.isEmpty() ? nil : (NSString *)item.title,
        item.url.isEmpty() ? nil : (NSURL *)item.url,
        true, // We assume here that drag previews need to be updated until proven otherwise in updatePreviewsForActiveDragSources().
        ++currentDragSourceItemIdentifier
    }};
}

bool DragDropInteractionState::hasStagedDragSource() const
{
    return m_stagedDragSource && stagedDragSource().action != WebCore::DragSourceActionNone;
}

void DragDropInteractionState::clearStagedDragSource(DidBecomeActive didBecomeActive)
{
    if (didBecomeActive == DidBecomeActive::Yes)
        m_activeDragSources.append(stagedDragSource());
    m_stagedDragSource = std::nullopt;
}

void DragDropInteractionState::dragAndDropSessionsDidEnd()
{
    // If any of UIKit's completion blocks are still in-flight when the drag interaction ends, we need to ensure that they are still invoked
    // to prevent UIKit from getting into an inconsistent state.
    if (auto completionBlock = takeDragCancelSetDownBlock())
        completionBlock();

    if (auto completionBlock = takeAddDragItemCompletionBlock())
        completionBlock(@[ ]);

    if (auto completionBlock = takeDragStartCompletionBlock())
        completionBlock();
}

void DragDropInteractionState::updatePreviewsForActiveDragSources()
{
    for (auto& source : m_activeDragSources) {
        if (!source.possiblyNeedsDragPreviewUpdate)
            continue;

        if (source.action & DragSourceActionImage || !(source.action & DragSourceActionLink)) {
            // Currently, non-image links are the only type of source for which we need to update
            // drag preview providers after the initial lift. All other dragged content should maintain
            // the same targeted drag preview used during the lift animation.
            continue;
        }

        UIDragItem *dragItem = dragItemMatchingIdentifier(m_dragSession.get(), source.itemIdentifier);
        if (!dragItem)
            continue;

        auto linkDraggingCenter = source.adjustedOrigin;
        RetainPtr<NSString> title = (NSString *)source.linkTitle;
        RetainPtr<NSURL> url = (NSURL *)source.linkURL;
        dragItem.previewProvider = [title, url, linkDraggingCenter] () -> UIDragPreview * {
            UIURLDragPreviewView *previewView = [UIURLDragPreviewView viewWithTitle:title.get() URL:url.get()];
            previewView.center = linkDraggingCenter;
            UIDragPreviewParameters *parameters = [[[UIDragPreviewParameters alloc] initWithTextLineRects:@[ [NSValue valueWithCGRect:previewView.bounds] ]] autorelease];
            return [[[UIDragPreview alloc] initWithView:previewView parameters:parameters] autorelease];
        };

        source.possiblyNeedsDragPreviewUpdate = false;
    }
}

} // namespace WebKit

#endif // ENABLE(DRAG_SUPPORT) && PLATFORM(IOS)
