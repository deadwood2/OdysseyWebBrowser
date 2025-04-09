/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
#include "LocalAuthenticator.h"

#if ENABLE(WEB_AUTHN)

#import <Security/SecItem.h>
#import <WebCore/AuthenticatorAssertionResponse.h>
#import <WebCore/AuthenticatorAttestationResponse.h>
#import <WebCore/CBORWriter.h>
#import <WebCore/ExceptionData.h>
#import <WebCore/PublicKeyCredentialCreationOptions.h>
#import <WebCore/PublicKeyCredentialRequestOptions.h>
#import <WebCore/WebAuthenticationConstants.h>
#import <WebCore/WebAuthenticationUtils.h>
#import <pal/crypto/CryptoDigest.h>
#import <wtf/HashSet.h>
#import <wtf/RetainPtr.h>
#import <wtf/RunLoop.h>
#import <wtf/Vector.h>
#import <wtf/spi/cocoa/SecuritySPI.h>
#import <wtf/text/StringHash.h>

namespace WebKit {
using namespace WebCore;

namespace LocalAuthenticatorInternal {

// See https://www.w3.org/TR/webauthn/#flags.
const uint8_t makeCredentialFlags = 0b01000101; // UP, UV and AT are set.
const uint8_t getAssertionFlags = 0b00000101; // UP and UV are set.
// Credential ID is currently SHA-1 of the corresponding public key.
const uint16_t credentialIdLength = 20;

static inline bool emptyTransportsOrContain(const Vector<AuthenticatorTransport>& transports, AuthenticatorTransport target)
{
    return transports.isEmpty() ? true : transports.contains(target);
}

static inline HashSet<String> produceHashSet(const Vector<PublicKeyCredentialDescriptor>& credentialDescriptors)
{
    HashSet<String> result;
    for (auto& credentialDescriptor : credentialDescriptors) {
        if (emptyTransportsOrContain(credentialDescriptor.transports, AuthenticatorTransport::Internal)
            && credentialDescriptor.type == PublicKeyCredentialType::PublicKey
            && credentialDescriptor.idVector.size() == credentialIdLength)
            result.add(String(reinterpret_cast<const char*>(credentialDescriptor.idVector.data()), credentialDescriptor.idVector.size()));
    }
    return result;
}

static inline Vector<uint8_t> toVector(NSData *data)
{
    Vector<uint8_t> result;
    result.append(reinterpret_cast<const uint8_t*>(data.bytes), data.length);
    return result;
}

} // LocalAuthenticatorInternal

LocalAuthenticator::LocalAuthenticator(UniqueRef<LocalConnection>&& connection)
    : m_connection(WTFMove(connection))
{
}

void LocalAuthenticator::makeCredential()
{
    using namespace LocalAuthenticatorInternal;
    ASSERT(m_state == State::Init);
    m_state = State::RequestReceived;
    auto& creationOptions = WTF::get<PublicKeyCredentialCreationOptions>(requestData().options);

    // The following implements https://www.w3.org/TR/webauthn/#op-make-cred as of 5 December 2017.
    // Skip Step 4-5 as requireResidentKey and requireUserVerification are enforced.
    // Skip Step 9 as extensions are not supported yet.
    // Step 8 is implicitly captured by all UnknownError exception receiveResponds.
    // Step 2.
    bool canFullfillPubKeyCredParams = false;
    for (auto& pubKeyCredParam : creationOptions.pubKeyCredParams) {
        if (pubKeyCredParam.type == PublicKeyCredentialType::PublicKey && pubKeyCredParam.alg == COSE::ES256) {
            canFullfillPubKeyCredParams = true;
            break;
        }
    }
    if (!canFullfillPubKeyCredParams) {
        receiveRespond(ExceptionData { NotSupportedError, "The platform attached authenticator doesn't support any provided PublicKeyCredentialParameters."_s });
        return;
    }

    // Step 3.
    auto excludeCredentialIds = produceHashSet(creationOptions.excludeCredentials);
    if (!excludeCredentialIds.isEmpty()) {
        // Search Keychain for the RP ID.
        NSDictionary *query = @{
            (id)kSecClass: (id)kSecClassKey,
            (id)kSecAttrKeyClass: (id)kSecAttrKeyClassPrivate,
            (id)kSecAttrLabel: creationOptions.rp.id,
            (id)kSecReturnAttributes: @YES,
            (id)kSecMatchLimit: (id)kSecMatchLimitAll,
#if HAVE(DATA_PROTECTION_KEYCHAIN)
            (id)kSecUseDataProtectionKeychain: @YES
#else
            (id)kSecAttrNoLegacy: @YES
#endif
        };
        CFTypeRef attributesArrayRef = nullptr;
        OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &attributesArrayRef);
        if (status && status != errSecItemNotFound) {
            LOG_ERROR("Couldn't query Keychain: %d", status);
            receiveRespond(ExceptionData { UnknownError, makeString("Couldn't query Keychain: ", status) });
            return;
        }
        auto retainAttributesArray = adoptCF(attributesArrayRef);

        for (NSDictionary *nsAttributes in (NSArray *)attributesArrayRef) {
            NSData *nsCredentialId = nsAttributes[(id)kSecAttrApplicationLabel];
            if (excludeCredentialIds.contains(String(reinterpret_cast<const char*>(nsCredentialId.bytes), nsCredentialId.length))) {
                receiveRespond(ExceptionData { NotAllowedError, "At least one credential matches an entry of the excludeCredentials list in the platform attached authenticator."_s });
                return;
            }
        }
    }

    // Step 6.
    // FIXME(rdar://problem/35900593): Update to a formal UI.
    // Get user consent.
    auto callback = [weakThis = makeWeakPtr(*this)](LocalConnection::UserConsent consent) {
        ASSERT(RunLoop::isMain());
        if (!weakThis)
            return;

        weakThis->continueMakeCredentialAfterUserConsented(consent);
    };
    m_connection->getUserConsent(
        "allow " + creationOptions.rp.id + " to create a public key credential for " + creationOptions.user.name,
        WTFMove(callback));
}

void LocalAuthenticator::continueMakeCredentialAfterUserConsented(LocalConnection::UserConsent consent)
{
    ASSERT(m_state == State::RequestReceived);
    m_state = State::UserConsented;
    auto& creationOptions = WTF::get<PublicKeyCredentialCreationOptions>(requestData().options);

    if (consent == LocalConnection::UserConsent::No) {
        receiveRespond(ExceptionData { NotAllowedError, "Couldn't get user consent."_s });
        return;
    }

    // Step 7.5.
    // Userhandle is stored in kSecAttrApplicationTag attribute.
    // Failures after this point could block users' accounts forever. Should we follow the spec?
    NSDictionary* deleteQuery = @{
        (id)kSecClass: (id)kSecClassKey,
        (id)kSecAttrLabel: creationOptions.rp.id,
        (id)kSecAttrApplicationTag: [NSData dataWithBytes:creationOptions.user.idVector.data() length:creationOptions.user.idVector.size()],
#if HAVE(DATA_PROTECTION_KEYCHAIN)
        (id)kSecUseDataProtectionKeychain: @YES
#else
        (id)kSecAttrNoLegacy: @YES
#endif
    };
    OSStatus status = SecItemDelete((__bridge CFDictionaryRef)deleteQuery);
    if (status && status != errSecItemNotFound) {
        LOG_ERROR("Couldn't delete older credential: %d", status);
        receiveRespond(ExceptionData { UnknownError, makeString("Couldn't delete older credential: ", status) });
        return;
    }

    // Step 7.1, 13. Apple Attestation
    auto callback = [weakThis = makeWeakPtr(*this)](SecKeyRef _Nullable privateKey, NSArray * _Nullable certificates, NSError * _Nullable error) {
        ASSERT(RunLoop::isMain());
        if (!weakThis)
            return;
        weakThis->continueMakeCredentialAfterAttested(privateKey, certificates, error);
    };
    m_connection->getAttestation(creationOptions.rp.id, creationOptions.user.name, requestData().hash, WTFMove(callback));
}

void LocalAuthenticator::continueMakeCredentialAfterAttested(SecKeyRef privateKey, NSArray *certificates, NSError *error)
{
    using namespace LocalAuthenticatorInternal;

    ASSERT(m_state == State::UserConsented);
    m_state = State::Attested;
    auto& creationOptions = WTF::get<PublicKeyCredentialCreationOptions>(requestData().options);

    if (error) {
        LOG_ERROR("Couldn't attest: %@", error);
        receiveRespond(ExceptionData { UnknownError, makeString("Couldn't attest: ", String(error.localizedDescription)) });
        return;
    }
    // Attestation Certificate and Attestation Issuing CA
    ASSERT(certificates && ([certificates count] == 2));

    // Step 7.2-7.4.
    // FIXME(183533): A single kSecClassKey item couldn't store all meta data. The following schema is a tentative solution
    // to accommodate the most important meta data, i.e. RP ID, Credential ID, and userhandle.
    // kSecAttrLabel: RP ID
    // kSecAttrApplicationLabel: Credential ID (auto-gen by Keychain)
    // kSecAttrApplicationTag: userhandle
    // Noted, the current DeviceIdentity.Framework would only allow us to pass the kSecAttrLabel as the inital attribute
    // for the Keychain item. Since that's the only clue we have to locate the unique item, we use the pattern username@rp.id
    // as the initial value.
    // Also noted, the vale of kSecAttrApplicationLabel is automatically generated by the Keychain, which is a SHA-1 hash of
    // the public key. We borrow it directly for now to workaround the stated limitations.
    // Update the Keychain item to the above schema.
    // FIXME(183533): DeviceIdentity.Framework would insert certificates into Keychain as well. We should update those as well.
    Vector<uint8_t> credentialId;
    {
        // -rk-ucrt is added by DeviceIdentity.Framework.
        String label = makeString(creationOptions.user.name, "@", creationOptions.rp.id, "-rk-ucrt");
        NSDictionary *credentialIdQuery = @{
            (id)kSecClass: (id)kSecClassKey,
            (id)kSecAttrKeyClass: (id)kSecAttrKeyClassPrivate,
            (id)kSecAttrLabel: label,
            (id)kSecReturnAttributes: @YES,
#if HAVE(DATA_PROTECTION_KEYCHAIN)
            (id)kSecUseDataProtectionKeychain: @YES
#else
            (id)kSecAttrNoLegacy: @YES
#endif
        };
        CFTypeRef attributesRef = nullptr;
        OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)credentialIdQuery, &attributesRef);
        if (status) {
            LOG_ERROR("Couldn't get Credential ID: %d", status);
            receiveRespond(ExceptionData { UnknownError, makeString("Couldn't get Credential ID: ", status) });
            return;
        }
        auto retainAttributes = adoptCF(attributesRef);

        NSDictionary *nsAttributes = (NSDictionary *)attributesRef;
        credentialId = toVector(nsAttributes[(id)kSecAttrApplicationLabel]);

        NSDictionary *updateQuery = @{
            (id)kSecClass: (id)kSecClassKey,
            (id)kSecAttrKeyClass: (id)kSecAttrKeyClassPrivate,
            (id)kSecAttrApplicationLabel: nsAttributes[(id)kSecAttrApplicationLabel],
#if HAVE(DATA_PROTECTION_KEYCHAIN)
            (id)kSecUseDataProtectionKeychain: @YES
#else
            (id)kSecAttrNoLegacy: @YES
#endif
        };
        NSDictionary *updateParams = @{
            (id)kSecAttrLabel: creationOptions.rp.id,
            (id)kSecAttrApplicationTag: [NSData dataWithBytes:creationOptions.user.idVector.data() length:creationOptions.user.idVector.size()],
        };
        status = SecItemUpdate((__bridge CFDictionaryRef)updateQuery, (__bridge CFDictionaryRef)updateParams);
        if (status) {
            LOG_ERROR("Couldn't update the Keychain item: %d", status);
            receiveRespond(ExceptionData { UnknownError, makeString("Couldn't update the Keychain item: ", status) });
            return;
        }
    }

    // Step 10.
    // FIXME(183533): store the counter.
    uint32_t counter = 0;

    // Step 11. https://www.w3.org/TR/webauthn/#attested-credential-data
    // credentialPublicKey
    Vector<uint8_t> cosePublicKey;
    {
        RetainPtr<CFDataRef> publicKeyDataRef;
        {
            auto publicKey = adoptCF(SecKeyCopyPublicKey(privateKey));
            CFErrorRef errorRef = nullptr;
            publicKeyDataRef = adoptCF(SecKeyCopyExternalRepresentation(publicKey.get(), &errorRef));
            auto retainError = adoptCF(errorRef);
            if (errorRef) {
                LOG_ERROR("Couldn't export the public key: %@", (NSError*)errorRef);
                receiveRespond(ExceptionData { UnknownError, makeString("Couldn't export the public key: ", String(((NSError*)errorRef).localizedDescription)) });
                return;
            }
            ASSERT(((NSData *)publicKeyDataRef.get()).length == (1 + 2 * ES256FieldElementLength)); // 04 | X | Y
        }

        // COSE Encoding
        Vector<uint8_t> x(ES256FieldElementLength);
        [(NSData *)publicKeyDataRef.get() getBytes: x.data() range:NSMakeRange(1, ES256FieldElementLength)];
        Vector<uint8_t> y(ES256FieldElementLength);
        [(NSData *)publicKeyDataRef.get() getBytes: y.data() range:NSMakeRange(1 + ES256FieldElementLength, ES256FieldElementLength)];
        cosePublicKey = encodeES256PublicKeyAsCBOR(WTFMove(x), WTFMove(y));
    }
    // FIXME(rdar://problem/38320512): Define Apple AAGUID.
    auto attestedCredentialData = buildAttestedCredentialData(Vector<uint8_t>(aaguidLength, 0), credentialId, cosePublicKey);

    // Step 12.
    auto authData = buildAuthData(creationOptions.rp.id, makeCredentialFlags, counter, attestedCredentialData);

    // Step 13. Apple Attestation Cont'
    // Assemble the attestation object:
    // https://www.w3.org/TR/webauthn/#attestation-object
    cbor::CBORValue::MapValue attestationStatementMap;
    {
        Vector<uint8_t> signature;
        {
            CFErrorRef errorRef = nullptr;
            // FIXME(183652): Reduce prompt for biometrics
            auto signatureRef = adoptCF(SecKeyCreateSignature(privateKey, kSecKeyAlgorithmECDSASignatureMessageX962SHA256, (__bridge CFDataRef)[NSData dataWithBytes:authData.data() length:authData.size()], &errorRef));
            auto retainError = adoptCF(errorRef);
            if (errorRef) {
                LOG_ERROR("Couldn't generate the signature: %@", (NSError*)errorRef);
                receiveRespond(ExceptionData { UnknownError, makeString("Couldn't generate the signature: ", String(((NSError*)errorRef).localizedDescription)) });
                return;
            }
            signature = toVector((NSData *)signatureRef.get());
        }
        attestationStatementMap[cbor::CBORValue("alg")] = cbor::CBORValue(COSE::ES256);
        attestationStatementMap[cbor::CBORValue("sig")] = cbor::CBORValue(signature);
        Vector<cbor::CBORValue> cborArray;
        for (size_t i = 0; i < [certificates count]; i++)
            cborArray.append(cbor::CBORValue(toVector((NSData *)adoptCF(SecCertificateCopyData((__bridge SecCertificateRef)certificates[i])).get())));
        attestationStatementMap[cbor::CBORValue("x5c")] = cbor::CBORValue(WTFMove(cborArray));
    }
    auto attestationObject = buildAttestationObject(WTFMove(authData), "Apple", WTFMove(attestationStatementMap), creationOptions.attestation);

    receiveRespond(AuthenticatorAttestationResponse::create(credentialId, attestationObject));
}

void LocalAuthenticator::getAssertion()
{
    using namespace LocalAuthenticatorInternal;
    ASSERT(m_state == State::Init);
    m_state = State::RequestReceived;
    auto& requestOptions = WTF::get<PublicKeyCredentialRequestOptions>(requestData().options);

    // The following implements https://www.w3.org/TR/webauthn/#op-get-assertion as of 5 December 2017.
    // Skip Step 2 as requireUserVerification is enforced.
    // Skip Step 8 as extensions are not supported yet.
    // Step 12 is implicitly captured by all UnknownError exception callbacks.
    // Step 3-5. Unlike the spec, if an allow list is provided and there is no intersection between existing ones and the allow list, we always return NotAllowedError.
    auto allowCredentialIds = produceHashSet(requestOptions.allowCredentials);
    if (!requestOptions.allowCredentials.isEmpty() && allowCredentialIds.isEmpty()) {
        receiveRespond(ExceptionData { NotAllowedError, "No matched credentials are found in the platform attached authenticator."_s });
        return;
    }

    // Search Keychain for the RP ID.
    NSDictionary *query = @{
        (id)kSecClass: (id)kSecClassKey,
        (id)kSecAttrKeyClass: (id)kSecAttrKeyClassPrivate,
        (id)kSecAttrLabel: requestOptions.rpId,
        (id)kSecReturnAttributes: @YES,
        (id)kSecMatchLimit: (id)kSecMatchLimitAll,
#if HAVE(DATA_PROTECTION_KEYCHAIN)
        (id)kSecUseDataProtectionKeychain: @YES
#else
        (id)kSecAttrNoLegacy: @YES
#endif
    };
    CFTypeRef attributesArrayRef = nullptr;
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &attributesArrayRef);
    if (status && status != errSecItemNotFound) {
        LOG_ERROR("Couldn't query Keychain: %d", status);
        receiveRespond(ExceptionData { UnknownError, makeString("Couldn't query Keychain: ", status) });
        return;
    }
    auto retainAttributesArray = adoptCF(attributesArrayRef);

    NSArray *intersectedCredentialsAttributes = nil;
    if (requestOptions.allowCredentials.isEmpty())
        intersectedCredentialsAttributes = (NSArray *)attributesArrayRef;
    else {
        NSMutableArray *result = [NSMutableArray arrayWithCapacity:allowCredentialIds.size()];
        for (NSDictionary *nsAttributes in (NSArray *)attributesArrayRef) {
            NSData *nsCredentialId = nsAttributes[(id)kSecAttrApplicationLabel];
            if (allowCredentialIds.contains(String(reinterpret_cast<const char*>(nsCredentialId.bytes), nsCredentialId.length)))
                [result addObject:nsAttributes];
        }
        intersectedCredentialsAttributes = result;
    }
    if (!intersectedCredentialsAttributes.count) {
        receiveRespond(ExceptionData { NotAllowedError, "No matched credentials are found in the platform attached authenticator."_s });
        return;
    }

    // Step 6.
    auto *selectedCredentialAttributes = m_connection->selectCredential(intersectedCredentialsAttributes);

    // Step 7. Get user consent.
    // FIXME(rdar://problem/35900593): Update to a formal UI.
    auto callback = [
        weakThis = makeWeakPtr(*this),
        credentialId = toVector(selectedCredentialAttributes[(id)kSecAttrApplicationLabel]),
        userhandle = toVector(selectedCredentialAttributes[(id)kSecAttrApplicationTag])
    ](LocalConnection::UserConsent consent, LAContext *context) {
        ASSERT(RunLoop::isMain());
        if (!weakThis)
            return;

        weakThis->continueGetAssertionAfterUserConsented(consent, context, credentialId, userhandle);
    };
    NSData *idData = selectedCredentialAttributes[(id)kSecAttrApplicationTag];
    StringView idStringView { static_cast<const UChar*>([idData bytes]), static_cast<unsigned>([idData length]) };
    m_connection->getUserConsent(
        makeString("log into ", requestOptions.rpId, " with ", idStringView),
        (__bridge SecAccessControlRef)selectedCredentialAttributes[(id)kSecAttrAccessControl],
        WTFMove(callback));
}

void LocalAuthenticator::continueGetAssertionAfterUserConsented(LocalConnection::UserConsent consent, LAContext *context, const Vector<uint8_t>& credentialId, const Vector<uint8_t>& userhandle)
{
    using namespace LocalAuthenticatorInternal;
    ASSERT(m_state == State::RequestReceived);
    m_state = State::UserConsented;

    if (consent == LocalConnection::UserConsent::No) {
        receiveRespond(ExceptionData { NotAllowedError, "Couldn't get user consent."_s });
        return;
    }

    // Step 9-10.
    // FIXME(183533): Due to the stated Keychain limitations, we can't save the counter value.
    // Therefore, it is always zero.
    uint32_t counter = 0;
    auto authData = buildAuthData(WTF::get<PublicKeyCredentialRequestOptions>(requestData().options).rpId, getAssertionFlags, counter, { });

    // Step 11.
    Vector<uint8_t> signature;
    {
        NSDictionary *query = @{
            (id)kSecClass: (id)kSecClassKey,
            (id)kSecAttrKeyClass: (id)kSecAttrKeyClassPrivate,
            (id)kSecAttrApplicationLabel: [NSData dataWithBytes:credentialId.data() length:credentialId.size()],
            (id)kSecUseAuthenticationContext: context,
            (id)kSecReturnRef: @YES,
#if HAVE(DATA_PROTECTION_KEYCHAIN)
            (id)kSecUseDataProtectionKeychain: @YES
#else
            (id)kSecAttrNoLegacy: @YES
#endif
        };
        CFTypeRef privateKeyRef = nullptr;
        OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)query, &privateKeyRef);
        if (status) {
            LOG_ERROR("Couldn't get the private key reference: %d", status);
            receiveRespond(ExceptionData { UnknownError, makeString("Couldn't get the private key reference: ", status) });
            return;
        }
        auto privateKey = adoptCF(privateKeyRef);

        NSMutableData *dataToSign = [NSMutableData dataWithBytes:authData.data() length:authData.size()];
        [dataToSign appendBytes:requestData().hash.data() length:requestData().hash.size()];

        CFErrorRef errorRef = nullptr;
        // FIXME: Converting CFTypeRef to SecKeyRef is quite subtle here.
        auto signatureRef = adoptCF(SecKeyCreateSignature((__bridge SecKeyRef)((id)privateKeyRef), kSecKeyAlgorithmECDSASignatureMessageX962SHA256, (__bridge CFDataRef)dataToSign, &errorRef));
        auto retainError = adoptCF(errorRef);
        if (errorRef) {
            LOG_ERROR("Couldn't generate the signature: %@", (NSError*)errorRef);
            receiveRespond(ExceptionData { UnknownError, makeString("Couldn't generate the signature: ", String(((NSError*)errorRef).localizedDescription)) });
            return;
        }
        signature = toVector((NSData *)signatureRef.get());
    }

    // Step 13.
    receiveRespond(AuthenticatorAssertionResponse::create(credentialId, authData, signature, userhandle));
}

} // namespace WebKit

#endif // ENABLE(WEB_AUTHN)
