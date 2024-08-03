/*
 * Copyright (C) 2014-2016 Apple Inc. All rights reserved.
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
#include "PolymorphicAccess.h"

#if ENABLE(JIT)

#include "BinarySwitch.h"
#include "CCallHelpers.h"
#include "CodeBlock.h"
#include "DirectArguments.h"
#include "GetterSetter.h"
#include "Heap.h"
#include "JITOperations.h"
#include "JSCInlines.h"
#include "LinkBuffer.h"
#include "ScopedArguments.h"
#include "ScratchRegisterAllocator.h"
#include "StructureStubClearingWatchpoint.h"
#include "StructureStubInfo.h"
#include <wtf/CommaPrinter.h>
#include <wtf/ListDump.h>

namespace JSC {

static const bool verbose = false;

void AccessGenerationResult::dump(PrintStream& out) const
{
    out.print(m_kind);
    if (m_code)
        out.print(":", m_code);
}

Watchpoint* AccessGenerationState::addWatchpoint(const ObjectPropertyCondition& condition)
{
    return WatchpointsOnStructureStubInfo::ensureReferenceAndAddWatchpoint(
        watchpoints, jit->codeBlock(), stubInfo, condition);
}

void AccessGenerationState::restoreScratch()
{
    allocator->restoreReusedRegistersByPopping(*jit, preservedReusedRegisterState);
}

void AccessGenerationState::succeed()
{
    restoreScratch();
    success.append(jit->jump());
}

void AccessGenerationState::calculateLiveRegistersForCallAndExceptionHandling(const RegisterSet& extra)
{
    if (!m_calculatedRegistersForCallAndExceptionHandling) {
        m_calculatedRegistersForCallAndExceptionHandling = true;

        m_liveRegistersToPreserveAtExceptionHandlingCallSite = jit->codeBlock()->jitCode()->liveRegistersToPreserveAtExceptionHandlingCallSite(jit->codeBlock(), stubInfo->callSiteIndex);
        m_needsToRestoreRegistersIfException = m_liveRegistersToPreserveAtExceptionHandlingCallSite.numberOfSetRegisters() > 0;
        if (m_needsToRestoreRegistersIfException)
            RELEASE_ASSERT(JITCode::isOptimizingJIT(jit->codeBlock()->jitType()));

        m_liveRegistersForCall = RegisterSet(m_liveRegistersToPreserveAtExceptionHandlingCallSite, allocator->usedRegisters());
        m_liveRegistersForCall.merge(extra);
        m_liveRegistersForCall.exclude(RegisterSet::registersToNotSaveForJSCall());
        m_liveRegistersForCall.merge(extra);
    }
}

void AccessGenerationState::preserveLiveRegistersToStackForCall(const RegisterSet& extra)
{
    calculateLiveRegistersForCallAndExceptionHandling(extra);
    
    unsigned extraStackPadding = 0;
    unsigned numberOfStackBytesUsedForRegisterPreservation = ScratchRegisterAllocator::preserveRegistersToStackForCall(*jit, liveRegistersForCall(), extraStackPadding);
    if (m_numberOfStackBytesUsedForRegisterPreservation != std::numeric_limits<unsigned>::max())
        RELEASE_ASSERT(numberOfStackBytesUsedForRegisterPreservation == m_numberOfStackBytesUsedForRegisterPreservation);
    m_numberOfStackBytesUsedForRegisterPreservation = numberOfStackBytesUsedForRegisterPreservation;
}

void AccessGenerationState::restoreLiveRegistersFromStackForCall(bool isGetter)
{
    RegisterSet dontRestore;
    if (isGetter) {
        // This is the result value. We don't want to overwrite the result with what we stored to the stack.
        // We sometimes have to store it to the stack just in case we throw an exception and need the original value.
        dontRestore.set(valueRegs);
    }
    restoreLiveRegistersFromStackForCall(dontRestore);
}

void AccessGenerationState::restoreLiveRegistersFromStackForCallWithThrownException()
{
    // Even if we're a getter, we don't want to ignore the result value like we normally do
    // because the getter threw, and therefore, didn't return a value that means anything.
    // Instead, we want to restore that register to what it was upon entering the getter
    // inline cache. The subtlety here is if the base and the result are the same register,
    // and the getter threw, we want OSR exit to see the original base value, not the result
    // of the getter call.
    RegisterSet dontRestore = liveRegistersForCall();
    // As an optimization here, we only need to restore what is live for exception handling.
    // We can construct the dontRestore set to accomplish this goal by having it contain only
    // what is live for call but not live for exception handling. By ignoring things that are
    // only live at the call but not the exception handler, we will only restore things live
    // at the exception handler.
    dontRestore.exclude(liveRegistersToPreserveAtExceptionHandlingCallSite());
    restoreLiveRegistersFromStackForCall(dontRestore);
}

void AccessGenerationState::restoreLiveRegistersFromStackForCall(const RegisterSet& dontRestore)
{
    unsigned extraStackPadding = 0;
    ScratchRegisterAllocator::restoreRegistersFromStackForCall(*jit, liveRegistersForCall(), dontRestore, m_numberOfStackBytesUsedForRegisterPreservation, extraStackPadding);
}

CallSiteIndex AccessGenerationState::callSiteIndexForExceptionHandlingOrOriginal()
{
    RELEASE_ASSERT(m_calculatedRegistersForCallAndExceptionHandling);

    if (!m_calculatedCallSiteIndex) {
        m_calculatedCallSiteIndex = true;

        if (m_needsToRestoreRegistersIfException)
            m_callSiteIndex = jit->codeBlock()->newExceptionHandlingCallSiteIndex(stubInfo->callSiteIndex);
        else
            m_callSiteIndex = originalCallSiteIndex();
    }

    return m_callSiteIndex;
}

const HandlerInfo& AccessGenerationState::originalExceptionHandler() const
{
    RELEASE_ASSERT(m_needsToRestoreRegistersIfException);
    HandlerInfo* exceptionHandler = jit->codeBlock()->handlerForIndex(stubInfo->callSiteIndex.bits());
    RELEASE_ASSERT(exceptionHandler);
    return *exceptionHandler;
}

CallSiteIndex AccessGenerationState::originalCallSiteIndex() const { return stubInfo->callSiteIndex; }

void AccessGenerationState::emitExplicitExceptionHandler()
{
    restoreScratch();
    jit->copyCalleeSavesToVMEntryFrameCalleeSavesBuffer();
    if (needsToRestoreRegistersIfException()) {
        // To the JIT that produces the original exception handling
        // call site, they will expect the OSR exit to be arrived
        // at from genericUnwind. Therefore we must model what genericUnwind
        // does here. I.e, set callFrameForCatch and copy callee saves.

        jit->storePtr(GPRInfo::callFrameRegister, jit->vm()->addressOfCallFrameForCatch());
        CCallHelpers::Jump jumpToOSRExitExceptionHandler = jit->jump();

        // We don't need to insert a new exception handler in the table
        // because we're doing a manual exception check here. i.e, we'll
        // never arrive here from genericUnwind().
        HandlerInfo originalHandler = originalExceptionHandler();
        jit->addLinkTask(
            [=] (LinkBuffer& linkBuffer) {
                linkBuffer.link(jumpToOSRExitExceptionHandler, originalHandler.nativeCode);
            });
    } else {
        jit->setupArguments(CCallHelpers::TrustedImmPtr(jit->vm()), GPRInfo::callFrameRegister);
        CCallHelpers::Call lookupExceptionHandlerCall = jit->call();
        jit->addLinkTask(
            [=] (LinkBuffer& linkBuffer) {
                linkBuffer.link(lookupExceptionHandlerCall, lookupExceptionHandler);
            });
        jit->jumpToExceptionHandler();
    }
}

AccessCase::AccessCase()
{
}

std::unique_ptr<AccessCase> AccessCase::tryGet(
    VM& vm, JSCell* owner, AccessType type, PropertyOffset offset, Structure* structure,
    const ObjectPropertyConditionSet& conditionSet, bool viaProxy, WatchpointSet* additionalSet)
{
    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = type;
    result->m_offset = offset;
    result->m_structure.set(vm, owner, structure);
    result->m_conditionSet = conditionSet;

    if (viaProxy || additionalSet) {
        result->m_rareData = std::make_unique<RareData>();
        result->m_rareData->viaProxy = viaProxy;
        result->m_rareData->additionalSet = additionalSet;
    }

    return result;
}

std::unique_ptr<AccessCase> AccessCase::get(
    VM& vm, JSCell* owner, AccessType type, PropertyOffset offset, Structure* structure,
    const ObjectPropertyConditionSet& conditionSet, bool viaProxy, WatchpointSet* additionalSet,
    PropertySlot::GetValueFunc customGetter, JSObject* customSlotBase)
{
    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = type;
    result->m_offset = offset;
    result->m_structure.set(vm, owner, structure);
    result->m_conditionSet = conditionSet;

    if (viaProxy || additionalSet || result->doesCalls() || customGetter || customSlotBase) {
        result->m_rareData = std::make_unique<RareData>();
        result->m_rareData->viaProxy = viaProxy;
        result->m_rareData->additionalSet = additionalSet;
        result->m_rareData->customAccessor.getter = customGetter;
        result->m_rareData->customSlotBase.setMayBeNull(vm, owner, customSlotBase);
    }

    return result;
}

std::unique_ptr<AccessCase> AccessCase::megamorphicLoad(VM& vm, JSCell* owner)
{
    UNUSED_PARAM(vm);
    UNUSED_PARAM(owner);
    
    if (GPRInfo::numberOfRegisters < 9)
        return nullptr;
    
    std::unique_ptr<AccessCase> result(new AccessCase());
    
    result->m_type = MegamorphicLoad;
    
    return result;
}

std::unique_ptr<AccessCase> AccessCase::replace(
    VM& vm, JSCell* owner, Structure* structure, PropertyOffset offset)
{
    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = Replace;
    result->m_offset = offset;
    result->m_structure.set(vm, owner, structure);

    return result;
}

std::unique_ptr<AccessCase> AccessCase::transition(
    VM& vm, JSCell* owner, Structure* oldStructure, Structure* newStructure, PropertyOffset offset,
    const ObjectPropertyConditionSet& conditionSet)
{
    RELEASE_ASSERT(oldStructure == newStructure->previousID());

    // Skip optimizing the case where we need a realloc, if we don't have
    // enough registers to make it happen.
    if (GPRInfo::numberOfRegisters < 6
        && oldStructure->outOfLineCapacity() != newStructure->outOfLineCapacity()
        && oldStructure->outOfLineCapacity()) {
        return nullptr;
    }

    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = Transition;
    result->m_offset = offset;
    result->m_structure.set(vm, owner, newStructure);
    result->m_conditionSet = conditionSet;

    return result;
}

std::unique_ptr<AccessCase> AccessCase::setter(
    VM& vm, JSCell* owner, AccessType type, Structure* structure, PropertyOffset offset,
    const ObjectPropertyConditionSet& conditionSet, PutPropertySlot::PutValueFunc customSetter,
    JSObject* customSlotBase)
{
    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = type;
    result->m_offset = offset;
    result->m_structure.set(vm, owner, structure);
    result->m_conditionSet = conditionSet;
    result->m_rareData = std::make_unique<RareData>();
    result->m_rareData->customAccessor.setter = customSetter;
    result->m_rareData->customSlotBase.setMayBeNull(vm, owner, customSlotBase);

    return result;
}

std::unique_ptr<AccessCase> AccessCase::in(
    VM& vm, JSCell* owner, AccessType type, Structure* structure,
    const ObjectPropertyConditionSet& conditionSet)
{
    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = type;
    result->m_structure.set(vm, owner, structure);
    result->m_conditionSet = conditionSet;

    return result;
}

std::unique_ptr<AccessCase> AccessCase::getLength(VM&, JSCell*, AccessType type)
{
    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = type;

    return result;
}

std::unique_ptr<AccessCase> AccessCase::getIntrinsic(
    VM& vm, JSCell* owner, JSFunction* getter, PropertyOffset offset,
    Structure* structure, const ObjectPropertyConditionSet& conditionSet)
{
    std::unique_ptr<AccessCase> result(new AccessCase());

    result->m_type = IntrinsicGetter;
    result->m_structure.set(vm, owner, structure);
    result->m_conditionSet = conditionSet;
    result->m_offset = offset;

    result->m_rareData = std::make_unique<RareData>();
    result->m_rareData->intrinsicFunction.set(vm, owner, getter);

    return result;
}

AccessCase::~AccessCase()
{
}

std::unique_ptr<AccessCase> AccessCase::fromStructureStubInfo(
    VM& vm, JSCell* owner, StructureStubInfo& stubInfo)
{
    switch (stubInfo.cacheType) {
    case CacheType::GetByIdSelf:
        return get(
            vm, owner, Load, stubInfo.u.byIdSelf.offset,
            stubInfo.u.byIdSelf.baseObjectStructure.get());

    case CacheType::PutByIdReplace:
        return replace(
            vm, owner, stubInfo.u.byIdSelf.baseObjectStructure.get(), stubInfo.u.byIdSelf.offset);

    default:
        return nullptr;
    }
}

std::unique_ptr<AccessCase> AccessCase::clone() const
{
    std::unique_ptr<AccessCase> result(new AccessCase());
    result->m_type = m_type;
    result->m_offset = m_offset;
    result->m_structure = m_structure;
    result->m_conditionSet = m_conditionSet;
    if (RareData* rareData = m_rareData.get()) {
        result->m_rareData = std::make_unique<RareData>();
        result->m_rareData->viaProxy = rareData->viaProxy;
        result->m_rareData->additionalSet = rareData->additionalSet;
        // NOTE: We don't copy the callLinkInfo, since that's created during code generation.
        result->m_rareData->customAccessor.opaque = rareData->customAccessor.opaque;
        result->m_rareData->customSlotBase = rareData->customSlotBase;
        result->m_rareData->intrinsicFunction = rareData->intrinsicFunction;
    }
    return result;
}

Vector<WatchpointSet*, 2> AccessCase::commit(VM& vm, const Identifier& ident)
{
    // It's fine to commit something that is already committed. That arises when we switch to using
    // newly allocated watchpoints. When it happens, it's not efficient - but we think that's OK
    // because most AccessCases have no extra watchpoints anyway.
    RELEASE_ASSERT(m_state == Primordial || m_state == Committed);
    
    Vector<WatchpointSet*, 2> result;
    
    if ((structure() && structure()->needImpurePropertyWatchpoint())
        || m_conditionSet.needImpurePropertyWatchpoint())
        result.append(vm.ensureWatchpointSetForImpureProperty(ident));

    if (additionalSet())
        result.append(additionalSet());
    
    m_state = Committed;
    
    return result;
}

bool AccessCase::guardedByStructureCheck() const
{
    if (viaProxy())
        return false;

    switch (m_type) {
    case MegamorphicLoad:
    case ArrayLength:
    case StringLength:
    case DirectArgumentsLength:
    case ScopedArgumentsLength:
        return false;
    default:
        return true;
    }
}

JSObject* AccessCase::alternateBase() const
{
    if (customSlotBase())
        return customSlotBase();
    return conditionSet().slotBaseCondition().object();
}

bool AccessCase::doesCalls(Vector<JSCell*>* cellsToMark) const
{
    switch (type()) {
    case Getter:
    case Setter:
    case CustomValueGetter:
    case CustomAccessorGetter:
    case CustomValueSetter:
    case CustomAccessorSetter:
        return true;
    case Transition:
        if (newStructure()->outOfLineCapacity() != structure()->outOfLineCapacity()
            && structure()->couldHaveIndexingHeader()) {
            if (cellsToMark)
                cellsToMark->append(newStructure());
            return true;
        }
        return false;
    default:
        return false;
    }
}

bool AccessCase::couldStillSucceed() const
{
    return m_conditionSet.structuresEnsureValidityAssumingImpurePropertyWatchpoint();
}

bool AccessCase::canBeReplacedByMegamorphicLoad() const
{
    if (type() == MegamorphicLoad)
        return true;
    
    return type() == Load
        && !viaProxy()
        && conditionSet().isEmpty()
        && !additionalSet()
        && !customSlotBase();
}

bool AccessCase::canReplace(const AccessCase& other) const
{
    // This puts in a good effort to try to figure out if 'other' is made superfluous by '*this'.
    // It's fine for this to return false if it's in doubt.

    switch (type()) {
    case MegamorphicLoad:
        return other.canBeReplacedByMegamorphicLoad();
    case ArrayLength:
    case StringLength:
    case DirectArgumentsLength:
    case ScopedArgumentsLength:
        return other.type() == type();
    default:
        if (!guardedByStructureCheck() || !other.guardedByStructureCheck())
            return false;
        
        return structure() == other.structure();
    }
}

void AccessCase::dump(PrintStream& out) const
{
    out.print(m_type, ":(");

    CommaPrinter comma;
    
    out.print(comma, m_state);

    if (m_type == Transition)
        out.print(comma, "structure = ", pointerDump(structure()), " -> ", pointerDump(newStructure()));
    else if (m_structure)
        out.print(comma, "structure = ", pointerDump(m_structure.get()));

    if (isValidOffset(m_offset))
        out.print(comma, "offset = ", m_offset);
    if (!m_conditionSet.isEmpty())
        out.print(comma, "conditions = ", m_conditionSet);

    if (RareData* rareData = m_rareData.get()) {
        if (rareData->viaProxy)
            out.print(comma, "viaProxy = ", rareData->viaProxy);
        if (rareData->additionalSet)
            out.print(comma, "additionalSet = ", RawPointer(rareData->additionalSet.get()));
        if (rareData->callLinkInfo)
            out.print(comma, "callLinkInfo = ", RawPointer(rareData->callLinkInfo.get()));
        if (rareData->customAccessor.opaque)
            out.print(comma, "customAccessor = ", RawPointer(rareData->customAccessor.opaque));
        if (rareData->customSlotBase)
            out.print(comma, "customSlotBase = ", RawPointer(rareData->customSlotBase.get()));
    }

    out.print(")");
}

bool AccessCase::visitWeak(VM& vm) const
{
    if (m_structure && !Heap::isMarked(m_structure.get()))
        return false;
    if (!m_conditionSet.areStillLive())
        return false;
    if (m_rareData) {
        if (m_rareData->callLinkInfo)
            m_rareData->callLinkInfo->visitWeak(vm);
        if (m_rareData->customSlotBase && !Heap::isMarked(m_rareData->customSlotBase.get()))
            return false;
        if (m_rareData->intrinsicFunction && !Heap::isMarked(m_rareData->intrinsicFunction.get()))
            return false;
    }
    return true;
}

bool AccessCase::propagateTransitions(SlotVisitor& visitor) const
{
    bool result = true;
    
    if (m_structure)
        result &= m_structure->markIfCheap(visitor);
    
    switch (m_type) {
    case Transition:
        if (Heap::isMarkedConcurrently(m_structure->previousID()))
            visitor.appendUnbarrieredReadOnlyPointer(m_structure.get());
        else
            result = false;
        break;
    default:
        break;
    }
    
    return result;
}

void AccessCase::generateWithGuard(
    AccessGenerationState& state, CCallHelpers::JumpList& fallThrough)
{
    SuperSamplerScope superSamplerScope(false);

    RELEASE_ASSERT(m_state == Committed);
    m_state = Generated;
    
    CCallHelpers& jit = *state.jit;
    VM& vm = *jit.vm();
    const Identifier& ident = *state.ident;
    StructureStubInfo& stubInfo = *state.stubInfo;
    JSValueRegs valueRegs = state.valueRegs;
    GPRReg baseGPR = state.baseGPR;
    GPRReg scratchGPR = state.scratchGPR;
    
    UNUSED_PARAM(vm);

    switch (m_type) {
    case ArrayLength: {
        ASSERT(!viaProxy());
        jit.load8(CCallHelpers::Address(baseGPR, JSCell::indexingTypeOffset()), scratchGPR);
        fallThrough.append(
            jit.branchTest32(
                CCallHelpers::Zero, scratchGPR, CCallHelpers::TrustedImm32(IsArray)));
        fallThrough.append(
            jit.branchTest32(
                CCallHelpers::Zero, scratchGPR, CCallHelpers::TrustedImm32(IndexingShapeMask)));
        break;
    }

    case StringLength: {
        ASSERT(!viaProxy());
        fallThrough.append(
            jit.branch8(
                CCallHelpers::NotEqual,
                CCallHelpers::Address(baseGPR, JSCell::typeInfoTypeOffset()),
                CCallHelpers::TrustedImm32(StringType)));
        break;
    }
        
    case DirectArgumentsLength: {
        ASSERT(!viaProxy());
        fallThrough.append(
            jit.branch8(
                CCallHelpers::NotEqual,
                CCallHelpers::Address(baseGPR, JSCell::typeInfoTypeOffset()),
                CCallHelpers::TrustedImm32(DirectArgumentsType)));

        fallThrough.append(
            jit.branchTestPtr(
                CCallHelpers::NonZero,
                CCallHelpers::Address(baseGPR, DirectArguments::offsetOfOverrides())));
        jit.load32(
            CCallHelpers::Address(baseGPR, DirectArguments::offsetOfLength()),
            valueRegs.payloadGPR());
        jit.boxInt32(valueRegs.payloadGPR(), valueRegs);
        state.succeed();
        return;
    }
        
    case ScopedArgumentsLength: {
        ASSERT(!viaProxy());
        fallThrough.append(
            jit.branch8(
                CCallHelpers::NotEqual,
                CCallHelpers::Address(baseGPR, JSCell::typeInfoTypeOffset()),
                CCallHelpers::TrustedImm32(ScopedArgumentsType)));

        fallThrough.append(
            jit.branchTest8(
                CCallHelpers::NonZero,
                CCallHelpers::Address(baseGPR, ScopedArguments::offsetOfOverrodeThings())));
        jit.load32(
            CCallHelpers::Address(baseGPR, ScopedArguments::offsetOfTotalLength()),
            valueRegs.payloadGPR());
        jit.boxInt32(valueRegs.payloadGPR(), valueRegs);
        state.succeed();
        return;
    }
        
    case MegamorphicLoad: {
        UniquedStringImpl* key = ident.impl();
        unsigned hash = IdentifierRepHash::hash(key);
        
        ScratchRegisterAllocator allocator(stubInfo.patch.usedRegisters);
        allocator.lock(baseGPR);
#if USE(JSVALUE32_64)
        allocator.lock(static_cast<GPRReg>(stubInfo.patch.baseTagGPR));
#endif
        allocator.lock(valueRegs);
        allocator.lock(scratchGPR);
        
        GPRReg intermediateGPR = scratchGPR;
        GPRReg maskGPR = allocator.allocateScratchGPR();
        GPRReg maskedHashGPR = allocator.allocateScratchGPR();
        GPRReg indexGPR = allocator.allocateScratchGPR();
        GPRReg offsetGPR = allocator.allocateScratchGPR();
        
        if (verbose) {
            dataLog("baseGPR = ", baseGPR, "\n");
            dataLog("valueRegs = ", valueRegs, "\n");
            dataLog("scratchGPR = ", scratchGPR, "\n");
            dataLog("intermediateGPR = ", intermediateGPR, "\n");
            dataLog("maskGPR = ", maskGPR, "\n");
            dataLog("maskedHashGPR = ", maskedHashGPR, "\n");
            dataLog("indexGPR = ", indexGPR, "\n");
            dataLog("offsetGPR = ", offsetGPR, "\n");
        }

        ScratchRegisterAllocator::PreservedState preservedState =
            allocator.preserveReusedRegistersByPushing(jit, ScratchRegisterAllocator::ExtraStackSpace::SpaceForCCall);

        CCallHelpers::JumpList myFailAndIgnore;
        CCallHelpers::JumpList myFallThrough;
        
        jit.emitLoadStructure(baseGPR, intermediateGPR, maskGPR);
        jit.loadPtr(
            CCallHelpers::Address(intermediateGPR, Structure::propertyTableUnsafeOffset()),
            intermediateGPR);
        
        myFailAndIgnore.append(jit.branchTestPtr(CCallHelpers::Zero, intermediateGPR));
        
        jit.load32(CCallHelpers::Address(intermediateGPR, PropertyTable::offsetOfIndexMask()), maskGPR);
        jit.loadPtr(CCallHelpers::Address(intermediateGPR, PropertyTable::offsetOfIndex()), indexGPR);
        jit.load32(
            CCallHelpers::Address(intermediateGPR, PropertyTable::offsetOfIndexSize()),
            intermediateGPR);

        jit.move(maskGPR, maskedHashGPR);
        jit.and32(CCallHelpers::TrustedImm32(hash), maskedHashGPR);
        jit.lshift32(CCallHelpers::TrustedImm32(2), intermediateGPR);
        jit.addPtr(indexGPR, intermediateGPR);
        
        CCallHelpers::Label loop = jit.label();
        
        jit.load32(CCallHelpers::BaseIndex(indexGPR, maskedHashGPR, CCallHelpers::TimesFour), offsetGPR);
        
        myFallThrough.append(
            jit.branch32(
                CCallHelpers::Equal,
                offsetGPR,
                CCallHelpers::TrustedImm32(PropertyTable::EmptyEntryIndex)));
        
        jit.sub32(CCallHelpers::TrustedImm32(1), offsetGPR);
        jit.mul32(CCallHelpers::TrustedImm32(sizeof(PropertyMapEntry)), offsetGPR, offsetGPR);
        jit.addPtr(intermediateGPR, offsetGPR);
        
        CCallHelpers::Jump collision =  jit.branchPtr(
            CCallHelpers::NotEqual,
            CCallHelpers::Address(offsetGPR, OBJECT_OFFSETOF(PropertyMapEntry, key)),
            CCallHelpers::TrustedImmPtr(key));
        
        // offsetGPR currently holds a pointer to the PropertyMapEntry, which has the offset and attributes.
        // Check them and then attempt the load.
        
        myFallThrough.append(
            jit.branchTest32(
                CCallHelpers::NonZero,
                CCallHelpers::Address(offsetGPR, OBJECT_OFFSETOF(PropertyMapEntry, attributes)),
                CCallHelpers::TrustedImm32(Accessor | CustomAccessor)));
        
        jit.load32(CCallHelpers::Address(offsetGPR, OBJECT_OFFSETOF(PropertyMapEntry, offset)), offsetGPR);
        
        jit.loadProperty(baseGPR, offsetGPR, valueRegs);
        
        allocator.restoreReusedRegistersByPopping(jit, preservedState);
        state.succeed();
        
        collision.link(&jit);

        jit.add32(CCallHelpers::TrustedImm32(1), maskedHashGPR);
        
        // FIXME: We could be smarter about this. Currently we're burning a GPR for the mask. But looping
        // around isn't super common so we could, for example, recompute the mask from the difference between
        // the table and index. But before we do that we should probably make it easier to multiply and
        // divide by the size of PropertyMapEntry. That probably involves making PropertyMapEntry be arranged
        // to have a power-of-2 size.
        jit.and32(maskGPR, maskedHashGPR);
        jit.jump().linkTo(loop, &jit);
        
        if (allocator.didReuseRegisters()) {
            myFailAndIgnore.link(&jit);
            allocator.restoreReusedRegistersByPopping(jit, preservedState);
            state.failAndIgnore.append(jit.jump());
            
            myFallThrough.link(&jit);
            allocator.restoreReusedRegistersByPopping(jit, preservedState);
            fallThrough.append(jit.jump());
        } else {
            state.failAndIgnore.append(myFailAndIgnore);
            fallThrough.append(myFallThrough);
        }
        return;
    }

    default: {
        if (viaProxy()) {
            fallThrough.append(
                jit.branch8(
                    CCallHelpers::NotEqual,
                    CCallHelpers::Address(baseGPR, JSCell::typeInfoTypeOffset()),
                    CCallHelpers::TrustedImm32(PureForwardingProxyType)));

            jit.loadPtr(CCallHelpers::Address(baseGPR, JSProxy::targetOffset()), scratchGPR);

            fallThrough.append(
                jit.branchStructure(
                    CCallHelpers::NotEqual,
                    CCallHelpers::Address(scratchGPR, JSCell::structureIDOffset()),
                    structure()));
        } else {
            fallThrough.append(
                jit.branchStructure(
                    CCallHelpers::NotEqual,
                    CCallHelpers::Address(baseGPR, JSCell::structureIDOffset()),
                    structure()));
        }
        break;
    } };

    generateImpl(state);
}

void AccessCase::generate(AccessGenerationState& state)
{
    RELEASE_ASSERT(m_state == Committed);
    m_state = Generated;
    
    generateImpl(state);
}

void AccessCase::generateImpl(AccessGenerationState& state)
{
    SuperSamplerScope superSamplerScope(false);
    if (verbose)
        dataLog("Generating code for: ", *this, "\n");
    
    ASSERT(m_state == Generated); // We rely on the callers setting this for us.
    
    CCallHelpers& jit = *state.jit;
    VM& vm = *jit.vm();
    CodeBlock* codeBlock = jit.codeBlock();
    StructureStubInfo& stubInfo = *state.stubInfo;
    const Identifier& ident = *state.ident;
    JSValueRegs valueRegs = state.valueRegs;
    GPRReg baseGPR = state.baseGPR;
    GPRReg scratchGPR = state.scratchGPR;

    ASSERT(m_conditionSet.structuresEnsureValidityAssumingImpurePropertyWatchpoint());

    for (const ObjectPropertyCondition& condition : m_conditionSet) {
        Structure* structure = condition.object()->structure();

        if (condition.isWatchableAssumingImpurePropertyWatchpoint()) {
            structure->addTransitionWatchpoint(state.addWatchpoint(condition));
            continue;
        }

        if (!condition.structureEnsuresValidityAssumingImpurePropertyWatchpoint(structure)) {
            // The reason why this cannot happen is that we require that PolymorphicAccess calls
            // AccessCase::generate() only after it has verified that
            // AccessCase::couldStillSucceed() returned true.
            
            dataLog("This condition is no longer met: ", condition, "\n");
            RELEASE_ASSERT_NOT_REACHED();
        }

        // We will emit code that has a weak reference that isn't otherwise listed anywhere.
        state.weakReferences.append(WriteBarrier<JSCell>(vm, codeBlock, structure));
        
        jit.move(CCallHelpers::TrustedImmPtr(condition.object()), scratchGPR);
        state.failAndRepatch.append(
            jit.branchStructure(
                CCallHelpers::NotEqual,
                CCallHelpers::Address(scratchGPR, JSCell::structureIDOffset()),
                structure));
    }

    switch (m_type) {
    case InHit:
    case InMiss:
        jit.boxBooleanPayload(m_type == InHit, valueRegs.payloadGPR());
        state.succeed();
        return;

    case Miss:
        jit.moveTrustedValue(jsUndefined(), valueRegs);
        state.succeed();
        return;

    case Load:
    case GetGetter:
    case Getter:
    case Setter:
    case CustomValueGetter:
    case CustomAccessorGetter:
    case CustomValueSetter:
    case CustomAccessorSetter: {
        if (isValidOffset(m_offset)) {
            Structure* currStructure;
            if (m_conditionSet.isEmpty())
                currStructure = structure();
            else
                currStructure = m_conditionSet.slotBaseCondition().object()->structure();
            currStructure->startWatchingPropertyForReplacements(vm, offset());
        }

        GPRReg baseForGetGPR;
        if (viaProxy()) {
            baseForGetGPR = valueRegs.payloadGPR();
            jit.loadPtr(
                CCallHelpers::Address(baseGPR, JSProxy::targetOffset()),
                baseForGetGPR);
        } else
            baseForGetGPR = baseGPR;

        GPRReg baseForAccessGPR;
        if (!m_conditionSet.isEmpty()) {
            jit.move(
                CCallHelpers::TrustedImmPtr(alternateBase()),
                scratchGPR);
            baseForAccessGPR = scratchGPR;
        } else
            baseForAccessGPR = baseForGetGPR;

        GPRReg loadedValueGPR = InvalidGPRReg;
        if (m_type != CustomValueGetter && m_type != CustomAccessorGetter && m_type != CustomValueSetter && m_type != CustomAccessorSetter) {
            if (m_type == Load || m_type == GetGetter)
                loadedValueGPR = valueRegs.payloadGPR();
            else
                loadedValueGPR = scratchGPR;

            GPRReg storageGPR;
            if (isInlineOffset(m_offset))
                storageGPR = baseForAccessGPR;
            else {
                jit.loadPtr(
                    CCallHelpers::Address(baseForAccessGPR, JSObject::butterflyOffset()),
                    loadedValueGPR);
                storageGPR = loadedValueGPR;
            }

#if USE(JSVALUE64)
            jit.load64(
                CCallHelpers::Address(storageGPR, offsetRelativeToBase(m_offset)), loadedValueGPR);
#else
            if (m_type == Load || m_type == GetGetter) {
                jit.load32(
                    CCallHelpers::Address(storageGPR, offsetRelativeToBase(m_offset) + TagOffset),
                    valueRegs.tagGPR());
            }
            jit.load32(
                CCallHelpers::Address(storageGPR, offsetRelativeToBase(m_offset) + PayloadOffset),
                loadedValueGPR);
#endif
        }

        if (m_type == Load || m_type == GetGetter) {
            state.succeed();
            return;
        }

        // Stuff for custom getters/setters.
        CCallHelpers::Call operationCall;

        // Stuff for JS getters/setters.
        CCallHelpers::DataLabelPtr addressOfLinkFunctionCheck;
        CCallHelpers::Call fastPathCall;
        CCallHelpers::Call slowPathCall;

        CCallHelpers::Jump success;
        CCallHelpers::Jump fail;

        // This also does the necessary calculations of whether or not we're an
        // exception handling call site.
        state.preserveLiveRegistersToStackForCall();

        jit.store32(
            CCallHelpers::TrustedImm32(state.callSiteIndexForExceptionHandlingOrOriginal().bits()),
            CCallHelpers::tagFor(static_cast<VirtualRegister>(CallFrameSlot::argumentCount)));

        if (m_type == Getter || m_type == Setter) {
            // Create a JS call using a JS call inline cache. Assume that:
            //
            // - SP is aligned and represents the extent of the calling compiler's stack usage.
            //
            // - FP is set correctly (i.e. it points to the caller's call frame header).
            //
            // - SP - FP is an aligned difference.
            //
            // - Any byte between FP (exclusive) and SP (inclusive) could be live in the calling
            //   code.
            //
            // Therefore, we temporarily grow the stack for the purpose of the call and then
            // shrink it after.

            RELEASE_ASSERT(!m_rareData->callLinkInfo);
            m_rareData->callLinkInfo = std::make_unique<CallLinkInfo>();
            
            // FIXME: If we generated a polymorphic call stub that jumped back to the getter
            // stub, which then jumped back to the main code, then we'd have a reachability
            // situation that the GC doesn't know about. The GC would ensure that the polymorphic
            // call stub stayed alive, and it would ensure that the main code stayed alive, but
            // it wouldn't know that the getter stub was alive. Ideally JIT stub routines would
            // be GC objects, and then we'd be able to say that the polymorphic call stub has a
            // reference to the getter stub.
            // https://bugs.webkit.org/show_bug.cgi?id=148914
            m_rareData->callLinkInfo->disallowStubs();
            
            m_rareData->callLinkInfo->setUpCall(
                CallLinkInfo::Call, stubInfo.codeOrigin, loadedValueGPR);

            CCallHelpers::JumpList done;

            // There is a "this" argument.
            unsigned numberOfParameters = 1;
            // ... and a value argument if we're calling a setter.
            if (m_type == Setter)
                numberOfParameters++;

            // Get the accessor; if there ain't one then the result is jsUndefined().
            if (m_type == Setter) {
                jit.loadPtr(
                    CCallHelpers::Address(loadedValueGPR, GetterSetter::offsetOfSetter()),
                    loadedValueGPR);
            } else {
                jit.loadPtr(
                    CCallHelpers::Address(loadedValueGPR, GetterSetter::offsetOfGetter()),
                    loadedValueGPR);
            }

            CCallHelpers::Jump returnUndefined = jit.branchTestPtr(
                CCallHelpers::Zero, loadedValueGPR);

            unsigned numberOfRegsForCall = CallFrame::headerSizeInRegisters + numberOfParameters;

            unsigned numberOfBytesForCall =
                numberOfRegsForCall * sizeof(Register) - sizeof(CallerFrameAndPC);

            unsigned alignedNumberOfBytesForCall =
                WTF::roundUpToMultipleOf(stackAlignmentBytes(), numberOfBytesForCall);

            jit.subPtr(
                CCallHelpers::TrustedImm32(alignedNumberOfBytesForCall),
                CCallHelpers::stackPointerRegister);

            CCallHelpers::Address calleeFrame = CCallHelpers::Address(
                CCallHelpers::stackPointerRegister,
                -static_cast<ptrdiff_t>(sizeof(CallerFrameAndPC)));

            jit.store32(
                CCallHelpers::TrustedImm32(numberOfParameters),
                calleeFrame.withOffset(CallFrameSlot::argumentCount * sizeof(Register) + PayloadOffset));

            jit.storeCell(
                loadedValueGPR, calleeFrame.withOffset(CallFrameSlot::callee * sizeof(Register)));

            jit.storeCell(
                baseForGetGPR,
                calleeFrame.withOffset(virtualRegisterForArgument(0).offset() * sizeof(Register)));

            if (m_type == Setter) {
                jit.storeValue(
                    valueRegs,
                    calleeFrame.withOffset(
                        virtualRegisterForArgument(1).offset() * sizeof(Register)));
            }

            CCallHelpers::Jump slowCase = jit.branchPtrWithPatch(
                CCallHelpers::NotEqual, loadedValueGPR, addressOfLinkFunctionCheck,
                CCallHelpers::TrustedImmPtr(0));

            fastPathCall = jit.nearCall();
            if (m_type == Getter)
                jit.setupResults(valueRegs);
            done.append(jit.jump());

            slowCase.link(&jit);
            jit.move(loadedValueGPR, GPRInfo::regT0);
#if USE(JSVALUE32_64)
            // We *always* know that the getter/setter, if non-null, is a cell.
            jit.move(CCallHelpers::TrustedImm32(JSValue::CellTag), GPRInfo::regT1);
#endif
            jit.move(CCallHelpers::TrustedImmPtr(m_rareData->callLinkInfo.get()), GPRInfo::regT2);
            slowPathCall = jit.nearCall();
            if (m_type == Getter)
                jit.setupResults(valueRegs);
            done.append(jit.jump());

            returnUndefined.link(&jit);
            if (m_type == Getter)
                jit.moveTrustedValue(jsUndefined(), valueRegs);

            done.link(&jit);

            jit.addPtr(CCallHelpers::TrustedImm32((codeBlock->stackPointerOffset() * sizeof(Register)) - state.preservedReusedRegisterState.numberOfBytesPreserved - state.numberOfStackBytesUsedForRegisterPreservation()),
                GPRInfo::callFrameRegister, CCallHelpers::stackPointerRegister);
            state.restoreLiveRegistersFromStackForCall(isGetter());

            jit.addLinkTask(
                [=, &vm] (LinkBuffer& linkBuffer) {
                    m_rareData->callLinkInfo->setCallLocations(
                        linkBuffer.locationOfNearCall(slowPathCall),
                        linkBuffer.locationOf(addressOfLinkFunctionCheck),
                        linkBuffer.locationOfNearCall(fastPathCall));

                    linkBuffer.link(
                        slowPathCall,
                        CodeLocationLabel(vm.getCTIStub(linkCallThunkGenerator).code()));
                });
        } else {
            // Need to make room for the C call so any of our stack spillage isn't overwritten. It's
            // hard to track if someone did spillage or not, so we just assume that we always need
            // to make some space here.
            jit.makeSpaceOnStackForCCall();

            // getter: EncodedJSValue (*GetValueFunc)(ExecState*, EncodedJSValue thisValue, PropertyName);
            // setter: void (*PutValueFunc)(ExecState*, EncodedJSValue thisObject, EncodedJSValue value);
            // Custom values are passed the slotBase (the property holder), custom accessors are passed the thisVaule (reciever).
            // FIXME: Remove this differences in custom values and custom accessors.
            // https://bugs.webkit.org/show_bug.cgi?id=158014
            GPRReg baseForCustomValue = m_type == CustomValueGetter || m_type == CustomValueSetter ? baseForAccessGPR : baseForGetGPR;
#if USE(JSVALUE64)
            if (m_type == CustomValueGetter || m_type == CustomAccessorGetter) {
                jit.setupArgumentsWithExecState(
                    baseForCustomValue,
                    CCallHelpers::TrustedImmPtr(ident.impl()));
            } else
                jit.setupArgumentsWithExecState(baseForCustomValue, valueRegs.gpr());
#else
            if (m_type == CustomValueGetter || m_type == CustomAccessorGetter) {
                jit.setupArgumentsWithExecState(
                    EABI_32BIT_DUMMY_ARG baseForCustomValue,
                    CCallHelpers::TrustedImm32(JSValue::CellTag),
                    CCallHelpers::TrustedImmPtr(ident.impl()));
            } else {
                jit.setupArgumentsWithExecState(
                    EABI_32BIT_DUMMY_ARG baseForCustomValue,
                    CCallHelpers::TrustedImm32(JSValue::CellTag),
                    valueRegs.payloadGPR(), valueRegs.tagGPR());
            }
#endif
            jit.storePtr(GPRInfo::callFrameRegister, &vm.topCallFrame);

            operationCall = jit.call();
            jit.addLinkTask(
                [=] (LinkBuffer& linkBuffer) {
                    linkBuffer.link(operationCall, FunctionPtr(m_rareData->customAccessor.opaque));
                });

            if (m_type == CustomValueGetter || m_type == CustomAccessorGetter)
                jit.setupResults(valueRegs);
            jit.reclaimSpaceOnStackForCCall();

            CCallHelpers::Jump noException =
                jit.emitExceptionCheck(CCallHelpers::InvertedExceptionCheck);

            state.restoreLiveRegistersFromStackForCallWithThrownException();
            state.emitExplicitExceptionHandler();
        
            noException.link(&jit);
            state.restoreLiveRegistersFromStackForCall(isGetter());
        }
        state.succeed();
        return;
    }

    case Replace: {
        if (InferredType* type = structure()->inferredTypeFor(ident.impl())) {
            if (verbose)
                dataLog("Have type: ", type->descriptor(), "\n");
            state.failAndRepatch.append(
                jit.branchIfNotType(valueRegs, scratchGPR, type->descriptor()));
        } else if (verbose)
            dataLog("Don't have type.\n");
        
        if (isInlineOffset(m_offset)) {
            jit.storeValue(
                valueRegs,
                CCallHelpers::Address(
                    baseGPR,
                    JSObject::offsetOfInlineStorage() +
                    offsetInInlineStorage(m_offset) * sizeof(JSValue)));
        } else {
            jit.loadPtr(CCallHelpers::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
            jit.storeValue(
                valueRegs,
                CCallHelpers::Address(
                    scratchGPR, offsetInButterfly(m_offset) * sizeof(JSValue)));
        }
        state.succeed();
        return;
    }

    case Transition: {
        // AccessCase::transition() should have returned null if this wasn't true.
        RELEASE_ASSERT(GPRInfo::numberOfRegisters >= 6 || !structure()->outOfLineCapacity() || structure()->outOfLineCapacity() == newStructure()->outOfLineCapacity());

        if (InferredType* type = newStructure()->inferredTypeFor(ident.impl())) {
            if (verbose)
                dataLog("Have type: ", type->descriptor(), "\n");
            state.failAndRepatch.append(
                jit.branchIfNotType(valueRegs, scratchGPR, type->descriptor()));
        } else if (verbose)
            dataLog("Don't have type.\n");
        
        // NOTE: This logic is duplicated in AccessCase::doesCalls(). It's important that doesCalls() knows
        // exactly when this would make calls.
        bool allocating = newStructure()->outOfLineCapacity() != structure()->outOfLineCapacity();
        bool reallocating = allocating && structure()->outOfLineCapacity();
        bool allocatingInline = allocating && !structure()->couldHaveIndexingHeader();

        ScratchRegisterAllocator allocator(stubInfo.patch.usedRegisters);
        allocator.lock(baseGPR);
#if USE(JSVALUE32_64)
        allocator.lock(static_cast<GPRReg>(stubInfo.patch.baseTagGPR));
#endif
        allocator.lock(valueRegs);
        allocator.lock(scratchGPR);

        GPRReg scratchGPR2 = InvalidGPRReg;
        GPRReg scratchGPR3 = InvalidGPRReg;
        if (allocatingInline) {
            scratchGPR2 = allocator.allocateScratchGPR();
            scratchGPR3 = allocator.allocateScratchGPR();
        }

        ScratchRegisterAllocator::PreservedState preservedState =
            allocator.preserveReusedRegistersByPushing(jit, ScratchRegisterAllocator::ExtraStackSpace::SpaceForCCall);
        
        CCallHelpers::JumpList slowPath;

        ASSERT(structure()->transitionWatchpointSetHasBeenInvalidated());

        if (allocating) {
            size_t newSize = newStructure()->outOfLineCapacity() * sizeof(JSValue);
            
            if (allocatingInline) {
                MarkedAllocator* allocator = vm.heap.allocatorForAuxiliaryData(newSize);
                
                if (!allocator) {
                    // Yuck, this case would suck!
                    slowPath.append(jit.jump());
                }
                
                jit.move(CCallHelpers::TrustedImmPtr(allocator), scratchGPR2);
                jit.emitAllocate(scratchGPR, allocator, scratchGPR2, scratchGPR3, slowPath);
                jit.addPtr(CCallHelpers::TrustedImm32(newSize + sizeof(IndexingHeader)), scratchGPR);
                
                if (reallocating) {
                    // Handle the case where we are reallocating (i.e. the old structure/butterfly
                    // already had out-of-line property storage).
                    size_t oldSize = structure()->outOfLineCapacity() * sizeof(JSValue);
                    ASSERT(newSize > oldSize);
            
                    jit.loadPtr(CCallHelpers::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR3);
                    
                    // We have scratchGPR = new storage, scratchGPR3 = old storage,
                    // scratchGPR2 = available
                    for (size_t offset = 0; offset < oldSize; offset += sizeof(void*)) {
                        jit.loadPtr(
                            CCallHelpers::Address(
                                scratchGPR3,
                                -static_cast<ptrdiff_t>(
                                    offset + sizeof(JSValue) + sizeof(void*))),
                            scratchGPR2);
                        jit.storePtr(
                            scratchGPR2,
                            CCallHelpers::Address(
                                scratchGPR,
                                -static_cast<ptrdiff_t>(offset + sizeof(JSValue) + sizeof(void*))));
                    }
                }
            } else {
                // Handle the case where we are allocating out-of-line using an operation.
                RegisterSet extraRegistersToPreserve;
                extraRegistersToPreserve.set(baseGPR);
                extraRegistersToPreserve.set(valueRegs);
                state.preserveLiveRegistersToStackForCall(extraRegistersToPreserve);
                
                jit.store32(
                    CCallHelpers::TrustedImm32(
                        state.callSiteIndexForExceptionHandlingOrOriginal().bits()),
                    CCallHelpers::tagFor(static_cast<VirtualRegister>(CallFrameSlot::argumentCount)));
                
                jit.makeSpaceOnStackForCCall();
                
                if (!reallocating) {
                    jit.setupArgumentsWithExecState(baseGPR);
                    
                    CCallHelpers::Call operationCall = jit.call();
                    jit.addLinkTask(
                        [=] (LinkBuffer& linkBuffer) {
                            linkBuffer.link(
                                operationCall,
                                FunctionPtr(operationReallocateButterflyToHavePropertyStorageWithInitialCapacity));
                        });
                } else {
                    // Handle the case where we are reallocating (i.e. the old structure/butterfly
                    // already had out-of-line property storage).
                    jit.setupArgumentsWithExecState(
                        baseGPR, CCallHelpers::TrustedImm32(newSize / sizeof(JSValue)));
                    
                    CCallHelpers::Call operationCall = jit.call();
                    jit.addLinkTask(
                        [=] (LinkBuffer& linkBuffer) {
                            linkBuffer.link(
                                operationCall,
                                FunctionPtr(operationReallocateButterflyToGrowPropertyStorage));
                        });
                }
                
                jit.reclaimSpaceOnStackForCCall();
                jit.move(GPRInfo::returnValueGPR, scratchGPR);
                
                CCallHelpers::Jump noException =
                    jit.emitExceptionCheck(CCallHelpers::InvertedExceptionCheck);
                
                state.restoreLiveRegistersFromStackForCallWithThrownException();
                state.emitExplicitExceptionHandler();
                
                noException.link(&jit);
                state.restoreLiveRegistersFromStackForCall();
            }
        }

        if (isInlineOffset(m_offset)) {
            jit.storeValue(
                valueRegs,
                CCallHelpers::Address(
                    baseGPR,
                    JSObject::offsetOfInlineStorage() +
                    offsetInInlineStorage(m_offset) * sizeof(JSValue)));
        } else {
            if (!allocating)
                jit.loadPtr(CCallHelpers::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
            jit.storeValue(
                valueRegs,
                CCallHelpers::Address(scratchGPR, offsetInButterfly(m_offset) * sizeof(JSValue)));
        }
        
        // If we had allocated using an operation then we would have already executed the store
        // barrier and we would have already stored the butterfly into the object.
        if (allocatingInline) {
            CCallHelpers::Jump ownerIsRememberedOrInEden = jit.jumpIfIsRememberedOrInEden(baseGPR);
            WriteBarrierBuffer& writeBarrierBuffer = jit.vm()->heap.writeBarrierBuffer();
            jit.load32(writeBarrierBuffer.currentIndexAddress(), scratchGPR2);
            slowPath.append(
                jit.branch32(
                    CCallHelpers::AboveOrEqual, scratchGPR2,
                    CCallHelpers::TrustedImm32(writeBarrierBuffer.capacity())));
            
            jit.add32(CCallHelpers::TrustedImm32(1), scratchGPR2);
            jit.store32(scratchGPR2, writeBarrierBuffer.currentIndexAddress());
            
            jit.move(CCallHelpers::TrustedImmPtr(writeBarrierBuffer.buffer()), scratchGPR3);
            // We use an offset of -sizeof(void*) because we already added 1 to scratchGPR2.
            jit.storePtr(
                baseGPR,
                CCallHelpers::BaseIndex(
                    scratchGPR3, scratchGPR2, CCallHelpers::ScalePtr,
                    static_cast<int32_t>(-sizeof(void*))));
            ownerIsRememberedOrInEden.link(&jit);
            
            // We set the new butterfly and the structure last. Doing it this way ensures that
            // whatever we had done up to this point is forgotten if we choose to branch to slow
            // path.
            
            jit.storePtr(scratchGPR, CCallHelpers::Address(baseGPR, JSObject::butterflyOffset()));
        }
        
        uint32_t structureBits = bitwise_cast<uint32_t>(newStructure()->id());
        jit.store32(
            CCallHelpers::TrustedImm32(structureBits),
            CCallHelpers::Address(baseGPR, JSCell::structureIDOffset()));

        allocator.restoreReusedRegistersByPopping(jit, preservedState);
        state.succeed();
        
        // We will have a slow path if we were allocating without the help of an operation.
        if (allocatingInline) {
            if (allocator.didReuseRegisters()) {
                slowPath.link(&jit);
                allocator.restoreReusedRegistersByPopping(jit, preservedState);
                state.failAndIgnore.append(jit.jump());
            } else
                state.failAndIgnore.append(slowPath);
        } else
            RELEASE_ASSERT(slowPath.empty());
        return;
    }

    case ArrayLength: {
        jit.loadPtr(CCallHelpers::Address(baseGPR, JSObject::butterflyOffset()), scratchGPR);
        jit.load32(CCallHelpers::Address(scratchGPR, ArrayStorage::lengthOffset()), scratchGPR);
        state.failAndIgnore.append(
            jit.branch32(CCallHelpers::LessThan, scratchGPR, CCallHelpers::TrustedImm32(0)));
        jit.boxInt32(scratchGPR, valueRegs);
        state.succeed();
        return;
    }

    case StringLength: {
        jit.load32(CCallHelpers::Address(baseGPR, JSString::offsetOfLength()), valueRegs.payloadGPR());
        jit.boxInt32(valueRegs.payloadGPR(), valueRegs);
        state.succeed();
        return;
    }
        
    case IntrinsicGetter: {
        RELEASE_ASSERT(isValidOffset(offset()));

        // We need to ensure the getter value does not move from under us. Note that GetterSetters
        // are immutable so we just need to watch the property not any value inside it.
        Structure* currStructure;
        if (m_conditionSet.isEmpty())
            currStructure = structure();
        else
            currStructure = m_conditionSet.slotBaseCondition().object()->structure();
        currStructure->startWatchingPropertyForReplacements(vm, offset());

        emitIntrinsicGetter(state);
        return;
    }

    case DirectArgumentsLength:
    case ScopedArgumentsLength:
    case MegamorphicLoad:
        // These need to be handled by generateWithGuard(), since the guard is part of the
        // algorithm. We can be sure that nobody will call generate() directly for these since they
        // are not guarded by structure checks.
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    RELEASE_ASSERT_NOT_REACHED();
}

PolymorphicAccess::PolymorphicAccess() { }
PolymorphicAccess::~PolymorphicAccess() { }

AccessGenerationResult PolymorphicAccess::addCases(
    VM& vm, CodeBlock* codeBlock, StructureStubInfo& stubInfo, const Identifier& ident,
    Vector<std::unique_ptr<AccessCase>, 2> originalCasesToAdd)
{
    SuperSamplerScope superSamplerScope(false);
    
    // This method will add the originalCasesToAdd to the list one at a time while preserving the
    // invariants:
    // - If a newly added case canReplace() any existing case, then the existing case is removed before
    //   the new case is added. Removal doesn't change order of the list. Any number of existing cases
    //   can be removed via the canReplace() rule.
    // - Cases in the list always appear in ascending order of time of addition. Therefore, if you
    //   cascade through the cases in reverse order, you will get the most recent cases first.
    // - If this method fails (returns null, doesn't add the cases), then both the previous case list
    //   and the previous stub are kept intact and the new cases are destroyed. It's OK to attempt to
    //   add more things after failure.
    
    // First ensure that the originalCasesToAdd doesn't contain duplicates.
    Vector<std::unique_ptr<AccessCase>> casesToAdd;
    for (unsigned i = 0; i < originalCasesToAdd.size(); ++i) {
        std::unique_ptr<AccessCase> myCase = WTFMove(originalCasesToAdd[i]);

        // Add it only if it is not replaced by the subsequent cases in the list.
        bool found = false;
        for (unsigned j = i + 1; j < originalCasesToAdd.size(); ++j) {
            if (originalCasesToAdd[j]->canReplace(*myCase)) {
                found = true;
                break;
            }
        }

        if (found)
            continue;
        
        casesToAdd.append(WTFMove(myCase));
    }

    if (verbose)
        dataLog("casesToAdd: ", listDump(casesToAdd), "\n");

    // If there aren't any cases to add, then fail on the grounds that there's no point to generating a
    // new stub that will be identical to the old one. Returning null should tell the caller to just
    // keep doing what they were doing before.
    if (casesToAdd.isEmpty())
        return AccessGenerationResult::MadeNoChanges;

    // Now add things to the new list. Note that at this point, we will still have old cases that
    // may be replaced by the new ones. That's fine. We will sort that out when we regenerate.
    for (auto& caseToAdd : casesToAdd) {
        commit(vm, m_watchpoints, codeBlock, stubInfo, ident, *caseToAdd);
        m_list.append(WTFMove(caseToAdd));
    }
    
    if (verbose)
        dataLog("After addCases: m_list: ", listDump(m_list), "\n");

    return AccessGenerationResult::Buffered;
}

AccessGenerationResult PolymorphicAccess::addCase(
    VM& vm, CodeBlock* codeBlock, StructureStubInfo& stubInfo, const Identifier& ident,
    std::unique_ptr<AccessCase> newAccess)
{
    Vector<std::unique_ptr<AccessCase>, 2> newAccesses;
    newAccesses.append(WTFMove(newAccess));
    return addCases(vm, codeBlock, stubInfo, ident, WTFMove(newAccesses));
}

bool PolymorphicAccess::visitWeak(VM& vm) const
{
    for (unsigned i = 0; i < size(); ++i) {
        if (!at(i).visitWeak(vm))
            return false;
    }
    if (Vector<WriteBarrier<JSCell>>* weakReferences = m_weakReferences.get()) {
        for (WriteBarrier<JSCell>& weakReference : *weakReferences) {
            if (!Heap::isMarked(weakReference.get()))
                return false;
        }
    }
    return true;
}

bool PolymorphicAccess::propagateTransitions(SlotVisitor& visitor) const
{
    bool result = true;
    for (unsigned i = 0; i < size(); ++i)
        result &= at(i).propagateTransitions(visitor);
    return result;
}

void PolymorphicAccess::dump(PrintStream& out) const
{
    out.print(RawPointer(this), ":[");
    CommaPrinter comma;
    for (auto& entry : m_list)
        out.print(comma, *entry);
    out.print("]");
}

void PolymorphicAccess::commit(
    VM& vm, std::unique_ptr<WatchpointsOnStructureStubInfo>& watchpoints, CodeBlock* codeBlock,
    StructureStubInfo& stubInfo, const Identifier& ident, AccessCase& accessCase)
{
    // NOTE: We currently assume that this is relatively rare. It mainly arises for accesses to
    // properties on DOM nodes. For sure we cache many DOM node accesses, but even in
    // Real Pages (TM), we appear to spend most of our time caching accesses to properties on
    // vanilla objects or exotic objects from within JSC (like Arguments, those are super popular).
    // Those common kinds of JSC object accesses don't hit this case.
    
    for (WatchpointSet* set : accessCase.commit(vm, ident)) {
        Watchpoint* watchpoint =
            WatchpointsOnStructureStubInfo::ensureReferenceAndAddWatchpoint(
                watchpoints, codeBlock, &stubInfo, ObjectPropertyCondition());
        
        set->add(watchpoint);
    }
}

AccessGenerationResult PolymorphicAccess::regenerate(
    VM& vm, CodeBlock* codeBlock, StructureStubInfo& stubInfo, const Identifier& ident)
{
    SuperSamplerScope superSamplerScope(false);
    
    if (verbose)
        dataLog("Regenerate with m_list: ", listDump(m_list), "\n");
    
    AccessGenerationState state;

    state.access = this;
    state.stubInfo = &stubInfo;
    state.ident = &ident;
    
    state.baseGPR = static_cast<GPRReg>(stubInfo.patch.baseGPR);
    state.valueRegs = stubInfo.valueRegs();

    ScratchRegisterAllocator allocator(stubInfo.patch.usedRegisters);
    state.allocator = &allocator;
    allocator.lock(state.baseGPR);
    allocator.lock(state.valueRegs);
#if USE(JSVALUE32_64)
    allocator.lock(static_cast<GPRReg>(stubInfo.patch.baseTagGPR));
#endif

    state.scratchGPR = allocator.allocateScratchGPR();
    
    CCallHelpers jit(&vm, codeBlock);
    state.jit = &jit;

    state.preservedReusedRegisterState =
        allocator.preserveReusedRegistersByPushing(jit, ScratchRegisterAllocator::ExtraStackSpace::NoExtraSpace);

    // Regenerating is our opportunity to figure out what our list of cases should look like. We
    // do this here. The newly produced 'cases' list may be smaller than m_list. We don't edit
    // m_list in-place because we may still fail, in which case we want the PolymorphicAccess object
    // to be unmutated. For sure, we want it to hang onto any data structures that may be referenced
    // from the code of the current stub (aka previous).
    ListType cases;
    unsigned srcIndex = 0;
    unsigned dstIndex = 0;
    while (srcIndex < m_list.size()) {
        std::unique_ptr<AccessCase> someCase = WTFMove(m_list[srcIndex++]);
        
        // If the case had been generated, then we have to keep the original in m_list in case we
        // fail to regenerate. That case may have data structures that are used by the code that it
        // had generated. If the case had not been generated, then we want to remove it from m_list.
        bool isGenerated = someCase->state() == AccessCase::Generated;
        
        [&] () {
            if (!someCase->couldStillSucceed())
                return;

            // Figure out if this is replaced by any later case.
            for (unsigned j = srcIndex; j < m_list.size(); ++j) {
                if (m_list[j]->canReplace(*someCase))
                    return;
            }
            
            if (isGenerated)
                cases.append(someCase->clone());
            else
                cases.append(WTFMove(someCase));
        }();
        
        if (isGenerated)
            m_list[dstIndex++] = WTFMove(someCase);
    }
    m_list.resize(dstIndex);
    
    if (verbose)
        dataLog("In regenerate: cases: ", listDump(cases), "\n");
    
    // Now that we've removed obviously unnecessary cases, we can check if the megamorphic load
    // optimization is applicable. Note that we basically tune megamorphicLoadCost according to code
    // size. It would be faster to just allow more repatching with many load cases, and avoid the
    // megamorphicLoad optimization, if we had infinite executable memory.
    if (cases.size() >= Options::maxAccessVariantListSize()) {
        unsigned numSelfLoads = 0;
        for (auto& newCase : cases) {
            if (newCase->canBeReplacedByMegamorphicLoad())
                numSelfLoads++;
        }
        
        if (numSelfLoads >= Options::megamorphicLoadCost()) {
            if (auto mega = AccessCase::megamorphicLoad(vm, codeBlock)) {
                cases.removeAllMatching(
                    [&] (std::unique_ptr<AccessCase>& newCase) -> bool {
                        return newCase->canBeReplacedByMegamorphicLoad();
                    });
                
                cases.append(WTFMove(mega));
            }
        }
    }
    
    if (verbose)
        dataLog("Optimized cases: ", listDump(cases), "\n");
    
    // At this point we're convinced that 'cases' contains the cases that we want to JIT now and we
    // won't change that set anymore.
    
    bool allGuardedByStructureCheck = true;
    bool hasJSGetterSetterCall = false;
    for (auto& newCase : cases) {
        commit(vm, state.watchpoints, codeBlock, stubInfo, ident, *newCase);
        allGuardedByStructureCheck &= newCase->guardedByStructureCheck();
        if (newCase->type() == AccessCase::Getter || newCase->type() == AccessCase::Setter)
            hasJSGetterSetterCall = true;
    }

    if (cases.isEmpty()) {
        // This is super unlikely, but we make it legal anyway.
        state.failAndRepatch.append(jit.jump());
    } else if (!allGuardedByStructureCheck || cases.size() == 1) {
        // If there are any proxies in the list, we cannot just use a binary switch over the structure.
        // We need to resort to a cascade. A cascade also happens to be optimal if we only have just
        // one case.
        CCallHelpers::JumpList fallThrough;

        // Cascade through the list, preferring newer entries.
        for (unsigned i = cases.size(); i--;) {
            fallThrough.link(&jit);
            fallThrough.clear();
            cases[i]->generateWithGuard(state, fallThrough);
        }
        state.failAndRepatch.append(fallThrough);
    } else {
        jit.load32(
            CCallHelpers::Address(state.baseGPR, JSCell::structureIDOffset()),
            state.scratchGPR);
        
        Vector<int64_t> caseValues(cases.size());
        for (unsigned i = 0; i < cases.size(); ++i)
            caseValues[i] = bitwise_cast<int32_t>(cases[i]->structure()->id());
        
        BinarySwitch binarySwitch(state.scratchGPR, caseValues, BinarySwitch::Int32);
        while (binarySwitch.advance(jit))
            cases[binarySwitch.caseIndex()]->generate(state);
        state.failAndRepatch.append(binarySwitch.fallThrough());
    }

    if (!state.failAndIgnore.empty()) {
        state.failAndIgnore.link(&jit);
        
        // Make sure that the inline cache optimization code knows that we are taking slow path because
        // of something that isn't patchable. The slow path will decrement "countdown" and will only
        // patch things if the countdown reaches zero. We increment the slow path count here to ensure
        // that the slow path does not try to patch.
#if CPU(X86) || CPU(X86_64)
        jit.move(CCallHelpers::TrustedImmPtr(&stubInfo.countdown), state.scratchGPR);
        jit.add8(CCallHelpers::TrustedImm32(1), CCallHelpers::Address(state.scratchGPR));
#else
        jit.load8(&stubInfo.countdown, state.scratchGPR);
        jit.add32(CCallHelpers::TrustedImm32(1), state.scratchGPR);
        jit.store8(state.scratchGPR, &stubInfo.countdown);
#endif
    }

    CCallHelpers::JumpList failure;
    if (allocator.didReuseRegisters()) {
        state.failAndRepatch.link(&jit);
        state.restoreScratch();
    } else
        failure = state.failAndRepatch;
    failure.append(jit.jump());

    CodeBlock* codeBlockThatOwnsExceptionHandlers = nullptr;
    CallSiteIndex callSiteIndexForExceptionHandling;
    if (state.needsToRestoreRegistersIfException() && hasJSGetterSetterCall) {
        // Emit the exception handler.
        // Note that this code is only reachable when doing genericUnwind from a pure JS getter/setter .
        // Note also that this is not reachable from custom getter/setter. Custom getter/setters will have 
        // their own exception handling logic that doesn't go through genericUnwind.
        MacroAssembler::Label makeshiftCatchHandler = jit.label();

        int stackPointerOffset = codeBlock->stackPointerOffset() * sizeof(EncodedJSValue);
        stackPointerOffset -= state.preservedReusedRegisterState.numberOfBytesPreserved;
        stackPointerOffset -= state.numberOfStackBytesUsedForRegisterPreservation();

        jit.loadPtr(vm.addressOfCallFrameForCatch(), GPRInfo::callFrameRegister);
        jit.addPtr(CCallHelpers::TrustedImm32(stackPointerOffset), GPRInfo::callFrameRegister, CCallHelpers::stackPointerRegister);

        state.restoreLiveRegistersFromStackForCallWithThrownException();
        state.restoreScratch();
        CCallHelpers::Jump jumpToOSRExitExceptionHandler = jit.jump();

        HandlerInfo oldHandler = state.originalExceptionHandler();
        CallSiteIndex newExceptionHandlingCallSite = state.callSiteIndexForExceptionHandling();
        jit.addLinkTask(
            [=] (LinkBuffer& linkBuffer) {
                linkBuffer.link(jumpToOSRExitExceptionHandler, oldHandler.nativeCode);

                HandlerInfo handlerToRegister = oldHandler;
                handlerToRegister.nativeCode = linkBuffer.locationOf(makeshiftCatchHandler);
                handlerToRegister.start = newExceptionHandlingCallSite.bits();
                handlerToRegister.end = newExceptionHandlingCallSite.bits() + 1;
                codeBlock->appendExceptionHandler(handlerToRegister);
            });

        // We set these to indicate to the stub to remove itself from the CodeBlock's
        // exception handler table when it is deallocated.
        codeBlockThatOwnsExceptionHandlers = codeBlock;
        ASSERT(JITCode::isOptimizingJIT(codeBlockThatOwnsExceptionHandlers->jitType()));
        callSiteIndexForExceptionHandling = state.callSiteIndexForExceptionHandling();
    }

    LinkBuffer linkBuffer(vm, jit, codeBlock, JITCompilationCanFail);
    if (linkBuffer.didFailToAllocate()) {
        if (verbose)
            dataLog("Did fail to allocate.\n");
        return AccessGenerationResult::GaveUp;
    }

    CodeLocationLabel successLabel = stubInfo.doneLocation();
        
    linkBuffer.link(state.success, successLabel);

    linkBuffer.link(failure, stubInfo.slowPathStartLocation());
    
    if (verbose)
        dataLog(*codeBlock, " ", stubInfo.codeOrigin, ": Generating polymorphic access stub for ", listDump(cases), "\n");

    MacroAssemblerCodeRef code = FINALIZE_CODE_FOR(
        codeBlock, linkBuffer,
        ("%s", toCString("Access stub for ", *codeBlock, " ", stubInfo.codeOrigin, " with return point ", successLabel, ": ", listDump(cases)).data()));

    bool doesCalls = false;
    Vector<JSCell*> cellsToMark;
    for (auto& entry : cases)
        doesCalls |= entry->doesCalls(&cellsToMark);
    
    m_stubRoutine = createJITStubRoutine(code, vm, codeBlock, doesCalls, cellsToMark, codeBlockThatOwnsExceptionHandlers, callSiteIndexForExceptionHandling);
    m_watchpoints = WTFMove(state.watchpoints);
    if (!state.weakReferences.isEmpty())
        m_weakReferences = std::make_unique<Vector<WriteBarrier<JSCell>>>(WTFMove(state.weakReferences));
    if (verbose)
        dataLog("Returning: ", code.code(), "\n");
    
    m_list = WTFMove(cases);
    
    AccessGenerationResult::Kind resultKind;
    if (m_list.size() >= Options::maxAccessVariantListSize())
        resultKind = AccessGenerationResult::GeneratedFinalCode;
    else
        resultKind = AccessGenerationResult::GeneratedNewCode;
    
    return AccessGenerationResult(resultKind, code.code());
}

void PolymorphicAccess::aboutToDie()
{
    if (m_stubRoutine)
        m_stubRoutine->aboutToDie();
}

} // namespace JSC

namespace WTF {

using namespace JSC;

void printInternal(PrintStream& out, AccessGenerationResult::Kind kind)
{
    switch (kind) {
    case AccessGenerationResult::MadeNoChanges:
        out.print("MadeNoChanges");
        return;
    case AccessGenerationResult::GaveUp:
        out.print("GaveUp");
        return;
    case AccessGenerationResult::Buffered:
        out.print("Buffered");
        return;
    case AccessGenerationResult::GeneratedNewCode:
        out.print("GeneratedNewCode");
        return;
    case AccessGenerationResult::GeneratedFinalCode:
        out.print("GeneratedFinalCode");
        return;
    }
    
    RELEASE_ASSERT_NOT_REACHED();
}

void printInternal(PrintStream& out, AccessCase::AccessType type)
{
    switch (type) {
    case AccessCase::Load:
        out.print("Load");
        return;
    case AccessCase::MegamorphicLoad:
        out.print("MegamorphicLoad");
        return;
    case AccessCase::Transition:
        out.print("Transition");
        return;
    case AccessCase::Replace:
        out.print("Replace");
        return;
    case AccessCase::Miss:
        out.print("Miss");
        return;
    case AccessCase::GetGetter:
        out.print("GetGetter");
        return;
    case AccessCase::Getter:
        out.print("Getter");
        return;
    case AccessCase::Setter:
        out.print("Setter");
        return;
    case AccessCase::CustomValueGetter:
        out.print("CustomValueGetter");
        return;
    case AccessCase::CustomAccessorGetter:
        out.print("CustomAccessorGetter");
        return;
    case AccessCase::CustomValueSetter:
        out.print("CustomValueSetter");
        return;
    case AccessCase::CustomAccessorSetter:
        out.print("CustomAccessorSetter");
        return;
    case AccessCase::IntrinsicGetter:
        out.print("IntrinsicGetter");
        return;
    case AccessCase::InHit:
        out.print("InHit");
        return;
    case AccessCase::InMiss:
        out.print("InMiss");
        return;
    case AccessCase::ArrayLength:
        out.print("ArrayLength");
        return;
    case AccessCase::StringLength:
        out.print("StringLength");
        return;
    case AccessCase::DirectArgumentsLength:
        out.print("DirectArgumentsLength");
        return;
    case AccessCase::ScopedArgumentsLength:
        out.print("ScopedArgumentsLength");
        return;
    }

    RELEASE_ASSERT_NOT_REACHED();
}

void printInternal(PrintStream& out, AccessCase::State state)
{
    switch (state) {
    case AccessCase::Primordial:
        out.print("Primordial");
        return;
    case AccessCase::Committed:
        out.print("Committed");
        return;
    case AccessCase::Generated:
        out.print("Generated");
        return;
    }

    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace WTF

#endif // ENABLE(JIT)


