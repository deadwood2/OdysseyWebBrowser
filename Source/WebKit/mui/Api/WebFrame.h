/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebFrame_H
#define WebFrame_H


/**
 *  @file  WebFrame.h
 *  WebFrame description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */
 
#include <wtf/RefPtr.h>
 
#include "WebKitTypes.h"
#include "WebValue.h"
#include <vector>

namespace WebCore {
    class AuthenticationChallenge;
    class BALObject;
    class DocumentLoader;
    class DOMWrapperWorld;
    class Frame;
    class HTMLFrameOwnerElement;
    class IntRect;
    class Page;
    class ResourceError;
    class SharedBuffer;
}

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSValue* JSObjectRef;
typedef struct OpaqueJSContext* JSGlobalContextRef;

class DOMDocument;
class DOMElement;
class DOMNode;
class WebArchive;
class WebDataSource;
class WebFrame;
class WebFrameLoaderClient;
class WebFrameObserver;
class WebFramePolicyListener;
class WebHistory;
class WebMutableURLRequest;
class WebObject;
class WebScriptWorld;
class WebView;
#if ENABLE(JS_ADDONS)
class BindingJS;
#endif

typedef enum {
    WebFrameLoadTypeStandard,
    WebFrameLoadTypeBack,
    WebFrameLoadTypeForward,
    WebFrameLoadTypeIndexedBackForward, // a multi-item hop in the backforward list
    WebFrameLoadTypeReload,
    WebFrameLoadTypeReloadAllowingStaleData,
    WebFrameLoadTypeSame,               // user loads same URL again (but not reload button)
    WebFrameLoadTypeInternal,           // maps to WebCore::FrameLoadTypeRedirectWithLockedHistory
    WebFrameLoadTypeReplace
} WebFrameLoadType;

#define WebKitErrorPlugInWillHandleLoad 204

    /**
     * get WebFrame from a frame
     * @param[in]: frame
     * @param[out]: WebFrame
     */
WebFrame* kit(WebCore::Frame*);

    /**
     * get frame from a webFrame
     * @param[in]: WebFrame
     * @param[out]: frame
     */
WEBKIT_OWB_API WebCore::Frame* core(WebFrame*);

class WEBKIT_OWB_API WebFrame
{
public:

    /**
     * create a new instance of WebFrame
     * @param[out]: WebFrame
     */
    static WebFrame* createInstance();

    /**
     * WebFrame destructor
     */
    virtual ~WebFrame();
protected:

    /**
     * WebFrame default constructor
     */
    WebFrame();

public:

    /**
     * @result The frame name.
       @discussion *** The string returned by this method is duplicated with strdup so it is up to the caller to free it ***
     */
    virtual const char* name();

    /**
     * setName
     * @description set the WebFrame's name
     * @param frameName the new WebFrame's name
     * @discussion The frame needs to have been initialize throught the WebView
     *             for this method to work.
     */
    virtual void setName(const char* frameName);

    /**
     * @result Returns the WebView for the document that includes this frame.
     */
    virtual WebView* webView();

    /**
     * Returns the DOM document of the frame.
       @description Returns nil if the frame does not contain a DOM document such as a standalone image. DOMDocument description
     */
    virtual DOMDocument* domDocument();

    /**
     *  loadRequest
     * @param request The web request to load.
     */
    virtual void loadRequest(WebMutableURLRequest* request);

    /**
     *  loadURL
     * @param request The url to load.
     */
    virtual void loadURL(const char* url);

    /**
     *  loadHTMLString
     * @param string The string to use for the main page of the document.
        @param URL The base URL to apply to relative URLs within the document.
     */
    virtual void loadHTMLString(const char*, const char* baseURL);

    /**
     *  loadAlternateHTMLString
     * Loads a page to display as a substitute for a URL that could not be reached.
        @discussion This allows clients to display page-loading errors in the webview itself.
        This is typically called while processing the WebFrameLoadDelegate method
        -webView:didFailProvisionalLoadWithError:forFrame: or one of the the DefaultPolicyDelegate methods
        -webView:decidePolicyForMIMEType:request:frame:decisionListener: or
        -webView:unableToImplementPolicyWithError:frame:. If it is called from within one of those
        three delegate methods then the back/forward list will be maintained appropriately.
        @param string The string to use for the main page of the document.
        @param baseURL The baseURL to apply to relative URLs within the document.
        @param unreachableURL The URL for which this page will serve as alternate content.
     */
    virtual void loadAlternateHTMLString(const char* str, const char* baseURL, const char* unreachableURL);

    /**
     *  loadArchive
     * Causes WebFrame to load a WebArchive.
        @param archive The archive to be loaded.
     */
    virtual void loadArchive(WebArchive *archive);

    /**
     *  dataSource
     * Returns the committed data source.  Will return nil if the
        provisional data source hasn't yet been loaded.
        @result The datasource for this frame.
     */
    virtual WebDataSource *dataSource();

    /**
     *  provisionalDataSource
     * @discussion Will return the provisional data source.  The provisional data source will
        be nil if no data source has been set on the frame, or the data source
        has successfully transitioned to the committed data source.
        @result The provisional datasource of this frame.
     */
    virtual WebDataSource *provisionalDataSource();

    /**
     *  stopLoading
     * @discussion Stop any pending loads on the frame's data source,
        and its children.
     */
    virtual void stopLoading();

    /**
     *  reload
     */
    virtual void reload();

    /**
     *  findFrameNamed
     * @discussion This method returns a frame with the given name. findFrameNamed returns self
        for _self and _current, the parent frame for _parent and the main frame for _top.
        findFrameNamed returns self for _parent and _top if the receiver is the mainFrame.
        findFrameNamed first searches from the current frame to all descending frames then the
        rest of the frames in the WebView. If still not found, findFrameNamed searches the
        frames of the other WebViews.
        @param name The name of the frame to find.
        @result The frame matching the provided name. nil if the frame is not found.
     */
    virtual WebFrame* findFrameNamed(const char* name);

    /**
     *  parentFrame
     * @result The frame containing this frame, or nil if this is a top level frame.
     */
    virtual WebFrame* parentFrame();

    /**
     * children
     * @result list of children contains by this frame, this content will be cleaned by owb
     */
    virtual std::vector<WebFrame*>* children();

    /**
     * get current form element
     */
    virtual DOMElement* currentForm();


    /**
     * get renderTree as external representation
     * @param[out]: external representation
     */
    virtual const char* renderTreeAsExternalRepresentation();

    /**
     * counterValueForElementById
     * @brief get the value associated with an element whose id is given.
     * @param id, the id of the element whose value we want
     * @return the counter value or 0 if the element was not found.
     */
    virtual char* counterValueForElementById(const char* /*id*/);

    /**
     * get the page number for element by id
     * @param id The name of element.
     * @param pageWidthInPixels The width of page in pixels.
     * @param pageHeightInPixels The height of page in pixels.
     * @param[out]: page number
     */
    virtual int pageNumberForElementById(const char* id, float pageWidthInPixels, float pageHeightInPixels);

    /**
     * get the number of pages
     * @param pageWidthInPixels The width of page in pixels.
     * @param pageHeightInPixels The height of page in pixels.
     * @param[out]: number of pages
     */
    virtual int numberOfPages(float pageWidthInPixels, float pageHeightInPixels);

    /**
     * get the scroll offset
     * @param[out]: scroll offset
     */
    virtual BalPoint scrollOffset();

    /**
     * layout
     */
    virtual void layout();

    /**
     * get if the first layout is done
     */
    virtual bool firstLayoutDone();

    /**
     * get load type
     * @param[out]: WebFrame load type
     */
    virtual WebFrameLoadType loadType();

    /**
     * setInPrintingMode
     * Not Implemented
     */
    //virtual void setInPrintingMode(bool value, HDC printDC);

    /**
     * getPrintedPageCount
     * Not Implemented
     */
    //virtual unsigned int getPrintedPageCount(HDC printDC);

    /**
     * spoolPages
     * Not Implemented
     */
    //virtual void* spoolPages(HDC printDC, UINT startPage, UINT endPage);


    /**
     * test if the frame is frame set
     */
    virtual bool isFrameSet();

    /**
     * get string representation
     */
    virtual const char* toString();

    /**
     * get tree dump
     */
    virtual const char* renderTreeDump() const;

    /**
     * get size
     */
    virtual BalPoint size();

    /**
     * test if the frame has scrollBars
     */
    virtual bool hasScrollBars();

    /**
     * get content bounds
     */
    virtual BalRectangle contentBounds();

    /**
     * get frame bounds
     */
    virtual BalRectangle frameBounds();


    /**
     * test if the frame is descendant of frame
     */
    virtual bool isDescendantOfFrame(WebFrame *ancestor);

    /**
     * set allows scrolling
     */
    virtual void setAllowsScrolling(bool flag);

    /**
     * get allows scrolling
     */
    virtual bool allowsScrolling();

    /*!
      @method setIsDisconnected
      @abstract Set whether a frame is disconnected
      @param flag true to mark the frame as disconnected, false keeps it a regular frame
    */
    virtual void setIsDisconnected(bool flag);

    /**
     * setExcludeFromTextSearch
     */
    virtual void setExcludeFromTextSearch(bool flag);

    /**
     * reloadFromOrigin
     */
    virtual void reloadFromOrigin();

    /**
     * contextForWorldID
     * @abstract returns a reference to the global JavaScript global object associated with the world ID.
     * @param WebScriptWorld, the isolated world.
     */
    virtual JSGlobalContextRef contextForScriptWorld(WebScriptWorld*);

    /**
     * get visible content rect
     */
    virtual BalRectangle visibleContentRect();


    virtual const char* layerTreeAsText();
    /**
     * test if the frame supports text encoding
     */
    virtual bool supportsTextEncoding();

    /**
     * get selected string
     */
    virtual const char* selectedString();

    /**
     * selectAll
     * @brief select all the document
     * @return true if the command was executed and worked, false otherwise
     */
    virtual bool selectAll();

    /**
     * deselectAll
     * @brief remove the current selection
     * @return true if the command was executed and worked, false otherwise
     */
    virtual bool deselectAll();

    /**
     *  shouldTreatURLAsSameAsCurrent
     * Not Implemented
     */
    virtual bool shouldTreatURLAsSameAsCurrent(const char*) const;

    /**
     *  addHistoryItemForFragmentScroll
     * Not Implemented
     */
    virtual void addHistoryItemForFragmentScroll();


    // WebFrame


    /**
     * invalidate webframe
     */
    void invalidate();

    /**
     * unmark all misspellings
     */
    void unmarkAllMisspellings();

    /**
     * unmark all bad grammar
     */
    void unmarkAllBadGrammar();

    void updateBackground();

    // WebFrame (matching WebCoreFrameBridge)

    /**
     *  elementWithName
     * Not Implemented
     */
    //HRESULT elementWithName(BSTR name, IDOMElement* form, IDOMElement** element);

    /**
     *  formForElement
     * Not Implemented
     */
    //HRESULT formForElement(IDOMElement* element, IDOMElement** form);

    /**
     *  elementDoesAutoComplete
     * Not Implemented
     */
    //HRESULT elementDoesAutoComplete(IDOMElement* element, bool* result);

    /**
     *  controlsInForm
     * Not Implemented
     */
    //HRESULT controlsInForm(IDOMElement* form, IDOMElement** controls, int* cControls);

    /**
     *  elementIsPassword
     * Not Implemented
     */
    //HRESULT elementIsPassword(IDOMElement* element, bool* result);

    /**
     *  searchForLabelsBeforeElement
     * Not Implemented
     */
    //HRESULT searchForLabelsBeforeElement(const BSTR* labels, unsigned cLabels, IDOMElement* beforeElement, unsigned* resultDistance, BOOL* resultIsInCellAbove, BSTR* result);

    /**
     *  matchLabelsAgainstElement
     * Not Implemented
     */
    //HRESULT matchLabelsAgainstElement(const BSTR* labels, int cLabels, IDOMElement* againstElement, BSTR* result);

    /**
     *  canProvideDocumentSource
     * Not Implemented
     */
    //HRESULT canProvideDocumentSource(bool* result);


    /**
     * Get the frame url
     * @discussion: the returned string should be freed by the caller or it will leaks.
     */
     const char* url() const;

    /**
     * return  the number of active animations
     */
    int numberOfActiveAnimations();

    /**
     * return true if the document only contains a standalone image to display
     */
    bool isDisplayingStandaloneImage();

    /**
     * Returns true if the WebFrame can load the given URL.
     */
    bool allowsFollowingLink(const char* url) const;

    /*
     * elementDoesAutoComplete
     */
    bool elementDoesAutoComplete(DOMElement *element);

    /**
     * pause Animation
     */
    bool pauseAnimation(const char* name, double time, const char* element);

    /**
     * pause Transition
     */
    bool pauseTransition(const char* name, double time, const char* element);

    /**
     * set frame editable
     */
    void setEditable(bool flag);

    /**
     * get pending frame unload event count
     */
    unsigned pendingFrameUnloadEventCount();

    /**
     * get webview
     */
    WebView* webView() const;

	/**
	 * set webview
	 */	   
	void setWebView(WebView*);

#if ENABLE(JS_ADDONS)
    BindingJS *bindingJS() {return m_bindingJS;}
#endif
    /**
     * add to JSWindow object
     * add an balObject to extend the javascript
     */
    void addToJSWindowObject(WebObject* object);

    /**
     * stringByEvaluatingJavaScriptInScriptWorld
     * Allows to evaluate a given JS script within the specified window shell.
     * @param[in]: Pointer to a WebScriptWorld object
     * @param[in]: JS global object
     * @param[in]: Script to evaluate
     * @param[in/out]: Result of the evaluation. Caller owns the string upon return.
     * @result: true if the evaluation succeeded, false otherwise.
     */
    virtual bool stringByEvaluatingJavaScriptInScriptWorld(WebScriptWorld*, void* jsGlobalObject, const char* script, const char** evaluationResult);

    //BCObserverAddons
    /**
     * observe implementation
     */
    //virtual void observe(const const char* &topic, BalObject* obj);
    //virtual void observe(const const char* &topic, OWBAL::Bookmarklet* bookmarklet);
 
#if 0   
    TransferSharedPtr<WebValue> getWrappedAttributeEventListener(const char* name);
    void addEventListener(const char* name, TransferSharedPtr<WebValue> value);
    void removeEventListener(const char* name, TransferSharedPtr<WebValue> value);
#endif
    void dispatchEvent(const char* name);
    
    void didChangeIcons(WebCore::DocumentLoader*);

protected:
     friend WebCore::Frame* core(WebFrame*);
     friend WebCore::Frame* core(const WebFrame*);
     friend class WebView;
     friend class WebFrameLoaderClient;

    /**
     *  globalContext
     * @result The frame's global JavaScript execution context.  Use this method to
        bridge between the WebKit and JavaScriptCore APIs.
     */
    virtual JSGlobalContextRef globalContext();

	
	/**
     *  init WebFrame
     */
	WTF::PassRefPtr<WebCore::Frame> createSubframeWithOwnerElement(WebView*, WebCore::Page*, WebCore::HTMLFrameOwnerElement*); 
 	void initWithWebView(WebView*, WebCore::Page*); 

    /**
     * get frame
     */
    WebCore::Frame* impl();

    /**
     *  loadHTMLString
     */
    void loadHTMLString(const char* string, const char* baseURL, const char* unreachableURL);

    /**
     *  computePageRects
     */
    //const Vector<WebCore::IntRect>& computePageRects(HDC printDC);

    /**
     *  setPrinting
     */
    //void setPrinting(bool printing, float minPageWidth, float minPageHeight, float maximumShrinkRatio, bool adjustViewSize); 

    /**
     *  headerAndFooterHeights
     */
    //void headerAndFooterHeights(float*, float*);

    /**
     *  printerMarginRect
     */
    //WebCore::IntRect printerMarginRect(HDC);

    /**
     *  printerMarginRect
     */
    //void spoolPage (PlatformGraphicsContext* pctx, WebCore::GraphicsContext* spoolCtx, HDC printDC, IWebUIDelegate*, float headerHeight, float footerHeight, unsigned page, unsigned pageCount);
    
    /**
     *  printerMarginRect
     */
    //void drawHeader(PlatformGraphicsContext* pctx, IWebUIDelegate*, const WebCore::IntRect& pageRect, float headerHeight);
    
    /**
     *  printerMarginRect
     */
    //void drawFooter(PlatformGraphicsContext* pctx, IWebUIDelegate*, const WebCore::IntRect& pageRect, unsigned page, unsigned pageCount, float headerHeight, float footerHeight);


private:
    /**
     * addToJSWindowObject
     * @brief Add the BALObject to the JavaScript global object associated with this WebFrame.
     * @internal
     */
    void addToJSWindowObject(WebCore::BALObject* object);

protected:
#if ENABLE(DAE)
    /**
     * addVKToJSWindowObject
     * @brief Add the VK values (see CE-HTML for those) to the JavaScript global object.
     * @warning This method is autogenerated by VKParser.cpp from VKValues.in.
     * @internal
     */
    void addVKToJSWindowObject();
#endif

    class WebFramePrivate;
    WebFramePrivate* d;
    WebFrameLoaderClient* m_loadClient;
    bool m_quickRedirectComing;
    bool m_inPrintingMode;
    int m_pageHeight;   // height of the page adjusted by margins
    WebFrameObserver* m_webFrameObserver;
    std::vector<WebFrame*> m_rc;
#if ENABLE(JS_ADDONS)
    BindingJS* m_bindingJS;
#endif
};

#endif
