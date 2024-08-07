/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Intel Corporation
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FontCascade.h"

#if USE(CAIRO)

#include "FontCache.h"
#include "SurrogatePairAwareTextIterator.h"
#include <unicode/normlzr.h>

namespace WebCore {

bool FontCascade::canReturnFallbackFontsForComplexText()
{
    return false;
}

bool FontCascade::canExpandAroundIdeographsInComplexText()
{
    return false;
}

const Font* FontCascade::fontForCombiningCharacterSequence(const UChar* characters, size_t length) const
{
    UErrorCode error = U_ZERO_ERROR;
    Vector<UChar, 4> normalizedCharacters(length);
#if COMPILER(GCC_OR_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    int32_t normalizedLength = unorm_normalize(characters, length, UNORM_NFC, UNORM_UNICODE_3_2, normalizedCharacters.data(), length, &error);
#if COMPILER(GCC_OR_CLANG)
#pragma GCC diagnostic pop
#endif
    if (U_FAILURE(error))
        return nullptr;

    UChar32 character;
    unsigned clusterLength = 0;
    SurrogatePairAwareTextIterator iterator(normalizedCharacters.data(), 0, normalizedLength, normalizedLength);
    if (!iterator.consume(character, clusterLength))
        return nullptr;

    const Font* baseFont = glyphDataForCharacter(character, false, NormalVariant).font;
    if (baseFont && (static_cast<int32_t>(clusterLength) == normalizedLength || baseFont->canRenderCombiningCharacterSequence(characters, length)))
        return baseFont;

    for (unsigned i = 0; !fallbackRangesAt(i).isNull(); ++i) {
        const Font* fallbackFont = fallbackRangesAt(i).fontForCharacter(character);
        if (!fallbackFont || fallbackFont == baseFont)
            continue;

        if (fallbackFont->canRenderCombiningCharacterSequence(characters, length))
            return fallbackFont;
    }

    if (auto systemFallback = FontCache::singleton().systemFallbackForCharacters(m_fontDescription, baseFont, false, characters, length)) {
        if (systemFallback->canRenderCombiningCharacterSequence(characters, length))
            return systemFallback.get();
    }

    return baseFont;
}

} // namespace WebCore

#endif // USE(CAIRO)
