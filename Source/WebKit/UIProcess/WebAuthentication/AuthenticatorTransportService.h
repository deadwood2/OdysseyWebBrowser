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

#pragma once

#if ENABLE(WEB_AUTHN)

#include <WebCore/AuthenticatorTransport.h>
#include <wtf/UniqueRef.h>
#include <wtf/WeakPtr.h>

namespace WebKit {

class Authenticator;

struct MockWebAuthenticationConfiguration;

class AuthenticatorTransportService : public CanMakeWeakPtr<AuthenticatorTransportService> {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(AuthenticatorTransportService);
public:
    class Observer : public CanMakeWeakPtr<Observer> {
    public:
        virtual ~Observer() = default;

        virtual void authenticatorAdded(Ref<Authenticator>&&) = 0;
    };

    static UniqueRef<AuthenticatorTransportService> create(WebCore::AuthenticatorTransport, Observer&);
    static UniqueRef<AuthenticatorTransportService> createMock(WebCore::AuthenticatorTransport, Observer&, const MockWebAuthenticationConfiguration&);

    virtual ~AuthenticatorTransportService() = default;

    // This operation is guaranteed to execute asynchronously.
    void startDiscovery();

protected:
    explicit AuthenticatorTransportService(Observer&);

    Observer* observer() const { return m_observer.get(); }

private:
    virtual void startDiscoveryInternal() = 0;

    WeakPtr<Observer> m_observer;
};

} // namespace WebKit

#endif // ENABLE(WEB_AUTHN)
