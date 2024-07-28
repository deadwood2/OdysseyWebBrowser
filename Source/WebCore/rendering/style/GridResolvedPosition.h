/*
 * Copyright (C) 2014 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GridResolvedPosition_h
#define GridResolvedPosition_h

#if ENABLE(CSS_GRID_LAYOUT)

#include "GridPosition.h"

namespace WebCore {

class GridSpan;
class RenderBox;
class RenderStyle;

enum GridTrackSizingDirection {
    ForColumns,
    ForRows
};

// This class represents a line index into one of the dimensions of the grid array.
// Wraps an unsigned integer just for the purpose of knowing what we manipulate in the grid code.
class GridResolvedPosition {
public:
    GridResolvedPosition(unsigned position)
        : m_integerPosition(position)
    {
    }

    GridResolvedPosition& operator*()
    {
        return *this;
    }

    GridResolvedPosition& operator++()
    {
        m_integerPosition++;
        return *this;
    }

    bool operator==(const GridResolvedPosition& other) const
    {
        return m_integerPosition == other.m_integerPosition;
    }

    bool operator!=(const GridResolvedPosition& other) const
    {
        return m_integerPosition != other.m_integerPosition;
    }

    bool operator<(const GridResolvedPosition& other) const
    {
        return m_integerPosition < other.m_integerPosition;
    }

    bool operator>(const GridResolvedPosition& other) const
    {
        return m_integerPosition > other.m_integerPosition;
    }

    bool operator<=(const GridResolvedPosition& other) const
    {
        return m_integerPosition <= other.m_integerPosition;
    }

    bool operator>=(const GridResolvedPosition& other) const
    {
        return m_integerPosition >= other.m_integerPosition;
    }

    unsigned toInt() const
    {
        return m_integerPosition;
    }

    GridResolvedPosition next() const
    {
        return GridResolvedPosition(m_integerPosition + 1);
    }

    GridResolvedPosition prev() const
    {
        return m_integerPosition ? m_integerPosition - 1 : 0;
    }

    static GridSpan resolveGridPositionsFromAutoPlacementPosition(const RenderStyle&, const RenderBox&, GridTrackSizingDirection, const GridResolvedPosition&);
    static GridSpan resolveGridPositionsFromStyle(const RenderStyle&, const RenderBox&, GridTrackSizingDirection);
    static unsigned explicitGridColumnCount(const RenderStyle&);
    static unsigned explicitGridRowCount(const RenderStyle&);
    static bool isNonExistentNamedLineOrArea(const String& lineName, const RenderStyle&, GridPositionSide);

private:
    unsigned m_integerPosition;
};

} // namespace WebCore

#endif // ENABLE(CSS_GRID_LAYOUT)

#endif // GridResolvedPosition_h
