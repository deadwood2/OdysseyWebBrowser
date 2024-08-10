/*
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CookieJarCurl.h"

namespace WebCore {

class CookieJarCurlDatabase : public CookieJarCurl {
    std::pair<String, bool> cookiesForDOM(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies) const override;
    void setCookiesFromDOM(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, const String&) const override;
    void setCookiesFromHTTPResponse(const NetworkStorageSession&, const URL&, const String&) const override;
    bool cookiesEnabled(const NetworkStorageSession&) const override;
    std::pair<String, bool> cookieRequestHeaderFieldValue(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies) const override;
    std::pair<String, bool> cookieRequestHeaderFieldValue(const NetworkStorageSession&, const CookieRequestHeaderFieldProxy&) const override;
    bool getRawCookies(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, Vector<Cookie>&) const override;
    void deleteCookie(const NetworkStorageSession&, const URL&, const String&) const override;
    void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& hostnames) const override;
    void deleteCookiesForHostnames(const NetworkStorageSession&, const Vector<String>& cookieHostNames) const override;
    void deleteAllCookies(const NetworkStorageSession&) const override;
    void deleteAllCookiesModifiedSince(const NetworkStorageSession&, WallTime) const override;
};

} // namespace WebCore
