#include "AcinerellaBuffer.h"
#include "MediaPlayerMorphOS.h"

#if ENABLE(VIDEO)

#include "SynchronousLoaderClient.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include <queue>
#include "AcinerellaDecoder.h"
#include "AcinerellaHLS.h"

#define D(x) 
#define DPUSH(x)
#define DF(x)

namespace WebCore {
namespace Acinerella {

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AcinerellaMuxedBuffer::setSinkFunction(Function<bool(int decoderIndex, int left, uint32_t bytesInBuffer)>&& sinkFunction)
{
	auto lock = Locker(m_lock);
	m_sinkFunction = WTFMove(sinkFunction);
}

void AcinerellaMuxedBuffer::setDecoderMask(uint32_t mask, uint32_t audioMask)
{
	auto lock = Locker(m_lock);
	m_decoderMask = mask;
	m_audioDecoderMask = audioMask;
}

void AcinerellaMuxedBuffer::push(RefPtr<AcinerellaPackage> &package)
{
	EP_SCOPE(push);
	DPUSH(dprintf("%s: %p package index %lu isFlush %d mask %lx\n", __PRETTY_FUNCTION__, this, package ? package->index() : -1, package ? package->isFlushPackage() : 0, m_decoderMask));

	// is this a valid package?
	if (package && package->isValid())
	{
		const auto index = package->index();
		
		{
			auto lock = Locker(m_lock);

			// Flush goes into all valid queues!
			if (package->isFlushPackage())
			{
				m_queueCompleteOrError = false;
				forValidDecoders([&](AcinerellaPackageQueue& queue, BinarySemaphore&) {
					queue.emplace(package);
				});
			}
			else if (isDecoderValid(index))
			{
				m_packages[index].emplace(package);
				m_bytes[index] += ac_get_package_size(package->package());
				DPUSH(dprintf("%s: pushing into packages queue @ index %d, size %d type %s\n", __PRETTY_FUNCTION__, index, m_packages[index].size(), (m_audioDecoderMask & (1UL << index)) ? "audio" : "video"));
			}
			else
			{
				DPUSH(dprintf("%s: no valid decoder at index %d, mask %lx\n", __PRETTY_FUNCTION__, m_decoderMask));
			}
		}
		
		if (isDecoderValid(index))
			m_events[index].signal();
	}
	else
	{
		m_queueCompleteOrError = true;
		DPUSH(dprintf("%s: EOS!!\n", __PRETTY_FUNCTION__));
	
		forValidDecoders([](AcinerellaPackageQueue&, BinarySemaphore& event) {
			event.signal();
		});
	}
}

void AcinerellaMuxedBuffer::push(RefPtr<AcinerellaPackage> &package, int index)
{
	EP_SCOPE(push);
	DPUSH(dprintf("%s: %p package index %lu isFlush %d mask %lx\n", __PRETTY_FUNCTION__, this, package ? package->index() : -1, package ? package->isFlushPackage() : 0, m_decoderMask));

	// is this a valid package?
	if (package && package->isValid())
	{
		{
			auto lock = Locker(m_lock);

			if (isDecoderValid(index))
			{
				m_packages[index].emplace(package);
				m_bytes[index] += package->isFlushPackage() ? 0 : ac_get_package_size(package->package());
				DPUSH(dprintf("%s: pushing into packages queue @ index %d, size %d type %s\n", __PRETTY_FUNCTION__, index, m_packages[index].size(), (m_audioDecoderMask & (1UL << index)) ? "audio" : "video"));
			}
			else
			{
				DPUSH(dprintf("%s: no valid decoder at index %d, mask %lx\n", __PRETTY_FUNCTION__, m_decoderMask));
			}
		}
		
		if (isDecoderValid(index))
			m_events[index].signal();
	}
}

void AcinerellaMuxedBuffer::flush()
{
	{
		auto lock = Locker(m_lock);
		m_queueCompleteOrError = false;
		forValidDecoders([](AcinerellaPackageQueue& queue, BinarySemaphore&) {
			DF(dprintf("[Mux]Flushing queue %p size %d\n", &queue, queue.size()));
			while (!queue.empty())
				queue.pop();
		});
	}
}

void AcinerellaMuxedBuffer::flush(int decoderIndex)
{
	{
		auto lock = Locker(m_lock);
		m_queueCompleteOrError = false;
		while (!m_packages[decoderIndex].empty())
			m_packages[decoderIndex].pop();
		m_bytes[decoderIndex] = 0;
	}
}

void AcinerellaMuxedBuffer::terminate()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	EP_SCOPE(terminate);

	{
		auto lock = Locker(m_lock);
		m_sinkFunction = nullptr;
		m_queueCompleteOrError = true;
	}

	forValidDecoders([](AcinerellaPackageQueue&, BinarySemaphore& event) {
		event.signal();
	});
}

RefPtr<AcinerellaPackage> AcinerellaMuxedBuffer::nextPackage(AcinerellaDecoder &decoder)
{
	D(dprintf("%s: isAudio %d index %lu\n", __func__, decoder.isAudio(), decoder.index()));
	EP_SCOPE(nextPackage);

	const auto index = decoder.index();

	for (;;)
	{
		bool requestMore = false;
		int sizeLeft = 0;
		uint32_t bytes = 0;
		RefPtr<AcinerellaPackage> hasPackage = nullptr;

		{
			auto lock = Locker(m_lock);
			D(dprintf("%s: packages %d complete %d\n", __func__, m_packages[index].size(), m_queueCompleteOrError));
			if (!m_packages[index].empty())
			{
				hasPackage = m_packages[index].front();
				m_bytes[index] -= ac_get_package_size(hasPackage->package());
				m_packages[index].pop();
				sizeLeft = m_packages[index].size();
				bytes = m_bytes[index];
				requestMore = sizeLeft < queueReadAheadSize;
			}
			else if (m_queueCompleteOrError)
			{
				return nullptr;
			}
			else
			{
				requestMore = true;
			}

			if (requestMore && m_sinkFunction && !m_queueCompleteOrError)
			{
				D(dprintf("%s: calling sink..\n", __func__));
				if (!m_sinkFunction(decoder.index(), sizeLeft, bytes))
					return hasPackage;
			}
		}
		
		if (hasPackage)
			return hasPackage;
		
		EP_EVENT(wait);
		m_events[index].waitFor(1_s);
	}

	return nullptr;
}

int AcinerellaMuxedBuffer::packagesForDecoder(int decoderIndex)
{
	auto lock = Locker(m_lock);
	return m_packages[decoderIndex].size();
}

uint32_t AcinerellaMuxedBuffer::bytesForDecoder(int decoderIndex)
{
	auto lock = Locker(m_lock);
	return m_bytes[decoderIndex];
}

uint32_t AcinerellaMuxedBuffer::maxBufferSizeForMediaSourceDecoder(int decoderIndex)
{
	// Must match multipliers from MediaElementSession::requiresPlaybackTargetRouteMonitoring
	// and setMaximumSourceBufferSize set in WebPage.cpp
	auto lock = Locker(m_lock);
	if ((m_audioDecoderMask & (1UL << decoderIndex)))
		return 5033164;
	return 28521267;
}

}
}
#endif
