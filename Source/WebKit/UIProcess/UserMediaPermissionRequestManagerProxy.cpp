/*
 * Copyright (C) 2014 Igalia S.L.
 * Copyright (C) 2016-2017 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "UserMediaPermissionRequestManagerProxy.h"

#include "APISecurityOrigin.h"
#include "APIUIClient.h"
#include "UserMediaProcessManager.h"
#include "WebAutomationSession.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebProcessPool.h"
#include "WebProcessProxy.h"
#include <WebCore/MediaConstraints.h>
#include <WebCore/MockRealtimeMediaSourceCenter.h>
#include <WebCore/RealtimeMediaSource.h>
#include <WebCore/SecurityOriginData.h>
#include <WebCore/UserMediaRequest.h>

using namespace WebCore;

namespace WebKit {

static const MediaProducer::MediaStateFlags activeCaptureMask = MediaProducer::HasActiveAudioCaptureDevice | MediaProducer::HasActiveVideoCaptureDevice;

UserMediaPermissionRequestManagerProxy::UserMediaPermissionRequestManagerProxy(WebPageProxy& page)
    : m_page(page)
    , m_rejectionTimer(RunLoop::main(), this, &UserMediaPermissionRequestManagerProxy::rejectionTimerFired)
    , m_watchdogTimer(RunLoop::main(), this, &UserMediaPermissionRequestManagerProxy::watchdogTimerFired)
{
#if ENABLE(MEDIA_STREAM)
    UserMediaProcessManager::singleton().addUserMediaPermissionRequestManagerProxy(*this);
#endif
}

UserMediaPermissionRequestManagerProxy::~UserMediaPermissionRequestManagerProxy()
{
#if ENABLE(MEDIA_STREAM)
    UserMediaProcessManager::singleton().removeUserMediaPermissionRequestManagerProxy(*this);
#endif
    invalidatePendingRequests();
}

void UserMediaPermissionRequestManagerProxy::invalidatePendingRequests()
{
    for (auto& request : m_pendingUserMediaRequests.values())
        request->invalidate();
    m_pendingUserMediaRequests.clear();

    for (auto& request : m_pendingDeviceRequests.values())
        request->invalidate();
    m_pendingDeviceRequests.clear();
}

void UserMediaPermissionRequestManagerProxy::stopCapture()
{
    invalidatePendingRequests();
    m_page.stopMediaCapture();
}

void UserMediaPermissionRequestManagerProxy::clearCachedState()
{
    invalidatePendingRequests();
}

Ref<UserMediaPermissionRequestProxy> UserMediaPermissionRequestManagerProxy::createPermissionRequest(uint64_t userMediaID, uint64_t mainFrameID, uint64_t frameID, Ref<SecurityOrigin>&& userMediaDocumentOrigin, Ref<SecurityOrigin>&& topLevelDocumentOrigin, Vector<CaptureDevice>&& audioDevices, Vector<CaptureDevice>&& videoDevices, String&& deviceIDHashSalt, MediaStreamRequest&& request)
{
    auto permissionRequest = UserMediaPermissionRequestProxy::create(*this, userMediaID, mainFrameID, frameID, WTFMove(userMediaDocumentOrigin), WTFMove(topLevelDocumentOrigin), WTFMove(audioDevices), WTFMove(videoDevices), WTFMove(deviceIDHashSalt), WTFMove(request));
    m_pendingUserMediaRequests.add(userMediaID, permissionRequest.ptr());
    return permissionRequest;
}

#if ENABLE(MEDIA_STREAM)
static uint64_t toWebCore(UserMediaPermissionRequestProxy::UserMediaAccessDenialReason reason)
{
    switch (reason) {
    case UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::NoConstraints:
        return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::NoConstraints);
    case UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::UserMediaDisabled:
        return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::UserMediaDisabled);
    case UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::NoCaptureDevices:
        return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::NoCaptureDevices);
    case UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::InvalidConstraint:
        return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::InvalidConstraint);
    case UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::HardwareError:
        return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::HardwareError);
    case UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied:
        return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::PermissionDenied);
    case UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::OtherFailure:
        return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::OtherFailure);
    }

    ASSERT_NOT_REACHED();
    return static_cast<uint64_t>(UserMediaRequest::MediaAccessDenialReason::OtherFailure);
}
#endif

void UserMediaPermissionRequestManagerProxy::userMediaAccessWasDenied(uint64_t userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason reason)
{
    if (!m_page.isValid())
        return;

    auto request = m_pendingUserMediaRequests.take(userMediaID);
    if (!request)
        return;

    if (reason == UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied)
        m_deniedRequests.append(DeniedRequest { request->mainFrameID(), request->userMediaDocumentSecurityOrigin(), request->topLevelDocumentSecurityOrigin(), request->requiresAudioCapture(), request->requiresVideoCapture() });

    denyRequest(userMediaID, reason, emptyString());
}

void UserMediaPermissionRequestManagerProxy::denyRequest(uint64_t userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason reason, const String& invalidConstraint)
{
    ASSERT(m_page.isValid());

#if ENABLE(MEDIA_STREAM)
    m_page.process().send(Messages::WebPage::UserMediaAccessWasDenied(userMediaID, toWebCore(reason), invalidConstraint), m_page.pageID());
#else
    UNUSED_PARAM(reason);
    UNUSED_PARAM(invalidConstraint);
#endif
}

void UserMediaPermissionRequestManagerProxy::userMediaAccessWasGranted(uint64_t userMediaID, CaptureDevice&& audioDevice, CaptureDevice&& videoDevice)
{
    ASSERT(audioDevice || videoDevice);

    if (!m_page.isValid())
        return;

#if ENABLE(MEDIA_STREAM)
    auto request = m_pendingUserMediaRequests.take(userMediaID);
    if (!request)
        return;

    grantAccess(userMediaID, WTFMove(audioDevice), WTFMove(videoDevice), request->deviceIdentifierHashSalt());
    m_grantedRequests.append(request.releaseNonNull());
#else
    UNUSED_PARAM(userMediaID);
    UNUSED_PARAM(audioDevice);
    UNUSED_PARAM(videoDevice);
#endif
}

#if ENABLE(MEDIA_STREAM)
void UserMediaPermissionRequestManagerProxy::resetAccess(uint64_t frameID)
{
    m_grantedRequests.removeAllMatching([frameID](const auto& grantedRequest) {
        return grantedRequest->mainFrameID() == frameID;
    });
    m_pregrantedRequests.clear();
    m_deniedRequests.clear();
}

const UserMediaPermissionRequestProxy* UserMediaPermissionRequestManagerProxy::searchForGrantedRequest(uint64_t frameID, const SecurityOrigin& userMediaDocumentOrigin, const SecurityOrigin& topLevelDocumentOrigin, bool needsAudio, bool needsVideo) const
{
    if (m_page.isMediaStreamCaptureMuted())
        return nullptr;

    bool checkForAudio = needsAudio;
    bool checkForVideo = needsVideo;
    for (const auto& grantedRequest : m_grantedRequests) {
        if (grantedRequest->requiresDisplayCapture())
            continue;
        if (!grantedRequest->userMediaDocumentSecurityOrigin().isSameSchemeHostPort(userMediaDocumentOrigin))
            continue;
        if (!grantedRequest->topLevelDocumentSecurityOrigin().isSameSchemeHostPort(topLevelDocumentOrigin))
            continue;
        if (grantedRequest->frameID() != frameID)
            continue;

        if (grantedRequest->requiresVideoCapture())
            checkForVideo = false;

        if (grantedRequest->requiresAudioCapture())
            checkForAudio = false;

        if (checkForVideo || checkForAudio)
            continue;

        return grantedRequest.ptr();
    }
    return nullptr;
}

bool UserMediaPermissionRequestManagerProxy::wasRequestDenied(uint64_t mainFrameID, const SecurityOrigin& userMediaDocumentOrigin, const SecurityOrigin& topLevelDocumentOrigin, bool needsAudio, bool needsVideo)
{
    for (const auto& deniedRequest : m_deniedRequests) {
        if (!deniedRequest.userMediaDocumentOrigin->isSameSchemeHostPort(userMediaDocumentOrigin))
            continue;
        if (!deniedRequest.topLevelDocumentOrigin->isSameSchemeHostPort(topLevelDocumentOrigin))
            continue;
        if (deniedRequest.mainFrameID != mainFrameID)
            continue;
        if (deniedRequest.isAudioDenied && needsAudio)
            return true;
        if (deniedRequest.isVideoDenied && needsVideo)
            return true;
    }
    return false;
}

void UserMediaPermissionRequestManagerProxy::grantAccess(uint64_t userMediaID, const CaptureDevice audioDevice, const CaptureDevice videoDevice, const String& deviceIdentifierHashSalt)
{
    UserMediaProcessManager::singleton().willCreateMediaStream(*this, !!audioDevice, !!videoDevice);
    m_page.process().send(Messages::WebPage::UserMediaAccessWasGranted(userMediaID, audioDevice, videoDevice, deviceIdentifierHashSalt), m_page.pageID());
}
#endif

void UserMediaPermissionRequestManagerProxy::rejectionTimerFired()
{
    uint64_t userMediaID = m_pendingRejections[0];
    m_pendingRejections.remove(0);

    denyRequest(userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied, emptyString());
    if (!m_pendingRejections.isEmpty())
        scheduleNextRejection();
}

void UserMediaPermissionRequestManagerProxy::scheduleNextRejection()
{
    const double mimimumDelayBeforeReplying = .25;
    if (!m_rejectionTimer.isActive())
        m_rejectionTimer.startOneShot(Seconds(mimimumDelayBeforeReplying + randomNumber()));
}

void UserMediaPermissionRequestManagerProxy::requestUserMediaPermissionForFrame(uint64_t userMediaID, uint64_t frameID, Ref<SecurityOrigin>&& userMediaDocumentOrigin, Ref<SecurityOrigin>&& topLevelDocumentOrigin, const MediaStreamRequest& userRequest)
{
#if ENABLE(MEDIA_STREAM)
    if (!UserMediaProcessManager::singleton().captureEnabled()) {
        m_pendingRejections.append(userMediaID);
        scheduleNextRejection();
        return;
    }

    RealtimeMediaSourceCenter::InvalidConstraintsHandler invalidHandler = [this, userMediaID](const String& invalidConstraint) {
        if (!m_page.isValid())
            return;

        denyRequest(userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::InvalidConstraint, invalidConstraint);
    };

    auto validHandler = [this, userMediaID, frameID, userMediaDocumentOrigin = userMediaDocumentOrigin.copyRef(), topLevelDocumentOrigin = topLevelDocumentOrigin.copyRef(), localUserRequest = userRequest](Vector<CaptureDevice>&& audioDevices, Vector<CaptureDevice>&& videoDevices, String&& deviceIdentifierHashSalt) mutable {
        if (!m_page.isValid() || !m_page.mainFrame())
            return;

        if (videoDevices.isEmpty() && audioDevices.isEmpty()) {
            denyRequest(userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::NoConstraints, emptyString());
            return;
        }

        if (wasRequestDenied(m_page.mainFrame()->frameID(), userMediaDocumentOrigin.get(), topLevelDocumentOrigin.get(), !audioDevices.isEmpty(), !videoDevices.isEmpty())) {
            denyRequest(userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied, emptyString());
            return;
        }

        auto* grantedRequest = searchForGrantedRequest(frameID, userMediaDocumentOrigin.get(), topLevelDocumentOrigin.get(), !audioDevices.isEmpty(), !videoDevices.isEmpty());
        if (grantedRequest) {
            if (m_page.isViewVisible()) {
            // We select the first available devices, but the current client API allows client to select which device to pick.
            // FIXME: Remove the possiblity for the client to do the device selection.
                auto audioDevice = !audioDevices.isEmpty() ? audioDevices[0] : CaptureDevice();
                auto videoDevice = !videoDevices.isEmpty() ? videoDevices[0] : CaptureDevice();
                grantAccess(userMediaID, WTFMove(audioDevice), WTFMove(videoDevice), grantedRequest->deviceIdentifierHashSalt());
            } else
                m_pregrantedRequests.append(createPermissionRequest(userMediaID, m_page.mainFrame()->frameID(), frameID, WTFMove(userMediaDocumentOrigin), WTFMove(topLevelDocumentOrigin), WTFMove(audioDevices), WTFMove(videoDevices), String(grantedRequest->deviceIdentifierHashSalt()), WTFMove(localUserRequest)));

            return;
        }
        auto userMediaOrigin = API::SecurityOrigin::create(userMediaDocumentOrigin.get());
        auto topLevelOrigin = API::SecurityOrigin::create(topLevelDocumentOrigin.get());

        auto pendingRequest = createPermissionRequest(userMediaID, m_page.mainFrame()->frameID(), frameID, WTFMove(userMediaDocumentOrigin), WTFMove(topLevelDocumentOrigin), WTFMove(audioDevices), WTFMove(videoDevices), WTFMove(deviceIdentifierHashSalt), WTFMove(localUserRequest));

        if (m_page.isControlledByAutomation()) {
            if (WebAutomationSession* automationSession = m_page.process().processPool().automationSession()) {
                if (automationSession->shouldAllowGetUserMediaForPage(m_page))
                    pendingRequest->allow();
                else
                    userMediaAccessWasDenied(userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::PermissionDenied);

                return;
            }
        }

        if (m_page.preferences().mockCaptureDevicesEnabled() && !m_page.preferences().mockCaptureDevicesPromptEnabled()) {
            pendingRequest->allow();
            return;
        }

        if (!m_page.uiClient().decidePolicyForUserMediaPermissionRequest(m_page, *m_page.process().webFrame(frameID), WTFMove(userMediaOrigin), WTFMove(topLevelOrigin), pendingRequest.get()))
            userMediaAccessWasDenied(userMediaID, UserMediaPermissionRequestProxy::UserMediaAccessDenialReason::UserMediaDisabled);
    };

    auto haveDeviceSaltHandler = [this, validHandler = WTFMove(validHandler), invalidHandler = WTFMove(invalidHandler), localUserRequest = userRequest](uint64_t userMediaID, String&& deviceIdentifierHashSalt, bool originHasPersistentAccess) mutable {

        auto pendingRequest = m_pendingDeviceRequests.take(userMediaID);
        if (!pendingRequest)
            return;

        if (!m_page.isValid())
            return;
        
        localUserRequest.audioConstraints.deviceIDHashSalt = deviceIdentifierHashSalt;
        localUserRequest.videoConstraints.deviceIDHashSalt = deviceIdentifierHashSalt;

        syncWithWebCorePrefs();
        
        RealtimeMediaSourceCenter::singleton().validateRequestConstraints(WTFMove(validHandler), WTFMove(invalidHandler), WTFMove(localUserRequest), WTFMove(deviceIdentifierHashSalt));
    };

    getUserMediaPermissionInfo(userMediaID, frameID, WTFMove(haveDeviceSaltHandler), WTFMove(userMediaDocumentOrigin), WTFMove(topLevelDocumentOrigin));
#else
    UNUSED_PARAM(userMediaID);
    UNUSED_PARAM(frameID);
    UNUSED_PARAM(userMediaDocumentOrigin);
    UNUSED_PARAM(topLevelDocumentOrigin);
    UNUSED_PARAM(userRequest);
#endif
}

#if ENABLE(MEDIA_STREAM)
void UserMediaPermissionRequestManagerProxy::getUserMediaPermissionInfo(uint64_t userMediaID, uint64_t frameID, UserMediaPermissionCheckProxy::CompletionHandler&& handler, Ref<SecurityOrigin>&& userMediaDocumentOrigin, Ref<SecurityOrigin>&& topLevelDocumentOrigin)
{
    auto userMediaOrigin = API::SecurityOrigin::create(userMediaDocumentOrigin.get());
    auto topLevelOrigin = API::SecurityOrigin::create(topLevelDocumentOrigin.get());

    auto request = UserMediaPermissionCheckProxy::create(userMediaID, frameID, WTFMove(handler), WTFMove(userMediaDocumentOrigin), WTFMove(topLevelDocumentOrigin));
    m_pendingDeviceRequests.add(userMediaID, request.copyRef());

    if (!m_page.uiClient().checkUserMediaPermissionForOrigin(m_page, *m_page.process().webFrame(frameID), userMediaOrigin.get(), topLevelOrigin.get(), request.get()))
        request->completionHandler()(userMediaID, String(), false);
} 
#endif

void UserMediaPermissionRequestManagerProxy::enumerateMediaDevicesForFrame(uint64_t userMediaID, uint64_t frameID, Ref<SecurityOrigin>&& userMediaDocumentOrigin, Ref<SecurityOrigin>&& topLevelDocumentOrigin)
{
#if ENABLE(MEDIA_STREAM)
    auto completionHandler = [this](uint64_t userMediaID, String&& deviceIdentifierHashSalt, bool originHasPersistentAccess) {
        auto request = m_pendingDeviceRequests.take(userMediaID);
        if (!request)
            return;

        if (!m_page.isValid())
            return;

        syncWithWebCorePrefs();

        auto devices = RealtimeMediaSourceCenter::singleton().getMediaStreamDevices();
        devices.removeAllMatching([&](auto& device) -> bool {
            return !device.enabled() || (device.type() != WebCore::CaptureDevice::DeviceType::Camera && device.type() != WebCore::CaptureDevice::DeviceType::Microphone);
        });

        m_page.process().send(Messages::WebPage::DidCompleteMediaDeviceEnumeration(userMediaID, devices, deviceIdentifierHashSalt, originHasPersistentAccess), m_page.pageID());
    };

    getUserMediaPermissionInfo(userMediaID, frameID, WTFMove(completionHandler), WTFMove(userMediaDocumentOrigin), WTFMove(topLevelDocumentOrigin));
#else
    UNUSED_PARAM(userMediaID);
    UNUSED_PARAM(frameID);
    UNUSED_PARAM(userMediaDocumentOrigin);
    UNUSED_PARAM(topLevelDocumentOrigin);
#endif
}

void UserMediaPermissionRequestManagerProxy::syncWithWebCorePrefs() const
{
#if ENABLE(MEDIA_STREAM)
    // Enable/disable the mock capture devices for the UI process as per the WebCore preferences. Note that
    // this is a noop if the preference hasn't changed since the last time this was called.
    bool mockDevicesEnabled = m_page.preferences().mockCaptureDevicesEnabled();
    MockRealtimeMediaSourceCenter::setMockRealtimeMediaSourceCenterEnabled(mockDevicesEnabled);
#endif
}

void UserMediaPermissionRequestManagerProxy::captureStateChanged(MediaProducer::MediaStateFlags oldState, MediaProducer::MediaStateFlags newState)
{
    if (!m_page.isValid())
        return;

#if ENABLE(MEDIA_STREAM)
    bool wasCapturingAudio = oldState & MediaProducer::AudioCaptureMask;
    bool wasCapturingVideo = oldState & MediaProducer::VideoCaptureMask;
    bool isCapturingAudio = newState & MediaProducer::AudioCaptureMask;
    bool isCapturingVideo = newState & MediaProducer::VideoCaptureMask;

    if ((wasCapturingAudio && !isCapturingAudio) || (wasCapturingVideo && !isCapturingVideo))
        UserMediaProcessManager::singleton().endedCaptureSession(*this);
    if ((!wasCapturingAudio && isCapturingAudio) || (!wasCapturingVideo && isCapturingVideo))
        UserMediaProcessManager::singleton().startedCaptureSession(*this);

    if (m_captureState == (newState & activeCaptureMask))
        return;

    m_captureState = newState & activeCaptureMask;

    Seconds interval;
    if (m_captureState & activeCaptureMask)
        interval = Seconds::fromHours(m_page.preferences().longRunningMediaCaptureStreamRepromptIntervalInHours());
    else
        interval = Seconds::fromMinutes(m_page.preferences().inactiveMediaCaptureSteamRepromptIntervalInMinutes());

    if (interval == m_currentWatchdogInterval)
        return;

    m_currentWatchdogInterval = interval;
    m_watchdogTimer.startOneShot(m_currentWatchdogInterval);
#endif
}

void UserMediaPermissionRequestManagerProxy::processPregrantedRequests()
{
    for (auto& request : m_pregrantedRequests)
        request->allow();
    m_pregrantedRequests.clear();
}

void UserMediaPermissionRequestManagerProxy::watchdogTimerFired()
{
    m_grantedRequests.clear();
    m_pregrantedRequests.clear();
    m_currentWatchdogInterval = 0_s;
}

} // namespace WebKit
