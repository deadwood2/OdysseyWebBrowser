#undef __OBJC__
#import "WebKit.h"
#import <WebCore/SharedBuffer.h>
#import <WebCore/BitmapImage.h>
#define __OBJC__
#import "WkFavIcon_private.h"
#import <proto/graphics.h>
#import <proto/intuition.h>
#import <graphics/gfx.h>
#import <cybergraphx/cybergraphics.h>
#import <proto/cybergraphics.h>
#import <cairo.h>

namespace WebKit {
	String generateFileNameForIcon(const WTF::String &inHost);
}

@implementation WkFavIconPrivate

- (BOOL)loadSharedData:(WebCore::SharedBuffer *)data
{
    auto image = WebCore::BitmapImage::create();
    if (image->setData(data, true) < WebCore::EncodedDataStatus::SizeAvailable)
        return NO;
	
	auto nativeImage = image->nativeImageForCurrentFrame();
	if (!nativeImage.get())
		return NO;

	auto& native = nativeImage->platformImage();

	unsigned char *imgdata;
	int width, height, stride, cairo_stride;

	cairo_surface_flush (native.get());

	imgdata = cairo_image_surface_get_data (native.get());
	if (nullptr == imgdata)
		return NO;

	width = cairo_image_surface_get_width (native.get());
	height = cairo_image_surface_get_height (native.get());
	stride = cairo_image_surface_get_stride (native.get());
	cairo_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

	UBYTE *bytes = (UBYTE *)malloc(cairo_stride * height);

	if (bytes == nullptr)
		return NO;

	for (int i = 0; i < height; i++)
	{
		memcpy(bytes + (i * width * 4), imgdata + (i * stride), cairo_stride);
	}

	@synchronized (self) {
		if (_dataPrescaled)
			free(_dataPrescaled);
		_dataPrescaled = bytes;
		_widthPrescaled = width;
		_heightPrescaled = height;
		_useAlpha = YES; //!
	}

	return YES;
}

- (WkFavIconPrivate *)initWithSharedData:(WebCore::SharedBuffer *)data forHost:(OBString *)host
{
	if ((self = [super init]))
	{
		_host = WTF::String::fromUTF8([[host lowercaseString] cString]);
		
		if (![self loadSharedData:data])
		{
			[self release];
			return nil;
		}
	}
	
	return self;
}

- (void)askMinMax:(struct MUI_MinMax *)minmaxinfo
{
	minmaxinfo->MinWidth += 12;
	minmaxinfo->MinHeight += 12;
	
	minmaxinfo->DefWidth += 18;
	minmaxinfo->DefHeight += 18;

	minmaxinfo->MaxWidth += 64;
	minmaxinfo->MaxHeight += 64;
}

- (void)onThreadDone
{
	[self redraw:MADF_DRAWUPDATE];
}

- (void)thread:(OBNumber *)targetHeight
{

	if (nullptr == _dataPrescaled)
	{
		RefPtr<WebCore::SharedBuffer> buffer = WebCore::SharedBuffer::createWithContentsOfFile(WebKit::generateFileNameForIcon(_host));
		if (buffer.get() && buffer->size())
		{
			[self loadSharedData:buffer.get()];
		}
	}

	if (_dataPrescaled)
	{
		UBYTE *data;
		LONG height = [targetHeight longValue];
		
		int xwidth = _widthPrescaled, xheight = _heightPrescaled;

		float ratio = ((float)height) / ((float)xheight);
		LONG width = floor(((float)xwidth) * ratio);
		
		data = (UBYTE *)malloc(width * height * 4);

		if (data)
		{
			auto surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
			auto source_surface = cairo_image_surface_create_for_data(_dataPrescaled, CAIRO_FORMAT_ARGB32,
				xwidth, xheight, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, xwidth));
			auto cairo = cairo_create(surface);

			if (surface && source_surface && cairo)
			{
				cairo_save(cairo);
				cairo_set_source_rgba(cairo, 0, 0, 0, 0);
				cairo_rectangle(cairo, 0, 0, width, height);
				cairo_scale(cairo, ((double)width) / ((double)xwidth), ((double)height) / ((double)xheight));
				cairo_pattern_set_filter(cairo_get_source(cairo), CAIRO_FILTER_GOOD);
				cairo_set_source_surface(cairo, source_surface, 0, 0);
				cairo_paint(cairo);

				unsigned char *imgdata;
				int width, height, stride;

				cairo_surface_flush(surface);
				imgdata = cairo_image_surface_get_data(surface);
				width = cairo_image_surface_get_width (surface);
				height = cairo_image_surface_get_height(surface);
				stride = cairo_image_surface_get_stride(surface);

				for (int i = 0; i < height; i++)
				{
					memcpy(data + (i * width * 4), imgdata + (i * stride), width * 4);
				}
			}
			else
			{
				free(data);
				data = nullptr;
			}

			cairo_destroy(cairo);
			cairo_surface_destroy(source_surface);
			cairo_surface_destroy(surface);

			free(_dataPrescaled);
			_dataPrescaled = NULL;
			
			@synchronized (self) {
				if (_data)
					free(_data);
				_data = data;
				_width = width;
				_height = height;
			}
			
			[[OBRunLoop mainRunLoop] performSelector:@selector(onThreadDone) target:self];
		}
	}
}

- (void)onShowTimer
{
	[_loadResizeTimer invalidate];
	[_loadResizeTimer autorelease];
	_loadResizeTimer = nil;
	
	[OBThread startWithObject:self selector:@selector(thread:) argument:[OBNumber numberWithLong:[self innerHeight]]];
}

- (BOOL)show:(struct LongRect *)clip
{
	if ([super show:clip])
	{
		if (_loadResizeTimer)
		{
			[_loadResizeTimer invalidate];
			[_loadResizeTimer autorelease];
			_loadResizeTimer = nil;
		}
		
		if ([self innerHeight] != _height || !_data)
		{
			_loadResizeTimer = [[OBScheduledTimer scheduledTimerWithInterval:1.0 perform:[OBPerform performSelector:@selector(onShowTimer) target:self] repeats:NO] retain];
		}

		return YES;
	}
	
	return NO;
}

- (void)dealloc
{
	if (_data)
		free(_data);
	if (_dataPrescaled)
		free(_dataPrescaled);
	if (_loadResizeTimer)
	{
		[_loadResizeTimer invalidate];
		[_loadResizeTimer autorelease];
	}
	[super dealloc];
}

- (BOOL)draw:(ULONG)flags
{
	[super draw:flags];

	@synchronized (self) {

		if (_data && nullptr == _dataPrescaled)
		{
			if (_useAlpha)
				WritePixelArrayAlpha(_data, 0, 0, _width * 4, [self rastPort],
					[self left] + (([self innerWidth] - _width) / 2), [self top], _width, _height, 0xFFFFFFFF);
			else
				WritePixelArray(_data, 0, 0, _width * 4, [self rastPort],
					[self left] + (([self innerWidth] - _width) / 2), [self top], _width, _height, RECTFMT_ARGB);
		}
	}

	return YES;
}

+ (WkFavIconPrivate *)cacheIconWithData:(WebCore::SharedBuffer *)data forHost:(OBString *)host
{
	if (data && data->size())
	{
		return [[[self alloc] initWithSharedData:data forHost:host] autorelease];
	}
	
	return nil;
}

@end

@implementation WkFavIcon

+ (WkFavIcon *)favIconForHost:(OBString *)host
{
	RefPtr<WebCore::SharedBuffer> buffer = WebCore::SharedBuffer::createWithContentsOfFile(WebKit::generateFileNameForIcon(WTF::String::fromUTF8([host cString])));
	if (buffer.get() && buffer->size())
	{
		return [[[WkFavIconPrivate alloc] initWithSharedData:buffer.get() forHost:host] autorelease];
	}
	
	return nil;
}

@end
