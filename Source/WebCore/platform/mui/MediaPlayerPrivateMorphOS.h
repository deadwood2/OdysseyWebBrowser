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

namespace WebCore
{

    enum
    {   IPC_CMD_NONE=0, IPC_CMD_LOAD, IPC_CMD_PLAY, IPC_CMD_PAUSE, IPC_CMD_SEEK, IPC_CMD_STOP};

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

        MediaPlayerPrivate(MediaPlayer*);
        ~MediaPlayerPrivate();
        static void registerMediaEngine(MediaEngineRegistrar);

        void didReceiveResponse(const ResourceResponse& response);
        void didFinishLoading();
        void didFailLoading(const ResourceError&);
        void didReceiveData(const char* data, unsigned length, int lengthReceived);

        void audioClose();
        static void fetchRequest(void *);

        FloatSize naturalSize() const override;
        bool hasVideo() const override;
        bool hasAudio() const override;

        void load(const String &url) override;
#if ENABLE(MEDIA_SOURCE)
        void load(const String& url, PassRefPtr<MediaSource>) override;
#endif
        void cancelLoad() override;

        void prepareToPlay() override;
        void play() override;
        void pause() override;

        bool paused() const override;
        bool seeking() const override;

        float duration() const override;
        float currentTime() const override;
        void seek(float) override;

        void setRate(float) override;
        void setVolume(float) override;

        MediaPlayer::NetworkState networkState() const override;
        MediaPlayer::ReadyState readyState() const override;

        virtual std::unique_ptr<PlatformTimeRanges> buffered() const override;
        float maxTimeSeekable() const override;
        unsigned long long totalBytes() const override;

        void setVisible(bool) override;
        void setSize(const IntSize&) override;
        virtual bool didLoadingProgress() const override;

        void paint(GraphicsContext*, const FloatRect&) override;
        bool hasSingleSecurityOrigin() const override;
        void setOutputPixelFormat(int pixfmt) override;
        bool supportsFullscreen() const override;

    private:

        bool totalBytesKnown() const;
        void didEnd();

        void repaint();

        void videoDecoder();
        void audioDecoder();
        void audioOutput();
        void playerLoop();
        bool audioOpen();
        void audioPause();
        void audioResume();
        void audioReset();
        void audioSetVolume(float volume);

        void sendCommand(IPCCommand cmd);
        IPCCommand waitCommand();
        bool commandInQueue();
        bool demux(bool &eof, bool &gotVideo, bool &videoFull, bool &gotAudio, bool &audioFull);

        MediaPlayer* player() {return m_player;}
        bool isEndReached() {return m_isEndReached;}

        void updateStates(MediaPlayer::NetworkState, MediaPlayer::ReadyState);
        float maxTimeLoaded() const;
        void cancelFetch();
        bool fetchData(unsigned long long startOffset);

        static void videoDecoderStart(void*);
        static void audioDecoderStart(void*);
        static void audioOutputStart(void*);
        static void playerLoopStart(void*);
        static void playerAdvance(void *);
        static void playerPaint(void *);
        static void callNetworkStateChanged(void *);
        static void callReadyStateChanged(void *);
        static void callTimeChanged(void *);

        static void getSupportedTypes(HashSet<String>&);
        static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters&);
        static bool isAvailable() {return true;}


        MediaPlayer* m_player;
        bool m_isEndReached;
        bool m_errorOccured;

        double m_volume;
        MediaPlayer::NetworkState m_networkState;
        MediaPlayer::ReadyState m_readyState;
        bool m_startedPlaying;
        mutable bool m_isStreaming;
        bool m_isSeeking;
        bool m_visible;
        FFMpegContext *m_ctx;
        WTF::Vector<IPCCommand> m_commandQueue;

        /* Thread/Synchronisation Handling */
        ThreadIdentifier m_thread;
        bool m_threadRunning;
        ThreadCondition m_condition;

        WTF::Vector<long long> m_pendingPaintQueue;
        mutable Mutex m_lock;
    };
}

#endif
#endif
