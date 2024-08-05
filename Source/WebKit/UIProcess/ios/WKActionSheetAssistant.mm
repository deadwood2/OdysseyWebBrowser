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
#import "WKActionSheetAssistant.h"

#if PLATFORM(IOS)

#import "APIUIClient.h"
#import "SandboxUtilities.h"
#import "TCCSPI.h"
#import "UIKitSPI.h"
#import "WKActionSheet.h"
#import "WKContentViewInteraction.h"
#import "WKNSURLExtras.h"
#import "WeakObjCPtr.h"
#import "WebPageProxy.h"
#import "_WKActivatedElementInfoInternal.h"
#import "_WKElementActionInternal.h"
#import <UIKit/UIView.h>
#import <WebCore/LocalizedStrings.h>
#import <WebCore/PathUtilities.h>
#import <WebCore/WebCoreNSURLExtras.h>
#import <wtf/SoftLinking.h>
#import <wtf/text/WTFString.h>

#if HAVE(APP_LINKS)
#import <WebCore/LaunchServicesSPI.h>
#endif

#if HAVE(SAFARI_SERVICES_FRAMEWORK)
#import <SafariServices/SSReadingList.h>
SOFT_LINK_FRAMEWORK(SafariServices)
SOFT_LINK_CLASS(SafariServices, SSReadingList)
#endif

SOFT_LINK_PRIVATE_FRAMEWORK(TCC)
SOFT_LINK(TCC, TCCAccessPreflight, TCCAccessPreflightResult, (CFStringRef service, CFDictionaryRef options), (service, options))
SOFT_LINK_CONSTANT(TCC, kTCCServicePhotos, CFStringRef)

using namespace WebKit;

#if HAVE(APP_LINKS)
static bool applicationHasAppLinkEntitlements()
{
    static bool hasEntitlement = processHasEntitlement(@"com.apple.private.canGetAppLinkInfo") && processHasEntitlement(@"com.apple.private.canModifyAppLinkPermissions");
    return hasEntitlement;
}

static LSAppLink *appLinkForURL(NSURL *url)
{
    __block LSAppLink *syncAppLink = nil;

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    [LSAppLink getAppLinkWithURL:url completionHandler:^(LSAppLink *appLink, NSError *error) {
        syncAppLink = [appLink retain];
        dispatch_semaphore_signal(semaphore);
    }];
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);

    return [syncAppLink autorelease];
}
#endif

@implementation WKActionSheetAssistant {
    WeakObjCPtr<id <WKActionSheetAssistantDelegate>> _delegate;
    RetainPtr<WKActionSheet> _interactionSheet;
    RetainPtr<_WKActivatedElementInfo> _elementInfo;
    std::optional<WebKit::InteractionInformationAtPosition> _positionInformation;
    WeakObjCPtr<UIView> _view;
    BOOL _needsLinkIndicator;
    BOOL _isPresentingDDUserInterface;
    BOOL _hasPendingActionSheet;
}

- (id <WKActionSheetAssistantDelegate>)delegate
{
    return _delegate.getAutoreleased();
}

- (void)setDelegate:(id <WKActionSheetAssistantDelegate>)delegate
{
    _delegate = delegate;
}

- (id)initWithView:(UIView *)view
{
    _view = view;
    return self;
}

- (void)dealloc
{
    [self cleanupSheet];
    [super dealloc];
}

- (BOOL)synchronouslyRetrievePositionInformation
{
    auto delegate = _delegate.get();
    if (!delegate)
        return NO;

    // FIXME: This should be asynchronous, since we control the presentation of the action sheet.
    _positionInformation = [delegate positionInformationForActionSheetAssistant:self];
    return !!_positionInformation;
}

- (UIView *)superviewForSheet
{
    UIView *view = _view.getAutoreleased();
    UIView *superview = [view window];

    // FIXME: WebKit has a delegate to retrieve the superview for the image sheet (superviewForImageSheetForWebView)
    // Do we need it in WK2?

    // Find the top most view with a view controller
    UIViewController *controller = nil;
    UIView *currentView = view;
    while (currentView) {
        UIViewController *aController = [UIViewController viewControllerForView:currentView];
        if (aController)
            controller = aController;

        currentView = [currentView superview];
    }
    if (controller)
        superview = controller.view;

    return superview;
}

- (CGRect)_presentationRectForSheetGivenPoint:(CGPoint)point inHostView:(UIView *)hostView
{
    CGPoint presentationPoint = [hostView convertPoint:point fromView:_view.getAutoreleased()];
    CGRect presentationRect = CGRectMake(presentationPoint.x, presentationPoint.y, 1.0, 1.0);

    return CGRectInset(presentationRect, -22.0, -22.0);
}

- (UIView *)hostViewForSheet
{
    return [self superviewForSheet];
}

static const CGFloat presentationElementRectPadding = 15;

- (CGRect)presentationRectForElementUsingClosestIndicatedRect
{
    UIView *view = [self superviewForSheet];
    auto delegate = _delegate.get();
    if (!view || !delegate || !_positionInformation)
        return CGRectZero;

    auto indicator = _positionInformation->linkIndicator;
    if (indicator.textRectsInBoundingRectCoordinates.isEmpty())
        return CGRectZero;

    WebCore::FloatPoint touchLocation = _positionInformation->request.point;
    WebCore::FloatPoint linkElementLocation = indicator.textBoundingRectInRootViewCoordinates.location();
    Vector<WebCore::FloatRect> indicatedRects;
    for (auto rect : indicator.textRectsInBoundingRectCoordinates) {
        rect.inflate(2);
        rect.moveBy(linkElementLocation);
        indicatedRects.append(rect);
    }

    for (auto path : WebCore::PathUtilities::pathsWithShrinkWrappedRects(indicatedRects, 0)) {
        auto boundingRect = path.fastBoundingRect();
        if (boundingRect.contains(touchLocation))
            return CGRectInset([view convertRect:(CGRect)boundingRect fromView:_view.getAutoreleased()], -presentationElementRectPadding, -presentationElementRectPadding);
    }

    return CGRectZero;
}

- (CGRect)presentationRectForIndicatedElement
{
    UIView *view = [self superviewForSheet];
    auto delegate = _delegate.get();
    if (!view || !delegate || !_positionInformation)
        return CGRectZero;

    auto elementBounds = _positionInformation->bounds;
    return CGRectInset([view convertRect:elementBounds fromView:_view.getAutoreleased()], -presentationElementRectPadding, -presentationElementRectPadding);
}

- (CGRect)initialPresentationRectInHostViewForSheet
{
    UIView *view = [self superviewForSheet];
    auto delegate = _delegate.get();
    if (!view || !delegate || !_positionInformation)
        return CGRectZero;

    return [self _presentationRectForSheetGivenPoint:_positionInformation->request.point inHostView:view];
}

- (CGRect)presentationRectInHostViewForSheet
{
    UIView *view = [self superviewForSheet];
    auto delegate = _delegate.get();
    if (!view || !delegate || !_positionInformation)
        return CGRectZero;

    CGRect boundingRect = _positionInformation->bounds;
    CGPoint fromPoint = _positionInformation->request.point;

    // FIXME: We must adjust our presentation point to take into account a change in document scale.

    // Test to see if we are still within the target node as it may have moved after rotation.
    if (!CGRectContainsPoint(boundingRect, fromPoint))
        fromPoint = CGPointMake(CGRectGetMidX(boundingRect), CGRectGetMidY(boundingRect));

    return [self _presentationRectForSheetGivenPoint:fromPoint inHostView:view];
}

- (void)updatePositionInformation
{
    auto delegate = _delegate.get();
    if ([delegate respondsToSelector:@selector(updatePositionInformationForActionSheetAssistant:)])
        [delegate updatePositionInformationForActionSheetAssistant:self];
}

- (BOOL)presentSheet
{
    // Calculate the presentation rect just before showing.
    CGRect presentationRect = CGRectZero;
    if (UI_USER_INTERFACE_IDIOM() != UIUserInterfaceIdiomPhone) {
        presentationRect = [self initialPresentationRectInHostViewForSheet];
        if (CGRectIsEmpty(presentationRect))
            return NO;
    }

    return [_interactionSheet presentSheetFromRect:presentationRect];
}

- (void)updateSheetPosition
{
    [_interactionSheet updateSheetPosition];
}

- (BOOL)isShowingSheet
{
    return _interactionSheet != nil;
}

- (NSArray *)currentAvailableActionTitles
{
    if (!_interactionSheet)
        return @[];
    
    NSMutableArray *array = [NSMutableArray array];
    
    for (UIAlertAction *action in _interactionSheet.get().actions)
        [array addObject:action.title];
    
    return array;
}

- (void)_createSheetWithElementActions:(NSArray *)actions showLinkTitle:(BOOL)showLinkTitle
{
    auto delegate = _delegate.get();
    if (!delegate)
        return;

    if (!_positionInformation)
        return;

    NSURL *targetURL = [NSURL URLWithString:_positionInformation->url];
    NSString *urlScheme = [targetURL scheme];
    BOOL isJavaScriptURL = [urlScheme length] && [urlScheme caseInsensitiveCompare:@"javascript"] == NSOrderedSame;
    // FIXME: We should check if Javascript is enabled in the preferences.

    _interactionSheet = adoptNS([[WKActionSheet alloc] init]);
    _interactionSheet.get().sheetDelegate = self;
    _interactionSheet.get().preferredStyle = UIAlertControllerStyleActionSheet;

    NSString *titleString = nil;
    BOOL titleIsURL = NO;
    if (showLinkTitle && [[targetURL absoluteString] length]) {
        if (isJavaScriptURL)
            titleString = WEB_UI_STRING_KEY("JavaScript", "JavaScript Action Sheet Title", "Title for action sheet for JavaScript link");
        else {
            titleString = WebCore::userVisibleString(targetURL);
            titleIsURL = YES;
        }
    } else
        titleString = _positionInformation->title;

    if ([titleString length]) {
        [_interactionSheet setTitle:titleString];
        // We should configure the text field's line breaking mode correctly here, based on whether
        // the title is an URL or not, but the appropriate UIAlertController SPIs are not available yet.
        // The code that used to do this in the UIActionSheet world has been saved for reference in
        // <rdar://problem/17049781> Configure the UIAlertController's title appropriately.
    }

    for (_WKElementAction *action in actions) {
        [_interactionSheet _addActionWithTitle:[action title] style:UIAlertActionStyleDefault handler:^{
            [action _runActionWithElementInfo:_elementInfo.get() forActionSheetAssistant:self];
            [self cleanupSheet];
        } shouldDismissHandler:^{
            return (BOOL)(!action.dismissalHandler || action.dismissalHandler());
        }];
    }

    [_interactionSheet addAction:[UIAlertAction actionWithTitle:WEB_UI_STRING_KEY("Cancel", "Cancel button label in button bar", "Title for Cancel button label in button bar")
                                                          style:UIAlertActionStyleCancel
                                                        handler:^(UIAlertAction *action) {
                                                            [self cleanupSheet];
                                                        }]];

    if ([delegate respondsToSelector:@selector(actionSheetAssistant:willStartInteractionWithElement:)])
        [delegate actionSheetAssistant:self willStartInteractionWithElement:_elementInfo.get()];
}

- (void)showImageSheet
{
    ASSERT(!_elementInfo);

    auto delegate = _delegate.get();
    if (!delegate)
        return;

    if (![self synchronouslyRetrievePositionInformation])
        return;

    void (^showImageSheetWithAlternateURLBlock)(NSURL*, NSDictionary *userInfo) = ^(NSURL *alternateURL, NSDictionary *userInfo) {
        NSURL *targetURL = [NSURL _web_URLWithWTFString:_positionInformation->url] ?: alternateURL;
        auto elementBounds = _positionInformation->bounds;
        auto elementInfo = adoptNS([[_WKActivatedElementInfo alloc] _initWithType:_WKActivatedElementTypeImage URL:targetURL location:_positionInformation->request.point title:_positionInformation->title ID:_positionInformation->idAttribute rect:elementBounds image:_positionInformation->image.get() userInfo:userInfo]);
        if ([delegate respondsToSelector:@selector(actionSheetAssistant:showCustomSheetForElement:)] && [delegate actionSheetAssistant:self showCustomSheetForElement:elementInfo.get()])
            return;
        auto defaultActions = [self defaultActionsForImageSheet:elementInfo.get()];

        RetainPtr<NSArray> actions = [delegate actionSheetAssistant:self decideActionsForElement:elementInfo.get() defaultActions:WTFMove(defaultActions)];

        if (![actions count])
            return;

        if (!alternateURL && userInfo) {
            [UIApp _cancelAllTouches];
            return;
        }

        [self _createSheetWithElementActions:actions.get() showLinkTitle:YES];
        if (!_interactionSheet)
            return;

        _elementInfo = WTFMove(elementInfo);

        if (![_interactionSheet presentSheet:presentationStyleForView(_view.getAutoreleased(), _positionInformation.value(), _elementInfo.get())])
            [self cleanupSheet];
    };

    if (_positionInformation->url.isEmpty() && _positionInformation->image && [delegate respondsToSelector:@selector(actionSheetAssistant:getAlternateURLForImage:completion:)]) {
        RetainPtr<UIImage> uiImage = adoptNS([[UIImage alloc] initWithCGImage:_positionInformation->image->makeCGImageCopy().get()]);

        _hasPendingActionSheet = YES;
        RetainPtr<WKActionSheetAssistant> retainedSelf(self);
        [delegate actionSheetAssistant:self getAlternateURLForImage:uiImage.get() completion:^(NSURL *alternateURL, NSDictionary *userInfo) {
            if (!retainedSelf->_hasPendingActionSheet)
                return;

            retainedSelf->_hasPendingActionSheet = NO;
            showImageSheetWithAlternateURLBlock(alternateURL, userInfo);
        }];
        return;
    }

    showImageSheetWithAlternateURLBlock(nil, nil);
}

static WKActionSheetPresentationStyle presentationStyleForView(UIView *view, const InteractionInformationAtPosition& positionInfo, _WKActivatedElementInfo *elementInfo)
{
    auto apparentElementRect = [view convertRect:positionInfo.bounds toView:view.window];
    if (CGRectIsEmpty(apparentElementRect))
        return WKActionSheetPresentAtTouchLocation;

    auto windowRect = view.window.bounds;
    apparentElementRect = CGRectIntersection(apparentElementRect, windowRect);

    auto leftInset = CGRectGetMinX(apparentElementRect) - CGRectGetMinX(windowRect);
    auto topInset = CGRectGetMinY(apparentElementRect) - CGRectGetMinY(windowRect);
    auto rightInset = CGRectGetMaxX(windowRect) - CGRectGetMaxX(apparentElementRect);
    auto bottomInset = CGRectGetMaxY(windowRect) - CGRectGetMaxY(apparentElementRect);

    // If at least this much of the window is available for the popover to draw in, then target the element rect when presenting the action menu popover.
    // Otherwise, there is not enough space to position the popover around the element, so revert to using the touch location instead.
    static const CGFloat minimumAvailableWidthOrHeightRatio = 0.4;
    if (std::max(leftInset, rightInset) <= minimumAvailableWidthOrHeightRatio * CGRectGetWidth(windowRect) && std::max(topInset, bottomInset) <= minimumAvailableWidthOrHeightRatio * CGRectGetHeight(windowRect))
        return WKActionSheetPresentAtTouchLocation;

    if (elementInfo.type == _WKActivatedElementTypeLink && positionInfo.linkIndicator.textRectsInBoundingRectCoordinates.size())
        return WKActionSheetPresentAtClosestIndicatorRect;

    return WKActionSheetPresentAtElementRect;
}

- (void)_appendOpenActionsForURL:(NSURL *)url actions:(NSMutableArray *)defaultActions elementInfo:(_WKActivatedElementInfo *)elementInfo
{
#if HAVE(APP_LINKS)
    ASSERT(_delegate);
    if (applicationHasAppLinkEntitlements() && [_delegate.get() actionSheetAssistant:self shouldIncludeAppLinkActionsForElement:elementInfo]) {
        LSAppLink *appLink = appLinkForURL(url);
        if (appLink) {
            NSString *title = WEB_UI_STRING("Open in Safari", "Title for Open in Safari Link action button");
            _WKElementAction *openInDefaultBrowserAction = [_WKElementAction _elementActionWithType:_WKElementActionTypeOpenInDefaultBrowser title:title actionHandler:^(_WKActivatedElementInfo *) {
                [appLink openInWebBrowser:YES setAppropriateOpenStrategyAndWebBrowserState:nil completionHandler:^(BOOL success, NSError *error) { }];
            }];
            [defaultActions addObject:openInDefaultBrowserAction];

            NSString *externalApplicationName = [appLink.targetApplicationProxy localizedNameForContext:nil];
            if (externalApplicationName) {
                NSString *title = [NSString stringWithFormat:WEB_UI_STRING("Open in “%@”", "Title for Open in External Application Link action button"), externalApplicationName];
                _WKElementAction *openInExternalApplicationAction = [_WKElementAction _elementActionWithType:_WKElementActionTypeOpenInExternalApplication title:title actionHandler:^(_WKActivatedElementInfo *) {
                    [appLink openInWebBrowser:NO setAppropriateOpenStrategyAndWebBrowserState:nil completionHandler:^(BOOL success, NSError *error) { }];
                }];
                [defaultActions addObject:openInExternalApplicationAction];
            }
        } else
            [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeOpen assistant:self]];
    } else
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeOpen assistant:self]];
#else
    [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeOpen assistant:self]];
#endif
}

- (RetainPtr<NSArray>)defaultActionsForLinkSheet:(_WKActivatedElementInfo *)elementInfo
{
    NSURL *targetURL = [elementInfo URL];
    if (!targetURL)
        return nil;

    auto defaultActions = adoptNS([[NSMutableArray alloc] init]);
    [self _appendOpenActionsForURL:targetURL actions:defaultActions.get() elementInfo:elementInfo];

#if HAVE(SAFARI_SERVICES_FRAMEWORK)
    if ([getSSReadingListClass() supportsURL:targetURL])
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeAddToReadingList assistant:self]];
#endif
    if (![[targetURL scheme] length] || [[targetURL scheme] caseInsensitiveCompare:@"javascript"] != NSOrderedSame) {
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeCopy assistant:self]];
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeShare assistant:self]];
    }

    return defaultActions;
}

- (RetainPtr<NSArray>)defaultActionsForImageSheet:(_WKActivatedElementInfo *)elementInfo
{
    NSURL *targetURL = [elementInfo URL];

    auto defaultActions = adoptNS([[NSMutableArray alloc] init]);
    if (targetURL) {
        [self _appendOpenActionsForURL:targetURL actions:defaultActions.get() elementInfo:elementInfo];
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeShare assistant:self]];
    }

#if HAVE(SAFARI_SERVICES_FRAMEWORK)
    if ([getSSReadingListClass() supportsURL:targetURL])
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeAddToReadingList assistant:self]];
#endif
    if (TCCAccessPreflight(getkTCCServicePhotos(), NULL) != kTCCAccessPreflightDenied)
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeSaveImage assistant:self]];
    if (!targetURL.scheme.length || [targetURL.scheme caseInsensitiveCompare:@"javascript"] != NSOrderedSame)
        [defaultActions addObject:[_WKElementAction _elementActionWithType:_WKElementActionTypeCopy assistant:self]];

    return defaultActions;
}

- (BOOL)needsLinkIndicator
{
    return _needsLinkIndicator;
}

- (void)showLinkSheet
{
    ASSERT(!_elementInfo);

    auto delegate = _delegate.get();
    if (!delegate)
        return;

    _needsLinkIndicator = YES;
    if (![self synchronouslyRetrievePositionInformation])
        return;

    NSURL *targetURL = [NSURL _web_URLWithWTFString:_positionInformation->url];
    if (!targetURL) {
        _needsLinkIndicator = NO;
        return;
    }

    auto elementInfo = adoptNS([[_WKActivatedElementInfo alloc] _initWithType:_WKActivatedElementTypeLink URL:targetURL location:_positionInformation->request.point title:_positionInformation->title ID:_positionInformation->idAttribute rect:_positionInformation->bounds image:_positionInformation->image.get()]);
    if ([delegate respondsToSelector:@selector(actionSheetAssistant:showCustomSheetForElement:)] && [delegate actionSheetAssistant:self showCustomSheetForElement:elementInfo.get()]) {
        _needsLinkIndicator = NO;
        return;
    }

    auto defaultActions = [self defaultActionsForLinkSheet:elementInfo.get()];

    RetainPtr<NSArray> actions = [delegate actionSheetAssistant:self decideActionsForElement:elementInfo.get() defaultActions:WTFMove(defaultActions)];

    if (![actions count]) {
        _needsLinkIndicator = NO;
        return;
    }

    [self _createSheetWithElementActions:actions.get() showLinkTitle:YES];
    if (!_interactionSheet) {
        _needsLinkIndicator = NO;
        return;
    }

    _elementInfo = WTFMove(elementInfo);

    if (![_interactionSheet presentSheet:presentationStyleForView(_view.getAutoreleased(), _positionInformation.value(), _elementInfo.get())])
        [self cleanupSheet];
}

- (void)showDataDetectorsSheet
{
#if ENABLE(DATA_DETECTION)
    auto delegate = _delegate.get();
    if (!delegate)
        return;

    if (![self synchronouslyRetrievePositionInformation])
        return;

    NSURL *targetURL = [NSURL _web_URLWithWTFString:_positionInformation->url];
    if (!targetURL)
        return;

    if (![[getDDDetectionControllerClass() tapAndHoldSchemes] containsObject:[targetURL scheme]])
        return;

    DDDetectionController *controller = [getDDDetectionControllerClass() sharedController];
    NSDictionary *context = nil;
    NSString *textAtSelection = nil;
    RetainPtr<NSMutableDictionary> extendedContext;

    if ([delegate respondsToSelector:@selector(dataDetectionContextForActionSheetAssistant:)])
        context = [delegate dataDetectionContextForActionSheetAssistant:self];
    if ([delegate respondsToSelector:@selector(selectedTextForActionSheetAssistant:)])
        textAtSelection = [delegate selectedTextForActionSheetAssistant:self];
    if (!_positionInformation->textBefore.isEmpty() || !_positionInformation->textAfter.isEmpty()) {
        extendedContext = adoptNS([@{
            getkDataDetectorsLeadingText() : _positionInformation->textBefore,
            getkDataDetectorsTrailingText() : _positionInformation->textAfter,
        } mutableCopy]);
        
        if (context)
            [extendedContext addEntriesFromDictionary:context];
        context = extendedContext.get();
    }
    NSArray *dataDetectorsActions = [controller actionsForURL:targetURL identifier:_positionInformation->dataDetectorIdentifier selectedText:textAtSelection results:_positionInformation->dataDetectorResults.get() context:context];
    if ([dataDetectorsActions count] == 0)
        return;

    NSMutableArray *elementActions = [NSMutableArray array];
    for (NSUInteger actionNumber = 0; actionNumber < [dataDetectorsActions count]; actionNumber++) {
        DDAction *action = [dataDetectorsActions objectAtIndex:actionNumber];
        RetainPtr<WKActionSheetAssistant> retainedSelf = self;
        _WKElementAction *elementAction = [_WKElementAction elementActionWithTitle:[action localizedName] actionHandler:^(_WKActivatedElementInfo *actionInfo) {
            retainedSelf.get()->_isPresentingDDUserInterface = action.hasUserInterface;
            [[getDDDetectionControllerClass() sharedController] performAction:action fromAlertController:retainedSelf.get()->_interactionSheet.get() interactionDelegate:retainedSelf.get()];
        }];
        elementAction.dismissalHandler = ^{
            return (BOOL)!action.hasUserInterface;
        };
        [elementActions addObject:elementAction];
    }

    [self _createSheetWithElementActions:elementActions showLinkTitle:NO];
    if (!_interactionSheet)
        return;

    if (elementActions.count <= 1)
        _interactionSheet.get().arrowDirections = UIPopoverArrowDirectionUp | UIPopoverArrowDirectionDown;

    if (![_interactionSheet presentSheet:WKActionSheetPresentAtTouchLocation])
        [self cleanupSheet];
#endif
}

- (void)cleanupSheet
{
    auto delegate = _delegate.get();
    if ([delegate respondsToSelector:@selector(actionSheetAssistantDidStopInteraction:)])
        [delegate actionSheetAssistantDidStopInteraction:self];

    [_interactionSheet doneWithSheet:!_isPresentingDDUserInterface];
    [_interactionSheet setSheetDelegate:nil];
    _interactionSheet = nil;
    _elementInfo = nil;
    _positionInformation = std::nullopt;
    _needsLinkIndicator = NO;
    _isPresentingDDUserInterface = NO;
    _hasPendingActionSheet = NO;
}

@end

#endif // PLATFORM(IOS)
