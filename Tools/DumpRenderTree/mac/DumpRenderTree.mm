/*
 * Copyright (C) 2005-2009, 2015 Apple Inc. All rights reserved.
 *           (C) 2007 Graham Dennis (graham.dennis@gmail.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "DumpRenderTree.h"

#import "AccessibilityController.h"
#import "CheckedMalloc.h"
#import "DefaultPolicyDelegate.h"
#import "DumpRenderTreeDraggingInfo.h"
#import "DumpRenderTreePasteboard.h"
#import "DumpRenderTreeWindow.h"
#import "EditingDelegate.h"
#import "EventSendingController.h"
#import "FrameLoadDelegate.h"
#import "HistoryDelegate.h"
#import "JavaScriptThreading.h"
#import "MockGeolocationProvider.h"
#import "MockWebNotificationProvider.h"
#import "NavigationController.h"
#import "ObjCPlugin.h"
#import "ObjCPluginFunction.h"
#import "PixelDumpSupport.h"
#import "PolicyDelegate.h"
#import "ResourceLoadDelegate.h"
#import "TestRunner.h"
#import "UIDelegate.h"
#import "WebArchiveDumpSupport.h"
#import "WebCoreTestSupport.h"
#import "WorkQueue.h"
#import "WorkQueueItem.h"
#import <CoreFoundation/CoreFoundation.h>
#import <JavaScriptCore/HeapStatistics.h>
#import <JavaScriptCore/Options.h>
#import <WebKit/DOMElement.h>
#import <WebKit/DOMExtensions.h>
#import <WebKit/DOMRange.h>
#import <WebKit/WebArchive.h>
#import <WebKit/WebBackForwardList.h>
#import <WebKit/WebCache.h>
#import <WebKit/WebCoreStatistics.h>
#import <WebKit/WebDataSourcePrivate.h>
#import <WebKit/WebDatabaseManagerPrivate.h>
#import <WebKit/WebDeviceOrientationProviderMock.h>
#import <WebKit/WebDocumentPrivate.h>
#import <WebKit/WebEditingDelegate.h>
#import <WebKit/WebFrameView.h>
#import <WebKit/WebHistory.h>
#import <WebKit/WebHistoryItemPrivate.h>
#import <WebKit/WebInspector.h>
#import <WebKit/WebKitNSStringExtras.h>
#import <WebKit/WebPluginDatabase.h>
#import <WebKit/WebPreferenceKeysPrivate.h>
#import <WebKit/WebPreferences.h>
#import <WebKit/WebPreferencesPrivate.h>
#import <WebKit/WebResourceLoadDelegate.h>
#import <WebKit/WebStorageManagerPrivate.h>
#import <WebKit/WebViewPrivate.h>
#import <WebKitSystemInterface.h>
#import <getopt.h>
#import <wtf/Assertions.h>
#import <wtf/FastMalloc.h>
#import <wtf/ObjcRuntimeExtras.h>
#import <wtf/RetainPtr.h>
#import <wtf/Threading.h>
#import <wtf/text/WTFString.h>

#if !PLATFORM(IOS)
#import <Carbon/Carbon.h>
#import <WebKit/WebDynamicScrollBarsView.h>
#endif

#if PLATFORM(IOS)
#import "DumpRenderTreeBrowserView.h"
#import "UIKitSPI.h"
#import <QuartzCore/QuartzCore.h>
#import <WebCore/CoreGraphicsSPI.h>
#import <WebKit/WAKWindow.h>
#import <WebKit/WebCoreThread.h>
#import <WebKit/WebCoreThreadRun.h>
#import <WebKit/WebDOMOperations.h>
#import <fcntl.h>
#endif

extern "C" {
#import <mach-o/getsect.h>
}

using namespace std;

#if !PLATFORM(IOS)
@interface DumpRenderTreeApplication : NSApplication
@end

@interface DumpRenderTreeEvent : NSEvent
@end
#else
@interface ScrollViewResizerDelegate : NSObject
@end

@implementation ScrollViewResizerDelegate
- (void)view:(UIWebDocumentView *)view didSetFrame:(CGRect)newFrame oldFrame:(CGRect)oldFrame asResultOfZoom:(BOOL)wasResultOfZoom
{
    UIView *scrollView = [view superview];
    while (![scrollView isKindOfClass:[UIWebScrollView class]])
        scrollView = [scrollView superview];

    ASSERT(scrollView && [scrollView isKindOfClass:[UIWebScrollView class]]);
    const CGSize scrollViewSize = [scrollView bounds].size;
    CGSize contentSize = newFrame.size;
    contentSize.height = CGRound(max(CGRectGetMaxY(newFrame), scrollViewSize.height));
    [(UIWebScrollView *)scrollView setContentSize:contentSize];
}
@end
#endif

@interface NSURLRequest (PrivateThingsWeShouldntReallyUse)
+(void)setAllowsAnyHTTPSCertificate:(BOOL)allow forHost:(NSString *)host;
@end

#if USE(APPKIT)
@interface NSSound ()
+ (void)_setAlertType:(NSUInteger)alertType;
@end
#endif

@interface WebView ()
- (BOOL)_flushCompositingChanges;
@end

#if !PLATFORM(IOS)
@interface WebView (WebViewInternalForTesting)
- (WebCore::Frame*)_mainCoreFrame;
@end
#endif

static void runTest(const string& testURL);

// Deciding when it's OK to dump out the state is a bit tricky.  All these must be true:
// - There is no load in progress
// - There is no work queued up (see workQueue var, below)
// - waitToDump==NO.  This means either waitUntilDone was never called, or it was called
//       and notifyDone was called subsequently.
// Note that the call to notifyDone and the end of the load can happen in either order.

volatile bool done;

NavigationController* gNavigationController = 0;
RefPtr<TestRunner> gTestRunner;

WebFrame *mainFrame = 0;
// This is the topmost frame that is loading, during a given load, or nil when no load is 
// in progress.  Usually this is the same as the main frame, but not always.  In the case
// where a frameset is loaded, and then new content is loaded into one of the child frames,
// that child frame is the "topmost frame that is loading".
WebFrame *topLoadingFrame = nil;     // !nil iff a load is in progress


CFMutableSetRef disallowedURLs = 0;
static CFRunLoopTimerRef waitToDumpWatchdog = 0;

// Delegates
static FrameLoadDelegate *frameLoadDelegate;
static UIDelegate *uiDelegate;
static EditingDelegate *editingDelegate;
static ResourceLoadDelegate *resourceLoadDelegate;
static HistoryDelegate *historyDelegate;
PolicyDelegate *policyDelegate;
DefaultPolicyDelegate *defaultPolicyDelegate;
#if PLATFORM(IOS)
static ScrollViewResizerDelegate *scrollViewResizerDelegate;
#endif

static int dumpPixelsForAllTests = NO;
static bool dumpPixelsForCurrentTest = false;
static int threaded;
static int dumpTree = YES;
static int useTimeoutWatchdog = YES;
static int forceComplexText;
static int useAcceleratedDrawing;
static int gcBetweenTests;
static int showWebView = NO;
static BOOL printSeparators;
static RetainPtr<CFStringRef> persistentUserStyleSheetLocation;
static std::set<std::string> allowedHosts;

static WebHistoryItem *prevTestBFItem = nil;  // current b/f item at the end of the previous test

#if PLATFORM(IOS)
const unsigned phoneViewHeight = 480;
const unsigned phoneViewWidth = 320;
const unsigned phoneBrowserScrollViewHeight = 416;
const unsigned phoneBrowserAddressBarOffset = 60;
const CGRect layoutTestViewportRect = { {0, 0}, {static_cast<CGFloat>(TestRunner::viewWidth), static_cast<CGFloat>(TestRunner::viewHeight)} };
UIWebBrowserView *gWebBrowserView = nil;
UIWebScrollView *gWebScrollView = nil;
DumpRenderTreeWindow *gDrtWindow = nil;
#endif

#ifdef __OBJC2__
static void swizzleAllMethods(Class imposter, Class original)
{
    unsigned int imposterMethodCount;
    Method* imposterMethods = class_copyMethodList(imposter, &imposterMethodCount);

    unsigned int originalMethodCount;
    Method* originalMethods = class_copyMethodList(original, &originalMethodCount);

    for (unsigned int i = 0; i < imposterMethodCount; i++) {
        SEL imposterMethodName = method_getName(imposterMethods[i]);

        // Attempt to add the method to the original class.  If it fails, the method already exists and we should
        // instead exchange the implementations.
        if (class_addMethod(original, imposterMethodName, method_getImplementation(imposterMethods[i]), method_getTypeEncoding(imposterMethods[i])))
            continue;

        unsigned int j = 0;
        for (; j < originalMethodCount; j++) {
            SEL originalMethodName = method_getName(originalMethods[j]);
            if (sel_isEqual(imposterMethodName, originalMethodName))
                break;
        }

        // If class_addMethod failed above then the method must exist on the original class.
        ASSERT(j < originalMethodCount);
        method_exchangeImplementations(imposterMethods[i], originalMethods[j]);
    }

    free(imposterMethods);
    free(originalMethods);
}
#endif

static void poseAsClass(const char* imposter, const char* original)
{
    Class imposterClass = objc_getClass(imposter);
    Class originalClass = objc_getClass(original);

#ifndef __OBJC2__
    class_poseAs(imposterClass, originalClass);
#else

    // Swizzle instance methods
    swizzleAllMethods(imposterClass, originalClass);
    // and then class methods
    swizzleAllMethods(object_getClass(imposterClass), object_getClass(originalClass));
#endif
}

void setPersistentUserStyleSheetLocation(CFStringRef url)
{
    persistentUserStyleSheetLocation = url;
}

static bool shouldIgnoreWebCoreNodeLeaks(const string& URLString)
{
    static char* const ignoreSet[] = {
        // Keeping this infrastructure around in case we ever need it again.
    };
    static const int ignoreSetCount = sizeof(ignoreSet) / sizeof(char*);
    
    for (int i = 0; i < ignoreSetCount; i++) {
        // FIXME: ignore case
        string curIgnore(ignoreSet[i]);
        // Match at the end of the URLString
        if (!URLString.compare(URLString.length() - curIgnore.length(), curIgnore.length(), curIgnore))
            return true;
    }
    return false;
}

#if !PLATFORM(IOS)
static NSSet *allowedFontFamilySet()
{
    static NSSet *fontFamilySet = [[NSSet setWithObjects:
        @"Ahem",
        @"Al Bayan",
        @"American Typewriter",
        @"Andale Mono",
        @"Apple Braille",
        @"Apple Color Emoji",
        @"Apple Chancery",
        @"Apple Garamond BT",
        @"Apple LiGothic",
        @"Apple LiSung",
        @"Apple Symbols",
        @"AppleGothic",
        @"AppleMyungjo",
        @"Arial Black",
        @"Arial Hebrew",
        @"Arial Narrow",
        @"Arial Rounded MT Bold",
        @"Arial Unicode MS",
        @"Arial",
        @"Ayuthaya",
        @"Baghdad",
        @"Baskerville",
        @"BiauKai",
        @"Big Caslon",
        @"Brush Script MT",
        @"Chalkboard",
        @"Chalkduster",
        @"Charcoal CY",
        @"Cochin",
        @"Comic Sans MS",
        @"Copperplate",
        @"Corsiva Hebrew",
        @"Courier New",
        @"Courier",
        @"DecoType Naskh",
        @"Devanagari MT",
        @"Didot",
        @"Euphemia UCAS",
        @"Futura",
        @"GB18030 Bitmap",
        @"Geeza Pro",
        @"Geneva CY",
        @"Geneva",
        @"Georgia",
        @"Gill Sans",
        @"Gujarati MT",
        @"GungSeo",
        @"Gurmukhi MT",
        @"HeadLineA",
        @"Hei",
        @"Heiti SC",
        @"Heiti TC",
        @"Helvetica CY",
        @"Helvetica Neue",
        @"Helvetica",
        @"Herculanum",
        @"Hiragino Kaku Gothic Pro",
        @"Hiragino Kaku Gothic ProN",
        @"Hiragino Kaku Gothic Std",
        @"Hiragino Kaku Gothic StdN",
        @"Hiragino Maru Gothic Monospaced",
        @"Hiragino Maru Gothic Pro",
        @"Hiragino Maru Gothic ProN",
        @"Hiragino Mincho Pro",
        @"Hiragino Mincho ProN",
        @"Hiragino Sans GB",
        @"Hoefler Text",
        @"Impact",
        @"InaiMathi",
        @"Kai",
        @"Kailasa",
        @"Kokonor",
        @"Krungthep",
        @"KufiStandardGK",
        @"LiHei Pro",
        @"LiSong Pro",
        @"Lucida Grande",
        @"Marker Felt",
        @"Menlo",
        @"Microsoft Sans Serif",
        @"Monaco",
        @"Mshtakan",
        @"Nadeem",
        @"New Peninim MT",
        @"Optima",
        @"Osaka",
        @"Papyrus",
        @"PCMyungjo",
        @"PilGi",
        @"Plantagenet Cherokee",
        @"Raanana",
        @"Sathu",
        @"Silom",
        @"Skia",
        @"Songti SC",
        @"Songti TC",
        @"STFangsong",
        @"STHeiti",
        @"STIXGeneral",
        @"STIXSizeOneSym",
        @"STKaiti",
        @"STSong",
        @"Symbol",
        @"System Font",
        @"Tahoma",
        @"Thonburi",
        @"Times New Roman",
        @"Times",
        @"Trebuchet MS",
        @"Verdana",
        @"Webdings",
        @"WebKit WeightWatcher",
        @"Wingdings 2",
        @"Wingdings 3",
        @"Wingdings",
        @"Zapf Dingbats",
        @"Zapfino",
        nil] retain];
    
    return fontFamilySet;
}

#if !ENABLE(PLATFORM_FONT_LOOKUP)
static IMP appKitAvailableFontFamiliesIMP;
static IMP appKitAvailableFontsIMP;

static NSArray *drt_NSFontManager_availableFontFamilies(id self, SEL _cmd)
{
    static NSArray *availableFontFamilies;
    if (availableFontFamilies)
        return availableFontFamilies;
    
    NSArray *availableFamilies = wtfCallIMP<id>(appKitAvailableFontFamiliesIMP, self, _cmd);

    NSMutableSet *prunedFamiliesSet = [NSMutableSet setWithArray:availableFamilies];
    [prunedFamiliesSet intersectSet:allowedFontFamilySet()];

    availableFontFamilies = [[prunedFamiliesSet allObjects] retain];
    return availableFontFamilies;
}

static NSArray *drt_NSFontManager_availableFonts(id self, SEL _cmd)
{
    static NSArray *availableFonts;
    if (availableFonts)
        return availableFonts;
    
    NSSet *allowedFamilies = allowedFontFamilySet();
    NSMutableArray *availableFontList = [[NSMutableArray alloc] initWithCapacity:[allowedFamilies count] * 2];
    for (NSString *fontFamily in allowedFontFamilySet()) {
        NSArray* fontsForFamily = [[NSFontManager sharedFontManager] availableMembersOfFontFamily:fontFamily];
        for (NSArray* fontInfo in fontsForFamily) {
            // Font name is the first entry in the array.
            [availableFontList addObject:[fontInfo objectAtIndex:0]];
        }
    }

    availableFonts = availableFontList;
    return availableFonts;
}

static void swizzleNSFontManagerMethods()
{
    Method availableFontFamiliesMethod = class_getInstanceMethod(objc_getClass("NSFontManager"), @selector(availableFontFamilies));
    ASSERT(availableFontFamiliesMethod);
    if (!availableFontFamiliesMethod) {
        NSLog(@"Failed to swizzle the \"availableFontFamilies\" method on NSFontManager");
        return;
    }
    
    appKitAvailableFontFamiliesIMP = method_setImplementation(availableFontFamiliesMethod, (IMP)drt_NSFontManager_availableFontFamilies);

    Method availableFontsMethod = class_getInstanceMethod(objc_getClass("NSFontManager"), @selector(availableFonts));
    ASSERT(availableFontsMethod);
    if (!availableFontsMethod) {
        NSLog(@"Failed to swizzle the \"availableFonts\" method on NSFontManager");
        return;
    }
    
    appKitAvailableFontsIMP = method_setImplementation(availableFontsMethod, (IMP)drt_NSFontManager_availableFonts);
}

#else

static NSArray *fontWhitelist()
{
    static NSArray *availableFonts;
    if (availableFonts)
        return availableFonts;

    NSMutableArray *availableFontList = [[NSMutableArray alloc] init];
    for (NSString *fontFamily in allowedFontFamilySet()) {
        NSArray* fontsForFamily = [[NSFontManager sharedFontManager] availableMembersOfFontFamily:fontFamily];
        [availableFontList addObject:fontFamily];
        for (NSArray* fontInfo in fontsForFamily) {
            // Font name is the first entry in the array.
            [availableFontList addObject:[fontInfo objectAtIndex:0]];
        }
    }

    availableFonts = availableFontList;
    return availableFonts;
}
#endif

// Activating system copies of these fonts overrides any others that could be preferred, such as ones
// in /Library/Fonts/Microsoft, and which don't always have the same metrics.
// FIXME: Switch to a solution from <rdar://problem/19553550> once it's available.
static void activateSystemCoreWebFonts()
{
    NSArray *coreWebFontNames = @[
        @"Andale Mono",
        @"Arial",
        @"Arial Black",
        @"Comic Sans MS",
        @"Courier New",
        @"Georgia",
        @"Impact",
        @"Times New Roman",
        @"Trebuchet MS",
        @"Verdana",
        @"Webdings"
    ];

    NSArray *fontFiles = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:[NSURL fileURLWithPath:@"/Library/Fonts" isDirectory:YES]
        includingPropertiesForKeys:@[NSURLFileResourceTypeKey, NSURLNameKey] options:0 error:0];

    for (NSURL *fontURL in fontFiles) {
        NSString *resourceType;
        NSString *fileName;
        if (![fontURL getResourceValue:&resourceType forKey:NSURLFileResourceTypeKey error:0]
            || ![fontURL getResourceValue:&fileName forKey:NSURLNameKey error:0])
            continue;
        if (![resourceType isEqualToString:NSURLFileResourceTypeRegular])
            continue;

        // Activate all font variations, such as Arial Bold Italic.ttf. This algorithm is not 100% precise, as it
        // also activates e.g. Arial Unicode, which is not a variation of Arial.
        for (NSString *coreWebFontName in coreWebFontNames) {
            if ([fileName hasPrefix:coreWebFontName]) {
                CTFontManagerRegisterFontsForURL((CFURLRef)fontURL, kCTFontManagerScopeProcess, 0);
                break;
            }
        }
    }
}

static void activateTestingFonts()
{
    static const char* fontFileNames[] = {
        "AHEM____.TTF",
        "WebKitWeightWatcher100.ttf",
        "WebKitWeightWatcher200.ttf",
        "WebKitWeightWatcher300.ttf",
        "WebKitWeightWatcher400.ttf",
        "WebKitWeightWatcher500.ttf",
        "WebKitWeightWatcher600.ttf",
        "WebKitWeightWatcher700.ttf",
        "WebKitWeightWatcher800.ttf",
        "WebKitWeightWatcher900.ttf",
        "SampleFont.sfont",
        0
    };

    NSMutableArray *fontURLs = [NSMutableArray array];
    NSURL *resourcesDirectory = [NSURL URLWithString:@"DumpRenderTree.resources" relativeToURL:[[NSBundle mainBundle] executableURL]];
    for (unsigned i = 0; fontFileNames[i]; ++i) {
        NSURL *fontURL = [resourcesDirectory URLByAppendingPathComponent:[NSString stringWithUTF8String:fontFileNames[i]]];
        [fontURLs addObject:[fontURL absoluteURL]];
    }

    CFArrayRef errors = 0;
    if (!CTFontManagerRegisterFontsForURLs((CFArrayRef)fontURLs, kCTFontManagerScopeProcess, &errors)) {
        NSLog(@"Failed to activate fonts: %@", errors);
        CFRelease(errors);
        exit(1);
    }
}

static void adjustFonts()
{
#if !ENABLE(PLATFORM_FONT_LOOKUP)
    swizzleNSFontManagerMethods();
#endif
    activateSystemCoreWebFonts();
    activateTestingFonts();
}
#else
static void activateFontIOS(const uint8_t* fontData, unsigned long length, std::string sectionName)
{
    CGDataProviderRef data = CGDataProviderCreateWithData(nullptr, fontData, length, nullptr);
    if (!data) {
        fprintf(stderr, "Failed to create CGDataProviderRef for the %s font.\n", sectionName.c_str());
        exit(1);
    }

    CGFontRef cgFont = CGFontCreateWithDataProvider(data);
    CGDataProviderRelease(data);
    if (!cgFont) {
        fprintf(stderr, "Failed to create CGFontRef for the %s font.\n", sectionName.c_str());
        exit(1);
    }

    CFErrorRef error = nullptr;
    CTFontManagerRegisterGraphicsFont(cgFont, &error);
    if (error) {
        fprintf(stderr, "Failed to add CGFont to CoreText for the %s font: %s.\n", sectionName.c_str(), CFStringGetCStringPtr(CFErrorCopyDescription(error), kCFStringEncodingUTF8));
        exit(1);
    }
    CGFontRelease(cgFont);
}

static void activateFontsIOS()
{
    // __asm() requires a string literal, so we can't do this as either local variables or template parameters.
#define fontData(sectionName) \
{ \
    extern const uint8_t start##sectionName __asm("section$start$__DATA$" # sectionName); \
    extern const uint8_t end##sectionName __asm("section$end$__DATA$" # sectionName); \
    activateFontIOS(&start##sectionName, &end##sectionName - &start##sectionName, #sectionName); \
}
    fontData(Ahem);
    fontData(WeightWatcher100);
    fontData(WeightWatcher200);
    fontData(WeightWatcher300);
    fontData(WeightWatcher400);
    fontData(WeightWatcher500);
    fontData(WeightWatcher600);
    fontData(WeightWatcher700);
    fontData(WeightWatcher800);
    fontData(WeightWatcher900);
}
#endif // !PLATFORM(IOS)


#if PLATFORM(IOS)
void adjustWebDocumentForFlexibleViewport(UIWebBrowserView *webBrowserView, UIWebScrollView *scrollView)
{
    // These values match MobileSafari's, see -[TabDocument _createDocumentView].
    [webBrowserView setMinimumScale:0.25f forDocumentTypes:UIEveryDocumentMask];
    [webBrowserView setMaximumScale:5.0f forDocumentTypes:UIEveryDocumentMask];
    [webBrowserView setInitialScale:UIWebViewScalesToFitScale forDocumentTypes:UIEveryDocumentMask];
    [webBrowserView setViewportSize:CGSizeMake(UIWebViewStandardViewportWidth, UIWebViewGrowsAndShrinksToFitHeight) forDocumentTypes:UIEveryDocumentMask];

    // Adjust the viewport view and viewport to have similar behavior
    // as the browser.
    [(DumpRenderTreeBrowserView *)webBrowserView setScrollingUsesUIWebScrollView:YES];
    [webBrowserView setDelegate:scrollViewResizerDelegate];

    CGRect viewportRect = CGRectMake(0, 0, phoneViewWidth, phoneBrowserScrollViewHeight);
    [scrollView setBounds:viewportRect];
    [scrollView setFrame:viewportRect];

    [webBrowserView setMinimumSize:viewportRect.size];
    [webBrowserView setAutoresizes:YES];
    CGRect browserViewFrame = [webBrowserView frame];
    browserViewFrame.origin = CGPointMake(0, phoneBrowserAddressBarOffset);
    [webBrowserView setFrame:browserViewFrame];
}

void adjustWebDocumentForStandardViewport(UIWebBrowserView *webBrowserView, UIWebScrollView *scrollView)
{
    [webBrowserView setMinimumScale:1.0f forDocumentTypes:UIEveryDocumentMask];
    [webBrowserView setMaximumScale:5.0f forDocumentTypes:UIEveryDocumentMask];
    [webBrowserView setInitialScale:1.0f forDocumentTypes:UIEveryDocumentMask];

    [(DumpRenderTreeBrowserView *)webBrowserView setScrollingUsesUIWebScrollView:NO];
    [webBrowserView setDelegate: nil];

    [scrollView setBounds:layoutTestViewportRect];
    [scrollView setFrame:layoutTestViewportRect];

    [webBrowserView setMinimumSize:layoutTestViewportRect.size];
    [webBrowserView setAutoresizes:NO];
    CGRect browserViewFrame = [webBrowserView frame];
    browserViewFrame.origin = CGPointZero;
    [webBrowserView setFrame:browserViewFrame];
}
#endif

#if !PLATFORM(IOS)
@interface DRTMockScroller : NSScroller
@end

@implementation DRTMockScroller

- (NSRect)rectForPart:(NSScrollerPart)partCode
{
    if (partCode != NSScrollerKnob)
        return [super rectForPart:partCode];

    NSRect frameRect = [self frame];
    NSRect bounds = [self bounds];
    BOOL isHorizontal = frameRect.size.width > frameRect.size.height;
    CGFloat trackLength = isHorizontal ? bounds.size.width : bounds.size.height;
    CGFloat minKnobSize = isHorizontal ? bounds.size.height : bounds.size.width;
    CGFloat knobLength = max(minKnobSize, static_cast<CGFloat>(round(trackLength * [self knobProportion])));
    CGFloat knobPosition = static_cast<CGFloat>((round([self doubleValue] * (trackLength - knobLength))));
    
    if (isHorizontal)
        return NSMakeRect(bounds.origin.x + knobPosition, bounds.origin.y, knobLength, bounds.size.height);

    return NSMakeRect(bounds.origin.x, bounds.origin.y +  + knobPosition, bounds.size.width, knobLength);
}

- (void)drawKnob
{
    if (![self isEnabled])
        return;

    NSRect knobRect = [self rectForPart:NSScrollerKnob];
    
    static NSColor *knobColor = [[NSColor colorWithDeviceRed:0x80 / 255.0 green:0x80 / 255.0 blue:0x80 / 255.0 alpha:1] retain];
    [knobColor set];

    NSRectFill(knobRect);
}

- (void)drawRect:(NSRect)dirtyRect
{
    static NSColor *trackColor = [[NSColor colorWithDeviceRed:0xC0 / 255.0 green:0xC0 / 255.0 blue:0xC0 / 255.0 alpha:1] retain];
    static NSColor *disabledTrackColor = [[NSColor colorWithDeviceRed:0xE0 / 255.0 green:0xE0 / 255.0 blue:0xE0 / 255.0 alpha:1] retain];

    if ([self isEnabled])
        [trackColor set];
    else
        [disabledTrackColor set];

    NSRectFill(dirtyRect);
    
    [self drawKnob];
}

@end

static void registerMockScrollbars()
{
    [WebDynamicScrollBarsView setCustomScrollerClass:[DRTMockScroller class]];
}
#endif

WebView *createWebViewAndOffscreenWindow()
{
#if !PLATFORM(IOS)
    NSRect rect = NSMakeRect(0, 0, TestRunner::viewWidth, TestRunner::viewHeight);
    WebView *webView = [[WebView alloc] initWithFrame:rect frameName:nil groupName:@"org.webkit.DumpRenderTree"];
#else
    UIWebBrowserView *webBrowserView = [[[DumpRenderTreeBrowserView alloc] initWithFrame:layoutTestViewportRect] autorelease];
    [webBrowserView setInputViewObeysDOMFocus:YES];
    WebView *webView = [[webBrowserView webView] retain];
    [webView setGroupName:@"org.webkit.DumpRenderTree"];
#endif

    [webView setUIDelegate:uiDelegate];
    [webView setFrameLoadDelegate:frameLoadDelegate];
    [webView setEditingDelegate:editingDelegate];
    [webView setResourceLoadDelegate:resourceLoadDelegate];
    [webView _setGeolocationProvider:[MockGeolocationProvider shared]];
    [webView _setDeviceOrientationProvider:[WebDeviceOrientationProviderMock shared]];
    [webView _setNotificationProvider:[MockWebNotificationProvider shared]];

    // Register the same schemes that Safari does
    [WebView registerURLSchemeAsLocal:@"feed"];
    [WebView registerURLSchemeAsLocal:@"feeds"];
    [WebView registerURLSchemeAsLocal:@"feedsearch"];
    
#if PLATFORM(MAC) && ENABLE(PLATFORM_FONT_LOOKUP)
    [WebView _setFontWhitelist:fontWhitelist()];
#endif

#if !PLATFORM(IOS)
    [webView setContinuousSpellCheckingEnabled:YES];
    [webView setAutomaticQuoteSubstitutionEnabled:NO];
    [webView setAutomaticLinkDetectionEnabled:NO];
    [webView setAutomaticDashSubstitutionEnabled:NO];
    [webView setAutomaticTextReplacementEnabled:NO];
    [webView setAutomaticSpellingCorrectionEnabled:YES];
    [webView setGrammarCheckingEnabled:YES];

    [webView setDefersCallbacks:NO];
    [webView setInteractiveFormValidationEnabled:YES];
    [webView setValidationMessageTimerMagnification:-1];
    
    // To make things like certain NSViews, dragging, and plug-ins work, put the WebView a window, but put it off-screen so you don't see it.
    // Put it at -10000, -10000 in "flipped coordinates", since WebCore and the DOM use flipped coordinates.
    NSScreen *firstScreen = [[NSScreen screens] firstObject];
    NSRect windowRect = (showWebView) ? NSOffsetRect(rect, 100, 100) : NSOffsetRect(rect, -10000, [firstScreen frame].size.height - rect.size.height + 10000);
    DumpRenderTreeWindow *window = [[DumpRenderTreeWindow alloc] initWithContentRect:windowRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];

    [window setColorSpace:[firstScreen colorSpace]];
    [window setCollectionBehavior:NSWindowCollectionBehaviorStationary];
    [[window contentView] addSubview:webView];
    if (showWebView)
        [window orderFront:nil];
    else
        [window orderBack:nil];
    [window setAutodisplay:NO];

    [window startListeningForAcceleratedCompositingChanges];
#else
    DumpRenderTreeWindow *drtWindow = [[DumpRenderTreeWindow alloc] initWithLayer:[webBrowserView layer]];
    [drtWindow setContentView:webView];
    [webBrowserView setWAKWindow:drtWindow];

    [[webView window] makeFirstResponder:[[[webView mainFrame] frameView] documentView]];

    CGRect uiWindowRect = layoutTestViewportRect;
    uiWindowRect.origin.y += [UIApp statusBarHeight];
    UIWindow *uiWindow = [[[UIWindow alloc] initWithFrame:uiWindowRect] autorelease];

    // The UIWindow and UIWebBrowserView are released when the DumpRenderTreeWindow is closed.
    drtWindow.uiWindow = uiWindow;
    drtWindow.browserView = webBrowserView;

    UIWebScrollView *scrollView = [[UIWebScrollView alloc] initWithFrame:layoutTestViewportRect];
    [scrollView addSubview:webBrowserView];

    [uiWindow addSubview:scrollView];
    [scrollView release];

    adjustWebDocumentForStandardViewport(webBrowserView, scrollView);
#endif
    
#if !PLATFORM(IOS)
    // For reasons that are not entirely clear, the following pair of calls makes WebView handle its
    // dynamic scrollbars properly. Without it, every frame will always have scrollbars.
    NSBitmapImageRep *imageRep = [webView bitmapImageRepForCachingDisplayInRect:[webView bounds]];
    [webView cacheDisplayInRect:[webView bounds] toBitmapImageRep:imageRep];
#else
    [[webView mainFrame] _setVisibleSize:CGSizeMake(phoneViewWidth, phoneViewHeight)];
    [[webView preferences] _setTelephoneNumberParsingEnabled:NO];

    // Initialize the global UIViews, and set the key UIWindow to be painted.
    if (!gWebBrowserView) {
        gWebBrowserView = [webBrowserView retain];
        gWebScrollView = [scrollView retain];
        gDrtWindow = [drtWindow retain];
        [uiWindow makeKeyAndVisible];
        [uiWindow retain];
    }
#endif

    return webView;
}

static void destroyWebViewAndOffscreenWindow()
{
    WebView *webView = [mainFrame webView];
#if !PLATFORM(IOS)
    NSWindow *window = [webView window];
#endif
    [webView close];
    mainFrame = nil;

#if !PLATFORM(IOS)
    // Work around problem where registering drag types leaves an outstanding
    // "perform selector" on the window, which retains the window. It's a bit
    // inelegant and perhaps dangerous to just blow them all away, but in practice
    // it probably won't cause any trouble (and this is just a test tool, after all).
    [NSObject cancelPreviousPerformRequestsWithTarget:window];

    [window close]; // releases when closed
#else
    UIWindow *uiWindow = [gWebBrowserView window];
    [uiWindow removeFromSuperview];
    [uiWindow release];
#endif

    [webView release];
}

static NSString *libraryPathForDumpRenderTree()
{
    char* dumpRenderTreeTemp = getenv("DUMPRENDERTREE_TEMP");
    if (dumpRenderTreeTemp)
        return [[NSFileManager defaultManager] stringWithFileSystemRepresentation:dumpRenderTreeTemp length:strlen(dumpRenderTreeTemp)];
    else
        return [@"~/Library/Application Support/DumpRenderTree" stringByExpandingTildeInPath];
}

// Called before each test.
static void resetWebPreferencesToConsistentValues()
{
    WebPreferences *preferences = [WebPreferences standardPreferences];

    [preferences setAllowUniversalAccessFromFileURLs:YES];
    [preferences setAllowFileAccessFromFileURLs:YES];
    [preferences setStandardFontFamily:@"Times"];
    [preferences setFixedFontFamily:@"Courier"];
    [preferences setSerifFontFamily:@"Times"];
    [preferences setSansSerifFontFamily:@"Helvetica"];
    [preferences setCursiveFontFamily:@"Apple Chancery"];
    [preferences setFantasyFontFamily:@"Papyrus"];
    [preferences setPictographFontFamily:@"Apple Color Emoji"];
    [preferences setDefaultFontSize:16];
    [preferences setDefaultFixedFontSize:13];
    [preferences setAntialiasedFontDilationEnabled:NO];
    [preferences setMinimumFontSize:0];
    [preferences setDefaultTextEncodingName:@"ISO-8859-1"];
    [preferences setJavaEnabled:NO];
    [preferences setJavaScriptEnabled:YES];
    [preferences setEditableLinkBehavior:WebKitEditableLinkOnlyLiveWithShiftKey];
#if !PLATFORM(IOS)
    [preferences setTabsToLinks:NO];
#endif
    [preferences setDOMPasteAllowed:YES];
#if !PLATFORM(IOS)
    [preferences setShouldPrintBackgrounds:YES];
#endif
    [preferences setCacheModel:WebCacheModelDocumentBrowser];
    [preferences setXSSAuditorEnabled:NO];
    [preferences setExperimentalNotificationsEnabled:NO];
    [preferences setPlugInsEnabled:YES];
#if !PLATFORM(IOS)
    [preferences setTextAreasAreResizable:YES];
#endif

    [preferences setPrivateBrowsingEnabled:NO];
    [preferences setAuthorAndUserStylesEnabled:YES];
    [preferences setShrinksStandaloneImagesToFit:YES];
    [preferences setJavaScriptCanOpenWindowsAutomatically:YES];
    [preferences setJavaScriptCanAccessClipboard:YES];
    [preferences setOfflineWebApplicationCacheEnabled:YES];
    [preferences setDeveloperExtrasEnabled:NO];
    [preferences setJavaScriptRuntimeFlags:WebKitJavaScriptRuntimeFlagsAllEnabled];
    [preferences setLoadsImagesAutomatically:YES];
    [preferences setLoadsSiteIconsIgnoringImageLoadingPreference:NO];
    [preferences setFrameFlatteningEnabled:NO];
    [preferences setSpatialNavigationEnabled:NO];
    [preferences setMetaRefreshEnabled:YES];

    if (persistentUserStyleSheetLocation) {
        [preferences setUserStyleSheetLocation:[NSURL URLWithString:(NSString *)(persistentUserStyleSheetLocation.get())]];
        [preferences setUserStyleSheetEnabled:YES];
    } else
        [preferences setUserStyleSheetEnabled:NO];
#if PLATFORM(IOS)
    [preferences setMediaPlaybackAllowsInline:YES];
    [preferences setMediaPlaybackRequiresUserGesture:NO];

    // Enable the tracker before creating the first WebView will
    // cause initialization to use the correct database paths.
    [preferences setStorageTrackerEnabled:YES];
#endif

#if ENABLE(IOS_TEXT_AUTOSIZING)
    // Disable text autosizing by default.
    [preferences _setMinimumZoomFontSize:0];
#endif

    // The back/forward cache is causing problems due to layouts during transition from one page to another.
    // So, turn it off for now, but we might want to turn it back on some day.
    [preferences setUsesPageCache:NO];
    [preferences setAcceleratedCompositingEnabled:YES];
#if USE(CA)
    [preferences setCanvasUsesAcceleratedDrawing:YES];
    [preferences setAcceleratedDrawingEnabled:useAcceleratedDrawing];
#endif
    [preferences setWebGLEnabled:NO];
    [preferences setCSSRegionsEnabled:YES];
    [preferences setUsePreHTML5ParserQuirks:NO];
    [preferences setAsynchronousSpellCheckingEnabled:NO];
#if !PLATFORM(IOS)
    ASSERT([preferences mockScrollbarsEnabled]);
#endif

#if ENABLE(WEB_AUDIO)
    [preferences setWebAudioEnabled:YES];
#endif

#if ENABLE(IOS_TEXT_AUTOSIZING)
    // Disable text autosizing by default.
    [preferences _setMinimumZoomFontSize:0];
#endif

#if ENABLE(MEDIA_SOURCE)
    [preferences setMediaSourceEnabled:YES];
#endif

    [preferences setHiddenPageDOMTimerThrottlingEnabled:NO];
    [preferences setHiddenPageCSSAnimationSuspensionEnabled:NO];

    [WebPreferences _clearNetworkLoaderSession];
    [WebPreferences _setCurrentNetworkLoaderSessionCookieAcceptPolicy:NSHTTPCookieAcceptPolicyOnlyFromMainDocumentDomain];
}

// Called once on DumpRenderTree startup.
static void setDefaultsToConsistentValuesForTesting()
{
#if PLATFORM(IOS)
    WebThreadLock();
#endif

    static const int NoFontSmoothing = 0;
    static const int BlueTintedAppearance = 1;

    NSString *libraryPath = libraryPathForDumpRenderTree();

    NSDictionary *dict = @{
        @"AppleKeyboardUIMode": @1,
        @"AppleAntiAliasingThreshold": @4,
        @"AppleFontSmoothing": @(NoFontSmoothing),
        @"AppleAquaColorVariant": @(BlueTintedAppearance),
        @"AppleHighlightColor": @"0.709800 0.835300 1.000000",
        @"AppleOtherHighlightColor":@"0.500000 0.500000 0.500000",
        @"AppleLanguages": @[ @"en" ],
        WebKitEnableFullDocumentTeardownPreferenceKey: @YES,
        WebKitFullScreenEnabledPreferenceKey: @YES,
        @"UseWebKitWebInspector": @YES,
#if !PLATFORM(IOS)
        @"NSPreferredSpellServerLanguage": @"en_US",
        @"NSUserDictionaryReplacementItems": @[],
        @"NSTestCorrectionDictionary": @{
            @"notationl": @"notational",
            @"mesage": @"message",
            @"wouldn": @"would",
            @"wellcome": @"welcome",
            @"hellolfworld": @"hello\nworld"
        },
#endif
        @"AppleScrollBarVariant": @"DoubleMax",
#if !PLATFORM(IOS)
        @"NSScrollAnimationEnabled": @NO,
#endif
        @"NSOverlayScrollersEnabled": @NO,
        @"AppleShowScrollBars": @"Always",
        @"NSButtonAnimationsEnabled": @NO, // Ideally, we should find a way to test animations, but for now, make sure that the dumped snapshot matches actual state.
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED > 101000
        @"AppleSystemFontOSSubversion": @(10),
#endif
        @"NSWindowDisplayWithRunLoopObserver": @YES, // Temporary workaround, see <rdar://problem/20351297>.
    };

    [[NSUserDefaults standardUserDefaults] setValuesForKeysWithDictionary:dict];

#if PLATFORM(MAC)
    // Make NSFont use the new defaults.
    [NSFont initialize];
#endif

    NSDictionary *processInstanceDefaults = @{
        WebDatabaseDirectoryDefaultsKey: [libraryPath stringByAppendingPathComponent:@"Databases"],
        WebStorageDirectoryDefaultsKey: [libraryPath stringByAppendingPathComponent:@"LocalStorage"],
        WebKitLocalCacheDefaultsKey: [libraryPath stringByAppendingPathComponent:@"LocalCache"],
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED > 101000
        // This needs to also be added to argument domain because of <rdar://problem/20210002>.
        @"AppleSystemFontOSSubversion": @(10),
#endif
    };

    [[NSUserDefaults standardUserDefaults] setVolatileDomain:processInstanceDefaults forName:NSArgumentDomain];
}

static void runThread(void* arg)
{
    static ThreadIdentifier previousId = 0;
    ThreadIdentifier currentId = currentThread();
    // Verify 2 successive threads do not get the same Id.
    ASSERT(previousId != currentId);
    previousId = currentId;
}

static void* runPthread(void* arg)
{
    runThread(arg);
    return 0;
}

static void testThreadIdentifierMap()
{
    // Imitate 'foreign' threads that are not created by WTF.
    pthread_t pthread;
    pthread_create(&pthread, 0, &runPthread, 0);
    pthread_join(pthread, 0);

    pthread_create(&pthread, 0, &runPthread, 0);
    pthread_join(pthread, 0);

    // Now create another thread using WTF. On OSX, it will have the same pthread handle
    // but should get a different ThreadIdentifier.
    createThread(runThread, 0, "DumpRenderTree: test");
}

static void allocateGlobalControllers()
{
    // FIXME: We should remove these and move to the ObjC standard [Foo sharedInstance] model
    gNavigationController = [[NavigationController alloc] init];
    frameLoadDelegate = [[FrameLoadDelegate alloc] init];
    uiDelegate = [[UIDelegate alloc] init];
    editingDelegate = [[EditingDelegate alloc] init];
    resourceLoadDelegate = [[ResourceLoadDelegate alloc] init];
    policyDelegate = [[PolicyDelegate alloc] init];
    historyDelegate = [[HistoryDelegate alloc] init];
    defaultPolicyDelegate = [[DefaultPolicyDelegate alloc] init];
#if PLATFORM(IOS)
    scrollViewResizerDelegate = [[ScrollViewResizerDelegate alloc] init];
#endif
}

// ObjC++ doens't seem to let me pass NSObject*& sadly.
static inline void releaseAndZero(NSObject** object)
{
    [*object release];
    *object = nil;
}

static void releaseGlobalControllers()
{
    releaseAndZero(&gNavigationController);
    releaseAndZero(&frameLoadDelegate);
    releaseAndZero(&editingDelegate);
    releaseAndZero(&resourceLoadDelegate);
    releaseAndZero(&uiDelegate);
    releaseAndZero(&policyDelegate);
#if PLATFORM(IOS)
    releaseAndZero(&scrollViewResizerDelegate);
#endif
}

static void initializeGlobalsFromCommandLineOptions(int argc, const char *argv[])
{
    struct option options[] = {
        {"notree", no_argument, &dumpTree, NO},
        {"pixel-tests", no_argument, &dumpPixelsForAllTests, YES},
        {"tree", no_argument, &dumpTree, YES},
        {"threaded", no_argument, &threaded, YES},
        {"complex-text", no_argument, &forceComplexText, YES},
        {"accelerated-drawing", no_argument, &useAcceleratedDrawing, YES},
        {"gc-between-tests", no_argument, &gcBetweenTests, YES},
        {"no-timeout", no_argument, &useTimeoutWatchdog, NO},
        {"allowed-host", required_argument, nullptr, 'a'},
        {"show-webview", no_argument, &showWebView, YES},
        {nullptr, 0, nullptr, 0}
    };
    
    int option;
    while ((option = getopt_long(argc, (char * const *)argv, "", options, nullptr)) != -1) {
        switch (option) {
            case '?':   // unknown or ambiguous option
            case ':':   // missing argument
                exit(1);
                break;
            case 'a': // "allowed-host"
                allowedHosts.insert(optarg);
                break;
        }
    }
}

static void addTestPluginsToPluginSearchPath(const char* executablePath)
{
#if !PLATFORM(IOS)
    NSString *pwd = [[NSString stringWithUTF8String:executablePath] stringByDeletingLastPathComponent];
    [WebPluginDatabase setAdditionalWebPlugInPaths:[NSArray arrayWithObject:pwd]];
    [[WebPluginDatabase sharedDatabase] refresh];
#endif
}

static bool useLongRunningServerMode(int argc, const char *argv[])
{
    // This assumes you've already called getopt_long
    return (argc == optind+1 && strcmp(argv[optind], "-") == 0);
}

static void runTestingServerLoop()
{
    // When DumpRenderTree run in server mode, we just wait around for file names
    // to be passed to us and read each in turn, passing the results back to the client
    char filenameBuffer[2048];
    while (fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {
        char *newLineCharacter = strchr(filenameBuffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (strlen(filenameBuffer) == 0)
            continue;

        runTest(filenameBuffer);
    }
}

static void prepareConsistentTestingEnvironment()
{
#if !PLATFORM(IOS)
    poseAsClass("DumpRenderTreePasteboard", "NSPasteboard");
    poseAsClass("DumpRenderTreeEvent", "NSEvent");
#else
    poseAsClass("DumpRenderTreeEvent", "GSEvent");
#endif

    [[WebPreferences standardPreferences] setAutosaves:NO];

#if !PLATFORM(IOS)
    // +[WebPreferences _switchNetworkLoaderToNewTestingSession] calls +[NSURLCache sharedURLCache], which initializes a default cache on disk.
    // Making the shared cache memory-only avoids touching the file system.
    RetainPtr<NSURLCache> sharedCache =
        adoptNS([[NSURLCache alloc] initWithMemoryCapacity:1024 * 1024
                                      diskCapacity:0
                                          diskPath:nil]);
    [NSURLCache setSharedURLCache:sharedCache.get()];

    [WebPreferences _switchNetworkLoaderToNewTestingSession];

    adjustFonts();
    registerMockScrollbars();

    // The mock scrollbars setting cannot be modified after creating a view, so we have to do it now.
    [[WebPreferences standardPreferences] setMockScrollbarsEnabled:YES];
#else
    activateFontsIOS();
#endif
    
    allocateGlobalControllers();
    
    makeLargeMallocFailSilently();

#if PLATFORM(MAC)
    NSActivityOptions options = (NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical) & ~(NSActivitySuddenTerminationDisabled | NSActivityAutomaticTerminationDisabled);
    static id assertion = [[[NSProcessInfo processInfo] beginActivityWithOptions:options reason:@"DumpRenderTree should not be subject to process suppression"] retain];
    ASSERT_UNUSED(assertion, assertion);
#endif
}

const char crashedMessage[] = "#CRASHED\n";

void writeCrashedMessageOnFatalError(int signalCode)
{
    // Reset the default action for the signal so that we run ReportCrash(8) on pending and
    // subsequent instances of the signal.
    signal(signalCode, SIG_DFL);

    // WRITE(2) and FSYNC(2) are considered safe to call from a signal handler by SIGACTION(2).
    write(STDERR_FILENO, &crashedMessage[0], sizeof(crashedMessage) - 1);
    fsync(STDERR_FILENO);
}

void dumpRenderTree(int argc, const char *argv[])
{
#if PLATFORM(IOS)
    NSString *identifier = [[NSBundle mainBundle] bundleIdentifier];
    const char *stdinPath = [[NSString stringWithFormat:@"/tmp/%@_IN", identifier] UTF8String];
    const char *stdoutPath = [[NSString stringWithFormat:@"/tmp/%@_OUT", identifier] UTF8String];
    const char *stderrPath = [[NSString stringWithFormat:@"/tmp/%@_ERROR", identifier] UTF8String];

    int infd = open(stdinPath, O_RDWR);
    dup2(infd, STDIN_FILENO);
    int outfd = open(stdoutPath, O_RDWR);
    dup2(outfd, STDOUT_FILENO);
    int errfd = open(stderrPath, O_RDWR | O_NONBLOCK);
    dup2(errfd, STDERR_FILENO);
#endif

    signal(SIGILL, &writeCrashedMessageOnFatalError);
    signal(SIGFPE, &writeCrashedMessageOnFatalError);
    signal(SIGBUS, &writeCrashedMessageOnFatalError);
    signal(SIGSEGV, &writeCrashedMessageOnFatalError);

    initializeGlobalsFromCommandLineOptions(argc, argv);
    prepareConsistentTestingEnvironment();
    addTestPluginsToPluginSearchPath(argv[0]);

    if (forceComplexText)
        [WebView _setAlwaysUsesComplexTextCodePath:YES];

#if USE(APPKIT)
    [NSSound _setAlertType:0];
#endif

    WebView *webView = createWebViewAndOffscreenWindow();
    mainFrame = [webView mainFrame];

    [[NSURLCache sharedURLCache] removeAllCachedResponses];
    [WebCache empty];

    [NSURLRequest setAllowsAnyHTTPSCertificate:YES forHost:@"localhost"];
    [NSURLRequest setAllowsAnyHTTPSCertificate:YES forHost:@"127.0.0.1"];

    // http://webkit.org/b/32689
    testThreadIdentifierMap();

    if (threaded)
        startJavaScriptThreads();

    if (useLongRunningServerMode(argc, argv)) {
        printSeparators = YES;
        runTestingServerLoop();
    } else {
        printSeparators = optind < argc - 1;
        for (int i = optind; i != argc; ++i)
            runTest(argv[i]);
    }

    if (threaded)
        stopJavaScriptThreads();

    destroyWebViewAndOffscreenWindow();
    
    releaseGlobalControllers();
    
#if !PLATFORM(IOS)
    [DumpRenderTreePasteboard releaseLocalPasteboards];
#endif

    // FIXME: This should be moved onto TestRunner and made into a HashSet
    if (disallowedURLs) {
        CFRelease(disallowedURLs);
        disallowedURLs = 0;
    }

#if PLATFORM(IOS)
    close(infd);
    close(outfd);
    close(errfd);
#endif
}

#if PLATFORM(IOS)
static int _argc;
static const char **_argv;

@implementation DumpRenderTree

- (void)_runDumpRenderTree
{
    dumpRenderTree(_argc, _argv);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [self performSelectorOnMainThread:@selector(_runDumpRenderTree) withObject:nil waitUntilDone:NO];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    /* Apps will get suspended or killed some time after entering the background state but we want to be able to run multiple copies of DumpRenderTree. Periodically check to see if our remaining background time dips below a threshold and create a new background task.
    */
    void (^expirationHandler)() = ^ {
        [application endBackgroundTask:backgroundTaskIdentifier];
        backgroundTaskIdentifier = UIBackgroundTaskInvalid;
    };

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{

        NSTimeInterval timeRemaining;
        while (true) {
            timeRemaining = [application backgroundTimeRemaining];
            if (timeRemaining <= 10.0 || backgroundTaskIdentifier == UIBackgroundTaskInvalid) {
                [application endBackgroundTask:backgroundTaskIdentifier];
                backgroundTaskIdentifier = [application beginBackgroundTaskWithExpirationHandler:expirationHandler];
            }
            sleep(5);
        }
    });
}

- (void)_webThreadEventLoopHasRun
{
    ASSERT(!WebThreadIsCurrent());
    _hasFlushedWebThreadRunQueue = YES;
}

- (void)_webThreadInvoked
{
    ASSERT(WebThreadIsCurrent());
    dispatch_async(dispatch_get_main_queue(), ^{
        [self _webThreadEventLoopHasRun];
    });
}

// The test can end in response to a delegate callback while there are still methods queued on the Web Thread.
// If we do not ensure the Web Thread has been run, the callback can be done on a WebView that no longer exists.
// To avoid this, _waitForWebThread dispatches a call to the WebThread event loop, actively processing the delegate
// callbacks in the main thread while waiting for the call to be invoked on the Web Thread.
- (void)_waitForWebThread
{
    ASSERT(!WebThreadIsCurrent());
    _hasFlushedWebThreadRunQueue = NO;
    WebThreadRun(^{
        [self _webThreadInvoked];
    });
    while (!_hasFlushedWebThreadRunQueue) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
        [pool release];
    }
}

@end
#endif

static bool returningFromMain = false;

void atexitFunction()
{
    if (returningFromMain)
        return;

    NSLog(@"DumpRenderTree is exiting unexpectedly. Generating a crash log.");
    __builtin_trap();
}

int DumpRenderTreeMain(int argc, const char *argv[])
{
    atexit(atexitFunction);

#if PLATFORM(IOS)
    _UIApplicationLoadWebKit();
#endif
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    setDefaultsToConsistentValuesForTesting(); // Must be called before NSApplication initialization.

#if !PLATFORM(IOS)
    [DumpRenderTreeApplication sharedApplication]; // Force AppKit to init itself

    dumpRenderTree(argc, argv);
#else
    _argc = argc;
    _argv = argv;
    UIApplicationMain(argc, (char**)argv, @"DumpRenderTree", @"DumpRenderTree");
#endif
    [WebCoreStatistics garbageCollectJavaScriptObjects];
    [WebCoreStatistics emptyCache]; // Otherwise SVGImages trigger false positives for Frame/Node counts
    if (JSC::Options::logHeapStatisticsAtExit())
        JSC::HeapStatistics::reportSuccess();
    [pool release];
    returningFromMain = true;
    return 0;
}

static NSInteger compareHistoryItems(id item1, id item2, void *context)
{
    return [[item1 target] caseInsensitiveCompare:[item2 target]];
}

static NSData *dumpAudio()
{
    const vector<char>& dataVector = gTestRunner->audioResult();
    
    NSData *data = [NSData dataWithBytes:dataVector.data() length:dataVector.size()];
    return data;
}

static void dumpHistoryItem(WebHistoryItem *item, int indent, BOOL current)
{
    int start = 0;
    if (current) {
        printf("curr->");
        start = 6;
    }
    for (int i = start; i < indent; i++)
        putchar(' ');
    
    NSString *urlString = [item URLString];
    if ([[NSURL URLWithString:urlString] isFileURL]) {
        NSRange range = [urlString rangeOfString:@"/LayoutTests/"];
        urlString = [@"(file test):" stringByAppendingString:[urlString substringFromIndex:(range.length + range.location)]];
    }
    
    printf("%s", [urlString UTF8String]);
    NSString *target = [item target];
    if (target && [target length] > 0)
        printf(" (in frame \"%s\")", [target UTF8String]);
    if ([item isTargetItem])
        printf("  **nav target**");
    putchar('\n');
    NSArray *kids = [item children];
    if (kids) {
        // must sort to eliminate arbitrary result ordering which defeats reproducible testing
        kids = [kids sortedArrayUsingFunction:&compareHistoryItems context:nil];
        for (unsigned i = 0; i < [kids count]; i++)
            dumpHistoryItem([kids objectAtIndex:i], indent+4, NO);
    }
}

static void dumpFrameScrollPosition(WebFrame *f)
{
    WebScriptObject* scriptObject = [f windowObject];
    NSPoint scrollPosition = NSMakePoint(
        [[scriptObject valueForKey:@"pageXOffset"] floatValue],
        [[scriptObject valueForKey:@"pageYOffset"] floatValue]);
    if (ABS(scrollPosition.x) > 0.00000001 || ABS(scrollPosition.y) > 0.00000001) {
        if ([f parentFrame] != nil)
            printf("frame '%s' ", [[f name] UTF8String]);
        printf("scrolled to %.f,%.f\n", scrollPosition.x, scrollPosition.y);
    }

    if (gTestRunner->dumpChildFrameScrollPositions()) {
        NSArray *kids = [f childFrames];
        if (kids)
            for (unsigned i = 0; i < [kids count]; i++)
                dumpFrameScrollPosition([kids objectAtIndex:i]);
    }
}

static NSString *dumpFramesAsText(WebFrame *frame)
{
    DOMDocument *document = [frame DOMDocument];
    DOMElement *documentElement = [document documentElement];

    if (!documentElement)
        return @"";

    NSMutableString *result = [[[NSMutableString alloc] init] autorelease];

    // Add header for all but the main frame.
    if ([frame parentFrame])
        result = [NSMutableString stringWithFormat:@"\n--------\nFrame: '%@'\n--------\n", [frame name]];

    [result appendFormat:@"%@\n", [documentElement innerText]];

    if (gTestRunner->dumpChildFramesAsText()) {
        NSArray *kids = [frame childFrames];
        if (kids) {
            for (unsigned i = 0; i < [kids count]; i++)
                [result appendString:dumpFramesAsText([kids objectAtIndex:i])];
        }
    }

    return result;
}

static NSData *dumpFrameAsPDF(WebFrame *frame)
{
#if !PLATFORM(IOS)
    if (!frame)
        return nil;

    // Sadly we have to dump to a file and then read from that file again
    // +[NSPrintOperation PDFOperationWithView:insideRect:] requires a rect and prints to a single page
    // likewise +[NSView dataWithPDFInsideRect:] also prints to a single continuous page
    // The goal of this function is to test "real" printing across multiple pages.
    // FIXME: It's possible there might be printing SPI to let us print a multi-page PDF to an NSData object
    NSString *path = [libraryPathForDumpRenderTree() stringByAppendingPathComponent:@"test.pdf"];

    NSMutableDictionary *printInfoDict = [NSMutableDictionary dictionaryWithDictionary:[[NSPrintInfo sharedPrintInfo] dictionary]];
    [printInfoDict setObject:NSPrintSaveJob forKey:NSPrintJobDisposition];
    [printInfoDict setObject:path forKey:NSPrintSavePath];

    NSPrintInfo *printInfo = [[NSPrintInfo alloc] initWithDictionary:printInfoDict];
    [printInfo setHorizontalPagination:NSAutoPagination];
    [printInfo setVerticalPagination:NSAutoPagination];
    [printInfo setVerticallyCentered:NO];

    NSPrintOperation *printOperation = [NSPrintOperation printOperationWithView:[frame frameView] printInfo:printInfo];
    [printOperation setShowPanels:NO];
    [printOperation runOperation];

    [printInfo release];

    NSData *pdfData = [NSData dataWithContentsOfFile:path];
    [[NSFileManager defaultManager] removeFileAtPath:path handler:nil];

    return pdfData;
#else
    return nil;
#endif
}

static void dumpBackForwardListForWebView(WebView *view)
{
    printf("\n============== Back Forward List ==============\n");
    WebBackForwardList *bfList = [view backForwardList];

    // Print out all items in the list after prevTestBFItem, which was from the previous test
    // Gather items from the end of the list, the print them out from oldest to newest
    NSMutableArray *itemsToPrint = [[NSMutableArray alloc] init];
    for (int i = [bfList forwardListCount]; i > 0; i--) {
        WebHistoryItem *item = [bfList itemAtIndex:i];
        // something is wrong if the item from the last test is in the forward part of the b/f list
        assert(item != prevTestBFItem);
        [itemsToPrint addObject:item];
    }
            
    assert([bfList currentItem] != prevTestBFItem);
    [itemsToPrint addObject:[bfList currentItem]];
    int currentItemIndex = [itemsToPrint count] - 1;

    for (int i = -1; i >= -[bfList backListCount]; i--) {
        WebHistoryItem *item = [bfList itemAtIndex:i];
        if (item == prevTestBFItem)
            break;
        [itemsToPrint addObject:item];
    }

    for (int i = [itemsToPrint count]-1; i >= 0; i--)
        dumpHistoryItem([itemsToPrint objectAtIndex:i], 8, i == currentItemIndex);

    [itemsToPrint release];
    printf("===============================================\n");
}

#if !PLATFORM(IOS)
static void changeWindowScaleIfNeeded(const char* testPathOrUR)
{
    bool hasHighDPIWindow = [[[mainFrame webView] window] backingScaleFactor] != 1;
    WTF::String localPathOrUrl = String(testPathOrUR);
    bool needsHighDPIWindow = localPathOrUrl.findIgnoringCase("/hidpi-") != notFound;
    if (hasHighDPIWindow == needsHighDPIWindow)
        return;

    CGFloat newScaleFactor = needsHighDPIWindow ? 2 : 1;
    // When the new scale factor is set on the window first, WebView doesn't see it as a new scale and stops propagating the behavior change to WebCore::Page.
    gTestRunner->setBackingScaleFactor(newScaleFactor);
    [[[mainFrame webView] window] _setWindowResolution:newScaleFactor displayIfChanged:YES];
}
#endif

static void sizeWebViewForCurrentTest()
{
    [uiDelegate resetWindowOrigin];

    // W3C SVG tests expect to be 480x360
    bool isSVGW3CTest = (gTestRunner->testURL().find("svg/W3C-SVG-1.1") != string::npos);
    if (isSVGW3CTest)
        [[mainFrame webView] setFrameSize:NSMakeSize(TestRunner::w3cSVGViewWidth, TestRunner::w3cSVGViewHeight)];
    else
        [[mainFrame webView] setFrameSize:NSMakeSize(TestRunner::viewWidth, TestRunner::viewHeight)];
}

static const char *methodNameStringForFailedTest()
{
    const char *errorMessage;
    if (gTestRunner->dumpAsText())
        errorMessage = "[documentElement innerText]";
    else if (gTestRunner->dumpDOMAsWebArchive())
        errorMessage = "[[mainFrame DOMDocument] webArchive]";
    else if (gTestRunner->dumpSourceAsWebArchive())
        errorMessage = "[[mainFrame dataSource] webArchive]";
    else
        errorMessage = "[mainFrame renderTreeAsExternalRepresentation]";

    return errorMessage;
}

static void dumpBackForwardListForAllWindows()
{
    CFArrayRef openWindows = (CFArrayRef)[DumpRenderTreeWindow openWindows];
    unsigned count = CFArrayGetCount(openWindows);
    for (unsigned i = 0; i < count; i++) {
        NSWindow *window = (NSWindow *)CFArrayGetValueAtIndex(openWindows, i);
#if !PLATFORM(IOS)
        WebView *webView = [[[window contentView] subviews] objectAtIndex:0];
#else
        ASSERT([[window contentView] isKindOfClass:[WebView class]]);
        WebView *webView = (WebView *)[window contentView];
#endif
        dumpBackForwardListForWebView(webView);
    }
}

static void invalidateAnyPreviousWaitToDumpWatchdog()
{
    if (waitToDumpWatchdog) {
        CFRunLoopTimerInvalidate(waitToDumpWatchdog);
        CFRelease(waitToDumpWatchdog);
        waitToDumpWatchdog = 0;
    }
}

void setWaitToDumpWatchdog(CFRunLoopTimerRef timer)
{
    ASSERT(timer);
    ASSERT(shouldSetWaitToDumpWatchdog());
    waitToDumpWatchdog = timer;
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), waitToDumpWatchdog, kCFRunLoopCommonModes);
}

bool shouldSetWaitToDumpWatchdog()
{
    return !waitToDumpWatchdog && useTimeoutWatchdog;
}

static void updateDisplay()
{
    WebView *webView = [mainFrame webView];
#if PLATFORM(IOS)
    [gWebBrowserView layoutIfNeeded]; // Re-enables tile painting, which was disabled when committing the frame load.
    [gDrtWindow layoutTilesNow];
    [webView _flushCompositingChanges];
#else
    if ([webView _isUsingAcceleratedCompositing])
        [webView display];
    else
        [webView displayIfNeeded];
#endif
}

void dump()
{
#if PLATFORM(IOS)
    WebThreadLock();
#endif

    updateDisplay();

    invalidateAnyPreviousWaitToDumpWatchdog();
    ASSERT(!gTestRunner->hasPendingWebNotificationClick());

    if (dumpTree) {
        NSString *resultString = nil;
        NSData *resultData = nil;
        NSString *resultMimeType = @"text/plain";

        if ([[[mainFrame dataSource] _responseMIMEType] isEqualToString:@"text/plain"]) {
            gTestRunner->setDumpAsText(true);
            gTestRunner->setGeneratePixelResults(false);
        }
        if (gTestRunner->dumpAsAudio()) {
            resultData = dumpAudio();
            resultMimeType = @"audio/wav";
        } else if (gTestRunner->dumpAsText()) {
            resultString = dumpFramesAsText(mainFrame);
        } else if (gTestRunner->dumpAsPDF()) {
            resultData = dumpFrameAsPDF(mainFrame);
            resultMimeType = @"application/pdf";
        } else if (gTestRunner->dumpDOMAsWebArchive()) {
            WebArchive *webArchive = [[mainFrame DOMDocument] webArchive];
            resultString = CFBridgingRelease(createXMLStringFromWebArchiveData((CFDataRef)[webArchive data]));
            resultMimeType = @"application/x-webarchive";
        } else if (gTestRunner->dumpSourceAsWebArchive()) {
            WebArchive *webArchive = [[mainFrame dataSource] webArchive];
            resultString = CFBridgingRelease(createXMLStringFromWebArchiveData((CFDataRef)[webArchive data]));
            resultMimeType = @"application/x-webarchive";
        } else
            resultString = [mainFrame renderTreeAsExternalRepresentationForPrinting:gTestRunner->isPrinting()];

        if (resultString && !resultData)
            resultData = [resultString dataUsingEncoding:NSUTF8StringEncoding];

        printf("Content-Type: %s\n", [resultMimeType UTF8String]);

        WTF::FastMallocStatistics mallocStats = WTF::fastMallocStatistics();
        printf("DumpMalloc: %li\n", mallocStats.committedVMBytes);

        if (gTestRunner->dumpAsAudio())
            printf("Content-Length: %lu\n", static_cast<unsigned long>([resultData length]));

        if (resultData) {
            fwrite([resultData bytes], 1, [resultData length], stdout);

            if (!gTestRunner->dumpAsText() && !gTestRunner->dumpDOMAsWebArchive() && !gTestRunner->dumpSourceAsWebArchive() && !gTestRunner->dumpAsAudio())
                dumpFrameScrollPosition(mainFrame);

            if (gTestRunner->dumpBackForwardList())
                dumpBackForwardListForAllWindows();
        } else
            printf("ERROR: nil result from %s", methodNameStringForFailedTest());

        // Stop the watchdog thread before we leave this test to make sure it doesn't
        // fire in between tests causing the next test to fail.
        // This is a speculative fix for: https://bugs.webkit.org/show_bug.cgi?id=32339
        invalidateAnyPreviousWaitToDumpWatchdog();

        if (printSeparators)
            puts("#EOF");       // terminate the content block
    }

    if (dumpPixelsForCurrentTest && gTestRunner->generatePixelResults())
        // FIXME: when isPrinting is set, dump the image with page separators.
        dumpWebViewAsPixelsAndCompareWithExpected(gTestRunner->expectedPixelHash());

    puts("#EOF");   // terminate the (possibly empty) pixels block
    fflush(stdout);

    done = YES;
    CFRunLoopStop(CFRunLoopGetMain());
}

static bool shouldLogFrameLoadDelegates(const char* pathOrURL)
{
    return strstr(pathOrURL, "loading/");
}

static bool shouldLogHistoryDelegates(const char* pathOrURL)
{
    return strstr(pathOrURL, "globalhistory/");
}

static bool shouldDumpAsText(const char* pathOrURL)
{
    return strstr(pathOrURL, "dumpAsText/");
}

static bool shouldEnableDeveloperExtras(const char* pathOrURL)
{
    return true;
}

#if PLATFORM(IOS)
static bool shouldMakeViewportFlexible(const char* pathOrURL)
{
    return strstr(pathOrURL, "viewport/");
}
#endif

static void resetWebViewToConsistentStateBeforeTesting()
{
    WebView *webView = [mainFrame webView];
#if PLATFORM(IOS)
    adjustWebDocumentForStandardViewport(gWebBrowserView, gWebScrollView);
    [webView _setAllowsMessaging:YES];
    [mainFrame setMediaDataLoadsAutomatically:YES];
#endif
    [webView setEditable:NO];
    [(EditingDelegate *)[webView editingDelegate] setAcceptsEditing:YES];
    [webView makeTextStandardSize:nil];
    [webView resetPageZoom:nil];
    [webView _scaleWebView:1.0 atOrigin:NSZeroPoint];
#if !PLATFORM(IOS)
    [webView _setCustomBackingScaleFactor:0];
#endif
    [webView setTabKeyCyclesThroughElements:YES];
    [webView setPolicyDelegate:defaultPolicyDelegate];
    [policyDelegate setPermissive:NO];
    [policyDelegate setControllerToNotifyDone:0];
    [frameLoadDelegate resetToConsistentState];
#if !PLATFORM(IOS)
    [webView _setDashboardBehavior:WebDashboardBehaviorUseBackwardCompatibilityMode to:NO];
#endif
    [webView _clearMainFrameName];
    [[webView undoManager] removeAllActions];
    [WebView _removeAllUserContentFromGroup:[webView groupName]];
#if !PLATFORM(IOS)
    [[webView window] setAutodisplay:NO];
#endif
    [webView setTracksRepaints:NO];

    [WebCache clearCachedCredentials];
    
    resetWebPreferencesToConsistentValues();

    TestRunner::setSerializeHTTPLoads(false);

    setlocale(LC_ALL, "");

    if (gTestRunner) {
        WebCoreTestSupport::resetInternalsObject([mainFrame globalContext]);
        // in the case that a test using the chrome input field failed, be sure to clean up for the next test
        gTestRunner->removeChromeInputField();
    }

#if !PLATFORM(IOS)
    if (WebCore::Frame* frame = [webView _mainCoreFrame])
        WebCoreTestSupport::clearWheelEventTestTrigger(*frame);
#endif

#if !PLATFORM(IOS)
    [webView setContinuousSpellCheckingEnabled:YES];
    [webView setAutomaticQuoteSubstitutionEnabled:NO];
    [webView setAutomaticLinkDetectionEnabled:NO];
    [webView setAutomaticDashSubstitutionEnabled:NO];
    [webView setAutomaticTextReplacementEnabled:NO];
    [webView setAutomaticSpellingCorrectionEnabled:YES];
    [webView setGrammarCheckingEnabled:YES];

    [WebView _setUsesTestModeFocusRingColor:YES];
#endif
    [WebView _resetOriginAccessWhitelists];
    [WebView _setAllowsRoundingHacks:NO];

    [[MockGeolocationProvider shared] stopTimer];
    [[MockWebNotificationProvider shared] reset];
    
#if !PLATFORM(IOS)
    // Clear the contents of the general pasteboard
    [[NSPasteboard generalPasteboard] declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
#endif

    [mainFrame _clearOpener];
}

#if PLATFORM(IOS)
// Work around <rdar://problem/9909073> WebKit's method of calling delegates on
// the main thread is not thread safe. If the web thread is attempting to call
// out to a delegate method on the main thread, we want to spin the main thread
// run loop until the delegate method completes before taking the web thread
// lock to prevent potentially re-entering WebCore.
static void WebThreadLockAfterDelegateCallbacksHaveCompleted()
{
    dispatch_semaphore_t delegateSemaphore = dispatch_semaphore_create(0);
    WebThreadRun(^{
        dispatch_semaphore_signal(delegateSemaphore);
    });

    while (dispatch_semaphore_wait(delegateSemaphore, DISPATCH_TIME_NOW)) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantPast]];
        [pool release];
    }

    WebThreadLock();
    
    dispatch_release(delegateSemaphore);
}
#endif

static NSString *testPathFromURL(NSURL* url)
{
    if ([url isFileURL]) {
        NSString *filePath = [url path];
        NSRange layoutTestsRange = [filePath rangeOfString:@"/LayoutTests/"];
        if (layoutTestsRange.location == NSNotFound)
            return nil;

        return [filePath substringFromIndex:NSMaxRange(layoutTestsRange)];
    }
    
    // HTTP test URLs look like: http://127.0.0.1:8000/inspector/resource-tree/resource-request-content-after-loading-and-clearing-cache.html
    if (![[url scheme] isEqualToString:@"http"] && ![[url scheme] isEqualToString:@"https"])
        return nil;

    if ([[url host] isEqualToString:@"127.0.0.1"] && ([[url port] intValue] == 8000 || [[url port] intValue] == 8443))
        return [url path];

    return nil;
}

static void runTest(const string& inputLine)
{
    ASSERT(!inputLine.empty());

    TestCommand command = parseInputLine(inputLine);
    const string& pathOrURL = command.pathOrURL;
    dumpPixelsForCurrentTest = command.shouldDumpPixels || dumpPixelsForAllTests;

    NSString *pathOrURLString = [NSString stringWithUTF8String:pathOrURL.c_str()];
    if (!pathOrURLString) {
        fprintf(stderr, "Failed to parse \"%s\" as UTF-8\n", pathOrURL.c_str());
        return;
    }

    NSURL *url;
    if ([pathOrURLString hasPrefix:@"http://"] || [pathOrURLString hasPrefix:@"https://"] || [pathOrURLString hasPrefix:@"file://"])
        url = [NSURL URLWithString:pathOrURLString];
    else
        url = [NSURL fileURLWithPath:pathOrURLString];
    if (!url) {
        fprintf(stderr, "Failed to parse \"%s\" as a URL\n", pathOrURL.c_str());
        return;
    }

    NSString *testPath = testPathFromURL(url);
    if (!testPath)
        testPath = [url absoluteString];
    NSString *informationString = [@"CRASHING TEST: " stringByAppendingString:testPath];
    WKSetCrashReportApplicationSpecificInformation((CFStringRef)informationString);

    const char* testURL([[url absoluteString] UTF8String]);
    
    resetWebViewToConsistentStateBeforeTesting();
#if !PLATFORM(IOS)
    changeWindowScaleIfNeeded(testURL);
#endif

    gTestRunner = TestRunner::create(testURL, command.expectedPixelHash);
    gTestRunner->setAllowedHosts(allowedHosts);
    gTestRunner->setCustomTimeout(command.timeout);
    topLoadingFrame = nil;
#if !PLATFORM(IOS)
    ASSERT(!draggingInfo); // the previous test should have called eventSender.mouseUp to drop!
    releaseAndZero(&draggingInfo);
#endif
    done = NO;

    sizeWebViewForCurrentTest();
    gTestRunner->setIconDatabaseEnabled(false);
    gTestRunner->clearAllApplicationCaches();

    if (disallowedURLs)
        CFSetRemoveAllValues(disallowedURLs);
    if (shouldLogFrameLoadDelegates(pathOrURL.c_str()))
        gTestRunner->setDumpFrameLoadCallbacks(true);

    if (shouldLogHistoryDelegates(pathOrURL.c_str()))
        [[mainFrame webView] setHistoryDelegate:historyDelegate];
    else
        [[mainFrame webView] setHistoryDelegate:nil];

    if (shouldEnableDeveloperExtras(pathOrURL.c_str())) {
        gTestRunner->setDeveloperExtrasEnabled(true);
        if (shouldDumpAsText(pathOrURL.c_str())) {
            gTestRunner->setDumpAsText(true);
            gTestRunner->setGeneratePixelResults(false);
        }
    }

#if PLATFORM(IOS)
    if (shouldMakeViewportFlexible(pathOrURL.c_str()))
        adjustWebDocumentForFlexibleViewport(gWebBrowserView, gWebScrollView);
#endif

    if ([WebHistory optionalSharedHistory])
        [WebHistory setOptionalSharedHistory:nil];

    lastMousePosition = NSZeroPoint;
    lastClickPosition = NSZeroPoint;

    [prevTestBFItem release];
    prevTestBFItem = [[[[mainFrame webView] backForwardList] currentItem] retain];

    auto& workQueue = WorkQueue::singleton();
    workQueue.clear();
    workQueue.setFrozen(false);

    bool ignoreWebCoreNodeLeaks = shouldIgnoreWebCoreNodeLeaks(testURL);
    if (ignoreWebCoreNodeLeaks)
        [WebCoreStatistics startIgnoringWebCoreNodeLeaks];

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [mainFrame loadRequest:[NSURLRequest requestWithURL:url]];
    [pool release];

    while (!done) {
        pool = [[NSAutoreleasePool alloc] init];
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 10, false);
        [pool release];
    }

#if PLATFORM(IOS)
    [(DumpRenderTree *)UIApp _waitForWebThread];
    WebThreadLockAfterDelegateCallbacksHaveCompleted();
#endif
    pool = [[NSAutoreleasePool alloc] init];
    [EventSendingController clearSavedEvents];
    [[mainFrame webView] setSelectedDOMRange:nil affinity:NSSelectionAffinityDownstream];

    workQueue.clear();

    if (gTestRunner->closeRemainingWindowsWhenComplete()) {
        NSArray* array = [DumpRenderTreeWindow openWindows];

        unsigned count = [array count];
        for (unsigned i = 0; i < count; i++) {
            NSWindow *window = [array objectAtIndex:i];

            // Don't try to close the main window
            if (window == [[mainFrame webView] window])
                continue;
            
#if !PLATFORM(IOS)
            WebView *webView = [[[window contentView] subviews] objectAtIndex:0];
#else
            ASSERT([[window contentView] isKindOfClass:[WebView class]]);
            WebView *webView = (WebView *)[window contentView];
#endif

            [webView close];
            [window close];
        }
    }

    // If developer extras enabled Web Inspector may have been open by the test.
    if (shouldEnableDeveloperExtras(pathOrURL.c_str())) {
        gTestRunner->closeWebInspector();
        gTestRunner->setDeveloperExtrasEnabled(false);
    }

    resetWebViewToConsistentStateBeforeTesting();

    // Loading an empty request synchronously replaces the document with a blank one, which is necessary
    // to stop timers, WebSockets and other activity that could otherwise spill output into next test's results.
    [mainFrame loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@""]]];

    [pool release];

    // We should only have our main window left open when we're done
    ASSERT(CFArrayGetCount(openWindowsRef) == 1);
    ASSERT(CFArrayGetValueAtIndex(openWindowsRef, 0) == [[mainFrame webView] window]);

    gTestRunner = nullptr;

    if (ignoreWebCoreNodeLeaks)
        [WebCoreStatistics stopIgnoringWebCoreNodeLeaks];

    if (gcBetweenTests)
        [WebCoreStatistics garbageCollectJavaScriptObjects];

    fputs("#EOF\n", stderr);
    fflush(stderr);
}

void displayWebView()
{
#if !PLATFORM(IOS)
    WebView *webView = [mainFrame webView];
    [webView display];

    // FIXME: Tracking repaints is not specific to Mac. We should enable such support on iOS.
    [webView setTracksRepaints:YES];
    [webView resetTrackedRepaints];
#else
    [gDrtWindow layoutTilesNow];
    [gDrtWindow setNeedsDisplayInRect:[gDrtWindow frame]];
    [CATransaction flush];
#endif
}

#if !PLATFORM(IOS)
@implementation DumpRenderTreeEvent

+ (NSPoint)mouseLocation
{
    return [[[mainFrame webView] window] convertBaseToScreen:lastMousePosition];
}

@end

@implementation DumpRenderTreeApplication

- (BOOL)isRunning
{
    // <rdar://problem/7686123> Java plug-in freezes unless NSApplication is running
    return YES;
}

@end
#endif
