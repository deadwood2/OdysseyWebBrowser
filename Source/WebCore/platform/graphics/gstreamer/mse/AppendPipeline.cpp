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

#include "config.h"
#include "AppendPipeline.h"

#if ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(MEDIA_SOURCE)

#include "AudioTrackPrivateGStreamer.h"
#include "GRefPtrGStreamer.h"
#include "GStreamerMediaDescription.h"
#include "GStreamerMediaSample.h"
#include "GStreamerUtilities.h"
#include "InbandTextTrackPrivateGStreamer.h"
#include "MediaDescription.h"
#include "SourceBufferPrivateGStreamer.h"
#include "VideoTrackPrivateGStreamer.h"
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>
#include <wtf/Condition.h>
#include <wtf/glib/GLibUtilities.h>
#include <wtf/glib/RunLoopSourcePriority.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_mse_debug);
#define GST_CAT_DEFAULT webkit_mse_debug

namespace WebCore {

static const char* dumpAppendState(AppendPipeline::AppendState appendState)
{
    switch (appendState) {
    case AppendPipeline::AppendState::Invalid:
        return "Invalid";
    case AppendPipeline::AppendState::NotStarted:
        return "NotStarted";
    case AppendPipeline::AppendState::Ongoing:
        return "Ongoing";
    case AppendPipeline::AppendState::KeyNegotiation:
        return "KeyNegotiation";
    case AppendPipeline::AppendState::DataStarve:
        return "DataStarve";
    case AppendPipeline::AppendState::Sampling:
        return "Sampling";
    case AppendPipeline::AppendState::LastSample:
        return "LastSample";
    case AppendPipeline::AppendState::Aborting:
        return "Aborting";
    default:
        return "(unknown)";
    }
}

static void appendPipelineAppsrcNeedData(GstAppSrc*, guint, AppendPipeline*);
static void appendPipelineDemuxerPadAdded(GstElement*, GstPad*, AppendPipeline*);
static void appendPipelineDemuxerPadRemoved(GstElement*, GstPad*, AppendPipeline*);
static void appendPipelineAppsinkCapsChanged(GObject*, GParamSpec*, AppendPipeline*);
static GstPadProbeReturn appendPipelineAppsrcDataLeaving(GstPad*, GstPadProbeInfo*, AppendPipeline*);
#if !LOG_DISABLED
static GstPadProbeReturn appendPipelinePadProbeDebugInformation(GstPad*, GstPadProbeInfo*, struct PadProbeInformation*);
#endif
static GstPadProbeReturn appendPipelineDemuxerBlackHolePadProbe(GstPad*, GstPadProbeInfo*, gpointer);
static GstFlowReturn appendPipelineAppsinkNewSample(GstElement*, AppendPipeline*);
static void appendPipelineAppsinkEOS(GstElement*, AppendPipeline*);

static void appendPipelineNeedContextMessageCallback(GstBus*, GstMessage* message, AppendPipeline* appendPipeline)
{
    GST_TRACE("received callback");
    appendPipeline->handleNeedContextSyncMessage(message);
}

static void appendPipelineApplicationMessageCallback(GstBus*, GstMessage* message, AppendPipeline* appendPipeline)
{
    appendPipeline->handleApplicationMessage(message);
}

AppendPipeline::AppendPipeline(Ref<MediaSourceClientGStreamerMSE> mediaSourceClient, Ref<SourceBufferPrivateGStreamer> sourceBufferPrivate, MediaPlayerPrivateGStreamerMSE& playerPrivate)
    : m_mediaSourceClient(mediaSourceClient.get())
    , m_sourceBufferPrivate(sourceBufferPrivate.get())
    , m_playerPrivate(&playerPrivate)
    , m_id(0)
    , m_appsrcAtLeastABufferLeft(false)
    , m_appsrcNeedDataReceived(false)
    , m_appsrcDataLeavingProbeId(0)
    , m_appendState(AppendState::NotStarted)
    , m_abortPending(false)
    , m_streamType(Unknown)
{
    ASSERT(WTF::isMainThread());

    GST_TRACE("Creating AppendPipeline (%p)", this);

    // FIXME: give a name to the pipeline, maybe related with the track it's managing.
    // The track name is still unknown at this time, though.
    m_pipeline = gst_pipeline_new(nullptr);

    m_bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline.get())));
    gst_bus_add_signal_watch_full(m_bus.get(), RunLoopSourcePriority::RunLoopDispatcher);
    gst_bus_enable_sync_message_emission(m_bus.get());

    g_signal_connect(m_bus.get(), "sync-message::need-context", G_CALLBACK(appendPipelineNeedContextMessageCallback), this);
    g_signal_connect(m_bus.get(), "message::application", G_CALLBACK(appendPipelineApplicationMessageCallback), this);

    // We assign the created instances here instead of adoptRef() because gst_bin_add_many()
    // below will already take the initial reference and we need an additional one for us.
    m_appsrc = gst_element_factory_make("appsrc", nullptr);
    m_demux = gst_element_factory_make("qtdemux", nullptr);
    m_appsink = gst_element_factory_make("appsink", nullptr);

    gst_app_sink_set_emit_signals(GST_APP_SINK(m_appsink.get()), TRUE);
    gst_base_sink_set_sync(GST_BASE_SINK(m_appsink.get()), FALSE);

    GRefPtr<GstPad> appsinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));
    g_signal_connect(appsinkPad.get(), "notify::caps", G_CALLBACK(appendPipelineAppsinkCapsChanged), this);

    setAppsrcDataLeavingProbe();

#if !LOG_DISABLED
    GRefPtr<GstPad> demuxerPad = adoptGRef(gst_element_get_static_pad(m_demux.get(), "sink"));
    m_demuxerDataEnteringPadProbeInformation.appendPipeline = this;
    m_demuxerDataEnteringPadProbeInformation.description = "demuxer data entering";
    m_demuxerDataEnteringPadProbeInformation.probeId = gst_pad_add_probe(demuxerPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelinePadProbeDebugInformation), &m_demuxerDataEnteringPadProbeInformation, nullptr);
    m_appsinkDataEnteringPadProbeInformation.appendPipeline = this;
    m_appsinkDataEnteringPadProbeInformation.description = "appsink data entering";
    m_appsinkDataEnteringPadProbeInformation.probeId = gst_pad_add_probe(appsinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelinePadProbeDebugInformation), &m_appsinkDataEnteringPadProbeInformation, nullptr);
#endif

    // These signals won't be connected outside of the lifetime of "this".
    g_signal_connect(m_appsrc.get(), "need-data", G_CALLBACK(appendPipelineAppsrcNeedData), this);
    g_signal_connect(m_demux.get(), "pad-added", G_CALLBACK(appendPipelineDemuxerPadAdded), this);
    g_signal_connect(m_demux.get(), "pad-removed", G_CALLBACK(appendPipelineDemuxerPadRemoved), this);
    g_signal_connect(m_appsink.get(), "new-sample", G_CALLBACK(appendPipelineAppsinkNewSample), this);
    g_signal_connect(m_appsink.get(), "eos", G_CALLBACK(appendPipelineAppsinkEOS), this);

    // Add_many will take ownership of a reference. That's why we used an assignment before.
    gst_bin_add_many(GST_BIN(m_pipeline.get()), m_appsrc.get(), m_demux.get(), nullptr);
    gst_element_link(m_appsrc.get(), m_demux.get());

    gst_element_set_state(m_pipeline.get(), GST_STATE_READY);
};

AppendPipeline::~AppendPipeline()
{
    ASSERT(WTF::isMainThread());

    {
        LockHolder locker(m_newSampleLock);
        setAppendState(AppendState::Invalid);
        m_newSampleCondition.notifyOne();
    }

    {
        LockHolder locker(m_padAddRemoveLock);
        m_playerPrivate = nullptr;
        m_padAddRemoveCondition.notifyOne();
    }

    GST_TRACE("Destroying AppendPipeline (%p)", this);

    // FIXME: Maybe notify appendComplete here?

    if (m_pipeline) {
        ASSERT(m_bus);
        gst_bus_remove_signal_watch(m_bus.get());
        gst_element_set_state(m_pipeline.get(), GST_STATE_NULL);
        m_pipeline = nullptr;
    }

    if (m_appsrc) {
        removeAppsrcDataLeavingProbe();
        g_signal_handlers_disconnect_by_data(m_appsrc.get(), this);
        m_appsrc = nullptr;
    }

    if (m_demux) {
#if !LOG_DISABLED
        GRefPtr<GstPad> demuxerPad = adoptGRef(gst_element_get_static_pad(m_demux.get(), "sink"));
        gst_pad_remove_probe(demuxerPad.get(), m_demuxerDataEnteringPadProbeInformation.probeId);
#endif

        g_signal_handlers_disconnect_by_data(m_demux.get(), this);
        m_demux = nullptr;
    }

    if (m_appsink) {
        GRefPtr<GstPad> appsinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));
        g_signal_handlers_disconnect_by_data(appsinkPad.get(), this);
        g_signal_handlers_disconnect_by_data(m_appsink.get(), this);

#if !LOG_DISABLED
        gst_pad_remove_probe(appsinkPad.get(), m_appsinkDataEnteringPadProbeInformation.probeId);
#endif

        m_appsink = nullptr;
    }

    m_appsinkCaps = nullptr;
    m_demuxerSrcPadCaps = nullptr;
};

void AppendPipeline::clearPlayerPrivate()
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("cleaning private player");

    {
        LockHolder locker(m_newSampleLock);
        // Make sure that AppendPipeline won't process more data from now on and
        // instruct handleNewSample to abort itself from now on as well.
        setAppendState(AppendState::Invalid);

        // Awake any pending handleNewSample operation in the streaming thread.
        m_newSampleCondition.notifyOne();
    }

    {
        LockHolder locker(m_padAddRemoveLock);
        m_playerPrivate = nullptr;
        m_padAddRemoveCondition.notifyOne();
    }

    // And now that no handleNewSample operations will remain stalled waiting
    // for the main thread, stop the pipeline.
    if (m_pipeline)
        gst_element_set_state(m_pipeline.get(), GST_STATE_NULL);
}

void AppendPipeline::handleNeedContextSyncMessage(GstMessage* message)
{
    const gchar* contextType = nullptr;
    gst_message_parse_context_type(message, &contextType);
    GST_TRACE("context type: %s", contextType);
    if (!g_strcmp0(contextType, "drm-preferred-decryption-system-id"))
        setAppendState(AppendPipeline::AppendState::KeyNegotiation);

    // MediaPlayerPrivateGStreamerBase will take care of setting up encryption.
    if (m_playerPrivate)
        m_playerPrivate->handleSyncMessage(message);
}

void AppendPipeline::handleApplicationMessage(GstMessage* message)
{
    ASSERT(WTF::isMainThread());

    const GstStructure* structure = gst_message_get_structure(message);

    if (gst_structure_has_name(structure, "appsrc-need-data")) {
        handleAppsrcNeedDataReceived();
        return;
    }

    if (gst_structure_has_name(structure, "appsrc-buffer-left")) {
        handleAppsrcAtLeastABufferLeft();
        return;
    }

    if (gst_structure_has_name(structure, "demuxer-connect-to-appsink")) {
        GRefPtr<GstPad> demuxerSrcPad;
        gst_structure_get(structure, "demuxer-src-pad", G_TYPE_OBJECT, &demuxerSrcPad.outPtr(), nullptr);
        ASSERT(demuxerSrcPad);
        connectDemuxerSrcPadToAppsink(demuxerSrcPad.get());
        return;
    }

    if (gst_structure_has_name(structure, "appsink-caps-changed")) {
        appsinkCapsChanged();
        return;
    }

    if (gst_structure_has_name(structure, "appsink-new-sample")) {
        GRefPtr<GstSample> newSample;
        gst_structure_get(structure, "new-sample", GST_TYPE_SAMPLE, &newSample.outPtr(), nullptr);

        appsinkNewSample(newSample.get());
        return;
    }

    if (gst_structure_has_name(structure, "appsink-eos")) {
        appsinkEOS();
        return;
    }

    ASSERT_NOT_REACHED();
}

void AppendPipeline::handleAppsrcNeedDataReceived()
{
    if (!m_appsrcAtLeastABufferLeft) {
        GST_TRACE("discarding until at least a buffer leaves appsrc");
        return;
    }

    ASSERT(m_appendState == AppendState::Ongoing || m_appendState == AppendState::Sampling);
    ASSERT(!m_appsrcNeedDataReceived);

    GST_TRACE("received need-data from appsrc");

    m_appsrcNeedDataReceived = true;
    checkEndOfAppend();
}

void AppendPipeline::handleAppsrcAtLeastABufferLeft()
{
    m_appsrcAtLeastABufferLeft = true;
    GST_TRACE("received buffer-left from appsrc");
#if LOG_DISABLED
    removeAppsrcDataLeavingProbe();
#endif
}

gint AppendPipeline::id()
{
    ASSERT(WTF::isMainThread());

    if (m_id)
        return m_id;

    static gint s_totalAudio = 0;
    static gint s_totalVideo = 0;
    static gint s_totalText = 0;

    switch (m_streamType) {
    case Audio:
        m_id = ++s_totalAudio;
        break;
    case Video:
        m_id = ++s_totalVideo;
        break;
    case Text:
        m_id = ++s_totalText;
        break;
    case Unknown:
    case Invalid:
        GST_ERROR("Trying to get id for a pipeline of Unknown/Invalid type");
        ASSERT_NOT_REACHED();
        break;
    }

    GST_DEBUG("streamType=%d, id=%d", static_cast<int>(m_streamType), m_id);

    return m_id;
}

void AppendPipeline::setAppendState(AppendState newAppendState)
{
    ASSERT(WTF::isMainThread());
    // Valid transitions:
    // NotStarted-->Ongoing-->DataStarve-->NotStarted
    //           |         |            `->Aborting-->NotStarted
    //           |         `->Sampling-···->Sampling-->LastSample-->NotStarted
    //           |         |                                     `->Aborting-->NotStarted
    //           |         `->KeyNegotiation-->Ongoing-->[...]
    //           `->Aborting-->NotStarted
    AppendState oldAppendState = m_appendState;
    AppendState nextAppendState = AppendState::Invalid;

    if (oldAppendState != newAppendState)
        GST_TRACE("%s --> %s", dumpAppendState(oldAppendState), dumpAppendState(newAppendState));

    bool ok = false;

    switch (oldAppendState) {
    case AppendState::NotStarted:
        switch (newAppendState) {
        case AppendState::Ongoing:
            ok = true;
            gst_element_set_state(m_pipeline.get(), GST_STATE_PLAYING);
            break;
        case AppendState::NotStarted:
            ok = true;
            if (m_pendingBuffer) {
                GST_TRACE("pushing pending buffer %p", m_pendingBuffer.get());
                gst_app_src_push_buffer(GST_APP_SRC(appsrc()), m_pendingBuffer.leakRef());
                nextAppendState = AppendState::Ongoing;
            }
            break;
        case AppendState::Aborting:
            ok = true;
            nextAppendState = AppendState::NotStarted;
            break;
        case AppendState::Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case AppendState::KeyNegotiation:
        switch (newAppendState) {
        case AppendState::Ongoing:
        case AppendState::Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case AppendState::Ongoing:
        switch (newAppendState) {
        case AppendState::KeyNegotiation:
        case AppendState::Sampling:
        case AppendState::Invalid:
            ok = true;
            break;
        case AppendState::DataStarve:
            ok = true;
            GST_DEBUG("received all pending samples");
            m_sourceBufferPrivate->didReceiveAllPendingSamples();
            if (m_abortPending)
                nextAppendState = AppendState::Aborting;
            else
                nextAppendState = AppendState::NotStarted;
            break;
        default:
            break;
        }
        break;
    case AppendState::DataStarve:
        switch (newAppendState) {
        case AppendState::NotStarted:
        case AppendState::Invalid:
            ok = true;
            break;
        case AppendState::Aborting:
            ok = true;
            nextAppendState = AppendState::NotStarted;
            break;
        default:
            break;
        }
        break;
    case AppendState::Sampling:
        switch (newAppendState) {
        case AppendState::Sampling:
        case AppendState::Invalid:
            ok = true;
            break;
        case AppendState::LastSample:
            ok = true;
            GST_DEBUG("received all pending samples");
            m_sourceBufferPrivate->didReceiveAllPendingSamples();
            if (m_abortPending)
                nextAppendState = AppendState::Aborting;
            else
                nextAppendState = AppendState::NotStarted;
            break;
        default:
            break;
        }
        break;
    case AppendState::LastSample:
        switch (newAppendState) {
        case AppendState::NotStarted:
        case AppendState::Invalid:
            ok = true;
            break;
        case AppendState::Aborting:
            ok = true;
            nextAppendState = AppendState::NotStarted;
            break;
        default:
            break;
        }
        break;
    case AppendState::Aborting:
        switch (newAppendState) {
        case AppendState::NotStarted:
            ok = true;
            resetPipeline();
            m_abortPending = false;
            nextAppendState = AppendState::NotStarted;
            break;
        case AppendState::Invalid:
            ok = true;
            break;
        default:
            break;
        }
        break;
    case AppendState::Invalid:
        ok = true;
        break;
    }

    if (ok)
        m_appendState = newAppendState;
    else
        GST_ERROR("Invalid append state transition %s --> %s", dumpAppendState(oldAppendState), dumpAppendState(newAppendState));

    ASSERT(ok);

    if (nextAppendState != AppendState::Invalid)
        setAppendState(nextAppendState);
}

void AppendPipeline::parseDemuxerSrcPadCaps(GstCaps* demuxerSrcPadCaps)
{
    ASSERT(WTF::isMainThread());

    m_demuxerSrcPadCaps = adoptGRef(demuxerSrcPadCaps);
    m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Unknown;

    GstStructure* structure = gst_caps_get_structure(m_demuxerSrcPadCaps.get(), 0);
    bool sizeConfigured = false;

#if GST_CHECK_VERSION(1, 5, 3) && ENABLE(ENCRYPTED_MEDIA)
    if (gst_structure_has_name(structure, "application/x-cenc")) {
        // Any previous decryptor should have been removed from the pipeline by disconnectFromAppSinkFromStreamingThread()
        ASSERT(!m_decryptor);

        m_decryptor = WebCore::createGstDecryptor(gst_structure_get_string(structure, "protection-system"));
        if (!m_decryptor) {
            GST_ERROR("decryptor not found for caps: %" GST_PTR_FORMAT, m_demuxerSrcPadCaps.get());
            return;
        }

        const gchar* originalMediaType = gst_structure_get_string(structure, "original-media-type");

        if (!MediaPlayerPrivateGStreamerMSE::supportsCodecs(originalMediaType)) {
            m_presentationSize = WebCore::FloatSize();
            m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Invalid;
        } else if (g_str_has_prefix(originalMediaType, "video/")) {
            int width = 0;
            int height = 0;
            float finalHeight = 0;

            if (gst_structure_get_int(structure, "width", &width) && gst_structure_get_int(structure, "height", &height)) {
                int ratioNumerator = 1;
                int ratioDenominator = 1;

                gst_structure_get_fraction(structure, "pixel-aspect-ratio", &ratioNumerator, &ratioDenominator);
                finalHeight = height * ((float) ratioDenominator / (float) ratioNumerator);
            }

            m_presentationSize = WebCore::FloatSize(width, finalHeight);
            m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Video;
        } else {
            m_presentationSize = WebCore::FloatSize();
            if (g_str_has_prefix(originalMediaType, "audio/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Audio;
            else if (g_str_has_prefix(originalMediaType, "text/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Text;
        }
        sizeConfigured = true;
    }
#endif

    if (!sizeConfigured) {
        const char* structureName = gst_structure_get_name(structure);
        GstVideoInfo info;

        if (!MediaPlayerPrivateGStreamerMSE::supportsCodecs(structureName)) {
            m_presentationSize = WebCore::FloatSize();
            m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Invalid;
        } else if (g_str_has_prefix(structureName, "video/") && gst_video_info_from_caps(&info, demuxerSrcPadCaps)) {
            float width, height;

            width = info.width;
            height = info.height * ((float) info.par_d / (float) info.par_n);

            m_presentationSize = WebCore::FloatSize(width, height);
            m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Video;
        } else {
            m_presentationSize = WebCore::FloatSize();
            if (g_str_has_prefix(structureName, "audio/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Audio;
            else if (g_str_has_prefix(structureName, "text/"))
                m_streamType = WebCore::MediaSourceStreamTypeGStreamer::Text;
        }
    }
}

void AppendPipeline::appsinkCapsChanged()
{
    ASSERT(WTF::isMainThread());

    if (!m_appsink)
        return;

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));
    GRefPtr<GstCaps> caps = adoptGRef(gst_pad_get_current_caps(pad.get()));

    if (!caps)
        return;

    // This means that we're right after a new track has appeared. Otherwise, it's a caps change inside the same track.
    bool previousCapsWereNull = !m_appsinkCaps;

    if (m_appsinkCaps != caps) {
        m_appsinkCaps = WTFMove(caps);
        if (m_playerPrivate)
            m_playerPrivate->trackDetected(this, m_track, previousCapsWereNull);
        didReceiveInitializationSegment();
        gst_element_set_state(m_pipeline.get(), GST_STATE_PLAYING);
    }
}

void AppendPipeline::checkEndOfAppend()
{
    ASSERT(WTF::isMainThread());

    if (!m_appsrcNeedDataReceived || (m_appendState != AppendState::Ongoing && m_appendState != AppendState::Sampling))
        return;

    GST_TRACE("end of append data mark was received");

    switch (m_appendState) {
    case AppendState::Ongoing:
        GST_TRACE("DataStarve");
        m_appsrcNeedDataReceived = false;
        setAppendState(AppendState::DataStarve);
        break;
    case AppendState::Sampling:
        GST_TRACE("LastSample");
        m_appsrcNeedDataReceived = false;
        setAppendState(AppendState::LastSample);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

void AppendPipeline::appsinkNewSample(GstSample* sample)
{
    ASSERT(WTF::isMainThread());

    {
        LockHolder locker(m_newSampleLock);

        // Ignore samples if we're not expecting them. Refuse processing if we're in Invalid state.
        if (m_appendState != AppendState::Ongoing && m_appendState != AppendState::Sampling) {
            GST_WARNING("Unexpected sample, appendState=%s", dumpAppendState(m_appendState));
            // FIXME: Return ERROR and find a more robust way to detect that all the
            // data has been processed, so we don't need to resort to these hacks.
            // All in all, return OK, even if it's not the proper thing to do. We don't want to break the demuxer.
            m_flowReturn = GST_FLOW_OK;
            m_newSampleCondition.notifyOne();
            return;
        }

        RefPtr<GStreamerMediaSample> mediaSample = WebCore::GStreamerMediaSample::create(sample, m_presentationSize, trackId());

        GST_TRACE("append: trackId=%s PTS=%f presentationSize=%.0fx%.0f", mediaSample->trackID().string().utf8().data(), mediaSample->presentationTime().toFloat(), mediaSample->presentationSize().width(), mediaSample->presentationSize().height());

        // If we're beyond the duration, ignore this sample and the remaining ones.
        MediaTime duration = m_mediaSourceClient->duration();
        if (duration.isValid() && !duration.indefiniteTime() && mediaSample->presentationTime() > duration) {
            GST_DEBUG("Detected sample (%f) beyond the duration (%f), declaring LastSample", mediaSample->presentationTime().toFloat(), duration.toFloat());
            setAppendState(AppendState::LastSample);
            m_flowReturn = GST_FLOW_OK;
            m_newSampleCondition.notifyOne();
            return;
        }

        // Add a gap sample if a gap is detected before the first sample.
        if (mediaSample->decodeTime() == MediaTime::zeroTime()
            && mediaSample->presentationTime() > MediaTime::zeroTime()
            && mediaSample->presentationTime() <= MediaTime::createWithDouble(0.1)) {
            GST_DEBUG("Adding gap offset");
            mediaSample->applyPtsOffset(MediaTime::zeroTime());
        }

        m_sourceBufferPrivate->didReceiveSample(*mediaSample);
        setAppendState(AppendState::Sampling);
        m_flowReturn = GST_FLOW_OK;
        m_newSampleCondition.notifyOne();
    }

    checkEndOfAppend();
}

void AppendPipeline::appsinkEOS()
{
    ASSERT(WTF::isMainThread());

    switch (m_appendState) {
    case AppendState::Aborting:
        // Ignored. Operation completion will be managed by the Aborting->NotStarted transition.
        return;
    case AppendState::Ongoing:
        // Finish Ongoing and Sampling states.
        setAppendState(AppendState::DataStarve);
        break;
    case AppendState::Sampling:
        setAppendState(AppendState::LastSample);
        break;
    default:
        GST_DEBUG("Unexpected EOS");
        break;
    }
}

void AppendPipeline::didReceiveInitializationSegment()
{
    ASSERT(WTF::isMainThread());

    WebCore::SourceBufferPrivateClient::InitializationSegment initializationSegment;

    GST_DEBUG("Notifying SourceBuffer for track %s", (m_track) ? m_track->id().string().utf8().data() : nullptr);
    initializationSegment.duration = m_mediaSourceClient->duration();

    switch (m_streamType) {
    case Audio: {
        WebCore::SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
        info.track = static_cast<AudioTrackPrivateGStreamer*>(m_track.get());
        info.description = WebCore::GStreamerMediaDescription::create(m_demuxerSrcPadCaps.get());
        initializationSegment.audioTracks.append(info);
        break;
    }
    case Video: {
        WebCore::SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
        info.track = static_cast<VideoTrackPrivateGStreamer*>(m_track.get());
        info.description = WebCore::GStreamerMediaDescription::create(m_demuxerSrcPadCaps.get());
        initializationSegment.videoTracks.append(info);
        break;
    }
    default:
        GST_ERROR("Unsupported stream type or codec");
        break;
    }

    m_sourceBufferPrivate->didReceiveInitializationSegment(initializationSegment);
}

AtomicString AppendPipeline::trackId()
{
    ASSERT(WTF::isMainThread());

    if (!m_track)
        return AtomicString();

    return m_track->id();
}

void AppendPipeline::resetPipeline()
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("resetting pipeline");
    m_appsrcAtLeastABufferLeft = false;
    setAppsrcDataLeavingProbe();

    {
        LockHolder locker(m_newSampleLock);
        m_newSampleCondition.notifyOne();
        gst_element_set_state(m_pipeline.get(), GST_STATE_READY);
        gst_element_get_state(m_pipeline.get(), nullptr, nullptr, 0);
    }

#if (!(LOG_DISABLED || defined(GST_DISABLE_GST_DEBUG)))
    {
        static unsigned i = 0;
        // This is here for debugging purposes. It does not make sense to have it as class member.
        WTF::String  dotFileName = String::format("reset-pipeline-%d", ++i);
        gst_debug_bin_to_dot_file(GST_BIN(m_pipeline.get()), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.utf8().data());
    }
#endif

}

void AppendPipeline::setAppsrcDataLeavingProbe()
{
    if (m_appsrcDataLeavingProbeId)
        return;

    GST_TRACE("setting appsrc data leaving probe");

    GRefPtr<GstPad> appsrcPad = adoptGRef(gst_element_get_static_pad(m_appsrc.get(), "src"));
    m_appsrcDataLeavingProbeId = gst_pad_add_probe(appsrcPad.get(), GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelineAppsrcDataLeaving), this, nullptr);
}

void AppendPipeline::removeAppsrcDataLeavingProbe()
{
    if (!m_appsrcDataLeavingProbeId)
        return;

    GST_TRACE("removing appsrc data leaving probe");

    GRefPtr<GstPad> appsrcPad = adoptGRef(gst_element_get_static_pad(m_appsrc.get(), "src"));
    gst_pad_remove_probe(appsrcPad.get(), m_appsrcDataLeavingProbeId);
    m_appsrcDataLeavingProbeId = 0;
}

void AppendPipeline::abort()
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("aborting");

    m_pendingBuffer = nullptr;

    // Abort already ongoing.
    if (m_abortPending)
        return;

    m_abortPending = true;
    if (m_appendState == AppendState::NotStarted)
        setAppendState(AppendState::Aborting);
    // Else, the automatic state transitions will take care when the ongoing append finishes.
}

GstFlowReturn AppendPipeline::pushNewBuffer(GstBuffer* buffer)
{
    GstFlowReturn result;

    if (m_abortPending) {
        m_pendingBuffer = adoptGRef(buffer);
        result = GST_FLOW_OK;
    } else {
        setAppendState(AppendPipeline::AppendState::Ongoing);
        GST_TRACE("pushing new buffer %p", buffer);
        result = gst_app_src_push_buffer(GST_APP_SRC(appsrc()), buffer);
    }

    return result;
}

void AppendPipeline::reportAppsrcAtLeastABufferLeft()
{
    GST_TRACE("buffer left appsrc, reposting to bus");
    GstStructure* structure = gst_structure_new_empty("appsrc-buffer-left");
    GstMessage* message = gst_message_new_application(GST_OBJECT(m_appsrc.get()), structure);
    gst_bus_post(m_bus.get(), message);
}

void AppendPipeline::reportAppsrcNeedDataReceived()
{
    GST_TRACE("received need-data signal at appsrc, reposting to bus");
    GstStructure* structure = gst_structure_new_empty("appsrc-need-data");
    GstMessage* message = gst_message_new_application(GST_OBJECT(m_appsrc.get()), structure);
    gst_bus_post(m_bus.get(), message);
}

GstFlowReturn AppendPipeline::handleNewAppsinkSample(GstElement* appsink)
{
    ASSERT(!WTF::isMainThread());

    // Even if we're disabled, it's important to pull the sample out anyway to
    // avoid deadlocks when changing to GST_STATE_NULL having a non empty appsink.
    GRefPtr<GstSample> sample = adoptGRef(gst_app_sink_pull_sample(GST_APP_SINK(appsink)));
    LockHolder locker(m_newSampleLock);

    if (!m_playerPrivate || m_appendState == AppendState::Invalid) {
        GST_WARNING("AppendPipeline has been disabled, ignoring this sample");
        return GST_FLOW_ERROR;
    }

    GstStructure* structure = gst_structure_new("appsink-new-sample", "new-sample", GST_TYPE_SAMPLE, sample.get(), nullptr);
    GstMessage* message = gst_message_new_application(GST_OBJECT(appsink), structure);
    gst_bus_post(m_bus.get(), message);
    GST_TRACE("appsink-new-sample message posted to bus");

    m_newSampleCondition.wait(m_newSampleLock);
    // We've been awaken because the sample was processed or because of
    // an exceptional condition (entered in Invalid state, destructor, etc.).
    // We can't reliably delete info here, appendPipelineAppsinkNewSampleMainThread will do it.

    return m_flowReturn;
}

void AppendPipeline::connectDemuxerSrcPadToAppsinkFromAnyThread(GstPad* demuxerSrcPad)
{
    if (!m_appsink)
        return;

    GST_DEBUG("connecting to appsink");

    if (m_demux->numsrcpads > 1) {
        GST_WARNING("Only one stream per SourceBuffer is allowed! Ignoring stream %d by adding a black hole probe.", m_demux->numsrcpads);
        gulong probeId = gst_pad_add_probe(demuxerSrcPad, GST_PAD_PROBE_TYPE_BUFFER, reinterpret_cast<GstPadProbeCallback>(appendPipelineDemuxerBlackHolePadProbe), nullptr, nullptr);
        g_object_set_data(G_OBJECT(demuxerSrcPad), "blackHoleProbeId", GULONG_TO_POINTER(probeId));
        return;
    }

    GRefPtr<GstPad> appsinkSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));

    // Only one stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(appsinkSinkPad.get()));

    gint64 timeLength = 0;
    if (gst_element_query_duration(m_demux.get(), GST_FORMAT_TIME, &timeLength)
        && static_cast<guint64>(timeLength) != GST_CLOCK_TIME_NONE)
        m_initialDuration = MediaTime(GST_TIME_AS_USECONDS(timeLength), G_USEC_PER_SEC);
    else
        m_initialDuration = MediaTime::positiveInfiniteTime();

    if (WTF::isMainThread())
        connectDemuxerSrcPadToAppsink(demuxerSrcPad);
    else {
        // Call connectDemuxerSrcPadToAppsink() in the main thread and wait.
        LockHolder locker(m_padAddRemoveLock);
        if (!m_playerPrivate)
            return;

        GstStructure* structure = gst_structure_new("demuxer-connect-to-appsink", "demuxer-src-pad", G_TYPE_OBJECT, demuxerSrcPad, nullptr);
        GstMessage* message = gst_message_new_application(GST_OBJECT(m_demux.get()), structure);
        gst_bus_post(m_bus.get(), message);
        GST_TRACE("demuxer-connect-to-appsink message posted to bus");

        m_padAddRemoveCondition.wait(m_padAddRemoveLock);
    }

    // Must be done in the thread we were called from (usually streaming thread).
    bool isData = (m_streamType == WebCore::MediaSourceStreamTypeGStreamer::Audio)
        || (m_streamType == WebCore::MediaSourceStreamTypeGStreamer::Video)
        || (m_streamType == WebCore::MediaSourceStreamTypeGStreamer::Text);

    if (isData) {
        // FIXME: Only add appsink one time. This method can be called several times.
        GRefPtr<GstObject> parent = adoptGRef(gst_element_get_parent(m_appsink.get()));
        if (!parent)
            gst_bin_add(GST_BIN(m_pipeline.get()), m_appsink.get());

#if ENABLE(ENCRYPTED_MEDIA)
        if (m_decryptor) {
            gst_object_ref(m_decryptor.get());
            gst_bin_add(GST_BIN(m_pipeline.get()), m_decryptor.get());

            GRefPtr<GstPad> decryptorSinkPad = adoptGRef(gst_element_get_static_pad(m_decryptor.get(), "sink"));
            gst_pad_link(demuxerSrcPad, decryptorSinkPad.get());

            GRefPtr<GstPad> decryptorSrcPad = adoptGRef(gst_element_get_static_pad(m_decryptor.get(), "src"));
            gst_pad_link(decryptorSrcPad.get(), appsinkSinkPad.get());

            gst_element_sync_state_with_parent(m_appsink.get());
            gst_element_sync_state_with_parent(m_decryptor.get());
        } else {
#endif
            gst_pad_link(demuxerSrcPad, appsinkSinkPad.get());
            gst_element_sync_state_with_parent(m_appsink.get());
#if ENABLE(ENCRYPTED_MEDIA)
        }
#endif
        gst_element_set_state(m_pipeline.get(), GST_STATE_PAUSED);
    }
}

void AppendPipeline::connectDemuxerSrcPadToAppsink(GstPad* demuxerSrcPad)
{
    ASSERT(WTF::isMainThread());
    GST_DEBUG("Connecting to appsink");

    LockHolder locker(m_padAddRemoveLock);
    GRefPtr<GstPad> sinkSinkPad = adoptGRef(gst_element_get_static_pad(m_appsink.get(), "sink"));

    // Only one stream per demuxer is supported.
    ASSERT(!gst_pad_is_linked(sinkSinkPad.get()));

    GRefPtr<GstCaps> caps = adoptGRef(gst_pad_get_current_caps(GST_PAD(demuxerSrcPad)));

    if (!caps || m_appendState == AppendState::Invalid || !m_playerPrivate) {
        m_padAddRemoveCondition.notifyOne();
        return;
    }

#ifndef GST_DISABLE_GST_DEBUG
    {
        GUniquePtr<gchar> strcaps(gst_caps_to_string(caps.get()));
        GST_DEBUG("%s", strcaps.get());
    }
#endif

    if (m_initialDuration > m_mediaSourceClient->duration()
        || (m_mediaSourceClient->duration().isInvalid() && m_initialDuration > MediaTime::zeroTime()))
        m_mediaSourceClient->durationChanged(m_initialDuration);

    parseDemuxerSrcPadCaps(gst_caps_ref(caps.get()));

    switch (m_streamType) {
    case WebCore::MediaSourceStreamTypeGStreamer::Audio:
        if (m_playerPrivate)
            m_track = WebCore::AudioTrackPrivateGStreamer::create(m_playerPrivate->pipeline(), id(), sinkSinkPad.get());
        break;
    case WebCore::MediaSourceStreamTypeGStreamer::Video:
        if (m_playerPrivate)
            m_track = WebCore::VideoTrackPrivateGStreamer::create(m_playerPrivate->pipeline(), id(), sinkSinkPad.get());
        break;
    case WebCore::MediaSourceStreamTypeGStreamer::Text:
        m_track = WebCore::InbandTextTrackPrivateGStreamer::create(id(), sinkSinkPad.get());
        break;
    case WebCore::MediaSourceStreamTypeGStreamer::Invalid:
        {
            GUniquePtr<gchar> strcaps(gst_caps_to_string(caps.get()));
            GST_DEBUG("Unsupported track codec: %s", strcaps.get());
        }
        // This is going to cause an error which will detach the SourceBuffer and tear down this
        // AppendPipeline, so we need the padAddRemove lock released before continuing.
        m_track = nullptr;
        m_padAddRemoveCondition.notifyOne();
        locker.unlockEarly();
        didReceiveInitializationSegment();
        return;
    default:
        // No useful data, but notify anyway to complete the append operation.
        GST_DEBUG("Received all pending samples (no data)");
        m_sourceBufferPrivate->didReceiveAllPendingSamples();
        break;
    }

    m_padAddRemoveCondition.notifyOne();
}

void AppendPipeline::disconnectDemuxerSrcPadFromAppsinkFromAnyThread(GstPad* demuxerSrcPad)
{
    // Must be done in the thread we were called from (usually streaming thread).
    if (!gst_pad_is_linked(demuxerSrcPad)) {
        gulong probeId = GPOINTER_TO_ULONG(g_object_get_data(G_OBJECT(demuxerSrcPad), "blackHoleProbeId"));
        if (probeId) {
            GST_DEBUG("Disconnecting black hole probe.");
            g_object_set_data(G_OBJECT(demuxerSrcPad), "blackHoleProbeId", nullptr);
            gst_pad_remove_probe(demuxerSrcPad, probeId);
        } else
            GST_WARNING("Not disconnecting demuxer src pad because it wasn't linked");
        return;
    }

    GST_DEBUG("Disconnecting appsink");

#if ENABLE(ENCRYPTED_MEDIA)
    if (m_decryptor) {
        gst_element_unlink(m_decryptor.get(), m_appsink.get());
        gst_element_unlink(m_demux.get(), m_decryptor.get());
        gst_element_set_state(m_decryptor.get(), GST_STATE_NULL);
        gst_bin_remove(GST_BIN(m_pipeline.get()), m_decryptor.get());
    } else
#endif
        gst_element_unlink(m_demux.get(), m_appsink.get());
}

static void appendPipelineAppsinkCapsChanged(GObject* appsinkPad, GParamSpec*, AppendPipeline* appendPipeline)
{
    GstStructure* structure = gst_structure_new_empty("appsink-caps-changed");
    GstMessage* message = gst_message_new_application(GST_OBJECT(appsinkPad), structure);
    gst_bus_post(appendPipeline->bus(), message);
    GST_TRACE("appsink-caps-changed message posted to bus");
}

static GstPadProbeReturn appendPipelineAppsrcDataLeaving(GstPad*, GstPadProbeInfo* info, AppendPipeline* appendPipeline)
{
    ASSERT(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER);

    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    gsize bufferSize = gst_buffer_get_size(buffer);

    GST_TRACE("buffer of size %" G_GSIZE_FORMAT " going thru", bufferSize);

    appendPipeline->reportAppsrcAtLeastABufferLeft();

    return GST_PAD_PROBE_OK;
}

#if !LOG_DISABLED
static GstPadProbeReturn appendPipelinePadProbeDebugInformation(GstPad*, GstPadProbeInfo* info, struct PadProbeInformation* padProbeInformation)
{
    ASSERT(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GST_TRACE("%s: buffer of size %" G_GSIZE_FORMAT " going thru", padProbeInformation->description, gst_buffer_get_size(buffer));
    return GST_PAD_PROBE_OK;
}
#endif

static GstPadProbeReturn appendPipelineDemuxerBlackHolePadProbe(GstPad*, GstPadProbeInfo* info, gpointer)
{
    ASSERT(GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GST_TRACE("buffer of size %" G_GSIZE_FORMAT " ignored", gst_buffer_get_size(buffer));
    return GST_PAD_PROBE_DROP;
}

static void appendPipelineAppsrcNeedData(GstAppSrc*, guint, AppendPipeline* appendPipeline)
{
    appendPipeline->reportAppsrcNeedDataReceived();
}

static void appendPipelineDemuxerPadAdded(GstElement*, GstPad* demuxerSrcPad, AppendPipeline* appendPipeline)
{
    appendPipeline->connectDemuxerSrcPadToAppsinkFromAnyThread(demuxerSrcPad);
}

static void appendPipelineDemuxerPadRemoved(GstElement*, GstPad* demuxerSrcPad, AppendPipeline* appendPipeline)
{
    appendPipeline->disconnectDemuxerSrcPadFromAppsinkFromAnyThread(demuxerSrcPad);
}

static GstFlowReturn appendPipelineAppsinkNewSample(GstElement* appsink, AppendPipeline* appendPipeline)
{
    return appendPipeline->handleNewAppsinkSample(appsink);
}

static void appendPipelineAppsinkEOS(GstElement*, AppendPipeline* appendPipeline)
{
    if (WTF::isMainThread())
        appendPipeline->appsinkEOS();
    else {
        GstStructure* structure = gst_structure_new_empty("appsink-eos");
        GstMessage* message = gst_message_new_application(GST_OBJECT(appendPipeline->appsink()), structure);
        gst_bus_post(appendPipeline->bus(), message);
        GST_TRACE("appsink-eos message posted to bus");
    }

    GST_DEBUG("%s main thread", (WTF::isMainThread()) ? "Is" : "Not");
}



} // namespace WebCore.

#endif // USE(GSTREAMER)
