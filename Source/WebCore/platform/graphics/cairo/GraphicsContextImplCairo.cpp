/*
 * Copyright (C) 2017 Metrological Group B.V.
 * Copyright (C) 2017 Igalia S.L.
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
#include "GraphicsContextImplCairo.h"

#if USE(CAIRO)

#include "CairoOperations.h"
#include "Font.h"
#include "GlyphBuffer.h"
#include "GraphicsContextPlatformPrivateCairo.h"

namespace WebCore {

GraphicsContext::GraphicsContextImplFactory GraphicsContextImplCairo::createFactory(PlatformContextCairo& platformContext)
{
    return GraphicsContext::GraphicsContextImplFactory(
        [&platformContext](GraphicsContext& context)
        {
            return std::make_unique<GraphicsContextImplCairo>(context, platformContext);
        });
}

GraphicsContext::GraphicsContextImplFactory GraphicsContextImplCairo::createFactory(cairo_t* cairoContext)
{
    return GraphicsContext::GraphicsContextImplFactory(
        [cairoContext](GraphicsContext& context)
        {
            return std::make_unique<GraphicsContextImplCairo>(context, cairoContext);
        });
}

GraphicsContextImplCairo::GraphicsContextImplCairo(GraphicsContext& context, PlatformContextCairo& platformContext)
    : GraphicsContextImpl(context, FloatRect { }, AffineTransform { })
    , m_platformContext(platformContext)
    , m_private(std::make_unique<GraphicsContextPlatformPrivate>(m_platformContext))
{
    m_platformContext.setGraphicsContextPrivate(m_private.get());
    m_private->syncContext(m_platformContext.cr());
}

GraphicsContextImplCairo::GraphicsContextImplCairo(GraphicsContext& context, cairo_t* cairoContext)
    : GraphicsContextImpl(context, FloatRect { }, AffineTransform { })
    , m_ownedPlatformContext(std::make_unique<PlatformContextCairo>(cairoContext))
    , m_platformContext(*m_ownedPlatformContext)
    , m_private(std::make_unique<GraphicsContextPlatformPrivate>(m_platformContext))
{
    m_platformContext.setGraphicsContextPrivate(m_private.get());
    m_private->syncContext(m_platformContext.cr());
}

GraphicsContextImplCairo::~GraphicsContextImplCairo()
{
    m_platformContext.setGraphicsContextPrivate(nullptr);
}

bool GraphicsContextImplCairo::hasPlatformContext() const
{
    return true;
}

PlatformContextCairo* GraphicsContextImplCairo::platformContext() const
{
    return &m_platformContext;
}

void GraphicsContextImplCairo::updateState(const GraphicsContextState& state, GraphicsContextState::StateChangeFlags flags)
{
    if (flags & GraphicsContextState::StrokeThicknessChange)
        Cairo::State::setStrokeThickness(m_platformContext, state.strokeThickness);

    if (flags & GraphicsContextState::StrokeStyleChange)
        Cairo::State::setStrokeStyle(m_platformContext, state.strokeStyle);

    if (flags & GraphicsContextState::ShadowChange) {
        if (state.shadowsIgnoreTransforms) {
            // Meaning that this graphics context is associated with a CanvasRenderingContext
            // We flip the height since CG and HTML5 Canvas have opposite Y axis
            auto& mutableState = const_cast<GraphicsContextState&>(graphicsContext().state());
            auto& shadowOffset = state.shadowOffset;
            mutableState.shadowOffset = { shadowOffset.width(), -shadowOffset.height() };
        }
    }

    if (flags & GraphicsContextState::CompositeOperationChange)
        Cairo::State::setCompositeOperation(m_platformContext, state.compositeOperator, state.blendMode);

    if (flags & GraphicsContextState::ShouldAntialiasChange)
        Cairo::State::setShouldAntialias(m_platformContext, state.shouldAntialias);
}

void GraphicsContextImplCairo::clearShadow()
{
}

void GraphicsContextImplCairo::setLineCap(LineCap lineCap)
{
    Cairo::setLineCap(m_platformContext, lineCap);
}

void GraphicsContextImplCairo::setLineDash(const DashArray& dashes, float dashOffset)
{
    Cairo::setLineDash(m_platformContext, dashes, dashOffset);
}

void GraphicsContextImplCairo::setLineJoin(LineJoin lineJoin)
{
    Cairo::setLineJoin(m_platformContext, lineJoin);
}

void GraphicsContextImplCairo::setMiterLimit(float miterLimit)
{
    Cairo::setMiterLimit(m_platformContext, miterLimit);
}

void GraphicsContextImplCairo::fillRect(const FloatRect& rect)
{
    auto& context = graphicsContext();
    auto& state = context.state();
    Cairo::fillRect(m_platformContext, rect, Cairo::FillSource(state), Cairo::ShadowState(state), context);
}

void GraphicsContextImplCairo::fillRect(const FloatRect& rect, const Color& color)
{
    auto& context = graphicsContext();
    Cairo::fillRect(m_platformContext, rect, color, Cairo::ShadowState(context.state()), context);
}

void GraphicsContextImplCairo::fillRect(const FloatRect& rect, Gradient& gradient)
{
    RefPtr<cairo_pattern_t> platformGradient = adoptRef(gradient.createPlatformGradient(1.0));

    Cairo::save(m_platformContext);
    Cairo::fillRect(m_platformContext, rect, platformGradient.get());
    Cairo::restore(m_platformContext);
}

void GraphicsContextImplCairo::fillRect(const FloatRect& rect, const Color& color, CompositeOperator compositeOperator, BlendMode blendMode)
{
    auto& context = graphicsContext();
    CompositeOperator previousOperator = context.state().compositeOperator;

    Cairo::State::setCompositeOperation(m_platformContext, compositeOperator, blendMode);
    Cairo::fillRect(m_platformContext, rect, color, Cairo::ShadowState(context.state()), context);
    Cairo::State::setCompositeOperation(m_platformContext, previousOperator, BlendModeNormal);
}

void GraphicsContextImplCairo::fillRoundedRect(const FloatRoundedRect& rect, const Color& color, BlendMode blendMode)
{
    auto& context = graphicsContext();

    CompositeOperator previousOperator = context.compositeOperation();
    Cairo::State::setCompositeOperation(m_platformContext, previousOperator, blendMode);

    Cairo::ShadowState shadowState(context.state());

    if (rect.isRounded())
        Cairo::fillRoundedRect(m_platformContext, rect, color, shadowState, context);
    else
        Cairo::fillRect(m_platformContext, rect.rect(), color, shadowState, context);

    Cairo::State::setCompositeOperation(m_platformContext, previousOperator, BlendModeNormal);
}

void GraphicsContextImplCairo::fillRectWithRoundedHole(const FloatRect& rect, const FloatRoundedRect& roundedHoleRect, const Color&)
{
    auto& context = graphicsContext();
    Cairo::fillRectWithRoundedHole(m_platformContext, rect, roundedHoleRect, { }, Cairo::ShadowState(context.state()), context);
}

void GraphicsContextImplCairo::fillPath(const Path& path)
{
    auto& context = graphicsContext();
    auto& state = context.state();
    Cairo::fillPath(m_platformContext, path, Cairo::FillSource(state), Cairo::ShadowState(state), context);
}

void GraphicsContextImplCairo::fillEllipse(const FloatRect& rect)
{
    Path path;
    path.addEllipse(rect);
    fillPath(path);
}

void GraphicsContextImplCairo::strokeRect(const FloatRect& rect, float lineWidth)
{
    auto& context = graphicsContext();
    auto& state = context.state();
    Cairo::strokeRect(m_platformContext, rect, lineWidth, Cairo::StrokeSource(state), Cairo::ShadowState(state), context);
}

void GraphicsContextImplCairo::strokePath(const Path& path)
{
    auto& context = graphicsContext();
    auto& state = context.state();
    Cairo::strokePath(m_platformContext, path, Cairo::StrokeSource(state), Cairo::ShadowState(state), context);
}

void GraphicsContextImplCairo::strokeEllipse(const FloatRect& rect)
{
    Path path;
    path.addEllipse(rect);
    strokePath(path);
}

void GraphicsContextImplCairo::clearRect(const FloatRect& rect)
{
    Cairo::clearRect(m_platformContext, rect);
}

void GraphicsContextImplCairo::drawGlyphs(const Font& font, const GlyphBuffer& glyphBuffer, unsigned from, unsigned numGlyphs, const FloatPoint& point, FontSmoothingMode fontSmoothing)
{
    UNUSED_PARAM(fontSmoothing);
    if (!font.platformData().size())
        return;

    auto xOffset = point.x();
    Vector<cairo_glyph_t> glyphs(numGlyphs);
    {
        ASSERT(from + numGlyphs <= glyphBuffer.size());
        auto* glyphsData = glyphBuffer.glyphs(from);
        auto* advances = glyphBuffer.advances(from);

        auto yOffset = point.y();
        for (size_t i = 0; i < numGlyphs; ++i) {
            glyphs[i] = { glyphsData[i], xOffset, yOffset };
            xOffset += advances[i].width();
        }
    }

    cairo_scaled_font_t* scaledFont = font.platformData().scaledFont();
    double syntheticBoldOffset = font.syntheticBoldOffset();

    auto& context = graphicsContext();
    auto& state = context.state();
    Cairo::drawGlyphs(m_platformContext, Cairo::FillSource(state), Cairo::StrokeSource(state),
        Cairo::ShadowState(state), point, scaledFont, syntheticBoldOffset, glyphs, xOffset,
        state.textDrawingMode, state.strokeThickness, state.shadowOffset, state.shadowColor, context);
}

ImageDrawResult GraphicsContextImplCairo::drawImage(Image& image, const FloatRect& destination, const FloatRect& source, const ImagePaintingOptions& imagePaintingOptions)
{
    return GraphicsContextImpl::drawImageImpl(graphicsContext(), image, destination, source, imagePaintingOptions);
}

ImageDrawResult GraphicsContextImplCairo::drawTiledImage(Image& image, const FloatRect& destination, const FloatPoint& source, const FloatSize& tileSize, const FloatSize& spacing, const ImagePaintingOptions& imagePaintingOptions)
{
    return GraphicsContextImpl::drawTiledImageImpl(graphicsContext(), image, destination, source, tileSize, spacing, imagePaintingOptions);
}

ImageDrawResult GraphicsContextImplCairo::drawTiledImage(Image& image, const FloatRect& destination, const FloatRect& source, const FloatSize& tileScaleFactor, Image::TileRule hRule, Image::TileRule vRule, const ImagePaintingOptions& imagePaintingOptions)
{
    return GraphicsContextImpl::drawTiledImageImpl(graphicsContext(), image, destination, source, tileScaleFactor, hRule, vRule, imagePaintingOptions);
}

void GraphicsContextImplCairo::drawNativeImage(const NativeImagePtr& image, const FloatSize& imageSize, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator compositeOperator, BlendMode blendMode, ImageOrientation orientation)
{
    UNUSED_PARAM(imageSize);
    auto& context = graphicsContext();
    auto& state = context.state();
    Cairo::drawNativeImage(m_platformContext, image.get(), destRect, srcRect, compositeOperator, blendMode, orientation, state.imageInterpolationQuality, state.alpha, Cairo::ShadowState(state), graphicsContext());
}

void GraphicsContextImplCairo::drawPattern(Image& image, const FloatRect& destRect, const FloatRect& tileRect, const AffineTransform& patternTransform, const FloatPoint& phase, const FloatSize&, CompositeOperator compositeOperator, BlendMode blendMode)
{
    if (auto surface = image.nativeImageForCurrentFrame())
        Cairo::drawPattern(m_platformContext, surface.get(), IntSize(image.size()), destRect, tileRect, patternTransform, phase, compositeOperator, blendMode);
}

void GraphicsContextImplCairo::drawRect(const FloatRect& rect, float borderThickness)
{
    auto& state = graphicsContext().state();
    Cairo::drawRect(m_platformContext, rect, borderThickness, state.fillColor, state.strokeStyle, state.strokeColor);
}

void GraphicsContextImplCairo::drawLine(const FloatPoint& point1, const FloatPoint& point2)
{
    auto& state = graphicsContext().state();
    Cairo::drawLine(m_platformContext, point1, point2, state.strokeStyle, state.strokeColor, state.strokeThickness, state.shouldAntialias);
}

void GraphicsContextImplCairo::drawLinesForText(const FloatPoint& point, const DashArray& widths, bool printing, bool doubleUnderlines, float strokeThickness)
{
    UNUSED_PARAM(strokeThickness);
    auto& state = graphicsContext().state();
    Cairo::drawLinesForText(m_platformContext, point, widths, printing, doubleUnderlines, state.strokeColor, state.strokeThickness);
}

void GraphicsContextImplCairo::drawLineForDocumentMarker(const FloatPoint& origin, float width, GraphicsContext::DocumentMarkerLineStyle style)
{
    Cairo::drawLineForDocumentMarker(m_platformContext, origin, width, style);
}

void GraphicsContextImplCairo::drawEllipse(const FloatRect& rect)
{
    auto& state = graphicsContext().state();
    Cairo::drawEllipse(*platformContext(), rect, state.fillColor, state.strokeStyle, state.strokeColor, state.strokeThickness);
}

void GraphicsContextImplCairo::drawPath(const Path&)
{
}

void GraphicsContextImplCairo::drawFocusRing(const Path& path, float width, float offset, const Color& color)
{
    UNUSED_PARAM(offset);
    Cairo::drawFocusRing(m_platformContext, path, width, color);
}

void GraphicsContextImplCairo::drawFocusRing(const Vector<FloatRect>& rects, float width, float offset, const Color& color)
{
    UNUSED_PARAM(offset);
    Cairo::drawFocusRing(m_platformContext, rects, width, color);
}

void GraphicsContextImplCairo::save()
{
    Cairo::save(m_platformContext);
}

void GraphicsContextImplCairo::restore()
{
    Cairo::restore(m_platformContext);
}

void GraphicsContextImplCairo::translate(float x, float y)
{
    Cairo::translate(m_platformContext, x, y);
}

void GraphicsContextImplCairo::rotate(float angleInRadians)
{
    Cairo::rotate(m_platformContext, angleInRadians);
}

void GraphicsContextImplCairo::scale(const FloatSize& size)
{
    Cairo::scale(m_platformContext, size);
}

void GraphicsContextImplCairo::concatCTM(const AffineTransform& transform)
{
    Cairo::concatCTM(m_platformContext, transform);
}

void GraphicsContextImplCairo::setCTM(const AffineTransform& transform)
{
    Cairo::State::setCTM(m_platformContext, transform);
}

AffineTransform GraphicsContextImplCairo::getCTM(GraphicsContext::IncludeDeviceScale)
{
    return Cairo::State::getCTM(m_platformContext);
}

void GraphicsContextImplCairo::beginTransparencyLayer(float opacity)
{
    Cairo::beginTransparencyLayer(m_platformContext, opacity);
}

void GraphicsContextImplCairo::endTransparencyLayer()
{
    Cairo::endTransparencyLayer(m_platformContext);
}

void GraphicsContextImplCairo::clip(const FloatRect& rect)
{
    Cairo::clip(m_platformContext, rect);
}

void GraphicsContextImplCairo::clipOut(const FloatRect& rect)
{
    Cairo::clipOut(m_platformContext, rect);
}

void GraphicsContextImplCairo::clipOut(const Path& path)
{
    Cairo::clipOut(m_platformContext, path);
}

void GraphicsContextImplCairo::clipPath(const Path& path, WindRule clipRule)
{
    Cairo::clipPath(m_platformContext, path, clipRule);
}

IntRect GraphicsContextImplCairo::clipBounds()
{
    return Cairo::State::getClipBounds(m_platformContext);
}

void GraphicsContextImplCairo::applyDeviceScaleFactor(float)
{
}

FloatRect GraphicsContextImplCairo::roundToDevicePixels(const FloatRect& rect, GraphicsContext::RoundingMode)
{
    return Cairo::State::roundToDevicePixels(m_platformContext, rect);
}

} // namespace WebCore

#endif // USE(CAIRO)
