/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#import <WebKit/WKFoundation.h>

#if WK_API_ENABLED

#import "PlatformUtilities.h"
#import "Test.h"
#import <WebKit/WKWebViewPrivate.h>
#import <wtf/RetainPtr.h>

static bool isDone;

@interface LoadAlternateHTMLStringFromProvisionalLoadErrorController : NSObject <WKNavigationDelegate>
@end

@implementation LoadAlternateHTMLStringFromProvisionalLoadErrorController

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
    [webView _loadAlternateHTMLString:@"error page" baseURL:nil forUnreachableURL:[NSURL URLWithString:@"data:"]];
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    isDone = true;
}

@end

static NSString *unloadableURL = @"data:";
static NSString *loadableURL = @"data:text/html,no%20error";

TEST(WKWebView, LoadAlternateHTMLStringFromProvisionalLoadError)
{
    auto webView = adoptNS([[WKWebView alloc] init]);
    auto controller = adoptNS([[LoadAlternateHTMLStringFromProvisionalLoadErrorController alloc] init]);
    [webView setNavigationDelegate:controller.get()];

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:unloadableURL]]];
    TestWebKitAPI::Util::run(&isDone);
    isDone = false;

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:loadableURL]]];
    TestWebKitAPI::Util::run(&isDone);
    isDone = false;

    [webView goBack];
    TestWebKitAPI::Util::run(&isDone);
    isDone = false;

    WKBackForwardList *list = [webView backForwardList];
    EXPECT_EQ((NSUInteger)0, list.backList.count);
    EXPECT_EQ((NSUInteger)1, list.forwardList.count);
    EXPECT_STREQ([[list.forwardList.firstObject URL] absoluteString].UTF8String, loadableURL.UTF8String);
    EXPECT_STREQ([[list.currentItem URL] absoluteString].UTF8String, unloadableURL.UTF8String);

    EXPECT_TRUE([webView canGoForward]);
    if (![webView canGoForward])
        return;

    [webView goForward];
    TestWebKitAPI::Util::run(&isDone);
    isDone = false;

    EXPECT_EQ((NSUInteger)1, list.backList.count);
    EXPECT_EQ((NSUInteger)0, list.forwardList.count);
    EXPECT_STREQ([[list.backList.firstObject URL] absoluteString].UTF8String, unloadableURL.UTF8String);
    EXPECT_STREQ([[list.currentItem URL] absoluteString].UTF8String, loadableURL.UTF8String);
}

#endif
