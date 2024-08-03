/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "InspectorNetworkAgent.h"

#include "CachedRawResource.h"
#include "CachedResource.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "DocumentThreadableLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTTPHeaderMap.h"
#include "HTTPHeaderNames.h"
#include "IconController.h"
#include "InspectorPageAgent.h"
#include "InspectorTimelineAgent.h"
#include "InstrumentingAgents.h"
#include "JSMainThreadExecState.h"
#include "MemoryCache.h"
#include "NetworkResourcesData.h"
#include "Page.h"
#include "ProgressTracker.h"
#include "ResourceError.h"
#include "ResourceLoader.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptableDocumentParser.h"
#include "SubresourceLoader.h"
#include "ThreadableLoaderClient.h"
#include "URL.h"
#include "WebSocketFrame.h"
#include <inspector/ContentSearchUtilities.h>
#include <inspector/IdentifiersFactory.h>
#include <inspector/InspectorFrontendRouter.h>
#include <inspector/InspectorValues.h>
#include <inspector/ScriptCallStack.h>
#include <inspector/ScriptCallStackFactory.h>
#include <wtf/RefPtr.h>
#include <wtf/Stopwatch.h>
#include <wtf/text/StringBuilder.h>

using namespace Inspector;

typedef Inspector::NetworkBackendDispatcherHandler::LoadResourceCallback LoadResourceCallback;

namespace WebCore {

namespace {

class InspectorThreadableLoaderClient final : public ThreadableLoaderClient {
    WTF_MAKE_NONCOPYABLE(InspectorThreadableLoaderClient);
public:
    InspectorThreadableLoaderClient(RefPtr<LoadResourceCallback>&& callback)
        : m_callback(WTFMove(callback)) { }

    virtual ~InspectorThreadableLoaderClient() { }

    void didReceiveResponse(unsigned long, const ResourceResponse& response) override
    {
        m_mimeType = response.mimeType();
        m_statusCode = response.httpStatusCode();

        // FIXME: This assumes text only responses. We should support non-text responses as well.
        TextEncoding textEncoding(response.textEncodingName());
        bool useDetector = false;
        if (!textEncoding.isValid()) {
            textEncoding = UTF8Encoding();
            useDetector = true;
        }

        m_decoder = TextResourceDecoder::create(ASCIILiteral("text/plain"), textEncoding, useDetector);
    }

    void didReceiveData(const char* data, int dataLength) override
    {
        if (!dataLength)
            return;

        if (dataLength == -1)
            dataLength = strlen(data);

        m_responseText.append(m_decoder->decode(data, dataLength));
    }

    void didFinishLoading(unsigned long, double) override
    {
        if (m_decoder)
            m_responseText.append(m_decoder->flush());

        m_callback->sendSuccess(m_responseText.toString(), m_mimeType, m_statusCode);
        dispose();
    }

    void didFail(const ResourceError& error) override
    {
        m_callback->sendFailure(error.isAccessControl() ? ASCIILiteral("Loading resource for inspector failed access control check") : ASCIILiteral("Loading resource for inspector failed"));
        dispose();
    }

    void didFailLoaderCreation()
    {
        m_callback->sendFailure(ASCIILiteral("Could not create a loader"));
        dispose();
    }

    void setLoader(RefPtr<ThreadableLoader>&& loader)
    {
        m_loader = WTFMove(loader);
    }

private:
    void dispose()
    {
        m_loader = nullptr;
        delete this;
    }

    RefPtr<LoadResourceCallback> m_callback;
    RefPtr<ThreadableLoader> m_loader;
    RefPtr<TextResourceDecoder> m_decoder;
    String m_mimeType;
    StringBuilder m_responseText;
    int m_statusCode;
};

} // namespace

InspectorNetworkAgent::InspectorNetworkAgent(WebAgentContext& context, InspectorPageAgent* pageAgent)
    : InspectorAgentBase(ASCIILiteral("Network"), context)
    , m_frontendDispatcher(std::make_unique<Inspector::NetworkFrontendDispatcher>(context.frontendRouter))
    , m_backendDispatcher(Inspector::NetworkBackendDispatcher::create(context.backendDispatcher, this))
    , m_pageAgent(pageAgent)
    , m_resourcesData(std::make_unique<NetworkResourcesData>())
{
}

void InspectorNetworkAgent::didCreateFrontendAndBackend(Inspector::FrontendRouter*, Inspector::BackendDispatcher*)
{
}

void InspectorNetworkAgent::willDestroyFrontendAndBackend(Inspector::DisconnectReason)
{
    ErrorString unused;
    disable(unused);
}

static Ref<InspectorObject> buildObjectForHeaders(const HTTPHeaderMap& headers)
{
    Ref<InspectorObject> headersObject = InspectorObject::create();
    
    for (const auto& header : headers)
        headersObject->setString(header.key, header.value);
    return headersObject;
}

Ref<Inspector::Protocol::Network::ResourceTiming> InspectorNetworkAgent::buildObjectForTiming(const NetworkLoadTiming& timing, ResourceLoader& resourceLoader)
{
    double monotonicTime = resourceLoader.loadTiming().startTime();
    double startTimeInInspector = m_environment.executionStopwatch()->elapsedTimeSinceMonotonicTime(monotonicTime);

    return Inspector::Protocol::Network::ResourceTiming::create()
        .setStartTime(startTimeInInspector)
        .setDomainLookupStart(timing.domainLookupStart)
        .setDomainLookupEnd(timing.domainLookupEnd)
        .setConnectStart(timing.connectStart)
        .setConnectEnd(timing.connectEnd)
        .setSecureConnectionStart(timing.secureConnectionStart)
        .setRequestStart(timing.requestStart)
        .setResponseStart(timing.responseStart)
        .release();
}

static Ref<Inspector::Protocol::Network::Request> buildObjectForResourceRequest(const ResourceRequest& request)
{
    auto requestObject = Inspector::Protocol::Network::Request::create()
        .setUrl(request.url().string())
        .setMethod(request.httpMethod())
        .setHeaders(buildObjectForHeaders(request.httpHeaderFields()))
        .release();
    if (request.httpBody() && !request.httpBody()->isEmpty()) {
        Vector<char> bytes;
        request.httpBody()->flatten(bytes);
        requestObject->setPostData(String::fromUTF8WithLatin1Fallback(bytes.data(), bytes.size()));
    }
    return requestObject;
}

RefPtr<Inspector::Protocol::Network::Response> InspectorNetworkAgent::buildObjectForResourceResponse(const ResourceResponse& response, ResourceLoader* resourceLoader)
{
    if (response.isNull())
        return nullptr;

    double status = response.httpStatusCode();
    Ref<InspectorObject> headers = buildObjectForHeaders(response.httpHeaderFields());

    auto responseObject = Inspector::Protocol::Network::Response::create()
        .setUrl(response.url().string())
        .setStatus(status)
        .setStatusText(response.httpStatusText())
        .setHeaders(WTFMove(headers))
        .setMimeType(response.mimeType())
        .release();

    responseObject->setFromDiskCache(response.source() == ResourceResponse::Source::DiskCache || response.source() == ResourceResponse::Source::DiskCacheAfterValidation);
    if (resourceLoader)
        responseObject->setTiming(buildObjectForTiming(response.networkLoadTiming(), *resourceLoader));

    return WTFMove(responseObject);
}

Ref<Inspector::Protocol::Network::CachedResource> InspectorNetworkAgent::buildObjectForCachedResource(CachedResource* cachedResource)
{
    auto resourceObject = Inspector::Protocol::Network::CachedResource::create()
        .setUrl(cachedResource->url())
        .setType(InspectorPageAgent::cachedResourceTypeJson(*cachedResource))
        .setBodySize(cachedResource->encodedSize())
        .release();

    auto resourceResponse = buildObjectForResourceResponse(cachedResource->response(), cachedResource->loader());
    resourceObject->setResponse(WTFMove(resourceResponse));

    String sourceMappingURL = InspectorPageAgent::sourceMapURLForResource(cachedResource);
    if (!sourceMappingURL.isEmpty())
        resourceObject->setSourceMapURL(sourceMappingURL);

    return resourceObject;
}

InspectorNetworkAgent::~InspectorNetworkAgent()
{
    if (m_enabled) {
        ErrorString unused;
        disable(unused);
    }
    ASSERT(!m_instrumentingAgents.inspectorNetworkAgent());
}

double InspectorNetworkAgent::timestamp()
{
    return m_environment.executionStopwatch()->elapsedTime();
}

void InspectorNetworkAgent::willSendRequest(unsigned long identifier, DocumentLoader& loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (request.hiddenFromInspector()) {
        m_hiddenRequestIdentifiers.add(identifier);
        return;
    }

    String requestId = IdentifiersFactory::requestId(identifier);
    m_resourcesData->resourceCreated(requestId, m_pageAgent->loaderId(&loader));

    CachedResource* cachedResource = InspectorPageAgent::cachedResource(loader.frame(), request.url());
    InspectorPageAgent::ResourceType type = cachedResource ? InspectorPageAgent::cachedResourceType(*cachedResource) : m_resourcesData->resourceType(requestId);
    if (type == InspectorPageAgent::OtherResource) {
        if (m_loadingXHRSynchronously)
            type = InspectorPageAgent::XHRResource;
        else if (equalIgnoringFragmentIdentifier(request.url(), loader.frameLoader()->icon().url()))
            type = InspectorPageAgent::ImageResource;
        else if (equalIgnoringFragmentIdentifier(request.url(), loader.url()) && !loader.isCommitted())
            type = InspectorPageAgent::DocumentResource;
    }

    m_resourcesData->setResourceType(requestId, type);

    for (auto& entry : m_extraRequestHeaders)
        request.setHTTPHeaderField(entry.key, entry.value);

    request.setReportLoadTiming(true);
    request.setReportRawHeaders(true);

    if (m_cacheDisabled) {
        request.setHTTPHeaderField(HTTPHeaderName::Pragma, "no-cache");
        request.setCachePolicy(ReloadIgnoringCacheData);
        request.setHTTPHeaderField(HTTPHeaderName::CacheControl, "no-cache");
    }

    Inspector::Protocol::Page::ResourceType resourceType = InspectorPageAgent::resourceTypeJson(type);

    RefPtr<Inspector::Protocol::Network::Initiator> initiatorObject = buildInitiatorObject(loader.frame() ? loader.frame()->document() : nullptr);
    m_frontendDispatcher->requestWillBeSent(requestId, m_pageAgent->frameId(loader.frame()), m_pageAgent->loaderId(&loader), loader.url().string(), buildObjectForResourceRequest(request), timestamp(), initiatorObject, buildObjectForResourceResponse(redirectResponse, nullptr), type != InspectorPageAgent::OtherResource ? &resourceType : nullptr);
}

void InspectorNetworkAgent::markResourceAsCached(unsigned long identifier)
{
    if (m_hiddenRequestIdentifiers.contains(identifier))
        return;

    m_frontendDispatcher->requestServedFromCache(IdentifiersFactory::requestId(identifier));
}

void InspectorNetworkAgent::didReceiveResponse(unsigned long identifier, DocumentLoader& loader, const ResourceResponse& response, ResourceLoader* resourceLoader)
{
    if (m_hiddenRequestIdentifiers.contains(identifier))
        return;

    String requestId = IdentifiersFactory::requestId(identifier);
    RefPtr<Inspector::Protocol::Network::Response> resourceResponse = buildObjectForResourceResponse(response, resourceLoader);

    bool isNotModified = response.httpStatusCode() == 304;

    CachedResource* cachedResource = nullptr;
    if (resourceLoader && resourceLoader->isSubresourceLoader() && !isNotModified)
        cachedResource = static_cast<SubresourceLoader*>(resourceLoader)->cachedResource();
    if (!cachedResource)
        cachedResource = InspectorPageAgent::cachedResource(loader.frame(), response.url());

    if (cachedResource) {
        // Use mime type from cached resource in case the one in response is empty.
        if (resourceResponse && response.mimeType().isEmpty())
            resourceResponse->setString(Inspector::Protocol::Network::Response::MimeType, cachedResource->response().mimeType());
        m_resourcesData->addCachedResource(requestId, cachedResource);
    }

    InspectorPageAgent::ResourceType type = m_resourcesData->resourceType(requestId);
    InspectorPageAgent::ResourceType newType = cachedResource ? InspectorPageAgent::cachedResourceType(*cachedResource) : type;

    // FIXME: XHRResource is returned for CachedResource::RawResource, it should be OtherResource unless it truly is an XHR.
    // RawResource is used for loading worker scripts, and those should stay as ScriptResource and not change to XHRResource.
    if (type != newType && newType != InspectorPageAgent::XHRResource && newType != InspectorPageAgent::OtherResource)
        type = newType;

    m_resourcesData->responseReceived(requestId, m_pageAgent->frameId(loader.frame()), response);
    m_resourcesData->setResourceType(requestId, type);

    m_frontendDispatcher->responseReceived(requestId, m_pageAgent->frameId(loader.frame()), m_pageAgent->loaderId(&loader), timestamp(), InspectorPageAgent::resourceTypeJson(type), resourceResponse);

    // If we revalidated the resource and got Not modified, send content length following didReceiveResponse
    // as there will be no calls to didReceiveData from the network stack.
    if (isNotModified && cachedResource && cachedResource->encodedSize())
        didReceiveData(identifier, nullptr, cachedResource->encodedSize(), 0);
}

static bool isErrorStatusCode(int statusCode)
{
    return statusCode >= 400;
}

void InspectorNetworkAgent::didReceiveData(unsigned long identifier, const char* data, int dataLength, int encodedDataLength)
{
    if (m_hiddenRequestIdentifiers.contains(identifier))
        return;

    String requestId = IdentifiersFactory::requestId(identifier);

    if (data) {
        NetworkResourcesData::ResourceData const* resourceData = m_resourcesData->data(requestId);
        if (resourceData && !m_loadingXHRSynchronously && (!resourceData->cachedResource() || resourceData->cachedResource()->dataBufferingPolicy() == DoNotBufferData || isErrorStatusCode(resourceData->httpStatusCode())))
            m_resourcesData->maybeAddResourceData(requestId, data, dataLength);
    }

    m_frontendDispatcher->dataReceived(requestId, timestamp(), dataLength, encodedDataLength);
}

void InspectorNetworkAgent::didFinishLoading(unsigned long identifier, DocumentLoader& loader, double finishTime)
{
    if (m_hiddenRequestIdentifiers.remove(identifier))
        return;

    String requestId = IdentifiersFactory::requestId(identifier);
    if (m_resourcesData->resourceType(requestId) == InspectorPageAgent::DocumentResource)
        m_resourcesData->addResourceSharedBuffer(requestId, loader.frameLoader()->documentLoader()->mainResourceData(), loader.frame()->document()->encoding());

    m_resourcesData->maybeDecodeDataToContent(requestId);

    double elapsedFinishTime = finishTime ? m_environment.executionStopwatch()->elapsedTimeSinceMonotonicTime(finishTime) : timestamp();

    String sourceMappingURL;
    NetworkResourcesData::ResourceData const* resourceData = m_resourcesData->data(requestId);
    if (resourceData && resourceData->cachedResource())
        sourceMappingURL = InspectorPageAgent::sourceMapURLForResource(resourceData->cachedResource());

    m_frontendDispatcher->loadingFinished(requestId, elapsedFinishTime, !sourceMappingURL.isEmpty() ? &sourceMappingURL : nullptr);
}

void InspectorNetworkAgent::didFailLoading(unsigned long identifier, DocumentLoader& loader, const ResourceError& error)
{
    if (m_hiddenRequestIdentifiers.remove(identifier))
        return;

    String requestId = IdentifiersFactory::requestId(identifier);

    if (m_resourcesData->resourceType(requestId) == InspectorPageAgent::DocumentResource) {
        Frame* frame = loader.frame();
        if (frame && frame->loader().documentLoader() && frame->document()) {
            m_resourcesData->addResourceSharedBuffer(requestId,
                frame->loader().documentLoader()->mainResourceData(),
                frame->document()->encoding());
        }
    }

    bool canceled = error.isCancellation();
    m_frontendDispatcher->loadingFailed(requestId, timestamp(), error.localizedDescription(), canceled ? &canceled : nullptr);
}

void InspectorNetworkAgent::didLoadResourceFromMemoryCache(DocumentLoader& loader, CachedResource& resource)
{
    String loaderId = m_pageAgent->loaderId(&loader);
    String frameId = m_pageAgent->frameId(loader.frame());
    unsigned long identifier = loader.frame()->page()->progress().createUniqueIdentifier();
    String requestId = IdentifiersFactory::requestId(identifier);
    m_resourcesData->resourceCreated(requestId, loaderId);
    m_resourcesData->addCachedResource(requestId, &resource);

    RefPtr<Inspector::Protocol::Network::Initiator> initiatorObject = buildInitiatorObject(loader.frame() ? loader.frame()->document() : nullptr);

    m_frontendDispatcher->requestServedFromMemoryCache(requestId, frameId, loaderId, loader.url().string(), timestamp(), initiatorObject, buildObjectForCachedResource(&resource));
}

void InspectorNetworkAgent::setInitialScriptContent(unsigned long identifier, const String& sourceString)
{
    m_resourcesData->setResourceContent(IdentifiersFactory::requestId(identifier), sourceString);
}

void InspectorNetworkAgent::didReceiveScriptResponse(unsigned long identifier)
{
    m_resourcesData->setResourceType(IdentifiersFactory::requestId(identifier), InspectorPageAgent::ScriptResource);
}

void InspectorNetworkAgent::didFinishXHRLoading(ThreadableLoaderClient*, unsigned long identifier, const String& decodedText)
{
    m_resourcesData->setResourceContent(IdentifiersFactory::requestId(identifier), decodedText);
}

void InspectorNetworkAgent::didReceiveXHRResponse(unsigned long identifier)
{
    m_resourcesData->setResourceType(IdentifiersFactory::requestId(identifier), InspectorPageAgent::XHRResource);
}

void InspectorNetworkAgent::willLoadXHRSynchronously()
{
    m_loadingXHRSynchronously = true;
}

void InspectorNetworkAgent::didLoadXHRSynchronously()
{
    m_loadingXHRSynchronously = false;
}

void InspectorNetworkAgent::willDestroyCachedResource(CachedResource& cachedResource)
{
    Vector<String> requestIds = m_resourcesData->removeCachedResource(&cachedResource);
    if (!requestIds.size())
        return;

    String content;
    bool base64Encoded;
    if (!InspectorPageAgent::cachedResourceContent(&cachedResource, &content, &base64Encoded))
        return;
    for (auto& id : requestIds)
        m_resourcesData->setResourceContent(id, content, base64Encoded);
}

void InspectorNetworkAgent::willRecalculateStyle()
{
    m_isRecalculatingStyle = true;
}

void InspectorNetworkAgent::didRecalculateStyle()
{
    m_isRecalculatingStyle = false;
    m_styleRecalculationInitiator = nullptr;
}

void InspectorNetworkAgent::didScheduleStyleRecalculation(Document& document)
{
    if (!m_styleRecalculationInitiator)
        m_styleRecalculationInitiator = buildInitiatorObject(&document);
}

RefPtr<Inspector::Protocol::Network::Initiator> InspectorNetworkAgent::buildInitiatorObject(Document* document)
{
    RefPtr<ScriptCallStack> stackTrace = createScriptCallStack(JSMainThreadExecState::currentState(), ScriptCallStack::maxCallStackSizeToCapture);
    if (stackTrace && stackTrace->size() > 0) {
        auto initiatorObject = Inspector::Protocol::Network::Initiator::create()
            .setType(Inspector::Protocol::Network::Initiator::Type::Script)
            .release();
        initiatorObject->setStackTrace(stackTrace->buildInspectorArray());
        return WTFMove(initiatorObject);
    }

    if (document && document->scriptableDocumentParser()) {
        auto initiatorObject = Inspector::Protocol::Network::Initiator::create()
            .setType(Inspector::Protocol::Network::Initiator::Type::Parser)
            .release();
        initiatorObject->setUrl(document->url().string());
        initiatorObject->setLineNumber(document->scriptableDocumentParser()->textPosition().m_line.oneBasedInt());
        return WTFMove(initiatorObject);
    }

    if (m_isRecalculatingStyle && m_styleRecalculationInitiator)
        return m_styleRecalculationInitiator;

    return Inspector::Protocol::Network::Initiator::create()
        .setType(Inspector::Protocol::Network::Initiator::Type::Other)
        .release();
}

#if ENABLE(WEB_SOCKETS)

void InspectorNetworkAgent::didCreateWebSocket(unsigned long identifier, const URL& requestURL)
{
    m_frontendDispatcher->webSocketCreated(IdentifiersFactory::requestId(identifier), requestURL.string());
}

void InspectorNetworkAgent::willSendWebSocketHandshakeRequest(unsigned long identifier, const ResourceRequest& request)
{
    auto requestObject = Inspector::Protocol::Network::WebSocketRequest::create()
        .setHeaders(buildObjectForHeaders(request.httpHeaderFields()))
        .release();
    m_frontendDispatcher->webSocketWillSendHandshakeRequest(IdentifiersFactory::requestId(identifier), timestamp(), WTFMove(requestObject));
}

void InspectorNetworkAgent::didReceiveWebSocketHandshakeResponse(unsigned long identifier, const ResourceResponse& response)
{
    auto responseObject = Inspector::Protocol::Network::WebSocketResponse::create()
        .setStatus(response.httpStatusCode())
        .setStatusText(response.httpStatusText())
        .setHeaders(buildObjectForHeaders(response.httpHeaderFields()))
        .release();
    m_frontendDispatcher->webSocketHandshakeResponseReceived(IdentifiersFactory::requestId(identifier), timestamp(), WTFMove(responseObject));
}

void InspectorNetworkAgent::didCloseWebSocket(unsigned long identifier)
{
    m_frontendDispatcher->webSocketClosed(IdentifiersFactory::requestId(identifier), timestamp());
}

void InspectorNetworkAgent::didReceiveWebSocketFrame(unsigned long identifier, const WebSocketFrame& frame)
{
    auto frameObject = Inspector::Protocol::Network::WebSocketFrame::create()
        .setOpcode(frame.opCode)
        .setMask(frame.masked)
        .setPayloadData(String(frame.payload, frame.payloadLength))
        .release();
    m_frontendDispatcher->webSocketFrameReceived(IdentifiersFactory::requestId(identifier), timestamp(), WTFMove(frameObject));
}

void InspectorNetworkAgent::didSendWebSocketFrame(unsigned long identifier, const WebSocketFrame& frame)
{
    auto frameObject = Inspector::Protocol::Network::WebSocketFrame::create()
        .setOpcode(frame.opCode)
        .setMask(frame.masked)
        .setPayloadData(String(frame.payload, frame.payloadLength))
        .release();
    m_frontendDispatcher->webSocketFrameSent(IdentifiersFactory::requestId(identifier), timestamp(), WTFMove(frameObject));
}

void InspectorNetworkAgent::didReceiveWebSocketFrameError(unsigned long identifier, const String& errorMessage)
{
    m_frontendDispatcher->webSocketFrameError(IdentifiersFactory::requestId(identifier), timestamp(), errorMessage);
}

#endif // ENABLE(WEB_SOCKETS)

void InspectorNetworkAgent::enable(ErrorString&)
{
    enable();
}

void InspectorNetworkAgent::enable()
{
    m_enabled = true;
    m_instrumentingAgents.setInspectorNetworkAgent(this);
}

void InspectorNetworkAgent::disable(ErrorString&)
{
    m_enabled = false;
    m_instrumentingAgents.setInspectorNetworkAgent(nullptr);
    m_resourcesData->clear();
    m_extraRequestHeaders.clear();
}

void InspectorNetworkAgent::setExtraHTTPHeaders(ErrorString&, const InspectorObject& headers)
{
    for (auto& entry : headers) {
        String stringValue;
        if (entry.value->asString(stringValue))
            m_extraRequestHeaders.set(entry.key, stringValue);
    }
}

void InspectorNetworkAgent::getResponseBody(ErrorString& errorString, const String& requestId, String* content, bool* base64Encoded)
{
    NetworkResourcesData::ResourceData const* resourceData = m_resourcesData->data(requestId);
    if (!resourceData) {
        errorString = ASCIILiteral("No resource with given identifier found");
        return;
    }

    if (resourceData->hasContent()) {
        *base64Encoded = resourceData->base64Encoded();
        *content = resourceData->content();
        return;
    }

    if (resourceData->isContentEvicted()) {
        errorString = ASCIILiteral("Request content was evicted from inspector cache");
        return;
    }

    if (resourceData->buffer() && !resourceData->textEncodingName().isNull()) {
        *base64Encoded = false;
        if (InspectorPageAgent::sharedBufferContent(resourceData->buffer(), resourceData->textEncodingName(), *base64Encoded, content))
            return;
    }

    if (resourceData->cachedResource()) {
        if (InspectorPageAgent::cachedResourceContent(resourceData->cachedResource(), content, base64Encoded))
            return;
    }

    errorString = ASCIILiteral("No data found for resource with given identifier");
}

void InspectorNetworkAgent::setCacheDisabled(ErrorString&, bool cacheDisabled)
{
    m_cacheDisabled = cacheDisabled;
    if (cacheDisabled)
        MemoryCache::singleton().evictResources();
}

void InspectorNetworkAgent::loadResource(ErrorString& errorString, const String& frameId, const String& urlString, Ref<LoadResourceCallback>&& callback)
{
    Frame* frame = m_pageAgent->assertFrame(errorString, frameId);
    if (!frame)
        return;

    Document* document = frame->document();
    if (!document) {
        errorString = ASCIILiteral("No Document instance for the specified frame");
        return;
    }

    URL url = document->completeURL(urlString);
    ResourceRequest request(url);
    request.setHTTPMethod(ASCIILiteral("GET"));
    request.setHiddenFromInspector(true);

    ThreadableLoaderOptions options;
    options.sendLoadCallbacks = SendCallbacks; // So we remove this from m_hiddenRequestIdentifiers on completion.
    options.defersLoadingPolicy = DefersLoadingPolicy::DisallowDefersLoading; // So the request is never deferred.
    options.mode = FetchOptions::Mode::NoCors;
    options.credentials = FetchOptions::Credentials::SameOrigin;
    options.contentSecurityPolicyEnforcement = ContentSecurityPolicyEnforcement::DoNotEnforce;

    // InspectorThreadableLoaderClient deletes itself when the load completes.
    InspectorThreadableLoaderClient* inspectorThreadableLoaderClient = new InspectorThreadableLoaderClient(callback.copyRef());

    auto loader = DocumentThreadableLoader::create(*document, *inspectorThreadableLoaderClient, WTFMove(request), options);
    if (!loader) {
        inspectorThreadableLoaderClient->didFailLoaderCreation();
        return;
    }

    // If the load already completed, inspectorThreadableLoaderClient will have been deleted and we will have already called the callback.
    if (!callback->isActive())
        return;

    inspectorThreadableLoaderClient->setLoader(WTFMove(loader));
}

static Ref<Inspector::Protocol::Page::SearchResult> buildObjectForSearchResult(const String& requestId, const String& frameId, const String& url, int matchesCount)
{
    auto searchResult = Inspector::Protocol::Page::SearchResult::create()
        .setUrl(url)
        .setFrameId(frameId)
        .setMatchesCount(matchesCount)
        .release();
    searchResult->setRequestId(requestId);
    return searchResult;
}

void InspectorNetworkAgent::searchOtherRequests(const JSC::Yarr::RegularExpression& regex, RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Page::SearchResult>>& result)
{
    Vector<NetworkResourcesData::ResourceData*> resources = m_resourcesData->resources();
    for (auto* resourceData : resources) {
        if (resourceData->hasContent()) {
            int matchesCount = ContentSearchUtilities::countRegularExpressionMatches(regex, resourceData->content());
            if (matchesCount)
                result->addItem(buildObjectForSearchResult(resourceData->requestId(), resourceData->frameId(), resourceData->url(), matchesCount));
        }
    }
}

void InspectorNetworkAgent::searchInRequest(ErrorString& errorString, const String& requestId, const String& query, bool caseSensitive, bool isRegex, RefPtr<Inspector::Protocol::Array<Inspector::Protocol::GenericTypes::SearchMatch>>& results)
{
    NetworkResourcesData::ResourceData const* resourceData = m_resourcesData->data(requestId);
    if (!resourceData) {
        errorString = ASCIILiteral("No resource with given identifier found");
        return;
    }

    if (!resourceData->hasContent()) {
        errorString = ASCIILiteral("No resource content");
        return;
    }

    results = ContentSearchUtilities::searchInTextByLines(resourceData->content(), query, caseSensitive, isRegex);
}

void InspectorNetworkAgent::mainFrameNavigated(DocumentLoader& loader)
{
    if (m_cacheDisabled)
        MemoryCache::singleton().evictResources();

    m_resourcesData->clear(m_pageAgent->loaderId(&loader));
}

} // namespace WebCore
