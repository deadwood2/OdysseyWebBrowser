/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 * Copyright (C) 2009 Fabien Coeurjoly
 * Copyright (C) 2009 Stanislaw Szymczyk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebDownload.h"
#include "WebDownloadPrivate.h"

#include <wtf/text/CString.h>
#include "WebDownloadDelegate.h"
#include "WebError.h"
#include "WebFrameNetworkingContext.h"
#include "WebMutableURLRequest.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLCredential.h"
#include "WebURLResponse.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <wtf/text/WTFString.h>
#include <NotImplemented.h>
#include <ResourceError.h>
#include <ResourceHandle.h>
#include <ResourceHandleInternal.h>
#include <ResourceRequest.h>
#include <ResourceHandleClient.h>
#include <ResourceResponse.h>
#include <FileIOLinux.h>

#include "gui.h"
#include <clib/debug_protos.h>

using namespace WebCore;

/*****************************************************************************************************/

WebDownloadPrivate::WebDownloadPrivate()
		: allowOverwrite(false)
		, allowResume(false)
		, quiet(false)
		, currentSize(0)
		, totalSize(0)
		, startOffset(0)
		, state(WEBKIT_WEB_DOWNLOAD_STATE_ERROR)
		, outputChannel(0)
		, downloadClient(0)
		, resourceHandle(0)
		, resourceRequest(0)
		, dl(0)
		{}

class DownloadClient : public ResourceHandleClient
{
WTF_MAKE_NONCOPYABLE(DownloadClient);
public:
    DownloadClient(WebDownload*);

    void didStart();
    virtual void didReceiveResponse(ResourceHandle*, const WebCore::ResourceResponse&) override;
    virtual void didReceiveData(ResourceHandle*, const char*, unsigned, int) override;
    virtual void didFinishLoading(ResourceHandle*, double) override;
    virtual void didFail(ResourceHandle*, const ResourceError&) override;
    virtual void wasBlocked(ResourceHandle*) override;
    virtual void cannotShowURL(ResourceHandle*) override;
private:
    WebDownload* m_download;
};

DownloadClient::DownloadClient(WebDownload* download)
        : m_download(download)
{
}

void DownloadClient::didStart()
{
    if (!m_download->downloadDelegate())
        return;

    WebDownloadPrivate* priv = m_download->getWebDownloadPrivate();

    priv->state = WEBKIT_WEB_DOWNLOAD_STATE_STARTED;
    m_download->downloadDelegate()->didBegin(m_download);
}

void DownloadClient::didReceiveResponse(ResourceHandle*, const WebCore::ResourceResponse& response)
{
    if (!m_download->downloadDelegate())
        return;
    
    WebDownloadPrivate* priv = m_download->getWebDownloadPrivate();

    WebURLResponse *webResponse = WebURLResponse::createInstance(response);
    m_download->downloadDelegate()->didReceiveResponse(m_download, webResponse);

	// Can we resume and do we want to resume?

	if(priv->resourceHandle.get()->isResuming())
	{
		if(priv->resourceHandle.get()->canResume())
		{
			priv->outputChannel = new OWBFile(priv->resourceHandle.get()->path());

		    // Fail if not possible to open destination file to writing
			if(priv->outputChannel->open('a') == -1)
		    {
				ResourceError resourceError(String(WebKitErrorDomain), WebURLErrorCannotCreateFile, String(), String("Can't create file"));
		        didFail(priv->resourceHandle.get(), resourceError);
		        return;
		    }

			m_download->downloadDelegate()->didCreateDestination(m_download, priv->destinationPath.latin1().data());
		}
		else
		{
			ResourceError resourceError(String(WebKitErrorDomain), WebURLErrorCannotCreateFile, String(), String("Can't create file"));
	        didFail(priv->resourceHandle.get(), resourceError);
	        return;
		}
	}
	else
	{
	    WTF::String suggestedFilename = webResponse->suggestedFilename();
	    if(suggestedFilename.length() == 0)
	    {
	        suggestedFilename = response.url().string().substring(response.url().pathAfterLastSlash());

			// Strip unwanted parts
			size_t questionPos = suggestedFilename.find('?');
			size_t hashPos = suggestedFilename.find('#');
			unsigned pathEnd;

			if (hashPos != notFound && (questionPos == notFound || questionPos > hashPos))
				pathEnd = hashPos;
			else if (questionPos != notFound)
				pathEnd = questionPos;
			else
				pathEnd = suggestedFilename.length();

			suggestedFilename = suggestedFilename.left(pathEnd);
	    }

		m_download->downloadDelegate()->decideDestinationWithSuggestedFilename(m_download, suggestedFilename.utf8().data());
	    delete webResponse;

	    // Fail if destination file path is not set
		if(priv->destinationPath.isEmpty())
	    {
			ResourceError resourceError(String(WebKitErrorDomain), WebURLErrorCannotCreateFile, String(), String("Can't create file"));
	        didFail(priv->resourceHandle.get(), resourceError);
	        return;
	    }

		/*
		// If we're here, it means resume wasn't supported after all
		if(priv->resourceHandle.get()->isResuming()) 
		{
			kprintf("Wanted to resume, but can't\n");
			priv->allowResume = false;
			priv->allowOverwrite = true; // discuss that
		}
		*/

		if(priv->allowResume)
		{
			priv->resourceHandle.get()->setStartOffset(priv->startOffset);
			priv->resourceHandle.get()->resume(priv->destinationPath);
			return;
		}
		else
		{
			priv->outputChannel = new OWBFile(priv->destinationPath);

		    // Fail if destination file already exists and can't be overwritten
		    if(!priv->allowOverwrite && priv->outputChannel->open('r') != -1)
		    {
				ResourceError resourceError(String(WebKitErrorDomain), WebURLErrorCannotCreateFile, String(), String("Can't create file"));
		        didFail(priv->resourceHandle.get(), resourceError);
		        return;
		    }
		    else
		        priv->outputChannel->close();

		    // Fail if not possible to open destination file to writing
		    if(priv->outputChannel->open('w') == -1)
		    {
				ResourceError resourceError(String(WebKitErrorDomain), WebURLErrorCannotCreateFile, String(), String("Can't create file"));
		        didFail(priv->resourceHandle.get(), resourceError);
		        return;
		    }

			m_download->downloadDelegate()->didCreateDestination(m_download, priv->destinationPath.latin1().data());
		}
	}
}

void DownloadClient::didReceiveData(ResourceHandle*, const char* data, unsigned length, int lengthReceived)
{
    if (!m_download->downloadDelegate())
        return;

    WebDownloadPrivate* priv = m_download->getWebDownloadPrivate();

    m_download->downloadDelegate()->didReceiveDataOfLength(m_download, length);

	if(priv->outputChannel)
	{
		// FIXME: handle write errors somehow
		priv->outputChannel->write(data, length);
		priv->currentSize += length;
	}
}

void DownloadClient::didFinishLoading(ResourceHandle*, double)
{
    if (!m_download->downloadDelegate())
        return;

    WebDownloadPrivate* priv = m_download->getWebDownloadPrivate();

    priv->state = WEBKIT_WEB_DOWNLOAD_STATE_FINISHED;

	if(priv->outputChannel)
	{
	    priv->outputChannel->close();
	    delete priv->outputChannel;
	    priv->outputChannel = NULL;
	}

    m_download->downloadDelegate()->didFinish(m_download);
}

void DownloadClient::didFail(ResourceHandle*, const ResourceError& resourceError)
{
    if (!m_download->downloadDelegate())
        return;

    WebDownloadPrivate* priv = m_download->getWebDownloadPrivate();
    priv->state = WEBKIT_WEB_DOWNLOAD_STATE_ERROR;

    priv->resourceHandle->clearClient();
    priv->resourceHandle->cancel();

	if(priv->outputChannel)
	{
	    priv->outputChannel->close();
	    delete priv->outputChannel;
	    priv->outputChannel = NULL;
	}

    WebError *error = WebError::createInstance(resourceError);
    m_download->downloadDelegate()->didFailWithError(m_download, error);
}

void DownloadClient::wasBlocked(ResourceHandle*)
{
    notImplemented();
}

void DownloadClient::cannotShowURL(ResourceHandle*)
{
    notImplemented();
}

/*********************************************************************************************/

WebDownload::WebDownload()
{
    m_request = 0;
    m_priv = new WebDownloadPrivate();
}

void WebDownload::init(ResourceHandle* handle, const ResourceRequest* request, const WebCore::ResourceResponse* response, TransferSharedPtr<WebDownloadDelegate> delegate)
{
    m_delegate = delegate;

    m_priv->downloadClient = new DownloadClient(this);
    m_priv->state = WEBKIT_WEB_DOWNLOAD_STATE_CREATED;
    m_priv->currentSize = 0;
    m_priv->startOffset = 0;
    m_priv->outputChannel = NULL;
    m_request = WebMutableURLRequest::createInstance(*request);
    m_response = WebURLResponse::createInstance(*response);
    m_priv->resourceHandle = handle;
    m_priv->resourceRequest = NULL;
    m_priv->requestUri = String(request->url().string());

    if(handle->getInternal()->m_context.get())
	m_priv->originURL = ((WebFrameNetworkingContext *)handle->getInternal()->m_context.get())->url();
}

void WebDownload::init(const URL& url, TransferSharedPtr<WebDownloadDelegate> delegate)
{
    m_delegate = delegate;

    m_priv->resourceRequest = new ResourceRequest(url);
    m_request = WebMutableURLRequest::createInstance(*m_priv->resourceRequest);

    m_priv->downloadClient = new DownloadClient(this);
    m_priv->currentSize = 0;
    m_priv->startOffset = 0;
    m_priv->outputChannel = NULL;
    m_priv->state = WEBKIT_WEB_DOWNLOAD_STATE_CREATED;
    m_priv->resourceHandle = nullptr;
    m_response = NULL;
    m_priv->requestUri = String(url.string());
}

void WebDownload::init(const URL& url, const String& originURL, TransferSharedPtr<WebDownloadDelegate> delegate)
{
    m_delegate = delegate;

    m_priv->resourceRequest = new ResourceRequest(url);
    m_request = WebMutableURLRequest::createInstance(*m_priv->resourceRequest);

    m_priv->downloadClient = new DownloadClient(this);
    m_priv->currentSize = 0;
    m_priv->startOffset = 0;
    m_priv->outputChannel = NULL;
    m_priv->state = WEBKIT_WEB_DOWNLOAD_STATE_CREATED;
    m_priv->resourceHandle = nullptr;
    m_response = NULL;
    m_priv->requestUri = String(url.string());
    m_priv->originURL = originURL;
}

WebDownload::~WebDownload()
{
    if(m_priv->resourceHandle)
    {
        if(m_priv->state == WEBKIT_WEB_DOWNLOAD_STATE_CREATED || m_priv->state == WEBKIT_WEB_DOWNLOAD_STATE_STARTED)
        {
            m_priv->resourceHandle->clearClient();
            m_priv->resourceHandle->cancel();
        }

        m_priv->resourceHandle.release();
        m_priv->resourceHandle = nullptr;
    }
    
    if (m_priv->resourceRequest)
        delete m_priv->resourceRequest;

    delete m_priv->downloadClient;

    if(m_priv->outputChannel)
	{
        m_priv->outputChannel->close();
        delete m_priv->outputChannel;
    }

    delete m_priv;

    if(m_delegate)
        m_delegate = 0;
    if(m_request)
        delete m_request;
    if(m_response)
        delete m_response;
}

WebDownload* WebDownload::createInstance()
{
    WebDownload* instance = new WebDownload();
    return instance;
}

WebDownload* WebDownload::createInstance(ResourceHandle* handle, const ResourceRequest* request, const WebCore::ResourceResponse* response, TransferSharedPtr<WebDownloadDelegate> delegate)
{
    WebDownload* instance = new WebDownload();
    instance->init(handle, request, response, delegate);
    return instance;
}

WebDownload* WebDownload::createInstance(const URL& url, TransferSharedPtr<WebDownloadDelegate> delegate)
{
    WebDownload* instance = new WebDownload();
    instance->init(url, delegate);
    return instance;
}

WebDownload* WebDownload::createInstance(const URL& url, const String& originURL, TransferSharedPtr<WebDownloadDelegate> delegate)
{
    WebDownload* instance = new WebDownload();
	instance->init(url, originURL, delegate);
    return instance;
}

void WebDownload::initWithRequest(
        /* [in] */ WebMutableURLRequest* request,
        /* [in] */ TransferSharedPtr<WebDownloadDelegate> delegate)
{
    notImplemented();
}

void WebDownload::initToResumeWithBundle(
        /* [in] */ const char* bundlePath,
        /* [in] */ TransferSharedPtr<WebDownloadDelegate> delegate)
{
    notImplemented();
}

bool WebDownload::canResumeDownloadDecodedWithEncodingMIMEType(
        /* [in] */ const char*)
{
    notImplemented();
    return false;
}

void WebDownload::start(bool quiet)
{
    if (m_priv->state != WEBKIT_WEB_DOWNLOAD_STATE_CREATED)
        return;

	// If quiet option is passed explicitely, use it
	if(quiet)
	{
		m_priv->quiet = quiet;
	}
	// Else, honor download general setting
	else
	{
		m_priv->quiet = getv(app, MA_OWBApp_DownloadStartAutomatically);
	}

	//kprintf("quiet %d\n", m_priv->quiet);

    if (m_priv->resourceHandle)
    {
		m_priv->resourceHandle->setClientInternal(m_priv->downloadClient);
		m_priv->resourceHandle->getInternal()->m_disableEncoding = (m_priv->requestUri.endsWith(".gz") || m_priv->requestUri.endsWith(".tgz")) == true; // HACK to disable on-the-fly gzip decoding
		m_priv->downloadClient->didStart();
        m_priv->downloadClient->didReceiveResponse(m_priv->resourceHandle.get(), m_response->resourceResponse());
    }
    else
    {
        m_priv->downloadClient->didStart();
		m_priv->resourceHandle = ResourceHandle::create(NULL, m_request->resourceRequest(), m_priv->downloadClient, false, false);
		if(m_priv->resourceHandle)
		{
			m_priv->resourceHandle->getInternal()->m_disableEncoding = (m_priv->requestUri.endsWith(".gz") || m_priv->requestUri.endsWith(".tgz")) == true; // HACK to disable on-the-fly gzip decoding
		}
    }
}

void WebDownload::cancel()
{
    if (!(m_priv->state == WEBKIT_WEB_DOWNLOAD_STATE_CREATED || m_priv->state == WEBKIT_WEB_DOWNLOAD_STATE_STARTED))
        return;

    if (m_priv->resourceHandle)
    {
		m_priv->resourceHandle->clearClient();
		m_priv->resourceHandle->cancel();
    }

    if(m_priv->outputChannel)
    {
        m_priv->outputChannel->close();
        delete m_priv->outputChannel;
		m_priv->outputChannel = NULL;
    }

    m_priv->state = WEBKIT_WEB_DOWNLOAD_STATE_CANCELLED;
}

void WebDownload::cancelForResume()
{
    notImplemented();
}

bool WebDownload::deletesFileUponFailure()
{
    return false;
}

char* WebDownload::bundlePathForTargetPath(const char* targetPath)
{
    notImplemented();
    return NULL;
}

WebMutableURLRequest* WebDownload::request()
{
    return m_request;
}

void WebDownload::setDeletesFileUponFailure(bool deletesFileUponFailure)
{
    notImplemented();
}

void WebDownload::setDestination(const char* path, bool allowOverwrite, bool allowResume)
{
    m_priv->destinationPath = path;
    m_priv->allowOverwrite = allowOverwrite;
    m_priv->allowResume = allowResume;
}

void WebDownload::cancelAuthenticationChallenge(WebURLAuthenticationChallenge*)
{
    notImplemented();
}

void WebDownload::continueWithoutCredentialForAuthenticationChallenge(WebURLAuthenticationChallenge* challenge)
{
    notImplemented();
}

void WebDownload::useCredential(WebURLCredential* credential, WebURLAuthenticationChallenge* challenge)
{
    notImplemented();
}

void WebDownload::setCommandUponCompletion(const char* command)
{
    m_priv->command = String(command);
}
