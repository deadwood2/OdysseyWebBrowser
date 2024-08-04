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

#import "config.h"
#import "MediaSessionManagerIOS.h"

#if PLATFORM(IOS)

#import "Logging.h"
#import "MediaPlayer.h"
#import "MediaPlayerSPI.h"
#import "PlatformMediaSession.h"
#import "SoftLinking.h"
#import "SystemMemory.h"
#import "WebCoreThreadRun.h"
#import <AVFoundation/AVAudioSession.h>
#import <MediaPlayer/MPMediaItem.h>
#import <MediaPlayer/MPNowPlayingInfoCenter.h>
#import <MediaPlayer/MPVolumeView.h>
#import <UIKit/UIApplication.h>
#import <objc/runtime.h>
#import <wtf/MainThread.h>
#import <wtf/RAMSize.h>
#import <wtf/RetainPtr.h>

SOFT_LINK_FRAMEWORK(AVFoundation)
SOFT_LINK_CLASS(AVFoundation, AVAudioSession)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionInterruptionNotification, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionInterruptionTypeKey, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAudioSessionInterruptionOptionKey, NSString *)

#define AVAudioSession getAVAudioSessionClass()
#define AVAudioSessionInterruptionNotification getAVAudioSessionInterruptionNotification()
#define AVAudioSessionInterruptionTypeKey getAVAudioSessionInterruptionTypeKey()
#define AVAudioSessionInterruptionOptionKey getAVAudioSessionInterruptionOptionKey()

SOFT_LINK_FRAMEWORK(UIKit)
SOFT_LINK_CLASS(UIKit, UIApplication)
SOFT_LINK_POINTER(UIKit, UIApplicationWillResignActiveNotification, NSString *)
SOFT_LINK_POINTER(UIKit, UIApplicationWillEnterForegroundNotification, NSString *)
SOFT_LINK_POINTER(UIKit, UIApplicationDidBecomeActiveNotification, NSString *)
SOFT_LINK_POINTER(UIKit, UIApplicationDidEnterBackgroundNotification, NSString *)

#define UIApplication getUIApplicationClass()
#define UIApplicationWillResignActiveNotification getUIApplicationWillResignActiveNotification()
#define UIApplicationWillEnterForegroundNotification getUIApplicationWillEnterForegroundNotification()
#define UIApplicationDidBecomeActiveNotification getUIApplicationDidBecomeActiveNotification()
#define UIApplicationDidEnterBackgroundNotification getUIApplicationDidEnterBackgroundNotification()

SOFT_LINK_FRAMEWORK(MediaPlayer)
SOFT_LINK_CLASS(MediaPlayer, MPAVRoutingController)
SOFT_LINK_CLASS(MediaPlayer, MPNowPlayingInfoCenter)
SOFT_LINK_CLASS(MediaPlayer, MPVolumeView)
SOFT_LINK_POINTER(MediaPlayer, MPMediaItemPropertyTitle, NSString *)
SOFT_LINK_POINTER(MediaPlayer, MPMediaItemPropertyPlaybackDuration, NSString *)
SOFT_LINK_POINTER(MediaPlayer, MPNowPlayingInfoPropertyElapsedPlaybackTime, NSString *)
SOFT_LINK_POINTER(MediaPlayer, MPNowPlayingInfoPropertyPlaybackRate, NSString *)
SOFT_LINK_POINTER(MediaPlayer, MPVolumeViewWirelessRoutesAvailableDidChangeNotification, NSString *)


#define MPMediaItemPropertyTitle getMPMediaItemPropertyTitle()
#define MPMediaItemPropertyPlaybackDuration getMPMediaItemPropertyPlaybackDuration()
#define MPNowPlayingInfoPropertyElapsedPlaybackTime getMPNowPlayingInfoPropertyElapsedPlaybackTime()
#define MPNowPlayingInfoPropertyPlaybackRate getMPNowPlayingInfoPropertyPlaybackRate()
#define MPVolumeViewWirelessRoutesAvailableDidChangeNotification getMPVolumeViewWirelessRoutesAvailableDidChangeNotification()

WEBCORE_EXPORT NSString* WebUIApplicationWillResignActiveNotification = @"WebUIApplicationWillResignActiveNotification";
WEBCORE_EXPORT NSString* WebUIApplicationWillEnterForegroundNotification = @"WebUIApplicationWillEnterForegroundNotification";
WEBCORE_EXPORT NSString* WebUIApplicationDidBecomeActiveNotification = @"WebUIApplicationDidBecomeActiveNotification";
WEBCORE_EXPORT NSString* WebUIApplicationDidEnterBackgroundNotification = @"WebUIApplicationDidEnterBackgroundNotification";

using namespace WebCore;

@interface WebMediaSessionHelper : NSObject {
    MediaSessionManageriOS* _callback;
    RetainPtr<MPVolumeView> _volumeView;
    RetainPtr<MPAVRoutingController> _airPlayPresenceRoutingController;
}

- (id)initWithCallback:(MediaSessionManageriOS*)callback;
- (void)allocateVolumeView;
- (void)setVolumeView:(RetainPtr<MPVolumeView>)volumeView;
- (void)clearCallback;
- (void)interruption:(NSNotification *)notification;
- (void)applicationWillEnterForeground:(NSNotification *)notification;
- (void)applicationWillResignActive:(NSNotification *)notification;
- (void)applicationDidEnterBackground:(NSNotification *)notification;
- (BOOL)hasWirelessTargetsAvailable;
- (void)startMonitoringAirPlayRoutes;
- (void)stopMonitoringAirPlayRoutes;
@end

namespace WebCore {

static MediaSessionManageriOS* platformMediaSessionManager = nullptr;

PlatformMediaSessionManager& PlatformMediaSessionManager::sharedManager()
{
    if (!platformMediaSessionManager)
        platformMediaSessionManager = new MediaSessionManageriOS;
    return *platformMediaSessionManager;
}

PlatformMediaSessionManager* PlatformMediaSessionManager::sharedManagerIfExists()
{
    return platformMediaSessionManager;
}

MediaSessionManageriOS::MediaSessionManageriOS()
    : PlatformMediaSessionManager()
    , m_objcObserver(adoptNS([[WebMediaSessionHelper alloc] initWithCallback:this]))
{
    resetRestrictions();
}

MediaSessionManageriOS::~MediaSessionManageriOS()
{
    [m_objcObserver clearCallback];
}

void MediaSessionManageriOS::resetRestrictions()
{
    static const size_t systemMemoryRequiredForVideoInBackgroundTabs = 1024 * 1024 * 1024;

    LOG(Media, "MediaSessionManageriOS::resetRestrictions");

    PlatformMediaSessionManager::resetRestrictions();

    if (ramSize() < systemMemoryRequiredForVideoInBackgroundTabs) {
        LOG(Media, "MediaSessionManageriOS::resetRestrictions - restricting video in background tabs because system memory = %zul", ramSize());
        addRestriction(PlatformMediaSession::Video, BackgroundTabPlaybackRestricted);
    }

    removeRestriction(PlatformMediaSession::Video, ConcurrentPlaybackNotPermitted);
    addRestriction(PlatformMediaSession::Video, BackgroundProcessPlaybackRestricted);

    addRestriction(PlatformMediaSession::VideoAudio, ConcurrentPlaybackNotPermitted);
    addRestriction(PlatformMediaSession::VideoAudio, BackgroundProcessPlaybackRestricted);

    removeRestriction(PlatformMediaSession::Audio, ConcurrentPlaybackNotPermitted);
    removeRestriction(PlatformMediaSession::Audio, BackgroundProcessPlaybackRestricted);

    removeRestriction(PlatformMediaSession::WebAudio, ConcurrentPlaybackNotPermitted);
    removeRestriction(PlatformMediaSession::WebAudio, BackgroundProcessPlaybackRestricted);
}

bool MediaSessionManageriOS::hasWirelessTargetsAvailable()
{
    return [m_objcObserver hasWirelessTargetsAvailable];
}

void MediaSessionManageriOS::configureWireLessTargetMonitoring()
{
    bool requiresMonitoring = anyOfSessions([] (PlatformMediaSession& session, size_t) {
        return session.requiresPlaybackTargetRouteMonitoring();
    });

    LOG(Media, "MediaSessionManageriOS::configureWireLessTargetMonitoring - requiresMonitoring = %s", requiresMonitoring ? "true" : "false");

    if (requiresMonitoring)
        [m_objcObserver startMonitoringAirPlayRoutes];
    else
        [m_objcObserver stopMonitoringAirPlayRoutes];
}

bool MediaSessionManageriOS::sessionWillBeginPlayback(PlatformMediaSession& session)
{
    if (!PlatformMediaSessionManager::sessionWillBeginPlayback(session))
        return false;

    LOG(Media, "MediaSessionManageriOS::sessionWillBeginPlayback");
    updateNowPlayingInfo();
    return true;
}

void MediaSessionManageriOS::removeSession(PlatformMediaSession& session)
{
    PlatformMediaSessionManager::removeSession(session);
    LOG(Media, "MediaSessionManageriOS::removeSession");
    updateNowPlayingInfo();
}

void MediaSessionManageriOS::sessionWillEndPlayback(PlatformMediaSession& session)
{
    PlatformMediaSessionManager::sessionWillEndPlayback(session);
    LOG(Media, "MediaSessionManageriOS::sessionWillEndPlayback");
    updateNowPlayingInfo();
}

void MediaSessionManageriOS::clientCharacteristicsChanged(PlatformMediaSession&)
{
    LOG(Media, "MediaSessionManageriOS::clientCharacteristicsChanged");
    updateNowPlayingInfo();
}

PlatformMediaSession* MediaSessionManageriOS::nowPlayingEligibleSession()
{
    return findSession([] (PlatformMediaSession& session, size_t) {
        PlatformMediaSession::MediaType type = session.mediaType();
        if (type != PlatformMediaSession::VideoAudio && type != PlatformMediaSession::Audio)
            return false;

        if (session.characteristics() & PlatformMediaSession::HasAudio)
            return true;

        return false;
    });
}

void MediaSessionManageriOS::updateNowPlayingInfo()
{
    MPNowPlayingInfoCenter *nowPlaying = (MPNowPlayingInfoCenter *)[getMPNowPlayingInfoCenterClass() defaultCenter];
    const PlatformMediaSession* currentSession = this->nowPlayingEligibleSession();

    LOG(Media, "MediaSessionManageriOS::updateNowPlayingInfo - currentSession = %p", currentSession);

    if (!currentSession) {
        if (m_nowPlayingActive) {
            LOG(Media, "MediaSessionManageriOS::updateNowPlayingInfo - clearing now playing info");
            [nowPlaying setNowPlayingInfo:nil];
            m_nowPlayingActive = false;
        }

        return;
    }

    String title = currentSession->title();
    double duration = currentSession->duration();
    double rate = currentSession->state() == PlatformMediaSession::Playing ? 1 : 0;
    double currentTime = currentSession->currentTime();
    if (m_reportedTitle == title && m_reportedRate == rate && m_reportedDuration == duration) {
        LOG(Media, "MediaSessionManageriOS::updateNowPlayingInfo - nothing new to show");
        return;
    }

    m_reportedRate = rate;
    m_reportedDuration = duration;
    m_reportedTitle = title;
    m_reportedCurrentTime = currentTime;

    auto info = adoptNS([[NSMutableDictionary alloc] init]);
    if (!title.isEmpty())
        info.get()[MPMediaItemPropertyTitle] = static_cast<NSString *>(title);
    if (std::isfinite(duration) && duration != MediaPlayer::invalidTime())
        info.get()[MPMediaItemPropertyPlaybackDuration] = @(duration);
    info.get()[MPNowPlayingInfoPropertyPlaybackRate] = @(rate);

    if (std::isfinite(currentTime) && currentTime != MediaPlayer::invalidTime())
        info.get()[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(currentTime);

    LOG(Media, "MediaSessionManageriOS::updateNowPlayingInfo - title = \"%s\", rate = %f, duration = %f, now = %f",
        title.utf8().data(), rate, duration, currentTime);

    m_nowPlayingActive = true;
    [nowPlaying setNowPlayingInfo:info.get()];
}

bool MediaSessionManageriOS::sessionCanLoadMedia(const PlatformMediaSession& session) const
{
    if (session.displayType() == PlatformMediaSession::Optimized)
        return true;

    return PlatformMediaSessionManager::sessionCanLoadMedia(session);
}

void MediaSessionManageriOS::externalOutputDeviceAvailableDidChange()
{
    forEachSession([haveTargets = [m_objcObserver hasWirelessTargetsAvailable]] (PlatformMediaSession& session, size_t) {
        session.externalOutputDeviceAvailableDidChange(haveTargets);
    });
}

void MediaSessionManageriOS::applicationDidEnterBackground(bool isSuspendedUnderLock)
{
    LOG(Media, "MediaSessionManageriOS::applicationDidEnterBackground");

    if (m_isInBackground)
        return;
    m_isInBackground = true;

    if (!isSuspendedUnderLock)
        return;

    forEachSession([this] (PlatformMediaSession& session, size_t) {
        if (restrictions(session.mediaType()) & BackgroundProcessPlaybackRestricted)
            session.beginInterruption(PlatformMediaSession::SuspendedUnderLock);
    });
}

void MediaSessionManageriOS::applicationWillEnterForeground(bool isSuspendedUnderLock)
{
    LOG(Media, "MediaSessionManageriOS::applicationWillEnterForeground");

    if (!m_isInBackground)
        return;
    m_isInBackground = false;

    if (!isSuspendedUnderLock)
        return;

    forEachSession([this] (PlatformMediaSession& session, size_t) {
        if (restrictions(session.mediaType()) & BackgroundProcessPlaybackRestricted)
            session.endInterruption(PlatformMediaSession::MayResumePlaying);
    });
}

} // namespace WebCore

@implementation WebMediaSessionHelper

- (void)allocateVolumeView
{
    if (pthread_main_np()) {
        [self setVolumeView:adoptNS([allocMPVolumeViewInstance() init])];
        return;
    }

    RetainPtr<WebMediaSessionHelper> strongSelf = self;
    dispatch_async(dispatch_get_main_queue(), [strongSelf]() {
        RetainPtr<MPVolumeView> volumeView = adoptNS([allocMPVolumeViewInstance() init]);
        callOnWebThreadOrDispatchAsyncOnMainThread([strongSelf, volumeView]() {
            [strongSelf setVolumeView:volumeView];
        });
    });
}

- (void)setVolumeView:(RetainPtr<MPVolumeView>)volumeView
{
    if (_volumeView)
        [[NSNotificationCenter defaultCenter] removeObserver:self name:MPVolumeViewWirelessRoutesAvailableDidChangeNotification object:_volumeView.get()];

    _volumeView = volumeView;

    if (_volumeView)
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(wirelessRoutesAvailableDidChange:) name:MPVolumeViewWirelessRoutesAvailableDidChangeNotification object:_volumeView.get()];
}

- (id)initWithCallback:(MediaSessionManageriOS*)callback
{
    LOG(Media, "-[WebMediaSessionHelper initWithCallback]");

    if (!(self = [super init]))
        return nil;
    
    _callback = callback;

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(interruption:) name:AVAudioSessionInterruptionNotification object:[AVAudioSession sharedInstance]];

    [center addObserver:self selector:@selector(applicationWillEnterForeground:) name:UIApplicationWillEnterForegroundNotification object:nil];
    [center addObserver:self selector:@selector(applicationWillEnterForeground:) name:WebUIApplicationWillEnterForegroundNotification object:nil];
    [center addObserver:self selector:@selector(applicationDidBecomeActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
    [center addObserver:self selector:@selector(applicationDidBecomeActive:) name:WebUIApplicationDidBecomeActiveNotification object:nil];
    [center addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:nil];
    [center addObserver:self selector:@selector(applicationWillResignActive:) name:WebUIApplicationWillResignActiveNotification object:nil];
    [center addObserver:self selector:@selector(applicationDidEnterBackground:) name:UIApplicationDidEnterBackgroundNotification object:nil];
    [center addObserver:self selector:@selector(applicationDidEnterBackground:) name:WebUIApplicationDidEnterBackgroundNotification object:nil];

    [self allocateVolumeView];

    // Now playing won't work unless we turn on the delivery of remote control events.
    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
    
    return self;
}

- (void)dealloc
{
    LOG(Media, "-[WebMediaSessionHelper dealloc]");

    if (!isMainThread()) {
        callOnMainThread([volumeView = WTFMove(_volumeView), routingController = WTFMove(_airPlayPresenceRoutingController)] () mutable {
            LOG(Media, "-[WebMediaSessionHelper dealloc] - dipatched to MainThread");

            volumeView.clear();

            if (!routingController)
                return;

            [routingController setDiscoveryMode:MPRouteDiscoveryModeDisabled];
            routingController.clear();
        });
    }

    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)clearCallback
{
    LOG(Media, "-[WebMediaSessionHelper clearCallback]");
    _callback = nil;
}

- (BOOL)hasWirelessTargetsAvailable
{
    LOG(Media, "-[WebMediaSessionHelper hasWirelessTargetsAvailable]");
    return _volumeView ? [_volumeView areWirelessRoutesAvailable] : NO;
}

- (void)startMonitoringAirPlayRoutes
{
    if (_airPlayPresenceRoutingController)
        return;

    LOG(Media, "-[WebMediaSessionHelper startMonitoringAirPlayRoutes]");

    callOnMainThread([protectedSelf = RetainPtr<WebMediaSessionHelper>(self)] () {
        LOG(Media, "-[WebMediaSessionHelper startMonitoringAirPlayRoutes] - dipatched to MainThread");

        if (protectedSelf->_airPlayPresenceRoutingController)
            return;

        protectedSelf->_airPlayPresenceRoutingController = adoptNS([allocMPAVRoutingControllerInstance() initWithName:@"WebCore - HTML media element checking for AirPlay route presence"]);
        [protectedSelf->_airPlayPresenceRoutingController setDiscoveryMode:MPRouteDiscoveryModePresence];
    });
}

- (void)stopMonitoringAirPlayRoutes
{
    if (!_airPlayPresenceRoutingController)
        return;

    LOG(Media, "-[WebMediaSessionHelper stopMonitoringAirPlayRoutes]");

    callOnMainThread([protectedSelf = RetainPtr<WebMediaSessionHelper>(self)] () {
        LOG(Media, "-[WebMediaSessionHelper stopMonitoringAirPlayRoutes] - dipatched to MainThread");

        if (!protectedSelf->_airPlayPresenceRoutingController)
            return;

        [protectedSelf->_airPlayPresenceRoutingController setDiscoveryMode:MPRouteDiscoveryModeDisabled];
        protectedSelf->_airPlayPresenceRoutingController = nil;
    });
}

- (void)interruption:(NSNotification *)notification
{
    if (!_callback || _callback->willIgnoreSystemInterruptions())
        return;

    NSUInteger type = [[[notification userInfo] objectForKey:AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
    PlatformMediaSession::EndInterruptionFlags flags = PlatformMediaSession::NoFlags;

    LOG(Media, "-[WebMediaSessionHelper interruption] - type = %i", (int)type);

    if (type == AVAudioSessionInterruptionTypeEnded && [[[notification userInfo] objectForKey:AVAudioSessionInterruptionOptionKey] unsignedIntegerValue] == AVAudioSessionInterruptionOptionShouldResume)
        flags = PlatformMediaSession::MayResumePlaying;

    WebThreadRun(^{
        if (!_callback)
            return;

        if (type == AVAudioSessionInterruptionTypeBegan)
            _callback->beginInterruption(PlatformMediaSession::SystemInterruption);
        else
            _callback->endInterruption(flags);

    });
}

- (void)applicationWillEnterForeground:(NSNotification *)notification
{
    UNUSED_PARAM(notification);

    if (!_callback || _callback->willIgnoreSystemInterruptions())
        return;

    LOG(Media, "-[WebMediaSessionHelper applicationWillEnterForeground]");

    BOOL isSuspendedUnderLock = [[[notification userInfo] objectForKey:@"isSuspendedUnderLock"] boolValue];

    WebThreadRun(^{
        if (!_callback)
            return;

        _callback->applicationWillEnterForeground(isSuspendedUnderLock);
    });
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
    UNUSED_PARAM(notification);

    if (!_callback || _callback->willIgnoreSystemInterruptions())
        return;

    LOG(Media, "-[WebMediaSessionHelper applicationDidBecomeActive]");

    WebThreadRun(^{
        if (!_callback)
            return;

        _callback->applicationDidEnterForeground();
    });
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
    UNUSED_PARAM(notification);

    if (!_callback || _callback->willIgnoreSystemInterruptions())
        return;

    LOG(Media, "-[WebMediaSessionHelper applicationWillResignActive]");

    WebThreadRun(^{
        if (!_callback)
            return;

        _callback->applicationWillEnterBackground();
    });
}

- (void)wirelessRoutesAvailableDidChange:(NSNotification *)notification
{
    UNUSED_PARAM(notification);

    if (!_callback)
        return;

    LOG(Media, "-[WebMediaSessionHelper wirelessRoutesAvailableDidChange]");

    WebThreadRun(^{
        if (!_callback)
            return;

        _callback->externalOutputDeviceAvailableDidChange();
    });
}

- (void)applicationDidEnterBackground:(NSNotification *)notification
{
    if (!_callback || _callback->willIgnoreSystemInterruptions())
        return;

    LOG(Media, "-[WebMediaSessionHelper applicationDidEnterBackground]");

    BOOL isSuspendedUnderLock = [[[notification userInfo] objectForKey:@"isSuspendedUnderLock"] boolValue];

    WebThreadRun(^{
        if (!_callback)
            return;

        _callback->applicationDidEnterBackground(isSuspendedUnderLock);
    });
}
@end

#endif // PLATFORM(IOS)
