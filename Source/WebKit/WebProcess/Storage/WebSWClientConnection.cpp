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
#include "WebSWClientConnection.h"

#if ENABLE(SERVICE_WORKER)

#include "DataReference.h"
#include "FormDataReference.h"
#include "Logging.h"
#include "ServiceWorkerClientFetch.h"
#include "StorageToWebProcessConnectionMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebSWOriginTable.h"
#include "WebSWServerConnectionMessages.h"
#include <WebCore/Document.h>
#include <WebCore/SerializedScriptValue.h>
#include <WebCore/ServiceWorkerClientData.h>
#include <WebCore/ServiceWorkerFetchResult.h>
#include <WebCore/ServiceWorkerJobData.h>
#include <WebCore/ServiceWorkerRegistrationData.h>

using namespace PAL;
using namespace WebCore;

namespace WebKit {

WebSWClientConnection::WebSWClientConnection(IPC::Connection& connection, SessionID sessionID)
    : m_sessionID(sessionID)
    , m_connection(connection)
    , m_swOriginTable(makeUniqueRef<WebSWOriginTable>())
{
    ASSERT(sessionID.isValid());
    bool result = sendSync(Messages::StorageToWebProcessConnection::EstablishSWServerConnection(sessionID), Messages::StorageToWebProcessConnection::EstablishSWServerConnection::Reply(m_identifier), Seconds::infinity(), IPC::SendSyncOption::DoNotProcessIncomingMessagesWhenWaitingForSyncReply);

    ASSERT_UNUSED(result, result);
}

WebSWClientConnection::~WebSWClientConnection()
{
}

void WebSWClientConnection::scheduleJobInServer(const ServiceWorkerJobData& jobData)
{
    send(Messages::WebSWServerConnection::ScheduleJobInServer(jobData));
}

void WebSWClientConnection::finishFetchingScriptInServer(const ServiceWorkerFetchResult& result)
{
    send(Messages::WebSWServerConnection::FinishFetchingScriptInServer(result));
}

void WebSWClientConnection::addServiceWorkerRegistrationInServer(ServiceWorkerRegistrationIdentifier identifier)
{
    send(Messages::WebSWServerConnection::AddServiceWorkerRegistrationInServer(identifier));
}

void WebSWClientConnection::removeServiceWorkerRegistrationInServer(ServiceWorkerRegistrationIdentifier identifier)
{
    send(Messages::WebSWServerConnection::RemoveServiceWorkerRegistrationInServer(identifier));
}

void WebSWClientConnection::postMessageToServiceWorker(ServiceWorkerIdentifier destinationIdentifier, MessageWithMessagePorts&& message, const ServiceWorkerOrClientIdentifier& sourceIdentifier)
{
    send(Messages::WebSWServerConnection::PostMessageToServiceWorker(destinationIdentifier, WTFMove(message), sourceIdentifier) );
}

void WebSWClientConnection::registerServiceWorkerClient(const SecurityOrigin& topOrigin, const WebCore::ServiceWorkerClientData& data, const std::optional<WebCore::ServiceWorkerIdentifier>& controllingServiceWorkerIdentifier)
{
    send(Messages::WebSWServerConnection::RegisterServiceWorkerClient { SecurityOriginData::fromSecurityOrigin(topOrigin), data, controllingServiceWorkerIdentifier });
}

void WebSWClientConnection::unregisterServiceWorkerClient(DocumentIdentifier contextIdentifier)
{
    send(Messages::WebSWServerConnection::UnregisterServiceWorkerClient { ServiceWorkerClientIdentifier { serverConnectionIdentifier(), contextIdentifier } });
}

void WebSWClientConnection::didResolveRegistrationPromise(const ServiceWorkerRegistrationKey& key)
{
    send(Messages::WebSWServerConnection::DidResolveRegistrationPromise(key));
}

bool WebSWClientConnection::mayHaveServiceWorkerRegisteredForOrigin(const SecurityOrigin& origin) const
{
    if (!m_swOriginTable->isImported())
        return true;

    return m_swOriginTable->contains(origin);
}

void WebSWClientConnection::setSWOriginTableSharedMemory(const SharedMemory::Handle& handle)
{
    m_swOriginTable->setSharedMemory(handle);
}

void WebSWClientConnection::setSWOriginTableIsImported()
{
    m_swOriginTable->setIsImported();
    while (!m_tasksPendingOriginImport.isEmpty())
        m_tasksPendingOriginImport.takeFirst()();
}

void WebSWClientConnection::didMatchRegistration(uint64_t matchingRequest, std::optional<ServiceWorkerRegistrationData>&& result)
{
    ASSERT(isMainThread());

    if (auto completionHandler = m_ongoingMatchRegistrationTasks.take(matchingRequest))
        completionHandler(WTFMove(result));
}

void WebSWClientConnection::didGetRegistrations(uint64_t matchingRequest, Vector<ServiceWorkerRegistrationData>&& registrations)
{
    ASSERT(isMainThread());

    if (auto completionHandler = m_ongoingGetRegistrationsTasks.take(matchingRequest))
        completionHandler(WTFMove(registrations));
}

void WebSWClientConnection::matchRegistration(const SecurityOrigin& topOrigin, const URL& clientURL, RegistrationCallback&& callback)
{
    ASSERT(isMainThread());

    if (!mayHaveServiceWorkerRegisteredForOrigin(topOrigin)) {
        callback(std::nullopt);
        return;
    }

    runOrDelayTaskForImport([this, callback = WTFMove(callback), topOrigin = SecurityOriginData::fromSecurityOrigin(topOrigin), clientURL]() mutable {
        uint64_t callbackID = ++m_previousCallbackIdentifier;
        m_ongoingMatchRegistrationTasks.add(callbackID, WTFMove(callback));
        send(Messages::WebSWServerConnection::MatchRegistration(callbackID, topOrigin, clientURL));
    });
}

void WebSWClientConnection::runOrDelayTaskForImport(WTF::Function<void()>&& task)
{
    if (m_swOriginTable->isImported())
        task();
    else
        m_tasksPendingOriginImport.append(WTFMove(task));
}

void WebSWClientConnection::whenRegistrationReady(const SecurityOrigin& topOrigin, const URL& clientURL, WhenRegistrationReadyCallback&& callback)
{
    uint64_t callbackID = ++m_previousCallbackIdentifier;
    m_ongoingRegistrationReadyTasks.add(callbackID, WTFMove(callback));
    send(Messages::WebSWServerConnection::WhenRegistrationReady(callbackID, SecurityOriginData::fromSecurityOrigin(topOrigin), clientURL));
}

void WebSWClientConnection::registrationReady(uint64_t callbackID, WebCore::ServiceWorkerRegistrationData&& registrationData)
{
    ASSERT(registrationData.activeWorker);
    if (auto callback = m_ongoingRegistrationReadyTasks.take(callbackID))
        callback(WTFMove(registrationData));
}

void WebSWClientConnection::getRegistrations(const SecurityOrigin& topOrigin, const URL& clientURL, GetRegistrationsCallback&& callback)
{
    ASSERT(isMainThread());

    if (!mayHaveServiceWorkerRegisteredForOrigin(topOrigin)) {
        callback({ });
        return;
    }

    runOrDelayTaskForImport([this, callback = WTFMove(callback), topOrigin = SecurityOriginData::fromSecurityOrigin(topOrigin), clientURL]() mutable {
        uint64_t callbackID = ++m_previousCallbackIdentifier;
        m_ongoingGetRegistrationsTasks.add(callbackID, WTFMove(callback));
        send(Messages::WebSWServerConnection::GetRegistrations(callbackID, topOrigin, clientURL));
    });
}

void WebSWClientConnection::startFetch(uint64_t fetchIdentifier, WebCore::ServiceWorkerRegistrationIdentifier serviceWorkerRegistrationIdentifier, const WebCore::ResourceRequest& request, const WebCore::FetchOptions& options, const String& referrer)
{
    send(Messages::WebSWServerConnection::StartFetch { fetchIdentifier, serviceWorkerRegistrationIdentifier, request, options, IPC::FormDataReference { request.httpBody() }, referrer });
}

void WebSWClientConnection::postMessageToServiceWorkerClient(DocumentIdentifier destinationContextIdentifier, MessageWithMessagePorts&& message, ServiceWorkerData&& source, const String& sourceOrigin)
{
    SWClientConnection::postMessageToServiceWorkerClient(destinationContextIdentifier, WTFMove(message), WTFMove(source), sourceOrigin);
}

void WebSWClientConnection::connectionToServerLost()
{
    auto registrationTasks = WTFMove(m_ongoingMatchRegistrationTasks);
    for (auto& callback : registrationTasks.values())
        callback(std::nullopt);

    auto getRegistrationTasks = WTFMove(m_ongoingGetRegistrationsTasks);
    for (auto& callback : getRegistrationTasks.values())
        callback({ });

    clearPendingJobs();
}

void WebSWClientConnection::syncTerminateWorker(ServiceWorkerIdentifier identifier)
{
    sendSync(Messages::WebSWServerConnection::SyncTerminateWorker(identifier), Messages::WebSWServerConnection::SyncTerminateWorker::Reply());
}

} // namespace WebKit

#endif // ENABLE(SERVICE_WORKER)
