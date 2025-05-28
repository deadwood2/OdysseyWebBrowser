/*
 * Copyright (C) 2020 Jacek Piszczek
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
#include "PlatformScreen.h"
#include "Widget.h"

#include "FloatRect.h"
#include "NotImplemented.h"

#include <proto/intuition.h>
#include <clib/alib_protos.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

namespace WebCore {

int screenDepth(Widget* widget)
{
	Boopsiobject *area = static_cast<Boopsiobject *>(widget->platformWidget());

	if (area && muiRenderInfo(area))
	{
		Boopsiobject *screen = reinterpret_cast<Boopsiobject *>(muiRenderInfo(area)->mri_Screen);
		if (screen)
		{
			ULONG depth = 32;
			DoMethod(screen, OM_GET, SA_Depth, &depth);
			return depth;
		}
	}

    notImplemented();
    return 32;
}

int screenDepthPerComponent(Widget*)
{
    notImplemented();
    return 8;
}

bool screenIsMonochrome(Widget*)
{
    return false;
}

bool screenHasInvertedColors()
{
    return false;
}

FloatRect screenRect(Widget* widget)
{
	Boopsiobject *area = static_cast<Boopsiobject *>(widget->platformWidget());

	if (area && muiRenderInfo(area))
	{
		Boopsiobject *screen = reinterpret_cast<Boopsiobject *>(muiRenderInfo(area)->mri_Screen);
		if (screen)
		{
			ULONG w = 1920, h = 1080;
			DoMethod(screen, OM_GET, SA_Width, &w);
			DoMethod(screen, OM_GET, SA_Height, &h);
			return { 0, 0, w, h };
		}
	}
	
    notImplemented();
	return { 0, 0, 1920, 1080 };
}

FloatRect screenAvailableRect(Widget* widget)
{
	return screenRect(widget);
}

bool screenSupportsExtendedColor(Widget*)
{
    return false;
}

} // namespace WebCore
