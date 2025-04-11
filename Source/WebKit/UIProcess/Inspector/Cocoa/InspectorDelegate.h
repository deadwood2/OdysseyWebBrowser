/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#import "APIInspectorClient.h"
#import "WKFoundation.h"
#import <wtf/WeakObjCPtr.h>

@class WKWebView;

@protocol _WKInspectorDelegate;

namespace WebKit {

class WebInspectorProxy;
class WebPageProxy;

class InspectorDelegate {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit InspectorDelegate(WKWebView *);

    std::unique_ptr<API::InspectorClient> createInspectorClient();

    RetainPtr<id <_WKInspectorDelegate>> delegate();
    void setDelegate(id <_WKInspectorDelegate>);

private:
    class InspectorClient : public API::InspectorClient {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        explicit InspectorClient(InspectorDelegate&);
        ~InspectorClient();

    private:
        // API::InspectorClient
        void didAttachLocalInspector(WebPageProxy&, WebInspectorProxy&) override;
        void browserDomainEnabled(WebPageProxy&, WebInspectorProxy&) override;
        void browserDomainDisabled(WebPageProxy&, WebInspectorProxy&) override;

        InspectorDelegate& m_inspectorDelegate;
    };

    WeakObjCPtr<WKWebView> m_webView;
    WeakObjCPtr<id <_WKInspectorDelegate>> m_delegate;

    struct {
        unsigned webviewDidAttachInspector : 1;
        unsigned webViewBrowserDomainEnabledForInspector : 1;
        unsigned webViewBrowserDomainDisabledForInspector : 1;
    } m_delegateMethods;
};

} // namespace WebKit
