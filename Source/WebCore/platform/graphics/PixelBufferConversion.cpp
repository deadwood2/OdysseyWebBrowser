/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
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
#include "PixelBufferConversion.h"

#include "AlphaPremultiplication.h"
#include "DestinationColorSpace.h"
#include "IntSize.h"
#include "PixelFormat.h"

#if USE(ACCELERATE) && USE(CG)
#include <Accelerate/Accelerate.h>
#endif

namespace WebCore {

#if USE(ACCELERATE) && USE(CG)

static inline vImage_CGImageFormat makeVImageCGImageFormat(const PixelBufferFormat& format)
{
    auto [bitsPerComponent, bitsPerPixel, bitmapInfo] = [] (const PixelBufferFormat& format) -> std::tuple<unsigned, unsigned, CGBitmapInfo> {
        switch (format.pixelFormat) {
        case PixelFormat::RGBA8:
            if (format.alphaFormat == AlphaPremultiplication::Premultiplied)
                return std::make_tuple(8u, 32u, kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedLast);
            else
                return std::make_tuple(8u, 32u, kCGBitmapByteOrder32Big | kCGImageAlphaLast);

        case PixelFormat::BGRA8:
            if (format.alphaFormat == AlphaPremultiplication::Premultiplied)
                return std::make_tuple(8u, 32u, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
            else
                return std::make_tuple(8u, 32u, kCGBitmapByteOrder32Little | kCGImageAlphaFirst);

        case PixelFormat::RGB10:
        case PixelFormat::RGB10A8:
            break;
        }

        // We currently only support 8 bit pixel formats for these conversions.

        ASSERT_NOT_REACHED();
        return std::make_tuple(8u, 32u, kCGBitmapByteOrder32Little | kCGImageAlphaFirst);
    }(format);

    vImage_CGImageFormat result;

    result.bitsPerComponent = bitsPerComponent;
    result.bitsPerPixel = bitsPerPixel;
    result.colorSpace = format.colorSpace.platformColorSpace();
    result.bitmapInfo = bitmapInfo;
    result.version = 0;
    result.decode = nullptr;
    result.renderingIntent = kCGRenderingIntentDefault;

    return result;
}

template<typename View> static vImage_Buffer makeVImageBuffer(const View& view, const IntSize& size)
{
    vImage_Buffer result;

    result.height = static_cast<vImagePixelCount>(size.height());
    result.width = static_cast<vImagePixelCount>(size.width());
    result.rowBytes = view.bytesPerRow;
    result.data = const_cast<uint8_t*>(view.rows);

    return result;
}

static void convertImagePixelsAccelerated(const ConstPixelBufferConversionView& source, const PixelBufferConversionView& destination, const IntSize& destinationSize)
{
    auto sourceVImageBuffer = makeVImageBuffer(source, destinationSize);
    auto destinationVImageBuffer = makeVImageBuffer(destination, destinationSize);

    if (source.format.colorSpace != destination.format.colorSpace) {
        // FIXME: Consider using vImageConvert_AnyToAny for all conversions, not just ones that need a color space conversion,
        // after judiciously performance testing them against each other.

        auto sourceCGImageFormat = makeVImageCGImageFormat(source.format);
        auto destinationCGImageFormat = makeVImageCGImageFormat(destination.format);

        vImage_Error converterCreateError = kvImageNoError;
        auto converter = adoptCF(vImageConverter_CreateWithCGImageFormat(&sourceCGImageFormat, &destinationCGImageFormat, nullptr, kvImageNoFlags, &converterCreateError));
        ASSERT_WITH_MESSAGE_UNUSED(converterCreateError, converterCreateError == kvImageNoError, "vImageConverter creation failed with error: %zd", converterCreateError);

        vImage_Error converterConvertError = vImageConvert_AnyToAny(converter.get(), &sourceVImageBuffer, &destinationVImageBuffer, nullptr, kvImageNoFlags);
        ASSERT_WITH_MESSAGE_UNUSED(converterConvertError, converterConvertError == kvImageNoError, "vImageConvert_AnyToAny failed conversion with error: %zd", converterConvertError);
        return;
    }

    if (source.format.alphaFormat != destination.format.alphaFormat) {
        if (destination.format.alphaFormat == AlphaPremultiplication::Unpremultiplied) {
            if (source.format.pixelFormat == PixelFormat::RGBA8)
                vImageUnpremultiplyData_RGBA8888(&sourceVImageBuffer, &destinationVImageBuffer, kvImageNoFlags);
            else
                vImageUnpremultiplyData_BGRA8888(&sourceVImageBuffer, &destinationVImageBuffer, kvImageNoFlags);
        } else {
            if (source.format.pixelFormat == PixelFormat::RGBA8)
                vImagePremultiplyData_RGBA8888(&sourceVImageBuffer, &destinationVImageBuffer, kvImageNoFlags);
            else
                vImagePremultiplyData_BGRA8888(&sourceVImageBuffer, &destinationVImageBuffer, kvImageNoFlags);
        }

        sourceVImageBuffer = destinationVImageBuffer;
    }

    if (source.format.pixelFormat != destination.format.pixelFormat) {
        // Swap pixel channels BGRA <-> RGBA.
        const uint8_t map[4] = { 2, 1, 0, 3 };
        vImagePermuteChannels_ARGB8888(&sourceVImageBuffer, &destinationVImageBuffer, map, kvImageNoFlags);
    }
}

#endif

enum class PixelFormatConversion { None, Permute };

template<PixelFormatConversion pixelFormatConversion>
#if CPU(BIG_ENDIAN)
static void convertSinglePixelPremultipliedToPremultiplied(PixelFormat sourcePixelFormat, const uint8_t* sourcePixel, PixelFormat destinationPixelFormat, uint8_t* destinationPixel)
#else
static void convertSinglePixelPremultipliedToPremultiplied(const uint8_t* sourcePixel, uint8_t* destinationPixel)
#endif
{
#if CPU(BIG_ENDIAN)
    uint8_t alpha = sourcePixel[sourcePixelFormat == PixelFormat::ARGB8 ? 0 : 3];
#else
    uint8_t alpha = sourcePixel[3];
#endif
    if (!alpha) {
        reinterpret_cast<uint32_t*>(destinationPixel)[0] = 0;
        return;
    }

    if constexpr (pixelFormatConversion == PixelFormatConversion::None)
        reinterpret_cast<uint32_t*>(destinationPixel)[0] = reinterpret_cast<const uint32_t*>(sourcePixel)[0];
    else {
#if CPU(BIG_ENDIAN)
        // Swap pixel channels ARGB <-> RGBA.
        if (destinationPixelFormat == PixelFormat::ARGB8)
        {
            destinationPixel[0] = sourcePixel[3];
            destinationPixel[1] = sourcePixel[0];
            destinationPixel[2] = sourcePixel[1];
            destinationPixel[3] = sourcePixel[2];
        }
        else
        {
            destinationPixel[0] = sourcePixel[1];
            destinationPixel[1] = sourcePixel[2];
            destinationPixel[2] = sourcePixel[3];
            destinationPixel[3] = sourcePixel[0];
        }
#else
        // Swap pixel channels BGRA <-> RGBA.
        destinationPixel[0] = sourcePixel[2];
        destinationPixel[1] = sourcePixel[1];
        destinationPixel[2] = sourcePixel[0];
        destinationPixel[3] = sourcePixel[3];
#endif
    }
}

template<PixelFormatConversion pixelFormatConversion>
#if CPU(BIG_ENDIAN)
static void convertSinglePixelPremultipliedToUnpremultiplied(PixelFormat sourcePixelFormat, const uint8_t* sourcePixel, PixelFormat destinationPixelFormat, uint8_t* destinationPixel)
#else
static void convertSinglePixelPremultipliedToUnpremultiplied(const uint8_t* sourcePixel, uint8_t* destinationPixel)
#endif
{
#if CPU(BIG_ENDIAN)
    uint8_t alpha = sourcePixel[sourcePixelFormat == PixelFormat::ARGB8 ? 0 : 3];
#else
    uint8_t alpha = sourcePixel[3];
#endif
    if (!alpha || alpha == 255) {
#if CPU(BIG_ENDIAN)
        convertSinglePixelPremultipliedToPremultiplied<pixelFormatConversion>(sourcePixelFormat, sourcePixel, destinationPixelFormat, destinationPixel);
#else
        convertSinglePixelPremultipliedToPremultiplied<pixelFormatConversion>(sourcePixel, destinationPixel);
#endif
        return;
    }

#if CPU(BIG_ENDIAN)
    UNUSED_PARAM(destinationPixelFormat);
    if constexpr (pixelFormatConversion == PixelFormatConversion::None) {
        if (sourcePixelFormat == PixelFormat::ARGB8) {
            destinationPixel[0] = alpha;
            destinationPixel[1] = (sourcePixel[1] * 255) / alpha;
            destinationPixel[2] = (sourcePixel[2] * 255) / alpha;
            destinationPixel[3] = (sourcePixel[3] * 255) / alpha;
        } else {
            destinationPixel[0] = (sourcePixel[0] * 255) / alpha;
            destinationPixel[1] = (sourcePixel[1] * 255) / alpha;
            destinationPixel[2] = (sourcePixel[2] * 255) / alpha;
            destinationPixel[3] = alpha;
        }
    } else {
        if (sourcePixelFormat == PixelFormat::ARGB8) {
            destinationPixel[0] = (sourcePixel[1] * 255) / alpha;
            destinationPixel[1] = (sourcePixel[2] * 255) / alpha;
            destinationPixel[2] = (sourcePixel[3] * 255) / alpha;
            destinationPixel[3] = alpha;
        } else {
            destinationPixel[0] = alpha;
            destinationPixel[1] = (sourcePixel[0] * 255) / alpha;
            destinationPixel[2] = (sourcePixel[1] * 255) / alpha;
            destinationPixel[3] = (sourcePixel[2] * 255) / alpha;
        }
    }
#else
    if constexpr (pixelFormatConversion == PixelFormatConversion::None) {
        destinationPixel[0] = (sourcePixel[0] * 255) / alpha;
        destinationPixel[1] = (sourcePixel[1] * 255) / alpha;
        destinationPixel[2] = (sourcePixel[2] * 255) / alpha;
        destinationPixel[3] = alpha;
    } else {
        // Swap pixel channels BGRA <-> RGBA.
        destinationPixel[0] = (sourcePixel[2] * 255) / alpha;
        destinationPixel[1] = (sourcePixel[1] * 255) / alpha;
        destinationPixel[2] = (sourcePixel[0] * 255) / alpha;
        destinationPixel[3] = alpha;
    }
#endif
}

template<PixelFormatConversion pixelFormatConversion>
#if CPU(BIG_ENDIAN)
static void convertSinglePixelUnpremultipliedToPremultiplied(PixelFormat sourcePixelFormat, const uint8_t* sourcePixel, PixelFormat destinationPixelFormat, uint8_t* destinationPixel)
#else
static void convertSinglePixelUnpremultipliedToPremultiplied(const uint8_t* sourcePixel, uint8_t* destinationPixel)
#endif
{
#if CPU(BIG_ENDIAN)
    uint8_t alpha = sourcePixel[sourcePixelFormat == PixelFormat::ARGB8 ? 0 : 3];
#else
    uint8_t alpha = sourcePixel[3];
#endif
    if (!alpha || alpha == 255) {
#if CPU(BIG_ENDIAN)
        convertSinglePixelPremultipliedToPremultiplied<pixelFormatConversion>(sourcePixelFormat, sourcePixel, destinationPixelFormat, destinationPixel);
#else
        convertSinglePixelPremultipliedToPremultiplied<pixelFormatConversion>(sourcePixel, destinationPixel);
#endif
        return;
    }

#if CPU(BIG_ENDIAN)
    UNUSED_PARAM(destinationPixelFormat);
    if constexpr (pixelFormatConversion == PixelFormatConversion::None) {
        if (sourcePixelFormat == PixelFormat::ARGB8) {
            destinationPixel[0] = alpha;
            destinationPixel[1] = (sourcePixel[1] * alpha + 254) / 255;
            destinationPixel[2] = (sourcePixel[2] * alpha + 254) / 255;
            destinationPixel[3] = (sourcePixel[3] * alpha + 254) / 255;
        } else {
            destinationPixel[0] = (sourcePixel[0] * alpha + 254) / 255;
            destinationPixel[1] = (sourcePixel[1] * alpha + 254) / 255;
            destinationPixel[2] = (sourcePixel[2] * alpha + 254) / 255;
            destinationPixel[3] = alpha;
        }
    } else {
        if (sourcePixelFormat == PixelFormat::ARGB8) {
            destinationPixel[0] = (sourcePixel[1] * alpha + 254) / 255;
            destinationPixel[1] = (sourcePixel[2] * alpha + 254) / 255;
            destinationPixel[2] = (sourcePixel[3] * alpha + 254) / 255;
            destinationPixel[3] = alpha;
        } else {
            destinationPixel[0] = alpha;
            destinationPixel[1] = (sourcePixel[0] * alpha + 254) / 255;
            destinationPixel[2] = (sourcePixel[1] * alpha + 254) / 255;
            destinationPixel[3] = (sourcePixel[2] * alpha + 254) / 255;
        }
    }
#else
    if constexpr (pixelFormatConversion == PixelFormatConversion::None) {
        destinationPixel[0] = (sourcePixel[0] * alpha + 254) / 255;
        destinationPixel[1] = (sourcePixel[1] * alpha + 254) / 255;
        destinationPixel[2] = (sourcePixel[2] * alpha + 254) / 255;
        destinationPixel[3] = alpha;
    } else {
        // Swap pixel channels BGRA <-> RGBA.
        destinationPixel[0] = (sourcePixel[2] * alpha + 254) / 255;
        destinationPixel[1] = (sourcePixel[1] * alpha + 254) / 255;
        destinationPixel[2] = (sourcePixel[0] * alpha + 254) / 255;
        destinationPixel[3] = alpha;
    }
#endif
}

template<PixelFormatConversion pixelFormatConversion>
#if CPU(BIG_ENDIAN)
static void convertSinglePixelUnpremultipliedToUnpremultiplied(PixelFormat sourcePixelFormat, const uint8_t* sourcePixel, PixelFormat destinationPixelFormat, uint8_t* destinationPixel)
#else
static void convertSinglePixelUnpremultipliedToUnpremultiplied(const uint8_t* sourcePixel, uint8_t* destinationPixel)
#endif
{
    if constexpr (pixelFormatConversion == PixelFormatConversion::None)
        reinterpret_cast<uint32_t*>(destinationPixel)[0] = reinterpret_cast<const uint32_t*>(sourcePixel)[0];
    else {
#if CPU(BIG_ENDIAN)
        UNUSED_PARAM(sourcePixelFormat);
        // Swap pixel channels ARGB <-> RGBA.
        if (destinationPixelFormat == PixelFormat::ARGB8) {
            destinationPixel[0] = sourcePixel[3];
            destinationPixel[1] = sourcePixel[0];
            destinationPixel[2] = sourcePixel[1];
            destinationPixel[3] = sourcePixel[2];
        }
        else {
            destinationPixel[0] = sourcePixel[1];
            destinationPixel[1] = sourcePixel[2];
            destinationPixel[2] = sourcePixel[3];
            destinationPixel[3] = sourcePixel[0];
        }
#else
        // Swap pixel channels BGRA <-> RGBA.
        destinationPixel[0] = sourcePixel[2];
        destinationPixel[1] = sourcePixel[1];
        destinationPixel[2] = sourcePixel[0];
        destinationPixel[3] = sourcePixel[3];
#endif
    }
}

#if CPU(BIG_ENDIAN)
template<void (*convertFunctor)(PixelFormat, const uint8_t*, PixelFormat, uint8_t*)>
#else
template<void (*convertFunctor)(const uint8_t*, uint8_t*)>
#endif
static void convertImagePixelsUnaccelerated(const ConstPixelBufferConversionView& source, const PixelBufferConversionView& destination, const IntSize& destinationSize)
{
    const uint8_t* sourceRows = source.rows;
    uint8_t* destinationRows = destination.rows;

    size_t bytesPerRow = destinationSize.width() * 4;
    for (int y = 0; y < destinationSize.height(); ++y) {
        for (size_t x = 0; x < bytesPerRow; x += 4)
#if CPU(BIG_ENDIAN)
            convertFunctor(source.format.pixelFormat, &sourceRows[x], destination.format.pixelFormat, &destinationRows[x]);
#else
            convertFunctor(&sourceRows[x], &destinationRows[x]);
#endif
        sourceRows += source.bytesPerRow;
        destinationRows += destination.bytesPerRow;
    }
}

void convertImagePixels(const ConstPixelBufferConversionView& source, const PixelBufferConversionView& destination, const IntSize& destinationSize)
{
    // We don't currently support converting pixel data with non-8-bit buffers.
    // BGRA8 is actually ARGB8 on BIG_ENDIAN.
    ASSERT(source.format.pixelFormat == PixelFormat::RGBA8 || source.format.pixelFormat == PixelFormat::BGRA8);
    ASSERT(destination.format.pixelFormat == PixelFormat::RGBA8 || destination.format.pixelFormat == PixelFormat::BGRA8);

#if USE(ACCELERATE) && USE(CG)
    if (source.format.alphaFormat == destination.format.alphaFormat && source.format.pixelFormat == destination.format.pixelFormat) {
        // FIXME: Can thes both just use per-row memcpy?
        if (source.format.alphaFormat == AlphaPremultiplication::Premultiplied)
            convertImagePixelsUnaccelerated<convertSinglePixelPremultipliedToPremultiplied<PixelFormatConversion::None>>(source, destination, destinationSize);
        else
            convertImagePixelsUnaccelerated<convertSinglePixelUnpremultipliedToUnpremultiplied<PixelFormatConversion::None>>(source, destination, destinationSize);
    } else
        convertImagePixelsAccelerated(source, destination, destinationSize);
#else
    // FIXME: We don't currently support converting pixel data between different color spaces in the non-accelerated path.
    // This could be added using conversion functions from ColorConversion.h.
    ASSERT(source.format.colorSpace == destination.format.colorSpace);

    if (source.format.alphaFormat == destination.format.alphaFormat) {
        if (source.format.pixelFormat == destination.format.pixelFormat) {
            if (source.format.alphaFormat == AlphaPremultiplication::Premultiplied)
                convertImagePixelsUnaccelerated<convertSinglePixelPremultipliedToPremultiplied<PixelFormatConversion::None>>(source, destination, destinationSize);
            else
                convertImagePixelsUnaccelerated<convertSinglePixelUnpremultipliedToUnpremultiplied<PixelFormatConversion::None>>(source, destination, destinationSize);
        } else {
            if (destination.format.alphaFormat == AlphaPremultiplication::Premultiplied)
                convertImagePixelsUnaccelerated<convertSinglePixelPremultipliedToPremultiplied<PixelFormatConversion::Permute>>(source, destination, destinationSize);
            else
                convertImagePixelsUnaccelerated<convertSinglePixelUnpremultipliedToUnpremultiplied<PixelFormatConversion::Permute>>(source, destination, destinationSize);
        }
    } else {
        if (source.format.pixelFormat == destination.format.pixelFormat) {
            if (source.format.alphaFormat == AlphaPremultiplication::Premultiplied)
                convertImagePixelsUnaccelerated<convertSinglePixelPremultipliedToUnpremultiplied<PixelFormatConversion::None>>(source, destination, destinationSize);
            else
                convertImagePixelsUnaccelerated<convertSinglePixelUnpremultipliedToPremultiplied<PixelFormatConversion::None>>(source, destination, destinationSize);
        } else {
            if (destination.format.alphaFormat == AlphaPremultiplication::Premultiplied)
                convertImagePixelsUnaccelerated<convertSinglePixelUnpremultipliedToPremultiplied<PixelFormatConversion::Permute>>(source, destination, destinationSize);
            else
                convertImagePixelsUnaccelerated<convertSinglePixelPremultipliedToUnpremultiplied<PixelFormatConversion::Permute>>(source, destination, destinationSize);
        }
    }
#endif
}

}
