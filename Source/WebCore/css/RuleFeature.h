/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003-2011, 2014 Apple Inc. All rights reserved.
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
 *
 */

#pragma once

#include "CSSSelector.h"
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class RuleData;
class StyleRule;

struct RuleFeature {
    RuleFeature(StyleRule* rule, unsigned selectorIndex, bool hasDocumentSecurityOrigin)
        : rule(rule)
        , selectorIndex(selectorIndex)
        , hasDocumentSecurityOrigin(hasDocumentSecurityOrigin) 
    { 
    }
    StyleRule* rule;
    unsigned selectorIndex;
    bool hasDocumentSecurityOrigin;
};

struct RuleFeatureSet {
    void add(const RuleFeatureSet&);
    void clear();
    void shrinkToFit();
    void collectFeatures(const RuleData&);

    HashSet<AtomicString> idsInRules;
    HashSet<AtomicString> idsMatchingAncestorsInRules;
    HashSet<AtomicString> classesInRules;
    HashSet<AtomicString> attributeCanonicalLocalNamesInRules;
    HashSet<AtomicString> attributeLocalNamesInRules;
    Vector<RuleFeature> siblingRules;
    Vector<RuleFeature> uncommonAttributeRules;
    HashMap<AtomicString, std::unique_ptr<Vector<RuleFeature>>> ancestorClassRules;

    struct AttributeRules {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        using SelectorKey = std::pair<AtomicStringImpl*, std::pair<AtomicStringImpl*, unsigned>>;
        HashMap<SelectorKey, const CSSSelector*> selectors;
        Vector<RuleFeature> features;
    };
    HashMap<AtomicString, std::unique_ptr<AttributeRules>> ancestorAttributeRulesForHTML;
    bool usesFirstLineRules { false };
    bool usesFirstLetterRules { false };

private:
    struct SelectorFeatures {
        bool hasSiblingSelector { false };
        Vector<AtomicString, 32> classesMatchingAncestors;
        Vector<const CSSSelector*> attributeSelectorsMatchingAncestors;
    };
    void recursivelyCollectFeaturesFromSelector(SelectorFeatures&, const CSSSelector&, bool matchesAncestor = false);
};

} // namespace WebCore
