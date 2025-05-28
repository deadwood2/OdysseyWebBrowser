#import "WkDownload.h"

namespace WebCore {
	class ResourceHandle;
	class ResourceRequest;
	class ResourceResponse;
}

@interface WkDownload (Private)

+ (WkDownload *)downloadWithHandle:(WebCore::ResourceHandle*)handle request:(const WebCore::ResourceRequest&)request response:(const WebCore::ResourceResponse&)response withDelegate:(id<WkDownloadDelegate>)delegate;

@end
