#import <ob/OBString.h>

@class OBURL, OBDictionary;

@interface WkResourceResponse : OBObject

- (OBURL *)url;

- (QUAD)expectedContentLength;

- (OBString *)textEncodingName;

- (OBString *)mimeType;

- (ULONG)httpStatusCode;

- (OBString *)httpStatusText;

- (OBDictionary *)headers;

- (BOOL)requestWithHttpAuth;

@end
