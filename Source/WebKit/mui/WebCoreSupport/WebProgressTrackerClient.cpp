/*
 * Copyright (C) Krzysztof Smiechowicz.  All rights reserved.
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
#include "WebProgressTrackerClient.h"

#include "WebFrame.h"
#include "WebView.h"
#include "WebNotificationDelegate.h"

WebProgressTrackerClient::WebProgressTrackerClient()
{
}

WebProgressTrackerClient::~WebProgressTrackerClient()
{
}

void WebProgressTrackerClient::progressTrackerDestroyed()
{
    delete this;
}

void WebProgressTrackerClient::progressStarted(WebCore::Frame&)
{
#ifdef BENCH_LOAD_TIME
    gettimeofday(&m_timerStart, NULL);
#endif

    m_webFrame->webView()->stopLoading(false);

    SharedPtr<WebNotificationDelegate> webNotificationDelegate = m_webFrame->webView()->webNotificationDelegate();
    if (webNotificationDelegate)
        webNotificationDelegate->startLoadNotification(m_webFrame);
}

void WebProgressTrackerClient::progressEstimateChanged(WebCore::Frame&)
{
    SharedPtr<WebNotificationDelegate> webNotificationDelegate = m_webFrame->webView()->webNotificationDelegate();
    if (webNotificationDelegate)
        webNotificationDelegate->progressNotification(m_webFrame);
}

void WebProgressTrackerClient::progressFinished(WebCore::Frame&)
{
    SharedPtr<WebNotificationDelegate> webNotificationDelegate = m_webFrame->webView()->webNotificationDelegate();
    if (webNotificationDelegate)
        webNotificationDelegate->finishedLoadNotification(m_webFrame);

#ifdef BENCH_LOAD_TIME
    gettimeofday(&m_timerStop, NULL);
    if (m_timerStart.tv_sec == m_timerStop.tv_sec)
        printf("load time: %06d us\n", static_cast<uint32_t> (m_timerStop.tv_usec - m_timerStart.tv_usec));
    else {
        int seconds = m_timerStop.tv_sec - m_timerStart.tv_sec;
        int microseconds = m_timerStop.tv_usec - m_timerStart.tv_usec;
        if (microseconds < 0) {
            seconds -= 1;
            microseconds = 1000000 + microseconds;
        }
        printf("load time: %d s %06d us\n", seconds, microseconds);
    }
    // This is meant to help script watching OWB activity so that they can kill it and
    // dump the statistics.
    fflush(stdout);
#endif
}
