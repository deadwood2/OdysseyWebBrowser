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

#if ENABLE(ENCRYPTED_MEDIA) && HAVE(AVCONTENTKEYSESSION)

#include "CDMInstance.h"
#include <wtf/Function.h>
#include <wtf/RetainPtr.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/WTFString.h>

OBJC_CLASS AVContentKeyRequest;
OBJC_CLASS AVContentKeySession;
OBJC_CLASS NSData;
OBJC_CLASS NSError;
OBJC_CLASS NSURL;
OBJC_CLASS WebCoreFPSContentKeySessionDelegate;

namespace WebCore {

class CDMInstanceFairPlayStreamingAVFObjC final : public CDMInstance {
public:
    CDMInstanceFairPlayStreamingAVFObjC();
    virtual ~CDMInstanceFairPlayStreamingAVFObjC();

    static bool supportsPersistableState();
    static bool supportsPersistentKeys();
    static bool mimeTypeIsPlayable(const String&);

    ImplementationType implementationType() const final { return ImplementationType::FairPlayStreaming; }

    SuccessValue initializeWithConfiguration(const CDMKeySystemConfiguration&) final;
    SuccessValue setDistinctiveIdentifiersAllowed(bool) final;
    SuccessValue setPersistentStateAllowed(bool) final;
    SuccessValue setServerCertificate(Ref<SharedBuffer>&&) final;
    SuccessValue setStorageDirectory(const String&) final;

    void requestLicense(LicenseType, const AtomicString& initDataType, Ref<SharedBuffer>&& initData, LicenseCallback) final;
    void updateLicense(const String&, LicenseType, const SharedBuffer&, LicenseUpdateCallback) final;
    void loadSession(LicenseType, const String&, const String&, LoadSessionCallback) final;
    void closeSession(const String&, CloseSessionCallback) final;
    void removeSessionData(const String&, LicenseType, RemoveSessionDataCallback) final;
    void storeRecordOfKeyUsage(const String&) final;

    const String& keySystem() const final;

    void didProvideRequest(AVContentKeyRequest *);
    void didProvideRenewingRequest(AVContentKeyRequest *);
    void didProvidePersistableRequest(AVContentKeyRequest *);
    void didFailToProvideRequest(AVContentKeyRequest *, NSError *);
    bool shouldRetryRequestForReason(AVContentKeyRequest *, NSString *);
    void sessionIdentifierChanged(NSData *);

    AVContentKeySession *contentKeySession() { return m_session.get(); }

private:
    WeakPtr<CDMInstanceFairPlayStreamingAVFObjC> createWeakPtr() { return m_weakPtrFactory.createWeakPtr(*this); }
    bool isLicenseTypeSupported(LicenseType) const;

    Vector<Ref<SharedBuffer>> keyIDs();

    WeakPtrFactory<CDMInstanceFairPlayStreamingAVFObjC> m_weakPtrFactory;
    RefPtr<SharedBuffer> m_serverCertificate;
    bool m_persistentStateAllowed { true };
    RetainPtr<NSURL> m_storageDirectory;
    RetainPtr<AVContentKeySession> m_session;
    RetainPtr<AVContentKeyRequest> m_request;
    RetainPtr<WebCoreFPSContentKeySessionDelegate> m_delegate;
    Vector<RetainPtr<NSData>> m_expiredSessions;
    String m_sessionId;

    LicenseCallback m_requestLicenseCallback;
    LicenseUpdateCallback m_updateLicenseCallback;
    CloseSessionCallback m_closeSessionCallback;
    RemoveSessionDataCallback m_removeSessionDataCallback;
};
}

SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstanceFairPlayStreamingAVFObjC, WebCore::CDMInstance::ImplementationType::FairPlayStreaming)

#endif // ENABLE(ENCRYPTED_MEDIA) && HAVE(AVCONTENTKEYSESSION)
