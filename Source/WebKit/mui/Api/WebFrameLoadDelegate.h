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

#ifndef WebFrameLoadDelegate_h
#define WebFrameLoadDelegate_h

#include "SharedObject.h"

class WebFrame;
class WebError;
class WebSecurityOrigin;

class WebFrameLoadDelegate : public SharedObject<WebFrameLoadDelegate> {

public:
    virtual ~WebFrameLoadDelegate() { }

    /**
     * windowObjectClearNotification
     */
    virtual void windowObjectClearNotification(WebFrame*, void*, void*) = 0;

    /**
     * titleChange : call when the title change
     */
    virtual void titleChange(WebFrame*, const char*) = 0;

    /**
     * didStartLoad : called when a frame starts loading
     */
    virtual void didStartProvisionalLoad(WebFrame*) = 0;

    /**
     * didReceiveServerRedirectForProvisionalLoadForFrame: called when a server-side redirection occurs.
     */
    virtual void didReceiveServerRedirectForProvisionalLoadForFrame(WebFrame*) = 0;

    /**
     * willPerformClientRedirectToURL: called when a client-side will occur.
     */
    virtual void willPerformClientRedirectToURL(WebFrame*, const char* url, double delaySeconds, double fireDate) = 0;

    /**
     * didCancelClientRedirectForFrame: called when a client-side redirection was cancelled before it occurred.
     */
    virtual void didCancelClientRedirectForFrame(WebFrame*) = 0;

    /**
     * didCommitLoad : called when a frame data source transitions from provisional state to committed
     */
    virtual void didCommitLoad(WebFrame*) = 0;

    /**
     * didFinishLoad : called when a frame has finished loading itself and its resources
     */
    virtual void didFinishLoad(WebFrame*) = 0;

    /**
     * didFailProvisionalLoad : called if a frame provisional loading failed
     */
    virtual void didFailProvisionalLoad(WebFrame*, WebError*) = 0;

    /**
     * didFailLoad : called if a frame loading failed
     */
    virtual void didFailLoad(WebFrame*, WebError*) = 0;

    /*
     * didFinishDocumentLoadForFrame : called if a document loading has finished
     */
    virtual void didFinishDocumentLoadForFrame(WebFrame*) = 0;

    /*
     * didHandleOnloadEventsForFrame : call when an onload are send on frame
     */
    virtual void didHandleOnloadEventsForFrame(WebFrame*) = 0;

    /*
     * didChangeLocationWithinPageForFrame
     */
    virtual void didChangeLocationWithinPageForFrame(WebFrame*) = 0;

    /*
     * willCloseFrame
     */
    virtual void willCloseFrame(WebFrame*) = 0;

    /*
     * didFirstLayoutInFrame
     */
    virtual void didFirstLayoutInFrame(WebFrame*) = 0;

    /*
     * didFirstVisuallyNonEmptyLayoutInFrame
     */
    virtual void didFirstVisuallyNonEmptyLayoutInFrame(WebFrame*) = 0;

    /*
     * dispatch not enought memory
     */
	virtual void dispatchNotEnoughMemory(WebFrame*) = 0;

    /*
     * didDisplayInsecureContent
     */
    virtual void didDisplayInsecureContent(WebFrame*) = 0;

    /*
     * didRunInsecureContent
     */
    virtual void didRunInsecureContent(WebFrame*, WebSecurityOrigin*) = 0;
    
    /*
     * didChangeIcons
     */
    virtual void didChangeIcons(WebFrame*) {}
};

#endif // WebFrameLoadDelegate_h
