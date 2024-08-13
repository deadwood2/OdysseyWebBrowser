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

#ifndef InspectorClientBal_h
#define InspectorClientBal_h

#include <JavaScriptCore/InspectorFrontendChannel.h>
#include <WebCore/InspectorClient.h>
#include <WebCore/InspectorFrontendClientLocal.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class CertificateInfo;
class Page;
}

class WebInspectorFrontendClient;
class WebNodeHighlight;
class WebView;

class WebInspectorClient final : public WebCore::InspectorClient, public Inspector::FrontendChannel {
public:
    explicit WebInspectorClient(WebView*);

    // InspectorClient API.
    void inspectedPageDestroyed() override;

    Inspector::FrontendChannel* openLocalFrontend(WebCore::InspectorController*) override;
    void bringFrontendToFront() override;

    void highlight() override;
    void hideHighlight() override;

    // FrontendChannel API.
    ConnectionType connectionType() const override { return ConnectionType::Local; }
    void sendMessageToFrontend(const WTF::String&) override;

    bool inspectorStartsAttached();
    void setInspectorStartsAttached(bool);

    bool inspectorAttachDisabled();
    void setInspectorAttachDisabled(bool);

    void releaseFrontend();

    WebInspectorFrontendClient* frontendClient() { return m_frontendClient; }

    void updateHighlight();

private:
    WebView* m_webView;
    WebCore::Page* m_frontendPage;
    WebInspectorFrontendClient* m_frontendClient;
    WTF::HashMap<WTF::String, WTF::String> *m_settings;
    WTF::HashMap<WTF::String, WTF::String> m_sessionSettings;
};

class WebInspectorFrontendClient final : public WebCore::InspectorFrontendClientLocal {
public:
    WebInspectorFrontendClient(WebView* inspectedWebView, WebView* frontendWebView, WebCore::Page* inspectorPage, WebInspectorClient* inspectorClient, std::unique_ptr<Settings>);
    virtual ~WebInspectorFrontendClient();

    // InspectorFrontendClient API.
    void frontendLoaded() override;

    WTF::String localizedStringsURL() override;

    void bringToFront() override;
    void closeWindow() override;
    void reopen() override;

    void setAttachedWindowHeight(unsigned) override;
    void setAttachedWindowWidth(unsigned) override;

    void inspectedURLChanged(const WTF::String& newURL) override;
    void showCertificate(const WebCore::CertificateInfo&) override;

    // InspectorFrontendClientLocal API.
    void attachWindow(DockSide) override;
    void detachWindow() override;

    void destroyInspectorView(bool notifyInspectorController);

private:
    WebView* m_frontendWebView;
    WebView* m_inspectedWebView;
    WebInspectorClient* m_inspectorClient;

    bool m_shouldAttachWhenShown;
    bool m_attached;

    WTF::String m_inspectedURL;
    bool m_destroyingInspectorView;
};

#endif
