/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#include "MediaSample.h"
#include "MediaTimeAVFoundation.h"

#include <runtime/Uint8ClampedArray.h>
#include <wtf/Vector.h>

namespace WebCore {
    
class MediaSampleAVFObjC final : public MediaSample {
public:
    static Ref<MediaSampleAVFObjC> create(CMSampleBufferRef sample, int trackID) { return adoptRef(*new MediaSampleAVFObjC(sample, trackID)); }
    static Ref<MediaSampleAVFObjC> create(CMSampleBufferRef sample, AtomicString trackID) { return adoptRef(*new MediaSampleAVFObjC(sample, trackID)); }
    static Ref<MediaSampleAVFObjC> create(CMSampleBufferRef sample, VideoRotation rotation = VideoRotation::None, bool mirrored = false) { return adoptRef(*new MediaSampleAVFObjC(sample, rotation, mirrored)); }
    static RefPtr<MediaSampleAVFObjC> createImageSample(Ref<JSC::Uint8ClampedArray>&&, unsigned long width, unsigned long height);
    static RefPtr<MediaSampleAVFObjC> createImageSample(Vector<uint8_t>&&, unsigned long width, unsigned long height);

    RefPtr<JSC::Uint8ClampedArray> getRGBAImageData() const final;

private:
    MediaSampleAVFObjC(CMSampleBufferRef sample)
        : m_sample(sample)
    {
    }
    MediaSampleAVFObjC(CMSampleBufferRef sample, AtomicString trackID)
        : m_sample(sample)
        , m_id(trackID)
    {
    }
    MediaSampleAVFObjC(CMSampleBufferRef sample, int trackID)
        : m_sample(sample)
        , m_id(String::format("%d", trackID))
    {
    }
    MediaSampleAVFObjC(CMSampleBufferRef sample, VideoRotation rotation, bool mirrored)
        : m_sample(sample)
        , m_rotation(rotation)
        , m_mirrored(mirrored)
    {
    }

    virtual ~MediaSampleAVFObjC() { }

    MediaTime presentationTime() const override;
    MediaTime outputPresentationTime() const override;
    MediaTime decodeTime() const override;
    MediaTime duration() const override;
    MediaTime outputDuration() const override;

    AtomicString trackID() const override { return m_id; }
    void setTrackID(const String& id) override { m_id = id; }

    size_t sizeInBytes() const override;
    FloatSize presentationSize() const override;

    SampleFlags flags() const override;
    PlatformSample platformSample() override;
    void dump(PrintStream&) const override;
    void offsetTimestampsBy(const MediaTime&) override;
    void setTimestamps(const MediaTime&, const MediaTime&) override;
    bool isDivisable() const override;
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime) override;
    Ref<MediaSample> createNonDisplayingCopy() const override;

    VideoRotation videoRotation() const final { return m_rotation; }
    bool videoMirrored() const final { return m_mirrored; }

    RetainPtr<CMSampleBufferRef> m_sample;
    AtomicString m_id;
    VideoRotation m_rotation { VideoRotation::None };
    bool m_mirrored { false };
};

}
