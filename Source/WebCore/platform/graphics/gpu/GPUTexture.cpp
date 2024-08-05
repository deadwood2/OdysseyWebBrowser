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

#include "config.h"
#include "GPUTexture.h"

#if ENABLE(WEBGPU)

#include "GPUDevice.h"
#include "GPUDrawable.h"
#include "Logging.h"

namespace WebCore {

RefPtr<GPUTexture> GPUTexture::create(GPUDevice* device, GPUTextureDescriptor* descriptor)
{
    RefPtr<GPUTexture> texture = adoptRef(new GPUTexture(device, descriptor));
    return texture;
}

RefPtr<GPUTexture> GPUTexture::createFromDrawable(GPUDrawable* other)
{
    RefPtr<GPUTexture> texture = adoptRef(new GPUTexture(other));
    return texture;
}

GPUTexture::~GPUTexture()
{
    LOG(WebGPU, "GPUTexture::~GPUTexture()");
}

#if !PLATFORM(COCOA)
unsigned long GPUTexture::width() const
{
    return 0;
}

unsigned long GPUTexture::height() const
{
    return 0;
}
#endif

} // namespace WebCore

#endif
