/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Portions Copyright (c) 2013 Company 100 Inc. All rights reserved.
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
#include "DataReference.h"
#include "WebCoreArgumentCoders.h"

#include <WebCore/CertificateInfo.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace IPC {

void ArgumentCoder<ResourceRequest>::encodePlatformData(Encoder& encoder, const ResourceRequest& resourceRequest)
{
    encoder << resourceRequest.url().string();
    encoder << resourceRequest.httpMethod();
    encoder << resourceRequest.httpHeaderFields();
    encoder << resourceRequest.timeoutInterval();

    // FIXME: Do not encode HTTP message body.
    // 1. It can be large and thus costly to send across.
    // 2. It is misleading to provide a body with some requests, while others use body streams, which cannot be serialized at all.
    FormData* httpBody = resourceRequest.httpBody();
    encoder << static_cast<bool>(httpBody);
    if (httpBody)
        encoder << httpBody->flattenToString();

    encoder << resourceRequest.firstPartyForCookies().string();
    encoder << resourceRequest.allowCookies();
    encoder.encodeEnum(resourceRequest.priority());
    encoder.encodeEnum(resourceRequest.cachePolicy());
    encoder.encodeEnum(resourceRequest.requester());

    encoder << static_cast<uint32_t>(resourceRequest.soupMessageFlags());
    encoder << resourceRequest.initiatingPageID();
}

bool ArgumentCoder<ResourceRequest>::decodePlatformData(Decoder& decoder, ResourceRequest& resourceRequest)
{
    String url;
    if (!decoder.decode(url))
        return false;
    resourceRequest.setURL(URL(URL(), url));

    String httpMethod;
    if (!decoder.decode(httpMethod))
        return false;
    resourceRequest.setHTTPMethod(httpMethod);

    HTTPHeaderMap headers;
    if (!decoder.decode(headers))
        return false;
    resourceRequest.setHTTPHeaderFields(WTFMove(headers));

    double timeoutInterval;
    if (!decoder.decode(timeoutInterval))
        return false;
    resourceRequest.setTimeoutInterval(timeoutInterval);

    bool hasHTTPBody;
    if (!decoder.decode(hasHTTPBody))
        return false;
    if (hasHTTPBody) {
        String httpBody;
        if (!decoder.decode(httpBody))
            return false;
        resourceRequest.setHTTPBody(FormData::create(httpBody.utf8()));
    }

    String firstPartyForCookies;
    if (!decoder.decode(firstPartyForCookies))
        return false;
    resourceRequest.setFirstPartyForCookies(URL(URL(), firstPartyForCookies));

    bool allowCookies;
    if (!decoder.decode(allowCookies))
        return false;
    resourceRequest.setAllowCookies(allowCookies);

    ResourceLoadPriority priority;
    if (!decoder.decodeEnum(priority))
        return false;
    resourceRequest.setPriority(priority);

    ResourceRequestCachePolicy cachePolicy;
    if (!decoder.decodeEnum(cachePolicy))
        return false;
    resourceRequest.setCachePolicy(cachePolicy);

    ResourceRequest::Requester requester;
    if (!decoder.decodeEnum(requester))
        return false;
    resourceRequest.setRequester(requester);

    uint32_t soupMessageFlags;
    if (!decoder.decode(soupMessageFlags))
        return false;
    resourceRequest.setSoupMessageFlags(static_cast<SoupMessageFlags>(soupMessageFlags));

    uint64_t initiatingPageID;
    if (!decoder.decode(initiatingPageID))
        return false;
    resourceRequest.setInitiatingPageID(initiatingPageID);

    return true;
}

void ArgumentCoder<CertificateInfo>::encode(Encoder& encoder, const CertificateInfo& certificateInfo)
{
    if (!certificateInfo.certificate()) {
        encoder << false;
        return;
    }

    GByteArray* certificateData = 0;
    g_object_get(G_OBJECT(certificateInfo.certificate()), "certificate", &certificateData, NULL);
    if (!certificateData) {
        encoder << false;
        return;
    }

    encoder << true;
    GRefPtr<GByteArray> certificate = adoptGRef(certificateData);
    encoder.encodeVariableLengthByteArray(IPC::DataReference(certificate->data, certificate->len));
    encoder << static_cast<uint32_t>(certificateInfo.tlsErrors());
}

bool ArgumentCoder<CertificateInfo>::decode(Decoder& decoder, CertificateInfo& certificateInfo)
{
    bool hasCertificate;
    if (!decoder.decode(hasCertificate))
        return false;

    if (!hasCertificate)
        return true;

    IPC::DataReference certificateDataReference;
    if (!decoder.decodeVariableLengthByteArray(certificateDataReference))
        return false;

    GByteArray* certificateData = g_byte_array_sized_new(certificateDataReference.size());
    certificateData = g_byte_array_append(certificateData, certificateDataReference.data(), certificateDataReference.size());
    GRefPtr<GByteArray> certificateBytes = adoptGRef(certificateData);

    GTlsBackend* backend = g_tls_backend_get_default();
    GRefPtr<GTlsCertificate> certificate = adoptGRef(G_TLS_CERTIFICATE(g_initable_new(
        g_tls_backend_get_certificate_type(backend), 0, 0, "certificate", certificateBytes.get(), nullptr)));
    certificateInfo.setCertificate(certificate.get());

    uint32_t tlsErrors;
    if (!decoder.decode(tlsErrors))
        return false;
    certificateInfo.setTLSErrors(static_cast<GTlsCertificateFlags>(tlsErrors));

    return true;
}

void ArgumentCoder<ResourceError>::encodePlatformData(Encoder& encoder, const ResourceError& resourceError)
{
    encoder.encodeEnum(resourceError.type());
    if (resourceError.isNull())
        return;

    encoder << resourceError.domain();
    encoder << resourceError.errorCode();
    encoder << resourceError.failingURL().string();
    encoder << resourceError.localizedDescription();

    encoder << CertificateInfo(resourceError);
}

bool ArgumentCoder<ResourceError>::decodePlatformData(Decoder& decoder, ResourceError& resourceError)
{
    ResourceErrorBase::Type errorType;
    if (!decoder.decodeEnum(errorType))
        return false;
    if (errorType == ResourceErrorBase::Type::Null) {
        resourceError = { };
        return true;
    }

    String domain;
    if (!decoder.decode(domain))
        return false;

    int errorCode;
    if (!decoder.decode(errorCode))
        return false;

    String failingURL;
    if (!decoder.decode(failingURL))
        return false;

    String localizedDescription;
    if (!decoder.decode(localizedDescription))
        return false;

    resourceError = ResourceError(domain, errorCode, URL(URL(), failingURL), localizedDescription);
    resourceError.setType(errorType);

    CertificateInfo certificateInfo;
    if (!decoder.decode(certificateInfo))
        return false;

    resourceError.setCertificate(certificateInfo.certificate());
    resourceError.setTLSErrors(certificateInfo.tlsErrors());
    return true;
}

void ArgumentCoder<ProtectionSpace>::encodePlatformData(Encoder&, const ProtectionSpace&)
{
    ASSERT_NOT_REACHED();
}

bool ArgumentCoder<ProtectionSpace>::decodePlatformData(Decoder&, ProtectionSpace&)
{
    ASSERT_NOT_REACHED();
    return false;
}

void ArgumentCoder<Credential>::encodePlatformData(Encoder&, const Credential&)
{
    ASSERT_NOT_REACHED();
}

bool ArgumentCoder<Credential>::decodePlatformData(Decoder&, Credential&)
{
    ASSERT_NOT_REACHED();
    return false;
}

}

