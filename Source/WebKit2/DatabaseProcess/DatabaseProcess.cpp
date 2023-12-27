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
#include "DatabaseProcess.h"

#if ENABLE(DATABASE_PROCESS)

#include "AsyncTask.h"
#include "DatabaseProcessCreationParameters.h"
#include "DatabaseProcessMessages.h"
#include "DatabaseProcessProxyMessages.h"
#include "DatabaseToWebProcessConnection.h"
#include "UniqueIDBDatabase.h"
#include "WebCrossThreadCopier.h"
#include "WebOriginDataManager.h"
#include "WebOriginDataManagerMessages.h"
#include "WebOriginDataManagerProxyMessages.h"
#include "WebsiteData.h"
#include <WebCore/FileSystem.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/TextEncoding.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace WebKit {

DatabaseProcess& DatabaseProcess::singleton()
{
    static NeverDestroyed<DatabaseProcess> databaseProcess;
    return databaseProcess;
}

DatabaseProcess::DatabaseProcess()
    : m_queue(WorkQueue::create("com.apple.WebKit.DatabaseProcess"))
    , m_webOriginDataManager(std::make_unique<WebOriginDataManager>(*this, *this))
{
    // Make sure the UTF8Encoding encoding and the text encoding maps have been built on the main thread before a background thread needs it.
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=135365 - Need a more explicit way of doing this besides accessing the UTF8Encoding.
    UTF8Encoding();
}

DatabaseProcess::~DatabaseProcess()
{
}

void DatabaseProcess::initializeConnection(IPC::Connection* connection)
{
    ChildProcess::initializeConnection(connection);
}

bool DatabaseProcess::shouldTerminate()
{
    return true;
}

void DatabaseProcess::didClose(IPC::Connection&)
{
    RunLoop::current().stop();
}

void DatabaseProcess::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (messageReceiverMap().dispatchMessage(connection, decoder))
        return;

    if (decoder.messageReceiverName() == Messages::DatabaseProcess::messageReceiverName()) {
        didReceiveDatabaseProcessMessage(connection, decoder);
        return;
    }
}

void DatabaseProcess::didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference, IPC::StringReference)
{
    RunLoop::current().stop();
}

PassRefPtr<UniqueIDBDatabase> DatabaseProcess::getOrCreateUniqueIDBDatabase(const UniqueIDBDatabaseIdentifier& identifier)
{
    auto addResult = m_idbDatabases.add(identifier, nullptr);

    if (!addResult.isNewEntry)
        return addResult.iterator->value;

    RefPtr<UniqueIDBDatabase> database = UniqueIDBDatabase::create(identifier);
    addResult.iterator->value = database.get();
    return database.release();
}

void DatabaseProcess::removeUniqueIDBDatabase(const UniqueIDBDatabase& database)
{
    const UniqueIDBDatabaseIdentifier& identifier = database.identifier();
    ASSERT(m_idbDatabases.contains(identifier));

    m_idbDatabases.remove(identifier);
}

void DatabaseProcess::initializeDatabaseProcess(const DatabaseProcessCreationParameters& parameters)
{
    // *********
    // IMPORTANT: Do not change the directory structure for indexed databases on disk without first consulting a reviewer from Apple (<rdar://problem/17454712>)
    // *********

    m_indexedDatabaseDirectory = parameters.indexedDatabaseDirectory;
    SandboxExtension::consumePermanently(parameters.indexedDatabaseDirectoryExtensionHandle);

    ensureIndexedDatabaseRelativePathExists(StringImpl::empty());
}

void DatabaseProcess::ensureIndexedDatabaseRelativePathExists(const String& relativePath)
{
    postDatabaseTask(createAsyncTask(*this, &DatabaseProcess::ensurePathExists, absoluteIndexedDatabasePathFromDatabaseRelativePath(relativePath)));
}

void DatabaseProcess::ensurePathExists(const String& path)
{
    ASSERT(!RunLoop::isMain());

    if (!makeAllDirectories(path))
        LOG_ERROR("Failed to make all directories for path '%s'", path.utf8().data());
}

String DatabaseProcess::absoluteIndexedDatabasePathFromDatabaseRelativePath(const String& relativePath)
{
    // FIXME: pathByAppendingComponent() was originally designed to append individual atomic components.
    // We don't have a function designed to append a multi-component subpath, but we should.
    return pathByAppendingComponent(m_indexedDatabaseDirectory, relativePath);
}

void DatabaseProcess::postDatabaseTask(std::unique_ptr<AsyncTask> task)
{
    ASSERT(RunLoop::isMain());

    MutexLocker locker(m_databaseTaskMutex);

    m_databaseTasks.append(WTF::move(task));

    m_queue->dispatch([this] {
        performNextDatabaseTask();
    });
}

void DatabaseProcess::performNextDatabaseTask()
{
    ASSERT(!RunLoop::isMain());

    std::unique_ptr<AsyncTask> task;
    {
        MutexLocker locker(m_databaseTaskMutex);
        ASSERT(!m_databaseTasks.isEmpty());
        task = m_databaseTasks.takeFirst();
    }

    task->performTask();
}

void DatabaseProcess::createDatabaseToWebProcessConnection()
{
#if OS(DARWIN)
    // Create the listening port.
    mach_port_t listeningPort;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &listeningPort);

    // Create a listening connection.
    RefPtr<DatabaseToWebProcessConnection> connection = DatabaseToWebProcessConnection::create(IPC::Connection::Identifier(listeningPort));
    m_databaseToWebProcessConnections.append(connection.release());

    IPC::Attachment clientPort(listeningPort, MACH_MSG_TYPE_MAKE_SEND);
    parentProcessConnection()->send(Messages::DatabaseProcessProxy::DidCreateDatabaseToWebProcessConnection(clientPort), 0);
#elif USE(UNIX_DOMAIN_SOCKETS)
    IPC::Connection::SocketPair socketPair = IPC::Connection::createPlatformConnection();
    m_databaseToWebProcessConnections.append(DatabaseToWebProcessConnection::create(socketPair.server));
    parentProcessConnection()->send(Messages::DatabaseProcessProxy::DidCreateDatabaseToWebProcessConnection(IPC::Attachment(socketPair.client)), 0);
#else
    notImplemented();
#endif
}

void DatabaseProcess::fetchWebsiteData(SessionID, uint64_t websiteDataTypes, uint64_t callbackID)
{
    struct CallbackAggregator final : public ThreadSafeRefCounted<CallbackAggregator> {
        explicit CallbackAggregator(std::function<void (WebsiteData)> completionHandler)
            : m_completionHandler(WTF::move(completionHandler))
        {
        }

        ~CallbackAggregator()
        {
            ASSERT(RunLoop::isMain());

            auto completionHandler = WTF::move(m_completionHandler);
            auto websiteData = WTF::move(m_websiteData);

            RunLoop::main().dispatch([completionHandler, websiteData] {
                completionHandler(websiteData);
            });
        }

        std::function<void (WebsiteData)> m_completionHandler;
        WebsiteData m_websiteData;
    };

    RefPtr<CallbackAggregator> callbackAggregator = adoptRef(new CallbackAggregator([this, callbackID](WebsiteData websiteData) {
        parentProcessConnection()->send(Messages::DatabaseProcessProxy::DidFetchWebsiteData(callbackID, websiteData), 0);
    }));

    if (websiteDataTypes & WebsiteDataTypeIndexedDBDatabases) {
        // FIXME: Pick the right database store based on the session ID.
        postDatabaseTask(std::make_unique<AsyncTask>([callbackAggregator, websiteDataTypes, this] {

            Vector<RefPtr<SecurityOrigin>> securityOrigins;

            for (const auto& originData : getIndexedDatabaseOrigins())
                securityOrigins.append(originData.securityOrigin());

            RunLoop::main().dispatch([callbackAggregator, securityOrigins] {
                for (const auto& securityOrigin : securityOrigins)
                    callbackAggregator->m_websiteData.entries.append(WebsiteData::Entry { securityOrigin, WebsiteDataTypeIndexedDBDatabases });
            });
        }));
    }
}

void DatabaseProcess::deleteWebsiteData(WebCore::SessionID, uint64_t websiteDataTypes, std::chrono::system_clock::time_point modifiedSince, uint64_t callbackID)
{
    struct CallbackAggregator final : public ThreadSafeRefCounted<CallbackAggregator> {
        explicit CallbackAggregator(std::function<void ()> completionHandler)
        : m_completionHandler(WTF::move(completionHandler))
        {
        }

        ~CallbackAggregator()
        {
            ASSERT(RunLoop::isMain());

            RunLoop::main().dispatch(WTF::move(m_completionHandler));
        }

        std::function<void ()> m_completionHandler;
    };

    RefPtr<CallbackAggregator> callbackAggregator = adoptRef(new CallbackAggregator([this, callbackID]() {
        parentProcessConnection()->send(Messages::DatabaseProcessProxy::DidDeleteWebsiteData(callbackID), 0);
    }));

    if (websiteDataTypes & WebsiteDataTypeIndexedDBDatabases) {
        postDatabaseTask(std::make_unique<AsyncTask>([this, callbackAggregator, modifiedSince] {
            double startDate = std::chrono::system_clock::to_time_t(modifiedSince);

            deleteIndexedDatabaseEntriesModifiedBetweenDates(startDate, std::numeric_limits<double>::max());
            RunLoop::main().dispatch([callbackAggregator] { });
        }));
    }
}

void DatabaseProcess::deleteWebsiteDataForOrigins(WebCore::SessionID, uint64_t websiteDataTypes, const Vector<SecurityOriginData>& origins, uint64_t callbackID)
{
    struct CallbackAggregator final : public ThreadSafeRefCounted<CallbackAggregator> {
        explicit CallbackAggregator(std::function<void ()> completionHandler)
            : m_completionHandler(WTF::move(completionHandler))
        {
        }

        ~CallbackAggregator()
        {
            ASSERT(RunLoop::isMain());

            RunLoop::main().dispatch(WTF::move(m_completionHandler));
        }

        std::function<void ()> m_completionHandler;
    };

    RefPtr<CallbackAggregator> callbackAggregator = adoptRef(new CallbackAggregator([this, callbackID]() {
        parentProcessConnection()->send(Messages::DatabaseProcessProxy::DidDeleteWebsiteDataForOrigins(callbackID), 0);
    }));

    if (websiteDataTypes & WebsiteDataTypeIndexedDBDatabases) {
        postDatabaseTask(std::make_unique<AsyncTask>([this, origins, callbackAggregator] {
            for (const auto& origin: origins)
                deleteIndexedDatabaseEntriesForOrigin(origin);

            RunLoop::main().dispatch([callbackAggregator] { });
        }));
    }
}

Vector<SecurityOriginData> DatabaseProcess::getIndexedDatabaseOrigins()
{
    Vector<SecurityOriginData> results;

    if (m_indexedDatabaseDirectory.isEmpty()) {
        return results;
    }

    Vector<String> originPaths = listDirectory(m_indexedDatabaseDirectory, "*");
    for (auto& originPath : originPaths) {
        URL url;
        url.setProtocol(ASCIILiteral("file"));
        url.setPath(originPath);

        String databaseIdentifier = url.lastPathComponent();

        RefPtr<SecurityOrigin> securityOrigin = SecurityOrigin::maybeCreateFromDatabaseIdentifier(databaseIdentifier);
        if (!securityOrigin)
            continue;

        results.append(SecurityOriginData::fromSecurityOrigin(*securityOrigin));
    }

    return results;
}

static void removeAllDatabasesForOriginPath(const String& originPath, double startDate, double endDate)
{
    // FIXME: We should also close/invalidate any live handles to the database files we are about to delete.
    // Right now:
    //     - For read-only operations, they will continue functioning as normal on the unlinked file.
    //     - For write operations, they will start producing errors as SQLite notices the missing backing store.
    // This is tracked by https://bugs.webkit.org/show_bug.cgi?id=135347

    Vector<String> databasePaths = listDirectory(originPath, "*");

    for (auto& databasePath : databasePaths) {
        String databaseFile = pathByAppendingComponent(databasePath, "IndexedDB.sqlite3");

        if (!fileExists(databaseFile))
            continue;

        time_t modTime;
        getFileModificationTime(databaseFile, modTime);

        if (modTime < startDate || modTime > endDate)
            continue;

        deleteFile(databaseFile);
        deleteEmptyDirectory(databasePath);
    }

    deleteEmptyDirectory(originPath);
}

void DatabaseProcess::deleteIndexedDatabaseEntriesForOrigin(const SecurityOriginData& originData)
{
    if (m_indexedDatabaseDirectory.isEmpty())
        return;

    Ref<SecurityOrigin> origin = originData.securityOrigin();
    String databaseIdentifier = origin->databaseIdentifier();
    String originPath = pathByAppendingComponent(m_indexedDatabaseDirectory, databaseIdentifier);

    removeAllDatabasesForOriginPath(originPath, std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
}

void DatabaseProcess::deleteIndexedDatabaseEntriesModifiedBetweenDates(double startDate, double endDate)
{
    if (m_indexedDatabaseDirectory.isEmpty())
        return;

    Vector<String> originPaths = listDirectory(m_indexedDatabaseDirectory, "*");
    for (auto& originPath : originPaths)
        removeAllDatabasesForOriginPath(originPath, startDate, endDate);
}

void DatabaseProcess::deleteAllIndexedDatabaseEntries()
{
    if (m_indexedDatabaseDirectory.isEmpty())
        return;

    Vector<String> originPaths = listDirectory(m_indexedDatabaseDirectory, "*");
    for (auto& originPath : originPaths)
        removeAllDatabasesForOriginPath(originPath, std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
}

void DatabaseProcess::getOrigins(WKOriginDataTypes types, std::function<void (const Vector<SecurityOriginData>&)> completion)
{
    if (!(types & kWKWebSQLDatabaseOriginData)) {
        completion(Vector<SecurityOriginData>());
        return;
    }

    postDatabaseTask(std::make_unique<AsyncTask>([completion, this] {
        completion(getIndexedDatabaseOrigins());
    }));
}

void DatabaseProcess::deleteEntriesForOrigin(WKOriginDataTypes types, const SecurityOriginData& origin, std::function<void ()> completion)
{
    if (!(types & kWKWebSQLDatabaseOriginData)) {
        completion();
        return;
    }

    postDatabaseTask(std::make_unique<AsyncTask>([this, origin, completion] {
        deleteIndexedDatabaseEntriesForOrigin(origin);
        completion();
    }));
}

void DatabaseProcess::deleteEntriesModifiedBetweenDates(WKOriginDataTypes types, double startDate, double endDate, std::function<void ()> completion)
{
    if (!(types & kWKWebSQLDatabaseOriginData)) {
        completion();
        return;
    }

    postDatabaseTask(std::make_unique<AsyncTask>([this, startDate, endDate, completion] {
        deleteIndexedDatabaseEntriesModifiedBetweenDates(startDate, endDate);
        completion();
    }));
}

void DatabaseProcess::deleteAllEntries(WKOriginDataTypes types, std::function<void ()> completion)
{
    if (!(types & kWKWebSQLDatabaseOriginData)) {
        completion();
        return;
    }

    postDatabaseTask(std::make_unique<AsyncTask>([this, completion] {
        deleteAllIndexedDatabaseEntries();
        completion();
    }));
}

#if !PLATFORM(COCOA)
void DatabaseProcess::initializeProcess(const ChildProcessInitializationParameters&)
{
}

void DatabaseProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}

void DatabaseProcess::initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&)
{
}
#endif

} // namespace WebKit

#endif // ENABLE(DATABASE_PROCESS)
