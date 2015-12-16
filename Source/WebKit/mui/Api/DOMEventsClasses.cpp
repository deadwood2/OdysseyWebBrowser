/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMEventsClasses.h"

#include <DOMWindow.h>
#include <Event.h>
#include <KeyboardEvent.h>
#include <MouseEvent.h>


void DOMEventListener::handleEvent(DOMEvent* /*evt*/)
{
}

// DOMEvent -------------------------------------------------------------------

DOMEvent::DOMEvent(PassRefPtr<WebCore::Event> e)
: m_event(0)
{
    m_event = e;
}

DOMEvent::~DOMEvent()
{
}

DOMEvent* DOMEvent::createInstance(PassRefPtr<WebCore::Event> e)
{
    if (!e)
        return 0;

    if (e->isKeyboardEvent()) 
        return new DOMKeyboardEvent(e);
    else if (e->isMouseEvent()) 
        return new DOMMouseEvent(e);
    /*
    else if (e->isMutationEvent())
        return new DOMMutationEvent(e);
    else if (e->isOverflowEvent())
        return new DOMOverflowEvent(e);
    else if (e->isWheelEvent())
        return new DOMWheelEvent(e);
    else if (e->isUIEvent())
        return new DOMUIEvent(e);
    */
    else
        return new DOMEvent(e);

    return 0;
}

const char* DOMEvent::type()
{
    return "";
}

DOMEventTarget* DOMEvent::target()
{
    return 0;
}

DOMEventTarget* DOMEvent::currentTarget()
{
    return 0;
}

unsigned short DOMEvent::eventPhase()
{
    return 0;
}

bool DOMEvent::bubbles()
{
    return false;
}

bool DOMEvent::cancelable()
{
    return false;
}

DOMTimeStamp DOMEvent::timeStamp()
{
    return 0;
}

void DOMEvent::stopPropagation()
{
}

void DOMEvent::preventDefault()
{
}

void DOMEvent::initEvent(const char* /*eventTypeArg*/, bool /*canBubbleArg*/, bool /*cancelableArg*/)
{
}

// DOMUIEvent -----------------------------------------------------------------

WebCore::DOMWindow* DOMUIEvent::view()
{
    return 0;
}

long DOMUIEvent::detail()
{
    return 0;
}

void DOMUIEvent::initUIEvent(const char* /*type*/, bool /*canBubble*/, bool /*cancelable*/, WebCore::DOMWindow* /*view*/, long /*detail*/)
{
}

long DOMUIEvent::keyCode()
{
    return 0;
}

long DOMUIEvent::charCode()
{
    return 0;
}

long DOMUIEvent::layerX()
{
    return 0;
}

long DOMUIEvent::layerY()
{
    return 0;
}

long DOMUIEvent::pageX()
{
    return 0;
}

long DOMUIEvent::pageY()
{
    return 0;
}

long DOMUIEvent::which()
{
    return 0;
}

// DOMKeyboardEvent -----------------------------------------------------------

const char* DOMKeyboardEvent::keyIdentifier()
{
    return "";
}

unsigned long DOMKeyboardEvent::keyLocation()
{
    return 0;
}

bool DOMKeyboardEvent::ctrlKey()
{
    if (!m_event || !m_event->isKeyboardEvent())
        return false;
    WebCore::KeyboardEvent* keyEvent = static_cast<WebCore::KeyboardEvent*>(m_event.get());

    return keyEvent->ctrlKey() ? true : false;
}

bool DOMKeyboardEvent::shiftKey()
{
    if (!m_event || !m_event->isKeyboardEvent())
        return false;
    WebCore::KeyboardEvent* keyEvent = static_cast<WebCore::KeyboardEvent*>(m_event.get());

    return keyEvent->shiftKey() ? true : false;
}

bool DOMKeyboardEvent::altKey()
{
    if (!m_event || !m_event->isKeyboardEvent())
        return false;
    WebCore::KeyboardEvent* keyEvent = static_cast<WebCore::KeyboardEvent*>(m_event.get());

    return keyEvent->altKey() ? true : false;
}

bool DOMKeyboardEvent::metaKey()
{
    if (!m_event || !m_event->isKeyboardEvent())
        return false;
    WebCore::KeyboardEvent* keyEvent = static_cast<WebCore::KeyboardEvent*>(m_event.get());

    return keyEvent->metaKey() ? true : false;
}

bool DOMKeyboardEvent::altGraphKey()
{
    if (!m_event || !m_event->isKeyboardEvent())
        return false;
    WebCore::KeyboardEvent* keyEvent = static_cast<WebCore::KeyboardEvent*>(m_event.get());

    return keyEvent->altGraphKey() ? true : false;
}

bool DOMKeyboardEvent::getModifierState(const char* /*keyIdentifierArg*/)
{
    return false;
}

void DOMKeyboardEvent::initKeyboardEvent(const char* /*type*/, bool /*canBubble*/, bool /*cancelable*/, WebCore::DOMWindow* /*view*/, const char* /*keyIdentifier*/, unsigned long /*keyLocation*/, bool /*ctrlKey*/, bool /*altKey*/, bool /*shiftKey*/, bool /*metaKey*/, bool /*graphKey*/)
{
}

// DOMMouseEvent --------------------------------------------------------------

long DOMMouseEvent::screenX()
{
    return 0;
}

long DOMMouseEvent::screenY()
{
    return 0;
}

long DOMMouseEvent::clientX()
{
    return 0;
}

long DOMMouseEvent::clientY()
{
    return 0;
}

bool DOMMouseEvent::ctrlKey()
{
    if (!m_event || !m_event->isMouseEvent())
        return false;
    WebCore::MouseEvent* mouseEvent = static_cast<WebCore::MouseEvent*>(m_event.get());

    return mouseEvent->ctrlKey() ? true : false;
}

bool DOMMouseEvent::shiftKey()
{
    if (!m_event || !m_event->isMouseEvent())
        return false;
    WebCore::MouseEvent* mouseEvent = static_cast<WebCore::MouseEvent*>(m_event.get());

    return mouseEvent->shiftKey() ? true : false;
}

bool DOMMouseEvent::altKey()
{
    if (!m_event || !m_event->isMouseEvent())
        return false;
    WebCore::MouseEvent* mouseEvent = static_cast<WebCore::MouseEvent*>(m_event.get());

    return mouseEvent->altKey() ? true : false;
}

bool DOMMouseEvent::metaKey()
{
    if (!m_event || !m_event->isMouseEvent())
        return false;
    WebCore::MouseEvent* mouseEvent = static_cast<WebCore::MouseEvent*>(m_event.get());

    return mouseEvent->metaKey() ? true : false;
}

unsigned short DOMMouseEvent::button()
{
    return 0;
}

DOMEventTarget* DOMMouseEvent::relatedTarget()
{
    return 0;
}

void DOMMouseEvent::initMouseEvent(const char* /*type*/, bool /*canBubble*/, bool /*cancelable*/, WebCore::DOMWindow* /*view*/, long /*detail*/, long /*screenX*/, long /*screenY*/, long /*clientX*/, long /*clientY*/, bool /*ctrlKey*/, bool /*altKey*/, bool /*shiftKey*/, bool /*metaKey*/, unsigned short /*button*/, DOMEventTarget* /*relatedTarget*/)
{
}

long DOMMouseEvent::offsetX()
{
    return 0;
}

long DOMMouseEvent::offsetY()
{
    return 0;
}

long DOMMouseEvent::x()
{
    return 0;
}

long DOMMouseEvent::y()
{
    return 0;
}

DOMNode* DOMMouseEvent::fromElement()
{
    return 0;
}

DOMNode* DOMMouseEvent::toElement()
{
    return 0;
}

// DOMMutationEvent -----------------------------------------------------------

DOMNode* DOMMutationEvent::relatedNode()
{
    return 0;
}

const char* DOMMutationEvent::prevValue()
{
    return "";
}

const char* DOMMutationEvent::newValue()
{
    return "";
}

const char* DOMMutationEvent::attrName()
{
    return "";
}

unsigned short DOMMutationEvent::attrChange()
{
    return 0;
}

void DOMMutationEvent::initMutationEvent(const char* /*type*/, bool /*canBubble*/, bool /*cancelable*/, DOMNode* /*relatedNode*/, const char* /*prevValue*/, const char* /*newValue*/, const char* /*attrName*/, unsigned short /*attrChange*/)
{
}

// DOMOverflowEvent -----------------------------------------------------------

unsigned short DOMOverflowEvent::orient()
{
    return 0;
}

bool DOMOverflowEvent::horizontalOverflow()
{
    return false;
}

bool DOMOverflowEvent::verticalOverflow()
{
    return false;
}

// DOMWheelEvent --------------------------------------------------------------

long DOMWheelEvent::screenX()
{
    return 0;
}

long DOMWheelEvent::screenY()
{
    return 0;
}

long DOMWheelEvent::clientX()
{
    return 0;
}

long DOMWheelEvent::clientY()
{
    return 0;
}

bool DOMWheelEvent::ctrlKey()
{
    return false;
}

bool DOMWheelEvent::shiftKey()
{
    return false;
}

bool DOMWheelEvent::altKey()
{
    return false;
}

bool DOMWheelEvent::metaKey()
{
    return false;
}

long DOMWheelEvent::wheelDelta()
{
    return 0;
}

long DOMWheelEvent::wheelDeltaX()
{
    return 0;
}

long DOMWheelEvent::wheelDeltaY()
{
    return 0;
}

long DOMWheelEvent::offsetX()
{
    return 0;
}

long DOMWheelEvent::offsetY()
{
    return 0;
}

long DOMWheelEvent::x()
{
    return 0;
}

long DOMWheelEvent::y()
{
    return 0;
}

bool DOMWheelEvent::isHorizontal()
{
    return false;
}

void DOMWheelEvent::initWheelEvent(long /*wheelDeltaX*/, long /*wheelDeltaY*/, WebCore::DOMWindow* /*view*/, long /*screenX*/, long /*screenY*/, long /*clientX*/, long /*clientY*/, bool /*ctrlKey*/, bool /*altKey*/, bool /*shiftKey*/, bool /*metaKey*/)
{
}
