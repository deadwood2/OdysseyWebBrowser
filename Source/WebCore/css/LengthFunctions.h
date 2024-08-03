/*
    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
    Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2011 Rik Cabanier (cabanier@adobe.com)
    Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
    Copyright (C) 2012 Motorola Mobility, Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef LengthFunctions_h
#define LengthFunctions_h

#include "LayoutUnit.h"
#include "Length.h"

namespace WebCore {

class FloatSize;
class LayoutUnit;
class RenderView;
struct Length;
struct LengthSize;

int minimumIntValueForLength(const Length&, LayoutUnit maximumValue);
int intValueForLength(const Length&, LayoutUnit maximumValue);
LayoutUnit minimumValueForLength(const Length&, LayoutUnit maximumValue);
WEBCORE_EXPORT LayoutUnit valueForLength(const Length&, LayoutUnit maximumValue);
float floatValueForLength(const Length&, LayoutUnit maximumValue);
WEBCORE_EXPORT float floatValueForLength(const Length&, float maximumValue);
FloatSize floatSizeForLengthSize(const LengthSize&, const FloatSize&);

inline LayoutUnit minimumValueForLength(const Length& length, LayoutUnit maximumValue)
{
    switch (length.type()) {
    case Fixed:
        return length.value();
    case Percent:
        // Don't remove the extra cast to float. It is needed for rounding on 32-bit Intel machines that use the FPU stack.
        return LayoutUnit(static_cast<float>(maximumValue * length.percent() / 100.0f));
    case Calculated:
        return length.nonNanCalculatedValue(maximumValue);
    case FillAvailable:
    case Auto:
        return 0;
    case Relative:
    case Intrinsic:
    case MinIntrinsic:
    case MinContent:
    case MaxContent:
    case FitContent:
    case Undefined:
        break;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

inline int minimumIntValueForLength(const Length& length, LayoutUnit maximumValue)
{
    return static_cast<int>(minimumValueForLength(length, maximumValue));
}

} // namespace WebCore

#endif // LengthFunctions_h
