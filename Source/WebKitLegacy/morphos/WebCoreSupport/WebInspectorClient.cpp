/*
 * Copyright (C) 2006-2010, 2014, 2015 Apple Inc.  All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "WebKit.h"
#include "WebInspectorClient.h"

#include "WebPage.h"
#include <WebCore/Page.h>
#include <WebCore/InspectorController.h>
#include <JavaScriptCore/InspectorAgentBase.h>
#include <wtf/HashMap.h>

#define D(x) 

using namespace WebCore;

namespace WebKit {

class WebInspectorFrontendClient final : public WebCore::InspectorFrontendClientLocal {
    WTF_MAKE_FAST_ALLOCATED;
public:
    WebInspectorFrontendClient(WebPage* inspectedWebView, WebCore::Page *frontendPage, WebInspectorClient* parent, std::unique_ptr<WebCore::InspectorFrontendClientLocal::Settings>&& settings)
		: InspectorFrontendClientLocal(&inspectedWebView->corePage()->inspectorController(), frontendPage, std::move(settings))
		, m_inspectedWebView(inspectedWebView)
		, m_frontendView(WebPage::fromCorePage(frontendPage))
		, m_inspectorClient(parent) {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	}

    virtual ~WebInspectorFrontendClient() {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
		destroyInspectorView();
	}

    // InspectorFrontendClient API.
    void frontendLoaded() override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
		InspectorFrontendClientLocal::frontendLoaded();
	}

    WTF::String localizedStringsURL() const override {
		return "file:///PROGDIR:Resources/WebInspectorUI/Localizations/en.lproj/localizedStrings.js"_s;
	}

    Inspector::DebuggableType debuggableType() const final { return Inspector::DebuggableType::Page; };
    String targetPlatformName() const final { return "MorphOS"_s; }
    String targetBuildVersion() const final { return "Unknown"_s; }
    String targetProductVersion() const final { return "Unknown"_s; }
    bool targetIsSimulator() const final { return false; }

    void bringToFront() override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    void closeWindow() override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    void reopen() override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    void resetState() override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
		InspectorFrontendClientLocal::resetState();
	}

    void setForcedAppearance(InspectorFrontendClient::Appearance) override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    bool supportsDockSide(DockSide) override {
		return false;
	}

    void setAttachedWindowHeight(unsigned) override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    void setAttachedWindowWidth(unsigned) override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    void setSheetRect(const WebCore::FloatRect&) override {
    
	}

    void inspectedURLChanged(const WTF::String& newURL) override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
		m_frontendView->_fInspectorURLChanged(newURL);
	}

    void showCertificate(const WebCore::CertificateInfo&) override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    // InspectorFrontendClientLocal API.
    void attachWindow(DockSide) override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    
	}

    void detachWindow() override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
    }

    void destroyInspectorView() {
        if (m_destroyingInspectorView)
			return;
		m_destroyingInspectorView = true;

		D(dprintf("%s: \n", __PRETTY_FUNCTION__));

		if (Page* frontendPage = m_frontendView->corePage())
			frontendPage->inspectorController().setInspectorFrontendClient(nullptr);
		if (Page* inspectedPage = m_inspectedWebView->corePage())
			inspectedPage->inspectorController().disconnectFrontend(*m_inspectorClient);

		m_inspectorClient->releaseFrontend();
	}

    bool canSave(InspectorFrontendClient::SaveMode) override {
		D(dprintf("%s: \n", __PRETTY_FUNCTION__));
		return m_frontendView->_fInspectorSave != nullptr;
	}
    
    void save(Vector<InspectorFrontendClient::SaveData>&& saveData, bool forceSaveAs) override {
		D(dprintf("%s: %p %p\n", __PRETTY_FUNCTION__, m_inspectedWebView, m_inspectedWebView->_fInspectorSave));
        for (auto save : saveData)
            m_frontendView->_fInspectorSave(save.url, save.content, save.base64Encoded);
	}

protected:
	WebPage* m_inspectedWebView;
	WebPage* m_frontendView;
	WebInspectorClient* m_inspectorClient;
	bool m_destroyingInspectorView = false;
};

class WebInspectorClientSettings : public WebCore::InspectorFrontendClientLocal::Settings
{
public:
	WebInspectorClientSettings() = default;
	~WebInspectorClientSettings() = default;

	String getProperty(const String& name) override
	{
		auto it = m_settings.find(name);
		if (it != m_settings.end())
			return it->value;
		return String();
	}
	
	void setProperty(const String& name, const String& value)
	{
		m_settings.set(name, value);
	}
	
	void deleteProperty(const String& name)
	{
		m_settings.remove(name);
	}
protected:
	HashMap<String, String> m_settings;
};

WebInspectorClient::WebInspectorClient(WebPage* webView)
	: m_inspectedPage(webView)
{
	D(dprintf("%s: %p\n", __PRETTY_FUNCTION__, webView));
}

WebInspectorClient::~WebInspectorClient()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
}

void WebInspectorClient::inspectedPageDestroyed()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
}

void WebInspectorClient::inspectedPageWillBeDestroyed()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	if (m_inspectedPage->_fInspectorDestroyed)
		m_inspectedPage->_fInspectorDestroyed();

	if (m_frontendClient)
		m_frontendClient->destroyInspectorView();

	m_frontendClient = nullptr;
	D(dprintf("%s: done!\n", __PRETTY_FUNCTION__));
}

Inspector::FrontendChannel* WebInspectorClient::openLocalFrontend(InspectorController* inspectorController)
{
	auto *frontendPage = m_inspectedPage->_fOpenInspectorWindow();
	D(dprintf("%s: fpage %p\n", __PRETTY_FUNCTION__, frontendPage));
	if (frontendPage)
	{
		WebPage* inspectorWebPage = WebPage::fromCorePage(frontendPage);
		inspectorWebPage->load("file:///PROGDIR:Resources/WebInspectorUI/Main.html", true);

		auto settings = makeUnique<WebInspectorClientSettings>();
		m_frontendClient = makeUnique<WebInspectorFrontendClient>(m_inspectedPage, frontendPage, this, std::move(settings));

		m_inspectedPage->corePage()->inspectorController().connectFrontend(*this);
		frontendPage->inspectorController().setInspectorFrontendClient(m_frontendClient.get());

		return this;
	}
	
	return nullptr;
}

void WebInspectorClient::bringFrontendToFront()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
}

void WebInspectorClient::closeInspector()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	if (m_frontendClient)
		m_frontendClient->destroyInspectorView();
}

void WebInspectorClient::highlight()
{
    m_inspectedPage->inspectorHighlightUpdated();
}

void WebInspectorClient::hideHighlight()
{
    m_inspectedPage->inspectorHighlightUpdated();
}

void WebInspectorClient::releaseFrontend()
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	m_frontendClient = nullptr;
}

void WebInspectorClient::sendMessageToFrontend(const WTF::String& message)
{
	D(dprintf("%s: \n", __PRETTY_FUNCTION__));
	if (m_frontendClient)
		m_frontendClient->frontendAPIDispatcher().dispatchMessageAsync(message);
}

}
