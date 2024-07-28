/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#include "JSNodeFilter.h"

#include "JSCallbackData.h"
#include "JSNode.h"
#include "NodeFilter.h"

namespace WebCore {

using namespace JSC;

// FIXME: The bindings generator is currently not able to generate
// callback function calls if they return something other than a
// boolean.
uint16_t JSNodeFilter::acceptNode(Node* node)
{
    Ref<JSNodeFilter> protect(*this);

    JSLockHolder lock(m_data->globalObject()->vm());

    ExecState* exec = m_data->globalObject()->globalExec();
    MarkedArgumentBuffer args;
    args.append(toJS(exec, m_data->globalObject(), node));
    if (exec->hadException())
        return NodeFilter::FILTER_REJECT;

    NakedPtr<Exception> returnedException;
    JSValue value = m_data->invokeCallback(args, JSCallbackData::CallbackType::FunctionOrObject, Identifier::fromString(exec, "acceptNode"), returnedException);
    if (returnedException) {
        // Rethrow exception.
        exec->vm().throwException(exec, returnedException);

        return NodeFilter::FILTER_REJECT;
    }

    uint16_t result = toUInt16(exec, value, NormalConversion);
    if (exec->hadException())
        return NodeFilter::FILTER_REJECT;

    return result;
}

} // namespace WebCore
