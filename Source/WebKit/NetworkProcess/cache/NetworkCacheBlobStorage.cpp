/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "NetworkCacheBlobStorage.h"

#include "Logging.h"
#include "NetworkCacheFileSystem.h"
#include <WebCore/FileSystem.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wtf/RunLoop.h>
#include <wtf/SHA1.h>

namespace WebKit {
namespace NetworkCache {

BlobStorage::BlobStorage(const String& blobDirectoryPath, Salt salt)
    : m_blobDirectoryPath(blobDirectoryPath)
    , m_salt(salt)
{
}

String BlobStorage::blobDirectoryPath() const
{
    return m_blobDirectoryPath.isolatedCopy();
}

void BlobStorage::synchronize()
{
    ASSERT(!RunLoop::isMain());

    WebCore::FileSystem::makeAllDirectories(blobDirectoryPath());

    m_approximateSize = 0;
    auto blobDirectory = blobDirectoryPath();
    traverseDirectory(blobDirectory, [this, &blobDirectory](const String& name, DirectoryEntryType type) {
        if (type != DirectoryEntryType::File)
            return;
        auto path = WebCore::FileSystem::pathByAppendingComponent(blobDirectory, name);
        auto filePath = WebCore::FileSystem::fileSystemRepresentation(path);
        struct stat stat;
        ::stat(filePath.data(), &stat);
        // No clients left for this blob.
        if (stat.st_nlink == 1)
            unlink(filePath.data());
        else
            m_approximateSize += stat.st_size;
    });

    LOG(NetworkCacheStorage, "(NetworkProcess) blob synchronization completed approximateSize=%zu", approximateSize());
}

String BlobStorage::blobPathForHash(const SHA1::Digest& hash) const
{
    auto hashAsString = SHA1::hexDigest(hash);
    return WebCore::FileSystem::pathByAppendingComponent(blobDirectoryPath(), String::fromUTF8(hashAsString));
}

BlobStorage::Blob BlobStorage::add(const String& path, const Data& data)
{
    ASSERT(!RunLoop::isMain());

    auto hash = computeSHA1(data, m_salt);
    if (data.isEmpty())
        return { data, hash };

    auto blobPath = WebCore::FileSystem::fileSystemRepresentation(blobPathForHash(hash));
    auto linkPath = WebCore::FileSystem::fileSystemRepresentation(path);
    unlink(linkPath.data());

    bool blobExists = access(blobPath.data(), F_OK) != -1;
    if (blobExists) {
        auto existingData = mapFile(blobPath.data());
        if (bytesEqual(existingData, data)) {
            if (link(blobPath.data(), linkPath.data()) == -1)
                WTFLogAlways("Failed to create hard link from %s to %s", blobPath.data(), linkPath.data());
            return { existingData, hash };
        }
        unlink(blobPath.data());
    }

    auto mappedData = data.mapToFile(blobPath.data());
    if (mappedData.isNull())
        return { };

    if (link(blobPath.data(), linkPath.data()) == -1)
        WTFLogAlways("Failed to create hard link from %s to %s", blobPath.data(), linkPath.data());

    m_approximateSize += mappedData.size();

    return { mappedData, hash };
}

BlobStorage::Blob BlobStorage::get(const String& path)
{
    ASSERT(!RunLoop::isMain());

    auto linkPath = WebCore::FileSystem::fileSystemRepresentation(path);
    auto data = mapFile(linkPath.data());

    return { data, computeSHA1(data, m_salt) };
}

void BlobStorage::remove(const String& path)
{
    ASSERT(!RunLoop::isMain());

    auto linkPath = WebCore::FileSystem::fileSystemRepresentation(path);
    unlink(linkPath.data());
}

unsigned BlobStorage::shareCount(const String& path)
{
    ASSERT(!RunLoop::isMain());

    auto linkPath = WebCore::FileSystem::fileSystemRepresentation(path);
    struct stat stat;
    if (::stat(linkPath.data(), &stat) < 0)
        return 0;
    // Link count is 2 in the single client case (the blob file and a link).
    return stat.st_nlink - 1;
}

}
}
