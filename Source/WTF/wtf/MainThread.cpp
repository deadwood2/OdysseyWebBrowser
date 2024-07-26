/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MainThread.h"

#include "CurrentTime.h"
#include "Deque.h"
#include "StdLibExtras.h"
#include "Threading.h"
#include <mutex>
#include <wtf/NeverDestroyed.h>
#include <wtf/ThreadSpecific.h>

namespace WTF {

struct FunctionWithContext {
    MainThreadFunction* function;
    void* context;

    FunctionWithContext(MainThreadFunction* function = nullptr, void* context = nullptr)
        : function(function)
        , context(context)
    {
    }
    bool operator == (const FunctionWithContext& o)
    {
        return function == o.function && context == o.context;
    }
};

class FunctionWithContextFinder {
public:
    FunctionWithContextFinder(const FunctionWithContext& m) : m(m) {}
    bool operator() (FunctionWithContext& o) { return o == m; }
    FunctionWithContext m;
};


typedef Deque<FunctionWithContext> FunctionQueue;

static bool callbacksPaused; // This global variable is only accessed from main thread.
#if !OS(DARWIN) || PLATFORM(EFL) || PLATFORM(GTK)
static ThreadIdentifier mainThreadIdentifier;
#endif

static std::mutex& mainThreadFunctionQueueMutex()
{
    static NeverDestroyed<std::mutex> mutex;
    
    return mutex;
}

static FunctionQueue& functionQueue()
{
    static NeverDestroyed<FunctionQueue> functionQueue;
    return functionQueue;
}


#if !OS(DARWIN) || PLATFORM(EFL) || PLATFORM(GTK)

void initializeMainThread()
{
    static bool initializedMainThread;
    if (initializedMainThread)
        return;
    initializedMainThread = true;

    mainThreadIdentifier = currentThread();

    mainThreadFunctionQueueMutex();
    initializeMainThreadPlatform();
    initializeGCThreads();
}

#else

static pthread_once_t initializeMainThreadKeyOnce = PTHREAD_ONCE_INIT;

static void initializeMainThreadOnce()
{
    mainThreadFunctionQueueMutex();
    initializeMainThreadPlatform();
}

void initializeMainThread()
{
    pthread_once(&initializeMainThreadKeyOnce, initializeMainThreadOnce);
}

#if !USE(WEB_THREAD)
static void initializeMainThreadToProcessMainThreadOnce()
{
    mainThreadFunctionQueueMutex();
    initializeMainThreadToProcessMainThreadPlatform();
}

void initializeMainThreadToProcessMainThread()
{
    pthread_once(&initializeMainThreadKeyOnce, initializeMainThreadToProcessMainThreadOnce);
}
#else
static pthread_once_t initializeWebThreadKeyOnce = PTHREAD_ONCE_INIT;

static void initializeWebThreadOnce()
{
    initializeWebThreadPlatform();
}

void initializeWebThread()
{
    pthread_once(&initializeWebThreadKeyOnce, initializeWebThreadOnce);
}
#endif // !USE(WEB_THREAD)

#endif

// 0.1 sec delays in UI is approximate threshold when they become noticeable. Have a limit that's half of that.
static const auto maxRunLoopSuspensionTime = std::chrono::milliseconds(50);

void dispatchFunctionsFromMainThread()
{
    ASSERT(isMainThread());

    if (callbacksPaused)
        return;

    auto startTime = std::chrono::steady_clock::now();

    FunctionWithContext invocation;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(mainThreadFunctionQueueMutex());
            if (!functionQueue().size())
                break;
            invocation = functionQueue().takeFirst();
        }

        invocation.function(invocation.context);

        // If we are running accumulated functions for too long so UI may become unresponsive, we need to
        // yield so the user input can be processed. Otherwise user may not be able to even close the window.
        // This code has effect only in case the scheduleDispatchFunctionsOnMainThread() is implemented in a way that
        // allows input events to be processed before we are back here.
        if (std::chrono::steady_clock::now() - startTime > maxRunLoopSuspensionTime) {
            scheduleDispatchFunctionsOnMainThread();
            break;
        }
    }
}

void callOnMainThread(MainThreadFunction* function, void* context)
{
    ASSERT(function);
    bool needToSchedule = false;
    {
        std::lock_guard<std::mutex> lock(mainThreadFunctionQueueMutex());
        needToSchedule = functionQueue().size() == 0;
        functionQueue().append(FunctionWithContext(function, context));
    }
    if (needToSchedule)
        scheduleDispatchFunctionsOnMainThread();
}

void cancelCallOnMainThread(MainThreadFunction* function, void* context)
{
    ASSERT(function);

    std::lock_guard<std::mutex> lock(mainThreadFunctionQueueMutex());

    FunctionWithContextFinder pred(FunctionWithContext(function, context));

    while (true) {
        // We must redefine 'i' each pass, because the itererator's operator= 
        // requires 'this' to be valid, and remove() invalidates all iterators
        FunctionQueue::iterator i(functionQueue().findIf(pred));
        if (i == functionQueue().end())
            break;
        functionQueue().remove(i);
    }
}

static void callFunctionObject(void* context)
{
    auto function = std::unique_ptr<std::function<void ()>>(static_cast<std::function<void ()>*>(context));
    (*function)();
}

void callOnMainThread(std::function<void ()> function)
{
    callOnMainThread(callFunctionObject, std::make_unique<std::function<void ()>>(WTF::move(function)).release());
}

void setMainThreadCallbacksPaused(bool paused)
{
    ASSERT(isMainThread());

    if (callbacksPaused == paused)
        return;

    callbacksPaused = paused;

    if (!callbacksPaused)
        scheduleDispatchFunctionsOnMainThread();
}

#if !OS(DARWIN) || PLATFORM(EFL) || PLATFORM(GTK)
bool isMainThread()
{
    return currentThread() == mainThreadIdentifier;
}
#endif

#if !USE(WEB_THREAD)
bool canAccessThreadLocalDataForThread(ThreadIdentifier threadId)
{
    return threadId == currentThread();
}
#endif

#if ENABLE(PARALLEL_GC)
static ThreadSpecific<bool>* isGCThread;
#endif

void initializeGCThreads()
{
#if ENABLE(PARALLEL_GC)
    isGCThread = new ThreadSpecific<bool>();
#endif
}

#if ENABLE(PARALLEL_GC)
void registerGCThread()
{
    if (!isGCThread) {
        // This happens if we're running in a process that doesn't care about
        // MainThread.
        return;
    }

    **isGCThread = true;
}

bool isMainThreadOrGCThread()
{
    if (isGCThread->isSet() && **isGCThread)
        return true;

    return isMainThread();
}
#elif OS(DARWIN) && !PLATFORM(EFL) && !PLATFORM(GTK)
// This is necessary because JavaScriptCore.exp doesn't support preprocessor macros.
bool isMainThreadOrGCThread()
{
    return isMainThread();
}
#endif

} // namespace WTF
