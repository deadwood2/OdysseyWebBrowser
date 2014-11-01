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

#ifndef WebURLAuthenticationChallenge_h
#define WebURLAuthenticationChallenge_h


/**
 *  @file  WebURLAuthenticationChallenge.h
 *  WebURLAuthenticationChallenge description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:07 $
 */
#include "WebKitTypes.h"

class WebURLProtectionSpace;
class WebURLCredential;
class WebURLResponse;
class WebError;
class WebURLAuthenticationChallengeSender;

namespace WebCore {
    class AuthenticationChallenge;
}
class WebURLAuthenticationChallengePrivate;

class WEBKIT_OWB_API WebURLAuthenticationChallenge {
protected:
    friend class WebFrameLoaderClient;
    friend class WebURLAuthenticationChallengeSender;
    /**
     * create a new instance of WebURLAuthenticationChallenge
     */
    static WebURLAuthenticationChallenge* createInstance(const WebCore::AuthenticationChallenge&);

    /**
     * create a new instance of WebURLAuthenticationChallenge
     */
    static WebURLAuthenticationChallenge* createInstance(const WebCore::AuthenticationChallenge&, WebURLAuthenticationChallengeSender*);
private:

    /**
     * WebURLAuthenticationChallenge constructor
     */
    WebURLAuthenticationChallenge(const WebCore::AuthenticationChallenge&, WebURLAuthenticationChallengeSender*);
public:

    /**
     * WebURLAuthenticationChallenge destructor
     */
    virtual ~WebURLAuthenticationChallenge();

    /**
     * initialise WebURLAuthenticationChallenge with protection space 
     */
    virtual void initWithProtectionSpace(WebURLProtectionSpace* space, WebURLCredential* proposedCredential, int previousFailureCount, WebURLResponse* failureResponse, WebError* error, WebURLAuthenticationChallengeSender* sender);

    /**
     *  initialise WebURLAuthenticationChallenge with authentication challenge
     */
    virtual void initWithAuthenticationChallenge(WebURLAuthenticationChallenge* challenge, WebURLAuthenticationChallengeSender* sender);

    /**
     * get error
     */
    virtual WebError* error();

    /**
     * get failure response
     */
    virtual WebURLResponse* failureResponse();

    /**
     * get previous failure count
     */
    virtual unsigned previousFailureCount();

    /**
     * get proposed credential
     */
    virtual WebURLCredential* proposedCredential();

    /**
     * get protection space
     */
    virtual WebURLProtectionSpace* protectionSpace();

    /**
     * get sender
     */
    virtual WebURLAuthenticationChallengeSender* sender();
   

protected:
    /**
     * get authentication challenge
     */ 
    const WebCore::AuthenticationChallenge& authenticationChallenge() const;

private:
    WebURLAuthenticationChallengePrivate* m_priv;    
    WebURLAuthenticationChallengeSender* m_sender;
};


#endif
