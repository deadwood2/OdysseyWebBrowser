/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#include <WebCore/AdClickAttribution.h>
#include <wtf/URL.h>
#include <wtf/WallTime.h>

using namespace WebCore;

namespace TestWebKitAPI {

constexpr uint32_t min6BitValue { 0 };
constexpr uint32_t max6BitValue { 63 };

// Positive test cases.

TEST(AdClickAttribution, ValidMinValues)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(min6BitValue), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));
    attribution.setConversion(AdClickAttribution::Conversion(min6BitValue, AdClickAttribution::Priority(min6BitValue)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();
    
    ASSERT_EQ(attributionURL.string(), "https://webkit.org/.well-known/ad-click-attribution/0/0");
    ASSERT_EQ(referrerURL.string(), "https://example.com/");
}

TEST(AdClickAttribution, ValidMidValues)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign((uint32_t)12), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));
    attribution.setConversion(AdClickAttribution::Conversion((uint32_t)44, AdClickAttribution::Priority((uint32_t)22)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_EQ(attributionURL.string(), "https://webkit.org/.well-known/ad-click-attribution/44/12");
    ASSERT_EQ(referrerURL.string(), "https://example.com/");
}

TEST(AdClickAttribution, ValidMaxValues)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));
    attribution.setConversion(AdClickAttribution::Conversion(max6BitValue, AdClickAttribution::Priority(max6BitValue)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_EQ(attributionURL.string(), "https://webkit.org/.well-known/ad-click-attribution/63/63");
    ASSERT_EQ(referrerURL.string(), "https://example.com/");
}

TEST(AdClickAttribution, EarliestTimeToSendAttributionMinimumDelay)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));
    auto now = WallTime::now();
    attribution.setConversion(AdClickAttribution::Conversion(max6BitValue, AdClickAttribution::Priority(max6BitValue)));
    auto earliestTimeToSend = attribution.earliestTimeToSend();
    ASSERT_TRUE(earliestTimeToSend);
    ASSERT_TRUE(earliestTimeToSend.value().secondsSinceEpoch() - 24_h >= now.secondsSinceEpoch());
}

// Negative test cases.

TEST(AdClickAttribution, InvalidCampaignId)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue + 1), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));
    attribution.setConversion(AdClickAttribution::Conversion(max6BitValue, AdClickAttribution::Priority(max6BitValue)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_TRUE(attributionURL.string().isEmpty());
    ASSERT_TRUE(referrerURL.string().isEmpty());
}

TEST(AdClickAttribution, InvalidSourceHost)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue), AdClickAttribution::Source("webkitorg"), AdClickAttribution::Destination("example.com"));
    attribution.setConversion(AdClickAttribution::Conversion(max6BitValue, AdClickAttribution::Priority(max6BitValue)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_TRUE(attributionURL.string().isEmpty());
    ASSERT_TRUE(referrerURL.string().isEmpty());
}

TEST(AdClickAttribution, InvalidDestinationHost)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue + 1), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("examplecom"));
    attribution.setConversion(AdClickAttribution::Conversion(max6BitValue, AdClickAttribution::Priority(max6BitValue)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_TRUE(attributionURL.string().isEmpty());
    ASSERT_TRUE(referrerURL.string().isEmpty());
}

TEST(AdClickAttribution, InvalidConversionData)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));
    attribution.setConversion(AdClickAttribution::Conversion((max6BitValue + 1), AdClickAttribution::Priority(max6BitValue)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_TRUE(attributionURL.string().isEmpty());
    ASSERT_TRUE(referrerURL.string().isEmpty());
}

TEST(AdClickAttribution, InvalidPriority)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));
    attribution.setConversion(AdClickAttribution::Conversion(max6BitValue, AdClickAttribution::Priority(max6BitValue + 1)));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_TRUE(attributionURL.string().isEmpty());
    ASSERT_TRUE(referrerURL.string().isEmpty());
}

TEST(AdClickAttribution, InvalidMissingConversion)
{
    AdClickAttribution attribution(AdClickAttribution::Campaign(max6BitValue), AdClickAttribution::Source("webkit.org"), AdClickAttribution::Destination("example.com"));

    auto attributionURL = attribution.url();
    auto referrerURL = attribution.referrer();

    ASSERT_TRUE(attributionURL.string().isEmpty());
    ASSERT_TRUE(referrerURL.string().isEmpty());
    ASSERT_FALSE(attribution.earliestTimeToSend());
}

} // namespace TestWebKitAPI
