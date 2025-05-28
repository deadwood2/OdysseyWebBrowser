#undef __OBJC__
#import "WebKit.h"
#import <WebCore/IntRect.h>
#import <WebCore/FloatRect.h>
#import <WebCore/PrintContext.h>
#define __OBJC__

#import "WkPrinting.h"
#import <libraries/ppd.h>

@class WkWebView, OBArray, WkPrintingStatePrivate;

namespace WebCore {
	class Frame;
	class PrintContext;
}

@interface WkPrintingProfile (Internal)

+ (OBArray /* OBString */ *)allProfiles;
+ (OBString *)defaultProfile;

@end

@interface WkPrintingProfilePrivate : WkPrintingProfile
{
	WkPrintingStatePrivate *_state; // WEAK
	Library                *_ppdBase;
	OBString               *_profile;
	PPD                    *_ppd;
	WkPrintingPage         *_page;
}

- (id)initWithProfile:(OBString *)profile state:(WkPrintingState *)state;
- (void)clearState;

@end

@interface WkPrintingPagePrivate : WkPrintingPage
{
	OBString *_name;
	OBString *_key;
	float     _width;
	float     _height;
	float     _marginLeft;
	float     _marginRight;
	float     _marginTop;
	float     _marginBottom;
}

+ (WkPrintingPagePrivate *)pageWithName:(OBString *)name key:(OBString *)key width:(float)width height:(float)height
	marginLeft:(float)mleft marginRight:(float)mright marginTop:(float)mtop marginBottom:(float)mbottom;

@end

@interface WkPrintingStatePrivate : WkPrintingState
{
	WkWebView             *_webView;
	WebCore::PrintContext *_context;
	WkPrintingProfile     *_profile;
	OBMutableArray        *_profiles;
	float                  _marginLeft;
	float                  _marginRight;
	float                  _marginTop;
	float                  _marginBottom;
	float                  _scale;
	bool                   _defaultMargins;
	bool                   _landscape;
	bool                   _printBackgrounds;
	LONG                   _previewedSheet;
	LONG                   _pagesPerSheet;
	LONG                   _copies;
	WkPrintingRange       *_range;
	WkPrintingState_Parity _parity;
}

- (id)initWithWebView:(WkWebView *)view frame:(WebCore::Frame *)frame;
- (void)invalidate;

- (WebCore::PrintContext *)context;

- (void)recalculatePages;

- (WebCore::FloatBoxExtent)printMargins;

- (WkPrintingPage *)pageWithMarginsApplied;

@end
