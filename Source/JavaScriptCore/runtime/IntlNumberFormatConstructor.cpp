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
#include "IntlNumberFormatConstructor.h"

#if ENABLE(INTL)

#include "Error.h"
#include "IntlNumberFormat.h"
#include "IntlNumberFormatPrototype.h"
#include "IntlObject.h"
#include "JSCJSValueInlines.h"
#include "JSCellInlines.h"
#include "Lookup.h"
#include "SlotVisitorInlines.h"
#include "StructureInlines.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(IntlNumberFormatConstructor);

static EncodedJSValue JSC_HOST_CALL IntlNumberFormatConstructorFuncSupportedLocalesOf(ExecState*);

}

#include "IntlNumberFormatConstructor.lut.h"

namespace JSC {

const ClassInfo IntlNumberFormatConstructor::s_info = { "Function", &Base::s_info, &numberFormatConstructorTable, CREATE_METHOD_TABLE(IntlNumberFormatConstructor) };

/* Source for IntlNumberFormatConstructor.lut.h
@begin numberFormatConstructorTable
  supportedLocalesOf             IntlNumberFormatConstructorFuncSupportedLocalesOf             DontEnum|Function 1
@end
*/

IntlNumberFormatConstructor* IntlNumberFormatConstructor::create(VM& vm, Structure* structure, IntlNumberFormatPrototype* numberFormatPrototype, Structure* numberFormatStructure)
{
    IntlNumberFormatConstructor* constructor = new (NotNull, allocateCell<IntlNumberFormatConstructor>(vm.heap)) IntlNumberFormatConstructor(vm, structure);
    constructor->finishCreation(vm, numberFormatPrototype, numberFormatStructure);
    return constructor;
}

Structure* IntlNumberFormatConstructor::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
}

IntlNumberFormatConstructor::IntlNumberFormatConstructor(VM& vm, Structure* structure)
    : InternalFunction(vm, structure)
{
}

void IntlNumberFormatConstructor::finishCreation(VM& vm, IntlNumberFormatPrototype* numberFormatPrototype, Structure* numberFormatStructure)
{
    Base::finishCreation(vm, ASCIILiteral("NumberFormat"));
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, numberFormatPrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(0), ReadOnly | DontEnum | DontDelete);
    m_numberFormatStructure.set(vm, this, numberFormatStructure);
}

EncodedJSValue JSC_HOST_CALL constructIntlNumberFormat(ExecState* exec)
{
    // 11.1.2 Intl.NumberFormat ([locales [, options]]) (ECMA-402 2.0)
    // 1. If NewTarget is undefined, let newTarget be the active function object, else let newTarget be NewTarget.
    JSValue newTarget = exec->newTarget();
    if (!newTarget || newTarget.isUndefined())
        newTarget = exec->callee();

    // 2. Let numberFormat be OrdinaryCreateFromConstructor(newTarget, %NumberFormatPrototype%).
    VM& vm = exec->vm();
    IntlNumberFormat* numberFormat = IntlNumberFormat::create(vm, jsCast<IntlNumberFormatConstructor*>(exec->callee()));
    if (numberFormat && !jsDynamicCast<IntlNumberFormatConstructor*>(newTarget)) {
        JSValue proto = asObject(newTarget)->getDirect(vm, vm.propertyNames->prototype);
        asObject(numberFormat)->setPrototypeWithCycleCheck(exec, proto);
    }

    // 3. ReturnIfAbrupt(numberFormat).
    ASSERT(numberFormat);

    // 4. Return InitializeNumberFormat(numberFormat, locales, options).
    // FIXME: return JSValue::encode(InitializeNumberFormat(numberFormat, locales, options));

    return JSValue::encode(numberFormat);
}

EncodedJSValue JSC_HOST_CALL callIntlNumberFormat(ExecState* exec)
{
    // 11.1.2 Intl.NumberFormat ([locales [, options]]) (ECMA-402 2.0)
    // 1. If NewTarget is undefined, let newTarget be the active function object, else let newTarget be NewTarget.
    // NewTarget is always undefined when called as a function.

    // 2. Let numberFormat be OrdinaryCreateFromConstructor(newTarget, %NumberFormatPrototype%).
    VM& vm = exec->vm();
    IntlNumberFormat* numberFormat = IntlNumberFormat::create(vm, jsCast<IntlNumberFormatConstructor*>(exec->callee()));

    // 3. ReturnIfAbrupt(numberFormat).
    ASSERT(numberFormat);

    // 4. Return InitializeNumberFormat(numberFormat, locales, options).
    // FIXME: return JSValue::encode(InitializeNumberFormat(numberFormat, locales, options));

    return JSValue::encode(numberFormat);
}

ConstructType IntlNumberFormatConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructIntlNumberFormat;
    return ConstructTypeHost;
}

CallType IntlNumberFormatConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callIntlNumberFormat;
    return CallTypeHost;
}

bool IntlNumberFormatConstructor::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, numberFormatConstructorTable, jsCast<IntlNumberFormatConstructor*>(object), propertyName, slot);
}

EncodedJSValue JSC_HOST_CALL IntlNumberFormatConstructorFuncSupportedLocalesOf(ExecState* exec)
{
    // 11.2.2 Intl.NumberFormat.supportedLocalesOf(locales [, options]) (ECMA-402 2.0)

    // 1. Let availableLocales be %NumberFormat%.[[availableLocales]].
    // FIXME: available = IntlNumberFormatConstructor::getAvailableLocales()

    // 2. Let requestedLocales be CanonicalizeLocaleList(locales).
    // FIXME: requested = CanonicalizeLocaleList(locales)

    // 3. Return SupportedLocales(availableLocales, requestedLocales, options).
    // FIXME: return JSValue::encode(SupportedLocales(available, requested, options));

    // Return empty array until properly implemented.
    VM& vm = exec->vm();
    JSGlobalObject* globalObject = exec->callee()->globalObject();
    JSArray* supportedLocales = JSArray::tryCreateUninitialized(vm, globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithUndecided), 0);
    if (!supportedLocales)
        return JSValue::encode(throwOutOfMemoryError(exec));

    return JSValue::encode(supportedLocales);
}

void IntlNumberFormatConstructor::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    IntlNumberFormatConstructor* thisObject = jsCast<IntlNumberFormatConstructor*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    Base::visitChildren(thisObject, visitor);

    visitor.append(&thisObject->m_numberFormatStructure);
}

} // namespace JSC

#endif // ENABLE(INTL)
