/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004-2010, 2012-2013, 2015, 2016 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "CollectionType.h"
#include "Color.h"
#include "ContainerNode.h"
#include "DocumentEventQueue.h"
#include "DocumentTiming.h"
#include "FocusDirection.h"
#include "FontSelectorClient.h"
#include "MediaProducer.h"
#include "MutationObserver.h"
#include "PageVisibilityState.h"
#include "PlatformEvent.h"
#include "ReferrerPolicy.h"
#include "Region.h"
#include "RenderPtr.h"
#include "ScriptExecutionContext.h"
#include "StringWithDirection.h"
#include "StyleChange.h"
#include "Supplementable.h"
#include "TextResourceDecoder.h"
#include "Timer.h"
#include "TreeScope.h"
#include "UserActionElementSet.h"
#include "ViewportArguments.h"
#include <chrono>
#include <memory>
#include <wtf/Deque.h>
#include <wtf/HashCountedSet.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/AtomicStringHash.h>

#if PLATFORM(IOS)
#include "EventTrackingRegions.h"
#endif

namespace JSC {
class ExecState;
#if ENABLE(WEB_REPLAY)
class InputCursor;
#endif
}

namespace WebCore {

class AXObjectCache;
class Attr;
class AuthorStyleSheets;
class CDATASection;
class CSSFontSelector;
class CSSStyleDeclaration;
class CSSStyleSheet;
class CachedCSSStyleSheet;
class CachedResourceLoader;
class CachedScript;
class CanvasRenderingContext;
class CharacterData;
class Comment;
class DOMImplementation;
class DOMNamedFlowCollection;
class DOMSelection;
class DOMWindow;
class DOMWrapperWorld;
class Database;
class DatabaseThread;
class DocumentFragment;
class DocumentLoader;
class DocumentMarkerController;
class DocumentParser;
class DocumentSharedObjectPool;
class DocumentType;
class ExtensionStyleSheets;
class FloatRect;
class FloatQuad;
class FormController;
class Frame;
class FrameView;
class HTMLAllCollection;
class HTMLBodyElement;
class HTMLCanvasElement;
class HTMLCollection;
class HTMLDocument;
class HTMLElement;
class HTMLFrameOwnerElement;
class HTMLHeadElement;
class HTMLIFrameElement;
class HTMLImageElement;
class HTMLMapElement;
class HTMLMediaElement;
class HTMLPictureElement;
class HTMLScriptElement;
class HitTestRequest;
class HitTestResult;
class IntPoint;
class LayoutPoint;
class LayoutRect;
class LiveNodeList;
class ScriptModuleLoader;
class JSNode;
class Locale;
class Location;
class MediaCanStartListener;
class MediaPlaybackTarget;
class MediaPlaybackTargetClient;
class MediaQueryList;
class MediaQueryMatcher;
class ScriptModuleLoader;
class MouseEventWithHitTestResults;
class NamedFlowCollection;
class NodeFilter;
class NodeIterator;
class Page;
class PlatformMouseEvent;
class ProcessingInstruction;
class QualifiedName;
class Range;
class RenderView;
class RenderFullScreen;
class ScriptableDocumentParser;
class ScriptElementData;
class ScriptRunner;
class SecurityOrigin;
class SelectorQuery;
class SelectorQueryCache;
class SerializedScriptValue;
class SegmentedString;
class Settings;
class StyleResolver;
class StyleSheet;
class StyleSheetContents;
class StyleSheetList;
class SVGDocumentExtensions;
class SVGSVGElement;
class Text;
class TextResourceDecoder;
class TreeWalker;
class VisitedLinkState;
class WebKitNamedFlow;
class XMLHttpRequest;
class XPathEvaluator;
class XPathExpression;
class XPathNSResolver;
class XPathResult;

enum class ShouldOpenExternalURLsPolicy;

using PlatformDisplayID = uint32_t;

#if ENABLE(XSLT)
class TransformSource;
#endif

#if ENABLE(DASHBOARD_SUPPORT)
struct AnnotatedRegionValue;
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
#include <WebKitAdditions/DocumentIOSForward.h>
#endif

#if ENABLE(TOUCH_EVENTS) || ENABLE(IOS_TOUCH_EVENTS)
class Touch;
class TouchList;
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
class RequestAnimationFrameCallback;
class ScriptedAnimationController;
#endif

#if ENABLE(TEXT_AUTOSIZING)
class TextAutosizer;
#endif

class FontFaceSet;

typedef int ExceptionCode;

#if PLATFORM(IOS)
class DeviceMotionClient;
class DeviceMotionController;
class DeviceOrientationClient;
class DeviceOrientationController;
#endif

#if ENABLE(IOS_TEXT_AUTOSIZING)
struct TextAutoSizingHash;
class TextAutoSizingKey;
class TextAutoSizingValue;

struct TextAutoSizingTraits : WTF::GenericHashTraits<TextAutoSizingKey> {
    static const bool emptyValueIsZero = true;
    static void constructDeletedValue(TextAutoSizingKey& slot);
    static bool isDeletedValue(const TextAutoSizingKey& value);
};
#endif

#if ENABLE(MEDIA_SESSION)
class MediaSession;
#endif

const uint64_t HTMLMediaElementInvalidID = 0;

enum PageshowEventPersistence {
    PageshowEventNotPersisted = 0,
    PageshowEventPersisted = 1
};

enum StyleResolverUpdateFlag { RecalcStyleImmediately, DeferRecalcStyle, RecalcStyleIfNeeded, DeferRecalcStyleIfNeeded };

enum NodeListInvalidationType {
    DoNotInvalidateOnAttributeChanges = 0,
    InvalidateOnClassAttrChange,
    InvalidateOnIdNameAttrChange,
    InvalidateOnNameAttrChange,
    InvalidateOnForAttrChange,
    InvalidateForFormControls,
    InvalidateOnHRefAttrChange,
    InvalidateOnAnyAttrChange,
};
const int numNodeListInvalidationTypes = InvalidateOnAnyAttrChange + 1;

enum class EventHandlerRemoval { One, All };
typedef HashCountedSet<Node*> EventTargetSet;

enum DocumentClass {
    DefaultDocumentClass = 0,
    HTMLDocumentClass = 1,
    XHTMLDocumentClass = 1 << 1,
    ImageDocumentClass = 1 << 2,
    PluginDocumentClass = 1 << 3,
    MediaDocumentClass = 1 << 4,
    SVGDocumentClass = 1 << 5,
    TextDocumentClass = 1 << 6,
    XMLDocumentClass = 1 << 7,
};

typedef unsigned char DocumentClassFlags;

enum class DocumentCompatibilityMode : unsigned char {
    NoQuirksMode = 1,
    QuirksMode = 1 << 1,
    LimitedQuirksMode = 1 << 2
};

enum DimensionsCheck { WidthDimensionsCheck = 1 << 0, HeightDimensionsCheck = 1 << 1, AllDimensionsCheck = 1 << 2 };

enum class SelectionRestorationMode {
    Restore,
    SetDefault,
};

enum class HttpEquivPolicy {
    Enabled,
    DisabledBySettings,
    DisabledByContentDispositionAttachmentSandbox
};

enum class CustomElementNameValidationStatus { Valid, ConflictsWithBuiltinNames, NoHyphen, ContainsUpperCase };

class Document
    : public ContainerNode
    , public TreeScope
    , public ScriptExecutionContext
    , public FontSelectorClient
    , public Supplementable<Document> {
public:
    static Ref<Document> create(Frame* frame, const URL& url)
    {
        return adoptRef(*new Document(frame, url));
    }

    static Ref<Document> createNonRenderedPlaceholder(Frame* frame, const URL& url)
    {
        return adoptRef(*new Document(frame, url, DefaultDocumentClass, NonRenderedPlaceholder));
    }
    static Ref<Document> create(ScriptExecutionContext&);

    virtual ~Document();

    // Nodes belonging to this document increase referencingNodeCount -
    // these are enough to keep the document from being destroyed, but
    // not enough to keep it from removing its children. This allows a
    // node that outlives its document to still have a valid document
    // pointer without introducing reference cycles.
    void incrementReferencingNodeCount()
    {
        ASSERT(!m_deletionHasBegun);
        ++m_referencingNodeCount;
    }

    void decrementReferencingNodeCount()
    {
        ASSERT(!m_deletionHasBegun || !m_referencingNodeCount);
        --m_referencingNodeCount;
        if (!m_referencingNodeCount && !refCount()) {
#if !ASSERT_DISABLED
            m_deletionHasBegun = true;
#endif
            delete this;
        }
    }

    unsigned referencingNodeCount() const { return m_referencingNodeCount; }

    void removedLastRef();

    WEBCORE_EXPORT static HashSet<Document*>& allDocuments();

    MediaQueryMatcher& mediaQueryMatcher();

    using ContainerNode::ref;
    using ContainerNode::deref;
    using TreeScope::rootNode;

    bool canContainRangeEndPoint() const final { return true; }

    Element* getElementByAccessKey(const String& key);
    void invalidateAccessKeyMap();

    void addImageElementByCaseFoldedUsemap(const AtomicStringImpl&, HTMLImageElement&);
    void removeImageElementByCaseFoldedUsemap(const AtomicStringImpl&, HTMLImageElement&);
    HTMLImageElement* imageElementByCaseFoldedUsemap(const AtomicStringImpl&) const;

    SelectorQuery* selectorQueryForString(const String&, ExceptionCode&);
    void clearSelectorQueryCache();

    // DOM methods & attributes for Document

    void setViewportArguments(const ViewportArguments& viewportArguments) { m_viewportArguments = viewportArguments; }
    ViewportArguments viewportArguments() const { return m_viewportArguments; }
#ifndef NDEBUG
    bool didDispatchViewportPropertiesChanged() const { return m_didDispatchViewportPropertiesChanged; }
#endif

    void setReferrerPolicy(ReferrerPolicy referrerPolicy) { m_referrerPolicy = referrerPolicy; }
    ReferrerPolicy referrerPolicy() const { return m_referrerPolicy; }

    WEBCORE_EXPORT DocumentType* doctype() const;

    WEBCORE_EXPORT DOMImplementation& implementation();
    
    Element* documentElement() const
    {
        return m_documentElement.get();
    }
    static ptrdiff_t documentElementMemoryOffset() { return OBJECT_OFFSETOF(Document, m_documentElement); }

    WEBCORE_EXPORT Element* activeElement();
    WEBCORE_EXPORT bool hasFocus() const;

    bool hasManifest() const;

    bool hasEverCalledWindowOpen() const;
    void markHasCalledWindowOpen();
    
    WEBCORE_EXPORT RefPtr<Element> createElementForBindings(const AtomicString& tagName, ExceptionCode&);
    WEBCORE_EXPORT Ref<DocumentFragment> createDocumentFragment();
    WEBCORE_EXPORT Ref<Text> createTextNode(const String& data);
    WEBCORE_EXPORT Ref<Comment> createComment(const String& data);
    WEBCORE_EXPORT RefPtr<CDATASection> createCDATASection(const String& data, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<ProcessingInstruction> createProcessingInstruction(const String& target, const String& data, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<Attr> createAttribute(const String& name, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<Attr> createAttributeNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode&, bool shouldIgnoreNamespaceChecks = false);
    WEBCORE_EXPORT RefPtr<Node> importNode(Node& nodeToImport, bool deep, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<Element> createElementNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode&);
    WEBCORE_EXPORT Ref<Element> createElement(const QualifiedName&, bool createdByParser);

    static CustomElementNameValidationStatus validateCustomElementName(const AtomicString&);

#if ENABLE(CSS_GRID_LAYOUT)
    bool isCSSGridLayoutEnabled() const;
#endif
#if ENABLE(CSS_REGIONS)
    RefPtr<DOMNamedFlowCollection> webkitGetNamedFlows();
#endif

    NamedFlowCollection& namedFlows();

    Element* elementFromPoint(int x, int y) { return elementFromPoint(LayoutPoint(x, y)); }
    WEBCORE_EXPORT Element* elementFromPoint(const LayoutPoint& clientPoint);

    WEBCORE_EXPORT RefPtr<Range> caretRangeFromPoint(int x, int y);
    RefPtr<Range> caretRangeFromPoint(const LayoutPoint& clientPoint);

    WEBCORE_EXPORT Element* scrollingElement();

    WEBCORE_EXPORT String readyState() const;

    WEBCORE_EXPORT String defaultCharsetForBindings() const;

    String charset() const { return Document::encoding(); }
    WEBCORE_EXPORT String characterSetWithUTF8Fallback() const;
    TextEncoding textEncoding() const;

    AtomicString encoding() const { return textEncoding().domName(); }

    WEBCORE_EXPORT void setCharset(const String&); // Used by ObjC / GOBject bindings only.

    void setContent(const String&);

    String suggestedMIMEType() const;

    void overrideMIMEType(const String&);
    WEBCORE_EXPORT String contentType() const;

    String contentLanguage() const { return m_contentLanguage; }
    void setContentLanguage(const String&);

    String xmlEncoding() const { return m_xmlEncoding; }
    String xmlVersion() const { return m_xmlVersion; }
    enum StandaloneStatus { StandaloneUnspecified, Standalone, NotStandalone };
    bool xmlStandalone() const { return m_xmlStandalone == Standalone; }
    StandaloneStatus xmlStandaloneStatus() const { return static_cast<StandaloneStatus>(m_xmlStandalone); }
    bool hasXMLDeclaration() const { return m_hasXMLDeclaration; }

    void setXMLEncoding(const String& encoding) { m_xmlEncoding = encoding; } // read-only property, only to be set from XMLDocumentParser
    WEBCORE_EXPORT void setXMLVersion(const String&, ExceptionCode&);
    WEBCORE_EXPORT void setXMLStandalone(bool, ExceptionCode&);
    void setHasXMLDeclaration(bool hasXMLDeclaration) { m_hasXMLDeclaration = hasXMLDeclaration ? 1 : 0; }

    String documentURI() const { return m_documentURI; }
    WEBCORE_EXPORT void setDocumentURI(const String&);

#if ENABLE(WEB_REPLAY)
    JSC::InputCursor& inputCursor() const { return *m_inputCursor; }
    void setInputCursor(PassRefPtr<JSC::InputCursor>);
#endif

    void visibilityStateChanged();
    WEBCORE_EXPORT String visibilityState() const;
    WEBCORE_EXPORT bool hidden() const;

    void setTimerThrottlingEnabled(bool);
    bool isTimerThrottlingEnabled() const { return m_isTimerThrottlingEnabled; }

    WEBCORE_EXPORT RefPtr<Node> adoptNode(Node& source, ExceptionCode&);

    WEBCORE_EXPORT Ref<HTMLCollection> images();
    WEBCORE_EXPORT Ref<HTMLCollection> embeds();
    WEBCORE_EXPORT Ref<HTMLCollection> plugins(); // an alias for embeds() required for the JS DOM bindings.
    WEBCORE_EXPORT Ref<HTMLCollection> applets();
    WEBCORE_EXPORT Ref<HTMLCollection> links();
    WEBCORE_EXPORT Ref<HTMLCollection> forms();
    WEBCORE_EXPORT Ref<HTMLCollection> anchors();
    WEBCORE_EXPORT Ref<HTMLCollection> scripts();
    Ref<HTMLCollection> all();

    Ref<HTMLCollection> windowNamedItems(const AtomicString& name);
    Ref<HTMLCollection> documentNamedItems(const AtomicString& name);

    // Other methods (not part of DOM)
    bool isSynthesized() const { return m_isSynthesized; }
    bool isHTMLDocument() const { return m_documentClasses & HTMLDocumentClass; }
    bool isXHTMLDocument() const { return m_documentClasses & XHTMLDocumentClass; }
    bool isXMLDocument() const { return m_documentClasses & XMLDocumentClass; }
    bool isImageDocument() const { return m_documentClasses & ImageDocumentClass; }
    bool isSVGDocument() const { return m_documentClasses & SVGDocumentClass; }
    bool isPluginDocument() const { return m_documentClasses & PluginDocumentClass; }
    bool isMediaDocument() const { return m_documentClasses & MediaDocumentClass; }
    bool isTextDocument() const { return m_documentClasses & TextDocumentClass; }
    bool hasSVGRootNode() const;
    virtual bool isFrameSet() const { return false; }

    static ptrdiff_t documentClassesMemoryOffset() { return OBJECT_OFFSETOF(Document, m_documentClasses); }
    static uint32_t isHTMLDocumentClassFlag() { return HTMLDocumentClass; }

    bool isSrcdocDocument() const { return m_isSrcdocDocument; }

    StyleResolver* styleResolverIfExists() const { return m_styleResolver.get(); }

    bool sawElementsInKnownNamespaces() const { return m_sawElementsInKnownNamespaces; }

    StyleResolver& ensureStyleResolver()
    { 
        if (!m_styleResolver)
            createStyleResolver();
        return *m_styleResolver;
    }
    StyleResolver& userAgentShadowTreeStyleResolver();

    CSSFontSelector& fontSelector() { return m_fontSelector; }

    void notifyRemovePendingSheetIfNeeded();

    WEBCORE_EXPORT bool haveStylesheetsLoaded() const;

    WEBCORE_EXPORT StyleSheetList& styleSheets();

    AuthorStyleSheets& authorStyleSheets() { return *m_authorStyleSheets; }
    const AuthorStyleSheets& authorStyleSheets() const { return *m_authorStyleSheets; }
    ExtensionStyleSheets& extensionStyleSheets() { return *m_extensionStyleSheets; }
    const ExtensionStyleSheets& extensionStyleSheets() const { return *m_extensionStyleSheets; }

    bool gotoAnchorNeededAfterStylesheetsLoad() { return m_gotoAnchorNeededAfterStylesheetsLoad; }
    void setGotoAnchorNeededAfterStylesheetsLoad(bool b) { m_gotoAnchorNeededAfterStylesheetsLoad = b; }

    /**
     * Called when one or more stylesheets in the document may have been added, removed or changed.
     *
     * Creates a new style resolver and assign it to this document. This is done by iterating through all nodes in
     * document (or those before <BODY> in a HTML document), searching for stylesheets. Stylesheets can be contained in
     * <LINK>, <STYLE> or <BODY> elements, as well as processing instructions (XML documents only). A list is
     * constructed from these which is used to create the a new style selector which collates all of the stylesheets
     * found and is used to calculate the derived styles for all rendering objects.
     */
    WEBCORE_EXPORT void styleResolverChanged(StyleResolverUpdateFlag);

    void scheduleOptimizedStyleSheetUpdate();

    void evaluateMediaQueryList();

    FormController& formController();
    Vector<String> formElementsState() const;
    void setStateForNewFormElements(const Vector<String>&);

    WEBCORE_EXPORT FrameView* view() const; // can be NULL
    Frame* frame() const { return m_frame; } // can be NULL
    WEBCORE_EXPORT Page* page() const; // can be NULL
    WEBCORE_EXPORT Settings* settings() const; // can be NULL

    float deviceScaleFactor() const;

    WEBCORE_EXPORT Ref<Range> createRange();

    // The last bool parameter is for ObjC bindings.
    WEBCORE_EXPORT Ref<NodeIterator> createNodeIterator(Node& root, unsigned long whatToShow = 0xFFFFFFFF, RefPtr<NodeFilter>&& = nullptr, bool = false);

    // The last bool parameter is for ObjC bindings.
    WEBCORE_EXPORT Ref<TreeWalker> createTreeWalker(Node& root, unsigned long whatToShow = 0xFFFFFFFF, RefPtr<NodeFilter>&& = nullptr, bool = false);

    // Special support for editing
    WEBCORE_EXPORT Ref<CSSStyleDeclaration> createCSSStyleDeclaration();
    Ref<Text> createEditingTextNode(const String&);

    void recalcStyle(Style::Change = Style::NoChange);
    WEBCORE_EXPORT void updateStyleIfNeeded();
    bool needsStyleRecalc() const { return !inPageCache() && (m_pendingStyleRecalcShouldForce || childNeedsStyleRecalc() || m_optimizedStyleSheetUpdateTimer.isActive()); }
    unsigned lastStyleUpdateSizeForTesting() const { return m_lastStyleUpdateSizeForTesting; }

    WEBCORE_EXPORT void updateLayout();
    
    // updateLayoutIgnorePendingStylesheets() forces layout even if we are waiting for pending stylesheet loads,
    // so calling this may cause a flash of unstyled content (FOUC).
    enum class RunPostLayoutTasks {
        Asynchronously,
        Synchronously,
    };
    WEBCORE_EXPORT void updateLayoutIgnorePendingStylesheets(RunPostLayoutTasks = RunPostLayoutTasks::Asynchronously);

    std::unique_ptr<RenderStyle> styleForElementIgnoringPendingStylesheets(Element&, const RenderStyle* parentStyle);

    // Returns true if page box (margin boxes and page borders) is visible.
    WEBCORE_EXPORT bool isPageBoxVisible(int pageIndex);

    // Returns the preferred page size and margins in pixels, assuming 96
    // pixels per inch. pageSize, marginTop, marginRight, marginBottom,
    // marginLeft must be initialized to the default values that are used if
    // auto is specified.
    WEBCORE_EXPORT void pageSizeAndMarginsInPixels(int pageIndex, IntSize& pageSize, int& marginTop, int& marginRight, int& marginBottom, int& marginLeft);

    CachedResourceLoader& cachedResourceLoader() { return m_cachedResourceLoader; }

    void didBecomeCurrentDocumentInFrame();
    void destroyRenderTree();
    void disconnectFromFrame();
    void prepareForDestruction();

    // Override ScriptExecutionContext methods to do additional work
    bool shouldBypassMainWorldContentSecurityPolicy() const final;
    void suspendActiveDOMObjects(ActiveDOMObject::ReasonForSuspension) final;
    void resumeActiveDOMObjects(ActiveDOMObject::ReasonForSuspension) final;
    void stopActiveDOMObjects() final;

    void suspendDeviceMotionAndOrientationUpdates();
    void resumeDeviceMotionAndOrientationUpdates();

    RenderView* renderView() const { return m_renderView.get(); }

    bool renderTreeBeingDestroyed() const { return m_renderTreeBeingDestroyed; }
    bool hasLivingRenderTree() const { return renderView() && !renderTreeBeingDestroyed(); }
    
    bool updateLayoutIfDimensionsOutOfDate(Element&, DimensionsCheck = AllDimensionsCheck);
    
    AXObjectCache* existingAXObjectCache() const;
    WEBCORE_EXPORT AXObjectCache* axObjectCache() const;
    void clearAXObjectCache();

    // to get visually ordered hebrew and arabic pages right
    void setVisuallyOrdered();
    bool visuallyOrdered() const { return m_visuallyOrdered; }
    
    WEBCORE_EXPORT DocumentLoader* loader() const;

    WEBCORE_EXPORT void open(Document* ownerDocument = nullptr);
    void implicitOpen();

    // close() is the DOM API document.close()
    WEBCORE_EXPORT void close();
    // In some situations (see the code), we ignore document.close().
    // explicitClose() bypass these checks and actually tries to close the
    // input stream.
    void explicitClose();
    // implicitClose() actually does the work of closing the input stream.
    void implicitClose();

    void cancelParsing();

    void write(const SegmentedString& text, Document* ownerDocument = nullptr);
    WEBCORE_EXPORT void write(const String& text, Document* ownerDocument = nullptr);
    WEBCORE_EXPORT void writeln(const String& text, Document* ownerDocument = nullptr);

    bool wellFormed() const { return m_wellFormed; }

    const URL& url() const final { return m_url; }
    void setURL(const URL&);
    const URL& urlForBindings() const { return m_url.isEmpty() ? blankURL() : m_url; }

    // To understand how these concepts relate to one another, please see the
    // comments surrounding their declaration.
    const URL& baseURL() const { return m_baseURL; }
    void setBaseURLOverride(const URL&);
    const URL& baseURLOverride() const { return m_baseURLOverride; }
    const URL& baseElementURL() const { return m_baseElementURL; }
    const String& baseTarget() const { return m_baseTarget; }
    void processBaseElement();

    WEBCORE_EXPORT URL completeURL(const String&) const final;
    URL completeURL(const String&, const URL& baseURLOverride) const;

    String userAgent(const URL&) const final;

    void disableEval(const String& errorMessage) final;

#if ENABLE(INDEXED_DATABASE)
    IDBClient::IDBConnectionProxy* idbConnectionProxy() final;
#endif
#if ENABLE(WEB_SOCKETS)
    SocketProvider* socketProvider() final;
#endif

    bool canNavigate(Frame* targetFrame);
    Frame* findUnsafeParentScrollPropagationBoundary();

    CSSStyleSheet& elementSheet();
    bool usesStyleBasedEditability() const;
    
    virtual Ref<DocumentParser> createParser();
    DocumentParser* parser() const { return m_parser.get(); }
    ScriptableDocumentParser* scriptableDocumentParser() const;
    
    bool printing() const { return m_printing; }
    void setPrinting(bool p) { m_printing = p; }

    bool paginatedForScreen() const { return m_paginatedForScreen; }
    void setPaginatedForScreen(bool p) { m_paginatedForScreen = p; }
    
    bool paginated() const { return printing() || paginatedForScreen(); }

    void setCompatibilityMode(DocumentCompatibilityMode);
    void lockCompatibilityMode() { m_compatibilityModeLocked = true; }
    static ptrdiff_t compatibilityModeMemoryOffset() { return OBJECT_OFFSETOF(Document, m_compatibilityMode); }

    WEBCORE_EXPORT String compatMode() const;

    bool inQuirksMode() const { return m_compatibilityMode == DocumentCompatibilityMode::QuirksMode; }
    bool inLimitedQuirksMode() const { return m_compatibilityMode == DocumentCompatibilityMode::LimitedQuirksMode; }
    bool inNoQuirksMode() const { return m_compatibilityMode == DocumentCompatibilityMode::NoQuirksMode; }

    enum ReadyState {
        Loading,
        Interactive,
        Complete
    };
    void setReadyState(ReadyState);
    void setParsing(bool);
    bool parsing() const { return m_bParsing; }
    std::chrono::milliseconds minimumLayoutDelay();

    bool shouldScheduleLayout();
    bool isLayoutTimerActive();
    std::chrono::milliseconds elapsedTime() const;
    
    void setTextColor(const Color& color) { m_textColor = color; }
    Color textColor() const { return m_textColor; }

    const Color& linkColor() const { return m_linkColor; }
    const Color& visitedLinkColor() const { return m_visitedLinkColor; }
    const Color& activeLinkColor() const { return m_activeLinkColor; }
    void setLinkColor(const Color& c) { m_linkColor = c; }
    void setVisitedLinkColor(const Color& c) { m_visitedLinkColor = c; }
    void setActiveLinkColor(const Color& c) { m_activeLinkColor = c; }
    void resetLinkColor();
    void resetVisitedLinkColor();
    void resetActiveLinkColor();
    VisitedLinkState& visitedLinkState() const { return *m_visitedLinkState; }

    MouseEventWithHitTestResults prepareMouseEvent(const HitTestRequest&, const LayoutPoint&, const PlatformMouseEvent&);

    WEBCORE_EXPORT String preferredStylesheetSet() const;
    WEBCORE_EXPORT String selectedStylesheetSet() const;
    WEBCORE_EXPORT void setSelectedStylesheetSet(const String&);

    enum class FocusRemovalEventsMode { Dispatch, DoNotDispatch };
    WEBCORE_EXPORT bool setFocusedElement(Element*, FocusDirection = FocusDirectionNone,
        FocusRemovalEventsMode = FocusRemovalEventsMode::Dispatch);
    Element* focusedElement() const { return m_focusedElement.get(); }
    UserActionElementSet& userActionElements()  { return m_userActionElements; }
    const UserActionElementSet& userActionElements() const { return m_userActionElements; }

    void setFocusNavigationStartingNode(Node*);
    Element* focusNavigationStartingNode(FocusDirection) const;

    void removeFocusedNodeOfSubtree(Node*, bool amongChildrenOnly = false);
    void hoveredElementDidDetach(Element*);
    void elementInActiveChainDidDetach(Element*);

    void updateHoverActiveState(const HitTestRequest&, Element*, StyleResolverUpdateFlag = RecalcStyleIfNeeded);

    // Updates for :target (CSS3 selector).
    void setCSSTarget(Element*);
    Element* cssTarget() const { return m_cssTarget; }
    static ptrdiff_t cssTargetMemoryOffset() { return OBJECT_OFFSETOF(Document, m_cssTarget); }

    WEBCORE_EXPORT void scheduleForcedStyleRecalc();
    void scheduleStyleRecalc();
    void unscheduleStyleRecalc();
    bool hasPendingStyleRecalc() const;
    bool hasPendingForcedStyleRecalc() const;
    void optimizedStyleSheetUpdateTimerFired();

    void registerNodeListForInvalidation(LiveNodeList&);
    void unregisterNodeListForInvalidation(LiveNodeList&);
    WEBCORE_EXPORT void registerCollection(HTMLCollection&);
    void unregisterCollection(HTMLCollection&);
    void collectionCachedIdNameMap(const HTMLCollection&);
    void collectionWillClearIdNameMap(const HTMLCollection&);
    bool shouldInvalidateNodeListAndCollectionCaches(const QualifiedName* attrName = nullptr) const;
    void invalidateNodeListAndCollectionCaches(const QualifiedName* attrName);

    void attachNodeIterator(NodeIterator*);
    void detachNodeIterator(NodeIterator*);
    void moveNodeIteratorsToNewDocument(Node*, Document*);

    void attachRange(Range*);
    void detachRange(Range*);

    void updateRangesAfterChildrenChanged(ContainerNode&);
    // nodeChildrenWillBeRemoved is used when removing all node children at once.
    void nodeChildrenWillBeRemoved(ContainerNode&);
    // nodeWillBeRemoved is only safe when removing one node at a time.
    void nodeWillBeRemoved(Node&);
    void removeFocusNavigationNodeOfSubtree(Node&, bool amongChildrenOnly = false);
    enum class AcceptChildOperation { Replace, InsertOrAdd };
    bool canAcceptChild(const Node& newChild, const Node* refChild, AcceptChildOperation) const;

    void textInserted(Node*, unsigned offset, unsigned length);
    void textRemoved(Node*, unsigned offset, unsigned length);
    void textNodesMerged(Text* oldNode, unsigned offset);
    void textNodeSplit(Text* oldNode);

    void createDOMWindow();
    void takeDOMWindowFrom(Document*);

    DOMWindow* domWindow() const { return m_domWindow.get(); }
    // In DOM Level 2, the Document's DOMWindow is called the defaultView.
    DOMWindow* defaultView() const { return domWindow(); } 

    // Helper functions for forwarding DOMWindow event related tasks to the DOMWindow if it exists.
    void setWindowAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value);
    void setWindowAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener>);
    EventListener* getWindowAttributeEventListener(const AtomicString& eventType);
    WEBCORE_EXPORT void dispatchWindowEvent(Event&, EventTarget* = nullptr);
    void dispatchWindowLoadEvent();

    WEBCORE_EXPORT RefPtr<Event> createEvent(const String& eventType, ExceptionCode&);

    // keep track of what types of event listeners are registered, so we don't
    // dispatch events unnecessarily
    enum ListenerType {
        DOMSUBTREEMODIFIED_LISTENER          = 1,
        DOMNODEINSERTED_LISTENER             = 1 << 1,
        DOMNODEREMOVED_LISTENER              = 1 << 2,
        DOMNODEREMOVEDFROMDOCUMENT_LISTENER  = 1 << 3,
        DOMNODEINSERTEDINTODOCUMENT_LISTENER = 1 << 4,
        DOMCHARACTERDATAMODIFIED_LISTENER    = 1 << 5,
        OVERFLOWCHANGED_LISTENER             = 1 << 6,
        ANIMATIONEND_LISTENER                = 1 << 7,
        ANIMATIONSTART_LISTENER              = 1 << 8,
        ANIMATIONITERATION_LISTENER          = 1 << 9,
        TRANSITIONEND_LISTENER               = 1 << 10,
        BEFORELOAD_LISTENER                  = 1 << 11,
        SCROLL_LISTENER                      = 1 << 12,
        FORCEWILLBEGIN_LISTENER              = 1 << 13,
        FORCECHANGED_LISTENER                = 1 << 14,
        FORCEDOWN_LISTENER                   = 1 << 15,
        FORCEUP_LISTENER                     = 1 << 16
    };

    bool hasListenerType(ListenerType listenerType) const { return (m_listenerTypes & listenerType); }
    bool hasListenerTypeForEventType(PlatformEvent::Type) const;
    void addListenerTypeIfNeeded(const AtomicString& eventType);

    bool hasMutationObserversOfType(MutationObserver::MutationType type) const
    {
        return m_mutationObserverTypes & type;
    }
    bool hasMutationObservers() const { return m_mutationObserverTypes; }
    void addMutationObserverTypes(MutationObserverOptions types) { m_mutationObserverTypes |= types; }

    WEBCORE_EXPORT CSSStyleDeclaration* getOverrideStyle(Element*, const String& pseudoElt);

    // Handles an HTTP header equivalent set by a meta tag using <meta http-equiv="..." content="...">. This is called
    // when a meta tag is encountered during document parsing, and also when a script dynamically changes or adds a meta
    // tag. This enables scripts to use meta tags to perform refreshes and set expiry dates in addition to them being
    // specified in an HTML file.
    void processHttpEquiv(const String& equiv, const String& content, bool isInDocumentHead);

#if PLATFORM(IOS)
    void processFormatDetection(const String&);

    // Called when <meta name="apple-mobile-web-app-orientations"> changes.
    void processWebAppOrientations();
#endif
    
    void processViewport(const String& features, ViewportArguments::Type origin);
    void updateViewportArguments();
    void processReferrerPolicy(const String& policy);

    // Returns the owning element in the parent document.
    // Returns 0 if this is the top level document.
    HTMLFrameOwnerElement* ownerElement() const;

    // Used by DOM bindings; no direction known.
    String title() const { return m_title.string(); }
    WEBCORE_EXPORT void setTitle(const String&);

    WEBCORE_EXPORT const AtomicString& dir() const;
    WEBCORE_EXPORT void setDir(const AtomicString&);

    void titleElementAdded(Element& titleElement);
    void titleElementRemoved(Element& titleElement);
    void titleElementTextChanged(Element& titleElement);

    WEBCORE_EXPORT String cookie(ExceptionCode&);
    WEBCORE_EXPORT void setCookie(const String&, ExceptionCode&);

    WEBCORE_EXPORT String referrer() const;

    WEBCORE_EXPORT String origin() const;

    WEBCORE_EXPORT String domain() const;
    void setDomain(const String& newDomain, ExceptionCode&);

    WEBCORE_EXPORT String lastModified() const;

    // The cookieURL is used to query the cookie database for this document's
    // cookies. For example, if the cookie URL is http://example.com, we'll
    // use the non-Secure cookies for example.com when computing
    // document.cookie.
    //
    // Q: How is the cookieURL different from the document's URL?
    // A: The two URLs are the same almost all the time.  However, if one
    //    document inherits the security context of another document, it
    //    inherits its cookieURL but not its URL.
    //
    const URL& cookieURL() const { return m_cookieURL; }
    void setCookieURL(const URL&);

    // The firstPartyForCookies is used to compute whether this document
    // appears in a "third-party" context for the purpose of third-party
    // cookie blocking.  The document is in a third-party context if the
    // cookieURL and the firstPartyForCookies are from different hosts.
    //
    // Note: Some ports (including possibly Apple's) only consider the
    //       document in a third-party context if the cookieURL and the
    //       firstPartyForCookies have a different registry-controlled
    //       domain.
    //
    const URL& firstPartyForCookies() const { return m_firstPartyForCookies; }
    void setFirstPartyForCookies(const URL& url) { m_firstPartyForCookies = url; }
    
    // The following implements the rule from HTML 4 for what valid names are.
    // To get this right for all the XML cases, we probably have to improve this or move it
    // and make it sensitive to the type of document.
    static bool isValidName(const String&);

    // The following breaks a qualified name into a prefix and a local name.
    // It also does a validity check, and returns false if the qualified name
    // is invalid.  It also sets ExceptionCode when name is invalid.
    static bool parseQualifiedName(const String& qualifiedName, String& prefix, String& localName, ExceptionCode&);

    // Checks to make sure prefix and namespace do not conflict (per DOM Core 3)
    static bool hasValidNamespaceForElements(const QualifiedName&);
    static bool hasValidNamespaceForAttributes(const QualifiedName&);

    HTMLBodyElement* body() const;
    WEBCORE_EXPORT HTMLElement* bodyOrFrameset() const;
    WEBCORE_EXPORT void setBodyOrFrameset(RefPtr<HTMLElement>&&, ExceptionCode&);

    Location* location() const;

    WEBCORE_EXPORT HTMLHeadElement* head();

    DocumentMarkerController& markers() const { return *m_markers; }

    WEBCORE_EXPORT bool execCommand(const String& command, bool userInterface = false, const String& value = String());
    WEBCORE_EXPORT bool queryCommandEnabled(const String& command);
    WEBCORE_EXPORT bool queryCommandIndeterm(const String& command);
    WEBCORE_EXPORT bool queryCommandState(const String& command);
    WEBCORE_EXPORT bool queryCommandSupported(const String& command);
    WEBCORE_EXPORT String queryCommandValue(const String& command);

    // designMode support
    enum InheritedBool { off = false, on = true, inherit };    
    void setDesignMode(InheritedBool value);
    InheritedBool getDesignMode() const;
    bool inDesignMode() const;
    WEBCORE_EXPORT String designMode() const;
    WEBCORE_EXPORT void setDesignMode(const String&);

    Document* parentDocument() const;
    Document& topDocument() const;
    
    ScriptRunner* scriptRunner() { return m_scriptRunner.get(); }
    ScriptModuleLoader* moduleLoader() { return m_moduleLoader.get(); }

    HTMLScriptElement* currentScript() const { return !m_currentScriptStack.isEmpty() ? m_currentScriptStack.last().get() : nullptr; }
    void pushCurrentScript(HTMLScriptElement*);
    void popCurrentScript();

#if ENABLE(XSLT)
    void applyXSLTransform(ProcessingInstruction* pi);
    RefPtr<Document> transformSourceDocument() { return m_transformSourceDocument; }
    void setTransformSourceDocument(Document* doc) { m_transformSourceDocument = doc; }

    void setTransformSource(std::unique_ptr<TransformSource>);
    TransformSource* transformSource() const { return m_transformSource.get(); }
#endif

    void incDOMTreeVersion() { m_domTreeVersion = ++s_globalTreeVersion; }
    uint64_t domTreeVersion() const { return m_domTreeVersion; }

    // XPathEvaluator methods
    WEBCORE_EXPORT RefPtr<XPathExpression> createExpression(const String& expression, RefPtr<XPathNSResolver>&&, ExceptionCode&);
    WEBCORE_EXPORT RefPtr<XPathNSResolver> createNSResolver(Node* nodeResolver);
    WEBCORE_EXPORT RefPtr<XPathResult> evaluate(const String& expression, Node* contextNode, RefPtr<XPathNSResolver>&&, unsigned short type, XPathResult*, ExceptionCode&);

    enum PendingSheetLayout { NoLayoutWithPendingSheets, DidLayoutWithPendingSheets, IgnoreLayoutWithPendingSheets };

    bool didLayoutWithPendingStylesheets() const { return m_pendingSheetLayout == DidLayoutWithPendingSheets; }

    bool hasNodesWithPlaceholderStyle() const { return m_hasNodesWithPlaceholderStyle; }
    void setHasNodesWithPlaceholderStyle() { m_hasNodesWithPlaceholderStyle = true; }

    void updateFocusAppearanceSoon(SelectionRestorationMode);
    void cancelFocusAppearanceUpdate();

    // Extension for manipulating canvas drawing contexts for use in CSS
    CanvasRenderingContext* getCSSCanvasContext(const String& type, const String& name, int width, int height);
    HTMLCanvasElement* getCSSCanvasElement(const String& name);

    bool isDNSPrefetchEnabled() const { return m_isDNSPrefetchEnabled; }
    void parseDNSPrefetchControlHeader(const String&);

    void postTask(Task&&) final; // Executes the task on context's thread asynchronously.

#if ENABLE(REQUEST_ANIMATION_FRAME)
    ScriptedAnimationController* scriptedAnimationController() { return m_scriptedAnimationController.get(); }
#endif
    void suspendScriptedAnimationControllerCallbacks();
    void resumeScriptedAnimationControllerCallbacks();
    void scriptedAnimationControllerSetThrottled(bool);
    
    void windowScreenDidChange(PlatformDisplayID);

    void finishedParsing();

    enum PageCacheState { NotInPageCache, AboutToEnterPageCache, InPageCache };

    PageCacheState pageCacheState() const { return m_pageCacheState; }
    void setPageCacheState(PageCacheState);

    // FIXME: Update callers to use pageCacheState() instead.
    bool inPageCache() const { return m_pageCacheState != NotInPageCache; }

    // Elements can register themselves for the "suspend()" and
    // "resume()" callbacks
    void registerForDocumentSuspensionCallbacks(Element*);
    void unregisterForDocumentSuspensionCallbacks(Element*);

    void documentWillBecomeInactive();
    void suspend(ActiveDOMObject::ReasonForSuspension);
    void resume(ActiveDOMObject::ReasonForSuspension);

    void registerForMediaVolumeCallbacks(Element*);
    void unregisterForMediaVolumeCallbacks(Element*);
    void mediaVolumeDidChange();

#if ENABLE(MEDIA_SESSION)
    MediaSession& defaultMediaSession();
#endif

    void registerForPrivateBrowsingStateChangedCallbacks(Element*);
    void unregisterForPrivateBrowsingStateChangedCallbacks(Element*);
    void storageBlockingStateDidChange();
    void privateBrowsingStateDidChange();

#if ENABLE(VIDEO_TRACK)
    void registerForCaptionPreferencesChangedCallbacks(Element*);
    void unregisterForCaptionPreferencesChangedCallbacks(Element*);
    void captionPreferencesChanged();
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    void registerForPageScaleFactorChangedCallbacks(HTMLMediaElement*);
    void unregisterForPageScaleFactorChangedCallbacks(HTMLMediaElement*);
    void pageScaleFactorChangedAndStable();
    void registerForUserInterfaceLayoutDirectionChangedCallbacks(HTMLMediaElement&);
    void unregisterForUserInterfaceLayoutDirectionChangedCallbacks(HTMLMediaElement&);
    void userInterfaceLayoutDirectionChanged();
#endif

    void registerForVisibilityStateChangedCallbacks(Element*);
    void unregisterForVisibilityStateChangedCallbacks(Element*);

#if ENABLE(VIDEO)
    void registerForAllowsMediaDocumentInlinePlaybackChangedCallbacks(HTMLMediaElement&);
    void unregisterForAllowsMediaDocumentInlinePlaybackChangedCallbacks(HTMLMediaElement&);
    void allowsMediaDocumentInlinePlaybackChanged();
#endif

    WEBCORE_EXPORT void setShouldCreateRenderers(bool);
    bool shouldCreateRenderers();

    void setDecoder(RefPtr<TextResourceDecoder>&&);
    TextResourceDecoder* decoder() const { return m_decoder.get(); }

    WEBCORE_EXPORT String displayStringModifiedByEncoding(const String&) const;
    RefPtr<StringImpl> displayStringModifiedByEncoding(PassRefPtr<StringImpl>) const;
    void displayBufferModifiedByEncoding(LChar* buffer, unsigned len) const
    {
        displayBufferModifiedByEncodingInternal(buffer, len);
    }
    void displayBufferModifiedByEncoding(UChar* buffer, unsigned len) const
    {
        displayBufferModifiedByEncodingInternal(buffer, len);
    }

    // Quirk for the benefit of Apple's Dictionary application.
    void setFrameElementsShouldIgnoreScrolling(bool ignore) { m_frameElementsShouldIgnoreScrolling = ignore; }
    bool frameElementsShouldIgnoreScrolling() const { return m_frameElementsShouldIgnoreScrolling; }

#if ENABLE(DASHBOARD_SUPPORT)
    void setAnnotatedRegionsDirty(bool f) { m_annotatedRegionsDirty = f; }
    bool annotatedRegionsDirty() const { return m_annotatedRegionsDirty; }
    bool hasAnnotatedRegions () const { return m_hasAnnotatedRegions; }
    void setHasAnnotatedRegions(bool f) { m_hasAnnotatedRegions = f; }
    WEBCORE_EXPORT const Vector<AnnotatedRegionValue>& annotatedRegions() const;
    void setAnnotatedRegions(const Vector<AnnotatedRegionValue>&);
#endif

    void removeAllEventListeners() final;

    WEBCORE_EXPORT const SVGDocumentExtensions* svgExtensions();
    WEBCORE_EXPORT SVGDocumentExtensions& accessSVGExtensions();

    void initSecurityContext();
    void initContentSecurityPolicy();

    void updateURLForPushOrReplaceState(const URL&);
    void statePopped(PassRefPtr<SerializedScriptValue>);

    bool processingLoadEvent() const { return m_processingLoadEvent; }
    bool loadEventFinished() const { return m_loadEventFinished; }

    bool isContextThread() const final;
    bool isJSExecutionForbidden() const final { return false; }

    void enqueueWindowEvent(Ref<Event>&&);
    void enqueueDocumentEvent(Ref<Event>&&);
    void enqueueOverflowEvent(Ref<Event>&&);
    void enqueuePageshowEvent(PageshowEventPersistence);
    void enqueueHashchangeEvent(const String& oldURL, const String& newURL);
    void enqueuePopstateEvent(RefPtr<SerializedScriptValue>&& stateObject);
    DocumentEventQueue& eventQueue() const final { return m_eventQueue; }

    WEBCORE_EXPORT void addMediaCanStartListener(MediaCanStartListener*);
    WEBCORE_EXPORT void removeMediaCanStartListener(MediaCanStartListener*);
    MediaCanStartListener* takeAnyMediaCanStartListener();

#if ENABLE(FULLSCREEN_API)
    bool webkitIsFullScreen() const { return m_fullScreenElement.get(); }
    bool webkitFullScreenKeyboardInputAllowed() const { return m_fullScreenElement.get() && m_areKeysEnabledInFullScreen; }
    Element* webkitCurrentFullScreenElement() const { return m_fullScreenElement.get(); }
    
    enum FullScreenCheckType {
        EnforceIFrameAllowFullScreenRequirement,
        ExemptIFrameAllowFullScreenRequirement,
    };

    void requestFullScreenForElement(Element*, unsigned short flags, FullScreenCheckType);
    WEBCORE_EXPORT void webkitCancelFullScreen();
    
    WEBCORE_EXPORT void webkitWillEnterFullScreenForElement(Element*);
    WEBCORE_EXPORT void webkitDidEnterFullScreenForElement(Element*);
    WEBCORE_EXPORT void webkitWillExitFullScreenForElement(Element*);
    WEBCORE_EXPORT void webkitDidExitFullScreenForElement(Element*);
    
    void setFullScreenRenderer(RenderFullScreen*);
    RenderFullScreen* fullScreenRenderer() const { return m_fullScreenRenderer; }
    void fullScreenRendererDestroyed();
    
    void fullScreenChangeDelayTimerFired();
    bool fullScreenIsAllowedForElement(Element*) const;
    void fullScreenElementRemoved();
    void removeFullScreenElementOfSubtree(Node*, bool amongChildrenOnly = false);
    bool isAnimatingFullScreen() const;
    WEBCORE_EXPORT void setAnimatingFullScreen(bool);

    WEBCORE_EXPORT bool webkitFullscreenEnabled() const;
    Element* webkitFullscreenElement() const { return !m_fullScreenElementStack.isEmpty() ? m_fullScreenElementStack.last().get() : nullptr; }
    WEBCORE_EXPORT void webkitExitFullscreen();
#endif

#if ENABLE(POINTER_LOCK)
    void exitPointerLock();
    Element* pointerLockElement() const;
#endif

    // Used to allow element that loads data without going through a FrameLoader to delay the 'load' event.
    void incrementLoadEventDelayCount() { ++m_loadEventDelayCount; }
    void decrementLoadEventDelayCount();
    bool isDelayingLoadEvent() const { return m_loadEventDelayCount; }

#if ENABLE(IOS_TOUCH_EVENTS)
#include <WebKitAdditions/DocumentIOS.h>
#elif ENABLE(TOUCH_EVENTS)
    RefPtr<Touch> createTouch(DOMWindow*, EventTarget*, int identifier, int pageX, int pageY, int screenX, int screenY, int radiusX, int radiusY, float rotationAngle, float force, ExceptionCode&) const;
#endif

#if ENABLE(DEVICE_ORIENTATION) && PLATFORM(IOS)
    DeviceMotionController* deviceMotionController() const;
    DeviceOrientationController* deviceOrientationController() const;
#endif

#if ENABLE(WEB_TIMING)
    const DocumentTiming& timing() const { return m_documentTiming; }
#endif

    double monotonicTimestamp() const;

#if ENABLE(REQUEST_ANIMATION_FRAME)
    int requestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback>);
    void cancelAnimationFrame(int id);
    void serviceScriptedAnimations(double timestamp);
#endif

    void sendWillRevealEdgeEventsIfNeeded(const IntPoint& oldPosition, const IntPoint& newPosition, const IntRect& visibleRect, const IntSize& contentsSize, Element* target = nullptr);

    EventTarget* errorEventTarget() final;
    void logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, int columnNumber, RefPtr<Inspector::ScriptCallStack>&&) final;

    void initDNSPrefetch();

    void didAddWheelEventHandler(Node&);
    void didRemoveWheelEventHandler(Node&, EventHandlerRemoval = EventHandlerRemoval::One);

    double lastHandledUserGestureTimestamp() const { return m_lastHandledUserGestureTimestamp; }
    void updateLastHandledUserGestureTimestamp();

#if ENABLE(TOUCH_EVENTS)
    bool hasTouchEventHandlers() const { return (m_touchEventTargets.get()) ? m_touchEventTargets->size() : false; }
#else
    bool hasTouchEventHandlers() const { return false; }
#endif

    // Used for testing. Count handlers in the main document, and one per frame which contains handlers.
    WEBCORE_EXPORT unsigned wheelEventHandlerCount() const;
    WEBCORE_EXPORT unsigned touchEventHandlerCount() const;

    WEBCORE_EXPORT void startTrackingStyleRecalcs();
    WEBCORE_EXPORT unsigned styleRecalcCount() const;

    void didAddTouchEventHandler(Node&);
    void didRemoveTouchEventHandler(Node&, EventHandlerRemoval = EventHandlerRemoval::One);

    void didRemoveEventTargetNode(Node&);

    const EventTargetSet* touchEventTargets() const
    {
#if ENABLE(TOUCH_EVENTS)
        return m_touchEventTargets.get();
#else
        return nullptr;
#endif
    }

    const EventTargetSet* wheelEventTargets() const { return m_wheelEventTargets.get(); }

    typedef std::pair<Region, bool> RegionFixedPair;
    RegionFixedPair absoluteRegionForEventTargets(const EventTargetSet*);

    LayoutRect absoluteEventHandlerBounds(bool&) final;

    bool visualUpdatesAllowed() const { return m_visualUpdatesAllowed; }

    bool isInDocumentWrite() { return m_writeRecursionDepth > 0; }

    void suspendScheduledTasks(ActiveDOMObject::ReasonForSuspension);
    void resumeScheduledTasks(ActiveDOMObject::ReasonForSuspension);

#if ENABLE(CSS_DEVICE_ADAPTATION)
    IntSize initialViewportSize() const;
#endif

#if ENABLE(TEXT_AUTOSIZING)
    TextAutosizer* textAutosizer() { return m_textAutosizer.get(); }
#endif

    void adjustFloatQuadsForScrollAndAbsoluteZoomAndFrameScale(Vector<FloatQuad>&, const RenderStyle&);
    void adjustFloatRectForScrollAndAbsoluteZoomAndFrameScale(FloatRect&, const RenderStyle&);

    bool hasActiveParser();
    void incrementActiveParserCount() { ++m_activeParserCount; }
    void decrementActiveParserCount();

    DocumentSharedObjectPool* sharedObjectPool() { return m_sharedObjectPool.get(); }

    void didRemoveAllPendingStylesheet();
    void setNeedsNotifyRemoveAllPendingStylesheet() { m_needsNotifyRemoveAllPendingStylesheet = true; }
    void clearStyleResolver();

    bool inStyleRecalc() const { return m_inStyleRecalc; }
    bool inRenderTreeUpdate() const { return m_inRenderTreeUpdate; }

    // Return a Locale for the default locale if the argument is null or empty.
    Locale& getCachedLocale(const AtomicString& locale = nullAtom);

    const Document* templateDocument() const;
    Document& ensureTemplateDocument();
    void setTemplateDocumentHost(Document* templateDocumentHost) { m_templateDocumentHost = templateDocumentHost; }
    Document* templateDocumentHost() { return m_templateDocumentHost; }

    void didAssociateFormControl(Element*);
    bool hasDisabledFieldsetElement() const { return m_disabledFieldsetElementsCount; }
    void addDisabledFieldsetElement() { m_disabledFieldsetElementsCount++; }
    void removeDisabledFieldsetElement() { ASSERT(m_disabledFieldsetElementsCount); m_disabledFieldsetElementsCount--; }

    WEBCORE_EXPORT void addConsoleMessage(MessageSource, MessageLevel, const String& message, unsigned long requestIdentifier = 0) final;

    WEBCORE_EXPORT SecurityOrigin* topOrigin() const final;

    Ref<FontFaceSet> fonts();

    void ensurePlugInsInjectedScript(DOMWrapperWorld&);

    void setVisualUpdatesAllowedByClient(bool);

#if ENABLE(SUBTLE_CRYPTO)
    bool wrapCryptoKey(const Vector<uint8_t>& key, Vector<uint8_t>& wrappedKey) final;
    bool unwrapCryptoKey(const Vector<uint8_t>& wrappedKey, Vector<uint8_t>& key) final;
#endif

    void setHasStyleWithViewportUnits() { m_hasStyleWithViewportUnits = true; }
    bool hasStyleWithViewportUnits() const { return m_hasStyleWithViewportUnits; }
    void updateViewportUnitsOnResize();

    WEBCORE_EXPORT void addAudioProducer(MediaProducer*);
    WEBCORE_EXPORT void removeAudioProducer(MediaProducer*);
    MediaProducer::MediaStateFlags mediaState() const { return m_mediaState; }
    WEBCORE_EXPORT void updateIsPlayingMedia(uint64_t = HTMLMediaElementInvalidID);
    void pageMutedStateDidChange();
    WeakPtr<Document> createWeakPtr() { return m_weakFactory.createWeakPtr(); }

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void addPlaybackTargetPickerClient(MediaPlaybackTargetClient&);
    void removePlaybackTargetPickerClient(MediaPlaybackTargetClient&);
    void showPlaybackTargetPicker(MediaPlaybackTargetClient&, bool);
    void playbackTargetPickerClientStateDidChange(MediaPlaybackTargetClient&, MediaProducer::MediaStateFlags);

    void setPlaybackTarget(uint64_t, Ref<MediaPlaybackTarget>&&);
    void playbackTargetAvailabilityDidChange(uint64_t, bool);
    void setShouldPlayToPlaybackTarget(uint64_t, bool);
#endif

    ShouldOpenExternalURLsPolicy shouldOpenExternalURLsPolicyToPropagate() const;
    bool shouldEnforceContentDispositionAttachmentSandbox() const;
    void applyContentDispositionAttachmentSandbox();

    void addViewportDependentPicture(HTMLPictureElement&);
    void removeViewportDependentPicture(HTMLPictureElement&);

#if ENABLE(MEDIA_STREAM)
    void setHasActiveMediaStreamTrack() { m_hasHadActiveMediaStreamTrack = true; }
    bool hasHadActiveMediaStreamTrack() const { return m_hasHadActiveMediaStreamTrack; }
#endif

    using ContainerNode::setAttributeEventListener;
    void setAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value);

protected:
    enum ConstructionFlags { Synthesized = 1, NonRenderedPlaceholder = 1 << 1 };
    Document(Frame*, const URL&, unsigned = DefaultDocumentClass, unsigned constructionFlags = 0);

    void clearXMLVersion() { m_xmlVersion = String(); }

    virtual Ref<Document> cloneDocumentWithoutChildren() const;

private:
    friend class Node;
    friend class IgnoreDestructiveWriteCountIncrementer;
    friend class IgnoreOpensDuringUnloadCountIncrementer;

    void updateTitleElement(Element* newTitleElement);

    void commonTeardown();

    RenderObject* renderer() const = delete;
    void setRenderer(RenderObject*) = delete;

    void createRenderTree();
    void detachParser();

    // FontSelectorClient
    void fontsNeedUpdate(FontSelector&) final;

    bool isDocument() const final { return true; }

    void childrenChanged(const ChildChange&) final;

    String nodeName() const final;
    NodeType nodeType() const final;
    bool childTypeAllowed(NodeType) const final;
    Ref<Node> cloneNodeInternal(Document&, CloningOperation) final;
    void cloneDataFromDocument(const Document&);

    void refScriptExecutionContext() final { ref(); }
    void derefScriptExecutionContext() final { deref(); }

    void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, RefPtr<Inspector::ScriptCallStack>&&, JSC::ExecState* = nullptr, unsigned long requestIdentifier = 0) final;

    std::chrono::milliseconds minimumTimerInterval() const final;

    std::chrono::milliseconds timerAlignmentInterval(bool hasReachedMaxNestingLevel) const final;

    void updateTitleFromTitleElement();
    void updateTitle(const StringWithDirection&);
    void updateFocusAppearanceTimerFired();
    void updateBaseURL();

    void buildAccessKeyMap(TreeScope* root);

    void createStyleResolver();

    void loadEventDelayTimerFired();

    void pendingTasksTimerFired();

    template <typename CharacterType>
    void displayBufferModifiedByEncodingInternal(CharacterType*, unsigned) const;

    PageVisibilityState pageVisibilityState() const;

    Node* nodeFromPoint(const LayoutPoint& clientPoint, LayoutPoint* localPoint = nullptr);

    template <CollectionType collectionType>
    Ref<HTMLCollection> ensureCachedCollection();

#if ENABLE(FULLSCREEN_API)
    void dispatchFullScreenChangeOrErrorEvent(Deque<RefPtr<Node>>&, const AtomicString& eventName, bool shouldNotifyMediaElement);
    void clearFullscreenElementStack();
    void popFullscreenElementStack();
    void pushFullscreenElementStack(Element*);
    void addDocumentToFullScreenChangeEventQueue(Document*);
#endif

    void setVisualUpdatesAllowed(ReadyState);
    void setVisualUpdatesAllowed(bool);
    void visualUpdatesSuppressionTimerFired();

    void addListenerType(ListenerType listenerType) { m_listenerTypes |= listenerType; }

    void didAssociateFormControlsTimerFired();

    void wheelEventHandlersChanged();

    HttpEquivPolicy httpEquivPolicy() const;

    // DOM Cookies caching.
    const String& cachedDOMCookies() const { return m_cachedDOMCookies; }
    void setCachedDOMCookies(const String&);
    bool isDOMCookieCacheValid() const { return m_cookieCacheExpiryTimer.isActive(); }
    void invalidateDOMCookieCache();
    void didLoadResourceSynchronously() final;

    void checkViewportDependentPictures();

    unsigned m_referencingNodeCount;

    std::unique_ptr<StyleResolver> m_styleResolver;
    std::unique_ptr<StyleResolver> m_userAgentShadowTreeStyleResolver;
    bool m_didCalculateStyleResolver;
    bool m_hasNodesWithPlaceholderStyle;
    bool m_needsNotifyRemoveAllPendingStylesheet;
    // But sometimes you need to ignore pending stylesheet count to
    // force an immediate layout when requested by JS.
    bool m_ignorePendingStylesheets;

    // If we do ignore the pending stylesheet count, then we need to add a boolean
    // to track that this happened so that we can do a full repaint when the stylesheets
    // do eventually load.
    PendingSheetLayout m_pendingSheetLayout;

    Frame* m_frame;
    RefPtr<DOMWindow> m_domWindow;

    Ref<CachedResourceLoader> m_cachedResourceLoader;
    RefPtr<DocumentParser> m_parser;
    unsigned m_activeParserCount;

    bool m_wellFormed;

    // Document URLs.
    URL m_url; // Document.URL: The URL from which this document was retrieved.
    URL m_baseURL; // Node.baseURI: The URL to use when resolving relative URLs.
    URL m_baseURLOverride; // An alternative base URL that takes precedence over m_baseURL (but not m_baseElementURL).
    URL m_baseElementURL; // The URL set by the <base> element.
    URL m_cookieURL; // The URL to use for cookie access.
    URL m_firstPartyForCookies; // The policy URL for third-party cookie blocking.

    // Document.documentURI:
    // Although URL-like, Document.documentURI can actually be set to any
    // string by content.  Document.documentURI affects m_baseURL unless the
    // document contains a <base> element, in which case the <base> element
    // takes precedence.
    //
    // This property is read-only from JavaScript, but writable from Objective C.
    String m_documentURI;

    String m_baseTarget;

    // MIME type of the document in case it was cloned or created by XHR.
    String m_overriddenMIMEType;

    std::unique_ptr<DOMImplementation> m_implementation;

    RefPtr<CSSStyleSheet> m_elementSheet;

    bool m_printing;
    bool m_paginatedForScreen;

    DocumentCompatibilityMode m_compatibilityMode;
    bool m_compatibilityModeLocked; // This is cheaper than making setCompatibilityMode virtual.

    Color m_textColor;

    bool m_focusNavigationStartingNodeIsRemoved;
    RefPtr<Node> m_focusNavigationStartingNode;
    RefPtr<Element> m_focusedElement;
    RefPtr<Element> m_hoveredElement;
    RefPtr<Element> m_activeElement;
    RefPtr<Element> m_documentElement;
    UserActionElementSet m_userActionElements;

    uint64_t m_domTreeVersion;
    static uint64_t s_globalTreeVersion;
    
    HashSet<NodeIterator*> m_nodeIterators;
    HashSet<Range*> m_ranges;

    unsigned m_listenerTypes;

    MutationObserverOptions m_mutationObserverTypes;

    std::unique_ptr<AuthorStyleSheets> m_authorStyleSheets;
    std::unique_ptr<ExtensionStyleSheets> m_extensionStyleSheets;
    RefPtr<StyleSheetList> m_styleSheetList;

    std::unique_ptr<FormController> m_formController;

    Color m_linkColor;
    Color m_visitedLinkColor;
    Color m_activeLinkColor;
    const std::unique_ptr<VisitedLinkState> m_visitedLinkState;

    bool m_visuallyOrdered;
    ReadyState m_readyState;
    bool m_bParsing;

    Timer m_optimizedStyleSheetUpdateTimer;
    Timer m_styleRecalcTimer;
    bool m_hasEverCalledWindowOpen { false };
    bool m_pendingStyleRecalcShouldForce;
    bool m_inStyleRecalc;
    bool m_closeAfterStyleRecalc;
    bool m_inRenderTreeUpdate { false };
    unsigned m_lastStyleUpdateSizeForTesting { 0 };

    bool m_gotoAnchorNeededAfterStylesheetsLoad;
    bool m_isDNSPrefetchEnabled;
    bool m_haveExplicitlyDisabledDNSPrefetch;
    bool m_frameElementsShouldIgnoreScrolling;
    SelectionRestorationMode m_updateFocusAppearanceRestoresSelection;

    // https://html.spec.whatwg.org/multipage/webappapis.html#ignore-destructive-writes-counter
    unsigned m_ignoreDestructiveWriteCount { 0 };

    // https://html.spec.whatwg.org/multipage/webappapis.html#ignore-opens-during-unload-counter
    unsigned m_ignoreOpensDuringUnloadCount { 0 };

    unsigned m_styleRecalcCount { 0 };

    StringWithDirection m_title;
    StringWithDirection m_rawTitle;
    RefPtr<Element> m_titleElement;

    std::unique_ptr<AXObjectCache> m_axObjectCache;
    const std::unique_ptr<DocumentMarkerController> m_markers;
    
    Timer m_updateFocusAppearanceTimer;

    Element* m_cssTarget;

    // FIXME: Merge these 2 variables into an enum. Also, FrameLoader::m_didCallImplicitClose
    // is almost a duplication of this data, so that should probably get merged in too.
    // FIXME: Document::m_processingLoadEvent and DocumentLoader::m_wasOnloadDispatched are roughly the same
    // and should be merged.
    bool m_processingLoadEvent;
    bool m_loadEventFinished;

    RefPtr<SerializedScriptValue> m_pendingStateObject;
    std::chrono::steady_clock::time_point m_startTime;
    bool m_overMinimumLayoutThreshold;
    
    std::unique_ptr<ScriptRunner> m_scriptRunner;
    std::unique_ptr<ScriptModuleLoader> m_moduleLoader;

    Vector<RefPtr<HTMLScriptElement>> m_currentScriptStack;

#if ENABLE(XSLT)
    std::unique_ptr<TransformSource> m_transformSource;
    RefPtr<Document> m_transformSourceDocument;
#endif

    String m_xmlEncoding;
    String m_xmlVersion;
    unsigned m_xmlStandalone : 2;
    unsigned m_hasXMLDeclaration : 1;

    String m_contentLanguage;

    RefPtr<TextResourceDecoder> m_decoder;

    InheritedBool m_designMode;

    HashSet<LiveNodeList*> m_listsInvalidatedAtDocument;
    HashSet<HTMLCollection*> m_collectionsInvalidatedAtDocument;
    unsigned m_nodeListAndCollectionCounts[numNodeListInvalidationTypes];

    RefPtr<XPathEvaluator> m_xpathEvaluator;

    std::unique_ptr<SVGDocumentExtensions> m_svgExtensions;

#if ENABLE(DASHBOARD_SUPPORT)
    Vector<AnnotatedRegionValue> m_annotatedRegions;
    bool m_hasAnnotatedRegions;
    bool m_annotatedRegionsDirty;
#endif

    HashMap<String, RefPtr<HTMLCanvasElement>> m_cssCanvasElements;

    bool m_createRenderers;
    PageCacheState m_pageCacheState { NotInPageCache };

    HashSet<Element*> m_documentSuspensionCallbackElements;
    HashSet<Element*> m_mediaVolumeCallbackElements;
    HashSet<Element*> m_privateBrowsingStateChangedElements;
#if ENABLE(VIDEO_TRACK)
    HashSet<Element*> m_captionPreferencesChangedElements;
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    HashSet<HTMLMediaElement*> m_pageScaleFactorChangedElements;
    HashSet<HTMLMediaElement*> m_userInterfaceLayoutDirectionChangedElements;
#endif

    HashSet<Element*> m_visibilityStateCallbackElements;
#if ENABLE(VIDEO)
    HashSet<HTMLMediaElement*> m_allowsMediaDocumentInlinePlaybackElements;
#endif

    HashMap<StringImpl*, Element*, ASCIICaseInsensitiveHash> m_elementsByAccessKey;
    bool m_accessKeyMapValid;

    DocumentOrderedMap m_imagesByUsemap;

    std::unique_ptr<SelectorQueryCache> m_selectorQueryCache;

    DocumentClassFlags m_documentClasses;

    bool m_isSynthesized;
    bool m_isNonRenderedPlaceholder;

    bool m_sawElementsInKnownNamespaces;
    bool m_isSrcdocDocument;

    RenderPtr<RenderView> m_renderView;
    mutable DocumentEventQueue m_eventQueue;

    WeakPtrFactory<Document> m_weakFactory;

    HashSet<MediaCanStartListener*> m_mediaCanStartListeners;

#if ENABLE(FULLSCREEN_API)
    bool m_areKeysEnabledInFullScreen;
    RefPtr<Element> m_fullScreenElement;
    Vector<RefPtr<Element>> m_fullScreenElementStack;
    RenderFullScreen* m_fullScreenRenderer;
    Timer m_fullScreenChangeDelayTimer;
    Deque<RefPtr<Node>> m_fullScreenChangeEventTargetQueue;
    Deque<RefPtr<Node>> m_fullScreenErrorEventTargetQueue;
    bool m_isAnimatingFullScreen;
    LayoutRect m_savedPlaceholderFrameRect;
    std::unique_ptr<RenderStyle> m_savedPlaceholderRenderStyle;
#endif

    HashSet<HTMLPictureElement*> m_viewportDependentPictures;

    int m_loadEventDelayCount;
    Timer m_loadEventDelayTimer;

    ViewportArguments m_viewportArguments;

    ReferrerPolicy m_referrerPolicy;

#if ENABLE(WEB_TIMING)
    DocumentTiming m_documentTiming;
#endif

    RefPtr<MediaQueryMatcher> m_mediaQueryMatcher;
    bool m_writeRecursionIsTooDeep;
    unsigned m_writeRecursionDepth;
    
#if ENABLE(TOUCH_EVENTS)
    std::unique_ptr<EventTargetSet> m_touchEventTargets;
#endif
    std::unique_ptr<EventTargetSet> m_wheelEventTargets;

    double m_lastHandledUserGestureTimestamp;

#if ENABLE(REQUEST_ANIMATION_FRAME)
    void clearScriptedAnimationController();
    RefPtr<ScriptedAnimationController> m_scriptedAnimationController;
#endif

#if ENABLE(DEVICE_ORIENTATION) && PLATFORM(IOS)
    std::unique_ptr<DeviceMotionClient> m_deviceMotionClient;
    std::unique_ptr<DeviceMotionController> m_deviceMotionController;
    std::unique_ptr<DeviceOrientationClient> m_deviceOrientationClient;
    std::unique_ptr<DeviceOrientationController> m_deviceOrientationController;
#endif

// FIXME: Find a better place for this functionality.
#if ENABLE(TELEPHONE_NUMBER_DETECTION)
public:

    // These functions provide a two-level setting:
    //    - A user-settable wantsTelephoneNumberParsing (at the Page / WebView level)
    //    - A read-only telephoneNumberParsingAllowed which is set by the
    //      document if it has the appropriate meta tag.
    //    - isTelephoneNumberParsingEnabled() == isTelephoneNumberParsingAllowed() && page()->settings()->isTelephoneNumberParsingEnabled()

    WEBCORE_EXPORT bool isTelephoneNumberParsingAllowed() const;
    WEBCORE_EXPORT bool isTelephoneNumberParsingEnabled() const;

private:
    friend void setParserFeature(const String& key, const String& value, Document*, void* userData);
    void setIsTelephoneNumberParsingAllowed(bool);

    bool m_isTelephoneNumberParsingAllowed;
#endif

    Timer m_pendingTasksTimer;
    Vector<Task> m_pendingTasks;

#if ENABLE(IOS_TEXT_AUTOSIZING)
public:
    void addAutoSizedNode(Text&, float size);
    void updateAutoSizedNodes();
    void clearAutoSizedNodes();

private:
    using TextAutoSizingMap = HashMap<TextAutoSizingKey, std::unique_ptr<TextAutoSizingValue>, TextAutoSizingHash, TextAutoSizingTraits>;
    TextAutoSizingMap m_textAutoSizedNodes;
#endif

#if ENABLE(TEXT_AUTOSIZING)
    std::unique_ptr<TextAutosizer> m_textAutosizer;
#endif

    void platformSuspendOrStopActiveDOMObjects();

    bool m_scheduledTasksAreSuspended;
    
    bool m_visualUpdatesAllowed;
    Timer m_visualUpdatesSuppressionTimer;

    RefPtr<NamedFlowCollection> m_namedFlows;

    void clearSharedObjectPool();
    Timer m_sharedObjectPoolClearTimer;

    std::unique_ptr<DocumentSharedObjectPool> m_sharedObjectPool;

#ifndef NDEBUG
    bool m_didDispatchViewportPropertiesChanged;
#endif

    typedef HashMap<AtomicString, std::unique_ptr<Locale>> LocaleIdentifierToLocaleMap;
    LocaleIdentifierToLocaleMap m_localeCache;

    RefPtr<Document> m_templateDocument;
    Document* m_templateDocumentHost; // Manually managed weakref (backpointer from m_templateDocument).

    Ref<CSSFontSelector> m_fontSelector;

#if ENABLE(WEB_REPLAY)
    RefPtr<JSC::InputCursor> m_inputCursor;
#endif

    Timer m_didAssociateFormControlsTimer;
    Timer m_cookieCacheExpiryTimer;
    String m_cachedDOMCookies;
    HashSet<RefPtr<Element>> m_associatedFormControls;
    unsigned m_disabledFieldsetElementsCount;

    bool m_hasInjectedPlugInsScript;
    bool m_renderTreeBeingDestroyed { false };
    bool m_hasPreparedForDestruction { false };

    bool m_hasStyleWithViewportUnits;
    bool m_isTimerThrottlingEnabled { false };
    bool m_isSuspended { false };

    HashSet<MediaProducer*> m_audioProducers;
    MediaProducer::MediaStateFlags m_mediaState { MediaProducer::IsNotPlaying };

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    typedef HashMap<uint64_t, WebCore::MediaPlaybackTargetClient*> TargetIdToClientMap;
    TargetIdToClientMap m_idToClientMap;
    typedef HashMap<WebCore::MediaPlaybackTargetClient*, uint64_t> TargetClientToIdMap;
    TargetClientToIdMap m_clientToIDMap;
#endif

#if ENABLE(MEDIA_SESSION)
    RefPtr<MediaSession> m_defaultMediaSession;
#endif
    bool m_areDeviceMotionAndOrientationUpdatesSuspended { false };

#if ENABLE(MEDIA_STREAM)
    bool m_hasHadActiveMediaStreamTrack { false };
#endif

#if ENABLE(INDEXED_DATABASE)
    RefPtr<IDBClient::IDBConnectionProxy> m_idbConnectionProxy;
#endif
#if ENABLE(WEB_SOCKETS)
    RefPtr<SocketProvider> m_socketProvider;
#endif
};

inline void Document::notifyRemovePendingSheetIfNeeded()
{
    if (m_needsNotifyRemoveAllPendingStylesheet)
        didRemoveAllPendingStylesheet();
}

inline TextEncoding Document::textEncoding() const
{
    if (auto* decoder = this->decoder())
        return decoder->encoding();
    return TextEncoding();
}

inline const Document* Document::templateDocument() const
{
    return m_templateDocumentHost ? this : m_templateDocument.get();
}

// Put these methods here, because they require the Document definition, but we really want to inline them.

inline bool Node::isDocumentNode() const
{
    return this == &document();
}

inline ScriptExecutionContext* Node::scriptExecutionContext() const
{
    return &document();
}

Element* eventTargetElementForDocument(Document*);

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::Document)
    static bool isType(const WebCore::ScriptExecutionContext& context) { return context.isDocument(); }
    static bool isType(const WebCore::Node& node) { return node.isDocumentNode(); }
SPECIALIZE_TYPE_TRAITS_END()
