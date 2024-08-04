/*
 * Copyright (C) 2004, 2005, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "SVGAnimatedNumber.h"
#include "SVGElement.h"

namespace WebCore {

class SVGStopElement final : public SVGElement {
public:
    static Ref<SVGStopElement> create(const QualifiedName&, Document&);

    Color stopColorIncludingOpacity() const;

private:
    SVGStopElement(const QualifiedName&, Document&);

    void parseAttribute(const QualifiedName&, const AtomicString&) final;
    void svgAttributeChanged(const QualifiedName&) final;

    bool isGradientStop() const final { return true; }

    RenderPtr<RenderElement> createElementRenderer(RenderStyle&&, const RenderTreePosition&) final;
    bool rendererIsNeeded(const RenderStyle&) final;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGStopElement)
        DECLARE_ANIMATED_NUMBER(Offset, offset)
    END_DECLARE_ANIMATED_PROPERTIES
};

} // namespace WebCore
