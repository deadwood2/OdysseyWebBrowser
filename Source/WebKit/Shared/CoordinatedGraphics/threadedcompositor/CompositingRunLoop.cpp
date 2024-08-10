/*
 * Copyright (C) 2014 Igalia S.L.
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
#include "CompositingRunLoop.h"

#if USE(COORDINATED_GRAPHICS_THREADED)

#include <wtf/HashMap.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/WorkQueue.h>

#if USE(GLIB_EVENT_LOOP)
#include <wtf/glib/RunLoopSourcePriority.h>
#endif

namespace WebKit {

class WorkQueuePool {
    WTF_MAKE_NONCOPYABLE(WorkQueuePool);
    friend NeverDestroyed<WorkQueuePool>;
public:
    static WorkQueuePool& singleton()
    {
        ASSERT(RunLoop::isMain());
        static NeverDestroyed<WorkQueuePool> workQueuePool;
        return workQueuePool;
    }

    void dispatch(void* context, Function<void ()>&& function)
    {
        ASSERT(RunLoop::isMain());
        getOrCreateWorkQueueForContext(context).dispatch(WTFMove(function));
    }

    RunLoop& runLoop(void* context)
    {
        return getOrCreateWorkQueueForContext(context).runLoop();
    }

    void invalidate(void* context)
    {
        auto workQueue = m_workQueueMap.take(context);
        ASSERT(workQueue);
        if (m_workQueueMap.isEmpty()) {
            m_sharedWorkQueue = nullptr;
            m_threadCount = 0;
        } else if (workQueue->hasOneRef())
            m_threadCount--;
    }

private:
    WorkQueuePool()
    {
        // FIXME: This is a sane default limit, but it should be configurable somehow.
        m_threadCountLimit = 1;
    }

    WorkQueue& getOrCreateWorkQueueForContext(void* context)
    {
        auto addResult = m_workQueueMap.add(context, nullptr);
        if (addResult.isNewEntry) {
            // FIXME: This is OK for now, and it works for a single-thread limit. But for configurations where more (but not unlimited)
            // threads could be used, one option would be to use a HashSet here and disperse the contexts across the available threads.
            if (m_threadCount >= m_threadCountLimit) {
                ASSERT(m_sharedWorkQueue);
                addResult.iterator->value = m_sharedWorkQueue;
            } else {
                addResult.iterator->value = WorkQueue::create("org.webkit.ThreadedCompositorWorkQueue");
                if (!m_threadCount)
                    m_sharedWorkQueue = addResult.iterator->value;
                m_threadCount++;
            }
        }

        return *addResult.iterator->value;
    }

    HashMap<void*, RefPtr<WorkQueue>> m_workQueueMap;
    RefPtr<WorkQueue> m_sharedWorkQueue;
    unsigned m_threadCount { 0 };
    unsigned m_threadCountLimit;
};

CompositingRunLoop::CompositingRunLoop(Function<void ()>&& updateFunction)
    : m_updateTimer(WorkQueuePool::singleton().runLoop(this), this, &CompositingRunLoop::updateTimerFired)
    , m_updateFunction(WTFMove(updateFunction))
{
#if USE(GLIB_EVENT_LOOP)
    m_updateTimer.setPriority(RunLoopSourcePriority::CompositingThreadUpdateTimer);
    m_updateTimer.setName("[WebKit] CompositingRunLoop");
#endif
}

CompositingRunLoop::~CompositingRunLoop()
{
    ASSERT(RunLoop::isMain());
    // Make sure the WorkQueue is deleted after the CompositingRunLoop, because m_updateTimer has a reference
    // of the WorkQueue run loop. Passing this is not a problem because the pointer will only be used as a
    // HashMap key by WorkQueuePool.
    RunLoop::main().dispatch([context = this] { WorkQueuePool::singleton().invalidate(context); });
}

void CompositingRunLoop::performTask(Function<void ()>&& function)
{
    ASSERT(RunLoop::isMain());
    WorkQueuePool::singleton().dispatch(this, WTFMove(function));
}

void CompositingRunLoop::performTaskSync(Function<void ()>&& function)
{
    ASSERT(RunLoop::isMain());
    LockHolder locker(m_dispatchSyncConditionMutex);
    WorkQueuePool::singleton().dispatch(this, [this, function = WTFMove(function)] {
        function();
        LockHolder locker(m_dispatchSyncConditionMutex);
        m_dispatchSyncCondition.notifyOne();
    });
    m_dispatchSyncCondition.wait(m_dispatchSyncConditionMutex);
}

void CompositingRunLoop::scheduleUpdate()
{
    LockHolder stateLocker(m_state.lock);
    scheduleUpdate(stateLocker);
}

void CompositingRunLoop::scheduleUpdate(LockHolder& stateLocker)
{
    // An update was requested. Depending on the state:
    //  - if Idle, enter the Scheduled state and start the update timer,
    //  - if Scheduled, do nothing,
    //  - if InProgress or PendingCompletion, mark an update as pending, meaning another
    //    update will be scheduled as soon as the current one is completed.

    UNUSED_PARAM(stateLocker);

    switch (m_state.update) {
    case UpdateState::Idle:
        m_state.update = UpdateState::Scheduled;
        m_updateTimer.startOneShot(0_s);
        return;
    case UpdateState::Scheduled:
        return;
    case UpdateState::InProgress:
    case UpdateState::PendingCompletion:
        m_state.pendingUpdate = true;
        return;
    }
}

void CompositingRunLoop::stopUpdates()
{
    // Stop everything.

    LockHolder locker(m_state.lock);
    m_updateTimer.stop();
    m_state.composition = CompositionState::Idle;
    m_state.update = UpdateState::Idle;
    m_state.pendingUpdate = false;
}

void CompositingRunLoop::compositionCompleted(LockHolder& stateLocker)
{
    // Composition has been signaled as completed, pushing the state into Idle.
    // Depending on the state of the scene update:
    //  - if Idle, Scheduled or InProgress, do nothing,
    //  - if PendingCompletion, schedule a new update in case a pending update was marked,
    //    or push the scene update state into Idle otherwise.

    UNUSED_PARAM(stateLocker);

    m_state.composition = CompositionState::Idle;

    switch (m_state.update) {
    case UpdateState::Idle:
    case UpdateState::Scheduled:
    case UpdateState::InProgress:
        return;
    case UpdateState::PendingCompletion:
        if (m_state.pendingUpdate) {
            m_state.pendingUpdate = false;
            m_state.update = UpdateState::Scheduled;
            m_updateTimer.startOneShot(0_s);
            return;
        }

        m_state.update = UpdateState::Idle;
        return;
    }
}

void CompositingRunLoop::updateCompleted(LockHolder& stateLocker)
{
    // Scene update has been signaled as completed. Depending on the state:
    //  - if Idle, Scheduled or InProgress, do nothing,
    //  - if InProgress, push the state into PendingCompletion if the composition state is
    //    InProgress, otherwise schedule a new update in case a pending update was marked,
    //    otherwise push the scene update state into Idle.

    UNUSED_PARAM(stateLocker);

    switch (m_state.update) {
    case UpdateState::Idle:
    case UpdateState::Scheduled:
        return;
    case UpdateState::InProgress:
        if (m_state.composition == CompositionState::InProgress) {
            m_state.update = UpdateState::PendingCompletion;
            return;
        }

        if (m_state.pendingUpdate) {
            m_state.pendingUpdate = false;
            m_state.update = UpdateState::Scheduled;
            m_updateTimer.startOneShot(0_s);
            return;
        }

        m_state.update = UpdateState::Idle;
        return;
    case UpdateState::PendingCompletion:
        return;
    }
}

void CompositingRunLoop::updateTimerFired()
{
    {
        // Both composition and scene update are now in progress.
        LockHolder locker(m_state.lock);
        m_state.composition = CompositionState::InProgress;
        m_state.update = UpdateState::InProgress;
    }
    m_updateFunction();
}

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS_THREADED)
