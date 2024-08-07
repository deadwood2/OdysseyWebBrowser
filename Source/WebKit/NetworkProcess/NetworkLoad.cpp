/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "NetworkLoad.h"

#include "AuthenticationManager.h"
#include "DownloadProxyMessages.h"
#include "NetworkProcess.h"
#include "NetworkSession.h"
#include "SessionTracker.h"
#include "WebCoreArgumentCoders.h"
#include "WebErrors.h"
#include <WebCore/NotImplemented.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/SharedBuffer.h>
#include <pal/SessionID.h>
#include <wtf/MainThread.h>
#include <wtf/Seconds.h>

#if PLATFORM(COCOA)
#include "NetworkDataTaskCocoa.h"
#endif

#if ENABLE(NETWORK_CAPTURE)
#include "NetworkCaptureManager.h"
#endif

namespace WebKit {

using namespace WebCore;

struct NetworkLoad::Throttle {
    Throttle(NetworkLoad& load, Seconds delay, ResourceResponse&& response, ResponseCompletionHandler&& handler)
        : timer(load, &NetworkLoad::throttleDelayCompleted)
        , response(WTFMove(response))
        , responseCompletionHandler(WTFMove(handler))
    {
        timer.startOneShot(delay);
    }
    Timer timer;
    ResourceResponse response;
    ResponseCompletionHandler responseCompletionHandler;
};

NetworkLoad::NetworkLoad(NetworkLoadClient& client, NetworkLoadParameters&& parameters, NetworkSession& networkSession)
    : m_client(client)
    , m_parameters(WTFMove(parameters))
    , m_currentRequest(m_parameters.request)
{
#if ENABLE(NETWORK_CAPTURE)
    switch (NetworkCapture::Manager::singleton().mode()) {
    case NetworkCapture::Manager::RecordReplayMode::Record:
        initializeForRecord(networkSession);
        break;
    case NetworkCapture::Manager::RecordReplayMode::Replay:
        initializeForReplay(networkSession);
        break;
    case NetworkCapture::Manager::RecordReplayMode::Disabled:
        initialize(networkSession);
        break;
    }
#else
    initialize(networkSession);
#endif
}

#if ENABLE(NETWORK_CAPTURE)
void NetworkLoad::initializeForRecord(NetworkSession& networkSession)
{
    m_recorder = std::make_unique<NetworkCapture::Recorder>();
    m_task = NetworkDataTask::create(networkSession, *this, m_parameters);
    if (!m_parameters.defersLoading) {
        m_task->resume();
        m_recorder->recordRequestSent(m_parameters.request);
    }
}

void NetworkLoad::initializeForReplay(NetworkSession& networkSession)
{
    m_replayer = std::make_unique<NetworkCapture::Replayer>();
    m_task = m_replayer->replayResource(networkSession, *this, m_parameters);
    if (!m_parameters.defersLoading)
        m_task->resume();
}
#endif

void NetworkLoad::initialize(NetworkSession& networkSession)
{
    m_task = NetworkDataTask::create(networkSession, *this, m_parameters);
    if (!m_parameters.defersLoading)
        m_task->resume();
}

NetworkLoad::~NetworkLoad()
{
    ASSERT(RunLoop::isMain());
    if (m_redirectCompletionHandler)
        m_redirectCompletionHandler({ });
    if (m_responseCompletionHandler)
        m_responseCompletionHandler(PolicyAction::Ignore);
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    if (m_challengeCompletionHandler)
        m_challengeCompletionHandler(AuthenticationChallengeDisposition::Cancel, { });
#endif
    if (m_task)
        m_task->clearClient();
}

void NetworkLoad::setDefersLoading(bool defers)
{
    if (m_task) {
        if (defers)
            m_task->suspend();
        else {
            m_task->resume();
#if ENABLE(NETWORK_CAPTURE)
            if (m_recorder)
                m_recorder->recordRequestSent(m_parameters.request);
#endif
        }
    }
}

void NetworkLoad::cancel()
{
    if (m_task)
        m_task->cancel();
}

void NetworkLoad::continueWillSendRequest(WebCore::ResourceRequest&& newRequest)
{
#if PLATFORM(COCOA)
    m_currentRequest.updateFromDelegatePreservingOldProperties(newRequest.nsURLRequest(DoNotUpdateHTTPBody));
#elif USE(SOUP)
    // FIXME: Implement ResourceRequest::updateFromDelegatePreservingOldProperties. See https://bugs.webkit.org/show_bug.cgi?id=126127.
    m_currentRequest.updateFromDelegatePreservingOldProperties(newRequest);
#endif

#if ENABLE(NETWORK_CAPTURE)
    if (m_recorder)
        m_recorder->recordRedirectSent(newRequest);
#endif

    auto redirectCompletionHandler = std::exchange(m_redirectCompletionHandler, nullptr);
    ASSERT(redirectCompletionHandler);
    if (m_currentRequest.isNull()) {
        NetworkLoadMetrics emptyMetrics;
        didCompleteWithError(cancelledError(m_currentRequest), emptyMetrics);
        if (redirectCompletionHandler)
            redirectCompletionHandler({ });
        return;
    }

    if (redirectCompletionHandler)
        redirectCompletionHandler(ResourceRequest(m_currentRequest));
}

void NetworkLoad::continueDidReceiveResponse()
{
    if (m_responseCompletionHandler) {
        auto responseCompletionHandler = std::exchange(m_responseCompletionHandler, nullptr);
        responseCompletionHandler(PolicyAction::Use);
    }
}

bool NetworkLoad::shouldCaptureExtraNetworkLoadMetrics() const
{
    return m_client.get().shouldCaptureExtraNetworkLoadMetrics();
}

NetworkLoadClient::ShouldContinueDidReceiveResponse NetworkLoad::sharedDidReceiveResponse(ResourceResponse&& response)
{
    response.setSource(ResourceResponse::Source::Network);
    if (m_parameters.needsCertificateInfo)
        response.includeCertificateInfo();

    return m_client.get().didReceiveResponse(WTFMove(response));
}

void NetworkLoad::sharedWillSendRedirectedRequest(ResourceRequest&& request, ResourceResponse&& redirectResponse)
{
    // We only expect to get the willSendRequest callback from ResourceHandle as the result of a redirect.
    ASSERT(!redirectResponse.isNull());
    ASSERT(RunLoop::isMain());

#if ENABLE(NETWORK_CAPTURE)
    if (m_recorder)
        m_recorder->recordRedirectReceived(request, redirectResponse);
#endif

    auto oldRequest = WTFMove(m_currentRequest);
    m_currentRequest = request;
    m_client.get().willSendRedirectedRequest(WTFMove(oldRequest), WTFMove(request), WTFMove(redirectResponse));
}

bool NetworkLoad::isAllowedToAskUserForCredentials() const
{
    return m_client.get().isAllowedToAskUserForCredentials();
}

void NetworkLoad::convertTaskToDownload(PendingDownload& pendingDownload, const ResourceRequest& updatedRequest, const ResourceResponse& response)
{
    if (!m_task)
        return;

    m_client = pendingDownload;
    m_currentRequest = updatedRequest;
    m_task->setPendingDownload(pendingDownload);

    if (m_responseCompletionHandler)
        NetworkProcess::singleton().findPendingDownloadLocation(*m_task.get(), std::exchange(m_responseCompletionHandler, nullptr), response);
}

void NetworkLoad::setPendingDownloadID(DownloadID downloadID)
{
    if (!m_task)
        return;

    m_task->setPendingDownloadID(downloadID);
}

void NetworkLoad::setSuggestedFilename(const String& suggestedName)
{
    if (!m_task)
        return;

    m_task->setSuggestedFilename(suggestedName);
}

void NetworkLoad::setPendingDownload(PendingDownload& pendingDownload)
{
    if (!m_task)
        return;

    m_task->setPendingDownload(pendingDownload);
}

void NetworkLoad::willPerformHTTPRedirection(ResourceResponse&& response, ResourceRequest&& request, RedirectCompletionHandler&& completionHandler)
{
    ASSERT(!m_redirectCompletionHandler);
    m_redirectCompletionHandler = WTFMove(completionHandler);
    sharedWillSendRedirectedRequest(WTFMove(request), WTFMove(response));
}

void NetworkLoad::didReceiveChallenge(const AuthenticationChallenge& challenge, ChallengeCompletionHandler&& completionHandler)
{
    m_challenge = challenge;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    m_challengeCompletionHandler = WTFMove(completionHandler);
    m_client.get().canAuthenticateAgainstProtectionSpaceAsync(challenge.protectionSpace());
#else
    completeAuthenticationChallenge(WTFMove(completionHandler));
#endif
}

void NetworkLoad::completeAuthenticationChallenge(ChallengeCompletionHandler&& completionHandler)
{
    bool isServerTrustEvaluation = m_challenge->protectionSpace().authenticationScheme() == ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested;
    if (!isAllowedToAskUserForCredentials() && !isServerTrustEvaluation) {
        completionHandler(AuthenticationChallengeDisposition::UseCredential, { });
        return;
    }

    if (!m_task)
        return;

    if (auto* pendingDownload = m_task->pendingDownload())
        NetworkProcess::singleton().authenticationManager().didReceiveAuthenticationChallenge(*pendingDownload, *m_challenge, WTFMove(completionHandler));
    else
        NetworkProcess::singleton().authenticationManager().didReceiveAuthenticationChallenge(m_parameters.webPageID, m_parameters.webFrameID, *m_challenge, WTFMove(completionHandler));
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
void NetworkLoad::continueCanAuthenticateAgainstProtectionSpace(bool result)
{
    if (!m_challengeCompletionHandler) {
        ASSERT_NOT_REACHED();
        return;
    }

    auto completionHandler = std::exchange(m_challengeCompletionHandler, nullptr);
    if (!result) {
        if (NetworkSession::allowsSpecificHTTPSCertificateForHost(*m_challenge))
            completionHandler(AuthenticationChallengeDisposition::UseCredential, serverTrustCredential(*m_challenge));
        else
            completionHandler(AuthenticationChallengeDisposition::RejectProtectionSpace, { });
        return;
    }

    completeAuthenticationChallenge(WTFMove(completionHandler));
}
#endif

void NetworkLoad::didReceiveResponseNetworkSession(ResourceResponse&& response, ResponseCompletionHandler&& completionHandler)
{
    ASSERT(RunLoop::isMain());
    ASSERT(!m_throttle);

    if (m_task && m_task->isDownload()) {
        NetworkProcess::singleton().findPendingDownloadLocation(*m_task.get(), WTFMove(completionHandler), response);
        return;
    }

    auto delay = NetworkProcess::singleton().loadThrottleLatency();
    if (delay > 0_s) {
        m_throttle = std::make_unique<Throttle>(*this, delay, WTFMove(response), WTFMove(completionHandler));
        return;
    }

    notifyDidReceiveResponse(WTFMove(response), WTFMove(completionHandler));
}

void NetworkLoad::notifyDidReceiveResponse(ResourceResponse&& response, ResponseCompletionHandler&& completionHandler)
{
    ASSERT(RunLoop::isMain());

#if ENABLE(NETWORK_CAPTURE)
    if (m_recorder)
        m_recorder->recordResponseReceived(response);
#endif

    if (sharedDidReceiveResponse(WTFMove(response)) == NetworkLoadClient::ShouldContinueDidReceiveResponse::No) {
        m_responseCompletionHandler = WTFMove(completionHandler);
        return;
    }
    completionHandler(PolicyAction::Use);
}

void NetworkLoad::didReceiveData(Ref<SharedBuffer>&& buffer)
{
    ASSERT(!m_throttle);

#if ENABLE(NETWORK_CAPTURE)
    if (m_recorder)
        m_recorder->recordDataReceived(buffer.get());
#endif

    // FIXME: This should be the encoded data length, not the decoded data length.
    auto size = buffer->size();
    m_client.get().didReceiveBuffer(WTFMove(buffer), size);
}

void NetworkLoad::didCompleteWithError(const ResourceError& error, const WebCore::NetworkLoadMetrics& networkLoadMetrics)
{
    ASSERT(!m_throttle);

#if ENABLE(NETWORK_CAPTURE)
    if (m_recorder)
        m_recorder->recordFinish(error);
#endif

    if (error.isNull())
        m_client.get().didFinishLoading(networkLoadMetrics);
    else
        m_client.get().didFailLoading(error);
}

void NetworkLoad::throttleDelayCompleted()
{
    ASSERT(m_throttle);

    auto throttle = WTFMove(m_throttle);

    notifyDidReceiveResponse(WTFMove(throttle->response), WTFMove(throttle->responseCompletionHandler));
}

void NetworkLoad::didSendData(uint64_t totalBytesSent, uint64_t totalBytesExpectedToSend)
{
    m_client.get().didSendData(totalBytesSent, totalBytesExpectedToSend);
}

void NetworkLoad::wasBlocked()
{
    m_client.get().didFailLoading(blockedError(m_currentRequest));
}

void NetworkLoad::cannotShowURL()
{
    m_client.get().didFailLoading(cannotShowURLError(m_currentRequest));
}

} // namespace WebKit
