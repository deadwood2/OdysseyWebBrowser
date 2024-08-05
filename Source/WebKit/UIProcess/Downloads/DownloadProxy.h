/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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

#ifndef DownloadProxy_h
#define DownloadProxy_h

#include "APIObject.h"
#include "Connection.h"
#include "DownloadID.h"
#include "SandboxExtension.h"
#include <WebCore/ResourceRequest.h>
#include <wtf/Forward.h>
#include <wtf/Ref.h>

namespace API {
class Data;
}

namespace WebCore {
class AuthenticationChallenge;
class ProtectionSpace;
class ResourceError;
class ResourceResponse;
}

namespace WebKit {

class DownloadID;
class DownloadProxyMap;
class WebPageProxy;
class WebProcessPool;

class DownloadProxy : public API::ObjectImpl<API::Object::Type::Download>, public IPC::MessageReceiver {
public:
    static Ref<DownloadProxy> create(DownloadProxyMap&, WebProcessPool&, const WebCore::ResourceRequest&);
    ~DownloadProxy();

    DownloadID downloadID() const { return m_downloadID; }
    const WebCore::ResourceRequest& request() const { return m_request; }
    API::Data* resumeData() const { return m_resumeData.get(); }

    void cancel();

    void invalidate();
    void processDidClose();

    void didReceiveDownloadProxyMessage(IPC::Connection&, IPC::Decoder&);
    void didReceiveSyncDownloadProxyMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);

private:
    explicit DownloadProxy(DownloadProxyMap&, WebProcessPool&, const WebCore::ResourceRequest&);

    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;

    // Message handlers.
    void didStart(const WebCore::ResourceRequest&, const String& suggestedFilename);
    void didReceiveAuthenticationChallenge(const WebCore::AuthenticationChallenge&, uint64_t challengeID);
    void didReceiveResponse(const WebCore::ResourceResponse&);
    void didReceiveData(uint64_t length);
    void shouldDecodeSourceDataOfMIMEType(const String& mimeType, bool& result);
    void didCreateDestination(const String& path);
    void didFinish();
    void didFail(const WebCore::ResourceError&, const IPC::DataReference& resumeData);
    void didCancel(const IPC::DataReference& resumeData);
#if USE(NETWORK_SESSION)
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    void canAuthenticateAgainstProtectionSpace(const WebCore::ProtectionSpace&);
#endif
    void willSendRequest(const WebCore::ResourceRequest& redirectRequest, const WebCore::ResourceResponse& redirectResponse);
#else
    void decideDestinationWithSuggestedFilename(const String& filename, const String& mimeType, String& destination, bool& allowOverwrite, SandboxExtension::Handle& sandboxExtensionHandle);
#endif
    void decideDestinationWithSuggestedFilenameAsync(DownloadID, const String& suggestedFilename);

    DownloadProxyMap& m_downloadProxyMap;
    RefPtr<WebProcessPool> m_processPool;
    DownloadID m_downloadID;

    RefPtr<API::Data> m_resumeData;
    WebCore::ResourceRequest m_request;
    String m_suggestedFilename;
};

} // namespace WebKit

#endif // DownloadProxy_h
