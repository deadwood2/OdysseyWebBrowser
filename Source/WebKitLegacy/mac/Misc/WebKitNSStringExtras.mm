/*
 * Copyright (C) 2005, 2007 Apple Inc. All rights reserved.
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

#import "WebKitNSStringExtras.h"

#import <WebCore/CoreGraphicsSPI.h>
#import <WebCore/FontCascade.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/TextRun.h>
#import <WebCore/WebCoreNSStringExtras.h>
#import <WebKitLegacy/WebNSFileManagerExtras.h>
#import <WebKitLegacy/WebNSObjectExtras.h>
#import <unicode/uchar.h>
#import <sys/param.h>

#if PLATFORM(IOS)
#import <WebKitLegacy/DOM.h>
#import <WebKitLegacy/WebFrame.h>
#import <WebKitLegacy/WebFrameView.h>
#import <WebKitLegacy/WebViewPrivate.h>
#endif

NSString *WebKitLocalCacheDefaultsKey = @"WebKitLocalCache";
NSString *WebKitResourceLoadStatisticsDirectoryDefaultsKey = @"WebKitResourceLoadStatisticsDirectory";

using namespace WebCore;

@implementation NSString (WebKitExtras)

#if !PLATFORM(IOS)
static BOOL canUseFastRenderer(const UniChar *buffer, unsigned length)
{
    unsigned i;
    for (i = 0; i < length; i++) {
        UCharDirection direction = u_charDirection(buffer[i]);
        if (direction == U_RIGHT_TO_LEFT || direction > U_OTHER_NEUTRAL)
            return NO;
    }
    return YES;
}

- (void)_web_drawAtPoint:(NSPoint)point font:(NSFont *)font textColor:(NSColor *)textColor
{
    [self _web_drawAtPoint:point font:font textColor:textColor allowingFontSmoothing:YES];
}

- (void)_web_drawAtPoint:(NSPoint)point font:(NSFont *)font textColor:(NSColor *)textColor allowingFontSmoothing:(BOOL)fontSmoothingIsAllowed
{
    if (!font)
        return;

    unsigned length = [self length];
    Vector<UniChar, 2048> buffer(length);

    [self getCharacters:buffer.data()];

    if (canUseFastRenderer(buffer.data(), length)) {
        // The following is a half-assed attempt to match AppKit's rounding rules for drawAtPoint.
        // It's probably incorrect for high DPI.
        // If you change this, be sure to test all the text drawn this way in Safari, including
        // the status bar, bookmarks bar, tab bar, and activity window.
        point.y = CGCeiling(point.y);

        NSGraphicsContext *nsContext = [NSGraphicsContext currentContext];
        CGContextRef cgContext = static_cast<CGContextRef>([nsContext graphicsPort]);
        GraphicsContext graphicsContext(cgContext);    

        // Safari doesn't flip the NSGraphicsContext before calling WebKit, yet WebCore requires a flipped graphics context.
        BOOL flipped = [nsContext isFlipped];
        if (!flipped)
            CGContextScaleCTM(cgContext, 1, -1);

        FontCascade webCoreFont(FontPlatformData(reinterpret_cast<CTFontRef>(font), [font pointSize]), fontSmoothingIsAllowed ? AutoSmoothing : Antialiased);
        TextRun run(StringView(buffer.data(), length));

        CGFloat red;
        CGFloat green;
        CGFloat blue;
        CGFloat alpha;
        [[textColor colorUsingColorSpaceName:NSDeviceRGBColorSpace] getRed:&red green:&green blue:&blue alpha:&alpha];
        graphicsContext.setFillColor(Color(static_cast<float>(red * 255), static_cast<float>(green * 255.0f), static_cast<float>(blue * 255.0f), static_cast<float>(alpha * 255.0f)));

        webCoreFont.drawText(graphicsContext, run, FloatPoint(point.x, (flipped ? point.y : (-1 * point.y))));

        if (!flipped)
            CGContextScaleCTM(cgContext, 1, -1);
    } else {
        // The given point is on the baseline.
        if ([[NSView focusView] isFlipped])
            point.y -= [font ascender];
        else
            point.y += [font descender];

        [self drawAtPoint:point withAttributes:[NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, textColor, NSForegroundColorAttributeName, nil]];
    }
}

- (void)_web_drawDoubledAtPoint:(NSPoint)textPoint
             withTopColor:(NSColor *)topColor
              bottomColor:(NSColor *)bottomColor
                     font:(NSFont *)font
{
    // turn off font smoothing so translucent text draws correctly (Radar 3118455)
    [self _web_drawAtPoint:textPoint font:font textColor:bottomColor allowingFontSmoothing:NO];

    textPoint.y += 1;
    [self _web_drawAtPoint:textPoint font:font textColor:topColor allowingFontSmoothing:NO];
}

- (float)_web_widthWithFont:(NSFont *)font
{
    unsigned length = [self length];
    Vector<UniChar, 2048> buffer(length);

    [self getCharacters:buffer.data()];

    if (canUseFastRenderer(buffer.data(), length)) {
        FontCascade webCoreFont(FontPlatformData(reinterpret_cast<CTFontRef>(font), [font pointSize]));
        TextRun run(StringView(buffer.data(), length));
        return webCoreFont.width(run);
    }

    return [self sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil]].width;
}
#endif // !PLATFORM(IOS)

- (NSString *)_web_stringByAbbreviatingWithTildeInPath
{
    NSString *resolvedHomeDirectory = [NSHomeDirectory() stringByResolvingSymlinksInPath];
    NSString *path;
    
    if ([self hasPrefix:resolvedHomeDirectory]) {
        NSString *relativePath = [self substringFromIndex:[resolvedHomeDirectory length]];
        path = [NSHomeDirectory() stringByAppendingPathComponent:relativePath];
    } else {
        path = self;
    }
        
    return [path stringByAbbreviatingWithTildeInPath];
}

- (NSString *)_web_stringByStrippingReturnCharacters
{
    NSMutableString *newString = [[self mutableCopy] autorelease];
    [newString replaceOccurrencesOfString:@"\r" withString:@"" options:NSLiteralSearch range:NSMakeRange(0, [newString length])];
    [newString replaceOccurrencesOfString:@"\n" withString:@"" options:NSLiteralSearch range:NSMakeRange(0, [newString length])];
    return newString;
}

- (BOOL)_webkit_isCaseInsensitiveEqualToString:(NSString *)string
{
    return stringIsCaseInsensitiveEqualToString(self, string);
}

-(BOOL)_webkit_hasCaseInsensitivePrefix:(NSString *)prefix
{
    return hasCaseInsensitivePrefix(self, prefix);
}

-(BOOL)_webkit_hasCaseInsensitiveSuffix:(NSString *)suffix
{
    return hasCaseInsensitiveSuffix(self, suffix);
}

-(BOOL)_webkit_hasCaseInsensitiveSubstring:(NSString *)substring
{
    return hasCaseInsensitiveSubstring(self, substring);
}

-(NSString *)_webkit_filenameByFixingIllegalCharacters
{
    return filenameByFixingIllegalCharacters(self);
}

-(NSString *)_webkit_stringByTrimmingWhitespace
{
    NSMutableString *trimmed = [[self mutableCopy] autorelease];
    CFStringTrimWhitespace((CFMutableStringRef)trimmed);
    return trimmed;
}

- (NSString *)_webkit_stringByCollapsingNonPrintingCharacters
{
    NSMutableString *result = [NSMutableString string];
    static NSCharacterSet *charactersToTurnIntoSpaces = nil;
    static NSCharacterSet *charactersToNotTurnIntoSpaces = nil;
    
    if (charactersToTurnIntoSpaces == nil) {
        NSMutableCharacterSet *set = [[NSMutableCharacterSet alloc] init];
        [set addCharactersInRange:NSMakeRange(0x00, 0x21)];
        [set addCharactersInRange:NSMakeRange(0x7F, 0x01)];
        charactersToTurnIntoSpaces = [set copy];
        [set release];
        charactersToNotTurnIntoSpaces = [[charactersToTurnIntoSpaces invertedSet] retain];
    }
    
    unsigned length = [self length];
    unsigned position = 0;
    while (position != length) {
        NSRange nonSpace = [self rangeOfCharacterFromSet:charactersToNotTurnIntoSpaces
            options:0 range:NSMakeRange(position, length - position)];
        if (nonSpace.location == NSNotFound) {
            break;
        }

        NSRange space = [self rangeOfCharacterFromSet:charactersToTurnIntoSpaces
            options:0 range:NSMakeRange(nonSpace.location, length - nonSpace.location)];
        if (space.location == NSNotFound) {
            space.location = length;
        }

        if (space.location > nonSpace.location) {
            if (position != 0) {
                [result appendString:@" "];
            }
            [result appendString:[self substringWithRange:
                NSMakeRange(nonSpace.location, space.location - nonSpace.location)]];
        }

        position = space.location;
    }
    
    return result;
}

- (NSString *)_webkit_stringByCollapsingWhitespaceCharacters
{
    NSMutableString *result = [[NSMutableString alloc] initWithCapacity:[self length]];
    NSCharacterSet *spaces = [NSCharacterSet whitespaceAndNewlineCharacterSet];
    static NSCharacterSet *notSpaces = nil;

    if (notSpaces == nil)
        notSpaces = [[spaces invertedSet] retain];

    unsigned length = [self length];
    unsigned position = 0;
    while (position != length) {
        NSRange nonSpace = [self rangeOfCharacterFromSet:notSpaces options:0 range:NSMakeRange(position, length - position)];
        if (nonSpace.location == NSNotFound)
            break;

        NSRange space = [self rangeOfCharacterFromSet:spaces options:0 range:NSMakeRange(nonSpace.location, length - nonSpace.location)];
        if (space.location == NSNotFound)
            space.location = length;

        if (space.location > nonSpace.location) {
            if (position != 0)
                [result appendString:@" "];
            [result appendString:[self substringWithRange:NSMakeRange(nonSpace.location, space.location - nonSpace.location)]];
        }

        position = space.location;
    }

    return [result autorelease];
}

#if PLATFORM(MAC)
// FIXME: This is here only for binary compatibility with Safari 8 and earlier.
-(NSString *)_webkit_fixedCarbonPOSIXPath
{
    return self;
}
#endif

#if PLATFORM(IOS)
+ (NSString *)_web_stringWithData:(NSData *)data textEncodingName:(NSString *)textEncodingName
{
    return [WebFrame stringWithData:data textEncodingName:textEncodingName];
}
#endif

+ (NSString *)_webkit_localCacheDirectoryWithBundleIdentifier:(NSString*)bundleIdentifier
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *cacheDir = [defaults objectForKey:WebKitLocalCacheDefaultsKey];

    if (!cacheDir || ![cacheDir isKindOfClass:[NSString class]]) {
#if PLATFORM(IOS)
        cacheDir = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Caches"];
#else
        char cacheDirectory[MAXPATHLEN];
        size_t cacheDirectoryLen = confstr(_CS_DARWIN_USER_CACHE_DIR, cacheDirectory, MAXPATHLEN);
    
        if (cacheDirectoryLen)
            cacheDir = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:cacheDirectory length:cacheDirectoryLen - 1];
#endif
    }

    return [cacheDir stringByAppendingPathComponent:bundleIdentifier];
}

+ (NSString *)_webkit_localStorageDirectoryWithBundleIdentifier:(NSString*)bundleIdentifier
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *storageDirectory = [defaults objectForKey:WebKitResourceLoadStatisticsDirectoryDefaultsKey];

    if (!storageDirectory || ![storageDirectory isKindOfClass:[NSString class]]) {
        NSError *error;
        NSString *storageDirectory = [[[NSFileManager defaultManager] URLForDirectory:NSApplicationSupportDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:YES error:&error] path];
        
        if (!storageDirectory || ![storageDirectory isKindOfClass:[NSString class]])
            storageDirectory = [NSHomeDirectory() stringByAppendingPathComponent:@"Library/Application Support"];
    }

    return [storageDirectory stringByAppendingPathComponent:bundleIdentifier];
}

@end
