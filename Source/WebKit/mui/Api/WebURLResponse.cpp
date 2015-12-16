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

#include "HTTPHeaderPropertyBag.h"

#include <wtf/Platform.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <URL.h>
#include <ResourceHandle.h>
#include <ResourceResponse.h>

using namespace WebCore;

static const char* HTTPMessageCopyLocalizedShortDescriptionForStatusCode(int statusCode)
{
    if (statusCode < 100 || statusCode >= 600)
        return "Server Error";
    else if (statusCode >= 100 && statusCode <= 199) {
        switch (statusCode) {
            case 100:
                return "Continue";
            case 101:
                return "Switching Protocols";
            default:
                return "Informational";
        }
    } else if (statusCode >= 200 && statusCode <= 299) {
        switch (statusCode) {
            case 200:
                return "OK";
            case 201:
                return "Created";
            case 202:
                return "Accepted";
            case 203:
                return "Non-Authoritative Information";
            case 204:
                return "No Content";
            case 205:
                return "Reset Content";
            case 206:
                return "Partial Content";
            default:
                return "Success";
        } 
    } else if (statusCode >= 300 && statusCode <= 399) {
        switch (statusCode) {
            case 300:
                return "Multiple Choices";
            case 301:
                return "Moved Permanently";
            case 302:
                return "Found";
            case 303:
                return "See Other";
            case 304:
                return "Not Modified";
            case 305:
                return "Use Proxy";
            case 307:
                return "Temporarily Redirected";
            case 306:   // 306 status code unused in HTTP
            default:
                return "Redirected";
        }
    } else if (statusCode >= 400 && statusCode <= 499) {
        switch (statusCode) {
            case 400:
                return "Bad Request";
            case 401:
                return "Unauthorized";
            case 402:
                return "Payment Required";
            case 403:
                return "Forbidden";
            case 404:
                return "Not Found";
            case 405:
                return "Method Not Allowed";
            case 406:
                return "Not Acceptable";
            case 407:
                return "Proxy Authentication Required";
            case 408:
                return "Request Timed Out";
            case 409:
                return "Conflict";
            case 410:
                return "Gone";
            case 411:
                return "Length Required";
            case 412:
                return "Precondition Failed";
            case 413:
                return "Request Entity Too Large";
            case 414:
                return "Requested-URI Too Long";
            case 415:
                return "Unsupported Media Type";
            case 416:
                return "Requested range not satisfiable";
            case 417:
                return "Expectation Failed";
            default:
                return "Client Error";
        }
    } else if (statusCode >= 500 && statusCode <= 599) {
        switch (statusCode) {
            case 500:
                return "Internal Server Error";
            case 501:
                return "Not Implemented";
            case 502:
                return "Bad Gateway";
            case 503:
                return "Service Unavailable";
            case 504:
                return "Gateway Time-out";
            case 505:
                return "HTTP Version not supported";
            default:
                return "Server Error";
        }
    }
    return "";
}

WebURLResponse::WebURLResponse()
{
}

WebURLResponse::~WebURLResponse()
{
}

WebURLResponse* WebURLResponse::createInstance()
{
    WebURLResponse* instance = new WebURLResponse();
    // fake an http response - so it has the IWebHTTPURLResponse interface
    instance->m_response = new ResourceResponse(URL(ParsedURLString, "http://"), String(), 0, String());
    return instance;
}

WebURLResponse* WebURLResponse::createInstance(const ResourceResponse& response)
{
    if (response.isNull())
        return 0;

    WebURLResponse* instance = new WebURLResponse();
    instance->m_response = const_cast<ResourceResponse*>(&response);

    return instance;
}

long long WebURLResponse::expectedContentLength()
{
    return m_response->expectedContentLength();
}

void WebURLResponse::initWithURL(const char* url, const char* mimeType, int expectedContentLength, const char* textEncodingName)
{
    m_response = new ResourceResponse(URL(ParsedURLString, url), String(mimeType), expectedContentLength, String(textEncodingName));
}

const char* WebURLResponse::MIMEType()
{
    return strdup(m_response->mimeType().utf8().data());
}

const char* WebURLResponse::suggestedFilename()
{
    return strdup(m_response->suggestedFilename().utf8().data());
}

const char* WebURLResponse::textEncodingName()
{
    return strdup(m_response->textEncodingName().utf8().data());
}

const char* WebURLResponse::_URL()
{
    return strdup(m_response->url().string().utf8().data());
}

HTTPHeaderPropertyBag* WebURLResponse::allHeaderFields()
{
    ASSERT(m_response->isHTTP());
    return HTTPHeaderPropertyBag::createInstance(this);
}

const char* WebURLResponse::localizedStringForStatusCode(int statusCode)
{
    ASSERT(m_response->isHTTP());
    
    return HTTPMessageCopyLocalizedShortDescriptionForStatusCode(statusCode);
}

int WebURLResponse::statusCode()
{
    ASSERT(m_response->isHTTP());
    return m_response->httpStatusCode();
}

bool WebURLResponse::isAttachment()
{
    return m_response->isAttachment();
}


/*OLE_HANDLE WebURLResponse::sslPeerCertificate()
{
#if USE(CFNETWORK)
    CFDictionaryRef dict = certificateDictionary();
    if (!dict)
        return E_FAIL;
    void* data = wkGetSSLPeerCertificateData(dict);
    if (!data)
        return E_FAIL;
    *result = (OLE_HANDLE)(ULONG64)data;
#endif

    return *result ? S_OK : E_FAIL;
}*/


const char* WebURLResponse::suggestedFileExtension()
{
    //FIXME : trouver l'extension

//     if (m_response->mimeType().isEmpty())
//         return E_FAIL;
// 
//     BString mimeType(m_response->mimeType());
//     HKEY key;
//     LONG err = RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("MIME\\Database\\Content Type"), 0, KEY_QUERY_VALUE, &key);
//     if (!err) {
//         HKEY subKey;
//         err = RegOpenKeyEx(key, mimeType, 0, KEY_QUERY_VALUE, &subKey);
//         if (!err) {
//             DWORD keyType = REG_SZ;
//             TCHAR extension[MAX_PATH];
//             DWORD keySize = sizeof(extension)/sizeof(extension[0]);
//             err = RegQueryValueEx(subKey, TEXT("Extension"), 0, &keyType, (LPBYTE)extension, &keySize);
//             if (!err && keyType != REG_SZ)
//                 err = ERROR_INVALID_DATA;
//             if (err) {
//                 // fallback handlers
//                 if (!_tcscmp(mimeType, TEXT("text/html"))) {
//                     _tcscpy(extension, TEXT(".html"));
//                     err = 0;
//                 } else if (!_tcscmp(mimeType, TEXT("application/xhtml+xml"))) {
//                     _tcscpy(extension, TEXT(".xhtml"));
//                     err = 0;
//                 } else if (!_tcscmp(mimeType, TEXT("image/svg+xml"))) {
//                     _tcscpy(extension, TEXT(".svg"));
//                     err = 0;
//                 }
//             }
//             if (!err) {
//                 *result = SysAllocString(extension);
//                 if (!*result)
//                     err = ERROR_OUTOFMEMORY;
//             }
//             RegCloseKey(subKey);
//         }
//         RegCloseKey(key);
//     }
// 
//     return HRESULT_FROM_WIN32(err);
    return "";
}

const ResourceResponse& WebURLResponse::resourceResponse() const
{
    return *m_response;
}
