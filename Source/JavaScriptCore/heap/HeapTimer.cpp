/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "HeapTimer.h"

#include "GCActivityCallback.h"
#include "IncrementalSweeper.h"
#include "JSObject.h"
#include "JSString.h"
#include "JSCInlines.h"
#include <wtf/MainThread.h>
#include <wtf/Threading.h>

#if PLATFORM(EFL)
#include <Ecore.h>
#elif USE(GLIB)
#include <glib.h>
#endif

namespace JSC {

#if USE(CF)
    
const CFTimeInterval HeapTimer::s_decade = 60 * 60 * 24 * 365 * 10;

static const void* retainAPILock(const void* info)
{
    static_cast<JSLock*>(const_cast<void*>(info))->ref();
    return info;
}

static void releaseAPILock(const void* info)
{
    static_cast<JSLock*>(const_cast<void*>(info))->deref();
}

HeapTimer::HeapTimer(VM* vm, CFRunLoopRef runLoop)
    : m_vm(vm)
    , m_runLoop(runLoop)
{
    memset(&m_context, 0, sizeof(CFRunLoopTimerContext));
    m_context.info = &vm->apiLock();
    m_context.retain = retainAPILock;
    m_context.release = releaseAPILock;
    m_timer = adoptCF(CFRunLoopTimerCreate(kCFAllocatorDefault, s_decade, s_decade, 0, 0, HeapTimer::timerDidFire, &m_context));
    CFRunLoopAddTimer(m_runLoop.get(), m_timer.get(), kCFRunLoopCommonModes);
}

HeapTimer::~HeapTimer()
{
    CFRunLoopRemoveTimer(m_runLoop.get(), m_timer.get(), kCFRunLoopCommonModes);
    CFRunLoopTimerInvalidate(m_timer.get());
}

void HeapTimer::timerDidFire(CFRunLoopTimerRef timer, void* context)
{
    JSLock* apiLock = static_cast<JSLock*>(context);
    apiLock->lock();

    VM* vm = apiLock->vm();
    // The VM has been destroyed, so we should just give up.
    if (!vm) {
        apiLock->unlock();
        return;
    }

    HeapTimer* heapTimer = 0;
    if (vm->heap.fullActivityCallback() && vm->heap.fullActivityCallback()->m_timer.get() == timer)
        heapTimer = vm->heap.fullActivityCallback();
    else if (vm->heap.edenActivityCallback() && vm->heap.edenActivityCallback()->m_timer.get() == timer)
        heapTimer = vm->heap.edenActivityCallback();
    else if (vm->heap.sweeper()->m_timer.get() == timer)
        heapTimer = vm->heap.sweeper();
    else
        RELEASE_ASSERT_NOT_REACHED();

    {
        JSLockHolder locker(vm);
        heapTimer->doWork();
    }

    apiLock->unlock();
}

#elif PLATFORM(EFL)

HeapTimer::HeapTimer(VM* vm)
    : m_vm(vm)
    , m_timer(0)
{
}

HeapTimer::~HeapTimer()
{
    stop();
}

Ecore_Timer* HeapTimer::add(double delay, void* agent)
{
    return ecore_timer_add(delay, reinterpret_cast<Ecore_Task_Cb>(timerEvent), agent);
}
    
void HeapTimer::stop()
{
    if (!m_timer)
        return;

    ecore_timer_del(m_timer);
    m_timer = 0;
}

bool HeapTimer::timerEvent(void* info)
{
    HeapTimer* agent = static_cast<HeapTimer*>(info);
    
    JSLockHolder locker(agent->m_vm);
    agent->doWork();
    agent->m_timer = 0;
    
    return ECORE_CALLBACK_CANCEL;
}

#elif USE(GLIB)

static GSourceFuncs heapTimerSourceFunctions = {
    nullptr, // prepare
    nullptr, // check
    // dispatch
    [](GSource* source, GSourceFunc callback, gpointer userData) -> gboolean
    {
        if (g_source_get_ready_time(source) == -1)
            return G_SOURCE_CONTINUE;
        g_source_set_ready_time(source, -1);
        return callback(userData);
    },
    nullptr, // finalize
    nullptr, // closure_callback
    nullptr, // closure_marshall
};

HeapTimer::HeapTimer(VM* vm)
    : m_vm(vm)
    , m_apiLock(&vm->apiLock())
    , m_timer(adoptGRef(g_source_new(&heapTimerSourceFunctions, sizeof(GSource))))
{
    g_source_set_name(m_timer.get(), "[JavaScriptCore] HeapTimer");
    g_source_set_callback(m_timer.get(), [](gpointer userData) -> gboolean {
        static_cast<HeapTimer*>(userData)->timerDidFire();
        return G_SOURCE_CONTINUE;
    }, this, nullptr);
    g_source_attach(m_timer.get(), g_main_context_get_thread_default());
}

HeapTimer::~HeapTimer()
{
    g_source_destroy(m_timer.get());
}

void HeapTimer::timerDidFire()
{
    m_apiLock->lock();

    if (!m_apiLock->vm()) {
        // The VM has been destroyed, so we should just give up.
        m_apiLock->unlock();
        return;
    }

    {
        JSLockHolder locker(m_vm);
        doWork();
    }

    m_apiLock->unlock();
}

#else
HeapTimer::HeapTimer(VM* vm)
    : m_vm(vm)
{
}

HeapTimer::~HeapTimer()
{
}

void HeapTimer::invalidate()
{
}

#endif
    

} // namespace JSC
