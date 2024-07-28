/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "JITDivGenerator.h"

#if ENABLE(JIT)

#include "JSCJSValueInlines.h"

namespace JSC {

void JITDivGenerator::loadOperand(CCallHelpers& jit, SnippetOperand& opr, JSValueRegs oprRegs, FPRReg destFPR)
{
    if (opr.isConstInt32()) {
        jit.move(CCallHelpers::Imm32(opr.asConstInt32()), m_scratchGPR);
        jit.convertInt32ToDouble(m_scratchGPR, destFPR);
#if USE(JSVALUE64)
    } else if (opr.isConstDouble()) {
        jit.move(CCallHelpers::Imm64(opr.asRawBits()), m_scratchGPR);
        jit.move64ToDouble(m_scratchGPR, destFPR);
#endif
    } else {
        if (!opr.definitelyIsNumber())
            m_slowPathJumpList.append(jit.branchIfNotNumber(oprRegs, m_scratchGPR));
        CCallHelpers::Jump notInt32 = jit.branchIfNotInt32(oprRegs);
        jit.convertInt32ToDouble(oprRegs.payloadGPR(), destFPR);
        CCallHelpers::Jump oprIsLoaded = jit.jump();
        notInt32.link(&jit);
        jit.unboxDoubleNonDestructive(oprRegs, destFPR, m_scratchGPR, m_scratchFPR);
        oprIsLoaded.link(&jit);
    }
}

void JITDivGenerator::generateFastPath(CCallHelpers& jit)
{
    ASSERT(m_scratchGPR != InvalidGPRReg);
    ASSERT(m_scratchGPR != m_left.payloadGPR());
    ASSERT(m_scratchGPR != m_right.payloadGPR());
#if USE(JSVALUE32_64)
    ASSERT(m_scratchGPR != m_left.tagGPR());
    ASSERT(m_scratchGPR != m_right.tagGPR());
    ASSERT(m_scratchFPR != InvalidFPRReg);
#endif

    ASSERT(!m_didEmitFastPath);
    if (!jit.supportsFloatingPoint())
        return;
    if (!m_leftOperand.mightBeNumber() || !m_rightOperand.mightBeNumber())
        return;

    ASSERT(!m_leftOperand.isConstInt32() || !m_rightOperand.isConstInt32());
    m_didEmitFastPath = true;
    loadOperand(jit, m_leftOperand, m_left, m_leftFPR);
    loadOperand(jit, m_rightOperand, m_right, m_rightFPR);

    jit.divDouble(m_rightFPR, m_leftFPR);

    // Is the result actually an integer? The DFG JIT would really like to know. If it's
    // not an integer, we increment a count. If this together with the slow case counter
    // are below threshold then the DFG JIT will compile this division with a speculation
    // that the remainder is zero.

    // As well, there are cases where a double result here would cause an important field
    // in the heap to sometimes have doubles in it, resulting in double predictions getting
    // propagated to a use site where it might cause damage (such as the index to an array
    // access). So if we are DFG compiling anything in the program, we want this code to
    // ensure that it produces integers whenever possible.
    
    CCallHelpers::JumpList notInt32;
    jit.branchConvertDoubleToInt32(m_leftFPR, m_scratchGPR, notInt32, m_scratchFPR);

    // If we've got an integer, we might as well make that the result of the division.
    jit.boxInt32(m_scratchGPR, m_result);
    m_endJumpList.append(jit.jump());

    notInt32.link(&jit);
#if USE(JSVALUE64)
    jit.moveDoubleTo64(m_leftFPR, m_scratchGPR);
    CCallHelpers::Jump notDoubleZero = jit.branchTest64(CCallHelpers::NonZero, m_scratchGPR);

    jit.move(GPRInfo::tagTypeNumberRegister, m_result.payloadGPR());
    m_endJumpList.append(jit.jump());

    notDoubleZero.link(&jit);
#endif
    if (m_resultProfile)
        jit.add32(CCallHelpers::TrustedImm32(1), CCallHelpers::AbsoluteAddress(m_resultProfile->addressOfSpecialFastPathCount()));
    jit.boxDouble(m_leftFPR, m_result);
}

} // namespace JSC

#endif // ENABLE(JIT)
