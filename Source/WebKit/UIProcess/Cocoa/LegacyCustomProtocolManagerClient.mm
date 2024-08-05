/*
 * Copyright (C) 2012-2017 Apple Inc. All rights reserved.
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
#import "LegacyCustomProtocolManagerClient.h"

#import "DataReference.h"
#import "LegacyCustomProtocolManagerProxy.h"
#import <WebCore/ResourceError.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/ResourceResponse.h>

using namespace WebCore;
using namespace WebKit;

@interface WKCustomProtocolLoader : NSObject <NSURLConnectionDelegate> {
@private
    LegacyCustomProtocolManagerProxy* _customProtocolManagerProxy;
    uint64_t _customProtocolID;
    NSURLCacheStoragePolicy _storagePolicy;
    NSURLConnection *_urlConnection;
}
- (id)initWithLegacyCustomProtocolManagerProxy:(LegacyCustomProtocolManagerProxy*)customProtocolManagerProxy customProtocolID:(uint64_t)customProtocolID request:(NSURLRequest *)request;
- (void)customProtocolManagerProxyDestroyed;
@end

@implementation WKCustomProtocolLoader

- (id)initWithLegacyCustomProtocolManagerProxy:(LegacyCustomProtocolManagerProxy*)customProtocolManagerProxy customProtocolID:(uint64_t)customProtocolID request:(NSURLRequest *)request
{
    self = [super init];
    if (!self)
        return nil;

    ASSERT(customProtocolManagerProxy);
    ASSERT(request);
    _customProtocolManagerProxy = customProtocolManagerProxy;
    _customProtocolID = customProtocolID;
    _storagePolicy = NSURLCacheStorageNotAllowed;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    _urlConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:NO];
    [_urlConnection scheduleInRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    [_urlConnection start];
#pragma clang diagnostic pop

    return self;
}

- (void)dealloc
{
    [_urlConnection cancel];
    [_urlConnection release];
    [super dealloc];
}

- (void)customProtocolManagerProxyDestroyed
{
    ASSERT(_customProtocolManagerProxy);
    _customProtocolManagerProxy = nullptr;
    [_urlConnection cancel];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    ResourceError coreError(error);
    _customProtocolManagerProxy->didFailWithError(_customProtocolID, coreError);
    _customProtocolManagerProxy->stopLoading(_customProtocolID);
}

- (NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:(NSCachedURLResponse *)cachedResponse
{
    ASSERT(_storagePolicy == NSURLCacheStorageNotAllowed);
    _storagePolicy = [cachedResponse storagePolicy];
    return cachedResponse;
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    ResourceResponse coreResponse(response);
    _customProtocolManagerProxy->didReceiveResponse(_customProtocolID, coreResponse, _storagePolicy);
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    IPC::DataReference coreData(static_cast<const uint8_t*>([data bytes]), [data length]);
    _customProtocolManagerProxy->didLoadData(_customProtocolID, coreData);
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse
{
    if (redirectResponse) {
        _customProtocolManagerProxy->wasRedirectedToRequest(_customProtocolID, request, redirectResponse);
        return nil;
    }
    return request;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    _customProtocolManagerProxy->didFinishLoading(_customProtocolID);
    _customProtocolManagerProxy->stopLoading(_customProtocolID);
}

@end

namespace WebKit {

void LegacyCustomProtocolManagerClient::startLoading(LegacyCustomProtocolManagerProxy& manager, uint64_t customProtocolID, const ResourceRequest& coreRequest)
{
    NSURLRequest *request = coreRequest.nsURLRequest(DoNotUpdateHTTPBody);
    if (!request)
        return;

    WKCustomProtocolLoader *loader = [[WKCustomProtocolLoader alloc] initWithLegacyCustomProtocolManagerProxy:&manager customProtocolID:customProtocolID request:request];
    ASSERT(loader);
    ASSERT(!m_loaderMap.contains(customProtocolID));
    m_loaderMap.add(customProtocolID, loader);
    [loader release];
}

void LegacyCustomProtocolManagerClient::stopLoading(LegacyCustomProtocolManagerProxy&, uint64_t customProtocolID)
{
    m_loaderMap.remove(customProtocolID);
}

void LegacyCustomProtocolManagerClient::invalidate(LegacyCustomProtocolManagerProxy&)
{
    while (!m_loaderMap.isEmpty()) {
        auto loader = m_loaderMap.take(m_loaderMap.begin()->key);
        ASSERT(loader);
        [loader customProtocolManagerProxyDestroyed];
    }
}

} // namespace WebKit
