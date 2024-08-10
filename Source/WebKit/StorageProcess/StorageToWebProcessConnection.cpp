/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "StorageToWebProcessConnection.h"

#include "Logging.h"
#include "StorageProcess.h"
#include "StorageProcessMessages.h"
#include "StorageToWebProcessConnectionMessages.h"
#include "WebIDBConnectionToClient.h"
#include "WebIDBConnectionToClientMessages.h"
#include "WebSWServerConnection.h"
#include "WebSWServerConnectionMessages.h"
#include <wtf/RunLoop.h>

#if ENABLE(SERVICE_WORKER)
#include "WebSWServerToContextConnection.h"
#include "WebSWServerToContextConnectionMessages.h"
#endif

using namespace PAL;
using namespace WebCore;

namespace WebKit {

Ref<StorageToWebProcessConnection> StorageToWebProcessConnection::create(IPC::Connection::Identifier connectionIdentifier)
{
    return adoptRef(*new StorageToWebProcessConnection(connectionIdentifier));
}

StorageToWebProcessConnection::StorageToWebProcessConnection(IPC::Connection::Identifier connectionIdentifier)
    : m_connection(IPC::Connection::createServerConnection(connectionIdentifier, *this))
{
    m_connection->setOnlySendMessagesAsDispatchWhenWaitingForSyncReplyWhenProcessingSuchAMessage(true);
    m_connection->open();
}

StorageToWebProcessConnection::~StorageToWebProcessConnection()
{
    m_connection->invalidate();

#if ENABLE(SERVICE_WORKER)
    unregisterSWConnections();
#endif
}

void StorageToWebProcessConnection::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageReceiverName() == Messages::StorageToWebProcessConnection::messageReceiverName()) {
        didReceiveStorageToWebProcessConnectionMessage(connection, decoder);
        return;
    }

    if (decoder.messageReceiverName() == Messages::StorageProcess::messageReceiverName()) {
        StorageProcess::singleton().didReceiveStorageProcessMessage(connection, decoder);
        return;
    }

#if ENABLE(INDEXED_DATABASE)
    if (decoder.messageReceiverName() == Messages::WebIDBConnectionToClient::messageReceiverName()) {
        auto iterator = m_webIDBConnections.find(decoder.destinationID());
        if (iterator != m_webIDBConnections.end())
            iterator->value->didReceiveMessage(connection, decoder);
        return;
    }
#endif

#if ENABLE(SERVICE_WORKER)
    if (decoder.messageReceiverName() == Messages::WebSWServerConnection::messageReceiverName()) {
        if (auto swConnection = m_swConnections.get(makeObjectIdentifier<SWServerConnectionIdentifierType>(decoder.destinationID())))
            swConnection->didReceiveMessage(connection, decoder);
        return;
    }

    if (decoder.messageReceiverName() == Messages::WebSWServerToContextConnection::messageReceiverName()) {
        if (auto* contextConnection = StorageProcess::singleton().connectionToContextProcessFromIPCConnection(connection)) {
            contextConnection->didReceiveMessage(connection, decoder);
            return;
        }
    }
#endif

    ASSERT_NOT_REACHED();
}

void StorageToWebProcessConnection::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageReceiverName() == Messages::StorageToWebProcessConnection::messageReceiverName()) {
        didReceiveSyncStorageToWebProcessConnectionMessage(connection, decoder, replyEncoder);
        return;
    }

#if ENABLE(SERVICE_WORKER)
    if (decoder.messageReceiverName() == Messages::WebSWServerConnection::messageReceiverName()) {
        if (auto swConnection = m_swConnections.get(makeObjectIdentifier<SWServerConnectionIdentifierType>(decoder.destinationID())))
            swConnection->didReceiveSyncMessage(connection, decoder, replyEncoder);
        return;
    }
#endif

    ASSERT_NOT_REACHED();
}

void StorageToWebProcessConnection::didClose(IPC::Connection& connection)
{
    UNUSED_PARAM(connection);

#if ENABLE(SERVICE_WORKER)
    if (RefPtr<WebSWServerToContextConnection> serverToContextConnection = StorageProcess::singleton().connectionToContextProcessFromIPCConnection(connection)) {
        // Service Worker process exited.
        StorageProcess::singleton().connectionToContextProcessWasClosed(serverToContextConnection.releaseNonNull());
        return;
    }
#endif

#if ENABLE(INDEXED_DATABASE)
    auto idbConnections = m_webIDBConnections;
    for (auto& connection : idbConnections.values())
        connection->disconnectedFromWebProcess();

    m_webIDBConnections.clear();
#endif

#if ENABLE(SERVICE_WORKER)
    unregisterSWConnections();
#endif
}

void StorageToWebProcessConnection::didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference messageReceiverName, IPC::StringReference messageName)
{

}

#if ENABLE(SERVICE_WORKER)

void StorageToWebProcessConnection::unregisterSWConnections()
{
    auto swConnections = WTFMove(m_swConnections);
    for (auto& swConnection : swConnections.values()) {
        if (swConnection)
            swConnection->server().removeConnection(swConnection->identifier());
    }
}

void StorageToWebProcessConnection::establishSWServerConnection(SessionID sessionID, SWServerConnectionIdentifier& serverConnectionIdentifier)
{
    auto& server = StorageProcess::singleton().swServerForSession(sessionID);
    auto connection = std::make_unique<WebSWServerConnection>(server, m_connection.get(), sessionID);

    serverConnectionIdentifier = connection->identifier();
    LOG(ServiceWorker, "StorageToWebProcessConnection::establishSWServerConnection - %s", serverConnectionIdentifier.loggingString().utf8().data());

    ASSERT(!m_swConnections.contains(serverConnectionIdentifier));
    m_swConnections.add(serverConnectionIdentifier, makeWeakPtr(*connection));
    server.addConnection(WTFMove(connection));
}

#endif

#if ENABLE(INDEXED_DATABASE)
static uint64_t generateIDBConnectionToServerIdentifier()
{
    ASSERT(RunLoop::isMain());
    static uint64_t identifier = 0;
    return ++identifier;
}

void StorageToWebProcessConnection::establishIDBConnectionToServer(SessionID sessionID, uint64_t& serverConnectionIdentifier)
{
    serverConnectionIdentifier = generateIDBConnectionToServerIdentifier();
    LOG(IndexedDB, "StorageToWebProcessConnection::establishIDBConnectionToServer - %" PRIu64, serverConnectionIdentifier);
    ASSERT(!m_webIDBConnections.contains(serverConnectionIdentifier));

    m_webIDBConnections.set(serverConnectionIdentifier, WebIDBConnectionToClient::create(*this, serverConnectionIdentifier, sessionID));
}

void StorageToWebProcessConnection::removeIDBConnectionToServer(uint64_t serverConnectionIdentifier)
{
    ASSERT(m_webIDBConnections.contains(serverConnectionIdentifier));

    auto connection = m_webIDBConnections.take(serverConnectionIdentifier);
    connection->disconnectedFromWebProcess();
}
#endif

} // namespace WebKit
