/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003-2009, 2015-2016 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "FunctionPrototype.h"

#include "BuiltinExecutables.h"
#include "BuiltinNames.h"
#include "Error.h"
#include "JSArray.h"
#include "JSBoundFunction.h"
#include "JSFunction.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "Interpreter.h"
#include "Lexer.h"
#include "JSCInlines.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(FunctionPrototype);

const ClassInfo FunctionPrototype::s_info = { "Function", &Base::s_info, 0, CREATE_METHOD_TABLE(FunctionPrototype) };

static EncodedJSValue JSC_HOST_CALL functionProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL functionProtoFuncBind(ExecState*);

FunctionPrototype::FunctionPrototype(VM& vm, Structure* structure)
    : InternalFunction(vm, structure)
{
}

void FunctionPrototype::finishCreation(VM& vm, const String& name)
{
    Base::finishCreation(vm, name);
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(0), DontDelete | ReadOnly | DontEnum);
}

void FunctionPrototype::addFunctionProperties(ExecState* exec, JSGlobalObject* globalObject, JSFunction** callFunction, JSFunction** applyFunction, JSFunction** hasInstanceSymbolFunction)
{
    VM& vm = exec->vm();

    JSFunction* toStringFunction = JSFunction::create(vm, globalObject, 0, vm.propertyNames->toString.string(), functionProtoFuncToString);
    putDirectWithoutTransition(vm, vm.propertyNames->toString, toStringFunction, DontEnum);

    *applyFunction = putDirectBuiltinFunctionWithoutTransition(vm, globalObject, vm.propertyNames->builtinNames().applyPublicName(), functionPrototypeApplyCodeGenerator(vm), DontEnum);
    *callFunction = putDirectBuiltinFunctionWithoutTransition(vm, globalObject, vm.propertyNames->builtinNames().callPublicName(), functionPrototypeCallCodeGenerator(vm), DontEnum);
    *hasInstanceSymbolFunction = putDirectBuiltinFunction(vm, globalObject, vm.propertyNames->hasInstanceSymbol, functionPrototypeSymbolHasInstanceCodeGenerator(vm), DontDelete | ReadOnly | DontEnum);

    JSFunction* bindFunction = JSFunction::create(vm, globalObject, 1, vm.propertyNames->bind.string(), functionProtoFuncBind);
    putDirectWithoutTransition(vm, vm.propertyNames->bind, bindFunction, DontEnum);
}

static EncodedJSValue JSC_HOST_CALL callFunctionPrototype(ExecState*)
{
    return JSValue::encode(jsUndefined());
}

// ECMA 15.3.4
CallType FunctionPrototype::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callFunctionPrototype;
    return CallType::Host;
}

EncodedJSValue JSC_HOST_CALL functionProtoFuncToString(ExecState* exec)
{
    VM& vm = exec->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    JSValue thisValue = exec->thisValue();
    if (thisValue.inherits(JSFunction::info())) {
        JSFunction* function = jsCast<JSFunction*>(thisValue);
        if (function->isHostOrBuiltinFunction())
            return JSValue::encode(jsMakeNontrivialString(exec, "function ", function->name(vm), "() {\n    [native code]\n}"));

        FunctionExecutable* executable = function->jsExecutable();
        if (executable->isClass()) {
            StringView classSource = executable->classSource().view();
            return JSValue::encode(jsString(exec, classSource.toStringWithoutCopying()));
        }

        String functionHeader = executable->isArrowFunction() ? "" : "function ";
        
        StringView source = executable->source().provider()->getRange(
            executable->parametersStartOffset(),
            executable->parametersStartOffset() + executable->source().length());
        return JSValue::encode(jsMakeNontrivialString(exec, functionHeader, function->name(vm), source));
    }

    if (thisValue.inherits(InternalFunction::info())) {
        InternalFunction* function = asInternalFunction(thisValue);
        return JSValue::encode(jsMakeNontrivialString(exec, "function ", function->name(), "() {\n    [native code]\n}"));
    }

    if (thisValue.isObject()) {
        JSObject* object = asObject(thisValue);
        if (object->inlineTypeFlags() & TypeOfShouldCallGetCallData) {
            CallData callData;
            if (object->methodTable(vm)->getCallData(object, callData) != CallType::None) {
                if (auto* classInfo = object->classInfo())
                    return JSValue::encode(jsMakeNontrivialString(exec, "function ", classInfo->className, "() {\n    [native code]\n}"));
            }
        }
    }

    return throwVMTypeError(exec, scope);
}

// 15.3.4.5 Function.prototype.bind (thisArg [, arg1 [, arg2, ...]])
EncodedJSValue JSC_HOST_CALL functionProtoFuncBind(ExecState* exec)
{
    VM& vm = exec->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    JSGlobalObject* globalObject = exec->callee()->globalObject();

    // Let Target be the this value.
    JSValue target = exec->thisValue();

    // If IsCallable(Target) is false, throw a TypeError exception.
    CallData callData;
    CallType callType = getCallData(target, callData);
    if (callType == CallType::None)
        return throwVMTypeError(exec, scope);
    // Primitive values are not callable.
    ASSERT(target.isObject());
    JSObject* targetObject = asObject(target);

    // Let A be a new (possibly empty) internal list of all of the argument values provided after thisArg (arg1, arg2 etc), in order.
    size_t numBoundArgs = exec->argumentCount() > 1 ? exec->argumentCount() - 1 : 0;
    JSArray* boundArgs;
    if (numBoundArgs) {
        boundArgs = JSArray::tryCreateUninitialized(vm, globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithContiguous), numBoundArgs);
        if (!boundArgs)
            return JSValue::encode(throwOutOfMemoryError(exec, scope));
        
        for (size_t i = 0; i < numBoundArgs; ++i)
            boundArgs->initializeIndex(vm, i, exec->argument(i + 1));
    } else
        boundArgs = nullptr;

    // If the [[Class]] internal property of Target is "Function", then ...
    // Else set the length own property of F to 0.
    unsigned length = 0;
    if (targetObject->hasOwnProperty(exec, exec->propertyNames().length)) {
        if (exec->hadException())
            return JSValue::encode(jsUndefined());

        // a. Let L be the length property of Target minus the length of A.
        // b. Set the length own property of F to either 0 or L, whichever is larger.
        JSValue lengthValue = target.get(exec, exec->propertyNames().length);
        if (lengthValue.isNumber()) {
            unsigned targetLength = (unsigned)lengthValue.asNumber();
            if (targetLength > numBoundArgs)
                length = targetLength - numBoundArgs;
        }
    }

    JSValue nameProp = target.get(exec, exec->propertyNames().name);
    JSString* name = nameProp.isString() ? nameProp.toString(exec) : jsEmptyString(exec);
    return JSValue::encode(JSBoundFunction::create(vm, exec, globalObject, targetObject, exec->argument(0), boundArgs, length, name->value(exec)));
}

} // namespace JSC
