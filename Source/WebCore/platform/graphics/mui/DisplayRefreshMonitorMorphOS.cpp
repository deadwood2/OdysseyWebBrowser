/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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
#include "DisplayRefreshMonitorMorphOS.h"

namespace WebCore {

RefPtr<DisplayRefreshMonitorMorphOS> DisplayRefreshMonitorMorphOS::create(PlatformDisplayID displayID)
{
    return adoptRef(*new DisplayRefreshMonitorMorphOS(displayID));
}

DisplayRefreshMonitorMorphOS::DisplayRefreshMonitorMorphOS(PlatformDisplayID displayID)
    : DisplayRefreshMonitor(displayID)
    , m_timer(RunLoop::main(), this, &DisplayRefreshMonitorMorphOS::displayLinkFired)
{
}

bool DisplayRefreshMonitorMorphOS::requestRefreshCallback()
{
    if (!isActive())
        return false;
    m_timer.startOneShot(33_ms);

    LockHolder lock(mutex());
    setIsActive(true);
    setIsScheduled(true);
    return true;
}

void DisplayRefreshMonitorMorphOS::displayLinkFired()
{
    {
        LockHolder lock(mutex());
        if (!isPreviousFrameDone())
            return;

        setIsPreviousFrameDone(false);
    }

    handleDisplayRefreshedNotificationOnMainThread(this);
}

} // namespace WebCore
