/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "InjectedBundle.h"
#import "InjectedBundlePage.h"
#import "JSWrapper.h"
#import "AccessibilityNotificationHandler.h"
#import "AccessibilityUIElement.h"

#import <JavaScriptCore/JSRetainPtr.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <WebKit/WKBundleFrame.h>
#import <objc/runtime.h>
#import <wtf/RetainPtr.h>

@interface NSObject (WebAccessibilityObjectWrapperAdditions)
+ (void)accessibilitySetShouldRepostNotifications:(BOOL)repost;
@end

@interface NSString (JSStringRefAdditions)
- (JSStringRef)createJSStringRef;
@end

@implementation NSString (JSStringRefAdditions)

- (JSStringRef)createJSStringRef
{
    return JSStringCreateWithCFString((CFStringRef)self);
}

@end

@implementation AccessibilityNotificationHandler

- (id)init
{
    if (!(self = [super init]))
        return nil;

    return self;
}

- (void)setPlatformElement:(id)platformElement
{
    m_platformElement = platformElement;
}

- (void)dealloc
{
    [self stopObserving];

    WKBundleFrameRef mainFrame = WKBundlePageGetMainFrame(WTR::InjectedBundle::singleton().page()->page());
    JSContextRef context = WKBundleFrameGetJavaScriptContext(mainFrame);

    JSValueUnprotect(context, m_notificationFunctionCallback);
    m_notificationFunctionCallback = 0;

    [super dealloc];
}

- (void)setCallback:(JSValueRef)callback
{
    if (!callback)
        return;

    WKBundleFrameRef mainFrame = WKBundlePageGetMainFrame(WTR::InjectedBundle::singleton().page()->page());
    JSContextRef context = WKBundleFrameGetJavaScriptContext(mainFrame);

    if (m_notificationFunctionCallback)
        JSValueUnprotect(context, m_notificationFunctionCallback);

    m_notificationFunctionCallback = callback;
    JSValueProtect(context, m_notificationFunctionCallback);
}

static Class webAccessibilityObjectWrapperClass()
{
    static Class cls = objc_getClass("WebAccessibilityObjectWrapper");
    ASSERT(cls);
    return cls;
}

- (void)startObserving
{
    // Once we start requesting notifications, it's on for the duration of the program.
    // This is to avoid any race conditions between tests turning this flag on and off. Instead
    // AccessibilityNotificationHandler can ignore events it doesn't care about.
    [webAccessibilityObjectWrapperClass() accessibilitySetShouldRepostNotifications:YES];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_notificationReceived:) name:@"AXDRTNotification" object:nil];
}

- (void)stopObserving
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

static JSValueRef makeValueRefForValue(JSContextRef context, id value)
{
    if ([value isKindOfClass:[NSString class]])
        return JSValueMakeString(context, adopt([value createJSStringRef]).get());
    if ([value isKindOfClass:[NSNumber class]]) {
        if (!strcmp([value objCType], @encode(BOOL)))
            return JSValueMakeBoolean(context, [value boolValue]);
        return JSValueMakeNumber(context, [value doubleValue]);
    }
    if ([value isKindOfClass:webAccessibilityObjectWrapperClass()])
        return toJS(context, WTR::AccessibilityUIElement::create(static_cast<PlatformUIElement>(value)).ptr());
    if ([value isKindOfClass:[NSDictionary class]])
        return makeObjectRefForDictionary(context, value);
    if ([value isKindOfClass:[NSArray class]])
        return makeArrayRefForArray(context, value);
    return nullptr;
}

static JSValueRef makeArrayRefForArray(JSContextRef context, NSArray *array)
{
    NSUInteger count = array.count;
    JSValueRef arguments[count];

    for (NSUInteger i = 0; i < count; i++)
        arguments[i] = makeValueRefForValue(context, [array objectAtIndex:i]);

    return JSObjectMakeArray(context, count, arguments, nullptr);
}

static JSValueRef makeObjectRefForDictionary(JSContextRef context, NSDictionary *dictionary)
{
    JSObjectRef object = JSObjectMake(context, nullptr, nullptr);

    [dictionary enumerateKeysAndObjectsUsingBlock:^(NSString *key, id obj, BOOL *stop)
    {
        if (JSValueRef propertyValue = makeValueRefForValue(context, obj))
            JSObjectSetProperty(context, object, adopt([key createJSStringRef]).get(), propertyValue, kJSPropertyAttributeNone, nullptr);
    }];

    return object;
}

- (void)_notificationReceived:(NSNotification *)notification
{
    NSString *notificationName = [[notification userInfo] objectForKey:@"notificationName"];
    if (!notificationName)
        return;
    if (m_platformElement && m_platformElement != [notification object])
        return;

    NSDictionary *userInfo = [[notification userInfo] objectForKey:@"userInfo"];

    WKBundleFrameRef mainFrame = WKBundlePageGetMainFrame(WTR::InjectedBundle::singleton().page()->page());
    JSContextRef context = WKBundleFrameGetJavaScriptContext(mainFrame);

    JSValueRef notificationNameArgument = JSValueMakeString(context, adopt([notificationName createJSStringRef]).get());
    JSValueRef userInfoArgument = makeObjectRefForDictionary(context, userInfo);
    if (m_platformElement) {
        // Listener for one element gets the notification name and userInfo.
        JSValueRef arguments[2];
        arguments[0] = notificationNameArgument;
        arguments[1] = userInfoArgument;
        JSObjectCallAsFunction(context, const_cast<JSObjectRef>(m_notificationFunctionCallback), 0, 2, arguments, 0);
    } else {
        // A global listener gets the element, notification name and userInfo.
        JSValueRef arguments[3];
        arguments[0] = toJS(context, WTR::AccessibilityUIElement::create([notification object]).ptr());
        arguments[1] = notificationNameArgument;
        arguments[2] = userInfoArgument;
        JSObjectCallAsFunction(context, const_cast<JSObjectRef>(m_notificationFunctionCallback), 0, 3, arguments, 0);
    }
}

@end

