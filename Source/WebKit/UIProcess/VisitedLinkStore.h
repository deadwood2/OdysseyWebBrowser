/*
 * Copyright (C) 2010-2017 Apple Inc. All rights reserved.
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

#include "APIObject.h"
#include "MessageReceiver.h"
#include "SharedStringHashStore.h"
#include "WebPageProxy.h"
#include "WebProcessLifetimeObserver.h"
#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/Identified.h>
#include <wtf/RefCounted.h>

namespace WebKit {

class WebPageProxy;
class WebProcessProxy;
    
class VisitedLinkStore final : public API::ObjectImpl<API::Object::Type::VisitedLinkStore>, private IPC::MessageReceiver, public WebProcessLifetimeObserver, public Identified<VisitedLinkStore>, private SharedStringHashStore::Client {
public:
    static Ref<VisitedLinkStore> create();

    explicit VisitedLinkStore();
    virtual ~VisitedLinkStore();

    void addProcess(WebProcessProxy&);
    void removeProcess(WebProcessProxy&);

    void addVisitedLinkHash(WebCore::SharedStringHash);
    bool containsVisitedLinkHash(WebCore::SharedStringHash);
    void removeVisitedLinkHash(WebCore::SharedStringHash);
    void removeAll();

private:
    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) final;

    // WebProcessLifetimeObserver
    void webProcessWillOpenConnection(WebProcessProxy&, IPC::Connection&) final;
    void webProcessDidCloseConnection(WebProcessProxy&, IPC::Connection&) final;

    // SharedStringHashStore::Client
    void didInvalidateSharedMemory() final;
    void didUpdateSharedStringHashes(const Vector<WebCore::SharedStringHash>& addedHashes, const Vector<WebCore::SharedStringHash>& removedHashes) final;

    void addVisitedLinkHashFromPage(uint64_t pageID, WebCore::SharedStringHash);

    void sendStoreHandleToProcess(WebProcessProxy&);

    HashSet<WebProcessProxy*> m_processes;
    SharedStringHashStore m_linkHashStore;
};

} // namespace WebKit
