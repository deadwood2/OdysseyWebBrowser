/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
#import "WebResourceLoadStatisticsStore.h"

namespace WebKit {

void WebResourceLoadStatisticsStore::registerUserDefaultsIfNeeded()
{
    static dispatch_once_t initOnce;

    dispatch_once(&initOnce, ^ {
        Seconds timeToLiveUserInteraction([[NSUserDefaults standardUserDefaults] doubleForKey:@"ResourceLoadStatisticsTimeToLiveUserInteraction"]);
        if (timeToLiveUserInteraction > 0_s && timeToLiveUserInteraction <= 24_h * 30)
            setTimeToLiveUserInteraction(timeToLiveUserInteraction);

        Seconds timeToLiveCookiePartitionFree([[NSUserDefaults standardUserDefaults] doubleForKey:@"ResourceLoadStatisticsTimeToLiveCookiePartitionFree"]);
        if (timeToLiveCookiePartitionFree > 0_s && timeToLiveCookiePartitionFree <= 24_h)
            setTimeToLiveCookiePartitionFree(timeToLiveCookiePartitionFree);

        Seconds minimumTimeBetweenDataRecordsRemoval([[NSUserDefaults standardUserDefaults] doubleForKey:@"ResourceLoadStatisticsMinimumTimeBetweenDataRecordsRemoval"]);
        if (minimumTimeBetweenDataRecordsRemoval > 0_s && minimumTimeBetweenDataRecordsRemoval < 1_h)
            setMinimumTimeBetweenDataRecordsRemoval(minimumTimeBetweenDataRecordsRemoval);

        Seconds grandfatheringTime([[NSUserDefaults standardUserDefaults] doubleForKey:@"ResourceLoadStatisticsGrandfatheringTime"]);
        if (grandfatheringTime > 0_s && grandfatheringTime <= 24_h * 7)
            setGrandfatheringTime(grandfatheringTime);
    });
}

}
