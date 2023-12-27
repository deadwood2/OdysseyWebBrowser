/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "WebPluginSiteDataManager.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "APIArray.h"
#include "PluginProcessManager.h"
#include "WebProcessMessages.h"
#include "WebProcessPool.h"

using namespace WebCore;

namespace WebKit {

class WebPluginSiteDataManager::GetSitesWithDataState {
public:
    explicit GetSitesWithDataState(WebPluginSiteDataManager* webPluginSiteDataManager, uint64_t callbackID)
        : m_webPluginSiteDataManager(webPluginSiteDataManager)
        , m_callbackID(callbackID)
        , m_plugins(webPluginSiteDataManager->m_processPool->pluginInfoStore().plugins())
    {
    }

    void getSitesWithDataForNextPlugin()
    {
        if (m_plugins.isEmpty()) {
            Vector<String> sites;
            copyToVector(m_sites, sites);

            m_webPluginSiteDataManager->didGetSitesWithDataForAllPlugins(sites, m_callbackID);
            return;
        }

        PluginProcessManager::singleton().getSitesWithData(m_plugins.last(), m_webPluginSiteDataManager, m_callbackID);
        m_plugins.removeLast();
    }

    void didGetSitesWithDataForSinglePlugin(const Vector<String>& sites)
    {
        for (size_t i = 0; i < sites.size(); ++i)
            m_sites.add(sites[i]);

        getSitesWithDataForNextPlugin();
    }
    
private:
    WebPluginSiteDataManager* m_webPluginSiteDataManager;
    uint64_t m_callbackID;
    Vector<PluginModuleInfo> m_plugins;
    HashSet<String> m_sites;
};

class WebPluginSiteDataManager::ClearSiteDataState {
public:
    explicit ClearSiteDataState(WebPluginSiteDataManager* webPluginSiteDataManager, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID)
        : m_webPluginSiteDataManager(webPluginSiteDataManager)
        , m_sites(sites)
        , m_flags(flags)
        , m_maxAgeInSeconds(maxAgeInSeconds)
        , m_callbackID(callbackID)
        , m_plugins(webPluginSiteDataManager->m_processPool->pluginInfoStore().plugins())
    {
    }

    void clearSiteDataForNextPlugin()
    {
        if (m_plugins.isEmpty()) {
            m_webPluginSiteDataManager->didClearSiteDataForAllPlugins(m_callbackID);
            return;
        }

        PluginProcessManager::singleton().clearSiteData(m_plugins.last(), m_webPluginSiteDataManager, m_sites, m_flags, m_maxAgeInSeconds, m_callbackID);
        m_plugins.removeLast();
    }

    void didClearSiteDataForSinglePlugin()
    {
        clearSiteDataForNextPlugin();
    }
    
private:
    WebPluginSiteDataManager* m_webPluginSiteDataManager;
    Vector<String> m_sites;
    uint64_t m_flags;
    uint64_t m_maxAgeInSeconds;
    uint64_t m_callbackID;
    Vector<PluginModuleInfo> m_plugins;
};

PassRefPtr<WebPluginSiteDataManager> WebPluginSiteDataManager::create(WebProcessPool* processPool)
{
    return adoptRef(new WebPluginSiteDataManager(processPool));
}

WebPluginSiteDataManager::WebPluginSiteDataManager(WebProcessPool* processPool)
    : m_processPool(processPool)
{
}

WebPluginSiteDataManager::~WebPluginSiteDataManager()
{
    ASSERT(m_arrayCallbacks.isEmpty());
    ASSERT(m_voidCallbacks.isEmpty());
    ASSERT(m_pendingGetSitesWithData.isEmpty());
    ASSERT(m_pendingClearSiteData.isEmpty());
}

void WebPluginSiteDataManager::invalidate()
{
    invalidateCallbackMap(m_arrayCallbacks, CallbackBase::Error::OwnerWasInvalidated);

    m_pendingGetSitesWithData.clear();
    m_pendingClearSiteData.clear();
}

void WebPluginSiteDataManager::getSitesWithData(std::function<void (API::Array*, CallbackBase::Error)> callbackFunction)
{
    RefPtr<ArrayCallback> callback = ArrayCallback::create(WTF::move(callbackFunction));

    if (!m_processPool) {
        callback->invalidate();
        return;
    }

    uint64_t callbackID = callback->callbackID();
    m_arrayCallbacks.set(callbackID, callback.release());

    ASSERT(!m_pendingGetSitesWithData.contains(callbackID));

    GetSitesWithDataState* state = new GetSitesWithDataState(this, callbackID);
    m_pendingGetSitesWithData.set(callbackID, std::unique_ptr<GetSitesWithDataState>(state));
    state->getSitesWithDataForNextPlugin();
}

void WebPluginSiteDataManager::didGetSitesWithData(const Vector<String>& sites, uint64_t callbackID)
{
    RefPtr<ArrayCallback> callback = m_arrayCallbacks.take(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        return;
    }

    callback->performCallbackWithReturnValue(API::Array::createStringArray(sites).ptr());
}

void WebPluginSiteDataManager::clearSiteData(API::Array* sites, uint64_t flags, uint64_t maxAgeInSeconds, std::function<void (CallbackBase::Error)> callbackFunction)
{
    RefPtr<VoidCallback> callback = VoidCallback::create(WTF::move(callbackFunction));
    if (!m_processPool) {
        // FIXME: If the context is invalid we should not call the callback. It'd be better to just return false from clearSiteData.
        callback->invalidate(CallbackBase::Error::OwnerWasInvalidated);
        return;
    }

    Vector<String> sitesVector;

    // If the array is empty, don't do anything.
    if (sites) {
        if (!sites->size()) {
            callback->performCallback();
            return;
        }

        for (size_t i = 0; i < sites->size(); ++i) {
            if (API::String* site = sites->at<API::String>(i))
                sitesVector.append(site->string());
        }
    }

    uint64_t callbackID = callback->callbackID();
    m_voidCallbacks.set(callbackID, callback.release());

    ASSERT(!m_pendingClearSiteData.contains(callbackID));

    ClearSiteDataState* state = new ClearSiteDataState(this, sitesVector, flags, maxAgeInSeconds, callbackID);
    m_pendingClearSiteData.set(callbackID, std::unique_ptr<ClearSiteDataState>(state));
    state->clearSiteDataForNextPlugin();
}

void WebPluginSiteDataManager::didClearSiteData(uint64_t callbackID)
{
    RefPtr<VoidCallback> callback = m_voidCallbacks.take(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        return;
    }

    callback->performCallback();
}

void WebPluginSiteDataManager::didGetSitesWithDataForSinglePlugin(const Vector<String>& sites, uint64_t callbackID)
{
    GetSitesWithDataState* state = m_pendingGetSitesWithData.get(callbackID);
    ASSERT(state);

    state->didGetSitesWithDataForSinglePlugin(sites);
}

void WebPluginSiteDataManager::didGetSitesWithDataForAllPlugins(const Vector<String>& sites, uint64_t callbackID)
{
    std::unique_ptr<GetSitesWithDataState> state = m_pendingGetSitesWithData.take(callbackID);
    ASSERT(state);

    didGetSitesWithData(sites, callbackID);
}

void WebPluginSiteDataManager::didClearSiteDataForSinglePlugin(uint64_t callbackID)
{
    ClearSiteDataState* state = m_pendingClearSiteData.get(callbackID);
    ASSERT(state);
    
    state->didClearSiteDataForSinglePlugin();
}

void WebPluginSiteDataManager::didClearSiteDataForAllPlugins(uint64_t callbackID)
{
    std::unique_ptr<ClearSiteDataState> state = m_pendingClearSiteData.take(callbackID);
    ASSERT(state);

    didClearSiteData(callbackID);
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
