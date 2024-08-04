/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef EditorState_h
#define EditorState_h

#include "ArgumentCoders.h"
#include <WebCore/Color.h>
#include <WebCore/IntRect.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(IOS)
#include <WebCore/SelectionRect.h>
#endif

namespace WebKit {

enum TypingAttributes {
    AttributeNone = 0,
    AttributeBold = 1,
    AttributeItalics = 2,
    AttributeUnderline = 4,
    AttributeStrikeThrough = 8
};

enum TextAlignment {
    NoAlignment = 0,
    LeftAlignment = 1,
    RightAlignment = 2,
    CenterAlignment = 3,
    JustifiedAlignment = 4,
};

enum ListType {
    NoList = 0,
    OrderedList,
    UnorderedList
};

struct EditorState {
    bool shouldIgnoreCompositionSelectionChange { false };

    bool selectionIsNone { true }; // This will be false when there is a caret selection.
    bool selectionIsRange { false };
    bool isContentEditable { false };
    bool isContentRichlyEditable { false };
    bool isInPasswordField { false };
    bool isInPlugin { false };
    bool hasComposition { false };
    bool isMissingPostLayoutData { false };

#if PLATFORM(IOS)
    WebCore::IntRect firstMarkedRect;
    WebCore::IntRect lastMarkedRect;
    String markedText;
#endif

#if PLATFORM(IOS) || PLATFORM(GTK) || PLATFORM(MAC)
    struct PostLayoutData {
        uint32_t typingAttributes { AttributeNone };
#if PLATFORM(IOS) || PLATFORM(GTK)
        WebCore::IntRect caretRectAtStart;
#endif
#if PLATFORM(IOS) || PLATFORM(MAC)
        WebCore::IntRect selectionClipRect;
        uint64_t selectedTextLength { 0 };
        uint32_t textAlignment { NoAlignment };
        WebCore::Color textColor { WebCore::Color::black };
        uint32_t enclosingListType { NoList };
#endif
#if PLATFORM(IOS)
        WebCore::IntRect caretRectAtEnd;
        Vector<WebCore::SelectionRect> selectionRects;
        String wordAtSelection;
        UChar32 characterAfterSelection { 0 };
        UChar32 characterBeforeSelection { 0 };
        UChar32 twoCharacterBeforeSelection { 0 };
        bool isReplaceAllowed { false };
        bool hasContent { false };
        bool isStableStateUpdate { false };
        bool insideFixedPosition { false };
#endif
#if PLATFORM(MAC)
        uint64_t candidateRequestStartPosition { 0 };
        String paragraphContextForCandidateRequest;
        String stringForCandidateRequest;
#endif

        void encode(IPC::Encoder&) const;
        static bool decode(IPC::Decoder&, PostLayoutData&);
    };

    const PostLayoutData& postLayoutData() const;
    PostLayoutData& postLayoutData();
#endif // PLATFORM(IOS) || PLATFORM(GTK) || PLATFORM(MAC)

    void encode(IPC::Encoder&) const;
    static bool decode(IPC::Decoder&, EditorState&);

#if PLATFORM(IOS) || PLATFORM(GTK) || PLATFORM(MAC)
private:
    PostLayoutData m_postLayoutData;
#endif
};

#if PLATFORM(IOS) || PLATFORM(GTK) || PLATFORM(MAC)
inline auto EditorState::postLayoutData() -> PostLayoutData&
{
    ASSERT_WITH_MESSAGE(!isMissingPostLayoutData, "Attempt to access post layout data before receiving it");
    return m_postLayoutData;
}

inline auto EditorState::postLayoutData() const -> const PostLayoutData&
{
    ASSERT_WITH_MESSAGE(!isMissingPostLayoutData, "Attempt to access post layout data before receiving it");
    return m_postLayoutData;
}
#endif

}

#endif // EditorState_h
