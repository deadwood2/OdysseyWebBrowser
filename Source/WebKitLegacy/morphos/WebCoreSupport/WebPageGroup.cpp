/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#include "WebKit.h"
#include "WebPageGroup.h"

#include "WebStorageNamespaceProvider.h"
#include "WebPage.h"
#include "WebVisitedLinkStore.h"
#include <WebCore/UserContentController.h>
#include <WebCore/CommonVM.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringHash.h>

using namespace WebCore;

namespace  WebKit {

// Any named groups will live for the lifetime of the process, thanks to the reference held by the RefPtr.
static HashMap<String, RefPtr<WebPageGroup>>& webViewGroups()
{
    static NeverDestroyed<HashMap<String, RefPtr<WebPageGroup>>> webViewGroups;

    return webViewGroups;
}

Ref<WebPageGroup> WebPageGroup::getOrCreate(const String& name, const String& localStorageDatabasePath)
{
    if (name.isEmpty())
        return adoptRef(*new WebPageGroup(String(), localStorageDatabasePath));

    auto& webViewGroup = webViewGroups().add(name, nullptr).iterator->value;
    if (!webViewGroup) {
        auto result = adoptRef(*new WebPageGroup(name, localStorageDatabasePath));
        webViewGroup = result.copyRef();
        return result;
    }

    if (!webViewGroup->m_storageNamespaceProvider && webViewGroup->m_localStorageDatabasePath.isEmpty() && !localStorageDatabasePath.isEmpty())
        webViewGroup->m_localStorageDatabasePath = localStorageDatabasePath;

    return *webViewGroup;
}

WebPageGroup* WebPageGroup::get(const String& name)
{
    ASSERT(!name.isEmpty());

    return webViewGroups().get(name);
}

WebPageGroup::WebPageGroup(const String& name, const String& localStorageDatabasePath)
    : m_name(name)
    , m_localStorageDatabasePath(localStorageDatabasePath)
    , m_worldForUserScripts(DOMWrapperWorld::create(WebCore::commonVM(), DOMWrapperWorld::Type::User))
    , m_userContentController(UserContentController::create())
    , m_visitedLinkStore(WebVisitedLinkStore::create())
{
}

WebPageGroup::~WebPageGroup()
{
    ASSERT(m_name.isEmpty());
    ASSERT(m_webPages.isEmpty());
}

void WebPageGroup::addWebPage(WebPage *webView)
{
    ASSERT(!m_webPages.contains(webView));

    m_webPages.add(webView);
}

void WebPageGroup::removeWebPage(WebPage *webView)
{
    ASSERT(m_webPages.contains(webView));

    m_webPages.remove(webView);
}

StorageNamespaceProvider& WebPageGroup::storageNamespaceProvider()
{
    if (!m_storageNamespaceProvider)
        m_storageNamespaceProvider = WebKit::WebStorageNamespaceProvider::create(m_localStorageDatabasePath);

    return *m_storageNamespaceProvider;
}

}

