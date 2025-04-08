/*
 * Copyright (C) 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * Copyright (C) 2017 NAVER Corp.
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

#include "config.h"
#include "CurlRequestScheduler.h"

#if USE(CURL)

#include "CurlRequestSchedulerClient.h"

#if PLATFORM(MUI)
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <unistd.h>
#include <bsdsocket/socketbasetags.h>
#include <aros/debug.h>
struct Library *SocketBase;
void init_SocketBase()
{
    SocketBase = OpenLibrary("bsdsocket.library", 4L);
    SocketBaseTags(
        SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))), (IPTR) &errno,
        SBTM_SETVAL(SBTC_LOGTAGPTR),       (IPTR) "cURL",
        TAG_DONE);
}
void close_SocketBase()
{
    CloseLibrary(SocketBase);
    SocketBase = NULL;
}
#endif

namespace WebCore {

CurlRequestScheduler::CurlRequestScheduler(long maxConnects, long maxTotalConnections, long maxHostConnections)
    : m_maxConnects(maxConnects)
    , m_maxTotalConnections(maxTotalConnections)
    , m_maxHostConnections(maxHostConnections)
{
}

bool CurlRequestScheduler::add(CurlRequestSchedulerClient* client)
{
    ASSERT(isMainThread());

    if (!client)
        return false;

    startTransfer(client);
    startThreadIfNeeded();

    return true;
}

void CurlRequestScheduler::cancel(CurlRequestSchedulerClient* client)
{
    ASSERT(isMainThread());

    if (!client)
        return;

    cancelTransfer(client);
}

void CurlRequestScheduler::callOnWorkerThread(WTF::Function<void()>&& task)
{
    {
        auto locker = holdLock(m_mutex);
        m_taskQueue.append(WTFMove(task));
    }

    startThreadIfNeeded();
}

void CurlRequestScheduler::startThreadIfNeeded()
{
    ASSERT(isMainThread());

    {
        auto locker = holdLock(m_mutex);
        if (m_runThread)
            return;
    }

    if (m_thread)
        m_thread->waitForCompletion();

    {
        auto locker = holdLock(m_mutex);
        m_runThread = true;
    }

    m_thread = Thread::create("curlThread", [this] {
#if PLATFORM(MUI)
        init_SocketBase();
        /* Increase priority so that network data is transported immediatelly */
        SetTaskPri(FindTask(NULL), 1);
#endif
        workerThread();

        auto locker = holdLock(m_mutex);
        m_runThread = false;
#if PLATFORM(MUI)
        close_SocketBase();
#endif
    });
}

void CurlRequestScheduler::stopThreadIfNoMoreJobRunning()
{
    ASSERT(!isMainThread());

#if !PLATFORM(MUI)
    /* Keep the original curlThread running until browser quits */
    auto locker = holdLock(m_mutex);
    if (m_activeJobs.size() || m_taskQueue.size())
        return;

    m_runThread = false;
#endif
}

#if PLATFORM(MUI)
void CurlRequestScheduler::stopCurlThread()
{
    stopThread();
}
#endif

void CurlRequestScheduler::stopThread()
{
    {
        auto locker = holdLock(m_mutex);
        m_runThread = false;
    }

    if (m_thread) {
        m_thread->waitForCompletion();
        m_thread = nullptr;
    }
}

void CurlRequestScheduler::executeTasks()
{
    ASSERT(!isMainThread());

    Vector<WTF::Function<void()>> taskQueue;

    {
        auto locker = holdLock(m_mutex);
        taskQueue = WTFMove(m_taskQueue);
    }

    for (auto& task : taskQueue)
        task();
}

void CurlRequestScheduler::workerThread()
{
    ASSERT(!isMainThread());

    m_curlMultiHandle = makeUnique<CurlMultiHandle>();
    m_curlMultiHandle->setMaxConnects(m_maxConnects);
    m_curlMultiHandle->setMaxTotalConnections(m_maxTotalConnections);
    m_curlMultiHandle->setMaxHostConnections(m_maxHostConnections);

    while (true) {
        {
            auto locker = holdLock(m_mutex);
            if (!m_runThread)
                break;
        }

        executeTasks();

        // Retry 'select' if it was interrupted by a process signal.
        int rc = 0;
        do {
            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            int maxfd = 0;

            const int selectTimeoutMS = 5;

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = selectTimeoutMS * 1000; // select waits microseconds

            m_curlMultiHandle->getFdSet(fdread, fdwrite, fdexcep, maxfd);

            // When the 3 file descriptors are empty, winsock will return -1
            // and bail out, stopping the file download. So make sure we
            // have valid file descriptors before calling select.
            if (maxfd >= 0)
#if PLATFORM(MUI)
                rc = WaitSelect(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout, nullptr);
            else {
                usleep(100 * 1000);
            }
#else
                rc = ::select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
#endif
        } while (rc == -1 && errno == EINTR);

        int activeCount = 0;
        while (m_curlMultiHandle->perform(activeCount) == CURLM_CALL_MULTI_PERFORM) { }

        // check the curl messages indicating completed transfers
        // and free their resources
        while (true) {
            int messagesInQueue = 0;
            CURLMsg* msg = m_curlMultiHandle->readInfo(messagesInQueue);
            if (!msg)
                break;

            ASSERT(msg->msg == CURLMSG_DONE);
            if (auto client = m_clientMaps.inlineGet(msg->easy_handle))
                completeTransfer(client, msg->data.result);
        }

        stopThreadIfNoMoreJobRunning();
    }

    m_curlMultiHandle = nullptr;
}

void CurlRequestScheduler::startTransfer(CurlRequestSchedulerClient* client)
{
    client->retain();

    auto task = [this, client]() {
        CURL* handle = client->setupTransfer();
        if (!handle) {
            completeTransfer(client, CURLE_FAILED_INIT);
            return;
        }

        m_curlMultiHandle->addHandle(handle);

        ASSERT(!m_clientMaps.contains(handle));
        m_clientMaps.set(handle, client);
    };

    auto locker = holdLock(m_mutex);
    m_activeJobs.add(client);
    m_taskQueue.append(WTFMove(task));
}

void CurlRequestScheduler::completeTransfer(CurlRequestSchedulerClient* client, CURLcode result)
{
    finalizeTransfer(client, [client, result]() {
        client->didCompleteTransfer(result);
    });
}

void CurlRequestScheduler::cancelTransfer(CurlRequestSchedulerClient* client)
{
    finalizeTransfer(client, [client]() {
        client->didCancelTransfer();
    });
}

void CurlRequestScheduler::finalizeTransfer(CurlRequestSchedulerClient* client, Function<void()> completionHandler)
{
    auto locker = holdLock(m_mutex);

    if (!m_activeJobs.contains(client))
        return;

    m_activeJobs.remove(client);

    auto task = [this, client, completionHandler = WTFMove(completionHandler)]() {
        if (client->handle()) {
            ASSERT(m_clientMaps.contains(client->handle()));
            m_clientMaps.remove(client->handle());
            m_curlMultiHandle->removeHandle(client->handle());
        }

        completionHandler();

        callOnMainThread([client]() {
            client->release();
        });
    };

    m_taskQueue.append(WTFMove(task));
}

}

#endif
