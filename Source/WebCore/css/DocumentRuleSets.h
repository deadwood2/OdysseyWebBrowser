/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "CSSDefaultStyleSheets.h"
#include "RuleFeature.h"
#include "RuleSet.h"
#include <memory>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class CSSStyleRule;
class CSSStyleSheet;
class ExtensionStyleSheets;
class InspectorCSSOMWrappers;
class MediaQueryEvaluator;

class DocumentRuleSets {
public:
    DocumentRuleSets(StyleResolver&);
    ~DocumentRuleSets();

    bool isAuthorStyleDefined() const { return m_isAuthorStyleDefined; }
    RuleSet& authorStyle() const { return *m_authorStyle.get(); }
    RuleSet* userStyle() const;
    const RuleFeatureSet& features() const;
    RuleSet* sibling() const { return m_siblingRuleSet.get(); }
    RuleSet* uncommonAttribute() const { return m_uncommonAttributeRuleSet.get(); }
    RuleSet* ancestorClassRules(const AtomicString& className) const;

    struct AttributeRules {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        Vector<const CSSSelector*> attributeSelectors;
        std::unique_ptr<RuleSet> ruleSet;
    };
    const AttributeRules* ancestorAttributeRulesForHTML(const AtomicString&) const;

    void setUsesSharedUserStyle(bool b) { m_usesSharedUserStyle = b; }
    void initializeUserStyle();

    void resetAuthorStyle();
    void appendAuthorStyleSheets(const Vector<RefPtr<CSSStyleSheet>>&, MediaQueryEvaluator*, InspectorCSSOMWrappers&, StyleResolver*);

    RuleFeatureSet& mutableFeatures();

private:
    void collectFeatures() const;
    void collectRulesFromUserStyleSheets(const Vector<RefPtr<CSSStyleSheet>>&, RuleSet& userStyle, const MediaQueryEvaluator&, StyleResolver&);

    bool m_isAuthorStyleDefined { false };
    std::unique_ptr<RuleSet> m_authorStyle;
    std::unique_ptr<RuleSet> m_userStyle;
    bool m_usesSharedUserStyle { false };

    StyleResolver& m_styleResolver;
    mutable RuleFeatureSet m_features;
    mutable unsigned m_defaultStyleVersionOnFeatureCollection { 0 };
    mutable std::unique_ptr<RuleSet> m_siblingRuleSet;
    mutable std::unique_ptr<RuleSet> m_uncommonAttributeRuleSet;
    mutable HashMap<AtomicString, std::unique_ptr<RuleSet>> m_ancestorClassRuleSets;
    mutable HashMap<AtomicString, std::unique_ptr<AttributeRules>> m_ancestorAttributeRuleSetsForHTML;
};

inline const RuleFeatureSet& DocumentRuleSets::features() const
{
    if (m_defaultStyleVersionOnFeatureCollection < CSSDefaultStyleSheets::defaultStyleVersion)
        collectFeatures();
    return m_features;
}

// FIXME: There should be just the const version.
inline RuleFeatureSet& DocumentRuleSets::mutableFeatures()
{
    if (m_defaultStyleVersionOnFeatureCollection < CSSDefaultStyleSheets::defaultStyleVersion)
        collectFeatures();
    return m_features;
}

} // namespace WebCore
