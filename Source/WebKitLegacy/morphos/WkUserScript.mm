#undef __OBJC__
#import "WebKit.h"
#import "WebPageGroup.h"
#import <WebCore/UserScript.h>
#import <WebCore/UserScriptTypes.h>
#import <WebCore/UserStyleSheet.h>
#import <WebCore/UserStyleSheetTypes.h>
#import <WebCore/UserContentController.h>
#import <WebCore/UserContentTypes.h>
#define __OBJC__
#import "WkUserScript_private.h"
#import <ob/OBFramework.h>

#define D(x) 

@interface WkUserScriptPrivate : WkUserScript
{
	WkUserScript_InjectPosition  _injectPosition;
	WkUserScript_InjectInFrames  _injectInFrames;
	OBString                    *_path;
	OBString                    *_content;
    OBString                    *_cssPath;
    OBString                    *_cssContent;
	OBArray                     *_whiteList;
	OBArray                     *_blackList;
}
@end

@implementation WkUserScriptPrivate

- (id)initWithContents:(OBString *)script withFile:(OBString *)path
    cssContents:(OBString *)css withCSSFile:(OBString *)cssPath injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist
{
	if ((self = [super init]))
	{
        D(dprintf("%s: path %s %s\n", __PRETTY_FUNCTION__, [path cString], [cssPath cString]));
		_content = [script copy];
		_path = [path copy];
        _cssPath = [cssPath copy];
        _cssContent = [css copy];
		_injectPosition = position;
		_injectInFrames = inFrames;
		_whiteList = [white retain];
		_blackList = [blacklist retain];
	}
	
	return self;
}

- (void)dealloc
{
	[_path release];
    [_cssPath release];
	[_content release];
    [_cssContent release];
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

- (OBString *)cssPath
{
    return _cssPath;
}

- (OBString *)css
{
	if (nil == _cssContent && _cssPath)
	{
		OBData *data = [OBData dataWithContentsOfFile:_cssPath];
		return [OBString stringFromData:data encoding:MIBENUM_UTF_8];
	}

	return _cssContent;

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
	return [[[WkUserScriptPrivate alloc] initWithContents:script withFile:nil cssContents:nil withCSSFile:nil injectPosition:position injectInFrames:inFrames whiteList:white blackList:blacklist] autorelease];
}

+ (WkUserScript *)userScriptWithContents:(OBString *)script css:(OBString *)css injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist
{
	return [[[WkUserScriptPrivate alloc] initWithContents:script withFile:nil cssContents:css withCSSFile:nil injectPosition:position injectInFrames:inFrames whiteList:white blackList:blacklist] autorelease];
}

+ (WkUserScript *)userScriptWithContentsOfFile:(OBString *)path injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist
{
	return [[[WkUserScriptPrivate alloc] initWithContents:nil withFile:path cssContents:nil withCSSFile:nil injectPosition:position injectInFrames:inFrames whiteList:white blackList:blacklist] autorelease];
}

+ (WkUserScript *)userScriptWithContentsOfFile:(OBString *)path cssFile:(OBString *)css injectPosition:(WkUserScript_InjectPosition)position injectInFrames:(WkUserScript_InjectInFrames)inFrames whiteList:(OBArray *)white blackList:(OBArray *)blacklist
{
	return [[[WkUserScriptPrivate alloc] initWithContents:nil withFile:path cssContents:nil withCSSFile:css injectPosition:position injectInFrames:inFrames whiteList:white blackList:blacklist] autorelease];
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

- (OBString *)cssPath
{
    return nil;
}

- (OBString *)css
{
    return nil;
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
    OBString *cssContents = [script css];

	if ([scriptContents length] || [cssContents length])
	{
		auto group = WebKit::WebPageGroup::getOrCreate("meh"_s, "PROGDIR:Cache/WebStorage"_s);
		WTF::Vector<WTF::String> white, black;
		WTF::Vector<WTF::String> whiteCSS, blackCSS;

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

        if ([cssContents length])
        {
            whiteCSS = white;
            blackCSS = black;
        }

        if ([scriptContents length])
        {
            group->userContentController().addUserScript(*group->wrapperWorldForUserScripts(),
                makeUnique<WebCore::UserScript>(WTF::String::fromUTF8([scriptContents cString]),
                    WTF::URL(WTF::URL(), WTF::String::fromUTF8([[OBString stringWithFormat:@"file:///script_%08lx", script] cString])),
                    WTFMove(white), WTFMove(black),
                    (WkUserScript_InjectPosition_AtDocumentStart == [script injectPosition] ? WebCore::UserScriptInjectionTime::DocumentStart : WebCore::UserScriptInjectionTime::DocumentEnd),
                    (WkUserScript_InjectInFrames_All == [script injectInFrames] ? WebCore::UserContentInjectedFrames::InjectInAllFrames : WebCore::UserContentInjectedFrames::InjectInTopFrameOnly), WebCore::WaitForNotificationBeforeInjecting::No));
        }
        
        if ([cssContents length])
        {
            D(dprintf("%s: adding css, %d/%d\n", __PRETTY_FUNCTION__, whiteCSS.size(), blackCSS.size()));
            group->userContentController().addUserStyleSheet(*group->wrapperWorldForUserScripts(),
                makeUnique<WebCore::UserStyleSheet>(WTF::String::fromUTF8([cssContents cString]),
                WTF::URL(WTF::URL(), WTF::String::fromUTF8([[OBString stringWithFormat:@"file:///css_%08lx", script] cString])),
                WTFMove(whiteCSS), WTFMove(blackCSS),
                (WkUserScript_InjectInFrames_All == [script injectInFrames] ? WebCore::UserContentInjectedFrames::InjectInAllFrames : WebCore::UserContentInjectedFrames::InjectInTopFrameOnly),
                WebCore::UserStyleLevel::User), WebCore::UserStyleInjectionTime::InjectInExistingDocuments);
        }
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
    [script retain];
	[_scripts removeObject:script];
	auto group = WebKit::WebPageGroup::getOrCreate("meh"_s, "PROGDIR:Cache/WebStorage"_s);
	group->userContentController().removeUserScript(*group->wrapperWorldForUserScripts(),
		WTF::URL(WTF::URL(), WTF::String::fromUTF8([[OBString stringWithFormat:@"file:///script_%08lx", script] cString])));
	group->userContentController().removeUserStyleSheet(*group->wrapperWorldForUserScripts(),
		WTF::URL(WTF::URL(), WTF::String::fromUTF8([[OBString stringWithFormat:@"file:///css_%08lx", script] cString])));
    [script release];
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
