#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include <wtf/Function.h>
#include <wtf/MessageQueue.h>
#include <wtf/Threading.h>
#include <wtf/text/WTFString.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/RunLoop.h>
#include "AcinerellaClient.h"
#include "AcinerellaBuffer.h"
#include "AcinerellaMuxer.h"
#include "AcinerellaDecoder.h"
#include "AcinerellaPointer.h"
#include "acinerella.h"

template<typename T>
using deleted_unique_ptr = std::unique_ptr<T,std::function<void(T*)>>;

struct Window;

namespace WebCore {
namespace Acinerella {

class Acinerella : public ThreadSafeRefCounted<Acinerella>, public AcinerellaNetworkBufferResourceLoaderProvider, public AcinerellaDecoderClient
{
friend class AcinerellaDecoder;
friend class AcinerellaMuxedBuffer;
public:
	Acinerella(AcinerellaClient *client, const String &url);
	virtual ~Acinerella() = default;

	static RefPtr<Acinerella> create(AcinerellaClient *client, const String &url) {
		return WTF::adoptRef(*new Acinerella(client, url));
	}

	void terminate();
	void warmUp();
	void coolDown();

	void play();

	void pause();
	bool paused();

	double duration() const { return m_duration; }
	
	bool hasAudio() { return m_audioDecoder.get(); }
	bool hasVideo() { return m_videoDecoder.get(); }
	
	void setVolume(double volume);
	void setMuted(bool muted);
	double volume() const { return m_volume; }
	bool muted() const { return m_muted; }

	bool canSeek();
	bool isSeeking();
	void seek(double time);
	
	bool isLive();
	bool ended();
	
	const String &url() const { return m_url; }

    void ref() override;
    void deref() override;
	RefPtr<PlatformMediaResourceLoader> createResourceLoader() override;
	String referrer() override;
	void selectStream() override;

	void paint(GraphicsContext&, const FloatRect&);
	void setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height);

protected:
	ac_instance *ac() { return m_acinerella ? m_acinerella->instance() : nullptr; }

	void threadEntryPoint();
	void dispatch(Function<void ()>&& function);
	void performTerminate();

	bool initialize();
	void initializeAfterDiscontinuity();
	
	void startSeeking(double pos);
	bool areDecodersReadyToPlay();

	int open();
	int close();
	int read(uint8_t *buf, int size);
	int64_t seek(int64_t pos, int whence);
	
	void demuxMorePackages(bool untilEOS = false);

	const WebCore::MediaPlayerMorphOSStreamSettings& streamSettings() override;

	void onDecoderWarmedUp(RefPtr<AcinerellaDecoder> decoder) override;
	void onDecoderReadyToPlay(RefPtr<AcinerellaDecoder> decoder) override;
	void onDecoderPlaying(RefPtr<AcinerellaDecoder> decoder, bool playing) override;
	void onDecoderEnded(RefPtr<AcinerellaDecoder> decoder) override;

	void onDecoderUpdatedBufferLength(RefPtr<AcinerellaDecoder> decoder, double buffer) override;
	void onDecoderUpdatedPosition(RefPtr<AcinerellaDecoder> decoder, double position) override;
	void onDecoderUpdatedDuration(RefPtr<AcinerellaDecoder> decoder, double duration) override;

	void onDecoderWantsToRender(RefPtr<AcinerellaDecoder> decoder) override;
	void onDecoderRenderUpdate(RefPtr<AcinerellaDecoder> decoder) override;
	void onDecoderNotReadyToRender(RefPtr<AcinerellaDecoder> decoder) override;

	void watchdogTimerFired();
	void dumpStatus();

protected:
	static int acOpenCallback(void *me);
	static int acCloseCallback(void *me);
	static int acReadCallback(void *me, uint8_t *buf, int size);
	static int64_t acSeekCallback(void *me, int64_t pos, int whence);

protected:
	AcinerellaClient                *m_client;
	String                           m_url;
	RefPtr<AcinerellaPointer>        m_acinerella;
	Lock                             m_acinerellaLock;
	RefPtr<AcinerellaNetworkBuffer>  m_networkBuffer;
	RunLoop::Timer<Acinerella>       m_watchdogTimer;

	RefPtr<AcinerellaMuxedBuffer>    m_muxer;
	RefPtr<AcinerellaDecoder>        m_audioDecoder;
	RefPtr<AcinerellaDecoder>        m_videoDecoder;

	double                           m_duration;
	double                           m_position;
	double                           m_volume = 1.f;
	double                           m_seekingPosition;
	bool                             m_muted = false;
	bool                             m_canSeek = true;
	bool                             m_isSeeking = false;
	bool                             m_isLive = false;
	bool                             m_ended = false;
	bool                             m_seekingForward;
	bool                             m_waitReady = false;
	bool                             m_ioDiscontinuity = false;
	bool                             m_liveEnded = false;
    volatile bool                    m_waitingForDemux = false;
	
	int64_t                          m_readPosition = -1;

    RefPtr<Thread>                   m_thread;
    MessageQueue<Function<void ()>>  m_queue;
    bool                             m_terminating = false;
    bool                             m_paused = false;
};

}
}

#endif
