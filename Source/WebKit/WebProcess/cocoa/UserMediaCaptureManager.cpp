/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "config.h"
#include "UserMediaCaptureManager.h"

#if PLATFORM(COCOA) && ENABLE(MEDIA_STREAM)

#include "SharedRingBufferStorage.h"
#include "UserMediaCaptureManagerMessages.h"
#include "UserMediaCaptureManagerProxyMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcess.h"
#include "WebProcessCreationParameters.h"
#include <WebCore/CaptureDevice.h>
#include <WebCore/MediaConstraints.h>
#include <WebCore/RealtimeMediaSourceCenter.h>
#include <WebCore/WebAudioBufferList.h>
#include <WebCore/WebAudioSourceProviderAVFObjC.h>

using namespace WebCore;

namespace WebKit {

static uint64_t nextSessionID()
{
    static uint64_t nextID = 0;
    return ++nextID;
}

class UserMediaCaptureManager::Source : public RealtimeMediaSource {
public:
    Source(const String& sourceID, Type type, const String& name, uint64_t id, UserMediaCaptureManager& manager)
        : RealtimeMediaSource(sourceID, type, name)
        , m_id(id)
        , m_manager(manager)
        , m_ringBuffer(makeUniqueRef<SharedRingBufferStorage>(nullptr))
    {
    }

    ~Source()
    {
        storage().invalidate();
    }

    SharedRingBufferStorage& storage() { return static_cast<SharedRingBufferStorage&>(m_ringBuffer.storage()); }
    const RealtimeMediaSourceCapabilities& capabilities() const final {
        if (!m_capabilities)
            m_capabilities = m_manager.capabilities(m_id);
        return m_capabilities.value();
    }

    const RealtimeMediaSourceSettings& settings() const final { return m_settings; }
    void setSettings(RealtimeMediaSourceSettings&& settings)
    {
        m_settings = WTFMove(settings);
        settingsDidChange();
    }

    const CAAudioStreamDescription& description() const { return m_description; }
    void setStorage(const SharedMemory::Handle& handle, const WebCore::CAAudioStreamDescription& description, uint64_t numberOfFrames)
    {
        m_description = description;

        if (handle.isNull()) {
            m_ringBuffer.deallocate();
            storage().setReadOnly(false);
            storage().setStorage(nullptr);
            return;
        }

        RefPtr<SharedMemory> memory = SharedMemory::map(handle, SharedMemory::Protection::ReadOnly);
        storage().setStorage(WTFMove(memory));
        storage().setReadOnly(true);

        m_ringBuffer.allocate(description, numberOfFrames);
    }

    void setRingBufferFrameBounds(uint64_t startFrame, uint64_t endFrame)
    {
        m_ringBuffer.setCurrentFrameBounds(startFrame, endFrame);
    }

    void audioSamplesAvailable(MediaTime time, uint64_t numberOfFrames)
    {
        WebAudioBufferList audioData(m_description, numberOfFrames);
        m_ringBuffer.fetch(audioData.list(), numberOfFrames, time.timeValue());

        RealtimeMediaSource::audioSamplesAvailable(time, audioData, m_description, numberOfFrames);
    }

    void applyConstraintsSucceeded(const WebCore::RealtimeMediaSourceSettings& settings)
    {
        auto callbacks = m_pendingApplyConstraintsCallbacks.takeFirst();
        setSettings(WebCore::RealtimeMediaSourceSettings(settings));
        callbacks.successHandler();
    }

    void applyConstraintsFailed(const String& failedConstraint, const String& errorMessage)
    {
        auto callbacks = m_pendingApplyConstraintsCallbacks.takeFirst();
        callbacks.failureHandler(failedConstraint, errorMessage);
    }

private:
    void startProducingData() final { m_manager.startProducingData(m_id); }
    void stopProducingData() final { m_manager.stopProducingData(m_id); }
    bool isCaptureSource() const final { return true; }

    // RealtimeMediaSource
    void beginConfiguration() final { }
    void commitConfiguration() final { }

    void applyConstraints(const WebCore::MediaConstraints& constraints, SuccessHandler&& successHandler, FailureHandler&& failureHandler) final {
        m_manager.applyConstraints(m_id, constraints);
        m_pendingApplyConstraintsCallbacks.append({ WTFMove(successHandler), WTFMove(failureHandler)});
    }

    uint64_t m_id;
    UserMediaCaptureManager& m_manager;
    mutable std::optional<RealtimeMediaSourceCapabilities> m_capabilities;
    RealtimeMediaSourceSettings m_settings;
    CAAudioStreamDescription m_description;
    CARingBuffer m_ringBuffer;

    struct ApplyConstraintsCallback {
        SuccessHandler successHandler;
        FailureHandler failureHandler;
    };
    Deque<ApplyConstraintsCallback> m_pendingApplyConstraintsCallbacks;
};

UserMediaCaptureManager::UserMediaCaptureManager(WebProcess& process)
    : m_process(process)
{
    m_process.addMessageReceiver(Messages::UserMediaCaptureManager::messageReceiverName(), *this);
}

UserMediaCaptureManager::~UserMediaCaptureManager()
{
    RealtimeMediaSourceCenter::singleton().unsetAudioFactory(*this);
    m_process.removeMessageReceiver(Messages::UserMediaCaptureManager::messageReceiverName());
}

const char* UserMediaCaptureManager::supplementName()
{
    return "UserMediaCaptureManager";
}

void UserMediaCaptureManager::initialize(const WebProcessCreationParameters& parameters)
{
    if (parameters.shouldCaptureAudioInUIProcess)
        RealtimeMediaSourceCenter::singleton().setAudioFactory(*this);
}

WebCore::CaptureSourceOrError UserMediaCaptureManager::createCaptureSource(const CaptureDevice& device, WebCore::RealtimeMediaSource::Type sourceType, const WebCore::MediaConstraints* constraints)
{
    if (!constraints)
        return { };

    uint64_t id = nextSessionID();
    RealtimeMediaSourceSettings settings;
    String errorMessage;
    bool succeeded;
    if (!m_process.sendSync(Messages::UserMediaCaptureManagerProxy::CreateMediaSourceForCaptureDeviceWithConstraints(id, device, sourceType, *constraints), Messages::UserMediaCaptureManagerProxy::CreateMediaSourceForCaptureDeviceWithConstraints::Reply(succeeded, errorMessage, settings), 0))
        return WTFMove(errorMessage);

    auto source = adoptRef(*new Source(String::number(id), sourceType, settings.label(), id, *this));
    source->setSettings(WTFMove(settings));
    m_sources.set(id, source.copyRef());
    return WebCore::CaptureSourceOrError(WTFMove(source));
}

void UserMediaCaptureManager::sourceStopped(uint64_t id)
{
    ASSERT(m_sources.contains(id));
    m_sources.get(id)->stop();
}

void UserMediaCaptureManager::captureFailed(uint64_t id)
{
    ASSERT(m_sources.contains(id));
    m_sources.get(id)->captureFailed();
}

void UserMediaCaptureManager::sourceMutedChanged(uint64_t id, bool muted)
{
    ASSERT(m_sources.contains(id));
    m_sources.get(id)->setMuted(muted);
}

void UserMediaCaptureManager::sourceSettingsChanged(uint64_t id, const RealtimeMediaSourceSettings& settings)
{
    ASSERT(m_sources.contains(id));
    m_sources.get(id)->setSettings(RealtimeMediaSourceSettings(settings));
}

void UserMediaCaptureManager::storageChanged(uint64_t id, const SharedMemory::Handle& handle, const WebCore::CAAudioStreamDescription& description, uint64_t numberOfFrames)
{
    ASSERT(m_sources.contains(id));
    m_sources.get(id)->setStorage(handle, description, numberOfFrames);
}

void UserMediaCaptureManager::ringBufferFrameBoundsChanged(uint64_t id, uint64_t startFrame, uint64_t endFrame)
{
    ASSERT(m_sources.contains(id));
    m_sources.get(id)->setRingBufferFrameBounds(startFrame, endFrame);
}

void UserMediaCaptureManager::audioSamplesAvailable(uint64_t id, MediaTime time, uint64_t numberOfFrames, uint64_t startFrame, uint64_t endFrame)
{
    ASSERT(m_sources.contains(id));
    auto& source = *m_sources.get(id);
    source.setRingBufferFrameBounds(startFrame, endFrame);
    source.audioSamplesAvailable(time, numberOfFrames);
}

void UserMediaCaptureManager::startProducingData(uint64_t id)
{
    m_process.send(Messages::UserMediaCaptureManagerProxy::StartProducingData(id), 0);
}

void UserMediaCaptureManager::stopProducingData(uint64_t id)
{
    m_process.send(Messages::UserMediaCaptureManagerProxy::StopProducingData(id), 0);
}

WebCore::RealtimeMediaSourceCapabilities UserMediaCaptureManager::capabilities(uint64_t id)
{
    WebCore::RealtimeMediaSourceCapabilities capabilities;
    m_process.sendSync(Messages::UserMediaCaptureManagerProxy::Capabilities(id), Messages::UserMediaCaptureManagerProxy::Capabilities::Reply(capabilities), 0);
    return capabilities;
}

void UserMediaCaptureManager::setMuted(uint64_t id, bool muted)
{
    m_process.send(Messages::UserMediaCaptureManagerProxy::SetMuted(id, muted), 0);
}

void UserMediaCaptureManager::applyConstraints(uint64_t id, const WebCore::MediaConstraints& constraints)
{
    m_process.send(Messages::UserMediaCaptureManagerProxy::ApplyConstraints(id, constraints), 0);
}

void UserMediaCaptureManager::applyConstraintsSucceeded(uint64_t id, const WebCore::RealtimeMediaSourceSettings& settings)
{
    ASSERT(m_sources.contains(id));
    auto& source = *m_sources.get(id);
    source.applyConstraintsSucceeded(settings);
}

void UserMediaCaptureManager::applyConstraintsFailed(uint64_t id, const String& failedConstraint, const String& message)
{
    ASSERT(m_sources.contains(id));
    auto& source = *m_sources.get(id);
    source.applyConstraintsFailed(failedConstraint, message);
}

}

#endif
