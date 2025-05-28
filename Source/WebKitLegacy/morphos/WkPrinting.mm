#import "WkPrinting_private.h"
#import <proto/dos.h>
#import <proto/obframework.h>
#import <proto/ppd.h>
#import <proto/exec.h>
#import <mui/MUIFramework.h>
#import "WkWebView.h"
#import <exec/resident.h>

extern "C" { void dprintf(const char *,...); }

@interface WkWebView ()
- (void)updatePrinting;
- (void)updatePrintPreviewSheet;
- (void)internalSetPageZoomFactor:(float)pageFactor textZoomFactor:(float)textFactor;
@end

template <class T> class ForListNodes {
public:
	ForListNodes(struct ::List *list) {
		_node = list ? reinterpret_cast<struct ::MinNode *>(list->lh_Head) : nullptr;
	}
	ForListNodes(struct ::MinList *list) {
		_node = list ? reinterpret_cast<struct ::MinNode *>(list->mlh_Head) : nullptr;
	}
	ForListNodes(struct ::List &list) {
		_node = reinterpret_cast<struct ::MinNode *>(list.lh_Head);
	}
	ForListNodes(struct ::MinList &list) {
		_node = reinterpret_cast<struct ::MinNode *>(list.mlh_Head);
	}
	T *nextNode() {
		if (_node && _node->mln_Succ) {
			auto t = reinterpret_cast<T *>(_node);
			_node = _node->mln_Succ;
			return t;
		}
		return nullptr;
	}
protected:
	struct ::MinNode *_node;
};

@implementation WkPrintingPagePrivate

- (id)initWithName:(OBString *)name key:(OBString *)key width:(float)width height:(float)height
	marginLeft:(float)mleft marginRight:(float)mright marginTop:(float)mtop marginBottom:(float)mbottom
{
	if ((self = [super init]))
	{
		_name = [name retain];
		_key = [key retain];
		_width = width;
		_height = height;
		_marginLeft = mleft;
		_marginRight = mright;
		_marginTop = mtop;
		_marginBottom = mbottom;
	}
	
	return self;
}

- (void)dealloc
{
	[_name release];
	[_key release];
	[super dealloc];
}

+ (WkPrintingPagePrivate *)pageWithName:(OBString *)name key:(OBString *)key width:(float)width height:(float)height
	marginLeft:(float)mleft marginRight:(float)mright marginTop:(float)mtop marginBottom:(float)mbottom
{
	return [[[self alloc] initWithName:name key:key width:width height:height marginLeft:mleft marginRight:mright marginTop:mtop marginBottom:mbottom] autorelease];
}

- (OBString *)name
{
	return _name;
}

- (OBString *)key
{
	return _key;
}

- (float)width
{
	return _width;
}

- (float)height
{
	return _height;
}

- (float)contentWidth
{
	return _width - (_marginLeft + _marginRight);
}

- (float)contentHeight
{
	return _height - (_marginTop + _marginBottom);
}

- (float)marginLeft
{
	return _marginLeft;
}

- (float)marginRight
{
	return _marginRight;
}

- (float)marginTop
{
	return _marginTop;
}

- (float)marginBottom
{
	return _marginBottom;
}

@end

@implementation WkPrintingProfilePrivate

- (WkPrintingPage *)pageForNode:(PAGE_SIZE_NODE *)node
{
	if (!node)
		return nil;
	
	if (!node->Name || !*node->Name)
		return nil;
	
	if (!node->Full_Name || !*node->Full_Name)
		return nil;

	float w = node->Width;
	float h = node->Height;
	float margins[4];
	w /= 72.f;
	h /= 72.f;
	margins[0] = node->Left_Margin;
	margins[1] = node->Right_Margin;
	margins[2] = node->Top_Margin;
	margins[3] = node->Bottom_Margin;
	margins[0] = margins[0] / 72.f;
	margins[1] = margins[1] / 72.f;
	margins[2] = margins[2] / 72.f;
	margins[3] = margins[3] / 72.f;
	margins[1] = w - margins[1];
	margins[2] = h - margins[2]; // wut da fuq?

	return [WkPrintingPagePrivate pageWithName:[OBString stringWithUTF8String:node->Full_Name] key:[OBString stringWithUTF8String:node->Name] width:w height:h
		marginLeft:margins[0] marginRight:margins[1] marginTop:margins[2] marginBottom:margins[3]];
}

- (id)initWithProfile:(OBString *)profile state:(WkPrintingState *)state
{
	if ((self = [super init]))
	{
		_profile = [profile retain];
		_state = (id)state; // !

		struct Library *PPDBase = _ppdBase = OpenLibrary("ppd.library", 50);
		if (PPDBase)
		{
			PPD_ERROR err;
			const char *path = [[OBString stringWithFormat:@"SYS:Prefs/Printers/Profiles/%@", _profile] nativeCString];
			_ppd = OpenPPDFromIFF((STRPTR)path, &err);
			
			if (_ppd)
			{
				_page = [[self pageForNode:_ppd->SelectedPageSize] retain];
				
				if (!_page)
				{
					_page = [[self defaultPageFormat] retain];
				}
				
				if (!_page)
				{
					_page = [[[self pageFormats] firstObject] retain];
				}
			}
		}
	}

	return self;
}

- (void)dealloc
{
	struct Library *PPDBase = _ppdBase;
	(void)PPDBase;
	if (_ppd)
		ClosePPD(_ppd);
	if (_ppdBase)
		CloseLibrary(_ppdBase);
	[_page release];
	[_profile release];
	[super dealloc];
}

- (void)clearState
{
	_state = nil;
}

- (OBArray*)pageFormats
{
	OBMutableArray *out = [OBMutableArray array];
	
	if (_ppd)
	{
		ForListNodes<PAGE_SIZE_NODE> fn(_ppd->PageSizes);
		PAGE_SIZE_NODE *node;

		while ((node = fn.nextNode()))
		{
			WkPrintingPage *page = [self pageForNode:node];
			if (page)
				[out addObject:page];
		}
	}
	else
	{
		[out addObject:[WkPrintingPagePrivate pageWithName:@"A4" key:@"A4" width:8.3f height:11.7f marginLeft:0.2 marginRight:0.2 marginTop:0.2 marginBottom:0.2]];
	}

	return out;
}

- (WkPrintingPage *)defaultPageFormat
{
	if (_ppd)
	{
		return [self pageForNode:_ppd->DefaultPageSize];
	}

	return nil;
}

- (BOOL)canSelectPageFormat
{
	struct Resident *rt = FindResident("MorphOS");
	if (rt->rt_Version > 3 || rt->rt_Revision >= 15)
		return YES;
	return NO;
}

- (void)setSelectedPageFormat:(WkPrintingPage *)page
{
	if (page)
	{
		[_page autorelease];
		_page = [page retain];
		// force refresh on page change
		[_state setProfile:[_state profile]];
	}
}

- (WkPrintingPage *)selectedPageFormat
{
	return _page;
}

- (OBString *)printerModel
{
	if (_ppd)
		return [OBString stringWithUTF8String:_ppd->Description.Model_Name];
	return nil;
}

- (OBString *)manufacturer
{
	if (_ppd)
		return [OBString stringWithUTF8String:_ppd->Description.Manufacturer];
	return nil;
}

- (LONG)psLevel
{
	if (_ppd && _ppd->Parameters.PS_Level == 2)
		return 2;
	return 3;
}

- (OBString *)name
{
	return _profile;
}

@end

@interface WkPrintingPDFProfile : WkPrintingProfilePrivate
{
	OBArray                *_formats;
}
@end

@implementation WkPrintingPDFProfile

- (id)initWithState:(WkPrintingStatePrivate *)state
{
	if ((self = [super init]))
	{
		_state = state; // CAUTION
	}

	return self;
}

- (void)dealloc
{
	[_formats release];
	[super dealloc];
}

- (void)clearState
{
	_state = nil;
}

- (BOOL)isPDFFilePrinter
{
	return YES;
}

- (OBString *)printerModel
{
	return @"PDF";
}

- (OBString *)manufacturer
{
	return @"MorphOS Team";
}

- (OBArray*)pageFormats
{
	if (!_formats)
	{
		OBMutableArray *out = [OBMutableArray array];
		[out addObject:[WkPrintingPagePrivate pageWithName:@"A4" key:@"A4" width:8.3f height:11.7f
			marginLeft:0.2 marginRight:0.2 marginTop:0.2 marginBottom:0.2]];
		[out addObject:[WkPrintingPagePrivate pageWithName:@"A3" key:@"A3" width:11.7f height:16.5f
			marginLeft:0.2 marginRight:0.2 marginTop:0.2 marginBottom:0.2]];
		[out addObject:[WkPrintingPagePrivate pageWithName:@"A2" key:@"A2" width:16.5f height:23.4f
			marginLeft:0.2 marginRight:0.2 marginTop:0.2 marginBottom:0.2]];
		[out addObject:[WkPrintingPagePrivate pageWithName:@"US Letter" key:@"USLetter" width:8.5f height:11.f
			marginLeft:0.2 marginRight:0.2 marginTop:0.2 marginBottom:0.2]];
		[out addObject:[WkPrintingPagePrivate pageWithName:@"US Legal" key:@"USLetter" width:8.5f height:14.f
			marginLeft:0.2 marginRight:0.2 marginTop:0.2 marginBottom:0.2]];
		_formats = [out retain];
	}
	
	return _formats;
}

- (BOOL)canSelectPageFormat
{
	return YES;
}

- (void)setSelectedPageFormat:(WkPrintingPage *)page
{
	if (page)
	{
		[_page autorelease];
		_page = [page retain];
		// force refresh on page change
		[_state setProfile:[_state profile]];
	}
}

- (WkPrintingPage *)selectedPageFormat
{
	if (_page)
		return _page;
	
	// force array generation
	return [[self pageFormats] firstObject];
}

- (OBString *)name
{
	return @"PDF";
}

@end

@implementation WkPrintingProfile

+ (OBArray /* OBString */ *)allProfiles
{
	BPTR lock = Lock("SYS:Prefs/Printers/Profiles", ACCESS_READ);
	OBMutableArray *out = nil;

	if (lock)
	{
		APTR buffer = OBAlloc(4096);
		struct ExAllControl *eac = (struct ExAllControl *)(buffer ? AllocDosObject(DOS_EXALLCONTROL, 0) : NULL);
		
		if (eac)
		{
			BOOL more;
			eac->eac_LastKey = 0;

			out = [OBMutableArray arrayWithCapacity:128];
			
			do
			{
				more = ExAll(lock, (struct ExAllData *)buffer, 4096, ED_TYPE, eac);
				if ((!more) && (IoErr() != ERROR_NO_MORE_ENTRIES)) {
					out = nil;
					break;
				}
				if (eac->eac_Entries == 0) {
					continue;
				}
				struct ExAllData *ead = (struct ExAllData *)buffer;
				do {
					if (ead->ed_Type < 0)
						[out addObject:[OBString stringWithCString:CONST_STRPTR(ead->ed_Name) encoding:MIBENUM_SYSTEM]];
					ead = ead->ed_Next;
				} while (ead);
			} while (more);
			
			FreeDosObject(DOS_EXALLCONTROL, eac);
		}
		
		if (buffer) OBFree(buffer);
		UnLock(lock);
	}
	
	return out;
}

+ (OBString *)defaultProfile
{
	OBString *defaultProfile = nil;
	char ppdPath[1024];

	if (-1 != GetVar("DefaultPrinter", ppdPath, sizeof(ppdPath), 0))
	{
		const char *name = PathPart(ppdPath);
		if (name)
			defaultProfile = [OBString stringWithCString:name encoding:MIBENUM_SYSTEM];
	}
	
	return defaultProfile;
}

+ (WkPrintingProfile *)spoolInfoForProfile:(OBString *)profile withState:(WkPrintingState *)state
{
	if (profile)
		return [[[WkPrintingProfilePrivate alloc] initWithProfile:profile state:state] autorelease];
	return nil;
}

+ (WkPrintingProfile *)pdfProfileWithState:(WkPrintingStatePrivate *)state
{
	return [[[WkPrintingPDFProfile alloc] initWithState:state] autorelease];
}

- (BOOL)canSelectPageFormat
{
	return NO;
}

- (void)setSelectedPageFormat:(WkPrintingPage *)page
{
	(void)page;
}

- (OBArray /* WkPrintingPage */*)pageFormats
{
	return nil;
}

- (WkPrintingPage *)defaultPageFormat
{
	return nil;
}

- (WkPrintingPage *)selectedPageFormat
{
	return nil;
}

- (OBString *)printerModel
{
	return nil;
}

- (OBString *)manufacturer
{
	return nil;
}

- (LONG)psLevel
{
	return 2;
}

- (BOOL)isPDFFilePrinter
{
	return NO;
}

- (OBString *)name
{
	return @"?";
}

@end

@implementation WkPrintingPage

- (OBString *)name
{
	return @"";
}

- (OBString *)key
{
	return @"";
}

- (float)width
{
	return 1.0f;
}

- (float)height
{
	return 1.0f;
}

- (float)contentWidth
{
	return 1.0f;
}

- (float)contentHeight
{
	return 1.0f;
}

- (float)marginLeft
{
	return 1.0f;
}

- (float)marginRight
{
	return 1.0f;
}

- (float)marginTop
{
	return 1.0f;
}

- (float)marginBottom
{
	return 1.0f;
}

@end

@implementation WkPrintingStatePrivate

- (id)initWithWebView:(WkWebView *)view frame:(WebCore::Frame *)frame
{
	if ((self = [super init]))
	{
		_webView = view;
		_context = new WebCore::PrintContext(frame);
		OBArray *profileNames = [WkPrintingProfile allProfiles];
		_profiles = [[OBMutableArray arrayWithCapacity:[profileNames count] + 1] retain];
		_scale = 1.0f;
		_pagesPerSheet = 1;
		_printBackgrounds = YES;
		_previewedSheet = 1;
		_copies = 1;

		OBString *defaultProfile = [WkPrintingProfile defaultProfile];
		OBEnumerator *eNames = [profileNames objectEnumerator];
		OBString *name;

		[_profiles addObject:[WkPrintingProfilePrivate pdfProfileWithState:self]];

		while ((name = [eNames nextObject]))
		{
			WkPrintingProfile *profile = [WkPrintingProfile spoolInfoForProfile:name withState:self];
			if (profile)
			{
				[_profiles addObject:profile];
				if ([name isEqualToString:defaultProfile])
					[self setProfile:_profile];
			}
		}

		if (!_profile)
			[self setProfile:[_profiles firstObject]];
	}

	return self;
}

- (void)dealloc
{
	delete _context;
	OBEnumerator *e = [_profiles objectEnumerator];
	WkPrintingPDFProfile *profile;
	while ((profile = [e nextObject]))
	{
		if ([profile isKindOfClass:[WkPrintingPDFProfile class]])
			[(id)profile clearState];
	}
	[_profile release];
	[_profiles release];
	[_range release];
	[super dealloc];
}

- (WkWebView *)webView
{
	return _webView;
}

- (WkPrintingProfile *)profile
{
	return _profile;
}

- (OBArray * /* WkPrintingProfile */)allProfiles
{
	return _profiles;
}

- (WkPrintingPage *)pageWithMarginsApplied
{
	WkPrintingPage *page = [_profile selectedPageFormat];
	
	if (_defaultMargins)
		return page;
	
	return [WkPrintingPagePrivate pageWithName:[page name] key:[page key] width:[page width] height:[page height]
		marginLeft:_marginLeft marginRight:_marginRight marginTop:_marginTop marginBottom:_marginBottom];
}

- (void)needsRedraw
{
	[_webView updatePrinting];
}

- (void)recalculatePages
{
	WkPrintingPage *page = [self pageWithMarginsApplied];

	if (page)
	{
		float contentWidth = [page contentWidth];
		float contentHeight = [page contentHeight];

		if (_landscape)
		{
			contentHeight = [page contentWidth];
			contentWidth = [page contentHeight];
		}

		_context->end();

		float mleft, mright, mtop, mbottom;

		mleft = _marginLeft;
		mright = _marginRight;
		mtop = _marginTop;
		mbottom = _marginBottom;
		
		// works, so meh
		[_webView internalSetPageZoomFactor:1.0f textZoomFactor:_scale];

		// RectEdges(U&& top, U&& right, U&& bottom, U&& left)
		WebCore::FloatBoxExtent margins(mtop * 72.f, mright * 72.f, mbottom * 72.f, mleft * 72.f);
		auto computedPageSize = _context->computedPageSize(WebCore::FloatSize(contentWidth * 72.f, contentHeight * 72.f), margins);
		_context->begin(computedPageSize.width(), computedPageSize.height());

		float fullPageHeight;
		_context->computePageRects(WebCore::FloatRect(0, 0, computedPageSize.width(), computedPageSize.height()), 0, 0,
			1.0f, fullPageHeight, false);

		if (_previewedSheet > LONG(_context->pageCount()))
			_previewedSheet = 1;

		[self needsRedraw];
		
		[[OBNotificationCenter defaultCenter] postNotificationName:kWkPrintingStateRecalculated object:self];
	}
}

- (void)setProfile:(WkPrintingProfile *)profile
{
	[_profile autorelease];
	_profile = [profile retain];

	WkPrintingPage *page = [_profile selectedPageFormat];

	if (_context && page)
	{
		_defaultMargins = YES;
		
		_marginLeft = [page marginLeft];
		_marginTop = [page marginTop];
		_marginRight = [page marginRight];
		_marginBottom = [page marginBottom];

		[self recalculatePages];
	}
}

- (void)invalidate
{
	if (_context)
		delete _context;
	_context = nullptr;
	_webView = nil;
}

- (WebCore::PrintContext *)context
{
	return _context;
}

- (WebCore::FloatBoxExtent)printMargins
{
	return WebCore::FloatBoxExtent(_marginTop * 72.f, _marginRight * 72.f,
			_marginBottom * 72.f, _marginLeft * 72.f);
}

// ---- Page and layout setup

- (void)setLandscape:(BOOL)landscape
{
	_landscape = landscape;
	[self recalculatePages];
}

- (BOOL)landscape
{
	return _landscape;
}

- (LONG)pagesPerSheet
{
	return _pagesPerSheet;
}

- (void)setPagesPerSheet:(LONG)pps
{
	switch (pps)
	{
	case 1:
	case 2:
	case 4:
	case 6:
	case 9:
		break;
	default:
		pps = 1;
	}
	_pagesPerSheet = pps;
	[self recalculatePages];
}

- (float)userScalingFactor
{
	return _scale;
}

- (void)setUserScalingFactor:(float)scaling
{
	if (_scale != scaling)
	{
		_scale = scaling;
		[self recalculatePages];
	}
}

- (float)marginLeft
{
	return _marginLeft;
}

- (float)marginTop
{
	return _marginTop;
}

- (float)marginRight
{
	return _marginRight;
}

- (float)marginBottom
{
	return _marginBottom;
}

- (void)setMarginLeft:(float)left top:(float)top right:(float)right bottom:(float)bottom
{
	_defaultMargins = NO;
	_marginLeft = left;
	_marginTop = top;
	_marginBottom = bottom;
	_marginRight = right;
	[self recalculatePages];
}

- (void)resetMarginsToPaperDefaults
{
	_defaultMargins = YES;
	_marginLeft = [[_profile selectedPageFormat] marginLeft];
	_marginTop = [[_profile selectedPageFormat] marginTop];
	_marginRight = [[_profile selectedPageFormat] marginRight];
	_marginBottom = [[_profile selectedPageFormat] marginBottom];
	[self recalculatePages];
}

// ---- Print job setup

- (void)validatePrintingRange
{
	if (_range && [_range pageStart] >= 1 && [_range pageEnd] <= [self sheets])
		return;
	[_range autorelease];
	_range = nil;
}

- (WkPrintingRange *)printingRange
{
	[self validatePrintingRange];

	if (_range)
		return _range;
	return [WkPrintingRange rangeFromPage:1 count:[self sheets]];
}

- (void)setPrintingRange:(WkPrintingRange *)range
{
	[_range autorelease];
	_range = [range retain];
}

- (WkPrintingState_Parity)parity
{
	return _parity;
}

- (void)setParity:(WkPrintingState_Parity)parity
{
	_parity = parity;
}

- (LONG)copies
{
	return _copies;
}

- (void)setCopies:(LONG)numCopies
{
	if (numCopies < 1)
		numCopies = 1;
	if (numCopies > 50)
		numCopies = 50;
	_copies = numCopies;
}

// ---- Info and preview

- (LONG)pages
{
	if (_context)
		return _context->pageCount();
	return 1;
}

- (LONG)sheets
{
	if (_context)
	{
		if (_pagesPerSheet > 1)
			return (_context->pageCount() / _pagesPerSheet) + ((_context->pageCount() % _pagesPerSheet) != 0 ? 1 : 0);
		return _context->pageCount();
	}

	return 1;
}

- (LONG)printJobSheets
{
	WkPrintingRange *range = [self printingRange];
	LONG start = [range pageStart];
	LONG end = [range pageEnd];
	LONG count = 0;
	
	for (LONG i = start; i <= end; i++)
	{
		if ((i & 1) == 0) // even steven
		{
			if (_parity != WkPrintingState_Parity_OddSheets)
				count ++;
		}
		else // odd steven
		{
			if (_parity != WkPrintingState_Parity_EvenSheets)
				count ++;
		}
	}
	
	return count;
}

- (LONG)previevedSheet
{
	return _previewedSheet;
}

- (void)setPrevievedSheet:(LONG)sheet
{
	if (_previewedSheet != sheet && sheet >= 1 && sheet <= [self sheets])
	{
		_previewedSheet = sheet;
		[_webView updatePrintPreviewSheet];
	}
}

- (BOOL)shouldPrintBackgrounds
{
	return _printBackgrounds;
}

- (void)setShouldPrintBackgrounds:(BOOL)printBackgrounds
{
	_printBackgrounds = printBackgrounds;
	[self needsRedraw];
}

- (OBDictionary *)settings
{
	OBMutableDictionary *out = [OBMutableDictionary dictionaryWithCapacity:16];
	[out setObject:[_profile name] forKey:@"profile"];
	[out setObject:[[_profile selectedPageFormat] key] forKey:@"pageFormat"];
	if (!_defaultMargins)
	{
		[out setObject:[OBArray arrayWithObjects:[OBNumber numberWithFloat:_marginLeft], [OBNumber numberWithFloat:_marginRight],
			[OBNumber numberWithFloat:_marginTop], [OBNumber numberWithFloat:_marginBottom], nil] forKey:@"margins"];
	}
	[out setObject:[OBNumber numberWithFloat:_scale] forKey:@"scale"];
	[out setObject:[OBNumber numberWithBool:_landscape] forKey:@"landscape"];
	[out setObject:[OBNumber numberWithBool:_printBackgrounds] forKey:@"backgrounds"];
	return out;
}

- (void)setSettings:(OBDictionary *)settings
{
	OBString *profileName = [settings objectForKey:@"profile"];
	OBString *pageFormat = [settings objectForKey:@"pageFormat"];
	OBArray *margins = [settings objectForKey:@"margins"];
	OBNumber *scale = [settings objectForKey:@"scale"];
	OBNumber *landscape = [settings objectForKey:@"landscape"];
	OBNumber *backgrounds = [settings objectForKey:@"backgrounds"];
	
	if (!profileName || ![profileName isKindOfClass:[OBString class]])
		return;
	
	if (!pageFormat || ![pageFormat isKindOfClass:[OBString class]])
		return;
	
	if (margins && ![margins isKindOfClass:[OBArray class]])
		return;

	if (!scale || ![scale isKindOfClass:[OBNumber class]])
		return;

	if (!landscape || ![landscape isKindOfClass:[OBNumber class]])
		return;

	if (!backgrounds || ![backgrounds isKindOfClass:[OBNumber class]])
		return;

	WkPrintingProfile *someprofile;
	OBEnumerator *e = [_profiles objectEnumerator];
	
	while ((someprofile = [e nextObject]))
	{
		if ([[someprofile name] isEqualToString:profileName])
		{
			[_profile autorelease];
			_profile = [someprofile retain];
			break;
		}
	}

	if ([_profile canSelectPageFormat])
	{
		WkPrintingPage *somepage;
		e = [[_profile pageFormats] objectEnumerator];
		while ((somepage = [e nextObject]))
		{
			if ([[somepage key] isEqualToString:pageFormat])
			{
				[_profile setSelectedPageFormat:somepage];
				break;
			}
		}
	}

	_scale = [scale floatValue];
	_landscape = [landscape boolValue];
	_printBackgrounds = [backgrounds boolValue];
	
	if (margins && [margins count] == 4)
	{
		OBNumber *margin;
		LONG index = 0;

		_defaultMargins = NO;

		e = [margins objectEnumerator];
		while ((margin = [e nextObject]))
		{
			if ([margin respondsToSelector:@selector(floatValue)])
			{
				switch (index)
				{
				case 0:
					_marginLeft = [margin floatValue];
					break;
				case 1:
					_marginRight = [margin floatValue];
					break;
				case 2:
					_marginTop = [margin floatValue];
					break;
				case 3:
					_marginBottom = [margin floatValue];
					break;
				default: break;
				}
			}
			
			index++;
		}
	}
	else
	{
		_defaultMargins = YES;
		_marginLeft = [[_profile selectedPageFormat] marginLeft];
		_marginTop = [[_profile selectedPageFormat] marginTop];
		_marginRight = [[_profile selectedPageFormat] marginRight];
		_marginBottom = [[_profile selectedPageFormat] marginBottom];
	}

	[self recalculatePages];
}

@end

@implementation WkPrintingState

- (WkWebView *)webView
{
	return nil;
}

- (WkPrintingProfile *)profile
{
	return nil;
}

- (void)setProfile:(WkPrintingProfile *)profile
{
	(void)profile;
}

- (OBArray * /* WkPrintingProfile */)allProfiles
{
	return nil;
}

- (LONG)pages
{
	return 1;
}

- (LONG)sheets
{
	return 1;
}

- (LONG)printJobSheets
{
	return 1;
}

- (WkPrintingRange *)printingRange
{
	return nil;
}

- (void)setPrintingRange:(WkPrintingRange *)range
{
	(void)range;
}

- (LONG)previevedSheet
{
	return 0;
}

- (void)setPrevievedSheet:(LONG)sheet
{
	(void)sheet;
}

- (float)userScalingFactor
{
	return 1.0f;
}

- (void)setUserScalingFactor:(float)scaling
{
	(void)scaling;
}

- (WkPrintingState_Parity)parity
{
	return WkPrintingState_Parity_AllSheets;
}

- (void)setParity:(WkPrintingState_Parity)parity
{
	(void)parity;
}

- (float)marginLeft
{
	return 0.2f;
}

- (float)marginTop
{
	return 0.2f;
}

- (float)marginRight
{
	return 0.2f;
}

- (float)marginBottom
{
	return 0.2f;
}

- (void)setMarginLeft:(float)left top:(float)top right:(float)right bottom:(float)bottom
{
	(void)left;
	(void)right;
	(void)top;
	(void)bottom;
}

- (void)resetMarginsToPaperDefaults
{

}

- (LONG)pagesPerSheet
{
	return 1;
}

- (void)setPagesPerSheet:(LONG)pps
{
	(void)pps;
}

- (void)setLandscape:(BOOL)landscape
{
	(void)landscape;
}

- (BOOL)landscape
{
	return NO;
}

- (BOOL)shouldPrintBackgrounds
{
	return NO;
}

- (void)setShouldPrintBackgrounds:(BOOL)printBackgrounds
{
	(void)printBackgrounds;
}

- (LONG)copies
{
	return 1;
}

- (void)setCopies:(LONG)numCopies
{
	(void)numCopies;
}

- (OBDictionary *)settings
{
	return nil;
}

- (void)setSettings:(OBDictionary *)settings
{
	(void)settings;
}

@end

@interface WkPrintingRangePrivate : WkPrintingRange
{
	LONG _start;
	LONG _end;
}
@end

@implementation WkPrintingRangePrivate

- (id)initWithStart:(LONG)start end:(LONG)end
{
	if ((self = [super init]))
	{
		_start = start;
		_end = end;
	}
	return self;
}

- (LONG)pageStart
{
	return _start;
}

- (LONG)pageEnd
{
	return _end;
}

@end

@implementation WkPrintingRange

+ (WkPrintingRange *)rangeWithPage:(LONG)pageNo
{
	return [[[WkPrintingRangePrivate alloc] initWithStart:pageNo end:pageNo] autorelease];
}

+ (WkPrintingRange *)rangeFromPage:(LONG)pageStart toPage:(LONG)pageEnd
{
	return [[[WkPrintingRangePrivate alloc] initWithStart:pageStart end:pageEnd] autorelease];
}

+ (WkPrintingRange *)rangeFromPage:(LONG)pageStart count:(LONG)count
{
	return [[[WkPrintingRangePrivate alloc] initWithStart:pageStart end:pageStart + count - 1] autorelease];
}

- (LONG)pageStart
{
	return 1;
}

- (LONG)pageEnd
{
	return 1;
}

- (LONG)count
{
	return 1 + ([self pageEnd] - [self pageStart]);
}

@end

