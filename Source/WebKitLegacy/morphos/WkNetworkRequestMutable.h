#import <ob/OBString.h>
#import <ob/OBCopying.h>

@class OBURL, OBDictionary, OBMutableDictionary, OBData, WkError;
@protocol WkMutableNetworkRequestHandler;

typedef enum {
   WkMutableNetworkRequestUseProtocolCachePolicy,
   WkMutableNetworkRequestReloadIgnoringCacheData,
   WkMutableNetworkRequestReturnCacheDataElseLoad,
   WkMutableNetworkRequestReturnCacheDataDontLoad
} WkMutableNetworkRequestCachePolicy;

@protocol WkNetworkRequest <OBObject, OBCopying, OBMutableCopying>
- (OBURL *)URL;
- (WkMutableNetworkRequestCachePolicy)cachePolicy;
- (float)timeoutInterval;
- (OBString *)HTTPMethod;
- (OBData *)HTTPBody;
- (OBDictionary *)allHTTPHeaderFields;
- (OBString *)valueForHTTPHeaderField:(OBString *)field;
- (OBURL *)mainDocumentURL;
- (BOOL)HTTPShouldHandleCookies;
- (BOOL)allowsAnyHTTPSClientCertificate;
- (OBString *)clientCertificate;
- (id)context;
@end

@protocol WkMutableNetworkRequestTarget <OBObject>

- (void)request:(id<WkMutableNetworkRequestHandler>)handler didCompleteWithData:(OBData *)data;
- (void)request:(id<WkMutableNetworkRequestHandler>)handler didFailWithError:(WkError *)error data:(OBData *)data;

@end

@protocol WkMutableNetworkRequestHandler <OBObject>

- (id<WkNetworkRequest>)request;
- (id<WkMutableNetworkRequestTarget>)target;
- (void)cancel;

@end

// Wraps WebKits network requests and allows their customization
@interface WkMutableNetworkRequest : OBObject<WkNetworkRequest>

+ (id<WkMutableNetworkRequestHandler>)performRequest:(id<WkNetworkRequest>)request withTarget:(id<WkMutableNetworkRequestTarget>)target;

+ (id)requestWithURL:(OBURL *)url;
+ (id)requestWithURL:(OBURL *)url cachePolicy:(WkMutableNetworkRequestCachePolicy)cachePolicy timeoutInterval:(float)timeout;

- (void)setURL:(OBURL *)value;
- (void)setCachePolicy:(WkMutableNetworkRequestCachePolicy)cachePolicy;
- (void)setTimeoutInterval:(float)interval;
- (void)setHTTPMethod:(OBString *)value;
- (void)setHTTPBody:(OBData *)body;

- (void)setAllHTTPHeaderFields:(OBDictionary *)allValues;
- (void)setValue:(OBString *)value forHTTPHeaderField:(OBString *)field;

- (void)setMainDocumentURL:(OBURL *)url;
- (void)setHTTPShouldHandleCookies:(BOOL)handleCookies;
- (void)setAllowsAnyHTTPSClientCertificate:(BOOL)allowsany;
- (void)setClientCertificate:(OBString *)certificate;

// Any object that may be used to identify the request by the caller
- (void)setContext:(id)userData;

@end
