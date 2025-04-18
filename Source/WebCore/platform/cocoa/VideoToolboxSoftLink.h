/*
 * Copyright (C) 2017-2019 Apple Inc. All rights reserved.
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

#pragma once

#if USE(VIDEOTOOLBOX)

#include <VideoToolbox/VideoToolbox.h>
#include <wtf/SoftLinking.h>

typedef struct OpaqueVTImageRotationSession* VTImageRotationSessionRef;
typedef struct OpaqueVTPixelBufferConformer* VTPixelBufferConformerRef;
typedef struct OpaqueVTPixelTransferSession* VTPixelTransferSessionRef;

SOFT_LINK_FRAMEWORK_FOR_HEADER(WebCore, VideoToolbox)

SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTSessionCopyProperty, OSStatus, (VTSessionRef session, CFStringRef propertyKey, CFAllocatorRef allocator, void* propertyValueOut), (session, propertyKey, allocator, propertyValueOut))
#define VTSessionCopyProperty softLink_VideoToolbox_VTSessionCopyProperty
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTSessionSetProperties, OSStatus, (VTSessionRef session, CFDictionaryRef propertyDictionary), (session, propertyDictionary))
#define VTSessionSetProperties softLink_VideoToolbox_VTSessionSetProperties
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTDecompressionSessionCreate, OSStatus, (CFAllocatorRef allocator, CMVideoFormatDescriptionRef videoFormatDescription, CFDictionaryRef videoDecoderSpecification, CFDictionaryRef destinationImageBufferAttributes, const VTDecompressionOutputCallbackRecord* outputCallback, VTDecompressionSessionRef* decompressionSessionOut), (allocator, videoFormatDescription, videoDecoderSpecification, destinationImageBufferAttributes, outputCallback, decompressionSessionOut))
#define VTDecompressionSessionCreate softLink_VideoToolbox_VTDecompressionSessionCreate
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTDecompressionSessionCanAcceptFormatDescription, Boolean, (VTDecompressionSessionRef session, CMFormatDescriptionRef newFormatDesc), (session, newFormatDesc))
#define VTDecompressionSessionCanAcceptFormatDescription softLink_VideoToolbox_VTDecompressionSessionCanAcceptFormatDescription
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTDecompressionSessionWaitForAsynchronousFrames, OSStatus, (VTDecompressionSessionRef session), (session))
#define VTDecompressionSessionWaitForAsynchronousFrames softLink_VideoToolbox_VTDecompressionSessionWaitForAsynchronousFrames
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTDecompressionSessionDecodeFrameWithOutputHandler, OSStatus, (VTDecompressionSessionRef session, CMSampleBufferRef sampleBuffer, VTDecodeFrameFlags decodeFlags, VTDecodeInfoFlags *infoFlagsOut, VTDecompressionOutputHandler outputHandler), (session, sampleBuffer, decodeFlags, infoFlagsOut, outputHandler))
#define VTDecompressionSessionDecodeFrameWithOutputHandler softLink_VideoToolbox_VTDecompressionSessionDecodeFrameWithOutputHandler
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTImageRotationSessionCreate, OSStatus, (CFAllocatorRef allocator, uint32_t rotationDegrees, VTImageRotationSessionRef* imageRotationSessionOut), (allocator, rotationDegrees, imageRotationSessionOut))
#define VTImageRotationSessionCreate softLink_VideoToolbox_VTImageRotationSessionCreate
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTImageRotationSessionSetProperty, OSStatus, (VTImageRotationSessionRef session, CFStringRef propertyKey, CFTypeRef propertyValue), (session, propertyKey, propertyValue))
#define VTImageRotationSessionSetProperty softLink_VideoToolbox_VTImageRotationSessionSetProperty
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTImageRotationSessionTransferImage, OSStatus, (VTImageRotationSessionRef session, CVPixelBufferRef sourceBuffer, CVPixelBufferRef destinationBuffer), (session, sourceBuffer, destinationBuffer))
#define VTImageRotationSessionTransferImage softLink_VideoToolbox_VTImageRotationSessionTransferImage
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, VTIsHardwareDecodeSupported, Boolean, (CMVideoCodecType codecType), (codecType))
#define VTIsHardwareDecodeSupported softLink_VideoToolbox_VTIsHardwareDecodeSupported
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, VTGetGVADecoderAvailability, OSStatus, (uint32_t* totalInstanceCountOut, uint32_t* freeInstanceCountOut), (totalInstanceCountOut, freeInstanceCountOut))
#define VTGetGVADecoderAvailability softLink_VideoToolbox_VTGetGVADecoderAvailability
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, VTCreateCGImageFromCVPixelBuffer, OSStatus, (CVPixelBufferRef pixelBuffer, CFDictionaryRef options, CGImageRef* imageOut), (pixelBuffer, options, imageOut))
#define VTCreateCGImageFromCVPixelBuffer softLink_VideoToolbox_VTCreateCGImageFromCVPixelBuffer
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, VTCopyHEVCDecoderCapabilitiesDictionary, CFDictionaryRef, (), ())
#define VTCopyHEVCDecoderCapabilitiesDictionary softLink_VideoToolbox_VTCopyHEVCDecoderCapabilitiesDictionary
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, VTGetHEVCCapabilitesForFormatDescription, OSStatus, (CMVideoFormatDescriptionRef formatDescription, CFDictionaryRef decoderCapabilitiesDict, Boolean* isDecodable, Boolean* mayBePlayable), (formatDescription, decoderCapabilitiesDict, isDecodable, mayBePlayable))
#define VTGetHEVCCapabilitesForFormatDescription softLink_VideoToolbox_VTGetHEVCCapabilitesForFormatDescription
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder, CFStringRef)
#define kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder get_VideoToolbox_kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTDecompressionPropertyKey_PixelBufferPool, CFStringRef)
#define kVTDecompressionPropertyKey_PixelBufferPool get_VideoToolbox_kVTDecompressionPropertyKey_PixelBufferPool()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTDecompressionPropertyKey_SuggestedQualityOfServiceTiers, CFStringRef)
#define kVTDecompressionPropertyKey_SuggestedQualityOfServiceTiers get_VideoToolbox_kVTDecompressionPropertyKey_SuggestedQualityOfServiceTiers()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTImageRotationPropertyKey_EnableHighSpeedTransfer, CFStringRef)
#define kVTImageRotationPropertyKey_EnableHighSpeedTransfer get_VideoToolbox_kVTImageRotationPropertyKey_EnableHighSpeedTransfer()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTImageRotationPropertyKey_FlipHorizontalOrientation, CFStringRef)
#define kVTImageRotationPropertyKey_FlipHorizontalOrientation get_VideoToolbox_kVTImageRotationPropertyKey_FlipHorizontalOrientation()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTImageRotationPropertyKey_FlipVerticalOrientation, CFStringRef)
#define kVTImageRotationPropertyKey_FlipVerticalOrientation get_VideoToolbox_kVTImageRotationPropertyKey_FlipVerticalOrientation()

SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTPixelTransferSessionCreate, OSStatus, (CFAllocatorRef allocator, VTPixelTransferSessionRef* pixelTransferSessionOut), (allocator, pixelTransferSessionOut))
#define VTPixelTransferSessionCreate softLink_VideoToolbox_VTPixelTransferSessionCreate
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTPixelTransferSessionTransferImage, OSStatus, (VTPixelTransferSessionRef session, CVPixelBufferRef sourceBuffer, CVPixelBufferRef destinationBuffer), (session, sourceBuffer, destinationBuffer))
#define VTPixelTransferSessionTransferImage softLink_VideoToolbox_VTPixelTransferSessionTransferImage
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTSessionSetProperty, OSStatus, (VTSessionRef session, CFStringRef propertyKey, CFTypeRef propertyValue), (session, propertyKey, propertyValue))
#define VTSessionSetProperty softLink_VideoToolbox_VTSessionSetProperty

SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTPixelTransferPropertyKey_ScalingMode, CFStringRef)
#define kVTPixelTransferPropertyKey_ScalingMode get_VideoToolbox_kVTPixelTransferPropertyKey_ScalingMode()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTScalingMode_Letterbox, CFStringRef)
#define kVTScalingMode_Letterbox get_VideoToolbox_kVTScalingMode_Letterbox()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTPixelTransferPropertyKey_EnableHardwareAcceleratedTransfer, CFStringRef)
#define kVTPixelTransferPropertyKey_EnableHardwareAcceleratedTransfer get_VideoToolbox_kVTPixelTransferPropertyKey_EnableHardwareAcceleratedTransfer()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTPixelTransferPropertyKey_EnableHighSpeedTransfer, CFStringRef)
#define kVTPixelTransferPropertyKey_EnableHighSpeedTransfer get_VideoToolbox_kVTPixelTransferPropertyKey_EnableHighSpeedTransfer()
SOFT_LINK_CONSTANT_FOR_HEADER(WebCore, VideoToolbox, kVTPixelTransferPropertyKey_RealTime, CFStringRef)
#define kVTPixelTransferPropertyKey_RealTime get_VideoToolbox_kVTPixelTransferPropertyKey_RealTime()

SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, kVTHEVCDecoderCapability_SupportedProfiles, CFStringRef)
#define kVTHEVCDecoderCapability_SupportedProfiles get_VideoToolbox_kVTHEVCDecoderCapability_SupportedProfiles()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, kVTHEVCDecoderCapability_PerProfileSupport, CFStringRef)
#define kVTHEVCDecoderCapability_PerProfileSupport get_VideoToolbox_kVTHEVCDecoderCapability_PerProfileSupport()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, kVTHEVCDecoderProfileCapability_IsHardwareAccelerated, CFStringRef)
#define kVTHEVCDecoderProfileCapability_IsHardwareAccelerated get_VideoToolbox_kVTHEVCDecoderProfileCapability_IsHardwareAccelerated()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, kVTHEVCDecoderProfileCapability_MaxDecodeLevel, CFStringRef)
#define kVTHEVCDecoderProfileCapability_MaxDecodeLevel get_VideoToolbox_kVTHEVCDecoderProfileCapability_MaxDecodeLevel()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(WebCore, VideoToolbox, kVTHEVCDecoderProfileCapability_MaxPlaybackLevel, CFStringRef)
#define kVTHEVCDecoderProfileCapability_MaxPlaybackLevel get_VideoToolbox_kVTHEVCDecoderProfileCapability_MaxPlaybackLevel()

SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTPixelBufferConformerCreateWithAttributes, OSStatus, (CFAllocatorRef allocator, CFDictionaryRef attributes, VTPixelBufferConformerRef* conformerOut), (allocator, attributes, conformerOut));
#define VTPixelBufferConformerCreateWithAttributes softLink_VideoToolbox_VTPixelBufferConformerCreateWithAttributes
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTPixelBufferConformerIsConformantPixelBuffer, Boolean, (VTPixelBufferConformerRef conformer, CVPixelBufferRef pixBuf), (conformer, pixBuf))
#define VTPixelBufferConformerIsConformantPixelBuffer softLink_VideoToolbox_VTPixelBufferConformerIsConformantPixelBuffer
SOFT_LINK_FUNCTION_FOR_HEADER(WebCore, VideoToolbox, VTPixelBufferConformerCopyConformedPixelBuffer, OSStatus, (VTPixelBufferConformerRef conformer, CVPixelBufferRef sourceBuffer, Boolean ensureModifiable, CVPixelBufferRef* conformedBufferOut), (conformer, sourceBuffer, ensureModifiable, conformedBufferOut))
#define VTPixelBufferConformerCopyConformedPixelBuffer softLink_VideoToolbox_VTPixelBufferConformerCopyConformedPixelBuffer

#endif // USE(VIDEOTOOLBOX)
