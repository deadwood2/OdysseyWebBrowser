/*
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2013, 2014 Adobe Systems Incorporated. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CanvasRenderingContext2D.h"

#include "BitmapImage.h"
#include "CSSFontSelector.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "CachedImage.h"
#include "CanvasGradient.h"
#include "CanvasPattern.h"
#include "DOMMatrix.h"
#include "DOMMatrixInit.h"
#include "DOMPath.h"
#include "DisplayListRecorder.h"
#include "DisplayListReplayer.h"
#include "FloatQuad.h"
#include "HTMLImageElement.h"
#include "HTMLVideoElement.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "RenderElement.h"
#include "RenderImage.h"
#include "RenderLayer.h"
#include "RenderTheme.h"
#include "SecurityOrigin.h"
#include "StrokeStyleApplier.h"
#include "StyleProperties.h"
#include "StyleResolver.h"
#include "TextMetrics.h"
#include "TextRun.h"
#include <wtf/CheckedArithmetic.h>
#include <wtf/MathExtras.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/TextStream.h>

#if USE(CG) && !PLATFORM(IOS)
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace WebCore {

using namespace HTMLNames;

#if USE(CG)
const CanvasRenderingContext2D::ImageSmoothingQuality defaultSmoothingQuality = CanvasRenderingContext2D::ImageSmoothingQuality::Low;
#else
const CanvasRenderingContext2D::ImageSmoothingQuality defaultSmoothingQuality = CanvasRenderingContext2D::ImageSmoothingQuality::Medium;
#endif

static const int defaultFontSize = 10;
static const char* const defaultFontFamily = "sans-serif";
static const char* const defaultFont = "10px sans-serif";

struct DisplayListDrawingContext {
    WTF_MAKE_FAST_ALLOCATED;
public:
    GraphicsContext context;
    DisplayList::Recorder recorder;
    DisplayList::DisplayList displayList;
    
    DisplayListDrawingContext(const FloatRect& clip)
        : recorder(context, displayList, clip, AffineTransform())
    {
    }
};

typedef HashMap<const CanvasRenderingContext2D*, std::unique_ptr<DisplayList::DisplayList>> ContextDisplayListHashMap;

static ContextDisplayListHashMap& contextDisplayListMap()
{
    static NeverDestroyed<ContextDisplayListHashMap> sharedHashMap;
    return sharedHashMap;
}

class CanvasStrokeStyleApplier : public StrokeStyleApplier {
public:
    CanvasStrokeStyleApplier(CanvasRenderingContext2D* canvasContext)
        : m_canvasContext(canvasContext)
    {
    }

    void strokeStyle(GraphicsContext* c) override
    {
        c->setStrokeThickness(m_canvasContext->lineWidth());
        c->setLineCap(m_canvasContext->getLineCap());
        c->setLineJoin(m_canvasContext->getLineJoin());
        c->setMiterLimit(m_canvasContext->miterLimit());
        const Vector<float>& lineDash = m_canvasContext->getLineDash();
        DashArray convertedLineDash(lineDash.size());
        for (size_t i = 0; i < lineDash.size(); ++i)
            convertedLineDash[i] = static_cast<DashArrayElement>(lineDash[i]);
        c->setLineDash(convertedLineDash, m_canvasContext->lineDashOffset());
    }

private:
    CanvasRenderingContext2D* m_canvasContext;
};

CanvasRenderingContext2D::CanvasRenderingContext2D(HTMLCanvasElement& canvas, bool usesCSSCompatibilityParseMode, bool usesDashboardCompatibilityMode)
    : CanvasRenderingContext(canvas)
    , m_stateStack(1)
    , m_usesCSSCompatibilityParseMode(usesCSSCompatibilityParseMode)
#if ENABLE(DASHBOARD_SUPPORT)
    , m_usesDashboardCompatibilityMode(usesDashboardCompatibilityMode)
#endif
{
#if !ENABLE(DASHBOARD_SUPPORT)
    ASSERT_UNUSED(usesDashboardCompatibilityMode, !usesDashboardCompatibilityMode);
#endif
}

void CanvasRenderingContext2D::unwindStateStack()
{
    // Ensure that the state stack in the ImageBuffer's context
    // is cleared before destruction, to avoid assertions in the
    // GraphicsContext dtor.
    if (size_t stackSize = m_stateStack.size()) {
        if (GraphicsContext* context = canvas().existingDrawingContext()) {
            while (--stackSize)
                context->restore();
        }
    }
}

CanvasRenderingContext2D::~CanvasRenderingContext2D()
{
#if !ASSERT_DISABLED
    unwindStateStack();
#endif

    if (UNLIKELY(tracksDisplayListReplay()))
        contextDisplayListMap().remove(this);
}

bool CanvasRenderingContext2D::isAccelerated() const
{
#if USE(IOSURFACE_CANVAS_BACKING_STORE) || ENABLE(ACCELERATED_2D_CANVAS)
    if (!canvas().hasCreatedImageBuffer())
        return false;
    auto* context = drawingContext();
    return context && context->isAcceleratedContext();
#else
    return false;
#endif
}

void CanvasRenderingContext2D::reset()
{
    unwindStateStack();
    m_stateStack.resize(1);
    m_stateStack.first() = State();
    m_path.clear();
    m_unrealizedSaveCount = 0;
    
    m_recordingContext = nullptr;
}

CanvasRenderingContext2D::State::State()
    : strokeStyle(Color::black)
    , fillStyle(Color::black)
    , lineWidth(1)
    , lineCap(ButtCap)
    , lineJoin(MiterJoin)
    , miterLimit(10)
    , shadowBlur(0)
    , shadowColor(Color::transparent)
    , globalAlpha(1)
    , globalComposite(CompositeSourceOver)
    , globalBlend(BlendModeNormal)
    , hasInvertibleTransform(true)
    , lineDashOffset(0)
    , imageSmoothingEnabled(true)
    , imageSmoothingQuality(defaultSmoothingQuality)
    , textAlign(StartTextAlign)
    , textBaseline(AlphabeticTextBaseline)
    , direction(Direction::Inherit)
    , unparsedFont(defaultFont)
{
}

CanvasRenderingContext2D::State::State(const State& other)
    : unparsedStrokeColor(other.unparsedStrokeColor)
    , unparsedFillColor(other.unparsedFillColor)
    , strokeStyle(other.strokeStyle)
    , fillStyle(other.fillStyle)
    , lineWidth(other.lineWidth)
    , lineCap(other.lineCap)
    , lineJoin(other.lineJoin)
    , miterLimit(other.miterLimit)
    , shadowOffset(other.shadowOffset)
    , shadowBlur(other.shadowBlur)
    , shadowColor(other.shadowColor)
    , globalAlpha(other.globalAlpha)
    , globalComposite(other.globalComposite)
    , globalBlend(other.globalBlend)
    , transform(other.transform)
    , hasInvertibleTransform(other.hasInvertibleTransform)
    , lineDashOffset(other.lineDashOffset)
    , imageSmoothingEnabled(other.imageSmoothingEnabled)
    , imageSmoothingQuality(other.imageSmoothingQuality)
    , textAlign(other.textAlign)
    , textBaseline(other.textBaseline)
    , direction(other.direction)
    , unparsedFont(other.unparsedFont)
    , font(other.font)
{
}

CanvasRenderingContext2D::State& CanvasRenderingContext2D::State::operator=(const State& other)
{
    if (this == &other)
        return *this;

    unparsedStrokeColor = other.unparsedStrokeColor;
    unparsedFillColor = other.unparsedFillColor;
    strokeStyle = other.strokeStyle;
    fillStyle = other.fillStyle;
    lineWidth = other.lineWidth;
    lineCap = other.lineCap;
    lineJoin = other.lineJoin;
    miterLimit = other.miterLimit;
    shadowOffset = other.shadowOffset;
    shadowBlur = other.shadowBlur;
    shadowColor = other.shadowColor;
    globalAlpha = other.globalAlpha;
    globalComposite = other.globalComposite;
    globalBlend = other.globalBlend;
    transform = other.transform;
    hasInvertibleTransform = other.hasInvertibleTransform;
    imageSmoothingEnabled = other.imageSmoothingEnabled;
    imageSmoothingQuality = other.imageSmoothingQuality;
    textAlign = other.textAlign;
    textBaseline = other.textBaseline;
    direction = other.direction;
    unparsedFont = other.unparsedFont;
    font = other.font;

    return *this;
}

CanvasRenderingContext2D::FontProxy::~FontProxy()
{
    if (realized())
        m_font.fontSelector()->unregisterForInvalidationCallbacks(*this);
}

CanvasRenderingContext2D::FontProxy::FontProxy(const FontProxy& other)
    : m_font(other.m_font)
{
    if (realized())
        m_font.fontSelector()->registerForInvalidationCallbacks(*this);
}

auto CanvasRenderingContext2D::FontProxy::operator=(const FontProxy& other) -> FontProxy&
{
    if (realized())
        m_font.fontSelector()->unregisterForInvalidationCallbacks(*this);

    m_font = other.m_font;

    if (realized())
        m_font.fontSelector()->registerForInvalidationCallbacks(*this);

    return *this;
}

inline void CanvasRenderingContext2D::FontProxy::update(FontSelector& selector)
{
    ASSERT(&selector == m_font.fontSelector()); // This is an invariant. We should only ever be registered for callbacks on m_font.m_fonts.m_fontSelector.
    if (realized())
        m_font.fontSelector()->unregisterForInvalidationCallbacks(*this);
    m_font.update(&selector);
    if (realized())
        m_font.fontSelector()->registerForInvalidationCallbacks(*this);
    ASSERT(&selector == m_font.fontSelector());
}

void CanvasRenderingContext2D::FontProxy::fontsNeedUpdate(FontSelector& selector)
{
    ASSERT_ARG(selector, &selector == m_font.fontSelector());
    ASSERT(realized());

    update(selector);
}

inline void CanvasRenderingContext2D::FontProxy::initialize(FontSelector& fontSelector, const RenderStyle& newStyle)
{
    // Beware! m_font.fontSelector() might not point to document.fontSelector()!
    ASSERT(newStyle.fontCascade().fontSelector() == &fontSelector);
    if (realized())
        m_font.fontSelector()->unregisterForInvalidationCallbacks(*this);
    m_font = newStyle.fontCascade();
    m_font.update(&fontSelector);
    ASSERT(&fontSelector == m_font.fontSelector());
    m_font.fontSelector()->registerForInvalidationCallbacks(*this);
}

inline const FontMetrics& CanvasRenderingContext2D::FontProxy::fontMetrics() const
{
    return m_font.fontMetrics();
}

inline const FontCascadeDescription& CanvasRenderingContext2D::FontProxy::fontDescription() const
{
    return m_font.fontDescription();
}

inline float CanvasRenderingContext2D::FontProxy::width(const TextRun& textRun, GlyphOverflow* overflow) const
{
    return m_font.width(textRun, 0, overflow);
}

inline void CanvasRenderingContext2D::FontProxy::drawBidiText(GraphicsContext& context, const TextRun& run, const FloatPoint& point, FontCascade::CustomFontNotReadyAction action) const
{
    context.drawBidiText(m_font, run, point, action);
}

void CanvasRenderingContext2D::realizeSaves()
{
    if (m_unrealizedSaveCount)
        realizeSavesLoop();

    if (m_unrealizedSaveCount) {
        static NeverDestroyed<String> consoleMessage(MAKE_STATIC_STRING_IMPL("CanvasRenderingContext2D.save() has been called without a matching restore() too many times. Ignoring save()."));
        canvas().document().addConsoleMessage(MessageSource::Rendering, MessageLevel::Error, consoleMessage);
    }
}

void CanvasRenderingContext2D::realizeSavesLoop()
{
    ASSERT(m_unrealizedSaveCount);
    ASSERT(m_stateStack.size() >= 1);
    GraphicsContext* context = drawingContext();
    do {
        if (m_stateStack.size() > MaxSaveCount)
            break;
        m_stateStack.append(state());
        if (context)
            context->save();
    } while (--m_unrealizedSaveCount);
}

void CanvasRenderingContext2D::restore()
{
    if (m_unrealizedSaveCount) {
        --m_unrealizedSaveCount;
        return;
    }
    ASSERT(m_stateStack.size() >= 1);
    if (m_stateStack.size() <= 1)
        return;
    m_path.transform(state().transform);
    m_stateStack.removeLast();
    if (std::optional<AffineTransform> inverse = state().transform.inverse())
        m_path.transform(inverse.value());
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    c->restore();
}

void CanvasRenderingContext2D::setStrokeStyle(CanvasStyle style)
{
    if (!style.isValid())
        return;

    if (state().strokeStyle.isValid() && state().strokeStyle.isEquivalentColor(style))
        return;

    if (style.isCurrentColor()) {
        if (style.hasOverrideAlpha()) {
            // FIXME: Should not use RGBA32 here.
            style = CanvasStyle(colorWithOverrideAlpha(currentColor(&canvas()).rgb(), style.overrideAlpha()));
        } else
            style = CanvasStyle(currentColor(&canvas()));
    } else
        checkOrigin(style.canvasPattern());

    realizeSaves();
    State& state = modifiableState();
    state.strokeStyle = style;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    state.strokeStyle.applyStrokeColor(*c);
    state.unparsedStrokeColor = String();
}

void CanvasRenderingContext2D::setFillStyle(CanvasStyle style)
{
    if (!style.isValid())
        return;

    if (state().fillStyle.isValid() && state().fillStyle.isEquivalentColor(style))
        return;

    if (style.isCurrentColor()) {
        if (style.hasOverrideAlpha()) {
            // FIXME: Should not use RGBA32 here.
            style = CanvasStyle(colorWithOverrideAlpha(currentColor(&canvas()).rgb(), style.overrideAlpha()));
        } else
            style = CanvasStyle(currentColor(&canvas()));
    } else
        checkOrigin(style.canvasPattern());

    realizeSaves();
    State& state = modifiableState();
    state.fillStyle = style;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    state.fillStyle.applyFillColor(*c);
    state.unparsedFillColor = String();
}

float CanvasRenderingContext2D::lineWidth() const
{
    return state().lineWidth;
}

void CanvasRenderingContext2D::setLineWidth(float width)
{
    if (!(std::isfinite(width) && width > 0))
        return;
    if (state().lineWidth == width)
        return;
    realizeSaves();
    modifiableState().lineWidth = width;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    c->setStrokeThickness(width);
}

String CanvasRenderingContext2D::lineCap() const
{
    return lineCapName(state().lineCap);
}

void CanvasRenderingContext2D::setLineCap(const String& s)
{
    LineCap cap;
    if (!parseLineCap(s, cap))
        return;
    if (state().lineCap == cap)
        return;
    realizeSaves();
    modifiableState().lineCap = cap;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    c->setLineCap(cap);
}

String CanvasRenderingContext2D::lineJoin() const
{
    return lineJoinName(state().lineJoin);
}

void CanvasRenderingContext2D::setLineJoin(const String& s)
{
    LineJoin join;
    if (!parseLineJoin(s, join))
        return;
    if (state().lineJoin == join)
        return;
    realizeSaves();
    modifiableState().lineJoin = join;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    c->setLineJoin(join);
}

float CanvasRenderingContext2D::miterLimit() const
{
    return state().miterLimit;
}

void CanvasRenderingContext2D::setMiterLimit(float limit)
{
    if (!(std::isfinite(limit) && limit > 0))
        return;
    if (state().miterLimit == limit)
        return;
    realizeSaves();
    modifiableState().miterLimit = limit;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    c->setMiterLimit(limit);
}

float CanvasRenderingContext2D::shadowOffsetX() const
{
    return state().shadowOffset.width();
}

void CanvasRenderingContext2D::setShadowOffsetX(float x)
{
    if (!std::isfinite(x))
        return;
    if (state().shadowOffset.width() == x)
        return;
    realizeSaves();
    modifiableState().shadowOffset.setWidth(x);
    applyShadow();
}

float CanvasRenderingContext2D::shadowOffsetY() const
{
    return state().shadowOffset.height();
}

void CanvasRenderingContext2D::setShadowOffsetY(float y)
{
    if (!std::isfinite(y))
        return;
    if (state().shadowOffset.height() == y)
        return;
    realizeSaves();
    modifiableState().shadowOffset.setHeight(y);
    applyShadow();
}

float CanvasRenderingContext2D::shadowBlur() const
{
    return state().shadowBlur;
}

void CanvasRenderingContext2D::setShadowBlur(float blur)
{
    if (!(std::isfinite(blur) && blur >= 0))
        return;
    if (state().shadowBlur == blur)
        return;
    realizeSaves();
    modifiableState().shadowBlur = blur;
    applyShadow();
}

String CanvasRenderingContext2D::shadowColor() const
{
    return Color(state().shadowColor).serialized();
}

void CanvasRenderingContext2D::setShadowColor(const String& colorString)
{
    Color color = parseColorOrCurrentColor(colorString, &canvas());
    if (!color.isValid())
        return;
    if (state().shadowColor == color)
        return;
    realizeSaves();
    modifiableState().shadowColor = color;
    applyShadow();
}

const Vector<float>& CanvasRenderingContext2D::getLineDash() const
{
    return state().lineDash;
}

static bool lineDashSequenceIsValid(const Vector<float>& dash)
{
    for (size_t i = 0; i < dash.size(); i++) {
        if (!std::isfinite(dash[i]) || dash[i] < 0)
            return false;
    }
    return true;
}

void CanvasRenderingContext2D::setLineDash(const Vector<float>& dash)
{
    if (!lineDashSequenceIsValid(dash))
        return;

    realizeSaves();
    modifiableState().lineDash = dash;
    // Spec requires the concatenation of two copies the dash list when the
    // number of elements is odd
    if (dash.size() % 2)
        modifiableState().lineDash.appendVector(dash);

    applyLineDash();
}

void CanvasRenderingContext2D::setWebkitLineDash(const Vector<float>& dash)
{
    if (!lineDashSequenceIsValid(dash))
        return;

    realizeSaves();
    modifiableState().lineDash = dash;

    applyLineDash();
}

float CanvasRenderingContext2D::lineDashOffset() const
{
    return state().lineDashOffset;
}

void CanvasRenderingContext2D::setLineDashOffset(float offset)
{
    if (!std::isfinite(offset) || state().lineDashOffset == offset)
        return;

    realizeSaves();
    modifiableState().lineDashOffset = offset;
    applyLineDash();
}

void CanvasRenderingContext2D::applyLineDash() const
{
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    DashArray convertedLineDash(state().lineDash.size());
    for (size_t i = 0; i < state().lineDash.size(); ++i)
        convertedLineDash[i] = static_cast<DashArrayElement>(state().lineDash[i]);
    c->setLineDash(convertedLineDash, state().lineDashOffset);
}

float CanvasRenderingContext2D::globalAlpha() const
{
    return state().globalAlpha;
}

void CanvasRenderingContext2D::setGlobalAlpha(float alpha)
{
    if (!(alpha >= 0 && alpha <= 1))
        return;
    if (state().globalAlpha == alpha)
        return;
    realizeSaves();
    modifiableState().globalAlpha = alpha;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    c->setAlpha(alpha);
}

String CanvasRenderingContext2D::globalCompositeOperation() const
{
    return compositeOperatorName(state().globalComposite, state().globalBlend);
}

void CanvasRenderingContext2D::setGlobalCompositeOperation(const String& operation)
{
    CompositeOperator op = CompositeSourceOver;
    BlendMode blendMode = BlendModeNormal;
    if (!parseCompositeAndBlendOperator(operation, op, blendMode))
        return;
    if ((state().globalComposite == op) && (state().globalBlend == blendMode))
        return;
    realizeSaves();
    modifiableState().globalComposite = op;
    modifiableState().globalBlend = blendMode;
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    c->setCompositeOperation(op, blendMode);
}

void CanvasRenderingContext2D::scale(float sx, float sy)
{
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    if (!std::isfinite(sx) || !std::isfinite(sy))
        return;

    AffineTransform newTransform = state().transform;
    newTransform.scaleNonUniform(sx, sy);
    if (state().transform == newTransform)
        return;

    realizeSaves();

    if (!sx || !sy) {
        modifiableState().hasInvertibleTransform = false;
        return;
    }

    modifiableState().transform = newTransform;
    c->scale(FloatSize(sx, sy));
    m_path.transform(AffineTransform().scaleNonUniform(1.0 / sx, 1.0 / sy));
}

void CanvasRenderingContext2D::rotate(float angleInRadians)
{
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    if (!std::isfinite(angleInRadians))
        return;

    AffineTransform newTransform = state().transform;
    newTransform.rotate(angleInRadians / piDouble * 180.0);
    if (state().transform == newTransform)
        return;

    realizeSaves();

    modifiableState().transform = newTransform;
    c->rotate(angleInRadians);
    m_path.transform(AffineTransform().rotate(-angleInRadians / piDouble * 180.0));
}

void CanvasRenderingContext2D::translate(float tx, float ty)
{
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    if (!std::isfinite(tx) | !std::isfinite(ty))
        return;

    AffineTransform newTransform = state().transform;
    newTransform.translate(tx, ty);
    if (state().transform == newTransform)
        return;

    realizeSaves();

    modifiableState().transform = newTransform;
    c->translate(tx, ty);
    m_path.transform(AffineTransform().translate(-tx, -ty));
}

void CanvasRenderingContext2D::transform(float m11, float m12, float m21, float m22, float dx, float dy)
{
    GraphicsContext* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    if (!std::isfinite(m11) | !std::isfinite(m21) | !std::isfinite(dx) | !std::isfinite(m12) | !std::isfinite(m22) | !std::isfinite(dy))
        return;

    AffineTransform transform(m11, m12, m21, m22, dx, dy);
    AffineTransform newTransform = state().transform * transform;
    if (state().transform == newTransform)
        return;

    realizeSaves();

    if (auto inverse = transform.inverse()) {
        modifiableState().transform = newTransform;
        c->concatCTM(transform);
        m_path.transform(inverse.value());
        return;
    }
    modifiableState().hasInvertibleTransform = false;
}

Ref<DOMMatrix> CanvasRenderingContext2D::getTransform() const
{
    return DOMMatrix::create(state().transform.toTransformationMatrix(), DOMMatrixReadOnly::Is2D::Yes);
}

void CanvasRenderingContext2D::setTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
    GraphicsContext* c = drawingContext();
    if (!c)
        return;

    if (!std::isfinite(m11) | !std::isfinite(m21) | !std::isfinite(dx) | !std::isfinite(m12) | !std::isfinite(m22) | !std::isfinite(dy))
        return;

    resetTransform();
    transform(m11, m12, m21, m22, dx, dy);
}

ExceptionOr<void> CanvasRenderingContext2D::setTransform(DOMMatrixInit&& matrixInit)
{
    auto checkValid = DOMMatrixReadOnly::validateAndFixup(matrixInit);
    if (checkValid.hasException())
        return checkValid.releaseException();

    setTransform(matrixInit.a.value_or(1), matrixInit.b.value_or(0), matrixInit.c.value_or(0), matrixInit.d.value_or(1), matrixInit.e.value_or(0), matrixInit.f.value_or(0));

    return { };
}

void CanvasRenderingContext2D::resetTransform()
{
    GraphicsContext* c = drawingContext();
    if (!c)
        return;

    AffineTransform ctm = state().transform;
    bool hasInvertibleTransform = state().hasInvertibleTransform;

    realizeSaves();

    c->setCTM(canvas().baseTransform());
    modifiableState().transform = AffineTransform();

    if (hasInvertibleTransform)
        m_path.transform(ctm);

    modifiableState().hasInvertibleTransform = true;
}

void CanvasRenderingContext2D::setStrokeColor(const String& color, std::optional<float> alpha)
{
    if (alpha) {
        setStrokeStyle(CanvasStyle::createFromStringWithOverrideAlpha(color, alpha.value()));
        return;
    }

    if (color == state().unparsedStrokeColor)
        return;

    realizeSaves();
    setStrokeStyle(CanvasStyle::createFromString(color));
    modifiableState().unparsedStrokeColor = color;
}

void CanvasRenderingContext2D::setStrokeColor(float grayLevel, float alpha)
{
    if (state().strokeStyle.isValid() && state().strokeStyle.isEquivalentRGBA(grayLevel, grayLevel, grayLevel, alpha))
        return;
    setStrokeStyle(CanvasStyle(grayLevel, alpha));
}

void CanvasRenderingContext2D::setStrokeColor(float r, float g, float b, float a)
{
    if (state().strokeStyle.isValid() && state().strokeStyle.isEquivalentRGBA(r, g, b, a))
        return;
    setStrokeStyle(CanvasStyle(r, g, b, a));
}

void CanvasRenderingContext2D::setStrokeColor(float c, float m, float y, float k, float a)
{
    if (state().strokeStyle.isValid() && state().strokeStyle.isEquivalentCMYKA(c, m, y, k, a))
        return;
    setStrokeStyle(CanvasStyle(c, m, y, k, a));
}

void CanvasRenderingContext2D::setFillColor(const String& color, std::optional<float> alpha)
{
    if (alpha) {
        setFillStyle(CanvasStyle::createFromStringWithOverrideAlpha(color, alpha.value()));
        return;
    }

    if (color == state().unparsedFillColor)
        return;

    realizeSaves();
    setFillStyle(CanvasStyle::createFromString(color));
    modifiableState().unparsedFillColor = color;
}

void CanvasRenderingContext2D::setFillColor(float grayLevel, float alpha)
{
    if (state().fillStyle.isValid() && state().fillStyle.isEquivalentRGBA(grayLevel, grayLevel, grayLevel, alpha))
        return;
    setFillStyle(CanvasStyle(grayLevel, alpha));
}

void CanvasRenderingContext2D::setFillColor(float r, float g, float b, float a)
{
    if (state().fillStyle.isValid() && state().fillStyle.isEquivalentRGBA(r, g, b, a))
        return;
    setFillStyle(CanvasStyle(r, g, b, a));
}

void CanvasRenderingContext2D::setFillColor(float c, float m, float y, float k, float a)
{
    if (state().fillStyle.isValid() && state().fillStyle.isEquivalentCMYKA(c, m, y, k, a))
        return;
    setFillStyle(CanvasStyle(c, m, y, k, a));
}

void CanvasRenderingContext2D::beginPath()
{
    m_path.clear();
}

static bool validateRectForCanvas(float& x, float& y, float& width, float& height)
{
    if (!std::isfinite(x) | !std::isfinite(y) | !std::isfinite(width) | !std::isfinite(height))
        return false;

    if (!width && !height)
        return false;

    if (width < 0) {
        width = -width;
        x -= width;
    }

    if (height < 0) {
        height = -height;
        y -= height;
    }

    return true;
}

inline void CanvasRenderingContext2D::clearPathForDashboardBackwardCompatibilityMode()
{
#if ENABLE(DASHBOARD_SUPPORT)
    if (m_usesDashboardCompatibilityMode)
        m_path.clear();
#endif
}

static bool isFullCanvasCompositeMode(CompositeOperator op)
{
    // See 4.8.11.1.3 Compositing
    // CompositeSourceAtop and CompositeDestinationOut are not listed here as the platforms already
    // implement the specification's behavior.
    return op == CompositeSourceIn || op == CompositeSourceOut || op == CompositeDestinationIn || op == CompositeDestinationAtop;
}

static WindRule toWindRule(CanvasRenderingContext2D::WindingRule rule)
{
    return rule == CanvasRenderingContext2D::WindingRule::Nonzero ? RULE_NONZERO : RULE_EVENODD;
}

String CanvasRenderingContext2D::stringForWindingRule(WindingRule windingRule)
{
    switch (windingRule) {
    case WindingRule::Nonzero:
        return ASCIILiteral("nonzero");
    case WindingRule::Evenodd:
        return ASCIILiteral("evenodd");
    }

    ASSERT_NOT_REACHED();
    return String();
}

void CanvasRenderingContext2D::fill(WindingRule windingRule)
{
    fillInternal(m_path, windingRule);
    clearPathForDashboardBackwardCompatibilityMode();
}

void CanvasRenderingContext2D::stroke()
{
    strokeInternal(m_path);
    clearPathForDashboardBackwardCompatibilityMode();
}

void CanvasRenderingContext2D::clip(WindingRule windingRule)
{
    clipInternal(m_path, windingRule);
    clearPathForDashboardBackwardCompatibilityMode();
}

void CanvasRenderingContext2D::fill(DOMPath& path, WindingRule windingRule)
{
    fillInternal(path.path(), windingRule);
}

void CanvasRenderingContext2D::stroke(DOMPath& path)
{
    strokeInternal(path.path());
}

void CanvasRenderingContext2D::clip(DOMPath& path, WindingRule windingRule)
{
    clipInternal(path.path(), windingRule);
}

void CanvasRenderingContext2D::fillInternal(const Path& path, WindingRule windingRule)
{
    auto* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    // If gradient size is zero, then paint nothing.
    auto* gradient = c->fillGradient();
    if (gradient && gradient->isZeroSize())
        return;

    if (!path.isEmpty()) {
        auto savedFillRule = c->fillRule();
        c->setFillRule(toWindRule(windingRule));

        if (isFullCanvasCompositeMode(state().globalComposite)) {
            beginCompositeLayer();
            c->fillPath(path);
            endCompositeLayer();
            didDrawEntireCanvas();
        } else if (state().globalComposite == CompositeCopy) {
            clearCanvas();
            c->fillPath(path);
            didDrawEntireCanvas();
        } else {
            c->fillPath(path);
            didDraw(path.fastBoundingRect());
        }
        
        c->setFillRule(savedFillRule);
    }
}

void CanvasRenderingContext2D::strokeInternal(const Path& path)
{
    auto* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    // If gradient size is zero, then paint nothing.
    auto* gradient = c->strokeGradient();
    if (gradient && gradient->isZeroSize())
        return;

    if (!path.isEmpty()) {
        if (isFullCanvasCompositeMode(state().globalComposite)) {
            beginCompositeLayer();
            c->strokePath(path);
            endCompositeLayer();
            didDrawEntireCanvas();
        } else if (state().globalComposite == CompositeCopy) {
            clearCanvas();
            c->strokePath(path);
            didDrawEntireCanvas();
        } else {
            FloatRect dirtyRect = path.fastBoundingRect();
            inflateStrokeRect(dirtyRect);
            c->strokePath(path);
            didDraw(dirtyRect);
        }
    }
}

void CanvasRenderingContext2D::clipInternal(const Path& path, WindingRule windingRule)
{
    auto* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    realizeSaves();
    c->canvasClip(path, toWindRule(windingRule));
}

inline void CanvasRenderingContext2D::beginCompositeLayer()
{
#if !USE(CAIRO)
    drawingContext()->beginTransparencyLayer(1);
#endif
}

inline void CanvasRenderingContext2D::endCompositeLayer()
{
#if !USE(CAIRO)
    drawingContext()->endTransparencyLayer();    
#endif
}

bool CanvasRenderingContext2D::isPointInPath(float x, float y, WindingRule windingRule)
{
    return isPointInPathInternal(m_path, x, y, windingRule);
}

bool CanvasRenderingContext2D::isPointInStroke(float x, float y)
{
    return isPointInStrokeInternal(m_path, x, y);
}

bool CanvasRenderingContext2D::isPointInPath(DOMPath& path, float x, float y, WindingRule windingRule)
{
    return isPointInPathInternal(path.path(), x, y, windingRule);
}

bool CanvasRenderingContext2D::isPointInStroke(DOMPath& path, float x, float y)
{
    return isPointInStrokeInternal(path.path(), x, y);
}

bool CanvasRenderingContext2D::isPointInPathInternal(const Path& path, float x, float y, WindingRule windingRule)
{
    auto* c = drawingContext();
    if (!c)
        return false;
    if (!state().hasInvertibleTransform)
        return false;

    auto transformedPoint = state().transform.inverse().value_or(AffineTransform()).mapPoint(FloatPoint(x, y));

    if (!std::isfinite(transformedPoint.x()) || !std::isfinite(transformedPoint.y()))
        return false;

    return path.contains(transformedPoint, toWindRule(windingRule));
}

bool CanvasRenderingContext2D::isPointInStrokeInternal(const Path& path, float x, float y)
{
    auto* c = drawingContext();
    if (!c)
        return false;
    if (!state().hasInvertibleTransform)
        return false;

    auto transformedPoint = state().transform.inverse().value_or(AffineTransform()).mapPoint(FloatPoint(x, y));
    if (!std::isfinite(transformedPoint.x()) || !std::isfinite(transformedPoint.y()))
        return false;

    CanvasStrokeStyleApplier applier(this);
    return path.strokeContains(&applier, transformedPoint);
}

void CanvasRenderingContext2D::clearRect(float x, float y, float width, float height)
{
    if (!validateRectForCanvas(x, y, width, height))
        return;
    auto* context = drawingContext();
    if (!context)
        return;
    if (!state().hasInvertibleTransform)
        return;
    FloatRect rect(x, y, width, height);

    bool saved = false;
    if (shouldDrawShadows()) {
        context->save();
        saved = true;
        context->setLegacyShadow(FloatSize(), 0, Color::transparent);
    }
    if (state().globalAlpha != 1) {
        if (!saved) {
            context->save();
            saved = true;
        }
        context->setAlpha(1);
    }
    if (state().globalComposite != CompositeSourceOver) {
        if (!saved) {
            context->save();
            saved = true;
        }
        context->setCompositeOperation(CompositeSourceOver);
    }
    context->clearRect(rect);
    if (saved)
        context->restore();
    didDraw(rect);
}

void CanvasRenderingContext2D::fillRect(float x, float y, float width, float height)
{
    if (!validateRectForCanvas(x, y, width, height))
        return;

    auto* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

    // from the HTML5 Canvas spec:
    // If x0 = x1 and y0 = y1, then the linear gradient must paint nothing
    // If x0 = x1 and y0 = y1 and r0 = r1, then the radial gradient must paint nothing
    auto* gradient = c->fillGradient();
    if (gradient && gradient->isZeroSize())
        return;

    FloatRect rect(x, y, width, height);

    if (rectContainsCanvas(rect)) {
        c->fillRect(rect);
        didDrawEntireCanvas();
    } else if (isFullCanvasCompositeMode(state().globalComposite)) {
        beginCompositeLayer();
        c->fillRect(rect);
        endCompositeLayer();
        didDrawEntireCanvas();
    } else if (state().globalComposite == CompositeCopy) {
        clearCanvas();
        c->fillRect(rect);
        didDrawEntireCanvas();
    } else {
        c->fillRect(rect);
        didDraw(rect);
    }
}

void CanvasRenderingContext2D::strokeRect(float x, float y, float width, float height)
{
    if (!validateRectForCanvas(x, y, width, height))
        return;

    auto* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;
    if (!(state().lineWidth >= 0))
        return;

    // If gradient size is zero, then paint nothing.
    auto* gradient = c->strokeGradient();
    if (gradient && gradient->isZeroSize())
        return;

    FloatRect rect(x, y, width, height);
    if (isFullCanvasCompositeMode(state().globalComposite)) {
        beginCompositeLayer();
        c->strokeRect(rect, state().lineWidth);
        endCompositeLayer();
        didDrawEntireCanvas();
    } else if (state().globalComposite == CompositeCopy) {
        clearCanvas();
        c->strokeRect(rect, state().lineWidth);
        didDrawEntireCanvas();
    } else {
        FloatRect boundingRect = rect;
        boundingRect.inflate(state().lineWidth / 2);
        c->strokeRect(rect, state().lineWidth);
        didDraw(boundingRect);
    }
}

void CanvasRenderingContext2D::setShadow(float width, float height, float blur, const String& colorString, std::optional<float> alpha)
{
    Color color = Color::transparent;
    if (!colorString.isNull()) {
        color = parseColorOrCurrentColor(colorString, &canvas());
        if (!color.isValid())
            return;
    }
    // FIXME: Should not use RGBA32 here.
    setShadow(FloatSize(width, height), blur, colorWithOverrideAlpha(color.rgb(), alpha));
}

void CanvasRenderingContext2D::setShadow(float width, float height, float blur, float grayLevel, float alpha)
{
    setShadow(FloatSize(width, height), blur, Color(grayLevel, grayLevel, grayLevel, alpha));
}

void CanvasRenderingContext2D::setShadow(float width, float height, float blur, float r, float g, float b, float a)
{
    setShadow(FloatSize(width, height), blur, Color(r, g, b, a));
}

void CanvasRenderingContext2D::setShadow(float width, float height, float blur, float c, float m, float y, float k, float a)
{
    setShadow(FloatSize(width, height), blur, Color(c, m, y, k, a));
}

void CanvasRenderingContext2D::clearShadow()
{
    setShadow(FloatSize(), 0, Color::transparent);
}

void CanvasRenderingContext2D::setShadow(const FloatSize& offset, float blur, const Color& color)
{
    if (state().shadowOffset == offset && state().shadowBlur == blur && state().shadowColor == color)
        return;
    bool wasDrawingShadows = shouldDrawShadows();
    realizeSaves();
    modifiableState().shadowOffset = offset;
    modifiableState().shadowBlur = blur;
    modifiableState().shadowColor = color;
    if (!wasDrawingShadows && !shouldDrawShadows())
        return;
    applyShadow();
}

void CanvasRenderingContext2D::applyShadow()
{
    auto* c = drawingContext();
    if (!c)
        return;

    if (shouldDrawShadows()) {
        float width = state().shadowOffset.width();
        float height = state().shadowOffset.height();
        c->setLegacyShadow(FloatSize(width, -height), state().shadowBlur, state().shadowColor);
    } else
        c->setLegacyShadow(FloatSize(), 0, Color::transparent);
}

bool CanvasRenderingContext2D::shouldDrawShadows() const
{
    return state().shadowColor.isVisible() && (state().shadowBlur || !state().shadowOffset.isZero());
}

enum class ImageSizeType { AfterDevicePixelRatio, BeforeDevicePixelRatio };
static LayoutSize size(HTMLImageElement& element, ImageSizeType sizeType = ImageSizeType::BeforeDevicePixelRatio)
{
    LayoutSize size;
    if (auto* cachedImage = element.cachedImage()) {
        size = cachedImage->imageSizeForRenderer(element.renderer(), 1.0f); // FIXME: Not sure about this.
        if (sizeType == ImageSizeType::AfterDevicePixelRatio && is<RenderImage>(element.renderer()) && cachedImage->image() && !cachedImage->image()->hasRelativeWidth())
            size.scale(downcast<RenderImage>(*element.renderer()).imageDevicePixelRatio());
    }
    return size;
}

static inline FloatSize size(HTMLCanvasElement& canvasElement)
{
    return canvasElement.size();
}

#if ENABLE(VIDEO)

static inline FloatSize size(HTMLVideoElement& video)
{
    auto* player = video.player();
    if (!player)
        return { };
    return player->naturalSize();
}

#endif

static inline FloatRect normalizeRect(const FloatRect& rect)
{
    return FloatRect(std::min(rect.x(), rect.maxX()),
        std::min(rect.y(), rect.maxY()),
        std::max(rect.width(), -rect.width()),
        std::max(rect.height(), -rect.height()));
}

ExceptionOr<void> CanvasRenderingContext2D::drawImage(CanvasImageSource&& image, float dx, float dy)
{
    return WTF::switchOn(image,
        [&] (RefPtr<HTMLImageElement>& imageElement) -> ExceptionOr<void> {
            LayoutSize destRectSize = size(*imageElement, ImageSizeType::AfterDevicePixelRatio);
            LayoutSize sourceRectSize = size(*imageElement, ImageSizeType::BeforeDevicePixelRatio);
            return this->drawImage(*imageElement, FloatRect { 0, 0, sourceRectSize.width(), sourceRectSize.height() }, FloatRect { dx, dy, destRectSize.width(), destRectSize.height() });
        },
        [&] (auto& element) -> ExceptionOr<void> {
            FloatSize elementSize = size(*element);
            return this->drawImage(*element, FloatRect { 0, 0, elementSize.width(), elementSize.height() }, FloatRect { dx, dy, elementSize.width(), elementSize.height() });
        }
    );
}

ExceptionOr<void> CanvasRenderingContext2D::drawImage(CanvasImageSource&& image, float dx, float dy, float dw, float dh)
{
    return WTF::switchOn(image,
        [&] (auto& element) -> ExceptionOr<void> {
            FloatSize elementSize = size(*element);
            return this->drawImage(*element, FloatRect { 0, 0, elementSize.width(), elementSize.height() }, FloatRect { dx, dy, dw, dh });
        }
    );
}

ExceptionOr<void> CanvasRenderingContext2D::drawImage(CanvasImageSource&& image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
{
    return WTF::switchOn(image,
        [&] (auto& element) -> ExceptionOr<void> {
            return this->drawImage(*element, FloatRect { sx, sy, sw, sh }, FloatRect { dx, dy, dw, dh });
        }
    );
}

ExceptionOr<void> CanvasRenderingContext2D::drawImage(HTMLImageElement& imageElement, const FloatRect& srcRect, const FloatRect& dstRect)
{
    return drawImage(imageElement, srcRect, dstRect, state().globalComposite, state().globalBlend);
}

ExceptionOr<void> CanvasRenderingContext2D::drawImage(HTMLImageElement& imageElement, const FloatRect& srcRect, const FloatRect& dstRect, const CompositeOperator& op, const BlendMode& blendMode)
{
    if (!std::isfinite(dstRect.x()) || !std::isfinite(dstRect.y()) || !std::isfinite(dstRect.width()) || !std::isfinite(dstRect.height())
        || !std::isfinite(srcRect.x()) || !std::isfinite(srcRect.y()) || !std::isfinite(srcRect.width()) || !std::isfinite(srcRect.height()))
        return { };

    if (!dstRect.width() || !dstRect.height())
        return { };

    if (!imageElement.complete())
        return { };

    FloatRect normalizedSrcRect = normalizeRect(srcRect);
    FloatRect normalizedDstRect = normalizeRect(dstRect);

    FloatRect imageRect = FloatRect(FloatPoint(), size(imageElement, ImageSizeType::BeforeDevicePixelRatio));
    if (!srcRect.width() || !srcRect.height())
        return Exception { IndexSizeError };

    // When the source rectangle is outside the source image, the source rectangle must be clipped
    // to the source image and the destination rectangle must be clipped in the same proportion.
    FloatRect originalNormalizedSrcRect = normalizedSrcRect;
    normalizedSrcRect.intersect(imageRect);
    if (normalizedSrcRect.isEmpty())
        return { };

    if (normalizedSrcRect != originalNormalizedSrcRect) {
        normalizedDstRect.setWidth(normalizedDstRect.width() * normalizedSrcRect.width() / originalNormalizedSrcRect.width());
        normalizedDstRect.setHeight(normalizedDstRect.height() * normalizedSrcRect.height() / originalNormalizedSrcRect.height());
        if (normalizedDstRect.isEmpty())
            return { };
    }

    GraphicsContext* c = drawingContext();
    if (!c)
        return { };
    if (!state().hasInvertibleTransform)
        return { };

    CachedImage* cachedImage = imageElement.cachedImage();
    if (!cachedImage)
        return { };

    Image* image = cachedImage->imageForRenderer(imageElement.renderer());
    if (!image)
        return { };

    ImageObserver* observer = image->imageObserver();

    if (image->isSVGImage()) {
        image->setImageObserver(nullptr);
        image->setContainerSize(imageRect.size());
    }

    if (image->isBitmapImage())
        downcast<BitmapImage>(*image).updateFromSettings(imageElement.document().settings());

    if (rectContainsCanvas(normalizedDstRect)) {
        c->drawImage(*image, normalizedDstRect, normalizedSrcRect, ImagePaintingOptions(op, blendMode));
        didDrawEntireCanvas();
    } else if (isFullCanvasCompositeMode(op)) {
        fullCanvasCompositedDrawImage(*image, normalizedDstRect, normalizedSrcRect, op);
        didDrawEntireCanvas();
    } else if (op == CompositeCopy) {
        clearCanvas();
        c->drawImage(*image, normalizedDstRect, normalizedSrcRect, ImagePaintingOptions(op, blendMode));
        didDrawEntireCanvas();
    } else {
        c->drawImage(*image, normalizedDstRect, normalizedSrcRect, ImagePaintingOptions(op, blendMode));
        didDraw(normalizedDstRect);
    }
    
    if (image->isSVGImage())
        image->setImageObserver(observer);

    checkOrigin(&imageElement);

    return { };
}

ExceptionOr<void> CanvasRenderingContext2D::drawImage(HTMLCanvasElement& sourceCanvas, const FloatRect& srcRect, const FloatRect& dstRect)
{
    FloatRect srcCanvasRect = FloatRect(FloatPoint(), sourceCanvas.size());

    if (!srcCanvasRect.width() || !srcCanvasRect.height())
        return Exception { InvalidStateError };

    if (!srcRect.width() || !srcRect.height())
        return Exception { IndexSizeError };

    if (!srcCanvasRect.contains(normalizeRect(srcRect)) || !dstRect.width() || !dstRect.height())
        return { };

    GraphicsContext* c = drawingContext();
    if (!c)
        return { };
    if (!state().hasInvertibleTransform)
        return { };

    // FIXME: Do this through platform-independent GraphicsContext API.
    ImageBuffer* buffer = sourceCanvas.buffer();
    if (!buffer)
        return { };

    checkOrigin(&sourceCanvas);

#if ENABLE(ACCELERATED_2D_CANVAS)
    // If we're drawing from one accelerated canvas 2d to another, avoid calling sourceCanvas.makeRenderingResultsAvailable()
    // as that will do a readback to software.
    CanvasRenderingContext* sourceContext = sourceCanvas.renderingContext();
    // FIXME: Implement an accelerated path for drawing from a WebGL canvas to a 2d canvas when possible.
    if (!isAccelerated() || !sourceContext || !sourceContext->isAccelerated() || !sourceContext->is2d())
        sourceCanvas.makeRenderingResultsAvailable();
#else
    sourceCanvas.makeRenderingResultsAvailable();
#endif

    if (rectContainsCanvas(dstRect)) {
        c->drawImageBuffer(*buffer, dstRect, srcRect, ImagePaintingOptions(state().globalComposite, state().globalBlend));
        didDrawEntireCanvas();
    } else if (isFullCanvasCompositeMode(state().globalComposite)) {
        fullCanvasCompositedDrawImage(*buffer, dstRect, srcRect, state().globalComposite);
        didDrawEntireCanvas();
    } else if (state().globalComposite == CompositeCopy) {
        clearCanvas();
        c->drawImageBuffer(*buffer, dstRect, srcRect, ImagePaintingOptions(state().globalComposite, state().globalBlend));
        didDrawEntireCanvas();
    } else {
        c->drawImageBuffer(*buffer, dstRect, srcRect, ImagePaintingOptions(state().globalComposite, state().globalBlend));
        didDraw(dstRect);
    }

    return { };
}

#if ENABLE(VIDEO)

ExceptionOr<void> CanvasRenderingContext2D::drawImage(HTMLVideoElement& video, const FloatRect& srcRect, const FloatRect& dstRect)
{
    if (video.readyState() == HTMLMediaElement::HAVE_NOTHING || video.readyState() == HTMLMediaElement::HAVE_METADATA)
        return { };

    FloatRect videoRect = FloatRect(FloatPoint(), size(video));
    if (!srcRect.width() || !srcRect.height())
        return Exception { IndexSizeError };

    if (!videoRect.contains(normalizeRect(srcRect)) || !dstRect.width() || !dstRect.height())
        return { };

    GraphicsContext* c = drawingContext();
    if (!c)
        return { };
    if (!state().hasInvertibleTransform)
        return { };

    checkOrigin(&video);

#if USE(CG) || (ENABLE(ACCELERATED_2D_CANVAS) && USE(GSTREAMER_GL) && USE(CAIRO))
    if (NativeImagePtr image = video.nativeImageForCurrentTime()) {
        c->drawNativeImage(image, FloatSize(video.videoWidth(), video.videoHeight()), dstRect, srcRect);
        if (rectContainsCanvas(dstRect))
            didDrawEntireCanvas();
        else
            didDraw(dstRect);

        return { };
    }
#endif

    GraphicsContextStateSaver stateSaver(*c);
    c->clip(dstRect);
    c->translate(dstRect.x(), dstRect.y());
    c->scale(FloatSize(dstRect.width() / srcRect.width(), dstRect.height() / srcRect.height()));
    c->translate(-srcRect.x(), -srcRect.y());
    video.paintCurrentFrameInContext(*c, FloatRect(FloatPoint(), size(video)));
    stateSaver.restore();
    didDraw(dstRect);

    return { };
}

#endif

void CanvasRenderingContext2D::drawImageFromRect(HTMLImageElement& imageElement, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, const String& compositeOperation)
{
    CompositeOperator op;
    auto blendOp = BlendModeNormal;
    if (!parseCompositeAndBlendOperator(compositeOperation, op, blendOp) || blendOp != BlendModeNormal)
        op = CompositeSourceOver;
    drawImage(imageElement, FloatRect { sx, sy, sw, sh }, FloatRect { dx, dy, dw, dh }, op, BlendModeNormal);
}

void CanvasRenderingContext2D::setAlpha(float alpha)
{
    setGlobalAlpha(alpha);
}

void CanvasRenderingContext2D::setCompositeOperation(const String& operation)
{
    setGlobalCompositeOperation(operation);
}

void CanvasRenderingContext2D::clearCanvas()
{
    auto* c = drawingContext();
    if (!c)
        return;

    c->save();
    c->setCTM(canvas().baseTransform());
    c->clearRect(FloatRect(0, 0, canvas().width(), canvas().height()));
    c->restore();
}

Path CanvasRenderingContext2D::transformAreaToDevice(const Path& path) const
{
    Path transformed(path);
    transformed.transform(state().transform);
    transformed.transform(canvas().baseTransform());
    return transformed;
}

Path CanvasRenderingContext2D::transformAreaToDevice(const FloatRect& rect) const
{
    Path path;
    path.addRect(rect);
    return transformAreaToDevice(path);
}

bool CanvasRenderingContext2D::rectContainsCanvas(const FloatRect& rect) const
{
    FloatQuad quad(rect);
    FloatQuad canvasQuad(FloatRect(0, 0, canvas().width(), canvas().height()));
    return state().transform.mapQuad(quad).containsQuad(canvasQuad);
}

template<class T> IntRect CanvasRenderingContext2D::calculateCompositingBufferRect(const T& area, IntSize* croppedOffset)
{
    IntRect canvasRect(0, 0, canvas().width(), canvas().height());
    canvasRect = canvas().baseTransform().mapRect(canvasRect);
    Path path = transformAreaToDevice(area);
    IntRect bufferRect = enclosingIntRect(path.fastBoundingRect());
    IntPoint originalLocation = bufferRect.location();
    bufferRect.intersect(canvasRect);
    if (croppedOffset)
        *croppedOffset = originalLocation - bufferRect.location();
    return bufferRect;
}

std::unique_ptr<ImageBuffer> CanvasRenderingContext2D::createCompositingBuffer(const IntRect& bufferRect)
{
    return ImageBuffer::create(bufferRect.size(), isAccelerated() ? Accelerated : Unaccelerated);
}

void CanvasRenderingContext2D::compositeBuffer(ImageBuffer& buffer, const IntRect& bufferRect, CompositeOperator op)
{
    IntRect canvasRect(0, 0, canvas().width(), canvas().height());
    canvasRect = canvas().baseTransform().mapRect(canvasRect);

    auto* c = drawingContext();
    if (!c)
        return;

    c->save();
    c->setCTM(AffineTransform());
    c->setCompositeOperation(op);

    c->save();
    c->clipOut(bufferRect);
    c->clearRect(canvasRect);
    c->restore();
    c->drawImageBuffer(buffer, bufferRect.location(), state().globalComposite);
    c->restore();
}

static void drawImageToContext(Image& image, GraphicsContext& context, const FloatRect& dest, const FloatRect& src, CompositeOperator op)
{
    context.drawImage(image, dest, src, op);
}

static void drawImageToContext(ImageBuffer& imageBuffer, GraphicsContext& context, const FloatRect& dest, const FloatRect& src, CompositeOperator op)
{
    context.drawImageBuffer(imageBuffer, dest, src, op);
}

template<class T> void CanvasRenderingContext2D::fullCanvasCompositedDrawImage(T& image, const FloatRect& dest, const FloatRect& src, CompositeOperator op)
{
    ASSERT(isFullCanvasCompositeMode(op));

    IntSize croppedOffset;
    auto bufferRect = calculateCompositingBufferRect(dest, &croppedOffset);
    if (bufferRect.isEmpty()) {
        clearCanvas();
        return;
    }

    auto buffer = createCompositingBuffer(bufferRect);
    if (!buffer)
        return;

    auto* c = drawingContext();
    if (!c)
        return;

    FloatRect adjustedDest = dest;
    adjustedDest.setLocation(FloatPoint(0, 0));
    AffineTransform effectiveTransform = c->getCTM();
    IntRect transformedAdjustedRect = enclosingIntRect(effectiveTransform.mapRect(adjustedDest));
    buffer->context().translate(-transformedAdjustedRect.location().x(), -transformedAdjustedRect.location().y());
    buffer->context().translate(croppedOffset.width(), croppedOffset.height());
    buffer->context().concatCTM(effectiveTransform);
    drawImageToContext(image, buffer->context(), adjustedDest, src, CompositeSourceOver);

    compositeBuffer(*buffer, bufferRect, op);
}

void CanvasRenderingContext2D::prepareGradientForDashboard(CanvasGradient& gradient) const
{
#if ENABLE(DASHBOARD_SUPPORT)
    if (m_usesDashboardCompatibilityMode)
        gradient.setDashboardCompatibilityMode();
#else
    UNUSED_PARAM(gradient);
#endif
}

static CanvasRenderingContext2D::Style toStyle(const CanvasStyle& style)
{
    if (auto* gradient = style.canvasGradient())
        return RefPtr<CanvasGradient> { gradient };
    if (auto* pattern = style.canvasPattern())
        return RefPtr<CanvasPattern> { pattern };
    return style.color();
}

CanvasRenderingContext2D::Style CanvasRenderingContext2D::strokeStyle() const
{
    return toStyle(state().strokeStyle);
}

void CanvasRenderingContext2D::setStrokeStyle(CanvasRenderingContext2D::Style&& style)
{
    WTF::switchOn(style,
        [this] (const String& string) { this->setStrokeColor(string); },
        [this] (const RefPtr<CanvasGradient>& gradient) { this->setStrokeStyle(CanvasStyle(*gradient)); },
        [this] (const RefPtr<CanvasPattern>& pattern) { this->setStrokeStyle(CanvasStyle(*pattern)); }
    );
}

CanvasRenderingContext2D::Style CanvasRenderingContext2D::fillStyle() const
{
    return toStyle(state().fillStyle);
}

void CanvasRenderingContext2D::setFillStyle(CanvasRenderingContext2D::Style&& style)
{
    WTF::switchOn(style,
        [this] (const String& string) { this->setFillColor(string); },
        [this] (const RefPtr<CanvasGradient>& gradient) { this->setFillStyle(CanvasStyle(*gradient)); },
        [this] (const RefPtr<CanvasPattern>& pattern) { this->setFillStyle(CanvasStyle(*pattern)); }
    );
}

ExceptionOr<Ref<CanvasGradient>> CanvasRenderingContext2D::createLinearGradient(float x0, float y0, float x1, float y1)
{
    if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1))
        return Exception { NotSupportedError };

    auto gradient = CanvasGradient::create(FloatPoint(x0, y0), FloatPoint(x1, y1));
    prepareGradientForDashboard(gradient.get());
    return WTFMove(gradient);
}

ExceptionOr<Ref<CanvasGradient>> CanvasRenderingContext2D::createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1)
{
    if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(r0) || !std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(r1))
        return Exception { NotSupportedError };

    if (r0 < 0 || r1 < 0)
        return Exception { IndexSizeError };

    auto gradient = CanvasGradient::create(FloatPoint(x0, y0), r0, FloatPoint(x1, y1), r1);
    prepareGradientForDashboard(gradient.get());
    return WTFMove(gradient);
}

ExceptionOr<RefPtr<CanvasPattern>> CanvasRenderingContext2D::createPattern(CanvasImageSource&& image, const String& repetition)
{
    bool repeatX, repeatY;
    if (!CanvasPattern::parseRepetitionType(repetition, repeatX, repeatY))
        return Exception { SyntaxError };

    return WTF::switchOn(image,
        [&] (auto& element) -> ExceptionOr<RefPtr<CanvasPattern>> { return this->createPattern(*element, repeatX, repeatY); }
    );
}

ExceptionOr<RefPtr<CanvasPattern>> CanvasRenderingContext2D::createPattern(HTMLImageElement& imageElement, bool repeatX, bool repeatY)
{
    auto* cachedImage = imageElement.cachedImage();

    // If the image loading hasn't started or the image is not complete, it is not fully decodable.
    if (!cachedImage || !imageElement.complete())
        return nullptr;

    if (cachedImage->status() == CachedResource::LoadError)
        return Exception { InvalidStateError };

    bool originClean = cachedImage->isOriginClean(canvas().securityOrigin());

    // FIXME: SVG images with animations can switch between clean and dirty (leaking cross-origin
    // data). We should either:
    //   1) Take a fixed snapshot of an SVG image when creating a pattern and determine then whether
    //      the origin is clean.
    //   2) Dynamically verify the origin checks at draw time, and dirty the canvas accordingly.
    // To be on the safe side, taint the origin for all patterns containing SVG images for now.
    if (cachedImage->image()->isSVGImage())
        originClean = false;

    return RefPtr<CanvasPattern> { CanvasPattern::create(*cachedImage->imageForRenderer(imageElement.renderer()), repeatX, repeatY, originClean) };
}

ExceptionOr<RefPtr<CanvasPattern>> CanvasRenderingContext2D::createPattern(HTMLCanvasElement& canvas, bool repeatX, bool repeatY)
{
    if (!canvas.width() || !canvas.height() || !canvas.buffer())
        return Exception { InvalidStateError };

    return RefPtr<CanvasPattern> { CanvasPattern::create(*canvas.copiedImage(), repeatX, repeatY, canvas.originClean()) };
}
    
#if ENABLE(VIDEO)

ExceptionOr<RefPtr<CanvasPattern>> CanvasRenderingContext2D::createPattern(HTMLVideoElement& videoElement, bool repeatX, bool repeatY)
{
    if (videoElement.readyState() < HTMLMediaElement::HAVE_CURRENT_DATA)
        return nullptr;
    
    checkOrigin(&videoElement);
    bool originClean = canvas().originClean();

#if USE(CG) || (ENABLE(ACCELERATED_2D_CANVAS) && USE(GSTREAMER_GL) && USE(CAIRO))
    if (auto nativeImage = videoElement.nativeImageForCurrentTime())
        return RefPtr<CanvasPattern> { CanvasPattern::create(BitmapImage::create(WTFMove(nativeImage)), repeatX, repeatY, originClean) };
#endif

    auto imageBuffer = ImageBuffer::create(size(videoElement), drawingContext() ? drawingContext()->renderingMode() : Accelerated);
    videoElement.paintCurrentFrameInContext(imageBuffer->context(), FloatRect(FloatPoint(), size(videoElement)));
    
    return RefPtr<CanvasPattern> { CanvasPattern::create(ImageBuffer::sinkIntoImage(WTFMove(imageBuffer), Unscaled).releaseNonNull(), repeatX, repeatY, originClean) };
}

#endif

void CanvasRenderingContext2D::didDrawEntireCanvas()
{
    didDraw(FloatRect(FloatPoint::zero(), canvas().size()), CanvasDidDrawApplyClip);
}

void CanvasRenderingContext2D::didDraw(const FloatRect& r, unsigned options)
{
    auto* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;

#if ENABLE(ACCELERATED_2D_CANVAS)
    // If we are drawing to hardware and we have a composited layer, just call contentChanged().
    if (isAccelerated()) {
        RenderBox* renderBox = canvas().renderBox();
        if (renderBox && renderBox->hasAcceleratedCompositing()) {
            renderBox->contentChanged(CanvasPixelsChanged);
            canvas().clearCopiedImage();
            canvas().notifyObserversCanvasChanged(r);
            return;
        }
    }
#endif

    FloatRect dirtyRect = r;
    if (options & CanvasDidDrawApplyTransform) {
        AffineTransform ctm = state().transform;
        dirtyRect = ctm.mapRect(r);
    }

    if (options & CanvasDidDrawApplyShadow && state().shadowColor.isVisible()) {
        // The shadow gets applied after transformation
        FloatRect shadowRect(dirtyRect);
        shadowRect.move(state().shadowOffset);
        shadowRect.inflate(state().shadowBlur);
        dirtyRect.unite(shadowRect);
    }

    if (options & CanvasDidDrawApplyClip) {
        // FIXME: apply the current clip to the rectangle. Unfortunately we can't get the clip
        // back out of the GraphicsContext, so to take clip into account for incremental painting,
        // we'd have to keep the clip path around.
    }

    canvas().didDraw(dirtyRect);
}

void CanvasRenderingContext2D::setTracksDisplayListReplay(bool tracksDisplayListReplay)
{
    if (tracksDisplayListReplay == m_tracksDisplayListReplay)
        return;

    m_tracksDisplayListReplay = tracksDisplayListReplay;
    if (!m_tracksDisplayListReplay)
        contextDisplayListMap().remove(this);
}

String CanvasRenderingContext2D::displayListAsText(DisplayList::AsTextFlags flags) const
{
    if (!m_recordingContext)
        return { };
    return m_recordingContext->displayList.asText(flags);
}

String CanvasRenderingContext2D::replayDisplayListAsText(DisplayList::AsTextFlags flags) const
{
    auto* displayList = contextDisplayListMap().get(this);
    if (!displayList)
        return { };
    return displayList->asText(flags);
}

void CanvasRenderingContext2D::paintRenderingResultsToCanvas()
{
    if (UNLIKELY(m_usesDisplayListDrawing)) {
        if (!m_recordingContext)
            return;

        FloatRect clip(FloatPoint::zero(), canvas().size());
        DisplayList::Replayer replayer(*canvas().drawingContext(), m_recordingContext->displayList);

        if (UNLIKELY(m_tracksDisplayListReplay)) {
            auto replayList = replayer.replay(clip, m_tracksDisplayListReplay);
            contextDisplayListMap().add(this, WTFMove(replayList));
        } else
            replayer.replay(clip);

        m_recordingContext->displayList.clear();
    }
}

GraphicsContext* CanvasRenderingContext2D::drawingContext() const
{
    if (UNLIKELY(m_usesDisplayListDrawing)) {
        if (!m_recordingContext)
            m_recordingContext = std::make_unique<DisplayListDrawingContext>(FloatRect(FloatPoint::zero(), canvas().size()));
        return &m_recordingContext->context;
    }

    return canvas().drawingContext();
}

static RefPtr<ImageData> createEmptyImageData(const IntSize& size)
{
    auto data = ImageData::create(size);
    if (data)
        data->data()->zeroFill();
    return data;
}

ExceptionOr<RefPtr<ImageData>> CanvasRenderingContext2D::createImageData(ImageData* imageData) const
{
    if (!imageData)
        return Exception { NotSupportedError };

    return createEmptyImageData(imageData->size());
}

ExceptionOr<RefPtr<ImageData>> CanvasRenderingContext2D::createImageData(float sw, float sh) const
{
    if (!sw || !sh)
        return Exception { IndexSizeError };

    FloatSize logicalSize(std::abs(sw), std::abs(sh));
    if (!logicalSize.isExpressibleAsIntSize())
        return nullptr;

    IntSize size = expandedIntSize(logicalSize);
    if (size.width() < 1)
        size.setWidth(1);
    if (size.height() < 1)
        size.setHeight(1);

    return createEmptyImageData(size);
}

ExceptionOr<RefPtr<ImageData>> CanvasRenderingContext2D::getImageData(float sx, float sy, float sw, float sh) const
{
    return getImageData(ImageBuffer::LogicalCoordinateSystem, sx, sy, sw, sh);
}

ExceptionOr<RefPtr<ImageData>> CanvasRenderingContext2D::webkitGetImageDataHD(float sx, float sy, float sw, float sh) const
{
    return getImageData(ImageBuffer::BackingStoreCoordinateSystem, sx, sy, sw, sh);
}

ExceptionOr<RefPtr<ImageData>> CanvasRenderingContext2D::getImageData(ImageBuffer::CoordinateSystem coordinateSystem, float sx, float sy, float sw, float sh) const
{
    if (!canvas().originClean()) {
        static NeverDestroyed<String> consoleMessage(MAKE_STATIC_STRING_IMPL("Unable to get image data from canvas because the canvas has been tainted by cross-origin data."));
        canvas().document().addConsoleMessage(MessageSource::Security, MessageLevel::Error, consoleMessage);
        return Exception { SecurityError };
    }

    if (!sw || !sh)
        return Exception { IndexSizeError };

    if (sw < 0) {
        sx += sw;
        sw = -sw;
    }    
    if (sh < 0) {
        sy += sh;
        sh = -sh;
    }

    FloatRect logicalRect(sx, sy, sw, sh);
    if (logicalRect.width() < 1)
        logicalRect.setWidth(1);
    if (logicalRect.height() < 1)
        logicalRect.setHeight(1);
    if (!logicalRect.isExpressibleAsIntRect())
        return nullptr;

    IntRect imageDataRect = enclosingIntRect(logicalRect);
    ImageBuffer* buffer = canvas().buffer();
    if (!buffer)
        return createEmptyImageData(imageDataRect.size());

    auto byteArray = buffer->getUnmultipliedImageData(imageDataRect, nullptr, coordinateSystem);
    if (!byteArray) {
        StringBuilder consoleMessage;
        consoleMessage.appendLiteral("Unable to get image data from canvas. Requested size was ");
        consoleMessage.appendNumber(imageDataRect.width());
        consoleMessage.appendLiteral(" x ");
        consoleMessage.appendNumber(imageDataRect.height());

        canvas().document().addConsoleMessage(MessageSource::Rendering, MessageLevel::Error, consoleMessage.toString());
        return Exception { InvalidStateError };
    }

    return ImageData::create(imageDataRect.size(), byteArray.releaseNonNull());
}

void CanvasRenderingContext2D::putImageData(ImageData& data, float dx, float dy)
{
    putImageData(data, dx, dy, 0, 0, data.width(), data.height());
}

void CanvasRenderingContext2D::webkitPutImageDataHD(ImageData& data, float dx, float dy)
{
    webkitPutImageDataHD(data, dx, dy, 0, 0, data.width(), data.height());
}

void CanvasRenderingContext2D::putImageData(ImageData& data, float dx, float dy, float dirtyX, float dirtyY, float dirtyWidth, float dirtyHeight)
{
    putImageData(data, ImageBuffer::LogicalCoordinateSystem, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight);
}

void CanvasRenderingContext2D::webkitPutImageDataHD(ImageData& data, float dx, float dy, float dirtyX, float dirtyY, float dirtyWidth, float dirtyHeight)
{
    putImageData(data, ImageBuffer::BackingStoreCoordinateSystem, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight);
}

void CanvasRenderingContext2D::drawFocusIfNeeded(Element& element)
{
    drawFocusIfNeededInternal(m_path, element);
}

void CanvasRenderingContext2D::drawFocusIfNeeded(DOMPath& path, Element& element)
{
    drawFocusIfNeededInternal(path.path(), element);
}

void CanvasRenderingContext2D::drawFocusIfNeededInternal(const Path& path, Element& element)
{
    auto* context = drawingContext();
    if (!element.focused() || !state().hasInvertibleTransform || path.isEmpty() || !element.isDescendantOf(canvas()) || !context)
        return;
    context->drawFocusRing(path, 1, 1, RenderTheme::focusRingColor());
}

void CanvasRenderingContext2D::putImageData(ImageData& data, ImageBuffer::CoordinateSystem coordinateSystem, float dx, float dy, float dirtyX, float dirtyY, float dirtyWidth, float dirtyHeight)
{
    ImageBuffer* buffer = canvas().buffer();
    if (!buffer)
        return;

    if (dirtyWidth < 0) {
        dirtyX += dirtyWidth;
        dirtyWidth = -dirtyWidth;
    }

    if (dirtyHeight < 0) {
        dirtyY += dirtyHeight;
        dirtyHeight = -dirtyHeight;
    }

    FloatRect clipRect(dirtyX, dirtyY, dirtyWidth, dirtyHeight);
    clipRect.intersect(IntRect(0, 0, data.width(), data.height()));
    IntSize destOffset(static_cast<int>(dx), static_cast<int>(dy));
    IntRect destRect = enclosingIntRect(clipRect);
    destRect.move(destOffset);
    destRect.intersect(IntRect(IntPoint(), coordinateSystem == ImageBuffer::LogicalCoordinateSystem ? buffer->logicalSize() : buffer->internalSize()));
    if (destRect.isEmpty())
        return;
    IntRect sourceRect(destRect);
    sourceRect.move(-destOffset);
    sourceRect.intersect(IntRect(0, 0, data.width(), data.height()));

    if (!sourceRect.isEmpty())
        buffer->putByteArray(Unmultiplied, data.data(), IntSize(data.width(), data.height()), sourceRect, IntPoint(destOffset), coordinateSystem);

    didDraw(destRect, CanvasDidDrawApplyNone); // ignore transform, shadow and clip
}

String CanvasRenderingContext2D::font() const
{
    if (!state().font.realized())
        return defaultFont;

    StringBuilder serializedFont;
    const auto& fontDescription = state().font.fontDescription();

    if (fontDescription.italic())
        serializedFont.appendLiteral("italic ");
    if (fontDescription.variantCaps() == FontVariantCaps::Small)
        serializedFont.appendLiteral("small-caps ");

    serializedFont.appendNumber(fontDescription.computedPixelSize());
    serializedFont.appendLiteral("px");

    for (unsigned i = 0; i < fontDescription.familyCount(); ++i) {
        if (i)
            serializedFont.append(',');
        // FIXME: We should append family directly to serializedFont rather than building a temporary string.
        String family = fontDescription.familyAt(i);
        if (family.startsWith("-webkit-"))
            family = family.substring(8);
        if (family.contains(' '))
            family = makeString('"', family, '"');

        serializedFont.append(' ');
        serializedFont.append(family);
    }

    return serializedFont.toString();
}

void CanvasRenderingContext2D::setFont(const String& newFont)
{
    if (newFont == state().unparsedFont && state().font.realized())
        return;

    auto parsedStyle = MutableStyleProperties::create();
    CSSParser::parseValue(parsedStyle, CSSPropertyFont, newFont, true, strictToCSSParserMode(!m_usesCSSCompatibilityParseMode));
    if (parsedStyle->isEmpty())
        return;

    String fontValue = parsedStyle->getPropertyValue(CSSPropertyFont);

    // According to http://lists.w3.org/Archives/Public/public-html/2009Jul/0947.html,
    // the "inherit" and "initial" values must be ignored.
    if (fontValue == "inherit" || fontValue == "initial")
        return;

    // The parse succeeded.
    String newFontSafeCopy(newFont); // Create a string copy since newFont can be deleted inside realizeSaves.
    realizeSaves();
    modifiableState().unparsedFont = newFontSafeCopy;

    // Map the <canvas> font into the text style. If the font uses keywords like larger/smaller, these will work
    // relative to the canvas.
    auto newStyle = RenderStyle::createPtr();

    Document& document = canvas().document();
    document.updateStyleIfNeeded();

    if (auto* computedStyle = canvas().computedStyle())
        newStyle->setFontDescription(computedStyle->fontDescription());
    else {
        FontCascadeDescription defaultFontDescription;
        defaultFontDescription.setOneFamily(defaultFontFamily);
        defaultFontDescription.setSpecifiedSize(defaultFontSize);
        defaultFontDescription.setComputedSize(defaultFontSize);

        newStyle->setFontDescription(defaultFontDescription);
    }

    newStyle->fontCascade().update(&document.fontSelector());

    // Now map the font property longhands into the style.
    StyleResolver& styleResolver = canvas().styleResolver();
    styleResolver.applyPropertyToStyle(CSSPropertyFontFamily, parsedStyle->getPropertyCSSValue(CSSPropertyFontFamily).get(), WTFMove(newStyle));
    styleResolver.applyPropertyToCurrentStyle(CSSPropertyFontStyle, parsedStyle->getPropertyCSSValue(CSSPropertyFontStyle).get());
    styleResolver.applyPropertyToCurrentStyle(CSSPropertyFontVariantCaps, parsedStyle->getPropertyCSSValue(CSSPropertyFontVariantCaps).get());
    styleResolver.applyPropertyToCurrentStyle(CSSPropertyFontWeight, parsedStyle->getPropertyCSSValue(CSSPropertyFontWeight).get());

    // As described in BUG66291, setting font-size and line-height on a font may entail a CSSPrimitiveValue::computeLengthDouble call,
    // which assumes the fontMetrics are available for the affected font, otherwise a crash occurs (see http://trac.webkit.org/changeset/96122).
    // The updateFont() calls below update the fontMetrics and ensure the proper setting of font-size and line-height.
    styleResolver.updateFont();
    styleResolver.applyPropertyToCurrentStyle(CSSPropertyFontSize, parsedStyle->getPropertyCSSValue(CSSPropertyFontSize).get());
    styleResolver.updateFont();
    styleResolver.applyPropertyToCurrentStyle(CSSPropertyLineHeight, parsedStyle->getPropertyCSSValue(CSSPropertyLineHeight).get());

    modifiableState().font.initialize(document.fontSelector(), *styleResolver.style());
}

String CanvasRenderingContext2D::textAlign() const
{
    return textAlignName(state().textAlign);
}

void CanvasRenderingContext2D::setTextAlign(const String& s)
{
    TextAlign align;
    if (!parseTextAlign(s, align))
        return;
    if (state().textAlign == align)
        return;
    realizeSaves();
    modifiableState().textAlign = align;
}

String CanvasRenderingContext2D::textBaseline() const
{
    return textBaselineName(state().textBaseline);
}

void CanvasRenderingContext2D::setTextBaseline(const String& s)
{
    TextBaseline baseline;
    if (!parseTextBaseline(s, baseline))
        return;
    if (state().textBaseline == baseline)
        return;
    realizeSaves();
    modifiableState().textBaseline = baseline;
}

inline TextDirection CanvasRenderingContext2D::toTextDirection(Direction direction, const RenderStyle** computedStyle) const
{
    auto* style = (computedStyle || direction == Direction::Inherit) ? canvas().computedStyle() : nullptr;
    if (computedStyle)
        *computedStyle = style;
    switch (direction) {
    case Direction::Inherit:
        return style ? style->direction() : LTR;
    case Direction::RTL:
        return RTL;
    case Direction::LTR:
        return LTR;
    }
    ASSERT_NOT_REACHED();
    return LTR;
}

String CanvasRenderingContext2D::direction() const
{
    if (state().direction == Direction::Inherit)
        canvas().document().updateStyleIfNeeded();
    return toTextDirection(state().direction) == RTL ? ASCIILiteral("rtl") : ASCIILiteral("ltr");
}

void CanvasRenderingContext2D::setDirection(const String& directionString)
{
    Direction direction;
    if (directionString == "inherit")
        direction = Direction::Inherit;
    else if (directionString == "rtl")
        direction = Direction::RTL;
    else if (directionString == "ltr")
        direction = Direction::LTR;
    else
        return;

    if (state().direction == direction)
        return;

    realizeSaves();
    modifiableState().direction = direction;
}

void CanvasRenderingContext2D::fillText(const String& text, float x, float y, std::optional<float> maxWidth)
{
    drawTextInternal(text, x, y, true, maxWidth);
}

void CanvasRenderingContext2D::strokeText(const String& text, float x, float y, std::optional<float> maxWidth)
{
    drawTextInternal(text, x, y, false, maxWidth);
}

static inline bool isSpaceThatNeedsReplacing(UChar c)
{
    // According to specification all space characters should be replaced with 0x0020 space character.
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-canvas-element.html#text-preparation-algorithm
    // The space characters according to specification are : U+0020, U+0009, U+000A, U+000C, and U+000D.
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/common-microsyntaxes.html#space-character
    // This function returns true for 0x000B also, so that this is backward compatible.
    // Otherwise, the test LayoutTests/canvas/philip/tests/2d.text.draw.space.collapse.space.html will fail
    return c == 0x0009 || c == 0x000A || c == 0x000B || c == 0x000C || c == 0x000D;
}

static void normalizeSpaces(String& text)
{
    size_t i = text.find(isSpaceThatNeedsReplacing);
    if (i == notFound)
        return;

    unsigned textLength = text.length();
    Vector<UChar> charVector(textLength);
    StringView(text).getCharactersWithUpconvert(charVector.data());

    charVector[i++] = ' ';

    for (; i < textLength; ++i) {
        if (isSpaceThatNeedsReplacing(charVector[i]))
            charVector[i] = ' ';
    }
    text = String::adopt(WTFMove(charVector));
}

Ref<TextMetrics> CanvasRenderingContext2D::measureText(const String& text)
{
    Ref<TextMetrics> metrics = TextMetrics::create();

    String normalizedText = text;
    normalizeSpaces(normalizedText);

    const RenderStyle* computedStyle;
    auto direction = toTextDirection(state().direction, &computedStyle);
    bool override = computedStyle ? isOverride(computedStyle->unicodeBidi()) : false;

    TextRun textRun(normalizedText, 0, 0, AllowTrailingExpansion, direction, override, true);
    auto& font = fontProxy();
    auto& fontMetrics = font.fontMetrics();

    GlyphOverflow glyphOverflow;
    glyphOverflow.computeBounds = true;
    float fontWidth = font.width(textRun, &glyphOverflow);
    metrics->setWidth(fontWidth);

    FloatPoint offset = textOffset(fontWidth, direction);

    metrics->setActualBoundingBoxAscent(glyphOverflow.top - offset.y());
    metrics->setActualBoundingBoxDescent(glyphOverflow.bottom + offset.y());
    metrics->setFontBoundingBoxAscent(fontMetrics.ascent() - offset.y());
    metrics->setFontBoundingBoxDescent(fontMetrics.descent() + offset.y());
    metrics->setEmHeightAscent(fontMetrics.ascent() - offset.y());
    metrics->setEmHeightDescent(fontMetrics.descent() + offset.y());
    metrics->setHangingBaseline(fontMetrics.ascent() - offset.y());
    metrics->setAlphabeticBaseline(-offset.y());
    metrics->setIdeographicBaseline(-fontMetrics.descent() - offset.y());

    metrics->setActualBoundingBoxLeft(glyphOverflow.left - offset.x());
    metrics->setActualBoundingBoxRight(fontWidth + glyphOverflow.right + offset.x());

    return metrics;
}

FloatPoint CanvasRenderingContext2D::textOffset(float width, TextDirection direction)
{
    auto& fontMetrics = fontProxy().fontMetrics();
    FloatPoint offset;

    switch (state().textBaseline) {
    case TopTextBaseline:
    case HangingTextBaseline:
        offset.setY(fontMetrics.ascent());
        break;
    case BottomTextBaseline:
    case IdeographicTextBaseline:
        offset.setY(-fontMetrics.descent());
        break;
    case MiddleTextBaseline:
        offset.setY(fontMetrics.height() / 2 - fontMetrics.descent());
        break;
    case AlphabeticTextBaseline:
    default:
        break;
    }

    bool isRTL = direction == RTL;
    auto align = state().textAlign;
    if (align == StartTextAlign)
        align = isRTL ? RightTextAlign : LeftTextAlign;
    else if (align == EndTextAlign)
        align = isRTL ? LeftTextAlign : RightTextAlign;

    switch (align) {
    case CenterTextAlign:
        offset.setX(-width / 2);
        break;
    case RightTextAlign:
        offset.setX(-width);
        break;
    default:
        break;
    }
    return offset;
}

void CanvasRenderingContext2D::drawTextInternal(const String& text, float x, float y, bool fill, std::optional<float> maxWidth)
{
    auto& fontProxy = this->fontProxy();
    const auto& fontMetrics = fontProxy.fontMetrics();

    auto* c = drawingContext();
    if (!c)
        return;
    if (!state().hasInvertibleTransform)
        return;
    if (!std::isfinite(x) | !std::isfinite(y))
        return;
    if (maxWidth && (!std::isfinite(maxWidth.value()) || maxWidth.value() <= 0))
        return;

    // If gradient size is zero, then paint nothing.
    auto* gradient = c->strokeGradient();
    if (!fill && gradient && gradient->isZeroSize())
        return;

    gradient = c->fillGradient();
    if (fill && gradient && gradient->isZeroSize())
        return;

    String normalizedText = text;
    normalizeSpaces(normalizedText);

    // FIXME: Need to turn off font smoothing.

    const RenderStyle* computedStyle;
    auto direction = toTextDirection(state().direction, &computedStyle);
    bool override = computedStyle ? isOverride(computedStyle->unicodeBidi()) : false;

    TextRun textRun(normalizedText, 0, 0, AllowTrailingExpansion, direction, override, true);
    float fontWidth = fontProxy.width(textRun);
    bool useMaxWidth = maxWidth && maxWidth.value() < fontWidth;
    float width = useMaxWidth ? maxWidth.value() : fontWidth;
    FloatPoint location(x, y);
    location += textOffset(width, direction);

    // The slop built in to this mask rect matches the heuristic used in FontCGWin.cpp for GDI text.
    FloatRect textRect = FloatRect(location.x() - fontMetrics.height() / 2, location.y() - fontMetrics.ascent() - fontMetrics.lineGap(),
        width + fontMetrics.height(), fontMetrics.lineSpacing());
    if (!fill)
        inflateStrokeRect(textRect);

#if USE(CG)
    const CanvasStyle& drawStyle = fill ? state().fillStyle : state().strokeStyle;
    if (drawStyle.canvasGradient() || drawStyle.canvasPattern()) {
        IntRect maskRect = enclosingIntRect(textRect);

        // If we have a shadow, we need to draw it before the mask operation.
        // Follow a procedure similar to paintTextWithShadows in TextPainter.

        if (shouldDrawShadows()) {
            GraphicsContextStateSaver stateSaver(*c);

            FloatSize offset(0, 2 * maskRect.height());

            FloatSize shadowOffset;
            float shadowRadius;
            Color shadowColor;
            c->getShadow(shadowOffset, shadowRadius, shadowColor);

            FloatRect shadowRect(maskRect);
            shadowRect.inflate(shadowRadius * 1.4);
            shadowRect.move(shadowOffset * -1);
            c->clip(shadowRect);

            shadowOffset += offset;

            c->setLegacyShadow(shadowOffset, shadowRadius, shadowColor);

            if (fill)
                c->setFillColor(Color::black);
            else
                c->setStrokeColor(Color::black);

            fontProxy.drawBidiText(*c, textRun, location + offset, FontCascade::UseFallbackIfFontNotReady);
        }

        auto maskImage = ImageBuffer::createCompatibleBuffer(maskRect.size(), ColorSpaceSRGB, *c);
        if (!maskImage)
            return;

        auto& maskImageContext = maskImage->context();

        if (fill)
            maskImageContext.setFillColor(Color::black);
        else {
            maskImageContext.setStrokeColor(Color::black);
            maskImageContext.setStrokeThickness(c->strokeThickness());
        }

        maskImageContext.setTextDrawingMode(fill ? TextModeFill : TextModeStroke);

        if (useMaxWidth) {
            maskImageContext.translate(location.x() - maskRect.x(), location.y() - maskRect.y());
            // We draw when fontWidth is 0 so compositing operations (eg, a "copy" op) still work.
            maskImageContext.scale(FloatSize((fontWidth > 0 ? (width / fontWidth) : 0), 1));
            fontProxy.drawBidiText(maskImageContext, textRun, FloatPoint(0, 0), FontCascade::UseFallbackIfFontNotReady);
        } else {
            maskImageContext.translate(-maskRect.x(), -maskRect.y());
            fontProxy.drawBidiText(maskImageContext, textRun, location, FontCascade::UseFallbackIfFontNotReady);
        }

        GraphicsContextStateSaver stateSaver(*c);
        c->clipToImageBuffer(*maskImage, maskRect);
        drawStyle.applyFillColor(*c);
        c->fillRect(maskRect);
        return;
    }
#endif

    c->setTextDrawingMode(fill ? TextModeFill : TextModeStroke);

    GraphicsContextStateSaver stateSaver(*c);
    if (useMaxWidth) {
        c->translate(location.x(), location.y());
        // We draw when fontWidth is 0 so compositing operations (eg, a "copy" op) still work.
        c->scale(FloatSize((fontWidth > 0 ? (width / fontWidth) : 0), 1));
        location = FloatPoint();
    }

    if (isFullCanvasCompositeMode(state().globalComposite)) {
        beginCompositeLayer();
        fontProxy.drawBidiText(*c, textRun, location, FontCascade::UseFallbackIfFontNotReady);
        endCompositeLayer();
        didDrawEntireCanvas();
    } else if (state().globalComposite == CompositeCopy) {
        clearCanvas();
        fontProxy.drawBidiText(*c, textRun, location, FontCascade::UseFallbackIfFontNotReady);
        didDrawEntireCanvas();
    } else {
        fontProxy.drawBidiText(*c, textRun, location, FontCascade::UseFallbackIfFontNotReady);
        didDraw(textRect);
    }
}

void CanvasRenderingContext2D::inflateStrokeRect(FloatRect& rect) const
{
    // Fast approximation of the stroke's bounding rect.
    // This yields a slightly oversized rect but is very fast
    // compared to Path::strokeBoundingRect().
    static const float root2 = sqrtf(2);
    float delta = state().lineWidth / 2;
    if (state().lineJoin == MiterJoin)
        delta *= state().miterLimit;
    else if (state().lineCap == SquareCap)
        delta *= root2;
    rect.inflate(delta);
}

auto CanvasRenderingContext2D::fontProxy() -> const FontProxy&
{
    canvas().document().updateStyleIfNeeded();
    if (!state().font.realized())
        setFont(state().unparsedFont);
    return state().font;
}

#if ENABLE(ACCELERATED_2D_CANVAS)

PlatformLayer* CanvasRenderingContext2D::platformLayer() const
{
    return canvas().buffer() ? canvas().buffer()->platformLayer() : nullptr;
}

#endif

static inline InterpolationQuality smoothingToInterpolationQuality(CanvasRenderingContext2D::ImageSmoothingQuality quality)
{
    switch (quality) {
    case CanvasRenderingContext2D::ImageSmoothingQuality::Low:
        return InterpolationLow;
    case CanvasRenderingContext2D::ImageSmoothingQuality::Medium:
        return InterpolationMedium;
    case CanvasRenderingContext2D::ImageSmoothingQuality::High:
        return InterpolationHigh;
    }

    ASSERT_NOT_REACHED();
    return InterpolationLow;
};

String CanvasRenderingContext2D::stringForImageSmoothingQuality(ImageSmoothingQuality imageSmoothingQuality)
{
    switch (imageSmoothingQuality) {
    case ImageSmoothingQuality::Low:
        return ASCIILiteral("low");
    case ImageSmoothingQuality::Medium:
        return ASCIILiteral("medium");
    case ImageSmoothingQuality::High:
        return ASCIILiteral("high");
    }

    ASSERT_NOT_REACHED();
    return String();
}

auto CanvasRenderingContext2D::imageSmoothingQuality() const -> ImageSmoothingQuality
{
    return state().imageSmoothingQuality;
}

void CanvasRenderingContext2D::setImageSmoothingQuality(ImageSmoothingQuality quality)
{
    if (quality == state().imageSmoothingQuality)
        return;

    realizeSaves();
    modifiableState().imageSmoothingQuality = quality;

    if (!state().imageSmoothingEnabled)
        return;

    if (auto* context = drawingContext())
        context->setImageInterpolationQuality(smoothingToInterpolationQuality(quality));
}

bool CanvasRenderingContext2D::imageSmoothingEnabled() const
{
    return state().imageSmoothingEnabled;
}

void CanvasRenderingContext2D::setImageSmoothingEnabled(bool enabled)
{
    if (enabled == state().imageSmoothingEnabled)
        return;

    realizeSaves();
    modifiableState().imageSmoothingEnabled = enabled;
    auto* c = drawingContext();
    if (c)
        c->setImageInterpolationQuality(enabled ? smoothingToInterpolationQuality(state().imageSmoothingQuality) : InterpolationNone);
}

void CanvasRenderingContext2D::setPath(DOMPath& path)
{
    m_path = path.path();
}

Ref<DOMPath> CanvasRenderingContext2D::getPath() const
{
    return DOMPath::create(m_path);
}

} // namespace WebCore
