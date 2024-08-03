/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SimpleLineLayoutFunctions.h"

#include "FontCache.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HitTestLocation.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InlineTextBox.h"
#include "PaintInfo.h"
#include "RenderBlockFlow.h"
#include "RenderIterator.h"
#include "RenderStyle.h"
#include "RenderText.h"
#include "RenderView.h"
#include "Settings.h"
#include "SimpleLineLayoutResolver.h"
#include "Text.h"
#include "TextDecorationPainter.h"
#include "TextPaintStyle.h"
#include "TextPainter.h"

#if ENABLE(TREE_DEBUGGING)
#include <stdio.h>
#endif

namespace WebCore {
namespace SimpleLineLayout {

static void paintDebugBorders(GraphicsContext& context, LayoutRect borderRect, const LayoutPoint& paintOffset)
{
    borderRect.moveBy(paintOffset);
    IntRect snappedRect = snappedIntRect(borderRect);
    if (snappedRect.isEmpty())
        return;
    GraphicsContextStateSaver stateSaver(context);
    context.setStrokeColor(Color(0, 255, 0));
    context.setFillColor(Color::transparent);
    context.drawRect(snappedRect);
}

static FloatRect computeOverflow(const RenderBlockFlow& flow, const FloatRect& layoutRect)
{
    auto overflowRect = layoutRect;
    auto strokeOverflow = std::ceil(flow.style().textStrokeWidth());
    overflowRect.inflate(strokeOverflow);

    auto letterSpacing = flow.style().fontCascade().letterSpacing();
    if (letterSpacing >= 0)
        return overflowRect;
    // Last letter's negative spacing shrinks layout rect. Push it to visual overflow.
    overflowRect.expand(-letterSpacing, 0);
    return overflowRect;
}

void paintFlow(const RenderBlockFlow& flow, const Layout& layout, PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.phase != PaintPhaseForeground)
        return;

    auto& style = flow.style();
    if (style.visibility() != VISIBLE)
        return;

    bool debugBordersEnabled = flow.frame().settings().simpleLineLayoutDebugBordersEnabled();

    TextPainter textPainter(paintInfo.context());
    textPainter.setFont(style.fontCascade());
    textPainter.setTextPaintStyle(computeTextPaintStyle(flow.frame(), style, paintInfo));

    Optional<TextDecorationPainter> textDecorationPainter;
    if (style.textDecorationsInEffect() != TextDecorationNone) {
        const RenderText* textRenderer = childrenOfType<RenderText>(flow).first();
        if (textRenderer) {
            textDecorationPainter = TextDecorationPainter(paintInfo.context(), style.textDecorationsInEffect(), *textRenderer, false);
            textDecorationPainter->setFont(style.fontCascade());
            textDecorationPainter->setBaseline(style.fontMetrics().ascent());
        }
    }

    LayoutRect paintRect = paintInfo.rect;
    paintRect.moveBy(-paintOffset);

    auto resolver = runResolver(flow, layout);
    float deviceScaleFactor = flow.document().deviceScaleFactor();
    for (auto run : resolver.rangeForRect(paintRect)) {
        if (run.start() == run.end())
            continue;

        FloatRect rect = run.rect();
        FloatRect visualOverflowRect = computeOverflow(flow, rect);
        if (paintRect.y() > visualOverflowRect.maxY() || paintRect.maxY() < visualOverflowRect.y())
            continue;

        // x position indicates the line offset from the rootbox. It's always 0 in case of simple line layout.
        TextRun textRun(run.text(), 0, run.expansion(), run.expansionBehavior());
        textRun.setTabSize(!style.collapseWhiteSpace(), style.tabSize());
        FloatPoint textOrigin = FloatPoint(rect.x() + paintOffset.x(), roundToDevicePixel(run.baselinePosition() + paintOffset.y(), deviceScaleFactor));
        textPainter.paintText(textRun, textRun.length(), rect, textOrigin);
        if (textDecorationPainter) {
            textDecorationPainter->setWidth(rect.width());
            textDecorationPainter->paintTextDecoration(textRun, textOrigin, rect.location() + paintOffset);
        }
        if (debugBordersEnabled)
            paintDebugBorders(paintInfo.context(), LayoutRect(run.rect()), paintOffset);
    }
}

bool hitTestFlow(const RenderBlockFlow& flow, const Layout& layout, const HitTestRequest& request, HitTestResult& result, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    if (hitTestAction != HitTestForeground)
        return false;

    if (!layout.runCount())
        return false;

    auto& style = flow.style();
    if (style.visibility() != VISIBLE || style.pointerEvents() == PE_NONE)
        return false;

    RenderObject& renderer = *flow.firstChild();
    LayoutRect rangeRect = locationInContainer.boundingBox();
    rangeRect.moveBy(-accumulatedOffset);

    auto resolver = lineResolver(flow, layout);
    for (FloatRect lineRect : resolver.rangeForRect(rangeRect)) {
        lineRect.moveBy(accumulatedOffset);
        if (!locationInContainer.intersects(lineRect))
            continue;
        renderer.updateHitTestResult(result, locationInContainer.point() - toLayoutSize(accumulatedOffset));
        if (!result.addNodeToRectBasedTestResult(renderer.node(), request, locationInContainer, lineRect))
            return true;
    }

    return false;
}

void collectFlowOverflow(RenderBlockFlow& flow, const Layout& layout)
{
    for (auto lineRect : lineResolver(flow, layout)) {
        LayoutRect visualOverflowRect = LayoutRect(computeOverflow(flow, lineRect));
        flow.addLayoutOverflow(LayoutRect(lineRect));
        flow.addVisualOverflow(visualOverflowRect);
    }
}

IntRect computeBoundingBox(const RenderObject& renderer, const Layout& layout)
{
    auto resolver = runResolver(downcast<RenderBlockFlow>(*renderer.parent()), layout);
    FloatRect boundingBoxRect;
    for (auto run : resolver.rangeForRenderer(renderer)) {
        FloatRect rect = run.rect();
        if (boundingBoxRect == FloatRect())
            boundingBoxRect = rect;
        else
            boundingBoxRect.uniteEvenIfEmpty(rect);
    }
    return enclosingIntRect(boundingBoxRect);
}

IntPoint computeFirstRunLocation(const RenderObject& renderer, const Layout& layout)
{
    auto resolver = runResolver(downcast<RenderBlockFlow>(*renderer.parent()), layout);
    auto range = resolver.rangeForRenderer(renderer);
    auto begin = range.begin();
    if (begin == range.end())
        return IntPoint(0, 0);
    return flooredIntPoint((*begin).rect().location());
}

Vector<IntRect> collectAbsoluteRects(const RenderObject& renderer, const Layout& layout, const LayoutPoint& accumulatedOffset)
{
    Vector<IntRect> rects;
    auto resolver = runResolver(downcast<RenderBlockFlow>(*renderer.parent()), layout);
    for (auto run : resolver.rangeForRenderer(renderer)) {
        FloatRect rect = run.rect();
        rects.append(enclosingIntRect(FloatRect(accumulatedOffset + rect.location(), rect.size())));
    }
    return rects;
}

Vector<FloatQuad> collectAbsoluteQuads(const RenderObject& renderer, const Layout& layout, bool* wasFixed)
{
    Vector<FloatQuad> quads;
    auto resolver = runResolver(downcast<RenderBlockFlow>(*renderer.parent()), layout);
    for (auto run : resolver.rangeForRenderer(renderer))
        quads.append(renderer.localToAbsoluteQuad(FloatQuad(run.rect()), UseTransforms, wasFixed));
    return quads;
}

#if ENABLE(TREE_DEBUGGING)
static void printPrefix(int& printedCharacters, int depth)
{
    fprintf(stderr, "-------- --");
    printedCharacters = 0;
    while (++printedCharacters <= depth * 2)
        fputc(' ', stderr);
}

void showLineLayoutForFlow(const RenderBlockFlow& flow, const Layout& layout, int depth)
{
    int printedCharacters = 0;
    printPrefix(printedCharacters, depth);

    fprintf(stderr, "SimpleLineLayout (%u lines, %u runs) (%p)\n", layout.lineCount(), layout.runCount(), &layout);
    ++depth;

    for (auto run : runResolver(flow, layout)) {
        FloatRect rect = run.rect();
        printPrefix(printedCharacters, depth);
        if (run.start() < run.end()) {
            fprintf(stderr, "line %u run(%u, %u) (%.2f, %.2f) (%.2f, %.2f) \"%s\"\n", run.lineIndex(), run.start(), run.end(),
                rect.x(), rect.y(), rect.width(), rect.height(), run.text().toStringWithoutCopying().utf8().data());
        } else {
            ASSERT(run.start() == run.end());
            fprintf(stderr, "line break %u run(%u, %u) (%.2f, %.2f) (%.2f, %.2f)\n", run.lineIndex(), run.start(), run.end(), rect.x(), rect.y(), rect.width(), rect.height());
        }
    }
}
#endif

}
}
