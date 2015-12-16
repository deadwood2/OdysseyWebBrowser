#ifndef _DOWNLOADDELEGATEMORPHOS_H
#define _DOWNLOADDELEGATEMORPHOS_H

#include "WebDownloadDelegate.h"

class DownloadDelegateMorphOS : public WebDownloadDelegate
{
public:

    static TransferSharedPtr<DownloadDelegateMorphOS> createInstance()
    {
        return new DownloadDelegateMorphOS();
    }

    ~DownloadDelegateMorphOS();
private:
    DownloadDelegateMorphOS();
    void decideDestinationWithSuggestedFilename(WebDownload *download, const char* filename);
    void didCancelAuthenticationChallenge(WebDownload* download, WebURLAuthenticationChallenge* challenge);
    void didCreateDestination(WebDownload* download, const char* destination);
    void didReceiveAuthenticationChallenge(WebDownload* download, WebURLAuthenticationChallenge* challenge);
    void didReceiveDataOfLength(WebDownload* download, unsigned length);
    void didReceiveResponse(WebDownload* download, WebURLResponse* response);
    void didReceiveExpectedContentLength(WebDownload* download, long long contentLength);
    void willResumeWithResponse(WebDownload* download, WebURLResponse* response, long long fromByte);
    WebMutableURLRequest* willSendRequest(WebDownload* download, WebMutableURLRequest* request, WebURLResponse* redirectResponse);
    bool shouldDecodeSourceDataOfMIMEType(WebDownload*, const char*);
    void didBegin(WebDownload* download);
    void didFinish(WebDownload* download);
    void didFailWithError(WebDownload* download, WebError* error);
    void didCreateDownload(WebDownload* download);
};

#endif
