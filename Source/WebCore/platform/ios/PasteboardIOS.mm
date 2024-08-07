/*
 * Copyright (C) 2007, 2008, 2012, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "Pasteboard.h"

#import "DragData.h"
#import "Image.h"
#import "NotImplemented.h"
#import "PasteboardStrategy.h"
#import "PlatformPasteboard.h"
#import "PlatformStrategies.h"
#import "RuntimeEnabledFeatures.h"
#import "SharedBuffer.h"
#import "URL.h"
#import "UTIUtilities.h"
#import "WebNSAttributedStringExtras.h"
#import <MobileCoreServices/MobileCoreServices.h>
#import <wtf/text/StringHash.h>

@interface NSAttributedString (NSAttributedStringKitAdditions)
- (id)initWithRTF:(NSData *)data documentAttributes:(NSDictionary **)dict;
- (id)initWithRTFD:(NSData *)data documentAttributes:(NSDictionary **)dict;
- (NSData *)RTFFromRange:(NSRange)range documentAttributes:(NSDictionary *)dict;
- (NSData *)RTFDFromRange:(NSRange)range documentAttributes:(NSDictionary *)dict;
- (BOOL)containsAttachments;
@end

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)

Pasteboard::Pasteboard(const String& pasteboardName)
    : m_pasteboardName(pasteboardName)
    , m_changeCount(platformStrategies()->pasteboardStrategy()->changeCount(pasteboardName))
{
}

void Pasteboard::setDragImage(DragImage, const IntPoint&)
{
    notImplemented();
}

std::unique_ptr<Pasteboard> Pasteboard::createForDragAndDrop()
{
    return std::make_unique<Pasteboard>("data interaction pasteboard");
}

std::unique_ptr<Pasteboard> Pasteboard::createForDragAndDrop(const DragData& dragData)
{
    return std::make_unique<Pasteboard>(dragData.pasteboardName());
}

#endif

static long changeCountForPasteboard(const String& pasteboardName = { })
{
    return platformStrategies()->pasteboardStrategy()->changeCount(pasteboardName);
}

// FIXME: Does this need to be declared in the header file?
WEBCORE_EXPORT NSString *WebArchivePboardType = @"Apple Web Archive pasteboard type";

Pasteboard::Pasteboard()
    : m_changeCount(0)
{
}

Pasteboard::Pasteboard(long changeCount)
    : m_changeCount(changeCount)
{
}

void Pasteboard::writeMarkup(const String&)
{
}

std::unique_ptr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    return std::make_unique<Pasteboard>(changeCountForPasteboard());
}

void Pasteboard::write(const PasteboardWebContent& content)
{
    platformStrategies()->pasteboardStrategy()->writeToPasteboard(content, m_pasteboardName);
}

String Pasteboard::resourceMIMEType(NSString *mimeType)
{
    return UTIFromMIMEType(mimeType);
}

void Pasteboard::write(const PasteboardImage& pasteboardImage)
{
    platformStrategies()->pasteboardStrategy()->writeToPasteboard(pasteboardImage, m_pasteboardName);
}

void Pasteboard::writePlainText(const String& text, SmartReplaceOption)
{
    // FIXME: We vend "public.text" here for backwards compatibility with pre-iOS 11 apps. In the future, we should stop vending this UTI,
    // and instead set data for concrete plain text types. See <https://bugs.webkit.org/show_bug.cgi?id=173317>.
    platformStrategies()->pasteboardStrategy()->writeToPasteboard(kUTTypeText, text, m_pasteboardName);
}

void Pasteboard::write(const PasteboardURL& pasteboardURL)
{
    platformStrategies()->pasteboardStrategy()->writeToPasteboard(pasteboardURL, m_pasteboardName);
}

void Pasteboard::writeTrustworthyWebURLsPboardType(const PasteboardURL&)
{
    // A trustworthy URL pasteboard type needs to be decided on
    // before we allow calls to this function. A page data transfer
    // should not use the same pasteboard type as this function for
    // URLs.
    ASSERT_NOT_REACHED();
}

bool Pasteboard::canSmartReplace()
{
    return false;
}

void Pasteboard::read(PasteboardPlainText& text)
{
    PasteboardStrategy& strategy = *platformStrategies()->pasteboardStrategy();
    text.text = strategy.readStringFromPasteboard(0, kUTTypeURL, m_pasteboardName);
    if (!text.text.isNull() && !text.text.isEmpty()) {
        text.isURL = true;
        return;
    }

    text.text = strategy.readStringFromPasteboard(0, kUTTypeText, m_pasteboardName);
    if (text.text.isEmpty())
        text.text = strategy.readStringFromPasteboard(0, kUTTypePlainText, m_pasteboardName);

    text.isURL = false;
}

static NSArray* supportedImageTypes()
{
    return @[(id)kUTTypePNG, (id)kUTTypeTIFF, (id)kUTTypeJPEG, (id)kUTTypeGIF];
}

Pasteboard::ReaderResult Pasteboard::readPasteboardWebContentDataForType(PasteboardWebContentReader& reader, PasteboardStrategy& strategy, NSString *type, int itemIndex)
{
    if ([type isEqualToString:WebArchivePboardType]) {
        auto buffer = strategy.readBufferFromPasteboard(itemIndex, WebArchivePboardType, m_pasteboardName);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return buffer && reader.readWebArchive(*buffer) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    if ([type isEqualToString:(NSString *)kUTTypeHTML]) {
        String htmlString = strategy.readStringFromPasteboard(itemIndex, kUTTypeHTML, m_pasteboardName);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return !htmlString.isNull() && reader.readHTML(htmlString) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    if ([type isEqualToString:(NSString *)kUTTypeFlatRTFD]) {
        RefPtr<SharedBuffer> buffer = strategy.readBufferFromPasteboard(itemIndex, kUTTypeFlatRTFD, m_pasteboardName);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return buffer && reader.readRTFD(*buffer) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    if ([type isEqualToString:(NSString *)kUTTypeRTF]) {
        RefPtr<SharedBuffer> buffer = strategy.readBufferFromPasteboard(itemIndex, kUTTypeRTF, m_pasteboardName);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return buffer && reader.readRTF(*buffer) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    if ([supportedImageTypes() containsObject:type]) {
        RefPtr<SharedBuffer> buffer = strategy.readBufferFromPasteboard(itemIndex, type, m_pasteboardName);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return buffer && reader.readImage(buffer.releaseNonNull(), type) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    if ([type isEqualToString:(NSString *)kUTTypeURL]) {
        String title;
        URL url = strategy.readURLFromPasteboard(itemIndex, kUTTypeURL, m_pasteboardName, title);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return !url.isNull() && reader.readURL(url, title) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    if (UTTypeConformsTo((CFStringRef)type, kUTTypePlainText)) {
        String string = strategy.readStringFromPasteboard(itemIndex, kUTTypePlainText, m_pasteboardName);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return !string.isNull() && reader.readPlainText(string) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    if (UTTypeConformsTo((CFStringRef)type, kUTTypeText)) {
        String string = strategy.readStringFromPasteboard(itemIndex, kUTTypeText, m_pasteboardName);
        if (m_changeCount != changeCount())
            return ReaderResult::PasteboardWasChangedExternally;
        return !string.isNull() && reader.readPlainText(string) ? ReaderResult::ReadType : ReaderResult::DidNotReadType;
    }

    return ReaderResult::DidNotReadType;
}

void Pasteboard::read(PasteboardWebContentReader& reader)
{
    reader.contentOrigin = readOrigin();
    if (respectsUTIFidelities()) {
        readRespectingUTIFidelities(reader);
        return;
    }

    PasteboardStrategy& strategy = *platformStrategies()->pasteboardStrategy();

    int numberOfItems = strategy.getPasteboardItemsCount(m_pasteboardName);

    if (!numberOfItems)
        return;

    NSArray *types = supportedWebContentPasteboardTypes();
    int numberOfTypes = [types count];

    for (int i = 0; i < numberOfItems; i++) {
        for (int typeIndex = 0; typeIndex < numberOfTypes; typeIndex++) {
            auto itemResult = readPasteboardWebContentDataForType(reader, strategy, [types objectAtIndex:typeIndex], i);
            if (itemResult == ReaderResult::PasteboardWasChangedExternally)
                return;
            if (itemResult == ReaderResult::ReadType)
                break;
        }
    }
}

bool Pasteboard::respectsUTIFidelities() const
{
    // For now, data interaction is the only feature that uses item-provider-based pasteboard representations.
    // In the future, we may need to consult the client layer to determine whether or not the pasteboard supports
    // item types ranked by fidelity.
    return m_pasteboardName == "data interaction pasteboard";
}

void Pasteboard::readRespectingUTIFidelities(PasteboardWebContentReader& reader)
{
    ASSERT(respectsUTIFidelities());
    auto& strategy = *platformStrategies()->pasteboardStrategy();
    for (NSUInteger index = 0, numberOfItems = strategy.getPasteboardItemsCount(m_pasteboardName); index < numberOfItems; ++index) {
#if ENABLE(ATTACHMENT_ELEMENT)
        auto info = strategy.informationForItemAtIndex(index, m_pasteboardName);
        bool canReadAttachment = RuntimeEnabledFeatures::sharedFeatures().attachmentElementEnabled() && !info.pathForFileUpload.isEmpty();
        if (canReadAttachment && info.preferredPresentationStyle == PasteboardItemPresentationStyle::Attachment) {
            reader.readFilePaths({ info.pathForFileUpload });
            continue;
        }
#endif
        // Try to read data from each type identifier that this pasteboard item supports, and WebKit also recognizes. Type identifiers are
        // read in order of fidelity, as specified by each pasteboard item.
        Vector<String> typesForItemInOrderOfFidelity;
        strategy.getTypesByFidelityForItemAtIndex(typesForItemInOrderOfFidelity, index, m_pasteboardName);
        ReaderResult result = ReaderResult::DidNotReadType;
        for (auto& type : typesForItemInOrderOfFidelity) {
            result = readPasteboardWebContentDataForType(reader, strategy, type, index);
            if (result == ReaderResult::PasteboardWasChangedExternally)
                return;
            if (result == ReaderResult::ReadType)
                break;
        }
#if ENABLE(ATTACHMENT_ELEMENT)
        if (canReadAttachment && result == ReaderResult::DidNotReadType)
            reader.readFilePaths({ info.pathForFileUpload });
#endif
    }
}

NSArray *Pasteboard::supportedWebContentPasteboardTypes()
{
    return @[(id)WebArchivePboardType, (id)kUTTypeFlatRTFD, (id)kUTTypeRTF, (id)kUTTypeHTML, (id)kUTTypePNG, (id)kUTTypeTIFF, (id)kUTTypeJPEG, (id)kUTTypeGIF, (id)kUTTypeURL, (id)kUTTypeText];
}

NSArray *Pasteboard::supportedFileUploadPasteboardTypes()
{
    return @[ (NSString *)kUTTypeContent, (NSString *)kUTTypeZipArchive, (NSString *)kUTTypeFolder ];
}

bool Pasteboard::hasData()
{
    return !!platformStrategies()->pasteboardStrategy()->getPasteboardItemsCount(m_pasteboardName);
}

static String utiTypeFromCocoaType(NSString *type)
{
    RetainPtr<CFStringRef> utiType = adoptCF(UTTypeCreatePreferredIdentifierForTag(kUTTagClassMIMEType, (CFStringRef)type, NULL));
    if (!utiType)
        return String();
    return String(adoptCF(UTTypeCopyPreferredTagWithClass(utiType.get(), kUTTagClassMIMEType)).get());
}

static RetainPtr<NSString> cocoaTypeFromHTMLClipboardType(const String& type)
{
    if (NSString *platformType = PlatformPasteboard::platformPasteboardTypeForSafeTypeForDOMToReadAndWrite(type)) {
        if (platformType.length)
            return platformType;
    }

    // Try UTI now.
    if (NSString *utiType = utiTypeFromCocoaType(type))
        return utiType;

    // No mapping, just pass the whole string though.
    return (NSString *)type;
}

void Pasteboard::clear(const String& type)
{
    // Since UIPasteboard enforces changeCount itself on writing, we don't check it here.

    RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type);
    if (!cocoaType)
        return;

    platformStrategies()->pasteboardStrategy()->writeToPasteboard(cocoaType.get(), String(), m_pasteboardName);
}

void Pasteboard::clear()
{
    platformStrategies()->pasteboardStrategy()->writeToPasteboard(String(), String(), m_pasteboardName);
}

String Pasteboard::readPlatformValueAsString(const String& domType, long changeCount, const String& pasteboardName)
{
    PasteboardStrategy& strategy = *platformStrategies()->pasteboardStrategy();

    int numberOfItems = strategy.getPasteboardItemsCount(pasteboardName);

    if (!numberOfItems)
        return String();

    // Grab the value off the pasteboard corresponding to the cocoaType.
    RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(domType);

    NSString *cocoaValue = nil;

    if ([cocoaType isEqualToString:(NSString *)kUTTypeURL]) {
        String title;
        URL url = strategy.readURLFromPasteboard(0, kUTTypeURL, pasteboardName, title);
        if (!url.isNull())
            cocoaValue = [(NSURL *)url absoluteString];
    } else if ([cocoaType isEqualToString:(NSString *)kUTTypePlainText]) {
        String value = strategy.readStringFromPasteboard(0, kUTTypePlainText, pasteboardName);
        if (!value.isNull())
            cocoaValue = [(NSString *)value precomposedStringWithCanonicalMapping];
    } else if (cocoaType)
        cocoaValue = (NSString *)strategy.readStringFromPasteboard(0, cocoaType.get(), pasteboardName);

    // Enforce changeCount ourselves for security. We check after reading instead of before to be
    // sure it doesn't change between our testing the change count and accessing the data.
    if (cocoaValue && changeCount == changeCountForPasteboard(pasteboardName))
        return cocoaValue;

    return String();
}

void Pasteboard::addHTMLClipboardTypesForCocoaType(ListHashSet<String>& resultTypes, const String& cocoaType)
{
    // UTI may not do these right, so make sure we get the right, predictable result.
    if ([cocoaType isEqualToString:(NSString *)kUTTypePlainText]
        || [cocoaType isEqualToString:(NSString *)kUTTypeUTF8PlainText]
        || [cocoaType isEqualToString:(NSString *)kUTTypeUTF16PlainText]) {
        resultTypes.add(ASCIILiteral("text/plain"));
        return;
    }
    if ([cocoaType isEqualToString:(NSString *)kUTTypeURL]) {
        resultTypes.add(ASCIILiteral("text/uri-list"));
        return;
    }
    if ([cocoaType isEqualToString:(NSString *)kUTTypeHTML]) {
        resultTypes.add(ASCIILiteral("text/html"));
        // We don't return here for App compatibility.
    }
    if (Pasteboard::shouldTreatCocoaTypeAsFile(cocoaType))
        return;
    String utiType = utiTypeFromCocoaType(cocoaType);
    if (!utiType.isEmpty()) {
        resultTypes.add(utiType);
        return;
    }
    // No mapping, just pass the whole string though.
    resultTypes.add(cocoaType);
}

void Pasteboard::writeString(const String& type, const String& data)
{
    RetainPtr<NSString> cocoaType = cocoaTypeFromHTMLClipboardType(type);
    if (!cocoaType)
        return;

    platformStrategies()->pasteboardStrategy()->writeToPasteboard(cocoaType.get(), data, m_pasteboardName);
}

Vector<String> Pasteboard::readFilePaths()
{
    Vector<String> filePaths;
    auto& strategy = *platformStrategies()->pasteboardStrategy();
    for (NSUInteger index = 0, numberOfItems = strategy.getPasteboardItemsCount(m_pasteboardName); index < numberOfItems; ++index) {
        // Currently, drag and drop is the only case on iOS where the "pasteboard" may contain file paths.
        auto filePath = strategy.informationForItemAtIndex(index, m_pasteboardName).pathForFileUpload;
        if (!filePath.isEmpty())
            filePaths.append(WTFMove(filePath));
    }
    return filePaths;
}

}
