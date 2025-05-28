#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include "acinerella.h"
#include <wtf/Seconds.h>
#include <wtf/Function.h>
#include <wtf/MessageQueue.h>
#include <wtf/Threading.h>
#include <wtf/text/WTFString.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <memory>
#include "AcinerellaBuffer.h"
#include "AcinerellaMuxer.h"
#include "AcinerellaPointer.h"

template<typename T>
using deleted_unique_ptr = std::unique_ptr<T,std::function<void(T*)>>;

namespace WebCore {

class GraphicsContext;
class FloatRect;
struct MediaPlayerMorphOSStreamSettings;

namespace Acinerella {

class Acinerella;

class AcinerellaDecodedFrame
{
public:
	explicit AcinerellaDecodedFrame(RefPtr<AcinerellaPointer> pointer, ac_decoder *decoder) : m_pointer(pointer), m_frame(ac_alloc_decoder_frame(decoder)) { }
	explicit AcinerellaDecodedFrame() : m_frame(nullptr) { }
	explicit AcinerellaDecodedFrame(const AcinerellaDecodedFrame &) = delete;
	explicit AcinerellaDecodedFrame(AcinerellaDecodedFrame &) = delete;
	explicit AcinerellaDecodedFrame(AcinerellaDecodedFrame && otter) : m_pointer(otter.m_pointer), m_frame(otter.m_frame) { otter.m_pointer = nullptr; otter.m_frame = nullptr; }
	~AcinerellaDecodedFrame() { ac_free_decoder_frame(m_frame); }
	
	AcinerellaDecodedFrame & operator=(const AcinerellaDecodedFrame &) = delete;
	AcinerellaDecodedFrame & operator=(AcinerellaDecodedFrame &) = delete;
	AcinerellaDecodedFrame & operator=(AcinerellaDecodedFrame && otter) { std::swap(m_pointer, otter.m_pointer); std::swap(m_frame, otter.m_frame); return *this; }

	ac_decoder_frame *frame() { return m_frame; }
	double pts() const { return m_frame ? m_frame->timecode : 0.f; }
	const ac_decoder_frame *frame() const { return m_frame; }
	
	RefPtr<AcinerellaPointer> &pointer() { return m_pointer; }

protected:
	RefPtr<AcinerellaPointer> m_pointer;
	ac_decoder_frame         *m_frame;
};

class AcinerellaDecoderClient
{
public:
	virtual const WebCore::MediaPlayerMorphOSStreamSettings& streamSettings() = 0;

	virtual void onDecoderWarmedUp(RefPtr<AcinerellaDecoder> decoder) = 0;
	virtual void onDecoderReadyToPlay(RefPtr<AcinerellaDecoder> decoder) = 0;

	virtual void onDecoderPlaying(RefPtr<AcinerellaDecoder> decoder, bool playing) = 0;
	virtual void onDecoderEnded(RefPtr<AcinerellaDecoder> decoder) = 0;

	virtual void onDecoderUpdatedBufferLength(RefPtr<AcinerellaDecoder> decoder, double buffer) = 0;
	virtual void onDecoderUpdatedPosition(RefPtr<AcinerellaDecoder> decoder, double position) = 0;
	virtual void onDecoderUpdatedDuration(RefPtr<AcinerellaDecoder> decoder, double duration) = 0;

	virtual void onDecoderWantsToRender(RefPtr<AcinerellaDecoder> decoder) = 0;
	virtual void onDecoderRenderUpdate(RefPtr<AcinerellaDecoder> decoder) = 0;
	virtual void onDecoderNotReadyToRender(RefPtr<AcinerellaDecoder> decoder) = 0;
};

class AcinerellaDecoder : public ThreadSafeRefCounted<AcinerellaDecoder>
{
protected:
	AcinerellaDecoder(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLiveStream, bool isHLS);
public:
	virtual ~AcinerellaDecoder();

	// call from: Acinerella thread
	void terminate();

    // buffer enough frames, signal warmedUp
	void warmUp();
    // true if enough data is buffered in the decoder
    virtual bool isWarmedUp() const = 0;

    // release AHI/overlay, etc
	void coolDown();

    // get ready to render 1st frame, setup (paused) AHI output, etc
    // do everything required for play() to be instantneous
	void prePlay();
	virtual bool isReadyToPlay() const = 0;

	void play();
	void pause(bool willSeek = false);
	virtual bool isPlaying() const = 0;
    bool isEnded() const;

	virtual bool isAudio() const = 0;
	virtual bool isVideo() const = 0;
	virtual bool isText() const = 0;

	void setVolume(float volume);

	double duration() const { return m_duration; }
	double bitRate() const { return m_bitrate; }
	const WTF::String &codec() const { return m_codec; }
	virtual double position() const = 0;
	virtual double bufferSize() const = 0;

	virtual void paint(GraphicsContext&, const FloatRect&) = 0;

	int index() const { return m_index; }
	
	virtual void dumpStatus() = 0;

protected:
	// call from: Own thread
	void threadEntryPoint();
	// call from: Any thread
	void dispatch(Function<void ()>&& function);
	// call from: Acinerella thread
	void performTerminate();

	// call from: Own thread
	bool decodeNextFrame();
	void decodeUntilBufferFull();
	void dropUntilPTS(double pts);
	void onPositionChanged();
	void onDurationChanged();
	void onReadyToPlay();
	void onEnded();
	virtual void flush();
	virtual void onGetReadyToPlay() { };
	virtual void onCoolDown() { };

	// call from: Own thread
	virtual bool onThreadInitialize() { return true; }
	virtual void onThreadShutdown() { }

	// call from: main thread/terminate
	virtual void onTerminate() { }

	// call from: Own thread, under m_lock!
	virtual void onDecoderChanged(RefPtr<AcinerellaPointer>) { }
	virtual void onFrameDecoded(const AcinerellaDecodedFrame &) { }

	// call from: Own thread
	virtual void startPlaying() = 0;
	virtual void stopPlaying() = 0;
	virtual void stopPlayingQuick() { };
	virtual void doSetVolume(double) { };

	// call from: Any thread
	virtual double readAheadTime() const = 0;

protected:
	AcinerellaDecoderClient           *m_client; // valid until terminate is called
    RefPtr<Thread>                     m_thread;
    MessageQueue<Function<void ()>>    m_queue;

	RefPtr<AcinerellaMuxedBuffer>      m_muxer;
	WTF::String                        m_codec;
	double                             m_duration;
	int                                m_bitrate;
	int                                m_index;
	bool                               m_isLive = false;
	bool                               m_isHLS = false;
	
	std::queue<AcinerellaDecodedFrame> m_decodedFrames;
	Lock                               m_lock;

	ac_decoder                        *m_lastDecoder = nullptr;

	bool                               m_readying = false;
	bool                               m_warminUp = false;
	bool                               m_terminating = false;
	bool                               m_decoderEOF = false;
	
	bool                               m_droppingFrames = false;
	bool                               m_droppingUntilKeyFrame = false;
	bool                               m_needsKF = false;
	double                             m_dropToPTS;
};

}
}

#endif
