/*
 * Copyright (C) 2016-2019 Apple Inc. All rights reserved.
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
#import "WebPage.h"

#import "AttributedString.h"
#import "LoadParameters.h"
#import "PluginView.h"
#import "WKAccessibilityWebPageObjectBase.h"
#import "WebPageProxyMessages.h"
#import "WebPaymentCoordinator.h"
#import "WebRemoteObjectRegistry.h"
#import <WebCore/DictionaryLookup.h>
#import <WebCore/Editor.h>
#import <WebCore/EventHandler.h>
#import <WebCore/FocusController.h>
#import <WebCore/HTMLConverter.h>
#import <WebCore/HitTestResult.h>
#import <WebCore/NodeRenderStyle.h>
#import <WebCore/PaymentCoordinator.h>
#import <WebCore/PlatformMediaSessionManager.h>
#import <WebCore/RenderElement.h>
#import <WebCore/RenderObject.h>
#import <WebCore/TextIterator.h>

#if PLATFORM(COCOA)

namespace WebKit {
using namespace WebCore;

void WebPage::platformDidReceiveLoadParameters(const LoadParameters& loadParameters)
{
    m_dataDetectionContext = loadParameters.dataDetectionContext;
}

void WebPage::requestActiveNowPlayingSessionInfo(CallbackID callbackID)
{
    bool hasActiveSession = false;
    String title = emptyString();
    double duration = NAN;
    double elapsedTime = NAN;
    uint64_t uniqueIdentifier = 0;
    bool registeredAsNowPlayingApplication = false;
    if (auto* sharedManager = WebCore::PlatformMediaSessionManager::sharedManagerIfExists()) {
        hasActiveSession = sharedManager->hasActiveNowPlayingSession();
        title = sharedManager->lastUpdatedNowPlayingTitle();
        duration = sharedManager->lastUpdatedNowPlayingDuration();
        elapsedTime = sharedManager->lastUpdatedNowPlayingElapsedTime();
        uniqueIdentifier = sharedManager->lastUpdatedNowPlayingInfoUniqueIdentifier();
        registeredAsNowPlayingApplication = sharedManager->registeredAsNowPlayingApplication();
    }

    send(Messages::WebPageProxy::NowPlayingInfoCallback(hasActiveSession, registeredAsNowPlayingApplication, title, duration, elapsedTime, uniqueIdentifier, callbackID));
}
    
void WebPage::performDictionaryLookupAtLocation(const FloatPoint& floatPoint)
{
    if (auto* pluginView = pluginViewForFrame(&m_page->mainFrame())) {
        if (pluginView->performDictionaryLookupAtLocation(floatPoint))
            return;
    }
    
    // Find the frame the point is over.
    HitTestResult result = m_page->mainFrame().eventHandler().hitTestResultAtPoint(m_page->mainFrame().view()->windowToContents(roundedIntPoint(floatPoint)), HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::DisallowUserAgentShadowContent | HitTestRequest::AllowChildFrameContent);
    auto [range, options] = DictionaryLookup::rangeAtHitTestResult(result);
    if (!range)
        return;
    
    auto* frame = result.innerNonSharedNode() ? result.innerNonSharedNode()->document().frame() : &m_page->focusController().focusedOrMainFrame();
    if (!frame)
        return;
    
    performDictionaryLookupForRange(*frame, *range, options, TextIndicatorPresentationTransition::Bounce);
}

void WebPage::performDictionaryLookupForSelection(Frame& frame, const VisibleSelection& selection, TextIndicatorPresentationTransition presentationTransition)
{
    auto [selectedRange, options] = DictionaryLookup::rangeForSelection(selection);
    if (selectedRange)
        performDictionaryLookupForRange(frame, *selectedRange, options, presentationTransition);
}

void WebPage::performDictionaryLookupOfCurrentSelection()
{
    auto& frame = m_page->focusController().focusedOrMainFrame();
    performDictionaryLookupForSelection(frame, frame.selection().selection(), TextIndicatorPresentationTransition::BounceAndCrossfade);
}
    
void WebPage::performDictionaryLookupForRange(Frame& frame, Range& range, NSDictionary *options, TextIndicatorPresentationTransition presentationTransition)
{
    send(Messages::WebPageProxy::DidPerformDictionaryLookup(dictionaryPopupInfoForRange(frame, range, options, presentationTransition)));
}

DictionaryPopupInfo WebPage::dictionaryPopupInfoForRange(Frame& frame, Range& range, NSDictionary *options, TextIndicatorPresentationTransition presentationTransition)
{
    Editor& editor = frame.editor();
    editor.setIsGettingDictionaryPopupInfo(true);
    
    DictionaryPopupInfo dictionaryPopupInfo;
    if (range.text().stripWhiteSpace().isEmpty()) {
        editor.setIsGettingDictionaryPopupInfo(false);
        return dictionaryPopupInfo;
    }
    
    Vector<FloatQuad> quads;
    range.absoluteTextQuads(quads);
    if (quads.isEmpty()) {
        editor.setIsGettingDictionaryPopupInfo(false);
        return dictionaryPopupInfo;
    }
    
    IntRect rangeRect = frame.view()->contentsToWindow(quads[0].enclosingBoundingBox());
    
    const RenderStyle* style = range.startContainer().renderStyle();
    float scaledAscent = style ? style->fontMetrics().ascent() * pageScaleFactor() : 0;
    dictionaryPopupInfo.origin = FloatPoint(rangeRect.x(), rangeRect.y() + scaledAscent);
    dictionaryPopupInfo.options = options;

#if PLATFORM(MAC)

    NSAttributedString *nsAttributedString = editingAttributedStringFromRange(range, IncludeImagesInAttributedString::No);
    
    RetainPtr<NSMutableAttributedString> scaledNSAttributedString = adoptNS([[NSMutableAttributedString alloc] initWithString:[nsAttributedString string]]);
    
    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    
    [nsAttributedString enumerateAttributesInRange:NSMakeRange(0, [nsAttributedString length]) options:0 usingBlock:^(NSDictionary *attributes, NSRange range, BOOL *stop) {
        RetainPtr<NSMutableDictionary> scaledAttributes = adoptNS([attributes mutableCopy]);
        
        NSFont *font = [scaledAttributes objectForKey:NSFontAttributeName];
        if (font)
            font = [fontManager convertFont:font toSize:font.pointSize * pageScaleFactor()];
        if (font)
            [scaledAttributes setObject:font forKey:NSFontAttributeName];
        
        [scaledNSAttributedString addAttributes:scaledAttributes.get() range:range];
    }];

#endif // PLATFORM(MAC)
    
    TextIndicatorOptions indicatorOptions = TextIndicatorOptionUseBoundingRectAndPaintAllContentForComplexRanges;
    if (presentationTransition == TextIndicatorPresentationTransition::BounceAndCrossfade)
        indicatorOptions |= TextIndicatorOptionIncludeSnapshotWithSelectionHighlight;
    
    auto textIndicator = TextIndicator::createWithRange(range, indicatorOptions, presentationTransition);
    if (!textIndicator) {
        editor.setIsGettingDictionaryPopupInfo(false);
        return dictionaryPopupInfo;
    }
    
    dictionaryPopupInfo.textIndicator = textIndicator->data();
#if PLATFORM(MAC)
    dictionaryPopupInfo.attributedString = scaledNSAttributedString;
#endif // PLATFORM(MAC)
    
#if PLATFORM(MACCATALYST)
    dictionaryPopupInfo.attributedString = adoptNS([[NSMutableAttributedString alloc] initWithString:range.text()]);
#endif // PLATFORM(MACCATALYST)
    
    editor.setIsGettingDictionaryPopupInfo(false);
    return dictionaryPopupInfo;
}

void WebPage::accessibilityTransferRemoteToken(RetainPtr<NSData> remoteToken)
{
    IPC::DataReference dataToken = IPC::DataReference(reinterpret_cast<const uint8_t*>([remoteToken bytes]), [remoteToken length]);
    send(Messages::WebPageProxy::RegisterWebProcessAccessibilityToken(dataToken));
}

#if ENABLE(APPLE_PAY)
WebPaymentCoordinator* WebPage::paymentCoordinator()
{
    if (!m_page)
        return nullptr;
    auto& client = m_page->paymentCoordinator().client();
    return is<WebPaymentCoordinator>(client) ? downcast<WebPaymentCoordinator>(&client) : nullptr;
}
#endif

void WebPage::getContentsAsAttributedString(CompletionHandler<void(const AttributedString&)>&& completionHandler)
{
    auto* documentElement = m_page->mainFrame().document()->documentElement();
    if (!documentElement) {
        completionHandler({ });
        return;
    }

    NSDictionary* documentAttributes = nil;

    AttributedString result;
    result.string = attributedStringFromRange(rangeOfContents(*documentElement), &documentAttributes);
    result.documentAttributes = documentAttributes;

    completionHandler({ result });
}

void WebPage::setRemoteObjectRegistry(WebRemoteObjectRegistry* registry)
{
    m_remoteObjectRegistry = makeWeakPtr(registry);
}

WebRemoteObjectRegistry* WebPage::remoteObjectRegistry()
{
    return m_remoteObjectRegistry.get();
}

void WebPage::updateMockAccessibilityElementAfterCommittingLoad()
{
    auto* document = mainFrame()->document();
    [m_mockAccessibilityElement setHasMainFramePlugin:document ? document->isPluginDocument() : false];
}

RetainPtr<CFDataRef> WebPage::pdfSnapshotAtSize(IntRect rect, IntSize bitmapSize, SnapshotOptions options)
{
    Frame* coreFrame = m_mainFrame->coreFrame();
    if (!coreFrame)
        return nullptr;

    FrameView* frameView = coreFrame->view();
    if (!frameView)
        return nullptr;

    auto data = adoptCF(CFDataCreateMutable(kCFAllocatorDefault, 0));

    auto dataConsumer = adoptCF(CGDataConsumerCreateWithCFData(data.get()));
    auto mediaBox = CGRectMake(0, 0, bitmapSize.width(), bitmapSize.height());
    auto pdfContext = adoptCF(CGPDFContextCreate(dataConsumer.get(), &mediaBox, nullptr));

    int64_t remainingHeight = bitmapSize.height();
    int64_t nextRectY = rect.y();
    while (remainingHeight > 0) {
        // PDFs have a per-page height limit of 200 inches at 72dpi.
        // We'll export one PDF page at a time, up to that maximum height.
        static const int64_t maxPageHeight = 72 * 200;
        bitmapSize.setHeight(std::min(remainingHeight, maxPageHeight));
        rect.setHeight(bitmapSize.height());
        rect.setY(nextRectY);

        CGRect mediaBox = CGRectMake(0, 0, bitmapSize.width(), bitmapSize.height());
        auto mediaBoxData = adoptCF(CFDataCreate(NULL, (const UInt8 *)&mediaBox, sizeof(CGRect)));
        auto dictionary = (CFDictionaryRef)@{
            (NSString *)kCGPDFContextMediaBox : (NSData *)mediaBoxData.get()
        };

        CGPDFContextBeginPage(pdfContext.get(), dictionary);

        GraphicsContext graphicsContext { pdfContext.get() };
        graphicsContext.scale({ 1, -1 });
        graphicsContext.translate(0, -bitmapSize.height());

        paintSnapshotAtSize(rect, bitmapSize, options, *coreFrame, *frameView, graphicsContext);

        CGPDFContextEndPage(pdfContext.get());

        nextRectY += bitmapSize.height();
        remainingHeight -= maxPageHeight;
    }

    CGPDFContextClose(pdfContext.get());

    return data;
}

} // namespace WebKit

#endif // PLATFORM(COCOA)
