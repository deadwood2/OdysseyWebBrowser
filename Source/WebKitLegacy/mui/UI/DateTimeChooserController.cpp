/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DateTimeChooserController.h"

#include "WebView.h"
#include "WebChromeClient.h"
#include "DateTimeChooserClient.h"
#include "Color.h"

#include "gui.h"

#undef String

using namespace WebCore;

DateTimeChooserController::DateTimeChooserController(WebChromeClient* chromeClient, DateTimeChooserClient* client, const DateTimeChooserParameters& parameters)
    : m_chromeClient(chromeClient)
    , m_client(client)
    , m_parameters(parameters)
{
}

DateTimeChooserController::~DateTimeChooserController()
{
}

void DateTimeChooserController::openUI()
{
    openDateTimeChooser();
}

void DateTimeChooserController::endChooser()
{
    if (m_chooser)
        m_chooser->endChooser();
}

void DateTimeChooserController::didChooseValue(const String& value)
{
    ASSERT(m_client);
    m_client->didChooseValue(value);
}

void DateTimeChooserController::didEndChooser()
{
    ASSERT(m_client);
    m_chooser = nullptr;
    m_client->didEndChooser();
}

const DateTimeChooserParameters & DateTimeChooserController::parameters()
{
    return m_parameters;
}

void DateTimeChooserController::openDateTimeChooser()
{
    ASSERT(!m_chooser);
    m_chooser = adoptRef(new DateTimeChooser);
    
    BalWidget *widget = m_chromeClient->webView()->viewWindow();

    if(widget)
    {
	//m_parameters
	DoMethod((Object *) widget->browser, MM_OWBBrowser_DateTimeChooser_ShowPopup, this);
    }
}

