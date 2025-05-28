#undef __OBJC__
#import "WebKit.h"
#import "BackForwardClient.h"
#define __OBJC__

#import "WkHistory.h"
#import <ob/OBArrayMutable.h>

@interface WkBackForwardListItemPrivate : WkBackForwardListItem
{
	OBString *_title;
	OBURL    *_url;
	OBURL    *_initialURL;
	WTF::RefPtr<WebCore::HistoryItem> _item;
}
- (WebCore::HistoryItem &)item;
@end

@interface WkBackForwardListPrivate : WkBackForwardList
{
	WTF::RefPtr<WebKit::BackForwardClientMorphOS> _client;
}

+ (id)backForwardListPrivate:(WTF::RefPtr<WebKit::BackForwardClientMorphOS>)bf;
- (WTF::RefPtr<WebKit::BackForwardClientMorphOS>)client;

@end
