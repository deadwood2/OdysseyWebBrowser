/*
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
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
#import "AVVideoCaptureSource.h"

#if ENABLE(MEDIA_STREAM) && USE(AVFOUNDATION)

#import "AVCaptureDeviceManager.h"
#import "GraphicsContextCG.h"
#import "ImageBuffer.h"
#import "IntRect.h"
#import "Logging.h"
#import "MediaConstraints.h"
#import "MediaSampleAVFObjC.h"
#import "NotImplemented.h"
#import "PlatformLayer.h"
#import "RealtimeMediaSourceCenter.h"
#import "RealtimeMediaSourceSettings.h"
#import "WebActionDisablingCALayerDelegate.h"
#import <AVFoundation/AVCaptureDevice.h>
#import <AVFoundation/AVCaptureInput.h>
#import <AVFoundation/AVCaptureOutput.h>
#import <AVFoundation/AVCaptureSession.h>
#import <AVFoundation/AVCaptureVideoPreviewLayer.h>
#import <objc/runtime.h>

#if PLATFORM(IOS)
#include "WebCoreThread.h"
#include "WebCoreThreadRun.h"
#endif

#import "CoreMediaSoftLink.h"
#import "CoreVideoSoftLink.h"

typedef AVCaptureConnection AVCaptureConnectionType;
typedef AVCaptureDevice AVCaptureDeviceTypedef;
typedef AVCaptureDeviceFormat AVCaptureDeviceFormatType;
typedef AVCaptureDeviceInput AVCaptureDeviceInputType;
typedef AVCaptureOutput AVCaptureOutputType;
typedef AVCaptureVideoDataOutput AVCaptureVideoDataOutputType;
typedef AVFrameRateRange AVFrameRateRangeType;
typedef AVCaptureVideoPreviewLayer AVCaptureVideoPreviewLayerType;

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)

SOFT_LINK_CLASS(AVFoundation, AVCaptureConnection)
SOFT_LINK_CLASS(AVFoundation, AVCaptureDevice)
SOFT_LINK_CLASS(AVFoundation, AVCaptureDeviceFormat)
SOFT_LINK_CLASS(AVFoundation, AVCaptureDeviceInput)
SOFT_LINK_CLASS(AVFoundation, AVCaptureOutput)
SOFT_LINK_CLASS(AVFoundation, AVCaptureVideoDataOutput)
SOFT_LINK_CLASS(AVFoundation, AVCaptureVideoPreviewLayer)
SOFT_LINK_CLASS(AVFoundation, AVFrameRateRange)

#define AVCaptureConnection getAVCaptureConnectionClass()
#define AVCaptureDevice getAVCaptureDeviceClass()
#define AVCaptureDeviceFormat getAVCaptureDeviceFormatClass()
#define AVCaptureDeviceInput getAVCaptureDeviceInputClass()
#define AVCaptureOutput getAVCaptureOutputClass()
#define AVCaptureVideoDataOutput getAVCaptureVideoDataOutputClass()
#define AVCaptureVideoPreviewLayer getAVCaptureVideoPreviewLayerClass()
#define AVFrameRateRange getAVFrameRateRangeClass()

SOFT_LINK_POINTER(AVFoundation, AVCaptureSessionPreset1280x720, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVCaptureSessionPreset960x540, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVCaptureSessionPreset640x480, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVCaptureSessionPreset352x288, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVCaptureSessionPreset320x240, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVCaptureSessionPresetLow, NSString *)

#define AVCaptureSessionPreset1280x720 getAVCaptureSessionPreset1280x720()
#define AVCaptureSessionPreset960x540 getAVCaptureSessionPreset960x540()
#define AVCaptureSessionPreset640x480 getAVCaptureSessionPreset640x480()
#define AVCaptureSessionPreset352x288 getAVCaptureSessionPreset352x288()
#define AVCaptureSessionPreset320x240 getAVCaptureSessionPreset320x240()
#define AVCaptureSessionPresetLow getAVCaptureSessionPresetLow()

using namespace WebCore;

namespace WebCore {

const OSType videoCaptureFormat = kCVPixelFormatType_32BGRA;

RefPtr<AVMediaCaptureSource> AVVideoCaptureSource::create(AVCaptureDeviceTypedef* device, const AtomicString& id, const MediaConstraints* constraints, String& invalidConstraint)
{
    auto source = adoptRef(new AVVideoCaptureSource(device, id));
    if (constraints) {
        auto result = source->applyConstraints(*constraints);
        if (result) {
            invalidConstraint = result.value().first;
            source = nullptr;
        }
    }

    return source;
}

AVVideoCaptureSource::AVVideoCaptureSource(AVCaptureDeviceTypedef* device, const AtomicString& id)
    : AVMediaCaptureSource(device, id, RealtimeMediaSource::Video)
{
}

AVVideoCaptureSource::~AVVideoCaptureSource()
{
}

static void updateSizeMinMax(int& min, int& max, int value)
{
    min = std::min<int>(min, value);
    max = std::max<int>(max, value);
}

static void updateAspectRatioMinMax(double& min, double& max, double value)
{
    min = std::min<double>(min, value);
    max = std::max<double>(max, value);
}

void AVVideoCaptureSource::initializeCapabilities(RealtimeMediaSourceCapabilities& capabilities)
{
    AVCaptureDeviceTypedef *videoDevice = device();

    if ([videoDevice position] == AVCaptureDevicePositionFront)
        capabilities.addFacingMode(RealtimeMediaSourceSettings::User);
    if ([videoDevice position] == AVCaptureDevicePositionBack)
        capabilities.addFacingMode(RealtimeMediaSourceSettings::Environment);

    Float64 lowestFrameRateRange = std::numeric_limits<double>::infinity();
    Float64 highestFrameRateRange = 0;
    int minimumWidth = std::numeric_limits<int>::infinity();
    int maximumWidth = 0;
    int minimumHeight = std::numeric_limits<int>::infinity();
    int maximumHeight = 0;
    double minimumAspectRatio = std::numeric_limits<double>::infinity();
    double maximumAspectRatio = 0;

    for (AVCaptureDeviceFormatType *format in [videoDevice formats]) {

        for (AVFrameRateRangeType *range in [format videoSupportedFrameRateRanges]) {
            lowestFrameRateRange = std::min<Float64>(lowestFrameRateRange, range.minFrameRate);
            highestFrameRateRange = std::max<Float64>(highestFrameRateRange, range.maxFrameRate);
        }

        if ([videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset1280x720]) {
            updateSizeMinMax(minimumWidth, maximumWidth, 1280);
            updateSizeMinMax(minimumHeight, maximumHeight, 720);
            updateAspectRatioMinMax(minimumAspectRatio, maximumAspectRatio, 1280.0 / 720);
        }
        if ([videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset960x540]) {
            updateSizeMinMax(minimumWidth, maximumWidth, 960);
            updateSizeMinMax(minimumHeight, maximumHeight, 540);
            updateAspectRatioMinMax(minimumAspectRatio, maximumAspectRatio, 960 / 540);
        }
        if ([videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset640x480]) {
            updateSizeMinMax(minimumWidth, maximumWidth, 640);
            updateSizeMinMax(minimumHeight, maximumHeight, 480);
            updateAspectRatioMinMax(minimumAspectRatio, maximumAspectRatio, 640 / 480);
        }
        if ([videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset352x288]) {
            updateSizeMinMax(minimumWidth, maximumWidth, 352);
            updateSizeMinMax(minimumHeight, maximumHeight, 288);
            updateAspectRatioMinMax(minimumAspectRatio, maximumAspectRatio, 352 / 288);
        }
        if ([videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset320x240]) {
            updateSizeMinMax(minimumWidth, maximumWidth, 320);
            updateSizeMinMax(minimumHeight, maximumHeight, 240);
            updateAspectRatioMinMax(minimumAspectRatio, maximumAspectRatio, 320 / 240);
        }
    }

    capabilities.setFrameRate(CapabilityValueOrRange(lowestFrameRateRange, highestFrameRateRange));
    capabilities.setWidth(CapabilityValueOrRange(minimumWidth, maximumWidth));
    capabilities.setHeight(CapabilityValueOrRange(minimumHeight, maximumHeight));
    capabilities.setAspectRatio(CapabilityValueOrRange(minimumAspectRatio, maximumAspectRatio));
}

void AVVideoCaptureSource::initializeSupportedConstraints(RealtimeMediaSourceSupportedConstraints& supportedConstraints)
{
    supportedConstraints.setSupportsFacingMode([device() position] != AVCaptureDevicePositionUnspecified);
    supportedConstraints.setSupportsWidth(true);
    supportedConstraints.setSupportsHeight(true);
    supportedConstraints.setSupportsAspectRatio(true);
    supportedConstraints.setSupportsFrameRate(true);
}

void AVVideoCaptureSource::updateSettings(RealtimeMediaSourceSettings& settings)
{
    settings.setDeviceId(id());

    if ([device() position] == AVCaptureDevicePositionFront)
        settings.setFacingMode(RealtimeMediaSourceSettings::User);
    else if ([device() position] == AVCaptureDevicePositionBack)
        settings.setFacingMode(RealtimeMediaSourceSettings::Environment);
    else
        settings.setFacingMode(RealtimeMediaSourceSettings::Unknown);
    
    settings.setFrameRate(m_frameRate);
    settings.setWidth(m_width);
    settings.setHeight(m_height);
    settings.setAspectRatio(static_cast<float>(m_width) / m_height);
}

bool AVVideoCaptureSource::applySize(const IntSize& size)
{
    NSString *preset = bestSessionPresetForVideoDimensions(size.width(), size.height());
    if (!preset || ![session() canSetSessionPreset:preset]) {
        LOG(Media, "AVVideoCaptureSource::applySize(%p), unable find or set preset for width: %i, height: %i", this, size.width(), size.height());
        return false;
    }

    return setPreset(preset);
}

static IntSize sizeForPreset(NSString* preset)
{
    if (!preset)
        return { };

    if ([preset isEqualToString:AVCaptureSessionPreset1280x720])
        return { 1280, 720 };

    if ([preset isEqualToString:AVCaptureSessionPreset960x540])
        return { 960, 540 };

    if ([preset isEqualToString:AVCaptureSessionPreset640x480])
        return { 640, 480 };

    if ([preset isEqualToString:AVCaptureSessionPreset352x288])
        return { 352, 288 };

    if ([preset isEqualToString:AVCaptureSessionPreset320x240])
        return { 320, 240 };
    
    return { };
    
}

bool AVVideoCaptureSource::setPreset(NSString *preset)
{
    if (!session()) {
        m_pendingPreset = preset;
        return true;
    }

    auto size = sizeForPreset(preset);
    if (size.width() == m_width && size.height() == m_height)
        return true;

    @try {
        session().sessionPreset = preset;
#if PLATFORM(MAC)
        auto settingsDictionary = @{ (NSString*)kCVPixelBufferPixelFormatTypeKey: @(videoCaptureFormat), (NSString*)kCVPixelBufferWidthKey: @(size.width()), (NSString*)kCVPixelBufferHeightKey: @(size.height()), };
        [m_videoOutput setVideoSettings:settingsDictionary];
#endif
    } @catch(NSException *exception) {
        LOG(Media, "AVVideoCaptureSource::applySize(%p), exception thrown configuring device: <%s> %s", this, [[exception name] UTF8String], [[exception reason] UTF8String]);
        return false;
    }

    return true;
}

bool AVVideoCaptureSource::applyFrameRate(double rate)
{
    AVFrameRateRangeType *bestFrameRateRange = nil;
    for (AVFrameRateRangeType *frameRateRange in [[device() activeFormat] videoSupportedFrameRateRanges]) {
        if (rate >= [frameRateRange minFrameRate] && rate <= [frameRateRange maxFrameRate]) {
            if (!bestFrameRateRange || CMTIME_COMPARE_INLINE([frameRateRange minFrameDuration], >, [bestFrameRateRange minFrameDuration]))
                bestFrameRateRange = frameRateRange;
        }
    }

    if (!bestFrameRateRange) {
        LOG(Media, "AVVideoCaptureSource::applyFrameRate(%p), frame rate %f not supported by video device", this, rate);
        return false;
    }

    NSError *error = nil;
    @try {
        if ([device() lockForConfiguration:&error]) {
            [device() setActiveVideoMinFrameDuration:[bestFrameRateRange minFrameDuration]];
            [device() unlockForConfiguration];
        }
    } @catch(NSException *exception) {
        LOG(Media, "AVVideoCaptureSource::applyFrameRate(%p), exception thrown configuring device: <%s> %s", this, [[exception name] UTF8String], [[exception reason] UTF8String]);
        return false;
    }

    if (error) {
        LOG(Media, "AVVideoCaptureSource::applyFrameRate(%p), failed to lock video device for configuration: %s", this, [[error localizedDescription] UTF8String]);
        return false;
    }

    LOG(Media, "AVVideoCaptureSource::applyFrameRate(%p) - set frame rate range to %f", this, rate);
    return true;
}

void AVVideoCaptureSource::applySizeAndFrameRate(std::optional<int> width, std::optional<int> height, std::optional<double> frameRate)
{
    setPreset(bestSessionPresetForVideoDimensions(WTFMove(width), WTFMove(height)));

    if (frameRate)
        applyFrameRate(frameRate.value());
}

void AVVideoCaptureSource::setupCaptureSession()
{
    NSError *error = nil;
    RetainPtr<AVCaptureDeviceInputType> videoIn = adoptNS([allocAVCaptureDeviceInputInstance() initWithDevice:device() error:&error]);
    if (error) {
        LOG(Media, "AVVideoCaptureSource::setupCaptureSession(%p), failed to allocate AVCaptureDeviceInput: %s", this, [[error localizedDescription] UTF8String]);
        return;
    }

    if (![session() canAddInput:videoIn.get()]) {
        LOG(Media, "AVVideoCaptureSource::setupCaptureSession(%p), unable to add video input device", this);
        return;
    }
    [session() addInput:videoIn.get()];

    m_videoOutput = adoptNS([allocAVCaptureVideoDataOutputInstance() init]);
    auto settingsDictionary = adoptNS([[NSMutableDictionary alloc] initWithObjectsAndKeys: [NSNumber numberWithInt:videoCaptureFormat], kCVPixelBufferPixelFormatTypeKey, nil]);
    if (m_pendingPreset) {
#if PLATFORM(MAC)
        auto size = sizeForPreset(m_pendingPreset.get());
        [settingsDictionary.get() setObject:[NSNumber numberWithInt:size.width()] forKey:(NSString*)kCVPixelBufferWidthKey];
        [settingsDictionary.get() setObject:[NSNumber numberWithInt:size.height()] forKey:(NSString*)kCVPixelBufferHeightKey];
#endif
    }

    [m_videoOutput setVideoSettings:settingsDictionary.get()];
    [m_videoOutput setAlwaysDiscardsLateVideoFrames:YES];
    setVideoSampleBufferDelegate(m_videoOutput.get());

    if (![session() canAddOutput:m_videoOutput.get()]) {
        LOG(Media, "AVVideoCaptureSource::setupCaptureSession(%p), unable to add video sample buffer output delegate", this);
        return;
    }
    [session() addOutput:m_videoOutput.get()];

#if PLATFORM(IOS)
    setPreset(m_pendingPreset.get());
#endif

}

void AVVideoCaptureSource::shutdownCaptureSession()
{
    m_buffer = nullptr;
    m_lastImage = nullptr;
    m_videoFrameTimeStamps.clear();
    m_frameRate = 0;
    m_width = 0;
    m_height = 0;
}

bool AVVideoCaptureSource::updateFramerate(CMSampleBufferRef sampleBuffer)
{
    CMTime sampleTime = CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    if (!CMTIME_IS_NUMERIC(sampleTime))
        return false;

    Float64 frameTime = CMTimeGetSeconds(sampleTime);
    Float64 oneSecondAgo = frameTime - 1;
    
    m_videoFrameTimeStamps.append(frameTime);
    
    while (m_videoFrameTimeStamps[0] < oneSecondAgo)
        m_videoFrameTimeStamps.remove(0);

    Float64 frameRate = m_frameRate;
    m_frameRate = (m_frameRate + m_videoFrameTimeStamps.size()) / 2;

    return frameRate != m_frameRate;
}

void AVVideoCaptureSource::processNewFrame(RetainPtr<CMSampleBufferRef> sampleBuffer)
{
    // Ignore frames delivered when the session is not running, we want to hang onto the last image
    // delivered before it stopped.
    if (m_lastImage && (!isProducingData() || muted()))
        return;

    CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(sampleBuffer.get());
    if (!formatDescription)
        return;

    updateFramerate(sampleBuffer.get());
    m_buffer = sampleBuffer;
    m_lastImage = nullptr;

    bool settingsChanged = false;
    CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(formatDescription);
    if (dimensions.width != m_width || dimensions.height != m_height) {
        m_width = dimensions.width;
        m_height = dimensions.height;
        settingsChanged = true;
    }

    if (settingsChanged)
        settingsDidChange();

    videoSampleAvailable(MediaSampleAVFObjC::create(m_buffer.get()));
}

void AVVideoCaptureSource::captureOutputDidOutputSampleBufferFromConnection(AVCaptureOutputType*, CMSampleBufferRef sampleBuffer, AVCaptureConnectionType*)
{
    RetainPtr<CMSampleBufferRef> buffer = sampleBuffer;

    scheduleDeferredTask([this, buffer] {
        this->processNewFrame(buffer);
    });
}

RefPtr<Image> AVVideoCaptureSource::currentFrameImage()
{
    if (!currentFrameCGImage())
        return nullptr;

    FloatRect imageRect(0, 0, m_width, m_height);
    std::unique_ptr<ImageBuffer> imageBuffer = ImageBuffer::create(imageRect.size(), Unaccelerated);

    if (!imageBuffer)
        return nullptr;

    paintCurrentFrameInContext(imageBuffer->context(), imageRect);

    return ImageBuffer::sinkIntoImage(WTFMove(imageBuffer));
}

RetainPtr<CGImageRef> AVVideoCaptureSource::currentFrameCGImage()
{
    if (m_lastImage)
        return m_lastImage;

    if (!m_buffer)
        return nullptr;

    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(CMSampleBufferGetImageBuffer(m_buffer.get()));
    ASSERT(CVPixelBufferGetPixelFormatType(pixelBuffer) == videoCaptureFormat);

    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    void *baseAddress = CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    size_t width = CVPixelBufferGetWidth(pixelBuffer);
    size_t height = CVPixelBufferGetHeight(pixelBuffer);

    RetainPtr<CGDataProviderRef> provider = adoptCF(CGDataProviderCreateWithData(NULL, baseAddress, bytesPerRow * height, NULL));
    m_lastImage = adoptCF(CGImageCreate(width, height, 8, 32, bytesPerRow, sRGBColorSpaceRef(), kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst, provider.get(), NULL, true, kCGRenderingIntentDefault));

    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

    return m_lastImage;
}

void AVVideoCaptureSource::paintCurrentFrameInContext(GraphicsContext& context, const FloatRect& rect)
{
    if (context.paintingDisabled() || !currentFrameCGImage())
        return;

    GraphicsContextStateSaver stateSaver(context);
    context.translate(rect.x(), rect.y() + rect.height());
    context.scale(FloatSize(1, -1));
    context.setImageInterpolationQuality(InterpolationLow);
    IntRect paintRect(IntPoint(0, 0), IntSize(rect.width(), rect.height()));
    CGContextDrawImage(context.platformContext(), CGRectMake(0, 0, paintRect.width(), paintRect.height()), m_lastImage.get());
}

NSString* AVVideoCaptureSource::bestSessionPresetForVideoDimensions(std::optional<int> width, std::optional<int> height) const
{
    if (!width && !height)
        return nil;

    AVCaptureDeviceTypedef *videoDevice = device();
    if ((!width || width.value() == 1280) && (!height || height.value() == 720))
        return [videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset1280x720] ? AVCaptureSessionPreset1280x720 : nil;

    if ((!width || width.value() == 960) && (!height || height.value() == 540 ))
        return [videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset960x540] ? AVCaptureSessionPreset960x540 : nil;

    if ((!width || width.value() == 640) && (!height || height.value() == 480 ))
        return [videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset640x480] ? AVCaptureSessionPreset640x480 : nil;

    if ((!width || width.value() == 352) && (!height || height.value() == 288 ))
        return [videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset352x288] ? AVCaptureSessionPreset352x288 : nil;

    if ((!width || width.value() == 320) && (!height || height.value() == 240 ))
        return [videoDevice supportsAVCaptureSessionPreset:AVCaptureSessionPreset320x240] ? AVCaptureSessionPreset320x240 : nil;

    return nil;
}

bool AVVideoCaptureSource::supportsSizeAndFrameRate(std::optional<int> width, std::optional<int> height, std::optional<double> frameRate)
{
    if (!height && !width && !frameRate)
        return true;

    if ((height || width) && !bestSessionPresetForVideoDimensions(WTFMove(width), WTFMove(height)))
        return false;

    if (!frameRate)
        return true;

    double rate = frameRate.value();
    for (AVFrameRateRangeType *range in [[device() activeFormat] videoSupportedFrameRateRanges]) {
        if (rate >= range.minFrameRate && rate <= range.maxFrameRate)
            return true;
    }

    return false;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
