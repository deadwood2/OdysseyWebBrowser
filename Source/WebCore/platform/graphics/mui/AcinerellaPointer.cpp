#include "AcinerellaPointer.h"

#if ENABLE(VIDEO)

#include <wtf/RefPtr.h>
#include <proto/exec.h>

#define D(x) 

namespace WebCore {
namespace Acinerella {

RefPtr<AcinerellaPointer> AcinerellaPointer::create(ac_instance *instance)
{
	RefPtr<AcinerellaPointer> pointer = WTF::adoptRef(*new AcinerellaPointer(instance ? instance : ac_init()));
	if (pointer && !pointer->instance())
		return nullptr;
	return pointer;
}

AcinerellaPointer::AcinerellaPointer(ac_instance *instance)
	: m_instance(deleted_unique_ptr<ac_instance>(instance, [](ac_instance*instance){ ac_free(instance); }))
{
	D(dprintf("%s(%p): created %p\n", __PRETTY_FUNCTION__, this, m_instance.get()));
}

AcinerellaPointer::~AcinerellaPointer()
{
	D(dprintf("%s(%p): killing %p\n", __PRETTY_FUNCTION__, this, m_instance.get()));
}

void AcinerellaPointer::setInstance(ac_instance *instance)
{
	for (int i = 0; i < maxDecoders; i++)
		m_decoders[i].reset();
	m_instance.reset();
	
	m_instance = deleted_unique_ptr<ac_instance>(instance, [](ac_instance*instance){ ac_free(instance); });
	D(dprintf("%s(%p): set %p\n", __PRETTY_FUNCTION__, this, m_instance.get()));
}

void AcinerellaPointer::setDecoder(int i, ac_decoder *decoder)
{
	m_decoders[i] = deleted_unique_ptr<ac_decoder>(decoder, [](ac_decoder* decoder){ ac_free_decoder(decoder); });
}

}
}

#endif
