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

#pragma once

#if PLATFORM(COCOA) && ENABLE(MEDIA_STREAM)

#include "Connection.h"
#include "MessageReceiver.h"
#include "UserMediaCaptureManager.h"
#include <WebCore/OrientationNotifier.h>
#include <WebCore/RealtimeMediaSource.h>
#include <WebCore/RealtimeMediaSourceIdentifier.h>
#include <wtf/UniqueRef.h>

namespace WebKit {

class SharedMemory;
class WebProcessProxy;

class UserMediaCaptureManagerProxy : private IPC::MessageReceiver {
    WTF_MAKE_FAST_ALLOCATED;
public:
    class ConnectionProxy {
    public:
        virtual ~ConnectionProxy() = default;
        virtual void addMessageReceiver(IPC::StringReference, IPC::MessageReceiver&) = 0;
        virtual void removeMessageReceiver(IPC::StringReference) = 0;
        virtual IPC::Connection& connection() = 0;
    };
    explicit UserMediaCaptureManagerProxy(UniqueRef<ConnectionProxy>&&);
    ~UserMediaCaptureManagerProxy();

    void clear();

    void setOrientation(uint64_t);

    void didReceiveMessageFromGPUProcess(IPC::Connection& connection, IPC::Decoder& decoder) { didReceiveMessage(connection, decoder); }
    void didReceiveSyncMessageFromGPUProcess(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& encoder) { didReceiveSyncMessage(connection, decoder, encoder); }

private:
    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) final;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) final;

    void createMediaSourceForCaptureDeviceWithConstraints(WebCore::RealtimeMediaSourceIdentifier, const WebCore::CaptureDevice& deviceID, String&&, const WebCore::MediaConstraints&, CompletionHandler<void(bool succeeded, String invalidConstraints, WebCore::RealtimeMediaSourceSettings&&)>&&);
    void startProducingData(WebCore::RealtimeMediaSourceIdentifier);
    void stopProducingData(WebCore::RealtimeMediaSourceIdentifier);
    void end(WebCore::RealtimeMediaSourceIdentifier);
    void capabilities(WebCore::RealtimeMediaSourceIdentifier, CompletionHandler<void(WebCore::RealtimeMediaSourceCapabilities&&)>&&);
    void setMuted(WebCore::RealtimeMediaSourceIdentifier, bool);
    void applyConstraints(WebCore::RealtimeMediaSourceIdentifier, const WebCore::MediaConstraints&);
    void clone(WebCore::RealtimeMediaSourceIdentifier clonedID, WebCore::RealtimeMediaSourceIdentifier cloneID);
    void requestToEnd(WebCore::RealtimeMediaSourceIdentifier);

    class SourceProxy;
    friend class SourceProxy;
    HashMap<WebCore::RealtimeMediaSourceIdentifier, std::unique_ptr<SourceProxy>> m_proxies;
    UniqueRef<ConnectionProxy> m_connectionProxy;
    WebCore::OrientationNotifier m_orientationNotifier { 0 };
};

}

#endif
