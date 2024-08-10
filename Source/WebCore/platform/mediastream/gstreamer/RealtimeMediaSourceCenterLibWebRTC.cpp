/*
 * Copyright (C) 2018 Metrological Group B.V.
 * Author: Thibault Saunier <tsaunier@igalia.com>
 * Author: Alejandro G. Castro <alex@igalia.com>
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

#if ENABLE(MEDIA_STREAM) && USE(LIBWEBRTC)
#include "RealtimeMediaSourceCenterLibWebRTC.h"

#include "GStreamerAudioCaptureSource.h"
#include "GStreamerCaptureDevice.h"
#include "GStreamerCaptureDeviceManager.h"
#include "GStreamerVideoCaptureSource.h"
#include <wtf/MainThread.h>

namespace WebCore {

RealtimeMediaSource::AudioCaptureFactory& RealtimeMediaSourceCenterLibWebRTC::audioCaptureSourceFactory()
{
    return RealtimeMediaSourceCenterLibWebRTC::singleton().audioFactory();
}

RealtimeMediaSourceCenterLibWebRTC& RealtimeMediaSourceCenterLibWebRTC::singleton()
{
    ASSERT(isMainThread());
    static NeverDestroyed<RealtimeMediaSourceCenterLibWebRTC> center;
    return center;
}

RealtimeMediaSourceCenter& RealtimeMediaSourceCenter::platformCenter()
{
    return RealtimeMediaSourceCenterLibWebRTC::singleton();
}

RealtimeMediaSourceCenterLibWebRTC::RealtimeMediaSourceCenterLibWebRTC()
{
}

RealtimeMediaSourceCenterLibWebRTC::~RealtimeMediaSourceCenterLibWebRTC()
{
}

RealtimeMediaSource::AudioCaptureFactory& RealtimeMediaSourceCenterLibWebRTC::audioFactory()
{
    if (m_audioFactoryOverride)
        return *m_audioFactoryOverride;

    return GStreamerAudioCaptureSource::factory();
}

RealtimeMediaSource::VideoCaptureFactory& RealtimeMediaSourceCenterLibWebRTC::videoFactory()
{
    return GStreamerVideoCaptureSource::factory();
}

CaptureDeviceManager& RealtimeMediaSourceCenterLibWebRTC::audioCaptureDeviceManager()
{
    return GStreamerAudioCaptureDeviceManager::singleton();
}

CaptureDeviceManager& RealtimeMediaSourceCenterLibWebRTC::videoCaptureDeviceManager()
{
    return GStreamerVideoCaptureDeviceManager::singleton();
}

CaptureDeviceManager& RealtimeMediaSourceCenterLibWebRTC::displayCaptureDeviceManager()
{
    return GStreamerDisplayCaptureDeviceManager::singleton();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM) && USE(LIBWEBRTC)
