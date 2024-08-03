/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef RenderThemeIOS_h
#define RenderThemeIOS_h

#if PLATFORM(IOS)

#include "RenderTheme.h"

namespace WebCore {
    
class RenderStyle;
class GraphicsContext;
struct AttachmentLayout;

class RenderThemeIOS final : public RenderTheme {
public:
    static Ref<RenderTheme> create();

    LengthBox popupInternalPaddingBox(const RenderStyle&) const override;

    static void adjustRoundBorderRadius(RenderStyle&, RenderBox&);

    static CFStringRef contentSizeCategory();

    WEBCORE_EXPORT static void setContentSizeCategory(const String&);

protected:
    FontCascadeDescription& cachedSystemFontDescription(CSSValueID systemFontID) const override;
    void updateCachedSystemFontDescription(CSSValueID, FontCascadeDescription&) const override;
    int baselinePosition(const RenderBox&) const override;

    bool isControlStyled(const RenderStyle&, const BorderData&, const FillLayer& background, const Color& backgroundColor) const override;

    // Methods for each appearance value.
    void adjustCheckboxStyle(StyleResolver&, RenderStyle&, const Element*) const override;
    bool paintCheckboxDecorations(const RenderObject&, const PaintInfo&, const IntRect&) override;

    void adjustRadioStyle(StyleResolver&, RenderStyle&, const Element*) const override;
    bool paintRadioDecorations(const RenderObject&, const PaintInfo&, const IntRect&) override;

    void adjustButtonStyle(StyleResolver&, RenderStyle&, const Element*) const override;
    bool paintButtonDecorations(const RenderObject&, const PaintInfo&, const IntRect&) override;
    bool paintPushButtonDecorations(const RenderObject&, const PaintInfo&, const IntRect&) override;
    void setButtonSize(RenderStyle&) const override;

    bool paintFileUploadIconDecorations(const RenderObject& inputRenderer, const RenderObject& buttonRenderer, const PaintInfo&, const IntRect&, Icon*, FileUploadDecorations) override;

    bool paintTextFieldDecorations(const RenderObject&, const PaintInfo&, const FloatRect&) override;
    bool paintTextAreaDecorations(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    void adjustMenuListButtonStyle(StyleResolver&, RenderStyle&, const Element*) const override;
    bool paintMenuListButtonDecorations(const RenderBox&, const PaintInfo&, const FloatRect&) override;

    void adjustSliderTrackStyle(StyleResolver&, RenderStyle&, const Element*) const override;
    bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;

    void adjustSliderThumbSize(RenderStyle&, const Element*) const override;
    bool paintSliderThumbDecorations(const RenderObject&, const PaintInfo&, const IntRect&) override;

    // Returns the repeat interval of the animation for the progress bar.
    double animationRepeatIntervalForProgressBar(RenderProgress&) const override;
    // Returns the duration of the animation for the progress bar.
    double animationDurationForProgressBar(RenderProgress&) const override;

    bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) override;

#if ENABLE(DATALIST_ELEMENT)
    IntSize sliderTickSize() const override;
    int sliderTickOffsetFromTrackCenter() const override;
#endif

    void adjustSearchFieldStyle(StyleResolver&, RenderStyle&, const Element*) const override;
    bool paintSearchFieldDecorations(const RenderObject&, const PaintInfo&, const IntRect&) override;

    Color platformActiveSelectionBackgroundColor() const override;
    Color platformInactiveSelectionBackgroundColor() const override;

#if ENABLE(TOUCH_EVENTS)
    Color platformTapHighlightColor() const override { return 0x4D1A1A1A; }
#endif

    bool shouldHaveSpinButton(const HTMLInputElement&) const override;
    bool shouldHaveCapsLockIndicator(const HTMLInputElement&) const override;

#if ENABLE(VIDEO)
    String mediaControlsStyleSheet() override;
    String mediaControlsScript() override;
#endif

#if ENABLE(ATTACHMENT_ELEMENT)
    LayoutSize attachmentIntrinsicSize(const RenderAttachment&) const override;
    int attachmentBaseline(const RenderAttachment&) const override;
    bool paintAttachment(const RenderObject&, const PaintInfo&, const IntRect&) override;
#endif

private:
    RenderThemeIOS();
    virtual ~RenderThemeIOS() { }

    const Color& shadowColor() const;
    FloatRect addRoundedBorderClip(const RenderObject& box, GraphicsContext&, const IntRect&);

    Color systemColor(CSSValueID) const override;

    String m_mediaControlsScript;
    String m_mediaControlsStyleSheet;

    mutable HashMap<int, Color> m_systemColorCache;
};

}

#endif // PLATFORM(IOS)
#endif // RenderThemeIOS_h
