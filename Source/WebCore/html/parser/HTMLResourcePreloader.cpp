/*
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLResourcePreloader.h"

#include "CachedResourceLoader.h"
#include "Document.h"

#include "MediaList.h"
#include "MediaQueryEvaluator.h"
#include "RenderView.h"

namespace WebCore {

URL PreloadRequest::completeURL(Document& document)
{
    return document.completeURL(m_resourceURL, m_baseURL.isEmpty() ? document.baseURL() : m_baseURL);
}

CachedResourceRequest PreloadRequest::resourceRequest(Document& document)
{
    ASSERT(isMainThread());
    CachedResourceRequest request(ResourceRequest(completeURL(document)));
    request.setInitiator(m_initiator);
    request.setAsPotentiallyCrossOrigin(m_crossOriginMode, document);
    return request;
}

void HTMLResourcePreloader::preload(PreloadRequestStream requests)
{
    for (auto& request : requests)
        preload(WTFMove(request));
}

static bool mediaAttributeMatches(Document& document, const RenderStyle* renderStyle, const String& attributeValue)
{
    auto mediaQueries = MediaQuerySet::createAllowingDescriptionSyntax(attributeValue);
    return MediaQueryEvaluator { "screen", document, renderStyle }.evaluate(mediaQueries.get());
}

void HTMLResourcePreloader::preload(std::unique_ptr<PreloadRequest> preload)
{
    ASSERT(m_document.frame());
    ASSERT(m_document.renderView());
    if (!preload->media().isEmpty() && !mediaAttributeMatches(m_document, &m_document.renderView()->style(), preload->media()))
        return;

    CachedResourceRequest request = preload->resourceRequest(m_document);
    m_document.cachedResourceLoader().preload(preload->resourceType(), request, preload->charset());
}


}
