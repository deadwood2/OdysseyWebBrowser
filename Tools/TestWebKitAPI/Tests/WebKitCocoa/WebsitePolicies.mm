/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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
#import "TestWKWebView.h"
#import <WebKit/WKNavigationDelegatePrivate.h>
#import <WebKit/WKPagePrivate.h>
#import <WebKit/WKPreferencesRefPrivate.h>
#import <WebKit/WKUIDelegatePrivate.h>
#import <WebKit/WKURLSchemeTaskPrivate.h>
#import <WebKit/WKUserContentControllerPrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <WebKit/WKWebsiteDataStorePrivate.h>
#import <WebKit/_WKUserContentExtensionStorePrivate.h>
#import <WebKit/_WKWebsiteDataStoreConfiguration.h>
#import <WebKit/_WKWebsitePolicies.h>
#import <wtf/MainThread.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/WTFString.h>

#if PLATFORM(IOS)
#import <WebKit/WKWebViewConfigurationPrivate.h>
#endif

#if WK_API_ENABLED

@interface WKWebView ()
- (WKPageRef)_pageForTesting;
@end

static bool doneCompiling;
static bool receivedAlert;

#if PLATFORM(MAC)
static std::optional<_WKAutoplayEvent> receivedAutoplayEvent;
static std::optional<_WKAutoplayEventFlags> receivedAutoplayEventFlags;
#endif

static size_t alertCount;

@interface ContentBlockingWebsitePoliciesDelegate : NSObject <WKNavigationDelegate, WKUIDelegate>
@end

@implementation ContentBlockingWebsitePoliciesDelegate

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    // _webView:decidePolicyForNavigationAction:decisionHandler: should be called instead if it is implemented.
    EXPECT_TRUE(false);
    decisionHandler(WKNavigationActionPolicyAllow);
}

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler
{
    switch (alertCount++) {
    case 0:
        // Default behavior.
        EXPECT_STREQ("content blockers enabled", message.UTF8String);
        break;
    case 1:
        // After having set websitePolicies.contentBlockersEnabled to false.
        EXPECT_STREQ("content blockers disabled", message.UTF8String);
        break;
    case 2:
        // After having reloaded without content blockers.
        EXPECT_STREQ("content blockers disabled", message.UTF8String);
        break;
    default:
        EXPECT_TRUE(false);
    }
    receivedAlert = true;
    completionHandler();
}

- (void)_webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy, _WKWebsitePolicies *))decisionHandler
{
    _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
    switch (alertCount) {
    case 0:
        // Verify the content blockers behave correctly with the default behavior.
        break;
    case 1:
        {
            // Verify disabling content blockers works correctly.
            websitePolicies.contentBlockersEnabled = false;
            
            // Verify calling the decisionHandler asynchronously works correctly.
            auto decisionHandlerCopy = Block_copy(decisionHandler);
            callOnMainThread([decisionHandlerCopy, websitePolicies = RetainPtr<_WKWebsitePolicies>(websitePolicies)] {
                decisionHandlerCopy(WKNavigationActionPolicyAllow, websitePolicies.get());
                Block_release(decisionHandlerCopy);
            });
        }
        return;
    case 2:
        // Verify enabling content blockers has no effect when reloading without content blockers.
        websitePolicies.contentBlockersEnabled = true;
        break;
    default:
        EXPECT_TRUE(false);
    }
    decisionHandler(WKNavigationActionPolicyAllow, websitePolicies);
}

@end

TEST(WebKit, WebsitePoliciesContentBlockersEnabled)
{
    [[_WKUserContentExtensionStore defaultStore] _removeAllContentExtensions];

    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);

    doneCompiling = false;
    NSString* contentBlocker = @"[{\"action\":{\"type\":\"css-display-none\",\"selector\":\".hidden\"},\"trigger\":{\"url-filter\":\".*\"}}]";
    [[_WKUserContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"WebsitePoliciesTest" encodedContentExtension:contentBlocker completionHandler:^(_WKUserContentFilter *filter, NSError *error) {
        EXPECT_TRUE(error == nil);
        [[configuration userContentController] _addUserContentFilter:filter];
        doneCompiling = true;
    }];
    TestWebKitAPI::Util::run(&doneCompiling);

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);

    auto delegate = adoptNS([[ContentBlockingWebsitePoliciesDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"contentBlockerCheck" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    alertCount = 0;
    receivedAlert = false;
    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&receivedAlert);

    receivedAlert = false;
    [webView reload];
    TestWebKitAPI::Util::run(&receivedAlert);

    receivedAlert = false;
    [webView _reloadWithoutContentBlockers];
    TestWebKitAPI::Util::run(&receivedAlert);

    [[_WKUserContentExtensionStore defaultStore] _removeAllContentExtensions];
}

@interface AutoplayPoliciesDelegate : NSObject <WKNavigationDelegate, WKUIDelegatePrivate>
@property (nonatomic, copy) _WKWebsiteAutoplayPolicy(^autoplayPolicyForURL)(NSURL *);
@property (nonatomic, copy) _WKWebsiteAutoplayQuirk(^allowedAutoplayQuirksForURL)(NSURL *);
@end

@implementation AutoplayPoliciesDelegate

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    // _webView:decidePolicyForNavigationAction:decisionHandler: should be called instead if it is implemented.
    EXPECT_TRUE(false);
    decisionHandler(WKNavigationActionPolicyAllow);
}

- (void)_webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy, _WKWebsitePolicies *))decisionHandler
{
    _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
    if (_allowedAutoplayQuirksForURL)
        websitePolicies.allowedAutoplayQuirks = _allowedAutoplayQuirksForURL(navigationAction.request.URL);
    if (_autoplayPolicyForURL)
        websitePolicies.autoplayPolicy = _autoplayPolicyForURL(navigationAction.request.URL);
    decisionHandler(WKNavigationActionPolicyAllow, websitePolicies);
}

#if PLATFORM(MAC)
- (void)_webView:(WKWebView *)webView handleAutoplayEvent:(_WKAutoplayEvent)event withFlags:(_WKAutoplayEventFlags)flags
{
    receivedAutoplayEventFlags = flags;
    receivedAutoplayEvent = event;
}
#endif

@end

TEST(WebKit, WebsitePoliciesAutoplayEnabled)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);

#if PLATFORM(IOS)
    [configuration setAllowsInlineMediaPlayback:YES];
#endif

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);

    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *requestWithAudio = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    NSURLRequest *requestWithoutAudio = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-no-audio-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];

    // iOS does not support volume changes to media elements.
#if PLATFORM(MAC)
    NSURLRequest *requestWithoutVolume = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-zero-volume-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
#endif

    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyAllowWithoutSound;
    }];
    [webView loadRequest:requestWithAudio];
    [webView waitForMessage:@"did-not-play"];

    [webView loadRequest:requestWithoutAudio];
    [webView waitForMessage:@"autoplayed"];

#if PLATFORM(MAC)
    [webView loadRequest:requestWithoutVolume];
    [webView waitForMessage:@"autoplayed"];
#endif

    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyDeny;
    }];

#if PLATFORM(MAC)
    [webView loadRequest:requestWithoutVolume];
    [webView waitForMessage:@"did-not-play"];
#endif

    [webView loadRequest:requestWithAudio];
    [webView waitForMessage:@"did-not-play"];

    // Test updating website policies.
    _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
    websitePolicies.autoplayPolicy = _WKWebsiteAutoplayPolicyAllow;
    [webView _updateWebsitePolicies:websitePolicies];
    [webView stringByEvaluatingJavaScript:@"playVideo()"];
    [webView waitForMessage:@"autoplayed"];

    [webView loadRequest:requestWithoutAudio];
    [webView waitForMessage:@"did-not-play"];

    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyAllow;
    }];
    [webView loadRequest:requestWithAudio];
    [webView waitForMessage:@"autoplayed"];

    [webView loadRequest:requestWithoutAudio];
    [webView waitForMessage:@"autoplayed"];

#if PLATFORM(MAC)
    [webView loadRequest:requestWithoutVolume];
    [webView waitForMessage:@"autoplayed"];
#endif

    NSURLRequest *requestWithAudioInIFrame = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-check-in-iframe" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];

    // If the top-level document allows autoplay, any iframes should also autoplay.
    [delegate setAutoplayPolicyForURL:^(NSURL *url) {
        if ([url.lastPathComponent isEqualToString:@"autoplay-check-in-iframe.html"])
            return _WKWebsiteAutoplayPolicyAllow;
        return _WKWebsiteAutoplayPolicyDeny;
    }];

    [webView loadRequest:requestWithAudioInIFrame];
    [webView waitForMessage:@"autoplayed"];

    // If the top-level document disallows autoplay, any iframes should also not autoplay.
    [delegate setAutoplayPolicyForURL:^(NSURL *url) {
        if ([url.lastPathComponent isEqualToString:@"autoplay-check-in-iframe.html"])
            return _WKWebsiteAutoplayPolicyDeny;
        return _WKWebsiteAutoplayPolicyAllow;
    }];

    [webView loadRequest:requestWithAudioInIFrame];
    [webView waitForMessage:@"did-not-play"];
}

#if PLATFORM(MAC)
static void runUntilReceivesAutoplayEvent(WKAutoplayEvent event)
{
    while (!receivedAutoplayEvent || *receivedAutoplayEvent != event)
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, true);
}

TEST(WebKit, WebsitePoliciesPlayAfterPreventedAutoplay)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 336, 276) configuration:configuration.get()]);

    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyDeny;
    }];
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];

    NSPoint playButtonClickPoint = NSMakePoint(20, 256);

    receivedAutoplayEvent = std::nullopt;
    NSURLRequest *jsPlayRequest = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"js-play-with-controls" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:jsPlayRequest];
    [webView waitForMessage:@"loaded"];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidPreventFromAutoplaying);

    [webView mouseDownAtPoint:playButtonClickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:playButtonClickPoint];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidPlayMediaPreventedFromAutoplaying);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);

    receivedAutoplayEvent = std::nullopt;
    [webView loadHTMLString:@"" baseURL:nil];

    NSURLRequest *autoplayRequest = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-with-controls" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:autoplayRequest];
    [webView waitForMessage:@"loaded"];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidPreventFromAutoplaying);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);

    [webView mouseDownAtPoint:playButtonClickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:playButtonClickPoint];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidPlayMediaPreventedFromAutoplaying);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);

    receivedAutoplayEvent = std::nullopt;
    [webView loadHTMLString:@"" baseURL:nil];

    NSURLRequest *noAutoplayRequest = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"no-autoplay-with-controls" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:noAutoplayRequest];
    [webView waitForMessage:@"loaded"];
    [webView mouseDownAtPoint:playButtonClickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:playButtonClickPoint];
    [webView waitForMessage:@"played"];
    ASSERT_TRUE(receivedAutoplayEvent == std::nullopt);

    receivedAutoplayEvent = std::nullopt;
    [webView loadHTMLString:@"" baseURL:nil];

    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyAllowWithoutSound;
    }];

    NSURLRequest *autoplayMutedRequest = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-muted-with-controls" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:autoplayMutedRequest];
    [webView waitForMessage:@"loaded"];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidPreventFromAutoplaying);

    [webView mouseDownAtPoint:playButtonClickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:playButtonClickPoint];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidPlayMediaPreventedFromAutoplaying);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);
}

TEST(WebKit, WebsitePoliciesPlayingWithoutInterference)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 336, 276) configuration:configuration.get()]);

    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyAllow;
    }];
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];

    receivedAutoplayEvent = std::nullopt;
    NSURLRequest *jsPlayRequest = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"js-autoplay-audio" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:jsPlayRequest];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidAutoplayMediaPastThresholdWithoutUserInterference);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);
}

TEST(WebKit, WebsitePoliciesUserInterferenceWithPlaying)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 336, 276) configuration:configuration.get()]);

    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyAllow;
    }];
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];

    receivedAutoplayEvent = std::nullopt;
    NSURLRequest *jsPlayRequest = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"js-play-with-controls" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:jsPlayRequest];
    [webView waitForMessage:@"playing"];
    ASSERT_TRUE(receivedAutoplayEvent == std::nullopt);

    WKPageSetMuted([webView _pageForTesting], kWKMediaAudioMuted);
    runUntilReceivesAutoplayEvent(kWKAutoplayEventUserDidInterfereWithPlayback);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);

    receivedAutoplayEvent = std::nullopt;
    [webView loadRequest:jsPlayRequest];
    [webView waitForMessage:@"playing"];
    ASSERT_TRUE(receivedAutoplayEvent == std::nullopt);

    const NSPoint muteButtonClickPoint = NSMakePoint(80, 256);
    [webView mouseDownAtPoint:muteButtonClickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:muteButtonClickPoint];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventUserDidInterfereWithPlayback);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);

    receivedAutoplayEvent = std::nullopt;
    [webView loadRequest:jsPlayRequest];
    [webView waitForMessage:@"playing"];
    ASSERT_TRUE(receivedAutoplayEvent == std::nullopt);

    const NSPoint playButtonClickPoint = NSMakePoint(20, 256);
    [webView mouseDownAtPoint:playButtonClickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:playButtonClickPoint];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventUserDidInterfereWithPlayback);
    ASSERT_TRUE(*receivedAutoplayEventFlags & kWKAutoplayEventFlagsHasAudio);
}

struct ParsedRange {
    ParsedRange(String string)
    {
        // This is a strict and unsafe Range header parser adequate only for tests.
        bool parsingMin = true;
        size_t min = 0;
        size_t max = 0;
        ASSERT(string.length() > 6);
        ASSERT(string.startsWith("bytes="));
        for (size_t i = 6; i < string.length(); ++i) {
            if (isASCIIDigit(string[i])) {
                if (parsingMin)
                    min = min * 10 + string[i] - '0';
                else
                    max = max * 10 + string[i] - '0';
            } else if (string[i] == '-') {
                if (parsingMin)
                    parsingMin = false;
                else
                    return;
            } else
                return;
        }
        if (min <= max)
            range = std::make_pair(min, max);
    }
    std::optional<std::pair<size_t, size_t>> range;
};

@interface TestSchemeHandler : NSObject <WKURLSchemeHandler>
- (instancetype)initWithVideoData:(RetainPtr<NSData>&&)data;
@end

@implementation TestSchemeHandler {
    RetainPtr<NSData> videoData;
}

- (instancetype)initWithVideoData:(RetainPtr<NSData>&&)data
{
    self = [super init];
    if (!self)
        return nil;
    
    videoData = WTFMove(data);
    
    return self;
}

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id <WKURLSchemeTask>)task
{
    if ([task.request.URL.path isEqualToString:@"/should-redirect"]) {
        [(id<WKURLSchemeTaskPrivate>)task _didPerformRedirection:[[[NSURLResponse alloc] initWithURL:task.request.URL MIMEType:nil expectedContentLength:0 textEncodingName:nil] autorelease] newRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"test:///autoplay-check.html"]]];
        
        NSData *data = [NSData dataWithContentsOfURL:[[NSBundle mainBundle] URLForResource:@"autoplay-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
        [task didReceiveResponse:[[[NSURLResponse alloc] initWithURL:task.request.URL MIMEType:@"text/html" expectedContentLength:data.length textEncodingName:nil] autorelease]];
        [task didReceiveData:data];
        [task didFinish];
        return;
    }
    
    ASSERT_TRUE([task.request.URL.path isEqualToString:@"/test.mp4"]);
    ParsedRange parsedRange([task.request valueForHTTPHeaderField:@"Range"]);
    ASSERT_TRUE(!!parsedRange.range);
    auto& range = *parsedRange.range;
    
    NSDictionary *headerFields = @{ @"Content-Length": [@(range.second - range.first) stringValue], @"Content-Range": [NSString stringWithFormat:@"bytes %lu-%lu/%lu", range.first, range.second, [videoData length]] };
    NSURLResponse *response = [[[NSHTTPURLResponse alloc] initWithURL:task.request.URL statusCode:200 HTTPVersion:(NSString *)kCFHTTPVersion1_1 headerFields:headerFields] autorelease];
    [task didReceiveResponse:response];
    [task didReceiveData:[videoData subdataWithRange:NSMakeRange(range.first, range.second - range.first)]];
    [task didFinish];
    
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id <WKURLSchemeTask>)task
{
}

@end

TEST(WebKit, WebsitePoliciesDuringRedirect)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    auto videoData = adoptNS([[NSData alloc] initWithContentsOfURL:[[NSBundle mainBundle] URLForResource:@"test" withExtension:@"mp4" subdirectory:@"TestWebKitAPI.resources"]]);
    [configuration setURLSchemeHandler:[[[TestSchemeHandler alloc] initWithVideoData:WTFMove(videoData)] autorelease] forURLScheme:@"test"];
    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);
    
    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [delegate setAutoplayPolicyForURL:^(NSURL *url) {
        if ([url.path isEqualToString:@"/should-redirect"])
            return _WKWebsiteAutoplayPolicyDeny;
        return _WKWebsiteAutoplayPolicyAllow;
    }];
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];
    
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"test:///should-redirect"]]];
    [webView waitForMessage:@"autoplayed"];
}

TEST(WebKit, WebsitePoliciesUpdates)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);
    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];

    NSURLRequest *requestWithAudio = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];

    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyDeny;
    }];
    [webView loadRequest:requestWithAudio];
    [webView waitForMessage:@"did-not-play"];

    _WKWebsitePolicies *policies = [[[_WKWebsitePolicies alloc] init] autorelease];
    policies.autoplayPolicy = _WKWebsiteAutoplayPolicyAllow;
    [webView _updateWebsitePolicies:policies];

    // Now that we updated our policies, a script should be able to autoplay media.
    [webView stringByEvaluatingJavaScript:@"playVideo()"];
    [webView waitForMessage:@"autoplayed"];

    [webView stringByEvaluatingJavaScript:@"pauseVideo()"];

    policies = [[[_WKWebsitePolicies alloc] init] autorelease];
    policies.autoplayPolicy = _WKWebsiteAutoplayPolicyDeny;
    [webView _updateWebsitePolicies:policies];

    // A script should no longer be able to autoplay media.
    receivedAutoplayEvent = std::nullopt;
    [webView stringByEvaluatingJavaScript:@"playVideo()"];
    runUntilReceivesAutoplayEvent(kWKAutoplayEventDidPreventFromAutoplaying);
}

TEST(WebKit, WebsitePoliciesArbitraryUserGestureQuirk)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);

    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    WKRetainPtr<WKPreferencesRef> preferences(AdoptWK, WKPreferencesCreate());
    WKPreferencesSetNeedsSiteSpecificQuirks(preferences.get(), true);
    WKPageGroupSetPreferences(WKPageGetPageGroup([webView _pageForTesting]), preferences.get());

    [delegate setAllowedAutoplayQuirksForURL:^_WKWebsiteAutoplayQuirk(NSURL *url)
    {
        return _WKWebsiteAutoplayQuirkArbitraryUserGestures;
    }];
    [delegate setAutoplayPolicyForURL:^(NSURL *)
    {
        return _WKWebsiteAutoplayPolicyDeny;
    }];

    NSURLRequest *request = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:request];
    [webView waitForMessage:@"did-not-play"];

    const NSPoint clickPoint = NSMakePoint(760, 560);
    [webView mouseDownAtPoint:clickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:clickPoint];

    [webView stringByEvaluatingJavaScript:@"playVideo()"];
    [webView waitForMessage:@"autoplayed"];
}

TEST(WebKit, WebsitePoliciesAutoplayQuirks)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);

    auto delegate = adoptNS([[AutoplayPoliciesDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    WKRetainPtr<WKPreferencesRef> preferences(AdoptWK, WKPreferencesCreate());
    WKPreferencesSetNeedsSiteSpecificQuirks(preferences.get(), true);
    WKPageGroupSetPreferences(WKPageGetPageGroup([webView _pageForTesting]), preferences.get());

    NSURLRequest *requestWithAudio = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];

    [delegate setAllowedAutoplayQuirksForURL:^_WKWebsiteAutoplayQuirk(NSURL *url) {
        return _WKWebsiteAutoplayQuirkSynthesizedPauseEvents;
    }];
    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyDeny;
    }];
    [webView loadRequest:requestWithAudio];
    [webView waitForMessage:@"did-not-play"];
    [webView waitForMessage:@"on-pause"];

    receivedAutoplayEvent = std::nullopt;
    [webView loadHTMLString:@"" baseURL:nil];

    NSURLRequest *requestWithAudioInFrame = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-check-in-iframe" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];

    [delegate setAllowedAutoplayQuirksForURL:^_WKWebsiteAutoplayQuirk(NSURL *url) {
        if ([url.lastPathComponent isEqualToString:@"autoplay-check-frame.html"])
            return _WKWebsiteAutoplayQuirkInheritedUserGestures;
        
        return _WKWebsiteAutoplayQuirkSynthesizedPauseEvents | _WKWebsiteAutoplayQuirkInheritedUserGestures;
    }];
    [delegate setAutoplayPolicyForURL:^(NSURL *) {
        return _WKWebsiteAutoplayPolicyDeny;
    }];
    [webView loadRequest:requestWithAudioInFrame];
    [webView waitForMessage:@"did-not-play"];
    [webView waitForMessage:@"on-pause"];

    receivedAutoplayEvent = std::nullopt;
    [webView loadHTMLString:@"" baseURL:nil];

    NSURLRequest *requestThatInheritsGesture = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"autoplay-inherits-gesture-from-document" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];
    [webView loadRequest:requestThatInheritsGesture];
    [webView waitForMessage:@"loaded"];

    // Click in the document, but not in the media element.
    const NSPoint clickPoint = NSMakePoint(760, 560);
    [webView mouseDownAtPoint:clickPoint simulatePressure:NO];
    [webView mouseUpAtPoint:clickPoint];

    [webView stringByEvaluatingJavaScript:@"play()"];
    [webView waitForMessage:@"playing"];
}
#endif // PLATFORM(MAC)

TEST(WebKit, InvalidCustomHeaders)
{
    auto websitePolicies = adoptNS([[_WKWebsitePolicies alloc] init]);
    [websitePolicies setCustomHeaderFields:@{@"invalidheader" : @"", @"noncustom" : @"header", @"    x-Custom ":@"  Needs Canonicalization\t ", @"x-other" : @"other value"}];
    NSDictionary<NSString *, NSString *> *canonicalized = [websitePolicies customHeaderFields];
    EXPECT_EQ(canonicalized.count, 2u);
    EXPECT_STREQ([canonicalized objectForKey:@"x-Custom"].UTF8String, "Needs Canonicalization");
    EXPECT_STREQ([canonicalized objectForKey:@"x-other"].UTF8String, "other value");
}

static bool firstTestDone;
static bool secondTestDone;
static bool thirdTestDone;
static bool fourthTestDone;

static void expectHeaders(id <WKURLSchemeTask> task, bool expected)
{
    NSURLRequest *request = task.request;
    if (expected) {
        EXPECT_STREQ([[request valueForHTTPHeaderField:@"X-key1"] UTF8String], "value1");
        EXPECT_STREQ([[request valueForHTTPHeaderField:@"X-key2"] UTF8String], "value2");
    } else {
        EXPECT_TRUE([request valueForHTTPHeaderField:@"X-key1"] == nil);
        EXPECT_TRUE([request valueForHTTPHeaderField:@"X-key2"] == nil);
    }
}

static void respond(id <WKURLSchemeTask>task, NSString *html = nil)
{
    [task didReceiveResponse:[[[NSURLResponse alloc] initWithURL:task.request.URL MIMEType:@"text/html" expectedContentLength:html.length textEncodingName:nil] autorelease]];
    [task didReceiveData:[html dataUsingEncoding:NSUTF8StringEncoding]];
    [task didFinish];
}

@interface CustomHeaderFieldsDelegate : NSObject <WKNavigationDelegatePrivate, WKURLSchemeHandler>
@end

@implementation CustomHeaderFieldsDelegate

- (void)_webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy, _WKWebsitePolicies *))decisionHandler
{
    _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
    [websitePolicies setCustomHeaderFields:@{@"X-key1": @"value1", @"X-key2": @"value2"}];
    if ([navigationAction.request.URL.path isEqualToString:@"/mainresource"]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            decisionHandler(WKNavigationActionPolicyAllow, websitePolicies);
        });
    } else
        decisionHandler(WKNavigationActionPolicyAllow, websitePolicies);
}

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask
{
    NSString *path = urlSchemeTask.request.URL.path;
    if ([path isEqualToString:@"/mainresource"]) {
        expectHeaders(urlSchemeTask, true);
        respond(urlSchemeTask, @"<script>fetch('subresource').then(function(response){fetch('test://differentsecurityorigin/crossoriginsubresource',{mode:'no-cors'})})</script>");
    } else if ([path isEqualToString:@"/subresource"]) {
        expectHeaders(urlSchemeTask, true);
        respond(urlSchemeTask);
    } else if ([path isEqualToString:@"/crossoriginsubresource"]) {
        expectHeaders(urlSchemeTask, false);
        respond(urlSchemeTask);
        firstTestDone = true;
    } else if ([path isEqualToString:@"/mainresourcewithiframe"]) {
        expectHeaders(urlSchemeTask, true);
        respond(urlSchemeTask, @"<iframe src='test://iframeorigin/iframemainresource'></iframe>");
    } else if ([path isEqualToString:@"/iframemainresource"]) {
        expectHeaders(urlSchemeTask, false);
        respond(urlSchemeTask, @"<script>fetch('iframesubresource').then(function(response){fetch('test://mainframeorigin/originaloriginsubresource',{mode:'no-cors'})})</script>");
    } else if ([path isEqualToString:@"/iframesubresource"]) {
        expectHeaders(urlSchemeTask, false);
        respond(urlSchemeTask);
    } else if ([path isEqualToString:@"/originaloriginsubresource"]) {
        expectHeaders(urlSchemeTask, false);
        respond(urlSchemeTask);
        secondTestDone = true;
    } else if ([path isEqualToString:@"/nestedtop"]) {
        expectHeaders(urlSchemeTask, true);
        respond(urlSchemeTask, @"<iframe src='test://otherorigin/nestedmid'></iframe>");
    } else if ([path isEqualToString:@"/nestedmid"]) {
        expectHeaders(urlSchemeTask, false);
        respond(urlSchemeTask, @"<iframe src='test://toporigin/nestedbottom'></iframe>");
    } else if ([path isEqualToString:@"/nestedbottom"]) {
        expectHeaders(urlSchemeTask, true);
        respond(urlSchemeTask);
        thirdTestDone = true;
    } else if ([path isEqualToString:@"/createaboutblankiframe"]) {
        expectHeaders(urlSchemeTask, true);
        respond(urlSchemeTask, @"<script>start=()=>{var s = document.createElement('script');s.text=\"fetch('test:///requestfromaboutblank')\";document.getElementById('iframeid').contentWindow.document.body.appendChild(s);}</script><body><iframe src='about:blank' id=iframeid onload='start()'></iframe></body>");
    } else if ([path isEqualToString:@"/requestfromaboutblank"]) {
        expectHeaders(urlSchemeTask, true);
        respond(urlSchemeTask);
        fourthTestDone = true;
    } else
        EXPECT_TRUE(false);
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask
{
}

@end

TEST(WebKit, CustomHeaderFields)
{
    auto delegate = adoptNS([[CustomHeaderFieldsDelegate alloc] init]);
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [configuration setURLSchemeHandler:delegate.get() forURLScheme:@"test"];
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);
    [webView setNavigationDelegate:delegate.get()];
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"test:///mainresource"]]];
    TestWebKitAPI::Util::run(&firstTestDone);
    
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"test://mainframeorigin/mainresourcewithiframe"]]];
    TestWebKitAPI::Util::run(&secondTestDone);

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"test://toporigin/nestedtop"]]];
    TestWebKitAPI::Util::run(&thirdTestDone);

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"test:///createaboutblankiframe"]]];
    TestWebKitAPI::Util::run(&fourthTestDone);
    fourthTestDone = false;
    [webView reload];
    TestWebKitAPI::Util::run(&fourthTestDone);
}

@interface PopUpPoliciesDelegate : NSObject <WKNavigationDelegate, WKUIDelegatePrivate>
@property (nonatomic, copy) _WKWebsitePopUpPolicy(^popUpPolicyForURL)(NSURL *);
@end

@implementation PopUpPoliciesDelegate

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    // _webView:decidePolicyForNavigationAction:decisionHandler: should be called instead if it is implemented.
    EXPECT_TRUE(false);
    decisionHandler(WKNavigationActionPolicyAllow);
}

- (void)_webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy, _WKWebsitePolicies *))decisionHandler
{
    _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
    if (_popUpPolicyForURL)
        websitePolicies.popUpPolicy = _popUpPolicyForURL(navigationAction.request.URL);
    decisionHandler(WKNavigationActionPolicyAllow, websitePolicies);
}

- (nullable WKWebView *)webView:(WKWebView *)webView createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration forNavigationAction:(WKNavigationAction *)navigationAction windowFeatures:(WKWindowFeatures *)windowFeatures
{
    return [[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration];
}

@end

TEST(WebKit, WebsitePoliciesPopUp)
{
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);

    auto webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);

    auto delegate = adoptNS([[PopUpPoliciesDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"pop-up-check" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]];

    [delegate setPopUpPolicyForURL:^_WKWebsitePopUpPolicy(NSURL *) {
        return _WKWebsitePopUpPolicyBlock;
    }];

    [webView loadRequest:request];
    [webView waitForMessage:@"pop-up-blocked"];

    [delegate setPopUpPolicyForURL:^_WKWebsitePopUpPolicy(NSURL *) {
        return _WKWebsitePopUpPolicyAllow;
    }];

    [webView loadRequest:request];
    [webView waitForMessage:@"pop-up-allowed"];
}

static bool done;

@interface WebsitePoliciesWebsiteDataStoreDelegate : NSObject <WKNavigationDelegatePrivate, WKURLSchemeHandler, WKUIDelegate>
@end

@implementation WebsitePoliciesWebsiteDataStoreDelegate

- (void)_webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy, _WKWebsitePolicies *))decisionHandler
{
    NSURL *url = navigationAction.request.URL;
    if ([url.path isEqualToString:@"/invalid"]) {
        _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
        websitePolicies.websiteDataStore = [[[WKWebsiteDataStore alloc] _initWithConfiguration:[[[_WKWebsiteDataStoreConfiguration alloc] init] autorelease]] autorelease];

        bool sawException = false;
        @try {
            decisionHandler(WKNavigationActionPolicyAllow, websitePolicies);
        } @catch (NSException *exception) {
            sawException = true;
        }
        EXPECT_TRUE(sawException);

        done = true;
    }
    if ([url.path isEqualToString:@"/checkStorage"]
        || [url.path isEqualToString:@"/checkCookies"]
        || [url.path isEqualToString:@"/mainFrame"]) {
        _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
        websitePolicies.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
        decisionHandler(WKNavigationActionPolicyAllow, websitePolicies);
    }
    if ([url.path isEqualToString:@"/subFrame"]) {
        _WKWebsitePolicies *websitePolicies = [[[_WKWebsitePolicies alloc] init] autorelease];
        websitePolicies.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];
        bool sawException = false;
        @try {
            decisionHandler(WKNavigationActionPolicyCancel, websitePolicies);
        } @catch (NSException *exception) {
            sawException = true;
        }
        EXPECT_TRUE(sawException);
        done = true;
    }
}

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id <WKURLSchemeTask>)task
{
    NSURL *url = task.request.URL;
    if ([url.path isEqualToString:@"/checkStorage"]) {
        NSString *html = @"<script>var oldValue = window.sessionStorage['storageKey']; window.sessionStorage['storageKey'] = 'value'; alert('old value: <' + (oldValue ? 'fail' : '') + '>');</script>";
        [task didReceiveResponse:[[NSURLResponse alloc] initWithURL:url MIMEType:@"text/html" expectedContentLength:html.length textEncodingName:nil]];
        [task didReceiveData:[html dataUsingEncoding:NSUTF8StringEncoding]];
        [task didFinish];
    }
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id <WKURLSchemeTask>)urlSchemeTask
{
}

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)(void))completionHandler
{
    EXPECT_STREQ(message.UTF8String, "old value: <>");
    completionHandler();
    done = true;
}

@end

RetainPtr<WKWebView> websiteDataStoreTestWebView()
{
    auto delegate = adoptNS([[WebsitePoliciesWebsiteDataStoreDelegate alloc] init]);
    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [configuration setURLSchemeHandler:delegate.get() forURLScheme:@"test"];
    [configuration setWebsiteDataStore:[WKWebsiteDataStore nonPersistentDataStore]];
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:configuration.get()]);
    [webView setNavigationDelegate:delegate.get()];
    [webView setUIDelegate:delegate.get()];
    return webView;
}

TEST(WebKit, UpdateWebsitePoliciesInvalid)
{
    auto webView = websiteDataStoreTestWebView();
    auto policies = adoptNS([[_WKWebsitePolicies alloc] init]);
    [policies setWebsiteDataStore:[WKWebsiteDataStore nonPersistentDataStore]];
    bool sawException = false;
    @try {
        [webView _updateWebsitePolicies:policies.get()];
    } @catch (NSException *exception) {
        sawException = true;
    }
    EXPECT_TRUE(sawException);
    
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"test:///invalid"]]];
    TestWebKitAPI::Util::run(&done);
    
    done = false;
    [webView loadHTMLString:@"<iframe src='subFrame'></iframe>" baseURL:[NSURL URLWithString:@"http://webkit.org/mainFrame"]];
    TestWebKitAPI::Util::run(&done);
}

TEST(WebKit, WebsitePoliciesDataStore)
{
    auto cookieWebView = websiteDataStoreTestWebView();
    NSString *alertOldCookie = @"<script>var oldCookie = document.cookie; document.cookie = 'key=value'; alert('old value: <' + oldCookie + '>');</script>";
    [cookieWebView loadHTMLString:alertOldCookie baseURL:[NSURL URLWithString:@"http://example.com/checkCookies"]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    [cookieWebView loadHTMLString:alertOldCookie baseURL:[NSURL URLWithString:@"http://example.com/checkCookies"]];
    TestWebKitAPI::Util::run(&done);
}

#endif // WK_API_ENABLED
