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
#include "Cursor.h"
#include <intuition/pointerclass.h>

namespace WebCore {

void Cursor::ensurePlatformCursor() const
{
    if (m_platformCursor || m_type == Type::Pointer)
        return;

    switch (m_type) {
    case Type::Pointer:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    case Type::Cross:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_AIMING);
        break;
    case Type::Hand:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_SELECTLINK);
        break;
    case Type::IBeam:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_SELECTTEXT);
        break;
    case Type::Wait:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_BUSY);
        break;
    case Type::Help:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_HELP);
        break;
    case Type::Move:
    case Type::MiddlePanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_MOVE);
        break;
    case Type::EastResize:
    case Type::EastPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_HORIZONTALRESIZE);
        break;
    case Type::NorthResize:
    case Type::NorthPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_VERTICALRESIZE);
        break;
    case Type::NorthEastResize:
    case Type::NorthEastPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_DIAGONALRESIZE1);
        break;
    case Type::NorthWestResize:
    case Type::NorthWestPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_DIAGONALRESIZE2);
        break;
    case Type::SouthResize:
    case Type::SouthPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_VERTICALRESIZE);
        break;
    case Type::SouthEastResize:
    case Type::SouthEastPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_DIAGONALRESIZE1);
        break;
    case Type::SouthWestResize:
    case Type::SouthWestPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_DIAGONALRESIZE2);
        break;
    case Type::WestResize:
    case Type::WestPanning:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_HORIZONTALRESIZE);
        break;
    case Type::NorthSouthResize:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_VERTICALRESIZE);
        break;
    case Type::EastWestResize:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_HORIZONTALRESIZE);
        break;
    case Type::NorthEastSouthWestResize:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_DIAGONALRESIZE1);
        break;
    case Type::NorthWestSouthEastResize:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_DIAGONALRESIZE1);
        break;
    case Type::ColumnResize:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_HORIZONTALRESIZE);
        break;
    case Type::RowResize:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_HORIZONTALRESIZE);
        break;
    case Type::VerticalText:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_SELECTTEXT);
        break;
    case Type::Cell:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_SELECTTEXT);
        break;
    case Type::ContextMenu:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    case Type::Alias:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    case Type::Progress:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_WORKING);
        break;
    case Type::NoDrop:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NOTAVAILABLE);
        break;
    case Type::NotAllowed:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NOTAVAILABLE);
        break;
    case Type::Copy:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    case Type::None:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_DOT);
        break;
    case Type::ZoomIn:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    case Type::ZoomOut:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    case Type::Grab:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    case Type::Grabbing:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_MOVE);
        break;
    case Type::Custom:
        m_platformCursor = WebCore::PlatformCursor(POINTERTYPE_NORMAL);
        break;
    }
}

}

