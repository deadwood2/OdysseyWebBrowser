/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"
#include "EventTarget.h"

#include "DOMWrapperWorld.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "InspectorInstrumentation.h"
#include "JSEventListener.h"
#include "NoEventDispatchAssertion.h"
#include "ScriptController.h"
#include "WebKitAnimationEvent.h"
#include "WebKitTransitionEvent.h"
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/Ref.h>
#include <wtf/SetForScope.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

using namespace WTF;

namespace WebCore {

Node* EventTarget::toNode()
{
    return nullptr;
}

DOMWindow* EventTarget::toDOMWindow()
{
    return nullptr;
}

bool EventTarget::isMessagePort() const
{
    return false;
}

bool EventTarget::addEventListener(const AtomicString& eventType, Ref<EventListener>&& listener, const AddEventListenerOptions& options)
{
    return ensureEventTargetData().eventListenerMap.add(eventType, WTFMove(listener), { options.capture, options.passive, options.once });
}

void EventTarget::addEventListenerForBindings(const AtomicString& eventType, RefPtr<EventListener>&& listener, AddEventListenerOptionsOrBoolean&& variant)
{
    if (!listener)
        return;

    auto visitor = WTF::makeVisitor([&](const AddEventListenerOptions& options) {
        addEventListener(eventType, listener.releaseNonNull(), options);
    }, [&](bool capture) {
        addEventListener(eventType, listener.releaseNonNull(), capture);
    });

    WTF::visit(visitor, variant);
}

void EventTarget::removeEventListenerForBindings(const AtomicString& eventType, RefPtr<EventListener>&& listener, ListenerOptionsOrBoolean&& variant)
{
    if (!listener)
        return;

    auto visitor = WTF::makeVisitor([&](const ListenerOptions& options) {
        removeEventListener(eventType, *listener, options);
    }, [&](bool capture) {
        removeEventListener(eventType, *listener, capture);
    });

    WTF::visit(visitor, variant);
}

bool EventTarget::removeEventListener(const AtomicString& eventType, EventListener& listener, const ListenerOptions& options)
{
    auto* data = eventTargetData();
    return data && data->eventListenerMap.remove(eventType, listener, options.capture);
}

bool EventTarget::setAttributeEventListener(const AtomicString& eventType, RefPtr<EventListener>&& listener, DOMWrapperWorld& isolatedWorld)
{
    auto* existingListener = attributeEventListener(eventType, isolatedWorld);
    if (!listener) {
        if (existingListener)
            removeEventListener(eventType, *existingListener, false);
        return false;
    }
    if (existingListener) {
        eventTargetData()->eventListenerMap.replace(eventType, *existingListener, listener.releaseNonNull(), { });
        return true;
    }
    return addEventListener(eventType, listener.releaseNonNull());
}

EventListener* EventTarget::attributeEventListener(const AtomicString& eventType, DOMWrapperWorld& isolatedWorld)
{
    for (auto& eventListener : eventListeners(eventType)) {
        auto& listener = eventListener->callback();
        if (!listener.isAttribute())
            continue;

        auto& listenerWorld = downcast<JSEventListener>(listener).isolatedWorld();
        if (&listenerWorld == &isolatedWorld)
            return &listener;
    }

    return nullptr;
}

bool EventTarget::hasActiveEventListeners(const AtomicString& eventType) const
{
    auto* data = eventTargetData();
    return data && data->eventListenerMap.containsActive(eventType);
}

ExceptionOr<bool> EventTarget::dispatchEventForBindings(Event& event)
{
    event.setUntrusted();

    if (!event.isInitialized() || event.isBeingDispatched())
        return Exception { INVALID_STATE_ERR };

    if (!scriptExecutionContext())
        return false;

    return dispatchEvent(event);
}

bool EventTarget::dispatchEvent(Event& event)
{
    ASSERT(event.isInitialized());
    ASSERT(!event.isBeingDispatched());

    event.setTarget(this);
    event.setCurrentTarget(this);
    event.setEventPhase(Event::AT_TARGET);
    bool defaultPrevented = fireEventListeners(event);
    event.resetPropagationFlags();
    event.setEventPhase(Event::NONE);
    return defaultPrevented;
}

void EventTarget::uncaughtExceptionInEventHandler()
{
}

static const AtomicString& legacyType(const Event& event)
{
    if (event.type() == eventNames().animationendEvent)
        return eventNames().webkitAnimationEndEvent;

    if (event.type() == eventNames().animationstartEvent)
        return eventNames().webkitAnimationStartEvent;

    if (event.type() == eventNames().animationiterationEvent)
        return eventNames().webkitAnimationIterationEvent;

    if (event.type() == eventNames().transitionendEvent)
        return eventNames().webkitTransitionEndEvent;

    // FIXME: This legacy name is not part of the specification (https://dom.spec.whatwg.org/#dispatching-events).
    if (event.type() == eventNames().wheelEvent)
        return eventNames().mousewheelEvent;

    return nullAtom;
}

bool EventTarget::fireEventListeners(Event& event)
{
    ASSERT_WITH_SECURITY_IMPLICATION(NoEventDispatchAssertion::isEventAllowedInMainThread());
    ASSERT(event.isInitialized());

    auto* data = eventTargetData();
    if (!data)
        return true;

    SetForScope<bool> firingEventListenersScope(data->isFiringEventListeners, true);

    if (auto* listenersVector = data->eventListenerMap.find(event.type())) {
        fireEventListeners(event, *listenersVector);
        return !event.defaultPrevented();
    }

    // Only fall back to legacy types for trusted events.
    if (!event.isTrusted())
        return !event.defaultPrevented();

    const AtomicString& legacyTypeName = legacyType(event);
    if (!legacyTypeName.isNull()) {
        if (auto* legacyListenersVector = data->eventListenerMap.find(legacyTypeName)) {
            AtomicString typeName = event.type();
            event.setType(legacyTypeName);
            fireEventListeners(event, *legacyListenersVector);
            event.setType(typeName);
        }
    }
    return !event.defaultPrevented();
}

// Intentionally creates a copy of the listeners vector to avoid event listeners added after this point from being run.
// Note that removal still has an effect due to the removed field in RegisteredEventListener.
void EventTarget::fireEventListeners(Event& event, EventListenerVector listeners)
{
    Ref<EventTarget> protectedThis(*this);
    ASSERT(!listeners.isEmpty());

    auto* context = scriptExecutionContext();
    bool contextIsDocument = is<Document>(context);
    InspectorInstrumentationCookie willDispatchEventCookie;
    if (contextIsDocument)
        willDispatchEventCookie = InspectorInstrumentation::willDispatchEvent(downcast<Document>(*context), event, true);

    for (auto& registeredListener : listeners) {
        if (UNLIKELY(registeredListener->wasRemoved()))
            continue;

        if (event.eventPhase() == Event::CAPTURING_PHASE && !registeredListener->useCapture())
            continue;
        if (event.eventPhase() == Event::BUBBLING_PHASE && registeredListener->useCapture())
            continue;

        // If stopImmediatePropagation has been called, we just break out immediately, without
        // handling any more events on this target.
        if (event.immediatePropagationStopped())
            break;

        // Do this before invocation to avoid reentrancy issues.
        if (registeredListener->isOnce())
            removeEventListener(event.type(), registeredListener->callback(), ListenerOptions(registeredListener->useCapture()));

        if (registeredListener->isPassive())
            event.setInPassiveListener(true);

        InspectorInstrumentation::willHandleEvent(context, event);
        registeredListener->callback().handleEvent(context, &event);

        if (registeredListener->isPassive())
            event.setInPassiveListener(false);
    }

    if (contextIsDocument)
        InspectorInstrumentation::didDispatchEvent(willDispatchEventCookie);
}

const EventListenerVector& EventTarget::eventListeners(const AtomicString& eventType)
{
    auto* data = eventTargetData();
    auto* listenerVector = data ? data->eventListenerMap.find(eventType) : nullptr;
    static NeverDestroyed<EventListenerVector> emptyVector;
    return listenerVector ? *listenerVector : emptyVector.get();
}

void EventTarget::removeAllEventListeners()
{
    auto* data = eventTargetData();
    if (!data)
        return;
    data->eventListenerMap.clear();
}

void EventTarget::visitJSEventListeners(JSC::SlotVisitor& visitor)
{
    EventTargetData* data = eventTargetDataConcurrently();
    if (!data)
        return;
    
    auto locker = holdLock(data->eventListenerMap.lock());
    EventListenerIterator iterator(&data->eventListenerMap);
    while (auto* listener = iterator.nextListener())
        listener->visitJSFunction(visitor);
}

} // namespace WebCore
