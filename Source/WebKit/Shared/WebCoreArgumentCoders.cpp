/*
 * Copyright (C) 2011-2017 Apple Inc. All rights reserved.
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
#include "WebCoreArgumentCoders.h"

#include "DataReference.h"
#include "ShareableBitmap.h"
#include <WebCore/AttachmentTypes.h>
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/BlobPart.h>
#include <WebCore/CacheQueryOptions.h>
#include <WebCore/CertificateInfo.h>
#include <WebCore/CompositionUnderline.h>
#include <WebCore/Credential.h>
#include <WebCore/Cursor.h>
#include <WebCore/DatabaseDetails.h>
#include <WebCore/DictationAlternative.h>
#include <WebCore/DictionaryPopupInfo.h>
#include <WebCore/DragData.h>
#include <WebCore/EventTrackingRegions.h>
#include <WebCore/FetchOptions.h>
#include <WebCore/FileChooser.h>
#include <WebCore/FilterOperation.h>
#include <WebCore/FilterOperations.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/GraphicsLayer.h>
#include <WebCore/IDBGetResult.h>
#include <WebCore/Image.h>
#include <WebCore/JSDOMExceptionHandling.h>
#include <WebCore/Length.h>
#include <WebCore/LengthBox.h>
#include <WebCore/MediaSelectionOption.h>
#include <WebCore/Pasteboard.h>
#include <WebCore/Path.h>
#include <WebCore/PluginData.h>
#include <WebCore/PromisedBlobInfo.h>
#include <WebCore/ProtectionSpace.h>
#include <WebCore/RectEdges.h>
#include <WebCore/Region.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceLoadStatistics.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/ScrollingConstraints.h>
#include <WebCore/ScrollingCoordinator.h>
#include <WebCore/SearchPopupMenu.h>
#include <WebCore/ServiceWorkerClientData.h>
#include <WebCore/ServiceWorkerClientIdentifier.h>
#include <WebCore/ServiceWorkerData.h>
#include <WebCore/TextCheckerClient.h>
#include <WebCore/TextIndicator.h>
#include <WebCore/TimingFunction.h>
#include <WebCore/TransformationMatrix.h>
#include <WebCore/URL.h>
#include <WebCore/UserStyleSheet.h>
#include <WebCore/ViewportArguments.h>
#include <WebCore/WindowFeatures.h>
#include <pal/SessionID.h>
#include <wtf/MonotonicTime.h>
#include <wtf/Seconds.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

#if PLATFORM(COCOA)
#include "ArgumentCodersCF.h"
#include "ArgumentCodersMac.h"
#endif

#if PLATFORM(IOS)
#include <WebCore/FloatQuad.h>
#include <WebCore/InspectorOverlay.h>
#include <WebCore/SelectionRect.h>
#include <WebCore/SharedBuffer.h>
#endif // PLATFORM(IOS)

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
#include <WebCore/MediaPlaybackTargetContext.h>
#endif

#if ENABLE(MEDIA_SESSION)
#include <WebCore/MediaSessionMetadata.h>
#endif

#if ENABLE(MEDIA_STREAM)
#include <WebCore/CaptureDevice.h>
#include <WebCore/MediaConstraints.h>
#endif

using namespace WebCore;
using namespace WebKit;

namespace IPC {

static void encodeSharedBuffer(Encoder& encoder, const SharedBuffer* buffer)
{
    SharedMemory::Handle handle;
    uint64_t bufferSize = buffer ? buffer->size() : 0;
    encoder << bufferSize;
    if (!bufferSize)
        return;

    auto sharedMemoryBuffer = SharedMemory::allocate(buffer->size());
    memcpy(sharedMemoryBuffer->data(), buffer->data(), buffer->size());
    sharedMemoryBuffer->createHandle(handle, SharedMemory::Protection::ReadOnly);
    encoder << handle;
}

static bool decodeSharedBuffer(Decoder& decoder, RefPtr<SharedBuffer>& buffer)
{
    uint64_t bufferSize = 0;
    if (!decoder.decode(bufferSize))
        return false;

    if (!bufferSize)
        return true;

    SharedMemory::Handle handle;
    if (!decoder.decode(handle))
        return false;

    auto sharedMemoryBuffer = SharedMemory::map(handle, SharedMemory::Protection::ReadOnly);
    buffer = SharedBuffer::create(static_cast<unsigned char*>(sharedMemoryBuffer->data()), bufferSize);

    return true;
}

static void encodeTypesAndData(Encoder& encoder, const Vector<String>& types, const Vector<RefPtr<SharedBuffer>>& data)
{
    ASSERT(types.size() == data.size());
    encoder << types;
    encoder << static_cast<uint64_t>(data.size());
    for (auto& buffer : data)
        encodeSharedBuffer(encoder, buffer.get());
}

static bool decodeTypesAndData(Decoder& decoder, Vector<String>& types, Vector<RefPtr<SharedBuffer>>& data)
{
    if (!decoder.decode(types))
        return false;

    uint64_t dataSize;
    if (!decoder.decode(dataSize))
        return false;

    ASSERT(dataSize == types.size());

    data.resize(dataSize);
    for (auto& buffer : data)
        decodeSharedBuffer(decoder, buffer);

    return true;
}

void ArgumentCoder<MonotonicTime>::encode(Encoder& encoder, const MonotonicTime& time)
{
    encoder << time.secondsSinceEpoch().value();
}

bool ArgumentCoder<MonotonicTime>::decode(Decoder& decoder, MonotonicTime& time)
{
    double value;
    if (!decoder.decode(value))
        return false;

    time = MonotonicTime::fromRawSeconds(value);
    return true;
}

void ArgumentCoder<Seconds>::encode(Encoder& encoder, const Seconds& seconds)
{
    encoder << seconds.value();
}

bool ArgumentCoder<Seconds>::decode(Decoder& decoder, Seconds& seconds)
{
    double value;
    if (!decoder.decode(value))
        return false;

    seconds = Seconds(value);
    return true;
}

void ArgumentCoder<AffineTransform>::encode(Encoder& encoder, const AffineTransform& affineTransform)
{
    SimpleArgumentCoder<AffineTransform>::encode(encoder, affineTransform);
}

bool ArgumentCoder<AffineTransform>::decode(Decoder& decoder, AffineTransform& affineTransform)
{
    return SimpleArgumentCoder<AffineTransform>::decode(decoder, affineTransform);
}

void ArgumentCoder<CacheQueryOptions>::encode(Encoder& encoder, const CacheQueryOptions& options)
{
    encoder << options.ignoreSearch;
    encoder << options.ignoreMethod;
    encoder << options.ignoreVary;
    encoder << options.cacheName;
}

bool ArgumentCoder<CacheQueryOptions>::decode(Decoder& decoder, CacheQueryOptions& options)
{
    bool ignoreSearch;
    if (!decoder.decode(ignoreSearch))
        return false;
    bool ignoreMethod;
    if (!decoder.decode(ignoreMethod))
        return false;
    bool ignoreVary;
    if (!decoder.decode(ignoreVary))
        return false;
    String cacheName;
    if (!decoder.decode(cacheName))
        return false;

    options.ignoreSearch = ignoreSearch;
    options.ignoreMethod = ignoreMethod;
    options.ignoreVary = ignoreVary;
    options.cacheName = WTFMove(cacheName);
    return true;
}

void ArgumentCoder<DOMCacheEngine::CacheInfo>::encode(Encoder& encoder, const DOMCacheEngine::CacheInfo& info)
{
    encoder << info.identifier;
    encoder << info.name;
}

auto ArgumentCoder<DOMCacheEngine::CacheInfo>::decode(Decoder& decoder) -> std::optional<DOMCacheEngine::CacheInfo>
{
    std::optional<uint64_t> identifier;
    decoder >> identifier;
    if (!identifier)
        return std::nullopt;
    
    std::optional<String> name;
    decoder >> name;
    if (!name)
        return std::nullopt;
    
    return {{ WTFMove(*identifier), WTFMove(*name) }};
}

void ArgumentCoder<DOMCacheEngine::Record>::encode(Encoder& encoder, const DOMCacheEngine::Record& record)
{
    encoder << record.identifier;

    encoder << record.requestHeadersGuard;
    encoder << record.request;
    encoder << record.options;
    encoder << record.referrer;

    encoder << record.responseHeadersGuard;
    encoder << record.response;
    encoder << record.updateResponseCounter;
    encoder << record.responseBodySize;

    WTF::switchOn(record.responseBody, [&](const Ref<SharedBuffer>& buffer) {
        encoder << true;
        encodeSharedBuffer(encoder, buffer.ptr());
    }, [&](const Ref<FormData>& formData) {
        encoder << false;
        encoder << true;
        formData->encode(encoder);
    }, [&](const std::nullptr_t&) {
        encoder << false;
        encoder << false;
    });
}

std::optional<DOMCacheEngine::Record> ArgumentCoder<DOMCacheEngine::Record>::decode(Decoder& decoder)
{
    uint64_t identifier;
    if (!decoder.decode(identifier))
        return std::nullopt;

    FetchHeaders::Guard requestHeadersGuard;
    if (!decoder.decode(requestHeadersGuard))
        return std::nullopt;

    WebCore::ResourceRequest request;
    if (!decoder.decode(request))
        return std::nullopt;

    std::optional<WebCore::FetchOptions> options;
    decoder >> options;
    if (!options)
        return std::nullopt;

    String referrer;
    if (!decoder.decode(referrer))
        return std::nullopt;

    FetchHeaders::Guard responseHeadersGuard;
    if (!decoder.decode(responseHeadersGuard))
        return std::nullopt;

    WebCore::ResourceResponse response;
    if (!decoder.decode(response))
        return std::nullopt;

    uint64_t updateResponseCounter;
    if (!decoder.decode(updateResponseCounter))
        return std::nullopt;

    uint64_t responseBodySize;
    if (!decoder.decode(responseBodySize))
        return std::nullopt;

    WebCore::DOMCacheEngine::ResponseBody responseBody;
    bool hasSharedBufferBody;
    if (!decoder.decode(hasSharedBufferBody))
        return std::nullopt;

    if (hasSharedBufferBody) {
        RefPtr<SharedBuffer> buffer;
        if (!decodeSharedBuffer(decoder, buffer))
            return std::nullopt;
        if (buffer)
            responseBody = buffer.releaseNonNull();
    } else {
        bool hasFormDataBody;
        if (!decoder.decode(hasFormDataBody))
            return std::nullopt;
        if (hasFormDataBody) {
            auto formData = FormData::decode(decoder);
            if (!formData)
                return std::nullopt;
            responseBody = formData.releaseNonNull();
        }
    }

    return {{ WTFMove(identifier), WTFMove(updateResponseCounter), WTFMove(requestHeadersGuard), WTFMove(request), WTFMove(options.value()), WTFMove(referrer), WTFMove(responseHeadersGuard), WTFMove(response), WTFMove(responseBody), responseBodySize }};
}

void ArgumentCoder<EventTrackingRegions>::encode(Encoder& encoder, const EventTrackingRegions& eventTrackingRegions)
{
    encoder << eventTrackingRegions.asynchronousDispatchRegion;
    encoder << eventTrackingRegions.eventSpecificSynchronousDispatchRegions;
}

bool ArgumentCoder<EventTrackingRegions>::decode(Decoder& decoder, EventTrackingRegions& eventTrackingRegions)
{
    Region asynchronousDispatchRegion;
    if (!decoder.decode(asynchronousDispatchRegion))
        return false;
    HashMap<String, Region> eventSpecificSynchronousDispatchRegions;
    if (!decoder.decode(eventSpecificSynchronousDispatchRegions))
        return false;
    eventTrackingRegions.asynchronousDispatchRegion = WTFMove(asynchronousDispatchRegion);
    eventTrackingRegions.eventSpecificSynchronousDispatchRegions = WTFMove(eventSpecificSynchronousDispatchRegions);
    return true;
}

void ArgumentCoder<TransformationMatrix>::encode(Encoder& encoder, const TransformationMatrix& transformationMatrix)
{
    encoder << transformationMatrix.m11();
    encoder << transformationMatrix.m12();
    encoder << transformationMatrix.m13();
    encoder << transformationMatrix.m14();

    encoder << transformationMatrix.m21();
    encoder << transformationMatrix.m22();
    encoder << transformationMatrix.m23();
    encoder << transformationMatrix.m24();

    encoder << transformationMatrix.m31();
    encoder << transformationMatrix.m32();
    encoder << transformationMatrix.m33();
    encoder << transformationMatrix.m34();

    encoder << transformationMatrix.m41();
    encoder << transformationMatrix.m42();
    encoder << transformationMatrix.m43();
    encoder << transformationMatrix.m44();
}

bool ArgumentCoder<TransformationMatrix>::decode(Decoder& decoder, TransformationMatrix& transformationMatrix)
{
    double m11;
    if (!decoder.decode(m11))
        return false;
    double m12;
    if (!decoder.decode(m12))
        return false;
    double m13;
    if (!decoder.decode(m13))
        return false;
    double m14;
    if (!decoder.decode(m14))
        return false;

    double m21;
    if (!decoder.decode(m21))
        return false;
    double m22;
    if (!decoder.decode(m22))
        return false;
    double m23;
    if (!decoder.decode(m23))
        return false;
    double m24;
    if (!decoder.decode(m24))
        return false;

    double m31;
    if (!decoder.decode(m31))
        return false;
    double m32;
    if (!decoder.decode(m32))
        return false;
    double m33;
    if (!decoder.decode(m33))
        return false;
    double m34;
    if (!decoder.decode(m34))
        return false;

    double m41;
    if (!decoder.decode(m41))
        return false;
    double m42;
    if (!decoder.decode(m42))
        return false;
    double m43;
    if (!decoder.decode(m43))
        return false;
    double m44;
    if (!decoder.decode(m44))
        return false;

    transformationMatrix.setMatrix(m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44);
    return true;
}

void ArgumentCoder<LinearTimingFunction>::encode(Encoder& encoder, const LinearTimingFunction& timingFunction)
{
    encoder.encodeEnum(timingFunction.type());
}

bool ArgumentCoder<LinearTimingFunction>::decode(Decoder&, LinearTimingFunction&)
{
    // Type is decoded by the caller. Nothing else to decode.
    return true;
}

void ArgumentCoder<CubicBezierTimingFunction>::encode(Encoder& encoder, const CubicBezierTimingFunction& timingFunction)
{
    encoder.encodeEnum(timingFunction.type());
    
    encoder << timingFunction.x1();
    encoder << timingFunction.y1();
    encoder << timingFunction.x2();
    encoder << timingFunction.y2();
    
    encoder.encodeEnum(timingFunction.timingFunctionPreset());
}

bool ArgumentCoder<CubicBezierTimingFunction>::decode(Decoder& decoder, CubicBezierTimingFunction& timingFunction)
{
    // Type is decoded by the caller.
    double x1;
    if (!decoder.decode(x1))
        return false;

    double y1;
    if (!decoder.decode(y1))
        return false;

    double x2;
    if (!decoder.decode(x2))
        return false;

    double y2;
    if (!decoder.decode(y2))
        return false;

    CubicBezierTimingFunction::TimingFunctionPreset preset;
    if (!decoder.decodeEnum(preset))
        return false;

    timingFunction.setValues(x1, y1, x2, y2);
    timingFunction.setTimingFunctionPreset(preset);

    return true;
}

void ArgumentCoder<StepsTimingFunction>::encode(Encoder& encoder, const StepsTimingFunction& timingFunction)
{
    encoder.encodeEnum(timingFunction.type());
    
    encoder << timingFunction.numberOfSteps();
    encoder << timingFunction.stepAtStart();
}

bool ArgumentCoder<StepsTimingFunction>::decode(Decoder& decoder, StepsTimingFunction& timingFunction)
{
    // Type is decoded by the caller.
    int numSteps;
    if (!decoder.decode(numSteps))
        return false;

    bool stepAtStart;
    if (!decoder.decode(stepAtStart))
        return false;

    timingFunction.setNumberOfSteps(numSteps);
    timingFunction.setStepAtStart(stepAtStart);

    return true;
}

void ArgumentCoder<FramesTimingFunction>::encode(Encoder& encoder, const FramesTimingFunction& timingFunction)
{
    encoder.encodeEnum(timingFunction.type());
    
    encoder << timingFunction.numberOfFrames();
}

bool ArgumentCoder<FramesTimingFunction>::decode(Decoder& decoder, FramesTimingFunction& timingFunction)
{
    // Type is decoded by the caller.
    int numFrames;
    if (!decoder.decode(numFrames))
        return false;

    timingFunction.setNumberOfFrames(numFrames);

    return true;
}

void ArgumentCoder<SpringTimingFunction>::encode(Encoder& encoder, const SpringTimingFunction& timingFunction)
{
    encoder.encodeEnum(timingFunction.type());
    
    encoder << timingFunction.mass();
    encoder << timingFunction.stiffness();
    encoder << timingFunction.damping();
    encoder << timingFunction.initialVelocity();
}

bool ArgumentCoder<SpringTimingFunction>::decode(Decoder& decoder, SpringTimingFunction& timingFunction)
{
    // Type is decoded by the caller.
    double mass;
    if (!decoder.decode(mass))
        return false;

    double stiffness;
    if (!decoder.decode(stiffness))
        return false;

    double damping;
    if (!decoder.decode(damping))
        return false;

    double initialVelocity;
    if (!decoder.decode(initialVelocity))
        return false;

    timingFunction.setValues(mass, stiffness, damping, initialVelocity);

    return true;
}

void ArgumentCoder<FloatPoint>::encode(Encoder& encoder, const FloatPoint& floatPoint)
{
    SimpleArgumentCoder<FloatPoint>::encode(encoder, floatPoint);
}

bool ArgumentCoder<FloatPoint>::decode(Decoder& decoder, FloatPoint& floatPoint)
{
    return SimpleArgumentCoder<FloatPoint>::decode(decoder, floatPoint);
}

std::optional<FloatPoint> ArgumentCoder<FloatPoint>::decode(Decoder& decoder)
{
    FloatPoint floatPoint;
    if (!SimpleArgumentCoder<FloatPoint>::decode(decoder, floatPoint))
        return std::nullopt;
    return WTFMove(floatPoint);
}

void ArgumentCoder<FloatPoint3D>::encode(Encoder& encoder, const FloatPoint3D& floatPoint)
{
    SimpleArgumentCoder<FloatPoint3D>::encode(encoder, floatPoint);
}

bool ArgumentCoder<FloatPoint3D>::decode(Decoder& decoder, FloatPoint3D& floatPoint)
{
    return SimpleArgumentCoder<FloatPoint3D>::decode(decoder, floatPoint);
}


void ArgumentCoder<FloatRect>::encode(Encoder& encoder, const FloatRect& floatRect)
{
    SimpleArgumentCoder<FloatRect>::encode(encoder, floatRect);
}

bool ArgumentCoder<FloatRect>::decode(Decoder& decoder, FloatRect& floatRect)
{
    return SimpleArgumentCoder<FloatRect>::decode(decoder, floatRect);
}

std::optional<FloatRect> ArgumentCoder<FloatRect>::decode(Decoder& decoder)
{
    FloatRect floatRect;
    if (!SimpleArgumentCoder<FloatRect>::decode(decoder, floatRect))
        return std::nullopt;
    return WTFMove(floatRect);
}


void ArgumentCoder<FloatBoxExtent>::encode(Encoder& encoder, const FloatBoxExtent& floatBoxExtent)
{
    SimpleArgumentCoder<FloatBoxExtent>::encode(encoder, floatBoxExtent);
}
    
bool ArgumentCoder<FloatBoxExtent>::decode(Decoder& decoder, FloatBoxExtent& floatBoxExtent)
{
    return SimpleArgumentCoder<FloatBoxExtent>::decode(decoder, floatBoxExtent);
}
    

void ArgumentCoder<FloatSize>::encode(Encoder& encoder, const FloatSize& floatSize)
{
    SimpleArgumentCoder<FloatSize>::encode(encoder, floatSize);
}

bool ArgumentCoder<FloatSize>::decode(Decoder& decoder, FloatSize& floatSize)
{
    return SimpleArgumentCoder<FloatSize>::decode(decoder, floatSize);
}


void ArgumentCoder<FloatRoundedRect>::encode(Encoder& encoder, const FloatRoundedRect& roundedRect)
{
    SimpleArgumentCoder<FloatRoundedRect>::encode(encoder, roundedRect);
}

bool ArgumentCoder<FloatRoundedRect>::decode(Decoder& decoder, FloatRoundedRect& roundedRect)
{
    return SimpleArgumentCoder<FloatRoundedRect>::decode(decoder, roundedRect);
}

#if PLATFORM(IOS)
void ArgumentCoder<FloatQuad>::encode(Encoder& encoder, const FloatQuad& floatQuad)
{
    SimpleArgumentCoder<FloatQuad>::encode(encoder, floatQuad);
}

std::optional<FloatQuad> ArgumentCoder<FloatQuad>::decode(Decoder& decoder)
{
    FloatQuad floatQuad;
    if (!SimpleArgumentCoder<FloatQuad>::decode(decoder, floatQuad))
        return std::nullopt;
    return WTFMove(floatQuad);
}

void ArgumentCoder<ViewportArguments>::encode(Encoder& encoder, const ViewportArguments& viewportArguments)
{
    SimpleArgumentCoder<ViewportArguments>::encode(encoder, viewportArguments);
}

bool ArgumentCoder<ViewportArguments>::decode(Decoder& decoder, ViewportArguments& viewportArguments)
{
    return SimpleArgumentCoder<ViewportArguments>::decode(decoder, viewportArguments);
}
#endif // PLATFORM(IOS)


void ArgumentCoder<IntPoint>::encode(Encoder& encoder, const IntPoint& intPoint)
{
    SimpleArgumentCoder<IntPoint>::encode(encoder, intPoint);
}

bool ArgumentCoder<IntPoint>::decode(Decoder& decoder, IntPoint& intPoint)
{
    return SimpleArgumentCoder<IntPoint>::decode(decoder, intPoint);
}

std::optional<WebCore::IntPoint> ArgumentCoder<IntPoint>::decode(Decoder& decoder)
{
    IntPoint intPoint;
    if (!SimpleArgumentCoder<IntPoint>::decode(decoder, intPoint))
        return std::nullopt;
    return WTFMove(intPoint);
}

void ArgumentCoder<IntRect>::encode(Encoder& encoder, const IntRect& intRect)
{
    SimpleArgumentCoder<IntRect>::encode(encoder, intRect);
}

bool ArgumentCoder<IntRect>::decode(Decoder& decoder, IntRect& intRect)
{
    return SimpleArgumentCoder<IntRect>::decode(decoder, intRect);
}

std::optional<IntRect> ArgumentCoder<IntRect>::decode(Decoder& decoder)
{
    IntRect rect;
    if (!decode(decoder, rect))
        return std::nullopt;
    return WTFMove(rect);
}

void ArgumentCoder<IntSize>::encode(Encoder& encoder, const IntSize& intSize)
{
    SimpleArgumentCoder<IntSize>::encode(encoder, intSize);
}

bool ArgumentCoder<IntSize>::decode(Decoder& decoder, IntSize& intSize)
{
    return SimpleArgumentCoder<IntSize>::decode(decoder, intSize);
}

std::optional<IntSize> ArgumentCoder<IntSize>::decode(Decoder& decoder)
{
    IntSize intSize;
    if (!SimpleArgumentCoder<IntSize>::decode(decoder, intSize))
        return std::nullopt;
    return WTFMove(intSize);
}

void ArgumentCoder<LayoutSize>::encode(Encoder& encoder, const LayoutSize& layoutSize)
{
    SimpleArgumentCoder<LayoutSize>::encode(encoder, layoutSize);
}

bool ArgumentCoder<LayoutSize>::decode(Decoder& decoder, LayoutSize& layoutSize)
{
    return SimpleArgumentCoder<LayoutSize>::decode(decoder, layoutSize);
}


void ArgumentCoder<LayoutPoint>::encode(Encoder& encoder, const LayoutPoint& layoutPoint)
{
    SimpleArgumentCoder<LayoutPoint>::encode(encoder, layoutPoint);
}

bool ArgumentCoder<LayoutPoint>::decode(Decoder& decoder, LayoutPoint& layoutPoint)
{
    return SimpleArgumentCoder<LayoutPoint>::decode(decoder, layoutPoint);
}


static void pathEncodeApplierFunction(Encoder& encoder, const PathElement& element)
{
    encoder.encodeEnum(element.type);

    switch (element.type) {
    case PathElementMoveToPoint: // The points member will contain 1 value.
        encoder << element.points[0];
        break;
    case PathElementAddLineToPoint: // The points member will contain 1 value.
        encoder << element.points[0];
        break;
    case PathElementAddQuadCurveToPoint: // The points member will contain 2 values.
        encoder << element.points[0];
        encoder << element.points[1];
        break;
    case PathElementAddCurveToPoint: // The points member will contain 3 values.
        encoder << element.points[0];
        encoder << element.points[1];
        encoder << element.points[2];
        break;
    case PathElementCloseSubpath: // The points member will contain no values.
        break;
    }
}

void ArgumentCoder<Path>::encode(Encoder& encoder, const Path& path)
{
    uint64_t numPoints = 0;
    path.apply([&numPoints](const PathElement&) {
        ++numPoints;
    });

    encoder << numPoints;

    path.apply([&encoder](const PathElement& pathElement) {
        pathEncodeApplierFunction(encoder, pathElement);
    });
}

bool ArgumentCoder<Path>::decode(Decoder& decoder, Path& path)
{
    uint64_t numPoints;
    if (!decoder.decode(numPoints))
        return false;
    
    path.clear();

    for (uint64_t i = 0; i < numPoints; ++i) {
    
        PathElementType elementType;
        if (!decoder.decodeEnum(elementType))
            return false;
        
        switch (elementType) {
        case PathElementMoveToPoint: { // The points member will contain 1 value.
            FloatPoint point;
            if (!decoder.decode(point))
                return false;
            path.moveTo(point);
            break;
        }
        case PathElementAddLineToPoint: { // The points member will contain 1 value.
            FloatPoint point;
            if (!decoder.decode(point))
                return false;
            path.addLineTo(point);
            break;
        }
        case PathElementAddQuadCurveToPoint: { // The points member will contain 2 values.
            FloatPoint controlPoint;
            if (!decoder.decode(controlPoint))
                return false;

            FloatPoint endPoint;
            if (!decoder.decode(endPoint))
                return false;

            path.addQuadCurveTo(controlPoint, endPoint);
            break;
        }
        case PathElementAddCurveToPoint: { // The points member will contain 3 values.
            FloatPoint controlPoint1;
            if (!decoder.decode(controlPoint1))
                return false;

            FloatPoint controlPoint2;
            if (!decoder.decode(controlPoint2))
                return false;

            FloatPoint endPoint;
            if (!decoder.decode(endPoint))
                return false;

            path.addBezierCurveTo(controlPoint1, controlPoint2, endPoint);
            break;
        }
        case PathElementCloseSubpath: // The points member will contain no values.
            path.closeSubpath();
            break;
        }
    }

    return true;
}

void ArgumentCoder<RecentSearch>::encode(Encoder& encoder, const RecentSearch& recentSearch)
{
    encoder << recentSearch.string << recentSearch.time;
}

std::optional<RecentSearch> ArgumentCoder<RecentSearch>::decode(Decoder& decoder)
{
    std::optional<String> string;
    decoder >> string;
    if (!string)
        return std::nullopt;
    
    std::optional<WallTime> time;
    decoder >> time;
    if (!time)
        return std::nullopt;
    
    return {{ WTFMove(*string), WTFMove(*time) }};
}

template<> struct ArgumentCoder<Region::Span> {
    static void encode(Encoder&, const Region::Span&);
    static std::optional<Region::Span> decode(Decoder&);
};

void ArgumentCoder<Region::Span>::encode(Encoder& encoder, const Region::Span& span)
{
    encoder << span.y;
    encoder << (uint64_t)span.segmentIndex;
}

std::optional<Region::Span> ArgumentCoder<Region::Span>::decode(Decoder& decoder)
{
    Region::Span span;
    if (!decoder.decode(span.y))
        return std::nullopt;
    
    uint64_t segmentIndex;
    if (!decoder.decode(segmentIndex))
        return std::nullopt;
    
    span.segmentIndex = segmentIndex;
    return WTFMove(span);
}

void ArgumentCoder<Region>::encode(Encoder& encoder, const Region& region)
{
    encoder.encode(region.shapeSegments());
    encoder.encode(region.shapeSpans());
}

bool ArgumentCoder<Region>::decode(Decoder& decoder, Region& region)
{
    Vector<int> segments;
    if (!decoder.decode(segments))
        return false;

    Vector<Region::Span> spans;
    if (!decoder.decode(spans))
        return false;
    
    region.setShapeSegments(segments);
    region.setShapeSpans(spans);
    region.updateBoundsFromShape();
    
    if (!region.isValid())
        return false;

    return true;
}

void ArgumentCoder<Length>::encode(Encoder& encoder, const Length& length)
{
    SimpleArgumentCoder<Length>::encode(encoder, length);
}

bool ArgumentCoder<Length>::decode(Decoder& decoder, Length& length)
{
    return SimpleArgumentCoder<Length>::decode(decoder, length);
}


void ArgumentCoder<ViewportAttributes>::encode(Encoder& encoder, const ViewportAttributes& viewportAttributes)
{
    SimpleArgumentCoder<ViewportAttributes>::encode(encoder, viewportAttributes);
}

bool ArgumentCoder<ViewportAttributes>::decode(Decoder& decoder, ViewportAttributes& viewportAttributes)
{
    return SimpleArgumentCoder<ViewportAttributes>::decode(decoder, viewportAttributes);
}


void ArgumentCoder<MimeClassInfo>::encode(Encoder& encoder, const MimeClassInfo& mimeClassInfo)
{
    encoder << mimeClassInfo.type << mimeClassInfo.desc << mimeClassInfo.extensions;
}

std::optional<MimeClassInfo> ArgumentCoder<MimeClassInfo>::decode(Decoder& decoder)
{
    MimeClassInfo mimeClassInfo;
    if (!decoder.decode(mimeClassInfo.type))
        return std::nullopt;
    if (!decoder.decode(mimeClassInfo.desc))
        return std::nullopt;
    if (!decoder.decode(mimeClassInfo.extensions))
        return std::nullopt;

    return WTFMove(mimeClassInfo);
}


void ArgumentCoder<PluginInfo>::encode(Encoder& encoder, const PluginInfo& pluginInfo)
{
    encoder << pluginInfo.name;
    encoder << pluginInfo.file;
    encoder << pluginInfo.desc;
    encoder << pluginInfo.mimes;
    encoder << pluginInfo.isApplicationPlugin;
    encoder.encodeEnum(pluginInfo.clientLoadPolicy);
#if PLATFORM(MAC)
    encoder << pluginInfo.bundleIdentifier;
    encoder << pluginInfo.versionString;
#endif
}

std::optional<WebCore::PluginInfo> ArgumentCoder<PluginInfo>::decode(Decoder& decoder)
{
    PluginInfo pluginInfo;
    if (!decoder.decode(pluginInfo.name))
        return std::nullopt;
    if (!decoder.decode(pluginInfo.file))
        return std::nullopt;
    if (!decoder.decode(pluginInfo.desc))
        return std::nullopt;
    if (!decoder.decode(pluginInfo.mimes))
        return std::nullopt;
    if (!decoder.decode(pluginInfo.isApplicationPlugin))
        return std::nullopt;
    if (!decoder.decodeEnum(pluginInfo.clientLoadPolicy))
        return std::nullopt;
#if PLATFORM(MAC)
    if (!decoder.decode(pluginInfo.bundleIdentifier))
        return std::nullopt;
    if (!decoder.decode(pluginInfo.versionString))
        return std::nullopt;
#endif

    return WTFMove(pluginInfo);
}

void ArgumentCoder<AuthenticationChallenge>::encode(Encoder& encoder, const AuthenticationChallenge& challenge)
{
    encoder << challenge.protectionSpace() << challenge.proposedCredential() << challenge.previousFailureCount() << challenge.failureResponse() << challenge.error();
}

bool ArgumentCoder<AuthenticationChallenge>::decode(Decoder& decoder, AuthenticationChallenge& challenge)
{    
    ProtectionSpace protectionSpace;
    if (!decoder.decode(protectionSpace))
        return false;

    Credential proposedCredential;
    if (!decoder.decode(proposedCredential))
        return false;

    unsigned previousFailureCount;    
    if (!decoder.decode(previousFailureCount))
        return false;

    ResourceResponse failureResponse;
    if (!decoder.decode(failureResponse))
        return false;

    ResourceError error;
    if (!decoder.decode(error))
        return false;
    
    challenge = AuthenticationChallenge(protectionSpace, proposedCredential, previousFailureCount, failureResponse, error);
    return true;
}


void ArgumentCoder<ProtectionSpace>::encode(Encoder& encoder, const ProtectionSpace& space)
{
    if (space.encodingRequiresPlatformData()) {
        encoder << true;
        encodePlatformData(encoder, space);
        return;
    }

    encoder << false;
    encoder << space.host() << space.port() << space.realm();
    encoder.encodeEnum(space.authenticationScheme());
    encoder.encodeEnum(space.serverType());
}

bool ArgumentCoder<ProtectionSpace>::decode(Decoder& decoder, ProtectionSpace& space)
{
    bool hasPlatformData;
    if (!decoder.decode(hasPlatformData))
        return false;

    if (hasPlatformData)
        return decodePlatformData(decoder, space);

    String host;
    if (!decoder.decode(host))
        return false;

    int port;
    if (!decoder.decode(port))
        return false;

    String realm;
    if (!decoder.decode(realm))
        return false;
    
    ProtectionSpaceAuthenticationScheme authenticationScheme;
    if (!decoder.decodeEnum(authenticationScheme))
        return false;

    ProtectionSpaceServerType serverType;
    if (!decoder.decodeEnum(serverType))
        return false;

    space = ProtectionSpace(host, port, serverType, realm, authenticationScheme);
    return true;
}

void ArgumentCoder<Credential>::encode(Encoder& encoder, const Credential& credential)
{
    if (credential.encodingRequiresPlatformData()) {
        encoder << true;
        encodePlatformData(encoder, credential);
        return;
    }

    encoder << false;
    encoder << credential.user() << credential.password();
    encoder.encodeEnum(credential.persistence());
}

bool ArgumentCoder<Credential>::decode(Decoder& decoder, Credential& credential)
{
    bool hasPlatformData;
    if (!decoder.decode(hasPlatformData))
        return false;

    if (hasPlatformData)
        return decodePlatformData(decoder, credential);

    String user;
    if (!decoder.decode(user))
        return false;

    String password;
    if (!decoder.decode(password))
        return false;

    CredentialPersistence persistence;
    if (!decoder.decodeEnum(persistence))
        return false;
    
    credential = Credential(user, password, persistence);
    return true;
}

static void encodeImage(Encoder& encoder, Image& image)
{
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(IntSize(image.size()), { });
    bitmap->createGraphicsContext()->drawImage(image, IntPoint());

    ShareableBitmap::Handle handle;
    bitmap->createHandle(handle);

    encoder << handle;
}

static bool decodeImage(Decoder& decoder, RefPtr<Image>& image)
{
    ShareableBitmap::Handle handle;
    if (!decoder.decode(handle))
        return false;
    
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::create(handle);
    if (!bitmap)
        return false;
    image = bitmap->createImage();
    if (!image)
        return false;
    return true;
}

static void encodeOptionalImage(Encoder& encoder, Image* image)
{
    bool hasImage = !!image;
    encoder << hasImage;

    if (hasImage)
        encodeImage(encoder, *image);
}

static bool decodeOptionalImage(Decoder& decoder, RefPtr<Image>& image)
{
    image = nullptr;

    bool hasImage;
    if (!decoder.decode(hasImage))
        return false;

    if (!hasImage)
        return true;

    return decodeImage(decoder, image);
}

#if !PLATFORM(IOS)
void ArgumentCoder<Cursor>::encode(Encoder& encoder, const Cursor& cursor)
{
    encoder.encodeEnum(cursor.type());
        
    if (cursor.type() != Cursor::Custom)
        return;

    if (cursor.image()->isNull()) {
        encoder << false; // There is no valid image being encoded.
        return;
    }

    encoder << true;
    encodeImage(encoder, *cursor.image());
    encoder << cursor.hotSpot();
#if ENABLE(MOUSE_CURSOR_SCALE)
    encoder << cursor.imageScaleFactor();
#endif
}

bool ArgumentCoder<Cursor>::decode(Decoder& decoder, Cursor& cursor)
{
    Cursor::Type type;
    if (!decoder.decodeEnum(type))
        return false;

    if (type > Cursor::Custom)
        return false;

    if (type != Cursor::Custom) {
        const Cursor& cursorReference = Cursor::fromType(type);
        // Calling platformCursor here will eagerly create the platform cursor for the cursor singletons inside WebCore.
        // This will avoid having to re-create the platform cursors over and over.
        (void)cursorReference.platformCursor();

        cursor = cursorReference;
        return true;
    }

    bool isValidImagePresent;
    if (!decoder.decode(isValidImagePresent))
        return false;

    if (!isValidImagePresent) {
        cursor = Cursor(&Image::nullImage(), IntPoint());
        return true;
    }

    RefPtr<Image> image;
    if (!decodeImage(decoder, image))
        return false;

    IntPoint hotSpot;
    if (!decoder.decode(hotSpot))
        return false;

    if (!image->rect().contains(hotSpot))
        return false;

#if ENABLE(MOUSE_CURSOR_SCALE)
    float scale;
    if (!decoder.decode(scale))
        return false;

    cursor = Cursor(image.get(), hotSpot, scale);
#else
    cursor = Cursor(image.get(), hotSpot);
#endif
    return true;
}
#endif

void ArgumentCoder<ResourceRequest>::encode(Encoder& encoder, const ResourceRequest& resourceRequest)
{
    encoder << resourceRequest.cachePartition();
    encoder << resourceRequest.hiddenFromInspector();

    if (resourceRequest.encodingRequiresPlatformData()) {
        encoder << true;
        encodePlatformData(encoder, resourceRequest);
        return;
    }
    encoder << false;
    resourceRequest.encodeWithoutPlatformData(encoder);
}

bool ArgumentCoder<ResourceRequest>::decode(Decoder& decoder, ResourceRequest& resourceRequest)
{
    String cachePartition;
    if (!decoder.decode(cachePartition))
        return false;
    resourceRequest.setCachePartition(cachePartition);

    bool isHiddenFromInspector;
    if (!decoder.decode(isHiddenFromInspector))
        return false;
    resourceRequest.setHiddenFromInspector(isHiddenFromInspector);

    bool hasPlatformData;
    if (!decoder.decode(hasPlatformData))
        return false;
    if (hasPlatformData)
        return decodePlatformData(decoder, resourceRequest);

    return resourceRequest.decodeWithoutPlatformData(decoder);
}

void ArgumentCoder<ResourceError>::encode(Encoder& encoder, const ResourceError& resourceError)
{
    encodePlatformData(encoder, resourceError);
}

bool ArgumentCoder<ResourceError>::decode(Decoder& decoder, ResourceError& resourceError)
{
    return decodePlatformData(decoder, resourceError);
}

#if PLATFORM(IOS)

void ArgumentCoder<SelectionRect>::encode(Encoder& encoder, const SelectionRect& selectionRect)
{
    encoder << selectionRect.rect();
    encoder << static_cast<uint32_t>(selectionRect.direction());
    encoder << selectionRect.minX();
    encoder << selectionRect.maxX();
    encoder << selectionRect.maxY();
    encoder << selectionRect.lineNumber();
    encoder << selectionRect.isLineBreak();
    encoder << selectionRect.isFirstOnLine();
    encoder << selectionRect.isLastOnLine();
    encoder << selectionRect.containsStart();
    encoder << selectionRect.containsEnd();
    encoder << selectionRect.isHorizontal();
}

std::optional<SelectionRect> ArgumentCoder<SelectionRect>::decode(Decoder& decoder)
{
    SelectionRect selectionRect;
    IntRect rect;
    if (!decoder.decode(rect))
        return std::nullopt;
    selectionRect.setRect(rect);

    uint32_t direction;
    if (!decoder.decode(direction))
        return std::nullopt;
    selectionRect.setDirection((TextDirection)direction);

    int intValue;
    if (!decoder.decode(intValue))
        return std::nullopt;
    selectionRect.setMinX(intValue);

    if (!decoder.decode(intValue))
        return std::nullopt;
    selectionRect.setMaxX(intValue);

    if (!decoder.decode(intValue))
        return std::nullopt;
    selectionRect.setMaxY(intValue);

    if (!decoder.decode(intValue))
        return std::nullopt;
    selectionRect.setLineNumber(intValue);

    bool boolValue;
    if (!decoder.decode(boolValue))
        return std::nullopt;
    selectionRect.setIsLineBreak(boolValue);

    if (!decoder.decode(boolValue))
        return std::nullopt;
    selectionRect.setIsFirstOnLine(boolValue);

    if (!decoder.decode(boolValue))
        return std::nullopt;
    selectionRect.setIsLastOnLine(boolValue);

    if (!decoder.decode(boolValue))
        return std::nullopt;
    selectionRect.setContainsStart(boolValue);

    if (!decoder.decode(boolValue))
        return std::nullopt;
    selectionRect.setContainsEnd(boolValue);

    if (!decoder.decode(boolValue))
        return std::nullopt;
    selectionRect.setIsHorizontal(boolValue);

    return WTFMove(selectionRect);
}

#endif

void ArgumentCoder<WindowFeatures>::encode(Encoder& encoder, const WindowFeatures& windowFeatures)
{
    encoder << windowFeatures.x;
    encoder << windowFeatures.y;
    encoder << windowFeatures.width;
    encoder << windowFeatures.height;
    encoder << windowFeatures.menuBarVisible;
    encoder << windowFeatures.statusBarVisible;
    encoder << windowFeatures.toolBarVisible;
    encoder << windowFeatures.locationBarVisible;
    encoder << windowFeatures.scrollbarsVisible;
    encoder << windowFeatures.resizable;
    encoder << windowFeatures.fullscreen;
    encoder << windowFeatures.dialog;
}

bool ArgumentCoder<WindowFeatures>::decode(Decoder& decoder, WindowFeatures& windowFeatures)
{
    if (!decoder.decode(windowFeatures.x))
        return false;
    if (!decoder.decode(windowFeatures.y))
        return false;
    if (!decoder.decode(windowFeatures.width))
        return false;
    if (!decoder.decode(windowFeatures.height))
        return false;
    if (!decoder.decode(windowFeatures.menuBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.statusBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.toolBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.locationBarVisible))
        return false;
    if (!decoder.decode(windowFeatures.scrollbarsVisible))
        return false;
    if (!decoder.decode(windowFeatures.resizable))
        return false;
    if (!decoder.decode(windowFeatures.fullscreen))
        return false;
    if (!decoder.decode(windowFeatures.dialog))
        return false;
    return true;
}


void ArgumentCoder<Color>::encode(Encoder& encoder, const Color& color)
{
    if (color.isExtended()) {
        encoder << true;
        encoder << color.asExtended().red();
        encoder << color.asExtended().green();
        encoder << color.asExtended().blue();
        encoder << color.asExtended().alpha();
        encoder << color.asExtended().colorSpace();
        return;
    }

    encoder << false;

    if (!color.isValid()) {
        encoder << false;
        return;
    }

    encoder << true;
    encoder << color.rgb();
}

bool ArgumentCoder<Color>::decode(Decoder& decoder, Color& color)
{
    bool isExtended;
    if (!decoder.decode(isExtended))
        return false;

    if (isExtended) {
        float red;
        float green;
        float blue;
        float alpha;
        ColorSpace colorSpace;
        if (!decoder.decode(red))
            return false;
        if (!decoder.decode(green))
            return false;
        if (!decoder.decode(blue))
            return false;
        if (!decoder.decode(alpha))
            return false;
        if (!decoder.decode(colorSpace))
            return false;
        color = Color(red, green, blue, alpha, colorSpace);
        return true;
    }

    bool isValid;
    if (!decoder.decode(isValid))
        return false;

    if (!isValid) {
        color = Color();
        return true;
    }

    RGBA32 rgba;
    if (!decoder.decode(rgba))
        return false;

    color = Color(rgba);
    return true;
}

#if ENABLE(DRAG_SUPPORT)
void ArgumentCoder<DragData>::encode(Encoder& encoder, const DragData& dragData)
{
    encoder << dragData.clientPosition();
    encoder << dragData.globalPosition();
    encoder.encodeEnum(dragData.draggingSourceOperationMask());
    encoder.encodeEnum(dragData.flags());
#if PLATFORM(COCOA)
    encoder << dragData.pasteboardName();
    encoder << dragData.fileNames();
#endif
    encoder.encodeEnum(dragData.dragDestinationAction());
}

bool ArgumentCoder<DragData>::decode(Decoder& decoder, DragData& dragData)
{
    IntPoint clientPosition;
    if (!decoder.decode(clientPosition))
        return false;

    IntPoint globalPosition;
    if (!decoder.decode(globalPosition))
        return false;

    DragOperation draggingSourceOperationMask;
    if (!decoder.decodeEnum(draggingSourceOperationMask))
        return false;

    DragApplicationFlags applicationFlags;
    if (!decoder.decodeEnum(applicationFlags))
        return false;

    String pasteboardName;
    Vector<String> fileNames;
#if PLATFORM(COCOA)
    if (!decoder.decode(pasteboardName))
        return false;

    if (!decoder.decode(fileNames))
        return false;
#endif

    DragDestinationAction destinationAction;
    if (!decoder.decodeEnum(destinationAction))
        return false;

    dragData = DragData(pasteboardName, clientPosition, globalPosition, draggingSourceOperationMask, applicationFlags, destinationAction);
    dragData.setFileNames(fileNames);

    return true;
}
#endif

void ArgumentCoder<CompositionUnderline>::encode(Encoder& encoder, const CompositionUnderline& underline)
{
    encoder << underline.startOffset;
    encoder << underline.endOffset;
    encoder << underline.thick;
    encoder << underline.color;
}

std::optional<CompositionUnderline> ArgumentCoder<CompositionUnderline>::decode(Decoder& decoder)
{
    CompositionUnderline underline;

    if (!decoder.decode(underline.startOffset))
        return std::nullopt;
    if (!decoder.decode(underline.endOffset))
        return std::nullopt;
    if (!decoder.decode(underline.thick))
        return std::nullopt;
    if (!decoder.decode(underline.color))
        return std::nullopt;

    return WTFMove(underline);
}

void ArgumentCoder<DatabaseDetails>::encode(Encoder& encoder, const DatabaseDetails& details)
{
    encoder << details.name();
    encoder << details.displayName();
    encoder << details.expectedUsage();
    encoder << details.currentUsage();
    encoder << details.creationTime();
    encoder << details.modificationTime();
}
    
bool ArgumentCoder<DatabaseDetails>::decode(Decoder& decoder, DatabaseDetails& details)
{
    String name;
    if (!decoder.decode(name))
        return false;

    String displayName;
    if (!decoder.decode(displayName))
        return false;

    uint64_t expectedUsage;
    if (!decoder.decode(expectedUsage))
        return false;

    uint64_t currentUsage;
    if (!decoder.decode(currentUsage))
        return false;

    double creationTime;
    if (!decoder.decode(creationTime))
        return false;

    double modificationTime;
    if (!decoder.decode(modificationTime))
        return false;

    details = DatabaseDetails(name, displayName, expectedUsage, currentUsage, creationTime, modificationTime);
    return true;
}

void ArgumentCoder<PasteboardCustomData>::encode(Encoder& encoder, const PasteboardCustomData& data)
{
    encoder << data.origin;
    encoder << data.orderedTypes;
    encoder << data.platformData;
    encoder << data.sameOriginCustomData;
}

bool ArgumentCoder<PasteboardCustomData>::decode(Decoder& decoder, PasteboardCustomData& data)
{
    if (!decoder.decode(data.origin))
        return false;

    if (!decoder.decode(data.orderedTypes))
        return false;

    if (!decoder.decode(data.platformData))
        return false;

    if (!decoder.decode(data.sameOriginCustomData))
        return false;

    return true;
}

#if PLATFORM(IOS)

void ArgumentCoder<Highlight>::encode(Encoder& encoder, const Highlight& highlight)
{
    encoder << static_cast<uint32_t>(highlight.type);
    encoder << highlight.usePageCoordinates;
    encoder << highlight.contentColor;
    encoder << highlight.contentOutlineColor;
    encoder << highlight.paddingColor;
    encoder << highlight.borderColor;
    encoder << highlight.marginColor;
    encoder << highlight.quads;
}

bool ArgumentCoder<Highlight>::decode(Decoder& decoder, Highlight& highlight)
{
    uint32_t type;
    if (!decoder.decode(type))
        return false;
    highlight.type = (HighlightType)type;

    if (!decoder.decode(highlight.usePageCoordinates))
        return false;
    if (!decoder.decode(highlight.contentColor))
        return false;
    if (!decoder.decode(highlight.contentOutlineColor))
        return false;
    if (!decoder.decode(highlight.paddingColor))
        return false;
    if (!decoder.decode(highlight.borderColor))
        return false;
    if (!decoder.decode(highlight.marginColor))
        return false;
    if (!decoder.decode(highlight.quads))
        return false;
    return true;
}

void ArgumentCoder<PasteboardURL>::encode(Encoder& encoder, const PasteboardURL& content)
{
    encoder << content.url;
    encoder << content.title;
}

bool ArgumentCoder<PasteboardURL>::decode(Decoder& decoder, PasteboardURL& content)
{
    if (!decoder.decode(content.url))
        return false;

    if (!decoder.decode(content.title))
        return false;

    return true;
}

void ArgumentCoder<PasteboardWebContent>::encode(Encoder& encoder, const PasteboardWebContent& content)
{
    encoder << content.contentOrigin;
    encoder << content.canSmartCopyOrDelete;
    encoder << content.dataInStringFormat;
    encoder << content.dataInHTMLFormat;

    encodeSharedBuffer(encoder, content.dataInWebArchiveFormat.get());
    encodeSharedBuffer(encoder, content.dataInRTFDFormat.get());
    encodeSharedBuffer(encoder, content.dataInRTFFormat.get());
    encodeSharedBuffer(encoder, content.dataInAttributedStringFormat.get());

    encodeTypesAndData(encoder, content.clientTypes, content.clientData);
}

bool ArgumentCoder<PasteboardWebContent>::decode(Decoder& decoder, PasteboardWebContent& content)
{
    if (!decoder.decode(content.contentOrigin))
        return false;
    if (!decoder.decode(content.canSmartCopyOrDelete))
        return false;
    if (!decoder.decode(content.dataInStringFormat))
        return false;
    if (!decoder.decode(content.dataInHTMLFormat))
        return false;
    if (!decodeSharedBuffer(decoder, content.dataInWebArchiveFormat))
        return false;
    if (!decodeSharedBuffer(decoder, content.dataInRTFDFormat))
        return false;
    if (!decodeSharedBuffer(decoder, content.dataInRTFFormat))
        return false;
    if (!decodeSharedBuffer(decoder, content.dataInAttributedStringFormat))
        return false;
    if (!decodeTypesAndData(decoder, content.clientTypes, content.clientData))
        return false;
    return true;
}

void ArgumentCoder<PasteboardImage>::encode(Encoder& encoder, const PasteboardImage& pasteboardImage)
{
    encodeOptionalImage(encoder, pasteboardImage.image.get());
    encoder << pasteboardImage.url.url;
    encoder << pasteboardImage.url.title;
    encoder << pasteboardImage.resourceMIMEType;
    encoder << pasteboardImage.suggestedName;
    encoder << pasteboardImage.imageSize;
    if (pasteboardImage.resourceData)
        encodeSharedBuffer(encoder, pasteboardImage.resourceData.get());
    encodeTypesAndData(encoder, pasteboardImage.clientTypes, pasteboardImage.clientData);
}

bool ArgumentCoder<PasteboardImage>::decode(Decoder& decoder, PasteboardImage& pasteboardImage)
{
    if (!decodeOptionalImage(decoder, pasteboardImage.image))
        return false;
    if (!decoder.decode(pasteboardImage.url.url))
        return false;
    if (!decoder.decode(pasteboardImage.url.title))
        return false;
    if (!decoder.decode(pasteboardImage.resourceMIMEType))
        return false;
    if (!decoder.decode(pasteboardImage.suggestedName))
        return false;
    if (!decoder.decode(pasteboardImage.imageSize))
        return false;
    if (!decodeSharedBuffer(decoder, pasteboardImage.resourceData))
        return false;
    if (!decodeTypesAndData(decoder, pasteboardImage.clientTypes, pasteboardImage.clientData))
        return false;
    return true;
}

#endif

#if PLATFORM(WPE)
void ArgumentCoder<PasteboardWebContent>::encode(Encoder& encoder, const PasteboardWebContent& content)
{
    encoder << content.text;
    encoder << content.markup;
}

bool ArgumentCoder<PasteboardWebContent>::decode(Decoder& decoder, PasteboardWebContent& content)
{
    if (!decoder.decode(content.text))
        return false;
    if (!decoder.decode(content.markup))
        return false;
    return true;
}
#endif // PLATFORM(WPE)

void ArgumentCoder<DictationAlternative>::encode(Encoder& encoder, const DictationAlternative& dictationAlternative)
{
    encoder << dictationAlternative.rangeStart;
    encoder << dictationAlternative.rangeLength;
    encoder << dictationAlternative.dictationContext;
}

std::optional<DictationAlternative> ArgumentCoder<DictationAlternative>::decode(Decoder& decoder)
{
    std::optional<unsigned> rangeStart;
    decoder >> rangeStart;
    if (!rangeStart)
        return std::nullopt;
    
    std::optional<unsigned> rangeLength;
    decoder >> rangeLength;
    if (!rangeLength)
        return std::nullopt;
    
    std::optional<uint64_t> dictationContext;
    decoder >> dictationContext;
    if (!dictationContext)
        return std::nullopt;
    
    return {{ WTFMove(*rangeStart), WTFMove(*rangeLength), WTFMove(*dictationContext) }};
}

void ArgumentCoder<FileChooserSettings>::encode(Encoder& encoder, const FileChooserSettings& settings)
{
    encoder << settings.allowsDirectories;
    encoder << settings.allowsMultipleFiles;
    encoder << settings.acceptMIMETypes;
    encoder << settings.acceptFileExtensions;
    encoder << settings.selectedFiles;
#if ENABLE(MEDIA_CAPTURE)
    encoder.encodeEnum(settings.mediaCaptureType);
#endif
}

bool ArgumentCoder<FileChooserSettings>::decode(Decoder& decoder, FileChooserSettings& settings)
{
    if (!decoder.decode(settings.allowsDirectories))
        return false;
    if (!decoder.decode(settings.allowsMultipleFiles))
        return false;
    if (!decoder.decode(settings.acceptMIMETypes))
        return false;
    if (!decoder.decode(settings.acceptFileExtensions))
        return false;
    if (!decoder.decode(settings.selectedFiles))
        return false;
#if ENABLE(MEDIA_CAPTURE)
    if (!decoder.decodeEnum(settings.mediaCaptureType))
        return false;
#endif

    return true;
}


void ArgumentCoder<GrammarDetail>::encode(Encoder& encoder, const GrammarDetail& detail)
{
    encoder << detail.location;
    encoder << detail.length;
    encoder << detail.guesses;
    encoder << detail.userDescription;
}

std::optional<GrammarDetail> ArgumentCoder<GrammarDetail>::decode(Decoder& decoder)
{
    std::optional<int> location;
    decoder >> location;
    if (!location)
        return std::nullopt;

    std::optional<int> length;
    decoder >> length;
    if (!length)
        return std::nullopt;

    std::optional<Vector<String>> guesses;
    decoder >> guesses;
    if (!guesses)
        return std::nullopt;

    std::optional<String> userDescription;
    decoder >> userDescription;
    if (!userDescription)
        return std::nullopt;

    return {{ WTFMove(*location), WTFMove(*length), WTFMove(*guesses), WTFMove(*userDescription) }};
}

void ArgumentCoder<TextCheckingRequestData>::encode(Encoder& encoder, const TextCheckingRequestData& request)
{
    encoder << request.sequence();
    encoder << request.text();
    encoder << request.mask();
    encoder.encodeEnum(request.processType());
}

bool ArgumentCoder<TextCheckingRequestData>::decode(Decoder& decoder, TextCheckingRequestData& request)
{
    int sequence;
    if (!decoder.decode(sequence))
        return false;

    String text;
    if (!decoder.decode(text))
        return false;

    TextCheckingTypeMask mask;
    if (!decoder.decode(mask))
        return false;

    TextCheckingProcessType processType;
    if (!decoder.decodeEnum(processType))
        return false;

    request = TextCheckingRequestData(sequence, text, mask, processType);
    return true;
}

void ArgumentCoder<TextCheckingResult>::encode(Encoder& encoder, const TextCheckingResult& result)
{
    encoder.encodeEnum(result.type);
    encoder << result.location;
    encoder << result.length;
    encoder << result.details;
    encoder << result.replacement;
}

std::optional<TextCheckingResult> ArgumentCoder<TextCheckingResult>::decode(Decoder& decoder)
{
    TextCheckingType type;
    if (!decoder.decodeEnum(type))
        return std::nullopt;
    
    std::optional<int> location;
    decoder >> location;
    if (!location)
        return std::nullopt;

    std::optional<int> length;
    decoder >> length;
    if (!length)
        return std::nullopt;

    std::optional<Vector<GrammarDetail>> details;
    decoder >> details;
    if (!details)
        return std::nullopt;

    std::optional<String> replacement;
    decoder >> replacement;
    if (!replacement)
        return std::nullopt;

    return {{ WTFMove(type), WTFMove(*location), WTFMove(*length), WTFMove(*details), WTFMove(*replacement) }};
}

void ArgumentCoder<UserStyleSheet>::encode(Encoder& encoder, const UserStyleSheet& userStyleSheet)
{
    encoder << userStyleSheet.source();
    encoder << userStyleSheet.url();
    encoder << userStyleSheet.whitelist();
    encoder << userStyleSheet.blacklist();
    encoder.encodeEnum(userStyleSheet.injectedFrames());
    encoder.encodeEnum(userStyleSheet.level());
}

bool ArgumentCoder<UserStyleSheet>::decode(Decoder& decoder, UserStyleSheet& userStyleSheet)
{
    String source;
    if (!decoder.decode(source))
        return false;

    URL url;
    if (!decoder.decode(url))
        return false;

    Vector<String> whitelist;
    if (!decoder.decode(whitelist))
        return false;

    Vector<String> blacklist;
    if (!decoder.decode(blacklist))
        return false;

    UserContentInjectedFrames injectedFrames;
    if (!decoder.decodeEnum(injectedFrames))
        return false;

    UserStyleLevel level;
    if (!decoder.decodeEnum(level))
        return false;

    userStyleSheet = UserStyleSheet(source, url, WTFMove(whitelist), WTFMove(blacklist), injectedFrames, level);
    return true;
}

#if ENABLE(MEDIA_SESSION)
void ArgumentCoder<MediaSessionMetadata>::encode(Encoder& encoder, const MediaSessionMetadata& result)
{
    encoder << result.artist();
    encoder << result.album();
    encoder << result.title();
    encoder << result.artworkURL();
}

bool ArgumentCoder<MediaSessionMetadata>::decode(Decoder& decoder, MediaSessionMetadata& result)
{
    String artist, album, title;
    URL artworkURL;
    if (!decoder.decode(artist))
        return false;
    if (!decoder.decode(album))
        return false;
    if (!decoder.decode(title))
        return false;
    if (!decoder.decode(artworkURL))
        return false;
    result = MediaSessionMetadata(title, artist, album, artworkURL);
    return true;
}
#endif

void ArgumentCoder<ScrollableAreaParameters>::encode(Encoder& encoder, const ScrollableAreaParameters& parameters)
{
    encoder.encodeEnum(parameters.horizontalScrollElasticity);
    encoder.encodeEnum(parameters.verticalScrollElasticity);

    encoder.encodeEnum(parameters.horizontalScrollbarMode);
    encoder.encodeEnum(parameters.verticalScrollbarMode);

    encoder << parameters.hasEnabledHorizontalScrollbar;
    encoder << parameters.hasEnabledVerticalScrollbar;
}

bool ArgumentCoder<ScrollableAreaParameters>::decode(Decoder& decoder, ScrollableAreaParameters& params)
{
    if (!decoder.decodeEnum(params.horizontalScrollElasticity))
        return false;
    if (!decoder.decodeEnum(params.verticalScrollElasticity))
        return false;

    if (!decoder.decodeEnum(params.horizontalScrollbarMode))
        return false;
    if (!decoder.decodeEnum(params.verticalScrollbarMode))
        return false;

    if (!decoder.decode(params.hasEnabledHorizontalScrollbar))
        return false;
    if (!decoder.decode(params.hasEnabledVerticalScrollbar))
        return false;
    
    return true;
}

void ArgumentCoder<FixedPositionViewportConstraints>::encode(Encoder& encoder, const FixedPositionViewportConstraints& viewportConstraints)
{
    encoder << viewportConstraints.alignmentOffset();
    encoder << viewportConstraints.anchorEdges();

    encoder << viewportConstraints.viewportRectAtLastLayout();
    encoder << viewportConstraints.layerPositionAtLastLayout();
}

bool ArgumentCoder<FixedPositionViewportConstraints>::decode(Decoder& decoder, FixedPositionViewportConstraints& viewportConstraints)
{
    FloatSize alignmentOffset;
    if (!decoder.decode(alignmentOffset))
        return false;
    
    ViewportConstraints::AnchorEdges anchorEdges;
    if (!decoder.decode(anchorEdges))
        return false;

    FloatRect viewportRectAtLastLayout;
    if (!decoder.decode(viewportRectAtLastLayout))
        return false;

    FloatPoint layerPositionAtLastLayout;
    if (!decoder.decode(layerPositionAtLastLayout))
        return false;

    viewportConstraints = FixedPositionViewportConstraints();
    viewportConstraints.setAlignmentOffset(alignmentOffset);
    viewportConstraints.setAnchorEdges(anchorEdges);

    viewportConstraints.setViewportRectAtLastLayout(viewportRectAtLastLayout);
    viewportConstraints.setLayerPositionAtLastLayout(layerPositionAtLastLayout);
    
    return true;
}

void ArgumentCoder<StickyPositionViewportConstraints>::encode(Encoder& encoder, const StickyPositionViewportConstraints& viewportConstraints)
{
    encoder << viewportConstraints.alignmentOffset();
    encoder << viewportConstraints.anchorEdges();

    encoder << viewportConstraints.leftOffset();
    encoder << viewportConstraints.rightOffset();
    encoder << viewportConstraints.topOffset();
    encoder << viewportConstraints.bottomOffset();

    encoder << viewportConstraints.constrainingRectAtLastLayout();
    encoder << viewportConstraints.containingBlockRect();
    encoder << viewportConstraints.stickyBoxRect();

    encoder << viewportConstraints.stickyOffsetAtLastLayout();
    encoder << viewportConstraints.layerPositionAtLastLayout();
}

bool ArgumentCoder<StickyPositionViewportConstraints>::decode(Decoder& decoder, StickyPositionViewportConstraints& viewportConstraints)
{
    FloatSize alignmentOffset;
    if (!decoder.decode(alignmentOffset))
        return false;
    
    ViewportConstraints::AnchorEdges anchorEdges;
    if (!decoder.decode(anchorEdges))
        return false;
    
    float leftOffset;
    if (!decoder.decode(leftOffset))
        return false;

    float rightOffset;
    if (!decoder.decode(rightOffset))
        return false;

    float topOffset;
    if (!decoder.decode(topOffset))
        return false;

    float bottomOffset;
    if (!decoder.decode(bottomOffset))
        return false;
    
    FloatRect constrainingRectAtLastLayout;
    if (!decoder.decode(constrainingRectAtLastLayout))
        return false;

    FloatRect containingBlockRect;
    if (!decoder.decode(containingBlockRect))
        return false;

    FloatRect stickyBoxRect;
    if (!decoder.decode(stickyBoxRect))
        return false;

    FloatSize stickyOffsetAtLastLayout;
    if (!decoder.decode(stickyOffsetAtLastLayout))
        return false;
    
    FloatPoint layerPositionAtLastLayout;
    if (!decoder.decode(layerPositionAtLastLayout))
        return false;
    
    viewportConstraints = StickyPositionViewportConstraints();
    viewportConstraints.setAlignmentOffset(alignmentOffset);
    viewportConstraints.setAnchorEdges(anchorEdges);

    viewportConstraints.setLeftOffset(leftOffset);
    viewportConstraints.setRightOffset(rightOffset);
    viewportConstraints.setTopOffset(topOffset);
    viewportConstraints.setBottomOffset(bottomOffset);
    
    viewportConstraints.setConstrainingRectAtLastLayout(constrainingRectAtLastLayout);
    viewportConstraints.setContainingBlockRect(containingBlockRect);
    viewportConstraints.setStickyBoxRect(stickyBoxRect);

    viewportConstraints.setStickyOffsetAtLastLayout(stickyOffsetAtLastLayout);
    viewportConstraints.setLayerPositionAtLastLayout(layerPositionAtLastLayout);

    return true;
}

#if !USE(COORDINATED_GRAPHICS)
void ArgumentCoder<FilterOperation>::encode(Encoder& encoder, const FilterOperation& filter)
{
    encoder.encodeEnum(filter.type());

    switch (filter.type()) {
    case FilterOperation::NONE:
    case FilterOperation::REFERENCE:
        ASSERT_NOT_REACHED();
        break;
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE:
        encoder << downcast<BasicColorMatrixFilterOperation>(filter).amount();
        break;
    case FilterOperation::INVERT:
    case FilterOperation::OPACITY:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST:
        encoder << downcast<BasicComponentTransferFilterOperation>(filter).amount();
        break;
    case FilterOperation::BLUR:
        encoder << downcast<BlurFilterOperation>(filter).stdDeviation();
        break;
    case FilterOperation::DROP_SHADOW: {
        const auto& dropShadowFilter = downcast<DropShadowFilterOperation>(filter);
        encoder << dropShadowFilter.location();
        encoder << dropShadowFilter.stdDeviation();
        encoder << dropShadowFilter.color();
        break;
    }
    case FilterOperation::DEFAULT:
        encoder.encodeEnum(downcast<DefaultFilterOperation>(filter).representedType());
        break;
    case FilterOperation::PASSTHROUGH:
        break;
    }
}

bool decodeFilterOperation(Decoder& decoder, RefPtr<FilterOperation>& filter)
{
    FilterOperation::OperationType type;
    if (!decoder.decodeEnum(type))
        return false;

    switch (type) {
    case FilterOperation::NONE:
    case FilterOperation::REFERENCE:
        ASSERT_NOT_REACHED();
        decoder.markInvalid();
        return false;
    case FilterOperation::GRAYSCALE:
    case FilterOperation::SEPIA:
    case FilterOperation::SATURATE:
    case FilterOperation::HUE_ROTATE: {
        double amount;
        if (!decoder.decode(amount))
            return false;
        filter = BasicColorMatrixFilterOperation::create(amount, type);
        break;
    }
    case FilterOperation::INVERT:
    case FilterOperation::OPACITY:
    case FilterOperation::BRIGHTNESS:
    case FilterOperation::CONTRAST: {
        double amount;
        if (!decoder.decode(amount))
            return false;
        filter = BasicComponentTransferFilterOperation::create(amount, type);
        break;
    }
    case FilterOperation::BLUR: {
        Length stdDeviation;
        if (!decoder.decode(stdDeviation))
            return false;
        filter = BlurFilterOperation::create(stdDeviation);
        break;
    }
    case FilterOperation::DROP_SHADOW: {
        IntPoint location;
        int stdDeviation;
        Color color;
        if (!decoder.decode(location))
            return false;
        if (!decoder.decode(stdDeviation))
            return false;
        if (!decoder.decode(color))
            return false;
        filter = DropShadowFilterOperation::create(location, stdDeviation, color);
        break;
    }
    case FilterOperation::DEFAULT: {
        FilterOperation::OperationType representedType;
        if (!decoder.decodeEnum(representedType))
            return false;
        filter = DefaultFilterOperation::create(representedType);
        break;
    }
    case FilterOperation::PASSTHROUGH:
        filter = PassthroughFilterOperation::create();
        break;
    }
            
    return true;
}


void ArgumentCoder<FilterOperations>::encode(Encoder& encoder, const FilterOperations& filters)
{
    encoder << static_cast<uint64_t>(filters.size());

    for (const auto& filter : filters.operations())
        encoder << *filter;
}

bool ArgumentCoder<FilterOperations>::decode(Decoder& decoder, FilterOperations& filters)
{
    uint64_t filterCount;
    if (!decoder.decode(filterCount))
        return false;

    for (uint64_t i = 0; i < filterCount; ++i) {
        RefPtr<FilterOperation> filter;
        if (!decodeFilterOperation(decoder, filter))
            return false;
        filters.operations().append(WTFMove(filter));
    }

    return true;
}
#endif // !USE(COORDINATED_GRAPHICS)

void ArgumentCoder<BlobPart>::encode(Encoder& encoder, const BlobPart& blobPart)
{
    encoder << static_cast<uint32_t>(blobPart.type());
    switch (blobPart.type()) {
    case BlobPart::Data:
        encoder << blobPart.data();
        break;
    case BlobPart::Blob:
        encoder << blobPart.url();
        break;
    }
}

std::optional<BlobPart> ArgumentCoder<BlobPart>::decode(Decoder& decoder)
{
    BlobPart blobPart;

    std::optional<uint32_t> type;
    decoder >> type;
    if (!type)
        return std::nullopt;

    switch (*type) {
    case BlobPart::Data: {
        std::optional<Vector<uint8_t>> data;
        decoder >> data;
        if (!data)
            return std::nullopt;
        blobPart = BlobPart(WTFMove(*data));
        break;
    }
    case BlobPart::Blob: {
        URL url;
        if (!decoder.decode(url))
            return std::nullopt;
        blobPart = BlobPart(url);
        break;
    }
    default:
        return std::nullopt;
    }

    return WTFMove(blobPart);
}

void ArgumentCoder<TextIndicatorData>::encode(Encoder& encoder, const TextIndicatorData& textIndicatorData)
{
    encoder << textIndicatorData.selectionRectInRootViewCoordinates;
    encoder << textIndicatorData.textBoundingRectInRootViewCoordinates;
    encoder << textIndicatorData.textRectsInBoundingRectCoordinates;
    encoder << textIndicatorData.contentImageWithoutSelectionRectInRootViewCoordinates;
    encoder << textIndicatorData.contentImageScaleFactor;
    encoder << textIndicatorData.estimatedBackgroundColor;
    encoder.encodeEnum(textIndicatorData.presentationTransition);
    encoder << static_cast<uint64_t>(textIndicatorData.options);

    encodeOptionalImage(encoder, textIndicatorData.contentImage.get());
    encodeOptionalImage(encoder, textIndicatorData.contentImageWithHighlight.get());
    encodeOptionalImage(encoder, textIndicatorData.contentImageWithoutSelection.get());
}

std::optional<TextIndicatorData> ArgumentCoder<TextIndicatorData>::decode(Decoder& decoder)
{
    TextIndicatorData textIndicatorData;
    if (!decoder.decode(textIndicatorData.selectionRectInRootViewCoordinates))
        return std::nullopt;

    if (!decoder.decode(textIndicatorData.textBoundingRectInRootViewCoordinates))
        return std::nullopt;

    if (!decoder.decode(textIndicatorData.textRectsInBoundingRectCoordinates))
        return std::nullopt;

    if (!decoder.decode(textIndicatorData.contentImageWithoutSelectionRectInRootViewCoordinates))
        return std::nullopt;

    if (!decoder.decode(textIndicatorData.contentImageScaleFactor))
        return std::nullopt;

    if (!decoder.decode(textIndicatorData.estimatedBackgroundColor))
        return std::nullopt;

    if (!decoder.decodeEnum(textIndicatorData.presentationTransition))
        return std::nullopt;

    uint64_t options;
    if (!decoder.decode(options))
        return std::nullopt;
    textIndicatorData.options = static_cast<TextIndicatorOptions>(options);

    if (!decodeOptionalImage(decoder, textIndicatorData.contentImage))
        return std::nullopt;

    if (!decodeOptionalImage(decoder, textIndicatorData.contentImageWithHighlight))
        return std::nullopt;

    if (!decodeOptionalImage(decoder, textIndicatorData.contentImageWithoutSelection))
        return std::nullopt;

    return WTFMove(textIndicatorData);
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void ArgumentCoder<MediaPlaybackTargetContext>::encode(Encoder& encoder, const MediaPlaybackTargetContext& target)
{
    bool hasPlatformData = target.encodingRequiresPlatformData();
    encoder << hasPlatformData;

    int32_t targetType = target.type();
    encoder << targetType;

    if (target.encodingRequiresPlatformData()) {
        encodePlatformData(encoder, target);
        return;
    }

    ASSERT(targetType == MediaPlaybackTargetContext::MockType);
    encoder << target.mockDeviceName();
    encoder << static_cast<int32_t>(target.mockState());
}

bool ArgumentCoder<MediaPlaybackTargetContext>::decode(Decoder& decoder, MediaPlaybackTargetContext& target)
{
    bool hasPlatformData;
    if (!decoder.decode(hasPlatformData))
        return false;

    int32_t targetType;
    if (!decoder.decode(targetType))
        return false;

    if (hasPlatformData)
        return decodePlatformData(decoder, target);

    ASSERT(targetType == MediaPlaybackTargetContext::MockType);

    String mockDeviceName;
    if (!decoder.decode(mockDeviceName))
        return false;

    int32_t mockState;
    if (!decoder.decode(mockState))
        return false;

    target = MediaPlaybackTargetContext(mockDeviceName, static_cast<MediaPlaybackTargetContext::State>(mockState));
    return true;
}
#endif

void ArgumentCoder<DictionaryPopupInfo>::encode(IPC::Encoder& encoder, const DictionaryPopupInfo& info)
{
    encoder << info.origin;
    encoder << info.textIndicator;

#if PLATFORM(COCOA)
    bool hadOptions = info.options;
    encoder << hadOptions;
    if (hadOptions)
        IPC::encode(encoder, info.options.get());

    bool hadAttributedString = info.attributedString;
    encoder << hadAttributedString;
    if (hadAttributedString)
        IPC::encode(encoder, info.attributedString.get());
#endif
}

bool ArgumentCoder<DictionaryPopupInfo>::decode(IPC::Decoder& decoder, DictionaryPopupInfo& result)
{
    if (!decoder.decode(result.origin))
        return false;

    std::optional<TextIndicatorData> textIndicator;
    decoder >> textIndicator;
    if (!textIndicator)
        return false;
    result.textIndicator = WTFMove(*textIndicator);

#if PLATFORM(COCOA)
    bool hadOptions;
    if (!decoder.decode(hadOptions))
        return false;
    if (hadOptions) {
        if (!IPC::decode(decoder, result.options))
            return false;
    } else
        result.options = nullptr;

    bool hadAttributedString;
    if (!decoder.decode(hadAttributedString))
        return false;
    if (hadAttributedString) {
        if (!IPC::decode(decoder, result.attributedString))
            return false;
    } else
        result.attributedString = nullptr;
#endif
    return true;
}

void ArgumentCoder<ExceptionDetails>::encode(IPC::Encoder& encoder, const ExceptionDetails& info)
{
    encoder << info.message;
    encoder << info.lineNumber;
    encoder << info.columnNumber;
    encoder << info.sourceURL;
}

bool ArgumentCoder<ExceptionDetails>::decode(IPC::Decoder& decoder, ExceptionDetails& result)
{
    if (!decoder.decode(result.message))
        return false;

    if (!decoder.decode(result.lineNumber))
        return false;

    if (!decoder.decode(result.columnNumber))
        return false;

    if (!decoder.decode(result.sourceURL))
        return false;

    return true;
}

void ArgumentCoder<ResourceLoadStatistics>::encode(Encoder& encoder, const WebCore::ResourceLoadStatistics& statistics)
{
    encoder << statistics.highLevelDomain;
    
    encoder << statistics.lastSeen.secondsSinceEpoch().value();
    
    // User interaction
    encoder << statistics.hadUserInteraction;
    encoder << statistics.mostRecentUserInteractionTime.secondsSinceEpoch().value();
    encoder << statistics.grandfathered;

    // Storage access
    encoder << statistics.storageAccessUnderTopFrameOrigins;

    // Top frame stats
    encoder << statistics.topFrameUniqueRedirectsTo;
    encoder << statistics.topFrameUniqueRedirectsFrom;

    // Subframe stats
    encoder << statistics.subframeUnderTopFrameOrigins;
    
    // Subresource stats
    encoder << statistics.subresourceUnderTopFrameOrigins;
    encoder << statistics.subresourceUniqueRedirectsTo;
    encoder << statistics.subresourceUniqueRedirectsFrom;

    // Prevalent Resource
    encoder << statistics.isPrevalentResource;
    encoder << statistics.dataRecordsRemoved;
}

std::optional<ResourceLoadStatistics> ArgumentCoder<ResourceLoadStatistics>::decode(Decoder& decoder)
{
    ResourceLoadStatistics statistics;
    if (!decoder.decode(statistics.highLevelDomain))
        return std::nullopt;
    
    double lastSeenTimeAsDouble;
    if (!decoder.decode(lastSeenTimeAsDouble))
        return std::nullopt;
    statistics.lastSeen = WallTime::fromRawSeconds(lastSeenTimeAsDouble);
    
    // User interaction
    if (!decoder.decode(statistics.hadUserInteraction))
        return std::nullopt;

    double mostRecentUserInteractionTimeAsDouble;
    if (!decoder.decode(mostRecentUserInteractionTimeAsDouble))
        return std::nullopt;
    statistics.mostRecentUserInteractionTime = WallTime::fromRawSeconds(mostRecentUserInteractionTimeAsDouble);

    if (!decoder.decode(statistics.grandfathered))
        return std::nullopt;

    // Storage access
    if (!decoder.decode(statistics.storageAccessUnderTopFrameOrigins))
        return std::nullopt;

    // Top frame stats
    if (!decoder.decode(statistics.topFrameUniqueRedirectsTo))
        return std::nullopt;    

    if (!decoder.decode(statistics.topFrameUniqueRedirectsFrom))
        return std::nullopt;

    // Subframe stats
    if (!decoder.decode(statistics.subframeUnderTopFrameOrigins))
        return std::nullopt;
    
    // Subresource stats
    if (!decoder.decode(statistics.subresourceUnderTopFrameOrigins))
        return std::nullopt;

    if (!decoder.decode(statistics.subresourceUniqueRedirectsTo))
        return std::nullopt;
    
    if (!decoder.decode(statistics.subresourceUniqueRedirectsFrom))
        return std::nullopt;
    
    // Prevalent Resource
    if (!decoder.decode(statistics.isPrevalentResource))
        return std::nullopt;

    if (!decoder.decode(statistics.dataRecordsRemoved))
        return std::nullopt;

    return WTFMove(statistics);
}

#if ENABLE(MEDIA_STREAM)
void ArgumentCoder<MediaConstraints>::encode(Encoder& encoder, const WebCore::MediaConstraints& constraint)
{
    encoder << constraint.mandatoryConstraints
        << constraint.advancedConstraints
        << constraint.deviceIDHashSalt
        << constraint.isValid;
}

bool ArgumentCoder<MediaConstraints>::decode(Decoder& decoder, WebCore::MediaConstraints& constraints)
{
    std::optional<WebCore::MediaTrackConstraintSetMap> mandatoryConstraints;
    decoder >> mandatoryConstraints;
    if (!mandatoryConstraints)
        return false;
    constraints.mandatoryConstraints = WTFMove(*mandatoryConstraints);
    return decoder.decode(constraints.advancedConstraints)
        && decoder.decode(constraints.deviceIDHashSalt)
        && decoder.decode(constraints.isValid);
}
#endif

#if ENABLE(INDEXED_DATABASE)
void ArgumentCoder<IDBKeyPath>::encode(Encoder& encoder, const IDBKeyPath& keyPath)
{
    bool isString = WTF::holds_alternative<String>(keyPath);
    encoder << isString;
    if (isString)
        encoder << WTF::get<String>(keyPath);
    else
        encoder << WTF::get<Vector<String>>(keyPath);
}

bool ArgumentCoder<IDBKeyPath>::decode(Decoder& decoder, IDBKeyPath& keyPath)
{
    bool isString;
    if (!decoder.decode(isString))
        return false;
    if (isString) {
        String string;
        if (!decoder.decode(string))
            return false;
        keyPath = string;
    } else {
        Vector<String> vector;
        if (!decoder.decode(vector))
            return false;
        keyPath = vector;
    }
    return true;
}
#endif

#if ENABLE(SERVICE_WORKER)
void ArgumentCoder<ServiceWorkerOrClientData>::encode(Encoder& encoder, const ServiceWorkerOrClientData& data)
{
    bool isServiceWorkerData = WTF::holds_alternative<ServiceWorkerData>(data);
    encoder << isServiceWorkerData;
    if (isServiceWorkerData)
        encoder << WTF::get<ServiceWorkerData>(data);
    else
        encoder << WTF::get<ServiceWorkerClientData>(data);
}

bool ArgumentCoder<ServiceWorkerOrClientData>::decode(Decoder& decoder, ServiceWorkerOrClientData& data)
{
    bool isServiceWorkerData;
    if (!decoder.decode(isServiceWorkerData))
        return false;
    if (isServiceWorkerData) {
        std::optional<ServiceWorkerData> workerData;
        decoder >> workerData;
        if (!workerData)
            return false;

        data = WTFMove(*workerData);
    } else {
        std::optional<ServiceWorkerClientData> clientData;
        decoder >> clientData;
        if (!clientData)
            return false;

        data = WTFMove(*clientData);
    }
    return true;
}

void ArgumentCoder<ServiceWorkerOrClientIdentifier>::encode(Encoder& encoder, const ServiceWorkerOrClientIdentifier& identifier)
{
    bool isServiceWorkerIdentifier = WTF::holds_alternative<ServiceWorkerIdentifier>(identifier);
    encoder << isServiceWorkerIdentifier;
    if (isServiceWorkerIdentifier)
        encoder << WTF::get<ServiceWorkerIdentifier>(identifier);
    else
        encoder << WTF::get<ServiceWorkerClientIdentifier>(identifier);
}

bool ArgumentCoder<ServiceWorkerOrClientIdentifier>::decode(Decoder& decoder, ServiceWorkerOrClientIdentifier& identifier)
{
    bool isServiceWorkerIdentifier;
    if (!decoder.decode(isServiceWorkerIdentifier))
        return false;
    if (isServiceWorkerIdentifier) {
        std::optional<ServiceWorkerIdentifier> workerIdentifier;
        decoder >> workerIdentifier;
        if (!workerIdentifier)
            return false;

        identifier = WTFMove(*workerIdentifier);
    } else {
        std::optional<ServiceWorkerClientIdentifier> clientIdentifier;
        decoder >> clientIdentifier;
        if (!clientIdentifier)
            return false;

        identifier = WTFMove(*clientIdentifier);
    }
    return true;
}
#endif

#if ENABLE(CSS_SCROLL_SNAP)

void ArgumentCoder<ScrollOffsetRange<float>>::encode(Encoder& encoder, const ScrollOffsetRange<float>& range)
{
    encoder << range.start;
    encoder << range.end;
}

auto ArgumentCoder<ScrollOffsetRange<float>>::decode(Decoder& decoder) -> std::optional<WebCore::ScrollOffsetRange<float>>
{
    WebCore::ScrollOffsetRange<float> range;
    float start;
    if (!decoder.decode(start))
        return std::nullopt;

    float end;
    if (!decoder.decode(end))
        return std::nullopt;

    range.start = start;
    range.end = end;
    return WTFMove(range);
}

#endif

void ArgumentCoder<MediaSelectionOption>::encode(Encoder& encoder, const MediaSelectionOption& option)
{
    encoder << option.displayName;
    encoder << option.type;
}

std::optional<MediaSelectionOption> ArgumentCoder<MediaSelectionOption>::decode(Decoder& decoder)
{
    std::optional<String> displayName;
    decoder >> displayName;
    if (!displayName)
        return std::nullopt;
    
    std::optional<MediaSelectionOption::Type> type;
    decoder >> type;
    if (!type)
        return std::nullopt;
    
    return {{ WTFMove(*displayName), WTFMove(*type) }};
}

void ArgumentCoder<PromisedBlobInfo>::encode(Encoder& encoder, const PromisedBlobInfo& info)
{
    encoder << info.blobURL;
    encoder << info.contentType;
    encoder << info.filename;
    encodeTypesAndData(encoder, info.additionalTypes, info.additionalData);
}

bool ArgumentCoder<PromisedBlobInfo>::decode(Decoder& decoder, PromisedBlobInfo& info)
{
    if (!decoder.decode(info.blobURL))
        return false;

    if (!decoder.decode(info.contentType))
        return false;

    if (!decoder.decode(info.filename))
        return false;

    if (!decodeTypesAndData(decoder, info.additionalTypes, info.additionalData))
        return false;

    return true;
}

#if ENABLE(ATTACHMENT_ELEMENT)

void ArgumentCoder<AttachmentInfo>::encode(Encoder& encoder, const AttachmentInfo& info)
{
    bool dataIsNull = !info.data;
    encoder << info.contentType << info.name << info.filePath << dataIsNull;
    if (!dataIsNull) {
        SharedBufferDataReference dataReference { info.data.get() };
        encoder << static_cast<DataReference&>(dataReference);
    }
}

bool ArgumentCoder<AttachmentInfo>::decode(Decoder& decoder, AttachmentInfo& info)
{
    if (!decoder.decode(info.contentType))
        return false;

    if (!decoder.decode(info.name))
        return false;

    if (!decoder.decode(info.filePath))
        return false;

    bool dataIsNull = true;
    if (!decoder.decode(dataIsNull))
        return false;

    if (!dataIsNull) {
        DataReference dataReference;
        if (!decoder.decode(dataReference))
            return false;
        info.data = SharedBuffer::create(dataReference.data(), dataReference.size());
    }

    return true;
}

#endif // ENABLE(ATTACHMENT_ELEMENT)

} // namespace IPC
