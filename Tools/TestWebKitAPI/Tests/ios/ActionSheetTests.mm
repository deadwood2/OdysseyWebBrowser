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

#if PLATFORM(IOS)

#import "InstanceMethodSwizzler.h"
#import "PlatformUtilities.h"
#import "TestWKWebView.h"
#import "UIKitSPI.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import <WebKit/WKUIDelegatePrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <WebKit/_WKActivatedElementInfo.h>
#import <WebKit/_WKElementAction.h>
#import <wtf/BlockPtr.h>
#import <wtf/RetainPtr.h>
#import <wtf/SoftLinking.h>

@interface ActionSheetObserver : NSObject<WKUIDelegatePrivate>
@property (nonatomic) BlockPtr<NSArray *(_WKActivatedElementInfo *, NSArray *)> presentationHandler;
@property (nonatomic) BlockPtr<NSDictionary *(void)> dataDetectionContextHandler;
@end

@implementation ActionSheetObserver

- (NSArray *)_webView:(WKWebView *)webView actionsForElement:(_WKActivatedElementInfo *)element defaultActions:(NSArray<_WKElementAction *> *)defaultActions
{
    return _presentationHandler ? _presentationHandler(element, defaultActions) : defaultActions;
}

- (NSDictionary *)_dataDetectionContextForWebView:(WKWebView *)WebView
{
    return _dataDetectionContextHandler ? _dataDetectionContextHandler() : @{ };
}

@end

namespace TestWebKitAPI {

class IPadUserInterfaceSwizzler {
public:
    IPadUserInterfaceSwizzler()
        : m_swizzler([UIDevice class], @selector(userInterfaceIdiom), reinterpret_cast<IMP>(padUserInterfaceIdiom))
    {
    }
private:
    static UIUserInterfaceIdiom padUserInterfaceIdiom()
    {
        return UIUserInterfaceIdiomPad;
    }
    InstanceMethodSwizzler m_swizzler;
};

TEST(ActionSheetTests, ImageMapDoesNotDestroySelection)
{
    IPadUserInterfaceSwizzler iPadUserInterface;

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:CGRectMake(0, 0, 1024, 768)]);
    auto observer = adoptNS([[ActionSheetObserver alloc] init]);
    [webView setUIDelegate:observer.get()];
    [webView synchronouslyLoadTestPageNamed:@"image-map"];
    [webView stringByEvaluatingJavaScript:@"selectTextNode(h1.childNodes[0])"];

    EXPECT_WK_STREQ("Hello world", [webView stringByEvaluatingJavaScript:@"getSelection().toString()"]);

    __block bool done = false;
    [observer setPresentationHandler:^(_WKActivatedElementInfo *element, NSArray *actions) {
        done = true;
        return actions;
    }];
    [webView _simulateLongPressActionAtLocation:CGPointMake(200, 200)];
    TestWebKitAPI::Util::run(&done);

    EXPECT_WK_STREQ("Hello world", [webView stringByEvaluatingJavaScript:@"getSelection().toString()"]);
}

TEST(ActionSheetTests, DataDetectorsLinkIsNotPresentedAsALink)
{
    IPadUserInterfaceSwizzler iPadUserInterface;

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:CGRectMake(0, 0, 1024, 768)]);
    auto observer = adoptNS([[ActionSheetObserver alloc] init]);
    [webView setUIDelegate:observer.get()];

    auto runTest = ^(NSString *phoneNumber) {
        [webView synchronouslyLoadHTMLString:[NSString stringWithFormat:@"<a style='position: absolute; top: 0; left: 0;' href='tel:%@'>telephone number</a>", phoneNumber]];

        __block bool done = false;
        __block bool succeeded = true;

        // We shouldn't present a normal action sheet, but instead a data detectors sheet.
        [observer setDataDetectionContextHandler:^{
            done = true;
            return @{ };
        }];
        [observer setPresentationHandler:^(_WKActivatedElementInfo *, NSArray *) {
            done = true;
            succeeded = false;
            return @[ ];
        }];
        [webView _simulateLongPressActionAtLocation:CGPointMake(5, 5)];
        TestWebKitAPI::Util::run(&done);

        return succeeded;
    };

    EXPECT_TRUE(runTest(@"4089961010"));
    EXPECT_TRUE(runTest(@"408 996 1010"));
    EXPECT_TRUE(runTest(@"1-866-MY-APPLE"));
    EXPECT_TRUE(runTest(@"(123) 456 - 7890"));
    EXPECT_TRUE(runTest(@"01 23 45 67 00"));
    EXPECT_TRUE(runTest(@"+33 (0)1 23 34 45 56"));
    EXPECT_TRUE(runTest(@"(0)1 12 23 34 56"));
    EXPECT_TRUE(runTest(@"010-1-800-MY-APPLE"));
    EXPECT_TRUE(runTest(@"+1-408-1234567"));
    EXPECT_TRUE(runTest(@"０８０８０８０８０８０"));
}

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000

static void presentActionSheetAndChooseAction(WKWebView *webView, ActionSheetObserver *observer, CGPoint location, _WKElementActionType actionType)
{
    __block RetainPtr<_WKElementAction> copyAction;
    __block RetainPtr<_WKActivatedElementInfo> copyElement;
    __block bool done = false;
    [observer setPresentationHandler:^(_WKActivatedElementInfo *element, NSArray *actions) {
        copyElement = element;
        for (_WKElementAction *action in actions) {
            if (action.type == actionType)
                copyAction = action;
        }
        done = true;
        return @[ copyAction.get() ];
    }];
    [webView _simulateLongPressActionAtLocation:location];
    TestWebKitAPI::Util::run(&done);

    EXPECT_TRUE(!!copyAction);
    EXPECT_TRUE(!!copyElement);
    [copyAction runActionWithElementInfo:copyElement.get()];
}

TEST(ActionSheetTests, CopyImageElementWithHREFAndTitle)
{
    UIApplicationInitialize();
    [UIPasteboard generalPasteboard].items = @[ ];

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:CGRectMake(0, 0, 320, 500)]);
    auto observer = adoptNS([[ActionSheetObserver alloc] init]);
    [webView setUIDelegate:observer.get()];
    [webView synchronouslyLoadTestPageNamed:@"image-in-link-and-input"];
    [webView stringByEvaluatingJavaScript:@"document.querySelector('a').setAttribute('title', 'hello world')"];

    presentActionSheetAndChooseAction(webView.get(), observer.get(), CGPointMake(100, 50), _WKElementActionTypeCopy);

    __block bool done = false;
    __block RetainPtr<NSItemProvider> itemProvider;
    [webView _doAfterNextPresentationUpdate:^() {
        NSArray <NSString *> *pasteboardTypes = [[UIPasteboard generalPasteboard] pasteboardTypes];
        EXPECT_EQ(2UL, pasteboardTypes.count);
        EXPECT_WK_STREQ((NSString *)kUTTypePNG, pasteboardTypes.firstObject);
        EXPECT_WK_STREQ((NSString *)kUTTypeURL, pasteboardTypes.lastObject);
        NSArray <NSItemProvider *> *itemProviders = [[UIPasteboard generalPasteboard] itemProviders];
        EXPECT_EQ(1UL, itemProviders.count);
        itemProvider = itemProviders.firstObject;
        EXPECT_EQ(2UL, [itemProvider registeredTypeIdentifiers].count);
        EXPECT_WK_STREQ((NSString *)kUTTypePNG, [itemProvider registeredTypeIdentifiers].firstObject);
        EXPECT_WK_STREQ((NSString *)kUTTypeURL, [itemProvider registeredTypeIdentifiers].lastObject);
        done = true;
    }];
    TestWebKitAPI::Util::run(&done);

    __block bool doneLoading = false;
    [itemProvider loadObjectOfClass:[NSURL class] completionHandler:^(id <NSItemProviderReading> result, NSError *) {
        EXPECT_TRUE([result isKindOfClass:[NSURL class]]);
        NSURL *url = (NSURL *)result;
        EXPECT_WK_STREQ("https://www.apple.com/", url.absoluteString);
        EXPECT_WK_STREQ("hello world", url._title);
        doneLoading = true;
    }];
    TestWebKitAPI::Util::run(&doneLoading);
}

TEST(ActionSheetTests, CopyImageElementWithHREF)
{
    UIApplicationInitialize();
    [UIPasteboard generalPasteboard].items = @[ ];

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:CGRectMake(0, 0, 320, 500)]);
    auto observer = adoptNS([[ActionSheetObserver alloc] init]);
    [webView setUIDelegate:observer.get()];
    [webView synchronouslyLoadTestPageNamed:@"image-in-link-and-input"];

    presentActionSheetAndChooseAction(webView.get(), observer.get(), CGPointMake(100, 50), _WKElementActionTypeCopy);

    __block bool done = false;
    __block RetainPtr<NSItemProvider> itemProvider;
    [webView _doAfterNextPresentationUpdate:^() {
        NSArray <NSString *> *pasteboardTypes = [[UIPasteboard generalPasteboard] pasteboardTypes];
        EXPECT_EQ(2UL, pasteboardTypes.count);
        EXPECT_WK_STREQ((NSString *)kUTTypePNG, pasteboardTypes.firstObject);
        EXPECT_WK_STREQ((NSString *)kUTTypeURL, pasteboardTypes.lastObject);
        NSArray <NSItemProvider *> *itemProviders = [[UIPasteboard generalPasteboard] itemProviders];
        EXPECT_EQ(1UL, itemProviders.count);
        itemProvider = itemProviders.firstObject;
        EXPECT_EQ(2UL, [itemProvider registeredTypeIdentifiers].count);
        EXPECT_WK_STREQ((NSString *)kUTTypePNG, [itemProvider registeredTypeIdentifiers].firstObject);
        EXPECT_WK_STREQ((NSString *)kUTTypeURL, [itemProvider registeredTypeIdentifiers].lastObject);
        done = true;
    }];
    TestWebKitAPI::Util::run(&done);

    __block bool doneLoading = false;
    [itemProvider loadObjectOfClass:[NSURL class] completionHandler:^(id <NSItemProviderReading> result, NSError *) {
        EXPECT_TRUE([result isKindOfClass:[NSURL class]]);
        NSURL *url = (NSURL *)result;
        EXPECT_WK_STREQ("https://www.apple.com/", url.absoluteString);
        EXPECT_WK_STREQ("https://www.apple.com/", url._title);
        doneLoading = true;
    }];
    TestWebKitAPI::Util::run(&doneLoading);
}

TEST(ActionSheetTests, CopyImageElementWithoutHREF)
{
    UIApplicationInitialize();
    [UIPasteboard generalPasteboard].items = @[ ];

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:CGRectMake(0, 0, 320, 500)]);
    auto observer = adoptNS([[ActionSheetObserver alloc] init]);
    [webView setUIDelegate:observer.get()];
    [webView synchronouslyLoadTestPageNamed:@"image-and-contenteditable"];

    presentActionSheetAndChooseAction(webView.get(), observer.get(), CGPointMake(100, 100), _WKElementActionTypeCopy);

    __block bool done = false;
    [webView _doAfterNextPresentationUpdate:^() {
        NSArray <NSString *> *pasteboardTypes = [[UIPasteboard generalPasteboard] pasteboardTypes];
        EXPECT_EQ(1UL, pasteboardTypes.count);
        EXPECT_WK_STREQ((NSString *)kUTTypePNG, pasteboardTypes.firstObject);
        NSArray <NSItemProvider *> *itemProviders = [[UIPasteboard generalPasteboard] itemProviders];
        EXPECT_EQ(1UL, itemProviders.count);
        NSItemProvider *itemProvider = itemProviders.firstObject;
        EXPECT_EQ(1UL, itemProvider.registeredTypeIdentifiers.count);
        EXPECT_WK_STREQ((NSString *)kUTTypePNG, itemProvider.registeredTypeIdentifiers.firstObject);
        done = true;
    }];
    TestWebKitAPI::Util::run(&done);
}

#endif // __IPHONE_OS_VERSION_MIN_REQUIRED >= 110000

} // namespace TestWebKitAPI

#endif // PLATFORM(IOS)
