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

#ifndef WebHitTestResults_H
#define WebHitTestResults_H


/**
 *  @file  WebHitTestResults.h
 *  WebHitTestResults description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */
#include "WebKitTypes.h"

namespace WebCore {
    class HitTestResult;
}

class DOMNode;
class WebFrame;

class WEBKIT_OWB_API WebHitTestResults
{
protected:
    friend class WebView;
    friend class WebNavigationAction;
    
    /**
     * create a new instance of WebHitTestResults
     * @param[in]: HitTestResult
     */
    static WebHitTestResults* createInstance(const WebCore::HitTestResult&);

private:

    /**
     * WebHitTestResults constructor
     * @param[in]: HitTestResult
     */
    WebHitTestResults(const WebCore::HitTestResult&);

public:

    /**
     * WebHitTestResults destructor
     */
    virtual ~WebHitTestResults();

    DOMNode* domNode();
    WebFrame* frame();
    const char* imageAltString();
    //Image* image();
//    BalRectangle imageRect();
    const char* imageURL();
    bool isSelected();
    const char* spellingToolTip();
    const char* title();
    const char* absoluteLinkURL();
    WebFrame* targetFrame();
    const char* titleDisplayString();
    const char* textContent();
    bool isContentEditable();

private:
    WebCore::HitTestResult* m_result;
};

#endif // WebHitTestResults_H
