/*
 * Copyright (C) 2009, 2011 Apple Inc. All Rights Reserved.
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

#import "WebApplicationCacheInternal.h"

#import "WebSecurityOriginInternal.h"
#import <WebCore/ApplicationCache.h>
#import <WebCore/ApplicationCacheStorage.h>
#import <WebCore/SecurityOrigin.h>
#import <wtf/RetainPtr.h>

#if PLATFORM(IOS)
#import "WebKitNSStringExtras.h"
#import "WebSQLiteDatabaseTrackerClient.h"
#import <WebCore/SQLiteDatabaseTracker.h>
#endif

using namespace WebCore;

@implementation WebApplicationCache

#if PLATFORM(IOS)
// FIXME: This will be removed when WebKitInitializeApplicationCachePathIfNecessary()
// is moved from WebView.mm to WebKitInitializeApplicationCacheIfNecessary() in this file.
// https://bugs.webkit.org/show_bug.cgi?id=57567 
+ (void)initializeWithBundleIdentifier:(NSString *)bundleIdentifier
{
    static BOOL initialized = NO;
    if (initialized)
        return;
    
    SQLiteDatabaseTracker::setClient(WebSQLiteDatabaseTrackerClient::sharedWebSQLiteDatabaseTrackerClient());

    webApplicationCacheStorage().setCacheDirectory([NSString _webkit_localCacheDirectoryWithBundleIdentifier:bundleIdentifier]);
    
    initialized = YES;
}
#endif

+ (long long)maximumSize
{
    return webApplicationCacheStorage().maximumSize();
}

+ (void)setMaximumSize:(long long)size
{
    webApplicationCacheStorage().deleteAllEntries();
    webApplicationCacheStorage().setMaximumSize(size);
}

+ (long long)defaultOriginQuota
{
    return webApplicationCacheStorage().defaultOriginQuota();
}

+ (void)setDefaultOriginQuota:(long long)size
{
    webApplicationCacheStorage().setDefaultOriginQuota(size);
}

+ (long long)diskUsageForOrigin:(WebSecurityOrigin *)origin
{
    return webApplicationCacheStorage().diskUsageForOrigin(*[origin _core]);
}

+ (void)deleteAllApplicationCaches
{
    webApplicationCacheStorage().deleteAllCaches();
}

+ (void)deleteCacheForOrigin:(WebSecurityOrigin *)origin
{
    webApplicationCacheStorage().deleteCacheForOrigin(*[origin _core]);
}

+ (NSArray *)originsWithCache
{
    HashSet<RefPtr<SecurityOrigin>> coreOrigins;
    webApplicationCacheStorage().getOriginsWithCache(coreOrigins);
    
    NSMutableArray *webOrigins = [[[NSMutableArray alloc] initWithCapacity:coreOrigins.size()] autorelease];
    
    HashSet<RefPtr<SecurityOrigin>>::const_iterator end = coreOrigins.end();
    for (HashSet<RefPtr<SecurityOrigin>>::const_iterator it = coreOrigins.begin(); it != end; ++it) {
        RetainPtr<WebSecurityOrigin> webOrigin = adoptNS([[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:(*it).get()]);
        [webOrigins addObject:webOrigin.get()];
    }
    
    return webOrigins;
}

@end

WebCore::ApplicationCacheStorage& webApplicationCacheStorage()
{
    return ApplicationCacheStorage::singleton();
}
