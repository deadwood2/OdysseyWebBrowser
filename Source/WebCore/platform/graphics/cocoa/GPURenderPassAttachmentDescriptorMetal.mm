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
#import "GPURenderPassAttachmentDescriptor.h"

#if ENABLE(WEBGPU)

#import "GPUTexture.h"
#import "Logging.h"

#import <Metal/Metal.h>

namespace WebCore {

GPURenderPassAttachmentDescriptor::GPURenderPassAttachmentDescriptor(MTLRenderPassAttachmentDescriptor *attachmentDescriptor)
{
    LOG(WebGPU, "GPURenderPassAttachmentDescriptor::GPURenderPassAttachmentDescriptor()");

    m_renderPassAttachmentDescriptor = attachmentDescriptor;
}

unsigned long GPURenderPassAttachmentDescriptor::loadAction() const
{
    if (!m_renderPassAttachmentDescriptor)
        return 0; // FIXME: WebGPU - There is probably a real value for this.

    return [m_renderPassAttachmentDescriptor loadAction];
}

void GPURenderPassAttachmentDescriptor::setLoadAction(unsigned long newLoadAction)
{
    if (!m_renderPassAttachmentDescriptor)
        return;

    [m_renderPassAttachmentDescriptor setLoadAction:static_cast<MTLLoadAction>(newLoadAction)];
}

unsigned long GPURenderPassAttachmentDescriptor::storeAction() const
{
    if (!m_renderPassAttachmentDescriptor)
        return 0; // FIXME: WebGPU - There is probably a real value for this.

    return [m_renderPassAttachmentDescriptor storeAction];
}

void GPURenderPassAttachmentDescriptor::setStoreAction(unsigned long newStoreAction)
{
    if (!m_renderPassAttachmentDescriptor)
        return;

    [m_renderPassAttachmentDescriptor setStoreAction:static_cast<MTLStoreAction>(newStoreAction)];
}

void GPURenderPassAttachmentDescriptor::setTexture(RefPtr<GPUTexture> texture)
{
    if (!m_renderPassAttachmentDescriptor)
        return;
    
    [m_renderPassAttachmentDescriptor setTexture:static_cast<id<MTLTexture>>(texture->platformTexture())];
}

MTLRenderPassAttachmentDescriptor *GPURenderPassAttachmentDescriptor::platformRenderPassAttachmentDescriptor()
{
    return m_renderPassAttachmentDescriptor.get();
}

} // namespace WebCore

#endif
