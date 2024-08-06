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

#ifndef WebMutableURLRequest_H
#define WebMutableURLRequest_H


/**
 *  @file  WebMutableURLRequest.h
 *  WebMutableURLRequest description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:07 $
 */
#include "WebKitTypes.h"

typedef enum
{
    WebURLRequestUseProtocolCachePolicy,
    WebURLRequestReloadIgnoringCacheData,
    WebURLRequestReturnCacheDataElseLoad,
    WebURLRequestReturnCacheDataDontLoad
} WebURLRequestCachePolicy;

namespace WebCore
{
    class FormData;
    class HTTPHeaderMap;
    class ResourceRequest;
}

class HTTPHeaderPropertyBag;


class WEBKIT_OWB_API WebMutableURLRequest
{
public:

    /**
     * create a new instance of WebMutableURLRequest
     */
    static WebMutableURLRequest* createInstance();

    /**
     * create a new instance of WebMutableURLRequest
     */
    static WebMutableURLRequest* createInstance(WebMutableURLRequest* req);



    /**
     * create an immutable instance of WebMutableURLRequest
     */
    static WebMutableURLRequest* createImmutableInstance();

protected:
    friend class WebDownload;
    friend class WebDataSource;
    friend class WebFrame;
    friend class WebFrameLoaderClient;
    friend class WebChromeClient;
    friend class WebView;

    /**
     * create a new instance of WebMutableURLRequest
     */
    static WebMutableURLRequest* createInstance(const WebCore::ResourceRequest&);

    /**
     * create an immutable instance of WebMutableURLRequest
     */
    static WebMutableURLRequest* createImmutableInstance(const WebCore::ResourceRequest&);

    /**
     *  WebMutableURLRequest constructor
     */
    WebMutableURLRequest(bool isMutable);

public:

    /**
     * WebMutableURLRequest destructor
     */
    virtual ~WebMutableURLRequest();

    /**
     * request with URL 
     */
    virtual void requestWithURL( const char* theURL, WebURLRequestCachePolicy cachePolicy, double timeoutInterval);

    /**
     * get all HTTP header fields 
     */
    virtual HTTPHeaderPropertyBag* allHTTPHeaderFields();

    /**
     * get cachePolicy 
     */
    virtual WebURLRequestCachePolicy cachePolicy();

    /**
     * get HTTP body
     */
    virtual const char* HTTPBody();

    /**
     * get HTTP body stream
     */
    virtual const char* HTTPBodyStream();

    /**
     * get HTTP method
     */
    virtual const char* HTTPMethod();

    /**
     * get HTTP should handle cookies
     */
    virtual bool HTTPShouldHandleCookies();

    /**
     * initialise WebMutableURLRequest with URL 
     */
    virtual void initWithURL(const char* url, WebURLRequestCachePolicy cachePolicy, double timeoutInterval);

    /**
     * get main document URL
     */
    virtual const char* mainDocumentURL();

    /**
     * get timeout interval
     */
    virtual double timeoutInterval();

    /**
     * get URL
     */
    virtual const char* _URL();

    /**
     * get host
     */
    virtual const char* host();

    /**
     * scheme
     */
    virtual const char* protocol();

    /**
     * get value for HTTP header field
     */
    virtual const char* valueForHTTPHeaderField(const char* field);

    /**
     * test if the WebMutableURLRequest is empty
     */
    virtual bool isEmpty();


    /**
     * is equal
     */
    //virtual bool isEqual(WebURLRequest*);

    /**
     * add value
     */
    virtual void addValue(const char* value, const char* field);

    /**
     * set all HTTP header fields
     */
    virtual void setAllHTTPHeaderFields(HTTPHeaderPropertyBag *headerFields);

    /**
     * set cache policy
     */
    virtual void setCachePolicy(WebURLRequestCachePolicy policy);

    /**
     * set HTTP body
     */
    virtual void setHTTPBody(const char* data);

    /**
     * set HTTP body stream
     */
    virtual void setHTTPBodyStream(const char* data);

    /**
     * set HTTP method
     */
    virtual void setHTTPMethod(const char* method);

    /**
     * set HTTP should handle cookies
     */
    virtual void setHTTPShouldHandleCookies(bool handleCookies);

    /**
     * set main document URL
     */
    virtual void setMainDocumentURL(const char* theURL);

    /**
     * set timeout interval
     */
    virtual void setTimeoutInterval(double timeoutInterval);

    /**
     * set URL
     */
    virtual void setURL(const char* theURL);

    /**
     * set value
     */
    virtual void setValue(const char* value, const char* field);

    /**
     * set allows any HTTPS certificate
     */
    virtual void setAllowsAnyHTTPSCertificate();


    /**
     * setClientCertificate
     */
    //virtual void setClientCertificate(OLE_HANDLE cert);


   const WebCore::ResourceRequest& resourceRequest() const;

protected:
    /**
     * set form data
     */
    void setFormData(const WebCore::FormData* data);
    
    /**
     * get form data
     */
    const WebCore::FormData* formData() const;

    /**
     * get resource request
     */
    const WebCore::HTTPHeaderMap& httpHeaderFields() const;

    bool m_isMutable;
    WebCore::ResourceRequest* m_request;
};

#endif
