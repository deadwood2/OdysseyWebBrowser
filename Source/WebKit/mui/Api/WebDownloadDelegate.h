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
#ifndef WebDownloadDelegate_h
#define WebDownloadDelegate_h


/**
 *  @file  WebDownloadDelegate.h
 *  WebDownloadDelegate description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */

#include "SharedObject.h"
#include "WebKitTypes.h"
#include <set>
#include <clib/debug_protos.h>

class WebError;
class WebDownload;
class WebURLResponse;
class WebMutableURLRequest;
class WebURLAuthenticationChallenge;

class WEBKIT_OWB_API WebDownloadDelegate : public SharedObject<WebDownloadDelegate>
{
public:
    /**
     *  WebDownloadDelegate default constructor
     */
	WebDownloadDelegate()
	{
	};
public:

    /**
     * WebDownloadDelegate destructor
     */
	virtual ~WebDownloadDelegate()
	{
        m_downloads.clear();
        /*std::set<WebDownload*>::iterator i = m_downloads.begin();
        for (;i != m_downloads.end(); ++i)
            delete (*i);*/
    };

    /**
     * set destination of download with the suggested filename
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: filename
     * @code
     * d->decideDestinationWithSuggestedFilename(download, filename);
     * @endcode
     */
    virtual void decideDestinationWithSuggestedFilename(WebDownload* download, const char* filename) = 0;

    /**
     * did cancel authentication challenge
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: WebURLAuthenticationChallenge
     * @code
     * d->didCancelAuthenticationChallenge(download, challenge);
     * @endcode
     */
    virtual void didCancelAuthenticationChallenge(WebDownload* download, WebURLAuthenticationChallenge* challenge) = 0;

    /**
     * did create destination
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: destination
     * @code
     * d->didCreateDestination(download, destination);
     * @endcode
     */
    virtual void didCreateDestination(WebDownload* download, const char* destination) = 0;

    /**
     * did fail with error
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: Error
     * @code
     * d->didFailWithError(download, error);
     * @endcode
     */
    virtual void didFailWithError(WebDownload* download, WebError* error) = 0;

    /**
     * did receive authentication challenge
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: WebURLAuthenticationChallenge
     * @code
     * d->didReceiveAuthenticationChallenge(download, challenge);
     * @endcode
     */
    virtual void didReceiveAuthenticationChallenge(WebDownload* download, WebURLAuthenticationChallenge* challenge) = 0;

    /**
     * did receive data of length
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: length
     * @code
     * d->didReceiveDataOfLength(download, length);
     * @endcode
     */
    virtual void didReceiveDataOfLength(WebDownload* download, unsigned length) = 0;

    /**
     * did receive response
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: WebMutableURLResponse
     * @code
     * d->didReceiveResponse(download, response);
     * @endcode
     */
    virtual void didReceiveResponse(WebDownload* download, WebURLResponse* response) = 0;

    /**
     * should decode source data of MIMEType
     * Not Implemented
     * @param[in]: download
     * @param[in]: encodingType
     * @code
     * bool s = d->shouldDecodeSourceDataOfMIMEType(download, encodingType);
     * @endcode
     */
    virtual bool shouldDecodeSourceDataOfMIMEType(WebDownload* download, const char *encodingType) = 0;

    /**
     * will resume with response
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: WebMutableURLResponse
     * @param[in]: long long
     * @code
     * d->willResumeWithResponse(download, response, fromByte);
     * @endcode
     */
    virtual void willResumeWithResponse(WebDownload* download, WebURLResponse* response, long long fromByte) = 0;

    /**
     * will send request
     * Not Implemented
     * @param[in]: WebDownload
     * @param[in]: WebMutableURLRequest
     * @param[in]: WebURLResponse
     * @param[out]: WebMutableURLRequest
     * @code
     * WebMutableURLRequest *w = d->willSendRequest(download, request, redirectResponse);
     * @endcode
     */
    virtual WebMutableURLRequest* willSendRequest(WebDownload* download, WebMutableURLRequest* request,  WebURLResponse* redirectResponse) = 0;

    /**
     * did begin
     * @param[in]: WebDownload
     * @code
     * d->didBegin(download);
     * @endcode
     */
    virtual void didBegin(WebDownload* download) = 0;

    /**
     * did finish
     * @param[in]: WebDownload
     * @code
     * d->didFinish(download);
     * @endcode
     */
    virtual void didFinish(WebDownload* download) = 0;

    // WebDownloadDelegate

    /**
     * register download
     * @param[in]: WebDownload
     * @code
     * d->registerDownload(download);
     * @endcode
     */
    void registerDownload(WebDownload* download)
    {
        if (m_downloads.find(download) != m_downloads.end())
            return;
        m_downloads.insert(download);
    }

    /**
     * unregister download
     * @param[in]: WebDownload
     * @code
     * d->unregisterDownload(download);
     * @endcode
     */
    void unregisterDownload(WebDownload* download)
    {
        if (m_downloads.find(download) != m_downloads.end()) {
            m_downloads.erase(download);
        }
    }

protected:
    std::set<WebDownload*> m_downloads;
};

#endif
