/*
 * Copyright (C) 2008-2017 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "WorkerRunLoop.h"
#include <memory>
#include <runtime/RuntimeFlags.h>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class ContentSecurityPolicyResponseHeaders;
class URL;
class NotificationClient;
class SecurityOrigin;
class SocketProvider;
class WorkerGlobalScope;
class WorkerLoaderProxy;
class WorkerReportingProxy;

enum class WorkerThreadStartMode {
    Normal,
    WaitForInspector,
};

namespace IDBClient {
class IDBConnectionProxy;
}

struct WorkerThreadStartupData;

class WorkerThread : public RefCounted<WorkerThread> {
public:
    virtual ~WorkerThread();

    bool start();
    void stop();

    ThreadIdentifier threadID() const { return m_thread ? m_thread->id() : 0; }
    WorkerRunLoop& runLoop() { return m_runLoop; }
    WorkerLoaderProxy& workerLoaderProxy() const { return m_workerLoaderProxy; }
    WorkerReportingProxy& workerReportingProxy() const { return m_workerReportingProxy; }

    // Number of active worker threads.
    WEBCORE_EXPORT static unsigned workerThreadCount();
    static void releaseFastMallocFreeMemoryInAllThreads();

#if ENABLE(NOTIFICATIONS)
    NotificationClient* getNotificationClient() { return m_notificationClient; }
    void setNotificationClient(NotificationClient* client) { m_notificationClient = client; }
#endif

    void startRunningDebuggerTasks();
    void stopRunningDebuggerTasks();
    
    JSC::RuntimeFlags runtimeFlags() const { return m_runtimeFlags; }

protected:
    WorkerThread(const URL&, const String& identifier, const String& userAgent, const String& sourceCode, WorkerLoaderProxy&, WorkerReportingProxy&, WorkerThreadStartMode, const ContentSecurityPolicyResponseHeaders&, bool shouldBypassMainWorldContentSecurityPolicy, const SecurityOrigin& topOrigin, MonotonicTime timeOrigin, IDBClient::IDBConnectionProxy*, SocketProvider*, JSC::RuntimeFlags);

    // Factory method for creating a new worker context for the thread.
    virtual Ref<WorkerGlobalScope> createWorkerGlobalScope(const URL&, const String& identifier, const String& userAgent, const ContentSecurityPolicyResponseHeaders&, bool shouldBypassMainWorldContentSecurityPolicy, Ref<SecurityOrigin>&& topOrigin, MonotonicTime timeOrigin) = 0;

    // Executes the event loop for the worker thread. Derived classes can override to perform actions before/after entering the event loop.
    virtual void runEventLoop();

    WorkerGlobalScope* workerGlobalScope() { return m_workerGlobalScope.get(); }

    IDBClient::IDBConnectionProxy* idbConnectionProxy();
    SocketProvider* socketProvider();

private:
    void workerThread();

    RefPtr<Thread> m_thread;
    WorkerRunLoop m_runLoop;
    WorkerLoaderProxy& m_workerLoaderProxy;
    WorkerReportingProxy& m_workerReportingProxy;
    JSC::RuntimeFlags m_runtimeFlags;
    bool m_pausedForDebugger { false };

    RefPtr<WorkerGlobalScope> m_workerGlobalScope;
    Lock m_threadCreationAndWorkerGlobalScopeMutex;

    std::unique_ptr<WorkerThreadStartupData> m_startupData;

#if ENABLE(NOTIFICATIONS)
    NotificationClient* m_notificationClient { nullptr };
#endif

#if ENABLE(INDEXED_DATABASE)
    RefPtr<IDBClient::IDBConnectionProxy> m_idbConnectionProxy;
#endif
    RefPtr<SocketProvider> m_socketProvider;
};

} // namespace WebCore
