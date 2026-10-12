#include "NetworkSession.h"
#include "WebProcess.h"

namespace WebKit {

#if HAS_CACHE_STORAGE
CacheStorage::Engine& NetworkSession::ensureCacheEngine()
{
    if (m_cacheEngine)
        return *(m_cacheEngine.get());

    m_cacheEngine = CacheStorage::Engine::create(*this, "PROGDIR:Cache/CacheStorage"_s);
    return *(m_cacheEngine.get());
}

void NetworkSession::clearCacheEngine()
{
    m_cacheEngine = nullptr;
}
#endif

PAL::SessionID NetworkSession::sessionID() const
{
    return WebProcess::singleton().sessionID();
}

void NetworkSession::shutdown()
{
#if HAS_CACHE_STORAGE

    if (m_cacheEngine)
        m_cacheEngine->shutdown();
    m_cacheEngine = nullptr;
#endif
}

}

