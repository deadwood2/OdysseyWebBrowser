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
#import "TestProtocol.h"

#import <WebKit/WKBrowsingContextController.h>
#import <wtf/RetainPtr.h>

static NSString *testScheme;

@implementation TestProtocol

+ (BOOL)canInitWithRequest:(NSURLRequest *)request
{
    return [request.URL.scheme caseInsensitiveCompare:testScheme] == NSOrderedSame;
}

+ (NSURLRequest *)canonicalRequestForRequest:(NSURLRequest *)request
{
    return request;
}

+ (BOOL)requestIsCacheEquivalent:(NSURLRequest *)a toRequest:(NSURLRequest *)b
{
    return NO;
}

+ (NSString *)scheme
{
    return testScheme;
}

+ (void)registerWithScheme:(NSString *)scheme
{
    testScheme = [scheme retain];
    [NSURLProtocol registerClass:[self class]];
#if WK_API_ENABLED
    [WKBrowsingContextController registerSchemeForCustomProtocol:testScheme];
#endif
}

+ (void)unregister
{
#if WK_API_ENABLED
    [WKBrowsingContextController unregisterSchemeForCustomProtocol:testScheme];
#endif
    [NSURLProtocol unregisterClass:[self class]];
    [testScheme release];
    testScheme = nil;
}

- (void)startLoading
{
    NSURL *requestURL = self.request.URL;
    EXPECT_TRUE([requestURL.scheme isEqualToString:testScheme]);

    NSData *data = [@"PASS" dataUsingEncoding:NSASCIIStringEncoding];
    RetainPtr<NSURLResponse> response = adoptNS([[NSURLResponse alloc] initWithURL:requestURL MIMEType:@"text/html" expectedContentLength:data.length textEncodingName:nil]);

    if ([requestURL.host isEqualToString:@"redirect"]) {
        NSURL *redirectURL = [NSURL URLWithString:[NSString stringWithFormat:@"%@://%@", testScheme, requestURL.query]];
        [self.client URLProtocol:self wasRedirectedToRequest:[NSURLRequest requestWithURL:redirectURL] redirectResponse:response.get()];
        return;
    }

    [self.client URLProtocol:self didReceiveResponse:response.get() cacheStoragePolicy:NSURLCacheStorageNotAllowed];
    [self.client URLProtocol:self didLoadData:data];
    [self.client URLProtocolDidFinishLoading:self];
}

- (void)stopLoading
{
}

@end
