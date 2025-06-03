#include "MediaPlayerPrivateMorphOS.h"

#if ENABLE(VIDEO)

#include "GraphicsContext.h"
#include "MediaPlayer.h"
#include "MediaSourcePrivateClient.h"
#include "NotImplemented.h"
#include "AcinerellaContainer.h"
#include "AudioTrackPrivateMorphOS.h"
#include "VideoTrackPrivateMorphOS.h"

#include "HTMLMediaElement.h"
#include "Frame.h"
#include "Page.h"
#include "CommonVM.h"
#include <proto/exec.h>
#include <exec/exec.h>

#if OS(AROS)
#include <aros/debug.h>
#undef D
#define dprintf bug
#endif

#define D(x)
#define DM(x)
#define DMHOST(x) 

namespace WebCore {

MediaPlayerMorphOSSettings &MediaPlayerMorphOSSettings::settings()
{
	static MediaPlayerMorphOSSettings m_playerSettings;
	return m_playerSettings;
}

class MediaPlayerFactoryMediaSourceMorphOS final : public MediaPlayerFactory {
public:
    MediaPlayerEnums::MediaEngineIdentifier identifier() const final { return MediaPlayerEnums::MediaEngineIdentifier::MorphOS; };

    std::unique_ptr<MediaPlayerPrivateInterface> createMediaEnginePlayer(MediaPlayer* player) const final { return makeUnique<MediaPlayerPrivateMorphOS>(player); }

    static bool isCGXVideoValid()
    {
        static bool checkDone = false;
        static bool valid;
        if (!checkDone)
        {
            struct Library *cgx = OpenLibrary("cgxvideo.library", 43);
            if (cgx)
            {
                valid = false;
                if (cgx->lib_Version > 43)
                    valid = true;
                else if (cgx->lib_Revision >= 18)
                    valid = true;
                CloseLibrary(cgx);
            }
            checkDone = true;
            if (!valid)
                dprintf("Wayfarer: please make sure cgxvideo.library 43.18 is installed in MOSSYS:Libs!\n");
        }
        return valid;
    }

    static void s_getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types, bool withHLS)
    {
		// Audio
		types.add(String("audio/aac"));
		types.add(String("audio/basic"));
		types.add(String("audio/mp3"));
		types.add(String("audio/mp4"));
		types.add(String("audio/flac"));
		types.add(String("audio/mpeg"));
		types.add(String("audio/vnd.wave"));
		types.add(String("audio/wav"));
		types.add(String("audio/wave"));

		types.add(String("audio/x-aiff"));
		types.add(String("audio/x-flac"));
		types.add(String("audio/x-m4a"));
		types.add(String("audio/x-pn-wav"));
		types.add(String("audio/x-wav"));

		types.add(String("audio/ogg"));
		types.add(String("audio/webm"));

		types.add(String("audio/x-scpls"));
		types.add(String("audio/mpa"));
		types.add(String("audio/mpa-robust"));

		// Video
		types.add(String("video/avi"));
		types.add(String("video/flv"));
		types.add(String("video/mp4"));
		types.add(String("video/3gpp")); // this is mp4
		types.add(String("video/vnd.objectvideo"));
		types.add(String("video/x-flv"));

		types.add(String("video/ogg"));
		types.add(String("video/x-theora+ogg"));
		types.add(String("video/webm"));

		// HLS
		if (withHLS)
		{
			types.add(String("audio/x-mpegurl"));
			types.add(String("application/x-mpegurl"));
			types.add(String("application/vnd.apple.mpegurl"));
		}
    }

	void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types) const final
	{
		s_getSupportedTypes(types, true);
	}

    static MediaPlayer::SupportsType s_supportsTypeAndCodecs(const MediaEngineSupportParameters& parameters)
    {
		String host;
		Page *page = parameters.page;
		
		DMHOST(dprintf("%s: page %p\n", __func__, page));
		
		if (nullptr == page)
		{
			Frame* frame = lexicalFrameFromCommonVM();
			DMHOST(dprintf("%s: vmframe %p\n", __func__, frame));
			if (frame)
			{
				Document *doc = frame->mainFrame().document();
				DMHOST(dprintf("%s: doc %p\n", __func__, doc));
				if (doc)
				{
					host = doc->url().host().toString();
					page = doc->page();
				}
			}
			else if (parameters.url.isValid())
			{
				host = parameters.url.host().toString();
			}
		}
		else
		{
			Document *doc = page->mainFrame().document();
			DMHOST(dprintf("%s: doc %p urlvalid %d (%s)\n", __func__, doc, parameters.url.isValid(), parameters.url.string().utf8().data()));
			if (doc)
			{
				host = doc->url().host().toString();
			}
			else if (parameters.url.isValid())
			{
				host = parameters.url.host().toString();
			}
		}
    
    	if (startsWithLettersIgnoringASCIICase(parameters.type.raw(), "image/"))
    	{
    		return MediaPlayer::SupportsType::IsNotSupported;
		}
		
    	if (startsWithLettersIgnoringASCIICase(parameters.url.string(), "data:"))
    	{
    		return MediaPlayer::SupportsType::IsNotSupported;
		}

#if !OS(AROS)
        if (!isCGXVideoValid())
        {
    		return MediaPlayer::SupportsType::IsNotSupported;
		}
#endif

		bool withHLS = true;

		DM(dprintf("%s: url '%s' content '%s' ctype '%s' isource %d istream %d profiles %d hlsOK %d host '%s' page %p\n", __func__,
			parameters.url.string().utf8().data(), parameters.type.raw().utf8().data(), parameters.type.containerType().utf8().data(),
			parameters.isMediaSource, parameters.isMediaStream, parameters.type.profiles().size(), withHLS,
			host.utf8().data(), page));

		if (MediaPlayerMorphOSSettings::settings().m_supportMediaForHost && !parameters.isMediaSource &&
			!MediaPlayerMorphOSSettings::settings().m_supportMediaForHost(page, host))
		{
			DM(dprintf("%s: rejecting due to supportMediaForHost check on host '%s'...\n", __func__, host.utf8().data()));
			return MediaPlayer::SupportsType::IsNotSupported;
		}

		if (MediaPlayerMorphOSSettings::settings().m_supportMediaSourceForHost && parameters.isMediaSource &&
			!MediaPlayerMorphOSSettings::settings().m_supportMediaSourceForHost(page, host))
		{
			DM(dprintf("%s: rejecting due to supportMediaSourceForHost check...\n", __func__));
			return MediaPlayer::SupportsType::IsNotSupported;
		}

    	if (startsWithLettersIgnoringASCIICase(parameters.url.string(), "blob:"))
    	{
#if ENABLE(MEDIA_SOURCE)
			if (!parameters.isMediaSource)
#endif
			return MediaPlayer::SupportsType::IsNotSupported;
		}
		
       	auto containerType = parameters.type.containerType();
		if (containerType.isEmpty())
		{
			DM(dprintf("%s: container empty, assume 'maybe'\n", __func__));
			return MediaPlayer::SupportsType::MayBeSupported;
		}
		HashSet<String, ASCIICaseInsensitiveHash> types;
		s_getSupportedTypes(types, withHLS);
		DM(dprintf("%s: '%s' contained in list? %d\n", __func__, parameters.type.containerType().utf8().data(), types.contains(containerType)));
		if (types.contains(containerType))
		{
			auto codecs = parameters.type.codecs();
			if (codecs.isEmpty())
			{
				DM(dprintf("%s: codecs empty, assume 'maybe'\n", __func__));
				return MediaPlayer::SupportsType::MayBeSupported;
			}

			DM(dprintf("%s: lists %d codecs\n", __func__, codecs.size()));
			for (size_t i = 0; i < codecs.size(); i++)
			{
				auto &codec = codecs.at(i);
				if (startsWithLettersIgnoringASCIICase(codec, "av01") || startsWithLettersIgnoringASCIICase(codec, "av1")) // requires ffmpeg 4.0 + additional libs
				{
					DM(dprintf("%s: rejecting unsupported codec %s\n", __func__, codec.utf8().data()));
					return MediaPlayer::SupportsType::IsNotSupported;
				}
#if 0
				// higher profile h264 seem to fail decoding (on vimeo, but work on yt!)
				else if (startsWithLettersIgnoringASCIICase(codec, "avc1.5") || startsWithLettersIgnoringASCIICase(codec, "avc1.6") || startsWithLettersIgnoringASCIICase(codec, "avc1.7") || startsWithLettersIgnoringASCIICase(codec, "avc1.8") || startsWithLettersIgnoringASCIICase(codec, "avc1.f"))
				{
					DM(dprintf("%s: rejecting unsupported codec %s\n", __func__, codec.utf8().data()));
					return MediaPlayer::SupportsType::IsNotSupported;
				}
#endif
				else if (startsWithLettersIgnoringASCIICase(codec, "hvc1")) // not enabled in ffmpeg (h265 variant)
				{
					if (MediaPlayerMorphOSSettings::settings().m_supportHVCForHost && !MediaPlayerMorphOSSettings::settings().m_supportHVCForHost(page, host))
					{
						DM(dprintf("%s: rejecting user disabled codec %s\n", __func__, codec.utf8().data()));
						return MediaPlayer::SupportsType::IsNotSupported;
					}
				}
                else if (startsWithLettersIgnoringASCIICase(codec, "vp9"))
                {
					if (MediaPlayerMorphOSSettings::settings().m_supportVP9ForHost && !MediaPlayerMorphOSSettings::settings().m_supportVP9ForHost(page, host))
					{
						DM(dprintf("%s: rejecting user disabled codec %s\n", __func__, codec.utf8().data()));
						return MediaPlayer::SupportsType::IsNotSupported;
					}
                }
				else
				{
					DM(dprintf("%s: we should be OK with codec %s\n", __func__, codec.utf8().data()));
				}
			}

			return MediaPlayer::SupportsType::IsSupported;
		}
		DM(dprintf("%s: not supported!\n", __func__));
        return MediaPlayer::SupportsType::IsNotSupported;
    }
	
    MediaPlayer::SupportsType supportsTypeAndCodecs(const MediaEngineSupportParameters& parameters) const final
    {
    	return s_supportsTypeAndCodecs(parameters);
	}
};

MediaPlayerPrivateMorphOS::MediaPlayerPrivateMorphOS(MediaPlayer* player)
	: m_player(player)
{
	notImplemented();
}

MediaPlayerPrivateMorphOS::~MediaPlayerPrivateMorphOS()
{
	if (m_acinerella)
		m_acinerella->terminate();

	// remove all pending requests that could be referencing 'this'
	if (MediaPlayerMorphOSSettings::settings().m_loadCancelled)
		MediaPlayerMorphOSSettings::settings().m_loadCancelled(m_player);
}

void MediaPlayerPrivateMorphOS::registerMediaEngine(MediaEngineRegistrar registrar)
{
	registrar(makeUnique<MediaPlayerFactoryMediaSourceMorphOS>());
}

MediaPlayer::SupportsType MediaPlayerPrivateMorphOS::extendedSupportsType(const MediaEngineSupportParameters& parameters, MediaPlayer::SupportsType type)
{
	(void)type;
	return MediaPlayerFactoryMediaSourceMorphOS::s_supportsTypeAndCodecs(parameters);
}

bool MediaPlayerPrivateMorphOS::supportsKeySystem(const String& keySystem, const String& mimeType)
{
	if (equalIgnoringASCIICase(keySystem, "org.w3c.clearkey") && !mimeType.isEmpty() && equalIgnoringASCIICase(mimeType, "application/x-mpegurl"))
		return true;
	return false;
}

void MediaPlayerPrivateMorphOS::load(const String& url)
{
	D(dprintf("%s: %s\n", __PRETTY_FUNCTION__, url.utf8().data()));

	cancelLoad();

	if (startsWithLettersIgnoringASCIICase(url, "about:"))
		return;

	if (!canLoad(false))
		return;

	m_networkState = MediaPlayer::NetworkState::Loading;
	m_player->networkStateChanged();
	m_readyState = MediaPlayer::ReadyState::HaveNothing;
	m_player->readyStateChanged();

#if 1
MediaPlayerMorphOSSettings::settings().m_networkingContextForRequests =
m_player->client().mediaPlayerPage()->mainFrame().loader().networkingContext();

MediaPlayerMorphOSSettings::settings().m_load = [](WebCore::MediaPlayer *player, const String &url, WebCore::MediaPlayerMorphOSInfo& info,
		MediaPlayerMorphOSStreamSettings &settings, Function<void()> &&yieldFunc) {
	};

#endif

	m_acinerella = Acinerella::Acinerella::create(this, url);
}

#if ENABLE(MEDIA_SOURCE)
void MediaPlayerPrivateMorphOS::load(const URL& url, const ContentType&, MediaSourcePrivateClient* client)
{
	D(dprintf("%s: %s\n", __PRETTY_FUNCTION__, url.string().utf8().data()));
	cancelLoad();

	if (startsWithLettersIgnoringASCIICase(url.string(), "about:"))
		return;
		
	if (!canLoad(true))
		return;

	m_networkState = MediaPlayer::NetworkState::Loading;
	m_player->networkStateChanged();
	m_readyState = MediaPlayer::ReadyState::HaveNothing;
	m_player->readyStateChanged();

	m_mediaSourcePrivate = MediaSourcePrivateMorphOS::create(*this, *client, url.string());
}
#endif

bool MediaPlayerPrivateMorphOS::canLoad(bool isMediaSource)
{
	Page *page = m_player->client().mediaPlayerPage();
	String host;
	Document *doc = page ? page->mainFrame().document() : nullptr;
	if (doc)
	{
		host = doc->url().host().toString();
	}

	D(dprintf("%s: page %p doc %p host %s\n", __PRETTY_FUNCTION__, page, doc, host.utf8().data()));

	bool ok = false;

	if (page)
	{
		if (isMediaSource)
		{
			ok = !MediaPlayerMorphOSSettings::settings().m_supportMediaSourceForHost ||
				MediaPlayerMorphOSSettings::settings().m_supportMediaSourceForHost(page, host);
		}
		else
		{
			ok = !MediaPlayerMorphOSSettings::settings().m_supportMediaForHost ||
				MediaPlayerMorphOSSettings::settings().m_supportMediaForHost(page, host);
		}
	}

	if (!ok)
	{
		m_networkState = WebCore::MediaPlayerEnums::NetworkState::FormatError;
		m_readyState = WebCore::MediaPlayerEnums::ReadyState::HaveNothing;
		m_player->networkStateChanged();
		m_player->readyStateChanged();
	}

	return ok;
}

void MediaPlayerPrivateMorphOS::cancelLoad()
{
	D(dprintf("%s:\n", __PRETTY_FUNCTION__));

	if (MediaPlayerMorphOSSettings::settings().m_loadCancelled)
		MediaPlayerMorphOSSettings::settings().m_loadCancelled(m_player);

#if ENABLE(MEDIA_SOURCE)
	if (m_mediaSourcePrivate)
		m_mediaSourcePrivate->orphan();
	m_mediaSourcePrivate = nullptr;
#endif

	m_prepareToPlay = m_acInitialized = false;
	pause();

	if (m_acinerella)
		m_acinerella->terminate();
}

void MediaPlayerPrivateMorphOS::prepareToPlay()
{
	D(dprintf("%s:\n", __PRETTY_FUNCTION__));
	m_prepareToPlay = true;

#if ENABLE(MEDIA_SOURCE)
	if (m_mediaSourcePrivate)
		m_mediaSourcePrivate->warmUp();
#endif

	if (m_acinerella && m_acInitialized)
		m_acinerella->warmUp();
}

bool MediaPlayerPrivateMorphOS::canSaveMediaData() const
{
	D(dprintf("%s:\n", __PRETTY_FUNCTION__));
	if (m_acinerella && !m_acinerella->isLive())
		return true;
	return false;
}

void MediaPlayerPrivateMorphOS::play()
{
	if (MediaPlayerMorphOSSettings::settings().m_willPlay)
		MediaPlayerMorphOSSettings::settings().m_willPlay(m_player);

	if (m_acinerella)
		m_acinerella->play();
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		m_mediaSourcePrivate->play();
#endif

	D(dprintf("%s:\n", __PRETTY_FUNCTION__));
	
	if (m_player)
	{
		if (m_acinerella)
		{
			m_player->muteChanged(m_acinerella->muted());
			m_player->volumeChanged(m_acinerella->volume());
		}
		
		m_player->rateChanged();
		m_player->playbackStateChanged();
	}
}

void MediaPlayerPrivateMorphOS::pause()
{
	if (m_acinerella)
		m_acinerella->pause();
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		m_mediaSourcePrivate->pause();
#endif
	D(dprintf("%s:\n", __PRETTY_FUNCTION__));

// NO, this will break the internal PLAYING state of HTMLMediaElement
//	m_player->playbackStateChanged();

	if (MediaPlayerMorphOSSettings::settings().m_pausedOrFinished)
		MediaPlayerMorphOSSettings::settings().m_pausedOrFinished(m_player);
}

void MediaPlayerPrivateMorphOS::setVolume(float volume)
{
	D(dprintf("%s: vol %f\n", __PRETTY_FUNCTION__, volume));
	if (m_acinerella)
		m_acinerella->setVolume(volume);
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		m_mediaSourcePrivate->setVolume(volume);
#endif
}

void MediaPlayerPrivateMorphOS::setMuted(bool muted)
{
	D(dprintf("%s: %d\n", __PRETTY_FUNCTION__, muted));
	if (m_acinerella)
		m_acinerella->setMuted(muted);
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		m_mediaSourcePrivate->setMuted(muted);
#endif
}

FloatSize MediaPlayerPrivateMorphOS::naturalSize() const
{
	return { float(m_width), float(m_height) };
}

float MediaPlayerPrivateMorphOS::duration() const
{
	return durationDouble();
}

double MediaPlayerPrivateMorphOS::durationDouble() const
{
#if ENABLE(MEDIA_SOURCE)
	if (m_mediaSourcePrivate)
		return m_mediaSourcePrivate->duration().toDouble();
#endif
	return m_duration;
}

MediaTime MediaPlayerPrivateMorphOS::durationMediaTime() const
{
	if (m_acinerella && m_acinerella->isLive())
		return MediaTime::invalidTime();
#if ENABLE(MEDIA_SOURCE)
	if (m_mediaSourcePrivate)
		return m_mediaSourcePrivate->duration();
#endif
	return MediaTime::createWithDouble(durationDouble());
}

bool MediaPlayerPrivateMorphOS::hasVideo() const
{
	if (m_acinerella)
		return m_acinerella->hasVideo();
#if ENABLE(MEDIA_SOURCE)
    else if (m_mediaSourcePrivate)
        return m_mediaSourcePrivate->hasVideo();
#endif
	return false;
}

bool MediaPlayerPrivateMorphOS::hasAudio() const
{
	if (m_acinerella)
		return m_acinerella->hasAudio();
#if ENABLE(MEDIA_SOURCE)
    else if (m_mediaSourcePrivate)
        return m_mediaSourcePrivate->hasAudio();
#endif
	return false;
}

void MediaPlayerPrivateMorphOS::setPageIsVisible(bool visible)
{
	m_visible = visible;
//	D(dprintf("%s: visible %d\n", __PRETTY_FUNCTION__, visible));
}

bool MediaPlayerPrivateMorphOS::seeking() const
{
	D(dprintf("%s: %d\n", __PRETTY_FUNCTION__, m_acinerella?m_acinerella->isSeeking():false));
	if (m_acinerella)
		return m_acinerella->isSeeking();
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		return m_mediaSourcePrivate->isSeeking();
#endif
	return false;
}

void MediaPlayerPrivateMorphOS::seek(float time)
{
	D(dprintf("%s: %f\n", __PRETTY_FUNCTION__, time));
	if (m_acinerella)
		return m_acinerella->seek(time);
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		return m_mediaSourcePrivate->seek(time);
#endif
}

bool MediaPlayerPrivateMorphOS::ended() const
{
	if (m_acinerella)
		return m_acinerella->ended();
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		return m_mediaSourcePrivate->ended();
#endif
	return true;
}

bool MediaPlayerPrivateMorphOS::paused() const
{
	if (m_acinerella)
		return m_acinerella->paused();
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		return m_mediaSourcePrivate->paused();
#endif
	return true;
}

std::optional<VideoPlaybackQualityMetrics> MediaPlayerPrivateMorphOS::videoPlaybackQualityMetrics()
{
	VideoPlaybackQualityMetrics metrics;
	metrics.totalVideoFrames = m_decodedFrameCount;
	metrics.droppedVideoFrames = m_droppedFrameCount;
	return metrics;
}

MediaPlayer::NetworkState MediaPlayerPrivateMorphOS::networkState() const
{
	return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivateMorphOS::readyState() const
{
	return m_readyState;
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateMorphOS::buffered() const
{
#if ENABLE(MEDIA_SOURCE)
	if (m_mediaSourcePrivate)
		return m_mediaSourcePrivate->buffered();
#endif
	return makeUnique<PlatformTimeRanges>(MediaTime::createWithDouble(std::max(0.0, m_currentTime - 1.0 )),
		MediaTime::createWithDouble(m_currentTime + 10.0));
}

void MediaPlayerPrivateMorphOS::paint(GraphicsContext& gc, const FloatRect& rect)
{
	if (gc.paintingDisabled() || !m_visible)
		return;

	if (m_acinerella)
		m_acinerella->paint(gc, rect);
#if ENABLE(MEDIA_SOURCE)
	else if (m_mediaSourcePrivate)
		m_mediaSourcePrivate->paint(gc, rect);
#endif
}

void MediaPlayerPrivateMorphOS::accNextFrameReady()
{
	if (!m_didDrawFrame)
	{
		if (m_player)
		{
			m_player->firstVideoFrameAvailable();
			m_player->repaint();
		}

		m_didDrawFrame = true;

		if (MediaPlayerMorphOSSettings::settings().m_overlayRequest)
		{
			MediaPlayerMorphOSSettings::settings().m_overlayRequest(m_player,
				[weak = makeWeakPtr(this)](void *ptr, int sx, int sy, int ml, int mt, int mr, int mb, int w, int h) {
				if (weak) {
					if (weak->m_acinerella)
						weak->m_acinerella->setOverlayWindowCoords((struct ::Window *)ptr, sx, sy, ml, mt, mr, mb, w, h);
#if ENABLE(MEDIA_SOURCE)
					else if (weak->m_mediaSourcePrivate)
						weak->m_mediaSourcePrivate->setOverlayWindowCoords((struct ::Window *)ptr, sx, sy, ml, mt, mr, mb, w, h);
#endif
				}
			});
		}
	}
    else
    {
        m_player->repaint();
    }
}

void MediaPlayerPrivateMorphOS::accNoFramesReady()
{
	// TODO: overlay shutdown?
	m_didDrawFrame = false;
	m_player->repaint();
}

void MediaPlayerPrivateMorphOS::accSetVideoSize(int width, int height)
{
	m_width = width;
	m_height = height;
	if (m_player)
		m_player->sizeChanged();
}

void MediaPlayerPrivateMorphOS::accFrameUpdateNeeded() 
{
	if (MediaPlayerMorphOSSettings::settings().m_overlayUpdate)
		MediaPlayerMorphOSSettings::settings().m_overlayUpdate(m_player);
}

bool MediaPlayerPrivateMorphOS::accCodecSupported(const String &codec)
{
	MediaEngineSupportParameters parameters;
	parameters.page = m_player->client().mediaPlayerPage();
	String ct; // audio/mp4; codecs="mp4a.40.2"
	ct = "video/mp4; codecs=\"";
	ct.append(codec);
	ct.append("\"");
	parameters.type = ContentType(ct);
	return MediaPlayerFactoryMediaSourceMorphOS::s_supportsTypeAndCodecs(parameters) == MediaPlayer::SupportsType::IsSupported;
}

bool MediaPlayerPrivateMorphOS::accIsURLValid(const String& url)
{
	if (m_failedHLSStreamURIs.contains(url))
		return false;
	return true;
}

void MediaPlayerPrivateMorphOS::accSetFrameCounts(unsigned decoded, unsigned dropped)
{
	m_decodedFrameCount = decoded;
	m_droppedFrameCount = dropped;
}

bool MediaPlayerPrivateMorphOS::didLoadingProgress() const
{
	if (m_didLoadingProgress)
	{
		m_didLoadingProgress = false;
		return true;
	}

	return false;
}

MediaPlayer::MovieLoadType MediaPlayerPrivateMorphOS::movieLoadType() const
{
	if (m_acinerella)
		return m_acinerella->isLive() ? MediaPlayer::MovieLoadType::LiveStream : MediaPlayer::MovieLoadType::Download;
	return MediaPlayer::MovieLoadType::Download;
}

float MediaPlayerPrivateMorphOS::maxTimeSeekable() const
{
	if (m_acinerella && m_acinerella->canSeek())
		return m_duration;
#if ENABLE(MEDIA_SOURCE)
	return m_duration;
#endif
	return 0.f;
}

void MediaPlayerPrivateMorphOS::accInitialized(MediaPlayerMorphOSInfo info)
{
	if (MediaPlayerMorphOSSettings::settings().m_load)
	{
		String url;

		if (info.m_width)
		{
			m_width = info.m_width;
			m_height = info.m_height;
		}

		accSetVideoSize(m_width, m_height);
		accSetReadyState(WebCore::MediaPlayerEnums::ReadyState::HaveMetadata);

#if ENABLE(MEDIA_SOURCE)
		if (m_mediaSourcePrivate)
			url = m_mediaSourcePrivate->url();
		else
#endif
			url = m_acinerella->url();

		MediaPlayerMorphOSSettings::settings().m_load(m_player, url, info, m_streamSettings,
			[this]() {
				if (m_acinerella) {
					m_acinerella->pause();
					m_acinerella->coolDown();
				}
#if ENABLE(MEDIA_SOURCE)
				else if (m_mediaSourcePrivate) {
					m_mediaSourcePrivate->pause();
					m_mediaSourcePrivate->coolDown();
				}
#endif
				m_didDrawFrame = false;
				m_player->playbackStateChanged();
			});

		m_acInitialized = true;
		m_player->characteristicChanged();

		// MediaSource has its own track handling!
		if (m_acinerella)
		{
			if (info.m_width)
			{
				m_videoTrack = VideoTrackPrivateMorphOS::create(makeWeakPtr(this), 0);
				m_player->addVideoTrack(*m_videoTrack.get());
			}
			
			if (info.m_channels)
			{
				m_audioTrack = AudioTrackPrivateMorphOS::create(makeWeakPtr(this), 0);
				m_player->addAudioTrack(*m_audioTrack.get());
			}
		}

		if (m_prepareToPlay && m_acinerella)
			m_acinerella->warmUp();
	#if ENABLE(MEDIA_SOURCE)
		else if (m_prepareToPlay && m_mediaSourcePrivate)
			m_mediaSourcePrivate->warmUp();
	#endif
	}
}

void MediaPlayerPrivateMorphOS::accUpdated(MediaPlayerMorphOSInfo info)
{
#if 1
	if (info.m_width)
	{
		m_width = info.m_width;
		m_height = info.m_height;
	}

	accSetVideoSize(m_width, m_height);
#else
	if (MediaPlayerMorphOSSettings::settings().m_update)
	{
		MediaPlayerMorphOSSettings::settings().m_update(m_player, info);
	}
#endif
}

void MediaPlayerPrivateMorphOS::accSetNetworkState(WebCore::MediaPlayerEnums::NetworkState state)
{
	m_networkState = state;
	m_player->networkStateChanged();
}

void MediaPlayerPrivateMorphOS::accSetReadyState(WebCore::MediaPlayerEnums::ReadyState state)
{
	m_readyState = state;
	m_player->readyStateChanged();
}

void MediaPlayerPrivateMorphOS::accSetBufferLength(double buffer)
{
	(void)buffer;
	m_player->bufferedTimeRangesChanged();
	m_player->seekableTimeRangesChanged();
}

void MediaPlayerPrivateMorphOS::accSetPosition(double pos)
{
	D(dprintf("%s: timechanged to %f\n", __func__, this, float(pos)));
	m_currentTime = pos;
	m_player->timeChanged();
}

void MediaPlayerPrivateMorphOS::accSetDuration(double dur)
{
#if ENABLE(MEDIA_SOURCE)
	if (m_mediaSourcePrivate)
	{
		m_player->durationChanged();
		return;
	}
#endif
	if (abs(dur - m_duration) >= 1.0)
	{
		D(dprintf("%s: changed to %f\n", __func__, this, float(dur)));
		m_duration = ceil(dur);
		m_player->durationChanged();
	}
}

void MediaPlayerPrivateMorphOS::accEnded()
{
	m_currentTime = m_duration;
	m_player->timeChanged();
	m_player->characteristicChanged();
	m_player->playbackStateChanged();

	if (MediaPlayerMorphOSSettings::settings().m_pausedOrFinished)
		MediaPlayerMorphOSSettings::settings().m_pausedOrFinished(m_player);
}

void MediaPlayerPrivateMorphOS::accFailed()
{
	if (m_acinerella && m_acinerella->isLive() && !m_acinerella->hlsStreamURL().isEmpty())
	{
		if (!m_failedHLSStreamURIs.contains(m_acinerella->hlsStreamURL()))
		{
			D(dprintf("%s: blacklist stream URL %s and retry\n", __func__, m_acinerella->hlsStreamURL().utf8().data()));
			m_failedHLSStreamURIs.add(m_acinerella->hlsStreamURL());

			String url = m_acinerella->url();
			m_acinerella->terminate();
			m_acinerella = Acinerella::Acinerella::create(this, url);

			if (m_acinerella)
				return;
		}
	}

	m_networkState = WebCore::MediaPlayerEnums::NetworkState::FormatError;
	m_readyState = WebCore::MediaPlayerEnums::ReadyState::HaveNothing;
	m_player->networkStateChanged();
	m_player->readyStateChanged();
	if (MediaPlayerMorphOSSettings::settings().m_pausedOrFinished)
		MediaPlayerMorphOSSettings::settings().m_pausedOrFinished(m_player);
}

RefPtr<PlatformMediaResourceLoader> MediaPlayerPrivateMorphOS::accCreateResourceLoader()
{
	return m_player->createResourceLoader();
}

String MediaPlayerPrivateMorphOS::accReferrer()
{
	return m_player->referrer();
}

void MediaPlayerPrivateMorphOS::onTrackEnabled(int index, bool enabled)
{
	(void)index;
	(void)enabled;
	D(dprintf("%s: %p, track %p enabled %d\n", __func__, this, index, enabled));
}

void MediaPlayerPrivateMorphOS::selectHLSStream(const String& url)
{
	if (m_acinerella)
		m_acinerella->selectStream(url, m_currentTime);
}

}

#undef D
#undef DM
#endif
