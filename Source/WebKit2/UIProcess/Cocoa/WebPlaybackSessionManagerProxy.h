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

#ifndef WebPlaybackSessionManagerProxy_h
#define WebPlaybackSessionManagerProxy_h
#if PLATFORM(IOS) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))

#include "MessageReceiver.h"
#include <WebCore/GraphicsLayer.h>
#include <WebCore/PlatformView.h>
#include <WebCore/TimeRanges.h>
#include <WebCore/WebPlaybackSessionModel.h>
#include <wtf/HashCountedSet.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if PLATFORM(IOS)
#include <WebCore/WebPlaybackSessionInterfaceAVKit.h>
#else
#include <WebCore/WebPlaybackSessionInterfaceMac.h>
#endif

#if PLATFORM(IOS)
typedef WebCore::WebPlaybackSessionInterfaceAVKit PlatformWebPlaybackSessionInterface;
#else
typedef WebCore::WebPlaybackSessionInterfaceMac PlatformWebPlaybackSessionInterface;
#endif

namespace WebKit {

class WebPageProxy;
class WebPlaybackSessionManagerProxy;

class WebPlaybackSessionModelContext final: public RefCounted<WebPlaybackSessionModelContext>, public WebCore::WebPlaybackSessionModel  {
public:
    static Ref<WebPlaybackSessionModelContext> create(WebPlaybackSessionManagerProxy& manager, uint64_t contextId)
    {
        return adoptRef(*new WebPlaybackSessionModelContext(manager, contextId));
    }
    virtual ~WebPlaybackSessionModelContext() { }

    void invalidate() { m_manager = nullptr; }

    // WebPlaybackSessionModel
    void addClient(WebCore::WebPlaybackSessionModelClient&) final;
    void removeClient(WebCore::WebPlaybackSessionModelClient&)final;

    void setDuration(double);
    void setCurrentTime(double);
    void setBufferedTime(double);
    void setPlaybackStartedTime(double);
    void setRate(bool isPlaying, float playbackRate);
    void setSeekableRanges(WebCore::TimeRanges&);
    void setCanPlayFastReverse(bool);
    void setAudioMediaSelectionOptions(const Vector<WTF::String>& options, uint64_t index);
    void setLegibleMediaSelectionOptions(const Vector<WTF::String>& options, uint64_t index);
    void setExternalPlayback(bool, WebPlaybackSessionModel::ExternalPlaybackTargetType, const String&);
    void setWirelessVideoPlaybackDisabled(bool);

private:
    friend class WebVideoFullscreenModelContext;

    WebPlaybackSessionModelContext(WebPlaybackSessionManagerProxy& manager, uint64_t contextId)
        : m_manager(&manager)
        , m_contextId(contextId)
    {
    }

    // WebPlaybackSessionModel
    void play() final;
    void pause() final;
    void togglePlayState() final;
    void beginScrubbing() final;
    void endScrubbing() final;
    void seekToTime(double) final;
    void fastSeek(double time) final;
    void beginScanningForward() final;
    void beginScanningBackward() final;
    void endScanning() final;
    void selectAudioMediaOption(uint64_t) final;
    void selectLegibleMediaOption(uint64_t) final;

    double playbackStartedTime() const final { return m_playbackStartedTime; }
    double duration() const final { return m_duration; }
    double currentTime() const final { return m_currentTime; }
    double bufferedTime() const final { return m_bufferedTime; }
    bool isPlaying() const final { return m_isPlaying; }
    bool isScrubbing() const final { return m_isScrubbing; }
    float playbackRate() const final { return m_playbackRate; }
    Ref<WebCore::TimeRanges> seekableRanges() const final { return m_seekableRanges.copyRef(); }
    bool canPlayFastReverse() const final { return m_canPlayFastReverse; }
    Vector<WTF::String> audioMediaSelectionOptions() const final { return m_audioMediaSelectionOptions; }
    uint64_t audioMediaSelectedIndex() const final { return m_audioMediaSelectedIndex; }
    Vector<WTF::String> legibleMediaSelectionOptions() const final { return m_legibleMediaSelectionOptions; }
    uint64_t legibleMediaSelectedIndex() const final { return m_legibleMediaSelectedIndex; }
    bool externalPlaybackEnabled() const final { return m_externalPlaybackEnabled; }
    WebPlaybackSessionModel::ExternalPlaybackTargetType externalPlaybackTargetType() const final { return m_externalPlaybackTargetType; }
    String externalPlaybackLocalizedDeviceName() const final { return m_externalPlaybackLocalizedDeviceName; }
    bool wirelessVideoPlaybackDisabled() const final { return m_wirelessVideoPlaybackDisabled; }

    WebPlaybackSessionManagerProxy* m_manager;
    uint64_t m_contextId;
    HashSet<WebCore::WebPlaybackSessionModelClient*> m_clients;
    double m_playbackStartedTime { 0 };
    bool m_playbackStartedTimeNeedsUpdate { false };
    double m_duration { 0 };
    double m_currentTime { 0 };
    double m_bufferedTime { 0 };
    bool m_isPlaying { false };
    bool m_isScrubbing { false };
    float m_playbackRate { 0 };
    Ref<WebCore::TimeRanges> m_seekableRanges { WebCore::TimeRanges::create() };
    bool m_canPlayFastReverse { false };
    Vector<WTF::String> m_audioMediaSelectionOptions;
    uint64_t m_audioMediaSelectedIndex { 0 };
    Vector<WTF::String> m_legibleMediaSelectionOptions;
    uint64_t m_legibleMediaSelectedIndex { 0 };
    bool m_externalPlaybackEnabled { false };
    WebPlaybackSessionModel::ExternalPlaybackTargetType m_externalPlaybackTargetType { WebPlaybackSessionModel::TargetTypeNone };
    String m_externalPlaybackLocalizedDeviceName;
    bool m_wirelessVideoPlaybackDisabled { false };
};

class WebPlaybackSessionManagerProxy : public RefCounted<WebPlaybackSessionManagerProxy>, private IPC::MessageReceiver {
public:
    static RefPtr<WebPlaybackSessionManagerProxy> create(WebPageProxy&);
    virtual ~WebPlaybackSessionManagerProxy();

    void invalidate();

    PlatformWebPlaybackSessionInterface* controlsManagerInterface();
    void requestControlledElementID();

private:
    friend class WebPlaybackSessionModelContext;
    friend class WebVideoFullscreenManagerProxy;

    explicit WebPlaybackSessionManagerProxy(WebPageProxy&);
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) final;

    typedef std::tuple<RefPtr<WebPlaybackSessionModelContext>, RefPtr<PlatformWebPlaybackSessionInterface>> ModelInterfaceTuple;
    ModelInterfaceTuple createModelAndInterface(uint64_t contextId);
    ModelInterfaceTuple& ensureModelAndInterface(uint64_t contextId);
    WebPlaybackSessionModelContext& ensureModel(uint64_t contextId);
    PlatformWebPlaybackSessionInterface& ensureInterface(uint64_t contextId);
    void addClientForContext(uint64_t contextId);
    void removeClientForContext(uint64_t contextId);

    // Messages from WebPlaybackSessionManager
    void setUpPlaybackControlsManagerWithID(uint64_t contextId);
    void clearPlaybackControlsManager();
    void resetMediaState(uint64_t contextId);
    void setCurrentTime(uint64_t contextId, double currentTime, double hostTime);
    void setBufferedTime(uint64_t contextId, double bufferedTime);
    void setSeekableRangesVector(uint64_t contextId, Vector<std::pair<double, double>> ranges);
    void setCanPlayFastReverse(uint64_t contextId, bool value);
    void setAudioMediaSelectionOptions(uint64_t contextId, Vector<String> options, uint64_t selectedIndex);
    void setLegibleMediaSelectionOptions(uint64_t contextId, Vector<String> options, uint64_t selectedIndex);
    void setExternalPlaybackProperties(uint64_t contextId, bool enabled, uint32_t targetType, String localizedDeviceName);
    void setWirelessVideoPlaybackDisabled(uint64_t contextId, bool);
    void setDuration(uint64_t contextId, double duration);
    void setPlaybackStartedTime(uint64_t contextId, double playbackStartedTime);
    void setRate(uint64_t contextId, bool isPlaying, double rate);
    void handleControlledElementIDResponse(uint64_t, String) const;

    // Messages to WebPlaybackSessionManager
    void play(uint64_t contextId);
    void pause(uint64_t contextId);
    void togglePlayState(uint64_t contextId);
    void beginScrubbing(uint64_t contextId);
    void endScrubbing(uint64_t contextId);
    void seekToTime(uint64_t contextId, double time);
    void fastSeek(uint64_t contextId, double time);
    void beginScanningForward(uint64_t contextId);
    void beginScanningBackward(uint64_t contextId);
    void endScanning(uint64_t contextId);
    void selectAudioMediaOption(uint64_t contextId, uint64_t index);
    void selectLegibleMediaOption(uint64_t contextId, uint64_t index);

    WebPageProxy* m_page;
    HashMap<uint64_t, ModelInterfaceTuple> m_contextMap;
    uint64_t m_controlsManagerContextId { 0 };
    HashCountedSet<uint64_t> m_clientCounts;
};

} // namespace WebKit

#endif // PLATFORM(IOS) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))

#endif // WebPlaybackSessionManagerProxy_h
