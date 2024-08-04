/*
 * Copyright (C) 2012, 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/text/AtomicStringImpl.h>

namespace WebCore {

class Document;
class Element;
class MediaQueryEvaluator;
class RuleSet;
class SelectorFilter;
class ShadowRoot;
class StyleSheetContents;

class StyleInvalidationAnalysis {
public:
    StyleInvalidationAnalysis(const Vector<StyleSheetContents*>&, const MediaQueryEvaluator&);
    StyleInvalidationAnalysis(const RuleSet&);

    bool dirtiesAllStyle() const { return m_dirtiesAllStyle; }
    bool hasShadowPseudoElementRulesInAuthorSheet() const { return m_hasShadowPseudoElementRulesInAuthorSheet; }
    void invalidateStyle(Document&);
    void invalidateStyle(ShadowRoot&);
    void invalidateStyle(Element&);

private:
    enum class CheckDescendants { Yes, No };
    CheckDescendants invalidateIfNeeded(Element&, const SelectorFilter*);
    void invalidateStyleForTree(Element&, SelectorFilter*);

    std::unique_ptr<RuleSet> m_ownedRuleSet;
    const RuleSet& m_ruleSet;
    bool m_dirtiesAllStyle { false };
    bool m_hasShadowPseudoElementRulesInAuthorSheet { false };
    bool m_didInvalidateHostChildren { false };
};

} // namespace WebCore
