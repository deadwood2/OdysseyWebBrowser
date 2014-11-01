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

#ifndef WebError_h
#define WebError_h


/**
 *  @file  WebError.h
 *  WebError description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */

#include "WebKitTypes.h"

class HTTPHeaderPropertyBag;

namespace WebCore {
    class ResourceError;
}

enum {
    WebKitErrorCannotShowMIMEType =                             100,
    WebKitErrorCannotShowURL =                                  101,
    WebKitErrorFrameLoadInterruptedByPolicyChange =             102,
    WebKitErrorCannotUseRestrictedPort =                        103,
};

enum {
    WebKitErrorCannotFindPlugIn =                               200,
    WebKitErrorCannotLoadPlugIn =                               201,
    WebKitErrorJavaUnavailable =                                202,
};

enum
{
    WebURLErrorUnknown =                         -1,
    WebURLErrorCancelled =                       -999,
    WebURLErrorBadURL =                          -1000,
    WebURLErrorTimedOut =                        -1001,
    WebURLErrorUnsupportedURL =                  -1002,
    WebURLErrorCannotFindHost =                  -1003,
    WebURLErrorCannotConnectToHost =             -1004,
    WebURLErrorNetworkConnectionLost =           -1005,
    WebURLErrorDNSLookupFailed =                 -1006,
    WebURLErrorHTTPTooManyRedirects =            -1007,
    WebURLErrorResourceUnavailable =             -1008,
    WebURLErrorNotConnectedToInternet =          -1009,
    WebURLErrorRedirectToNonExistentLocation =   -1010,
    WebURLErrorBadServerResponse =               -1011,
    WebURLErrorUserCancelledAuthentication =     -1012,
    WebURLErrorUserAuthenticationRequired =      -1013,
    WebURLErrorZeroByteResource =                -1014,
    WebURLErrorFileDoesNotExist =                -1100,
    WebURLErrorFileIsDirectory =                 -1101,
    WebURLErrorNoPermissionsToReadFile =         -1102,
    WebURLErrorSecureConnectionFailed =          -1200,
    WebURLErrorServerCertificateHasBadDate =     -1201,
    WebURLErrorServerCertificateUntrusted =      -1202,
    WebURLErrorServerCertificateHasUnknownRoot = -1203,
    WebURLErrorServerCertificateNotYetValid =    -1204,
    WebURLErrorClientCertificateRejected =       -1205,
    WebURLErrorClientCertificateRequired =       -1206,
    WebURLErrorCannotLoadFromNetwork =           -2000,

    // Download and file I/O errors
    WebURLErrorCannotCreateFile =                -3000,
    WebURLErrorCannotOpenFile =                  -3001,
    WebURLErrorCannotCloseFile =                 -3002,
    WebURLErrorCannotWriteToFile =               -3003,
    WebURLErrorCannotRemoveFile =                -3004,
    WebURLErrorCannotMoveFile =                  -3005,
    WebURLErrorDownloadDecodingFailedMidStream = -3006,
    WebURLErrorDownloadDecodingFailedToComplete =-3007,
};

// FIXME: We should have better names! Also those should be localized.
#define WebKitCannotShowURL " WebKitCannotShowURL"
#define WebKitErrorDomain "WebKitErrorDomain"
#define WebURLErrorDomain "URLErrorDomain"
#define WebKitErrorMIMETypeKey "WebKitErrorMIMETypeKey"
#define WebKitErrorMIMETypeError "WebKitErrorMIMETypeError"
#define WebKitErrorPlugInNameKey "WebKitErrorPlugInNameKey"
#define WebKitErrorPlugInPageURLStringKey "WebKitErrorPlugInPageURLStringKey"
#define WebPOSIXErrorDomain "NSPOSIXErrorDomain"
#define WebKitFileDoesNotExist "WebKitFileDoesNotExist"

class WEBKIT_OWB_API WebError {
public:
    /**
     * create a new instance of WebError
     * @param[out]: WebError
     */
    static WebError* createInstance();
protected:
    friend class DownloadClient;
    friend class WebDataSource;
    friend class WebDownload;
    friend class WebFrameLoaderClient;
    friend class WebURLAuthenticationChallenge;
    /**
     * create a new instance of WebError
     * @param[in]: ResourceError
     * @param[in]: HTTPHeaderPropertyBag
     * @param[out]: WebError
     */
    static WebError* createInstance(const WebCore::ResourceError&, HTTPHeaderPropertyBag* userInfo = 0);

private:
    /**
     * WebError constructor
     * @param[in]: ResourceError
     * @param[in]: HTTPHeaderPropertyBag
     */
    WebError(const WebCore::ResourceError&, HTTPHeaderPropertyBag* userInfo);

public:

    /**
     * WebError destructor
     */
    virtual ~WebError();

    /**
     * initialise WebError
     * @param[in]: domain
     * @param[in]: code
     * @param[in]: url
     */
    virtual void init(const char* domain, int code, const char* url);

    /**
     * get code
     * @param[out]: code
     */
    virtual int code();

    /**
     * get domain
     * @param[out]: domain
     */
    virtual const char* domain();

    /**
     * get localizedDescription
     * @param[out]: localizedDescription
     */
    virtual const char* localizedDescription();

    /**
     * get localized failure reason
     * @param[out]: localized failure reason
     */
    virtual const char* localizedFailureReason();

    /**
     * get localized recovery suggestion
     * @param[out]: localized recovery suggestion
     */
    virtual const char* localizedRecoverySuggestion();

    /**
     * get user informations
     * @param[out]: user informations
     */
    virtual HTTPHeaderPropertyBag* userInfo();

    /**
     * get failing URL
     * @param[out]: url
     */
    virtual const char* failingURL();

    /**
     * test if the error is policy change error
     * @param[out]: status
     */
    virtual bool isPolicyChangeError();


    /**
     * sslPeerCertificate
     * Not Implemented
     */
    //virtual OLE_HANDLE sslPeerCertificate();
    
    /**
     * get resource error
     * @param[out]: resource error
     */
    const WebCore::ResourceError& resourceError() const;

private:
    HTTPHeaderPropertyBag* m_userInfo;
    WebCore::ResourceError* m_error;
};

#endif
