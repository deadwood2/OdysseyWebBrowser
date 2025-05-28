#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/WTFString.h>
#include <wtf/Function.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/threads/BinarySemaphore.h>
#include "acinerella.h"
#include "AcinerellaPointer.h"
#include <queue>

namespace WebCore {
namespace Acinerella {

class AcinerellaMuxedBuffer;
class AcinerellaDecoder;

class AcinerellaPackage : public ThreadSafeRefCounted<AcinerellaPackage>
{
	AcinerellaPackage(RefPtr<AcinerellaPointer>&pointer, ac_package *package) : m_acinerella(pointer), m_package(package) { }
	AcinerellaPackage& operator=(AcinerellaPackage&& otter) = delete;
	AcinerellaPackage& operator=(AcinerellaPackage const & otter) = delete;

public:
    static Ref<AcinerellaPackage> create(RefPtr<AcinerellaPointer>&pointer, ac_package *package)
    {
        return adoptRef(*new AcinerellaPackage(pointer, package));
    }

	~AcinerellaPackage() { if (m_package) ac_free_package(m_package); }

	ac_package *package() { return m_package; }
	int index() const { return m_package ? m_package->stream_index : -1; }
	bool isFlushPackage() const { return m_package == ac_flush_packet(); }
	bool isValid() const { return nullptr != m_package; }

	RefPtr<AcinerellaPointer>& acinerella() { return m_acinerella; }

protected:
	RefPtr<AcinerellaPointer> m_acinerella;
	ac_package               *m_package;
};

class AcinerellaMuxedBuffer : public ThreadSafeRefCounted<AcinerellaMuxedBuffer>
{
public:
	AcinerellaMuxedBuffer() = default;
	~AcinerellaMuxedBuffer() = default;

	static RefPtr<AcinerellaMuxedBuffer> create() {
		return WTF::adoptRef(*new AcinerellaMuxedBuffer());
	}

	static constexpr int maxDecoders = 16;
	static constexpr int queueReadAheadSize = 64;

	void setSinkFunction(Function<bool(int decoderIndex, int packagesLeft, uint32_t bufferSize)>&& sinkFunction);
	void setDecoderMask(uint32_t mask, uint32_t audioMask = 0);

	// To be called on Acinerella thread
	void push(RefPtr<AcinerellaPackage> &package);
	void push(RefPtr<AcinerellaPackage> &package, int decoderIndex);
	void flush();
	void flush(int decoderIndex);
	void terminate();

	// This is meant to be called from the decoder threads. Will block until a valid package can be returned
	// or sinkfunction returns false (MediaStream)
	RefPtr<AcinerellaPackage> nextPackage(AcinerellaDecoder &decoder);
	bool isEOS() const { return m_queueCompleteOrError; }

	int packagesForDecoder(int decoderIndex);
	uint32_t bytesForDecoder(int decoderIndex);

	uint32_t maxBufferSizeForMediaSourceDecoder(int decoderIndex);

protected:
	typedef std::queue<RefPtr<AcinerellaPackage>> AcinerellaPackageQueue;

	inline bool isDecoderValid(int index) {
		return 0 != (m_decoderMask & (1UL << index));
	}

	inline void forValidDecoders(Function<void(AcinerellaPackageQueue& queue, BinarySemaphore& event)> &&function) {
		uint32_t mask = m_decoderMask;
		for (int i = 0; mask && (i < maxDecoders); i++) {
			if (isDecoderValid(i)) {
				function(m_packages[i], m_events[i]);
				mask &= ~(1L << i);
			}
		}
	}

	inline void whileValidDecoders(Function<bool(AcinerellaPackageQueue& queue, BinarySemaphore& event)> &&function) {
		uint32_t mask = m_decoderMask;
		for (int i = 0; mask && (i < maxDecoders); i++) {
			if (isDecoderValid(i)) {
				if (!function(m_packages[i], m_events[i]))
					return;
				mask &= ~(1L << i);
			}
		}
	}

	inline void forInvalidDecoders(Function<void(AcinerellaPackageQueue& queue, BinarySemaphore& event)> &&function) {
		for (int i = 0; i < maxDecoders; i++) {
			if (!isDecoderValid(i)) {
				function(m_packages[i], m_events[i]);
			}
		}
	}

	bool needsToCallSinkFunction() {
		bool hasNonFullQueues = false;
		if (m_queueCompleteOrError)
			return false;
		whileValidDecoders([&](AcinerellaPackageQueue& queue, BinarySemaphore&) -> bool {
			if (queue.size() < queueReadAheadSize) {
				hasNonFullQueues = true;
				return false;
			}
			return true; // continue scanning
		});
		return hasNonFullQueues;
	}

protected:
	Function<bool(int, int, uint32_t)> m_sinkFunction;

	AcinerellaPackageQueue   m_packages[maxDecoders];
	uint32_t                 m_bytes[maxDecoders] = { 0 };
	BinarySemaphore          m_events[maxDecoders];
	Lock                     m_lock;

	uint32_t                m_decoderMask = 0;
	uint32_t                m_audioDecoderMask = 0;
	bool                    m_queueCompleteOrError = false;
};

}
}

#endif
