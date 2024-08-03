/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2015 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RTCPeerConnection_h
#define RTCPeerConnection_h

#if ENABLE(WEB_RTC)

#include "ActiveDOMObject.h"
#include "Dictionary.h"
#include "EventTarget.h"
// FIXME: Workaround for bindings bug http://webkit.org/b/150121
#include "JSMediaStream.h"
#include "PeerConnectionBackend.h"
#include "RTCRtpTransceiver.h"
#include "ScriptWrappable.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaStream;
class MediaStreamTrack;
class PeerConnectionBackend;
class RTCConfiguration;
class RTCDataChannel;
class RTCIceCandidate;
class RTCPeerConnectionErrorCallback;
class RTCSessionDescription;
class RTCStatsCallback;

class RTCPeerConnection final : public RefCounted<RTCPeerConnection>, public PeerConnectionBackendClient, public RTCRtpSenderClient, public EventTargetWithInlineData, public ActiveDOMObject {
public:
    static Ref<RTCPeerConnection> create(ScriptExecutionContext&);
    ~RTCPeerConnection();

    void initializeWith(Document&, const Dictionary&, ExceptionCode&);

    const Vector<RefPtr<RTCRtpSender>>& getSenders() const { return m_transceiverSet->getSenders(); }
    const Vector<RefPtr<RTCRtpReceiver>>& getReceivers() const { return m_transceiverSet->getReceivers(); }
    const Vector<RefPtr<RTCRtpTransceiver>>& getTransceivers() const override { return m_transceiverSet->list(); }

    // Part of legacy MediaStream-based API (mostly implemented as JS built-ins)
    Vector<RefPtr<MediaStream>> getRemoteStreams() const { return m_backend->getRemoteStreams(); }

    RefPtr<RTCRtpSender> addTrack(Ref<MediaStreamTrack>&&, const Vector<MediaStream*>&, ExceptionCode&);
    void removeTrack(RTCRtpSender&, ExceptionCode&);

    // This enum is mirrored in RTCRtpTransceiver.h
    enum class RtpTransceiverDirection { Sendrecv, Sendonly, Recvonly, Inactive };

    struct RtpTransceiverInit {
        RtpTransceiverDirection direction;
    };

    RefPtr<RTCRtpTransceiver> addTransceiver(Ref<MediaStreamTrack>&&, const RtpTransceiverInit&, ExceptionCode&);
    RefPtr<RTCRtpTransceiver> addTransceiver(const String& kind, const RtpTransceiverInit&, ExceptionCode&);

    void queuedCreateOffer(const Dictionary& offerOptions, PeerConnection::SessionDescriptionPromise&&);
    void queuedCreateAnswer(const Dictionary& answerOptions, PeerConnection::SessionDescriptionPromise&&);

    void queuedSetLocalDescription(RTCSessionDescription&, PeerConnection::VoidPromise&&);
    RefPtr<RTCSessionDescription> localDescription() const;
    RefPtr<RTCSessionDescription> currentLocalDescription() const;
    RefPtr<RTCSessionDescription> pendingLocalDescription() const;

    void queuedSetRemoteDescription(RTCSessionDescription&, PeerConnection::VoidPromise&&);
    RefPtr<RTCSessionDescription> remoteDescription() const;
    RefPtr<RTCSessionDescription> currentRemoteDescription() const;
    RefPtr<RTCSessionDescription> pendingRemoteDescription() const;

    String signalingState() const;

    void queuedAddIceCandidate(RTCIceCandidate&, PeerConnection::VoidPromise&&);

    String iceGatheringState() const;
    String iceConnectionState() const;

    RTCConfiguration* getConfiguration() const;
    void setConfiguration(const Dictionary& configuration, ExceptionCode&);

    void privateGetStats(MediaStreamTrack*, PeerConnection::StatsPromise&&);

    RefPtr<RTCDataChannel> createDataChannel(String label, const Dictionary& dataChannelDict, ExceptionCode&);

    void close();

    // EventTarget
    EventTargetInterface eventTargetInterface() const override { return RTCPeerConnectionEventTargetInterfaceType; }
    ScriptExecutionContext* scriptExecutionContext() const override { return ActiveDOMObject::scriptExecutionContext(); }

    using RefCounted<RTCPeerConnection>::ref;
    using RefCounted<RTCPeerConnection>::deref;

private:
    RTCPeerConnection(ScriptExecutionContext&);

    RefPtr<RTCRtpTransceiver> completeAddTransceiver(Ref<RTCRtpTransceiver>&&, const RtpTransceiverInit&);

    // EventTarget implementation.
    void refEventTarget() override { ref(); }
    void derefEventTarget() override { deref(); }

    // ActiveDOMObject
    void stop() override;
    const char* activeDOMObjectName() const override;
    bool canSuspendForDocumentSuspension() const override;

    // PeerConnectionBackendClient
    void addTransceiver(RefPtr<RTCRtpTransceiver>&&) override;
    void setSignalingState(PeerConnectionStates::SignalingState) override;
    void updateIceGatheringState(PeerConnectionStates::IceGatheringState) override;
    void updateIceConnectionState(PeerConnectionStates::IceConnectionState) override;

    void scheduleNegotiationNeededEvent() override;

    RTCRtpSenderClient& senderClient() { return *this; }
    void fireEvent(Event&) override;
    PeerConnectionStates::SignalingState internalSignalingState() const override { return m_signalingState; }
    PeerConnectionStates::IceGatheringState internalIceGatheringState() const override { return m_iceGatheringState; }
    PeerConnectionStates::IceConnectionState internalIceConnectionState() const override { return m_iceConnectionState; }

    // RTCRtpSenderClient
    void replaceTrack(RTCRtpSender&, RefPtr<MediaStreamTrack>&&, PeerConnection::VoidPromise&&) override;

    PeerConnectionStates::SignalingState m_signalingState { PeerConnectionStates::SignalingState::Stable };
    PeerConnectionStates::IceGatheringState m_iceGatheringState { PeerConnectionStates::IceGatheringState::New };
    PeerConnectionStates::IceConnectionState m_iceConnectionState { PeerConnectionStates::IceConnectionState::New };

    std::unique_ptr<RtpTransceiverSet> m_transceiverSet { std::unique_ptr<RtpTransceiverSet>(new RtpTransceiverSet()) };

    Vector<RefPtr<RTCDataChannel>> m_dataChannels;

    std::unique_ptr<PeerConnectionBackend> m_backend;

    RefPtr<RTCConfiguration> m_configuration;
};

} // namespace WebCore

#endif // ENABLE(WEB_RTC)

#endif // RTCPeerConnection_h
