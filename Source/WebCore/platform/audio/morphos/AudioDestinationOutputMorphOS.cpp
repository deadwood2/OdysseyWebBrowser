#include "config.h"

#if ENABLE(WEB_AUDIO)
#include "AudioDestinationOutputMorphOS.h"
#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#undef AHI_BASE_NAME
#define AHI_BASE_NAME m_ahiBase

#define D(x) 

namespace WebCore {

AudioDestinationOutputMorphOS::AudioDestinationOutputMorphOS(AudioDestinationRenderer &parent)
	: m_renderer(parent)
{
	for (int i = 0; i < 2; i++)
	{
		m_ahiSample[i].ahisi_Address = nullptr;
	}
}

AudioDestinationOutputMorphOS::~AudioDestinationOutputMorphOS()
{
	ahiCleanup();
}

bool AudioDestinationOutputMorphOS::start()
{
	if (!ahiInit())
		return false;

	D(dprintf("[AD]%s:\n", __func__));
	AHI_ControlAudio(m_ahiControl, AHIC_Play, TRUE, TAG_DONE);
	return true;
}

void AudioDestinationOutputMorphOS::stop()
{
	D(dprintf("[AD]%s:\n", __func__));
	if (m_ahiControl)
		AHI_ControlAudio(m_ahiControl, AHIC_Play, FALSE, TAG_DONE);
}

#if OS(AROS)
AROS_UFH3(void, AROS_SoundFunc,
        AROS_UFHA(struct Hook *, hook, A0),
        AROS_UFHA(struct AHIAudioCtrl *, actrl, A2),
        AROS_UFHA(struct AHISoundMessage *, smsg, A1))
{
    AROS_USERFUNC_INIT

	AHIAudioCtrl *ahiCtrl = reinterpret_cast<AHIAudioCtrl *>(actrl);
	AudioDestinationOutputMorphOS *me = reinterpret_cast<AudioDestinationOutputMorphOS *>(ahiCtrl->ahiac_UserData);
	AudioDestinationOutputMorphOS::soundFunc(me);

    AROS_USERFUNC_EXIT
}
#endif

bool AudioDestinationOutputMorphOS::ahiInit()
{
	D(dprintf("[AD]%s:\n", __func__));
	
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
					TRAP_LIB, 0, (void (*)(void))AudioDestinationOutputMorphOS::soundFunc
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
					AHIA_MixFreq, 44100,
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
						m_ahiSampleLength = maxSamples * 44100 / mixFreq;
						m_ahiSampleLength = std::max(m_ahiSampleLength, (44100 / 2 / m_ahiSampleLength) * m_ahiSampleLength);
						m_ahiSampleLength = (m_ahiSampleLength / 128) * 128;

						D(dprintf("[AD]%s: sample length: %d\n", __func__, m_ahiSampleLength));

						for (int i = 0; i < 2; i++)
						{
							m_ahiSample[i].ahisi_Type = AHIST_S16S;
							m_ahiSample[i].ahisi_Length = m_ahiSampleLength;
							m_ahiSample[i].ahisi_Address = fastAlignedMalloc(16, (m_ahiSampleLength * 4) + 16); // reserve a little extra data at the end to simplify render routine!
						}
						
						if (m_ahiSample[0].ahisi_Address && m_ahiSample[1].ahisi_Address)
						{
							bzero(m_ahiSample[0].ahisi_Address, m_ahiSampleLength * 4);
							bzero(m_ahiSample[1].ahisi_Address, m_ahiSampleLength * 4);

							m_ahiSampleBeingPlayed = 0;

							if (0 == AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, reinterpret_cast<APTR>(&m_ahiSample[0]), m_ahiControl)
								&& 0 == AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, reinterpret_cast<APTR>(&m_ahiSample[1]), m_ahiControl))
							{
								D(dprintf("[AD]%s: samples loaded - samplelen %d\n", __func__, m_ahiSampleLength));

								m_ahiThreadShuttingDown = false;
								m_ahiThread = Thread::create("WebAudio AHI Pump", [this] {
									ahiThreadEntryPoint();
								});

								if (0 == AHI_ControlAudio(m_ahiControl, AHIC_Play, FALSE, TAG_DONE))
								{
									D(dprintf("[AD]%s: calling AHI_Play!\n", __func__));
									AHI_Play(m_ahiControl,
										AHIP_BeginChannel, 0,
										AHIP_Freq, 44100,
										AHIP_Vol, 0x10000L, AHIP_Pan, 0x8000L,
										AHIP_Sound, 0, AHIP_Offset, 0, AHIP_Length, 0,
										AHIP_EndChannel, 0,
										TAG_DONE);
								}
								else
								{
									D(dprintf("[AD]%s: AHI_ControlAudio failed\n", __func__));
								}
							}

							return true;
						}

						for (int i = 0; i < 2; i++)
						{
							fastAlignedFree(m_ahiSample[i].ahisi_Address);
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

void AudioDestinationOutputMorphOS::ahiCleanup()
{
	D(dprintf("[AD]%s:\n", __func__));

	if (m_ahiControl)
	{
		AHI_ControlAudio(m_ahiControl, AHIC_Play, FALSE, TAG_DONE);
		AHI_FreeAudio(m_ahiControl);
		m_ahiControl = nullptr;
		D(dprintf("[AD]%s: AHI control disposed\n", __func__));
	}

	if (m_ahiThread)
	{
		m_ahiThreadShuttingDown = true;
		if (m_pumpTask)
			Signal(m_pumpTask, SIGF_SINGLE);
	}

	for (int i = 0; i < 2; i++)
	{
		fastAlignedFree(m_ahiSample[i].ahisi_Address);
		m_ahiSample[i].ahisi_Address = nullptr;
	}

	if (m_ahiThread)
	{
		m_ahiThread->waitForCompletion();
		D(dprintf("[AD]%s: AHI thread shut down... \n", __func__));
		m_ahiThread = nullptr;
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

#undef AHI_BASE_NAME
#define AHI_BASE_NAME me->m_ahiBase
#if OS(MORPHOS)
void AudioDestinationOutputMorphOS::soundFunc()
{
	AHIAudioCtrl *ahiCtrl = reinterpret_cast<AHIAudioCtrl *>(REG_A2);
	AudioDestinationOutputMorphOS *me = reinterpret_cast<AudioDestinationOutputMorphOS *>(ahiCtrl->ahiac_UserData);
#endif
#if OS(AROS)
void AudioDestinationOutputMorphOS::soundFunc(void *ptr)
{
	AudioDestinationOutputMorphOS *me = reinterpret_cast<AudioDestinationOutputMorphOS *>(ptr);
#endif
	
	me->m_ahiSampleBeingPlayed ++;
	AHI_SetSound(0, me->m_ahiSampleBeingPlayed % 2, 0, 0, me->m_ahiControl, 0);

	Signal(me->m_pumpTask, SIGF_SINGLE);
}

void AudioDestinationOutputMorphOS::ahiThreadEntryPoint()
{
	m_pumpTask = FindTask(0);
	SetTaskPri(m_pumpTask, 2);
	while (!m_ahiThreadShuttingDown)
	{
		Wait(SIGF_SINGLE);
		uint32_t index = m_ahiSampleBeingPlayed % 2; // this sample will play next
		m_renderer.render((int16_t *)(m_ahiSample[index].ahisi_Address), m_ahiSampleLength);
	}
}

}
#endif

