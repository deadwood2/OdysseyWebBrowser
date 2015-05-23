/*
 * Copyright (C) 2009-2013 Fabien Coeurjoly.
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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


#include "config.h"
#include "PlatformScreen.h"
#include <wtf/Assertions.h>
#include "Logging.h"
#include "HostWindow.h"
#include "ScrollView.h"
#include "Widget.h"
#include "FrameView.h"
#include "FloatRect.h"
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <proto/alib.h>
#include <utility/tagitem.h>
#include <aros-overrides.h>


namespace WebCore {

int screenHorizontalDPI(Widget* widget) 
{
    UNUSED_PARAM(widget);
    return 0; 
}

int screenVerticalDPI(Widget* widget)
{
    UNUSED_PARAM(widget);
    return 0;
}

int screenDepth(Widget* widget)
{
    UNUSED_PARAM(widget);
    int depth = 32;
    #warning "screenDepth disabled"
#if 0
    if (widget->root()->hostWindow()->platformPageClient()) {
		Object *window = widget->root()->hostWindow()->platformPageClient()->window;
        if (window)
         {
	   struct Window* win;
	   GetAttr(MUIA_Window_Window, window, (ULONG*)&win);  
	   
	   if(win)
	     {
	       depth = GetCyberMapAttr(win->WScreen->RastPort.BitMap, CYBRMATTR_DEPTH);
	     }
	 }
    }
#endif
    return depth;
}

int screenDepthPerComponent(Widget* widget)
{
    UNUSED_PARAM(widget);
    int depth = 8;
    #warning "screenDepthPerComponent disabled"
#if 0
    if (widget->root()->hostWindow()->platformPageClient()) {
        unsigned long id = INVALID_ID;
        struct DisplayInfo displayInfo;

        Object *window = widget->root()->hostWindow()->platformPageClient()->window;
        if (window)
	  {
	    struct Window* win;
	    GetAttr(MUIA_Window_Window, window, (ULONG*)&win);

	    if(win)
	      {
		GetAttr((Object *)win->WScreen, SA_DisplayID, &id);
	      }
	  }
        if (INVALID_ID != id
         && GetDisplayInfoData(NULL, &displayInfo, sizeof(displayInfo), DTAG_DISP, id) >= 48)
            depth = (displayInfo.RedBits + displayInfo.GreenBits + displayInfo.BlueBits) / 3;
    }
#endif
    return depth;
}

bool screenIsMonochrome(Widget* widget)
{
    return screenDepth(widget) < 2;
}

FloatRect screenRect(Widget* widget)
{
    int x = 0, y = 0, width = 800, height = 600;

	if (widget->root()->hostWindow()->platformPageClient())
	{
		Object* window = widget->root()->hostWindow()->platformPageClient()->window;
        if (window)
		{
		    struct Window* win;
		    GetAttr(MUIA_Window_Window, window, (ULONGPTR)&win);

			if(win)
			{
				x = win->WScreen->LeftEdge;
				y = win->WScreen->TopEdge;
				width = win->WScreen->Width;
				height = win->WScreen->Height;
			}
		}
    }

    return FloatRect(x, y, width, height);
}

FloatRect screenAvailableRect(Widget* widget)
{
    return screenRect(widget);
}

void screenColorProfile(ColorProfile&)
{
}

bool screenHasInvertedColors()
{
    return false;
}

} // namespace WebCore
