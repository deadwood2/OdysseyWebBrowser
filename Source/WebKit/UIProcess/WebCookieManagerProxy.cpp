/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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
#include "WebCookieManagerProxy.h"

#include "APIArray.h"
#include "APISecurityOrigin.h"
#include "NetworkProcessMessages.h"
#include "OptionalCallbackID.h"
#include "WebCookieManagerMessages.h"
#include "WebCookieManagerProxyMessages.h"
#include "WebProcessPool.h"
#include <WebCore/Cookie.h>
#include <WebCore/SecurityOriginData.h>

using namespace WebCore;

namespace WebKit {

const char* WebCookieManagerProxy::supplementName()
{
    return "WebCookieManagerProxy";
}

Ref<WebCookieManagerProxy> WebCookieManagerProxy::create(WebProcessPool* processPool)
{
    return adoptRef(*new WebCookieManagerProxy(processPool));
}

WebCookieManagerProxy::WebCookieManagerProxy(WebProcessPool* processPool)
    : WebContextSupplement(processPool)
#if USE(SOUP)
    , m_cookiePersistentStorageType(SoupCookiePersistentStorageSQLite)
#endif
{
    WebContextSupplement::processPool()->addMessageReceiver(Messages::WebCookieManagerProxy::messageReceiverName(), *this);
}

WebCookieManagerProxy::~WebCookieManagerProxy()
{
    ASSERT(m_cookieObservers.isEmpty());
}

void WebCookieManagerProxy::initializeClient(const WKCookieManagerClientBase* client)
{
    m_client.initialize(client);
}

// WebContextSupplement

void WebCookieManagerProxy::processPoolDestroyed()
{
    m_callbacks.invalidate(CallbackBase::Error::OwnerWasInvalidated);

    Vector<Observer*> observers;
    for (auto& observerSet : m_cookieObservers.values()) {
        for (auto* observer : observerSet)
            observers.append(observer);
    }

    for (auto* observer : observers)
        observer->managerDestroyed();

    ASSERT(m_cookieObservers.isEmpty());
}

void WebCookieManagerProxy::processDidClose(WebProcessProxy*)
{
    m_callbacks.invalidate(CallbackBase::Error::ProcessExited);
}

void WebCookieManagerProxy::processDidClose(NetworkProcessProxy*)
{
    m_callbacks.invalidate(CallbackBase::Error::ProcessExited);
}

void WebCookieManagerProxy::refWebContextSupplement()
{
    API::Object::ref();
}

void WebCookieManagerProxy::derefWebContextSupplement()
{
    API::Object::deref();
}

void WebCookieManagerProxy::getHostnamesWithCookies(SessionID sessionID, Function<void (API::Array*, CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::GetHostnamesWithCookies(sessionID, callbackID));
}

void WebCookieManagerProxy::didGetHostnamesWithCookies(const Vector<String>& hostnames, WebKit::CallbackID callbackID)
{
    auto callback = m_callbacks.take<ArrayCallback>(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        return;
    }

    callback->performCallbackWithReturnValue(API::Array::createStringArray(hostnames).ptr());
}

void WebCookieManagerProxy::deleteCookiesForHostname(SessionID sessionID, const String& hostname)
{
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::DeleteCookiesForHostname(sessionID, hostname));
}

void WebCookieManagerProxy::deleteAllCookies(SessionID sessionID)
{
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::DeleteAllCookies(sessionID));
}

void WebCookieManagerProxy::deleteCookie(SessionID sessionID, const Cookie& cookie, Function<void (CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::DeleteCookie(sessionID, cookie, callbackID));
}

void WebCookieManagerProxy::deleteAllCookiesModifiedSince(SessionID sessionID, std::chrono::system_clock::time_point time, Function<void (CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::DeleteAllCookiesModifiedSince(sessionID, time, callbackID));
}

void WebCookieManagerProxy::setCookie(SessionID sessionID, const Cookie& cookie, Function<void (CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::SetCookie(sessionID, cookie, callbackID));
}

void WebCookieManagerProxy::setCookies(SessionID sessionID, const Vector<Cookie>& cookies, const URL& url, const URL& mainDocumentURL, Function<void (CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::SetCookies(sessionID, cookies, url, mainDocumentURL, callbackID));
}

void WebCookieManagerProxy::getAllCookies(SessionID sessionID, Function<void (const Vector<Cookie>&, CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::GetAllCookies(sessionID, callbackID));
}

void WebCookieManagerProxy::getCookies(SessionID sessionID, const URL& url, Function<void (const Vector<Cookie>&, CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::GetCookies(sessionID, url, callbackID));
}

void WebCookieManagerProxy::didSetCookies(WebKit::CallbackID callbackID)
{
    m_callbacks.take<VoidCallback>(callbackID)->performCallback();
}

void WebCookieManagerProxy::didGetCookies(const Vector<Cookie>& cookies, WebKit::CallbackID callbackID)
{
    m_callbacks.take<GetCookiesCallback>(callbackID)->performCallbackWithReturnValue(cookies);
}

void WebCookieManagerProxy::didDeleteCookies(WebKit::CallbackID callbackID)
{
    m_callbacks.take<VoidCallback>(callbackID)->performCallback();
}

void WebCookieManagerProxy::startObservingCookieChanges(SessionID sessionID)
{
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::StartObservingCookieChanges(sessionID));
}

void WebCookieManagerProxy::stopObservingCookieChanges(SessionID sessionID)
{
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::StopObservingCookieChanges(sessionID));
}


void WebCookieManagerProxy::setCookieObserverCallback(WebCore::SessionID sessionID, WTF::Function<void ()>&& callback)
{
    if (callback)
        m_legacyCookieObservers.set(sessionID, WTFMove(callback));
    else
        m_legacyCookieObservers.remove(sessionID);
}

void WebCookieManagerProxy::registerObserver(WebCore::SessionID sessionID, Observer& observer)
{
    auto result = m_cookieObservers.set(sessionID, HashSet<Observer*>());
    result.iterator->value.add(&observer);

    if (result.isNewEntry)
        startObservingCookieChanges(sessionID);
}

void WebCookieManagerProxy::unregisterObserver(WebCore::SessionID sessionID, Observer& observer)
{
    auto iterator = m_cookieObservers.find(sessionID);
    if (iterator == m_cookieObservers.end())
        return;

    iterator->value.remove(&observer);
    if (!iterator->value.isEmpty())
        return;

    m_cookieObservers.remove(iterator);
    stopObservingCookieChanges(sessionID);
}

void WebCookieManagerProxy::cookiesDidChange(SessionID sessionID)
{
    m_client.cookiesDidChange(this);
    auto legacyIterator = m_legacyCookieObservers.find(sessionID);
    if (legacyIterator != m_legacyCookieObservers.end())
        ((*legacyIterator).value)();

    auto iterator = m_cookieObservers.find(sessionID);
    if (iterator == m_cookieObservers.end())
        return;

    for (auto* observer : iterator->value)
        observer->cookiesDidChange();
}

void WebCookieManagerProxy::setHTTPCookieAcceptPolicy(SessionID, HTTPCookieAcceptPolicy policy, Function<void (CallbackBase::Error)>&& callbackFunction)
{
#if PLATFORM(COCOA)
    if (!processPool()->isUsingTestingNetworkSession())
        persistHTTPCookieAcceptPolicy(policy);
#endif
#if USE(SOUP)
    processPool()->setInitialHTTPCookieAcceptPolicy(policy);
#endif

    // The policy is not sent to newly created processes (only Soup does that via setInitialHTTPCookieAcceptPolicy()). This is not a serious problem, because:
    // - When testing, we only have one WebProcess and one NetworkProcess, and WebKitTestRunner never restarts them;
    // - When not testing, Cocoa has the policy persisted, and thus new processes use it (even for ephemeral sessions).
    processPool()->sendToAllProcesses(Messages::WebCookieManager::SetHTTPCookieAcceptPolicy(policy, OptionalCallbackID()));

    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcess(Messages::WebCookieManager::SetHTTPCookieAcceptPolicy(policy, OptionalCallbackID(callbackID)));
}

void WebCookieManagerProxy::getHTTPCookieAcceptPolicy(SessionID, Function<void (HTTPCookieAcceptPolicy, CallbackBase::Error)>&& callbackFunction)
{
    auto callbackID = m_callbacks.put(WTFMove(callbackFunction));
    processPool()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::WebCookieManager::GetHTTPCookieAcceptPolicy(callbackID));
}

void WebCookieManagerProxy::didGetHTTPCookieAcceptPolicy(uint32_t policy, WebKit::CallbackID callbackID)
{
    m_callbacks.take<HTTPCookieAcceptPolicyCallback>(callbackID)->performCallbackWithReturnValue(policy);
}

void WebCookieManagerProxy::didSetHTTPCookieAcceptPolicy(WebKit::CallbackID callbackID)
{
    m_callbacks.take<VoidCallback>(callbackID)->performCallback();
}

void WebCookieManagerProxy::setCookieStoragePartitioningEnabled(bool enabled)
{
#if PLATFORM(COCOA)
    processPool()->sendToNetworkingProcess(Messages::NetworkProcess::SetCookieStoragePartitioningEnabled(enabled));
#else
    UNUSED_PARAM(enabled);
#endif
}

} // namespace WebKit
