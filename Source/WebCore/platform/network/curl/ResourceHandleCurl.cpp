/*
 * Copyright (C) 2004, 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2005, 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 * All rights reserved.
 * Copyright (C) 2017 NAVER Corp.
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
#include "ResourceHandle.h"

#if USE(CURL)

#include "CookieManager.h"
#include "CredentialStorage.h"
#include "CurlContext.h"
#include "FileSystem.h"
#include "Logging.h"
#include "ResourceHandleInternal.h"
#include "SynchronousLoaderClient.h"
#include <wtf/CompletionHandler.h>

#if PLATFORM(MUI)
#include "gui.h"
#include <clib/debug_protos.h>
#define D(x)
#undef String
#undef set
#undef get
#endif

namespace WebCore {

ResourceHandleInternal::~ResourceHandleInternal()
{
    if (m_delegate)
        m_delegate->releaseHandle();
}

ResourceHandle::~ResourceHandle() = default;

bool ResourceHandle::start()
{
    ASSERT(isMainThread());

    CurlContext::singleton();

    // The frame could be null if the ResourceHandle is not associated to any
    // Frame, e.g. if we are downloading a file.
    // If the frame is not null but the page is null this must be an attempted
    // load from an unload handler, so let's just block it.
    // If both the frame and the page are not null the context is valid.
    if (d->m_context && !d->m_context->isValid())
        return false;

    // Only allow the POST and GET methods for non-HTTP requests.
    const ResourceRequest& request = firstRequest();
    if (!request.url().protocolIsInHTTPFamily() && request.httpMethod() != "GET" && request.httpMethod() != "POST") {
        scheduleFailure(InvalidURLFailure); // Error must not be reported immediately
        return true;
    }

#if PLATFORM(MUI)
    if ((!d->m_user.isEmpty() || !d->m_pass.isEmpty()) && !shouldUseCredentialStorage()) {
        // Credentials for ftp can only be passed in URL, the didReceiveAuthenticationChallenge delegate call won't be made.
        URL urlWithCredentials(d->m_firstRequest.url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        d->m_firstRequest.setURL(urlWithCredentials);
    }
#endif

    d->m_delegate = adoptRef(new ResourceHandleCurlDelegate(this));
    d->m_delegate->start();
    return true;
}

void ResourceHandle::cancel()
{
    if (d->m_delegate)
        d->m_delegate->cancel();
}

#if OS(WINDOWS)

void ResourceHandle::setHostAllowsAnyHTTPSCertificate(const String& host)
{
    ASSERT(isMainThread());

    CurlContext::singleton().sslHandle().setHostAllowsAnyHTTPSCertificate(host);
}

void ResourceHandle::setClientCertificateInfo(const String& host, const String& certificate, const String& key)
{
    ASSERT(isMainThread());

    if (FileSystem::fileExists(certificate))
        CurlContext::singleton().sslHandle().setClientCertificateInfo(host, certificate, key);
    else
        LOG(Network, "Invalid client certificate file: %s!\n", certificate.latin1().data());
}

#endif

#if OS(WINDOWS) && USE(CF)

void ResourceHandle::setClientCertificate(const String&, CFDataRef)
{
}

#endif

void ResourceHandle::platformSetDefersLoading(bool defers)
{
    ASSERT(isMainThread());

    if (d->m_delegate)
        d->m_delegate->setDefersLoading(defers);
}

#if PLATFORM(MUI)
void ResourceHandle::setCookies()
{
    URL url = getInternal()->m_firstRequest.url();
    if ((cookieManager().cookiePolicy() == CookieStorageAcceptPolicyOnlyFromMainDocumentDomain)
      && (getInternal()->m_firstRequest.firstPartyForCookies() != url)
      && cookieManager().getCookie(url, WithHttpOnlyCookies).isEmpty())
        return;
    cookieManager().setCookies(url, getInternal()->m_response.httpHeaderField(String("Set-Cookie")));
    checkAndSendCookies(url);
}

void ResourceHandle::checkAndSendCookies(URL& url)
{
#if 0
    // Cookies are a part of the http protocol only
    if (!String(d->m_curlHandle.url()).startsWith("http"))
	return;

    if (url.isEmpty())
        url = URL(ParsedURLString, d->m_curlHandle.url());

    // Prepare a cookie header if there are cookies related to this url.
    String cookiePairs = cookieManager().getCookie(url, WithHttpOnlyCookies);
    if (!cookiePairs.isEmpty() && d->m_handle) {
        CString cookieChar = cookiePairs.ascii();
        LOG(Network, "CURL POST Cookie : %s \n", cookieChar.data());
        curl_easy_setopt(d->m_handle, CURLOPT_COOKIE, cookieChar.data());
    }
#endif
}

void ResourceHandle::setStartOffset(unsigned long long offset)
{
    ResourceHandleInternal* d = getInternal();
    d->m_startOffset = offset;
}

unsigned long long ResourceHandle::startOffset()
{
    ResourceHandleInternal* d = getInternal();
    return d->m_startOffset;
}

void ResourceHandle::setCanResume(bool value)
{
    ResourceHandleInternal* d = getInternal();
    d->m_canResume = value;
}

bool ResourceHandle::canResume()
{
    ResourceHandleInternal* d = getInternal();
    return d->m_canResume;
}

bool ResourceHandle::isResuming()
{
    ResourceHandleInternal* d = getInternal();
    return d->m_startOffset != 0;
}
#endif

bool ResourceHandle::shouldUseCredentialStorage()
{
    return (!client() || client()->shouldUseCredentialStorage(this)) && firstRequest().url().protocolIsInHTTPFamily();
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    ASSERT(isMainThread());

    String partition = firstRequest().cachePartition();

    if (!d->m_user.isNull() && !d->m_pass.isNull()) {
        Credential credential(d->m_user, d->m_pass, CredentialPersistenceNone);

        URL urlToStore;
        if (challenge.failureResponse().httpStatusCode() == 401)
            urlToStore = challenge.failureResponse().url();
        CredentialStorage::defaultCredentialStorage().set(partition, credential, challenge.protectionSpace(), urlToStore);
        
        if (d->m_delegate)
            d->m_delegate->setAuthentication(credential.user(), credential.password());

        d->m_user = String();
        d->m_pass = String();
        // FIXME: Per the specification, the user shouldn't be asked for credentials if there were incorrect ones provided explicitly.
        return;
    }

    if (shouldUseCredentialStorage()) {
        if (/*!d->m_initialCredential.isEmpty() ||*/ challenge.previousFailureCount()) { // MORPHOS: the original check is weird 
            // The stored credential wasn't accepted, stop using it.
            // There is a race condition here, since a different credential might have already been stored by another ResourceHandle,
            // but the observable effect should be very minor, if any.
            CredentialStorage::defaultCredentialStorage().remove(partition, challenge.protectionSpace());
        }

        if (!challenge.previousFailureCount()) {
            Credential credential = CredentialStorage::defaultCredentialStorage().get(partition, challenge.protectionSpace());
            if (!credential.isEmpty() && credential != d->m_initialCredential) {
                ASSERT(credential.persistence() == CredentialPersistenceNone);
                if (challenge.failureResponse().httpStatusCode() == 401) {
                    // Store the credential back, possibly adding it as a default for this directory.
                    CredentialStorage::defaultCredentialStorage().set(partition, credential, challenge.protectionSpace(), challenge.failureResponse().url());
                }

                if (d->m_delegate)
                    d->m_delegate->setAuthentication(credential.user(), credential.password());
                return;
            }
        }
    }

    d->m_currentWebChallenge = challenge;

    if (client()) {
        auto protectedThis = makeRef(*this);
        client()->didReceiveAuthenticationChallenge(this, d->m_currentWebChallenge);
    }
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
    ASSERT(isMainThread());

    if (challenge != d->m_currentWebChallenge)
        return;

    if (credential.isEmpty()) {
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    String partition = firstRequest().cachePartition();

    if (shouldUseCredentialStorage()) {
        if (challenge.failureResponse().httpStatusCode() == 401) {
            URL urlToStore = challenge.failureResponse().url();
            CredentialStorage::defaultCredentialStorage().set(partition, credential, challenge.protectionSpace(), urlToStore);
#if PLATFORM(MUI)
            String host = challenge.protectionSpace().host();
            String realm = challenge.protectionSpace().realm();
            //kprintf("Storing credentials in db for host %s realm %s (%s %s)\n", host.utf8().data(), realm.utf8().data(), credential.user().utf8().data(), credential.password().utf8().data());
            methodstack_push_sync(app, 4, MM_OWBApp_SetCredential, &host, &realm, &credential);
#endif
        }
    }

    if (d->m_delegate)
        d->m_delegate->setAuthentication(credential.user(), credential.password());

    clearAuthentication();
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    ASSERT(isMainThread());

    if (challenge != d->m_currentWebChallenge)
        return;

    clearAuthentication();

    didReceiveResponse(ResourceResponse(d->m_response), [this, protectedThis = makeRef(*this)] {
        ASSERT(isMainThread());

        if (d->m_delegate)
            d->m_delegate->continueDidReceiveResponse();
    });
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    ASSERT(isMainThread());

    if (challenge != d->m_currentWebChallenge)
        return;

    if (client()) {
        auto protectedThis = makeRef(*this);
        client()->receivedCancellation(this, challenge);
    }
}

void ResourceHandle::receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&)
{
    ASSERT_NOT_REACHED();
}

void ResourceHandle::receivedChallengeRejection(const AuthenticationChallenge&)
{
    ASSERT_NOT_REACHED();
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentialsPolicy, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    ASSERT(isMainThread());

    SynchronousLoaderClient client;
    bool defersLoading = false;
    bool shouldContentSniff = true;
    bool shouldContentEncodingSniff = true;
    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(context, request, &client, defersLoading, shouldContentSniff, shouldContentEncodingSniff));

    handle->d->m_delegate = adoptRef(new ResourceHandleCurlDelegate(handle.get()));
    handle->d->m_delegate->dispatchSynchronousJob();

    error = client.error();
    data.swap(client.mutableData());
    response = client.response();
}

void ResourceHandle::platformContinueSynchronousDidReceiveResponse()
{
    ASSERT(isMainThread());

    if (d->m_delegate)
        d->m_delegate->platformContinueSynchronousDidReceiveResponse();
}

} // namespace WebCore

#endif
