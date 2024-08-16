#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include "AcinerellaBuffer.h"
#include <wtf/RunLoop.h>
#include <wtf/URL.h>

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

struct HLSChunk
{
	String    m_url;
	int64_t   m_mediaSequence;
	double    m_duration;
};

class HLSStream
{
public:
	HLSStream() = default;
	HLSStream(const URL &baseURL, const String &data);
	HLSStream& operator+=(HLSStream& append);

	const HLSChunk &current() const { static HLSChunk empty; return m_chunks.empty() ? empty: m_chunks.front(); }
	void pop() { if (!m_chunks.empty()) m_chunks.pop(); }
	bool ended() const { return m_ended; }
	bool empty() const { return m_chunks.empty(); }
	size_t size() const { return m_chunks.size(); }
	void clear() { while (!empty()) pop(); m_mediaSequence = -1; }
	double targetDuration() const { return m_targetDuration; }
	int64_t mediaSequence() const { return m_mediaSequence; }
	int64_t initialMediaSequence() const { return m_initialMediaSequence; }
	double initialTimeStamp() const;

protected:
	std::queue<HLSChunk> m_chunks;
	int64_t              m_mediaSequence = -1;
	int64_t              m_initialMediaSequence = -1;
	double               m_targetDuration = 0;
	bool                 m_ended = false;
};

class AcinerellaNetworkBufferHLS : public AcinerellaNetworkBuffer
{
public:
	AcinerellaNetworkBufferHLS(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead);
	virtual ~AcinerellaNetworkBufferHLS();

	void start(uint64_t from = 0) override;
	void stop() override;

	bool canSeek() { return false; }

	int read(uint8_t *outBuffer, int size, int64_t ignore) override;

	int64_t length() override;
	int64_t position() override;

	const Vector<HLSStreamInfo>& streams() const { return m_streams; }
	void selectStream(const HLSStreamInfo &stream);
	bool hasStreamSelection() const override { return true; }

	double initialTimeStamp() const override { return m_stream.initialTimeStamp(); }

protected:

	void masterPlaylistReceived(bool succ);
	void childPlaylistReceived(bool succ);

	void refreshTimerFired();
	void chunkSwallowed();

protected:
	RunLoop::Timer<AcinerellaNetworkBufferHLS>  m_playlistRefreshTimer;
	RefPtr<AcinerellaNetworkBuffer>             m_chunkRequest;
	RefPtr<AcinerellaNetworkBuffer>             m_chunkRequestInRead;
	std::queue<RefPtr<AcinerellaNetworkBuffer>> m_chunksRequestPreviouslyRead;
	RefPtr<AcinerellaNetworkFileRequest>        m_hlsRequest;
	Vector<HLSStreamInfo>                       m_streams;
	HLSStreamInfo                               m_selectedStream;
	BinarySemaphore                             m_event;
	HLSStream                                   m_stream;
	Lock                                        m_lock;
	URL                                         m_baseURL;
	bool                                        m_hasMasterList = false;
	bool                                        m_stopping = false;
	bool                                        m_ended = false;
};

}
}

#endif
