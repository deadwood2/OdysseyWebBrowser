#include "config.h"

#include "AcinerellaAudioDecoder.h"
#include "AcinerellaContainer.h"
#include "MediaPlayerMorphOS.h"

#if ENABLE(VIDEO)
#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <intuition/intuition.h>

namespace WebCore {
namespace Acinerella {

// #pragma GCC optimize ("O0")

#undef AHI_BASE_NAME
#define AHI_BASE_NAME m_ahiBase

#define D(x)
#define DTHREAD(x) 
#define DAAR(x)
#define DFILL(x)
#define DSPAM(x)

AcinerellaAudioRequest::AcinerellaAudioRequest(const int frequency, const int channels)
{
    DAAR(dprintf("[AD]%s: %p\n", __func__, this));
    m_msgPort = CreateMsgPort();

    if (nullptr != m_msgPort)
    {
        m_request = reinterpret_cast<AHIRequest*>(CreateIORequest(m_msgPort, sizeof(struct AHIRequest)));
        if (nullptr != m_request)
        {
            m_request->ahir_Version = 5;
            m_frequency = frequency;
            m_channels = channels;
            
            if (0 != OpenDevice("ahi.device", 0, reinterpret_cast<IORequest *>(m_request), 0))
            {
                DeleteIORequest(reinterpret_cast<IORequest *>(m_request));
                m_request = nullptr;
                DeleteMsgPort(m_msgPort);
                m_msgPort = nullptr;
            }
        }
    }
}

AcinerellaAudioRequest::AcinerellaAudioRequest(RefPtr<AcinerellaAudioRequest> primary)
{
    DAAR(dprintf("[AD]%s: %p\n", __func__, this));
    m_primaryRequest = primary;
    if (!!m_primaryRequest && primary->isValid())
    {
        m_msgPort = primary->m_msgPort;
        m_request = reinterpret_cast<AHIRequest*>(malloc(sizeof(struct AHIRequest)));
        m_frequency = primary->m_frequency;
        m_channels = primary->m_channels;
        m_volume = primary->m_volume;
        
        if (nullptr != m_request)
        {
            memcpy(m_request, primary->m_request, sizeof(struct AHIRequest));
        }
    }
}

AcinerellaAudioRequest::~AcinerellaAudioRequest()
{
    DAAR(dprintf("[AD]%s: %p\n", __func__, this));

    if (m_pending)
        WaitIO(reinterpret_cast<IORequest *>(m_request));

    if (!!m_primaryRequest)
    {
        free(m_request);
    }
    else if (m_request)
    {
        CloseDevice(reinterpret_cast<IORequest *>(m_request));
        DeleteIORequest(reinterpret_cast<IORequest *>(m_request));
        DeleteMsgPort(m_msgPort);
    }
}

void AcinerellaAudioRequest::setVolume(double volume)
{
    m_volume = (LONG) (double(0x10000L) * volume);
}

size_t AcinerellaAudioRequest::push(const uint8_t *frames, const size_t byteCount)
{
    DAAR(dprintf("[AD]%s: %p bytes %lu\n", __func__, this, byteCount));

    if (m_pending)
    {
        dprintf("[AD]%s: attempted to write to a pending buffer\n", __func__);
        return 0;
    }

    size_t max = std::min(sizeof(m_buffer) - m_bufferUsed, byteCount);

    if (max > 0)
    {
        memcpy(m_buffer + m_bufferUsed, frames, max);
        m_bufferUsed += max;
    }

    return max;
}

void AcinerellaAudioRequest::play(RefPtr<AcinerellaAudioRequest> continuation)
{
    DAAR(dprintf("[AD]%s: %p\n", __func__, this));
    if (m_pending)
    {
        dprintf("[AD]%s: attempted to play a pending buffer\n", __func__);
        return;
    }

    m_request->ahir_Std.io_Command = CMD_WRITE;
    m_request->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
    m_request->ahir_Std.io_Data = m_buffer;
    m_request->ahir_Std.io_Length = m_bufferUsed;
    m_request->ahir_Std.io_Offset = m_paused ? m_continuation : 0;
    m_request->ahir_Frequency = m_frequency;
    m_request->ahir_Volume = m_volume;
    m_request->ahir_Position = 0x00008000; // balance
    m_request->ahir_Type = (m_channels == 2) ? AHIST_S16S : AHIST_M16S;
    m_request->ahir_Link = !!continuation ? continuation->m_request : nullptr;
    m_continuation = sizeof(m_buffer);
    m_paused = false;
    m_pending = true;

    SendIO(reinterpret_cast<IORequest *>(m_request));
}

void AcinerellaAudioRequest::pause()
{
    if (m_pending)
    {
        AbortIO(reinterpret_cast<IORequest *>(m_request));
        WaitIO(reinterpret_cast<IORequest *>(m_request));
        m_pending = false;
        m_paused = true;
        m_continuation = m_request->ahir_Std.io_Actual;
        DAAR(dprintf("[AD]%s: %p continuation %lu of %lu\n", __func__, this, m_continuation, m_bufferUsed));
    }
}

void AcinerellaAudioRequest::stop()
{
    if (m_pending)
    {
        AbortIO(reinterpret_cast<IORequest *>(m_request));
        WaitIO(reinterpret_cast<IORequest *>(m_request));
        m_pending = false;
        m_paused = false;
        m_continuation = sizeof(m_buffer);
        m_bufferUsed = 0;
    }
}

size_t AcinerellaAudioRequest::dataToPlayAfterResuming()
{
    if (m_paused && m_bufferUsed && m_continuation < m_bufferUsed)
        return m_bufferUsed - m_continuation;
    return 0;
}

void AcinerellaAudioRequest::onDone()
{
    m_pending = false;
    m_continuation = sizeof(m_buffer);
    m_bufferUsed = 0;
}

RefPtr<AcinerellaAudioRequest> AcinerellaAudioRequest::poll(RefPtr<AcinerellaAudioRequest>& requestA, RefPtr<AcinerellaAudioRequest>& requestB)
{
    struct AHIRequest *r;

    DAAR(dprintf("[AD]%s:\n", __func__));

    while ((r = reinterpret_cast<AHIRequest*>(GetMsg(requestA->m_msgPort))))
    {
        if (requestA->m_request == r)
        {
            requestA->onDone();
            DAAR(dprintf("[AD]%s: %p: %d, %p: %d\n", __func__, requestA.get(), requestA->m_pending, requestB.get(),requestB->m_pending));
            return requestA;
        }
        else if (requestB->m_request == r)
        {
            requestB->onDone();
            DAAR(dprintf("[AD]%s: %p: %d, %p: %d\n", __func__, requestA.get(), requestA->m_pending, requestB.get(),requestB->m_pending));
            return requestB;
        }

    }
    
    DAAR(dprintf("[AD]%s: %p: %d, %p: %d\n", __func__, requestA.get(), requestA->m_pending, requestB.get(),requestB->m_pending));
    return nullptr;
}


///--------------------------------------- 

AcinerellaAudioDecoder::AcinerellaAudioDecoder(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLiveStream, bool isHLS)
	: AcinerellaDecoder(client, acinerella, buffer, index, info, isLiveStream, isHLS)
	, m_audioRate(ac_get_audio_rate(acinerella->decoder(index)))
	, m_audioChannels(info.additional_info.audio_info.channel_count)
	, m_audioBits(info.additional_info.audio_info.bit_depth)
{
	D(dprintf("[AD]%s: %p\n", __func__, this));
}

void AcinerellaAudioDecoder::startPlaying()
{
	D(dprintf("[AD]%s: %p\n", __func__, this));
	EP_EVENT(start);
	initializeAudio();

	m_playing = true;
    signalAHIThread([&](){
        m_ahiThreadTransitionPlaying.store(true);
        m_ahiThreadTransitionPaused.store(false);
    });
}

void AcinerellaAudioDecoder::stopPlaying()
{
	D(dprintf("[AD]%s: %p\n", __func__, this));
	EP_EVENT(stop);

    m_playing = false;
    signalAHIThread([&](){
        m_ahiThreadTransitionPaused.store(true);
        m_ahiThreadTransitionPlaying.store(false);
    });
}

void AcinerellaAudioDecoder::onCoolDown()
{
	ahiCleanup();
}

void AcinerellaAudioDecoder::doSetVolume(double volume)
{
    m_volume = volume;
    signalAHIThread([&](){
        m_ahiThreadVolumeChanged.store(true);
    });
}

bool AcinerellaAudioDecoder::isReadyToPlay() const
{
	return isWarmedUp() && !!m_ahiThread;
}

bool AcinerellaAudioDecoder::isWarmedUp() const
{
	return (bufferSize() >= readAheadTime()) || m_decoderEOF;
}

bool AcinerellaAudioDecoder::isPlaying() const
{
	return m_playing;
}

bool AcinerellaAudioDecoder::initializeAudio()
{
	D(dprintf("[AD]%s:\n", __func__));
	EP_SCOPE(initializeAudio);

	if (m_ahiThread)
		return true;
    
    m_ahiThreadShuttingDown = false;
    m_ahiThreadTransitionPlaying.store(false);
    m_ahiThreadTransitionPaused.store(false);

    m_ahiThread = Thread::create("Acinerella AHI Pump"_s, [this] {
        ahiThreadEntryPoint();
    });
    
    if (!!m_ahiThread)
    {
        m_ahiThreadReady.wait();

        decodeUntilBufferFull();
        auto lock = Locker(m_lock);
        if (!m_decodedFrames.isEmpty())
        {
            const auto *frame = m_decodedFrames.first().frame();
            dispatch([this, positionToAnnounce(frame->timecode), protectedThis(Ref{*this})]() {
                if (!m_ahiThreadShuttingDown)
                {
                    D(dprintf("[AD]initializeAudio: initialized to %f\n", float(m_position)));
                    m_position = positionToAnnounce;
                    onPositionChanged();
                }
            });
        }

        return true;
    }
    
    return false;
}

void AcinerellaAudioDecoder::signalAHIThread(std::function<void()>&& func)
{
    auto lock = Locker(m_ahiThreadAccessLock);
    if (m_ahiThreadTask)
    {
        func();
        Signal(m_ahiThreadTask, 1UL << m_ahiThreadStateSignal);
    }
}

void AcinerellaAudioDecoder::onThreadShutdown()
{
	ahiCleanup();
}

void AcinerellaAudioDecoder::onGetReadyToPlay()
{
	D(dprintf("[AD]%s: readying %d warmup %d warmedup %d\n", __func__, m_readying, m_warminUp, isWarmedUp()));
	initializeAudio();
}

void AcinerellaAudioDecoder::ahiCleanup()
{
	D(dprintf("[AD]%s:\n", __func__));
	EP_SCOPE(ahiCleanup);

	if (m_ahiThread)
	{
        signalAHIThread([&]() {
            m_ahiThreadShuttingDown = true;
        });
		m_ahiThread->waitForCompletion();
		D(dprintf("[AD]%s: AHI thread shut down... \n", __func__));
		m_ahiThread = nullptr;
	}

	D(dprintf("[AD]%s: done\n", __func__));
}

void AcinerellaAudioDecoder::onFrameDecoded(const AcinerellaDecodedFrame &frame)
{
	auto *avframe = frame.frame();
	m_bufferedSamples += avframe->buffer_size / 4; // 16bitStereo = 4BPF
	m_bufferedSeconds = double(m_bufferedSamples) / double(m_audioRate);

	DSPAM(dprintf("[AD]%s: buffered %f. frametime %f\n", __func__, float(m_bufferedSeconds), float(avframe->timecode)));

    if (m_didUnderrun && m_playing && isWarmedUp())
    {
        signalAHIThread([&](){
            m_ahiThreadTransitionPlaying.store(true);
            m_ahiThreadTransitionPaused.store(false);
        });
    }

#if 0
	if (m_isHLS)// && avframe->timecode <= 0.0)
	{
		auto *nonconstframe = const_cast<ac_decoder_frame *>(avframe);
		nonconstframe->timecode = m_liveTimeCode;
		m_liveTimeCode += (double((avframe->buffer_size / 4)) / double(m_audioRate));
	}
#endif
}

bool AcinerellaAudioDecoder::acceptPackage(RefPtr<AcinerellaPackage>&, double pts)
{
    if (pts < m_position)
        return false;
    return true;
}

double AcinerellaAudioDecoder::position() const
{
	return m_position;
}

void AcinerellaAudioDecoder::flush(bool willSeek)
{
	D(dprintf("[AD]%s: flushing audio\n", __func__));
	EP_SCOPE(flush);

    if (willSeek)
    {
        ahiCleanup();

        AcinerellaDecoder::flush(true);

        m_bufferedSeconds = 0;
        m_bufferedSamples = 0;
        m_position = 0;
        m_ahiFrameOffset = 0;
        m_didUnderrun = false;

        if (m_playing)
        {
            decodeUntilBufferFull();
            startPlaying();
        }
    }
    else
    {
        AcinerellaDecoder::flush(false);
        m_bufferedSeconds = 0;
        m_bufferedSamples = 0;
        if (m_playing)
        {
            decodeUntilBufferFull();
            startPlaying();
        }
    }
}

void AcinerellaAudioDecoder::dumpStatus()
{
	auto lock = Locker(m_lock);
	dprintf("[\033[33mA]: WM %d IR %d PL %d BUF %f BS %d DF %d POS %f LIV %d HLS %d\033[0m\n", isWarmedUp(), isReadyToPlay(), isPlaying(),
		float(bufferSize()), m_bufferedSamples, m_decodedFrames.size(), float(position()), m_isLive, m_isHLS);
}

bool AcinerellaAudioDecoder::fillBuffer(RefPtr<AcinerellaAudioRequest>& request, double desiredPosition)
{
    bool success = false;
    bool didPopFrames = false;

    DFILL(dprintf("[AD]%s: request %p afo %lu\n", __func__, request.get(), m_ahiFrameOffset));

    request->setIsEOF(false);

    {
        auto lock = Locker(m_lock);

        for (;;)
        {
            if (m_decodedFrames.isEmpty())
                break;

            if (m_didUnderrun)
            {
                D(dprintf("[AD]%s: Underrun, %f buffered. warmedup %d\n", __func__, float(bufferSize()), isWarmedUp()));
                if (!isWarmedUp())
                    break;
                m_didUnderrun = false;
            }

            const auto *frame = m_decodedFrames.first().frame();

            DFILL(dprintf("[AD]%s: processing frame @ %f, afo %lu of %lu\n", __func__, float(frame->timecode), m_ahiFrameOffset, frame->buffer_size));

            // skip frames
            if (!m_isLive && desiredPosition > frame->timecode)
            {
                m_bufferedSamples -= frame->buffer_size / 4; // 16bitStereo = 4BPF
                m_bufferedSeconds = double(m_bufferedSamples) / double(m_audioRate);
                m_decodedFrames.removeFirst();
                m_ahiFrameOffset = 0;
                didPopFrames = true;
                continue;
            }

            size_t bytesCopied = request->push(frame->pBuffer + m_ahiFrameOffset, frame->buffer_size - m_ahiFrameOffset);
            m_ahiFrameOffset += bytesCopied;
            request->setTimeToAnnounce(frame->timecode);

            if (frame->buffer_size == int(m_ahiFrameOffset))
            {
                DFILL(dprintf("[AD]%s: pop frame sized %d at %f\n", __func__, frame->buffer_size, float(frame->timecode)));
                m_bufferedSamples -= frame->buffer_size / 4; // 16bitStereo = 4BPF
                m_bufferedSeconds = double(m_bufferedSamples) / double(m_audioRate);
                m_decodedFrames.removeFirst();
                m_ahiFrameOffset = 0;
                didPopFrames = true;
            }
            
            if (request->isFull())
            {
                DFILL(dprintf("[AD]%s: request %p is full, afo %lu, bc %lu\n", __func__, request.get(), m_ahiFrameOffset, bytesCopied));
                success = true;
                break;
            }
        }
    }

    if (!request->isEmpty() && m_decoderEOF)
    {
        success = true;
        request->setIsEOF(true);
    }
    
    if (!request->isFull() && !success)
    {
        m_didUnderrun = true;
        didPopFrames = true;
    }
    
    if (didPopFrames)
    {
        dispatch([this, protectedThis(Ref{*this})]() {
            decodeUntilBufferFull();
        });
    }

    return success;
}

void AcinerellaAudioDecoder::ahiThreadEntryPoint()
{
    DTHREAD(dprintf("[AD]%s: started\n", __func__));

    auto requestA = AcinerellaAudioRequest::create(m_audioRate, m_audioChannels);
    auto requestB = AcinerellaAudioRequest::create(requestA);

    if (!requestA || !requestB || !requestA->isValid() || !requestB->isValid())
    {
        DTHREAD(dprintf("[AD]%s: failed to initialize buffers\n", __func__));
        m_ahiThreadShuttingDown = true;
        m_ahiThreadReady.signal();
        return;
    }

    {
        Locker lock(m_ahiThreadAccessLock);

        m_ahiThreadTask = FindTask(0);
        m_ahiThreadStateSignal = AllocSignal(-1);
    }

    
    requestA->setVolume(m_volume);
    requestB->setVolume(m_volume);

    m_ahiThreadReady.signal();
    DTHREAD(dprintf("[AD]%s: ready\n", __func__));

    const ULONG mpSigBit = 1UL << requestA->sigBit();
    const ULONG comSigBit = 1UL << m_ahiThreadStateSignal;
    bool playing = false;
    double nextPositionToAnnounce = -1.0;
    double desiredPosition = -1.0;

	while (!m_ahiThreadShuttingDown)
	{
		ULONG signals = Wait(mpSigBit | comSigBit);
  
        if (mpSigBit && signals)
        {
            auto requestComplete = AcinerellaAudioRequest::poll(requestA, requestB);
            
            if (!!requestComplete)
            {
                if (requestComplete->isEOF())
                {
                    DTHREAD(dprintf("[AD]%s: ended!\n", __func__));
                    playing = false;

                    dispatch([this, protectedThis(Ref{*this})]() {
                        stopPlaying();
                        if (!m_ahiThreadShuttingDown)
                        {
                            m_position = m_duration;
                            onPositionChanged();
                            onEnded();
                        }
                    });
                }
                else
                {
                    desiredPosition = requestComplete->timeToAnnounce();
                
                    if (requestComplete->timeToAnnounce() > nextPositionToAnnounce)
                    {
                        nextPositionToAnnounce = requestComplete->timeToAnnounce() + 0.2;
                        dispatch([this, positionToAnnounce(requestComplete->timeToAnnounce()), protectedThis(Ref{*this})]() {
                            if (!m_ahiThreadShuttingDown)
                            {
                                m_position = positionToAnnounce;
                                onPositionChanged();
                            }
                        });
                    }
                }

                if (playing)
                {
                    if (fillBuffer(requestComplete, desiredPosition))
                    {
                        requestComplete->play(requestComplete == requestA ? requestB : requestA);
                    }
                    else
                    {
                        playing = false;
                    }
                }
            }

        }
        
        if (m_ahiThreadTransitionPaused.load())
        {
            DTHREAD(dprintf("[AD]%s: >> paused\n", __func__));
            m_ahiThreadTransitionPaused.store(false);
            playing = false;
            SetTaskPri(FindTask(0), 0);
            nextPositionToAnnounce = -1.0;
            requestA->pause();
            requestB->pause();
        }
        else if (m_ahiThreadTransitionPlaying.load())
        {
            DTHREAD(dprintf("[AD]%s: >> playing\n", __func__));
            m_ahiThreadTransitionPlaying.store(false);
            playing = true;
            SetTaskPri(FindTask(0), 50);

            // if both are reporting they have data to play, 
            if (requestA->dataToPlayAfterResuming() && requestB->dataToPlayAfterResuming())
            {
                DTHREAD(dprintf("[AD]%s: >> resuming both...\n", __func__));
                if (requestA->dataToPlayAfterResuming() < requestB->dataToPlayAfterResuming())
                {
                    requestA->play(nullptr);
                    requestB->play(requestA);
                }
                else
                {
                    requestB->play(nullptr);
                    requestA->play(requestB);
                }
            }
            else if (requestA->dataToPlayAfterResuming())
            {
                DTHREAD(dprintf("[AD]%s: >> resuming A...\n", __func__));
                requestB->stop();
                if (fillBuffer(requestB, desiredPosition))
                {
                    requestA->play(nullptr);
                    requestB->play(requestA);
                }
                else
                {
                    playing = false;
                }
            }
            else if (requestB->dataToPlayAfterResuming())
            {
                DTHREAD(dprintf("[AD]%s: >> resuming B...\n", __func__));
                requestA->stop();
                if (fillBuffer(requestA, desiredPosition))
                {
                    requestB->play(nullptr);
                    requestA->play(requestB);
                }
                else
                {
                    playing = false;
                }
            }
            else
            {
                DTHREAD(dprintf("[AD]%s: >> starting both...\n", __func__));
                requestA->stop();
                requestB->stop();
            
                if (fillBuffer(requestA, desiredPosition) && fillBuffer(requestB, desiredPosition))
                {
                    requestA->play(nullptr);
                    requestB->play(requestA);
                }
                else
                {
                    playing = false;
                }
            }
        }
        else if (m_ahiThreadVolumeChanged.load())
        {
            requestA->setVolume(m_volume);
            requestB->setVolume(m_volume);
            m_ahiThreadVolumeChanged.store(false);
        }
	}
 
    SetTaskPri(FindTask(0), 0);
    DTHREAD(dprintf("[AD]%s: bye!\n", __func__));
    Locker lock(m_ahiThreadAccessLock);
    m_ahiThreadTask = nullptr;
    DTHREAD(dprintf("[AD]%s: exiting...\n", __func__));
}

}
}

#undef D
#endif
