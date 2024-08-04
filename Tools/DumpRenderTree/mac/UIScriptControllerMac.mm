/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
#import "UIScriptController.h"

#import "DumpRenderTree.h"
#import "UIScriptContext.h"
#import <JavaScriptCore/JSContext.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <JavaScriptCore/JSValue.h>
#import <WebKit/WebKit.h>
#import <WebKit/WebViewPrivate.h>

#if PLATFORM(MAC)

namespace WTR {

void UIScriptController::doAsyncTask(JSValueRef callback)
{
    unsigned callbackID = m_context->prepareForAsyncTask(callback, CallbackTypeNonPersistent);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_context)
            return;
        m_context->asyncTaskComplete(callbackID);
    });
}

void UIScriptController::doAfterPresentationUpdate(JSValueRef callback)
{
    return doAsyncTask(callback);
}

void UIScriptController::doAfterNextStablePresentationUpdate(JSValueRef callback)
{
    doAsyncTask(callback);
}

void UIScriptController::doAfterVisibleContentRectUpdate(JSValueRef callback)
{
    doAsyncTask(callback);
}

void UIScriptController::insertText(JSStringRef, int, int)
{
}

void UIScriptController::zoomToScale(double scale, JSValueRef callback)
{
    unsigned callbackID = m_context->prepareForAsyncTask(callback, CallbackTypeNonPersistent);

    WebView *webView = [mainFrame webView];
    [webView _scaleWebView:scale atOrigin:NSZeroPoint];

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_context)
            return;
        m_context->asyncTaskComplete(callbackID);
    });
}

void UIScriptController::simulateAccessibilitySettingsChangeNotification(JSValueRef)
{
}

JSObjectRef UIScriptController::contentsOfUserInterfaceItem(JSStringRef interfaceItem) const
{
#if JSC_OBJC_API_ENABLED
    WebView *webView = [mainFrame webView];
    RetainPtr<CFStringRef> interfaceItemCF = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, interfaceItem));
    NSDictionary *contentDictionary = [webView _contentsOfUserInterfaceItem:(NSString *)interfaceItemCF.get()];
    return JSValueToObject(m_context->jsContext(), [JSValue valueWithObject:contentDictionary inContext:[JSContext contextWithJSGlobalContextRef:m_context->jsContext()]].JSValueRef, nullptr);
#else
    UNUSED_PARAM(interfaceItem);
    return nullptr;
#endif
}

void UIScriptController::overridePreference(JSStringRef preferenceRef, JSStringRef valueRef)
{
    WebPreferences *preferences = mainFrame.webView.preferences;

    RetainPtr<CFStringRef> value = adoptCF(JSStringCopyCFString(kCFAllocatorDefault, valueRef));
    if (JSStringIsEqualToUTF8CString(preferenceRef, "WebKitMinimumFontSize"))
        preferences.minimumFontSize = [(NSString *)value.get() doubleValue];
}

void UIScriptController::removeViewFromWindow(JSValueRef callback)
{
    unsigned callbackID = m_context->prepareForAsyncTask(callback, CallbackTypeNonPersistent);

    WebView *webView = [mainFrame webView];
    [webView removeFromSuperview];

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_context)
            return;
        m_context->asyncTaskComplete(callbackID);
    });
}

void UIScriptController::addViewToWindow(JSValueRef callback)
{
    unsigned callbackID = m_context->prepareForAsyncTask(callback, CallbackTypeNonPersistent);

    WebView *webView = [mainFrame webView];
    [[mainWindow contentView] addSubview:webView];

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_context)
            return;
        m_context->asyncTaskComplete(callbackID);
    });
}

}

#endif // PLATFORM(MAC)
