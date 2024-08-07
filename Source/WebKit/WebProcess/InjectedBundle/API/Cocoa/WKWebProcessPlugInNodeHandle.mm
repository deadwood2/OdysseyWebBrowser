/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#import "WKWebProcessPlugInNodeHandleInternal.h"

#import "WKSharedAPICast.h"
#import "WKWebProcessPlugInFrameInternal.h"
#import <WebCore/HTMLTextFormControlElement.h>
#import <WebCore/IntRect.h>
#import <WebKit/WebImage.h>

#if WK_API_ENABLED

using namespace WebKit;

@implementation WKWebProcessPlugInNodeHandle {
    API::ObjectStorage<InjectedBundleNodeHandle> _nodeHandle;
}

- (void)dealloc
{
    _nodeHandle->~InjectedBundleNodeHandle();
    [super dealloc];
}

+ (WKWebProcessPlugInNodeHandle *)nodeHandleWithJSValue:(JSValue *)value inContext:(JSContext *)context
{
    JSContextRef contextRef = [context JSGlobalContextRef];
    JSObjectRef objectRef = JSValueToObject(contextRef, [value JSValueRef], 0);
    auto nodeHandle = InjectedBundleNodeHandle::getOrCreate(contextRef, objectRef);
    if (!nodeHandle)
        return nil;

    return [wrapper(*nodeHandle.leakRef()) autorelease];
}

- (WKWebProcessPlugInFrame *)htmlIFrameElementContentFrame
{
    auto frame = _nodeHandle->htmlIFrameElementContentFrame();
    if (!frame)
        return nil;

    return [wrapper(*frame.leakRef()) autorelease];
}

#if PLATFORM(IOS)
- (UIImage *)renderedImageWithOptions:(WKSnapshotOptions)options
{
    return [self renderedImageWithOptions:options width:nil];
}

- (UIImage *)renderedImageWithOptions:(WKSnapshotOptions)options width:(NSNumber *)width
{
    std::optional<float> optionalWidth;
    if (width)
        optionalWidth = width.floatValue;

    RefPtr<WebImage> image = _nodeHandle->renderedImage(toSnapshotOptions(options), options & kWKSnapshotOptionsExcludeOverflow, optionalWidth);
    if (!image)
        return nil;

    return [[[UIImage alloc] initWithCGImage:image->bitmap().makeCGImage().get()] autorelease];
}
#endif

#if PLATFORM(MAC)
- (NSImage *)renderedImageWithOptions:(WKSnapshotOptions)options
{
    return [self renderedImageWithOptions:options width:nil];
}

- (NSImage *)renderedImageWithOptions:(WKSnapshotOptions)options width:(NSNumber *)width
{
    std::optional<float> optionalWidth;
    if (width)
        optionalWidth = width.floatValue;

    RefPtr<WebImage> image = _nodeHandle->renderedImage(toSnapshotOptions(options), options & kWKSnapshotOptionsExcludeOverflow, optionalWidth);
    if (!image)
        return nil;

    return [[[NSImage alloc] initWithCGImage:image->bitmap().makeCGImage().get() size:NSZeroSize] autorelease];
}
#endif

- (CGRect)elementBounds
{
    return _nodeHandle->elementBounds();
}

- (BOOL)HTMLInputElementIsAutoFilled
{
    return _nodeHandle->isHTMLInputElementAutoFilled();
}

- (void)setHTMLInputElementIsAutoFilled:(BOOL)isAutoFilled
{
    _nodeHandle->setHTMLInputElementAutoFilled(isAutoFilled);
}

- (BOOL)isHTMLInputElementAutoFillButtonEnabled
{
    return _nodeHandle->isHTMLInputElementAutoFillButtonEnabled();
}

static WebCore::AutoFillButtonType toAutoFillButtonType(_WKAutoFillButtonType autoFillButtonType)
{
    switch (autoFillButtonType) {
    case _WKAutoFillButtonTypeNone:
        return WebCore::AutoFillButtonType::None;
    case _WKAutoFillButtonTypeContacts:
        return WebCore::AutoFillButtonType::Contacts;
    case _WKAutoFillButtonTypeCredentials:
        return WebCore::AutoFillButtonType::Credentials;
    case _WKAutoFillButtonTypeStrongConfirmationPassword:
        return WebCore::AutoFillButtonType::StrongConfirmationPassword;
    case _WKAutoFillButtonTypeStrongPassword:
        return WebCore::AutoFillButtonType::StrongPassword;
    }
    ASSERT_NOT_REACHED();
    return WebCore::AutoFillButtonType::None;
}

static _WKAutoFillButtonType toWKAutoFillButtonType(WebCore::AutoFillButtonType autoFillButtonType)
{
    switch (autoFillButtonType) {
    case WebCore::AutoFillButtonType::None:
        return _WKAutoFillButtonTypeNone;
    case WebCore::AutoFillButtonType::Contacts:
        return _WKAutoFillButtonTypeContacts;
    case WebCore::AutoFillButtonType::Credentials:
        return _WKAutoFillButtonTypeCredentials;
    case WebCore::AutoFillButtonType::StrongConfirmationPassword:
        return _WKAutoFillButtonTypeStrongConfirmationPassword;
    case WebCore::AutoFillButtonType::StrongPassword:
        return _WKAutoFillButtonTypeStrongPassword;
    }
    ASSERT_NOT_REACHED();
    return _WKAutoFillButtonTypeNone;

}

- (void)setHTMLInputElementAutoFillButtonEnabledWithButtonType:(_WKAutoFillButtonType)autoFillButtonType
{
    _nodeHandle->setHTMLInputElementAutoFillButtonEnabled(toAutoFillButtonType(autoFillButtonType));
}

- (_WKAutoFillButtonType)htmlInputElementAutoFillButtonType
{
    return toWKAutoFillButtonType(_nodeHandle->htmlInputElementAutoFillButtonType());
}

- (_WKAutoFillButtonType)htmlInputElementLastAutoFillButtonType
{
    return toWKAutoFillButtonType(_nodeHandle->htmlInputElementLastAutoFillButtonType());
}

- (BOOL)HTMLInputElementIsUserEdited
{
    return _nodeHandle->htmlInputElementLastChangeWasUserEdit();
}

- (BOOL)HTMLTextAreaElementIsUserEdited
{
    return _nodeHandle->htmlTextAreaElementLastChangeWasUserEdit();
}

- (BOOL)isTextField
{
    return _nodeHandle->isTextField();
}

- (WKWebProcessPlugInNodeHandle *)HTMLTableCellElementCellAbove
{
    auto nodeHandle = _nodeHandle->htmlTableCellElementCellAbove();
    if (!nodeHandle)
        return nil;

    return [wrapper(*nodeHandle.leakRef()) autorelease];
}

- (WKWebProcessPlugInFrame *)frame
{
    return [wrapper(*_nodeHandle->document()->documentFrame().leakRef()) autorelease];
}

- (InjectedBundleNodeHandle&)_nodeHandle
{
    return *_nodeHandle;
}

#pragma mark WKObject protocol implementation

- (API::Object&)_apiObject
{
    return *_nodeHandle;
}

@end

#endif // WK_API_ENABLED
