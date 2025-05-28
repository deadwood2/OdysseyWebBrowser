#include "AcinerellaDecoder.h"

#if ENABLE(VIDEO)
#include "acinerella.h"
#include "AcinerellaContainer.h"
#include "MediaPlayerMorphOS.h"
#include <proto/exec.h>

#define D(x)
#define DNF(x)  //if (!isAudio()) {x;}
#define DI(x)
#define DBF(x)
#define DPOS(x) 
#define DLIFETIME(x) 

// #pragma GCC optimize ("O0")

namespace WebCore {
namespace Acinerella {

AcinerellaDecoder::AcinerellaDecoder(AcinerellaDecoderClient *client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLiveStream, bool isHLS)
	: m_client(client)
	, m_muxer(buffer)
	, m_index(index)
	, m_isLive(isLiveStream)
	, m_isHLS(isHLS)
{
	DLIFETIME(dprintf("%s: %p ++\033[0m\n", __func__, this));
	auto ac = acinerella->instance();
	m_duration = std::max(ac_get_stream_duration(ac, index), double(ac->info.duration)/1000.0);

	(void)info;

	// simulated duration of 3 chunks
	if (m_isLive)
		m_duration = 15.f;

	m_bitrate = ac->info.bitrate;
	m_lastDecoder = acinerella->decoder(m_index);
	m_codec = ac_codec_name(acinerella->instance(), index);
}

AcinerellaDecoder::~AcinerellaDecoder()
{
	DLIFETIME(dprintf("%s: %p --\033[0m\n", __func__, this));
}

void AcinerellaDecoder::warmUp()
{
	if (!m_terminating && !m_thread)
	{
		DI(dprintf("%s: %p starting thread\033[0m\n", __func__, this));
		m_thread = Thread::create(isAudio() ? "Acinerella Audio Decoder" : "Acinerella Video Decoder", [this] {
			threadEntryPoint();
		});
	}

	D(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, this));
	dispatch([this] {
		m_warminUp = true;
		decodeUntilBufferFull();
	});
}

void AcinerellaDecoder::coolDown()
{
	D(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, this));
	dispatch([this] {
		m_readying = false;
		stopPlaying();
		onCoolDown();
	});
}

void AcinerellaDecoder::prePlay()
{
	D(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, this));
	dispatch([this](){
		decodeUntilBufferFull();
		onGetReadyToPlay();
		D(dprintf("[%s]prePlay: %p ready %d\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, isReadyToPlay()));
		m_readying = true; // force onReadyToPlay() if ready
		if (isReadyToPlay())
		{
			onReadyToPlay();
		}
	});
}

void AcinerellaDecoder::play()
{
	D(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, this));
	dispatch([this](){
		decodeUntilBufferFull();
		onGetReadyToPlay();
		if (isReadyToPlay())
		{
			startPlaying();
		}
		else
		{
			D(dprintf("[%s]%s: %p not ready to play just yet bs %f rahs %f\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, this, float(bufferSize()), float(readAheadTime())));
		}
	});
}

void AcinerellaDecoder::onReadyToPlay()
{
	D(dprintf("[%s]%s: %p readying %d\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, this, m_readying));
	if (m_readying)
	{
		m_readying = false;
		if (m_client)
			m_client->onDecoderReadyToPlay(makeRef(*this));
	}
}

void AcinerellaDecoder::pause(bool willSeek)
{
	D(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV",__func__, this));

	stopPlayingQuick(); // let VideoDecoder stop pumping frames immediately

	dispatch([this, willSeek](){
		m_readying = false;
		stopPlaying();
		if (willSeek)
			flush();
	});
}

void AcinerellaDecoder::setVolume(float volume)
{
	if (isAudio())
		dispatch([this, volume](){ doSetVolume(volume); });
}

bool AcinerellaDecoder::decodeNextFrame()
{
	EP_SCOPE(DNF);
	RefPtr<AcinerellaPackage> buffer;

	DNF(dprintf("[%s]%s: this %p term %d decEOF %d %d %d buffer %f\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this, m_terminating, m_decoderEOF,
		m_droppingFrames, m_droppingUntilKeyFrame, double(bufferSize())));

	if (m_terminating)
		return false;

	if ((buffer = m_muxer->nextPackage(*this)))
	{
		AcinerellaDecodedFrame frame;
		auto acinerella = buffer->acinerella();
		auto *decoder = !!acinerella ? acinerella->decoder(m_index) : nullptr;

		if (!decoder)
			return false;

		if (m_lastDecoder != decoder)
		{
			DNF(dprintf("[%s]%s: changing decoder from %p to %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, m_lastDecoder, decoder));
			m_lastDecoder = decoder;
			onDecoderChanged(acinerella);
		}

		// used both if acinerella sends us stuff AND in case of discontinuity
		// either way, we must flush caches here!
		if (buffer->isFlushPackage())
		{
			DNF(dprintf("[%s]%s: got flush packet! (live %d hls %d)\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, m_isLive, m_isHLS));
			
			if (!m_isHLS)
			{
				ac_flush_buffers(decoder);
				flush();
			}

			return true;
		}

		if (buffer->package())
		{
			double pts = ac_get_package_pts(acinerella->instance(), buffer->package());

			if (m_droppingFrames)
			{
				if (pts < m_dropToPTS)
				{
					m_needsKF = true; // dropped frames - we'll need a keyframe!
					ac_flush_buffers(decoder);
					return true;
				}
				else if (m_needsKF)
				{
					m_droppingUntilKeyFrame = true;
					m_droppingFrames = false;
					m_needsKF = false;
					return true;
				}
				else
				{
					m_droppingFrames = false;
				}
			}
			else if (m_droppingUntilKeyFrame)
			{
				if (ac_get_package_keyframe(buffer->package()))
				{
					m_droppingUntilKeyFrame = false;
				}
				else
				{
					return true;
				}
			}
		}

		DNF(dprintf("[%s]%s: package %p ts %f\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, buffer->package(), float(ac_get_package_pts(acinerella->instance(), buffer->package()))));

		auto rcPush = ac_push_package(decoder, buffer->package());
		if (rcPush != PUSH_PACKAGE_SUCCESS)
		{
			DNF(dprintf("[%s]%s: failed ac_push_package %d\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, rcPush));
			return true; // don't fail decoding - keep going instead!
		}

		for (;;)
		{
			AcinerellaDecodedFrame frame = AcinerellaDecodedFrame(acinerella, decoder);
			auto rcFrame = ac_receive_frame(decoder, frame.frame());
			
			switch (rcFrame)
			{
			case RECEIVE_FRAME_SUCCESS:
				{
#if defined(EP_PROFILING) && EP_PROFILING
					{
						char buffer[128];
						sprintf(buffer, "frame TS %f", float(frame.frame()->timecode));
						EP_EVENTSTR(buffer);
					}
#endif
					auto lock = Locker(m_lock);
					onFrameDecoded(frame);
					DNF(dprintf("[%s]%s: decoded frame @ %f\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, float(frame.frame()->timecode)));
					m_decodedFrames.emplace(WTFMove(frame));
					m_decoderEOF = false;
				}
				break;
			case RECEIVE_FRAME_NEED_PACKET:
				// we'll have to call decodeNextFrame again
//				DNF(dprintf("[%s]%s: NEED_PACKET\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__));
				return true;
			case RECEIVE_FRAME_ERROR:
				DNF(dprintf("[%s]%s: FRAME_ERROR\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__));
				return false;
			case RECEIVE_FRAME_EOF:
				DNF(dprintf("[%s]%s: FRAME_EOF\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__));
				m_decoderEOF = true;
				return false;
			}
		}
	}
	else if (m_muxer->isEOS())
	{
		m_decoderEOF = true;
	}

	return false;
}

void AcinerellaDecoder::decodeUntilBufferFull()
{
	EP_SCOPE(untilBufferFull);

	DBF(dprintf("[%s]%s: %p - start! wmup %d prep %d\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this, m_warminUp, m_readying));

	while (bufferSize() < readAheadTime())
	{
		if (!decodeNextFrame())
			break;
	}

	if (m_warminUp && bufferSize() >= readAheadTime())
	{
		m_warminUp = false;
		if (m_client)
			m_client->onDecoderWarmedUp(makeRef(*this));
	}

	if (isReadyToPlay() && m_readying)
	{
		onReadyToPlay();
	}

	DBF(dprintf("[%s]%s: %p - buffer full (%f s)\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this, float(bufferSize())));
}

void AcinerellaDecoder::dropUntilPTS(double pts)
{
	DBF(dprintf("[%s]%s: %p - start!\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));

	m_dropToPTS = pts;
	m_droppingFrames = true;
	m_droppingUntilKeyFrame = false;
}

void AcinerellaDecoder::flush()
{
	D(dprintf("[%s]%s: islive %d\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this, m_isLive));
	auto lock = Locker(m_lock);

	while (!m_decodedFrames.empty())
		m_decodedFrames.pop();
		
	m_decoderEOF = false;
}

void AcinerellaDecoder::onPositionChanged()
{
#if defined(EP_PROFILING) && EP_PROFILING
	{
		char buffer[128];
		sprintf(buffer, "position %f", float(position()));
		EP_EVENTSTR(buffer);
	}
#endif
	DPOS(dprintf("[%s]%s: %p to %f\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this, position()));
	if (m_client)
		m_client->onDecoderUpdatedPosition(makeRef(*this), position());
}

void AcinerellaDecoder::onDurationChanged()
{
	D(dprintf("[%s]%s: %p to %f\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this, duration()));
	if (m_client)
		m_client->onDecoderUpdatedDuration(makeRef(*this), duration());
}

void AcinerellaDecoder::onEnded()
{
	EP_EVENT(ended);
	D(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
	if (m_client)
		m_client->onDecoderEnded(makeRef(*this));
}

void AcinerellaDecoder::terminate()
{
	EP_SCOPE(terminate);

	DLIFETIME(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
	m_terminating = true;
	if (!m_thread)
		return;

	onTerminate();

	DLIFETIME(dprintf("[%s]%s: %p disp..\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
	m_queue.append(makeUnique<Function<void ()>>([this] {
		performTerminate();
	}));
	m_thread->waitForCompletion();

	DLIFETIME(dprintf("[%s]%s: %p completed\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
	ASSERT(m_queue.killed());
	m_thread = nullptr;
	m_client = nullptr;
	m_muxer = nullptr;
	while (!m_decodedFrames.empty())
		m_decodedFrames.pop();

	DLIFETIME(dprintf("[%s]%s: %p done\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
}

void AcinerellaDecoder::threadEntryPoint()
{
	SetTaskPri(FindTask(0), isAudio() ? 3 : 2);

	RefPtr<AcinerellaDecoder> refSelf = WTF::makeRef(*this);

	DI(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
	if (!onThreadInitialize())
	{
		// TODO: signal failure to parent
	}
	
	while (auto function = m_queue.waitForMessage())
	{
		(*function)();
	}
	
	DI(dprintf("[%s]%s: %p .. shutting down...\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
	onThreadShutdown();
	DI(dprintf("[%s]%s: %p onThreadShutdown done\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
}

void AcinerellaDecoder::dispatch(Function<void ()>&& function)
{
	ASSERT(!m_queue.killed() && m_thread);
	if (m_terminating)
		return;
	m_queue.append(makeUnique<Function<void ()>>(WTFMove(function)));
}

void AcinerellaDecoder::performTerminate()
{
	DLIFETIME(dprintf("[%s]%s: %p\033[0m\n", isAudio() ? "\033[33mA":"\033[35mV", __func__, this));
	ASSERT(!isMainThread());
	m_queue.kill();
}

}
}

#undef D
#endif
