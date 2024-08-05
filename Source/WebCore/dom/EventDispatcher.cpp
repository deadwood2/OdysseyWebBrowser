/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004-2016 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010, 2011, 2012, 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "EventDispatcher.h"

#include "CompositionEvent.h"
#include "EventContext.h"
#include "EventPath.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HTMLInputElement.h"
#include "InputEvent.h"
#include "KeyboardEvent.h"
#include "MainFrame.h"
#include "MouseEvent.h"
#include "NoEventDispatchAssertion.h"
#include "ScopedEventQueue.h"
#include "ShadowRoot.h"
#include "TextEvent.h"
#include "TouchEvent.h"

namespace WebCore {

void EventDispatcher::dispatchScopedEvent(Node& node, Event& event)
{
    // We need to set the target here because it can go away by the time we actually fire the event.
    event.setTarget(EventPath::eventTargetRespectingTargetRules(node));
    ScopedEventQueue::singleton().enqueueEvent(event);
}

static void callDefaultEventHandlersInTheBubblingOrder(Event& event, const EventPath& path)
{
    if (path.isEmpty())
        return;

    // Non-bubbling events call only one default event handler, the one for the target.
    path.contextAt(0).node()->defaultEventHandler(event);
    ASSERT(!event.defaultPrevented());

    if (event.defaultHandled() || !event.bubbles())
        return;

    size_t size = path.size();
    for (size_t i = 1; i < size; ++i) {
        path.contextAt(i).node()->defaultEventHandler(event);
        ASSERT(!event.defaultPrevented());
        if (event.defaultHandled())
            return;
    }
}

static void dispatchEventInDOM(Event& event, const EventPath& path)
{
    // Trigger capturing event handlers, starting at the top and working our way down.
    event.setEventPhase(Event::CAPTURING_PHASE);

    for (size_t i = path.size() - 1; i > 0; --i) {
        const EventContext& eventContext = path.contextAt(i);
        if (eventContext.currentTargetSameAsTarget())
            continue;
        eventContext.handleLocalEvents(event);
        if (event.propagationStopped())
            return;
    }

    event.setEventPhase(Event::AT_TARGET);
    path.contextAt(0).handleLocalEvents(event);
    if (event.propagationStopped())
        return;

    // Trigger bubbling event handlers, starting at the bottom and working our way up.
    size_t size = path.size();
    for (size_t i = 1; i < size; ++i) {
        const EventContext& eventContext = path.contextAt(i);
        if (eventContext.currentTargetSameAsTarget())
            event.setEventPhase(Event::AT_TARGET);
        else if (event.bubbles())
            event.setEventPhase(Event::BUBBLING_PHASE);
        else
            continue;
        eventContext.handleLocalEvents(event);
        if (event.propagationStopped())
            return;
    }
}

static bool shouldSuppressEventDispatchInDOM(Node& node, Event& event)
{
    if (!event.isTrusted())
        return false;

    auto frame = node.document().frame();
    if (!frame)
        return false;

    if (!frame->mainFrame().loader().shouldSuppressKeyboardInput())
        return false;

    if (is<TextEvent>(event)) {
        auto& textEvent = downcast<TextEvent>(event);
        return textEvent.isKeyboard() || textEvent.isComposition();
    }

    return is<CompositionEvent>(event) || is<InputEvent>(event) || is<KeyboardEvent>(event);
}

bool EventDispatcher::dispatchEvent(Node& node, Event& event)
{
    ASSERT_WITH_SECURITY_IMPLICATION(NoEventDispatchAssertion::isEventAllowedInMainThread());
    Ref<Node> protectedNode(node);
    RefPtr<FrameView> view = node.document().view();
    EventPath eventPath(node, event);

    if (EventTarget* relatedTarget = event.relatedTarget())
        eventPath.setRelatedTarget(node, *relatedTarget);
#if ENABLE(TOUCH_EVENTS)
    if (is<TouchEvent>(event))
        eventPath.retargetTouchLists(downcast<TouchEvent>(event));
#endif

    ChildNodesLazySnapshot::takeChildNodesLazySnapshot();

    EventTarget* target = EventPath::eventTargetRespectingTargetRules(node);
    event.setTarget(target);
    if (!event.target())
        return true;

    ASSERT_WITH_SECURITY_IMPLICATION(NoEventDispatchAssertion::isEventAllowedInMainThread());

    InputElementClickState clickHandlingState;
    if (is<HTMLInputElement>(node))
        downcast<HTMLInputElement>(node).willDispatchEvent(event, clickHandlingState);

    if (shouldSuppressEventDispatchInDOM(node, event))
        event.stopPropagation();

    if (!event.propagationStopped() && !eventPath.isEmpty()) {
        event.setEventPath(eventPath);
        dispatchEventInDOM(event, eventPath);
        event.clearEventPath();
    }

    auto* finalTarget = event.target();
    event.setTarget(EventPath::eventTargetRespectingTargetRules(node));
    event.setCurrentTarget(nullptr);
    event.resetPropagationFlags();
    event.setEventPhase(Event::NONE);

    if (clickHandlingState.stateful)
        downcast<HTMLInputElement>(node).didDispatchClickEvent(event, clickHandlingState);

    // Call default event handlers. While the DOM does have a concept of preventing
    // default handling, the detail of which handlers are called is an internal
    // implementation detail and not part of the DOM.
    if (!event.defaultPrevented() && !event.defaultHandled())
        callDefaultEventHandlersInTheBubblingOrder(event, eventPath);

    event.setTarget(finalTarget);
    event.setCurrentTarget(nullptr);

    return !event.defaultPrevented();
}

}
