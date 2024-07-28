/*
 * Copyright (C) 2015, 2016 Apple Inc. All rights reserved.
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

#ifndef JSBoundSlotBaseFunction_h
#define JSBoundSlotBaseFunction_h

#include "JSFunction.h"

namespace JSC {

class CustomGetterSetter;

EncodedJSValue JSC_HOST_CALL boundSlotBaseFunctionCall(ExecState*);

class JSBoundSlotBaseFunction : public JSFunction {
public:
    typedef JSFunction Base;

    // The Type is set to the number of arguments the resultant function will have.
    enum class Type { Getter = 0, Setter = 1 };

    static const unsigned StructureFlags = Base::StructureFlags;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        ASSERT(globalObject);
        return Structure::create(vm, globalObject, prototype, TypeInfo(JSFunctionType, StructureFlags), info());
    }

    static JSBoundSlotBaseFunction* create(VM&, JSGlobalObject*, JSObject*, CustomGetterSetter*, const Type, const String&);

    JSObject* boundSlotBase() const { return m_boundSlotBase.get(); }
    CustomGetterSetter* customGetterSetter() const { return m_getterSetter.get(); }
    bool isSetter() const { return m_type == Type::Setter; }

    DECLARE_EXPORT_INFO;

protected:
    static void visitChildren(JSCell*, SlotVisitor&);

private:
    JSBoundSlotBaseFunction(VM&, JSGlobalObject*, Structure*, Type);

    void finishCreation(VM&, NativeExecutable*, JSObject*, CustomGetterSetter*, const String&);

    WriteBarrier<JSObject> m_boundSlotBase;
    WriteBarrier<CustomGetterSetter> m_getterSetter;
    Type m_type;
};

} // namespace JSC

#endif // JSBoundSlotBaseFunction_h
