/*
 * Copyright (C) 2020-2022 Jacek Piszczek
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
#include "DragImage.h"

#include "Element.h"
#include "Image.h"
#include "TextFlags.h"
#include "TextIndicator.h"
#include "FontCascade.h"
#include "FontDescription.h"
#include "FontSelector.h"
#include "GraphicsContextCairo.h"
#include "StringTruncator.h"
#include "FloatRoundedRect.h"
#include <cairo.h>
#include <wtf/URL.h>

namespace WebCore {

IntSize dragImageSize(DragImageRef image)
{
    if (image)
        return { cairo_image_surface_get_width(image.get()), cairo_image_surface_get_height(image.get()) };

    return { 0, 0 };
}

void deleteDragImage(DragImageRef)
{
}

DragImageRef scaleDragImage(DragImageRef image, FloatSize scale)
{
    if (!image)
        return nullptr;

    IntSize imageSize = dragImageSize(image);
    IntSize scaledSize(imageSize);
    scaledSize.scale(scale.width(), scale.height());
    if (imageSize == scaledSize)
        return image;

    RefPtr<cairo_surface_t> scaledSurface = adoptRef(cairo_surface_create_similar(image.get(), CAIRO_CONTENT_COLOR_ALPHA, scaledSize.width(), scaledSize.height()));

    RefPtr<cairo_t> context = adoptRef(cairo_create(scaledSurface.get()));
    cairo_scale(context.get(), scale.width(), scale.height());
    cairo_pattern_set_extend(cairo_get_source(context.get()), CAIRO_EXTEND_PAD);
    cairo_pattern_set_filter(cairo_get_source(context.get()), CAIRO_FILTER_BEST);
    cairo_set_operator(context.get(), CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(context.get(), image.get(), 0, 0);
    cairo_paint(context.get());

    return scaledSurface;
}

DragImageRef dissolveDragImageToFraction(DragImageRef image, float fraction)
{
    if (!image)
        return nullptr;

    RefPtr<cairo_t> context = adoptRef(cairo_create(image.get()));
    cairo_set_operator(context.get(), CAIRO_OPERATOR_DEST_IN);
    cairo_set_source_rgba(context.get(), 0, 0, 0, fraction);
    cairo_paint(context.get());
    return image;
}

DragImageRef createDragImageFromImage(Image* image, ImageOrientation)
{
    return image->currentNativeImage()->platformImage();
}

static bool shouldUseFontSmoothing = true;

static bool isOneLeftToRightRun(const TextRun& run)
{
    for (int i = 0; i < run.length(); i++) {
        UCharDirection direction = u_charDirection(run[i]);
        if (direction == U_RIGHT_TO_LEFT || direction > U_OTHER_NEUTRAL)
            return false;
    }
    return true;
}

static void doDrawTextAtPoint(GraphicsContext& context, const String& text, const IntPoint& point, const FontCascade& font, const Color& color, int underlinedIndex)
{
    TextRun run(text);

    context.setFillColor(color);
    if (isOneLeftToRightRun(run))
        font.drawText(context, run, point);
    else
        context.drawBidiText(font, run, point);

    if (underlinedIndex >= 0) {
        ASSERT_WITH_SECURITY_IMPLICATION(underlinedIndex < static_cast<int>(text.length()));

        int beforeWidth;
        if (underlinedIndex > 0) {
            TextRun beforeRun(StringView(text).substring(0, underlinedIndex));
            beforeWidth = font.width(beforeRun);
        } else
            beforeWidth = 0;

        TextRun underlinedRun(StringView(text).substring(underlinedIndex, 1));
        int underlinedWidth = font.width(underlinedRun);

        IntPoint underlinePoint(point);
        underlinePoint.move(beforeWidth, 1);

        context.setStrokeColor(color);
        context.drawLineForText(FloatRect(underlinePoint, FloatSize(underlinedWidth, font.size() / 16)), false);
    }
}

void WebCoreDrawDoubledTextAtPoint(GraphicsContext& context, const String& text, const IntPoint& point, const FontCascade& font, const Color& topColor, const Color& bottomColor, int underlinedIndex = -1)
{
    context.save();

    IntPoint textPos = point;

    doDrawTextAtPoint(context, text, textPos, font, bottomColor, underlinedIndex);
    textPos.move(0, -1);
    doDrawTextAtPoint(context, text, textPos, font, topColor, underlinedIndex);

    context.restore();
}

const float DragLabelBorderX = 4;
// Keep border_y in synch with DragController::LinkDragBorderInset.
const float DragLabelBorderY = 2;
const float DragLabelRadius = 5;
const float LabelBorderYOffset = 2;

const float MaxDragLabelWidth = 200;
const float MaxDragLabelStringWidth = (MaxDragLabelWidth - 2 * DragLabelBorderX);

const float DragLinkLabelFontsize = 11;
const float DragLinkUrlFontSize = 10;

static FontCascade dragLabelFont(int size, bool bold)
{
    FontCascade result;

    FontCascadeDescription description;
    description.setWeight(bold ? boldWeightValue() : normalWeightValue());
    description.setOneFamily("Times New Roman"_s);
    description.setSpecifiedSize((float)size);
    description.setComputedSize((float)size);
    result = FontCascade(WTFMove(description));
    result.update();
    return result;
}

DragImageRef createDragImageIconForCachedImageFilename(const String&)
{
    return nullptr;
}

DragImageRef createDragImageForLink(Element&, URL& url, const String& inLabel, TextIndicatorData&, float)
{
    // This is more or less an exact match for the Mac OS X code.

    const FontCascade* labelFont;
    const FontCascade* urlFont;

    static const FontCascade normalRenderingModeLabelFont = dragLabelFont(DragLinkLabelFontsize, true);
    static const FontCascade normalRenderingModeURLFont = dragLabelFont(DragLinkUrlFontSize, false);
    labelFont = &normalRenderingModeLabelFont;
    urlFont = &normalRenderingModeURLFont;

    bool drawURLString = true;
    bool clipURLString = false;
    bool clipLabelString = false;

    String urlString = url.string();
    String label = inLabel;
    if (label.isEmpty()) {
        drawURLString = false;
        label = urlString;
    }

    // First step in drawing the link drag image width.
    TextRun labelRun(label);
    TextRun urlRun(urlString);
    IntSize labelSize(labelFont->width(labelRun), labelFont->metricsOfPrimaryFont().ascent() + labelFont->metricsOfPrimaryFont().descent());

    if (labelSize.width() > MaxDragLabelStringWidth) {
        labelSize.setWidth(MaxDragLabelStringWidth);
        clipLabelString = true;
    }
    
    IntSize urlStringSize;
    IntSize imageSize(labelSize.width() + DragLabelBorderX * 2, labelSize.height() + DragLabelBorderY * 2);

    if (drawURLString) {
        urlStringSize.setWidth(urlFont->width(urlRun));
        urlStringSize.setHeight(urlFont->metricsOfPrimaryFont().ascent() + urlFont->metricsOfPrimaryFont().descent());
        imageSize.setHeight(imageSize.height() + urlStringSize.height());
        if (urlStringSize.width() > MaxDragLabelStringWidth) {
            imageSize.setWidth(MaxDragLabelWidth);
            clipURLString = true;
        } else
            imageSize.setWidth(std::max(labelSize.width(), urlStringSize.width()) + DragLabelBorderX * 2);
    }

    auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, imageSize.width(), imageSize.height());
    if (surface)
    {
        auto cairo = cairo_create(surface);
        if (cairo)
        {
            auto context = new WebCore::GraphicsContextCairo(cairo);
            if (context)
            {
                // On Mac alpha is {0.7, 0.7, 0.7, 0.8}, however we can't control alpha
                // for drag images on win, so we use 1
                constexpr auto backgroundColor = SRGBA<uint8_t> { 255, 255, 255 };
                static const IntSize radii(DragLabelRadius, DragLabelRadius);
                IntRect rect(0, 0, imageSize.width(), imageSize.height());
                context->fillRoundedRect(FloatRoundedRect(rect, radii, radii, radii, radii), backgroundColor);
             
                // Draw the text
                constexpr auto topColor = Color::black; // original alpha = 0.75
                constexpr auto bottomColor = Color::white.colorWithAlphaByte(127); // original alpha = 0.5
                if (drawURLString) {
                    if (clipURLString)
                        urlString = StringTruncator::rightTruncate(urlString, imageSize.width() - (DragLabelBorderX * 2.0f), *urlFont);
                    IntPoint textPos(DragLabelBorderX, imageSize.height() - (LabelBorderYOffset + urlFont->metricsOfPrimaryFont().descent()));
                    WebCoreDrawDoubledTextAtPoint(*context, urlString, textPos, *urlFont, topColor, bottomColor);
                }
                
                if (clipLabelString)
                    label = StringTruncator::rightTruncate(label, imageSize.width() - (DragLabelBorderX * 2.0f), *labelFont);

                IntPoint textPos(DragLabelBorderX, DragLabelBorderY + labelFont->metricsOfPrimaryFont().descent());
                WebCoreDrawDoubledTextAtPoint(*context, label, textPos, *labelFont, topColor, bottomColor);

                delete context;
                return surface;
            }
            
            cairo_destroy(cairo);
        }

        cairo_surface_destroy(surface);
    }
    
    return nullptr;
}


DragImageRef createDragImageForColor(const Color&, const FloatRect&, float, Path&)
{
    return nullptr;
}

}
