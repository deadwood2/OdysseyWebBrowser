/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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
#include "Download.h"

#include "AuthenticationManager.h"
#include "Connection.h"
#include "DataReference.h"
#include "DownloadManager.h"
#include "DownloadProxyMessages.h"
#include "SandboxExtension.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/NotImplemented.h>

using namespace WebCore;

#define RELEASE_LOG_IF_ALLOWED(...) RELEASE_LOG_IF(isAlwaysOnLoggingAllowed(), __VA_ARGS__)
#define RELEASE_LOG_ERROR_IF_ALLOWED(...) RELEASE_LOG_ERROR_IF(isAlwaysOnLoggingAllowed(), __VA_ARGS__)

namespace WebKit {

#if USE(NETWORK_SESSION) && PLATFORM(COCOA)
Download::Download(DownloadManager& downloadManager, DownloadID downloadID, NSURLSessionDownloadTask* download, const WebCore::SessionID& sessionID, const String& suggestedName)
#else
Download::Download(DownloadManager& downloadManager, DownloadID downloadID, const ResourceRequest& request, const String& suggestedName)
#endif
    : m_downloadManager(downloadManager)
    , m_downloadID(downloadID)
#if !USE(NETWORK_SESSION)
    , m_request(request)
#endif
#if USE(NETWORK_SESSION) && PLATFORM(COCOA)
    , m_download(download)
    , m_sessionID(sessionID)
#endif
    , m_suggestedName(suggestedName)
{
    ASSERT(m_downloadID.downloadID());

    m_downloadManager.didCreateDownload();
}

Download::~Download()
{
    platformInvalidate();

    m_downloadManager.didDestroyDownload();
}

#if !USE(NETWORK_SESSION)
void Download::didStart()
{
    send(Messages::DownloadProxy::DidStart(m_request, m_suggestedName));
}
#endif

#if !USE(NETWORK_SESSION)
void Download::didReceiveAuthenticationChallenge(const AuthenticationChallenge& authenticationChallenge)
{
    m_downloadManager.downloadsAuthenticationManager().didReceiveAuthenticationChallenge(*this, authenticationChallenge);
}

void Download::didReceiveResponse(const ResourceResponse& response)
{
    RELEASE_LOG_IF_ALLOWED("Download task (%llu) created", downloadID().downloadID());

    send(Messages::DownloadProxy::DidReceiveResponse(response));
}
#endif

void Download::didReceiveData(uint64_t length)
{
    if (!m_hasReceivedData) {
        RELEASE_LOG_IF_ALLOWED("Download task (%llu) started receiving data", downloadID().downloadID());
        m_hasReceivedData = true;
    }

    send(Messages::DownloadProxy::DidReceiveData(length));
}

bool Download::shouldDecodeSourceDataOfMIMEType(const String& mimeType)
{
    bool result;
    if (!sendSync(Messages::DownloadProxy::ShouldDecodeSourceDataOfMIMEType(mimeType), Messages::DownloadProxy::ShouldDecodeSourceDataOfMIMEType::Reply(result)))
        return true;

    return result;
}

#if !USE(NETWORK_SESSION)
String Download::decideDestinationWithSuggestedFilename(const String& filename, bool& allowOverwrite)
{
    String destination;
    SandboxExtension::Handle sandboxExtensionHandle;
    if (!sendSync(Messages::DownloadProxy::DecideDestinationWithSuggestedFilename(filename), Messages::DownloadProxy::DecideDestinationWithSuggestedFilename::Reply(destination, allowOverwrite, sandboxExtensionHandle)))
        return String();

    m_sandboxExtension = SandboxExtension::create(sandboxExtensionHandle);
    if (m_sandboxExtension)
        m_sandboxExtension->consume();

    return destination;
}
#endif

void Download::didCreateDestination(const String& path)
{
    send(Messages::DownloadProxy::DidCreateDestination(path));
}

void Download::didFinish()
{
    RELEASE_LOG_IF_ALLOWED("Download task (%llu) finished", downloadID().downloadID());

    platformDidFinish();

    send(Messages::DownloadProxy::DidFinish());

    if (m_sandboxExtension) {
        m_sandboxExtension->revoke();
        m_sandboxExtension = nullptr;
    }

    m_downloadManager.downloadFinished(this);
}

void Download::didFail(const ResourceError& error, const IPC::DataReference& resumeData)
{
    RELEASE_LOG_IF_ALLOWED("Download task (%llu) failed, isTimeout = %d, isCancellation = %d, errCode = %d",
        downloadID().downloadID(), error.isTimeout(), error.isCancellation(), error.errorCode());

    send(Messages::DownloadProxy::DidFail(error, resumeData));

    if (m_sandboxExtension) {
        m_sandboxExtension->revoke();
        m_sandboxExtension = nullptr;
    }
    m_downloadManager.downloadFinished(this);
}

void Download::didCancel(const IPC::DataReference& resumeData)
{
    RELEASE_LOG_IF_ALLOWED("Download task (%llu) canceled", downloadID().downloadID());

    send(Messages::DownloadProxy::DidCancel(resumeData));

    if (m_sandboxExtension) {
        m_sandboxExtension->revoke();
        m_sandboxExtension = nullptr;
    }
    m_downloadManager.downloadFinished(this);
}

IPC::Connection* Download::messageSenderConnection()
{
    return m_downloadManager.downloadProxyConnection();
}

uint64_t Download::messageSenderDestinationID()
{
    return m_downloadID.downloadID();
}

bool Download::isAlwaysOnLoggingAllowed() const
{
#if USE(NETWORK_SESSION) && PLATFORM(COCOA)
    return m_sessionID.isAlwaysOnLoggingAllowed();
#else
    return false;
#endif
}

} // namespace WebKit
