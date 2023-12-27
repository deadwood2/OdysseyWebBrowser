/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
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
#include "MediaSession.h"

#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#include "Logging.h"
#include "MediaPlayer.h"
#include "MediaSessionManager.h"

namespace WebCore {

const double kClientDataBufferingTimerThrottleDelay = 0.1;

#if !LOG_DISABLED
static const char* stateName(MediaSession::State state)
{
#define CASE(state) case MediaSession::state: return #state
    switch (state) {
    CASE(Idle);
    CASE(Playing);
    CASE(Paused);
    CASE(Interrupted);
    }

    ASSERT_NOT_REACHED();
    return "";
}
#endif

std::unique_ptr<MediaSession> MediaSession::create(MediaSessionClient& client)
{
    return std::make_unique<MediaSession>(client);
}

MediaSession::MediaSession(MediaSessionClient& client)
    : m_client(client)
    , m_clientDataBufferingTimer(*this, &MediaSession::clientDataBufferingTimerFired)
    , m_state(Idle)
    , m_stateToRestore(Idle)
    , m_notifyingClient(false)
{
    ASSERT(m_client.mediaType() >= None && m_client.mediaType() <= WebAudio);
    MediaSessionManager::sharedManager().addSession(*this);
}

MediaSession::~MediaSession()
{
    MediaSessionManager::sharedManager().removeSession(*this);
}

void MediaSession::setState(State state)
{
    LOG(Media, "MediaSession::setState(%p) - %s", this, stateName(state));
    m_state = state;
}

void MediaSession::beginInterruption(InterruptionType type)
{
    LOG(Media, "MediaSession::beginInterruption(%p), state = %s, interruption count = %i", this, stateName(m_state), m_interruptionCount);

    if (++m_interruptionCount > 1 || (type == EnteringBackground && client().overrideBackgroundPlaybackRestriction()))
        return;

    m_stateToRestore = state();
    m_notifyingClient = true;
    setState(Interrupted);
    client().suspendPlayback();
    m_notifyingClient = false;
}

void MediaSession::endInterruption(EndInterruptionFlags flags)
{
    LOG(Media, "MediaSession::endInterruption(%p) - flags = %i, stateToRestore = %s, interruption count = %i", this, (int)flags, stateName(m_stateToRestore), m_interruptionCount);

    if (!m_interruptionCount) {
        LOG(Media, "MediaSession::endInterruption(%p) - !! ignoring spurious interruption end !!", this);
        return;
    }

    if (--m_interruptionCount)
        return;

    State stateToRestore = m_stateToRestore;
    m_stateToRestore = Idle;
    setState(Paused);

    bool shouldResume = flags & MayResumePlaying && stateToRestore == Playing;
    client().mayResumePlayback(shouldResume);
}

bool MediaSession::clientWillBeginPlayback()
{
    if (m_notifyingClient)
        return true;

    if (!MediaSessionManager::sharedManager().sessionWillBeginPlayback(*this)) {
        if (state() == Interrupted)
            m_stateToRestore = Playing;
        return false;
    }

    setState(Playing);
    updateClientDataBuffering();
    return true;
}

bool MediaSession::clientWillPausePlayback()
{
    if (m_notifyingClient)
        return true;

    LOG(Media, "MediaSession::clientWillPausePlayback(%p)- state = %s", this, stateName(m_state));
    if (state() == Interrupted) {
        m_stateToRestore = Paused;
        LOG(Media, "      setting stateToRestore to \"Paused\"");
        return false;
    }
    
    setState(Paused);
    MediaSessionManager::sharedManager().sessionWillEndPlayback(*this);
    if (!m_clientDataBufferingTimer.isActive())
        m_clientDataBufferingTimer.startOneShot(kClientDataBufferingTimerThrottleDelay);
    return true;
}

void MediaSession::pauseSession()
{
    LOG(Media, "MediaSession::pauseSession(%p)", this);
    m_client.suspendPlayback();
}

MediaSession::MediaType MediaSession::mediaType() const
{
    return m_client.mediaType();
}

MediaSession::MediaType MediaSession::presentationType() const
{
    return m_client.presentationType();
}

String MediaSession::title() const
{
    return m_client.mediaSessionTitle();
}

double MediaSession::duration() const
{
    return m_client.mediaSessionDuration();
}

double MediaSession::currentTime() const
{
    return m_client.mediaSessionCurrentTime();
}
    
bool MediaSession::canReceiveRemoteControlCommands() const
{
    return m_client.canReceiveRemoteControlCommands();
}

void MediaSession::didReceiveRemoteControlCommand(RemoteControlCommandType command)
{
    m_client.didReceiveRemoteControlCommand(command);
}

void MediaSession::visibilityChanged()
{
    if (!m_clientDataBufferingTimer.isActive())
        m_clientDataBufferingTimer.startOneShot(kClientDataBufferingTimerThrottleDelay);
}

void MediaSession::clientDataBufferingTimerFired()
{
    LOG(Media, "MediaSession::clientDataBufferingTimerFired(%p)- visible = %s", this, m_client.elementIsHidden() ? "false" : "true");

    updateClientDataBuffering();

    if (m_state != Playing || !m_client.elementIsHidden())
        return;

    MediaSessionManager::SessionRestrictions restrictions = MediaSessionManager::sharedManager().restrictions(mediaType());
    if ((restrictions & MediaSessionManager::BackgroundTabPlaybackRestricted) == MediaSessionManager::BackgroundTabPlaybackRestricted)
        pauseSession();
}

void MediaSession::updateClientDataBuffering()
{
    if (m_clientDataBufferingTimer.isActive())
        m_clientDataBufferingTimer.stop();

    m_client.setShouldBufferData(MediaSessionManager::sharedManager().sessionCanLoadMedia(*this));
}

bool MediaSession::isHidden() const
{
    return m_client.elementIsHidden();
}

MediaSession::DisplayType MediaSession::displayType() const
{
    return m_client.displayType();
}

String MediaSessionClient::mediaSessionTitle() const
{
    return String();
}

double MediaSessionClient::mediaSessionDuration() const
{
    return MediaPlayer::invalidTime();
}

double MediaSessionClient::mediaSessionCurrentTime() const
{
    return MediaPlayer::invalidTime();
}
}
#endif
