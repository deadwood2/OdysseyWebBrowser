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

#include "config.h"
#include "WebNavigationAction.h"
#include "WebFrame.h"
#include "WebHitTestResults.h"

#include <wtf/text/CString.h>
#include "DOMCoreClasses.h"
#include "EventHandler.h"
#include "Frame.h"
#include "HitTestResult.h"
#include "HTMLFormElement.h"
#include <MouseEvent.h>
#include <NavigationAction.h>
#include <UIEventWithKeyState.h>

using namespace WebCore;

WebNavigationAction::WebNavigationAction(const WebCore::NavigationAction* action, HTMLFormElement* form, WebFrame* frame)
    : m_action(action)
    , m_form(form)
    , m_frame(frame)
{
}

WebNavigationAction::~WebNavigationAction()
{
}

WebNavigationAction* WebNavigationAction::createInstance(const WebCore::NavigationAction* action, HTMLFormElement* form, WebFrame* frame)
{
    WebNavigationAction* instance = new WebNavigationAction(action, form, frame); 
    return instance;
}

WebNavigationAction* WebNavigationAction::createInstance()
{
     return new WebNavigationAction(new WebCore::NavigationAction(), 0, 0);
}


const char* WebNavigationAction::url() 
{
    return strdup(m_action->url().string().utf8().data());
}

WebNavigationType WebNavigationAction::type() 
{ 
    return static_cast<WebNavigationType>(m_action->type()); 
}

static const MouseEvent* findMouseEvent(const Event* event)
{
    for (const Event* e = event; e; e = e->underlyingEvent())
        if (e->isMouseEvent())
            return static_cast<const MouseEvent*>(e);
    return 0;
}


WebHitTestResults* WebNavigationAction::webHitTestResults()
{
    if (const MouseEvent* mouseEvent = findMouseEvent(m_action->event()))
        return WebHitTestResults::createInstance(core(m_frame)->eventHandler().hitTestResultAtPoint(mouseEvent->absoluteLocation(), false));
    return 0;
}

unsigned short WebNavigationAction::button()
{
    if (const MouseEvent* mouseEvent = findMouseEvent(m_action->event()))
        return mouseEvent->button();
    return 0;
}

int WebNavigationAction::modifiers()
{
    if (const UIEventWithKeyState* keyEvent = findEventWithKeyState(const_cast<Event*>(m_action->event()))) {
        int mod = 0;

        if (keyEvent->ctrlKey())
            mod |= WEBCONTROL;
        if (keyEvent->shiftKey())
            mod |= WEBSHIFT;
        if (keyEvent->altKey())
            mod |= WEBALT;
        return mod;
    }
    return 0;
}

DOMNode* WebNavigationAction::form()
{
    if (m_form)
        return DOMNode::createInstance(m_form);
    return 0;
}

