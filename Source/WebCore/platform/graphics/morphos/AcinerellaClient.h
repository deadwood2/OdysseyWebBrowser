#pragma once
#include "MediaPlayerEnums.h"
#include "MediaPlayerMorphOS.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class PlatformMediaResourceLoader;
struct MediaPlayerMorphOSStreamSettings;

namespace Acinerella {

#include <wtf/Seconds.h>

class AcinerellaClient
{
public:
	virtual const MediaPlayerMorphOSStreamSettings& streamSettings() = 0;
	virtual void accInitialized(MediaPlayerMorphOSInfo info) = 0;
	virtual void accUpdated(MediaPlayerMorphOSInfo info) = 0;
	virtual void accSetNetworkState(WebCore::MediaPlayerEnums::NetworkState state) = 0;
	virtual void accSetReadyState(WebCore::MediaPlayerEnums::ReadyState state) = 0;
	virtual void accSetBufferLength(double buffer) = 0;
	virtual void accSetPosition(double position) = 0;
	virtual void accSetDuration(double duration) = 0;
	virtual void accSetVideoSize(int width, int height) = 0;
	virtual void accSetFrameCounts(unsigned decoded, unsigned dropped) = 0;
	virtual void accEnded() = 0;
	virtual void accFailed() = 0; // on initialization!
	virtual void accNextFrameReady() = 0;
	virtual void accNoFramesReady() = 0;
	virtual void accFrameUpdateNeeded() = 0;
	virtual bool accCodecSupported(const String &codec) = 0;
	virtual bool accIsURLValid(const String& url) = 0;
	
	virtual RefPtr<PlatformMediaResourceLoader> accCreateResourceLoader() = 0;
	virtual String accReferrer() = 0;
};

}
}
