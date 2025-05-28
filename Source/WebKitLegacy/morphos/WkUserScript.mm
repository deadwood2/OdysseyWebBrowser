#undef __OBJC__
#import "WebKit.h"
#import "WebPageGroup.h"
#import <WebCore/UserScript.h>
#import <WebCore/UserScriptTypes.h>
#import <WebCore/UserContentController.h>
#import <WebCore/UserContentTypes.h>
#define __OBJC__
#import "WkUserScript_private.h"
#import <ob/OBFramework.h>

@interface WkUserScriptPrivate : WkUserScript
{
	WkUserScript_InjectPosition  _injectPosition;
	WkUserScript_InjectInFrames  _injectInFrames;
	OBString                    *_path;
	OBString                    *_content;
	OBArray                     *_whiteList;
	OBArray                     *_blackList;
}
@end

@implementation WkUserScriptPrivate

- (id)initWithContents:(OBString *)script withFile:(OBString *)path injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist
{
	if ((self = [super init]))
	{
		_content = [script copy];
		_path = [path copy];
		_injectPosition = position;
		_injectInFrames = inFrames;
		_whiteList = white;
		_blackList = blacklist;
	}
	
	return self;
}

- (void)dealloc
{
	[_path release];
	[_content release];
	[_whiteList release];
	[_blackList release];
	[super dealloc];
}

- (WkUserScript_InjectPosition)injectPosition
{
	return _injectPosition;
}

- (WkUserScript_InjectInFrames)injectInFrames
{
	return _injectInFrames;
}

- (OBString *)path
{
	return _path;
}

- (OBString *)script
{
	if (nil == _content && _path)
	{
		OBData *data = [OBData dataWithContentsOfFile:_path];
		return [OBString stringFromData:data encoding:MIBENUM_UTF_8];
	}

	return _content;
}

- (OBArray * /* OBString */)whiteList
{
	return _whiteList;
}

- (OBArray * /* OBString */)blackList
{
	return _blackList;
}

@end

@implementation WkUserScript

+ (WkUserScript *)userScriptWithContents:(OBString *)script injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist
{
	return [[[WkUserScriptPrivate alloc] initWithContents:script withFile:nil injectPosition:position injectInFrames:inFrames whiteList:white blackList:blacklist] autorelease];
}

+ (WkUserScript *)userScriptWithContentsOfFile:(OBString *)path injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist
{
	return [[[WkUserScriptPrivate alloc] initWithContents:nil withFile:path injectPosition:position injectInFrames:inFrames whiteList:white blackList:blacklist] autorelease];
}

- (WkUserScript_InjectPosition)injectPosition
{
	return WkUserScript_InjectPosition_AtDocumentEnd;
}

- (WkUserScript_InjectInFrames)injectInFrames
{
	return WkUserScript_InjectInFrames_All;
}

- (OBString *)path
{
	return @"";
}

- (OBString *)script
{
	return @"";
}

- (OBArray * /* OBString */)whiteList
{
	return nil;
}

- (OBArray * /* OBString */)blackList
{
	return nil;
}

@end

@implementation WkUserScripts

OBMutableArray *_scripts;

+ (void)initialize
{
	_scripts = [OBMutableArray new];
}

+ (void)loadScript:(WkUserScript *)script
{
	OBString *scriptContents = [script script];
	if (scriptContents)
	{
		auto group = WebKit::WebPageGroup::getOrCreate("meh", "PROGDIR:Cache/Storage");
		WTF::Vector<WTF::String> white, black;

        OBEnumerator *e = [[script whiteList] objectEnumerator];
        OBString *url;

        while ((url = [e nextObject]))
        {
            white.append(WTF::String::fromUTF8([url cString]));
        }
        
        e = [[script blackList] objectEnumerator];
        while ((url = [e nextObject]))
        {
            black.append(WTF::String::fromUTF8([url cString]));
        }
  
		group->userContentController().addUserScript(*group->wrapperWorldForUserScripts(),
			makeUnique<WebCore::UserScript>(WTF::String::fromUTF8([scriptContents cString]),
				WTF::URL(WTF::URL(), WTF::String([[OBString stringWithFormat:@"file:///script_%08lx", script] cString])),
				WTFMove(white), WTFMove(black),
				(WkUserScript_InjectPosition_AtDocumentStart == [script injectPosition] ? WebCore::UserScriptInjectionTime::DocumentStart : WebCore::UserScriptInjectionTime::DocumentEnd),
				(WkUserScript_InjectInFrames_All == [script injectInFrames] ? WebCore::UserContentInjectedFrames::InjectInAllFrames : WebCore::UserContentInjectedFrames::InjectInTopFrameOnly), WebCore::WaitForNotificationBeforeInjecting::No));
	}
}

+ (void)addUserScript:(WkUserScript *)script
{
	if (OBNotFound == [_scripts indexOfObject:script])
	{
		[_scripts addObject:script];
		[self loadScript:script];
	}
}

+ (void)removeUserScript:(WkUserScript *)script
{
	[_scripts removeObject:script];
	auto group = WebKit::WebPageGroup::getOrCreate("meh", "PROGDIR:Cache/Storage");
	group->userContentController().removeUserScript(*group->wrapperWorldForUserScripts(),
		WTF::URL(WTF::URL(), WTF::String([[OBString stringWithFormat:@"file:///script_%08lx", script] cString])));
}

+ (OBArray *)userScripts
{
	return [[_scripts copy] autorelease];
}

+ (void)shutdown
{
	[_scripts release];
}

@end
