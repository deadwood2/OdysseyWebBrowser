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

#ifndef ScrollbarThemeBal_h
#define ScrollbarThemeBal_h

#include "ScrollbarThemeComposite.h"

namespace WebCore {

class ScrollbarThemeBal : public ScrollbarThemeComposite {
public:
    ScrollbarThemeBal() 
      {
	/*
	if(!m_loaded)
	  {
	    up = Image::loadPlatformResource("ScrollbarTheme/up");
	    down = Image::loadPlatformResource("ScrollbarTheme/down");
	    left = Image::loadPlatformResource("ScrollbarTheme/left");
	    right = Image::loadPlatformResource("ScrollbarTheme/right");
	    thumbH = Image::loadPlatformResource("ScrollbarTheme/thumbH");
	    thumbHL = Image::loadPlatformResource("ScrollbarTheme/thumbHL");
	    thumbHR = Image::loadPlatformResource("ScrollbarTheme/thumbHR");
	    thumbV = Image::loadPlatformResource("ScrollbarTheme/thumbV");
	    thumbVU = Image::loadPlatformResource("ScrollbarTheme/thumbVU");
	    thumbVD = Image::loadPlatformResource("ScrollbarTheme/thumbVD");
	    bg = Image::loadPlatformResource("ScrollbarTheme/bg");
	    bgh = Image::loadPlatformResource("ScrollbarTheme/bgh");

	    m_loaded = true;
	
	  }
	*/
      };
    virtual ~ScrollbarThemeBal();

    virtual int scrollbarThickness(ScrollbarControlSize = RegularScrollbar);

    virtual void themeChanged();
    
    virtual bool invalidateOnMouseEnterExit();
    
protected:
    virtual bool hasButtons(ScrollbarThemeClient*) { return true; }
    virtual bool hasThumb(ScrollbarThemeClient*);

    virtual IntRect backButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool painting = false);
    virtual IntRect forwardButtonRect(ScrollbarThemeClient*, ScrollbarPart, bool painting = false);
    virtual IntRect trackRect(ScrollbarThemeClient*, bool painting = false);

    virtual bool shouldCenterOnThumb(ScrollbarThemeClient*, const PlatformMouseEvent&);
    virtual bool shouldSnapBackToDragOrigin(ScrollbarThemeClient*, const PlatformMouseEvent&);

    virtual void paintTrackBackground(GraphicsContext*, ScrollbarThemeClient*, const IntRect&);
    virtual void paintTrackPiece(GraphicsContext*, ScrollbarThemeClient*, const IntRect&, ScrollbarPart);
    virtual void paintButton(GraphicsContext*, ScrollbarThemeClient*, const IntRect&, ScrollbarPart);
    virtual void paintThumb(GraphicsContext*, ScrollbarThemeClient*, const IntRect&);

private:
    static bool m_loaded;
    static RefPtr<WebCore::Image> left, right, up, down, thumbH, thumbHL, thumbHR, thumbV, thumbVU, thumbVD, bg, bgh;

};

}
#endif
