/*
 * Copyright (C) 2004-2008, 2013-2014 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "config.h"
#include "AtomicString.h"

#include "AtomicStringTable.h"
#include "HashSet.h"
#include "IntegerToStringConversion.h"
#include "StringHash.h"
#include "Threading.h"
#include "WTFThreadData.h"
#include "dtoa.h"
#include <wtf/unicode/UTF8.h>

#if USE(WEB_THREAD)
#include "SpinLock.h"
#endif

namespace WTF {

using namespace Unicode;

static_assert(sizeof(AtomicString) == sizeof(String), "AtomicString and String must be same size!");

#if USE(WEB_THREAD)

class AtomicStringTableLocker : public SpinLockHolder {
    WTF_MAKE_NONCOPYABLE(AtomicStringTableLocker);

    static StaticSpinLock s_stringTableLock;
public:
    AtomicStringTableLocker()
        : SpinLockHolder(&s_stringTableLock)
    {
    }
};

StaticSpinLock AtomicStringTableLocker::s_stringTableLock;

#else

class AtomicStringTableLocker {
    WTF_MAKE_NONCOPYABLE(AtomicStringTableLocker);
public:
    AtomicStringTableLocker() { }
};

#endif // USE(WEB_THREAD)

static ALWAYS_INLINE HashSet<StringImpl*>& stringTable()
{
    return wtfThreadData().atomicStringTable()->table();
}

template<typename T, typename HashTranslator>
static inline Ref<StringImpl> addToStringTable(const T& value)
{
    AtomicStringTableLocker locker;

    HashSet<StringImpl*>::AddResult addResult = stringTable().add<HashTranslator>(value);

    // If the string is newly-translated, then we need to adopt it.
    // The boolean in the pair tells us if that is so.
    if (addResult.isNewEntry)
        return adoptRef(**addResult.iterator);
    return **addResult.iterator;
}

struct CStringTranslator {
    static unsigned hash(const LChar* c)
    {
        return StringHasher::computeHashAndMaskTop8Bits(c);
    }

    static inline bool equal(StringImpl* r, const LChar* s)
    {
        return WTF::equal(r, s);
    }

    static void translate(StringImpl*& location, const LChar* const& c, unsigned hash)
    {
        location = &StringImpl::create(c).leakRef();
        location->setHash(hash);
        location->setIsAtomic(true);
    }
};

RefPtr<StringImpl> AtomicString::add(const LChar* c)
{
    if (!c)
        return nullptr;
    if (!*c)
        return StringImpl::empty();

    return addToStringTable<const LChar*, CStringTranslator>(c);
}

template<typename CharacterType>
struct HashTranslatorCharBuffer {
    const CharacterType* s;
    unsigned length;
};

typedef HashTranslatorCharBuffer<UChar> UCharBuffer;
struct UCharBufferTranslator {
    static unsigned hash(const UCharBuffer& buf)
    {
        return StringHasher::computeHashAndMaskTop8Bits(buf.s, buf.length);
    }

    static bool equal(StringImpl* const& str, const UCharBuffer& buf)
    {
        return WTF::equal(str, buf.s, buf.length);
    }

    static void translate(StringImpl*& location, const UCharBuffer& buf, unsigned hash)
    {
        location = &StringImpl::create8BitIfPossible(buf.s, buf.length).leakRef();
        location->setHash(hash);
        location->setIsAtomic(true);
    }
};

template<typename CharacterType>
struct HashAndCharacters {
    unsigned hash;
    const CharacterType* characters;
    unsigned length;
};

template<typename CharacterType>
struct HashAndCharactersTranslator {
    static unsigned hash(const HashAndCharacters<CharacterType>& buffer)
    {
        ASSERT(buffer.hash == StringHasher::computeHashAndMaskTop8Bits(buffer.characters, buffer.length));
        return buffer.hash;
    }

    static bool equal(StringImpl* const& string, const HashAndCharacters<CharacterType>& buffer)
    {
        return WTF::equal(string, buffer.characters, buffer.length);
    }

    static void translate(StringImpl*& location, const HashAndCharacters<CharacterType>& buffer, unsigned hash)
    {
        location = &StringImpl::create(buffer.characters, buffer.length).leakRef();
        location->setHash(hash);
        location->setIsAtomic(true);
    }
};

struct HashAndUTF8Characters {
    unsigned hash;
    const char* characters;
    unsigned length;
    unsigned utf16Length;
};

struct HashAndUTF8CharactersTranslator {
    static unsigned hash(const HashAndUTF8Characters& buffer)
    {
        return buffer.hash;
    }

    static bool equal(StringImpl* const& string, const HashAndUTF8Characters& buffer)
    {
        if (buffer.utf16Length != string->length())
            return false;

        // If buffer contains only ASCII characters UTF-8 and UTF16 length are the same.
        if (buffer.utf16Length != buffer.length) {
            if (string->is8Bit())
                return equalLatin1WithUTF8(string->characters8(), buffer.characters, buffer.characters + buffer.length);

            return equalUTF16WithUTF8(string->characters16(), buffer.characters, buffer.characters + buffer.length);
        }

        if (string->is8Bit()) {
            const LChar* stringCharacters = string->characters8();

            for (unsigned i = 0; i < buffer.length; ++i) {
                ASSERT(isASCII(buffer.characters[i]));
                if (stringCharacters[i] != buffer.characters[i])
                    return false;
            }

            return true;
        }

        const UChar* stringCharacters = string->characters16();

        for (unsigned i = 0; i < buffer.length; ++i) {
            ASSERT(isASCII(buffer.characters[i]));
            if (stringCharacters[i] != buffer.characters[i])
                return false;
        }

        return true;
    }

    static void translate(StringImpl*& location, const HashAndUTF8Characters& buffer, unsigned hash)
    {
        UChar* target;
        RefPtr<StringImpl> newString = StringImpl::createUninitialized(buffer.utf16Length, target);

        bool isAllASCII;
        const char* source = buffer.characters;
        if (convertUTF8ToUTF16(&source, source + buffer.length, &target, target + buffer.utf16Length, &isAllASCII) != conversionOK)
            ASSERT_NOT_REACHED();

        if (isAllASCII)
            newString = StringImpl::create(buffer.characters, buffer.length);

        location = newString.release().leakRef();
        location->setHash(hash);
        location->setIsAtomic(true);
    }
};

RefPtr<StringImpl> AtomicString::add(const UChar* s, unsigned length)
{
    if (!s)
        return nullptr;

    if (!length)
        return StringImpl::empty();
    
    UCharBuffer buffer = { s, length };
    return addToStringTable<UCharBuffer, UCharBufferTranslator>(buffer);
}

Ref<StringImpl> AtomicString::add(const UChar* s, unsigned length, unsigned existingHash)
{
    ASSERT(s);
    ASSERT(existingHash);

    if (!length)
        return *StringImpl::empty();

    HashAndCharacters<UChar> buffer = { existingHash, s, length };
    return addToStringTable<HashAndCharacters<UChar>, HashAndCharactersTranslator<UChar>>(buffer);
}

RefPtr<StringImpl> AtomicString::add(const UChar* s)
{
    if (!s)
        return nullptr;

    unsigned length = 0;
    while (s[length] != UChar(0))
        ++length;

    if (!length)
        return StringImpl::empty();

    UCharBuffer buffer = { s, length };
    return addToStringTable<UCharBuffer, UCharBufferTranslator>(buffer);
}

struct SubstringLocation {
    StringImpl* baseString;
    unsigned start;
    unsigned length;
};

struct SubstringTranslator {
    static void translate(StringImpl*& location, const SubstringLocation& buffer, unsigned hash)
    {
        location = &StringImpl::createSubstringSharingImpl(buffer.baseString, buffer.start, buffer.length).leakRef();
        location->setHash(hash);
        location->setIsAtomic(true);
    }
};

struct SubstringTranslator8 : SubstringTranslator {
    static unsigned hash(const SubstringLocation& buffer)
    {
        return StringHasher::computeHashAndMaskTop8Bits(buffer.baseString->characters8() + buffer.start, buffer.length);
    }

    static bool equal(StringImpl* const& string, const SubstringLocation& buffer)
    {
        return WTF::equal(string, buffer.baseString->characters8() + buffer.start, buffer.length);
    }
};

struct SubstringTranslator16 : SubstringTranslator {
    static unsigned hash(const SubstringLocation& buffer)
    {
        return StringHasher::computeHashAndMaskTop8Bits(buffer.baseString->characters16() + buffer.start, buffer.length);
    }

    static bool equal(StringImpl* const& string, const SubstringLocation& buffer)
    {
        return WTF::equal(string, buffer.baseString->characters16() + buffer.start, buffer.length);
    }
};

RefPtr<StringImpl> AtomicString::add(StringImpl* baseString, unsigned start, unsigned length)
{
    if (!baseString)
        return nullptr;

    if (!length || start >= baseString->length())
        return StringImpl::empty();

    unsigned maxLength = baseString->length() - start;
    if (length >= maxLength) {
        if (!start)
            return add(baseString);
        length = maxLength;
    }

    SubstringLocation buffer = { baseString, start, length };
    if (baseString->is8Bit())
        return addToStringTable<SubstringLocation, SubstringTranslator8>(buffer);
    return addToStringTable<SubstringLocation, SubstringTranslator16>(buffer);
}
    
typedef HashTranslatorCharBuffer<LChar> LCharBuffer;
struct LCharBufferTranslator {
    static unsigned hash(const LCharBuffer& buf)
    {
        return StringHasher::computeHashAndMaskTop8Bits(buf.s, buf.length);
    }

    static bool equal(StringImpl* const& str, const LCharBuffer& buf)
    {
        return WTF::equal(str, buf.s, buf.length);
    }

    static void translate(StringImpl*& location, const LCharBuffer& buf, unsigned hash)
    {
        location = &StringImpl::create(buf.s, buf.length).leakRef();
        location->setHash(hash);
        location->setIsAtomic(true);
    }
};

typedef HashTranslatorCharBuffer<char> CharBuffer;
struct CharBufferFromLiteralDataTranslator {
    static unsigned hash(const CharBuffer& buf)
    {
        return StringHasher::computeHashAndMaskTop8Bits(reinterpret_cast<const LChar*>(buf.s), buf.length);
    }

    static bool equal(StringImpl* const& str, const CharBuffer& buf)
    {
        return WTF::equal(str, buf.s, buf.length);
    }

    static void translate(StringImpl*& location, const CharBuffer& buf, unsigned hash)
    {
        location = &StringImpl::createFromLiteral(buf.s, buf.length).leakRef();
        location->setHash(hash);
        location->setIsAtomic(true);
    }
};

RefPtr<StringImpl> AtomicString::add(const LChar* s, unsigned length)
{
    if (!s)
        return nullptr;

    if (!length)
        return StringImpl::empty();

    LCharBuffer buffer = { s, length };
    return addToStringTable<LCharBuffer, LCharBufferTranslator>(buffer);
}

Ref<StringImpl> AtomicString::addFromLiteralData(const char* characters, unsigned length)
{
    ASSERT(characters);
    ASSERT(length);

    CharBuffer buffer = { characters, length };
    return addToStringTable<CharBuffer, CharBufferFromLiteralDataTranslator>(buffer);
}

Ref<StringImpl> AtomicString::addSlowCase(StringImpl& string)
{
    if (!string.length())
        return *StringImpl::empty();

    if (string.isSymbol()) {
        if (string.is8Bit())
            return *add(string.characters8(), string.length());
        return *add(string.characters16(), string.length());
    }

    ASSERT_WITH_MESSAGE(!string.isAtomic(), "AtomicString should not hit the slow case if the string is already atomic.");

    AtomicStringTableLocker locker;
    auto addResult = stringTable().add(&string);

    if (addResult.isNewEntry) {
        ASSERT(*addResult.iterator == &string);
        string.setIsAtomic(true);
    }

    return **addResult.iterator;
}

Ref<StringImpl> AtomicString::addSlowCase(AtomicStringTable& stringTable, StringImpl& string)
{
    if (!string.length())
        return *StringImpl::empty();

    if (string.isSymbol()) {
        if (string.is8Bit())
            return *add(string.characters8(), string.length());
        return *add(string.characters16(), string.length());
    }

    ASSERT_WITH_MESSAGE(!string.isAtomic(), "AtomicString should not hit the slow case if the string is already atomic.");

    AtomicStringTableLocker locker;
    auto addResult = stringTable.table().add(&string);

    if (addResult.isNewEntry) {
        ASSERT(*addResult.iterator == &string);
        string.setIsAtomic(true);
    }

    return **addResult.iterator;
}

void AtomicString::remove(StringImpl* string)
{
    ASSERT(string->isAtomic());
    AtomicStringTableLocker locker;
    HashSet<StringImpl*>& atomicStringTable = stringTable();
    HashSet<StringImpl*>::iterator iterator = atomicStringTable.find(string);
    ASSERT_WITH_MESSAGE(iterator != atomicStringTable.end(), "The string being removed is atomic in the string table of an other thread!");
    atomicStringTable.remove(iterator);
}

AtomicString AtomicString::lower() const
{
    // Note: This is a hot function in the Dromaeo benchmark.
    StringImpl* impl = this->impl();
    if (UNLIKELY(!impl))
        return AtomicString();

    RefPtr<StringImpl> lowercasedString = impl->lower();
    if (LIKELY(lowercasedString == impl))
        return *this;

    AtomicString result;
    result.m_string = addSlowCase(*lowercasedString);
    return result;
}

AtomicString AtomicString::convertToASCIILowercase() const
{
    StringImpl* impl = this->impl();
    if (UNLIKELY(!impl))
        return AtomicString();

    // Convert short strings without allocating a new StringImpl, since
    // there's a good chance these strings are already in the atomic
    // string table and so no memory allocation will be required.
    unsigned length;
    const unsigned localBufferSize = 100;
    if (impl->is8Bit() && (length = impl->length()) <= localBufferSize) {
        const LChar* characters = impl->characters8();
        unsigned failingIndex;
        for (unsigned i = 0; i < length; ++i) {
            if (UNLIKELY(isASCIIUpper(characters[i]))) {
                failingIndex = i;
                goto SlowPath;
            }
        }
        return *this;
SlowPath:
        LChar localBuffer[localBufferSize];
        for (unsigned i = 0; i < failingIndex; ++i)
            localBuffer[i] = characters[i];
        for (unsigned i = failingIndex; i < length; ++i)
            localBuffer[i] = toASCIILower(characters[i]);
        return AtomicString(localBuffer, length);
    }

    RefPtr<StringImpl> convertedString = impl->convertToASCIILowercase();
    if (LIKELY(convertedString == impl))
        return *this;

    AtomicString result;
    result.m_string = addSlowCase(*convertedString);
    return result;
}

AtomicStringImpl* AtomicString::findSlowCase(StringImpl& string)
{
    ASSERT_WITH_MESSAGE(!string.isAtomic(), "AtomicStringImpls should return from the fast case.");

    if (!string.length())
        return static_cast<AtomicStringImpl*>(StringImpl::empty());

    if (string.isSymbol()) {
        if (string.is8Bit())
            return findInternal(string.characters8(), string.length());
        return findInternal(string.characters16(), string.length());
    }

    AtomicStringTableLocker locker;
    HashSet<StringImpl*>& atomicStringTable = stringTable();
    auto iterator = atomicStringTable.find(&string);
    if (iterator != atomicStringTable.end())
        return static_cast<AtomicStringImpl*>(*iterator);
    return nullptr;
}

AtomicString AtomicString::fromUTF8Internal(const char* charactersStart, const char* charactersEnd)
{
    HashAndUTF8Characters buffer;
    buffer.characters = charactersStart;
    buffer.hash = calculateStringHashAndLengthFromUTF8MaskingTop8Bits(charactersStart, charactersEnd, buffer.length, buffer.utf16Length);

    if (!buffer.hash)
        return nullAtom;

    AtomicString atomicString;
    atomicString.m_string = addToStringTable<HashAndUTF8Characters, HashAndUTF8CharactersTranslator>(buffer);
    return atomicString;
}

AtomicString AtomicString::number(int number)
{
    return numberToStringSigned<AtomicString>(number);
}

AtomicString AtomicString::number(unsigned number)
{
    return numberToStringUnsigned<AtomicString>(number);
}

AtomicString AtomicString::number(double number)
{
    NumberToStringBuffer buffer;
    return String(numberToFixedPrecisionString(number, 6, buffer, true));
}

AtomicStringImpl* AtomicString::findInternal(const LChar* characters, unsigned length)
{
    AtomicStringTableLocker locker;
    auto& table = stringTable();

    LCharBuffer buffer = { characters, length };
    auto iterator = table.find<LCharBufferTranslator>(buffer);
    if (iterator != table.end())
        return static_cast<AtomicStringImpl*>(*iterator);
    return nullptr;
}

AtomicStringImpl* AtomicString::findInternal(const UChar* characters, unsigned length)
{
    AtomicStringTableLocker locker;
    auto& table = stringTable();

    UCharBuffer buffer = { characters, length };
    auto iterator = table.find<UCharBufferTranslator>(buffer);
    if (iterator != table.end())
        return static_cast<AtomicStringImpl*>(*iterator);
    return nullptr;
}

#if !ASSERT_DISABLED
bool AtomicString::isInAtomicStringTable(StringImpl* string)
{
    AtomicStringTableLocker locker;
    return stringTable().contains(string);
}
#endif

#ifndef NDEBUG
void AtomicString::show() const
{
    m_string.show();
}
#endif

} // namespace WTF
