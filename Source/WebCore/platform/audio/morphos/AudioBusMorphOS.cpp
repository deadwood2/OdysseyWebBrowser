#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioBus.h"

namespace WebCore {

RefPtr<AudioBus> AudioBus::loadPlatformResource(const char* name, float sampleRate)
{
	return nullptr;
}

}

#endif

