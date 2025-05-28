#include "config.h"

#include "AudioTrackPrivateMorphOS.h"

#define D(x) 

#if ENABLE(VIDEO)

namespace WebCore {

AudioTrackPrivateMorphOS::AudioTrackPrivateMorphOS(WeakPtr<MediaPlayerPrivateMorphOS> player, int index)
	: m_index(index)
	, m_player(player)
{
	m_id = "A" + String::number(index);
	D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
}

AudioTrackPrivate::Kind AudioTrackPrivateMorphOS::kind() const
{
	return AudioTrackPrivate::Kind();
}

void AudioTrackPrivateMorphOS::disconnect()
{
	D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
	m_player = nullptr;
}

void AudioTrackPrivateMorphOS::setEnabled(bool setenabled)
{
	if (setenabled != enabled())
	{
		AudioTrackPrivate::setEnabled(setenabled);
		if (m_player)
			m_player->onTrackEnabled(m_index, setenabled);
	}
}

#if ENABLE(MEDIA_SOURCE)

AudioTrackPrivateMorphOSMS::AudioTrackPrivateMorphOSMS(MediaSourceBufferPrivateMorphOS *source, int index)
    : AudioTrackPrivateMorphOS(nullptr, index)
    , m_source(source)
{

}

void AudioTrackPrivateMorphOSMS::disconnect()
{
    m_source = nullptr;
}

void AudioTrackPrivateMorphOSMS::setEnabled(bool setenabled)
{
	if (setenabled != enabled())
	{
		AudioTrackPrivate::setEnabled(setenabled);
		if (m_source)
			m_source->onTrackEnabled(m_index, setenabled);
	}

}

#endif

}

#endif
