/*
 * Copyright (C) 2009-2017 Apple Inc. All rights reserved.
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

#pragma once

#import "IOSurface.h"
#import "IntSize.h"
#import <QuartzCore/QuartzCore.h>

namespace WebCore {
class GraphicsLayer;
class GraphicsContext3D;
}

#if PLATFORM(MAC)
@interface WebGLLayer : CALayer
#else
@interface WebGLLayer : CAEAGLLayer
#endif
{
    WebCore::GraphicsContext3D* _context;
    float _devicePixelRatio;
#if PLATFORM(MAC)
    std::unique_ptr<WebCore::IOSurface> _contentsBuffer;
    std::unique_ptr<WebCore::IOSurface> _drawingBuffer;
    std::unique_ptr<WebCore::IOSurface> _spareBuffer;
    WebCore::IntSize _bufferSize;
    BOOL _usingAlpha;
#endif
}

@property (nonatomic) WebCore::GraphicsContext3D* context;

- (id)initWithGraphicsContext3D:(WebCore::GraphicsContext3D*)context;

- (CGImageRef)copyImageSnapshotWithColorSpace:(CGColorSpaceRef)colorSpace;

#if PLATFORM(MAC)
- (void)allocateIOSurfaceBackingStoreWithSize:(WebCore::IntSize)size usingAlpha:(BOOL)usingAlpha;
- (void)bindFramebufferToNextAvailableSurface;
#endif

@end

