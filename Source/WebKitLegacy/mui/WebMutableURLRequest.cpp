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
#include "WebMutableURLRequest.h"

#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <FormData.h>
#include <ResourceHandle.h>
#include <ResourceRequest.h>
#include <ResourceRequestBase.h>

using namespace WebCore;

inline WebCore::ResourceRequestCachePolicy core(WebURLRequestCachePolicy policy)
{
    return static_cast<WebCore::ResourceRequestCachePolicy>(policy);
}

inline WebURLRequestCachePolicy kit(WebCore::ResourceRequestCachePolicy policy)
{
    return static_cast<WebURLRequestCachePolicy>(policy);
}

WebMutableURLRequest::WebMutableURLRequest(bool isMutable)
    : m_isMutable(isMutable)
{
}

WebMutableURLRequest* WebMutableURLRequest::createInstance()
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(true);
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createInstance(WebMutableURLRequest* req)
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(true);
    instance->m_request = static_cast<WebMutableURLRequest*>(req)->m_request;
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createInstance(const ResourceRequest& request)
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(true);
    instance->m_request = const_cast<ResourceRequest*>(&request);
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createImmutableInstance()
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(false);
    return instance;
}

WebMutableURLRequest* WebMutableURLRequest::createImmutableInstance(const ResourceRequest& request)
{
    WebMutableURLRequest* instance = new WebMutableURLRequest(false);
    instance->m_request = const_cast<ResourceRequest*>(&request);
    return instance;
}

WebMutableURLRequest::~WebMutableURLRequest()
{
}

void WebMutableURLRequest::requestWithURL(const char* /*theURL*/, WebURLRequestCachePolicy /*cachePolicy*/, double /*timeoutInterval*/)
{
}

HTTPHeaderPropertyBag* WebMutableURLRequest::allHTTPHeaderFields()
{
    return 0;
}

WebURLRequestCachePolicy WebMutableURLRequest::cachePolicy()
{
    return kit(m_request->cachePolicy());
}

const char* WebMutableURLRequest::HTTPBody()
{
    return "";
}

const char* WebMutableURLRequest::HTTPBodyStream()
{
    return "";
}

const char* WebMutableURLRequest::HTTPMethod()
{
    return strdup(m_request->httpMethod().utf8().data());
}

bool WebMutableURLRequest::HTTPShouldHandleCookies()
{
  return true;
  //    return m_request.allowCookies();
}

void WebMutableURLRequest::initWithURL(const char* url, WebURLRequestCachePolicy cachePolicy, double timeoutInterval)
{
    m_request->setURL(URL(ParsedURLString, url));
    m_request->setCachePolicy(core(cachePolicy));
    m_request->setTimeoutInterval(timeoutInterval);
}

const char* WebMutableURLRequest::mainDocumentURL()
{
  return strdup(m_request->firstPartyForCookies().string().utf8().data());
}

double WebMutableURLRequest::timeoutInterval()
{
    return m_request->timeoutInterval();
}

const char* WebMutableURLRequest::_URL()
{
    return strdup(m_request->url().string().utf8().data());
}

const char* WebMutableURLRequest::host()
{
    return strdup(m_request->url().host().utf8().data());
}

const char* WebMutableURLRequest::protocol()
{
    return strdup(m_request->url().protocol().utf8().data());
}

const char* WebMutableURLRequest::valueForHTTPHeaderField(const char* field)
{
    return strdup(m_request->httpHeaderField(field).utf8().data());
}

bool WebMutableURLRequest::isEmpty()
{
    return m_request->isEmpty();
}

/*
bool isEqual(WebURLRequest*)
{
    return false;
}
*/

void WebMutableURLRequest::addValue(const char* value, const char* field)
{
  //    m_request.addHTTPHeaderField(WTF::AtomicString(value), String(field)); 
}

void WebMutableURLRequest::setAllHTTPHeaderFields(HTTPHeaderPropertyBag *)
{
}

void WebMutableURLRequest::setCachePolicy(WebURLRequestCachePolicy policy)
{
    m_request->setCachePolicy(core(policy));
}

void WebMutableURLRequest::setHTTPBody(const char*)
{
}

void WebMutableURLRequest::setHTTPBodyStream(const char*)
{
}

void WebMutableURLRequest::setHTTPMethod(const char* method)
{
    m_request->setHTTPMethod(method);
}

void WebMutableURLRequest::setHTTPShouldHandleCookies(bool /*handleCookies*/)
{
}

void WebMutableURLRequest::setMainDocumentURL(const char* theURL)
{
  //m_request.setFirstPartyForCookies(URL(ParsedURLString, theURL));
}

void WebMutableURLRequest::setTimeoutInterval(double timeoutInterval)
{
    m_request->setTimeoutInterval(timeoutInterval);
}

void WebMutableURLRequest::setURL(const char* url)
{
    m_request->setURL(URL(ParsedURLString, url));
}

void WebMutableURLRequest::setValue(const char* value, const char* field)
{
    m_request->setHTTPHeaderField(field, value);
}

void WebMutableURLRequest::setAllowsAnyHTTPSCertificate()
{
    //ResourceHandle::setHostAllowsAnyHTTPSCertificate(m_request->url().host());
}

// void WebMutableURLRequest::setClientCertificate(OLE_HANDLE cert)
// {
//     if (!cert)
//         return E_POINTER;
// 
//     PCCERT_CONTEXT certContext = reinterpret_cast<PCCERT_CONTEXT>((ULONG64)cert);
//     RetainPtr<CFDataRef> certData(AdoptCF, copyCert(certContext));
//     ResourceHandle::setClientCertificate(m_request->url().host(), certData.get());
//     return S_OK;
// }

void WebMutableURLRequest::setFormData(const FormData* data)
{
    m_request->setHTTPBody(const_cast<FormData*>(data));
}

const FormData* WebMutableURLRequest::formData() const
{
    return m_request->httpBody();
}

const HTTPHeaderMap& WebMutableURLRequest::httpHeaderFields() const
{
    return m_request->httpHeaderFields();
}

const ResourceRequest& WebMutableURLRequest::resourceRequest() const
{
    return *m_request;
}

