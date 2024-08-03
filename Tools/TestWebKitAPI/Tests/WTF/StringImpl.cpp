/*
 * Copyright (C) 2012, 2016 Apple Inc. All rights reserved.
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

#include "config.h"

#include <wtf/text/SymbolImpl.h>
#include <wtf/text/WTFString.h>

namespace TestWebKitAPI {

TEST(WTF, StringImplCreationFromLiteral)
{
    // Constructor using the template to determine the size.
    auto stringWithTemplate = StringImpl::createFromLiteral("Template Literal");
    ASSERT_EQ(strlen("Template Literal"), stringWithTemplate->length());
    ASSERT_TRUE(equal(stringWithTemplate.get(), "Template Literal"));
    ASSERT_TRUE(stringWithTemplate->is8Bit());

    // Constructor taking the size explicitely.
    const char* programmaticStringData = "Explicit Size Literal";
    auto programmaticString = StringImpl::createFromLiteral(programmaticStringData, strlen(programmaticStringData));
    ASSERT_EQ(strlen(programmaticStringData), programmaticString->length());
    ASSERT_TRUE(equal(programmaticString.get(), programmaticStringData));
    ASSERT_EQ(programmaticStringData, reinterpret_cast<const char*>(programmaticString->characters8()));
    ASSERT_TRUE(programmaticString->is8Bit());

    // Constructor without explicit size.
    const char* stringWithoutLengthLiteral = "No Size Literal";
    auto programmaticStringNoLength = StringImpl::createFromLiteral(stringWithoutLengthLiteral);
    ASSERT_EQ(strlen(stringWithoutLengthLiteral), programmaticStringNoLength->length());
    ASSERT_TRUE(equal(programmaticStringNoLength.get(), stringWithoutLengthLiteral));
    ASSERT_EQ(stringWithoutLengthLiteral, reinterpret_cast<const char*>(programmaticStringNoLength->characters8()));
    ASSERT_TRUE(programmaticStringNoLength->is8Bit());
}

TEST(WTF, StringImplReplaceWithLiteral)
{
    auto testStringImpl = StringImpl::createFromLiteral("1224");
    ASSERT_TRUE(testStringImpl->is8Bit());

    // Cases for 8Bit source.
    testStringImpl = testStringImpl->replace('2', "", 0);
    ASSERT_TRUE(equal(testStringImpl.get(), "14"));

    testStringImpl = StringImpl::createFromLiteral("1224");
    ASSERT_TRUE(testStringImpl->is8Bit());

    testStringImpl = testStringImpl->replace('3', "NotFound", 8);
    ASSERT_TRUE(equal(testStringImpl.get(), "1224"));

    testStringImpl = testStringImpl->replace('2', "3", 1);
    ASSERT_TRUE(equal(testStringImpl.get(), "1334"));

    testStringImpl = StringImpl::createFromLiteral("1224");
    ASSERT_TRUE(testStringImpl->is8Bit());
    testStringImpl = testStringImpl->replace('2', "555", 3);
    ASSERT_TRUE(equal(testStringImpl.get(), "15555554"));

    // Cases for 16Bit source.
    String testString = String::fromUTF8("résumé");
    ASSERT_FALSE(testString.impl()->is8Bit());

    testStringImpl = testString.impl()->replace('2', "NotFound", 8);
    ASSERT_TRUE(equal(testStringImpl.get(), String::fromUTF8("résumé").impl()));

    testStringImpl = testString.impl()->replace(UChar(0x00E9 /*U+00E9 is 'é'*/), "e", 1);
    ASSERT_TRUE(equal(testStringImpl.get(), "resume"));

    testString = String::fromUTF8("résumé");
    ASSERT_FALSE(testString.impl()->is8Bit());
    testStringImpl = testString.impl()->replace(UChar(0x00E9 /*U+00E9 is 'é'*/), "", 0);
    ASSERT_TRUE(equal(testStringImpl.get(), "rsum"));

    testString = String::fromUTF8("résumé");
    ASSERT_FALSE(testString.impl()->is8Bit());
    testStringImpl = testString.impl()->replace(UChar(0x00E9 /*U+00E9 is 'é'*/), "555", 3);
    ASSERT_TRUE(equal(testStringImpl.get(), "r555sum555"));
}

TEST(WTF, StringImplEqualIgnoringASCIICaseBasic)
{
    auto a = StringImpl::createFromLiteral("aBcDeFG");
    auto b = StringImpl::createFromLiteral("ABCDEFG");
    auto c = StringImpl::createFromLiteral("abcdefg");
    const char d[] = "aBcDeFG";
    auto empty = StringImpl::create(reinterpret_cast<const LChar*>(""));
    auto shorter = StringImpl::createFromLiteral("abcdef");
    auto different = StringImpl::createFromLiteral("abcrefg");

    // Identity.
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), a.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(b.ptr(), b.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(c.ptr(), c.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), d));
    ASSERT_TRUE(equalIgnoringASCIICase(b.ptr(), d));
    ASSERT_TRUE(equalIgnoringASCIICase(c.ptr(), d));

    // Transitivity.
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), b.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(b.ptr(), c.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), c.ptr()));

    // Negative cases.
    ASSERT_FALSE(equalIgnoringASCIICase(a.ptr(), empty.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(b.ptr(), empty.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(c.ptr(), empty.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(a.ptr(), shorter.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(b.ptr(), shorter.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(c.ptr(), shorter.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(a.ptr(), different.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(b.ptr(), different.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(c.ptr(), different.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(empty.ptr(), d));
    ASSERT_FALSE(equalIgnoringASCIICase(shorter.ptr(), d));
    ASSERT_FALSE(equalIgnoringASCIICase(different.ptr(), d));
}

TEST(WTF, StringImplEqualIgnoringASCIICaseWithNull)
{
    auto reference = StringImpl::createFromLiteral("aBcDeFG");
    StringImpl* nullStringImpl = nullptr;
    ASSERT_FALSE(equalIgnoringASCIICase(nullStringImpl, reference.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(reference.ptr(), nullStringImpl));
    ASSERT_TRUE(equalIgnoringASCIICase(nullStringImpl, nullStringImpl));
}

TEST(WTF, StringImplEqualIgnoringASCIICaseWithEmpty)
{
    auto a = StringImpl::create(reinterpret_cast<const LChar*>(""));
    auto b = StringImpl::create(reinterpret_cast<const LChar*>(""));
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), b.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(b.ptr(), a.ptr()));
}

static Ref<StringImpl> stringFromUTF8(const char* characters)
{
    return String::fromUTF8(characters).releaseImpl().releaseNonNull();
}

TEST(WTF, StringImplEqualIgnoringASCIICaseWithLatin1Characters)
{
    auto a = stringFromUTF8("aBcéeFG");
    auto b = stringFromUTF8("ABCÉEFG");
    auto c = stringFromUTF8("ABCéEFG");
    auto d = stringFromUTF8("abcéefg");
    const char e[] = "aBcéeFG";

    // Identity.
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), a.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(b.ptr(), b.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(c.ptr(), c.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(d.ptr(), d.ptr()));

    // All combination.
    ASSERT_FALSE(equalIgnoringASCIICase(a.ptr(), b.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), c.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(a.ptr(), d.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(b.ptr(), c.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(b.ptr(), d.ptr()));
    ASSERT_TRUE(equalIgnoringASCIICase(c.ptr(), d.ptr()));
    ASSERT_FALSE(equalIgnoringASCIICase(a.ptr(), e));
    ASSERT_FALSE(equalIgnoringASCIICase(b.ptr(), e));
    ASSERT_FALSE(equalIgnoringASCIICase(c.ptr(), e));
    ASSERT_FALSE(equalIgnoringASCIICase(d.ptr(), e));
}

TEST(WTF, StringImplFindIgnoringASCIICaseBasic)
{
    auto referenceA = stringFromUTF8("aBcéeFG");
    auto referenceB = stringFromUTF8("ABCÉEFG");

    // Search the exact string.
    EXPECT_EQ(static_cast<size_t>(0), referenceA->findIgnoringASCIICase(referenceA.ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceB->findIgnoringASCIICase(referenceB.ptr()));

    // A and B are distinct by the non-ascii character é/É.
    EXPECT_EQ(static_cast<size_t>(notFound), referenceA->findIgnoringASCIICase(referenceB.ptr()));
    EXPECT_EQ(static_cast<size_t>(notFound), referenceB->findIgnoringASCIICase(referenceA.ptr()));

    // Find the prefix.
    EXPECT_EQ(static_cast<size_t>(0), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("a").ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceA->findIgnoringASCIICase(stringFromUTF8("abcé").ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("A").ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceA->findIgnoringASCIICase(stringFromUTF8("ABCé").ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("a").ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceB->findIgnoringASCIICase(stringFromUTF8("abcÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("A").ptr()));
    EXPECT_EQ(static_cast<size_t>(0), referenceB->findIgnoringASCIICase(stringFromUTF8("ABCÉ").ptr()));

    // Not a prefix.
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("x").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("accé").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("abcÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("X").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("ABDé").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("ABCÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("y").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("accÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("abcé").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("Y").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("ABdÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("ABCé").ptr()));

    // Find the infix.
    EXPECT_EQ(static_cast<size_t>(2), referenceA->findIgnoringASCIICase(stringFromUTF8("cée").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceA->findIgnoringASCIICase(stringFromUTF8("ée").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceA->findIgnoringASCIICase(stringFromUTF8("cé").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceA->findIgnoringASCIICase(stringFromUTF8("c").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceA->findIgnoringASCIICase(stringFromUTF8("é").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceA->findIgnoringASCIICase(stringFromUTF8("Cée").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceA->findIgnoringASCIICase(stringFromUTF8("éE").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceA->findIgnoringASCIICase(stringFromUTF8("Cé").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceA->findIgnoringASCIICase(stringFromUTF8("C").ptr()));

    EXPECT_EQ(static_cast<size_t>(2), referenceB->findIgnoringASCIICase(stringFromUTF8("cÉe").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceB->findIgnoringASCIICase(stringFromUTF8("Ée").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceB->findIgnoringASCIICase(stringFromUTF8("cÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceB->findIgnoringASCIICase(stringFromUTF8("c").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceB->findIgnoringASCIICase(stringFromUTF8("É").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceB->findIgnoringASCIICase(stringFromUTF8("CÉe").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceB->findIgnoringASCIICase(stringFromUTF8("ÉE").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceB->findIgnoringASCIICase(stringFromUTF8("CÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(2), referenceB->findIgnoringASCIICase(stringFromUTF8("C").ptr()));

    // Not an infix.
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("céd").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("Ée").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("bé").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("x").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("É").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("CÉe").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("éd").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("CÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("Y").ptr()));

    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("cée").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("Éc").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("cé").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("W").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("é").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("bÉe").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("éE").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("BÉ").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("z").ptr()));

    // Find the suffix.
    EXPECT_EQ(static_cast<size_t>(6), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("g").ptr()));
    EXPECT_EQ(static_cast<size_t>(4), referenceA->findIgnoringASCIICase(stringFromUTF8("efg").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceA->findIgnoringASCIICase(stringFromUTF8("éefg").ptr()));
    EXPECT_EQ(static_cast<size_t>(6), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("G").ptr()));
    EXPECT_EQ(static_cast<size_t>(4), referenceA->findIgnoringASCIICase(stringFromUTF8("EFG").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceA->findIgnoringASCIICase(stringFromUTF8("éEFG").ptr()));

    EXPECT_EQ(static_cast<size_t>(6), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("g").ptr()));
    EXPECT_EQ(static_cast<size_t>(4), referenceB->findIgnoringASCIICase(stringFromUTF8("efg").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceB->findIgnoringASCIICase(stringFromUTF8("Éefg").ptr()));
    EXPECT_EQ(static_cast<size_t>(6), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("G").ptr()));
    EXPECT_EQ(static_cast<size_t>(4), referenceB->findIgnoringASCIICase(stringFromUTF8("EFG").ptr()));
    EXPECT_EQ(static_cast<size_t>(3), referenceB->findIgnoringASCIICase(stringFromUTF8("ÉEFG").ptr()));

    // Not a suffix.
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("X").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("edg").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("Éefg").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(StringImpl::createFromLiteral("w").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("dFG").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceA->findIgnoringASCIICase(stringFromUTF8("ÉEFG").ptr()));

    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("Z").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("ffg").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("éefg").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(StringImpl::createFromLiteral("r").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("EgG").ptr()));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), referenceB->findIgnoringASCIICase(stringFromUTF8("éEFG").ptr()));
}

TEST(WTF, StringImplFindIgnoringASCIICaseWithValidOffset)
{
    auto reference = stringFromUTF8("ABCÉEFGaBcéeFG");
    EXPECT_EQ(static_cast<size_t>(0), reference->findIgnoringASCIICase(stringFromUTF8("ABC").ptr(), 0));
    EXPECT_EQ(static_cast<size_t>(7), reference->findIgnoringASCIICase(stringFromUTF8("ABC").ptr(), 1));
    EXPECT_EQ(static_cast<size_t>(0), reference->findIgnoringASCIICase(stringFromUTF8("ABCÉ").ptr(), 0));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(stringFromUTF8("ABCÉ").ptr(), 1));
    EXPECT_EQ(static_cast<size_t>(7), reference->findIgnoringASCIICase(stringFromUTF8("ABCé").ptr(), 0));
    EXPECT_EQ(static_cast<size_t>(7), reference->findIgnoringASCIICase(stringFromUTF8("ABCé").ptr(), 1));
}

TEST(WTF, StringImplFindIgnoringASCIICaseWithInvalidOffset)
{
    auto reference = stringFromUTF8("ABCÉEFGaBcéeFG");
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(stringFromUTF8("ABC").ptr(), 15));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(stringFromUTF8("ABC").ptr(), 16));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(stringFromUTF8("ABCÉ").ptr(), 17));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(stringFromUTF8("ABCÉ").ptr(), 42));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(stringFromUTF8("ABCÉ").ptr(), std::numeric_limits<unsigned>::max()));
}

TEST(WTF, StringImplFindIgnoringASCIICaseOnNull)
{
    auto reference = stringFromUTF8("ABCÉEFG");
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(nullptr));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(nullptr, 0));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(nullptr, 3));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(nullptr, 7));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(nullptr, 8));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(nullptr, 42));
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(nullptr, std::numeric_limits<unsigned>::max()));
}

TEST(WTF, StringImplFindIgnoringASCIICaseOnEmpty)
{
    auto reference = stringFromUTF8("ABCÉEFG");
    auto empty = StringImpl::create(reinterpret_cast<const LChar*>(""));
    EXPECT_EQ(static_cast<size_t>(0), reference->findIgnoringASCIICase(empty.ptr()));
    EXPECT_EQ(static_cast<size_t>(0), reference->findIgnoringASCIICase(empty.ptr(), 0));
    EXPECT_EQ(static_cast<size_t>(3), reference->findIgnoringASCIICase(empty.ptr(), 3));
    EXPECT_EQ(static_cast<size_t>(7), reference->findIgnoringASCIICase(empty.ptr(), 7));
    EXPECT_EQ(static_cast<size_t>(7), reference->findIgnoringASCIICase(empty.ptr(), 8));
    EXPECT_EQ(static_cast<size_t>(7), reference->findIgnoringASCIICase(empty.ptr(), 42));
    EXPECT_EQ(static_cast<size_t>(7), reference->findIgnoringASCIICase(empty.ptr(), std::numeric_limits<unsigned>::max()));
}

TEST(WTF, StringImplFindIgnoringASCIICaseWithPatternLongerThanReference)
{
    auto reference = stringFromUTF8("ABCÉEFG");
    auto pattern = stringFromUTF8("XABCÉEFG");
    EXPECT_EQ(static_cast<size_t>(WTF::notFound), reference->findIgnoringASCIICase(pattern.ptr()));
    EXPECT_EQ(static_cast<size_t>(1), pattern->findIgnoringASCIICase(reference.ptr()));
}

TEST(WTF, StringImplStartsWithIgnoringASCIICaseBasic)
{
    auto reference = stringFromUTF8("aBcéX");
    auto referenceEquivalent = stringFromUTF8("AbCéx");

    // Identity.
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(reference.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*reference.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(referenceEquivalent.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*referenceEquivalent.ptr()));
    ASSERT_TRUE(referenceEquivalent->startsWithIgnoringASCIICase(reference.ptr()));
    ASSERT_TRUE(referenceEquivalent->startsWithIgnoringASCIICase(*reference.ptr()));
    ASSERT_TRUE(referenceEquivalent->startsWithIgnoringASCIICase(referenceEquivalent.ptr()));
    ASSERT_TRUE(referenceEquivalent->startsWithIgnoringASCIICase(*referenceEquivalent.ptr()));

    // Proper prefixes.
    auto aLower = StringImpl::createFromLiteral("a");
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(aLower.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*aLower.ptr()));
    auto aUpper = StringImpl::createFromLiteral("A");
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(aUpper.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*aUpper.ptr()));

    auto abcLower = StringImpl::createFromLiteral("abc");
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(abcLower.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*abcLower.ptr()));
    auto abcUpper = StringImpl::createFromLiteral("ABC");
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(abcUpper.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*abcUpper.ptr()));

    auto abcAccentLower = stringFromUTF8("abcé");
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(abcAccentLower.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*abcAccentLower.ptr()));
    auto abcAccentUpper = stringFromUTF8("ABCé");
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(abcAccentUpper.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*abcAccentUpper.ptr()));

    // Negative cases.
    auto differentFirstChar = stringFromUTF8("bBcéX");
    auto differentFirstCharProperPrefix = stringFromUTF8("CBcé");
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(differentFirstChar.ptr()));
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(*differentFirstChar.ptr()));
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(differentFirstCharProperPrefix.ptr()));
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(*differentFirstCharProperPrefix.ptr()));

    auto uppercaseAccent = stringFromUTF8("aBcÉX");
    auto uppercaseAccentProperPrefix = stringFromUTF8("aBcÉX");
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(uppercaseAccent.ptr()));
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(*uppercaseAccent.ptr()));
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(uppercaseAccentProperPrefix.ptr()));
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(*uppercaseAccentProperPrefix.ptr()));
}

TEST(WTF, StringImplStartsWithIgnoringASCIICaseWithNull)
{
    auto reference = StringImpl::createFromLiteral("aBcDeFG");
    ASSERT_FALSE(reference->startsWithIgnoringASCIICase(nullptr));

    auto empty = StringImpl::create(reinterpret_cast<const LChar*>(""));
    ASSERT_FALSE(empty->startsWithIgnoringASCIICase(nullptr));
}

TEST(WTF, StringImplStartsWithIgnoringASCIICaseWithEmpty)
{
    auto reference = StringImpl::createFromLiteral("aBcDeFG");
    auto empty = StringImpl::create(reinterpret_cast<const LChar*>(""));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(empty.ptr()));
    ASSERT_TRUE(reference->startsWithIgnoringASCIICase(*empty.ptr()));
    ASSERT_TRUE(empty->startsWithIgnoringASCIICase(empty.ptr()));
    ASSERT_TRUE(empty->startsWithIgnoringASCIICase(*empty.ptr()));
    ASSERT_FALSE(empty->startsWithIgnoringASCIICase(reference.ptr()));
    ASSERT_FALSE(empty->startsWithIgnoringASCIICase(*reference.ptr()));
}

TEST(WTF, StartsWithLettersIgnoringASCIICase)
{
    String string("Test tEST");
    ASSERT_TRUE(startsWithLettersIgnoringASCIICase(string, "test t"));
    ASSERT_TRUE(startsWithLettersIgnoringASCIICase(string, "test te"));
    ASSERT_TRUE(startsWithLettersIgnoringASCIICase(string, "test test"));
    ASSERT_FALSE(startsWithLettersIgnoringASCIICase(string, "test tex"));

    ASSERT_TRUE(startsWithLettersIgnoringASCIICase(string, ""));
    ASSERT_TRUE(startsWithLettersIgnoringASCIICase(String(""), ""));

    ASSERT_FALSE(startsWithLettersIgnoringASCIICase(String(), "t"));
    ASSERT_FALSE(startsWithLettersIgnoringASCIICase(String(), ""));
}

TEST(WTF, StringImplEndsWithIgnoringASCIICaseBasic)
{
    auto reference = stringFromUTF8("XÉCbA");
    auto referenceEquivalent = stringFromUTF8("xÉcBa");

    // Identity.
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(reference.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*reference.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(referenceEquivalent.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*referenceEquivalent.ptr()));
    ASSERT_TRUE(referenceEquivalent->endsWithIgnoringASCIICase(reference.ptr()));
    ASSERT_TRUE(referenceEquivalent->endsWithIgnoringASCIICase(*reference.ptr()));
    ASSERT_TRUE(referenceEquivalent->endsWithIgnoringASCIICase(referenceEquivalent.ptr()));
    ASSERT_TRUE(referenceEquivalent->endsWithIgnoringASCIICase(*referenceEquivalent.ptr()));

    // Proper suffixes.
    auto aLower = StringImpl::createFromLiteral("a");
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(aLower.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*aLower.ptr()));
    auto aUpper = StringImpl::createFromLiteral("a");
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(aUpper.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*aUpper.ptr()));

    auto abcLower = StringImpl::createFromLiteral("cba");
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(abcLower.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*abcLower.ptr()));
    auto abcUpper = StringImpl::createFromLiteral("CBA");
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(abcUpper.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*abcUpper.ptr()));

    auto abcAccentLower = stringFromUTF8("Écba");
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(abcAccentLower.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*abcAccentLower.ptr()));
    auto abcAccentUpper = stringFromUTF8("ÉCBA");
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(abcAccentUpper.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*abcAccentUpper.ptr()));

    // Negative cases.
    auto differentLastChar = stringFromUTF8("XÉCbB");
    auto differentLastCharProperSuffix = stringFromUTF8("ÉCbb");
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(differentLastChar.ptr()));
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(*differentLastChar.ptr()));
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(differentLastCharProperSuffix.ptr()));
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(*differentLastCharProperSuffix.ptr()));

    auto lowercaseAccent = stringFromUTF8("aBcéX");
    auto loweraseAccentProperSuffix = stringFromUTF8("aBcéX");
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(lowercaseAccent.ptr()));
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(*lowercaseAccent.ptr()));
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(loweraseAccentProperSuffix.ptr()));
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(*loweraseAccentProperSuffix.ptr()));
}

TEST(WTF, StringImplEndsWithIgnoringASCIICaseWithNull)
{
    auto reference = StringImpl::createFromLiteral("aBcDeFG");
    ASSERT_FALSE(reference->endsWithIgnoringASCIICase(nullptr));

    auto empty = StringImpl::create(reinterpret_cast<const LChar*>(""));
    ASSERT_FALSE(empty->endsWithIgnoringASCIICase(nullptr));
}

TEST(WTF, StringImplEndsWithIgnoringASCIICaseWithEmpty)
{
    auto reference = StringImpl::createFromLiteral("aBcDeFG");
    auto empty = StringImpl::create(reinterpret_cast<const LChar*>(""));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(empty.ptr()));
    ASSERT_TRUE(reference->endsWithIgnoringASCIICase(*empty.ptr()));
    ASSERT_TRUE(empty->endsWithIgnoringASCIICase(empty.ptr()));
    ASSERT_TRUE(empty->endsWithIgnoringASCIICase(*empty.ptr()));
    ASSERT_FALSE(empty->endsWithIgnoringASCIICase(reference.ptr()));
    ASSERT_FALSE(empty->endsWithIgnoringASCIICase(*reference.ptr()));
}

TEST(WTF, StringImplCreateNullSymbol)
{
    auto reference = StringImpl::createNullSymbol();
    ASSERT_TRUE(reference->isSymbol());
    ASSERT_TRUE(reference->isNullSymbol());
    ASSERT_FALSE(reference->isAtomic());
    ASSERT_EQ(0u, reference->length());
    ASSERT_TRUE(equal(reference.ptr(), ""));
}

TEST(WTF, StringImplCreateSymbol)
{
    auto original = stringFromUTF8("original");
    auto reference = StringImpl::createSymbol(original);
    ASSERT_TRUE(reference->isSymbol());
    ASSERT_FALSE(reference->isNullSymbol());
    ASSERT_FALSE(reference->isAtomic());
    ASSERT_FALSE(original->isSymbol());
    ASSERT_FALSE(original->isAtomic());
    ASSERT_EQ(original->length(), reference->length());
    ASSERT_TRUE(equal(reference.ptr(), "original"));

    auto empty = stringFromUTF8("");
    auto emptyReference = StringImpl::createSymbol(empty);
    ASSERT_TRUE(emptyReference->isSymbol());
    ASSERT_FALSE(emptyReference->isNullSymbol());
    ASSERT_FALSE(emptyReference->isAtomic());
    ASSERT_FALSE(empty->isSymbol());
    ASSERT_FALSE(empty->isNullSymbol());
    ASSERT_TRUE(empty->isAtomic());
    ASSERT_EQ(empty->length(), emptyReference->length());
    ASSERT_TRUE(equal(emptyReference.ptr(), ""));
}

TEST(WTF, StringImplSymbolToAtomicString)
{
    auto original = stringFromUTF8("original");
    auto reference = StringImpl::createSymbol(original);
    ASSERT_TRUE(reference->isSymbol());
    ASSERT_FALSE(reference->isAtomic());

    auto atomic = AtomicStringImpl::add(reference.ptr());
    ASSERT_TRUE(atomic->isAtomic());
    ASSERT_FALSE(atomic->isSymbol());
    ASSERT_TRUE(reference->isSymbol());
    ASSERT_FALSE(reference->isAtomic());
}

TEST(WTF, StringImplNullSymbolToAtomicString)
{
    auto reference = StringImpl::createNullSymbol();
    ASSERT_TRUE(reference->isSymbol());
    ASSERT_FALSE(reference->isAtomic());

    auto atomic = AtomicStringImpl::add(reference.ptr());
    ASSERT_TRUE(atomic->isAtomic());
    ASSERT_FALSE(atomic->isSymbol());
    ASSERT_TRUE(reference->isSymbol());
    ASSERT_FALSE(reference->isAtomic());
}

} // namespace TestWebKitAPI
