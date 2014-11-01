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

#ifndef WebSecurityOrigin_h
#define WebSecurityOrigin_h


/**
 *  @file  WebSecurityOrigin.h
 *  WebSecurityOrigin description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:07 $
 */
#include "WebKitTypes.h"

namespace WebCore {
    class SecurityOrigin;
}

class WEBKIT_OWB_API WebSecurityOrigin {
protected:
    friend class WebChromeClient;
    friend class WebDatabaseManager;
    friend class WebDatabaseTracker;
    friend class WebFrameLoaderClient;

    /**
     * create a new instance of WebSecurityOrigin
     */
    static WebSecurityOrigin* createInstance(WebCore::SecurityOrigin* origin);

    /**
     * get SecurityOrigin 
     */
    WebCore::SecurityOrigin* securityOrigin() const { return m_securityOrigin; }

public:

    /**
     * WebSecurityOrigin destructor
     */
    virtual ~WebSecurityOrigin();

    /**
     * get protocol
     */
    virtual const char* protocol();

    /**
     * get domain
     */
    virtual const char* host();

    /**
     * get port
     */
    virtual unsigned short port();

    /**
     * get usage
     */
    virtual unsigned long long usage();

    /**
     * get database quota
     */
    virtual unsigned long long quota();

    /**
     * set database quota
     */
    virtual void setQuota(unsigned long long);
private:

    /**
     * WebSecurityOrigin constructor
     */
    WebSecurityOrigin(WebCore::SecurityOrigin*);

    WebCore::SecurityOrigin* m_securityOrigin;
};

#endif // WebSecurityOrigin_h
