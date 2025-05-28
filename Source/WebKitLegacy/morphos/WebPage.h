#pragma once
#include <wtf/RefPtr.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <WebCore/PageIdentifier.h>
#include <WebCore/Color.h>
#include <WebCore/GraphicsTypes.h>
#include <WebCore/FindOptions.h>
#include <WebCore/LengthBox.h>
#include <WebCore/SelectionData.h>
#include <WebCore/DragImage.h>
#include "WebViewDelegate.h"
#include "WebFrame.h"
#include <intuition/classusr.h>

namespace WebCore {
	class Page;
	class Frame;
	class FrameView;
	class IntRect;
	class KeyboardEvent;
	class ResourceError;
	class ContextMenuItem;
	class CertificateInfo;
	class AutofillElements;
	class HTMLInputElement;
	class HistoryItem;
	class PrintContext;
	class FullscreenManager;
	class DragItem;
	class DataTransfer;
};

struct RastPort;
struct IntuiMessage;

namespace WebKit {

class WebPage;
class WebPageGroup;
class WebFrame;
class WebViewDrawContext;
class WebViewPrintingContext;
class WebChromeClient;
class WebPageCreationParameters;
class WebDocumentLoader;
class BackForwardClientMorphOS;

WebCore::Page* core(WebPage *webView);
WebPage *kit(WebCore::Page* page);

WebCore::Frame& mainframe(WebCore::Page& page);
const WebCore::Frame& mainframe(const WebCore::Page& page);

class WebPage : public WebViewDelegate, public WTF::RefCounted<WebPage>
{
friend class WebChromeClient;
public:
    static Ref<WebPage> create(WebCore::PageIdentifier, WebPageCreationParameters&&);

    virtual ~WebPage();

	WebCore::Page *corePage();
	const WebCore::Page *corePage() const;
	static WebPage *fromCorePage(WebCore::Page *corePage);

    WebCore::PageIdentifier pageID() const { return m_pageID; }
    PAL::SessionID sessionID() const;

	void load(const char *url, bool ignoreCaches = false);
	void loadData(const char *data, size_t length, const char *url);
	bool reload(const char *url);
	void stop();
	
	WebCore::CertificateInfo getCertificate(void);
	
	void run(const char *js);
	void *evaluate(const char *js, WTF::Function<void *(const char *)>&& response);
	
	void *getInnerHTML(WTF::Function<void *(const char *)>&& cb);
	void setInnerHTML(const char *html);

	bool goBack();
	bool goForward();
	bool canGoBack();
	bool canGoForward();
	void goToItem(WebCore::HistoryItem& item);

	void willBeDisposed();

	bool javaScriptEnabled() const;
	void setJavaScriptEnabled(bool enabled);
	
	bool adBlockingEnabled() const;
	void setAdBlockingEnabled(bool enabled);
	
	bool thirdPartyCookiesAllowed() const;
	void setThirdPartyCookiesAllowed(bool blocked);

	bool darkModeEnabled() const;
	void setDarkModeEnabled(bool enabled);

	bool touchEventsEnabled() const;
	void setTouchEventsEnabled(bool enabled);

	void setVisibleSize(const int width, const int height);
	void setScroll(const int x, const int y);
	int scrollLeft();
	int scrollTop();
	void draw(struct RastPort *rp, const int x, const int y, const int width, const int height, bool updateMode);
	bool handleIntuiMessage(IntuiMessage *imsg, const int mouseX, const int mouseY, bool mouseInside, bool isDefaultHandler);
	bool checkDownloadable(IntuiMessage *imsg, const int mouseX, const int mouseY, WTF::URL &outURL);
	bool handleMUIKey(int muikey, bool isDefaultHandler);

	// printableWidth/Height and margins are in points/pixels (not inches)
	void printPreview(struct RastPort *rp,
		const int x, const int y, const int width, const int height, LONG sheet, LONG pagesPerSheet,
		float printableWidth, float printableHeight, bool landscape,
		const WebCore::FloatBoxExtent& margins, WebCore::PrintContext *context, bool printBackgrounds);
	void printStart(float printableWidth, float printableHeight, bool landscape, LONG pagesPerSheet,
		WebCore::FloatBoxExtent margins, WebCore::PrintContext *context, int psLevel, bool printBackgrounds, const char *file);
	void pdfStart(float printableWidth, float printableHeight, bool landscape, LONG pagesPerSheet, WebCore::FloatBoxExtent margins,
		WebCore::PrintContext *context, bool printBackgrounds, const char *file);

	bool printSpool(WebCore::PrintContext *context, int pageNo);
	void printingFinished(void);

	void onContextMenuItemSelected(ULONG action, const char *title);

    void addResourceRequest(unsigned long, const WebCore::ResourceRequest&);
    void removeResourceRequest(unsigned long);

    void didStartPageTransition();
    void didCompletePageTransition();
    void didCommitLoad(WebFrame& frame);
	void didFinishDocumentLoad(WebFrame& frame);
	void didFinishLoad(WebFrame& frame);
	void didFailLoad(const WebCore::ResourceError& error);

    Ref<WebCore::DocumentLoader> createDocumentLoader(WebCore::Frame&, const WebCore::ResourceRequest&, const WebCore::SubstituteData&);
    void updateCachedDocumentLoader(WebDocumentLoader&, WebCore::Frame&);

    void scalePage(double scale, const WebCore::IntPoint& origin);
    double pageScaleFactor() const;
    double totalScaleFactor() const;
    double viewScaleFactor() const;

    float pageZoomFactor() const;
    float textZoomFactor() const;
    void setPageAndTextZoomFactors(float pageZoomFactor, float textZoomFactor);

    bool mainFrameIsScrollable() const { return m_mainFrameIsScrollable; }

    void setAlwaysShowsHorizontalScroller(bool);
    void setAlwaysShowsVerticalScroller(bool);

    bool alwaysShowsHorizontalScroller() const { return m_alwaysShowsHorizontalScroller; };
    bool alwaysShowsVerticalScroller() const { return m_alwaysShowsVerticalScroller; };

	bool handleEditingKeyboardEvent(WebCore::KeyboardEvent& event);

    const std::optional<WebCore::Color>& backgroundColor() const { return m_backgroundColor; }
	
    void setInterpolationQuality(WebCore::InterpolationQuality quality) { m_interpolation = quality; }
    WebCore::InterpolationQuality interpolationQuality() const { return m_interpolation; }

    void setInterpolationQualityForImageViews(WebCore::InterpolationQuality quality) { m_imageInterpolation = quality; }
    WebCore::InterpolationQuality interpolationQualityForImageViews() const { return m_imageInterpolation; }

	void setRequiresUserGestureForMediaPlayback(bool requiresGesture);
	bool requiresUserGestureForMediaPlayback();
	
	void setInvisiblePlaybackNotAllowed(bool invisible);
	bool invisiblePlaybackNotAllowed();

    WebCore::IntSize size() const;
    WebCore::IntRect bounds() const { return WebCore::IntRect(WebCore::IntPoint(), size()); }

    WebFrame& topLevelFrame() const { return m_mainFrame; }

    WebCore::Frame* mainFrame() const; // May return nullptr.
    WebCore::FrameView* mainFrameView() const; // May return nullptr.

	WTF::RefPtr<WebKit::BackForwardClientMorphOS> backForwardClient();

    void goActive();
    void goInactive();
    void goVisible();
    bool isVisible() const { return m_isVisible; }
    void goHidden();
	
	void setLowPowerMode(bool lowPowerMode);

	bool localStorageEnabled();
	void setLocalStorageEnabled(bool enabled);
	
	bool offlineCacheEnabled();
	void setOfflineCacheEnabled(bool enabled);

	void startLiveResize();
	void endLiveResize();
	
    void setFocusedElement(WebCore::Element *);
    WebCore::IntRect getElementBounds(WebCore::Element *);
	void setFullscreenElement(WebCore::Element *);
    WebCore::FullscreenManager* fullscreenManager();
    bool isFullscreen() const;
    void exitFullscreen();

	void startedEditingElement(WebCore::HTMLInputElement *);
	bool hasAutofillElements();
	void clearAutofillElements();
	void setAutofillElements(const WTF::String &login, const WTF::String &password);
	bool getAutofillElements(WTF::String &outlogin, WTF::String &outPassword);

	void setCursor(int);

	bool drawRect(const int x, const int y, const int width, const int height, struct RastPort *rp);
	void invalidate();

	bool search(const WTF::String &string, WebCore::FindOptions &options, bool& outWrapped);
	
	void loadUserStyleSheet(const WTF::String &path);
	
	bool allowsScrolling();
	void setAllowsScrolling(bool allows);
	
	bool editable();
	void setEditable(bool editable);
	
	WTF::String primaryLanguage();
	WTF::String additionalLanguage();
	void setSpellingLanguages(const WTF::String &language, const WTF::String &additional);

	enum class ContextMenuHandling // keep in sync with WkSettings!!
	{
		Default,
		Override,
		OverrideWithShift,
		OverrideWithAlt,
		OverrideWithControl,
	};
	
	void setContextMenuHandling(ContextMenuHandling handling) { m_cmHandling = handling; }
	ContextMenuHandling contextMenuHandling() const { return m_cmHandling; }

	// WkHitTest support...
	WebCore::Frame *fromHitTest(WebCore::HitTestResult &hitTest) const;
	bool hitTestImageToClipboard(WebCore::HitTestResult &hitTest) const;
	bool hitTestSaveImageToFile(WebCore::HitTestResult &hitTest, const WTF::String &path) const;
	void hitTestReplaceSelectedTextWidth(WebCore::HitTestResult &hitTest, const WTF::String &text) const;
	void hitTestCopySelectedText(WebCore::HitTestResult &hitTest) const;
	void hitTestCutSelectedText(WebCore::HitTestResult &hitTest) const;
	void hitTestPaste(WebCore::HitTestResult &hitTest) const;
	void hitTestSelectAll(WebCore::HitTestResult &hitTest) const;

	enum class ContextMenuImageFloat
	{
		None,
		Left,
		Right
	};

	void hitTestSetImageFloat(WebCore::HitTestResult &hitTest, ContextMenuImageFloat imageFloat);
	ContextMenuImageFloat hitTestImageFloat(WebCore::HitTestResult &hitTest) const;

	void startDownload(const WTF::URL &url);
	void flushCompositing();

	WTF::String misspelledWord(WebCore::HitTestResult &hitTest);
	WTF::Vector<WTF::String> misspelledWordSuggestions(WebCore::HitTestResult &hitTest);
	void markWord(WebCore::HitTestResult &hitTest);
	void learnMisspelled(WebCore::HitTestResult &hitTest);
	void ignoreMisspelled(WebCore::HitTestResult &hitTest);
	void replaceMisspelled(WebCore::HitTestResult &hitTest, const WTF::String &replacement);
	bool canUndo();
	bool canRedo();
	void undo();
	void redo();

	void startDrag(WebCore::DragItem&&, WebCore::DataTransfer&, WebCore::Frame&);
	bool isDragging(void) const { return m_dragging; };
	void drawDragImage(struct RastPort *rp, const int x, const int y, const int width, const int height);

protected:
	WebPage(WebCore::PageIdentifier, WebPageCreationParameters&&);

       enum class WebPageScrollByMode
       {
               Pixels,
               Units,
               Pages
       };

	// WebChrome methods
    void repaint(const WebCore::IntRect&);
    void internalScroll(int scrollX, int scrollY);
    void scrollBy(int xDelta, int yDelta, WebPageScrollByMode mode, WebCore::Frame *inFrame = nullptr);
    void wheelScrollOrZoomBy(const int xDelta, const int yDelta, ULONG qualifiers, WebCore::Frame *inFrame = nullptr);
    void frameSizeChanged(WebCore::Frame& frame, int width, int height);

    void closeWindow();
    void closeWindowSoon();
    void closeWindowTimerFired();

    bool transparent() const { return m_transparent; }
    bool usesLayeredWindow() const { return m_usesLayeredWindow; }

    int mouseCursorToSet(ULONG qualifiers, bool mouseInside);
    
    void endDragging(int mouseX, int mouseY, int mouseGlobalX, int mouseGlobalY, bool doDrop);

private:
	Ref<WebFrame> m_mainFrame;
	WebCore::Page *m_page { nullptr };
	RefPtr<WebPageGroup> m_webPageGroup;
	WebViewDrawContext  *m_drawContext { nullptr };
    WebViewPrintingContext *m_printingContext { nullptr };
    WebCore::PageIdentifier m_pageID;
    WebCore::AutofillElements *m_autofillElements { nullptr };
    WebCore::InterpolationQuality m_interpolation = WebCore::InterpolationQuality::Default;
    WebCore::InterpolationQuality m_imageInterpolation = WebCore::InterpolationQuality::Default;
    WTF::HashSet<unsigned long> m_trackedNetworkResourceRequestIdentifiers;
    uint64_t m_pendingNavigationID { 0 };
	uint32_t m_lastQualifier { 0 };
	int  m_clickCount { 0 };
	int  m_cursor { 0 };
	int  m_cursorLock { 0 };
	int  m_middleClick[2];
	int  m_mouseLastX, m_mouseLastY;
	bool m_transparent { false };
	bool m_usesLayeredWindow { false };
    bool m_mainFrameProgressCompleted { false };
    bool m_alwaysShowsHorizontalScroller { false };
    bool m_alwaysShowsVerticalScroller { false };
    bool m_mainFrameIsScrollable { true };
    bool m_trackMouse { false };
    bool m_trackMiddle { false };
    bool m_trackMiddleDidScroll { false };
    bool m_ignoreScroll { false };
    bool m_orphaned { false };
    bool m_adBlocking { true };
    bool m_justWentActive { false };
    bool m_isActive { false };
    bool m_isVisible { false };
    bool m_needsCompositingFlush { false };
    bool m_transitioning { false };
    bool m_cursorOverLink { false };
    bool m_darkMode { false };
    bool m_dragging { false };
    bool m_dragInside { false };
    RefPtr<WebCore::Element> m_focusedElement;
    RefPtr<WebCore::Element> m_fullscreenElement;
    ContextMenuHandling m_cmHandling { ContextMenuHandling::Default };
    std::optional<WebCore::Color> m_backgroundColor { WebCore::Color::white };
    WTF::URL m_hoveredURL;
    WebCore::SelectionData m_dragData;
    WebCore::DragImage m_dragImage;
};

}
