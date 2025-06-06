/*
 * Copyright (C) 2009-2017 Apple Inc. All rights reserved.
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

#include "BatchedTransitionOptimizer.h"
#include "CodeBlock.h"
#include "CodeCache.h"
#include "Debugger.h"
#include "Exception.h"
#include "JIT.h"
#include "JSCInlines.h"
#include "LLIntEntrypoint.h"
#include "Parser.h"
#include "ProgramCodeBlock.h"
#include "TypeProfiler.h"
#include "VMInlines.h"
#include <wtf/CommaPrinter.h>

namespace JSC {

const ClassInfo ProgramExecutable::s_info = { "ProgramExecutable", &ScriptExecutable::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(ProgramExecutable) };

ProgramExecutable::ProgramExecutable(ExecState* exec, const SourceCode& source)
    : ScriptExecutable(exec->vm().programExecutableStructure.get(), exec->vm(), source, false, DerivedContextType::None, false, EvalContextType::None, NoIntrinsic)
{
    ASSERT(source.provider()->sourceType() == SourceProviderSourceType::Program);
    VM& vm = exec->vm();
    if (vm.typeProfiler() || vm.controlFlowProfiler())
        vm.functionHasExecutedCache()->insertUnexecutedRange(sourceID(), typeProfilingStartOffset(vm), typeProfilingEndOffset(vm));
}

void ProgramExecutable::destroy(JSCell* cell)
{
    static_cast<ProgramExecutable*>(cell)->ProgramExecutable::~ProgramExecutable();
}

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-hasrestrictedglobalproperty
enum class GlobalPropertyLookUpStatus {
    NotFound,
    Configurable,
    NonConfigurable,
};
static GlobalPropertyLookUpStatus hasRestrictedGlobalProperty(ExecState* exec, JSGlobalObject* globalObject, PropertyName propertyName)
{
    PropertyDescriptor descriptor;
    if (!globalObject->getOwnPropertyDescriptor(exec, propertyName, descriptor))
        return GlobalPropertyLookUpStatus::NotFound;
    if (descriptor.configurable())
        return GlobalPropertyLookUpStatus::Configurable;
    return GlobalPropertyLookUpStatus::NonConfigurable;
}

JSObject* ProgramExecutable::initializeGlobalProperties(VM& vm, CallFrame* callFrame, JSScope* scope)
{
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    RELEASE_ASSERT(scope);
    JSGlobalObject* globalObject = scope->globalObject(vm);
    RELEASE_ASSERT(globalObject);
    ASSERT(&globalObject->vm() == &vm);

    ParserError error;
    JSParserStrictMode strictMode = isStrictMode() ? JSParserStrictMode::Strict : JSParserStrictMode::NotStrict;
    DebuggerMode debuggerMode = globalObject->hasInteractiveDebugger() ? DebuggerOn : DebuggerOff;

    UnlinkedProgramCodeBlock* unlinkedCodeBlock = vm.codeCache()->getUnlinkedProgramCodeBlock(
        vm, this, source(), strictMode, debuggerMode, error);

    if (globalObject->hasDebugger())
        globalObject->debugger()->sourceParsed(callFrame, source().provider(), error.line(), error.message());

    if (error.isValid())
        return error.toErrorObject(globalObject, source());

    JSValue nextPrototype = globalObject->getPrototypeDirect(vm);
    while (nextPrototype && nextPrototype.isObject()) {
        if (UNLIKELY(asObject(nextPrototype)->type() == ProxyObjectType)) {
            ExecState* exec = globalObject->globalExec();
            return createTypeError(exec, "Proxy is not allowed in the global prototype chain."_s);
        }
        nextPrototype = asObject(nextPrototype)->getPrototypeDirect(vm);
    }
    
    JSGlobalLexicalEnvironment* globalLexicalEnvironment = globalObject->globalLexicalEnvironment();
    const VariableEnvironment& variableDeclarations = unlinkedCodeBlock->variableDeclarations();
    const VariableEnvironment& lexicalDeclarations = unlinkedCodeBlock->lexicalDeclarations();
    // The ES6 spec says that no vars/global properties/let/const can be duplicated in the global scope.
    // This carried out section 15.1.8 of the ES6 spec: http://www.ecma-international.org/ecma-262/6.0/index.html#sec-globaldeclarationinstantiation
    {
        ExecState* exec = globalObject->globalExec();
        // Check for intersection of "var" and "let"/"const"/"class"
        for (auto& entry : lexicalDeclarations) {
            if (variableDeclarations.contains(entry.key))
                return createSyntaxError(exec, makeString("Can't create duplicate variable: '", String(entry.key.get()), "'"));
        }

        // Check if any new "let"/"const"/"class" will shadow any pre-existing global property names (with configurable = false), or "var"/"let"/"const" variables.
        // It's an error to introduce a shadow.
        for (auto& entry : lexicalDeclarations) {
            // The ES6 spec says that RestrictedGlobalProperty can't be shadowed.
            GlobalPropertyLookUpStatus status = hasRestrictedGlobalProperty(exec, globalObject, entry.key.get());
            RETURN_IF_EXCEPTION(throwScope, throwScope.exception());
            switch (status) {
            case GlobalPropertyLookUpStatus::NonConfigurable:
                return createSyntaxError(exec, makeString("Can't create duplicate variable that shadows a global property: '", String(entry.key.get()), "'"));
            case GlobalPropertyLookUpStatus::Configurable:
                // Lexical bindings can shadow global properties if the given property's attribute is configurable.
                // https://tc39.github.io/ecma262/#sec-globaldeclarationinstantiation step 5-c, `hasRestrictedGlobal` becomes false
                // However we may emit GlobalProperty look up in bytecodes already and it may cache the value for the global scope.
                // To make it invalid,
                // 1. In LLInt and Baseline, we bump the global lexical binding epoch and it works.
                // 3. In DFG and FTL, we watch the watchpoint and jettison once it is fired.
                break;
            case GlobalPropertyLookUpStatus::NotFound:
                break;
            }

            bool hasProperty = globalLexicalEnvironment->hasProperty(exec, entry.key.get());
            RETURN_IF_EXCEPTION(throwScope, throwScope.exception());
            if (hasProperty) {
                if (UNLIKELY(entry.value.isConst() && !vm.globalConstRedeclarationShouldThrow() && !isStrictMode())) {
                    // We only allow "const" duplicate declarations under this setting.
                    // For example, we don't "let" variables to be overridden by "const" variables.
                    if (globalLexicalEnvironment->isConstVariable(entry.key.get()))
                        continue;
                }
                return createSyntaxError(exec, makeString("Can't create duplicate variable: '", String(entry.key.get()), "'"));
            }
        }

        // Check if any new "var"s will shadow any previous "let"/"const"/"class" names.
        // It's an error to introduce a shadow.
        if (!globalLexicalEnvironment->isEmpty()) {
            for (auto& entry : variableDeclarations) {
                bool hasProperty = globalLexicalEnvironment->hasProperty(exec, entry.key.get());
                RETURN_IF_EXCEPTION(throwScope, throwScope.exception());
                if (hasProperty)
                    return createSyntaxError(exec, makeString("Can't create duplicate variable: '", String(entry.key.get()), "'"));
            }
        }
    }


    m_unlinkedProgramCodeBlock.set(vm, this, unlinkedCodeBlock);

    BatchedTransitionOptimizer optimizer(vm, globalObject);

    for (size_t i = 0, numberOfFunctions = unlinkedCodeBlock->numberOfFunctionDecls(); i < numberOfFunctions; ++i) {
        UnlinkedFunctionExecutable* unlinkedFunctionExecutable = unlinkedCodeBlock->functionDecl(i);
        ASSERT(!unlinkedFunctionExecutable->name().isEmpty());
        globalObject->addFunction(callFrame, unlinkedFunctionExecutable->name());
        if (vm.typeProfiler() || vm.controlFlowProfiler()) {
            vm.functionHasExecutedCache()->insertUnexecutedRange(sourceID(), 
                unlinkedFunctionExecutable->typeProfilingStartOffset(), 
                unlinkedFunctionExecutable->typeProfilingEndOffset());
        }
    }

    for (auto& entry : variableDeclarations) {
        ASSERT(entry.value.isVar());
        globalObject->addVar(callFrame, Identifier::fromUid(&vm, entry.key.get()));
        throwScope.assertNoException();
    }

    {
        JSGlobalLexicalEnvironment* globalLexicalEnvironment = jsCast<JSGlobalLexicalEnvironment*>(globalObject->globalScope());
        SymbolTable* symbolTable = globalLexicalEnvironment->symbolTable();
        ConcurrentJSLocker locker(symbolTable->m_lock);
        for (auto& entry : lexicalDeclarations) {
            if (UNLIKELY(entry.value.isConst() && !vm.globalConstRedeclarationShouldThrow() && !isStrictMode())) {
                if (symbolTable->contains(locker, entry.key.get()))
                    continue;
            }
            ScopeOffset offset = symbolTable->takeNextScopeOffset(locker);
            SymbolTableEntry newEntry(VarOffset(offset), static_cast<unsigned>(entry.value.isConst() ? PropertyAttribute::ReadOnly : PropertyAttribute::None));
            newEntry.prepareToWatch();
            symbolTable->add(locker, entry.key.get(), newEntry);
            
            ScopeOffset offsetForAssert = globalLexicalEnvironment->addVariables(1, jsTDZValue());
            RELEASE_ASSERT(offsetForAssert == offset);
        }
    }
    if (lexicalDeclarations.size()) {
#if ENABLE(DFG_JIT)
        for (auto& entry : lexicalDeclarations) {
            // If WatchpointSet exists, just fire it. Since DFG WatchpointSet addition is also done on the main thread, we can sync them.
            // So that we do not create WatchpointSet here. DFG will create if necessary on the main thread.
            // And it will only create not-invalidated watchpoint set if the global lexical environment binding doesn't exist, which is why this code works.
            if (auto* watchpointSet = globalObject->getReferencedPropertyWatchpointSet(entry.key.get()))
                watchpointSet->fireAll(vm, "Lexical binding shadows an existing global property");
        }
#endif
        globalObject->bumpGlobalLexicalBindingEpoch(vm);
    }
    return nullptr;
}

void ProgramExecutable::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    ProgramExecutable* thisObject = jsCast<ProgramExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);
    visitor.append(thisObject->m_unlinkedProgramCodeBlock);
    visitor.append(thisObject->m_programCodeBlock);
}

} // namespace JSC
