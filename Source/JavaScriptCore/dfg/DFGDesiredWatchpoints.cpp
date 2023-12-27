/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
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
#include "DFGDesiredWatchpoints.h"

#if ENABLE(DFG_JIT)

#include "ArrayBufferNeuteringWatchpoint.h"
#include "CodeBlock.h"
#include "JSCInlines.h"

namespace JSC { namespace DFG {

void ArrayBufferViewWatchpointAdaptor::add(
    CodeBlock* codeBlock, JSArrayBufferView* view, Watchpoint* watchpoint)
{
    ArrayBufferNeuteringWatchpoint* neuteringWatchpoint =
        ArrayBufferNeuteringWatchpoint::create(*codeBlock->vm());
    neuteringWatchpoint->set()->add(watchpoint);
    codeBlock->addConstant(neuteringWatchpoint);
    codeBlock->vm()->heap.addReference(neuteringWatchpoint, view->buffer());
}

void InferredValueAdaptor::add(
    CodeBlock* codeBlock, InferredValue* inferredValue, Watchpoint* watchpoint)
{
    codeBlock->addConstant(inferredValue); // For common users, it doesn't really matter if it's weak or not. If references to it go away, we go away, too.
    inferredValue->add(watchpoint);
}

DesiredWatchpoints::DesiredWatchpoints() { }
DesiredWatchpoints::~DesiredWatchpoints() { }

void DesiredWatchpoints::addLazily(WatchpointSet* set)
{
    m_sets.addLazily(set);
}

void DesiredWatchpoints::addLazily(InlineWatchpointSet& set)
{
    m_inlineSets.addLazily(&set);
}

void DesiredWatchpoints::addLazily(InferredValue* inferredValue)
{
    m_inferredValues.addLazily(inferredValue);
}

void DesiredWatchpoints::addLazily(JSArrayBufferView* view)
{
    m_bufferViews.addLazily(view);
}

bool DesiredWatchpoints::consider(Structure* structure)
{
    if (!structure->dfgShouldWatch())
        return false;
    addLazily(structure->transitionWatchpointSet());
    return true;
}

void DesiredWatchpoints::reallyAdd(CodeBlock* codeBlock, CommonData& commonData)
{
    m_sets.reallyAdd(codeBlock, commonData);
    m_inlineSets.reallyAdd(codeBlock, commonData);
    m_inferredValues.reallyAdd(codeBlock, commonData);
    m_bufferViews.reallyAdd(codeBlock, commonData);
}

bool DesiredWatchpoints::areStillValid() const
{
    return m_sets.areStillValid()
        && m_inlineSets.areStillValid()
        && m_inferredValues.areStillValid()
        && m_bufferViews.areStillValid();
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

