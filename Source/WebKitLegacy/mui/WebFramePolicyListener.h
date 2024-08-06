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

#ifndef WebFramePolicyListener_h
#define WebFramePolicyListener_h


/**
 *  @file  WebFramePolicyListener.h
 *  WebFramePolicyListener description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */
#include "WebKitTypes.h"

typedef enum {
    WebPolicyUse,
    WebPolicyDownload,
    WebPolicyIgnore,
} WebPolicyAction;

class WebFrame;

class WebFramePolicyListener {
public:

    /**
     * @brief create a new instance of WebFramePolicyListener
     * @param webFrame the associated WebFrame
     * @return a newly allocated WebFramePolicyListener
     */
    static WebFramePolicyListener* createInstance(WebFrame* webFrame);

    /**
     * ~WebFramePolicyListener destructor
     */
    virtual ~WebFramePolicyListener();
protected:

    /**
     * WebFramePolicyListener constructor
     * @param webFrame the associated WebFrame
     * @internal
     */
    WebFramePolicyListener(WebFrame* webFrame);

public:

    /**
     * @brief Use the resource
     * @details If there remain more policy decisions to be made, then
     * the next policy delegate method gets to decide. This will be
     * either the next navigation policy delegate if there is a redirect,
     * or the content policy delegate. If there are no more policy
     * decisions to be made, the resource will be displayed inline if
     * possible. If there is no view available to display the resource
     * inline, then unableToImplementPolicyWithError:frame: will be
     * called with an appropriate error.
     *
     * If a new window is going to be created for this navigation as a
     * result of frame targetting, then it will be created once you call
     * this method.
     */
    virtual void use(void);

    /**
     * @brief Download the resource instead of displaying it.
     * @details This method is more than just a convenience because it
     * allows an in-progress navigation to be converted to a download
     * based on content type, without having to stop and restart the
     * load.
     */
    virtual void download(void);

    /**
     * @brief Do nothing (but the client may choose to handle the request itself)
     * @details A policy of ignore prevents WebKit from doing anything
     * further with the load, however, the client is still free to handle
     * the request in some other way, such as opening a new window,
     * opening a new window behind the current one, opening the URL in an
     * external app, revealing the location in Finder if a file URL, etc.
     */
    virtual void ignore(void);


    /**
     * @brief continue a form submission
     */
    virtual void continueSubmit(void);


    /**
     * @brief called when a policy decision should be made.
     * @param webPolicyAction the WebPolicyAction description the decision to be made.
     */
    void receivedPolicyDecision(WebPolicyAction webPolicyAction);

    /**
     * @brief invalidate a previous decision.
     */
    void invalidate();

private:
    WebFrame* m_frame;
};

#endif
