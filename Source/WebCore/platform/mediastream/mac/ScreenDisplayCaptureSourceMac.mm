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
#include "ScreenDisplayCaptureSourceMac.h"

#if ENABLE(MEDIA_STREAM) && PLATFORM(MAC)

#include "GraphicsContextCG.h"
#include "ImageBuffer.h"
#include "Logging.h"
#include "MediaConstraints.h"
#include "MediaSampleAVFObjC.h"
#include "NotImplemented.h"
#include "PlatformLayer.h"
#include "RealtimeMediaSourceSettings.h"
#include <wtf/CurrentTime.h>

#include "CoreVideoSoftLink.h"

extern "C" {
size_t CGDisplayModeGetPixelsWide(CGDisplayModeRef);
size_t CGDisplayModeGetPixelsHigh(CGDisplayModeRef);
}

namespace WebCore {

static int32_t roundUpToMacroblockMultiple(int32_t size)
{
    return (size + 15) & ~15;
}

std::optional<CGDirectDisplayID> ScreenDisplayCaptureSourceMac::updateDisplayID(CGDirectDisplayID displayID)
{
    uint32_t displayCount = 0;
    auto err = CGGetActiveDisplayList(0, nullptr, &displayCount);
    if (err) {
        RELEASE_LOG(Media, "CGGetActiveDisplayList() returned error %d when trying to get display count", static_cast<int>(err));
        return std::nullopt;
    }

    if (!displayCount) {
        RELEASE_LOG(Media, "CGGetActiveDisplayList() returned a display count of 0");
        return std::nullopt;
    }

    CGDirectDisplayID activeDisplays[displayCount];
    err = CGGetActiveDisplayList(displayCount, &(activeDisplays[0]), &displayCount);
    if (err) {
        RELEASE_LOG(Media, "CGGetActiveDisplayList() returned error %d when trying to get the active display list", static_cast<int>(err));
        return std::nullopt;
    }

    auto displayMask = CGDisplayIDToOpenGLDisplayMask(displayID);
    for (auto display : activeDisplays) {
        if (displayMask == CGDisplayIDToOpenGLDisplayMask(display))
            return display;
    }

    return std::nullopt;
}

CaptureSourceOrError ScreenDisplayCaptureSourceMac::create(const String& deviceID, const MediaConstraints* constraints)
{
    bool ok;
    auto displayID = deviceID.toUIntStrict(&ok);
    if (!ok) {
        RELEASE_LOG(Media, "Display ID does not convert to 32-bit integer");
        return { };
    }

    auto actualDisplayID = updateDisplayID(displayID);
    if (!actualDisplayID)
        return { };

    auto source = adoptRef(*new ScreenDisplayCaptureSourceMac(actualDisplayID.value()));
    if (constraints && source->applyConstraints(*constraints))
        return { };

    return CaptureSourceOrError(WTFMove(source));
}

ScreenDisplayCaptureSourceMac::ScreenDisplayCaptureSourceMac(uint32_t displayID)
    : DisplayCaptureSourceCocoa("Screen")
    , m_displayID(displayID)
{
}

ScreenDisplayCaptureSourceMac::~ScreenDisplayCaptureSourceMac()
{
    if (m_observingDisplayChanges)
        CGDisplayRemoveReconfigurationCallback(displayReconfigurationCallBack, this);

    m_currentFrame = nullptr;
}

bool ScreenDisplayCaptureSourceMac::createDisplayStream()
{
    static const int screenQueueMaximumLength = 6;

    auto actualDisplayID = updateDisplayID(m_displayID);
    if (!actualDisplayID) {
        captureFailed();
        return false;
    }

    if (m_displayID != actualDisplayID.value()) {
        m_displayID = actualDisplayID.value();
        RELEASE_LOG(Media, "ScreenDisplayCaptureSourceMac::create(%p), display ID changed to %d", this, static_cast<int>(m_displayID));
    }

    if (!m_displayStream) {

        if (size().isEmpty()) {
            CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(m_displayID);
            auto screenWidth = CGDisplayModeGetPixelsWide(displayMode);
            auto screenHeight = CGDisplayModeGetPixelsHigh(displayMode);
            if (!screenWidth || !screenHeight) {
                RELEASE_LOG(Media, "ScreenDisplayCaptureSourceMac::createDisplayStream(%p), unable to get screen width/height", this);
                captureFailed();
                return false;
            }
            setWidth(screenWidth);
            setHeight(screenHeight);
            CGDisplayModeRelease(displayMode);
        }

        if (!m_captureQueue)
            m_captureQueue = adoptOSObject(dispatch_queue_create("ScreenDisplayCaptureSourceMac Capture Queue", DISPATCH_QUEUE_SERIAL));

        double frameTime = 1 / frameRate();
        auto frameTimeCF = adoptCF(CFNumberCreate(nullptr,  kCFNumberDoubleType,  &frameTime));
        int depth = screenQueueMaximumLength;
        auto depthCF = adoptCF(CFNumberCreate(nullptr,  kCFNumberIntType,  &depth));
        CFTypeRef keys[] = {
            kCGDisplayStreamMinimumFrameTime,
            kCGDisplayStreamQueueDepth,
            kCGDisplayStreamColorSpace,
            kCGDisplayStreamShowCursor,
        };
        CFTypeRef values[] = {
            frameTimeCF.get(),
            depthCF.get(),
            sRGBColorSpaceRef(),
            kCFBooleanTrue,
        };
        auto streamOptions = adoptCF(CFDictionaryCreate(kCFAllocatorDefault, keys, values, WTF_ARRAY_LENGTH(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

        auto weakThis = m_weakFactory.createWeakPtr(*this);
        m_frameAvailableBlock = Block_copy(^(CGDisplayStreamFrameStatus status, uint64_t displayTime, IOSurfaceRef frameSurface, CGDisplayStreamUpdateRef updateRef) {
            if (!weakThis)
                return;

            weakThis->frameAvailable(status, displayTime, frameSurface, updateRef);
        });

        m_displayStream = adoptCF(CGDisplayStreamCreateWithDispatchQueue(m_displayID, size().width(), size().height(), kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, streamOptions.get(), m_captureQueue.get(), m_frameAvailableBlock));
        if (!m_displayStream) {
            RELEASE_LOG(Media, "ScreenDisplayCaptureSourceMac::createDisplayStream(%p), CGDisplayStreamCreate failed", this);
            captureFailed();
            return false;
        }
    }

    if (!m_observingDisplayChanges) {
        CGDisplayRegisterReconfigurationCallback(displayReconfigurationCallBack, this);
        m_observingDisplayChanges = true;
    }

    return true;
}

void ScreenDisplayCaptureSourceMac::startProducingData()
{
    DisplayCaptureSourceCocoa::startProducingData();

    if (m_isRunning)
        return;

    startDisplayStream();
}

void ScreenDisplayCaptureSourceMac::stopProducingData()
{
    DisplayCaptureSourceCocoa::stopProducingData();

    if (!m_isRunning)
        return;

    if (m_displayStream)
        CGDisplayStreamStop(m_displayStream.get());

    m_isRunning = false;
}

RetainPtr<CMSampleBufferRef> ScreenDisplayCaptureSourceMac::sampleBufferFromPixelBuffer(CVPixelBufferRef pixelBuffer)
{
    if (!pixelBuffer)
        return nullptr;

    CMTime sampleTime = CMTimeMake((elapsedTime() + .1) * 100, 100);
    CMSampleTimingInfo timingInfo = { kCMTimeInvalid, sampleTime, sampleTime };

    CMVideoFormatDescriptionRef formatDescription = nullptr;
    auto status = CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault, (CVImageBufferRef)pixelBuffer, &formatDescription);
    if (status) {
        RELEASE_LOG(Media, "Failed to initialize CMVideoFormatDescription with error code: %d", static_cast<int>(status));
        return nullptr;
    }

    CMSampleBufferRef sampleBuffer;
    status = CMSampleBufferCreateReadyWithImageBuffer(kCFAllocatorDefault, (CVImageBufferRef)pixelBuffer, formatDescription, &timingInfo, &sampleBuffer);
    CFRelease(formatDescription);
    if (status) {
        RELEASE_LOG(Media, "Failed to initialize CMSampleBuffer with error code: %d", static_cast<int>(status));
        return nullptr;
    }

    return adoptCF(sampleBuffer);
}

RetainPtr<CVPixelBufferRef> ScreenDisplayCaptureSourceMac::pixelBufferFromIOSurface(IOSurfaceRef surface)
{
    if (!m_bufferAttributes) {
        m_bufferAttributes = adoptCF(CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

        auto format = IOSurfaceGetPixelFormat(surface);
        if (format == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange || format == kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) {

            // If the width x height isn't a multiple of 16 x 16 and the surface has extra memory in the planes, set pixel buffer attributes to reflect it.
            auto width = IOSurfaceGetWidth(surface);
            auto height = IOSurfaceGetHeight(surface);
            int32_t extendedRight = roundUpToMacroblockMultiple(width) - width;
            int32_t extendedBottom = roundUpToMacroblockMultiple(height) - height;

            if ((IOSurfaceGetBytesPerRowOfPlane(surface, 0) >= width + extendedRight)
                && (IOSurfaceGetBytesPerRowOfPlane(surface, 1) >= width + extendedRight)
                && (IOSurfaceGetAllocSize(surface) >= (height + extendedBottom) * IOSurfaceGetBytesPerRowOfPlane(surface, 0) * 3 / 2)) {
                    auto cfInt = adoptCF(CFNumberCreate(nullptr,  kCFNumberIntType,  &extendedRight));
                    CFDictionarySetValue(m_bufferAttributes.get(), kCVPixelBufferExtendedPixelsRightKey, cfInt.get());
                    cfInt = adoptCF(CFNumberCreate(nullptr,  kCFNumberIntType,  &extendedBottom));
                    CFDictionarySetValue(m_bufferAttributes.get(), kCVPixelBufferExtendedPixelsBottomKey, cfInt.get());
            }
        }

        CFDictionarySetValue(m_bufferAttributes.get(), kCVPixelBufferOpenGLCompatibilityKey, kCFBooleanTrue);
    }

    CVPixelBufferRef pixelBuffer;
    auto status = CVPixelBufferCreateWithIOSurface(kCFAllocatorDefault, surface, m_bufferAttributes.get(), &pixelBuffer);
    if (status) {
        RELEASE_LOG(Media, "Failed to initialize CMVideoFormatDescription with error code: %d", static_cast<int>(status));
        return nullptr;
    }

    return adoptCF(pixelBuffer);
}

void ScreenDisplayCaptureSourceMac::generateFrame()
{
    if (!m_currentFrame.ioSurface())
        return;

    DisplaySurface currentFrame;
    {
        LockHolder lock(m_currentFrameMutex);
        currentFrame = m_currentFrame.ioSurface();
    }

    auto pixelBuffer = pixelBufferFromIOSurface(currentFrame.ioSurface());
    if (!pixelBuffer)
        return;

    auto sampleBuffer = sampleBufferFromPixelBuffer(pixelBuffer.get());
    if (!sampleBuffer)
        return;

    videoSampleAvailable(MediaSampleAVFObjC::create(sampleBuffer.get()));
}

void ScreenDisplayCaptureSourceMac::startDisplayStream()
{
    auto actualDisplayID = updateDisplayID(m_displayID);
    if (!actualDisplayID)
        return;

    if (m_displayID != actualDisplayID.value()) {
        m_displayID = actualDisplayID.value();
        RELEASE_LOG(Media, "ScreenDisplayCaptureSourceMac::create(%p), display ID changed to %d", this, static_cast<int>(m_displayID));
    }

    if (!m_displayStream && !createDisplayStream())
        return;

    auto err = CGDisplayStreamStart(m_displayStream.get());
    if (err) {
        RELEASE_LOG(Media, "ScreenDisplayCaptureSourceMac::startProducingData(%p), CGDisplayStreamStart failed with error %d", this, static_cast<int>(err));
        captureFailed();
        return;
    }

    m_isRunning = true;
}

bool ScreenDisplayCaptureSourceMac::applySize(const IntSize& newSize)
{
    if (size() == newSize)
        return true;

    m_bufferAttributes = nullptr;
    m_displayStream = nullptr;
    return true;
}

bool ScreenDisplayCaptureSourceMac::applyFrameRate(double rate)
{
    if (frameRate() != rate) {
        m_bufferAttributes = nullptr;
        m_displayStream = nullptr;
    }

    return DisplayCaptureSourceCocoa::applyFrameRate(rate);
}

void ScreenDisplayCaptureSourceMac::commitConfiguration()
{
    if (m_isRunning && !m_displayStream)
        startDisplayStream();
}

void ScreenDisplayCaptureSourceMac::displayWasReconfigured(CGDirectDisplayID, CGDisplayChangeSummaryFlags)
{
    // FIXME: implement!
}

void ScreenDisplayCaptureSourceMac::displayReconfigurationCallBack(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *userInfo)
{
    if (userInfo)
        reinterpret_cast<ScreenDisplayCaptureSourceMac *>(userInfo)->displayWasReconfigured(display, flags);
}

void ScreenDisplayCaptureSourceMac::frameAvailable(CGDisplayStreamFrameStatus status, uint64_t displayTime, IOSurfaceRef frameSurface, CGDisplayStreamUpdateRef updateRef)
{
    switch (status) {
    case kCGDisplayStreamFrameStatusFrameComplete:
        break;

    case kCGDisplayStreamFrameStatusFrameIdle:
        break;

    case kCGDisplayStreamFrameStatusFrameBlank:
        RELEASE_LOG(Media, "ScreenDisplayCaptureSourceMac::frameAvailable(%p), kCGDisplayStreamFrameStatusFrameBlank", this);
        break;

    case kCGDisplayStreamFrameStatusStopped:
        RELEASE_LOG(Media, "ScreenDisplayCaptureSourceMac::frameAvailable(%p), kCGDisplayStreamFrameStatusStopped", this);
        break;
    }

    if (!frameSurface || !displayTime)
        return;

    size_t count;
    auto* rects = CGDisplayStreamUpdateGetRects(updateRef, kCGDisplayStreamUpdateDirtyRects, &count);
    if (!rects || !count)
        return;

    LockHolder lock(m_currentFrameMutex);
    m_lastFrameTime = monotonicallyIncreasingTime();
    m_currentFrame = frameSurface;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM) && PLATFORM(MAC)
