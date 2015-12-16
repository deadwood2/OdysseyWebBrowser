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

#include "config.h"
#include "WebEditorClient.h"

#include "DOMRange.h"
#include "DOMCSSClasses.h"
#include "DOMHTMLClasses.h"
#include "WebEditingDelegate.h"
#include "WebView.h"

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <Document.h>
#include <HTMLElement.h>
#include <HTMLInputElement.h>
#include <HTMLNames.h>
#include <KeyboardEvent.h>
#include <PlatformKeyboardEvent.h>
#include <NotImplemented.h>
#include <Range.h>
#include <UndoStep.h>
#include <VisibleSelection.h> 

#include "TextBoundaries.h"
#include "gui.h"
#include "utils.h"
#if !OS(AROS)
#include <proto/spellchecker.h>
#endif
#include <clib/debug_protos.h>

#define D(x)

using namespace WebCore;
using namespace HTMLNames;


class WebEditorUndoCommand
{
public:
    WebEditorUndoCommand(PassRefPtr<UndoStep>, bool isUndo);
    void execute();
    bool isUndo() { return m_isUndo; }

private:
	RefPtr<UndoStep> m_step;
    bool m_isUndo;
};

WebEditorUndoCommand::WebEditorUndoCommand(PassRefPtr<UndoStep> step, bool isUndo)
    : m_step(step)
    , m_isUndo(isUndo) 
{ 
}

void WebEditorUndoCommand::execute()
{
    if (m_isUndo)
        m_step->unapply();
    else
		m_step->reapply();
}

class WebEditorUndoTarget
{
public:
    WebEditorUndoTarget();
    ~WebEditorUndoTarget();
    void invoke(String actionName, WebEditorUndoCommand* obj);
    void append(String actionName, WebEditorUndoCommand* obj);
    void clear();
    bool canUndo();
    bool canRedo();
    void undo();
    void redo();

private:
    Vector<WebEditorUndoCommand*> m_undoCommandList;
    Vector<String> m_actionName;
};

WebEditorUndoTarget::WebEditorUndoTarget()
{
}

WebEditorUndoTarget::~WebEditorUndoTarget()
{
    clear();
}

void WebEditorUndoTarget::invoke(String actionName, WebEditorUndoCommand *undoCommand)
{
    if(undoCommand) {
        undoCommand->execute();
    }
}


void WebEditorUndoTarget::append(String actionName, WebEditorUndoCommand *undoCommand)
{
    m_undoCommandList.append(undoCommand);
    m_actionName.append(actionName);
}

void WebEditorUndoTarget::clear()
{
    m_undoCommandList.clear();
    m_actionName.clear();
}

bool WebEditorClient::canCopyCut(Frame*, bool defaultValue) const 
{
    return defaultValue;
}

bool WebEditorClient::canPaste(Frame*, bool defaultValue) const 
{
    return defaultValue;
}

bool WebEditorUndoTarget::canUndo()
{
    for (unsigned i = 0; i < m_undoCommandList.size(); ++i) {
        if (m_undoCommandList[i]->isUndo())
            return true;
    }
    return false;
}

bool WebEditorUndoTarget::canRedo()
{
    for (unsigned i = 0; i < m_undoCommandList.size(); ++i) {
        if (!m_undoCommandList[i]->isUndo())
            return true;
    }
    return false;
}


void WebEditorUndoTarget::undo()
{
    for (int i = m_undoCommandList.size() - 1; i >= 0; --i) {
        if (m_undoCommandList[i]->isUndo()) {
            m_undoCommandList[i]->execute();
            m_undoCommandList.remove(i);
            return;
        }
    }
}

void WebEditorUndoTarget::redo()
{
    for (int i = m_undoCommandList.size() - 1; i >= 0; --i) {
        if (!m_undoCommandList[i]->isUndo()) {
            m_undoCommandList[i]->execute();
            m_undoCommandList.remove(i);
            return;
        }
    }
}

WebEditorClient::WebEditorClient(WebView* webView)
    : m_webView(webView)
    , m_undoTarget(0)
{
    m_undoTarget = new WebEditorUndoTarget();
}

WebEditorClient::~WebEditorClient()
{
    delete m_undoTarget;
}

void WebEditorClient::pageDestroyed()
{
    delete this;
}

bool WebEditorClient::isContinuousSpellCheckingEnabled()
{
    bool enabled = m_webView->isContinuousSpellCheckingEnabled();
    return !!enabled;
}

void WebEditorClient::toggleContinuousSpellChecking()
{
	m_webView->setContinuousSpellCheckingEnabled(isContinuousSpellCheckingEnabled() ? false : true);
}

bool WebEditorClient::isGrammarCheckingEnabled()
{
    bool enabled = m_webView->isGrammarCheckingEnabled();
    return !!enabled;
}

void WebEditorClient::toggleGrammarChecking()
{
	m_webView->setGrammarCheckingEnabled(isGrammarCheckingEnabled() ? false : true);
}

/*static void initViewSpecificSpelling(WebView* view)
{
    // we just use this as a flag to indicate that we've spell checked the document
    // and need to close the spell checker out when the view closes.
    int tag = view->spellCheckerDocumentTag();
}*/

int WebEditorClient::spellCheckerDocumentTag()
{
    // we don't use the concept of spelling tags
    notImplemented();
    return 0;
}

bool WebEditorClient::shouldBeginEditing(Range* range)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMRange* domRange = DOMRange::createInstance(range);
        bool result = editing->shouldBeginEditingInDOMRange(m_webView, domRange);
        delete domRange;
        return result;
    }
    return true;
}

bool WebEditorClient::shouldEndEditing(Range* range)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMRange* domRange = DOMRange::createInstance(range);
        bool result = editing->shouldEndEditingInDOMRange(m_webView, domRange);
        delete domRange;
        return result;
    } 
    return true;
}

void WebEditorClient::didBeginEditing()
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        editing->webViewDidBeginEditing();
    }
}

void WebEditorClient::respondToChangedContents()
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->webViewDidChange();
}

void WebEditorClient::respondToChangedSelection(Frame*)
{
    m_webView->selectionChanged();
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->webViewDidChangeSelection();
}

void WebEditorClient::discardedComposition(Frame*)
{
    notImplemented();
}

void WebEditorClient::didEndEditing()
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        editing->webViewDidEndEditing();
    }
}

void WebEditorClient::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void WebEditorClient::willWriteSelectionToPasteboard(WebCore::Range*)
{
    notImplemented();
}

void WebEditorClient::getClientPasteboardDataForRange(WebCore::Range*, Vector<String>&, Vector<RefPtr<WebCore::SharedBuffer> >&)
{
    notImplemented();
}

void WebEditorClient::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

bool WebEditorClient::shouldDeleteRange(Range* range)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMRange* domRange = DOMRange::createInstance(range);
        bool result = editing->shouldDeleteDOMRange(m_webView, domRange);
        delete domRange;
        return result;
    }
    return true; 
}

bool WebEditorClient::shouldInsertNode(WebCore::Node* node, Range* replacingRange, EditorInsertAction givenAction)
{ 
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMRange* domRange = DOMRange::createInstance(replacingRange);
        DOMNode* domNode = DOMNode::createInstance(node);
        bool result = editing->shouldInsertNode(m_webView, domNode, domRange, (WebViewInsertAction)givenAction);
        delete domNode;
        delete domRange;
        return result;
    }
    return true; 
}

bool WebEditorClient::shouldInsertText(const String& str, Range* replacingRange, EditorInsertAction givenAction)
{     
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMRange* domRange = DOMRange::createInstance(replacingRange);
        bool result = editing->shouldInsertText(m_webView, strdup(str.utf8().data()), domRange, (WebViewInsertAction)givenAction);
        delete domRange;
        return result;
    }
    return true; 
}

bool WebEditorClient::isSelectTrailingWhitespaceEnabled(void)
{
    return m_webView->isSelectTrailingWhitespaceEnabled();
}


bool WebEditorClient::shouldApplyStyle(StyleProperties* style, Range* toElementsInDOMRange)
{
  /*
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMRange* domRange = DOMRange::createInstance(toElementsInDOMRange);
        DOMCSSStyleDeclaration* css = DOMCSSStyleDeclaration::createInstance(style);
        bool result = editing->shouldApplyStyle(m_webView, css, domRange);
        delete domRange;
        delete css;
        return result;
	}*/
    return true;
}

void WebEditorClient::didApplyStyle()
{
    notImplemented();
}

bool WebEditorClient::shouldMoveRangeAfterDelete(Range* /*range*/, Range* /*rangeToBeReplaced*/)
{ 
    notImplemented(); return true; 
}

bool WebEditorClient::shouldChangeTypingStyle(StyleProperties* currentStyle, StyleProperties* toProposedStyle)
{
  /*
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMCSSStyleDeclaration* current = DOMCSSStyleDeclaration::createInstance(currentStyle);
        DOMCSSStyleDeclaration* proposed = DOMCSSStyleDeclaration::createInstance(toProposedStyle);
        bool result = editing->shouldChangeTypingStyle(m_webView, current, proposed);
        delete current;
        delete proposed;
        return result;
	}*/
    return false; 
}

void WebEditorClient::webViewDidChangeTypingStyle(WebNotification* /*notification*/)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->webViewDidChangeTypingStyle();
}

void WebEditorClient::webViewDidChangeSelection(WebNotification* /*notification*/)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->webViewDidChangeSelection();
}

bool WebEditorClient::shouldShowDeleteInterface(HTMLElement* element)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMHTMLElement* elem = DOMHTMLElement::createInstance(element);
        return editing->shouldShowDeleteInterface(elem);
    }
    return false;
}

bool WebEditorClient::smartInsertDeleteEnabled(void)
{
    return m_webView->smartInsertDeleteEnabled();
}

bool WebEditorClient::shouldChangeSelectedRange(WebCore::Range* current, WebCore::Range* proposed, WebCore::EAffinity selection, bool stillSelected)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        DOMRange* currentRange = DOMRange::createInstance(current);
        DOMRange* proposedRange = DOMRange::createInstance(proposed);
        bool result = editing->shouldChangeSelectedDOMRange(m_webView, currentRange, proposedRange, (WebSelectionAffinity)selection, stillSelected);
        delete currentRange;
        delete proposedRange;
        return result;
    } 
    return true; 
}

void WebEditorClient::textFieldDidBeginEditing(Element* e)
{
    /*IWebFormDelegate* formDelegate;
    if (SUCCEEDED(m_webView->formDelegate(&formDelegate)) && formDelegate) {
        IDOMElement* domElement = DOMElement::createInstance(e);
        if (domElement) {
            IDOMHTMLInputElement* domInputElement;
            if (SUCCEEDED(domElement->QueryInterface(IID_IDOMHTMLInputElement, (void**)&domInputElement))) {
                formDelegate->textFieldDidBeginEditing(domInputElement, kit(e->document()->frame()));
                domInputElement->Release();
            }
            domElement->Release();
        }
        formDelegate->Release();
    }*/
}

void WebEditorClient::textFieldDidEndEditing(Element* element)
{
	DoMethod(m_webView->viewWindow()->browser, MM_OWBBrowser_Autofill_HidePopup);
}

void WebEditorClient::textDidChangeInTextField(Element* element)
{
    if (HTMLInputElement* inputElement = element->toInputElement())
	{
		DoMethod(m_webView->viewWindow()->browser, MM_OWBBrowser_Autofill_DidChangeInTextField, inputElement);
	}
}

bool WebEditorClient::doTextFieldCommandFromEvent(Element* e, KeyboardEvent* ke)
{
	// XXX: any better place to catch the event?
	return DoMethod(m_webView->viewWindow()->browser, MM_OWBBrowser_Autofill_HandleNavigationEvent, ke);
}

void WebEditorClient::textWillBeDeletedInTextField(Element* e)
{
    // We're using the deleteBackward command for all deletion operations since the autofill code treats all deletions the same way.
    /*IWebFormDelegate* formDelegate;
    if (SUCCEEDED(m_webView->formDelegate(&formDelegate)) && formDelegate) {
        IDOMElement* domElement = DOMElement::createInstance(e);
        if (domElement) {
            IDOMHTMLInputElement* domInputElement;
            if (SUCCEEDED(domElement->QueryInterface(IID_IDOMHTMLInputElement, (void**)&domInputElement))) {
                BOOL result;
                formDelegate->doPlatformCommand(domInputElement, BString(L"DeleteBackward"), kit(e->document()->frame()), &result);
                domInputElement->Release();
            }
            domElement->Release();
        }
        formDelegate->Release();
    }*/
}

void WebEditorClient::textDidChangeInTextArea(Element* e)
{
    /*IWebFormDelegate* formDelegate;
    if (SUCCEEDED(m_webView->formDelegate(&formDelegate)) && formDelegate) {
        IDOMElement* domElement = DOMElement::createInstance(e);
        if (domElement) {
            IDOMHTMLTextAreaElement* domTextAreaElement;
            if (SUCCEEDED(domElement->QueryInterface(IID_IDOMHTMLTextAreaElement, (void**)&domTextAreaElement))) {
                formDelegate->textDidChangeInTextArea(domTextAreaElement, kit(e->document()->frame()));
                domTextAreaElement->Release();
            }
            domElement->Release();
        }
        formDelegate->Release();
    }*/
}



static String undoNameForEditAction(EditAction editAction)
{
    switch (editAction) {
        case EditActionUnspecified: return "";
        case EditActionSetColor: return "Set Color";
        case EditActionSetBackgroundColor: return "Set Background Color";
        case EditActionTurnOffKerning: return "Turn Off Kerning";
        case EditActionTightenKerning: return "Tighten Kerning";
        case EditActionLoosenKerning: return "Loosen Kerning";
        case EditActionUseStandardKerning: return "Use Standard Kerning";
        case EditActionTurnOffLigatures: return "Turn Off Ligatures";
        case EditActionUseStandardLigatures: return "Use Standard Ligatures";
        case EditActionUseAllLigatures: return "Use All Ligatures";
        case EditActionRaiseBaseline: return "Raise Baseline";
        case EditActionLowerBaseline: return "Lower Baseline";
        case EditActionSetTraditionalCharacterShape: return "Set Traditional Character Shape";
        case EditActionSetFont: return "Set Font";
        case EditActionChangeAttributes: return "Change Attributes";
        case EditActionAlignLeft: return "Align Left";
        case EditActionAlignRight: return "Align Right";
        case EditActionCenter: return "Center";
        case EditActionJustify: return "Justify";
        case EditActionSetWritingDirection: return "Set Writing Direction";
        case EditActionSubscript: return "Subscript";
        case EditActionSuperscript: return "Superscript";
        case EditActionUnderline: return "Underline";
        case EditActionOutline: return "Outline";
        case EditActionUnscript: return "Unscript";
        case EditActionDrag: return "Drag";
        case EditActionCut: return "Cut";
        case EditActionPaste: return "Paste";
        case EditActionPasteFont: return "Paste Font";
        case EditActionPasteRuler: return "Paste Ruler";
        case EditActionTyping: return "Typing";
        case EditActionCreateLink: return "Create Link";
        case EditActionUnlink: return "Unlink";
        case EditActionInsertList: return "Insert List";
        case EditActionFormatBlock: return "Formatting";
        case EditActionIndent: return "Indent";
        case EditActionOutdent: return "Outdent";
        case EditActionBold: return "Bold";
        case EditActionItalics: return "Italics";
        case EditActionDictation: return "Dictation";
        case EditActionDelete: return "Delete";
        case EditActionInsert: return "Insert";
    }
    return String();
}

void WebEditorClient::registerUndoStep(PassRefPtr<UndoStep> step)
{
    String actionName = undoNameForEditAction(step->editingAction());
    WebEditorUndoCommand* undoCommand = new WebEditorUndoCommand(step, true);
    if (!undoCommand)
        return;
    m_undoTarget->append(actionName, undoCommand);
}

void WebEditorClient::registerRedoStep(PassRefPtr<UndoStep> step)
{
    String actionName = undoNameForEditAction(step->editingAction());
    WebEditorUndoCommand* undoCommand = new WebEditorUndoCommand(step, false);
    if (!undoCommand)
        return;
    m_undoTarget->append(actionName, undoCommand);
}

void WebEditorClient::clearUndoRedoOperations()
{
    m_undoTarget->clear();
}

bool WebEditorClient::canUndo() const
{
    return m_undoTarget->canUndo();
}

bool WebEditorClient::canRedo() const
{
    return m_undoTarget->canRedo();
}

void WebEditorClient::undo()
{   
    m_undoTarget->undo();
}

void WebEditorClient::redo()
{
    m_undoTarget->redo();
}

void WebEditorClient::handleKeyboardEvent(KeyboardEvent* evt)
{
    if (m_webView->handleEditingKeyboardEvent(evt))
        evt->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent* )
{
}

bool WebEditorClient::isEditable()
{
    return false;
}

bool WebEditorClient::shouldEraseMarkersAfterChangeSelection(TextCheckingType) const
{
    return true;
}

void WebEditorClient::ignoreWordInSpellDocument(const String& word)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->ignoreWordInSpellDocument(m_webView, strdup(word.utf8().data()));
}

void WebEditorClient::learnWord(const String& word)
{
	/*
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->learnWord(strdup(word.utf8().data()));
	*/

#if OS(MORPHOS)
	D(kprintf("learnWord(%s)\n", word.utf8().data()));

	String wordToLearn = word.lower();
	STRPTR cword;

	if(wordToLearn.length() <= 1)
		return;

	cword = utf8_to_local(wordToLearn.utf8().data());

	if(cword)
	{
		APTR dictionary = get_dictionary();
		if(!dictionary)
		{
			open_dictionary(NULL);
		}

		if(dictionary)
		{
			D(kprintf("Learning <%s>\n", cword));

			if(dictionary_can_learn())
			{
				Learn(dictionary, (STRPTR) cword);
			}
		}

		free(cword);
	}
#endif
}

void WebEditorClient::checkSpellingOfString(const UChar* text, int length, int* misspellingLocation, int* misspellingLength)
{
	/*
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    *misspellingLocation = -1;
    *misspellingLength = 0;
    if (editing)
        editing->checkSpellingOfString(m_webView, strdup(String(text, length).utf8().data()), length, misspellingLocation, misspellingLength);
	*/

	D(kprintf("checkSpellingOfString(%s)\n", String(text, length).utf8().data()));

	int start = 0, len = 0, i = 0;
	bool wordReached = false;
	String word;
	STRPTR cword;
	UChar *tmp = (UChar *) text;

	*misspellingLocation = -1;
	*misspellingLength = 0;

	if(length <= 1)
		return;

	while(i < length)
	{
		if(!wordReached && !u_isalnum(*tmp))
		{
			start++;
		}
		else if(u_isalnum(*tmp))
		{
			wordReached = true;
			len++;
		}
		else if(wordReached && !u_isalnum(*tmp))
		{
			break;
		}

		i++;
		tmp++;
	}

	if(start > 0 || len > 0)
	{
		word = String(text + start, len);
	}
	else
	{
		word = String(text, length);
	}

	word = word.lower();

	if(word.length() <= 1)
		return;

	cword = utf8_to_local(word.utf8().data());

	if(cword)
	{
		APTR dictionary = get_dictionary();
		if(!dictionary)
		{	
			open_dictionary(NULL);
		}

		if(dictionary)
		{
#if !OS(AROS)
			D(kprintf("Checking <%s>\n", cword));

			STRPTR *res = (STRPTR *) Suggest(dictionary, (STRPTR) cword, NULL);

			if(res != (STRPTR *) -1)
			{
				*misspellingLocation = start;
				*misspellingLength = len;
			}
#endif
		}

		free(cword);
	}
}

void WebEditorClient::checkGrammarOfString(const UChar* text, int length, Vector<GrammarDetail>& details, int* badGrammarLocation, int* badGrammarLength)
{
    /*details.clear();
    *badGrammarLocation = -1;
    *badGrammarLength = 0;

    COMPtr<IWebEditingDelegate> ed;
    if (FAILED(m_webView->editingDelegate(&ed)) || !ed.get())
        return;

    initViewSpecificSpelling(m_webView);
    COMPtr<IEnumWebGrammarDetails> enumDetailsObj;
    if (FAILED(ed->checkGrammarOfString(m_webView, text, length, &enumDetailsObj, badGrammarLocation, badGrammarLength)))
        return;

    while (true) {
        ULONG fetched;
        COMPtr<IWebGrammarDetail> detailObj;
        if (enumDetailsObj->Next(1, &detailObj, &fetched) != S_OK)
            break;

        GrammarDetail detail;
        if (FAILED(detailObj->length(&detail.length)))
            continue;
        if (FAILED(detailObj->location(&detail.location)))
            continue;
        BSTR userDesc;
        if (FAILED(detailObj->userDescription(&userDesc)))
            continue;
        detail.userDescription = String(userDesc, SysStringLen(userDesc));
        SysFreeString(userDesc);

        COMPtr<IEnumSpellingGuesses> enumGuessesObj;
        if (FAILED(detailObj->guesses(&enumGuessesObj)))
            continue;
        while (true) {
            BSTR guess;
            if (enumGuessesObj->Next(1, &guess, &fetched) != S_OK)
                break;
            detail.guesses.append(String(guess, SysStringLen(guess)));
            SysFreeString(guess);
        }

        details.append(detail);
    }*/

	//kprintf("checkGrammarOfString(%s)\n", String(text, length).utf8().data());
	*badGrammarLocation = -1;
	*badGrammarLength = 0;
}

String WebEditorClient::getAutoCorrectSuggestionForMisspelledWord(const String& inputWord)
{
    // This method can be implemented using customized algorithms for the particular browser.
    // Currently, it computes an empty string.
    return String();
}


void WebEditorClient::updateSpellingUIWithGrammarString(const String& string, const WebCore::GrammarDetail& detail)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing) {
        char** guessesTab = (char**)fastMalloc(detail.guesses.size() * sizeof(char*));
        for (unsigned i = 0; i < detail.guesses.size(); i++)
            guessesTab[i] = strdup(detail.guesses[i].utf8().data());
        
        editing->updateSpellingUIWithGrammarString(strdup(string.utf8().data()), detail.location, detail.length, strdup(detail.userDescription.utf8().data()), guessesTab, (int)detail.guesses.size());
        for (unsigned i = 0; i < detail.guesses.size(); i++)
            fastFree(guessesTab[i]);
    }
}

void WebEditorClient::updateSpellingUIWithMisspelledWord(const String& word)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->updateSpellingUIWithMisspelledWord(strdup(word.utf8().data()));
}

void WebEditorClient::showSpellingUI(bool show)
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        editing->showSpellingUI(show);
}

bool WebEditorClient::spellingUIIsShowing()
{
    SharedPtr<WebEditingDelegate> editing = m_webView->webEditingDelegate();
    if (editing)
        return editing->spellingUIIsShowing();
    return false;
}

void WebEditorClient::getGuessesForWord(const String& word, const String& context, Vector<String>& guesses)
{
	D(kprintf("getGuessesForWord(%s)\n", word.utf8().data()));

	String isolatedWord;
	auto upconvertedCharacters = StringView(word).upconvertedCharacters();
	const UChar *text = upconvertedCharacters;
	const UChar *tmp = text;
	STRPTR cword;
	bool wordReached = false;
	int start = 0, len = 0;
	unsigned i = 0;

	guesses.clear();

	while(i < word.length())
	{
		if(!wordReached && !u_isalnum(*tmp))
		{
			start++;
		}
		else if(u_isalnum(*tmp))
		{
			wordReached = true;
			len++;
		}
		else if(wordReached && !u_isalnum(*tmp))
		{
			break;
		}

		i++;
		tmp++;
	}

	if(start > 0 || len > 0)
	{
		isolatedWord = String(text + start, len);
	}
	else
	{
		isolatedWord = word;
	}

	isolatedWord = isolatedWord.lower();

	cword = utf8_to_local(isolatedWord.utf8().data());

	if(cword)
	{
		APTR dictionary = get_dictionary();
		if(!dictionary)
		{
			open_dictionary(NULL);
		}

		if(dictionary)
		{
#if !OS(AROS)
			D(kprintf("Checking <%s>\n", cword));

			STRPTR *res = (STRPTR *) Suggest(dictionary, (STRPTR) cword, NULL);

			if(res && res != (STRPTR *) -1)
			{
				while(*res)
				{
					D(kprintf("Suggested: <%s>\n", *res));
					STRPTR suggestedWord = local_to_utf8(*res);
					if(suggestedWord && suggestedWord[0] != '\0')
					{
						guesses.append(String::fromUTF8(suggestedWord));
						free(suggestedWord);
					}

					res++;
				}
			}
#endif
		}

		free(cword);
	}

    /*guesses.clear();

    COMPtr<IWebEditingDelegate> ed;
    if (FAILED(m_webView->editingDelegate(&ed)) || !ed.get())
        return;

    COMPtr<IEnumSpellingGuesses> enumGuessesObj;
    if (FAILED(ed->guessesForWord(BString(word), &enumGuessesObj)))
        return;

    while (true) {
        ULONG fetched;
        BSTR guess;
        if (enumGuessesObj->Next(1, &guess, &fetched) != S_OK)
            break;
        guesses.append(String(guess, SysStringLen(guess)));
        SysFreeString(guess);
    }*/
}

void WebEditorClient::willSetInputMethodState()
{
}

void WebEditorClient::setInputMethodState(bool enabled)
{
    m_webView->setInputMethodState(enabled);
}

void WebEditorClient::didChangeSelectionAndUpdateLayout()
{
}
