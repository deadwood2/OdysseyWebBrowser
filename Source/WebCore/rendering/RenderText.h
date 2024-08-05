/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#pragma once

#include "RenderElement.h"
#include "RenderTextLineBoxes.h"
#include "SimpleLineLayout.h"
#include "Text.h"
#include <wtf/Forward.h>
#include <wtf/text/TextBreakIterator.h>

namespace WebCore {

class Font;
class InlineTextBox;
struct GlyphOverflow;

class RenderText : public RenderObject {
public:
    RenderText(Text&, const String&);
    RenderText(Document&, const String&);

    virtual ~RenderText();

    const char* renderName() const override;

    WEBCORE_EXPORT Text* textNode() const;

    virtual bool isTextFragment() const;

    const RenderStyle& style() const;
    const RenderStyle& firstLineStyle() const;
    const RenderStyle* getCachedPseudoStyle(PseudoId, const RenderStyle* parentStyle = nullptr) const;

    Color selectionBackgroundColor() const;
    Color selectionForegroundColor() const;
    Color selectionEmphasisMarkColor() const;

    virtual String originalText() const;

    void extractTextBox(InlineTextBox& box) { m_lineBoxes.extract(box); }
    void attachTextBox(InlineTextBox& box) { m_lineBoxes.attach(box); }
    void removeTextBox(InlineTextBox& box) { m_lineBoxes.remove(box); }

    StringImpl* text() const { return m_text.impl(); }
    String textWithoutConvertingBackslashToYenSymbol() const;

    InlineTextBox* createInlineTextBox() { return m_lineBoxes.createAndAppendLineBox(*this); }
    void dirtyLineBoxes(bool fullLayout);

    void absoluteRects(Vector<IntRect>&, const LayoutPoint& accumulatedOffset) const final;
    Vector<IntRect> absoluteRectsForRange(unsigned startOffset = 0, unsigned endOffset = UINT_MAX, bool useSelectionHeight = false, bool* wasFixed = nullptr) const;
#if PLATFORM(IOS)
    void collectSelectionRects(Vector<SelectionRect>&, unsigned startOffset = 0, unsigned endOffset = std::numeric_limits<unsigned>::max()) override;
#endif

    void absoluteQuads(Vector<FloatQuad>&, bool* wasFixed) const final;
    Vector<FloatQuad> absoluteQuadsForRange(unsigned startOffset = 0, unsigned endOffset = UINT_MAX, bool useSelectionHeight = false, bool* wasFixed = nullptr) const;

    Vector<FloatQuad> absoluteQuadsClippedToEllipsis() const;

    Position positionForPoint(const LayoutPoint&) override;
    VisiblePosition positionForPoint(const LayoutPoint&, const RenderRegion*) override;

    bool is8Bit() const { return m_text.impl()->is8Bit(); }
    const LChar* characters8() const { return m_text.impl()->characters8(); }
    const UChar* characters16() const { return m_text.impl()->characters16(); }
    UChar characterAt(unsigned) const;
    UChar uncheckedCharacterAt(unsigned) const;
    UChar operator[](unsigned i) const { return uncheckedCharacterAt(i); }
    unsigned textLength() const { return m_text.impl()->length(); } // non virtual implementation of length()
    void positionLineBox(InlineTextBox&);

    virtual float width(unsigned from, unsigned len, const FontCascade&, float xPos, HashSet<const Font*>* fallbackFonts = 0, GlyphOverflow* = 0) const;
    virtual float width(unsigned from, unsigned len, float xPos, bool firstLine = false, HashSet<const Font*>* fallbackFonts = 0, GlyphOverflow* = 0) const;

    float minLogicalWidth() const;
    float maxLogicalWidth() const;

    void trimmedPrefWidths(float leadWidth,
                           float& beginMinW, bool& beginWS,
                           float& endMinW, bool& endWS,
                           bool& hasBreakableChar, bool& hasBreak,
                           float& beginMaxW, float& endMaxW,
                           float& minW, float& maxW, bool& stripFrontSpaces);
    float hangablePunctuationStartWidth(unsigned index) const;
    float hangablePunctuationEndWidth(unsigned index) const;
    unsigned firstCharacterIndexStrippingSpaces() const;
    unsigned lastCharacterIndexStrippingSpaces() const;
    bool isHangableStopOrComma(UChar) const;
    
    WEBCORE_EXPORT virtual IntRect linesBoundingBox() const;
    LayoutRect linesVisualOverflowBoundingBox() const;

    WEBCORE_EXPORT IntPoint firstRunLocation() const;

    virtual void setText(const String&, bool force = false);
    void setTextWithOffset(const String&, unsigned offset, unsigned len, bool force = false);

    bool canBeSelectionLeaf() const override { return true; }
    void setSelectionState(SelectionState) final;
    LayoutRect selectionRectForRepaint(const RenderLayerModelObject* repaintContainer, bool clipToVisibleContent = true) override;
    LayoutRect localCaretRect(InlineBox*, unsigned caretOffset, LayoutUnit* extraWidthToEndOfLine = 0) override;

    LayoutRect collectSelectionRectsForLineBoxes(const RenderLayerModelObject* repaintContainer, bool clipToVisibleContent, Vector<LayoutRect>& rects);

    LayoutUnit marginLeft() const { return minimumValueForLength(style().marginLeft(), 0); }
    LayoutUnit marginRight() const { return minimumValueForLength(style().marginRight(), 0); }

    LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject* repaintContainer) const final;

    InlineTextBox* firstTextBox() const { return m_lineBoxes.first(); }
    InlineTextBox* lastTextBox() const { return m_lineBoxes.last(); }

    int caretMinOffset() const override;
    int caretMaxOffset() const override;
    unsigned countRenderedCharacterOffsetsUntil(unsigned) const;
    bool containsRenderedCharacterOffset(unsigned) const;
    bool containsCaretOffset(unsigned) const;
    bool hasRenderedText() const;

    // FIXME: These should return unsigneds.
    int previousOffset(int current) const final;
    int previousOffsetForBackwardDeletion(int current) const final;
    int nextOffset(int current) const final;

    bool containsReversedText() const { return m_containsReversedText; }

    void momentarilyRevealLastTypedCharacter(unsigned offsetAfterLastTypedCharacter);

    InlineTextBox* findNextInlineTextBox(int offset, int& pos) const { return m_lineBoxes.findNext(offset, pos); }

    bool isAllCollapsibleWhitespace() const;

    bool canUseSimpleFontCodePath() const { return m_canUseSimpleFontCodePath; }

    void removeAndDestroyTextBoxes();

    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle);

    virtual std::unique_ptr<InlineTextBox> createTextBox(); // Subclassed by RenderSVGInlineText.

#if ENABLE(TEXT_AUTOSIZING)
    float candidateComputedTextSize() const { return m_candidateComputedTextSize; }
    void setCandidateComputedTextSize(float s) { m_candidateComputedTextSize = s; }
#endif

    void ensureLineBoxes();
    void deleteLineBoxesBeforeSimpleLineLayout();
    const SimpleLineLayout::Layout* simpleLineLayout() const;

    StringView stringView(unsigned start = 0, std::optional<unsigned> stop = std::nullopt) const;

    LayoutUnit topOfFirstText() const;
    
    bool containsOnlyWhitespace(unsigned from, unsigned len) const;
    
    bool canUseSimplifiedTextMeasuring() const { return m_canUseSimplifiedTextMeasuring; }

    Vector<std::pair<unsigned, unsigned>> draggedContentRangesBetweenOffsets(unsigned startOffset, unsigned endOffset) const;

protected:
    virtual void computePreferredLogicalWidths(float leadWidth);
    void willBeDestroyed() override;

    virtual void setRenderedText(const String&);
    virtual UChar previousCharacter() const;

    RenderTextLineBoxes m_lineBoxes;

private:
    RenderText(Node&, const String&);

    bool canHaveChildren() const final { return false; }

    void computePreferredLogicalWidths(float leadWidth, HashSet<const Font*>& fallbackFonts, GlyphOverflow&);

    bool computeCanUseSimpleFontCodePath() const;
    
    // Make length() private so that callers that have a RenderText*
    // will use the more efficient textLength() instead, while
    // callers with a RenderObject* can continue to use length().
    unsigned length() const final { return textLength(); }

    bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation&, const LayoutPoint&, HitTestAction) final { ASSERT_NOT_REACHED(); return false; }

    float widthFromCache(const FontCascade&, unsigned start, unsigned len, float xPos, HashSet<const Font*>* fallbackFonts, GlyphOverflow*, const RenderStyle&) const;
    bool isAllASCII() const { return m_isAllASCII; }
    bool computeUseBackslashAsYenSymbol() const;

    void secureText(UChar mask);

    LayoutRect collectSelectionRectsForLineBoxes(const RenderLayerModelObject* repaintContainer, bool clipToVisibleContent, Vector<LayoutRect>*);
    bool computeCanUseSimplifiedTextMeasuring() const;

    void node() const = delete;
    void container() const = delete; // Use parent() instead.
    void container(const RenderLayerModelObject&, bool&) const = delete; // Use parent() instead.

    // We put the bitfield first to minimize padding on 64-bit.
    unsigned m_hasBreakableChar : 1; // Whether or not we can be broken into multiple lines.
    unsigned m_hasBreak : 1; // Whether or not we have a hard break (e.g., <pre> with '\n').
    unsigned m_hasTab : 1; // Whether or not we have a variable width tab character (e.g., <pre> with '\t').
    unsigned m_hasBeginWS : 1; // Whether or not we begin with WS (only true if we aren't pre)
    unsigned m_hasEndWS : 1; // Whether or not we end with WS (only true if we aren't pre)
    unsigned m_linesDirty : 1; // This bit indicates that the text run has already dirtied specific
                           // line boxes, and this hint will enable layoutInlineChildren to avoid
                           // just dirtying everything when character data is modified (e.g., appended/inserted
                           // or removed).
    unsigned m_containsReversedText : 1;
    unsigned m_isAllASCII : 1;
    unsigned m_canUseSimpleFontCodePath : 1;
    mutable unsigned m_knownToHaveNoOverflowAndNoFallbackFonts : 1;
    unsigned m_useBackslashAsYenSymbol : 1;
    unsigned m_originalTextDiffersFromRendered : 1;
    unsigned m_canUseSimplifiedTextMeasuring : 1;

#if ENABLE(TEXT_AUTOSIZING)
    // FIXME: This should probably be part of the text sizing structures in Document instead. That would save some memory.
    float m_candidateComputedTextSize;
#endif
    float m_minWidth;
    float m_maxWidth;
    float m_beginMinWidth;
    float m_endMinWidth;

    String m_text;
};

inline UChar RenderText::uncheckedCharacterAt(unsigned i) const
{
    ASSERT_WITH_SECURITY_IMPLICATION(i < textLength());
    return is8Bit() ? characters8()[i] : characters16()[i];
}

inline UChar RenderText::characterAt(unsigned i) const
{
    if (i >= textLength())
        return 0;

    return uncheckedCharacterAt(i);
}

inline const RenderStyle& RenderText::style() const
{
    return parent()->style();
}

inline const RenderStyle& RenderText::firstLineStyle() const
{
    return parent()->firstLineStyle();
}

inline const RenderStyle* RenderText::getCachedPseudoStyle(PseudoId pseudoId, const RenderStyle* parentStyle) const
{
    return parent()->getCachedPseudoStyle(pseudoId, parentStyle);
}

inline Color RenderText::selectionBackgroundColor() const
{
    return parent()->selectionBackgroundColor();
}

inline Color RenderText::selectionForegroundColor() const
{
    return parent()->selectionForegroundColor();
}

inline Color RenderText::selectionEmphasisMarkColor() const
{
    return parent()->selectionEmphasisMarkColor();
}

void applyTextTransform(const RenderStyle&, String&, UChar);
void makeCapitalized(String*, UChar previous);
LineBreakIteratorMode mapLineBreakToIteratorMode(LineBreak);
    
inline RenderText* Text::renderer() const
{
    return downcast<RenderText>(Node::renderer());
}

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderText, isText())
