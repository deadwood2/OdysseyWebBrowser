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
#include "WebHitTestResults.h"

#include "WebFrame.h"
#include "DOMCoreClasses.h"
#include "WebFrameLoaderClient.h"

#include <wtf/text/CString.h>
#include <Document.h>
#include <Frame.h>
#include <HitTestResult.h>
#include <FrameLoader.h>
#include <Image.h>

using namespace WebCore;

WebHitTestResults::WebHitTestResults(const HitTestResult& result)
    : m_result(new HitTestResult(result))
{
}

WebHitTestResults::~WebHitTestResults()
{
}

WebHitTestResults* WebHitTestResults::createInstance(const HitTestResult& result)
{
    WebHitTestResults* instance = new WebHitTestResults(result); 

    return instance;
}

DOMNode* WebHitTestResults::domNode()
{
    return DOMNode::createInstance(m_result->innerNonSharedNode());
}

WebFrame* WebHitTestResults::frame()
{
  if (!(m_result->innerNonSharedNode() && m_result->innerNonSharedNode()->document().frame()))
        return 0;
    Frame* coreFrame = m_result->innerNonSharedNode()->document().frame();
    return static_cast<WebFrameLoaderClient&>(coreFrame->loader().client()).webFrame();
}

const char* WebHitTestResults::imageAltString()
{
    return strdup(m_result->altDisplayString().utf8().data());
}

/*Image* WebHitTestResults::image()
{
    return m_result->image();
}*/

/*BalRectangle WebHitTestResults::imageRect()
{
    return m_result->boundingBox();
}*/

const char* WebHitTestResults::imageURL()
{
    return strdup(m_result->absoluteImageURL().string().utf8().data());
}

bool WebHitTestResults::isSelected()
{
    return m_result->isSelected();
}

const char* WebHitTestResults::spellingToolTip()
{
    TextDirection dir;
    return strdup(m_result->spellingToolTip(dir).utf8().data());
}

const char* WebHitTestResults::title()
{
    TextDirection dir;
    return strdup(m_result->title(dir).utf8().data());
}

const char* WebHitTestResults::absoluteLinkURL()
{
    return strdup(m_result->absoluteLinkURL().string().utf8().data());
}

WebFrame* WebHitTestResults::targetFrame()
{
    if (!m_result->targetFrame())
        return 0;
    return kit(m_result->targetFrame());
}

const char* WebHitTestResults::titleDisplayString()
{
    return strdup(m_result->titleDisplayString().utf8().data());
}

const char* WebHitTestResults::textContent()
{
    return strdup(m_result->textContent().utf8().data());
}

bool WebHitTestResults::isContentEditable()
{
    return m_result->isContentEditable();
}

