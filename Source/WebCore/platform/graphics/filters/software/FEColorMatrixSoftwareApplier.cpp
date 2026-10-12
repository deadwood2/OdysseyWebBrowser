/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2021-2022 Apple Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "FEColorMatrixSoftwareApplier.h"

#include "FEColorMatrix.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "PixelBuffer.h"
#include <wtf/MathExtras.h>

#if USE(ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

namespace WebCore {

FEColorMatrixSoftwareApplier::FEColorMatrixSoftwareApplier(const FEColorMatrix& effect)
    : Base(effect)
{
    if (m_effect.type() == ColorMatrixType::FECOLORMATRIX_TYPE_SATURATE)
        FEColorMatrix::calculateSaturateComponents(m_components, m_effect.values()[0]);
    else if (m_effect.type() == ColorMatrixType::FECOLORMATRIX_TYPE_HUEROTATE)
        FEColorMatrix::calculateHueRotateComponents(m_components, m_effect.values()[0]);
}

inline void FEColorMatrixSoftwareApplier::matrix(float& red, float& green, float& blue, float& alpha) const
{
    float r = red;
    float g = green;
    float b = blue;
    float a = alpha;

    const auto& values = m_effect.values();

    red   = values[ 0] * r + values[ 1] * g + values[ 2] * b + values[ 3] * a + values[ 4] * 255;
    green = values[ 5] * r + values[ 6] * g + values[ 7] * b + values[ 8] * a + values[ 9] * 255;
    blue  = values[10] * r + values[11] * g + values[12] * b + values[13] * a + values[14] * 255;
    alpha = values[15] * r + values[16] * g + values[17] * b + values[18] * a + values[19] * 255;
}

inline void FEColorMatrixSoftwareApplier::saturateAndHueRotate(float& red, float& green, float& blue) const
{
    float r = red;
    float g = green;
    float b = blue;

    red     = r * m_components[0] + g * m_components[1] + b * m_components[2];
    green   = r * m_components[3] + g * m_components[4] + b * m_components[5];
    blue    = r * m_components[6] + g * m_components[7] + b * m_components[8];
}

// FIXME: this should use the luminance(...) function in ColorLuminance.h.
inline void FEColorMatrixSoftwareApplier::luminance(float& red, float& green, float& blue, float& alpha) const
{
    alpha = 0.2125 * red + 0.7154 * green + 0.0721 * blue;
    red = 0;
    green = 0;
    blue = 0;
}

#if USE(ACCELERATE)
void FEColorMatrixSoftwareApplier::applyPlatformAccelerated(PixelBuffer& pixelBuffer) const
{
    auto* pixelBytes = pixelBuffer.bytes().data();
    auto bufferSize = pixelBuffer.size();
    const int32_t divisor = 256;

    vImage_Buffer src;
    src.width = bufferSize.width();
    src.height = bufferSize.height();
    src.rowBytes = bufferSize.width() * 4;
    src.data = pixelBytes;
    
    vImage_Buffer dest;
    dest.width = bufferSize.width();
    dest.height = bufferSize.height();
    dest.rowBytes = bufferSize.width() * 4;
    dest.data = pixelBytes;

    switch (m_effect.type()) {
    case ColorMatrixType::FECOLORMATRIX_TYPE_UNKNOWN:
        break;

    case ColorMatrixType::FECOLORMATRIX_TYPE_MATRIX: {
        const auto& values = m_effect.values();

        const int16_t matrix[4 * 4] = {
            static_cast<int16_t>(roundf(values[ 0] * divisor)),
            static_cast<int16_t>(roundf(values[ 5] * divisor)),
            static_cast<int16_t>(roundf(values[10] * divisor)),
            static_cast<int16_t>(roundf(values[15] * divisor)),

            static_cast<int16_t>(roundf(values[ 1] * divisor)),
            static_cast<int16_t>(roundf(values[ 6] * divisor)),
            static_cast<int16_t>(roundf(values[11] * divisor)),
            static_cast<int16_t>(roundf(values[16] * divisor)),

            static_cast<int16_t>(roundf(values[ 2] * divisor)),
            static_cast<int16_t>(roundf(values[ 7] * divisor)),
            static_cast<int16_t>(roundf(values[12] * divisor)),
            static_cast<int16_t>(roundf(values[17] * divisor)),

            static_cast<int16_t>(roundf(values[ 3] * divisor)),
            static_cast<int16_t>(roundf(values[ 8] * divisor)),
            static_cast<int16_t>(roundf(values[13] * divisor)),
            static_cast<int16_t>(roundf(values[18] * divisor)),
        };
        vImageMatrixMultiply_ARGB8888(&src, &dest, matrix, divisor, nullptr, nullptr, kvImageNoFlags);
        break;
    }

    case ColorMatrixType::FECOLORMATRIX_TYPE_SATURATE:
    case ColorMatrixType::FECOLORMATRIX_TYPE_HUEROTATE: {
        const int16_t matrix[4 * 4] = {
            static_cast<int16_t>(roundf(m_components[0] * divisor)),
            static_cast<int16_t>(roundf(m_components[3] * divisor)),
            static_cast<int16_t>(roundf(m_components[6] * divisor)),
            0,

            static_cast<int16_t>(roundf(m_components[1] * divisor)),
            static_cast<int16_t>(roundf(m_components[4] * divisor)),
            static_cast<int16_t>(roundf(m_components[7] * divisor)),
            0,

            static_cast<int16_t>(roundf(m_components[2] * divisor)),
            static_cast<int16_t>(roundf(m_components[5] * divisor)),
            static_cast<int16_t>(roundf(m_components[8] * divisor)),
            0,

            0,
            0,
            0,
            divisor,
        };
        vImageMatrixMultiply_ARGB8888(&src, &dest, matrix, divisor, nullptr, nullptr, kvImageNoFlags);
        break;
    }
    case ColorMatrixType::FECOLORMATRIX_TYPE_LUMINANCETOALPHA: {
        const int16_t matrix[4 * 4] = {
            0,
            0,
            0,
            static_cast<int16_t>(roundf(0.2125 * divisor)),

            0,
            0,
            0,
            static_cast<int16_t>(roundf(0.7154 * divisor)),

            0,
            0,
            0,
            static_cast<int16_t>(roundf(0.0721 * divisor)),

            0,
            0,
            0,
            0,
        };
        vImageMatrixMultiply_ARGB8888(&src, &dest, matrix, divisor, nullptr, nullptr, kvImageNoFlags);
        break;
    }
    }
}
#endif

void FEColorMatrixSoftwareApplier::applyPlatformUnaccelerated(PixelBuffer& pixelBuffer) const
{
    auto pixelByteLength = pixelBuffer.bytes().size();

    switch (m_effect.type()) {
    case ColorMatrixType::FECOLORMATRIX_TYPE_UNKNOWN:
        break;

    case ColorMatrixType::FECOLORMATRIX_TYPE_MATRIX:
        for (unsigned pixelByteOffset = 0; pixelByteOffset < pixelByteLength; pixelByteOffset += 4) {
            float red = pixelBuffer.item(pixelByteOffset);
            float green = pixelBuffer.item(pixelByteOffset + 1);
            float blue = pixelBuffer.item(pixelByteOffset + 2);
            float alpha = pixelBuffer.item(pixelByteOffset + 3);
            matrix(red, green, blue, alpha);
            pixelBuffer.set(pixelByteOffset, red);
            pixelBuffer.set(pixelByteOffset + 1, green);
            pixelBuffer.set(pixelByteOffset + 2, blue);
            pixelBuffer.set(pixelByteOffset + 3, alpha);
        }
        break;

    case ColorMatrixType::FECOLORMATRIX_TYPE_SATURATE:
    case ColorMatrixType::FECOLORMATRIX_TYPE_HUEROTATE:
        for (unsigned pixelByteOffset = 0; pixelByteOffset < pixelByteLength; pixelByteOffset += 4) {
            float red = pixelBuffer.item(pixelByteOffset);
            float green = pixelBuffer.item(pixelByteOffset + 1);
            float blue = pixelBuffer.item(pixelByteOffset + 2);
            float alpha = pixelBuffer.item(pixelByteOffset + 3);
            saturateAndHueRotate(red, green, blue);
            pixelBuffer.set(pixelByteOffset, red);
            pixelBuffer.set(pixelByteOffset + 1, green);
            pixelBuffer.set(pixelByteOffset + 2, blue);
            pixelBuffer.set(pixelByteOffset + 3, alpha);
        }
        break;

    case ColorMatrixType::FECOLORMATRIX_TYPE_LUMINANCETOALPHA:
        for (unsigned pixelByteOffset = 0; pixelByteOffset < pixelByteLength; pixelByteOffset += 4) {
            float red = pixelBuffer.item(pixelByteOffset);
            float green = pixelBuffer.item(pixelByteOffset + 1);
            float blue = pixelBuffer.item(pixelByteOffset + 2);
            float alpha = pixelBuffer.item(pixelByteOffset + 3);
            luminance(red, green, blue, alpha);
            pixelBuffer.set(pixelByteOffset, red);
            pixelBuffer.set(pixelByteOffset + 1, green);
            pixelBuffer.set(pixelByteOffset + 2, blue);
            pixelBuffer.set(pixelByteOffset + 3, alpha);
        }
        break;
    }
}

#if OS(MORPHOS)
static inline uint32_t matrixOnePixel(const int32_t *values, const uint32_t pixel)
{
       int32_t alpha, a;
       int32_t red, r, green, g, blue, b;

       r = (pixel >> 24);
       g = (pixel >> 16) & 0xff;
       b = (pixel >> 8) & 0xff;
       a = pixel & 0xff;

       red     = values[ 0] * r + values[ 1] * g + values[ 2] * b + values[ 3] * a + values[ 4];
       green   = values[ 5] * r + values[ 6] * g + values [7] * b + values[ 8] * a + values[ 9];
       blue    = values[10] * r + values[11] * g + values[12] * b + values[13] * a + values[14];
       alpha   = values[15] * r + values[16] * g + values[17] * b + values[18] * a + values[19];

       if (red < 0) red = 0;
       if (green < 0) green = 0;
       if (blue < 0) blue = 0;
       if (alpha < 0) alpha = 0;

       red >>= 8;
       green >>= 8;
       blue >>= 8;
       alpha >>= 8;

       if (red > 255) red = 255;
       if (green > 255) green = 255;
       if (blue > 255) blue = 255;
       if (alpha > 255) alpha = 255;

       return (red << 24) | (green << 16) | (blue << 8) | alpha;
}

void applyMatrixFast(const Vector<float>& matrix, uint8_t *pixels, const int width, const int height)
{
       const size_t pixelArrayLength = width * height;

       int32_t values[20];
       for (size_t i = 0; i < 20; i++)
       {
               values[i] = roundf(matrix[i] * 256.f);
       }

       values[4]  *= 256;
       values[9]  *= 256;
       values[14] *= 256;
       values[19] *= 256;

       uint32_t *p = (uint32_t *)pixels;
       for (size_t i = 0; i < pixelArrayLength; i++)
       {
               uint32_t pixel = matrixOnePixel(values, *p);
               *p++ = pixel;
       }
}

static inline uint32_t saturateOnePixel(int32_t *components, const uint32_t pixel)
{
       int32_t a;
       int32_t red, r, green, g, blue, b;

       r = (pixel >> 24);
       g = (pixel >> 16) & 0xff;
       b = (pixel >> 8) & 0xff;
       a = pixel & 0xff;

    red     = r * components[0] + g * components[1] + b * components[2];
    green   = r * components[3] + g * components[4] + b * components[5];
    blue    = r * components[6] + g * components[7] + b * components[8];

       if (red < 0) red = 0;
       if (green < 0) green = 0;
       if (blue < 0) blue = 0;

       red >>= 8;
       green >>= 8;
       blue >>= 8;

       if (red > 255) red = 255;
       if (green > 255) green = 255;
       if (blue > 255) blue = 255;

       return (red << 24) | (green << 16) | (blue << 8) | a;
}

void applyHueSaturateFast(const float *components, uint8_t *pixels, const int width, const int height)
{
       const size_t pixelArrayLength = width * height;

       int32_t values[9];
       for (size_t i = 0; i < 9; i++)
       {
               values[i] = roundf(components[i] * 256.f);
       }

       uint32_t *p = (uint32_t *)pixels;
       for (size_t i = 0; i < pixelArrayLength; i++)
       {
               uint32_t pixel = saturateOnePixel(values, *p);
               *p++ = pixel;
       }
}

static inline uint32_t luminanceOnePixel(const uint32_t pixel)
{
       int32_t alpha;
       int32_t r, g, b;

       r = (pixel >> 24);
       g = (pixel >> 16) & 0xff;
       b = (pixel >> 8) & 0xff;

       alpha = 54 * r + 183 * g + 18 * b;

       if (alpha < 0) alpha = 0;
       alpha >>= 8;
       if (alpha > 255) alpha = 255;

       return alpha;
}

void applyLuminanceFast(uint8_t *pixels, const int width, const int height)
{
       const size_t pixelArrayLength = width * height;

       uint32_t *p = (uint32_t *)pixels;
       for (size_t i = 0; i < pixelArrayLength; i++)
       {
               uint32_t pixel = luminanceOnePixel(*p);
               *p++ = pixel;
       }
}

#endif

void FEColorMatrixSoftwareApplier::applyPlatform(PixelBuffer& pixelBuffer) const
{
#if USE(ACCELERATE)
    const auto& values = m_effect.values();

    // vImageMatrixMultiply_ARGB8888 takes a 4x4 matrix, if any value in the last column of the FEColorMatrix 5x4 matrix
    // is not zero, fall back to non-vImage code.
    if (m_effect.type() != ColorMatrixType::FECOLORMATRIX_TYPE_MATRIX || (!values[4] && !values[9] && !values[14] && !values[19])) {
        applyPlatformAccelerated(pixelBuffer);
        return;
    }
#elif OS(MORPHOS)
    switch (m_effect.type()) {
    case ColorMatrixType::FECOLORMATRIX_TYPE_MATRIX:
       applyMatrixFast(m_effect.values(), pixelBuffer.bytes().data(), pixelBuffer.size().width(), pixelBuffer.size().height());
       return;
    case ColorMatrixType::FECOLORMATRIX_TYPE_SATURATE:
    case ColorMatrixType::FECOLORMATRIX_TYPE_HUEROTATE:
       applyHueSaturateFast(m_components, pixelBuffer.bytes().data(), pixelBuffer.size().width(), pixelBuffer.size().height());
       return;
       case ColorMatrixType::FECOLORMATRIX_TYPE_LUMINANCETOALPHA:
       applyLuminanceFast(pixelBuffer.bytes().data(), pixelBuffer.size().width(), pixelBuffer.size().height());
       return;
    default:
        break;
    }
#endif
    applyPlatformUnaccelerated(pixelBuffer);
}

bool FEColorMatrixSoftwareApplier::apply(const Filter&, const FilterImageVector& inputs, FilterImage& result) const
{
    auto& input = inputs[0].get();

    RefPtr resultImage = result.imageBuffer();
    if (!resultImage)
        return false;

    RefPtr inputImage = input.imageBuffer();
    if (inputImage) {
        auto inputImageRect = input.absoluteImageRectRelativeTo(result);
        resultImage->context().drawImageBuffer(*inputImage, inputImageRect);
    }

    PixelBufferFormat format { AlphaPremultiplication::Unpremultiplied, PixelFormat::RGBA8, result.colorSpace() };
    IntRect imageRect(IntPoint(), resultImage->truncatedLogicalSize());
    auto pixelBuffer = resultImage->getPixelBuffer(format, imageRect);
    if (!pixelBuffer)
        return false;

    applyPlatform(*pixelBuffer);
    resultImage->putPixelBuffer(*pixelBuffer, imageRect);
    return true;
}

} // namespace WebCore
