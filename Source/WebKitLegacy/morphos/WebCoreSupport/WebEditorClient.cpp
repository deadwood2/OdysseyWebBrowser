/*
 * Copyright (C) 2006-2017 Apple Inc.  All rights reserved.
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

#include "WebEditorClient.h"
#include "WebPage.h"
#include <WebCore/Document.h>
#include <WebCore/HTMLElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/LocalizedStrings.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/Frame.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/Range.h>
#include <WebCore/Settings.h>
#include <WebCore/UndoStep.h>
#include <WebCore/UserTypingGestureIndicator.h>
#include <WebCore/VisibleSelection.h>
#include <WebCore/FrameSelection.h>
#include <WebCore/AutofillElements.h>
#include <WebCore/Editor.h>
#include <WebCore/UndoStep.h>
#include <wtf/text/StringView.h>
#include <proto/exec.h>
#include <proto/spellchecker.h>
#include <libraries/spellchecker.h>
#include <unicode/ubrk.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"

using namespace WebCore;
using namespace HTMLNames;

// WebEditorClient ------------------------------------------------------------------

namespace WebKit {

struct Library *WebEditorClient::m_spellcheckerLibrary;
APTR WebEditorClient::m_globalSpellDictionary;
WTF::String WebEditorClient::m_globalLanguage;
WTF::HashSet<WebEditorClient*> WebEditorClient::m_editors;

struct WebEditorClientCleanup
{
	~WebEditorClientCleanup() {
		if (WebEditorClient::m_globalSpellDictionary)
			CloseDictionary(WebEditorClient::m_globalSpellDictionary);
		WebEditorClient::m_globalSpellDictionary = nullptr;
		if (WebEditorClient::m_spellcheckerLibrary)
			CloseLibrary(WebEditorClient::m_spellcheckerLibrary);
		WebEditorClient::m_spellcheckerLibrary = nullptr;
	}
};
static WebEditorClientCleanup __weCleanup;

WebEditorClient::WebEditorClient(WebPage* webPage)
    : m_webPage(webPage)
{
	m_editors.add(this);
}

WebEditorClient::~WebEditorClient()
{
	m_editors.remove(this);
}

void WebEditorClient::setSpellCheckingEnabled(bool enabled)
{
	if (!m_spellcheckerLibrary)
	{
		m_spellcheckerLibrary = OpenLibrary("spellchecker.library", 0);
	}

	struct Library *SpellCheckerBase = m_spellcheckerLibrary;
	if (SpellCheckerBase)
	{
		bool check = false;
		
		if (!enabled && m_globalSpellDictionary)
		{
			CloseDictionary(m_globalSpellDictionary);
			m_globalSpellDictionary = nullptr;
			check = true;
		}

		if (enabled && !m_globalSpellDictionary)
		{
			auto ulanguage = m_globalLanguage.utf8();
			struct TagItem dicttags[] = {{ SCA_UTF8, TRUE}, { m_globalLanguage.length() ? SCA_Name : TAG_IGNORE, (IPTR)ulanguage.data()}, {SCA_WaitOpen, TRUE}, {TAG_DONE, 0} };
			m_globalSpellDictionary = OpenDictionary(nullptr, dicttags);
			check = true;
		}

		if (check)
		{
			for (WTF::HashSet<WebEditorClient*>::iterator it = m_editors.begin(); it != m_editors.end(); ++it)
			{
				(*it)->onSpellcheckingLanguageChanged();
			}
		}
	}
}

void WebEditorClient::setSpellCheckingLanguage(const WTF::String &language)
{
	if (!m_spellcheckerLibrary)
	{
		m_spellcheckerLibrary = OpenLibrary("spellchecker.library", 0);
	}

	if (m_globalLanguage != language)
	{
		m_globalLanguage = language;

		if (m_globalSpellDictionary)
		{
			CloseDictionary(m_globalSpellDictionary);
			auto ulanguage = m_globalLanguage.utf8();
			struct TagItem dicttags[] = {{ SCA_UTF8, TRUE}, { m_globalLanguage.length() ? SCA_Name : TAG_IGNORE, (IPTR)ulanguage.data()}, {SCA_WaitOpen, TRUE}, {TAG_DONE, 0} };
			m_globalSpellDictionary = OpenDictionary(nullptr, dicttags);
		}

		for (WTF::HashSet<WebEditorClient*>::iterator it = m_editors.begin(); it != m_editors.end(); ++it)
			(*it)->onSpellcheckingLanguageChanged();
	}
}

void WebEditorClient::getGuessesForWord(const WTF::String &word, WTF::Vector<WTF::String> &outGuesses)
{
    outGuesses.clear();

	struct Library *SpellCheckerBase = m_spellcheckerLibrary;
	(void)SpellCheckerBase;

	if (m_globalSpellDictionary)
	{
		// NOTE: utf8() returns a temporary object, so this is fine inside a function call, but not fine inside a taglist
		STRPTR *suggestions = Suggest(m_globalSpellDictionary, word.utf8().data(), NULL);
		if (suggestions)
		{
			int i = 0;
			while (suggestions[i])
			{
				outGuesses.append(WTF::String::fromUTF8(suggestions[i]));
				i++;
			}
		}
	}

	if (m_spellDictionary)
	{
		// NOTE: utf8() returns a temporary object, so this is fine inside a function call, but not fine inside a taglist
		STRPTR *suggestions = Suggest(m_spellDictionary, word.utf8().data(), NULL);
		if (suggestions)
		{
			int i = 0;
			while (suggestions[i])
			{
				outGuesses.append(WTF::String::fromUTF8(suggestions[i]));
				i++;
			}
		}
	}
	
	if (m_additionalSpellDictionary)
	{
		// NOTE: utf8() returns a temporary object, so this is fine inside a function call, but not fine inside a taglist
		STRPTR *suggestions = Suggest(m_additionalSpellDictionary, word.utf8().data(), NULL);
		if (suggestions)
		{
			int i = 0;
			while (suggestions[i])
			{
				outGuesses.append(WTF::String::fromUTF8(suggestions[i]));
				i++;
			}
		}
	}
}

void WebEditorClient::setSpellCheckingLanguages(const WTF::String &language, const WTF::String &languageAdditional)
{
	if (!m_spellcheckerLibrary)
	{
		m_spellcheckerLibrary = OpenLibrary("spellchecker.library", 0);
	}

	bool doChanged = false;

	if (m_language != language)
	{
		struct Library *SpellCheckerBase = m_spellcheckerLibrary;
		(void)SpellCheckerBase;
	
		m_language = language;
		if (m_spellDictionary)
			CloseDictionary(m_spellDictionary);
		m_spellDictionary = nullptr;
		if (m_language.length())
		{
			auto ulanguage = m_language.utf8();
			struct TagItem dicttags[] = {{ SCA_UTF8, TRUE}, { ulanguage.length() ? SCA_Name : TAG_IGNORE, (IPTR)ulanguage.data()}, {SCA_WaitOpen, TRUE}, {TAG_DONE, 0} };
			m_spellDictionary = OpenDictionary(nullptr, dicttags);
		}

		doChanged = true;
	}

	if (m_additionalLanguage != languageAdditional)
	{
		struct Library *SpellCheckerBase = m_spellcheckerLibrary;
		(void)SpellCheckerBase;
	
		m_additionalLanguage = languageAdditional;
		if (m_additionalSpellDictionary)
			CloseDictionary(m_additionalSpellDictionary);
		m_additionalSpellDictionary = nullptr;
		if (m_additionalLanguage.length())
		{
			auto ulanguage = m_additionalLanguage.utf8();
			struct TagItem dicttags[] = {{ SCA_UTF8, TRUE}, { ulanguage.length() ? SCA_Name : TAG_IGNORE, (IPTR)ulanguage.data()}, {SCA_WaitOpen, TRUE}, {TAG_DONE, 0} };
			m_additionalSpellDictionary = OpenDictionary(nullptr, dicttags);
		}

		doChanged = true;
	}
	
	if (doChanged)
		onSpellcheckingLanguageChanged();
}

void WebEditorClient::onSpellcheckingLanguageChanged()
{
	Document *document = m_webPage->mainFrame()->document();
	if (document)
	{
		document->editor().checkEntireDocument();
	}
}


void WebEditorClient::getAvailableDictionaries(WTF::Vector<WTF::String> &outDictionaries, WTF::String &outDefault)
{
	if (!m_spellcheckerLibrary)
	{
		m_spellcheckerLibrary = OpenLibrary("spellchecker.library", 0);
	}
	struct Library *SpellCheckerBase = m_spellcheckerLibrary;
	if (SpellCheckerBase)
	{
		char buffer[4096];
		STRPTR defaultLanguage = NULL;
		struct TagItem tags[] = {{SCA_UTF8, TRUE}, {SCA_Default, (IPTR)&defaultLanguage}, {TAG_DONE,0}};
		STRPTR *l = (STRPTR *)ListDictionaries(buffer, sizeof(buffer), tags);
		if (l)
		{
			if (defaultLanguage && *defaultLanguage)
				outDefault = WTF::String::fromUTF8(defaultLanguage);
			for (int i = 0; l[i]; i++)
			{
				outDictionaries.constructAndAppend(WTF::String::fromUTF8(l[i]));
			}
		}
	}
}

void WebEditorClient::replaceMisspelledWord(const WTF::String& replacement)
{

}
	
bool WebEditorClient::isContinuousSpellCheckingEnabled()
{
	return m_globalSpellDictionary != nullptr || m_spellDictionary != nullptr || m_additionalSpellDictionary != nullptr;
}

void WebEditorClient::toggleContinuousSpellChecking()
{
	// ignore, we don't use this in our flows...
}

bool WebEditorClient::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

void WebEditorClient::toggleGrammarChecking()
{
    notImplemented();
}

int WebEditorClient::spellCheckerDocumentTag()
{
    // we don't use the concept of spelling tags
    notImplemented();
    ASSERT_NOT_REACHED();
    return 0;
}

bool WebEditorClient::shouldBeginEditing(const WebCore::SimpleRange& range)
{
    return true;
}

bool WebEditorClient::shouldEndEditing(const WebCore::SimpleRange& range)
{
    notImplemented();
    return true;
}

void WebEditorClient::didBeginEditing()
{
    notImplemented();
}

void WebEditorClient::respondToChangedContents()
{
    notImplemented();
}

void WebEditorClient::respondToChangedSelection(Frame*)
{
//    m_webPage->selectionChanged();
    notImplemented();
}

void WebEditorClient::discardedComposition(Frame*)
{
    notImplemented();
}

void WebEditorClient::canceledComposition()
{
    notImplemented();
}

void WebEditorClient::didEndEditing()
{
    notImplemented();
}

void WebEditorClient::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void WebEditorClient::willWriteSelectionToPasteboard(const std::optional<WebCore::SimpleRange>&)
{
    notImplemented();
}

void WebEditorClient::getClientPasteboardData(const std::optional<SimpleRange>&, Vector<String>& pasteboardTypes, Vector<RefPtr<SharedBuffer>>& pasteboardData)
{
    notImplemented();
}

bool WebEditorClient::shouldDeleteRange(const std::optional<WebCore::SimpleRange>&)
{
    notImplemented();
    return true;
}

bool WebEditorClient::shouldInsertNode(WebCore::Node&, const std::optional<WebCore::SimpleRange>&, WebCore::EditorInsertAction)
{ 
    notImplemented();
    return true;
}

bool WebEditorClient::shouldInsertText(const WTF::String&, const std::optional<WebCore::SimpleRange>&, WebCore::EditorInsertAction)
{
    notImplemented();
    return true;
}

bool WebEditorClient::shouldChangeSelectedRange(const std::optional<WebCore::SimpleRange>& fromRange, const std::optional<WebCore::SimpleRange>& toRange, WebCore::Affinity, bool stillSelectingg)
{
    notImplemented();
    return true;
}

bool WebEditorClient::shouldApplyStyle(const WebCore::StyleProperties &, const std::optional<WebCore::SimpleRange>&)
{
    notImplemented();
    return true;
}

void WebEditorClient::didApplyStyle()
{
    notImplemented();
}

bool WebEditorClient::shouldMoveRangeAfterDelete(const WebCore::SimpleRange&, const WebCore::SimpleRange&)
{
    notImplemented();
    return true;
}

bool WebEditorClient::smartInsertDeleteEnabled(void)
{
    Page* page = m_webPage->corePage();
    if (!page)
        return false;
    return page->settings().smartInsertDeleteEnabled();
}

bool WebEditorClient::isSelectTrailingWhitespaceEnabled(void) const
{
    Page* page = m_webPage->corePage();
    if (!page)
        return false;
    return page->settings().selectTrailingWhitespaceEnabled();
}

void WebEditorClient::textFieldDidBeginEditing(Element* e)
{
//    notImplemented();
	
    if (is<HTMLInputElement>(e) && !m_webPage->hasAutofillElements())
    {
		HTMLInputElement* element = downcast<HTMLInputElement>(e);
		m_webPage->startedEditingElement(element);
	}
}

void WebEditorClient::textFieldDidEndEditing(Element* e)
{
//    notImplemented();
}

void WebEditorClient::textDidChangeInTextField(Element* e)
{
    if (!UserTypingGestureIndicator::processingUserTypingGesture() || UserTypingGestureIndicator::focusedElementAtGestureStart() != e)
        return;
    //notImplemented();
}

bool WebEditorClient::doTextFieldCommandFromEvent(Element* e, KeyboardEvent* ke)
{
    bool result = false;
    notImplemented();
    return result;
}

void WebEditorClient::textWillBeDeletedInTextField(Element* e)
{
    //notImplemented();
}

void WebEditorClient::textDidChangeInTextArea(Element* e)
{
    //notImplemented();
}

static String undoNameForEditAction(EditAction editAction)
{
    switch (editAction) {
    case EditAction::Unspecified:
    case EditAction::InsertReplacement:
        return String();
    case EditAction::SetColor: return WEB_UI_STRING_KEY("Set Color", "Set Color (Undo action name)", "Undo action name");
    case EditAction::SetBackgroundColor: return WEB_UI_STRING_KEY("Set Background Color", "Set Background Color (Undo action name)", "Undo action name");
    case EditAction::TurnOffKerning: return WEB_UI_STRING_KEY("Turn Off Kerning", "Turn Off Kerning (Undo action name)", "Undo action name");
    case EditAction::TightenKerning: return WEB_UI_STRING_KEY("Tighten Kerning", "Tighten Kerning (Undo action name)", "Undo action name");
    case EditAction::LoosenKerning: return WEB_UI_STRING_KEY("Loosen Kerning", "Loosen Kerning (Undo action name)", "Undo action name");
    case EditAction::UseStandardKerning: return WEB_UI_STRING_KEY("Use Standard Kerning", "Use Standard Kerning (Undo action name)", "Undo action name");
    case EditAction::TurnOffLigatures: return WEB_UI_STRING_KEY("Turn Off Ligatures", "Turn Off Ligatures (Undo action name)", "Undo action name");
    case EditAction::UseStandardLigatures: return WEB_UI_STRING_KEY("Use Standard Ligatures", "Use Standard Ligatures (Undo action name)", "Undo action name");
    case EditAction::UseAllLigatures: return WEB_UI_STRING_KEY("Use All Ligatures", "Use All Ligatures (Undo action name)", "Undo action name");
    case EditAction::RaiseBaseline: return WEB_UI_STRING_KEY("Raise Baseline", "Raise Baseline (Undo action name)", "Undo action name");
    case EditAction::LowerBaseline: return WEB_UI_STRING_KEY("Lower Baseline", "Lower Baseline (Undo action name)", "Undo action name");
    case EditAction::SetTraditionalCharacterShape: return WEB_UI_STRING_KEY("Set Traditional Character Shape", "Set Traditional Character Shape (Undo action name)", "Undo action name");
    case EditAction::SetFont: return WEB_UI_STRING_KEY("Set Font", "Set Font (Undo action name)", "Undo action name");
    case EditAction::ChangeAttributes: return WEB_UI_STRING_KEY("Change Attributes", "Change Attributes (Undo action name)", "Undo action name");
    case EditAction::AlignLeft: return WEB_UI_STRING_KEY("Align Left", "Align Left (Undo action name)", "Undo action name");
    case EditAction::AlignRight: return WEB_UI_STRING_KEY("Align Right", "Align Right (Undo action name)", "Undo action name");
    case EditAction::Center: return WEB_UI_STRING_KEY("Center", "Center (Undo action name)", "Undo action name");
    case EditAction::Justify: return WEB_UI_STRING_KEY("Justify", "Justify (Undo action name)", "Undo action name");
    case EditAction::SetInlineWritingDirection:
    case EditAction::SetBlockWritingDirection:
        return WEB_UI_STRING_KEY("Set Writing Direction", "Set Writing Direction (Undo action name)", "Undo action name");
    case EditAction::Subscript: return WEB_UI_STRING_KEY("Subscript", "Subscript (Undo action name)", "Undo action name");
    case EditAction::Superscript: return WEB_UI_STRING_KEY("Superscript", "Superscript (Undo action name)", "Undo action name");
    case EditAction::Bold: return WEB_UI_STRING_KEY("Bold", "Bold (Undo action name)", "Undo action name");
    case EditAction::Italics: return WEB_UI_STRING_KEY("Italics", "Italics (Undo action name)", "Undo action name");
    case EditAction::Underline: return WEB_UI_STRING_KEY("Underline", "Underline (Undo action name)", "Undo action name");
    case EditAction::Outline: return WEB_UI_STRING_KEY("Outline", "Outline (Undo action name)", "Undo action name");
    case EditAction::Unscript: return WEB_UI_STRING_KEY("Unscript", "Unscript (Undo action name)", "Undo action name");
    case EditAction::DeleteByDrag: return WEB_UI_STRING_KEY("Drag", "Drag (Undo action name)", "Undo action name");
    case EditAction::Cut: return WEB_UI_STRING_KEY("Cut", "Cut (Undo action name)", "Undo action name");
    case EditAction::Paste: return WEB_UI_STRING_KEY("Paste", "Paste (Undo action name)", "Undo action name");
    case EditAction::PasteFont: return WEB_UI_STRING_KEY("Paste Font", "Paste Font (Undo action name)", "Undo action name");
    case EditAction::PasteRuler: return WEB_UI_STRING_KEY("Paste Ruler", "Paste Ruler (Undo action name)", "Undo action name");
    case EditAction::TypingDeleteSelection:
    case EditAction::TypingDeleteBackward:
    case EditAction::TypingDeleteForward:
    case EditAction::TypingDeleteWordBackward:
    case EditAction::TypingDeleteWordForward:
    case EditAction::TypingDeleteLineBackward:
    case EditAction::TypingDeleteLineForward:
    case EditAction::TypingInsertText:
    case EditAction::TypingInsertLineBreak:
    case EditAction::TypingInsertParagraph:
        return WEB_UI_STRING_KEY("Typing", "Typing (Undo action name)", "Undo action name");
    case EditAction::CreateLink: return WEB_UI_STRING_KEY("Create Link", "Create Link (Undo action name)", "Undo action name");
    case EditAction::Unlink: return WEB_UI_STRING_KEY("Unlink", "Unlink (Undo action name)", "Undo action name");
    case EditAction::InsertUnorderedList:
    case EditAction::InsertOrderedList:
        return WEB_UI_STRING_KEY("Insert List", "Insert List (Undo action name)", "Undo action name");
    case EditAction::FormatBlock: return WEB_UI_STRING_KEY("Formatting", "Format Block (Undo action name)", "Undo action name");
    case EditAction::Indent: return WEB_UI_STRING_KEY("Indent", "Indent (Undo action name)", "Undo action name");
    case EditAction::Outdent: return WEB_UI_STRING_KEY("Outdent", "Outdent (Undo action name)", "Undo action name");
    default: return String();
    }
}

void WebEditorClient::registerUndoStep(UndoStep& step)
{
	m_undo.append(WebEditorUndoStep::create(step));
	if (m_webPage && m_webPage->_fUndoRedoChanged)
		m_webPage->_fUndoRedoChanged();
}

void WebEditorClient::registerRedoStep(UndoStep& step)
{
	m_redo.append(WebEditorUndoStep::create(step));
	if (m_webPage && m_webPage->_fUndoRedoChanged)
		m_webPage->_fUndoRedoChanged();
}

void WebEditorClient::clearUndoRedoOperations()
{
	m_undo.clear();
	m_redo.clear();
	if (m_webPage && m_webPage->_fUndoRedoChanged)
		m_webPage->_fUndoRedoChanged();
}

bool WebEditorClient::canCopyCut(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canPaste(Frame*, bool defaultValue) const
{
    return defaultValue;
}

bool WebEditorClient::canUndo() const
{
	return !m_undo.isEmpty();
}

bool WebEditorClient::canRedo() const
{
	return !m_redo.isEmpty();
}

void WebEditorClient::undo()
{
	if (!m_undo.isEmpty())
	{
		auto last = m_undo.last();
		m_undo.removeLast();
		last->unapply();
		if (m_webPage && m_webPage->_fUndoRedoChanged)
			m_webPage->_fUndoRedoChanged();
	}
}

void WebEditorClient::redo()
{
	if (!m_redo.isEmpty())
	{
		auto last = m_redo.last();
		m_redo.removeLast();
		last->reapply();
		if (m_webPage && m_webPage->_fUndoRedoChanged)
			m_webPage->_fUndoRedoChanged();
	}
}

void WebEditorClient::handleKeyboardEvent(KeyboardEvent& event)
{
    if (m_webPage->handleEditingKeyboardEvent(event))
        event.setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent&)
{
}

bool WebEditorClient::shouldEraseMarkersAfterChangeSelection(TextCheckingType) const
{
    return false;
}

void WebEditorClient::ignoreWordInSpellDocument(const String& word)
{
	m_ignoredWords.add(word);
}

void WebEditorClient::learnWord(const String& word)
{
	if (m_globalSpellDictionary || m_spellDictionary)
	{
		struct Library *SpellCheckerBase = m_spellcheckerLibrary;
		(void)SpellCheckerBase;
		auto uword = word.utf8();
		if (m_spellDictionary)
			Learn(m_spellDictionary, uword.data());
		else
			Learn(m_globalSpellDictionary, uword.data());
	}
}

void WebEditorClient::checkSpellingOfString(StringView text, int* misspellingLocation, int* misspellingLength)
{
    *misspellingLocation = -1;
    *misspellingLength = 0;
	auto string = text.toStringWithoutCopying();
	auto len = string.length();
	unsigned start = len;
	unsigned end = 0;
	unsigned i = 0;

	for (;;)
	{
		for (; i < len; i++)
		{
			auto ch = string[i];
			
			if (ch == '\n' || ch == '.' || ch == '\r' || ch == '\t')
				continue;
			
			if (u_isalnum(ch))
			{
				start = i;
				end = i;
				break;
			}
		}
		
		if (start >= len)
		{
			return;
		}

		for (; i < len; i++)
		{
			auto ch = string[i];

			if (ch == '\n' || ch == '.' || ch == '\r' || ch == '\t')
				break;

			if (ch != '\'' && !u_isalnum(ch))
			{
				break;
			}
			end = i;
		}
		
		if (start <= end)
		{
			struct Library *SpellCheckerBase = m_spellcheckerLibrary;
			(void)SpellCheckerBase;

			auto word = string.substring(start, end - start + 1);
			auto uword = word.utf8();

			bool found = false;
#if 0
dprintf("spellingof '%s' global %d %s %d %s %d\n", uword.data(), SpellCheck(m_globalSpellDictionary, uword.data(), NULL),
m_language.utf8().data(), m_spellDictionary?SpellCheck(m_spellDictionary, uword.data(), NULL):-1,
m_additionalLanguage.utf8().data(), m_additionalSpellDictionary ? SpellCheck(m_additionalSpellDictionary, uword.data(), NULL):-1);
#endif
			if (m_globalSpellDictionary)
				found = SpellCheck(m_globalSpellDictionary, uword.data(), NULL);

			if (!found && m_spellDictionary)
				found = SpellCheck(m_spellDictionary, uword.data(), NULL);

			if (!found && m_additionalSpellDictionary)
				found = SpellCheck(m_additionalSpellDictionary, uword.data(), NULL);

			if (!found && m_ignoredWords.contains(word))
				found = true;

			// ignore words if all dicts are disabled!
			if (!found && !m_globalSpellDictionary && !m_spellDictionary && !m_additionalSpellDictionary)
				found = true;

			if (!found)
			{
				*misspellingLocation = start;
				*misspellingLength = end - start + 1;
				return;
			}
			
			start = end + 1;
		}
		else
		{
			return;
		}
	}
}

String WebEditorClient::getAutoCorrectSuggestionForMisspelledWord(const String& inputWord)
{
    // This method can be implemented using customized algorithms for the particular browser.
    // Currently, it computes an empty string.
    return String();
}

void WebEditorClient::checkGrammarOfString(StringView text, Vector<GrammarDetail>& details, int* badGrammarLocation, int* badGrammarLength)
{
	notImplemented();
}

void WebEditorClient::updateSpellingUIWithGrammarString(const String& string, const WebCore::GrammarDetail& detail)
{
	notImplemented();
}

void WebEditorClient::updateSpellingUIWithMisspelledWord(const String& word)
{
	notImplemented();
}

void WebEditorClient::showSpellingUI(bool show)
{
	notImplemented();
}

bool WebEditorClient::spellingUIIsShowing()
{
	notImplemented();
	return false;
}

void WebEditorClient::getGuessesForWord(const String& word, const String& context, const VisibleSelection&, Vector<String>& guesses)
{
    guesses.clear();
#if 0
	struct Library *SpellCheckerBase = m_spellcheckerLibrary;
	(void)SpellCheckerBase;

	if (m_globalSpellDictionary)
	{
		STRPTR *suggestions = Suggest(m_globalSpellDictionary, word.utf8().data(), NULL);
		if (suggestions)
		{
			int i = 0;
			while (suggestions[i])
			{
				guesses.append(WTF::String::fromUTF8(suggestions[i]));
				i++;
			}
		}
	}
#endif
}

void WebEditorClient::willSetInputMethodState()
{
}

void WebEditorClient::setInputMethodState(WebCore::Element*)
{
}

}

