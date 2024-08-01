/*
 * Copyright (C) 2015-2016 Apple Inc. All rights reserved.
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
#include "B3MemoryValue.h"

#if ENABLE(B3_JIT)

namespace JSC { namespace B3 {

MemoryValue::~MemoryValue()
{
}

size_t MemoryValue::accessByteSize() const
{
    switch (opcode()) {
    case Load8Z:
    case Load8S:
    case Store8:
        return 1;
    case Load16Z:
    case Load16S:
    case Store16:
        return 2;
    case Load:
        return sizeofType(type());
    case Store:
        return sizeofType(child(0)->type());
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

void MemoryValue::dumpMeta(CommaPrinter& comma, PrintStream& out) const
{
    if (m_offset)
        out.print(comma, "offset = ", m_offset);
}

Value* MemoryValue::cloneImpl() const
{
    return new MemoryValue(*this);
}

} } // namespace JSC::B3

#endif // ENABLE(B3_JIT)
