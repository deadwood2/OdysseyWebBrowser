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

#include "SandboxExtension.h"
#include <pal/SessionID.h>
#include <wtf/Seconds.h>
#include <wtf/URL.h>
#include <wtf/text/WTFString.h>

#if USE(SOUP)
#include "SoupCookiePersistentStorageType.h"
#endif

#if USE(CURL)
#include <WebCore/CurlProxySettings.h>
#endif

namespace IPC {
class Encoder;
class Decoder;
}

#if PLATFORM(COCOA)
extern "C" CFStringRef const WebKit2HTTPProxyDefaultsKey;
extern "C" CFStringRef const WebKit2HTTPSProxyDefaultsKey;
#endif
    
namespace WebKit {

enum class AllowsCellularAccess : bool { No, Yes };

struct NetworkSessionCreationParameters {
    void encode(IPC::Encoder&) const;
    static Optional<NetworkSessionCreationParameters> decode(IPC::Decoder&);
    static NetworkSessionCreationParameters privateSessionParameters(const PAL::SessionID&);
    
    PAL::SessionID sessionID { PAL::SessionID::defaultSessionID() };
    String boundInterfaceIdentifier;
    AllowsCellularAccess allowsCellularAccess { AllowsCellularAccess::Yes };
#if PLATFORM(COCOA)
    RetainPtr<CFDictionaryRef> proxyConfiguration;
    String sourceApplicationBundleIdentifier;
    String sourceApplicationSecondaryIdentifier;
    bool shouldLogCookieInformation { false };
    Seconds loadThrottleLatency;
    URL httpProxy;
    URL httpsProxy;
#endif
#if USE(SOUP)
    String cookiePersistentStoragePath;
    SoupCookiePersistentStorageType cookiePersistentStorageType { SoupCookiePersistentStorageType::Text };
#endif
#if USE(CURL)
    String cookiePersistentStorageFile;
    WebCore::CurlProxySettings proxySettings;
#endif
    String resourceLoadStatisticsDirectory;
    SandboxExtension::Handle resourceLoadStatisticsDirectoryExtensionHandle;
    bool enableResourceLoadStatistics { false };
};

} // namespace WebKit
