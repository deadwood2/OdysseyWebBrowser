#pragma once

#if ENABLE(VIDEO)

#include "MediaSample.h"
#include "FloatSize.h"
#include "AcinerellaPointer.h"
#include "AcinerellaMuxer.h"
#include <wtf/text/AtomString.h>

namespace WebCore {

class MediaSampleMorphOS final : public MediaSample {
	MediaSampleMorphOS(RefPtr<Acinerella::AcinerellaPackage>& sample, const FloatSize& presentationSize, TrackID trackId);
	virtual ~MediaSampleMorphOS();
public:
    static Ref<MediaSampleMorphOS> create(RefPtr<Acinerella::AcinerellaPackage>& sample, const FloatSize& presentationSize, TrackID trackId)
    {
        return adoptRef(*new MediaSampleMorphOS(sample, presentationSize, trackId));
    }

    MediaTime presentationTime() const override { return m_pts; }
    MediaTime decodeTime() const override { return m_dts; }
    MediaTime duration() const override { return m_duration; }
    TrackID trackID() const override { return m_trackId; }
    size_t sizeInBytes() const override { return m_size; }
    FloatSize presentationSize() const override { return m_presentationSize; }
    void offsetTimestampsBy(const MediaTime&) override;
    void setTimestamps(const MediaTime&, const MediaTime&) override;
    bool isDivisable() const override { return false; }
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime&, UseEndTime = UseEndTime::DoNotUse) override { return { nullptr, nullptr }; }
    Ref<MediaSample> createNonDisplayingCopy() const override;
    std::optional<MediaSample::ByteRange> byteRange() const override;

    SampleFlags flags() const override { return m_flags; }
    PlatformSample platformSample() const override;
    PlatformSample::Type platformSampleType() const override { return PlatformSample::MorphOSSampleType; }
    void dump(PrintStream&) const override { };

	RefPtr<Acinerella::AcinerellaPackage> package() { return m_sample; }

protected:
    MediaTime m_pts;
    MediaTime m_dts;
    MediaTime m_duration;
    TrackID m_trackId;
    size_t m_size;
    RefPtr<Acinerella::AcinerellaPackage> m_sample;
    FloatSize m_presentationSize;
    MediaSample::SampleFlags m_flags;
};

}

#endif
