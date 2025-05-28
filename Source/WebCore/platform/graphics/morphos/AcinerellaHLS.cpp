#include "AcinerellaHLS.h"
#include <wtf/DateMath.h>
#include <wtf/text/StringToIntegerConversion.h>

#if ENABLE(VIDEO)

#include <proto/exec.h>

namespace WebCore {
namespace Acinerella {

#define D(x) 
#define DCONTENTS(x) 
#define DIO(x)
#define DENC(x) 

static const String rnReplace("\r\n");
static const String rnReplacement("\n");

class HLSMasterPlaylistParser
{
public:
	HLSMasterPlaylistParser(const URL &baseURL, const String &sdata)
	{
		Vector<String> lines = String(sdata).replace(rnReplace, rnReplacement).split('\n');

		// Must contain at the very least:
		// #EXTM3U
		// #EXT-X-VERSION:*
		// #EXT-X-STREAM-INF
		// url
		//
		// So 4 lines at minimum, otherwise it won't be a valid playlist
		if (lines.size() >= 4 && equalIgnoringASCIICase(lines[0], "#extm3u"))
		{
			// Format kida-verified. Look for Streams
			HLSStreamInfo info;
			bool hopingForM3U8 = false;
			bool foundChunks = false;

			for (size_t i = 2; i < lines.size(); i++)
			{
				String& line = lines[i];

				DCONTENTS(dprintf("[M]: %s\n", line.utf8().data()));

				if (startsWithLettersIgnoringASCIICase(line, "#ext-x-stream-inf:"))
				{
					// reset
					info = { };

// #EXT-X-STREAM-INF:BANDWIDTH=5552610,CODECS="mp4a.40.2,avc1.4d402a",RESOLUTION=1920x1080,FRAME-RATE=60,VIDEO-RANGE=SDR,CLOSED-CAPTIONS=NONE

					String params = line.substringSharingImpl(18); // skip over #EXT-X-STREAM-INF:

					for (;;)
					{
						// find the next =
						size_t pos = params.find('=');
						if (WTF::notFound == pos)
							break;

						String key = params.substringSharingImpl(0, pos);
						String param;

						size_t epos = WTF::notFound;
						if (params.characterAt(pos + 1) == '\"')
						{
							epos = params.find('\"', pos + 2);
							if (WTF::notFound == epos)
								break;
							param = params.substring(pos+2, epos-(pos+2));
							epos++;
						}
						else
						{
							epos = params.find(',');
							if (WTF::notFound != epos)
								param = params.substring(pos+1, epos-(pos+1));
							else
								param = params.substring(pos+1);
						}
						DCONTENTS(dprintf("[M]: %s = %s\n", key.utf8().data(), param.utf8().data()));

						if (startsWithLettersIgnoringASCIICase(key, "resolution"))
						{
							auto res = param.convertToASCIILowercase().split('x');
							if (res.size() >= 2)
							{
								info.m_width = parseIntegerAllowingTrailingJunk<int>(res[0]).value_or(0);
								info.m_height = parseIntegerAllowingTrailingJunk<int>(res[1]).value_or(0);
							}
						}
						else if (startsWithLettersIgnoringASCIICase(key, "frame-rate"))
						{
							info.m_fps = parseIntegerAllowingTrailingJunk<int>(param).value_or(0);
						}
						else if (startsWithLettersIgnoringASCIICase(key, "codecs"))
						{
							info.m_codecs = param.split(',');
						}
						else if (startsWithLettersIgnoringASCIICase(key, "bandwidth"))
						{
							info.m_bandwidth = parseIntegerAllowingTrailingJunk<int>(param).value_or(0);
						}

						if (WTF::notFound == epos)
							break;
						// skip to next
						params = params.substring(epos+1);
					}

					// next line should be our link...
					hopingForM3U8 = true;
				}
				else if (hopingForM3U8)
				{
					if (line.contains("m3u8"))
					{
						info.m_url = URL(baseURL, line).string();
						D(dprintf("[M]: append stream bw %d res %dx%d fps %d '%s'\n", info.m_bandwidth, info.m_width, info.m_height, info.m_fps, info.m_url.utf8().data()));
						m_streams.append(info);
						info.m_url = "";
						hopingForM3U8 = false;
					}
				}
				else if (startsWithLettersIgnoringASCIICase(line, "#extinf:"))
				{
					foundChunks = true;
				}
			}
			
			if (info.m_url.length())
			{
				m_streams.append(info);
			}
			
			// this isn't master but a direct HLS link!
			if (m_streams.size() == 0 && foundChunks)
			{
				info.m_url = baseURL.string();
				m_streams.append(info);
				D(dprintf("%s: this is no master HLS %s\n", __PRETTY_FUNCTION__, baseURL.string().utf8().data()));
			}
		}
	}
	
	Vector<HLSStreamInfo> &streams() { return m_streams; }

protected:
	Vector<HLSStreamInfo> m_streams;
};

HLSStream::HLSStream(const URL &baseURL, const String &sdata)
{
	Vector<String> lines = String(sdata).replace(rnReplace, rnReplacement).split('\n');

	if (lines.size() >= 2 && equalIgnoringASCIICase(lines[0], "#EXTM3U"))
	{
		String keyURL;
		unsigned char iv[16];
		bool hopingForURL = false;
		bool hasIV = false;
		double duration = 0.f;
		double programTimeDate = 0;
		m_mediaSequence = -1;

		for (size_t i = 1; i < lines.size(); i++)
		{
			auto &line = lines[i];

			DCONTENTS(dprintf("[P]: %s\n", line.utf8().data()));

			if (startsWithLettersIgnoringASCIICase(line, "#ext-x-media-sequence"))
			{
				m_mediaSequence = parseIntegerAllowingTrailingJunk<uint64_t>(line.substring(22)).value_or(0);
				D(dprintf("mediaseq: %llu\n", m_mediaSequence));
				m_mediaSequence--; //! we want 1st added chunk to have the right sequence!
			}
			// #EXT-X-TARGETDURATION:1
			else if (startsWithLettersIgnoringASCIICase(line, "#ext-x-targetduration:"))
			{
				m_targetDuration = line.substring(22).toDouble();
				duration = m_targetDuration;
			}
			else if (startsWithLettersIgnoringASCIICase(line, "#ext-x-program-date-time"))
			{
				bool local;
				auto time = WTF::parseES5DateFromNullTerminatedCharacters(line.substring(25).utf8().data(), local);
				if (time == time)
				{
					programTimeDate = time / 1000.0;
					m_initialTimeStamp = programTimeDate;
				}
			}
			else if (startsWithLettersIgnoringASCIICase(line, "#extinf:"))
			{
				duration = line.substring(8).toDouble();
				if (duration <= 0.0)
					duration = m_targetDuration;
				hopingForURL = true;
			}
			else if (hopingForURL && !startsWithLettersIgnoringASCIICase(line,"#"))
			{
				HLSChunk chunk;
				chunk.m_mediaSequence = ++m_mediaSequence;
				chunk.m_duration = duration;
				chunk.m_encryption.m_hasIV = hasIV;
				if (hasIV)
					memcpy(&chunk.m_encryption.m_iv[0], &iv[0], sizeof(chunk.m_encryption.m_iv));
//				chunk.m_programDateTime = programTimeDate;
//				programTimeDate += duration;
				m_remainingDuration += duration;

				chunk.m_url = URL(baseURL, line).string();
				if (keyURL.length() > 0)
					chunk.m_encryption.m_keyURL = URL(baseURL, keyURL).string();
				m_chunks.emplace(WTFMove(chunk));
				
				duration = m_targetDuration; // reset
			}
			// #EXT-X-KEY:METHOD=AES-128,URI="keys/1.key",IV=0xf1dd959d87ccb58a9bf47ebd8bb73e24
			else if (startsWithLettersIgnoringASCIICase(line, "#ext-x-key"))
			{
				size_t encryption = line.findIgnoringASCIICase("METHOD=AES-128");
				if (WTF::notFound != encryption)
				{
					size_t uri = line.findIgnoringASCIICase("URI=\"");
					if (WTF::notFound != uri)
					{
						size_t uriEnd = line.findIgnoringASCIICase("\"", uri + 6);
						if (WTF::notFound != uri)
						{
							keyURL = line.substring(uri + 5, uriEnd - (uri + 5));
							DCONTENTS(dprintf("[P]: Encryption key URI '%s'\n", keyURL.utf8().data()));
							m_encrypted = true;
						}
					}
					
					size_t ivpos = line.findIgnoringASCIICase("IV=");
					if (WTF::notFound != ivpos)
					{
						String ivs = line.substring(ivpos + 3);
						if (WTF::notFound != ivs.find(","))
						{
							ivs = ivs.substring(0,ivs.find(",") - 1);
						}

						if (startsWithLettersIgnoringASCIICase(ivs, "0x"))
						{
							ivs = ivs.substring(2);
							auto ascii = ivs.ascii();
							const char *c = ascii.data();
							if (c && 32 == strlen(c))
							{
								for (int i = 0; i < 16; i++)
								{
									char x[3];
									x[0] = *c++;
									x[1] = *c++;
									x[2] = 0;
									iv[i] = strtoul(x, NULL, 16);
								}
								hasIV = true;
							}
						}
					}
				}
				else if (line.findIgnoringASCIICase("METHOD=NONE") == WTF::notFound)
				{
					DCONTENTS(dprintf("[P]: unknown encryption, bailing out!\n"));
					return;
				}
			}
			else if (startsWithLettersIgnoringASCIICase(line, "#ext-x-endlist"))
			{
				m_ended = true;
				break; // no chunks must follow this!
			}
			else if (startsWithLettersIgnoringASCIICase(line, "#ext-x-map:"))
			{
				// #ext-x-map:URI="url",BYTERANGE=length[@offset]
				size_t urlPos = line.findIgnoringASCIICase("URI=", 11);
				if (WTF::notFound != urlPos)
				{
					int quotes = line.characterStartingAt(urlPos + 4) == '\"' ? 1 : 0;

					String url = line.substring(urlPos + 4 + quotes);
					if (quotes)
					{
						url = url.substring(0, url.find("\"", 1));
					}
					else
					{
						size_t comma = url.find(",");
						if (WTF::notFound != comma)
							url = url.substring(0, comma);
					}
					
					m_map = HLSMap(URL(baseURL, url).string());
					D(dprintf("EXTMAP: %s\n", m_map.m_url.utf8().data()));
				}
			}
			// #EXT-X-PROGRAM-DATE-TIME:2021-05-05T15:44:32.030+00:00
			// dprintf("parse l '%s' (got %d)\n", line.utf8().data(), m_chunks.size());
		}
		
		m_initialMediaSequence = m_mediaSequence;
	}
}

HLSStream& HLSStream::operator+=(HLSStream& append)
{
	while (!append.m_chunks.empty())
	{
		if (append.m_chunks.front().m_mediaSequence > m_mediaSequence || (m_chunks.empty() && m_mediaSequence == -1))
		{
			m_mediaSequence = append.m_chunks.front().m_mediaSequence;
			m_remainingDuration += append.m_chunks.front().m_duration;
			m_chunks.emplace(append.m_chunks.front());
		}

		append.m_chunks.pop();
	}

	m_ended = append.m_ended;

	// this would happen on initial append
	if (-1 == m_initialMediaSequence)
	{
		m_initialMediaSequence = append.m_initialMediaSequence;
		m_targetDuration = append.m_targetDuration;
		m_initialTimeStamp = append.m_initialTimeStamp;
		m_map = append.m_map;
		m_encrypted = append.m_encrypted;
	}

	return *this;
}

void HLSStream::assign(const HLSStream& source)
{
	m_initialMediaSequence = source.m_initialMediaSequence;
	m_targetDuration = source.m_targetDuration;
	m_initialTimeStamp = source.m_initialTimeStamp;
	m_map = source.m_map;
	m_chunks = source.m_chunks;
	m_ended = source.m_ended;
	m_remainingDuration = source.m_remainingDuration;
	m_encrypted = source.m_encrypted;
}

void HLSStream::clear()
{
	while (!empty())
		pop();
	m_mediaSequence = -1;
	m_map = HLSMap();
	m_remainingDuration = 0;
}

void HLSStream::popUntil(double position)
{
	while (position > 0 && !empty())
	{
		double duration = m_chunks.front().m_duration;
		position -= duration;
		m_initialTimeStamp += duration;
		pop();
	}
}

AcinerellaNetworkBufferHLS::AcinerellaNetworkBufferHLS(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead)
	: AcinerellaNetworkBuffer(resourceProvider, url, readAhead)
	, m_playlistRefreshTimer(RunLoop::current(), this, &AcinerellaNetworkBufferHLS::refreshTimerFired)
	, m_baseURL({}, url)
{
	D(dprintf("%s(%p) - url %s\n", __func__, this, url.utf8().data()));
}

AcinerellaNetworkBufferHLS::~AcinerellaNetworkBufferHLS()
{
	D(dprintf("%s(%p) \n", __func__, this));
	stop();
	if (m_chunkRequestInRead)
		m_chunkRequestInRead->die();
}

// main thread
void AcinerellaNetworkBufferHLS::start(uint64_t from)
{
	D(dprintf("%s(%p) - from %llu\n", __func__, this, from));
	(void)from;

	if (!m_hasMasterList)
	{
		m_stopping = false;
		m_stream.clear();
		m_hasMasterList = false;

		m_hlsRequest = AcinerellaNetworkFileRequest::create(m_url, [this, protect = makeRef(*this)](bool succ) { masterPlaylistReceived(succ); });
	}
	else
	{
		D(dprintf("%s(%p) - restart?!?\n", __func__, this));
	}
}

// main thread
void AcinerellaNetworkBufferHLS::stop()
{
	D(dprintf("%s(%p) \n", __func__, this));
	m_stopping = true;

	m_playlistRefreshTimer.stop();

	if (m_hlsRequest)
		m_hlsRequest->cancel();
	m_hlsRequest = nullptr;
	
	RefPtr<AcinerellaNetworkBuffer> chunkRequest;
	{
		auto lock = Locker(m_lock);
		std::swap(m_chunkRequest, chunkRequest);
	}

	if (chunkRequest)
		chunkRequest->stop();

	{
		auto lock = Locker(m_lock);
		
		if (m_chunkRequestInRead)
			m_chunkRequestInRead->die();
	}

	m_event.signal();

	D(dprintf("%s(%p) killing old chunks\n", __func__, this));
	auto lock = Locker(m_lock);
	while (!m_chunksRequestPreviouslyRead.empty())
	{
		m_chunksRequestPreviouslyRead.front()->die();
		m_chunksRequestPreviouslyRead.pop();
	}
}

// main thread
void AcinerellaNetworkBufferHLS::masterPlaylistReceived(bool succ)
{
	D(dprintf("%s(%p) \n", __func__, this));
	if (succ && m_hlsRequest && !m_stopping)
	{
		auto buffer = m_hlsRequest->buffer();
		m_hlsRequest->cancel();

		if (buffer && buffer->size())
		{
			HLSMasterPlaylistParser parser(m_baseURL, String::fromUTF8(buffer->data(), buffer->size()));

			m_hasMasterList = true;
			m_streams = WTFMove(parser.streams());

			if (m_streams.size())
			{
				m_selectedStream.m_url = emptyString();
				m_provider->selectStream();
				D(dprintf("%s(%p): selected %dx%d %s \n", __func__, this, m_selectedStream.m_width, m_selectedStream.m_height, m_selectedStream.m_url.utf8().data()));

				// no stream selected?
				if (m_selectedStream.m_url.isEmpty())
				{
					// break out of read()
					m_stopping = true;
					m_event.signal();
					m_hlsRequest = nullptr;
					return;
				}

				m_hlsRequest = AcinerellaNetworkFileRequest::create(m_selectedStream.m_url, [this, protect = makeRef(*this)](bool succ) { childPlaylistReceived(succ); });
				return;
			}
		}

		m_hlsRequest = nullptr;
	}
	
	if (m_hlsRequest)
		m_hlsRequest->cancel();
	m_hlsRequest = nullptr;
}

void AcinerellaNetworkBufferHLS::selectStream(const HLSStreamInfo &stream, bool reload, double position)
{
	D(dprintf("%s(%p) \n", __func__, this));
	m_selectedStream = stream;
	
	if (reload)
	{
		if (m_terminatedHLS)
		{
			skip(position);
		}
		else
		{
			while (!m_stream.empty())
				m_stream.clear();
			refreshTimerFired();
		}
	}
}

// main thread
void AcinerellaNetworkBufferHLS::childPlaylistReceived(bool succ)
{
	D(dprintf("%s(%p) \n", __func__, this));
	if (succ && m_hlsRequest && !m_stopping)
	{
		auto buffer = m_hlsRequest->buffer();

		if (buffer && buffer->size())
		{
			auto contents = String::fromUTF8(buffer->data(), buffer->size());
			bool initial = m_stream.empty();
			HLSStream stream(URL({}, m_selectedStream.m_url), contents);
			m_stream += stream; // append and merge :)
			m_isEncrypted = m_stream.isEncrypted();
			
			if (initial)
			{
				m_terminatedHLS = m_stream.ended();
				if (m_terminatedHLS)
				{
					m_terminatedStream.assign(m_stream);
					m_fullDuration = m_terminatedStream.remainingDuration();
					D(dprintf("%s(%p) terminated, duration %f chunks %d (%d)\n", __func__, this, float(m_fullDuration), m_terminatedStream.size(), m_stream.size()));
				}
				else
				{
					// this is live HLS; since these may have a history leading to 'now', let's skip all but the last 10 chunks
					D(dprintf("%s(%p) live! skip %d chunks\n", __func__, this, std::max(0, int(m_stream.size()) - 10)));
					while (m_stream.size() > 10)
						m_stream.pop();
				}
			}

			D(dprintf("%s(%p) queue %d mediaseq %llu %d cr %p\n", __func__, this, m_stream.size(), m_stream.mediaSequence(), m_stream.empty(), m_chunkRequest.get()));
			// start loading chunks!
			if (!m_stream.empty() && !m_chunkRequest)
			{
				if (initial && 0 != m_stream.map().m_url.length())
				{
					D(dprintf("%s(%p) initial %d, map url %s\n", __func__, this, initial, m_stream.map().m_url.utf8().data()));
					m_initializationChunkRequest = AcinerellaNetworkFileRequest::create(m_stream.map().m_url, [this, protect = makeRef(*this)](bool succ) { initializationSegmentReceived(succ); });
				}
				else
				{
					bool shouldRequest = false;
					{
						auto lock = Locker(m_lock);
						if (!m_chunkRequestInRead)
							shouldRequest = true;
					}
					if (shouldRequest)
						requestNextChunk();
				}
			}
		}
	}

	double duration = m_stream.targetDuration();
	if (duration <= 1.0 && !m_stream.empty())
		duration = std::min(m_stream.current().m_duration, duration);
	if (duration <= 1.0)
		duration = 3.0;
	duration *= 0.5;
	
	if (!m_stream.ended())
	{
		D(dprintf("%s(%p): refresh in %f \n", __func__, this, duration));
		m_playlistRefreshTimer.startOneShot(Seconds(duration));
	}
	else
	{
		D(dprintf("%s(%p): --! stream ended ! \n", __func__, this));
	}
	
	if (m_hlsRequest)
		m_hlsRequest->cancel();
	m_hlsRequest = nullptr;
}

void AcinerellaNetworkBufferHLS::requestNextChunk()
{
	if (!m_stream.empty())
	{
		if (!encryptionKeyNeeded(m_stream.current()))
		{
			RefPtr<AcinerellaNetworkBuffer> chunkRequest;
			RefPtr<SharedBuffer> key = encryptionKey(m_stream.current());

			D(dprintf("%s(%p) chunk '%s' duration %f encrypted? %d\n", __func__, this, m_stream.current().m_url.utf8().data(), m_stream.current().m_duration, !!key));
			
			if (!!key)
				chunkRequest = AcinerellaNetworkBuffer::createPreloadingEncryptedBuffer(m_provider, m_stream.current().m_url, key, m_stream.current().m_encryption.m_hasIV ? &m_stream.current().m_encryption.m_iv[0] : nullptr, m_initializationChunkRequest ? m_initializationChunkRequest->buffer() : nullptr);
			else
				chunkRequest = AcinerellaNetworkBuffer::createPreloadingBuffer(m_provider, m_stream.current().m_url, m_initializationChunkRequest ? m_initializationChunkRequest->buffer() : nullptr);

			m_stream.pop();

			chunkRequest->start();

			{
				auto lock = Locker(m_lock);
				m_chunkRequest = chunkRequest;
			}

			// wake up the ::read
			m_event.signal();
		}
	}
}

bool AcinerellaNetworkBufferHLS::encryptionKeyNeeded(const HLSChunk& chunk)
{
	if (!chunk.m_encryption.m_keyURL.isEmpty())
	{
		DENC(dprintf("%s(%p) key needed %s requested/cached? %d kc %d\n", __func__, this, chunk.m_encryption.m_keyURL.utf8().data(), m_keys.find(chunk.m_encryption.m_keyURL) != m_keys.end(), m_keys.size()));
		auto it = m_keys.find(chunk.m_encryption.m_keyURL);

		if (it == m_keys.end())
		{
			DENC(dprintf("%s(%p) requesting a key...\n", "encryptionKeyNeeded", this));
			m_keys.emplace(std::make_pair(chunk.m_encryption.m_keyURL, AcinerellaNetworkFileRequest::create(chunk.m_encryption.m_keyURL, [this, protect = makeRef(*this), url = chunk.m_encryption.m_keyURL](bool) {

				requestNextChunk();
				DENC(dprintf("%s(%p) key obtained\n", "encryptionKeyNeeded", this));
			})));
			
			return true;
		}

		// still waiting for key!
		return !it->second->buffer();
	}
	
	return false;
}

RefPtr<SharedBuffer> AcinerellaNetworkBufferHLS::encryptionKey(const HLSChunk& chunk)
{
	if (!chunk.m_encryption.m_keyURL.isEmpty())
	{
		auto it = m_keys.find(chunk.m_encryption.m_keyURL);
		if (m_keys.end() != it)
			return it->second->buffer();
	}
	return nullptr;
}

void AcinerellaNetworkBufferHLS::initializationSegmentReceived(bool succ)
{
	if (succ)
	{
		if (!m_stream.empty() && !m_chunkRequest)
		{
			requestNextChunk();
		}
	}
}

// main thread
void AcinerellaNetworkBufferHLS::refreshTimerFired()
{
	D(dprintf("%s(%p) \n", __func__, this));
	if (m_hlsRequest)
		m_hlsRequest->cancel();
	m_hlsRequest = AcinerellaNetworkFileRequest::create(m_selectedStream.m_url, [this, protect = makeRef(*this)](bool succ) { childPlaylistReceived(succ); });
}

// main thread
void AcinerellaNetworkBufferHLS::chunkSwallowed()
{
	D(dprintf("%s(%p): cr %p streams %d\n", __func__, this, m_chunkRequest.get(), m_stream.size()));

	{
		auto lock = Locker(m_lock);
		while (!m_chunksRequestPreviouslyRead.empty())
		{
			m_chunksRequestPreviouslyRead.front()->stop();
			m_chunksRequestPreviouslyRead.pop();
		}
	}

	if (!m_stream.empty())
	{
		requestNextChunk();
	}
}

int64_t AcinerellaNetworkBufferHLS::length()
{
	auto lock = Locker(m_lock);
	DIO(dprintf("%s(%p): %lld\n", __PRETTY_FUNCTION__, this, m_chunkRequestInRead ? m_chunkRequestInRead->length() : 0));
	if (m_chunkRequestInRead)
		return m_chunkRequestInRead->length();
	return 0;
}

int64_t AcinerellaNetworkBufferHLS::position()
{
	auto lock = Locker(m_lock);
	DIO(dprintf("%s(%p): %lld\n", __PRETTY_FUNCTION__, this, m_chunkRequestInRead ? m_chunkRequestInRead->position() : 0));
	if (m_chunkRequestInRead)
		return m_chunkRequestInRead->position();
	return 0;
}

// acinerella decoder thread
int AcinerellaNetworkBufferHLS::read(uint8_t *outBuffer, int size, int64_t readPosition)
{
	DIO(dprintf("%s(%p):>> requested %ld inread %p rpos %lld\n", __PRETTY_FUNCTION__, this, size, m_chunkRequestInRead.get(), readPosition));

	while (!m_stopping)
	{
		bool needsToWait = false;
		bool signalChunkSwallowed = false;

		{
			auto lock = Locker(m_lock);

			if (!m_chunkRequestInRead)
			{
				m_chunkRequestInRead = m_chunkRequest;
				m_chunkRequest = nullptr;
				needsToWait = m_chunkRequestInRead.get() == nullptr;
				signalChunkSwallowed = !needsToWait;
				DIO(dprintf("%s(%p): chunk swapped to %p needswait %d\n", __PRETTY_FUNCTION__, this, m_chunkRequestInRead.get(), needsToWait));
			}
		}

		if (m_skipping)
		{
			DIO(dprintf("%s(%p): skipping, %d\n", __PRETTY_FUNCTION__, this, m_initializationPending));

			{
				auto lock = Locker(m_lock);
				m_skipping = false;
				if (m_chunkRequestInRead)
				{
					m_chunksRequestPreviouslyRead.emplace(m_chunkRequestInRead);
					m_chunkRequestInRead = nullptr;
				}
			}
			
			WTF::callOnMainThread([this, protect = makeRef(*this)]() {
				chunkSwallowed();
			});
		}

		if (m_chunkRequestInRead)
		{
			int read = m_chunkRequestInRead->read(outBuffer, size, readPosition);

			if (signalChunkSwallowed)
			{
				WTF::callOnMainThread([this, protect = makeRef(*this)]() {
					chunkSwallowed();
				});
			}

			if (m_initializationPending && 0 == read)
			{
				DIO(dprintf("-- eRead_EOFWhileInitializing!!\n"));
				return eRead_EOFWhileInitializing;
			}
			else if (0 == read)
			{
				bool ended = false;
				{
					auto lock = Locker(m_lock);
					ended = m_stream.empty() && m_stream.ended();
					m_chunksRequestPreviouslyRead.emplace(m_chunkRequestInRead);
					m_chunkRequestInRead = nullptr;
					DIO(dprintf("%s(%p): discontinuity, ended %d \n", __PRETTY_FUNCTION__, this, ended));
				}

				return ended ? eRead_EOF : eRead_Discontinuity;
			}
			else
			{
				DIO(dprintf("%s(%p):<< read %d from chunk %p bytesleft %lld\n", __PRETTY_FUNCTION__, this, read, m_chunkRequestInRead.get(),
					m_chunkRequestInRead->length() - m_chunkRequestInRead->position()));
				return read;
			}
		}

		if (m_stopping)
			return eRead_EOF;

		if (needsToWait)
		{
			m_event.waitFor(10_s);
		}
	}
	
	if (m_stopping && m_chunkRequestInRead)
	{
		auto lock = Locker(m_lock);
		m_chunksRequestPreviouslyRead.emplace(m_chunkRequestInRead);
		m_chunkRequestInRead = nullptr;
	}

	DIO(dprintf("%s(%p): failing out (%d)\n", __PRETTY_FUNCTION__, this, m_stopping));
	return -1;
}

bool AcinerellaNetworkBufferHLS::markLastFrameRead()
{
	DIO(dprintf("%s(%p): \n", __PRETTY_FUNCTION__, this));
	bool doSwallow = false;
	bool hasMore = true;

	{
		auto lock = Locker(m_lock);
		if (m_chunkRequestInRead)
		{
			m_chunksRequestPreviouslyRead.emplace(m_chunkRequestInRead);
			m_chunkRequestInRead = nullptr;
			doSwallow = true;
		}

		if (m_stream.ended() && m_stream.empty())
			hasMore = false;
	}
	
	if (doSwallow)
	{
		WTF::callOnMainThread([this, protect = makeRef(*this)]() {
			chunkSwallowed();
		});
	}
	
	return hasMore;
}

void AcinerellaNetworkBufferHLS::skip(double startTime)
{
	auto lock = Locker(m_lock);
	RefPtr<AcinerellaNetworkBuffer> chunkRequest;
	std::swap(m_chunkRequest, chunkRequest);
	m_stream.clear();
	m_stream.assign(m_terminatedStream);
	D(double duration = m_stream.remainingDuration());
	D(size_t size = m_stream.size());
	m_stream.popUntil(startTime);
	D(dprintf("%s(%p): skipped, next HLS chunk @ %lu (%lu), duration %f > %f, its %f\n", __PRETTY_FUNCTION__, this, size, m_stream.size(), float(duration), float(m_stream.remainingDuration()), float(m_stream.initialTimeStamp())));
	m_skipping = true;
	if (chunkRequest)
		chunkRequest->stop();
	m_event.signal();
}

double AcinerellaNetworkBufferHLS::initialTimeStamp() const
{
	if (!m_terminatedHLS)
		return 0;
	return m_stream.initialTimeStamp();
}

}
}

#undef D
#endif
