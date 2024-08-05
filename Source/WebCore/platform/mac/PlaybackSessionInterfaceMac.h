/*
 * Copyright (C) 2016-2017 Apple Inc. All rights reserved.
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

#if PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE)

#include "HTMLMediaElementEnums.h"
#include "PlaybackSessionInterface.h"
#include "PlaybackSessionModel.h"
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

OBJC_CLASS WebPlaybackControlsManager;

namespace WebCore {
class IntRect;
class PlaybackSessionModel;

class WEBCORE_EXPORT PlaybackSessionInterfaceMac final
    : public PlaybackSessionInterface
    , public PlaybackSessionModelClient
    , public RefCounted<PlaybackSessionInterfaceMac> {
public:
    static Ref<PlaybackSessionInterfaceMac> create(PlaybackSessionModel&);
    virtual ~PlaybackSessionInterfaceMac();
    PlaybackSessionModel* playbackSessionModel() const { return m_playbackSessionModel; }

    // PlaybackSessionInterface
    WEBCORE_EXPORT void resetMediaState() final { }

    // PlaybackSessionModelClient
    WEBCORE_EXPORT void durationChanged(double) final;
    WEBCORE_EXPORT void currentTimeChanged(double /*currentTime*/, double /*anchorTime*/) final;
    WEBCORE_EXPORT void rateChanged(bool /*isPlaying*/, float /*playbackRate*/) final;
    WEBCORE_EXPORT void seekableRangesChanged(const TimeRanges&, double /*lastModifiedTime*/, double /*liveUpdateInterval*/) final;
    WEBCORE_EXPORT void audioMediaSelectionOptionsChanged(const Vector<MediaSelectionOption>& /*options*/, uint64_t /*selectedIndex*/) final;
    WEBCORE_EXPORT void legibleMediaSelectionOptionsChanged(const Vector<MediaSelectionOption>& /*options*/, uint64_t /*selectedIndex*/) final;
    WEBCORE_EXPORT void audioMediaSelectionIndexChanged(uint64_t) final;
    WEBCORE_EXPORT void legibleMediaSelectionIndexChanged(uint64_t) final;
    WEBCORE_EXPORT void externalPlaybackChanged(bool /* enabled */, PlaybackSessionModel::ExternalPlaybackTargetType, const String& /* localizedDeviceName */) final;

    WEBCORE_EXPORT void invalidate();
    WEBCORE_EXPORT void ensureControlsManager();
#if ENABLE(WEB_PLAYBACK_CONTROLS_MANAGER)
    WEBCORE_EXPORT void setPlayBackControlsManager(WebPlaybackControlsManager *);
    WEBCORE_EXPORT WebPlaybackControlsManager *playBackControlsManager();
#endif
    WEBCORE_EXPORT void beginScrubbing();
    WEBCORE_EXPORT void endScrubbing();

private:
    PlaybackSessionInterfaceMac(PlaybackSessionModel&);
    PlaybackSessionModel* m_playbackSessionModel { nullptr };
#if ENABLE(WEB_PLAYBACK_CONTROLS_MANAGER)
    WebPlaybackControlsManager *m_playbackControlsManager  { nullptr };

    void updatePlaybackControlsManagerTiming(double currentTime, double anchorTime, double playbackRate, bool isPlaying);
#endif

};

}

#endif // PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE)
