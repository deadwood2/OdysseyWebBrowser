#import "WkFileDialog_private.h"
#undef __OBJC__
#include <wtf/WallTime.h>
#include <wtf/text/WTFString.h>
#include <WebCore/CurlDownload.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/TextEncoding.h>
#include "WebProcess.h"
#define __OBJC__
#import <ob/OBFramework.h>

@implementation WkFileDialogResponseHandlerPrivate

- (id)initWithChooser:(WebCore::FileChooser&)chooser
{
    self = [super init];
    if (!self)
        return nil;
    _chooser = &chooser;
    return self;
}

- (void)cancelled
{
	if (_chooser)
	{
		WebKit::WebProcess::singleton().returnedFromConstrainedRunLoop();
	}
    _chooser = nullptr;
}

- (void)selectedFile:(OBString *)file
{
	if (file && _chooser)
	{
		const char *cpath = [file nativeCString];
		_chooser->chooseFile(WTF::String(cpath, strlen(cpath), MIBENUM_SYSTEM));
	}
	_chooser = nullptr;

	WebKit::WebProcess::singleton().returnedFromConstrainedRunLoop();
}

- (void)selectedFiles:(OBArray /* OBString */ *)files
{
	ULONG count = [files count];

	if (!_chooser)
	{
		return;
	}

	if (0 == count)
	{
		[self cancelled];
	}
	else if (1 == count)
	{
		const char *cpath = [[files firstObject] nativeCString];
		_chooser->chooseFile(WTF::String(cpath, strlen(cpath), MIBENUM_SYSTEM));
		_chooser = nullptr;
	}
	else
	{
	    WTF::Vector<WTF::String> names(count);
    	for (ULONG i = 0; i < count; i++)
    	{
			const char *cpath = [[files objectAtIndex:i] nativeCString];
        	names[i] = WTF::String(cpath, strlen(cpath), MIBENUM_SYSTEM);
		}
		_chooser->chooseFiles(names);
		_chooser = nullptr;
	}
	
	WebKit::WebProcess::singleton().returnedFromConstrainedRunLoop();
}

- (BOOL)allowsDirectories
{
	if (_chooser)
		return _chooser->settings().allowsDirectories;
	return NO;
}

- (BOOL)allowsMultipleFiles
{
	if (_chooser)
		return _chooser->settings().allowsMultipleFiles;
	return NO;
}

- (OBArray *)arrayFromStringVector:(const WTF::Vector<WTF::String> &)vector
{
	ULONG count = vector.size();
	OBMutableArray *out = [OBMutableArray arrayWithCapacity:count];
	for (ULONG i = 0; i < count; i++)
	{
		auto u = vector[i].utf8();
		[out addObject:[OBString stringWithUTF8String:u.data()]];
	}
	return nil;
}

- (OBArray /* OBString */ *)acceptedMimeTypes
{
	if (_chooser)
		return [self arrayFromStringVector:_chooser->settings().acceptMIMETypes];
	return nil;
}

- (OBArray /* OBString */ *)acceptedFileExtensions
{
	if (_chooser)
		return [self arrayFromStringVector:_chooser->settings().acceptFileExtensions];
	return nil;
}

- (OBArray /* OBString */ *)selectedFiles
{
	if (_chooser)
		return [self arrayFromStringVector:_chooser->settings().selectedFiles];
	return nil;
}

@end
