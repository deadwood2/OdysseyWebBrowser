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
#include "WebError.h"
#include "HTTPHeaderPropertyBag.h"

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <ResourceError.h>

using namespace WebCore;

WebError::WebError(const ResourceError& error, HTTPHeaderPropertyBag* userInfo)
    : m_userInfo(userInfo)
    , m_error(const_cast<ResourceError*>(&error))
{
}

WebError::~WebError()
{
    if(m_userInfo)
        delete m_userInfo;
}

WebError* WebError::createInstance(const ResourceError& error, HTTPHeaderPropertyBag* userInfo)
{
    WebError* instance = new WebError(error, userInfo);
    return instance;
}

WebError* WebError::createInstance()
{
    return createInstance(ResourceError());
}

void WebError::init(const char* domain, int code, const char* url)
{
    m_error = new ResourceError(domain, code, url, String());
}
  
int WebError::code()
{
    return m_error->errorCode();
}
        
const char* WebError::domain()
{
    return strdup(m_error->domain().utf8().data());
}
               
const char* WebError::localizedDescription()
{
    return strdup(m_error->localizedDescription().utf8().data());
}

        
const char* WebError::localizedFailureReason()
{
    return "";
}
        
const char* WebError::localizedRecoverySuggestion()
{
    return "";
}
       
HTTPHeaderPropertyBag* WebError::userInfo()
{
    return m_userInfo;
}

const char* WebError::failingURL()
{
    return strdup(m_error->failingURL().utf8().data());
}

bool WebError::isPolicyChangeError()
{
    return m_error->domain() == String("WebKitErrorDomain")
        && m_error->errorCode() == WebKitErrorFrameLoadInterruptedByPolicyChange;
}

const ResourceError& WebError::resourceError() const
{
    return *m_error;
}

