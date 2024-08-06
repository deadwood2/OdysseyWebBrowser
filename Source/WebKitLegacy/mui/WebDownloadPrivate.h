/*
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

#ifndef WebDownloadPrivate_h
#define WebDownloadPrivate_h


/**
 *  @file  WebDownloadPrivate.h
 *  WebDownloadPrivate description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2013/03/06 00:13:17 $
 */

#include "BALBase.h"
#include <wtf/text/WTFString.h>
#include "FileIOLinux.h"

namespace WebCore {
    class KURL;
    class ResourceHandle;
    struct ResourceRequest;
    class ResourceResponse;
    class File;
}

class DefaultDownloadDelegate;
class WebURLAuthenticationChallenge;
class WebURLCredential;
class WebMutableURLRequest;

typedef enum
{
    WEBKIT_WEB_DOWNLOAD_STATE_ERROR = -1,
    WEBKIT_WEB_DOWNLOAD_STATE_CREATED = 0,
    WEBKIT_WEB_DOWNLOAD_STATE_STARTED,
    WEBKIT_WEB_DOWNLOAD_STATE_CANCELLED,
    WEBKIT_WEB_DOWNLOAD_STATE_FINISHED
} WebDownloadState;

struct download;

class WebDownloadPrivate
{
public:
	WebDownloadPrivate();

    WTF::String requestUri;
	WTF::String originURL;
    WTF::String destinationPath;
	WTF::String command;
	WTF::String mimetype;
    bool allowOverwrite;
	bool allowResume;
	bool quiet;
    unsigned long long currentSize;
	unsigned long long totalSize;
	unsigned long long startOffset;
    WebDownloadState state;
	WebCore::OWBFile* outputChannel;
    DownloadClient* downloadClient;
    RefPtr<WebCore::ResourceHandle> resourceHandle;
    WebCore::ResourceRequest* resourceRequest;
	struct downloadnode *dl;
};

#endif
