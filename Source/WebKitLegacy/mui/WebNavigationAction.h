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

#ifndef WebNavigationAction_h
#define WebNavigationAction_h


/**
 *  @file  WebNavigationAction.h
 *  WebNavigationAction description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:07 $
 */
#include "WebKitTypes.h"
#include <string>

typedef enum {
    WebNavigationTypeLinkClicked,
    WebNavigationTypeFormSubmitted,
    WebNavigationTypeBackForward,
    WebNavigationTypeReload,
    WebNavigationTypeFormResubmitted,
    WebNavigationTypeOther
} WebNavigationType;

typedef enum {
    WEBALT = 1,
    WEBCONTROL = 2,
    WEBSHIFT = 4
} WebModifiers;

namespace WebCore {
    class HTMLFormElement;
    class NavigationAction;
}

class DOMNode;
class WebFrame;
class WebHitTestResults;


class WEBKIT_OWB_API WebNavigationAction
{
public:

    /**
     * create new instance of WebNavigationAction
     * @param[in]: NavigationType
     * @param[in]: url
     * @param[out]: WebNavigationAction
     * @code
     * WebNavigationAction* w = WebNavigationAction::createInstance(action, f);
     * @endcode
     */
    static WebNavigationAction* createInstance();

protected:
    friend class WebFrameLoaderClient;
    /**
     * create new instance of WebNavigationAction
     * @param[in]: NavigationAction
     * @param[out]: WebNavigationAction
     */

    static WebNavigationAction* createInstance(const WebCore::NavigationAction*, WebCore::HTMLFormElement* form, WebFrame* frame);

private:
    /**
     * WebNavigationAction constructor
     * @param[in]: NavigationAction
     * @param[in]: Frame
     */
    WebNavigationAction(const WebCore::NavigationAction*, WebCore::HTMLFormElement* form, WebFrame* frame);

public:

    /**
     * WebNavigationAction destructor
     */
    virtual ~WebNavigationAction();

    const char* url();
    WebNavigationType type();
    WebHitTestResults* webHitTestResults();
    unsigned short button();
    int modifiers();
    DOMNode* form();

private:
    const WebCore::NavigationAction *m_action;
    WebCore::HTMLFormElement* m_form;
    WebFrame* m_frame;
};

#endif
