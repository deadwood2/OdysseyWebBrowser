/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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


#ifndef WebPlaybackSessionInterfaceAVKit_h
#define WebPlaybackSessionInterfaceAVKit_h

#if PLATFORM(IOS)

#include "EventListener.h"
#include "HTMLMediaElementEnums.h"
#include "Timer.h"
#include "WebPlaybackSessionInterface.h"
#include "WebPlaybackSessionModel.h"
#include <functional>
#include <objc/objc.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

OBJC_CLASS WebAVPlayerController;
OBJC_CLASS AVPlayerViewController;
OBJC_CLASS UIViewController;
OBJC_CLASS UIWindow;
OBJC_CLASS UIView;

namespace WTF {
class String;
}

namespace WebCore {
class IntRect;
class WebPlaybackSessionModel;
class WebPlaybackSessionChangeObserver;

class WEBCORE_EXPORT WebPlaybackSessionInterfaceAVKit
    : public WebPlaybackSessionInterface
    , public WebPlaybackSessionModelClient
    , public RefCounted<WebPlaybackSessionInterfaceAVKit> {

public:
    static Ref<WebPlaybackSessionInterfaceAVKit> create(WebPlaybackSessionModel& model)
    {
        return adoptRef(*new WebPlaybackSessionInterfaceAVKit(model));
    }
    virtual ~WebPlaybackSessionInterfaceAVKit();
    WebPlaybackSessionModel* webPlaybackSessionModel() const { return m_playbackSessionModel; }

    // WebPlaybackSessionInterface
    WEBCORE_EXPORT void resetMediaState() override;

    // WebPlaybackSessionModelClient
    WEBCORE_EXPORT void durationChanged(double) override;
    WEBCORE_EXPORT void currentTimeChanged(double currentTime, double anchorTime) override;
    WEBCORE_EXPORT void bufferedTimeChanged(double) override;
    WEBCORE_EXPORT void rateChanged(bool isPlaying, float playbackRate) override;
    WEBCORE_EXPORT void seekableRangesChanged(const TimeRanges&) override;
    WEBCORE_EXPORT void canPlayFastReverseChanged(bool) override;
    WEBCORE_EXPORT void audioMediaSelectionOptionsChanged(const Vector<String>& options, uint64_t selectedIndex) override;
    WEBCORE_EXPORT void legibleMediaSelectionOptionsChanged(const Vector<String>& options, uint64_t selectedIndex) override;
    WEBCORE_EXPORT void externalPlaybackChanged(bool enabled, WebPlaybackSessionModel::ExternalPlaybackTargetType, const String& localizedDeviceName) override;
    WEBCORE_EXPORT void wirelessVideoPlaybackDisabledChanged(bool) override;

    WEBCORE_EXPORT virtual void invalidate();

    WebAVPlayerController *playerController() const { return m_playerController.get(); }

protected:
    WEBCORE_EXPORT WebPlaybackSessionInterfaceAVKit(WebPlaybackSessionModel&);

    RetainPtr<WebAVPlayerController> m_playerController;
    WebPlaybackSessionModel* m_playbackSessionModel { nullptr };
};

}

#endif

#endif
