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

#ifndef WebVideoFullscreenInterfaceMac_h
#define WebVideoFullscreenInterfaceMac_h

#if PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE)

#include <WebCore/HTMLMediaElementEnums.h>
#include <WebCore/WebVideoFullscreenInterface.h>
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class IntRect;
class WebVideoFullscreenChangeObserver;
class WebVideoFullscreenModel;

class WEBCORE_EXPORT WebVideoFullscreenInterfaceMac
    : public WebVideoFullscreenInterface
    , public RefCounted<WebVideoFullscreenInterfaceMac> {

public:
    static Ref<WebVideoFullscreenInterfaceMac> create()
    {
        return adoptRef(*new WebVideoFullscreenInterfaceMac());
    }
    virtual ~WebVideoFullscreenInterfaceMac();
    WEBCORE_EXPORT void setWebVideoFullscreenModel(WebVideoFullscreenModel*);
    WEBCORE_EXPORT void setWebVideoFullscreenChangeObserver(WebVideoFullscreenChangeObserver*);

    WEBCORE_EXPORT void resetMediaState() override { }
    WEBCORE_EXPORT void setDuration(double) override { }
    WEBCORE_EXPORT void setCurrentTime(double /*currentTime*/, double /*anchorTime*/) override { }
    WEBCORE_EXPORT void setBufferedTime(double) override { }
    WEBCORE_EXPORT void setRate(bool /*isPlaying*/, float /*playbackRate*/) override { }
    WEBCORE_EXPORT void setVideoDimensions(bool /*hasVideo*/, float /*width*/, float /*height*/) override { }
    WEBCORE_EXPORT void setSeekableRanges(const TimeRanges&) override { }
    WEBCORE_EXPORT void setCanPlayFastReverse(bool) override { }
    WEBCORE_EXPORT void setAudioMediaSelectionOptions(const Vector<WTF::String>& /*options*/, uint64_t /*selectedIndex*/) override { }
    WEBCORE_EXPORT void setLegibleMediaSelectionOptions(const Vector<WTF::String>& /*options*/, uint64_t /*selectedIndex*/) override { }
    WEBCORE_EXPORT void setExternalPlayback(bool /*enabled*/, ExternalPlaybackTargetType, WTF::String /*localizedDeviceName*/) override { }
    WEBCORE_EXPORT void setWirelessVideoPlaybackDisabled(bool) override { }

    WEBCORE_EXPORT void setupFullscreen(NSView& /*layerHostedView*/, const IntRect& /*initialRect*/, HTMLMediaElementEnums::VideoFullscreenMode, bool /*allowsPictureInPicturePlayback*/) { }
    WEBCORE_EXPORT void enterFullscreen() { }
    WEBCORE_EXPORT void exitFullscreen(const IntRect& /*finalRect*/) { }
    WEBCORE_EXPORT void cleanupFullscreen() { }
    WEBCORE_EXPORT void invalidate() { }
    WEBCORE_EXPORT void requestHideAndExitFullscreen() { }
    WEBCORE_EXPORT void preparedToReturnToInline(bool /*visible*/, const IntRect& /*inlineRect*/) { }

    HTMLMediaElementEnums::VideoFullscreenMode mode() const { return m_mode; }
    bool hasMode(HTMLMediaElementEnums::VideoFullscreenMode mode) const { return m_mode & mode; }
    bool isMode(HTMLMediaElementEnums::VideoFullscreenMode mode) const { return m_mode == mode; }
    void setMode(HTMLMediaElementEnums::VideoFullscreenMode);
    void clearMode(HTMLMediaElementEnums::VideoFullscreenMode);

    WEBCORE_EXPORT bool mayAutomaticallyShowVideoPictureInPicture() const { return false; }
    void applicationDidBecomeActive() { }

protected:
    WebVideoFullscreenModel* m_videoFullscreenModel { nullptr };
    WebVideoFullscreenChangeObserver* m_fullscreenChangeObserver { nullptr };
    HTMLMediaElementEnums::VideoFullscreenMode m_mode { HTMLMediaElementEnums::VideoFullscreenModeNone };
};

}

#endif

#endif /* WebVideoFullscreenInterfaceMac_h */
