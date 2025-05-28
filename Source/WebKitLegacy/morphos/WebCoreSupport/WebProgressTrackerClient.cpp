/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebKit.h"
#include "WebProgressTrackerClient.h"

#include "WebPage.h"
#include <WebCore/Frame.h>
#include <WebCore/Page.h>
#include <WebCore/ProgressTracker.h>

namespace WebKit {
using namespace WebCore;

WebProgressTrackerClient::WebProgressTrackerClient(WebPage& webPage)
    : m_webPage(webPage)
{
}
    
void WebProgressTrackerClient::progressStarted(Frame& originatingProgressFrame)
{
    if (!originatingProgressFrame.isMainFrame())
        return;
	if (m_webPage._fProgressStarted)
		m_webPage._fProgressStarted();
}

void WebProgressTrackerClient::progressEstimateChanged(Frame& originatingProgressFrame)
{
    if (!originatingProgressFrame.isMainFrame())
        return;
    
    float progress = m_webPage.corePage()->progress().estimatedProgress();
	if (m_webPage._fProgressUpdated)
		m_webPage._fProgressUpdated(progress);
}

void WebProgressTrackerClient::progressFinished(Frame& originatingProgressFrame)
{
    if (!originatingProgressFrame.isMainFrame())
        return;
	if (m_webPage._fProgressFinished)
		m_webPage._fProgressFinished();
}

} // namespace WebKit
