/*
 *  Copyright (C) 2004-2016 Apple Inc. All rights reserved.
 *  Copyright (C) 2005-2007 Alexey Proskuryakov <ap@webkit.org>
 *  Copyright (C) 2007, 2008 Julien Chaffraix <jchaffraix@webkit.org>
 *  Copyright (C) 2008, 2011 Google Inc. All rights reserved.
 *  Copyright (C) 2012 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "XMLHttpRequest.h"

#include "Blob.h"
#include "CachedResourceRequestInitiators.h"
#include "ContentSecurityPolicy.h"
#include "CrossOriginAccessControl.h"
#include "DOMFormData.h"
#include "DOMImplementation.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "File.h"
#include "HTMLDocument.h"
#include "HTTPHeaderNames.h"
#include "HTTPParsers.h"
#include "InspectorInstrumentation.h"
#include "JSDOMBinding.h"
#include "JSDOMWindow.h"
#include "MemoryCache.h"
#include "ParsedContentType.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ScriptController.h"
#include "SecurityOriginPolicy.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"
#include "ThreadableLoader.h"
#include "XMLDocument.h"
#include "XMLHttpRequestProgressEvent.h"
#include "XMLHttpRequestUpload.h"
#include "markup.h"
#include <heap/Strong.h>
#include <mutex>
#include <runtime/ArrayBuffer.h>
#include <runtime/ArrayBufferView.h>
#include <runtime/JSCInlines.h>
#include <runtime/JSLock.h>
#include <wtf/Ref.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

namespace WebCore {

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, xmlHttpRequestCounter, ("XMLHttpRequest"));

// Histogram enum to see when we can deprecate xhr.send(ArrayBuffer).
enum XMLHttpRequestSendArrayBufferOrView {
    XMLHttpRequestSendArrayBuffer,
    XMLHttpRequestSendArrayBufferView,
    XMLHttpRequestSendArrayBufferOrViewMax,
};

static void replaceCharsetInMediaType(String& mediaType, const String& charsetValue)
{
    unsigned pos = 0, len = 0;

    findCharsetInMediaType(mediaType, pos, len);

    if (!len) {
        // When no charset found, do nothing.
        return;
    }

    // Found at least one existing charset, replace all occurrences with new charset.
    while (len) {
        mediaType.replace(pos, len, charsetValue);
        unsigned start = pos + charsetValue.length();
        findCharsetInMediaType(mediaType, pos, len, start);
    }
}

static void logConsoleError(ScriptExecutionContext* context, const String& message)
{
    if (!context)
        return;
    // FIXME: It's not good to report the bad usage without indicating what source line it came from.
    // We should pass additional parameters so we can tell the console where the mistake occurred.
    context->addConsoleMessage(MessageSource::JS, MessageLevel::Error, message);
}

Ref<XMLHttpRequest> XMLHttpRequest::create(ScriptExecutionContext& context)
{
    auto xmlHttpRequest = adoptRef(*new XMLHttpRequest(context));
    xmlHttpRequest->suspendIfNeeded();
    return xmlHttpRequest;
}

XMLHttpRequest::XMLHttpRequest(ScriptExecutionContext& context)
    : ActiveDOMObject(&context)
    , m_progressEventThrottle(this)
    , m_resumeTimer(*this, &XMLHttpRequest::resumeTimerFired)
    , m_networkErrorTimer(*this, &XMLHttpRequest::networkErrorTimerFired)
    , m_timeoutTimer(*this, &XMLHttpRequest::didReachTimeout)
{
#ifndef NDEBUG
    xmlHttpRequestCounter.increment();
#endif
}

XMLHttpRequest::~XMLHttpRequest()
{
#ifndef NDEBUG
    xmlHttpRequestCounter.decrement();
#endif
}

Document* XMLHttpRequest::document() const
{
    ASSERT(scriptExecutionContext());
    return downcast<Document>(scriptExecutionContext());
}

SecurityOrigin* XMLHttpRequest::securityOrigin() const
{
    return scriptExecutionContext()->securityOrigin();
}

#if ENABLE(DASHBOARD_SUPPORT)

bool XMLHttpRequest::usesDashboardBackwardCompatibilityMode() const
{
    if (scriptExecutionContext()->isWorkerGlobalScope())
        return false;
    Settings* settings = document()->settings();
    return settings && settings->usesDashboardBackwardCompatibilityMode();
}

#endif

XMLHttpRequest::State XMLHttpRequest::readyState() const
{
    return m_state;
}

String XMLHttpRequest::responseText(ExceptionCode& ec)
{
    if (m_responseType != ResponseType::EmptyString && m_responseType != ResponseType::Text) {
        ec = INVALID_STATE_ERR;
        return { };
    }
    return responseTextIgnoringResponseType();
}

void XMLHttpRequest::didCacheResponse()
{
    ASSERT(doneWithoutErrors());
    m_responseCacheIsValid = true;
    m_responseBuilder.clear();
}

Document* XMLHttpRequest::responseXML(ExceptionCode& ec)
{
    if (m_responseType != ResponseType::EmptyString && m_responseType != ResponseType::Document) {
        ec = INVALID_STATE_ERR;
        return nullptr;
    }

    if (!doneWithoutErrors())
        return nullptr;

    if (!m_createdDocument) {
        String mimeType = responseMIMEType();
        bool isHTML = equalLettersIgnoringASCIICase(mimeType, "text/html");

        // The W3C spec requires the final MIME type to be some valid XML type, or text/html.
        // If it is text/html, then the responseType of "document" must have been supplied explicitly.
        if ((m_response.isHTTP() && !responseIsXML() && !isHTML)
            || (isHTML && m_responseType == ResponseType::EmptyString)
            || scriptExecutionContext()->isWorkerGlobalScope()) {
            m_responseDocument = nullptr;
        } else {
            if (isHTML)
                m_responseDocument = HTMLDocument::create(0, m_url);
            else
                m_responseDocument = XMLDocument::create(0, m_url);
            // FIXME: Set Last-Modified.
            m_responseDocument->setContent(m_responseBuilder.toStringPreserveCapacity());
            m_responseDocument->setSecurityOriginPolicy(scriptExecutionContext()->securityOriginPolicy());
            m_responseDocument->overrideMIMEType(mimeType);

            if (!m_responseDocument->wellFormed())
                m_responseDocument = nullptr;
        }
        m_createdDocument = true;
    }

    return m_responseDocument.get();
}

Ref<Blob> XMLHttpRequest::createResponseBlob()
{
    ASSERT(m_responseType == ResponseType::Blob);
    ASSERT(doneWithoutErrors());

    if (!m_binaryResponseBuilder)
        return Blob::create();

    // FIXME: We just received the data from NetworkProcess, and are sending it back. This is inefficient.
    Vector<uint8_t> data;
    data.append(m_binaryResponseBuilder->data(), m_binaryResponseBuilder->size());
    m_binaryResponseBuilder = nullptr;
    String normalizedContentType = Blob::normalizedContentType(responseMIMEType()); // responseMIMEType defaults to text/xml which may be incorrect.
    return Blob::create(WTFMove(data), normalizedContentType);
}

RefPtr<ArrayBuffer> XMLHttpRequest::createResponseArrayBuffer()
{
    ASSERT(m_responseType == ResponseType::Arraybuffer);
    ASSERT(doneWithoutErrors());

    auto result = m_binaryResponseBuilder ? m_binaryResponseBuilder->createArrayBuffer() : ArrayBuffer::create(nullptr, 0);
    m_binaryResponseBuilder = nullptr;
    return result;
}

void XMLHttpRequest::setTimeout(unsigned timeout, ExceptionCode& ec)
{
    if (scriptExecutionContext()->isDocument() && !m_async) {
        logConsoleError(scriptExecutionContext(), "XMLHttpRequest.timeout cannot be set for synchronous HTTP(S) requests made from the window context.");
        ec = INVALID_ACCESS_ERR;
        return;
    }
    m_timeoutMilliseconds = timeout;
    if (!m_timeoutTimer.isActive())
        return;
    if (!m_timeoutMilliseconds) {
        m_timeoutTimer.stop();
        return;
    }
    std::chrono::duration<double> interval = std::chrono::milliseconds { m_timeoutMilliseconds } - (std::chrono::steady_clock::now() - m_sendingTime);
    m_timeoutTimer.startOneShot(std::max(0.0, interval.count()));
}

void XMLHttpRequest::setResponseType(ResponseType type, ExceptionCode& ec)
{
    if (m_state >= LOADING) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // Newer functionality is not available to synchronous requests in window contexts, as a spec-mandated
    // attempt to discourage synchronous XHR use. responseType is one such piece of functionality.
    // We'll only disable this functionality for HTTP(S) requests since sync requests for local protocols
    // such as file: and data: still make sense to allow.
    if (!m_async && scriptExecutionContext()->isDocument() && m_url.protocolIsInHTTPFamily()) {
        logConsoleError(scriptExecutionContext(), "XMLHttpRequest.responseType cannot be changed for synchronous HTTP(S) requests made from the window context.");
        ec = INVALID_ACCESS_ERR;
        return;
    }

    m_responseType = type;
}

String XMLHttpRequest::responseURL() const
{
    URL responseURL(m_response.url());
    responseURL.removeFragmentIdentifier();

    return responseURL.string();
}

void XMLHttpRequest::setLastSendLineAndColumnNumber(unsigned lineNumber, unsigned columnNumber)
{
    m_lastSendLineNumber = lineNumber;
    m_lastSendColumnNumber = columnNumber;
}

XMLHttpRequestUpload* XMLHttpRequest::upload()
{
    if (!m_upload)
        m_upload = std::make_unique<XMLHttpRequestUpload>(this);
    return m_upload.get();
}

void XMLHttpRequest::changeState(State newState)
{
    if (m_state != newState) {
        m_state = newState;
        callReadyStateChangeListener();
    }
}

void XMLHttpRequest::callReadyStateChangeListener()
{
    if (!scriptExecutionContext())
        return;

    // Check whether sending load and loadend events before sending readystatechange event, as it may change m_error/m_state values.
    bool shouldSendLoadEvent = (m_state == DONE && !m_error);

    if (m_async || (m_state <= OPENED || m_state == DONE))
        m_progressEventThrottle.dispatchReadyStateChangeEvent(Event::create(eventNames().readystatechangeEvent, false, false), m_state == DONE ? FlushProgressEvent : DoNotFlushProgressEvent);

    if (shouldSendLoadEvent) {
        m_progressEventThrottle.dispatchProgressEvent(eventNames().loadEvent);
        m_progressEventThrottle.dispatchProgressEvent(eventNames().loadendEvent);
    }
}

void XMLHttpRequest::setWithCredentials(bool value, ExceptionCode& ec)
{
    if (m_state > OPENED || m_sendFlag) {
        ec = INVALID_STATE_ERR;
        return;
    }

    m_includeCredentials = value;
}

bool XMLHttpRequest::isAllowedHTTPMethod(const String& method)
{
    return !equalLettersIgnoringASCIICase(method, "trace")
        && !equalLettersIgnoringASCIICase(method, "track")
        && !equalLettersIgnoringASCIICase(method, "connect");
}

String XMLHttpRequest::uppercaseKnownHTTPMethod(const String& method)
{
    const char* const methods[] = { "DELETE", "GET", "HEAD", "OPTIONS", "POST", "PUT" };
    for (auto* value : methods) {
        if (equalIgnoringASCIICase(method, value)) {
            // Don't bother allocating a new string if it's already all uppercase.
            if (method == value)
                break;
            return ASCIILiteral(value);
        }
    }
    return method;
}

static bool isForbiddenRequestHeader(const String& name)
{
    HTTPHeaderName headerName;
    if (!findHTTPHeaderName(name, headerName))
        return false;

    switch (headerName) {
    case HTTPHeaderName::AcceptCharset:
    case HTTPHeaderName::AcceptEncoding:
    case HTTPHeaderName::AccessControlRequestHeaders:
    case HTTPHeaderName::AccessControlRequestMethod:
    case HTTPHeaderName::Connection:
    case HTTPHeaderName::ContentLength:
    case HTTPHeaderName::ContentTransferEncoding:
    case HTTPHeaderName::Cookie:
    case HTTPHeaderName::Cookie2:
    case HTTPHeaderName::Date:
    case HTTPHeaderName::DNT:
    case HTTPHeaderName::Expect:
    case HTTPHeaderName::Host:
    case HTTPHeaderName::KeepAlive:
    case HTTPHeaderName::Origin:
    case HTTPHeaderName::Referer:
    case HTTPHeaderName::TE:
    case HTTPHeaderName::Trailer:
    case HTTPHeaderName::TransferEncoding:
    case HTTPHeaderName::Upgrade:
    case HTTPHeaderName::UserAgent:
    case HTTPHeaderName::Via:
        return true;

    default:
        return false;
    }
}

bool XMLHttpRequest::isAllowedHTTPHeader(const String& name)
{
    if (isForbiddenRequestHeader(name))
        return false;

    if (name.startsWith("proxy-", false))
        return false;

    if (name.startsWith("sec-", false))
        return false;

    return true;
}

void XMLHttpRequest::open(const String& method, const String& url, ExceptionCode& ec)
{
    // If the async argument is omitted, set async to true.
    return open(method, scriptExecutionContext()->completeURL(url), true, ec);
}

void XMLHttpRequest::open(const String& method, const URL& url, bool async, ExceptionCode& ec)
{
    if (!internalAbort())
        return;

    State previousState = m_state;
    m_state = UNSENT;
    m_error = false;
    m_sendFlag = false;
    m_uploadComplete = false;

    // clear stuff from possible previous load
    clearResponse();
    clearRequest();

    ASSERT(m_state == UNSENT);

    if (!isValidHTTPToken(method)) {
        ec = SYNTAX_ERR;
        return;
    }

    if (!isAllowedHTTPMethod(method)) {
        ec = SECURITY_ERR;
        return;
    }

    if (!async && scriptExecutionContext()->isDocument()) {
        if (document()->settings() && !document()->settings()->syncXHRInDocumentsEnabled()) {
            logConsoleError(scriptExecutionContext(), "Synchronous XMLHttpRequests are disabled for this page.");
            ec = INVALID_ACCESS_ERR;
            return;
        }

        // Newer functionality is not available to synchronous requests in window contexts, as a spec-mandated
        // attempt to discourage synchronous XHR use. responseType is one such piece of functionality.
        // We'll only disable this functionality for HTTP(S) requests since sync requests for local protocols
        // such as file: and data: still make sense to allow.
        if (url.protocolIsInHTTPFamily() && m_responseType != ResponseType::EmptyString) {
            logConsoleError(scriptExecutionContext(), "Synchronous HTTP(S) requests made from the window context cannot have XMLHttpRequest.responseType set.");
            ec = INVALID_ACCESS_ERR;
            return;
        }

        // Similarly, timeouts are disabled for synchronous requests as well.
        if (m_timeoutMilliseconds > 0) {
            logConsoleError(scriptExecutionContext(), "Synchronous XMLHttpRequests must not have a timeout value set.");
            ec = INVALID_ACCESS_ERR;
            return;
        }
    }

    m_method = uppercaseKnownHTTPMethod(method);

    m_url = url;
    scriptExecutionContext()->contentSecurityPolicy()->upgradeInsecureRequestIfNeeded(m_url, ContentSecurityPolicy::InsecureRequestType::Load);

    m_async = async;

    ASSERT(!m_loader);

    // Check previous state to avoid dispatching readyState event
    // when calling open several times in a row.
    if (previousState != OPENED)
        changeState(OPENED);
    else
        m_state = OPENED;
}

void XMLHttpRequest::open(const String& method, const String& url, bool async, const String& user, const String& password, ExceptionCode& ec)
{
    URL urlWithCredentials = scriptExecutionContext()->completeURL(url);
    if (!user.isNull()) {
        urlWithCredentials.setUser(user);
        if (!password.isNull())
            urlWithCredentials.setPass(password);
    }

    open(method, urlWithCredentials, async, ec);
}

bool XMLHttpRequest::initSend(ExceptionCode& ec)
{
    if (!scriptExecutionContext())
        return false;

    if (m_state != OPENED || m_sendFlag) {
        ec = INVALID_STATE_ERR;
        return false;
    }
    ASSERT(!m_loader);

    // FIXME: Convert this to check the isolated world's Content Security Policy once webkit.org/b/104520 is solved.
    if (!scriptExecutionContext()->contentSecurityPolicy()->allowConnectToSource(m_url, scriptExecutionContext()->shouldBypassMainWorldContentSecurityPolicy())) {
        if (m_async) {
            setPendingActivity(this);
            m_timeoutTimer.stop();
            m_networkErrorTimer.startOneShot(0);
        } else
            ec = NETWORK_ERR;
        return false;
    }

    m_error = false;
    return true;
}

void XMLHttpRequest::send(ExceptionCode& ec)
{
    send(String(), ec);
}

void XMLHttpRequest::send(Document* document, ExceptionCode& ec)
{
    ASSERT(document);

    if (!initSend(ec))
        return;

    if (m_method != "GET" && m_method != "HEAD" && m_url.protocolIsInHTTPFamily()) {
        if (!m_requestHeaders.contains(HTTPHeaderName::ContentType)) {
#if ENABLE(DASHBOARD_SUPPORT)
            if (usesDashboardBackwardCompatibilityMode())
                m_requestHeaders.set(HTTPHeaderName::ContentType, ASCIILiteral("application/x-www-form-urlencoded"));
            else
#endif
                // FIXME: this should include the charset used for encoding.
                m_requestHeaders.set(HTTPHeaderName::ContentType, document->isHTMLDocument() ? ASCIILiteral("text/html;charset=UTF-8") : ASCIILiteral("application/xml;charset=UTF-8"));
        }

        // FIXME: According to XMLHttpRequest Level 2, this should use the Document.innerHTML algorithm
        // from the HTML5 specification to serialize the document.
        String body = createMarkup(*document);

        m_requestEntityBody = FormData::create(UTF8Encoding().encode(body, EntitiesForUnencodables));
        if (m_upload)
            m_requestEntityBody->setAlwaysStream(true);
    }

    createRequest(ec);
}

void XMLHttpRequest::send(const String& body, ExceptionCode& ec)
{
    if (!initSend(ec))
        return;

    if (!body.isNull() && m_method != "GET" && m_method != "HEAD" && m_url.protocolIsInHTTPFamily()) {
        String contentType = m_requestHeaders.get(HTTPHeaderName::ContentType);
        if (contentType.isNull()) {
#if ENABLE(DASHBOARD_SUPPORT)
            if (usesDashboardBackwardCompatibilityMode())
                m_requestHeaders.set(HTTPHeaderName::ContentType, ASCIILiteral("application/x-www-form-urlencoded"));
            else
#endif
                m_requestHeaders.set(HTTPHeaderName::ContentType, ASCIILiteral("text/plain;charset=UTF-8"));
        } else {
            replaceCharsetInMediaType(contentType, "UTF-8");
            m_requestHeaders.set(HTTPHeaderName::ContentType, contentType);
        }

        m_requestEntityBody = FormData::create(UTF8Encoding().encode(body, EntitiesForUnencodables));
        if (m_upload)
            m_requestEntityBody->setAlwaysStream(true);
    }

    createRequest(ec);
}

void XMLHttpRequest::send(Blob* body, ExceptionCode& ec)
{
    if (!initSend(ec))
        return;

    if (m_method != "GET" && m_method != "HEAD" && m_url.protocolIsInHTTPFamily()) {
        if (!m_requestHeaders.contains(HTTPHeaderName::ContentType)) {
            const String& blobType = body->type();
            if (!blobType.isEmpty() && isValidContentType(blobType))
                m_requestHeaders.set(HTTPHeaderName::ContentType, blobType);
            else {
                // From FileAPI spec, whenever media type cannot be determined, empty string must be returned.
                m_requestHeaders.set(HTTPHeaderName::ContentType, emptyString());
            }
        }

        m_requestEntityBody = FormData::create();
        m_requestEntityBody->appendBlob(body->url());
    }

    createRequest(ec);
}

void XMLHttpRequest::send(DOMFormData* body, ExceptionCode& ec)
{
    if (!initSend(ec))
        return;

    if (m_method != "GET" && m_method != "HEAD" && m_url.protocolIsInHTTPFamily()) {
        m_requestEntityBody = FormData::createMultiPart(*(static_cast<FormDataList*>(body)), body->encoding(), document());

        m_requestEntityBody->generateFiles(document());

        if (!m_requestHeaders.contains(HTTPHeaderName::ContentType))
            m_requestHeaders.set(HTTPHeaderName::ContentType, makeString("multipart/form-data; boundary=", m_requestEntityBody->boundary().data()));
    }

    createRequest(ec);
}

void XMLHttpRequest::send(ArrayBuffer* body, ExceptionCode& ec)
{
    String consoleMessage("ArrayBuffer is deprecated in XMLHttpRequest.send(). Use ArrayBufferView instead.");
    scriptExecutionContext()->addConsoleMessage(MessageSource::JS, MessageLevel::Warning, consoleMessage);

    sendBytesData(body->data(), body->byteLength(), ec);
}

void XMLHttpRequest::send(ArrayBufferView* body, ExceptionCode& ec)
{
    sendBytesData(body->baseAddress(), body->byteLength(), ec);
}

void XMLHttpRequest::sendBytesData(const void* data, size_t length, ExceptionCode& ec)
{
    if (!initSend(ec))
        return;

    if (m_method != "GET" && m_method != "HEAD" && m_url.protocolIsInHTTPFamily()) {
        m_requestEntityBody = FormData::create(data, length);
        if (m_upload)
            m_requestEntityBody->setAlwaysStream(true);
    }

    createRequest(ec);
}

void XMLHttpRequest::createRequest(ExceptionCode& ec)
{
    // Only GET request is supported for blob URL.
    if (!m_async && m_url.protocolIsBlob() && m_method != "GET") {
        ec = NETWORK_ERR;
        return;
    }

    m_sendFlag = true;

    // The presence of upload event listeners forces us to use preflighting because POSTing to an URL that does not
    // permit cross origin requests should look exactly like POSTing to an URL that does not respond at all.
    // Also, only async requests support upload progress events.
    bool uploadEvents = false;
    if (m_async) {
        m_progressEventThrottle.dispatchProgressEvent(eventNames().loadstartEvent);
        if (m_requestEntityBody && m_upload) {
            uploadEvents = m_upload->hasEventListeners();
            m_upload->dispatchProgressEvent(eventNames().loadstartEvent);
        }
    }

    m_sameOriginRequest = securityOrigin()->canRequest(m_url);

    // We also remember whether upload events should be allowed for this request in case the upload listeners are
    // added after the request is started.
    m_uploadEventsAllowed = m_sameOriginRequest || uploadEvents || !isSimpleCrossOriginAccessRequest(m_method, m_requestHeaders);

    ResourceRequest request(m_url);
    request.setRequester(ResourceRequest::Requester::XHR);
    request.setHTTPMethod(m_method);

    if (m_requestEntityBody) {
        ASSERT(m_method != "GET");
        ASSERT(m_method != "HEAD");
        request.setHTTPBody(WTFMove(m_requestEntityBody));
    }

    if (!m_requestHeaders.isEmpty())
        request.setHTTPHeaderFields(m_requestHeaders);

    ThreadableLoaderOptions options;
    options.sendLoadCallbacks = SendCallbacks;
    options.preflightPolicy = uploadEvents ? ForcePreflight : ConsiderPreflight;
    options.credentials = m_includeCredentials ? FetchOptions::Credentials::Include : FetchOptions::Credentials::SameOrigin;
    options.mode = FetchOptions::Mode::Cors;
    options.contentSecurityPolicyEnforcement = scriptExecutionContext()->shouldBypassMainWorldContentSecurityPolicy() ? ContentSecurityPolicyEnforcement::DoNotEnforce : ContentSecurityPolicyEnforcement::EnforceConnectSrcDirective;
    options.initiator = cachedResourceRequestInitiators().xmlhttprequest;
    options.sameOriginDataURLFlag = SameOriginDataURLFlag::Set;

    if (m_timeoutMilliseconds) {
        if (!m_async)
            request.setTimeoutInterval(m_timeoutMilliseconds / 1000.0);
        else {
            m_sendingTime = std::chrono::steady_clock::now();
            m_timeoutTimer.startOneShot(std::chrono::milliseconds { m_timeoutMilliseconds });
        }
    }

    m_exceptionCode = 0;
    m_error = false;

    if (m_async) {
        if (m_upload)
            request.setReportUploadProgress(true);

        // ThreadableLoader::create can return null here, for example if we're no longer attached to a page or if a content blocker blocks the load.
        // This is true while running onunload handlers.
        // FIXME: Maybe we need to be able to send XMLHttpRequests from onunload, <http://bugs.webkit.org/show_bug.cgi?id=10904>.
        m_loader = ThreadableLoader::create(*scriptExecutionContext(), *this, WTFMove(request), options);

        // Either loader is null or some error was synchronously sent to us.
        ASSERT(m_loader || !m_sendFlag);

        // Neither this object nor the JavaScript wrapper should be deleted while
        // a request is in progress because we need to keep the listeners alive,
        // and they are referenced by the JavaScript wrapper.
        if (m_loader)
            setPendingActivity(this);
    } else {
        InspectorInstrumentation::willLoadXHRSynchronously(scriptExecutionContext());
        ThreadableLoader::loadResourceSynchronously(*scriptExecutionContext(), WTFMove(request), *this, options);
        InspectorInstrumentation::didLoadXHRSynchronously(scriptExecutionContext());
    }

    if (!m_exceptionCode && m_error)
        m_exceptionCode = NETWORK_ERR;
    ec = m_exceptionCode;
}

void XMLHttpRequest::abort()
{
    // internalAbort() calls dropProtection(), which may release the last reference.
    Ref<XMLHttpRequest> protectedThis(*this);

    if (!internalAbort())
        return;

    clearResponseBuffers();

    // Clear headers as required by the spec
    m_requestHeaders.clear();
    if ((m_state == OPENED && m_sendFlag) || m_state == HEADERS_RECEIVED || m_state == LOADING) {
        ASSERT(!m_loader);
        m_sendFlag = false;
        changeState(DONE);
        dispatchErrorEvents(eventNames().abortEvent);
    }
    m_state = UNSENT;
}

bool XMLHttpRequest::internalAbort()
{
    m_error = true;

    // FIXME: when we add the support for multi-part XHR, we will have to think be careful with this initialization.
    m_receivedLength = 0;

    m_decoder = nullptr;

    m_timeoutTimer.stop();

    if (!m_loader)
        return true;

    // Cancelling m_loader may trigger a window.onload callback which can call open() on the same xhr.
    // This would create internalAbort reentrant call.
    // m_loader is set to null before being cancelled to exit early in any reentrant internalAbort() call.
    auto loader = WTFMove(m_loader);
    loader->cancel();

    // If window.onload callback calls open() and send() on the same xhr, m_loader is now set to a new value.
    // The function calling internalAbort() should abort to let the open() and send() calls continue properly.
    // We ask the function calling internalAbort() to exit by returning false.
    // Save this information to a local variable since we are going to drop protection.
    bool newLoadStarted = m_loader;

    dropProtection();

    return !newLoadStarted;
}

void XMLHttpRequest::clearResponse()
{
    m_response = ResourceResponse();
    clearResponseBuffers();
}

void XMLHttpRequest::clearResponseBuffers()
{
    m_responseBuilder.clear();
    m_responseEncoding = String();
    m_createdDocument = false;
    m_responseDocument = nullptr;
    m_binaryResponseBuilder = nullptr;
    m_responseCacheIsValid = false;
}

void XMLHttpRequest::clearRequest()
{
    m_requestHeaders.clear();
    m_requestEntityBody = nullptr;
}

void XMLHttpRequest::genericError()
{
    clearResponse();
    clearRequest();
    m_sendFlag = false;
    m_error = true;

    changeState(DONE);
}

void XMLHttpRequest::networkError()
{
    genericError();
    dispatchErrorEvents(eventNames().errorEvent);
    internalAbort();
}

void XMLHttpRequest::networkErrorTimerFired()
{
    networkError();
    dropProtection();
}
    
void XMLHttpRequest::abortError()
{
    genericError();
    dispatchErrorEvents(eventNames().abortEvent);
}

void XMLHttpRequest::dropProtection()
{
    // The XHR object itself holds on to the responseText, and
    // thus has extra cost even independent of any
    // responseText or responseXML objects it has handed
    // out. But it is protected from GC while loading, so this
    // can't be recouped until the load is done, so only
    // report the extra cost at that point.
    JSC::VM& vm = scriptExecutionContext()->vm();
    JSC::JSLockHolder lock(vm);
    // FIXME: Adopt reportExtraMemoryVisited, and switch to reportExtraMemoryAllocated.
    // https://bugs.webkit.org/show_bug.cgi?id=142595
    vm.heap.deprecatedReportExtraMemory(m_responseBuilder.length() * 2);

    unsetPendingActivity(this);
}

void XMLHttpRequest::overrideMimeType(const String& override, ExceptionCode& ec)
{
    if (m_state == LOADING || m_state == DONE) {
        ec = INVALID_STATE_ERR;
        return;
    }

    m_mimeTypeOverride = override;
}

void XMLHttpRequest::setRequestHeader(const String& name, const String& value, ExceptionCode& ec)
{
    if (m_state != OPENED || m_sendFlag) {
#if ENABLE(DASHBOARD_SUPPORT)
        if (usesDashboardBackwardCompatibilityMode())
            return;
#endif

        ec = INVALID_STATE_ERR;
        return;
    }

    String normalizedValue = stripLeadingAndTrailingHTTPSpaces(value);
    if (!isValidHTTPToken(name) || !isValidHTTPHeaderValue(normalizedValue)) {
        ec = SYNTAX_ERR;
        return;
    }

    // A privileged script (e.g. a Dashboard widget) can set any headers.
    if (!securityOrigin()->canLoadLocalResources() && !isAllowedHTTPHeader(name)) {
        logConsoleError(scriptExecutionContext(), "Refused to set unsafe header \"" + name + "\"");
        return;
    }

    m_requestHeaders.add(name, normalizedValue);
}

String XMLHttpRequest::getAllResponseHeaders() const
{
    if (m_state < HEADERS_RECEIVED || m_error)
        return emptyString();

    StringBuilder stringBuilder;

    for (const auto& header : m_response.httpHeaderFields()) {
        stringBuilder.append(header.key);
        stringBuilder.append(':');
        stringBuilder.append(' ');
        stringBuilder.append(header.value);
        stringBuilder.append('\r');
        stringBuilder.append('\n');
    }

    return stringBuilder.toString();
}

String XMLHttpRequest::getResponseHeader(const String& name) const
{
    if (m_state < HEADERS_RECEIVED || m_error)
        return String();

    return m_response.httpHeaderField(name);
}

String XMLHttpRequest::responseMIMEType() const
{
    String mimeType = extractMIMETypeFromMediaType(m_mimeTypeOverride);
    if (mimeType.isEmpty()) {
        if (m_response.isHTTP())
            mimeType = extractMIMETypeFromMediaType(m_response.httpHeaderField(HTTPHeaderName::ContentType));
        else
            mimeType = m_response.mimeType();
        if (mimeType.isEmpty())
            mimeType = ASCIILiteral("text/xml");
    }
    return mimeType;
}

bool XMLHttpRequest::responseIsXML() const
{
    return DOMImplementation::isXMLMIMEType(responseMIMEType());
}

int XMLHttpRequest::status() const
{
    if (m_state == UNSENT || m_state == OPENED || m_error)
        return 0;

    return m_response.httpStatusCode();
}

String XMLHttpRequest::statusText() const
{
    if (m_state == UNSENT || m_state == OPENED || m_error)
        return String();

    return m_response.httpStatusText();
}

void XMLHttpRequest::didFail(const ResourceError& error)
{
    // If we are already in an error state, for instance we called abort(), bail out early.
    if (m_error)
        return;

    if (error.isCancellation()) {
        m_exceptionCode = ABORT_ERR;
        abortError();
        return;
    }

    // In case of worker sync timeouts.
    if (error.isTimeout()) {
        didReachTimeout();
        return;
    }

    // Network failures are already reported to Web Inspector by ResourceLoader.
    if (error.domain() == errorDomainWebKitInternal) {
        String message = makeString("XMLHttpRequest cannot load ", error.failingURL().string(), ". ", error.localizedDescription());
        logConsoleError(scriptExecutionContext(), message);
    } else if (error.isAccessControl()) {
        String message = makeString("XMLHttpRequest cannot load ", error.failingURL().string(), " due to access control checks.");
        logConsoleError(scriptExecutionContext(), message);
    }

    // In case didFail is called synchronously on an asynchronous XHR call, let's dispatch network error asynchronously
    if (m_async && m_sendFlag && !m_loader) {
        m_sendFlag = false;
        setPendingActivity(this);
        m_timeoutTimer.stop();
        m_networkErrorTimer.startOneShot(0);
        return;
    }
    m_exceptionCode = NETWORK_ERR;
    networkError();
}

void XMLHttpRequest::didFinishLoading(unsigned long identifier, double)
{
    if (m_error)
        return;

    if (m_state < HEADERS_RECEIVED)
        changeState(HEADERS_RECEIVED);

    if (m_decoder)
        m_responseBuilder.append(m_decoder->flush());

    m_responseBuilder.shrinkToFit();

    Optional<String> decodedText;
    if (!m_binaryResponseBuilder)
        decodedText = m_responseBuilder.toStringPreserveCapacity();
    InspectorInstrumentation::didFinishXHRLoading(scriptExecutionContext(), this, identifier, decodedText, m_url, m_lastSendURL, m_lastSendLineNumber, m_lastSendColumnNumber);

    bool hadLoader = m_loader;
    m_loader = nullptr;

    m_sendFlag = false;
    changeState(DONE);
    m_responseEncoding = String();
    m_decoder = nullptr;

    m_timeoutTimer.stop();

    if (hadLoader)
        dropProtection();
}

void XMLHttpRequest::didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    if (!m_upload)
        return;

    if (m_uploadEventsAllowed)
        m_upload->dispatchThrottledProgressEvent(true, bytesSent, totalBytesToBeSent);
    if (bytesSent == totalBytesToBeSent && !m_uploadComplete) {
        m_uploadComplete = true;
        if (m_uploadEventsAllowed) {
            m_upload->dispatchProgressEvent(eventNames().loadEvent);
            m_upload->dispatchProgressEvent(eventNames().loadendEvent);
        }
    }
}

void XMLHttpRequest::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    InspectorInstrumentation::didReceiveXHRResponse(scriptExecutionContext(), identifier);

    m_response = response;
    if (!m_mimeTypeOverride.isEmpty())
        m_response.setHTTPHeaderField(HTTPHeaderName::ContentType, m_mimeTypeOverride);
}

static inline bool shouldDecodeResponse(XMLHttpRequest::ResponseType type)
{
    switch (type) {
    case XMLHttpRequest::ResponseType::EmptyString:
    case XMLHttpRequest::ResponseType::Document:
    case XMLHttpRequest::ResponseType::Json:
    case XMLHttpRequest::ResponseType::Text:
        return true;
    case XMLHttpRequest::ResponseType::Arraybuffer:
    case XMLHttpRequest::ResponseType::Blob:
        return false;
    }
    ASSERT_NOT_REACHED();
    return true;
}

void XMLHttpRequest::didReceiveData(const char* data, int len)
{
    if (m_error)
        return;

    if (m_state < HEADERS_RECEIVED)
        changeState(HEADERS_RECEIVED);

    // FIXME: Should we update "Content-Type" header field with m_mimeTypeOverride value in case it has changed since didReceiveResponse?
    if (!m_mimeTypeOverride.isEmpty())
        m_responseEncoding = extractCharsetFromMediaType(m_mimeTypeOverride);
    if (m_responseEncoding.isEmpty())
        m_responseEncoding = m_response.textEncodingName();

    bool useDecoder = shouldDecodeResponse(m_responseType);

    if (useDecoder && !m_decoder) {
        if (!m_responseEncoding.isEmpty())
            m_decoder = TextResourceDecoder::create("text/plain", m_responseEncoding);
        // allow TextResourceDecoder to look inside the m_response if it's XML or HTML
        else if (responseIsXML()) {
            m_decoder = TextResourceDecoder::create("application/xml");
            // Don't stop on encoding errors, unlike it is done for other kinds of XML resources. This matches the behavior of previous WebKit versions, Firefox and Opera.
            m_decoder->useLenientXMLDecoding();
        } else if (equalLettersIgnoringASCIICase(responseMIMEType(), "text/html"))
            m_decoder = TextResourceDecoder::create("text/html", "UTF-8");
        else
            m_decoder = TextResourceDecoder::create("text/plain", "UTF-8");
    }

    if (!len)
        return;

    if (len == -1)
        len = strlen(data);

    if (useDecoder)
        m_responseBuilder.append(m_decoder->decode(data, len));
    else {
        // Buffer binary data.
        if (!m_binaryResponseBuilder)
            m_binaryResponseBuilder = SharedBuffer::create();
        m_binaryResponseBuilder->append(data, len);
    }

    if (!m_error) {
        m_receivedLength += len;

        if (m_async) {
            long long expectedLength = m_response.expectedContentLength();
            bool lengthComputable = expectedLength > 0 && m_receivedLength <= expectedLength;
            unsigned long long total = lengthComputable ? expectedLength : 0;
            m_progressEventThrottle.dispatchThrottledProgressEvent(lengthComputable, m_receivedLength, total);
        }

        if (m_state != LOADING)
            changeState(LOADING);
        else
            // Firefox calls readyStateChanged every time it receives data, 4449442
            callReadyStateChangeListener();
    }
}

void XMLHttpRequest::dispatchErrorEvents(const AtomicString& type)
{
    if (!m_uploadComplete) {
        m_uploadComplete = true;
        if (m_upload && m_uploadEventsAllowed) {
            m_upload->dispatchProgressEvent(eventNames().progressEvent);
            m_upload->dispatchProgressEvent(type);
            m_upload->dispatchProgressEvent(eventNames().loadendEvent);
        }
    }
    m_progressEventThrottle.dispatchProgressEvent(eventNames().progressEvent);
    m_progressEventThrottle.dispatchProgressEvent(type);
    m_progressEventThrottle.dispatchProgressEvent(eventNames().loadendEvent);
}

void XMLHttpRequest::didReachTimeout()
{
    // internalAbort() calls dropProtection(), which may release the last reference.
    Ref<XMLHttpRequest> protectedThis(*this);
    if (!internalAbort())
        return;

    clearResponse();
    clearRequest();

    m_sendFlag = false;
    m_error = true;
    m_exceptionCode = TIMEOUT_ERR;

    if (!m_async) {
        m_state = DONE;
        m_exceptionCode = TIMEOUT_ERR;
        return;
    }

    changeState(DONE);

    dispatchErrorEvents(eventNames().timeoutEvent);
}

bool XMLHttpRequest::canSuspendForDocumentSuspension() const
{
    // If the load event has not fired yet, cancelling the load in suspend() may cause
    // the load event to be fired and arbitrary JS execution, which would be unsafe.
    // Therefore, we prevent suspending in this case.
    return document()->loadEventFinished();
}

const char* XMLHttpRequest::activeDOMObjectName() const
{
    return "XMLHttpRequest";
}

void XMLHttpRequest::suspend(ReasonForSuspension reason)
{
    m_progressEventThrottle.suspend();

    if (m_resumeTimer.isActive()) {
        m_resumeTimer.stop();
        m_dispatchErrorOnResuming = true;
    }

    if (reason == ActiveDOMObject::PageCache && m_loader) {
        // Going into PageCache, abort the request and dispatch a network error on resuming.
        genericError();
        m_dispatchErrorOnResuming = true;
        bool aborted = internalAbort();
        // It should not be possible to restart the load when aborting in suspend() because
        // we are not allowed to execute in JS in suspend().
        ASSERT_UNUSED(aborted, aborted);
    }
}

void XMLHttpRequest::resume()
{
    m_progressEventThrottle.resume();

    // We are not allowed to execute arbitrary JS in resume() so dispatch
    // the error event in a timer.
    if (m_dispatchErrorOnResuming && !m_resumeTimer.isActive())
        m_resumeTimer.startOneShot(0);
}

void XMLHttpRequest::resumeTimerFired()
{
    ASSERT(m_dispatchErrorOnResuming);
    m_dispatchErrorOnResuming = false;
    dispatchErrorEvents(eventNames().errorEvent);
}

void XMLHttpRequest::stop()
{
    internalAbort();
}

void XMLHttpRequest::contextDestroyed()
{
    ASSERT(!m_loader);
    ActiveDOMObject::contextDestroyed();
}

} // namespace WebCore
