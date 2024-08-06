/*
 * Copyright (C) 2007 Pleyo.  All rights reserved.
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


#ifndef WebObject_h
#define WebObject_h

#include "SharedObject.h"
#include "TransferSharedPtr.h"
#include "WebKitDefines.h"
#include <vector>

// This statement simplify client code.
using std::vector;

namespace WebCore {
    class BALObject;
};

class WebValue;
class WebFrame;

/**
 * This class is used to expose a native C++ object in JavaScript
 * To use it, just inherits from WebObject and call addMethod / addProperty
 * in your constructor to register the JavaScript method properties.
 * Also implement invoke, getProperty and setProperty.
 */
class WEBKIT_OWB_API WebObject : public SharedObject<WebObject> {

public:
    explicit WebObject();
    explicit WebObject(WebCore::BALObject*);
    virtual ~WebObject() { deleteMe(); }

    bool hasMethod(const char* name);
    virtual const char* getName() = 0;
    bool hasProperty(const char* name);
    void addMethod(const char*);
    void removeMethod(const char*);
    void addProperty(const char*);
    void removeProperty(const char*);

    /**
     * @brief method called whenever JavaScript needs to execute a method on the object
     * @warning this method will be called only for methods registered with addMethod.
     * @param webFrame the WebFrame in which the call was made.
     * @param name the string representing the name of the method to be invoked.
     * @param args a vector of WebValue representing the arguments given during the invocation.
     */
    virtual TransferSharedPtr<WebValue> invoke(WebFrame* webFrame, const char* name, const std::vector<SharedPtr<WebValue> >& args) = 0;

    /**
     * @brief method called when JavaScript wants to get a property on the object.
     * @warning this method will be called only for the properties registered with addProperty.
     * @param webFrame the WebFrame in which the property request was made.
     * @param name the string representing the name of the property.
     * @return TransferSharedPtr<WebValue> a WebValue representing the property.
     */
    virtual TransferSharedPtr<WebValue> getProperty(WebFrame* webFrame, const char* name) = 0;

    /**
     * @brief method called when JavaScript wants to set a property on the object.
     * @warning this method will be called only for the properties registered with addProperty.
     * @param webFrame the WebFrame in which the property request was made.
     * @param name the string representing the name of the property.
     * @param value the new value.
     */
    virtual void setProperty(WebFrame* webFrame, const char* name, TransferSharedPtr<WebValue> value) = 0;

    /**
     * @internal
     */
    WebCore::BALObject* internalObject() const { return m_internalObject; }

private:
    // FIXME: This is needed to get the addon to compile. If we had not inlined, the destructor we would have issue
    // linking (undefined reference to WebObject's typeof [or vtable]).
    void deleteMe();

    WebCore::BALObject* m_internalObject;
};

#endif // WebObject_h
