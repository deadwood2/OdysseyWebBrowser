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

#include "config.h"
#include "WebAutomationSessionProxy.h"

#include "AutomationProtocolObjects.h"
#include "WebAutomationSessionMessages.h"
#include "WebAutomationSessionProxyMessages.h"
#include "WebAutomationSessionProxyScriptSource.h"
#include "WebFrame.h"
#include "WebImage.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSObject.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRefPrivate.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <WebCore/CookieJar.h>
#include <WebCore/DOMWindow.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameTree.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLFrameElementBase.h>
#include <WebCore/JSElement.h>
#include <WebCore/MainFrame.h>
#include <WebCore/UUID.h>

namespace WebKit {

template <typename T>
static JSObjectRef toJSArray(JSContextRef context, const Vector<T>& data, JSValueRef (*converter)(JSContextRef, const T&), JSValueRef* exception)
{
    ASSERT_ARG(converter, converter);

    if (data.isEmpty())
        return JSObjectMakeArray(context, 0, nullptr, exception);

    Vector<JSValueRef, 8> convertedData;
    convertedData.reserveCapacity(data.size());

    for (auto& originalValue : data) {
        JSValueRef convertedValue = converter(context, originalValue);
        JSValueProtect(context, convertedValue);
        convertedData.uncheckedAppend(convertedValue);
    }

    JSObjectRef array = JSObjectMakeArray(context, convertedData.size(), convertedData.data(), exception);

    for (auto& convertedValue : convertedData)
        JSValueUnprotect(context, convertedValue);

    return array;
}

static inline JSRetainPtr<JSStringRef> toJSString(const String& string)
{
    return JSRetainPtr<JSStringRef>(Adopt, OpaqueJSString::create(string).leakRef());
}

static inline JSValueRef toJSValue(JSContextRef context, const String& string)
{
    return JSValueMakeString(context, toJSString(string).get());
}

static inline JSValueRef callPropertyFunction(JSContextRef context, JSObjectRef object, const String& propertyName, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ASSERT_ARG(object, object);
    ASSERT_ARG(object, JSValueIsObject(context, object));

    JSObjectRef function = const_cast<JSObjectRef>(JSObjectGetProperty(context, object, toJSString(propertyName).get(), exception));
    ASSERT(JSObjectIsFunction(context, function));

    return JSObjectCallAsFunction(context, function, object, argumentCount, arguments, exception);
}

WebAutomationSessionProxy::WebAutomationSessionProxy(const String& sessionIdentifier)
    : m_sessionIdentifier(sessionIdentifier)
{
    WebProcess::singleton().addMessageReceiver(Messages::WebAutomationSessionProxy::messageReceiverName(), *this);
}

WebAutomationSessionProxy::~WebAutomationSessionProxy()
{
    WebProcess::singleton().removeMessageReceiver(Messages::WebAutomationSessionProxy::messageReceiverName());
}

static JSValueRef evaluate(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ASSERT_ARG(argumentCount, argumentCount == 1);
    ASSERT_ARG(arguments, JSValueIsString(context, arguments[0]));

    if (argumentCount != 1)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> script(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    return JSEvaluateScript(context, script.get(), nullptr, nullptr, 0, exception);
}

static JSValueRef createUUID(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return toJSValue(context, WebCore::createCanonicalUUIDString().convertToASCIIUppercase());
}

static JSValueRef evaluateJavaScriptCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ASSERT_ARG(argumentCount, argumentCount == 4);
    ASSERT_ARG(arguments, JSValueIsNumber(context, arguments[0]));
    ASSERT_ARG(arguments, JSValueIsNumber(context, arguments[1]));
    ASSERT_ARG(arguments, JSValueIsString(context, arguments[2]));
    ASSERT_ARG(arguments, JSValueIsBoolean(context, arguments[3]));

    auto automationSessionProxy = WebProcess::singleton().automationSessionProxy();
    if (!automationSessionProxy)
        return JSValueMakeUndefined(context);

    uint64_t frameID = JSValueToNumber(context, arguments[0], exception);
    uint64_t callbackID = JSValueToNumber(context, arguments[1], exception);
    JSRetainPtr<JSStringRef> result(Adopt, JSValueToStringCopy(context, arguments[2], exception));

    bool resultIsErrorName = JSValueToBoolean(context, arguments[3]);

    if (resultIsErrorName) {
        if (result->string() == "JavaScriptTimeout") {
            String errorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::JavaScriptTimeout);
            automationSessionProxy->didEvaluateJavaScriptFunction(frameID, callbackID, String(), errorType);
        } else {
            ASSERT_NOT_REACHED();
            String errorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::InternalError);
            automationSessionProxy->didEvaluateJavaScriptFunction(frameID, callbackID, String(), errorType);
        }
    } else
        automationSessionProxy->didEvaluateJavaScriptFunction(frameID, callbackID, result->string(), String());

    return JSValueMakeUndefined(context);
}

JSObjectRef WebAutomationSessionProxy::scriptObjectForFrame(WebFrame& frame)
{
    if (JSObjectRef scriptObject = m_webFrameScriptObjectMap.get(frame.frameID()))
        return scriptObject;

    JSValueRef exception = nullptr;
    JSGlobalContextRef context = frame.jsContext();

    JSValueRef sessionIdentifier = toJSValue(context, m_sessionIdentifier);
    JSObjectRef evaluateFunction = JSObjectMakeFunctionWithCallback(context, nullptr, evaluate);
    JSObjectRef createUUIDFunction = JSObjectMakeFunctionWithCallback(context, nullptr, createUUID);

    String script = StringImpl::createWithoutCopying(WebAutomationSessionProxyScriptSource, sizeof(WebAutomationSessionProxyScriptSource));

    JSObjectRef scriptObjectFunction = const_cast<JSObjectRef>(JSEvaluateScript(context, toJSString(script).get(), nullptr, nullptr, 0, &exception));
    ASSERT(JSValueIsObject(context, scriptObjectFunction));

    JSValueRef arguments[] = { sessionIdentifier, evaluateFunction, createUUIDFunction };
    JSObjectRef scriptObject = const_cast<JSObjectRef>(JSObjectCallAsFunction(context, scriptObjectFunction, nullptr, WTF_ARRAY_LENGTH(arguments), arguments, &exception));
    ASSERT(JSValueIsObject(context, scriptObject));

    JSValueProtect(context, scriptObject);
    m_webFrameScriptObjectMap.add(frame.frameID(), scriptObject);

    return scriptObject;
}

WebCore::Element* WebAutomationSessionProxy::elementForNodeHandle(WebFrame& frame, const String& nodeHandle)
{
    // Don't use scriptObjectForFrame() since we can assume if the script object
    // does not exist, there are no nodes mapped to handles. Using scriptObjectForFrame()
    // will make a new script object if it can't find one, preventing us from returning fast.
    JSObjectRef scriptObject = m_webFrameScriptObjectMap.get(frame.frameID());
    if (!scriptObject)
        return nullptr;

    JSGlobalContextRef context = frame.jsContext();

    JSValueRef functionArguments[] = {
        toJSValue(context, nodeHandle)
    };

    JSValueRef result = callPropertyFunction(context, scriptObject, ASCIILiteral("nodeForIdentifier"), WTF_ARRAY_LENGTH(functionArguments), functionArguments, nullptr);
    JSObjectRef element = JSValueToObject(context, result, nullptr);
    if (!element)
        return nullptr;

    auto elementWrapper = JSC::jsDynamicCast<WebCore::JSElement*>(toJS(element));
    if (!elementWrapper)
        return nullptr;

    return &elementWrapper->wrapped();
}

void WebAutomationSessionProxy::didClearWindowObjectForFrame(WebFrame& frame)
{
    uint64_t frameID = frame.frameID();
    if (JSObjectRef scriptObject = m_webFrameScriptObjectMap.take(frameID))
        JSValueUnprotect(frame.jsContext(), scriptObject);

    String errorMessage = ASCIILiteral("Callback was not called before the unload event.");
    String errorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::JavaScriptError);

    auto pendingFrameCallbacks = m_webFramePendingEvaluateJavaScriptCallbacksMap.take(frameID);
    for (uint64_t callbackID : pendingFrameCallbacks)
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidEvaluateJavaScriptFunction(callbackID, String(), errorType), 0);
}

void WebAutomationSessionProxy::evaluateJavaScriptFunction(uint64_t pageID, uint64_t frameID, const String& function, Vector<String> arguments, bool expectsImplicitCallbackArgument, int callbackTimeout, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page)
        return;

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame)
        return;

    JSObjectRef scriptObject = scriptObjectForFrame(*frame);
    if (!scriptObject)
        return;

    JSValueRef exception = nullptr;
    JSGlobalContextRef context = frame->jsContext();

    if (expectsImplicitCallbackArgument) {
        auto result = m_webFramePendingEvaluateJavaScriptCallbacksMap.add(frameID, Vector<uint64_t>());
        result.iterator->value.append(callbackID);
    }

    JSValueRef functionArguments[] = {
        toJSValue(context, function),
        toJSArray(context, arguments, toJSValue, &exception),
        JSValueMakeBoolean(context, expectsImplicitCallbackArgument),
        JSValueMakeNumber(context, frameID),
        JSValueMakeNumber(context, callbackID),
        JSObjectMakeFunctionWithCallback(context, nullptr, evaluateJavaScriptCallback),
        JSValueMakeNumber(context, callbackTimeout)
    };

    callPropertyFunction(context, scriptObject, ASCIILiteral("evaluateJavaScriptFunction"), WTF_ARRAY_LENGTH(functionArguments), functionArguments, &exception);

    if (!exception)
        return;

    String errorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::JavaScriptError);

    JSRetainPtr<JSStringRef> exceptionMessage;
    if (JSValueIsObject(context, exception)) {
        JSValueRef nameValue = JSObjectGetProperty(context, const_cast<JSObjectRef>(exception), toJSString(ASCIILiteral("name")).get(), nullptr);
        JSRetainPtr<JSStringRef> exceptionName(Adopt, JSValueToStringCopy(context, nameValue, nullptr));
        if (exceptionName->string() == "NodeNotFound")
            errorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::NodeNotFound);
        else if (exceptionName->string() == "InvalidElementState")
            errorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::InvalidElementState);        

        JSValueRef messageValue = JSObjectGetProperty(context, const_cast<JSObjectRef>(exception), toJSString(ASCIILiteral("message")).get(), nullptr);
        exceptionMessage.adopt(JSValueToStringCopy(context, messageValue, nullptr));
    } else
        exceptionMessage.adopt(JSValueToStringCopy(context, exception, nullptr));

    didEvaluateJavaScriptFunction(frameID, callbackID, exceptionMessage->string(), errorType);
}

void WebAutomationSessionProxy::didEvaluateJavaScriptFunction(uint64_t frameID, uint64_t callbackID, const String& result, const String& errorType)
{
    auto findResult = m_webFramePendingEvaluateJavaScriptCallbacksMap.find(frameID);
    if (findResult != m_webFramePendingEvaluateJavaScriptCallbacksMap.end()) {
        findResult->value.removeFirst(callbackID);
        ASSERT(!findResult->value.contains(callbackID));
        if (findResult->value.isEmpty())
            m_webFramePendingEvaluateJavaScriptCallbacksMap.remove(findResult);
    }

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidEvaluateJavaScriptFunction(callbackID, result, errorType), 0);
}

void WebAutomationSessionProxy::resolveChildFrameWithOrdinal(uint64_t pageID, uint64_t frameID, uint32_t ordinal, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, windowNotFoundErrorType), 0);
        return;
    }

    String frameNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::FrameNotFound);

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebCore::Frame* coreFrame = frame->coreFrame();
    if (!coreFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebCore::Frame* coreChildFrame = coreFrame->tree().scopedChild(ordinal);
    if (!coreChildFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebFrame* childFrame = WebFrame::fromCoreFrame(*coreChildFrame);
    if (!childFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, childFrame->frameID(), String()), 0);
}

void WebAutomationSessionProxy::resolveChildFrameWithNodeHandle(uint64_t pageID, uint64_t frameID, const String& nodeHandle, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, windowNotFoundErrorType), 0);
        return;
    }

    String frameNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::FrameNotFound);

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebCore::Element* coreElement = elementForNodeHandle(*frame, nodeHandle);
    if (!coreElement || !coreElement->isFrameElementBase()) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebCore::Frame* coreFrameFromElement = static_cast<WebCore::HTMLFrameElementBase*>(coreElement)->contentFrame();
    if (!coreFrameFromElement) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebFrame* frameFromElement = WebFrame::fromCoreFrame(*coreFrameFromElement);
    if (!frameFromElement) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, frameFromElement->frameID(), String()), 0);
}

void WebAutomationSessionProxy::resolveChildFrameWithName(uint64_t pageID, uint64_t frameID, const String& name, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, windowNotFoundErrorType), 0);
        return;
    }

    String frameNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::FrameNotFound);

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebCore::Frame* coreFrame = frame->coreFrame();
    if (!coreFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebCore::Frame* coreChildFrame = coreFrame->tree().scopedChild(name);
    if (!coreChildFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebFrame* childFrame = WebFrame::fromCoreFrame(*coreChildFrame);
    if (!childFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveChildFrame(callbackID, childFrame->frameID(), String()), 0);
}

void WebAutomationSessionProxy::resolveParentFrame(uint64_t pageID, uint64_t frameID, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveParentFrame(callbackID, 0, windowNotFoundErrorType), 0);
        return;
    }

    String frameNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::FrameNotFound);

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveParentFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebFrame* parentFrame = frame->parentFrame();
    if (!parentFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveParentFrame(callbackID, 0, frameNotFoundErrorType), 0);
        return;
    }

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidResolveParentFrame(callbackID, parentFrame->frameID(), String()), 0);
}

void WebAutomationSessionProxy::focusFrame(uint64_t pageID, uint64_t frameID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page)
        return;

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame)
        return;

    WebCore::Frame* coreFrame = frame->coreFrame();
    if (!coreFrame)
        return;

    WebCore::Document* coreDocument = coreFrame->document();
    if (!coreDocument)
        return;

    WebCore::DOMWindow* coreDOMWindow = coreDocument->domWindow();
    if (!coreDOMWindow)
        return;

    coreDOMWindow->focus(true);
}

void WebAutomationSessionProxy::computeElementLayout(uint64_t pageID, uint64_t frameID, String nodeHandle, bool scrollIntoViewIfNeeded, bool useViewportCoordinates, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidComputeElementLayout(callbackID, WebCore::IntRect(), windowNotFoundErrorType), 0);
        return;
    }

    String frameNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::FrameNotFound);
    String nodeNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::NodeNotFound);

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidComputeElementLayout(callbackID, WebCore::IntRect(), frameNotFoundErrorType), 0);
        return;
    }

    WebCore::Element* coreElement = elementForNodeHandle(*frame, nodeHandle);
    if (!coreElement) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidComputeElementLayout(callbackID, WebCore::IntRect(), nodeNotFoundErrorType), 0);
        return;
    }

    if (scrollIntoViewIfNeeded)
        coreElement->scrollIntoViewIfNeeded(false);

    WebCore::IntRect rect = coreElement->clientRect();

    WebCore::Frame* coreFrame = frame->coreFrame();
    if (!coreFrame) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidComputeElementLayout(callbackID, WebCore::IntRect(), frameNotFoundErrorType), 0);
        return;
    }

    WebCore::FrameView *coreFrameView = coreFrame->view();
    if (!coreFrameView) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidComputeElementLayout(callbackID, WebCore::IntRect(), frameNotFoundErrorType), 0);
        return;
    }

    if (useViewportCoordinates)
        rect.moveBy(WebCore::IntPoint(0, -coreFrameView->topContentInset()));
    else
        rect = coreFrameView->rootViewToContents(rect);

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidComputeElementLayout(callbackID, rect, String()), 0);
}

void WebAutomationSessionProxy::takeScreenshot(uint64_t pageID, uint64_t callbackID)
{
    ShareableBitmap::Handle handle;

    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidTakeScreenshot(callbackID, handle, windowNotFoundErrorType), 0);
        return;
    }

    WebCore::FrameView* frameView = page->mainFrameView();
    if (!frameView) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidTakeScreenshot(callbackID, handle, String()), 0);
        return;
    }

    WebCore::IntRect snapshotRect = WebCore::IntRect(WebCore::IntPoint(0, 0), frameView->contentsSize());
    if (snapshotRect.isEmpty()) {
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidTakeScreenshot(callbackID, handle, String()), 0);
        return;
    }

    RefPtr<WebImage> image = page->scaledSnapshotWithOptions(snapshotRect, 1, SnapshotOptionsShareable);
    if (image)
        image->bitmap()->createHandle(handle, SharedMemory::Protection::ReadOnly);

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidTakeScreenshot(callbackID, handle, String()), 0);    
}

void WebAutomationSessionProxy::getCookiesForFrame(uint64_t pageID, uint64_t frameID, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidGetCookiesForFrame(callbackID, Vector<WebCore::Cookie>(), windowNotFoundErrorType), 0);
        return;
    }

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame || !frame->coreFrame() || !frame->coreFrame()->document()) {
        String frameNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::FrameNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidGetCookiesForFrame(callbackID, Vector<WebCore::Cookie>(), frameNotFoundErrorType), 0);
        return;
    }

    // This returns the same list of cookies as when evaluating `document.cookies` in JavaScript.
    auto& document = *frame->coreFrame()->document();
    Vector<WebCore::Cookie> foundCookies;
    WebCore::getRawCookies(document, document.cookieURL(), foundCookies);

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidGetCookiesForFrame(callbackID, foundCookies, String()), 0);
}

void WebAutomationSessionProxy::deleteCookie(uint64_t pageID, uint64_t frameID, String cookieName, uint64_t callbackID)
{
    WebPage* page = WebProcess::singleton().webPage(pageID);
    if (!page) {
        String windowNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::WindowNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidDeleteCookie(callbackID, windowNotFoundErrorType), 0);
        return;
    }

    WebFrame* frame = frameID ? WebProcess::singleton().webFrame(frameID) : page->mainWebFrame();
    if (!frame || !frame->coreFrame() || !frame->coreFrame()->document()) {
        String frameNotFoundErrorType = Inspector::Protocol::AutomationHelpers::getEnumConstantValue(Inspector::Protocol::Automation::ErrorMessage::FrameNotFound);
        WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidDeleteCookie(callbackID, frameNotFoundErrorType), 0);
        return;
    }

    auto& document = *frame->coreFrame()->document();
    WebCore::deleteCookie(document, document.cookieURL(), cookieName);

    WebProcess::singleton().parentProcessConnection()->send(Messages::WebAutomationSession::DidDeleteCookie(callbackID, String()), 0);
}

} // namespace WebKit
