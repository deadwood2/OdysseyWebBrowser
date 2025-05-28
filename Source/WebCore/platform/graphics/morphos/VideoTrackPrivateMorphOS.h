#pragma once

#if ENABLE(VIDEO)

#include "VideoTrackPrivate.h"
#include "MediaPlayerPrivateMorphOS.h"
#include "MediaSourceBufferPrivateMorphOS.h"
#include <wtf/WeakPtr.h>

namespace WebCore {

class VideoTrackPrivateMorphOS : public VideoTrackPrivate
{
public:

    static RefPtr<VideoTrackPrivateMorphOS> create(WeakPtr<MediaPlayerPrivateMorphOS> player, int index)
    {
        return adoptRef(*new VideoTrackPrivateMorphOS(player, index));
    }

    Kind kind() const final;
	void setSelected(bool selected) override;
    virtual void disconnect();

    int trackIndex() const override { return m_index; }

    AtomString id() const override { return AtomString(m_id); }
    AtomString label() const override { return AtomString(m_label); }
    AtomString language() const override { return AtomString(m_language); }

protected:
    VideoTrackPrivateMorphOS(WeakPtr<MediaPlayerPrivateMorphOS>, int index);

	int m_index;
    String m_id;
    String m_label;
    String m_language;
    WeakPtr<MediaPlayerPrivateMorphOS> m_player;
};

#if ENABLE(MEDIA_SOURCE)
class VideoTrackPrivateMorphOSMS final : public VideoTrackPrivateMorphOS
{
public:

    static RefPtr<VideoTrackPrivateMorphOSMS> create(MediaSourceBufferPrivateMorphOS* source, int index)
    {
        return adoptRef(*new VideoTrackPrivateMorphOSMS(source, index));
    }

	void setSelected(bool selected) override;
    void disconnect() override;

private:
    VideoTrackPrivateMorphOSMS(MediaSourceBufferPrivateMorphOS *source, int index);
    MediaSourceBufferPrivateMorphOS *m_source;
};
#endif

}

#endif
