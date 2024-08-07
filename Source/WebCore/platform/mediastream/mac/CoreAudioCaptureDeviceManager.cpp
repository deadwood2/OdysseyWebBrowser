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
#include "CoreAudioCaptureDeviceManager.h"

#if ENABLE(MEDIA_STREAM) && PLATFORM(MAC)

#include "CoreAudioCaptureDevice.h"
#include "Logging.h"
#include <AudioUnit/AudioUnit.h>
#include <CoreMedia/CMSync.h>
#include <wtf/Assertions.h>
#include <wtf/NeverDestroyed.h>

#import <pal/cf/CoreMediaSoftLink.h>

namespace WebCore {

CoreAudioCaptureDeviceManager& CoreAudioCaptureDeviceManager::singleton()
{
    static NeverDestroyed<CoreAudioCaptureDeviceManager> manager;
    return manager;
}

const Vector<CaptureDevice>& CoreAudioCaptureDeviceManager::captureDevices()
{
    coreAudioCaptureDevices();
    return m_devices;
}

std::optional<CaptureDevice> CoreAudioCaptureDeviceManager::captureDeviceWithPersistentID(CaptureDevice::DeviceType type, const String& deviceID)
{
    ASSERT_UNUSED(type, type == CaptureDevice::DeviceType::Microphone);
    for (auto& device : captureDevices()) {
        if (device.persistentId() == deviceID)
            return device;
    }
    return std::nullopt;
}

static bool deviceHasInputStreams(AudioObjectID deviceID)
{
    UInt32 dataSize = 0;
    AudioObjectPropertyAddress address = { kAudioDevicePropertyStreams, kAudioDevicePropertyScopeInput, kAudioObjectPropertyElementMaster };
    auto err = AudioObjectGetPropertyDataSize(deviceID, &address, 0, nullptr, &dataSize);

    return !err && dataSize;
}

static bool isValidCaptureDevice(const CoreAudioCaptureDevice& device)
{
    // Ignore unnamed devices and aggregate devices created by VPIO.
    return !device.label().isEmpty() && !device.label().startsWith("VPAUAggregateAudioDevice");
}

Vector<CoreAudioCaptureDevice>& CoreAudioCaptureDeviceManager::coreAudioCaptureDevices()
{
    static bool initialized;
    if (!initialized) {
        initialized = true;
        refreshAudioCaptureDevices();

        AudioObjectPropertyAddress address = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
        auto err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address, devicesChanged, this);
        if (err)
            LOG_ERROR("CoreAudioCaptureDeviceManager::devices(%p) AudioObjectAddPropertyListener returned error %d (%.4s)", this, (int)err, (char*)&err);
    }

    return m_coreAudioCaptureDevices;
}

std::optional<CoreAudioCaptureDevice> CoreAudioCaptureDeviceManager::coreAudioDeviceWithUID(const String& deviceID)
{
    for (auto& device : coreAudioCaptureDevices()) {
        if (device.persistentId() == deviceID)
            return device;
    }
    return std::nullopt;
}


void CoreAudioCaptureDeviceManager::refreshAudioCaptureDevices()
{
    AudioObjectPropertyAddress address = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };
    UInt32 dataSize = 0;
    auto err = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &dataSize);
    if (err) {
        LOG(Media, "CoreAudioCaptureDeviceManager::refreshAudioCaptureDevices(%p) failed to get size of device list %d (%.4s)", this, (int)err, (char*)&err);
        return;
    }

    size_t deviceCount = dataSize / sizeof(AudioObjectID);
    AudioObjectID deviceIDs[deviceCount];
    err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &dataSize, deviceIDs);
    if (err) {
        LOG(Media, "CoreAudioCaptureDeviceManager::refreshAudioCaptureDevices(%p) failed to get device list %d (%.4s)", this, (int)err, (char*)&err);
        return;
    }

    bool haveDeviceChanges = false;
    for (size_t i = 0; i < deviceCount; i++) {
        AudioObjectID deviceID = deviceIDs[i];
        if (!deviceHasInputStreams(deviceID))
            continue;

        if (std::any_of(m_coreAudioCaptureDevices.begin(), m_coreAudioCaptureDevices.end(), [deviceID](auto& device) { return device.deviceID() == deviceID; }))
            continue;

        auto device = CoreAudioCaptureDevice::create(deviceID);
        if (!device || !isValidCaptureDevice(device.value()))
            continue;

        m_coreAudioCaptureDevices.append(WTFMove(device.value()));
        haveDeviceChanges = true;
    }

    for (auto& device : m_coreAudioCaptureDevices) {
        bool isAlive = device.isAlive();
        if (device.enabled() != isAlive) {
            device.setEnabled(isAlive);
            haveDeviceChanges = true;
        }
    }

    if (!haveDeviceChanges)
        return;

    m_devices = Vector<CaptureDevice>();

    for (auto &device : m_coreAudioCaptureDevices) {
        CaptureDevice captureDevice(device.persistentId(), CaptureDevice::DeviceType::Microphone, device.label());
        captureDevice.setEnabled(device.enabled());
        m_devices.append(captureDevice);
    }

    for (auto& observer : m_observers.values())
        observer();
}

OSStatus CoreAudioCaptureDeviceManager::devicesChanged(AudioObjectID, UInt32, const AudioObjectPropertyAddress*, void* userData)
{
    static_cast<CoreAudioCaptureDeviceManager*>(userData)->refreshAudioCaptureDevices();
    return 0;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM) && PLATFORM(MAC)
