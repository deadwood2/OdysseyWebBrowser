/*
    Copyright (C) 2012 Intel Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef WebPlatformStrategies_h
#define WebPlatformStrategies_h

#include "CookiesStrategy.h"
#include "LoaderStrategy.h"
#include "PasteboardStrategy.h"
#include "PlatformStrategies.h"

class WebPlatformStrategies : public WebCore::PlatformStrategies, private WebCore::CookiesStrategy, private WebCore::LoaderStrategy
{
public:
    static void initialize();

private:
    WebPlatformStrategies();

    // WebCore::PlatformStrategies
    virtual WebCore::CookiesStrategy* createCookiesStrategy() override;
    virtual WebCore::LoaderStrategy* createLoaderStrategy() override;
    virtual WebCore::PasteboardStrategy* createPasteboardStrategy() override;
    virtual WebCore::BlobRegistry* createBlobRegistry() override;

    // WebCore::CookiesStrategy
    virtual String cookiesForDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&);
    virtual void setCookiesFromDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, const String&);
    virtual bool cookiesEnabled(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&);
    virtual String cookieRequestHeaderFieldValue(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&);
    virtual String cookieRequestHeaderFieldValue(WebCore::SessionID, const WebCore::URL& firstParty, const WebCore::URL&);
    virtual bool getRawCookies(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, Vector<WebCore::Cookie>&);
    virtual void deleteCookie(const WebCore::NetworkStorageSession&, const WebCore::URL&, const String&);
    virtual void addCookie(const WebCore::NetworkStorageSession&, const WebCore::URL&, const WebCore::Cookie&);

    // WebCore::DatabaseStrategy
    // - Using default implementation.

    // WebCore::LoaderStrategy
    virtual RefPtr<WebCore::SubresourceLoader> loadResource(WebCore::Frame&, WebCore::CachedResource&, const WebCore::ResourceRequest&, const WebCore::ResourceLoaderOptions&) override;
    virtual void loadResourceSynchronously(WebCore::NetworkingContext*, unsigned long identifier, const WebCore::ResourceRequest&, WebCore::StoredCredentials, WebCore::ClientCredentialPolicy, WebCore::ResourceError&, WebCore::ResourceResponse&, Vector<char>& data) override;

    virtual void remove(WebCore::ResourceLoader*) override;
    virtual void setDefersLoading(WebCore::ResourceLoader*, bool) override;
    virtual void crossOriginRedirectReceived(WebCore::ResourceLoader*, const WebCore::URL& redirectURL) override;

    virtual void servePendingRequests(WebCore::ResourceLoadPriority minimumPriority = WebCore::ResourceLoadPriority::VeryLow) override;
    virtual void suspendPendingRequests() override;
    virtual void resumePendingRequests() override;

    virtual void createPingHandle(WebCore::NetworkingContext*, WebCore::ResourceRequest&, bool shouldUseCredentialStorage, bool shouldFollowRedirects) override;
    void storeDerivedDataToCache(const SHA1::Digest& bodyKey, const String& type, const String& partition, WebCore::SharedBuffer&) override;
};

#endif // WebPlatformStrategies_h

