/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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


#pragma once

#if PLATFORM(IOS) || (PLATFORM(MAC) && ENABLE(VIDEO_PRESENTATION_MODE))

#include "FloatRect.h"
#include "HTMLMediaElementEnums.h"
#include "PlaybackSessionModel.h"

namespace WebCore {

class VideoFullscreenModelClient;

class VideoFullscreenModel {
public:
    virtual ~VideoFullscreenModel() { };
    virtual void addClient(VideoFullscreenModelClient&) = 0;
    virtual void removeClient(VideoFullscreenModelClient&)= 0;

    virtual void requestFullscreenMode(HTMLMediaElementEnums::VideoFullscreenMode, bool finishedWithMedia = false) = 0;
    virtual void setVideoLayerFrame(FloatRect) = 0;
    enum VideoGravity { VideoGravityResize, VideoGravityResizeAspect, VideoGravityResizeAspectFill };
    virtual void setVideoLayerGravity(VideoGravity) = 0;
    virtual void fullscreenModeChanged(HTMLMediaElementEnums::VideoFullscreenMode) = 0;

    virtual bool isVisible() const = 0;
    virtual FloatSize videoDimensions() const = 0;
    virtual bool hasVideo() const = 0;
};

class VideoFullscreenModelClient {
public:
    virtual ~VideoFullscreenModelClient() = default;
    virtual void hasVideoChanged(bool) = 0;
    virtual void videoDimensionsChanged(const FloatSize&) = 0;
};

WEBCORE_EXPORT bool supportsPictureInPicture();
    
}

#endif

