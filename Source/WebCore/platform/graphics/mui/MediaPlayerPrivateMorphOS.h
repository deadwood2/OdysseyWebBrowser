#pragma once

#include "config.h"

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

class MediaPlayerPrivateMorphOS : public MediaPlayerPrivateInterface, public Acinerella::AcinerellaClient, public CanMakeWeakPtr<MediaPlayerPrivateMorphOS>
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    MediaPlayerPrivateMorphOS(MediaPlayer*);
    virtual ~MediaPlayerPrivateMorphOS();

    static void registerMediaEngine(MediaEngineRegistrar);
    static MediaPlayer::SupportsType extendedSupportsType(const MediaEngineSupportParameters&, MediaPlayer::SupportsType);
    static bool supportsKeySystem(const String& keySystem, const String& mimeType);
	
    void load(const String&) final;
#if ENABLE(MEDIA_SOURCE)
    void load(const String& url, MediaSourcePrivateClient*) final;
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

    float duration() const final { return m_duration; }
    double durationDouble() const final { return m_duration; }
    MediaTime durationMediaTime() const final;

    unsigned decodedFrameCount() const { return m_decodedFrameCount; }
    unsigned droppedFrameCount() const { return m_droppedFrameCount; }

	float maxTimeSeekable() const final;
    float currentTime() const final { return m_currentTime; }

    bool hasVideo() const final;
    bool hasAudio() const final;

    void setVisible(bool) final;
    bool seeking() const final;
    bool paused() const final;

    void setVolume(float) final;
    void setMuted(bool) final;

	bool supportsScanning() const { return true; }
    void seek(float) final;
    bool ended() const final;

    Optional<VideoPlaybackQualityMetrics> videoPlaybackQualityMetrics() final;

    MediaPlayer::NetworkState networkState() const final;
    MediaPlayer::ReadyState readyState() const final;
    std::unique_ptr<PlatformTimeRanges> buffered() const final;
    void paint(GraphicsContext&, const FloatRect&) final;
    bool didLoadingProgress() const final;
	MediaPlayer::MovieLoadType movieLoadType() const final;

	void accInitialized(MediaPlayerMorphOSInfo info) override;
	void accSetNetworkState(WebCore::MediaPlayerEnums::NetworkState state) override;
	void accSetReadyState(WebCore::MediaPlayerEnums::ReadyState state) override;
	void accSetBufferLength(double buffer) override;
	void accSetPosition(double buffer) override;
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
	void accSetFrameCounts(unsigned decoded, unsigned dropped) override;
	void setSize(const IntSize&) override;

	void setLoadingProgresssed(bool flag) { m_didLoadingProgress = flag; }
	void onActiveSourceBuffersChanged() { if (m_player) m_player->activeSourceBuffersChanged(); }

	const MediaPlayerMorphOSStreamSettings &streamSettings() { return m_streamSettings; }

	void onTrackEnabled(int index, bool enabled);

protected:
	MediaPlayer* m_player;
	RefPtr<Acinerella::Acinerella> m_acinerella;
	MediaPlayer::NetworkState m_networkState = { MediaPlayer::NetworkState::Empty };
	MediaPlayer::ReadyState m_readyState = { MediaPlayer::ReadyState::HaveNothing };
	MediaPlayerMorphOSStreamSettings m_streamSettings;
	double m_duration = 0.f;
	double m_currentTime = 0.f;
	int   m_width = 320;
	int   m_height = 240;
	bool  m_prepareToPlay = false;
	bool  m_acInitialized = false;
	bool  m_visible = false;
	bool  m_didDrawFrame = false;
	unsigned m_decodedFrameCount = 0;
	unsigned m_droppedFrameCount = 0;
	mutable bool  m_didLoadingProgress = false;

#if ENABLE(MEDIA_SOURCE)
	RefPtr<MediaSourcePrivateMorphOS> m_mediaSourcePrivate;
#endif

friend class Acinerella::Acinerella;
};

};

#endif

