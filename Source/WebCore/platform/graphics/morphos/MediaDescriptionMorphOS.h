#pragma once

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

#include "MediaDescription.h"
#include <wtf/text/AtomString.h>

namespace WebCore {

class MediaDescriptionMorphOS : public MediaDescription
{
	enum class Type {
		Video, Audio, Text,
	};

	MediaDescriptionMorphOS(Type type, String&& codec);
	
public:
	~MediaDescriptionMorphOS() = default;

	static RefPtr<MediaDescription> createVideoWithCodec(String&& codec);
	static RefPtr<MediaDescription> createAudioWithCodec(String&& codec);
	static RefPtr<MediaDescription> createTextWithCodec(String&& codec);

    bool isVideo() const override { return m_type == Type::Video; }
    bool isAudio() const override { return m_type == Type::Audio; }
    bool isText() const override { return m_type == Type::Text; }

protected:
	Type         m_type;
};

}

#endif
