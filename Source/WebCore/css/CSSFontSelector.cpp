/*
 * Copyright (C) 2007, 2008, 2011, 2013 Apple Inc. All rights reserved.
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "CSSFontSelector.h"

#include "CachedFont.h"
#include "CSSFontFace.h"
#include "CSSFontFaceRule.h"
#include "CSSFontFaceSource.h"
#include "CSSFontFamily.h"
#include "CSSFontFeatureValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSPropertyNames.h"
#include "CSSSegmentedFontFace.h"
#include "CSSUnicodeRangeValue.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "CSSValuePool.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "Font.h"
#include "FontCache.h"
#include "FontFace.h"
#include "FontFaceSet.h"
#include "FontSelectorClient.h"
#include "FontVariantBuilder.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "SVGFontFaceElement.h"
#include "SVGNames.h"
#include "Settings.h"
#include "StyleProperties.h"
#include "StyleResolver.h"
#include "StyleRule.h"
#include "WebKitFontFamilyNames.h"
#include <wtf/Ref.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

static unsigned fontSelectorId;

CSSFontSelector::CSSFontSelector(Document& document)
    : m_document(&document)
    , m_cssFontFaceSet(CSSFontFaceSet::create())
    , m_beginLoadingTimer(*this, &CSSFontSelector::beginLoadTimerFired)
    , m_uniqueId(++fontSelectorId)
    , m_version(0)
{
    ASSERT(m_document);
    FontCache::singleton().addClient(*this);
    m_cssFontFaceSet->addClient(*this);
}

CSSFontSelector::~CSSFontSelector()
{
    clearDocument();
    m_cssFontFaceSet->removeClient(*this);
    FontCache::singleton().removeClient(*this);
}

FontFaceSet& CSSFontSelector::fontFaceSet()
{
    if (!m_fontFaceSet) {
        ASSERT(m_document);
        m_fontFaceSet = FontFaceSet::create(*m_document, m_cssFontFaceSet.get());
    }

    return *m_fontFaceSet;
}

bool CSSFontSelector::isEmpty() const
{
    return !m_cssFontFaceSet->faceCount();
}

void CSSFontSelector::buildStarted()
{
    m_buildIsUnderway = true;
    m_stagingArea.clear();
    m_cssFontFaceSet->purge();
    ++m_version;

    m_cssConnectionsPossiblyToRemove.clear();
    m_cssConnectionsEncounteredDuringBuild.clear();
    for (size_t i = 0; i < m_cssFontFaceSet->faceCount(); ++i) {
        CSSFontFace& face = m_cssFontFaceSet.get()[i];
        if (face.cssConnection())
            m_cssConnectionsPossiblyToRemove.add(&face);
    }
}

void CSSFontSelector::buildCompleted()
{
    if (!m_buildIsUnderway)
        return;

    m_buildIsUnderway = false;

    // Some font faces weren't re-added during the build process.
    for (auto& face : m_cssConnectionsPossiblyToRemove) {
        auto* connection = face->cssConnection();
        ASSERT(connection);
        if (!m_cssConnectionsEncounteredDuringBuild.contains(connection))
            m_cssFontFaceSet->remove(*face);
    }

    for (auto& item : m_stagingArea)
        addFontFaceRule(item.styleRuleFontFace, item.isInitiatingElementInUserAgentShadowTree);
    m_stagingArea.clear();
}

void CSSFontSelector::addFontFaceRule(StyleRuleFontFace& fontFaceRule, bool isInitiatingElementInUserAgentShadowTree)
{
    if (m_buildIsUnderway) {
        m_cssConnectionsEncounteredDuringBuild.add(&fontFaceRule);
        m_stagingArea.append({fontFaceRule, isInitiatingElementInUserAgentShadowTree});
        return;
    }

    const StyleProperties& style = fontFaceRule.properties();
    RefPtr<CSSValue> fontFamily = style.getPropertyCSSValue(CSSPropertyFontFamily);
    RefPtr<CSSValue> fontStyle = style.getPropertyCSSValue(CSSPropertyFontStyle);
    RefPtr<CSSValue> fontWeight = style.getPropertyCSSValue(CSSPropertyFontWeight);
    RefPtr<CSSValue> src = style.getPropertyCSSValue(CSSPropertySrc);
    RefPtr<CSSValue> unicodeRange = style.getPropertyCSSValue(CSSPropertyUnicodeRange);
    RefPtr<CSSValue> featureSettings = style.getPropertyCSSValue(CSSPropertyFontFeatureSettings);
    RefPtr<CSSValue> variantLigatures = style.getPropertyCSSValue(CSSPropertyFontVariantLigatures);
    RefPtr<CSSValue> variantPosition = style.getPropertyCSSValue(CSSPropertyFontVariantPosition);
    RefPtr<CSSValue> variantCaps = style.getPropertyCSSValue(CSSPropertyFontVariantCaps);
    RefPtr<CSSValue> variantNumeric = style.getPropertyCSSValue(CSSPropertyFontVariantNumeric);
    RefPtr<CSSValue> variantAlternates = style.getPropertyCSSValue(CSSPropertyFontVariantAlternates);
    RefPtr<CSSValue> variantEastAsian = style.getPropertyCSSValue(CSSPropertyFontVariantEastAsian);
    if (!is<CSSValueList>(fontFamily.get()) || !is<CSSValueList>(src.get()) || (unicodeRange && !is<CSSValueList>(*unicodeRange)))
        return;

    CSSValueList& familyList = downcast<CSSValueList>(*fontFamily);
    if (!familyList.length())
        return;

    if (!fontStyle)
        fontStyle = CSSValuePool::singleton().createIdentifierValue(CSSValueNormal).ptr();

    if (!fontWeight)
        fontWeight = CSSValuePool::singleton().createIdentifierValue(CSSValueNormal);

    CSSValueList* rangeList = downcast<CSSValueList>(unicodeRange.get());

    CSSValueList& srcList = downcast<CSSValueList>(*src);
    if (!srcList.length())
        return;

    m_creatingFont = true;
    Ref<CSSFontFace> fontFace = CSSFontFace::create(this, &fontFaceRule);

    if (!fontFace->setFamilies(*fontFamily))
        return;
    if (!fontFace->setStyle(*fontStyle))
        return;
    if (!fontFace->setWeight(*fontWeight))
        return;
    if (rangeList && !fontFace->setUnicodeRange(*rangeList))
        return;
    if (variantLigatures && !fontFace->setVariantLigatures(*variantLigatures))
        return;
    if (variantPosition && !fontFace->setVariantPosition(*variantPosition))
        return;
    if (variantCaps && !fontFace->setVariantCaps(*variantCaps))
        return;
    if (variantNumeric && !fontFace->setVariantNumeric(*variantNumeric))
        return;
    if (variantAlternates && !fontFace->setVariantAlternates(*variantAlternates))
        return;
    if (variantEastAsian && !fontFace->setVariantEastAsian(*variantEastAsian))
        return;
    if (featureSettings)
        fontFace->setFeatureSettings(*featureSettings);

    CSSFontFace::appendSources(fontFace, srcList, m_document, isInitiatingElementInUserAgentShadowTree);
    if (fontFace->allSourcesFailed())
        return;

    if (RefPtr<CSSFontFace> existingFace = m_cssFontFaceSet->lookupByCSSConnection(fontFaceRule)) {
        m_cssFontFaceSet->remove(*existingFace);
        if (auto* existingWrapper = existingFace->existingWrapper())
            existingWrapper->adopt(fontFace.get());
    }

    m_cssFontFaceSet->add(fontFace.get());
    m_creatingFont = false;
    ++m_version;
}

void CSSFontSelector::registerForInvalidationCallbacks(FontSelectorClient& client)
{
    m_clients.add(&client);
}

void CSSFontSelector::unregisterForInvalidationCallbacks(FontSelectorClient& client)
{
    m_clients.remove(&client);
}

void CSSFontSelector::dispatchInvalidationCallbacks()
{
    ++m_version;

    Vector<FontSelectorClient*> clients;
    copyToVector(m_clients, clients);
    for (size_t i = 0; i < clients.size(); ++i)
        clients[i]->fontsNeedUpdate(*this);
}

void CSSFontSelector::fontLoaded()
{
    dispatchInvalidationCallbacks();
}

void CSSFontSelector::fontModified()
{
    if (!m_creatingFont)
        dispatchInvalidationCallbacks();
}

void CSSFontSelector::fontCacheInvalidated()
{
    dispatchInvalidationCallbacks();
}

static const AtomicString& resolveGenericFamily(Document* document, const FontDescription& fontDescription, const AtomicString& familyName)
{
    if (!document || !document->frame())
        return familyName;

    const Settings& settings = document->frame()->settings();

    UScriptCode script = fontDescription.script();
    if (familyName == serifFamily)
        return settings.serifFontFamily(script);
    if (familyName == sansSerifFamily)
        return settings.sansSerifFontFamily(script);
    if (familyName == cursiveFamily)
        return settings.cursiveFontFamily(script);
    if (familyName == fantasyFamily)
        return settings.fantasyFontFamily(script);
    if (familyName == monospaceFamily)
        return settings.fixedFontFamily(script);
    if (familyName == pictographFamily)
        return settings.pictographFontFamily(script);
    if (familyName == standardFamily)
        return settings.standardFontFamily(script);

    return familyName;
}

FontRanges CSSFontSelector::fontRangesForFamily(const FontDescription& fontDescription, const AtomicString& familyName)
{
    ASSERT(!m_buildIsUnderway); // If this ASSERT() fires, it usually means you forgot a document.updateStyleIfNeeded() somewhere.

    // FIXME: The spec (and Firefox) says user specified generic families (sans-serif etc.) should be resolved before the @font-face lookup too.
    bool resolveGenericFamilyFirst = familyName == standardFamily;

    AtomicString familyForLookup = resolveGenericFamilyFirst ? resolveGenericFamily(m_document, fontDescription, familyName) : familyName;
    CSSSegmentedFontFace* face = m_cssFontFaceSet->getFontFace(fontDescription.traitsMask(), familyForLookup);
    if (!face) {
        if (!resolveGenericFamilyFirst)
            familyForLookup = resolveGenericFamily(m_document, fontDescription, familyName);
        return FontRanges(FontCache::singleton().fontForFamily(fontDescription, familyForLookup));
    }

    return face->fontRanges(fontDescription);
}

void CSSFontSelector::clearDocument()
{
    if (!m_document) {
        ASSERT(!m_beginLoadingTimer.isActive());
        ASSERT(m_fontsToBeginLoading.isEmpty());
        return;
    }

    m_beginLoadingTimer.stop();

    CachedResourceLoader& cachedResourceLoader = m_document->cachedResourceLoader();
    for (auto& fontHandle : m_fontsToBeginLoading) {
        // Balances incrementRequestCount() in beginLoadingFontSoon().
        cachedResourceLoader.decrementRequestCount(*fontHandle);
    }
    m_fontsToBeginLoading.clear();

    m_document = nullptr;

    m_cssFontFaceSet->clear();
    m_clients.clear();
}

void CSSFontSelector::beginLoadingFontSoon(CachedFont& font)
{
    if (!m_document)
        return;

    m_fontsToBeginLoading.append(&font);
    // Increment the request count now, in order to prevent didFinishLoad from being dispatched
    // after this font has been requested but before it began loading. Balanced by
    // decrementRequestCount() in beginLoadTimerFired() and in clearDocument().
    m_document->cachedResourceLoader().incrementRequestCount(font);
    m_beginLoadingTimer.startOneShot(0);
}

void CSSFontSelector::beginLoadTimerFired()
{
    Vector<CachedResourceHandle<CachedFont>> fontsToBeginLoading;
    fontsToBeginLoading.swap(m_fontsToBeginLoading);

    // CSSFontSelector could get deleted via beginLoadIfNeeded() or loadDone() unless protected.
    Ref<CSSFontSelector> protectedThis(*this);

    CachedResourceLoader& cachedResourceLoader = m_document->cachedResourceLoader();
    for (auto& fontHandle : fontsToBeginLoading) {
        fontHandle->beginLoadIfNeeded(cachedResourceLoader);
        // Balances incrementRequestCount() in beginLoadingFontSoon().
        cachedResourceLoader.decrementRequestCount(*fontHandle);
    }
    // Ensure that if the request count reaches zero, the frame loader will know about it.
    cachedResourceLoader.loadDone(nullptr);
    // New font loads may be triggered by layout after the document load is complete but before we have dispatched
    // didFinishLoading for the frame. Make sure the delegate is always dispatched by checking explicitly.
    if (m_document && m_document->frame())
        m_document->frame()->loader().checkLoadComplete();
}


size_t CSSFontSelector::fallbackFontCount()
{
    if (!m_document)
        return 0;

    if (Settings* settings = m_document->settings())
        return settings->fontFallbackPrefersPictographs() ? 1 : 0;

    return 0;
}

RefPtr<Font> CSSFontSelector::fallbackFontAt(const FontDescription& fontDescription, size_t index)
{
    ASSERT_UNUSED(index, !index);

    if (!m_document)
        return nullptr;

    Settings* settings = m_document->settings();
    if (!settings || !settings->fontFallbackPrefersPictographs())
        return nullptr;

    return FontCache::singleton().fontForFamily(fontDescription, settings->pictographFontFamily());
}

}
