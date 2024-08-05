/*
 * Copyright (C) 2015, 2016 Apple Inc. All rights reserved.
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
#include "APIPageConfiguration.h"

#include "APIProcessPoolConfiguration.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include "WebProcessPool.h"
#include "WebUserContentControllerProxy.h"

using namespace WebCore;
using namespace WebKit;

namespace API {

Ref<PageConfiguration> PageConfiguration::create()
{
    return adoptRef(*new PageConfiguration);
}

PageConfiguration::PageConfiguration()
{
}

PageConfiguration::~PageConfiguration()
{
}

Ref<PageConfiguration> PageConfiguration::copy() const
{
    auto copy = create();

    copy->m_processPool = this->m_processPool;
    copy->m_userContentController = this->m_userContentController;
    copy->m_pageGroup = this->m_pageGroup;
    copy->m_preferences = this->m_preferences;
    copy->m_preferenceValues = this->m_preferenceValues;
    copy->m_relatedPage = this->m_relatedPage;
    copy->m_visitedLinkStore = this->m_visitedLinkStore;
    copy->m_websiteDataStore = this->m_websiteDataStore;
    copy->m_sessionID = this->m_sessionID;
    copy->m_treatsSHA1SignedCertificatesAsInsecure = this->m_treatsSHA1SignedCertificatesAsInsecure;
#if PLATFORM(IOS)
    copy->m_alwaysRunsAtForegroundPriority = this->m_alwaysRunsAtForegroundPriority;
#endif
    copy->m_initialCapitalizationEnabled = this->m_initialCapitalizationEnabled;
    copy->m_cpuLimit = this->m_cpuLimit;
    copy->m_controlledByAutomation = this->m_controlledByAutomation;
    copy->m_overrideContentSecurityPolicy = this->m_overrideContentSecurityPolicy;

    return copy;
}


WebProcessPool* PageConfiguration::processPool()
{
    return m_processPool.get();
}

void PageConfiguration::setProcessPool(WebProcessPool* processPool)
{
    m_processPool = processPool;
}

WebUserContentControllerProxy* PageConfiguration::userContentController()
{
    return m_userContentController.get();
}

void PageConfiguration::setUserContentController(WebUserContentControllerProxy* userContentController)
{
    m_userContentController = userContentController;
}

WebPageGroup* PageConfiguration::pageGroup()
{
    return m_pageGroup.get();
}

void PageConfiguration::setPageGroup(WebPageGroup* pageGroup)
{
    m_pageGroup = pageGroup;
}

WebPreferences* PageConfiguration::preferences()
{
    return m_preferences.get();
}

void PageConfiguration::setPreferences(WebPreferences* preferences)
{
    m_preferences = preferences;
}

WebPageProxy* PageConfiguration::relatedPage()
{
    return m_relatedPage.get();
}

void PageConfiguration::setRelatedPage(WebPageProxy* relatedPage)
{
    m_relatedPage = relatedPage;
}


VisitedLinkStore* PageConfiguration::visitedLinkStore()
{
    return m_visitedLinkStore.get();
}

void PageConfiguration::setVisitedLinkStore(VisitedLinkStore* visitedLinkStore)
{
    m_visitedLinkStore = visitedLinkStore;
}

API::WebsiteDataStore* PageConfiguration::websiteDataStore()
{
    return m_websiteDataStore.get();
}

void PageConfiguration::setWebsiteDataStore(API::WebsiteDataStore* websiteDataStore)
{
    m_websiteDataStore = websiteDataStore;

    if (m_websiteDataStore)
        m_sessionID = m_websiteDataStore->websiteDataStore().sessionID();
    else
        m_sessionID = WebCore::SessionID();
}

WebCore::SessionID PageConfiguration::sessionID()
{
    ASSERT(!m_websiteDataStore || m_websiteDataStore->websiteDataStore().sessionID() == m_sessionID || m_sessionID == SessionID::legacyPrivateSessionID());

    return m_sessionID;
}

void PageConfiguration::setSessionID(WebCore::SessionID sessionID)
{
    m_sessionID = sessionID;
}

} // namespace API
