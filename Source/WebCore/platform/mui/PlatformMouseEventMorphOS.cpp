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
#include "PlatformMouseEvent.h"

#include <wtf/CurrentTime.h>
#include <wtf/Assertions.h>
#include <math.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <clib/debug_protos.h>

namespace WebCore {

static double lastclick = 0;
static bool isdouble = false;

// Keep this in sync with the other platform event constructors
PlatformMouseEvent::PlatformMouseEvent(BalEventButton *event)
{
    //printf("PlatformMouseEvent eventbutton x=%d y=%d\n", (int)event->x, (int)event->y);
    m_timestamp = WTF::currentTime();
    m_position = IntPoint((int)event->MouseX, (int)event->MouseY);
    m_globalPosition = IntPoint((int)event->MouseX, (int)event->MouseY);

	m_modifiers = 0;

	if (event->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
		m_modifiers |= ShiftKey;

	if (event->Qualifier & IEQUALIFIER_CONTROL)
		m_modifiers |= CtrlKey;

	if( event->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
		m_modifiers |= AltKey;

	if (event->Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))
		m_modifiers |= MetaKey;

	m_type = PlatformEvent::MouseMoved;
    m_button = NoButton;
    m_clickCount = 0;

	if (IDCMP_MOUSEBUTTONS == event->Class)
	{
		if ((event->Code & ~IECODE_UP_PREFIX) == IECODE_LBUTTON)
           m_button = LeftButton;
        else if ((event->Code & ~IECODE_UP_PREFIX) == IECODE_MBUTTON)
            m_button = MiddleButton;
        else if ((event->Code & ~IECODE_UP_PREFIX) == IECODE_RBUTTON)
            m_button = RightButton;

		if (NoButton != m_button)
		{
			if (event->Code & IECODE_UP_PREFIX)
			{
				m_type = PlatformEvent::MouseReleased;
                m_clickCount = 0;
			}
			else
			{
				// sucky code :)
				if(event->Code == IECODE_LBUTTON && DoubleClick((ULONG) lastclick, (ULONG) ((lastclick - floor(lastclick)) * 1000000), (ULONG) m_timestamp, (ULONG) ((m_timestamp - floor(m_timestamp)) * 1000000)))
				{
					if(isdouble)
						m_clickCount = 3;
					else
						m_clickCount = 2;
					
					isdouble = true;
				}
				else
				{
					m_clickCount = 1;
					isdouble = false;
				}

				m_type = PlatformEvent::MousePressed;

				if(event->Code == IECODE_LBUTTON)
					lastclick = WTF::currentTime();
            }
        }
    }
	else
	{ // IDCMP_MOUSEMOVE == event->Class
        if (event->Qualifier & IEQUALIFIER_LEFTBUTTON)
            m_button = LeftButton;
        else if (event->Qualifier & IEQUALIFIER_MIDBUTTON)
            m_button = MiddleButton;
        else if (event->Qualifier & IEQUALIFIER_RBUTTON)
            m_button = RightButton;
    }
}

}
