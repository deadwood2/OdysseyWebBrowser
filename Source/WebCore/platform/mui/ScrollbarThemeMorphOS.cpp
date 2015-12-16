/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "ScrollbarThemeMorphOS.h"

#include "BALBase.h"
#include "ChromeClient.h"
#include "ColorSpace.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "Logging.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "GraphicsContext.h"
#include "Scrollbar.h"
#include "Settings.h"

namespace WebCore {

// Constants used to figure the drag rect outside which we should snap the
// scrollbar thumb back to its origin.  These calculations are based on
// observing the behavior of the MSVC8 main window scrollbar + some
// guessing/extrapolation.
static const int kOffEndMultiplier = 3;
static const int kOffSideMultiplier = 8;

bool ScrollbarThemeBal::m_loaded = false;
  RefPtr<WebCore::Image> ScrollbarThemeBal::up = Image::loadPlatformResource("ScrollbarTheme/up");            
  RefPtr<WebCore::Image> ScrollbarThemeBal::down = Image::loadPlatformResource("ScrollbarTheme/down");        
  RefPtr<WebCore::Image> ScrollbarThemeBal::left = Image::loadPlatformResource("ScrollbarTheme/left");        
  RefPtr<WebCore::Image> ScrollbarThemeBal::right = Image::loadPlatformResource("ScrollbarTheme/right");      
  RefPtr<WebCore::Image> ScrollbarThemeBal::thumbH = Image::loadPlatformResource("ScrollbarTheme/thumbH");    
  RefPtr<WebCore::Image> ScrollbarThemeBal::thumbHL = Image::loadPlatformResource("ScrollbarTheme/thumbHL");  
  RefPtr<WebCore::Image> ScrollbarThemeBal::thumbHR = Image::loadPlatformResource("ScrollbarTheme/thumbHR");  
  RefPtr<WebCore::Image> ScrollbarThemeBal::thumbV = Image::loadPlatformResource("ScrollbarTheme/thumbV");    
  RefPtr<WebCore::Image> ScrollbarThemeBal::thumbVU = Image::loadPlatformResource("ScrollbarTheme/thumbVU");  
  RefPtr<WebCore::Image> ScrollbarThemeBal::thumbVD = Image::loadPlatformResource("ScrollbarTheme/thumbVD");  
  RefPtr<WebCore::Image> ScrollbarThemeBal::bg = Image::loadPlatformResource("ScrollbarTheme/bg");            
  RefPtr<WebCore::Image> ScrollbarThemeBal::bgh = Image::loadPlatformResource("ScrollbarTheme/bgh");          


ScrollbarTheme* ScrollbarTheme::nativeTheme()
{
    static ScrollbarThemeBal theme;
    return &theme;
}

ScrollbarThemeBal::~ScrollbarThemeBal()
{
}


int ScrollbarThemeBal::scrollbarThickness(ScrollbarControlSize controlSize)
{
    UNUSED_PARAM(controlSize);
    return 11;
}

bool ScrollbarThemeBal::hasThumb(Scrollbar& scrollbar)
{
    return thumbLength(scrollbar) > 0;
}

IntRect ScrollbarThemeBal::backButtonRect(Scrollbar& scrollbar, ScrollbarPart part, bool)
{
    if (part == BackButtonEndPart)
        return IntRect();

    // Our desired rect is essentially 17x17.

    // Our actual rect will shrink to half the available space when
    // we have < 34 pixels left.  This allows the scrollbar
    // to scale down and function even at tiny sizes.
    int thickness = scrollbarThickness();
    if (scrollbar.orientation() == HorizontalScrollbar)
        return IntRect(scrollbar.x(), scrollbar.y(),
                       scrollbar.width() < 2 * thickness ? scrollbar.width() / 2 : thickness, thickness);
    return IntRect(scrollbar.x(), scrollbar.y(),
                   thickness, scrollbar.height() < 2 * thickness ? scrollbar.height() / 2 : thickness);
}

IntRect ScrollbarThemeBal::forwardButtonRect(Scrollbar& scrollbar, ScrollbarPart part, bool)
{
    if (part == ForwardButtonStartPart)
        return IntRect();
    
    // Our desired rect is essentially 17x17.

    // Our actual rect will shrink to half the available space when
    // we have < 34 pixels left.  This allows the scrollbar
    // to scale down and function even at tiny sizes.
    int thickness = scrollbarThickness();
    if (scrollbar.orientation() == HorizontalScrollbar) {
        int w = scrollbar.width() < 2 * thickness ? scrollbar.width() / 2 : thickness;
        return IntRect(scrollbar.x() + scrollbar.width() - w, scrollbar.y(), w, thickness);
    }

    int h = scrollbar.height() < 2 * thickness ? scrollbar.height() / 2 : thickness;
    return IntRect(scrollbar.x(), scrollbar.y() + scrollbar.height() - h, thickness, h);
}

IntRect ScrollbarThemeBal::trackRect(Scrollbar& scrollbar, bool)
{
    int thickness = scrollbarThickness();
    if (scrollbar.orientation() == HorizontalScrollbar) {
        if (scrollbar.width() < 2 * thickness)
            return IntRect();
        return IntRect(scrollbar.x() + thickness, scrollbar.y(), scrollbar.width() - 2 * thickness, thickness);
    }
    if (scrollbar.height() < 2 * thickness)
        return IntRect();
    return IntRect(scrollbar.x(), scrollbar.y() + thickness, thickness, scrollbar.height() - 2 * thickness);
}

void ScrollbarThemeBal::paintButton(GraphicsContext& context, Scrollbar& scrollbar, const IntRect& rect, ScrollbarPart part)
{
    context.save();
    bool start = (part == BackButtonStartPart);
    //RefPtr<WebCore::Image> left = Image::loadPlatformResource("ScrollbarTheme/left");
    if (!left->isNull()) {
        IntPoint startPos(rect.location());
        if (start) {
            if (scrollbar.orientation() == HorizontalScrollbar)
                context.drawImage(left.get(), ColorSpaceDeviceRGB, startPos);
            else {
	      //RefPtr<WebCore::Image> up = Image::loadPlatformResource("ScrollbarTheme/up");
                context.drawImage(up.get(), ColorSpaceDeviceRGB, startPos);
            }
        } else {
            if (scrollbar.orientation() == HorizontalScrollbar) {
	      //RefPtr<WebCore::Image> right = Image::loadPlatformResource("ScrollbarTheme/right");
                context.drawImage(right.get(), ColorSpaceDeviceRGB, startPos);
            } else {
	      //RefPtr<WebCore::Image> down = Image::loadPlatformResource("ScrollbarTheme/down");
                context.drawImage(down.get(), ColorSpaceDeviceRGB, startPos);
            }
        }
    } else {    
        context.drawRect(rect);
		context.fillRect(IntRect(rect.x() + 1, rect.y() + 1, rect.width() - 2, rect.height() - 2), Color::gray, ColorSpaceDeviceRGB);

        if (start) {
            if (scrollbar.orientation() == HorizontalScrollbar) {
                context.drawLine(IntPoint(rect.maxX(), rect.y()), IntPoint(rect.x() - 1, (rect.maxY() + rect.y())/2));
                context.drawLine(IntPoint(rect.maxX(), rect.maxY()), IntPoint(rect.x() - 1, (rect.maxY() + rect.y())/2));
            } else {
                context.drawLine(IntPoint(rect.x(), rect.maxY()), IntPoint((rect.x() + rect.maxX())/2, rect.y() + 1));
                context.drawLine(IntPoint(rect.maxX() - 1, rect.maxY()), IntPoint((rect.x() + rect.maxX())/2, rect.y() + 1));
            }
        } else {
            if (scrollbar.orientation() == HorizontalScrollbar) {
                context.drawLine(IntPoint(rect.x(), rect.y()), IntPoint(rect.maxX() - 1, (rect.maxY() + rect.y())/2));
                context.drawLine(IntPoint(rect.x(), rect.maxY()), IntPoint(rect.maxX() - 1, (rect.maxY() + rect.y())/2));
            } else {
                context.drawLine(IntPoint(rect.x(), rect.y()), IntPoint((rect.x() + rect.maxX())/2, rect.maxY() - 1));
                context.drawLine(IntPoint(rect.maxX(), rect.y()), IntPoint((rect.x() + rect.maxX())/2, rect.maxY() - 1));
            }
        }
    }
    context.restore();
}

void ScrollbarThemeBal::paintThumb(GraphicsContext& context, Scrollbar& scrollbar, const IntRect& rect)
{
    context.save();
    if (scrollbar.orientation() == HorizontalScrollbar) {
      //RefPtr<WebCore::Image> thumbH = Image::loadPlatformResource("ScrollbarTheme/thumbH");
        if (!thumbH->isNull()) {
	  //RefPtr<WebCore::Image> thumbHL = Image::loadPlatformResource("ScrollbarTheme/thumbHL");
	  //RefPtr<WebCore::Image> thumbHR = Image::loadPlatformResource("ScrollbarTheme/thumbHR");
            IntPoint startPos(rect.location());
            IntPoint endPos(rect.maxX() - thumbHR->width(), rect.y());
            IntRect destRect(rect.x() + thumbHL->width() - 1, rect.y(), rect.width() - thumbHR->width() - thumbHL->width() + 1, rect.height());

            context.drawImage(thumbHL.get(), ColorSpaceDeviceRGB, startPos);
            context.drawTiledImage(thumbH.get(), ColorSpaceDeviceRGB, destRect, IntPoint(0, 0), IntSize(thumbH->width(), thumbH->height()));
            context.drawImage(thumbHR.get(), ColorSpaceDeviceRGB, endPos);

        } else {
            context.drawRect(rect);
            context.fillRect(IntRect(rect.x() + 1, rect.y() + 1, rect.width() - 2, rect.height() - 2), Color::gray, ColorSpaceDeviceRGB);
        }
    } else {
      //RefPtr<WebCore::Image> thumbV = Image::loadPlatformResource("ScrollbarTheme/thumbV");
        if (!thumbV->isNull()) {
	  //RefPtr<WebCore::Image> thumbVU = Image::loadPlatformResource("ScrollbarTheme/thumbVU");
	  //RefPtr<WebCore::Image> thumbVD = Image::loadPlatformResource("ScrollbarTheme/thumbVD");
            IntPoint startPos(rect.location());
            IntPoint endPos(rect.x(), rect.maxY() - thumbVD->height());
            IntRect destRect(rect.x(), rect.y() + thumbVU->height(), rect.width(), rect.height() - thumbVU->height() - thumbVD->height());

            context.drawImage(thumbVU.get(), ColorSpaceDeviceRGB, startPos);
            context.drawTiledImage(thumbV.get(), ColorSpaceDeviceRGB, destRect, IntPoint(0, 0), IntSize(thumbV->width(), thumbV->height()));
            context.drawImage(thumbVD.get(), ColorSpaceDeviceRGB, endPos);
        } else {
            context.drawRect(rect);
            context.fillRect(IntRect(rect.x() + 1, rect.y() + 1, rect.width() - 2, rect.height() - 2), Color::gray, ColorSpaceDeviceRGB);
        }
    }
    context.restore();
}


void ScrollbarThemeBal::paintTrackBackground(GraphicsContext& context, Scrollbar& scrollbar, const IntRect& rect)
{
    // Just assume a forward track part.  We only paint the track as a single piece when there is no thumb.
    if (!hasThumb(scrollbar))
        paintTrackPiece(context, scrollbar, rect, ForwardTrackPart);
}

bool ScrollbarThemeBal::invalidateOnMouseEnterExit()
{
    return false;
}

void ScrollbarThemeBal::themeChanged()
{
}

bool ScrollbarThemeBal::shouldCenterOnThumb(Scrollbar&, const PlatformMouseEvent& evt)
{
    return evt.shiftKey() && evt.button() == LeftButton;
}

bool ScrollbarThemeBal::shouldSnapBackToDragOrigin(Scrollbar& scrollbar, const PlatformMouseEvent& evt)
{
    // Find the rect within which we shouldn't snap, by expanding the track rect
    // in both dimensions.
    IntRect rect = trackRect(scrollbar);
    const bool horz = scrollbar.orientation() == HorizontalScrollbar;
    const int thickness = scrollbarThickness(scrollbar.controlSize());
    rect.inflateX((horz ? kOffEndMultiplier : kOffSideMultiplier) * thickness);
    rect.inflateY((horz ? kOffSideMultiplier : kOffEndMultiplier) * thickness);

    // Convert the event to local coordinates.
    IntPoint mousePosition = scrollbar.convertFromContainingWindow(evt.position());
    mousePosition.move(scrollbar.x(), scrollbar.y());
    // We should snap iff the event is outside our calculated rect.
    return !rect.contains(mousePosition);
}

void ScrollbarThemeBal::paintTrackPiece(GraphicsContext& context, Scrollbar& scrollbar, const IntRect& rect, ScrollbarPart partType)
{
    UNUSED_PARAM(partType);
    context.save();
    
    //RefPtr<WebCore::Image> bg = Image::loadPlatformResource("ScrollbarTheme/bg");
    if (!bg->isNull()) {
        if (scrollbar.orientation() == HorizontalScrollbar) {
	  //RefPtr<WebCore::Image> bgh = Image::loadPlatformResource("ScrollbarTheme/bgh");
            IntRect destRect(rect.x() - 1, rect.y(), rect.width() + 2, rect.height());
            context.drawTiledImage(bgh.get(), ColorSpaceDeviceRGB, destRect, IntPoint(0, 0), IntSize(bgh->width(), bgh->height()));
        } else {
            IntRect destRect(rect.x(), rect.y() - 1, rect.width(), rect.height() + 2 );
			context.drawTiledImage(bg.get(), ColorSpaceDeviceRGB, destRect, IntPoint(0, 0), IntSize(bg->width(), bg->height()));
        }
    } else {
		context.fillRect(rect, Color::white, ColorSpaceDeviceRGB);
        if (scrollbar.orientation() == HorizontalScrollbar)
            context.drawLine(IntPoint(rect.x(), (rect.maxY() + rect.y()) / 2), IntPoint(rect.maxX(), (rect.maxY() + rect.y()) / 2));
        else
            context.drawLine(IntPoint((rect.maxX() + rect.x()) / 2, rect.y()), IntPoint((rect.maxX() + rect.x()) / 2, rect.maxY()));
    }
    context.restore();
}


}
