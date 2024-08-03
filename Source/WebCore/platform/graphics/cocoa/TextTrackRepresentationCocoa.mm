/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#if (PLATFORM(IOS) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))) && ENABLE(VIDEO_TRACK)
#include "TextTrackRepresentationCocoa.h"

#include "FloatRect.h"
#include "GraphicsContextCG.h"
#include "IntRect.h"

#import <WebCore/QuartzCoreSPI.h>

#if PLATFORM(IOS)
#include "WebCoreThread.h"
#include "WebCoreThreadRun.h"
#endif

using namespace WebCore;

@interface WebCoreTextTrackRepresentationCocoaHelper : NSObject <CALayerDelegate> {
    TextTrackRepresentationCocoa* _parent;
}
- (id)initWithParent:(TextTrackRepresentationCocoa*)parent;
@property (assign) TextTrackRepresentationCocoa* parent;
@end

@implementation WebCoreTextTrackRepresentationCocoaHelper
- (id)initWithParent:(TextTrackRepresentationCocoa*)parent
{
    if (!(self = [super init]))
        return nil;

    self.parent = parent;

    return self;
}

- (void)dealloc
{
    self.parent = nullptr;
    [super dealloc];
}

- (void)setParent:(TextTrackRepresentationCocoa*)parent
{
    if (_parent)
        [_parent->platformLayer() removeObserver:self forKeyPath:@"bounds"];

    _parent = parent;

    if (_parent)
        [_parent->platformLayer() addObserver:self forKeyPath:@"bounds" options:0 context:0];
}

- (TextTrackRepresentationCocoa*)parent
{
    return _parent;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    UNUSED_PARAM(change);
    UNUSED_PARAM(context);
#if PLATFORM(IOS)
    WebThreadRun(^{
        if (_parent && [keyPath isEqual:@"bounds"] && object == _parent->platformLayer())
            _parent->client().textTrackRepresentationBoundsChanged(_parent->bounds());
    });
#else
    if (_parent && [keyPath isEqual:@"bounds"] && object == _parent->platformLayer())
        _parent->client().textTrackRepresentationBoundsChanged(_parent->bounds());
#endif
}

- (id)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    UNUSED_PARAM(layer);
    UNUSED_PARAM(event);
    // Returning a NSNull from this delegate method disables all implicit CALayer actions.
    return [NSNull null];
}

@end

std::unique_ptr<TextTrackRepresentation> TextTrackRepresentation::create(TextTrackRepresentationClient& client)
{
    return std::make_unique<TextTrackRepresentationCocoa>(client);
}

TextTrackRepresentationCocoa::TextTrackRepresentationCocoa(TextTrackRepresentationClient& client)
    : m_client(client)
    , m_layer(adoptNS([[CALayer alloc] init]))
    , m_delegate(adoptNS([[WebCoreTextTrackRepresentationCocoaHelper alloc] initWithParent:this]))
{
    [m_layer.get() setDelegate:m_delegate.get()];
    [m_layer.get() setContentsGravity:kCAGravityBottom];
}

TextTrackRepresentationCocoa::~TextTrackRepresentationCocoa()
{
    [m_layer.get() setDelegate:nil];
    [m_delegate.get() setParent:nullptr];
}

void TextTrackRepresentationCocoa::update()
{
    if (auto representation = m_client.createTextTrackRepresentationImage())
        [m_layer.get() setContents:(id)representation->getCGImageRef()];
}

void TextTrackRepresentationCocoa::setContentScale(float scale)
{
    [m_layer.get() setContentsScale:scale];
}

IntRect TextTrackRepresentationCocoa::bounds() const
{
    return enclosingIntRect(FloatRect([m_layer.get() bounds]));
}

#endif // (PLATFORM(IOS) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))) && ENABLE(VIDEO_TRACK)
