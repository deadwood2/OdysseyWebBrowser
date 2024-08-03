/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#import "config.h"
#import "MockRealtimeVideoSourceMac.h"

#if ENABLE(MEDIA_STREAM)
#import "GraphicsContextCG.h"
#import "ImageBuffer.h"
#import "MediaConstraints.h"
#import "MediaSampleAVFObjC.h"
#import "NotImplemented.h"
#import "PlatformLayer.h"
#import "RealtimeMediaSourceSettings.h"
#import <QuartzCore/CALayer.h>
#import <QuartzCore/CATransaction.h>
#import <objc/runtime.h>

#import "CoreMediaSoftLink.h"
#import "CoreVideoSoftLink.h"

namespace WebCore {

Ref<MockRealtimeVideoSource> MockRealtimeVideoSource::create()
{
    return adoptRef(*new MockRealtimeVideoSourceMac());
}

MockRealtimeVideoSourceMac::MockRealtimeVideoSourceMac()
    : MockRealtimeVideoSource()
{
}

RetainPtr<CMSampleBufferRef> MockRealtimeVideoSourceMac::CMSampleBufferFromPixelBuffer(CVPixelBufferRef pixelBuffer)
{
    if (!pixelBuffer)
        return nullptr;

    CMSampleTimingInfo timingInfo;

    timingInfo.presentationTimeStamp = CMTimeMake(elapsedTime() * 1000, 1000);
    timingInfo.decodeTimeStamp = kCMTimeInvalid;
    timingInfo.duration = kCMTimeInvalid;

    CMVideoFormatDescriptionRef formatDescription = nullptr;
    OSStatus status = CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, (CVImageBufferRef)pixelBuffer, &formatDescription);
    if (status != noErr) {
        LOG_ERROR("Failed to initialize CMVideoFormatDescription with error code: %d", status);
        return nullptr;
    }

    CMSampleBufferRef sampleBuffer;
    status = CMSampleBufferCreateReadyWithImageBuffer(kCFAllocatorDefault, (CVImageBufferRef)pixelBuffer, formatDescription, &timingInfo, &sampleBuffer);
    CFRelease(formatDescription);
    if (status != noErr) {
        LOG_ERROR("Failed to initialize CMSampleBuffer with error code: %d", status);
        return nullptr;
    }

    return adoptCF(sampleBuffer);
}

RetainPtr<CVPixelBufferRef> MockRealtimeVideoSourceMac::pixelBufferFromCGImage(CGImageRef image) const
{
    CGSize frameSize = CGSizeMake(CGImageGetWidth(image), CGImageGetHeight(image));
    CFDictionaryRef options = (__bridge CFDictionaryRef) @{
        (__bridge NSString *)kCVPixelBufferCGImageCompatibilityKey: @(NO),
        (__bridge NSString *)kCVPixelBufferCGBitmapContextCompatibilityKey: @(NO)
    };
    CVPixelBufferRef pixelBuffer;
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, frameSize.width, frameSize.height, kCVPixelFormatType_32ARGB, options, &pixelBuffer);
    if (status != kCVReturnSuccess)
        return nullptr;

    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    void* data = CVPixelBufferGetBaseAddress(pixelBuffer);
    auto rgbColorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    auto context = adoptCF(CGBitmapContextCreate(data, frameSize.width, frameSize.height, 8, CVPixelBufferGetBytesPerRow(pixelBuffer), rgbColorSpace.get(), (CGBitmapInfo) kCGImageAlphaNoneSkipFirst));
    CGContextDrawImage(context.get(), CGRectMake(0, 0, CGImageGetWidth(image), CGImageGetHeight(image)), image);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

    return adoptCF(pixelBuffer);
}

PlatformLayer* MockRealtimeVideoSourceMac::platformLayer() const
{
    if (m_previewLayer)
        return m_previewLayer.get();

    m_previewLayer = adoptNS([[CALayer alloc] init]);
    m_previewLayer.get().name = @"MockRealtimeVideoSourceMac preview layer";
    m_previewLayer.get().contentsGravity = kCAGravityResizeAspect;
    m_previewLayer.get().anchorPoint = CGPointZero;
    m_previewLayer.get().needsDisplayOnBoundsChange = YES;
#if !PLATFORM(IOS)
    m_previewLayer.get().autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
#endif

    updatePlatformLayer();

    return m_previewLayer.get();
}

void MockRealtimeVideoSourceMac::updatePlatformLayer() const
{
    if (!m_previewLayer)
        return;

    [CATransaction begin];
    [CATransaction setAnimationDuration:0];
    [CATransaction setDisableActions:YES];

    do {
        RefPtr<Image> image = imageBuffer()->copyImage();
        if (!image)
            break;

        m_previewImage = image->getCGImageRef();
        if (!m_previewImage)
            break;

        m_previewLayer.get().contents = (NSObject*)(m_previewImage.get());
    } while (0);

    [CATransaction commit];
}

void MockRealtimeVideoSourceMac::updateSampleBuffer()
{
    auto pixelBuffer = pixelBufferFromCGImage(imageBuffer()->copyImage()->getCGImageRef());
    auto sampleBuffer = CMSampleBufferFromPixelBuffer(pixelBuffer.get());
    
    mediaDataUpdated(MediaSampleAVFObjC::create(sampleBuffer.get()));
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
