#include "config.h"
#include "MediaSourcePrivateMorphOS.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

#include "ContentType.h"
#include "MediaSourcePrivateClient.h"
#include "MediaPlayerPrivateMorphOS.h"

#include <proto/exec.h>

#define USE_WDG

#define D(x)
#define DLIFETIME(x)
#define DDUMP(x)
#define DSEEK(x) 
#define DEOS(x)
#define DPLAY(x) 
#define DBUFFER(x)
#define DSOURCE(x)
#define DRS(x) 
// #pragma GCC optimize ("O0")

namespace WebCore {

Ref<MediaSourcePrivateMorphOS> MediaSourcePrivateMorphOS::create(MediaPlayerPrivateMorphOS& parent, MediaSourcePrivateClient& client, const String &url)
{
    auto source = adoptRef(*new MediaSourcePrivateMorphOS(parent, client, url));
    client.setPrivateAndOpen(source.copyRef());
    return source;
}

MediaSourcePrivateMorphOS::MediaSourcePrivateMorphOS(MediaPlayerPrivateMorphOS& parent, MediaSourcePrivateClient& client, const String &url)
    : MediaSourcePrivate(client)
    , m_player(WeakPtr{parent})
    , m_watchdogTimer(RunLoop::current(), this, &MediaSourcePrivateMorphOS::watchdogTimerFired)
    , m_seekingWatchdogTimer(RunLoop::current(), this, &MediaSourcePrivateMorphOS::seekingWatchdogTimerFired)
{
	DLIFETIME(dprintf("%s: \n", __PRETTY_FUNCTION__));
	m_url = url.substring(5);
}

MediaSourcePrivateMorphOS::~MediaSourcePrivateMorphOS()
{
	DLIFETIME(dprintf("%s: bye!\n", __PRETTY_FUNCTION__));
	m_watchdogTimer.stop();
    m_seekingWatchdogTimer.stop();

    for (auto& sourceBufferPrivate : m_sourceBuffers)
        sourceBufferPrivate->clearMediaSource();
}

MediaSourcePrivate::AddStatus MediaSourcePrivateMorphOS::addSourceBuffer(const ContentType& contentType, bool webMParserEnabled, RefPtr<SourceBufferPrivate>& buffer)
{
	D(dprintf("%s: '%s'\n", __PRETTY_FUNCTION__, contentType.raw().utf8().data()));
    (void)webMParserEnabled;

    MediaEngineSupportParameters parameters;
    parameters.isMediaSource = true;
    parameters.type = contentType;

    if (MediaPlayerPrivateMorphOS::extendedSupportsType(parameters, MediaPlayer::SupportsType::MayBeSupported) == MediaPlayer::SupportsType::IsNotSupported)
	{
		return MediaSourcePrivate::AddStatus::NotSupported;
	}

	buffer = MediaSourceBufferPrivateMorphOS::create(this);
	RefPtr<MediaSourceBufferPrivateMorphOS> sourceBufferPrivate = static_cast<MediaSourceBufferPrivateMorphOS*>(buffer.get());
	m_sourceBuffers.add(sourceBufferPrivate);

	if (!m_paused)
	{
		sourceBufferPrivate->prePlay();
	}
	
	return MediaSourcePrivate::AddStatus::Ok;
}

void MediaSourcePrivateMorphOS::onSourceBufferRemoved(RefPtr<MediaSourceBufferPrivateMorphOS>& buffer)
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	if (m_paintingBuffer == buffer)
		m_paintingBuffer = nullptr;
	m_sourceBuffers.remove(buffer);
	m_activeSourceBuffers.remove(buffer);
	buffer->clearMediaSource();
	if (m_player)
		m_player->notifyActiveSourceBuffersChanged();
}

void MediaSourcePrivateMorphOS::durationChanged(const MediaTime& duration)
{
    MediaSourcePrivate::durationChanged(duration);
    if (m_player)
		m_player->accSetDuration(duration.toDouble());
}

void MediaSourcePrivateMorphOS::markEndOfStream(EndOfStreamStatus status)
{
	DEOS(dprintf("%s: \n", __PRETTY_FUNCTION__));
    if (status == EndOfStreamStatus::NoError && m_player)
        m_player->accSetNetworkState(MediaPlayer::NetworkState::Loaded, { });
    m_ended = true;
    MediaSourcePrivate::markEndOfStream(status);
}

void MediaSourcePrivateMorphOS::unmarkEndOfStream()
{
	DEOS(dprintf("%s: \n", __PRETTY_FUNCTION__));
	m_ended = false;
    MediaSourcePrivate::unmarkEndOfStream();
}

MediaPlayer::ReadyState MediaSourcePrivateMorphOS::mediaPlayerReadyState() const
{
	if (m_player)
		return m_player->readyState();
    return m_readyState;
}

void MediaSourcePrivateMorphOS::setMediaPlayerReadyState(MediaPlayer::ReadyState rs)
{
	DRS(dprintf("%s: %d\n", __PRETTY_FUNCTION__, int(rs)));
	m_readyState = rs;
	if (m_player)
		m_player->accSetReadyState(rs);
}

void MediaSourcePrivateMorphOS::onSourceBufferLoadingProgressed()
{
	DBUFFER(dprintf("[MS]onSourceBufferLoadingProgressed: \n"));
#if 0
	if (readyState() < MediaPlayer::ReadyState::HaveCurrentData)
		setReadyState(MediaPlayer::ReadyState::HaveCurrentData);
#endif

	if (m_player)
		m_player->setLoadingProgresssed(true);
}

#if 0
MediaTime MediaSourcePrivateMorphOS::duration()
{
	return m_client->duration();
}
#endif

MediaTime MediaSourcePrivateMorphOS::currentMediaTime() const
{
	//if (m_seekCompleted != SeekCompleted && m_player->currentTime() < m_seekingPos)
	//	return MediaTime::createWithDouble(m_seekingPos);
	if (m_seekCompleted != Pending && m_seeking)
		return MediaTime::invalidTime();
	if (m_player)
		return m_player->currentTime();
	return { };
}

bool MediaSourcePrivateMorphOS::isLiveStream() const
{
	return std::isinf(duration().toFloat());
}

RefPtr<MediaPlayerPrivateInterface> MediaSourcePrivateMorphOS::player() const
{
    return m_player.get();
}

#if 0
const PlatformTimeRanges& MediaSourcePrivateMorphOS::buffered()
{
	return m_client->buffered();
}
#endif

void MediaSourcePrivateMorphOS::setVolume(double vol)
{
    if (vol != m_volume)
    {
        m_volume = vol;
        
        for (auto& sourceBufferPrivate : m_activeSourceBuffers)
            sourceBufferPrivate->setVolume(vol);
    }
}

void MediaSourcePrivateMorphOS::setMuted(bool muted)
{
    if (muted != m_muted)
    {
        m_muted = muted;
        
        for (auto& sourceBufferPrivate : m_activeSourceBuffers)
            sourceBufferPrivate->setVolume(muted ? 0 : m_volume);
    }
}

void MediaSourcePrivateMorphOS::dumpStatus()
{
	dprintf("\033[37m[MS%p]: POS %f BUF %d ACT %d PAU %d SEE %d WAR %d INI %d AUD %d VID %d PAI %p LIVE %d DUR %f\033[0m\n", this, float(m_position), m_sourceBuffers.size(), m_activeSourceBuffers.size(), m_paused, m_seeking, m_waitReady, m_initialized, m_hasAudio, m_hasVideo, m_paintingBuffer.get(), isLiveStream(), duration().toFloat());
    for (auto& sourceBufferPrivate : m_activeSourceBuffers)
		sourceBufferPrivate->dumpStatus();
	dprintf("\033[37m[MS%p]: -- \033[0m\n", this);
}

void MediaSourcePrivateMorphOS::watchdogTimerFired()
{
	DDUMP(dumpStatus());

	if (m_player)
	{
		m_player->accSetPosition(m_position);
		if (!!m_paintingBuffer)
		{
			unsigned decoded, dropped;
			m_paintingBuffer->getFrameCounts(decoded, dropped);
			m_player->accSetFrameCounts(decoded, dropped);
		}
	}

	if (!m_paused && !m_seeking)
	{
		bool allPlaying = true;
		bool allReady = true;

		for (auto& sourceBufferPrivate : m_activeSourceBuffers)
		{
			if (!sourceBufferPrivate->areDecodersPlaying())
				allPlaying = false;

			if (!sourceBufferPrivate->areDecodersReadyToPlay())
			{
				allReady = false;
				break;
			}
		}
		
		if (allReady && !allPlaying)
		{
			for (auto& sourceBufferPrivate : m_activeSourceBuffers)
			{
				sourceBufferPrivate->play();
			}
		}
	}

	m_watchdogTimer.startOneShot(Seconds(0.5));
}

void MediaSourcePrivateMorphOS::orphan()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	m_orphaned = true;
	m_paintingBuffer = nullptr;
	m_player.clear();
    m_seekingWatchdogTimer.stop();
    m_watchdogTimer.stop();
}

void MediaSourcePrivateMorphOS::warmUp()
{
	DBUFFER(dprintf("%s: \n", __PRETTY_FUNCTION__));
	for (auto& sourceBufferPrivate : m_activeSourceBuffers)
		sourceBufferPrivate->warmUp();
}

void MediaSourcePrivateMorphOS::coolDown()
{
	DBUFFER(dprintf("%s: \n", __PRETTY_FUNCTION__));
	m_paused = true;
	for (auto& sourceBufferPrivate : m_sourceBuffers)
		sourceBufferPrivate->coolDown();
}

void MediaSourcePrivateMorphOS::play()
{
	m_paused = false;

#ifdef USE_WDG
	m_watchdogTimer.startOneShot(Seconds(0.5));
#endif

	if (areDecodersReadyToPlay())
	{
		DPLAY(dprintf("%s: decoders ready!\n", __PRETTY_FUNCTION__));
		m_waitReady = false;

		for (auto& sourceBufferPrivate : m_activeSourceBuffers)
			sourceBufferPrivate->play();
	}
	else
	{
		DPLAY(dprintf("%s: prePlay, act buffers %d\n", __PRETTY_FUNCTION__, m_activeSourceBuffers.size()));
		m_waitReady = true;
		for (auto& sourceBufferPrivate : m_activeSourceBuffers)
			sourceBufferPrivate->prePlay();
	}
}

void MediaSourcePrivateMorphOS::pause()
{
	m_watchdogTimer.stop();

	DPLAY(dprintf("%s: \n", __PRETTY_FUNCTION__));
	for (auto& sourceBufferPrivate : m_activeSourceBuffers)
		sourceBufferPrivate->pause();
	m_paused = true;
	m_waitReady = false;
}

bool MediaSourcePrivateMorphOS::isSeeking() const
{
	return m_seeking || m_seekCompleted != SeekCompleted;
}

void MediaSourcePrivateMorphOS::seekToTarget(const SeekTarget& target)
{
	DSEEK(dprintf("%s: %f ini %d seeking %d asb %d\n", __PRETTY_FUNCTION__, target.time.toFloat(), areDecodersInitialized(), m_seeking, m_activeSourceBuffers.size()));

	m_seeking = true;
	m_seekTarget = target;
	m_seekCompleted = Pending;

	for (auto& sourceBufferPrivate : m_activeSourceBuffers)
		sourceBufferPrivate->pause();

	DSEEK(dprintf("%s: %f ini %d seeking %d asb %d\n", __PRETTY_FUNCTION__, m_seekTarget.time.toFloat(), areDecodersInitialized(), m_seeking, m_activeSourceBuffers.size()));

    SeekTarget pendingSeek = m_seekTarget;

    m_seekingWatchdogTimer.startOneShot(Seconds(6.0));

    waitForTarget(pendingSeek)->whenSettled(RunLoop::current(), [this, protect = Ref{*this}] (auto&& result) mutable {
        DSEEK(dprintf(">> MediaSourcePrivateMorphOS::seekToTarget: seek state %d\n", int(m_seekCompleted)));
        if (!result || m_seekCompleted != Pending || m_orphaned || m_sourceBuffers.size() == 0)
            return;

        auto seekedTime = *result;
        m_lastSeekTime = seekedTime;

        for (auto& sourceBufferPrivate : m_activeSourceBuffers) {
            sourceBufferPrivate->willSeek(m_lastSeekTime.toDouble());
        }

        seekToTime(seekedTime)->whenSettled(RunLoop::current(), [this, protect = Ref{*this}]() mutable {
            maybeCompleteSeek();
        });
    });
}

void MediaSourcePrivateMorphOS::maybeCompleteSeek()
{
	DSEEK(dprintf("%s: paused %d\n", __PRETTY_FUNCTION__, m_paused));

    if (m_seekCompleted != Pending)
        return;

	m_seekCompleted = SeekCompleted;
    m_seekingWatchdogTimer.stop();

	for (auto& sourceBufferPrivate : m_activeSourceBuffers)
		sourceBufferPrivate->prePlay();

	if (m_player)
		m_player->accSeeked(m_lastSeekTime.toFloat());

	m_seeking = false;
	if (!m_paused)
		play();
}

void MediaSourcePrivateMorphOS::seekingWatchdogTimerFired()
{
	DSEEK(dprintf("%s: ispending seek? %d\n", __PRETTY_FUNCTION__, m_seekCompleted == Pending));

    if (m_seekCompleted == Pending)
    {
        // go back in time: there's a better change we have this buffered already and playback will resume
        SeekTarget st({ MediaTime::createWithDouble(m_seekTarget.time.toDouble() - 10.0) });
        m_seekCompleted = SeekCompleted;
        seekToTarget(st);
    }
}

void MediaSourcePrivateMorphOS::paint(GraphicsContext& gc, const FloatRect& rect)
{
	if (!!m_paintingBuffer)
		m_paintingBuffer->paint(gc, rect);
}

void MediaSourcePrivateMorphOS::setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height)
{
	if (!!m_paintingBuffer)
		m_paintingBuffer->setOverlayWindowCoords(w, scrollx, scrolly, mleft, mtop, mright, mbottom, width, height);
}

const WebCore::MediaPlayerMorphOSStreamSettings& MediaSourcePrivateMorphOS::streamSettings()
{
	static WebCore::MediaPlayerMorphOSStreamSettings defaults;
	if (m_player)
	{
		return m_player->streamSettings();
	}
	return defaults;
}

void MediaSourcePrivateMorphOS::onSourceBufferInitialized(RefPtr<MediaSourceBufferPrivateMorphOS> &sourceBufferPrivate)
{
	WTF::callOnMainThread([this, protect = Ref{*this}, source = sourceBufferPrivate]() {
		D(dprintf("onSourceBufferInitialized: allinitialized %d seeking %d\n", areDecodersInitialized(), m_seeking));
		if (areDecodersInitialized())
		{
			MediaPlayerMorphOSInfo info;

			for (auto& sourceBufferPrivate : m_activeSourceBuffers) {
				auto &minfo = sourceBufferPrivate->info();

				if (minfo.m_width) {
					info.m_width = minfo.m_width;
					info.m_height = minfo.m_height;
					info.m_bitRate = minfo.m_bitRate;
					info.m_videoCodec = minfo.m_videoCodec;
					m_hasVideo = true;
				}
				
				if (minfo.m_channels) {
					info.m_channels = minfo.m_channels;
					info.m_bits = minfo.m_bits;
					info.m_frequency = minfo.m_frequency;
					info.m_audioCodec = minfo.m_audioCodec;
					m_hasAudio = true;
				}

				info.m_duration = duration().toFloat(); //! client provides us with the actual duration!
				info.m_isDownloadable = false;
				info.m_isMediaSource = true;
			}

			if (!m_initialized)
			{
				m_initialized = true;

                // Might be incorrect but helps initialize player if a stream is being added at the time a
                // seek has just been issued - common on YT streams not starting from t=0
                if (m_position > 0.1)
                    source->seekToTime(MediaTime::createWithDouble(m_position));

				if (m_player)
					m_player->accInitialized(info);
			}
			else if (m_player)
			{
				m_player->accUpdated(info);
			}
		}
	});
}

void MediaSourcePrivateMorphOS::onSourceBufferReadyToPaint(RefPtr<MediaSourceBufferPrivateMorphOS>& buffer)
{
	DSOURCE(dprintf("%s: ready %d\n", __PRETTY_FUNCTION__, areDecodersReadyToPlay()));

	m_paintingBuffer = buffer;
	if (m_player)
		m_player->accNextFrameReady();
}

void MediaSourcePrivateMorphOS::onSourceBuffersReadyToPlay()
{
	if (m_waitReady && areDecodersReadyToPlay())
	{
		play();
	}
}

void MediaSourcePrivateMorphOS::onSourceBufferFrameUpdate(RefPtr<MediaSourceBufferPrivateMorphOS>& buffer)
{
	if (m_paintingBuffer == buffer && m_player)
	{
		m_player->accFrameUpdateNeeded();
	}
}


void MediaSourcePrivateMorphOS::onAudioSourceBufferUpdatedPosition(RefPtr<MediaSourceBufferPrivateMorphOS>&, double position)
{
	if (m_orphaned)
		return;

	if (m_paintingBuffer)
		m_paintingBuffer->setAudioPresentationTime(position);

	m_position = position;

	if (m_seekCompleted != Pending && m_seeking)
	{
		DSEEK(dprintf("%s: seek done!\n", __func__));
		m_seeking = false;
	}
}

void MediaSourcePrivateMorphOS::onVideoSourceBufferUpdatedPosition(RefPtr<MediaSourceBufferPrivateMorphOS>& buffer, double position)
{
    (void)buffer;

	if (m_orphaned)
		return;

	// Is this a lone video MediaSource player?
	if (1 == m_activeSourceBuffers.size())
	{
		m_position = position;

		if (m_seekCompleted != Pending && m_seeking)
		{
			DSEEK(dprintf("%s: seek done!\n", __func__));
			m_seeking = false;
		}
	}
}

bool MediaSourcePrivateMorphOS::areDecodersReadyToPlay()
{
    for (auto& sourceBufferPrivate : m_activeSourceBuffers)
	{
		if (!sourceBufferPrivate->areDecodersReadyToPlay())
			return false;
	}

	return m_activeSourceBuffers.size() > 0;
}

bool MediaSourcePrivateMorphOS::areDecodersInitialized()
{
    for (auto& sourceBufferPrivate : m_sourceBuffers)
	{
		if (!sourceBufferPrivate->isInitialized())
			return false;
	}

	return true;
}

void MediaSourcePrivateMorphOS::onSourceBufferDidChangeActiveState(RefPtr<MediaSourceBufferPrivateMorphOS>& buffer, bool active)
{
	DSOURCE(dprintf("%s: source %p active %d total active %d total %d paus %d\n", __PRETTY_FUNCTION__, buffer.get(), active, m_activeSourceBuffers.size(), m_sourceBuffers.size(), m_paused));
    if (active && !m_activeSourceBuffers.contains(buffer))
    {
        m_activeSourceBuffers.add(buffer);
        if (m_player)
			m_player->onActiveSourceBuffersChanged();
//        durationChanged(duration());
        if (!m_paused)
        {
			m_waitReady = true;
            buffer->prePlay();
            DSOURCE(dprintf("%s: preplay...\n", __PRETTY_FUNCTION__));
		}
		else
		{
			buffer->prePlay();
            DSOURCE(dprintf("%s: warmup...\n", __PRETTY_FUNCTION__));
		}
    }
    else if (!active && m_activeSourceBuffers.contains(buffer))
    {
		if (m_paintingBuffer == buffer)
		{
			m_paintingBuffer->setOverlayWindowCoords(nullptr, 0, 0, 0, 0, 0, 0, 0, 0);
			m_paintingBuffer = nullptr;
		}
    
        buffer->coolDown();
    
		m_activeSourceBuffers.remove(buffer);
        if (m_player)
			m_player->onActiveSourceBuffersChanged();
    }
}

void MediaSourcePrivateMorphOS::onSourceBufferEnded(RefPtr<MediaSourceBufferPrivateMorphOS>&)
{
	DEOS(dprintf("[MS]%s: input data ended? %d paused %d\n", __func__, m_ended, m_paused));
	if (m_ended)
	{
        bool allEnded = true;
        for (auto& sourceBufferPrivate : m_activeSourceBuffers)
        {
            DEOS(dprintf("[MS]%s: source %p is ended %d\n", __func__, sourceBufferPrivate.get(), sourceBufferPrivate->isEnded()));

            if (!sourceBufferPrivate->isEnded())
            {
                allEnded = false;
                break;
            }
        }

        if (allEnded)
        {
            DEOS(dprintf("[MS]%s: all decoders have reported end of stream!\n", __func__));
            m_position = duration().toFloat();
            if (m_player)
                m_player->accEnded();
        }
	}
}

}


#endif
