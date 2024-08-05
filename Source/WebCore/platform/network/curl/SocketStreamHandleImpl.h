/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "SocketStreamHandle.h"

#if PLATFORM(WIN)
#include <winsock2.h>
#endif

#include "SessionID.h"
#include <curl/curl.h>
#include <wtf/Deque.h>
#include <wtf/Lock.h>
#include <wtf/RefCounted.h>
#include <wtf/StreamBuffer.h>
#include <wtf/Threading.h>

namespace WebCore {

class SocketStreamHandleClient;

class SocketStreamHandleImpl : public SocketStreamHandle {
public:
    static Ref<SocketStreamHandleImpl> create(const URL& url, SocketStreamHandleClient& client, SessionID, const String&, SourceApplicationAuditToken&&) { return adoptRef(*new SocketStreamHandleImpl(url, client)); }

    virtual ~SocketStreamHandleImpl();

    void platformSend(const char* data, size_t length, Function<void(bool)>&&) final;
    void platformClose() final;
private:
    SocketStreamHandleImpl(const URL&, SocketStreamHandleClient&);

    size_t bufferedAmount() final;
    std::optional<size_t> platformSendInternal(const char*, size_t);
    bool sendPendingData();

    bool readData(CURL*);
    bool sendData(CURL*);
    bool waitForAvailableData(CURL*, std::chrono::milliseconds selectTimeout);

    void startThread();
    void stopThread();

    void didReceiveData();
    void didOpenSocket();

    struct SocketData {
        SocketData(std::unique_ptr<char[]>&& source, size_t length)
        {
            data = WTFMove(source);
            size = length;
        }

        SocketData(SocketData&& other)
        {
            data = WTFMove(other.data);
            size = other.size;
            other.size = 0;
        }

        std::unique_ptr<char[]> data;
        size_t size { 0 };
    };

    RefPtr<Thread> m_workerThread;
    std::atomic<bool> m_stopThread { false };
    Lock m_mutexSend;
    Lock m_mutexReceive;
    Deque<SocketData> m_sendData;
    Deque<SocketData> m_receiveData;

    StreamBuffer<char, 1024 * 1024> m_buffer;
    static const unsigned maxBufferSize = 100 * 1024 * 1024;
};

} // namespace WebCore
