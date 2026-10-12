/*
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
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
#include "CurlSSLHandle.h"

namespace WebCore {

void CurlSSLHandle::platformInitialize()
{
    static String caCertPath = "MOSSYS:Data/SSL/curl-ca-bundle.crt"_s;
    setCACertPath(WTFMove(caCertPath));

#if 1
    constexpr auto cipherList =
        "TLS_CHACHA20_POLY1305_SHA256:"
        "TLS_AES_128_GCM_SHA256:"
        "TLS_AES_256_GCM_SHA384:"
        "ECDHE-ECDSA-CHACHA20-POLY1305:"
        "ECDHE-RSA-CHACHA20-POLY1305:"
        "ECDHE-ECDSA-AES128-GCM-SHA256:"
        "ECDHE-RSA-AES128-GCM-SHA256:"
        "ECDHE-ECDSA-AES256-GCM-SHA384:"
        "ECDHE-RSA-AES256-GCM-SHA384:"
        "ECDHE-ECDSA-AES128-SHA:"
        "ECDHE-ECDSA-AES256-SHA:"
        "ECDHE-RSA-AES128-SHA:"
        "ECDHE-RSA-AES256-SHA:"
        "AES128-GCM-SHA256:"
        "AES256-GCM-SHA384:"
        "AES128-SHA:"
        "AES256-SHA";
#else
    constexpr auto cipherList =
        "TLS_CHACHA20_POLY1305_SHA256:"
        "TLS_AES_128_GCM_SHA256:"
        "TLS_AES_256_GCM_SHA384:"
        "TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256:"
        "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256:"
        "TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256:"
        "TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256:"
        "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:"
        "TLS_DHE_RSA_WITH_AES_128_GCM_SHA256:"
        "TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384:"
        "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384:"
        "TLS_DHE_RSA_WITH_AES_256_GCM_SHA384:"
        "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256:"
        "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256:"
        "TLS_DHE_RSA_WITH_AES_128_CBC_SHA256:"
        "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384:"
        "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384:"
        "TLS_DHE_RSA_WITH_AES_256_CBC_SHA256:"
        "TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA:"
        "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA:"
        "TLS_DHE_RSA_WITH_AES_128_CBC_SHA:"
        "TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA:"
        "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:"
        "TLS_DHE_RSA_WITH_AES_256_CBC_SHA:"
        "TLS_RSA_WITH_AES_128_GCM_SHA256:"
        "TLS_RSA_WITH_AES_256_GCM_SHA384:"
        "TLS_RSA_WITH_AES_128_CBC_SHA256:"
        "TLS_RSA_WITH_AES_256_CBC_SHA256:"
        "TLS_RSA_WITH_AES_128_CBC_SHA:"
        "TLS_RSA_WITH_AES_256_CBC_SHA";
#endif

    const auto cipherListTLS1_3 =
        "TLS_CHACHA20_POLY1305_SHA256:"
        "TLS_AES_128_GCM_SHA256:"
        "TLS_AES_256_GCM_SHA384";

    setCipherList(cipherList);
    setCipherListTLS1_3(cipherListTLS1_3);
}

} // namespace WebCore
