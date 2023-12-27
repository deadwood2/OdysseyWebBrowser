/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2011 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "ArrayConstructor.h"

#include "ArrayPrototype.h"
#include "ButterflyInlines.h"
#include "CopiedSpaceInlines.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Lookup.h"
#include "JSCInlines.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL arrayConstructorIsArray(ExecState*);

}

#include "ArrayConstructor.lut.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ArrayConstructor);

const ClassInfo ArrayConstructor::s_info = { "Function", &InternalFunction::s_info, &arrayConstructorTable, CREATE_METHOD_TABLE(ArrayConstructor) };

/* Source for ArrayConstructor.lut.h
@begin arrayConstructorTable
  isArray   arrayConstructorIsArray     DontEnum|Function 1
  of        arrayConstructorOf          DontEnum|Function 0
  from      arrayConstructorFrom        DontEnum|Function 0
@end
*/

ArrayConstructor::ArrayConstructor(VM& vm, Structure* structure)
    : InternalFunction(vm, structure)
{
}

void ArrayConstructor::finishCreation(VM& vm, ArrayPrototype* arrayPrototype)
{
    Base::finishCreation(vm, arrayPrototype->classInfo()->className);
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, arrayPrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool ArrayConstructor::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, arrayConstructorTable, jsCast<ArrayConstructor*>(object), propertyName, slot);
}

// ------------------------------ Functions ---------------------------

JSObject* constructArrayWithSizeQuirk(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, JSValue length)
{
    if (!length.isNumber())
        return constructArrayNegativeIndexed(exec, profile, globalObject, &length, 1);
    
    uint32_t n = length.toUInt32(exec);
    if (n != length.toNumber(exec))
        return exec->vm().throwException(exec, createRangeError(exec, ASCIILiteral("Array size is not a small enough positive integer.")));
    return constructEmptyArray(exec, profile, globalObject, n);
}

static inline JSObject* constructArrayWithSizeQuirk(ExecState* exec, const ArgList& args)
{
    JSGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();

    // a single numeric argument denotes the array size (!)
    if (args.size() == 1)
        return constructArrayWithSizeQuirk(exec, 0, globalObject, args.at(0));

    // otherwise the array is constructed with the arguments in it
    return constructArray(exec, 0, globalObject, args);
}

static EncodedJSValue JSC_HOST_CALL constructWithArrayConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructArrayWithSizeQuirk(exec, args));
}

ConstructType ArrayConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithArrayConstructor;
    return ConstructTypeHost;
}

static EncodedJSValue JSC_HOST_CALL callArrayConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructArrayWithSizeQuirk(exec, args));
}

CallType ArrayConstructor::getCallData(JSCell*, CallData& callData)
{
    // equivalent to 'new Array(....)'
    callData.native.function = callArrayConstructor;
    return CallTypeHost;
}

EncodedJSValue JSC_HOST_CALL arrayConstructorIsArray(ExecState* exec)
{
    return JSValue::encode(jsBoolean(exec->argument(0).inherits(JSArray::info())));
}

} // namespace JSC
