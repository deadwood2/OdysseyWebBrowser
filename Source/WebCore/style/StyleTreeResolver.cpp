/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2004-2010, 2012-2016 Apple Inc. All rights reserved.
 *           (C) 2007 Eric Seidel (eric@webkit.org)
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
#include "StyleTreeResolver.h"

#include "CSSAnimationController.h"
#include "CSSFontSelector.h"
#include "ComposedTreeAncestorIterator.h"
#include "ComposedTreeIterator.h"
#include "ElementIterator.h"
#include "HTMLBodyElement.h"
#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "HTMLProgressElement.h"
#include "HTMLSlotElement.h"
#include "LoaderStrategy.h"
#include "MainFrame.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "PlatformStrategies.h"
#include "RenderElement.h"
#include "RenderView.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StyleFontSizeFunctions.h"
#include "StyleResolver.h"
#include "StyleScope.h"
#include "Text.h"

namespace WebCore {

namespace Style {

TreeResolver::TreeResolver(Document& document)
    : m_document(document)
{
}

TreeResolver::~TreeResolver()
{
}

TreeResolver::Scope::Scope(Document& document)
    : styleResolver(document.styleScope().resolver())
    , sharingResolver(document, styleResolver.ruleSets(), selectorFilter)
{
}

TreeResolver::Scope::Scope(ShadowRoot& shadowRoot, Scope& enclosingScope)
    : styleResolver(shadowRoot.styleScope().resolver())
    , sharingResolver(shadowRoot.documentScope(), styleResolver.ruleSets(), selectorFilter)
    , shadowRoot(&shadowRoot)
    , enclosingScope(&enclosingScope)
{
    styleResolver.setOverrideDocumentElementStyle(enclosingScope.styleResolver.overrideDocumentElementStyle());
}

TreeResolver::Scope::~Scope()
{
    styleResolver.setOverrideDocumentElementStyle(nullptr);
}

TreeResolver::Parent::Parent(Document& document)
    : element(nullptr)
    , style(*document.renderStyle())
{
}

TreeResolver::Parent::Parent(Element& element, const RenderStyle& style, Change change)
    : element(&element)
    , style(style)
    , change(change)
{
}

void TreeResolver::pushScope(ShadowRoot& shadowRoot)
{
    m_scopeStack.append(adoptRef(*new Scope(shadowRoot, scope())));
}

void TreeResolver::pushEnclosingScope()
{
    ASSERT(scope().enclosingScope);
    m_scopeStack.append(*scope().enclosingScope);
}

void TreeResolver::popScope()
{
    return m_scopeStack.removeLast();
}

std::unique_ptr<RenderStyle> TreeResolver::styleForElement(Element& element, const RenderStyle& inheritedStyle)
{
    if (element.hasCustomStyleResolveCallbacks()) {
        RenderStyle* shadowHostStyle = scope().shadowRoot ? m_update->elementStyle(*scope().shadowRoot->host()) : nullptr;
        if (auto customStyle = element.resolveCustomStyle(inheritedStyle, shadowHostStyle)) {
            if (customStyle->relations)
                commitRelations(WTFMove(customStyle->relations), *m_update);

            return WTFMove(customStyle->renderStyle);
        }
    }

    if (auto style = scope().sharingResolver.resolve(element, *m_update))
        return style;

    auto elementStyle = scope().styleResolver.styleForElement(element, &inheritedStyle, parentBoxStyle(), MatchAllRules, nullptr, &scope().selectorFilter);

    if (elementStyle.relations)
        commitRelations(WTFMove(elementStyle.relations), *m_update);

    return WTFMove(elementStyle.renderStyle);
}

static void resetStyleForNonRenderedDescendants(Element& current)
{
    // FIXME: This is not correct with shadow trees. This should be done with ComposedTreeIterator.
    bool elementNeedingStyleRecalcAffectsNextSiblingElementStyle = false;
    for (auto& child : childrenOfType<Element>(current)) {
        bool affectedByPreviousSibling = child.styleIsAffectedByPreviousSibling() && elementNeedingStyleRecalcAffectsNextSiblingElementStyle;
        if (child.needsStyleRecalc() || elementNeedingStyleRecalcAffectsNextSiblingElementStyle)
            elementNeedingStyleRecalcAffectsNextSiblingElementStyle = child.affectsNextSiblingElementStyle();

        if (child.needsStyleRecalc() || affectedByPreviousSibling) {
            child.resetComputedStyle();
            child.resetStyleRelations();
            child.setHasValidStyle();
        }

        if (child.childNeedsStyleRecalc()) {
            resetStyleForNonRenderedDescendants(child);
            child.clearChildNeedsStyleRecalc();
        }
    }
}

static bool affectsRenderedSubtree(Element& element, const RenderStyle& newStyle)
{
    if (element.renderer())
        return true;
    if (newStyle.display() != NONE)
        return true;
    if (element.rendererIsNeeded(newStyle))
        return true;
#if ENABLE(CSS_REGIONS)
    if (element.shouldMoveToFlowThread(newStyle))
        return true;
#endif
    return false;
}

static const RenderStyle* renderOrDisplayContentsStyle(const Element& element)
{
    if (auto* renderStyle = element.renderStyle())
        return renderStyle;
    if (element.hasDisplayContents())
        return element.existingComputedStyle();
    return nullptr;
}

ElementUpdate TreeResolver::resolveElement(Element& element)
{
    if (m_didSeePendingStylesheet && !element.renderer() && !m_document.isIgnoringPendingStylesheets()) {
        m_document.setHasNodesWithMissingStyle();
        return { };
    }

    auto newStyle = styleForElement(element, parent().style);

    if (!affectsRenderedSubtree(element, *newStyle))
        return { };

    auto* existingStyle = renderOrDisplayContentsStyle(element);

    if (m_didSeePendingStylesheet && (!existingStyle || existingStyle->isNotFinal())) {
        newStyle->setIsNotFinal();
        m_document.setHasNodesWithNonFinalStyle();
    }

    auto update = createAnimatedElementUpdate(WTFMove(newStyle), element, parent().change);

    if (&element == m_document.documentElement()) {
        m_documentElementStyle = RenderStyle::clonePtr(*update.style);
        scope().styleResolver.setOverrideDocumentElementStyle(m_documentElementStyle.get());

        if (update.change != NoChange && existingStyle && existingStyle->computedFontPixelSize() != update.style->computedFontPixelSize()) {
            // "rem" units are relative to the document element's font size so we need to recompute everything.
            // In practice this is rare.
            scope().styleResolver.invalidateMatchedPropertiesCache();
            update.change = std::max(update.change, Force);
        }
    }

    // This is needed for resolving color:-webkit-text for subsequent elements.
    // FIXME: We shouldn't mutate document when resolving style.
    if (&element == m_document.body())
        m_document.setTextColor(update.style->visitedDependentColor(CSSPropertyColor));

    // FIXME: These elements should not change renderer based on appearance property.
    if (element.hasTagName(HTMLNames::meterTag) || is<HTMLProgressElement>(element)) {
        if (existingStyle && update.style->appearance() != existingStyle->appearance())
            update.change = Detach;
    }

    return update;
}

const RenderStyle* TreeResolver::parentBoxStyle() const
{
    // 'display: contents' doesn't generate boxes.
    for (unsigned i = m_parentStack.size(); i; --i) {
        auto& parent = m_parentStack[i - 1];
        if (parent.style.display() == NONE)
            return nullptr;
        if (parent.style.display() != CONTENTS)
            return &parent.style;
    }
    ASSERT_NOT_REACHED();
    return nullptr;
}

ElementUpdate TreeResolver::createAnimatedElementUpdate(std::unique_ptr<RenderStyle> newStyle, Element& element, Change parentChange)
{
    auto validity = element.styleValidity();
    bool recompositeLayer = element.styleResolutionShouldRecompositeLayer();

    auto makeUpdate = [&] (std::unique_ptr<RenderStyle> style, Change change) {
        if (validity >= Validity::SubtreeInvalid)
            change = std::max(change, validity == Validity::SubtreeAndRenderersInvalid ? Detach : Force);
        if (parentChange >= Force)
            change = std::max(change, parentChange);
        return ElementUpdate { WTFMove(style), change, recompositeLayer };
    };

    auto* renderer = element.renderer();

    bool shouldReconstruct = validity >= Validity::SubtreeAndRenderersInvalid || parentChange == Detach;
    if (shouldReconstruct)
        return makeUpdate(WTFMove(newStyle), Detach);

    if (!renderer) {
        auto change = Detach;
        if (auto* oldStyle = renderOrDisplayContentsStyle(element))
            change = determineChange(*oldStyle, *newStyle);
        return makeUpdate(WTFMove(newStyle), change);
    }

    std::unique_ptr<RenderStyle> animatedStyle;
    if (element.document().frame()->animation().updateAnimations(*renderer, *newStyle, animatedStyle))
        recompositeLayer = true;

    if (animatedStyle) {
        auto change = determineChange(renderer->style(), *animatedStyle);
        if (renderer->hasInitialAnimatedStyle()) {
            renderer->setHasInitialAnimatedStyle(false);
            // When we initialize a newly created renderer with initial animated style we don't inherit it to descendants.
            // The first animation frame needs to correct this.
            // FIXME: We should compute animated style correctly during initial style resolution when we don't have renderers yet.
            //        https://bugs.webkit.org/show_bug.cgi?id=171926
            change = std::max(change, Inherit);
        }
        // If animation forces render tree reconstruction pass the original style. The animation will be applied on renderer construction.
        // FIXME: We should always use the animated style here.
        auto style = change == Detach ? WTFMove(newStyle) : WTFMove(animatedStyle);
        return makeUpdate(WTFMove(style), change);
    }

    auto change = determineChange(renderer->style(), *newStyle);
    return makeUpdate(WTFMove(newStyle), change);
}

void TreeResolver::pushParent(Element& element, const RenderStyle& style, Change change)
{
    scope().selectorFilter.pushParent(&element);

    Parent parent(element, style, change);

    if (auto* shadowRoot = element.shadowRoot()) {
        pushScope(*shadowRoot);
        parent.didPushScope = true;
    }
    else if (is<HTMLSlotElement>(element) && downcast<HTMLSlotElement>(element).assignedNodes()) {
        pushEnclosingScope();
        parent.didPushScope = true;
    }

    m_parentStack.append(WTFMove(parent));
}

void TreeResolver::popParent()
{
    auto& parentElement = *parent().element;

    parentElement.setHasValidStyle();
    parentElement.clearChildNeedsStyleRecalc();

    if (parent().didPushScope)
        popScope();

    scope().selectorFilter.popParent();

    m_parentStack.removeLast();
}

void TreeResolver::popParentsToDepth(unsigned depth)
{
    ASSERT(depth);
    ASSERT(m_parentStack.size() >= depth);

    while (m_parentStack.size() > depth)
        popParent();
}

static bool shouldResolvePseudoElement(const PseudoElement* pseudoElement)
{
    if (!pseudoElement)
        return false;
    return pseudoElement->needsStyleRecalc();
}

static bool shouldResolveElement(const Element& element, Style::Change parentChange)
{
    if (parentChange >= Inherit)
        return true;
    if (parentChange == NoInherit) {
        auto* existingStyle = renderOrDisplayContentsStyle(element);
        if (existingStyle && existingStyle->hasExplicitlyInheritedProperties())
            return true;
    }
    if (element.needsStyleRecalc())
        return true;
    if (shouldResolvePseudoElement(element.beforePseudoElement()))
        return true;
    if (shouldResolvePseudoElement(element.afterPseudoElement()))
        return true;

    return false;
}

static void clearNeedsStyleResolution(Element& element)
{
    element.setHasValidStyle();
    if (auto* before = element.beforePseudoElement())
        before->setHasValidStyle();
    if (auto* after = element.afterPseudoElement())
        after->setHasValidStyle();
}

static bool hasLoadingStylesheet(const Style::Scope& styleScope, const Element& element, bool checkDescendants)
{
    if (!styleScope.hasPendingSheetsInBody())
        return false;
    if (styleScope.hasPendingSheetInBody(element))
        return true;
    if (!checkDescendants)
        return false;
    for (auto& descendant : descendantsOfType<Element>(element)) {
        if (styleScope.hasPendingSheetInBody(descendant))
            return true;
    };
    return false;
}

void TreeResolver::resolveComposedTree()
{
    ASSERT(m_parentStack.size() == 1);
    ASSERT(m_scopeStack.size() == 1);

    auto descendants = composedTreeDescendants(m_document);
    auto it = descendants.begin();
    auto end = descendants.end();

    // FIXME: SVG <use> element may cause tree mutations during style recalc.
    it.dropAssertions();

    while (it != end) {
        popParentsToDepth(it.depth());

        auto& node = *it;
        auto& parent = this->parent();

        ASSERT(node.isConnected());
        ASSERT(node.containingShadowRoot() == scope().shadowRoot);
        ASSERT(node.parentElement() == parent.element || is<ShadowRoot>(node.parentNode()) || node.parentElement()->shadowRoot());

        if (is<Text>(node)) {
            auto& text = downcast<Text>(node);
            if (text.styleValidity() >= Validity::SubtreeAndRenderersInvalid && parent.change != Detach)
                m_update->addText(text, parent.element, { });

            text.setHasValidStyle();
            it.traverseNextSkippingChildren();
            continue;
        }

        auto& element = downcast<Element>(node);

        if (it.depth() > Settings::defaultMaximumRenderTreeDepth) {
            resetStyleForNonRenderedDescendants(element);
            element.clearChildNeedsStyleRecalc();
            it.traverseNextSkippingChildren();
            continue;
        }

        // FIXME: We should deal with this during style invalidation.
        bool affectedByPreviousSibling = element.styleIsAffectedByPreviousSibling() && parent.elementNeedingStyleRecalcAffectsNextSiblingElementStyle;
        if (element.needsStyleRecalc() || parent.elementNeedingStyleRecalcAffectsNextSiblingElementStyle)
            parent.elementNeedingStyleRecalcAffectsNextSiblingElementStyle = element.affectsNextSiblingElementStyle();

        auto* style = renderOrDisplayContentsStyle(element);
        auto change = NoChange;

        bool shouldResolve = shouldResolveElement(element, parent.change) || affectedByPreviousSibling;
        if (shouldResolve) {
            if (!element.hasDisplayContents())
                element.resetComputedStyle();
            element.resetStyleRelations();

            if (element.hasCustomStyleResolveCallbacks())
                element.willRecalcStyle(parent.change);

            auto elementUpdate = resolveElement(element);

            if (element.hasCustomStyleResolveCallbacks())
                element.didRecalcStyle(elementUpdate.change);

            style = elementUpdate.style.get();
            change = elementUpdate.change;

            if (affectedByPreviousSibling && change != Detach)
                change = Force;

            if (elementUpdate.style)
                m_update->addElement(element, parent.element, WTFMove(elementUpdate));

            clearNeedsStyleResolution(element);
        }

        if (!style) {
            resetStyleForNonRenderedDescendants(element);
            element.clearChildNeedsStyleRecalc();
        }

        bool shouldIterateChildren = style && (element.childNeedsStyleRecalc() || change != NoChange);

        if (!m_didSeePendingStylesheet)
            m_didSeePendingStylesheet = hasLoadingStylesheet(m_document.styleScope(), element, !shouldIterateChildren);

        if (!shouldIterateChildren) {
            it.traverseNextSkippingChildren();
            continue;
        }

        pushParent(element, *style, change);

        it.traverseNext();
    }

    popParentsToDepth(1);
}

std::unique_ptr<Update> TreeResolver::resolve()
{
    auto& renderView = *m_document.renderView();

    Element* documentElement = m_document.documentElement();
    if (!documentElement) {
        m_document.styleScope().resolver();
        return nullptr;
    }
    if (!documentElement->childNeedsStyleRecalc() && !documentElement->needsStyleRecalc())
        return nullptr;

    m_didSeePendingStylesheet = m_document.styleScope().hasPendingSheetsBeforeBody();

    m_update = std::make_unique<Update>(m_document);
    m_scopeStack.append(adoptRef(*new Scope(m_document)));
    m_parentStack.append(Parent(m_document));

    // Pseudo element removal and similar may only work with these flags still set. Reset them after the style recalc.
    renderView.setUsesFirstLineRules(renderView.usesFirstLineRules() || scope().styleResolver.usesFirstLineRules());
    renderView.setUsesFirstLetterRules(renderView.usesFirstLetterRules() || scope().styleResolver.usesFirstLetterRules());

    resolveComposedTree();

    renderView.setUsesFirstLineRules(scope().styleResolver.usesFirstLineRules());
    renderView.setUsesFirstLetterRules(scope().styleResolver.usesFirstLetterRules());

    ASSERT(m_scopeStack.size() == 1);
    ASSERT(m_parentStack.size() == 1);
    m_parentStack.clear();
    popScope();

    if (m_update->roots().isEmpty())
        return { };

    return WTFMove(m_update);
}

static Vector<Function<void ()>>& postResolutionCallbackQueue()
{
    static NeverDestroyed<Vector<Function<void ()>>> vector;
    return vector;
}

void queuePostResolutionCallback(Function<void ()>&& callback)
{
    postResolutionCallbackQueue().append(WTFMove(callback));
}

static void suspendMemoryCacheClientCalls(Document& document)
{
    Page* page = document.page();
    if (!page || !page->areMemoryCacheClientCallsEnabled())
        return;

    page->setMemoryCacheClientCallsEnabled(false);

    postResolutionCallbackQueue().append([protectedMainFrame = Ref<MainFrame>(page->mainFrame())] {
        if (Page* page = protectedMainFrame->page())
            page->setMemoryCacheClientCallsEnabled(true);
    });
}

static unsigned resolutionNestingDepth;

PostResolutionCallbackDisabler::PostResolutionCallbackDisabler(Document& document)
{
    ++resolutionNestingDepth;

    if (resolutionNestingDepth == 1)
        platformStrategies()->loaderStrategy()->suspendPendingRequests();

    // FIXME: It's strange to build this into the disabler.
    suspendMemoryCacheClientCalls(document);
}

PostResolutionCallbackDisabler::~PostResolutionCallbackDisabler()
{
    if (resolutionNestingDepth == 1) {
        // Get size each time through the loop because a callback can add more callbacks to the end of the queue.
        auto& queue = postResolutionCallbackQueue();
        for (size_t i = 0; i < queue.size(); ++i)
            queue[i]();
        queue.clear();

        platformStrategies()->loaderStrategy()->resumePendingRequests();
    }

    --resolutionNestingDepth;
}

bool postResolutionCallbacksAreSuspended()
{
    return resolutionNestingDepth;
}

}
}
