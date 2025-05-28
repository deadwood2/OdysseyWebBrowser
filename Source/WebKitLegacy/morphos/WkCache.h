#import <ob/OBURL.h>

@interface WkCache

+ (void)clearCookiesForHost:(OBString *)host;
+ (void)clearAllCookies;

+ (void)clearHttpCacheForHost:(OBString *)host;

@end
