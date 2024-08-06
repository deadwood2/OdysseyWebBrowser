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
#ifndef WebWindowConfirm_H
#define WebWindowConfirm_H

#include "WebWindow.h"
#include <stdint.h>
#include <string>

namespace WebCore {
    class Font;
    class TextRun;
}

class WebView;

class WEBKIT_OWB_API WebWindowConfirm : public WebWindow
{
public:
    static WebWindowConfirm* createWebWindowConfirm(bool modal, const char* text, WebView *);
    virtual ~WebWindowConfirm();
    virtual bool onKeyDown(BalEventKey event);
    virtual bool onMouseMotion(BalEventMotion event);
    virtual bool onMouseButtonDown(BalEventButton event);
    virtual bool onQuit(BalQuitEvent);

    bool value();
protected:
    virtual void paint(BalRectangle rect);
private:
    WebWindowConfirm(bool modal, const char* text, WebView *);
    void drawButton();
    void updateButton();
    void getLineBreak(WebCore::Font& font, WebCore::TextRun& text, uint16_t maxLength, uint8_t* wordBreak, uint16_t* wordLength);

    std::string m_text;
    std::string m_buttonOk;
    std::string m_buttonCancel;
    bool m_button;
    bool m_stateLeft;
    bool m_stateRigth;
    WebView *m_view;
    bool isPainted;
    bool m_value;
    bool m_confirm;
    bool isThemable;
    BalRectangle m_buttonOKRect;
    BalRectangle m_buttonCancelRect;
};

#endif


