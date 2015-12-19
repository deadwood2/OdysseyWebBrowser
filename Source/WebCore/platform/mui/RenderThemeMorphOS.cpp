/*
 * Copyright (C) 2006, 2007 Apple Inc.
 * Copyright (C) 2009 Google Inc.
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

#include "config.h"
#include "RenderThemeMorphOS.h"

#include "CSSValueKeywords.h"
#include "Frame.h"
#include "HTMLMediaElement.h"
#include "HostWindow.h"
#include "InputType.h"
#include "InputTypeNames.h"
#include "MediaControlElements.h"
#if ENABLE(VIDEO)
#include "MediaPlayerPrivateMorphOS.h"
#endif
#include "Page.h"
#include "PaintInfo.h"
#include "RenderFullScreen.h"
#include "RenderProgress.h"
#include "RenderSlider.h"
#include "RenderView.h"
#include "UserAgentStyleSheets.h"
#include "LocalizedStrings.h"
#include "StringTruncator.h"
#include "wtf/text/CString.h"

#include <proto/dos.h>

#include <clib/debug_protos.h>
#define D(x)

extern char *utf8_to_local(const char *);

namespace WebCore {

// Sizes (unit px)
const unsigned smallRadius = 1;
const unsigned largeRadius = 3;
const unsigned lineWidth = 1;
const float marginSize = 4;
const float mediaControlsHeight = 32;
const float mediaSliderThumbWidth = 9;
const float mediaSliderThumbHeight = 22;
const float mediaVolumeSliderThumbWidth = 25;
const float mediaVolumeSliderThumbHeight = 25;
const float mediaSliderThumbRadius = 5;
const float sliderThumbWidth = 15;
const float sliderThumbHeight = 25;

// Checkbox check scalers
const float checkboxLeftX = 7 / 40.0;
const float checkboxLeftY = 1 / 2.0;
const float checkboxMiddleX = 19 / 50.0;
const float checkboxMiddleY = 7 / 25.0;
const float checkboxRightX = 33 / 40.0;
const float checkboxRightY = 1 / 5.0;
const float checkboxStrokeThickness = 6.5;

// Radio button scaler
const float radioButtonCheckStateScaler = 7 / 30.0;

// Multipliers
const unsigned paddingDivisor = 5;
const unsigned fullScreenEnlargementFactor = 2;
const float scaleFactorThreshold = 2.0;

// Colors
const RGBA32 caretBottom = 0xff2163bf;
const RGBA32 caretTop = 0xff69a5fa;

const RGBA32 regularBottom = 0xffdcdee4;
const RGBA32 regularTop = 0xfff7f2ee;
const RGBA32 hoverBottom = 0xffb5d3fc;
const RGBA32 hoverTop = 0xffcceaff;
const RGBA32 depressedBottom = 0xff3388ff;
const RGBA32 depressedTop = 0xff66a0f2;
const RGBA32 disabledBottom = 0xffe7e7e7;
const RGBA32 disabledTop = 0xffefefef;

const RGBA32 regularBottomOutline = 0xff6e7073;
const RGBA32 regularTopOutline = 0xffb9b8b8;
const RGBA32 hoverBottomOutline = 0xff2163bf;
const RGBA32 hoverTopOutline = 0xff69befa;
const RGBA32 depressedBottomOutline = 0xff0c3d81;
const RGBA32 depressedTopOutline = 0xff1d4d70;
const RGBA32 disabledOutline = 0xffd5d9de;

const RGBA32 progressRegularBottom = caretTop;
const RGBA32 progressRegularTop = caretBottom;

const RGBA32 rangeSliderRegularBottom = 0xfff6f2ee;
const RGBA32 rangeSliderRegularTop = 0xffdee0e5;
const RGBA32 rangeSliderRollBottom = 0xffc9e8fe;
const RGBA32 rangeSliderRollTop = 0xffb5d3fc;

const RGBA32 rangeSliderRegularBottomOutline = 0xffb9babd;
const RGBA32 rangeSliderRegularTopOutline = 0xffb7b7b7;
const RGBA32 rangeSliderRollBottomOutline = 0xff67abe0;
const RGBA32 rangeSliderRollTopOutline = 0xff69adf9;

const RGBA32 dragRegularLight = 0xfffdfdfd;
const RGBA32 dragRegularDark = 0xffbababa;
const RGBA32 dragRollLight = 0xfff2f2f2;
const RGBA32 dragRollDark = 0xff69a8ff;

const RGBA32 selection = 0xff2b8fff;

const RGBA32 blackPen = Color::black;
const RGBA32 focusRingPen = 0xff7dadd9;

float RenderThemeBal::defaultFontSize = 16;

// We aim to match IE here.
// -IE uses a font based on the encoding as the default font for form controls.
// -Gecko uses MS Shell Dlg (actually calls GetStockObject(DEFAULT_GUI_FONT),
// which returns MS Shell Dlg)
// -Safari uses Lucida Grande.
//
// FIXME: The only case where we know we don't match IE is for ANSI encodings.
// IE uses MS Shell Dlg there, which we render incorrectly at certain pixel
// sizes (e.g. 15px). So we just use Arial for now.
const String& RenderThemeBal::defaultGUIFont()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(String, fontFace, (ASCIILiteral("Arial")));
    return fontFace;
}

static Ref<Gradient> createLinearGradient(RGBA32 top, RGBA32 bottom, const IntPoint& a, const IntPoint& b)
{
    Ref<Gradient> gradient = Gradient::create(a, b);
    gradient.get().addColorStop(0.0, Color(top));
    gradient.get().addColorStop(1.0, Color(bottom));
    return gradient;
}

static Path roundedRectForBorder(const RenderObject& object, const FloatRect& rect)
{
    RenderStyle& style = object.style();
    LengthSize topLeftRadius = style.borderTopLeftRadius();
    LengthSize topRightRadius = style.borderTopRightRadius();
    LengthSize bottomLeftRadius = style.borderBottomLeftRadius();
    LengthSize bottomRightRadius = style.borderBottomRightRadius();

    Path roundedRect;
    roundedRect.addRoundedRect(rect, IntSize(bottomRightRadius.width().value(), bottomRightRadius.height().value()));
    return roundedRect;
}

static RenderSlider* determineRenderSlider(RenderObject* object)
{
    ASSERT(object->isSliderThumb());
    // The RenderSlider is an ancestor of the slider thumb.
    while (object && !object->isSlider())
        object = object->parent();
    return downcast<RenderSlider>(object);
}

static float determineFullScreenMultiplier(Element* element)
{
    float fullScreenMultiplier = 1.0;
    UNUSED_PARAM(element);

#if 0 //ENABLE(FULLSCREEN_API) && ENABLE(VIDEO)
    if (element && element->document()->webkitIsFullScreen() && element->document()->webkitCurrentFullScreenElement() == parentMediaElement(element)) {
        if (element->document()->page()->deviceScaleFactor() < scaleFactorThreshold)
            fullScreenMultiplier = fullScreenEnlargementFactor;

        // The way the BlackBerry port implements the FULLSCREEN_API for media elements
        // might result in the controls being oversized, proportionally to the current page
        // scale. That happens because the fullscreen element gets sized to be as big as the
        // viewport size, and the viewport size might get outstretched to fit to the screen dimensions.
        // To fix that, lets strips out the Page scale factor from the media controls multiplier.
        float scaleFactor = element->document()->view()->hostWindow()->platformPageClient()->currentZoomFactor();
        static ViewportArguments defaultViewportArguments;
        float scaleFactorFudge = 1 / element->document()->page()->deviceScaleFactor();
        fullScreenMultiplier /= scaleFactor * scaleFactorFudge;
    }
#endif
    return fullScreenMultiplier;
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    UNUSED_PARAM(page);
    static RenderTheme& theme = RenderThemeBal::create().leakRef();
    return &theme;
}

Ref<RenderTheme> RenderThemeBal::create()
{
    return adoptRef(*new RenderThemeBal());
}

RenderThemeBal::RenderThemeBal()
{
}

RenderThemeBal::~RenderThemeBal()
{
}

String RenderThemeBal::extraDefaultStyleSheet()
{
	return String("");
}

#if ENABLE(VIDEO)
String RenderThemeBal::extraMediaControlsStyleSheet()
{
	return String("");
}

String RenderThemeBal::formatMediaControlsRemainingTime(float, float duration) const
{
    // This is a workaround to make the appearance of media time controller in
    // in-page mode the same as in fullscreen mode.
    return formatMediaControlsTime(duration);
}
#endif

double RenderThemeBal::caretBlinkInterval() const
{
	return 1.0;
}

void RenderThemeBal::setButtonStyle(RenderStyle& style) const
{
    Length vertPadding(int(style.fontSize() / paddingDivisor), Fixed);
    style.setPaddingTop(vertPadding);
    style.setPaddingBottom(vertPadding);
}

void RenderThemeBal::adjustButtonStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    setButtonStyle(style);
    //style.setCursor(CURSOR_WEBKIT_GRAB);
}

void RenderThemeBal::adjustTextAreaStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    setButtonStyle(style);
}

bool RenderThemeBal::paintTextArea(const RenderObject& object, const PaintInfo& info, const FloatRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, IntRect(rect));
}

void RenderThemeBal::adjustTextFieldStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    setButtonStyle(style);
}

bool RenderThemeBal::paintTextFieldOrTextAreaOrSearchField(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    GraphicsContext* context = info.context;

    context->save();
    context->setStrokeStyle(SolidStroke);
    context->setStrokeThickness(lineWidth);
    if (!isEnabled(object))
        context->setStrokeColor(disabledOutline, ColorSpaceDeviceRGB);
    else if (isPressed(object))
        info.context->setStrokeGradient(createLinearGradient(depressedTopOutline, depressedBottomOutline, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    else if (isHovered(object) || isFocused(object))
        context->setStrokeGradient(createLinearGradient(hoverTopOutline, hoverBottomOutline, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    else
        context->setStrokeGradient(createLinearGradient(regularTopOutline, regularBottomOutline, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));

    Path textFieldRoundedRectangle = roundedRectForBorder(object, rect);
    if (object.style().appearance() == SearchFieldPart) {
        // We force the fill color to White so as to match the background color of the search cancel button graphic.
        context->setFillColor(Color::white, ColorSpaceDeviceRGB);
        context->fillPath(textFieldRoundedRectangle);
        context->strokePath(textFieldRoundedRectangle);
    } else
        context->strokePath(textFieldRoundedRectangle);
    context->restore();
    return false;
}

bool RenderThemeBal::paintTextField(const RenderObject& object, const PaintInfo& info, const FloatRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, IntRect(rect));
}

void RenderThemeBal::adjustSearchFieldStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    setButtonStyle(style);
}

void RenderThemeBal::adjustSearchFieldCancelButtonStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    static const float defaultControlFontPixelSize = 13;
    static const float defaultCancelButtonSize = 9;
    static const float minCancelButtonSize = 5;
    static const float maxCancelButtonSize = 21;

    // Scale the button size based on the font size
    float fontScale = style.fontSize() / defaultControlFontPixelSize;
    int cancelButtonSize = lroundf(std::min(std::max(minCancelButtonSize, defaultCancelButtonSize * fontScale), maxCancelButtonSize));
    Length length(cancelButtonSize, Fixed);
    style.setWidth(length);
    style.setHeight(length);
}

bool RenderThemeBal::paintSearchField(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextFieldOrTextAreaOrSearchField(object, info, rect);
}

bool RenderThemeBal::paintSearchFieldCancelButton(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
    ASSERT(object->parent());
    if (!object.parent() || !object.parent()->isBox())
        return false;

    RenderBox* parentRenderBox = downcast<RenderBox>(object.parent());

    IntRect parentBox = parentRenderBox->absoluteContentBox();
    IntRect bounds = rect;
    // Make sure the scaled button stays square and fits in its parent's box.
    bounds.setHeight(std::min(parentBox.width(), std::min(parentBox.height(), bounds.height())));
    bounds.setWidth(bounds.height());

    // Put the button in the middle vertically, and round up the value.
    // So if it has to be one pixel off-center, it would be one pixel closer
    // to the bottom of the field. This would look better with the text.
    bounds.setY(parentBox.y() + (parentBox.height() - bounds.height() + 1) / 2);

	static Image* cancelImage = Image::loadPlatformResource("SearchCancel").leakRef();
	static Image* cancelPressedImage = Image::loadPlatformResource("SearchCancelPressed").leakRef();
    paintInfo.context->drawImage(isPressed(object) ? cancelPressedImage : cancelImage, object.style().colorSpace(), bounds);
    return false;
}

void RenderThemeBal::adjustMenuListButtonStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    // These seem to be reasonable padding values from observation.
    const int paddingLeft = 8;
    const int paddingRight = 4;

    const int minHeight = style.fontSize() * 2;

    style.resetPadding();
    style.setHeight(Length(Auto));

    style.setPaddingRight(Length(minHeight + paddingRight, Fixed));
    style.setPaddingLeft(Length(paddingLeft, Fixed));
    //style.setCursor(CURSOR_WEBKIT_GRAB);
}

void RenderThemeBal::calculateButtonSize(RenderStyle& style) const
{
    int size = style.fontSize();
    Length length(size, Fixed);
    if (style.appearance() == CheckboxPart || style.appearance() == RadioPart) {
        style.setWidth(length);
        style.setHeight(length);
        return;
    }

    // If the width and height are both specified, then we have nothing to do.
    if (!style.width().isIntrinsicOrAuto() && !style.height().isAuto())
        return;

    if (style.width().isIntrinsicOrAuto())
        style.setWidth(length);

    if (style.height().isAuto())
        style.setHeight(length);
}

bool RenderThemeBal::paintCheckbox(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    return paintButton(object, info, rect);
}

void RenderThemeBal::setCheckboxSize(RenderStyle& style) const
{
    calculateButtonSize(style);
}

bool RenderThemeBal::paintRadio(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    return paintButton(object, info, rect);
}

void RenderThemeBal::setRadioSize(RenderStyle& style) const
{
    //calculateButtonSize(style);

    if (!style.width().isIntrinsicOrAuto() && !style.height().isAuto())
        return;

    // FIXME:  A hard-coded size of 13 is used.  This is wrong but necessary for now.  It matches Firefox.
    // At different DPI settings on Windows, querying the theme gives you a larger size that accounts for
    // the higher DPI.  Until our entire engine honors a DPI setting other than 96, we can't rely on the theme's
    // metrics.
    if (style.width().isIntrinsicOrAuto())
        style.setWidth(Length(13, Fixed));
    if (style.height().isAuto())
        style.setHeight(Length(13, Fixed));
}

// If this function returns false, WebCore assumes the button is fully decorated
bool RenderThemeBal::paintButton(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    ASSERT(info.context);
    info.context->save();

    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeThickness(lineWidth);

    Color check(blackPen);
    if (!isEnabled(object)) {
        info.context->setFillGradient(createLinearGradient(disabledTop, disabledBottom, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
        info.context->setStrokeColor(disabledOutline, ColorSpaceDeviceRGB);
    } else if (isPressed(object)) {
        info.context->setFillGradient(createLinearGradient(depressedTop, depressedBottom, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
        info.context->setStrokeGradient(createLinearGradient(depressedTopOutline, depressedBottomOutline, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    } else {
        info.context->setFillGradient(createLinearGradient(regularTop, regularBottom, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
        info.context->setStrokeGradient(createLinearGradient(regularTopOutline, regularBottomOutline, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    }

    ControlPart part = object.style().appearance();
    switch (part) {
    case CheckboxPart: {
        FloatSize smallCorner(smallRadius, smallRadius);
        Path path;
        path.addRoundedRect(rect, smallCorner);
        info.context->fillPath(path);
        info.context->strokePath(path);

        if (isChecked(object)) {
            Path checkPath;
            IntRect rect2 = rect;
            rect2.inflate(-1);
            checkPath.moveTo(FloatPoint(rect2.x() + rect2.width() * checkboxLeftX, rect2.y() + rect2.height() * checkboxLeftY));
            checkPath.addLineTo(FloatPoint(rect2.x() + rect2.width() * checkboxMiddleX, rect2.maxY() - rect2.height() * checkboxMiddleY));
            checkPath.addLineTo(FloatPoint(rect2.x() + rect2.width() * checkboxRightX, rect2.y() + rect2.height() * checkboxRightY));
            info.context->setLineCap(RoundCap);
            info.context->setStrokeColor(blackPen, ColorSpaceDeviceRGB);
            info.context->setStrokeThickness(rect2.width() / checkboxStrokeThickness);
            info.context->fillPath(checkPath);
            info.context->strokePath(checkPath);
        }
        break;
    }
	case RadioPart: {
        Path path;
        path.addEllipse(rect);
        info.context->fillPath(path);
        info.context->strokePath(path);

        if (isChecked(object)) {
            IntRect rect2 = rect;
            rect2.inflate(-3);
            info.context->setFillColor(check, ColorSpaceDeviceRGB);
            info.context->setStrokeColor(check, ColorSpaceDeviceRGB);
            info.context->drawEllipse(rect2);
        }
        break;
	}
    case ButtonPart:
    case PushButtonPart: {
        FloatSize largeCorner(largeRadius, largeRadius);
        Path path;
        path.addRoundedRect(rect, largeCorner);
        info.context->fillPath(path);
        info.context->strokePath(path);
        break;
    }
    case SquareButtonPart: {
        Path path;
        path.addRect(rect);
        info.context->fillPath(path);
        info.context->strokePath(path);
        break;
    }
    default:
        info.context->restore();
        return true;
    }

    info.context->restore();
    return false;
}

void RenderThemeBal::adjustMenuListStyle(StyleResolver& css, RenderStyle& style, Element* element) const
{
    adjustMenuListButtonStyle(css, style, element);
}

void RenderThemeBal::adjustCheckboxStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    setCheckboxSize(style);
    style.setBoxShadow(nullptr);
    Length margin(marginSize, Fixed);
    style.setMarginBottom(margin);
    style.setMarginRight(margin);
    //style.setCursor(CURSOR_WEBKIT_GRAB);
}

void RenderThemeBal::adjustRadioStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    setRadioSize(style);
    style.setBoxShadow(nullptr);
    Length margin(marginSize, Fixed);
    style.setMarginBottom(margin);
    style.setMarginRight(margin);
    //style.setCursor(CURSOR_WEBKIT_GRAB);
}

static void paintMenuListBackground(GraphicsContext* context, const Path& menuListPath, const Color& backgroundColor)
{
    ASSERT(context);
    context->save();
    context->setFillColor(backgroundColor, ColorSpaceDeviceRGB);
    context->fillPath(menuListPath);
    context->restore();
}

const float baseFontSize = 11.0f;
const float baseArrowHeight = 4.0f;
const float baseArrowWidth = 5.0f;
const float baseSpaceBetweenArrows = 2.0f;
const int arrowPaddingLeft = 6;
const int arrowPaddingRight = 6;
const int paddingBeforeSeparator = 4;
const int baseBorderRadius = 5;
const int styledPopupPaddingLeft = 8;
const int styledPopupPaddingTop = 1;
const int styledPopupPaddingBottom = 2;

static void drawArrowsAndSeparator(const RenderObject& o, const PaintInfo& paintInfo, IntRect& bounds)
{
    // Since we actually know the size of the control here, we restrict the font scale to make sure the arrows will fit vertically in the bounds
    float fontScale = std::min(o.style().fontSize() / baseFontSize, bounds.height() / (baseArrowHeight * 2 + baseSpaceBetweenArrows));
    float centerY = bounds.y() + bounds.height() / 2.0f;
    float arrowHeight = baseArrowHeight * fontScale;
    float arrowWidth = baseArrowWidth * fontScale;
    float leftEdge = bounds.maxX() - arrowPaddingRight * o.style().effectiveZoom() - arrowWidth;
    float spaceBetweenArrows = baseSpaceBetweenArrows * fontScale;

    if (bounds.width() < arrowWidth + arrowPaddingLeft * o.style().effectiveZoom())
        return;

    GraphicsContextStateSaver stateSaver(*paintInfo.context);

    paintInfo.context->setFillColor(o.style().visitedDependentColor(CSSPropertyColor), o.style().colorSpace());
    paintInfo.context->setStrokeStyle(NoStroke);

    FloatPoint arrow1[3];
    arrow1[0] = FloatPoint(leftEdge, centerY - spaceBetweenArrows / 2.0f);
    arrow1[1] = FloatPoint(leftEdge + arrowWidth, centerY - spaceBetweenArrows / 2.0f);
    arrow1[2] = FloatPoint(leftEdge + arrowWidth / 2.0f, centerY - spaceBetweenArrows / 2.0f - arrowHeight);

    // Draw the top arrow
    paintInfo.context->drawConvexPolygon(3, arrow1, true);

    FloatPoint arrow2[3];
    arrow2[0] = FloatPoint(leftEdge, centerY + spaceBetweenArrows / 2.0f);
    arrow2[1] = FloatPoint(leftEdge + arrowWidth, centerY + spaceBetweenArrows / 2.0f);
    arrow2[2] = FloatPoint(leftEdge + arrowWidth / 2.0f, centerY + spaceBetweenArrows / 2.0f + arrowHeight);

    // Draw the bottom arrow
    paintInfo.context->drawConvexPolygon(3, arrow2, true);

    Color leftSeparatorColor(0, 0, 0, 40);
    Color rightSeparatorColor(255, 255, 255, 40);

    int separatorSpace = 2; // Deliberately ignores zoom since it looks nicer if it stays thin.
    int leftEdgeOfSeparator = static_cast<int>(leftEdge - arrowPaddingLeft * o.style().effectiveZoom()); // FIXME: Round?

    // Draw the separator to the left of the arrows
    paintInfo.context->setStrokeThickness(1.0f); // Deliberately ignores zoom since it looks nicer if it stays thin.
    paintInfo.context->setStrokeStyle(SolidStroke);
    paintInfo.context->setStrokeColor(leftSeparatorColor, ColorSpaceDeviceRGB);
    paintInfo.context->drawLine(IntPoint(leftEdgeOfSeparator, bounds.y()),
                                IntPoint(leftEdgeOfSeparator, bounds.maxY()));

    paintInfo.context->setStrokeColor(rightSeparatorColor, ColorSpaceDeviceRGB);
    paintInfo.context->drawLine(IntPoint(leftEdgeOfSeparator + separatorSpace, bounds.y()),
                                IntPoint(leftEdgeOfSeparator + separatorSpace, bounds.maxY()));
}

bool RenderThemeBal::paintMenuList(const RenderObject& o, const PaintInfo& paintInfo, const FloatRect& r)
{
    IntRect bounds = IntRect(r.x() + o.style().borderLeftWidth(),
                             r.y() + o.style().borderTopWidth(),
                             r.width() - o.style().borderLeftWidth() - o.style().borderRightWidth(),
                             r.height() - o.style().borderTopWidth() - o.style().borderBottomWidth());
    // Draw the gradients to give the styled popup menu a button appearance
    Path menuListRoundedRectangle = roundedRectForBorder(o, r);
    paintMenuListBackground(paintInfo.context, menuListRoundedRectangle, Color::white);

    EBorderStyle v = INSET;
    o.style().setBorderTopStyle(v);
    o.style().setBorderLeftStyle(v);
    o.style().setBorderBottomStyle(v);
    o.style().setBorderRightStyle(v);
    int borderWidth = 1;
    o.style().setBorderTopWidth(borderWidth);
    o.style().setBorderLeftWidth(borderWidth);
    o.style().setBorderBottomWidth(borderWidth);
    o.style().setBorderRightWidth(borderWidth);

    drawArrowsAndSeparator(o, paintInfo, bounds);

    // Stroke an outline around the entire control.
    paintInfo.context->setStrokeStyle(SolidStroke);
    paintInfo.context->setStrokeThickness(lineWidth);
    if (!isEnabled(o))
        paintInfo.context->setStrokeColor(disabledOutline, ColorSpaceDeviceRGB);
    else if (isPressed(o))
        paintInfo.context->setStrokeGradient(createLinearGradient(depressedTopOutline, depressedBottomOutline, IntPoint(r.maxXMinYCorner()), IntPoint(r.maxXMaxYCorner())));
    else
        paintInfo.context->setStrokeGradient(createLinearGradient(regularTopOutline, regularBottomOutline, IntPoint(r.maxXMinYCorner()), IntPoint(r.maxXMaxYCorner())));

    paintInfo.context->strokePath(menuListRoundedRectangle);

    return false;
}

bool RenderThemeBal::paintMenuListButtonDecorations(const RenderObject& o, const PaintInfo& paintInfo, const FloatRect& r)
{
    // Note, this method is only called if the menu list explicitly specifies either a border or background color.
    // Otherwise, RenderThemeBal::paintMenuList is called. We need to fit the arrow button with the border box
    // of the menu-list so as to not occlude the custom border.

    // We compute menuListRoundedRectangle with respect to the dimensions of the entire menu-list control (i.e. rect) and
    // its border radius so that we clip the contour of the arrow button (when we paint it below) to match the contour of
    // the control.
    Path menuListRoundedRectangle = roundedRectForBorder(o, r);

    // 1. Paint the background of the entire control.
    Color fillColor = o.style().visitedDependentColor(CSSPropertyBackgroundColor);
    if (!fillColor.isValid())
        fillColor = Color::white;
    paintMenuListBackground(paintInfo.context, menuListRoundedRectangle, fillColor);

    // 2. Paint the background of the button and its arrow.
    IntRect bounds = IntRect(r.x() + o.style().borderLeftWidth(),
                         r.y() + o.style().borderTopWidth(),
                         r.width() - o.style().borderLeftWidth() - o.style().borderRightWidth(),
                         r.height() - o.style().borderTopWidth() - o.style().borderBottomWidth());

    drawArrowsAndSeparator(o, paintInfo, bounds);

    return false;
}

void RenderThemeBal::adjustSliderThumbSize(RenderStyle& style, Element* element) const
{
    float fullScreenMultiplier = 1;
    ControlPart part = style.appearance();

    if (part == MediaSliderThumbPart || part == MediaVolumeSliderThumbPart) {
        RenderSlider* slider = determineRenderSlider(element->renderer());
        if (slider)
            fullScreenMultiplier = determineFullScreenMultiplier(element /* slider->element() */);
    }

    if (part == SliderThumbHorizontalPart || part == SliderThumbVerticalPart) {
        style.setWidth(Length((part == SliderThumbVerticalPart ? sliderThumbHeight : sliderThumbWidth) * fullScreenMultiplier, Fixed));
        style.setHeight(Length((part == SliderThumbVerticalPart ? sliderThumbWidth : sliderThumbHeight) * fullScreenMultiplier, Fixed));
    } else if (part == MediaVolumeSliderThumbPart) {
        style.setWidth(Length(mediaVolumeSliderThumbWidth * fullScreenMultiplier, Fixed));
        style.setHeight(Length(mediaVolumeSliderThumbHeight * fullScreenMultiplier, Fixed));
    } else if (part == MediaSliderThumbPart) {
        style.setWidth(Length(mediaSliderThumbWidth * fullScreenMultiplier, Fixed));
        style.setHeight(Length(mediaSliderThumbHeight * fullScreenMultiplier, Fixed));
    }
}

bool RenderThemeBal::paintSliderTrack(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    const static int SliderTrackHeight = 5;
    IntRect rect2;
    if (object.style().appearance() == SliderHorizontalPart) {
        rect2.setHeight(SliderTrackHeight);
        rect2.setWidth(rect.width());
        rect2.setX(rect.x());
        rect2.setY(rect.y() + (rect.height() - SliderTrackHeight) / 2);
    } else {
        rect2.setHeight(rect.height());
        rect2.setWidth(SliderTrackHeight);
        rect2.setX(rect.x() + (rect.width() - SliderTrackHeight) / 2);
        rect2.setY(rect.y());
    }
    return paintSliderTrackRect(object, info, rect2);
}

bool RenderThemeBal::paintSliderTrackRect(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    return paintSliderTrackRect(object, info, rect, rangeSliderRegularTopOutline, rangeSliderRegularBottomOutline,
                rangeSliderRegularTop, rangeSliderRegularBottom);
}

bool RenderThemeBal::paintSliderTrackRect(const RenderObject& object, const PaintInfo& info, const IntRect& rect,
        RGBA32 strokeColorStart, RGBA32 strokeColorEnd, RGBA32 fillColorStart, RGBA32 fillColorEnd)
{
    UNUSED_PARAM(object);

    FloatSize smallCorner(smallRadius, smallRadius);

    info.context->save();
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeThickness(lineWidth);

    info.context->setStrokeGradient(createLinearGradient(strokeColorStart, strokeColorEnd, rect.maxXMinYCorner(), rect. maxXMaxYCorner()));
    info.context->setFillGradient(createLinearGradient(fillColorStart, fillColorEnd, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));

    Path path;
    path.addRoundedRect(rect, smallCorner);
    info.context->fillPath(path);

    info.context->restore();

#if ENABLE(DATALIST_ELEMENT)
    paintSliderTicks(object, info, rect);
#endif
    return false;
}

bool RenderThemeBal::paintSliderThumb(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    FloatSize largeCorner(largeRadius, largeRadius);

    info.context->save();
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeThickness(lineWidth);

    if (isPressed(object) || isHovered(object)) {
        info.context->setStrokeGradient(createLinearGradient(hoverTopOutline, hoverBottomOutline, rect.maxXMinYCorner(), rect. maxXMaxYCorner()));
        info.context->setFillGradient(createLinearGradient(hoverTop, hoverBottom, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    } else {
        info.context->setStrokeGradient(createLinearGradient(regularTopOutline, regularBottomOutline, rect.maxXMinYCorner(), rect. maxXMaxYCorner()));
        info.context->setFillGradient(createLinearGradient(regularTop, regularBottom, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    }

    Path path;
    path.addRoundedRect(rect, largeCorner);
    info.context->fillPath(path);

    bool isVertical = rect.width() > rect.height();
    IntPoint startPoint(rect.x() + (isVertical ? 5 : 2), rect.y() + (isVertical ? 2 : 5));
    IntPoint endPoint(rect.x() + (isVertical ? 20 : 2), rect.y() + (isVertical ? 2 : 20));
    const int lineOffset = 2;
    const int shadowOffset = 1;

    for (int i = 0; i < 3; i++) {
        if (isVertical) {
            startPoint.setY(startPoint.y() + lineOffset);
            endPoint.setY(endPoint.y() + lineOffset);
        } else {
            startPoint.setX(startPoint.x() + lineOffset);
            endPoint.setX(endPoint.x() + lineOffset);
        }
        if (isPressed(object) || isHovered(object))
            info.context->setStrokeColor(dragRollLight, ColorSpaceDeviceRGB);
        else
            info.context->setStrokeColor(dragRegularLight, ColorSpaceDeviceRGB);
        info.context->drawLine(startPoint, endPoint);

        if (isVertical) {
            startPoint.setY(startPoint.y() + shadowOffset);
            endPoint.setY(endPoint.y() + shadowOffset);
        } else {
            startPoint.setX(startPoint.x() + shadowOffset);
            endPoint.setX(endPoint.x() + shadowOffset);
        }
        if (isPressed(object) || isHovered(object))
            info.context->setStrokeColor(dragRollDark, ColorSpaceDeviceRGB);
        else
            info.context->setStrokeColor(dragRegularDark, ColorSpaceDeviceRGB);
        info.context->drawLine(startPoint, endPoint);
    }
    info.context->restore();
    return false;
}

#if ENABLE(DATALIST_ELEMENT)
IntSize RenderThemeBal::sliderTickSize() const
{
  return IntSize(1, 6);
}

int RenderThemeBal::sliderTickOffsetFromTrackCenter() const
{
  static const int sliderTickOffset = -12;

  return sliderTickOffset;
}

LayoutUnit RenderThemeBal::sliderTickSnappingThreshold() const
{
  // The same threshold value as the Chromium port.
  return 5;
}
#endif

bool RenderThemeBal::supportsDataListUI(const AtomicString& type) const
{
#if ENABLE(DATALIST_ELEMENT)
  // FIXME: We need to support other types.
  return type == InputTypeNames::range();
#else
  UNUSED_PARAM(type);
  return false;
#endif
}

void RenderThemeBal::adjustMediaControlStyle(StyleResolver&, RenderStyle& style, Element* element) const
{
#if ENABLE(VIDEO)
	float fullScreenMultiplier = determineFullScreenMultiplier(element);
    HTMLMediaElement* mediaElement = parentMediaElement(element);
    if (!mediaElement)
        return;

    // We use multiples of mediaControlsHeight to make all objects scale evenly
    Length zero(0, Fixed);
    Length controlsHeight(mediaControlsHeight * fullScreenMultiplier, Fixed);
    Length timeWidth(mediaControlsHeight * 3 / 2 * fullScreenMultiplier, Fixed);
    Length volumeHeight(mediaControlsHeight * 4 * fullScreenMultiplier, Fixed);
    Length padding(mediaControlsHeight / 8 * fullScreenMultiplier, Fixed);
    float fontSize = mediaControlsHeight / 2 * fullScreenMultiplier;

    switch (style.appearance()) {
    case MediaPlayButtonPart:
    case MediaEnterFullscreenButtonPart:
    case MediaExitFullscreenButtonPart:
    case MediaMuteButtonPart:
        style.setWidth(controlsHeight);
        style.setHeight(controlsHeight);
        style.setBottom(zero);
        break;
    case MediaCurrentTimePart:
    case MediaTimeRemainingPart:
        style.setWidth(timeWidth);
        style.setHeight(controlsHeight);
        style.setPaddingRight(padding);
        style.setFontSize(static_cast<int>(fontSize));
        style.setBottom(zero);
        break;
    case MediaVolumeSliderContainerPart:
        style.setWidth(controlsHeight);
        style.setHeight(volumeHeight);
        style.setBottom(controlsHeight);
        break;
    default:
        break;
    }

    if (!std::isfinite(mediaElement->duration())) {
        // Live streams have infinite duration with no timeline. Force the mute
        // and fullscreen buttons to the right. This is needed when webkit does
        // not render the timeline container because it has a webkit-box-flex
        // of 1 and normally allows those buttons to be on the right.
        switch (style.appearance()) {
        case MediaEnterFullscreenButtonPart:
        case MediaExitFullscreenButtonPart:
            style.setPosition(AbsolutePosition);
            style.setBottom(zero);
            style.setRight(controlsHeight);
            break;
        case MediaMuteButtonPart:
            style.setPosition(AbsolutePosition);
            style.setBottom(zero);
            style.setRight(zero);
            break;
        default:
            break;
        }
    }
#else
    UNUSED_PARAM(style);
    UNUSED_PARAM(element);
#endif
}

void RenderThemeBal::adjustSliderTrackStyle(StyleResolver&, RenderStyle& style, Element* element) const
{
    float fullScreenMultiplier = determineFullScreenMultiplier(element);

    // We use multiples of mediaControlsHeight to make all objects scale evenly
    Length controlsHeight(mediaControlsHeight * fullScreenMultiplier, Fixed);
    Length mediaSliderHeight(19, Fixed);
    Length volumeHeight(mediaControlsHeight * 4 * fullScreenMultiplier, Fixed);
    switch (style.appearance()) {
    case MediaSliderPart:
        style.setHeight(mediaSliderHeight);
        break;
    case MediaVolumeSliderPart:
        style.setWidth(controlsHeight);
        style.setHeight(volumeHeight);
        break;
    case MediaFullScreenVolumeSliderPart:
    default:
        break;
    }
}

static bool paintMediaButton(GraphicsContext* context, const IntRect& rect, Image* image)
{
    context->drawImage(image, ColorSpaceDeviceRGB, rect);
    return false;
}

bool RenderThemeBal::paintMediaPlayButton(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = parentMediaElement(object);

    if (!mediaElement)
        return false;

	static Image* mediaPlay = Image::loadPlatformResource("MediaTheme/Play").leakRef();
	static Image* mediaPause = Image::loadPlatformResource("MediaTheme/Pause").leakRef();
	static Image* mediaPlayHovered = Image::loadPlatformResource("MediaTheme/PlayHovered").leakRef();
	static Image* mediaPauseHovered = Image::loadPlatformResource("MediaTheme/PauseHovered").leakRef();
	static Image* mediaPlayPressed = Image::loadPlatformResource("MediaTheme/PlayPressed").leakRef();
	static Image* mediaPausePressed = Image::loadPlatformResource("MediaTheme/PausePressed").leakRef();

	if (isPressed(object))
		return paintMediaButton(paintInfo.context, rect, mediaElement->canPlay() ? mediaPlayPressed : mediaPausePressed);
	else if(isHovered(object))
		return paintMediaButton(paintInfo.context, rect, mediaElement->canPlay() ? mediaPlayHovered : mediaPauseHovered);
	else
		return paintMediaButton(paintInfo.context, rect, mediaElement->canPlay() ? mediaPlay : mediaPause);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBal::paintMediaMuteButton(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = parentMediaElement(object);

    if (!mediaElement)
        return false;

	static Image* mediaMute = Image::loadPlatformResource("MediaTheme/SoundMute").leakRef();
	static Image* mediaUnmute = Image::loadPlatformResource("MediaTheme/SoundUnmute").leakRef();
	static Image* mediaMuteHovered = Image::loadPlatformResource("MediaTheme/SoundMuteHovered").leakRef();
	static Image* mediaUnmuteHovered = Image::loadPlatformResource("MediaTheme/SoundUnmuteHovered").leakRef();
	static Image* mediaMutePressed = Image::loadPlatformResource("MediaTheme/SoundMutePressed").leakRef();
	static Image* mediaUnmutePressed = Image::loadPlatformResource("MediaTheme/SoundUnmutePressed").leakRef();

	if (isPressed(object))
		return paintMediaButton(paintInfo.context, rect, mediaElement->muted() || !mediaElement->volume() ? mediaUnmutePressed : mediaMutePressed);
	else if(isHovered(object))
		return paintMediaButton(paintInfo.context, rect, mediaElement->muted() || !mediaElement->volume() ? mediaUnmuteHovered : mediaMuteHovered);
	else
		return paintMediaButton(paintInfo.context, rect, mediaElement->muted() || !mediaElement->volume() ? mediaUnmute : mediaMute);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBal::paintMediaFullscreenButton(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = parentMediaElement(object);
    if (!mediaElement)
        return false;

	static Image* mediaEnterFullscreen = Image::loadPlatformResource("MediaTheme/FullScreen").leakRef();
	static Image* mediaEnterFullscreenHovered = Image::loadPlatformResource("MediaTheme/FullScreenHovered").leakRef();
	static Image* mediaEnterFullscreenPressed = Image::loadPlatformResource("MediaTheme/FullScreenPressed").leakRef();

	Image* buttonImage;
	if (isPressed(object))
		buttonImage = mediaEnterFullscreenPressed;
	else if(isHovered(object))
		buttonImage = mediaEnterFullscreenHovered;
	else
		buttonImage = mediaEnterFullscreen;
#if 0//ENABLE(FULLSCREEN_API)
    static Image* mediaExitFullscreen = Image::loadPlatformResource("MediaTheme/FullScreen").leakRef();

    if (mediaElement->document()->webkitIsFullScreen() && mediaElement->document()->webkitCurrentFullScreenElement() == mediaElement)
        buttonImage = mediaExitFullscreen;
#endif
	return paintMediaButton(paintInfo.context, rect, buttonImage);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

/*
bool RenderThemeBal::paintMediaSliderTrack(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = parentMediaElement(object);
    if (!mediaElement)
        return false;

    float fullScreenMultiplier = determineFullScreenMultiplier(mediaElement);
    float loaded = 0;
    // FIXME: replace loaded with commented out one when buffer bug is fixed (see comment in
    // MediaPlayerPrivateMMrenderer::percentLoaded).
	loaded = mediaElement->percentLoaded();
	//if (mediaElement->player() && mediaElement->player()->implementation())
	//	  loaded = static_cast<MediaPlayerPrivate *>(mediaElement->player()->implementation())->percentLoaded();
    float position = mediaElement->duration() > 0 ? (mediaElement->currentTime() / mediaElement->duration()) : 0;

    int x = ceil(rect.x() + 2 * fullScreenMultiplier - fullScreenMultiplier / 2);
	int y = ceil(rect.y() + 14 * fullScreenMultiplier + fullScreenMultiplier / 2);
    int w = ceil(rect.width() - 4 * fullScreenMultiplier + fullScreenMultiplier / 2);
	int h = ceil(2 * fullScreenMultiplier);
    IntRect rect2(x, y, w, h);

    int wPlayed = ceil(w * position);
    int wLoaded = ceil((w - mediaSliderThumbWidth * fullScreenMultiplier) * loaded + mediaSliderThumbWidth * fullScreenMultiplier);

    IntRect played(x, y, wPlayed, h);
    IntRect buffered(x, y, wLoaded, h);

    // This is to paint main slider bar.
    bool result = paintSliderTrackRect(object, paintInfo, rect2);

    if (loaded > 0 || position > 0) {
        // This is to paint buffered bar.
        paintSliderTrackRect(object, paintInfo, buffered, Color::darkGray, Color::darkGray, Color::darkGray, Color::darkGray);

        // This is to paint played part of bar (left of slider thumb) using selection color.
        paintSliderTrackRect(object, paintInfo, played, selection, selection, selection, selection);
    }
    return result;

#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBal::paintMediaSliderThumb(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    RenderSlider* slider = determineRenderSlider(object);
    if (!slider)
        return false;

    float fullScreenMultiplier = determineFullScreenMultiplier(toElement(slider->node()));

    paintInfo.context->save();
    Path mediaThumbRoundedRectangle;
    mediaThumbRoundedRectangle.addRoundedRect(rect, FloatSize(mediaSliderThumbRadius * fullScreenMultiplier, mediaSliderThumbRadius * fullScreenMultiplier));
    paintInfo.context->setStrokeStyle(SolidStroke);
    paintInfo.context->setStrokeThickness(0.5);
    paintInfo.context->setStrokeColor(Color::black, ColorSpaceDeviceRGB);

    if (isPressed(object) || isHovered(object) || slider->inDragMode()) {
        paintInfo.context->setFillGradient(createLinearGradient(selection, Color(selection).dark().rgb(),
                rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    } else {
        paintInfo.context->setFillGradient(createLinearGradient(Color::white, Color(Color::white).dark().rgb(),
                rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    }
    paintInfo.context->fillPath(mediaThumbRoundedRectangle);
    paintInfo.context->restore();

    return true;
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}
*/
#if ENABLE(VIDEO)
typedef WTF::HashMap<const char*, Image*> MediaControlImageMap;
static MediaControlImageMap* gMediaControlImageMap = 0;

static Image* platformResource(const char* name)
{
    if (!gMediaControlImageMap)
        gMediaControlImageMap = new MediaControlImageMap();
    if (Image* image = gMediaControlImageMap->get(name))
        return image;
    if (Image* image = Image::loadPlatformResource(name).leakRef()) {
        gMediaControlImageMap->set(name, image);
        return image;
    }
    ASSERT_NOT_REACHED();
    return 0;
}
#endif

#if ENABLE(VIDEO)
static Image* getMediaSliderThumb()
{
	static Image* mediaSliderThumb = platformResource("Mediatheme/SliderThumb");
    return mediaSliderThumb;
}
#endif

bool RenderThemeBal::paintMediaSliderTrack(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = parentMediaElement(object);
    if (!mediaElement)
        return false;

    RenderStyle& style = object.style();
    GraphicsContext* context = paintInfo.context;

    // Draw the border of the time bar.
    // FIXME: this should be a rounded rect but need to fix GraphicsContextSkia first.
    // https://bugs.webkit.org/show_bug.cgi?id=30143
    context->save();
    context->setShouldAntialias(true);
    context->setStrokeStyle(SolidStroke);
    context->setStrokeColor(style.visitedDependentColor(CSSPropertyBorderLeftColor), ColorSpaceDeviceRGB);
    context->setStrokeThickness(style.borderLeftWidth());
    context->setFillColor(style.visitedDependentColor(CSSPropertyBackgroundColor), ColorSpaceDeviceRGB);
    context->drawRect(rect);
    context->restore();

    // Draw the buffered ranges.
    // FIXME: Draw multiple ranges if there are multiple buffered ranges.
    IntRect bufferedRect = rect;
    bufferedRect.inflate(-style.borderLeftWidth());

    double bufferedWidth = 0.0;
    if (mediaElement->percentLoaded() > 0.0) {
        // Account for the width of the slider thumb.
        Image* mediaSliderThumb = getMediaSliderThumb();
        double thumbWidth = mediaSliderThumb->width() / 2.0 + 1.0;
        double rectWidth = bufferedRect.width() - thumbWidth;
        if (rectWidth < 0.0)
            rectWidth = 0.0;
        bufferedWidth = rectWidth * mediaElement->percentLoaded() + thumbWidth;
    }
    bufferedRect.setWidth(bufferedWidth);

    // Don't bother drawing an empty area.
    if (!bufferedRect.isEmpty()) {
        IntPoint sliderTopLeft = bufferedRect.location();
        IntPoint sliderTopRight = sliderTopLeft;
        sliderTopRight.move(0, bufferedRect.height());

        Color startColor = object.style().visitedDependentColor(CSSPropertyColor);

        context->save();
        context->setStrokeStyle(NoStroke);
        context->setFillGradient(createLinearGradient(startColor.dark().rgb(),
                Color(startColor.red() / 2, startColor.green() / 2, startColor.blue() / 2, startColor.alpha()).dark().rgb(),
                sliderTopLeft, sliderTopRight));
        context->fillRect(bufferedRect);
        context->restore();
    }

    return true;
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBal::paintMediaSliderThumb(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
    UNUSED_PARAM(object);

	static Image* mediaSliderThumb = Image::loadPlatformResource("MediaTheme/SliderThumb").leakRef();
	return paintMediaButton(paintInfo.context, rect, mediaSliderThumb);
}

bool RenderThemeBal::paintMediaVolumeSliderTrack(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
    float pad = rect.width() * 0.45;
    float x = rect.x() + pad;
    float y = rect.y() + pad;
    float width = rect.width() * 0.1;
    float height = rect.height() - (2.0 * pad);

    IntRect rect2(x, y, width, height);

    return paintSliderTrackRect(object, paintInfo, rect2);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

bool RenderThemeBal::paintMediaVolumeSliderThumb(const RenderObject& object, const PaintInfo& paintInfo, const IntRect& rect)
{
#if ENABLE(VIDEO)
	static Image* mediaVolumeThumb = Image::loadPlatformResource("MediaTheme/VolumeSliderThumb").leakRef();
    UNUSED_PARAM(object);

    return paintMediaButton(paintInfo.context, rect, mediaVolumeThumb);
#else
    UNUSED_PARAM(object);
    UNUSED_PARAM(paintInfo);
    UNUSED_PARAM(rect);
    return false;
#endif
}

Color RenderThemeBal::platformFocusRingColor() const
{
    return focusRingPen;
}

#if ENABLE(TOUCH_EVENTS)
Color RenderThemeBal::platformTapHighlightColor() const
{
    return Color(0, 168, 223, 50);
}
#endif

Color RenderThemeBal::platformActiveSelectionBackgroundColor() const
{
    Color c(0xFF618ECE);
    return c;
}

Color RenderThemeBal::platformInactiveSelectionBackgroundColor() const
{
    Color c(0xFFCFCFCF);
    return c;
}

Color RenderThemeBal::platformActiveSelectionForegroundColor() const
{
    Color c(0xFF000000);
    return c;
}

Color RenderThemeBal::platformInactiveSelectionForegroundColor() const
{
    Color c(0xFF3F3F3F);
    return c;
}

Color RenderThemeBal::platformActiveListBoxSelectionBackgroundColor() const
{
    Color c(0xFF618ECE);
    return c;
}

Color RenderThemeBal::platformInactiveListBoxSelectionBackgroundColor() const
{
    Color c(0xFFCFCFCF);
    return c;
}

Color RenderThemeBal::platformActiveListBoxSelectionForegroundColor() const
{
    Color c(0xFF000000);
    return c;
}

Color RenderThemeBal::platformInactiveListBoxSelectionForegroundColor() const
{
    Color c(0xFF3F3F3F);
    return c;
}

Color RenderThemeBal::platformActiveTextSearchHighlightColor() const
{
    return Color(255, 150, 50); // Orange.
}

Color RenderThemeBal::platformInactiveTextSearchHighlightColor() const
{
    return Color(255, 255, 0); // Yellow.
}

void RenderThemeBal::updateCachedSystemFontDescription(CSSValueID, FontDescription& fontDescription) const
{
    // It was called by RenderEmbeddedObject::paintReplaced to render alternative string.
    // To avoid cairo_error while rendering, fontDescription should be passed.
    fontDescription.setOneFamily("Sans");
    fontDescription.setSpecifiedSize(defaultFontSize);
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setWeight(FontWeightNormal);
    fontDescription.setItalic(FontItalicOff);
}


#if ENABLE(PROGRESS_ELEMENT)
void RenderThemeBal::adjustProgressBarStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}

double RenderThemeBal::animationRepeatIntervalForProgressBar(RenderProgress* renderProgress) const
{
    return renderProgress->isDeterminate() ? 0.0 : 0.1;
}

double RenderThemeBal::animationDurationForProgressBar(RenderProgress* renderProgress) const
{
    return renderProgress->isDeterminate() ? 0.0 : 2.0;
}

bool RenderThemeBal::paintProgressBar(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
#if 1// ENABLE(PROGRESS_ELEMENT)
	return true;
#else
    if (!object->isProgress())
        return true;

	RenderProgress* renderProgress = toRenderProgress(object);

    FloatSize smallCorner(smallRadius, smallRadius);

    info.context->save();
    info.context->setStrokeStyle(SolidStroke);
    info.context->setStrokeThickness(lineWidth);

    info.context->setStrokeGradient(createLinearGradient(rangeSliderRegularTopOutline, rangeSliderRegularBottomOutline, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));
    info.context->setFillGradient(createLinearGradient(rangeSliderRegularTop, rangeSliderRegularBottom, rect.maxXMinYCorner(), rect.maxXMaxYCorner()));

    Path path;
    path.addRoundedRect(rect, smallCorner);
    info.context->fillPath(path);

    IntRect rect2 = rect;
    rect2.setX(rect2.x() + 1);
    rect2.setHeight(rect2.height() - 2);
    rect2.setY(rect2.y() + 1);
    info.context->setStrokeStyle(NoStroke);
    info.context->setStrokeThickness(0);
    if (renderProgress->isDeterminate()) {
        rect2.setWidth(rect2.width() * renderProgress->position() - 2);
        info.context->setFillGradient(createLinearGradient(progressRegularTop, progressRegularBottom, rect2.maxXMinYCorner(), rect2.maxXMaxYCorner()));
	}
	else
	{
        // Animating
        rect2.setWidth(rect2.width() - 2);
        RefPtr<Gradient> gradient = Gradient::create(rect2.minXMaxYCorner(), rect2.maxXMaxYCorner());
        gradient->addColorStop(0.0, Color(progressRegularBottom));
        gradient->addColorStop(renderProgress->animationProgress(), Color(progressRegularTop));
        gradient->addColorStop(1.0, Color(progressRegularBottom));
        info.context->setFillGradient(gradient);
    }
    Path path2;
    path2.addRoundedRect(rect2, smallCorner);
    info.context->fillPath(path2);

    info.context->restore();
    return false;
#endif
}
#endif

} // namespace WebCore
