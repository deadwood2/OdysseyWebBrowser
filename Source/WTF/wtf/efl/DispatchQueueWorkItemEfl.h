/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef DispatchQueueWorkItemEfl_h
#define DispatchQueueWorkItemEfl_h

#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>
#include <wtf/FunctionDispatcher.h>
#include <wtf/RefCounted.h>
#include <wtf/WorkQueue.h>

class WorkItem {
public:
    WorkItem(Ref<WorkQueue>&& workQueue, Function<void ()>&& function)
        : m_workQueue(WTFMove(workQueue))
        , m_function(WTFMove(function))
    {
    }

    void dispatch() { m_function(); }

private:
    Ref<WorkQueue> m_workQueue;
    Function<void ()> m_function;
};

class TimerWorkItem : public WorkItem {
public:
    static std::unique_ptr<TimerWorkItem> create(Ref<WorkQueue>&& workQueue, Function<void ()>&& function, std::chrono::nanoseconds delayNanoSeconds)
    {
        ASSERT(delayNanoSeconds.count() >= 0);
        return std::unique_ptr<TimerWorkItem>(new TimerWorkItem(WTFMove(workQueue), WTFMove(function), monotonicallyIncreasingTime() * 1000000000.0 + delayNanoSeconds.count()));
    }
    double expirationTimeNanoSeconds() const { return m_expirationTimeNanoSeconds; }
    bool hasExpired(double currentTimeNanoSeconds) const { return currentTimeNanoSeconds >= m_expirationTimeNanoSeconds; }

protected:
    TimerWorkItem(Ref<WorkQueue>&& workQueue, Function<void ()>&& function, double expirationTimeNanoSeconds)
        : WorkItem(WTFMove(workQueue), WTFMove(function))
        , m_expirationTimeNanoSeconds(expirationTimeNanoSeconds)
    {
    }

private:
    double m_expirationTimeNanoSeconds;
};

#endif // DispatchQueueWorkItemEfl_h
