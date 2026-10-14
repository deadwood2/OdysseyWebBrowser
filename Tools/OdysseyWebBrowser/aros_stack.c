/*
 * Copyright (C) 2026 The AROS OWB port authors. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS BE
 * LIABLE FOR ANY DAMAGES ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE.
 */

/*
 * Main task stack for OWB on AROS.
 *
 * AROS startup code does not read __stack: a program runs on the stack of
 * the CLI that started it (Stack command, 262144 bytes by default on AROS
 * One; Run inherits it) or the one in its icon. JavaScriptCore takes its
 * stack limit from the task bounds, so on 256 KB a page cannot recurse past
 * ~1700 JS frames or call a function with more than ~8K arguments
 * (String.fromCharCode.apply, used by Discord's gateway inflate).
 *
 * The switch happens in a PROGRAM_ENTRIES function at priority -30. That is
 * after fromwb (-50), which owns the Workbench message, and before initexit
 * (-20: libraries, INIT, constructors), crt program startup (0: the jmp_buf
 * exit() and abort() return to, atexit handlers) and main (127). All of those
 * run on the new stack: exit() and abort() longjmp within it, atexit handlers
 * and destructors run on it, and control comes back here only by returning
 * from __startup_entries_next(). NewStackSwap then restores the task bounds
 * and SP, and the stack is freed. Only fromwb and the startup entry itself
 * run on the original stack.
 *
 * Ownership: the stack is allocated here, used only by the main task and
 * freed here after NewStackSwap returns. Threads created by WTF have their
 * own stacks and are not affected.
 *
 * Size: 8 MB, the main thread stack upstream WebKit is normally run with on
 * Linux; JavaScriptCore caps its own use at maxPerThreadStackUsage (5 MB) and
 * the rest is headroom for native frames (layout, cairo, MUI). A task that
 * already has at least this much keeps its stack.
 *
 * If the stack cannot be allocated OWB does not start: it reports the error
 * and returns RETURN_FAIL. A Workbench start (WBenchMsg, set by fromwb) gets
 * a requester, since its Output() may be NIL: or nothing at all; a CLI start
 * writes to Output() and gets the requester only when it has no Output().
 * OWB_SIMULATE_STACK_ALLOC_FAIL (local or global variable) forces that path
 * for testing.
 *
 * This runs before the C library is set up: only exec (through the SysBase
 * argument; the global is not set yet), dos (opened by the startup entry)
 * and libraries opened here through local bases may be used. The automatic
 * library bases, IntuitionBase among them, are opened later by initexit.
 */

#if defined(__AROS__)

/* Intuition calls in this file use the base opened here, not the automatic one. */
#define __INTUITION_LIBBASE owbIntuitionBase

#include <aros/startup.h>
#include <aros/symbolsets.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#define OWB_MAIN_STACK_SIZE (8 * 1024 * 1024)

static const char owbStackError[] =
    "OWB needs 8 MB of memory for its stack and could not get it.";

static void owb_stack_report_failure(struct ExecBase* SysBase)
{
    BPTR out = WBenchMsg ? BNULL : Output();
    if (out) {
        FPuts(out, (CONST_STRPTR)owbStackError);
        FPuts(out, (CONST_STRPTR)"\n");
        return;
    }

    struct IntuitionBase* owbIntuitionBase =
        (struct IntuitionBase*)OpenLibrary((CONST_STRPTR)"intuition.library", 36);
    if (owbIntuitionBase) {
        struct EasyStruct es = {
            sizeof(struct EasyStruct), 0,
            (CONST_STRPTR)"OWB", (CONST_STRPTR)owbStackError, (CONST_STRPTR)"OK"
        };
        EasyRequestArgs(NULL, &es, NULL, NULL);
        CloseLibrary((struct Library*)owbIntuitionBase);
    }
}

static void owb_stack_run(struct ExecBase* SysBase)
{
    __startup_entries_next();
}

static void owb_stack_entry(struct ExecBase* SysBase)
{
    struct Task* self = FindTask(NULL);
    IPTR have = (IPTR)self->tc_SPUpper - (IPTR)self->tc_SPLower;
    if (have >= OWB_MAIN_STACK_SIZE) {
        __startup_entries_next();
        return;
    }

    char flag[2];
    APTR stack = NULL;
    if (GetVar((CONST_STRPTR)"OWB_SIMULATE_STACK_ALLOC_FAIL", (STRPTR)flag, sizeof(flag), 0) < 0)
        stack = AllocVec(OWB_MAIN_STACK_SIZE, MEMF_ANY);
    if (!stack) {
        owb_stack_report_failure(SysBase);
        __startup_error = RETURN_FAIL;
        return;
    }

    struct StackSwapStruct sss;
    sss.stk_Lower = stack;
    sss.stk_Upper = (APTR)((IPTR)stack + OWB_MAIN_STACK_SIZE);
    sss.stk_Pointer = sss.stk_Upper;
    struct StackSwapArgs args = { { (IPTR)SysBase } };
    NewStackSwap(&sss, (APTR)owb_stack_run, &args);
    FreeVec(stack);
}

ADD2SET(owb_stack_entry, PROGRAM_ENTRIES, -30);

#endif /* __AROS__ */
