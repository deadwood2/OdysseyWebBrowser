/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
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

#import "WebCache.h"

#import "WebApplicationCacheInternal.h"
#import "WebNSObjectExtras.h"
#import "WebPreferences.h"
#import "WebSystemInterface.h"
#import "WebView.h"
#import "WebViewInternal.h"
#import <WebCore/ApplicationCacheStorage.h>
#import <WebCore/CredentialStorage.h>
#import <WebCore/CrossOriginPreflightResultCache.h>
#import <WebCore/MemoryCache.h>
#import <runtime/InitializeThreading.h>
#import <wtf/MainThread.h>
#import <wtf/RunLoop.h>

#if PLATFORM(IOS)
#import "MemoryMeasure.h"
#import "WebFrameInternal.h"
#import <WebCore/CachedImage.h>
#import <WebCore/Frame.h>
#import <WebCore/PageCache.h>
#import <WebCore/WebCoreThreadRun.h>
#endif

#if ENABLE(CACHE_PARTITIONING)
#import <WebCore/Document.h>
#endif

@implementation WebCache

+ (void)initialize
{
#if !PLATFORM(IOS)
    JSC::initializeThreading();
    WTF::initializeMainThreadToProcessMainThread();
    RunLoop::initializeMainRunLoop();
#endif
    InitWebCoreSystemInterface();   
}

+ (NSArray *)statistics
{
    WebCore::MemoryCache::Statistics s = WebCore::MemoryCache::singleton().getStatistics();

    return [NSArray arrayWithObjects:
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.count], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.count], @"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.count], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.count], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.size], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.size] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.size], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.size], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.liveSize], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.liveSize] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.liveSize], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.liveSize], @"JavaScript",
            nil],
        [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:s.images.decodedSize], @"Images",
            [NSNumber numberWithInt:s.cssStyleSheets.decodedSize] ,@"CSS",
#if ENABLE(XSLT)
            [NSNumber numberWithInt:s.xslStyleSheets.decodedSize], @"XSL",
#else
            [NSNumber numberWithInt:0], @"XSL",
#endif
            [NSNumber numberWithInt:s.scripts.decodedSize], @"JavaScript",
            nil],
        nil];
}

+ (void)empty
{
    // Toggling the cache model like this forces the cache to evict all its in-memory resources.
    WebCacheModel cacheModel = [WebView _cacheModel];
    [WebView _setCacheModel:WebCacheModelDocumentViewer];
    [WebView _setCacheModel:cacheModel];

    // Empty the application cache.
    webApplicationCacheStorage().empty();

    // Empty the Cross-Origin Preflight cache
    WebCore::CrossOriginPreflightResultCache::singleton().empty();
}

#if PLATFORM(IOS)
+ (void)emptyInMemoryResources
{
    // This method gets called from MobileSafari after it calls [WebView
    // _close]. [WebView _close] schedules its work on the WebThread. So we
    // schedule this method on the WebThread as well so as to pick up all the
    // dead resources left behind after closing the WebViews
    WebThreadRun(^{
        WebKit::MemoryMeasure measurer("[WebCache emptyInMemoryResources]");

        // Toggling the cache model like this forces the cache to evict all its in-memory resources.
        WebCacheModel cacheModel = [WebView _cacheModel];
        [WebView _setCacheModel:WebCacheModelDocumentViewer];
        [WebView _setCacheModel:cacheModel];

        WebCore::MemoryCache::singleton().pruneLiveResources(true);
    });
}

+ (void)sizeOfDeadResources:(int *)resources
{
    WebCore::MemoryCache::Statistics stats = WebCore::MemoryCache::singleton().getStatistics();
    if (resources) {
        *resources = (stats.images.size - stats.images.liveSize)
                     + (stats.cssStyleSheets.size - stats.cssStyleSheets.liveSize)
#if ENABLE(XSLT)
                     + (stats.xslStyleSheets.size - stats.xslStyleSheets.liveSize)
#endif
                     + (stats.scripts.size - stats.scripts.liveSize);
    }
}

+ (bool)addImageToCache:(CGImageRef)image forURL:(NSURL *)url
{
    return [WebCache addImageToCache:image forURL:url forFrame:nil];
}

+ (bool)addImageToCache:(CGImageRef)image forURL:(NSURL *)url forFrame:(WebFrame *)frame
{
    if (!image || !url || ![[url absoluteString] length])
        return false;
    WebCore::SecurityOrigin* topOrigin = nullptr;
#if ENABLE(CACHE_PARTITIONING)
    if (frame)
        topOrigin = core(frame)->document()->topOrigin();
#endif
    return WebCore::MemoryCache::singleton().addImageToCache(RetainPtr<CGImageRef>(image), url, topOrigin ? topOrigin->domainForCachePartition() : emptyString());
}

+ (void)removeImageFromCacheForURL:(NSURL *)url
{
    [WebCache removeImageFromCacheForURL:url forFrame:nil];
}

+ (void)removeImageFromCacheForURL:(NSURL *)url forFrame:(WebFrame *)frame
{
    if (!url)
        return;
    WebCore::SecurityOrigin* topOrigin = nullptr;
#if ENABLE(CACHE_PARTITIONING)
    if (frame)
        topOrigin = core(frame)->document()->topOrigin();
#endif
    WebCore::MemoryCache::singleton().removeImageFromCache(url, topOrigin ? topOrigin->domainForCachePartition() : emptyString());
}

+ (CGImageRef)imageForURL:(NSURL *)url
{
    if (!url)
        return nullptr;
    
    WebCore::ResourceRequest request(url);
    WebCore::CachedResource* cachedResource = WebCore::MemoryCache::singleton().resourceForRequest(request, WebCore::SessionID::defaultSessionID());
    if (!is<WebCore::CachedImage>(cachedResource))
        return nullptr;
    WebCore::CachedImage& cachedImage = downcast<WebCore::CachedImage>(*cachedResource);
    if (!cachedImage.hasImage())
        return nullptr;
    return cachedImage.image()->getCGImageRef();
}

#endif // PLATFORM(IOS)

+ (void)setDisabled:(BOOL)disabled
{
    if (!pthread_main_np())
        return [[self _webkit_invokeOnMainThread] setDisabled:disabled];

    WebCore::MemoryCache::singleton().setDisabled(disabled);
}

+ (BOOL)isDisabled
{
    return WebCore::MemoryCache::singleton().disabled();
}

+ (void)clearCachedCredentials
{
    [WebView _makeAllWebViewsPerformSelector:@selector(_clearCredentials)];
    WebCore::CredentialStorage::defaultCredentialStorage().clearCredentials();
}

@end
