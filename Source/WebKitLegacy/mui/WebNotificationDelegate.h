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

#ifndef WebNotificationDelegate_h
#define WebNotificationDelegate_h

#include "SharedObject.h"

class WebFrame;

// This is a custom extension
// It is kept for compatibility reason but should be rolled out someday.
// WARNING: IF YOU WANT TO ADD A METHOD HERE, THINK TWICE AS IT SHOULD NOT BE
// THE RIGHT PLACE.

class WebNotificationDelegate : public SharedObject<WebNotificationDelegate> {

public:
    virtual ~WebNotificationDelegate() { }

    /**
     * startLoadNotification: called when a frame starts loading its resources.
     * @discussion: This is called for each frame so it can be called several
     *              time for the same WebView.
     * @discussion: Deprecated, use WebFrameLoad::didStartLoad instead.
     */
    virtual void startLoadNotification(WebFrame*) = 0;

    /**
     * progressNotification: called by the WebView each time there was progress in loading the page
     * @discussion: This can be called several time for the same WebView::estimateProgress.
     */
    virtual void progressNotification(WebFrame*) = 0;

    /**
     * finishedLoadNotification: called when a frame has finished loading its resources.
     * @discussion: This is called for each frame so it can be called several
     *              time for the same WebView.
     * @discussion: Deprecated, use WebFrameLoad::didFinishLoad instead.
     */
    virtual void finishedLoadNotification(WebFrame*) = 0;

};

#endif // WebNotificationDelegate_h
