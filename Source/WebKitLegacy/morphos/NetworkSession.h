#pragma once
#include "WebKit.h"
#include <pal/SessionID.h>
#include <wtf/HashSet.h>
#include <wtf/Ref.h>
#include <wtf/Seconds.h>
#include <wtf/UniqueRef.h>
#include <wtf/WeakHashSet.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/WTFString.h>

#define HAS_CACHE_STORAGE 0

#if HAS_CACHE_STORAGE
#include "cache/CacheStorageEngine.h"
#endif

namespace WebKit {

class NetworkSession : public CanMakeWeakPtr<NetworkSession> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static std::unique_ptr<NetworkSession> create() { return std::make_unique<NetworkSession>(); }
    virtual ~NetworkSession() = default;

#if HAS_CACHE_STORAGE
    CacheStorage::Engine& ensureCacheEngine();
    void clearCacheEngine();
#endif

    PAL::SessionID sessionID() const;
    
    void shutdown();

protected:
#if HAS_CACHE_STORAGE
    RefPtr<CacheStorage::Engine> m_cacheEngine;
#endif
};

}
