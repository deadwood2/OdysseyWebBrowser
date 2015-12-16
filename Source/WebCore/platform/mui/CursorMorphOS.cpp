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
#include "Cursor.h"
#include <wtf/Assertions.h>
#include "Logging.h"

#include <cstdio>
#include <string.h>
#include <proto/intuition.h>
#include <intuition/pointerclass.h>

namespace WebCore {

Cursor::Cursor(const Cursor & other)
  : m_platformCursor(other.m_platformCursor)
{
	//fprintf(stderr, "%s (%p)\n", __PRETTY_FUNCTION__, c);
}

Cursor::~Cursor()
{

}

void Cursor::ensurePlatformCursor() const
{
#if !OS(AROS)
	ULONG pointertype;

	switch(m_type)
	{
		default:
		case Pointer:
			pointertype = POINTERTYPE_NORMAL;
			break;
		case Hand:
			pointertype = POINTERTYPE_SELECTLINK;
			break;
		case Cross:
			pointertype = POINTERTYPE_AIMING;
			break;
		case Wait:
			pointertype = POINTERTYPE_BUSY;
			break;
		case Help:
			pointertype = POINTERTYPE_HELP;
			break;
		case IBeam:
		case VerticalText:
			pointertype = POINTERTYPE_SELECTTEXT;
			break;
		case Move:
		case Grab:
		case Grabbing:
			pointertype = POINTERTYPE_MOVE;
			break;
		case EastResize:
		case WestResize:
		case EastWestResize:
		case ColumnResize:
			pointertype = POINTERTYPE_HORIZONTALRESIZE;
			break;
		case NorthResize:
		case SouthResize:
		case NorthSouthResize:
		case RowResize:
			pointertype = POINTERTYPE_VERTICALRESIZE;
			break;
		case NorthEastResize:
		case SouthWestResize:
		case NorthEastSouthWestResize:
			pointertype = POINTERTYPE_DIAGONALRESIZE1;
			break;
		case SouthEastResize:
		case NorthWestResize:
		case NorthWestSouthEastResize:
			pointertype = POINTERTYPE_DIAGONALRESIZE2;
			break;
		case Progress:
			pointertype = POINTERTYPE_WORKING;
			break;
		case NoDrop:
		case NotAllowed:
			pointertype = POINTERTYPE_NOTAVAILABLE;
			break;
	}

	m_platformCursor = (void *) pointertype;
#endif
}

Cursor& Cursor::operator=(const Cursor& other)
{
	//fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
	m_platformCursor = other.m_platformCursor;
	return *this;
}

}
