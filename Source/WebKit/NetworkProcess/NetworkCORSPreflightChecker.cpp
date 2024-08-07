/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NetworkCORSPreflightChecker.h"

#include "AuthenticationManager.h"
#include "Logging.h"
#include "NetworkLoadParameters.h"
#include "SessionTracker.h"
#include <WebCore/CrossOriginAccessControl.h>
#include <WebCore/SecurityOrigin.h>

#define RELEASE_LOG_IF_ALLOWED(fmt, ...) RELEASE_LOG_IF(m_parameters.sessionID.isAlwaysOnLoggingAllowed(), Network, "%p - NetworkCORSPreflightChecker::" fmt, this, ##__VA_ARGS__)

namespace WebKit {

using namespace WebCore;

NetworkCORSPreflightChecker::NetworkCORSPreflightChecker(Parameters&& parameters, CompletionCallback&& completionCallback)
    : m_parameters(WTFMove(parameters))
    , m_completionCallback(WTFMove(completionCallback))
{
}

NetworkCORSPreflightChecker::~NetworkCORSPreflightChecker()
{
    if (m_task) {
        ASSERT(m_task->client() == this);
        m_task->clearClient();
        m_task->cancel();
    }
}

void NetworkCORSPreflightChecker::startPreflight()
{
    RELEASE_LOG_IF_ALLOWED("startPreflight");

    NetworkLoadParameters loadParameters;
    loadParameters.sessionID = m_parameters.sessionID;
    loadParameters.request = createAccessControlPreflightRequest(m_parameters.originalRequest, m_parameters.sourceOrigin, m_parameters.originalRequest.httpReferrer());
    loadParameters.shouldFollowRedirects = false;
    if (auto* networkSession = SessionTracker::networkSession(loadParameters.sessionID)) {
        m_task = NetworkDataTask::create(*networkSession, *this, WTFMove(loadParameters));
        m_task->resume();
    } else
        ASSERT_NOT_REACHED();
}

void NetworkCORSPreflightChecker::willPerformHTTPRedirection(WebCore::ResourceResponse&&, WebCore::ResourceRequest&&, RedirectCompletionHandler&& completionHandler)
{
    RELEASE_LOG_IF_ALLOWED("willPerformHTTPRedirection");
    completionHandler({ });
    m_completionCallback(Result::Failure);
}

void NetworkCORSPreflightChecker::didReceiveChallenge(const WebCore::AuthenticationChallenge&, ChallengeCompletionHandler&& completionHandler)
{
    RELEASE_LOG_IF_ALLOWED("didReceiveChallenge");
    completionHandler(AuthenticationChallengeDisposition::Cancel, { });
    m_completionCallback(Result::Failure);
}

void NetworkCORSPreflightChecker::didReceiveResponseNetworkSession(WebCore::ResourceResponse&& response, ResponseCompletionHandler&& completionHandler)
{
    RELEASE_LOG_IF_ALLOWED("didReceiveResponseNetworkSession");
    m_response = WTFMove(response);
    completionHandler(PolicyAction::Use);
}

void NetworkCORSPreflightChecker::didReceiveData(Ref<WebCore::SharedBuffer>&&)
{
    RELEASE_LOG_IF_ALLOWED("didReceiveData");
}

void NetworkCORSPreflightChecker::didCompleteWithError(const WebCore::ResourceError& error, const WebCore::NetworkLoadMetrics&)
{
    if (!error.isNull()) {
        RELEASE_LOG_IF_ALLOWED("didCompleteWithError");
        m_completionCallback(Result::Failure);
        return;
    }

    RELEASE_LOG_IF_ALLOWED("didComplete http_status_code: %d", m_response.httpStatusCode());

    String errorDescription;
    if (!validatePreflightResponse(m_parameters.originalRequest, m_response, m_parameters.storedCredentialsPolicy, m_parameters.sourceOrigin, errorDescription)) {
        RELEASE_LOG_IF_ALLOWED("didComplete, AccessControl error: %s", errorDescription.utf8().data());
        m_completionCallback(Result::Failure);
        return;
    }
    m_completionCallback(Result::Success);
}

void NetworkCORSPreflightChecker::didSendData(uint64_t totalBytesSent, uint64_t totalBytesExpectedToSend)
{
}

void NetworkCORSPreflightChecker::wasBlocked()
{
    RELEASE_LOG_IF_ALLOWED("wasBlocked");
    m_completionCallback(Result::Failure);
}

void NetworkCORSPreflightChecker::cannotShowURL()
{
    RELEASE_LOG_IF_ALLOWED("cannotShowURL");
    m_completionCallback(Result::Failure);
}

} // Namespace WebKit
