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

#import "config.h"
#import "TestController.h"

#import "PlatformWebView.h"
#import "TestInvocation.h"
#import <Foundation/Foundation.h>
#import <WebKit/WKPreferencesRefPrivate.h>
#import <WebKit/WKProcessPoolPrivate.h>
#import <WebKit/WKStringCF.h>
#import <WebKit/WKUserContentControllerPrivate.h>
#import <WebKit/WKWebView.h>
#import <WebKit/WKWebViewConfiguration.h>
#import <WebKit/WKWebViewConfigurationPrivate.h>
#import <wtf/MainThread.h>

namespace WTR {

void TestController::notifyDone()
{
}

void TestController::platformInitialize()
{
    NSString *identifier = [[NSBundle mainBundle] bundleIdentifier];
    const char *stdinPath = [[NSString stringWithFormat:@"/tmp/%@_IN", identifier] UTF8String];
    const char *stdoutPath = [[NSString stringWithFormat:@"/tmp/%@_OUT", identifier] UTF8String];
    const char *stderrPath = [[NSString stringWithFormat:@"/tmp/%@_ERROR", identifier] UTF8String];

    int infd = open(stdinPath, O_RDWR);
    dup2(infd, STDIN_FILENO);
    int outfd = open(stdoutPath, O_RDWR);
    dup2(outfd, STDOUT_FILENO);
    int errfd = open(stderrPath, O_RDWR | O_NONBLOCK);
    dup2(errfd, STDERR_FILENO);
}

void TestController::platformDestroy()
{
}

void TestController::initializeInjectedBundlePath()
{
    NSString *nsBundlePath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"WebKitTestRunnerInjectedBundle.bundle"];
    m_injectedBundlePath.adopt(WKStringCreateWithCFString((CFStringRef)nsBundlePath));
}

void TestController::initializeTestPluginDirectory()
{
    m_testPluginDirectory.adopt(WKStringCreateWithCFString((CFStringRef)[[NSBundle mainBundle] bundlePath]));
}

void TestController::platformResetPreferencesToConsistentValues()
{
    WKPreferencesRef preferences = platformPreferences();
    // Note that WKPreferencesSetTextAutosizingEnabled has no effect on iOS.
    WKPreferencesSetMinimumZoomFontSize(preferences, 0);
}

void TestController::platformResetStateToConsistentValues()
{
    cocoaResetStateToConsistentValues();
}

void TestController::platformConfigureViewForTest(const TestInvocation& test)
{
    if (test.options().useFlexibleViewport) {
        const unsigned phoneViewHeight = 480;
        const unsigned phoneViewWidth = 320;

        mainWebView()->resizeTo(phoneViewWidth, phoneViewHeight);
        // We also pass data to InjectedBundle::beginTesting() to have it call
        // WKBundlePageSetUseTestingViewportConfiguration(false).
    }
}

void TestController::updatePlatformSpecificTestOptionsForTest(TestOptions&, const std::string&) const
{
}

void TestController::platformInitializeContext()
{
}

void TestController::runModal(PlatformWebView* view)
{
    UIWindow *window = [view->platformView() window];
    if (!window)
        return;
    // FIXME: how to perform on iOS?
//    [[UIApplication sharedApplication] runModalForWindow:window];
}

const char* TestController::platformLibraryPathForTesting()
{
    return [[@"~/Library/Application Support/WebKitTestRunner" stringByExpandingTildeInPath] UTF8String];
}

void TestController::setHidden(bool)
{
    // FIXME: implement for iOS
}

} // namespace WTR
