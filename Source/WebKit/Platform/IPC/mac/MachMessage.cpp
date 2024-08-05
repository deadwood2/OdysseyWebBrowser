/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#include "MachMessage.h"

#include <mach/mach.h>

namespace IPC {

std::unique_ptr<MachMessage> MachMessage::create(size_t size)
{
    void* memory = WTF::fastMalloc(sizeof(MachMessage) + size);
    return std::unique_ptr<MachMessage> { new (NotNull, memory) MachMessage { size } };
}

MachMessage::MachMessage(size_t size)
    : m_size { size }
    , m_shouldFreeDescriptors { true }
{
}

MachMessage::~MachMessage()
{
    if (m_shouldFreeDescriptors)
        ::mach_msg_destroy(header());
}

size_t MachMessage::messageSize(size_t bodySize, size_t portDescriptorCount, size_t memoryDescriptorCount)
{
    size_t messageSize = sizeof(mach_msg_header_t) + bodySize;

    if (portDescriptorCount || memoryDescriptorCount) {
        messageSize += sizeof(mach_msg_body_t);

        if (portDescriptorCount)
            messageSize += (portDescriptorCount * sizeof(mach_msg_port_descriptor_t));
        if (memoryDescriptorCount)
            messageSize += (memoryDescriptorCount * sizeof(mach_msg_ool_descriptor_t));
    }

    return round_msg(messageSize);
}

mach_msg_header_t* MachMessage::header()
{
    return reinterpret_cast<mach_msg_header_t*>(m_buffer);
}

void MachMessage::leakDescriptors()
{
    m_shouldFreeDescriptors = false;
}

}
