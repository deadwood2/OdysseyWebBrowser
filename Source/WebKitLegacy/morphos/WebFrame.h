/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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

#pragma once

#include "WebFrameLoaderClient.h"
#include <JavaScriptCore/ConsoleTypes.h>
#include <JavaScriptCore/JSBase.h>
#include <WebCore/FrameLoaderClient.h>
#include <WebCore/FrameLoaderTypes.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

namespace PAL {
class SessionID;
}

namespace WebCore {
class CertificateInfo;
class Frame;
class HTMLFrameOwnerElement;
class IntPoint;
class IntRect;
}

namespace WebKit {

class WebPage;
struct FrameInfoData;
struct WebsitePoliciesData;

class WebFrame : public WTF::RefCounted<WebFrame> {
public:
	static Ref<WebFrame> create() { return adoptRef(*new WebFrame); }
    static Ref<WebFrame> createWithCoreMainFrame(WebPage*, WebCore::Frame*);
    static Ref<WebFrame> createSubframe(WebPage*, const String& frameName, WebCore::HTMLFrameOwnerElement*);
    ~WebFrame();

	void initWithCoreMainFrame(WebPage&, WebCore::Frame&);

    // Called when the FrameLoaderClient (and therefore the WebCore::Frame) is being torn down.
    void invalidate();

    WebPage* page() const;

    static WebFrame* fromCoreFrame(const WebCore::Frame&);
    WebCore::Frame* coreFrame() const { return m_coreFrame; }

    FrameInfoData info() const;
    WebCore::FrameIdentifier frameID() const { return m_frameID; }

#if 0
    enum class ForNavigationAction { No, Yes };
    uint64_t setUpPolicyListener(WebCore::PolicyCheckIdentifier, WebCore::FramePolicyFunction&&, ForNavigationAction);
    void invalidatePolicyListener();
    void didReceivePolicyDecision(uint64_t listenerID, WebCore::PolicyCheckIdentifier, WebCore::PolicyAction, uint64_t navigationID, DownloadID, std::optional<WebsitePoliciesData>&&);

    uint64_t setUpWillSubmitFormListener(CompletionHandler<void()>&&);
    void continueWillSubmitForm(uint64_t);
#endif

    void startDownload(const WebCore::ResourceRequest&, const String& suggestedName = { });
    void startDownload(const WTF::URL &, const String& suggestedName = { });
    void convertMainResourceLoadToDownload(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);

    void addConsoleMessage(MessageSource, MessageLevel, const String&, uint64_t requestID = 0);

    String source() const;
    String contentsAsString() const;
    String selectionAsString() const;

    WebCore::IntSize size() const;

    // WKBundleFrame API and SPI functions
    bool isMainFrame() const;
    String name() const;
    URL url() const;
    WebCore::CertificateInfo certificateInfo() const;
    String innerText() const;
    bool isFrameSet() const;
    WebFrame* parentFrame() const;
//    Ref<API::Array> childFrames();
    JSGlobalContextRef jsContext();
//    JSGlobalContextRef jsContextForWorld(InjectedBundleScriptWorld*);
    WebCore::IntRect contentBounds() const;
    WebCore::IntRect visibleContentBounds() const;
    WebCore::IntRect visibleContentBoundsExcludingScrollbars() const;
    WebCore::IntSize scrollOffset() const;
    bool hasHorizontalScrollbar() const;
    bool hasVerticalScrollbar() const;
    bool getDocumentBackgroundColor(double* red, double* green, double* blue, double* alpha);
    bool containsAnyFormElements() const;
    bool containsAnyFormControls() const;
    void stopLoading();
    bool handlesPageScaleGesture() const;
    bool requiresUnifiedScaleFactor() const;
    void setAccessibleName(const String&);

    static WebFrame* frameForContext(JSContextRef);

#if 0
    JSValueRef jsWrapperForWorld(InjectedBundleNodeHandle*, InjectedBundleScriptWorld*);
    JSValueRef jsWrapperForWorld(InjectedBundleRangeHandle*, InjectedBundleScriptWorld*);
#endif

    static String counterValue(JSObjectRef element);

    unsigned pendingUnloadCount() const;
    
    bool allowsFollowingLink(const URL&) const;

    String provisionalURL() const;
    String suggestedFilenameForResourceWithURL(const URL&) const;
    String mimeTypeForResourceWithURL(const URL&) const;

    void setTextDirection(const String&);

    void documentLoaderDetached(uint64_t navigationID);

    WebFrameLoaderClient* frameLoaderClient() const;

private:
    WebFrame();

    WebCore::Frame* m_coreFrame { nullptr };

    uint64_t m_policyListenerID { 0 };
    std::optional<WebCore::PolicyCheckIdentifier> m_policyIdentifier;
    WebCore::FramePolicyFunction m_policyFunction;
//    ForNavigationAction m_policyFunctionForNavigationAction { ForNavigationAction::No };
    HashMap<uint64_t, CompletionHandler<void()>> m_willSubmitFormCompletionHandlers;
//    DownloadID m_policyDownloadID { 0 };

//    LoadListener* m_loadListener { nullptr };
    
    WebCore::FrameIdentifier m_frameID;
};

} // namespace WebKit
