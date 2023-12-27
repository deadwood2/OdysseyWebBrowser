/*
 * Copyright (C) 2007, 2010, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebSecurityOriginInternal.h"

#import "WebApplicationCacheQuotaManager.h"
#import "WebDatabaseQuotaManager.h"
#import "WebQuotaManager.h"
#import <WebCore/URL.h>
#import <WebCore/DatabaseManager.h>
#import <WebCore/SecurityOrigin.h>

using namespace WebCore;

@implementation WebSecurityOrigin

+ (id)webSecurityOriginFromDatabaseIdentifier:(NSString *)databaseIdentifier
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::maybeCreateFromDatabaseIdentifier(databaseIdentifier);
    if (!origin)
        return nil;

    return [[[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:origin.get()] autorelease];
}

- (id)initWithURL:(NSURL *)url
{
    self = [super init];
    if (!self)
        return nil;

    RefPtr<SecurityOrigin> origin = SecurityOrigin::create(URL([url absoluteURL]));
    SecurityOrigin* rawOrigin = origin.release().leakRef();
    _private = reinterpret_cast<WebSecurityOriginPrivate *>(rawOrigin);

    return self;
}

- (NSString *)protocol
{
    return reinterpret_cast<SecurityOrigin*>(_private)->protocol();
}

- (NSString *)host
{
    return reinterpret_cast<SecurityOrigin*>(_private)->host();
}

- (NSString *)databaseIdentifier
{
    return reinterpret_cast<SecurityOrigin*>(_private)->databaseIdentifier();
}

#if PLATFORM(IOS)
- (NSString *)toString
{
    return reinterpret_cast<SecurityOrigin*>(_private)->toString();
}
#endif

- (NSString *)stringValue
{
    return reinterpret_cast<SecurityOrigin*>(_private)->toString();
}

- (unsigned short)port
{
    return reinterpret_cast<SecurityOrigin*>(_private)->port();
}

// FIXME: Overriding isEqual: without overriding hash will cause trouble if this ever goes into an NSSet or is the key in an NSDictionary,
// since two equal objects could have different hashes.
- (BOOL)isEqual:(id)anObject
{
    if (![anObject isMemberOfClass:[WebSecurityOrigin class]])
        return NO;
    
    return [self _core]->equal([anObject _core]);
}

- (void)dealloc
{
    if (_private)
        reinterpret_cast<SecurityOrigin*>(_private)->deref();
    if (_applicationCacheQuotaManager)
        [(NSObject *)_applicationCacheQuotaManager release];
    if (_databaseQuotaManager)
        [(NSObject *)_databaseQuotaManager release];
    [super dealloc];
}

- (void)finalize
{
    if (_private)
        reinterpret_cast<SecurityOrigin*>(_private)->deref();
    [super finalize];
}

@end

@implementation WebSecurityOrigin (WebInternal)

- (id)_initWithWebCoreSecurityOrigin:(SecurityOrigin*)origin
{
    ASSERT(origin);
    self = [super init];
    if (!self)
        return nil;

    origin->ref();
    _private = reinterpret_cast<WebSecurityOriginPrivate *>(origin);

    return self;
}

- (SecurityOrigin *)_core
{
    return reinterpret_cast<SecurityOrigin*>(_private);
}

@end


// MARK: -
// MARK: WebQuotaManagers

@implementation WebSecurityOrigin (WebQuotaManagers)

- (id<WebQuotaManager>)applicationCacheQuotaManager
{
    if (!_applicationCacheQuotaManager)
        _applicationCacheQuotaManager = [[WebApplicationCacheQuotaManager alloc] initWithOrigin:self];
    return _applicationCacheQuotaManager;
}

- (id<WebQuotaManager>)databaseQuotaManager
{
    if (!_databaseQuotaManager)
        _databaseQuotaManager = [[WebDatabaseQuotaManager alloc] initWithOrigin:self];
    return _databaseQuotaManager;
}

@end


// MARK: -
// MARK: Deprecated

// FIXME: The following methods are deprecated and should removed later.
// Clients should instead get a WebQuotaManager, and query / set the quota via the Manager.
// NOTE: the <WebCore/DatabaseManager.h> #include should be removed as well.

@implementation WebSecurityOrigin (Deprecated)

- (unsigned long long)usage
{
    return DatabaseManager::singleton().usageForOrigin(reinterpret_cast<SecurityOrigin*>(_private));
}

- (unsigned long long)quota
{
    return DatabaseManager::singleton().quotaForOrigin(reinterpret_cast<SecurityOrigin*>(_private));
}

- (void)setQuota:(unsigned long long)quota
{
    DatabaseManager::singleton().setQuota(reinterpret_cast<SecurityOrigin*>(_private), quota);
}

@end
