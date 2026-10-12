#include "config.h"
#include <wtf/RefPtr.h>
#include "SharedMemory.h"

namespace WebCore {

MorphOSHandleData::MorphOSHandleData(size_t size)
    : m_data(malloc(size))
    , m_size(size)
{
}

MorphOSHandleData::~MorphOSHandleData()
{
    free(m_data);
}

RefPtr<SharedMemory> SharedMemory::allocate(size_t size)
{
    RefPtr<MorphOSHandleData> shared = adoptRef(new MorphOSHandleData(size));
    if (!!shared)
    {
        RefPtr<SharedMemory> memory = adoptRef(new SharedMemory);
        memory->m_size = size;
        memory->m_data = shared->data();
        memory->m_handle.m_shared = shared;

        return memory;
    }
}

auto SharedMemory::createHandle(Protection protection) -> std::optional<Handle>
{
    MorphOSHandle handle;
    handle.m_shared = m_handle.m_shared;
    return { Handle(WTFMove(handle), m_size) };
}

RefPtr<SharedMemory> SharedMemory::map(Handle&& handle, Protection protection)
{
    if (!handle.m_handle.m_shared)
        return nullptr;

    RefPtr<SharedMemory> memory = adoptRef(new SharedMemory);
    memory->m_size = handle.m_handle.m_shared->size();
    memory->m_data = handle.m_handle.m_shared->data();
    memory->m_handle = handle.m_handle;
    return memory;
}

SharedMemory::~SharedMemory()
{
}

}
