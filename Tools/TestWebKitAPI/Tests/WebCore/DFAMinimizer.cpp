/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#include <WebCore/CombinedURLFilters.h>
#include <WebCore/NFA.h>
#include <WebCore/NFAToDFA.h>
#include <WebCore/URLFilterParser.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace TestWebKitAPI {

class DFAMinimizerTest : public testing::Test {
public:
    virtual void SetUp()
    {
        WTF::initializeMainThread();
    }
};

static unsigned countLiveNodes(const ContentExtensions::DFA& dfa)
{
    unsigned counter = 0;
    for (const auto& node : dfa.nodes) {
        if (!node.isKilled())
            ++counter;
    }
    return counter;
}

static Vector<ContentExtensions::NFA> createNFAs(ContentExtensions::CombinedURLFilters& combinedURLFilters)
{
    Vector<ContentExtensions::NFA> nfas;

    combinedURLFilters.processNFAs([&](ContentExtensions::NFA&& nfa) {
        nfas.append(WTF::move(nfa));
    });

    return nfas;
}

ContentExtensions::DFA buildDFAFromPatterns(Vector<const char*> patterns)
{
    ContentExtensions::CombinedURLFilters combinedURLFilters;
    ContentExtensions::URLFilterParser parser(combinedURLFilters);

    for (const char* pattern : patterns)
        parser.addPattern(pattern, false, 0);
    Vector<ContentExtensions::NFA> nfas = createNFAs(combinedURLFilters);
    return ContentExtensions::NFAToDFA::convert(nfas[0]);
}

TEST_F(DFAMinimizerTest, BasicSearch)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ ".*foo", ".*bar", ".*bang"});
    EXPECT_EQ(static_cast<size_t>(10), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(7), countLiveNodes(dfa));
}

TEST_F(DFAMinimizerTest, FallbackTransitionsWithDifferentiatorDoNotMerge1)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ "^a.a", "^b.a", "^bac", "^bbc", "^BCC"});
    EXPECT_EQ(static_cast<size_t>(13), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(6), countLiveNodes(dfa));
}

TEST_F(DFAMinimizerTest, FallbackTransitionsWithDifferentiatorDoNotMerge2)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ "^bbc", "^BCC", "^a.a", "^b.a"});
    EXPECT_EQ(static_cast<size_t>(11), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(6), countLiveNodes(dfa));
}

TEST_F(DFAMinimizerTest, FallbackTransitionsWithDifferentiatorDoNotMerge3)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ "^a.c", "^b.c", "^baa", "^bba", "^BCA"});
    EXPECT_EQ(static_cast<size_t>(13), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(6), countLiveNodes(dfa));
}

TEST_F(DFAMinimizerTest, FallbackTransitionsWithDifferentiatorDoNotMerge4)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ "^baa", "^bba", "^BCA", "^a.c", "^b.c"});
    EXPECT_EQ(static_cast<size_t>(13), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(6), countLiveNodes(dfa));
}

TEST_F(DFAMinimizerTest, FallbackTransitionsToOtherNodeInSameGroupDoesNotDifferentiateGroup)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ "^aac", "^a.c", "^b.c"});
    EXPECT_EQ(static_cast<size_t>(9), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(5), countLiveNodes(dfa));
}

TEST_F(DFAMinimizerTest, SimpleFallBackTransitionDifferentiator1)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ "^a.bc.de", "^a.bd.ef"});
    EXPECT_EQ(static_cast<size_t>(12), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(11), countLiveNodes(dfa));
}

TEST_F(DFAMinimizerTest, SimpleFallBackTransitionDifferentiator2)
{
    ContentExtensions::DFA dfa = buildDFAFromPatterns({ "^cb.", "^db.b"});
    EXPECT_EQ(static_cast<size_t>(8), countLiveNodes(dfa));
    dfa.minimize();
    EXPECT_EQ(static_cast<size_t>(7), countLiveNodes(dfa));
}

} // namespace TestWebKitAPI
