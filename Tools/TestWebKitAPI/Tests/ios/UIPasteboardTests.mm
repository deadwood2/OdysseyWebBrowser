/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#if PLATFORM(IOS) && WK_API_ENABLED

#import "PlatformUtilities.h"
#import "TestWKWebView.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import <UIKit/UIPasteboard.h>
#import <WebKit/WKPreferencesPrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <wtf/SoftLinking.h>

SOFT_LINK_FRAMEWORK(UIKit)
SOFT_LINK(UIKit, UIApplicationInitialize, void, (void), ())

namespace TestWebKitAPI {

NSData *dataForPasteboardType(CFStringRef type)
{
    return [[UIPasteboard generalPasteboard] dataForPasteboardType:(NSString *)type inItemSet:[NSIndexSet indexSetWithIndex:0]].firstObject;
}

RetainPtr<TestWKWebView> setUpWebViewForPasteboardTests()
{
    // UIPasteboard's type coercion codepaths only take effect when the UIApplication has been initialized.
    UIApplicationInitialize();

    [UIPasteboard generalPasteboard].items = @[];
    EXPECT_TRUE(!dataForPasteboardType(kUTTypeUTF8PlainText).length);
    EXPECT_TRUE(!dataForPasteboardType(kUTTypeUTF16PlainText).length);

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:CGRectMake(0, 0, 320, 500)]);
    WKPreferences *preferences = [webView configuration].preferences;
    preferences._javaScriptCanAccessClipboard = YES;
    preferences._domPasteAllowed = YES;
    [webView synchronouslyLoadTestPageNamed:@"rich-and-plain-text"];
    return webView;
}

TEST(UIPasteboardTests, CopyPlainTextWritesConcreteTypes)
{
    auto webView = setUpWebViewForPasteboardTests();
    [webView stringByEvaluatingJavaScript:@"selectPlainText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('copy')"];

    auto utf8Result = adoptNS([[NSString alloc] initWithData:dataForPasteboardType(kUTTypeUTF8PlainText) encoding:NSUTF8StringEncoding]);
    auto utf16Result = adoptNS([[NSString alloc] initWithData:dataForPasteboardType(kUTTypeUTF16PlainText) encoding:NSUTF16StringEncoding]);
    EXPECT_WK_STREQ("Hello world", [utf8Result UTF8String]);
    EXPECT_WK_STREQ("Hello world", [utf16Result UTF8String]);
}

TEST(UIPasteboardTests, CopyRichTextWritesConcreteTypes)
{
    auto webView = setUpWebViewForPasteboardTests();
    [webView stringByEvaluatingJavaScript:@"selectRichText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('copy')"];

    auto utf8Result = adoptNS([[NSString alloc] initWithData:dataForPasteboardType(kUTTypeUTF8PlainText) encoding:NSUTF8StringEncoding]);
    auto utf16Result = adoptNS([[NSString alloc] initWithData:dataForPasteboardType(kUTTypeUTF16PlainText) encoding:NSUTF16StringEncoding]);
    EXPECT_WK_STREQ("Hello world", [utf8Result UTF8String]);
    EXPECT_WK_STREQ("Hello world", [utf16Result UTF8String]);
}

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000

TEST(UIPasteboardTests, DoNotPastePlainTextAsURL)
{
    auto webView = setUpWebViewForPasteboardTests();

    NSString *testString = @"[helloworld]";
    [UIPasteboard generalPasteboard].string = testString;

    [webView stringByEvaluatingJavaScript:@"selectPlainText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    EXPECT_WK_STREQ(testString, [webView stringByEvaluatingJavaScript:@"plain.value"]);

    [webView stringByEvaluatingJavaScript:@"selectRichText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    EXPECT_WK_STREQ(testString, [webView stringByEvaluatingJavaScript:@"rich.textContent"]);
    EXPECT_FALSE([webView stringByEvaluatingJavaScript:@"!!rich.querySelector('a')"].boolValue);
}

TEST(UIPasteboardTests, PastePlainTextAsURL)
{
    auto webView = setUpWebViewForPasteboardTests();

    NSString *testString = @"https://www.apple.com/iphone";
    [UIPasteboard generalPasteboard].string = testString;

    [webView stringByEvaluatingJavaScript:@"selectPlainText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    EXPECT_WK_STREQ(testString, [webView stringByEvaluatingJavaScript:@"plain.value"]);

    [webView stringByEvaluatingJavaScript:@"selectRichText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    EXPECT_WK_STREQ(testString, [webView stringByEvaluatingJavaScript:@"rich.textContent"]);
    EXPECT_TRUE([webView stringByEvaluatingJavaScript:@"!!rich.querySelector('a')"].boolValue);
}

TEST(UIPasteboardTests, PasteURLWithPlainTextAsURL)
{
    auto webView = setUpWebViewForPasteboardTests();

    NSString *testString = @"thisisdefinitelyaurl";
    [UIPasteboard generalPasteboard].URL = [NSURL URLWithString:testString];

    [webView stringByEvaluatingJavaScript:@"selectPlainText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    EXPECT_WK_STREQ(testString, [webView stringByEvaluatingJavaScript:@"plain.value"]);

    [webView stringByEvaluatingJavaScript:@"selectRichText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    EXPECT_WK_STREQ(testString, [webView stringByEvaluatingJavaScript:@"rich.textContent"]);
    EXPECT_TRUE([webView stringByEvaluatingJavaScript:@"!!rich.querySelector('a')"].boolValue);
}

#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000

} // namespace TestWebKitAPI

#endif
