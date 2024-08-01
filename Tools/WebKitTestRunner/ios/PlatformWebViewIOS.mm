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
#import "PlatformWebView.h"

#import "TestController.h"
#import "TestRunnerWKWebView.h"
#import <WebKit/WKImageCG.h>
#import <WebKit/WKPreferencesPrivate.h>
#import <WebKit/WKWebViewConfiguration.h>
#import <WebKit/WKWebViewPrivate.h>
#import <wtf/RetainPtr.h>

@interface WKWebView (Details)
- (WKPageRef)_pageForTesting;
@end

@interface WebKitTestRunnerWindow : UIWindow {
    WTR::PlatformWebView* _platformWebView;
    CGPoint _fakeOrigin;
    BOOL _initialized;
}
@property (nonatomic, assign) WTR::PlatformWebView* platformWebView;
@end

@implementation WebKitTestRunnerWindow
@synthesize platformWebView = _platformWebView;

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame]))
        _initialized = YES;

    return self;
}

- (BOOL)isKeyWindow
{
    return [super isKeyWindow] && (_platformWebView ? _platformWebView->windowIsKey() : YES);
}

- (void)setFrameOrigin:(CGPoint)point
{
    _fakeOrigin = point;
}

- (void)setFrame:(CGRect)windowFrame
{
    if (!_initialized) {
        [super setFrame:windowFrame];
        return;
    }

    CGRect currentFrame = [super frame];

    _fakeOrigin = windowFrame.origin;

    [super setFrame:CGRectMake(currentFrame.origin.x, currentFrame.origin.y, windowFrame.size.width, windowFrame.size.height)];
}

- (CGRect)frameRespectingFakeOrigin
{
    CGRect currentFrame = [self frame];
    return CGRectMake(_fakeOrigin.x, _fakeOrigin.y, currentFrame.size.width, currentFrame.size.height);
}

- (CGFloat)backingScaleFactor
{
    return 1;
}

@end

@interface UIWindow ()

- (void)_setWindowResolution:(CGFloat)resolution displayIfChanged:(BOOL)displayIfChanged;

@end

namespace WTR {

PlatformWebView::PlatformWebView(WKWebViewConfiguration* configuration, const TestOptions& options)
    : m_windowIsKey(true)
    , m_options(options)
{
    CGRect rect = CGRectMake(0, 0, TestController::viewWidth, TestController::viewHeight);
    m_view = [[TestRunnerWKWebView alloc] initWithFrame:rect configuration:configuration];

    m_window = [[WebKitTestRunnerWindow alloc] initWithFrame:rect];
    m_window.platformWebView = this;

    [m_window addSubview:m_view];
    [m_window makeKeyAndVisible];
}

PlatformWebView::~PlatformWebView()
{
    m_window.platformWebView = nil;
    [m_view release];
    [m_window release];
}

void PlatformWebView::setWindowIsKey(bool isKey)
{
    m_windowIsKey = isKey;
    if (isKey)
        [m_window makeKeyWindow];
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    WKRect frame = windowFrame();
    frame.size.width = width;
    frame.size.height = height;
    setWindowFrame(frame);
}

WKPageRef PlatformWebView::page()
{
    return [m_view _pageForTesting];
}

void PlatformWebView::focus()
{
    makeWebViewFirstResponder();
    setWindowIsKey(true);
}

WKRect PlatformWebView::windowFrame()
{
    CGRect frame = [m_window frameRespectingFakeOrigin];

    WKRect wkFrame;
    wkFrame.origin.x = frame.origin.x;
    wkFrame.origin.y = frame.origin.y;
    wkFrame.size.width = frame.size.width;
    wkFrame.size.height = frame.size.height;
    return wkFrame;
}

void PlatformWebView::setWindowFrame(WKRect frame)
{
    [m_window setFrame:CGRectMake(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height)];
    [platformView() setFrame:CGRectMake(0, 0, frame.size.width, frame.size.height)];
}

void PlatformWebView::didInitializeClients()
{
    // Set a temporary 1x1 window frame to force a WindowAndViewFramesChanged notification. <rdar://problem/13380145>
    WKRect wkFrame = windowFrame();
    [m_window setFrame:CGRectMake(0, 0, 1, 1)];
    setWindowFrame(wkFrame);
}

void PlatformWebView::addChromeInputField()
{
    UITextField* textField = [[UITextField alloc] initWithFrame:CGRectMake(0, 0, 100, 20)];
    textField.tag = 1;
    [m_window addSubview:textField];
    [textField release];
}

void PlatformWebView::removeChromeInputField()
{
    UITextField* textField = (UITextField*)[m_window viewWithTag:1];
    if (textField) {
        [textField removeFromSuperview];
        makeWebViewFirstResponder();
        [textField release];
    }
}

void PlatformWebView::makeWebViewFirstResponder()
{
    // FIXME: iOS equivalent?
    // [m_window makeFirstResponder:m_view];
}

void PlatformWebView::changeWindowScaleIfNeeded(float)
{
    // Retina only surface.
}

WKRetainPtr<WKImageRef> PlatformWebView::windowSnapshotImage()
{
    // FIXME: Need an implementation of this, or we're depending on software paints!
    return nullptr;
}

bool PlatformWebView::viewSupportsOptions(const TestOptions& options) const
{
    if (m_options.overrideLanguages != options.overrideLanguages)
        return false;

    return true;
}

void PlatformWebView::setNavigationGesturesEnabled(bool enabled)
{
}

} // namespace WTR
