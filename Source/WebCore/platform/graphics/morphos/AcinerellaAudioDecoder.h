#pragma once

#if ENABLE(VIDEO)

#include "AcinerellaDecoder.h"
#include <devices/ahi.h>
#include <exec/libraries.h>

namespace WebCore {
namespace Acinerella {

class AcinerellaAudioRequest : public ThreadSafeRefCounted<AcinerellaAudioRequest>
{
    AcinerellaAudioRequest(const int frequency, const int channels);
    AcinerellaAudioRequest(RefPtr<AcinerellaAudioRequest> primary);
public:
	static RefPtr<AcinerellaAudioRequest> create(const int frequency, const int channels) {
		return WTF::adoptRef(*new AcinerellaAudioRequest(frequency, channels));
	}
	static RefPtr<AcinerellaAudioRequest> create(RefPtr<AcinerellaAudioRequest> primary) {
		return WTF::adoptRef(*new AcinerellaAudioRequest(primary));
	}

    ~AcinerellaAudioRequest();

    bool isValid() const { return m_request != nullptr; }
    bool isPending() const { return m_pending; }
    bool isEmpty() const { return m_bufferUsed == 0; }
    bool isFull() const { return m_bufferUsed == sizeof(m_buffer); }
    int sigBit() const { return m_msgPort->mp_SigBit; }
    
    void setVolume(double volume);
    
    double timeToAnnounce() const { return m_timeToAnnounce; }
    void setTimeToAnnounce(double tta) { m_timeToAnnounce = tta; }
    
    bool isEOF() const { return m_isEOF; }
    void setIsEOF(bool iseof) { m_isEOF = iseof; }

    size_t push(const uint8_t *frames, const size_t byteCount);
    void play(RefPtr<AcinerellaAudioRequest> continuation);
    void pause();
    void stop();
    size_t dataToPlayAfterResuming();

    static RefPtr<AcinerellaAudioRequest> poll(RefPtr<AcinerellaAudioRequest>& requestA, RefPtr<AcinerellaAudioRequest>& requestB);

private:
    void onDone();

    static constexpr size_t sAudioBufferSize = 44100; // effectively 0.25s of 16 bit stereo audio @ 44100

    struct AHIRequest* m_request = nullptr;
    struct MsgPort* m_msgPort = nullptr;

    RefPtr<AcinerellaAudioRequest> m_primaryRequest;
    int      m_frequency;
    int      m_channels;
    uint32_t m_volume = 65536;
    size_t   m_bufferUsed = 0;
    uint8_t  m_buffer[sAudioBufferSize];
    size_t   m_continuation = sizeof(m_buffer);
    double   m_timeToAnnounce;
    bool     m_pending = false;
    bool     m_paused = false;
    bool     m_isEOF = false;
};

class AcinerellaAudioDecoder : public AcinerellaDecoder
{
	AcinerellaAudioDecoder(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLive, bool isHLS);
public:
	static RefPtr<AcinerellaAudioDecoder> create(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLive, bool isHLS)
	{
		return WTF::adoptRef(*new AcinerellaAudioDecoder(client, acinerella, buffer, index, info, isLive, isHLS));
	}

	int rate() const { return m_audioRate; }
	int channels() const { return m_audioChannels; }
	int bits() const { return m_audioBits; }
	
	bool isAudio() const override { return true; }
	bool isVideo() const override { return false; }
	bool isText() const override { return false; }
	
	double readAheadTime() const override { return m_isLive ? 4.0 : 2.0; }

	bool isReadyToPlay() const override;
	bool isPlaying() const override;
    bool isWarmedUp() const override;

	double position() const override;
	double bufferSize() const override { return m_bufferedSeconds; }
	void paint(GraphicsContext&, const FloatRect&) override { }

	void dumpStatus() override;

protected:
	void startPlaying() override;
	void stopPlaying() override;
	void doSetVolume(double volume) override;

	void onThreadShutdown() override;
	void onGetReadyToPlay() override;
	void onFrameDecoded(const AcinerellaDecodedFrame &frame) override;
    bool acceptPackage(RefPtr<AcinerellaPackage>&, double pts) override;
	void flush(bool willSeek) override;
	bool initializeAudio();
	void onCoolDown() override;
	void ahiCleanup();

	bool fillBuffer(RefPtr<AcinerellaAudioRequest>& request, double desiredPosition);
	void ahiThreadEntryPoint();
	void stopPlayingQuick() override { m_playing = false; };
 
    void signalAHIThread(std::function<void()>&& func);

protected:
	BinarySemaphore m_ahiSampleConsumed;
	RefPtr<Thread>  m_ahiThread;
    BinarySemaphore m_ahiThreadReady;
    Lock            m_ahiThreadAccessLock;
    struct Task    *m_ahiThreadTask = nullptr;
    int             m_ahiThreadStateSignal = -1;
	bool            m_ahiThreadShuttingDown = false;
    size_t          m_ahiFrameOffset = 0;
 
    Atomic<bool>    m_ahiThreadTransitionPlaying = false;
    Atomic<bool>    m_ahiThreadTransitionPaused = false;
    Atomic<bool>    m_ahiThreadVolumeChanged = false;

	uint32_t        m_bufferedSamples = 0;
	AcinerellaThreadsafeNumber<double> m_bufferedSeconds = 0.f;
	volatile bool   m_playing = false;
	double          m_position = 0.0;
	double          m_liveTimeCode = 0.0;
    double          m_volume = 1.0;
	bool            m_didUnderrun = false;
	int             m_audioRate;
	int             m_audioChannels;
	int             m_audioBits;
};

}
}

#endif
