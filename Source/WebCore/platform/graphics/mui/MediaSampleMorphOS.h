#pragma once
#include "config.h"

#if ENABLE(VIDEO)

#include "MediaSample.h"
#include "FloatSize.h"
#include "AcinerellaPointer.h"
#include "AcinerellaMuxer.h"
#include <wtf/text/AtomString.h>

namespace WebCore {

class MediaSampleMorphOS final : public MediaSample {
	MediaSampleMorphOS(RefPtr<Acinerella::AcinerellaPackage>& sample, const FloatSize& presentationSize, const String& trackId);

public:
    static Ref<MediaSampleMorphOS> create(RefPtr<Acinerella::AcinerellaPackage>& sample, const FloatSize& presentationSize, const String& trackId)
    {
        return adoptRef(*new MediaSampleMorphOS(sample, presentationSize, trackId));
    }

    MediaTime presentationTime() const override { return m_pts; }
    MediaTime decodeTime() const override { return m_dts; }
    MediaTime duration() const override { return m_duration; }
    AtomString trackID() const override { return AtomString(m_trackId); }
    void setTrackID(const String& trackId) override { m_trackId = trackId; }
    size_t sizeInBytes() const override { return m_size; }
    FloatSize presentationSize() const override { return m_presentationSize; }
    void offsetTimestampsBy(const MediaTime&) override;
    void setTimestamps(const MediaTime&, const MediaTime&) override;
    bool isDivisable() const override { return false; }
    std::pair<RefPtr<MediaSample>, RefPtr<MediaSample>> divide(const MediaTime& presentationTime) override { (void)presentationTime; return { nullptr, nullptr }; }
    Ref<MediaSample> createNonDisplayingCopy() const override;

    SampleFlags flags() const override { return m_flags; }
    PlatformSample platformSample() override;
    void dump(PrintStream&) const override { };

	RefPtr<Acinerella::AcinerellaPackage> package() { return m_sample; }

protected:
    MediaTime m_pts;
    MediaTime m_dts;
    MediaTime m_duration;
    String m_trackId;
    size_t m_size;
    RefPtr<Acinerella::AcinerellaPackage> m_sample;
    FloatSize m_presentationSize;
    MediaSample::SampleFlags m_flags;
};

}

#endif
