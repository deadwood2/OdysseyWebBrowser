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

#ifndef WebURLCredential_h
#define WebURLCredential_h


/**
 *  @file  WebURLCredential.h
 *  WebURLCredential description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:07 $
 */
#include "WebKitTypes.h"

typedef enum {
    WebURLCredentialPersistenceNone,
    WebURLCredentialPersistenceForSession,
    WebURLCredentialPersistencePermanent
} WebURLCredentialPersistence;

class WebURLCredentialPrivate;

namespace WebCore {
    class Credential;
}

class WEBKIT_OWB_API WebURLCredential
{
public:

    /**
     * create a new instance of WebURLCredential
     */
    static WebURLCredential* createInstance();


    /**
     * WebURLCredential destructor
     */
    virtual ~WebURLCredential();

    /**
     * test if the WebURLCredential has password
     */
    virtual bool hasPassword();

    /**
     * initialize WebURLCredential with user
     */
    virtual void initWithUser(const char* user, const char* password, WebURLCredentialPersistence persistence);

    /**
     * get password
     */
    virtual const char* password();

    /**
     * get url credential persistence
     */
    virtual WebURLCredentialPersistence persistence();

    /**
     * get user
     */
    virtual const char* user();

protected:
    friend class WebURLAuthenticationChallenge;
    friend class WebURLAuthenticationChallengeSender;

    WebCore::Credential& credential();
    static WebURLCredential* createInstance(const WebCore::Credential&);

private:
    WebURLCredential();
    WebURLCredential(const WebCore::Credential&);

    WebURLCredentialPrivate* m_priv;
};


#endif
