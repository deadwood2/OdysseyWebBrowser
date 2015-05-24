/*
 * Copyright 2011 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __OWBPRINTCONTEXT__
#define __OWBPRINTCONTEXT__

#include "config.h"
#include "GraphicsContext.h"
#include "Frame.h"
#include "FrameView.h"
#include "PrintContext.h"

#include <clib/debug_protos.h>

namespace WebCore {

// Simple class to override some of PrintContext behavior.
class OWBPrintContext : public PrintContext {
	WTF_MAKE_NONCOPYABLE(OWBPrintContext);
public:
	OWBPrintContext(Frame* frame)
        : PrintContext(frame)
        , m_printedPageWidth(0)
    {
    }

	virtual ~OWBPrintContext() { }

    virtual void begin(float width, float height)
    {
        ASSERT(!m_printedPageWidth);
        m_printedPageWidth = width;
        PrintContext::begin(m_printedPageWidth, height);
    }

    virtual void end()
    {
        PrintContext::end();
    }

    virtual float getPageShrink(int pageNumber) const
    {
        IntRect pageRect = m_pageRects[pageNumber];
        return m_printedPageWidth / pageRect.width();
    }

    // Spools the printed page, a subrect of m_frame. Skip the scale step.
    // NativeTheme doesn't play well with scaling. Scaling is done browser side
    // instead. Returns the scale to be applied.
    // On Linux, we don't have the problem with NativeTheme, hence we let WebKit
    // do the scaling and ignore the return value.
    virtual float spoolPage(GraphicsContext& ctx, int pageNumber, float scale = 1.0)
    {
        IntRect pageRect = m_pageRects[pageNumber];
	//kprintf("spoolPage %d pageRect [%d %d %d %d] m_printedPageWidth %f scale %f\n", pageNumber, pageRect.x(), pageRect.y(), pageRect.width(), pageRect.height(), m_printedPageWidth, scale);

        ctx.save();

        ctx.scale(WebCore::FloatSize(scale, scale));

        ctx.translate(static_cast<float>(-pageRect.x()),
                      static_cast<float>(-pageRect.y()));
        ctx.clip(pageRect);
        m_frame->view()->paintContents(&ctx, pageRect);
        ctx.restore();
        return scale;
    }

    void spoolAllPagesWithBoundaries(GraphicsContext& graphicsContext, const FloatSize& pageSizeInPixels)
    {
        if (!m_frame->document() || !m_frame->view() || !m_frame->document()->renderView())
            return;

        m_frame->document()->updateLayout();

        float pageHeight;
        computePageRects(FloatRect(FloatPoint(0, 0), pageSizeInPixels), 0, 0, 1, pageHeight);

        const float pageWidth = pageSizeInPixels.width();
        size_t numPages = pageRects().size();
        int totalHeight = numPages * (pageSizeInPixels.height() + 1) - 1;

        // Fill the whole background by white.
        graphicsContext.setFillColor(Color(255, 255, 255), ColorSpaceDeviceRGB);
        graphicsContext.fillRect(FloatRect(0, 0, pageWidth, totalHeight));

        graphicsContext.save();

        int currentHeight = 0;
        for (size_t pageIndex = 0; pageIndex < numPages; pageIndex++) {
            // Draw a line for a page boundary if this isn't the first page.
            if (pageIndex > 0) {
                graphicsContext.save();
                graphicsContext.setStrokeColor(Color(0, 0, 255), ColorSpaceDeviceRGB);
                graphicsContext.setFillColor(Color(0, 0, 255), ColorSpaceDeviceRGB);
                graphicsContext.drawLine(IntPoint(0, currentHeight),
                                         IntPoint(pageWidth, currentHeight));
                graphicsContext.restore();
            }

            graphicsContext.save();

            graphicsContext.translate(0, currentHeight);

            spoolPage(graphicsContext, pageIndex);
            graphicsContext.restore();

            currentHeight += pageSizeInPixels.height() + 1;
        }

        graphicsContext.restore();
    }

    virtual void computePageRects(const FloatRect& printRect, float headerHeight, float footerHeight, float userScaleFactor, float& outPageHeight)
    {
        PrintContext::computePageRects(printRect, headerHeight, footerHeight, userScaleFactor, outPageHeight);
    }

    virtual int pageCount() const
    {
        return PrintContext::pageCount();
    }

    virtual bool shouldUseBrowserOverlays() const
    {
        return true;
    }

private:
    // Set when printing.
    float m_printedPageWidth;
};

}

#endif
