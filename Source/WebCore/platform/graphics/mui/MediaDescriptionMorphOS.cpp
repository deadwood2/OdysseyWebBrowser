#include "MediaDescriptionMorphOS.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

namespace WebCore {

MediaDescriptionMorphOS::MediaDescriptionMorphOS(MediaDescriptionMorphOS::Type type, const AtomString &codec)
	: m_codec(codec)
	, m_type(type)
{
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createVideoWithCodec(const AtomString &codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Video, codec));
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createAudioWithCodec(const AtomString &codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Audio, codec));
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createTextWithCodec(const AtomString &codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Text, codec));
}

}

#endif
