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
#include <wtf/bal/PtrAndFlags.h>
#include "WebContextMenuClient.h"
#include "UserGestureIndicator.h"
#include <wtf/text/CString.h>

#include "WebDownload.h"
#include "WebHitTestResults.h"
#include "WebView.h"

#include <ContextMenu.h>
#include <Editor.h>
#include <Event.h>
#include <MainFrame.h>
#include <FrameLoader.h>
#include <FrameLoadRequest.h>
#include <Page.h>
#include <ResourceRequest.h>
#include <NotImplemented.h>

#include "gui.h"
#include <clib/debug_protos.h>
#undef String

using namespace WebCore;

WebContextMenuClient::WebContextMenuClient(WebView* webView)
    : m_webView(webView)
{
}

WebContextMenuClient::~WebContextMenuClient()
{
}

void WebContextMenuClient::contextMenuDestroyed()
{
    delete this;
}

PlatformMenuDescription WebContextMenuClient::getCustomMenuFromDefaultItems(ContextMenu* menu)
{
    return menu->releasePlatformDescription();
}

void WebContextMenuClient::contextMenuItemSelected(ContextMenuItem* item, const ContextMenu* parentMenu)
{
    DoMethod(app, MM_OWBApp_SelectUserMenu, (APTR) item, (APTR) &(m_webView->page()->contextMenuController()));
}

void WebContextMenuClient::downloadURL(const URL& url)
{
    SharedPtr<WebDownloadDelegate> downloadDelegate = m_webView->downloadDelegate();

    if(downloadDelegate)
    {
		String originURL;
		char *tmp = (char *) m_webView->mainFrameURL();
		if(tmp)
		{
			originURL = tmp;
			free(tmp);
		}
		WebDownload* download = WebDownload::createInstance(url, originURL, downloadDelegate);
        download->start();
    }
}

void WebContextMenuClient::copyImageToClipboard(const HitTestResult&)
{
    notImplemented();
}


void WebContextMenuClient::searchWithGoogle(const Frame* frame)
{
    String searchString = frame->editor().selectedText();
    searchString.stripWhiteSpace();
    String encoded = encodeWithURLEscapeSequences(searchString);
    encoded.replace("%20", "+");
    
    String url("http://www.google.com/search?q=");
    url.append(encoded);
    url.append("&ie=UTF-8&oe=UTF-8");

    if (Page* page = frame->page()) {
      UserGestureIndicator indicator(DefinitelyProcessingUserGesture);
      page->mainFrame().loader().urlSelected(URL(ParsedURLString, url), String(), 0,
              LockHistory::No, LockBackForwardList::No, MaybeSendReferrer, ShouldOpenExternalURLsPolicy::ShouldNotAllow);
    };
}

void WebContextMenuClient::lookUpInDictionary(Frame*)
{
    notImplemented();
}

void WebContextMenuClient::speak(const String&)
{
    notImplemented();
}

void WebContextMenuClient::stopSpeaking()
{
    notImplemented();
}

bool WebContextMenuClient::isSpeaking()
{
    return false;
}

WebCore::ContextMenuItem WebContextMenuClient::shareMenuItem(const WebCore::HitTestResult&)
{
    return WebCore::ContextMenuItem();
}
