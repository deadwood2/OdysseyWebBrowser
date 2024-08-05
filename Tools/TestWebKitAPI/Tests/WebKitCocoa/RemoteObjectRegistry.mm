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

#if WK_API_ENABLED

#import "PlatformUtilities.h"
#import "RemoteObjectRegistry.h"
#import "Test.h"
#import "WKWebViewConfigurationExtras.h"
#import <WebKit/WKProcessPoolPrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <WebKit/_WKRemoteObjectInterface.h>
#import <WebKit/_WKRemoteObjectRegistry.h>
#import <wtf/RefCounted.h>
#import <wtf/RetainPtr.h>

static bool isDone;

TEST(WebKit2, RemoteObjectRegistry)
{
    @autoreleasepool {
        NSString * const testPlugInClassName = @"RemoteObjectRegistryPlugIn";
        auto configuration = retainPtr([WKWebViewConfiguration _test_configurationWithTestPlugInClassName:testPlugInClassName]);
        auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectZero configuration:configuration.get()]);

        isDone = false;

        _WKRemoteObjectInterface *interface = remoteObjectInterface();
        id <RemoteObjectProtocol> object = [[webView _remoteObjectRegistry] remoteObjectProxyWithInterface:interface];

        [object sayHello:@"Hello, World!"];

        [webView evaluateJavaScript:@"helloString" completionHandler:^(id result, NSError *error) {
            EXPECT_TRUE([result isKindOfClass:[NSString class]]);
            EXPECT_WK_STREQ(result, @"Hello, World!");
            isDone = true;
        }];
        TestWebKitAPI::Util::run(&isDone);

        isDone = false;
        [object sayHello:@"Hello Again!" completionHandler:^(NSString *result) {
            EXPECT_TRUE([result isKindOfClass:[NSString class]]);
            EXPECT_WK_STREQ(result, @"Your string was 'Hello Again!'");
            isDone = true;
        }];
        TestWebKitAPI::Util::run(&isDone);

        isDone = false;
        [object selectionAndClickInformationForClickAtPoint:[NSValue valueWithPoint:NSMakePoint(12, 34)] completionHandler:^(NSDictionary *result) {
            EXPECT_TRUE([result isEqual:@{ @"URL": [NSURL URLWithString:@"http://www.webkit.org/"] }]);
            isDone = true;
        }];
        TestWebKitAPI::Util::run(&isDone);

        isDone = false;
        [object takeRange:NSMakeRange(345, 123) completionHandler:^(NSUInteger location, NSUInteger length) {
            EXPECT_EQ(345U, location);
            EXPECT_EQ(123U, length);
            isDone = true;
        }];
        TestWebKitAPI::Util::run(&isDone);

        isDone = false;
        [object takeSize:CGSizeMake(123.45, 678.91) completionHandler:^(CGFloat width, CGFloat height) {
            EXPECT_EQ(123.45, width);
            EXPECT_EQ(678.91, height);
            isDone = true;
        }];
        TestWebKitAPI::Util::run(&isDone);

        isDone = false;

        class DoneWhenDestroyed : public RefCounted<DoneWhenDestroyed> {
        public:
            ~DoneWhenDestroyed() { isDone = true; }
        };

        {
            RefPtr<DoneWhenDestroyed> doneWhenDestroyed = adoptRef(*new DoneWhenDestroyed);
            [object doNotCallCompletionHandler:[doneWhenDestroyed]() {
            }];
        }

        TestWebKitAPI::Util::run(&isDone);
    }
}

#endif // WK_API_ENABLED
