#undef __OBJC__
#import "WebKit.h"
#import "WebProcess.h"
#import <WebCore/ResourceRequest.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/ResourceHandleClient.h>
#import <WebCore/SharedBuffer.h>
#import <wtf/URL.h>
#define __OBJC__
#import "WkNetworkRequestMutable.h"

@interface WkMutableNetworkRequestPrivate : WkMutableNetworkRequest
{
	OBURL *_url;
	OBURL *_mainDocumentURL;
	WkMutableNetworkRequestCachePolicy _cachePolicy;
	float _timeout;
	OBString *_httpMethod;
	OBString *_clientCertificate;
	OBData *_httpBody;
	OBMutableDictionary *_headers;
	BOOL _shouldHandleCoookies;
	BOOL _allowsAnyClientCertificate;
	id _context;
}
@end

struct WkMutableNetworkRequestPrivateTranslator
{
	static WebCore::ResourceRequest fromNetworkRequest(WkMutableNetworkRequestPrivate *request);
};
