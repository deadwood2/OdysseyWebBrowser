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

#include <exec/lists.h>
#include <string.h>
#include <aros/debug.h>

#if OS(AROS)

#include "mui/execallocator.h"
#include "mui/arosbailout.h"

namespace WTF {

static void* allocateWithCheck(size_t bytes)
{
    void * ptr = allocator_getmem_page_aligned(bytes);
    if (likely(ptr))
        return ptr;

    if (aros_is_memory_bailout())
        aros_bailout_jump();

    return nullptr;
}

void* OSAllocator::reserveUncommitted(size_t bytes, Usage, bool, bool, bool)
{
    return allocateWithCheck(bytes);
}

void* OSAllocator::reserveAndCommit(size_t bytes, Usage, bool, bool, bool)
{
    return allocateWithCheck(bytes);
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
