/*
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
 * 3. Neither the name of Ericsson nor the names of its contributors
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

#include "config.h"

#if ENABLE(WEB_RTC)
#include "MediaEndpoint.h"

#include "MediaPayload.h"
#include "RTCDataChannelHandler.h"
#include "RealtimeMediaSource.h"

namespace WebCore {

class EmptyRealtimeMediaSource final : public RealtimeMediaSource {
public:
    static Ref<EmptyRealtimeMediaSource> create() { return adoptRef(*new EmptyRealtimeMediaSource()); }

private:
    EmptyRealtimeMediaSource() : RealtimeMediaSource(emptyString(), Type::None, emptyString()) { }

    const RealtimeMediaSourceCapabilities& capabilities() const final { return RealtimeMediaSourceCapabilities::emptyCapabilities(); }
    const RealtimeMediaSourceSettings& settings() const final { return m_sourceSettings; }

    RealtimeMediaSourceSettings m_sourceSettings;
};

class EmptyMediaEndpoint final : public MediaEndpoint {
private:
    void setConfiguration(MediaEndpointConfiguration&&) final { }

    void generateDtlsInfo() final { }
    MediaPayloadVector getDefaultAudioPayloads() final { return MediaPayloadVector(); }
    MediaPayloadVector getDefaultVideoPayloads() final { return MediaPayloadVector(); }
    MediaPayloadVector filterPayloads(const MediaPayloadVector&, const MediaPayloadVector&) final { return MediaPayloadVector(); }

    UpdateResult updateReceiveConfiguration(MediaEndpointSessionConfiguration*, bool) final { return UpdateResult::Failed; }
    UpdateResult updateSendConfiguration(MediaEndpointSessionConfiguration*, const RealtimeMediaSourceMap&, bool) final { return UpdateResult::Failed; }

    void addRemoteCandidate(const IceCandidate&, const String&, const String&, const String&) final { }

    Ref<RealtimeMediaSource> createMutedRemoteSource(const String&, RealtimeMediaSource::Type) final { return EmptyRealtimeMediaSource::create(); }
    void replaceSendSource(RealtimeMediaSource&, const String&) final { }
    void replaceMutedRemoteSourceMid(const String&, const String&) final { };

    std::unique_ptr<RTCDataChannelHandler> createDataChannelHandler(const String&, const RTCDataChannelInit&) final { return nullptr; }
    void stop() final { }
};

static std::unique_ptr<MediaEndpoint> createMediaEndpoint(MediaEndpointClient&)
{
    return std::make_unique<EmptyMediaEndpoint>();
}

CreateMediaEndpoint MediaEndpoint::create = createMediaEndpoint;

} // namespace WebCore

#endif // ENABLE(WEB_RTC)
