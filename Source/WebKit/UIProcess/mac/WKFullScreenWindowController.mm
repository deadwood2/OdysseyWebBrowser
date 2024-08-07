/*
 * Copyright (C) 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#if ENABLE(FULLSCREEN_API) && !PLATFORM(IOS)

#import "WKFullScreenWindowController.h"

#import "LayerTreeContext.h"
#import "WKAPICast.h"
#import "WKViewInternal.h"
#import "WKViewPrivate.h"
#import "WebFullScreenManagerProxy.h"
#import "WebPageProxy.h"
#import <QuartzCore/QuartzCore.h>
#import <WebCore/FloatRect.h>
#import <WebCore/GeometryUtilities.h>
#import <WebCore/IntRect.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/WebCoreFullScreenPlaceholderView.h>
#import <WebCore/WebCoreFullScreenWindow.h>
#import <pal/system/SleepDisabler.h>
#import <wtf/BlockObjCExceptions.h>

using namespace WebKit;
using namespace WebCore;

static const NSTimeInterval DefaultWatchdogTimerInterval = 1;

enum FullScreenState : NSInteger {
    NotInFullScreen,
    WaitingToEnterFullScreen,
    EnteringFullScreen,
    InFullScreen,
    WaitingToExitFullScreen,
    ExitingFullScreen,
};

@interface NSWindow (WebNSWindowDetails)
- (void)exitFullScreenMode:(id)sender;
- (void)enterFullScreenMode:(id)sender;
@end

@interface WKFullScreenWindowController (Private) <NSAnimationDelegate>
- (void)_replaceView:(NSView*)view with:(NSView*)otherView;
- (WebFullScreenManagerProxy*)_manager;
- (void)_startEnterFullScreenAnimationWithDuration:(NSTimeInterval)duration;
- (void)_startExitFullScreenAnimationWithDuration:(NSTimeInterval)duration;
@end

static NSRect convertRectToScreen(NSWindow *window, NSRect rect)
{
    return [window convertRectToScreen:rect];
}

static void makeResponderFirstResponderIfDescendantOfView(NSWindow *window, NSResponder *responder, NSView *view)
{
    if ([responder isKindOfClass:[NSView class]] && [(NSView *)responder isDescendantOf:view])
        [window makeFirstResponder:responder];
}

@implementation WKFullScreenWindowController

#pragma mark -
#pragma mark Initialization
- (id)initWithWindow:(NSWindow *)window webView:(NSView *)webView page:(WebPageProxy&)page
{
    self = [super initWithWindow:window];
    if (!self)
        return nil;
    [window setDelegate:self];
    [window setCollectionBehavior:([window collectionBehavior] | NSWindowCollectionBehaviorFullScreenPrimary)];

    NSView *contentView = [window contentView];
    contentView.hidden = YES;
    contentView.autoresizesSubviews = YES;

    _backgroundView = adoptNS([[NSView alloc] initWithFrame:contentView.bounds]);
    _backgroundView.get().layer = [CALayer layer];
    _backgroundView.get().wantsLayer = YES;
    _backgroundView.get().autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [contentView addSubview:_backgroundView.get()];

    _clipView = adoptNS([[NSView alloc] initWithFrame:contentView.bounds]);
    [_clipView setWantsLayer:YES];
    [_clipView setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
    [_backgroundView addSubview:_clipView.get()];

    [self windowDidLoad];
    [window displayIfNeeded];
    _webView = webView;
    _page = &page;
    
    return self;
}

- (void)dealloc
{
    [[self window] setDelegate:nil];
    
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    if (_repaintCallback) {
        _repaintCallback->invalidate(WebKit::CallbackBase::Error::OwnerWasInvalidated);
        // invalidate() calls completeFinishExitFullScreenAnimationAfterRepaint, which
        // clears _repaintCallback.
        ASSERT(!_repaintCallback);
    }

    [super dealloc];
}

#pragma mark -
#pragma mark Accessors

@synthesize initialFrame=_initialFrame;
@synthesize finalFrame=_finalFrame;

- (BOOL)isFullScreen
{
    return _fullScreenState == WaitingToEnterFullScreen
        || _fullScreenState == EnteringFullScreen
        || _fullScreenState == InFullScreen;
}

- (WebCoreFullScreenPlaceholderView*)webViewPlaceholder
{
    return _webViewPlaceholder.get();
}

- (void)setSavedConstraints:(NSArray *)savedConstraints
{
    _savedConstraints = savedConstraints;
}

- (NSArray *)savedConstraints
{
    return _savedConstraints.get();
}

#pragma mark -
#pragma mark NSWindowController overrides

- (void)cancelOperation:(id)sender
{
    // If the page doesn't respond in DefaultWatchdogTimerInterval seconds, it could be because
    // the WebProcess has hung, so exit anyway.
    if (!_watchdogTimer) {
        [self _manager]->requestExitFullScreen();
        _watchdogTimer = adoptNS([[NSTimer alloc] initWithFireDate:[NSDate dateWithTimeIntervalSinceNow:DefaultWatchdogTimerInterval] interval:0 target:self selector:@selector(_watchdogTimerFired:) userInfo:nil repeats:NO]);
        [[NSRunLoop mainRunLoop] addTimer:_watchdogTimer.get() forMode:NSDefaultRunLoopMode];
    }
}

#pragma mark -
#pragma mark Exposed Interface

static RetainPtr<CGDataProviderRef> createImageProviderWithCopiedData(CGDataProviderRef sourceProvider)
{
    RetainPtr<CFDataRef> data = adoptCF(CGDataProviderCopyData(sourceProvider));
    return adoptCF(CGDataProviderCreateWithCFData(data.get()));
}

static RetainPtr<CGImageRef> createImageWithCopiedData(CGImageRef sourceImage)
{
    size_t width = CGImageGetWidth(sourceImage);
    size_t height = CGImageGetHeight(sourceImage);
    size_t bitsPerComponent = CGImageGetBitsPerComponent(sourceImage);
    size_t bitsPerPixel = CGImageGetBitsPerPixel(sourceImage);
    size_t bytesPerRow = CGImageGetBytesPerRow(sourceImage);
    CGColorSpaceRef colorSpace = CGImageGetColorSpace(sourceImage);
    CGBitmapInfo bitmapInfo = CGImageGetBitmapInfo(sourceImage);
    RetainPtr<CGDataProviderRef> provider = createImageProviderWithCopiedData(CGImageGetDataProvider(sourceImage));
    bool shouldInterpolate = CGImageGetShouldInterpolate(sourceImage);
    CGColorRenderingIntent intent = CGImageGetRenderingIntent(sourceImage);

    return adoptCF(CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow, colorSpace, bitmapInfo, provider.get(), 0, shouldInterpolate, intent));
}

- (void)enterFullScreen:(NSScreen *)screen
{
    if ([self isFullScreen])
        return;
    _fullScreenState = WaitingToEnterFullScreen;

    if (!screen)
        screen = [NSScreen mainScreen];
    NSRect screenFrame = [screen frame];

    NSRect webViewFrame = convertRectToScreen([_webView window], [_webView convertRect:[_webView frame] toView:nil]);

    // Flip coordinate system:
    webViewFrame.origin.y = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - NSMaxY(webViewFrame);

    CGWindowID windowID = [[_webView window] windowNumber];
    RetainPtr<CGImageRef> webViewContents = adoptCF(CGWindowListCreateImage(NSRectToCGRect(webViewFrame), kCGWindowListOptionIncludingWindow, windowID, kCGWindowImageShouldBeOpaque));

    // Using the returned CGImage directly would result in calls to the WindowServer every time
    // the image was painted. Instead, copy the image data into our own process to eliminate that
    // future overhead.
    webViewContents = createImageWithCopiedData(webViewContents.get());

    // Screen updates to be re-enabled in _startEnterFullScreenAnimationWithDuration:
    NSDisableScreenUpdates();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[self window] setAutodisplay:NO];
#pragma clang diagnostic pop

    [self _manager]->saveScrollPosition();
    _savedTopContentInset = _page->topContentInset();
    _page->setTopContentInset(0);
    [[self window] setFrame:screenFrame display:NO];

    // Painting is normally suspended when the WKView is removed from the window, but this is
    // unnecessary in the full-screen animation case, and can cause bugs; see
    // https://bugs.webkit.org/show_bug.cgi?id=88940 and https://bugs.webkit.org/show_bug.cgi?id=88374
    // We will resume the normal behavior in _startEnterFullScreenAnimationWithDuration:
    _page->setSuppressVisibilityUpdates(true);

    // Swap the webView placeholder into place.
    if (!_webViewPlaceholder) {
        _webViewPlaceholder = adoptNS([[WebCoreFullScreenPlaceholderView alloc] initWithFrame:[_webView frame]]);
        [_webViewPlaceholder setAction:@selector(cancelOperation:)];
    }
    [_webViewPlaceholder setTarget:nil];
    [_webViewPlaceholder setContents:(id)webViewContents.get()];
    self.savedConstraints = _webView.superview.constraints;
    [self _replaceView:_webView with:_webViewPlaceholder.get()];
    
    // Then insert the WebView into the full screen window
    NSView *contentView = [[self window] contentView];
    [_clipView addSubview:_webView positioned:NSWindowBelow relativeTo:nil];
    _webView.frame = NSInsetRect(contentView.bounds, 0, -_page->topContentInset());

    _savedScale = _page->pageScaleFactor();
    _page->scalePage(1, IntPoint());
    [self _manager]->setAnimatingFullScreen(true);
    [self _manager]->willEnterFullScreen();
}

- (void)beganEnterFullScreenWithInitialFrame:(NSRect)initialFrame finalFrame:(NSRect)finalFrame
{
    if (_fullScreenState != WaitingToEnterFullScreen)
        return;
    _fullScreenState = EnteringFullScreen;

    _initialFrame = initialFrame;
    _finalFrame = finalFrame;

    [self.window orderBack: self]; // Make sure the full screen window is part of the correct Space.
    [[self window] enterFullScreenMode:self];
}

static const float minVideoWidth = 480 + 20 + 20; // Note: Keep in sync with mediaControlsApple.css (video:-webkit-full-screen::-webkit-media-controls-panel)

- (void)finishedEnterFullScreenAnimation:(bool)completed
{
    if (_fullScreenState != EnteringFullScreen)
        return;
    
    if (completed) {
        _fullScreenState = InFullScreen;

        // Screen updates to be re-enabled ta the end of the current block.
        NSDisableScreenUpdates();
        [self _manager]->didEnterFullScreen();
        [self _manager]->setAnimatingFullScreen(false);

        [_backgroundView.get().layer removeAllAnimations];
        [[_clipView layer] removeAllAnimations];
        [[_clipView layer] setMask:nil];

        [_webViewPlaceholder setExitWarningVisible:YES];
        [_webViewPlaceholder setTarget:self];

        NSSize minContentSize = self.window.contentMinSize;
        minContentSize.width = minVideoWidth;
        self.window.contentMinSize = minContentSize;
    } else {
        // Transition to fullscreen failed. Clean up.
        _fullScreenState = NotInFullScreen;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [[self window] setAutodisplay:YES];
#pragma clang diagnostic pop
        _page->setSuppressVisibilityUpdates(false);

        NSResponder *firstResponder = [[self window] firstResponder];
        [self _replaceView:_webViewPlaceholder.get() with:_webView];
        BEGIN_BLOCK_OBJC_EXCEPTIONS
        [NSLayoutConstraint activateConstraints:self.savedConstraints];
        END_BLOCK_OBJC_EXCEPTIONS
        self.savedConstraints = nil;
        makeResponderFirstResponderIfDescendantOfView(_webView.window, firstResponder, _webView);
        [[_webView window] makeKeyAndOrderFront:self];

        _page->scalePage(_savedScale, IntPoint());
        [self _manager]->restoreScrollPosition();
        _page->setTopContentInset(_savedTopContentInset);
        [self _manager]->didExitFullScreen();
        [self _manager]->setAnimatingFullScreen(false);
    }

    NSEnableScreenUpdates();
}

- (void)exitFullScreen
{
    if (_watchdogTimer) {
        [_watchdogTimer invalidate];
        _watchdogTimer.clear();
    }

    if (![self isFullScreen])
        return;
    _fullScreenState = WaitingToExitFullScreen;

    [_webViewPlaceholder setExitWarningVisible:NO];

    // Screen updates to be re-enabled in _startExitFullScreenAnimationWithDuration: or beganExitFullScreenWithInitialFrame:finalFrame:
    NSDisableScreenUpdates();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[self window] setAutodisplay:NO];
#pragma clang diagnostic pop

    // See the related comment in enterFullScreen:
    // We will resume the normal behavior in _startExitFullScreenAnimationWithDuration:
    _page->setSuppressVisibilityUpdates(true);
    [_webViewPlaceholder setTarget:nil];

    [self _manager]->setAnimatingFullScreen(true);
    [self _manager]->willExitFullScreen();
}

- (void)requestExitFullScreen
{
    [self _manager]->requestExitFullScreen();
}

- (void)beganExitFullScreenWithInitialFrame:(NSRect)initialFrame finalFrame:(NSRect)finalFrame
{
    if (_fullScreenState != WaitingToExitFullScreen)
        return;
    _fullScreenState = ExitingFullScreen;

    if (![[self window] isOnActiveSpace]) {
        // If the full screen window is not in the active space, the NSWindow full screen animation delegate methods
        // will never be called. So call finishedExitFullScreenAnimation explicitly.
        [self finishedExitFullScreenAnimation:YES];

        // Because we are breaking the normal animation pattern, re-enable screen updates
        // as exitFullScreen has disabled them, but _startExitFullScreenAnimationWithDuration:
        // will never be called.
        NSEnableScreenUpdates();
    }

    [[self window] exitFullScreenMode:self];
}

- (void)finishedExitFullScreenAnimation:(bool)completed
{
    if (_fullScreenState == InFullScreen) {
        // If we are currently in the InFullScreen state, this notification is unexpected, meaning
        // fullscreen was exited without being initiated by WebKit. Do not return early, but continue to
        // clean up our state by calling those methods which would have been called by -exitFullscreen,
        // and proceed to close the fullscreen window.
        [self _manager]->requestExitFullScreen();
        [_webViewPlaceholder setTarget:nil];
        [self _manager]->setAnimatingFullScreen(false);
        [self _manager]->willExitFullScreen();
    } else if (_fullScreenState != ExitingFullScreen)
        return;
    _fullScreenState = NotInFullScreen;

    NSResponder *firstResponder = [[self window] firstResponder];

    // Screen updates to be re-enabled in completeFinishExitFullScreenAnimationAfterRepaint.
    NSDisableScreenUpdates();
    _page->setSuppressVisibilityUpdates(true);
    [[self window] orderOut:self];
    NSView *contentView = [[self window] contentView];
    contentView.hidden = YES;
    [_backgroundView.get().layer removeAllAnimations];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[_webViewPlaceholder window] setAutodisplay:NO];
#pragma clang diagnostic pop

    [self _replaceView:_webViewPlaceholder.get() with:_webView];
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [NSLayoutConstraint activateConstraints:self.savedConstraints];
    END_BLOCK_OBJC_EXCEPTIONS
    self.savedConstraints = nil;
    makeResponderFirstResponderIfDescendantOfView(_webView.window, firstResponder, _webView);

    [[_webView window] makeKeyAndOrderFront:self];

    // These messages must be sent after the swap or flashing will occur during forceRepaint:
    [self _manager]->didExitFullScreen();
    [self _manager]->setAnimatingFullScreen(false);
    _page->scalePage(_savedScale, IntPoint());
    [self _manager]->restoreScrollPosition();
    _page->setTopContentInset(_savedTopContentInset);

    if (_repaintCallback) {
        _repaintCallback->invalidate(WebKit::CallbackBase::Error::OwnerWasInvalidated);
        // invalidate() calls completeFinishExitFullScreenAnimationAfterRepaint, which
        // clears _repaintCallback.
        ASSERT(!_repaintCallback);
    }
    _repaintCallback = VoidCallback::create([self](WebKit::CallbackBase::Error) {
        [self completeFinishExitFullScreenAnimationAfterRepaint];
    });
    _page->forceRepaint(_repaintCallback.copyRef());
}

- (void)completeFinishExitFullScreenAnimationAfterRepaint
{
    _repaintCallback = nullptr;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[_webView window] setAutodisplay:YES];
#pragma clang diagnostic pop
    [[_webView window] displayIfNeeded];
    _page->setSuppressVisibilityUpdates(false);
    NSEnableScreenUpdates();
}

- (void)performClose:(id)sender
{
    if ([self isFullScreen])
        [self cancelOperation:sender];
}

- (void)close
{
    // We are being asked to close rapidly, most likely because the page 
    // has closed or the web process has crashed.  Just walk through our
    // normal exit full screen sequence, but don't wait to be called back
    // in response.
    if ([self isFullScreen])
        [self exitFullScreen];
    
    if (_fullScreenState == ExitingFullScreen)
        [self finishedExitFullScreenAnimation:YES];

    _webView = nil;

    [super close];
}

#pragma mark -
#pragma mark Custom NSWindow Full Screen Animation

- (NSArray *)customWindowsToEnterFullScreenForWindow:(NSWindow *)window
{
    return @[self.window];
}

- (NSArray *)customWindowsToExitFullScreenForWindow:(NSWindow *)window
{
    return @[self.window];
}

- (void)window:(NSWindow *)window startCustomAnimationToEnterFullScreenWithDuration:(NSTimeInterval)duration
{
    [self _startEnterFullScreenAnimationWithDuration:duration];
}

- (void)window:(NSWindow *)window startCustomAnimationToExitFullScreenWithDuration:(NSTimeInterval)duration
{
    [self _startExitFullScreenAnimationWithDuration:duration];
}

- (void)windowDidFailToEnterFullScreen:(NSWindow *)window
{
    [self finishedEnterFullScreenAnimation:NO];
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification
{
    [self finishedEnterFullScreenAnimation:YES];
}

- (void)windowDidFailToExitFullScreen:(NSWindow *)window
{
    [self finishedExitFullScreenAnimation:NO];
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
    [self finishedExitFullScreenAnimation:YES];
}

#pragma mark -
#pragma mark Internal Interface

- (WebFullScreenManagerProxy*)_manager
{
    if (!_page)
        return nullptr;
    return _page->fullScreenManager();
}

- (void)_replaceView:(NSView*)view with:(NSView*)otherView
{
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    [otherView setFrame:[view frame]];        
    [otherView setAutoresizingMask:[view autoresizingMask]];
    [otherView removeFromSuperview];
    [[view superview] addSubview:otherView positioned:NSWindowAbove relativeTo:view];
    [view removeFromSuperview];
    [CATransaction commit];
}

static CAMediaTimingFunction *timingFunctionForDuration(CFTimeInterval duration)
{
    if (duration >= 0.8)
        return [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
    return [CAMediaTimingFunction functionWithControlPoints:.25 :0 :0 :1];
}

enum AnimationDirection { AnimateIn, AnimateOut };
static CAAnimation *zoomAnimation(const FloatRect& initialFrame, const FloatRect& finalFrame, const FloatRect& screenFrame, CFTimeInterval duration, AnimationDirection direction)
{
    CABasicAnimation *scaleAnimation = [CABasicAnimation animationWithKeyPath:@"transform"];
    FloatRect scaleRect = smallestRectWithAspectRatioAroundRect(finalFrame.size().aspectRatio(), initialFrame);
    CGAffineTransform resetOriginTransform = CGAffineTransformMakeTranslation(screenFrame.x() - finalFrame.x(), screenFrame.y() - finalFrame.y());
    CGAffineTransform scaleTransform = CGAffineTransformMakeScale(scaleRect.width() / finalFrame.width(), scaleRect.height() / finalFrame.height());
    CGAffineTransform translateTransform = CGAffineTransformMakeTranslation(scaleRect.x() - screenFrame.x(), scaleRect.y() - screenFrame.y());

    CGAffineTransform finalTransform = CGAffineTransformConcat(CGAffineTransformConcat(resetOriginTransform, scaleTransform), translateTransform);
    NSValue *scaleValue = [NSValue valueWithCATransform3D:CATransform3DMakeAffineTransform(finalTransform)];
    if (direction == AnimateIn)
        scaleAnimation.fromValue = scaleValue;
    else
        scaleAnimation.toValue = scaleValue;

    scaleAnimation.duration = duration;
    scaleAnimation.removedOnCompletion = NO;
    scaleAnimation.fillMode = kCAFillModeBoth;
    scaleAnimation.timingFunction = timingFunctionForDuration(duration);
    return scaleAnimation;
}

static CALayer *createMask(const FloatRect& bounds)
{
    CALayer *maskLayer = [CALayer layer];
    maskLayer.anchorPoint = CGPointZero;
    maskLayer.frame = bounds;
    maskLayer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
    maskLayer.autoresizingMask = (NSViewWidthSizable | NSViewHeightSizable);
    return maskLayer;
}

static CAAnimation *maskAnimation(const FloatRect& initialFrame, const FloatRect& finalFrame, const FloatRect& screenFrame, CFTimeInterval duration, AnimationDirection direction)
{
    CABasicAnimation *boundsAnimation = [CABasicAnimation animationWithKeyPath:@"bounds"];
    FloatRect boundsRect = largestRectWithAspectRatioInsideRect(initialFrame.size().aspectRatio(), finalFrame);
    NSValue *boundsValue = [NSValue valueWithRect:FloatRect(FloatPoint(), boundsRect.size())];
    if (direction == AnimateIn)
        boundsAnimation.fromValue = boundsValue;
    else
        boundsAnimation.toValue = boundsValue;

    CABasicAnimation *positionAnimation = [CABasicAnimation animationWithKeyPath:@"position"];
    NSValue *positionValue = [NSValue valueWithPoint:FloatPoint(boundsRect.location() - screenFrame.location())];
    if (direction == AnimateIn)
        positionAnimation.fromValue = positionValue;
    else
        positionAnimation.toValue = positionValue;

    CAAnimationGroup *animation = [CAAnimationGroup animation];
    animation.animations = @[boundsAnimation, positionAnimation];
    animation.duration = duration;
    animation.removedOnCompletion = NO;
    animation.fillMode = kCAFillModeBoth;
    animation.timingFunction = timingFunctionForDuration(duration);
    return animation;
}

static CAAnimation *fadeAnimation(CFTimeInterval duration, AnimationDirection direction)
{
    CABasicAnimation *fadeAnimation = [CABasicAnimation animationWithKeyPath:@"backgroundColor"];
    if (direction == AnimateIn)
        fadeAnimation.toValue = (id)CGColorGetConstantColor(kCGColorBlack);
    else
        fadeAnimation.fromValue = (id)CGColorGetConstantColor(kCGColorBlack);
    fadeAnimation.duration = duration;
    fadeAnimation.removedOnCompletion = NO;
    fadeAnimation.fillMode = kCAFillModeBoth;
    fadeAnimation.timingFunction = timingFunctionForDuration(duration);
    return fadeAnimation;
}

- (void)_startEnterFullScreenAnimationWithDuration:(NSTimeInterval)duration
{
    NSView* contentView = [[self window] contentView];

    [[_clipView layer] addAnimation:zoomAnimation(_initialFrame, _finalFrame, self.window.screen.frame, duration, AnimateIn) forKey:@"fullscreen"];
    CALayer *maskLayer = createMask(contentView.bounds);
    [maskLayer addAnimation:maskAnimation(_initialFrame, _finalFrame, self.window.screen.frame, duration, AnimateIn) forKey:@"fullscreen"];
    [_clipView layer].mask = maskLayer;

    contentView.hidden = NO;
    [_backgroundView.get().layer addAnimation:fadeAnimation(duration, AnimateIn) forKey:@"fullscreen"];

    NSWindow* window = [self window];
    NSWindowCollectionBehavior behavior = [window collectionBehavior];
    [window setCollectionBehavior:(behavior | NSWindowCollectionBehaviorCanJoinAllSpaces)];
    [window makeKeyAndOrderFront:self];
    [window setCollectionBehavior:behavior];
    [window makeFirstResponder:_webView];

    _page->setSuppressVisibilityUpdates(false);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[self window] setAutodisplay:YES];
#pragma clang diagnostic pop
    [[self window] displayIfNeeded];
    NSEnableScreenUpdates();
}

- (void)_startExitFullScreenAnimationWithDuration:(NSTimeInterval)duration
{
    if ([self isFullScreen]) {
        // We still believe we're in full screen mode, so we must have been asked to exit full
        // screen by the system full screen button.
        [self _manager]->requestExitFullScreen();
        [self exitFullScreen];
        _fullScreenState = ExitingFullScreen;
    }

    [[_clipView layer] addAnimation:zoomAnimation(_initialFrame, _finalFrame, self.window.screen.frame, duration, AnimateOut) forKey:@"fullscreen"];
    NSView* contentView = [[self window] contentView];
    CALayer *maskLayer = createMask(contentView.bounds);
    [maskLayer addAnimation:maskAnimation(_initialFrame, _finalFrame, self.window.screen.frame, duration, AnimateOut) forKey:@"fullscreen"];
    [_clipView layer].mask = maskLayer;

    contentView.hidden = NO;
    [_backgroundView.get().layer addAnimation:fadeAnimation(duration, AnimateOut) forKey:@"fullscreen"];

    _page->setSuppressVisibilityUpdates(false);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [[self window] setAutodisplay:YES];
#pragma clang diagnostic pop
    [[self window] displayIfNeeded];
    NSEnableScreenUpdates();
}

- (void)_watchdogTimerFired:(NSTimer *)timer
{
    [self exitFullScreen];
}

@end

#endif // ENABLE(FULLSCREEN_API) && !PLATFORM(IOS)
