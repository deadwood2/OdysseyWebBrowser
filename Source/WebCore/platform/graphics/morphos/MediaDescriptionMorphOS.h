#pragma once

#include "config.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

#include "MediaDescription.h"
#include <wtf/text/AtomString.h>

namespace WebCore {

class MediaDescriptionMorphOS : public MediaDescription
{
	enum class Type {
		Video, Audio, Text,
	};

	MediaDescriptionMorphOS(Type type, const AtomString &codec);
	
public:
	~MediaDescriptionMorphOS() = default;

	static RefPtr<MediaDescription> createVideoWithCodec(const AtomString &codec);
	static RefPtr<MediaDescription> createAudioWithCodec(const AtomString &codec);
	static RefPtr<MediaDescription> createTextWithCodec(const AtomString &codec);

    AtomString codec() const override { return m_codec; }
    bool isVideo() const override { return m_type == Type::Video; }
    bool isAudio() const override { return m_type == Type::Audio; }
    bool isText() const override { return m_type == Type::Text; }

protected:
	AtomString m_codec;
	Type       m_type;
};

}

#endif
