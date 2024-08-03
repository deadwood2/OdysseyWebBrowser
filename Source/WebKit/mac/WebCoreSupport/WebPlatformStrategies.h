/*
 * Copyright (C) 2010, 2016 Apple Inc. All rights reserved.
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

#ifndef WebPlatformStrategies_h
#define WebPlatformStrategies_h

#include <WebCore/CookiesStrategy.h>
#include <WebCore/LoaderStrategy.h>
#include <WebCore/PasteboardStrategy.h>
#include <WebCore/PlatformStrategies.h>

struct PasteboardImage;
struct PasteboardWebContent;

class WebPlatformStrategies : public WebCore::PlatformStrategies, private WebCore::CookiesStrategy, private WebCore::PasteboardStrategy {
public:
    static void initializeIfNecessary();
    
private:
    WebPlatformStrategies();
    
    // WebCore::PlatformStrategies
    WebCore::CookiesStrategy* createCookiesStrategy() override;
    WebCore::LoaderStrategy* createLoaderStrategy() override;
    WebCore::PasteboardStrategy* createPasteboardStrategy() override;
    WebCore::BlobRegistry* createBlobRegistry() override;

    // WebCore::CookiesStrategy
    String cookiesForDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) override;
    void setCookiesFromDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, const String&) override;
    bool cookiesEnabled(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) override;
    String cookieRequestHeaderFieldValue(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) override;
    String cookieRequestHeaderFieldValue(WebCore::SessionID, const WebCore::URL& firstParty, const WebCore::URL&) override;
    bool getRawCookies(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, Vector<WebCore::Cookie>&) override;
    void deleteCookie(const WebCore::NetworkStorageSession&, const WebCore::URL&, const String&) override;
    void addCookie(const WebCore::NetworkStorageSession&, const WebCore::URL&, const WebCore::Cookie&) override;

    // WebCore::PasteboardStrategy
#if PLATFORM(IOS)
    void writeToPasteboard(const WebCore::PasteboardWebContent&) override;
    void writeToPasteboard(const WebCore::PasteboardImage&) override;
    void writeToPasteboard(const String& pasteboardType, const String&) override;
    int getPasteboardItemsCount() override;
    String readStringFromPasteboard(int index, const String& pasteboardType) override;
    RefPtr<WebCore::SharedBuffer> readBufferFromPasteboard(int index, const String& pasteboardType) override;
    WebCore::URL readURLFromPasteboard(int index, const String& pasteboardType) override;
    long changeCount() override;
#endif
    void getTypes(Vector<String>& types, const String& pasteboardName) override;
    RefPtr<WebCore::SharedBuffer> bufferForType(const String& pasteboardType, const String& pasteboardName) override;
    void getPathnamesForType(Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName) override;
    String stringForType(const String& pasteboardType, const String& pasteboardName) override;
    long changeCount(const String& pasteboardName) override;
    String uniqueName() override;
    WebCore::Color color(const String& pasteboardName) override;
    WebCore::URL url(const String& pasteboardName) override;

    long addTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName) override;
    long setTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName) override;
    long copy(const String& fromPasteboard, const String& toPasteboard) override;
    long setBufferForType(WebCore::SharedBuffer*, const String& pasteboardType, const String& pasteboardName) override;
    long setPathnamesForType(const Vector<String>&, const String& pasteboardType, const String& pasteboardName) override;
    long setStringForType(const String&, const String& pasteboardType, const String& pasteboardName) override;
};

#endif // WebPlatformStrategies_h
