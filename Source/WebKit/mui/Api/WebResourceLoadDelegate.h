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


#ifndef WebResourceLoadDelegate_h
#define WebResourceLoadDelegate_h

#include "SharedObject.h"

class WebDataSource;
class WebError;
class WebMutableURLRequest;
class WebURLResponse;
class WebURLAuthenticationChallenge;
class WebView;

class WebResourceLoadDelegate : public SharedObject<WebResourceLoadDelegate> {
public:
    virtual ~WebResourceLoadDelegate() {};

    virtual void identifierForInitialRequest(WebView *webView, WebMutableURLRequest *request, WebDataSource *dataSource, unsigned long identifier) = 0;
        
    virtual WebMutableURLRequest* willSendRequest(WebView *webView, unsigned long identifier, WebMutableURLRequest *request, WebURLResponse *redirectResponse, WebDataSource *dataSource) = 0;
        
    virtual void didReceiveAuthenticationChallenge(WebView *webView, unsigned long identifier, WebURLAuthenticationChallenge *challenge, WebDataSource *dataSource) = 0;
        
    virtual void didCancelAuthenticationChallenge(WebView *webView, unsigned long identifier, WebURLAuthenticationChallenge *challenge, WebDataSource *dataSource) = 0;
        
    virtual void didReceiveResponse(WebView *webView, unsigned long identifier, WebURLResponse *response, WebDataSource *dataSource) = 0;
        
    virtual void didReceiveContentLength(WebView *webView, unsigned long identifier, unsigned length, WebDataSource *dataSource) = 0;
        
    virtual void didFinishLoadingFromDataSource(WebView *webView, unsigned long identifier, WebDataSource *dataSource) = 0;
        
    virtual void didFailLoadingWithError(WebView *webView, unsigned long identifier, WebError *error, WebDataSource *dataSource) = 0;
        
    virtual void plugInFailedWithError(WebView *webView, WebError *error, WebDataSource *dataSource) = 0;
};

#endif // WebResourceLoadDelegate_h
