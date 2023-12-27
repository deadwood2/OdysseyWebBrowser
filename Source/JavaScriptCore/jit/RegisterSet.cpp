/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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
#include "RegisterSet.h"

#if ENABLE(JIT)

#include "GPRInfo.h"
#include "MacroAssembler.h"
#include "JSCInlines.h"
#include <wtf/CommaPrinter.h>

namespace JSC {

RegisterSet RegisterSet::stackRegisters()
{
    return RegisterSet(
        MacroAssembler::stackPointerRegister,
        MacroAssembler::framePointerRegister);
}

RegisterSet RegisterSet::reservedHardwareRegisters()
{
#if CPU(ARM64)
    return RegisterSet(ARM64Registers::lr);
#else
    return RegisterSet();
#endif
}

RegisterSet RegisterSet::runtimeRegisters()
{
#if USE(JSVALUE64)
    return RegisterSet(GPRInfo::tagTypeNumberRegister, GPRInfo::tagMaskRegister);
#else
    return RegisterSet();
#endif
}

RegisterSet RegisterSet::specialRegisters()
{
    return RegisterSet(
        stackRegisters(), reservedHardwareRegisters(), runtimeRegisters());
}

RegisterSet RegisterSet::calleeSaveRegisters()
{
    RegisterSet result;
#if CPU(X86)
    result.set(X86Registers::ebx);
    result.set(X86Registers::ebp);
    result.set(X86Registers::edi);
    result.set(X86Registers::esi);
#elif CPU(X86_64)
    result.set(X86Registers::ebx);
    result.set(X86Registers::ebp);
    result.set(X86Registers::r12);
    result.set(X86Registers::r13);
    result.set(X86Registers::r14);
    result.set(X86Registers::r15);
#elif CPU(ARM_THUMB2)
    result.set(ARMRegisters::r4);
    result.set(ARMRegisters::r5);
    result.set(ARMRegisters::r6);
    result.set(ARMRegisters::r8);
#if !PLATFORM(IOS)
    result.set(ARMRegisters::r9);
#endif
    result.set(ARMRegisters::r10);
    result.set(ARMRegisters::r11);
#elif CPU(ARM_TRADITIONAL)
    result.set(ARMRegisters::r4);
    result.set(ARMRegisters::r5);
    result.set(ARMRegisters::r6);
    result.set(ARMRegisters::r7);
    result.set(ARMRegisters::r8);
    result.set(ARMRegisters::r9);
    result.set(ARMRegisters::r10);
    result.set(ARMRegisters::r11);
#elif CPU(ARM64)
    // We don't include LR in the set of callee-save registers even though it technically belongs
    // there. This is because we use this set to describe the set of registers that need to be saved
    // beyond what you would save by the platform-agnostic "preserve return address" and "restore
    // return address" operations in CCallHelpers.
    for (
        ARM64Registers::RegisterID reg = ARM64Registers::x19;
        reg <= ARM64Registers::x28;
        reg = static_cast<ARM64Registers::RegisterID>(reg + 1))
        result.set(reg);
    result.set(ARM64Registers::fp);
    for (
        ARM64Registers::FPRegisterID reg = ARM64Registers::q8;
        reg <= ARM64Registers::q15;
        reg = static_cast<ARM64Registers::FPRegisterID>(reg + 1))
        result.set(reg);
#else
    UNREACHABLE_FOR_PLATFORM();
#endif
    return result;
}

RegisterSet RegisterSet::allGPRs()
{
    RegisterSet result;
    for (MacroAssembler::RegisterID reg = MacroAssembler::firstRegister(); reg <= MacroAssembler::lastRegister(); reg = static_cast<MacroAssembler::RegisterID>(reg + 1))
        result.set(reg);
    return result;
}

RegisterSet RegisterSet::allFPRs()
{
    RegisterSet result;
    for (MacroAssembler::FPRegisterID reg = MacroAssembler::firstFPRegister(); reg <= MacroAssembler::lastFPRegister(); reg = static_cast<MacroAssembler::FPRegisterID>(reg + 1))
        result.set(reg);
    return result;
}

RegisterSet RegisterSet::allRegisters()
{
    RegisterSet result;
    result.merge(allGPRs());
    result.merge(allFPRs());
    return result;
}

size_t RegisterSet::numberOfSetGPRs() const
{
    RegisterSet temp = *this;
    temp.filter(allGPRs());
    return temp.numberOfSetRegisters();
}

size_t RegisterSet::numberOfSetFPRs() const
{
    RegisterSet temp = *this;
    temp.filter(allFPRs());
    return temp.numberOfSetRegisters();
}

void RegisterSet::dump(PrintStream& out) const
{
    CommaPrinter comma;
    out.print("[");
    for (Reg reg = Reg::first(); reg <= Reg::last(); reg = reg.next()) {
        if (get(reg))
            out.print(comma, reg);
    }
    out.print("]");
}

} // namespace JSC

#endif // ENABLE(JIT)

