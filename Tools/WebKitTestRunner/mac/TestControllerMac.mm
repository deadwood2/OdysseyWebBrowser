/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "TestController.h"

#import "CrashReporterInfo.h"
#import "PlatformWebView.h"
#import "PoseAsClass.h"
#import "TestInvocation.h"
#import "WebKitTestRunnerPasteboard.h"
#import <WebKit/WKContextPrivate.h>
#import <WebKit/WKPageGroup.h>
#import <WebKit/WKStringCF.h>
#import <WebKit/WKURLCF.h>
#import <WebKit/_WKUserContentExtensionStore.h>
#import <WebKit/_WKUserContentExtensionStorePrivate.h>
#import <mach-o/dyld.h>

@interface NSSound ()
+ (void)_setAlertType:(NSUInteger)alertType;
@end

namespace WTR {

void TestController::notifyDone()
{
}

void TestController::platformInitialize()
{
    poseAsClass("WebKitTestRunnerPasteboard", "NSPasteboard");
    poseAsClass("WebKitTestRunnerEvent", "NSEvent");

    [NSSound _setAlertType:0];
}

void TestController::platformDestroy()
{
    [WebKitTestRunnerPasteboard releaseLocalPasteboards];
}

void TestController::initializeInjectedBundlePath()
{
    NSString *nsBundlePath = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"WebKitTestRunnerInjectedBundle.bundle"];
    m_injectedBundlePath.adopt(WKStringCreateWithCFString((CFStringRef)nsBundlePath));
}

void TestController::initializeTestPluginDirectory()
{
    m_testPluginDirectory.adopt(WKStringCreateWithCFString((CFStringRef)[[NSBundle mainBundle] bundlePath]));
}

void TestController::platformWillRunTest(const TestInvocation& testInvocation)
{
    setCrashReportApplicationSpecificInformationToURL(testInvocation.url());
}

static bool shouldUseThreadedScrolling(const TestInvocation& test)
{
    return test.urlContains("tiled-drawing/");
}

void TestController::platformResetPreferencesToConsistentValues()
{
#if WK_API_ENABLED
    __block bool doneRemoving = false;
    [[_WKUserContentExtensionStore defaultStore] removeContentExtensionForIdentifier:@"TestContentExtensions" completionHandler:^(NSError *error)
    {
        doneRemoving = true;
    }];
    platformRunUntil(doneRemoving, 0);
    [[_WKUserContentExtensionStore defaultStore] _removeAllContentExtensions];
#endif
}

void TestController::platformConfigureViewForTest(const TestInvocation& test)
{
    ViewOptions viewOptions;

    viewOptions.useThreadedScrolling = shouldUseThreadedScrolling(test);
    viewOptions.useRemoteLayerTree = shouldUseRemoteLayerTree();
    viewOptions.shouldShowWebView = shouldShowWebView();

    ensureViewSupportsOptions(viewOptions);

#if WK_API_ENABLED
    if (!test.urlContains("contentextensions/"))
        return;

    RetainPtr<CFURLRef> testURL = adoptCF(WKURLCopyCFURL(kCFAllocatorDefault, test.url()));
    NSURL *filterURL = [(NSURL *)testURL.get() URLByAppendingPathExtension:@"json"];

    NSStringEncoding encoding;
    NSString *contentExtensionString = [[NSString alloc] initWithContentsOfURL:filterURL usedEncoding:&encoding error:NULL];
    if (!contentExtensionString)
        return;
    
    __block bool doneCompiling = false;
    [[_WKUserContentExtensionStore defaultStore] compileContentExtensionForIdentifier:@"TestContentExtensions" encodedContentExtension:contentExtensionString completionHandler:^(_WKUserContentFilter *filter, NSError *error)
    {
        if (!error)
            WKPageGroupAddUserContentFilter(WKPageGetPageGroup(TestController::singleton().mainWebView()->page()), (__bridge WKUserContentFilterRef)filter);
        else
            NSLog(@"%@", [error helpAnchor]);
        doneCompiling = true;
    }];
    platformRunUntil(doneCompiling, 0);
#endif
}

void TestController::platformRunUntil(bool& done, double timeout)
{
    NSDate *endDate = (timeout > 0) ? [NSDate dateWithTimeIntervalSinceNow:timeout] : [NSDate distantFuture];

    while (!done && [endDate compare:[NSDate date]] == NSOrderedDescending)
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:endDate];
}

#if ENABLE(PLATFORM_FONT_LOOKUP)
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

static NSSet *systemHiddenFontFamilySet()
{
    static NSSet *fontFamilySet = [[NSSet setWithObjects:
        @".LucidaGrandeUI",
        nil] retain];

    return fontFamilySet;
}

static WKRetainPtr<WKArrayRef> generateWhitelist()
{
    WKMutableArrayRef result = WKMutableArrayCreate();
    for (NSString *fontFamily in allowedFontFamilySet()) {
        NSArray *fontsForFamily = [[NSFontManager sharedFontManager] availableMembersOfFontFamily:fontFamily];
        WKRetainPtr<WKStringRef> familyInFont = adoptWK(WKStringCreateWithUTF8CString([fontFamily UTF8String]));
        WKArrayAppendItem(result, familyInFont.get());
        for (NSArray *fontInfo in fontsForFamily) {
            // Font name is the first entry in the array.
            WKRetainPtr<WKStringRef> fontName = adoptWK(WKStringCreateWithUTF8CString([[fontInfo objectAtIndex:0] UTF8String]));
            WKArrayAppendItem(result, fontName.get());
        }
    }

    for (NSString *hiddenFontFamily in systemHiddenFontFamilySet())
        WKArrayAppendItem(result, WKStringCreateWithUTF8CString([hiddenFontFamily UTF8String]));

    return adoptWK(result);
}
#endif

void TestController::platformInitializeContext()
{
    // Testing uses a private session, which is memory only. However creating one instantiates a shared NSURLCache,
    // and if we haven't created one yet, the default one will be created on disk.
    // Making the shared cache memory-only avoids touching the file system.
    RetainPtr<NSURLCache> sharedCache =
        adoptNS([[NSURLCache alloc] initWithMemoryCapacity:1024 * 1024
                                      diskCapacity:0
                                          diskPath:nil]);
    [NSURLCache setSharedURLCache:sharedCache.get()];

#if ENABLE(PLATFORM_FONT_LOOKUP)
    WKContextSetFontWhitelist(m_context.get(), generateWhitelist().get());
#endif
}

void TestController::setHidden(bool hidden)
{
    NSWindow *window = [mainWebView()->platformView() window];
    if (!window)
        return;

    if (hidden)
        [window orderOut:nil];
    else
        [window makeKeyAndOrderFront:nil];
}

void TestController::runModal(PlatformWebView* view)
{
    NSWindow *window = [view->platformView() window];
    if (!window)
        return;
    [NSApp runModalForWindow:window];
}

const char* TestController::platformLibraryPathForTesting()
{
    return [[@"~/Library/Application Support/DumpRenderTree" stringByExpandingTildeInPath] UTF8String];
}

} // namespace WTR
