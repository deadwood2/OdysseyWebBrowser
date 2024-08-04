/*
 * Copyright (C) 2015, 2016 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(WEB_RTC)

#include "ExceptionOr.h"
#include "RTCSessionDescription.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class MediaEndpointSessionConfiguration;
class SDPProcessor;
class DOMError;

class MediaEndpointSessionDescription : public RefCounted<MediaEndpointSessionDescription> {
public:
    static Ref<MediaEndpointSessionDescription> create(RTCSessionDescription::SdpType, RefPtr<MediaEndpointSessionConfiguration>&&);
    static ExceptionOr<Ref<MediaEndpointSessionDescription>> create(RefPtr<RTCSessionDescription>&&, const SDPProcessor&);
    virtual ~MediaEndpointSessionDescription() { } // FIXME: Why is this virtual? There are no other virtual functions in this class.

    RefPtr<RTCSessionDescription> toRTCSessionDescription(const SDPProcessor&) const;

    RTCSessionDescription::SdpType type() const { return m_type; }
    const String& typeString() const;
    MediaEndpointSessionConfiguration* configuration() const { return m_configuration.get(); }

    bool isLaterThan(MediaEndpointSessionDescription* other) const;

private:
    MediaEndpointSessionDescription(RTCSessionDescription::SdpType type, RefPtr<MediaEndpointSessionConfiguration>&& configuration, RefPtr<RTCSessionDescription>&& rtcDescription)
        : m_type(type)
        , m_configuration(configuration)
        , m_rtcDescription(WTFMove(rtcDescription))
    { }

    RTCSessionDescription::SdpType m_type;
    RefPtr<MediaEndpointSessionConfiguration> m_configuration;

    RefPtr<RTCSessionDescription> m_rtcDescription;
};

} // namespace WebCore

#endif // ENABLE(WEB_RTC)
