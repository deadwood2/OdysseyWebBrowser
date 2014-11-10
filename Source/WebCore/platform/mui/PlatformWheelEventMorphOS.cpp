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
#include "PlatformWheelEvent.h"

#include <wtf/CurrentTime.h>
#include "Scrollbar.h"
#include <cstdio>
#include <intuition/intuition.h>
#include <devices/rawkeycodes.h>
#include <devices/inputevent.h>
#include <clib/debug_protos.h>

namespace WebCore {

// Keep this in sync with the other platform event constructors
PlatformWheelEvent::PlatformWheelEvent(BalEventScroll* event)
{
    double deltax = 0.0, deltay = 0.0;

    m_deltaX = 0;
    m_deltaY = 0;
    m_useLatchedEventNode = false;

	m_modifiers = 0;

	if (event->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
		m_modifiers |= ShiftKey;

	if (event->Qualifier & IEQUALIFIER_CONTROL)
		m_modifiers |= CtrlKey;

	if( event->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
		m_modifiers |= AltKey;

	if (event->Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))
		m_modifiers |= MetaKey;

	switch(event->Code)
	{
		case RAWKEY_NM_WHEEL_UP:
			deltay = +1;
			break;
		case RAWKEY_NM_WHEEL_DOWN:
			deltay = -1;
			break;
		case RAWKEY_NM_WHEEL_LEFT:
			deltax = +1;
			break;
		case RAWKEY_NM_WHEEL_RIGHT:
			deltax = -1;
			break;
	}

    m_deltaX = deltax;
    m_deltaY = deltay;

    m_wheelTicksX = m_deltaX;
    m_wheelTicksY = m_deltaY;

	if (ctrlKey())
	{
		m_granularity = ScrollByPageWheelEvent;
        m_granularity = ScrollByPageWheelEvent;
		/*
		deltax = deltax * 10000;
		deltay = deltay * 10000;
		*/
		deltax = deltay = 0; // Keep ctrl for zoom
	}
	else if (shiftKey())
	{
		// kprintf("window (%d, %d)\n", event->IDCMPWindow->Width, event->IDCMPWindow->Height);
        m_granularity = ScrollByPageWheelEvent;
		deltax = (int)deltax * event->IDCMPWindow->Width / 50;
		deltay = (int)deltay * event->IDCMPWindow->Height / 50;
	}
	else
	{
		m_granularity = ScrollByPixelWheelEvent;

		deltax *= static_cast<float>(Scrollbar::pixelsPerLineStep());
		deltay *= static_cast<float>(Scrollbar::pixelsPerLineStep());
    }

	if (altKey())
    {
        double temp;
        temp   = deltax;
        deltax = deltay;
        deltay = temp;

		temp = m_wheelTicksX;
		m_wheelTicksY = m_wheelTicksX;
		m_wheelTicksX = temp;
    }

    m_deltaX = deltax;
    m_deltaY = deltay;

	m_type = PlatformEvent::Wheel;
	
	m_timestamp = WTF::currentTime();
    m_position = IntPoint((int)event->MouseX, (int)event->MouseY);
    m_globalPosition = IntPoint((int)event->MouseX, (int)event->MouseY);
    m_directionInvertedFromDevice = false;}

}
