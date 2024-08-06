/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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

#ifndef WebDownload_h
#define WebDownload_h


/**
 *  @file  WebDownload.h
 *  WebDownload description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */
#include "SharedPtr.h"
#include "TransferSharedPtr.h"
#include "WebDownloadDelegate.h"
#include "WebKitTypes.h"

namespace WebCore {
    class URL;
    class ResourceHandle;
    class ResourceRequest;
    class ResourceResponse;
}

namespace WTF {
	class String;
}

class WebURLAuthenticationChallenge;
class WebURLCredential;
class WebMutableURLRequest;
class WebURLResponse;

typedef enum
{
    WEBKIT_WEB_DOWNLOAD_ERROR_CANCELLED_BY_USER,
    WEBKIT_WEB_DOWNLOAD_ERROR_NETWORK
} WebDownloadError;

class DownloadClient;

class WebDownloadPrivate;

class WEBKIT_OWB_API WebDownload
{
public:

    /**
     * create new instance of WebDownload
     * @param[in]: url
     * @param[in]: WebDownloadDelegate
     */
    static WebDownload* createInstance(const WebCore::URL&, TransferSharedPtr<WebDownloadDelegate>);

    /**
     * create new instance of WebDownload
     * @param[in]: url
	 * @param[in]: originURL
     * @param[in]: WebDownloadDelegate
     */
	static WebDownload* createInstance(const WebCore::URL&, const WTF::String&, TransferSharedPtr<WebDownloadDelegate>);

    /**
     * create new instance of WebDownload
     * @param[in]: resource handle
     * @param[in]: resource request
     * @param[in]: resource response
     * @param[in]: WebDownloadDelegate
     */
    static WebDownload* createInstance(WebCore::ResourceHandle*, const WebCore::ResourceRequest*, const WebCore::ResourceResponse*, TransferSharedPtr<WebDownloadDelegate>);

    /**
     * create new instance of WebDownload
     */
    static WebDownload* createInstance();
private:

    /**
     * WebDownload constructor
     */
    WebDownload();

    /**
     * initialise WebDownload
     * @param[in]: resource handle
     * @param[in]: resource request
     * @param[in]: resource response     */
    void init(WebCore::ResourceHandle*, const WebCore::ResourceRequest*, const WebCore::ResourceResponse*, TransferSharedPtr<WebDownloadDelegate>);

    /**
     * initialise WebDownload
     * @param[in]: url
     * @param[in]: WebDownloadDelegate
     */
    void init(const WebCore::URL&, TransferSharedPtr<WebDownloadDelegate>);

    /**
     * initialise WebDownload
     * @param[in]: url
	 * @param[in]: orginURL
     * @param[in]: WebDownloadDelegate
     */
	void init(const WebCore::URL&, const WTF::String&, TransferSharedPtr<WebDownloadDelegate>);
public:

    /**
     *  WebDownload destructor
     */
    virtual ~WebDownload();

    /**
     * initialise with request
     * Not Implemented
     * @param[in]: WebMutableURLRequest
     * @param[in]: WebDownloadDelegate
     */
    virtual void initWithRequest(WebMutableURLRequest* request, TransferSharedPtr<WebDownloadDelegate> delegate);

    /**
     *  initialise to resume with bundle
     * Not Implemented
     * @param[in]: bundle path
     * @param[in]: DefaultDownloadDelegate
     */
    virtual void initToResumeWithBundle(const char* bundlePath, TransferSharedPtr<WebDownloadDelegate> delegate);

    /**
     * can resume download decoded with encoding MIMEType
     * Not Implemented
     * @param[in]: MIMEType
     * @param[out] : status
     */
    virtual bool canResumeDownloadDecodedWithEncodingMIMEType(const char* mimeType);

    /**
     * start
     * Not Implemented
     */
	virtual void start(bool quiet = false);

    /**
     * cancel
     * Not Implemented
     */
    virtual void cancel();

    /**
     * cancel for resume
     * Not Implemented
     */
    virtual void cancelForResume();

    /**
     * deletes file upon failure
     * Not Implemented
     * @param[out]: status
     */
    virtual bool deletesFileUponFailure();

    /**
     * get bundle path for target path
     * Not Implemented
     * @param[in]: target path
     * @param[out]: bundle path
     */
    virtual char* bundlePathForTargetPath(const char* target);

    /**
     * get request
     * Not Implemented
     * @param[out]: WebMutableURLRequest
     */
    virtual WebMutableURLRequest* request();

    /**
     * set deletes file upon failure
     * Not Implemented
     * @param[in]: status
     */
    virtual void setDeletesFileUponFailure(bool deletesFileUponFailure);

    /**
     * set destination
     * Not Implemented
     * @param[in]: path
     * @param[in]: allow overwrite
     */
	virtual void setDestination(const char* path, bool allowOverwrite, bool allowResume);

    /**
     * cancel authentication challenge
     * Not Implemented
     * @param[in]: WebURLAuthenticationChallenge
     */
    virtual void cancelAuthenticationChallenge(WebURLAuthenticationChallenge* challenge);

    /**
     * continue without credential for authentication challenge
     * Not Implemented
     * @param[in]: WebURLAuthenticationChallenge
     */
    virtual void continueWithoutCredentialForAuthenticationChallenge(WebURLAuthenticationChallenge* challenge);

    /**
     * use credential
     * Not Implemented
     * @param[in]: credential
     * @param[in]: challenge
     */
    virtual void useCredential(WebURLCredential* credential, WebURLAuthenticationChallenge* challenge);

	virtual void setCommandUponCompletion(const char* command);

    WebDownloadPrivate* getWebDownloadPrivate() { return m_priv; }

    TransferSharedPtr<WebDownloadDelegate> downloadDelegate() { return m_delegate; }

protected:
    WebMutableURLRequest* m_request;
    WebURLResponse *m_response;
    SharedPtr<WebDownloadDelegate> m_delegate;
    WebDownloadPrivate *m_priv;

#ifndef NDEBUG
    double m_startTime;
    double m_dataTime;
    int m_received;
#endif
};


#endif
