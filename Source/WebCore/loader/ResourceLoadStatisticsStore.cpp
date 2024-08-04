/*
 * Copyright (C) 2016-2017 Apple Inc. All rights reserved.
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
#include "ResourceLoadStatisticsStore.h"

#include "KeyedCoding.h"
#include "Logging.h"
#include "NetworkStorageSession.h"
#include "PlatformStrategies.h"
#include "ResourceLoadStatistics.h"
#include "SharedBuffer.h"
#include "URL.h"
#include <wtf/CurrentTime.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

static const auto statisticsModelVersion = 3;
// 30 days in seconds
static auto timeToLiveUserInteraction = 2592000;

Ref<ResourceLoadStatisticsStore> ResourceLoadStatisticsStore::create()
{
    return adoptRef(*new ResourceLoadStatisticsStore());
}
    
bool ResourceLoadStatisticsStore::isPrevalentResource(const String& primaryDomain) const
{
    auto mapEntry = m_resourceStatisticsMap.find(primaryDomain);
    if (mapEntry == m_resourceStatisticsMap.end())
        return false;

    return mapEntry->value.isPrevalentResource;
}
    
ResourceLoadStatistics& ResourceLoadStatisticsStore::ensureResourceStatisticsForPrimaryDomain(const String& primaryDomain)
{
    auto addResult = m_resourceStatisticsMap.ensure(primaryDomain, [&primaryDomain] {
        return ResourceLoadStatistics(primaryDomain);
    });

    return addResult.iterator->value;
}

void ResourceLoadStatisticsStore::setResourceStatisticsForPrimaryDomain(const String& primaryDomain, ResourceLoadStatistics&& statistics)
{
    m_resourceStatisticsMap.set(primaryDomain, WTFMove(statistics));
}

typedef HashMap<String, ResourceLoadStatistics>::KeyValuePairType StatisticsValue;

std::unique_ptr<KeyedEncoder> ResourceLoadStatisticsStore::createEncoderFromData()
{
    auto encoder = KeyedEncoder::encoder();

    encoder->encodeUInt32("version", statisticsModelVersion);
    encoder->encodeObjects("browsingStatistics", m_resourceStatisticsMap.begin(), m_resourceStatisticsMap.end(), [this](KeyedEncoder& encoderInner, const StatisticsValue& origin) {
        origin.value.encode(encoderInner);
    });

    return encoder;
}

void ResourceLoadStatisticsStore::readDataFromDecoder(KeyedDecoder& decoder)
{
    if (m_resourceStatisticsMap.size())
        return;

    unsigned version;
    if (!decoder.decodeUInt32("version", version))
        version = 1;
    Vector<ResourceLoadStatistics> loadedStatistics;
    bool succeeded = decoder.decodeObjects("browsingStatistics", loadedStatistics, [this, version](KeyedDecoder& decoderInner, ResourceLoadStatistics& statistics) {
        return statistics.decode(decoderInner, version);
    });

    if (!succeeded)
        return;

    for (auto& statistics : loadedStatistics)
        m_resourceStatisticsMap.set(statistics.highLevelDomain, statistics);
}

String ResourceLoadStatisticsStore::statisticsForOrigin(const String& origin)
{
    auto iter = m_resourceStatisticsMap.find(origin);
    if (iter == m_resourceStatisticsMap.end())
        return emptyString();
    
    return "Statistics for " + origin + ":\n" + iter->value.toString();
}

Vector<ResourceLoadStatistics> ResourceLoadStatisticsStore::takeStatistics()
{
    Vector<ResourceLoadStatistics> statistics;
    statistics.reserveInitialCapacity(m_resourceStatisticsMap.size());
    for (auto& statistic : m_resourceStatisticsMap.values())
        statistics.uncheckedAppend(WTFMove(statistic));

    m_resourceStatisticsMap.clear();

    return statistics;
}

void ResourceLoadStatisticsStore::mergeStatistics(const Vector<ResourceLoadStatistics>& statistics)
{
    for (auto& statistic : statistics) {
        auto result = m_resourceStatisticsMap.ensure(statistic.highLevelDomain, [&statistic] {
            return ResourceLoadStatistics(statistic.highLevelDomain);
        });
        
        result.iterator->value.merge(statistic);
    }
}

void ResourceLoadStatisticsStore::setNotificationCallback(std::function<void()> handler)
{
    m_dataAddedHandler = WTFMove(handler);
}

void ResourceLoadStatisticsStore::fireDataModificationHandler()
{
    if (m_dataAddedHandler)
        m_dataAddedHandler();
}

void ResourceLoadStatisticsStore::setTimeToLiveUserInteraction(double seconds)
{
    if (seconds >= 0)
        timeToLiveUserInteraction = seconds;
}

void ResourceLoadStatisticsStore::processStatistics(std::function<void(ResourceLoadStatistics&)>&& processFunction)
{
    for (auto& resourceStatistic : m_resourceStatisticsMap.values())
        processFunction(resourceStatistic);
}

bool ResourceLoadStatisticsStore::hasHadRecentUserInteraction(ResourceLoadStatistics& resourceStatistic)
{
    if (!resourceStatistic.hadUserInteraction)
        return false;

    if (currentTime() > resourceStatistic.mostRecentUserInteraction + timeToLiveUserInteraction) {
        // Drop privacy sensitive data because we no longer need it.
        // Set timestamp to 0.0 so that statistics merge will know
        // it has been reset as opposed to its default -1.
        resourceStatistic.mostRecentUserInteraction = 0;
        resourceStatistic.hadUserInteraction = false;
        return false;
    }

    return true;
}

Vector<String> ResourceLoadStatisticsStore::prevalentResourceDomainsWithoutUserInteraction()
{
    Vector<String> prevalentResources;
    for (auto& resourceStatistic : m_resourceStatisticsMap.values()) {
        if (resourceStatistic.isPrevalentResource && !hasHadRecentUserInteraction(resourceStatistic))
            prevalentResources.append(resourceStatistic.highLevelDomain);
    }
    return prevalentResources;
}

void ResourceLoadStatisticsStore::updateStatisticsForRemovedDataRecords(const Vector<String>& prevalentResourceDomains)
{
    for (auto& prevalentResourceDomain : prevalentResourceDomains) {
        ResourceLoadStatistics& statisic = ensureResourceStatisticsForPrimaryDomain(prevalentResourceDomain);
        ++statisic.dataRecordsRemoved;
    }
}
}
