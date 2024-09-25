#include "MediaSourceBufferPrivateMorphOS.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

#include "SourceBufferPrivateClient.h"
#include "MediaPlayerPrivateMorphOS.h"
#include "MediaSourcePrivateMorphOS.h"
#include "MediaDescriptionMorphOS.h"
#include "InbandTextTrackPrivate.h"
#include "AcinerellaAudioDecoder.h"
#include "AcinerellaVideoDecoder.h"
#include "AcinerellaMuxer.h"
#include "AudioTrackPrivateMorphOS.h"
#include "VideoTrackPrivateMorphOS.h"
#include "MediaSampleMorphOS.h"

#include <proto/dos.h>
#include <proto/exec.h>

#if OS(AROS)
#include <aros/debug.h>
#undef D
#define dprintf bug

namespace WTF {
template<class T, class... Args>
ALWAYS_INLINE decltype(auto) makeUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}
} // namespace WTF

using namespace WTF;
#endif

#define D(x)
#define DR(x)
#define DM(x)
#define DI(x)
#define DN(x)    
#define DIO(x)
#define DIOCC(x)
#define DBR(x)
#define DRMS(x) 

// #pragma GCC optimize ("O0")

// #define DEBUG_FILE

namespace WebCore {

MediaSourceChunkReader::MediaSourceChunkReader(MediaSourceBufferPrivateMorphOS *source, InitializationCallback &&icb, ChunkDecodedCallback &&ccb)
	: m_initializationCallback(WTFMove(icb))
	, m_chunkDecodedCallback(WTFMove(ccb))
	, m_source(source)
{
	m_thread = Thread::create("Acinerella Media Source Chunk Reader", [this] {
		while (auto function = m_queue.waitForMessage())
		{
			(*function)();
		}
	});
}

MediaSourceChunkReader::~MediaSourceChunkReader()
{
	terminate();
}

void MediaSourceChunkReader::terminate()
{
	if (m_terminating)
		return;

	m_terminating = true;
	m_event.signal();

	m_queue.append(makeUnique<Function<void ()>>([this] {
		m_queue.kill();
	}));

	m_thread->waitForCompletion();
	m_thread = nullptr;

#ifdef DEBUG_FILE
	if (0 != m_debugFile)
	{
		Close(m_debugFile);
		m_debugFile = 0;
	}
#endif
}

void MediaSourceChunkReader::decode(Vector<unsigned char>&& data)
{
	DR(dprintf("[MS]%s\n", __func__));

	{
		auto lock = holdLock(m_lock);
		m_buffer = WTFMove(data);

#ifdef DEBUG_FILE
		if (0 == m_debugFile)
		{
			char foo[32];
			sprintf(foo, "ram:%08lx.mp4", this);
			m_debugFile = Open(foo, MODE_NEWFILE);
		}
		if (m_debugFile)
			Write(m_debugFile, m_buffer.data(), m_buffer.size());
#endif
	}

	dispatch([this] {
		decodeAllMediaSamples();
	});

	m_event.signal();
}

void MediaSourceChunkReader::signalEOF()
{
	DR(dprintf("[MS]%s\n", __func__));
	m_bufferEOF = true;

	dispatch([this] {
		decodeAllMediaSamples();
	});

	m_event.signal();
}

void MediaSourceChunkReader::getSamples(MediaSamplesList& outSamples)
{
	auto lock = holdLock(m_lock);
	std::swap(outSamples, m_samples);
	DR(dprintf("[MS]%s: %d samples\n", __func__, outSamples.size()));
}

void MediaSourceChunkReader::dispatch(Function<void ()>&& function)
{
	DR(dprintf("[MS]%s\n", __func__));
	ASSERT(isMainThread());
	ASSERT(!m_queue.killed() && m_thread);
	m_queue.append(makeUnique<Function<void ()>>(WTFMove(function)));
}

bool MediaSourceChunkReader::initialize()
{
	DR(dprintf("[MS]%s\n", __func__));
	EP_SCOPE(initialize);

	m_acinerella = Acinerella::AcinerellaPointer::create();

	if (m_acinerella)
	{
		DR(dprintf("[MS] ac_open()... \n"));
		int score;
		auto probe = ac_probe_input_buffer(m_buffer.data(), m_buffer.size(), nullptr, &score);

		if (!probe)
			return false;

		if (-1 == ac_open(m_acinerella->instance(), static_cast<void *>(this), nullptr, &acReadCallback, nullptr, nullptr, probe))
		{
			return false;
		}
		
		DR(dprintf("[MS] ac_open() success!\n"));
		m_audioDecoderMask = 0;
		m_videoDecoderMask = 0;
		
		for (int i = 0; i < std::min(Acinerella::AcinerellaMuxedBuffer::maxDecoders, m_acinerella->instance()->stream_count); i++)
		{
			ac_stream_info info;
			ac_get_stream_info(m_acinerella->instance(), i, &info);

			switch (info.stream_type)
			{
			case AC_STREAM_TYPE_VIDEO:
				m_videoDecoderMask |= (1UL << i);
				break;

			case AC_STREAM_TYPE_AUDIO:
				m_audioDecoderMask |= (1UL << i);
				break;
			
			default:
				break;
			}
		}
		
		return true;
	}
		
	return false;
}

void MediaSourceChunkReader::getMeta(WebCore::SourceBufferPrivateClient::InitializationSegment& initializationSegment, MediaPlayerMorphOSInfo& minfo)
{
	DM(dprintf("[MS]%s\n", __func__));
	auto metaCinerella = m_acinerella;

	if (metaCinerella)
	{
		// Need to build and forward the InitializationSegment now...
		double duration = 0.0;
		minfo.m_width = 0;
		minfo.m_isLive = false;
		minfo.m_channels = 0;
        minfo.m_isDownloadable = false;

		DM(dprintf("%s: streams %d\n", __func__, metaCinerella->instance()->stream_count));

		for (int i = 0; i < std::min(Acinerella::AcinerellaMuxedBuffer::maxDecoders, metaCinerella->instance()->stream_count); i++)
		{
			ac_stream_info info;
			ac_get_stream_info(metaCinerella->instance(), i, &info);

			DM(dprintf("%s: index %d st %d\n", __func__, i, info.stream_type));

			switch (info.stream_type)
			{
			case AC_STREAM_TYPE_VIDEO:
				{
					WebCore::SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation videoTrackInformation;
					videoTrackInformation.track = VideoTrackPrivateMorphOSMS::create(m_source, i);
					videoTrackInformation.description = MediaDescriptionMorphOS::createVideoWithCodec(ac_codec_name(metaCinerella->instance(), i));
					initializationSegment.videoTracks.append(WTFMove(videoTrackInformation));
					duration = std::max(duration, std::max(double(ac_get_stream_duration(metaCinerella->instance(), i)), double(metaCinerella->instance()->info.duration)/1000.0));
					DM(dprintf("%s: video %d %f codec %s\n", __func__, i, float(duration), ac_codec_name(metaCinerella->instance(), i)));
					minfo.m_width = info.additional_info.video_info.frame_width;
					minfo.m_height = info.additional_info.video_info.frame_height;
				}
				break;
				
			case AC_STREAM_TYPE_AUDIO:
				{
					WebCore::SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation audioTrackInformation;
					audioTrackInformation.track = AudioTrackPrivateMorphOSMS::create(m_source, i);
					audioTrackInformation.description = MediaDescriptionMorphOS::createAudioWithCodec(ac_codec_name(metaCinerella->instance(), i));
					initializationSegment.audioTracks.append(WTFMove(audioTrackInformation));
					duration = std::max(duration, std::max(double(ac_get_stream_duration(metaCinerella->instance(), i)), double(metaCinerella->instance()->info.duration)/1000.0));
					DM(dprintf("%s: audio %d %f codec %s\n", __func__, i, float(duration), ac_codec_name(metaCinerella->instance(), i)));
					minfo.m_channels = info.additional_info.audio_info.channel_count;
					minfo.m_frequency = info.additional_info.audio_info.samples_per_second;
					minfo.m_bits = info.additional_info.audio_info.bit_depth;
				}
				break;

			default:
				break;
			}
		}

		initializationSegment.duration = MediaTime::createWithDouble(duration);
		minfo.m_duration = duration;
		m_highestPTS = duration; // we don't need to synchronize duration if we already know it
	}
}

bool MediaSourceChunkReader::keepDecoding()
{
	auto lock = holdLock(m_lock);

// dprintf("[MS]%s: term %d bs %d bp %d eof %d reof %d lover %d\n", __func__, m_terminating, m_buffer.size(), m_bufferPosition, m_bufferEOF, m_readEOF, m_leftOver.size());

	if (m_terminating || m_readEOF)
		return false;

	if (m_bufferEOF)
		return true;

	if (m_leftOver.size() > 0 && m_buffer.size() == 0)
		return false;
		
	return true;
}

void MediaSourceChunkReader::decodeAllMediaSamples()
{
	DR(dprintf("[MS]%s\n", __func__));

	if (!m_acinerella)
	{
		WebCore::SourceBufferPrivateClient::InitializationSegment segment;
		MediaPlayerMorphOSInfo info;
		
		if (initialize())
		{
			getMeta(segment, info);
			m_initializationCallback(true, segment, info);
		}
		else
		{
			m_initializationCallback(false, segment, info);
			return;
		}
	}

	
	DN(dprintf("%s: audiomask %lx videomask %lx\n", __func__, m_audioDecoderMask, m_videoDecoderMask));
	DN(int total = 0);
	
	while (keepDecoding())
	{
		RefPtr<Acinerella::AcinerellaPackage> package = Acinerella::AcinerellaPackage::create(m_acinerella, ac_read_package(m_acinerella->instance()));

		if (package.get() && package->package())
		{
			String trackID;
			if (m_audioDecoderMask & (1uL << package->index()))
			{
				trackID = "A" + String::number(package->index());
			}
			else if (m_videoDecoderMask & (1uL << package->index()))
			{
				trackID = "V" + String::number(package->index());
			}
			else
			{
				// reject unknown packets completely
				continue;
			}

			RefPtr<MediaSample> mediaSample = MediaSampleMorphOS::create(package, FloatSize(320, 240), trackID);
			
			if (mediaSample->presentationTime().toDouble() - 1.0 > m_highestPTS)
			{
				m_highestPTS = mediaSample->presentationTime().toDouble();
			}
			
			DN(m_decodeCount++);
			DN(if (0 == (m_decodeCount % 15)) dprintf("%s: %s sample created (PTS %f)\n", __func__, (m_audioDecoderMask & (1uLL << package->index())) ? "audio" : "video",
				mediaSample->presentationTime().toFloat()));
			DN(total++);

			auto lock = holdLock(m_lock);
			m_samples.emplace_back(mediaSample);
		}
		else if (m_bufferEOF)
		{
			m_readEOF = true;
		}
	}

	DN(dprintf("%s: total decoded packets %lu\n", __func__, total));
}

int MediaSourceChunkReader::read(uint8_t *buf, int size)
{
	EP_SCOPE(read);

	int sizeLeft = size;
	int pos = 0;

	DIO(dprintf("[MS]%s>> %p size %d\n", __func__, this, size));

	while (sizeLeft > 0)
	{
		bool wait = false;
		bool chunkDecoded = false;

		{
			auto lock = holdLock(m_lock);
			int leftBufferSize = m_leftOver.size();
			
			// Acinerella will call us once with size=1024 on startup (probing), then keep calling with AC_BUFSIZE
			ASSERT(leftBufferSize < size);
			if (leftBufferSize > size)
				return -1;
			
			if (leftBufferSize > 0)
			{
				int toCopy = leftBufferSize;
			
				DIO(dprintf("[MS]%s: [O] left %d pos %d bs %d tc %d\n", __func__, sizeLeft, pos, leftBufferSize, toCopy));
				
				if (toCopy > 0)
				{
					memcpy(buf + pos, m_leftOver.data(), toCopy);
					sizeLeft -= toCopy;
					pos += toCopy;
					m_leftOver.clear();
				}
				
				if (m_bufferEOF)
				{
					return size - sizeLeft;
				}
				
				continue;
			}
			else
			{
				int bufferSize = m_buffer.size();
				int toCopy = std::min(sizeLeft, bufferSize - m_bufferPosition);
				
				DIO(dprintf("[MS]%s: [B] left %d pos %d bs %d bp %d tc %d\n", __func__, sizeLeft, pos, bufferSize, m_bufferPosition, toCopy));
				
				if (toCopy > 0)
				{
					memcpy(buf + pos, m_buffer.data() + m_bufferPosition, toCopy);
					sizeLeft -= toCopy;
					pos += toCopy;
					m_bufferPosition += toCopy;
				}

				if ((m_bufferPosition >= int(m_buffer.size() - AC_BUFSIZE)) && (m_bufferPosition > 0))
				{
					int leftOver = m_buffer.size() - m_bufferPosition;

					if (leftOver)
					{
						m_leftOver.resize(leftOver);
						memcpy(m_leftOver.data(), m_buffer.data() + m_bufferPosition, leftOver);
					}

					m_buffer.clear();
					m_bufferPosition = 0;

					DIOCC(dprintf("[MS]%s -- chunk consumed, leftover %d \n", __func__, leftOver));

					chunkDecoded = true;
					wait = true;
				}
				else if (m_terminating || m_bufferEOF)
				{
					return -1;
				}
				else if (0 == bufferSize)
				{
					wait = true;
				}
			}
		} // lock

		if (chunkDecoded)
			m_chunkDecodedCallback(true);

		if (wait)
			m_event.waitFor(10_s);
	}
	
	DIO(dprintf("[MS]%s<< read %d pos %d\n", __func__, size - sizeLeft, pos));

	return size - sizeLeft;
}

int MediaSourceChunkReader::acReadCallback(void *me, uint8_t *buf, int size)
{
	return static_cast<MediaSourceChunkReader *>(me)->read(buf, size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Ref<MediaSourceBufferPrivateMorphOS> MediaSourceBufferPrivateMorphOS::create(MediaSourcePrivateMorphOS* parent)
{
	D(dprintf("[MS]%s\n", __func__));
	return adoptRef(*new MediaSourceBufferPrivateMorphOS(parent));
}

MediaSourceBufferPrivateMorphOS::MediaSourceBufferPrivateMorphOS(MediaSourcePrivateMorphOS* parent)
    : m_mediaSource(parent)
    , m_client(0)
{
	memset(&m_decodersStarved, 0, sizeof(m_decodersStarved));
	m_reader = MediaSourceChunkReader::create(this,
		[this](bool success, WebCore::SourceBufferPrivateClient::InitializationSegment& segment, MediaPlayerMorphOSInfo& info){
			initialize(success, segment, info);
		},
		[this](bool success){
			appendComplete(success);
		}
	);

	m_thread = Thread::create("Acinerella Media Source Buffer", [this] {
		threadEntryPoint();
	});

	DI(dprintf("[MS]%s: %p hello!\n", __func__, this));
}

MediaSourceBufferPrivateMorphOS::~MediaSourceBufferPrivateMorphOS()
{
	DI(dprintf("[MS]%s: %p bye!\n", __func__, this));
	clearMediaSource();
}

void MediaSourceBufferPrivateMorphOS::setClient(SourceBufferPrivateClient* client)
{
	D(dprintf("[MS]%s client %p main %d\n", __func__, client, isMainThread()));
	m_client = client;
}

void MediaSourceBufferPrivateMorphOS::append(Vector<unsigned char>&&vector)
{
	EP_EVENT(append);
	D(dprintf("[MS]%s bytes %lu main %d\n", __func__, vector.size(), isMainThread()));
	m_reader->decode(WTFMove(vector));
	m_appendCount ++;
//	m_client->sourceBufferPrivateAppendComplete(SourceBufferPrivateClient::ReadStreamFailed);
}

void MediaSourceBufferPrivateMorphOS::appendComplete(bool success)
{
	DI(dprintf("[MS]%s: %p %d\n", __func__, this, success));
	WTF::callOnMainThread([success, this, protect = makeRef(*this)]() {
		EP_EVENT(appendComplete);
		if (m_client && !m_terminating)
		{
			MediaSourceChunkReader::MediaSamplesList samples;
			m_reader->getSamples(samples);
			m_appendCompleteCount ++;
			DI(dprintf("[MS]%s: %p %d, queue %d samples\n", __func__, this, success, samples.size()));

			for (auto sample : samples)
			{
				m_client->sourceBufferPrivateDidReceiveSample(*sample.get());
			}
			
			if (m_mediaSource && success)
			{
				m_mediaSource->onSourceBufferLoadingProgressed();
			}

			m_client->sourceBufferPrivateAppendComplete(success ? SourceBufferPrivateClient::AppendSucceeded : SourceBufferPrivateClient::ParsingFailed);

#if 0
			if (m_info.m_duration < m_reader->highestPTS())
			{
				m_info.m_duration = m_reader->highestPTS();
				RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
				if (m_mediaSource)
					m_mediaSource->onSourceBufferChangedDuration(me, m_info.m_duration);
			}
#endif
		}
	});
}

void MediaSourceBufferPrivateMorphOS::willSeek(double time)
{
	DI(dprintf("[MS]%s %p\n", __func__, this));

	if (m_seeking)
		return;
		
	m_seeking = true;
	m_seekTime = time;

	for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
	{
		if (!!m_decoders[i])
		{
			m_decoders[i]->pause(true);
		}
	}
	
	flush();
}

void MediaSourceBufferPrivateMorphOS::signalEOF()
{
	m_reader->signalEOF();
}

void MediaSourceBufferPrivateMorphOS::setVolume(double vol)
{
	for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
	{
		if (!!m_decoders[i])
		{
			m_decoders[i]->setVolume(vol);
		}
	}
}

void MediaSourceBufferPrivateMorphOS::clearMediaSource()
{
	DI(dprintf("[MS]%s %p\n", __func__, this));

	terminate();
	m_mediaSource = nullptr;
}

void MediaSourceBufferPrivateMorphOS::abort()
{
	DI(dprintf("[MS]%s %p\n", __func__, this));

}

void MediaSourceBufferPrivateMorphOS::terminate()
{
	EP_SCOPE(abort);

	if (m_terminating)
		return;

	DI(dprintf("[MS]%s %p\n", __func__, this));
	
	m_terminating = true;
	m_paintingDecoder = nullptr;

	m_event.signal();

	DI(dprintf("[MS]%s %p muxer shutdown\n", __func__, this));
	if (m_muxer)
		m_muxer->terminate();

	for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
	{
		if (!!m_decoders[i])
		{
			DI(dprintf("[MS]%s %p decoder %p shutdown\n", __func__, this, m_decoders[i].get()));
			m_decoders[i]->terminate();
			m_decoders[i] = nullptr;
		}
	}

	if (m_reader)
		m_reader->terminate();
	m_reader = nullptr;

	DI(dprintf("[MS]%s %p thread shutdown\n", __func__, this));
	if (!m_thread)
	{
		DI(dprintf("[MS]%s %p already done\n", __func__, this));
		return;
	}
	ASSERT(isMainThread());
	ASSERT(!m_queue.killed() && m_thread);

	if (!m_thread)
		return;

	m_queue.append(makeUnique<Function<void ()>>([this] {
		performTerminate();
	}));

	m_thread->waitForCompletion();
	m_thread = nullptr;
    m_segment = WebCore::SourceBufferPrivateClient::InitializationSegment();
	ASSERT(m_queue.killed());
	DI(dprintf("[MS]%s %p done\n", __func__, this));

}

void MediaSourceBufferPrivateMorphOS::resetParserState()
{
	D(dprintf("[MS]%s\n", __func__));
}

void MediaSourceBufferPrivateMorphOS::removedFromMediaSource()
{
	D(dprintf("[MS]%s\n", __func__));
	terminate();
	RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
	if (m_mediaSource)
		m_mediaSource->onSourceBufferRemoved(me);
    m_segment = WebCore::SourceBufferPrivateClient::InitializationSegment();
}

void MediaSourceBufferPrivateMorphOS::onTrackEnabled(int index, bool enabled)
{
	D(dprintf("[MS]%s: %d enabled %d\n", __func__, index, enabled));
    if (m_mediaSource)
    {
        if (enabled)
        {
            if (!!m_decoders[index])
            {
                if (m_mediaSource->paused())
                {
                    m_decoders[index]->warmUp();
                }
                else
                {
                    m_decoders[index]->prePlay();
                }
            }
        }
        else if (!enabled)
        {
            if (!!m_decoders[index])
            {
                m_decoders[index]->coolDown();
            }
        }
    }
}

void MediaSourceBufferPrivateMorphOS::dumpStatus()
{
	int numDec = 0;
	for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
	{
		if (!!m_decoders[i])
			numDec++;
	}
	dprintf("\033[36m[MSB%p]: DEC %d SEEK %d TERM %d RMS %d AC %d ACC %d ENQCNT %d\033[0m\n", this, numDec, m_seeking, m_terminating, m_readyForMoreSamples, m_appendCount, m_appendCompleteCount, m_enqueueCount);
	for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
	{
		if (!!m_decoders[i])
		{
			dprintf("[%d]", m_muxer->packagesForDecoder(i));
			m_decoders[i]->dumpStatus();
		}
	}
	dprintf("\033[36m[MSB%p]: -- \033[0m\n", this);
}

void MediaSourceBufferPrivateMorphOS::getFrameCounts(unsigned& decoded, unsigned &dropped) const
{
	Acinerella::AcinerellaVideoDecoder *decoder = static_cast<Acinerella::AcinerellaVideoDecoder *>(m_paintingDecoder.get());
	if (decoder)
	{
		decoded = decoder->decodedFrameCount();
		dropped = decoder->droppedFrameCount();
	}
	else
	{
		decoded = dropped = 0;
	}
}

void MediaSourceBufferPrivateMorphOS::flush(const AtomicString& trackID)
{
	int trackNo = atoi(trackID.string().ascii().data() + 1);
	D(dprintf("[MS]%s(as): %s -> %d\n", __func__, trackID.string().utf8().data(), trackNo));
	if (trackNo >= 0 && trackNo < Acinerella::AcinerellaMuxedBuffer::maxDecoders)
	{
		m_muxer->flush(trackNo);
		RefPtr<Acinerella::AcinerellaPackage> package = Acinerella::AcinerellaPackage::create(m_reader->acinerella(), ac_flush_packet());
		m_muxer->push(package, trackNo);
	}
}

void MediaSourceBufferPrivateMorphOS::becomeReadyForMoreSamples(int index)
{
	DRMS(dprintf("[MS]%s index %d\n", __func__, index));
	m_readyForMoreSamples = true;
	if (!m_decodersStarved[index])
	{
		m_decodersStarved[index] = true;

		if (!!m_decoders[index])
		{
			WTF::callOnMainThread([this, protect = makeRef(*this), isVideo = m_decoders[index]->isVideo(), index]() {
				AtomicString id;

				DBR(dprintf("[MS]becomeReadyForMoreSamples %d\n", index));

				if (isVideo) {
					id = "V" + String::number(index);
				}
				else {
					id = "A" + String::number(index);
				}

				m_requestedMoreFrames = true;
				if (m_client)
					m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples(id);
			});
		}
	}
}

void MediaSourceBufferPrivateMorphOS::flush()
{
	D(dprintf("[MS]%s\n", __func__));

	if (m_muxer && m_reader)
	{
		m_muxer->flush();
		RefPtr<Acinerella::AcinerellaPackage> package = Acinerella::AcinerellaPackage::create(m_reader->acinerella(), ac_flush_packet());
		m_muxer->push(package);
	}
}

void MediaSourceBufferPrivateMorphOS::enqueueSample(Ref<MediaSample>&&sample, const AtomicString&)
{
	auto msample = static_cast<MediaSampleMorphOS *>(&sample.get());
	RefPtr<Acinerella::AcinerellaPackage> package = msample->package();
	int index = package->index();

    m_enqueuedSamples = true;
	m_enqueueCount ++;

	D(if (0 == (m_enqueueCount % 50) || (msample->isSync() && !(m_audioDecoderMask & (1uLL << package->index())))) dprintf("[MS][%s]%s PTS %f key %d\n", __func__, (m_audioDecoderMask & (1uLL << package->index())) ? "A":"V", msample->presentationTime().toFloat(), msample->isSync()));

	if (index >= Acinerella::AcinerellaMuxedBuffer::maxDecoders)
		return;

	m_requestedMoreFrames = false;
	m_eos = false;

	if (m_seeking)
	{
		double pts = msample->presentationTime().toDouble();
		if (msample->isSync() && pts > m_seekTime - 2.5 && pts < m_seekTime + 15.0)
		{
			m_seeking = false;
			D(dprintf("[MS]Keyframe found!\n"));
			play();
		}
		else
		{
			return;
		}
	}

	m_muxer->push(package);
	
	if (!!m_decoders[index] && m_muxer->packagesForDecoder(index) >= Acinerella::AcinerellaMuxedBuffer::queueReadAheadSize &&
		m_decodersStarved[index])
	{
		m_decodersStarved[index] = false;
		m_decoders[index]->warmUp();
	}
}

void MediaSourceBufferPrivateMorphOS::allSamplesInTrackEnqueued(const AtomicString&)
{
	D(dprintf("[MS]%s\n", __func__));
	m_eos = true;
	RefPtr<Acinerella::AcinerellaPackage> nothing;
	m_muxer->push(nothing);
}

bool MediaSourceBufferPrivateMorphOS::isReadyForMoreSamples(const AtomicString&)
{
// 	D(dprintf("[MS]%s %d\n", __func__, m_readyForMoreSamples));
	return m_readyForMoreSamples;
}

void MediaSourceBufferPrivateMorphOS::setActive(bool isActive)
{
	D(dprintf("[MS]%s\n", __func__));
    if (m_mediaSource)
    {
		RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
        m_mediaSource->onSourceBufferDidChangeActiveState(me, isActive);
	}
}

void MediaSourceBufferPrivateMorphOS::notifyClientWhenReadyForMoreSamples(const AtomicString&trackID)
{
	D(dprintf("[MS]%s\n", __func__));
	(void)trackID;
//    if (m_client)
//        m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples(trackID);
}

bool MediaSourceBufferPrivateMorphOS::canSetMinimumUpcomingPresentationTime(const AtomicString&) const
{
	DBR(dprintf("[MS]%s\n", __func__));
	// WARNING: HACK
	// SourceBuffer calls this after a batch of enqueueSample calls
	auto *me = const_cast<MediaSourceBufferPrivateMorphOS *>(this);
    if (m_enqueuedSamples)
    {
        me->m_enqueuedSamples = false;
        me->warmUp();
    }
	return false;
}

MediaPlayer::ReadyState MediaSourceBufferPrivateMorphOS::readyState() const
{
	D(dprintf("[MS]%s\n", __func__));
	if (m_mediaSource)
		return m_mediaSource->readyState();
	return MediaPlayer::ReadyState::HaveNothing;
}

void MediaSourceBufferPrivateMorphOS::setReadyState(MediaPlayer::ReadyState rs)
{
	D(dprintf("[MS]%s %d\n", __func__, int(rs)));
	if (m_mediaSource)
		m_mediaSource->setReadyState(rs);
}

void MediaSourceBufferPrivateMorphOS::initialize(bool success,
	WebCore::SourceBufferPrivateClient::InitializationSegment& segment,
	MediaPlayerMorphOSInfo& minfo)
{
	RefPtr<Acinerella::AcinerellaPointer> acinerella = m_reader->acinerella();

	if (!success)
	{
		return; // TODO: how do we handle this?
	}

	EP_SCOPE(initialize);
	DM(dprintf("[MS]ac initialized, stream count %d\n", acinerella->instance()->stream_count));
	double duration = 0.0;
	uint32_t decoderIndexMask = 0;

	m_muxer = Acinerella::AcinerellaMuxedBuffer::create();
	m_info = minfo;
    m_segment = segment;
	m_audioDecoderMask = 0;

	for (int i = 0; i < std::min(Acinerella::AcinerellaMuxedBuffer::maxDecoders, acinerella->instance()->stream_count); i++)
	{
		ac_stream_info info;
		ac_get_stream_info(acinerella->instance(), i, &info);

		switch (info.stream_type)
		{
		case AC_STREAM_TYPE_VIDEO:
			DM(dprintf("video stream: %dx%d\n", info.additional_info.video_info.frame_width, info.additional_info.video_info.frame_height));
			acinerella->setDecoder(i, ac_create_decoder(acinerella->instance(), i));
			m_decoders[i] = Acinerella::AcinerellaVideoDecoder::create(this, acinerella, m_muxer, i, info, false);
			if (!!m_decoders[i])
			{
				duration = std::max(duration, m_decoders[i]->duration());
				DM(dprintf("[MS] video decoder created, duration %f\n", duration));
				decoderIndexMask |= (1ULL << i);
				ac_decoder_fake_seek(acinerella->decoder(i));
				Acinerella::AcinerellaVideoDecoder *vdecoder = static_cast<Acinerella::AcinerellaVideoDecoder *>(m_decoders[i].get());
				vdecoder->setCanDropKeyFrames(true); // needed for Media Source to function better on seek/catchup, we don't want this for single-file non-MS playback
			}
			break;

		case AC_STREAM_TYPE_AUDIO:
			DM(dprintf("audio stream: %d %d %d\n", info.additional_info.audio_info.samples_per_second,
				info.additional_info.audio_info.channel_count, info.additional_info.audio_info.bit_depth));
			acinerella->setDecoder(i, ac_create_decoder(acinerella->instance(), i));
			m_decoders[i] = Acinerella::AcinerellaAudioDecoder::create(this, acinerella, m_muxer, i, info, false);
			if (!!m_decoders[i])
			{
				duration = std::max(duration, m_decoders[i]->duration());
				DM(dprintf("[MS] audio decoder created, duration %f\n", float(duration)));
				decoderIndexMask |= (1ULL << i);
				m_audioDecoderMask |= (1ULL << i);
				ac_decoder_fake_seek(acinerella->decoder(i));
			}
			break;
			
		case AC_STREAM_TYPE_UNKNOWN:
			break;
		}
	}

	if (decoderIndexMask != 0)
	{
		m_muxer->setDecoderMask(decoderIndexMask, m_audioDecoderMask);
		m_muxer->setSinkFunction([this, protectedThis = makeRef(*this)](int decoderIndex, int sizeLeft) {
			if (0 == sizeLeft)
				becomeReadyForMoreSamples(decoderIndex);
			return false; // avoid blocking the pipeline!
		});

		warmUp();

		DM(dprintf("[MS] duration %f size %dx%d\n", float(duration), m_info.m_width, m_info.m_height));
		WTF::callOnMainThread([segment, duration, this, protect = makeRef(*this)]() {
			DM(dprintf("[MS] calling sourceBufferPrivateDidReceiveInitializationSegment, duration %f size %dx%d\n", float(duration), m_info.m_width, m_info.m_height));
			if (m_client)
				m_client->sourceBufferPrivateDidReceiveInitializationSegment(segment);
		});

		m_metaInitDone = true;
		RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
		if (m_mediaSource)
			m_mediaSource->onSourceBufferInitialized(me);
	}
}

void MediaSourceBufferPrivateMorphOS::warmUp()
{
	D(dprintf("[MS] warmUp\n"));

	dispatch([this]() {
		for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
		{
			if (!!m_decoders[i])
			{
				D(dprintf("[MS] warmup decoder index %d - %p\n", i, m_decoders[i].get()));
				m_decoders[i]->warmUp();
			}
		}
	});
}

void MediaSourceBufferPrivateMorphOS::coolDown()
{
	dispatch([this]() {
		for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
		{
			if (!!m_decoders[i])
			{
				D(dprintf("[MS] coolDown decoder index %d - %p\n", i, m_decoders[i].get()));
				m_decoders[i]->coolDown();
			}
		}
	});
}

void MediaSourceBufferPrivateMorphOS::threadEntryPoint()
{
	while (auto function = m_queue.waitForMessage())
	{
		(*function)();
	}
}

void MediaSourceBufferPrivateMorphOS::dispatch(Function<void ()>&& function)
{
	ASSERT(isMainThread());
	ASSERT(!m_queue.killed() && m_thread);
	m_queue.append(makeUnique<Function<void ()>>(WTFMove(function)));
}

void MediaSourceBufferPrivateMorphOS::performTerminate()
{
	D(dprintf("[MS]%s: %p\n", __func__, this));

	ASSERT(!isMainThread());
	m_queue.kill();
}

void MediaSourceBufferPrivateMorphOS::play()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	EP_EVENT(play);
	dispatch([this] {
		D(dprintf("%s: ... \n", __PRETTY_FUNCTION__));
		for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
		{
			if (!!m_decoders[i])
			{
				D(dprintf("%s: play at index %d\n", __PRETTY_FUNCTION__, i));
				m_decoders[i]->play();
			}
		}
	});
}

void MediaSourceBufferPrivateMorphOS::prePlay()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	EP_EVENT(play);
	dispatch([this] {
		D(dprintf("%s: ... \n", __PRETTY_FUNCTION__));
		for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
		{
			if (!!m_decoders[i])
			{
				D(dprintf("%s: play at index %d\n", __PRETTY_FUNCTION__, i));
				m_decoders[i]->prePlay();
			}
		}
	});
}

void MediaSourceBufferPrivateMorphOS::pause()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	EP_EVENT(pause);
	dispatch([this] {
		for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
		{
			if (!!m_decoders[i])
			{
				m_decoders[i]->pause();
			}
		}
	});
}

const WebCore::MediaPlayerMorphOSStreamSettings& MediaSourceBufferPrivateMorphOS::streamSettings()
{
	static WebCore::MediaPlayerMorphOSStreamSettings defaults;
	if (m_mediaSource)
	{
		return m_mediaSource->streamSettings();
	}
	return defaults;
}

void MediaSourceBufferPrivateMorphOS::onDecoderWarmedUp(RefPtr<Acinerella::AcinerellaDecoder>)
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
}

void MediaSourceBufferPrivateMorphOS::onDecoderReadyToPlay(RefPtr<Acinerella::AcinerellaDecoder>)
{
	D(dprintf("%s: decoders ready: %d\n", __PRETTY_FUNCTION__, areDecodersReadyToPlay()));
	if (areDecodersReadyToPlay())
	{
		if (m_mediaSource)
			m_mediaSource->onSourceBuffersReadyToPlay();
	}
}

void MediaSourceBufferPrivateMorphOS::onDecoderPlaying(RefPtr<Acinerella::AcinerellaDecoder>, bool)
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));

}

void MediaSourceBufferPrivateMorphOS::onDecoderUpdatedBufferLength(RefPtr<Acinerella::AcinerellaDecoder>, double)
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));

}

void MediaSourceBufferPrivateMorphOS::onDecoderUpdatedPosition(RefPtr<Acinerella::AcinerellaDecoder> decoder, double position)
{
	D(dprintf("[MS]%s decoder %p isaudio %d mask %x position %f\n", __func__, decoder.get(), ((1ULL << decoder->index()) & m_audioDecoderMask) ? 1 : 0,
		m_audioDecoderMask, float(position)));

	if ((1ULL << decoder->index()) & m_audioDecoderMask)
	{
		RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
		if (m_mediaSource)
			m_mediaSource->onAudioSourceBufferUpdatedPosition(me, position);
	}
}

void MediaSourceBufferPrivateMorphOS::onDecoderUpdatedDuration(RefPtr<Acinerella::AcinerellaDecoder>, double)
{
	// live streams
}

void MediaSourceBufferPrivateMorphOS::onDecoderEnded(RefPtr<Acinerella::AcinerellaDecoder> decoder)
{
	WTF::callOnMainThread([this, protect = makeRef(*this), decoder]() {
		if (m_mediaSource && !m_terminating)
		{
			RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
			m_mediaSource->onSourceBufferEnded(me);
		}
	});
}

void MediaSourceBufferPrivateMorphOS::paint(GraphicsContext& gc, const FloatRect& rect)
{
//	DI(dprintf("[MS]%s: %p decoder %p\n", __func__, this, m_paintingDecoder.get()));
	if (!!m_paintingDecoder)
		m_paintingDecoder->paint(gc, rect);
}

void MediaSourceBufferPrivateMorphOS::setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height)
{
	DI(dprintf("[MS]%s: %p decoder %p\n", __func__, this, m_paintingDecoder.get()));
	if (!!m_paintingDecoder)
	{
		Acinerella::AcinerellaVideoDecoder *decoder = static_cast<Acinerella::AcinerellaVideoDecoder *>(m_paintingDecoder.get());
		decoder->setOverlayWindowCoords(w, scrollx, scrolly, mleft, mtop, mright, mbottom, width, height);
	}
}

void MediaSourceBufferPrivateMorphOS::setAudioPresentationTime(double apts)
{
	if (!!m_paintingDecoder)
	{
		Acinerella::AcinerellaVideoDecoder *decoder = static_cast<Acinerella::AcinerellaVideoDecoder *>(m_paintingDecoder.get());
		decoder->setAudioPresentationTime(apts);
	}
}

bool MediaSourceBufferPrivateMorphOS::areDecodersReadyToPlay()
{
	for (int i = 0; i < Acinerella::AcinerellaMuxedBuffer::maxDecoders; i++)
	{
		if (!!m_decoders[i])
		{
			if (!m_decoders[i]->isReadyToPlay())
            {
                D(dprintf("[MS]%s: not yet ready index %d type %s\n", __func__, i, m_decoders[i]->isAudio() ? "A" : "V"));
				return false;
            }
		}
	}
	return true;
}

void MediaSourceBufferPrivateMorphOS::onDecoderWantsToRender(RefPtr<Acinerella::AcinerellaDecoder> decoder)
{
	DI(if (!m_paintingDecoder) dprintf("[MS]%s: %p decoder %p\n", __func__, this, m_paintingDecoder.get()));
	EP_EVENT(readyToPaint);
	m_paintingDecoder = decoder;

	WTF::callOnMainThread([this, protect = makeRef(*this), decoder]() {
		if (m_mediaSource && !m_terminating)
		{
			RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
			EP_EVENT(readyToPaintMT);
			m_mediaSource->onSourceBufferReadyToPaint(me);
		}
	});
}

void MediaSourceBufferPrivateMorphOS::onDecoderNotReadyToRender(RefPtr<Acinerella::AcinerellaDecoder> decoder)
{
	if (decoder == m_paintingDecoder)
		m_paintingDecoder = nullptr;
}

void MediaSourceBufferPrivateMorphOS::onDecoderRenderUpdate(RefPtr<Acinerella::AcinerellaDecoder> decoder)
{
	if (decoder == m_paintingDecoder)
	{
		WTF::callOnMainThread([this, protect = makeRef(*this), decoder]() {
			if (m_mediaSource && !m_terminating) {
				RefPtr<MediaSourceBufferPrivateMorphOS> me = makeRef(*this);
				m_mediaSource->onSourceBufferFrameUpdate(me);
			}
		});
	}
}

#if 0
void MediaSourceBufferPrivateMorphOS::onDecoderRanOutOfFrames(RefPtr<Acinerella::AcinerellaDecoder> decoder)
{
	if (!m_requestedMoreFrames)
	{
		WTF::callOnMainThread([this, protect = makeRef(*this), decoder]() {
			if (!m_requestedMoreFrames)
			{
				AtomicString id;

				dprintf("ODROOF!\n");

				if (decoder->isVideo()) {
					id = "V" + String::number(decoder->index());
				}
				else {
					id = "A" + String::number(decoder->index());
				}

				m_requestedMoreFrames = true;
				if (m_client)
					m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples(id);
			}
		});
	}
}
#endif

} // namespace
#endif
