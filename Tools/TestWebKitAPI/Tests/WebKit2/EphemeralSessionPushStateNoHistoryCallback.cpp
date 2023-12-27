/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#include "config.h"

#if WK_HAVE_C_SPI

#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "Test.h"
#include <WebKit/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool testDone;

static void didNavigateWithNavigationData(WKContextRef context, WKPageRef page, WKNavigationDataRef navigationData, WKFrameRef frame, const void* clientInfo)
{
    // This should never be called when navigating in Private Browsing.
    FAIL();
}

static void didSameDocumentNavigationForFrame(WKPageRef page, WKFrameRef frame, WKSameDocumentNavigationType type, WKTypeRef userData, const void *clientInfo)
{
    testDone = true;
}

TEST(WebKit2, EphemeralSessionPushStateNoHistoryCallback)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());

    WKContextHistoryClientV0 historyClient;
    memset(&historyClient, 0, sizeof(historyClient));

    historyClient.base.version = 0;
    historyClient.didNavigateWithNavigationData = didNavigateWithNavigationData;

    WKContextSetHistoryClient(context.get(), &historyClient.base);

    PlatformWebView webView(context.get());

    WKPageLoaderClientV0 pageLoaderClient;
    memset(&pageLoaderClient, 0, sizeof(pageLoaderClient));

    pageLoaderClient.base.version = 0;
    pageLoaderClient.didSameDocumentNavigationForFrame = didSameDocumentNavigationForFrame;

    WKPageSetPageLoaderClient(webView.page(), &pageLoaderClient.base);

    WKSessionRef session = WKSessionCreate(true);
    WKPageSetSession(webView.page(), session);

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("push-state", "html"));
    WKPageLoadURL(webView.page(), url.get());

    Util::run(&testDone);
}

} // namespace TestWebKitAPI

#endif
