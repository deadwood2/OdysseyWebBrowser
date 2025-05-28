#undef __OBJC__
#import "WebKit.h"
#import <WebCore/HitTestResult.h>
#import "WebPage.h"
#import <wtf/URL.h>
#define __OBJC__

#import "WkHitTest.h"

@interface WkHitTestPrivate : WkHitTest
{
	WTF::RefPtr<WebKit::WebPage> _page;
	WebCore::HitTestResult      *_hitTest;
}

+ (id)hitTestFromHitTestResult:(const WebCore::HitTestResult &)hittest onWebPage:(const WTF::RefPtr<WebKit::WebPage> &)webPage;
- (WebCore::HitTestResult *)hitTestInternal;

@end

