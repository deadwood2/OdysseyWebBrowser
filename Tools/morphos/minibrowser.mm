#import <ob/OBFramework.h>
#import <mui/MUIFramework.h>
#import <mui/PowerTerm_mcc.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <cairo.h>
#include <stdio.h>
#import <proto/muimaster.h>
#import <libraries/asl.h>
#import <clib/alib_protos.h>

#include <signal.h>
#include <locale.h>

#include <string.h>

#include <algorithm>

unsigned long __stack = 2 * 1024 * 1024;

extern "C" {
        void dprintf(const char *fmt, ...);
};

#import <WebKitLegacy/morphos/WkWebView.h>
#import <WebKitLegacy/morphos/WkSettings.h>
#import <WebKitLegacy/morphos/WkCertificateViewer.h>
#import <WebKitLegacy/morphos/WkError.h>
#import <WebKitLegacy/morphos/WkDownload.h>
#import <WebKitLegacy/morphos/WkFileDialog.h>
#import <WebKitLegacy/morphos/WkPrinting.h>
#import <WebKitLegacy/morphos/WkNetworkRequestMutable.h>

@interface BrowserWindow : MUIWindow<WkWebViewClientDelegate, WkWebViewBackForwardListDelegate,
	WkWebViewNetworkProtocolHandlerDelegate, WkWebViewDialogDelegate, WkWebViewAutofillDelegate, WkMutableNetworkRequestTarget>
{
	WkWebView *_view;
 	MUIString *_address;
	MUICycle  *_userAgents;
	MUIGroup  *_topGroup;
	MUIGroup  *_bottomGroup;
	MUIGroup  *_loading;
	MUIButton *_back;
	MUIButton *_forward;
	MUIButton *_stop;
	MUIButton *_reload;
	MUIButton *_certificate;
	MUICheckmark *_adBlock;
	MUICheckmark *_script;
	WkError   *_lastError;
}

- (WkWebView *)webView;

@end

@interface BrowserGroup : MUIGroup<WkWebViewScrollingDelegate>
{
	WkWebView *_view;
	MUIScrollbar *_horiz;
	MUIScrollbar *_vert;
	int           _documentWidth;
	int           _documentHeight;
	int           _viewWidth;
	int           _viewHeight;
	int           _horizStep;
	int           _vertStep;
	bool          _needsUpdate;
}

- (void)doScroll;

@end

@interface BrowserLogWindow : MUIWindow<WkWebViewDebugConsoleDelegate>
{
	WkWebView    *_view;
	MCCPowerTerm *_logview;
	MUIString    *_command;
	MUIButton    *_goButton;
}

+ (id)logWindowForView:(WkWebView *)view;
+ (id)getWindowForView:(WkWebView *)view;

- (WkWebView *)webView;

@end

@interface BrowserDownloadWindow : MUIWindow<WkDownloadDelegate>
{
	MUIList *_downloads;
}
+ (id)sharedInstance;
@end

@implementation BrowserGroup

- (id)initWithWkWebView:(WkWebView *)view
{
	MUIGroup *inner;
	if ((self = [super initHorizontalWithObjects:
		inner = [MUIGroup groupWithObjects:_view = view, _horiz = [MUIScrollbar horizontalScrollbar], nil],
		_vert = [MUIScrollbar verticalScrollbar],
		nil]))
	{
		[inner setInnerLeft:0];
		[inner setInnerRight:0];
		
		[_vert notify:@selector(first) performSelector:@selector(doScroll) withTarget:self];
		[_horiz notify:@selector(first) performSelector:@selector(doScroll) withTarget:self];
		
		[_view setScrollingDelegate:self];
		
		[_vert setDeltaFactor:10];
		[_horiz setDeltaFactor:10];
	}
	
	return self;
}

+ (id)browserGroupWithWkWebView:(WkWebView *)view
{
	return [[[self alloc] initWithWkWebView:view] autorelease];
}

- (void)updateScrollers
{
	_needsUpdate = false;
	
	if (_documentWidth > 0)
	{
		if ([_view isPrinting])
		{
			[_vert noNotifySetEntries:[[_view printingState] sheets]];
			[_vert noNotifySetPropVisible:1];
			[_vert noNotifySetFirst:[[_view printingState] previevedSheet]];

			[_horiz noNotifySetEntries:1];
		}
		else
		{
			if (_documentWidth >= _viewWidth)
			{
				[_horiz noNotifySetEntries:_documentWidth];
				[_horiz noNotifySetPropVisible:_viewWidth];
			}
			else
			{
				[_horiz noNotifySetEntries:1];
			}
			
			if (_documentHeight >= _viewHeight)
			{
				[_vert noNotifySetEntries:_documentHeight];
				[_vert noNotifySetPropVisible:_viewHeight];
			}
			else
				[_vert noNotifySetEntries:1];
		}
	}
	else
	{
		[_vert noNotifySetEntries:1];
		[_horiz noNotifySetEntries:1];
	}
}

- (void)webView:(WkWebView *)view didReceiveResponse:(WkResourceResponse *)response
{

}

- (void)webView:(WkWebView *)view changedContentsSizeToWidth:(int)width height:(int)height
{
	_documentWidth = width;
	_documentHeight = height;
	if (!_needsUpdate)
	{
		_needsUpdate = true;
		[[OBRunLoop mainRunLoop] performSelector:@selector(updateScrollers) target:self];
	}
}

- (void)webView:(WkWebView *)view scrolledToLeft:(int)left top:(int)top
{
	[_horiz noNotifySetFirst:left];
	[_vert noNotifySetFirst:top];
}

- (void)webView:(WkWebView *)view changedContentsSizeToShowPrintingSheets:(int)sheets
{
	_needsUpdate = true;
	[[OBRunLoop mainRunLoop] performSelector:@selector(updateScrollers) target:self];
}

- (void)webView:(WkWebView *)view scrolledToSheet:(int)sheet
{
	[_vert noNotifySetFirst:sheet - 1];
}

- (void)doScroll
{
	if ([_view isPrinting])
	{
		[[_view printingState] setPrevievedSheet:[_vert first] + 1];
	}
	else
	{
		[_view scrollToLeft:[_horiz first] top:[_vert first]];
	}
}

- (BOOL)show:(struct LongRect *)clip
{
	if ([super show:clip])
	{
		_viewWidth = [_view innerWidth];
		_viewHeight = [_view innerHeight];

		if (!_needsUpdate)
		{
			_needsUpdate = true;
			[[OBRunLoop mainRunLoop] performSelector:@selector(updateScrollers) target:self];
		}

		return YES;
	}
	
	return NO;
}

@end


@interface MiniMenuitem : MUIMenuitem

+ (id)itemWithTitle:(OBString *)title shortcut:(OBString *)shortcut selector:(SEL)selector;
+ (id)checkmarkItemWithTitle:(OBString *)title shortcut:(OBString *)shortcut selector:(SEL)selector;

@end

@implementation MiniMenuitem

+ (id)itemWithTitle:(OBString *)title shortcut:(OBString *)shortcut selector:(SEL)selector
{
	return [[[self alloc] initWithTitle:title shortcut:shortcut selector:selector] autorelease];
}

+ (id)checkmarkItemWithTitle:(OBString *)title shortcut:(OBString *)shortcut selector:(SEL)selector
{
	MiniMenuitem *item = [[[self alloc] initWithTitle:title shortcut:shortcut selector:selector] autorelease];
	item.checkit = YES;
	return item;
}

- (void)about
{
	MUIApplication *app = [MUIApplication currentApplication];
	OBArray *windows = [app objects];
	
	for (int i = 0; i < [windows count]; i++)
	{
		MUIWindow *window = [windows objectAtIndex:i];
		if ([window isKindOfClass:[MCCAboutbox class]])
		{
			window.open = YES;
			return;
		}
	}
	
	MCCAboutbox *about = [MCCAboutbox new];
	[app addObject:about];
	about.open = YES;
}

- (void)quit
{
	[[MUIApplication currentApplication] quit];
}

- (MUIWindow *)activeWindow
{
	MUIApplication *app = [MUIApplication currentApplication];
	OBArray *windows = [app objects];
	for (int i = 0; i < [windows count]; i++)
	{
		MUIWindow *window = [windows objectAtIndex:i];
		if ([window activate])
			return window;
	}
	return nil;
}

- (void)debugWindow
{
	BrowserWindow *w = (BrowserWindow *)[self activeWindow];
	if ([w isKindOfClass:[BrowserWindow class]])
	{
		[BrowserLogWindow logWindowForView:[w webView]];
	}
}

- (void)newWindow
{
	BrowserWindow *w = [[BrowserWindow new] autorelease];
	[[MUIApplication currentApplication] addObject:w];
	[w setOpen:YES];
}

@end

@implementation BrowserWindow

static int _windowID = 1;

- (WkWebView *)webView
{
	return _view;
}

- (void)navigate
{
	OBString *urlString = [_address contents];
	OBURL *url = [OBURL URLWithString:urlString];
	dprintf("%s: %s -> %p\n", __PRETTY_FUNCTION__, [urlString cString], url);
	[_view load:url];
}

- (void)postClose
{
	OBEnumerator *e = [[[MUIApplication currentApplication] objects] objectEnumerator];
	MUIWindow *win;

	while ((win = [e nextObject]))
	{
		if ([win isKindOfClass:[self class]] && win != self)
		{
			[[MUIApplication currentApplication] removeObject:self];
			return;
		}
	}

	[[OBRunLoop currentRunLoop] performSelector:@selector(quit) target:[MUIApplication currentApplication]];
}

- (void)doClose
{
	[self setOpen:NO];
	[[OBRunLoop mainRunLoop] performSelector:@selector(postClose) target:self];
}

- (void)navigateTo:(OBString *)to
{
	[_view load:[OBURL URLWithString:to]];
}

- (void)settingsUpdated
{
	WkSettings *settings = [WkSettings settings];
	[settings setJavaScriptEnabled:[_script selected]];
	[settings setAdBlockerEnabled:[_adBlock selected]];
	[_view setSettings:settings];
}

- (void)trustCertificate:(OBArray *)params
{
	WkCertificate *cert = [params firstObject];
	OBURL *url = [params lastObject];
	OBString *certPath = [OBString stringWithFormat:@"T:%@.pem", [url host]];
	[[cert certificate] writeToFile:certPath];
	[WkGlobalSettings setCustomCertificate:certPath forHost:[url host] withKey:nil];
}

- (void)closeCertificate:(MUIWindow *)window
{
	[[window retain] autorelease];
	[window setOpen:NO];
	[[MUIApplication currentApplication] removeObject:window];
}

- (void)showCertificate
{
	WkCertificateChain *cert = _lastError ? [_lastError certificates] : [_view certificateChain];
	MUIWindow *window = [[MUIWindow new] autorelease];
	[window setTitle:[OBString stringWithFormat:@"Certificate for %@", _lastError ? [[_lastError URL] absoluteString] : [[_view URL] absoluteString]]];
	WkCertificateVerifier *cv = [WkCertificateVerifier verifierForCertificateChain:cert];

	// test handling of self-signed certs...
	if (![cert verify:nullptr] && [[cert certificates] count] == 1)
	{
		MUIButton *trust;
		[window setRootObject:[MUIGroup groupWithObjects:cv,
		 	[MUIGroup groupWithObjects:[MUIRectangle rectangleWithWeight:100],
		 		trust = [MUIButton buttonWithLabel:@"Trust this certificate"], nil],
		 	nil]];
		[trust notify:@selector(pressed) trigger:NO performSelector:@selector(trustCertificate:) withTarget:self withObject:
			[OBArray arrayWithObjects:[[cert certificates] lastObject], _lastError ? [_lastError URL] : [_view URL], nil]];
	}
	else
	{
		[window setRootObject:cv];
	}
	[[MUIApplication currentApplication] addObject:window];
	[window setOpen:YES];
	[window notify:@selector(closeRequest) trigger:YES performSelector:@selector(closeCertificate:) withTarget:self withObject:window];
}

- (void)onPrinting
{
	if ([_view isPrinting])
		[_view endPrinting];
	else
		[_view beginPrinting];
}

- (void)onPrintingPDF
{
	WkPrintingState *state = [_view beginPrinting];
	[_view spoolToFile:@"RAM:webkitty.pdf" withDelegate:nil];
}

- (void)onPrintingPaper
{
	WkPrintingState *state = [_view beginPrinting];
	WkPrintingProfile *profile = [state profile];
	if ([profile canSelectPageFormat])
	{
		OBArray *pages = [profile pageFormats];
		ULONG index = [pages indexOfObject:[profile selectedPageFormat]];
		if (OBNotFound == index || ([pages count] - 1 == index))
			[profile setSelectedPageFormat:[pages firstObject]];
		else
			[profile setSelectedPageFormat:[pages objectAtIndex:index + 1]];
	}
}

- (void)onPrintingPage
{
	WkPrintingState *state = [_view beginPrinting];
	if ([state pages] > 1)
	{
		if ([state previevedSheet] + 1 == [state sheets])
			[state setPrevievedSheet:0];
		else
			[state setPrevievedSheet:[state previevedSheet] + 1];
	}
}

- (void)onPrintingLandscape
{
	WkPrintingState *state = [_view beginPrinting];
	[state setLandscape:[state landscape] ? NO : YES];
}

- (void)onPPS:(ULONG)active
{
	WkPrintingState *state = [_view beginPrinting];
	switch (active)
	{
	case 1: [state setPagesPerSheet:2]; break;
	case 2: [state setPagesPerSheet:4]; break;
	case 3: [state setPagesPerSheet:6]; break;
	case 4: [state setPagesPerSheet:9]; break;
	case 0: default: [state setPagesPerSheet:1]; break;
	}
}

- (void)request:(id<WkMutableNetworkRequestHandler>)handler didCompleteWithData:(OBData *)data
{
	dprintf("%s: handler %p data %ld\n", __PRETTY_FUNCTION__, handler, [data length]);
}

- (void)request:(id<WkMutableNetworkRequestHandler>)handler didFailWithError:(WkError *)error data:(OBData *)data
{
	dprintf("%s: handler %p error %d data %ld\n", __PRETTY_FUNCTION__, handler, [error errorCode], [data length]);
}

- (void)doRequest
{
	WkMutableNetworkRequest *request = [WkMutableNetworkRequest requestWithURL:[OBURL URLWithString:@"https://morph.zone/news/"]];
	[WkMutableNetworkRequest performRequest:request withTarget:self];
}

- (id)initWithView:(WkWebView *)view
{
	if ((self = [super init]))
	{
		MUIButton *button;
		MUIButton *debug;
		MUIButton *print,*printNextPaper, *printNextPage, *printPDF, *printLandscape, *request;
		MUICycle *pps;

		self.rootObject = [MUIGroup groupWithObjects:
			_topGroup = [MUIGroup horizontalGroupWithObjects:
				_back = [MUIButton buttonWithLabel:@"\33I[5:PROGDIR:MiniResources/icons8-go-back-20.png]"],
				_forward = [MUIButton buttonWithLabel:@"\33I[5:PROGDIR:MiniResources/icons8-circled-right-20.png]"],
				_stop = [MUIButton buttonWithLabel:@"\33I[5:PROGDIR:MiniResources/icons8-no-entry-20.png]"],
				_reload = [MUIButton buttonWithLabel:@"\33I[5:PROGDIR:MiniResources/icons8-restart-20.png]"],
				_certificate = [MUIButton buttonWithLabel:@"\33I[5:PROGDIR:MiniResources/icons8-certificate-20.png]"],
				_address = [MUIString stringWithContents:@"https://"],
				nil],
			[BrowserGroup browserGroupWithWkWebView:_view = view],
			_bottomGroup = [MUIGroup horizontalGroupWithObjects:
				_userAgents = [MUICycle cycleWithEntries:[OBArray arrayWithObjects:@"Safari", @"Chrome", @"WebKitty", @"iPad 12.2", @"IE10", nil]],
				debug = [MUIButton buttonWithLabel:@"Debug Stats"],
				[MUICheckmark checkmarkWithLabel:@"AdBlocker" checkmark:&_adBlock],
				[MUICheckmark checkmarkWithLabel:@"JS" checkmark:&_script],
				print = [MUIButton buttonWithLabel:@"Printing"],
				printNextPaper = [MUIButton buttonWithLabel:@"Paper >>"],
				printNextPage = [MUIButton buttonWithLabel:@"Page >> "],
				printLandscape = [MUIButton buttonWithLabel:@"Landscape"],
				printPDF = [MUIButton buttonWithLabel:@"PDF"],
				pps = [MUICycle cycleWithEntryList:@"1", @"2", @"4", @"6", @"9", nil],
				request = [MUIButton buttonWithLabel:@"HTTP Request"],
				[MUIRectangle rectangleWithWeight:300],
				_loading = [MUIGroup groupWithPages:[MUIRectangle rectangleWithWeight:20], [[MCCBusy new] autorelease], nil],
				nil],
			nil];
		self.title = @"Test";
		self.iD = _windowID++;

		[self notify:@selector(closeRequest) performSelector:@selector(doClose) withTarget:self];

		[_address notify:@selector(acknowledge) performSelector:@selector(navigate) withTarget:self];
		[_address setWeight:300];
		[_address setMaxLen:4000];
		[_address setCycleChain:YES];
		
		[_view setClientDelegate:self];
		[_view setBackForwardListDelegate:self];
		[_view setCustomProtocolHandler:self forProtocol:@"mini"];
		[_view setDownloadDelegate:[BrowserDownloadWindow sharedInstance]];
		[_view setDialogDelegate:self];

// annoying :)
//		[_view setAutofillDelegate:self];

		[_back notify:@selector(pressed) trigger:NO performSelector:@selector(goBack) withTarget:_view];
		[_forward notify:@selector(pressed) trigger:NO performSelector:@selector(goForward) withTarget:_view];
		[_stop notify:@selector(pressed) trigger:NO performSelector:@selector(stopLoading) withTarget:_view];
		[_reload notify:@selector(pressed) trigger:NO performSelector:@selector(reload) withTarget:_view];
		
		[_back setHorizWeight:0];
		[_forward setHorizWeight:0];
		[_stop setHorizWeight:0];
		[_reload setHorizWeight:0];
		[_certificate setHorizWeight:0];
		
		[_adBlock setSelected:YES];
		[_script setSelected:YES];
		
		[_adBlock notify:@selector(selected) performSelector:@selector(settingsUpdated) withTarget:self];
		[_script notify:@selector(selected) performSelector:@selector(settingsUpdated) withTarget:self];

		[_back setDisabled:YES];
		[_forward setDisabled:YES];

		[_certificate notify:@selector(selected) trigger:NO performSelector:@selector(showCertificate) withTarget:self];
		
		[print notify:@selector(selected) trigger:NO performSelector:@selector(onPrinting) withTarget:self];
		[printNextPaper notify:@selector(selected) trigger:NO performSelector:@selector(onPrintingPaper) withTarget:self];
		[printNextPage notify:@selector(selected) trigger:NO performSelector:@selector(onPrintingPage) withTarget:self];
		[printPDF notify:@selector(selected) trigger:NO performSelector:@selector(onPrintingPDF) withTarget:self];
		[printLandscape notify:@selector(selected) trigger:NO performSelector:@selector(onPrintingLandscape) withTarget:self];
		[pps notify:@selector(active) performSelector:@selector(onPPS:) withRawTriggerValueTarget:self];
		[request notify:@selector(selected) trigger:NO performSelector:@selector(doRequest) withTarget:self];

		#define ADDBUTTON(__title__, __address__) \
			[_topGroup addObject:button = [MUIButton buttonWithLabel:__title__]]; \
			[button notify:@selector(pressed) trigger:NO performSelector:@selector(navigateTo:) withTarget:self withObject:__address__];

		ADDBUTTON(@"Scroll", @"http://saku.bbs.fi/cgi-bin/discus/show.cgi?tpc=2593&post=54149");
		ADDBUTTON(@"Print", @"http://tunkki.dk/~jaca/texttest.htm");
		ADDBUTTON(@"Zone", @"https://morph.zone");
		ADDBUTTON(@"Ggle", @"https://www.google.com");
		ADDBUTTON(@"GGlogin", @"https://accounts.google.com/");
		ADDBUTTON(@"ReCaptcha", @"https://patrickhlauke.github.io/recaptcha/");
		ADDBUTTON(@"HTML5", @"http://html5test.com");

		[debug notify:@selector(pressed) trigger:NO performSelector:@selector(dumpDebug) withTarget:_view];

		[self setMenustrip:[MUIMenustrip menustripWithObjects:
			[MUIMenu menuWithTitle:@"MiniBrowser" objects:
				[MiniMenuitem itemWithTitle:@"New Window..." shortcut:@"N" selector:@selector(newWindow)],
				[MiniMenuitem itemWithTitle:@"Console" shortcut:@"D" selector:@selector(debugWindow)],
				[MUIMenuitem barItem],
				[MiniMenuitem itemWithTitle:@"About..." shortcut:@"?" selector:@selector(about)],
				[MiniMenuitem itemWithTitle:@"Quit" shortcut:@"Q" selector:@selector(quit)],
				nil],
			nil]];
	
	}
	
	return self;
}

- (id)init
{
	return [self initWithView:[[WkWebView new] autorelease]];
}

- (void)dealloc
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
	[_view setClientDelegate:nil];
	[_view setScrollingDelegate:nil];
	[_view setDownloadDelegate:nil];
	[_view setAutofillDelegate:nil];
	[_view setCustomProtocolHandler:nil forProtocol:@"mini"];
	[_lastError release];
	[super dealloc];
}

- (OBString *)userAgentForURL:(OBString *)url
{
	switch ([_userAgents active])
	{
	case 1:
		return @"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_6) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.100 Safari/537.36";
	case 2:
		return @"Mozilla/5.0 (MorphOS; PowerPC 3_14) WebKitty/605.1.15 (KHTML, like Gecko)";
	case 3:
		return @"Mozilla/5.0 (iPad; CPU OS 12_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/12.1 Mobile/15E148 Safari/604.1";
	case 4:
		return @"Mozilla/5.0 (compatible; MSIE 10.0; Windows NT 6.2)";
	}

	return nil;
}

- (void)webView:(WkWebView *)view changedTitle:(OBString *)newtitle
{
	[self setTitle:newtitle];
}

- (void)webView:(WkWebView *)view changedDocumentURL:(OBURL *)newurl
{
	[_address noNotifySetContents:[newurl absoluteString]];
	[_lastError release];
	_lastError = nil;
}

- (void)webView:(WkWebView *)view changedFavIcon:(WkFavIcon *)favicon
{

}

- (BOOL)webView:(WkWebView *)view shouldLoadFavIconForURL:(OBURL *)url
{
	return NO;
}

- (void)webView:(WkWebView *)view changedHoveredURL:(OBURL *)hoveredURL
{

}

- (void)webView:(WkWebView *)view documentReady:(BOOL)ready
{
	[_loading setActivePage:!ready];
}

- (BOOL)webView:(WkWebView *)view wantsToCreateNewViewWithURL:(OBURL *)url options:(OBDictionary *)options
{
	return YES;
}

- (void)webViewDidLoadInsecureContent:(WkWebView *)view
{
}

- (void)webView:(WkWebView *)view createdNewWebView:(WkWebView *)newview
{
	BrowserWindow *newWindow = [[[BrowserWindow alloc] initWithView:newview] autorelease];
	[[MUIApplication currentApplication] addObject:newWindow];
	[newWindow setOpen:YES];
}

- (void)webViewChangedBackForwardList:(WkWebView *)view
{
	[_back setDisabled:![view canGoBack]];
	[_forward setDisabled:![_view canGoForward]];
}

- (void)webView:(WkWebView *)view didFailLoadingWithError:(WkError *)error
{
	[_lastError release];
	_lastError = [error retain];

	switch ([error type])
	{
	case WkErrorType_Null:
		break;
	case WkErrorType_General:
		[_view setHTML:[OBString stringWithFormat:@"<html><body><center>Error: %d<br>loading %@<br>%@</center></body></html>", [error errorCode], [[error URL] host], [error localizedDescription]]];
		break;
	case WkErrorType_Timeout:
		[_view setHTML:[OBString stringWithFormat:@"<html><body><center>Timeout loading %@<br>%@</center></body></html>", [[error URL] host], [error localizedDescription]]];
		break;
	case WkErrorType_Cancellation:
		break;
	case WkErrorType_AccessControl:
		[_view setHTML:[OBString stringWithFormat:@"<html><body><center>Access Controls Error: %d<br>loading %@<br>%@</center></body></html>", [error errorCode], [[error URL] host], [error localizedDescription]]];
		break;
	case WkErrorType_SSLConnection:
		[_view setHTML:[OBString stringWithFormat:@"<html><body><center>SSL Connection Error: %d<br>loading %@<br>%@</center></body></html>", [error errorCode], [[error URL] host], [error localizedDescription]]];
		break;
	case WkErrorType_SSLCertification:
		[_view setHTML:[OBString stringWithFormat:@"<html><body><center>Server %@ presented an untrusted certificate<br><a href=\"mini:showcertificate\">Show Certificate</a>&nbsp;<a href=\"mini:back\">Go Back</a></center></body></html>", [[error URL] host]]];
		break;
	}
}

- (void)ignoreDelegate:(id<WkConfirmDownloadResponseDelegate>)delegate
{
	dprintf("ignore delegate!\n");
	[delegate ignore];
}

- (void)webView:(WkWebView *)view confirmDownloadOfURL:(OBURL *)url mimeType:(OBString *)mime size:(QUAD)size withSuggestedName:(OBString *)suggestedName withResponseDelegate:(id<WkConfirmDownloadResponseDelegate>)delegate
{
	dprintf("%s: url %s mime %s name '%s' delegate %p\n", __PRETTY_FUNCTION__, [[url absoluteString] cString], [mime cString], [suggestedName cString], delegate);
//	[[OBRunLoop mainRunLoop] performSelector:@selector(ignore) target:delegate];
	[OBScheduledTimer scheduledTimerWithInterval:5.0 perform:[OBPerform performSelector:@selector(ignoreDelegate:) target:self withObject:delegate] repeats:NO];
}

- (void)webView:(WkWebView *)view wantsToNavigateToCustomProtocol:(OBString *)protocol withArguments:(OBString *)arguments
{
	if ([arguments isEqualToString:@"showcertificate"])
	{
		if (_lastError)
		{
			[self showCertificate];
		}
	}
	else if ([arguments isEqualToString:@"back"])
	{
	
	}
}

- (void)webView:(WkWebView *)view issuedAuthenticationChallengeAtURL:(OBURL *)url withResponseDelegate:(id<WkAuthenticationChallengeResponseDelegate>)delegate
{
	dprintf("issuedAuthenticationChallengeAtURL...will cancel\n");
	[delegate cancel];
}

- (void)webViewRequestedPrinting:(WkWebView *)view
{
}

- (OBString *)aslFile:(OBString *)oldpath title:(OBString *)title doSave:(BOOL)save reference:(MUIArea *)ref
{
        APTR requester = MUI_AllocAslRequest(ASL_FileRequest, NULL);
        OBString *path = nil;

        if (MUI_AslRequestTags(requester,
                ASLFR_Window, [ref window],
                ASLFR_TitleText, [title nativeCString],
                ASLFR_DoSaveMode, save,
                oldpath ? ASLFR_InitialDrawer : TAG_IGNORE, [[oldpath pathPart] nativeCString],
                oldpath ? ASLFR_InitialFile : TAG_IGNORE, [[oldpath filePart] nativeCString],
                TAG_DONE))
        {
                struct FileRequester *aslfr = (struct FileRequester *)requester;
                OBString *drawer = [OBString stringWithCString:aslfr->fr_Drawer encoding:MIBENUM_SYSTEM];
                OBString *fileName = [OBString stringWithCString:aslfr->fr_File encoding:MIBENUM_SYSTEM];
                path = drawer;
                if (path)
                        path = [path stringByAddingPathComponent:fileName];
                else
                        path = fileName;
                path = [path absolutePath];

                if (path && save)
                {
                        BPTR lock = Lock([path nativeCString], ACCESS_READ);
                        if (lock)
                        {
                                UnLock(lock);

                                if (0 == [MUIRequest request:NULL title:@"JSPopup" message:[OBString stringWithFormat:OBL(@"File %@ already exists!", @"File overwrite requester"), fileName]
                                        buttons:[OBArray arrayWithObjects:OBL(@"Overwrite", @"File overwrite requester button"), @"Cancel", nil]])
                                        return nil;
                        }
                }
        }

        MUI_FreeAslRequest(requester);

        return path;
}

- (OBArray *)aslFiles:(OBString *)oldpath title:(OBString *)title reference:(MUIArea *)ref
{
        APTR requester = MUI_AllocAslRequest(ASL_FileRequest, NULL);

        if (MUI_AslRequestTags(requester,
                ASLFR_Window, [ref window],
                ASLFR_TitleText, [title nativeCString],
                ASLFR_DoMultiSelect, YES,
                oldpath ? ASLFR_InitialDrawer : TAG_IGNORE, [[oldpath pathPart] nativeCString],
                oldpath ? ASLFR_InitialFile : TAG_IGNORE, [[oldpath filePart] nativeCString],
                TAG_DONE))
        {
                struct FileRequester *aslfr = (struct FileRequester *)requester;
                OBString *drawer = [OBString stringWithCString:aslfr->fr_Drawer encoding:MIBENUM_SYSTEM];
                OBMutableArray *out = [OBMutableArray arrayWithCapacity:aslfr->fr_NumArgs];

                for (LONG i = 0; i < aslfr->fr_NumArgs; i++)
                {
                        OBString *fileName = [OBString stringWithCString:(const char *)aslfr->fr_ArgList[i].wa_Name encoding:MIBENUM_SYSTEM];
                        OBString *path = nil;

                        path = drawer;
                        if (path)
                                path = [path stringByAddingPathComponent:fileName];
                        else
                                path = fileName;
                        path = [path absolutePath];
                        [out addObject:path];
                }

                return out;
        }

        MUI_FreeAslRequest(requester);

        return nil;
}

- (void)webView:(WkWebView *)view wantsToOpenFileSelectionPanelWithSettings:(id<WkFileDialogSettings>)settings responseHandler:(id<WkFileDialogResponseHandler>)handler
{
	if ([settings allowsMultipleFiles])
	{
		OBArray *files = [self aslFiles:@"" title:@"JSPopup" reference:[self rootObject]];
		[handler selectedFiles:files];
	}
	else
	{
		OBString *file = [self aslFile:@"" title:@"JSPopup" doSave:NO reference:[self rootObject]];
		[handler selectedFile:file];
	}
}

- (void)webView:(WkWebView *)view wantsToShowJavaScriptAlertWithMessage:(OBString *)message
{
	[MUIRequest request:self title:@"JSAlert" message:message buttons:[OBArray arrayWithObject:@"OK"]];
}

- (BOOL)webView:(WkWebView *)view wantsToShowJavaScriptConfirmPanelWithMessage:(OBString *)message
{
	if (1 == [MUIRequest request:self title:@"JSAlert" message:message buttons:[OBArray arrayWithObjects:@"OK", @"Cancel", nil]])
		return YES;
	return NO;
}

- (OBString *)webView:(WkWebView *)view wantsToShowJavaScriptPromptPanelWithMessage:(OBString *)message defaultValue:(OBString *)defaultValue
{
	return defaultValue;
}

- (void)webView:(WkWebView *)view willSubmitFormWithLogin:(OBString *)login password:(OBString *)password atURL:(OBURL *)url
{
	dprintf(">> WillSubmitForm @ %s (%s, %s)\n", [[url absoluteString] cString], [login cString], [password cString]);
}

- (void)webView:(WkWebView *)view selectedAutofillFieldAtURL:(OBURL *)url withPrefilledLogin:(OBString *)login
{
	MUIString *l, *p;
	MUIGroup *g = [[MUIGroup groupWithObjects:l = [MUIString stringWithContents:login], p = [MUIString string], nil] retain];
	if (1 == [MUIRequest request:self title:@"Input Credentials" message:@"Input login credentials" buttons:[OBArray arrayWithObjects:@"OK", @"Cancel", nil] object:g])
	{
		[view autofillElementsWithLogin:[l contents] password:[p contents]];
	}
	
	[g release];
}

- (void)webView:(WkWebView *)view didReceiveResponse:(WkResourceResponse *)response
{

}

@end

@implementation BrowserLogWindow

- (void)go
{
	OBString *exec = [_command contents];

	OBString *log = [OBString stringWithFormat:@">> %@\r\n", exec];
	const char *clog = [log cString];
	[_logview writeUnicode:(APTR)clog length:strlen(clog) format:MUIV_PowerTerm_WriteUnicode_UTF8];

	OBString *res = [_view evaluateJavaScript:exec];
	log = [OBString stringWithFormat:@"<< %@\r\n", res];
	clog = [log cString];
	[_logview writeUnicode:(APTR)clog length:strlen(clog) format:MUIV_PowerTerm_WriteUnicode_UTF8];
}

- (id)initWithView:(WkWebView *)view
{
	if ((self = [super init]))
	{
		MUIScrollbar *sc;
		_view = [view retain];
		[self setTitle:@"Console"];
		[self setRootObject:[MUIGroup groupWithObjects:
			[MUIGroup horizontalGroupWithObjects:_logview = [[MCCPowerTerm new] autorelease], sc = [MUIScrollbar verticalScrollbar], nil],
			[MUIGroup horizontalGroupWithObjects:_command = [MUIString string], _goButton = [MUIButton buttonWithLabel:@"Run"], nil],
			nil]];
		[_command setCycleChain:YES];
		[_command setMaxLen:2048];
		[_logview setUTFEnable:YES];
		[_logview setWrap:YES];
		[_logview setResizable:YES];
		[_logview setScroller:sc];
		[_logview setResizableHistory:YES];
		[_goButton notify:@selector(pressed) trigger:NO performSelector:@selector(go) withTarget:self];
		[_command notify:@selector(acknowledge) performSelector:@selector(go) withTarget:self];
		[_view setDebugConsoleDelegate:self];
	}
	return self;
}

- (void)dealloc
{
	[_view setDebugConsoleDelegate:nil];
	[_view release];
	[super dealloc];
}

+ (id)logWindowForView:(WkWebView *)view
{
	id win = [self getWindowForView:view];
	if (nil == win)
	{
		win = [[[self alloc] initWithView:view] autorelease];
		if (win)
		{
			[[MUIApplication currentApplication] addObject:win];
			[win setOpen:YES];
		}
	}
	return win;
}

+ (id)getWindowForView:(WkWebView *)view
{
	OBEnumerator *e = [[[MUIApplication currentApplication] objects] objectEnumerator];
	MUIWindow *w;
	while ((w = [e nextObject]))
	{
		if ([w isKindOfClass:[self class]])
		{
			BrowserLogWindow *bw = (id)w;
			if ([bw webView] == view)
			{
				[bw toFront];
				return bw;
			}
		}
	}
	
	return nil;
}

- (WkWebView *)webView
{
	return _view;
}

- (void)setCloseRequest:(BOOL)closerequest
{
	[[self retain] autorelease];
	[self setOpen:NO];
	[[MUIApplication currentApplication] removeObject:self];
}

- (void)webView:(WkWebView *)view outputConsoleMessage:(OBString *)message level:(WkWebViewDebugConsoleLogLevel)level atLine:(ULONG)lineno
{
	OBString *lstring = @"?";
	switch (level)
	{
	case WkWebViewDebugConsoleLogLevel_Info: lstring = @"INF"; break;
	case WkWebViewDebugConsoleLogLevel_Log: lstring = @"LOG"; break;
	case WkWebViewDebugConsoleLogLevel_Debug: lstring = @"DBG"; break;
	case WkWebViewDebugConsoleLogLevel_Error: lstring = @"ERR"; break;
	case WkWebViewDebugConsoleLogLevel_Warning: lstring = @"WARN"; break;
	}
	OBString *log = [OBString stringWithFormat:@"[%@] %@; at line %d\r\n", lstring, message, lineno];
	const char *clog = [log cString];
	[_logview writeUnicode:(APTR)clog length:strlen(clog) format:MUIV_PowerTerm_WriteUnicode_UTF8];
}

@end

@interface WkDownload (MUIList) <MUIListEntry>
@end

@implementation WkDownload (MUIList)

- (OBArray *)listDisplay
{
	return [OBArray arrayWithObjects:
		[[self url] absoluteString],
		[self filename],
		[OBString stringWithFormat:@"%ld / %ld", [self downloadedSize], [self size]],
		nil];
}

@end

@implementation BrowserDownloadWindow

static BrowserDownloadWindow *_instance;

- (void)onCancel
{
	WkDownload *dl = [_downloads objectAtIndex:[_downloads active]];
	if (dl)
	{
		dprintf("%s\n", __PRETTY_FUNCTION__);
		[dl cancelForResume];
	}
}

- (id)init
{
	if ((self = [super init]))
	{
		MUIButton *cancel;
		[self setTitle:@"Downloads"];
		[self setRootObject:[MUIGroup groupWithObjects:
			_downloads = [[MUIList new] autorelease],
			[MUIGroup horizontalGroupWithObjects:
				[MUIRectangle rectangleWithWeight:200],
				cancel = [MUIButton buttonWithLabel:@"Cancel"],
			 	nil],
			nil]];
		
		_downloads.title = YES;
		_downloads.titles = [OBArray arrayWithObjects:@"URL", @"File", @"Progress", nil];
		_downloads.format = @"BAR,BAR,";
		
		[self setID:200];
		[cancel notify:@selector(pressed) trigger:NO performSelector:@selector(onCancel) withTarget:self];
	}
	
	return self;
}

- (void)dealloc
{
	_instance = nil;
	[super dealloc];
}

+ (id)sharedInstance
{
	if (nil == _instance)
		_instance = [BrowserDownloadWindow new];
	return _instance;
}

- (void)downloadDidBegin:(WkDownload *)download
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
	[_downloads addObject:download];
	[self setOpen:YES];
}

- (void)download:(WkDownload *)download didRedirect:(OBURL *)newURL
{

}

- (void)didReceiveResponse:(WkDownload *)download
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
	[_downloads redraw:[_downloads indexOfObject:download]];
}

- (OBString *)decideFilenameForDownload:(WkDownload *)download withSuggestedName:(OBString *)suggestedName
{
	dprintf("%s: %s\n", __PRETTY_FUNCTION__, [suggestedName cString]);
	return [@"SYS:Downloads" stringByAddingPathComponent:suggestedName];
}

- (void)download:(WkDownload *)download didReceiveBytes:(size_t)bytes
{
//	dprintf("%s\n", __PRETTY_FUNCTION__);
	[_downloads redraw:[_downloads indexOfObject:download]];
}

- (void)downloadDidFinish:(WkDownload *)download
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
	// [_downloads removeObject:download];
	[_downloads redraw:[_downloads indexOfObject:download]];
}

- (void)download:(WkDownload *)download didFailWithError:(WkError *)error
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
	[_downloads redraw:[_downloads indexOfObject:download]];
}

- (void)downloadNeedsAuthenticationCredentials:(WkDownload *)download
{
}

@end

@interface MiniAppDelegate : OBObject<OBApplicationDelegate>
@end

@implementation MiniAppDelegate

- (void)dealloc
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
	[super dealloc];
}

- (void)applicationWillRun
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
}

- (BOOL)applicationShouldTerminate
{
	OBArray *allWindows = [[MUIApplication currentApplication] objects];
	OBEnumerator *e = [allWindows objectEnumerator];
	MUIWindow *w;
	
	while ((w = [e nextObject]))
	{
		[w setOpen:NO];
	}
	
	[[MUIApplication currentApplication] removeAllObjects];

	BOOL shouldTerminate = [WkWebView readyToQuit];
	dprintf("%s %d\n", __PRETTY_FUNCTION__, shouldTerminate);
	return shouldTerminate;
}

- (void)applicationDidTerminate
{
	dprintf("%s\n", __PRETTY_FUNCTION__);
}

@end

#define VERSION   "$VER: MiniBrowser 1.0 (19.04.2020) (C)2020 Jacek Piszczek, Harry Sintonen / MorphOS Team"
#define COPYRIGHT @"2020 Jacek Piszczek, Harry Sintonen / MorphOS Team"

__attribute__ ((visibility ("default")))
int muiMain(int argc, char *argv[])
{
    signal(SIGINT, SIG_IGN);
	setlocale(LC_TIME, "C");
	setlocale(LC_NUMERIC, "C");
	setlocale(LC_CTYPE, "en-US");

	MUIApplication *app = [MUIApplication new];

	if (app)
	{
		MiniAppDelegate *delegate = [MiniAppDelegate new];
		[app setBase:@"WEKBITTY"];

		app.title = @"WebKitty MiniBrowser";
		app.author = @"Jacek Piszczek, Harry Sintonen";
		app.copyright = COPYRIGHT;
		app.applicationVersion = [OBString stringWithCString:VERSION encoding:MIBENUM_ISO_8859_1];
		[[OBApplication currentApplication] setDelegate:delegate];

		MUIWindow *win = [[BrowserWindow new] autorelease];
		MUIWindow *dlwin = [BrowserDownloadWindow sharedInstance];
		[app instantiateWithWindows:win, dlwin, nil];

		win.open = YES;

		[app run];
		
		dprintf("%s: runloop exited!\n", __PRETTY_FUNCTION__);
		
		[[OBApplication currentApplication] setDelegate:nil];
		[delegate release];
	}
	
dprintf("release..\n");
	[[BrowserDownloadWindow sharedInstance] release];
	[app release];

dprintf("destructors be called next\n");
	return 0;
}
