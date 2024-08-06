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
#ifndef WebNavigationData_h
#define WebNavigationData_h

#include <WebKitTypes.h>
#include <string>

class WebNavigationData;
class WebMutableURLRequest;
class WebURLResponse;

class WEBKIT_OWB_API WebNavigationData {

    public:
    static WebNavigationData* createInstance(const char* url, const char* title, WebMutableURLRequest* request, WebURLResponse* response, bool hasSubstituteData, const char* clientRedirectSource)
    {
        return new WebNavigationData(url, title, request, response, hasSubstituteData, clientRedirectSource);
    }

    ~WebNavigationData()
    {
    }

    const char* url() const { return m_url.c_str(); }
    const char* title() const { return m_title.c_str(); }
    WebMutableURLRequest* originalRequest() const { return m_request; }
    WebURLResponse* response() const { return m_response; }
    bool hasSubstituteData() const { return m_hasSubstituteData; }
    const char* clientRedirectSource() const { return m_clientRedirectSource.c_str(); }

    private:
    std::string m_url;
    std::string m_title;
    WebMutableURLRequest* m_request;
    WebURLResponse* m_response;
    bool m_hasSubstituteData;
    std::string m_clientRedirectSource;

    private:
    WebNavigationData(const char* url, const char* title, WebMutableURLRequest* request, WebURLResponse* response, bool hasSubstituteData, const char* clientRedirectSource)
        : m_url(url)
        , m_title(title)
        , m_request(request)
        , m_response(response)
        , m_hasSubstituteData(hasSubstituteData)
        , m_clientRedirectSource(clientRedirectSource)
    {

    }


};

#endif // WebNavigationData_h
