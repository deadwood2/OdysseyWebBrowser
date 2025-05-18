#pragma once

#include "config.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

#include "MediaSourcePrivate.h"
#include "MediaSourceBufferPrivateMorphOS.h"
#include <wtf/MediaTime.h>
#include <wtf/RunLoop.h>

struct Window;

namespace WebCore {

class GraphicsContext;
class FloatRect;
class MediaPlayerPrivateMorphOS;

class MediaSourcePrivateMorphOS final : public MediaSourcePrivate {
public:
    static Ref<MediaSourcePrivateMorphOS> create(MediaPlayerPrivateMorphOS&, MediaSourcePrivateClient&, const String &url);
    virtual ~MediaSourcePrivateMorphOS();

private:
    MediaSourcePrivateMorphOS(MediaPlayerPrivateMorphOS&, MediaSourcePrivateClient&, const String &url);

public:
    // MediaSourcePrivate Overrides
    AddStatus addSourceBuffer(const ContentType&, bool webMParserEnabled, RefPtr<SourceBufferPrivate>&) override;
    void durationChanged(const MediaTime&) override;
    void markEndOfStream(EndOfStreamStatus) override;
    void unmarkEndOfStream() override;
    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;
    void waitForSeekCompleted() override;
    void seekCompleted() override;

    bool isLiveStream() const;

    MediaTime duration();
    MediaTime currentMediaTime();
    std::unique_ptr<PlatformTimeRanges> buffered();

	const WebCore::MediaPlayerMorphOSStreamSettings& streamSettings();
	void onSourceBufferInitialized(RefPtr<MediaSourceBufferPrivateMorphOS>&);
	void onSourceBufferReadyToPaint(RefPtr<MediaSourceBufferPrivateMorphOS>&);
	void onSourceBufferRemoved(RefPtr<MediaSourceBufferPrivateMorphOS>&);
	void onSourceBufferFrameUpdate(RefPtr<MediaSourceBufferPrivateMorphOS>&);
	void onSourceBufferDidChangeActiveState(RefPtr<MediaSourceBufferPrivateMorphOS>&, bool active);
	void onSourceBuffersReadyToPlay();
	void onAudioSourceBufferUpdatedPosition(RefPtr<MediaSourceBufferPrivateMorphOS>&, double);
	void onSourceBufferEnded(RefPtr<MediaSourceBufferPrivateMorphOS>&);
	void onSourceBufferLoadingProgressed();

	bool paused() const { return m_paused; }
	bool ended() const { return m_ended; }
    bool isEnded() const override { return m_ended; };
	bool isSeeking() const { return m_seeking; }

	void seek(double time);

    void orphan();
    WeakPtr<MediaPlayerPrivateMorphOS> &player() { return m_player; }
    WeakPtr<MediaPlayerPrivateMorphOS> const &player() const { return m_player; }
    void warmUp();
    void coolDown();

	void play();
	void pause();

	void paint(GraphicsContext&, const FloatRect&);
	void setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height);

	const String &url() const { return m_url; }
 
    bool hasVideo() const { return m_hasVideo; }
    bool hasAudio() const { return m_hasVideo; }
    
    void setVolume(double vol);
    void setMuted(bool muted);
    
    void dumpStatus();

protected:
	bool areDecodersReadyToPlay();
	bool areDecodersInitialized();
	
	void watchdogTimerFired();

private:
	WeakPtr<MediaPlayerPrivateMorphOS>               m_player;
    Ref<MediaSourcePrivateClient>                    m_client;
    String                                           m_url;
	HashSet<RefPtr<MediaSourceBufferPrivateMorphOS>> m_sourceBuffers;
	HashSet<RefPtr<MediaSourceBufferPrivateMorphOS>> m_activeSourceBuffers;
	RefPtr<MediaSourceBufferPrivateMorphOS>          m_paintingBuffer;
	MediaPlayer::ReadyState                          m_readyState = MediaPlayer::ReadyState::HaveNothing;
	RunLoop::Timer<MediaSourcePrivateMorphOS>        m_watchdogTimer;
    bool                                             m_orphaned = false;
	bool                                             m_paused = true;
	bool                                             m_ended = false;
	bool                                             m_waitReady = false;
	bool                                             m_initialized = false;
    bool                                             m_hasVideo = false;
    bool                                             m_hasAudio = false;
	double                                           m_volume = 1.f;
	bool                                             m_muted = false;

	double                                           m_position = 0;
	double                                           m_seekingPos;
	bool                                             m_seeking = false;
	bool                                             m_clientSeekDone = false;
};

}

#endif
