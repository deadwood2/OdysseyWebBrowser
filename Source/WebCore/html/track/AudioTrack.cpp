/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 * Copyright (C) 2011, 2012, 2013 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#if ENABLE(VIDEO_TRACK)

#include "AudioTrack.h"

#include "AudioTrackList.h"
#include "Event.h"
#include "HTMLMediaElement.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

const AtomicString& AudioTrack::alternativeKeyword()
{
    static NeverDestroyed<AtomicString> alternative("alternative", AtomicString::ConstructFromLiteral);
    return alternative;
}

const AtomicString& AudioTrack::descriptionKeyword()
{
    static NeverDestroyed<AtomicString> description("description", AtomicString::ConstructFromLiteral);
    return description;
}

const AtomicString& AudioTrack::mainKeyword()
{
    static NeverDestroyed<AtomicString> main("main", AtomicString::ConstructFromLiteral);
    return main;
}

const AtomicString& AudioTrack::mainDescKeyword()
{
    static NeverDestroyed<AtomicString> mainDesc("main-desc", AtomicString::ConstructFromLiteral);
    return mainDesc;
}

const AtomicString& AudioTrack::translationKeyword()
{
    static NeverDestroyed<AtomicString> translation("translation", AtomicString::ConstructFromLiteral);
    return translation;
}

const AtomicString& AudioTrack::commentaryKeyword()
{
    static NeverDestroyed<AtomicString> commentary("commentary", AtomicString::ConstructFromLiteral);
    return commentary;
}

AudioTrack::AudioTrack(AudioTrackClient* client, PassRefPtr<AudioTrackPrivate> trackPrivate)
    : MediaTrackBase(MediaTrackBase::AudioTrack, trackPrivate->id(), trackPrivate->label(), trackPrivate->language())
    , m_enabled(trackPrivate->enabled())
    , m_client(client)
    , m_private(trackPrivate)
{
    m_private->setClient(this);
    updateKindFromPrivate();
}

AudioTrack::~AudioTrack()
{
    m_private->setClient(nullptr);
}

void AudioTrack::setPrivate(PassRefPtr<AudioTrackPrivate> trackPrivate)
{
    ASSERT(m_private);
    ASSERT(trackPrivate);

    if (m_private == trackPrivate)
        return;

    m_private->setClient(nullptr);
    m_private = trackPrivate;
    m_private->setClient(this);

    m_private->setEnabled(m_enabled);
    updateKindFromPrivate();
}

bool AudioTrack::isValidKind(const AtomicString& value) const
{
    return value == alternativeKeyword()
        || value == descriptionKeyword()
        || value == mainKeyword()
        || value == mainDescKeyword()
        || value == translationKeyword()
        || value == commentaryKeyword();
}

void AudioTrack::setEnabled(const bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    m_private->setEnabled(enabled);

    if (m_client)
        m_client->audioTrackEnabledChanged(this);
}

size_t AudioTrack::inbandTrackIndex()
{
    ASSERT(m_private);
    return m_private->trackIndex();
}

void AudioTrack::enabledChanged(AudioTrackPrivate* trackPrivate, bool enabled)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);
    m_enabled = enabled;

    if (m_client)
        m_client->audioTrackEnabledChanged(this);
}

void AudioTrack::idChanged(TrackPrivateBase* trackPrivate, const AtomicString& id)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);
    setId(id);
}

void AudioTrack::labelChanged(TrackPrivateBase* trackPrivate, const AtomicString& label)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);
    setLabel(label);
}

void AudioTrack::languageChanged(TrackPrivateBase* trackPrivate, const AtomicString& language)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);
    setLanguage(language);
}

void AudioTrack::willRemove(TrackPrivateBase* trackPrivate)
{
    ASSERT_UNUSED(trackPrivate, trackPrivate == m_private);
    mediaElement()->removeAudioTrack(*this);
}

void AudioTrack::updateKindFromPrivate()
{
    switch (m_private->kind()) {
    case AudioTrackPrivate::Alternative:
        setKind(AudioTrack::alternativeKeyword());
        break;
    case AudioTrackPrivate::Description:
        setKind(AudioTrack::descriptionKeyword());
        break;
    case AudioTrackPrivate::Main:
        setKind(AudioTrack::mainKeyword());
        break;
    case AudioTrackPrivate::MainDesc:
        setKind(AudioTrack::mainDescKeyword());
        break;
    case AudioTrackPrivate::Translation:
        setKind(AudioTrack::translationKeyword());
        break;
    case AudioTrackPrivate::Commentary:
        setKind(AudioTrack::commentaryKeyword());
        break;
    case AudioTrackPrivate::None:
        setKind(emptyString());
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

} // namespace WebCore

#endif
