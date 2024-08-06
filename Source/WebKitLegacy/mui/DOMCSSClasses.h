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

#ifndef DOMCSSClasses_H
#define DOMCSSClasses_H

#include "DOMCoreClasses.h"

#include <CSSStyleDeclaration.h>

class DOMCSSStyleDeclaration : public DOMObject
{
private:
    DOMCSSStyleDeclaration(WebCore::CSSStyleDeclaration* d);

protected:
    friend class DOMDocument;
    friend class DOMElement;
    friend class WebEditorClient;
    static DOMCSSStyleDeclaration* createInstance(WebCore::CSSStyleDeclaration* d);

public:
    ~DOMCSSStyleDeclaration();
    // WebScriptObject
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

    // DOMCSSStyleDeclaration
    virtual const char* cssText();
    
    virtual void setCssText(const char* cssText);
    
    virtual const char* getPropertyValue(const char* propertyName);
    
    //virtual DOMCSSValue* getPropertyCSSValue(const char* propertyName);
    
    virtual const char* removeProperty(const char* propertyName);
    
    virtual const char* getPropertyPriority(const char* propertyName);
    
    virtual void setProperty(const char* propertyName, const char* value, const char* priority);
    
    virtual unsigned length();
    
    virtual const char* item(unsigned index);
    
//    virtual DOMCSSRule* parentRule();

protected:
    unsigned long m_refCount;
    WebCore::CSSStyleDeclaration* m_style;
};

#endif
