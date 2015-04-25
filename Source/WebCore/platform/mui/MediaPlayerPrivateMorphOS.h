/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef MediaPlayerPrivateMorphOS_h
#define MediaPlayerPrivateMorphOS_h

#if ENABLE(VIDEO)

#include <wtf/Threading.h>

#include "MediaPlayerPrivate.h"
#include "ResourceError.h"
#include "ResourceResponse.h"
#include "Timer.h"


namespace WebCore {

	enum { IPC_CMD_NONE=0, IPC_CMD_LOAD, IPC_CMD_PLAY, IPC_CMD_PAUSE, IPC_CMD_SEEK, IPC_CMD_STOP };

	class IPCCommand
	{
	public:
		IPCCommand(int cmd, float seekTo = 0.0)
		: m_command(cmd)
		, m_seekTo(seekTo)
		{}

		IPCCommand()
		: m_command(IPC_CMD_NONE)
		, m_seekTo(0.0)
		{}

		int id()
		{
			return m_command;
		}

		float seekTo()
		{
			return m_seekTo;
		}

		const char* name()
		{
			const char *name;
			switch(id())
			{
				case IPC_CMD_LOAD:
					name = "IPC_CMD_LOAD";
					break;
				case IPC_CMD_PLAY:
					name = "IPC_CMD_PLAY";
					break;
					case IPC_CMD_PAUSE:
					name = "IPC_CMD_PAUSE";
					break;
				case IPC_CMD_SEEK:
					name = "IPC_CMD_SEEK";
					break;
				case IPC_CMD_STOP:
					name = "IPC_CMD_STOP";
					break;
				default:
					name = "<unknown>";
					break;
			}

			return name;
		}

	private:
		int m_command;
		float m_seekTo;
	};

    class GraphicsContext;
    class IntSize;
    class IntRect;
    class FFMpegContext;

    class MediaPlayerPrivate : public MediaPlayerPrivateInterface
    {
    public:

        static void registerMediaEngine(MediaEngineRegistrar);
        MediaPlayerPrivate(MediaPlayer*);
        ~MediaPlayerPrivate();

        FloatSize naturalSize() const;
        bool hasVideo() const;
        bool hasAudio() const;

        void load(const String &url);
#if ENABLE(MEDIA_SOURCE)
	void load(const String& url, PassRefPtr<MediaSource>);
#endif
        void cancelLoad();

		void prepareToPlay();
        void play();
        void pause();

        bool paused() const;
        bool seeking() const;

        float duration() const;
        float currentTime() const;
        void seek(float);

        void setRate(float);
        void setVolume(float);
	void volumeChanged();

        int dataRate() const;

        MediaPlayer::NetworkState networkState() const;
        MediaPlayer::ReadyState readyState() const;

        virtual std::unique_ptr<PlatformTimeRanges> buffered() const;
        float maxTimeSeekable() const;
        unsigned bytesLoaded() const;
        bool totalBytesKnown() const;
        unsigned long long totalBytes() const;

        void setVisible(bool);
        void setSize(const IntSize&);

	virtual bool didLoadingProgress() const;

        void loadStateChanged();
        void sizeChanged();
        void timeChanged();
        void didEnd();
	void durationChanged();
	void loadingFailed(MediaPlayer::NetworkState);

        void repaint();
        void paint(GraphicsContext*, const FloatRect&) override;

	bool hasSingleSecurityOrigin() const;

	void setOutputPixelFormat(int pixfmt);

	bool supportsFullscreen() const;

	bool supportsSave() const;

	/**/
	FFMpegContext *privatectx();

	void videoDecoder();
	void audioDecoder();
	void audioOutput();
	void playerLoop();
	bool audioOpen();
	void audioClose();
	void audioPause();
	void audioResume();
	void audioReset();
	void audioSetVolume(float volume);

	void sendCommand(IPCCommand cmd);
	IPCCommand waitCommand();
	bool commandInQueue();
	bool demux(bool &eof, bool &gotVideo, bool &videoFull, bool &gotAudio, bool &audioFull);
	void didReceiveResponse(const ResourceResponse& response);
	void didFinishLoading();
	void didFailLoading(const ResourceError&);
	void didReceiveData(const char* data, unsigned length, int lengthReceived);
	bool fetchData(unsigned long long startOffset);
	void cancelFetch();
	MediaPlayer* player() { return m_player; }
	bool isEndReached() { return m_isEndReached; }

	static void videoDecoderStart(void*);
	static void audioDecoderStart(void*);
	static void audioOutputStart(void*);
	static void playerLoopStart(void*);
	static void  playerAdvance(void *);
	static void  playerPaint(void *);
	static void  fetchRequest(void *);
	static void callNetworkStateChanged(void *);
	static void callReadyStateChanged(void *);
	static void callTimeChanged(void *);
	/**/

    private:

        static void getSupportedTypes(HashSet<String>&);
	static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);
        static bool isAvailable() { return true; }

	void updateStates(MediaPlayer::NetworkState, MediaPlayer::ReadyState);
        void cancelSeek();
        float maxTimeLoaded() const;

    private:
        MediaPlayer* m_player;
        float m_rate;
        float m_endTime;
        bool m_isEndReached;
	bool m_errorOccured;

        double m_volume;
        MediaPlayer::NetworkState m_networkState;
        MediaPlayer::ReadyState m_readyState;
        bool m_startedPlaying;
        mutable bool m_isStreaming;
	bool m_isSeeking;
        IntSize m_size;
        bool m_visible;
	FFMpegContext *m_ctx;
	WTF::Vector<IPCCommand> m_commandQueue;

	/* Thread/Synchronisation Handling */
	ThreadIdentifier m_thread;
	bool m_threadRunning;
	ThreadCondition m_condition;

    public:
	WTF::Vector<long long> m_pendingPaintQueue;
	mutable Mutex m_lock;
    };
}

#endif
#endif
