/*
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "stdafx.h"
#include "WebKitBrowserWindow.h"

#include "MiniBrowserLibResource.h"
#include <WebKit/WKInspector.h>
#include <vector>

std::wstring
createString(WKStringRef wkString)
{
    size_t maxSize = WKStringGetLength(wkString);

    std::vector<WKChar> wkCharBuffer(maxSize);
    size_t actualLength = WKStringGetCharacters(wkString, wkCharBuffer.data(), maxSize);
    return std::wstring(wkCharBuffer.data(), actualLength);
}

std::wstring createString(WKURLRef wkURL)
{
    WKRetainPtr<WKStringRef> url = adoptWK(WKURLCopyString(wkURL));
    return createString(url.get());
}

std::vector<char> toNullTerminatedUTF8(const wchar_t* src, size_t srcLength)
{
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, src, srcLength, 0, 0, nullptr, nullptr);
    std::vector<char> utf8Buffer(utf8Length + 1);
    WideCharToMultiByte(CP_UTF8, 0, src, srcLength,
        utf8Buffer.data(), utf8Length, nullptr, nullptr);
    utf8Buffer[utf8Length] = '\0';
    return utf8Buffer;
}

WKRetainPtr<WKStringRef>
createWKString(_bstr_t str)
{
    auto utf8 = toNullTerminatedUTF8(str, str.length());
    return adoptWK(WKStringCreateWithUTF8CString(utf8.data()));
}

WKRetainPtr<WKURLRef>
createWKURL(_bstr_t str)
{
    auto utf8 = toNullTerminatedUTF8(str, str.length());
    return adoptWK(WKURLCreateWithUTF8CString(utf8.data()));
}

Ref<BrowserWindow> WebKitBrowserWindow::create(HWND mainWnd, HWND urlBarWnd, bool, bool)
{
    return adoptRef(*new WebKitBrowserWindow(mainWnd, urlBarWnd));
}

WebKitBrowserWindow::WebKitBrowserWindow(HWND mainWnd, HWND urlBarWnd)
    : m_hMainWnd(mainWnd)
    , m_urlBarWnd(urlBarWnd)
{
    RECT rect = { };
    auto conf = adoptWK(WKPageConfigurationCreate());

    auto prefs = WKPreferencesCreate();
    WKPreferencesSetDeveloperExtrasEnabled(prefs, true);
    WKPageConfigurationSetPreferences(conf.get(), prefs);

    auto context = adoptWK(WKContextCreate());
    WKPageConfigurationSetContext(conf.get(), context.get());

    m_view = adoptWK(WKViewCreate(rect, conf.get(), mainWnd));
    auto page = WKViewGetPage(m_view.get());

    WKPageLoaderClientV0 loadClient = {{ 0, this }};
    loadClient.didReceiveTitleForFrame = didReceiveTitleForFrame;
    loadClient.didCommitLoadForFrame = didCommitLoadForFrame;
    WKPageSetPageLoaderClient(page, &loadClient.base);
}

HRESULT WebKitBrowserWindow::init()
{
    return S_OK;
}

HWND WebKitBrowserWindow::hwnd()
{
    return WKViewGetWindow(m_view.get());
}

HRESULT WebKitBrowserWindow::loadURL(const BSTR& url)
{
    auto page = WKViewGetPage(m_view.get());
    WKPageLoadURL(page, createWKURL(url).get());
    return true;
}

HRESULT WebKitBrowserWindow::loadHTMLString(const BSTR& str)
{
    auto page = WKViewGetPage(m_view.get());
    auto url = createWKURL(L"about:");
    WKPageLoadHTMLString(page, createWKString(_bstr_t(str)).get(), url.get());
    return true;
}

void WebKitBrowserWindow::navigateForwardOrBackward(UINT menuID)
{
    auto page = WKViewGetPage(m_view.get());
    if (menuID == IDM_HISTORY_FORWARD)
        WKPageGoForward(page);
    else
        WKPageGoBack(page);
}

void WebKitBrowserWindow::navigateToHistory(UINT menuID)
{
    // Not implemented
}

void WebKitBrowserWindow::setPreference(UINT menuID, bool enable)
{
    auto page = WKViewGetPage(m_view.get());
    auto pgroup = WKPageGetPageGroup(page);
    auto pref = WKPageGroupGetPreferences(pgroup);
    switch (menuID) {
    case IDM_DISABLE_IMAGES:
        WKPreferencesSetLoadsImagesAutomatically(pref, !enable);
        break;
    case IDM_DISABLE_JAVASCRIPT:
        WKPreferencesSetJavaScriptEnabled(pref, !enable);
        break;
    }
}

void WebKitBrowserWindow::print()
{
    // Not implemented
}

void WebKitBrowserWindow::launchInspector()
{
    auto page = WKViewGetPage(m_view.get());
    auto inspector = WKPageGetInspector(page);
    WKInspectorShow(inspector);
}

void WebKitBrowserWindow::setUserAgent(_bstr_t& customUAString)
{
    auto page = WKViewGetPage(m_view.get());
    auto ua = createWKString(customUAString);
    WKPageSetCustomUserAgent(page, ua.get());
}

_bstr_t WebKitBrowserWindow::userAgent()
{
    auto page = WKViewGetPage(m_view.get());
    auto ua = adoptWK(WKPageCopyUserAgent(page));
    return createString(ua.get()).c_str();
}

void WebKitBrowserWindow::showLayerTree()
{
    // Not implemented
}

void WebKitBrowserWindow::updateStatistics(HWND hDlg)
{
    // Not implemented
}


void WebKitBrowserWindow::resetZoom()
{
    auto page = WKViewGetPage(m_view.get());
    WKPageSetPageZoomFactor(page, 1);
}

void WebKitBrowserWindow::zoomIn()
{
    auto page = WKViewGetPage(m_view.get());
    double s = WKPageGetPageZoomFactor(page);
    WKPageSetPageZoomFactor(page, s * 1.25);
}

void WebKitBrowserWindow::zoomOut()
{
    auto page = WKViewGetPage(m_view.get());
    double s = WKPageGetPageZoomFactor(page);
    WKPageSetPageZoomFactor(page, s * 0.8);
}

static WebKitBrowserWindow& toWebKitBrowserWindow(const void *clientInfo)
{
    return *const_cast<WebKitBrowserWindow*>(static_cast<const WebKitBrowserWindow*>(clientInfo));
}

void WebKitBrowserWindow::didReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;
    std::wstring titleString = createString(title) + L" [WebKit]";
    auto& thisWindow = toWebKitBrowserWindow(clientInfo);
    SetWindowText(thisWindow.m_hMainWnd, titleString.c_str());
}

void WebKitBrowserWindow::didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void *clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;
    auto& thisWindow = toWebKitBrowserWindow(clientInfo);

    WKRetainPtr<WKURLRef> wkurl = adoptWK(WKFrameCopyURL(frame));
    std::wstring urlString = createString(wkurl.get());
    SetWindowText(thisWindow.m_urlBarWnd, urlString.c_str());
}
