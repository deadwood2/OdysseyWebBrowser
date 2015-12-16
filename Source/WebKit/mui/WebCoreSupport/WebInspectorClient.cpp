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

#include "config.h"
#include "WebInspectorClient.h"

#include <wtf/text/CString.h>
#include "IntRect.h"
#include "NotImplemented.h"
#include "Frame.h"
#include "FrameView.h"
#include "InspectorController.h"
#include "Page.h"
#include <wtf/text/WTFString.h>
#include "WebFrameLoadDelegate.h"
#include "JSActionDelegate.h"
#include "WebPreferences.h"
#include "WebUtils.h"
#include "WebView.h"
#include "WebFrame.h"
#include <inspector/InspectorAgentBase.h>

#include <stdlib.h>
#include <string>

#if ENABLE(DAE)
#include "WebApplication.h"
#include "WebApplicationManager.h"
#endif

#include "gui.h"
#include "utils.h"
#include <proto/intuition.h>
#include <clib/debug_protos.h>
#include <proto/asl.h>
#include "asl.h"
#define D(x)
#undef get
#undef set
#undef String


using namespace WebCore;
using namespace std;

WebInspectorClient::WebInspectorClient(WebView *view)
	: m_webView(view)
	, m_frontendPage(0)
	, m_frontendClient(0)
{
}

WebInspectorClient::~WebInspectorClient()
{
    D(kprintf("WebInspectorClient::~WebInspectorClient\n"));
}

void WebInspectorClient::inspectorDestroyed()
{
    D(kprintf("inspectorDestroyed\n"));
    closeInspectorFrontend();
    delete this;
}

WebCore::InspectorFrontendChannel* WebInspectorClient::openInspectorFrontend(InspectorController* inspectorController)
{
    const char* inspectorURL = getenv("OWB_INSPECTOR_URL");
    if (!inspectorURL)
    {
	inspectorURL = "PROGDIR:webinspector/inspector.html";
    }

#if 1
    string url = "file:///";
    url.append(inspectorURL);
    BalWidget *widget = (BalWidget *) DoMethod(app, MM_OWBApp_AddWindow, url.c_str(), FALSE, NULL, FALSE, NULL, FALSE);
#else
    Object *window = (Object *) getv(app, MA_OWBApp_ActiveWindow);
    BalWidget *inspectedwidget = m_webView->viewWindow();
    BalWidget *widget = (BalWidget *) DoMethod(window, MM_OWBWindow_CreateInspector, inspectedwidget->browser);
#endif

    if(widget)
    {
		m_frontendPage = core(widget->webView);
		if(m_frontendPage)
		{
		    WebInspectorFrontendClient * frontendClient = new WebInspectorFrontendClient(m_webView, widget->webView, m_frontendPage, this,
		            std::make_unique<WebCore::InspectorFrontendClientLocal::Settings>() );
			m_frontendClient = frontendClient;
			m_frontendPage->inspectorController().setInspectorFrontendClient(frontendClient);
			return this;
		}
    }
    return 0;
}

void WebInspectorClient::closeInspectorFrontend()
{
    if (m_frontendClient)
        m_frontendClient->destroyInspectorView(false);
}

void WebInspectorClient::bringFrontendToFront()
{
    m_frontendClient->bringToFront();
}

void WebInspectorClient::releaseFrontend()
{
    D(kprintf("releaseFrontend\n"));
    m_frontendPage = 0;
    m_frontendClient = 0;
}

void WebInspectorClient::highlight()
{
	//hideHighlight();
}

void WebInspectorClient::hideHighlight()
{
	/*
	IntRect rect(0, 0, core(m_webView->mainFrame())->view()->contentsWidth(), core(m_webView->mainFrame())->view()->contentsHeight());
	m_webView->addToDirtyRegion(rect);
	m_webView->sendExposeEvent(rect);
	*/
}

void WebInspectorClient::updateHighlight()
{
}

bool WebInspectorClient::sendMessageToFrontend(const String& message)
{
    return doDispatchMessageOnFrontendPage(m_frontendPage, message);

    /*
    if (!m_frontendPage)
        return false;

    Frame* frame = m_frontendPage->mainFrame();
    if (!frame)
        return false;

    ScriptController* scriptController = frame->script();
    if (!scriptController)
        return false;

    String dispatchToFrontend("WebInspector.dispatchMessageFromBackend(");
    dispatchToFrontend.append(message);
    dispatchToFrontend.append(");");
    scriptController->executeScript(dispatchToFrontend);
    return true;
    */
}

void WebInspectorClient::loadSettings()
{
}

void WebInspectorClient::saveSettings()
{
}


WebInspectorFrontendClient::WebInspectorFrontendClient(WebView* inspectedWebView, WebView* frontendWebView, Page* inspectorPage, WebInspectorClient* inspectorClient, std::unique_ptr<Settings> settings)
    : InspectorFrontendClientLocal(&core(inspectedWebView)->inspectorController(), inspectorPage, std::move(settings))
    , m_frontendWebView(frontendWebView)
    , m_inspectedWebView(inspectedWebView)
    , m_inspectorClient(inspectorClient)
    , m_attached(false)
    , m_destroyingInspectorView(false)
{
}

WebInspectorFrontendClient::~WebInspectorFrontendClient()
{
	D(kprintf("WebInspectorFrontendClient::~WebInspectorFrontendClient\n"));
	destroyInspectorView(true); 
}

void WebInspectorFrontendClient::frontendLoaded()
{
    InspectorFrontendClientLocal::frontendLoaded();

    //if (m_attached)
    //    restoreAttachedWindowHeight();

    setAttachedWindow(m_attached ? InspectorFrontendClient::DockSide::Bottom : InspectorFrontendClient::DockSide::Undocked);
}

String WebInspectorFrontendClient::localizedStringsURL()
{
    return String();
}

void WebInspectorFrontendClient::bringToFront()
{
    // If no preference is set - default to an attached window. This is important for inspector LayoutTests.
    //bool shouldAttach = m_inspectorClient->inspectorStartsAttached(); 

    /*if (m_inspectedWebView->page())
        m_inspectedWebView->page()->inspectorController()->setWindowVisible(true, true);*/
}

void WebInspectorFrontendClient::closeWindow()
{
	D(kprintf("closeWindow\n"));
	destroyInspectorView(true);
}

void WebInspectorFrontendClient::attachWindow(DockSide)
{
    if (m_attached)
        return;

    //m_inspectorClient->setInspectorStartsAttached(true);

    //m_attached = true;
    //restoreAttachedWindowHeight();
}

void WebInspectorFrontendClient::detachWindow()
{
    if (!m_attached)
        return;

    //m_inspectorClient->setInspectorStartsAttached(false); 
}

void WebInspectorFrontendClient::setAttachedWindowHeight(unsigned height)
{
    notImplemented();
}

void WebInspectorFrontendClient::setAttachedWindowWidth(unsigned) 
{
    notImplemented();
}

void WebInspectorFrontendClient::setToolbarHeight(unsigned) 
{
    notImplemented();
}


void WebInspectorFrontendClient::inspectedURLChanged(const String& newURL)
{
    D(kprintf("inspectedURLChanged <%s>\n", newURL.latin1().data()));
    notImplemented();
}

void WebInspectorFrontendClient::destroyInspectorView(bool notifyInspectorController)
{
    D(kprintf("this %p destroyInspectorView(%d)\n", this, notifyInspectorController));

    m_inspectorClient->releaseFrontend();

    if (m_destroyingInspectorView)
        return;

    m_destroyingInspectorView = true;

	if (notifyInspectorController)
	{
		D(kprintf("disconnectFrontend\n"));
		if(m_inspectedWebView)
		{
			core(m_inspectedWebView)->inspectorController().disconnectFrontend(Inspector::DisconnectReason::InspectorDestroyed);
		}
		if(m_inspectorClient)
		{
			m_inspectorClient->updateHighlight();
		}
	}
}
