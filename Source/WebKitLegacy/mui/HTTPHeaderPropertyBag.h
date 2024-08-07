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

#ifndef HTTPHeaderPropertyBag_H
#define HTTPHeaderPropertyBag_H


/**
 *  @file  HTTPHeaderPropertyBag.h
 *  HTTPHeaderPropertyBag description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2013/03/06 00:13:16 $
 */
#include "BALBase.h"
#include <wtf/text/WTFString.h>
#include "WebURLResponse.h"
#include "WebURLResponse.h"

class HTTPHeaderPropertyBag {

public:

    /**
     * create new instance of HTTPHeaderPropertyBag
     * @param[out]: HTTPHeaderPropertyBag
     * @code
     * HTTPHeaderPropertyBag *h = HTTPHeaderPropertyBag::createInstance(response);
     * @endcode
     */
    static HTTPHeaderPropertyBag* createInstance(WebURLResponse*);


    /**
     * Read
     * @param[in]: property name
     * @param[out]: property value
     * @code
     * WTF::String s = h->Read(pName);
     * @endcode
     */
    virtual WTF::String Read(WTF::String pszPropName);


    /**
     * HTTPHeaderPropertyBag destructor
     */
    virtual ~HTTPHeaderPropertyBag();

private:

    /**
     * HTTPHeaderPropertyBag constructor
     * @param[in]: response
     */
    HTTPHeaderPropertyBag(WebURLResponse*);

    WebURLResponse* m_response;
};

#endif //HTTPHeaderPropertyBag_H
