/*
 * Copyright (C) 2012-2015 Apple Inc. All rights reserved.
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
#include "ScrollingTree.h"

#if ENABLE(ASYNC_SCROLLING)

#include "EventNames.h"
#include "Logging.h"
#include "PlatformWheelEvent.h"
#include "ScrollingStateTree.h"
#include "ScrollingTreeFrameScrollingNode.h"
#include "ScrollingTreeNode.h"
#include "ScrollingTreeOverflowScrollingNode.h"
#include "ScrollingTreeScrollingNode.h"
#include "TextStream.h"
#include <wtf/TemporaryChange.h>

namespace WebCore {

ScrollingTree::ScrollingTree()
{
}

ScrollingTree::~ScrollingTree()
{
}

bool ScrollingTree::shouldHandleWheelEventSynchronously(const PlatformWheelEvent& wheelEvent)
{
    // This method is invoked by the event handling thread
    LockHolder lock(m_mutex);

    bool shouldSetLatch = wheelEvent.shouldConsiderLatching();
    
    if (hasLatchedNode() && !shouldSetLatch)
        return false;

    if (shouldSetLatch)
        m_latchedNode = 0;
    
    if (!m_eventTrackingRegions.isEmpty() && m_rootNode) {
        ScrollingTreeFrameScrollingNode& frameScrollingNode = downcast<ScrollingTreeFrameScrollingNode>(*m_rootNode);
        FloatPoint position = wheelEvent.position();
        position.move(frameScrollingNode.viewToContentsOffset(m_mainFrameScrollPosition));

        const EventNames& names = eventNames();
        IntPoint roundedPosition = roundedIntPoint(position);
        bool isSynchronousDispatchRegion = m_eventTrackingRegions.trackingTypeForPoint(names.wheelEvent, roundedPosition) == TrackingType::Synchronous
            || m_eventTrackingRegions.trackingTypeForPoint(names.mousewheelEvent, roundedPosition) == TrackingType::Synchronous;
        LOG_WITH_STREAM(Scrolling, stream << "ScrollingTree::shouldHandleWheelEventSynchronously: wheelEvent at " << wheelEvent.position() << " mapped to content point " << position << ", in non-fast region " << isSynchronousDispatchRegion);

        if (isSynchronousDispatchRegion)
            return true;
    }
    return false;
}

void ScrollingTree::setOrClearLatchedNode(const PlatformWheelEvent& wheelEvent, ScrollingNodeID nodeID)
{
    if (wheelEvent.shouldConsiderLatching())
        setLatchedNode(nodeID);
    else if (wheelEvent.shouldResetLatching())
        clearLatchedNode();
}

void ScrollingTree::handleWheelEvent(const PlatformWheelEvent& wheelEvent)
{
    if (m_rootNode)
        downcast<ScrollingTreeScrollingNode>(*m_rootNode).handleWheelEvent(wheelEvent);
}

void ScrollingTree::viewportChangedViaDelegatedScrolling(ScrollingNodeID nodeID, const WebCore::FloatRect& fixedPositionRect, double scale)
{
    ScrollingTreeNode* node = nodeForID(nodeID);
    if (!is<ScrollingTreeScrollingNode>(node))
        return;

    downcast<ScrollingTreeScrollingNode>(*node).updateLayersAfterViewportChange(fixedPositionRect, scale);
}

void ScrollingTree::scrollPositionChangedViaDelegatedScrolling(ScrollingNodeID nodeID, const WebCore::FloatPoint& scrollPosition, bool inUserInteration)
{
    ScrollingTreeNode* node = nodeForID(nodeID);
    if (!is<ScrollingTreeOverflowScrollingNode>(node))
        return;

    // Update descendant nodes
    downcast<ScrollingTreeOverflowScrollingNode>(*node).updateLayersAfterDelegatedScroll(scrollPosition);

    // Update GraphicsLayers and scroll state.
    scrollingTreeNodeDidScroll(nodeID, scrollPosition, inUserInteration ? SyncScrollingLayerPosition : SetScrollingLayerPosition);
}

void ScrollingTree::commitNewTreeState(std::unique_ptr<ScrollingStateTree> scrollingStateTree)
{
    bool rootStateNodeChanged = scrollingStateTree->hasNewRootStateNode();
    
    ScrollingStateScrollingNode* rootNode = scrollingStateTree->rootStateNode();
    if (rootNode
        && (rootStateNodeChanged
            || rootNode->hasChangedProperty(ScrollingStateFrameScrollingNode::EventTrackingRegion)
            || rootNode->hasChangedProperty(ScrollingStateNode::ScrollLayer))) {
        LockHolder lock(m_mutex);

        if (rootStateNodeChanged || rootNode->hasChangedProperty(ScrollingStateNode::ScrollLayer))
            m_mainFrameScrollPosition = FloatPoint();
        if (rootStateNodeChanged || rootNode->hasChangedProperty(ScrollingStateFrameScrollingNode::EventTrackingRegion))
            m_eventTrackingRegions = scrollingStateTree->rootStateNode()->eventTrackingRegions();
    }
    
    bool scrollRequestIsProgammatic = rootNode ? rootNode->requestedScrollPositionRepresentsProgrammaticScroll() : false;
    TemporaryChange<bool> changeHandlingProgrammaticScroll(m_isHandlingProgrammaticScroll, scrollRequestIsProgammatic);

    removeDestroyedNodes(*scrollingStateTree);

    OrphanScrollingNodeMap orphanNodes;
    updateTreeFromStateNode(rootNode, orphanNodes);
}

void ScrollingTree::updateTreeFromStateNode(const ScrollingStateNode* stateNode, OrphanScrollingNodeMap& orphanNodes)
{
    if (!stateNode) {
        m_nodeMap.clear();
        m_rootNode = nullptr;
        return;
    }
    
    ScrollingNodeID nodeID = stateNode->scrollingNodeID();
    ScrollingNodeID parentNodeID = stateNode->parentNodeID();

    auto it = m_nodeMap.find(nodeID);

    RefPtr<ScrollingTreeNode> node;
    if (it != m_nodeMap.end())
        node = it->value;
    else {
        node = createScrollingTreeNode(stateNode->nodeType(), nodeID);
        if (!parentNodeID) {
            // This is the root node. Clear the node map.
            ASSERT(stateNode->nodeType() == FrameScrollingNode);
            m_rootNode = node;
            m_nodeMap.clear();
        } 
        m_nodeMap.set(nodeID, node.get());
    }

    if (parentNodeID) {
        auto parentIt = m_nodeMap.find(parentNodeID);
        ASSERT_WITH_SECURITY_IMPLICATION(parentIt != m_nodeMap.end());
        if (parentIt != m_nodeMap.end()) {
            ScrollingTreeNode* parent = parentIt->value;
            node->setParent(parent);
            parent->appendChild(node);
        }
    }

    node->updateBeforeChildren(*stateNode);
    
    // Move all children into the orphanNodes map. Live ones will get added back as we recurse over children.
    if (auto nodeChildren = node->children()) {
        for (auto& childScrollingNode : *nodeChildren) {
            childScrollingNode->setParent(nullptr);
            orphanNodes.add(childScrollingNode->scrollingNodeID(), childScrollingNode);
        }
        nodeChildren->clear();
    }

    // Now update the children if we have any.
    if (auto children = stateNode->children()) {
        for (auto& child : *children)
            updateTreeFromStateNode(child.get(), orphanNodes);
    }

    node->updateAfterChildren(*stateNode);
}

void ScrollingTree::removeDestroyedNodes(const ScrollingStateTree& stateTree)
{
    for (const auto& removedNodeID : stateTree.removedNodes()) {
        m_nodeMap.remove(removedNodeID);
        if (removedNodeID == m_latchedNode)
            clearLatchedNode();
    }
}

ScrollingTreeNode* ScrollingTree::nodeForID(ScrollingNodeID nodeID) const
{
    if (!nodeID)
        return nullptr;

    return m_nodeMap.get(nodeID);
}

void ScrollingTree::setMainFramePinState(bool pinnedToTheLeft, bool pinnedToTheRight, bool pinnedToTheTop, bool pinnedToTheBottom)
{
    LockHolder locker(m_swipeStateMutex);

    m_mainFramePinnedToTheLeft = pinnedToTheLeft;
    m_mainFramePinnedToTheRight = pinnedToTheRight;
    m_mainFramePinnedToTheTop = pinnedToTheTop;
    m_mainFramePinnedToTheBottom = pinnedToTheBottom;
}

FloatPoint ScrollingTree::mainFrameScrollPosition()
{
    LockHolder lock(m_mutex);
    return m_mainFrameScrollPosition;
}

void ScrollingTree::setMainFrameScrollPosition(FloatPoint position)
{
    LockHolder lock(m_mutex);
    m_mainFrameScrollPosition = position;
}

TrackingType ScrollingTree::eventTrackingTypeForPoint(const AtomicString& eventName, IntPoint p)
{
    LockHolder lock(m_mutex);
    
    return m_eventTrackingRegions.trackingTypeForPoint(eventName, p);
}

bool ScrollingTree::isRubberBandInProgress()
{
    LockHolder lock(m_mutex);    

    return m_mainFrameIsRubberBanding;
}

void ScrollingTree::setMainFrameIsRubberBanding(bool isRubberBanding)
{
    LockHolder locker(m_mutex);

    m_mainFrameIsRubberBanding = isRubberBanding;
}

bool ScrollingTree::isScrollSnapInProgress()
{
    LockHolder lock(m_mutex);
    
    return m_mainFrameIsScrollSnapping;
}
    
void ScrollingTree::setMainFrameIsScrollSnapping(bool isScrollSnapping)
{
    LockHolder locker(m_mutex);
    
    m_mainFrameIsScrollSnapping = isScrollSnapping;
}

void ScrollingTree::setCanRubberBandState(bool canRubberBandAtLeft, bool canRubberBandAtRight, bool canRubberBandAtTop, bool canRubberBandAtBottom)
{
    LockHolder locker(m_swipeStateMutex);

    m_rubberBandsAtLeft = canRubberBandAtLeft;
    m_rubberBandsAtRight = canRubberBandAtRight;
    m_rubberBandsAtTop = canRubberBandAtTop;
    m_rubberBandsAtBottom = canRubberBandAtBottom;
}

bool ScrollingTree::rubberBandsAtLeft()
{
    LockHolder lock(m_swipeStateMutex);

    return m_rubberBandsAtLeft;
}

bool ScrollingTree::rubberBandsAtRight()
{
    LockHolder lock(m_swipeStateMutex);

    return m_rubberBandsAtRight;
}

bool ScrollingTree::rubberBandsAtBottom()
{
    LockHolder lock(m_swipeStateMutex);

    return m_rubberBandsAtBottom;
}

bool ScrollingTree::rubberBandsAtTop()
{
    LockHolder lock(m_swipeStateMutex);

    return m_rubberBandsAtTop;
}

bool ScrollingTree::isHandlingProgrammaticScroll()
{
    return m_isHandlingProgrammaticScroll;
}

void ScrollingTree::setScrollPinningBehavior(ScrollPinningBehavior pinning)
{
    LockHolder locker(m_swipeStateMutex);
    
    m_scrollPinningBehavior = pinning;
}

ScrollPinningBehavior ScrollingTree::scrollPinningBehavior()
{
    LockHolder lock(m_swipeStateMutex);
    
    return m_scrollPinningBehavior;
}

bool ScrollingTree::willWheelEventStartSwipeGesture(const PlatformWheelEvent& wheelEvent)
{
    if (wheelEvent.phase() != PlatformWheelEventPhaseBegan)
        return false;

    LockHolder lock(m_swipeStateMutex);

    if (wheelEvent.deltaX() > 0 && m_mainFramePinnedToTheLeft && !m_rubberBandsAtLeft)
        return true;
    if (wheelEvent.deltaX() < 0 && m_mainFramePinnedToTheRight && !m_rubberBandsAtRight)
        return true;
    if (wheelEvent.deltaY() > 0 && m_mainFramePinnedToTheTop && !m_rubberBandsAtTop)
        return true;
    if (wheelEvent.deltaY() < 0 && m_mainFramePinnedToTheBottom && !m_rubberBandsAtBottom)
        return true;

    return false;
}

void ScrollingTree::setScrollingPerformanceLoggingEnabled(bool flag)
{
    m_scrollingPerformanceLoggingEnabled = flag;
}

bool ScrollingTree::scrollingPerformanceLoggingEnabled()
{
    return m_scrollingPerformanceLoggingEnabled;
}

ScrollingNodeID ScrollingTree::latchedNode()
{
    LockHolder locker(m_mutex);
    return m_latchedNode;
}

void ScrollingTree::setLatchedNode(ScrollingNodeID node)
{
    LockHolder locker(m_mutex);
    m_latchedNode = node;
}

void ScrollingTree::clearLatchedNode()
{
    LockHolder locker(m_mutex);
    m_latchedNode = 0;
}

} // namespace WebCore

#endif // ENABLE(ASYNC_SCROLLING)
