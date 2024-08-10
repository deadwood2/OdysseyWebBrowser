/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "AuthenticationChallengeProxy.h"

#include "AuthenticationDecisionListener.h"
#include "AuthenticationManagerMessages.h"
#include "ChildProcessProxy.h"
#include "WebCertificateInfo.h"
#include "WebCoreArgumentCoders.h"
#include "WebCredential.h"
#include "WebProcessProxy.h"
#include "WebProtectionSpace.h"

#if HAVE(SEC_KEY_PROXY)
#include "SecKeyProxyStore.h"
#endif

namespace WebKit {

AuthenticationChallengeProxy::AuthenticationChallengeProxy(WebCore::AuthenticationChallenge&& authenticationChallenge, uint64_t challengeID, IPC::Connection* connection)
    : m_coreAuthenticationChallenge(WTFMove(authenticationChallenge))
    , m_challengeID(challengeID)
    , m_connection(connection)
{
    ASSERT(m_challengeID);
    m_listener = AuthenticationDecisionListener::create(this);
}

AuthenticationChallengeProxy::~AuthenticationChallengeProxy()
{
    // If an outstanding AuthenticationChallengeProxy is being destroyed even though it hasn't been responded to yet,
    // we cancel it here so the process isn't waiting for an answer forever.
    if (m_challengeID)
        m_connection->send(Messages::AuthenticationManager::CancelChallenge(m_challengeID), 0);

    if (m_listener)
        m_listener->detachChallenge();
}

void AuthenticationChallengeProxy::useCredential(WebCredential* credential)
{
    if (!m_challengeID)
        return;

    uint64_t challengeID = m_challengeID;
    m_challengeID = 0;

    if (!credential) {
        m_connection->send(Messages::AuthenticationManager::ContinueWithoutCredentialForChallenge(challengeID), 0);
        return;
    }

#if HAVE(SEC_KEY_PROXY)
    if (protectionSpace()->authenticationScheme() == WebCore::ProtectionSpaceAuthenticationSchemeClientCertificateRequested) {
        if (!m_secKeyProxyStore) {
            m_connection->send(Messages::AuthenticationManager::ContinueWithoutCredentialForChallenge(challengeID), 0);
            return;
        }
        m_secKeyProxyStore->initialize(credential->credential());
        sendClientCertificateCredentialOverXpc(challengeID, credential->credential());
        return;
    }
#endif
    m_connection->send(Messages::AuthenticationManager::UseCredentialForChallenge(challengeID, credential->credential()), 0);
}

void AuthenticationChallengeProxy::cancel()
{
    if (!m_challengeID)
        return;

    m_connection->send(Messages::AuthenticationManager::CancelChallenge(m_challengeID), 0);

    m_challengeID = 0;
}

void AuthenticationChallengeProxy::performDefaultHandling()
{
    if (!m_challengeID)
        return;

    m_connection->send(Messages::AuthenticationManager::PerformDefaultHandling(m_challengeID), 0);

    m_challengeID = 0;
}

void AuthenticationChallengeProxy::rejectProtectionSpaceAndContinue()
{
    if (!m_challengeID)
        return;

    m_connection->send(Messages::AuthenticationManager::RejectProtectionSpaceAndContinue(m_challengeID), 0);

    m_challengeID = 0;
}

WebCredential* AuthenticationChallengeProxy::proposedCredential() const
{
    if (!m_webCredential)
        m_webCredential = WebCredential::create(m_coreAuthenticationChallenge.proposedCredential());
        
    return m_webCredential.get();
}

WebProtectionSpace* AuthenticationChallengeProxy::protectionSpace() const
{
    if (!m_webProtectionSpace)
        m_webProtectionSpace = WebProtectionSpace::create(m_coreAuthenticationChallenge.protectionSpace());
        
    return m_webProtectionSpace.get();
}

#if HAVE(SEC_KEY_PROXY)
void AuthenticationChallengeProxy::setSecKeyProxyStore(SecKeyProxyStore& store)
{
    m_secKeyProxyStore = makeWeakPtr(store);
}
#endif

} // namespace WebKit
