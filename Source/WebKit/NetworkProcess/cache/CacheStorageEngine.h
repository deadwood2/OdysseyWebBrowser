/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CacheStorageEngineCaches.h"
#include "NetworkCacheData.h"
#include "WebsiteData.h"
#include <WebCore/ClientOrigin.h>
#include <wtf/HashMap.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/WorkQueue.h>

namespace IPC {
class Connection;
}

namespace WebCore {
class SessionID;
}

namespace WTF {
class CallbackAggregator;
};

namespace WebKit {

namespace CacheStorage {

using CacheIdentifier = uint64_t;
using LockCount = uint64_t;

class Engine : public ThreadSafeRefCounted<Engine> {
public:
    ~Engine();

    static Engine& from(PAL::SessionID);
    static void destroyEngine(PAL::SessionID);
    static void fetchEntries(PAL::SessionID, bool shouldComputeSize, WTF::CompletionHandler<void(Vector<WebsiteData::Entry>)>&&);

    static Ref<Engine> create(String&& rootPath) { return adoptRef(*new Engine(WTFMove(rootPath))); }

    bool shouldPersist() const { return !!m_ioQueue;}

    void open(const WebCore::ClientOrigin&, const String& cacheName, WebCore::DOMCacheEngine::CacheIdentifierCallback&&);
    void remove(uint64_t cacheIdentifier, WebCore::DOMCacheEngine::CacheIdentifierCallback&&);
    void retrieveCaches(const WebCore::ClientOrigin&, uint64_t updateCounter, WebCore::DOMCacheEngine::CacheInfosCallback&&);

    void retrieveRecords(uint64_t cacheIdentifier, WebCore::URL&&, WebCore::DOMCacheEngine::RecordsCallback&&);
    void putRecords(uint64_t cacheIdentifier, Vector<WebCore::DOMCacheEngine::Record>&&, WebCore::DOMCacheEngine::RecordIdentifiersCallback&&);
    void deleteMatchingRecords(uint64_t cacheIdentifier, WebCore::ResourceRequest&&, WebCore::CacheQueryOptions&&, WebCore::DOMCacheEngine::RecordIdentifiersCallback&&);

    void lock(uint64_t cacheIdentifier);
    void unlock(uint64_t cacheIdentifier);

    void writeFile(const String& filename, NetworkCache::Data&&, WebCore::DOMCacheEngine::CompletionCallback&&);
    void readFile(const String& filename, WTF::Function<void(const NetworkCache::Data&, int error)>&&);
    void removeFile(const String& filename);

    const String& rootPath() const { return m_rootPath; }
    const NetworkCache::Salt& salt() const { return m_salt.value(); }
    uint64_t nextCacheIdentifier() { return ++m_nextCacheIdentifier; }

    void clearMemoryRepresentation(const WebCore::ClientOrigin&, WebCore::DOMCacheEngine::CompletionCallback&&);
    String representation();

    void clearAllCaches(WTF::CallbackAggregator&);
    void clearCachesForOrigin(const WebCore::SecurityOriginData&, WTF::CallbackAggregator&);

private:
    static Engine& defaultEngine();
    explicit Engine(String&& rootPath);

    String cachesRootPath(const WebCore::ClientOrigin&);

    void fetchEntries(bool /* shouldComputeSize */, WTF::CompletionHandler<void(Vector<WebsiteData::Entry>)>&&);

    void initialize(WTF::Function<void(std::optional<WebCore::DOMCacheEngine::Error>&&)>&&);

    using CachesOrError = Expected<std::reference_wrapper<Caches>, WebCore::DOMCacheEngine::Error>;
    using CachesCallback = WTF::Function<void(CachesOrError&&)>;
    void readCachesFromDisk(const WebCore::ClientOrigin&, CachesCallback&&);

    using CacheOrError = Expected<std::reference_wrapper<Cache>, WebCore::DOMCacheEngine::Error>;
    using CacheCallback = WTF::Function<void(CacheOrError&&)>;
    void readCache(uint64_t cacheIdentifier, CacheCallback&&);

    Cache* cache(uint64_t cacheIdentifier);

    HashMap<WebCore::ClientOrigin, RefPtr<Caches>> m_caches;
    uint64_t m_nextCacheIdentifier { 0 };
    String m_rootPath;
    RefPtr<WorkQueue> m_ioQueue;
    std::optional<NetworkCache::Salt> m_salt;
    HashMap<CacheIdentifier, LockCount> m_cacheLocks;
};

} // namespace CacheStorage

} // namespace WebKit
