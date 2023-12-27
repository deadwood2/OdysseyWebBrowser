/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (c) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#import "config.h"
#import "FontPlatformData.h"

#import "CoreTextSPI.h"
#import "SharedBuffer.h"
#import "WebCoreSystemInterface.h"
#import <wtf/text/WTFString.h>

#if !PLATFORM(IOS)
#import <AppKit/NSFont.h>
#else
#import "CoreGraphicsSPI.h"
#import <CoreText/CoreText.h>
#endif

namespace WebCore {

// These CoreText Text Spacing feature selectors are not defined in CoreText.
enum TextSpacingCTFeatureSelector { TextSpacingProportional, TextSpacingFullWidth, TextSpacingHalfWidth, TextSpacingThirdWidth, TextSpacingQuarterWidth };

FontPlatformData::FontPlatformData(CTFontRef font, float size, bool syntheticBold, bool syntheticOblique, FontOrientation orientation, FontWidthVariant widthVariant)
    : FontPlatformData(adoptCF(CTFontCopyGraphicsFont(font, NULL)).get(), size, syntheticBold, syntheticOblique, orientation, widthVariant)
{
    ASSERT_ARG(font, font);
    m_font = font;
    CFRetain(m_font);
    m_isColorBitmapFont = CTFontGetSymbolicTraits(font) & kCTFontTraitColorGlyphs;
    m_isCompositeFontReference = CTFontGetSymbolicTraits(font) & kCTFontCompositeTrait;
}

FontPlatformData::~FontPlatformData()
{
    if (isValidCTFontRef(m_font))
        CFRelease(m_font);
}

void FontPlatformData::platformDataInit(const FontPlatformData& f)
{
    m_font = isValidCTFontRef(f.m_font) ? static_cast<CTFontRef>(const_cast<void *>(CFRetain(f.m_font))) : f.m_font;

#if PLATFORM(IOS)
    m_isEmoji = f.m_isEmoji;
#endif
    m_cgFont = f.m_cgFont;
    m_ctFont = f.m_ctFont;
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& f)
{
    m_cgFont = f.m_cgFont;
#if PLATFORM(IOS)
    m_isEmoji = f.m_isEmoji;
#endif
    if (isValidCTFontRef(m_font) && isValidCTFontRef(f.m_font) && CFEqual(m_font, f.m_font))
        return *this;
    if (isValidCTFontRef(f.m_font))
        CFRetain(f.m_font);
    if (isValidCTFontRef(m_font))
        CFRelease(m_font);
    m_font = f.m_font;
    m_ctFont = f.m_ctFont;

    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    bool result = false;
    if (m_font || other.m_font) {
#if PLATFORM(IOS)
        result = isValidCTFontRef(m_font) && isValidCTFontRef(other.m_font) && CFEqual(m_font, other.m_font);
#if !ASSERT_DISABLED
        if (result)
            ASSERT(m_isEmoji == other.m_isEmoji);
#endif
#else
        result = m_font == other.m_font;
#endif // PLATFORM(IOS)
        return result;
    }
#if PLATFORM(IOS) && !ASSERT_DISABLED
    if (m_cgFont == other.m_cgFont)
        ASSERT(m_isEmoji == other.m_isEmoji);
#endif
    return m_cgFont == other.m_cgFont;
}

void FontPlatformData::setFont(CTFontRef font)
{
    ASSERT_ARG(font, font);
    ASSERT(m_font != reinterpret_cast<CTFontRef>(-1));

    if (m_font == font)
        return;

    CFRetain(font);
    if (m_font)
        CFRelease(m_font);
    m_font = font;
    m_size = CTFontGetSize(font);
    m_cgFont = adoptCF(CTFontCopyGraphicsFont(font, nullptr));

    CTFontSymbolicTraits traits = CTFontGetSymbolicTraits(m_font);
    m_isColorBitmapFont = traits & kCTFontTraitColorGlyphs;
    m_isCompositeFontReference = traits & kCTFontCompositeTrait;
    
    m_ctFont = nullptr;
}

bool FontPlatformData::roundsGlyphAdvances() const
{
#if USE(APPKIT)
    return [(NSFont *)m_font renderingMode] == NSFontAntialiasedIntegerAdvancementsRenderingMode;
#else
    return false;
#endif
}


bool FontPlatformData::allowsLigatures() const
{
    if (!m_font)
        return false;

    RetainPtr<CFCharacterSetRef> characterSet = adoptCF(CTFontCopyCharacterSet(ctFont()));
    return !(characterSet.get() && CFCharacterSetIsCharacterMember(characterSet.get(), 'a'));
}

inline int mapFontWidthVariantToCTFeatureSelector(FontWidthVariant variant)
{
    switch(variant) {
    case RegularWidth:
        return TextSpacingProportional;

    case HalfWidth:
        return TextSpacingHalfWidth;

    case ThirdWidth:
        return TextSpacingThirdWidth;

    case QuarterWidth:
        return TextSpacingQuarterWidth;
    }

    ASSERT_NOT_REACHED();
    return TextSpacingProportional;
}

static CFDictionaryRef createFeatureSettingDictionary(int featureTypeIdentifier, int featureSelectorIdentifier)
{
    RetainPtr<CFNumberRef> featureTypeIdentifierNumber = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureTypeIdentifier));
    RetainPtr<CFNumberRef> featureSelectorIdentifierNumber = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureSelectorIdentifier));

    const void* settingKeys[] = { kCTFontFeatureTypeIdentifierKey, kCTFontFeatureSelectorIdentifierKey };
    const void* settingValues[] = { featureTypeIdentifierNumber.get(), featureSelectorIdentifierNumber.get() };

    return CFDictionaryCreate(kCFAllocatorDefault, settingKeys, settingValues, WTF_ARRAY_LENGTH(settingKeys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

static CTFontDescriptorRef cascadeToLastResortFontDescriptor()
{
    static CTFontDescriptorRef descriptor;
    if (descriptor)
        return descriptor;

    RetainPtr<CTFontDescriptorRef> lastResort = adoptCF(CTFontDescriptorCreateWithNameAndSize(CFSTR("LastResort"), 0));

    const void* descriptors[] = { lastResort.get() };
    RetainPtr<CFArrayRef> array = adoptCF(CFArrayCreate(kCFAllocatorDefault, descriptors, WTF_ARRAY_LENGTH(descriptors), &kCFTypeArrayCallBacks));

    const void* keys[] = { kCTFontCascadeListAttribute };
    const void* values[] = { array.get() };
    RetainPtr<CFDictionaryRef> attributes = adoptCF(CFDictionaryCreate(kCFAllocatorDefault, keys, values, WTF_ARRAY_LENGTH(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    descriptor = CTFontDescriptorCreateWithAttributes(attributes.get());

    return descriptor;
}

static CTFontDescriptorRef cascadeToLastResortAndDisableSwashesFontDescriptor()
{
    static CTFontDescriptorRef descriptor;
    if (descriptor)
        return descriptor;

    RetainPtr<CFDictionaryRef> lineInitialSwashesOffSetting = adoptCF(createFeatureSettingDictionary(kSmartSwashType, kLineInitialSwashesOffSelector));
    RetainPtr<CFDictionaryRef> lineFinalSwashesOffSetting = adoptCF(createFeatureSettingDictionary(kSmartSwashType, kLineFinalSwashesOffSelector));

    const void* settingDictionaries[] = { lineInitialSwashesOffSetting.get(), lineFinalSwashesOffSetting.get() };
    RetainPtr<CFArrayRef> featureSettings = adoptCF(CFArrayCreate(kCFAllocatorDefault, settingDictionaries, WTF_ARRAY_LENGTH(settingDictionaries), &kCFTypeArrayCallBacks));

    const void* keys[] = { kCTFontFeatureSettingsAttribute };
    const void* values[] = { featureSettings.get() };
    RetainPtr<CFDictionaryRef> attributes = adoptCF(CFDictionaryCreate(kCFAllocatorDefault, keys, values, WTF_ARRAY_LENGTH(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    descriptor = CTFontDescriptorCreateCopyWithAttributes(cascadeToLastResortFontDescriptor(), attributes.get());

    return descriptor;
}

CGFloat FontPlatformData::ctFontSize() const
{
#if PLATFORM(IOS)
    // Apple Color Emoji size is adjusted (and then re-adjusted by Core Text) and capped.
    return !m_isEmoji ? m_size : m_size <= 15 ? 4 * (m_size + 2) / static_cast<CGFloat>(5) : 16;
#else
    return m_size;
#endif
}

CTFontRef FontPlatformData::ctFont() const
{
    if (m_ctFont)
        return m_ctFont.get();

    ASSERT(m_cgFont.get());
    m_ctFont = m_font;
    if (m_ctFont) {
        CTFontDescriptorRef fontDescriptor;
        RetainPtr<CFStringRef> postScriptName = adoptCF(CTFontCopyPostScriptName(m_ctFont.get()));
        // Hoefler Text Italic has line-initial and -final swashes enabled by default, so disable them.
        if (CFEqual(postScriptName.get(), CFSTR("HoeflerText-Italic")) || CFEqual(postScriptName.get(), CFSTR("HoeflerText-BlackItalic")))
            fontDescriptor = cascadeToLastResortAndDisableSwashesFontDescriptor();
        else
            fontDescriptor = cascadeToLastResortFontDescriptor();
        m_ctFont = adoptCF(CTFontCreateCopyWithAttributes(m_ctFont.get(), ctFontSize(), 0, fontDescriptor));
    } else
        m_ctFont = adoptCF(CTFontCreateWithGraphicsFont(m_cgFont.get(), ctFontSize(), 0, cascadeToLastResortFontDescriptor()));

    if (m_widthVariant != RegularWidth) {
        int featureTypeValue = kTextSpacingType;
        int featureSelectorValue = mapFontWidthVariantToCTFeatureSelector(m_widthVariant);
        RetainPtr<CTFontDescriptorRef> sourceDescriptor = adoptCF(CTFontCopyFontDescriptor(m_ctFont.get()));
        RetainPtr<CFNumberRef> featureType = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureTypeValue));
        RetainPtr<CFNumberRef> featureSelector = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureSelectorValue));
        RetainPtr<CTFontDescriptorRef> newDescriptor = adoptCF(CTFontDescriptorCreateCopyWithFeature(sourceDescriptor.get(), featureType.get(), featureSelector.get()));
        RetainPtr<CTFontRef> newFont = adoptCF(CTFontCreateWithFontDescriptor(newDescriptor.get(), m_size, 0));

        if (newFont)
            m_ctFont = newFont;
    }

    return m_ctFont.get();
}

RetainPtr<CFTypeRef> FontPlatformData::objectForEqualityCheck(CTFontRef ctFont)
{
#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED <= 80000) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED <= 101000)
    auto fontDescriptor = adoptCF(CTFontCopyFontDescriptor(ctFont));
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=138683 This is a shallow pointer compare for web fonts
    // because the URL contains the address of the font. This means we might erroneously get false negatives.
    RetainPtr<CFURLRef> url = adoptCF(static_cast<CFURLRef>(CTFontDescriptorCopyAttribute(fontDescriptor.get(), kCTFontReferenceURLAttribute)));
    ASSERT(CFGetTypeID(url.get()) == CFURLGetTypeID());
    return url;
#else
    return adoptCF(CTFontCopyGraphicsFont(ctFont, 0));
#endif
}

RetainPtr<CFTypeRef> FontPlatformData::objectForEqualityCheck() const
{
    return objectForEqualityCheck(ctFont());
}

PassRefPtr<SharedBuffer> FontPlatformData::openTypeTable(uint32_t table) const
{
    if (RetainPtr<CFDataRef> data = adoptCF(CGFontCopyTableForTag(cgFont(), table)))
        return SharedBuffer::wrapCFData(data.get());
    
    return nullptr;
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    RetainPtr<CFStringRef> cgFontDescription = adoptCF(CFCopyDescription(cgFont()));
    return String(cgFontDescription.get()) + " " + String::number(m_size)
            + (m_syntheticBold ? " synthetic bold" : "") + (m_syntheticOblique ? " synthetic oblique" : "") + (m_orientation ? " vertical orientation" : "");
}
#endif

} // namespace WebCore
