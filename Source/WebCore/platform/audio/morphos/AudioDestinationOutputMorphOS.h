#pragma once
#include "AudioBus.h"
#include <wtf/MessageQueue.h>
#include <wtf/Threading.h>
#include <wtf/threads/BinarySemaphore.h>
#include <devices/ahi.h>
#include <exec/libraries.h>

namespace WebCore {

class AudioDestinationRenderer
{
public:
	virtual void render(int16_t *samplesStereo16s, size_t count) = 0;
};

class AudioDestinationOutputMorphOS
{
public:
	AudioDestinationOutputMorphOS(AudioDestinationRenderer &parent);
	~AudioDestinationOutputMorphOS();

	bool start();
	void stop();

protected:
	bool ahiInit();
	void ahiCleanup();
	void ahiThreadEntryPoint();

	static void soundFunc();

protected:
	AudioDestinationRenderer& m_renderer;

	Library        *m_ahiBase = nullptr;
	MsgPort        *m_ahiPort = nullptr;
	AHIRequest     *m_ahiIO = nullptr;
	AHIAudioCtrl   *m_ahiControl = nullptr;
	AHISampleInfo   m_ahiSample[2];
	uint32_t        m_ahiSampleLength;
	uint32_t        m_ahiSampleBeingPlayed;

	BinarySemaphore m_ahiSampleConsumed;
	RefPtr<Thread>  m_ahiThread;
	bool            m_ahiThreadShuttingDown = false;
};

} // namespace
