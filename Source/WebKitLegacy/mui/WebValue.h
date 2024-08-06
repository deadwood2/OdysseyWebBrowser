/*
 * Copyright (C) 2009 Pleyo.  All rights reserved.
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

#ifndef WebValue_h
#define WebValue_h

#include "WebKitDefines.h"
#include "SharedObject.h"
#include "SharedPtr.h"
#include "TransferSharedPtr.h"

namespace WebCore {
    class BALValue;
};

class WebArray;
class WebFrame;
class WebObject;

class WEBKIT_OWB_API WebValue : public SharedObject<WebValue> {

public:
    static TransferSharedPtr<WebValue> createInstance(WebFrame*);
    static TransferSharedPtr<WebValue> createInstance(WebCore::BALValue*);

   ~WebValue();

    bool isUndefined() const;
    bool isNull() const;
    bool isUndefinedOrNull() const;
    bool isBoolean() const;
    bool isNumber() const;
    bool isString() const;
    bool isGetterSetter() const;
    /* 
     * isObject
     * @discussion returns whether this value is a WebObject.
     */
    bool isObject() const;
    bool isException() const;
    bool isArray() const;

    bool toBoolean() const;
    double toNumber() const;
    int toInt(bool& ok) const; // Helper method
    const char* toString() const;
    TransferSharedPtr<WebObject> toObject() const;
    WebArray* toArray() const;

    // Return value converted from string color (named or valued) to binary color. returns 0x0 if problem when decoding color.
    unsigned int toRGBA32() const;

    TransferSharedPtr<WebObject> toWebObject() const;
    WebArray* toWebArray() const;

    void balUndefined();
    void balNull();
    void balNaN();
    void balBoolean(bool b);
    void balNumber(double d);
    void balString(const char* s);
    void balObject(TransferSharedPtr<WebObject> obj);
    void balException(short code, const char* name, const char* description);
    void balArray(WebArray* array);

    WebFrame* webFrame() const { return m_webFrame; }

    private:
    friend class WebValueHelper;
    friend class WebFrame;

    WebValue(WebFrame*);
    explicit WebValue(WebCore::BALValue*);
 
    WebCore::BALValue* m_val;
    SharedPtr<WebObject> m_obj;
    WebArray* m_array;
    WebFrame* m_webFrame; // Could this member be removed?
};

#endif // WebValue_h
