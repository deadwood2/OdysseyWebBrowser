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

#pragma once

#if ENABLE(INDEXED_DATABASE)

#include "EventTarget.h"
#include "IDBActiveDOMObject.h"
#include "IDBError.h"
#include "IDBResourceIdentifier.h"
#include "IndexedDB.h"
#include <heap/Strong.h>

namespace WebCore {

class DOMError;
class Event;
class IDBCursor;
class IDBIndex;
class IDBKeyData;
class IDBObjectStore;
class IDBResultData;
class IDBValue;
class ScopeGuard;
class ThreadSafeDataBuffer;

namespace IDBClient {
class IDBConnectionProxy;
class IDBConnectionToServer;
}

class IDBRequest : public EventTargetWithInlineData, public IDBActiveDOMObject, public RefCounted<IDBRequest> {
public:
    static Ref<IDBRequest> create(ScriptExecutionContext&, IDBObjectStore&, IDBTransaction&);
    static Ref<IDBRequest> create(ScriptExecutionContext&, IDBCursor&, IDBTransaction&);
    static Ref<IDBRequest> createCount(ScriptExecutionContext&, IDBIndex&, IDBTransaction&);
    static Ref<IDBRequest> createGet(ScriptExecutionContext&, IDBIndex&, IndexedDB::IndexRecordType, IDBTransaction&);

    const IDBResourceIdentifier& resourceIdentifier() const { return m_resourceIdentifier; }

    virtual ~IDBRequest();

    IDBCursor* cursorResult() const { return m_cursorResult.get(); }
    IDBDatabase* databaseResult() const { return m_databaseResult.get(); }
    JSC::JSValue scriptResult() const { return m_scriptResult.get(); }
    unsigned short errorCode(ExceptionCode&) const;
    RefPtr<DOMError> error(ExceptionCodeWithMessage&) const;
    IDBObjectStore* objectStoreSource() const { return m_objectStoreSource.get(); }
    IDBIndex* indexSource() const { return m_indexSource.get(); }
    IDBCursor* cursorSource() const { return m_cursorSource.get(); }
    RefPtr<IDBTransaction> transaction() const;
    const String& readyState() const;

    bool isDone() const { return m_isDone; }

    uint64_t sourceObjectStoreIdentifier() const;
    uint64_t sourceIndexIdentifier() const;
    IndexedDB::IndexRecordType requestedIndexRecordType() const;

    ScriptExecutionContext* scriptExecutionContext() const final { return ActiveDOMObject::scriptExecutionContext(); }

    using RefCounted::ref;
    using RefCounted::deref;

    void requestCompleted(const IDBResultData&);

    void setResult(const IDBKeyData&);
    void setResult(uint64_t);
    void setResultToStructuredClone(const IDBValue&);
    void setResultToUndefined();

    void willIterateCursor(IDBCursor&);
    void didOpenOrIterateCursor(const IDBResultData&);

    const IDBCursor* pendingCursor() const { return m_pendingCursor.get(); }

    void setSource(IDBCursor&);
    void setVersionChangeTransaction(IDBTransaction&);

    IndexedDB::RequestType requestType() const { return m_requestType; }

    bool hasPendingActivity() const final;

protected:
    IDBRequest(ScriptExecutionContext&, IDBClient::IDBConnectionProxy&);

    void enqueueEvent(Ref<Event>&&);
    bool dispatchEvent(Event&) override;

    void setResult(Ref<IDBDatabase>&&);

    IDBClient::IDBConnectionProxy& connectionProxy() { return m_connectionProxy.get(); }

    // FIXME: Protected data members aren't great for maintainability.
    // Consider adding protected helper functions and making these private.
    bool m_isDone { false };
    RefPtr<IDBTransaction> m_transaction;
    bool m_shouldExposeTransactionToDOM { true };
    RefPtr<DOMError> m_domError;
    IndexedDB::RequestType m_requestType { IndexedDB::RequestType::Other };
    bool m_contextStopped { false };
    Event* m_openDatabaseSuccessEvent { nullptr };

private:
    IDBRequest(ScriptExecutionContext&, IDBObjectStore&, IDBTransaction&);
    IDBRequest(ScriptExecutionContext&, IDBCursor&, IDBTransaction&);
    IDBRequest(ScriptExecutionContext&, IDBIndex&, IDBTransaction&);
    IDBRequest(ScriptExecutionContext&, IDBIndex&, IndexedDB::IndexRecordType, IDBTransaction&);

    void clearResult();

    EventTargetInterface eventTargetInterface() const override;

    const char* activeDOMObjectName() const final;
    bool canSuspendForDocumentSuspension() const final;
    void stop() final;
    virtual void cancelForStop();

    void refEventTarget() final { RefCounted::ref(); }
    void derefEventTarget() final { RefCounted::deref(); }
    void uncaughtExceptionInEventHandler() final;

    virtual bool isOpenDBRequest() const { return false; }

    void onError();
    void onSuccess();

    IDBCursor* resultCursor();

    // Could consider storing these three in a union or union-like class instead.
    JSC::Strong<JSC::Unknown> m_scriptResult;
    RefPtr<IDBCursor> m_cursorResult;
    RefPtr<IDBDatabase> m_databaseResult;

    IDBError m_idbError;
    IDBResourceIdentifier m_resourceIdentifier;

    // Could consider storing these three in a union or union-like class instead.
    RefPtr<IDBObjectStore> m_objectStoreSource;
    RefPtr<IDBIndex> m_indexSource;
    RefPtr<IDBCursor> m_cursorSource;

    bool m_hasPendingActivity { true };
    IndexedDB::IndexRecordType m_requestedIndexRecordType;

    RefPtr<IDBCursor> m_pendingCursor;

    std::unique_ptr<ScopeGuard> m_cursorRequestNotifier;

    Ref<IDBClient::IDBConnectionProxy> m_connectionProxy;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
