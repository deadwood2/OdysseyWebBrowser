#include "WebKit.h"
#include "WebStorageTrackerClient.h"

#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityOriginData.h>
#include <wtf/MainThread.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

WebStorageTrackerClient* WebStorageTrackerClient::sharedWebStorageTrackerClient()
{
    static WebStorageTrackerClient* sharedClient = new WebStorageTrackerClient();
    return sharedClient;
}

WebStorageTrackerClient::WebStorageTrackerClient()
{
}

WebStorageTrackerClient::~WebStorageTrackerClient()
{
}

void WebStorageTrackerClient::dispatchDidModifyOrigin(SecurityOrigin* origin)
{
}

void WebStorageTrackerClient::dispatchDidModifyOrigin(const String& originIdentifier)
{
}

void WebStorageTrackerClient::didFinishLoadingOrigins()
{
}

}
