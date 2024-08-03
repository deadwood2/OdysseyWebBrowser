/*
 * Copyright (C) 2014-2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NetworkCache.h"

#if ENABLE(NETWORK_CACHE)

#include "Logging.h"
#include "NetworkCacheSpeculativeLoadManager.h"
#include "NetworkCacheStatistics.h"
#include "NetworkCacheStorage.h"
#include <WebCore/CacheValidation.h>
#include <WebCore/FileSystem.h>
#include <WebCore/HTTPHeaderNames.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/PlatformCookieJar.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/SharedBuffer.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RunLoop.h>
#include <wtf/text/StringBuilder.h>

#if PLATFORM(COCOA)
#include <notify.h>
#endif

namespace WebKit {
namespace NetworkCache {

static const AtomicString& resourceType()
{
    ASSERT(WTF::isMainThread());
    static NeverDestroyed<const AtomicString> resource("Resource", AtomicString::ConstructFromLiteral);
    return resource;
}

Cache& singleton()
{
    static NeverDestroyed<Cache> instance;
    return instance;
}

#if PLATFORM(GTK)
static void dumpFileChanged(Cache* cache)
{
    cache->dumpContentsToFile();
}
#endif

bool Cache::initialize(const String& cachePath, const Parameters& parameters)
{
    m_storage = Storage::open(cachePath);

#if ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION)
    if (parameters.enableNetworkCacheSpeculativeRevalidation)
        m_speculativeLoadManager = std::make_unique<SpeculativeLoadManager>(*m_storage);
#endif

    if (parameters.enableEfficacyLogging)
        m_statistics = Statistics::open(cachePath);

#if PLATFORM(COCOA)
    // Triggers with "notifyutil -p com.apple.WebKit.Cache.dump".
    if (m_storage) {
        int token;
        notify_register_dispatch("com.apple.WebKit.Cache.dump", &token, dispatch_get_main_queue(), ^(int) {
            dumpContentsToFile();
        });
    }
#endif
#if PLATFORM(GTK)
    // Triggers with "touch $cachePath/dump".
    if (m_storage) {
        CString dumpFilePath = WebCore::fileSystemRepresentation(WebCore::pathByAppendingComponent(m_storage->basePath(), "dump"));
        GRefPtr<GFile> dumpFile = adoptGRef(g_file_new_for_path(dumpFilePath.data()));
        GFileMonitor* monitor = g_file_monitor_file(dumpFile.get(), G_FILE_MONITOR_NONE, nullptr, nullptr);
        g_signal_connect_swapped(monitor, "changed", G_CALLBACK(dumpFileChanged), this);
    }
#endif

    LOG(NetworkCache, "(NetworkProcess) opened cache storage, success %d", !!m_storage);
    return !!m_storage;
}

void Cache::setCapacity(size_t maximumSize)
{
    if (!m_storage)
        return;
    m_storage->setCapacity(maximumSize);
}

static Key makeCacheKey(const WebCore::ResourceRequest& request)
{
#if ENABLE(CACHE_PARTITIONING)
    String partition = request.cachePartition();
#else
    String partition;
#endif

    // FIXME: This implements minimal Range header disk cache support. We don't parse
    // ranges so only the same exact range request will be served from the cache.
    String range = request.httpHeaderField(WebCore::HTTPHeaderName::Range);
    return { partition, resourceType(), range, request.url().string() };
}

static bool cachePolicyAllowsExpired(WebCore::ResourceRequestCachePolicy policy)
{
    switch (policy) {
    case WebCore::ReturnCacheDataElseLoad:
    case WebCore::ReturnCacheDataDontLoad:
        return true;
    case WebCore::UseProtocolCachePolicy:
    case WebCore::ReloadIgnoringCacheData:
        return false;
    }
    ASSERT_NOT_REACHED();
    return false;
}

static bool responseHasExpired(const WebCore::ResourceResponse& response, std::chrono::system_clock::time_point timestamp, Optional<std::chrono::microseconds> maxStale)
{
    if (response.cacheControlContainsNoCache())
        return true;

    auto age = WebCore::computeCurrentAge(response, timestamp);
    auto lifetime = WebCore::computeFreshnessLifetimeForHTTPFamily(response, timestamp);

    auto maximumStaleness = maxStale ? maxStale.value() : 0ms;
    bool hasExpired = age - lifetime > maximumStaleness;

#ifndef LOG_DISABLED
    if (hasExpired)
        LOG(NetworkCache, "(NetworkProcess) needsRevalidation hasExpired age=%f lifetime=%f max-stale=%g", age, lifetime, maxStale);
#endif

    return hasExpired;
}

static bool responseNeedsRevalidation(const WebCore::ResourceResponse& response, const WebCore::ResourceRequest& request, std::chrono::system_clock::time_point timestamp)
{
    auto requestDirectives = WebCore::parseCacheControlDirectives(request.httpHeaderFields());
    if (requestDirectives.noCache)
        return true;
    // For requests we ignore max-age values other than zero.
    if (requestDirectives.maxAge && requestDirectives.maxAge.value() == 0ms)
        return true;

    return responseHasExpired(response, timestamp, requestDirectives.maxStale);
}

static UseDecision makeUseDecision(const Entry& entry, const WebCore::ResourceRequest& request)
{
    // The request is conditional so we force revalidation from the network. We merely check the disk cache
    // so we can update the cache entry.
    if (request.isConditional() && !entry.redirectRequest())
        return UseDecision::Validate;

    if (!WebCore::verifyVaryingRequestHeaders(entry.varyingRequestHeaders(), request))
        return UseDecision::NoDueToVaryingHeaderMismatch;

    // We never revalidate in the case of a history navigation.
    if (cachePolicyAllowsExpired(request.cachePolicy()))
        return UseDecision::Use;

    if (!responseNeedsRevalidation(entry.response(), request, entry.timeStamp()))
        return UseDecision::Use;

    if (!entry.response().hasCacheValidatorFields())
        return UseDecision::NoDueToMissingValidatorFields;

    return entry.redirectRequest() ? UseDecision::NoDueToExpiredRedirect : UseDecision::Validate;
}

static RetrieveDecision makeRetrieveDecision(const WebCore::ResourceRequest& request)
{
    // FIXME: Support HEAD requests.
    if (request.httpMethod() != "GET")
        return RetrieveDecision::NoDueToHTTPMethod;
    if (request.requester() == WebCore::ResourceRequest::Requester::Media)
        return RetrieveDecision::NoDueToStreamingMedia;
    if (request.cachePolicy() == WebCore::ReloadIgnoringCacheData && !request.isConditional())
        return RetrieveDecision::NoDueToReloadIgnoringCache;

    return RetrieveDecision::Yes;
}

// http://tools.ietf.org/html/rfc7231#page-48
static bool isStatusCodeCacheableByDefault(int statusCode)
{
    switch (statusCode) {
    case 200: // OK
    case 203: // Non-Authoritative Information
    case 204: // No Content
    case 206: // Partial Content
    case 300: // Multiple Choices
    case 301: // Moved Permanently
    case 404: // Not Found
    case 405: // Method Not Allowed
    case 410: // Gone
    case 414: // URI Too Long
    case 501: // Not Implemented
        return true;
    default:
        return false;
    }
}

static bool isStatusCodePotentiallyCacheable(int statusCode)
{
    switch (statusCode) {
    case 201: // Created
    case 202: // Accepted
    case 205: // Reset Content
    case 302: // Found
    case 303: // See Other
    case 307: // Temporary redirect
    case 403: // Forbidden
    case 406: // Not Acceptable
    case 415: // Unsupported Media Type
        return true;
    default:
        return false;
    }
}

static bool isMediaMIMEType(const String& mimeType)
{
    if (mimeType.startsWith("video/", /*caseSensitive*/ false))
        return true;
    if (mimeType.startsWith("audio/", /*caseSensitive*/ false))
        return true;
    return false;
}

static StoreDecision makeStoreDecision(const WebCore::ResourceRequest& originalRequest, const WebCore::ResourceResponse& response)
{
    if (!originalRequest.url().protocolIsInHTTPFamily() || !response.isHTTP())
        return StoreDecision::NoDueToProtocol;

    if (originalRequest.httpMethod() != "GET")
        return StoreDecision::NoDueToHTTPMethod;

    auto requestDirectives = WebCore::parseCacheControlDirectives(originalRequest.httpHeaderFields());
    if (requestDirectives.noStore)
        return StoreDecision::NoDueToNoStoreRequest;

    if (response.cacheControlContainsNoStore())
        return StoreDecision::NoDueToNoStoreResponse;

    if (!isStatusCodeCacheableByDefault(response.httpStatusCode())) {
        // http://tools.ietf.org/html/rfc7234#section-4.3.2
        bool hasExpirationHeaders = response.expires() || response.cacheControlMaxAge();
        bool expirationHeadersAllowCaching = isStatusCodePotentiallyCacheable(response.httpStatusCode()) && hasExpirationHeaders;
        if (!expirationHeadersAllowCaching)
            return StoreDecision::NoDueToHTTPStatusCode;
    }

    bool isMainResource = originalRequest.requester() == WebCore::ResourceRequest::Requester::Main;
    bool storeUnconditionallyForHistoryNavigation = isMainResource || originalRequest.priority() == WebCore::ResourceLoadPriority::VeryHigh;
    if (!storeUnconditionallyForHistoryNavigation) {
        auto now = std::chrono::system_clock::now();
        bool hasNonZeroLifetime = !response.cacheControlContainsNoCache() && WebCore::computeFreshnessLifetimeForHTTPFamily(response, now) > 0ms;

        bool possiblyReusable = response.hasCacheValidatorFields() || hasNonZeroLifetime;
        if (!possiblyReusable)
            return StoreDecision::NoDueToUnlikelyToReuse;
    }

    // Media loaded via XHR is likely being used for MSE streaming (YouTube and Netflix for example).
    // Streaming media fills the cache quickly and is unlikely to be reused.
    // FIXME: We should introduce a separate media cache partition that doesn't affect other resources.
    // FIXME: We should also make sure make the MSE paths are copy-free so we can use mapped buffers from disk effectively.
    auto requester = originalRequest.requester();
    bool isDefinitelyStreamingMedia = requester == WebCore::ResourceRequest::Requester::Media;
    bool isLikelyStreamingMedia = requester == WebCore::ResourceRequest::Requester::XHR && isMediaMIMEType(response.mimeType());
    if (isLikelyStreamingMedia || isDefinitelyStreamingMedia)
        return StoreDecision::NoDueToStreamingMedia;

    return StoreDecision::Yes;
}

void Cache::retrieve(const WebCore::ResourceRequest& request, const GlobalFrameID& frameID, std::function<void (std::unique_ptr<Entry>)>&& completionHandler)
{
    ASSERT(isEnabled());
    ASSERT(request.url().protocolIsInHTTPFamily());

    LOG(NetworkCache, "(NetworkProcess) retrieving %s priority %d", request.url().string().ascii().data(), static_cast<int>(request.priority()));

    if (m_statistics)
        m_statistics->recordRetrievalRequest(frameID.first);

    Key storageKey = makeCacheKey(request);

#if ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION)
    bool canUseSpeculativeRevalidation = m_speculativeLoadManager && !request.isConditional();
    if (canUseSpeculativeRevalidation)
        m_speculativeLoadManager->registerLoad(frameID, request, storageKey);
#endif

    auto retrieveDecision = makeRetrieveDecision(request);
    if (retrieveDecision != RetrieveDecision::Yes) {
        if (m_statistics)
            m_statistics->recordNotUsingCacheForRequest(frameID.first, storageKey, request, retrieveDecision);

        completionHandler(nullptr);
        return;
    }

#if ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION)
    if (canUseSpeculativeRevalidation && m_speculativeLoadManager->retrieve(frameID, storageKey, request, [request, completionHandler](std::unique_ptr<Entry> entry) {
        if (entry && WebCore::verifyVaryingRequestHeaders(entry->varyingRequestHeaders(), request))
            completionHandler(WTFMove(entry));
        else
            completionHandler(nullptr);
    }))
        return;
#endif

    auto startTime = std::chrono::system_clock::now();
    auto priority = static_cast<unsigned>(request.priority());

    m_storage->retrieve(storageKey, priority, [this, request, completionHandler = WTFMove(completionHandler), startTime, storageKey, frameID](std::unique_ptr<Storage::Record> record) {
        if (!record) {
            LOG(NetworkCache, "(NetworkProcess) not found in storage");

            if (m_statistics)
                m_statistics->recordRetrievalFailure(frameID.first, storageKey, request);

            completionHandler(nullptr);
            return false;
        }

        ASSERT(record->key == storageKey);

        auto entry = Entry::decodeStorageRecord(*record);

        auto useDecision = entry ? makeUseDecision(*entry, request) : UseDecision::NoDueToDecodeFailure;
        switch (useDecision) {
        case UseDecision::Use:
            break;
        case UseDecision::Validate:
            entry->setNeedsValidation(true);
            break;
        default:
            entry = nullptr;
        };

#if !LOG_DISABLED
        auto elapsedMS = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime).count());
        LOG(NetworkCache, "(NetworkProcess) retrieve complete useDecision=%d priority=%d time=%" PRIi64 "ms", static_cast<int>(useDecision), static_cast<int>(request.priority()), elapsedMS);
#endif
        completionHandler(WTFMove(entry));

        if (m_statistics)
            m_statistics->recordRetrievedCachedEntry(frameID.first, storageKey, request, useDecision);
        return useDecision != UseDecision::NoDueToDecodeFailure;
    });
}

std::unique_ptr<Entry> Cache::store(const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response, RefPtr<WebCore::SharedBuffer>&& responseData, Function<void (MappedBody&)>&& completionHandler)
{
    ASSERT(isEnabled());
    ASSERT(responseData);

    LOG(NetworkCache, "(NetworkProcess) storing %s, partition %s", request.url().string().latin1().data(), makeCacheKey(request).partition().latin1().data());

    StoreDecision storeDecision = makeStoreDecision(request, response);
    if (storeDecision != StoreDecision::Yes) {
        LOG(NetworkCache, "(NetworkProcess) didn't store, storeDecision=%d", static_cast<int>(storeDecision));
        auto key = makeCacheKey(request);

        auto isSuccessfulRevalidation = response.httpStatusCode() == 304;
        if (!isSuccessfulRevalidation) {
            // Make sure we don't keep a stale entry in the cache.
            remove(key);
        }

        if (m_statistics)
            m_statistics->recordNotCachingResponse(key, storeDecision);

        return nullptr;
    }

    auto cacheEntry = std::make_unique<Entry>(makeCacheKey(request), response, WTFMove(responseData), WebCore::collectVaryingRequestHeaders(request, response));
    auto record = cacheEntry->encodeAsStorageRecord();

    m_storage->store(record, [completionHandler = WTFMove(completionHandler)](const Data& bodyData) {
        MappedBody mappedBody;
#if ENABLE(SHAREABLE_RESOURCE)
        if (auto sharedMemory = bodyData.tryCreateSharedMemory()) {
            mappedBody.shareableResource = ShareableResource::create(sharedMemory.releaseNonNull(), 0, bodyData.size());
            ASSERT(mappedBody.shareableResource);
            mappedBody.shareableResource->createHandle(mappedBody.shareableResourceHandle);
        }
#endif
        completionHandler(mappedBody);
        LOG(NetworkCache, "(NetworkProcess) stored");
    });

    return cacheEntry;
}

std::unique_ptr<Entry> Cache::storeRedirect(const WebCore::ResourceRequest& request, const WebCore::ResourceResponse& response, const WebCore::ResourceRequest& redirectRequest)
{
    ASSERT(isEnabled());

    LOG(NetworkCache, "(NetworkProcess) storing redirect %s -> %s", request.url().string().latin1().data(), redirectRequest.url().string().latin1().data());

    StoreDecision storeDecision = makeStoreDecision(request, response);
    if (storeDecision != StoreDecision::Yes) {
        LOG(NetworkCache, "(NetworkProcess) didn't store redirect, storeDecision=%d", static_cast<int>(storeDecision));
        auto key = makeCacheKey(request);
        if (m_statistics)
            m_statistics->recordNotCachingResponse(key, storeDecision);

        return nullptr;
    }

    std::unique_ptr<Entry> cacheEntry = std::make_unique<Entry>(makeCacheKey(request), response, redirectRequest, WebCore::collectVaryingRequestHeaders(request, response));

    auto record = cacheEntry->encodeAsStorageRecord();

    m_storage->store(record, nullptr);
    
    return cacheEntry;
}

std::unique_ptr<Entry> Cache::update(const WebCore::ResourceRequest& originalRequest, const GlobalFrameID& frameID, const Entry& existingEntry, const WebCore::ResourceResponse& validatingResponse)
{
    LOG(NetworkCache, "(NetworkProcess) updating %s", originalRequest.url().string().latin1().data());

    WebCore::ResourceResponse response = existingEntry.response();
    WebCore::updateResponseHeadersAfterRevalidation(response, validatingResponse);

    auto updateEntry = std::make_unique<Entry>(existingEntry.key(), response, existingEntry.buffer(), WebCore::collectVaryingRequestHeaders(originalRequest, response));
    auto updateRecord = updateEntry->encodeAsStorageRecord();

    m_storage->store(updateRecord, { });

    if (m_statistics)
        m_statistics->recordRevalidationSuccess(frameID.first, existingEntry.key(), originalRequest);

    return updateEntry;
}

void Cache::remove(const Key& key)
{
    ASSERT(isEnabled());

    m_storage->remove(key);
}

void Cache::remove(const WebCore::ResourceRequest& request)
{
    remove(makeCacheKey(request));
}

void Cache::traverse(Function<void (const TraversalEntry*)>&& traverseHandler)
{
    ASSERT(isEnabled());

    // Protect against clients making excessive traversal requests.
    const unsigned maximumTraverseCount = 3;
    if (m_traverseCount >= maximumTraverseCount) {
        WTFLogAlways("Maximum parallel cache traverse count exceeded. Ignoring traversal request.");

        RunLoop::main().dispatch([traverseHandler = WTFMove(traverseHandler)] {
            traverseHandler(nullptr);
        });
        return;
    }

    ++m_traverseCount;

    m_storage->traverse(resourceType(), 0, [this, traverseHandler = WTFMove(traverseHandler)](const Storage::Record* record, const Storage::RecordInfo& recordInfo) {
        if (!record) {
            --m_traverseCount;
            traverseHandler(nullptr);
            return;
        }

        auto entry = Entry::decodeStorageRecord(*record);
        if (!entry)
            return;

        TraversalEntry traversalEntry { *entry, recordInfo };
        traverseHandler(&traversalEntry);
    });
}

String Cache::dumpFilePath() const
{
    return WebCore::pathByAppendingComponent(m_storage->versionPath(), "dump.json");
}

void Cache::dumpContentsToFile()
{
    if (!m_storage)
        return;
    auto fd = WebCore::openFile(dumpFilePath(), WebCore::OpenForWrite);
    if (!WebCore::isHandleValid(fd))
        return;
    auto prologue = String("{\n\"entries\": [\n").utf8();
    WebCore::writeToFile(fd, prologue.data(), prologue.length());

    struct Totals {
        unsigned count { 0 };
        double worth { 0 };
        size_t bodySize { 0 };
    };
    Totals totals;
    auto flags = Storage::TraverseFlag::ComputeWorth | Storage::TraverseFlag::ShareCount;
    size_t capacity = m_storage->capacity();
    m_storage->traverse(resourceType(), flags, [fd, totals, capacity](const Storage::Record* record, const Storage::RecordInfo& info) mutable {
        if (!record) {
            StringBuilder epilogue;
            epilogue.appendLiteral("{}\n],\n");
            epilogue.appendLiteral("\"totals\": {\n");
            epilogue.appendLiteral("\"capacity\": ");
            epilogue.appendNumber(capacity);
            epilogue.appendLiteral(",\n");
            epilogue.appendLiteral("\"count\": ");
            epilogue.appendNumber(totals.count);
            epilogue.appendLiteral(",\n");
            epilogue.appendLiteral("\"bodySize\": ");
            epilogue.appendNumber(totals.bodySize);
            epilogue.appendLiteral(",\n");
            epilogue.appendLiteral("\"averageWorth\": ");
            epilogue.appendNumber(totals.count ? totals.worth / totals.count : 0);
            epilogue.appendLiteral("\n");
            epilogue.appendLiteral("}\n}\n");
            auto writeData = epilogue.toString().utf8();
            WebCore::writeToFile(fd, writeData.data(), writeData.length());
            WebCore::closeFile(fd);
            return;
        }
        auto entry = Entry::decodeStorageRecord(*record);
        if (!entry)
            return;
        ++totals.count;
        totals.worth += info.worth;
        totals.bodySize += info.bodySize;

        StringBuilder json;
        entry->asJSON(json, info);
        json.appendLiteral(",\n");
        auto writeData = json.toString().utf8();
        WebCore::writeToFile(fd, writeData.data(), writeData.length());
    });
}

void Cache::deleteDumpFile()
{
    WorkQueue::create("com.apple.WebKit.Cache.delete")->dispatch([path = dumpFilePath().isolatedCopy()] {
        WebCore::deleteFile(path);
    });
}

void Cache::clear(std::chrono::system_clock::time_point modifiedSince, std::function<void ()>&& completionHandler)
{
    LOG(NetworkCache, "(NetworkProcess) clearing cache");

    if (m_statistics)
        m_statistics->clear();

    if (!m_storage) {
        RunLoop::main().dispatch(WTFMove(completionHandler));
        return;
    }
    String anyType;
    m_storage->clear(anyType, modifiedSince, WTFMove(completionHandler));

    deleteDumpFile();
}

void Cache::clear()
{
    clear(std::chrono::system_clock::time_point::min(), nullptr);
}

String Cache::recordsPath() const
{
    return m_storage ? m_storage->recordsPath() : String();
}

}
}

#endif
