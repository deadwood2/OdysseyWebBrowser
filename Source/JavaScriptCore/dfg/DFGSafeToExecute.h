/*
 * Copyright (C) 2013-2018 Apple Inc. All rights reserved.
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

#if ENABLE(DFG_JIT)

#include "DFGGraph.h"

namespace JSC { namespace DFG {

template<typename AbstractStateType>
class SafeToExecuteEdge {
public:
    SafeToExecuteEdge(AbstractStateType& state)
        : m_state(state)
    {
    }
    
    void operator()(Node*, Edge edge)
    {
        m_maySeeEmptyChild |= !!(m_state.forNode(edge).m_type & SpecEmpty);

        switch (edge.useKind()) {
        case UntypedUse:
        case Int32Use:
        case DoubleRepUse:
        case DoubleRepRealUse:
        case Int52RepUse:
        case NumberUse:
        case RealNumberUse:
        case BooleanUse:
        case CellUse:
        case CellOrOtherUse:
        case ObjectUse:
        case ArrayUse:
        case FunctionUse:
        case FinalObjectUse:
        case RegExpObjectUse:
        case ProxyObjectUse:
        case DerivedArrayUse:
        case MapObjectUse:
        case SetObjectUse:
        case WeakMapObjectUse:
        case WeakSetObjectUse:
        case DataViewObjectUse:
        case ObjectOrOtherUse:
        case StringIdentUse:
        case StringUse:
        case StringOrOtherUse:
        case SymbolUse:
        case BigIntUse:
        case StringObjectUse:
        case StringOrStringObjectUse:
        case NotStringVarUse:
        case NotSymbolUse:
        case NotCellUse:
        case OtherUse:
        case MiscUse:
        case AnyIntUse:
        case DoubleRepAnyIntUse:
            return;
            
        case KnownInt32Use:
            if (m_state.forNode(edge).m_type & ~SpecInt32Only)
                m_result = false;
            return;

        case KnownBooleanUse:
            if (m_state.forNode(edge).m_type & ~SpecBoolean)
                m_result = false;
            return;
            
        case KnownCellUse:
            if (m_state.forNode(edge).m_type & ~SpecCell)
                m_result = false;
            return;
            
        case KnownStringUse:
            if (m_state.forNode(edge).m_type & ~SpecString)
                m_result = false;
            return;

        case KnownPrimitiveUse:
            if (m_state.forNode(edge).m_type & ~(SpecHeapTop & ~SpecObject))
                m_result = false;
            return;

        case KnownOtherUse:
            if (m_state.forNode(edge).m_type & ~SpecOther)
                m_result = false;
            return;
            
        case LastUseKind:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    bool result() const { return m_result; }
    bool maySeeEmptyChild() const { return m_maySeeEmptyChild; }
private:
    AbstractStateType& m_state;
    bool m_result { true };
    bool m_maySeeEmptyChild { false };
};

// Determines if it's safe to execute a node within the given abstract state. This may
// return false conservatively. If it returns true, then you can hoist the given node
// up to the given point and expect that it will not crash. It also guarantees that the
// node will not produce a malformed JSValue or object pointer when executed in the
// given state. But this doesn't guarantee that the node will produce the result you
// wanted. For example, you may have a GetByOffset from a prototype that only makes
// semantic sense if you've also checked that some nearer prototype doesn't also have
// a property of the same name. This could still return true even if that check hadn't
// been performed in the given abstract state. That's fine though: the load can still
// safely execute before that check, so long as that check continues to guard any
// user-observable things done to the loaded value.
template<typename AbstractStateType>
bool safeToExecute(AbstractStateType& state, Graph& graph, Node* node, bool ignoreEmptyChildren = false)
{
    SafeToExecuteEdge<AbstractStateType> safeToExecuteEdge(state);
    DFG_NODE_DO_TO_CHILDREN(graph, node, safeToExecuteEdge);
    if (!safeToExecuteEdge.result())
        return false;

    if (!ignoreEmptyChildren && safeToExecuteEdge.maySeeEmptyChild()) {
        // We conservatively assume if the empty value flows into a node,
        // it might not be able to handle it (e.g, crash). In general, the bytecode generator
        // emits code in such a way that most node types don't need to worry about the empty value
        // because they will never see it. However, code motion has to consider the empty
        // value so it does not insert/move nodes to a place where they will crash. E.g, the
        // type check hoisting phase needs to insert CheckStructureOrEmpty instead of CheckStructure
        // for hoisted structure checks because it can not guarantee that a particular local is not
        // the empty value.
        switch (node->op()) {
        case CheckNotEmpty:
        case CheckStructureOrEmpty:
            break;
        default:
            return false;
        }
    }

    // NOTE: This tends to lie when it comes to effectful nodes, because it knows that they aren't going to
    // get hoisted anyway.

    switch (node->op()) {
    case JSConstant:
    case DoubleConstant:
    case Int52Constant:
    case LazyJSConstant:
    case Identity:
    case IdentityWithProfile:
    case ToThis:
    case CreateThis:
    case ObjectCreate:
    case ObjectKeys:
    case GetCallee:
    case SetCallee:
    case GetArgumentCountIncludingThis:
    case SetArgumentCountIncludingThis:
    case GetRestLength:
    case GetLocal:
    case SetLocal:
    case PutStack:
    case KillStack:
    case GetStack:
    case MovHint:
    case ZombieHint:
    case ExitOK:
    case Phantom:
    case Upsilon:
    case Phi:
    case Flush:
    case PhantomLocal:
    case SetArgument:
    case ArithBitNot:
    case ArithBitAnd:
    case ArithBitOr:
    case ArithBitXor:
    case BitLShift:
    case BitRShift:
    case BitURShift:
    case ValueToInt32:
    case UInt32ToNumber:
    case DoubleAsInt32:
    case ArithAdd:
    case ArithClz32:
    case ArithSub:
    case ArithNegate:
    case ArithMul:
    case ArithIMul:
    case ArithDiv:
    case ArithMod:
    case ArithAbs:
    case ArithMin:
    case ArithMax:
    case ArithPow:
    case ArithRandom:
    case ArithSqrt:
    case ArithFRound:
    case ArithRound:
    case ArithFloor:
    case ArithCeil:
    case ArithTrunc:
    case ArithUnary:
    case ValueBitAnd:
    case ValueBitXor:
    case ValueBitOr:
    case ValueNegate:
    case ValueAdd:
    case ValueSub:
    case ValueMul:
    case ValueDiv:
    case TryGetById:
    case DeleteById:
    case DeleteByVal:
    case GetById:
    case GetByIdWithThis:
    case GetByValWithThis:
    case GetByIdFlush:
    case GetByIdDirect:
    case GetByIdDirectFlush:
    case PutById:
    case PutByIdFlush:
    case PutByIdWithThis:
    case PutByValWithThis:
    case PutByIdDirect:
    case PutGetterById:
    case PutSetterById:
    case PutGetterSetterById:
    case PutGetterByVal:
    case PutSetterByVal:
    case DefineDataProperty:
    case DefineAccessorProperty:
    case CheckStructure:
    case CheckStructureOrEmpty:
    case GetExecutable:
    case GetButterfly:
    case CallDOMGetter:
    case CallDOM:
    case CheckSubClass:
    case CheckArray:
    case Arrayify:
    case ArrayifyToStructure:
    case GetScope:
    case SkipScope:
    case GetGlobalObject:
    case GetGlobalThis:
    case GetClosureVar:
    case PutClosureVar:
    case GetGlobalVar:
    case GetGlobalLexicalVariable:
    case PutGlobalVariable:
    case CheckCell:
    case CheckBadCell:
    case CheckNotEmpty:
    case AssertNotEmpty:
    case CheckStringIdent:
    case RegExpExec:
    case RegExpExecNonGlobalOrSticky:
    case RegExpTest:
    case RegExpMatchFast:
    case RegExpMatchFastGlobal:
    case CompareLess:
    case CompareLessEq:
    case CompareGreater:
    case CompareGreaterEq:
    case CompareBelow:
    case CompareBelowEq:
    case CompareEq:
    case CompareStrictEq:
    case CompareEqPtr:
    case SameValue:
    case Call:
    case DirectCall:
    case TailCallInlinedCaller:
    case DirectTailCallInlinedCaller:
    case Construct:
    case DirectConstruct:
    case CallVarargs:
    case CallEval:
    case TailCallVarargsInlinedCaller:
    case TailCallForwardVarargsInlinedCaller:
    case ConstructVarargs:
    case LoadVarargs:
    case CallForwardVarargs:
    case ConstructForwardVarargs:
    case NewObject:
    case NewArray:
    case NewArrayWithSize:
    case NewArrayBuffer:
    case NewArrayWithSpread:
    case Spread:
    case NewRegexp:
    case NewSymbol:
    case ProfileType:
    case ProfileControlFlow:
    case CheckTypeInfoFlags:
    case ParseInt:
    case OverridesHasInstance:
    case InstanceOf:
    case InstanceOfCustom:
    case IsEmpty:
    case IsUndefined:
    case IsUndefinedOrNull:
    case IsBoolean:
    case IsNumber:
    case NumberIsInteger:
    case IsObject:
    case IsObjectOrNull:
    case IsFunction:
    case IsCellWithType:
    case IsTypedArrayView:
    case TypeOf:
    case LogicalNot:
    case CallObjectConstructor:
    case ToPrimitive:
    case ToString:
    case ToNumber:
    case ToObject:
    case NumberToStringWithRadix:
    case NumberToStringWithValidRadixConstant:
    case SetFunctionName:
    case StrCat:
    case CallStringConstructor:
    case NewStringObject:
    case MakeRope:
    case InByVal:
    case InById:
    case HasOwnProperty:
    case PushWithScope:
    case CreateActivation:
    case CreateDirectArguments:
    case CreateScopedArguments:
    case CreateClonedArguments:
    case GetFromArguments:
    case GetArgument:
    case PutToArguments:
    case NewFunction:
    case NewGeneratorFunction:
    case NewAsyncGeneratorFunction:
    case NewAsyncFunction:
    case Jump:
    case Branch:
    case Switch:
    case EntrySwitch:
    case Return:
    case TailCall:
    case DirectTailCall:
    case TailCallVarargs:
    case TailCallForwardVarargs:
    case Throw:
    case ThrowStaticError:
    case CountExecution:
    case SuperSamplerBegin:
    case SuperSamplerEnd:
    case ForceOSRExit:
    case CPUIntrinsic:
    case CheckTraps:
    case LogShadowChickenPrologue:
    case LogShadowChickenTail:
    case StringFromCharCode:
    case NewTypedArray:
    case Unreachable:
    case ExtractOSREntryLocal:
    case ExtractCatchLocal:
    case ClearCatchLocals:
    case CheckTierUpInLoop:
    case CheckTierUpAtReturn:
    case CheckTierUpAndOSREnter:
    case LoopHint:
    case InvalidationPoint:
    case NotifyWrite:
    case CheckInBounds:
    case ConstantStoragePointer:
    case Check:
    case CheckVarargs:
    case MultiPutByOffset:
    case ValueRep:
    case DoubleRep:
    case Int52Rep:
    case BooleanToNumber:
    case FiatInt52:
    case GetGetter:
    case GetSetter:
    case GetEnumerableLength:
    case HasGenericProperty:
    case HasStructureProperty:
    case HasIndexedProperty:
    case GetDirectPname:
    case GetPropertyEnumerator:
    case GetEnumeratorStructurePname:
    case GetEnumeratorGenericPname:
    case ToIndexString:
    case PhantomNewObject:
    case PhantomNewFunction:
    case PhantomNewGeneratorFunction:
    case PhantomNewAsyncGeneratorFunction:
    case PhantomNewAsyncFunction:
    case PhantomCreateActivation:
    case PhantomNewRegexp:
    case PutHint:
    case CheckStructureImmediate:
    case MaterializeNewObject:
    case MaterializeCreateActivation:
    case PhantomDirectArguments:
    case PhantomCreateRest:
    case PhantomSpread:
    case PhantomNewArrayWithSpread:
    case PhantomNewArrayBuffer:
    case PhantomClonedArguments:
    case GetMyArgumentByVal:
    case GetMyArgumentByValOutOfBounds:
    case ForwardVarargs:
    case CreateRest:
    case GetPrototypeOf:
    case StringReplace:
    case StringReplaceRegExp:
    case GetRegExpObjectLastIndex:
    case SetRegExpObjectLastIndex:
    case RecordRegExpCachedResult:
    case GetDynamicVar:
    case PutDynamicVar:
    case ResolveScopeForHoistingFuncDeclInEval:
    case ResolveScope:
    case MapHash:
    case NormalizeMapKey:
    case StringValueOf:
    case StringSlice:
    case ToLowerCase:
    case GetMapBucket:
    case GetMapBucketHead:
    case GetMapBucketNext:
    case LoadKeyFromMapBucket:
    case LoadValueFromMapBucket:
    case ExtractValueFromWeakMapGet:
    case WeakMapGet:
    case WeakSetAdd:
    case WeakMapSet:
    case AtomicsAdd:
    case AtomicsAnd:
    case AtomicsCompareExchange:
    case AtomicsExchange:
    case AtomicsLoad:
    case AtomicsOr:
    case AtomicsStore:
    case AtomicsSub:
    case AtomicsXor:
    case AtomicsIsLockFree:
    case InitializeEntrypointArguments:
    case MatchStructure:
    case DataViewGetInt:
    case DataViewGetFloat:
        return true;

    case ArraySlice:
    case ArrayIndexOf: {
        // You could plausibly move this code around as long as you proved the
        // incoming array base structure is an original array at the hoisted location.
        // Instead of doing that extra work, we just conservatively return false.
        return false;
    }

    case BottomValue:
        // If in doubt, assume that this isn't safe to execute, just because we have no way of
        // compiling this node.
        return false;

    case StoreBarrier:
    case FencedStoreBarrier:
    case PutStructure:
    case NukeStructureAndSetButterfly:
        // We conservatively assume that these cannot be put anywhere, which forces the compiler to
        // keep them exactly where they were. This is sort of overkill since the clobberize effects
        // already force these things to be ordered precisely. I'm just not confident enough in my
        // effect based memory model to rely solely on that right now.
        return false;
        
    case FilterCallLinkStatus:
    case FilterGetByIdStatus:
    case FilterPutByIdStatus:
    case FilterInByIdStatus:
        // We don't want these to be moved anywhere other than where we put them, since we want them
        // to capture "profiling" at the point in control flow here the user put them.
        return false;

    case GetByVal:
    case GetIndexedPropertyStorage:
    case GetArrayLength:
    case GetVectorLength:
    case ArrayPop:
    case StringCharAt:
    case StringCharCodeAt:
        return node->arrayMode().alreadyChecked(graph, node, state.forNode(graph.child(node, 0)));

    case ArrayPush:
        return node->arrayMode().alreadyChecked(graph, node, state.forNode(graph.varArgChild(node, 1)));

    case GetTypedArrayByteOffset:
        return !(state.forNode(node->child1()).m_type & ~(SpecTypedArrayView));
            
    case PutByValDirect:
    case PutByVal:
    case PutByValAlias:
        return node->arrayMode().modeForPut().alreadyChecked(
            graph, node, state.forNode(graph.varArgChild(node, 0)));

    case AllocatePropertyStorage:
    case ReallocatePropertyStorage:
        return state.forNode(node->child1()).m_structure.isSubsetOf(
            RegisteredStructureSet(node->transition()->previous));
        
    case GetByOffset:
    case GetGetterSetterByOffset:
    case PutByOffset: {
        StorageAccessData& data = node->storageAccessData();
        PropertyOffset offset = data.offset;
        // Graph::isSafeToLoad() is all about proofs derived from PropertyConditions. Those don't
        // know anything about inferred types. But if we have a proof derived from watching a
        // structure that has a type proof, then the next case below will deal with it.
        if (state.structureClobberState() == StructuresAreWatched) {
            if (JSObject* knownBase = node->child2()->dynamicCastConstant<JSObject*>(graph.m_vm)) {
                if (graph.isSafeToLoad(knownBase, offset))
                    return true;
            }
        }
        
        StructureAbstractValue& value = state.forNode(node->child2()).m_structure;
        if (value.isInfinite())
            return false;
        for (unsigned i = value.size(); i--;) {
            Structure* thisStructure = value[i].get();
            if (!thisStructure->isValidOffset(offset))
                return false;
        }
        return true;
    }
        
    case MultiGetByOffset: {
        // We can't always guarantee that the MultiGetByOffset is safe to execute if it
        // contains loads from prototypes. If the load requires a check in IR, which is rare, then
        // we currently claim that we don't know if it's safe to execute because finding that
        // check in the abstract state would be hard. If the load requires watchpoints, we just
        // check if we're not in a clobbered state (i.e. in between a side effect and an
        // invalidation point).
        for (const MultiGetByOffsetCase& getCase : node->multiGetByOffsetData().cases) {
            GetByOffsetMethod method = getCase.method();
            switch (method.kind()) {
            case GetByOffsetMethod::Invalid:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            case GetByOffsetMethod::Constant: // OK because constants are always safe to execute.
            case GetByOffsetMethod::Load: // OK because the MultiGetByOffset has its own checks for loading from self.
                break;
            case GetByOffsetMethod::LoadFromPrototype:
                // Only OK if the state isn't clobbered. That's almost always the case.
                if (state.structureClobberState() != StructuresAreWatched)
                    return false;
                if (!graph.isSafeToLoad(method.prototype()->cast<JSObject*>(), method.offset()))
                    return false;
                break;
            }
        }
        return true;
    }

    case DataViewSet:
        return false;

    case SetAdd:
    case MapSet:
        return false;

    case LastNodeType:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
    
    RELEASE_ASSERT_NOT_REACHED();
    return false;
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
