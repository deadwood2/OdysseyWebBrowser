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

	void decode(Vector<unsigned char>&&);
	void signalEOF();
	void getSamples(MediaSamplesList& outSamples);
	void terminate();

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
	void signalEOF();

    void setVolume(double vol);

	const MediaPlayerMorphOSInfo &info() { return m_info; }
	bool isInitialized() { return m_metaInitDone; }

	void paint(GraphicsContext&, const FloatRect&);
	void setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height);

	void setAudioPresentationTime(double apts);
	bool areDecodersReadyToPlay();

    void onTrackEnabled(int index, bool enabled);
    void dumpStatus();

	void getFrameCounts(unsigned& decoded, unsigned &dropped) const;

private:
	explicit MediaSourceBufferPrivateMorphOS(MediaSourcePrivateMorphOS*);

    void setClient(SourceBufferPrivateClient*) override;

    void append(Vector<unsigned char>&&) override;
    void abort() override;
    void resetParserState() override;
    void removedFromMediaSource() override;

    void flush(const AtomicString&) override;
    void enqueueSample(Ref<MediaSample>&&, const AtomicString&)  override;
    void allSamplesInTrackEnqueued(const AtomicString&)  override;
    bool isReadyForMoreSamples(const AtomicString&)  override;
    void setActive(bool) override;
    void notifyClientWhenReadyForMoreSamples(const AtomicString&)  override;
    bool canSetMinimumUpcomingPresentationTime(const AtomicString&) const override;
	
	void flush();
	void becomeReadyForMoreSamples(int decoderIndex);

    MediaPlayer::ReadyState readyState() const override;
    void setReadyState(MediaPlayer::ReadyState) override;

	void initialize(bool success, WebCore::SourceBufferPrivateClient::InitializationSegment& segment, MediaPlayerMorphOSInfo& info);
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

private:
	MediaSourcePrivateMorphOS                    *m_mediaSource;
	SourceBufferPrivateClient                    *m_client;
	RefPtr<MediaSourceChunkReader>                m_reader;
	RefPtr<Acinerella::AcinerellaMuxedBuffer>     m_muxer;
	RefPtr<Acinerella::AcinerellaDecoder>         m_decoders[Acinerella::AcinerellaMuxedBuffer::maxDecoders];
	RefPtr<Acinerella::AcinerellaDecoder>         m_paintingDecoder;
	bool                                          m_decodersStarved[Acinerella::AcinerellaMuxedBuffer::maxDecoders];

    RefPtr<Thread>                                m_thread;
    MessageQueue<Function<void ()>>               m_queue;
	BinarySemaphore                               m_event;
	Lock                                          m_lock;
	
	uint32_t                                      m_audioDecoderMask = 0;
	bool                                          m_enableVideo = false;
	bool                                          m_enableAudio = true;
	bool                                          m_terminating = false;
	bool                                          m_eos = false;

	MediaPlayerMorphOSInfo                        m_info;
    WebCore::SourceBufferPrivateClient::InitializationSegment m_segment;
	bool                                          m_metaInitDone = false;

	bool                                          m_readyForMoreSamples = true;
	bool                                          m_requestedMoreFrames = false;
    bool                                          m_enqueuedSamples = false;
	
	bool                                          m_seeking = false;
	double                                        m_seekTime;
	
	int                                           m_enqueueCount = 0;
	int                                           m_appendCount = 0;
	int                                           m_appendCompleteCount = 0;
};

}

#endif
