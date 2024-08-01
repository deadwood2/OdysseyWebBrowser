/*
 * Copyright (C) 2016 Canon, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY CANON INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CANON INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSKeyValueIterator_h
#define JSKeyValueIterator_h

#include "JSDOMBinding.h"
#include <runtime/JSDestructibleObject.h>
#include <type_traits>

namespace WebCore {

template<typename JSWrapper>
class JSKeyValueIteratorPrototype : public JSC::JSNonFinalObject {
public:
    using DOMWrapped = typename std::remove_reference<decltype(std::declval<JSWrapper>().wrapped())>::type;
    using Base = JSC::JSNonFinalObject;

    static JSKeyValueIteratorPrototype* create(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSKeyValueIteratorPrototype* prototype = new (NotNull, JSC::allocateCell<JSKeyValueIteratorPrototype>(vm.heap)) JSKeyValueIteratorPrototype(vm, structure);
        prototype->finishCreation(vm, globalObject);
        return prototype;
    }

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

    static JSC::EncodedJSValue JSC_HOST_CALL next(JSC::ExecState*);

private:
    JSKeyValueIteratorPrototype(JSC::VM& vm, JSC::Structure* structure) : Base(vm, structure) { }

    void finishCreation(JSC::VM&, JSC::JSGlobalObject*);
};

enum class IterationKind { Key, Value, KeyValue };

template<typename JSWrapper>
class JSKeyValueIterator: public JSDOMObject {
public:
    using DOMWrapped = typename std::remove_reference<decltype(std::declval<JSWrapper>().wrapped())>::type;
    using Base = JSDOMObject;

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

    static JSKeyValueIterator* create(JSC::VM& vm, JSC::Structure* structure, JSWrapper& iteratedObject, IterationKind kind)
    {
        JSKeyValueIterator* instance = new (NotNull, JSC::allocateCell<JSKeyValueIterator>(vm.heap)) JSKeyValueIterator(structure, iteratedObject, kind);
        instance->finishCreation(vm);
        return instance;
    }

    static JSKeyValueIteratorPrototype<JSWrapper>* createPrototype(JSC::VM& vm, JSC::JSGlobalObject* globalObject)
    {
        return JSKeyValueIteratorPrototype<JSWrapper>::create(vm, globalObject,
            JSKeyValueIteratorPrototype<JSWrapper>::createStructure(vm, globalObject, globalObject->objectPrototype()));
    }

    bool next(JSC::ExecState&, JSC::JSValue&);

private:
    JSKeyValueIterator(JSC::Structure* structure, JSWrapper& iteratedObject, IterationKind kind)
        : Base(structure, *iteratedObject.globalObject())
        , m_iterator(iteratedObject.wrapped().createIterator())
        , m_kind(kind)
    {
    }

    static void destroy(JSC::JSCell*);

    typename DOMWrapped::Iterator m_iterator;
    IterationKind m_kind;
};

template<typename JSWrapper>
JSKeyValueIterator<JSWrapper>* createIterator(JSDOMGlobalObject& globalObject, JSWrapper& wrapper, IterationKind kind)
{
    return JSKeyValueIterator<JSWrapper>::create(globalObject.vm(), getDOMStructure<JSKeyValueIterator<JSWrapper>>(globalObject.vm(), globalObject), wrapper, kind);
}

template<typename JSWrapper>
void JSKeyValueIterator<JSWrapper>::destroy(JSCell* cell)
{
    JSKeyValueIterator<JSWrapper>* thisObject = JSC::jsCast<JSKeyValueIterator<JSWrapper>*>(cell);
    thisObject->JSKeyValueIterator<JSWrapper>::~JSKeyValueIterator();
}

template<typename JSWrapper>
bool JSKeyValueIterator<JSWrapper>::next(JSC::ExecState& state, JSC::JSValue& value)
{
    typename DOMWrapped::Iterator::Key nextKey;
    typename DOMWrapped::Iterator::Value nextValue;
    if (m_iterator.next(nextKey, nextValue)) {
        value = JSC::jsUndefined();
        return true;
    }
    if (m_kind == IterationKind::Value)
        value = toJS(&state, globalObject(), nextValue);
    else if (m_kind == IterationKind::Key)
        value = toJS(&state, globalObject(), nextKey);
    else
        value = jsPair(state, globalObject(), nextKey, nextValue);
    return false;
}

template<typename JSWrapper>
JSC::EncodedJSValue JSC_HOST_CALL JSKeyValueIteratorPrototype<JSWrapper>::next(JSC::ExecState* state)
{
    JSKeyValueIterator<JSWrapper>* iterator = JSC::jsDynamicCast<JSKeyValueIterator<JSWrapper>*>(state->thisValue());
    if (!iterator)
        return JSC::JSValue::encode(throwTypeError(state, ASCIILiteral("Cannot call next() on a non-Iterator object")));

    JSC::JSValue result;
    bool isDone = iterator->next(*state, result);
    return JSC::JSValue::encode(createIteratorResultObject(state, result, isDone));
}

template<typename JSWrapper>
void JSKeyValueIteratorPrototype<JSWrapper>::finishCreation(JSC::VM& vm, JSC::JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));

    JSC_NATIVE_INTRINSIC_FUNCTION_WITHOUT_TRANSITION(vm.propertyNames->next, next, JSC::DontEnum, 0, JSC::NoIntrinsic);
}

}

#endif // !defined(JSKeyValueIterator_h)
