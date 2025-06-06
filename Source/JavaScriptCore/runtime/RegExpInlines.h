/*
 *  Copyright (C) 1999-2001, 2004 Harri Porten (porten@kde.org)
 *  Copyright (c) 2007, 2008, 2016 Apple Inc. All rights reserved.
 *  Copyright (C) 2009 Torch Mobile, Inc.
 *  Copyright (C) 2010 Peter Varga (pvarga@inf.u-szeged.hu), University of Szeged
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

#pragma once

#include "RegExp.h"
#include "JSCInlines.h"
#include "Yarr.h"
#include "YarrInterpreter.h"
#include "YarrJIT.h"

#define REGEXP_FUNC_TEST_DATA_GEN 0

#if REGEXP_FUNC_TEST_DATA_GEN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

namespace JSC {

#if REGEXP_FUNC_TEST_DATA_GEN
class RegExpFunctionalTestCollector {
    // This class is not thread safe.
protected:
    static const char* const s_fileName;

public:
    static RegExpFunctionalTestCollector* get();

    ~RegExpFunctionalTestCollector();

    void outputOneTest(RegExp*, String, int, int*, int);
    void clearRegExp(RegExp* regExp)
    {
        if (regExp == m_lastRegExp)
            m_lastRegExp = 0;
    }

private:
    RegExpFunctionalTestCollector();

    void outputEscapedString(const String&, bool escapeSlash = false);

    static RegExpFunctionalTestCollector* s_instance;
    FILE* m_file;
    RegExp* m_lastRegExp;
};
#endif // REGEXP_FUNC_TEST_DATA_GEN

ALWAYS_INLINE bool RegExp::hasCodeFor(Yarr::YarrCharSize charSize)
{
    if (hasCode()) {
#if ENABLE(YARR_JIT)
        if (m_state != JITCode)
            return true;
        if ((charSize == Yarr::Char8) && (m_regExpJITCode.has8BitCode()))
            return true;
        if ((charSize == Yarr::Char16) && (m_regExpJITCode.has16BitCode()))
            return true;
#else
        UNUSED_PARAM(charSize);
        return true;
#endif
    }
    return false;
}

class PatternContextBufferHolder {
    WTF_FORBID_HEAP_ALLOCATION;
public:
    PatternContextBufferHolder(VM& vm, bool needBuffer)
        : m_vm(vm)
    {
#if ENABLE(YARR_JIT_ALL_PARENS_EXPRESSIONS)
        if (needBuffer)
            m_buffer = m_vm.acquireRegExpPatternContexBuffer();
#else
        UNUSED_PARAM(needBuffer);
#endif

    }

    ~PatternContextBufferHolder()
    {
#if ENABLE(YARR_JIT_ALL_PARENS_EXPRESSIONS)
        if (buffer())
            m_vm.releaseRegExpPatternContexBuffer();
#else
        UNUSED_PARAM(m_vm);
#endif
    }

    void* buffer() { return m_buffer; }
    unsigned size() { return buffer() ? VM::patternContextBufferSize : 0; }

private:
    VM& m_vm;
    void* m_buffer { nullptr };
};

ALWAYS_INLINE void RegExp::compileIfNecessary(VM& vm, Yarr::YarrCharSize charSize)
{
    if (hasCodeFor(charSize))
        return;

    if (m_state == ParseError)
        return;

    compile(&vm, charSize);
}

template<typename VectorType>
ALWAYS_INLINE int RegExp::matchInline(VM& vm, const String& s, unsigned startOffset, VectorType& ovector)
{
#if ENABLE(REGEXP_TRACING)
    m_rtMatchCallCount++;
    m_rtMatchTotalSubjectStringLen += (double)(s.length() - startOffset);
#endif

    compileIfNecessary(vm, s.is8Bit() ? Yarr::Char8 : Yarr::Char16);

    auto throwError = [&] {
        auto throwScope = DECLARE_THROW_SCOPE(vm);
        ExecState* exec = vm.topCallFrame;
        throwScope.throwException(exec, errorToThrow(exec));
        if (!hasHardError(m_constructionErrorCode))
            reset();
        return -1;
    };

    if (m_state == ParseError)
        return throwError();

    int offsetVectorSize = (m_numSubpatterns + 1) * 2;
    ovector.resize(offsetVectorSize);
    int* offsetVector = ovector.data();

    int result;
#if ENABLE(YARR_JIT)
    if (m_state == JITCode) {
        {
            PatternContextBufferHolder patternContextBufferHolder(vm, m_regExpJITCode.usesPatternContextBuffer());

            if (s.is8Bit())
                result = m_regExpJITCode.execute(s.characters8(), startOffset, s.length(), offsetVector, patternContextBufferHolder.buffer(), patternContextBufferHolder.size()).start;
            else
                result = m_regExpJITCode.execute(s.characters16(), startOffset, s.length(), offsetVector, patternContextBufferHolder.buffer(), patternContextBufferHolder.size()).start;
        }

        if (result == Yarr::JSRegExpJITCodeFailure) {
            // JIT'ed code couldn't handle expression, so punt back to the interpreter.
            byteCodeCompileIfNecessary(&vm);
            if (m_state == ParseError)
                return throwError();
            result = Yarr::interpret(m_regExpBytecode.get(), s, startOffset, reinterpret_cast<unsigned*>(offsetVector));
        }

#if ENABLE(YARR_JIT_DEBUG)
        matchCompareWithInterpreter(s, startOffset, offsetVector, result);
#endif
    } else
#endif
        result = Yarr::interpret(m_regExpBytecode.get(), s, startOffset, reinterpret_cast<unsigned*>(offsetVector));

    // FIXME: The YARR engine should handle unsigned or size_t length matches.
    // The YARR Interpreter is "unsigned" clean, while the YARR JIT hasn't been addressed.
    // The offset vector handling needs to change as well.
    // Right now we convert a match where the offsets overflowed into match failure.
    // There are two places in WebCore that call the interpreter directly that need to
    // have their offsets changed to int as well. They are yarr/RegularExpression.cpp
    // and inspector/ContentSearchUtilities.cpp
    if (s.length() > INT_MAX) {
        bool overflowed = false;

        if (result < -1)
            overflowed = true;

        for (unsigned i = 0; i <= m_numSubpatterns; i++) {
            if ((offsetVector[i*2] < -1) || ((offsetVector[i*2] >= 0) && (offsetVector[i*2+1] < -1))) {
                overflowed = true;
                offsetVector[i*2] = -1;
                offsetVector[i*2+1] = -1;
            }
        }

        if (overflowed)
            result = -1;
    }

    ASSERT(result >= -1);

#if REGEXP_FUNC_TEST_DATA_GEN
    RegExpFunctionalTestCollector::get()->outputOneTest(this, s, startOffset, offsetVector, result);
#endif

#if ENABLE(REGEXP_TRACING)
    if (result != -1)
        m_rtMatchFoundCount++;
#endif

    return result;
}

ALWAYS_INLINE bool RegExp::hasMatchOnlyCodeFor(Yarr::YarrCharSize charSize)
{
    if (hasCode()) {
#if ENABLE(YARR_JIT)
        if (m_state != JITCode)
            return true;
        if ((charSize == Yarr::Char8) && (m_regExpJITCode.has8BitCodeMatchOnly()))
            return true;
        if ((charSize == Yarr::Char16) && (m_regExpJITCode.has16BitCodeMatchOnly()))
            return true;
#else
        UNUSED_PARAM(charSize);
        return true;
#endif
    }

    return false;
}

ALWAYS_INLINE void RegExp::compileIfNecessaryMatchOnly(VM& vm, Yarr::YarrCharSize charSize)
{
    if (hasMatchOnlyCodeFor(charSize))
        return;

    if (m_state == ParseError)
        return;

    compileMatchOnly(&vm, charSize);
}

ALWAYS_INLINE MatchResult RegExp::matchInline(VM& vm, const String& s, unsigned startOffset)
{
#if ENABLE(REGEXP_TRACING)
    m_rtMatchOnlyCallCount++;
    m_rtMatchOnlyTotalSubjectStringLen += (double)(s.length() - startOffset);
#endif

    compileIfNecessaryMatchOnly(vm, s.is8Bit() ? Yarr::Char8 : Yarr::Char16);

    auto throwError = [&] {
        auto throwScope = DECLARE_THROW_SCOPE(vm);
        ExecState* exec = vm.topCallFrame;
        throwScope.throwException(exec, errorToThrow(exec));
        if (!hasHardError(m_constructionErrorCode))
            reset();
        return MatchResult::failed();
    };

    if (m_state == ParseError)
        return throwError();

#if ENABLE(YARR_JIT)
    MatchResult result;

    if (m_state == JITCode) {
        {
            PatternContextBufferHolder patternContextBufferHolder(vm, m_regExpJITCode.usesPatternContextBuffer());
            if (s.is8Bit())
                result = m_regExpJITCode.execute(s.characters8(), startOffset, s.length(), patternContextBufferHolder.buffer(), patternContextBufferHolder.size());
            else
                result = m_regExpJITCode.execute(s.characters16(), startOffset, s.length(), patternContextBufferHolder.buffer(), patternContextBufferHolder.size());
        }

#if ENABLE(REGEXP_TRACING)
        if (!result)
            m_rtMatchOnlyFoundCount++;
#endif
        if (result.start != static_cast<size_t>(Yarr::JSRegExpJITCodeFailure))
            return result;

        // JIT'ed code couldn't handle expression, so punt back to the interpreter.
        byteCodeCompileIfNecessary(&vm);
        if (m_state == ParseError)
            return throwError();
    }
#endif

    int offsetVectorSize = (m_numSubpatterns + 1) * 2;
    int* offsetVector;
    Vector<int, 32> nonReturnedOvector;
    nonReturnedOvector.grow(offsetVectorSize);
    offsetVector = nonReturnedOvector.data();
    int r = Yarr::interpret(m_regExpBytecode.get(), s, startOffset, reinterpret_cast<unsigned*>(offsetVector));
#if REGEXP_FUNC_TEST_DATA_GEN
    RegExpFunctionalTestCollector::get()->outputOneTest(this, s, startOffset, offsetVector, result);
#endif

    if (r >= 0) {
#if ENABLE(REGEXP_TRACING)
        m_rtMatchOnlyFoundCount++;
#endif
        return MatchResult(r, reinterpret_cast<unsigned*>(offsetVector)[1]);
    }

    return MatchResult::failed();
}

} // namespace JSC
