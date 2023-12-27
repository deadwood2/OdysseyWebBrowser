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

#import "PlatformUtilities.h"
#import <WebKit/WKPreferences.h>
#import <WebKit/WKUIDelegatePrivate.h>
#import <WebKit/WKWebView.h>
#import <WebKit/WKWebViewConfiguration.h>
#import <wtf/RetainPtr.h>

#if WK_API_ENABLED

@class ModalAlertsUIDelegate;

static bool isDone;
static RetainPtr<WKWebView> openedWebView;
static RetainPtr<ModalAlertsUIDelegate> sharedUIDelegate;

static unsigned dialogsSeen;
static const unsigned dialogsExpected = 3;

static void sawDialog()
{
    if (++dialogsSeen == dialogsExpected)
        isDone = true;
}

@interface ModalAlertsUIDelegate : NSObject <WKUIDelegate>
@end

@implementation ModalAlertsUIDelegate

- (WKWebView *)webView:(WKWebView *)webView createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration forNavigationAction:(WKNavigationAction *)navigationAction windowFeatures:(WKWindowFeatures *)windowFeatures
{
    openedWebView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration]);
    [openedWebView setUIDelegate:sharedUIDelegate.get()];
    return openedWebView.get();
}

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler
{
    EXPECT_STREQ([[[[frame request] URL] absoluteString] UTF8String], "about:blank");
    EXPECT_STREQ([[[frame securityOrigin] protocol] UTF8String], "file");
    EXPECT_STREQ([[[frame securityOrigin] host] UTF8String], "");
    EXPECT_EQ([[frame securityOrigin] port], 0);

    completionHandler();
    sawDialog();
}

- (void)webView:(WKWebView *)webView runJavaScriptConfirmPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(BOOL result))completionHandler
{
    EXPECT_STREQ([[[[frame request] URL] absoluteString] UTF8String], "about:blank");
    EXPECT_STREQ([[[frame securityOrigin] protocol] UTF8String], "file");
    EXPECT_STREQ([[[frame securityOrigin] host] UTF8String], "");
    EXPECT_EQ([[frame securityOrigin] port], 0);

    completionHandler(true);
    sawDialog();
}

- (void)webView:(WKWebView *)webView runJavaScriptTextInputPanelWithPrompt:(NSString *)prompt defaultText:(NSString *)defaultText initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(NSString *result))completionHandler
{
    EXPECT_STREQ([[[[frame request] URL] absoluteString] UTF8String], "about:blank");
    EXPECT_STREQ([[[frame securityOrigin] protocol] UTF8String], "file");
    EXPECT_STREQ([[[frame securityOrigin] host] UTF8String], "");
    EXPECT_EQ([[frame securityOrigin] port], 0);

    completionHandler(@"");
    sawDialog();
}

@end

TEST(WebKit2, ModalAlerts)
{
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)]);

    sharedUIDelegate = adoptNS([[ModalAlertsUIDelegate alloc] init]);
    [webView setUIDelegate:sharedUIDelegate.get()];

    [webView configuration].preferences.javaScriptCanOpenWindowsAutomatically = YES;

    NSURLRequest *request = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"modal-alerts-in-new-about-blank-window" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&isDone);
}

#endif
