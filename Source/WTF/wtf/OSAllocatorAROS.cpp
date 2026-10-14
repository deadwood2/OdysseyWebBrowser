/*
 * Copyright (C) 2012 Krzysztof Smiechowicz. All rights reserved.
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
#include "OSAllocator.h"

#if OS(AROS)

#include "mui/execallocator.h"
#include "mui/arosbailout.h"

namespace WTF {

static void* allocateWithCheck(size_t bytes, bool executable)
{
    void * ptr = allocator_getmem_page_aligned(bytes, executable);
    if (likely(ptr))
        return ptr;

    if (aros_is_memory_bailout())
        aros_bailout_jump();

    return nullptr;
}

void* OSAllocator::reserveUncommitted(size_t bytes, Usage, bool, bool executable, bool, bool)
{
    return allocateWithCheck(bytes, executable);
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage, bool, bool executable, bool, bool)
{
    return allocateWithCheck(bytes, executable);
}

// try* variants (2.46): may fail and return null; no out-of-memory bailout.
void* OSAllocator::tryReserveUncommitted(size_t bytes, Usage, bool, bool executable, bool, bool)
{
    return allocator_getmem_page_aligned(bytes, executable);
}

void* OSAllocator::tryReserveAndCommit(size_t bytes, Usage, bool, bool executable, bool, bool)
{
    return allocator_getmem_page_aligned(bytes, executable);
}

// Used for JSC's structure heap. AROS has no virtual memory: this allocates
// (and zeroes) real memory, so its size is kept small for PLATFORM(MUI) with
// STRUCTURE_HEAP_ADDRESS_SIZE_IN_MB (OptionsMUI.cmake). Not executable.
void* OSAllocator::tryReserveUncommittedAligned(size_t bytes, size_t alignment, Usage, bool, bool executable, bool, bool)
{
    if (executable)
        return nullptr;
    return allocator_getmem_aligned(bytes, alignment);
}

// No memory protection on AROS.
void OSAllocator::protect(void*, size_t, bool, bool)
{
}

void OSAllocator::commit(void* address, size_t bytes, bool, bool)
{
    memset(address, 0, bytes);
}

void OSAllocator::decommit(void* address, size_t bytes)
{
    (void)address;
    (void)bytes;
}

void OSAllocator::releaseDecommitted(void* address, size_t bytes)
{
    allocator_freemem(address, bytes);
}

} // namespace WTF

#endif
