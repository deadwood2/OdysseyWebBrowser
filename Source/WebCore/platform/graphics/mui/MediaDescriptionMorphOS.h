#pragma once

#include "config.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

#include "MediaDescription.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

class MediaDescriptionMorphOS : public MediaDescription
{
	enum class Type {
		Video, Audio, Text,
	};

	MediaDescriptionMorphOS(Type type, const AtomicString &codec);
	
public:
	~MediaDescriptionMorphOS() = default;

	static RefPtr<MediaDescription> createVideoWithCodec(const AtomicString &codec);
	static RefPtr<MediaDescription> createAudioWithCodec(const AtomicString &codec);
	static RefPtr<MediaDescription> createTextWithCodec(const AtomicString &codec);

    AtomicString codec() const override { return m_codec; }
    bool isVideo() const override { return m_type == Type::Video; }
    bool isAudio() const override { return m_type == Type::Audio; }
    bool isText() const override { return m_type == Type::Text; }

protected:
	AtomicString m_codec;
	Type       m_type;
};

}

#endif
