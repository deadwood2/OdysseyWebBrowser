#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include <wtf/ThreadSafeRefCounted.h>
#include "acinerella.h"

namespace WebCore {
namespace Acinerella {

class AcinerellaPointer : public ThreadSafeRefCounted<AcinerellaPointer>
{
template<typename T>
using deleted_unique_ptr = std::unique_ptr<T,std::function<void(T*)>>;
	AcinerellaPointer(ac_instance *instance = nullptr);
public:
	~AcinerellaPointer();
	
	static RefPtr<AcinerellaPointer> create(ac_instance *instance = nullptr);
	static constexpr int maxDecoders = 32;

	void setInstance(ac_instance *instance);
	ac_instance *instance() { return m_instance.get(); }

	void setDecoder(int index, ac_decoder *);
	ac_decoder *decoder(int index) { return m_decoders[index].get(); };

protected:
	deleted_unique_ptr<ac_instance> m_instance;
	deleted_unique_ptr<ac_decoder>  m_decoders[maxDecoders];
};

}
}

#endif
