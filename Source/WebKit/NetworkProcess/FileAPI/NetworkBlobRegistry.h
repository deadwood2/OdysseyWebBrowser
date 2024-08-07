/*
 * Copyright (C) 2013-2016 Apple Inc. All rights reserved.
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

#include <WebCore/URLHash.h>
#include <wtf/Function.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>

namespace WebCore {
class BlobDataFileReference;
class BlobPart;
}

namespace WebKit {

class NetworkConnectionToWebProcess;
class SandboxExtension;

class NetworkBlobRegistry {
WTF_MAKE_NONCOPYABLE(NetworkBlobRegistry);
public:
    NetworkBlobRegistry();
    static NetworkBlobRegistry& singleton();

    void registerFileBlobURL(NetworkConnectionToWebProcess*, const WebCore::URL&, const String& path, RefPtr<SandboxExtension>&&, const String& contentType);
    void registerBlobURL(NetworkConnectionToWebProcess*, const WebCore::URL&, Vector<WebCore::BlobPart>&&, const String& contentType);
    void registerBlobURL(NetworkConnectionToWebProcess*, const WebCore::URL&, const WebCore::URL& srcURL, bool shouldBypassConnectionCheck);
    void registerBlobURLOptionallyFileBacked(NetworkConnectionToWebProcess*, const WebCore::URL&, const WebCore::URL& srcURL, const String& fileBackedPath, const String& contentType);
    void registerBlobURLForSlice(NetworkConnectionToWebProcess*, const WebCore::URL&, const WebCore::URL& srcURL, int64_t start, int64_t end);
    void unregisterBlobURL(NetworkConnectionToWebProcess*, const WebCore::URL&);
    uint64_t blobSize(NetworkConnectionToWebProcess*, const WebCore::URL&);
    void writeBlobsToTemporaryFiles(const Vector<String>& blobURLs, Function<void (const Vector<String>&)>&& completionHandler);
    void writeBlobToFilePath(const WebCore::URL& blobURL, const String& path, Function<void(bool success)>&& completionHandler);

    void connectionToWebProcessDidClose(NetworkConnectionToWebProcess*);

    Vector<RefPtr<WebCore::BlobDataFileReference>> filesInBlob(NetworkConnectionToWebProcess&, const WebCore::URL&);
    Vector<RefPtr<WebCore::BlobDataFileReference>> filesInBlob(const WebCore::URL&);

private:
    ~NetworkBlobRegistry();

    typedef HashMap<NetworkConnectionToWebProcess*, HashSet<WebCore::URL>> BlobForConnectionMap;
    BlobForConnectionMap m_blobsForConnection;
};

}
