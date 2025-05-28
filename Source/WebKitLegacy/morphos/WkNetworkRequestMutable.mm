#import "WkNetworkRequestMutable_private.h"
#import "WkError_private.h"
#import "WkWebView.h"
#import <ob/OBFramework.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"

@implementation WkMutableNetworkRequestPrivate

- (id)initWithURL:(OBURL *)url cachePolicy:(WkMutableNetworkRequestCachePolicy)cachePolicy timeoutInterval:(float)timeout
{
	if ((self = [super init]))
	{
		_url = [url copy];
		_cachePolicy = cachePolicy;
		_timeout = timeout;
		_httpMethod = @"GET";
	}
	
	return self;
}

- (id)initWithURL:(OBURL *)url
{
	return [self initWithURL:url cachePolicy:WkMutableNetworkRequestUseProtocolCachePolicy timeoutInterval:60.0];
}

- (void)dealloc
{
	[_url release];
	[_mainDocumentURL release];
	[_httpMethod release];
	[_clientCertificate release];
	[_httpBody release];
	[_headers release];
	[_context release];
	[super dealloc];
}

- (id)mutableCopy
{
	WkMutableNetworkRequest *copy = [[WkMutableNetworkRequestPrivate alloc] initWithURL:_url cachePolicy:_cachePolicy timeoutInterval:_timeout];

	if (copy)
	{
		[copy setHTTPMethod:_httpMethod];
		[copy setHTTPBody:_httpBody];
		[copy setAllHTTPHeaderFields:_headers];
		[copy setMainDocumentURL:_mainDocumentURL];
		[copy setHTTPShouldHandleCookies:_shouldHandleCoookies];
		[copy setAllowsAnyHTTPSClientCertificate:_allowsAnyClientCertificate];
		[copy setClientCertificate:_clientCertificate];
		[copy setContext:_context];
	}
	
	return copy;
}

- (OBURL *)URL
{
	return _url;
}

- (void)setURL:(OBURL *)value
{
	[_url autorelease];
	_url = [value copy];
}

- (WkMutableNetworkRequestCachePolicy)cachePolicy
{
	return _cachePolicy;
}

- (void)setCachePolicy:(WkMutableNetworkRequestCachePolicy)cachePolicy
{
	_cachePolicy = cachePolicy;
}

- (float)timeoutInterval
{
	return _timeout;
}

- (void)setTimeoutInterval:(float)interval
{
	_timeout = interval;
}

- (OBString *)HTTPMethod
{
	return _httpMethod;
}

- (void)setHTTPMethod:(OBString *)value
{
	[_httpMethod autorelease];
	_httpMethod = [value copy];
}

- (OBData *)HTTPBody
{
	return _httpBody;
}

- (void)setHTTPBody:(OBData *)body
{
	[_httpBody autorelease];
	_httpBody = [body copy];
}

- (OBDictionary *)allHTTPHeaderFields
{
	return _headers;
}

- (OBString *)valueForHTTPHeaderField:(OBString *)field
{
	return [_headers objectForKey:field];
}

- (void)setAllHTTPHeaderFields:(OBDictionary *)allValues
{
	[_headers autorelease];
	_headers = [allValues copy];
}

- (void)setValue:(OBString *)value forHTTPHeaderField:(OBString *)field
{
	if (nil == _headers)
		_headers = [OBMutableDictionary new];
	[_headers setObject:value forKey:field];
}

- (OBURL *)mainDocumentURL
{
	return _mainDocumentURL;
}

- (void)setMainDocumentURL:(OBURL *)url
{
	[_mainDocumentURL autorelease];
	_mainDocumentURL = [url copy];
}

- (BOOL)HTTPShouldHandleCookies
{
	return _shouldHandleCoookies;
}

- (void)setHTTPShouldHandleCookies:(BOOL)handleCookies
{
	_shouldHandleCoookies = handleCookies;
}

- (BOOL)allowsAnyHTTPSClientCertificate
{
	return _allowsAnyClientCertificate;
}

- (void)setAllowsAnyHTTPSClientCertificate:(BOOL)allowsany
{
	_allowsAnyClientCertificate = allowsany;
}

- (OBString *)clientCertificate
{
	return _clientCertificate;
}

- (void)setClientCertificate:(OBString *)certificate
{
	[_clientCertificate autorelease];
	_clientCertificate = [certificate copy];
}

- (id)context
{
	return _context;
}

- (void)setContext:(id)userData
{
	[_context autorelease];
	_context = [userData retain];
}

@end

WebCore::ResourceRequest WkMutableNetworkRequestPrivateTranslator::fromNetworkRequest(WkMutableNetworkRequestPrivate *request)
{
	WebCore::ResourceRequest out;
	
	out.setURL(WTF::URL(WTF::URL(),	WTF::String::fromUTF8([[[request URL] absoluteString] cString])));

	switch ([request cachePolicy])
	{
	case WkMutableNetworkRequestReloadIgnoringCacheData:
		out.setCachePolicy(WebCore::ResourceRequestCachePolicy::ReloadIgnoringCacheData);
		break;
	case WkMutableNetworkRequestReturnCacheDataDontLoad:
		out.setCachePolicy(WebCore::ResourceRequestCachePolicy::ReturnCacheDataDontLoad);
		break;
	case WkMutableNetworkRequestReturnCacheDataElseLoad:
		out.setCachePolicy(WebCore::ResourceRequestCachePolicy::ReturnCacheDataElseLoad);
		break;
	case WkMutableNetworkRequestUseProtocolCachePolicy: default: break;
	}

	out.setTimeoutInterval([request timeoutInterval]);
	out.setHTTPMethod(WTF::String::fromUTF8([[request HTTPMethod] cString]));

	OBData *body = [request HTTPBody];
	if ([body length])
	{
		out.setHTTPBody(WebCore::FormData::create([body bytes], [body length]));
	}

	OBDictionary *headers = [request allHTTPHeaderFields];
	OBEnumerator *e = [headers keyEnumerator];
	OBString *header;
	while ((header = [e nextObject]))
	{
		OBString *val = [headers objectForKey:header];
		if (val)
		{
			out.setHTTPHeaderField(WTF::String::fromUTF8([header cString]), WTF::String::fromUTF8([val cString]));
		}
	}

	if ([request mainDocumentURL])
		out.setHTTPReferrer(WTF::String::fromUTF8([[[request mainDocumentURL] absoluteString] cString]));

	out.setAllowCookies([request HTTPShouldHandleCookies]);

#if 0
	// NOT YET IMPLEMENTED!

- (BOOL)allowsAnyHTTPSClientCertificate;
- (void)setAllowsAnyHTTPSClientCertificate:(BOOL)allowsany;

- (OBString *)clientCertificate;
- (void)setClientCertificate:(OBString *)certificate;
#endif

	return out;
}

@interface WkMutableNetworkRequestHandlerImpl : OBObject<WkMutableNetworkRequestHandler>
{
	id<WkMutableNetworkRequestTarget> _target;
	id<WkNetworkRequest>              _request;
	void                             *_innerClient;
}

- (void)onError:(WkError *)error withData:(OBData *)data;
- (void)onFinishWithData:(OBData *)data;

@end

class WkMutableNetworkRequestClient final : private WebCore::ResourceHandleClient {
public:
	WkMutableNetworkRequestClient(WkMutableNetworkRequestHandlerImpl *parent, const WebCore::ResourceRequest& request)
		: m_parent([parent retain])
	{
		m_handle = WebCore::ResourceHandle::create(WebKit::WebProcess::singleton().networkingContext().get(), request, this, false, false, true, nullptr, false);
		if (nullptr == m_handle)
		{
			[m_parent onError:[WkError errorWithURL:nil errorType:WkErrorType_Cancellation code:0] withData:nil];
			[m_parent autorelease];
			m_parent = nil;
		}
	}
	~WkMutableNetworkRequestClient() = default;
	void cancel() {
		if (m_handle)
			m_handle->cancel();
		m_handle = nullptr;
	}
private:
    void didReceiveBuffer(WebCore::ResourceHandle*, Ref<WebCore::SharedBuffer>&&buffer, int encodedDataLength) final {
    	if (m_resourceData)
    		m_resourceData->append(buffer);
		else
			m_resourceData = &buffer.get();
	}
	
    void didFinishLoading(WebCore::ResourceHandle*, const WebCore::NetworkLoadMetrics&) final {
    	OBData *resp = nil;
    	if (m_resourceData && m_resourceData->size())
    	{
    		resp = [OBData dataWithBytes:m_resourceData->data() length:m_resourceData->size()];
		}
		m_handle = nullptr;
		[m_parent onFinishWithData:resp];
    	[m_parent autorelease];
    	m_parent = nil;
	}
    void didFail(WebCore::ResourceHandle*, const WebCore::ResourceError&error) final {
    	OBData *resp = nil;
    	if (m_resourceData && m_resourceData->size())
    	{
    		resp = [OBData dataWithBytes:m_resourceData->data() length:m_resourceData->size()];
		}
		m_handle = nullptr;
		[m_parent onError:[WkError errorWithResourceError:error] withData:resp];
    	[m_parent autorelease];
    	m_parent = nil;
	}
    void willSendRequestAsync(WebCore::ResourceHandle*, WebCore::ResourceRequest&& request, WebCore::ResourceResponse&&, CompletionHandler<void(WebCore::ResourceRequest&&)>&& completion) final {
		m_currentRequest = WTFMove(request);
		completion(WebCore::ResourceRequest { m_currentRequest });
	}
    void didReceiveResponseAsync(WebCore::ResourceHandle*, WebCore::ResourceResponse&&, CompletionHandler<void()>&& completion) {
    	completion();
	}
private:
	WkMutableNetworkRequestHandlerImpl *m_parent = nil;
    RefPtr<WebCore::SharedBuffer> m_resourceData;
    RefPtr<WebCore::ResourceHandle> m_handle;
    WebCore::ResourceRequest m_currentRequest;
};

@implementation WkMutableNetworkRequestHandlerImpl

- (id)initRequest:(id<WkNetworkRequest>)request withTarget:(id<WkMutableNetworkRequestTarget>)target
{
	if ((self = [super init]))
	{
		_request = [request copy];
		_target = [target retain];
		_innerClient = (void *)new WkMutableNetworkRequestClient(self, WkMutableNetworkRequestPrivateTranslator::fromNetworkRequest(request));
		if (nullptr == _innerClient)
		{
			[_target request:self didFailWithError:[WkError errorWithURL:nil errorType:WkErrorType_Cancellation code:0] data:0];
			[self release];
			return nil;
		}
	}
	
	return self;
}

- (void)dealloc
{
	delete static_cast<WkMutableNetworkRequestClient *>(_innerClient);
	[_target release];
	[_request release];
	[super dealloc];
}

- (void)cancel
{
	static_cast<WkMutableNetworkRequestClient *>(_innerClient)->cancel();
}

- (void)onError:(WkError *)error withData:(OBData *)data
{
	[_target request:self didFailWithError:error data:data];
	[_target autorelease];
	_target = nil;
}

- (void)onFinishWithData:(OBData *)data
{
	[_target request:self didCompleteWithData:data];
	[_target autorelease];
	_target = nil;
}

- (id<WkNetworkRequest>)request
{
	return _request;
}

- (id<WkMutableNetworkRequestTarget>)target
{
	return _target;
}

@end

@implementation WkMutableNetworkRequest

+ (id<WkMutableNetworkRequestHandler>)performRequest:(id<WkNetworkRequest>)request withTarget:(id<WkMutableNetworkRequestTarget>)target
{
	[WkWebView class]; // force a warm-up via +initialize
	return [[[WkMutableNetworkRequestHandlerImpl alloc] initRequest:request withTarget:target] autorelease];
}

+ (id)requestWithURL:(OBURL *)url
{
	[WkWebView class]; // force a warm-up via +initialize
	return [[[WkMutableNetworkRequestPrivate alloc] initWithURL:url] autorelease];
}

+ (id)requestWithURL:(OBURL *)url cachePolicy:(WkMutableNetworkRequestCachePolicy)cachePolicy timeoutInterval:(float)timeout
{
	[WkWebView class]; // force a warm-up via +initialize
	return [[[WkMutableNetworkRequestPrivate alloc] initWithURL:url cachePolicy:cachePolicy timeoutInterval:timeout] autorelease];
}

- (id)copy
{
	return [self retain];
}

- (id)mutableCopy
{
	return nil;
}

- (OBURL *)URL
{
	return nil;
}

- (void)setURL:(OBURL *)value
{
}

- (WkMutableNetworkRequestCachePolicy)cachePolicy
{
	return WkMutableNetworkRequestCachePolicy::WkMutableNetworkRequestUseProtocolCachePolicy;
}

- (void)setCachePolicy:(WkMutableNetworkRequestCachePolicy)cachePolicy
{
}

- (float)timeoutInterval
{
	return 60.0f;
}

- (void)setTimeoutInterval:(float)interval
{
}

- (OBString *)HTTPMethod
{
	return nil;
}

- (void)setHTTPMethod:(OBString *)value
{
}

- (OBData *)HTTPBody
{
	return nil;
}

- (void)setHTTPBody:(OBData *)body
{
}

- (OBDictionary *)allHTTPHeaderFields
{
	return nil;
}

- (OBString *)valueForHTTPHeaderField:(OBString *)field
{
	return nil;
}

- (void)setAllHTTPHeaderFields:(OBDictionary *)allValues
{
}

- (void)setValue:(OBString *)value forHTTPHeaderField:(OBString *)field
{
}

- (OBURL *)mainDocumentURL
{
	return nil;
}

- (void)setMainDocumentURL:(OBURL *)url
{
}

- (BOOL)HTTPShouldHandleCookies
{
	return NO;
}

- (void)setHTTPShouldHandleCookies:(BOOL)handleCookies
{
}

- (BOOL)allowsAnyHTTPSClientCertificate
{
	return NO;
}

- (void)setAllowsAnyHTTPSClientCertificate:(BOOL)allowsany
{
}

- (OBString *)clientCertificate
{
	return nil;
}

- (void)setClientCertificate:(OBString *)certificate
{
}

- (id)context
{
	return nil;
}

- (void)setContext:(id)userData
{

}

@end
