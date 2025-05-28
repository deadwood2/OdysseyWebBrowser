#import "WkHistory_private.h"
#import <ob/OBFramework.h>

#undef __OBJC__
#define __MORPHOS_DISABLE
#import "BackForwardClient.h"
#import <WebCore/HistoryItem.h>
#import <wtf/URL.h>
#define __OBJC__

@implementation WkBackForwardListItemPrivate

- (id)initWithItem:(WTF::RefPtr<WebCore::HistoryItem>)item
{
	if ((self = [super init]))
	{
		auto uurl = item->url().string().utf8();
		auto uoriginal = item->originalURL().string().utf8();
		auto utitle = item->title().utf8();

		_url = [[OBURL URLWithString:[OBString stringWithUTF8String:uurl.data()]] retain];
		_initialURL = [[OBURL URLWithString:[OBString stringWithUTF8String:uoriginal.data()]] retain];
		_title = [[OBString stringWithUTF8String:utitle.data()] retain];
		_item = item;
	}
	
	return self;
}

+ (WkBackForwardListItemPrivate *)itemWithListItem:(WTF::RefPtr<WebCore::HistoryItem>)item
{
	return [[[self alloc] initWithItem:item] autorelease];
}

- (void)dealloc
{
	[_url release];
	[_initialURL release];
	[_title release];
	[super dealloc];
}

- (OBURL *)URL
{
	return _url;
}

- (OBURL *)initialURL
{
	return _initialURL;
}

- (OBString *)title
{
	return _title;
}

- (WebCore::HistoryItem &)item
{
	return *_item.get();
}

@end

@implementation WkBackForwardListItem

- (OBURL *)URL
{
	return nil;
}

- (OBURL *)initialURL
{
	return nil;
}

- (OBString *)title
{
	return nil;
}

- (BOOL)isEqual:(id)otherObject
{
	if ([otherObject isKindOfClass:[WkBackForwardListItem class]])
	{
		return [[self URL] isEqual:[otherObject URL]] && [[self initialURL] isEqual:[otherObject initialURL]] && [[self title] isEqual:[otherObject title]];
	}
	
	return NO;
}

@end

@implementation WkBackForwardListPrivate

- (id)initWithClient:(WTF::RefPtr<WebKit::BackForwardClientMorphOS>)bf
{
	if ((self = [super init]))
	{
		_client = bf;
	}
	
	return self;
}

+ (id)backForwardListPrivate:(WTF::RefPtr<WebKit::BackForwardClientMorphOS>)bf
{
	return [[[self alloc] initWithClient:bf] autorelease];
}

- (WTF::RefPtr<WebKit::BackForwardClientMorphOS>)client
{
	return _client;
}

- (WkBackForwardListItem *)backItem
{
	if (_client && _client->backItem())
		return [WkBackForwardListItemPrivate itemWithListItem:_client->backItem()];
	return nil;
}

- (WkBackForwardListItem *)forwardItem
{
	if (_client && _client->forwardItem())
		return [WkBackForwardListItemPrivate itemWithListItem:_client->forwardItem()];
	return nil;
}

- (WkBackForwardListItem *)currentItem
{
	if (_client && _client->currentItem())
		return [WkBackForwardListItemPrivate itemWithListItem:_client->currentItem()];
	return nil;
}

- (OBArray *)backList
{
	if (_client)
	{
		HistoryItemVector history;
		_client->forwardListWithLimit(32, history);
		OBMutableArray *out = [OBMutableArray arrayWithCapacity:history.size()];
		for (auto it = history.begin(); it != history.end(); it++)
		{
			[out addObject:[WkBackForwardListItemPrivate itemWithListItem: WTF::RefPtr<WebCore::HistoryItem>(&it->get())]];
		}
		return out;
	}
	return nil;
}

- (OBArray *)forwardList
{
	if (_client)
	{
		HistoryItemVector history;
		_client->backListWithLimit(32, history);
		OBMutableArray *out = [OBMutableArray arrayWithCapacity:history.size()];
		for (auto it = history.begin(); it != history.end(); it++)
		{
			[out addObject:[WkBackForwardListItemPrivate itemWithListItem: WTF::RefPtr<WebCore::HistoryItem>(&it->get())]];
		}
		return out;
	}
	return nil;
}

@end

@implementation WkBackForwardList

- (WkBackForwardListItem *)backItem
{
	return 0;
}

- (WkBackForwardListItem *)forwardItem
{
	return 0;
}

- (WkBackForwardListItem *)currentItem
{
	return 0;
}

- (OBArray __wkListType *)backList
{
	return 0;
}

- (OBArray __wkListType *)forwardList
{
	return 0;
}

@end
