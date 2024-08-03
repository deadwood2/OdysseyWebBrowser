/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2012, 2013 Apple Inc. All rights reserved.
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
#include "CSSStyleSheet.h"

#include "AuthorStyleSheets.h"
#include "CSSCharsetRule.h"
#include "CSSFontFaceRule.h"
#include "CSSImportRule.h"
#include "CSSKeyframesRule.h"
#include "CSSParser.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CachedCSSStyleSheet.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "ExtensionStyleSheets.h"
#include "HTMLLinkElement.h"
#include "HTMLNames.h"
#include "HTMLStyleElement.h"
#include "MediaList.h"
#include "Node.h"
#include "SVGNames.h"
#include "SVGStyleElement.h"
#include "SecurityOrigin.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "StyleSheetContents.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

class StyleSheetCSSRuleList final : public CSSRuleList {
public:
    StyleSheetCSSRuleList(CSSStyleSheet* sheet) : m_styleSheet(sheet) { }
    
private:
    void ref() final { m_styleSheet->ref(); }
    void deref() final { m_styleSheet->deref(); }

    unsigned length() const final { return m_styleSheet->length(); }
    CSSRule* item(unsigned index) const final { return m_styleSheet->item(index); }

    CSSStyleSheet* styleSheet() const final { return m_styleSheet; }

    CSSStyleSheet* m_styleSheet;
};

#if !ASSERT_DISABLED
static bool isAcceptableCSSStyleSheetParent(Node* parentNode)
{
    // Only these nodes can be parents of StyleSheets, and they need to call clearOwnerNode() when moved out of document.
    return !parentNode
        || parentNode->isDocumentNode()
        || is<HTMLLinkElement>(*parentNode)
        || is<HTMLStyleElement>(*parentNode)
        || is<SVGStyleElement>(*parentNode)
        || parentNode->nodeType() == Node::PROCESSING_INSTRUCTION_NODE;
}
#endif

Ref<CSSStyleSheet> CSSStyleSheet::create(Ref<StyleSheetContents>&& sheet, CSSImportRule* ownerRule)
{ 
    return adoptRef(*new CSSStyleSheet(WTFMove(sheet), ownerRule));
}

Ref<CSSStyleSheet> CSSStyleSheet::create(Ref<StyleSheetContents>&& sheet, Node* ownerNode)
{ 
    return adoptRef(*new CSSStyleSheet(WTFMove(sheet), ownerNode, TextPosition::minimumPosition(), false));
}

Ref<CSSStyleSheet> CSSStyleSheet::createInline(Node& ownerNode, const URL& baseURL, const TextPosition& startPosition, const String& encoding)
{
    CSSParserContext parserContext(ownerNode.document(), baseURL, encoding);
    return adoptRef(*new CSSStyleSheet(StyleSheetContents::create(baseURL.string(), parserContext), &ownerNode, startPosition, true));
}

CSSStyleSheet::CSSStyleSheet(Ref<StyleSheetContents>&& contents, CSSImportRule* ownerRule)
    : m_contents(WTFMove(contents))
    , m_isInlineStylesheet(false)
    , m_isDisabled(false)
    , m_mutatedRules(false)
    , m_ownerNode(0)
    , m_ownerRule(ownerRule)
    , m_startPosition()
{
    m_contents->registerClient(this);
}

CSSStyleSheet::CSSStyleSheet(Ref<StyleSheetContents>&& contents, Node* ownerNode, const TextPosition& startPosition, bool isInlineStylesheet)
    : m_contents(WTFMove(contents))
    , m_isInlineStylesheet(isInlineStylesheet)
    , m_isDisabled(false)
    , m_mutatedRules(false)
    , m_ownerNode(ownerNode)
    , m_ownerRule(0)
    , m_startPosition(startPosition)
{
    ASSERT(isAcceptableCSSStyleSheetParent(ownerNode));
    m_contents->registerClient(this);
}

CSSStyleSheet::~CSSStyleSheet()
{
    // For style rules outside the document, .parentStyleSheet can become null even if the style rule
    // is still observable from JavaScript. This matches the behavior of .parentNode for nodes, but
    // it's not ideal because it makes the CSSOM's behavior depend on the timing of garbage collection.
    for (unsigned i = 0; i < m_childRuleCSSOMWrappers.size(); ++i) {
        if (m_childRuleCSSOMWrappers[i])
            m_childRuleCSSOMWrappers[i]->setParentStyleSheet(0);
    }
    if (m_mediaCSSOMWrapper)
        m_mediaCSSOMWrapper->clearParentStyleSheet();

    m_contents->unregisterClient(this);
}

CSSStyleSheet::WhetherContentsWereClonedForMutation CSSStyleSheet::willMutateRules()
{
    // If we are the only client it is safe to mutate.
    if (m_contents->hasOneClient() && !m_contents->isInMemoryCache()) {
        m_contents->setMutable();
        return ContentsWereNotClonedForMutation;
    }
    // Only cacheable stylesheets should have multiple clients.
    ASSERT(m_contents->isCacheable());

    // Copy-on-write.
    m_contents->unregisterClient(this);
    m_contents = m_contents->copy();
    m_contents->registerClient(this);

    m_contents->setMutable();

    // Any existing CSSOM wrappers need to be connected to the copied child rules.
    reattachChildRuleCSSOMWrappers();

    return ContentsWereClonedForMutation;
}

void CSSStyleSheet::didMutateRuleFromCSSStyleDeclaration()
{
    ASSERT(m_contents->isMutable());
    ASSERT(m_contents->hasOneClient());
    didMutate();
}

void CSSStyleSheet::didMutateRules(RuleMutationType mutationType, WhetherContentsWereClonedForMutation contentsWereClonedForMutation, StyleRuleKeyframes* insertedKeyframesRule)
{
    ASSERT(m_contents->isMutable());
    ASSERT(m_contents->hasOneClient());

    Document* owner = ownerDocument();
    if (!owner)
        return;

    if (mutationType == RuleInsertion && !contentsWereClonedForMutation && !owner->authorStyleSheets().activeStyleSheetsContains(this)) {
        if (insertedKeyframesRule) {
            if (StyleResolver* resolver = owner->styleResolverIfExists())
                resolver->addKeyframeStyle(*insertedKeyframesRule);
            return;
        }
        owner->scheduleOptimizedStyleSheetUpdate();
        return;
    }

    owner->styleResolverChanged(DeferRecalcStyle);

    m_mutatedRules = true;
}

void CSSStyleSheet::didMutate()
{
    Document* owner = ownerDocument();
    if (!owner)
        return;
    owner->styleResolverChanged(DeferRecalcStyle);
}

void CSSStyleSheet::clearOwnerNode()
{
    Document* owner = ownerDocument();
    m_ownerNode = 0;
    if (!owner)
        return;
    owner->styleResolverChanged(DeferRecalcStyleIfNeeded);
}

void CSSStyleSheet::reattachChildRuleCSSOMWrappers()
{
    for (unsigned i = 0; i < m_childRuleCSSOMWrappers.size(); ++i) {
        if (!m_childRuleCSSOMWrappers[i])
            continue;
        m_childRuleCSSOMWrappers[i]->reattach(*m_contents->ruleAt(i));
    }
}

void CSSStyleSheet::setDisabled(bool disabled)
{ 
    if (disabled == m_isDisabled)
        return;
    m_isDisabled = disabled;

    didMutate();
}

void CSSStyleSheet::setMediaQueries(Ref<MediaQuerySet>&& mediaQueries)
{
    m_mediaQueries = WTFMove(mediaQueries);
    if (m_mediaCSSOMWrapper && m_mediaQueries)
        m_mediaCSSOMWrapper->reattach(m_mediaQueries.get());
    reportMediaQueryWarningIfNeeded(ownerDocument(), m_mediaQueries.get());
}

unsigned CSSStyleSheet::length() const
{
    return m_contents->ruleCount();
}

CSSRule* CSSStyleSheet::item(unsigned index)
{
    unsigned ruleCount = length();
    if (index >= ruleCount)
        return 0;

    if (m_childRuleCSSOMWrappers.isEmpty())
        m_childRuleCSSOMWrappers.grow(ruleCount);
    ASSERT(m_childRuleCSSOMWrappers.size() == ruleCount);
    
    RefPtr<CSSRule>& cssRule = m_childRuleCSSOMWrappers[index];
    if (!cssRule) {
        if (index == 0 && m_contents->hasCharsetRule()) {
            ASSERT(!m_contents->ruleAt(0));
            cssRule = CSSCharsetRule::create(this, m_contents->encodingFromCharsetRule());
        } else
            cssRule = m_contents->ruleAt(index)->createCSSOMWrapper(this);
    }
    return cssRule.get();
}

bool CSSStyleSheet::canAccessRules() const
{
    if (m_isInlineStylesheet)
        return true;
    URL baseURL = m_contents->baseURL();
    if (baseURL.isEmpty())
        return true;
    Document* document = ownerDocument();
    if (!document)
        return true;
    if (document->securityOrigin()->canRequest(baseURL))
        return true;
    return false;
}

RefPtr<CSSRuleList> CSSStyleSheet::rules()
{
    if (!canAccessRules())
        return nullptr;
    // IE behavior.
    RefPtr<StaticCSSRuleList> nonCharsetRules = StaticCSSRuleList::create();
    unsigned ruleCount = length();
    for (unsigned i = 0; i < ruleCount; ++i) {
        CSSRule* rule = item(i);
        if (rule->type() == CSSRule::CHARSET_RULE)
            continue;
        nonCharsetRules->rules().append(rule);
    }
    return nonCharsetRules;
}

unsigned CSSStyleSheet::deprecatedInsertRule(const String& ruleString, ExceptionCode& ec)
{
    if (auto* document = ownerDocument())
        document->addConsoleMessage(MessageSource::JS, MessageLevel::Warning, ASCIILiteral("Calling CSSStyleSheet.insertRule() with one argument is deprecated. Please pass the index argument as well: insertRule(x, 0)."));

    return insertRule(ruleString, 0, ec);
}

unsigned CSSStyleSheet::insertRule(const String& ruleString, unsigned index, ExceptionCode& ec)
{
    ASSERT(m_childRuleCSSOMWrappers.isEmpty() || m_childRuleCSSOMWrappers.size() == m_contents->ruleCount());

    ec = 0;
    if (index > length()) {
        ec = INDEX_SIZE_ERR;
        return 0;
    }
    CSSParser p(m_contents.get().parserContext());
    RefPtr<StyleRuleBase> rule = p.parseRule(m_contents.ptr(), ruleString);

    if (!rule) {
        ec = SYNTAX_ERR;
        return 0;
    }

    RuleMutationScope mutationScope(this, RuleInsertion, is<StyleRuleKeyframes>(*rule) ? downcast<StyleRuleKeyframes>(rule.get()) : nullptr);

    bool success = m_contents.get().wrapperInsertRule(rule.releaseNonNull(), index);
    if (!success) {
        ec = HIERARCHY_REQUEST_ERR;
        return 0;
    }        
    if (!m_childRuleCSSOMWrappers.isEmpty())
        m_childRuleCSSOMWrappers.insert(index, RefPtr<CSSRule>());

    return index;
}

void CSSStyleSheet::deleteRule(unsigned index, ExceptionCode& ec)
{
    ASSERT(m_childRuleCSSOMWrappers.isEmpty() || m_childRuleCSSOMWrappers.size() == m_contents->ruleCount());

    ec = 0;
    if (index >= length()) {
        ec = INDEX_SIZE_ERR;
        return;
    }
    RuleMutationScope mutationScope(this);

    m_contents->wrapperDeleteRule(index);

    if (!m_childRuleCSSOMWrappers.isEmpty()) {
        if (m_childRuleCSSOMWrappers[index])
            m_childRuleCSSOMWrappers[index]->setParentStyleSheet(nullptr);
        m_childRuleCSSOMWrappers.remove(index);
    }
}

int CSSStyleSheet::addRule(const String& selector, const String& style, Optional<unsigned> index, ExceptionCode& ec)
{
    StringBuilder text;
    text.append(selector);
    text.appendLiteral(" { ");
    text.append(style);
    if (!style.isEmpty())
        text.append(' ');
    text.append('}');
    insertRule(text.toString(), index.valueOr(length()), ec);
    
    // As per Microsoft documentation, always return -1.
    return -1;
}

RefPtr<CSSRuleList> CSSStyleSheet::cssRules()
{
    if (!canAccessRules())
        return nullptr;
    if (!m_ruleListCSSOMWrapper)
        m_ruleListCSSOMWrapper = std::make_unique<StyleSheetCSSRuleList>(this);
    return m_ruleListCSSOMWrapper.get();
}

String CSSStyleSheet::href() const
{
    return m_contents->originalURL();
}

URL CSSStyleSheet::baseURL() const
{
    return m_contents->baseURL();
}

bool CSSStyleSheet::isLoading() const
{
    return m_contents->isLoading();
}

MediaList* CSSStyleSheet::media() const 
{
    if (!m_mediaQueries)
        return nullptr;

    if (!m_mediaCSSOMWrapper)
        m_mediaCSSOMWrapper = MediaList::create(m_mediaQueries.get(), const_cast<CSSStyleSheet*>(this));
    return m_mediaCSSOMWrapper.get();
}

CSSStyleSheet* CSSStyleSheet::parentStyleSheet() const 
{ 
    return m_ownerRule ? m_ownerRule->parentStyleSheet() : nullptr;
}

Document* CSSStyleSheet::ownerDocument() const
{
    const CSSStyleSheet* root = this;
    while (root->parentStyleSheet())
        root = root->parentStyleSheet();
    return root->ownerNode() ? &root->ownerNode()->document() : nullptr;
}

void CSSStyleSheet::clearChildRuleCSSOMWrappers()
{
    m_childRuleCSSOMWrappers.clear();
}

CSSStyleSheet::RuleMutationScope::RuleMutationScope(CSSStyleSheet* sheet, RuleMutationType mutationType, StyleRuleKeyframes* insertedKeyframesRule)
    : m_styleSheet(sheet)
    , m_mutationType(mutationType)
    , m_insertedKeyframesRule(insertedKeyframesRule)
{
    ASSERT(m_styleSheet);
    m_contentsWereClonedForMutation = m_styleSheet->willMutateRules();
}

CSSStyleSheet::RuleMutationScope::RuleMutationScope(CSSRule* rule)
    : m_styleSheet(rule ? rule->parentStyleSheet() : nullptr)
    , m_mutationType(OtherMutation)
    , m_contentsWereClonedForMutation(ContentsWereNotClonedForMutation)
    , m_insertedKeyframesRule(nullptr)
{
    if (m_styleSheet)
        m_contentsWereClonedForMutation = m_styleSheet->willMutateRules();
}

CSSStyleSheet::RuleMutationScope::~RuleMutationScope()
{
    if (m_styleSheet)
        m_styleSheet->didMutateRules(m_mutationType, m_contentsWereClonedForMutation, m_insertedKeyframesRule);
}

}
