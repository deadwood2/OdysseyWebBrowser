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
#include <math.h>

#include "NotImplemented.h"
#include "LocalizedStrings.h"
#include <wtf/text/WTFString.h>

#include "include/macros/vapor.h"
#include "owb_cat.h"


namespace WebCore {

String submitButtonDefaultLabel()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_SUBMIT));
}

String inputElementAltText()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_SUBMIT));
}

String resetButtonDefaultLabel()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_RESET));
}

String searchableIndexIntroduction()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_SEARCHABLE_INDEX));
}

String fileButtonChooseFileLabel()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_CHOOSE_FILE));
}

String fileButtonChooseMultipleFilesLabel()
{
        return String::fromUTF8(GSI(MSG_LOCALIZED_CHOOSE_FILE));
}

String fileButtonNoFileSelectedLabel()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_NO_FILE));
}

String fileButtonNoFilesSelectedLabel()
{
        return String::fromUTF8(GSI(MSG_LOCALIZED_NO_FILE));
}

String defaultDetailsSummaryText()
{
        return String::fromUTF8(GSI(MSG_LOCALIZED_DETAILS));
}

String contextMenuItemTagOpenLinkInNewWindow()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_IN_NEW_WINDOW));
}

String contextMenuItemTagOpenLinkInNewTab()
{
  return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_IN_NEW_TAB));
}

String contextMenuItemTagOpenLinkInNewBackgroundTab()
{
  return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_IN_NEW_BACKGROUND_TAB));
}

String contextMenuItemTagDownloadLinkToDisk()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_DOWNLOAD_LINK));
}

String contextMenuItemTagCopyLinkToClipboard()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_COPY_LINK_LOCATION));
}

String contextMenuItemTagOpenImageInNewWindow()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_IMAGE_IN_NEW_WINDOW));
}

String contextMenuItemTagDownloadImageToDisk()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_SAVE_IMAGE_AS));
}

String contextMenuItemTagCopyImageToClipboard()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_COPY_IMAGE));
}

String contextMenuItemTagCopyImageUrlToClipboard()
{
        return String::fromUTF8(GSI(MSG_LOCALIZED_COPY_IMAGE_URL));
}

String contextMenuItemTagOpenImageInNewTab()
{
  return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_IMAGE_IN_NEW_TAB));
}

String contextMenuItemTagBlockImage()
{
  return String::fromUTF8(GSI(MSG_LOCALIZED_BLOCK_IMAGES));
}

String contextMenuItemTagOpenFrameInNewWindow()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_FRAME_IN_NEW_WINDOW));
}

String contextMenuItemTagOpenFrameInNewTab()
{
  return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_FRAME_IN_NEW_TAB));
}

String contextMenuItemTagCopy()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_COPY));
    return stockLabel;
}

String contextMenuItemTagDelete()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_DELETE));
    return stockLabel;
}

String contextMenuItemTagSelectAll()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_SELECT_ALL));
    return stockLabel;
}

String contextMenuItemTagUnicode()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_INSERT_UNICODE));
}

String contextMenuItemTagInputMethods()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_INPUT_METHODS));
}

String contextMenuItemTagGoBack()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_BACK));
    return stockLabel;
}

String contextMenuItemTagGoForward()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_FORWARD));
    return stockLabel;
}

String contextMenuItemTagStop()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_STOP));
    return stockLabel;
}

String contextMenuItemTagReload()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_RELOAD));
}

String contextMenuItemTagCut()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_CUT));
    return stockLabel;
}

String contextMenuItemTagPaste()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_PASTE));
    return stockLabel;
}

String contextMenuItemTagNoGuessesFound()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_NO_GUESSES));
}

String contextMenuItemTagIgnoreSpelling()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_IGNORE_SPELLING));
}

String contextMenuItemTagLearnSpelling()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_LEARN_SPELLING));
}

String contextMenuItemTagSearchWeb()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_SEARCH));
}

String contextMenuItemTagLookUpInDictionary()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_LOOKUP_IN_DICTIONARY));
}

String contextMenuItemTagOpenLink()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_LINK));
}

String contextMenuItemTagIgnoreGrammar()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_IGNORE_GRAMMAR));
}

String contextMenuItemTagSpellingMenu()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_SPELLING_AND_GRAMMAR));
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return String::fromUTF8(show ? GSI(MSG_LOCALIZED_SHOW_SPELLING_AND_GRAMMAR) : GSI(MSG_LOCALIZED_HIDE_SPELLING_AND_GRAMMAR));
}

String contextMenuItemTagCheckSpelling()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_CHECK_DOCUMENT));
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_CHECK_SPELLING_WHILE_TYPING));
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_CHECK_GRAMMAR_WITH_SPELLING));
}

String contextMenuItemTagDictionaryMenu()
{
        return String::fromUTF8(GSI(MSG_LOCALIZED_DICTIONARIES));
}

String contextMenuItemTagFontMenu()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_FONT));
}

String contextMenuItemTagBold()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_BOLD));
    return stockLabel;
}

String contextMenuItemTagItalic()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_ITALIC));
    return stockLabel;
}

String contextMenuItemTagUnderline()
{
    static String stockLabel = String::fromUTF8(GSI(MSG_LOCALIZED_UNDERLINE));
    return stockLabel;
}

String contextMenuItemTagOutline()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_OUTLINE));
}

String contextMenuItemTagInspectElement()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_INSPECT_ELEMENT));
}

String searchMenuNoRecentSearchesText()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_NO_RECENT_SEARCHES));
}

String searchMenuRecentSearchesText()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_RECENT_SEARCHES));
}

String searchMenuClearRecentSearchesText()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_CLEAR_RECENT_SEARCHES));
}

String AXDefinitionListTermText()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_TERM));
}

String AXDefinitionListDefinitionText()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_DEFINITION));
}

String contextMenuItemTagLeftToRight()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_LEFT_TO_RIGHT));
}

String contextMenuItemTagDefaultDirection()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_DEFAULT_DIRECTION));
}

String contextMenuItemTagRightToLeft()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_RIGHT_TO_LEFT));
}

String contextMenuItemTagWritingDirectionMenu()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_WRITING_DIRECTION));
}

String contextMenuItemTagTextDirectionMenu()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_TEXT_DIRECTION));
}

String contextMenuItemTagOpenVideoInNewWindow()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_MEDIA_IN_NEW_WINDOW));
}
String contextMenuItemTagOpenAudioInNewWindow()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_OPEN_MEDIA_IN_NEW_WINDOW));
}
String contextMenuItemTagCopyVideoLinkToClipboard()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_COPYMEDIAURL));
}
String contextMenuItemTagCopyAudioLinkToClipboard()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_COPYMEDIAURL));
}
String contextMenuItemTagToggleMediaControls()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_MEDIACONTROLS));
}
String contextMenuItemTagToggleMediaLoop()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_MEDIALOOP));
}
String contextMenuItemTagEnterVideoFullscreen()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_SWITCH_TO_FULLSCREEN));
}
String contextMenuItemTagMediaPlay()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_PLAY));
}
String contextMenuItemTagMediaPause()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_PAUSE));
}
String contextMenuItemTagMediaMute()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_MUTE));
}
String contextMenuItemTagDownloadMedia()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_DOWNLOADMEDIA));
}

String contextMenuItemTagDownloadVideoToDisk()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_DOWNLOADMEDIA));
}

String contextMenuItemTagDownloadAudioToDisk()
{
    return String::fromUTF8(GSI(MSG_LOCALIZED_DOWNLOADMEDIA));
}

#if ENABLE(INPUT_TYPE_WEEK)
String weekFormatInLDML()
{
    return String();
}
#endif

}

