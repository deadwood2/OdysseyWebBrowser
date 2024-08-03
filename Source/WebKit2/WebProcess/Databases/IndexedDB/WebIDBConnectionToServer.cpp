/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "WebIDBConnectionToServer.h"

#if ENABLE(INDEXED_DATABASE)

#include "DataReference.h"
#include "DatabaseToWebProcessConnectionMessages.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkProcessConnection.h"
#include "WebIDBConnectionToClientMessages.h"
#include "WebIDBResult.h"
#include "WebProcess.h"
#include "WebToDatabaseProcessConnection.h"
#include <WebCore/IDBConnectionToServer.h>
#include <WebCore/IDBCursorInfo.h>
#include <WebCore/IDBDatabaseException.h>
#include <WebCore/IDBError.h>
#include <WebCore/IDBIndexInfo.h>
#include <WebCore/IDBKeyRangeData.h>
#include <WebCore/IDBObjectStoreInfo.h>
#include <WebCore/IDBOpenDBRequest.h>
#include <WebCore/IDBRequestData.h>
#include <WebCore/IDBResourceIdentifier.h>
#include <WebCore/IDBResultData.h>
#include <WebCore/IDBTransactionInfo.h>
#include <WebCore/IDBValue.h>

using namespace WebCore;

namespace WebKit {

Ref<WebIDBConnectionToServer> WebIDBConnectionToServer::create()
{
    return adoptRef(*new WebIDBConnectionToServer);
}

WebIDBConnectionToServer::WebIDBConnectionToServer()
{
    relaxAdoptionRequirement();

    m_isOpenInServer = sendSync(Messages::DatabaseToWebProcessConnection::EstablishIDBConnectionToServer(), Messages::DatabaseToWebProcessConnection::EstablishIDBConnectionToServer::Reply(m_identifier));
    m_connectionToServer = IDBClient::IDBConnectionToServer::create(*this);
}

WebIDBConnectionToServer::~WebIDBConnectionToServer()
{
    if (m_isOpenInServer)
        send(Messages::DatabaseToWebProcessConnection::RemoveIDBConnectionToServer(m_identifier));
}

IPC::Connection* WebIDBConnectionToServer::messageSenderConnection()
{
    return &WebProcess::singleton().webToDatabaseProcessConnection()->connection();
}

IDBClient::IDBConnectionToServer& WebIDBConnectionToServer::coreConnectionToServer()
{
    return *m_connectionToServer;
}

void WebIDBConnectionToServer::deleteDatabase(const IDBRequestData& requestData)
{
    send(Messages::WebIDBConnectionToClient::DeleteDatabase(requestData));
}

void WebIDBConnectionToServer::openDatabase(const IDBRequestData& requestData)
{
    send(Messages::WebIDBConnectionToClient::OpenDatabase(requestData));
}

void WebIDBConnectionToServer::abortTransaction(const IDBResourceIdentifier& transactionIdentifier)
{
    send(Messages::WebIDBConnectionToClient::AbortTransaction(transactionIdentifier));
}

void WebIDBConnectionToServer::commitTransaction(const IDBResourceIdentifier& transactionIdentifier)
{
    send(Messages::WebIDBConnectionToClient::CommitTransaction(transactionIdentifier));
}

void WebIDBConnectionToServer::didFinishHandlingVersionChangeTransaction(uint64_t databaseConnectionIdentifier, const IDBResourceIdentifier& transactionIdentifier)
{
    send(Messages::WebIDBConnectionToClient::DidFinishHandlingVersionChangeTransaction(databaseConnectionIdentifier, transactionIdentifier));
}

void WebIDBConnectionToServer::createObjectStore(const IDBRequestData& requestData, const IDBObjectStoreInfo& info)
{
    send(Messages::WebIDBConnectionToClient::CreateObjectStore(requestData, info));
}

void WebIDBConnectionToServer::deleteObjectStore(const IDBRequestData& requestData, const String& objectStoreName)
{
    send(Messages::WebIDBConnectionToClient::DeleteObjectStore(requestData, objectStoreName));
}

void WebIDBConnectionToServer::clearObjectStore(const IDBRequestData& requestData, uint64_t objectStoreIdentifier)
{
    send(Messages::WebIDBConnectionToClient::ClearObjectStore(requestData, objectStoreIdentifier));
}

void WebIDBConnectionToServer::createIndex(const IDBRequestData& requestData, const IDBIndexInfo& info)
{
    send(Messages::WebIDBConnectionToClient::CreateIndex(requestData, info));
}

void WebIDBConnectionToServer::deleteIndex(const IDBRequestData& requestData, uint64_t objectStoreIdentifier, const String& indexName)
{
    send(Messages::WebIDBConnectionToClient::DeleteIndex(requestData, objectStoreIdentifier, indexName));
}

void WebIDBConnectionToServer::putOrAdd(const IDBRequestData& requestData, const IDBKeyData& keyData, const IDBValue& value, const IndexedDB::ObjectStoreOverwriteMode mode)
{
    send(Messages::WebIDBConnectionToClient::PutOrAdd(requestData, keyData, value, static_cast<unsigned>(mode)));
}

void WebIDBConnectionToServer::getRecord(const IDBRequestData& requestData, const IDBGetRecordData& getRecordData)
{
    send(Messages::WebIDBConnectionToClient::GetRecord(requestData, getRecordData));
}

void WebIDBConnectionToServer::getCount(const IDBRequestData& requestData, const IDBKeyRangeData& range)
{
    send(Messages::WebIDBConnectionToClient::GetCount(requestData, range));
}

void WebIDBConnectionToServer::deleteRecord(const IDBRequestData& requestData, const IDBKeyRangeData& range)
{
    send(Messages::WebIDBConnectionToClient::DeleteRecord(requestData, range));
}

void WebIDBConnectionToServer::openCursor(const IDBRequestData& requestData, const IDBCursorInfo& info)
{
    send(Messages::WebIDBConnectionToClient::OpenCursor(requestData, info));
}

void WebIDBConnectionToServer::iterateCursor(const IDBRequestData& requestData, const IDBKeyData& key, unsigned long count)
{
    send(Messages::WebIDBConnectionToClient::IterateCursor(requestData, key, count));
}

void WebIDBConnectionToServer::establishTransaction(uint64_t databaseConnectionIdentifier, const IDBTransactionInfo& info)
{
    send(Messages::WebIDBConnectionToClient::EstablishTransaction(databaseConnectionIdentifier, info));
}

void WebIDBConnectionToServer::databaseConnectionClosed(uint64_t databaseConnectionIdentifier)
{
    send(Messages::WebIDBConnectionToClient::DatabaseConnectionClosed(databaseConnectionIdentifier));
}

void WebIDBConnectionToServer::abortOpenAndUpgradeNeeded(uint64_t databaseConnectionIdentifier, const IDBResourceIdentifier& transactionIdentifier)
{
    send(Messages::WebIDBConnectionToClient::AbortOpenAndUpgradeNeeded(databaseConnectionIdentifier, transactionIdentifier));
}

void WebIDBConnectionToServer::didFireVersionChangeEvent(uint64_t databaseConnectionIdentifier, const IDBResourceIdentifier& requestIdentifier)
{
    send(Messages::WebIDBConnectionToClient::DidFireVersionChangeEvent(databaseConnectionIdentifier, requestIdentifier));
}

void WebIDBConnectionToServer::openDBRequestCancelled(const IDBRequestData& requestData)
{
    send(Messages::WebIDBConnectionToClient::OpenDBRequestCancelled(requestData));
}

void WebIDBConnectionToServer::confirmDidCloseFromServer(uint64_t databaseConnectionIdentifier)
{
    send(Messages::WebIDBConnectionToClient::ConfirmDidCloseFromServer(databaseConnectionIdentifier));
}

void WebIDBConnectionToServer::getAllDatabaseNames(const WebCore::SecurityOriginData& topOrigin, const WebCore::SecurityOriginData& openingOrigin, uint64_t callbackID)
{
    send(Messages::WebIDBConnectionToClient::GetAllDatabaseNames(m_identifier, topOrigin, openingOrigin, callbackID));
}

void WebIDBConnectionToServer::didDeleteDatabase(const IDBResultData& result)
{
    m_connectionToServer->didDeleteDatabase(result);
}

void WebIDBConnectionToServer::didOpenDatabase(const IDBResultData& result)
{
    m_connectionToServer->didOpenDatabase(result);
}

void WebIDBConnectionToServer::didAbortTransaction(const IDBResourceIdentifier& transactionIdentifier, const IDBError& error)
{
    m_connectionToServer->didAbortTransaction(transactionIdentifier, error);
}

void WebIDBConnectionToServer::didCommitTransaction(const IDBResourceIdentifier& transactionIdentifier, const IDBError& error)
{
    m_connectionToServer->didCommitTransaction(transactionIdentifier, error);
}

void WebIDBConnectionToServer::didCreateObjectStore(const IDBResultData& result)
{
    m_connectionToServer->didCreateObjectStore(result);
}

void WebIDBConnectionToServer::didDeleteObjectStore(const IDBResultData& result)
{
    m_connectionToServer->didDeleteObjectStore(result);
}

void WebIDBConnectionToServer::didClearObjectStore(const IDBResultData& result)
{
    m_connectionToServer->didClearObjectStore(result);
}

void WebIDBConnectionToServer::didCreateIndex(const IDBResultData& result)
{
    m_connectionToServer->didCreateIndex(result);
}

void WebIDBConnectionToServer::didDeleteIndex(const IDBResultData& result)
{
    m_connectionToServer->didDeleteIndex(result);
}

void WebIDBConnectionToServer::didPutOrAdd(const IDBResultData& result)
{
    m_connectionToServer->didPutOrAdd(result);
}

static void preregisterSandboxExtensionsIfNecessary(const WebIDBResult& result)
{
    auto resultType = result.resultData().type();
    if (resultType != IDBResultType::GetRecordSuccess && resultType != IDBResultType::OpenCursorSuccess && resultType != IDBResultType::IterateCursorSuccess) {
        ASSERT(resultType == IDBResultType::Error);
        return;
    }

    const auto& filePaths = result.resultData().getResult().value().blobFilePaths();

#if ENABLE(SANDBOX_EXTENSIONS)
    ASSERT(filePaths.size() == result.handles().size());
#endif

    if (!filePaths.isEmpty())
        WebProcess::singleton().networkConnection().connection().send(Messages::NetworkConnectionToWebProcess::PreregisterSandboxExtensionsForOptionallyFileBackedBlob(filePaths, result.handles()), 0);
}

void WebIDBConnectionToServer::didGetRecord(const WebIDBResult& result)
{
    preregisterSandboxExtensionsIfNecessary(result);
    m_connectionToServer->didGetRecord(result.resultData());
}

void WebIDBConnectionToServer::didGetCount(const IDBResultData& result)
{
    m_connectionToServer->didGetCount(result);
}

void WebIDBConnectionToServer::didDeleteRecord(const IDBResultData& result)
{
    m_connectionToServer->didDeleteRecord(result);
}

void WebIDBConnectionToServer::didOpenCursor(const WebIDBResult& result)
{
    preregisterSandboxExtensionsIfNecessary(result);
    m_connectionToServer->didOpenCursor(result.resultData());
}

void WebIDBConnectionToServer::didIterateCursor(const WebIDBResult& result)
{
    preregisterSandboxExtensionsIfNecessary(result);
    m_connectionToServer->didIterateCursor(result.resultData());
}

void WebIDBConnectionToServer::fireVersionChangeEvent(uint64_t uniqueDatabaseConnectionIdentifier, const IDBResourceIdentifier& requestIdentifier, uint64_t requestedVersion)
{
    m_connectionToServer->fireVersionChangeEvent(uniqueDatabaseConnectionIdentifier, requestIdentifier, requestedVersion);
}

void WebIDBConnectionToServer::didStartTransaction(const IDBResourceIdentifier& transactionIdentifier, const IDBError& error)
{
    m_connectionToServer->didStartTransaction(transactionIdentifier, error);
}

void WebIDBConnectionToServer::didCloseFromServer(uint64_t databaseConnectionIdentifier, const IDBError& error)
{
    m_connectionToServer->didCloseFromServer(databaseConnectionIdentifier, error);
}

void WebIDBConnectionToServer::notifyOpenDBRequestBlocked(const IDBResourceIdentifier& requestIdentifier, uint64_t oldVersion, uint64_t newVersion)
{
    m_connectionToServer->notifyOpenDBRequestBlocked(requestIdentifier, oldVersion, newVersion);
}

void WebIDBConnectionToServer::didGetAllDatabaseNames(uint64_t callbackID, const Vector<String>& databaseNames)
{
    m_connectionToServer->didGetAllDatabaseNames(callbackID, databaseNames);
}

void WebIDBConnectionToServer::connectionToServerLost()
{
    m_connectionToServer->connectionToServerLost({ WebCore::IDBDatabaseException::UnknownError, ASCIILiteral("An internal error was encountered in the Indexed Database server") });
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
