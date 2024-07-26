/*
 * Copyright (C) 2008, 2009, 2012-2015 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 * Copyright (C) 2012 Igalia, S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BytecodeGenerator.h"

#include "BuiltinExecutables.h"
#include "Interpreter.h"
#include "JSFunction.h"
#include "JSLexicalEnvironment.h"
#include "JSTemplateRegistryKey.h"
#include "LowLevelInterpreter.h"
#include "JSCInlines.h"
#include "Options.h"
#include "StackAlignment.h"
#include "StrongInlines.h"
#include "UnlinkedCodeBlock.h"
#include "UnlinkedInstructionStream.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

using namespace std;

namespace JSC {

void Label::setLocation(unsigned location)
{
    m_location = location;
    
    unsigned size = m_unresolvedJumps.size();
    for (unsigned i = 0; i < size; ++i)
        m_generator.instructions()[m_unresolvedJumps[i].second].u.operand = m_location - m_unresolvedJumps[i].first;
}

ParserError BytecodeGenerator::generate()
{
    SamplingRegion samplingRegion("Bytecode Generation");

    m_codeBlock->setThisRegister(m_thisRegister.virtualRegister());
    
    // If we have declared a variable named "arguments" and we are using arguments then we should
    // perform that assignment now.
    if (m_needToInitializeArguments)
        initializeVariable(variable(propertyNames().arguments), m_argumentsRegister);

    pushLexicalScope(m_scopeNode, true);

    {
        RefPtr<RegisterID> temp = newTemporary();
        RefPtr<RegisterID> globalScope = m_topMostScope;
        for (auto functionPair : m_functionsToInitialize) {
            FunctionMetadataNode* metadata = functionPair.first;
            FunctionVariableType functionType = functionPair.second;
            emitNewFunction(temp.get(), metadata);
            if (functionType == NormalFunctionVariable)
                initializeVariable(variable(metadata->ident()) , temp.get());
            else if (functionType == GlobalFunctionVariable)
                emitPutToScope(globalScope.get(), Variable(metadata->ident()), temp.get(), ThrowIfNotFound);
            else
                RELEASE_ASSERT_NOT_REACHED();
        }
    }
    
    bool callingClassConstructor = constructorKind() != ConstructorKind::None && !isConstructor();
    if (!callingClassConstructor)
        m_scopeNode->emitBytecode(*this);

    m_staticPropertyAnalyzer.kill();

    for (unsigned i = 0; i < m_tryRanges.size(); ++i) {
        TryRange& range = m_tryRanges[i];
        int start = range.start->bind();
        int end = range.end->bind();
        
        // This will happen for empty try blocks and for some cases of finally blocks:
        //
        // try {
        //    try {
        //    } finally {
        //        return 42;
        //        // *HERE*
        //    }
        // } finally {
        //    print("things");
        // }
        //
        // The return will pop scopes to execute the outer finally block. But this includes
        // popping the try context for the inner try. The try context is live in the fall-through
        // part of the finally block not because we will emit a handler that overlaps the finally,
        // but because we haven't yet had a chance to plant the catch target. Then when we finish
        // emitting code for the outer finally block, we repush the try contex, this time with a
        // new start index. But that means that the start index for the try range corresponding
        // to the inner-finally-following-the-return (marked as "*HERE*" above) will be greater
        // than the end index of the try block. This is harmless since end < start handlers will
        // never get matched in our logic, but we do the runtime a favor and choose to not emit
        // such handlers at all.
        if (end <= start)
            continue;
        
        ASSERT(range.tryData->handlerType != HandlerType::Illegal);
        UnlinkedHandlerInfo info(static_cast<uint32_t>(start), static_cast<uint32_t>(end),
            static_cast<uint32_t>(range.tryData->target->bind()), range.tryData->handlerType);
        m_codeBlock->addExceptionHandler(info);
    }
    
    m_codeBlock->setInstructions(std::make_unique<UnlinkedInstructionStream>(m_instructions));

    m_codeBlock->shrinkToFit();

    if (m_expressionTooDeep)
        return ParserError(ParserError::OutOfMemory);
    return ParserError(ParserError::ErrorNone);
}

BytecodeGenerator::BytecodeGenerator(VM& vm, ProgramNode* programNode, UnlinkedProgramCodeBlock* codeBlock, DebuggerMode debuggerMode, ProfilerMode profilerMode, const VariableEnvironment* parentScopeTDZVariables)
    : m_shouldEmitDebugHooks(Options::forceDebuggerBytecodeGeneration() || debuggerMode == DebuggerOn)
    , m_shouldEmitProfileHooks(Options::forceProfilerBytecodeGeneration() || profilerMode == ProfilerOn)
    , m_scopeNode(programNode)
    , m_codeBlock(vm, codeBlock)
    , m_thisRegister(CallFrame::thisArgumentOffset())
    , m_codeType(GlobalCode)
    , m_vm(&vm)
{
    ASSERT_UNUSED(parentScopeTDZVariables, !parentScopeTDZVariables->size());

    for (auto& constantRegister : m_linkTimeConstantRegisters)
        constantRegister = nullptr;

    m_codeBlock->setNumParameters(1); // Allocate space for "this"

    emitOpcode(op_enter);

    allocateAndEmitScope();

    const FunctionStack& functionStack = programNode->functionStack();

    for (size_t i = 0; i < functionStack.size(); ++i) {
        FunctionMetadataNode* function = functionStack[i];
        m_functionsToInitialize.append(std::make_pair(function, GlobalFunctionVariable));
    }
    if (Options::validateBytecode()) {
        for (auto& entry : programNode->varDeclarations())
            RELEASE_ASSERT(entry.value.isVar());
    }
    codeBlock->setVariableDeclarations(programNode->varDeclarations());
}

BytecodeGenerator::BytecodeGenerator(VM& vm, FunctionNode* functionNode, UnlinkedFunctionCodeBlock* codeBlock, DebuggerMode debuggerMode, ProfilerMode profilerMode, const VariableEnvironment* parentScopeTDZVariables)
    : m_shouldEmitDebugHooks(Options::forceDebuggerBytecodeGeneration() || debuggerMode == DebuggerOn)
    , m_shouldEmitProfileHooks(Options::forceProfilerBytecodeGeneration() || profilerMode == ProfilerOn)
    , m_scopeNode(functionNode)
    , m_codeBlock(vm, codeBlock)
    , m_codeType(FunctionCode)
    , m_vm(&vm)
    , m_isBuiltinFunction(codeBlock->isBuiltinFunction())
    , m_usesNonStrictEval(codeBlock->usesEval() && !codeBlock->isStrictMode())
{
    for (auto& constantRegister : m_linkTimeConstantRegisters)
        constantRegister = nullptr;

    if (m_isBuiltinFunction)
        m_shouldEmitDebugHooks = false;
    
    SymbolTable* functionSymbolTable = SymbolTable::create(*m_vm);
    functionSymbolTable->setUsesNonStrictEval(m_usesNonStrictEval);
    int symbolTableConstantIndex = addConstantValue(functionSymbolTable)->index();

    Vector<Identifier> boundParameterProperties;
    FunctionParameters& parameters = *functionNode->parameters(); 
    if (!parameters.hasDefaultParameterValues()) { 
        // If we do have default parameters, they will be allocated in a separate scope.
        for (size_t i = 0; i < parameters.size(); i++) {
            auto pattern = parameters.at(i).first;
            if (pattern->isBindingNode())
                continue;
            pattern->collectBoundIdentifiers(boundParameterProperties);
        }
    }

    bool shouldCaptureSomeOfTheThings = m_shouldEmitDebugHooks || m_codeBlock->needsFullScopeChain();
    bool shouldCaptureAllOfTheThings = m_shouldEmitDebugHooks || codeBlock->usesEval();
    bool needsArguments = functionNode->usesArguments() || codeBlock->usesEval();
    if (shouldCaptureAllOfTheThings)
        functionNode->varDeclarations().markAllVariablesAsCaptured();
    
    auto captures = [&] (UniquedStringImpl* uid) -> bool {
        if (!shouldCaptureSomeOfTheThings)
            return false;
        if (needsArguments && uid == propertyNames().arguments.impl()) {
            // Actually, we only need to capture the arguments object when we "need full activation"
            // because of name scopes. But historically we did it this way, so for now we just preserve
            // the old behavior.
            // FIXME: https://bugs.webkit.org/show_bug.cgi?id=143072
            return true;
        }
        return functionNode->captures(uid);
    };
    auto varKind = [&] (UniquedStringImpl* uid) -> VarKind {
        return captures(uid) ? VarKind::Scope : VarKind::Stack;
    };

    emitOpcode(op_enter);

    allocateAndEmitScope();
    
    m_calleeRegister.setIndex(JSStack::Callee);
    
    if (functionNameIsInScope(functionNode->ident(), functionNode->functionMode())
        && functionNameScopeIsDynamic(codeBlock->usesEval(), codeBlock->isStrictMode())) {
        emitPushFunctionNameScope(functionNode->ident(), &m_calleeRegister);
    }
    
    if (shouldCaptureSomeOfTheThings) {
        m_lexicalEnvironmentRegister = addVar();
        // We can allocate the "var" environment if we don't have default parameter expressions. If we have
        // default parameter expressions, we have to hold off on allocating the "var" environment because
        // the parent scope of the "var" environment is the parameter environment.
        if (!parameters.hasDefaultParameterValues())
            initializeVarLexicalEnvironment(symbolTableConstantIndex);
    }

    // Make sure the code block knows about all of our parameters, and make sure that parameters
    // needing destructuring are noted.
    m_parameters.grow(parameters.size() + 1); // reserve space for "this"
    m_thisRegister.setIndex(initializeNextParameter()->index()); // this
    for (unsigned i = 0; i < parameters.size(); ++i)
        initializeNextParameter();
    
    // Figure out some interesting facts about our arguments.
    bool capturesAnyArgumentByName = false;
    if (functionNode->hasCapturedVariables()) {
        FunctionParameters& parameters = *functionNode->parameters();
        for (size_t i = 0; i < parameters.size(); ++i) {
            auto pattern = parameters.at(i).first;
            if (!pattern->isBindingNode())
                continue;
            const Identifier& ident = static_cast<const BindingNode*>(pattern)->boundProperty();
            capturesAnyArgumentByName |= captures(ident.impl());
        }
    }
    
    if (capturesAnyArgumentByName)
        ASSERT(m_lexicalEnvironmentRegister);

    // Need to know what our functions are called. Parameters have some goofy behaviors when it
    // comes to functions of the same name.
    for (FunctionMetadataNode* function : functionNode->functionStack())
        m_functions.add(function->ident().impl());
    
    if (needsArguments) {
        // Create the arguments object now. We may put the arguments object into the activation if
        // it is captured. Either way, we create two arguments object variables: one is our
        // private variable that is immutable, and another that is the user-visible variable. The
        // immutable one is only used here, or during formal parameter resolutions if we opt for
        // DirectArguments.
        
        m_argumentsRegister = addVar();
        m_argumentsRegister->ref();
    }
    
    if (needsArguments && !codeBlock->isStrictMode() && !parameters.hasDefaultParameterValues()) {
        // If we captured any formal parameter by name, then we use ScopedArguments. Otherwise we
        // use DirectArguments. With ScopedArguments, we lift all of our arguments into the
        // activation.
        
        if (capturesAnyArgumentByName) {
            functionSymbolTable->setArgumentsLength(vm, parameters.size());
            
            // For each parameter, we have two possibilities:
            // Either it's a binding node with no function overlap, in which case it gets a name
            // in the symbol table - or it just gets space reserved in the symbol table. Either
            // way we lift the value into the scope.
            for (unsigned i = 0; i < parameters.size(); ++i) {
                ScopeOffset offset = functionSymbolTable->takeNextScopeOffset();
                functionSymbolTable->setArgumentOffset(vm, i, offset);
                if (UniquedStringImpl* name = visibleNameForParameter(parameters.at(i).first)) {
                    VarOffset varOffset(offset);
                    SymbolTableEntry entry(varOffset);
                    // Stores to these variables via the ScopedArguments object will not do
                    // notifyWrite(), since that would be cumbersome. Also, watching formal
                    // parameters when "arguments" is in play is unlikely to be super profitable.
                    // So, we just disable it.
                    entry.disableWatching();
                    functionSymbolTable->set(name, entry);
                }
                emitOpcode(op_put_to_scope);
                instructions().append(m_lexicalEnvironmentRegister->index());
                instructions().append(UINT_MAX);
                instructions().append(virtualRegisterForArgument(1 + i).offset());
                instructions().append(ResolveModeAndType(ThrowIfNotFound, LocalClosureVar).operand());
                instructions().append(symbolTableConstantIndex);
                instructions().append(offset.offset());
            }
            
            // This creates a scoped arguments object and copies the overflow arguments into the
            // scope. It's the equivalent of calling ScopedArguments::createByCopying().
            emitOpcode(op_create_scoped_arguments);
            instructions().append(m_argumentsRegister->index());
            instructions().append(m_lexicalEnvironmentRegister->index());
        } else {
            // We're going to put all parameters into the DirectArguments object. First ensure
            // that the symbol table knows that this is happening.
            for (unsigned i = 0; i < parameters.size(); ++i) {
                if (UniquedStringImpl* name = visibleNameForParameter(parameters.at(i).first))
                    functionSymbolTable->set(name, SymbolTableEntry(VarOffset(DirectArgumentsOffset(i))));
            }
            
            emitOpcode(op_create_direct_arguments);
            instructions().append(m_argumentsRegister->index());
        }
    } else if (!parameters.hasDefaultParameterValues()) {
        // Create the formal parameters the normal way. Any of them could be captured, or not. If
        // captured, lift them into the scope. We can not do this if we have default parameter expressions
        // because when default parameter expressions exist, they belong in their own lexical environment
        // separate from the "var" lexical environment.
        for (unsigned i = 0; i < parameters.size(); ++i) {
            UniquedStringImpl* name = visibleNameForParameter(parameters.at(i).first);
            if (!name)
                continue;
            
            if (!captures(name)) {
                // This is the easy case - just tell the symbol table about the argument. It will
                // be accessed directly.
                functionSymbolTable->set(name, SymbolTableEntry(VarOffset(virtualRegisterForArgument(1 + i))));
                continue;
            }
            
            ScopeOffset offset = functionSymbolTable->takeNextScopeOffset();
            const Identifier& ident =
                static_cast<const BindingNode*>(parameters.at(i).first)->boundProperty();
            functionSymbolTable->set(name, SymbolTableEntry(VarOffset(offset)));
            
            emitOpcode(op_put_to_scope);
            instructions().append(m_lexicalEnvironmentRegister->index());
            instructions().append(addConstant(ident));
            instructions().append(virtualRegisterForArgument(1 + i).offset());
            instructions().append(ResolveModeAndType(ThrowIfNotFound, LocalClosureVar).operand());
            instructions().append(symbolTableConstantIndex);
            instructions().append(offset.offset());
        }
    }
    
    if (needsArguments && (codeBlock->isStrictMode() || parameters.hasDefaultParameterValues())) {
        // Allocate an out-of-bands arguments object.
        emitOpcode(op_create_out_of_band_arguments);
        instructions().append(m_argumentsRegister->index());
    }
    
    // Now declare all variables.
    for (const Identifier& ident : boundParameterProperties) {
        ASSERT(!parameters.hasDefaultParameterValues());
        createVariable(ident, varKind(ident.impl()), functionSymbolTable);
    }
    for (FunctionMetadataNode* function : functionNode->functionStack()) {
        const Identifier& ident = function->ident();
        createVariable(ident, varKind(ident.impl()), functionSymbolTable);
        m_functionsToInitialize.append(std::make_pair(function, NormalFunctionVariable));
    }
    for (auto& entry : functionNode->varDeclarations()) {
        ASSERT(!entry.value.isLet() && !entry.value.isConst());
        if (!entry.value.isVar()) // This is either a parameter or callee.
            continue;
        // Variables named "arguments" are never const.
        createVariable(Identifier::fromUid(m_vm, entry.key.get()), varKind(entry.key.get()), functionSymbolTable, IgnoreExisting);
    }

    // There are some variables that need to be preinitialized to something other than Undefined:
    //
    // - "arguments": unless it's used as a function or parameter, this should refer to the
    //   arguments object.
    //
    // - callee: unless it's used as a var, function, or parameter, this should refer to the
    //   callee (i.e. our function).
    //
    // - functions: these always override everything else.
    //
    // The most logical way to do all of this is to initialize none of the variables until now,
    // and then initialize them in BytecodeGenerator::generate() in such an order that the rules
    // for how these things override each other end up holding. We would initialize the callee
    // first, then "arguments", then all arguments, then the functions.
    //
    // But some arguments are already initialized by default, since if they aren't captured and we
    // don't have "arguments" then we just point the symbol table at the stack slot of those
    // arguments. We end up initializing the rest of the arguments that have an uncomplicated
    // binding (i.e. don't involve destructuring) above when figuring out how to lay them out,
    // because that's just the simplest thing. This means that when we initialize them, we have to
    // watch out for the things that override arguments (namely, functions).
    //
    // We also initialize callee here as well, just because it's so weird. We know whether we want
    // to do this because we can just check if it's in the symbol table.
    if (functionNameIsInScope(functionNode->ident(), functionNode->functionMode())
        && !functionNameScopeIsDynamic(codeBlock->usesEval(), codeBlock->isStrictMode())
        && functionSymbolTable->get(functionNode->ident().impl()).isNull()) {
        if (captures(functionNode->ident().impl())) {
            ScopeOffset offset;
            {
                ConcurrentJITLocker locker(functionSymbolTable->m_lock);
                offset = functionSymbolTable->takeNextScopeOffset(locker);
                functionSymbolTable->add(
                    locker, functionNode->ident().impl(),
                    SymbolTableEntry(VarOffset(offset), ReadOnly));
            }
            
            emitOpcode(op_put_to_scope);
            instructions().append(m_lexicalEnvironmentRegister->index());
            instructions().append(addConstant(functionNode->ident()));
            instructions().append(m_calleeRegister.index());
            instructions().append(ResolveModeAndType(ThrowIfNotFound, LocalClosureVar).operand());
            instructions().append(symbolTableConstantIndex);
            instructions().append(offset.offset());
        } else {
            functionSymbolTable->add(
                functionNode->ident().impl(),
                SymbolTableEntry(VarOffset(m_calleeRegister.virtualRegister()), ReadOnly));
        }
    }
    
    // This is our final act of weirdness. "arguments" is overridden by everything except the
    // callee. We add it to the symbol table if it's not already there and it's not an argument.
    if (needsArguments) {
        // If "arguments" is overridden by a function or destructuring parameter name, then it's
        // OK for us to call createVariable() because it won't change anything. It's also OK for
        // us to them tell BytecodeGenerator::generate() to write to it because it will do so
        // before it initializes functions and destructuring parameters. But if "arguments" is
        // overridden by a "simple" function parameter, then we have to bail: createVariable()
        // would assert and BytecodeGenerator::generate() would write the "arguments" after the
        // argument value had already been properly initialized.
        
        bool haveParameterNamedArguments = false;
        for (unsigned i = 0; i < parameters.size(); ++i) {
            UniquedStringImpl* name = visibleNameForParameter(parameters.at(i).first);
            if (name == propertyNames().arguments.impl()) {
                haveParameterNamedArguments = true;
                break;
            }
        }
        
        if (!haveParameterNamedArguments) {
            createVariable(
                propertyNames().arguments, varKind(propertyNames().arguments.impl()), functionSymbolTable);
            m_needToInitializeArguments = true;
        }
    }

    m_newTargetRegister = addVar();
    if (isConstructor()) {
        emitMove(m_newTargetRegister, &m_thisRegister);
        if (constructorKind() == ConstructorKind::Derived) {
            emitMoveEmptyValue(&m_thisRegister);
        } else
            emitCreateThis(&m_thisRegister);
    } else if (constructorKind() != ConstructorKind::None) {
        emitThrowTypeError("Cannot call a class constructor");
    } else if (functionNode->usesThis() || codeBlock->usesEval()) {
        m_codeBlock->addPropertyAccessInstruction(instructions().size());
        emitOpcode(op_to_this);
        instructions().append(kill(&m_thisRegister));
        instructions().append(0);
        instructions().append(0);
    }

    // All "addVar()"s needs to happen before "initializeDefaultParameterValuesAndSetupFunctionScopeStack()" is called
    // because a function's default parameter ExpressionNodes will use temporary registers.
    m_TDZStack.append(std::make_pair(*parentScopeTDZVariables, false));
    initializeDefaultParameterValuesAndSetupFunctionScopeStack(parameters, functionNode, functionSymbolTable, symbolTableConstantIndex, captures);
}

BytecodeGenerator::BytecodeGenerator(VM& vm, EvalNode* evalNode, UnlinkedEvalCodeBlock* codeBlock, DebuggerMode debuggerMode, ProfilerMode profilerMode, const VariableEnvironment* parentScopeTDZVariables)
    : m_shouldEmitDebugHooks(Options::forceDebuggerBytecodeGeneration() || debuggerMode == DebuggerOn)
    , m_shouldEmitProfileHooks(Options::forceProfilerBytecodeGeneration() || profilerMode == ProfilerOn)
    , m_scopeNode(evalNode)
    , m_codeBlock(vm, codeBlock)
    , m_thisRegister(CallFrame::thisArgumentOffset())
    , m_codeType(EvalCode)
    , m_vm(&vm)
    , m_usesNonStrictEval(codeBlock->usesEval() && !codeBlock->isStrictMode())
{
    for (auto& constantRegister : m_linkTimeConstantRegisters)
        constantRegister = nullptr;

    m_codeBlock->setNumParameters(1);

    emitOpcode(op_enter);

    allocateAndEmitScope();

    const DeclarationStacks::FunctionStack& functionStack = evalNode->functionStack();
    for (size_t i = 0; i < functionStack.size(); ++i)
        m_codeBlock->addFunctionDecl(makeFunction(functionStack[i]));

    const VariableEnvironment& varDeclarations = evalNode->varDeclarations();
    unsigned numVariables = varDeclarations.size();
    Vector<Identifier, 0, UnsafeVectorOverflow> variables;
    variables.reserveCapacity(numVariables);
    for (auto& entry : varDeclarations) {
        ASSERT(entry.value.isVar());
        ASSERT(entry.key->isAtomic() || entry.key->isSymbol());
        variables.append(Identifier::fromUid(m_vm, entry.key.get()));
    }
    codeBlock->adoptVariables(variables);

    m_TDZStack.append(std::make_pair(*parentScopeTDZVariables, false));
}

BytecodeGenerator::~BytecodeGenerator()
{
}

void BytecodeGenerator::initializeDefaultParameterValuesAndSetupFunctionScopeStack(
    FunctionParameters& parameters, FunctionNode* functionNode, SymbolTable* functionSymbolTable, 
    int symbolTableConstantIndex, const std::function<bool (UniquedStringImpl*)>& captures)
{
    Vector<std::pair<Identifier, RefPtr<RegisterID>>> valuesToMoveIntoVars;
    if (parameters.hasDefaultParameterValues()) {
        // Refer to the ES6 spec section 9.2.12: http://www.ecma-international.org/ecma-262/6.0/index.html#sec-functiondeclarationinstantiation
        // This implements step 21.
        VariableEnvironment environment;
        Vector<Identifier> allParameterNames; 
        for (unsigned i = 0; i < parameters.size(); i++)
            parameters.at(i).first->collectBoundIdentifiers(allParameterNames);
        IdentifierSet parameterSet;
        for (auto& ident : allParameterNames) {
            parameterSet.add(ident.impl());
            auto addResult = environment.add(ident);
            addResult.iterator->value.setIsLet(); // When we have default parameter expressions, parameters act like "let" variables.
            if (captures(ident.impl()))
                addResult.iterator->value.setIsCaptured();
        }
        
        // This implements step 25 of section 9.2.12.
        pushLexicalScopeInternal(environment, true, nullptr, TDZRequirement::UnderTDZ, ScopeType::LetConstScope, ScopeRegisterType::Block);

        RefPtr<RegisterID> temp = newTemporary();
        for (unsigned i = 0; i < parameters.size(); i++) {
            std::pair<DestructuringPatternNode*, ExpressionNode*> parameter = parameters.at(i);
            RefPtr<RegisterID> parameterValue = &registerFor(virtualRegisterForArgument(1 + i));
            emitMove(temp.get(), parameterValue.get());
            if (parameter.second) {
                RefPtr<RegisterID> condition = emitIsUndefined(newTemporary(), parameterValue.get());
                RefPtr<Label> skipDefaultParameterBecauseNotUndefined = newLabel();
                emitJumpIfFalse(condition.get(), skipDefaultParameterBecauseNotUndefined.get());
                emitNode(temp.get(), parameter.second);
                emitLabel(skipDefaultParameterBecauseNotUndefined.get());
            }

            parameter.first->bindValue(*this, temp.get());
        }

        // Final act of weirdness for default parameters. If a "var" also
        // has the same name as a parameter, it should start out as the
        // value of that parameter. Note, though, that they will be distinct
        // bindings.
        // This is step 28 of section 9.2.12. 
        for (auto& entry : functionNode->varDeclarations()) {
            if (!entry.value.isVar()) // This is either a parameter or callee.
                continue;

            if (parameterSet.contains(entry.key)) {
                Identifier ident = Identifier::fromUid(m_vm, entry.key.get());
                Variable var = variable(ident);
                RegisterID* scope = emitResolveScope(nullptr, var);
                RefPtr<RegisterID> value = emitGetFromScope(newTemporary(), scope, var, DoNotThrowIfNotFound);
                valuesToMoveIntoVars.append(std::make_pair(ident, value));
            }
        }

        // Functions with default parameter expressions must have a separate environment
        // record for parameters and "var"s. The "var" environment record must have the
        // parameter environment record as its parent.
        // See step 28 of section 9.2.12.
        if (m_lexicalEnvironmentRegister)
            initializeVarLexicalEnvironment(symbolTableConstantIndex);
    }

    if (m_lexicalEnvironmentRegister)
        pushScopedControlFlowContext();
    m_symbolTableStack.append(SymbolTableStackEntry{ Strong<SymbolTable>(*m_vm, functionSymbolTable), m_lexicalEnvironmentRegister, false, symbolTableConstantIndex });

    // This completes step 28 of section 9.2.12.
    for (unsigned i = 0; i < valuesToMoveIntoVars.size(); i++) {
        ASSERT(parameters.hasDefaultParameterValues());
        Variable var = variable(valuesToMoveIntoVars[i].first);
        RegisterID* scope = emitResolveScope(nullptr, var);
        emitPutToScope(scope, var, valuesToMoveIntoVars[i].second.get(), DoNotThrowIfNotFound);
    }

    if (!parameters.hasDefaultParameterValues()) {
        ASSERT(!valuesToMoveIntoVars.size());
        // Initialize destructuring parameters the old way as if we don't have any default parameter values.
        // If we have default parameter values, we handle this case above.
        for (unsigned i = 0; i < parameters.size(); i++) {
            DestructuringPatternNode* pattern = parameters.at(i).first;
            if (!pattern->isBindingNode()) {
                RefPtr<RegisterID> parameterValue = &registerFor(virtualRegisterForArgument(1 + i));
                pattern->bindValue(*this, parameterValue.get());
            }
        }
    }
}

RegisterID* BytecodeGenerator::initializeNextParameter()
{
    VirtualRegister reg = virtualRegisterForArgument(m_codeBlock->numParameters());
    RegisterID& parameter = registerFor(reg);
    parameter.setIndex(reg.offset());
    m_codeBlock->addParameter();
    return &parameter;
}

void BytecodeGenerator::initializeVarLexicalEnvironment(int symbolTableConstantIndex)
{
    RELEASE_ASSERT(m_lexicalEnvironmentRegister);
    m_codeBlock->setActivationRegister(m_lexicalEnvironmentRegister->virtualRegister());
    emitOpcode(op_create_lexical_environment);
    instructions().append(m_lexicalEnvironmentRegister->index());
    instructions().append(scopeRegister()->index());
    instructions().append(symbolTableConstantIndex);
    instructions().append(addConstantValue(jsUndefined())->index());

    emitOpcode(op_mov);
    instructions().append(scopeRegister()->index());
    instructions().append(m_lexicalEnvironmentRegister->index());
}

UniquedStringImpl* BytecodeGenerator::visibleNameForParameter(DestructuringPatternNode* pattern)
{
    if (pattern->isBindingNode()) {
        const Identifier& ident = static_cast<const BindingNode*>(pattern)->boundProperty();
        if (!m_functions.contains(ident.impl()))
            return ident.impl();
    }
    return nullptr;
}

RegisterID* BytecodeGenerator::newRegister()
{
    m_calleeRegisters.append(virtualRegisterForLocal(m_calleeRegisters.size()));
    int numCalleeRegisters = max<int>(m_codeBlock->m_numCalleeRegisters, m_calleeRegisters.size());
    numCalleeRegisters = WTF::roundUpToMultipleOf(stackAlignmentRegisters(), numCalleeRegisters);
    m_codeBlock->m_numCalleeRegisters = numCalleeRegisters;
    return &m_calleeRegisters.last();
}

void BytecodeGenerator::reclaimFreeRegisters()
{
    while (m_calleeRegisters.size() && !m_calleeRegisters.last().refCount())
        m_calleeRegisters.removeLast();
}

RegisterID* BytecodeGenerator::newBlockScopeVariable()
{
    reclaimFreeRegisters();

    return newRegister();
}

RegisterID* BytecodeGenerator::newTemporary()
{
    reclaimFreeRegisters();

    RegisterID* result = newRegister();
    result->setTemporary();
    return result;
}

LabelScopePtr BytecodeGenerator::newLabelScope(LabelScope::Type type, const Identifier* name)
{
    // Reclaim free label scopes.
    while (m_labelScopes.size() && !m_labelScopes.last().refCount())
        m_labelScopes.removeLast();

    // Allocate new label scope.
    LabelScope scope(type, name, labelScopeDepth(), newLabel(), type == LabelScope::Loop ? newLabel() : PassRefPtr<Label>()); // Only loops have continue targets.
    m_labelScopes.append(scope);
    return LabelScopePtr(m_labelScopes, m_labelScopes.size() - 1);
}

PassRefPtr<Label> BytecodeGenerator::newLabel()
{
    // Reclaim free label IDs.
    while (m_labels.size() && !m_labels.last().refCount())
        m_labels.removeLast();

    // Allocate new label ID.
    m_labels.append(*this);
    return &m_labels.last();
}

PassRefPtr<Label> BytecodeGenerator::emitLabel(Label* l0)
{
    unsigned newLabelIndex = instructions().size();
    l0->setLocation(newLabelIndex);

    if (m_codeBlock->numberOfJumpTargets()) {
        unsigned lastLabelIndex = m_codeBlock->lastJumpTarget();
        ASSERT(lastLabelIndex <= newLabelIndex);
        if (newLabelIndex == lastLabelIndex) {
            // Peephole optimizations have already been disabled by emitting the last label
            return l0;
        }
    }

    m_codeBlock->addJumpTarget(newLabelIndex);

    // This disables peephole optimizations when an instruction is a jump target
    m_lastOpcodeID = op_end;
    return l0;
}

void BytecodeGenerator::emitOpcode(OpcodeID opcodeID)
{
#ifndef NDEBUG
    size_t opcodePosition = instructions().size();
    ASSERT(opcodePosition - m_lastOpcodePosition == opcodeLength(m_lastOpcodeID) || m_lastOpcodeID == op_end);
    m_lastOpcodePosition = opcodePosition;
#endif
    instructions().append(opcodeID);
    m_lastOpcodeID = opcodeID;
}

UnlinkedArrayProfile BytecodeGenerator::newArrayProfile()
{
    return m_codeBlock->addArrayProfile();
}

UnlinkedArrayAllocationProfile BytecodeGenerator::newArrayAllocationProfile()
{
    return m_codeBlock->addArrayAllocationProfile();
}

UnlinkedObjectAllocationProfile BytecodeGenerator::newObjectAllocationProfile()
{
    return m_codeBlock->addObjectAllocationProfile();
}

UnlinkedValueProfile BytecodeGenerator::emitProfiledOpcode(OpcodeID opcodeID)
{
    UnlinkedValueProfile result = m_codeBlock->addValueProfile();
    emitOpcode(opcodeID);
    return result;
}

void BytecodeGenerator::emitLoopHint()
{
    emitOpcode(op_loop_hint);
}

void BytecodeGenerator::retrieveLastBinaryOp(int& dstIndex, int& src1Index, int& src2Index)
{
    ASSERT(instructions().size() >= 4);
    size_t size = instructions().size();
    dstIndex = instructions().at(size - 3).u.operand;
    src1Index = instructions().at(size - 2).u.operand;
    src2Index = instructions().at(size - 1).u.operand;
}

void BytecodeGenerator::retrieveLastUnaryOp(int& dstIndex, int& srcIndex)
{
    ASSERT(instructions().size() >= 3);
    size_t size = instructions().size();
    dstIndex = instructions().at(size - 2).u.operand;
    srcIndex = instructions().at(size - 1).u.operand;
}

void ALWAYS_INLINE BytecodeGenerator::rewindBinaryOp()
{
    ASSERT(instructions().size() >= 4);
    instructions().shrink(instructions().size() - 4);
    m_lastOpcodeID = op_end;
}

void ALWAYS_INLINE BytecodeGenerator::rewindUnaryOp()
{
    ASSERT(instructions().size() >= 3);
    instructions().shrink(instructions().size() - 3);
    m_lastOpcodeID = op_end;
}

PassRefPtr<Label> BytecodeGenerator::emitJump(Label* target)
{
    size_t begin = instructions().size();
    emitOpcode(op_jmp);
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfTrue(RegisterID* cond, Label* target)
{
    if (m_lastOpcodeID == op_less) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jless);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_lesseq) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jlesseq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greater) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jgreater);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greatereq) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jgreatereq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_eq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jeq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_neq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jneq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    }

    size_t begin = instructions().size();

    emitOpcode(op_jtrue);
    instructions().append(cond->index());
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfFalse(RegisterID* cond, Label* target)
{
    if (m_lastOpcodeID == op_less && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jnless);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_lesseq && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jnlesseq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greater && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jngreater);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greatereq && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jngreatereq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_not) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jtrue);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_eq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jneq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_neq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jeq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    }

    size_t begin = instructions().size();
    emitOpcode(op_jfalse);
    instructions().append(cond->index());
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfNotFunctionCall(RegisterID* cond, Label* target)
{
    size_t begin = instructions().size();

    emitOpcode(op_jneq_ptr);
    instructions().append(cond->index());
    instructions().append(Special::CallFunction);
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfNotFunctionApply(RegisterID* cond, Label* target)
{
    size_t begin = instructions().size();

    emitOpcode(op_jneq_ptr);
    instructions().append(cond->index());
    instructions().append(Special::ApplyFunction);
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

bool BytecodeGenerator::hasConstant(const Identifier& ident) const
{
    UniquedStringImpl* rep = ident.impl();
    return m_identifierMap.contains(rep);
}

unsigned BytecodeGenerator::addConstant(const Identifier& ident)
{
    UniquedStringImpl* rep = ident.impl();
    IdentifierMap::AddResult result = m_identifierMap.add(rep, m_codeBlock->numberOfIdentifiers());
    if (result.isNewEntry)
        m_codeBlock->addIdentifier(ident);

    return result.iterator->value;
}

// We can't hash JSValue(), so we use a dedicated data member to cache it.
RegisterID* BytecodeGenerator::addConstantEmptyValue()
{
    if (!m_emptyValueRegister) {
        int index = m_nextConstantOffset;
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(JSValue());
        m_emptyValueRegister = &m_constantPoolRegisters[index];
    }

    return m_emptyValueRegister;
}

RegisterID* BytecodeGenerator::addConstantValue(JSValue v, SourceCodeRepresentation sourceCodeRepresentation)
{
    if (!v)
        return addConstantEmptyValue();

    int index = m_nextConstantOffset;

    EncodedJSValueWithRepresentation valueMapKey { JSValue::encode(v), sourceCodeRepresentation };
    JSValueMap::AddResult result = m_jsValueMap.add(valueMapKey, m_nextConstantOffset);
    if (result.isNewEntry) {
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(v, sourceCodeRepresentation);
    } else
        index = result.iterator->value;
    return &m_constantPoolRegisters[index];
}

RegisterID* BytecodeGenerator::emitMoveLinkTimeConstant(RegisterID* dst, LinkTimeConstant type)
{
    unsigned constantIndex = static_cast<unsigned>(type);
    if (!m_linkTimeConstantRegisters[constantIndex]) {
        int index = m_nextConstantOffset;
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(type);
        m_linkTimeConstantRegisters[constantIndex] = &m_constantPoolRegisters[index];
    }

    emitOpcode(op_mov);
    instructions().append(dst->index());
    instructions().append(m_linkTimeConstantRegisters[constantIndex]->index());

    return dst;
}

unsigned BytecodeGenerator::addRegExp(RegExp* r)
{
    return m_codeBlock->addRegExp(r);
}

RegisterID* BytecodeGenerator::emitMoveEmptyValue(RegisterID* dst)
{
    RefPtr<RegisterID> emptyValue = addConstantEmptyValue();

    emitOpcode(op_mov);
    instructions().append(dst->index());
    instructions().append(emptyValue->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitMove(RegisterID* dst, RegisterID* src)
{
    ASSERT(src != m_emptyValueRegister);

    m_staticPropertyAnalyzer.mov(dst->index(), src->index());
    emitOpcode(op_mov);
    instructions().append(dst->index());
    instructions().append(src->index());

    return dst;
}

RegisterID* BytecodeGenerator::emitUnaryOp(OpcodeID opcodeID, RegisterID* dst, RegisterID* src)
{
    emitOpcode(opcodeID);
    instructions().append(dst->index());
    instructions().append(src->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitInc(RegisterID* srcDst)
{
    emitOpcode(op_inc);
    instructions().append(srcDst->index());
    return srcDst;
}

RegisterID* BytecodeGenerator::emitDec(RegisterID* srcDst)
{
    emitOpcode(op_dec);
    instructions().append(srcDst->index());
    return srcDst;
}

RegisterID* BytecodeGenerator::emitBinaryOp(OpcodeID opcodeID, RegisterID* dst, RegisterID* src1, RegisterID* src2, OperandTypes types)
{
    emitOpcode(opcodeID);
    instructions().append(dst->index());
    instructions().append(src1->index());
    instructions().append(src2->index());

    if (opcodeID == op_bitor || opcodeID == op_bitand || opcodeID == op_bitxor ||
        opcodeID == op_add || opcodeID == op_mul || opcodeID == op_sub || opcodeID == op_div)
        instructions().append(types.toInt());

    return dst;
}

RegisterID* BytecodeGenerator::emitEqualityOp(OpcodeID opcodeID, RegisterID* dst, RegisterID* src1, RegisterID* src2)
{
    if (m_lastOpcodeID == op_typeof) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (src1->index() == dstIndex
            && src1->isTemporary()
            && m_codeBlock->isConstantRegisterIndex(src2->index())
            && m_codeBlock->constantRegister(src2->index()).get().isString()) {
            const String& value = asString(m_codeBlock->constantRegister(src2->index()).get())->tryGetValue();
            if (value == "undefined") {
                rewindUnaryOp();
                emitOpcode(op_is_undefined);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "boolean") {
                rewindUnaryOp();
                emitOpcode(op_is_boolean);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "number") {
                rewindUnaryOp();
                emitOpcode(op_is_number);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "string") {
                rewindUnaryOp();
                emitOpcode(op_is_string);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "object") {
                rewindUnaryOp();
                emitOpcode(op_is_object_or_null);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "function") {
                rewindUnaryOp();
                emitOpcode(op_is_function);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
        }
    }

    emitOpcode(opcodeID);
    instructions().append(dst->index());
    instructions().append(src1->index());
    instructions().append(src2->index());
    return dst;
}

void BytecodeGenerator::emitTypeProfilerExpressionInfo(const JSTextPosition& startDivot, const JSTextPosition& endDivot)
{
    ASSERT(vm()->typeProfiler());

    unsigned start = startDivot.offset; // Ranges are inclusive of their endpoints, AND 0 indexed.
    unsigned end = endDivot.offset - 1; // End Ranges already go one past the inclusive range, so subtract 1.
    unsigned instructionOffset = instructions().size() - 1;
    m_codeBlock->addTypeProfilerExpressionInfo(instructionOffset, start, end);
}

void BytecodeGenerator::emitProfileType(RegisterID* registerToProfile, ProfileTypeBytecodeFlag flag)
{
    if (!vm()->typeProfiler())
        return;

    if (!registerToProfile)
        return;

    emitOpcode(op_profile_type);
    instructions().append(registerToProfile->index());
    instructions().append(0);
    instructions().append(flag);
    instructions().append(0);
    instructions().append(resolveType());

    // Don't emit expression info for this version of profile type. This generally means
    // we're profiling information for something that isn't in the actual text of a JavaScript
    // program. For example, implicit return undefined from a function call.
}

void BytecodeGenerator::emitProfileType(RegisterID* registerToProfile, const JSTextPosition& startDivot, const JSTextPosition& endDivot)
{
    emitProfileType(registerToProfile, ProfileTypeBytecodeDoesNotHaveGlobalID, startDivot, endDivot);
}

void BytecodeGenerator::emitProfileType(RegisterID* registerToProfile, ProfileTypeBytecodeFlag flag, const JSTextPosition& startDivot, const JSTextPosition& endDivot)
{
    if (!vm()->typeProfiler())
        return;

    if (!registerToProfile)
        return;

    // The format of this instruction is: op_profile_type regToProfile, TypeLocation*, flag, identifier?, resolveType?
    emitOpcode(op_profile_type);
    instructions().append(registerToProfile->index());
    instructions().append(0);
    instructions().append(flag);
    instructions().append(0);
    instructions().append(resolveType());

    emitTypeProfilerExpressionInfo(startDivot, endDivot);
}

void BytecodeGenerator::emitProfileType(RegisterID* registerToProfile, const Variable& var, const JSTextPosition& startDivot, const JSTextPosition& endDivot)
{
    if (!vm()->typeProfiler())
        return;

    if (!registerToProfile)
        return;

    ProfileTypeBytecodeFlag flag;
    int symbolTableOrScopeDepth;
    if (var.local() || var.offset().isScope()) {
        flag = ProfileTypeBytecodeLocallyResolved;
        symbolTableOrScopeDepth = var.symbolTableConstantIndex();
    } else {
        flag = ProfileTypeBytecodeClosureVar;
        symbolTableOrScopeDepth = localScopeDepth();
    }

    // The format of this instruction is: op_profile_type regToProfile, TypeLocation*, flag, identifier?, resolveType?
    emitOpcode(op_profile_type);
    instructions().append(registerToProfile->index());
    instructions().append(symbolTableOrScopeDepth);
    instructions().append(flag);
    instructions().append(addConstant(var.ident()));
    instructions().append(resolveType());

    emitTypeProfilerExpressionInfo(startDivot, endDivot);
}

void BytecodeGenerator::emitProfileControlFlow(int textOffset)
{
    if (vm()->controlFlowProfiler()) {
        RELEASE_ASSERT(textOffset >= 0);
        size_t bytecodeOffset = instructions().size();
        m_codeBlock->addOpProfileControlFlowBytecodeOffset(bytecodeOffset);

        emitOpcode(op_profile_control_flow);
        instructions().append(textOffset);
    }
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, bool b)
{
    return emitLoad(dst, jsBoolean(b));
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, const Identifier& identifier)
{
    JSString*& stringInMap = m_stringMap.add(identifier.impl(), nullptr).iterator->value;
    if (!stringInMap)
        stringInMap = jsOwnedString(vm(), identifier.string());
    return emitLoad(dst, JSValue(stringInMap));
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, JSValue v, SourceCodeRepresentation sourceCodeRepresentation)
{
    RegisterID* constantID = addConstantValue(v, sourceCodeRepresentation);
    if (dst)
        return emitMove(dst, constantID);
    return constantID;
}

RegisterID* BytecodeGenerator::emitLoadGlobalObject(RegisterID* dst)
{
    if (!m_globalObjectRegister) {
        int index = m_nextConstantOffset;
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(JSValue());
        m_globalObjectRegister = &m_constantPoolRegisters[index];
        m_codeBlock->setGlobalObjectRegister(VirtualRegister(index));
    }
    if (dst)
        emitMove(dst, m_globalObjectRegister);
    return m_globalObjectRegister;
}

void BytecodeGenerator::pushLexicalScope(VariableEnvironmentNode* node, bool canOptimizeTDZChecks, RegisterID** constantSymbolTableResult)
{
    VariableEnvironment& environment = node->lexicalVariables();
    pushLexicalScopeInternal(environment, canOptimizeTDZChecks, constantSymbolTableResult, TDZRequirement::UnderTDZ, ScopeType::LetConstScope, ScopeRegisterType::Block);
}

void BytecodeGenerator::pushLexicalScopeInternal(VariableEnvironment& environment, bool canOptimizeTDZChecks, 
    RegisterID** constantSymbolTableResult, TDZRequirement tdzRequirement, ScopeType scopeType, ScopeRegisterType scopeRegisterType)
{
    if (!environment.size())
        return;

    if (m_shouldEmitDebugHooks)
        environment.markAllVariablesAsCaptured();

    Strong<SymbolTable> symbolTable(*m_vm, SymbolTable::create(*m_vm));
    switch (scopeType) {
    case ScopeType::CatchScope:
        symbolTable->setScopeType(SymbolTable::ScopeType::CatchScope);
        break;
    case ScopeType::LetConstScope:
        symbolTable->setScopeType(SymbolTable::ScopeType::LexicalScope);
        break;
    case ScopeType::FunctionNameScope:
        symbolTable->setScopeType(SymbolTable::ScopeType::FunctionNameScope);
        break;
    }

    bool hasCapturedVariables = false;
    {
        ConcurrentJITLocker locker(symbolTable->m_lock);
        for (auto& entry : environment) {
            ASSERT(entry.value.isLet() || entry.value.isConst());
            ASSERT(!entry.value.isVar());
            SymbolTableEntry symbolTableEntry = symbolTable->get(locker, entry.key.get());
            ASSERT(symbolTableEntry.isNull());

            VarKind varKind = entry.value.isCaptured() ? VarKind::Scope : VarKind::Stack;
            VarOffset varOffset;
            if (varKind == VarKind::Scope) {
                varOffset = VarOffset(symbolTable->takeNextScopeOffset(locker));
                hasCapturedVariables = true;
            } else {
                ASSERT(varKind == VarKind::Stack);
                RegisterID* local = newBlockScopeVariable();
                local->ref();
                varOffset = VarOffset(local->virtualRegister());
            }

            SymbolTableEntry newEntry(varOffset, entry.value.isConst() ? ReadOnly : 0);
            symbolTable->add(locker, entry.key.get(), newEntry);
        }
    }

    RegisterID* newScope = nullptr;
    RegisterID* constantSymbolTable = nullptr;
    int symbolTableConstantIndex = 0;
    if (vm()->typeProfiler()) {
        constantSymbolTable = addConstantValue(symbolTable.get());
        symbolTableConstantIndex = constantSymbolTable->index();
    }
    if (hasCapturedVariables) {
        if (scopeRegisterType == ScopeRegisterType::Block) {
            newScope = newBlockScopeVariable();
            newScope->ref();
        } else
            newScope = addVar();
        if (!constantSymbolTable) {
            ASSERT(!vm()->typeProfiler());
            constantSymbolTable = addConstantValue(symbolTable->cloneScopePart(*m_vm));
            symbolTableConstantIndex = constantSymbolTable->index();
        }
        if (constantSymbolTableResult)
            *constantSymbolTableResult = constantSymbolTable;

        emitOpcode(op_create_lexical_environment);
        instructions().append(newScope->index());
        instructions().append(scopeRegister()->index());
        instructions().append(constantSymbolTable->index());
        instructions().append(addConstantValue(tdzRequirement == TDZRequirement::UnderTDZ ? jsTDZValue() : jsUndefined())->index());

        emitMove(scopeRegister(), newScope);

        pushScopedControlFlowContext();
    }

    m_symbolTableStack.append(SymbolTableStackEntry{ symbolTable, newScope, false, symbolTableConstantIndex });
    if (tdzRequirement == TDZRequirement::UnderTDZ)
        m_TDZStack.append(std::make_pair(environment, canOptimizeTDZChecks));

    if (tdzRequirement == TDZRequirement::UnderTDZ) {
        // Prefill stack variables with the TDZ empty value.
        // Scope variables will be initialized to the TDZ empty value when JSLexicalEnvironment is allocated.
        for (auto& entry : environment) {
            SymbolTableEntry symbolTableEntry = symbolTable->get(entry.key.get());
            ASSERT(!symbolTableEntry.isNull());
            VarOffset offset = symbolTableEntry.varOffset();
            if (offset.isScope()) {
                ASSERT(newScope);
                continue;
            }
            ASSERT(offset.isStack());
            emitMoveEmptyValue(&registerFor(offset.stackOffset()));
        }
    }
}

void BytecodeGenerator::popLexicalScope(VariableEnvironmentNode* node)
{
    VariableEnvironment& environment = node->lexicalVariables();
    popLexicalScopeInternal(environment, TDZRequirement::UnderTDZ);
}

void BytecodeGenerator::popLexicalScopeInternal(VariableEnvironment& environment, TDZRequirement tdzRequirement)
{
    if (!environment.size())
        return;

    if (m_shouldEmitDebugHooks)
        environment.markAllVariablesAsCaptured();

    SymbolTableStackEntry stackEntry = m_symbolTableStack.takeLast();
    Strong<SymbolTable> symbolTable = stackEntry.m_symbolTable;
    ConcurrentJITLocker locker(symbolTable->m_lock);
    bool hasCapturedVariables = false;
    for (auto& entry : environment) {
        if (entry.value.isCaptured()) {
            hasCapturedVariables = true;
            continue;
        }
        SymbolTableEntry symbolTableEntry = symbolTable->get(locker, entry.key.get());
        ASSERT(!symbolTableEntry.isNull());
        VarOffset offset = symbolTableEntry.varOffset();
        ASSERT(offset.isStack());
        RegisterID* local = &registerFor(offset.stackOffset());
        local->deref();
    }

    if (hasCapturedVariables) {
        RELEASE_ASSERT(stackEntry.m_scope);
        emitPopScope(scopeRegister(), stackEntry.m_scope);
        popScopedControlFlowContext();
        stackEntry.m_scope->deref();
    }

    if (tdzRequirement == TDZRequirement::UnderTDZ)
        m_TDZStack.removeLast();
}

void BytecodeGenerator::prepareLexicalScopeForNextForLoopIteration(VariableEnvironmentNode* node, RegisterID* loopSymbolTable)
{
    VariableEnvironment& environment = node->lexicalVariables();
    if (!environment.size())
        return;
    if (m_shouldEmitDebugHooks)
        environment.markAllVariablesAsCaptured();
    if (!environment.hasCapturedVariables())
        return;

    RELEASE_ASSERT(loopSymbolTable);

    // This function needs to do setup for a for loop's activation if any of
    // the for loop's lexically declared variables are captured (that is, variables
    // declared in the loop header, not the loop body). This function needs to
    // make a copy of the current activation and copy the values from the previous
    // activation into the new activation because each iteration of a for loop
    // gets a new activation.

    SymbolTableStackEntry stackEntry = m_symbolTableStack.last();
    Strong<SymbolTable> symbolTable = stackEntry.m_symbolTable;
    RegisterID* loopScope = stackEntry.m_scope;
    ASSERT(symbolTable->scopeSize());
    ASSERT(loopScope);
    Vector<std::pair<RegisterID*, Identifier>> activationValuesToCopyOver;

    {
        ConcurrentJITLocker locker(symbolTable->m_lock);
        activationValuesToCopyOver.reserveInitialCapacity(symbolTable->scopeSize());

        for (auto end = symbolTable->end(locker), ptr = symbolTable->begin(locker); ptr != end; ++ptr) {
            if (!ptr->value.varOffset().isScope())
                continue;

            RefPtr<UniquedStringImpl> ident = ptr->key;
            Identifier identifier = Identifier::fromUid(m_vm, ident.get());

            RegisterID* transitionValue = newBlockScopeVariable();
            transitionValue->ref();
            emitGetFromScope(transitionValue, loopScope, variableForLocalEntry(identifier, ptr->value, loopSymbolTable->index(), true), DoNotThrowIfNotFound);
            activationValuesToCopyOver.uncheckedAppend(std::make_pair(transitionValue, identifier));
        }
    }

    // We need this dynamic behavior of the executing code to ensure
    // each loop iteration has a new activation object. (It's pretty ugly).
    // Also, this new activation needs to be assigned to the same register
    // as the previous scope because the loop body is compiled under
    // the assumption that the scope's register index is constant even
    // though the value in that register will change on each loop iteration.
    RefPtr<RegisterID> parentScope = emitGetParentScope(newTemporary(), loopScope);
    emitMove(scopeRegister(), parentScope.get());

    emitOpcode(op_create_lexical_environment);
    instructions().append(loopScope->index());
    instructions().append(scopeRegister()->index());
    instructions().append(loopSymbolTable->index());
    instructions().append(addConstantValue(jsTDZValue())->index());

    emitMove(scopeRegister(), loopScope);

    {
        ConcurrentJITLocker locker(symbolTable->m_lock);
        for (auto pair : activationValuesToCopyOver) {
            const Identifier& identifier = pair.second;
            SymbolTableEntry entry = symbolTable->get(locker, identifier.impl());
            RELEASE_ASSERT(!entry.isNull());
            RegisterID* transitionValue = pair.first;
            emitPutToScope(loopScope, variableForLocalEntry(identifier, entry, loopSymbolTable->index(), true), transitionValue, DoNotThrowIfNotFound);
            transitionValue->deref();
        }
    }
}

Variable BytecodeGenerator::variable(const Identifier& property)
{
    if (property == propertyNames().thisIdentifier) {
        return Variable(property, VarOffset(thisRegister()->virtualRegister()), thisRegister(),
            ReadOnly, Variable::SpecialVariable, 0, false);
    }
    
    // We can optimize lookups if the lexical variable is found before a "with" or "catch"
    // scope because we're guaranteed static resolution. If we have to pass through
    // a "with" or "catch" scope we loose this guarantee.
    // We can't optimize cases like this:
    // {
    //     let x = ...;
    //     with (o) {
    //         doSomethingWith(x);
    //     }
    // }
    // Because we can't gaurantee static resolution on x.
    // But, in this case, we are guaranteed static resolution:
    // {
    //     let x = ...;
    //     with (o) {
    //         let x = ...;
    //         doSomethingWith(x);
    //     }
    // }
    for (unsigned i = m_symbolTableStack.size(); i--; ) {
        SymbolTableStackEntry& stackEntry = m_symbolTableStack[i];
        if (stackEntry.m_isWithScope)
            return Variable(property);
        Strong<SymbolTable>& symbolTable = stackEntry.m_symbolTable;
        SymbolTableEntry symbolTableEntry = symbolTable->get(property.impl());
        if (symbolTableEntry.isNull())
            continue;
        if (symbolTable->scopeType() == SymbolTable::ScopeType::FunctionNameScope && m_usesNonStrictEval) {
            // We don't know if an eval has introduced a "var" named the same thing as the function name scope variable name.
            // We resort to dynamic lookup to answer this question.
            return Variable(property);
        }
        return variableForLocalEntry(property, symbolTableEntry, stackEntry.m_symbolTableConstantIndex, symbolTable->scopeType() == SymbolTable::ScopeType::LexicalScope);
    }

    return Variable(property);
}

Variable BytecodeGenerator::variableForLocalEntry(
    const Identifier& property, const SymbolTableEntry& entry, int symbolTableConstantIndex, bool isLexicallyScoped)
{
    VarOffset offset = entry.varOffset();
    
    RegisterID* local;
    if (offset.isStack())
        local = &registerFor(offset.stackOffset());
    else
        local = nullptr;
    
    return Variable(property, offset, local, entry.getAttributes(), Variable::NormalVariable, symbolTableConstantIndex, isLexicallyScoped);
}

void BytecodeGenerator::createVariable(
    const Identifier& property, VarKind varKind, SymbolTable* symbolTable, ExistingVariableMode existingVariableMode)
{
    ASSERT(property != propertyNames().thisIdentifier);
    ConcurrentJITLocker locker(symbolTable->m_lock);
    SymbolTableEntry entry = symbolTable->get(locker, property.impl());
    
    if (!entry.isNull()) {
        if (existingVariableMode == IgnoreExisting)
            return;
        
        // Do some checks to ensure that the variable we're being asked to create is sufficiently
        // compatible with the one we have already created.

        VarOffset offset = entry.varOffset();
        
        // We can't change our minds about whether it's captured.
        if (offset.kind() != varKind) {
            dataLog(
                "Trying to add variable called ", property, " as ", varKind,
                " but it was already added as ", offset, ".\n");
            RELEASE_ASSERT_NOT_REACHED();
        }

        return;
    }
    
    VarOffset varOffset;
    if (varKind == VarKind::Scope)
        varOffset = VarOffset(symbolTable->takeNextScopeOffset(locker));
    else {
        ASSERT(varKind == VarKind::Stack);
        varOffset = VarOffset(virtualRegisterForLocal(m_calleeRegisters.size()));
    }
    SymbolTableEntry newEntry(varOffset, 0);
    symbolTable->add(locker, property.impl(), newEntry);
    
    if (varKind == VarKind::Stack) {
        RegisterID* local = addVar();
        RELEASE_ASSERT(local->index() == varOffset.stackOffset().offset());
    }
}

void BytecodeGenerator::emitCheckHasInstance(RegisterID* dst, RegisterID* value, RegisterID* base, Label* target)
{
    size_t begin = instructions().size();
    emitOpcode(op_check_has_instance);
    instructions().append(dst->index());
    instructions().append(value->index());
    instructions().append(base->index());
    instructions().append(target->bind(begin, instructions().size()));
}

// Indicates the least upper bound of resolve type based on local scope. The bytecode linker
// will start with this ResolveType and compute the least upper bound including intercepting scopes.
ResolveType BytecodeGenerator::resolveType()
{
    for (unsigned i = m_symbolTableStack.size(); i--; ) {
        if (m_symbolTableStack[i].m_isWithScope)
            return Dynamic;
        if (m_usesNonStrictEval && m_symbolTableStack[i].m_symbolTable->scopeType() == SymbolTable::ScopeType::FunctionNameScope) {
            // What we really want here is something like LocalClosureVarWithVarInjectionsCheck but it's probably
            // not worth inventing just for the function name scope.
            return Dynamic;
        }
    }

    if (m_usesNonStrictEval)
        return GlobalPropertyWithVarInjectionChecks;
    return GlobalProperty;
}

RegisterID* BytecodeGenerator::emitResolveScope(RegisterID* dst, const Variable& variable)
{
    switch (variable.offset().kind()) {
    case VarKind::Stack:
        return nullptr;
        
    case VarKind::DirectArgument:
        return argumentsRegister();
        
    case VarKind::Scope:
        // This always refers to the activation that *we* allocated, and not the current scope that code
        // lives in. Note that this will change once we have proper support for block scoping. Once that
        // changes, it will be correct for this code to return scopeRegister(). The only reason why we
        // don't do that already is that m_lexicalEnvironment is required by ConstDeclNode. ConstDeclNode
        // requires weird things because it is a shameful pile of nonsense, but block scoping would make
        // that code sensible and obviate the need for us to do bad things.
        for (unsigned i = m_symbolTableStack.size(); i--; ) {
            SymbolTableStackEntry& stackEntry = m_symbolTableStack[i];
            // We should not resolve a variable to VarKind::Scope if a "with" scope lies in between the current
            // scope and the resolved scope.
            RELEASE_ASSERT(!stackEntry.m_isWithScope);

            if (stackEntry.m_symbolTable->get(variable.ident().impl()).isNull())
                continue;
            
            RegisterID* scope = stackEntry.m_scope;
            RELEASE_ASSERT(scope);
            return scope;
        }

        RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
        
    case VarKind::Invalid:
        // Indicates non-local resolution.
        
        m_codeBlock->addPropertyAccessInstruction(instructions().size());
        
        // resolve_scope dst, id, ResolveType, depth
        dst = tempDestination(dst);
        emitOpcode(op_resolve_scope);
        instructions().append(kill(dst));
        instructions().append(scopeRegister()->index());
        instructions().append(addConstant(variable.ident()));
        instructions().append(resolveType());
        instructions().append(localScopeDepth());
        instructions().append(0);
        return dst;
    }
    
    RELEASE_ASSERT_NOT_REACHED();
    return nullptr;
}

RegisterID* BytecodeGenerator::emitGetFromScope(RegisterID* dst, RegisterID* scope, const Variable& variable, ResolveMode resolveMode)
{
    switch (variable.offset().kind()) {
    case VarKind::Stack:
        return emitMove(dst, variable.local());
        
    case VarKind::DirectArgument: {
        UnlinkedValueProfile profile = emitProfiledOpcode(op_get_from_arguments);
        instructions().append(kill(dst));
        instructions().append(scope->index());
        instructions().append(variable.offset().capturedArgumentsOffset().offset());
        instructions().append(profile);
        return dst;
    }
        
    case VarKind::Scope:
    case VarKind::Invalid: {
        m_codeBlock->addPropertyAccessInstruction(instructions().size());
        
        // get_from_scope dst, scope, id, ResolveModeAndType, Structure, Operand
        UnlinkedValueProfile profile = emitProfiledOpcode(op_get_from_scope);
        instructions().append(kill(dst));
        instructions().append(scope->index());
        instructions().append(addConstant(variable.ident()));
        instructions().append(ResolveModeAndType(resolveMode, variable.offset().isScope() ? LocalClosureVar : resolveType()).operand());
        instructions().append(localScopeDepth());
        instructions().append(variable.offset().isScope() ? variable.offset().scopeOffset().offset() : 0);
        instructions().append(profile);
        return dst;
    } }
    
    RELEASE_ASSERT_NOT_REACHED();
}

RegisterID* BytecodeGenerator::emitPutToScope(RegisterID* scope, const Variable& variable, RegisterID* value, ResolveMode resolveMode)
{
    switch (variable.offset().kind()) {
    case VarKind::Stack:
        emitMove(variable.local(), value);
        return value;
        
    case VarKind::DirectArgument:
        emitOpcode(op_put_to_arguments);
        instructions().append(scope->index());
        instructions().append(variable.offset().capturedArgumentsOffset().offset());
        instructions().append(value->index());
        return value;
        
    case VarKind::Scope:
    case VarKind::Invalid: {
        m_codeBlock->addPropertyAccessInstruction(instructions().size());
        
        // put_to_scope scope, id, value, ResolveModeAndType, Structure, Operand
        emitOpcode(op_put_to_scope);
        instructions().append(scope->index());
        instructions().append(addConstant(variable.ident()));
        instructions().append(value->index());
        ScopeOffset offset;
        if (variable.offset().isScope()) {
            offset = variable.offset().scopeOffset();
            instructions().append(ResolveModeAndType(resolveMode, LocalClosureVar).operand());
            instructions().append(variable.symbolTableConstantIndex());
        } else {
            ASSERT(resolveType() != LocalClosureVar);
            instructions().append(ResolveModeAndType(resolveMode, resolveType()).operand());
            instructions().append(localScopeDepth());
        }
        instructions().append(!!offset ? offset.offset() : 0);
        return value;
    } }
    
    RELEASE_ASSERT_NOT_REACHED();
}

RegisterID* BytecodeGenerator::initializeVariable(const Variable& variable, RegisterID* value)
{
    RELEASE_ASSERT(variable.offset().kind() != VarKind::Invalid);
    RegisterID* scope = emitResolveScope(nullptr, variable);
    return emitPutToScope(scope, variable, value, ThrowIfNotFound);
}

RegisterID* BytecodeGenerator::emitInstanceOf(RegisterID* dst, RegisterID* value, RegisterID* basePrototype)
{
    emitOpcode(op_instanceof);
    instructions().append(dst->index());
    instructions().append(value->index());
    instructions().append(basePrototype->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitGetById(RegisterID* dst, RegisterID* base, const Identifier& property)
{
    m_codeBlock->addPropertyAccessInstruction(instructions().size());

    UnlinkedValueProfile profile = emitProfiledOpcode(op_get_by_id);
    instructions().append(kill(dst));
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitPutById(RegisterID* base, const Identifier& property, RegisterID* value)
{
    unsigned propertyIndex = addConstant(property);

    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    m_codeBlock->addPropertyAccessInstruction(instructions().size());

    emitOpcode(op_put_by_id);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(value->index());
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);

    return value;
}

RegisterID* BytecodeGenerator::emitDirectPutById(RegisterID* base, const Identifier& property, RegisterID* value, PropertyNode::PutType putType)
{
    ASSERT(!parseIndex(property));
    unsigned propertyIndex = addConstant(property);

    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    m_codeBlock->addPropertyAccessInstruction(instructions().size());
    
    emitOpcode(op_put_by_id);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(value->index());
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(putType == PropertyNode::KnownDirect || property != m_vm->propertyNames->underscoreProto);
    return value;
}

void BytecodeGenerator::emitPutGetterById(RegisterID* base, const Identifier& property, RegisterID* getter)
{
    unsigned propertyIndex = addConstant(property);
    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    emitOpcode(op_put_getter_by_id);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(getter->index());
}

void BytecodeGenerator::emitPutSetterById(RegisterID* base, const Identifier& property, RegisterID* setter)
{
    unsigned propertyIndex = addConstant(property);
    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    emitOpcode(op_put_setter_by_id);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(setter->index());
}

void BytecodeGenerator::emitPutGetterSetter(RegisterID* base, const Identifier& property, RegisterID* getter, RegisterID* setter)
{
    unsigned propertyIndex = addConstant(property);

    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    emitOpcode(op_put_getter_setter);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(getter->index());
    instructions().append(setter->index());
}

RegisterID* BytecodeGenerator::emitDeleteById(RegisterID* dst, RegisterID* base, const Identifier& property)
{
    emitOpcode(op_del_by_id);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(addConstant(property));
    return dst;
}

RegisterID* BytecodeGenerator::emitGetByVal(RegisterID* dst, RegisterID* base, RegisterID* property)
{
    for (size_t i = m_forInContextStack.size(); i > 0; i--) {
        ForInContext* context = m_forInContextStack[i - 1].get();
        if (context->local() != property)
            continue;

        if (!context->isValid())
            break;

        if (context->type() == ForInContext::IndexedForInContextType) {
            property = static_cast<IndexedForInContext*>(context)->index();
            break;
        }

        ASSERT(context->type() == ForInContext::StructureForInContextType);
        StructureForInContext* structureContext = static_cast<StructureForInContext*>(context);
        UnlinkedValueProfile profile = emitProfiledOpcode(op_get_direct_pname);
        instructions().append(kill(dst));
        instructions().append(base->index());
        instructions().append(property->index());
        instructions().append(structureContext->index()->index());
        instructions().append(structureContext->enumerator()->index());
        instructions().append(profile);
        return dst;
    }

    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    UnlinkedValueProfile profile = emitProfiledOpcode(op_get_by_val);
    instructions().append(kill(dst));
    instructions().append(base->index());
    instructions().append(property->index());
    instructions().append(arrayProfile);
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitPutByVal(RegisterID* base, RegisterID* property, RegisterID* value)
{
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    emitOpcode(op_put_by_val);
    instructions().append(base->index());
    instructions().append(property->index());
    instructions().append(value->index());
    instructions().append(arrayProfile);

    return value;
}

RegisterID* BytecodeGenerator::emitDirectPutByVal(RegisterID* base, RegisterID* property, RegisterID* value)
{
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    emitOpcode(op_put_by_val_direct);
    instructions().append(base->index());
    instructions().append(property->index());
    instructions().append(value->index());
    instructions().append(arrayProfile);
    return value;
}

RegisterID* BytecodeGenerator::emitDeleteByVal(RegisterID* dst, RegisterID* base, RegisterID* property)
{
    emitOpcode(op_del_by_val);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(property->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitPutByIndex(RegisterID* base, unsigned index, RegisterID* value)
{
    emitOpcode(op_put_by_index);
    instructions().append(base->index());
    instructions().append(index);
    instructions().append(value->index());
    return value;
}

RegisterID* BytecodeGenerator::emitCreateThis(RegisterID* dst)
{
    size_t begin = instructions().size();
    m_staticPropertyAnalyzer.createThis(m_thisRegister.index(), begin + 3);

    m_codeBlock->addPropertyAccessInstruction(instructions().size());
    emitOpcode(op_create_this); 
    instructions().append(m_thisRegister.index()); 
    instructions().append(m_thisRegister.index()); 
    instructions().append(0);
    instructions().append(0);
    return dst;
}

void BytecodeGenerator::emitTDZCheck(RegisterID* target)
{
    emitOpcode(op_check_tdz);
    instructions().append(target->index());
}

bool BytecodeGenerator::needsTDZCheck(const Variable& variable)
{
    for (unsigned i = m_TDZStack.size(); i--;) {
        VariableEnvironment& identifiers = m_TDZStack[i].first;
        if (identifiers.contains(variable.ident().impl()))
            return true;
    }

    return false;
}

void BytecodeGenerator::emitTDZCheckIfNecessary(const Variable& variable, RegisterID* target, RegisterID* scope)
{
    if (needsTDZCheck(variable)) {
        if (target)
            emitTDZCheck(target);
        else {
            RELEASE_ASSERT(!variable.isLocal() && scope);
            RefPtr<RegisterID> result = emitGetFromScope(newTemporary(), scope, variable, DoNotThrowIfNotFound);
            emitTDZCheck(result.get());
        }
    }
}

void BytecodeGenerator::liftTDZCheckIfPossible(const Variable& variable)
{
    RefPtr<UniquedStringImpl> identifier(variable.ident().impl());
    for (unsigned i = m_TDZStack.size(); i--;) {
        VariableEnvironment& environment = m_TDZStack[i].first;
        if (environment.contains(identifier)) {
            bool isSyntacticallyAbleToOptimizeTDZ = m_TDZStack[i].second;
            if (isSyntacticallyAbleToOptimizeTDZ) {
                bool wasRemoved = environment.remove(identifier);
                RELEASE_ASSERT(wasRemoved);
            }
            break;
        }
    }
}

void BytecodeGenerator::getVariablesUnderTDZ(VariableEnvironment& result)
{
    for (auto& pair : m_TDZStack) {
        VariableEnvironment& environment = pair.first;
        for (auto entry : environment)
            result.add(entry.key.get());
    }
}

RegisterID* BytecodeGenerator::emitNewObject(RegisterID* dst)
{
    size_t begin = instructions().size();
    m_staticPropertyAnalyzer.newObject(dst->index(), begin + 2);

    emitOpcode(op_new_object);
    instructions().append(dst->index());
    instructions().append(0);
    instructions().append(newObjectAllocationProfile());
    return dst;
}

unsigned BytecodeGenerator::addConstantBuffer(unsigned length)
{
    return m_codeBlock->addConstantBuffer(length);
}

JSString* BytecodeGenerator::addStringConstant(const Identifier& identifier)
{
    JSString*& stringInMap = m_stringMap.add(identifier.impl(), nullptr).iterator->value;
    if (!stringInMap) {
        stringInMap = jsString(vm(), identifier.string());
        addConstantValue(stringInMap);
    }
    return stringInMap;
}

JSTemplateRegistryKey* BytecodeGenerator::addTemplateRegistryKeyConstant(const TemplateRegistryKey& templateRegistryKey)
{
    JSTemplateRegistryKey*& templateRegistryKeyInMap = m_templateRegistryKeyMap.add(templateRegistryKey, nullptr).iterator->value;
    if (!templateRegistryKeyInMap) {
        templateRegistryKeyInMap = JSTemplateRegistryKey::create(*vm(), templateRegistryKey);
        addConstantValue(templateRegistryKeyInMap);
    }
    return templateRegistryKeyInMap;
}

RegisterID* BytecodeGenerator::emitNewArray(RegisterID* dst, ElementNode* elements, unsigned length)
{
#if !ASSERT_DISABLED
    unsigned checkLength = 0;
#endif
    bool hadVariableExpression = false;
    if (length) {
        for (ElementNode* n = elements; n; n = n->next()) {
            if (!n->value()->isConstant()) {
                hadVariableExpression = true;
                break;
            }
            if (n->elision())
                break;
#if !ASSERT_DISABLED
            checkLength++;
#endif
        }
        if (!hadVariableExpression) {
            ASSERT(length == checkLength);
            unsigned constantBufferIndex = addConstantBuffer(length);
            JSValue* constantBuffer = m_codeBlock->constantBuffer(constantBufferIndex).data();
            unsigned index = 0;
            for (ElementNode* n = elements; index < length; n = n->next()) {
                ASSERT(n->value()->isConstant());
                constantBuffer[index++] = static_cast<ConstantNode*>(n->value())->jsValue(*this);
            }
            emitOpcode(op_new_array_buffer);
            instructions().append(dst->index());
            instructions().append(constantBufferIndex);
            instructions().append(length);
            instructions().append(newArrayAllocationProfile());
            return dst;
        }
    }

    Vector<RefPtr<RegisterID>, 16, UnsafeVectorOverflow> argv;
    for (ElementNode* n = elements; n; n = n->next()) {
        if (!length)
            break;
        length--;
        ASSERT(!n->value()->isSpreadExpression());
        argv.append(newTemporary());
        // op_new_array requires the initial values to be a sequential range of registers
        ASSERT(argv.size() == 1 || argv[argv.size() - 1]->index() == argv[argv.size() - 2]->index() - 1);
        emitNode(argv.last().get(), n->value());
    }
    ASSERT(!length);
    emitOpcode(op_new_array);
    instructions().append(dst->index());
    instructions().append(argv.size() ? argv[0]->index() : 0); // argv
    instructions().append(argv.size()); // argc
    instructions().append(newArrayAllocationProfile());
    return dst;
}

RegisterID* BytecodeGenerator::emitNewFunction(RegisterID* dst, FunctionMetadataNode* function)
{
    return emitNewFunctionInternal(dst, m_codeBlock->addFunctionDecl(makeFunction(function)));
}

RegisterID* BytecodeGenerator::emitNewFunctionInternal(RegisterID* dst, unsigned index)
{
    emitOpcode(op_new_func);
    instructions().append(dst->index());
    instructions().append(scopeRegister()->index());
    instructions().append(index);
    return dst;
}

RegisterID* BytecodeGenerator::emitNewRegExp(RegisterID* dst, RegExp* regExp)
{
    emitOpcode(op_new_regexp);
    instructions().append(dst->index());
    instructions().append(addRegExp(regExp));
    return dst;
}

RegisterID* BytecodeGenerator::emitNewFunctionExpression(RegisterID* r0, FuncExprNode* n)
{
    FunctionMetadataNode* metadata = n->metadata();
    unsigned index = m_codeBlock->addFunctionExpr(makeFunction(metadata));

    emitOpcode(op_new_func_exp);
    instructions().append(r0->index());
    instructions().append(scopeRegister()->index());
    instructions().append(index);
    return r0;
}

RegisterID* BytecodeGenerator::emitNewDefaultConstructor(RegisterID* dst, ConstructorKind constructorKind, const Identifier& name)
{
    UnlinkedFunctionExecutable* executable = m_vm->builtinExecutables()->createDefaultConstructor(constructorKind, name);

    unsigned index = m_codeBlock->addFunctionExpr(executable);

    emitOpcode(op_new_func_exp);
    instructions().append(dst->index());
    instructions().append(scopeRegister()->index());
    instructions().append(index);
    return dst;
}

RegisterID* BytecodeGenerator::emitCall(RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    return emitCall(op_call, dst, func, expectedFunction, callArguments, divot, divotStart, divotEnd);
}

RegisterID* BytecodeGenerator::emitCallEval(RegisterID* dst, RegisterID* func, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    return emitCall(op_call_eval, dst, func, NoExpectedFunction, callArguments, divot, divotStart, divotEnd);
}

ExpectedFunction BytecodeGenerator::expectedFunctionForIdentifier(const Identifier& identifier)
{
    if (identifier == m_vm->propertyNames->Object || identifier == m_vm->propertyNames->ObjectPrivateName)
        return ExpectObjectConstructor;
    if (identifier == m_vm->propertyNames->Array || identifier == m_vm->propertyNames->ArrayPrivateName)
        return ExpectArrayConstructor;
    return NoExpectedFunction;
}

ExpectedFunction BytecodeGenerator::emitExpectedFunctionSnippet(RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, Label* done)
{
    RefPtr<Label> realCall = newLabel();
    switch (expectedFunction) {
    case ExpectObjectConstructor: {
        // If the number of arguments is non-zero, then we can't do anything interesting.
        if (callArguments.argumentCountIncludingThis() >= 2)
            return NoExpectedFunction;
        
        size_t begin = instructions().size();
        emitOpcode(op_jneq_ptr);
        instructions().append(func->index());
        instructions().append(Special::ObjectConstructor);
        instructions().append(realCall->bind(begin, instructions().size()));
        
        if (dst != ignoredResult())
            emitNewObject(dst);
        break;
    }
        
    case ExpectArrayConstructor: {
        // If you're doing anything other than "new Array()" or "new Array(foo)" then we
        // don't do inline it, for now. The only reason is that call arguments are in
        // the opposite order of what op_new_array expects, so we'd either need to change
        // how op_new_array works or we'd need an op_new_array_reverse. Neither of these
        // things sounds like it's worth it.
        if (callArguments.argumentCountIncludingThis() > 2)
            return NoExpectedFunction;
        
        size_t begin = instructions().size();
        emitOpcode(op_jneq_ptr);
        instructions().append(func->index());
        instructions().append(Special::ArrayConstructor);
        instructions().append(realCall->bind(begin, instructions().size()));
        
        if (dst != ignoredResult()) {
            if (callArguments.argumentCountIncludingThis() == 2) {
                emitOpcode(op_new_array_with_size);
                instructions().append(dst->index());
                instructions().append(callArguments.argumentRegister(0)->index());
                instructions().append(newArrayAllocationProfile());
            } else {
                ASSERT(callArguments.argumentCountIncludingThis() == 1);
                emitOpcode(op_new_array);
                instructions().append(dst->index());
                instructions().append(0);
                instructions().append(0);
                instructions().append(newArrayAllocationProfile());
            }
        }
        break;
    }
        
    default:
        ASSERT(expectedFunction == NoExpectedFunction);
        return NoExpectedFunction;
    }
    
    size_t begin = instructions().size();
    emitOpcode(op_jmp);
    instructions().append(done->bind(begin, instructions().size()));
    emitLabel(realCall.get());
    
    return expectedFunction;
}

RegisterID* BytecodeGenerator::emitCall(OpcodeID opcodeID, RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    ASSERT(opcodeID == op_call || opcodeID == op_call_eval);
    ASSERT(func->refCount());

    if (m_shouldEmitProfileHooks)
        emitMove(callArguments.profileHookRegister(), func);

    // Generate code for arguments.
    unsigned argument = 0;
    if (callArguments.argumentsNode()) {
        ArgumentListNode* n = callArguments.argumentsNode()->m_listNode;
        if (n && n->m_expr->isSpreadExpression()) {
            RELEASE_ASSERT(!n->m_next);
            auto expression = static_cast<SpreadExpressionNode*>(n->m_expr)->expression();
            RefPtr<RegisterID> argumentRegister;
            argumentRegister = expression->emitBytecode(*this, callArguments.argumentRegister(0));
            RefPtr<RegisterID> thisRegister = emitMove(newTemporary(), callArguments.thisRegister());
            return emitCallVarargs(dst, func, callArguments.thisRegister(), argumentRegister.get(), newTemporary(), 0, callArguments.profileHookRegister(), divot, divotStart, divotEnd);
        }
        for (; n; n = n->m_next)
            emitNode(callArguments.argumentRegister(argument++), n);
    }
    
    // Reserve space for call frame.
    Vector<RefPtr<RegisterID>, JSStack::CallFrameHeaderSize, UnsafeVectorOverflow> callFrame;
    for (int i = 0; i < JSStack::CallFrameHeaderSize; ++i)
        callFrame.append(newTemporary());

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_will_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    emitExpressionInfo(divot, divotStart, divotEnd);

    RefPtr<Label> done = newLabel();
    expectedFunction = emitExpectedFunctionSnippet(dst, func, expectedFunction, callArguments, done.get());
    
    // Emit call.
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    UnlinkedValueProfile profile = emitProfiledOpcode(opcodeID);
    ASSERT(dst);
    ASSERT(dst != ignoredResult());
    instructions().append(dst->index());
    instructions().append(func->index());
    instructions().append(callArguments.argumentCountIncludingThis());
    instructions().append(callArguments.stackOffset());
    instructions().append(m_codeBlock->addLLIntCallLinkInfo());
    instructions().append(0);
    instructions().append(arrayProfile);
    instructions().append(profile);
    
    if (expectedFunction != NoExpectedFunction)
        emitLabel(done.get());

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    return dst;
}

RegisterID* BytecodeGenerator::emitCallVarargs(RegisterID* dst, RegisterID* func, RegisterID* thisRegister, RegisterID* arguments, RegisterID* firstFreeRegister, int32_t firstVarArgOffset, RegisterID* profileHookRegister, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    return emitCallVarargs(op_call_varargs, dst, func, thisRegister, arguments, firstFreeRegister, firstVarArgOffset, profileHookRegister, divot, divotStart, divotEnd);
}

RegisterID* BytecodeGenerator::emitConstructVarargs(RegisterID* dst, RegisterID* func, RegisterID* thisRegister, RegisterID* arguments, RegisterID* firstFreeRegister, int32_t firstVarArgOffset, RegisterID* profileHookRegister, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    return emitCallVarargs(op_construct_varargs, dst, func, thisRegister, arguments, firstFreeRegister, firstVarArgOffset, profileHookRegister, divot, divotStart, divotEnd);
}
    
RegisterID* BytecodeGenerator::emitCallVarargs(OpcodeID opcode, RegisterID* dst, RegisterID* func, RegisterID* thisRegister, RegisterID* arguments, RegisterID* firstFreeRegister, int32_t firstVarArgOffset, RegisterID* profileHookRegister, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    if (m_shouldEmitProfileHooks) {
        emitMove(profileHookRegister, func);
        emitOpcode(op_profile_will_call);
        instructions().append(profileHookRegister->index());
    }
    
    emitExpressionInfo(divot, divotStart, divotEnd);

    // Emit call.
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    UnlinkedValueProfile profile = emitProfiledOpcode(opcode);
    ASSERT(dst != ignoredResult());
    instructions().append(dst->index());
    instructions().append(func->index());
    instructions().append(thisRegister ? thisRegister->index() : 0);
    instructions().append(arguments->index());
    instructions().append(firstFreeRegister->index());
    instructions().append(firstVarArgOffset);
    instructions().append(arrayProfile);
    instructions().append(profile);
    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(profileHookRegister->index());
    }
    return dst;
}

void BytecodeGenerator::emitCallDefineProperty(RegisterID* newObj, RegisterID* propertyNameRegister,
    RegisterID* valueRegister, RegisterID* getterRegister, RegisterID* setterRegister, unsigned options, const JSTextPosition& position)
{
    RefPtr<RegisterID> descriptorRegister = emitNewObject(newTemporary());

    RefPtr<RegisterID> trueRegister = emitLoad(newTemporary(), true);
    if (options & PropertyConfigurable)
        emitDirectPutById(descriptorRegister.get(), propertyNames().configurable, trueRegister.get(), PropertyNode::Unknown);
    if (options & PropertyWritable)
        emitDirectPutById(descriptorRegister.get(), propertyNames().writable, trueRegister.get(), PropertyNode::Unknown);
    else if (valueRegister) {
        RefPtr<RegisterID> falseRegister = emitLoad(newTemporary(), false);
        emitDirectPutById(descriptorRegister.get(), propertyNames().writable, falseRegister.get(), PropertyNode::Unknown);
    }
    if (options & PropertyEnumerable)
        emitDirectPutById(descriptorRegister.get(), propertyNames().enumerable, trueRegister.get(), PropertyNode::Unknown);

    if (valueRegister)
        emitDirectPutById(descriptorRegister.get(), propertyNames().value, valueRegister, PropertyNode::Unknown);
    if (getterRegister)
        emitDirectPutById(descriptorRegister.get(), propertyNames().get, getterRegister, PropertyNode::Unknown);
    if (setterRegister)
        emitDirectPutById(descriptorRegister.get(), propertyNames().set, setterRegister, PropertyNode::Unknown);

    RefPtr<RegisterID> definePropertyRegister = emitMoveLinkTimeConstant(newTemporary(), LinkTimeConstant::DefinePropertyFunction);

    CallArguments callArguments(*this, nullptr, 3);
    emitLoad(callArguments.thisRegister(), jsUndefined());
    emitMove(callArguments.argumentRegister(0), newObj);
    emitMove(callArguments.argumentRegister(1), propertyNameRegister);
    emitMove(callArguments.argumentRegister(2), descriptorRegister.get());

    emitCall(newTemporary(), definePropertyRegister.get(), NoExpectedFunction, callArguments, position, position, position);
}

RegisterID* BytecodeGenerator::emitReturn(RegisterID* src)
{
    if (isConstructor()) {
        bool derived = constructorKind() == ConstructorKind::Derived;
        if (derived && src->index() == m_thisRegister.index())
            emitTDZCheck(src);

        RefPtr<Label> isObjectLabel = newLabel();
        emitJumpIfTrue(emitIsObject(newTemporary(), src), isObjectLabel.get());

        if (derived) {
            RefPtr<Label> isUndefinedLabel = newLabel();
            emitJumpIfTrue(emitIsUndefined(newTemporary(), src), isUndefinedLabel.get());
            emitThrowTypeError("Cannot return a non-object type in the constructor of a derived class.");
            emitLabel(isUndefinedLabel.get());
            if (constructorKind() == ConstructorKind::Derived)
                emitTDZCheck(&m_thisRegister);
        }

        emitUnaryNoDstOp(op_ret, &m_thisRegister);

        emitLabel(isObjectLabel.get());
    }

    return emitUnaryNoDstOp(op_ret, src);
}

RegisterID* BytecodeGenerator::emitUnaryNoDstOp(OpcodeID opcodeID, RegisterID* src)
{
    emitOpcode(opcodeID);
    instructions().append(src->index());
    return src;
}

RegisterID* BytecodeGenerator::emitConstruct(RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    ASSERT(func->refCount());

    if (m_shouldEmitProfileHooks)
        emitMove(callArguments.profileHookRegister(), func);

    // Generate code for arguments.
    unsigned argument = 0;
    if (ArgumentsNode* argumentsNode = callArguments.argumentsNode()) {
        
        ArgumentListNode* n = callArguments.argumentsNode()->m_listNode;
        if (n && n->m_expr->isSpreadExpression()) {
            RELEASE_ASSERT(!n->m_next);
            auto expression = static_cast<SpreadExpressionNode*>(n->m_expr)->expression();
            RefPtr<RegisterID> argumentRegister;
            argumentRegister = expression->emitBytecode(*this, callArguments.argumentRegister(0));
            return emitConstructVarargs(dst, func, callArguments.thisRegister(), argumentRegister.get(), newTemporary(), 0, callArguments.profileHookRegister(), divot, divotStart, divotEnd);
        }
        
        for (ArgumentListNode* n = argumentsNode->m_listNode; n; n = n->m_next)
            emitNode(callArguments.argumentRegister(argument++), n);
    }

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_will_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    // Reserve space for call frame.
    Vector<RefPtr<RegisterID>, JSStack::CallFrameHeaderSize, UnsafeVectorOverflow> callFrame;
    for (int i = 0; i < JSStack::CallFrameHeaderSize; ++i)
        callFrame.append(newTemporary());

    emitExpressionInfo(divot, divotStart, divotEnd);
    
    RefPtr<Label> done = newLabel();
    expectedFunction = emitExpectedFunctionSnippet(dst, func, expectedFunction, callArguments, done.get());

    UnlinkedValueProfile profile = emitProfiledOpcode(op_construct);
    ASSERT(dst != ignoredResult());
    instructions().append(dst->index());
    instructions().append(func->index());
    instructions().append(callArguments.argumentCountIncludingThis());
    instructions().append(callArguments.stackOffset());
    instructions().append(m_codeBlock->addLLIntCallLinkInfo());
    instructions().append(0);
    instructions().append(0);
    instructions().append(profile);

    if (expectedFunction != NoExpectedFunction)
        emitLabel(done.get());

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    return dst;
}

RegisterID* BytecodeGenerator::emitStrcat(RegisterID* dst, RegisterID* src, int count)
{
    emitOpcode(op_strcat);
    instructions().append(dst->index());
    instructions().append(src->index());
    instructions().append(count);

    return dst;
}

void BytecodeGenerator::emitToPrimitive(RegisterID* dst, RegisterID* src)
{
    emitOpcode(op_to_primitive);
    instructions().append(dst->index());
    instructions().append(src->index());
}

void BytecodeGenerator::emitGetScope()
{
    emitOpcode(op_get_scope);
    instructions().append(scopeRegister()->index());
}

RegisterID* BytecodeGenerator::emitPushWithScope(RegisterID* objectScope)
{
    pushScopedControlFlowContext();
    RegisterID* newScope = newBlockScopeVariable();
    newScope->ref();

    emitOpcode(op_push_with_scope);
    instructions().append(newScope->index());
    instructions().append(objectScope->index());
    instructions().append(scopeRegister()->index());

    emitMove(scopeRegister(), newScope);
    m_symbolTableStack.append(SymbolTableStackEntry{ Strong<SymbolTable>(), newScope, true, 0 });

    return newScope;
}

RegisterID* BytecodeGenerator::emitGetParentScope(RegisterID* dst, RegisterID* scope)
{
    emitOpcode(op_get_parent_scope);
    instructions().append(dst->index());
    instructions().append(scope->index());
    return dst;
}

void BytecodeGenerator::emitPopScope(RegisterID* dst, RegisterID* scope)
{
    RefPtr<RegisterID> parentScope = emitGetParentScope(newTemporary(), scope);
    emitMove(dst, parentScope.get());
}

void BytecodeGenerator::emitPopWithScope()
{
    emitPopScope(scopeRegister(), scopeRegister());
    popScopedControlFlowContext();
    SymbolTableStackEntry stackEntry = m_symbolTableStack.takeLast();
    stackEntry.m_scope->deref();
    RELEASE_ASSERT(stackEntry.m_isWithScope);
}

void BytecodeGenerator::emitDebugHook(DebugHookID debugHookID, unsigned line, unsigned charOffset, unsigned lineStart)
{
#if ENABLE(DEBUG_WITH_BREAKPOINT)
    if (debugHookID != DidReachBreakpoint)
        return;
#else
    if (!m_shouldEmitDebugHooks)
        return;
#endif
    JSTextPosition divot(line, charOffset, lineStart);
    emitExpressionInfo(divot, divot, divot);
    emitOpcode(op_debug);
    instructions().append(debugHookID);
    instructions().append(false);
}

void BytecodeGenerator::pushFinallyContext(StatementNode* finallyBlock)
{
    // Reclaim free label scopes.
    while (m_labelScopes.size() && !m_labelScopes.last().refCount())
        m_labelScopes.removeLast();

    ControlFlowContext scope;
    scope.isFinallyBlock = true;
    FinallyContext context = {
        finallyBlock,
        nullptr,
        nullptr,
        static_cast<unsigned>(m_scopeContextStack.size()),
        static_cast<unsigned>(m_switchContextStack.size()),
        static_cast<unsigned>(m_forInContextStack.size()),
        static_cast<unsigned>(m_tryContextStack.size()),
        static_cast<unsigned>(m_labelScopes.size()),
        static_cast<unsigned>(m_symbolTableStack.size()),
        m_finallyDepth,
        m_localScopeDepth
    };
    scope.finallyContext = context;
    m_scopeContextStack.append(scope);
    m_finallyDepth++;
}

void BytecodeGenerator::pushIteratorCloseContext(RegisterID* iterator, ThrowableExpressionData* node)
{
    // Reclaim free label scopes.
    while (m_labelScopes.size() && !m_labelScopes.last().refCount())
        m_labelScopes.removeLast();

    ControlFlowContext scope;
    scope.isFinallyBlock = true;
    FinallyContext context = {
        nullptr,
        iterator,
        node,
        static_cast<unsigned>(m_scopeContextStack.size()),
        static_cast<unsigned>(m_switchContextStack.size()),
        static_cast<unsigned>(m_forInContextStack.size()),
        static_cast<unsigned>(m_tryContextStack.size()),
        static_cast<unsigned>(m_labelScopes.size()),
        static_cast<unsigned>(m_symbolTableStack.size()),
        m_finallyDepth,
        m_localScopeDepth
    };
    scope.finallyContext = context;
    m_scopeContextStack.append(scope);
    m_finallyDepth++;
}

void BytecodeGenerator::popFinallyContext()
{
    ASSERT(m_scopeContextStack.size());
    ASSERT(m_scopeContextStack.last().isFinallyBlock);
    ASSERT(m_scopeContextStack.last().finallyContext.finallyBlock);
    ASSERT(!m_scopeContextStack.last().finallyContext.iterator);
    ASSERT(!m_scopeContextStack.last().finallyContext.enumerationNode);
    ASSERT(m_finallyDepth > 0);
    m_scopeContextStack.removeLast();
    m_finallyDepth--;
}

void BytecodeGenerator::popIteratorCloseContext()
{
    ASSERT(m_scopeContextStack.size());
    ASSERT(m_scopeContextStack.last().isFinallyBlock);
    ASSERT(!m_scopeContextStack.last().finallyContext.finallyBlock);
    ASSERT(m_scopeContextStack.last().finallyContext.iterator);
    ASSERT(m_scopeContextStack.last().finallyContext.enumerationNode);
    ASSERT(m_finallyDepth > 0);
    m_scopeContextStack.removeLast();
    m_finallyDepth--;
}

LabelScopePtr BytecodeGenerator::breakTarget(const Identifier& name)
{
    // Reclaim free label scopes.
    //
    // The condition was previously coded as 'm_labelScopes.size() && !m_labelScopes.last().refCount()',
    // however sometimes this appears to lead to GCC going a little haywire and entering the loop with
    // size 0, leading to segfaulty badness.  We are yet to identify a valid cause within our code to
    // cause the GCC codegen to misbehave in this fashion, and as such the following refactoring of the
    // loop condition is a workaround.
    while (m_labelScopes.size()) {
        if  (m_labelScopes.last().refCount())
            break;
        m_labelScopes.removeLast();
    }

    if (!m_labelScopes.size())
        return LabelScopePtr::null();

    // We special-case the following, which is a syntax error in Firefox:
    // label:
    //     break;
    if (name.isEmpty()) {
        for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
            LabelScope* scope = &m_labelScopes[i];
            if (scope->type() != LabelScope::NamedLabel) {
                ASSERT(scope->breakTarget());
                return LabelScopePtr(m_labelScopes, i);
            }
        }
        return LabelScopePtr::null();
    }

    for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
        LabelScope* scope = &m_labelScopes[i];
        if (scope->name() && *scope->name() == name) {
            ASSERT(scope->breakTarget());
            return LabelScopePtr(m_labelScopes, i);
        }
    }
    return LabelScopePtr::null();
}

LabelScopePtr BytecodeGenerator::continueTarget(const Identifier& name)
{
    // Reclaim free label scopes.
    while (m_labelScopes.size() && !m_labelScopes.last().refCount())
        m_labelScopes.removeLast();

    if (!m_labelScopes.size())
        return LabelScopePtr::null();

    if (name.isEmpty()) {
        for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
            LabelScope* scope = &m_labelScopes[i];
            if (scope->type() == LabelScope::Loop) {
                ASSERT(scope->continueTarget());
                return LabelScopePtr(m_labelScopes, i);
            }
        }
        return LabelScopePtr::null();
    }

    // Continue to the loop nested nearest to the label scope that matches
    // 'name'.
    LabelScopePtr result = LabelScopePtr::null();
    for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
        LabelScope* scope = &m_labelScopes[i];
        if (scope->type() == LabelScope::Loop) {
            ASSERT(scope->continueTarget());
            result = LabelScopePtr(m_labelScopes, i);
        }
        if (scope->name() && *scope->name() == name)
            return result; // may be null.
    }
    return LabelScopePtr::null();
}

void BytecodeGenerator::allocateAndEmitScope()
{
    m_scopeRegister = addVar();
    m_scopeRegister->ref();
    m_codeBlock->setScopeRegister(scopeRegister()->virtualRegister());
    emitGetScope();
    m_topMostScope = addVar();
    emitMove(m_topMostScope, scopeRegister());
}

void BytecodeGenerator::emitComplexPopScopes(RegisterID* scope, ControlFlowContext* topScope, ControlFlowContext* bottomScope)
{
    while (topScope > bottomScope) {
        // First we count the number of dynamic scopes we need to remove to get
        // to a finally block.
        int nNormalScopes = 0;
        while (topScope > bottomScope) {
            if (topScope->isFinallyBlock)
                break;
            ++nNormalScopes;
            --topScope;
        }

        if (nNormalScopes) {
            // We need to remove a number of dynamic scopes to get to the next
            // finally block
            RefPtr<RegisterID> parentScope = newTemporary();
            while (nNormalScopes--) {
                parentScope = emitGetParentScope(parentScope.get(), scope);
                emitMove(scope, parentScope.get());
            }

            // If topScope == bottomScope then there isn't a finally block left to emit.
            if (topScope == bottomScope)
                return;
        }
        
        Vector<ControlFlowContext> savedScopeContextStack;
        Vector<SwitchInfo> savedSwitchContextStack;
        Vector<std::unique_ptr<ForInContext>> savedForInContextStack;
        Vector<TryContext> poppedTryContexts;
        Vector<SymbolTableStackEntry> savedSymbolTableStack;
        LabelScopeStore savedLabelScopes;
        while (topScope > bottomScope && topScope->isFinallyBlock) {
            RefPtr<Label> beforeFinally = emitLabel(newLabel().get());
            
            // Save the current state of the world while instating the state of the world
            // for the finally block.
            FinallyContext finallyContext = topScope->finallyContext;
            bool flipScopes = finallyContext.scopeContextStackSize != m_scopeContextStack.size();
            bool flipSwitches = finallyContext.switchContextStackSize != m_switchContextStack.size();
            bool flipForIns = finallyContext.forInContextStackSize != m_forInContextStack.size();
            bool flipTries = finallyContext.tryContextStackSize != m_tryContextStack.size();
            bool flipLabelScopes = finallyContext.labelScopesSize != m_labelScopes.size();
            bool flipSymbolTableStack = finallyContext.symbolTableStackSize != m_symbolTableStack.size();
            int topScopeIndex = -1;
            int bottomScopeIndex = -1;
            if (flipScopes) {
                topScopeIndex = topScope - m_scopeContextStack.begin();
                bottomScopeIndex = bottomScope - m_scopeContextStack.begin();
                savedScopeContextStack = m_scopeContextStack;
                m_scopeContextStack.shrink(finallyContext.scopeContextStackSize);
            }
            if (flipSwitches) {
                savedSwitchContextStack = m_switchContextStack;
                m_switchContextStack.shrink(finallyContext.switchContextStackSize);
            }
            if (flipForIns) {
                savedForInContextStack.swap(m_forInContextStack);
                m_forInContextStack.shrink(finallyContext.forInContextStackSize);
            }
            if (flipTries) {
                while (m_tryContextStack.size() != finallyContext.tryContextStackSize) {
                    ASSERT(m_tryContextStack.size() > finallyContext.tryContextStackSize);
                    TryContext context = m_tryContextStack.last();
                    m_tryContextStack.removeLast();
                    TryRange range;
                    range.start = context.start;
                    range.end = beforeFinally;
                    range.tryData = context.tryData;
                    m_tryRanges.append(range);
                    poppedTryContexts.append(context);
                }
            }
            if (flipLabelScopes) {
                savedLabelScopes = m_labelScopes;
                while (m_labelScopes.size() > finallyContext.labelScopesSize)
                    m_labelScopes.removeLast();
            }
            if (flipSymbolTableStack) {
                savedSymbolTableStack = m_symbolTableStack;
                m_symbolTableStack.shrink(finallyContext.symbolTableStackSize);
            }
            int savedFinallyDepth = m_finallyDepth;
            m_finallyDepth = finallyContext.finallyDepth;
            int savedDynamicScopeDepth = m_localScopeDepth;
            m_localScopeDepth = finallyContext.dynamicScopeDepth;
            
            if (finallyContext.finallyBlock) {
                // Emit the finally block.
                emitNode(finallyContext.finallyBlock);
            } else {
                // Emit the IteratorClose block.
                ASSERT(finallyContext.iterator);
                emitIteratorClose(finallyContext.iterator, finallyContext.enumerationNode);
            }

            RefPtr<Label> afterFinally = emitLabel(newLabel().get());
            
            // Restore the state of the world.
            if (flipScopes) {
                m_scopeContextStack = savedScopeContextStack;
                topScope = &m_scopeContextStack[topScopeIndex]; // assert it's within bounds
                bottomScope = m_scopeContextStack.begin() + bottomScopeIndex; // don't assert, since it the index might be -1.
            }
            if (flipSwitches)
                m_switchContextStack = savedSwitchContextStack;
            if (flipForIns)
                m_forInContextStack.swap(savedForInContextStack);
            if (flipTries) {
                ASSERT(m_tryContextStack.size() == finallyContext.tryContextStackSize);
                for (unsigned i = poppedTryContexts.size(); i--;) {
                    TryContext context = poppedTryContexts[i];
                    context.start = afterFinally;
                    m_tryContextStack.append(context);
                }
                poppedTryContexts.clear();
            }
            if (flipLabelScopes)
                m_labelScopes = savedLabelScopes;
            if (flipSymbolTableStack)
                m_symbolTableStack = savedSymbolTableStack;
            m_finallyDepth = savedFinallyDepth;
            m_localScopeDepth = savedDynamicScopeDepth;
            
            --topScope;
        }
    }
}

void BytecodeGenerator::emitPopScopes(RegisterID* scope, int targetScopeDepth)
{
    ASSERT(labelScopeDepth() - targetScopeDepth >= 0);

    size_t scopeDelta = labelScopeDepth() - targetScopeDepth;
    ASSERT(scopeDelta <= m_scopeContextStack.size());
    if (!scopeDelta)
        return;

    if (!m_finallyDepth) {
        RefPtr<RegisterID> parentScope = newTemporary();
        while (scopeDelta--) {
            parentScope = emitGetParentScope(parentScope.get(), scope);
            emitMove(scope, parentScope.get());
        }
        return;
    }

    emitComplexPopScopes(scope, &m_scopeContextStack.last(), &m_scopeContextStack.last() - scopeDelta);
}

TryData* BytecodeGenerator::pushTry(Label* start)
{
    TryData tryData;
    tryData.target = newLabel();
    tryData.handlerType = HandlerType::Illegal;
    m_tryData.append(tryData);
    TryData* result = &m_tryData.last();
    
    TryContext tryContext;
    tryContext.start = start;
    tryContext.tryData = result;
    
    m_tryContextStack.append(tryContext);
    
    return result;
}

void BytecodeGenerator::popTryAndEmitCatch(TryData* tryData, RegisterID* exceptionRegister, RegisterID* thrownValueRegister, Label* end, HandlerType handlerType)
{
    m_usesExceptions = true;
    
    ASSERT_UNUSED(tryData, m_tryContextStack.last().tryData == tryData);
    
    TryRange tryRange;
    tryRange.start = m_tryContextStack.last().start;
    tryRange.end = end;
    tryRange.tryData = m_tryContextStack.last().tryData;
    m_tryRanges.append(tryRange);
    m_tryContextStack.removeLast();
    
    emitLabel(tryRange.tryData->target.get());
    tryRange.tryData->handlerType = handlerType;

    emitOpcode(op_catch);
    instructions().append(exceptionRegister->index());
    instructions().append(thrownValueRegister->index());

    bool foundLocalScope = false;
    for (unsigned i = m_symbolTableStack.size(); i--; ) {
        // Note that if we don't find a local scope in the current function/program, 
        // we must grab the outer-most scope of this bytecode generation.
        if (m_symbolTableStack[i].m_scope) {
            foundLocalScope = true;
            emitMove(scopeRegister(), m_symbolTableStack[i].m_scope);
            break;
        }
    }
    if (!foundLocalScope)
        emitMove(scopeRegister(), m_topMostScope);
}

int BytecodeGenerator::localScopeDepth() const
{
    return m_localScopeDepth;
}

int BytecodeGenerator::labelScopeDepth() const
{ 
    return localScopeDepth() + m_finallyDepth;
}

void BytecodeGenerator::emitThrowReferenceError(const String& message)
{
    emitOpcode(op_throw_static_error);
    instructions().append(addConstantValue(addStringConstant(Identifier::fromString(m_vm, message)))->index());
    instructions().append(true);
}

void BytecodeGenerator::emitThrowTypeError(const String& message)
{
    emitOpcode(op_throw_static_error);
    instructions().append(addConstantValue(addStringConstant(Identifier::fromString(m_vm, message)))->index());
    instructions().append(false);
}

void BytecodeGenerator::emitPushFunctionNameScope(const Identifier& property, RegisterID* callee)
{
    // There is some nuance here:
    // If we're in strict mode code, the function name scope variable acts exactly like a "const" variable.
    // If we're not in strict mode code, we want to allow bogus assignments to the name scoped variable.
    // This means any assignment to the variable won't throw, but it won't actually assign a new value to it.
    // To accomplish this, we don't report that this scope is a lexical scope. This will prevent
    // any throws when trying to assign to the variable (while still ensuring it keeps its original
    // value). There is some ugliness and exploitation of a leaky abstraction here, but it's better than 
    // having a completely new op code and a class to handle name scopes which are so close in functionality
    // to lexical environments.
    VariableEnvironment nameScopeEnvironment;
    auto addResult = nameScopeEnvironment.add(property);
    addResult.iterator->value.setIsCaptured();
    addResult.iterator->value.setIsConst(); // The function name scope name acts like a const variable.
    unsigned numVars = m_codeBlock->m_numVars;
    pushLexicalScopeInternal(nameScopeEnvironment, true, nullptr, TDZRequirement::NotUnderTDZ, ScopeType::FunctionNameScope, ScopeRegisterType::Var);
    ASSERT_UNUSED(numVars, m_codeBlock->m_numVars == static_cast<int>(numVars + 1)); // Should have only created one new "var" for the function name scope.
    bool shouldTreatAsLexicalVariable = isStrictMode();
    Variable functionVar = variableForLocalEntry(property, m_symbolTableStack.last().m_symbolTable->get(property.impl()), m_symbolTableStack.last().m_symbolTableConstantIndex, shouldTreatAsLexicalVariable);
    emitPutToScope(m_symbolTableStack.last().m_scope, functionVar, callee, ThrowIfNotFound);
}

void BytecodeGenerator::pushScopedControlFlowContext()
{
    ControlFlowContext context;
    context.isFinallyBlock = false;
    m_scopeContextStack.append(context);
    m_localScopeDepth++;
}

void BytecodeGenerator::popScopedControlFlowContext()
{
    ASSERT(m_scopeContextStack.size());
    ASSERT(!m_scopeContextStack.last().isFinallyBlock);
    m_scopeContextStack.removeLast();
    m_localScopeDepth--;
}

void BytecodeGenerator::emitPushCatchScope(const Identifier& property, RegisterID* exceptionValue, VariableEnvironment& environment)
{
    RELEASE_ASSERT(environment.contains(property.impl()));
    pushLexicalScopeInternal(environment, true, nullptr, TDZRequirement::NotUnderTDZ, ScopeType::CatchScope, ScopeRegisterType::Block);
    Variable exceptionVar = variable(property);
    RELEASE_ASSERT(exceptionVar.isResolved());
    RefPtr<RegisterID> scope = emitResolveScope(nullptr, exceptionVar);
    emitPutToScope(scope.get(), exceptionVar, exceptionValue, ThrowIfNotFound);
}

void BytecodeGenerator::emitPopCatchScope(VariableEnvironment& environment) 
{
    popLexicalScopeInternal(environment, TDZRequirement::NotUnderTDZ);
}

void BytecodeGenerator::beginSwitch(RegisterID* scrutineeRegister, SwitchInfo::SwitchType type)
{
    SwitchInfo info = { static_cast<uint32_t>(instructions().size()), type };
    switch (type) {
        case SwitchInfo::SwitchImmediate:
            emitOpcode(op_switch_imm);
            break;
        case SwitchInfo::SwitchCharacter:
            emitOpcode(op_switch_char);
            break;
        case SwitchInfo::SwitchString:
            emitOpcode(op_switch_string);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
    }

    instructions().append(0); // place holder for table index
    instructions().append(0); // place holder for default target    
    instructions().append(scrutineeRegister->index());
    m_switchContextStack.append(info);
}

static int32_t keyForImmediateSwitch(ExpressionNode* node, int32_t min, int32_t max)
{
    UNUSED_PARAM(max);
    ASSERT(node->isNumber());
    double value = static_cast<NumberNode*>(node)->value();
    int32_t key = static_cast<int32_t>(value);
    ASSERT(key == value);
    ASSERT(key >= min);
    ASSERT(key <= max);
    return key - min;
}

static int32_t keyForCharacterSwitch(ExpressionNode* node, int32_t min, int32_t max)
{
    UNUSED_PARAM(max);
    ASSERT(node->isString());
    StringImpl* clause = static_cast<StringNode*>(node)->value().impl();
    ASSERT(clause->length() == 1);
    
    int32_t key = (*clause)[0];
    ASSERT(key >= min);
    ASSERT(key <= max);
    return key - min;
}

static void prepareJumpTableForSwitch(
    UnlinkedSimpleJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount,
    RefPtr<Label>* labels, ExpressionNode** nodes, int32_t min, int32_t max,
    int32_t (*keyGetter)(ExpressionNode*, int32_t min, int32_t max))
{
    jumpTable.min = min;
    jumpTable.branchOffsets.resize(max - min + 1);
    jumpTable.branchOffsets.fill(0);
    for (uint32_t i = 0; i < clauseCount; ++i) {
        // We're emitting this after the clause labels should have been fixed, so 
        // the labels should not be "forward" references
        ASSERT(!labels[i]->isForward());
        jumpTable.add(keyGetter(nodes[i], min, max), labels[i]->bind(switchAddress, switchAddress + 3)); 
    }
}

static void prepareJumpTableForStringSwitch(UnlinkedStringJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes)
{
    for (uint32_t i = 0; i < clauseCount; ++i) {
        // We're emitting this after the clause labels should have been fixed, so 
        // the labels should not be "forward" references
        ASSERT(!labels[i]->isForward());
        
        ASSERT(nodes[i]->isString());
        StringImpl* clause = static_cast<StringNode*>(nodes[i])->value().impl();
        jumpTable.offsetTable.add(clause, labels[i]->bind(switchAddress, switchAddress + 3));
    }
}

void BytecodeGenerator::endSwitch(uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes, Label* defaultLabel, int32_t min, int32_t max)
{
    SwitchInfo switchInfo = m_switchContextStack.last();
    m_switchContextStack.removeLast();
    
    switch (switchInfo.switchType) {
    case SwitchInfo::SwitchImmediate:
    case SwitchInfo::SwitchCharacter: {
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);

        UnlinkedSimpleJumpTable& jumpTable = m_codeBlock->addSwitchJumpTable();
        prepareJumpTableForSwitch(
            jumpTable, switchInfo.bytecodeOffset, clauseCount, labels, nodes, min, max,
            switchInfo.switchType == SwitchInfo::SwitchImmediate
                ? keyForImmediateSwitch
                : keyForCharacterSwitch); 
        break;
    }
        
    case SwitchInfo::SwitchString: {
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfStringSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);

        UnlinkedStringJumpTable& jumpTable = m_codeBlock->addStringSwitchJumpTable();
        prepareJumpTableForStringSwitch(jumpTable, switchInfo.bytecodeOffset, clauseCount, labels, nodes);
        break;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

RegisterID* BytecodeGenerator::emitThrowExpressionTooDeepException()
{
    // It would be nice to do an even better job of identifying exactly where the expression is.
    // And we could make the caller pass the node pointer in, if there was some way of getting
    // that from an arbitrary node. However, calling emitExpressionInfo without any useful data
    // is still good enough to get us an accurate line number.
    m_expressionTooDeep = true;
    return newTemporary();
}

bool BytecodeGenerator::isArgumentNumber(const Identifier& ident, int argumentNumber)
{
    RegisterID* registerID = variable(ident).local();
    if (!registerID)
        return false;
    return registerID->index() == CallFrame::argumentOffset(argumentNumber);
}

bool BytecodeGenerator::emitReadOnlyExceptionIfNeeded(const Variable& variable)
{
    if (isStrictMode() || variable.isConst()) {
        emitOpcode(op_throw_static_error);
        instructions().append(addConstantValue(addStringConstant(Identifier::fromString(m_vm, StrictModeReadonlyPropertyWriteError)))->index());
        instructions().append(false);
        return true;
    }
    return false;
}
    
void BytecodeGenerator::emitEnumeration(ThrowableExpressionData* node, ExpressionNode* subjectNode, const std::function<void(BytecodeGenerator&, RegisterID*)>& callBack, VariableEnvironmentNode* forLoopNode, RegisterID* forLoopSymbolTable)
{
    RefPtr<RegisterID> subject = newTemporary();
    emitNode(subject.get(), subjectNode);
    RefPtr<RegisterID> iterator = emitGetById(newTemporary(), subject.get(), propertyNames().iteratorSymbol);
    {
        CallArguments args(*this, nullptr);
        emitMove(args.thisRegister(), subject.get());
        emitCall(iterator.get(), iterator.get(), NoExpectedFunction, args, node->divot(), node->divotStart(), node->divotEnd());
    }

    RefPtr<Label> loopDone = newLabel();
    // RefPtr<Register> iterator's lifetime must be longer than IteratorCloseContext.
    pushIteratorCloseContext(iterator.get(), node);
    {
        LabelScopePtr scope = newLabelScope(LabelScope::Loop);
        RefPtr<RegisterID> value = newTemporary();
        emitLoad(value.get(), jsUndefined());

        emitJump(scope->continueTarget());

        RefPtr<Label> loopStart = newLabel();
        emitLabel(loopStart.get());
        emitLoopHint();

        RefPtr<Label> tryStartLabel = newLabel();
        emitLabel(tryStartLabel.get());
        TryData* tryData = pushTry(tryStartLabel.get());
        callBack(*this, value.get());
        emitJump(scope->continueTarget());

        // IteratorClose sequence for throw-ed control flow.
        {
            RefPtr<Label> catchHere = emitLabel(newLabel().get());
            RefPtr<RegisterID> exceptionRegister = newTemporary();
            RefPtr<RegisterID> thrownValueRegister = newTemporary();
            popTryAndEmitCatch(tryData, exceptionRegister.get(),
                thrownValueRegister.get(), catchHere.get(), HandlerType::SynthesizedFinally);

            RefPtr<Label> catchDone = newLabel();

            RefPtr<RegisterID> returnMethod = emitGetById(newTemporary(), iterator.get(), propertyNames().returnKeyword);
            emitJumpIfTrue(emitIsUndefined(newTemporary(), returnMethod.get()), catchDone.get());

            RefPtr<Label> returnCallTryStart = newLabel();
            emitLabel(returnCallTryStart.get());
            TryData* returnCallTryData = pushTry(returnCallTryStart.get());

            CallArguments returnArguments(*this, nullptr);
            emitMove(returnArguments.thisRegister(), iterator.get());
            emitCall(value.get(), returnMethod.get(), NoExpectedFunction, returnArguments, node->divot(), node->divotStart(), node->divotEnd());

            emitLabel(catchDone.get());
            emitThrow(exceptionRegister.get());

            // Absorb exception.
            popTryAndEmitCatch(returnCallTryData, newTemporary(),
                newTemporary(), catchDone.get(), HandlerType::SynthesizedFinally);
            emitThrow(exceptionRegister.get());
        }

        emitLabel(scope->continueTarget());
        if (forLoopNode)
            prepareLexicalScopeForNextForLoopIteration(forLoopNode, forLoopSymbolTable);

        {
            emitIteratorNext(value.get(), iterator.get(), node);
            emitJumpIfTrue(emitGetById(newTemporary(), value.get(), propertyNames().done), loopDone.get());
            emitGetById(value.get(), value.get(), propertyNames().value);
            emitJump(loopStart.get());
        }

        emitLabel(scope->breakTarget());
    }

    // IteratorClose sequence for break-ed control flow.
    popIteratorCloseContext();
    emitIteratorClose(iterator.get(), node);
    emitLabel(loopDone.get());
}

#if ENABLE(ES6_TEMPLATE_LITERAL_SYNTAX)
RegisterID* BytecodeGenerator::emitGetTemplateObject(RegisterID* dst, TaggedTemplateNode* taggedTemplate)
{
    TemplateRegistryKey::StringVector rawStrings;
    TemplateRegistryKey::StringVector cookedStrings;

    TemplateStringListNode* templateString = taggedTemplate->templateLiteral()->templateStrings();
    for (; templateString; templateString = templateString->next()) {
        rawStrings.append(templateString->value()->raw().impl());
        cookedStrings.append(templateString->value()->cooked().impl());
    }

    RefPtr<RegisterID> getTemplateObject = nullptr;
    Variable var = variable(propertyNames().getTemplateObjectPrivateName);
    if (RegisterID* local = var.local())
        getTemplateObject = emitMove(newTemporary(), local);
    else {
        getTemplateObject = newTemporary();
        RefPtr<RegisterID> scope = newTemporary();
        moveToDestinationIfNeeded(scope.get(), emitResolveScope(scope.get(), var));
        emitGetFromScope(getTemplateObject.get(), scope.get(), var, ThrowIfNotFound);
    }

    CallArguments arguments(*this, nullptr);
    emitLoad(arguments.thisRegister(), JSValue(addTemplateRegistryKeyConstant(TemplateRegistryKey(rawStrings, cookedStrings))));
    return emitCall(dst, getTemplateObject.get(), NoExpectedFunction, arguments, taggedTemplate->divot(), taggedTemplate->divotStart(), taggedTemplate->divotEnd());
}
#endif

RegisterID* BytecodeGenerator::emitGetEnumerableLength(RegisterID* dst, RegisterID* base)
{
    emitOpcode(op_get_enumerable_length);
    instructions().append(dst->index());
    instructions().append(base->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitHasGenericProperty(RegisterID* dst, RegisterID* base, RegisterID* propertyName)
{
    emitOpcode(op_has_generic_property);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(propertyName->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitHasIndexedProperty(RegisterID* dst, RegisterID* base, RegisterID* propertyName)
{
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    emitOpcode(op_has_indexed_property);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(propertyName->index());
    instructions().append(arrayProfile);
    return dst;
}

RegisterID* BytecodeGenerator::emitHasStructureProperty(RegisterID* dst, RegisterID* base, RegisterID* propertyName, RegisterID* enumerator)
{
    emitOpcode(op_has_structure_property);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(propertyName->index());
    instructions().append(enumerator->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitGetPropertyEnumerator(RegisterID* dst, RegisterID* base)
{
    emitOpcode(op_get_property_enumerator);
    instructions().append(dst->index());
    instructions().append(base->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitEnumeratorStructurePropertyName(RegisterID* dst, RegisterID* enumerator, RegisterID* index)
{
    emitOpcode(op_enumerator_structure_pname);
    instructions().append(dst->index());
    instructions().append(enumerator->index());
    instructions().append(index->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitEnumeratorGenericPropertyName(RegisterID* dst, RegisterID* enumerator, RegisterID* index)
{
    emitOpcode(op_enumerator_generic_pname);
    instructions().append(dst->index());
    instructions().append(enumerator->index());
    instructions().append(index->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitToIndexString(RegisterID* dst, RegisterID* index)
{
    emitOpcode(op_to_index_string);
    instructions().append(dst->index());
    instructions().append(index->index());
    return dst;
}


RegisterID* BytecodeGenerator::emitIsObject(RegisterID* dst, RegisterID* src)
{
    emitOpcode(op_is_object);
    instructions().append(dst->index());
    instructions().append(src->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitIsUndefined(RegisterID* dst, RegisterID* src)
{
    emitOpcode(op_is_undefined);
    instructions().append(dst->index());
    instructions().append(src->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitIteratorNext(RegisterID* dst, RegisterID* iterator, const ThrowableExpressionData* node)
{
    {
        RefPtr<RegisterID> next = emitGetById(newTemporary(), iterator, propertyNames().next);
        CallArguments nextArguments(*this, nullptr);
        emitMove(nextArguments.thisRegister(), iterator);
        emitCall(dst, next.get(), NoExpectedFunction, nextArguments, node->divot(), node->divotStart(), node->divotEnd());
    }
    {
        RefPtr<Label> typeIsObject = newLabel();
        emitJumpIfTrue(emitIsObject(newTemporary(), dst), typeIsObject.get());
        emitThrowTypeError(ASCIILiteral("Iterator result interface is not an object."));
        emitLabel(typeIsObject.get());
    }
    return dst;
}

void BytecodeGenerator::emitIteratorClose(RegisterID* iterator, const ThrowableExpressionData* node)
{
    RefPtr<Label> done = newLabel();
    RefPtr<RegisterID> returnMethod = emitGetById(newTemporary(), iterator, propertyNames().returnKeyword);
    emitJumpIfTrue(emitIsUndefined(newTemporary(), returnMethod.get()), done.get());

    RefPtr<RegisterID> value = newTemporary();
    CallArguments returnArguments(*this, nullptr);
    emitMove(returnArguments.thisRegister(), iterator);
    emitCall(value.get(), returnMethod.get(), NoExpectedFunction, returnArguments, node->divot(), node->divotStart(), node->divotEnd());
    emitJumpIfTrue(emitIsObject(newTemporary(), value.get()), done.get());
    emitThrowTypeError(ASCIILiteral("Iterator result interface is not an object."));
    emitLabel(done.get());
}

void BytecodeGenerator::pushIndexedForInScope(RegisterID* localRegister, RegisterID* indexRegister)
{
    if (!localRegister)
        return;
    m_forInContextStack.append(std::make_unique<IndexedForInContext>(localRegister, indexRegister));
}

void BytecodeGenerator::popIndexedForInScope(RegisterID* localRegister)
{
    if (!localRegister)
        return;
    m_forInContextStack.removeLast();
}

void BytecodeGenerator::pushStructureForInScope(RegisterID* localRegister, RegisterID* indexRegister, RegisterID* propertyRegister, RegisterID* enumeratorRegister)
{
    if (!localRegister)
        return;
    m_forInContextStack.append(std::make_unique<StructureForInContext>(localRegister, indexRegister, propertyRegister, enumeratorRegister));
}

void BytecodeGenerator::popStructureForInScope(RegisterID* localRegister)
{
    if (!localRegister)
        return;
    m_forInContextStack.removeLast();
}

void BytecodeGenerator::invalidateForInContextForLocal(RegisterID* localRegister)
{
    // Lexically invalidating ForInContexts is kind of weak sauce, but it only occurs if 
    // either of the following conditions is true:
    // 
    // (1) The loop iteration variable is re-assigned within the body of the loop.
    // (2) The loop iteration variable is captured in the lexical scope of the function.
    //
    // These two situations occur sufficiently rarely that it's okay to use this style of 
    // "analysis" to make iteration faster. If we didn't want to do this, we would either have 
    // to perform some flow-sensitive analysis to see if/when the loop iteration variable was 
    // reassigned, or we'd have to resort to runtime checks to see if the variable had been 
    // reassigned from its original value.
    for (size_t i = m_forInContextStack.size(); i > 0; i--) {
        ForInContext* context = m_forInContextStack[i - 1].get();
        if (context->local() != localRegister)
            continue;
        context->invalidate();
        break;
    }
}

} // namespace JSC
