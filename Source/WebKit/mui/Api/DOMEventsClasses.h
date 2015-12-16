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

#ifndef DOMEventsClasses_H
#define DOMEventsClasses_H

#include "DOMCoreClasses.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class Event;
    class DOMWindow;
}

class DOMEventTarget {
};

class DOMUIEvent;
typedef unsigned long long DOMTimeStamp;

class DOMEventListener : public DOMObject
{
public:
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMEventListener
    virtual void handleEvent(DOMEvent* evt);
};

class DOMEvent : public DOMObject
{
public:
    static DOMEvent* createInstance(PassRefPtr<WebCore::Event> e);
protected:
    DOMEvent(PassRefPtr<WebCore::Event> e);
    ~DOMEvent();

public:
    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMEvent
    virtual const char* type();
    
    virtual DOMEventTarget* target();
    
    virtual DOMEventTarget* currentTarget();
    
    virtual unsigned short eventPhase();
    
    virtual bool bubbles();
    
    virtual bool cancelable();
    
    virtual DOMTimeStamp timeStamp();
    
    virtual void stopPropagation();
    
    virtual void preventDefault();
    
    virtual void initEvent(const char* eventTypeArg, bool canBubbleArg, bool cancelableArg);

    // DOMEvent methods
    WebCore::Event* coreEvent() { return m_event.get(); }

protected:
    RefPtr<WebCore::Event> m_event;
};

class DOMUIEvent : public DOMEvent
{
public:
    DOMUIEvent(PassRefPtr<WebCore::Event> e) : DOMEvent(e) {}

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMEvent
    virtual const char* type() { return DOMEvent::type(); }
    
    virtual DOMEventTarget* target() { return DOMEvent::target(); }
    
    virtual DOMEventTarget* currentTarget() { return DOMEvent::currentTarget(); }
    
    virtual unsigned short eventPhase() { return DOMEvent::eventPhase(); }
    
    virtual bool bubbles() { return DOMEvent::bubbles(); }
    
    virtual bool cancelable() { return DOMEvent::cancelable(); }
    
    virtual DOMTimeStamp timeStamp() { return DOMEvent::timeStamp(); }
    
    virtual void stopPropagation() { DOMEvent::stopPropagation(); }
    
    virtual void preventDefault() { DOMEvent::preventDefault(); }
    
    virtual void initEvent(const char* eventTypeArg, bool canBubbleArg, bool cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }
    
    // IDOMUIEvent
    virtual WebCore::DOMWindow* view();
    
    virtual long detail();
    
    virtual void initUIEvent(const char* type, bool canBubble, bool cancelable, WebCore::DOMWindow* view, long detail);
    
    virtual long keyCode();
    
    virtual long charCode();
    
    virtual long layerX();
    
    virtual long layerY();
    
    virtual long pageX();
    
    virtual long pageY();
    
    virtual long which();
};

class DOMKeyboardEvent : public DOMUIEvent
{
public:
    DOMKeyboardEvent(PassRefPtr<WebCore::Event> e) : DOMUIEvent(e) { }

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMEvent
    virtual const char* type() { return DOMEvent::type(); }
    
    virtual DOMEventTarget* target() { return DOMEvent::target(); }
    
    virtual DOMEventTarget* currentTarget() { return DOMEvent::currentTarget(); }
    
    virtual unsigned short eventPhase() { return DOMEvent::eventPhase(); }
    
    virtual bool bubbles() { return DOMEvent::bubbles(); }
    
    virtual bool cancelable() { return DOMEvent::cancelable(); }
    
    virtual DOMTimeStamp timeStamp() { return DOMEvent::timeStamp(); }
    
    virtual void stopPropagation() { DOMEvent::stopPropagation(); }
    
    virtual void preventDefault() { DOMEvent::preventDefault(); }
    
    virtual void initEvent(const char* eventTypeArg, bool canBubbleArg, bool cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // IDOMUIEvent
    virtual WebCore::DOMWindow* view() { return DOMUIEvent::view(); }
    
    virtual long detail() { return DOMUIEvent::detail(); }
    
    virtual void initUIEvent(const char* type, bool canBubble, bool cancelable, WebCore::DOMWindow* view, long detail) {  DOMUIEvent::initUIEvent(type, canBubble, cancelable, view, detail); }
    
    virtual long keyCode() { return DOMUIEvent::keyCode(); }
    
    virtual long charCode() { return DOMUIEvent::charCode(); }
    
    virtual long layerX() { return DOMUIEvent::layerX(); }
    
    virtual long layerY() { return DOMUIEvent::layerY(); }
    
    virtual long pageX() { return DOMUIEvent::pageX(); }
    
    virtual long pageY() { return DOMUIEvent::pageY(); }
    
    virtual long which() { return DOMUIEvent::which(); }
    
    // DOMKeyboardEvent
    virtual const char* keyIdentifier();
    
    virtual unsigned long keyLocation();
    
    virtual bool ctrlKey();
    
    virtual bool shiftKey();
    
    virtual bool altKey();
    
    virtual bool metaKey();
    
    virtual bool altGraphKey();
    
    virtual bool getModifierState(const char* keyIdentifierArg);
    
    virtual void initKeyboardEvent(const char* type, bool canBubble, bool cancelable, WebCore::DOMWindow* view, const char* keyIdentifier, unsigned long keyLocation, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool graphKey);
};

class DOMMouseEvent : public DOMUIEvent
{
public:
    DOMMouseEvent(PassRefPtr<WebCore::Event> e) : DOMUIEvent(e) { }

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // DOMEvent
    virtual const char* type() { return DOMEvent::type(); }
    
    virtual DOMEventTarget* target() { return DOMEvent::target(); }
    
    virtual DOMEventTarget* currentTarget() { return DOMEvent::currentTarget(); }
    
    virtual unsigned short eventPhase() { return DOMEvent::eventPhase(); }
    
    virtual bool bubbles() { return DOMEvent::bubbles(); }
    
    virtual bool cancelable() { return DOMEvent::cancelable(); }
    
    virtual DOMTimeStamp timeStamp() { return DOMEvent::timeStamp(); }
    
    virtual void stopPropagation() { DOMEvent::stopPropagation(); }
    
    virtual void preventDefault() { DOMEvent::preventDefault(); }
    
    virtual void initEvent(const char* eventTypeArg, bool canBubbleArg, bool cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // DOMUIEvent
    virtual WebCore::DOMWindow* view() { return DOMUIEvent::view(); }
    
    virtual long detail() { return DOMUIEvent::detail(); }
    
    virtual void initUIEvent(const char* type, bool canBubble, bool cancelable, WebCore::DOMWindow* view, long detail) {  DOMUIEvent::initUIEvent(type, canBubble, cancelable, view, detail); }
    
    virtual long keyCode() { return DOMUIEvent::keyCode(); }
    
    virtual long charCode() { return DOMUIEvent::charCode(); }
    
    virtual long layerX() { return DOMUIEvent::layerX(); }
    
    virtual long layerY() { return DOMUIEvent::layerY(); }
    
    virtual long pageX() { return DOMUIEvent::pageX(); }
    
    virtual long pageY() { return DOMUIEvent::pageY(); }
    
    virtual long which() { return DOMUIEvent::which(); }

    // DOMMouseEvent
    virtual long screenX();
    
    virtual long screenY();
    
    virtual long clientX();
    
    virtual long clientY();
    
    virtual bool ctrlKey();
    
    virtual bool shiftKey();
    
    virtual bool altKey();
    
    virtual bool metaKey();
    
    virtual unsigned short button();
    
    virtual DOMEventTarget* relatedTarget();
    
    virtual void initMouseEvent(const char* type, bool canBubble, bool cancelable, WebCore::DOMWindow* view, long detail, long screenX, long screenY, long clientX, long clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, unsigned short button, DOMEventTarget* relatedTarget);
    
    virtual long offsetX();
    
    virtual long offsetY();
    
    virtual long x();
    
    virtual long y();
    
    virtual DOMNode* fromElement();
    
    virtual DOMNode* toElement();
};

class DOMMutationEvent : public DOMEvent
{
public:
    DOMMutationEvent(PassRefPtr<WebCore::Event> e) : DOMEvent(e) { }

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // DOMEvent
    virtual const char* type() { return DOMEvent::type(); }
    
    virtual DOMEventTarget* target() { return DOMEvent::target(); }
    
    virtual DOMEventTarget* currentTarget() { return DOMEvent::currentTarget(); }
    
    virtual unsigned short eventPhase() { return DOMEvent::eventPhase(); }
    
    virtual bool bubbles() { return DOMEvent::bubbles(); }
    
    virtual bool cancelable() { return DOMEvent::cancelable(); }
    
    virtual DOMTimeStamp timeStamp() { return DOMEvent::timeStamp(); }
    
    virtual void stopPropagation() { DOMEvent::stopPropagation(); }
    
    virtual void preventDefault() { DOMEvent::preventDefault(); }
    
    virtual void initEvent(const char* eventTypeArg, bool canBubbleArg, bool cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // DOMMutationEvent
    virtual DOMNode* relatedNode();
    
    virtual const char* prevValue();
    
    virtual const char* newValue();
    
    virtual const char* attrName();
    
    virtual unsigned short attrChange();
    
    virtual void initMutationEvent(const char* type, bool canBubble, bool cancelable, DOMNode* relatedNode, const char* prevValue, const char* newValue, const char* attrName, unsigned short attrChange);
};

class DOMOverflowEvent : public DOMEvent
{
public:
    DOMOverflowEvent(PassRefPtr<WebCore::Event> e) : DOMEvent(e) { }

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMEvent
    virtual const char* type() { return DOMEvent::type(); }
    
    virtual DOMEventTarget* target() { return DOMEvent::target(); }
    
    virtual DOMEventTarget* currentTarget() { return DOMEvent::currentTarget(); }
    
    virtual unsigned short eventPhase() { return DOMEvent::eventPhase(); }
    
    virtual bool bubbles() { return DOMEvent::bubbles(); }
    
    virtual bool cancelable() { return DOMEvent::cancelable(); }
    
    virtual DOMTimeStamp timeStamp() { return DOMEvent::timeStamp(); }
    
    virtual void stopPropagation() { DOMEvent::stopPropagation(); }
    
    virtual void preventDefault() { DOMEvent::preventDefault(); }
    
    virtual void initEvent(const char* eventTypeArg, bool canBubbleArg, bool cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }

    // DOMOverflowEvent
    virtual unsigned short orient();
    
    virtual bool horizontalOverflow();
    
    virtual bool verticalOverflow();
};

class DOMWheelEvent : public DOMUIEvent
{
public:
    DOMWheelEvent(PassRefPtr<WebCore::Event> e) : DOMUIEvent(e) { }

    /**
     * throw exception
     * @discussion Throws an exception in the current script execution context.
        @result Either NO if an exception could not be raised, YES otherwise.
     */
    virtual bool throwException(const char* exceptionMessage)
    { 
        return DOMObject::throwException(exceptionMessage); 
    }

    /**
     * remove web script key 
     * @param name The name of the method to call in the script environment.
        @param args The arguments to pass to the script environment.
        @discussion Calls the specified method in the script environment using the
        specified arguments.
        @result Returns the result of calling the script method.
     */
    virtual void removeWebScriptKey(const char* name)
    {
        DOMObject::removeWebScriptKey(name);
    }

    /**
     *  stringRepresentation 
     * @discussion Converts the target object to a string representation.  The coercion
        of non string objects type is dependent on the script environment.
        @result Returns the string representation of the object.
     */
    virtual const char* stringRepresentation()
    {
        return DOMObject::stringRepresentation();
    }

    /**
     *  setException 
     * @param description The description of the exception.
        @discussion Raises an exception in the script environment in the context of the
        current object.
     */
    virtual void setException(const char* description)
    {
        DOMObject::setException(description);
    }

    // IDOMEvent
    virtual const char* type() { return DOMEvent::type(); }
    
    virtual DOMEventTarget* target() { return DOMEvent::target(); }
    
    virtual DOMEventTarget* currentTarget() { return DOMEvent::currentTarget(); }
    
    virtual unsigned short eventPhase() { return DOMEvent::eventPhase(); }
    
    virtual bool bubbles() { return DOMEvent::bubbles(); }
    
    virtual bool cancelable() { return DOMEvent::cancelable(); }
    
    virtual DOMTimeStamp timeStamp() { return DOMEvent::timeStamp(); }
    
    virtual void stopPropagation() { DOMEvent::stopPropagation(); }
    
    virtual void preventDefault() { DOMEvent::preventDefault(); }
    
    virtual void initEvent(const char* eventTypeArg, bool canBubbleArg, bool cancelableArg) { return DOMEvent::initEvent(eventTypeArg, canBubbleArg, cancelableArg); }
    
    // IDOMUIEvent
    virtual WebCore::DOMWindow* view() { return DOMUIEvent::view(); }
    
    virtual long detail() { return DOMUIEvent::detail(); }
    
    virtual void initUIEvent(const char* type, bool canBubble, bool cancelable, WebCore::DOMWindow* view, long detail) { return DOMUIEvent::initUIEvent(type, canBubble, cancelable, view, detail); }
    
    virtual long keyCode() { return DOMUIEvent::keyCode(); }
    
    virtual long charCode() { return DOMUIEvent::charCode(); }
    
    virtual long layerX() { return DOMUIEvent::layerX(); }
    
    virtual long layerY() { return DOMUIEvent::layerY(); }
    
    virtual long pageX() { return DOMUIEvent::pageX(); }
    
    virtual long pageY() { return DOMUIEvent::pageY(); }
    
    virtual long which() { return DOMUIEvent::which(); }

    // IDOMWheelEvent
    virtual long screenX();
    
    virtual long screenY();
    
    virtual long clientX();
    
    virtual long clientY();
    
    virtual bool ctrlKey();
    
    virtual bool shiftKey();
    
    virtual bool altKey();
    
    virtual bool metaKey();
    
    virtual long wheelDelta();
    
    virtual long wheelDeltaX();
    
    virtual long wheelDeltaY();
    
    virtual long offsetX();
    
    virtual long offsetY();
    
    virtual long x();
    
    virtual long y();
    
    virtual bool isHorizontal();
    
    virtual void initWheelEvent(long wheelDeltaX, long wheelDeltaY, WebCore::DOMWindow* view, long screenX, long screenY, long clientX, long clientY, bool ctrlKey, bool altKey, bool shiftKey, bool metaKey);
};

#endif
