/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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

#ifndef AsyncTask_h
#define AsyncTask_h

#include "BAssert.h"
#include "Inline.h"
#include "Mutex.h"
#include <atomic>
#include <condition_variable>
#include <thread>

namespace bmalloc {

template<typename Object, typename Function>
class AsyncTask {
public:
    AsyncTask(Object&, const Function&);
    ~AsyncTask();

    void run();

private:
    enum State { Sleeping, Running, RunRequested };

    void runSlowCase();

    static void threadEntryPoint(AsyncTask*);
    void threadRunLoop();

    std::atomic<State> m_state;

    Mutex m_conditionMutex;
    std::condition_variable_any m_condition;

    std::thread m_thread;

    Object& m_object;
    Function m_function;
};

template<typename Object, typename Function>
AsyncTask<Object, Function>::AsyncTask(Object& object, const Function& function)
    : m_state(Running)
    , m_condition()
    , m_thread(std::thread(&AsyncTask::threadEntryPoint, this))
    , m_object(object)
    , m_function(function)
{
}

template<typename Object, typename Function>
AsyncTask<Object, Function>::~AsyncTask()
{
    // We'd like to mark our destructor deleted but C++ won't allow it because
    // we are an automatic member of Heap.
    RELEASE_BASSERT(0);
}

template<typename Object, typename Function>
inline void AsyncTask<Object, Function>::run()
{
    if (m_state == RunRequested)
        return;
    runSlowCase();
}

template<typename Object, typename Function>
NO_INLINE void AsyncTask<Object, Function>::runSlowCase()
{
    State oldState = m_state.exchange(RunRequested);
    if (oldState == RunRequested || oldState == Running)
        return;

    BASSERT(oldState == Sleeping);
    std::lock_guard<Mutex> lock(m_conditionMutex);
    m_condition.notify_all();
}

template<typename Object, typename Function>
void AsyncTask<Object, Function>::threadEntryPoint(AsyncTask* asyncTask)
{
    asyncTask->threadRunLoop();
}

template<typename Object, typename Function>
void AsyncTask<Object, Function>::threadRunLoop()
{
    // This loop ratchets downward from most active to least active state. While
    // we ratchet downward, any other thread may reset our state.

    // We require any state change while we are sleeping to signal to our
    // condition variable and wake us up.

    while (1) {
        State expectedState = RunRequested;
        if (m_state.compare_exchange_weak(expectedState, Running))
            (m_object.*m_function)();

        expectedState = Running;
        if (m_state.compare_exchange_weak(expectedState, Sleeping)) {
            std::unique_lock<Mutex> lock(m_conditionMutex);
            m_condition.wait(lock, [&]() { return m_state != Sleeping; });
        }
    }
}

} // namespace bmalloc

#endif // AsyncTask_h
