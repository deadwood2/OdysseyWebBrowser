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

#include "config.h"

#include "WebFramePolicyListener.h"

#include "WebFrame.h"
#include "WebFrameLoaderClient.h"

#include <Frame.h>
#include <FrameLoader.h>
#include <FrameLoaderClient.h>

using namespace WebCore;


WebFramePolicyListener::WebFramePolicyListener(WebFrame* frame)
    : m_frame(frame)
{
}

WebFramePolicyListener::~WebFramePolicyListener()
{
}

WebFramePolicyListener* WebFramePolicyListener::createInstance(WebFrame* frame)
{
    WebFramePolicyListener* instance = new WebFramePolicyListener(frame);
    return instance;
}


void WebFramePolicyListener::use(void)
{
    receivedPolicyDecision(WebPolicyUse);
}

void WebFramePolicyListener::download(void)
{
    receivedPolicyDecision(WebPolicyDownload);
}

void WebFramePolicyListener::ignore(void)
{
    receivedPolicyDecision(WebPolicyIgnore);
}

void WebFramePolicyListener::continueSubmit(void)
{
    receivedPolicyDecision(WebPolicyUse);
}

void WebFramePolicyListener::receivedPolicyDecision(WebPolicyAction action)
{
    Frame* coreFrame = core(m_frame);
    if (coreFrame)
        static_cast<WebFrameLoaderClient&>(coreFrame->loader().client()).receivedPolicyDecision(static_cast<PolicyAction>(action));
}

void WebFramePolicyListener::invalidate()
{
    m_frame = 0;
}

