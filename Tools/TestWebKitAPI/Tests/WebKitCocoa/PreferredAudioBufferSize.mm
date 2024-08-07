/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#if WK_HAVE_C_SPI && WK_API_ENABLED

#import "PlatformUtilities.h"
#import "Test.h"
#import "TestWKWebView.h"
#import "WebCoreTestSupport.h"
#import <JavaScriptCore/JSContext.h>
#import <WebKit/WKPreferencesPrivate.h>
#import <WebKit/WKUIDelegatePrivate.h>
#import <WebKit/_WKFullscreenDelegate.h>
#import <wtf/RetainPtr.h>

class PreferredAudioBufferSize : public testing::Test {
public:
    void SetUp() override
    {
        configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
        WKRetainPtr<WKContextRef> context(AdoptWK, TestWebKitAPI::Util::createContextForInjectedBundleTest("InternalsInjectedBundleTest"));
        configuration.get().processPool = (WKProcessPool *)context.get();
        configuration.get().preferences._lowPowerVideoAudioBufferSizeEnabled = YES;
    }

    void createView()
    {
        webView = adoptNS([[TestWKWebView alloc] initWithFrame:NSMakeRect(0, 0, 300, 300) configuration:configuration.get() addToWindow:YES]);
    }

    double preferredAudioBufferSize() const
    {
        return [webView stringByEvaluatingJavaScript:@"window.internals.preferredAudioBufferSize()"].doubleValue;
    }

    RetainPtr<WKWebViewConfiguration> configuration;
    RetainPtr<TestWKWebView> webView;
};

TEST_F(PreferredAudioBufferSize, Empty)
{
    createView();
    [webView synchronouslyLoadHTMLString:@""];
    EXPECT_EQ(512, preferredAudioBufferSize());
}

TEST_F(PreferredAudioBufferSize, AudioElement)
{
    createView();
    [webView synchronouslyLoadTestPageNamed:@"audio-only"];
    [webView waitForMessage:@"playing"];
    EXPECT_EQ(4096, preferredAudioBufferSize());
}

TEST_F(PreferredAudioBufferSize, DISABLED_WebAudio)
{
    createView();
    [webView synchronouslyLoadTestPageNamed:@"web-audio-only"];
    [webView waitForMessage:@"playing"];
    EXPECT_EQ(128, preferredAudioBufferSize());
}

TEST_F(PreferredAudioBufferSize, VideoOnly)
{
    createView();
    [webView synchronouslyLoadTestPageNamed:@"video-without-audio"];
    [webView waitForMessage:@"playing"];
    EXPECT_EQ(512, preferredAudioBufferSize());
}

TEST_F(PreferredAudioBufferSize, VideoWithAudio)
{
    createView();
    [webView synchronouslyLoadTestPageNamed:@"video-with-audio"];
    [webView waitForMessage:@"playing"];
    EXPECT_EQ(4096, preferredAudioBufferSize());
}

TEST_F(PreferredAudioBufferSize, DISABLED_AudioWithWebAudio)
{
    createView();
    [webView synchronouslyLoadTestPageNamed:@"audio-with-web-audio"];
    [webView waitForMessage:@"playing"];
    EXPECT_EQ(128, preferredAudioBufferSize());
}

TEST_F(PreferredAudioBufferSize, VideoWithAudioAndWebAudio)
{
    createView();
    [webView synchronouslyLoadTestPageNamed:@"video-with-audio-and-web-audio"];
    [webView waitForMessage:@"playing"];
    EXPECT_EQ(128, preferredAudioBufferSize());
}

#endif // WK_HAVE_C_SPI && WK_API_ENABLED