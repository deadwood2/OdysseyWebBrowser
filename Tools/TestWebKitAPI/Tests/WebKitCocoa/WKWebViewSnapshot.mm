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

#import "config.h"

#if WK_API_ENABLED

#import "PlatformUtilities.h"
#import "Test.h"
#import "TestNavigationDelegate.h"
#import <WebKit/WKSnapshotConfiguration.h>
#import <wtf/RetainPtr.h>

static bool isDone;

#if PLATFORM(MAC)
typedef NSImage *PlatformImage;
typedef NSWindow *PlatformWindow;

static RetainPtr<CGImageRef> convertToCGImage(NSImage *image)
{
    return [image CGImageForProposedRect:nil context:nil hints:nil];
}

#else
typedef UIImage *PlatformImage;
typedef UIWindow *PlatformWindow;

static RetainPtr<CGImageRef> convertToCGImage(UIImage *image)
{
    return image.CGImage;
}
#endif

static NSInteger getPixelIndex(NSInteger x, NSInteger y, NSInteger width)
{
    return (y * width + x) * 4;
}

TEST(WKWebView, SnapshotImageError)
{
    CGFloat viewWidth = 800;
    CGFloat viewHeight = 600;
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, viewWidth, viewHeight)]);
    
    [webView loadHTMLString:@"<body style='background-color: red;'></body>" baseURL:nil];
    [webView _test_waitForDidFinishNavigation];
    [webView _killWebContentProcessAndResetState];

    RetainPtr<WKSnapshotConfiguration> snapshotConfiguration = adoptNS([[WKSnapshotConfiguration alloc] init]);
    [snapshotConfiguration setRect:NSMakeRect(0, 0, viewWidth, viewHeight)];
    [snapshotConfiguration setSnapshotWidth:@(viewWidth)];

    isDone = false;
    [webView takeSnapshotWithConfiguration:snapshotConfiguration.get() completionHandler:^(PlatformImage snapshotImage, NSError *error) {
        EXPECT_NULL(snapshotImage);
        EXPECT_WK_STREQ(@"WKErrorDomain", error.domain);

        isDone = true;
    }];

    TestWebKitAPI::Util::run(&isDone);
}

TEST(WKWebView, SnapshotImageBaseCase)
{
    NSInteger viewWidth = 800;
    NSInteger viewHeight = 600;
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, viewWidth, viewHeight)]);

    RetainPtr<PlatformWindow> window;
    CGFloat backingScaleFactor;

#if PLATFORM(MAC)
    window = adoptNS([[NSWindow alloc] initWithContentRect:[webView frame] styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO]);
    [[window contentView] addSubview:webView.get()];
    backingScaleFactor = [window backingScaleFactor];
#elif PLATFORM(IOS)
    window = adoptNS([[UIWindow alloc] initWithFrame:[webView frame]]);
    [window addSubview:webView.get()];
    backingScaleFactor = [[window screen] scale];
#endif

    [webView loadHTMLString:@"<body style='background-color:red;'><div style='background-color:blue; position:absolute; width:100px; height:100px; top:50px; left:50px'></div></body>" baseURL:nil];
    [webView _test_waitForDidFinishNavigation];

    RetainPtr<WKSnapshotConfiguration> snapshotConfiguration = adoptNS([[WKSnapshotConfiguration alloc] init]);
    [snapshotConfiguration setRect:NSMakeRect(0, 0, viewWidth, viewHeight)];
    [snapshotConfiguration setSnapshotWidth:@(viewWidth)];

    isDone = false;
    [webView takeSnapshotWithConfiguration:snapshotConfiguration.get() completionHandler:^(PlatformImage snapshotImage, NSError *error) {
        EXPECT_NULL(error);

        EXPECT_EQ(viewWidth, snapshotImage.size.width);

        RetainPtr<CGImageRef> cgImage = convertToCGImage(snapshotImage);
        RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());

        NSInteger viewWidthInPixels = viewWidth * backingScaleFactor;
        NSInteger viewHeightInPixels = viewHeight * backingScaleFactor;

        unsigned char rgba[viewWidthInPixels * viewHeightInPixels * 4];
        RetainPtr<CGContextRef> context = CGBitmapContextCreate(rgba, viewWidthInPixels, viewHeightInPixels, 8, 4 * viewWidthInPixels, colorSpace.get(), kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
        CGContextDrawImage(context.get(), CGRectMake(0, 0, viewWidthInPixels, viewHeightInPixels), cgImage.get());

        NSInteger pixelIndex = getPixelIndex(0, 0, viewWidthInPixels);
        EXPECT_EQ(255, rgba[pixelIndex]);
        EXPECT_EQ(0, rgba[pixelIndex + 1]);
        EXPECT_EQ(0, rgba[pixelIndex + 2]);

        // Inside the blue div (50, 50, 100, 100)
        pixelIndex = getPixelIndex(55 * backingScaleFactor, 55 * backingScaleFactor, viewWidthInPixels);
        EXPECT_EQ(0, rgba[pixelIndex]);
        EXPECT_EQ(0, rgba[pixelIndex + 1]);
        EXPECT_EQ(255, rgba[pixelIndex + 2]);

        pixelIndex = getPixelIndex(155 * backingScaleFactor, 155 * backingScaleFactor, viewWidthInPixels);
        EXPECT_EQ(255, rgba[pixelIndex]);
        EXPECT_EQ(0, rgba[pixelIndex + 1]);
        EXPECT_EQ(0, rgba[pixelIndex + 2]);

        isDone = true;
    }];

    TestWebKitAPI::Util::run(&isDone);
}

TEST(WKWebView, SnapshotImageScale)
{
    CGFloat viewWidth = 800;
    CGFloat viewHeight = 600;
    CGFloat scaleFactor = 2;
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, viewWidth, viewHeight)]);

    [webView loadHTMLString:@"<body style='background-color: red;'></body>" baseURL:nil];
    [webView _test_waitForDidFinishNavigation];

    RetainPtr<WKSnapshotConfiguration> snapshotConfiguration = adoptNS([[WKSnapshotConfiguration alloc] init]);
    [snapshotConfiguration setRect:NSMakeRect(0, 0, viewWidth, viewHeight)];
    [snapshotConfiguration setSnapshotWidth:@(viewWidth * scaleFactor)];

    isDone = false;
    [webView takeSnapshotWithConfiguration:snapshotConfiguration.get() completionHandler:^(PlatformImage snapshotImage, NSError *error) {
        EXPECT_NULL(error);
        EXPECT_NOT_NULL(snapshotImage);
        EXPECT_EQ(viewWidth * scaleFactor, snapshotImage.size.width);
        EXPECT_EQ(viewHeight * scaleFactor, snapshotImage.size.height);

        isDone = true;
    }];

    TestWebKitAPI::Util::run(&isDone);
}

TEST(WKWebView, SnapshotImageNilConfiguration)
{
    CGFloat viewWidth = 800;
    CGFloat viewHeight = 600;
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, viewWidth, viewHeight)]);

    [webView loadHTMLString:@"<body style='background-color: red;'></body>" baseURL:nil];
    [webView _test_waitForDidFinishNavigation];

    isDone = false;
    [webView takeSnapshotWithConfiguration:nil completionHandler:^(PlatformImage snapshotImage, NSError *error) {
        EXPECT_NULL(error);
        EXPECT_NOT_NULL(snapshotImage);
        EXPECT_EQ([webView bounds].size.width, snapshotImage.size.width);

        isDone = true;
    }];

    TestWebKitAPI::Util::run(&isDone);
}

TEST(WKWebView, SnapshotImageUninitializedConfiguration)
{
    CGFloat viewWidth = 800;
    CGFloat viewHeight = 600;
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, viewWidth, viewHeight)]);

    [webView loadHTMLString:@"<body style='background-color: red;'></body>" baseURL:nil];
    [webView _test_waitForDidFinishNavigation];

    RetainPtr<WKSnapshotConfiguration> snapshotConfiguration = adoptNS([[WKSnapshotConfiguration alloc] init]);

    isDone = false;
    [webView takeSnapshotWithConfiguration:snapshotConfiguration.get() completionHandler:^(PlatformImage snapshotImage, NSError *error) {
        EXPECT_NULL(error);
        EXPECT_NOT_NULL(snapshotImage);
        EXPECT_EQ([webView bounds].size.width, snapshotImage.size.width);

        isDone = true;
    }];

    TestWebKitAPI::Util::run(&isDone);
}

TEST(WKWebView, SnapshotImageUninitializedSnapshotWidth)
{
    CGFloat viewWidth = 800;
    CGFloat viewHeight = 600;
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, viewWidth, viewHeight)]);

    [webView loadHTMLString:@"<body style='background-color: red;'></body>" baseURL:nil];
    [webView _test_waitForDidFinishNavigation];

    RetainPtr<WKSnapshotConfiguration> snapshotConfiguration = adoptNS([[WKSnapshotConfiguration alloc] init]);
    [snapshotConfiguration setRect:NSMakeRect(0, 0, viewWidth, viewHeight)];

    isDone = false;
    [webView takeSnapshotWithConfiguration:snapshotConfiguration.get() completionHandler:^(PlatformImage snapshotImage, NSError *error) {
        EXPECT_NULL(error);
        EXPECT_NOT_NULL(snapshotImage);
        EXPECT_EQ([snapshotConfiguration rect].size.width, snapshotImage.size.width);

        isDone = true;
    }];

    TestWebKitAPI::Util::run(&isDone);
}

TEST(WKWebView, SnapshotImageLargeAsyncDecoding)
{
    NSInteger viewWidth = 800;
    NSInteger viewHeight = 600;
    RetainPtr<WKWebView> webView = adoptNS([[WKWebView alloc] initWithFrame:NSMakeRect(0, 0, viewWidth, viewHeight)]);

    NSURL *fileURL = [[NSBundle mainBundle] URLForResource:@"large-red-square-image" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"];
    [webView loadFileURL:fileURL allowingReadAccessToURL:fileURL];
    [webView _test_waitForDidFinishNavigation];

    RetainPtr<WKSnapshotConfiguration> snapshotConfiguration = adoptNS([[WKSnapshotConfiguration alloc] init]);
    [snapshotConfiguration setRect:NSMakeRect(0, 0, viewWidth, viewHeight)];
    [snapshotConfiguration setSnapshotWidth:@(viewWidth)];

    isDone = false;
    [webView takeSnapshotWithConfiguration:snapshotConfiguration.get() completionHandler:^(PlatformImage snapshotImage, NSError *error) {
        EXPECT_NULL(error);

        EXPECT_EQ(viewWidth, snapshotImage.size.width);

        RetainPtr<CGImageRef> cgImage = convertToCGImage(snapshotImage);
        RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());

        unsigned char rgba[viewWidth * viewHeight * 4];
        RetainPtr<CGContextRef> context = CGBitmapContextCreate(rgba, viewWidth, viewHeight, 8, 4 * viewWidth, colorSpace.get(), kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
        CGContextDrawImage(context.get(), CGRectMake(0, 0, viewWidth, viewHeight), cgImage.get());

        // Top-left corner of the div (0, 0, 100, 100)
        NSInteger pixelIndex = getPixelIndex(0, 0, viewWidth);
        EXPECT_EQ(255, rgba[pixelIndex]);
        EXPECT_EQ(0, rgba[pixelIndex + 1]);
        EXPECT_EQ(0, rgba[pixelIndex + 2]);

        // Right-bottom corner of the div (0, 0, 100, 100)
        pixelIndex = getPixelIndex(99, 99, viewWidth);
        EXPECT_EQ(255, rgba[pixelIndex]);
        EXPECT_EQ(0, rgba[pixelIndex + 1]);
        EXPECT_EQ(0, rgba[pixelIndex + 2]);

        // Outside the div (0, 0, 100, 100)
        pixelIndex = getPixelIndex(100, 100, viewWidth);
        EXPECT_EQ(255, rgba[pixelIndex]);
        EXPECT_EQ(255, rgba[pixelIndex + 1]);
        EXPECT_EQ(255, rgba[pixelIndex + 2]);

        isDone = true;
    }];

    TestWebKitAPI::Util::run(&isDone);
}

#endif
