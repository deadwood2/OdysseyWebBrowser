#include "AcinerellaAudioDecoder.h"
#include "AcinerellaContainer.h"
#include "MediaPlayerMorphOS.h"

#if ENABLE(VIDEO)
#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#if OS(AROS)
#include <aros/debug.h>
#undef D
#define dprintf bug
#endif

namespace WebCore {
namespace Acinerella {

// #pragma GCC optimize ("O0")

#undef AHI_BASE_NAME
#define AHI_BASE_NAME m_ahiBase

#define D(x)
#define DSYNC(x)
#define DSPAM(x) 
#define DSPAMTS(x) 

AcinerellaAudioDecoder::AcinerellaAudioDecoder(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLiveStream)
	: AcinerellaDecoder(client, acinerella, buffer, index, info, isLiveStream)
	, m_audioRate(ac_get_audio_rate(acinerella->decoder(index)))
	, m_audioChannels(info.additional_info.audio_info.channel_count)
	, m_audioBits(info.additional_info.audio_info.bit_depth)
{
	D(dprintf("[AD]%s: %p\n", __func__, this));
	for (int i = 0; i < 2; i++)
	{
		m_ahiSample[i].ahisi_Address = nullptr;
		m_ahiSampleTimestamp[i] = 0.f;
	}
}

void AcinerellaAudioDecoder::startPlaying()
{
	D(dprintf("[AD]%s: %p\n", __func__, this));
	EP_EVENT(start);
	initializeAudio();
	
	if (m_ahiControl)
	{
		AHI_ControlAudio(m_ahiControl, AHIC_Play, TRUE, TAG_DONE);
		m_playing = true;
	}
}

void AcinerellaAudioDecoder::stopPlaying()
{
	D(dprintf("[AD]%s: %p\n", __func__, this));
	EP_EVENT(stop);
	if (m_ahiControl)
	{
		AHI_ControlAudio(m_ahiControl, AHIC_Play, FALSE, TAG_DONE);
		m_playing = false;
	}
}

void AcinerellaAudioDecoder::onCoolDown()
{
	ahiCleanup();
}

void AcinerellaAudioDecoder::doSetVolume(double volume)
{
	if (m_ahiControl)
	{
		AHI_SetVol(0, (LONG) (double(0x10000L) * volume),
			0x8000L, m_ahiControl, AHISF_IMM);
	}
}

bool AcinerellaAudioDecoder::isReadyToPlay() const
{
	return isWarmedUp() && m_ahiControl;
}

bool AcinerellaAudioDecoder::isWarmedUp() const
{
	return (bufferSize() >= readAheadTime()) || m_decoderEOF;
}

bool AcinerellaAudioDecoder::isPlaying() const
{
	return m_playing;
}

#if OS(AROS)
AROS_UFH3(void, AROS_SoundFunc,
        AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(struct AHIAudioCtrl *, actrl, A2),
        AROS_UFHA(struct AHISoundMessage *, smsg, A1))
{
    AROS_USERFUNC_INIT

	AHIAudioCtrl *ahiCtrl = reinterpret_cast<AHIAudioCtrl *>(actrl);
	AcinerellaAudioDecoder *me = reinterpret_cast<AcinerellaAudioDecoder *>(ahiCtrl->ahiac_UserData);
	AcinerellaAudioDecoder::soundFunc(me);

    AROS_USERFUNC_EXIT
}
#endif


bool AcinerellaAudioDecoder::initializeAudio()
{
	D(dprintf("[AD]%s:\n", __func__));
	EP_SCOPE(initializeAudio);
	
	if (m_ahiControl)
		return true;
	
	if ((m_ahiPort = CreateMsgPort()))
	{
		if ((m_ahiIO = reinterpret_cast<AHIRequest *>(CreateIORequest(m_ahiPort, sizeof(AHIRequest)))))
		{
			m_ahiIO->ahir_Version = 4;
			if (0 == OpenDevice(AHINAME, AHI_NO_UNIT, reinterpret_cast<IORequest *>(m_ahiIO), 0))
			{
				m_ahiBase = reinterpret_cast<Library *>(m_ahiIO->ahir_Std.io_Device);

				D(dprintf("[AD]%s: ahiBase %p\n", __func__, m_ahiBase));

#if OS(MORPHOS)
				static struct EmulLibEntry GATE_SoundFunc = {
					TRAP_LIB, 0, (void (*)(void))AcinerellaAudioDecoder::soundFunc
				};

				static struct Hook __soundHook = {
					{NULL,NULL},
					(ULONG (*)()) &GATE_SoundFunc,
					NULL, NULL,
				};
#endif
#if OS(AROS)
				static struct Hook __soundHook = {
					{NULL,NULL},
					(APTR) AROS_SoundFunc,
					NULL, NULL,
				};
#endif

				if ((m_ahiControl = AHI_AllocAudio(
					AHIA_UserData, reinterpret_cast<IPTR>(this),
					AHIA_AudioID, AHI_DEFAULT_ID,
					AHIA_Sounds, 2, AHIA_Channels, 1,
					AHIA_MixFreq, m_audioRate,
					AHIA_SoundFunc, reinterpret_cast<IPTR>(&__soundHook),
					TAG_DONE)))
				{
					ULONG maxSamples = 0, mixFreq = 0;

					AHI_GetAudioAttrs(AHI_INVALID_ID, m_ahiControl,
						AHIDB_MaxPlaySamples, reinterpret_cast<IPTR>(&maxSamples),
						TAG_DONE);
					AHI_ControlAudio(m_ahiControl,
						AHIC_MixFreq_Query, reinterpret_cast<IPTR>(&mixFreq),
						TAG_DONE);
					
					D(dprintf("[AD]%s: control allocated\n", __func__));
					
					if (maxSamples && mixFreq)
					{
						m_ahiSampleLength = maxSamples * m_audioRate / mixFreq;
						
						m_ahiSampleLength = std::max(m_ahiSampleLength, (m_audioRate / 2 / m_ahiSampleLength) * m_ahiSampleLength);
						
						D(dprintf("[AD]%s: sample length: %d\n", __func__, m_ahiSampleLength));

						for (int i = 0; i < 2; i++)
						{
							m_ahiSample[i].ahisi_Type = AHIST_S16S;
							m_ahiSample[i].ahisi_Length = m_ahiSampleLength;
							m_ahiSample[i].ahisi_Address = malloc(m_ahiSampleLength * 4);
						}
						
						if (m_ahiSample[0].ahisi_Address && m_ahiSample[1].ahisi_Address)
						{
							bzero(m_ahiSample[0].ahisi_Address, m_ahiSampleLength * 4);
							bzero(m_ahiSample[1].ahisi_Address, m_ahiSampleLength * 4);
							
							m_ahiSampleBeingPlayed = 0;
							m_ahiThreadShuttingDown = false;
							m_ahiFrameOffset = 0;
							m_didFlushBuffers = true;
							
							fillBuffer(0);
							
							m_ahiThread = Thread::create("Acinerella AHI Pump", [this] {
								ahiThreadEntryPoint();
							});
							
							if (0 == AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, reinterpret_cast<APTR>(&m_ahiSample[0]), m_ahiControl)
								&& 0 == AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, reinterpret_cast<APTR>(&m_ahiSample[1]), m_ahiControl))
							{
								D(dprintf("[AD]%s: samples loaded - samplelen %d\n", __func__, m_ahiSampleLength));
								if (0 == AHI_ControlAudio(m_ahiControl, AHIC_Play, FALSE, TAG_DONE))
								{
									D(dprintf("[AD]%s: calling AHI_Play!\n", __func__));
									AHI_Play(m_ahiControl,
										AHIP_BeginChannel, 0,
										AHIP_Freq, m_audioRate,
										AHIP_Vol, 0x10000L, AHIP_Pan, 0x8000L,
										AHIP_Sound, 0, AHIP_Offset, 0, AHIP_Length, 0,
										AHIP_EndChannel, 0,
										TAG_DONE);
									m_playing = false;
									return true;
								}
								else
								{
									D(dprintf("[AD]%s: AHI_ControlAudio failed\n", __func__));
								}
							}
						}
						
						for (int i = 0; i < 2; i++)
						{
							AHI_UnloadSound(i, m_ahiControl);
							free(m_ahiSample[i].ahisi_Address);
							m_ahiSample[i].ahisi_Address = nullptr;
						}
					}

					AHI_FreeAudio(m_ahiControl);
					m_ahiControl = 0;
				}
			}
			
			DeleteIORequest(reinterpret_cast<IORequest *>(m_ahiIO));
			m_ahiIO = nullptr;
			m_ahiBase = nullptr;
		}
		
		DeleteMsgPort(m_ahiPort);
		m_ahiPort = nullptr;
	}
	
	return false;
}

void AcinerellaAudioDecoder::onThreadShutdown()
{
	ahiCleanup();
}

void AcinerellaAudioDecoder::onGetReadyToPlay()
{
	D(dprintf("[AD]%s: waplay %d readying %d warmup %d warmedup %d\n", __func__, m_waitingToPlay, m_readying, m_warminUp, isWarmedUp()));
	initializeAudio();
}

void AcinerellaAudioDecoder::ahiCleanup()
{
	D(dprintf("[AD]%s:\n", __func__));
	EP_SCOPE(ahiCleanup);

	if (m_ahiControl)
	{
		AHI_ControlAudio(m_ahiControl, AHIC_Play, FALSE, TAG_DONE);
	}

	if (m_ahiThread)
	{
		m_ahiThreadShuttingDown = true;
		if (m_pumpTask)
			Signal(m_pumpTask, SIGF_SINGLE);
	}

	if (m_ahiControl)
	{
		AHI_FreeAudio(m_ahiControl);
		m_ahiControl = nullptr;
		D(dprintf("[AD]%s: AHI control disposed\n", __func__));
	}

	if (m_ahiThread)
	{
		m_ahiThread->waitForCompletion();
		D(dprintf("[AD]%s: AHI thread shut down... \n", __func__));
		m_ahiThread = nullptr;
	}

	for (int i = 0; i < 2; i++)
	{
		free(m_ahiSample[i].ahisi_Address);
		m_ahiSample[i].ahisi_Address = nullptr;
	}

	if (m_ahiIO)
	{
		CloseDevice(reinterpret_cast<IORequest *>(m_ahiIO));
		DeleteIORequest(reinterpret_cast<IORequest *>(m_ahiIO));
		m_ahiIO = nullptr;
		m_ahiBase = nullptr;
	}

	if (m_ahiPort)
	{
		DeleteMsgPort(m_ahiPort);
		m_ahiPort = nullptr;
	}

	D(dprintf("[AD]%s: done\n", __func__));
}

void AcinerellaAudioDecoder::onFrameDecoded(const AcinerellaDecodedFrame &frame)
{
	m_bufferedSamples += frame.frame()->buffer_size / 4; // 16bitStereo = 4BPF
	m_bufferedSeconds = double(m_bufferedSamples) / double(m_audioRate);

	DSPAM(dprintf("[AD]%s: buffered %f\n", __func__, m_bufferedSeconds));
}

double AcinerellaAudioDecoder::position() const
{
	return m_position;
}

void AcinerellaAudioDecoder::flush()
{
	D(dprintf("[AD]%s: flushing audio\n", __func__));
	EP_SCOPE(flush);

	if (m_ahiControl)
	{
		AHI_ControlAudio(m_ahiControl, AHIC_Play, FALSE, TAG_DONE);
		AHI_UnloadSound(0, m_ahiControl);
		AHI_UnloadSound(1, m_ahiControl);
	}
	
	AcinerellaDecoder::flush();

	m_ahiFrameOffset = 0;
	m_bufferedSeconds = 0;
	m_bufferedSamples = 0;
	m_didFlushBuffers = true;

	if (m_ahiControl)
	{
		bzero(m_ahiSample[0].ahisi_Address, m_ahiSampleLength * 4);
		bzero(m_ahiSample[1].ahisi_Address, m_ahiSampleLength * 4);

		AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, reinterpret_cast<APTR>(&m_ahiSample[0]), m_ahiControl);
		AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, reinterpret_cast<APTR>(&m_ahiSample[1]), m_ahiControl);
	}

		D({
			auto lock = holdLock(m_lock);
			if (!m_decodedFrames.empty())
			{
				dprintf("First audio frame @ %f\n", float(m_decodedFrames.front().frame()->timecode));
			}
			else
			{
				dprintf("Decoder empty\n");
			}
		});

	if (m_playing && m_ahiControl)
	{
		decodeUntilBufferFull();

		uint32_t index = m_ahiSampleBeingPlayed % 2; // this sample will play next
		fillBuffer(index);
		AHI_ControlAudio(m_ahiControl, AHIC_Play, TRUE, TAG_DONE);
	}
}

void AcinerellaAudioDecoder::dumpStatus()
{
	auto lock = holdLock(m_lock);
	dprintf("[\033[33mA]: WM %d IR %d PL %d BUF %f BS %d DF %d POS %f\033[0m\n", isWarmedUp(), isReadyToPlay(), isPlaying(),
		float(bufferSize()), m_bufferedSamples, m_decodedFrames.size(), float(position()));
}

#undef AHI_BASE_NAME
#define AHI_BASE_NAME me->m_ahiBase
#if OS(MORPHOS)
void AcinerellaAudioDecoder::soundFunc()
{
	AHIAudioCtrl *ahiCtrl = reinterpret_cast<AHIAudioCtrl *>(REG_A2);
	AcinerellaAudioDecoder *me = reinterpret_cast<AcinerellaAudioDecoder *>(ahiCtrl->ahiac_UserData);
#endif
#if OS(AROS)
void AcinerellaAudioDecoder::soundFunc(void *ptr)
{
	AcinerellaAudioDecoder *me = reinterpret_cast<AcinerellaAudioDecoder *>(ptr);
#endif
	
	me->m_ahiSampleBeingPlayed ++;
	AHI_SetSound(0, me->m_ahiSampleBeingPlayed % 2, 0, 0, me->m_ahiControl, 0);

	// D(dprintf("[AD]%s: setSound %d (%d)\n", __func__, me->m_ahiSampleBeingPlayed % 2, me->m_ahiSampleBeingPlayed));

	Signal(me->m_pumpTask, SIGF_SINGLE);
}

void AcinerellaAudioDecoder::fillBuffer(int index)
{
	EP_SCOPE(fillBuffer);
	uint32_t offset = 0;
	uint32_t bytesLeft = m_ahiSampleLength * 4;
	bool didPopFrames = false;
	bool didSetTimecode = false;

	DSPAM(dprintf("[AD]%s: sample %d, next index: %d\n", __func__, m_ahiSampleBeingPlayed, index));

	{
		auto lock = holdLock(m_lock);
		while (!m_decodedFrames.empty() && bytesLeft)
		{
			if (0 == bytesLeft)
				break;

			if (m_decodedFrames.empty())
				break;

			if (m_didUnderrun)
			{
				D(dprintf("[AD]%s: Underrun, %f buddered\n", __func__, float(bufferSize())));
				if (!isWarmedUp())
					break;
				m_didUnderrun = false;
			}

			const auto *frame = m_decodedFrames.front().frame();
			size_t copyBytes = std::min(frame->buffer_size - m_ahiFrameOffset, bytesLeft);
			
			DSPAM(dprintf("[AD]%s: cb %d of %d afo %d bs %d\n", __func__, copyBytes, offset, m_ahiFrameOffset, frame->buffer_size));
			
			memcpy(reinterpret_cast<uint8_t*>(m_ahiSample[index].ahisi_Address) + offset, frame->pBuffer + m_ahiFrameOffset, copyBytes);
			
			m_ahiFrameOffset += copyBytes;
			offset += copyBytes;
			bytesLeft -= copyBytes;

			if (!didSetTimecode)
			{
				m_ahiSampleTimestamp[index] = frame->timecode;
				if (m_didFlushBuffers) // index should always be 0 with initial
				{
					m_ahiSampleTimestamp[1] = m_ahiSampleTimestamp[0] = frame->timecode;
					m_didFlushBuffers = false;
				}
				didSetTimecode = true;
				DSPAMTS(dprintf("[AD]%s: set timecode %f\n", __func__, float(frame->timecode)));
			}
		
			if (frame->buffer_size == int(m_ahiFrameOffset))
			{
				DSPAM(dprintf("[AD]%s: pop frame sized %d at %f\n", __func__, frame->buffer_size, float(frame->timecode)));
				m_bufferedSamples -= frame->buffer_size / 4; // 16bitStereo = 4BPF
				m_bufferedSeconds = double(m_bufferedSamples) / double(m_audioRate);
				m_decodedFrames.pop();
				m_ahiFrameOffset = 0;
				didPopFrames = true;
			}
		}
	}

	if (bytesLeft > 0)
	{
		// Clear remaining buffer in case of an underrun
		bzero(reinterpret_cast<uint8_t*>(m_ahiSample[index].ahisi_Address) + offset, bytesLeft);
		m_didUnderrun = true;
	}

#if 0
				BPTR f = Open("sys:out.raw", MODE_READWRITE);
				if (f)
				{
					Seek(f, 0, OFFSET_END);
					Write(f, m_ahiSample[index].ahisi_Address, m_ahiSampleLength * 4);
					Close(f);
				}
#endif

	if (m_isLive)
	{
		if (didPopFrames)
		{
			dispatch([this, protectedThis(makeRef(*this))]() {
				if (!m_ahiThreadShuttingDown)
				{
					m_position += 0.5f;
					onPositionChanged();
					decodeUntilBufferFull();
				}
			});
		}
	}
	else
	{
		double positionToAnnounce = index == 0 ? m_ahiSampleTimestamp[1] : m_ahiSampleTimestamp[0];
		
		DSYNC(dprintf("[A] %s: PTS %f, bleft %d offset %d timeleft %f dp %d eof %d\n", __func__, positionToAnnounce, bytesLeft, offset, m_bufferedSeconds, didPopFrames, m_decoderEOF));

		if (didPopFrames)
		{
			dispatch([this, positionToAnnounce, protectedThis(makeRef(*this))]() {
				if (!m_ahiThreadShuttingDown)
				{
					m_position = positionToAnnounce;
					onPositionChanged();
					decodeUntilBufferFull();
				}
			});
		}
	}
	
	if ((bytesLeft == m_ahiSampleLength * 4) && (m_decoderEOF || (m_position + 1.5f > m_duration && !m_isLive)))
	{
		dispatch([this, protectedThis(makeRef(*this))]() {
			stopPlaying();
			if (!m_ahiThreadShuttingDown)
			{
				m_position = m_duration;
				onEnded();
			}
		});
	}
}

void AcinerellaAudioDecoder::ahiThreadEntryPoint()
{
	m_pumpTask = FindTask(0);
	SetTaskPri(m_pumpTask, 50);
	while (!m_ahiThreadShuttingDown)
	{
		Wait(SIGF_SINGLE);
		uint32_t index = m_ahiSampleBeingPlayed % 2; // this sample will play next
		fillBuffer(index);
	}
}


}
}

#undef D
#endif
