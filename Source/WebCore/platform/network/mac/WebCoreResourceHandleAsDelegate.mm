/*
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
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
#import "WebCoreResourceHandleAsDelegate.h"

#if !USE(CFURLCONNECTION)

#import "AuthenticationChallenge.h"
#import "AuthenticationMac.h"
#import "Logging.h"
#import "ResourceHandle.h"
#import "ResourceHandleClient.h"
#import "ResourceRequest.h"
#import "ResourceResponse.h"
#import "SharedBuffer.h"
#import "WebCoreURLResponse.h"
#import <pal/spi/cf/CFNetworkSPI.h>

using namespace WebCore;

@implementation WebCoreResourceHandleAsDelegate

- (id)initWithHandle:(ResourceHandle*)handle
{
    self = [self init];
    if (!self)
        return nil;
    m_handle = handle;
    return self;
}

- (void)detachHandle
{
    m_handle = nullptr;
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)newRequest redirectResponse:(NSURLResponse *)redirectResponse
{
    UNUSED_PARAM(connection);

    if (!m_handle)
        return nil;

    redirectResponse = synthesizeRedirectResponseIfNecessary([connection currentRequest], newRequest, redirectResponse);
    
    // See <rdar://problem/5380697>. This is a workaround for a behavior change in CFNetwork where willSendRequest gets called more often.
    if (!redirectResponse)
        return newRequest;

#if !LOG_DISABLED
    if ([redirectResponse isKindOfClass:[NSHTTPURLResponse class]])
        LOG(Network, "Handle %p delegate connection:%p willSendRequest:%@ redirectResponse:%d, Location:<%@>", m_handle, connection, [newRequest description], static_cast<int>([(id)redirectResponse statusCode]), [[(id)redirectResponse allHeaderFields] objectForKey:@"Location"]);
    else
        LOG(Network, "Handle %p delegate connection:%p willSendRequest:%@ redirectResponse:non-HTTP", m_handle, connection, [newRequest description]); 
#endif

    return m_handle->willSendRequest(newRequest, redirectResponse).nsURLRequest(UpdateHTTPBody);
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection *)connection
{
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connectionShouldUseCredentialStorage:%p", m_handle, connection);

#if PLATFORM(IOS)
    return NO;
#else
    if (!m_handle)
        return NO;

    return m_handle->shouldUseCredentialStorage();
#endif
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p didReceiveAuthenticationChallenge:%p", m_handle, connection, challenge);

    if (!m_handle) {
        [[challenge sender] cancelAuthenticationChallenge:challenge];
        return;
    }
    m_handle->didReceiveAuthenticationChallenge(core(challenge));
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace
{
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p canAuthenticateAgainstProtectionSpace:%@://%@:%u realm:%@ method:%@ %@%@", m_handle, connection, [protectionSpace protocol], [protectionSpace host], [protectionSpace port], [protectionSpace realm], [protectionSpace authenticationMethod], [protectionSpace isProxy] ? @"proxy:" : @"", [protectionSpace isProxy] ? [protectionSpace proxyType] : @"");

    if (!m_handle)
        return NO;

    return m_handle->canAuthenticateAgainstProtectionSpace(ProtectionSpace(protectionSpace));
}
#endif

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    LOG(Network, "Handle %p delegate connection:%p didReceiveResponse:%p (HTTP status %d, reported MIMEType '%s')", m_handle, connection, response, [response respondsToSelector:@selector(statusCode)] ? [(id)response statusCode] : 0, [[response MIMEType] UTF8String]);

    if (!m_handle || !m_handle->client())
        return;

    // Avoid MIME type sniffing if the response comes back as 304 Not Modified.
    int statusCode = [response respondsToSelector:@selector(statusCode)] ? [(id)response statusCode] : 0;
    if (statusCode != 304) {
        bool isMainResourceLoad = m_handle->firstRequest().requester() == ResourceRequest::Requester::Main;
        adjustMIMETypeIfNecessary([response _CFURLResponse], isMainResourceLoad);
    }

#if !PLATFORM(IOS)
    if ([m_handle->firstRequest().nsURLRequest(DoNotUpdateHTTPBody) _propertyForKey:@"ForceHTMLMIMEType"])
        [response _setMIMEType:@"text/html"];
#endif

    ResourceResponse resourceResponse(response);
    resourceResponse.setSource(ResourceResponse::Source::Network);
    ResourceHandle::getConnectionTimingData(connection, resourceResponse.deprecatedNetworkLoadMetrics());

    m_handle->didReceiveResponse(WTFMove(resourceResponse));
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data lengthReceived:(long long)lengthReceived
{
    UNUSED_PARAM(connection);
    UNUSED_PARAM(lengthReceived);

    LOG(Network, "Handle %p delegate connection:%p didReceiveData:%p lengthReceived:%lld", m_handle, connection, data, lengthReceived);

#if PLATFORM(IOS)
    if ([data length] == 0) // <rdar://problem/5532931>
        return;
#endif

    if (!m_handle || !m_handle->client())
        return;
    // FIXME: If we get more than 2B bytes in a single chunk, this code won't do the right thing.
    // However, with today's computers and networking speeds, this won't happen in practice.
    // Could be an issue with a giant local file.

    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=19793
    // -1 means we do not provide any data about transfer size to inspector so it would use
    // Content-Length headers or content size to show transfer size.
    m_handle->client()->didReceiveBuffer(m_handle, SharedBuffer::create(data), -1);
}

- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite
{
    UNUSED_PARAM(connection);
    UNUSED_PARAM(bytesWritten);

    LOG(Network, "Handle %p delegate connection:%p didSendBodyData:%d totalBytesWritten:%d totalBytesExpectedToWrite:%d", m_handle, connection, bytesWritten, totalBytesWritten, totalBytesExpectedToWrite);

    if (!m_handle || !m_handle->client())
        return;
    m_handle->client()->didSendData(m_handle, totalBytesWritten, totalBytesExpectedToWrite);
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connectionDidFinishLoading:%p", m_handle, connection);

    if (!m_handle || !m_handle->client())
        return;

    m_handle->client()->didFinishLoading(m_handle);
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p didFailWithError:%@", m_handle, connection, error);

    if (!m_handle || !m_handle->client())
        return;

    m_handle->client()->didFail(m_handle, error);
}


- (NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:(NSCachedURLResponse *)cachedResponse
{
    LOG(Network, "Handle %p delegate connection:%p willCacheResponse:%p", m_handle, connection, cachedResponse);

    UNUSED_PARAM(connection);

    if (!m_handle || !m_handle->client())
        return nil;

    return m_handle->client()->willCacheResponse(m_handle, cachedResponse);
}

@end

#endif // !USE(CFURLCONNECTION)

