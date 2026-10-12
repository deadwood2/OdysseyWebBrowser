#import "WkDownload.h"

namespace WebCore {
	class ResourceHandle;
	class ResourceRequest;
	class ResourceResponse;
}

namespace WTF {
    class URL;
}

@interface WkDownload (Private)

+ (WkDownload *)downloadWithHandle:(WebCore::ResourceHandle*)handle request:(const WebCore::ResourceRequest&)request response:(const WebCore::ResourceResponse&)response withDelegate:(id<WkDownloadDelegate>)delegate;

+ (WkDownload *)downloadWithDataURL:(const WTF::URL&)url pageURL:(OBURL *)pageurl filename:(OBString *)name withDelegate:(id<WkDownloadDelegate>)delegate;
+ (WkDownload *)downloadWithBlobURL:(const WTF::URL&)url pageURL:(OBURL *)pageurl filename:(OBString *)name withDelegate:(id<WkDownloadDelegate>)delegate;

- (void)setFilename:(OBString *)f;

- (OBString *)temporaryPath;
- (void)setTemporaryPath:(OBString *)path;

@end
