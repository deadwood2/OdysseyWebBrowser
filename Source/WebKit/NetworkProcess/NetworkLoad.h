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

#pragma once

#include "DownloadID.h"
#include "NetworkDataTask.h"
#include "NetworkLoadClient.h"
#include "NetworkLoadParameters.h"
#include <WebCore/AuthenticationChallenge.h>
#include <wtf/CompletionHandler.h>
#include <wtf/Optional.h>

#if ENABLE(NETWORK_CAPTURE)
#include "NetworkCaptureRecorder.h"
#include "NetworkCaptureReplayer.h"
#endif

namespace WebKit {

class NetworkLoad final : private NetworkDataTaskClient
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    NetworkLoad(NetworkLoadClient&, NetworkLoadParameters&&, NetworkSession&);
    ~NetworkLoad();

    void setDefersLoading(bool);
    void cancel();

    bool isAllowedToAskUserForCredentials() const;

    const WebCore::ResourceRequest& currentRequest() const { return m_currentRequest; }
    void clearCurrentRequest() { m_currentRequest = WebCore::ResourceRequest(); }

    const NetworkLoadParameters& parameters() const { return m_parameters; }

    void continueWillSendRequest(WebCore::ResourceRequest&&);
    void continueDidReceiveResponse();

    void convertTaskToDownload(PendingDownload&, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);
    void setPendingDownloadID(DownloadID);
    void setSuggestedFilename(const String&);
    void setPendingDownload(PendingDownload&);
    DownloadID pendingDownloadID() { return m_task->pendingDownloadID(); }

    bool shouldCaptureExtraNetworkLoadMetrics() const final;

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    void continueCanAuthenticateAgainstProtectionSpace(bool);
#endif

private:
#if ENABLE(NETWORK_CAPTURE)
    void initializeForRecord(NetworkSession&);
    void initializeForReplay(NetworkSession&);
#endif
    void initialize(NetworkSession&);

    NetworkLoadClient::ShouldContinueDidReceiveResponse sharedDidReceiveResponse(WebCore::ResourceResponse&&);
    void sharedWillSendRedirectedRequest(WebCore::ResourceRequest&&, WebCore::ResourceResponse&&);

    // NetworkDataTaskClient
    void willPerformHTTPRedirection(WebCore::ResourceResponse&&, WebCore::ResourceRequest&&, RedirectCompletionHandler&&) final;
    void didReceiveChallenge(const WebCore::AuthenticationChallenge&, ChallengeCompletionHandler&&) final;
    void didReceiveResponseNetworkSession(WebCore::ResourceResponse&&, ResponseCompletionHandler&&) final;
    void didReceiveData(Ref<WebCore::SharedBuffer>&&) final;
    void didCompleteWithError(const WebCore::ResourceError&, const WebCore::NetworkLoadMetrics&) final;
    void didSendData(uint64_t totalBytesSent, uint64_t totalBytesExpectedToSend) final;
    void wasBlocked() final;
    void cannotShowURL() final;

    void notifyDidReceiveResponse(WebCore::ResourceResponse&&, ResponseCompletionHandler&&);
    void throttleDelayCompleted();

    void completeAuthenticationChallenge(ChallengeCompletionHandler&&);

    std::reference_wrapper<NetworkLoadClient> m_client;
    const NetworkLoadParameters m_parameters;
    CompletionHandler<void(WebCore::ResourceRequest&&)> m_redirectCompletionHandler;
    RefPtr<NetworkDataTask> m_task;
    std::optional<WebCore::AuthenticationChallenge> m_challenge;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    ChallengeCompletionHandler m_challengeCompletionHandler;
#endif
    ResponseCompletionHandler m_responseCompletionHandler;
    
    struct Throttle;
    std::unique_ptr<Throttle> m_throttle;

    WebCore::ResourceRequest m_currentRequest; // Updated on redirects.

#if ENABLE(NETWORK_CAPTURE)
    std::unique_ptr<NetworkCapture::Recorder> m_recorder;
    std::unique_ptr<NetworkCapture::Replayer> m_replayer;
#endif
};

} // namespace WebKit
