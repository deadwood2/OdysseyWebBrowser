/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 * Copyright (C) 2020 Jacek Piszczek
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

#include "NotImplemented.h"
#include "TextEncoding.h"
#include "WindowsKeyboardCodes.h"
#include <wtf/HexNumber.h>
#include <wtf/ASCIICType.h>

#include <exec/types.h>
#include <devices/rawkeycodes.h>
#include <intuition/intuition.h>
#include <clib/alib_protos.h>
#include <intuition/intuimessageclass.h>
#include <proto/keymap.h>

namespace WebCore {

inline int rawkeyCodeToWindows(const ULONG rawkey, char c, const bool isUp)
{
	switch (rawkey)
	{
		case RAWKEY_TAB:       return VK_TAB;
		case RAWKEY_INSERT:    return VK_INSERT;
		case RAWKEY_PAGEUP:    return VK_PRIOR;
		case RAWKEY_PAGEDOWN:  return VK_NEXT;
		case RAWKEY_F11:       return VK_F11;
		case RAWKEY_UP:        return VK_UP;
		case RAWKEY_DOWN:      return VK_DOWN;
		case RAWKEY_RIGHT:     return VK_RIGHT;
		case RAWKEY_LEFT:      return VK_LEFT;
		case RAWKEY_F1:        return VK_F1;
		case RAWKEY_F2:        return VK_F2;
		case RAWKEY_F3:        return VK_F3;
		case RAWKEY_F4:        return VK_F4;
		case RAWKEY_F5:        return VK_F5;
		case RAWKEY_F6:        return VK_F6;
		case RAWKEY_F7:        return VK_F7;
		case RAWKEY_F8:        return VK_F8;
		case RAWKEY_F9:        return VK_F9;
		case RAWKEY_F10:       return VK_F10;
		case RAWKEY_HELP:      return VK_HELP;
		case RAWKEY_LSHIFT:    return VK_SHIFT;
		case RAWKEY_RSHIFT:    return VK_SHIFT;
		case RAWKEY_CAPSLOCK:  return VK_CAPITAL;
		case RAWKEY_LCONTROL:  return VK_CONTROL;
		case RAWKEY_LALT:      return VK_MENU;
		case RAWKEY_RALT:      return isUp ? VK_MENU : VK_CONTROL;
		case RAWKEY_LAMIGA:    return VK_LWIN;
		case RAWKEY_RAMIGA:    return VK_RWIN;
		case RAWKEY_PRTSCREEN: return VK_PRINT;
		case RAWKEY_PAUSE:     return VK_PAUSE;
		case RAWKEY_F12:       return VK_F12;
		case RAWKEY_HOME:      return VK_HOME;
		case RAWKEY_END:       return VK_END;
		case RAWKEY_NUMLOCK:   return VK_NUMLOCK;
		case RAWKEY_SCRLOCK:   return VK_SCROLL;
		case RAWKEY_BACKSPACE: return VK_BACK;
		case RAWKEY_RETURN:    return VK_RETURN;
		case RAWKEY_ESCAPE:    return VK_ESCAPE;
		case RAWKEY_SPACE:     return VK_SPACE;
		case RAWKEY_DELETE:    return VK_DELETE;
//		case RAWKEY_TILDE:     return VK_OEM_3;
		default:
			switch (c)
			{
			case 'a': return VK_A;
			case 'b': return VK_B;
			case 'c': return VK_C;
			case 'd': return VK_D;
			case 'e': return VK_E;
			case 'f': return VK_F;
			case 'g': return VK_G;
			case 'h': return VK_H;
			case 'i': return VK_I;
			case 'j': return VK_J;
			case 'k': return VK_K;
			case 'l': return VK_L;
			case 'm': return VK_M;
			case 'n': return VK_N;
			case 'o': return VK_O;
			case 'p': return VK_P;
			case 'q': return VK_Q;
			case 'r': return VK_R;
			case 's': return VK_S;
			case 't': return VK_T;
			case 'u': return VK_U;
			case 'v': return VK_V;
			case 'w': return VK_W;
			case 'x': return VK_X;
			case 'y': return VK_Y;
			case 'z': return VK_Z;
			case ')':
			case '0': return VK_0;
			case '!':
			case '1': return VK_1;
			case '@':
			case '2': return VK_2;
			case '#':
			case '3': return VK_3;
			case '$':
			case '4': return VK_4;
			case '%':
			case '5': return VK_5;
			case '^':
			case '6': return VK_6;
			case '&':
			case '7': return VK_7;
			case '*':
			case '8': return VK_8;
			case '(':
			case '9': return VK_9;
			case ':':
			case ';': return VK_OEM_1;
			case '=':
			case '+': return VK_OEM_PLUS;
			case ',':
			case '<': return VK_OEM_COMMA;
			case '-':
			case '_': return VK_OEM_MINUS;
			case '.':
			case '>': return VK_OEM_PERIOD;
			case '/':
			case '?': return VK_OEM_2;
			case '`':
			case '~': return VK_OEM_3;
			case '[':
			case '{': return VK_OEM_4;
			case '\\':
			case '|': return VK_OEM_5;
			case ']':
			case '}': return VK_OEM_6;
			case '\'':
			case '"': return VK_OEM_7;
			}
			return 0;
	}
}

static String keyIdentifierForWindowsKeyCode(unsigned short keyCode)
{
    switch (keyCode) {
        case VK_MENU:
            return "Alt";
        case VK_CONTROL:
            return "Control";
        case VK_SHIFT:
            return "Shift";
        case VK_CAPITAL:
            return "CapsLock";
        case VK_LWIN:
        case VK_RWIN:
            return "Win";
        case VK_CLEAR:
            return "Clear";
        case VK_DOWN:
            return "Down";
        // "End"
        case VK_END:
            return "End";
        // "Enter"
        case VK_RETURN:
            return "Enter";
        case VK_EXECUTE:
            return "Execute";
        case VK_F1:
            return "F1";
        case VK_F2:
            return "F2";
        case VK_F3:
            return "F3";
        case VK_F4:
            return "F4";
        case VK_F5:
            return "F5";
        case VK_F6:
            return "F6";
        case VK_F7:
            return "F7";
        case VK_F8:
            return "F8";
        case VK_F9:
            return "F9";
        case VK_F10:
            return "F11";
        case VK_F12:
            return "F12";
        case VK_F13:
            return "F13";
        case VK_F14:
            return "F14";
        case VK_F15:
            return "F15";
        case VK_F16:
            return "F16";
        case VK_F17:
            return "F17";
        case VK_F18:
            return "F18";
        case VK_F19:
            return "F19";
        case VK_F20:
            return "F20";
        case VK_F21:
            return "F21";
        case VK_F22:
            return "F22";
        case VK_F23:
            return "F23";
        case VK_F24:
            return "F24";
        case VK_HELP:
            return "Help";
        case VK_HOME:
            return "Home";
        case VK_INSERT:
            return "Insert";
        case VK_LEFT:
            return "Left";
        case VK_NEXT:
            return "PageDown";
        case VK_PRIOR:
            return "PageUp";
        case VK_PAUSE:
            return "Pause";
        case VK_SNAPSHOT:
            return "PrintScreen";
        case VK_RIGHT:
            return "Right";
        case VK_SCROLL:
            return "Scroll";
        case VK_SELECT:
            return "Select";
        case VK_UP:
            return "Up";
        // Standard says that DEL becomes U+007F.
        case VK_DELETE:
            return "U+007F";
        default:
            return makeString("U+", hex(toASCIIUpper(keyCode), 4));
    }
}

// FIXME: This is incomplete. We should change this to mirror
// more like what Firefox does, and generate these switch statements
// at build time.
// https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/code
String keyCodeForHardwareKeyCode(unsigned keyCode)
{
    switch (keyCode) {
    case RAWKEY_ESCAPE:
        return "Escape"_s;
    case RAWKEY_1:
        return "Digit1"_s;
    case RAWKEY_2:
        return "Digit2"_s;
    case RAWKEY_3:
        return "Digit3"_s;
    case RAWKEY_4:
        return "Digit4"_s;
    case RAWKEY_5:
        return "Digit5"_s;
    case RAWKEY_6:
        return "Digit6"_s;
    case RAWKEY_7:
        return "Digit7"_s;
    case RAWKEY_8:
        return "Digit8"_s;
    case RAWKEY_9:
        return "Digit9"_s;
    case RAWKEY_0:
        return "Digit0"_s;
    case RAWKEY_MINUS:
        return "Minus"_s;
    case RAWKEY_EQUAL:
        return "Equal"_s;
    case RAWKEY_BACKSPACE:
        return "Backspace"_s;
    case RAWKEY_TAB:
        return "Tab"_s;
    case RAWKEY_Q:
        return "KeyQ"_s;
    case RAWKEY_W:
        return "KeyW"_s;
    case RAWKEY_E:
        return "KeyE"_s;
    case RAWKEY_R:
        return "KeyR"_s;
    case RAWKEY_T:
        return "KeyT"_s;
    case RAWKEY_Y:
        return "KeyY"_s;
    case RAWKEY_U:
        return "KeyU"_s;
    case RAWKEY_I:
        return "KeyI"_s;
    case RAWKEY_O:
        return "KeyO"_s;
    case RAWKEY_P:
        return "KeyP"_s;
    case RAWKEY_LBRACKET:
        return "BracketLeft"_s;
    case RAWKEY_RBRACKET:
        return "BracketRight"_s;
    case RAWKEY_RETURN:
        return "Enter"_s;
    case RAWKEY_CONTROL:
        return "ControlLeft"_s;
    case RAWKEY_A:
        return "KeyA"_s;
    case RAWKEY_S:
        return "KeyS"_s;
    case RAWKEY_D:
        return "KeyD"_s;
    case RAWKEY_F:
        return "KeyF"_s;
    case RAWKEY_G:
        return "KeyG"_s;
    case RAWKEY_H:
        return "KeyH"_s;
    case RAWKEY_J:
        return "KeyJ"_s;
    case RAWKEY_K:
        return "KeyK"_s;
    case RAWKEY_L:
        return "KeyL"_s;
    case RAWKEY_SEMICOLON:
        return "Semicolon"_s;
    case RAWKEY_QUOTE:
        return "Quote"_s;
    case RAWKEY_TILDE:
        return "Backquote"_s;
    case RAWKEY_LSHIFT:
        return "ShiftLeft"_s;
    case RAWKEY_BACKSLASH:
        return "Backslash"_s;
    case RAWKEY_Z:
        return "KeyZ"_s;
    case RAWKEY_X:
        return "KeyX"_s;
    case RAWKEY_C:
        return "KeyC"_s;
    case RAWKEY_V:
        return "KeyV"_s;
    case RAWKEY_B:
        return "KeyB"_s;
    case RAWKEY_N:
        return "KeyN"_s;
    case RAWKEY_M:
        return "KeyM"_s;
    case RAWKEY_COMMA:
        return "Comma"_s;
    case RAWKEY_PERIOD:
        return "Period"_s;
    case RAWKEY_SLASH:
        return "Slash"_s;
    case RAWKEY_RSHIFT:
        return "ShiftRight"_s;
    case RAWKEY_KP_MULTIPLY:
        return "NumpadMultiply"_s;
    case RAWKEY_LALT:
        return "AltLeft"_s;
    case RAWKEY_SPACE:
        return "Space"_s;
    case RAWKEY_CAPSLOCK:
        return "CapsLock"_s;
    case RAWKEY_F1:
        return "F1"_s;
    case RAWKEY_F2:
        return "F2"_s;
    case RAWKEY_F3:
        return "F3"_s;
    case RAWKEY_F4:
        return "F4"_s;
    case RAWKEY_F5:
        return "F5"_s;
    case RAWKEY_F6:
        return "F6"_s;
    case RAWKEY_F7:
        return "F7"_s;
    case RAWKEY_F8:
        return "F8"_s;
    case RAWKEY_F9:
        return "F9"_s;
    case RAWKEY_F10:
        return "F10"_s;
    case RAWKEY_NUMLOCK:
        return "NumLock"_s;
    case RAWKEY_SCRLOCK:
        return "ScrollLock"_s;
    case RAWKEY_KP_7:
        return "Numpad7"_s;
    case RAWKEY_KP_8:
        return "Numpad8"_s;
    case RAWKEY_KP_9:
        return "Numpad9"_s;
    case RAWKEY_KP_MINUS:
        return "NumpadSubtract"_s;
    case RAWKEY_KP_4:
        return "Numpad4"_s;
    case RAWKEY_KP_5:
        return "Numpad5"_s;
    case RAWKEY_KP_6:
        return "Numpad6"_s;
    case RAWKEY_KP_PLUS:
        return "NumpadAdd"_s;
    case RAWKEY_KP_1:
        return "Numpad1"_s;
    case RAWKEY_KP_2:
        return "Numpad2"_s;
    case RAWKEY_KP_3:
        return "Numpad3"_s;
    case RAWKEY_KP_0:
        return "Numpad0"_s;
    case RAWKEY_KP_DECIMAL:
        return "NumpadDecimal"_s;
//    case 0x005E:
//        return "IntlBackslash"_s;
    case RAWKEY_F11:
        return "F11"_s;
    case RAWKEY_F12:
        return "F12"_s;
    case RAWKEY_KP_ENTER:
        return "NumpadEnter"_s;
    case RAWKEY_KP_DIVIDE:
        return "NumpadDivide"_s;
    case RAWKEY_PRTSCREEN:
        return "PrintScreen"_s;
    case RAWKEY_RALT:
        return "AltRight"_s;
    case RAWKEY_HOME:
        return "Home"_s;
    case RAWKEY_UP:
        return "ArrowUp"_s;
    case RAWKEY_PAGEUP:
        return "PageUp"_s;
    case RAWKEY_LEFT:
        return "ArrowLeft"_s;
    case RAWKEY_RIGHT:
        return "ArrowRight"_s;
    case RAWKEY_END:
        return "End"_s;
    case RAWKEY_DOWN:
        return "ArrowDown"_s;
    case RAWKEY_PAGEDOWN:
        return "PageDown"_s;
    case RAWKEY_INSERT:
        return "Insert"_s;
    case RAWKEY_DELETE:
        return "Delete"_s;
#if 0
    case 0x0079:
        return "AudioVolumeMute"_s;
    case 0x007A:
        return "AudioVolumeDown"_s;
    case 0x007B:
        return "AudioVolumeUp"_s;
#endif
    case RAWKEY_PAUSE:
        return "Pause"_s;
    default:
        break;
    }
    return "Unidentified"_s;
}

static PlatformEvent::Type eventTypeForIntuiMessage(struct IntuiMessage *imsg)
{
    return (imsg->Code & IECODE_UP_PREFIX) ? PlatformEvent::KeyUp : PlatformEvent::KeyDown;
}

static OptionSet<PlatformEvent::Modifier> modifiersForIntuiMessage(struct IntuiMessage *imsg)
{
    OptionSet<PlatformEvent::Modifier> modifiers;
    if (imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))
        modifiers.add(PlatformEvent::Modifier::ShiftKey);
    if (imsg->Qualifier & (IEQUALIFIER_CONTROL))
        modifiers.add(PlatformEvent::Modifier::ControlKey);
    if (imsg->Qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT))
        modifiers.add(PlatformEvent::Modifier::AltKey);
    if (imsg->Qualifier & (IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND))
        modifiers.add(PlatformEvent::Modifier::MetaKey);
    if (imsg->Qualifier & (IEQUALIFIER_CAPSLOCK))
        modifiers.add(PlatformEvent::Modifier::CapsLockKey);
    return modifiers;
}

namespace {
	static ULONG _lastQualifiers;
}

PlatformKeyboardEvent::PlatformKeyboardEvent(struct IntuiMessage *imsg)
	: PlatformEvent(eventTypeForIntuiMessage(imsg), modifiersForIntuiMessage(imsg), WTF::WallTime::fromRawSeconds(imsg->Seconds))
    , m_autoRepeat(false)
    , m_isSystemKey(false)
	, m_intuiMessage(imsg)
{
	ULONG inputChar;
	DoMethod(reinterpret_cast<Boopsiobject *>(imsg), OM_GET, IMSGA_UCS4, &inputChar);

	UChar ch[2] = { UChar(inputChar), 0 };

	ULONG keyval = imsg->Code & ~IECODE_UP_PREFIX;
	bool isUp = !!(imsg->Code & IECODE_UP_PREFIX);

	// String containing the input
    m_text = WTF::String(ch, 1);
    m_unmodifiedText = m_text;

	struct InputEvent ie;
	char c = '\0';

	ie.ie_NextEvent = NULL;
	ie.ie_Class = IECLASS_RAWKEY;
	ie.ie_SubClass = 0;
	ie.ie_Code = keyval;
	ie.ie_Qualifier = 0;
	ie.ie_EventAddress = (APTR *) *((ULONG *)imsg->IAddress);

	if (MapRawKey(&ie, (STRPTR)&c, 1, NULL) != 1)
		c = '\0';

	// Windows VK key
    m_windowsVirtualKeyCode = rawkeyCodeToWindows(keyval, c, isUp);

	// String describing the VK key
    m_key = (inputChar >= ' ' && inputChar != 0x7F) ? m_text : keyIdentifierForWindowsKeyCode(m_windowsVirtualKeyCode);
	
	// Input 2 String
    m_code = keyCodeForHardwareKeyCode(keyval);

	// Rawkey 2 String
    m_keyIdentifier = keyIdentifierForWindowsKeyCode(m_windowsVirtualKeyCode);
    m_isKeypad = false;//keyval >= GDK_KEY_KP_Space && keyval <= GDK_KEY_KP_9;

// dead key handling needed here
//    // To match the behavior of IE, we return VK_PROCESSKEY for keys that triggered composition results.
//    if (compositionResults.compositionUpdated())
//        m_windowsVirtualKeyCode = VK_PROCESSKEY;

	_lastQualifiers = imsg->Qualifier;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
	shiftKey = !!(_lastQualifiers & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT));
	ctrlKey  = !!(_lastQualifiers & IEQUALIFIER_CONTROL);
	altKey   = !!(_lastQualifiers & (IEQUALIFIER_LALT|IEQUALIFIER_RALT));
	metaKey  = !!(_lastQualifiers & (IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND));
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
#if 0
	} else if (type == PlatformEvent::Char && m_compositionResults.compositionUpdated()) {
        // Having empty text, prevents this Char (which is a DOM keypress) event
        // from going to the DOM. Keys that trigger composition events should not
        // fire keypress.
        m_text = String();
        m_unmodifiedText = String();
#endif
    } else {
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
    return _lastQualifiers & IEQUALIFIER_CAPSLOCK;
}

}
