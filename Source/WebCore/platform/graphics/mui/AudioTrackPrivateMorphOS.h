#pragma once

#if ENABLE(VIDEO)

#include "AudioTrackPrivate.h"
#include "MediaPlayerPrivateMorphOS.h"
#include "MediaSourceBufferPrivateMorphOS.h"
#include <wtf/WeakPtr.h>

namespace WebCore {

class AudioTrackPrivateMorphOS : public AudioTrackPrivate
{
public:

    static RefPtr<AudioTrackPrivateMorphOS> create(WeakPtr<MediaPlayerPrivateMorphOS> player, int index)
    {
        return adoptRef(*new AudioTrackPrivateMorphOS(player, index));
    }

    Kind kind() const final;

    virtual void disconnect();

    void setEnabled(bool) override;

    int trackIndex() const override { return m_index; }

    AtomicString id() const override { return AtomicString(m_id); }
    AtomicString label() const override { return AtomicString(m_label); }
    AtomicString language() const override { return AtomicString(m_language); }

protected:
    AudioTrackPrivateMorphOS(WeakPtr<MediaPlayerPrivateMorphOS>, int index);

	int m_index;
    String m_id;
    String m_label;
    String m_language;
    WeakPtr<MediaPlayerPrivateMorphOS> m_player;
};

#if ENABLE(MEDIA_SOURCE)
class AudioTrackPrivateMorphOSMS : public AudioTrackPrivateMorphOS
{
public:

    static RefPtr<AudioTrackPrivateMorphOSMS> create(MediaSourceBufferPrivateMorphOS *source, int index)
    {
        return adoptRef(*new AudioTrackPrivateMorphOSMS(source, index));
    }

    void disconnect() override;
    void setEnabled(bool) override;

protected:
    AudioTrackPrivateMorphOSMS(MediaSourceBufferPrivateMorphOS*, int index);
    MediaSourceBufferPrivateMorphOS *m_source;
};
#endif

}

#endif
