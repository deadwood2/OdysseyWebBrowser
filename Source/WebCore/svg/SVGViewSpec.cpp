/*
 * Copyright (C) 2007, 2010 Rob Buis <buis@kde.org>
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

#include "config.h"
#include "SVGViewSpec.h"

#include "Document.h"
#include "SVGAnimatedTransformList.h"
#include "SVGElement.h"
#include "SVGFitToViewBox.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGTransformList.h"
#include "SVGTransformable.h"

namespace WebCore {

// Define custom animated property 'viewBox'.
const SVGPropertyInfo* SVGViewSpec::viewBoxPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = nullptr;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedRect,
                                             PropertyIsReadOnly,
                                             SVGNames::viewBoxAttr,
                                             viewBoxIdentifier(),
                                             0,
                                             0);
    }
    return s_propertyInfo;
}

// Define custom animated property 'preserveAspectRatio'.
const SVGPropertyInfo* SVGViewSpec::preserveAspectRatioPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = nullptr;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedPreserveAspectRatio,
                                             PropertyIsReadOnly,
                                             SVGNames::preserveAspectRatioAttr,
                                             preserveAspectRatioIdentifier(),
                                             0,
                                             0);
    }
    return s_propertyInfo;
}


// Define custom non-animated property 'transform'.
const SVGPropertyInfo* SVGViewSpec::transformPropertyInfo()
{
    static const SVGPropertyInfo* s_propertyInfo = nullptr;
    if (!s_propertyInfo) {
        s_propertyInfo = new SVGPropertyInfo(AnimatedTransformList,
                                             PropertyIsReadOnly,
                                             SVGNames::transformAttr,
                                             transformIdentifier(),
                                             0,
                                             0);
    }
    return s_propertyInfo;
}

SVGViewSpec::SVGViewSpec(SVGElement& contextElement)
    : m_contextElement(&contextElement)
{
}

const AtomicString& SVGViewSpec::viewBoxIdentifier()
{
    static NeverDestroyed<AtomicString> s_identifier("SVGViewSpecViewBoxAttribute", AtomicString::ConstructFromLiteral);
    return s_identifier;
}

const AtomicString& SVGViewSpec::preserveAspectRatioIdentifier()
{
    static NeverDestroyed<AtomicString> s_identifier("SVGViewSpecPreserveAspectRatioAttribute", AtomicString::ConstructFromLiteral);
    return s_identifier;
}

const AtomicString& SVGViewSpec::transformIdentifier()
{
    static NeverDestroyed<AtomicString> s_identifier("SVGViewSpecTransformAttribute", AtomicString::ConstructFromLiteral);
    return s_identifier;
}

ExceptionOr<void> SVGViewSpec::setZoomAndPan(unsigned short)
{
    // SVGViewSpec and all of its content is read-only.
    return Exception { NO_MODIFICATION_ALLOWED_ERR };
}

String SVGViewSpec::transformString() const
{
    return SVGPropertyTraits<SVGTransformListValues>::toString(m_transform);
}

String SVGViewSpec::viewBoxString() const
{
    return SVGPropertyTraits<FloatRect>::toString(m_viewBox);
}

String SVGViewSpec::preserveAspectRatioString() const
{
    return SVGPropertyTraits<SVGPreserveAspectRatioValue>::toString(m_preserveAspectRatio);
}

SVGElement* SVGViewSpec::viewTarget() const
{
    if (!m_contextElement)
        return nullptr;
    auto* element = m_contextElement->treeScope().getElementById(m_viewTargetString);
    if (!is<SVGElement>(element))
        return nullptr;
    return downcast<SVGElement>(element);
}

RefPtr<SVGTransformList> SVGViewSpec::transform()
{
    if (!m_contextElement)
        return nullptr;
    // Return the animVal here, as its readonly by default - which is exactly what we want here.
    return static_reference_cast<SVGAnimatedTransformList>(lookupOrCreateTransformWrapper(this))->animVal();
}

RefPtr<SVGAnimatedRect> SVGViewSpec::viewBoxAnimated()
{
    if (!m_contextElement)
        return nullptr;
    return static_reference_cast<SVGAnimatedRect>(lookupOrCreateViewBoxWrapper(this));
}

RefPtr<SVGAnimatedPreserveAspectRatio> SVGViewSpec::preserveAspectRatioAnimated()
{
    if (!m_contextElement)
        return nullptr;
    return static_reference_cast<SVGAnimatedPreserveAspectRatio>(lookupOrCreatePreserveAspectRatioWrapper(this));
}

Ref<SVGAnimatedProperty> SVGViewSpec::lookupOrCreateViewBoxWrapper(SVGViewSpec* ownerType)
{
    ASSERT(ownerType);
    ASSERT(ownerType->m_contextElement);
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGElement, SVGAnimatedRect, FloatRect>(ownerType->m_contextElement, viewBoxPropertyInfo(), ownerType->m_viewBox);
}

Ref<SVGAnimatedProperty> SVGViewSpec::lookupOrCreatePreserveAspectRatioWrapper(SVGViewSpec* ownerType)
{
    ASSERT(ownerType);
    ASSERT(ownerType->m_contextElement);
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGElement, SVGAnimatedPreserveAspectRatio, SVGPreserveAspectRatioValue>(ownerType->m_contextElement, preserveAspectRatioPropertyInfo(), ownerType->m_preserveAspectRatio);
}

Ref<SVGAnimatedProperty> SVGViewSpec::lookupOrCreateTransformWrapper(SVGViewSpec* ownerType)
{
    ASSERT(ownerType);
    ASSERT(ownerType->m_contextElement);
    return SVGAnimatedProperty::lookupOrCreateWrapper<SVGElement, SVGAnimatedTransformList, SVGTransformListValues>(ownerType->m_contextElement, transformPropertyInfo(), ownerType->m_transform);
}

void SVGViewSpec::reset()
{
    m_zoomAndPan = SVGZoomAndPanMagnify;
    m_transform.clear();
    m_viewBox = { };
    m_preserveAspectRatio = { };
    m_viewTargetString = emptyString();
}

static const UChar svgViewSpec[] = {'s', 'v', 'g', 'V', 'i', 'e', 'w'};
static const UChar viewBoxSpec[] = {'v', 'i', 'e', 'w', 'B', 'o', 'x'};
static const UChar preserveAspectRatioSpec[] = {'p', 'r', 'e', 's', 'e', 'r', 'v', 'e', 'A', 's', 'p', 'e', 'c', 't', 'R', 'a', 't', 'i', 'o'};
static const UChar transformSpec[] = {'t', 'r', 'a', 'n', 's', 'f', 'o', 'r', 'm'};
static const UChar zoomAndPanSpec[] = {'z', 'o', 'o', 'm', 'A', 'n', 'd', 'P', 'a', 'n'};
static const UChar viewTargetSpec[] =  {'v', 'i', 'e', 'w', 'T', 'a', 'r', 'g', 'e', 't'};

bool SVGViewSpec::parseViewSpec(const String& viewSpec)
{
    auto upconvertedCharacters = StringView(viewSpec).upconvertedCharacters();
    const UChar* currViewSpec = upconvertedCharacters;
    const UChar* end = currViewSpec + viewSpec.length();

    if (currViewSpec >= end || !m_contextElement)
        return false;

    if (!skipString(currViewSpec, end, svgViewSpec, WTF_ARRAY_LENGTH(svgViewSpec)))
        return false;

    if (currViewSpec >= end || *currViewSpec != '(')
        return false;
    currViewSpec++;

    while (currViewSpec < end && *currViewSpec != ')') {
        if (*currViewSpec == 'v') {
            if (skipString(currViewSpec, end, viewBoxSpec, WTF_ARRAY_LENGTH(viewBoxSpec))) {
                if (currViewSpec >= end || *currViewSpec != '(')
                    return false;
                currViewSpec++;
                FloatRect viewBox;
                if (!SVGFitToViewBox::parseViewBox(&m_contextElement->document(), currViewSpec, end, viewBox, false))
                    return false;
                setViewBoxBaseValue(viewBox);
                if (currViewSpec >= end || *currViewSpec != ')')
                    return false;
                currViewSpec++;
            } else if (skipString(currViewSpec, end, viewTargetSpec, WTF_ARRAY_LENGTH(viewTargetSpec))) {
                if (currViewSpec >= end || *currViewSpec != '(')
                    return false;
                const UChar* viewTargetStart = ++currViewSpec;
                while (currViewSpec < end && *currViewSpec != ')')
                    currViewSpec++;
                if (currViewSpec >= end)
                    return false;
                m_viewTargetString = String(viewTargetStart, currViewSpec - viewTargetStart);
                currViewSpec++;
            } else
                return false;
        } else if (*currViewSpec == 'z') {
            if (!skipString(currViewSpec, end, zoomAndPanSpec, WTF_ARRAY_LENGTH(zoomAndPanSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            if (!parse(currViewSpec, end, m_zoomAndPan))
                return false;
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else if (*currViewSpec == 'p') {
            if (!skipString(currViewSpec, end, preserveAspectRatioSpec, WTF_ARRAY_LENGTH(preserveAspectRatioSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            SVGPreserveAspectRatioValue preserveAspectRatio;
            if (!preserveAspectRatio.parse(currViewSpec, end, false))
                return false;
            setPreserveAspectRatioBaseValue(preserveAspectRatio);
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else if (*currViewSpec == 't') {
            if (!skipString(currViewSpec, end, transformSpec, WTF_ARRAY_LENGTH(transformSpec)))
                return false;
            if (currViewSpec >= end || *currViewSpec != '(')
                return false;
            currViewSpec++;
            SVGTransformable::parseTransformAttribute(m_transform, currViewSpec, end, SVGTransformable::DoNotClearList);
            if (currViewSpec >= end || *currViewSpec != ')')
                return false;
            currViewSpec++;
        } else
            return false;

        if (currViewSpec < end && *currViewSpec == ';')
            currViewSpec++;
    }
    
    if (currViewSpec >= end || *currViewSpec != ')')
        return false;

    return true;
}

}
