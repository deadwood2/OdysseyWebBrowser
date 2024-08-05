/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "SessionTracker.h"

#include "NetworkSession.h"
#include <WebCore/NetworkStorageSession.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RunLoop.h>

using namespace WebCore;

namespace WebKit {

static String& identifierBase()
{
    ASSERT(RunLoop::isMain());

    static NeverDestroyed<String> base;
    return base;
}

const String& SessionTracker::getIdentifierBase()
{
    return identifierBase();
}

void SessionTracker::setIdentifierBase(const String& identifier)
{
    ASSERT(RunLoop::isMain());

    identifierBase() = identifier;
}

#if USE(NETWORK_SESSION)
static HashMap<SessionID, RefPtr<NetworkSession>>& staticSessionMap()
{
    ASSERT(RunLoop::isMain());

    static NeverDestroyed<HashMap<SessionID, RefPtr<NetworkSession>>> map;
    return map;
}

NetworkSession* SessionTracker::networkSession(SessionID sessionID)
{
    if (sessionID == SessionID::defaultSessionID())
        return &NetworkSession::defaultSession();
    return staticSessionMap().get(sessionID);
}

void SessionTracker::setSession(SessionID sessionID, Ref<NetworkSession>&& session)
{
    ASSERT(sessionID != SessionID::defaultSessionID());
    staticSessionMap().set(sessionID, WTFMove(session));
}
#endif

void SessionTracker::destroySession(SessionID sessionID)
{
    ASSERT(RunLoop::isMain());
#if USE(NETWORK_SESSION)
    auto session = staticSessionMap().take(sessionID);
    if (session)
        session->invalidateAndCancel();
#endif
    NetworkStorageSession::destroySession(sessionID);
}

} // namespace WebKit
