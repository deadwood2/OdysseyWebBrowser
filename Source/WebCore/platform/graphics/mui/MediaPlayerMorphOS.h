#pragma once

#if ENABLE(VIDEO)

#include <wtf/Function.h>

#define EP_PROFILING 0
#include <libeventprofiler.h>

namespace WebCore {

class NetworkingContext;
class MediaPlayer;
class Page;

struct MediaPlayerMorphOSInfo
{
	float m_duration = 0;
	int   m_frequency = 0;
	int   m_bits;
	int   m_channels = 0;
	int   m_width = 0;
	int   m_height;
	bool  m_isLive = false;
    bool  m_isDownloadable = false;
    bool  m_isMediaSource = false;
};

struct MediaPlayerMorphOSStreamSettings
{
	bool m_decodeVideo = true;
	
    // NOTE: keep in sync with WkGlobalSettings_LoopFilter
    enum class SkipLoopFilter {
        Default,
        NonRef,
        BiDirectional,
        NonIntra,
        NonKey,
        All
    };

    SkipLoopFilter m_loopFilter = SkipLoopFilter::All;
};

struct MediaPlayerMorphOSSettings
{
public:
    static MediaPlayerMorphOSSettings &settings();

	NetworkingContext *m_networkingContextForRequests = nullptr;

	Function<bool(WebCore::Page *page, const String &host)> m_supportMediaForHost;
	Function<bool(WebCore::Page *page, const String &host)> m_supportMediaSourceForHost;
	Function<bool(WebCore::Page *page, const String &host)> m_supportHLSForHost;
	Function<bool(WebCore::Page *page, const String &host)> m_supportVP9ForHost;
	Function<bool(WebCore::Page *page, const String &host)> m_supportHVCForHost;

	Function<void(WebCore::MediaPlayer *player, const String &url,
		MediaPlayerMorphOSInfo &info, MediaPlayerMorphOSStreamSettings &settings,
		Function<void()> &&yieldFunc)> m_load;
	Function<void(WebCore::MediaPlayer *player)> m_loadCancelled;
	Function<void(WebCore::MediaPlayer *player)> m_willPlay;

	Function<void(WebCore::MediaPlayer *player,
		Function<void(void *windowPtr, int scrollX, int scrollY, int left, int top, int right, int bottom, int width, int height)>&&)> m_overlayRequest;
	Function<void(WebCore::MediaPlayer *player)> m_overlayUpdate;
};

}

#endif
