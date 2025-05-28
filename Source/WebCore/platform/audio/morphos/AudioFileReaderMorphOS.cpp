#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioFileReader.h"
#include "AudioBus.h"
#include "NotImplemented.h"
#include "../../graphics/morphos/acinerella.h"
#include "../../graphics/morphos/AcinerellaDecoder.h"

#include <proto/dos.h>

extern "C" {void dprintf(const char *,...);}

#define DINIT(x)
#define DDECODE(x) 
#define DIO(x)
#define READ_FLOATS 1

namespace WebCore {

class AcinerellaSoundReader
{
public:
	AcinerellaSoundReader(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
		: m_instance(ac_init())
		, m_decoder(nullptr)
		, m_data((const uint8_t *)data)
		, m_dataSize(int64_t(dataSize))
	{
		m_instance = ac_init();

		if (-1 != ac_open(m_instance, static_cast<void *>(this), nullptr, &sacReadCallback, &sacSeekCallback, nullptr, nullptr))
		{
			int audioIndex = -1;

			for (int i = 0; i < m_instance->stream_count; i++)
			{
				ac_stream_info info;
				ac_get_stream_info(m_instance, i, &info);
				switch (info.stream_type)
				{
				case AC_STREAM_TYPE_VIDEO:
					DINIT(dprintf("video stream: %dx%d index %d\n", info.additional_info.video_info.frame_width, info.additional_info.video_info.frame_height, i));
					break;

				case AC_STREAM_TYPE_AUDIO:
					DINIT(dprintf("audio stream: %d %d %d\n", info.additional_info.audio_info.samples_per_second,
						info.additional_info.audio_info.channel_count, info.additional_info.audio_info.bit_depth));
					if (-1 == audioIndex)
					{
						audioIndex = i;
						m_channels = std::min(2, (mixToMono ? 1 : info.additional_info.audio_info.channel_count));
						m_rate = info.additional_info.audio_info.samples_per_second;
					}
					break;
					
				case AC_STREAM_TYPE_UNKNOWN:
					break;
				}
			}
			
			if (-1 != audioIndex)
			{
#if READ_FLOATS
				ac_audio_output_format format = (mixToMono || m_channels == 1) ? AC_AUDIO_OUTPUT_FLOAT_1 : AC_AUDIO_OUTPUT_FLOAT_2;
				ac_set_audio_output_format(m_instance, format, sampleRate);
#else
				ac_set_audio_output_format(m_instance, AC_AUDIO_OUTPUT_16_2, sampleRate);
#endif

				m_decoder = ac_create_decoder(m_instance, audioIndex);
				m_duration = ac_get_stream_duration(m_instance, audioIndex);
			}
		}
		else
		{
			DINIT(dprintf("acinerella failed to ac_open :|\n"));
		}
	}

	bool hasAudioStream() const { return nullptr != m_decoder; }
	double duration() const { return m_duration; }
	int channels() const { return m_channels; }
	int originalRate() const { return m_rate; }
	
	// MONO
	bool decode(float *data, size_t length)
	{
#if READ_FLOATS
		ssize_t bytesLeft = ssize_t(length) * 4; //!!
#else
		ssize_t bytesLeft = ssize_t(length) * 2; //!!
#endif
		DDECODE(dprintf("%s: length %d samples, bleft %d\n", __PRETTY_FUNCTION__, length, bytesLeft));
		size_t outOffset = 0;
		bool eof = false;

		while (!eof)
		{
			auto *package = ac_read_package(m_instance);
			if (nullptr == package)
				break;
				
			auto rcPush = ac_push_package(m_decoder, package);
			if (rcPush != PUSH_PACKAGE_SUCCESS)
			{
				DDECODE(dprintf("[%s]%s: failed ac_push_package %d\033[0m\n", "\033[33mA", __func__, rcPush));
				continue;
			}

			bool keepGoing = true;
			while (keepGoing)
			{
				auto frame = ac_alloc_decoder_frame(m_decoder);

				if (nullptr == frame) // OOM
					return false;

				auto rcFrame = ac_receive_frame(m_decoder, frame);
				
				switch (rcFrame)
				{
				case RECEIVE_FRAME_SUCCESS:
					{
						const size_t copyBytes = std::min(frame->buffer_size, bytesLeft);
						DDECODE(dprintf("[%s]%s: FRAME_SUCCESS, copy %d bytes, left %d\033[0m\n", "\033[33mA", __func__, copyBytes, bytesLeft));
#if READ_FLOATS
						const float *in = (const float *)frame->pBuffer;
						for (size_t i = 0; i < copyBytes / 4; i++)
						{
							data[outOffset++] = in[i];
						}
#else
						const int16_t *in = (const int16_t *)frame->pBuffer;
						static constexpr float fdivider = 32767;

						for (size_t i = 0; i < copyBytes / 2; i++)
						{
							data[outOffset++] = float(in[i]) / fdivider;
						}
#endif
						bytesLeft -= copyBytes;
					}
					break;
				case RECEIVE_FRAME_NEED_PACKET:
				case RECEIVE_FRAME_ERROR:
					keepGoing = false;
					break;
				case RECEIVE_FRAME_EOF:
					DDECODE(dprintf("[%s]%s: FRAME_EOF\033[0m\n", "\033[33mA", __func__));
					eof = true;
					keepGoing = false;
					break;;
				}
				
				ac_free_decoder_frame(frame);
			}

		}

		DDECODE(dprintf("[%s]%s: done, bytes left %d\033[0m\n", "\033[33mA", __func__, bytesLeft));
		return true;
	}
	
	// STEREO
	bool decode(float *dataA, float *dataB, size_t length)
	{
		DDECODE(dprintf("%s: length %d samples\n", __PRETTY_FUNCTION__, length));
#if READ_FLOATS
		ssize_t bytesLeft = ssize_t(length) * 8; //!!
#else
		ssize_t bytesLeft = ssize_t(length) * 4; //!!
#endif
		size_t outOffset = 0;
		bool eof = false;

		while (!eof)
		{
			auto *package = ac_read_package(m_instance);
			if (nullptr == package)
				break;
				
			auto rcPush = ac_push_package(m_decoder, package);
			if (rcPush != PUSH_PACKAGE_SUCCESS)
			{
				DDECODE(dprintf("[%s]%s: failed ac_push_package %d\033[0m\n", "\033[33mA", __func__, rcPush));
				continue;
			}

			bool keepGoing = true;
			while (keepGoing)
			{
				auto frame = ac_alloc_decoder_frame(m_decoder);

				if (nullptr == frame) // OOM
					return false;

				auto rcFrame = ac_receive_frame(m_decoder, frame);
				
				switch (rcFrame)
				{
				case RECEIVE_FRAME_SUCCESS:
					{
						const size_t copyBytes = std::min(frame->buffer_size, bytesLeft);
						DDECODE(dprintf("[%s]%s: FRAME_SUCCESS, copy %d bytes...\033[0m\n", "\033[33mA", __func__, copyBytes));
#if READ_FLOATS
						const float *in = (const float *)frame->pBuffer;
						for (size_t i = 0; i < copyBytes / 4; i+= 2)
						{
							dataA[outOffset] = in[i];
							dataB[outOffset++] = in[i+1];
						}
#else
						const int16_t *in = (const int16_t *)frame->pBuffer;
						static constexpr float fdivider = 32767;

						for (size_t i = 0; i < copyBytes / 2; i+= 2)
						{
							dataA[outOffset] = float(in[i]) / fdivider;
							dataB[outOffset++] = float(in[i+1]) / fdivider;
						}
#endif
						bytesLeft -= copyBytes;
					}
					break;
				case RECEIVE_FRAME_NEED_PACKET:
				case RECEIVE_FRAME_ERROR:
					keepGoing = false;
					break;
				case RECEIVE_FRAME_EOF:
					DDECODE(dprintf("[%s]%s: FRAME_EOF\033[0m\n", "\033[33mA", __func__));
					eof = true;
					keepGoing = false;
					break;;
				}
				
				ac_free_decoder_frame(frame);
			}

		}

		DDECODE(dprintf("[%s]%s: done, bytes left %d\033[0m\n", "\033[33mA", __func__, bytesLeft));
		return true;
	}

	~AcinerellaSoundReader()
	{
		if (m_decoder)
			ac_free_decoder(m_decoder);
		ac_free(m_instance);
	}

protected:
	int read(uint8_t *buf, int size)
	{
		DIO(dprintf("%s: %d %lld %lld\n", __PRETTY_FUNCTION__, size, m_readPosition, m_actualReadPosition));
		if (m_readPosition != -1)
		{
			m_actualReadPosition = m_readPosition;
		}

		int64_t sizeRead = std::min(int64_t(size), (m_dataSize - m_actualReadPosition));
		
		if (sizeRead > 0)
		{
			memcpy(buf, m_data + m_actualReadPosition, sizeRead);
			m_actualReadPosition += sizeRead;
			m_readPosition = m_actualReadPosition;
		}
		else if (sizeRead < 0)
		{
			sizeRead = 0;
		}

		DIO(dprintf("%s: > %d\n", __PRETTY_FUNCTION__, int(sizeRead)));
		return sizeRead;
	}

	#ifndef AVSEEK_SIZE
	#define AVSEEK_SIZE 0x10000
	#endif

	int64_t seek(int64_t pos, int whence)
	{
		DIO(dprintf("%s: %lld %d\n", __PRETTY_FUNCTION__, pos, whence));
		int64_t newPosition = pos;
		auto streamPos = m_actualReadPosition;
		auto streamLength = m_dataSize;
			
		// we're not actually seeking until a read happens,
		// so this var may contain the value from a previous seek call
		if (m_readPosition != -1)
			streamPos = m_readPosition;

		switch (whence)
		{
		case SEEK_END:
			newPosition = streamLength + pos;
			break;
		case SEEK_CUR:
			newPosition = streamPos + pos;
			break;
		case AVSEEK_SIZE:
			return streamLength;
		default:
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
			
		return newPosition;
	}

	static int sacReadCallback(void *me, uint8_t *buf, int size)
	{
		return static_cast<AcinerellaSoundReader *>(me)->read(buf, size);
	}

	static int64_t sacSeekCallback(void *me, int64_t pos, int whence)
	{
		return static_cast<AcinerellaSoundReader *>(me)->seek(pos, whence);
	}

protected:
	ac_instance   *m_instance;
	ac_decoder    *m_decoder;
	const uint8_t *m_data;
	int64_t        m_dataSize;
	int64_t        m_actualReadPosition = 0;
	int64_t        m_readPosition = 0;
	double         m_duration;
	int            m_channels;
	int            m_rate;
};

RefPtr<AudioBus> createBusFromInMemoryAudioFile(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
{
	DINIT(dprintf("%s\n", __PRETTY_FUNCTION__));
	AcinerellaSoundReader reader(data, dataSize, mixToMono, sampleRate);

#if 0
	{
		BPTR file = Open("ram:sample.wav", MODE_NEWFILE);
		Write(file, (APTR)data, dataSize);
		Close(file);
	}
#endif

	if (reader.hasAudioStream())
	{
		DINIT(dprintf("%s: initialized, duration %fs, channels %d mixMono %d rate %f\n", __PRETTY_FUNCTION__, float(reader.duration()), reader.channels(), mixToMono, float(sampleRate)));
		auto bus = AudioBus::create(reader.channels(), ceil(reader.duration() * double(sampleRate)));
		if (bus)
		{
			bus->setSampleRate(sampleRate);

			if (mixToMono || reader.channels() == 1)
			{
				AudioChannel *channel = bus->channel(0);
				if (reader.decode(channel->mutableData(), channel->length()))
					return bus;
				return nullptr;
			}

			AudioChannel *channelA = bus->channel(0);
			AudioChannel *channelB = bus->channel(1);
			
			if ((channelA->length() == channelB->length()) && reader.decode(channelA->mutableData(), channelB->mutableData(), channelA->length()))
				return bus;
				
			return nullptr;
		}
	}

	return nullptr;
}

RefPtr<AudioBus> createBusFromAudioFile(const char* filePath, bool mixToMono, float sampleRate)
{
	notImplemented();
	return nullptr;
}

}

#endif

