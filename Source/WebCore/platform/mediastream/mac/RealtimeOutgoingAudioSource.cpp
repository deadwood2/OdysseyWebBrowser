/*
 * Copyright (C) 2017 Apple Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RealtimeOutgoingAudioSource.h"

#if USE(LIBWEBRTC)

#include "CAAudioStreamDescription.h"
#include "LibWebRTCAudioFormat.h"
#include "LibWebRTCProvider.h"

namespace WebCore {

static inline AudioStreamBasicDescription libwebrtcAudioFormat(Float64 sampleRate, size_t channelCount)
{
    // FIXME: Microphones can have more than two channels. In such case, we should do the mix down based on AudioChannelLayoutTag.
    size_t libWebRTCChannelCount = channelCount >= 2 ? 2 : channelCount;
    AudioStreamBasicDescription streamFormat;
    FillOutASBDForLPCM(streamFormat, sampleRate, libWebRTCChannelCount, LibWebRTCAudioFormat::sampleSize, LibWebRTCAudioFormat::sampleSize, LibWebRTCAudioFormat::isFloat, LibWebRTCAudioFormat::isBigEndian, LibWebRTCAudioFormat::isNonInterleaved);
    return streamFormat;
}

RealtimeOutgoingAudioSource::RealtimeOutgoingAudioSource(Ref<MediaStreamTrackPrivate>&& audioSource)
    : m_audioSource(WTFMove(audioSource))
    , m_sampleConverter(AudioSampleDataSource::create(LibWebRTCAudioFormat::sampleRate * 2))
    , m_silenceAudioTimer(*this, &RealtimeOutgoingAudioSource::sendSilence)
{
    m_audioSource->addObserver(*this);
    initializeConverter();
}

bool RealtimeOutgoingAudioSource::setSource(Ref<MediaStreamTrackPrivate>&& newSource)
{
    m_audioSource->removeObserver(*this);
    m_audioSource = WTFMove(newSource);
    m_audioSource->addObserver(*this);

    initializeConverter();
    return true;
}

void RealtimeOutgoingAudioSource::initializeConverter()
{
    m_muted = m_audioSource->muted();
    m_enabled = m_audioSource->enabled();
    handleMutedIfNeeded();
}

void RealtimeOutgoingAudioSource::stop()
{
    m_silenceAudioTimer.stop();
    m_audioSource->removeObserver(*this);
}

void RealtimeOutgoingAudioSource::sourceMutedChanged()
{
    m_muted = m_audioSource->muted();
    handleMutedIfNeeded();
}

void RealtimeOutgoingAudioSource::sourceEnabledChanged()
{
    m_enabled = m_audioSource->enabled();
    handleMutedIfNeeded();
}

void RealtimeOutgoingAudioSource::handleMutedIfNeeded()
{
    bool isSilenced = m_muted || !m_enabled;
    m_sampleConverter->setMuted(isSilenced);
    if (isSilenced && !m_silenceAudioTimer.isActive())
        m_silenceAudioTimer.startRepeating(1_s);
    if (!isSilenced && m_silenceAudioTimer.isActive())
        m_silenceAudioTimer.stop();
}

void RealtimeOutgoingAudioSource::sendSilence()
{
    LibWebRTCProvider::callOnWebRTCSignalingThread([this, protectedThis = makeRef(*this)] {
        size_t chunkSampleCount = m_outputStreamDescription.sampleRate() / 100;
        size_t bufferSize = chunkSampleCount * LibWebRTCAudioFormat::sampleByteSize * m_outputStreamDescription.numberOfChannels();

        if (!bufferSize)
            return;

        m_audioBuffer.grow(bufferSize);
        memset(m_audioBuffer.data(), 0, bufferSize);
        for (auto sink : m_sinks)
            sink->OnData(m_audioBuffer.data(), LibWebRTCAudioFormat::sampleSize, m_outputStreamDescription.sampleRate(), m_outputStreamDescription.numberOfChannels(), chunkSampleCount);
    });
}

bool RealtimeOutgoingAudioSource::isReachingBufferedAudioDataHighLimit()
{
    auto writtenAudioDuration = m_writeCount / m_inputStreamDescription.sampleRate();
    auto readAudioDuration = m_readCount / m_outputStreamDescription.sampleRate();

    ASSERT(writtenAudioDuration >= readAudioDuration);
    return writtenAudioDuration > readAudioDuration + 0.5;
}

bool RealtimeOutgoingAudioSource::isReachingBufferedAudioDataLowLimit()
{
    auto writtenAudioDuration = m_writeCount / m_inputStreamDescription.sampleRate();
    auto readAudioDuration = m_readCount / m_outputStreamDescription.sampleRate();

    ASSERT(writtenAudioDuration >= readAudioDuration);
    return writtenAudioDuration < readAudioDuration + 0.1;
}

void RealtimeOutgoingAudioSource::audioSamplesAvailable(const MediaTime&, const PlatformAudioData& audioData, const AudioStreamDescription& streamDescription, size_t sampleCount)
{
    if (m_inputStreamDescription != streamDescription) {
        m_inputStreamDescription = toCAAudioStreamDescription(streamDescription);
        auto status  = m_sampleConverter->setInputFormat(m_inputStreamDescription);
        ASSERT_UNUSED(status, !status);

        m_outputStreamDescription = libwebrtcAudioFormat(LibWebRTCAudioFormat::sampleRate, streamDescription.numberOfChannels());
        status = m_sampleConverter->setOutputFormat(m_outputStreamDescription.streamDescription());
        ASSERT(!status);
    }

    // Let's skip pushing samples if we are too slow pulling them.
    if (m_skippingAudioData) {
        if (!isReachingBufferedAudioDataLowLimit())
            return;
        m_skippingAudioData = false;
    } else if (isReachingBufferedAudioDataHighLimit()) {
        m_skippingAudioData = true;
        return;
    }

    // If we change the audio track or its sample rate changes, the timestamp based on m_writeCount may be wrong.
    // FIXME: We should update m_writeCount to be valid according the new sampleRate.
    m_sampleConverter->pushSamples(MediaTime(m_writeCount, static_cast<uint32_t>(m_inputStreamDescription.sampleRate())), audioData, sampleCount);
    m_writeCount += sampleCount;

    LibWebRTCProvider::callOnWebRTCSignalingThread([protectedThis = makeRef(*this)] {
        protectedThis->pullAudioData();
    });
}

void RealtimeOutgoingAudioSource::pullAudioData()
{
    // libwebrtc expects 10 ms chunks.
    size_t chunkSampleCount = m_outputStreamDescription.sampleRate() / 100;
    size_t bufferSize = chunkSampleCount * LibWebRTCAudioFormat::sampleByteSize * m_outputStreamDescription.numberOfChannels();
    m_audioBuffer.grow(bufferSize);

    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0].mNumberChannels = m_outputStreamDescription.numberOfChannels();
    bufferList.mBuffers[0].mDataByteSize = bufferSize;
    bufferList.mBuffers[0].mData = m_audioBuffer.data();

    m_sampleConverter->pullAvalaibleSamplesAsChunks(bufferList, chunkSampleCount, m_readCount, [this, chunkSampleCount] {
        m_readCount += chunkSampleCount;
        for (auto sink : m_sinks)
            sink->OnData(m_audioBuffer.data(), LibWebRTCAudioFormat::sampleSize, m_outputStreamDescription.sampleRate(), m_outputStreamDescription.numberOfChannels(), chunkSampleCount);
    });
}

} // namespace WebCore

#endif // USE(LIBWEBRTC)
