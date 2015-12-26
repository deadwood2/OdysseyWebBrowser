/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestInterfaceLeadingUnderscore.h"

#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMWrapperCache.h"
#include <runtime/FunctionPrototype.h>
#include <runtime/JSCInlines.h>
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

// Attributes

JSC::EncodedJSValue jsTestInterfaceLeadingUnderscoreConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestInterfaceLeadingUnderscoreConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::EncodedJSValue);
JSC::EncodedJSValue jsTestInterfaceLeadingUnderscoreReadonly(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);

class JSTestInterfaceLeadingUnderscorePrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestInterfaceLeadingUnderscorePrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestInterfaceLeadingUnderscorePrototype* ptr = new (NotNull, JSC::allocateCell<JSTestInterfaceLeadingUnderscorePrototype>(vm.heap)) JSTestInterfaceLeadingUnderscorePrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestInterfaceLeadingUnderscorePrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

using JSTestInterfaceLeadingUnderscoreConstructor = JSDOMConstructorNotConstructable<JSTestInterfaceLeadingUnderscore>;

template<> JSValue JSTestInterfaceLeadingUnderscoreConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestInterfaceLeadingUnderscoreConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestInterfaceLeadingUnderscore::prototype(vm, globalObject), DontDelete | ReadOnly | DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String(ASCIILiteral("TestInterfaceLeadingUnderscore"))), ReadOnly | DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), ReadOnly | DontEnum);
}

template<> const ClassInfo JSTestInterfaceLeadingUnderscoreConstructor::s_info = { "TestInterfaceLeadingUnderscore", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestInterfaceLeadingUnderscoreConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestInterfaceLeadingUnderscorePrototypeTableValues[] =
{
    { "constructor", DontEnum, NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestInterfaceLeadingUnderscoreConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestInterfaceLeadingUnderscoreConstructor) } },
    { "readonly", ReadOnly | CustomAccessor | DOMAttribute, NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestInterfaceLeadingUnderscoreReadonly), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) } },
};

const ClassInfo JSTestInterfaceLeadingUnderscorePrototype::s_info = { "TestInterfaceLeadingUnderscorePrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestInterfaceLeadingUnderscorePrototype) };

void JSTestInterfaceLeadingUnderscorePrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestInterfaceLeadingUnderscore::info(), JSTestInterfaceLeadingUnderscorePrototypeTableValues, *this);
}

const ClassInfo JSTestInterfaceLeadingUnderscore::s_info = { "TestInterfaceLeadingUnderscore", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestInterfaceLeadingUnderscore) };

JSTestInterfaceLeadingUnderscore::JSTestInterfaceLeadingUnderscore(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestInterfaceLeadingUnderscore>&& impl)
    : JSDOMWrapper<TestInterfaceLeadingUnderscore>(structure, globalObject, WTFMove(impl))
{
}

void JSTestInterfaceLeadingUnderscore::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestInterfaceLeadingUnderscore::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestInterfaceLeadingUnderscorePrototype::create(vm, &globalObject, JSTestInterfaceLeadingUnderscorePrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestInterfaceLeadingUnderscore::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestInterfaceLeadingUnderscore>(vm, globalObject);
}

JSValue JSTestInterfaceLeadingUnderscore::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestInterfaceLeadingUnderscoreConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestInterfaceLeadingUnderscore::destroy(JSC::JSCell* cell)
{
    JSTestInterfaceLeadingUnderscore* thisObject = static_cast<JSTestInterfaceLeadingUnderscore*>(cell);
    thisObject->JSTestInterfaceLeadingUnderscore::~JSTestInterfaceLeadingUnderscore();
}

template<> inline JSTestInterfaceLeadingUnderscore* IDLAttribute<JSTestInterfaceLeadingUnderscore>::cast(ExecState& state, EncodedJSValue thisValue)
{
    return jsDynamicDowncast<JSTestInterfaceLeadingUnderscore*>(state.vm(), JSValue::decode(thisValue));
}

EncodedJSValue jsTestInterfaceLeadingUnderscoreConstructor(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestInterfaceLeadingUnderscorePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(state, throwScope);
    return JSValue::encode(JSTestInterfaceLeadingUnderscore::getConstructor(state->vm(), prototype->globalObject()));
}

bool setJSTestInterfaceLeadingUnderscoreConstructor(ExecState* state, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicDowncast<JSTestInterfaceLeadingUnderscorePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(state, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(state->vm(), state->propertyNames().constructor, JSValue::decode(encodedValue));
}

static inline JSValue jsTestInterfaceLeadingUnderscoreReadonlyGetter(ExecState& state, JSTestInterfaceLeadingUnderscore& thisObject, ThrowScope& throwScope)
{
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(state);
    auto& impl = thisObject.wrapped();
    JSValue result = toJS<IDLDOMString>(state, impl.readonly());
    return result;
}

EncodedJSValue jsTestInterfaceLeadingUnderscoreReadonly(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    return IDLAttribute<JSTestInterfaceLeadingUnderscore>::get<jsTestInterfaceLeadingUnderscoreReadonlyGetter, CastedThisErrorBehavior::Assert>(*state, thisValue, "readonly");
}

bool JSTestInterfaceLeadingUnderscoreOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    return false;
}

void JSTestInterfaceLeadingUnderscoreOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestInterfaceLeadingUnderscore = static_cast<JSTestInterfaceLeadingUnderscore*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestInterfaceLeadingUnderscore->wrapped(), jsTestInterfaceLeadingUnderscore);
}

JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject* globalObject, Ref<TestInterfaceLeadingUnderscore>&& impl)
{
    // If you hit this failure the interface definition has the ImplementationLacksVTable
    // attribute. You should remove that attribute. If the class has subclasses
    // that may be passed through this toJS() function you should use the SkipVTableValidation
    // attribute to TestInterfaceLeadingUnderscore.
    static_assert(!std::is_polymorphic<TestInterfaceLeadingUnderscore>::value, "TestInterfaceLeadingUnderscore is polymorphic but the IDL claims it is not");
    return createWrapper<TestInterfaceLeadingUnderscore>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestInterfaceLeadingUnderscore& impl)
{
    return wrap(state, globalObject, impl);
}

TestInterfaceLeadingUnderscore* JSTestInterfaceLeadingUnderscore::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicDowncast<JSTestInterfaceLeadingUnderscore*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}