#pragma once

#include "config.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

#include "SourceBufferPrivate.h"
#include "AcinerellaPointer.h"
#include "AcinerellaBuffer.h"
#include "AcinerellaMuxer.h"
#include "AcinerellaDecoder.h"
#include "MediaPlayerMorphOS.h"
#include "SourceBufferPrivateClient.h"
#include "MediaSample.h"

#include <wtf/Function.h>
#include <wtf/MessageQueue.h>
#include <wtf/Threading.h>
#include <wtf/StdList.h>
#include <wtf/text/WTFString.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/RunLoop.h>

#include <dos/dos.h>

struct Window;

namespace WebCore {

class GraphicsContext;
class FloatRect;
class MediaSourcePrivateMorphOS;
class MediaPlayerPrivateMorphOS;
class MediaSourceBufferPrivateMorphOS;

class MediaSourceChunkReader : public WTF::ThreadSafeRefCounted<MediaSourceChunkReader>
{
public:
	typedef Function<void(bool success, WebCore::SourceBufferPrivateClient::InitializationSegment& segment, MediaPlayerMorphOSInfo& info)> InitializationCallback;
	typedef Function<void(bool)> ChunkDecodedCallback;
protected:
	MediaSourceChunkReader(MediaSourceBufferPrivateMorphOS *, InitializationCallback &&, ChunkDecodedCallback &&);
public:
	virtual ~MediaSourceChunkReader();

	typedef WTF::StdList<WTF::RefPtr<WebCore::MediaSample>> MediaSamplesList;

    static Ref<MediaSourceChunkReader> create(MediaSourceBufferPrivateMorphOS *source, InitializationCallback &&icb, ChunkDecodedCallback && ccb) {
		return adoptRef(*new MediaSourceChunkReader(source, WTFMove(icb), WTFMove(ccb)));
	}

	void decode(Vector<unsigned char>&&, bool signalComplete = true);
	void signalEOF();
	void getSamples(MediaSamplesList& outSamples);
	void terminate();

	int numDecoders() const { return m_numDecoders; }
	double highestPTS() const { return m_highestPTS; }

	RefPtr<Acinerella::AcinerellaPointer>& acinerella() { return m_acinerella; }

protected:
	bool initialize();
	void getMeta(WebCore::SourceBufferPrivateClient::InitializationSegment& segment, MediaPlayerMorphOSInfo& info);
	void decodeAllMediaSamples();
	void dispatch(Function<void ()>&& function);

	bool keepDecoding();

	int read(uint8_t *buf, int size);
	static int acReadCallback(void *me, uint8_t *buf, int size);

protected:
	InitializationCallback                m_initializationCallback;
	ChunkDecodedCallback                  m_chunkDecodedCallback;

	RefPtr<Acinerella::AcinerellaPointer> m_acinerella;
	uint32_t                              m_audioDecoderMask = 0;
	uint32_t                              m_videoDecoderMask = 0;
	MediaSourceBufferPrivateMorphOS      *m_source;
	MediaSamplesList                      m_samples;

    RefPtr<Thread>                        m_thread;
    MessageQueue<Function<void ()>>       m_queue;
	BinarySemaphore                       m_event;
	Lock                                  m_lock;
	bool                                  m_terminating = false;

	Vector<unsigned char>                 m_buffer;
	Vector<unsigned char>                 m_leftOver;
	int                                   m_bufferPosition = 0;
	bool                                  m_bufferEOF = false;
	bool                                  m_readEOF = false;
	double                                m_highestPTS = 0.0;

	int                                   m_decodeCount = 0;
	int                                   m_decodeAppendCount = 0;
	int                                   m_numDecoders = 0;
	bool                                  m_signalComplete = true;
	
	BPTR                                  m_debugFile = 0;
};

class MediaSourceBufferPrivateMorphOS final : public SourceBufferPrivate, public Acinerella::AcinerellaDecoderClient {
public:
    static Ref<MediaSourceBufferPrivateMorphOS> create(MediaSourcePrivateMorphOS*);
    virtual ~MediaSourceBufferPrivateMorphOS();

	void play();
	void prePlay();
	void pause();

	void warmUp();
	void coolDown();
    void clearMediaSource();
    void terminate();

	void willSeek(double seekTo);
	void seekToTime(const MediaTime&) override;
	void signalEOF();

    void setVolume(double vol);

	const MediaPlayerMorphOSInfo &info() { return m_info; }
	bool isInitialized() { return m_metaInitDone; }

	void paint(GraphicsContext&, const FloatRect&);
	void setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height);

	void setAudioPresentationTime(double apts);
	bool areDecodersReadyToPlay();
	bool areDecodersPlaying();
	float decodersBufferedTime();

    void onTrackEnabled(int index, bool enabled);
    void dumpStatus();

	void getFrameCounts(unsigned& decoded, unsigned &dropped) const;
	
	bool didFailDecodingFrames() const { return m_readerFailed; }

private:
	explicit MediaSourceBufferPrivateMorphOS(MediaSourcePrivateMorphOS*);

    void append(Vector<unsigned char>&&) override;
    void abort() override;
    void resetParserState() override;
    void removedFromMediaSource() override;

    void flush(const AtomString&) override;
    void enqueueSample(Ref<MediaSample>&&, const AtomString&)  override;
    void allSamplesInTrackEnqueued(const AtomString&)  override;
    bool isReadyForMoreSamples(const AtomString&)  override;
    void setActive(bool) override;
    void notifyClientWhenReadyForMoreSamples(const AtomString&)  override;
    bool canSetMinimumUpcomingPresentationTime(const AtomString&) const override;

//	bool isActive() const override { return m_isActive; }
	bool isSeeking() const override;
	bool isSeekingInternal() const { return m_seeking; }
	MediaTime currentMediaTime() const override;
	MediaTime duration() const override;

	void flush();
	void becomeReadyForMoreSamples(int decoderIndex);

    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

	void initialize(bool success, WebCore::SourceBufferPrivateClient::InitializationSegment& segment, MediaPlayerMorphOSInfo& info);
	void reinitialize(bool success, WebCore::SourceBufferPrivateClient::InitializationSegment& segment, MediaPlayerMorphOSInfo& info);
	void appendComplete(bool success);

	void threadEntryPoint();
	void dispatch(Function<void ()>&& function);
	void performTerminate();

	// AcinerellaDecoderClient
	const WebCore::MediaPlayerMorphOSStreamSettings& streamSettings() override;
	void onDecoderWarmedUp(RefPtr<Acinerella::AcinerellaDecoder> decoder) override;
	void onDecoderReadyToPlay(RefPtr<Acinerella::AcinerellaDecoder> decoder) override;
	void onDecoderPlaying(RefPtr<Acinerella::AcinerellaDecoder> decoder, bool playing) override;
	void onDecoderUpdatedBufferLength(RefPtr<Acinerella::AcinerellaDecoder> decoder, double buffer) override;
	void onDecoderUpdatedPosition(RefPtr<Acinerella::AcinerellaDecoder> decoder, double position) override;
	void onDecoderUpdatedDuration(RefPtr<Acinerella::AcinerellaDecoder> decoder, double duration) override;
	void onDecoderEnded(RefPtr<Acinerella::AcinerellaDecoder> decoder)  override;
	void onDecoderWantsToRender(RefPtr<Acinerella::AcinerellaDecoder> decoder) override;
	void onDecoderNotReadyToRender(RefPtr<Acinerella::AcinerellaDecoder> decoder) override;
	void onDecoderRenderUpdate(RefPtr<Acinerella::AcinerellaDecoder> decoder) override;

	void seekTimerFired();

private:
	MediaSourcePrivateMorphOS                    *m_mediaSource;
	RefPtr<MediaSourceChunkReader>                m_reader;
	RefPtr<Acinerella::AcinerellaMuxedBuffer>     m_muxer;
	RefPtr<Acinerella::AcinerellaDecoder>         m_decoders[Acinerella::AcinerellaMuxedBuffer::maxDecoders];
	RefPtr<Acinerella::AcinerellaDecoder>         m_paintingDecoder;
	bool                                          m_decodersStarved[Acinerella::AcinerellaMuxedBuffer::maxDecoders];
	uint32_t                                      m_maxBuffer[Acinerella::AcinerellaMuxedBuffer::maxDecoders];
	int                                           m_numDecoders = 0;

    RefPtr<Thread>                                m_thread;
    MessageQueue<Function<void ()>>               m_queue;
	BinarySemaphore                               m_event;
	Lock                                          m_lock;

	Vector<unsigned char>                         m_initializationBuffer;

	uint32_t                                      m_audioDecoderMask = 0;
	bool                                          m_enableVideo = false;
	bool                                          m_enableAudio = true;
	bool                                          m_terminating = false;
	bool                                          m_eos = false;
	std::atomic<bool>                             m_appendCompletePending = false;
	bool                                          m_appendCompleteDelayed = false;

	MediaPlayerMorphOSInfo                        m_info;
    WebCore::SourceBufferPrivateClient::InitializationSegment m_segment;
	bool                                          m_metaInitDone = false;

	bool                                          m_readyForMoreSamples = true;
	bool                                          m_requestedMoreFrames = false;
    bool                                          m_enqueuedSamples = false;
    bool                                          m_isActive = false;
	
	bool                                          m_seeking = false;
	bool                                          m_seekingWaitsForFrames = false;
	bool                                          m_postSeekingAppendDone = false;
	double                                        m_seekTime;
	
	int                                           m_enqueueCount = 0;
	int                                           m_appendCount = 0;
	int                                           m_appendCompleteCount = 0;
	bool                                          m_readerFailed = false;
	bool                                          m_mustAppendInitializationSegment = false;
};

}

#endif
