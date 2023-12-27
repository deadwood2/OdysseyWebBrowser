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

#import <WebKit/WKNavigationPrivate.h>
#import <WebKit/WKNavigationDelegate.h>
#import <WebKit/WKWebView.h>
#import <wtf/RetainPtr.h>
#import "PlatformUtilities.h"
#import "Test.h"

#if WK_API_ENABLED

static bool isDone;
static RetainPtr<WKNavigation> currentNavigation;

@interface NavigationDelegate : NSObject <WKNavigationDelegate>
@end

@implementation NavigationDelegate

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
    EXPECT_EQ(currentNavigation, navigation);
    EXPECT_NOT_NULL(navigation._request);
}

- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation
{
    EXPECT_EQ(currentNavigation, navigation);
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    EXPECT_EQ(currentNavigation, navigation);

    isDone = true;
}

@end

TEST(WKNavigation, NavigationDelegate)
{
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)]);

    NavigationDelegate *delegate = [[NavigationDelegate alloc] init];
    [webView setNavigationDelegate:delegate];

    @autoreleasepool {
        EXPECT_EQ(delegate, [webView navigationDelegate]);
    }

    [delegate release];
    EXPECT_NULL([webView navigationDelegate]);
}

TEST(WKNavigation, LoadRequest)
{
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)]);

    RetainPtr<NavigationDelegate> delegate = adoptNS([[NavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"simple" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];

    currentNavigation = [webView loadRequest:request];
    ASSERT_NOT_NULL(currentNavigation);
    ASSERT_TRUE([[currentNavigation _request] isEqual:request]);

    isDone = false;
    TestWebKitAPI::Util::run(&isDone);
}

@interface DidFailProvisionalNavigationDelegate : NSObject <WKNavigationDelegate>
@end

@implementation DidFailProvisionalNavigationDelegate

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
    EXPECT_EQ(currentNavigation, navigation);
    EXPECT_NOT_NULL(navigation._request);
}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
    EXPECT_EQ(currentNavigation, navigation);
    EXPECT_NOT_NULL(navigation._request);

    EXPECT_TRUE([error.domain isEqualToString:NSURLErrorDomain]);
    EXPECT_EQ(NSURLErrorUnsupportedURL, error.code);

    isDone = true;
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    decisionHandler(WKNavigationActionPolicyAllow);
}

@end

TEST(WKNavigation, DidFailProvisionalNavigation)
{
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)]);

    RetainPtr<DidFailProvisionalNavigationDelegate> delegate = adoptNS([[DidFailProvisionalNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"non-existant-scheme://"]];

    currentNavigation = [webView loadRequest:request];
    ASSERT_NOT_NULL(currentNavigation);
    ASSERT_TRUE([[currentNavigation _request] isEqual:request]);

    isDone = false;
    TestWebKitAPI::Util::run(&isDone);
}

@interface DecidePolicyForPageCacheNavigationDelegate : NSObject <WKNavigationDelegate>
@property (nonatomic) BOOL decidedPolicyForBackForwardNavigation;
@end

@implementation DecidePolicyForPageCacheNavigationDelegate

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    isDone = true;
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    if (navigationAction.navigationType == WKNavigationTypeBackForward)
        _decidedPolicyForBackForwardNavigation = YES;

    decisionHandler(WKNavigationActionPolicyAllow);
}

@end

TEST(WKNavigation, DecidePolicyForPageCacheNavigation)
{
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)]);

    RetainPtr<DecidePolicyForPageCacheNavigationDelegate> delegate = adoptNS([[DecidePolicyForPageCacheNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"data:text/html,1"]];

    isDone = false;
    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&isDone);

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"data:text/html,2"]];

    isDone = false;
    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&isDone);

    isDone = false;
    [webView goBack];
    TestWebKitAPI::Util::run(&isDone);

    ASSERT_TRUE([delegate decidedPolicyForBackForwardNavigation]);
}

#endif
