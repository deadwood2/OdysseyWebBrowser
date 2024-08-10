/*
 * Copyright (C) 2016 Metrological Group B.V.
 * Copyright (C) 2016 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#if ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(MEDIA_SOURCE)

#include "GStreamerCommon.h"
#include "MediaPlayerPrivateGStreamerMSE.h"
#include "MediaSourceClientGStreamerMSE.h"
#include "SourceBufferPrivateGStreamer.h"

#include <atomic>
#include <gst/gst.h>
#include <mutex>
#include <wtf/Condition.h>
#include <wtf/Threading.h>

namespace WebCore {

#if !LOG_DISABLED || ENABLE(ENCRYPTED_MEDIA)
struct PadProbeInformation {
    AppendPipeline* appendPipeline;
    const char* description;
    gulong probeId;
};
#endif

class AppendPipeline : public ThreadSafeRefCounted<AppendPipeline> {
public:
    enum class AppendState { Invalid, NotStarted, Ongoing, DataStarve, Sampling, LastSample, Aborting };

    AppendPipeline(Ref<MediaSourceClientGStreamerMSE>, Ref<SourceBufferPrivateGStreamer>, MediaPlayerPrivateGStreamerMSE&);
    virtual ~AppendPipeline();

    void handleNeedContextSyncMessage(GstMessage*);
    void handleApplicationMessage(GstMessage*);
    void handleStateChangeMessage(GstMessage*);

    gint id();
    AppendState appendState() { return m_appendState; }
    void setAppendState(AppendState);

    GstFlowReturn handleNewAppsinkSample(GstElement*);
    GstFlowReturn pushNewBuffer(GstBuffer*);

    // Takes ownership of caps.
    void parseDemuxerSrcPadCaps(GstCaps*);
    void appsinkCapsChanged();
    void appsinkNewSample(GRefPtr<GstSample>&&);
    void appsinkEOS();
    void handleEndOfAppend();
    void didReceiveInitializationSegment();
    AtomicString trackId();
    void abort();

    void clearPlayerPrivate();
    Ref<SourceBufferPrivateGStreamer> sourceBufferPrivate() { return m_sourceBufferPrivate.get(); }
    GstBus* bus() { return m_bus.get(); }
    GstElement* pipeline() { return m_pipeline.get(); }
    GstElement* appsrc() { return m_appsrc.get(); }
    GstElement* appsink() { return m_appsink.get(); }
    GstCaps* demuxerSrcPadCaps() { return m_demuxerSrcPadCaps.get(); }
    GstCaps* appsinkCaps() { return m_appsinkCaps.get(); }
    MediaPlayerPrivateGStreamerMSE* playerPrivate() { return m_playerPrivate; }
    RefPtr<WebCore::TrackPrivateBase> track() { return m_track; }
    WebCore::MediaSourceStreamTypeGStreamer streamType() { return m_streamType; }

    void disconnectDemuxerSrcPadFromAppsinkFromAnyThread(GstPad*);
    void appendPipelineDemuxerNoMorePadsFromAnyThread();
    void connectDemuxerSrcPadToAppsinkFromAnyThread(GstPad*);
    void connectDemuxerSrcPadToAppsink(GstPad*);

private:
    void resetPipeline();
    void checkEndOfAppend();
    void demuxerNoMorePads();

    void consumeAppsinkAvailableSamples();

    GstPadProbeReturn appsrcEndOfAppendCheckerProbe(GstPadProbeInfo*);

    static void staticInitialization();

    static std::once_flag s_staticInitializationFlag;
    static GType s_endOfAppendMetaType;
    static const GstMetaInfo* s_webKitEndOfAppendMetaInfo;

    // Used only for asserting that there is only one streaming thread.
    // Only the pointers are compared.
    WTF::Thread* m_streamingThread;

    Ref<MediaSourceClientGStreamerMSE> m_mediaSourceClient;
    Ref<SourceBufferPrivateGStreamer> m_sourceBufferPrivate;
    MediaPlayerPrivateGStreamerMSE* m_playerPrivate;

    // (m_mediaType, m_id) is unique.
    gint m_id;

    MediaTime m_initialDuration;

    GRefPtr<GstElement> m_pipeline;
    GRefPtr<GstBus> m_bus;
    GRefPtr<GstElement> m_appsrc;
    GRefPtr<GstElement> m_demux;
    GRefPtr<GstElement> m_parser; // Optional.
    // The demuxer has one src stream only, so only one appsink is needed and linked to it.
    GRefPtr<GstElement> m_appsink;

    // Used to avoid unnecessary notifications per sample.
    // It is read and written from the streaming thread and written from the main thread.
    // The main thread must set it to false before actually pulling samples.
    // This strategy ensures that at any time, there are at most two notifications in the bus
    // queue, instead of it growing unbounded.
    std::atomic_flag m_wasBusAlreadyNotifiedOfAvailableSamples;

    Lock m_padAddRemoveLock;
    Condition m_padAddRemoveCondition;

    GRefPtr<GstCaps> m_appsinkCaps;
    GRefPtr<GstCaps> m_demuxerSrcPadCaps;
    FloatSize m_presentationSize;

#if !LOG_DISABLED
    struct PadProbeInformation m_demuxerDataEnteringPadProbeInformation;
    struct PadProbeInformation m_appsinkDataEnteringPadProbeInformation;
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    struct PadProbeInformation m_appsinkPadEventProbeInformation;
#endif
    // Keeps track of the states of append processing, to avoid performing actions inappropriate for the current state
    // (eg: processing more samples when the last one has been detected, etc.). See setAppendState() for valid
    // transitions.
    AppendState m_appendState;

    // Aborts can only be completed when the normal sample detection has finished. Meanwhile, the willing to abort is
    // expressed in this field.
    bool m_abortPending;

    WebCore::MediaSourceStreamTypeGStreamer m_streamType;
    RefPtr<WebCore::TrackPrivateBase> m_track;

    GRefPtr<GstBuffer> m_pendingBuffer;
};

} // namespace WebCore.

#endif // USE(GSTREAMER)
