#import "WkCache.h"
#import <ob/OBString.h>
#define __MORPHOS_DISABLE
#undef __OBJC__
#import "WebKit.h"
#import <WebCore/CurlCacheManager.h>
#import <WebCore/CookieJarDB.h>
#import <WebCore/NetworkStorageSession.h>
#import "../WebCoreSupport/NetworkStorageSessionMap.h"
#define __OBJC__

using namespace WebCore;
using namespace WTF;

@implementation WkCache

+ (void)clearCookiesForHost:(OBString *)host
{
	CookieJarDB& db = NetworkStorageSessionMap::defaultStorageSession().cookieDatabase();
	String sHost = String::fromUTF8([host cString]);
	db.deleteCookiesForHostname(sHost, IncludeHttpOnlyCookies::Yes);
}

+ (void)clearAllCookies
{
	CookieJarDB& db = NetworkStorageSessionMap::defaultStorageSession().cookieDatabase();
	db.deleteAllCookies();
}

+ (void)clearHttpCacheForHost:(OBString *)host
{
	String sHost = String::fromUTF8([host cString]);
	CurlCacheManager::singleton().removeEntriesMatchingHost(sHost);
}

@end
