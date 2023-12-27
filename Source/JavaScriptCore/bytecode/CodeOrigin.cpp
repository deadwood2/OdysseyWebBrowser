/*
 * Copyright (C) 2012-2015 Apple Inc. All rights reserved.
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
#include "CodeOrigin.h"

#include "CallFrame.h"
#include "CodeBlock.h"
#include "Executable.h"
#include "JSCInlines.h"

namespace JSC {

unsigned CodeOrigin::inlineDepthForCallFrame(InlineCallFrame* inlineCallFrame)
{
    unsigned result = 1;
    for (InlineCallFrame* current = inlineCallFrame; current; current = current->caller.inlineCallFrame)
        result++;
    return result;
}

unsigned CodeOrigin::inlineDepth() const
{
    return inlineDepthForCallFrame(inlineCallFrame);
}

bool CodeOrigin::isApproximatelyEqualTo(const CodeOrigin& other) const
{
    CodeOrigin a = *this;
    CodeOrigin b = other;

    if (!a.isSet())
        return !b.isSet();
    if (!b.isSet())
        return false;
    
    if (a.isHashTableDeletedValue())
        return b.isHashTableDeletedValue();
    if (b.isHashTableDeletedValue())
        return false;
    
    for (;;) {
        ASSERT(a.isSet());
        ASSERT(b.isSet());
        
        if (a.bytecodeIndex != b.bytecodeIndex)
            return false;
        
        if ((!!a.inlineCallFrame) != (!!b.inlineCallFrame))
            return false;
        
        if (!a.inlineCallFrame)
            return true;
        
        if (a.inlineCallFrame->executable.get() != b.inlineCallFrame->executable.get())
            return false;
        
        a = a.inlineCallFrame->caller;
        b = b.inlineCallFrame->caller;
    }
}

unsigned CodeOrigin::approximateHash() const
{
    if (!isSet())
        return 0;
    if (isHashTableDeletedValue())
        return 1;
    
    unsigned result = 2;
    CodeOrigin codeOrigin = *this;
    for (;;) {
        result += codeOrigin.bytecodeIndex;
        
        if (!codeOrigin.inlineCallFrame)
            return result;
        
        result += WTF::PtrHash<JSCell*>::hash(codeOrigin.inlineCallFrame->executable.get());
        
        codeOrigin = codeOrigin.inlineCallFrame->caller;
    }
}

Vector<CodeOrigin> CodeOrigin::inlineStack() const
{
    Vector<CodeOrigin> result(inlineDepth());
    result.last() = *this;
    unsigned index = result.size() - 2;
    for (InlineCallFrame* current = inlineCallFrame; current; current = current->caller.inlineCallFrame)
        result[index--] = current->caller;
    RELEASE_ASSERT(!result[0].inlineCallFrame);
    return result;
}

void CodeOrigin::dump(PrintStream& out) const
{
    if (!isSet()) {
        out.print("<none>");
        return;
    }
    
    Vector<CodeOrigin> stack = inlineStack();
    for (unsigned i = 0; i < stack.size(); ++i) {
        if (i)
            out.print(" --> ");
        
        if (InlineCallFrame* frame = stack[i].inlineCallFrame) {
            out.print(frame->briefFunctionInformation(), ":<", RawPointer(frame->executable.get()), "> ");
            if (frame->isClosureCall)
                out.print("(closure) ");
        }
        
        out.print("bc#", stack[i].bytecodeIndex);
    }
}

void CodeOrigin::dumpInContext(PrintStream& out, DumpContext*) const
{
    dump(out);
}

JSFunction* InlineCallFrame::calleeConstant() const
{
    if (calleeRecovery.isConstant())
        return jsCast<JSFunction*>(calleeRecovery.constant());
    return nullptr;
}

void InlineCallFrame::visitAggregate(SlotVisitor& visitor)
{
    // FIXME: This is an antipattern for two reasons. References introduced by the DFG
    // that aren't in the original CodeBlock being compiled should be weakly referenced.
    // Inline call frames aren't in the original CodeBlock, so they qualify as weak. Also,
    // those weak references should already be tracked in the DFG as weak FrozenValues. So,
    // there is probably no need for this. We already have assertions that this should be
    // unnecessary. Finally, just marking the executable and not anything else in the inline
    // call frame is almost certainly insufficient for what this method thought it was going
    // to accomplish.
    // https://bugs.webkit.org/show_bug.cgi?id=146613
    visitor.append(&executable);
}

JSFunction* InlineCallFrame::calleeForCallFrame(ExecState* exec) const
{
    return jsCast<JSFunction*>(calleeRecovery.recover(exec));
}

CodeBlockHash InlineCallFrame::hash() const
{
    return jsCast<FunctionExecutable*>(executable.get())->codeBlockFor(
        specializationKind())->hash();
}

CString InlineCallFrame::hashAsStringIfPossible() const
{
    return jsCast<FunctionExecutable*>(executable.get())->codeBlockFor(
        specializationKind())->hashAsStringIfPossible();
}

CString InlineCallFrame::inferredName() const
{
    return jsCast<FunctionExecutable*>(executable.get())->inferredName().utf8();
}

CodeBlock* InlineCallFrame::baselineCodeBlock() const
{
    return jsCast<FunctionExecutable*>(executable.get())->baselineCodeBlockFor(specializationKind());
}

void InlineCallFrame::dumpBriefFunctionInformation(PrintStream& out) const
{
    out.print(inferredName(), "#", hashAsStringIfPossible());
}

void InlineCallFrame::dumpInContext(PrintStream& out, DumpContext* context) const
{
    out.print(briefFunctionInformation(), ":<", RawPointer(executable.get()));
    if (executable->isStrictMode())
        out.print(" (StrictMode)");
    out.print(", bc#", caller.bytecodeIndex, ", ", kind);
    if (isClosureCall)
        out.print(", closure call");
    else
        out.print(", known callee: ", inContext(calleeRecovery.constant(), context));
    out.print(", numArgs+this = ", arguments.size());
    out.print(", stackOffset = ", stackOffset);
    out.print(" (", virtualRegisterForLocal(0), " maps to ", virtualRegisterForLocal(0) + stackOffset, ")>");
}

void InlineCallFrame::dump(PrintStream& out) const
{
    dumpInContext(out, 0);
}

} // namespace JSC

namespace WTF {

void printInternal(PrintStream& out, JSC::InlineCallFrame::Kind kind)
{
    switch (kind) {
    case JSC::InlineCallFrame::Call:
        out.print("Call");
        return;
    case JSC::InlineCallFrame::Construct:
        out.print("Construct");
        return;
    case JSC::InlineCallFrame::CallVarargs:
        out.print("CallVarargs");
        return;
    case JSC::InlineCallFrame::ConstructVarargs:
        out.print("ConstructVarargs");
        return;
    case JSC::InlineCallFrame::GetterCall:
        out.print("GetterCall");
        return;
    case JSC::InlineCallFrame::SetterCall:
        out.print("SetterCall");
        return;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace WTF

