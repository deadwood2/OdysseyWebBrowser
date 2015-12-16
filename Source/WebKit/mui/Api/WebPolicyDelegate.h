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

#ifndef WebPolicyDelegate_h
#define WebPolicyDelegate_h

#include "SharedObject.h"
#include "WebKitTypes.h"

class WebError;
class WebFrame;
class WebFramePolicyListener;
class WebMutableURLRequest;
class WebNavigationAction;
class WebView;

namespace WebCore
{
    class ResourceResponse;
}

class WEBKIT_OWB_API WebPolicyDelegate : public SharedObject<WebPolicyDelegate> {
public:

    /**
     * DefaultPolicyDelegate destructor
     */
    virtual ~WebPolicyDelegate() {};

    /**
     * decide policy for navigation action
     * @param[in]: WebView
     * @param[in]: WebNavigationAction
     * @param[in]: WebMutableURLRequest
     * @param[in]: WebFrame
     * @param[in]: WebFramePolicyListener
     * @code
     * d->decidePolicyForNavigationAction(webView, actionInformation, request, frame, listener);
     * @endcode
     */
    virtual void decidePolicyForNavigationAction( 
        /* [in] */ WebView *webView,
        /* [in] */ WebNavigationAction *actionInformation,
        /* [in] */ WebMutableURLRequest *request,
        /* [in] */ WebFrame *frame,
        /* [in] */ WebFramePolicyListener *listener) = 0;
    
    /**
     * decide policy for new window action
     * @param[in]: WebView
     * @param[in]: WebNavigationAction
     * @param[in]: WebMutableURLRequest
     * @param[in]: frame name
     * @param[in]: WebFramePolicyListener
     * @code
     * d->decidePolicyForNewWindowAction(webView, actionInformation, request, frameName, listener);
     * @endcode
     */
    virtual void decidePolicyForNewWindowAction( 
        /* [in] */ WebView *webView,
        /* [in] */ WebNavigationAction *actionInformation,
        /* [in] */ WebMutableURLRequest *request,
        /* [in] */ const char* frameName,
        /* [in] */ WebFramePolicyListener *listener) = 0;
    
    /**
     * decide policy for MIMEType
     * @param[in]: WebView
     * @param[in]: ResourceResponse
     * @param[in]: WebMutableURLRequest
     * @param[in]: WebFrame
     * @param[in]: WebFramePolicyListener
     * @code
     * d->decidePolicyForMIMEType(webView, type, request, frame, listener);
     * @endcode
     */
    virtual bool decidePolicyForMIMEType( 
        /* [in] */ WebView *webView,
        /* [in] */ const WebCore::ResourceResponse &response,
        /* [in] */ WebMutableURLRequest *request,
        /* [in] */ WebFrame *frame,
        /* [in] */ WebFramePolicyListener *listener) = 0;
    
    /**
     * unable to implement policy with error
     * @param[in]: webView
     * @param[in]: error
     * @param[in]: frame
     * @code
     * d->unableToImplementPolicyWithError(webView, error, frame);
     * @endcode
     */
    virtual void unableToImplementPolicyWithError( 
        /* [in] */ WebView *webView,
        /* [in] */ WebError *error,
        /* [in] */ WebFrame *frame) = 0;
};

#endif // WebPolicyDelegate_h
