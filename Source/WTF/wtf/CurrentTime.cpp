/*
 * Copyright (C) 2006-2016 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Google Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <wtf/MonotonicTime.h>

#include <time.h>
#include <wtf/WallTime.h>

#if OS(DARWIN)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mutex>
#include <sys/time.h>
#elif OS(WINDOWS)

// Windows is first since we want to use hires timers, despite USE(CF)
// being defined.
// If defined, WIN32_LEAN_AND_MEAN disables timeBeginPeriod/timeEndPeriod.
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdint.h>
#else
#include <sys/time.h>
#endif

#if OS(FUCHSIA)
#include <zircon/syscalls.h>
#endif

#if PLATFORM(MUI)
extern long get_DST_offset(void);
extern long get_GMT_offset(void);
#endif

#if USE(GLIB)
#include <glib.h>
#endif

namespace WTF {

#if OS(WINDOWS)

// Number of 100 nanosecond between January 1, 1601 and January 1, 1970.
static const ULONGLONG epochBias = 116444736000000000ULL;
static const double hundredsOfNanosecondsPerMillisecond = 10000;

static double lowResUTCTime()
{
    FILETIME fileTime;

    GetSystemTimeAsFileTime(&fileTime);

    // As per Windows documentation for FILETIME, copy the resulting FILETIME structure to a
    // ULARGE_INTEGER structure using memcpy (using memcpy instead of direct assignment can
    // prevent alignment faults on 64-bit Windows).

    ULARGE_INTEGER dateTime;
    memcpy(&dateTime, &fileTime, sizeof(dateTime));

    // Windows file times are in 100s of nanoseconds.
    return (dateTime.QuadPart - epochBias) / hundredsOfNanosecondsPerMillisecond;
}

#if USE(QUERY_PERFORMANCE_COUNTER)

static LARGE_INTEGER qpcFrequency;
static bool syncedTime;

static double highResUpTime()
{
    // We use QPC, but only after sanity checking its result, due to bugs:
    // http://support.microsoft.com/kb/274323
    // http://support.microsoft.com/kb/895980
    // http://msdn.microsoft.com/en-us/library/ms644904.aspx ("...you can get different results on different processors due to bugs in the basic input/output system (BIOS) or the hardware abstraction layer (HAL)."

    static LARGE_INTEGER qpcLast;
    static DWORD tickCountLast;
    static bool inited;

    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
#if defined(_M_IX86) || defined(__i386__)
    DWORD tickCount = GetTickCount();
#else
    ULONGLONG tickCount = GetTickCount64();
#endif

    if (inited) {
        __int64 qpcElapsed = ((qpc.QuadPart - qpcLast.QuadPart) * 1000) / qpcFrequency.QuadPart;
        __int64 tickCountElapsed;
        if (tickCount >= tickCountLast)
            tickCountElapsed = (tickCount - tickCountLast);
        else {
#if COMPILER(MINGW)
            __int64 tickCountLarge = tickCount + 0x100000000ULL;
#else
            __int64 tickCountLarge = tickCount + 0x100000000I64;
#endif
            tickCountElapsed = tickCountLarge - tickCountLast;
        }

        // force a re-sync if QueryPerformanceCounter differs from GetTickCount by more than 500ms.
        // (500ms value is from http://support.microsoft.com/kb/274323)
        __int64 diff = tickCountElapsed - qpcElapsed;
        if (diff > 500 || diff < -500)
            syncedTime = false;
    } else
        inited = true;

    qpcLast = qpc;
    tickCountLast = tickCount;

    return (1000.0 * qpc.QuadPart) / static_cast<double>(qpcFrequency.QuadPart);
}

static bool qpcAvailable()
{
    static bool available;
    static bool checked;

    if (checked)
        return available;

    available = QueryPerformanceFrequency(&qpcFrequency);
    checked = true;
    return available;
}

static inline double currentTime()
{
    // Use a combination of ftime and QueryPerformanceCounter.
    // ftime returns the information we want, but doesn't have sufficient resolution.
    // QueryPerformanceCounter has high resolution, but is only usable to measure time intervals.
    // To combine them, we call ftime and QueryPerformanceCounter initially. Later calls will use QueryPerformanceCounter
    // by itself, adding the delta to the saved ftime.  We periodically re-sync to correct for drift.
    static double syncLowResUTCTime;
    static double syncHighResUpTime;
    static double lastUTCTime;

    double lowResTime = lowResUTCTime();

    if (!qpcAvailable())
        return lowResTime / 1000.0;

    double highResTime = highResUpTime();

    if (!syncedTime) {
        timeBeginPeriod(1); // increase time resolution around low-res time getter
        syncLowResUTCTime = lowResTime = lowResUTCTime();
        timeEndPeriod(1); // restore time resolution
        syncHighResUpTime = highResTime;
        syncedTime = true;
    }

    double highResElapsed = highResTime - syncHighResUpTime;
    double utc = syncLowResUTCTime + highResElapsed;

    // force a clock re-sync if we've drifted
    double lowResElapsed = lowResTime - syncLowResUTCTime;
    const double maximumAllowedDriftMsec = 15.625 * 2.0; // 2x the typical low-res accuracy
    if (fabs(highResElapsed - lowResElapsed) > maximumAllowedDriftMsec)
        syncedTime = false;

    // make sure time doesn't run backwards (only correct if difference is < 2 seconds, since DST or clock changes could occur)
    const double backwardTimeLimit = 2000.0;
    if (utc < lastUTCTime && (lastUTCTime - utc) < backwardTimeLimit)
        return lastUTCTime / 1000.0;
    lastUTCTime = utc;
    return utc / 1000.0;
}

#else

static inline double currentTime()
{
    static bool init = false;
    static double lastTime;
    static DWORD lastTickCount;
    if (!init) {
        lastTime = lowResUTCTime();
        lastTickCount = GetTickCount();
        init = true;
        return lastTime;
    }

    DWORD tickCountNow = GetTickCount();
    DWORD elapsed = tickCountNow - lastTickCount;
    double timeNow = lastTime + (double)elapsed / 1000.;
    if (elapsed >= 0x7FFFFFFF) {
        lastTime = timeNow;
        lastTickCount = tickCountNow;
    }
    return timeNow;
}

#endif // USE(QUERY_PERFORMANCE_COUNTER)

#elif USE(GLIB)

// Note: GTK on Windows will pick up the PLATFORM(WIN) implementation above which provides
// better accuracy compared with Windows implementation of g_get_current_time:
// (http://www.google.com/codesearch/p?hl=en#HHnNRjks1t0/glib-2.5.2/glib/gmain.c&q=g_get_current_time).
// Non-Windows GTK builds could use gettimeofday() directly but for the sake of consistency lets use GTK function.
static inline double currentTime()
{
    GTimeVal now;
    g_get_current_time(&now);
    return static_cast<double>(now.tv_sec) + static_cast<double>(now.tv_usec / 1000000.0);
}

#else

static inline double currentTime()
{
    struct timeval now;

	gettimeofday(&now, 0);
#if PLATFORM(MUI)
	return now.tv_sec + now.tv_usec / 1000000.0 + (double) (get_GMT_offset()) + (double) (get_DST_offset());
#else
	return now.tv_sec + now.tv_usec / 1000000.0;
#endif
}

#endif

WallTime WallTime::now()
{
    return fromRawSeconds(currentTime());
}

MonotonicTime MonotonicTime::now()
{
#if USE(GLIB)
    return fromRawSeconds(static_cast<double>(g_get_monotonic_time() / 1000000.0));
#elif OS(DARWIN)
    // Based on listing #2 from Apple QA 1398, but modified to be thread-safe.
    static mach_timebase_info_data_t timebaseInfo;
    static std::once_flag initializeTimerOnceFlag;
    std::call_once(initializeTimerOnceFlag, [] {
        kern_return_t kr = mach_timebase_info(&timebaseInfo);
        ASSERT_UNUSED(kr, kr == KERN_SUCCESS);
        ASSERT(timebaseInfo.denom);
    });

    return fromRawSeconds((mach_absolute_time() * timebaseInfo.numer) / (1.0e9 * timebaseInfo.denom));
#elif OS(FUCHSIA)
    return fromRawSeconds(zx_clock_get_monotonic() / static_cast<double>(ZX_SEC(1)));
#elif OS(LINUX) || OS(FREEBSD) || OS(OPENBSD) || OS(NETBSD)
    struct timespec ts { };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return fromRawSeconds(static_cast<double>(ts.tv_sec) + ts.tv_nsec / 1.0e9);
#else
    static double lastTime = 0;
    double currentTimeNow = currentTime();
    if (currentTimeNow < lastTime)
        return lastTime;
    lastTime = currentTimeNow;
    return fromRawSeconds(currentTimeNow);
#endif
}

} // namespace WTF
