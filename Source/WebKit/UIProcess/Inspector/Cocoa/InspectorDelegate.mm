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

#import "config.h"
#import "InspectorDelegate.h"

#import "WebInspectorProxy.h"
#import "WebPageProxy.h"
#import "_WKInspectorDelegate.h"
#import "_WKInspectorInternal.h"

namespace WebKit {

InspectorDelegate::InspectorDelegate(WKWebView *webView)
    : m_webView(webView)
{
}

std::unique_ptr<API::InspectorClient> InspectorDelegate::createInspectorClient()
{
    return makeUnique<InspectorClient>(*this);
}

RetainPtr<id <_WKInspectorDelegate>> InspectorDelegate::delegate()
{
    return m_delegate.get();
}

void InspectorDelegate::setDelegate(id <_WKInspectorDelegate> delegate)
{
    m_delegate = delegate;

    m_delegateMethods.webviewDidAttachInspector = [delegate respondsToSelector:@selector(_webView:didAttachLocalInspector:)];
    m_delegateMethods.webViewBrowserDomainEnabledForInspector = [delegate respondsToSelector:@selector(_webView:browserDomainEnabledForInspector:)];
    m_delegateMethods.webViewBrowserDomainDisabledForInspector = [delegate respondsToSelector:@selector(_webView:browserDomainDisabledForInspector:)];
}

InspectorDelegate::InspectorClient::InspectorClient(InspectorDelegate& delegate)
    : m_inspectorDelegate(delegate)
{
}

InspectorDelegate::InspectorClient::~InspectorClient() = default;

void InspectorDelegate::InspectorClient::didAttachLocalInspector(WebPageProxy&, WebInspectorProxy& inspector)
{
    if (!m_inspectorDelegate.m_delegateMethods.webviewDidAttachInspector)
        return;

    auto& delegate = m_inspectorDelegate.m_delegate;
    if (!delegate)
        return;

    [delegate _webView:m_inspectorDelegate.m_webView.get().get() didAttachLocalInspector:wrapper(inspector)];
}

void InspectorDelegate::InspectorClient::browserDomainEnabled(WebPageProxy&, WebInspectorProxy& inspector)
{
    if (!m_inspectorDelegate.m_delegateMethods.webViewBrowserDomainEnabledForInspector)
        return;

    auto& delegate = m_inspectorDelegate.m_delegate;
    if (!delegate)
        return;

    [delegate _webView:m_inspectorDelegate.m_webView.get().get() browserDomainEnabledForInspector:wrapper(inspector)];
}

void InspectorDelegate::InspectorClient::browserDomainDisabled(WebPageProxy&, WebInspectorProxy& inspector)
{
    if (!m_inspectorDelegate.m_delegateMethods.webViewBrowserDomainDisabledForInspector)
        return;

    auto& delegate = m_inspectorDelegate.m_delegate;
    if (!delegate)
        return;

    [delegate _webView:m_inspectorDelegate.m_webView.get().get() browserDomainDisabledForInspector:wrapper(inspector)];
}

} // namespace WebKit
