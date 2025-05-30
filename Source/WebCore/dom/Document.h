/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004-2018 Apple Inc. All rights reserved.
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

#include "CSSRegisteredCustomProperty.h"
#include "Color.h"
#include "ContainerNode.h"
#include "DisabledAdaptations.h"
#include "DocumentEventQueue.h"
#include "DocumentIdentifier.h"
#include "DocumentTiming.h"
#include "FocusDirection.h"
#include "FontSelectorClient.h"
#include "FrameDestructionObserver.h"
#include "GenericTaskQueue.h"
#include "MediaProducer.h"
#include "MutationObserver.h"
#include "OrientationNotifier.h"
#include "PlatformEvent.h"
#include "ReferrerPolicy.h"
#include "Region.h"
#include "RenderPtr.h"
#include "ScriptExecutionContext.h"
#include "SecurityPolicyViolationEvent.h"
#include "StringWithDirection.h"
#include "StyleColor.h"
#include "Supplementable.h"
#include "TextResourceDecoder.h"
#include "Timer.h"
#include "TreeScope.h"
#include "UserActionElementSet.h"
#include "ViewportArguments.h"
#include "VisibilityState.h"
#include <pal/SessionID.h>
#include <wtf/Deque.h>
#include <wtf/Forward.h>
#include <wtf/HashCountedSet.h>
#include <wtf/HashSet.h>
#include <wtf/Logger.h>
#include <wtf/ObjectIdentifier.h>
#include <wtf/UniqueRef.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/AtomicStringHash.h>

#if PLATFORM(IOS_FAMILY)
#include "EventTrackingRegions.h"
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
#include <wtf/ThreadingPrimitives.h>
#endif

namespace JSC {
class ExecState;
class InputCursor;
}

namespace WebCore {

class ApplicationStateChangeListener;
class AXObjectCache;
class Attr;
class CDATASection;
class CSSCustomPropertyValue;
class CSSFontSelector;
class CSSStyleDeclaration;
class CSSStyleSheet;
class CachedCSSStyleSheet;
class CachedFrameBase;
class CachedResourceLoader;
class CachedScript;
class CanvasRenderingContext2D;
class CharacterData;
class Comment;
class ConstantPropertyMap;
class DOMImplementation;
class DOMSelection;
class DOMWindow;
class DOMWrapperWorld;
class Database;
class DatabaseThread;
class DeferredPromise;
class DocumentAnimationScheduler;
class DocumentFragment;
class DocumentLoader;
class DocumentMarkerController;
class DocumentParser;
class DocumentSharedObjectPool;
class DocumentTimeline;
class DocumentType;
class ExtensionStyleSheets;
class FloatQuad;
class FloatRect;
class FontFaceSet;
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
class HitTestLocation;
class HitTestRequest;
class HitTestResult;
class ImageBitmapRenderingContext;
class IntPoint;
class JSNode;
class LayoutPoint;
class LayoutRect;
class LiveNodeList;
class Locale;
class Location;
class MediaCanStartListener;
class MediaPlaybackTarget;
class MediaPlaybackTargetClient;
class MediaQueryList;
class MediaQueryMatcher;
class MouseEventWithHitTestResults;
class NodeFilter;
class NodeIterator;
class Page;
class PaintWorkletGlobalScope;
class PlatformMouseEvent;
class ProcessingInstruction;
class QualifiedName;
class Quirks;
class Range;
class RenderFullScreen;
class RenderTreeBuilder;
class RenderView;
class RequestAnimationFrameCallback;
class SVGDocumentExtensions;
class SVGSVGElement;
class SVGUseElement;
class SWClientConnection;
class ScriptElementData;
class ScriptModuleLoader;
class ScriptRunner;
class ScriptableDocumentParser;
class ScriptedAnimationController;
class SecurityOrigin;
class SegmentedString;
class SelectorQuery;
class SelectorQueryCache;
class SerializedScriptValue;
class Settings;
class StringCallback;
class StyleResolver;
class StyleSheet;
class StyleSheetContents;
class StyleSheetList;
class Text;
class TextResourceDecoder;
class TreeWalker;
class UndoManager;
class VisibilityChangeClient;
class VisitedLinkState;
class WebAnimation;
class WebGL2RenderingContext;
class WebGLRenderingContext;
class WebGPURenderingContext;
class WebMetalRenderingContext;
class WindowProxy;
class Worklet;
class XPathEvaluator;
class XPathExpression;
class XPathNSResolver;
class XPathResult;

template<typename> class ExceptionOr;

enum CollectionType;
enum class ShouldOpenExternalURLsPolicy : uint8_t;

enum class RouteSharingPolicy : uint8_t;

using PlatformDisplayID = uint32_t;

#if ENABLE(XSLT)
class TransformSource;
#endif

#if ENABLE(DASHBOARD_SUPPORT)
struct AnnotatedRegionValue;
#endif

#if ENABLE(TOUCH_EVENTS) || ENABLE(IOS_TOUCH_EVENTS)
class Touch;
class TouchList;
#endif

#if PLATFORM(IOS_FAMILY)
class DeviceMotionClient;
class DeviceMotionController;
class DeviceOrientationClient;
class DeviceOrientationController;
#endif

#if ENABLE(TEXT_AUTOSIZING)
class TextAutoSizing;
#endif

#if ENABLE(MEDIA_SESSION)
class MediaSession;
#endif

#if ENABLE(ATTACHMENT_ELEMENT)
class HTMLAttachmentElement;
#endif

#if ENABLE(INTERSECTION_OBSERVER)
class IntersectionObserver;
#endif

namespace Style {
class Scope;
};

const uint64_t HTMLMediaElementInvalidID = 0;

enum PageshowEventPersistence { PageshowEventNotPersisted, PageshowEventPersisted };

enum NodeListInvalidationType {
    DoNotInvalidateOnAttributeChanges,
    InvalidateOnClassAttrChange,
    InvalidateOnIdNameAttrChange,
    InvalidateOnNameAttrChange,
    InvalidateOnForTypeAttrChange,
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

enum class SelectionRestorationMode { Restore, SetDefault };

enum class HttpEquivPolicy {
    Enabled,
    DisabledBySettings,
    DisabledByContentDispositionAttachmentSandbox
};

enum class CustomElementNameValidationStatus {
    Valid,
    FirstCharacterIsNotLowercaseASCIILetter,
    ContainsNoHyphen,
    ContainsUppercaseASCIILetter,
    ContainsDisallowedCharacter,
    ConflictsWithStandardElementName
};

using RenderingContext = Variant<
#if ENABLE(WEBGL)
    RefPtr<WebGLRenderingContext>,
#endif
#if ENABLE(WEBGL2)
    RefPtr<WebGL2RenderingContext>,
#endif
#if ENABLE(WEBGPU)
    RefPtr<WebGPURenderingContext>,
#endif
#if ENABLE(WEBMETAL)
    RefPtr<WebMetalRenderingContext>,
#endif
    RefPtr<ImageBitmapRenderingContext>,
    RefPtr<CanvasRenderingContext2D>
>;

class DocumentParserYieldToken {
    WTF_MAKE_FAST_ALLOCATED;
public:
    WEBCORE_EXPORT DocumentParserYieldToken(Document&);
    WEBCORE_EXPORT ~DocumentParserYieldToken();

private:
    WeakPtr<Document> m_document;
};

class Document
    : public ContainerNode
    , public TreeScope
    , public ScriptExecutionContext
    , public FontSelectorClient
    , public CanMakeWeakPtr<Document>
    , public FrameDestructionObserver
    , public Supplementable<Document>
    , public Logger::Observer {
    WTF_MAKE_ISO_ALLOCATED(Document);
public:
    static Ref<Document> create(const URL&);
    static Ref<Document> createNonRenderedPlaceholder(Frame&, const URL&);
    static Ref<Document> create(Document&);

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
            m_refCount = 1; // Avoid double destruction through use of RefPtr<T>. (This is a security mitigation in case of programmer error. It will ASSERT in debug builds.)
            delete this;
        }
    }

    unsigned referencingNodeCount() const { return m_referencingNodeCount; }

    void removedLastRef();

    DocumentIdentifier identifier() const { return m_identifier; }

    using DocumentsMap = HashMap<DocumentIdentifier, Document*>;
    WEBCORE_EXPORT static DocumentsMap::ValuesIteratorRange allDocuments();
    WEBCORE_EXPORT static DocumentsMap& allDocumentsMap();

    MediaQueryMatcher& mediaQueryMatcher();

    using ContainerNode::ref;
    using ContainerNode::deref;
    using TreeScope::rootNode;

    bool canContainRangeEndPoint() const final { return true; }

    Element* elementForAccessKey(const String& key);
    void invalidateAccessKeyCache();

    ExceptionOr<SelectorQuery&> selectorQueryForString(const String&);
    void clearSelectorQueryCache();

    void setViewportArguments(const ViewportArguments& viewportArguments) { m_viewportArguments = viewportArguments; }
    ViewportArguments viewportArguments() const { return m_viewportArguments; }

    WEBCORE_EXPORT void setOverrideViewportArguments(const Optional<ViewportArguments>&);

    OptionSet<DisabledAdaptations> disabledAdaptations() const { return m_disabledAdaptations; }
#ifndef NDEBUG
    bool didDispatchViewportPropertiesChanged() const { return m_didDispatchViewportPropertiesChanged; }
#endif

    void setReferrerPolicy(ReferrerPolicy);
    ReferrerPolicy referrerPolicy() const { return m_referrerPolicy.valueOr(ReferrerPolicy::NoReferrerWhenDowngrade); }

    WEBCORE_EXPORT DocumentType* doctype() const;

    WEBCORE_EXPORT DOMImplementation& implementation();
    
    Element* documentElement() const { return m_documentElement.get(); }
    static ptrdiff_t documentElementMemoryOffset() { return OBJECT_OFFSETOF(Document, m_documentElement); }

    WEBCORE_EXPORT Element* activeElement();
    WEBCORE_EXPORT bool hasFocus() const;

    bool hasManifest() const;
    
    WEBCORE_EXPORT ExceptionOr<Ref<Element>> createElementForBindings(const AtomicString& tagName);
    WEBCORE_EXPORT Ref<DocumentFragment> createDocumentFragment();
    WEBCORE_EXPORT Ref<Text> createTextNode(const String& data);
    WEBCORE_EXPORT Ref<Comment> createComment(const String& data);
    WEBCORE_EXPORT ExceptionOr<Ref<CDATASection>> createCDATASection(const String& data);
    WEBCORE_EXPORT ExceptionOr<Ref<ProcessingInstruction>> createProcessingInstruction(const String& target, const String& data);
    WEBCORE_EXPORT ExceptionOr<Ref<Attr>> createAttribute(const String& name);
    WEBCORE_EXPORT ExceptionOr<Ref<Attr>> createAttributeNS(const AtomicString& namespaceURI, const String& qualifiedName, bool shouldIgnoreNamespaceChecks = false);
    WEBCORE_EXPORT ExceptionOr<Ref<Node>> importNode(Node& nodeToImport, bool deep);
    WEBCORE_EXPORT ExceptionOr<Ref<Element>> createElementNS(const AtomicString& namespaceURI, const String& qualifiedName);
    WEBCORE_EXPORT Ref<Element> createElement(const QualifiedName&, bool createdByParser);

    static CustomElementNameValidationStatus validateCustomElementName(const AtomicString&);

    WEBCORE_EXPORT RefPtr<Range> caretRangeFromPoint(int x, int y);
    RefPtr<Range> caretRangeFromPoint(const LayoutPoint& clientPoint);

    WEBCORE_EXPORT Element* scrollingElementForAPI();
    Element* scrollingElement();

    enum ReadyState { Loading, Interactive,  Complete };
    ReadyState readyState() const { return m_readyState; }

    WEBCORE_EXPORT String defaultCharsetForLegacyBindings() const;

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
    enum class StandaloneStatus : uint8_t { Unspecified, Standalone, NotStandalone };
    bool xmlStandalone() const { return m_xmlStandalone == StandaloneStatus::Standalone; }
    StandaloneStatus xmlStandaloneStatus() const { return m_xmlStandalone; }
    bool hasXMLDeclaration() const { return m_hasXMLDeclaration; }

    void setXMLEncoding(const String& encoding) { m_xmlEncoding = encoding; } // read-only property, only to be set from XMLDocumentParser
    WEBCORE_EXPORT ExceptionOr<void> setXMLVersion(const String&);
    WEBCORE_EXPORT void setXMLStandalone(bool);
    void setHasXMLDeclaration(bool hasXMLDeclaration) { m_hasXMLDeclaration = hasXMLDeclaration; }

    String documentURI() const { return m_documentURI; }
    WEBCORE_EXPORT void setDocumentURI(const String&);

    WEBCORE_EXPORT VisibilityState visibilityState() const;
    void visibilityStateChanged();
    WEBCORE_EXPORT bool hidden() const;

    void setTimerThrottlingEnabled(bool);
    bool isTimerThrottlingEnabled() const { return m_isTimerThrottlingEnabled; }

    WEBCORE_EXPORT ExceptionOr<Ref<Node>> adoptNode(Node& source);

    WEBCORE_EXPORT Ref<HTMLCollection> images();
    WEBCORE_EXPORT Ref<HTMLCollection> embeds();
    WEBCORE_EXPORT Ref<HTMLCollection> plugins(); // an alias for embeds() required for the JS DOM bindings.
    WEBCORE_EXPORT Ref<HTMLCollection> applets();
    WEBCORE_EXPORT Ref<HTMLCollection> links();
    WEBCORE_EXPORT Ref<HTMLCollection> forms();
    WEBCORE_EXPORT Ref<HTMLCollection> anchors();
    WEBCORE_EXPORT Ref<HTMLCollection> scripts();
    Ref<HTMLCollection> all();
    Ref<HTMLCollection> allFilteredByName(const AtomicString&);

    Ref<HTMLCollection> windowNamedItems(const AtomicString&);
    Ref<HTMLCollection> documentNamedItems(const AtomicString&);

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

    bool sawElementsInKnownNamespaces() const { return m_sawElementsInKnownNamespaces; }

    StyleResolver& userAgentShadowTreeStyleResolver();

    CSSFontSelector& fontSelector() { return m_fontSelector; }

    WEBCORE_EXPORT bool haveStylesheetsLoaded() const;
    bool isIgnoringPendingStylesheets() const { return m_ignorePendingStylesheets; }

    WEBCORE_EXPORT StyleSheetList& styleSheets();

    Style::Scope& styleScope() { return *m_styleScope; }
    const Style::Scope& styleScope() const { return *m_styleScope; }
    ExtensionStyleSheets& extensionStyleSheets() { return *m_extensionStyleSheets; }
    const ExtensionStyleSheets& extensionStyleSheets() const { return *m_extensionStyleSheets; }

    bool gotoAnchorNeededAfterStylesheetsLoad() { return m_gotoAnchorNeededAfterStylesheetsLoad; }
    void setGotoAnchorNeededAfterStylesheetsLoad(bool b) { m_gotoAnchorNeededAfterStylesheetsLoad = b; }

    void evaluateMediaQueryList();

    FormController& formController();
    Vector<String> formElementsState() const;
    void setStateForNewFormElements(const Vector<String>&);

    WEBCORE_EXPORT FrameView* view() const; // Can be null.
    WEBCORE_EXPORT Page* page() const; // Can be null.
    const Settings& settings() const { return m_settings.get(); }
    Settings& mutableSettings() { return m_settings.get(); }

    const Quirks& quirks() const { return m_quirks; }

    float deviceScaleFactor() const;

    WEBCORE_EXPORT bool useSystemAppearance() const;
    WEBCORE_EXPORT bool useDarkAppearance(const RenderStyle*) const;

    OptionSet<StyleColor::Options> styleColorOptions(const RenderStyle*) const;

    WEBCORE_EXPORT Ref<Range> createRange();

    // The last bool parameter is for ObjC bindings.
    WEBCORE_EXPORT Ref<NodeIterator> createNodeIterator(Node& root, unsigned long whatToShow = 0xFFFFFFFF, RefPtr<NodeFilter>&& = nullptr, bool = false);

    // The last bool parameter is for ObjC bindings.
    WEBCORE_EXPORT Ref<TreeWalker> createTreeWalker(Node& root, unsigned long whatToShow = 0xFFFFFFFF, RefPtr<NodeFilter>&& = nullptr, bool = false);

    // Special support for editing
    WEBCORE_EXPORT Ref<CSSStyleDeclaration> createCSSStyleDeclaration();
    Ref<Text> createEditingTextNode(const String&);

    enum class ResolveStyleType { Normal, Rebuild };
    void resolveStyle(ResolveStyleType = ResolveStyleType::Normal);
    WEBCORE_EXPORT bool updateStyleIfNeeded();
    bool needsStyleRecalc() const;
    unsigned lastStyleUpdateSizeForTesting() const { return m_lastStyleUpdateSizeForTesting; }

    WEBCORE_EXPORT void updateLayout();
    
    // updateLayoutIgnorePendingStylesheets() forces layout even if we are waiting for pending stylesheet loads,
    // so calling this may cause a flash of unstyled content (FOUC).
    enum class RunPostLayoutTasks { Asynchronously, Synchronously };
    WEBCORE_EXPORT void updateLayoutIgnorePendingStylesheets(RunPostLayoutTasks = RunPostLayoutTasks::Asynchronously);

    std::unique_ptr<RenderStyle> styleForElementIgnoringPendingStylesheets(Element&, const RenderStyle* parentStyle, PseudoId = PseudoId::None);

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
    void prepareForDestruction();

    // Override ScriptExecutionContext methods to do additional work
    WEBCORE_EXPORT bool shouldBypassMainWorldContentSecurityPolicy() const final;
    void suspendActiveDOMObjects(ReasonForSuspension) final;
    void resumeActiveDOMObjects(ReasonForSuspension) final;
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

    Optional<uint64_t> pageID() const;
    // to get visually ordered hebrew and arabic pages right
    void setVisuallyOrdered();
    bool visuallyOrdered() const { return m_visuallyOrdered; }
    
    WEBCORE_EXPORT DocumentLoader* loader() const;

    WEBCORE_EXPORT ExceptionOr<RefPtr<WindowProxy>> openForBindings(DOMWindow& activeWindow, DOMWindow& firstDOMWindow, const String& url, const AtomicString& name, const String& features);
    WEBCORE_EXPORT ExceptionOr<Document&> openForBindings(Document* responsibleDocument, const String&, const String&);

    // FIXME: We should rename this at some point and give back the name 'open' to the HTML specified ones.
    WEBCORE_EXPORT ExceptionOr<void> open(Document* responsibleDocument = nullptr);
    void implicitOpen();

    WEBCORE_EXPORT ExceptionOr<void> closeForBindings();

    // FIXME: We should rename this at some point and give back the name 'close' to the HTML specified one.
    WEBCORE_EXPORT void close();
    // In some situations (see the code), we ignore document.close().
    // explicitClose() bypass these checks and actually tries to close the
    // input stream.
    void explicitClose();
    // implicitClose() actually does the work of closing the input stream.
    void implicitClose();

    void cancelParsing();

    ExceptionOr<void> write(Document* responsibleDocument, SegmentedString&&);
    WEBCORE_EXPORT ExceptionOr<void> write(Document* responsibleDocument, Vector<String>&&);
    WEBCORE_EXPORT ExceptionOr<void> writeln(Document* responsibleDocument, Vector<String>&&);

    bool wellFormed() const { return m_wellFormed; }

    const URL& url() const final { return m_url; }
    void setURL(const URL&);
    const URL& urlForBindings() const { return m_url.isEmpty() ? WTF::blankURL() : m_url; }

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
    WEBCORE_EXPORT PAL::SessionID sessionID() const final;

    String userAgent(const URL&) const final;

    void disableEval(const String& errorMessage) final;
    void disableWebAssembly(const String& errorMessage) final;

#if ENABLE(INDEXED_DATABASE)
    IDBClient::IDBConnectionProxy* idbConnectionProxy() final;
#endif
    SocketProvider* socketProvider() final;

    bool canNavigate(Frame* targetFrame, const URL& destinationURL = URL());

    bool usesStyleBasedEditability() const;
    void setHasElementUsingStyleBasedEditability();
    
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

    void setReadyState(ReadyState);
    void setParsing(bool);
    bool parsing() const { return m_bParsing; }
    Seconds minimumLayoutDelay();

    bool shouldScheduleLayout();
    bool isLayoutTimerActive();
    Seconds timeSinceDocumentCreation() const;
    
    void setTextColor(const Color& color) { m_textColor = color; }
    const Color& textColor() const { return m_textColor; }

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

    enum class FocusRemovalEventsMode { Dispatch, DoNotDispatch };
    WEBCORE_EXPORT bool setFocusedElement(Element*, FocusDirection = FocusDirectionNone,
        FocusRemovalEventsMode = FocusRemovalEventsMode::Dispatch);
    Element* focusedElement() const { return m_focusedElement.get(); }
    UserActionElementSet& userActionElements()  { return m_userActionElements; }
    const UserActionElementSet& userActionElements() const { return m_userActionElements; }

    void setFocusNavigationStartingNode(Node*);
    Element* focusNavigationStartingNode(FocusDirection) const;

    enum class NodeRemoval { Node, ChildrenOfNode };
    void adjustFocusedNodeOnNodeRemoval(Node&, NodeRemoval = NodeRemoval::Node);
    void adjustFocusNavigationNodeOnNodeRemoval(Node&, NodeRemoval = NodeRemoval::Node);

    void hoveredElementDidDetach(Element&);
    void elementInActiveChainDidDetach(Element&);

    void updateHoverActiveState(const HitTestRequest&, Element*);

    // Updates for :target (CSS3 selector).
    void setCSSTarget(Element*);
    Element* cssTarget() const { return m_cssTarget; }
    static ptrdiff_t cssTargetMemoryOffset() { return OBJECT_OFFSETOF(Document, m_cssTarget); }

    WEBCORE_EXPORT void scheduleFullStyleRebuild();
    void scheduleStyleRecalc();
    void unscheduleStyleRecalc();
    bool hasPendingStyleRecalc() const;
    bool hasPendingFullStyleRebuild() const;

    void registerNodeListForInvalidation(LiveNodeList&);
    void unregisterNodeListForInvalidation(LiveNodeList&);
    WEBCORE_EXPORT void registerCollection(HTMLCollection&);
    void unregisterCollection(HTMLCollection&);
    void collectionCachedIdNameMap(const HTMLCollection&);
    void collectionWillClearIdNameMap(const HTMLCollection&);
    bool shouldInvalidateNodeListAndCollectionCaches() const;
    bool shouldInvalidateNodeListAndCollectionCachesForAttribute(const QualifiedName& attrName) const;

    template <typename InvalidationFunction>
    void invalidateNodeListAndCollectionCaches(InvalidationFunction);

    void attachNodeIterator(NodeIterator&);
    void detachNodeIterator(NodeIterator&);
    void moveNodeIteratorsToNewDocument(Node& node, Document& newDocument)
    {
        if (!m_nodeIterators.isEmpty())
            moveNodeIteratorsToNewDocumentSlowCase(node, newDocument);
    }

    void attachRange(Range&);
    void detachRange(Range&);

    void updateRangesAfterChildrenChanged(ContainerNode&);
    // nodeChildrenWillBeRemoved is used when removing all node children at once.
    void nodeChildrenWillBeRemoved(ContainerNode&);
    // nodeWillBeRemoved is only safe when removing one node at a time.
    void nodeWillBeRemoved(Node&);

    enum class AcceptChildOperation { Replace, InsertOrAdd };
    bool canAcceptChild(const Node& newChild, const Node* refChild, AcceptChildOperation) const;

    void textInserted(Node&, unsigned offset, unsigned length);
    void textRemoved(Node&, unsigned offset, unsigned length);
    void textNodesMerged(Text& oldNode, unsigned offset);
    void textNodeSplit(Text& oldNode);

    void createDOMWindow();
    void takeDOMWindowFrom(Document&);

    DOMWindow* domWindow() const { return m_domWindow.get(); }
    // In DOM Level 2, the Document's DOMWindow is called the defaultView.
    WEBCORE_EXPORT WindowProxy* windowProxy() const;

    bool hasBrowsingContext() const { return !!frame(); }

    Document& contextDocument() const;
    void setContextDocument(Document& document) { m_contextDocument = makeWeakPtr(document); }

    // Helper functions for forwarding DOMWindow event related tasks to the DOMWindow if it exists.
    void setWindowAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value, DOMWrapperWorld&);
    void setWindowAttributeEventListener(const AtomicString& eventType, RefPtr<EventListener>&&, DOMWrapperWorld&);
    EventListener* getWindowAttributeEventListener(const AtomicString& eventType, DOMWrapperWorld&);
    WEBCORE_EXPORT void dispatchWindowEvent(Event&, EventTarget* = nullptr);
    void dispatchWindowLoadEvent();

    WEBCORE_EXPORT ExceptionOr<Ref<Event>> createEvent(const String& eventType);

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
        FORCEUP_LISTENER                     = 1 << 16,
        RESIZE_LISTENER                      = 1 << 17
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

    CSSStyleDeclaration* getOverrideStyle(Element*, const String&) { return nullptr; }

    // Handles an HTTP header equivalent set by a meta tag using <meta http-equiv="..." content="...">. This is called
    // when a meta tag is encountered during document parsing, and also when a script dynamically changes or adds a meta
    // tag. This enables scripts to use meta tags to perform refreshes and set expiry dates in addition to them being
    // specified in an HTML file.
    void processHttpEquiv(const String& equiv, const String& content, bool isInDocumentHead);

#if PLATFORM(IOS_FAMILY)
    void processFormatDetection(const String&);

    // Called when <meta name="apple-mobile-web-app-orientations"> changes.
    void processWebAppOrientations();
#endif
    
    void processViewport(const String& features, ViewportArguments::Type origin);
    void processDisabledAdaptations(const String& adaptations);
    void updateViewportArguments();
    void processReferrerPolicy(const String& policy, ReferrerPolicySource);

#if ENABLE(DARK_MODE_CSS)
    void processSupportedColorSchemes(const String& colorSchemes);
#endif

    // Returns the owning element in the parent document.
    // Returns nullptr if this is the top level document.
    HTMLFrameOwnerElement* ownerElement() const;

    // Used by DOM bindings; no direction known.
    const String& title() const { return m_title.string; }
    WEBCORE_EXPORT void setTitle(const String&);

    WEBCORE_EXPORT const AtomicString& dir() const;
    WEBCORE_EXPORT void setDir(const AtomicString&);

    void titleElementAdded(Element& titleElement);
    void titleElementRemoved(Element& titleElement);
    void titleElementTextChanged(Element& titleElement);

    WEBCORE_EXPORT ExceptionOr<String> cookie();
    WEBCORE_EXPORT ExceptionOr<void> setCookie(const String&);

    WEBCORE_EXPORT String referrer() const;

    WEBCORE_EXPORT String origin() const final;

    WEBCORE_EXPORT String domain() const;
    ExceptionOr<void> setDomain(const String& newDomain);

    void overrideLastModified(const Optional<WallTime>&);
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

    bool isFullyActive() const;

    // The full URL corresponding to the "site for cookies" in the Same-Site Cookies spec.,
    // <https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-00>. It is either
    // the URL of the top-level document or the null URL depending on whether the registrable
    // domain of this document's URL matches the registrable domain of its parent's/opener's
    // URL. For the top-level document, it is set to the document's URL.
    const URL& siteForCookies() const { return m_siteForCookies; }
    void setSiteForCookies(const URL& url) { m_siteForCookies = url; }
    
    // The following implements the rule from HTML 4 for what valid names are.
    // To get this right for all the XML cases, we probably have to improve this or move it
    // and make it sensitive to the type of document.
    static bool isValidName(const String&);

    // The following breaks a qualified name into a prefix and a local name.
    // It also does a validity check, and returns an error if the qualified name is invalid.
    static ExceptionOr<std::pair<AtomicString, AtomicString>> parseQualifiedName(const String& qualifiedName);
    static ExceptionOr<QualifiedName> parseQualifiedName(const AtomicString& namespaceURI, const String& qualifiedName);

    // Checks to make sure prefix and namespace do not conflict (per DOM Core 3)
    static bool hasValidNamespaceForElements(const QualifiedName&);
    static bool hasValidNamespaceForAttributes(const QualifiedName&);

    // This is the "HTML body element" as defined by CSSOM View spec, the first body child of the
    // document element. See http://dev.w3.org/csswg/cssom-view/#the-html-body-element.
    WEBCORE_EXPORT HTMLBodyElement* body() const;

    // This is the "body element" as defined by HTML5, the first body or frameset child of the
    // document element. See https://html.spec.whatwg.org/multipage/dom.html#the-body-element-2.
    WEBCORE_EXPORT HTMLElement* bodyOrFrameset() const;
    WEBCORE_EXPORT ExceptionOr<void> setBodyOrFrameset(RefPtr<HTMLElement>&&);

    Location* location() const;

    WEBCORE_EXPORT HTMLHeadElement* head();

    DocumentMarkerController& markers() const { return *m_markers; }

    WEBCORE_EXPORT bool execCommand(const String& command, bool userInterface = false, const String& value = String());
    WEBCORE_EXPORT bool queryCommandEnabled(const String& command);
    WEBCORE_EXPORT bool queryCommandIndeterm(const String& command);
    WEBCORE_EXPORT bool queryCommandState(const String& command);
    WEBCORE_EXPORT bool queryCommandSupported(const String& command);
    WEBCORE_EXPORT String queryCommandValue(const String& command);

    UndoManager& undoManager() const { return m_undoManager.get(); }

    // designMode support
    enum InheritedBool { off = false, on = true, inherit };    
    void setDesignMode(InheritedBool value);
    InheritedBool getDesignMode() const;
    bool inDesignMode() const;
    WEBCORE_EXPORT String designMode() const;
    WEBCORE_EXPORT void setDesignMode(const String&);

    Document* parentDocument() const;
    WEBCORE_EXPORT Document& topDocument() const;
    
    ScriptRunner& scriptRunner() { return *m_scriptRunner; }
    ScriptModuleLoader& moduleLoader() { return *m_moduleLoader; }

    HTMLScriptElement* currentScript() const { return !m_currentScriptStack.isEmpty() ? m_currentScriptStack.last().get() : nullptr; }
    void pushCurrentScript(HTMLScriptElement*);
    void popCurrentScript();

    bool shouldDeferAsynchronousScriptsUntilParsingFinishes() const;

#if ENABLE(XSLT)
    void scheduleToApplyXSLTransforms();
    void applyPendingXSLTransformsNowIfScheduled();
    RefPtr<Document> transformSourceDocument() { return m_transformSourceDocument; }
    void setTransformSourceDocument(Document* document) { m_transformSourceDocument = document; }

    void setTransformSource(std::unique_ptr<TransformSource>);
    TransformSource* transformSource() const { return m_transformSource.get(); }
#endif

    void incDOMTreeVersion() { m_domTreeVersion = ++s_globalTreeVersion; }
    uint64_t domTreeVersion() const { return m_domTreeVersion; }

    WEBCORE_EXPORT String originIdentifierForPasteboard();

    // XPathEvaluator methods
    WEBCORE_EXPORT ExceptionOr<Ref<XPathExpression>> createExpression(const String& expression, RefPtr<XPathNSResolver>&&);
    WEBCORE_EXPORT Ref<XPathNSResolver> createNSResolver(Node* nodeResolver);
    WEBCORE_EXPORT ExceptionOr<Ref<XPathResult>> evaluate(const String& expression, Node* contextNode, RefPtr<XPathNSResolver>&&, unsigned short type, XPathResult*);

    bool hasNodesWithNonFinalStyle() const { return m_hasNodesWithNonFinalStyle; }
    void setHasNodesWithNonFinalStyle() { m_hasNodesWithNonFinalStyle = true; }
    bool hasNodesWithMissingStyle() const { return m_hasNodesWithMissingStyle; }
    void setHasNodesWithMissingStyle() { m_hasNodesWithMissingStyle = true; }

    // Extension for manipulating canvas drawing contexts for use in CSS
    Optional<RenderingContext> getCSSCanvasContext(const String& type, const String& name, int width, int height);
    HTMLCanvasElement* getCSSCanvasElement(const String& name);
    String nameForCSSCanvasElement(const HTMLCanvasElement&) const;

    bool isDNSPrefetchEnabled() const { return m_isDNSPrefetchEnabled; }
    void parseDNSPrefetchControlHeader(const String&);

    WEBCORE_EXPORT void postTask(Task&&) final; // Executes the task on context's thread asynchronously.

    ScriptedAnimationController* scriptedAnimationController() { return m_scriptedAnimationController.get(); }
    void suspendScriptedAnimationControllerCallbacks();
    void resumeScriptedAnimationControllerCallbacks();
    
    void windowScreenDidChange(PlatformDisplayID);

    void finishedParsing();

    enum PageCacheState { NotInPageCache, AboutToEnterPageCache, InPageCache };

    PageCacheState pageCacheState() const { return m_pageCacheState; }
    void setPageCacheState(PageCacheState);

    void registerForDocumentSuspensionCallbacks(Element&);
    void unregisterForDocumentSuspensionCallbacks(Element&);

    void documentWillBecomeInactive();
    void suspend(ReasonForSuspension);
    void resume(ReasonForSuspension);

    void registerForMediaVolumeCallbacks(Element&);
    void unregisterForMediaVolumeCallbacks(Element&);
    void mediaVolumeDidChange();

    bool audioPlaybackRequiresUserGesture() const;
    bool videoPlaybackRequiresUserGesture() const;

#if ENABLE(MEDIA_SESSION)
    MediaSession& defaultMediaSession();
#endif

    void registerForPrivateBrowsingStateChangedCallbacks(Element&);
    void unregisterForPrivateBrowsingStateChangedCallbacks(Element&);
    void storageBlockingStateDidChange();
    void privateBrowsingStateDidChange();

#if ENABLE(VIDEO_TRACK)
    void registerForCaptionPreferencesChangedCallbacks(Element&);
    void unregisterForCaptionPreferencesChangedCallbacks(Element&);
    void captionPreferencesChanged();
#endif

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    void registerForPageScaleFactorChangedCallbacks(HTMLMediaElement&);
    void unregisterForPageScaleFactorChangedCallbacks(HTMLMediaElement&);
    void pageScaleFactorChangedAndStable();
    void registerForUserInterfaceLayoutDirectionChangedCallbacks(HTMLMediaElement&);
    void unregisterForUserInterfaceLayoutDirectionChangedCallbacks(HTMLMediaElement&);
    void userInterfaceLayoutDirectionChanged();
#endif

    void registerForVisibilityStateChangedCallbacks(VisibilityChangeClient&);
    void unregisterForVisibilityStateChangedCallbacks(VisibilityChangeClient&);

#if ENABLE(VIDEO)
    void registerForAllowsMediaDocumentInlinePlaybackChangedCallbacks(HTMLMediaElement&);
    void unregisterForAllowsMediaDocumentInlinePlaybackChangedCallbacks(HTMLMediaElement&);
    void allowsMediaDocumentInlinePlaybackChanged();

    void stopAllMediaPlayback();
    void suspendAllMediaPlayback();
    void resumeAllMediaPlayback();
#endif

    WEBCORE_EXPORT void setShouldCreateRenderers(bool);
    bool shouldCreateRenderers();

    void setDecoder(RefPtr<TextResourceDecoder>&&);
    TextResourceDecoder* decoder() const { return m_decoder.get(); }

    WEBCORE_EXPORT String displayStringModifiedByEncoding(const String&) const;

#if ENABLE(DASHBOARD_SUPPORT)
    void setHasAnnotatedRegions(bool f) { m_hasAnnotatedRegions = f; }
    WEBCORE_EXPORT const Vector<AnnotatedRegionValue>& annotatedRegions() const;
#endif

    enum class AnnotationsAction { Invalidate, Update };
    void invalidateRenderingDependentRegions(AnnotationsAction = AnnotationsAction::Invalidate);
    void invalidateScrollbarDependentRegions();
    void updateZOrderDependentRegions();

    void removeAllEventListeners() final;

    WEBCORE_EXPORT const SVGDocumentExtensions* svgExtensions();
    WEBCORE_EXPORT SVGDocumentExtensions& accessSVGExtensions();

    void addSVGUseElement(SVGUseElement&);
    void removeSVGUseElement(SVGUseElement&);
    HashSet<SVGUseElement*> const svgUseElements() const { return m_svgUseElements; }

    void initSecurityContext();
    void initContentSecurityPolicy(ContentSecurityPolicy* previousPolicy);

    void updateURLForPushOrReplaceState(const URL&);
    void statePopped(Ref<SerializedScriptValue>&&);

    bool processingLoadEvent() const { return m_processingLoadEvent; }
    bool loadEventFinished() const { return m_loadEventFinished; }

    bool isContextThread() const final;
    bool isSecureContext() const final;
    bool isJSExecutionForbidden() const final { return false; }

    void enqueueWindowEvent(Ref<Event>&&);
    void enqueueDocumentEvent(Ref<Event>&&);
    void enqueueOverflowEvent(Ref<Event>&&);
    void dispatchPageshowEvent(PageshowEventPersistence);
    WEBCORE_EXPORT void enqueueSecurityPolicyViolationEvent(SecurityPolicyViolationEvent::Init&&);
    void enqueueHashchangeEvent(const String& oldURL, const String& newURL);
    void dispatchPopstateEvent(RefPtr<SerializedScriptValue>&& stateObject);
    DocumentEventQueue& eventQueue() const final { return m_eventQueue; }

    WEBCORE_EXPORT void addMediaCanStartListener(MediaCanStartListener&);
    WEBCORE_EXPORT void removeMediaCanStartListener(MediaCanStartListener&);
    MediaCanStartListener* takeAnyMediaCanStartListener();

#if ENABLE(FULLSCREEN_API)
    bool webkitIsFullScreen() const { return m_fullScreenElement.get(); }
    bool webkitFullScreenKeyboardInputAllowed() const { return m_fullScreenElement.get() && m_areKeysEnabledInFullScreen; }
    Element* webkitCurrentFullScreenElement() const { return m_fullScreenElement.get(); }
    Element* webkitCurrentFullScreenElementForBindings() const { return ancestorElementInThisScope(webkitCurrentFullScreenElement()); }

    enum FullScreenCheckType {
        EnforceIFrameAllowFullScreenRequirement,
        ExemptIFrameAllowFullScreenRequirement,
    };

    void requestFullScreenForElement(Element*, FullScreenCheckType);
    WEBCORE_EXPORT void webkitCancelFullScreen();
    
    WEBCORE_EXPORT void webkitWillEnterFullScreen(Element&);
    WEBCORE_EXPORT void webkitDidEnterFullScreen();
    WEBCORE_EXPORT void webkitWillExitFullScreen();
    WEBCORE_EXPORT void webkitDidExitFullScreen();
    
    void setFullScreenRenderer(RenderTreeBuilder&, RenderFullScreen&);
    RenderFullScreen* fullScreenRenderer() const { return m_fullScreenRenderer.get(); }

    void dispatchFullScreenChangeEvents();
    bool fullScreenIsAllowedForElement(Element&) const;
    void fullScreenElementRemoved();
    void adjustFullScreenElementOnNodeRemoval(Node&, NodeRemoval = NodeRemoval::Node);

    WEBCORE_EXPORT bool isAnimatingFullScreen() const;
    WEBCORE_EXPORT void setAnimatingFullScreen(bool);

    WEBCORE_EXPORT bool areFullscreenControlsHidden() const;
    WEBCORE_EXPORT void setFullscreenControlsHidden(bool);

    WEBCORE_EXPORT bool webkitFullscreenEnabled() const;
    Element* webkitFullscreenElement() const { return !m_fullScreenElementStack.isEmpty() ? m_fullScreenElementStack.last().get() : nullptr; }
    Element* webkitFullscreenElementForBindings() const { return ancestorElementInThisScope(webkitFullscreenElement()); }
    WEBCORE_EXPORT void webkitExitFullscreen();
#endif

#if ENABLE(POINTER_LOCK)
    WEBCORE_EXPORT void exitPointerLock();
#endif

    // Used to allow element that loads data without going through a FrameLoader to delay the 'load' event.
    void incrementLoadEventDelayCount() { ++m_loadEventDelayCount; }
    void decrementLoadEventDelayCount();
    bool isDelayingLoadEvent() const { return m_loadEventDelayCount; }
    void checkCompleted();

#if ENABLE(IOS_TOUCH_EVENTS)
#include <WebKitAdditions/DocumentIOS.h>
#endif

#if ENABLE(DEVICE_ORIENTATION) && PLATFORM(IOS_FAMILY)
    DeviceMotionController& deviceMotionController() const;
    DeviceOrientationController& deviceOrientationController() const;
    WEBCORE_EXPORT void simulateDeviceOrientationChange(double alpha, double beta, double gamma);
#endif

    const DocumentTiming& timing() const { return m_documentTiming; }

    WEBCORE_EXPORT double monotonicTimestamp() const;

    int requestAnimationFrame(Ref<RequestAnimationFrameCallback>&&);
    void cancelAnimationFrame(int id);

    EventTarget* errorEventTarget() final;
    void logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, int columnNumber, RefPtr<Inspector::ScriptCallStack>&&) final;

    void initDNSPrefetch();

    void didAddWheelEventHandler(Node&);
    void didRemoveWheelEventHandler(Node&, EventHandlerRemoval = EventHandlerRemoval::One);

    MonotonicTime lastHandledUserGestureTimestamp() const { return m_lastHandledUserGestureTimestamp; }
    bool hasHadUserInteraction() const { return static_cast<bool>(m_lastHandledUserGestureTimestamp); }
    void updateLastHandledUserGestureTimestamp(MonotonicTime);
    bool processingUserGestureForMedia() const;

    void setUserDidInteractWithPage(bool userDidInteractWithPage) { ASSERT(&topDocument() == this); m_userDidInteractWithPage = userDidInteractWithPage; }
    bool userDidInteractWithPage() const { ASSERT(&topDocument() == this); return m_userDidInteractWithPage; }

    // Used for testing. Count handlers in the main document, and one per frame which contains handlers.
    WEBCORE_EXPORT unsigned wheelEventHandlerCount() const;
    WEBCORE_EXPORT unsigned touchEventHandlerCount() const;

    WEBCORE_EXPORT void startTrackingStyleRecalcs();
    WEBCORE_EXPORT unsigned styleRecalcCount() const;

#if ENABLE(TOUCH_EVENTS)
    bool hasTouchEventHandlers() const { return (m_touchEventTargets.get()) ? m_touchEventTargets->size() : false; }
    bool touchEventTargetsContain(Node& node) const { return m_touchEventTargets ? m_touchEventTargets->contains(&node) : false; }
#else
    bool hasTouchEventHandlers() const { return false; }
    bool touchEventTargetsContain(Node&) const { return false; }
#endif
#if ENABLE(POINTER_EVENTS)
    void updateTouchActionElements(Element&, const RenderStyle&);
    const HashSet<RefPtr<Element>>* touchActionElements() const { return m_touchActionElements.get(); }
#endif

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
    RegionFixedPair absoluteEventRegionForNode(Node&);
    RegionFixedPair absoluteRegionForEventTargets(const EventTargetSet*);

    LayoutRect absoluteEventHandlerBounds(bool&) final;

    bool visualUpdatesAllowed() const { return m_visualUpdatesAllowed; }

    bool isInDocumentWrite() { return m_writeRecursionDepth > 0; }

    void suspendScheduledTasks(ReasonForSuspension);
    void resumeScheduledTasks(ReasonForSuspension);

#if ENABLE(CSS_DEVICE_ADAPTATION)
    IntSize initialViewportSize() const;
#endif

    void convertAbsoluteToClientQuads(Vector<FloatQuad>&, const RenderStyle&);
    void convertAbsoluteToClientRects(Vector<FloatRect>&, const RenderStyle&);
    void convertAbsoluteToClientRect(FloatRect&, const RenderStyle&);

    bool hasActiveParser();
    void incrementActiveParserCount() { ++m_activeParserCount; }
    void decrementActiveParserCount();

    std::unique_ptr<DocumentParserYieldToken> createParserYieldToken()
    {
        return std::make_unique<DocumentParserYieldToken>(*this);
    }

    bool hasActiveParserYieldToken() const { return m_parserYieldTokenCount; }

    DocumentSharedObjectPool* sharedObjectPool() { return m_sharedObjectPool.get(); }

    void invalidateMatchedPropertiesCacheAndForceStyleRecalc();

    void didRemoveAllPendingStylesheet();
    void didClearStyleResolver();

    bool inStyleRecalc() const { return m_inStyleRecalc; }
    bool inRenderTreeUpdate() const { return m_inRenderTreeUpdate; }
    bool isResolvingTreeStyle() const { return m_isResolvingTreeStyle; }
    void setIsResolvingTreeStyle(bool);

    void updateTextRenderer(Text&, unsigned offsetOfReplacedText, unsigned lengthOfReplacedText);

    // Return a Locale for the default locale if the argument is null or empty.
    Locale& getCachedLocale(const AtomicString& locale = nullAtom());

    const Document* templateDocument() const;
    Document& ensureTemplateDocument();
    void setTemplateDocumentHost(Document* templateDocumentHost) { m_templateDocumentHost = templateDocumentHost; }
    Document* templateDocumentHost() { return m_templateDocumentHost; }

    void didAssociateFormControl(Element&);
    bool hasDisabledFieldsetElement() const { return m_disabledFieldsetElementsCount; }
    void addDisabledFieldsetElement() { m_disabledFieldsetElementsCount++; }
    void removeDisabledFieldsetElement() { ASSERT(m_disabledFieldsetElementsCount); m_disabledFieldsetElementsCount--; }

    WEBCORE_EXPORT void addConsoleMessage(std::unique_ptr<Inspector::ConsoleMessage>&&) final;

    // The following addConsoleMessage function is deprecated.
    // Callers should try to create the ConsoleMessage themselves.
    WEBCORE_EXPORT void addConsoleMessage(MessageSource, MessageLevel, const String& message, unsigned long requestIdentifier = 0) final;

    // The following addMessage function is deprecated.
    // Callers should try to create the ConsoleMessage themselves.
    void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, RefPtr<Inspector::ScriptCallStack>&&, JSC::ExecState* = nullptr, unsigned long requestIdentifier = 0) final;

    SecurityOrigin& securityOrigin() const { return *SecurityContext::securityOrigin(); }
    SecurityOrigin& topOrigin() const final { return topDocument().securityOrigin(); }

    Ref<FontFaceSet> fonts();

    void ensurePlugInsInjectedScript(DOMWrapperWorld&);

    void setVisualUpdatesAllowedByClient(bool);

#if ENABLE(WEB_CRYPTO)
    bool wrapCryptoKey(const Vector<uint8_t>& key, Vector<uint8_t>& wrappedKey) final;
    bool unwrapCryptoKey(const Vector<uint8_t>& wrappedKey, Vector<uint8_t>& key) final;
#endif

    void setHasStyleWithViewportUnits() { m_hasStyleWithViewportUnits = true; }
    bool hasStyleWithViewportUnits() const { return m_hasStyleWithViewportUnits; }
    void updateViewportUnitsOnResize();

    WEBCORE_EXPORT void addAudioProducer(MediaProducer&);
    WEBCORE_EXPORT void removeAudioProducer(MediaProducer&);
    MediaProducer::MediaStateFlags mediaState() const { return m_mediaState; }
    void noteUserInteractionWithMediaElement();
    bool isCapturing() const { return MediaProducer::isCapturing(m_mediaState); }
    WEBCORE_EXPORT void updateIsPlayingMedia(uint64_t = HTMLMediaElementInvalidID);
    void pageMutedStateDidChange();

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    void addPlaybackTargetPickerClient(MediaPlaybackTargetClient&);
    void removePlaybackTargetPickerClient(MediaPlaybackTargetClient&);
    void showPlaybackTargetPicker(MediaPlaybackTargetClient&, bool, RouteSharingPolicy, const String&);
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

    void addAppearanceDependentPicture(HTMLPictureElement&);
    void removeAppearanceDependentPicture(HTMLPictureElement&);

#if ENABLE(INTERSECTION_OBSERVER)
    void addIntersectionObserver(IntersectionObserver&);
    void removeIntersectionObserver(IntersectionObserver&);
    unsigned numberOfIntersectionObservers() const { return m_intersectionObservers.size(); }
    void scheduleForcedIntersectionObservationUpdate();
    void updateIntersectionObservations();
#endif

#if ENABLE(MEDIA_STREAM)
    void setHasCaptureMediaStreamTrack() { m_hasHadCaptureMediaStreamTrack = true; }
    bool hasHadCaptureMediaStreamTrack() const { return m_hasHadCaptureMediaStreamTrack; }
    void setDeviceIDHashSalt(const String&);
    String deviceIDHashSalt() const { return m_idHashSalt; }
    void stopMediaCapture();
    void registerForMediaStreamStateChangeCallbacks(HTMLMediaElement&);
    void unregisterForMediaStreamStateChangeCallbacks(HTMLMediaElement&);
    void mediaStreamCaptureStateChanged();
#endif

// FIXME: Find a better place for this functionality.
#if ENABLE(TELEPHONE_NUMBER_DETECTION)
    // These functions provide a two-level setting:
    //    - A user-settable wantsTelephoneNumberParsing (at the Page / WebView level)
    //    - A read-only telephoneNumberParsingAllowed which is set by the
    //      document if it has the appropriate meta tag.
    //    - isTelephoneNumberParsingEnabled() == isTelephoneNumberParsingAllowed() && page()->settings()->isTelephoneNumberParsingEnabled()
    WEBCORE_EXPORT bool isTelephoneNumberParsingAllowed() const;
    WEBCORE_EXPORT bool isTelephoneNumberParsingEnabled() const;
#endif

    using ContainerNode::setAttributeEventListener;
    void setAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value, DOMWrapperWorld& isolatedWorld);

    DOMSelection* getSelection();

    void didInsertInDocumentShadowRoot(ShadowRoot&);
    void didRemoveInDocumentShadowRoot(ShadowRoot&);
    const HashSet<ShadowRoot*>& inDocumentShadowRoots() const { return m_inDocumentShadowRoots; }

    void attachToCachedFrame(CachedFrameBase&);
    void detachFromCachedFrame(CachedFrameBase&);

    ConstantPropertyMap& constantProperties() const { return *m_constantPropertyMap; }

    void orientationChanged(int orientation);
    OrientationNotifier& orientationNotifier() { return m_orientationNotifier; }

    WEBCORE_EXPORT const AtomicString& bgColor() const;
    WEBCORE_EXPORT void setBgColor(const String&);
    WEBCORE_EXPORT const AtomicString& fgColor() const;
    WEBCORE_EXPORT void setFgColor(const String&);
    WEBCORE_EXPORT const AtomicString& alinkColor() const;
    WEBCORE_EXPORT void setAlinkColor(const String&);
    WEBCORE_EXPORT const AtomicString& linkColorForBindings() const;
    WEBCORE_EXPORT void setLinkColorForBindings(const String&);
    WEBCORE_EXPORT const AtomicString& vlinkColor() const;
    WEBCORE_EXPORT void setVlinkColor(const String&);

    // Per https://html.spec.whatwg.org/multipage/obsolete.html#dom-document-clear, this method does nothing.
    void clear() { }
    // Per https://html.spec.whatwg.org/multipage/obsolete.html#dom-document-captureevents, this method does nothing.
    void captureEvents() { }
    // Per https://html.spec.whatwg.org/multipage/obsolete.html#dom-document-releaseevents, this method does nothing.
    void releaseEvents() { }

#if ENABLE(TEXT_AUTOSIZING)
    TextAutoSizing& textAutoSizing();
#endif

    Logger& logger();

    void hasStorageAccess(Ref<DeferredPromise>&& passedPromise);
    void requestStorageAccess(Ref<DeferredPromise>&& passedPromise);
    void setUserGrantsStorageAccessOverride(bool value) { m_grantStorageAccessOverride = value; }

    WEBCORE_EXPORT void setConsoleMessageListener(RefPtr<StringCallback>&&); // For testing.

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    DocumentAnimationScheduler& animationScheduler();
#endif

    WEBCORE_EXPORT DocumentTimeline& timeline();
    DocumentTimeline* existingTimeline() const { return m_timeline.get(); }
    Vector<RefPtr<WebAnimation>> getAnimations();
        
#if ENABLE(ATTACHMENT_ELEMENT)
    void registerAttachmentIdentifier(const String&);
    void didInsertAttachmentElement(HTMLAttachmentElement&);
    void didRemoveAttachmentElement(HTMLAttachmentElement&);
    WEBCORE_EXPORT RefPtr<HTMLAttachmentElement> attachmentForIdentifier(const String&) const;
    const HashMap<String, Ref<HTMLAttachmentElement>>& attachmentElementsByIdentifier() const { return m_attachmentIdentifierToElementMap; }
#endif

#if ENABLE(SERVICE_WORKER)
    void setServiceWorkerConnection(SWClientConnection*);
#endif

    void addApplicationStateChangeListener(ApplicationStateChangeListener&);
    void removeApplicationStateChangeListener(ApplicationStateChangeListener&);
    void forEachApplicationStateChangeListener(const Function<void(ApplicationStateChangeListener&)>&);

#if ENABLE(IOS_TOUCH_EVENTS)
    bool handlingTouchEvent() const { return m_handlingTouchEvent; }
#endif

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    bool hasRequestedPageSpecificStorageAccessWithUserInteraction(const String& primaryDomain);
    void setHasRequestedPageSpecificStorageAccessWithUserInteraction(const String& primaryDomain);
#endif

    String signedPublicKeyAndChallengeString(unsigned keySizeIndex, const String& challengeString, const URL&);

    void consumeTemporaryTimeUserGesture();

    void registerArticleElement(Element&);
    void unregisterArticleElement(Element&);
    void updateMainArticleElementAfterLayout();
    bool hasMainArticleElement() const { return !!m_mainArticleElement; }

    const CSSRegisteredCustomPropertySet& getCSSRegisteredCustomPropertySet() const { return m_CSSRegisteredPropertySet; }
    bool registerCSSProperty(CSSRegisteredCustomProperty&&);

#if ENABLE(CSS_PAINTING_API)
    Worklet& ensurePaintWorklet();
    PaintWorkletGlobalScope* paintWorkletGlobalScopeForName(const String& name);
    void setPaintWorkletGlobalScopeForName(const String& name, Ref<PaintWorkletGlobalScope>&&);
#endif

    void setAsRunningUserScripts() { m_isRunningUserScripts = true; }
    bool isRunningUserScripts() const { return m_isRunningUserScripts; }

    void frameWasDisconnectedFromOwner();

    WEBCORE_EXPORT bool hitTest(const HitTestRequest&, HitTestResult&);
    bool hitTest(const HitTestRequest&, const HitTestLocation&, HitTestResult&);
#if !ASSERT_DISABLED
    bool inHitTesting() const { return m_inHitTesting; }
#endif

protected:
    enum ConstructionFlags { Synthesized = 1, NonRenderedPlaceholder = 1 << 1 };
    Document(Frame*, const URL&, unsigned = DefaultDocumentClass, unsigned constructionFlags = 0);

    void clearXMLVersion() { m_xmlVersion = String(); }

    virtual Ref<Document> cloneDocumentWithoutChildren() const;

private:
    friend class DocumentParserYieldToken;
    friend class Node;
    friend class ThrowOnDynamicMarkupInsertionCountIncrementer;
    friend class IgnoreOpensDuringUnloadCountIncrementer;
    friend class IgnoreDestructiveWriteCountIncrementer;

    bool shouldInheritContentSecurityPolicy() const;

    void updateTitleElement(Element& changingTitleElement);
    void willDetachPage() final;
    void frameDestroyed() final;

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

    Seconds minimumDOMTimerInterval() const final;

    Seconds domTimerAlignmentInterval(bool hasReachedMaxNestingLevel) const final;

    void updateTitleFromTitleElement();
    void updateTitle(const StringWithDirection&);
    void updateBaseURL();

    void invalidateAccessKeyCacheSlowCase();
    void buildAccessKeyCache();

    void moveNodeIteratorsToNewDocumentSlowCase(Node&, Document&);

    void loadEventDelayTimerFired();

    void pendingTasksTimerFired();
    bool isCookieAverse() const;

    void detachFromFrame();

    template<CollectionType> Ref<HTMLCollection> ensureCachedCollection();

#if ENABLE(FULLSCREEN_API)
    void dispatchFullScreenChangeOrErrorEvent(Deque<RefPtr<Node>>&, const AtomicString& eventName, bool shouldNotifyMediaElement);
    void clearFullscreenElementStack();
    void popFullscreenElementStack();
    void pushFullscreenElementStack(Element&);
    void addDocumentToFullScreenChangeEventQueue(Document&);
#endif

    void dispatchDisabledAdaptationsDidChangeForMainFrame();

    void setVisualUpdatesAllowed(ReadyState);
    void setVisualUpdatesAllowed(bool);
    void visualUpdatesSuppressionTimerFired();

    void addListenerType(ListenerType listenerType) { m_listenerTypes |= listenerType; }

    void didAssociateFormControlsTimerFired();

    void wheelEventHandlersChanged();

#if ENABLE(DASHBOARD_SUPPORT)
    void setAnnotatedRegionsDirty(bool f = true) { m_annotatedRegionsDirty = f; }
    bool annotatedRegionsDirty() const { return m_annotatedRegionsDirty; }
    bool hasAnnotatedRegions () const { return m_hasAnnotatedRegions; }
    void setAnnotatedRegions(const Vector<AnnotatedRegionValue>&);
    void updateAnnotatedRegions();
#endif

    HttpEquivPolicy httpEquivPolicy() const;
    AXObjectCache* existingAXObjectCacheSlow() const;

    // DOM Cookies caching.
    const String& cachedDOMCookies() const { return m_cachedDOMCookies; }
    void setCachedDOMCookies(const String&);
    bool isDOMCookieCacheValid() const { return m_cookieCacheExpiryTimer.isActive(); }
    void invalidateDOMCookieCache();
    void didLoadResourceSynchronously() final;

    void checkViewportDependentPictures();
    void checkAppearanceDependentPictures();

    bool canNavigateInternal(Frame& targetFrame);
    bool isNavigationBlockedByThirdPartyIFrameRedirectBlocking(Frame& targetFrame, const URL& destinationURL);

#if ENABLE(INTERSECTION_OBSERVER)
    void notifyIntersectionObserversTimerFired();
#endif

#if USE(QUICK_LOOK)
    bool shouldEnforceQuickLookSandbox() const;
    void applyQuickLookSandbox();
#endif

    bool shouldEnforceHTTP09Sandbox() const;

    void platformSuspendOrStopActiveDOMObjects();

    bool domainIsRegisterable(const String&) const;

    void enableTemporaryTimeUserGesture();

    bool isBodyPotentiallyScrollable(HTMLBodyElement&);

    const Ref<Settings> m_settings;

    UniqueRef<Quirks> m_quirks;

    std::unique_ptr<StyleResolver> m_userAgentShadowTreeStyleResolver;

    RefPtr<DOMWindow> m_domWindow;
    WeakPtr<Document> m_contextDocument;

    Ref<CachedResourceLoader> m_cachedResourceLoader;
    RefPtr<DocumentParser> m_parser;

    unsigned m_parserYieldTokenCount { 0 };

    // Document URLs.
    URL m_url; // Document.URL: The URL from which this document was retrieved.
    URL m_baseURL; // Node.baseURI: The URL to use when resolving relative URLs.
    URL m_baseURLOverride; // An alternative base URL that takes precedence over m_baseURL (but not m_baseElementURL).
    URL m_baseElementURL; // The URL set by the <base> element.
    URL m_cookieURL; // The URL to use for cookie access.
    URL m_firstPartyForCookies; // The policy URL for third-party cookie blocking.
    URL m_siteForCookies; // The policy URL for Same-Site cookies.

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

    RefPtr<Node> m_focusNavigationStartingNode;
    RefPtr<Element> m_focusedElement;
    RefPtr<Element> m_hoveredElement;
    RefPtr<Element> m_activeElement;
    RefPtr<Element> m_documentElement;
    UserActionElementSet m_userActionElements;

    uint64_t m_domTreeVersion;
    static uint64_t s_globalTreeVersion;

    String m_uniqueIdentifier;

    HashSet<NodeIterator*> m_nodeIterators;
    HashSet<Range*> m_ranges;

    std::unique_ptr<Style::Scope> m_styleScope;
    std::unique_ptr<ExtensionStyleSheets> m_extensionStyleSheets;
    RefPtr<StyleSheetList> m_styleSheetList;

    std::unique_ptr<FormController> m_formController;

    Color m_textColor { Color::black };
    Color m_linkColor;
    Color m_visitedLinkColor;
    Color m_activeLinkColor;
    const std::unique_ptr<VisitedLinkState> m_visitedLinkState;

    StringWithDirection m_title;
    StringWithDirection m_rawTitle;
    RefPtr<Element> m_titleElement;

    std::unique_ptr<AXObjectCache> m_axObjectCache;
    const std::unique_ptr<DocumentMarkerController> m_markers;
    
    Timer m_styleRecalcTimer;

    Element* m_cssTarget { nullptr };

    RefPtr<SerializedScriptValue> m_pendingStateObject;
    MonotonicTime m_documentCreationTime;
    bool m_overMinimumLayoutThreshold { false };
    
    std::unique_ptr<ScriptRunner> m_scriptRunner;
    std::unique_ptr<ScriptModuleLoader> m_moduleLoader;

    Vector<RefPtr<HTMLScriptElement>> m_currentScriptStack;

#if ENABLE(XSLT)
    void applyPendingXSLTransformsTimerFired();

    std::unique_ptr<TransformSource> m_transformSource;
    RefPtr<Document> m_transformSourceDocument;
    Timer m_applyPendingXSLTransformsTimer;
    bool m_hasPendingXSLTransforms { false };
#endif

    String m_xmlEncoding;
    String m_xmlVersion;
    StandaloneStatus m_xmlStandalone { StandaloneStatus::Unspecified };
    bool m_hasXMLDeclaration { false };

    String m_contentLanguage;

    RefPtr<TextResourceDecoder> m_decoder;

    HashSet<LiveNodeList*> m_listsInvalidatedAtDocument;
    HashSet<HTMLCollection*> m_collectionsInvalidatedAtDocument;
    unsigned m_nodeListAndCollectionCounts[numNodeListInvalidationTypes];

    RefPtr<XPathEvaluator> m_xpathEvaluator;

    std::unique_ptr<SVGDocumentExtensions> m_svgExtensions;
    HashSet<SVGUseElement*> m_svgUseElements;

#if ENABLE(DARK_MODE_CSS)
    OptionSet<ColorSchemes> m_supportedColorSchemes;
    bool m_allowsColorSchemeTransformations { true };
#endif

#if ENABLE(DASHBOARD_SUPPORT)
    Vector<AnnotatedRegionValue> m_annotatedRegions;
    bool m_hasAnnotatedRegions { false };
    bool m_annotatedRegionsDirty { false };
#endif

    HashMap<String, RefPtr<HTMLCanvasElement>> m_cssCanvasElements;

    HashSet<Element*> m_documentSuspensionCallbackElements;
    HashSet<Element*> m_mediaVolumeCallbackElements;
    HashSet<Element*> m_privateBrowsingStateChangedElements;
#if ENABLE(VIDEO_TRACK)
    HashSet<Element*> m_captionPreferencesChangedElements;
#endif

    Element* m_mainArticleElement { nullptr };
    HashSet<Element*> m_articleElements;

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    HashSet<HTMLMediaElement*> m_pageScaleFactorChangedElements;
    HashSet<HTMLMediaElement*> m_userInterfaceLayoutDirectionChangedElements;
#endif

    HashSet<VisibilityChangeClient*> m_visibilityStateCallbackClients;
#if ENABLE(VIDEO)
    HashSet<HTMLMediaElement*> m_allowsMediaDocumentInlinePlaybackElements;
#endif

    std::unique_ptr<HashMap<String, Element*, ASCIICaseInsensitiveHash>> m_accessKeyCache;

    std::unique_ptr<ConstantPropertyMap> m_constantPropertyMap;

    std::unique_ptr<SelectorQueryCache> m_selectorQueryCache;

    DocumentClassFlags m_documentClasses;

    RenderPtr<RenderView> m_renderView;
    mutable DocumentEventQueue m_eventQueue;

    HashSet<MediaCanStartListener*> m_mediaCanStartListeners;

#if ENABLE(FULLSCREEN_API)
    RefPtr<Element> m_fullScreenElement;
    Vector<RefPtr<Element>> m_fullScreenElementStack;
    WeakPtr<RenderFullScreen> m_fullScreenRenderer { nullptr };
    GenericTaskQueue<Timer> m_fullScreenTaskQueue;
    Deque<RefPtr<Node>> m_fullScreenChangeEventTargetQueue;
    Deque<RefPtr<Node>> m_fullScreenErrorEventTargetQueue;
    LayoutRect m_savedPlaceholderFrameRect;
    std::unique_ptr<RenderStyle> m_savedPlaceholderRenderStyle;

    bool m_areKeysEnabledInFullScreen { false };
    bool m_isAnimatingFullScreen { false };
    bool m_areFullscreenControlsHidden { false };
#endif

    HashSet<HTMLPictureElement*> m_viewportDependentPictures;
    HashSet<HTMLPictureElement*> m_appearanceDependentPictures;

#if ENABLE(INTERSECTION_OBSERVER)
    Vector<WeakPtr<IntersectionObserver>> m_intersectionObservers;
    Vector<WeakPtr<IntersectionObserver>> m_intersectionObserversWithPendingNotifications;
    Timer m_intersectionObserversNotifyTimer;
#endif

    Timer m_loadEventDelayTimer;

    ViewportArguments m_viewportArguments;
    Optional<ViewportArguments> m_overrideViewportArguments;
    OptionSet<DisabledAdaptations> m_disabledAdaptations;

    DocumentTiming m_documentTiming;

    RefPtr<MediaQueryMatcher> m_mediaQueryMatcher;
    
#if ENABLE(TOUCH_EVENTS)
    std::unique_ptr<EventTargetSet> m_touchEventTargets;
#endif
#if ENABLE(POINTER_EVENTS)
    std::unique_ptr<HashSet<RefPtr<Element>>> m_touchActionElements;
#endif
    std::unique_ptr<EventTargetSet> m_wheelEventTargets;

    MonotonicTime m_lastHandledUserGestureTimestamp;

    void clearScriptedAnimationController();
    RefPtr<ScriptedAnimationController> m_scriptedAnimationController;

    void notifyMediaCaptureOfVisibilityChanged();

    void didLogMessage(const WTFLogChannel&, WTFLogLevel, Vector<JSONLogValue>&&) final;

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    bool hasFrameSpecificStorageAccess() const;
    void setHasFrameSpecificStorageAccess(bool);
#endif

#if ENABLE(DEVICE_ORIENTATION) && PLATFORM(IOS_FAMILY)
    std::unique_ptr<DeviceMotionClient> m_deviceMotionClient;
    std::unique_ptr<DeviceMotionController> m_deviceMotionController;
    std::unique_ptr<DeviceOrientationClient> m_deviceOrientationClient;
    std::unique_ptr<DeviceOrientationController> m_deviceOrientationController;
#endif

    GenericTaskQueue<Timer> m_logMessageTaskQueue;

    Timer m_pendingTasksTimer;
    Vector<Task> m_pendingTasks;

#if ENABLE(TEXT_AUTOSIZING)
    std::unique_ptr<TextAutoSizing> m_textAutoSizing;
#endif

    Timer m_visualUpdatesSuppressionTimer;

    void clearSharedObjectPool();
    Timer m_sharedObjectPoolClearTimer;

    std::unique_ptr<DocumentSharedObjectPool> m_sharedObjectPool;

    typedef HashMap<AtomicString, std::unique_ptr<Locale>> LocaleIdentifierToLocaleMap;
    LocaleIdentifierToLocaleMap m_localeCache;

    RefPtr<Document> m_templateDocument;
    Document* m_templateDocumentHost { nullptr }; // Manually managed weakref (backpointer from m_templateDocument).

    Ref<CSSFontSelector> m_fontSelector;

    HashSet<MediaProducer*> m_audioProducers;

    HashSet<ShadowRoot*> m_inDocumentShadowRoots;

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    typedef HashMap<uint64_t, WebCore::MediaPlaybackTargetClient*> TargetIdToClientMap;
    TargetIdToClientMap m_idToClientMap;
    typedef HashMap<WebCore::MediaPlaybackTargetClient*, uint64_t> TargetClientToIdMap;
    TargetClientToIdMap m_clientToIDMap;
#endif

#if ENABLE(MEDIA_SESSION)
    RefPtr<MediaSession> m_defaultMediaSession;
#endif

#if ENABLE(INDEXED_DATABASE)
    RefPtr<IDBClient::IDBConnectionProxy> m_idbConnectionProxy;
#endif

#if ENABLE(ATTACHMENT_ELEMENT)
    HashMap<String, Ref<HTMLAttachmentElement>> m_attachmentIdentifierToElementMap;
#endif

    Timer m_didAssociateFormControlsTimer;
    Timer m_cookieCacheExpiryTimer;

    RefPtr<SocketProvider> m_socketProvider;

    String m_cachedDOMCookies;

    Optional<WallTime> m_overrideLastModified;

    HashSet<RefPtr<Element>> m_associatedFormControls;
    unsigned m_disabledFieldsetElementsCount { 0 };

    unsigned m_listenerTypes { 0 };
    unsigned m_referencingNodeCount { 0 };
    int m_loadEventDelayCount { 0 };
    unsigned m_lastStyleUpdateSizeForTesting { 0 };

    // https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#throw-on-dynamic-markup-insertion-counter
    unsigned m_throwOnDynamicMarkupInsertionCount { 0 };

    // https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#ignore-opens-during-unload-counter
    unsigned m_ignoreOpensDuringUnloadCount { 0 };

    // https://html.spec.whatwg.org/multipage/dynamic-markup-insertion.html#ignore-destructive-writes-counter
    unsigned m_ignoreDestructiveWriteCount { 0 };

    unsigned m_activeParserCount { 0 };
    unsigned m_styleRecalcCount { 0 };

    unsigned m_writeRecursionDepth { 0 };

    InheritedBool m_designMode { inherit };
    MediaProducer::MediaStateFlags m_mediaState { MediaProducer::IsNotPlaying };
    bool m_userHasInteractedWithMediaElement { false };
    PageCacheState m_pageCacheState { NotInPageCache };
    Optional<ReferrerPolicy> m_referrerPolicy;
    ReadyState m_readyState { Complete };

    MutationObserverOptions m_mutationObserverTypes { 0 };

    bool m_writeRecursionIsTooDeep { false };
    bool m_wellFormed { false };
    bool m_createRenderers { true };

    bool m_hasNodesWithNonFinalStyle { false };
    bool m_hasNodesWithMissingStyle { false };
    // But sometimes you need to ignore pending stylesheet count to
    // force an immediate layout when requested by JS.
    bool m_ignorePendingStylesheets { false };

    bool m_hasElementUsingStyleBasedEditability { false };
    bool m_focusNavigationStartingNodeIsRemoved { false };

    bool m_printing { false };
    bool m_paginatedForScreen { false };

    DocumentCompatibilityMode m_compatibilityMode { DocumentCompatibilityMode::NoQuirksMode };
    bool m_compatibilityModeLocked { false }; // This is cheaper than making setCompatibilityMode virtual.

    // FIXME: Merge these 2 variables into an enum. Also, FrameLoader::m_didCallImplicitClose
    // is almost a duplication of this data, so that should probably get merged in too.
    // FIXME: Document::m_processingLoadEvent and DocumentLoader::m_wasOnloadDispatched are roughly the same
    // and should be merged.
    bool m_processingLoadEvent { false };
    bool m_loadEventFinished { false };

    bool m_visuallyOrdered { false };
    bool m_bParsing { false }; // FIXME: rename

    bool m_needsFullStyleRebuild { false };
    bool m_inStyleRecalc { false };
    bool m_closeAfterStyleRecalc { false };
    bool m_inRenderTreeUpdate { false };
    bool m_isResolvingTreeStyle { false };

    bool m_gotoAnchorNeededAfterStylesheetsLoad { false };
    bool m_isDNSPrefetchEnabled { false };
    bool m_haveExplicitlyDisabledDNSPrefetch { false };

    bool m_isSynthesized { false };
    bool m_isNonRenderedPlaceholder { false };

    bool m_sawElementsInKnownNamespaces { false };
    bool m_isSrcdocDocument { false };

    bool m_hasInjectedPlugInsScript { false };
    bool m_renderTreeBeingDestroyed { false };
    bool m_hasPreparedForDestruction { false };

    bool m_hasStyleWithViewportUnits { false };
    bool m_isTimerThrottlingEnabled { false };
    bool m_isSuspended { false };

    bool m_scheduledTasksAreSuspended { false };
    bool m_visualUpdatesAllowed { true };

    bool m_areDeviceMotionAndOrientationUpdatesSuspended { false };
    bool m_userDidInteractWithPage { false };
#if !ASSERT_DISABLED
    bool m_inHitTesting { false };
#endif

#if ENABLE(TELEPHONE_NUMBER_DETECTION)
    bool m_isTelephoneNumberParsingAllowed { true };
#endif

#if ENABLE(INTERSECTION_OBSERVER)
    bool m_needsForcedIntersectionObservationUpdate { false };
#endif

#if ENABLE(MEDIA_STREAM)
    HashSet<HTMLMediaElement*> m_mediaStreamStateChangeElements;
    String m_idHashSalt;
    bool m_hasHadCaptureMediaStreamTrack { false };
#endif

#ifndef NDEBUG
    bool m_didDispatchViewportPropertiesChanged { false };
#endif

    OrientationNotifier m_orientationNotifier;
    mutable PAL::SessionID m_sessionID;
    mutable RefPtr<Logger> m_logger;
    RefPtr<StringCallback> m_consoleMessageListener;

    static bool hasEverCreatedAnAXObjectCache;

    bool m_grantStorageAccessOverride { false };

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    RefPtr<DocumentAnimationScheduler> m_animationScheduler;
#endif
    RefPtr<DocumentTimeline> m_timeline;
    DocumentIdentifier m_identifier;

#if ENABLE(SERVICE_WORKER)
    RefPtr<SWClientConnection> m_serviceWorkerConnection;
#endif

    HashSet<ApplicationStateChangeListener*> m_applicationStateChangeListeners;
    
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    String m_primaryDomainRequestedPageSpecificStorageAccessWithUserInteraction { };
#endif
    
    std::unique_ptr<UserGestureIndicator> m_temporaryUserGesture;

    CSSRegisteredCustomPropertySet m_CSSRegisteredPropertySet;

#if ENABLE(CSS_PAINTING_API)
    RefPtr<Worklet> m_paintWorklet;
    HashMap<String, Ref<PaintWorkletGlobalScope>> m_paintWorkletGlobalScopes;
#endif

    bool m_isRunningUserScripts { false };

    Ref<UndoManager> m_undoManager;
};

Element* eventTargetElementForDocument(Document*);

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

inline AXObjectCache* Document::existingAXObjectCache() const
{
    if (!hasEverCreatedAnAXObjectCache)
        return nullptr;
    return existingAXObjectCacheSlow();
}

inline Ref<Document> Document::create(const URL& url)
{
    return adoptRef(*new Document(nullptr, url));
}

inline Ref<Document> Document::createNonRenderedPlaceholder(Frame& frame, const URL& url)
{
    return adoptRef(*new Document(&frame, url, DefaultDocumentClass, NonRenderedPlaceholder));
}

inline void Document::invalidateAccessKeyCache()
{
    if (UNLIKELY(m_accessKeyCache))
        invalidateAccessKeyCacheSlowCase();
}

// These functions are here because they require the Document class definition and we want to inline them.

inline ScriptExecutionContext* Node::scriptExecutionContext() const
{
    return &document().contextDocument();
}

inline ActiveDOMObject::ActiveDOMObject(Document& document)
    : ActiveDOMObject(static_cast<ScriptExecutionContext*>(&document.contextDocument()))
{
}

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::Document)
    static bool isType(const WebCore::ScriptExecutionContext& context) { return context.isDocument(); }
    static bool isType(const WebCore::Node& node) { return node.isDocumentNode(); }
SPECIALIZE_TYPE_TRAITS_END()
