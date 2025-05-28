/*
 * Copyright (C) 2020 Jacek Piszczek
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

#pragma once

#include "Image.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/URL.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class SelectionData {
    WTF_MAKE_FAST_ALLOCATED;
public:

    void setText(const String&);
    const String& text() const { return m_text; }
    bool hasText() const { return !m_text.isEmpty(); }
    void clearText() { m_text = emptyString(); }

    void setMarkup(const String& newMarkup) { m_markup = newMarkup; }
    const String& markup() const { return m_markup; }
    bool hasMarkup() const { return !m_markup.isEmpty(); }
    void clearMarkup() { m_markup = emptyString(); }

    void setURL(const URL&, const String&);
    const URL& url() const { return m_url; }
    const String& urlLabel() const;
    bool hasURL() const { return !m_url.isEmpty() && m_url.isValid(); }
    void clearURL() { m_url = URL(); }

    void setURIList(const String&);
    const String& uriList() const { return m_uriList; }
    const Vector<String>& filenames() const { return m_filenames; }
    bool hasURIList() const { return !m_uriList.isEmpty(); }
    bool hasFilenames() const { return !m_filenames.isEmpty(); }
    void clearURIList() { m_uriList = emptyString(); }

    void setImage(Image* newImage) { m_image = newImage; }
    Image* image() const { return m_image.get(); }
    bool hasImage() const { return m_image; }
    void clearImage() { m_image = nullptr; }

    void setUnknownTypeData(const String& type, const String& data) { m_unknownTypeData.set(type, data); }
    String unknownTypeData(const String& type) const { return m_unknownTypeData.get(type); }
    const HashMap<String, String>& unknownTypes() const { return m_unknownTypeData; }
    bool hasUnknownTypeData() const { return !m_unknownTypeData.isEmpty(); }

    void setCanSmartReplace(bool canSmartReplace) { m_canSmartReplace = canSmartReplace; }
    bool canSmartReplace() const { return m_canSmartReplace; }

    void clearAll();
    void clearAllExceptFilenames();

private:
    String m_text;
    String m_markup;
    URL m_url;
    String m_uriList;
    Vector<String> m_filenames;
    RefPtr<Image> m_image;
    HashMap<String, String> m_unknownTypeData;
    bool m_canSmartReplace { false };
};

} // namespace WebCore
