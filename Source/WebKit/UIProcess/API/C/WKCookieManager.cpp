/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "WKCookieManager.h"

#include "APIArray.h"
#include "WKAPICast.h"
#include "WebCookieManagerProxy.h"

using namespace WebKit;

WKTypeID WKCookieManagerGetTypeID()
{
    return toAPI(WebCookieManagerProxy::APIType);
}

void WKCookieManagerSetClient(WKCookieManagerRef cookieManagerRef, const WKCookieManagerClientBase* wkClient)
{
    toImpl(cookieManagerRef)->initializeClient(wkClient);
}

void WKCookieManagerGetHostnamesWithCookies(WKCookieManagerRef cookieManagerRef, void* context, WKCookieManagerGetCookieHostnamesFunction callback)
{
    toImpl(cookieManagerRef)->getHostnamesWithCookies(WebCore::SessionID::defaultSessionID(), toGenericCallbackFunction(context, callback));
}

void WKCookieManagerDeleteCookiesForHostname(WKCookieManagerRef cookieManagerRef, WKStringRef hostname)
{
    toImpl(cookieManagerRef)->deleteCookiesForHostname(WebCore::SessionID::defaultSessionID(), toImpl(hostname)->string());
}

void WKCookieManagerDeleteAllCookies(WKCookieManagerRef cookieManagerRef)
{
    toImpl(cookieManagerRef)->deleteAllCookies(WebCore::SessionID::defaultSessionID());
}

void WKCookieManagerDeleteAllCookiesModifiedAfterDate(WKCookieManagerRef cookieManagerRef, double date)
{
    using namespace std::chrono;

    auto time = system_clock::time_point(duration_cast<system_clock::duration>(duration<double>(date)));
    toImpl(cookieManagerRef)->deleteAllCookiesModifiedSince(WebCore::SessionID::defaultSessionID(), time, [](CallbackBase::Error){});
}

void WKCookieManagerSetHTTPCookieAcceptPolicy(WKCookieManagerRef cookieManager, WKHTTPCookieAcceptPolicy policy)
{
    toImpl(cookieManager)->setHTTPCookieAcceptPolicy(WebCore::SessionID::defaultSessionID(), toHTTPCookieAcceptPolicy(policy), [](CallbackBase::Error){});
}

void WKCookieManagerGetHTTPCookieAcceptPolicy(WKCookieManagerRef cookieManager, void* context, WKCookieManagerGetHTTPCookieAcceptPolicyFunction callback)
{
    toImpl(cookieManager)->getHTTPCookieAcceptPolicy(WebCore::SessionID::defaultSessionID(), toGenericCallbackFunction<WKHTTPCookieAcceptPolicy, HTTPCookieAcceptPolicy>(context, callback));
}

void WKCookieManagerSetCookieStoragePartitioningEnabled(WKCookieManagerRef cookieManager, bool enabled)
{
    toImpl(cookieManager)->setCookieStoragePartitioningEnabled(enabled);
}

void WKCookieManagerStartObservingCookieChanges(WKCookieManagerRef cookieManager)
{
    toImpl(cookieManager)->startObservingCookieChanges(WebCore::SessionID::defaultSessionID());
}

void WKCookieManagerStopObservingCookieChanges(WKCookieManagerRef cookieManager)
{
    toImpl(cookieManager)->stopObservingCookieChanges(WebCore::SessionID::defaultSessionID());
}
