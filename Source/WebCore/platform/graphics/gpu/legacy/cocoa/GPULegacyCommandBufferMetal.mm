/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "GPULegacyCommandBuffer.h"

#if ENABLE(WEBMETAL)

#import "GPULegacyCommandQueue.h"
#import "GPULegacyDrawable.h"
#import "Logging.h"
#import <Metal/Metal.h>
#import <wtf/BlockPtr.h>
#import <wtf/MainThread.h>

namespace WebCore {

GPULegacyCommandBuffer::GPULegacyCommandBuffer(const GPULegacyCommandQueue& queue, Function<void()>&& completedCallback)
{
    LOG(WebMetal, "GPULegacyCommandBuffer::GPULegacyCommandBuffer()");

    m_metal = [queue.metal() commandBuffer];

    [m_metal addCompletedHandler:makeBlockPtr([completedCallback = WTFMove(completedCallback)] (id<MTLCommandBuffer>) mutable {
        callOnMainThread(WTFMove(completedCallback));
    }).get()];
}

void GPULegacyCommandBuffer::presentDrawable(GPULegacyDrawable& drawable) const
{
    if (!m_metal || !drawable.metal())
        return;

    [m_metal presentDrawable:drawable.metal()];
    drawable.release();
}

void GPULegacyCommandBuffer::commit() const
{
    [m_metal commit];
}

} // namespace WebCore

#endif
