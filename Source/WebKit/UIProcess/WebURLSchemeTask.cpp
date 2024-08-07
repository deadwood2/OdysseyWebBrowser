/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
#include "WebURLSchemeTask.h"

#include "DataReference.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebURLSchemeHandler.h"

using namespace WebCore;

namespace WebKit {

Ref<WebURLSchemeTask> WebURLSchemeTask::create(WebURLSchemeHandler& handler, WebPageProxy& page, uint64_t resourceIdentifier, ResourceRequest&& request)
{
    return adoptRef(*new WebURLSchemeTask(handler, page, resourceIdentifier, WTFMove(request)));
}

WebURLSchemeTask::WebURLSchemeTask(WebURLSchemeHandler& handler, WebPageProxy& page, uint64_t resourceIdentifier, ResourceRequest&& request)
    : m_urlSchemeHandler(handler)
    , m_page(&page)
    , m_identifier(resourceIdentifier)
    , m_pageIdentifier(page.pageID())
    , m_request(WTFMove(request))
{
}

auto WebURLSchemeTask::didPerformRedirection(WebCore::ResourceResponse&& response, WebCore::ResourceRequest&& request) -> ExceptionType
{
    if (m_stopped)
        return ExceptionType::TaskAlreadyStopped;
    
    if (m_completed)
        return ExceptionType::CompleteAlreadyCalled;
    
    if (m_dataSent)
        return ExceptionType::DataAlreadySent;
    
    if (m_responseSent)
        return ExceptionType::RedirectAfterResponse;
    
    m_request = request;
    m_page->send(Messages::WebPage::URLSchemeTaskDidPerformRedirection(m_urlSchemeHandler->identifier(), m_identifier, response, request));

    return ExceptionType::None;
}

auto WebURLSchemeTask::didReceiveResponse(const ResourceResponse& response) -> ExceptionType
{
    if (m_stopped)
        return ExceptionType::TaskAlreadyStopped;

    if (m_completed)
        return ExceptionType::CompleteAlreadyCalled;

    if (m_dataSent)
        return ExceptionType::DataAlreadySent;

    m_responseSent = true;

    response.includeCertificateInfo();
    m_page->send(Messages::WebPage::URLSchemeTaskDidReceiveResponse(m_urlSchemeHandler->identifier(), m_identifier, response));
    return ExceptionType::None;
}

auto WebURLSchemeTask::didReceiveData(Ref<SharedBuffer> buffer) -> ExceptionType
{
    if (m_stopped)
        return ExceptionType::TaskAlreadyStopped;

    if (m_completed)
        return ExceptionType::CompleteAlreadyCalled;

    if (!m_responseSent)
        return ExceptionType::NoResponseSent;

    m_dataSent = true;
    m_page->send(Messages::WebPage::URLSchemeTaskDidReceiveData(m_urlSchemeHandler->identifier(), m_identifier, IPC::SharedBufferDataReference(buffer.ptr())));
    return ExceptionType::None;
}

auto WebURLSchemeTask::didComplete(const ResourceError& error) -> ExceptionType
{
    if (m_stopped)
        return ExceptionType::TaskAlreadyStopped;

    if (m_completed)
        return ExceptionType::CompleteAlreadyCalled;

    if (!m_responseSent && error.isNull())
        return ExceptionType::NoResponseSent;

    m_completed = true;
    m_page->send(Messages::WebPage::URLSchemeTaskDidComplete(m_urlSchemeHandler->identifier(), m_identifier, error));
    m_urlSchemeHandler->taskCompleted(*this);

    return ExceptionType::None;
}

void WebURLSchemeTask::pageDestroyed()
{
    ASSERT(m_page);
    m_page = nullptr;
    m_stopped = true;
}

void WebURLSchemeTask::stop()
{
    ASSERT(!m_stopped);
    m_stopped = true;
}

} // namespace WebKit
