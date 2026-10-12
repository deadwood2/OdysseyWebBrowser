
#include "StorageTrackerClient.h"
#include <WebCore/SecurityOrigin.h>

namespace WebKit {

class WebStorageTrackerClient : public WebCore::StorageTrackerClient {
public:
    static WebStorageTrackerClient* sharedWebStorageTrackerClient();
    
    virtual ~WebStorageTrackerClient();
    void dispatchDidModifyOrigin(const String& originIdentifier) override;
    virtual void dispatchDidModifyOrigin(WebCore::SecurityOrigin*);

private:
    WebStorageTrackerClient();

    // WebCore::StorageTrackerClient
    void didFinishLoadingOrigins() override;
};

}
