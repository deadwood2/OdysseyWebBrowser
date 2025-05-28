#include "AcinerellaBuffer.h"
#include "MediaPlayerMorphOS.h"

#if ENABLE(VIDEO)

#include "CurlRequest.h"
#include "NetworkingContext.h"
#include "NetworkStorageSession.h"
#include "SameSiteInfo.h"
#include "SynchronousLoaderClient.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "CurlRequestClient.h"
#include "PlatformMediaResourceLoader.h"
#include <queue>
#include "AcinerellaDecoder.h"
#include "AcinerellaHLS.h"

#if ENABLE(WEB_CRYPTO)

#include "CryptoAlgorithmAesCbcCfbParams.h"
#include "CryptoAlgorithmAesKeyParams.h"
#include "CryptoKeyAES.h"
#include "CryptoAlgorithmAES_CBC.h"

#endif

#define D(x)
#define DP(x) 

namespace WebCore {
namespace Acinerella {

class AcinerellaNetworkBufferInternal : public AcinerellaNetworkBuffer, public CurlRequestClient
{
public:
	AcinerellaNetworkBufferInternal(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead)
		: AcinerellaNetworkBuffer(resourceProvider, url, readAhead)
	{
	
	}
	
	~AcinerellaNetworkBufferInternal()
	{
		if (m_curlRequest)
		{
// dprintf("-- AcinerellaNetworkBufferInternal ORPHANED!!\n");
			m_curlRequest->cancel();
		}
	}

	void ref() override { ThreadSafeRefCounted<AcinerellaNetworkBuffer>::ref(); }
	void deref() override { ThreadSafeRefCounted<AcinerellaNetworkBuffer>::deref(); }

	void start(uint64_t from = 0) override
	{
		D(dprintf("%s(%p) - from %llu\n", __PRETTY_FUNCTION__, this, from));

		if (m_dead)
			return;

		if (m_curlRequest)
			m_curlRequest->cancel();
		m_curlRequest = nullptr;

		{
			auto lock = Locker(m_bufferLock);
			while (!m_buffer.empty())
				m_buffer.pop();

			m_finishedLoading = false;
			m_didFailLoading = false;

			m_bufferRead = 0;
			m_bufferPositionAbs = from;
			m_bufferSize = 0;
			m_redirectCount = 0;
			m_isPaused = false;
		}

		m_request = ResourceRequest(m_url);
		m_request.setCachePolicy(ResourceRequestCachePolicy::DoNotUseAnyCache);
		m_curlRequest = createCurlRequest(m_request);
		if (m_curlRequest)
		{
			m_curlRequest->setResumeOffset(static_cast<long long>(m_bufferPositionAbs));
			m_curlRequest->start();
		}
	}

	void stop() override
	{
		D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));

		if (m_curlRequest)
			m_curlRequest->cancel();
		m_curlRequest = nullptr;
		m_finishedLoading = true;
		m_isPaused = false;
		
		m_eventSemaphore.signal();
		D(dprintf("%s(%p) ..\n", __PRETTY_FUNCTION__, this));
	}
	
	int64_t position() override
	{
		return int64_t(m_bufferPositionAbs);
	}

	bool canSeek() override
	{
		return m_length > 0;
	}

	int read(uint8_t *outBuffer, int size, int64_t readPosition) override
	{
		D(dprintf("%s(%p): requested %ld from %lld (current %lld max %lld)\n", "nbRead", this, size, readPosition, m_bufferPositionAbs, m_length));
		int sizeWritten = 0;
		int sizeLeft = size;
		bool resume = false;

		if (-1 != readPosition && int64_t(m_bufferPositionAbs) != readPosition)
		{
			m_seekProcessed = false;

			D(dprintf("%s: seek to %llu\n", "nbRead", readPosition));

			WTF::callOnMainThread([this, seekTo = readPosition, protect = makeRef(*this)]() {
				start(seekTo);
				m_seekProcessed = true;
			});

			while (!m_dead && !m_seekProcessed)
			{
				m_eventSemaphore.waitFor(10_s);
			}

			D(dprintf("%s: seek to %llu processed...\n", "nbRead", m_bufferPositionAbs));
		}

		while (sizeWritten < size)
		{
			bool canReadMore = false;

			{
				auto lock = Locker(m_bufferLock);
				if (!m_buffer.empty())
				{
					auto buffer = m_buffer.front();
					int write = std::min(int(buffer->size() - m_bufferRead), sizeLeft);

					memcpy(outBuffer + sizeWritten, buffer->data() + m_bufferRead, write);
					m_bufferRead += write;
					m_bufferPositionAbs += write;
					sizeLeft -= write;
					sizeWritten += write;

					D(dprintf("%s: read %d from current block\n", "nbRead", write));

					if (m_bufferRead == int(buffer->size()))
					{
						m_buffer.pop();
						m_bufferSize -= m_bufferRead;
						m_bufferRead = 0;
						
						// Check if we don't need to resume reading...
						if (m_isPaused && m_bufferRead < (m_readAhead / 2))
						{
							resume = true;
						}
					}
					
					canReadMore = !m_buffer.empty();
				}
			}
			
			if (m_nonRecoverableErrors >= ms_maxNonRecoverableErrors)
			{
				return eRead_Error;
			}
			else if (sizeWritten < size && !m_finishedLoading && !canReadMore)
			{
				WTF::callOnMainThread([this, protect = makeRef(*this)]() {
					continueBuffering();
				});
				m_eventSemaphore.waitFor(10_s);
			}
			else if (m_finishedLoading && !canReadMore)
			{
				break;
			}
		}

		if (resume)
		{
			WTF::callOnMainThread([this, protect = makeRef(*this)]() {
				continueBuffering();
			});
		}

		D(dprintf("%s(%p): written %ld\n", "nbRead", this, sizeWritten));
		return sizeWritten;
	}
	
	void continueBuffering()
	{
		D(dprintf("%s(%p): pau %d underbuf %d fini %d\n", __PRETTY_FUNCTION__, this, m_isPaused, m_bufferRead < (m_readAhead / 2), m_finishedLoading));
		if (m_isPaused && m_bufferRead < (m_readAhead / 2) && !m_finishedLoading && !m_dead && m_nonRecoverableErrors < ms_maxNonRecoverableErrors)
		{
			if (m_curlRequest)
			{
				m_curlRequest->resume();
			}
			else
			{
				uint64_t abs;

				{
					auto lock = Locker(m_bufferLock);
					abs = m_bufferPositionAbs + m_bufferSize;
				}

				m_request = ResourceRequest(m_url);
				m_curlRequest = createCurlRequest(m_request);
				if (m_curlRequest)
				{
					m_curlRequest->setResumeOffset(static_cast<long long>(abs));
					m_curlRequest->start();
				}
			}
			D(dprintf("%s(%p): resuming...\n", __PRETTY_FUNCTION__, this));
			m_isPaused = false;
		}
	}
	
	Ref<CurlRequest> createCurlRequest(ResourceRequest&request)
	{
		auto context = MediaPlayerMorphOSSettings::settings().m_networkingContextForRequests;
		if (context)
		{
			auto& storageSession = *context->storageSession();
			auto includeSecureCookies = request.url().protocolIs("https") ? IncludeSecureCookies::Yes : IncludeSecureCookies::No;
			String cookieHeaderField = storageSession.cookieRequestHeaderFieldValue(request.firstPartyForCookies(), SameSiteInfo::create(request), request.url(), std::nullopt, std::nullopt, includeSecureCookies, ShouldAskITP::Yes, ShouldRelaxThirdPartyCookieBlocking::No).first;
			if (!cookieHeaderField.isEmpty())
				request.addHTTPHeaderField(HTTPHeaderName::Cookie, cookieHeaderField);
		}

		auto curlRequest = CurlRequest::create(WTFMove(request), *this);
		return curlRequest;
	}

	void curlDidSendData(CurlRequest&, unsigned long long, unsigned long long) override
	{
		D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
	}

	inline bool shouldRedirectAsGET(const ResourceRequest& request, bool crossOrigin)
	{
		if ((request.httpMethod() == "GET") || (request.httpMethod() == "HEAD"))
			return false;

		if (!request.url().protocolIsInHTTPFamily())
			return true;

		if (m_response.isSeeOther())
			return true;

		if ((m_response.isMovedPermanently() || m_response.isFound()) && (request.httpMethod() == "POST"))
			return true;

		if (crossOrigin && (request.httpMethod() == "DELETE"))
			return true;

		return false;
	}
	
	void curlDidReceiveResponse(CurlRequest& request, CurlResponse&& response) override
	{
		D(dprintf("%s(%p): %s\n", __PRETTY_FUNCTION__, this, m_url.utf8().data()));
		if (m_curlRequest.get() == &request)
		{
			D(dprintf("%s(%p)..\n", __PRETTY_FUNCTION__, this));
			m_response = ResourceResponse(response);
			
			// only set on 1st request (or when we're reading from pos=0)
			if (0 == m_bufferPositionAbs)
				m_length = reinterpret_cast<int64_t>(m_response.expectedContentLength());

			if (m_response.shouldRedirect())
			{
				static const int maxRedirects = 20;

				if (m_redirectCount++ > maxRedirects)
				{
					m_didFailLoading = true;
					m_eventSemaphore.signal();
					return;
				}

				String location = m_response.httpHeaderField(HTTPHeaderName::Location);
				URL newURL = URL(m_response.url(), location);
				bool crossOrigin = !protocolHostAndPortAreEqual(m_request.url(), newURL);

				ResourceRequest newRequest = m_request;
				newRequest.setURL(newURL);

				if (shouldRedirectAsGET(newRequest, crossOrigin)) {
					newRequest.setHTTPMethod("GET");
					newRequest.setHTTPBody(nullptr);
					newRequest.clearHTTPContentType();
				}

				if (crossOrigin) {
					// If the network layer carries over authentication headers from the original request
					// in a cross-origin redirect, we want to clear those headers here.
					newRequest.clearHTTPAuthorization();
					newRequest.clearHTTPOrigin();
				}

				m_curlRequest->cancel();
				m_curlRequest = createCurlRequest(newRequest);
				if (m_curlRequest)
				{
					m_curlRequest->setResumeOffset(static_cast<long long>(m_bufferPositionAbs));
					m_curlRequest->start();
				}

				D(dprintf("%s(%p): redirected to %s\n", __PRETTY_FUNCTION__, this, location.utf8().data()));
				return;
			}
			
			request.completeDidReceiveResponse();
		}
	}
	
	void curlDidReceiveBuffer(CurlRequest& request, Ref<SharedBuffer>&&buffer) override
	{
		D(dprintf("%s(%p): %d bytes, currently buffered size: %d\n", __PRETTY_FUNCTION__, this, buffer->size(), m_bufferSize));
		if (m_curlRequest.get() == &request)
		{
			if (buffer->size())
			{
				{
					auto lock = Locker(m_bufferLock);
					m_bufferSize += buffer->size();
					m_buffer.push(RefPtr<SharedBuffer>(WTFMove(buffer)));

					if (m_bufferSize > m_readAhead && !m_isPaused)
					{
						if (m_curlRequest)
						{
							D(dprintf("%s: suspending...\n", __PRETTY_FUNCTION__));
							m_curlRequest->suspend();
							m_isPaused = true;
						}
					}
				}

				m_eventSemaphore.signal();
			}
		}
	}
	
	void curlDidComplete(CurlRequest& request, NetworkLoadMetrics&&) override
	{
		D(dprintf("%s(%p): %s %lld\n", __PRETTY_FUNCTION__, this, m_url.utf8().data(), m_length));
		if (m_curlRequest.get() == &request)
		{
			m_finishedLoading = true;
			m_eventSemaphore.signal();
			m_curlRequest->cancel();
			m_curlRequest = nullptr;
			m_nonRecoverableErrors = 0;
		}
	}
	
	void curlDidFailWithError(CurlRequest& request, ResourceError&& error, CertificateInfo&&) override
	{
		if (m_curlRequest.get() == &request)
		{
			m_curlRequest->cancel();
			m_curlRequest = nullptr;
			m_isPaused = true;
			m_didFailLoading = true;
			m_eventSemaphore.signal();
			if (error.type() != ResourceError::Type::Timeout && error.type() != ResourceError::Type::Cancellation)
				m_nonRecoverableErrors ++;
			D(dprintf("%s(%p): error type %d code %d %s nrcount %d\n", __PRETTY_FUNCTION__, this, int(error.type()), int(error.errorCode()), error.localizedDescription().utf8().data(), m_nonRecoverableErrors));
		}
	}
protected:
	ResourceRequest                  m_request;
	ResourceResponse                 m_response;
	unsigned                         m_redirectCount = 0;
	BinarySemaphore                  m_eventSemaphore;
	RefPtr<CurlRequest>              m_curlRequest;
	Lock                             m_bufferLock;
	std::queue<RefPtr<SharedBuffer>> m_buffer;
	// pos within the front() chunk
	int                              m_bufferRead = 0;
	// abs position in the stream that we've read (not in the buffer anymore)
	uint64_t                         m_bufferPositionAbs = 0;
	// size of all chunks on the list (- m_bufferRead)
	int                              m_bufferSize = 0;
	bool                             m_finishedLoading = false;
	bool                             m_didFailLoading = false;
	bool                             m_isPaused = false;
	bool                             m_seekProcessed = true;
	int                              m_nonRecoverableErrors = 0;
	static const int                 ms_maxNonRecoverableErrors = 3;
};

class AcinerellaPreloadingNetworkBufferInternal : public AcinerellaNetworkBuffer
{
public:
	AcinerellaPreloadingNetworkBufferInternal(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, RefPtr<SharedBuffer> prepend)
		: AcinerellaNetworkBuffer(resourceProvider, url, 0)
	{
		DP(dprintf("%s(%p): prepend %d\n", __PRETTY_FUNCTION__, this, !!prepend?prepend->size():0));
		if (!!prepend)
		{
			m_buffer = SharedBuffer::create(prepend->data(), prepend->size());
		}
	}
	
	AcinerellaPreloadingNetworkBufferInternal(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, RefPtr<SharedBuffer> key, const unsigned char *iv, RefPtr<SharedBuffer> prepend)
		: AcinerellaNetworkBuffer(resourceProvider, url, 0)
	{
		DP(dprintf("%s(%p): prepend %d\n", __PRETTY_FUNCTION__, this, !!prepend?prepend->size():0));
		if (!!prepend)
		{
			m_buffer = SharedBuffer::create(prepend->data(), prepend->size());
		}
		
		m_encryptionKey = key;
		if (iv)
		{
			m_hasIV = true;
			memcpy(&m_iv[0], iv, sizeof(m_iv));
		}
	}
	
	~AcinerellaPreloadingNetworkBufferInternal()
	{
		DP(dprintf("%s(%p): \n", __PRETTY_FUNCTION__, this));
		m_dataReceived = true;
		m_eventSemaphore.signal();
		if (!!m_request)
			m_request->cancel();
	}

	void start(uint64_t) override
	{
		DP(dprintf("%s(%p): \n", __PRETTY_FUNCTION__, this));
		if (!!m_request)
			return;

		m_request = AcinerellaNetworkFileRequest::create(m_url, [this, protect = makeRef(*this)](bool success) {
			if (success)
			{
				auto buffer = m_request->buffer();
				if (!!m_buffer)
					m_buffer->append(buffer->data(), buffer->size());
				else
					m_buffer = buffer;

				DP(dprintf("%s(%p): received, total len %d (%d)\n", __PRETTY_FUNCTION__, this, m_buffer->size(), buffer->size()));

#if ENABLE(WEB_CRYPTO)
				if (!!m_encryptionKey && !!m_buffer)
				{
					CryptoAlgorithmAesCbcCfbParams params;
					auto key = CryptoKeyAES::importRaw(CryptoAlgorithmIdentifier::AES_CBC, m_encryptionKey->copyData(), true, CryptoKeyUsageDecrypt);
					if (m_hasIV)
					{
						auto asAB = JSC::ArrayBuffer::tryCreate(&m_iv[0], sizeof(m_iv));
						if (asAB)
						{
							params.iv = BufferSource(asAB);
						}
					}
					auto decryptResult = CryptoAlgorithmAES_CBC::platformDecrypt(params, *key, m_buffer->copyData(), CryptoAlgorithmAES_CBC::Padding::No);
					if (!decryptResult.hasException())
					{
						auto encrypted = decryptResult.releaseReturnValue();
						DP(dprintf("%s(%p): encryption succeded!\n", __PRETTY_FUNCTION__, this));
						m_buffer->clear();
						m_buffer->append(encrypted.data(), encrypted.sizeInBytes());
					}
					else
					{
						DP(dprintf("%s(%p): encryption failed :(\n", __PRETTY_FUNCTION__, this));
					}
				}
#endif
				m_length = m_buffer->size();
			}

			m_dataReceived = true;
			m_eventSemaphore.signal();
		});
	}

	void stop() override
	{
		if (!!m_request)
			m_request->cancel();
		m_request = nullptr;
		m_dataReceived = true;
		m_eventSemaphore.signal();
	}
	
	int64_t position() override
	{
		return int64_t(m_bufferPositionAbs);
	}

	bool canSeek() override
	{
		return true;
	}

	int read(uint8_t *outBuffer, int size, int64_t readPosition) override
	{
		DP(dprintf("%s(%p): >> received? %d\n", __PRETTY_FUNCTION__, this, m_dataReceived));
		while (!m_dataReceived)
			m_eventSemaphore.waitFor(10_s);
		
		if (!!m_buffer)
		{
			if (-1 != readPosition && int64_t(m_bufferPositionAbs) != readPosition)
			{
				DP(dprintf("%s(%p): seek to %lld\n", __PRETTY_FUNCTION__, this, readPosition));
				m_bufferPositionAbs = readPosition;
			}

			int sizeMax = m_buffer->size() - m_bufferPositionAbs;
			size = std::min(size, sizeMax);

			if (size > 0 && size < int(m_buffer->size()))
			{
				memcpy(outBuffer, m_buffer->data() + m_bufferPositionAbs, size);

				DP(dprintf("%s(%p): read from %d, size %d\n", __PRETTY_FUNCTION__, this, int(m_bufferPositionAbs), size));
				m_bufferPositionAbs += size;
				return size;
			}

			DP(dprintf("%s(%p): read from %d, size %d\n", __PRETTY_FUNCTION__, this, int(m_bufferPositionAbs), 0));
			return 0;
		}
		
		DP(dprintf("%s(%p): read failed!\n", __PRETTY_FUNCTION__, this));
		return -1;
	}

protected:
	int32_t                              m_bufferPositionAbs = 0;
	RefPtr<AcinerellaNetworkFileRequest> m_request;
	BinarySemaphore                      m_eventSemaphore;
	RefPtr<SharedBuffer>                 m_buffer;
	RefPtr<SharedBuffer>                 m_encryptionKey;
	unsigned char                        m_iv[16];
	bool                                 m_dataReceived = false;
	bool                                 m_hasIV = false;
};

AcinerellaNetworkBuffer::AcinerellaNetworkBuffer(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead)
	: m_provider(resourceProvider)
	, m_url(url)
	, m_readAhead(readAhead)
{
	D(dprintf("%s(%p) - %s\n", __PRETTY_FUNCTION__, this, url.utf8().data()));
	if (m_provider)
		m_provider->ref();
}

AcinerellaNetworkBuffer::~AcinerellaNetworkBuffer()
{
	if (m_provider)
		m_provider->deref();
}

void AcinerellaNetworkBuffer::die()
{
	if (m_provider)
		m_provider->deref();
	m_provider = nullptr;
	m_dead = true;
	stop();
}

RefPtr<AcinerellaNetworkBuffer> AcinerellaNetworkBuffer::create(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead)
{
#if 0
	if (startsWithLettersIgnoringASCIICase(url, "blob:"))
		return RefPtr<AcinerellaNetworkBuffer>(WTF::adoptRef(*new AcinerellaNetworkBufferPlatformMediaResourceLoader(resourceProvider, url, readAhead)));
#endif
	if (url.contains("m3u8"))
		return RefPtr<AcinerellaNetworkBuffer>(WTF::adoptRef(*new AcinerellaNetworkBufferHLS(resourceProvider, url, readAhead)));
	return RefPtr<AcinerellaNetworkBuffer>(WTF::adoptRef(*new AcinerellaNetworkBufferInternal(resourceProvider, url, readAhead)));
}

RefPtr<AcinerellaNetworkBuffer> AcinerellaNetworkBuffer::createDisregardingFileType(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, size_t readAhead)
{
	return RefPtr<AcinerellaNetworkBuffer>(WTF::adoptRef(*new AcinerellaNetworkBufferInternal(resourceProvider, url, readAhead)));
}

RefPtr<AcinerellaNetworkBuffer> AcinerellaNetworkBuffer::createPreloadingBuffer(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, RefPtr<SharedBuffer> prepend)
{
	return RefPtr<AcinerellaNetworkBuffer>(WTF::adoptRef(*new AcinerellaPreloadingNetworkBufferInternal(resourceProvider, url, prepend)));
}

RefPtr<AcinerellaNetworkBuffer> AcinerellaNetworkBuffer::createPreloadingEncryptedBuffer(AcinerellaNetworkBufferResourceLoaderProvider *resourceProvider, const String &url, RefPtr<SharedBuffer> key, const unsigned char *iv, RefPtr<SharedBuffer> prepend)
{
	return RefPtr<AcinerellaNetworkBuffer>(WTF::adoptRef(*new AcinerellaPreloadingNetworkBufferInternal(resourceProvider, url, key, iv, prepend)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class AcinerellaNetworkFileRequestInternal : public AcinerellaNetworkFileRequest, CurlRequestClient
{
public:
	AcinerellaNetworkFileRequestInternal(const String &url, Function<void(bool)>&& onFinished)
		: AcinerellaNetworkFileRequest(url, WTFMove(onFinished))
	{
		m_request = ResourceRequest(m_url);
		m_request.setCachePolicy(ResourceRequestCachePolicy::DoNotUseAnyCache);
		m_curlRequest = createCurlRequest(m_request);

		if (m_curlRequest)
		{
			m_curlRequest->start();
		}
		else
		{
			m_onFinished(false);
		}
	}

	AcinerellaNetworkFileRequestInternal(const String &url, Function<void(RefPtr<SharedBuffer>)>&& onFinished)
		: AcinerellaNetworkFileRequest(url, WTFMove(onFinished))
	{
		m_request = ResourceRequest(m_url);
		m_request.setCachePolicy(ResourceRequestCachePolicy::DoNotUseAnyCache);
		m_curlRequest = createCurlRequest(m_request);

		if (m_curlRequest)
		{
			m_curlRequest->start();
		}
		else
		{
			m_onFinished2(nullptr);
		}
	}

	void onFinished(bool success)
	{
		if (m_onFinished)
		{
			Function<void(bool)> onTmpFinished;
			std::swap(m_onFinished, onTmpFinished);
			WTF::callOnMainThread([this, success, onFinished(std::move(onTmpFinished)), protect = makeRef(*this)]() {
				onFinished(success);
			});
		}
		else if (m_onFinished2)
		{
			Function<void(RefPtr<SharedBuffer>)> onTmpFinished;
			std::swap(m_onFinished2, onTmpFinished);
			WTF::callOnMainThread([this, success, onFinished(std::move(onTmpFinished)), protect = makeRef(*this)]() {
				onFinished(success ? buffer() : nullptr);
			});
		}
	}

	void cancel() override
	{
		if (m_curlRequest)
			m_curlRequest->cancel();
		m_onFinished = nullptr;
		m_onFinished2 = nullptr;
	}

	RefPtr<SharedBuffer> buffer() override { return m_buffer; }

	Ref<CurlRequest> createCurlRequest(ResourceRequest&request)
	{
		auto context = MediaPlayerMorphOSSettings::settings().m_networkingContextForRequests;
		if (context)
		{
			auto& storageSession = *context->storageSession();
			auto includeSecureCookies = request.url().protocolIs("https") ? IncludeSecureCookies::Yes : IncludeSecureCookies::No;
			String cookieHeaderField = storageSession.cookieRequestHeaderFieldValue(request.firstPartyForCookies(), SameSiteInfo::create(request), request.url(), std::nullopt, std::nullopt, includeSecureCookies, ShouldAskITP::Yes, ShouldRelaxThirdPartyCookieBlocking::No).first;
			if (!cookieHeaderField.isEmpty())
				request.addHTTPHeaderField(HTTPHeaderName::Cookie, cookieHeaderField);
		}

		auto curlRequest = CurlRequest::create(WTFMove(request), *this);
		return curlRequest;
	}

	void curlDidSendData(CurlRequest&, unsigned long long, unsigned long long) override
	{
	}

	inline bool shouldRedirectAsGET(const ResourceRequest& request, bool crossOrigin)
	{
		if ((request.httpMethod() == "GET") || (request.httpMethod() == "HEAD"))
			return false;

		if (!request.url().protocolIsInHTTPFamily())
			return true;

		if (m_response.isSeeOther())
			return true;

		if ((m_response.isMovedPermanently() || m_response.isFound()) && (request.httpMethod() == "POST"))
			return true;

		if (crossOrigin && (request.httpMethod() == "DELETE"))
			return true;

		return false;
	}
	
	void curlDidReceiveResponse(CurlRequest& request, CurlResponse&& response) override
	{
		D(dprintf("%s(%p): %s\n", __PRETTY_FUNCTION__, this, m_url.utf8().data()));
		if (m_curlRequest.get() == &request)
		{
			D(dprintf("%s(%p)..\n", __PRETTY_FUNCTION__, this));
			m_response = ResourceResponse(response);

			if (m_response.shouldRedirect())
			{
				static const int maxRedirects = 20;

				if (m_redirectCount++ > maxRedirects)
				{
					onFinished(false);
					return;
				}

				String location = m_response.httpHeaderField(HTTPHeaderName::Location);
				URL newURL = URL(m_response.url(), location);
				bool crossOrigin = !protocolHostAndPortAreEqual(m_request.url(), newURL);

				ResourceRequest newRequest = m_request;
				newRequest.setURL(newURL);

				if (shouldRedirectAsGET(newRequest, crossOrigin)) {
					newRequest.setHTTPMethod("GET");
					newRequest.setHTTPBody(nullptr);
					newRequest.clearHTTPContentType();
				}

				if (crossOrigin) {
					// If the network layer carries over authentication headers from the original request
					// in a cross-origin redirect, we want to clear those headers here.
					newRequest.clearHTTPAuthorization();
					newRequest.clearHTTPOrigin();
				}

				m_curlRequest->cancel();
				m_curlRequest = createCurlRequest(newRequest);
				if (m_curlRequest)
				{
					m_curlRequest->start();
				}
				else
				{
					onFinished(false);
				}

				D(dprintf("%s(%p): redirected to %s\n", __PRETTY_FUNCTION__, this, location.utf8().data()));
				return;
			}
			
			request.completeDidReceiveResponse();
		}
	}
	
	void curlDidReceiveBuffer(CurlRequest& request, Ref<SharedBuffer>&&buffer) override
	{
		if (m_curlRequest.get() == &request)
		{
			if (buffer->size())
			{
				D(dprintf("%s(%p): append %d\n", __PRETTY_FUNCTION__, this, buffer->size()));
				if (!m_buffer)
					m_buffer = RefPtr<SharedBuffer>(WTFMove(buffer));
				else
					m_buffer->append(WTFMove(buffer));
			}
		}
	}
	
	void curlDidComplete(CurlRequest& request, NetworkLoadMetrics&&) override
	{
		D(dprintf("%s(%p): %s %d OK %d onfini %d\n", __PRETTY_FUNCTION__, this, m_url.utf8().data(), m_buffer?m_buffer->size():0, m_curlRequest.get() == &request, !!m_onFinished));
		if (m_curlRequest.get() == &request)
		{
			onFinished(true);
		}
	}
	
	void curlDidFailWithError(CurlRequest& request, ResourceError&&, CertificateInfo&&) override
	{
		D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
		if (m_curlRequest.get() == &request)
		{
			onFinished(false);
		}
	}

	void ref() override { ThreadSafeRefCounted<AcinerellaNetworkFileRequest>::ref(); }
	void deref() override { ThreadSafeRefCounted<AcinerellaNetworkFileRequest>::deref(); }

protected:
	ResourceRequest      m_request;
	ResourceResponse     m_response;
	unsigned             m_redirectCount = 0;
	BinarySemaphore      m_eventSemaphore;
	RefPtr<CurlRequest>  m_curlRequest;
	RefPtr<SharedBuffer> m_buffer;
};

RefPtr<AcinerellaNetworkFileRequest> AcinerellaNetworkFileRequest::create(const String &url, Function<void(bool)>&& onFinished)
{
	return adoptRef(*new AcinerellaNetworkFileRequestInternal(url, WTFMove(onFinished)));
}

RefPtr<AcinerellaNetworkFileRequest> AcinerellaNetworkFileRequest::createRefResult(const String &url, Function<void(RefPtr<SharedBuffer>)>&& onFinished)
{
	return adoptRef(*new AcinerellaNetworkFileRequestInternal(url, WTFMove(onFinished)));
}

}
}
#endif
