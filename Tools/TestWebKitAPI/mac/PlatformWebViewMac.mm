/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import <Carbon/Carbon.h>
#import <WebKit/WKRetainPtr.h>
#import <WebKit/WKViewPrivate.h>

@interface ActiveOffscreenWindow : NSWindow
@end

@implementation ActiveOffscreenWindow
- (BOOL)isKeyWindow
{
    return YES;
}
- (BOOL)isVisible
{
    return YES;
}
@end

namespace TestWebKitAPI {

void PlatformWebView::initialize(WKPageConfigurationRef configuration, Class wkViewSubclass)
{
    NSRect rect = NSMakeRect(0, 0, 800, 600);
    m_view = [[wkViewSubclass alloc] initWithFrame:rect configurationRef:configuration];
    [m_view setWindowOcclusionDetectionEnabled:NO];

    NSRect windowRect = NSOffsetRect(rect, -10000, [(NSScreen *)[[NSScreen screens] objectAtIndex:0] frame].size.height - rect.size.height + 10000);
    m_window = [[ActiveOffscreenWindow alloc] initWithContentRect:windowRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];
    [m_window setColorSpace:[[NSScreen mainScreen] colorSpace]];
    [[m_window contentView] addSubview:m_view];
    [m_window orderBack:nil];
    [m_window setAutodisplay:NO];
    [m_window setReleasedWhenClosed:NO];
}

PlatformWebView::PlatformWebView(WKPageConfigurationRef configuration)
{
    initialize(configuration, [WKView class]);
}

PlatformWebView::PlatformWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef)
{
    WKRetainPtr<WKPageConfigurationRef> configuration = adoptWK(WKPageConfigurationCreate());
    
    WKPageConfigurationSetContext(configuration.get(), contextRef);
    WKPageConfigurationSetPageGroup(configuration.get(), pageGroupRef);
    
    initialize(configuration.get(), [WKView class]);
}

PlatformWebView::PlatformWebView(WKPageRef relatedPage)
{
    WKRetainPtr<WKPageConfigurationRef> configuration = adoptWK(WKPageConfigurationCreate());
    
    WKPageConfigurationSetContext(configuration.get(), WKPageGetContext(relatedPage));
    WKPageConfigurationSetPageGroup(configuration.get(), WKPageGetPageGroup(relatedPage));
    WKPageConfigurationSetRelatedPage(configuration.get(), relatedPage);

    initialize(configuration.get(), [WKView class]);
}

PlatformWebView::PlatformWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef, Class wkViewSubclass)
{
    WKRetainPtr<WKPageConfigurationRef> configuration = adoptWK(WKPageConfigurationCreate());
    
    WKPageConfigurationSetContext(configuration.get(), contextRef);
    WKPageConfigurationSetPageGroup(configuration.get(), pageGroupRef);
    
    initialize(configuration.get(), wkViewSubclass);
}

PlatformWebView::~PlatformWebView()
{
    [m_window close];
    [m_window release];
    [m_view release];
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    [m_view setFrame:NSMakeRect(0, 0, width, height)];
}

WKPageRef PlatformWebView::page() const
{
    return [m_view pageRef];
}

void PlatformWebView::focus()
{
    // Implement.
}

void PlatformWebView::simulateSpacebarKeyPress()
{
    NSEvent *event = [NSEvent keyEventWithType:NSKeyDown
                                      location:NSMakePoint(5, 5)
                                 modifierFlags:0
                                     timestamp:GetCurrentEventTime()
                                  windowNumber:[m_window windowNumber]
                                       context:[NSGraphicsContext currentContext]
                                    characters:@" "
                   charactersIgnoringModifiers:@" "
                                     isARepeat:NO
                                       keyCode:0x31];

    [m_view keyDown:event];

    event = [NSEvent keyEventWithType:NSKeyUp
                             location:NSMakePoint(5, 5)
                        modifierFlags:0
                            timestamp:GetCurrentEventTime()
                         windowNumber:[m_window windowNumber]
                              context:[NSGraphicsContext currentContext]
                           characters:@" "
          charactersIgnoringModifiers:@" "
                            isARepeat:NO
                              keyCode:0x31];

    [m_view keyUp:event];
}

void PlatformWebView::simulateRightClick(unsigned x, unsigned y)
{
    NSEvent *event = [NSEvent mouseEventWithType:NSRightMouseDown
                                        location:NSMakePoint(x, y)
                                   modifierFlags:0
                                       timestamp:GetCurrentEventTime()
                                    windowNumber:[m_window windowNumber]
                                         context:[NSGraphicsContext currentContext]
                                     eventNumber:0
                                      clickCount:0
                                        pressure:0];


    [m_view rightMouseDown:event];

    event = [NSEvent mouseEventWithType:NSRightMouseUp
                               location:NSMakePoint(x, y)
                          modifierFlags:0
                              timestamp:GetCurrentEventTime()
                           windowNumber:[m_window windowNumber]
                                context:[NSGraphicsContext currentContext]
                            eventNumber:0
                             clickCount:0
                               pressure:0];

    [m_view rightMouseUp:event];

}
    
void PlatformWebView::simulateMouseMove(unsigned x, unsigned y)
{   
    NSEvent *event = [NSEvent mouseEventWithType:NSMouseMoved
                               location:NSMakePoint(x, y)
                          modifierFlags:0
                              timestamp:GetCurrentEventTime()
                           windowNumber:[m_window windowNumber]
                                context:[NSGraphicsContext currentContext]
                            eventNumber:0
                             clickCount:0
                               pressure:0];
    
    [m_view mouseMoved:event];
    
}

static NSEventType eventTypeForButton(WKEventMouseButton button)
{
    switch (button) {
    case kWKEventMouseButtonLeftButton:
        return NSLeftMouseDown;
    case kWKEventMouseButtonRightButton:
        return NSRightMouseDown;
    case kWKEventMouseButtonMiddleButton:
        return NSOtherMouseDown;
    case kWKEventMouseButtonNoButton:
        return NSLeftMouseDown;
    }

    return NSLeftMouseDown;
}

static NSEventModifierFlags modifierFlagsForWKModifiers(WKEventModifiers modifiers)
{
    NSEventModifierFlags returnVal = 0;
    if (modifiers & kWKEventModifiersShiftKey)
        returnVal |= NSShiftKeyMask;
    if (modifiers & kWKEventModifiersControlKey)
        returnVal |= NSControlKeyMask;
    if (modifiers & kWKEventModifiersAltKey)
        returnVal |= NSAlternateKeyMask;
    if (modifiers & kWKEventModifiersMetaKey)
        returnVal |= NSCommandKeyMask;

    return returnVal;
}
    
void PlatformWebView::simulateButtonClick(WKEventMouseButton button, unsigned x, unsigned y, WKEventModifiers modifiers)
{
    NSEvent *event = [NSEvent mouseEventWithType:eventTypeForButton(button)
                                        location:NSMakePoint(x, y)
                                   modifierFlags:modifierFlagsForWKModifiers(modifiers)
                                       timestamp:GetCurrentEventTime()
                                    windowNumber:[m_window windowNumber]
                                         context:[NSGraphicsContext currentContext]
                                     eventNumber:0
                                      clickCount:0
                                        pressure:0];

    [m_view mouseDown:event];
}

} // namespace TestWebKitAPI
