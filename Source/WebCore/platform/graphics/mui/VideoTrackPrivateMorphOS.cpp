#include "config.h"

#include "VideoTrackPrivateMorphOS.h"

#define D(x) 

#if ENABLE(VIDEO)

namespace WebCore {

VideoTrackPrivateMorphOS::VideoTrackPrivateMorphOS(WeakPtr<MediaPlayerPrivateMorphOS> player, int index)
	: m_index(index)
	, m_player(player)
{
	m_id = "V" + String::number(index);
	D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
}

VideoTrackPrivate::Kind VideoTrackPrivateMorphOS::kind() const
{
	return VideoTrackPrivate::Kind();
}

void VideoTrackPrivateMorphOS::setSelected(bool setselected)
{
	if (setselected != selected())
	{
		VideoTrackPrivate::setSelected(setselected);
		if (m_player)
			m_player->onTrackEnabled(m_index, setselected);
	}
}

void VideoTrackPrivateMorphOS::disconnect()
{
	D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
	m_player = nullptr;
}

#if ENABLE(MEDIA_SOURCE)

void VideoTrackPrivateMorphOSMS::setSelected(bool setselected)
{
	if (setselected != selected())
	{
		VideoTrackPrivate::setSelected(setselected);
        if (m_source)
            m_source->onTrackEnabled(m_index, setselected);
    }
}

void VideoTrackPrivateMorphOSMS::disconnect()
{
    m_source = nullptr;
}

VideoTrackPrivateMorphOSMS::VideoTrackPrivateMorphOSMS(MediaSourceBufferPrivateMorphOS *source, int index)
    : VideoTrackPrivateMorphOS(nullptr, index)
    , m_source(source)
{
}

#endif

}

#endif
