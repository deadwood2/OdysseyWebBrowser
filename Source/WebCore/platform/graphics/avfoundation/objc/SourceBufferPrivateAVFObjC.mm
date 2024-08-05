/*
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
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
#import "SourceBufferPrivateAVFObjC.h"

#if ENABLE(MEDIA_SOURCE) && USE(AVFOUNDATION)

#import "AVAssetTrackUtilities.h"
#import "AVFoundationSPI.h"
#import "AudioTrackPrivateMediaSourceAVFObjC.h"
#import "CDMSessionAVContentKeySession.h"
#import "CDMSessionMediaSourceAVFObjC.h"
#import "InbandTextTrackPrivateAVFObjC.h"
#import "Logging.h"
#import "MediaDescription.h"
#import "MediaPlayerPrivateMediaSourceAVFObjC.h"
#import "MediaSample.h"
#import "MediaSampleAVFObjC.h"
#import "MediaSourcePrivateAVFObjC.h"
#import "MediaTimeAVFoundation.h"
#import "NotImplemented.h"
#import "SourceBufferPrivateClient.h"
#import "TimeRanges.h"
#import "VideoTrackPrivateMediaSourceAVFObjC.h"
#import "WebCoreDecompressionSession.h"
#import <AVFoundation/AVAssetTrack.h>
#import <QuartzCore/CALayer.h>
#import <map>
#import <objc/runtime.h>
#import <runtime/TypedArrayInlines.h>
#import <wtf/BlockObjCExceptions.h>
#import <wtf/HashCountedSet.h>
#import <wtf/MainThread.h>
#import <wtf/SoftLinking.h>
#import <wtf/WeakPtr.h>
#import <wtf/text/AtomicString.h>
#import <wtf/text/CString.h>

#pragma mark - Soft Linking

#import "CoreMediaSoftLink.h"

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)

SOFT_LINK_CLASS(AVFoundation, AVAssetTrack)
SOFT_LINK_CLASS(AVFoundation, AVStreamDataParser)
SOFT_LINK_CLASS(AVFoundation, AVSampleBufferAudioRenderer)
SOFT_LINK_CLASS(AVFoundation, AVSampleBufferDisplayLayer)
SOFT_LINK_CLASS(AVFoundation, AVStreamSession)

SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVMediaTypeVideo, NSString *)
SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVMediaTypeAudio, NSString *)
SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVMediaTypeText, NSString *)

SOFT_LINK_CONSTANT(AVFoundation, AVMediaCharacteristicVisual, NSString*)
SOFT_LINK_CONSTANT(AVFoundation, AVMediaCharacteristicAudible, NSString*)
SOFT_LINK_CONSTANT(AVFoundation, AVMediaCharacteristicLegible, NSString*)
SOFT_LINK_CONSTANT(AVFoundation, AVSampleBufferDisplayLayerFailedToDecodeNotification, NSString*)
SOFT_LINK_CONSTANT(AVFoundation, AVSampleBufferDisplayLayerFailedToDecodeNotificationErrorKey, NSString*)

#define AVMediaTypeVideo getAVMediaTypeVideo()
#define AVMediaTypeAudio getAVMediaTypeAudio()
#define AVMediaTypeText getAVMediaTypeText()
#define AVSampleBufferDisplayLayerFailedToDecodeNotification getAVSampleBufferDisplayLayerFailedToDecodeNotification()
#define AVSampleBufferDisplayLayerFailedToDecodeNotificationErrorKey getAVSampleBufferDisplayLayerFailedToDecodeNotificationErrorKey()

#define AVMediaCharacteristicVisual getAVMediaCharacteristicVisual()
#define AVMediaCharacteristicAudible getAVMediaCharacteristicAudible()
#define AVMediaCharacteristicLegible getAVMediaCharacteristicLegible()

#pragma mark -
#pragma mark AVStreamSession

@interface AVStreamSession : NSObject
- (void)addStreamDataParser:(AVStreamDataParser *)streamDataParser;
- (void)removeStreamDataParser:(AVStreamDataParser *)streamDataParser;
@end

#pragma mark -
#pragma mark WebAVStreamDataParserListener

@interface WebAVStreamDataParserListener : NSObject<AVStreamDataParserOutputHandling> {
    WeakPtr<WebCore::SourceBufferPrivateAVFObjC> _parent;
    AVStreamDataParser* _parser;
}
@property (assign) WeakPtr<WebCore::SourceBufferPrivateAVFObjC> parent;
- (id)initWithParser:(AVStreamDataParser*)parser parent:(WeakPtr<WebCore::SourceBufferPrivateAVFObjC>)parent;
@end

@implementation WebAVStreamDataParserListener
- (id)initWithParser:(AVStreamDataParser*)parser parent:(WeakPtr<WebCore::SourceBufferPrivateAVFObjC>)parent
{
    self = [super init];
    if (!self)
        return nil;

    ASSERT(parent);
    _parent = parent;
    _parser = parser;
    [_parser setDelegate:self];
    return self;
}

@synthesize parent=_parent;

- (void)dealloc
{
    [_parser setDelegate:nil];
    [super dealloc];
}

- (void)invalidate
{
    [_parser setDelegate:nil];
    _parser = nullptr;
}

- (void)streamDataParser:(AVStreamDataParser *)streamDataParser didParseStreamDataAsAsset:(AVAsset *)asset
{
    ASSERT_UNUSED(streamDataParser, streamDataParser == _parser);

    RetainPtr<AVAsset*> protectedAsset = asset;
    callOnMainThread([parent = _parent, protectedAsset = WTFMove(protectedAsset)] {
        if (parent)
            parent->didParseStreamDataAsAsset(protectedAsset.get());
    });
}

- (void)streamDataParser:(AVStreamDataParser *)streamDataParser didParseStreamDataAsAsset:(AVAsset *)asset withDiscontinuity:(BOOL)discontinuity
{
    UNUSED_PARAM(discontinuity);
    ASSERT_UNUSED(streamDataParser, streamDataParser == _parser);

    RetainPtr<AVAsset*> protectedAsset = asset;
    callOnMainThread([parent = _parent, protectedAsset = WTFMove(protectedAsset)] {
        if (parent)
            parent->didParseStreamDataAsAsset(protectedAsset.get());
    });
}

- (void)streamDataParser:(AVStreamDataParser *)streamDataParser didFailToParseStreamDataWithError:(NSError *)error
{
    ASSERT_UNUSED(streamDataParser, streamDataParser == _parser);

    RetainPtr<NSError> protectedError = error;
    callOnMainThread([parent = _parent, protectedError = WTFMove(protectedError)] {
        if (parent)
            parent->didFailToParseStreamDataWithError(protectedError.get());
    });
}

- (void)streamDataParser:(AVStreamDataParser *)streamDataParser didProvideMediaData:(CMSampleBufferRef)sample forTrackID:(CMPersistentTrackID)trackID mediaType:(NSString *)nsMediaType flags:(AVStreamDataParserOutputMediaDataFlags)flags
{
    ASSERT_UNUSED(streamDataParser, streamDataParser == _parser);

    RetainPtr<CMSampleBufferRef> protectedSample = sample;
    callOnMainThread([parent = _parent, protectedSample = WTFMove(protectedSample), trackID, mediaType = String(nsMediaType), flags] {
        if (parent)
            parent->didProvideMediaDataForTrackID(trackID, protectedSample.get(), mediaType, flags);
    });
}

- (void)streamDataParser:(AVStreamDataParser *)streamDataParser didReachEndOfTrackWithTrackID:(CMPersistentTrackID)trackID mediaType:(NSString *)nsMediaType
{
    ASSERT_UNUSED(streamDataParser, streamDataParser == _parser);

    callOnMainThread([parent = _parent, trackID, mediaType = String(nsMediaType)] {
        if (parent)
            parent->didReachEndOfTrackWithTrackID(trackID, mediaType);
    });
}

- (void)streamDataParserWillProvideContentKeyRequestInitializationData:(AVStreamDataParser *)streamDataParser forTrackID:(CMPersistentTrackID)trackID
{
    ASSERT_UNUSED(streamDataParser, streamDataParser == _parser);

    // We must call synchronously to the main thread, as the AVStreamSession must be associated
    // with the streamDataParser before the delegate method returns.
    dispatch_sync(dispatch_get_main_queue(), [parent = _parent, trackID]() {
        if (parent)
            parent->willProvideContentKeyRequestInitializationDataForTrackID(trackID);
    });
}

- (void)streamDataParser:(AVStreamDataParser *)streamDataParser didProvideContentKeyRequestInitializationData:(NSData *)initData forTrackID:(CMPersistentTrackID)trackID
{
    ASSERT_UNUSED(streamDataParser, streamDataParser == _parser);

    OSObjectPtr<dispatch_semaphore_t> hasSessionSemaphore = adoptOSObject(dispatch_semaphore_create(0));
    callOnMainThread([parent = _parent, protectedInitData = RetainPtr<NSData>(initData), trackID, hasSessionSemaphore] {
        if (parent)
            parent->didProvideContentKeyRequestInitializationDataForTrackID(protectedInitData.get(), trackID, hasSessionSemaphore);
    });
    dispatch_semaphore_wait(hasSessionSemaphore.get(), DISPATCH_TIME_FOREVER);
}
@end

@interface WebAVSampleBufferErrorListener : NSObject {
    WebCore::SourceBufferPrivateAVFObjC* _parent;
    Vector<RetainPtr<AVSampleBufferDisplayLayer>> _layers;
    Vector<RetainPtr<AVSampleBufferAudioRenderer>> _renderers;
}

- (id)initWithParent:(WebCore::SourceBufferPrivateAVFObjC*)parent;
- (void)invalidate;
- (void)beginObservingLayer:(AVSampleBufferDisplayLayer *)layer;
- (void)stopObservingLayer:(AVSampleBufferDisplayLayer *)layer;
- (void)beginObservingRenderer:(AVSampleBufferAudioRenderer *)renderer;
- (void)stopObservingRenderer:(AVSampleBufferAudioRenderer *)renderer;
@end

@implementation WebAVSampleBufferErrorListener

- (id)initWithParent:(WebCore::SourceBufferPrivateAVFObjC*)parent
{
    if (!(self = [super init]))
        return nil;

    _parent = parent;
    return self;
}

- (void)dealloc
{
    [self invalidate];
    [super dealloc];
}

- (void)invalidate
{
    if (!_parent && !_layers.size() && !_renderers.size())
        return;

    for (auto& layer : _layers) {
        [layer removeObserver:self forKeyPath:@"error"];
        [layer removeObserver:self forKeyPath:@"outputObscuredDueToInsufficientExternalProtection"];
    }
    _layers.clear();

    for (auto& renderer : _renderers)
        [renderer removeObserver:self forKeyPath:@"error"];
    _renderers.clear();

    [[NSNotificationCenter defaultCenter] removeObserver:self];

    _parent = nullptr;
}

- (void)beginObservingLayer:(AVSampleBufferDisplayLayer*)layer
{
    ASSERT(_parent);
    ASSERT(!_layers.contains(layer));

    _layers.append(layer);
    [layer addObserver:self forKeyPath:@"error" options:NSKeyValueObservingOptionNew context:nullptr];
    [layer addObserver:self forKeyPath:@"outputObscuredDueToInsufficientExternalProtection" options:NSKeyValueObservingOptionNew context:nullptr];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(layerFailedToDecode:) name:AVSampleBufferDisplayLayerFailedToDecodeNotification object:layer];
}

- (void)stopObservingLayer:(AVSampleBufferDisplayLayer*)layer
{
    ASSERT(_parent);
    ASSERT(_layers.contains(layer));

    [layer removeObserver:self forKeyPath:@"error"];
    [layer removeObserver:self forKeyPath:@"outputObscuredDueToInsufficientExternalProtection"];
    _layers.remove(_layers.find(layer));

    [[NSNotificationCenter defaultCenter] removeObserver:self name:AVSampleBufferDisplayLayerFailedToDecodeNotification object:layer];
}

- (void)beginObservingRenderer:(AVSampleBufferAudioRenderer*)renderer
{
    ASSERT(_parent);
    ASSERT(!_renderers.contains(renderer));

    _renderers.append(renderer);
    [renderer addObserver:self forKeyPath:@"error" options:NSKeyValueObservingOptionNew context:nullptr];
}

- (void)stopObservingRenderer:(AVSampleBufferAudioRenderer*)renderer
{
    ASSERT(_parent);
    ASSERT(_renderers.contains(renderer));

    [renderer removeObserver:self forKeyPath:@"error"];
    _renderers.remove(_renderers.find(renderer));
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(keyPath);
    ASSERT(_parent);

    RetainPtr<WebAVSampleBufferErrorListener> protectedSelf = self;
    if ([object isKindOfClass:getAVSampleBufferDisplayLayerClass()]) {
        RetainPtr<AVSampleBufferDisplayLayer> layer = (AVSampleBufferDisplayLayer *)object;
        ASSERT(_layers.contains(layer.get()));

        if ([keyPath isEqualTo:@"error"]) {
            RetainPtr<NSError> error = [change valueForKey:NSKeyValueChangeNewKey];
            callOnMainThread([protectedSelf = WTFMove(protectedSelf), layer = WTFMove(layer), error = WTFMove(error)] {
                protectedSelf->_parent->layerDidReceiveError(layer.get(), error.get());
            });
        } else if ([keyPath isEqualTo:@"outputObscuredDueToInsufficientExternalProtection"]) {
            if ([[change valueForKey:NSKeyValueChangeNewKey] boolValue]) {
                RetainPtr<NSError> error = [NSError errorWithDomain:@"com.apple.WebKit" code:'HDCP' userInfo:nil];
                callOnMainThread([protectedSelf = WTFMove(protectedSelf), layer = WTFMove(layer), error = WTFMove(error)] {
                    protectedSelf->_parent->layerDidReceiveError(layer.get(), error.get());
                });
            }
        } else
            ASSERT_NOT_REACHED();

    } else if ([object isKindOfClass:getAVSampleBufferAudioRendererClass()]) {
        RetainPtr<AVSampleBufferAudioRenderer> renderer = (AVSampleBufferAudioRenderer *)object;
        RetainPtr<NSError> error = [change valueForKey:NSKeyValueChangeNewKey];

        ASSERT(_renderers.contains(renderer.get()));
        ASSERT([keyPath isEqualTo:@"error"]);

        callOnMainThread([protectedSelf = WTFMove(protectedSelf), renderer = WTFMove(renderer), error = WTFMove(error)] {
            protectedSelf->_parent->rendererDidReceiveError(renderer.get(), error.get());
        });
    } else
        ASSERT_NOT_REACHED();
}

- (void)layerFailedToDecode:(NSNotification*)note
{
    RetainPtr<AVSampleBufferDisplayLayer> layer = (AVSampleBufferDisplayLayer *)[note object];
    RetainPtr<NSError> error = [[note userInfo] valueForKey:AVSampleBufferDisplayLayerFailedToDecodeNotificationErrorKey];

    RetainPtr<WebAVSampleBufferErrorListener> protectedSelf = self;
    callOnMainThread([protectedSelf = WTFMove(protectedSelf), layer = WTFMove(layer), error = WTFMove(error)] {
        if (!protectedSelf->_parent || !protectedSelf->_layers.contains(layer.get()))
            return;
        protectedSelf->_parent->layerDidReceiveError(layer.get(), error.get());
    });
}
@end

#pragma mark -

@interface WebBufferConsumedContext : NSObject {
    WeakPtr<WebCore::SourceBufferPrivateAVFObjC> _parent;
}
@property (readonly) WebCore::SourceBufferPrivateAVFObjC* parent;
@end

@implementation WebBufferConsumedContext
- (id)initWithParent:(WeakPtr<WebCore::SourceBufferPrivateAVFObjC>)parent
{
    self = [super init];
    if (self)
        _parent = parent;
    return self;
}

@dynamic parent;
- (WebCore::SourceBufferPrivateAVFObjC*)parent
{
    return _parent.get();
}
@end

namespace WebCore {

#pragma mark -
#pragma mark MediaDescriptionAVFObjC

class MediaDescriptionAVFObjC final : public MediaDescription {
public:
    static RefPtr<MediaDescriptionAVFObjC> create(AVAssetTrack* track) { return adoptRef(new MediaDescriptionAVFObjC(track)); }
    virtual ~MediaDescriptionAVFObjC() { }

    AtomicString codec() const override { return m_codec; }
    bool isVideo() const override { return m_isVideo; }
    bool isAudio() const override { return m_isAudio; }
    bool isText() const override { return m_isText; }
    
protected:
    MediaDescriptionAVFObjC(AVAssetTrack* track)
        : m_isVideo([track hasMediaCharacteristic:AVMediaCharacteristicVisual])
        , m_isAudio([track hasMediaCharacteristic:AVMediaCharacteristicAudible])
        , m_isText([track hasMediaCharacteristic:AVMediaCharacteristicLegible])
    {
        NSArray* formatDescriptions = [track formatDescriptions];
        CMFormatDescriptionRef description = [formatDescriptions count] ? (CMFormatDescriptionRef)[formatDescriptions objectAtIndex:0] : 0;
        if (description) {
            FourCharCode codec = CMFormatDescriptionGetMediaSubType(description);
            m_codec = AtomicString(reinterpret_cast<LChar*>(&codec), 4);
        }
    }

    AtomicString m_codec;
    bool m_isVideo;
    bool m_isAudio;
    bool m_isText;
};

#pragma mark -
#pragma mark SourceBufferPrivateAVFObjC

static NSString *kBufferConsumedContext = @"BufferConsumedContext";

static void bufferWasConsumedCallback(CMNotificationCenterRef, const void*, CFStringRef notificationName, const void*, CFTypeRef payload)
{
    if (!isMainThread()) {
        callOnMainThread([notificationName, payload = retainPtr(payload)] {
            bufferWasConsumedCallback(nullptr, nullptr, notificationName, nullptr, payload.get());
        });
        return;
    }

    if (!CFEqual(kCMSampleBufferConsumerNotification_BufferConsumed, notificationName))
        return;

    ASSERT(CFGetTypeID(payload) == CFDictionaryGetTypeID());
    auto context = (WebBufferConsumedContext *)[(NSDictionary *)payload valueForKey:kBufferConsumedContext];
    if (!context)
        return;

    if (auto sourceBuffer = context.parent)
        sourceBuffer->bufferWasConsumed();
}

RefPtr<SourceBufferPrivateAVFObjC> SourceBufferPrivateAVFObjC::create(MediaSourcePrivateAVFObjC* parent)
{
    return adoptRef(new SourceBufferPrivateAVFObjC(parent));
}

SourceBufferPrivateAVFObjC::SourceBufferPrivateAVFObjC(MediaSourcePrivateAVFObjC* parent)
    : m_weakFactory(this)
    , m_appendWeakFactory(this)
    , m_parser(adoptNS([allocAVStreamDataParserInstance() init]))
    , m_delegate(adoptNS([[WebAVStreamDataParserListener alloc] initWithParser:m_parser.get() parent:createWeakPtr()]))
    , m_errorListener(adoptNS([[WebAVSampleBufferErrorListener alloc] initWithParent:this]))
    , m_isAppendingGroup(adoptOSObject(dispatch_group_create()))
    , m_mediaSource(parent)
{
    CMNotificationCenterAddListener(CMNotificationCenterGetDefaultLocalCenter(), this, bufferWasConsumedCallback, kCMSampleBufferConsumerNotification_BufferConsumed, nullptr, 0);
}

SourceBufferPrivateAVFObjC::~SourceBufferPrivateAVFObjC()
{
    ASSERT(!m_client);
    destroyParser();
    destroyRenderers();

    CMNotificationCenterRemoveListener(CMNotificationCenterGetDefaultLocalCenter(), this, bufferWasConsumedCallback, kCMSampleBufferConsumerNotification_BufferConsumed, nullptr);

    if (m_hasSessionSemaphore)
        dispatch_semaphore_signal(m_hasSessionSemaphore.get());
}

void SourceBufferPrivateAVFObjC::didParseStreamDataAsAsset(AVAsset* asset)
{
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::didParseStreamDataAsAsset(%p)", this);

    if (!m_mediaSource)
        return;

    if (m_mediaSource->player()->shouldCheckHardwareSupport()) {
        for (AVAssetTrack *track in [asset tracks]) {
            if (!assetTrackMeetsHardwareDecodeRequirements(track, m_mediaSource->player()->mediaContentTypesRequiringHardwareSupport())) {
                m_parsingSucceeded = false;
                return;
            }
        }
    }

    m_asset = asset;

    m_videoTracks.clear();
    m_audioTracks.clear();

    SourceBufferPrivateClient::InitializationSegment segment;

    if ([m_asset respondsToSelector:@selector(overallDurationHint)])
        segment.duration = toMediaTime([m_asset overallDurationHint]);

    if (segment.duration.isInvalid() || segment.duration == MediaTime::zeroTime())
        segment.duration = toMediaTime([m_asset duration]);

    for (AVAssetTrack* track in [m_asset tracks]) {
        if ([track hasMediaCharacteristic:AVMediaCharacteristicLegible]) {
            // FIXME(125161): Handle in-band text tracks.
            continue;
        }

        if ([track hasMediaCharacteristic:AVMediaCharacteristicVisual]) {
            SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
            RefPtr<VideoTrackPrivateMediaSourceAVFObjC> videoTrack = VideoTrackPrivateMediaSourceAVFObjC::create(track, this);
            info.track = videoTrack;
            m_videoTracks.append(videoTrack);
            info.description = MediaDescriptionAVFObjC::create(track);
            segment.videoTracks.append(info);
        } else if ([track hasMediaCharacteristic:AVMediaCharacteristicAudible]) {
            SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
            RefPtr<AudioTrackPrivateMediaSourceAVFObjC> audioTrack = AudioTrackPrivateMediaSourceAVFObjC::create(track, this);
            info.track = audioTrack;
            m_audioTracks.append(audioTrack);
            info.description = MediaDescriptionAVFObjC::create(track);
            segment.audioTracks.append(info);
        }

        // FIXME(125161): Add TextTrack support
    }

    if (m_mediaSource)
        m_mediaSource->player()->characteristicsChanged();

    if (m_client)
        m_client->sourceBufferPrivateDidReceiveInitializationSegment(segment);
}

void SourceBufferPrivateAVFObjC::didFailToParseStreamDataWithError(NSError *error)
{
#if LOG_DISABLED
    UNUSED_PARAM(error);
#endif
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::didFailToParseStreamDataWithError(%p) - error:\"%s\"", this, String([error description]).utf8().data());

    m_parsingSucceeded = false;
}

struct ProcessCodedFrameInfo {
    SourceBufferPrivateAVFObjC* sourceBuffer;
    int trackID;
    const String& mediaType;
};

void SourceBufferPrivateAVFObjC::didProvideMediaDataForTrackID(int trackID, CMSampleBufferRef sampleBuffer, const String& mediaType, unsigned)
{
    processCodedFrame(trackID, sampleBuffer, mediaType);
}

bool SourceBufferPrivateAVFObjC::processCodedFrame(int trackID, CMSampleBufferRef sampleBuffer, const String&)
{
    if (trackID != m_enabledVideoTrackID && !m_audioRenderers.contains(trackID)) {
        // FIXME(125161): We don't handle text tracks, and passing this sample up to SourceBuffer
        // will just confuse its state. Drop this sample until we can handle text tracks properly.
        return false;
    }

    if (m_client) {
        Ref<MediaSample> mediaSample = MediaSampleAVFObjC::create(sampleBuffer, trackID);
        LOG(MediaSourceSamples, "SourceBufferPrivateAVFObjC::processCodedFrame(%p) - sample(%s)", this, toString(mediaSample.get()).utf8().data());
        m_client->sourceBufferPrivateDidReceiveSample(mediaSample);
    }

    return true;
}

void SourceBufferPrivateAVFObjC::didReachEndOfTrackWithTrackID(int, const String&)
{
    notImplemented();
}

void SourceBufferPrivateAVFObjC::willProvideContentKeyRequestInitializationDataForTrackID(int trackID)
{
    if (!m_mediaSource)
        return;

    ASSERT(m_parser);

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::willProvideContentKeyRequestInitializationDataForTrackID(%p) - track:%d", this, trackID);
    m_protectedTrackID = trackID;

    if (CDMSessionMediaSourceAVFObjC* session = m_mediaSource->player()->cdmSession())
        session->addParser(m_parser.get());
    else if (!CDMSessionAVContentKeySession::isAvailable()) {
        BEGIN_BLOCK_OBJC_EXCEPTIONS;
        [m_mediaSource->player()->streamSession() addStreamDataParser:m_parser.get()];
        END_BLOCK_OBJC_EXCEPTIONS;
    }
#else
    UNUSED_PARAM(trackID);
#endif
}

void SourceBufferPrivateAVFObjC::didProvideContentKeyRequestInitializationDataForTrackID(NSData* initData, int trackID, OSObjectPtr<dispatch_semaphore_t> hasSessionSemaphore)
{
    if (!m_mediaSource)
        return;

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::didProvideContentKeyRequestInitializationDataForTrackID(%p) - track:%d", this, trackID);
    m_protectedTrackID = trackID;
    RefPtr<Uint8Array> initDataArray = Uint8Array::create([initData length]);
    [initData getBytes:initDataArray->data() length:initDataArray->length()];
    m_mediaSource->sourceBufferKeyNeeded(this, initDataArray.get());
    if (auto session = m_mediaSource->player()->cdmSession()) {
        session->addParser(m_parser.get());
        dispatch_semaphore_signal(hasSessionSemaphore.get());
    } else {
        if (m_hasSessionSemaphore)
            dispatch_semaphore_signal(m_hasSessionSemaphore.get());
        m_hasSessionSemaphore = hasSessionSemaphore;
    }
#else
    UNUSED_PARAM(initData);
    UNUSED_PARAM(trackID);
    UNUSED_PARAM(hasSessionSemaphore);
#endif
}

void SourceBufferPrivateAVFObjC::setClient(SourceBufferPrivateClient* client)
{
    m_client = client;
}

static dispatch_queue_t globalDataParserQueue()
{
    static dispatch_queue_t globalQueue;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        globalQueue = dispatch_queue_create("SourceBufferPrivateAVFObjC data parser queue", DISPATCH_QUEUE_CONCURRENT);
    });
    return globalQueue;
}

void SourceBufferPrivateAVFObjC::append(const unsigned char* data, unsigned length)
{
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::append(%p) - data:%p, length:%d", this, data, length);

    RetainPtr<NSData> nsData = adoptNS([[NSData alloc] initWithBytes:data length:length]);
    WeakPtr<SourceBufferPrivateAVFObjC> weakThis = m_appendWeakFactory.createWeakPtr();
    RetainPtr<AVStreamDataParser> parser = m_parser;
    RetainPtr<WebAVStreamDataParserListener> delegate = m_delegate;

    m_parsingSucceeded = true;
    dispatch_group_enter(m_isAppendingGroup.get());

    dispatch_async(globalDataParserQueue(), [nsData, weakThis, parser, delegate, isAppendingGroup = m_isAppendingGroup, parserStateWasReset = m_parserStateWasReset] {
        if (parserStateWasReset)
            [parser appendStreamData:nsData.get() withFlags:AVStreamDataParserStreamDataDiscontinuity];
        else
            [parser appendStreamData:nsData.get()];

        callOnMainThread([weakThis] {
            if (weakThis)
                weakThis->appendCompleted();
        });
        dispatch_group_leave(isAppendingGroup.get());
    });
    m_parserStateWasReset = false;
}

void SourceBufferPrivateAVFObjC::appendCompleted()
{
    if (m_parsingSucceeded && m_mediaSource)
        m_mediaSource->player()->setLoadingProgresssed(true);

    if (m_client)
        m_client->sourceBufferPrivateAppendComplete(m_parsingSucceeded ? SourceBufferPrivateClient::AppendSucceeded : SourceBufferPrivateClient::ParsingFailed);
}

void SourceBufferPrivateAVFObjC::abort()
{
    // The parsing queue may be blocked waiting for the main thread to provide it a AVStreamSession. We
    // were asked to abort, and that cancels all outstanding append operations. Without cancelling this
    // semaphore, the m_isAppendingGroup wait operation will deadlock.
    if (m_hasSessionSemaphore)
        dispatch_semaphore_signal(m_hasSessionSemaphore.get());
    dispatch_group_wait(m_isAppendingGroup.get(), DISPATCH_TIME_FOREVER);
    m_appendWeakFactory.revokeAll();
    m_delegate.get().parent = m_appendWeakFactory.createWeakPtr();
}

void SourceBufferPrivateAVFObjC::resetParserState()
{
    m_parserStateWasReset = true;
}

void SourceBufferPrivateAVFObjC::destroyParser()
{
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    if (m_mediaSource && m_mediaSource->player()->hasStreamSession())
        [m_mediaSource->player()->streamSession() removeStreamDataParser:m_parser.get()];
#endif

    [m_delegate invalidate];
    m_delegate = nullptr;
    m_parser = nullptr;
}

void SourceBufferPrivateAVFObjC::destroyRenderers()
{
    if (m_displayLayer)
        setVideoLayer(nullptr);

    if (m_decompressionSession)
        setDecompressionSession(nullptr);

    for (auto& renderer : m_audioRenderers.values()) {
        if (m_mediaSource)
            m_mediaSource->player()->removeAudioRenderer(renderer.get());
        [renderer flush];
        [renderer stopRequestingMediaData];
        [m_errorListener stopObservingRenderer:renderer.get()];
    }

    m_audioRenderers.clear();
}

void SourceBufferPrivateAVFObjC::removedFromMediaSource()
{
    destroyParser();
    destroyRenderers();

    if (m_mediaSource)
        m_mediaSource->removeSourceBuffer(this);
}

MediaPlayer::ReadyState SourceBufferPrivateAVFObjC::readyState() const
{
    return m_mediaSource ? m_mediaSource->player()->readyState() : MediaPlayer::HaveNothing;
}

void SourceBufferPrivateAVFObjC::setReadyState(MediaPlayer::ReadyState readyState)
{
    if (m_mediaSource)
        m_mediaSource->player()->setReadyState(readyState);
}

bool SourceBufferPrivateAVFObjC::hasVideo() const
{
    return m_client && m_client->sourceBufferPrivateHasVideo();
}

bool SourceBufferPrivateAVFObjC::hasSelectedVideo() const
{
    return m_enabledVideoTrackID != -1;
}

bool SourceBufferPrivateAVFObjC::hasAudio() const
{
    return m_client && m_client->sourceBufferPrivateHasAudio();
}

void SourceBufferPrivateAVFObjC::trackDidChangeEnabled(VideoTrackPrivateMediaSourceAVFObjC* track)
{
    int trackID = track->trackID();
    if (!track->selected() && m_enabledVideoTrackID == trackID) {
        m_enabledVideoTrackID = -1;
        [m_parser setShouldProvideMediaData:NO forTrackID:trackID];

        if (m_decompressionSession)
            m_decompressionSession->stopRequestingMediaData();
    } else if (track->selected()) {
        m_enabledVideoTrackID = trackID;
        [m_parser setShouldProvideMediaData:YES forTrackID:trackID];

        if (m_decompressionSession) {
            m_decompressionSession->requestMediaDataWhenReady([this, trackID] {
                didBecomeReadyForMoreSamples(trackID);
            });
        }
    }

    m_mediaSource->hasSelectedVideoChanged(*this);
}

void SourceBufferPrivateAVFObjC::trackDidChangeEnabled(AudioTrackPrivateMediaSourceAVFObjC* track)
{
    int trackID = track->trackID();

    if (!track->enabled()) {
        RetainPtr<AVSampleBufferAudioRenderer> renderer = m_audioRenderers.get(trackID);
        [m_parser setShouldProvideMediaData:NO forTrackID:trackID];
        if (m_mediaSource)
            m_mediaSource->player()->removeAudioRenderer(renderer.get());
    } else {
        [m_parser setShouldProvideMediaData:YES forTrackID:trackID];
        RetainPtr<AVSampleBufferAudioRenderer> renderer;
        if (!m_audioRenderers.contains(trackID)) {
            renderer = adoptNS([allocAVSampleBufferAudioRendererInstance() init]);
            [renderer requestMediaDataWhenReadyOnQueue:dispatch_get_main_queue() usingBlock:^{
                didBecomeReadyForMoreSamples(trackID);
            }];
            m_audioRenderers.set(trackID, renderer);
            [m_errorListener beginObservingRenderer:renderer.get()];
        } else
            renderer = m_audioRenderers.get(trackID);

        if (m_mediaSource)
            m_mediaSource->player()->addAudioRenderer(renderer.get());
    }
}

void SourceBufferPrivateAVFObjC::setCDMSession(CDMSessionMediaSourceAVFObjC* session)
{
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    if (session == m_session)
        return;

    if (m_session)
        m_session->removeSourceBuffer(this);

    m_session = session;

    if (m_session) {
        m_session->addSourceBuffer(this);
        if (m_hasSessionSemaphore) {
            dispatch_semaphore_signal(m_hasSessionSemaphore.get());
            m_hasSessionSemaphore = nullptr;
        }

        if (m_hdcpError) {
            WeakPtr<SourceBufferPrivateAVFObjC> weakThis = createWeakPtr();
            callOnMainThread([weakThis] {
                if (!weakThis || !weakThis->m_session || !weakThis->m_hdcpError)
                    return;

                bool ignored = false;
                weakThis->m_session->layerDidReceiveError(nullptr, weakThis->m_hdcpError.get(), ignored);
            });
        }
    }
#else
    UNUSED_PARAM(session);
#endif
}

void SourceBufferPrivateAVFObjC::flush()
{
    flushVideo();

    for (auto& renderer : m_audioRenderers.values())
        flush(renderer.get());
}

void SourceBufferPrivateAVFObjC::registerForErrorNotifications(SourceBufferPrivateAVFObjCErrorClient* client)
{
    ASSERT(!m_errorClients.contains(client));
    m_errorClients.append(client);
}

void SourceBufferPrivateAVFObjC::unregisterForErrorNotifications(SourceBufferPrivateAVFObjCErrorClient* client)
{
    ASSERT(m_errorClients.contains(client));
    m_errorClients.remove(m_errorClients.find(client));
}

void SourceBufferPrivateAVFObjC::layerDidReceiveError(AVSampleBufferDisplayLayer *layer, NSError *error)
{
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::layerDidReceiveError(%p): layer(%p), error(%@)", this, layer, [error description]);

    // FIXME(142246): Remove the following once <rdar://problem/20027434> is resolved.
    bool anyIgnored = false;
    for (auto& client : m_errorClients) {
        bool shouldIgnore = false;
        client->layerDidReceiveError(layer, error, shouldIgnore);
        anyIgnored |= shouldIgnore;
    }
    if (anyIgnored)
        return;

    int errorCode = [[[error userInfo] valueForKey:@"OSStatus"] intValue];

    if (m_client)
        m_client->sourceBufferPrivateDidReceiveRenderingError(errorCode);
}

void SourceBufferPrivateAVFObjC::rendererDidReceiveError(AVSampleBufferAudioRenderer *renderer, NSError *error)
{
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::rendererDidReceiveError(%p): renderer(%p), error(%@)", this, renderer, [error description]);

    if ([error code] == 'HDCP')
        m_hdcpError = error;

    // FIXME(142246): Remove the following once <rdar://problem/20027434> is resolved.
    bool anyIgnored = false;
    for (auto& client : m_errorClients) {
        bool shouldIgnore = false;
        client->rendererDidReceiveError(renderer, error, shouldIgnore);
        anyIgnored |= shouldIgnore;
    }
    if (anyIgnored)
        return;
}

void SourceBufferPrivateAVFObjC::flush(const AtomicString& trackIDString)
{
    int trackID = trackIDString.toInt();
    LOG(MediaSource, "SourceBufferPrivateAVFObjC::flush(%p) - trackId: %d", this, trackID);

    if (trackID == m_enabledVideoTrackID) {
        flushVideo();
    } else if (m_audioRenderers.contains(trackID))
        flush(m_audioRenderers.get(trackID).get());
}

void SourceBufferPrivateAVFObjC::flushVideo()
{
    [m_displayLayer flush];

    if (m_decompressionSession) {
        m_decompressionSession->flush();
        m_decompressionSession->notifyWhenHasAvailableVideoFrame([weakThis = createWeakPtr()] {
            if (weakThis && weakThis->m_mediaSource)
                weakThis->m_mediaSource->player()->setHasAvailableVideoFrame(true);
        });
    }

    m_cachedSize = std::nullopt;

    if (m_mediaSource) {
        m_mediaSource->player()->setHasAvailableVideoFrame(false);
        m_mediaSource->player()->flushPendingSizeChanges();
    }
}

void SourceBufferPrivateAVFObjC::flush(AVSampleBufferAudioRenderer *renderer)
{
    [renderer flush];

    if (m_mediaSource)
        m_mediaSource->player()->setHasAvailableAudioSample(renderer, false);
}

void SourceBufferPrivateAVFObjC::enqueueSample(Ref<MediaSample>&& sample, const AtomicString& trackIDString)
{
    int trackID = trackIDString.toInt();
    if (trackID != m_enabledVideoTrackID && !m_audioRenderers.contains(trackID))
        return;

    PlatformSample platformSample = sample->platformSample();
    if (platformSample.type != PlatformSample::CMSampleBufferType)
        return;

    LOG(MediaSourceSamples, "SourceBufferPrivateAVFObjC::enqueueSample(%p) - sample(%s)", this, toString(sample.get()).utf8().data());

    if (trackID == m_enabledVideoTrackID) {
        CMFormatDescriptionRef formatDescription = CMSampleBufferGetFormatDescription(platformSample.sample.cmSampleBuffer);
        FloatSize formatSize = FloatSize(CMVideoFormatDescriptionGetPresentationDimensions(formatDescription, true, true));
        if (!m_cachedSize || formatSize != m_cachedSize.value()) {
            LOG(MediaSource, "SourceBufferPrivateAVFObjC::enqueueSample(%p) - size change detected: {width=%lf, height=%lf}", formatSize.width(), formatSize.height());
            bool sizeWasNull = !m_cachedSize;
            m_cachedSize = formatSize;
            if (m_mediaSource) {
                if (sizeWasNull)
                    m_mediaSource->player()->setNaturalSize(formatSize);
                else
                    m_mediaSource->player()->sizeWillChangeAtTime(sample->presentationTime(), formatSize);
            }
        }

        if (m_decompressionSession)
            m_decompressionSession->enqueueSample(platformSample.sample.cmSampleBuffer);

        if (m_displayLayer) {
            if (m_mediaSource && !m_mediaSource->player()->hasAvailableVideoFrame() && !sample->isNonDisplaying()) {
                auto context = adoptNS([[WebBufferConsumedContext alloc] initWithParent:createWeakPtr()]);
                CMSampleBufferRef rawSampleCopy;
                CMSampleBufferCreateCopy(kCFAllocatorDefault, platformSample.sample.cmSampleBuffer, &rawSampleCopy);
                auto sampleCopy = adoptCF(rawSampleCopy);
                CMSetAttachment(sampleCopy.get(), kCMSampleBufferAttachmentKey_PostNotificationWhenConsumed, @{kBufferConsumedContext: context.get()}, kCMAttachmentMode_ShouldNotPropagate);
                [m_displayLayer enqueueSampleBuffer:sampleCopy.get()];
            } else
                [m_displayLayer enqueueSampleBuffer:platformSample.sample.cmSampleBuffer];
        }
    } else {
        auto renderer = m_audioRenderers.get(trackID);
        [renderer enqueueSampleBuffer:platformSample.sample.cmSampleBuffer];
        if (m_mediaSource && !sample->isNonDisplaying())
            m_mediaSource->player()->setHasAvailableAudioSample(renderer.get(), true);
    }
}

void SourceBufferPrivateAVFObjC::bufferWasConsumed()
{
    if (m_mediaSource)
        m_mediaSource->player()->setHasAvailableVideoFrame(true);
}

bool SourceBufferPrivateAVFObjC::isReadyForMoreSamples(const AtomicString& trackIDString)
{
    int trackID = trackIDString.toInt();
    if (trackID == m_enabledVideoTrackID) {
        if (m_decompressionSession)
            return m_decompressionSession->isReadyForMoreMediaData();

        return [m_displayLayer isReadyForMoreMediaData];
    }

    if (m_audioRenderers.contains(trackID))
        return [m_audioRenderers.get(trackID) isReadyForMoreMediaData];

    return false;
}

void SourceBufferPrivateAVFObjC::setActive(bool isActive)
{
    if (m_mediaSource)
        m_mediaSource->sourceBufferPrivateDidChangeActiveState(this, isActive);
}

MediaTime SourceBufferPrivateAVFObjC::fastSeekTimeForMediaTime(MediaTime time, MediaTime negativeThreshold, MediaTime positiveThreshold)
{
    if (!m_client)
        return time;
    return m_client->sourceBufferPrivateFastSeekTimeForMediaTime(time, negativeThreshold, positiveThreshold);
}

void SourceBufferPrivateAVFObjC::willSeek()
{
    flush();
}

void SourceBufferPrivateAVFObjC::seekToTime(MediaTime time)
{
    if (m_client)
        m_client->sourceBufferPrivateSeekToTime(time);
}

FloatSize SourceBufferPrivateAVFObjC::naturalSize()
{
    return m_cachedSize.value_or(FloatSize());
}

void SourceBufferPrivateAVFObjC::didBecomeReadyForMoreSamples(int trackID)
{
    LOG(Media, "SourceBufferPrivateAVFObjC::didBecomeReadyForMoreSamples(%p) - track(%d)", this, trackID);
    if (trackID == m_enabledVideoTrackID) {
        if (m_decompressionSession)
            m_decompressionSession->stopRequestingMediaData();
        [m_displayLayer stopRequestingMediaData];
    } else if (m_audioRenderers.contains(trackID))
        [m_audioRenderers.get(trackID) stopRequestingMediaData];
    else
        return;

    if (m_client)
        m_client->sourceBufferPrivateDidBecomeReadyForMoreSamples(AtomicString::number(trackID));
}

void SourceBufferPrivateAVFObjC::notifyClientWhenReadyForMoreSamples(const AtomicString& trackIDString)
{
    int trackID = trackIDString.toInt();
    if (trackID == m_enabledVideoTrackID) {
        if (m_decompressionSession) {
            m_decompressionSession->requestMediaDataWhenReady([this, trackID] {
                didBecomeReadyForMoreSamples(trackID);
            });
        }
        if (m_displayLayer) {
            [m_displayLayer requestMediaDataWhenReadyOnQueue:dispatch_get_main_queue() usingBlock:^ {
                didBecomeReadyForMoreSamples(trackID);
            }];
        }
    } else if (m_audioRenderers.contains(trackID)) {
        [m_audioRenderers.get(trackID) requestMediaDataWhenReadyOnQueue:dispatch_get_main_queue() usingBlock:^ {
            didBecomeReadyForMoreSamples(trackID);
        }];
    }
}

void SourceBufferPrivateAVFObjC::setVideoLayer(AVSampleBufferDisplayLayer* layer)
{
    if (layer == m_displayLayer)
        return;

    ASSERT(!layer || !m_decompressionSession || hasSelectedVideo());

    if (m_displayLayer) {
        [m_displayLayer flush];
        [m_displayLayer stopRequestingMediaData];
        [m_errorListener stopObservingLayer:m_displayLayer.get()];
    }

    m_displayLayer = layer;

    if (m_displayLayer) {
        [m_displayLayer requestMediaDataWhenReadyOnQueue:dispatch_get_main_queue() usingBlock:^ {
            didBecomeReadyForMoreSamples(m_enabledVideoTrackID);
        }];
        [m_errorListener beginObservingLayer:m_displayLayer.get()];
        if (m_client)
            m_client->sourceBufferPrivateReenqueSamples(AtomicString::number(m_enabledVideoTrackID));
    }
}

void SourceBufferPrivateAVFObjC::setDecompressionSession(WebCoreDecompressionSession* decompressionSession)
{
    if (m_decompressionSession == decompressionSession)
        return;

    if (m_decompressionSession) {
        m_decompressionSession->stopRequestingMediaData();
        m_decompressionSession->invalidate();
    }

    m_decompressionSession = decompressionSession;

    if (!m_decompressionSession)
        return;

    WeakPtr<SourceBufferPrivateAVFObjC> weakThis = createWeakPtr();
    m_decompressionSession->requestMediaDataWhenReady([weakThis] {
        if (weakThis)
            weakThis->didBecomeReadyForMoreSamples(weakThis->m_enabledVideoTrackID);
    });
    m_decompressionSession->notifyWhenHasAvailableVideoFrame([weakThis = createWeakPtr()] {
        if (weakThis && weakThis->m_mediaSource)
            weakThis->m_mediaSource->player()->setHasAvailableVideoFrame(true);
    });
    if (m_client)
        m_client->sourceBufferPrivateReenqueSamples(AtomicString::number(m_enabledVideoTrackID));
}

}

#endif // ENABLE(MEDIA_SOURCE) && USE(AVFOUNDATION)
