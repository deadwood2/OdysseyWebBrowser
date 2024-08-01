/*
 * Copyright (C) 2015 Yusuke Suzuki <utatane.tea@gmail.com>.
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

#ifndef JSGeneratorFunction_h
#define JSGeneratorFunction_h

#include "JSFunction.h"

namespace JSC {

class JSGlobalObject;
class LLIntOffsetsExtractor;
class LLIntDesiredOffsets;

class JSGeneratorFunction : public JSFunction {
    friend class JIT;
#if ENABLE(DFG_JIT)
    friend class DFG::SpeculativeJIT;
    friend class DFG::JITCompiler;
#endif
    friend class VM;
public:
    typedef JSFunction Base;

    enum class GeneratorResumeMode : int32_t {
        NormalMode = 0,
        ReturnMode = 1,
        ThrowMode = 2
    };

    const static unsigned StructureFlags = Base::StructureFlags;

    DECLARE_EXPORT_INFO;

    static JSGeneratorFunction* create(VM&, FunctionExecutable*, JSScope*);
    static JSGeneratorFunction* createWithInvalidatedReallocationWatchpoint(VM&, FunctionExecutable*, JSScope*);

    static size_t allocationSize(size_t inlineCapacity)
    {
        ASSERT_UNUSED(inlineCapacity, !inlineCapacity);
        return sizeof(JSGeneratorFunction);
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        ASSERT(globalObject);
        return Structure::create(vm, globalObject, prototype, TypeInfo(JSFunctionType, StructureFlags), info());
    }

private:
    JSGeneratorFunction(VM&, FunctionExecutable*, JSScope*);

    static JSGeneratorFunction* createImpl(VM&, FunctionExecutable*, JSScope*);

    friend class LLIntOffsetsExtractor;
};

} // namespace JSC

#endif // JSGeneratorFunction_h
