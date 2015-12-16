/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
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
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebEditorClient_H
#define WebEditorClient_H

#include <EditorClient.h>
#include <TextCheckerClient.h> 
#include <wtf/text/StringView.h>

class WebView;
class WebNotification;
class WebEditorUndoTarget;

class WebEditorClient : public WebCore::EditorClient, public WebCore::TextCheckerClient {
public:
    WebEditorClient(WebView*);
    virtual ~WebEditorClient();

    virtual void pageDestroyed();
    virtual void frameWillDetachPage(WebCore::Frame*) { } 

    virtual bool isContinuousSpellCheckingEnabled();
    virtual void toggleGrammarChecking();
    virtual bool isGrammarCheckingEnabled();
    virtual void toggleContinuousSpellChecking();
    virtual int spellCheckerDocumentTag();

    virtual bool isEditable();

    virtual bool shouldBeginEditing(WebCore::Range*);
    virtual bool shouldEndEditing(WebCore::Range*);
    virtual bool shouldInsertText(const WTF::String&, WebCore::Range*, WebCore::EditorInsertAction);

    virtual void didBeginEditing();
    virtual void didEndEditing();
    virtual void willWriteSelectionToPasteboard(WebCore::Range*);
    virtual void didWriteSelectionToPasteboard();
    virtual void getClientPasteboardDataForRange(WebCore::Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData);
    virtual void didSetSelectionTypesForPasteboard();

    virtual void respondToChangedContents();
    virtual void respondToChangedSelection(WebCore::Frame*);

    bool shouldShowDeleteInterface(WebCore::HTMLElement*);
    bool shouldDeleteRange(WebCore::Range*);

    bool shouldInsertNode(WebCore::Node*, WebCore::Range* replacingRange, WebCore::EditorInsertAction);
    bool shouldApplyStyle(WebCore::StyleProperties*, WebCore::Range*);
    void didApplyStyle() override;
    bool shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*);
    bool shouldChangeTypingStyle(WebCore::StyleProperties* currentStyle, WebCore::StyleProperties* toProposedStyle);

    void webViewDidChangeTypingStyle(WebNotification*);
    void webViewDidChangeSelection(WebNotification*);

    bool smartInsertDeleteEnabled();
    bool isSelectTrailingWhitespaceEnabled();

    void registerUndoStep(PassRefPtr<WebCore::UndoStep>);
    void registerRedoStep(PassRefPtr<WebCore::UndoStep>);
    void clearUndoRedoOperations();

    bool canCopyCut(WebCore::Frame*, bool defaultValue) const;
    bool canPaste(WebCore::Frame*, bool defaultValue) const; 

    bool canUndo() const;
    bool canRedo() const;
    
    void undo();
    void redo();    
    
    virtual bool shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity, bool stillSelecting);
    virtual void textFieldDidBeginEditing(WebCore::Element*);
    virtual void textFieldDidEndEditing(WebCore::Element*);
    virtual void textDidChangeInTextField(WebCore::Element*);
    virtual bool doTextFieldCommandFromEvent(WebCore::Element*, WebCore::KeyboardEvent*);
    virtual void textWillBeDeletedInTextField(WebCore::Element* input);
    virtual void textDidChangeInTextArea(WebCore::Element*);

    void handleKeyboardEvent(WebCore::KeyboardEvent*);
    void handleInputMethodKeydown(WebCore::KeyboardEvent*);

    virtual bool shouldEraseMarkersAfterChangeSelection(WebCore::TextCheckingType) const; 
    virtual void ignoreWordInSpellDocument(const WTF::String&);
    virtual void learnWord(const WTF::String&);
    virtual void checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength);
    virtual void checkGrammarOfString(const UChar*, int length, Vector<WebCore::GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength);
    virtual WTF::String getAutoCorrectSuggestionForMisspelledWord(const WTF::String&);
    virtual void updateSpellingUIWithGrammarString(const WTF::String&, const WebCore::GrammarDetail& detail);
    virtual void updateSpellingUIWithMisspelledWord(const WTF::String&);
    virtual void showSpellingUI(bool show);
    virtual bool spellingUIIsShowing();
    virtual void getGuessesForWord(const WTF::String& word, const WTF::String& context, WTF::Vector<WTF::String>& guesses);

    virtual void willSetInputMethodState();

    virtual void setInputMethodState(bool);
    virtual void requestCheckingOfString(WTF::PassRefPtr<WebCore::TextCheckingRequest>) { } 
    virtual WebCore::TextCheckerClient* textChecker() { return this; } 

    virtual void checkSpellingOfString(StringView, int* misspellingLocation, int* misspellingLength) { };
    virtual void checkGrammarOfString(StringView, Vector<WebCore::GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength) { };
    void overflowScrollPositionChanged() { };
    virtual void discardedComposition(WebCore::Frame *) override;
    virtual void didChangeSelectionAndUpdateLayout() override;

private:
    WebView* m_webView;
    WebEditorUndoTarget* m_undoTarget;
};

#endif // WebEditorClient_H
