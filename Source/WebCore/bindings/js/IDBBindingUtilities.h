/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBBindingUtilities_h
#define IDBBindingUtilities_h

#if ENABLE(INDEXED_DATABASE)

#include "Dictionary.h"
#include <bindings/ScriptValue.h>
#include <wtf/Forward.h>

namespace WebCore {

class DOMRequestState;
class IDBIndexInfo;
class IDBKey;
class IDBKeyData;
class IDBKeyPath;
class IndexKey;
class SharedBuffer;
class ThreadSafeDataBuffer;

IDBKeyPath idbKeyPathFromValue(JSC::ExecState*, JSC::JSValue);

bool injectIDBKeyIntoScriptValue(DOMRequestState*, PassRefPtr<IDBKey>, Deprecated::ScriptValue&, const IDBKeyPath&);
bool injectIDBKeyIntoScriptValue(JSC::ExecState&, const IDBKeyData&, JSC::JSValue, const IDBKeyPath&);

RefPtr<IDBKey> createIDBKeyFromScriptValueAndKeyPath(JSC::ExecState*, const Deprecated::ScriptValue&, const IDBKeyPath&);
RefPtr<IDBKey> maybeCreateIDBKeyFromScriptValueAndKeyPath(JSC::ExecState&, const Deprecated::ScriptValue&, const IDBKeyPath&);
RefPtr<IDBKey> maybeCreateIDBKeyFromScriptValueAndKeyPath(JSC::ExecState&, const JSC::JSValue&, const IDBKeyPath&);

bool canInjectIDBKeyIntoScriptValue(DOMRequestState*, const JSC::JSValue&, const IDBKeyPath&);
bool canInjectIDBKeyIntoScriptValue(JSC::ExecState&, const JSC::JSValue&, const IDBKeyPath&);

Deprecated::ScriptValue deserializeIDBValue(DOMRequestState*, PassRefPtr<SerializedScriptValue>);
Deprecated::ScriptValue deserializeIDBValueData(ScriptExecutionContext&, const ThreadSafeDataBuffer& valueData);
Deprecated::ScriptValue deserializeIDBValueBuffer(DOMRequestState*, PassRefPtr<SharedBuffer>, bool keyIsDefined);
WEBCORE_EXPORT Deprecated::ScriptValue deserializeIDBValueBuffer(JSC::ExecState*, Vector<uint8_t>&&, bool keyIsDefined);

JSC::JSValue deserializeIDBValueDataToJSValue(JSC::ExecState&, const ThreadSafeDataBuffer& valueData);

Deprecated::ScriptValue idbKeyToScriptValue(DOMRequestState*, PassRefPtr<IDBKey>);
RefPtr<IDBKey> scriptValueToIDBKey(DOMRequestState*, const JSC::JSValue&);
RefPtr<IDBKey> scriptValueToIDBKey(JSC::ExecState&, const JSC::JSValue&);

Deprecated::ScriptValue idbKeyDataToScriptValue(ScriptExecutionContext*, const IDBKeyData&);

JSC::JSValue idbValueDataToJSValue(JSC::ExecState&, const ThreadSafeDataBuffer& valueData);
void generateIndexKeyForValue(JSC::ExecState&, const IDBIndexInfo&, JSC::JSValue, IndexKey& outKey);
JSC::JSValue idbKeyDataToJSValue(JSC::ExecState&, const IDBKeyData&);

}

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBBindingUtilities_h
