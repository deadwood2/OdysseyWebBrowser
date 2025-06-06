/*
 * Copyright (C) 2003-2017 Apple Inc.  All rights reserved.
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

#include <wtf/Platform.h>

/*
   no namespaces because this file has to be includable from C and Objective-C

   Note, this file uses many GCC extensions, but it should be compatible with
   C, Objective C, C++, and Objective C++.

   For non-debug builds, everything is disabled by default except for "always
   on" logging. Defining any of the symbols explicitly prevents this from
   having any effect.
*/

#undef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <wtf/ExportMacros.h>

#if USE(OS_LOG)
#include <os/log.h>
#endif

#ifdef __cplusplus
#include <cstdlib>
#include <type_traits>

#if OS(WINDOWS)
#if !COMPILER(GCC_COMPATIBLE)
extern "C" void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
#endif
#include <intrin.h>
#endif
#endif

#ifdef NDEBUG
/* Disable ASSERT* macros in release mode. */
#define ASSERTIONS_DISABLED_DEFAULT 1
#else
#define ASSERTIONS_DISABLED_DEFAULT 0
#endif

#ifndef BACKTRACE_DISABLED
#define BACKTRACE_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ASSERT_DISABLED
#define ASSERT_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ASSERT_MSG_DISABLED
#define ASSERT_MSG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ASSERT_ARG_DISABLED
#define ASSERT_ARG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef FATAL_DISABLED
#define FATAL_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef ERROR_DISABLED
#define ERROR_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef LOG_DISABLED
#define LOG_DISABLED ASSERTIONS_DISABLED_DEFAULT
#endif

#ifndef RELEASE_LOG_DISABLED
#define RELEASE_LOG_DISABLED !(USE(OS_LOG))
#endif

#if COMPILER(GCC_COMPATIBLE)
#define WTF_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define WTF_PRETTY_FUNCTION __FUNCTION__
#endif

#if COMPILER(MINGW)
/* By default MinGW emits warnings when C99 format attributes are used, even if __USE_MINGW_ANSI_STDIO is defined */
#define WTF_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments) __attribute__((__format__(gnu_printf, formatStringArgument, extraArguments)))
#elif COMPILER(GCC_COMPATIBLE) && !defined(__OBJC__)
/* WTF logging functions can process %@ in the format string to log a NSObject* but the printf format attribute
   emits a warning when %@ is used in the format string.  Until <rdar://problem/5195437> is resolved we can't include
   the attribute when being used from Objective-C code in case it decides to use %@. */
#define WTF_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments) __attribute__((__format__(printf, formatStringArgument, extraArguments)))
#else
#define WTF_ATTRIBUTE_PRINTF(formatStringArgument, extraArguments)
#endif

#if PLATFORM(IOS_FAMILY)
/* For a project that uses WTF but has no config.h, we need to explicitly set the export defines here. */
#ifndef WTF_EXPORT_PRIVATE
#define WTF_EXPORT_PRIVATE
#endif
#endif // PLATFORM(IOS_FAMILY)

/* These helper functions are always declared, but not necessarily always defined if the corresponding function is disabled. */

#ifdef __cplusplus
extern "C" {
#endif

/* CRASH() - Raises a fatal error resulting in program termination and triggering either the debugger or the crash reporter.

   Use CRASH() in response to known, unrecoverable errors like out-of-memory.
   Macro is enabled in both debug and release mode.
   To test for unknown errors and verify assumptions, use ASSERT instead, to avoid impacting performance in release builds.

   Signals are ignored by the crash reporter on OS X so we must do better.
*/
#if COMPILER(GCC_COMPATIBLE) || COMPILER(MSVC)
#define NO_RETURN_DUE_TO_CRASH NO_RETURN
#else
#define NO_RETURN_DUE_TO_CRASH
#endif

typedef enum { WTFLogChannelOff, WTFLogChannelOn, WTFLogChannelOnWithAccumulation } WTFLogChannelState;
typedef enum { WTFLogLevelAlways, WTFLogLevelError, WTFLogLevelWarning, WTFLogLevelInfo, WTFLogLevelDebug } WTFLogLevel;

typedef struct {
    WTFLogChannelState state;
    const char* name;
    WTFLogLevel level;
#if !RELEASE_LOG_DISABLED
    const char* subsystem;
    __unsafe_unretained os_log_t osLogChannel;
#endif
} WTFLogChannel;

#define LOG_CHANNEL(name) JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, name)
#define LOG_CHANNEL_ADDRESS(name) &LOG_CHANNEL(name),
#define JOIN_LOG_CHANNEL_WITH_PREFIX(prefix, channel) JOIN_LOG_CHANNEL_WITH_PREFIX_LEVEL_2(prefix, channel)
#define JOIN_LOG_CHANNEL_WITH_PREFIX_LEVEL_2(prefix, channel) prefix ## channel

#define LOG_CHANNEL_WEBKIT_SUBSYSTEM "com.apple.WebKit"

#define DECLARE_LOG_CHANNEL(name) \
    extern WTFLogChannel LOG_CHANNEL(name);

#if !defined(DEFINE_LOG_CHANNEL)
#if RELEASE_LOG_DISABLED
#define DEFINE_LOG_CHANNEL(name, subsystem) \
    WTFLogChannel LOG_CHANNEL(name) = { WTFLogChannelOff, #name, WTFLogLevelError };
#else
#define DEFINE_LOG_CHANNEL(name, subsystem) \
    WTFLogChannel LOG_CHANNEL(name) = { WTFLogChannelOff, #name, WTFLogLevelError, subsystem, OS_LOG_DEFAULT };
#endif
#endif

WTF_EXPORT_PRIVATE void WTFReportNotImplementedYet(const char* file, int line, const char* function);
WTF_EXPORT_PRIVATE void WTFReportAssertionFailure(const char* file, int line, const char* function, const char* assertion);
WTF_EXPORT_PRIVATE void WTFReportAssertionFailureWithMessage(const char* file, int line, const char* function, const char* assertion, const char* format, ...) WTF_ATTRIBUTE_PRINTF(5, 6);
WTF_EXPORT_PRIVATE void WTFReportArgumentAssertionFailure(const char* file, int line, const char* function, const char* argName, const char* assertion);
WTF_EXPORT_PRIVATE void WTFReportFatalError(const char* file, int line, const char* function, const char* format, ...) WTF_ATTRIBUTE_PRINTF(4, 5);
WTF_EXPORT_PRIVATE void WTFReportError(const char* file, int line, const char* function, const char* format, ...) WTF_ATTRIBUTE_PRINTF(4, 5);
WTF_EXPORT_PRIVATE void WTFLog(WTFLogChannel*, const char* format, ...) WTF_ATTRIBUTE_PRINTF(2, 3);
WTF_EXPORT_PRIVATE void WTFLogVerbose(const char* file, int line, const char* function, WTFLogChannel*, const char* format, ...) WTF_ATTRIBUTE_PRINTF(5, 6);
WTF_EXPORT_PRIVATE void WTFLogAlwaysV(const char* format, va_list);
WTF_EXPORT_PRIVATE void WTFLogAlways(const char* format, ...) WTF_ATTRIBUTE_PRINTF(1, 2);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH void WTFLogAlwaysAndCrash(const char* format, ...) WTF_ATTRIBUTE_PRINTF(1, 2);
WTF_EXPORT_PRIVATE WTFLogChannel* WTFLogChannelByName(WTFLogChannel*[], size_t count, const char*);
WTF_EXPORT_PRIVATE void WTFInitializeLogChannelStatesFromString(WTFLogChannel*[], size_t count, const char*);
WTF_EXPORT_PRIVATE void WTFLogWithLevel(WTFLogChannel*, WTFLogLevel, const char* format, ...) WTF_ATTRIBUTE_PRINTF(3, 4);
WTF_EXPORT_PRIVATE void WTFSetLogChannelLevel(WTFLogChannel*, WTFLogLevel);
WTF_EXPORT_PRIVATE bool WTFWillLogWithLevel(WTFLogChannel*, WTFLogLevel);

WTF_EXPORT_PRIVATE void WTFGetBacktrace(void** stack, int* size);
WTF_EXPORT_PRIVATE void WTFReportBacktrace(void);
WTF_EXPORT_PRIVATE void WTFPrintBacktrace(void** stack, int size);
#if !RELEASE_LOG_DISABLED
WTF_EXPORT_PRIVATE void WTFReleaseLogStackTrace(WTFLogChannel*);
#endif

WTF_EXPORT_PRIVATE bool WTFIsDebuggerAttached(void);

#if COMPILER(MSVC)
#define WTFBreakpointTrap()  __debugbreak()
#elif ASAN_ENABLED
#define WTFBreakpointTrap()  __builtin_trap()
#elif CPU(X86_64) || CPU(X86)
#define WTFBreakpointTrap()  asm volatile ("int3")
#elif CPU(ARM_THUMB2)
#define WTFBreakpointTrap()  asm volatile ("bkpt #0")
#elif CPU(ARM64)
#define WTFBreakpointTrap()  asm volatile ("brk #0")
#else
#define WTFBreakpointTrap() WTFCrash() // Not implemented.
#endif

#if COMPILER(MSVC)
#define WTFBreakpointTrapUnderConstexprContext() __debugbreak()
#else
#define WTFBreakpointTrapUnderConstexprContext() __builtin_trap()
#endif

#ifndef CRASH

#if defined(NDEBUG) && OS(DARWIN)
// Crash with a SIGTRAP i.e EXC_BREAKPOINT.
// We are not using __builtin_trap because it is only guaranteed to abort, but not necessarily
// trigger a SIGTRAP. Instead, we use inline asm to ensure that we trigger the SIGTRAP.
#define CRASH() do { \
    WTFBreakpointTrap(); \
    __builtin_unreachable(); \
} while (0)
#define CRASH_UNDER_CONSTEXPR_CONTEXT() do { \
    WTFBreakpointTrapUnderConstexprContext(); \
    __builtin_unreachable(); \
} while (0)
#elif !ENABLE(DEVELOPER_MODE) && !OS(DARWIN)
#ifdef __cplusplus
#define CRASH() std::abort()
#define CRASH_UNDER_CONSTEXPR_CONTEXT() std::abort()
#else
#define CRASH() abort()
#define CRASH_UNDER_CONSTEXPR_CONTEXT() abort()
#endif // __cplusplus
#else
#define CRASH() WTFCrash()
#define CRASH_UNDER_CONSTEXPR_CONTEXT() WTFCrash()
#endif

#endif // !defined(CRASH)

WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH void WTFCrash(void);

#ifndef CRASH_WITH_SECURITY_IMPLICATION
#define CRASH_WITH_SECURITY_IMPLICATION() WTFCrashWithSecurityImplication()
#endif

WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH void WTFCrashWithSecurityImplication(void);

#ifdef __cplusplus
}
#endif

/* BACKTRACE

  Print a backtrace to the same location as ASSERT messages.
*/

#if BACKTRACE_DISABLED

#define BACKTRACE() ((void)0)

#else

#define BACKTRACE() do { \
    WTFReportBacktrace(); \
} while(false)

#endif

/* ASSERT, ASSERT_NOT_REACHED, ASSERT_UNUSED

  These macros are compiled out of release builds.
  Expressions inside them are evaluated in debug builds only.
*/

#if OS(WINDOWS)
/* FIXME: Change to use something other than ASSERT to avoid this conflict with the underlying platform */
#undef ASSERT
#endif

#if ASSERT_DISABLED

#define ASSERT(assertion, ...) ((void)0)
#define ASSERT_UNDER_CONSTEXPR_CONTEXT(assertion) ((void)0)
#define ASSERT_AT(assertion, file, line, function) ((void)0)
#define ASSERT_NOT_REACHED(...) ((void)0)
#define ASSERT_NOT_IMPLEMENTED_YET() ((void)0)
#define ASSERT_IMPLIES(condition, assertion) ((void)0)
#define NO_RETURN_DUE_TO_ASSERT

#define ASSERT_UNUSED(variable, assertion, ...) ((void)variable)

#if ENABLE(SECURITY_ASSERTIONS)
#define ASSERT_WITH_SECURITY_IMPLICATION(assertion) \
    (!(assertion) ? \
        (WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion), \
         CRASH_WITH_SECURITY_IMPLICATION()) : \
        (void)0)

#define ASSERT_WITH_SECURITY_IMPLICATION_DISABLED 0
#else
#define ASSERT_WITH_SECURITY_IMPLICATION(assertion) ((void)0)
#define ASSERT_WITH_SECURITY_IMPLICATION_DISABLED 1
#endif

#else

#define ASSERT(assertion, ...) do { \
    if (!(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion); \
        CRASH_WITH_INFO(__VA_ARGS__); \
    } \
} while (0)

#define ASSERT_UNDER_CONSTEXPR_CONTEXT(assertion) do { \
    if (!(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion); \
        CRASH_UNDER_CONSTEXPR_CONTEXT(); \
    } \
} while (0)

#define ASSERT_AT(assertion, file, line, function) do { \
    if (!(assertion)) { \
        WTFReportAssertionFailure(file, line, function, #assertion); \
        CRASH(); \
    } \
} while (0)

#define ASSERT_NOT_REACHED(...) do { \
    WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, 0); \
    CRASH_WITH_INFO(__VA_ARGS__); \
} while (0)

#define ASSERT_NOT_IMPLEMENTED_YET() do { \
    WTFReportNotImplementedYet(__FILE__, __LINE__, WTF_PRETTY_FUNCTION); \
    CRASH(); \
} while (0)

#define ASSERT_IMPLIES(condition, assertion) do { \
    if ((condition) && !(assertion)) { \
        WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #condition " => " #assertion); \
        CRASH(); \
    } \
} while (0)

#define ASSERT_UNUSED(variable, assertion, ...) ASSERT(assertion, __VA_ARGS__)

#define NO_RETURN_DUE_TO_ASSERT NO_RETURN_DUE_TO_CRASH

/* ASSERT_WITH_SECURITY_IMPLICATION
 
   Failure of this assertion indicates a possible security vulnerability.
   Class of vulnerabilities that it tests include bad casts, out of bounds
   accesses, use-after-frees, etc. Please file a bug using the security
   template - https://bugs.webkit.org/enter_bug.cgi?product=Security.
 
*/
#define ASSERT_WITH_SECURITY_IMPLICATION(assertion) \
    (!(assertion) ? \
        (WTFReportAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion), \
         CRASH_WITH_SECURITY_IMPLICATION()) : \
        (void)0)
#define ASSERT_WITH_SECURITY_IMPLICATION_DISABLED 0
#endif

/* ASSERT_WITH_MESSAGE */

#if ASSERT_MSG_DISABLED
#define ASSERT_WITH_MESSAGE(assertion, ...) ((void)0)
#else
#define ASSERT_WITH_MESSAGE(assertion, ...) do { \
    if (!(assertion)) { \
        WTFReportAssertionFailureWithMessage(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion, __VA_ARGS__); \
        CRASH(); \
    } \
} while (0)
#endif

/* ASSERT_WITH_MESSAGE_UNUSED */

#if ASSERT_MSG_DISABLED
#define ASSERT_WITH_MESSAGE_UNUSED(variable, assertion, ...) ((void)variable)
#else
#define ASSERT_WITH_MESSAGE_UNUSED(variable, assertion, ...) do { \
    if (!(assertion)) { \
        WTFReportAssertionFailureWithMessage(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #assertion, __VA_ARGS__); \
        CRASH(); \
    } \
} while (0)
#endif
                        
                        
/* ASSERT_ARG */

#if ASSERT_ARG_DISABLED

#define ASSERT_ARG(argName, assertion) ((void)0)

#else

#define ASSERT_ARG(argName, assertion) do { \
    if (!(assertion)) { \
        WTFReportArgumentAssertionFailure(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, #argName, #assertion); \
        CRASH(); \
    } \
} while (0)

#endif

/* COMPILE_ASSERT */
#ifndef COMPILE_ASSERT
#if COMPILER_SUPPORTS(C_STATIC_ASSERT)
/* Unlike static_assert below, this also works in plain C code. */
#define COMPILE_ASSERT(exp, name) _Static_assert((exp), #name)
#else
#define COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif
#endif

/* FATAL */

#if FATAL_DISABLED
#define FATAL(...) ((void)0)
#else
#define FATAL(...) do { \
    WTFReportFatalError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, __VA_ARGS__); \
    CRASH(); \
} while (0)
#endif

/* LOG_ERROR */

#if ERROR_DISABLED
#define LOG_ERROR(...) ((void)0)
#else
#define LOG_ERROR(...) WTFReportError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, __VA_ARGS__)
#endif

/* LOG */

#if LOG_DISABLED
#define LOG(channel, ...) ((void)0)
#else
#define LOG(channel, ...) WTFLog(&LOG_CHANNEL(channel), __VA_ARGS__)
#endif

/* LOG_VERBOSE */

#if LOG_DISABLED
#define LOG_VERBOSE(channel, ...) ((void)0)
#else
#define LOG_VERBOSE(channel, ...) WTFLogVerbose(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, &LOG_CHANNEL(channel), __VA_ARGS__)
#endif

/* LOG_WITH_LEVEL */

#if LOG_DISABLED
#define LOG_WITH_LEVEL(channel, level, ...) ((void)0)
#else
#define LOG_WITH_LEVEL(channel, level, ...) WTFLogWithLevel(&LOG_CHANNEL(channel), level, __VA_ARGS__)
#endif

/* RELEASE_LOG */

#if RELEASE_LOG_DISABLED
#define RELEASE_LOG(channel, ...) ((void)0)
#define RELEASE_LOG_ERROR(channel, ...) LOG_ERROR(__VA_ARGS__)
#define RELEASE_LOG_FAULT(channel, ...) LOG_ERROR(__VA_ARGS__)

#define RELEASE_LOG_IF(isAllowed, channel, ...) ((void)0)
#define RELEASE_LOG_ERROR_IF(isAllowed, channel, ...) do { if (isAllowed) RELEASE_LOG_ERROR(channel, __VA_ARGS__); } while (0)

#define RELEASE_LOG_WITH_LEVEL(channel, level, ...) ((void)0)
#define RELEASE_LOG_WITH_LEVEL_IF(isAllowed, channel, level, ...) do { if (isAllowed) RELEASE_LOG_WITH_LEVEL(channel, level, __VA_ARGS__); } while (0)

#define RELEASE_LOG_STACKTRACE(channel) ((void)0)
#else
#define RELEASE_LOG(channel, ...) os_log(LOG_CHANNEL(channel).osLogChannel, __VA_ARGS__)
#define RELEASE_LOG_ERROR(channel, ...) os_log_error(LOG_CHANNEL(channel).osLogChannel, __VA_ARGS__)
#define RELEASE_LOG_FAULT(channel, ...) os_log_fault(LOG_CHANNEL(channel).osLogChannel, __VA_ARGS__)
#define RELEASE_LOG_INFO(channel, ...) os_log_info(LOG_CHANNEL(channel).osLogChannel, __VA_ARGS__)

#define RELEASE_LOG_IF(isAllowed, channel, ...) do { if (isAllowed) RELEASE_LOG(      channel, __VA_ARGS__); } while (0)
#define RELEASE_LOG_ERROR_IF(isAllowed, channel, ...) do { if (isAllowed) RELEASE_LOG_ERROR(channel, __VA_ARGS__); } while (0)
#define RELEASE_LOG_INFO_IF(isAllowed, channel, ...) do { if (isAllowed) RELEASE_LOG_INFO(channel, __VA_ARGS__); } while (0)

#define RELEASE_LOG_WITH_LEVEL(channel, logLevel, ...) do { \
    if (LOG_CHANNEL(channel).level >= (logLevel)) \
        os_log(LOG_CHANNEL(channel).osLogChannel, __VA_ARGS__); \
} while (0)

#define RELEASE_LOG_WITH_LEVEL_IF(isAllowed, channel, logLevel, ...) do { \
    if ((isAllowed) && LOG_CHANNEL(channel).level >= (logLevel)) \
        os_log(LOG_CHANNEL(channel).osLogChannel, __VA_ARGS__); \
} while (0)

#define RELEASE_LOG_STACKTRACE(channel) WTFReleaseLogStackTrace(&LOG_CHANNEL(channel))
#endif


/* RELEASE_ASSERT */

#if ASSERT_DISABLED
#define RELEASE_ASSERT(assertion, ...) do { \
    if (UNLIKELY(!(assertion))) \
        CRASH_WITH_INFO(__VA_ARGS__); \
} while (0)
#define RELEASE_ASSERT_WITH_MESSAGE(assertion, ...) RELEASE_ASSERT(assertion)
#define RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(assertion) RELEASE_ASSERT(assertion)
#define RELEASE_ASSERT_NOT_REACHED(...) CRASH_WITH_INFO(__VA_ARGS__)
#define RELEASE_ASSERT_UNDER_CONSTEXPR_CONTEXT(assertion) do { \
    if (UNLIKELY(!(assertion))) { \
        CRASH_UNDER_CONSTEXPR_CONTEXT(); \
    } \
} while (0)
#else
#define RELEASE_ASSERT(assertion, ...) ASSERT(assertion, __VA_ARGS__)
#define RELEASE_ASSERT_WITH_MESSAGE(assertion, ...) ASSERT_WITH_MESSAGE(assertion, __VA_ARGS__)
#define RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(assertion) ASSERT_WITH_SECURITY_IMPLICATION(assertion)
#define RELEASE_ASSERT_NOT_REACHED() ASSERT_NOT_REACHED()
#define RELEASE_ASSERT_UNDER_CONSTEXPR_CONTEXT(assertion) ASSERT_UNDER_CONSTEXPR_CONTEXT(assertion)
#endif

#ifdef __cplusplus

// The combination of line, file, function, and counter should be a unique number per call to this crash. This tricks the compiler into not coalescing calls to WTFCrashWithInfo.
// The easiest way to fill these values per translation unit is to pass __LINE__, __FILE__, WTF_PRETTY_FUNCTION, and __COUNTER__.
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter, uint64_t reason, uint64_t misc1, uint64_t misc2, uint64_t misc3, uint64_t misc4, uint64_t misc5, uint64_t misc6);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter, uint64_t reason, uint64_t misc1, uint64_t misc2, uint64_t misc3, uint64_t misc4, uint64_t misc5);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter, uint64_t reason, uint64_t misc1, uint64_t misc2, uint64_t misc3, uint64_t misc4);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter, uint64_t reason, uint64_t misc1, uint64_t misc2, uint64_t misc3);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter, uint64_t reason, uint64_t misc1, uint64_t misc2);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter, uint64_t reason, uint64_t misc1);
WTF_EXPORT_PRIVATE NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter, uint64_t reason);
NO_RETURN_DUE_TO_CRASH NOT_TAIL_CALLED void WTFCrashWithInfo(int line, const char* file, const char* function, int counter);

inline void WTFCrashWithInfo(int, const char*, const char*, int)
#if COMPILER(CLANG)
    __attribute__((optnone))
#endif
{
    CRASH();
}

namespace WTF {
inline void isIntegralType() { }

template<typename T, typename... Types>
void isIntegralType(T, Types... types)
{
    static_assert(std::is_integral<T>::value || std::is_enum<T>::value, "All types need to be integral bitwise_cast to integral type for logging");
    isIntegralType(types...);
}
}

inline void compilerFenceForCrash()
{
#if OS(WINDOWS) && !COMPILER(GCC_COMPATIBLE)
    _ReadWriteBarrier();
#else
    asm volatile("" ::: "memory");
#endif
}

#ifndef CRASH_WITH_INFO
// This is useful if you are going to stuff data into registers before crashing. Like the crashWithInfo functions below...
// GCC doesn't like the ##__VA_ARGS__ here since this macro is called from another macro so we just CRASH instead there.
#if COMPILER(CLANG) || COMPILER(MSVC)
#define CRASH_WITH_INFO(...) do { \
        WTF::isIntegralType(__VA_ARGS__); \
        compilerFenceForCrash(); \
        WTFCrashWithInfo(__LINE__, __FILE__, WTF_PRETTY_FUNCTION, __COUNTER__, ##__VA_ARGS__); \
    } while (false)
#else
#define CRASH_WITH_INFO(...) CRASH()
#endif
#endif // CRASH_WITH_INFO

#ifndef CRASH_WITH_SECURITY_IMPLICATION_AND_INFO
#define CRASH_WITH_SECURITY_IMPLICATION_AND_INFO CRASH_WITH_INFO
#endif // CRASH_WITH_SECURITY_IMPLICATION_AND_INFO

#else /* not __cplusplus */

#ifndef CRASH_WITH_INFO
#define CRASH_WITH_INFO() CRASH()
#endif

#endif /* __cplusplus */

/* UNREACHABLE_FOR_PLATFORM */

#if COMPILER(CLANG)
// This would be a macro except that its use of #pragma works best around
// a function. Hence it uses macro naming convention.
IGNORE_WARNINGS_BEGIN("missing-noreturn")
static inline void UNREACHABLE_FOR_PLATFORM()
{
    // This *MUST* be a release assert. We use it in places where it's better to crash than to keep
    // going.
    RELEASE_ASSERT_NOT_REACHED();
}
IGNORE_WARNINGS_END
#else
#define UNREACHABLE_FOR_PLATFORM() RELEASE_ASSERT_NOT_REACHED()
#endif
