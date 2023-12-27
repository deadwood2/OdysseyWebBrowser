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


#ifndef WebVideoFullscreenInterfaceAVKit_h
#define WebVideoFullscreenInterfaceAVKit_h

#if PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 80000

#include <WebCore/EventListener.h>
#include <WebCore/HTMLMediaElement.h>
#include <WebCore/PlatformLayer.h>
#include <WebCore/WebVideoFullscreenInterface.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/ThreadSafeRefCounted.h>

OBJC_CLASS WebAVPlayerController;
OBJC_CLASS AVPlayerViewController;
OBJC_CLASS UIViewController;
OBJC_CLASS UIWindow;
OBJC_CLASS UIView;
OBJC_CLASS CALayer;
OBJC_CLASS WebAVVideoLayer;
OBJC_CLASS WebCALayerHostWrapper;

namespace WTF {
class String;
}

namespace WebCore {
class IntRect;
class WebVideoFullscreenModel;
    
class WebVideoFullscreenChangeObserver {
public:
    virtual ~WebVideoFullscreenChangeObserver() { };
    virtual void didSetupFullscreen() = 0;
    virtual void didEnterFullscreen() = 0;
    virtual void didExitFullscreen() = 0;
    virtual void didCleanupFullscreen() = 0;
    virtual void fullscreenMayReturnToInline() = 0;
};

class WEBCORE_EXPORT WebVideoFullscreenInterfaceAVKit
    : public WebVideoFullscreenInterface
    , public ThreadSafeRefCounted<WebVideoFullscreenInterfaceAVKit> {

public:
    static Ref<WebVideoFullscreenInterfaceAVKit> create()
    {
        return adoptRef(*new WebVideoFullscreenInterfaceAVKit());
    }
    virtual ~WebVideoFullscreenInterfaceAVKit() { }
    WEBCORE_EXPORT void setWebVideoFullscreenModel(WebVideoFullscreenModel*);
    WEBCORE_EXPORT void setWebVideoFullscreenChangeObserver(WebVideoFullscreenChangeObserver*);
    
    WEBCORE_EXPORT virtual void resetMediaState() override;
    WEBCORE_EXPORT virtual void setDuration(double) override;
    WEBCORE_EXPORT virtual void setCurrentTime(double currentTime, double anchorTime) override;
    WEBCORE_EXPORT virtual void setBufferedTime(double bufferedTime) override;
    WEBCORE_EXPORT virtual void setRate(bool isPlaying, float playbackRate) override;
    WEBCORE_EXPORT virtual void setVideoDimensions(bool hasVideo, float width, float height) override;
    WEBCORE_EXPORT virtual void setSeekableRanges(const TimeRanges&) override;
    WEBCORE_EXPORT virtual void setCanPlayFastReverse(bool) override;
    WEBCORE_EXPORT virtual void setAudioMediaSelectionOptions(const Vector<WTF::String>& options, uint64_t selectedIndex) override;
    WEBCORE_EXPORT virtual void setLegibleMediaSelectionOptions(const Vector<WTF::String>& options, uint64_t selectedIndex) override;
    WEBCORE_EXPORT virtual void setExternalPlayback(bool enabled, ExternalPlaybackTargetType, WTF::String localizedDeviceName) override;
    
    WEBCORE_EXPORT virtual void setupFullscreen(PlatformLayer&, const IntRect& initialRect, UIView *, HTMLMediaElement::VideoFullscreenMode, bool allowOptimizedFullscreen);
    WEBCORE_EXPORT virtual void enterFullscreen();
    WEBCORE_EXPORT virtual void exitFullscreen(const IntRect& finalRect);
    WEBCORE_EXPORT virtual void cleanupFullscreen();
    WEBCORE_EXPORT virtual void invalidate();
    WEBCORE_EXPORT virtual void requestHideAndExitFullscreen();
    WEBCORE_EXPORT virtual void preparedToReturnToInline(bool visible, const IntRect& inlineRect);

    HTMLMediaElement::VideoFullscreenMode mode() const { return m_mode; }
    bool allowOptimizedFullscreen() const { return m_allowOptimizedFullscreen; }
    void setIsOptimized(bool);
    WEBCORE_EXPORT bool mayAutomaticallyShowVideoOptimized() const;
    void fullscreenMayReturnToInline(std::function<void(bool)> callback);

    void willStartOptimizedFullscreen();
    void didStartOptimizedFullscreen();
    void willStopOptimizedFullscreen();
    void didStopOptimizedFullscreen();
    void willCancelOptimizedFullscreen();
    void didCancelOptimizedFullscreen();
    void prepareForOptimizedFullscreenStopWithCompletionHandler(void (^)(BOOL));

    void setMode(HTMLMediaElement::VideoFullscreenMode);
    void clearMode(HTMLMediaElement::VideoFullscreenMode);
    bool hasMode(HTMLMediaElement::VideoFullscreenMode mode) const { return m_mode & mode; }
    bool isMode(HTMLMediaElement::VideoFullscreenMode mode) const { return m_mode == mode; }

protected:
    WEBCORE_EXPORT WebVideoFullscreenInterfaceAVKit();
    void beginSession();
    void setupFullscreenInternal(PlatformLayer&, const IntRect& initialRect, UIView *, HTMLMediaElement::VideoFullscreenMode, bool allowOptimizedFullscreen);
    void enterFullscreenOptimized();
    void enterFullscreenStandard();
    void exitFullscreenInternal(const IntRect& finalRect);
    void cleanupFullscreenInternal();

    RetainPtr<WebAVPlayerController> m_playerController;
    RetainPtr<AVPlayerViewController> m_playerViewController;
    RetainPtr<CALayer> m_videoLayer;
    RetainPtr<WebAVVideoLayer> m_videoLayerContainer;
    RetainPtr<WebCALayerHostWrapper> m_layerHostWrapper;
    WebVideoFullscreenModel* m_videoFullscreenModel { nullptr };
    WebVideoFullscreenChangeObserver* m_fullscreenChangeObserver { nullptr };

    // These are only used when fullscreen is presented in a separate window.
    RetainPtr<UIWindow> m_window;
    RetainPtr<UIViewController> m_viewController;
    RetainPtr<UIView> m_parentView;
    RetainPtr<UIWindow> m_parentWindow;
    HTMLMediaElement::VideoFullscreenMode m_mode { HTMLMediaElement::VideoFullscreenModeNone };
    std::function<void(bool)> m_prepareToInlineCallback;
    bool m_allowOptimizedFullscreen { false };
    bool m_exitRequested { false };
    bool m_exitCompleted { false };
    bool m_enterRequested { false };

    void doEnterFullscreen();
};

}

#endif

#endif
