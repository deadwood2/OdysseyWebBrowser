/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO)

#include "HTMLMediaSession.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#include "Logging.h"
#include "MediaSessionManager.h"
#include "Page.h"
#include "ScriptController.h"
#include "SourceBuffer.h"

#if PLATFORM(IOS)
#include "AudioSession.h"
#include "RuntimeApplicationChecksIOS.h"
#endif

namespace WebCore {

#if !LOG_DISABLED
static String restrictionName(HTMLMediaSession::BehaviorRestrictions restriction)
{
    StringBuilder restrictionBuilder;
#define CASE(restrictionType) \
    if (restriction & HTMLMediaSession::restrictionType) { \
        if (!restrictionBuilder.isEmpty()) \
            restrictionBuilder.append(", "); \
        restrictionBuilder.append(#restrictionType); \
    } \

    CASE(NoRestrictions);
    CASE(RequireUserGestureForLoad);
    CASE(RequireUserGestureForRateChange);
    CASE(RequireUserGestureForAudioRateChange);
    CASE(RequireUserGestureForFullscreen);
    CASE(RequirePageConsentToLoadMedia);
    CASE(RequirePageConsentToResumeMedia);
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    CASE(RequireUserGestureToShowPlaybackTargetPicker);
    CASE(WirelessVideoPlaybackDisabled);
#endif
    CASE(RequireUserGestureForAudioRateChange);

    return restrictionBuilder.toString();
}
#endif

HTMLMediaSession::HTMLMediaSession(MediaSessionClient& client)
    : MediaSession(client)
    , m_restrictions(NoRestrictions)
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , m_targetAvailabilityChangedTimer(*this, &HTMLMediaSession::targetAvailabilityChangedTimerFired)
#endif
{
}

void HTMLMediaSession::registerWithDocument(const HTMLMediaElement& element)
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    element.document().addPlaybackTargetPickerClient(*this);
#else
    UNUSED_PARAM(element);
#endif
}

void HTMLMediaSession::unregisterWithDocument(const HTMLMediaElement& element)
{
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    element.document().removePlaybackTargetPickerClient(*this);
#else
    UNUSED_PARAM(element);
#endif
}

void HTMLMediaSession::addBehaviorRestriction(BehaviorRestrictions restriction)
{
    LOG(Media, "HTMLMediaSession::addBehaviorRestriction - adding %s", restrictionName(restriction).utf8().data());
    m_restrictions |= restriction;
}

void HTMLMediaSession::removeBehaviorRestriction(BehaviorRestrictions restriction)
{
    LOG(Media, "HTMLMediaSession::removeBehaviorRestriction - removing %s", restrictionName(restriction).utf8().data());
    m_restrictions &= ~restriction;
}

bool HTMLMediaSession::playbackPermitted(const HTMLMediaElement& element) const
{
    if (m_restrictions & RequireUserGestureForRateChange && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::playbackPermitted - returning FALSE");
        return false;
    }

    if (m_restrictions & RequireUserGestureForAudioRateChange && element.hasAudio() && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::playbackPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::dataLoadingPermitted(const HTMLMediaElement&) const
{
    if (m_restrictions & RequireUserGestureForLoad && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::dataLoadingPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::fullscreenPermitted(const HTMLMediaElement&) const
{
    if (m_restrictions & RequireUserGestureForFullscreen && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::fullscreenPermitted - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::pageAllowsDataLoading(const HTMLMediaElement& element) const
{
    Page* page = element.document().page();
    if (m_restrictions & RequirePageConsentToLoadMedia && page && !page->canStartMedia()) {
        LOG(Media, "HTMLMediaSession::pageAllowsDataLoading - returning FALSE");
        return false;
    }

    return true;
}

bool HTMLMediaSession::pageAllowsPlaybackAfterResuming(const HTMLMediaElement& element) const
{
    Page* page = element.document().page();
    if (m_restrictions & RequirePageConsentToResumeMedia && page && !page->canStartMedia()) {
        LOG(Media, "HTMLMediaSession::pageAllowsPlaybackAfterResuming - returning FALSE");
        return false;
    }

    return true;
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
bool HTMLMediaSession::currentPlaybackTargetIsWireless(const HTMLMediaElement& element) const
{
    MediaPlayer* player = element.player();
    if (!player) {
        LOG(Media, "HTMLMediaSession::currentPlaybackTargetIsWireless - returning FALSE because player is NULL");
        return false;
    }

    bool isWireless = player->isCurrentPlaybackTargetWireless();
    LOG(Media, "HTMLMediaSession::currentPlaybackTargetIsWireless - returning %s", isWireless ? "TRUE" : "FALSE");

    return isWireless;
}

void HTMLMediaSession::showPlaybackTargetPicker(const HTMLMediaElement& element)
{
    LOG(Media, "HTMLMediaSession::showPlaybackTargetPicker");

    if (m_restrictions & RequireUserGestureToShowPlaybackTargetPicker && !ScriptController::processingUserGesture()) {
        LOG(Media, "HTMLMediaSession::showPlaybackTargetPicker - returning early because of permissions");
        return;
    }

    if (!element.document().page()) {
        LOG(Media, "HTMLMediaSession::showingPlaybackTargetPickerPermitted - returning early because page is NULL");
        return;
    }

#if !PLATFORM(IOS)
    if (!element.hasVideo()) {
        LOG(Media, "HTMLMediaSession::showPlaybackTargetPicker - returning early because element has no video");
        return;
    }
#endif

    element.document().showPlaybackTargetPicker(*this, is<HTMLVideoElement>(element));
}

bool HTMLMediaSession::hasWirelessPlaybackTargets(const HTMLMediaElement&) const
{
#if PLATFORM(IOS)
    // FIXME: consolidate Mac and iOS implementations
    m_hasPlaybackTargets = MediaSessionManager::sharedManager().hasWirelessTargetsAvailable();
#endif

    LOG(Media, "HTMLMediaSession::hasWirelessPlaybackTargets - returning %s", m_hasPlaybackTargets ? "TRUE" : "FALSE");

    return m_hasPlaybackTargets;
}

bool HTMLMediaSession::wirelessVideoPlaybackDisabled(const HTMLMediaElement& element) const
{
    Settings* settings = element.document().settings();
    if (!settings || !settings->mediaPlaybackAllowsAirPlay()) {
        LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning TRUE because of settings");
        return true;
    }

    if (element.fastHasAttribute(HTMLNames::webkitwirelessvideoplaybackdisabledAttr)) {
        LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning TRUE because of attribute");
        return true;
    }

#if PLATFORM(IOS)
    String legacyAirplayAttributeValue = element.fastGetAttribute(HTMLNames::webkitairplayAttr);
    if (equalIgnoringCase(legacyAirplayAttributeValue, "deny")) {
        LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning TRUE because of legacy attribute");
        return true;
    }
    if (equalIgnoringCase(legacyAirplayAttributeValue, "allow")) {
        LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning FALSE because of legacy attribute");
        return false;
    }
#endif

    MediaPlayer* player = element.player();
    if (!player)
        return true;

    bool disabled = player->wirelessVideoPlaybackDisabled();
    LOG(Media, "HTMLMediaSession::wirelessVideoPlaybackDisabled - returning %s because media engine says so", disabled ? "TRUE" : "FALSE");
    
    return disabled;
}

void HTMLMediaSession::setWirelessVideoPlaybackDisabled(const HTMLMediaElement& element, bool disabled)
{
    if (disabled)
        addBehaviorRestriction(WirelessVideoPlaybackDisabled);
    else
        removeBehaviorRestriction(WirelessVideoPlaybackDisabled);

    MediaPlayer* player = element.player();
    if (!player)
        return;

    LOG(Media, "HTMLMediaSession::setWirelessVideoPlaybackDisabled - disabled %s", disabled ? "TRUE" : "FALSE");
    player->setWirelessVideoPlaybackDisabled(disabled);
}

void HTMLMediaSession::setHasPlaybackTargetAvailabilityListeners(const HTMLMediaElement& element, bool hasListeners)
{
    LOG(Media, "HTMLMediaSession::setHasPlaybackTargetAvailabilityListeners - hasListeners %s", hasListeners ? "TRUE" : "FALSE");

#if PLATFORM(IOS)
    UNUSED_PARAM(element);
    m_hasPlaybackTargetAvailabilityListeners = hasListeners;
    MediaSessionManager::sharedManager().configureWireLessTargetMonitoring();
#else
    UNUSED_PARAM(hasListeners);
    element.document().playbackTargetPickerClientStateDidChange(*this, element.mediaState());
#endif
}

void HTMLMediaSession::setPlaybackTarget(Ref<MediaPlaybackTarget>&& device)
{
    m_playbackTarget = WTF::move(device);
    client().setWirelessPlaybackTarget(*m_playbackTarget.copyRef());
}

void HTMLMediaSession::targetAvailabilityChangedTimerFired()
{
    client().wirelessRoutesAvailableDidChange();
}

void HTMLMediaSession::externalOutputDeviceAvailableDidChange(bool hasTargets)
{
    if (m_hasPlaybackTargets == hasTargets)
        return;

    LOG(Media, "HTMLMediaSession::externalOutputDeviceAvailableDidChange - hasTargets %s", hasTargets ? "TRUE" : "FALSE");

    m_hasPlaybackTargets = hasTargets;
    if (!m_targetAvailabilityChangedTimer.isActive())
        m_targetAvailabilityChangedTimer.startOneShot(0);
}

bool HTMLMediaSession::canPlayToWirelessPlaybackTarget() const
{
    if (!m_playbackTarget || !m_playbackTarget->hasActiveRoute())
        return false;

    return client().canPlayToWirelessPlaybackTarget();
}

bool HTMLMediaSession::isPlayingToWirelessPlaybackTarget() const
{
    if (!m_playbackTarget || !m_playbackTarget->hasActiveRoute())
        return false;

    return client().isPlayingToWirelessPlaybackTarget();
}

void HTMLMediaSession::setShouldPlayToPlaybackTarget(bool shouldPlay)
{
    m_shouldPlayToPlaybackTarget = shouldPlay;
    client().setShouldPlayToPlaybackTarget(shouldPlay);
}

void HTMLMediaSession::mediaStateDidChange(const HTMLMediaElement& element, MediaProducer::MediaStateFlags state)
{
    element.document().playbackTargetPickerClientStateDidChange(*this, state);
}
#endif

MediaPlayer::Preload HTMLMediaSession::effectivePreloadForElement(const HTMLMediaElement& element) const
{
    MediaSessionManager::SessionRestrictions restrictions = MediaSessionManager::sharedManager().restrictions(mediaType());
    MediaPlayer::Preload preload = element.preloadValue();

    if ((restrictions & MediaSessionManager::MetadataPreloadingNotPermitted) == MediaSessionManager::MetadataPreloadingNotPermitted)
        return MediaPlayer::None;

    if ((restrictions & MediaSessionManager::AutoPreloadingNotPermitted) == MediaSessionManager::AutoPreloadingNotPermitted) {
        if (preload > MediaPlayer::MetaData)
            return MediaPlayer::MetaData;
    }

    return preload;
}

bool HTMLMediaSession::requiresFullscreenForVideoPlayback(const HTMLMediaElement& element) const
{
    if (!MediaSessionManager::sharedManager().sessionRestrictsInlineVideoPlayback(*this))
        return false;

    Settings* settings = element.document().settings();
    if (!settings || !settings->mediaPlaybackAllowsInline())
        return true;

    if (element.fastHasAttribute(HTMLNames::webkit_playsinlineAttr))
        return false;

#if PLATFORM(IOS)
    if (applicationIsDumpRenderTree())
        return false;
#endif

    return true;
}

void HTMLMediaSession::mediaEngineUpdated(const HTMLMediaElement& element)
{
    LOG(Media, "HTMLMediaSession::mediaEngineUpdated");

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (m_restrictions & WirelessVideoPlaybackDisabled)
        setWirelessVideoPlaybackDisabled(element, true);
    if (m_playbackTarget)
        client().setWirelessPlaybackTarget(*m_playbackTarget.copyRef());
    if (m_shouldPlayToPlaybackTarget)
        client().setShouldPlayToPlaybackTarget(m_shouldPlayToPlaybackTarget);
#else
    UNUSED_PARAM(element);
#endif
    
}

bool HTMLMediaSession::allowsAlternateFullscreen(const HTMLMediaElement& element) const
{
    Settings* settings = element.document().settings();
    return settings && settings->allowsAlternateFullscreen();
}

#if ENABLE(MEDIA_SOURCE)
const unsigned fiveMinutesOf1080PVideo = 290 * 1024 * 1024; // 290 MB is approximately 5 minutes of 8Mbps (1080p) content.
const unsigned fiveMinutesStereoAudio = 14 * 1024 * 1024; // 14 MB is approximately 5 minutes of 384kbps content.

size_t HTMLMediaSession::maximumMediaSourceBufferSize(const SourceBuffer& buffer) const
{
    // A good quality 1080p video uses 8,000 kbps and stereo audio uses 384 kbps, so assume 95% for video and 5% for audio.
    const float bufferBudgetPercentageForVideo = .95;
    const float bufferBudgetPercentageForAudio = .05;

    size_t maximum;
    Settings* settings = buffer.document().settings();
    if (settings)
        maximum = settings->maximumSourceBufferSize();
    else
        maximum = fiveMinutesOf1080PVideo + fiveMinutesStereoAudio;

    // Allow a SourceBuffer to buffer as though it is audio-only even if it doesn't have any active tracks (yet).
    size_t bufferSize = static_cast<size_t>(maximum * bufferBudgetPercentageForAudio);
    if (buffer.hasVideo())
        bufferSize += static_cast<size_t>(maximum * bufferBudgetPercentageForVideo);

    // FIXME: we might want to modify this algorithm to:
    // - decrease the maximum size for background tabs
    // - decrease the maximum size allowed for inactive elements when a process has more than one
    //   element, eg. so a page with many elements which are played one at a time doesn't keep
    //   everything buffered after an element has finished playing.

    return bufferSize;
}
#endif

}

#endif // ENABLE(VIDEO)
