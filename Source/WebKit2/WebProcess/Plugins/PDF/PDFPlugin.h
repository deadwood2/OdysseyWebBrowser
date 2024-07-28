/*
 * Copyright (C) 2011, 2012, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PDFPlugin_h
#define PDFPlugin_h

#include "PDFKitImports.h"

#if ENABLE(PDFKIT_PLUGIN)
#if USE(DEPRECATED_PDF_PLUGIN)

#include "DeprecatedPDFPlugin.h"

#else // USE(DEPRECATED_PDF_PLUGIN)

#include "Plugin.h"
#include "WebEvent.h"
#include "WebHitTestResultData.h"
#include <WebCore/AffineTransform.h>
#include <WebCore/FindOptions.h>
#include <WebCore/PageOverlay.h>
#include <WebCore/ScrollableArea.h>
#include <wtf/RetainPtr.h>

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSValue* JSObjectRef;
typedef const struct OpaqueJSValue* JSValueRef;

OBJC_CLASS NSArray;
OBJC_CLASS NSAttributedString;
OBJC_CLASS NSData;
OBJC_CLASS NSString;
OBJC_CLASS PDFAnnotation;
OBJC_CLASS PDFLayerController;
OBJC_CLASS PDFSelection;
OBJC_CLASS WKPDFPluginAccessibilityObject;
OBJC_CLASS WKPDFLayerControllerDelegate;

namespace IPC {
class DataReference;
}

namespace WebCore {
class Element;
struct PluginInfo;
}

namespace WebKit {

class PDFPluginAnnotation;
class PDFPluginPasswordField;
class PluginView;
class WebFrame;

class PDFPlugin final : public Plugin {
public:
    static Ref<PDFPlugin> create(WebFrame*);
    ~PDFPlugin();

    static WebCore::PluginInfo pluginInfo();

    WebCore::IntSize size() const { return m_size; }
    float scaleFactor() const;

    void didMutatePDFDocument() { m_pdfDocumentWasMutated = true; }

    void setActiveAnnotation(PDFAnnotation *);

    void notifyDisplayModeChanged(int);

    void notifySelectionChanged(PDFSelection *);

    void clickedLink(NSURL *);
    void saveToPDF();
    void openWithNativeApplication();
    void writeItemsToPasteboard(NSString *pasteboardName, NSArray *items, NSArray *types);
    void showDefinitionForAttributedString(NSAttributedString *, CGPoint);
    void performWebSearch(NSString *);
    void performSpotlightSearch(NSString *);
    void invalidatePDFRect(WebCore::IntRect);
    void invalidateHUD();
    void scrollToPoint(WebCore::IntPoint);

    void zoomIn();
    void zoomOut();

    void focusNextAnnotation();
    void focusPreviousAnnotation();

    void attemptToUnlockPDF(const String& password);

    WebCore::IntPoint convertFromPluginToPDFView(const WebCore::IntPoint&) const;
    WebCore::IntPoint convertFromRootViewToPlugin(const WebCore::IntPoint&) const;
    WebCore::IntPoint convertFromPDFViewToRootView(const WebCore::IntPoint&) const;

    WebCore::IntRect convertFromPDFViewToRootView(const WebCore::IntRect&) const;
    WebCore::FloatRect convertFromPDFViewToScreen(const WebCore::FloatRect&) const;

    WebCore::IntRect boundsOnScreen() const;
    
    bool showContextMenuAtPoint(const WebCore::IntPoint&);

    String lookupTextAtLocation(const WebCore::FloatPoint&, WebHitTestResultData&, PDFSelection **, NSDictionary **) const;
    WebCore::FloatRect rectForSelectionInRootView(PDFSelection *) const;

    PDFLayerController *pdfLayerController() const { return m_pdfLayerController.get(); }
    WebFrame* webFrame() const { return m_frame; }

    bool isLocked() const;

private:
    explicit PDFPlugin(WebFrame*);

    // Plugin functions.
    virtual bool initialize(const Parameters&) override;
    virtual void destroy() override;
    virtual void paint(WebCore::GraphicsContext&, const WebCore::IntRect& dirtyRectInWindowCoordinates) override;
    virtual void updateControlTints(WebCore::GraphicsContext&) override { }
    virtual bool supportsSnapshotting() const override { return false; }
    virtual RefPtr<ShareableBitmap> snapshot() override;
    virtual PlatformLayer* pluginLayer() override { return nullptr; }
    virtual bool isTransparent() override { return false; }
    virtual bool wantsWheelEvents() override { return false; }
    virtual void geometryDidChange(const WebCore::IntSize& pluginSize, const WebCore::IntRect& clipRect, const WebCore::AffineTransform& pluginToRootViewTransform) override;
    virtual void contentsScaleFactorChanged(float) override { }
    virtual void visibilityDidChange(bool) override { }
    virtual void frameDidFinishLoading(uint64_t requestID) override;
    virtual void frameDidFail(uint64_t requestID, bool wasCancelled) override;
    virtual void didEvaluateJavaScript(uint64_t requestID, const String& result) override;
    virtual void streamWillSendRequest(uint64_t streamID, const WebCore::URL& requestURL, const WebCore::URL& responseURL, int responseStatus) override { }
    virtual void streamDidReceiveResponse(uint64_t streamID, const WebCore::URL& responseURL, uint32_t streamLength, uint32_t lastModifiedTime, const String& mimeType, const String& headers, const String& suggestedFileName) override;
    virtual void streamDidReceiveData(uint64_t streamID, const char* bytes, int length) override;
    virtual void streamDidFinishLoading(uint64_t streamID) override;
    virtual void streamDidFail(uint64_t streamID, bool wasCancelled) override;
    virtual void manualStreamDidReceiveResponse(const WebCore::URL& responseURL, uint32_t streamLength, uint32_t lastModifiedTime, const WTF::String& mimeType, const WTF::String& headers, const String& suggestedFileName) override;
    virtual void manualStreamDidReceiveData(const char* bytes, int length) override;
    virtual void manualStreamDidFinishLoading() override;
    virtual void manualStreamDidFail(bool wasCancelled) override;
    virtual bool handleMouseEvent(const WebMouseEvent&) override;
    virtual bool handleWheelEvent(const WebWheelEvent&) override { return false; }
    virtual bool handleMouseEnterEvent(const WebMouseEvent&) override;
    virtual bool handleMouseLeaveEvent(const WebMouseEvent&) override;
    virtual bool handleContextMenuEvent(const WebMouseEvent&) override;
    virtual bool handleKeyboardEvent(const WebKeyboardEvent&) override { return false; }
    virtual bool handleEditingCommand(const String& commandName, const String& argument) override;
    virtual bool isEditingCommandEnabled(const String&) override;
    virtual void setFocus(bool) override { }
    virtual NPObject* pluginScriptableNPObject() override { return nullptr; }
    virtual void windowFocusChanged(bool) override { }
    virtual void windowAndViewFramesChanged(const WebCore::IntRect& windowFrameInScreenCoordinates, const WebCore::IntRect& viewFrameInWindowCoordinates) override { }
    virtual void windowVisibilityChanged(bool) override { }
    virtual uint64_t pluginComplexTextInputIdentifier() const override { return 0; }
    virtual void sendComplexTextInput(const String& textInput) override { }
    virtual void setLayerHostingMode(LayerHostingMode) override { }
    virtual WebCore::Scrollbar* horizontalScrollbar() override { return nullptr; }
    virtual WebCore::Scrollbar* verticalScrollbar() override { return nullptr; }
    virtual void storageBlockingStateChanged(bool) override { }
    virtual void privateBrowsingStateChanged(bool) override { }
    virtual bool getFormValue(String& formValue) override { return false; }
    virtual bool handleScroll(WebCore::ScrollDirection, WebCore::ScrollGranularity) override { return false; }
    virtual RefPtr<WebCore::SharedBuffer> liveResourceData() const override;

    virtual bool handlesPageScaleFactor() const override { return false; }
    virtual bool requiresUnifiedScaleFactor() const override { return true; }

    virtual bool isBeingAsynchronouslyInitialized() const override { return false; }

    virtual RetainPtr<PDFDocument> pdfDocumentForPrinting() const override { return m_pdfDocument; }
    virtual NSObject *accessibilityObject() const override;

    virtual unsigned countFindMatches(const String& target, WebCore::FindOptions, unsigned maxMatchCount) override;
    virtual bool findString(const String& target, WebCore::FindOptions, unsigned maxMatchCount) override;

    PDFSelection *nextMatchForString(const String& target, BOOL searchForward, BOOL caseSensitive, BOOL wrapSearch, PDFSelection *initialSelection, BOOL startInSelection);

    bool performDictionaryLookupAtLocation(const WebCore::FloatPoint&) override;
    String getSelectionString() const override;
    String getSelectionForWordAtPoint(const WebCore::FloatPoint&) const override;
    bool existingSelectionContainsPoint(const WebCore::FloatPoint&) const override;

    virtual bool shouldAllowScripting() override { return false; }
    virtual bool shouldAllowNavigationFromDrags() override { return true; }
    virtual bool shouldAlwaysAutoStart() const override { return true; }
    virtual bool canCreateTransientPaintingSnapshot() const override { return false; }

    // PDFPlugin functions.
    void pdfDocumentDidLoad();
    void addArchiveResource();
    void computePageBoxes();
    void calculateSizes();
    void didCalculateSizes();
    void runScriptsInPDFDocument();

    NSEvent *nsEventForWebMouseEvent(const WebMouseEvent&);

    bool supportsForms();
    bool isFullFramePlugin() const;

    void createPasswordEntryForm();

    RetainPtr<PDFDocument> pdfDocument() const { return m_pdfDocument; }
    void setPDFDocument(RetainPtr<PDFDocument> document) { m_pdfDocument = document; }

    WebCore::IntSize pdfDocumentSize() const { return m_pdfDocumentSize; }
    void setPDFDocumentSize(WebCore::IntSize size) { m_pdfDocumentSize = size; }

    NSData *liveData() const;
    NSData *rawData() const { return (NSData *)m_data.get(); }

    enum class UpdateCursor {
        IfNeeded,
        Force
    };

    enum class HitTestResult {
        None,
        Text,
        HUD
    };

    void updateCursor(const WebMouseEvent&, UpdateCursor = UpdateCursor::IfNeeded);

    JSObjectRef makeJSPDFDoc(JSContextRef);
    static JSValueRef jsPDFDocPrint(JSContextRef, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

    void convertPostScriptDataIfNeeded();

    PluginView* pluginView();
    const PluginView* pluginView() const;

    class HUD : public WebCore::PageOverlay::Client {
    public:
        explicit HUD(PDFPlugin& plugin);
        virtual ~HUD();

        void invalidate();
        bool containsPointInRootView(WebCore::IntPoint);

        enum class AnimateVisibilityTransition { No, Yes };
        void setVisible(bool, AnimateVisibilityTransition);

    private:
        virtual void pageOverlayDestroyed(WebCore::PageOverlay&) override;
        virtual void willMoveToPage(WebCore::PageOverlay&, WebCore::Page*) override;
        virtual void didMoveToPage(WebCore::PageOverlay&, WebCore::Page*) override;
        virtual void drawRect(WebCore::PageOverlay&, WebCore::GraphicsContext&, const WebCore::IntRect& dirtyRect) override;
        virtual bool mouseEvent(WebCore::PageOverlay&, const WebCore::PlatformMouseEvent&) override;

        WebCore::IntRect frameInRootView() const;

        bool m_visible { false };
        Ref<WebCore::PageOverlay> m_overlay;
        PDFPlugin& m_plugin;
    };

    WebFrame* m_frame;

    bool m_isPostScript { false };
    bool m_pdfDocumentWasMutated { false };
    bool m_usingContinuousMode { true };

    RetainPtr<PDFLayerController> m_pdfLayerController;
    RetainPtr<WKPDFPluginAccessibilityObject> m_accessibilityObject;

    RefPtr<PDFPluginAnnotation> m_activeAnnotation;
    RefPtr<PDFPluginPasswordField> m_passwordField;
    RefPtr<WebCore::Element> m_passwordContainer;

    WebCore::AffineTransform m_rootViewToPluginTransform;
    WebCore::AffineTransform m_pluginToRootViewTransform;
    WebMouseEvent m_lastMouseEvent;
    WebCore::IntPoint m_lastMousePositionInPluginCoordinates;

    String m_temporaryPDFUUID;

    String m_lastFoundString;

    HitTestResult m_lastHitTestResult;
    
    RetainPtr<WKPDFLayerControllerDelegate> m_pdfLayerControllerDelegate;

    WebCore::IntSize m_size;

    WebCore::URL m_sourceURL;

    String m_suggestedFilename;
    RetainPtr<CFMutableDataRef> m_data;

    RetainPtr<PDFDocument> m_pdfDocument;
    WebCore::IntSize m_pdfDocumentSize; // All pages, including gaps.

    HUD m_HUD;
};

} // namespace WebKit

SPECIALIZE_TYPE_TRAITS_PLUGIN(PDFPlugin, PDFPluginType)

#endif // USE(DEPRECATED_PDF_PLUGIN)
#endif // ENABLE(PDFKIT_PLUGIN)

#endif // PDFPlugin_h
