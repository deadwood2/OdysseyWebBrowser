#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/WTFString.h>
#include <wtf/Function.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/threads/BinarySemaphore.h>
#include "SharedBuffer.h"
#include "acinerella.h"
#include "AcinerellaPointer.h"
#include <queue>

namespace WebCore {

class PlatformMediaResourceLoader;

namespace Acinerella {

class AcinerellaMuxedBuffer;
class AcinerellaDecoder;

class AcinerellaNetworkBufferResourceLoaderProvider
{
public:
    virtual void ref() = 0;
    virtual void deref() = 0;
	virtual RefPtr<PlatformMediaResourceLoader> createResourceLoader() = 0;
	virtual String referrer() = 0;
	virtual void selectStream() = 0;
};

class AcinerellaNetworkBuffer : public ThreadSafeRefCounted<AcinerellaNetworkBuffer>
{
protected:
	AcinerellaNetworkBuffer(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead);
public:
	virtual ~AcinerellaNetworkBuffer();

	static RefPtr<AcinerellaNetworkBuffer> create(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead = 1 * 1024 * 1024);
	static RefPtr<AcinerellaNetworkBuffer> createDisregardingFileType(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead = 1 * 1024 * 1024);
	static RefPtr<AcinerellaNetworkBuffer> createPreloadingBuffer(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, RefPtr<SharedBuffer> prepend = nullptr);
	static RefPtr<AcinerellaNetworkBuffer> createPreloadingEncryptedBuffer(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, RefPtr<SharedBuffer> key, const unsigned char *iv, RefPtr<SharedBuffer> prepend = nullptr);

	const String &url() const { return m_url; }

	// Main Thread Methods
	virtual void start(uint64_t from = 0) = 0;
	virtual void stop() = 0;
	virtual bool canSeek() { return true; }
	virtual bool canSkip() { return false; }
	void die();

	virtual void skip(double startTime) { (void)startTime; }
	virtual bool knowsDuration() { return false; }
	virtual double duration() { return 0.0; }

	virtual bool hasStreamSelection() const { return false; }
	virtual double initialTimeStamp() const { return .0; }

	virtual void setIsInitializing(bool) { };
	
	virtual bool markLastFrameRead() { return true; };
	
	virtual bool isEncrypted() const { return false; }

	// Acinerella Thread Methods
	static const int eRead_EOFWhileInitializing = -3;
	static const int eRead_Discontinuity = -2;
	static const int eRead_Error = -1;
	static const int eRead_EOF = 0;
	
	virtual int read(uint8_t *outBuffer, int size, int64_t position = -1) = 0;

	virtual int64_t length() { return m_length; }
	virtual int64_t position() { return 0; }

protected:
	AcinerellaNetworkBufferResourceLoaderProvider *m_provider;
	String                                         m_url;
    int64_t                                        m_length;
    int                                            m_readAhead;
    bool                                           m_dead = false;
};

class AcinerellaNetworkFileRequest : public ThreadSafeRefCounted<AcinerellaNetworkFileRequest>
{
protected:
	AcinerellaNetworkFileRequest(const String &url, Function<void(bool)>&& onFinished)
		: m_url(url)
		, m_onFinished(WTFMove(onFinished))
	{
	}
	AcinerellaNetworkFileRequest(const String &url, Function<void(RefPtr<SharedBuffer>)>&& onFinished)
		: m_url(url)
		, m_onFinished2(WTFMove(onFinished))
	{
	}
public:
	static RefPtr<AcinerellaNetworkFileRequest> create(const String &url, Function<void(bool)>&& onFinished);
	static RefPtr<AcinerellaNetworkFileRequest> createRefResult(const String &url, Function<void(RefPtr<SharedBuffer>)>&& onFinished);
	virtual ~AcinerellaNetworkFileRequest() = default;
	
	virtual RefPtr<SharedBuffer> buffer() = 0;
	virtual void cancel() = 0;

protected:
	String               m_url;
	Function<void(bool)> m_onFinished;
	Function<void(RefPtr<SharedBuffer>)> m_onFinished2;
};

}
}

#endif
