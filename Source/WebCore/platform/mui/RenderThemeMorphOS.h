/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef RenderThemeMorphOS_h
#define RenderThemeMorphOS_h

#include "RenderTheme.h"
#include "BALBase.h"

namespace WebCore {

class RenderThemeBal : public RenderTheme {
public:
    static Ref<RenderTheme> create();
    virtual ~RenderThemeBal();

    virtual String extraDefaultStyleSheet() override;

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet() override;
    virtual String formatMediaControlsRemainingTime(float currentTime, float duration) const override;
#endif
    virtual bool supportsHover(const RenderStyle*) const { return true; }

    virtual double caretBlinkInterval() const override;

    virtual bool paintCheckbox(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual void setCheckboxSize(RenderStyle&) const override;
    virtual bool paintRadio(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual void setRadioSize(RenderStyle&) const override;
    virtual bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual void adjustMenuListStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual void adjustSliderThumbSize(RenderStyle&, Element*) const override;
    virtual bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;

#if ENABLE(DATALIST_ELEMENT)
    virtual IntSize sliderTickSize() const override;
    virtual int sliderTickOffsetFromTrackCenter() const override;
    virtual LayoutUnit sliderTickSnappingThreshold() const override;
#endif

    virtual bool supportsDataListUI(const AtomicString&) const override;

#if ENABLE(PROGRESS_ELEMENT)
    virtual void adjustProgressBarStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual double animationRepeatIntervalForProgressBar(RenderProgress*) const override;
    virtual double animationDurationForProgressBar(RenderProgress*) const override;
#endif

#if ENABLE(TOUCH_EVENTS)
    virtual Color platformTapHighlightColor() const override;
#endif

    virtual Color platformFocusRingColor() const override;
    virtual bool supportsFocusRing(const RenderStyle& style) const  override{ return style.hasAppearance(); }

    virtual void adjustButtonStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual void adjustTextFieldStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintTextField(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    virtual void adjustTextAreaStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    virtual void adjustSearchFieldStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual void adjustSearchFieldCancelButtonStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintSearchField(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintSearchFieldCancelButton(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void adjustMenuListButtonStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintMenuListButtonDecorations(const RenderObject&, const PaintInfo&, const FloatRect&) override;
    virtual void adjustCheckboxStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual void adjustRadioStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    virtual void adjustMediaControlStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual void adjustSliderTrackStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintMediaFullscreenButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaVolumeSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaVolumeSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaPlayButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaMuteButton(const RenderObject&, const PaintInfo&, const IntRect&) override;

    // The platform selection color.
    virtual Color platformActiveSelectionBackgroundColor() const override;
    virtual Color platformInactiveSelectionBackgroundColor() const override;
    virtual Color platformActiveSelectionForegroundColor() const override;
    virtual Color platformInactiveSelectionForegroundColor() const override;

    // List Box selection color
    virtual Color platformActiveListBoxSelectionBackgroundColor() const override;
    virtual Color platformActiveListBoxSelectionForegroundColor() const override;
    virtual Color platformInactiveListBoxSelectionBackgroundColor() const override;
    virtual Color platformInactiveListBoxSelectionForegroundColor() const override;

    // Highlighting colors for TextMatches.
    virtual Color platformActiveTextSearchHighlightColor() const override;
    virtual Color platformInactiveTextSearchHighlightColor() const override;

    WTF::String fileListNameForWidth(const Vector<String>&, const WebCore::Font&, int) const;

protected:
    virtual void updateCachedSystemFontDescription(CSSValueID systemFontID, FontDescription&) const override;

private:
    static const String& defaultGUIFont();

    // The default variable-width font size. We use this as the default font
    // size for the "system font", and as a base size (which we then shrink) for
    // form control fonts.
    static float defaultFontSize;

    RenderThemeBal();
    void setButtonStyle(RenderStyle&) const;
    void calculateButtonSize(RenderStyle&) const;

    void paintMenuListButtonGradientAndArrow(GraphicsContext*, const RenderObject&, IntRect buttonRect, const Path& clipPath);
    bool paintTextFieldOrTextAreaOrSearchField(const RenderObject&, const PaintInfo&, const IntRect&);
    bool paintSliderTrackRect(const RenderObject&, const PaintInfo&, const IntRect&);
    bool paintSliderTrackRect(const RenderObject&, const PaintInfo&, const IntRect&, RGBA32 strokeColorStart,
                RGBA32 strokeColorEnd, RGBA32 fillColorStart, RGBA32 fillColorEnd);

};

} // namespace WebCore

#endif // RenderThemeBal_h
