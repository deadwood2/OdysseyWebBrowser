#import "WkResourceResponse_private.h"
#undef __OBJC__
#import <wtf/FastMalloc.h>
#import <WebCore/ResourceResponse.h>
#define __OBJC__

#import <ob/OBURL.h>
#import <ob/OBDictionaryMutable.h>

@implementation WkResourceResponsePrivate

- (id)initWithResponse:(const WebCore::ResourceResponse &)response auth:(BOOL)auth
{
	if ((self = [super init]))
	{
		auto uurl = response.url().string().utf8();
		_url = [[OBURL URLWithString:[OBString stringWithUTF8String:uurl.data()]] retain];
		auto umime = response.mimeType().utf8();
		_mimeType = [[OBString stringWithUTF8String:umime.data()] retain];
		auto ustatus = response.httpStatusText().utf8();
		_httpStatusText = [[OBString stringWithUTF8String:ustatus.data()] retain];
		auto uencoding = response.textEncodingName().utf8();
		_textEncodingName = [[OBString stringWithUTF8String:uencoding.data()] retain];
		_httpStatusCode = response.httpStatusCode();
		_expectedContentLength = QUAD(response.expectedContentLength());
		OBMutableDictionary *headers = [OBMutableDictionary dictionaryWithCapacity:32];
        WebCore::HTTPHeaderMap::const_iterator end = response.httpHeaderFields().end();
        for (WebCore::HTTPHeaderMap::const_iterator it = response.httpHeaderFields().begin(); it != end; ++it) {
			auto ukey = it->key.utf8();
			auto udata = it->value.utf8();
			[headers setObject:[OBString stringWithUTF8String:udata.data()] forKey:[OBString stringWithUTF8String:ukey.data()]];
		}
		_httpHeaders = [headers retain];
		_hadAuth = auth;
	}

	return self;
}

- (void)dealloc
{
	[_url release];
	[_mimeType release];
	[_httpStatusText release];
	[_httpHeaders release];
	[_textEncodingName release];
	[super dealloc];
}

+ (WkResourceResponsePrivate *)responseWithResponse:(const WebCore::ResourceResponse &)response auth:(BOOL)auth
{
	return [[[self alloc] initWithResponse:response auth:auth] autorelease];
}

- (OBURL *)url
{
	return _url;
}

- (QUAD)expectedContentLength
{
	return _expectedContentLength;
}

- (OBString *)textEncodingName
{
	return _textEncodingName;
}

- (OBString *)mimeType
{
	return _mimeType;
}

- (ULONG)httpStatusCode
{
	return _httpStatusCode;
}

- (OBString *)httpStatusText
{
	return _httpStatusText;
}

- (OBDictionary *)headers
{
	return _httpHeaders;
}

- (BOOL)requestWithHttpAuth
{
	return _hadAuth;
}

@end

@implementation WkResourceResponse

- (OBURL *)url
{
	return nil;
}

- (QUAD)expectedContentLength
{
	return 0;
}

- (OBString *)textEncodingName
{
	return nil;
}

- (OBString *)mimeType
{
	return nil;
}

- (ULONG)httpStatusCode
{
	return 0;
}

- (OBString *)httpStatusText
{
	return nil;
}

- (OBDictionary *)headers
{
	return nil;
}

- (BOOL)requestWithHttpAuth
{
	return NO;
}

@end
