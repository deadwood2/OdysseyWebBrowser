/*
 * Copyright (C) 2012-2017 Apple Inc. All rights reserved.
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
#include "NetworkProcessCreationParameters.h"

#include "ArgumentCoders.h"
#include "WebCoreArgumentCoders.h"

#if PLATFORM(COCOA)
#include "ArgumentCodersCF.h"
#endif

namespace WebKit {

NetworkProcessCreationParameters::NetworkProcessCreationParameters()
{
}

void NetworkProcessCreationParameters::encode(IPC::Encoder& encoder) const
{
    encoder << defaultSessionParameters;
    encoder << privateBrowsingEnabled;
    encoder.encodeEnum(cacheModel);
    encoder << diskCacheSizeOverride;
    encoder << canHandleHTTPSServerTrustEvaluation;
    encoder << cacheStorageDirectory;
    encoder << cacheStoragePerOriginQuota;
    encoder << cacheStorageDirectoryExtensionHandle;
    encoder << diskCacheDirectory;
    encoder << diskCacheDirectoryExtensionHandle;
    encoder << shouldEnableNetworkCache;
    encoder << shouldEnableNetworkCacheEfficacyLogging;
#if ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION)
    encoder << shouldEnableNetworkCacheSpeculativeRevalidation;
#endif
#if PLATFORM(MAC)
    encoder << uiProcessCookieStorageIdentifier;
#endif
#if PLATFORM(IOS)
    encoder << cookieStorageDirectoryExtensionHandle;
    encoder << containerCachesDirectoryExtensionHandle;
    encoder << parentBundleDirectoryExtensionHandle;
#endif
    encoder << shouldSuppressMemoryPressureHandler;
    encoder << shouldUseTestingNetworkSession;
    encoder << loadThrottleLatency;
    encoder << urlSchemesRegisteredForCustomProtocols;
    encoder << presentingApplicationPID;
#if PLATFORM(COCOA)
    encoder << parentProcessName;
    encoder << uiProcessBundleIdentifier;
    encoder << nsURLCacheMemoryCapacity;
    encoder << nsURLCacheDiskCapacity;
    encoder << sourceApplicationBundleIdentifier;
    encoder << sourceApplicationSecondaryIdentifier;
#if PLATFORM(IOS)
    encoder << ctDataConnectionServiceType;
#endif
    encoder << httpProxy;
    encoder << httpsProxy;
#if PLATFORM(COCOA)
    IPC::encode(encoder, networkATSContext.get());
#endif
    encoder << cookieStoragePartitioningEnabled;
    encoder << storageAccessAPIEnabled;
#endif
#if USE(SOUP)
    encoder << cookiePersistentStoragePath;
    encoder << cookiePersistentStorageType;
    encoder.encodeEnum(cookieAcceptPolicy);
    encoder << ignoreTLSErrors;
    encoder << languages;
    encoder << proxySettings;
#endif
#if HAVE(CFNETWORK_STORAGE_PARTITIONING) && !RELEASE_LOG_DISABLED
    encoder << logCookieInformation;
#endif
#if OS(LINUX)
    encoder << memoryPressureMonitorHandle;
#endif
#if ENABLE(NETWORK_CAPTURE)
    encoder << recordReplayMode;
    encoder << recordReplayCacheLocation;
#endif
}

bool NetworkProcessCreationParameters::decode(IPC::Decoder& decoder, NetworkProcessCreationParameters& result)
{
    std::optional<NetworkSessionCreationParameters> defaultSessionParameters;
    decoder >> defaultSessionParameters;
    if (!defaultSessionParameters)
        return false;
    result.defaultSessionParameters = WTFMove(*defaultSessionParameters);

    if (!decoder.decode(result.privateBrowsingEnabled))
        return false;
    if (!decoder.decodeEnum(result.cacheModel))
        return false;
    if (!decoder.decode(result.diskCacheSizeOverride))
        return false;
    if (!decoder.decode(result.canHandleHTTPSServerTrustEvaluation))
        return false;
    if (!decoder.decode(result.cacheStorageDirectory))
        return false;
    if (!decoder.decode(result.cacheStoragePerOriginQuota))
        return false;
    
    std::optional<SandboxExtension::Handle> cacheStorageDirectoryExtensionHandle;
    decoder >> cacheStorageDirectoryExtensionHandle;
    if (!cacheStorageDirectoryExtensionHandle)
        return false;
    result.cacheStorageDirectoryExtensionHandle = WTFMove(*cacheStorageDirectoryExtensionHandle);

    if (!decoder.decode(result.diskCacheDirectory))
        return false;
    
    std::optional<SandboxExtension::Handle> diskCacheDirectoryExtensionHandle;
    decoder >> diskCacheDirectoryExtensionHandle;
    if (!diskCacheDirectoryExtensionHandle)
        return false;
    result.diskCacheDirectoryExtensionHandle = WTFMove(*diskCacheDirectoryExtensionHandle);

    if (!decoder.decode(result.shouldEnableNetworkCache))
        return false;
    if (!decoder.decode(result.shouldEnableNetworkCacheEfficacyLogging))
        return false;
#if ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION)
    if (!decoder.decode(result.shouldEnableNetworkCacheSpeculativeRevalidation))
        return false;
#endif
#if PLATFORM(MAC)
    if (!decoder.decode(result.uiProcessCookieStorageIdentifier))
        return false;
#endif
#if PLATFORM(IOS)
    std::optional<SandboxExtension::Handle> cookieStorageDirectoryExtensionHandle;
    decoder >> cookieStorageDirectoryExtensionHandle;
    if (!cookieStorageDirectoryExtensionHandle)
        return false;
    result.cookieStorageDirectoryExtensionHandle = WTFMove(*cookieStorageDirectoryExtensionHandle);

    std::optional<SandboxExtension::Handle> containerCachesDirectoryExtensionHandle;
    decoder >> containerCachesDirectoryExtensionHandle;
    if (!containerCachesDirectoryExtensionHandle)
        return false;
    result.containerCachesDirectoryExtensionHandle = WTFMove(*containerCachesDirectoryExtensionHandle);

    std::optional<SandboxExtension::Handle> parentBundleDirectoryExtensionHandle;
    decoder >> parentBundleDirectoryExtensionHandle;
    if (!parentBundleDirectoryExtensionHandle)
        return false;
    result.parentBundleDirectoryExtensionHandle = WTFMove(*parentBundleDirectoryExtensionHandle);
#endif
    if (!decoder.decode(result.shouldSuppressMemoryPressureHandler))
        return false;
    if (!decoder.decode(result.shouldUseTestingNetworkSession))
        return false;
    if (!decoder.decode(result.loadThrottleLatency))
        return false;
    if (!decoder.decode(result.urlSchemesRegisteredForCustomProtocols))
        return false;
    if (!decoder.decode(result.presentingApplicationPID))
        return false;
#if PLATFORM(COCOA)
    if (!decoder.decode(result.parentProcessName))
        return false;
    if (!decoder.decode(result.uiProcessBundleIdentifier))
        return false;
    if (!decoder.decode(result.nsURLCacheMemoryCapacity))
        return false;
    if (!decoder.decode(result.nsURLCacheDiskCapacity))
        return false;
    if (!decoder.decode(result.sourceApplicationBundleIdentifier))
        return false;
    if (!decoder.decode(result.sourceApplicationSecondaryIdentifier))
        return false;
#if PLATFORM(IOS)
    if (!decoder.decode(result.ctDataConnectionServiceType))
        return false;
#endif
    if (!decoder.decode(result.httpProxy))
        return false;
    if (!decoder.decode(result.httpsProxy))
        return false;
#if PLATFORM(COCOA)
    if (!IPC::decode(decoder, result.networkATSContext))
        return false;
#endif
    if (!decoder.decode(result.cookieStoragePartitioningEnabled))
        return false;
    if (!decoder.decode(result.storageAccessAPIEnabled))
        return false;
#endif

#if USE(SOUP)
    if (!decoder.decode(result.cookiePersistentStoragePath))
        return false;
    if (!decoder.decode(result.cookiePersistentStorageType))
        return false;
    if (!decoder.decodeEnum(result.cookieAcceptPolicy))
        return false;
    if (!decoder.decode(result.ignoreTLSErrors))
        return false;
    if (!decoder.decode(result.languages))
        return false;
    if (!decoder.decode(result.proxySettings))
        return false;
#endif

#if HAVE(CFNETWORK_STORAGE_PARTITIONING) && !RELEASE_LOG_DISABLED
    if (!decoder.decode(result.logCookieInformation))
        return false;
#endif

#if OS(LINUX)
    if (!decoder.decode(result.memoryPressureMonitorHandle))
        return false;
#endif

#if ENABLE(NETWORK_CAPTURE)
    if (!decoder.decode(result.recordReplayMode))
        return false;
    if (!decoder.decode(result.recordReplayCacheLocation))
        return false;
#endif

    return true;
}

} // namespace WebKit
