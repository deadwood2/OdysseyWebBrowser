/*
 * Copyright (C) 2020 Jacek Piszczek
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#include "AudioBus.h"
#include "AudioDestination.h"
#include "AudioDestinationOutputMorphOS.h"

#include <wtf/RefPtr.h>

namespace WebCore {

class AudioDestinationMorphOS : public AudioDestination, public AudioDestinationRenderer {
public:
    AudioDestinationMorphOS(AudioIOCallback&, float sampleRate);
    ~AudioDestinationMorphOS();

    void start(Function<void(Function<void()>&&)>&& dispatchToRenderThread, CompletionHandler<void(bool)>&& = [](bool) { }) override;
    void stop(CompletionHandler<void(bool)>&& = [](bool) { }) override;

    bool isPlaying() override { return m_isPlaying; }
    float sampleRate() const override { return m_sampleRate; }

    unsigned framesPerBuffer() const final;

private:
    void startRendering(CompletionHandler<void(bool)>&&);
    void stopRendering(CompletionHandler<void(bool)>&&);
    void setIsPlaying(bool playing) { m_isPlaying = playing; }
    void render(int16_t *samplesStereo, size_t count) override;
    void renderOnRenderingTheadIfPlaying(int16_t *samplesStereo, size_t count);
    void renderOnRenderingThead(int16_t *samplesStereo, size_t count);

private:
    RefPtr<AudioBus> m_renderBus;
    AudioDestinationOutputMorphOS m_output;
    AudioIOPosition m_outputTimestamp;

    Lock m_dispatchToRenderThreadLock;
    Function<void(Function<void()>&&)> m_dispatchToRenderThread;

    float m_sampleRate;
    double m_sampleTime = 0;
    bool m_isPlaying;
    bool m_audioSinkAvailable;
};

} // namespace WebCore

