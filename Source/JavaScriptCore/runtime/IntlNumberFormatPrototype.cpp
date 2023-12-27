/*
 * Copyright (C) 2015 Andy VanWagoner (thetalecrafter@gmail.com)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IntlNumberFormatPrototype.h"

#if ENABLE(INTL)

#include "Error.h"
#include "IntlNumberFormat.h"
#include "JSBoundFunction.h"
#include "JSCJSValueInlines.h"
#include "JSCellInlines.h"
#include "JSObject.h"
#include "ObjectConstructor.h"
#include "StructureInlines.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(IntlNumberFormatPrototype);

static EncodedJSValue JSC_HOST_CALL IntlNumberFormatPrototypeGetterFormat(ExecState*);
static EncodedJSValue JSC_HOST_CALL IntlNumberFormatPrototypeFuncResolvedOptions(ExecState*);

}

#include "IntlNumberFormatPrototype.lut.h"

namespace JSC {

const ClassInfo IntlNumberFormatPrototype::s_info = { "Object", &IntlNumberFormat::s_info, &numberFormatPrototypeTable, CREATE_METHOD_TABLE(IntlNumberFormatPrototype) };

/* Source for IntlNumberFormatPrototype.lut.h
@begin numberFormatPrototypeTable
  format           IntlNumberFormatPrototypeGetterFormat         DontEnum|Accessor
  resolvedOptions  IntlNumberFormatPrototypeFuncResolvedOptions  DontEnum|Function 0
@end
*/

IntlNumberFormatPrototype* IntlNumberFormatPrototype::create(VM& vm, JSGlobalObject*, Structure* structure)
{
    IntlNumberFormatPrototype* object = new (NotNull, allocateCell<IntlNumberFormatPrototype>(vm.heap)) IntlNumberFormatPrototype(vm, structure);
    object->finishCreation(vm, structure);
    return object;
}

Structure* IntlNumberFormatPrototype::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
}

IntlNumberFormatPrototype::IntlNumberFormatPrototype(VM& vm, Structure* structure)
    : IntlNumberFormat(vm, structure)
{
}

void IntlNumberFormatPrototype::finishCreation(VM& vm, Structure*)
{
    Base::finishCreation(vm);
}

bool IntlNumberFormatPrototype::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSObject>(exec, numberFormatPrototypeTable, jsCast<IntlNumberFormatPrototype*>(object), propertyName, slot);
}

EncodedJSValue JSC_HOST_CALL IntlNumberFormatPrototypeGetterFormat(ExecState* exec)
{
    // 11.3.3 Intl.NumberFormat.prototype.format (ECMA-402 2.0)
    // 1. Let nf be this NumberFormat object.
    IntlNumberFormat* nf = jsDynamicCast<IntlNumberFormat*>(exec->thisValue());
    if (!nf)
        return JSValue::encode(throwTypeError(exec, ASCIILiteral("Intl.NumberFormat.prototype.format called on value that's not an object initialized as a NumberFormat")));
    
    JSBoundFunction* boundFormat = nf->boundFormat();
    // 2. If nf.[[boundFormat]] is undefined,
    if (!boundFormat) {
        VM& vm = exec->vm();
        JSGlobalObject* globalObject = nf->globalObject();
        // a. Let F be a new built-in function object as defined in 11.3.4.
        // b. The value of F’s length property is 1.
        JSFunction* targetObject = JSFunction::create(vm, globalObject, 1, ASCIILiteral("format"), IntlNumberFormatFuncFormatNumber, NoIntrinsic);
        JSArray* boundArgs = JSArray::tryCreateUninitialized(vm, globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithUndecided), 0);
        if (!boundArgs)
            return JSValue::encode(throwOutOfMemoryError(exec));

        // c. Let bf be BoundFunctionCreate(F, «this value»).
        boundFormat = JSBoundFunction::create(vm, globalObject, targetObject, nf, boundArgs, 1, ASCIILiteral("format"));
        // d. Set nf.[[boundFormat]] to bf.
        nf->setBoundFormat(vm, boundFormat);
    }
    // 3. Return nf.[[boundFormat]].
    return JSValue::encode(boundFormat);
}

EncodedJSValue JSC_HOST_CALL IntlNumberFormatPrototypeFuncResolvedOptions(ExecState* exec)
{
    // 11.3.5 Intl.NumberFormat.prototype.resolvedOptions() (ECMA-402 2.0)
    IntlNumberFormat* nf = jsDynamicCast<IntlNumberFormat*>(exec->thisValue());
    if (!nf)
        return JSValue::encode(throwTypeError(exec, ASCIILiteral("Intl.NumberFormat.prototype.resolvedOptions called on value that's not an object initialized as a NumberFormat")));

    // The function returns a new object whose properties and attributes are set as if constructed by an object literal assigning to each of the following properties the value of the corresponding internal slot of this NumberFormat object (see 11.4): locale, numberingSystem, style, currency, currencyDisplay, minimumIntegerDigits, minimumFractionDigits, maximumFractionDigits, minimumSignificantDigits, maximumSignificantDigits, and useGrouping. Properties whose corresponding internal slots are not present are not assigned.

    JSObject* options = constructEmptyObject(exec);

    // FIXME: Populate object from internal slots.

    return JSValue::encode(options);
}

} // namespace JSC

#endif // ENABLE(INTL)
