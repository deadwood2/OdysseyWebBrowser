#include "MediaDescriptionMorphOS.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

namespace WebCore {

MediaDescriptionMorphOS::MediaDescriptionMorphOS(MediaDescriptionMorphOS::Type type, const AtomicString &codec)
	: m_codec(codec)
	, m_type(type)
{
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createVideoWithCodec(const AtomicString &codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Video, codec));
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createAudioWithCodec(const AtomicString &codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Audio, codec));
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createTextWithCodec(const AtomicString &codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Text, codec));
}

}

#endif
