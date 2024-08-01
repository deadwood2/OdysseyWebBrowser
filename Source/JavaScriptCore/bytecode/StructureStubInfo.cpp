/*
 * Copyright (C) 2008, 2014, 2015 Apple Inc. All rights reserved.
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
#include "StructureStubInfo.h"

#include "JSObject.h"
#include "PolymorphicAccess.h"
#include "Repatch.h"

namespace JSC {

#if ENABLE(JIT)
StructureStubInfo::StructureStubInfo(AccessType accessType)
    : callSiteIndex(UINT_MAX)
    , accessType(accessType)
    , cacheType(CacheType::Unset)
    , countdown(1) // For a totally clear stub, we'll patch it after the first execution.
    , repatchCount(0)
    , numberOfCoolDowns(0)
    , resetByGC(false)
    , tookSlowPath(false)
    , everConsidered(false)
{
}

StructureStubInfo::~StructureStubInfo()
{
}

void StructureStubInfo::initGetByIdSelf(CodeBlock* codeBlock, Structure* baseObjectStructure, PropertyOffset offset)
{
    cacheType = CacheType::GetByIdSelf;
    
    u.byIdSelf.baseObjectStructure.set(
        *codeBlock->vm(), codeBlock, baseObjectStructure);
    u.byIdSelf.offset = offset;
}

void StructureStubInfo::initPutByIdReplace(CodeBlock* codeBlock, Structure* baseObjectStructure, PropertyOffset offset)
{
    cacheType = CacheType::PutByIdReplace;
    
    u.byIdSelf.baseObjectStructure.set(
        *codeBlock->vm(), codeBlock, baseObjectStructure);
    u.byIdSelf.offset = offset;
}

void StructureStubInfo::initStub(CodeBlock*, std::unique_ptr<PolymorphicAccess> stub)
{
    cacheType = CacheType::Stub;
    u.stub = stub.release();
}

void StructureStubInfo::deref()
{
    switch (cacheType) {
    case CacheType::Stub:
        delete u.stub;
        return;
    case CacheType::Unset:
    case CacheType::GetByIdSelf:
    case CacheType::PutByIdReplace:
        return;
    }

    RELEASE_ASSERT_NOT_REACHED();
}

void StructureStubInfo::aboutToDie()
{
    switch (cacheType) {
    case CacheType::Stub:
        u.stub->aboutToDie();
        return;
    case CacheType::Unset:
    case CacheType::GetByIdSelf:
    case CacheType::PutByIdReplace:
        return;
    }

    RELEASE_ASSERT_NOT_REACHED();
}

MacroAssemblerCodePtr StructureStubInfo::addAccessCase(
    CodeBlock* codeBlock, const Identifier& ident, std::unique_ptr<AccessCase> accessCase)
{
    VM& vm = *codeBlock->vm();
    
    if (!accessCase)
        return MacroAssemblerCodePtr();
    
    if (cacheType == CacheType::Stub)
        return u.stub->regenerateWithCase(vm, codeBlock, *this, ident, WTFMove(accessCase));

    std::unique_ptr<PolymorphicAccess> access = std::make_unique<PolymorphicAccess>();
    
    Vector<std::unique_ptr<AccessCase>> accessCases;
    
    std::unique_ptr<AccessCase> previousCase =
        AccessCase::fromStructureStubInfo(vm, codeBlock, *this);
    if (previousCase)
        accessCases.append(WTFMove(previousCase));

    accessCases.append(WTFMove(accessCase));

    MacroAssemblerCodePtr result =
        access->regenerateWithCases(vm, codeBlock, *this, ident, WTFMove(accessCases));

    if (!result)
        return MacroAssemblerCodePtr();

    initStub(codeBlock, WTFMove(access));
    return result;
}

void StructureStubInfo::reset(CodeBlock* codeBlock)
{
    if (cacheType == CacheType::Unset)
        return;
    
    if (Options::verboseOSR()) {
        // This can be called from GC destructor calls, so we don't try to do a full dump
        // of the CodeBlock.
        dataLog("Clearing structure cache (kind ", static_cast<int>(accessType), ") in ", RawPointer(codeBlock), ".\n");
    }

    switch (accessType) {
    case AccessType::Get:
        resetGetByID(codeBlock, *this);
        break;
    case AccessType::Put:
        resetPutByID(codeBlock, *this);
        break;
    case AccessType::In:
        resetIn(codeBlock, *this);
        break;
    }
    
    deref();
    cacheType = CacheType::Unset;
}

void StructureStubInfo::visitWeakReferences(CodeBlock* codeBlock)
{
    VM& vm = *codeBlock->vm();

    switch (cacheType) {
    case CacheType::GetByIdSelf:
    case CacheType::PutByIdReplace:
        if (Heap::isMarked(u.byIdSelf.baseObjectStructure.get()))
            return;
        break;
    case CacheType::Stub:
        if (u.stub->visitWeak(vm))
            return;
        break;
    default:
        return;
    }

    reset(codeBlock);
    resetByGC = true;
}

bool StructureStubInfo::containsPC(void* pc) const
{
    if (cacheType != CacheType::Stub)
        return false;
    return u.stub->containsPC(pc);
}
#endif

} // namespace JSC
