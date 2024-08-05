/*
* Copyright (C) 2017 Sony Interactive Entertainment Inc.
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "PlatformCookieJar.h"

namespace WebCore {

class Cookie;
class NetworkStorageSession;
class URL;

class CookieJarCurl {
public:
    virtual String cookiesForDOM(const NetworkStorageSession&, const URL& firstParty, const URL&) = 0;
    virtual void setCookiesFromDOM(const NetworkStorageSession&, const URL& firstParty, const URL&, const String&) = 0;
    virtual bool cookiesEnabled(const NetworkStorageSession&, const URL& firstParty, const URL&) = 0;
    virtual String cookieRequestHeaderFieldValue(const NetworkStorageSession&, const URL& firstParty, const URL&) = 0;
    virtual bool getRawCookies(const NetworkStorageSession&, const URL& firstParty, const URL&, Vector<Cookie>&) = 0;
    virtual void deleteCookie(const NetworkStorageSession&, const URL&, const String&) = 0;
    virtual void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& hostnames) = 0;
    virtual void deleteCookiesForHostnames(const NetworkStorageSession&, const Vector<String>& cookieHostNames) = 0;
    virtual void deleteAllCookies(const NetworkStorageSession&) = 0;
    virtual void deleteAllCookiesModifiedSince(const NetworkStorageSession&, std::chrono::system_clock::time_point) = 0;
};

class CookieJarCurlFileSystem : public CookieJarCurl {
    String cookiesForDOM(const NetworkStorageSession&, const URL& firstParty, const URL&) override;
    void setCookiesFromDOM(const NetworkStorageSession&, const URL& firstParty, const URL&, const String&) override;
    bool cookiesEnabled(const NetworkStorageSession&, const URL& firstParty, const URL&) override;
    String cookieRequestHeaderFieldValue(const NetworkStorageSession&, const URL& firstParty, const URL&) override;
    bool getRawCookies(const NetworkStorageSession&, const URL& firstParty, const URL&, Vector<Cookie>&) override;
    void deleteCookie(const NetworkStorageSession&, const URL&, const String&) override;
    void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& hostnames) override;
    void deleteCookiesForHostnames(const NetworkStorageSession&, const Vector<String>& cookieHostNames) override;
    void deleteAllCookies(const NetworkStorageSession&) override;
    void deleteAllCookiesModifiedSince(const NetworkStorageSession&, std::chrono::system_clock::time_point) override;
};

}
