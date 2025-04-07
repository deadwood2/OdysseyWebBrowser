/*
 * Copyright (C) 2017-2019 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CoreAudioCaptureSource.h"

#if ENABLE(MEDIA_STREAM)

#include "AVAudioSessionCaptureDevice.h"
#include "AVAudioSessionCaptureDeviceManager.h"
#include "AudioSampleBufferList.h"
#include "AudioSampleDataSource.h"
#include "AudioSession.h"
#include "CoreAudioCaptureDevice.h"
#include "CoreAudioCaptureDeviceManager.h"
#include "CoreAudioCaptureSourceIOS.h"
#include "Logging.h"
#include "Timer.h"
#include "WebAudioSourceProviderAVFObjC.h"
#include <AudioToolbox/AudioConverter.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreMedia/CMSync.h>
#include <mach/mach_time.h>
#include <pal/avfoundation/MediaTimeAVFoundation.h>
#include <pal/spi/cf/CoreAudioSPI.h>
#include <sys/time.h>
#include <wtf/Algorithms.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <pal/cf/CoreMediaSoftLink.h>

namespace WebCore {
using namespace PAL;

#if PLATFORM(MAC)
CoreAudioCaptureSourceFactory& CoreAudioCaptureSourceFactory::singleton()
{
    static NeverDestroyed<CoreAudioCaptureSourceFactory> factory;
    return factory.get();
}
#endif

const UInt32 outputBus = 0;
const UInt32 inputBus = 1;

class CoreAudioSharedUnit {
public:
    static CoreAudioSharedUnit& singleton();
    CoreAudioSharedUnit();

    void addClient(CoreAudioCaptureSource&);
    void removeClient(CoreAudioCaptureSource&);

    void startProducingData();
    void stopProducingData();
    bool isProducingData() { return m_ioUnitStarted; }

    OSStatus suspend();
    OSStatus resume();

    bool isSuspended() const { return m_suspended; }

    OSStatus setupAudioUnit();
    void cleanupAudioUnit();
    OSStatus reconfigureAudioUnit();

    void addEchoCancellationSource(AudioSampleDataSource&);
    void removeEchoCancellationSource(AudioSampleDataSource&);

    static size_t preferredIOBufferSize();

    const CAAudioStreamDescription& microphoneFormat() const { return m_microphoneProcFormat; }

    double volume() const { return m_volume; }
    int sampleRate() const { return m_sampleRate; }
    bool enableEchoCancellation() const { return m_enableEchoCancellation; }

    void setVolume(double volume) { m_volume = volume; }
    void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }
    void setEnableEchoCancellation(bool enableEchoCancellation) { m_enableEchoCancellation = enableEchoCancellation; }

    bool hasAudioUnit() const { return m_ioUnit; }

    void setCaptureDevice(String&&, uint32_t);

    void devicesChanged(const Vector<CaptureDevice>&);

private:
    OSStatus configureSpeakerProc();
    OSStatus configureMicrophoneProc();
    OSStatus defaultOutputDevice(uint32_t*);
    OSStatus defaultInputDevice(uint32_t*);

    static OSStatus microphoneCallback(void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32, AudioBufferList*);
    OSStatus processMicrophoneSamples(AudioUnitRenderActionFlags&, const AudioTimeStamp&, UInt32, UInt32, AudioBufferList*);

    static OSStatus speakerCallback(void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32, AudioBufferList*);
    OSStatus provideSpeakerData(AudioUnitRenderActionFlags&, const AudioTimeStamp&, UInt32, UInt32, AudioBufferList*);

    OSStatus startInternal();
    void stopInternal();

    void unduck();

    void verifyIsCapturing();
    void devicesChanged();
    void captureFailed();

    void forEachClient(const Function<void(CoreAudioCaptureSource&)>& apply) const;

    HashSet<CoreAudioCaptureSource*> m_clients;
    mutable RecursiveLock m_clientsLock;

    AudioUnit m_ioUnit { nullptr };

    // Only read/modified from the IO thread.
    Vector<Ref<AudioSampleDataSource>> m_activeSources;

    enum QueueAction { Add, Remove };
    Vector<std::pair<QueueAction, Ref<AudioSampleDataSource>>> m_pendingSources;

#if PLATFORM(MAC)
    uint32_t m_captureDeviceID { 0 };
#endif

    CAAudioStreamDescription m_microphoneProcFormat;
    RefPtr<AudioSampleBufferList> m_microphoneSampleBuffer;
    uint64_t m_latestMicTimeStamp { 0 };

    CAAudioStreamDescription m_speakerProcFormat;
    RefPtr<AudioSampleBufferList> m_speakerSampleBuffer;

    double m_DTSConversionRatio { 0 };

    bool m_ioUnitInitialized { false };
    bool m_ioUnitStarted { false };

    Lock m_pendingSourceQueueLock;
    Lock m_internalStateLock;

    int32_t m_producingCount { 0 };

    mutable std::unique_ptr<RealtimeMediaSourceCapabilities> m_capabilities;
    mutable Optional<RealtimeMediaSourceSettings> m_currentSettings;

#if !LOG_DISABLED
    void checkTimestamps(const AudioTimeStamp&, uint64_t, double);

    String m_ioUnitName;
    uint64_t m_speakerProcsCalled { 0 };
#endif

    String m_persistentID;

    uint64_t m_microphoneProcsCalled { 0 };
    uint64_t m_microphoneProcsCalledLastTime { 0 };
    Timer m_verifyCapturingTimer;

    bool m_enableEchoCancellation { true };
    double m_volume { 1 };
    int m_sampleRate;

    bool m_suspended { false };
};

CoreAudioSharedUnit& CoreAudioSharedUnit::singleton()
{
    static NeverDestroyed<CoreAudioSharedUnit> singleton;
    return singleton;
}

CoreAudioSharedUnit::CoreAudioSharedUnit()
    : m_verifyCapturingTimer(*this, &CoreAudioSharedUnit::verifyIsCapturing)
{
    m_sampleRate = AudioSession::sharedSession().sampleRate();
}

void CoreAudioSharedUnit::addClient(CoreAudioCaptureSource& client)
{
    auto locker = holdLock(m_clientsLock);
    m_clients.add(&client);
}

void CoreAudioSharedUnit::removeClient(CoreAudioCaptureSource& client)
{
    auto locker = holdLock(m_clientsLock);
    m_clients.remove(&client);
}

void CoreAudioSharedUnit::forEachClient(const Function<void(CoreAudioCaptureSource&)>& apply) const
{
    Vector<CoreAudioCaptureSource*> clientsCopy;
    {
        auto locker = holdLock(m_clientsLock);
        clientsCopy = copyToVector(m_clients);
    }
    for (auto* client : clientsCopy) {
        auto locker = holdLock(m_clientsLock);
        // Make sure the client has not been destroyed.
        if (!m_clients.contains(client))
            continue;
        apply(*client);
    }
}

void CoreAudioSharedUnit::setCaptureDevice(String&& persistentID, uint32_t captureDeviceID)
{
    m_persistentID = WTFMove(persistentID);

#if PLATFORM(MAC)
    if (m_captureDeviceID == captureDeviceID)
        return;

    m_captureDeviceID = captureDeviceID;
    reconfigureAudioUnit();
#else
    UNUSED_PARAM(captureDeviceID);
#endif
}

void CoreAudioSharedUnit::devicesChanged(const Vector<CaptureDevice>& devices)
{
    if (!m_ioUnit)
        return;

    if (WTF::anyOf(devices, [this] (auto& device) { return m_persistentID == device.persistentId(); }))
        return;

    captureFailed();
}

void CoreAudioSharedUnit::addEchoCancellationSource(AudioSampleDataSource& source)
{
    if (!source.setOutputFormat(m_speakerProcFormat)) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::addEchoCancellationSource: source %p configureOutput failed", &source);
        return;
    }

    std::lock_guard<Lock> lock(m_pendingSourceQueueLock);
    m_pendingSources.append({ QueueAction::Add, source });
}

void CoreAudioSharedUnit::removeEchoCancellationSource(AudioSampleDataSource& source)
{
    std::lock_guard<Lock> lock(m_pendingSourceQueueLock);
    m_pendingSources.append({ QueueAction::Remove, source });
}

size_t CoreAudioSharedUnit::preferredIOBufferSize()
{
    return AudioSession::sharedSession().bufferSize();
}

OSStatus CoreAudioSharedUnit::setupAudioUnit()
{
    if (m_ioUnit)
        return 0;

    ASSERT(!m_clients.isEmpty());

    mach_timebase_info_data_t timebaseInfo;
    mach_timebase_info(&timebaseInfo);
    m_DTSConversionRatio = 1e-9 * static_cast<double>(timebaseInfo.numer) / static_cast<double>(timebaseInfo.denom);

    AudioComponentDescription ioUnitDescription = { kAudioUnitType_Output, kAudioUnitSubType_VoiceProcessingIO, kAudioUnitManufacturer_Apple, 0, 0 };
    AudioComponent ioComponent = AudioComponentFindNext(nullptr, &ioUnitDescription);
    ASSERT(ioComponent);
    if (!ioComponent) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) unable to find vpio unit component", this);
        return -1;
    }

#if !LOG_DISABLED
    CFStringRef name = nullptr;
    AudioComponentCopyName(ioComponent, &name);
    if (name) {
        m_ioUnitName = name;
        CFRelease(name);
        RELEASE_LOG(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) created \"%s\" component", this, m_ioUnitName.utf8().data());
    }
#endif

    auto err = AudioComponentInstanceNew(ioComponent, &m_ioUnit);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) unable to open vpio unit, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    if (!m_enableEchoCancellation) {
        uint32_t param = 0;
        err = AudioUnitSetProperty(m_ioUnit, kAUVoiceIOProperty_VoiceProcessingEnableAGC, kAudioUnitScope_Global, inputBus, &param, sizeof(param));
        if (err) {
            RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) unable to set vpio automatic gain control, error %d (%.4s)", this, (int)err, (char*)&err);
            return err;
        }
        param = 1;
        err = AudioUnitSetProperty(m_ioUnit, kAUVoiceIOProperty_BypassVoiceProcessing, kAudioUnitScope_Global, inputBus, &param, sizeof(param));
        if (err) {
            RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) unable to set vpio unit echo cancellation, error %d (%.4s)", this, (int)err, (char*)&err);
            return err;
        }
    }

#if PLATFORM(IOS_FAMILY)
    uint32_t param = 1;
    err = AudioUnitSetProperty(m_ioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, inputBus, &param, sizeof(param));
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) unable to enable vpio unit input, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }
#else
    if (!m_captureDeviceID) {
        err = defaultInputDevice(&m_captureDeviceID);
        if (err)
            return err;
    }

    err = AudioUnitSetProperty(m_ioUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, inputBus, &m_captureDeviceID, sizeof(m_captureDeviceID));
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) unable to set vpio unit capture device ID %d, error %d (%.4s)", this, (int)m_captureDeviceID, (int)err, (char*)&err);
        return err;
    }
#endif

    err = configureMicrophoneProc();
    if (err)
        return err;

    err = configureSpeakerProc();
    if (err)
        return err;

    err = AudioUnitInitialize(m_ioUnit);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::setupAudioUnit(%p) AudioUnitInitialize() failed, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }
    m_ioUnitInitialized = true;
    m_suspended = false;

    unduck();

    return err;
}

void CoreAudioSharedUnit::unduck()
{
    uint32_t outputDevice;
    if (!defaultOutputDevice(&outputDevice))
        AudioDeviceDuck(outputDevice, 1.0, nullptr, 0);
}

OSStatus CoreAudioSharedUnit::configureMicrophoneProc()
{
    AURenderCallbackStruct callback = { microphoneCallback, this };
    auto err = AudioUnitSetProperty(m_ioUnit, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, inputBus, &callback, sizeof(callback));
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::configureMicrophoneProc(%p) unable to set vpio unit mic proc, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    AudioStreamBasicDescription microphoneProcFormat = { };

    UInt32 size = sizeof(microphoneProcFormat);
    err = AudioUnitGetProperty(m_ioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, inputBus, &microphoneProcFormat, &size);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::configureMicrophoneProc(%p) unable to get output stream format, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    microphoneProcFormat.mSampleRate = m_sampleRate;
    err = AudioUnitSetProperty(m_ioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, inputBus, &microphoneProcFormat, size);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::configureMicrophoneProc(%p) unable to set output stream format, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    m_microphoneSampleBuffer = AudioSampleBufferList::create(microphoneProcFormat, preferredIOBufferSize() * 2);
    m_microphoneProcFormat = microphoneProcFormat;

    return err;
}

OSStatus CoreAudioSharedUnit::configureSpeakerProc()
{
    AURenderCallbackStruct callback = { speakerCallback, this };
    auto err = AudioUnitSetProperty(m_ioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, outputBus, &callback, sizeof(callback));
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::configureSpeakerProc(%p) unable to set vpio unit speaker proc, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    AudioStreamBasicDescription speakerProcFormat = { };

    UInt32 size = sizeof(speakerProcFormat);
    err = AudioUnitGetProperty(m_ioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, outputBus, &speakerProcFormat, &size);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::configureSpeakerProc(%p) unable to get input stream format, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    speakerProcFormat.mSampleRate = m_sampleRate;
    err = AudioUnitSetProperty(m_ioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, outputBus, &speakerProcFormat, size);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::configureSpeakerProc(%p) unable to get input stream format, error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    m_speakerSampleBuffer = AudioSampleBufferList::create(speakerProcFormat, preferredIOBufferSize() * 2);
    m_speakerProcFormat = speakerProcFormat;

    return err;
}

#if !LOG_DISABLED
void CoreAudioSharedUnit::checkTimestamps(const AudioTimeStamp& timeStamp, uint64_t sampleTime, double hostTime)
{
    if (!timeStamp.mSampleTime || sampleTime == m_latestMicTimeStamp || !hostTime)
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::checkTimestamps: unusual timestamps, sample time = %lld, previous sample time = %lld, hostTime %f", sampleTime, m_latestMicTimeStamp, hostTime);
}
#endif

OSStatus CoreAudioSharedUnit::provideSpeakerData(AudioUnitRenderActionFlags& /*ioActionFlags*/, const AudioTimeStamp& timeStamp, UInt32 /*inBusNumber*/, UInt32 inNumberFrames, AudioBufferList* ioData)
{
    // Called when the audio unit needs data to play through the speakers.
#if !LOG_DISABLED
    ++m_speakerProcsCalled;
#endif

    if (m_speakerSampleBuffer->sampleCapacity() < inNumberFrames) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::provideSpeakerData: speaker sample buffer size (%d) too small for amount of sample data requested (%d)!", m_speakerSampleBuffer->sampleCapacity(), (int)inNumberFrames);
        return kAudio_ParamError;
    }

    // Add/remove sources from the queue, but only if we get the lock immediately. Otherwise try
    // again on the next callback.
    {
        std::unique_lock<Lock> lock(m_pendingSourceQueueLock, std::try_to_lock);
        if (lock.owns_lock()) {
            for (auto& pair : m_pendingSources) {
                if (pair.first == QueueAction::Add)
                    m_activeSources.append(pair.second.copyRef());
                else {
                    auto removeSource = pair.second.copyRef();
                    m_activeSources.removeFirstMatching([&removeSource](auto& source) {
                        return source.ptr() == removeSource.ptr();
                    });
                }
            }
            m_pendingSources.clear();
        }
    }

    if (m_activeSources.isEmpty())
        return 0;

    double adjustedHostTime = m_DTSConversionRatio * timeStamp.mHostTime;
    uint64_t sampleTime = timeStamp.mSampleTime;
#if !LOG_DISABLED
    checkTimestamps(timeStamp, sampleTime, adjustedHostTime);
#endif
    m_speakerSampleBuffer->setTimes(adjustedHostTime, sampleTime);

    AudioBufferList& bufferList = m_speakerSampleBuffer->bufferList();
    for (uint32_t i = 0; i < bufferList.mNumberBuffers; ++i)
        bufferList.mBuffers[i] = ioData->mBuffers[i];

    bool firstSource = true;
    for (auto& source : m_activeSources) {
        source->pullSamples(*m_speakerSampleBuffer.get(), inNumberFrames, adjustedHostTime, sampleTime, firstSource ? AudioSampleDataSource::Copy : AudioSampleDataSource::Mix);
        firstSource = false;
    }

    return noErr;
}

OSStatus CoreAudioSharedUnit::speakerCallback(void *inRefCon, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData)
{
    ASSERT(ioActionFlags);
    ASSERT(inTimeStamp);
    auto dataSource = static_cast<CoreAudioSharedUnit*>(inRefCon);
    return dataSource->provideSpeakerData(*ioActionFlags, *inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

OSStatus CoreAudioSharedUnit::processMicrophoneSamples(AudioUnitRenderActionFlags& ioActionFlags, const AudioTimeStamp& timeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* /*ioData*/)
{
    ++m_microphoneProcsCalled;

    // Pull through the vpio unit to our mic buffer.
    m_microphoneSampleBuffer->reset();
    AudioBufferList& bufferList = m_microphoneSampleBuffer->bufferList();
    auto err = AudioUnitRender(m_ioUnit, &ioActionFlags, &timeStamp, inBusNumber, inNumberFrames, &bufferList);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::processMicrophoneSamples(%p) AudioUnitRender failed with error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    double adjustedHostTime = m_DTSConversionRatio * timeStamp.mHostTime;
    uint64_t sampleTime = timeStamp.mSampleTime;
#if !LOG_DISABLED
    checkTimestamps(timeStamp, sampleTime, adjustedHostTime);
#endif
    m_latestMicTimeStamp = sampleTime;
    m_microphoneSampleBuffer->setTimes(adjustedHostTime, sampleTime);

    if (m_volume != 1.0)
        m_microphoneSampleBuffer->applyGain(m_volume);

    forEachClient([&](auto& client) {
        if (client.isProducingData())
            client.audioSamplesAvailable(MediaTime(sampleTime, m_microphoneProcFormat.sampleRate()), m_microphoneSampleBuffer->bufferList(), m_microphoneProcFormat, inNumberFrames);
    });
    return noErr;
}

OSStatus CoreAudioSharedUnit::microphoneCallback(void *inRefCon, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData)
{
    ASSERT(ioActionFlags);
    ASSERT(inTimeStamp);
    CoreAudioSharedUnit* dataSource = static_cast<CoreAudioSharedUnit*>(inRefCon);
    return dataSource->processMicrophoneSamples(*ioActionFlags, *inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

void CoreAudioSharedUnit::cleanupAudioUnit()
{
    if (m_ioUnitInitialized) {
        ASSERT(m_ioUnit);
        auto err = AudioUnitUninitialize(m_ioUnit);
        if (err)
            RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::cleanupAudioUnit(%p) AudioUnitUninitialize failed with error %d (%.4s)", this, (int)err, (char*)&err);
        m_ioUnitInitialized = false;
    }

    if (m_ioUnit) {
        AudioComponentInstanceDispose(m_ioUnit);
        m_ioUnit = nullptr;
    }

    m_microphoneSampleBuffer = nullptr;
    m_speakerSampleBuffer = nullptr;
#if !LOG_DISABLED
    m_ioUnitName = emptyString();
#endif
}

OSStatus CoreAudioSharedUnit::reconfigureAudioUnit()
{
    OSStatus err;
    if (!hasAudioUnit())
        return 0;

    if (m_ioUnitStarted) {
        err = AudioOutputUnitStop(m_ioUnit);
        if (err) {
            RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::reconfigureAudioUnit(%p) AudioOutputUnitStop failed with error %d (%.4s)", this, (int)err, (char*)&err);
            return err;
        }
    }

    cleanupAudioUnit();
    err = setupAudioUnit();
    if (err)
        return err;

    if (m_ioUnitStarted) {
        err = AudioOutputUnitStart(m_ioUnit);
        if (err) {
            RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::reconfigureAudioUnit(%p) AudioOutputUnitStart failed with error %d (%.4s)", this, (int)err, (char*)&err);
            return err;
        }
    }
    return err;
}

void CoreAudioSharedUnit::startProducingData()
{
    ASSERT(isMainThread());

    if (++m_producingCount != 1)
        return;

    if (m_ioUnitStarted)
        return;

    if (m_ioUnit) {
        cleanupAudioUnit();
        ASSERT(!m_ioUnit);
    }

    if (startInternal())
        captureFailed();
}

OSStatus CoreAudioSharedUnit::resume()
{
    ASSERT(isMainThread());
    ASSERT(m_suspended);
    ASSERT(!m_ioUnitStarted);

    m_suspended = false;

    if (!m_ioUnit)
        return 0;

    startInternal();

    return 0;
}

OSStatus CoreAudioSharedUnit::startInternal()
{
    OSStatus err;
    if (!m_ioUnit) {
        err = setupAudioUnit();
        if (err) {
            cleanupAudioUnit();
            ASSERT(!m_ioUnit);
            return err;
        }
        ASSERT(m_ioUnit);
    }

    unduck();

    err = AudioOutputUnitStart(m_ioUnit);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::start(%p) AudioOutputUnitStart failed with error %d (%.4s)", this, (int)err, (char*)&err);
        return err;
    }

    m_ioUnitStarted = true;

    m_verifyCapturingTimer.startRepeating(10_s);
    m_microphoneProcsCalled = 0;
    m_microphoneProcsCalledLastTime = 0;

    return 0;
}

void CoreAudioSharedUnit::verifyIsCapturing()
{
    if (m_microphoneProcsCalledLastTime != m_microphoneProcsCalled) {
        m_microphoneProcsCalledLastTime = m_microphoneProcsCalled;
        if (m_verifyCapturingTimer.repeatInterval() == 10_s)
            m_verifyCapturingTimer.startRepeating(2_s);
        return;
    }

    RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::verifyIsCapturing - no audio received in %d seconds, failing", static_cast<int>(m_verifyCapturingTimer.repeatInterval().value()));
    captureFailed();
}

void CoreAudioSharedUnit::captureFailed()
{
    RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::captureFailed - capture failed");
    forEachClient([](auto& client) {
        client.captureFailed();
    });

    m_producingCount = 0;

    {
        auto locker = holdLock(m_clientsLock);
        m_clients.clear();
    }

    stopInternal();
    cleanupAudioUnit();
}

void CoreAudioSharedUnit::stopProducingData()
{
    ASSERT(isMainThread());
    ASSERT(m_producingCount);

    if (m_producingCount && --m_producingCount)
        return;

    stopInternal();
    cleanupAudioUnit();
}

OSStatus CoreAudioSharedUnit::suspend()
{
    ASSERT(isMainThread());

    m_suspended = true;
    stopInternal();

    return 0;
}

void CoreAudioSharedUnit::stopInternal()
{
    m_verifyCapturingTimer.stop();

    if (!m_ioUnit || !m_ioUnitStarted)
        return;

    auto err = AudioOutputUnitStop(m_ioUnit);
    if (err) {
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::stop(%p) AudioOutputUnitStop failed with error %d (%.4s)", this, (int)err, (char*)&err);
        return;
    }

    m_ioUnitStarted = false;
}

OSStatus CoreAudioSharedUnit::defaultInputDevice(uint32_t* deviceID)
{
    ASSERT(m_ioUnit);

    UInt32 propertySize = sizeof(*deviceID);
    auto err = AudioUnitGetProperty(m_ioUnit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, inputBus, deviceID, &propertySize);
    if (err)
        RELEASE_LOG_ERROR(WebRTC, "CoreAudioSharedUnit::defaultInputDevice(%p) unable to get default input device ID, error %d (%.4s)", this, (int)err, (char*)&err);

    return err;
}

OSStatus CoreAudioSharedUnit::defaultOutputDevice(uint32_t* deviceID)
{
    OSErr err = -1;
#if PLATFORM(MAC)
    AudioObjectPropertyAddress address = { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };

    if (AudioObjectHasProperty(kAudioObjectSystemObject, &address)) {
        UInt32 propertySize = sizeof(AudioDeviceID);
        err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &propertySize, deviceID);
    }
#else
    UNUSED_PARAM(deviceID);
#endif
    return err;
}

CaptureSourceOrError CoreAudioCaptureSource::create(String&& deviceID, String&& hashSalt, const MediaConstraints* constraints)
{
#if PLATFORM(MAC)
    auto device = CoreAudioCaptureDeviceManager::singleton().coreAudioDeviceWithUID(deviceID);
    if (!device)
        return { };

    auto source = adoptRef(*new CoreAudioCaptureSource(WTFMove(deviceID), String { device->label() }, WTFMove(hashSalt), device->deviceID()));
#elif PLATFORM(IOS_FAMILY)
    auto device = AVAudioSessionCaptureDeviceManager::singleton().audioSessionDeviceWithUID(WTFMove(deviceID));
    if (!device)
        return { };

    auto source = adoptRef(*new CoreAudioCaptureSource(WTFMove(deviceID), String { device->label() }, WTFMove(hashSalt), 0));
#endif

    if (constraints) {
        if (auto result = source->applyConstraints(*constraints))
            return WTFMove(result->badConstraint);
    }
    return CaptureSourceOrError(WTFMove(source));
}

void CoreAudioCaptureSourceFactory::beginInterruption()
{
    if (!isMainThread()) {
        callOnMainThread([this] {
            beginInterruption();
        });
        return;
    }
    ASSERT(isMainThread());

    if (auto* source = coreAudioActiveSource()) {
        source->beginInterruption();
        return;
    }
    CoreAudioSharedUnit::singleton().suspend();
}

void CoreAudioCaptureSourceFactory::endInterruption()
{
    if (!isMainThread()) {
        callOnMainThread([this] {
            endInterruption();
        });
        return;
    }
    ASSERT(isMainThread());

    if (auto* source = coreAudioActiveSource()) {
        source->endInterruption();
        return;
    }
    CoreAudioSharedUnit::singleton().reconfigureAudioUnit();
}

void CoreAudioCaptureSourceFactory::scheduleReconfiguration()
{
    if (!isMainThread()) {
        callOnMainThread([this] {
            scheduleReconfiguration();
        });
        return;
    }
    ASSERT(isMainThread());

    if (auto* source = coreAudioActiveSource()) {
        source->scheduleReconfiguration();
        return;
    }
    CoreAudioSharedUnit::singleton().reconfigureAudioUnit();
}

AudioCaptureFactory& CoreAudioCaptureSource::factory()
{
    return CoreAudioCaptureSourceFactory::singleton();
}

CaptureDeviceManager& CoreAudioCaptureSourceFactory::audioCaptureDeviceManager()
{
#if PLATFORM(MAC)
    return CoreAudioCaptureDeviceManager::singleton();
#else
    return AVAudioSessionCaptureDeviceManager::singleton();
#endif
}

void CoreAudioCaptureSourceFactory::devicesChanged(const Vector<CaptureDevice>& devices)
{
    CoreAudioSharedUnit::singleton().devicesChanged(devices);
}

CoreAudioCaptureSource::CoreAudioCaptureSource(String&& deviceID, String&& label, String&& hashSalt, uint32_t captureDeviceID)
    : RealtimeMediaSource(RealtimeMediaSource::Type::Audio, WTFMove(label), WTFMove(deviceID), WTFMove(hashSalt))
    , m_captureDeviceID(captureDeviceID)
{
}

void CoreAudioCaptureSource::initializeToStartProducingData()
{
    if (m_isReadyToStart)
        return;

    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER);
    m_isReadyToStart = true;

    auto& unit = CoreAudioSharedUnit::singleton();
    unit.setCaptureDevice(String { persistentID() }, m_captureDeviceID);

    initializeEchoCancellation(unit.enableEchoCancellation());
    initializeSampleRate(unit.sampleRate());
    initializeVolume(unit.volume());

    unit.addClient(*this);

#if PLATFORM(IOS_FAMILY)
    // We ensure that we unsuspend ourselves on the constructor as a capture source
    // is created when getUserMedia grants access which only happens when the process is foregrounded.
    if (unit.isSuspended())
        unit.reconfigureAudioUnit();
#endif
}

CoreAudioCaptureSource::~CoreAudioCaptureSource()
{
#if PLATFORM(IOS_FAMILY)
    CoreAudioCaptureSourceFactory::singleton().unsetCoreAudioActiveSource(*this);
#endif

    CoreAudioSharedUnit::singleton().removeClient(*this);
}

void CoreAudioCaptureSource::addEchoCancellationSource(AudioSampleDataSource& source)
{
    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER);
    CoreAudioSharedUnit::singleton().addEchoCancellationSource(source);
}

void CoreAudioCaptureSource::removeEchoCancellationSource(AudioSampleDataSource& source)
{
    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER);
    CoreAudioSharedUnit::singleton().removeEchoCancellationSource(source);
}

void CoreAudioCaptureSource::startProducingData()
{
#if PLATFORM(IOS_FAMILY)
    CoreAudioCaptureSourceFactory::singleton().setCoreAudioActiveSource(*this);
#endif

    auto& unit = CoreAudioSharedUnit::singleton();

    auto isSuspended = unit.isSuspended();
    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER, isSuspended);

    if (isSuspended) {
        m_suspendType = SuspensionType::WhilePlaying;
        return;
    }

    initializeToStartProducingData();
    unit.startProducingData();
}

void CoreAudioCaptureSource::stopProducingData()
{
    auto& unit = CoreAudioSharedUnit::singleton();

    auto isSuspended = unit.isSuspended();
    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER, isSuspended);

    if (isSuspended) {
        m_suspendType = SuspensionType::WhilePaused;
        return;
    }

    unit.stopProducingData();
}

const RealtimeMediaSourceCapabilities& CoreAudioCaptureSource::capabilities()
{
    if (!m_capabilities) {
        RealtimeMediaSourceCapabilities capabilities(settings().supportedConstraints());
        capabilities.setDeviceId(hashedId());
        capabilities.setEchoCancellation(RealtimeMediaSourceCapabilities::EchoCancellation::ReadWrite);
        capabilities.setVolume(CapabilityValueOrRange(0.0, 1.0));
        capabilities.setSampleRate(CapabilityValueOrRange(8000, 96000));
        m_capabilities = WTFMove(capabilities);
    }
    return m_capabilities.value();
}

const RealtimeMediaSourceSettings& CoreAudioCaptureSource::settings()
{
    if (!m_currentSettings) {
        RealtimeMediaSourceSettings settings;
        settings.setVolume(volume());
        settings.setSampleRate(sampleRate());
        settings.setDeviceId(hashedId());
        settings.setLabel(name());
        settings.setEchoCancellation(echoCancellation());

        RealtimeMediaSourceSupportedConstraints supportedConstraints;
        supportedConstraints.setSupportsDeviceId(true);
        supportedConstraints.setSupportsEchoCancellation(true);
        supportedConstraints.setSupportsVolume(true);
        supportedConstraints.setSupportsSampleRate(true);
        settings.setSupportedConstraints(supportedConstraints);

        m_currentSettings = WTFMove(settings);
    }
    return m_currentSettings.value();
}

void CoreAudioCaptureSource::settingsDidChange(OptionSet<RealtimeMediaSourceSettings::Flag> settings)
{
    if (settings.contains(RealtimeMediaSourceSettings::Flag::EchoCancellation)) {
        CoreAudioSharedUnit::singleton().setEnableEchoCancellation(echoCancellation());
        scheduleReconfiguration();
    }
    if (settings.contains(RealtimeMediaSourceSettings::Flag::SampleRate)) {
        CoreAudioSharedUnit::singleton().setSampleRate(sampleRate());
        scheduleReconfiguration();
    }

    m_currentSettings = WTF::nullopt;
}

void CoreAudioCaptureSource::scheduleReconfiguration()
{
    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER);

    ASSERT(isMainThread());
    auto& unit = CoreAudioSharedUnit::singleton();
    if (!unit.hasAudioUnit() || m_reconfigurationState != ReconfigurationState::None)
        return;

    m_reconfigurationState = ReconfigurationState::Ongoing;
    scheduleDeferredTask([this, &unit] {
        if (unit.isSuspended()) {
            m_reconfigurationState = ReconfigurationState::Required;
            return;
        }

        unit.reconfigureAudioUnit();
        m_reconfigurationState = ReconfigurationState::None;
    });
}

void CoreAudioCaptureSource::beginInterruption()
{
    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER);

    ASSERT(isMainThread());
    auto& unit = CoreAudioSharedUnit::singleton();
    if (!unit.hasAudioUnit() || unit.isSuspended() || m_suspendPending)
        return;

    m_suspendPending = true;
    scheduleDeferredTask([this, &unit] {
        m_suspendType = unit.isProducingData() ? SuspensionType::WhilePlaying : SuspensionType::WhilePaused;
        unit.suspend();
        notifyMutedChange(true);
        m_suspendPending = false;
    });
}

void CoreAudioCaptureSource::endInterruption()
{
    ALWAYS_LOG_IF(loggerPtr(), LOGIDENTIFIER);

    ASSERT(isMainThread());
    auto& unit = CoreAudioSharedUnit::singleton();
    if (!unit.hasAudioUnit() || !unit.isSuspended() || m_resumePending)
        return;

    auto type = m_suspendType;
    m_suspendType = SuspensionType::None;
    if (type != SuspensionType::WhilePlaying && m_reconfigurationState != ReconfigurationState::Required)
        return;

    m_resumePending = true;
    scheduleDeferredTask([this, type, &unit] {
        if (m_reconfigurationState == ReconfigurationState::Required)
            unit.reconfigureAudioUnit();
        if (type == SuspensionType::WhilePlaying) {
            unit.resume();
            notifyMutedChange(false);
        }
        m_reconfigurationState = ReconfigurationState::None;
        m_resumePending = false;
    });
}

bool CoreAudioCaptureSource::interrupted() const
{
    auto& unit = CoreAudioSharedUnit::singleton();
    if (unit.isSuspended())
        return true;

    return RealtimeMediaSource::interrupted();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
