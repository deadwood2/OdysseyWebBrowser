/*
 * Copyright (C) 2004, 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2005, 2006 Michael Emmel mike.emmel@gmail.com
 * All rights reserved.
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

#include "CachedResourceLoader.h"
#include "CookieManager.h"
#include "CredentialStorage.h"
#include "FileSystem.h"
#include "Logging.h"
#include "NetworkingContext.h"
#include "NotImplemented.h"
#include "ResourceHandleInternal.h"
#include "ResourceHandleManager.h"
#include "SharedBuffer.h"
#include "SSLHandle.h"

#if PLATFORM(WIN) && USE(CF)
#include <wtf/PassRefPtr.h>
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(MUI)
#include "gui.h"
#include <clib/debug_protos.h>
#define D(x)
#undef String
#undef set
#undef get
#endif

namespace WebCore {

class WebCoreSynchronousLoader : public ResourceHandleClient {
public:
    WebCoreSynchronousLoader();

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse&);
    virtual void didReceiveData(ResourceHandle*, const char*, unsigned, int encodedDataLength);
    virtual void didFinishLoading(ResourceHandle*, double /*finishTime*/);
    virtual void didFail(ResourceHandle*, const ResourceError&);

    ResourceResponse resourceResponse() const { return m_response; }
    ResourceError resourceError() const { return m_error; }
    Vector<char> data() const { return m_data; }

private:
    ResourceResponse m_response;
    ResourceError m_error;
    Vector<char> m_data;
};

WebCoreSynchronousLoader::WebCoreSynchronousLoader()
{
}

void WebCoreSynchronousLoader::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    m_response = response;
}

void WebCoreSynchronousLoader::didReceiveData(ResourceHandle*, const char* data, unsigned length, int)
{
    m_data.append(data, length);
}

void WebCoreSynchronousLoader::didFinishLoading(ResourceHandle*, double)
{
}

void WebCoreSynchronousLoader::didFail(ResourceHandle*, const ResourceError& error)
{
    m_error = error;
}

ResourceHandleInternal::~ResourceHandleInternal()
{
    fastFree(m_url);
    if (m_customHeaders)
        curl_slist_free_all(m_customHeaders);
}

ResourceHandle::~ResourceHandle()
{
    cancel();
}

bool ResourceHandle::start()
{
    // The frame could be null if the ResourceHandle is not associated to any
    // Frame, e.g. if we are downloading a file.
    // If the frame is not null but the page is null this must be an attempted
    // load from an unload handler, so let's just block it.
    // If both the frame and the page are not null the context is valid.
    if (d->m_context && !d->m_context->isValid())
        return false;

#if PLATFORM(MUI)
    if ((!d->m_user.isEmpty() || !d->m_pass.isEmpty()) && !shouldUseCredentialStorage()) {
        // Credentials for ftp can only be passed in URL, the didReceiveAuthenticationChallenge delegate call won't be made.
        URL urlWithCredentials(d->m_firstRequest.url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        d->m_firstRequest.setURL(urlWithCredentials);
    }
#endif

    ResourceHandleManager::sharedInstance()->add(this);
    return true;
}

void ResourceHandle::cancel()
{
    clearClient();
    ResourceHandleManager::sharedInstance()->cancel(this);
}

void ResourceHandle::setHostAllowsAnyHTTPSCertificate(const String& host)
{
    allowsAnyHTTPSCertificateHosts(host.lower());
}

void ResourceHandle::setClientCertificateInfo(const String& host, const String& certificate, const String& key)
{
    if (fileExists(certificate))
        addAllowedClientCertificate(host, certificate, key);
    else
        LOG(Network, "Invalid client certificate file: %s!\n", certificate.latin1().data());
}

#if PLATFORM(WIN) && USE(CF)
// FIXME:  The CFDataRef will need to be something else when
// building without 
static HashMap<String, RetainPtr<CFDataRef> >& clientCerts()
{
    static HashMap<String, RetainPtr<CFDataRef> > certs;
    return certs;
}

void ResourceHandle::setClientCertificate(const String& host, CFDataRef cert)
{
    clientCerts().set(host.lower(), cert);
}
#endif

void ResourceHandle::platformSetDefersLoading(bool defers)
{
    if (!d->m_handle)
        return;

    if (defers) {
        CURLcode error = curl_easy_pause(d->m_handle, CURLPAUSE_ALL);
        // If we could not defer the handle, so don't do it.
        if (error != CURLE_OK)
            return;
    } else {
        CURLcode error = curl_easy_pause(d->m_handle, CURLPAUSE_CONT);
        if (error != CURLE_OK)
            // Restarting the handle has failed so just cancel it.
            cancel();
    }
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
    // Cookies are a part of the http protocol only
    if (!String(d->m_url).startsWith("http"))
	return;

    if (url.isEmpty())
        url = URL(ParsedURLString, d->m_url);

    // Prepare a cookie header if there are cookies related to this url.
    String cookiePairs = cookieManager().getCookie(url, WithHttpOnlyCookies);
    if (!cookiePairs.isEmpty() && d->m_handle) {
        CString cookieChar = cookiePairs.ascii();
        LOG(Network, "CURL POST Cookie : %s \n", cookieChar.data());
        curl_easy_setopt(d->m_handle, CURLOPT_COOKIE, cookieChar.data());
    }
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

String ResourceHandle::path()
{
    ResourceHandleInternal* d = getInternal();
    return d->m_path;
}

void ResourceHandle::resume(String path)
{
    cancel();
    d->m_path = path;
    ResourceHandleManager::sharedInstance()->add(this);
}
#endif

bool ResourceHandle::shouldUseCredentialStorage()
{
    return (!client() || client()->shouldUseCredentialStorage(this)) && firstRequest().url().protocolIsInHTTPFamily();
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials storedCredentials, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    UNUSED_PARAM(storedCredentials);
    WebCoreSynchronousLoader syncLoader;
    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(context, request, &syncLoader, true, false));

    ResourceHandleManager* manager = ResourceHandleManager::sharedInstance();

    manager->dispatchSynchronousJob(handle.get());

    error = syncLoader.resourceError();
    data = syncLoader.data();
    response = syncLoader.resourceResponse();
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    if (!d->m_user.isNull() && !d->m_pass.isNull()) {
        Credential credential(d->m_user, d->m_pass, CredentialPersistenceNone);

        URL urlToStore;
        if (challenge.failureResponse().httpStatusCode() == 401)
            urlToStore = challenge.failureResponse().url();
        CredentialStorage::defaultCredentialStorage().set(credential, challenge.protectionSpace(), urlToStore);
        
        String userpass = credential.user() + ":" + credential.password();
        curl_easy_setopt(d->m_handle, CURLOPT_USERPWD, userpass.utf8().data());

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
            CredentialStorage::defaultCredentialStorage().remove(challenge.protectionSpace());
        }

        if (!challenge.previousFailureCount()) {
            Credential credential = CredentialStorage::defaultCredentialStorage().get(challenge.protectionSpace());
            if (!credential.isEmpty() && credential != d->m_initialCredential) {
                ASSERT(credential.persistence() == CredentialPersistenceNone);
                if (challenge.failureResponse().httpStatusCode() == 401) {
                    // Store the credential back, possibly adding it as a default for this directory.
                    CredentialStorage::defaultCredentialStorage().set(credential, challenge.protectionSpace(), challenge.failureResponse().url());
                }

                String userpass = credential.user() + ":" + credential.password();
                curl_easy_setopt(d->m_handle, CURLOPT_USERPWD, userpass.utf8().data());
                return;
            }
        }
    }

    d->m_currentWebChallenge = challenge;
    
    if (client())
        client()->didReceiveAuthenticationChallenge(this, d->m_currentWebChallenge);
}

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
    if (challenge != d->m_currentWebChallenge)
        return;

    if (credential.isEmpty()) {
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    if (shouldUseCredentialStorage()) {
        if (challenge.failureResponse().httpStatusCode() == 401) {
            URL urlToStore = challenge.failureResponse().url();
            CredentialStorage::defaultCredentialStorage().set(credential, challenge.protectionSpace(), urlToStore);
#if PLATFORM(MUI)
            String host = challenge.protectionSpace().host();
            String realm = challenge.protectionSpace().realm();
            //kprintf("Storing credentials in db for host %s realm %s (%s %s)\n", host.utf8().data(), realm.utf8().data(), credential.user().utf8().data(), credential.password().utf8().data());
            methodstack_push_sync(app, 4, MM_OWBApp_SetCredential, &host, &realm, &credential);
#endif
        }
    }

    String userpass = credential.user() + ":" + credential.password();
    curl_easy_setopt(d->m_handle, CURLOPT_USERPWD, userpass.utf8().data());

    clearAuthentication();
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    if (challenge != d->m_currentWebChallenge)
        return;

    String userpass = "";
    curl_easy_setopt(d->m_handle, CURLOPT_USERPWD, userpass.utf8().data());

    clearAuthentication();
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    if (challenge != d->m_currentWebChallenge)
        return;

    if (client())
        client()->receivedCancellation(this, challenge);
}

void ResourceHandle::receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&)
{
    ASSERT_NOT_REACHED();
}

void ResourceHandle::receivedChallengeRejection(const AuthenticationChallenge&)
{
    ASSERT_NOT_REACHED();
}

} // namespace WebCore

#endif
