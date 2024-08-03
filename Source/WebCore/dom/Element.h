/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2016 Apple Inc. All rights reserved.
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

#ifndef Element_h
#define Element_h

#include "AXTextStateChangeIntent.h"
#include "Document.h"
#include "ElementData.h"
#include "HTMLNames.h"
#include "RegionOversetState.h"
#include "ScrollTypes.h"
#include "SimulatedClickOptions.h"
#include "StyleChange.h"

namespace WebCore {

class ClientRect;
class ClientRectList;
class DatasetDOMStringMap;
class Dictionary;
class DOMTokenList;
class ElementRareData;
class HTMLDocument;
class IntSize;
class JSCustomElementInterface;
class KeyboardEvent;
class Locale;
class PlatformKeyboardEvent;
class PlatformMouseEvent;
class PlatformWheelEvent;
class PseudoElement;
class RenderNamedFlowFragment;
class RenderTreePosition;
class WebAnimation;
struct ElementStyle;

enum SpellcheckAttributeState {
    SpellcheckAttributeTrue,
    SpellcheckAttributeFalse,
    SpellcheckAttributeDefault
};

enum class SelectionRevealMode {
    Reveal,
    RevealUpToMainFrame, // Scroll overflow and iframes, but not the main frame.
    DoNotReveal
};

class Element : public ContainerNode {
public:
    static Ref<Element> create(const QualifiedName&, Document&);
    virtual ~Element();

    WEBCORE_EXPORT bool hasAttribute(const QualifiedName&) const;
    WEBCORE_EXPORT const AtomicString& getAttribute(const QualifiedName&) const;
    WEBCORE_EXPORT void setAttribute(const QualifiedName&, const AtomicString& value);
    WEBCORE_EXPORT void setAttributeWithoutSynchronization(const QualifiedName&, const AtomicString& value);
    void setSynchronizedLazyAttribute(const QualifiedName&, const AtomicString& value);
    bool removeAttribute(const QualifiedName&);
    Vector<String> getAttributeNames() const;

    // Typed getters and setters for language bindings.
    WEBCORE_EXPORT int getIntegralAttribute(const QualifiedName& attributeName) const;
    WEBCORE_EXPORT void setIntegralAttribute(const QualifiedName& attributeName, int value);
    WEBCORE_EXPORT unsigned getUnsignedIntegralAttribute(const QualifiedName& attributeName) const;
    WEBCORE_EXPORT void setUnsignedIntegralAttribute(const QualifiedName& attributeName, unsigned value);

    // Call this to get the value of an attribute that is known not to be the style
    // attribute or one of the SVG animatable attributes.
    bool hasAttributeWithoutSynchronization(const QualifiedName&) const;
    const AtomicString& attributeWithoutSynchronization(const QualifiedName&) const;
#ifndef NDEBUG
    WEBCORE_EXPORT bool fastAttributeLookupAllowed(const QualifiedName&) const;
#endif

#ifdef DUMP_NODE_STATISTICS
    bool hasNamedNodeMap() const;
#endif
    WEBCORE_EXPORT bool hasAttributes() const;
    // This variant will not update the potentially invalid attributes. To be used when not interested
    // in style attribute or one of the SVG animation attributes.
    bool hasAttributesWithoutUpdate() const;

    WEBCORE_EXPORT bool hasAttribute(const AtomicString& name) const;
    WEBCORE_EXPORT bool hasAttributeNS(const AtomicString& namespaceURI, const AtomicString& localName) const;

    WEBCORE_EXPORT const AtomicString& getAttribute(const AtomicString& name) const;
    WEBCORE_EXPORT const AtomicString& getAttributeNS(const AtomicString& namespaceURI, const AtomicString& localName) const;

    WEBCORE_EXPORT void setAttribute(const AtomicString& name, const AtomicString& value, ExceptionCode&);
    static bool parseAttributeName(QualifiedName&, const AtomicString& namespaceURI, const AtomicString& qualifiedName, ExceptionCode&);
    WEBCORE_EXPORT void setAttributeNS(const AtomicString& namespaceURI, const AtomicString& qualifiedName, const AtomicString& value, ExceptionCode&);

    const AtomicString& getIdAttribute() const;
    void setIdAttribute(const AtomicString&);

    const AtomicString& getNameAttribute() const;

    // Call this to get the value of the id attribute for style resolution purposes.
    // The value will already be lowercased if the document is in compatibility mode,
    // so this function is not suitable for non-style uses.
    const AtomicString& idForStyleResolution() const;

    // Internal methods that assume the existence of attribute storage, one should use hasAttributes()
    // before calling them.
    AttributeIteratorAccessor attributesIterator() const { return elementData()->attributesIterator(); }
    unsigned attributeCount() const;
    const Attribute& attributeAt(unsigned index) const;
    const Attribute* findAttributeByName(const QualifiedName&) const;
    unsigned findAttributeIndexByName(const QualifiedName& name) const { return elementData()->findAttributeIndexByName(name); }
    unsigned findAttributeIndexByName(const AtomicString& name, bool shouldIgnoreAttributeCase) const { return elementData()->findAttributeIndexByName(name, shouldIgnoreAttributeCase); }

    WEBCORE_EXPORT void scrollIntoView(bool alignToTop = true);
    WEBCORE_EXPORT void scrollIntoViewIfNeeded(bool centerIfNeeded = true);
    WEBCORE_EXPORT void scrollIntoViewIfNotVisible(bool centerIfNotVisible = true);

    WEBCORE_EXPORT void scrollByLines(int lines);
    WEBCORE_EXPORT void scrollByPages(int pages);

    WEBCORE_EXPORT double offsetLeft();
    WEBCORE_EXPORT double offsetTop();
    WEBCORE_EXPORT double offsetWidth();
    WEBCORE_EXPORT double offsetHeight();

    bool mayCauseRepaintInsideViewport(const IntRect* visibleRect = nullptr) const;

    // FIXME: Replace uses of offsetParent in the platform with calls
    // to the render layer and merge bindingsOffsetParent and offsetParent.
    WEBCORE_EXPORT Element* bindingsOffsetParent();

    const Element* rootElement() const;

    Element* offsetParent();
    WEBCORE_EXPORT double clientLeft();
    WEBCORE_EXPORT double clientTop();
    WEBCORE_EXPORT double clientWidth();
    WEBCORE_EXPORT double clientHeight();
    virtual int scrollLeft();
    virtual int scrollTop();
    virtual void setScrollLeft(int);
    virtual void setScrollTop(int);
    virtual int scrollWidth();
    virtual int scrollHeight();

    WEBCORE_EXPORT IntRect boundsInRootViewSpace();

    Ref<ClientRectList> getClientRects();
    Ref<ClientRect> getBoundingClientRect();

    // Returns the absolute bounding box translated into client coordinates.
    WEBCORE_EXPORT IntRect clientRect() const;
    // Returns the absolute bounding box translated into screen coordinates.
    WEBCORE_EXPORT IntRect screenRect() const;

    WEBCORE_EXPORT bool removeAttribute(const AtomicString& name);
    WEBCORE_EXPORT bool removeAttributeNS(const AtomicString& namespaceURI, const AtomicString& localName);

    Ref<Attr> detachAttribute(unsigned index);

    WEBCORE_EXPORT RefPtr<Attr> getAttributeNode(const AtomicString& name);
    WEBCORE_EXPORT RefPtr<Attr> getAttributeNodeNS(const AtomicString& namespaceURI, const AtomicString& localName);
    WEBCORE_EXPORT RefPtr<Attr> setAttributeNode(Attr&, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<Attr> setAttributeNodeNS(Attr&, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<Attr> removeAttributeNode(Attr&, ExceptionCode&);

    RefPtr<Attr> attrIfExists(const QualifiedName&);
    RefPtr<Attr> attrIfExists(const AtomicString& localName, bool shouldIgnoreAttributeCase);
    Ref<Attr> ensureAttr(const QualifiedName&);

    const Vector<RefPtr<Attr>>& attrNodeList();

    virtual CSSStyleDeclaration* cssomStyle();

    const QualifiedName& tagQName() const { return m_tagName; }
#if ENABLE(CSS_SELECTOR_JIT)
    static ptrdiff_t tagQNameMemoryOffset() { return OBJECT_OFFSETOF(Element, m_tagName); }
#endif // ENABLE(CSS_SELECTOR_JIT)
    String tagName() const { return nodeName(); }
    bool hasTagName(const QualifiedName& tagName) const { return m_tagName.matches(tagName); }
    bool hasTagName(const HTMLQualifiedName& tagName) const { return ContainerNode::hasTagName(tagName); }
    bool hasTagName(const MathMLQualifiedName& tagName) const { return ContainerNode::hasTagName(tagName); }
    bool hasTagName(const SVGQualifiedName& tagName) const { return ContainerNode::hasTagName(tagName); }

    // A fast function for checking the local name against another atomic string.
    bool hasLocalName(const AtomicString& other) const { return m_tagName.localName() == other; }

    const AtomicString& localName() const final { return m_tagName.localName(); }
    const AtomicString& prefix() const final { return m_tagName.prefix(); }
    const AtomicString& namespaceURI() const final { return m_tagName.namespaceURI(); }

    void setPrefix(const AtomicString&, ExceptionCode&) final;

    String nodeName() const override;

    Ref<Element> cloneElementWithChildren(Document&);
    Ref<Element> cloneElementWithoutChildren(Document&);

    void normalizeAttributes();
    String nodeNamePreservingCase() const;

    WEBCORE_EXPORT void setBooleanAttribute(const QualifiedName& name, bool);

    // For exposing to DOM only.
    WEBCORE_EXPORT NamedNodeMap& attributes() const;

    enum AttributeModificationReason {
        ModifiedDirectly,
        ModifiedByCloning
    };

    // This method is called whenever an attribute is added, changed or removed.
    virtual void attributeChanged(const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue, AttributeModificationReason = ModifiedDirectly);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) { }

    // Only called by the parser immediately after element construction.
    void parserSetAttributes(const Vector<Attribute>&);

    // Remove attributes that might introduce scripting from the vector leaving the element unchanged.
    void stripScriptingAttributes(Vector<Attribute>&) const;

    const ElementData* elementData() const { return m_elementData.get(); }
    static ptrdiff_t elementDataMemoryOffset() { return OBJECT_OFFSETOF(Element, m_elementData); }
    UniqueElementData& ensureUniqueElementData();

    void synchronizeAllAttributes() const;

    // Clones attributes only.
    void cloneAttributesFromElement(const Element&);

    // Clones all attribute-derived data, including subclass specifics (through copyNonAttributeProperties.)
    void cloneDataFromElement(const Element&);

    bool hasEquivalentAttributes(const Element* other) const;

    virtual void copyNonAttributePropertiesFromElement(const Element&) { }

    virtual RenderPtr<RenderElement> createElementRenderer(RenderStyle&&, const RenderTreePosition&);
    virtual bool rendererIsNeeded(const RenderStyle&);

    WEBCORE_EXPORT ShadowRoot* shadowRoot() const;
    WEBCORE_EXPORT RefPtr<ShadowRoot> createShadowRoot(ExceptionCode&);

    enum class ShadowRootMode { Open, Closed };
    struct ShadowRootInit {
        ShadowRootMode mode;
    };

    ShadowRoot* shadowRootForBindings(JSC::ExecState&) const;
    RefPtr<ShadowRoot> attachShadow(const ShadowRootInit&, ExceptionCode&);

    ShadowRoot* userAgentShadowRoot() const;
    WEBCORE_EXPORT ShadowRoot& ensureUserAgentShadowRoot();

#if ENABLE(CUSTOM_ELEMENTS)
    void setCustomElementIsResolved(JSCustomElementInterface&);
    JSCustomElementInterface* customElementInterface() const;
#endif

    // FIXME: this should not be virtual, do not override this.
    virtual const AtomicString& shadowPseudoId() const;

    bool inActiveChain() const { return isUserActionElement() && isUserActionElementInActiveChain(); }
    bool active() const { return isUserActionElement() && isUserActionElementActive(); }
    bool hovered() const { return isUserActionElement() && isUserActionElementHovered(); }
    bool focused() const { return isUserActionElement() && isUserActionElementFocused(); }
    bool hasFocusWithin() const { return getFlag(HasFocusWithin); };

    virtual void setActive(bool flag = true, bool pause = false);
    virtual void setHovered(bool flag = true);
    virtual void setFocus(bool flag);
    void setHasFocusWithin(bool flag);

    bool tabIndexSetExplicitly() const;
    virtual bool supportsFocus() const;
    virtual bool isFocusable() const;
    virtual bool isKeyboardFocusable(KeyboardEvent&) const;
    virtual bool isMouseFocusable() const;

    virtual bool shouldUseInputMethod();

    virtual int tabIndex() const;
    WEBCORE_EXPORT void setTabIndex(int);
    virtual Element* focusDelegate();

    WEBCORE_EXPORT Element* insertAdjacentElement(const String& where, Element& newChild, ExceptionCode&);
    WEBCORE_EXPORT void insertAdjacentHTML(const String& where, const String& html, ExceptionCode&);
    WEBCORE_EXPORT void insertAdjacentText(const String& where, const String& text, ExceptionCode&);

    bool ieForbidsInsertHTML() const;

    const RenderStyle* computedStyle(PseudoId = NOPSEUDO) override;

    bool needsStyleInvalidation() const;

    // Methods for indicating the style is affected by dynamic updates (e.g., children changing, our position changing in our sibling list, etc.)
    bool styleAffectedByActive() const { return hasRareData() && rareDataStyleAffectedByActive(); }
    bool styleAffectedByEmpty() const { return hasRareData() && rareDataStyleAffectedByEmpty(); }
    bool styleAffectedByFocusWithin() const { return hasRareData() && rareDataStyleAffectedByFocusWithin(); }
    bool childrenAffectedByHover() const { return getFlag(ChildrenAffectedByHoverRulesFlag); }
    bool childrenAffectedByDrag() const { return hasRareData() && rareDataChildrenAffectedByDrag(); }
    bool childrenAffectedByFirstChildRules() const { return getFlag(ChildrenAffectedByFirstChildRulesFlag); }
    bool childrenAffectedByLastChildRules() const { return getFlag(ChildrenAffectedByLastChildRulesFlag); }
    bool childrenAffectedByBackwardPositionalRules() const { return hasRareData() && rareDataChildrenAffectedByBackwardPositionalRules(); }
    bool childrenAffectedByPropertyBasedBackwardPositionalRules() const { return hasRareData() && rareDataChildrenAffectedByPropertyBasedBackwardPositionalRules(); }
    bool affectsNextSiblingElementStyle() const { return getFlag(AffectsNextSiblingElementStyle); }
    unsigned childIndex() const { return hasRareData() ? rareDataChildIndex() : 0; }

    bool hasFlagsSetDuringStylingOfChildren() const;

    void setStyleAffectedByEmpty();
    void setStyleAffectedByFocusWithin();
    void setChildrenAffectedByHover() { setFlag(ChildrenAffectedByHoverRulesFlag); }
    void setStyleAffectedByActive();
    void setChildrenAffectedByDrag();
    void setChildrenAffectedByFirstChildRules() { setFlag(ChildrenAffectedByFirstChildRulesFlag); }
    void setChildrenAffectedByLastChildRules() { setFlag(ChildrenAffectedByLastChildRulesFlag); }
    void setChildrenAffectedByBackwardPositionalRules();
    void setChildrenAffectedByPropertyBasedBackwardPositionalRules();
    void setAffectsNextSiblingElementStyle() { setFlag(AffectsNextSiblingElementStyle); }
    void setStyleIsAffectedByPreviousSibling() { setFlag(StyleIsAffectedByPreviousSibling); }
    void setChildIndex(unsigned);

    void setRegionOversetState(RegionOversetState);
    RegionOversetState regionOversetState() const;

    AtomicString computeInheritedLanguage() const;
    Locale& locale() const;

    virtual void accessKeyAction(bool /*sendToAnyEvent*/) { }

    virtual bool isURLAttribute(const Attribute&) const { return false; }
    virtual bool attributeContainsURL(const Attribute& attribute) const { return isURLAttribute(attribute); }
    virtual String completeURLsInAttributeValue(const URL& base, const Attribute&) const;
    virtual bool isHTMLContentAttribute(const Attribute&) const { return false; }

    WEBCORE_EXPORT URL getURLAttribute(const QualifiedName&) const;
    URL getNonEmptyURLAttribute(const QualifiedName&) const;

    virtual const AtomicString& imageSourceURL() const;
    virtual String target() const { return String(); }

    static AXTextStateChangeIntent defaultFocusTextStateChangeIntent() { return AXTextStateChangeIntent(AXTextStateChangeTypeSelectionMove, AXTextSelection { AXTextSelectionDirectionDiscontiguous, AXTextSelectionGranularityUnknown, true }); }
    void updateFocusAppearanceAfterAttachIfNeeded();
    virtual void focus(bool restorePreviousSelection = true, FocusDirection = FocusDirectionNone);
    virtual void updateFocusAppearance(SelectionRestorationMode, SelectionRevealMode = SelectionRevealMode::Reveal);
    virtual void blur();

    WEBCORE_EXPORT String innerHTML() const;
    WEBCORE_EXPORT String outerHTML() const;
    WEBCORE_EXPORT void setInnerHTML(const String&, ExceptionCode&);
    WEBCORE_EXPORT void setOuterHTML(const String&, ExceptionCode&);
    WEBCORE_EXPORT String innerText();
    WEBCORE_EXPORT String outerText();
 
    virtual String title() const;

    const AtomicString& pseudo() const;
    WEBCORE_EXPORT void setPseudo(const AtomicString&);

    LayoutSize minimumSizeForResizing() const;
    void setMinimumSizeForResizing(const LayoutSize&);

    // Use Document::registerForDocumentActivationCallbacks() to subscribe to these
    virtual void prepareForDocumentSuspension() { }
    virtual void resumeFromDocumentSuspension() { }

    // Use Document::registerForMediaVolumeCallbacks() to subscribe to this
    virtual void mediaVolumeDidChange() { }

    // Use Document::registerForPrivateBrowsingStateChangedCallbacks() to subscribe to this.
    virtual void privateBrowsingStateDidChange() { }

    virtual void willBecomeFullscreenElement();
    virtual void ancestorWillEnterFullscreen() { }
    virtual void didBecomeFullscreenElement() { }
    virtual void willStopBeingFullscreenElement() { }

    // Use Document::registerForVisibilityStateChangedCallbacks() to subscribe to this.
    virtual void visibilityStateChanged() { }

#if ENABLE(VIDEO_TRACK)
    virtual void captionPreferencesChanged() { }
#endif

    bool isFinishedParsingChildren() const { return isParsingChildrenFinished(); }
    void finishParsingChildren() override;
    void beginParsingChildren() final;

    WEBCORE_EXPORT PseudoElement* beforePseudoElement() const;
    WEBCORE_EXPORT PseudoElement* afterPseudoElement() const;
    bool childNeedsShadowWalker() const;
    void didShadowTreeAwareChildrenChange();

    virtual bool matchesValidPseudoClass() const;
    virtual bool matchesInvalidPseudoClass() const;
    virtual bool matchesReadWritePseudoClass() const;
    virtual bool matchesIndeterminatePseudoClass() const;
    virtual bool matchesDefaultPseudoClass() const;
    WEBCORE_EXPORT bool matches(const String& selectors, ExceptionCode&);
    WEBCORE_EXPORT Element* closest(const String& selectors, ExceptionCode&);
    virtual bool shouldAppearIndeterminate() const;

    WEBCORE_EXPORT DOMTokenList& classList();

    DatasetDOMStringMap& dataset();

#if ENABLE(VIDEO)
    virtual bool isMediaElement() const { return false; }
#endif

    virtual bool isFormControlElement() const { return false; }
    virtual bool isSpinButtonElement() const { return false; }
    virtual bool isTextFormControl() const { return false; }
    virtual bool isOptionalFormControl() const { return false; }
    virtual bool isRequiredFormControl() const { return false; }
    virtual bool isInRange() const { return false; }
    virtual bool isOutOfRange() const { return false; }
    virtual bool isFrameElementBase() const { return false; }
    virtual bool isUploadButton() const { return false; }
    virtual bool isSliderContainerElement() const { return false; }

    bool canContainRangeEndPoint() const override;

    // Used for disabled form elements; if true, prevents mouse events from being dispatched
    // to event listeners, and prevents DOMActivate events from being sent at all.
    virtual bool isDisabledFormControl() const { return false; }

    virtual bool childShouldCreateRenderer(const Node&) const;

    bool hasPendingResources() const;
    void setHasPendingResources();
    void clearHasPendingResources();
    virtual void buildPendingResource() { };

#if ENABLE(FULLSCREEN_API)
    enum {
        ALLOW_KEYBOARD_INPUT = 1 << 0,
        LEGACY_MOZILLA_REQUEST = 1 << 1,
    };
    
    WEBCORE_EXPORT void webkitRequestFullScreen(unsigned short flags);
    WEBCORE_EXPORT bool containsFullScreenElement() const;
    void setContainsFullScreenElement(bool);
    void setContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(bool);

    // W3C API
    WEBCORE_EXPORT void webkitRequestFullscreen();
#endif

#if ENABLE(POINTER_LOCK)
    void requestPointerLock();
#endif

#if ENABLE(INDIE_UI)
    void setUIActions(const AtomicString&);
    const AtomicString& UIActions() const;
#endif
    
    virtual bool isSpellCheckingEnabled() const;

    RenderNamedFlowFragment* renderNamedFlowFragment() const;

#if ENABLE(CSS_REGIONS)
    virtual bool shouldMoveToFlowThread(const RenderStyle&) const;
    
    WEBCORE_EXPORT const AtomicString& webkitRegionOverset() const;
    Vector<RefPtr<Range>> webkitGetRegionFlowRanges() const;
#endif

    bool hasID() const;
    bool hasClass() const;
    bool hasName() const;
    const SpaceSplitString& classNames() const;

    IntPoint savedLayerScrollPosition() const;
    void setSavedLayerScrollPosition(const IntPoint&);

    bool dispatchMouseEvent(const PlatformMouseEvent&, const AtomicString& eventType, int clickCount = 0, Element* relatedTarget = nullptr);
    bool dispatchWheelEvent(const PlatformWheelEvent&);
    bool dispatchKeyEvent(const PlatformKeyboardEvent&);
    void dispatchSimulatedClick(Event* underlyingEvent, SimulatedClickMouseEventOptions = SendNoEvents, SimulatedClickVisualOptions = ShowPressedLook);
    void dispatchSimulatedClickForBindings(Event* underlyingEvent);
    void dispatchFocusInEvent(const AtomicString& eventType, RefPtr<Element>&& oldFocusedElement);
    void dispatchFocusOutEvent(const AtomicString& eventType, RefPtr<Element>&& newFocusedElement);
    virtual void dispatchFocusEvent(RefPtr<Element>&& oldFocusedElement, FocusDirection);
    virtual void dispatchBlurEvent(RefPtr<Element>&& newFocusedElement);

    WEBCORE_EXPORT bool dispatchMouseForceWillBegin();

    virtual bool willRecalcStyle(Style::Change);
    virtual void didRecalcStyle(Style::Change);
    virtual void willResetComputedStyle();
    virtual void willAttachRenderers();
    virtual void didAttachRenderers();
    virtual void willDetachRenderers();
    virtual void didDetachRenderers();
    virtual Optional<ElementStyle> resolveCustomStyle(const RenderStyle& parentStyle, const RenderStyle* shadowHostStyle);

    LayoutRect absoluteEventHandlerBounds(bool& includesFixedPositionElements) override;

    void setBeforePseudoElement(Ref<PseudoElement>&&);
    void setAfterPseudoElement(Ref<PseudoElement>&&);
    void clearBeforePseudoElement();
    void clearAfterPseudoElement();
    void resetComputedStyle();
    void clearStyleDerivedDataBeforeDetachingRenderer();
    void clearHoverAndActiveStatusBeforeDetachingRenderer();

    WEBCORE_EXPORT URL absoluteLinkURL() const;

#if ENABLE(TOUCH_EVENTS)
    bool allowsDoubleTapGesture() const override;
#endif

    StyleResolver& styleResolver();
    ElementStyle resolveStyle(const RenderStyle* parentStyle);

    bool hasDisplayContents() const;
    void setHasDisplayContents(bool);

    virtual void isVisibleInViewportChanged() { }

    using ContainerNode::setAttributeEventListener;
    void setAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value);

    bool isNamedFlowContentElement() const { return hasRareData() && rareDataIsNamedFlowContentElement(); }
    void setIsNamedFlowContentElement();
    void clearIsNamedFlowContentElement();

#if ENABLE(WEB_ANIMATIONS)
    Vector<WebAnimation*> getAnimations();
#endif

protected:
    Element(const QualifiedName&, Document&, ConstructionType);

    InsertionNotificationRequest insertedInto(ContainerNode&) override;
    void removedFrom(ContainerNode&) override;
    void childrenChanged(const ChildChange&) override;
    void removeAllEventListeners() final;
    virtual void parserDidSetAttributes();
    void didMoveToNewDocument(Document*) override;

    void clearTabIndexExplicitlyIfNeeded();
    void setTabIndexExplicitly(int);

    // classAttributeChanged() exists to share code between
    // parseAttribute (called via setAttribute()) and
    // svgAttributeChanged (called when element.className.baseValue is set)
    void classAttributeChanged(const AtomicString& newClassString);

    void addShadowRoot(Ref<ShadowRoot>&&);

    static void mergeWithNextTextNode(Text& node, ExceptionCode&);

private:
    bool isTextNode() const;

    bool isUserActionElementInActiveChain() const;
    bool isUserActionElementActive() const;
    bool isUserActionElementFocused() const;
    bool isUserActionElementHovered() const;

    virtual void didAddUserAgentShadowRoot(ShadowRoot*) { }
    virtual bool alwaysCreateUserAgentShadowRoot() const { return false; }

    // FIXME: Remove the need for Attr to call willModifyAttribute/didModifyAttribute.
    friend class Attr;

    enum SynchronizationOfLazyAttribute { NotInSynchronizationOfLazyAttribute = 0, InSynchronizationOfLazyAttribute };

    void didAddAttribute(const QualifiedName&, const AtomicString&);
    void willModifyAttribute(const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue);
    void didModifyAttribute(const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue);
    void didRemoveAttribute(const QualifiedName&, const AtomicString& oldValue);

    void synchronizeAttribute(const QualifiedName&) const;
    void synchronizeAttribute(const AtomicString& localName) const;

    void updateName(const AtomicString& oldName, const AtomicString& newName);
    void updateNameForTreeScope(TreeScope&, const AtomicString& oldName, const AtomicString& newName);
    void updateNameForDocument(HTMLDocument&, const AtomicString& oldName, const AtomicString& newName);

    enum class NotifyObservers { No, Yes };
    void updateId(const AtomicString& oldId, const AtomicString& newId, NotifyObservers = NotifyObservers::Yes);
    void updateIdForTreeScope(TreeScope&, const AtomicString& oldId, const AtomicString& newId, NotifyObservers = NotifyObservers::Yes);

    enum HTMLDocumentNamedItemMapsUpdatingCondition { AlwaysUpdateHTMLDocumentNamedItemMaps, UpdateHTMLDocumentNamedItemMapsOnlyIfDiffersFromNameAttribute };
    void updateIdForDocument(HTMLDocument&, const AtomicString& oldId, const AtomicString& newId, HTMLDocumentNamedItemMapsUpdatingCondition);
    void updateLabel(TreeScope&, const AtomicString& oldForAttributeValue, const AtomicString& newForAttributeValue);

    Node* insertAdjacent(const String& where, Ref<Node>&& newChild, ExceptionCode&);

    void scrollByUnits(int units, ScrollGranularity);

    NodeType nodeType() const final;
    bool childTypeAllowed(NodeType) const final;

    void setAttributeInternal(unsigned index, const QualifiedName&, const AtomicString& value, SynchronizationOfLazyAttribute);
    void addAttributeInternal(const QualifiedName&, const AtomicString& value, SynchronizationOfLazyAttribute);
    void removeAttributeInternal(unsigned index, SynchronizationOfLazyAttribute);

    LayoutRect absoluteEventBounds(bool& boundsIncludeAllDescendantElements, bool& includesFixedPositionElements);
    LayoutRect absoluteEventBoundsOfElementAndDescendants(bool& includesFixedPositionElements);
    
#if ENABLE(TREE_DEBUGGING)
    void formatForDebugger(char* buffer, unsigned length) const override;
#endif

    void cancelFocusAppearanceUpdate();

    // The cloneNode function is private so that non-virtual cloneElementWith/WithoutChildren are used instead.
    Ref<Node> cloneNodeInternal(Document&, CloningOperation) override;
    virtual Ref<Element> cloneElementWithoutAttributesAndChildren(Document&);

    void removeShadowRoot();

    const RenderStyle* existingComputedStyle();
    const RenderStyle& resolveComputedStyle();

    bool rareDataStyleAffectedByEmpty() const;
    bool rareDataStyleAffectedByFocusWithin() const;
    bool rareDataIsNamedFlowContentElement() const;
    bool rareDataChildrenAffectedByHover() const;
    bool rareDataStyleAffectedByActive() const;
    bool rareDataChildrenAffectedByDrag() const;
    bool rareDataChildrenAffectedByLastChildRules() const;
    bool rareDataChildrenAffectedByBackwardPositionalRules() const;
    bool rareDataChildrenAffectedByPropertyBasedBackwardPositionalRules() const;
    unsigned rareDataChildIndex() const;

    SpellcheckAttributeState spellcheckAttributeState() const;

    void unregisterNamedFlowContentElement();

    void createUniqueElementData();

    ElementRareData* elementRareData() const;
    ElementRareData& ensureElementRareData();

    void detachAllAttrNodesFromElement();
    void detachAttrNodeFromElementWithValue(Attr*, const AtomicString& value);

    bool isJavaScriptURLAttribute(const Attribute&) const;

    // Anyone thinking of using this should call document instead of ownerDocument.
    void ownerDocument() const = delete;

    QualifiedName m_tagName;
    RefPtr<ElementData> m_elementData;
};

inline bool Node::hasAttributes() const
{
    return is<Element>(*this) && downcast<Element>(*this).hasAttributes();
}

inline NamedNodeMap* Node::attributes() const
{
    return is<Element>(*this) ? &downcast<Element>(*this).attributes() : nullptr;
}

inline Element* Node::parentElement() const
{
    ContainerNode* parent = parentNode();
    return is<Element>(parent) ? downcast<Element>(parent) : nullptr;
}

inline const Element* Element::rootElement() const
{
    if (inDocument())
        return document().documentElement();

    const Element* highest = this;
    while (highest->parentElement())
        highest = highest->parentElement();
    return highest;
}

inline bool Element::hasAttributeWithoutSynchronization(const QualifiedName& name) const
{
    ASSERT(fastAttributeLookupAllowed(name));
    return elementData() && findAttributeByName(name);
}

inline const AtomicString& Element::attributeWithoutSynchronization(const QualifiedName& name) const
{
    ASSERT(fastAttributeLookupAllowed(name));
    if (elementData()) {
        if (const Attribute* attribute = findAttributeByName(name))
            return attribute->value();
    }
    return nullAtom;
}

inline bool Element::hasAttributesWithoutUpdate() const
{
    return elementData() && !elementData()->isEmpty();
}

inline const AtomicString& Element::idForStyleResolution() const
{
    return hasID() ? elementData()->idForStyleResolution() : nullAtom;
}

inline const AtomicString& Element::getIdAttribute() const
{
    if (hasID())
        return elementData()->findAttributeByName(HTMLNames::idAttr)->value();
    return nullAtom;
}

inline const AtomicString& Element::getNameAttribute() const
{
    if (hasName())
        return elementData()->findAttributeByName(HTMLNames::nameAttr)->value();
    return nullAtom;
}

inline void Element::setIdAttribute(const AtomicString& value)
{
    setAttributeWithoutSynchronization(HTMLNames::idAttr, value);
}

inline const SpaceSplitString& Element::classNames() const
{
    ASSERT(hasClass());
    ASSERT(elementData());
    return elementData()->classNames();
}

inline unsigned Element::attributeCount() const
{
    ASSERT(elementData());
    return elementData()->length();
}

inline const Attribute& Element::attributeAt(unsigned index) const
{
    ASSERT(elementData());
    return elementData()->attributeAt(index);
}

inline const Attribute* Element::findAttributeByName(const QualifiedName& name) const
{
    ASSERT(elementData());
    return elementData()->findAttributeByName(name);
}

inline bool Element::hasID() const
{
    return elementData() && elementData()->hasID();
}

inline bool Element::hasClass() const
{
    return elementData() && elementData()->hasClass();
}

inline bool Element::hasName() const
{
    return elementData() && elementData()->hasName();
}

inline UniqueElementData& Element::ensureUniqueElementData()
{
    if (!elementData() || !elementData()->isUnique())
        createUniqueElementData();
    return static_cast<UniqueElementData&>(*m_elementData);
}

inline bool shouldIgnoreAttributeCase(const Element& element)
{
    return element.isHTMLElement() && element.document().isHTMLDocument();
}

inline void Element::setHasFocusWithin(bool flag)
{
    if (hasFocusWithin() == flag)
        return;
    setFlag(flag, HasFocusWithin);
    if (styleAffectedByFocusWithin())
        setNeedsStyleRecalc();
}

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::Element)
    static bool isType(const WebCore::Node& node) { return node.isElementNode(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif
