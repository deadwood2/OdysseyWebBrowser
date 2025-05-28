#undef __OBJC__
#import "WebKit.h"
#import "WebPage.h"
#import <WebCore/Image.h>
#import "WebEditorClient.h"
#define __OBJC__

#import "WkHitTest_private.h"
#import <ob/OBURL.h>
#import <ob/OBArrayMutable.h>

@implementation WkHitTestPrivate

- (id)initFromHitTestResult:(const WebCore::HitTestResult &)hittest onWebPage:(const WTF::RefPtr<WebKit::WebPage> &)webPage
{
	if ((self = [super init]))
	{
		_hitTest = new WebCore::HitTestResult(hittest);
		_page = webPage;
		if (nullptr == _hitTest)
		{
			[self release];
			return nil;
		}
	}
	
	return self;
}

- (void)dealloc
{
	delete _hitTest;
	[super dealloc];
}

+ (id)hitTestFromHitTestResult:(const WebCore::HitTestResult &)hittest onWebPage:(const WTF::RefPtr<WebKit::WebPage> &)webPage
{
	return [[[self alloc] initFromHitTestResult:hittest onWebPage:webPage] autorelease];
}

- (WebCore::HitTestResult *)hitTestInternal
{
	return _hitTest;
}

- (OBString *)stringFromWTFString:(const WTF::String &)string
{
	if (string.length())
	{
		auto udata = string.utf8();
		return [OBString stringWithUTF8String:udata.data()];
	}
	return nil;
}

- (OBString *)selectedText
{
	return [self stringFromWTFString:_hitTest->selectedText()];
}

- (OBString *)title
{
	WebCore::TextDirection td(WebCore::TextDirection::LTR);
	return [self stringFromWTFString:_hitTest->title(td)];
}

- (OBString *)altText
{
	return [self stringFromWTFString:_hitTest->altDisplayString()];
}

- (OBString *)textContent
{
	return [self stringFromWTFString:_hitTest->textContent()];
}

- (BOOL)isContentEditable
{
	return _hitTest->isContentEditable();
}

- (OBString *)misspelledWord
{
	auto miss = _page->misspelledWord(*_hitTest);
	if (miss.length())
	{
		auto umiss = miss.utf8();
		return [OBString stringWithUTF8String:umiss.data()];
	}
	return nil;
}

- (OBArray *)guessesForMisspelledWord
{
	OBMutableArray *out = [OBMutableArray arrayWithCapacity:16];
	auto list = _page->misspelledWordSuggestions(*_hitTest);
	for (WTF::String &s : list)
	{
		auto us = s.utf8();
		[out addObject:[OBString stringWithUTF8String:us.data()]];
	}

	return out;
}

- (OBString *)enabledDictionary
{
	WTF::String lang = WebKit::WebEditorClient::getSpellCheckingLanguage();
	WTF::String defalang;
	WTF::Vector<WTF::String> languages;
	WebKit::WebEditorClient::getAvailableDictionaries(languages, defalang);
	if (lang.length())
	{
		auto ulang = lang.utf8();
		return [OBString stringWithUTF8String:ulang.data()];
	}
	if (defalang.length())
	{
		auto ulang = defalang.utf8();
		return [OBString stringWithUTF8String:ulang.data()];
	}
	return nil;
}

- (OBArray /* OBString */ *)availableDictionaries
{
	WTF::String defalang;
	WTF::Vector<WTF::String> languages;
	WebKit::WebEditorClient::getAvailableDictionaries(languages, defalang);
	OBMutableArray *out = [OBMutableArray arrayWithCapacity:languages.size()];
	for (const WTF::String &s : languages)
	{
		auto ulang = s.utf8();
		[out addObject:[OBString stringWithUTF8String:ulang.data()]];
	}
	return out;
}

- (void)learnMissspelledWord
{
	_page->learnMisspelled(*_hitTest);
}

- (void)ignoreMisspelledWord
{
	_page->ignoreMisspelled(*_hitTest);
}

- (void)replaceMisspelledWord:(OBString *)correctWord
{
	_page->replaceMisspelled(*_hitTest, WTF::String::fromUTF8([correctWord cString]));
}

- (OBURL *)linkURL
{
	OBString *linkText = [self stringFromWTFString:_hitTest->absoluteLinkURL().string()];
	if (linkText)
		return [OBURL URLWithString:linkText];
	return nil;
}

- (BOOL)isImage
{
	return _hitTest->image() ? YES : NO;
}

- (OBURL *)imageURL
{
	OBString *linkText = [self stringFromWTFString:_hitTest->absoluteImageURL().string()];
	if (linkText)
		return [OBURL URLWithString:linkText];
	return nil;
}

- (OBString *)imageFileExtension
{
	if (_hitTest->image())
		return [self stringFromWTFString:_hitTest->image()->filenameExtension()];
	return nil;
}

- (OBString *)imageMimeType
{
	if (_hitTest->image())
		return [self stringFromWTFString:_hitTest->image()->mimeType()];
	return nil;
}

- (LONG)imageWidth
{
	if (_hitTest->image())
		return _hitTest->imageRect().width();
	return 0;
}

- (LONG)imageHeight
{
	if (_hitTest->image())
		return _hitTest->imageRect().height();
	return 0;
}

- (void)downloadLinkFile
{
	_page->startDownload(_hitTest->absoluteLinkURL());
}

- (void)downloadImageFile
{
	_page->startDownload(_hitTest->absoluteImageURL());
}

- (BOOL)copyImageToClipboard
{
	return _page->hitTestImageToClipboard(*_hitTest);
}

- (BOOL)saveImageToFile:(OBString *)path
{
	return _page->hitTestSaveImageToFile(*_hitTest, WTF::String::fromUTF8([path cString]));
}

- (void)replaceSelectedTextWidth:(OBString *)replacement
{
	_page->hitTestReplaceSelectedTextWidth(*_hitTest, WTF::String::fromUTF8([replacement cString]));
}

- (void)cutSelectedText
{
	_page->hitTestCutSelectedText(*_hitTest);
}

- (void)pasteText
{
	_page->hitTestPaste(*_hitTest);
}

- (void)selectAll
{
	_page->hitTestSelectAll(*_hitTest);
}

- (void)setImageFloat:(WkHitTestImageFloat)imageFloat
{
	_page->hitTestSetImageFloat(*_hitTest, WebKit::WebPage::ContextMenuImageFloat(imageFloat));
}

- (WkHitTestImageFloat)imageFloat
{
	return WkHitTestImageFloat(_page->hitTestImageFloat(*_hitTest));
}

@end

@implementation WkHitTest

- (OBString *)selectedText
{
	return nil;
}

- (OBString *)title
{
	return nil;
}

- (OBString *)altText
{
	return nil;
}

- (OBString *)textContent
{
	return nil;
}

- (BOOL)isContentEditable
{
	return NO;
}

- (OBString *)misspelledWord
{
	return nil;
}

- (OBArray *)guessesForMisspelledWord
{
	return nil;
}

- (OBArray /* OBString */ *)availableDictionaries
{
	return nil;
}

- (OBString *)enabledDictionary
{
	return nil;
}

- (void)learnMissspelledWord
{

}

- (void)ignoreMisspelledWord
{

}

- (void)replaceMisspelledWord:(OBString *)correctWord
{
	(void)correctWord;
}

- (OBURL *)linkURL
{
	return nil;
}

- (BOOL)isImage
{
	return NO;
}

- (OBURL *)imageURL
{
	return nil;
}

- (OBString *)imageFileExtension
{
	return nil;
}

- (OBString *)imageMimeType
{
	return nil;
}

- (LONG)imageWidth
{
	return 0;
}

- (LONG)imageHeight
{
	return 0;
}

- (BOOL)copyImageToClipboard
{
	return NO;
}

- (BOOL)saveImageToFile:(OBString *)path
{
	(void)path;
	return NO;
}

- (void)replaceSelectedTextWidth:(OBString *)replacement
{
	(void)replacement;
}

- (void)cutSelectedText
{

}

- (void)pasteText
{

}

- (void)selectAll
{

}

- (void)downloadLinkFile
{

}

- (void)downloadImageFile
{

}

- (void)setImageFloat:(WkHitTestImageFloat)imagefloat
{

}

- (WkHitTestImageFloat)imageFloat
{
	return WkHitTestImageFloat_None;
}

@end
