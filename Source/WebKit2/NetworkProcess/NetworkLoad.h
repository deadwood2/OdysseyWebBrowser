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

#ifndef NetworkLoad_h
#define NetworkLoad_h

#include "NetworkLoadClient.h"
#include "NetworkLoadParameters.h"
#include "RemoteNetworkingContext.h"
#include <WebCore/ResourceHandleClient.h>

#if USE(NETWORK_SESSION)
#include "DownloadID.h"
#include "NetworkSession.h"
#include <WebCore/AuthenticationChallenge.h>
#endif

namespace WebKit {

class NetworkLoad : public WebCore::ResourceHandleClient
#if USE(NETWORK_SESSION)
    , public NetworkDataTaskClient
#endif
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    NetworkLoad(NetworkLoadClient&, const NetworkLoadParameters&);
    ~NetworkLoad();

    void setDefersLoading(bool);
    void cancel();

    const WebCore::ResourceRequest& currentRequest() const { return m_currentRequest; }
    void clearCurrentRequest() { m_currentRequest = WebCore::ResourceRequest(); }

    void continueWillSendRequest(const WebCore::ResourceRequest&);
    void continueDidReceiveResponse();

#if USE(NETWORK_SESSION)
    void convertTaskToDownload(DownloadID);
    void setPendingDownloadID(DownloadID);
    void setPendingDownload(PendingDownload&);
    DownloadID pendingDownloadID() { return m_task->pendingDownloadID(); }
    
    // NetworkDataTaskClient
    virtual void willPerformHTTPRedirection(const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, RedirectCompletionHandler) final override;
    virtual void didReceiveChallenge(const WebCore::AuthenticationChallenge&, ChallengeCompletionHandler) final override;
    virtual void didReceiveResponseNetworkSession(const WebCore::ResourceResponse&, ResponseCompletionHandler) final override;
    virtual void didReceiveData(RefPtr<WebCore::SharedBuffer>&&) final override;
    virtual void didCompleteWithError(const WebCore::ResourceError&) final override;
    virtual void didBecomeDownload() final override;
    virtual void didSendData(uint64_t totalBytesSent, uint64_t totalBytesExpectedToSend) override;
    virtual void wasBlocked() override;
    virtual void cannotShowURL() override;
#endif
    // ResourceHandleClient
    virtual void willSendRequestAsync(WebCore::ResourceHandle*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse) override;
    virtual void didSendData(WebCore::ResourceHandle*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent) override;
    virtual void didReceiveResponseAsync(WebCore::ResourceHandle*, const WebCore::ResourceResponse&) override;
    virtual void didReceiveData(WebCore::ResourceHandle*, const char*, unsigned, int encodedDataLength) override;
    virtual void didReceiveBuffer(WebCore::ResourceHandle*, PassRefPtr<WebCore::SharedBuffer>, int encodedDataLength) override;
    virtual void didFinishLoading(WebCore::ResourceHandle*, double finishTime) override;
    virtual void didFail(WebCore::ResourceHandle*, const WebCore::ResourceError&) override;
    virtual void wasBlocked(WebCore::ResourceHandle*) override;
    virtual void cannotShowURL(WebCore::ResourceHandle*) override;
    virtual bool shouldUseCredentialStorage(WebCore::ResourceHandle*) override;
    virtual void didReceiveAuthenticationChallenge(WebCore::ResourceHandle*, const WebCore::AuthenticationChallenge&) override;
    virtual void didCancelAuthenticationChallenge(WebCore::ResourceHandle*, const WebCore::AuthenticationChallenge&) override;
    virtual void receivedCancellation(WebCore::ResourceHandle*, const WebCore::AuthenticationChallenge&) override;
    virtual bool usesAsyncCallbacks() override { return true; }
    virtual bool loadingSynchronousXHR() override { return m_client.isSynchronous(); }

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual void canAuthenticateAgainstProtectionSpaceAsync(WebCore::ResourceHandle*, const WebCore::ProtectionSpace&) override;
#endif
#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    virtual bool supportsDataArray() override;
    virtual void didReceiveDataArray(WebCore::ResourceHandle*, CFArrayRef) override;
#endif
#if PLATFORM(COCOA)
#if USE(CFNETWORK)
    virtual void willCacheResponseAsync(WebCore::ResourceHandle*, CFCachedURLResponseRef) override;
#else
    virtual void willCacheResponseAsync(WebCore::ResourceHandle*, NSCachedURLResponse *) override;
#endif
#endif

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    void continueCanAuthenticateAgainstProtectionSpace(bool);
#endif

#if !USE(NETWORK_SESSION)
    WebCore::ResourceHandle* handle() const { return m_handle.get(); }
#endif

private:
    NetworkLoadClient::ShouldContinueDidReceiveResponse sharedDidReceiveResponse(const WebCore::ResourceResponse&);
    void sharedWillSendRedirectedRequest(const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);

    NetworkLoadClient& m_client;
    const NetworkLoadParameters m_parameters;
#if USE(NETWORK_SESSION)
    RefPtr<NetworkDataTask> m_task;
    WebCore::AuthenticationChallenge m_challenge;
    ChallengeCompletionHandler m_challengeCompletionHandler;
    ResponseCompletionHandler m_responseCompletionHandler;
    RedirectCompletionHandler m_redirectCompletionHandler;
#else
    RefPtr<RemoteNetworkingContext> m_networkingContext;
#endif
    RefPtr<WebCore::ResourceHandle> m_handle;

    WebCore::ResourceRequest m_currentRequest; // Updated on redirects.
};

} // namespace WebKit

#endif // NetworkLoad_h
