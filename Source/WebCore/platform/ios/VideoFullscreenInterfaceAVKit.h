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

#if PLATFORM(IOS)

#include "EventListener.h"
#include "HTMLMediaElementEnums.h"
#include "PlatformLayer.h"
#include "PlaybackSessionInterfaceAVKit.h"
#include "VideoFullscreenModel.h"
#include <objc/objc.h>
#include <wtf/Function.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/RunLoop.h>

OBJC_CLASS AVPlayerViewController;
OBJC_CLASS UIViewController;
OBJC_CLASS UIWindow;
OBJC_CLASS UIView;
OBJC_CLASS CALayer;
OBJC_CLASS WebAVPlayerController;
OBJC_CLASS WebAVPlayerLayerView;
OBJC_CLASS WebAVPlayerLayer;
OBJC_CLASS WebAVPlayerViewControllerDelegate;

namespace WTF {
class String;
}

namespace WebCore {
class IntRect;
class FloatSize;
class VideoFullscreenModel;
class VideoFullscreenChangeObserver;
    
class WEBCORE_EXPORT VideoFullscreenInterfaceAVKit final
    : public VideoFullscreenModelClient
    , public PlaybackSessionModelClient
    , public ThreadSafeRefCounted<VideoFullscreenInterfaceAVKit> {

public:
    static Ref<VideoFullscreenInterfaceAVKit> create(PlaybackSessionInterfaceAVKit&);
    virtual ~VideoFullscreenInterfaceAVKit();
    WEBCORE_EXPORT void setVideoFullscreenModel(VideoFullscreenModel*);
    WEBCORE_EXPORT void setVideoFullscreenChangeObserver(VideoFullscreenChangeObserver*);
    PlaybackSessionModel* playbackSessionModel() const { return m_playbackSessionInterface->playbackSessionModel(); }

    // VideoFullscreenModelClient
    WEBCORE_EXPORT void hasVideoChanged(bool) final;
    WEBCORE_EXPORT void videoDimensionsChanged(const FloatSize&) final;

    // PlaybackSessionModelClient
    WEBCORE_EXPORT void externalPlaybackChanged(bool enabled, PlaybackSessionModel::ExternalPlaybackTargetType, const String& localizedDeviceName) final;

    WEBCORE_EXPORT virtual void setupFullscreen(UIView&, const IntRect& initialRect, UIView *, HTMLMediaElementEnums::VideoFullscreenMode, bool allowsPictureInPicturePlayback);
    WEBCORE_EXPORT virtual void enterFullscreen();
    WEBCORE_EXPORT virtual void exitFullscreen(const IntRect& finalRect);
    WEBCORE_EXPORT virtual void cleanupFullscreen();
    WEBCORE_EXPORT virtual void invalidate();
    WEBCORE_EXPORT virtual void requestHideAndExitFullscreen();
    WEBCORE_EXPORT virtual void preparedToReturnToInline(bool visible, const IntRect& inlineRect);

    enum class ExitFullScreenReason {
        DoneButtonTapped,
        FullScreenButtonTapped,
        PinchGestureHandled,
        RemoteControlStopEventReceived,
        PictureInPictureStarted
    };

    VideoFullscreenModel* model() const { return m_videoFullscreenModel; }
    bool shouldExitFullscreenWithReason(ExitFullScreenReason);
    HTMLMediaElementEnums::VideoFullscreenMode mode() const { return m_mode; }
    bool allowsPictureInPicturePlayback() const { return m_allowsPictureInPicturePlayback; }
    WEBCORE_EXPORT bool mayAutomaticallyShowVideoPictureInPicture() const;
    void fullscreenMayReturnToInline(WTF::Function<void(bool)>&& callback);
    bool wirelessVideoPlaybackDisabled() const;
    void applicationDidBecomeActive();

    void willStartPictureInPicture();
    void didStartPictureInPicture();
    void failedToStartPictureInPicture();
    void willStopPictureInPicture();
    void didStopPictureInPicture();
    void prepareForPictureInPictureStopWithCompletionHandler(void (^)(BOOL));

    void setMode(HTMLMediaElementEnums::VideoFullscreenMode);
    void clearMode(HTMLMediaElementEnums::VideoFullscreenMode);
    bool hasMode(HTMLMediaElementEnums::VideoFullscreenMode mode) const { return m_mode & mode; }
    bool isMode(HTMLMediaElementEnums::VideoFullscreenMode mode) const { return m_mode == mode; }

protected:
    WEBCORE_EXPORT VideoFullscreenInterfaceAVKit(PlaybackSessionInterfaceAVKit&);
    void beginSession();
    void enterPictureInPicture();
    void enterFullscreenStandard();
    void watchdogTimerFired();
    WebAVPlayerController *playerController() const;

    Ref<PlaybackSessionInterfaceAVKit> m_playbackSessionInterface;
    RetainPtr<WebAVPlayerViewControllerDelegate> m_playerViewControllerDelegate;
    RetainPtr<AVPlayerViewController> m_playerViewController;
    VideoFullscreenModel* m_videoFullscreenModel { nullptr };
    VideoFullscreenChangeObserver* m_fullscreenChangeObserver { nullptr };

    // These are only used when fullscreen is presented in a separate window.
    RetainPtr<UIWindow> m_window;
    RetainPtr<UIViewController> m_viewController;
    RetainPtr<UIView> m_parentView;
    RetainPtr<UIWindow> m_parentWindow;
    RetainPtr<WebAVPlayerLayerView> m_playerLayerView;
    HTMLMediaElementEnums::VideoFullscreenMode m_mode { HTMLMediaElementEnums::VideoFullscreenModeNone };
    WTF::Function<void(bool)> m_prepareToInlineCallback;
    RunLoop::Timer<VideoFullscreenInterfaceAVKit> m_watchdogTimer;
    bool m_allowsPictureInPicturePlayback { false };
    bool m_exitRequested { false };
    bool m_exitCompleted { false };
    bool m_enterRequested { false };
    bool m_wirelessVideoPlaybackDisabled { true };
    bool m_shouldReturnToFullscreenWhenStoppingPiP { false };
    bool m_shouldReturnToFullscreenAfterEnteringForeground { false };
    bool m_restoringFullscreenForPictureInPictureStop { false };

    void doEnterFullscreen();
};

}

#endif

