/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#include "WebResourceLoader.h"

#include "DataReference.h"
#include "Logging.h"
#include "NetworkProcessConnection.h"
#include "NetworkResourceLoaderMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebErrors.h"
#include "WebProcess.h"
#include <WebCore/ApplicationCacheHost.h>
#include <WebCore/CertificateInfo.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceLoader.h>
#include <WebCore/SubresourceLoader.h>

using namespace WebCore;

#define RELEASE_LOG_IF_ALLOWED(...) RELEASE_LOG_IF(isAlwaysOnLoggingAllowed(), __VA_ARGS__)

namespace WebKit {

Ref<WebResourceLoader> WebResourceLoader::create(Ref<ResourceLoader>&& coreLoader)
{
    return adoptRef(*new WebResourceLoader(WTFMove(coreLoader)));
}

WebResourceLoader::WebResourceLoader(Ref<WebCore::ResourceLoader>&& coreLoader)
    : m_coreLoader(WTFMove(coreLoader))
{
}

WebResourceLoader::~WebResourceLoader()
{
}

IPC::Connection* WebResourceLoader::messageSenderConnection()
{
    return &WebProcess::singleton().networkConnection().connection();
}

uint64_t WebResourceLoader::messageSenderDestinationID()
{
    return m_coreLoader->identifier();
}

void WebResourceLoader::detachFromCoreLoader()
{
    m_coreLoader = nullptr;
}

void WebResourceLoader::willSendRequest(ResourceRequest&& proposedRequest, ResourceResponse&& redirectResponse)
{
    LOG(Network, "(WebProcess) WebResourceLoader::willSendRequest to '%s'", proposedRequest.url().string().latin1().data());
    RELEASE_LOG_IF_ALLOWED("WebResourceLoader::willSendRequest, WebResourceLoader = %p", this);

    RefPtr<WebResourceLoader> protectedThis(this);

    if (m_coreLoader->documentLoader()->applicationCacheHost()->maybeLoadFallbackForRedirect(m_coreLoader.get(), proposedRequest, redirectResponse))
        return;

    m_coreLoader->willSendRequest(WTFMove(proposedRequest), redirectResponse, [protectedThis](ResourceRequest&& request) {
        if (!protectedThis->m_coreLoader)
            return;

        protectedThis->send(Messages::NetworkResourceLoader::ContinueWillSendRequest(request));
    });
}

void WebResourceLoader::didSendData(uint64_t bytesSent, uint64_t totalBytesToBeSent)
{
    m_coreLoader->didSendData(bytesSent, totalBytesToBeSent);
}

void WebResourceLoader::didReceiveResponse(const ResourceResponse& response, bool needsContinueDidReceiveResponseMessage)
{
    LOG(Network, "(WebProcess) WebResourceLoader::didReceiveResponse for '%s'. Status %d.", m_coreLoader->url().string().latin1().data(), response.httpStatusCode());
    RELEASE_LOG_IF_ALLOWED("WebResourceLoader::didReceiveResponse, WebResourceLoader = %p, status = %d.", this, response.httpStatusCode());

    Ref<WebResourceLoader> protect(*this);

    if (m_coreLoader->documentLoader()->applicationCacheHost()->maybeLoadFallbackForResponse(m_coreLoader.get(), response))
        return;

    bool shoudCallCoreLoaderDidReceiveResponse = true;
#if USE(QUICK_LOOK)
    // Refrain from calling didReceiveResponse if QuickLook will convert this response, since the MIME type of the
    // converted resource isn't yet known. WebResourceLoaderQuickLookDelegate will later call didReceiveResponse upon
    // receiving the converted data.
    bool isMainLoad = m_coreLoader->documentLoader()->mainResourceLoader() == m_coreLoader;
    if (isMainLoad && QuickLookHandle::shouldCreateForMIMEType(response.mimeType())) {
        m_coreLoader->documentLoader()->setQuickLookHandle(QuickLookHandle::create(*m_coreLoader, response));
        shoudCallCoreLoaderDidReceiveResponse = false;
    }
#endif
    if (shoudCallCoreLoaderDidReceiveResponse)
        m_coreLoader->didReceiveResponse(response);

    // If m_coreLoader becomes null as a result of the didReceiveResponse callback, we can't use the send function(). 
    if (!m_coreLoader)
        return;

    if (needsContinueDidReceiveResponseMessage)
        send(Messages::NetworkResourceLoader::ContinueDidReceiveResponse());
}

void WebResourceLoader::didReceiveData(const IPC::DataReference& data, int64_t encodedDataLength)
{
    LOG(Network, "(WebProcess) WebResourceLoader::didReceiveData of size %lu for '%s'", data.size(), m_coreLoader->url().string().latin1().data());
    RELEASE_LOG_IF_ALLOWED("WebResourceLoader::didReceiveData, WebResourceLoader = %p, size = %lu", this, data.size());

#if USE(QUICK_LOOK)
    if (QuickLookHandle* quickLookHandle = m_coreLoader->documentLoader()->quickLookHandle()) {
        if (quickLookHandle->didReceiveData(adoptCF(CFDataCreate(kCFAllocatorDefault, data.data(), data.size())).get()))
            return;
    }
#endif
    m_coreLoader->didReceiveData(reinterpret_cast<const char*>(data.data()), data.size(), encodedDataLength, DataPayloadBytes);
}

void WebResourceLoader::didFinishResourceLoad(double finishTime)
{
    LOG(Network, "(WebProcess) WebResourceLoader::didFinishResourceLoad for '%s'", m_coreLoader->url().string().latin1().data());
    RELEASE_LOG_IF_ALLOWED("WebResourceLoader::didFinishResourceLoad, WebResourceLoader = %p", this);

#if USE(QUICK_LOOK)
    if (QuickLookHandle* quickLookHandle = m_coreLoader->documentLoader()->quickLookHandle()) {
        if (quickLookHandle->didFinishLoading())
            return;
    }
#endif
    m_coreLoader->didFinishLoading(finishTime);
}

void WebResourceLoader::didFailResourceLoad(const ResourceError& error)
{
    LOG(Network, "(WebProcess) WebResourceLoader::didFailResourceLoad for '%s'", m_coreLoader->url().string().latin1().data());
    RELEASE_LOG_IF_ALLOWED("WebResourceLoader::didFailResourceLoad, WebResourceLoader = %p", this);

#if USE(QUICK_LOOK)
    if (QuickLookHandle* quickLookHandle = m_coreLoader->documentLoader()->quickLookHandle())
        quickLookHandle->didFail();
#endif
    if (m_coreLoader->documentLoader()->applicationCacheHost()->maybeLoadFallbackForError(m_coreLoader.get(), error))
        return;
    m_coreLoader->didFail(error);
}

#if ENABLE(SHAREABLE_RESOURCE)
void WebResourceLoader::didReceiveResource(const ShareableResource::Handle& handle, double finishTime)
{
    LOG(Network, "(WebProcess) WebResourceLoader::didReceiveResource for '%s'", m_coreLoader->url().string().latin1().data());
    RELEASE_LOG_IF_ALLOWED("WebResourceLoader::didReceiveResource, WebResourceLoader = %p", this);

    RefPtr<SharedBuffer> buffer = handle.tryWrapInSharedBuffer();

#if USE(QUICK_LOOK)
    if (QuickLookHandle* quickLookHandle = m_coreLoader->documentLoader()->quickLookHandle()) {
        if (buffer) {
            if (quickLookHandle->didReceiveData(buffer->existingCFData())) {
                quickLookHandle->didFinishLoading();
                return;
            }
        } else
            quickLookHandle->didFail();
    }
#endif

    if (!buffer) {
        LOG_ERROR("Unable to create buffer from ShareableResource sent from the network process.");
        m_coreLoader->didFail(internalError(m_coreLoader->request().url()));
        return;
    }

    Ref<WebResourceLoader> protect(*this);

    // Only send data to the didReceiveData callback if it exists.
    if (unsigned bufferSize = buffer->size())
        m_coreLoader->didReceiveBuffer(buffer.releaseNonNull(), bufferSize, DataPayloadWholeResource);

    if (!m_coreLoader)
        return;

    m_coreLoader->didFinishLoading(finishTime);
}
#endif

bool WebResourceLoader::isAlwaysOnLoggingAllowed() const
{
    return resourceLoader() && resourceLoader()->isAlwaysOnLoggingAllowed();
}

} // namespace WebKit
