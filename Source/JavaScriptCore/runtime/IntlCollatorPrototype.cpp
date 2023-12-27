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
#include "IntlCollatorPrototype.h"

#if ENABLE(INTL)

#include "Error.h"
#include "IntlCollator.h"
#include "JSBoundFunction.h"
#include "JSCJSValueInlines.h"
#include "JSCellInlines.h"
#include "JSObject.h"
#include "ObjectConstructor.h"
#include "StructureInlines.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(IntlCollatorPrototype);

static EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeGetterCompare(ExecState*);
static EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeFuncResolvedOptions(ExecState*);

}

#include "IntlCollatorPrototype.lut.h"

namespace JSC {

const ClassInfo IntlCollatorPrototype::s_info = { "Object", &IntlCollator::s_info, &collatorPrototypeTable, CREATE_METHOD_TABLE(IntlCollatorPrototype) };

/* Source for IntlCollatorPrototype.lut.h
@begin collatorPrototypeTable
  compare          IntlCollatorPrototypeGetterCompare        DontEnum|Accessor
  resolvedOptions  IntlCollatorPrototypeFuncResolvedOptions  DontEnum|Function 0
@end
*/

IntlCollatorPrototype* IntlCollatorPrototype::create(VM& vm, JSGlobalObject*, Structure* structure)
{
    IntlCollatorPrototype* object = new (NotNull, allocateCell<IntlCollatorPrototype>(vm.heap)) IntlCollatorPrototype(vm, structure);
    object->finishCreation(vm);
    return object;
}

Structure* IntlCollatorPrototype::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
}

IntlCollatorPrototype::IntlCollatorPrototype(VM& vm, Structure* structure)
    : IntlCollator(vm, structure)
{
}

void IntlCollatorPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
}

bool IntlCollatorPrototype::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return getStaticFunctionSlot<JSObject>(exec, collatorPrototypeTable, jsCast<IntlCollatorPrototype*>(object), propertyName, slot);
}

EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeGetterCompare(ExecState* exec)
{
    // 10.3.3 Intl.Collator.prototype.compare (ECMA-402 2.0)
    // 1. Let collator be this Collator object.
    IntlCollator* collator = jsDynamicCast<IntlCollator*>(exec->thisValue());
    if (!collator)
        return JSValue::encode(throwTypeError(exec, ASCIILiteral("Intl.Collator.prototype.compare called on value that's not an object initialized as a Collator")));

    JSBoundFunction* boundCompare = collator->boundCompare();
    // 2. If collator.[[boundCompare]] is undefined,
    if (!boundCompare) {
        VM& vm = exec->vm();
        JSGlobalObject* globalObject = collator->globalObject();
        // a. Let F be a new built-in function object as defined in 11.3.4.
        // b. The value of F’s length property is 2.
        JSFunction* targetObject = JSFunction::create(vm, globalObject, 2, ASCIILiteral("compare"), IntlCollatorFuncCompare, NoIntrinsic);
        JSArray* boundArgs = JSArray::tryCreateUninitialized(vm, globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithUndecided), 0);
        if (!boundArgs)
            return JSValue::encode(throwOutOfMemoryError(exec));

        // c. Let bc be BoundFunctionCreate(F, «this value»).
        boundCompare = JSBoundFunction::create(vm, globalObject, targetObject, collator, boundArgs, 2, ASCIILiteral("compare"));
        // d. Set collator.[[boundCompare]] to bc.
        collator->setBoundCompare(vm, boundCompare);
    }
    // 3. Return collator.[[boundCompare]].
    return JSValue::encode(boundCompare);
}

EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeFuncResolvedOptions(ExecState* exec)
{
    // 10.3.5 Intl.Collator.prototype.resolvedOptions() (ECMA-402 2.0)
    IntlCollator* collator = jsDynamicCast<IntlCollator*>(exec->thisValue());
    if (!collator)
        return JSValue::encode(throwTypeError(exec, ASCIILiteral("Intl.Collator.prototype.resolvedOptions called on value that's not an object initialized as a Collator")));

    // The function returns a new object whose properties and attributes are set as if constructed by an object literal assigning to each of the following properties the value of the corresponding internal slot of this Collator object (see 10.4): locale, usage, sensitivity, ignorePunctuation, collation, as well as those properties shown in Table 1 whose keys are included in the %Collator%[[relevantExtensionKeys]] internal slot of the standard built-in object that is the initial value of Intl.Collator.

    JSObject* options = constructEmptyObject(exec);

    // FIXME: Populate object from internal slots.

    return JSValue::encode(options);
}

} // namespace JSC

#endif // ENABLE(INTL)
