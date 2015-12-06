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

String AXDescriptionListText()
{
        return String::fromUTF8("Description");
}

String AXDefinitionListTermText()
{
	return String::fromUTF8(GSI(MSG_LOCALIZED_TERM));
}

String AXDefinitionListDefinitionText()
{
	return String::fromUTF8(GSI(MSG_LOCALIZED_DEFINITION));
}

String AXFileUploadButtonText()
{
    return String();
}

String AXButtonActionVerb()
{
    return String();
}

String AXRadioButtonActionVerb()
{
    return String();
}

String AXTextFieldActionVerb()
{
    return String();
}

String AXCheckedCheckBoxActionVerb()
{
    return String();
}

String AXUncheckedCheckBoxActionVerb()
{
    return String();
}

String AXMenuListPopupActionVerb()
{
    return String();
}

String AXMenuListActionVerb()
{
    return String();
}

String AXLinkActionVerb()
{
    return String();
}

String AXListItemActionVerb()
{
    return String();
}

String AXSearchFieldCancelButtonText()
{
    return String();
}

String AXAutoFillButtonText()
{
    return String::fromUTF8("autofill");
}

String unknownFileSizeText()
{
	return String::fromUTF8(GSI(MSG_LOCALIZED_UNKNOWN));
}

String imageTitle(const String&, const IntSize&)
{
    return String();
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

String multipleFileUploadText(unsigned)
{
    return String();
}

#if ENABLE(VIDEO)

String mediaElementLoadingStateText()
{
    return String::fromUTF8("Loading ...");
}

String mediaElementLiveBroadcastStateText()
{
    return String::fromUTF8("Live broadcast");
}

String localizedMediaControlElementString(const String& name)
{
    if (name == "AudioElement")
        return String::fromUTF8("audio element controller");
    if (name == "VideoElement")
        return String::fromUTF8("video element controller");
    if (name == "MuteButton")
        return String::fromUTF8("mute");
    if (name == "UnMuteButton")
        return String::fromUTF8("unmute");
    if (name == "PlayButton")
        return String::fromUTF8("play");
    if (name == "PauseButton")
        return String::fromUTF8("pause");
    if (name == "Slider")
        return String::fromUTF8("movie time");
    if (name == "SliderThumb")
        return String::fromUTF8("timeline slider thumb");
    if (name == "RewindButton")
        return String::fromUTF8("back 30 seconds");
    if (name == "ReturnToRealtimeButton")
        return String::fromUTF8("return to realtime");
    if (name == "CurrentTimeDisplay")
        return String::fromUTF8("elapsed time");
    if (name == "TimeRemainingDisplay")
        return String::fromUTF8("remaining time");
    if (name == "StatusDisplay")
        return String::fromUTF8("status");
    if (name == "FullscreenButton")
        return String::fromUTF8("fullscreen");
    if (name == "SeekForwardButton")
        return String::fromUTF8("fast forward");
    if (name == "SeekBackButton")
        return String::fromUTF8("fast reverse");

    ASSERT_NOT_REACHED();
    return String();
}

String localizedMediaControlElementHelpText(const String& name)
{
    if (name == "AudioElement")
        return String::fromUTF8("audio element playback controls and status display");
    if (name == "VideoElement")
        return String::fromUTF8("video element playback controls and status display");
    if (name == "MuteButton")
        return String::fromUTF8("mute audio tracks");
    if (name == "UnMuteButton")
        return String::fromUTF8("unmute audio tracks");
    if (name == "PlayButton")
        return String::fromUTF8("begin playback");
    if (name == "PauseButton")
        return String::fromUTF8("pause playback");
    if (name == "Slider")
        return String::fromUTF8("movie time scrubber");
    if (name == "SliderThumb")
        return String::fromUTF8("movie time scrubber thumb");
    if (name == "RewindButton")
        return String::fromUTF8("seek movie back 30 seconds");
    if (name == "ReturnToRealtimeButton")
        return String::fromUTF8("return streaming movie to real time");
    if (name == "CurrentTimeDisplay")
        return String::fromUTF8("current movie time in seconds");
    if (name == "TimeRemainingDisplay")
        return String::fromUTF8("number of seconds of movie remaining");
    if (name == "StatusDisplay")
        return String::fromUTF8("current movie status");
    if (name == "SeekBackButton")
        return String::fromUTF8("seek quickly back");
    if (name == "SeekForwardButton")
        return String::fromUTF8("seek quickly forward");
    if (name == "FullscreenButton")
        return String::fromUTF8("Play movie in fullscreen mode");

    ASSERT_NOT_REACHED();
    return String();
}

String localizedMediaTimeDescription(float time)
{
    if (!std::isfinite(time))
        return String::fromUTF8("indefinite time");

    int seconds = (int)fabsf(time);
    int days = seconds / (60 * 60 * 24);
    int hours = seconds / (60 * 60);
    int minutes = (seconds / 60) % 60;
    seconds %= 60;

    String timeString;
    if (days) {
        timeString.append(String::number(days));
        timeString.append(" days ");
    }

    if (hours) {
        timeString.append(String::number(hours));
        timeString.append(" hours ");

    }

    if (minutes) {
        timeString.append(String::number(minutes));
	timeString.append( " minutes ");
    }

    timeString.append(String::number(seconds));
    timeString.append(" seconds");
    return timeString;
}

#endif // ENABLE(VIDEO)

String validationMessagePatternMismatchText()
{
    return String::fromUTF8("pattern mismatch");
}

String validationMessageRangeOverflowText(const String&)
{
    return String::fromUTF8("range overflow");
}

String validationMessageRangeUnderflowText(const String&)
{
    return String::fromUTF8("range underflow");
}

String validationMessageStepMismatchText(const String&, const String&)
{
    return String::fromUTF8("step mismatch");
}

String validationMessageTooLongText(int, int)
{
    return String::fromUTF8("too long");
}

String validationMessageTypeMismatchText()
{
    return String::fromUTF8("type mismatch");
}

String validationMessageTypeMismatchForEmailText()
{
    return String::fromUTF8("type mismatch");
}

String validationMessageTypeMismatchForMultipleEmailText()
{
    return String::fromUTF8("type mismatch");
}

String validationMessageTypeMismatchForURLText()
{
    return String::fromUTF8("type mismatch");
}

String validationMessageValueMissingText()
{
    return String::fromUTF8("value missing");
}

String validationMessageValueMissingForCheckboxText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForMultipleFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForRadioText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForSelectText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String missingPluginText()
{
	return String::fromUTF8("Missing plugin");
}

String crashedPluginText()
{
	return String::fromUTF8("Crashed plugin");
}

String blockedPluginByContentSecurityPolicyText()
{
    return String::fromUTF8("Blocked plugin");
}

String insecurePluginVersionText()
{
    return String::fromUTF8("Insecure plugin");
}

String inactivePluginText()
{
    return String::fromUTF8("Inactive plugin");
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

String validationMessageBadInputForNumberText()
{
    return String();
}

String clickToExitFullScreenText()
{
    return String();
}

#if ENABLE(INPUT_TYPE_WEEK)
String weekFormatInLDML()
{
    return String();
}
#endif

#if ENABLE(VIDEO_TRACK)
String textTrackClosedCaptionsText()
{
  return String::fromUTF8("Closed Captions");
}

String textTrackSubtitlesText()
{
  return String::fromUTF8("Subtitleq");
}

String textTrackOffMenuItemText()
{
  return String::fromUTF8("Off");
}

String textTrackAutomaticMenuItemText()
{
  return String::fromUTF8("Automatic (todo)");
}

String textTrackNoLabelText()
{
  return String::fromUTF8("No Label");
}

String audioTrackNoLabelText()
{
  return String::fromUTF8("No Label");
}
#endif

String snapshottedPlugInLabelTitle()
{
  return String::fromUTF8("Snapshotted Plug-In");
}

String snapshottedPlugInLabelSubtitle()
{
  return String::fromUTF8("Click to restart");
}

}

