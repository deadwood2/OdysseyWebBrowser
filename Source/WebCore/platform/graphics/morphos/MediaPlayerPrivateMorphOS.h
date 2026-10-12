#pragma once

#if ENABLE(VIDEO)

#include "MediaPlayerPrivate.h"
#include "PlatformLayer.h"
#include "MediaPlayerMorphOS.h"
#include "AcinerellaClient.h"

#if ENABLE(MEDIA_SOURCE)
#include "MediaSourcePrivateMorphOS.h"
#endif

namespace WebCore {

namespace Acinerella {
	class Acinerella;
}

template<typename T>
using deleted_unique_ptr = std::unique_ptr<T,std::function<void(T*)>>;

class MediaPlayerPrivateMorphOS : public MediaPlayerPrivateInterface, public Acinerella::AcinerellaClient, public CanMakeWeakPtr<MediaPlayerPrivateMorphOS>, public RefCounted<MediaPlayerPrivateMorphOS>
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    MediaPlayerPrivateMorphOS(MediaPlayer*);
    virtual ~MediaPlayerPrivateMorphOS();

    void ref() final { RefCounted::ref(); }
    void deref() final { RefCounted::deref(); }

    static void registerMediaEngine(MediaEngineRegistrar);
    static MediaPlayer::SupportsType extendedSupportsType(const MediaEngineSupportParameters&, MediaPlayer::SupportsType);
    static bool supportsKeySystem(const String& keySystem, const String& mimeType);
	
    void load(const String&) final;
#if ENABLE(MEDIA_SOURCE)
    void load(const URL&, const ContentType&, MediaSourcePrivateClient&) final;
#endif
#if ENABLE(MEDIA_STREAM)
	void load(MediaStreamPrivate&) final { };
#endif
    void cancelLoad() final;
    void prepareToPlay() final;
    bool canSaveMediaData() const final;
    bool canLoad(bool isMediaSource);

    bool supportsPictureInPicture() const override { return false; }
    bool supportsFullscreen() const override { return true; }

	void play() final;
    void pause() final;
    FloatSize naturalSize() const final;

    unsigned decodedFrameCount() const { return m_decodedFrameCount; }
    unsigned droppedFrameCount() const { return m_droppedFrameCount; }

	MediaTime maxTimeSeekable() const final;
    MediaTime duration() const final;

    MediaTime currentTime() const final;

    bool hasVideo() const final;
    bool hasAudio() const final;

    void setPageIsVisible(bool) final;
    bool seeking() const final;
    bool paused() const final;

    void setVolume(float) final;
    void setMuted(bool) final;

	bool supportsScanning() const { return true; }
    void seekToTarget(const SeekTarget&) final;
    bool ended() const final;

    std::optional<VideoPlaybackQualityMetrics> videoPlaybackQualityMetrics() final;

    MediaPlayer::NetworkState networkState() const final;
    MediaPlayer::ReadyState readyState() const final;
    const PlatformTimeRanges& buffered() const override;
    void paint(GraphicsContext&, const FloatRect&) final;
    bool didLoadingProgress() const final;
	MediaPlayer::MovieLoadType movieLoadType() const final;

	void accInitialized(MediaPlayerMorphOSInfo info) override;
	void accUpdated(MediaPlayerMorphOSInfo info) override;
	void accSetNetworkState(WebCore::MediaPlayerEnums::NetworkState state, const WTF::String &error) override;
	void accSetReadyState(WebCore::MediaPlayerEnums::ReadyState state) override;
	void accSetBufferLength(double buffer) override;
	void accSetPosition(double buffer) override;
    void accSeeked(double position) override;
	void accSetDuration(double buffer) override;
	void accEnded() override;
	void accFailed() override;
	RefPtr<PlatformMediaResourceLoader> accCreateResourceLoader() override;
	String accReferrer() override;
	void accNextFrameReady() override;
	void accSetVideoSize(int width, int height) override;
	void accNoFramesReady() override;
	void accFrameUpdateNeeded() override;
	bool accCodecSupported(const String &codec) override;
	bool accIsURLValid(const String& url) override;
	void accSetFrameCounts(unsigned decoded, unsigned dropped) override;

	void setLoadingProgresssed(bool flag) { m_didLoadingProgress = flag; }
	void onActiveSourceBuffersChanged() { if (m_player) m_player->activeSourceBuffersChanged(); }

	const MediaPlayerMorphOSStreamSettings &streamSettings() { return m_streamSettings; }

	void onTrackEnabled(int index, bool enabled);

	void selectHLSStream(const String& url) override;

    DestinationColorSpace colorSpace() final { return DestinationColorSpace::SRGB(); }
    
    String errorMessage() const final;
protected:
	MediaPlayer* m_player;
	RefPtr<Acinerella::Acinerella> m_acinerella;
	MediaPlayer::NetworkState m_networkState = { MediaPlayer::NetworkState::Empty };
	MediaPlayer::ReadyState m_readyState = { MediaPlayer::ReadyState::HaveNothing };
	MediaPlayerMorphOSStreamSettings m_streamSettings;
	MediaTime m_duration = MediaTime::invalidTime();
	MediaTime m_currentTime;
    String m_errorMessage;
    PlatformTimeRanges m_buffered;
	int   m_width = 1280;
	int   m_height = 740;
	bool  m_prepareToPlay = false;
	bool  m_acInitialized = false;
	bool  m_visible = false;
	bool  m_didDrawFrame = false;
	unsigned m_decodedFrameCount = 0;
	unsigned m_droppedFrameCount = 0;
	mutable bool  m_didLoadingProgress = false;
	HashSet<String> m_failedHLSStreamURIs;

	RefPtr<AudioTrackPrivate> m_audioTrack;
	RefPtr<VideoTrackPrivate> m_videoTrack;

#if ENABLE(MEDIA_SOURCE)
	RefPtr<MediaSourcePrivateMorphOS> m_mediaSourcePrivate;
#endif

friend class Acinerella::Acinerella;
};

};

#endif

