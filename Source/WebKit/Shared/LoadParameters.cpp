/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "LoadParameters.h"

#include "WebCoreArgumentCoders.h"

namespace WebKit {

void LoadParameters::encode(IPC::Encoder& encoder) const
{
    encoder << navigationID;
    encoder << request;

    encoder << static_cast<bool>(request.httpBody());
    if (request.httpBody())
        request.httpBody()->encode(encoder);

    encoder << sandboxExtensionHandle;
    encoder << data;
    encoder << string;
    encoder << MIMEType;
    encoder << encodingName;
    encoder << baseURLString;
    encoder << unreachableURLString;
    encoder << provisionalLoadErrorURLString;
    encoder << shouldOpenExternalURLsPolicy;
    encoder << userData;

    platformEncode(encoder);
}

bool LoadParameters::decode(IPC::Decoder& decoder, LoadParameters& data)
{
    if (!decoder.decode(data.navigationID))
        return false;

    if (!decoder.decode(data.request))
        return false;

    bool hasHTTPBody;
    if (!decoder.decode(hasHTTPBody))
        return false;

    if (hasHTTPBody) {
        RefPtr<WebCore::FormData> formData = WebCore::FormData::decode(decoder);
        if (!formData)
            return false;
        data.request.setHTTPBody(WTFMove(formData));
    }

    std::optional<SandboxExtension::Handle> sandboxExtensionHandle;
    decoder >> sandboxExtensionHandle;
    if (!sandboxExtensionHandle)
        return false;
    data.sandboxExtensionHandle = WTFMove(*sandboxExtensionHandle);

    if (!decoder.decode(data.data))
        return false;

    if (!decoder.decode(data.string))
        return false;

    if (!decoder.decode(data.MIMEType))
        return false;

    if (!decoder.decode(data.encodingName))
        return false;

    if (!decoder.decode(data.baseURLString))
        return false;

    if (!decoder.decode(data.unreachableURLString))
        return false;

    if (!decoder.decode(data.provisionalLoadErrorURLString))
        return false;

    if (!decoder.decode(data.shouldOpenExternalURLsPolicy))
        return false;

    if (!decoder.decode(data.userData))
        return false;

    if (!platformDecode(decoder, data))
        return false;

    return true;
}

#if !PLATFORM(COCOA)

void LoadParameters::platformEncode(IPC::Encoder&) const
{
}

bool LoadParameters::platformDecode(IPC::Decoder&, LoadParameters&)
{
    return true;
}

#endif // !PLATFORM(COCOA)


} // namespace WebKit
