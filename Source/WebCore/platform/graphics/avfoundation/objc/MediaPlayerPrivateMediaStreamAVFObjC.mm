/*
 * Copyright (C) 2015-2020 Apple Inc. All rights reserved.
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
#import "MediaPlayerPrivateMediaStreamAVFObjC.h"

#if ENABLE(MEDIA_STREAM) && USE(AVFOUNDATION)

#import "AudioTrackPrivateMediaStream.h"
#import "GraphicsContextCG.h"
#import "Logging.h"
#import "LocalSampleBufferDisplayLayer.h"
#import "MediaStreamPrivate.h"
#import "PixelBufferConformerCV.h"
#import "VideoFullscreenLayerManagerObjC.h"
#import "VideoTrackPrivateMediaStream.h"
#import <CoreGraphics/CGAffineTransform.h>
#import <objc_runtime.h>
#import <pal/avfoundation/MediaTimeAVFoundation.h>
#import <pal/spi/mac/AVFoundationSPI.h>
#import <pal/system/Clock.h>
#import <wtf/MainThread.h>
#import <wtf/NeverDestroyed.h>

#import "CoreVideoSoftLink.h"
#import <pal/cf/CoreMediaSoftLink.h>
#import <pal/cocoa/AVFoundationSoftLink.h>

@interface WebRootSampleBufferBoundsChangeListener : NSObject {
    WeakPtr<WebCore::MediaPlayerPrivateMediaStreamAVFObjC> _parent;
}

- (id)initWithParent:(WebCore::MediaPlayerPrivateMediaStreamAVFObjC*)callback;
- (void)invalidate;
- (void)begin;
- (void)stop;
@end

@implementation WebRootSampleBufferBoundsChangeListener

- (id)initWithParent:(WebCore::MediaPlayerPrivateMediaStreamAVFObjC*)parent
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
    ASSERT(_parent->rootLayer());

    [_parent->rootLayer() addObserver:self forKeyPath:@"bounds" options:NSKeyValueObservingOptionNew context:nil];
}

- (void)stop
{
    if (!_parent)
        return;

    if (_parent->rootLayer())
        [_parent->rootLayer() removeObserver:self forKeyPath:@"bounds"];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    UNUSED_PARAM(context);
    UNUSED_PARAM(keyPath);
    ASSERT(_parent);

    if (!_parent)
        return;

    if ([[change valueForKey:NSKeyValueChangeNotificationIsPriorKey] boolValue])
        return;

    if ((CALayer *)object == _parent->rootLayer()) {
        if ([keyPath isEqualToString:@"bounds"]) {
            if (!_parent)
                return;

            if (isMainThread()) {
                _parent->rootLayerBoundsDidChange();
                return;
            }

            callOnMainThread([protectedSelf = RetainPtr<WebRootSampleBufferBoundsChangeListener>(self)] {
                if (!protectedSelf->_parent)
                    return;

                protectedSelf->_parent->rootLayerBoundsDidChange();
            });
        }
    }

}
@end

namespace WebCore {
using namespace PAL;

#pragma mark -
#pragma mark MediaPlayerPrivateMediaStreamAVFObjC

static const double rendererLatency = 0.02;

MediaPlayerPrivateMediaStreamAVFObjC::MediaPlayerPrivateMediaStreamAVFObjC(MediaPlayer* player)
    : m_player(player)
    , m_clock(PAL::Clock::create())
    , m_videoFullscreenLayerManager(makeUnique<VideoFullscreenLayerManagerObjC>())
#if !RELEASE_LOG_DISABLED
    , m_logger(player->mediaPlayerLogger())
    , m_logIdentifier(player->mediaPlayerLogIdentifier())
#endif
    , m_boundsChangeListener(adoptNS([[WebRootSampleBufferBoundsChangeListener alloc] initWithParent:this]))
{
    INFO_LOG(LOGIDENTIFIER);
}

MediaPlayerPrivateMediaStreamAVFObjC::~MediaPlayerPrivateMediaStreamAVFObjC()
{
    INFO_LOG(LOGIDENTIFIER);

    for (const auto& track : m_audioTrackMap.values())
        track->pause();

    if (m_mediaStreamPrivate) {
        m_mediaStreamPrivate->removeObserver(*this);

        for (auto& track : m_mediaStreamPrivate->tracks())
            track->removeObserver(*this);
    }

    [m_boundsChangeListener invalidate];

    destroyLayers();

    auto audioTrackMap = WTFMove(m_audioTrackMap);
    for (auto& track : audioTrackMap.values())
        track->clear();

    m_videoTrackMap.clear();
}

#pragma mark -
#pragma mark MediaPlayer Factory Methods

class MediaPlayerFactoryMediaStreamAVFObjC final : public MediaPlayerFactory {
private:
    MediaPlayerEnums::MediaEngineIdentifier identifier() const final { return MediaPlayerEnums::MediaEngineIdentifier::AVFoundationMediaStream; };

    std::unique_ptr<MediaPlayerPrivateInterface> createMediaEnginePlayer(MediaPlayer* player) const final
    {
        return makeUnique<MediaPlayerPrivateMediaStreamAVFObjC>(player);
    }

    void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types) const final
    {
        return MediaPlayerPrivateMediaStreamAVFObjC::getSupportedTypes(types);
    }

    MediaPlayer::SupportsType supportsTypeAndCodecs(const MediaEngineSupportParameters& parameters) const final
    {
        return MediaPlayerPrivateMediaStreamAVFObjC::supportsType(parameters);
    }
};

void MediaPlayerPrivateMediaStreamAVFObjC::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (!isAvailable())
        return;

    registrar(makeUnique<MediaPlayerFactoryMediaStreamAVFObjC>());
}

bool MediaPlayerPrivateMediaStreamAVFObjC::isAvailable()
{
    return isAVFoundationFrameworkAvailable() && isCoreMediaFrameworkAvailable() && getAVSampleBufferDisplayLayerClass();
}

void MediaPlayerPrivateMediaStreamAVFObjC::getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types)
{
    // FIXME: Is it really correct to list no supported types?
    types.clear();
}

MediaPlayer::SupportsType MediaPlayerPrivateMediaStreamAVFObjC::supportsType(const MediaEngineSupportParameters& parameters)
{
    return parameters.isMediaStream ? MediaPlayer::SupportsType::IsSupported : MediaPlayer::SupportsType::IsNotSupported;
}

#pragma mark -
#pragma mark AVSampleBuffer Methods

MediaTime MediaPlayerPrivateMediaStreamAVFObjC::calculateTimelineOffset(const MediaSample& sample, double latency)
{
    MediaTime sampleTime = sample.outputPresentationTime();
    if (!sampleTime || !sampleTime.isValid())
        sampleTime = sample.presentationTime();
    MediaTime timelineOffset = streamTime() - sampleTime + MediaTime::createWithDouble(latency);
    if (timelineOffset.timeScale() != sampleTime.timeScale())
        timelineOffset = PAL::toMediaTime(CMTimeConvertScale(PAL::toCMTime(timelineOffset), sampleTime.timeScale(), kCMTimeRoundingMethod_Default));
    return timelineOffset;
}

CGAffineTransform MediaPlayerPrivateMediaStreamAVFObjC::videoTransformationMatrix(MediaSample& sample, bool forceUpdate)
{
    if (!forceUpdate && m_transformIsValid)
        return m_videoTransform;

    CMSampleBufferRef sampleBuffer = sample.platformSample().sample.cmSampleBuffer;
    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(CMSampleBufferGetImageBuffer(sampleBuffer));
    size_t width = CVPixelBufferGetWidth(pixelBuffer);
    size_t height = CVPixelBufferGetHeight(pixelBuffer);
    if (!width || !height)
        return CGAffineTransformIdentity;

    ASSERT(m_videoRotation >= MediaSample::VideoRotation::None);
    ASSERT(m_videoRotation <= MediaSample::VideoRotation::Left);

    m_videoTransform = CGAffineTransformMakeRotation(static_cast<int>(m_videoRotation) * M_PI / 180);
    if (sample.videoMirrored())
        m_videoTransform = CGAffineTransformScale(m_videoTransform, -1, 1);

    m_transformIsValid = true;
    return m_videoTransform;
}

void MediaPlayerPrivateMediaStreamAVFObjC::enqueueCorrectedVideoSample(MediaSample& sample)
{
    if (m_sampleBufferDisplayLayer) {
        if (m_sampleBufferDisplayLayer->didFail())
            return;

        if (sample.videoRotation() != m_videoRotation || sample.videoMirrored() != m_videoMirrored) {
            m_videoRotation = sample.videoRotation();
            m_videoMirrored = sample.videoMirrored();
            m_sampleBufferDisplayLayer->updateAffineTransform(videoTransformationMatrix(sample, true));
            updateDisplayLayer();
        }
        m_sampleBufferDisplayLayer->enqueueSample(sample);
    }

    if (!m_hasEverEnqueuedVideoFrame) {
        m_hasEverEnqueuedVideoFrame = true;
        m_player->firstVideoFrameAvailable();
    }
}

void MediaPlayerPrivateMediaStreamAVFObjC::enqueueVideoSample(MediaStreamTrackPrivate& track, MediaSample& sample)
{
    if (&track != m_mediaStreamPrivate->activeVideoTrack())
        return;

    if (!m_imagePainter.mediaSample || m_displayMode != PausedImage) {
        m_imagePainter.mediaSample = &sample;
        m_imagePainter.cgImage = nullptr;
        if (m_readyState < MediaPlayer::ReadyState::HaveEnoughData)
            updateReadyState();
    }

    if (m_displayMode != LivePreview && !m_waitingForFirstImage)
        return;

    auto videoTrack = m_videoTrackMap.get(track.id());
    MediaTime timelineOffset = videoTrack->timelineOffset();
    if (timelineOffset == MediaTime::invalidTime()) {
        timelineOffset = calculateTimelineOffset(sample, rendererLatency);
        videoTrack->setTimelineOffset(timelineOffset);

        INFO_LOG(LOGIDENTIFIER, "timeline offset for track ", track.id(), " set to ", timelineOffset);
    }

    DEBUG_LOG(LOGIDENTIFIER, "original sample = ", sample);
    sample.offsetTimestampsBy(timelineOffset);
    DEBUG_LOG(LOGIDENTIFIER, "updated sample = ", sample);

    if (WILL_LOG(WTFLogLevel::Debug)) {
        MediaTime now = streamTime();
        double delta = (sample.presentationTime() - now).toDouble();
        if (delta < 0)
            DEBUG_LOG(LOGIDENTIFIER, "*NOTE* sample at time is ", now, " is", -delta, " seconds late");
        else if (delta < .01)
            DEBUG_LOG(LOGIDENTIFIER, "*NOTE* audio sample at time ", now, " is only ", delta, " seconds early");
        else if (delta > .3)
            DEBUG_LOG(LOGIDENTIFIER, "*NOTE* audio sample at time ", now, " is ", delta, " seconds early!");
    }

    enqueueCorrectedVideoSample(sample);
    if (m_waitingForFirstImage) {
        m_waitingForFirstImage = false;
        updateDisplayMode();
    }
}

AudioSourceProvider* MediaPlayerPrivateMediaStreamAVFObjC::audioSourceProvider()
{
    // FIXME: This should return a mix of all audio tracks - https://bugs.webkit.org/show_bug.cgi?id=160305
    return nullptr;
}

void MediaPlayerPrivateMediaStreamAVFObjC::sampleBufferDisplayLayerStatusDidChange(SampleBufferDisplayLayer& layer)
{
    ASSERT(&layer == m_sampleBufferDisplayLayer.get());
    UNUSED_PARAM(layer);
    if (!m_activeVideoTrack)
        return;

    if (auto track = m_videoTrackMap.get(m_activeVideoTrack->id()))
        track->setTimelineOffset(MediaTime::invalidTime());
}

void MediaPlayerPrivateMediaStreamAVFObjC::applicationDidBecomeActive()
{
    if (m_sampleBufferDisplayLayer && m_sampleBufferDisplayLayer->didFail()) {
        flushRenderers();
        if (m_imagePainter.mediaSample)
            enqueueCorrectedVideoSample(*m_imagePainter.mediaSample);
        updateDisplayMode();
    }
}

void MediaPlayerPrivateMediaStreamAVFObjC::flushRenderers()
{
    if (m_sampleBufferDisplayLayer)
        m_sampleBufferDisplayLayer->flush();
}

void MediaPlayerPrivateMediaStreamAVFObjC::ensureLayers()
{
    if (m_sampleBufferDisplayLayer)
        return;

    if (!m_mediaStreamPrivate || !m_mediaStreamPrivate->activeVideoTrack() || !m_mediaStreamPrivate->activeVideoTrack()->enabled())
        return;

    auto size = snappedIntRect(m_player->playerContentBoxRect()).size();
    m_sampleBufferDisplayLayer = SampleBufferDisplayLayer::create(*this, hideRootLayer(), size);

    if (!m_sampleBufferDisplayLayer) {
        ERROR_LOG(LOGIDENTIFIER, "Creating the SampleBufferDisplayLayer failed.");
        return;
    }

    updateRenderingMode();
    updateDisplayLayer();

    m_videoFullscreenLayerManager->setVideoLayer(m_sampleBufferDisplayLayer->rootLayer(), size);

    [m_boundsChangeListener begin];
}

void MediaPlayerPrivateMediaStreamAVFObjC::destroyLayers()
{
    if (m_sampleBufferDisplayLayer)
        m_sampleBufferDisplayLayer = nullptr;

    updateRenderingMode();
    
    m_videoFullscreenLayerManager->didDestroyVideoLayer();
}

#pragma mark -
#pragma mark MediaPlayerPrivateInterface Overrides

void MediaPlayerPrivateMediaStreamAVFObjC::load(const String&)
{
    // This media engine only supports MediaStream URLs.
    scheduleDeferredTask([this] {
        setNetworkState(MediaPlayer::NetworkState::FormatError);
    });
}

#if ENABLE(MEDIA_SOURCE)
void MediaPlayerPrivateMediaStreamAVFObjC::load(const String&, MediaSourcePrivateClient*)
{
    // This media engine only supports MediaStream URLs.
    scheduleDeferredTask([this] {
        setNetworkState(MediaPlayer::NetworkState::FormatError);
    });
}
#endif

void MediaPlayerPrivateMediaStreamAVFObjC::load(MediaStreamPrivate& stream)
{
    INFO_LOG(LOGIDENTIFIER);

    m_intrinsicSize = FloatSize();

    m_mediaStreamPrivate = &stream;
    m_mediaStreamPrivate->addObserver(*this);
    m_ended = !m_mediaStreamPrivate->active();

    scheduleDeferredTask([this] {
        updateTracks();
        setNetworkState(MediaPlayer::NetworkState::Idle);
        updateReadyState();
    });
}

bool MediaPlayerPrivateMediaStreamAVFObjC::didPassCORSAccessCheck() const
{
    // We are only doing a check on the active video track since the sole consumer of this API is canvas.
    // FIXME: We should change the name of didPassCORSAccessCheck if it is expected to stay like this.
    const auto* track = m_mediaStreamPrivate->activeVideoTrack();
    return !track || !track->isIsolated();
}

void MediaPlayerPrivateMediaStreamAVFObjC::cancelLoad()
{
    INFO_LOG(LOGIDENTIFIER);
    if (playing())
        pause();
}

void MediaPlayerPrivateMediaStreamAVFObjC::prepareToPlay()
{
    INFO_LOG(LOGIDENTIFIER);
}

PlatformLayer* MediaPlayerPrivateMediaStreamAVFObjC::rootLayer() const
{
    return m_sampleBufferDisplayLayer ? m_sampleBufferDisplayLayer->rootLayer() : nullptr;
}

PlatformLayer* MediaPlayerPrivateMediaStreamAVFObjC::platformLayer() const
{
    if (!m_sampleBufferDisplayLayer || !m_sampleBufferDisplayLayer->rootLayer() || m_displayMode == None)
        return nullptr;

    return m_videoFullscreenLayerManager->videoInlineLayer();
}

MediaPlayerPrivateMediaStreamAVFObjC::DisplayMode MediaPlayerPrivateMediaStreamAVFObjC::currentDisplayMode() const
{
    if (m_intrinsicSize.isEmpty() || !metaDataAvailable() || !m_sampleBufferDisplayLayer)
        return None;

    if (auto* track = m_mediaStreamPrivate->activeVideoTrack()) {
        if (!track->enabled() || track->muted() || track->ended())
            return PaintItBlack;
    }

    if (m_waitingForFirstImage)
        return WaitingForFirstImage;

    if (playing() && !m_ended) {
        if (!m_mediaStreamPrivate->isProducingData())
            return PausedImage;
        return LivePreview;
    }

    if (m_playbackState == PlaybackState::None || m_ended)
        return PaintItBlack;

    return PausedImage;
}

bool MediaPlayerPrivateMediaStreamAVFObjC::updateDisplayMode()
{
    DisplayMode displayMode = currentDisplayMode();

    if (displayMode == m_displayMode)
        return false;

    INFO_LOG(LOGIDENTIFIER, "updated to ", static_cast<int>(displayMode));
    m_displayMode = displayMode;

    if (m_sampleBufferDisplayLayer)
        m_sampleBufferDisplayLayer->updateDisplayMode(m_displayMode < PausedImage, hideRootLayer());

    return true;
}

void MediaPlayerPrivateMediaStreamAVFObjC::play()
{
    ALWAYS_LOG(LOGIDENTIFIER);

    if (!metaDataAvailable() || playing() || m_ended)
        return;

    m_playbackState = PlaybackState::Playing;
    if (!m_clock->isRunning())
        m_clock->start();

    for (const auto& track : m_audioTrackMap.values())
        track->play();

    updateDisplayMode();

    scheduleDeferredTask([this] {
        updateReadyState();
        if (m_player)
            m_player->rateChanged();
    });
}

void MediaPlayerPrivateMediaStreamAVFObjC::pause()
{
    ALWAYS_LOG(LOGIDENTIFIER);

    if (!metaDataAvailable() || !playing() || m_ended)
        return;

    m_pausedTime = currentMediaTime();
    m_playbackState = PlaybackState::Paused;

    for (const auto& track : m_audioTrackMap.values())
        track->pause();

    updateDisplayMode();
    flushRenderers();

    scheduleDeferredTask([this] {
        if (m_player)
            m_player->rateChanged();
    });
}

void MediaPlayerPrivateMediaStreamAVFObjC::setVolume(float volume)
{
    if (m_volume == volume)
        return;

    ALWAYS_LOG(LOGIDENTIFIER, volume);
    m_volume = volume;
    for (const auto& track : m_audioTrackMap.values())
        track->setVolume(m_muted ? 0 : m_volume);
}

void MediaPlayerPrivateMediaStreamAVFObjC::setMuted(bool muted)
{
    if (muted == m_muted)
        return;

    ALWAYS_LOG(LOGIDENTIFIER, muted);
    m_muted = muted;
    for (const auto& track : m_audioTrackMap.values())
        track->setVolume(m_muted ? 0 : m_volume);
}

bool MediaPlayerPrivateMediaStreamAVFObjC::hasVideo() const
{
    if (!metaDataAvailable())
        return false;
    
    return m_mediaStreamPrivate->hasVideo();
}

bool MediaPlayerPrivateMediaStreamAVFObjC::hasAudio() const
{
    if (!metaDataAvailable())
        return false;
    
    return m_mediaStreamPrivate->hasAudio();
}

void MediaPlayerPrivateMediaStreamAVFObjC::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    if (m_visible)
        flushRenderers();
}

MediaTime MediaPlayerPrivateMediaStreamAVFObjC::durationMediaTime() const
{
    return MediaTime::positiveInfiniteTime();
}

MediaTime MediaPlayerPrivateMediaStreamAVFObjC::currentMediaTime() const
{
    if (paused())
        return m_pausedTime;

    return streamTime();
}

MediaTime MediaPlayerPrivateMediaStreamAVFObjC::streamTime() const
{
    return MediaTime::createWithDouble(m_clock->currentTime());
}

MediaPlayer::NetworkState MediaPlayerPrivateMediaStreamAVFObjC::networkState() const
{
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivateMediaStreamAVFObjC::readyState() const
{
    return m_readyState;
}

MediaPlayer::ReadyState MediaPlayerPrivateMediaStreamAVFObjC::currentReadyState()
{
    if (!m_mediaStreamPrivate || !m_mediaStreamPrivate->active() || !m_mediaStreamPrivate->tracks().size())
        return MediaPlayer::ReadyState::HaveNothing;

    bool allTracksAreLive = true;
    for (auto& track : m_mediaStreamPrivate->tracks()) {
        if (!track->enabled() || track->readyState() != MediaStreamTrackPrivate::ReadyState::Live)
            allTracksAreLive = false;

        if (track == m_mediaStreamPrivate->activeVideoTrack() && !m_imagePainter.mediaSample) {
            if (!m_haveSeenMetadata || m_waitingForFirstImage)
                return MediaPlayer::ReadyState::HaveNothing;
            allTracksAreLive = false;
        }
    }

    if (m_waitingForFirstImage || (!allTracksAreLive && !m_haveSeenMetadata))
        return MediaPlayer::ReadyState::HaveMetadata;

    return MediaPlayer::ReadyState::HaveEnoughData;
}

void MediaPlayerPrivateMediaStreamAVFObjC::updateReadyState()
{
    MediaPlayer::ReadyState newReadyState = currentReadyState();

    if (newReadyState != m_readyState) {
        ALWAYS_LOG(LOGIDENTIFIER, "updated to ", (int)newReadyState);
        setReadyState(newReadyState);
    }
}

void MediaPlayerPrivateMediaStreamAVFObjC::activeStatusChanged()
{
    scheduleDeferredTask([this] {
        bool ended = !m_mediaStreamPrivate->active();
        if (ended && playing())
            pause();

        updateReadyState();
        updateDisplayMode();

        if (ended != m_ended) {
            m_ended = ended;
            if (m_player) {
                m_player->timeChanged();
                m_player->characteristicChanged();
            }
        }
    });
}

void MediaPlayerPrivateMediaStreamAVFObjC::updateRenderingMode()
{
    if (!updateDisplayMode())
        return;

    scheduleDeferredTask([this] {
        m_transformIsValid = false;
        if (m_player)
            m_player->renderingModeChanged();
    });

}

void MediaPlayerPrivateMediaStreamAVFObjC::characteristicsChanged()
{
    bool sizeChanged = false;

    FloatSize intrinsicSize = m_mediaStreamPrivate->intrinsicSize();
    if (intrinsicSize.height() != m_intrinsicSize.height() || intrinsicSize.width() != m_intrinsicSize.width()) {
        m_intrinsicSize = intrinsicSize;
        sizeChanged = true;
        if (m_playbackState == PlaybackState::None)
            m_playbackState = PlaybackState::Paused;
    }

    updateTracks();
    updateDisplayMode();

    scheduleDeferredTask([this, sizeChanged] {
        updateReadyState();

        if (!m_player)
            return;

        m_player->characteristicChanged();
        if (sizeChanged) {
            m_player->sizeChanged();
        }
    });
}

void MediaPlayerPrivateMediaStreamAVFObjC::didAddTrack(MediaStreamTrackPrivate&)
{
    updateTracks();
}

void MediaPlayerPrivateMediaStreamAVFObjC::didRemoveTrack(MediaStreamTrackPrivate&)
{
    updateTracks();
}

void MediaPlayerPrivateMediaStreamAVFObjC::sampleBufferUpdated(MediaStreamTrackPrivate& track, MediaSample& mediaSample)
{
    ASSERT(track.id() == mediaSample.trackID());
    ASSERT(mediaSample.platformSample().type == PlatformSample::CMSampleBufferType);
    ASSERT(m_mediaStreamPrivate);

    if (streamTime().toDouble() < 0)
        return;

    switch (track.type()) {
    case RealtimeMediaSource::Type::None:
        // Do nothing.
        break;
    case RealtimeMediaSource::Type::Audio:
        break;
    case RealtimeMediaSource::Type::Video:
        if (&track == m_activeVideoTrack.get())
            enqueueVideoSample(track, mediaSample);
        break;
    }
}

void MediaPlayerPrivateMediaStreamAVFObjC::readyStateChanged(MediaStreamTrackPrivate&)
{
    scheduleDeferredTask([this] {
        updateReadyState();
    });
}

bool MediaPlayerPrivateMediaStreamAVFObjC::supportsPictureInPicture() const
{
#if PLATFORM(IOS_FAMILY)
    for (const auto& track : m_videoTrackMap.values()) {
        if (track->streamTrack().isCaptureTrack())
            return false;
    }
#endif
    
    return true;
}

void MediaPlayerPrivateMediaStreamAVFObjC::setVideoFullscreenLayer(PlatformLayer* videoFullscreenLayer, WTF::Function<void()>&& completionHandler)
{
    updateCurrentFrameImage();
    m_videoFullscreenLayerManager->setVideoFullscreenLayer(videoFullscreenLayer, WTFMove(completionHandler), m_imagePainter.cgImage);
}

void MediaPlayerPrivateMediaStreamAVFObjC::setVideoFullscreenFrame(FloatRect frame)
{
    m_videoFullscreenLayerManager->setVideoFullscreenFrame(frame);
}

typedef enum {
    Add,
    Remove,
    Configure
} TrackState;

template <typename RefT>
void updateTracksOfType(HashMap<String, RefT>& trackMap, RealtimeMediaSource::Type trackType, MediaStreamTrackPrivateVector& currentTracks, RefT (*itemFactory)(MediaStreamTrackPrivate&), const Function<void(typename RefT::ValueType&, int, TrackState)>& configureTrack)
{
    Vector<RefT> removedTracks;
    Vector<RefT> addedTracks;
    Vector<RefPtr<MediaStreamTrackPrivate>> addedPrivateTracks;

    for (const auto& track : currentTracks) {
        if (track->type() != trackType)
            continue;

        if (!trackMap.contains(track->id()))
            addedPrivateTracks.append(track);
    }

    for (const auto& track : trackMap.values()) {
        auto& streamTrack = track->streamTrack();
        if (currentTracks.contains(&streamTrack))
            continue;

        removedTracks.append(track);
    }
    for (auto& track : removedTracks)
        trackMap.remove(track->streamTrack().id());

    for (auto& track : addedPrivateTracks) {
        RefT newTrack = itemFactory(*track.get());
        trackMap.add(track->id(), newTrack);
        addedTracks.append(newTrack);
    }

    int index = 0;
    for (auto& track : removedTracks)
        configureTrack(*track, index++, TrackState::Remove);

    index = 0;
    for (auto& track : addedTracks)
        configureTrack(*track, index++, TrackState::Add);

    index = 0;
    for (const auto& track : trackMap.values())
        configureTrack(*track, index++, TrackState::Configure);
}

void MediaPlayerPrivateMediaStreamAVFObjC::checkSelectedVideoTrack()
{
    if (m_pendingSelectedTrackCheck)
        return;

    m_pendingSelectedTrackCheck = true;
    scheduleDeferredTask([this] {
        auto oldVideoTrack = m_activeVideoTrack;
        bool hideVideoLayer = true;
        m_activeVideoTrack = nullptr;
        if (m_mediaStreamPrivate->activeVideoTrack()) {
            for (const auto& track : m_videoTrackMap.values()) {
                if (&track->streamTrack() == m_mediaStreamPrivate->activeVideoTrack()) {
                    m_activeVideoTrack = m_mediaStreamPrivate->activeVideoTrack();
                    if (track->selected())
                        hideVideoLayer = false;
                    break;
                }
            }
        }

        if (oldVideoTrack != m_activeVideoTrack) {
            m_imagePainter.reset();
            if (m_displayMode == None)
                m_waitingForFirstImage = true;
        }
        ensureLayers();
        if (m_sampleBufferDisplayLayer) {
            if (!m_activeVideoTrack)
                m_sampleBufferDisplayLayer->clearEnqueuedSamples();
            m_sampleBufferDisplayLayer->updateDisplayMode(hideVideoLayer || m_displayMode < PausedImage, hideRootLayer());
        }

        m_pendingSelectedTrackCheck = false;
        updateDisplayMode();
    });
}

void MediaPlayerPrivateMediaStreamAVFObjC::updateTracks()
{
    MediaStreamTrackPrivateVector currentTracks = m_mediaStreamPrivate->tracks();

    auto setAudioTrackState = [this](AudioTrackPrivateMediaStream& track, int index, TrackState state)
    {
        switch (state) {
        case TrackState::Remove:
            track.streamTrack().removeObserver(*this);
            track.clear();
            m_player->removeAudioTrack(track);
            break;
        case TrackState::Add:
            track.streamTrack().addObserver(*this);
            m_player->addAudioTrack(track);
            break;
        case TrackState::Configure:
            track.setTrackIndex(index);
            bool enabled = track.streamTrack().enabled() && !track.streamTrack().muted();
            track.setEnabled(enabled);
            break;
        }
    };
    updateTracksOfType(m_audioTrackMap, RealtimeMediaSource::Type::Audio, currentTracks, &AudioTrackPrivateMediaStream::create, WTFMove(setAudioTrackState));

    auto setVideoTrackState = [this](VideoTrackPrivateMediaStream& track, int index, TrackState state)
    {
        switch (state) {
        case TrackState::Remove:
            track.streamTrack().removeObserver(*this);
            m_player->removeVideoTrack(track);
            checkSelectedVideoTrack();
            break;
        case TrackState::Add:
            track.streamTrack().addObserver(*this);
            m_player->addVideoTrack(track);
            break;
        case TrackState::Configure:
            track.setTrackIndex(index);
            bool selected = &track.streamTrack() == m_mediaStreamPrivate->activeVideoTrack();
            track.setSelected(selected);
            checkSelectedVideoTrack();
            break;
        }
    };
    updateTracksOfType(m_videoTrackMap, RealtimeMediaSource::Type::Video, currentTracks, &VideoTrackPrivateMediaStream::create, WTFMove(setVideoTrackState));
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateMediaStreamAVFObjC::seekable() const
{
    return makeUnique<PlatformTimeRanges>();
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateMediaStreamAVFObjC::buffered() const
{
    return makeUnique<PlatformTimeRanges>();
}

void MediaPlayerPrivateMediaStreamAVFObjC::paint(GraphicsContext& context, const FloatRect& rect)
{
    paintCurrentFrameInContext(context, rect);
}

void MediaPlayerPrivateMediaStreamAVFObjC::updateCurrentFrameImage()
{
    if (m_imagePainter.cgImage || !m_imagePainter.mediaSample)
        return;

    if (!m_imagePainter.pixelBufferConformer)
        m_imagePainter.pixelBufferConformer = makeUnique<PixelBufferConformerCV>((__bridge CFDictionaryRef)@{ (__bridge NSString *)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA) });

    ASSERT(m_imagePainter.pixelBufferConformer);
    if (!m_imagePainter.pixelBufferConformer)
        return;

    auto pixelBuffer = static_cast<CVPixelBufferRef>(CMSampleBufferGetImageBuffer(m_imagePainter.mediaSample->platformSample().sample.cmSampleBuffer));
    m_imagePainter.cgImage = m_imagePainter.pixelBufferConformer->createImageFromPixelBuffer(pixelBuffer);
}

void MediaPlayerPrivateMediaStreamAVFObjC::paintCurrentFrameInContext(GraphicsContext& context, const FloatRect& destRect)
{
    if (m_displayMode == None || !metaDataAvailable() || context.paintingDisabled())
        return;

    if (m_displayMode != PaintItBlack && m_imagePainter.mediaSample)
        updateCurrentFrameImage();

    GraphicsContextStateSaver stateSaver(context);
    if (m_displayMode == PaintItBlack) {
        context.fillRect(IntRect(IntPoint(), IntSize(destRect.width(), destRect.height())), Color::black);
        return;
    }

    if (!m_imagePainter.cgImage || !m_imagePainter.mediaSample)
        return;

    auto image = m_imagePainter.cgImage.get();
    FloatRect imageRect(0, 0, CGImageGetWidth(image), CGImageGetHeight(image));
    AffineTransform videoTransform = videoTransformationMatrix(*m_imagePainter.mediaSample);
    FloatRect transformedDestRect = videoTransform.inverse().valueOr(AffineTransform()).mapRect(destRect);
    context.concatCTM(videoTransform);
    context.drawNativeImage(image, imageRect.size(), transformedDestRect, imageRect);
}

void MediaPlayerPrivateMediaStreamAVFObjC::acceleratedRenderingStateChanged()
{
    if (m_player->renderingCanBeAccelerated())
        ensureLayers();
    else
        destroyLayers();
}

String MediaPlayerPrivateMediaStreamAVFObjC::engineDescription() const
{
    static NeverDestroyed<String> description(MAKE_STATIC_STRING_IMPL("AVFoundation MediaStream Engine"));
    return description;
}

void MediaPlayerPrivateMediaStreamAVFObjC::setReadyState(MediaPlayer::ReadyState readyState)
{
    if (m_readyState == readyState)
        return;

    if (readyState != MediaPlayer::ReadyState::HaveNothing)
        m_haveSeenMetadata = true;
    m_readyState = readyState;
    characteristicsChanged();

    m_player->readyStateChanged();
}

void MediaPlayerPrivateMediaStreamAVFObjC::setNetworkState(MediaPlayer::NetworkState networkState)
{
    if (m_networkState == networkState)
        return;

    m_networkState = networkState;
    m_player->networkStateChanged();
}

void MediaPlayerPrivateMediaStreamAVFObjC::setBufferingPolicy(MediaPlayer::BufferingPolicy policy)
{
    if (policy != MediaPlayer::BufferingPolicy::Default && m_sampleBufferDisplayLayer)
        m_sampleBufferDisplayLayer->flushAndRemoveImage();
}

void MediaPlayerPrivateMediaStreamAVFObjC::scheduleDeferredTask(Function<void ()>&& function)
{
    ASSERT(function);
    callOnMainThread([weakThis = makeWeakPtr(*this), function = WTFMove(function)] {
        if (!weakThis)
            return;

        function();
    });
}

void MediaPlayerPrivateMediaStreamAVFObjC::CurrentFramePainter::reset()
{
    cgImage = nullptr;
    mediaSample = nullptr;
    pixelBufferConformer = nullptr;
}

void MediaPlayerPrivateMediaStreamAVFObjC::updateDisplayLayer()
{
    if (!m_sampleBufferDisplayLayer)
        return;

    auto bounds = rootLayer().bounds;
    auto videoBounds = bounds;
    if (m_videoRotation == MediaSample::VideoRotation::Right || m_videoRotation == MediaSample::VideoRotation::Left)
        std::swap(videoBounds.size.width, videoBounds.size.height);

    m_sampleBufferDisplayLayer->updateBoundsAndPosition(videoBounds, { bounds.size.width / 2, bounds.size.height / 2});
}

void MediaPlayerPrivateMediaStreamAVFObjC::rootLayerBoundsDidChange()
{
    updateDisplayLayer();
}

#if !RELEASE_LOG_DISABLED
WTFLogChannel& MediaPlayerPrivateMediaStreamAVFObjC::logChannel() const
{
    return LogMedia;
}
#endif

}

#endif
