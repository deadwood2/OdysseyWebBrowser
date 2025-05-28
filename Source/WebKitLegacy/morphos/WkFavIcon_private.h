#import "WkFavIcon.h"
#undef __OBJC__
#import "WebKit.h"
#define __OBJC__

namespace WebCore {
	class SharedBuffer;
};

struct BitMap;
@class OBString, OBScheduledTimer;

@interface WkFavIconPrivate : WkFavIcon
{
	WTF::String _host;
	OBScheduledTimer *_loadResizeTimer;
	UBYTE      *_data;
	UBYTE      *_dataPrescaled;
	LONG        _width;
	LONG        _height;
	LONG        _widthPrescaled;
	LONG        _heightPrescaled;
	BOOL        _useAlpha;
}

+ (WkFavIconPrivate *)cacheIconWithData:(WebCore::SharedBuffer *)data forHost:(OBString *)host;

@end
