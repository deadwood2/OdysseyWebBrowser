/*
 * Copyright (C) 2013, 2016 Apple Inc. All rights reserved.
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
#include "JSCryptoOperationData.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "JSDOMConvertBufferSource.h"
#include <heap/HeapInlines.h>

using namespace JSC;

namespace WebCore {

CryptoOperationData cryptoOperationDataFromJSValue(ExecState& state, ThrowScope& scope, JSValue value)
{
    VM& vm = state.vm();
    if (auto* buffer = toUnsharedArrayBuffer(vm, value))
        return { static_cast<uint8_t*>(buffer->data()), buffer->byteLength() };

    if (auto bufferView = toUnsharedArrayBufferView(vm, value))
        return { static_cast<uint8_t*>(bufferView->baseAddress()), bufferView->byteLength() };

    throwTypeError(&state, scope, ASCIILiteral("Only ArrayBuffer and ArrayBufferView objects can be passed as CryptoOperationData"));
    return { };
}

} // namespace WebCore

#endif // ENABLE(SUBTLE_CRYPTO)
