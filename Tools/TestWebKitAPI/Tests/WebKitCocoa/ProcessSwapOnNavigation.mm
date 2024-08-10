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

#import "config.h"

#import "PlatformUtilities.h"
#import "Test.h"
#import <WebKit/WKInspector.h>
#import <WebKit/WKNavigationDelegatePrivate.h>
#import <WebKit/WKNavigationPrivate.h>
#import <WebKit/WKPreferencesPrivate.h>
#import <WebKit/WKProcessPoolPrivate.h>
#import <WebKit/WKUIDelegatePrivate.h>
#import <WebKit/WKURLSchemeHandler.h>
#import <WebKit/WKURLSchemeTaskPrivate.h>
#import <WebKit/WKWebViewConfigurationPrivate.h>
#import <WebKit/WKWebViewPrivate.h>
#import <WebKit/WKWebsiteDataStorePrivate.h>
#import <WebKit/WKWebsiteDataStoreRef.h>
#import <WebKit/WebKit.h>
#import <WebKit/_WKExperimentalFeature.h>
#import <WebKit/_WKProcessPoolConfiguration.h>
#import <WebKit/_WKWebsiteDataStoreConfiguration.h>
#import <WebKit/_WKWebsitePolicies.h>
#import <wtf/Deque.h>
#import <wtf/HashMap.h>
#import <wtf/HashSet.h>
#import <wtf/RetainPtr.h>
#import <wtf/Vector.h>
#import <wtf/text/StringHash.h>
#import <wtf/text/WTFString.h>

#if WK_API_ENABLED

@interface WKProcessPool ()
- (WKContextRef)_contextForTesting;
@end

static bool done;
static bool failed;
static bool didCreateWebView;
static int numberOfDecidePolicyCalls;

static RetainPtr<NSMutableArray> receivedMessages = adoptNS([@[] mutableCopy]);
bool didReceiveAlert;
static bool receivedMessage;
static bool serverRedirected;
static HashSet<pid_t> seenPIDs;
@interface PSONMessageHandler : NSObject <WKScriptMessageHandler>
@end

@implementation PSONMessageHandler
- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
    if ([message body])
        [receivedMessages addObject:[message body]];
    else
        [receivedMessages addObject:@""];

    receivedMessage = true;
    if ([message.webView _webProcessIdentifier])
        seenPIDs.add([message.webView _webProcessIdentifier]);
}
@end

@interface PSONNavigationDelegate : NSObject <WKNavigationDelegate> {
    @public WKNavigationActionPolicy navigationActionPolicyToUse;
}
@end

@implementation PSONNavigationDelegate

- (instancetype) init
{
    self = [super init];
    navigationActionPolicyToUse = WKNavigationActionPolicyAllow;
    return self;
}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
    seenPIDs.add([webView _webProcessIdentifier]);
    failed = true;
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    seenPIDs.add([webView _webProcessIdentifier]);
    done = true;
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
    ++numberOfDecidePolicyCalls;
    seenPIDs.add([webView _webProcessIdentifier]);
    decisionHandler(navigationActionPolicyToUse);
}

- (void)webView:(WKWebView *)webView didReceiveServerRedirectForProvisionalNavigation:(WKNavigation *)navigation
{
    seenPIDs.add([webView _webProcessIdentifier]);
    serverRedirected = true;
}

@end

static RetainPtr<WKWebView> createdWebView;

@interface PSONUIDelegate : NSObject <WKUIDelegatePrivate>
- (instancetype)initWithNavigationDelegate:(PSONNavigationDelegate *)navigationDelegate;
@end

@implementation PSONUIDelegate {
    RetainPtr<PSONNavigationDelegate> _navigationDelegate;
}

- (instancetype)initWithNavigationDelegate:(PSONNavigationDelegate *)navigationDelegate
{
    if (!(self = [super init]))
        return nil;

    _navigationDelegate = navigationDelegate;
    return self;
}

- (nullable WKWebView *)webView:(WKWebView *)webView createWebViewWithConfiguration:(WKWebViewConfiguration *)configuration forNavigationAction:(WKNavigationAction *)navigationAction windowFeatures:(WKWindowFeatures *)windowFeatures
{
    createdWebView = adoptNS([[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 800, 600) configuration:configuration]);
    [createdWebView setNavigationDelegate:_navigationDelegate.get()];
    didCreateWebView = true;
    return createdWebView.get();
}

- (void)webView:(WKWebView *)webView runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WKFrameInfo *)frame completionHandler:(void (^)())completionHandler
{
    didReceiveAlert = true;
    completionHandler();
}

@end

@interface PSONScheme : NSObject <WKURLSchemeHandler> {
    const char* _bytes;
    HashMap<String, String> _redirects;
    HashMap<String, RetainPtr<NSData *>> _dataMappings;
}
- (instancetype)initWithBytes:(const char*)bytes;
- (void)addRedirectFromURLString:(NSString *)sourceURLString toURLString:(NSString *)destinationURLString;
- (void)addMappingFromURLString:(NSString *)urlString toData:(const char*)data;
@end

@implementation PSONScheme

- (instancetype)initWithBytes:(const char*)bytes
{
    self = [super init];
    _bytes = bytes;
    return self;
}

- (void)addRedirectFromURLString:(NSString *)sourceURLString toURLString:(NSString *)destinationURLString
{
    _redirects.set(sourceURLString, destinationURLString);
}

- (void)addMappingFromURLString:(NSString *)urlString toData:(const char*)data
{
    _dataMappings.set(urlString, [NSData dataWithBytesNoCopy:(void*)data length:strlen(data) freeWhenDone:NO]);
}

- (void)webView:(WKWebView *)webView startURLSchemeTask:(id <WKURLSchemeTask>)task
{
    NSURL *finalURL = task.request.URL;
    auto target = _redirects.get(task.request.URL.absoluteString);
    if (!target.isEmpty()) {
        auto redirectResponse = adoptNS([[NSURLResponse alloc] initWithURL:task.request.URL MIMEType:nil expectedContentLength:0 textEncodingName:nil]);

        finalURL = [NSURL URLWithString:(NSString *)target];
        auto request = adoptNS([[NSURLRequest alloc] initWithURL:finalURL]);

        [(id<WKURLSchemeTaskPrivate>)task _didPerformRedirection:redirectResponse.get() newRequest:request.get()];
    }

    RetainPtr<NSURLResponse> response = adoptNS([[NSURLResponse alloc] initWithURL:finalURL MIMEType:@"text/html" expectedContentLength:1 textEncodingName:nil]);
    [task didReceiveResponse:response.get()];

    if (auto data = _dataMappings.get([finalURL absoluteString]))
        [task didReceiveData:data.get()];
    else if (_bytes) {
        RetainPtr<NSData> data = adoptNS([[NSData alloc] initWithBytesNoCopy:(void *)_bytes length:strlen(_bytes) freeWhenDone:NO]);
        [task didReceiveData:data.get()];
    } else
        [task didReceiveData:[@"Hello" dataUsingEncoding:NSUTF8StringEncoding]];

    [task didFinish];
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id <WKURLSchemeTask>)task
{
}

@end

static const char* testBytes = R"PSONRESOURCE(
<head>
<script>

function log(msg)
{
    window.webkit.messageHandlers.pson.postMessage(msg);
}

window.onload = function(evt) {
    if (window.history.state != "onloadCalled")
        setTimeout('window.history.replaceState("onloadCalled", "");', 0);
}

window.onpageshow = function(evt) {
    log("PageShow called. Persisted: " + evt.persisted + ", and window.history.state is: " + window.history.state);
}

</script>
</head>
)PSONRESOURCE";

#if PLATFORM(MAC)

static const char* windowOpenCrossOriginNoOpenerTestBytes = R"PSONRESOURCE(
<script>
window.onload = function() {
    window.open("pson2://host/main2.html", "_blank", "noopener");
}
</script>
)PSONRESOURCE";

static const char* windowOpenCrossOriginWithOpenerTestBytes = R"PSONRESOURCE(
<script>
window.onload = function() {
    window.open("pson2://host/main2.html");
}
</script>
)PSONRESOURCE";

static const char* windowOpenSameOriginNoOpenerTestBytes = R"PSONRESOURCE(
<script>
window.onload = function() {
    if (!opener)
        window.open("pson1://host/main2.html", "_blank", "noopener");
}
</script>
)PSONRESOURCE";

static const char* dummyBytes = R"PSONRESOURCE(
<body>TEST</body>
)PSONRESOURCE";

#endif // PLATFORM(MAC)

TEST(ProcessSwap, Basic)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    processPoolConfiguration.get().maximumPrewarmedProcessCount = 1;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON2"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid1 = [webView _webProcessIdentifier];

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main2.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid2 = [webView _webProcessIdentifier];

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main2.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid3 = [webView _webProcessIdentifier];

    EXPECT_EQ(pid1, pid2);
    EXPECT_FALSE(pid2 == pid3);

    // 3 loads, 3 decidePolicy calls (e.g. the load that did perform a process swap should not have generated an additional decidePolicy call)
    EXPECT_EQ(numberOfDecidePolicyCalls, 3);
}

TEST(ProcessSwap, Back)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler1 = adoptNS([[PSONScheme alloc] initWithBytes:testBytes]);
    RetainPtr<PSONScheme> handler2 = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler1.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler2.get() forURLScheme:@"PSON2"];

    RetainPtr<PSONMessageHandler> messageHandler = adoptNS([[PSONMessageHandler alloc] init]);
    [[webViewConfiguration userContentController] addScriptMessageHandler:messageHandler.get() name:@"pson"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid1 = [webView _webProcessIdentifier];

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main2.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid2 = [webView _webProcessIdentifier];

    [webView goBack];

    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid3 = [webView _webProcessIdentifier];

    // 3 loads, 3 decidePolicy calls (e.g. any load that performs a process swap should not have generated an
    // additional decidePolicy call as a result of the process swap)
    EXPECT_EQ(numberOfDecidePolicyCalls, 3);

    EXPECT_EQ([receivedMessages count], 2u);
    EXPECT_TRUE([receivedMessages.get()[0] isEqualToString:@"PageShow called. Persisted: false, and window.history.state is: null"]);
    EXPECT_TRUE([receivedMessages.get()[1] isEqualToString:@"PageShow called. Persisted: true, and window.history.state is: onloadCalled"]);

    EXPECT_EQ(2u, seenPIDs.size());

    EXPECT_FALSE(pid1 == pid2);
    EXPECT_FALSE(pid2 == pid3);
    EXPECT_TRUE(pid1 == pid3);
}

#if PLATFORM(MAC)

TEST(ProcessSwap, CrossOriginWindowOpenNoOpener)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler1 = adoptNS([[PSONScheme alloc] initWithBytes:windowOpenCrossOriginNoOpenerTestBytes]);
    RetainPtr<PSONScheme> handler2 = adoptNS([[PSONScheme alloc] initWithBytes:dummyBytes]);
    [webViewConfiguration setURLSchemeHandler:handler1.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler2.get() forURLScheme:@"PSON2"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    TestWebKitAPI::Util::run(&didCreateWebView);
    didCreateWebView = false;

    TestWebKitAPI::Util::run(&done);

    EXPECT_EQ(2, numberOfDecidePolicyCalls);

    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);
    auto pid2 = [createdWebView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);

    EXPECT_NE(pid1, pid2);
}

TEST(ProcessSwap, CrossOriginWindowOpenWithOpener)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    processPoolConfiguration.get().processSwapsOnWindowOpenWithOpener = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler1 = adoptNS([[PSONScheme alloc] initWithBytes:windowOpenCrossOriginWithOpenerTestBytes]);
    RetainPtr<PSONScheme> handler2 = adoptNS([[PSONScheme alloc] initWithBytes:dummyBytes]);
    [webViewConfiguration setURLSchemeHandler:handler1.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler2.get() forURLScheme:@"PSON2"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    TestWebKitAPI::Util::run(&didCreateWebView);
    didCreateWebView = false;

    TestWebKitAPI::Util::run(&done);

    EXPECT_EQ(2, numberOfDecidePolicyCalls);

    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);
    auto pid2 = [createdWebView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);

    EXPECT_NE(pid1, pid2);
}

TEST(ProcessSwap, SameOriginWindowOpenNoOpener)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:windowOpenSameOriginNoOpenerTestBytes]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    TestWebKitAPI::Util::run(&didCreateWebView);
    didCreateWebView = false;

    TestWebKitAPI::Util::run(&done);

    EXPECT_EQ(2, numberOfDecidePolicyCalls);

    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);
    auto pid2 = [createdWebView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);

    EXPECT_EQ(pid1, pid2);
}

#endif // PLATFORM(MAC)

TEST(ProcessSwap, ServerRedirectFromNewWebView)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] init]);
    [handler addRedirectFromURLString:@"pson://host/main1.html" toURLString:@"psonredirected://host/main1.html"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"pson"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&serverRedirected);
    serverRedirected = false;

    seenPIDs.add([webView _webProcessIdentifier]);

    TestWebKitAPI::Util::run(&done);
    done = false;

    seenPIDs.add([webView _webProcessIdentifier]);

    EXPECT_FALSE(serverRedirected);
    EXPECT_EQ(2, numberOfDecidePolicyCalls);
    EXPECT_EQ(1u, seenPIDs.size());
}

TEST(ProcessSwap, ServerRedirect)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler1 = adoptNS([[PSONScheme alloc] init]);
    [handler1 addRedirectFromURLString:@"pson://host/main1.html" toURLString:@"psonredirected://host/main1.html"];
    [webViewConfiguration setURLSchemeHandler:handler1.get() forURLScheme:@"pson"];
    RetainPtr<PSONScheme> handler2 = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler2.get() forURLScheme:@"originalload"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"originalload://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pidAfterFirstLoad = [webView _webProcessIdentifier];

    EXPECT_EQ(1, numberOfDecidePolicyCalls);
    EXPECT_EQ(1u, seenPIDs.size());
    EXPECT_TRUE(*seenPIDs.begin() == pidAfterFirstLoad);

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&serverRedirected);
    serverRedirected = false;

    seenPIDs.add([webView _webProcessIdentifier]);

    TestWebKitAPI::Util::run(&done);
    done = false;

    seenPIDs.add([webView _webProcessIdentifier]);

    EXPECT_FALSE(serverRedirected);
    EXPECT_EQ(3, numberOfDecidePolicyCalls);
    EXPECT_EQ(2u, seenPIDs.size());
}

TEST(ProcessSwap, ServerRedirect2)
{
    // This tests a load that *starts out* to the same origin as the previous load, but then redirects to a new origin.
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler1 = adoptNS([[PSONScheme alloc] init]);
    [handler1 addRedirectFromURLString:@"pson://host/main2.html" toURLString:@"psonredirected://host/main1.html"];
    [webViewConfiguration setURLSchemeHandler:handler1.get() forURLScheme:@"pson"];
    RetainPtr<PSONScheme> handler2 = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler2.get() forURLScheme:@"psonredirected"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pidAfterFirstLoad = [webView _webProcessIdentifier];

    EXPECT_FALSE(serverRedirected);
    EXPECT_EQ(1, numberOfDecidePolicyCalls);
    EXPECT_EQ(1u, seenPIDs.size());
    EXPECT_TRUE(*seenPIDs.begin() == pidAfterFirstLoad);

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main2.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&serverRedirected);
    serverRedirected = false;

    seenPIDs.add([webView _webProcessIdentifier]);

    TestWebKitAPI::Util::run(&done);
    done = false;

    seenPIDs.add([webView _webProcessIdentifier]);

    EXPECT_FALSE(serverRedirected);
    EXPECT_EQ(3, numberOfDecidePolicyCalls);
    EXPECT_EQ(2u, seenPIDs.size());
}

static const char* sessionStorageTestBytes = R"PSONRESOURCE(
<head>
<script>

function log(msg)
{
    window.webkit.messageHandlers.pson.postMessage(msg);
}

window.onload = function(evt) {
    log(sessionStorage.psonKey);
    sessionStorage.psonKey = "I exist!";
}

</script>
</head>
)PSONRESOURCE";

TEST(ProcessSwap, SessionStorage)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler1 = adoptNS([[PSONScheme alloc] initWithBytes:sessionStorageTestBytes]);
    auto handler2 = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler1.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler2.get() forURLScheme:@"PSON2"];

    auto messageHandler = adoptNS([[PSONMessageHandler alloc] init]);
    [[webViewConfiguration userContentController] addScriptMessageHandler:messageHandler.get() name:@"pson"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid1 = [webView _webProcessIdentifier];

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid2 = [webView _webProcessIdentifier];

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid3 = [webView _webProcessIdentifier];

    // Verify the web pages are in different processes
    EXPECT_NE(pid1, pid2);
    EXPECT_NE(pid1, pid3);
    EXPECT_NE(pid2, pid3);

    // Verify the sessionStorage values were as expected
    EXPECT_EQ([receivedMessages count], 2u);
    EXPECT_TRUE([receivedMessages.get()[0] isEqualToString:@""]);
    EXPECT_TRUE([receivedMessages.get()[1] isEqualToString:@"I exist!"]);
}

static const char* mainFramesOnlyMainFrame = R"PSONRESOURCE(
<script>
function loaded() {
    setTimeout('window.frames[0].location.href = "pson2://host2/main.html"', 0);
}
</script>
<body onload="loaded();">
Some text
<iframe src="pson1://host/iframe.html"></iframe>
</body>
)PSONRESOURCE";

static const char* mainFramesOnlySubframe = R"PSONRESOURCE(
Some content
)PSONRESOURCE";


static const char* mainFramesOnlySubframe2 = R"PSONRESOURCE(
<script>
    window.webkit.messageHandlers.pson.postMessage("Done");
</script>
)PSONRESOURCE";

TEST(ProcessSwap, MainFramesOnly)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler = adoptNS([[PSONScheme alloc] init]);
    [handler addMappingFromURLString:@"pson1://host/main.html" toData:mainFramesOnlyMainFrame];
    [handler addMappingFromURLString:@"pson1://host/iframe" toData:mainFramesOnlySubframe];
    [handler addMappingFromURLString:@"pson2://host2/main.html" toData:mainFramesOnlySubframe2];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON2"];

    auto messageHandler = adoptNS([[PSONMessageHandler alloc] init]);
    [[webViewConfiguration userContentController] addScriptMessageHandler:messageHandler.get() name:@"pson"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;

    EXPECT_EQ(1u, seenPIDs.size());
}

TEST(ProcessSwap, OnePreviousProcessRemains)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host1/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host2/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host3/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    // Navigations to 3 different domains, we expect to have seen 3 different PIDs
    EXPECT_EQ(3u, seenPIDs.size());

    // But only 2 of those processes should still be alive
    EXPECT_EQ(2u, [processPool _webProcessCountIgnoringPrewarmed]);
}

static const char* pageCache1Bytes = R"PSONRESOURCE(
<script>
window.addEventListener('pageshow', function(event) {
    if (event.persisted)
        window.webkit.messageHandlers.pson.postMessage("Was persisted");
});
</script>
)PSONRESOURCE";

TEST(ProcessSwap, PageCache1)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler = adoptNS([[PSONScheme alloc] init]);
    [handler addMappingFromURLString:@"pson1://host/main.html" toData:pageCache1Bytes];
    [handler addMappingFromURLString:@"pson2://host/main.html" toData:pageCache1Bytes];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON2"];

    auto messageHandler = adoptNS([[PSONMessageHandler alloc] init]);
    [[webViewConfiguration userContentController] addScriptMessageHandler:messageHandler.get() name:@"pson"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main.html"]];

    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pidAfterLoad1 = [webView _webProcessIdentifier];

    EXPECT_EQ(1u, [processPool _webProcessCountIgnoringPrewarmed]);

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main.html"]];

    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pidAfterLoad2 = [webView _webProcessIdentifier];

    EXPECT_EQ(2u, [processPool _webProcessCountIgnoringPrewarmed]);
    EXPECT_NE(pidAfterLoad1, pidAfterLoad2);

    [webView goBack];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pidAfterLoad3 = [webView _webProcessIdentifier];

    EXPECT_EQ(2u, [processPool _webProcessCountIgnoringPrewarmed]);
    EXPECT_EQ(pidAfterLoad1, pidAfterLoad3);
    EXPECT_EQ(1u, [receivedMessages count]);
    EXPECT_TRUE([receivedMessages.get()[0] isEqualToString:@"Was persisted" ]);
    EXPECT_EQ(2u, seenPIDs.size());

    [webView goForward];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pidAfterLoad4 = [webView _webProcessIdentifier];

    EXPECT_EQ(2u, [processPool _webProcessCountIgnoringPrewarmed]);
    EXPECT_EQ(pidAfterLoad2, pidAfterLoad4);
    EXPECT_EQ(2u, [receivedMessages count]);
    EXPECT_TRUE([receivedMessages.get()[1] isEqualToString:@"Was persisted" ]);
    EXPECT_EQ(2u, seenPIDs.size());
}

TEST(ProcessSwap, NumberOfPrewarmedProcesses)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    [processPoolConfiguration setMaximumPrewarmedProcessCount:1];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host1/main.html"]];
    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&done);
    done = false;

    EXPECT_EQ(2u, [processPool _webProcessCount]);
    EXPECT_EQ(1u, [processPool _webProcessCountIgnoringPrewarmed]);
    EXPECT_EQ(1u, [processPool _prewarmedWebProcessCount]);

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host3/main.html"]];
    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&done);
    done = false;

    EXPECT_EQ(3u, [processPool _webProcessCount]);
    EXPECT_EQ(2u, [processPool _webProcessCountIgnoringPrewarmed]);
    EXPECT_EQ(1u, [processPool _prewarmedWebProcessCount]);
}

static const char* visibilityBytes = R"PSONRESOURCE(
<script>
window.addEventListener('pageshow', function(event) {
    var msg = window.location.href + " - pageshow ";
    msg += event.persisted ? "persisted" : "NOT persisted";
    window.webkit.messageHandlers.pson.postMessage(msg);
});

window.addEventListener('pagehide', function(event) {
    var msg = window.location.href + " - pagehide ";
    msg += event.persisted ? "persisted" : "NOT persisted";
    window.webkit.messageHandlers.pson.postMessage(msg);
});
</script>
)PSONRESOURCE";

TEST(ProcessSwap, PageShowHide)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler = adoptNS([[PSONScheme alloc] init]);
    [handler addMappingFromURLString:@"pson1://host/main.html" toData:visibilityBytes];
    [handler addMappingFromURLString:@"pson2://host/main.html" toData:visibilityBytes];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON2"];

    auto messageHandler = adoptNS([[PSONMessageHandler alloc] init]);
    [[webViewConfiguration userContentController] addScriptMessageHandler:messageHandler.get() name:@"pson"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main.html"]];

    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main.html"]];

    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    [webView goBack];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    [webView goForward];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    EXPECT_EQ(7u, [receivedMessages count]);
    EXPECT_TRUE([receivedMessages.get()[0] isEqualToString:@"pson1://host/main.html - pageshow NOT persisted" ]);
    EXPECT_TRUE([receivedMessages.get()[1] isEqualToString:@"pson1://host/main.html - pagehide persisted" ]);
    EXPECT_TRUE([receivedMessages.get()[2] isEqualToString:@"pson2://host/main.html - pageshow NOT persisted" ]);
    EXPECT_TRUE([receivedMessages.get()[3] isEqualToString:@"pson2://host/main.html - pagehide persisted" ]);
    EXPECT_TRUE([receivedMessages.get()[4] isEqualToString:@"pson1://host/main.html - pageshow persisted" ]);
    EXPECT_TRUE([receivedMessages.get()[5] isEqualToString:@"pson1://host/main.html - pagehide persisted" ]);
    EXPECT_TRUE([receivedMessages.get()[6] isEqualToString:@"pson2://host/main.html - pageshow persisted" ]);
}

// Disabling the page cache explicitly is (for some reason) not available on iOS.
#if !TARGET_OS_IPHONE
static const char* loadUnloadBytes = R"PSONRESOURCE(
<script>
window.addEventListener('unload', function(event) {
    var msg = window.location.href + " - unload";
    window.webkit.messageHandlers.pson.postMessage(msg);
});

window.addEventListener('load', function(event) {
    var msg = window.location.href + " - load";
    window.webkit.messageHandlers.pson.postMessage(msg);
});
</script>
)PSONRESOURCE";

TEST(ProcessSwap, LoadUnload)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler = adoptNS([[PSONScheme alloc] init]);
    [handler addMappingFromURLString:@"pson1://host/main.html" toData:loadUnloadBytes];
    [handler addMappingFromURLString:@"pson2://host/main.html" toData:loadUnloadBytes];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON2"];

    auto messageHandler = adoptNS([[PSONMessageHandler alloc] init]);
    [[webViewConfiguration userContentController] addScriptMessageHandler:messageHandler.get() name:@"pson"];
    [[webViewConfiguration preferences] _setUsesPageCache:NO];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main.html"]];

    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main.html"]];

    [webView loadRequest:request];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    [webView goBack];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    [webView goForward];
    TestWebKitAPI::Util::run(&receivedMessage);
    receivedMessage = false;
    TestWebKitAPI::Util::run(&done);
    done = false;

    EXPECT_EQ(7u, [receivedMessages count]);
    EXPECT_TRUE([receivedMessages.get()[0] isEqualToString:@"pson1://host/main.html - load" ]);
    EXPECT_TRUE([receivedMessages.get()[1] isEqualToString:@"pson1://host/main.html - unload" ]);
    EXPECT_TRUE([receivedMessages.get()[2] isEqualToString:@"pson2://host/main.html - load" ]);
    EXPECT_TRUE([receivedMessages.get()[3] isEqualToString:@"pson2://host/main.html - unload" ]);
    EXPECT_TRUE([receivedMessages.get()[4] isEqualToString:@"pson1://host/main.html - load" ]);
    EXPECT_TRUE([receivedMessages.get()[5] isEqualToString:@"pson1://host/main.html - unload" ]);
    EXPECT_TRUE([receivedMessages.get()[6] isEqualToString:@"pson2://host/main.html - load" ]);
}

TEST(ProcessSwap, DisableForInspector)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    webViewConfiguration.get().preferences._developerExtrasEnabled = YES;

    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON2"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main1.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid1 = [webView _webProcessIdentifier];

    // FIXME: use ObjC equivalent for WKInspectorRef when available.
    WKInspectorShow(WKPageGetInspector([webView _pageRefForTransitionToWKWebView]));

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main2.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid2 = [webView _webProcessIdentifier];

    WKInspectorClose(WKPageGetInspector([webView _pageRefForTransitionToWKWebView]));

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main2.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid3 = [webView _webProcessIdentifier];

    EXPECT_EQ(pid1, pid2);
    EXPECT_FALSE(pid2 == pid3);
    EXPECT_EQ(numberOfDecidePolicyCalls, 3);
}

#endif // !TARGET_OS_IPHONE

static const char* sameOriginBlobNavigationTestBytes = R"PSONRESOURCE(
<!DOCTYPE html>
<html>
<body>
<p><a id="link">Click here</a></p>
<script>
const blob = new Blob(['<!DOCTYPE html><html><p>PASS</p></html>'], {type: 'text/html'});
link.href = URL.createObjectURL(blob);
</script>
)PSONRESOURCE";

TEST(ProcessSwap, SameOriginBlobNavigation)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:sameOriginBlobNavigationTestBytes]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]]];

    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);

    [webView _evaluateJavaScriptWithoutUserGesture:@"document.getElementById('link').click()" completionHandler: nil];

    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid2 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);
    EXPECT_EQ(2, numberOfDecidePolicyCalls);
    EXPECT_EQ(pid1, pid2);
}

TEST(ProcessSwap, CrossOriginBlobNavigation)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:sameOriginBlobNavigationTestBytes]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON1"];
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON2"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson1://host/main1.html"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);

    bool finishedRunningScript = false;
    String blobURL;
    [webView _evaluateJavaScriptWithoutUserGesture:@"document.getElementById('link').href" completionHandler: [&] (id result, NSError *error) {
        blobURL = String([NSString stringWithFormat:@"%@", result]);
        finishedRunningScript = true;
    }];
    TestWebKitAPI::Util::run(&finishedRunningScript);

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson2://host/main1.html"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid2 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);

    finishedRunningScript = false;
    String script = "document.getElementById('link').href = '" + blobURL + "'";
    [webView _evaluateJavaScriptWithoutUserGesture:(NSString *)script completionHandler: [&] (id result, NSError *error) {
        finishedRunningScript = true;
    }];
    TestWebKitAPI::Util::run(&finishedRunningScript);

    // This navigation will fail.
    [webView _evaluateJavaScriptWithoutUserGesture:@"document.getElementById('link').click()" completionHandler: [&] (id result, NSError *error) {
        done = true;
    }];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid3 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid3);

    EXPECT_EQ(2, numberOfDecidePolicyCalls);
    EXPECT_NE(pid1, pid2);
    EXPECT_EQ(pid2, pid3);
}

TEST(ProcessSwap, NavigateToAboutBlank)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:sameOriginBlobNavigationTestBytes]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"about:blank"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid2 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);

    EXPECT_EQ(2, numberOfDecidePolicyCalls);
    EXPECT_EQ(pid1, pid2);
}

TEST(ProcessSwap, NavigateToDataURL)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:sameOriginBlobNavigationTestBytes]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"data:text/plain,PASS"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid2 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);

    EXPECT_EQ(2, numberOfDecidePolicyCalls);
    EXPECT_EQ(pid1, pid2);
}

TEST(ProcessSwap, ProcessReuse)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    [processPoolConfiguration setProcessSwapsOnNavigation:YES];
    [processPoolConfiguration setAlwaysKeepAndReuseSwappedProcesses:YES];
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    auto handler = adoptNS([[PSONScheme alloc] init]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto delegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:delegate.get()];

    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host1/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid1 = [webView _webProcessIdentifier];

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host2/main.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid2 = [webView _webProcessIdentifier];

    request = [NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host1/main2.html"]];
    [webView loadRequest:request];

    TestWebKitAPI::Util::run(&done);
    done = false;

    auto pid3 = [webView _webProcessIdentifier];

    // Two process swaps have occurred, but we should only have ever seen 2 pids.
    EXPECT_EQ(2u, seenPIDs.size());
    EXPECT_NE(pid1, pid2);
    EXPECT_NE(pid2, pid3);
    EXPECT_EQ(pid1, pid3);
}

static const char* navigateToInvalidURLTestBytes = R"PSONRESOURCE(
<!DOCTYPE html>
<html>
<body onload="setTimeout(() => alert('DONE'), 0); location.href = 'http://A=a%B=b'">
)PSONRESOURCE";

TEST(ProcessSwap, NavigateToInvalidURL)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:navigateToInvalidURLTestBytes]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];
    auto uiDelegate = adoptNS([[PSONUIDelegate alloc] initWithNavigationDelegate:navigationDelegate.get()]);
    [webView setUIDelegate:uiDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid1 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid1);

    TestWebKitAPI::Util::run(&didReceiveAlert);
    didReceiveAlert = false;
    auto pid2 = [webView _webProcessIdentifier];
    EXPECT_TRUE(!!pid2);

    EXPECT_EQ(2, numberOfDecidePolicyCalls);
    EXPECT_EQ(pid1, pid2);
}

static const char* navigateToDataURLThenBackBytes = R"PSONRESOURCE(
<script>
onpageshow = function(event) {
    // Location changes need to happen outside the onload handler to generate history entries.
    setTimeout(function() {
      window.location.href = "data:text/html,<body onload='history.back()'></body>";
    }, 0);
}

</script>
)PSONRESOURCE";

TEST(ProcessSwap, NavigateToDataURLThenBack)
{
    auto processPoolConfiguration = adoptNS([[_WKProcessPoolConfiguration alloc] init]);
    processPoolConfiguration.get().processSwapsOnNavigation = YES;
    processPoolConfiguration.get().pageCacheEnabled = NO;
    auto processPool = adoptNS([[WKProcessPool alloc] _initWithConfiguration:processPoolConfiguration.get()]);

    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    [webViewConfiguration setProcessPool:processPool.get()];
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:navigateToDataURLThenBackBytes]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/main1.html"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid1 = [webView _webProcessIdentifier];

    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid2 = [webView _webProcessIdentifier];

    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid3 = [webView _webProcessIdentifier];

    EXPECT_EQ(3, numberOfDecidePolicyCalls);
    EXPECT_EQ(1u, seenPIDs.size());
    EXPECT_EQ(pid1, pid2);
    EXPECT_EQ(pid2, pid3);
}

TEST(ProcessSwap, APIControlledProcessSwapping)
{
    auto webViewConfiguration = adoptNS([[WKWebViewConfiguration alloc] init]);
    RetainPtr<PSONScheme> handler = adoptNS([[PSONScheme alloc] initWithBytes:"Hello World!"]);
    [webViewConfiguration setURLSchemeHandler:handler.get() forURLScheme:@"PSON"];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600) configuration:webViewConfiguration.get()]);
    auto navigationDelegate = adoptNS([[PSONNavigationDelegate alloc] init]);
    [webView setNavigationDelegate:navigationDelegate.get()];

    numberOfDecidePolicyCalls = 0;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/1"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid1 = [webView _webProcessIdentifier];

    // Navigating from the above URL to this URL normally should not process swap.
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/2"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid2 = [webView _webProcessIdentifier];

    EXPECT_EQ(1u, seenPIDs.size());
    EXPECT_EQ(pid1, pid2);

    // Navigating from the above URL to this URL normally should not process swap,
    // but we'll explicitly ask for a swap.
    navigationDelegate->navigationActionPolicyToUse = _WKNavigationActionPolicyAllowInNewProcess;
    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"pson://host/3"]]];
    TestWebKitAPI::Util::run(&done);
    done = false;
    auto pid3 = [webView _webProcessIdentifier];

    EXPECT_EQ(3, numberOfDecidePolicyCalls);
    EXPECT_EQ(2u, seenPIDs.size());
    EXPECT_NE(pid1, pid3);
}

#endif // WK_API_ENABLED
