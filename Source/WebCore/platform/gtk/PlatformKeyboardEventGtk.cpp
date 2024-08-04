/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 * All rights reserved.
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
#include "PlatformKeyboardEvent.h"

#include "GtkVersioning.h"
#include "NotImplemented.h"
#include "TextEncoding.h"
#include "WindowsKeyboardCodes.h"
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <wtf/CurrentTime.h>
#include <wtf/glib/GUniquePtr.h>

namespace WebCore {

// FIXME: This is incomplete.  We should change this to mirror
// more like what Firefox does, and generate these switch statements
// at build time.
// https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/key/Key_Values
String PlatformKeyboardEvent::keyValueForGdkKeyCode(unsigned keyCode)
{
    switch (keyCode) {
    // Modifier keys.
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
        return ASCIILiteral("Alt");
    // Firefox uses GDK_KEY_Mode_switch for AltGraph as well.
    case GDK_KEY_ISO_Level3_Shift:
    case GDK_KEY_ISO_Level3_Latch:
    case GDK_KEY_ISO_Level3_Lock:
    case GDK_KEY_ISO_Level5_Shift:
    case GDK_KEY_ISO_Level5_Latch:
    case GDK_KEY_ISO_Level5_Lock:
        return ASCIILiteral("AltGraph");
    case GDK_KEY_Caps_Lock:
        return ASCIILiteral("CapsLock");
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
        return ASCIILiteral("Control");
    // Fn: This is typically a hardware key that does not generate a separate code.
    // FnLock.
    case GDK_KEY_Hyper_L:
    case GDK_KEY_Hyper_R:
        return ASCIILiteral("Hyper");
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
        return ASCIILiteral("Meta");
    case GDK_KEY_Num_Lock:
        return ASCIILiteral("NumLock");
    case GDK_KEY_Scroll_Lock:
        return ASCIILiteral("ScrollLock");
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
        return ASCIILiteral("Shift");
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
        return ASCIILiteral("Super");
    // Symbol.
    // SymbolLock.

    // Whitespace keys.
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
    case GDK_KEY_3270_Enter:
        return ASCIILiteral("Enter");
    case GDK_KEY_Tab:
    case GDK_KEY_KP_Tab:
        return ASCIILiteral("Tab");

    // Navigation keys.
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
        return ASCIILiteral("ArrowDown");
    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
        return ASCIILiteral("ArrowLeft");
    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
        return ASCIILiteral("ArrowRight");
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
        return ASCIILiteral("ArrowUp");
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
        return ASCIILiteral("End");
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
        return ASCIILiteral("Home");
    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
        return ASCIILiteral("PageDown");
    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
        return ASCIILiteral("PageUp");

    // Editing keys.
    case GDK_KEY_BackSpace:
        return ASCIILiteral("Backspace");
    case GDK_KEY_Clear:
        return ASCIILiteral("Clear");
    case GDK_KEY_Copy:
        return ASCIILiteral("Copy");
    case GDK_KEY_3270_CursorSelect:
        return ASCIILiteral("CrSel");
    case GDK_KEY_Cut:
        return ASCIILiteral("Cut");
    case GDK_KEY_Delete:
    case GDK_KEY_KP_Delete:
        return ASCIILiteral("Delete");
    case GDK_KEY_3270_EraseEOF:
        return ASCIILiteral("EraseEof");
    case GDK_KEY_3270_ExSelect:
        return ASCIILiteral("ExSel");
    case GDK_KEY_Insert:
    case GDK_KEY_KP_Insert:
        return ASCIILiteral("Insert");
    case GDK_KEY_Paste:
        return ASCIILiteral("Paste");
    case GDK_KEY_Redo:
        return ASCIILiteral("Redo");
    case GDK_KEY_Undo:
        return ASCIILiteral("Undo");

    // UI keys.
    // Accept.
    // Again.
    case GDK_KEY_3270_Attn:
        return ASCIILiteral("Attn");
    case GDK_KEY_Cancel:
        return ASCIILiteral("Cancel");
    case GDK_KEY_Menu:
        return ASCIILiteral("ContextMenu");
    case GDK_KEY_Escape:
        return ASCIILiteral("Escape");
    case GDK_KEY_Execute:
        return ASCIILiteral("Execute");
    case GDK_KEY_Find:
        return ASCIILiteral("Find");
    case GDK_KEY_Help:
        return ASCIILiteral("Help");
    case GDK_KEY_Pause:
    case GDK_KEY_Break:
        return ASCIILiteral("Pause");
    case GDK_KEY_3270_Play:
        return ASCIILiteral("Play");
    // Props.
    case GDK_KEY_Select:
        return ASCIILiteral("Select");
    case GDK_KEY_ZoomIn:
        return ASCIILiteral("ZoomIn");
    case GDK_KEY_ZoomOut:
        return ASCIILiteral("ZoomOut");

    // Device keys.
    case GDK_KEY_MonBrightnessDown:
        return ASCIILiteral("BrightnessDown");
    case GDK_KEY_MonBrightnessUp:
        return ASCIILiteral("BrightnessUp");
    case GDK_KEY_Eject:
        return ASCIILiteral("Eject");
    case GDK_KEY_LogOff:
        return ASCIILiteral("LogOff");
    // Power.
    case GDK_KEY_PowerDown:
    case GDK_KEY_PowerOff:
        return ASCIILiteral("PowerOff");
    case GDK_KEY_3270_PrintScreen:
    case GDK_KEY_Print:
    case GDK_KEY_Sys_Req:
        return ASCIILiteral("PrintScreen");
    case GDK_KEY_Hibernate:
        return ASCIILiteral("Hibernate");
    case GDK_KEY_Standby:
    case GDK_KEY_Suspend:
    case GDK_KEY_Sleep:
        return ASCIILiteral("Standby");
    case GDK_KEY_WakeUp:
        return ASCIILiteral("WakeUp");

    // IME keys.
    case GDK_KEY_MultipleCandidate:
        return ASCIILiteral("AllCandidates");
    case GDK_KEY_Eisu_Shift:
    case GDK_KEY_Eisu_toggle:
        return ASCIILiteral("Alphanumeric");
    case GDK_KEY_Codeinput:
        return ASCIILiteral("CodeInput");
    case GDK_KEY_Multi_key:
        return ASCIILiteral("Compose");
    case GDK_KEY_Henkan:
        return ASCIILiteral("Convert");
    case GDK_KEY_dead_grave:
    case GDK_KEY_dead_acute:
    case GDK_KEY_dead_circumflex:
    case GDK_KEY_dead_tilde:
    case GDK_KEY_dead_macron:
    case GDK_KEY_dead_breve:
    case GDK_KEY_dead_abovedot:
    case GDK_KEY_dead_diaeresis:
    case GDK_KEY_dead_abovering:
    case GDK_KEY_dead_doubleacute:
    case GDK_KEY_dead_caron:
    case GDK_KEY_dead_cedilla:
    case GDK_KEY_dead_ogonek:
    case GDK_KEY_dead_iota:
    case GDK_KEY_dead_voiced_sound:
    case GDK_KEY_dead_semivoiced_sound:
    case GDK_KEY_dead_belowdot:
    case GDK_KEY_dead_hook:
    case GDK_KEY_dead_horn:
    case GDK_KEY_dead_stroke:
    case GDK_KEY_dead_abovecomma:
    case GDK_KEY_dead_abovereversedcomma:
    case GDK_KEY_dead_doublegrave:
    case GDK_KEY_dead_belowring:
    case GDK_KEY_dead_belowmacron:
    case GDK_KEY_dead_belowcircumflex:
    case GDK_KEY_dead_belowtilde:
    case GDK_KEY_dead_belowbreve:
    case GDK_KEY_dead_belowdiaeresis:
    case GDK_KEY_dead_invertedbreve:
    case GDK_KEY_dead_belowcomma:
    case GDK_KEY_dead_currency:
    case GDK_KEY_dead_a:
    case GDK_KEY_dead_A:
    case GDK_KEY_dead_e:
    case GDK_KEY_dead_E:
    case GDK_KEY_dead_i:
    case GDK_KEY_dead_I:
    case GDK_KEY_dead_o:
    case GDK_KEY_dead_O:
    case GDK_KEY_dead_u:
    case GDK_KEY_dead_U:
    case GDK_KEY_dead_small_schwa:
    case GDK_KEY_dead_capital_schwa:
        return ASCIILiteral("Dead");
    // FinalMode
    case GDK_KEY_ISO_First_Group:
        return ASCIILiteral("GroupFirst");
    case GDK_KEY_ISO_Last_Group:
        return ASCIILiteral("GroupLast");
    case GDK_KEY_ISO_Next_Group:
        return ASCIILiteral("GroupNext");
    case GDK_KEY_ISO_Prev_Group:
        return ASCIILiteral("GroupPrevious");
    case GDK_KEY_Mode_switch:
        return ASCIILiteral("ModeChange");
    // NextCandidate.
    case GDK_KEY_Muhenkan:
        return ASCIILiteral("NonConvert");
    case GDK_KEY_PreviousCandidate:
        return ASCIILiteral("PreviousCandidate");
    // Process.
    case GDK_KEY_SingleCandidate:
        return ASCIILiteral("SingleCandidate");

    // Korean and Japanese keys.
    case GDK_KEY_Hangul:
        return ASCIILiteral("HangulMode");
    case GDK_KEY_Hangul_Hanja:
        return ASCIILiteral("HanjaMode");
    case GDK_KEY_Hangul_Jeonja:
        return ASCIILiteral("JunjaMode");
    case GDK_KEY_Hankaku:
        return ASCIILiteral("Hankaku");
    case GDK_KEY_Hiragana:
        return ASCIILiteral("Hiragana");
    case GDK_KEY_Hiragana_Katakana:
        return ASCIILiteral("HiraganaKatakana");
    case GDK_KEY_Kana_Lock:
    case GDK_KEY_Kana_Shift:
        return ASCIILiteral("KanaMode");
    case GDK_KEY_Kanji:
        return ASCIILiteral("KanjiMode");
    case GDK_KEY_Katakana:
        return ASCIILiteral("Katakana");
    case GDK_KEY_Romaji:
        return ASCIILiteral("Romaji");
    case GDK_KEY_Zenkaku:
        return ASCIILiteral("Zenkaku");
    case GDK_KEY_Zenkaku_Hankaku:
        return ASCIILiteral("ZenkakuHanaku");

    // Multimedia keys.
    // ChannelDown.
    // ChannelUp.
    case GDK_KEY_Close:
        return ASCIILiteral("Close");
    case GDK_KEY_MailForward:
        return ASCIILiteral("MailForward");
    case GDK_KEY_Reply:
        return ASCIILiteral("MailReply");
    case GDK_KEY_Send:
        return ASCIILiteral("MailSend");
    case GDK_KEY_AudioForward:
        return ASCIILiteral("MediaFastForward");
    case GDK_KEY_AudioPause:
        return ASCIILiteral("MediaPause");
    case GDK_KEY_AudioPlay:
        return ASCIILiteral("MediaPlay");
    // MediaPlayPause
    case GDK_KEY_AudioRecord:
        return ASCIILiteral("MediaRecord");
    case GDK_KEY_AudioRewind:
        return ASCIILiteral("MediaRewind");
    case GDK_KEY_AudioStop:
        return ASCIILiteral("MediaStop");
    case GDK_KEY_AudioNext:
        return ASCIILiteral("MediaTrackNext");
    case GDK_KEY_AudioPrev:
        return ASCIILiteral("MediaTrackPrevious");
    case GDK_KEY_New:
        return ASCIILiteral("New");
    case GDK_KEY_Open:
        return ASCIILiteral("Open");
    // Print.
    case GDK_KEY_Save:
        return ASCIILiteral("Save");
    case GDK_KEY_Spell:
        return ASCIILiteral("SpellCheck");

    // Function keys.
    case GDK_KEY_F1:
        return ASCIILiteral("F1");
    case GDK_KEY_F2:
        return ASCIILiteral("F2");
    case GDK_KEY_F3:
        return ASCIILiteral("F3");
    case GDK_KEY_F4:
        return ASCIILiteral("F4");
    case GDK_KEY_F5:
        return ASCIILiteral("F5");
    case GDK_KEY_F6:
        return ASCIILiteral("F6");
    case GDK_KEY_F7:
        return ASCIILiteral("F7");
    case GDK_KEY_F8:
        return ASCIILiteral("F8");
    case GDK_KEY_F9:
        return ASCIILiteral("F9");
    case GDK_KEY_F10:
        return ASCIILiteral("F10");
    case GDK_KEY_F11:
        return ASCIILiteral("F11");
    case GDK_KEY_F12:
        return ASCIILiteral("F12");
    case GDK_KEY_F13:
        return ASCIILiteral("F13");
    case GDK_KEY_F14:
        return ASCIILiteral("F14");
    case GDK_KEY_F15:
        return ASCIILiteral("F15");
    case GDK_KEY_F16:
        return ASCIILiteral("F16");
    case GDK_KEY_F17:
        return ASCIILiteral("F17");
    case GDK_KEY_F18:
        return ASCIILiteral("F18");
    case GDK_KEY_F19:
        return ASCIILiteral("F19");
    case GDK_KEY_F20:
        return ASCIILiteral("F20");

    default: {
        guint32 unicodeCharacter = gdk_keyval_to_unicode(keyCode);
        if (unicodeCharacter) {
            // UTF-8 will use up to 6 bytes.
            char utf8[7] = { 0 };
            g_unichar_to_utf8(unicodeCharacter, utf8);
            return String::fromUTF8(utf8);
        }
        return ASCIILiteral("Unidentified");
    }
    }
}

// FIXME: This is incomplete. We should change this to mirror
// more like what Firefox does, and generate these switch statements
// at build time.
// https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/code
String PlatformKeyboardEvent::keyCodeForHardwareKeyCode(unsigned keyCode)
{
    switch (keyCode) {
    case 0x0009:
        return ASCIILiteral("Escape");
    case 0x000A:
        return ASCIILiteral("Digit1");
    case 0x000B:
        return ASCIILiteral("Digit2");
    case 0x000C:
        return ASCIILiteral("Digit3");
    case 0x000D:
        return ASCIILiteral("Digit4");
    case 0x000E:
        return ASCIILiteral("Digit5");
    case 0x000F:
        return ASCIILiteral("Digit6");
    case 0x0010:
        return ASCIILiteral("Digit7");
    case 0x0011:
        return ASCIILiteral("Digit8");
    case 0x0012:
        return ASCIILiteral("Digit9");
    case 0x0013:
        return ASCIILiteral("Digit0");
    case 0x0014:
        return ASCIILiteral("Minus");
    case 0x0015:
        return ASCIILiteral("Equal");
    case 0x0016:
        return ASCIILiteral("Backspace");
    case 0x0017:
        return ASCIILiteral("Tab");
    case 0x0018:
        return ASCIILiteral("KeyQ");
    case 0x0019:
        return ASCIILiteral("KeyW");
    case 0x001A:
        return ASCIILiteral("KeyE");
    case 0x001B:
        return ASCIILiteral("KeyR");
    case 0x001C:
        return ASCIILiteral("KeyT");
    case 0x001D:
        return ASCIILiteral("KeyY");
    case 0x001E:
        return ASCIILiteral("KeyU");
    case 0x001F:
        return ASCIILiteral("KeyI");
    case 0x0020:
        return ASCIILiteral("KeyO");
    case 0x0021:
        return ASCIILiteral("KeyP");
    case 0x0022:
        return ASCIILiteral("BracketLeft");
    case 0x0023:
        return ASCIILiteral("BracketRight");
    case 0x0024:
        return ASCIILiteral("Enter");
    case 0x0025:
        return ASCIILiteral("ControlLeft");
    case 0x0026:
        return ASCIILiteral("KeyA");
    case 0x0027:
        return ASCIILiteral("KeyS");
    case 0x0028:
        return ASCIILiteral("KeyD");
    case 0x0029:
        return ASCIILiteral("KeyF");
    case 0x002A:
        return ASCIILiteral("KeyG");
    case 0x002B:
        return ASCIILiteral("KeyH");
    case 0x002C:
        return ASCIILiteral("KeyJ");
    case 0x002D:
        return ASCIILiteral("KeyK");
    case 0x002E:
        return ASCIILiteral("KeyL");
    case 0x002F:
        return ASCIILiteral("Semicolon");
    case 0x0030:
        return ASCIILiteral("Quote");
    case 0x0031:
        return ASCIILiteral("Backquote");
    case 0x0032:
        return ASCIILiteral("ShiftLeft");
    case 0x0033:
        return ASCIILiteral("Backslash");
    case 0x0034:
        return ASCIILiteral("KeyZ");
    case 0x0035:
        return ASCIILiteral("KeyX");
    case 0x0036:
        return ASCIILiteral("KeyC");
    case 0x0037:
        return ASCIILiteral("KeyV");
    case 0x0038:
        return ASCIILiteral("KeyB");
    case 0x0039:
        return ASCIILiteral("KeyN");
    case 0x003A:
        return ASCIILiteral("KeyM");
    case 0x003B:
        return ASCIILiteral("Comma");
    case 0x003C:
        return ASCIILiteral("Period");
    case 0x003D:
        return ASCIILiteral("Slash");
    case 0x003E:
        return ASCIILiteral("ShiftRight");
    case 0x003F:
        return ASCIILiteral("NumpadMultiply");
    case 0x0040:
        return ASCIILiteral("AltLeft");
    case 0x0041:
        return ASCIILiteral("Space");
    case 0x0042:
        return ASCIILiteral("CapsLock");
    case 0x0043:
        return ASCIILiteral("F1");
    case 0x0044:
        return ASCIILiteral("F2");
    case 0x0045:
        return ASCIILiteral("F3");
    case 0x0046:
        return ASCIILiteral("F4");
    case 0x0047:
        return ASCIILiteral("F5");
    case 0x0048:
        return ASCIILiteral("F6");
    case 0x0049:
        return ASCIILiteral("F7");
    case 0x004A:
        return ASCIILiteral("F8");
    case 0x004B:
        return ASCIILiteral("F9");
    case 0x004C:
        return ASCIILiteral("F10");
    case 0x004D:
        return ASCIILiteral("NumLock");
    case 0x004E:
        return ASCIILiteral("ScrollLock");
    case 0x004F:
        return ASCIILiteral("Numpad7");
    case 0x0050:
        return ASCIILiteral("Numpad8");
    case 0x0051:
        return ASCIILiteral("Numpad9");
    case 0x0052:
        return ASCIILiteral("NumpadSubtract");
    case 0x0053:
        return ASCIILiteral("Numpad4");
    case 0x0054:
        return ASCIILiteral("Numpad5");
    case 0x0055:
        return ASCIILiteral("Numpad6");
    case 0x0056:
        return ASCIILiteral("NumpadAdd");
    case 0x0057:
        return ASCIILiteral("Numpad1");
    case 0x0058:
        return ASCIILiteral("Numpad2");
    case 0x0059:
        return ASCIILiteral("Numpad3");
    case 0x005A:
        return ASCIILiteral("Numpad0");
    case 0x005B:
        return ASCIILiteral("NumpadDecimal");
    case 0x005E:
        return ASCIILiteral("IntlBackslash");
    case 0x005F:
        return ASCIILiteral("F11");
    case 0x0060:
        return ASCIILiteral("F12");
    case 0x0061:
        return ASCIILiteral("IntlRo");
    case 0x0064:
        return ASCIILiteral("Convert");
    case 0x0065:
        return ASCIILiteral("KanaMode");
    case 0x0066:
        return ASCIILiteral("NonConvert");
    case 0x0068:
        return ASCIILiteral("NumpadEnter");
    case 0x0069:
        return ASCIILiteral("ControlRight");
    case 0x006A:
        return ASCIILiteral("NumpadDivide");
    case 0x006B:
        return ASCIILiteral("PrintScreen");
    case 0x006C:
        return ASCIILiteral("AltRight");
    case 0x006E:
        return ASCIILiteral("Home");
    case 0x006F:
        return ASCIILiteral("ArrowUp");
    case 0x0070:
        return ASCIILiteral("PageUp");
    case 0x0071:
        return ASCIILiteral("ArrowLeft");
    case 0x0072:
        return ASCIILiteral("ArrowRight");
    case 0x0073:
        return ASCIILiteral("End");
    case 0x0074:
        return ASCIILiteral("ArrowDown");
    case 0x0075:
        return ASCIILiteral("PageDown");
    case 0x0076:
        return ASCIILiteral("Insert");
    case 0x0077:
        return ASCIILiteral("Delete");
    case 0x0079:
        return ASCIILiteral("AudioVolumeMute");
    case 0x007A:
        return ASCIILiteral("AudioVolumeDown");
    case 0x007B:
        return ASCIILiteral("AudioVolumeUp");
    case 0x007D:
        return ASCIILiteral("NumpadEqual");
    case 0x007F:
        return ASCIILiteral("Pause");
    case 0x0081:
        return ASCIILiteral("NumpadComma");
    case 0x0082:
        return ASCIILiteral("Lang1");
    case 0x0083:
        return ASCIILiteral("Lang2");
    case 0x0084:
        return ASCIILiteral("IntlYen");
    case 0x0085:
        return ASCIILiteral("OSLeft");
    case 0x0086:
        return ASCIILiteral("OSRight");
    case 0x0087:
        return ASCIILiteral("ContextMenu");
    case 0x0088:
        return ASCIILiteral("BrowserStop");
    case 0x0089:
        return ASCIILiteral("Again");
    case 0x008A:
        return ASCIILiteral("Props");
    case 0x008B:
        return ASCIILiteral("Undo");
    case 0x008C:
        return ASCIILiteral("Select");
    case 0x008D:
        return ASCIILiteral("Copy");
    case 0x008E:
        return ASCIILiteral("Open");
    case 0x008F:
        return ASCIILiteral("Paste");
    case 0x0090:
        return ASCIILiteral("Find");
    case 0x0091:
        return ASCIILiteral("Cut");
    case 0x0092:
        return ASCIILiteral("Help");
    case 0x0094:
        return ASCIILiteral("LaunchApp2");
    case 0x0097:
        return ASCIILiteral("WakeUp");
    case 0x0098:
        return ASCIILiteral("LaunchApp1");
    case 0x00A3:
        return ASCIILiteral("LaunchMail");
    case 0x00A4:
        return ASCIILiteral("BrowserFavorites");
    case 0x00A6:
        return ASCIILiteral("BrowserBack");
    case 0x00A7:
        return ASCIILiteral("BrowserForward");
    case 0x00A9:
        return ASCIILiteral("Eject");
    case 0x00AB:
        return ASCIILiteral("MediaTrackNext");
    case 0x00AC:
        return ASCIILiteral("MediaPlayPause");
    case 0x00AD:
        return ASCIILiteral("MediaTrackPrevious");
    case 0x00AE:
        return ASCIILiteral("MediaStop");
    case 0x00B3:
        return ASCIILiteral("LaunchMediaPlayer");
    case 0x00B4:
        return ASCIILiteral("BrowserHome");
    case 0x00B5:
        return ASCIILiteral("BrowserRefresh");
    case 0x00BF:
        return ASCIILiteral("F13");
    case 0x00C0:
        return ASCIILiteral("F14");
    case 0x00C1:
        return ASCIILiteral("F15");
    case 0x00C2:
        return ASCIILiteral("F16");
    case 0x00C3:
        return ASCIILiteral("F17");
    case 0x00C4:
        return ASCIILiteral("F18");
    case 0x00C5:
        return ASCIILiteral("F19");
    case 0x00C6:
        return ASCIILiteral("F20");
    case 0x00C7:
        return ASCIILiteral("F21");
    case 0x00C8:
        return ASCIILiteral("F22");
    case 0x00C9:
        return ASCIILiteral("F23");
    case 0x00CA:
        return ASCIILiteral("F24");
    case 0x00E1:
        return ASCIILiteral("BrowserSearch");
    default:
        return ASCIILiteral("Unidentified");
    }
}

// FIXME: This is incomplete. We should change this to mirror
// more like what Firefox does, and generate these switch statements
// at build time.
String PlatformKeyboardEvent::keyIdentifierForGdkKeyCode(unsigned keyCode)
{
    switch (keyCode) {
        case GDK_Menu:
        case GDK_Alt_L:
        case GDK_Alt_R:
            return "Alt";
        case GDK_Clear:
            return "Clear";
        case GDK_Down:
            return "Down";
            // "End"
        case GDK_End:
            return "End";
            // "Enter"
        case GDK_ISO_Enter:
        case GDK_KP_Enter:
        case GDK_Return:
            return "Enter";
        case GDK_Execute:
            return "Execute";
        case GDK_F1:
            return "F1";
        case GDK_F2:
            return "F2";
        case GDK_F3:
            return "F3";
        case GDK_F4:
            return "F4";
        case GDK_F5:
            return "F5";
        case GDK_F6:
            return "F6";
        case GDK_F7:
            return "F7";
        case GDK_F8:
            return "F8";
        case GDK_F9:
            return "F9";
        case GDK_F10:
            return "F10";
        case GDK_F11:
            return "F11";
        case GDK_F12:
            return "F12";
        case GDK_F13:
            return "F13";
        case GDK_F14:
            return "F14";
        case GDK_F15:
            return "F15";
        case GDK_F16:
            return "F16";
        case GDK_F17:
            return "F17";
        case GDK_F18:
            return "F18";
        case GDK_F19:
            return "F19";
        case GDK_F20:
            return "F20";
        case GDK_F21:
            return "F21";
        case GDK_F22:
            return "F22";
        case GDK_F23:
            return "F23";
        case GDK_F24:
            return "F24";
        case GDK_Help:
            return "Help";
        case GDK_Home:
            return "Home";
        case GDK_Insert:
            return "Insert";
        case GDK_Left:
            return "Left";
        case GDK_Page_Down:
            return "PageDown";
        case GDK_Page_Up:
            return "PageUp";
        case GDK_Pause:
            return "Pause";
        case GDK_3270_PrintScreen:
        case GDK_Print:
            return "PrintScreen";
        case GDK_Right:
            return "Right";
        case GDK_Select:
            return "Select";
        case GDK_Up:
            return "Up";
            // Standard says that DEL becomes U+007F.
        case GDK_Delete:
            return "U+007F";
        case GDK_BackSpace:
            return "U+0008";
        case GDK_ISO_Left_Tab:
        case GDK_3270_BackTab:
        case GDK_Tab:
            return "U+0009";
        default:
            return String::format("U+%04X", gdk_keyval_to_unicode(gdk_keyval_to_upper(keyCode)));
    }
}

int PlatformKeyboardEvent::windowsKeyCodeForGdkKeyCode(unsigned keycode)
{
    switch (keycode) {
        case GDK_KP_0:
            return VK_NUMPAD0;// (60) Numeric keypad 0 key
        case GDK_KP_1:
            return VK_NUMPAD1;// (61) Numeric keypad 1 key
        case GDK_KP_2:
            return  VK_NUMPAD2; // (62) Numeric keypad 2 key
        case GDK_KP_3:
            return VK_NUMPAD3; // (63) Numeric keypad 3 key
        case GDK_KP_4:
            return VK_NUMPAD4; // (64) Numeric keypad 4 key
        case GDK_KP_5:
            return VK_NUMPAD5; //(65) Numeric keypad 5 key
        case GDK_KP_6:
            return VK_NUMPAD6; // (66) Numeric keypad 6 key
        case GDK_KP_7:
            return VK_NUMPAD7; // (67) Numeric keypad 7 key
        case GDK_KP_8:
            return VK_NUMPAD8; // (68) Numeric keypad 8 key
        case GDK_KP_9:
            return VK_NUMPAD9; // (69) Numeric keypad 9 key
        case GDK_KP_Multiply:
            return VK_MULTIPLY; // (6A) Multiply key
        case GDK_KP_Add:
            return VK_ADD; // (6B) Add key
        case GDK_KP_Subtract:
            return VK_SUBTRACT; // (6D) Subtract key
        case GDK_KP_Decimal:
            return VK_DECIMAL; // (6E) Decimal key
        case GDK_KP_Divide:
            return VK_DIVIDE; // (6F) Divide key

        case GDK_KP_Page_Up:
            return VK_PRIOR; // (21) PAGE UP key
        case GDK_KP_Page_Down:
            return VK_NEXT; // (22) PAGE DOWN key
        case GDK_KP_End:
            return VK_END; // (23) END key
        case GDK_KP_Home:
            return VK_HOME; // (24) HOME key
        case GDK_KP_Left:
            return VK_LEFT; // (25) LEFT ARROW key
        case GDK_KP_Up:
            return VK_UP; // (26) UP ARROW key
        case GDK_KP_Right:
            return VK_RIGHT; // (27) RIGHT ARROW key
        case GDK_KP_Down:
            return VK_DOWN; // (28) DOWN ARROW key

        case GDK_BackSpace:
            return VK_BACK; // (08) BACKSPACE key
        case GDK_ISO_Left_Tab:
        case GDK_3270_BackTab:
        case GDK_Tab:
            return VK_TAB; // (09) TAB key
        case GDK_Clear:
            return VK_CLEAR; // (0C) CLEAR key
        case GDK_ISO_Enter:
        case GDK_KP_Enter:
        case GDK_Return:
            return VK_RETURN; //(0D) Return key

            // VK_SHIFT (10) SHIFT key
            // VK_CONTROL (11) CTRL key

        case GDK_Menu:
            return VK_APPS;  // (5D) Applications key (Natural keyboard)

            // VK_MENU (12) ALT key

        case GDK_Pause:
            return VK_PAUSE; // (13) PAUSE key
        case GDK_Caps_Lock:
            return VK_CAPITAL; // (14) CAPS LOCK key
        case GDK_Kana_Lock:
        case GDK_Kana_Shift:
            return VK_KANA; // (15) Input Method Editor (IME) Kana mode
        case GDK_Hangul:
            return VK_HANGUL; // VK_HANGUL (15) IME Hangul mode
            // VK_JUNJA (17) IME Junja mode
            // VK_FINAL (18) IME final mode
        case GDK_Hangul_Hanja:
            return VK_HANJA; // (19) IME Hanja mode
        case GDK_Kanji:
            return VK_KANJI; // (19) IME Kanji mode
        case GDK_Escape:
            return VK_ESCAPE; // (1B) ESC key
            // VK_CONVERT (1C) IME convert
            // VK_NONCONVERT (1D) IME nonconvert
            // VK_ACCEPT (1E) IME accept
            // VK_MODECHANGE (1F) IME mode change request
        case GDK_space:
            return VK_SPACE; // (20) SPACEBAR
        case GDK_Page_Up:
            return VK_PRIOR; // (21) PAGE UP key
        case GDK_Page_Down:
            return VK_NEXT; // (22) PAGE DOWN key
        case GDK_End:
            return VK_END; // (23) END key
        case GDK_Home:
            return VK_HOME; // (24) HOME key
        case GDK_Left:
            return VK_LEFT; // (25) LEFT ARROW key
        case GDK_Up:
            return VK_UP; // (26) UP ARROW key
        case GDK_Right:
            return VK_RIGHT; // (27) RIGHT ARROW key
        case GDK_Down:
            return VK_DOWN; // (28) DOWN ARROW key
        case GDK_Select:
            return VK_SELECT; // (29) SELECT key
        case GDK_Print:
            return VK_SNAPSHOT; // (2C) PRINT SCREEN key
        case GDK_Execute:
            return VK_EXECUTE;// (2B) EXECUTE key
        case GDK_Insert:
        case GDK_KP_Insert:
            return VK_INSERT; // (2D) INS key
        case GDK_Delete:
        case GDK_KP_Delete:
            return VK_DELETE; // (2E) DEL key
        case GDK_Help:
            return VK_HELP; // (2F) HELP key
        case GDK_0:
        case GDK_parenright:
            return VK_0;    //  (30) 0) key
        case GDK_1:
        case GDK_exclam:
            return VK_1; //  (31) 1 ! key
        case GDK_2:
        case GDK_at:
            return VK_2; //  (32) 2 & key
        case GDK_3:
        case GDK_numbersign:
            return VK_3; //case '3': case '#';
        case GDK_4:
        case GDK_dollar: //  (34) 4 key '$';
            return VK_4;
        case GDK_5:
        case GDK_percent:
            return VK_5; //  (35) 5 key  '%'
        case GDK_6:
        case GDK_asciicircum:
            return VK_6; //  (36) 6 key  '^'
        case GDK_7:
        case GDK_ampersand:
            return VK_7; //  (37) 7 key  case '&'
        case GDK_8:
        case GDK_asterisk:
            return VK_8; //  (38) 8 key  '*'
        case GDK_9:
        case GDK_parenleft:
            return VK_9; //  (39) 9 key '('
        case GDK_a:
        case GDK_A:
            return VK_A; //  (41) A key case 'a': case 'A': return 0x41;
        case GDK_b:
        case GDK_B:
            return VK_B; //  (42) B key case 'b': case 'B': return 0x42;
        case GDK_c:
        case GDK_C:
            return VK_C; //  (43) C key case 'c': case 'C': return 0x43;
        case GDK_d:
        case GDK_D:
            return VK_D; //  (44) D key case 'd': case 'D': return 0x44;
        case GDK_e:
        case GDK_E:
            return VK_E; //  (45) E key case 'e': case 'E': return 0x45;
        case GDK_f:
        case GDK_F:
            return VK_F; //  (46) F key case 'f': case 'F': return 0x46;
        case GDK_g:
        case GDK_G:
            return VK_G; //  (47) G key case 'g': case 'G': return 0x47;
        case GDK_h:
        case GDK_H:
            return VK_H; //  (48) H key case 'h': case 'H': return 0x48;
        case GDK_i:
        case GDK_I:
            return VK_I; //  (49) I key case 'i': case 'I': return 0x49;
        case GDK_j:
        case GDK_J:
            return VK_J; //  (4A) J key case 'j': case 'J': return 0x4A;
        case GDK_k:
        case GDK_K:
            return VK_K; //  (4B) K key case 'k': case 'K': return 0x4B;
        case GDK_l:
        case GDK_L:
            return VK_L; //  (4C) L key case 'l': case 'L': return 0x4C;
        case GDK_m:
        case GDK_M:
            return VK_M; //  (4D) M key case 'm': case 'M': return 0x4D;
        case GDK_n:
        case GDK_N:
            return VK_N; //  (4E) N key case 'n': case 'N': return 0x4E;
        case GDK_o:
        case GDK_O:
            return VK_O; //  (4F) O key case 'o': case 'O': return 0x4F;
        case GDK_p:
        case GDK_P:
            return VK_P; //  (50) P key case 'p': case 'P': return 0x50;
        case GDK_q:
        case GDK_Q:
            return VK_Q; //  (51) Q key case 'q': case 'Q': return 0x51;
        case GDK_r:
        case GDK_R:
            return VK_R; //  (52) R key case 'r': case 'R': return 0x52;
        case GDK_s:
        case GDK_S:
            return VK_S; //  (53) S key case 's': case 'S': return 0x53;
        case GDK_t:
        case GDK_T:
            return VK_T; //  (54) T key case 't': case 'T': return 0x54;
        case GDK_u:
        case GDK_U:
            return VK_U; //  (55) U key case 'u': case 'U': return 0x55;
        case GDK_v:
        case GDK_V:
            return VK_V; //  (56) V key case 'v': case 'V': return 0x56;
        case GDK_w:
        case GDK_W:
            return VK_W; //  (57) W key case 'w': case 'W': return 0x57;
        case GDK_x:
        case GDK_X:
            return VK_X; //  (58) X key case 'x': case 'X': return 0x58;
        case GDK_y:
        case GDK_Y:
            return VK_Y; //  (59) Y key case 'y': case 'Y': return 0x59;
        case GDK_z:
        case GDK_Z:
            return VK_Z; //  (5A) Z key case 'z': case 'Z': return 0x5A;
        case GDK_Meta_L:
            return VK_LWIN; // (5B) Left Windows key (Microsoft Natural keyboard)
        case GDK_Meta_R:
            return VK_RWIN; // (5C) Right Windows key (Natural keyboard)
            // VK_SLEEP (5F) Computer Sleep key
            // VK_SEPARATOR (6C) Separator key
            // VK_SUBTRACT (6D) Subtract key
            // VK_DECIMAL (6E) Decimal key
            // VK_DIVIDE (6F) Divide key
            // handled by key code above

        case GDK_Num_Lock:
            return VK_NUMLOCK; // (90) NUM LOCK key

        case GDK_Scroll_Lock:
            return VK_SCROLL; // (91) SCROLL LOCK key

        case GDK_Shift_L:
            return VK_LSHIFT; // (A0) Left SHIFT key
        case GDK_Shift_R:
            return VK_RSHIFT; // (A1) Right SHIFT key
        case GDK_Control_L:
            return VK_LCONTROL; // (A2) Left CONTROL key
        case GDK_Control_R:
            return VK_RCONTROL; // (A3) Right CONTROL key
        case GDK_Alt_L:
            return VK_LMENU; // (A4) Left MENU key
        case GDK_Alt_R:
            return VK_RMENU; // (A5) Right MENU key

            // VK_BROWSER_BACK (A6) Windows 2000/XP: Browser Back key
            // VK_BROWSER_FORWARD (A7) Windows 2000/XP: Browser Forward key
            // VK_BROWSER_REFRESH (A8) Windows 2000/XP: Browser Refresh key
            // VK_BROWSER_STOP (A9) Windows 2000/XP: Browser Stop key
            // VK_BROWSER_SEARCH (AA) Windows 2000/XP: Browser Search key
            // VK_BROWSER_FAVORITES (AB) Windows 2000/XP: Browser Favorites key
            // VK_BROWSER_HOME (AC) Windows 2000/XP: Browser Start and Home key
            // VK_VOLUME_MUTE (AD) Windows 2000/XP: Volume Mute key
            // VK_VOLUME_DOWN (AE) Windows 2000/XP: Volume Down key
            // VK_VOLUME_UP (AF) Windows 2000/XP: Volume Up key
            // VK_MEDIA_NEXT_TRACK (B0) Windows 2000/XP: Next Track key
            // VK_MEDIA_PREV_TRACK (B1) Windows 2000/XP: Previous Track key
            // VK_MEDIA_STOP (B2) Windows 2000/XP: Stop Media key
            // VK_MEDIA_PLAY_PAUSE (B3) Windows 2000/XP: Play/Pause Media key
            // VK_LAUNCH_MAIL (B4) Windows 2000/XP: Start Mail key
            // VK_LAUNCH_MEDIA_SELECT (B5) Windows 2000/XP: Select Media key
            // VK_LAUNCH_APP1 (B6) Windows 2000/XP: Start Application 1 key
            // VK_LAUNCH_APP2 (B7) Windows 2000/XP: Start Application 2 key

            // VK_OEM_1 (BA) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ';:' key
        case GDK_semicolon:
        case GDK_colon:
            return VK_OEM_1; //case ';': case ':': return 0xBA;
            // VK_OEM_PLUS (BB) Windows 2000/XP: For any country/region, the '+' key
        case GDK_plus:
        case GDK_equal:
            return VK_OEM_PLUS; //case '=': case '+': return 0xBB;
            // VK_OEM_COMMA (BC) Windows 2000/XP: For any country/region, the ',' key
        case GDK_comma:
        case GDK_less:
            return VK_OEM_COMMA; //case ',': case '<': return 0xBC;
            // VK_OEM_MINUS (BD) Windows 2000/XP: For any country/region, the '-' key
        case GDK_minus:
        case GDK_underscore:
            return VK_OEM_MINUS; //case '-': case '_': return 0xBD;
            // VK_OEM_PERIOD (BE) Windows 2000/XP: For any country/region, the '.' key
        case GDK_period:
        case GDK_greater:
            return VK_OEM_PERIOD; //case '.': case '>': return 0xBE;
            // VK_OEM_2 (BF) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '/?' key
        case GDK_slash:
        case GDK_question:
            return VK_OEM_2; //case '/': case '?': return 0xBF;
            // VK_OEM_3 (C0) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '`~' key
        case GDK_asciitilde:
        case GDK_quoteleft:
            return VK_OEM_3; //case '`': case '~': return 0xC0;
            // VK_OEM_4 (DB) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '[{' key
        case GDK_bracketleft:
        case GDK_braceleft:
            return VK_OEM_4; //case '[': case '{': return 0xDB;
            // VK_OEM_5 (DC) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '\|' key
        case GDK_backslash:
        case GDK_bar:
            return VK_OEM_5; //case '\\': case '|': return 0xDC;
            // VK_OEM_6 (DD) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ']}' key
        case GDK_bracketright:
        case GDK_braceright:
            return VK_OEM_6; // case ']': case '}': return 0xDD;
            // VK_OEM_7 (DE) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
        case GDK_quoteright:
        case GDK_quotedbl:
            return VK_OEM_7; // case '\'': case '"': return 0xDE;
            // VK_OEM_8 (DF) Used for miscellaneous characters; it can vary by keyboard.
            // VK_OEM_102 (E2) Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard
            // VK_PROCESSKEY (E5) Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key
            // VK_PACKET (E7) Windows 2000/XP: Used to pass Unicode characters as if they were keystrokes. The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT,SendInput, WM_KEYDOWN, and WM_KEYUP
            // VK_ATTN (F6) Attn key
            // VK_CRSEL (F7) CrSel key
            // VK_EXSEL (F8) ExSel key
            // VK_EREOF (F9) Erase EOF key
            // VK_PLAY (FA) Play key
            // VK_ZOOM (FB) Zoom key
            // VK_NONAME (FC) Reserved for future use
            // VK_PA1 (FD) PA1 key
            // VK_OEM_CLEAR (FE) Clear key
        case GDK_F1:
        case GDK_F2:
        case GDK_F3:
        case GDK_F4:
        case GDK_F5:
        case GDK_F6:
        case GDK_F7:
        case GDK_F8:
        case GDK_F9:
        case GDK_F10:
        case GDK_F11:
        case GDK_F12:
        case GDK_F13:
        case GDK_F14:
        case GDK_F15:
        case GDK_F16:
        case GDK_F17:
        case GDK_F18:
        case GDK_F19:
        case GDK_F20:
        case GDK_F21:
        case GDK_F22:
        case GDK_F23:
        case GDK_F24:
            return VK_F1 + (keycode - GDK_F1);
        case GDK_KEY_VoidSymbol:
            return VK_PROCESSKEY;
        default:
            return 0;
    }

}

String PlatformKeyboardEvent::singleCharacterString(unsigned val)
{
    switch (val) {
        case GDK_ISO_Enter:
        case GDK_KP_Enter:
        case GDK_Return:
            return String("\r");
        case GDK_BackSpace:
            return String("\x8");
        case GDK_Tab:
            return String("\t");
        default:
            gunichar c = gdk_keyval_to_unicode(val);
            glong nwc;
            gunichar2* uchar16 = g_ucs4_to_utf16(&c, 1, 0, &nwc, 0);

            String retVal;
            if (uchar16)
                retVal = String((UChar*)uchar16, nwc);
            else
                retVal = String();

            g_free(uchar16);

            return retVal;
    }
}

static PlatformEvent::Type eventTypeForGdkKeyEvent(GdkEventKey* event)
{
    return event->type == GDK_KEY_RELEASE ? PlatformEvent::KeyUp : PlatformEvent::KeyDown;
}

static OptionSet<PlatformEvent::Modifier> modifiersForGdkKeyEvent(GdkEventKey* event)
{
    OptionSet<PlatformEvent::Modifier> modifiers;
    if (event->state & GDK_SHIFT_MASK || event->keyval == GDK_3270_BackTab)
        modifiers |= PlatformEvent::Modifier::ShiftKey;
    if (event->state & GDK_CONTROL_MASK)
        modifiers |= PlatformEvent::Modifier::CtrlKey;
    if (event->state & GDK_MOD1_MASK)
        modifiers |= PlatformEvent::Modifier::AltKey;
    if (event->state & GDK_META_MASK)
        modifiers |= PlatformEvent::Modifier::MetaKey;
    if (event->state & GDK_LOCK_MASK)
        modifiers |= PlatformEvent::Modifier::CapsLockKey;
    return modifiers;
}

// Keep this in sync with the other platform event constructors
PlatformKeyboardEvent::PlatformKeyboardEvent(GdkEventKey* event, const CompositionResults& compositionResults)
    : PlatformEvent(eventTypeForGdkKeyEvent(event), modifiersForGdkKeyEvent(event), currentTime())
    , m_text(compositionResults.simpleString.length() ? compositionResults.simpleString : singleCharacterString(event->keyval))
    , m_unmodifiedText(m_text)
    , m_key(keyValueForGdkKeyCode(event->keyval))
    , m_code(keyCodeForHardwareKeyCode(event->hardware_keycode))
    , m_keyIdentifier(keyIdentifierForGdkKeyCode(event->keyval))
    , m_windowsVirtualKeyCode(windowsKeyCodeForGdkKeyCode(event->keyval))
    , m_handledByInputMethod(false)
    , m_autoRepeat(false)
    , m_isKeypad(event->keyval >= GDK_KP_Space && event->keyval <= GDK_KP_9)
    , m_isSystemKey(false)
    , m_gdkEventKey(event)
    , m_compositionResults(compositionResults)
{
    // To match the behavior of IE, we return VK_PROCESSKEY for keys that triggered composition results.
    if (compositionResults.compositionUpdated())
        m_windowsVirtualKeyCode = VK_PROCESSKEY;
}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool backwardCompatibilityMode)
{
    // Can only change type from KeyDown to RawKeyDown or Char, as we lack information for other conversions.
    ASSERT(m_type == KeyDown);
    m_type = type;

    if (backwardCompatibilityMode)
        return;

    if (type == PlatformEvent::RawKeyDown) {
        m_text = String();
        m_unmodifiedText = String();
    } else if (type == PlatformEvent::Char && m_compositionResults.compositionUpdated()) {
        // Having empty text, prevents this Char (which is a DOM keypress) event
        // from going to the DOM. Keys that trigger composition events should not
        // fire keypress.
        m_text = String();
        m_unmodifiedText = String();
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    return gdk_keymap_get_caps_lock_state(gdk_keymap_get_default());
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
    GdkModifierType state;
    gtk_get_current_event_state(&state);

    shiftKey = state & GDK_SHIFT_MASK;
    ctrlKey = state & GDK_CONTROL_MASK;
    altKey = state & GDK_MOD1_MASK;
    metaKey = state & GDK_META_MASK;
}

bool PlatformKeyboardEvent::modifiersContainCapsLock(unsigned modifier)
{
    if (!(modifier & GDK_LOCK_MASK))
        return false;

    // In X11 GDK_LOCK_MASK could be CapsLock or ShiftLock, depending on the modifier mapping of the X server.
    // What GTK+ does in the X11 backend is checking if there is a key bound to GDK_KEY_Caps_Lock, so we do
    // the same here. This will also return true in Wayland if there's a caps lock key, so it's not worth it
    // checking the actual display here.
    static bool lockMaskIsCapsLock = false;
    static bool initialized = false;
    if (!initialized) {
        GUniqueOutPtr<GdkKeymapKey> keys;
        int entriesCount;
        lockMaskIsCapsLock = gdk_keymap_get_entries_for_keyval(gdk_keymap_get_default(), GDK_KEY_Caps_Lock, &keys.outPtr(), &entriesCount) && entriesCount;
    }
    return lockMaskIsCapsLock;
}

}
