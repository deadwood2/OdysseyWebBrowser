/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefCounted.h>

#if USE(UNIX_DOMAIN_SOCKETS)
#include "Attachment.h"
#include <wtf/Optional.h>
#endif

namespace IPC {
class Decoder;
class Encoder;
}

#if OS(DARWIN)
namespace WebCore {
class MachSendRight;
}
#endif

namespace WebKit {

class SharedMemory : public RefCounted<SharedMemory> {
public:
    enum class Protection {
        ReadOnly,
        ReadWrite
    };

    class Handle {
        WTF_MAKE_NONCOPYABLE(Handle);
    public:
        Handle();
        ~Handle();

        bool isNull() const;

        void clear();

        void encode(IPC::Encoder&) const;
        static bool decode(IPC::Decoder&, Handle&);

#if USE(UNIX_DOMAIN_SOCKETS)
        IPC::Attachment releaseAttachment() const;
        void adoptAttachment(IPC::Attachment&&);
#endif
    private:
        friend class SharedMemory;
#if USE(UNIX_DOMAIN_SOCKETS)
        mutable IPC::Attachment m_attachment;
#elif OS(DARWIN)
        mutable mach_port_t m_port;
        size_t m_size;
#elif OS(WINDOWS)
        mutable HANDLE m_handle;
        size_t m_size;
#endif
    };

    static RefPtr<SharedMemory> allocate(size_t);
    static RefPtr<SharedMemory> create(void*, size_t, Protection);
    static RefPtr<SharedMemory> map(const Handle&, Protection);
#if USE(UNIX_DOMAIN_SOCKETS)
    static RefPtr<SharedMemory> wrapMap(void*, size_t, int fileDescriptor);
#endif
#if OS(WINDOWS)
    static RefPtr<SharedMemory> adopt(HANDLE, size_t, Protection);
#endif

    ~SharedMemory();

    bool createHandle(Handle&, Protection);

    size_t size() const { return m_size; }
    void* data() const
    {
        ASSERT(m_data);
        return m_data;
    }

#if OS(WINDOWS)
    HANDLE handle() const { return m_handle; }
#endif

    // Return the system page size in bytes.
    static unsigned systemPageSize();

private:
#if OS(DARWIN)
    WebCore::MachSendRight createSendRight(Protection) const;
#endif

    size_t m_size;
    void* m_data;
#if PLATFORM(COCOA)
    Protection m_protection;
#endif

#if USE(UNIX_DOMAIN_SOCKETS)
    std::optional<int> m_fileDescriptor;
    bool m_isWrappingMap { false };
#elif OS(DARWIN)
    mach_port_t m_port;
#elif OS(WINDOWS)
    HANDLE m_handle;
#endif
};

};
