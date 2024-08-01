/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "MockRealtimeMediaSource.h"

#if ENABLE(MEDIA_STREAM)
#include "Logging.h"
#include "MediaConstraints.h"
#include "MediaStreamTrackSourcesRequestClient.h"
#include "NotImplemented.h"
#include "RealtimeMediaSourceSettings.h"
#include <math.h>
#include <wtf/CurrentTime.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringView.h>

namespace WebCore {

const AtomicString& MockRealtimeMediaSource::mockAudioSourcePersistentID()
{
    static NeverDestroyed<AtomicString> id("239c24b1-2b15-11e3-8224-0800200c9a66", AtomicString::ConstructFromLiteral);
    return id;
}

const AtomicString& MockRealtimeMediaSource::mockVideoSourcePersistentID()
{
    static NeverDestroyed<AtomicString> id("239c24b0-2b15-11e3-8224-0800200c9a66", AtomicString::ConstructFromLiteral);
    return id;
}

const AtomicString& MockRealtimeMediaSource::mockAudioSourceName()
{
    static NeverDestroyed<AtomicString> name("Mock audio device", AtomicString::ConstructFromLiteral);
    return name;
}

const AtomicString& MockRealtimeMediaSource::mockVideoSourceName()
{
    static NeverDestroyed<AtomicString> name("Mock video device", AtomicString::ConstructFromLiteral);
    return name;
}

RefPtr<TrackSourceInfo> MockRealtimeMediaSource::trackSourceWithUID(const String& id, MediaConstraints*)
{
    // FIXME: validate constraints.

    if (mockAudioSourcePersistentID() == id)
        return TrackSourceInfo::create(mockAudioSourcePersistentID(), id, TrackSourceInfo::Audio, "Mock audio device");

    if (mockVideoSourcePersistentID() == id)
        return TrackSourceInfo::create(mockVideoSourcePersistentID(), id, TrackSourceInfo::Video, "Mock video device");
    
    return nullptr;
}

MockRealtimeMediaSource::MockRealtimeMediaSource(const String& id, RealtimeMediaSource::Type type, const String& name)
    : RealtimeMediaSource(id, type, name)
{
    if (type == RealtimeMediaSource::Audio)
        setPersistentID(mockAudioSourcePersistentID());
    else
        setPersistentID(mockVideoSourcePersistentID());
}

RefPtr<RealtimeMediaSourceCapabilities> MockRealtimeMediaSource::capabilities()
{
    if (m_capabilities)
        return m_capabilities;

    m_capabilities = RealtimeMediaSourceCapabilities::create(supportedConstraints());
    m_capabilities->setDeviceId(id());
    initializeCapabilities(*m_capabilities.get());

    return m_capabilities;
}

const RealtimeMediaSourceSettings& MockRealtimeMediaSource::settings()
{
    if (m_currentSettings.deviceId().isEmpty()) {
        m_currentSettings.setSupportedConstraits(supportedConstraints());
        m_currentSettings.setDeviceId(id());
    }

    updateSettings(m_currentSettings);
    return m_currentSettings;
}

RealtimeMediaSourceSupportedConstraints& MockRealtimeMediaSource::supportedConstraints()
{
    if (m_supportedConstraints.supportsDeviceId())
        return m_supportedConstraints;

    m_supportedConstraints.setSupportsDeviceId(true);
    initializeSupportedConstraints(m_supportedConstraints);

    return m_supportedConstraints;
}


} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
