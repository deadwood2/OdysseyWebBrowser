/*
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2006-2016 Apple Inc.  All rights reserved.
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

#ifndef MainThreadSharedTimer_h
#define MainThreadSharedTimer_h

#include "SharedTimer.h"
#include <wtf/NeverDestroyed.h>

#if PLATFORM(GTK)
#include <wtf/RunLoop.h>
#endif

namespace WebCore {

class MainThreadSharedTimer final : public SharedTimer {
    friend class WTF::NeverDestroyed<MainThreadSharedTimer>;
public:
    static MainThreadSharedTimer& singleton();

    void setFiredFunction(std::function<void()>&&) override;
    void setFireInterval(Seconds) override;
    void stop() override;
    void invalidate() override;

    // FIXME: This should be private, but CF and Windows implementations
    // need to call this from non-member functions at the moment.
    void fired();

private:
    MainThreadSharedTimer();

    std::function<void()> m_firedFunction;
#if PLATFORM(GTK)
    RunLoop::Timer<MainThreadSharedTimer> m_timer;
#endif
};

} // namespace WebCore

#endif // MainThreadSharedTimer
