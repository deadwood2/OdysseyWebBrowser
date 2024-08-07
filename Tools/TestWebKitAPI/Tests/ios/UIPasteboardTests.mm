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
#import "UIKitSPI.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import <UIKit/UIPasteboard.h>
#import <WebKit/WKPreferencesPrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <wtf/SoftLinking.h>

typedef void (^DataLoadCompletionBlock)(NSData *, NSError *);

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110300

static void checkJSONWithLogging(NSString *jsonString, NSDictionary *expected)
{
    BOOL success = TestWebKitAPI::Util::jsonMatchesExpectedValues(jsonString, expected);
    EXPECT_TRUE(success);
    if (!success)
        NSLog(@"Expected JSON: %@ to match values: %@", jsonString, expected);
}

#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 110300

namespace TestWebKitAPI {

NSData *dataForPasteboardType(CFStringRef type)
{
    return [[UIPasteboard generalPasteboard] dataForPasteboardType:(NSString *)type inItemSet:[NSIndexSet indexSetWithIndex:0]].firstObject;
}

RetainPtr<TestWKWebView> setUpWebViewForPasteboardTests(NSString *pageName)
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
    [webView synchronouslyLoadTestPageNamed:pageName];
    return webView;
}

TEST(UIPasteboardTests, CopyPlainTextWritesConcreteTypes)
{
    auto webView = setUpWebViewForPasteboardTests(@"rich-and-plain-text");
    [webView stringByEvaluatingJavaScript:@"selectPlainText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('copy')"];

    auto utf8Result = adoptNS([[NSString alloc] initWithData:dataForPasteboardType(kUTTypeUTF8PlainText) encoding:NSUTF8StringEncoding]);
    EXPECT_WK_STREQ("Hello world", [utf8Result UTF8String]);
}

TEST(UIPasteboardTests, CopyRichTextWritesConcreteTypes)
{
    auto webView = setUpWebViewForPasteboardTests(@"rich-and-plain-text");
    [webView stringByEvaluatingJavaScript:@"selectRichText()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('copy')"];

    auto utf8Result = adoptNS([[NSString alloc] initWithData:dataForPasteboardType(kUTTypeUTF8PlainText) encoding:NSUTF8StringEncoding]);
    EXPECT_WK_STREQ("Hello world", [utf8Result UTF8String]);
}

TEST(UIPasteboardTests, DoNotPastePlainTextAsURL)
{
    auto webView = setUpWebViewForPasteboardTests(@"rich-and-plain-text");

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
    auto webView = setUpWebViewForPasteboardTests(@"rich-and-plain-text");

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
    auto webView = setUpWebViewForPasteboardTests(@"rich-and-plain-text");

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

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110300

TEST(UIPasteboardTests, DataTransferGetDataWhenPastingURL)
{
    auto webView = setUpWebViewForPasteboardTests(@"dump-datatransfer-types");

    NSString *testURLString = @"https://www.apple.com/";
    [UIPasteboard generalPasteboard].URL = [NSURL URLWithString:testURLString];

    [webView stringByEvaluatingJavaScript:@"destination.focus()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    checkJSONWithLogging([webView stringByEvaluatingJavaScript:@"output.value"], @{
        @"paste" : @{
            @"text/plain" : testURLString,
            @"text/uri-list" : testURLString
        }
    });
}

TEST(UIPasteboardTests, DataTransferGetDataWhenPastingPlatformRepresentations)
{
    auto webView = setUpWebViewForPasteboardTests(@"dump-datatransfer-types");
    [webView stringByEvaluatingJavaScript:@"ignoreInlineStyles = true"];

    // This simulates how a native app on iOS might write to the pasteboard when copying.
    RetainPtr<NSURL> testURL = [NSURL URLWithString:@"https://www.apple.com/"];
    RetainPtr<NSString> testPlainTextString = @"WebKit";
    RetainPtr<NSString> testMarkupString = @"<a href=\"https://www.webkit.org/\">The WebKit Project</a>";
    auto itemProvider = adoptNS([[NSItemProvider alloc] init]);
    [itemProvider registerDataRepresentationForTypeIdentifier:(NSString *)kUTTypeHTML visibility:NSItemProviderRepresentationVisibilityAll loadHandler:^NSProgress *(DataLoadCompletionBlock completionHandler)
    {
        completionHandler([testMarkupString dataUsingEncoding:NSUTF8StringEncoding], nil);
        return nil;
    }];
    [itemProvider registerObject:testURL.get() visibility:NSItemProviderRepresentationVisibilityAll];
    [itemProvider registerObject:testPlainTextString.get() visibility:NSItemProviderRepresentationVisibilityAll];

    [UIPasteboard generalPasteboard].itemProviders = @[ itemProvider.get() ];
    [webView stringByEvaluatingJavaScript:@"destination.focus()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    checkJSONWithLogging([webView stringByEvaluatingJavaScript:@"output.value"], @{
        @"paste" : @{
            @"text/plain" : testPlainTextString.get(),
            @"text/uri-list" : [testURL absoluteString],
            @"text/html" : testMarkupString.get()
        }
    });
}

TEST(UIPasteboardTests, DataTransferSetDataCannotWritePlatformTypes)
{
    auto webView = setUpWebViewForPasteboardTests(@"dump-datatransfer-types");

    [webView stringByEvaluatingJavaScript:@"customData = { 'text/plain' : '年年年', '年年年年年年' : 'test', 'com.adobe.pdf' : '🔥🔥🔥', 'text/rtf' : 'not actually rich text!' }"];
    [webView stringByEvaluatingJavaScript:@"writeCustomData = true"];
    [webView stringByEvaluatingJavaScript:@"select(rich)"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('copy')"];
    [webView stringByEvaluatingJavaScript:@"destination.focus()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    checkJSONWithLogging([webView stringByEvaluatingJavaScript:@"output.value"], @{
        @"paste": @{
            @"text/plain": @"年年年",
            @"年年年年年年": @"test",
            @"com.adobe.pdf": @"🔥🔥🔥",
            @"text/rtf": @"not actually rich text!"
        }
    });

    // Most importantly, the system pasteboard should not contain the PDF UTI.
    NSData *pdfData = [[UIPasteboard generalPasteboard] dataForPasteboardType:(NSString *)kUTTypePDF];
    EXPECT_EQ(0UL, pdfData.length);

    // However, the system pasteboard should contain a plain text string.
    EXPECT_WK_STREQ("年年年", [UIPasteboard generalPasteboard].string);
}

TEST(UIPasteboardTests, DataTransferGetDataCannotReadArbitraryPlatformTypes)
{
    auto webView = setUpWebViewForPasteboardTests(@"dump-datatransfer-types");
    auto itemProvider = adoptNS([[NSItemProvider alloc] init]);
    [itemProvider registerDataRepresentationForTypeIdentifier:(NSString *)kUTTypeMP3 visibility:NSItemProviderRepresentationVisibilityAll loadHandler:^NSProgress *(DataLoadCompletionBlock completionHandler)
    {
        completionHandler([@"this is a test" dataUsingEncoding:NSUTF8StringEncoding], nil);
        return nil;
    }];
    [itemProvider registerDataRepresentationForTypeIdentifier:@"org.WebKit.TestWebKitAPI.custom-pasteboard-type" visibility:NSItemProviderRepresentationVisibilityAll loadHandler:^NSProgress *(DataLoadCompletionBlock completionHandler)
    {
        completionHandler([@"this is another test" dataUsingEncoding:NSUTF8StringEncoding], nil);
        return nil;
    }];

    [webView stringByEvaluatingJavaScript:@"destination.focus()"];
    [webView stringByEvaluatingJavaScript:@"document.execCommand('paste')"];
    checkJSONWithLogging([webView stringByEvaluatingJavaScript:@"output.value"], @{
        @"paste": @{ }
    });
}

#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 110300

} // namespace TestWebKitAPI

#endif
