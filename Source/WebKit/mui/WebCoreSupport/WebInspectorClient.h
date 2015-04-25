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

#include "InspectorClient.h"
#include "InspectorFrontendClientLocal.h"

#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include "SharedPtr.h"
#include <wtf/Forward.h>
#include <wtf/text/StringHash.h>

namespace WebCore {
    class Node;
    class Page;
}

class WebView;
class WebInspectorFrontendClient;
class WebInspector;

class WebInspectorClient : public WebCore::InspectorClient, public WebCore::InspectorFrontendChannel {
public:
    WebInspectorClient(WebView*);
    virtual ~WebInspectorClient();
    //WTF::PassOwnPtr<WebCore::InspectorFrontendClientLocal::Settings> createFrontendSettings(); 
 
    void disconnectFrontendClient() { m_frontendClient = 0; }

    virtual void inspectorDestroyed();

    virtual WebCore::InspectorFrontendChannel* openInspectorFrontend(WebCore::InspectorController*);
    virtual void closeInspectorFrontend();
    virtual void bringFrontendToFront();

    virtual void highlight();
	virtual void hideHighlight();

    virtual bool sendMessageToFrontend(const WTF::String& message);

    //bool inspectorStartsAttached();
    //void setInspectorStartsAttached(bool);
    //bool inspectorAttachDisabled(); 
    //void setInspectorAttachDisabled(bool); 

    void releaseFrontend();

    WebInspectorFrontendClient* frontendClient() { return m_frontendClient; }

    void updateHighlight();

    void loadSettings();
    void saveSettings();

private:
    WebView* m_webView;
    WebCore::Page* m_frontendPage;
    WebInspectorFrontendClient* m_frontendClient;
    WTF::HashMap<WTF::String, WTF::String> *m_settings;
    WTF::HashMap<WTF::String, WTF::String> m_sessionSettings;
};

class WebInspectorFrontendClient : public WebCore::InspectorFrontendClientLocal {
public:
    WebInspectorFrontendClient(WebView* inspectedWebView, WebView* frontendWebView, WebCore::Page* inspectorPage, WebInspectorClient* inspectorClient, std::unique_ptr<Settings>);
    virtual ~WebInspectorFrontendClient();

    virtual void frontendLoaded();
    
    virtual WTF::String localizedStringsURL();
    
    virtual void bringToFront();
    virtual void closeWindow();
    
    virtual void attachWindow(DockSide); 
    virtual void detachWindow();
    
    virtual void setAttachedWindowHeight(unsigned height);
    virtual void setAttachedWindowWidth(unsigned); 
    virtual void setToolbarHeight(unsigned); 

    virtual void inspectedURLChanged(const WTF::String& newURL);

	void destroyInspectorView(bool notifyInspectorController);

    void disconnectInspectorClient() { m_inspectorClient = 0; }

private:
    WebView* m_frontendWebView;
    WebView* m_inspectedWebView;
    WebInspectorClient* m_inspectorClient;

    bool m_shouldAttachWhenShown;
    bool m_attached;

    WTF::String m_inspectedURL;
    bool m_destroyingInspectorView;
#if ENABLE(DAE)
    SharedPtr<WebApplication> m_application;
#endif
};

#endif
