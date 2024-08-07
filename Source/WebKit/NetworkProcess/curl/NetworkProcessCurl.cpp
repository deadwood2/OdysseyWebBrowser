/*
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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
#include "NetworkProcess.h"

#include "NetworkCache.h"
#include "NetworkProcessCreationParameters.h"
#include "ResourceCachesToClear.h"
#include "WebCookieManager.h"
#include <WebCore/CertificateInfo.h>
#include <WebCore/FileSystem.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/ResourceHandle.h>
#include <wtf/RAMSize.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

void NetworkProcess::platformInitializeNetworkProcess(const NetworkProcessCreationParameters& parameters)
{
    notImplemented();
}

void NetworkProcess::platformSetURLCacheSize(unsigned, uint64_t)
{
    notImplemented();
}

void NetworkProcess::allowSpecificHTTPSCertificateForHost(const CertificateInfo& certificateInfo, const String& host)
{
    notImplemented();
}

void NetworkProcess::clearCacheForAllOrigins(uint32_t cachesToClear)
{
    notImplemented();
}

void NetworkProcess::clearDiskCache(WallTime, Function<void()>&&)
{
    notImplemented();
}

void NetworkProcess::platformTerminate()
{
    notImplemented();
}

} // namespace WebKit
