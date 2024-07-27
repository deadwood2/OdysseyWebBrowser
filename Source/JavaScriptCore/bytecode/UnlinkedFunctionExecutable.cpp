/*
 * Copyright (C) 2012, 2013, 2015 Apple Inc. All Rights Reserved.
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
#include "UnlinkedFunctionExecutable.h"

#include "BytecodeGenerator.h"
#include "ClassInfo.h"
#include "CodeCache.h"
#include "Executable.h"
#include "ExecutableInfo.h"
#include "FunctionOverrides.h"
#include "JSCInlines.h"
#include "JSString.h"
#include "Parser.h"
#include "SourceProvider.h"
#include "Structure.h"
#include "SymbolTable.h"
#include "UnlinkedInstructionStream.h"
#include <wtf/DataLog.h>

namespace JSC {

static_assert(sizeof(UnlinkedFunctionExecutable) <= 256, "UnlinkedFunctionExecutable should fit in a 256-byte cell.");

const ClassInfo UnlinkedFunctionExecutable::s_info = { "UnlinkedFunctionExecutable", 0, 0, CREATE_METHOD_TABLE(UnlinkedFunctionExecutable) };

static UnlinkedFunctionCodeBlock* generateFunctionCodeBlock(
    VM& vm, UnlinkedFunctionExecutable* executable, const SourceCode& source,
    CodeSpecializationKind kind, DebuggerMode debuggerMode, ProfilerMode profilerMode,
    UnlinkedFunctionKind functionKind, ParserError& error)
{
    JSParserBuiltinMode builtinMode = executable->isBuiltinFunction() ? JSParserBuiltinMode::Builtin : JSParserBuiltinMode::NotBuiltin;
    JSParserStrictMode strictMode = executable->isInStrictContext() ? JSParserStrictMode::Strict : JSParserStrictMode::NotStrict;
    ASSERT(isFunctionParseMode(executable->parseMode()));
    std::unique_ptr<FunctionNode> function = parse<FunctionNode>(
        &vm, source, executable->name(), builtinMode, strictMode, executable->parseMode(), error, nullptr);

    if (!function) {
        ASSERT(error.isValid());
        return nullptr;
    }

    function->finishParsing(executable->name(), executable->functionMode());
    executable->recordParse(function->features(), function->hasCapturedVariables());
    
    UnlinkedFunctionCodeBlock* result = UnlinkedFunctionCodeBlock::create(&vm, FunctionCode,
        ExecutableInfo(function->needsActivation(), function->usesEval(), function->isStrictMode(), kind == CodeForConstruct, functionKind == UnlinkedBuiltinFunction, executable->constructorKind()));
    auto generator(std::make_unique<BytecodeGenerator>(vm, function.get(), result, debuggerMode, profilerMode, executable->parentScopeTDZVariables()));
    error = generator->generate();
    if (error.isValid())
        return nullptr;
    return result;
}

UnlinkedFunctionExecutable::UnlinkedFunctionExecutable(VM* vm, Structure* structure, const SourceCode& source, RefPtr<SourceProvider>&& sourceOverride, FunctionMetadataNode* node, UnlinkedFunctionKind kind, ConstructAbility constructAbility, VariableEnvironment& parentScopeTDZVariables)
    : Base(*vm, structure)
    , m_name(node->ident())
    , m_inferredName(node->inferredName())
    , m_sourceOverride(WTF::move(sourceOverride))
    , m_firstLineOffset(node->firstLine() - source.firstLine())
    , m_lineCount(node->lastLine() - node->firstLine())
    , m_unlinkedFunctionNameStart(node->functionNameStart() - source.startOffset())
    , m_unlinkedBodyStartColumn(node->startColumn())
    , m_unlinkedBodyEndColumn(m_lineCount ? node->endColumn() : node->endColumn() - node->startColumn())
    , m_startOffset(node->source().startOffset() - source.startOffset())
    , m_sourceLength(node->source().length())
    , m_parametersStartOffset(node->parametersStart())
    , m_typeProfilingStartOffset(node->functionKeywordStart())
    , m_typeProfilingEndOffset(node->startStartOffset() + node->source().length() - 1)
    , m_parameterCount(node->parameterCount())
    , m_parseMode(node->parseMode())
    , m_features(0)
    , m_isInStrictContext(node->isInStrictContext())
    , m_hasCapturedVariables(false)
    , m_isBuiltinFunction(kind == UnlinkedBuiltinFunction)
    , m_constructAbility(static_cast<unsigned>(constructAbility))
    , m_constructorKind(static_cast<unsigned>(node->constructorKind()))
    , m_functionMode(node->functionMode())
{
    ASSERT(m_constructorKind == static_cast<unsigned>(node->constructorKind()));
    m_parentScopeTDZVariables.swap(parentScopeTDZVariables);
}

void UnlinkedFunctionExecutable::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    UnlinkedFunctionExecutable* thisObject = jsCast<UnlinkedFunctionExecutable*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_codeBlockForCall);
    visitor.append(&thisObject->m_codeBlockForConstruct);
    visitor.append(&thisObject->m_nameValue);
}

FunctionExecutable* UnlinkedFunctionExecutable::link(VM& vm, const SourceCode& ownerSource, int overrideLineNumber)
{
    SourceCode source = m_sourceOverride ? SourceCode(m_sourceOverride) : ownerSource;
    unsigned firstLine = source.firstLine() + m_firstLineOffset;
    unsigned startOffset = source.startOffset() + m_startOffset;
    unsigned lineCount = m_lineCount;

    // Adjust to one-based indexing.
    bool startColumnIsOnFirstSourceLine = !m_firstLineOffset;
    unsigned startColumn = m_unlinkedBodyStartColumn + (startColumnIsOnFirstSourceLine ? source.startColumn() : 1);
    bool endColumnIsOnStartLine = !lineCount;
    unsigned endColumn = m_unlinkedBodyEndColumn + (endColumnIsOnStartLine ? startColumn : 1);

    SourceCode code(source.provider(), startOffset, startOffset + m_sourceLength, firstLine, startColumn);
    FunctionOverrides::OverrideInfo overrideInfo;
    bool hasFunctionOverride = false;

    if (UNLIKELY(Options::functionOverrides())) {
        hasFunctionOverride = FunctionOverrides::initializeOverrideFor(code, overrideInfo);
        if (hasFunctionOverride) {
            firstLine = overrideInfo.firstLine;
            lineCount = overrideInfo.lineCount;
            startColumn = overrideInfo.startColumn;
            endColumn = overrideInfo.endColumn;
            code = overrideInfo.sourceCode;
        }
    }

    FunctionExecutable* result = FunctionExecutable::create(vm, code, this, firstLine, firstLine + lineCount, startColumn, endColumn);
    if (overrideLineNumber != -1)
        result->setOverrideLineNumber(overrideLineNumber);

    if (UNLIKELY(hasFunctionOverride)) {
        result->overrideParameterAndTypeProfilingStartEndOffsets(
            overrideInfo.parametersStartOffset,
            overrideInfo.typeProfilingStartOffset,
            overrideInfo.typeProfilingEndOffset);
    }

    return result;
}

UnlinkedFunctionExecutable* UnlinkedFunctionExecutable::fromGlobalCode(
    const Identifier& name, ExecState& exec, const SourceCode& source, 
    JSObject*& exception, int overrideLineNumber)
{
    ParserError error;
    VM& vm = exec.vm();
    CodeCache* codeCache = vm.codeCache();
    UnlinkedFunctionExecutable* executable = codeCache->getFunctionExecutableFromGlobalCode(vm, name, source, error);

    auto& globalObject = *exec.lexicalGlobalObject();
    if (globalObject.hasDebugger())
        globalObject.debugger()->sourceParsed(&exec, source.provider(), error.line(), error.message());

    if (error.isValid()) {
        exception = error.toErrorObject(&globalObject, source, overrideLineNumber);
        return nullptr;
    }

    return executable;
}

UnlinkedFunctionCodeBlock* UnlinkedFunctionExecutable::codeBlockFor(
    VM& vm, const SourceCode& source, CodeSpecializationKind specializationKind, 
    DebuggerMode debuggerMode, ProfilerMode profilerMode, ParserError& error)
{
    switch (specializationKind) {
    case CodeForCall:
        if (UnlinkedFunctionCodeBlock* codeBlock = m_codeBlockForCall.get())
            return codeBlock;
        break;
    case CodeForConstruct:
        if (UnlinkedFunctionCodeBlock* codeBlock = m_codeBlockForConstruct.get())
            return codeBlock;
        break;
    }

    UnlinkedFunctionCodeBlock* result = generateFunctionCodeBlock(
        vm, this, source, specializationKind, debuggerMode, profilerMode, 
        isBuiltinFunction() ? UnlinkedBuiltinFunction : UnlinkedNormalFunction, 
        error);
    
    if (error.isValid())
        return nullptr;

    switch (specializationKind) {
    case CodeForCall:
        m_codeBlockForCall.set(vm, this, result);
        break;
    case CodeForConstruct:
        m_codeBlockForConstruct.set(vm, this, result);
        break;
    }
    return result;
}

} // namespace JSC
