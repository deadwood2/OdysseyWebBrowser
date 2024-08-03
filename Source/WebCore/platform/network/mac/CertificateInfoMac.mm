/*
 * Copyright (C) 2010, 2015 Apple Inc. All rights reserved.
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

#import "config.h"
#import "CertificateInfo.h"

#import "NotImplemented.h"
#import <wtf/spi/cocoa/SecuritySPI.h>

namespace WebCore {

#if PLATFORM(COCOA)
RetainPtr<CFArrayRef> CertificateInfo::certificateChainFromSecTrust(SecTrustRef trust)
{
    auto count = SecTrustGetCertificateCount(trust);
    auto certificateChain = CFArrayCreateMutable(0, count, &kCFTypeArrayCallBacks);
    for (CFIndex i = 0; i < count; i++)
        CFArrayAppendValue(certificateChain, SecTrustGetCertificateAtIndex(trust, i));
    return adoptCF((CFArrayRef)certificateChain);
}
#endif

CertificateInfo::Type CertificateInfo::type() const
{
#if HAVE(SEC_TRUST_SERIALIZATION)
    if (m_trust)
        return Type::Trust;
#endif
    if (m_certificateChain)
        return Type::CertificateChain;
    return Type::None;
}

CFArrayRef CertificateInfo::certificateChain() const
{
#if HAVE(SEC_TRUST_SERIALIZATION)
    if (m_certificateChain)
        return m_certificateChain.get();

    if (m_trust) 
        m_certificateChain = CertificateInfo::certificateChainFromSecTrust(m_trust.get());
#endif

    return m_certificateChain.get();
}

bool CertificateInfo::containsNonRootSHA1SignedCertificate() const
{
#if HAVE(SEC_TRUST_SERIALIZATION)
    if (m_trust) {
        // Allow only the root certificate (the last in the chain) to be SHA1.
        for (CFIndex i = 0, size = SecTrustGetCertificateCount(trust()) - 1; i < size; ++i) {
            auto certificate = SecTrustGetCertificateAtIndex(trust(), i);
            if (SecCertificateGetSignatureHashAlgorithm(certificate) == kSecSignatureHashAlgorithmSHA1)
                return true;
        }

        return false;
    }
#endif

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101100) || PLATFORM(IOS)
    if (m_certificateChain) {
        // Allow only the root certificate (the last in the chain) to be SHA1.
        for (CFIndex i = 0, size = CFArrayGetCount(m_certificateChain.get()) - 1; i < size; ++i) {
            auto certificate = (SecCertificateRef)CFArrayGetValueAtIndex(m_certificateChain.get(), i);
            if (SecCertificateGetSignatureHashAlgorithm(certificate) == kSecSignatureHashAlgorithmSHA1)
                return true;
        }
        return false;
    }
#else
    notImplemented();
#endif

    return false;
}

#ifndef NDEBUG
void CertificateInfo::dump() const
{
#if HAVE(SEC_TRUST_SERIALIZATION)
    if (m_trust) {
        CFIndex entries = SecTrustGetCertificateCount(trust());

        NSLog(@"CertificateInfo SecTrust\n");
        NSLog(@"  Entries: %ld\n", entries);
        for (CFIndex i = 0; i < entries; ++i) {
            RetainPtr<CFStringRef> summary = adoptCF(SecCertificateCopySubjectSummary(SecTrustGetCertificateAtIndex(trust(), i)));
            NSLog(@"  %@", (NSString *)summary.get());
        }

        return;
    }
#endif
    if (m_certificateChain) {
        CFIndex entries = CFArrayGetCount(m_certificateChain.get());

        NSLog(@"CertificateInfo (Certificate Chain)\n");
        NSLog(@"  Entries: %ld\n", entries);
        for (CFIndex i = 0; i < entries; ++i) {
            RetainPtr<CFStringRef> summary = adoptCF(SecCertificateCopySubjectSummary((SecCertificateRef)CFArrayGetValueAtIndex(m_certificateChain.get(), i)));
            NSLog(@"  %@", (NSString *)summary.get());
        }

        return;
    }
    
    NSLog(@"CertificateInfo (Empty)\n");
}
#endif

} // namespace WebCore
