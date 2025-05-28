#include "config.h"

#if ENABLE(VIDEO)

#include "AcinerellaContainer.h"
#include "AcinerellaAudioDecoder.h"
#include "AcinerellaVideoDecoder.h"
#include "AcinerellaBuffer.h"
#include "AcinerellaMuxer.h"
#include "AcinerellaHLS.h"
#include "PlatformMediaResourceLoader.h"
#include <proto/exec.h>
#include <exec/system.h>

#define D(x) 
#define DNP(x)
#define DIO(x) 
#define DINIT(x)

#define DDUMP(x) 

namespace WebCore {
namespace Acinerella {

Acinerella::Acinerella(AcinerellaClient *client, const String &url)
	: m_client(client)
	, m_url(url)
	, m_watchdogTimer(RunLoop::current(), this, &Acinerella::watchdogTimerFired)
{
	D(dprintf("%s: %p url '%s'\n", __func__, this, url.utf8().data()));
	m_networkBuffer = AcinerellaNetworkBuffer::create(this, m_url);
	m_isHLS = m_url.contains("m3u8");

	if (m_networkBuffer)
	{
		m_canSeek = m_isHLS ? false : true; // will be updated later
		
		D(dprintf("%s: isHLS %d canseek %d\n", __func__, m_isHLS, m_canSeek));
		
		m_networkBuffer->start();
	}
	ref();
	m_thread = Thread::create("Acinerella", [this] {
		threadEntryPoint();
	});
	if (!m_thread)
		deref();
}

void Acinerella::play()
{
	D(dprintf("%s: paused %d ended %d\n", __PRETTY_FUNCTION__, m_paused, m_ended));
	if (m_ended)
		return;

	m_paused = false;
	m_waitReady = true;
	m_liveEnded = false;

	m_watchdogTimer.startOneShot(Seconds(0.5));

	dispatch([this] {
		if (areDecodersReadyToPlay())
		{
			D(dprintf("%s: calling play()...\n", __PRETTY_FUNCTION__));
			m_waitReady = false;
			if (m_audioDecoder)
				m_audioDecoder->play();
			if (m_videoDecoder)
				m_videoDecoder->play();
		}
		else
		{
			D(dprintf("%s: calling prePlay()...\n", __PRETTY_FUNCTION__));
			m_waitReady = true;
			if (m_audioDecoder)
				m_audioDecoder->prePlay();
			if (m_videoDecoder)
				m_videoDecoder->prePlay();
		}
	});
}

void Acinerella::pause()
{
	D(dprintf("%s: paused %d ended %d\n", __PRETTY_FUNCTION__, m_paused, m_ended));
	if (m_ended)
		return;

	m_paused = true;
	m_waitReady = false;

	m_watchdogTimer.stop();

	dispatch([this] {
		if (m_audioDecoder)
			m_audioDecoder->pause();
		if (m_videoDecoder)
			m_videoDecoder->pause();
	});
}

bool Acinerella::paused()
{
	return m_paused;
}

void Acinerella::setVolume(double volume)
{
	m_volume = volume;
	dispatch([this, volume, muted = m_muted] {
		if (m_audioDecoder)
			m_audioDecoder->setVolume(muted ? 0.f : volume);
	});
}

void Acinerella::setMuted(bool muted)
{
	m_muted = muted;
	dispatch([this, muted, volume = m_volume] {
		if (m_audioDecoder)
			m_audioDecoder->setVolume(muted ? 0.f : volume);
	});
}

bool Acinerella::canSeek()
{
	return m_canSeek;
}

bool Acinerella::isSeeking()
{
	return m_isSeeking;
}

void Acinerella::seek(double time)
{
	dispatch([this,time]() {
		startSeeking(time);
	});
}

void Acinerella::dumpStatus()
{
	dprintf("\033[37m[AC%p]: PAU %d SEE %d WAR %d DUR %f\033[0m\n", this,
		m_paused, m_isSeeking, m_waitReady, float(duration()));
	if (m_audioDecoder)
		m_audioDecoder->dumpStatus();
	if (m_videoDecoder)
		m_videoDecoder->dumpStatus();
	dprintf("\033[37m[AC%p]: -- \033[0m\n", this);
}

void Acinerella::watchdogTimerFired()
{
	DDUMP(dumpStatus());
	m_watchdogTimer.startOneShot(Seconds(0.5));

	if (m_client)
	{
		m_client->accSetPosition(m_position);
		m_client->accSetDuration(m_duration);
		m_client->accSetReadyState(WebCore::MediaPlayerEnums::ReadyState::HaveEnoughData);
		if (m_videoDecoder)
		{
			auto *vd = static_cast<AcinerellaVideoDecoder*>(m_videoDecoder.get());
			m_client->accSetFrameCounts(vd->decodedFrameCount(), vd->droppedFrameCount());
		}
	}
}

bool Acinerella::isLive()
{
	return (m_isHLS || !m_canSeek) && !m_canSkip;
}

bool Acinerella::ended()
{
	return m_ended;
}

void Acinerella::ref()
{
	ThreadSafeRefCounted<Acinerella>::ref();
}

void Acinerella::deref()
{
	ThreadSafeRefCounted<Acinerella>::deref();
}

RefPtr<PlatformMediaResourceLoader> Acinerella::createResourceLoader()
{
	if (m_client)
		return m_client->accCreateResourceLoader();
	return nullptr;
}

String Acinerella::referrer()
{
	if (m_client)
		return m_client->accReferrer();
	return String();
}

void Acinerella::selectStream()
{
	HLSStreamInfo selected;
	auto *hls = static_cast<AcinerellaNetworkBufferHLS*>(m_networkBuffer.get());

	UQUAD clock = 0;
	NewGetSystemAttrsA(&clock, sizeof(clock), SYSTEMINFOTYPE_PPC_CPUCLOCK, NULL);

	for (auto info : hls->streams())
	{
		DINIT(dprintf("HLS stream: %dx%d\n", info.m_width, info.m_height));

		if (!m_client->accIsURLValid(info.m_url))
			continue;

		if (info.m_height > 720 || info.m_fps > 30)
			continue;
			
		// drop 720 and otters on slower CPUs
		if (clock < 1600000000 && info.m_height > 560)
			continue;

		// some heuristics for streams that only report bandwidth
		if (info.m_height == 0 && info.m_bandwidth != 0)
		{
			// slower CPUs: limit to 2Mbps
			if (clock < 1600000000 && info.m_bandwidth >= 2000000)
				continue;

			// faster CPUs: limit to 3.2Mbps
			if (info.m_bandwidth > 3200000)
				continue;
		}


		bool codecsOK = true;
		if (info.m_codecs.size())
		{
			for (const auto &codec : info.m_codecs)
			{
				if (!m_client || !m_client->accCodecSupported(codec))
				{
					codecsOK = false;
					break;
				}
			}
		}
		if (!codecsOK)
			continue;

		if (!selected.m_url.length())
		{
			selected = info;
		}
		// try to pick the best bandwidth stream among the ones we have not ruled out already!
		else if (selected.m_bandwidth < info.m_bandwidth)
		{
			selected = info;
		}
	}
	
	if (selected.m_url.length())
	{
		DINIT(dprintf("HLS stream selected: %dx%d %s\n", selected.m_width, selected.m_height, selected.m_url.utf8().data()));
		DINIT(for (const auto &codec : selected.m_codecs) { dprintf("\t\tcodec: %s\n", codec.utf8().data()); });
		m_hlsStreamURL = selected.m_url;
		hls->selectStream(selected);
	}
}

void Acinerella::selectStream(const String& url, double position)
{
	if (m_hlsStreamURL != url && m_networkBuffer->hasStreamSelection())
	{
		auto *hls = static_cast<AcinerellaNetworkBufferHLS*>(m_networkBuffer.get());

		for (auto info : hls->streams())
		{
			if (info.m_url == url)
			{
				m_hlsStreamURL = url;
				hls->selectStream(info, true, position);
			}
		}
	}
}

void Acinerella::startSeeking(double pos)
{
	if (m_canSkip)
	{
		D(dprintf("ac%s(%p): skipping HLS to %f ended %d\n", __func__, this, float(pos), m_ended));

		if (m_audioDecoder)
			m_audioDecoder->pause(true);
		if (m_videoDecoder)
			m_videoDecoder->pause(true);

		m_position = pos;
		m_ended = false;
		m_ioDiscontinuity = true;
		m_networkBuffer->skip(pos);

		RefPtr<AcinerellaPointer> acinerella;
		RefPtr<AcinerellaMuxedBuffer> muxer;

		{
			auto lock = Locker(m_acinerellaLock);
			acinerella = m_acinerella;
			muxer = m_muxer;
		}

		if (muxer)
		{
			muxer->flush();
			RefPtr<AcinerellaPackage> package = AcinerellaPackage::create(acinerella, ac_flush_packet());
			muxer->push(package);
		}

		if (!m_paused)
			m_waitReady = true;

		if (m_audioDecoder)
			m_audioDecoder->prePlay();
		if (m_videoDecoder)
			m_videoDecoder->prePlay();

		dispatch([this, protectedThis = makeRef(*this)]() {
			initializeAfterDiscontinuity();
		});

		return;
	}

	if (m_isSeeking || !canSeek())
		return;

	D(dprintf("ac%s(%p): ended %d paused %d \n", __func__, this, m_ended, m_paused));

	m_isSeeking = true;
	if (m_ended)
		m_paused = false;
	m_ended = false;
	m_seekingPosition = pos;

	if (m_audioDecoder)
		m_audioDecoder->pause(true);
	if (m_videoDecoder)
		m_videoDecoder->pause(true);
	
	double currentPosition = m_audioDecoder ? m_audioDecoder->position() : (m_videoDecoder ? m_videoDecoder->position() : 0.f);
	D(dprintf("ac%s(%p): %f current %f \n", __func__, this, pos, currentPosition));

	m_seekingForward = pos > currentPosition;

	RefPtr<AcinerellaPointer> acinerella;
	RefPtr<AcinerellaMuxedBuffer> muxer;

	{
		auto lock = Locker(m_acinerellaLock);
		acinerella = m_acinerella;
		muxer = m_muxer;
	}

	int direction = int(pos - currentPosition);

	if (acinerella && m_videoDecoder && acinerella->decoder(m_videoDecoder->index()))
		ac_seek(acinerella->decoder(m_videoDecoder->index()), direction, int(pos * 1000.f)); // ac_seek takes millisecs

	if (acinerella && m_audioDecoder && acinerella->decoder(m_audioDecoder->index()))
		ac_seek(acinerella->decoder(m_audioDecoder->index()), direction, int(pos * 1000.f)); // ac_seek takes millisecs

	D(dprintf("ac%s(%p): ac_Seek done\n", __func__, this));
	
	if (muxer)
	{
		muxer->flush();
		RefPtr<AcinerellaPackage> package = AcinerellaPackage::create(acinerella, ac_flush_packet());
		muxer->push(package);
	}

	D(dprintf("ac%s(%p): muxer flushed, paused %d\n", __func__, this, m_paused));

	if (!m_paused)
		m_waitReady = true;

	if (m_audioDecoder)
		m_audioDecoder->prePlay();
	if (m_videoDecoder)
		m_videoDecoder->prePlay();
}

bool Acinerella::areDecodersReadyToPlay()
{
	bool audioReady = !m_audioDecoder || m_audioDecoder->isReadyToPlay();
	bool videoReady = !m_videoDecoder || m_videoDecoder->isReadyToPlay();
	D(dprintf("%s: audio %d video %d\n", __func__, audioReady, videoReady));
	return audioReady && videoReady;
}

void Acinerella::terminate()
{
	D(dprintf("ac%s: %p\n", __func__, this));
	m_terminating = true;
	m_client = nullptr;

	m_watchdogTimer.stop();

	// Prevent muxer from obtaining more data
	if (m_networkBuffer)
		m_networkBuffer->die();

	D(dprintf("ac%s: %p network done\n", __func__, this));

	if (!m_thread)
		return;
	ASSERT(isMainThread());
	ASSERT(!m_queue.killed() && m_thread);

	if (!m_thread)
		return;

	m_queue.append(makeUnique<Function<void ()>>([this] {
		performTerminate();
	}));
	m_thread->waitForCompletion();
	ASSERT(m_queue.killed());
	m_thread = nullptr;
	
	m_networkBuffer = nullptr;

	D(dprintf("ac%s: %p done\n", __func__, this));
	deref();
}

void Acinerella::warmUp()
{
	dispatch([this]() {
		D(dprintf("warmUp: %p\n", this));
		if (m_audioDecoder)
			m_audioDecoder->warmUp();
		if (m_videoDecoder)
			m_videoDecoder->warmUp();
	});
}

void Acinerella::coolDown()
{
	dispatch([this]() {
		D(dprintf("coolDown: %p\n", this));
		if (m_audioDecoder)
			m_audioDecoder->coolDown();
		if (m_videoDecoder)
			m_videoDecoder->coolDown();
	});

}

void Acinerella::threadEntryPoint()
{
	SetTaskPri(FindTask(0), 1);

	D(dprintf("%s: %p\n", __func__, this));
	if (initialize())
	{
		while (auto function = m_queue.waitForMessage())
		{
			(*function)();
		}
	}

	// MUST be terminated first - this does not wait as we have no thread
	// but the decoder threads may be waiting for data and this will
	// unblock them!
	if (m_muxer)
		m_muxer->terminate();

	D(dprintf("ac%s: %p muxer done\n", __func__, this));

	if (m_videoDecoder)
		m_videoDecoder->terminate();

	D(dprintf("ac%s: %p video done\n", __func__, this));

	if (m_audioDecoder)
		m_audioDecoder->terminate();

	D(dprintf("ac%s: %p audio done\n", __func__, this));

	{
		auto lock = Locker(m_acinerellaLock);
		m_acinerella = nullptr;
		m_muxer = nullptr;
	}

	D(dprintf("%s: %p exits...\n", __func__, this));
}

void Acinerella::paint(GraphicsContext& gc, const FloatRect& rect)
{
	if (!!m_videoDecoder)
		m_videoDecoder->paint(gc, rect);
}

void Acinerella::setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height)
{
	if (!!m_videoDecoder)
	{
		auto *vd = static_cast<AcinerellaVideoDecoder*>(m_videoDecoder.get());
		vd->setOverlayWindowCoords(w, scrollx, scrolly, mleft, mtop, mright, mbottom, width, height);
	}
}

void Acinerella::dispatch(Function<void ()>&& function)
{
	ASSERT(isMainThread());
	ASSERT(!m_queue.killed() && m_thread);
	m_queue.append(makeUnique<Function<void ()>>(WTFMove(function)));
}

void Acinerella::performTerminate()
{
	D(dprintf("%s: %p\n", __func__, this));
	ASSERT(!isMainThread());
	m_queue.kill();
}

bool Acinerella::initialize()
{
	DINIT(dprintf("%s: %p\n", __func__, this));
    RefPtr<AcinerellaPointer> acinerella(AcinerellaPointer::create());
	if (acinerella && acinerella->instance())
	{
        m_acinerella = acinerella;
        m_networkBuffer->setIsInitializing(true);
		if (-1 == ac_open(acinerella->instance(), static_cast<void *>(this), &acOpenCallback, &acReadCallback, &acSeekCallback, &acCloseCallback, nullptr))
		{
			m_acinerella = nullptr;
			DINIT(dprintf("---- ac failed to open :(\n"));
			WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
				if (m_client)
					m_client->accFailed();
			});
		}
		else
		{
			int audioIndex = -1;
			int videoIndex = -1;

			m_networkBuffer->setIsInitializing(false);

			// update once we know the size
			m_canSeek = m_networkBuffer->canSeek();
			m_canSkip = m_networkBuffer->canSkip();

			DINIT(dprintf("ac initialized, stream count %d, canseek %d canskip %d\n", acinerella->instance()->stream_count, m_canSeek, m_canSkip));

			m_muxer = AcinerellaMuxedBuffer::create();

			for (int i = 0; i < std::min(AcinerellaMuxedBuffer::maxDecoders, acinerella->instance()->stream_count); i++)
			{
				ac_stream_info info;
				ac_get_stream_info(acinerella->instance(), i, &info);
				switch (info.stream_type)
				{
				case AC_STREAM_TYPE_VIDEO:
					DINIT(dprintf("video stream: %dx%d index %d ev %d\n", info.additional_info.video_info.frame_width, info.additional_info.video_info.frame_height, i, streamSettings().m_decodeVideo));
					if (-1 == videoIndex && streamSettings().m_decodeVideo)
						videoIndex = i;
					break;

				case AC_STREAM_TYPE_AUDIO:
					DINIT(dprintf("audio stream: %d %d %d\n", info.additional_info.audio_info.samples_per_second,
						info.additional_info.audio_info.channel_count, info.additional_info.audio_info.bit_depth));
					if (-1 == audioIndex)
						audioIndex = i;
					break;
					
				case AC_STREAM_TYPE_UNKNOWN:
					break;
				}
			}

			DINIT(dprintf("ac init audio %d video %d\n", audioIndex, videoIndex));

			if (-1 != audioIndex || -1 != videoIndex)
			{
				ac_stream_info info;
				uint32_t decoderMask = 0;

				if (-1 != audioIndex)
				{
					ac_get_stream_info(acinerella->instance(), audioIndex, &info);
					acinerella->setDecoder(audioIndex, ac_create_decoder(acinerella->instance(), audioIndex));
					DINIT(dprintf("audio decoder: %p\n", acinerella->decoder(audioIndex)));
					m_audioDecoder = AcinerellaAudioDecoder::create(this, acinerella, m_muxer, audioIndex, info, isLive(), m_isHLS);
					if (!!m_audioDecoder)
						decoderMask |= (1UL << audioIndex);
				}

				if (-1 != videoIndex)
				{
					ac_get_stream_info(acinerella->instance(), videoIndex, &info);
					acinerella->setDecoder(videoIndex, ac_create_decoder(acinerella->instance(), videoIndex));
					DINIT(dprintf("video decoder: %p\n", acinerella->decoder(videoIndex)));
					m_videoDecoder = AcinerellaVideoDecoder::create(this, acinerella, m_muxer, videoIndex, info, isLive(), m_isHLS);
					if (!!m_videoDecoder)
						decoderMask |= (1UL << videoIndex);
				}

				WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
					if (m_client)
					{
						m_client->accSetNetworkState(WebCore::MediaPlayerEnums::NetworkState::Loading);
					}
				});

				double audioDuration = 0;
				double videoDuration = 0;

				if (m_audioDecoder)
				{
					audioDuration = m_audioDecoder->duration();
					if (audioDuration <= 0.f && m_networkBuffer->length() > 0)
					{
						double total = m_networkBuffer->length();
						total *= 8;
						total /= m_audioDecoder->bitRate();
						audioDuration = total;
					}
				}
				
				if (m_videoDecoder)
				{
					videoDuration = m_videoDecoder->duration();
					if (videoDuration <= 0.f && m_networkBuffer->length() > 0)
					{
						double total = m_networkBuffer->length();
						total *= 8;
						total /= m_videoDecoder->bitRate();
						videoDuration = total;
					}
				}
				
				m_muxer->setDecoderMask(decoderMask);
				m_muxer->setSinkFunction([this, protectedThis = makeRef(*this)](int, int, uint32_t) -> bool {
					// look ma, a lambda within a lambda
                    if (!m_waitingForDemux) {
                        m_waitingForDemux = true;
                        protectedThis->dispatch([this]() {
                            demuxMorePackages();
                        });
                    }
					return true;
				});
				
				if (0 != decoderMask)
				{
					// known length? assume our length is fine
					// unknown? assume Inf
					if (m_isHLS && m_networkBuffer->knowsDuration())
						m_duration = m_networkBuffer->duration();
					else if (m_isHLS)
						m_duration = 0.0;
					else if (m_networkBuffer->length() > 0)
						m_duration = std::max(audioDuration, videoDuration);
					else
						m_duration = std::numeric_limits<double>::infinity();
			
					MediaPlayerMorphOSInfo& minfo = m_info;
					minfo.m_duration = m_duration;

					if (m_audioDecoder)
					{
						AcinerellaAudioDecoder* audio = static_cast<AcinerellaAudioDecoder *>(m_audioDecoder.get());
						minfo.m_frequency = audio->rate();
						minfo.m_bits = audio->bits();
						minfo.m_channels = audio->channels();
						minfo.m_audioCodec = audio->codec();
					}
					else
					{
						minfo.m_frequency = 0;
					}
			
					if (m_videoDecoder)
					{
						AcinerellaVideoDecoder* video = static_cast<AcinerellaVideoDecoder *>(m_videoDecoder.get());
						minfo.m_width = video->frameWidth();
						minfo.m_height = video->frameHeight();
						minfo.m_videoCodec = video->codec();
						minfo.m_bitRate = video->bitRate();
					}
					else
					{
						minfo.m_width = minfo.m_height = 0;
					}
					minfo.m_isLive = !m_canSkip && !m_canSeek;
                    minfo.m_isDownloadable = false;// todo
                    minfo.m_isHLS = m_isHLS;
                    minfo.m_clearKeyDRM = m_networkBuffer->isEncrypted();

                    if (m_isHLS)
                    {
						auto *hls = static_cast<AcinerellaNetworkBufferHLS*>(m_networkBuffer.get());
						minfo.m_hlsStreams.reserveCapacity(hls->streams().size());
						for (auto& info : hls->streams())
						{
							MediaPlayerMorphOSInfoTrack quality;
							quality.m_fps = info.m_fps;
							quality.m_width = info.m_width;
							quality.m_height = info.m_height;
							quality.m_bitRate = info.m_bandwidth;
							quality.m_codecs = info.m_codecs;
							quality.m_url = info.m_url;
							minfo.m_hlsStreams.append(WTFMove(quality));
						}
						minfo.m_selectedHLSStreamURL = hls->selectedStream().m_url;
					}
					
					WTF::callOnMainThread([this, minfo, protectedThis = makeRef(*this)]() {
						if (m_client)
						{
							m_client->accSetDuration(m_duration);
							m_client->accInitialized(minfo);
						}
					});
				}
				else
				{
					WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
						if (m_client)
							m_client->accFailed();
					});
				}
			}
		}
	}
	else
	{
		WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
		if (m_client)
			m_client->accFailed();
		});
	}

	if (m_acinerella)
		return true;
	return false;
}

void Acinerella::initializeAfterDiscontinuity()
{
	// Eat everything from the previous buffer!
	D(dprintf("[RE]decode previous packages...\n"));
	demuxMorePackages(true);

	m_ioDiscontinuity = false;
    m_networkBuffer->setIsInitializing(true);

	auto acinerella = AcinerellaPointer::create();
	if (-1 == ac_open(acinerella->instance(), static_cast<void *>(this), &acOpenCallback, &acReadCallback, &acSeekCallback, &acCloseCallback, nullptr))
	{
		WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
		if (m_client)
			m_client->accFailed();
		});
	}
	else
	{
		D(dprintf("[RE]ac initialized, stream count %d\n", acinerella->instance()->stream_count));
		int audioIndex = -1;
		int videoIndex = -1;
        m_networkBuffer->setIsInitializing(false);

		for (int i = 0; i < std::min(AcinerellaMuxedBuffer::maxDecoders, acinerella->instance()->stream_count); i++)
		{
			ac_stream_info info;
			ac_get_stream_info(acinerella->instance(), i, &info);
			switch (info.stream_type)
			{
			case AC_STREAM_TYPE_VIDEO:
				D(dprintf("[RE]video stream: %dx%d\n", info.additional_info.video_info.frame_width, info.additional_info.video_info.frame_height));
				if (-1 == videoIndex && streamSettings().m_decodeVideo)
					videoIndex = i;
				break;

			case AC_STREAM_TYPE_AUDIO:
				D(dprintf("[RE]audio stream: %d %d %d\n", info.additional_info.audio_info.samples_per_second,
					info.additional_info.audio_info.channel_count, info.additional_info.audio_info.bit_depth));
				if (-1 == audioIndex)
					audioIndex = i;
				break;
				
			case AC_STREAM_TYPE_UNKNOWN:
				break;
			}
		}
		
		// PROBLEM: indexes of both tracks MUST be the same as for the previous chunk
		// for most HLS streams this should be the same...
		if (m_audioDecoder && m_audioDecoder->index() != audioIndex)
			audioIndex = -1;
		
		if (m_videoDecoder && m_videoDecoder->index() != videoIndex)
			videoIndex = -1;

		DINIT(dprintf("[RE]ac init audio %d video %d\n", audioIndex, videoIndex));
		bool updateInfo = false;

		if (-1 != audioIndex || -1 != videoIndex)
		{
			if (-1 != audioIndex)
				acinerella->setDecoder(audioIndex, ac_create_decoder(acinerella->instance(), audioIndex));
			
			if (-1 != videoIndex)
				acinerella->setDecoder(videoIndex, ac_create_decoder(acinerella->instance(), videoIndex));
			
			{
				auto lock = Locker(m_acinerellaLock);
				m_acinerella = acinerella;
			}

			// Flush packet!
			RefPtr<AcinerellaPackage> package = AcinerellaPackage::create(acinerella, ac_flush_packet());
			m_muxer->push(package);
			DINIT(dprintf("[RE]muxer sent an ac_flush_packet!\n"));

			if (m_audioDecoder)
				m_audioDecoder->warmUp();
			if (m_videoDecoder)
				m_videoDecoder->warmUp();

			MediaPlayerMorphOSInfo& minfo = m_info;

			if (m_audioDecoder)
			{
				AcinerellaAudioDecoder* audio = static_cast<AcinerellaAudioDecoder *>(m_audioDecoder.get());
				if (minfo.m_frequency != audio->rate() || minfo.m_audioCodec != audio->codec())
				{
					minfo.m_frequency = audio->rate();
					minfo.m_bits = audio->bits();
					minfo.m_channels = audio->channels();
					minfo.m_audioCodec = audio->codec();
					updateInfo = true;
				}
			}
	
			if (m_videoDecoder)
			{
				AcinerellaVideoDecoder* video = static_cast<AcinerellaVideoDecoder *>(m_videoDecoder.get());
				if (minfo.m_width != video->frameWidth() || minfo.m_videoCodec != video->codec() || minfo.m_bitRate != video->bitRate())
				{
					minfo.m_width = video->frameWidth();
					minfo.m_height = video->frameHeight();
					minfo.m_videoCodec = video->codec();
					minfo.m_bitRate = video->bitRate();
					updateInfo = true;
				}
			}

			auto *hls = static_cast<AcinerellaNetworkBufferHLS*>(m_networkBuffer.get());
			if (minfo.m_selectedHLSStreamURL != hls->selectedStream().m_url)
			{
				minfo.m_selectedHLSStreamURL = hls->selectedStream().m_url;
				updateInfo = true;
			}
		}

		WTF::callOnMainThread([this, update = updateInfo, minfo = m_info, protectedThis = makeRef(*this)]() {
			if (m_client)
			{
				m_client->accSetReadyState(WebCore::MediaPlayerEnums::ReadyState::HaveFutureData);
				if (update)
					m_client->accUpdated(minfo);
			}
		});
	}
}

void Acinerella::demuxMorePackages(bool untilEOS)
{
	RefPtr<AcinerellaPointer> acinerella;
	RefPtr<AcinerellaMuxedBuffer> muxer;

	{
		auto lock = Locker(m_acinerellaLock);
		acinerella = m_acinerella;
		muxer = m_muxer;
	}

	DNP(dprintf("%s: %p %p %p %p ueos %d term %d hls %d\n", __func__ , this, muxer.get(), acinerella.get(), acinerella->instance(), untilEOS, m_terminating, m_isHLS));

	int packages = 0;
	if (muxer && acinerella && acinerella->instance())
	{

		while (!m_terminating && (untilEOS || (packages < 128)))
		{
			RefPtr<AcinerellaPackage> package = AcinerellaPackage::create(acinerella, ac_read_package(acinerella->instance()));
			const bool hasPackage = !!package->package();

			// don't send EOF singnalling packages to muxer if this is a live stream!
			if ((!m_isHLS || m_liveEnded) || hasPackage)
			{
				DNP(if(!package->package()) dprintf("%s: EOF packet!\n", __func__));
				muxer->push(package);
			}
			else if (!hasPackage && m_isHLS && !m_ioDiscontinuity)
			{
				bool hasMore = m_networkBuffer->markLastFrameRead();
				DNP(dprintf("%s: mark lfr, %d\n", __func__, hasMore));
				if (!hasMore && !m_liveEnded)
				{
					DNP(if(!package->package()) dprintf("%s: EOF packet!\n", __func__));
					m_liveEnded = true;
					m_ioDiscontinuity = true;
					muxer->push(package);
				}
				else
				{
					m_ioDiscontinuity = true;

					dispatch([this, protectedThis = makeRef(*this)]() {
						initializeAfterDiscontinuity();
					});
				}
			}

			if (!package->package())
			{
				break;
			}
			
			packages++;
		}
	}
 
	DNP(dprintf("%s: done, %d\n", __func__, packages));
    m_waitingForDemux = false;
}

const WebCore::MediaPlayerMorphOSStreamSettings& Acinerella::streamSettings()
{
	static MediaPlayerMorphOSStreamSettings defaults;
	if (m_client)
		return m_client->streamSettings();
	return defaults;
}

void Acinerella::onDecoderWarmedUp(RefPtr<AcinerellaDecoder> decoder)
{
	D(dprintf("%s:\n", __func__));
	(void)decoder;
	WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
		if (m_client)
		{
			m_client->accSetReadyState(WebCore::MediaPlayerEnums::ReadyState::HaveEnoughData);
		}
	});
}

void Acinerella::onDecoderReadyToPlay(RefPtr<AcinerellaDecoder> decoder)
{
	D(dprintf("%s:\n", __func__));
	(void)decoder;

	dispatch([this, protectedThis = makeRef(*this)]() {
		D(dprintf("onDecoderReadyToPlay: ready %d\n", areDecodersReadyToPlay()));

		if (areDecodersReadyToPlay()) {

			if (m_waitReady) {
				m_waitReady = false;
			
				if (m_audioDecoder)
					m_audioDecoder->play();
				if (m_videoDecoder)
					m_videoDecoder->play();
			}
		}
	});
	
	if (m_videoDecoder == decoder)
	{
		int width = static_cast<AcinerellaVideoDecoder *>(decoder.get())->frameWidth();
		int height = static_cast<AcinerellaVideoDecoder *>(decoder.get())->frameHeight();
		WTF::callOnMainThread([width, height, this, protect = makeRef(*this)]() {
			if (m_client)
				m_client->accSetVideoSize(width, height);
		});
	}
}

void Acinerella::onDecoderPlaying(RefPtr<AcinerellaDecoder>, bool /*playing*/)
{
//	WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
//	});
}

void Acinerella::onDecoderEnded(RefPtr<AcinerellaDecoder>)
{
	D(dprintf("%s: %p\n", __func__ , this));
	m_ended = true;
	m_paused = true;

	WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
		if (m_client)
			m_client->accEnded();
	});
}

void Acinerella::onDecoderUpdatedBufferLength(RefPtr<AcinerellaDecoder>, double buffer)
{
	WTF::callOnMainThread([this, buffer, protectedThis = makeRef(*this)]() {
		if (m_client)
			m_client->accSetBufferLength(buffer);
	});
}

void Acinerella::onDecoderUpdatedPosition(RefPtr<AcinerellaDecoder> decoder, double pos)
{
	D(dprintf("%s: %p %f\n", __func__ , this, float(pos)));
	if (decoder->isAudio() || !m_audioDecoder)
	{
		if (m_isSeeking && pos >= (m_seekingPosition - 5.f) && pos <= (m_seekingPosition + 5.f))
		{
			D(dprintf("--endseeking\n"));
			m_isSeeking = false;
	
			if (!m_paused)
			{
				WTF::callOnMainThread([this, protectedThis = makeRef(*this)]() {
					if (m_client)
						play();
				});
			}
		}
	}

	if (m_isSeeking)
		return;

	if (decoder->isAudio() && !!m_videoDecoder)
		static_cast<AcinerellaVideoDecoder *>(m_videoDecoder.get())->setAudioPresentationTime(pos);
	
	if (decoder->isAudio() || !m_audioDecoder)
	{
		m_position = pos + m_networkBuffer->initialTimeStamp();
	}
}

void Acinerella::onDecoderUpdatedDuration(RefPtr<AcinerellaDecoder> decoder, double duration)
{
	if (!isLive() && (decoder->isAudio() || !m_audioDecoder))
	{
		m_duration = duration;
	}
}

void Acinerella::onDecoderWantsToRender(RefPtr<AcinerellaDecoder>)
{
	WTF::callOnMainThread([this, protect = makeRef(*this)]() {
		if (m_client)
			m_client->accNextFrameReady();
	});
}

void Acinerella::onDecoderNotReadyToRender(RefPtr<AcinerellaDecoder>)
{
	WTF::callOnMainThread([this, protect = makeRef(*this)]() {
		if (m_client)
			m_client->accNoFramesReady();
	});
}

void Acinerella::onDecoderRenderUpdate(RefPtr<AcinerellaDecoder>)
{
	WTF::callOnMainThread([this, protect = makeRef(*this)]() {
		if (m_client)
			m_client->accFrameUpdateNeeded();
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

// callback from acinerella on acinerella's main thread!
int Acinerella::open()
{
	return 0;
}

// callback from acinerella on acinerella's main thread!
int Acinerella::close()
{
	return 0;
}

// callback from acinerella on acinerella's main thread!
int Acinerella::read(uint8_t *buf, int size)
{
 	DIO(dprintf("%s: %p size %d discontinuity %d\n", "acRead", this, size, m_ioDiscontinuity));

	if (m_ioDiscontinuity)
		return AcinerellaNetworkBuffer::eRead_EOF;

	RefPtr<AcinerellaNetworkBuffer> buffer(m_networkBuffer);
	if (buffer)
	{
		int rc = buffer->read(buf, size, m_readPosition);

 		DIO(dprintf("%s: %p >> rc %d\n", "acRead", this, rc));

		if (rc >= 0)
			m_readPosition = -1;

		if (rc == AcinerellaNetworkBuffer::eRead_Discontinuity)
		{
			DIO(dprintf("%s: %p >> eRead_Discontinuity\n", "acRead", this));
			rc = AcinerellaNetworkBuffer::eRead_EOF;
			m_ioDiscontinuity = true;
			dispatch([this, protectedThis = makeRef(*this)]() {
				initializeAfterDiscontinuity();
			});
		}
		else if (rc == AcinerellaNetworkBuffer::eRead_EOF && buffer->hasStreamSelection())
		{
			D(dprintf("HLSEOF!\n"));
			// fail further reads, EOF!
			m_ioDiscontinuity = true;
			// let demuxer signal EOF to decoders
			m_liveEnded = true;
		}
		else if (rc == AcinerellaNetworkBuffer::eRead_EOFWhileInitializing)
		{
			rc = 0;
		}
		
		return rc;
	}

	return AcinerellaNetworkBuffer::eRead_EOF;
}

#ifndef AVSEEK_SIZE
#define AVSEEK_SIZE 0x10000
#endif

// callback from acinerella on acinerella's main thread!
int64_t Acinerella::seek(int64_t pos, int whence)
{
	RefPtr<AcinerellaNetworkBuffer> buffer(m_networkBuffer);
	if (buffer && m_canSeek)
	{
		int64_t newPosition = pos;
		auto streamPos = buffer->position();
		auto streamLength = buffer->length();
		
		// we're not actually seeking until a read happens,
		// so this var may contain the value from a previous seek call
		if (m_readPosition != -1)
			streamPos = m_readPosition;

		DIO(dprintf("%s(%p): seek (whence %d pos %lld) streamPos %lld length %lld\n", "acSeek", this, whence, pos, streamPos, streamLength));

		switch (whence)
		{
		case SEEK_END:
			DIO(dprintf("SEEK_END: %lld + %lld\n", streamLength, pos));
			newPosition = streamLength + pos;
			break;
		case SEEK_CUR:
			DIO(dprintf("SEEK_CUR: %lld + %lld\n", streamPos, pos));
			newPosition = streamPos + pos;
			break;
		case AVSEEK_SIZE:
			DIO(dprintf("AVSEEK_SIZE: %lld\n", streamLength));
			return streamLength;
		default:
			DIO(dprintf("SEEK_POS: %lld\n", pos));
			break;
		}
		
		if (newPosition < -1)
			return newPosition = 0;
		if (streamLength && newPosition > streamLength)
			newPosition = streamLength;
		
		if (streamPos != newPosition)
			m_readPosition = newPosition;
		else
			m_readPosition = -1;
		
		DIO(dprintf("%s(%p):>> seek to %lld/rp %lld (%d %lld)\n", "acSeek", this, newPosition, m_readPosition, whence, pos));
		
		return newPosition;
	}

	return -1;
}

int Acinerella::acOpenCallback(void *me)
{
	return static_cast<Acinerella *>(me)->open();
}

int Acinerella::acCloseCallback(void *me)
{
	return static_cast<Acinerella *>(me)->close();
}

int Acinerella::acReadCallback(void *me, uint8_t *buf, int size)
{
	return static_cast<Acinerella *>(me)->read(buf, size);
}

int64_t Acinerella::acSeekCallback(void *me, int64_t pos, int whence)
{
	return static_cast<Acinerella *>(me)->seek(pos, whence);
}

}
}

#undef D
#endif
