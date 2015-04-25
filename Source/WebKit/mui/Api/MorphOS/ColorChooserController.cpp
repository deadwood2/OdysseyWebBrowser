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
#include "ColorChooserController.h"

#include "WebView.h"
#include "WebChromeClient.h"
#include "ColorChooserClient.h"
#include "Color.h"

#include "gui.h"

using namespace WebCore;

ColorChooserController::ColorChooserController(WebChromeClient* chromeClient, ColorChooserClient* client)
    : m_chromeClient(chromeClient)
    , m_client(client)
{
}

ColorChooserController::~ColorChooserController()
{
}

void ColorChooserController::openUI()
{
    openColorChooser();
}

void ColorChooserController::setSelectedColor(const Color& color)
{
    ASSERT(m_chooser);
	m_chooser->setSelectedColor(color);
}

void ColorChooserController::endChooser()
{
    if (m_chooser)
        m_chooser->endChooser();
}

void ColorChooserController::didChooseColor(const Color& color)
{
    ASSERT(m_client);
	m_client->didChooseColor(color);
}

void ColorChooserController::didEndChooser()
{
    ASSERT(m_client);
	m_chooser.reset();
    m_client->didEndChooser();
}

IntRect ColorChooserController::elementRectRelativeToRootView() const
{
    ASSERT(m_client);
	return m_client->elementRectRelativeToRootView();
}

void ColorChooserController::openColorChooser()
{
    ASSERT(!m_chooser);
	m_chooser = std::make_unique<ColorChooser>();

	BalWidget *widget = m_chromeClient->webView()->viewWindow();

	if(widget)
	{
		Color color = m_client->currentColor();
		DoMethod((Object *) widget->browser, MM_OWBBrowser_ColorChooser_ShowPopup, this, &color);
	}
}

