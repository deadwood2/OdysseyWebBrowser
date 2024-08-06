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

#ifndef WebURLAuthenticationChallengeSender_h
#define WebURLAuthenticationChallengeSender_h


/**
 *  @file  WebURLAuthenticationChallengeSender.h
 *  WebURLAuthenticationChallengeSender description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:07 $
 */
#include "WebKitTypes.h"
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class AuthenticationClient;
	class ResourceHandle;
}

class WebURLAuthenticationChallenge;
class WebURLCredential;

class WebURLAuthenticationChallengeSender {
protected:
    friend class WebURLAuthenticationChallenge;
    /**
     * create a new instance of WebURLAuthenticationChallengeSender
     */
    static WebURLAuthenticationChallengeSender* createInstance(WebCore::AuthenticationClient*);
	static WebURLAuthenticationChallengeSender* createInstance(PassRefPtr<WebCore::ResourceHandle>);
private:

    /**
     *  WebURLAuthenticationChallengeSender constructor
     */
    WebURLAuthenticationChallengeSender(WebCore::AuthenticationClient*);
	WebURLAuthenticationChallengeSender(PassRefPtr<WebCore::ResourceHandle>);
public:

    /**
     * WebURLAuthenticationChallengeSender delete
     */
    virtual ~WebURLAuthenticationChallengeSender();

    /**
     * cancel authentication challenge
     */
    virtual void cancelAuthenticationChallenge(WebURLAuthenticationChallenge* challenge);

    /**
     * continue without credential for authentication challenge
     */
    virtual void continueWithoutCredentialForAuthenticationChallenge(WebURLAuthenticationChallenge* challenge);

    /**
     * use credential
     */
    virtual void useCredential(WebURLCredential* credential, WebURLAuthenticationChallenge* challenge);

protected:
    /**
     * get resource handle
     */
    WebCore::AuthenticationClient* authenticationClient() const;

    /**
     * get resource handle
     */
    WebCore::ResourceHandle* resourceHandle() const;

private:

    WebCore::AuthenticationClient* m_authenticationClient;
	RefPtr<WebCore::ResourceHandle> m_handle;
};

#endif
