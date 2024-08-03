/*
 * Copyright (C) 2009, 2010, 2013-2016 Apple Inc. All rights reserved.
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

#ifndef Executable_h
#define Executable_h

#include "ArityCheckMode.h"
#include "CallData.h"
#include "CodeBlockHash.h"
#include "CodeSpecializationKind.h"
#include "CompilationResult.h"
#include "ExecutableInfo.h"
#include "HandlerInfo.h"
#include "InferredValue.h"
#include "JITCode.h"
#include "JSGlobalObject.h"
#include "SourceCode.h"
#include "TypeSet.h"
#include "UnlinkedCodeBlock.h"
#include "UnlinkedFunctionExecutable.h"

namespace JSC {

class CodeBlock;
class EvalCodeBlock;
class FunctionCodeBlock;
class JSScope;
class JSWASMModule;
class LLIntOffsetsExtractor;
class ProgramCodeBlock;
class ModuleProgramCodeBlock;
class JSScope;
class WebAssemblyCodeBlock;
class ModuleProgramCodeBlock;
class JSModuleRecord;
class JSScope;

enum CompilationKind { FirstCompilation, OptimizingCompilation };

inline bool isCall(CodeSpecializationKind kind)
{
    if (kind == CodeForCall)
        return true;
    ASSERT(kind == CodeForConstruct);
    return false;
}

class ExecutableBase : public JSCell {
    friend class JIT;

protected:
    static const int NUM_PARAMETERS_IS_HOST = 0;
    static const int NUM_PARAMETERS_NOT_COMPILED = -1;

    ExecutableBase(VM& vm, Structure* structure, int numParameters, Intrinsic intrinsic)
        : JSCell(vm, structure)
        , m_numParametersForCall(numParameters)
        , m_numParametersForConstruct(numParameters)
        , m_intrinsic(intrinsic)
    {
    }

    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
    }

public:
    typedef JSCell Base;
    static const unsigned StructureFlags = Base::StructureFlags;

    static const bool needsDestruction = true;
    static void destroy(JSCell*);
        
    CodeBlockHash hashFor(CodeSpecializationKind) const;

    bool isEvalExecutable() const
    {
        return type() == EvalExecutableType;
    }
    bool isFunctionExecutable() const
    {
        return type() == FunctionExecutableType;
    }
    bool isProgramExecutable() const
    {
        return type() == ProgramExecutableType;
    }
    bool isModuleProgramExecutable()
    {
        return type() == ModuleProgramExecutableType;
    }


    bool isHostFunction() const
    {
        ASSERT((m_numParametersForCall == NUM_PARAMETERS_IS_HOST) == (m_numParametersForConstruct == NUM_PARAMETERS_IS_HOST));
        return m_numParametersForCall == NUM_PARAMETERS_IS_HOST;
    }

#if ENABLE(WEBASSEMBLY)
    bool isWebAssemblyExecutable() const
    {
        return type() == WebAssemblyExecutableType;
    }
#endif

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto) { return Structure::create(vm, globalObject, proto, TypeInfo(CellType, StructureFlags), info()); }
        
    void clearCode();

    DECLARE_EXPORT_INFO;

protected:
    int m_numParametersForCall;
    int m_numParametersForConstruct;

public:
    PassRefPtr<JITCode> generatedJITCodeForCall()
    {
        ASSERT(m_jitCodeForCall);
        return m_jitCodeForCall;
    }

    PassRefPtr<JITCode> generatedJITCodeForConstruct()
    {
        ASSERT(m_jitCodeForConstruct);
        return m_jitCodeForConstruct;
    }
        
    PassRefPtr<JITCode> generatedJITCodeFor(CodeSpecializationKind kind)
    {
        if (kind == CodeForCall)
            return generatedJITCodeForCall();
        ASSERT(kind == CodeForConstruct);
        return generatedJITCodeForConstruct();
    }
    
    MacroAssemblerCodePtr entrypointFor(CodeSpecializationKind kind, ArityCheckMode arity)
    {
        // Check if we have a cached result. We only have it for arity check because we use the
        // no-arity entrypoint in non-virtual calls, which will "cache" this value directly in
        // machine code.
        if (arity == MustCheckArity) {
            switch (kind) {
            case CodeForCall:
                if (MacroAssemblerCodePtr result = m_jitCodeForCallWithArityCheck)
                    return result;
                break;
            case CodeForConstruct:
                if (MacroAssemblerCodePtr result = m_jitCodeForConstructWithArityCheck)
                    return result;
                break;
            }
        }
        MacroAssemblerCodePtr result =
            generatedJITCodeFor(kind)->addressForCall(arity);
        if (arity == MustCheckArity) {
            // Cache the result; this is necessary for the JIT's virtual call optimizations.
            switch (kind) {
            case CodeForCall:
                m_jitCodeForCallWithArityCheck = result;
                break;
            case CodeForConstruct:
                m_jitCodeForConstructWithArityCheck = result;
                break;
            }
        }
        return result;
    }

    static ptrdiff_t offsetOfJITCodeWithArityCheckFor(
        CodeSpecializationKind kind)
    {
        switch (kind) {
        case CodeForCall:
            return OBJECT_OFFSETOF(ExecutableBase, m_jitCodeForCallWithArityCheck);
        case CodeForConstruct:
            return OBJECT_OFFSETOF(ExecutableBase, m_jitCodeForConstructWithArityCheck);
        }
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
    
    static ptrdiff_t offsetOfNumParametersFor(CodeSpecializationKind kind)
    {
        if (kind == CodeForCall)
            return OBJECT_OFFSETOF(ExecutableBase, m_numParametersForCall);
        ASSERT(kind == CodeForConstruct);
        return OBJECT_OFFSETOF(ExecutableBase, m_numParametersForConstruct);
    }

    bool hasJITCodeForCall() const
    {
        return m_numParametersForCall >= 0;
    }
        
    bool hasJITCodeForConstruct() const
    {
        return m_numParametersForConstruct >= 0;
    }
        
    bool hasJITCodeFor(CodeSpecializationKind kind) const
    {
        if (kind == CodeForCall)
            return hasJITCodeForCall();
        ASSERT(kind == CodeForConstruct);
        return hasJITCodeForConstruct();
    }

    // Intrinsics are only for calls, currently.
    Intrinsic intrinsic() const { return m_intrinsic; }
        
    Intrinsic intrinsicFor(CodeSpecializationKind kind) const
    {
        if (isCall(kind))
            return intrinsic();
        return NoIntrinsic;
    }
    
    void dump(PrintStream&) const;
        
protected:
    Intrinsic m_intrinsic;
    RefPtr<JITCode> m_jitCodeForCall;
    RefPtr<JITCode> m_jitCodeForConstruct;
    MacroAssemblerCodePtr m_jitCodeForCallWithArityCheck;
    MacroAssemblerCodePtr m_jitCodeForConstructWithArityCheck;
};

class NativeExecutable final : public ExecutableBase {
    friend class JIT;
    friend class LLIntOffsetsExtractor;
public:
    typedef ExecutableBase Base;
    static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

    static NativeExecutable* create(VM& vm, PassRefPtr<JITCode> callThunk, NativeFunction function, PassRefPtr<JITCode> constructThunk, NativeFunction constructor, Intrinsic intrinsic, const String& name);

    static void destroy(JSCell*);

    CodeBlockHash hashFor(CodeSpecializationKind) const;

    NativeFunction function() { return m_function; }
    NativeFunction constructor() { return m_constructor; }
        
    NativeFunction nativeFunctionFor(CodeSpecializationKind kind)
    {
        if (kind == CodeForCall)
            return function();
        ASSERT(kind == CodeForConstruct);
        return constructor();
    }
        
    static ptrdiff_t offsetOfNativeFunctionFor(CodeSpecializationKind kind)
    {
        if (kind == CodeForCall)
            return OBJECT_OFFSETOF(NativeExecutable, m_function);
        ASSERT(kind == CodeForConstruct);
        return OBJECT_OFFSETOF(NativeExecutable, m_constructor);
    }

    static Structure* createStructure(VM&, JSGlobalObject*, JSValue proto);
        
    DECLARE_INFO;

    const String& name() const { return m_name; }

protected:
    void finishCreation(VM&, PassRefPtr<JITCode> callThunk, PassRefPtr<JITCode> constructThunk, const String& name);

private:
    friend class ExecutableBase;

    NativeExecutable(VM&, NativeFunction function, NativeFunction constructor, Intrinsic);

    NativeFunction m_function;
    NativeFunction m_constructor;

    String m_name;
};

class ScriptExecutable : public ExecutableBase {
public:
    typedef ExecutableBase Base;
    static const unsigned StructureFlags = Base::StructureFlags;

    static void destroy(JSCell*);
        
    CodeBlockHash hashFor(CodeSpecializationKind) const;

    const SourceCode& source() const { return m_source; }
    intptr_t sourceID() const { return m_source.providerID(); }
    const String& sourceURL() const { return m_source.provider()->url(); }
    int firstLine() const { return m_firstLine; }
    void setOverrideLineNumber(int overrideLineNumber) { m_overrideLineNumber = overrideLineNumber; }
    bool hasOverrideLineNumber() const { return m_overrideLineNumber != -1; }
    int overrideLineNumber() const { return m_overrideLineNumber; }
    int lastLine() const { return m_lastLine; }
    unsigned startColumn() const { return m_startColumn; }
    unsigned endColumn() const { return m_endColumn; }
    unsigned typeProfilingStartOffset() const { return m_typeProfilingStartOffset; }
    unsigned typeProfilingEndOffset() const { return m_typeProfilingEndOffset; }

    bool usesEval() const { return m_features & EvalFeature; }
    bool usesArguments() const { return m_features & ArgumentsFeature; }
    bool isArrowFunctionContext() const { return m_isArrowFunctionContext; }
    bool isStrictMode() const { return m_features & StrictModeFeature; }
    DerivedContextType derivedContextType() const { return static_cast<DerivedContextType>(m_derivedContextType); }
    EvalContextType evalContextType() const { return static_cast<EvalContextType>(m_evalContextType); }

    ECMAMode ecmaMode() const { return isStrictMode() ? StrictMode : NotStrictMode; }
        
    void setNeverInline(bool value) { m_neverInline = value; }
    void setNeverOptimize(bool value) { m_neverOptimize = value; }
    void setNeverFTLOptimize(bool value) { m_neverFTLOptimize = value; }
    void setDidTryToEnterInLoop(bool value) { m_didTryToEnterInLoop = value; }
    void setCanUseOSRExitFuzzing(bool value) { m_canUseOSRExitFuzzing = value; }
    bool neverInline() const { return m_neverInline; }
    bool neverOptimize() const { return m_neverOptimize; }
    bool neverFTLOptimize() const { return m_neverFTLOptimize; }
    bool didTryToEnterInLoop() const { return m_didTryToEnterInLoop; }
    bool isInliningCandidate() const { return !neverInline(); }
    bool isOkToOptimize() const { return !neverOptimize(); }
    bool canUseOSRExitFuzzing() const { return m_canUseOSRExitFuzzing; }
    
    bool* addressOfDidTryToEnterInLoop() { return &m_didTryToEnterInLoop; }

    CodeFeatures features() const { return m_features; }
        
    DECLARE_EXPORT_INFO;

    void recordParse(CodeFeatures features, bool hasCapturedVariables, int firstLine, int lastLine, unsigned startColumn, unsigned endColumn)
    {
        m_features = features;
        m_hasCapturedVariables = hasCapturedVariables;
        m_firstLine = firstLine;
        m_lastLine = lastLine;
        ASSERT(startColumn != UINT_MAX);
        m_startColumn = startColumn;
        ASSERT(endColumn != UINT_MAX);
        m_endColumn = endColumn;
    }

    void installCode(CodeBlock*);
    void installCode(VM&, CodeBlock*, CodeType, CodeSpecializationKind);
    CodeBlock* newCodeBlockFor(CodeSpecializationKind, JSFunction*, JSScope*, JSObject*& exception);
    CodeBlock* newReplacementCodeBlockFor(CodeSpecializationKind);

    // This function has an interesting GC story. Callers of this function are asking us to create a CodeBlock
    // that is not jettisoned before this function returns. Callers are essentially asking for a strong reference
    // to the CodeBlock. Because the Executable may be allocating the CodeBlock, we require callers to pass in
    // their CodeBlock*& reference because it's safe for CodeBlock to be jettisoned if Executable is the only thing
    // to point to it. This forces callers to have a CodeBlock* in a register or on the stack that will be marked
    // by conservative GC if a GC happens after we create the CodeBlock.
    template <typename ExecutableType>
    JSObject* prepareForExecution(ExecState*, JSFunction*, JSScope*, CodeSpecializationKind, CodeBlock*& resultCodeBlock);

    template <typename Functor> void forEachCodeBlock(Functor&&);

private:
    friend class ExecutableBase;
    JSObject* prepareForExecutionImpl(ExecState*, JSFunction*, JSScope*, CodeSpecializationKind, CodeBlock*&);

protected:
    ScriptExecutable(Structure*, VM&, const SourceCode&, bool isInStrictContext, DerivedContextType, bool isInArrowFunctionContext, EvalContextType, Intrinsic);

    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        vm.heap.addExecutable(this); // Balanced by Heap::deleteUnmarkedCompiledCode().

#if ENABLE(CODEBLOCK_SAMPLING)
        if (SamplingTool* sampler = vm.interpreter->sampler())
            sampler->notifyOfScope(vm, this);
#endif
    }

    CodeFeatures m_features;
    bool m_didTryToEnterInLoop;
    bool m_hasCapturedVariables : 1;
    bool m_neverInline : 1;
    bool m_neverOptimize : 1;
    bool m_neverFTLOptimize : 1;
    bool m_isArrowFunctionContext : 1;
    bool m_canUseOSRExitFuzzing : 1;
    unsigned m_derivedContextType : 2; // DerivedContextType
    unsigned m_evalContextType : 2; // EvalContextType

    int m_overrideLineNumber;
    int m_firstLine;
    int m_lastLine;
    unsigned m_startColumn;
    unsigned m_endColumn;
    unsigned m_typeProfilingStartOffset;
    unsigned m_typeProfilingEndOffset;
    SourceCode m_source;
};

class EvalExecutable final : public ScriptExecutable {
    friend class LLIntOffsetsExtractor;
public:
    typedef ScriptExecutable Base;
    static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

    static void destroy(JSCell*);

    EvalCodeBlock* codeBlock()
    {
        return m_evalCodeBlock.get();
    }

    static EvalExecutable* create(ExecState*, const SourceCode&, bool isInStrictContext, DerivedContextType, bool isArrowFunctionContext, EvalContextType, const VariableEnvironment*);

    PassRefPtr<JITCode> generatedJITCode()
    {
        return generatedJITCodeForCall();
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
    {
        return Structure::create(vm, globalObject, proto, TypeInfo(EvalExecutableType, StructureFlags), info());
    }
        
    DECLARE_INFO;

    ExecutableInfo executableInfo() const { return ExecutableInfo(usesEval(), isStrictMode(), false, false, ConstructorKind::None, JSParserCommentMode::Classic, SuperBinding::NotNeeded, SourceParseMode::ProgramMode, derivedContextType(), isArrowFunctionContext(), false, evalContextType()); }

    unsigned numVariables() { return m_unlinkedEvalCodeBlock->numVariables(); }
    unsigned numberOfFunctionDecls() { return m_unlinkedEvalCodeBlock->numberOfFunctionDecls(); }

private:
    friend class ExecutableBase;
    friend class ScriptExecutable;

    EvalExecutable(ExecState*, const SourceCode&, bool inStrictContext, DerivedContextType, bool isArrowFunctionContext, EvalContextType);

    static void visitChildren(JSCell*, SlotVisitor&);

    WriteBarrier<EvalCodeBlock> m_evalCodeBlock;
    WriteBarrier<UnlinkedEvalCodeBlock> m_unlinkedEvalCodeBlock;
};

class ProgramExecutable final : public ScriptExecutable {
    friend class LLIntOffsetsExtractor;
public:
    typedef ScriptExecutable Base;
    static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

    static ProgramExecutable* create(ExecState* exec, const SourceCode& source)
    {
        ProgramExecutable* executable = new (NotNull, allocateCell<ProgramExecutable>(*exec->heap())) ProgramExecutable(exec, source);
        executable->finishCreation(exec->vm());
        return executable;
    }


    JSObject* initializeGlobalProperties(VM&, CallFrame*, JSScope*);

    static void destroy(JSCell*);

    ProgramCodeBlock* codeBlock()
    {
        return m_programCodeBlock.get();
    }

    JSObject* checkSyntax(ExecState*);

    PassRefPtr<JITCode> generatedJITCode()
    {
        return generatedJITCodeForCall();
    }
        
    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
    {
        return Structure::create(vm, globalObject, proto, TypeInfo(ProgramExecutableType, StructureFlags), info());
    }
        
    DECLARE_INFO;

    ExecutableInfo executableInfo() const { return ExecutableInfo(usesEval(), isStrictMode(), false, false, ConstructorKind::None, JSParserCommentMode::Classic, SuperBinding::NotNeeded, SourceParseMode::ProgramMode, derivedContextType(), isArrowFunctionContext(), false, EvalContextType::None); }

private:
    friend class ExecutableBase;
    friend class ScriptExecutable;

    ProgramExecutable(ExecState*, const SourceCode&);

    static void visitChildren(JSCell*, SlotVisitor&);

    WriteBarrier<UnlinkedProgramCodeBlock> m_unlinkedProgramCodeBlock;
    WriteBarrier<ProgramCodeBlock> m_programCodeBlock;
};

class ModuleProgramExecutable final : public ScriptExecutable {
    friend class LLIntOffsetsExtractor;
public:
    typedef ScriptExecutable Base;
    static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

    static ModuleProgramExecutable* create(ExecState*, const SourceCode&);

    static void destroy(JSCell*);

    ModuleProgramCodeBlock* codeBlock()
    {
        return m_moduleProgramCodeBlock.get();
    }

    PassRefPtr<JITCode> generatedJITCode()
    {
        return generatedJITCodeForCall();
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
    {
        return Structure::create(vm, globalObject, proto, TypeInfo(ModuleProgramExecutableType, StructureFlags), info());
    }

    DECLARE_INFO;

    ExecutableInfo executableInfo() const { return ExecutableInfo(usesEval(), isStrictMode(), false, false, ConstructorKind::None, JSParserCommentMode::Module, SuperBinding::NotNeeded, SourceParseMode::ModuleEvaluateMode, derivedContextType(), isArrowFunctionContext(), false, EvalContextType::None); }

    UnlinkedModuleProgramCodeBlock* unlinkedModuleProgramCodeBlock() { return m_unlinkedModuleProgramCodeBlock.get(); }

    SymbolTable* moduleEnvironmentSymbolTable() { return m_moduleEnvironmentSymbolTable.get(); }

private:
    friend class ExecutableBase;
    friend class ScriptExecutable;

    ModuleProgramExecutable(ExecState*, const SourceCode&);

    static void visitChildren(JSCell*, SlotVisitor&);

    WriteBarrier<UnlinkedModuleProgramCodeBlock> m_unlinkedModuleProgramCodeBlock;
    WriteBarrier<SymbolTable> m_moduleEnvironmentSymbolTable;
    WriteBarrier<ModuleProgramCodeBlock> m_moduleProgramCodeBlock;
};

class FunctionExecutable final : public ScriptExecutable {
    friend class JIT;
    friend class LLIntOffsetsExtractor;
public:
    typedef ScriptExecutable Base;
    static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

    static FunctionExecutable* create(
        VM& vm, const SourceCode& source, UnlinkedFunctionExecutable* unlinkedExecutable, 
        unsigned firstLine, unsigned lastLine, unsigned startColumn, unsigned endColumn, Intrinsic intrinsic)
    {
        FunctionExecutable* executable = new (NotNull, allocateCell<FunctionExecutable>(vm.heap)) FunctionExecutable(vm, source, unlinkedExecutable, firstLine, lastLine, startColumn, endColumn, intrinsic);
        executable->finishCreation(vm);
        return executable;
    }
    static FunctionExecutable* fromGlobalCode(
        const Identifier& name, ExecState&, const SourceCode&, 
        JSObject*& exception, int overrideLineNumber);

    static void destroy(JSCell*);
        
    UnlinkedFunctionExecutable* unlinkedExecutable() const
    {
        return m_unlinkedExecutable.get();
    }

    // Returns either call or construct bytecode. This can be appropriate
    // for answering questions that that don't vary between call and construct --
    // for example, argumentsRegister().
    FunctionCodeBlock* eitherCodeBlock()
    {
        if (m_codeBlockForCall)
            return m_codeBlockForCall.get();
        return m_codeBlockForConstruct.get();
    }
        
    bool isGeneratedForCall() const
    {
        return !!m_codeBlockForCall;
    }

    FunctionCodeBlock* codeBlockForCall()
    {
        return m_codeBlockForCall.get();
    }

    bool isGeneratedForConstruct() const
    {
        return m_codeBlockForConstruct.get();
    }

    FunctionCodeBlock* codeBlockForConstruct()
    {
        return m_codeBlockForConstruct.get();
    }
        
    bool isGeneratedFor(CodeSpecializationKind kind)
    {
        if (kind == CodeForCall)
            return isGeneratedForCall();
        ASSERT(kind == CodeForConstruct);
        return isGeneratedForConstruct();
    }
        
    FunctionCodeBlock* codeBlockFor(CodeSpecializationKind kind)
    {
        if (kind == CodeForCall)
            return codeBlockForCall();
        ASSERT(kind == CodeForConstruct);
        return codeBlockForConstruct();
    }

    FunctionCodeBlock* baselineCodeBlockFor(CodeSpecializationKind);
        
    FunctionCodeBlock* profiledCodeBlockFor(CodeSpecializationKind kind)
    {
        return baselineCodeBlockFor(kind);
    }

    RefPtr<TypeSet> returnStatementTypeSet() 
    {
        if (!m_returnStatementTypeSet)
            m_returnStatementTypeSet = TypeSet::create();

        return m_returnStatementTypeSet;
    }
        
    FunctionMode functionMode() { return m_unlinkedExecutable->functionMode(); }
    bool isBuiltinFunction() const { return m_unlinkedExecutable->isBuiltinFunction(); }
    ConstructAbility constructAbility() const { return m_unlinkedExecutable->constructAbility(); }
    bool isClass() const { return !classSource().isNull(); }
    bool isArrowFunction() const { return parseMode() == SourceParseMode::ArrowFunctionMode; }
    bool isGetter() const { return parseMode() == SourceParseMode::GetterMode; }
    bool isSetter() const { return parseMode() == SourceParseMode::SetterMode; }
    DerivedContextType derivedContextType() const { return m_unlinkedExecutable->derivedContextType(); }
    bool isClassConstructorFunction() const { return m_unlinkedExecutable->isClassConstructorFunction(); }
    const Identifier& name() { return m_unlinkedExecutable->name(); }
    const Identifier& ecmaName() { return m_unlinkedExecutable->ecmaName(); }
    const Identifier& inferredName() { return m_unlinkedExecutable->inferredName(); }
    size_t parameterCount() const { return m_unlinkedExecutable->parameterCount(); } // Excluding 'this'!
    SourceParseMode parseMode() const { return m_unlinkedExecutable->parseMode(); }
    JSParserCommentMode commentMode() const { return m_unlinkedExecutable->commentMode(); }
    const SourceCode& classSource() const { return m_unlinkedExecutable->classSource(); }

    static void visitChildren(JSCell*, SlotVisitor&);
    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
    {
        return Structure::create(vm, globalObject, proto, TypeInfo(FunctionExecutableType, StructureFlags), info());
    }

    unsigned parametersStartOffset() const { return m_parametersStartOffset; }

    void overrideParameterAndTypeProfilingStartEndOffsets(unsigned parametersStartOffset, unsigned typeProfilingStartOffset, unsigned typeProfilingEndOffset)
    {
        m_parametersStartOffset = parametersStartOffset;
        m_typeProfilingStartOffset = typeProfilingStartOffset;
        m_typeProfilingEndOffset = typeProfilingEndOffset;
    }

    DECLARE_INFO;

    InferredValue* singletonFunction() { return m_singletonFunction.get(); }

private:
    friend class ExecutableBase;
    FunctionExecutable(
        VM&, const SourceCode&, UnlinkedFunctionExecutable*, unsigned firstLine, 
        unsigned lastLine, unsigned startColumn, unsigned endColumn, Intrinsic);
    
    void finishCreation(VM&);

    friend class ScriptExecutable;
    
    unsigned m_parametersStartOffset;
    WriteBarrier<UnlinkedFunctionExecutable> m_unlinkedExecutable;
    WriteBarrier<FunctionCodeBlock> m_codeBlockForCall;
    WriteBarrier<FunctionCodeBlock> m_codeBlockForConstruct;
    RefPtr<TypeSet> m_returnStatementTypeSet;
    WriteBarrier<InferredValue> m_singletonFunction;
};

#if ENABLE(WEBASSEMBLY)
class WebAssemblyExecutable final : public ExecutableBase {
public:
    typedef ExecutableBase Base;
    static const unsigned StructureFlags = Base::StructureFlags | StructureIsImmortal;

    static WebAssemblyExecutable* create(VM& vm, const SourceCode& source, JSWASMModule* module, unsigned functionIndex)
    {
        WebAssemblyExecutable* executable = new (NotNull, allocateCell<WebAssemblyExecutable>(vm.heap)) WebAssemblyExecutable(vm, source, module, functionIndex);
        executable->finishCreation(vm);
        return executable;
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
    {
        return Structure::create(vm, globalObject, proto, TypeInfo(WebAssemblyExecutableType, StructureFlags), info());
    }

    static void destroy(JSCell*);

    DECLARE_INFO;

    void prepareForExecution(ExecState*);

    WebAssemblyCodeBlock* codeBlockForCall()
    {
        return m_codeBlockForCall.get();
    }

private:
    friend class ExecutableBase;
    WebAssemblyExecutable(VM&, const SourceCode&, JSWASMModule*, unsigned functionIndex);

    static void visitChildren(JSCell*, SlotVisitor&);

    SourceCode m_source;
    WriteBarrier<JSWASMModule> m_module;
    unsigned m_functionIndex;

    WriteBarrier<WebAssemblyCodeBlock> m_codeBlockForCall;
};
#endif

template <typename ExecutableType>
JSObject* ScriptExecutable::prepareForExecution(ExecState* exec, JSFunction* function, JSScope* scope, CodeSpecializationKind kind, CodeBlock*& resultCodeBlock)
{
    if (hasJITCodeFor(kind)) {
        if (std::is_same<ExecutableType, EvalExecutable>::value)
            resultCodeBlock = jsCast<CodeBlock*>(jsCast<EvalExecutable*>(this)->codeBlock());
        else if (std::is_same<ExecutableType, ProgramExecutable>::value)
            resultCodeBlock = jsCast<CodeBlock*>(jsCast<ProgramExecutable*>(this)->codeBlock());
        else if (std::is_same<ExecutableType, ModuleProgramExecutable>::value)
            resultCodeBlock = jsCast<CodeBlock*>(jsCast<ModuleProgramExecutable*>(this)->codeBlock());
        else if (std::is_same<ExecutableType, FunctionExecutable>::value)
            resultCodeBlock = jsCast<CodeBlock*>(jsCast<FunctionExecutable*>(this)->codeBlockFor(kind));
        else
            RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }
    return prepareForExecutionImpl(exec, function, scope, kind, resultCodeBlock);
}

} // namespace JSC

#endif // Executable_h
