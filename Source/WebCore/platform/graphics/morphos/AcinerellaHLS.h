#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include "AcinerellaBuffer.h"
#include <wtf/RunLoop.h>
#include <wtf/URL.h>
#include <wtf/StdMap.h>

namespace WebCore {

class Page;

namespace Acinerella {

struct HLSStreamInfo
{
	String         m_url;
	Vector<String> m_codecs;
	int            m_bandwidth = 0;
	int            m_fps = 30;
	int            m_width = 0;
	int            m_height = 0;
};

struct HLSEncryption
{
	String        m_keyURL;
	bool          m_hasIV;
	unsigned char m_iv[16];
};

struct HLSChunk
{
	String        m_url;
	int64_t       m_mediaSequence;
	double        m_duration;
	double        m_programDateTime;
	HLSEncryption m_encryption;
};

struct HLSMap
{
	String m_url;
	int    m_length = 0;
	int    m_offset = 0;
	
	HLSMap() { }
	HLSMap(const String& url) : m_url(url) { }
};

class HLSStream
{
public:
	HLSStream() = default;
	HLSStream(const URL &baseURL, const String &data);
	HLSStream& operator+=(HLSStream& append); // caution: destructive
	void assign(const HLSStream& source);

	const HLSChunk &current() const { static HLSChunk empty; return m_chunks.empty() ? empty: m_chunks.front(); }
	void pop() { if (!m_chunks.empty()) m_remainingDuration -= m_chunks.front().m_duration; m_chunks.pop(); }
	void popUntil(double position);
	bool ended() const { return m_ended; }
	bool empty() const { return m_chunks.empty(); }
	size_t size() const { return m_chunks.size(); }
	void clear();
	double targetDuration() const { return m_targetDuration; }
	int64_t mediaSequence() const { return m_mediaSequence; }
	int64_t initialMediaSequence() const { return m_initialMediaSequence; }
	double initialTimeStamp() const { return m_initialTimeStamp; }
	double remainingDuration() const { return m_remainingDuration; }
	const HLSMap &map() const { return m_map; }
	bool isEncrypted() const { return m_encrypted; }

protected:
	HLSMap               m_map;
	std::queue<HLSChunk> m_chunks;
	int64_t              m_mediaSequence = -1;
	int64_t              m_initialMediaSequence = -1;
	double               m_targetDuration = 0;
	double               m_initialTimeStamp = 0;
	double               m_remainingDuration = 0;
	bool                 m_ended = false;
	bool                 m_encrypted = false;
};

class AcinerellaNetworkBufferHLS : public AcinerellaNetworkBuffer
{
public:
	AcinerellaNetworkBufferHLS(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead);
	virtual ~AcinerellaNetworkBufferHLS();

	void start(uint64_t from = 0) override;
	void stop() override;

	bool canSeek() override { return m_terminatedHLS; }
	bool canSkip() override { return m_terminatedHLS; }
	void skip(double startTime) override;
	
	bool knowsDuration() override { return canSkip(); }
	double duration() override { return m_fullDuration; }

	int read(uint8_t *outBuffer, int size, int64_t ignore) override;

	int64_t length() override;
	int64_t position() override;

	const Vector<HLSStreamInfo>& streams() const { return m_streams; }
	void selectStream(const HLSStreamInfo &stream, bool reload = false, double skipPosition = 0.0);
	const HLSStreamInfo& selectedStream() const { return m_selectedStream; }
	bool hasStreamSelection() const override { return true; }

	double initialTimeStamp() const override;
	void setIsInitializing(bool initializing) override { m_initializationPending = initializing; };
	bool markLastFrameRead() override;

	bool isEncrypted() const override { return m_isEncrypted; }

protected:

	void masterPlaylistReceived(bool succ);
	void childPlaylistReceived(bool succ);
	void initializationSegmentReceived(bool succ);

	void refreshTimerFired();
	void chunkSwallowed();
	
	bool encryptionKeyNeeded(const HLSChunk& chunk);
	RefPtr<SharedBuffer> encryptionKey(const HLSChunk& chunk);
	void requestNextChunk();

protected:
	RunLoop::Timer<AcinerellaNetworkBufferHLS>  m_playlistRefreshTimer;
	RefPtr<AcinerellaNetworkBuffer>             m_chunkRequest;
	RefPtr<AcinerellaNetworkBuffer>             m_chunkRequestInRead;
	std::queue<RefPtr<AcinerellaNetworkBuffer>> m_chunksRequestPreviouslyRead;
	RefPtr<AcinerellaNetworkFileRequest>        m_hlsRequest;
	RefPtr<AcinerellaNetworkFileRequest>        m_initializationChunkRequest;
	Vector<HLSStreamInfo>                       m_streams;
	HLSStreamInfo                               m_selectedStream;
	BinarySemaphore                             m_event;
	HLSStream                                   m_stream;
	HLSStream                                   m_terminatedStream;
	Lock                                        m_lock;
	URL                                         m_baseURL;
	bool                                        m_hasMasterList = false;
	bool                                        m_stopping = false;
	bool                                        m_terminatedHLS = false;
	double                                      m_fullDuration = 0;
	std::atomic<bool>                           m_skipping = false;
	bool                                        m_initializationPending = false;
	bool                                        m_isEncrypted = false;

	struct StringKeyHash {
		std::size_t operator()(const String& k) const {
			return k.hash();
		}
	};

 	struct StringKeyEquals {
		bool operator()(const String& a, const String& b) const {
			return a == b;
		}
	};
	std::unordered_map<String, RefPtr<AcinerellaNetworkFileRequest>, StringKeyHash, StringKeyEquals> m_keys;
};

}
}

#endif
