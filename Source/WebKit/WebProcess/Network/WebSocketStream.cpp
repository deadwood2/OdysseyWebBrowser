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

#include "config.h"
#include "WebSocketStream.h"

#include "DataReference.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkProcessConnection.h"
#include "NetworkSocketStreamMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcess.h"
#include <WebCore/SessionID.h>
#include <WebCore/SocketStreamError.h>
#include <WebCore/SocketStreamHandleClient.h>
#include <wtf/NeverDestroyed.h>

using namespace WebCore;

namespace WebKit {

static uint64_t nextAvailableIdentifier = 1;

static HashMap<uint64_t, WebSocketStream*>& globalWebSocketStreamMap()
{
    static NeverDestroyed<HashMap<uint64_t, WebSocketStream*>> globalMap;
    return globalMap;
}

WebSocketStream* WebSocketStream::streamWithIdentifier(uint64_t identifier)
{
    return globalWebSocketStreamMap().get(identifier);
}

void WebSocketStream::networkProcessCrashed()
{
    for (auto& stream : globalWebSocketStreamMap().values()) {
        for (auto& callback : stream->m_sendDataCallbacks.values())
            callback(false);
        stream->m_client.didFailSocketStream(*stream, SocketStreamError(0, { }, "Network process crashed."));
    }

    globalWebSocketStreamMap().clear();
}

Ref<WebSocketStream> WebSocketStream::create(const URL& url, SocketStreamHandleClient& client, SessionID sessionID, const String& credentialPartition)
{
    return adoptRef(*new WebSocketStream(url, client, sessionID, credentialPartition));
}

WebSocketStream::WebSocketStream(const WebCore::URL& url, WebCore::SocketStreamHandleClient& client, WebCore::SessionID sessionID, const String& cachePartition)
    : SocketStreamHandle(url, client)
    , m_identifier(nextAvailableIdentifier++)
    , m_client(client)
{
    WebProcess::singleton().networkConnection().connection().send(Messages::NetworkConnectionToWebProcess::CreateSocketStream(url, sessionID, cachePartition, m_identifier), 0);

    ASSERT(!globalWebSocketStreamMap().contains(m_identifier));
    globalWebSocketStreamMap().set(m_identifier, this);
}

WebSocketStream::~WebSocketStream()
{
    ASSERT(globalWebSocketStreamMap().contains(m_identifier));
    globalWebSocketStreamMap().remove(m_identifier);
}

IPC::Connection* WebSocketStream::messageSenderConnection()
{
    return &WebProcess::singleton().networkConnection().connection();
}

uint64_t WebSocketStream::messageSenderDestinationID()
{
    return m_identifier;
}

void WebSocketStream::platformSend(const char* data, size_t length, Function<void(bool)>&& completionHandler)
{
    static uint64_t nextDataIdentifier = 1;
    uint64_t dataIdentifier = nextDataIdentifier++;
    send(Messages::NetworkSocketStream::SendData(IPC::DataReference(reinterpret_cast<const uint8_t *>(data), length), dataIdentifier));
    ASSERT(!m_sendDataCallbacks.contains(dataIdentifier));
    m_sendDataCallbacks.set(dataIdentifier, WTFMove(completionHandler));
}

void WebSocketStream::didSendData(uint64_t identifier, bool success)
{
    ASSERT(m_sendDataCallbacks.contains(identifier));
    m_sendDataCallbacks.take(identifier)(success);
}
    
void WebSocketStream::platformClose()
{
    send(Messages::NetworkSocketStream::Close());
}

size_t WebSocketStream::bufferedAmount()
{
    return m_bufferedAmount;
}

void WebSocketStream::didOpenSocketStream()
{
    m_state = Open;
    m_client.didOpenSocketStream(*this);
}

void WebSocketStream::didCloseSocketStream()
{
    m_state = Closed;
    m_client.didCloseSocketStream(*this);
}

void WebSocketStream::didReceiveSocketStreamData(const IPC::DataReference& data)
{
    m_client.didReceiveSocketStreamData(*this, reinterpret_cast<const char*>(data.data()), data.size());
}

void WebSocketStream::didFailToReceiveSocketStreamData()
{
    m_client.didFailToReceiveSocketStreamData(*this);
}

void WebSocketStream::didUpdateBufferedAmount(uint64_t newAmount)
{
    m_bufferedAmount = newAmount;
    m_client.didUpdateBufferedAmount(*this, newAmount);
}

void WebSocketStream::didFailSocketStream(WebCore::SocketStreamError&& error)
{
    m_client.didFailSocketStream(*this, WTFMove(error));
}

} // namespace WebKit
