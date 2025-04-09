/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#import "config.h"
#import "LocalSampleBufferDisplayLayer.h"

#if ENABLE(MEDIA_STREAM) && USE(AVFOUNDATION)

#import "Color.h"
#import "IntSize.h"
#import "MediaSample.h"

#import <AVFoundation/AVSampleBufferDisplayLayer.h>
#import <QuartzCore/CALayer.h>
#import <QuartzCore/CATransaction.h>

#import <wtf/MainThread.h>
#import <wtf/cf/TypeCastsCF.h>

#import <pal/cocoa/AVFoundationSoftLink.h>

using namespace WebCore;

@interface WebAVSampleBufferStatusChangeListener : NSObject {
    WeakPtr<LocalSampleBufferDisplayLayer> _parent;
}

- (id)initWithParent:(LocalSampleBufferDisplayLayer*)callback;
- (void)invalidate;
- (void)begin;
- (void)stop;
@end

@implementation WebAVSampleBufferStatusChangeListener

- (id)initWithParent:(LocalSampleBufferDisplayLayer*)parent
{
    if (!(self = [super init]))
        return nil;

    _parent = makeWeakPtr(parent);

    return self;
}

- (void)dealloc
{
    [self invalidate];
    [super dealloc];
}

- (void)invalidate
{
    [self stop];
    _parent = nullptr;
}

- (void)begin
{
    ASSERT(_parent);
    ASSERT(_parent->displayLayer());
    ASSERT(_parent->rootLayer());

    [_parent->displayLayer() addObserver:self forKeyPath:@"status" options:NSKeyValueObservingOptionNew context:nil];
    [_parent->displayLayer() addObserver:self forKeyPath:@"error" options:NSKeyValueObservingOptionNew context:nil];
}

- (void)stop
{
    if (!_parent)
        return;

    if (_parent->displayLayer()) {
        [_parent->displayLayer() removeObserver:self forKeyPath:@"status"];
        [_parent->displayLayer() removeObserver:self forKeyPath:@"error"];
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(keyPath);
    UNUSED_PARAM(change);
    ASSERT(_parent);

    if (!_parent)
        return;

    if ([object isKindOfClass:PAL::getAVSampleBufferDisplayLayerClass()]) {
        ASSERT(object == _parent->displayLayer());

        if ([keyPath isEqualToString:@"status"]) {
            callOnMainThread([protectedSelf = RetainPtr<WebAVSampleBufferStatusChangeListener>(self)] {
                if (!protectedSelf->_parent)
                    return;

                protectedSelf->_parent->layerStatusDidChange();
            });
            return;
        }

        if ([keyPath isEqualToString:@"error"]) {
            callOnMainThread([protectedSelf = RetainPtr<WebAVSampleBufferStatusChangeListener>(self)] {
                if (!protectedSelf->_parent)
                    return;

                protectedSelf->_parent->layerErrorDidChange();
            });
            return;
        }
    }
}
@end

namespace WebCore {

static void runWithoutAnimations(const WTF::Function<void()>& function)
{
    [CATransaction begin];
    [CATransaction setAnimationDuration:0];
    [CATransaction setDisableActions:YES];
    function();
    [CATransaction commit];
}

std::unique_ptr<LocalSampleBufferDisplayLayer> LocalSampleBufferDisplayLayer::create(Client& client, bool hideRootLayer, IntSize size)
{
    auto sampleBufferDisplayLayer = adoptNS([PAL::allocAVSampleBufferDisplayLayerInstance() init]);
    if (!sampleBufferDisplayLayer)
        return nullptr;

    return makeUnique<LocalSampleBufferDisplayLayer>(WTFMove(sampleBufferDisplayLayer), client, hideRootLayer, size);
}

LocalSampleBufferDisplayLayer::LocalSampleBufferDisplayLayer(RetainPtr<AVSampleBufferDisplayLayer>&& sampleBufferDisplayLayer, Client& client, bool hideRootLayer, IntSize size)
    : SampleBufferDisplayLayer(client)
    , m_statusChangeListener(adoptNS([[WebAVSampleBufferStatusChangeListener alloc] initWithParent:this]))
    , m_sampleBufferDisplayLayer(WTFMove(sampleBufferDisplayLayer))
{
    m_sampleBufferDisplayLayer.get().backgroundColor = cachedCGColor(Color::black);
    m_sampleBufferDisplayLayer.get().anchorPoint = { .5, .5 };
    m_sampleBufferDisplayLayer.get().needsDisplayOnBoundsChange = YES;
    m_sampleBufferDisplayLayer.get().videoGravity = AVLayerVideoGravityResizeAspectFill;

    m_rootLayer = adoptNS([[CALayer alloc] init]);
    m_rootLayer.get().hidden = hideRootLayer;

    m_rootLayer.get().backgroundColor = cachedCGColor(Color::black);
    m_rootLayer.get().needsDisplayOnBoundsChange = YES;

    m_rootLayer.get().bounds = CGRectMake(0, 0, size.width(), size.height());

    [m_statusChangeListener begin];

    [m_rootLayer addSublayer:m_sampleBufferDisplayLayer.get()];

#ifndef NDEBUG
    [m_sampleBufferDisplayLayer setName:@"LocalSampleBufferDisplayLayer AVSampleBufferDisplayLayer"];
    [m_rootLayer setName:@"LocalSampleBufferDisplayLayer AVSampleBufferDisplayLayer parent"];
#endif
}

LocalSampleBufferDisplayLayer::~LocalSampleBufferDisplayLayer()
{
    [m_statusChangeListener stop];

    m_pendingVideoSampleQueue.clear();

    [m_sampleBufferDisplayLayer stopRequestingMediaData];
    [m_sampleBufferDisplayLayer flush];
    m_sampleBufferDisplayLayer = nullptr;

    m_rootLayer = nullptr;
}

void LocalSampleBufferDisplayLayer::layerStatusDidChange()
{
    ASSERT(isMainThread());
    if (m_sampleBufferDisplayLayer.get().status != AVQueuedSampleBufferRenderingStatusRendering)
        return;
    if (!m_client)
        return;
    m_client->sampleBufferDisplayLayerStatusDidChange(*this);
}

void LocalSampleBufferDisplayLayer::layerErrorDidChange()
{
    ASSERT(isMainThread());
    // FIXME: Log error.
}

PlatformLayer* LocalSampleBufferDisplayLayer::displayLayer()
{
    return m_sampleBufferDisplayLayer.get();
}

PlatformLayer* LocalSampleBufferDisplayLayer::rootLayer()
{
    return m_rootLayer.get();
}

bool LocalSampleBufferDisplayLayer::didFail() const
{
    return [m_sampleBufferDisplayLayer status] == AVQueuedSampleBufferRenderingStatusFailed;
}

void LocalSampleBufferDisplayLayer::updateDisplayMode(bool hideDisplayLayer, bool hideRootLayer)
{
    if (m_rootLayer.get().hidden == hideRootLayer && m_sampleBufferDisplayLayer.get().hidden == hideDisplayLayer)
        return;

    runWithoutAnimations([&] {
        m_sampleBufferDisplayLayer.get().hidden = hideDisplayLayer;
        m_rootLayer.get().hidden = hideRootLayer;
    });
}

CGRect LocalSampleBufferDisplayLayer::bounds() const
{
    return m_rootLayer.get().bounds;
}

void LocalSampleBufferDisplayLayer::updateAffineTransform(CGAffineTransform transform)
{
    runWithoutAnimations([&] {
        m_sampleBufferDisplayLayer.get().affineTransform = transform;
    });
}

void LocalSampleBufferDisplayLayer::updateBoundsAndPosition(CGRect videoBounds, CGPoint position)
{
    runWithoutAnimations([&] {
        m_sampleBufferDisplayLayer.get().bounds = videoBounds;
        m_sampleBufferDisplayLayer.get().position = position;
    });
}

void LocalSampleBufferDisplayLayer::updateRootLayerBoundsAndPosition(CGRect videoBounds, CGPoint position)
{
    runWithoutAnimations([&] {
        m_rootLayer.get().bounds = videoBounds;
        m_rootLayer.get().position = position;
        m_sampleBufferDisplayLayer.get().bounds = videoBounds;
        m_sampleBufferDisplayLayer.get().position = position;
    });
}

void LocalSampleBufferDisplayLayer::flush()
{
    [m_sampleBufferDisplayLayer flush];
}

void LocalSampleBufferDisplayLayer::flushAndRemoveImage()
{
    [m_sampleBufferDisplayLayer flushAndRemoveImage];
}

void LocalSampleBufferDisplayLayer::enqueueSample(MediaSample& sample)
{
    if (![m_sampleBufferDisplayLayer isReadyForMoreMediaData]) {
        addSampleToPendingQueue(sample);
        requestNotificationWhenReadyForVideoData();
        return;
    }

    [m_sampleBufferDisplayLayer enqueueSampleBuffer:sample.platformSample().sample.cmSampleBuffer];
}

void LocalSampleBufferDisplayLayer::removeOldSamplesFromPendingQueue()
{
    if (m_pendingVideoSampleQueue.isEmpty() || !m_client)
        return;

    auto decodeTime = m_pendingVideoSampleQueue.first()->decodeTime();
    if (!decodeTime.isValid() || decodeTime < MediaTime::zeroTime()) {
        while (m_pendingVideoSampleQueue.size() > 5)
            m_pendingVideoSampleQueue.removeFirst();

        return;
    }

    MediaTime now = m_client->streamTime();
    while (!m_pendingVideoSampleQueue.isEmpty()) {
        if (m_pendingVideoSampleQueue.first()->decodeTime() > now)
            break;
        m_pendingVideoSampleQueue.removeFirst();
    }
}

void LocalSampleBufferDisplayLayer::addSampleToPendingQueue(MediaSample& sample)
{
    removeOldSamplesFromPendingQueue();
    m_pendingVideoSampleQueue.append(sample);
}

void LocalSampleBufferDisplayLayer::clearEnqueuedSamples()
{
    m_pendingVideoSampleQueue.clear();
}

void LocalSampleBufferDisplayLayer::requestNotificationWhenReadyForVideoData()
{
    auto weakThis = makeWeakPtr(*this);
    [m_sampleBufferDisplayLayer requestMediaDataWhenReadyOnQueue:dispatch_get_main_queue() usingBlock:^{
        if (!weakThis)
            return;

        [m_sampleBufferDisplayLayer stopRequestingMediaData];

        while (!m_pendingVideoSampleQueue.isEmpty()) {
            if (![m_sampleBufferDisplayLayer isReadyForMoreMediaData]) {
                requestNotificationWhenReadyForVideoData();
                return;
            }

            auto sample = m_pendingVideoSampleQueue.takeFirst();
            enqueueSample(sample.get());
        }
    }];
}

}

#endif
