/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011, 2012, 2013 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(VIDEO_TRACK)

#include "AudioTrackPrivate.h"
#include "ExceptionCode.h"
#include "PlatformExportMacros.h"
#include "TrackBase.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class AudioTrack;

class AudioTrackClient {
public:
    virtual ~AudioTrackClient() { }
    virtual void audioTrackEnabledChanged(AudioTrack*) = 0;
};

class AudioTrack final : public MediaTrackBase, public AudioTrackPrivateClient {
public:
    static Ref<AudioTrack> create(AudioTrackClient* client, PassRefPtr<AudioTrackPrivate> trackPrivate)
    {
        return adoptRef(*new AudioTrack(client, trackPrivate));
    }
    virtual ~AudioTrack();

    static const AtomicString& alternativeKeyword();
    static const AtomicString& descriptionKeyword();
    static const AtomicString& mainKeyword();
    static const AtomicString& mainDescKeyword();
    static const AtomicString& translationKeyword();
    static const AtomicString& commentaryKeyword();
    const AtomicString& defaultKindKeyword() const override { return emptyAtom; }

    bool enabled() const override { return m_enabled; }
    void setEnabled(const bool);

    void clearClient() override { m_client = nullptr; }
    AudioTrackClient* client() const { return m_client; }

    size_t inbandTrackIndex();

    void setPrivate(PassRefPtr<AudioTrackPrivate>);

protected:
    AudioTrack(AudioTrackClient*, PassRefPtr<AudioTrackPrivate>);

private:
    bool isValidKind(const AtomicString&) const override;

    // AudioTrackPrivateClient
    void enabledChanged(AudioTrackPrivate*, bool) override;

    // TrackPrivateBaseClient
    void idChanged(TrackPrivateBase*, const AtomicString&) override;
    void labelChanged(TrackPrivateBase*, const AtomicString&) override;
    void languageChanged(TrackPrivateBase*, const AtomicString&) override;
    void willRemove(TrackPrivateBase*) override;

    void updateKindFromPrivate();

    bool m_enabled;
    AudioTrackClient* m_client;

    RefPtr<AudioTrackPrivate> m_private;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::AudioTrack)
    static bool isType(const WebCore::TrackBase& track) { return track.type() == WebCore::TrackBase::AudioTrack; }
SPECIALIZE_TYPE_TRAITS_END()

#endif
