#include "config.h"
#include "MediaDescriptionMorphOS.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE)

namespace WebCore {

MediaDescriptionMorphOS::MediaDescriptionMorphOS(MediaDescriptionMorphOS::Type type, String&& codec)
	: MediaDescription(std::move(codec))
	, m_type(type)
{
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createVideoWithCodec(String&& codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Video, std::move(codec)));
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createAudioWithCodec(String&& codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Audio, std::move(codec)));
}

RefPtr<MediaDescription> MediaDescriptionMorphOS::createTextWithCodec(String&& codec)
{
	return adoptRef(*new MediaDescriptionMorphOS(Type::Text, std::move(codec)));
}

}

#endif
