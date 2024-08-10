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

#import "config.h"
#import "WKTextFinderClient.h"

#if PLATFORM(MAC) && WK_API_ENABLED

#import "APIFindClient.h"
#import "APIFindMatchesClient.h"
#import "WebImage.h"
#import "WebPageProxy.h"
#import <algorithm>
#import <pal/spi/mac/NSTextFinderSPI.h>
#import <wtf/BlockPtr.h>
#import <wtf/Deque.h>

// FIXME: Implement scrollFindMatchToVisible.
// FIXME: The NSTextFinder overlay doesn't move with scrolling; we should have a mode where we manage the overlay.

static const NSUInteger maximumFindMatches = 1000;

@interface WKTextFinderClient ()

- (void)didFindStringMatchesWithRects:(const Vector<Vector<WebCore::IntRect>>&)rects didWrapAround:(BOOL)didWrapAround;

- (void)getImageForMatchResult:(id <NSTextFinderAsynchronousDocumentFindMatch>)findMatch completionHandler:(void (^)(NSImage *generatedImage))completionHandler;
- (void)didGetImageForMatchResult:(WebKit::WebImage *)string;

@end

namespace WebKit {
using namespace WebCore;

class TextFinderFindClient : public API::FindMatchesClient, public API::FindClient {
public:
    explicit TextFinderFindClient(WKTextFinderClient *textFinderClient)
        : m_textFinderClient(textFinderClient)
    {
    }

private:
    void didFindStringMatches(WebPageProxy* page, const String&, const Vector<Vector<WebCore::IntRect>>& matchRects, int32_t) override
    {
        [m_textFinderClient didFindStringMatchesWithRects:matchRects didWrapAround:NO];
    }

    void didGetImageForMatchResult(WebPageProxy* page, WebImage* image, int32_t index) override
    {
        [m_textFinderClient didGetImageForMatchResult:image];
    }

    void didFindString(WebPageProxy*, const String&, const Vector<WebCore::IntRect>& matchRects, uint32_t, int32_t, bool didWrapAround) override
    {
        [m_textFinderClient didFindStringMatchesWithRects:{ matchRects } didWrapAround:didWrapAround];
    }

    void didFailToFindString(WebPageProxy*, const String& string) override
    {
        [m_textFinderClient didFindStringMatchesWithRects:{ } didWrapAround:NO];
    }

    RetainPtr<WKTextFinderClient> m_textFinderClient;
};
    
}

@interface WKTextFinderMatch : NSObject <NSTextFinderAsynchronousDocumentFindMatch>

@property (nonatomic, readonly) unsigned index;

@end

@implementation WKTextFinderMatch {
    WKTextFinderClient *_client;
    NSView *_view;
    RetainPtr<NSArray> _rects;
    unsigned _index;
}

- (instancetype)initWithClient:(WKTextFinderClient *)client view:(NSView *)view index:(unsigned)index rects:(NSArray *)rects
{
    self = [super init];

    if (!self)
        return nil;

    _client = client;
    _view = view;
    _rects = rects;
    _index = index;

    return self;
}

- (NSView *)containingView
{
    return _view;
}

- (NSArray *)textRects
{
    return _rects.get();
}

- (void)generateTextImage:(void (^)(NSImage *generatedImage))completionHandler
{
    [_client getImageForMatchResult:self completionHandler:completionHandler];
}

- (unsigned)index
{
    return _index;
}

@end

@implementation WKTextFinderClient {
    WebKit::WebPageProxy *_page;
    NSView *_view;
    Deque<WTF::Function<void(NSArray *, bool didWrap)>> _findReplyCallbacks;
    Deque<WTF::Function<void(NSImage *)>> _imageReplyCallbacks;
}

- (instancetype)initWithPage:(WebKit::WebPageProxy&)page view:(NSView *)view
{
    self = [super init];

    if (!self)
        return nil;

    _page = &page;
    _view = view;
    
    _page->setFindMatchesClient(std::make_unique<WebKit::TextFinderFindClient>(self));
    _page->setFindClient(std::make_unique<WebKit::TextFinderFindClient>(self));

    return self;
}

- (void)willDestroyView:(NSView *)view
{
    _page->setFindMatchesClient(nullptr);
    _page->setFindClient(nullptr);

    _page = nullptr;
    _view = nil;
}

#pragma mark - NSTextFinderClient SPI

- (void)replaceMatches:(NSArray *)matches withString:(NSString *)replacementText inSelectionOnly:(BOOL)selectionOnly resultCollector:(void (^)(NSUInteger replacementCount))resultCollector
{
    Vector<uint32_t> matchIndices;
    matchIndices.reserveCapacity(matches.count);
    for (id match in matches) {
        if ([match isKindOfClass:WKTextFinderMatch.class])
            matchIndices.uncheckedAppend([(WKTextFinderMatch *)match index]);
    }
    _page->replaceMatches(WTFMove(matchIndices), replacementText, selectionOnly, [collector = makeBlockPtr(resultCollector)] (uint64_t numberOfReplacements, auto error) {
        collector(error == WebKit::CallbackBase::Error::None ? numberOfReplacements : 0);
    });
}

- (void)findMatchesForString:(NSString *)targetString relativeToMatch:(id <NSTextFinderAsynchronousDocumentFindMatch>)relativeMatch findOptions:(NSTextFinderAsynchronousDocumentFindOptions)findOptions maxResults:(NSUInteger)maxResults resultCollector:(void (^)(NSArray *matches, BOOL didWrap))resultCollector
{
    // Limit the number of results, for performance reasons; NSTextFinder always
    // passes either 1 or NSUIntegerMax, which results in search time being
    // proportional to document length.
    maxResults = std::min(maxResults, maximumFindMatches);

    unsigned kitFindOptions = 0;

    if (findOptions & NSTextFinderAsynchronousDocumentFindOptionsBackwards)
        kitFindOptions |= WebKit::FindOptionsBackwards;
    if (findOptions & NSTextFinderAsynchronousDocumentFindOptionsWrap)
        kitFindOptions |= WebKit::FindOptionsWrapAround;
    if (findOptions & NSTextFinderAsynchronousDocumentFindOptionsCaseInsensitive)
        kitFindOptions |= WebKit::FindOptionsCaseInsensitive;
    if (findOptions & NSTextFinderAsynchronousDocumentFindOptionsStartsWith)
        kitFindOptions |= WebKit::FindOptionsAtWordStarts;

    RetainPtr<NSProgress> progress = [NSProgress progressWithTotalUnitCount:1];
    auto copiedResultCollector = Block_copy(resultCollector);
    _findReplyCallbacks.append([progress, copiedResultCollector] (NSArray *matches, bool didWrap) {
        [progress setCompletedUnitCount:1];
        copiedResultCollector(matches, didWrap);
        Block_release(copiedResultCollector);
    });

    if (maxResults == 1)
        _page->findString(targetString, static_cast<WebKit::FindOptions>(kitFindOptions), maxResults);
    else
        _page->findStringMatches(targetString, static_cast<WebKit::FindOptions>(kitFindOptions), maxResults);
}

- (void)getSelectedText:(void (^)(NSString *selectedTextString))completionHandler
{
    void (^copiedCompletionHandler)(NSString *) = Block_copy(completionHandler);

    _page->getSelectionOrContentsAsString([copiedCompletionHandler] (const String& string, WebKit::CallbackBase::Error) {
        copiedCompletionHandler(string);
        Block_release(copiedCompletionHandler);
    });
}

- (void)selectFindMatch:(id <NSTextFinderAsynchronousDocumentFindMatch>)findMatch completionHandler:(void (^)(void))completionHandler
{
    ASSERT([findMatch isKindOfClass:[WKTextFinderMatch class]]);
    ASSERT(!completionHandler);

    WKTextFinderMatch *textFinderMatch = static_cast<WKTextFinderMatch *>(findMatch);
    _page->selectFindMatch(textFinderMatch.index);
}

#pragma mark - FindMatchesClient

static RetainPtr<NSArray> arrayFromRects(const Vector<WebCore::IntRect>& matchRects)
{
    RetainPtr<NSMutableArray> nsMatchRects = adoptNS([[NSMutableArray alloc] initWithCapacity:matchRects.size()]);
    for (auto& rect : matchRects)
        [nsMatchRects addObject:[NSValue valueWithRect:rect]];
    return nsMatchRects;
}

- (void)didFindStringMatchesWithRects:(const Vector<Vector<WebCore::IntRect>>&)rectsForMatches didWrapAround:(BOOL)didWrapAround
{
    if (_findReplyCallbacks.isEmpty())
        return;

    auto replyCallback = _findReplyCallbacks.takeFirst();
    unsigned matchCount = rectsForMatches.size();
    RetainPtr<NSMutableArray> matchObjects = adoptNS([[NSMutableArray alloc] initWithCapacity:matchCount]);
    for (unsigned i = 0; i < matchCount; i++) {
        RetainPtr<NSArray> nsMatchRects = arrayFromRects(rectsForMatches[i]);
        RetainPtr<WKTextFinderMatch> match = adoptNS([[WKTextFinderMatch alloc] initWithClient:self view:_view index:i rects:nsMatchRects.get()]);
        [matchObjects addObject:match.get()];
    }

    replyCallback(matchObjects.get(), didWrapAround);
}

- (void)didGetImageForMatchResult:(WebKit::WebImage *)image
{
    if (_imageReplyCallbacks.isEmpty())
        return;

    WebCore::IntSize size = image->bitmap().size();
    size.scale(1 / _page->deviceScaleFactor());

    auto imageCallback = _imageReplyCallbacks.takeFirst();
    imageCallback(adoptNS([[NSImage alloc] initWithCGImage:image->bitmap().makeCGImage().get() size:size]).get());
}

#pragma mark - WKTextFinderMatch callbacks

- (void)getImageForMatchResult:(id <NSTextFinderAsynchronousDocumentFindMatch>)findMatch completionHandler:(void (^)(NSImage *generatedImage))completionHandler
{
    ASSERT([findMatch isKindOfClass:[WKTextFinderMatch class]]);

    WKTextFinderMatch *textFinderMatch = static_cast<WKTextFinderMatch *>(findMatch);

    auto copiedImageCallback = Block_copy(completionHandler);
    _imageReplyCallbacks.append([copiedImageCallback] (NSImage *image) {
        copiedImageCallback(image);
        Block_release(copiedImageCallback);
    });

    // FIXME: There is no guarantee that this will ever result in didGetImageForMatchResult
    // being called (and thus us calling our completion handler); we should harden this
    // against all of the early returns in FindController::getImageForFindMatch.
    _page->getImageForFindMatch(textFinderMatch.index);
}

@end

#endif // PLATFORM(MAC)
