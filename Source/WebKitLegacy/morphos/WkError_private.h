#import "WkError.h"

namespace WebCore {
	class ResourceError;
}

@interface WkError (Private)

+ (WkError *)errorWithResourceError:(const WebCore::ResourceError &)error;
+ (WkError *)errorWithURL:(OBURL *)url errorType:(WkErrorType)type code:(int)code;

@end
