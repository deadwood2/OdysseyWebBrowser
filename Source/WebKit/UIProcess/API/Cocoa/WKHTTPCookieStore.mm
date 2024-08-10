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

#import "config.h"
#import "WKHTTPCookieStoreInternal.h"

#if WK_API_ENABLED

#import "HTTPCookieAcceptPolicy.h"
#import <WebCore/Cookie.h>
#import <WebCore/URL.h>
#import <pal/spi/cf/CFNetworkSPI.h>
#import <wtf/HashMap.h>
#import <wtf/RetainPtr.h>
#import <wtf/WeakObjCPtr.h>

static NSArray<NSHTTPCookie *> *coreCookiesToNSCookies(const Vector<WebCore::Cookie>& coreCookies)
{
    NSMutableArray<NSHTTPCookie *> *nsCookies = [NSMutableArray arrayWithCapacity:coreCookies.size()];

    for (auto& cookie : coreCookies)
        [nsCookies addObject:(NSHTTPCookie *)cookie];

    return nsCookies;
}

class WKHTTPCookieStoreObserver : public API::HTTPCookieStore::Observer {
public:
    explicit WKHTTPCookieStoreObserver(id<WKHTTPCookieStoreObserver> observer)
        : m_observer(observer)
    {
    }

private:
    void cookiesDidChange(API::HTTPCookieStore& cookieStore) final
    {
        [m_observer.get() cookiesDidChangeInCookieStore:WebKit::wrapper(cookieStore)];
    }

    WeakObjCPtr<id<WKHTTPCookieStoreObserver>> m_observer;
};

@implementation WKHTTPCookieStore {
    HashMap<id<WKHTTPCookieStoreObserver>, std::unique_ptr<WKHTTPCookieStoreObserver>> _observers;
}

- (void)dealloc
{
    for (auto& observer : _observers.values())
        _cookieStore->unregisterObserver(*observer);

    _cookieStore->API::HTTPCookieStore::~HTTPCookieStore();

    [super dealloc];
}

- (void)getAllCookies:(void (^)(NSArray<NSHTTPCookie *> *))completionHandler
{
    _cookieStore->cookies([handler = adoptNS([completionHandler copy])](const Vector<WebCore::Cookie>& cookies) {
        auto rawHandler = (void (^)(NSArray<NSHTTPCookie *> *))handler.get();
        rawHandler(coreCookiesToNSCookies(cookies));
    });
}

- (void)setCookie:(NSHTTPCookie *)cookie completionHandler:(void (^)(void))completionHandler
{
    _cookieStore->setCookie(cookie, [handler = adoptNS([completionHandler copy])]() {
        auto rawHandler = (void (^)())handler.get();
        if (rawHandler)
            rawHandler();
    });

}

- (void)deleteCookie:(NSHTTPCookie *)cookie completionHandler:(void (^)(void))completionHandler
{
    _cookieStore->deleteCookie(cookie, [handler = adoptNS([completionHandler copy])]() {
        auto rawHandler = (void (^)())handler.get();
        if (rawHandler)
            rawHandler();
    });
}

- (void)addObserver:(id<WKHTTPCookieStoreObserver>)observer
{
    auto result = _observers.add(observer, nullptr);
    if (!result.isNewEntry)
        return;

    result.iterator->value = std::make_unique<WKHTTPCookieStoreObserver>(observer);
    _cookieStore->registerObserver(*result.iterator->value);
}

- (void)removeObserver:(id<WKHTTPCookieStoreObserver>)observer
{
    auto result = _observers.take(observer);
    if (!result)
        return;

    _cookieStore->unregisterObserver(*result);
}

#pragma mark WKObject protocol implementation

- (API::Object&)_apiObject
{
    return *_cookieStore;
}

@end

#endif
