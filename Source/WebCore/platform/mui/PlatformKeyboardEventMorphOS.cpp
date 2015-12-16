/*
 * Copyright (C) 2009-2013 Fabien Coeurjoly.
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
 *     documentation and/or other materials http://gaara-fr.com/index2.phpprovided with the distribution.
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
#include "PlatformKeyboardEvent.h"

#include "Logging.h"
#include "KeyboardCodes.h"
#include "BALBase.h"
#include "TextEncoding.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#include <clib/debug_protos.h>
#include <proto/keymap.h>
#include <proto/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <intuition/intuition.h>
#include <devices/keymap.h>
#include <devices/rawkeycodes.h>
#undef String

#define D(x)

#define WCHAR wchar_t

#if OS(AROS)
#include "utils.h"

#include <proto/console.h>

struct Device *ConsoleDevice;
struct IOStdReq cioreq;


static __attribute__((constructor)) void unicode_map_init(void)
{
    cioreq.io_Message.mn_Length = sizeof(struct IOStdReq);
    OpenDevice ("console.device", -1, (struct IORequest *)&cioreq, 0);
    ConsoleDevice = cioreq.io_Device;
}

static __attribute__((destructor)) void unicode_map_exit(void)
{
    if (ConsoleDevice)
        CloseDevice ((struct IORequest *)&cioreq);
}
#endif

namespace WebCore {

static bool prev_AltKey;
static bool prev_ShiftKey;
static bool prev_CtrlKey;
static bool prev_MetaKey;

static int ConvertAmigaKeyToVirtualKey(struct IntuiMessage *im)
{
	if (IDCMP_RAWKEY == im->Class)
	{
		switch (im->Code & ~IECODE_UP_PREFIX)
		{
	        case RAWKEY_TAB:       return VK_TAB;     break;
	        case RAWKEY_INSERT:    return VK_INSERT;  break;
	        case RAWKEY_PAGEUP:    return VK_PRIOR;   break;
	        case RAWKEY_PAGEDOWN:  return VK_NEXT;    break;
	        case RAWKEY_F11:       return VK_F11;     break;
	        case RAWKEY_UP:        return VK_UP;      break;
	        case RAWKEY_DOWN:      return VK_DOWN;    break;
	        case RAWKEY_RIGHT:     return VK_RIGHT;   break;
	        case RAWKEY_LEFT:      return VK_LEFT;    break;
	        case RAWKEY_F1:        return VK_F1;      break;
	        case RAWKEY_F2:        return VK_F2;      break;
	        case RAWKEY_F3:        return VK_F3;      break;
	        case RAWKEY_F4:        return VK_F4;      break;
	        case RAWKEY_F5:        return VK_F5;      break;
	        case RAWKEY_F6:        return VK_F6;      break;
	        case RAWKEY_F7:        return VK_F7;      break;
	        case RAWKEY_F8:        return VK_F8;      break;
	        case RAWKEY_F9:        return VK_F9;      break;
	        case RAWKEY_F10:       return VK_F10;     break;
	        case RAWKEY_HELP:      return VK_HELP;    break;
	        case RAWKEY_LSHIFT:    return VK_SHIFT;   break;
	        case RAWKEY_RSHIFT:    return VK_SHIFT;   break;
	        case RAWKEY_CAPSLOCK:  return VK_CAPITAL; break;
	        case RAWKEY_LCONTROL:  return VK_CONTROL; break;
	        case RAWKEY_LALT:      return VK_MENU;    break;
	        case RAWKEY_RALT:      return (im->Code & IECODE_UP_PREFIX )? VK_MENU : VK_CONTROL; break;
			case RAWKEY_LAMIGA:    return VK_LWIN;    break;
			case RAWKEY_RAMIGA:    return VK_RWIN;    break;
	        case RAWKEY_PRTSCREEN: return VK_PRINT;   break;
	        case RAWKEY_PAUSE:     return VK_PAUSE;   break;
	        case RAWKEY_F12:       return VK_F12;     break;
	        case RAWKEY_HOME:      return VK_HOME;    break;
	        case RAWKEY_END:       return VK_END;     break;
			case RAWKEY_NUMLOCK:   return VK_NUMLOCK; break;
			case RAWKEY_SCRLOCK:   return VK_SCROLL;  break;
			case RAWKEY_BACKSPACE: return VK_BACK;    break;
			case RAWKEY_RETURN:    return VK_RETURN;  break;
			case RAWKEY_ESCAPE:    return VK_ESCAPE;  break;
			case RAWKEY_SPACE:     return VK_SPACE;   break;
			case RAWKEY_DELETE:    return VK_DELETE;  break;

	        default:
			{
				struct InputEvent ie;
				char c = '?';

				ie.ie_NextEvent = NULL;
				ie.ie_Class = IECLASS_RAWKEY;
				ie.ie_SubClass = 0;
				ie.ie_Code = im->Code & ~IECODE_UP_PREFIX;
				ie.ie_Qualifier = 0x8000; //im->Qualifier & ~(IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND);
				//ie.ie_Qualifier = im->Qualifier & ~(IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND);
				ie.ie_EventAddress = (APTR *) *((ULONG *)im->IAddress);

				if (MapRawKey(&ie, (STRPTR)&c, 1, NULL) == 1)
				{
					switch(c)
					{
						case 'a':
						case 'A':
							return VK_A;
						case 'b':
						case 'B':
							return VK_B;
						case 'c':
						case 'C':
							return VK_C;
						case 'd':
						case 'D':
							return VK_D;
						case 'e':
						case 'E':
							return VK_E;
						case 'f':
						case 'F':
							return VK_F;
						case 'g':
						case 'G':
							return VK_G;
						case 'h':
						case 'H':
							return VK_H;
						case 'i':
						case 'I':
							return VK_I;
						case 'j':
						case 'J':
							return VK_J;
						case 'k':
						case 'K':
							return VK_K;
						case 'l':
						case 'L':
							return VK_L;
						case 'm':
						case 'M':
							return VK_M;
						case 'n':
						case 'N':
							return VK_N;
						case 'o':
						case 'O':
							return VK_O;
						case 'p':
						case 'P':
							return VK_P;
						case 'q':
						case 'Q':
							return VK_Q;
						case 'r':
						case 'R':
							return VK_R;
						case 's':
						case 'S':
							return VK_S;
						case 't':
						case 'T':
							return VK_T;
						case 'u':
						case 'U':
							return VK_U;
						case 'v':
						case 'V':
							return VK_V;
						case 'w':
						case 'W':
							return VK_W;
						case 'x':
						case 'X':
							return VK_X;
						case 'y':
						case 'Y':
							return VK_Y;
						case 'z':
						case 'Z':
							return VK_Z;
						case ')':
						case '0':
							return VK_0;
						case '!':
						case '1':
							return VK_1;
						case '@':
						case '2':
							return VK_2;
						case '#':
						case '3':
							return VK_3;
						case '$':
						case '4':
							return VK_4;
						case '%':
						case '5':
							return VK_5;
						case '^':
						case '6':
							return VK_6;
						case '&':
						case '7':
							return VK_7;
						case '*':
						case '8':
							return VK_8;
						case '(':
						case '9':
							return VK_9;

						case ':':
						case ';':
							return VK_OEM_1;
						case '=':
						case '+':
							return VK_OEM_PLUS;
						case ',':
						case '<':
							return VK_OEM_COMMA;
						case '-':
						case '_':
							return VK_OEM_MINUS;
						case '.':
						case '>':
							return VK_OEM_PERIOD;
						case '/':
						case '?':
							return VK_OEM_2;
						case '`':
						case '~':
							return VK_OEM_3;
						case '[':
						case '{':
							return VK_OEM_4;
						case '\\':
						case '|':
							return VK_OEM_5;
						case ']':
						case '}':
							return VK_OEM_6;
						case '\'':
						case '"':
							return VK_OEM_7;
						default:
							return 0;
					}
				}
			}
	        break;
        }
	}
	return 0;
}

static String keyIdentifierForAmigaKeyCode(struct IntuiMessage *im)
{
	if (IDCMP_RAWKEY == im->Class)
	{
		switch (im->Code & ~IECODE_UP_PREFIX)
		{
	        case RAWKEY_INSERT:    return "Insert";   break;
	        case RAWKEY_PAGEUP:    return "PageUp";   break;
	        case RAWKEY_PAGEDOWN:  return "PageDown"; break;
	        case RAWKEY_F11:       return "F11";      break;
	        case RAWKEY_UP:        return "Up";       break;
	        case RAWKEY_DOWN:      return "Down";     break;
	        case RAWKEY_RIGHT:     return "Right";    break;
	        case RAWKEY_LEFT:      return "Left";     break;
	        case RAWKEY_F1:        return "F1";       break;
	        case RAWKEY_F2:        return "F2";       break;
	        case RAWKEY_F3:        return "F3";       break;
	        case RAWKEY_F4:        return "F4";       break;
	        case RAWKEY_F5:        return "F5";       break;
	        case RAWKEY_F6:        return "F6";       break;
	        case RAWKEY_F7:        return "F7";       break;
	        case RAWKEY_F8:        return "F8";       break;
	        case RAWKEY_F9:        return "F9";       break;
	        case RAWKEY_F10:       return "F10";      break;
	        case RAWKEY_HELP:      return "Help";     break;
	        case RAWKEY_LALT:      return "Alt";      break;
	        case RAWKEY_RALT:      return "AltGr";    break;
	        case RAWKEY_PAUSE:     return "Pause";    break;
	        case RAWKEY_F12:       return "F12";      break;
	        case RAWKEY_HOME:      return "Home";     break;
	        case RAWKEY_END:       return "End";      break;
			case RAWKEY_PRTSCREEN: return "PrintScreen"; break;
			case RAWKEY_KP_ENTER:
			case RAWKEY_RETURN:    return "Enter";    break;
			case RAWKEY_DELETE:    return "U+007F";   break;
			case RAWKEY_BACKSPACE: return "U+0008";   break;
			case RAWKEY_TAB:       return "U+0009";   break;
			case RAWKEY_SPACE:     return "U+0020";   break;

		default:
			{
				struct InputEvent ie;
				TEXT c = '?';

				ie.ie_NextEvent = NULL;
				ie.ie_Class = IECLASS_RAWKEY;
				ie.ie_SubClass = 0;
				ie.ie_Code = im->Code & ~IECODE_UP_PREFIX;
				ie.ie_Qualifier = 0x8000; //im->Qualifier & ~(IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND);
				//ie.ie_Qualifier = im->Qualifier & ~(IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND);
				ie.ie_EventAddress = (APTR *) *((ULONG *)im->IAddress);

				if (MapRawKey(&ie, (STRPTR)&c, 1, NULL) == 1)
				{
					bool bMeta = im->Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND);

					if(bMeta && c == 'a')
						return "U+0001";
					else if(bMeta && c == 'c')
						return "U+0003";
					else if(bMeta && c == 'v')
						return "U+0016";
					else if(bMeta && c == 'x')
						return "U+0018";
					else if(c == 0xA4) // Euro
						return "U+20AC";
					else
						return String::format("U+%04X", u_toupper((UChar) c));
				}	 
			}
        }
	}

	return "U+0000";
}

PlatformKeyboardEvent::PlatformKeyboardEvent(BalEventKey* event)
	: m_windowsVirtualKeyCode(ConvertAmigaKeyToVirtualKey(event))
	, m_autoRepeat(false)
    , m_isKeypad(event->Qualifier & IEQUALIFIER_NUMERICPAD)
    , m_balEventKey(event)
{
	m_type = PlatformEvent::KeyDown;

	m_modifiers = 0;

	if (event->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
		m_modifiers |= ShiftKey;

	if (event->Qualifier & IEQUALIFIER_CONTROL)
		m_modifiers |= CtrlKey;

	if( event->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT))
		m_modifiers |= AltKey;

        if( event->Qualifier & IEQUALIFIER_RALT)
	        m_modifiers |= CtrlKey;

	if (event->Qualifier & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND))
		m_modifiers |= MetaKey;

#if OS(MORPHOS)
	UChar aSrc[2];
	if (IDCMP_RAWKEY == event->Class)
	{
        if (event->Code & IECODE_UP_PREFIX)
            m_type = PlatformEvent::KeyUp;

		switch (event->Code & ~IECODE_UP_PREFIX)
		{
	        case RAWKEY_TAB:
	        case RAWKEY_INSERT:
	        case RAWKEY_PAGEUP:
	        case RAWKEY_PAGEDOWN:
	        case RAWKEY_F11:
	        case RAWKEY_UP:
	        case RAWKEY_DOWN:
	        case RAWKEY_RIGHT:
	        case RAWKEY_LEFT:
	        case RAWKEY_F1:
	        case RAWKEY_F2:
	        case RAWKEY_F3:
	        case RAWKEY_F4:
	        case RAWKEY_F5:
	        case RAWKEY_F6:
	        case RAWKEY_F7:
	        case RAWKEY_F8:
	        case RAWKEY_F9:
	        case RAWKEY_F10:
	        case RAWKEY_HELP:
	        case RAWKEY_LALT:
	        case RAWKEY_RALT:
	        case RAWKEY_PAUSE:
	        case RAWKEY_F12:
	        case RAWKEY_HOME:
		case RAWKEY_END:
			aSrc[0] = 0; break;

			default:
			{
				if(is_morphos2())
				{
					struct InputEvent ie;
					WCHAR c = (WCHAR)'?';

					ie.ie_NextEvent = NULL;
					ie.ie_Class = IECLASS_RAWKEY;
					ie.ie_SubClass = 0;
					ie.ie_Code = event->Code & ~IECODE_UP_PREFIX;
					ie.ie_Qualifier = event->Qualifier;
					ie.ie_EventAddress = (APTR *) *((ULONG *)event->IAddress);

					if (MapRawKeyUCS4(&ie, (WSTRPTR *)&c, 1, NULL) == 1)
					{
						if(metaKey())
						{
							if(c == L'a')
								c = 1;
							else if(c == L'c')
								c = 3;
							else if(c == L'v')
								c = 16;
							else if(c == L'x')
								c = 18;
						}

						if(c == 0xA4) c = 0x20AC; // Euro

						aSrc[0] = (UChar) c;
					}
					else
					{
	                    aSrc[0] = 0;
					}				 
				}
				else
				{
					struct InputEvent ie;
					char c = '?';

					ie.ie_NextEvent = NULL;
					ie.ie_Class = IECLASS_RAWKEY;
					ie.ie_SubClass = 0;
					ie.ie_Code = event->Code & ~IECODE_UP_PREFIX;
					ie.ie_Qualifier = event->Qualifier;
					ie.ie_EventAddress = (APTR *) *((ULONG *)event->IAddress);

					if (MapRawKey(&ie, (STRPTR)&c, 1, NULL) == 1)
					{
						if(metaKey())
						{
							if(c == 'a')
								c = 1;
							else if(c == 'c')
								c = 3;
							else if(c == 'v')
								c = 16;
							else if(c == 'x')
								c = 18;
						}

						aSrc[0] = (UChar) c;
					}
					else
					{
	                    aSrc[0] = 0;		
					}
				}
			}
		}
	}
    aSrc[1] = 0;

    String aText(aSrc);
    String aUnmodifiedText(aSrc);
    String aKeyIdentifier = keyIdentifierForAmigaKeyCode(event);

    m_text = aText;
    m_unmodifiedText = aUnmodifiedText;
	m_keyIdentifier  = aKeyIdentifier;
#endif

#if OS(AROS)
    char buffer[10];
    if (IDCMP_RAWKEY == event->Class) {
        int length;
        struct InputEvent ievent;
        if (event->Code & IECODE_UP_PREFIX)
            m_type = KeyUp;

        ievent.ie_Class = IECLASS_RAWKEY;
        ievent.ie_Code = event->Code;
        ievent.ie_Qualifier = event->Qualifier;
        ievent.ie_position.ie_addr = *((APTR*)event->IAddress);

        length = RawKeyConvert(&ievent, buffer, sizeof(buffer) - 1, NULL);
        if(length >= 0)
            buffer[length] = 0;
        if(length > 1)
            buffer[0] = 0;
    }

    if(buffer[0])
    {
        char *keyConverted = local_to_utf8(buffer);
        if(keyConverted)
        {
            D(bug("%2x converted to %2x%2x\n", buffer[0], keyConverted[0], keyConverted[1]));
            m_text = String::fromUTF8(keyConverted);
            m_unmodifiedText = String::fromUTF8(keyConverted);
            free(keyConverted);
        }
    }

    m_keyIdentifier = keyIdentifierForAmigaKeyCode(event);
#endif

	prev_AltKey   = altKey();
	prev_ShiftKey = shiftKey();
	prev_CtrlKey  = ctrlKey();
	prev_MetaKey  = metaKey();

	D(kprintf("Qualifier Shift %d Alt %d Ctrl %d Meta %d\n", m_shiftKey, m_altKey, m_ctrlKey, m_metaKey));
	D(kprintf("RawKey %x Qualifier %x\n", m_balEventKey->Code, m_balEventKey->Qualifier));
	D(kprintf("Text <%s>\n", m_text.latin1().data()));
	D(kprintf("Unmodified Text <%s>\n", m_unmodifiedText.latin1().data()));
	D(kprintf("VirtualKey %x\n", m_windowsVirtualKeyCode));
	D(kprintf("KeyIdentifier <%s>\n", m_keyIdentifier.latin1().data()));
	D(kprintf("\n\n\n"));

}

void PlatformKeyboardEvent::disambiguateKeyDownEvent(Type type, bool backwardCompatibilityMode)
{
    // Can only change type from KeyDown to RawKeyDown or Char, as we lack information for other conversions.
    ASSERT(m_type == PlatformEvent::KeyDown);
    m_type = type;

    if (backwardCompatibilityMode)
        return;

	if (type == PlatformEvent::RawKeyDown)
	{
        m_text = String();
        m_unmodifiedText = String();
	}
	else
	{
		if (m_text.isEmpty() && m_windowsVirtualKeyCode && m_balEventKey->Code < RAWKEY_ESCAPE)
            m_text.append(UChar(m_windowsVirtualKeyCode));

		D(kprintf("DisambiguateKeyDownEvent <%s>\n", m_text.latin1().data()));
        m_keyIdentifier = String();
        m_windowsVirtualKeyCode = 0;
    }
}

bool PlatformKeyboardEvent::currentCapsLockState()
{
	D(kprintf("currentCapsLockState\n"));
    return false;
}

BalEventKey* PlatformKeyboardEvent::balEventKey() const
{
    return m_balEventKey;
}

void PlatformKeyboardEvent::getCurrentModifierState(bool& shiftKey, bool& ctrlKey, bool& altKey, bool& metaKey)
{
	shiftKey = prev_ShiftKey;
	ctrlKey  = prev_CtrlKey;
	altKey   = prev_AltKey;
	metaKey  = prev_MetaKey;

	D(kprintf("getCurrentModifierState Shift %d Alt %d Ctrl %d Meta %d\n", shiftKey, altKey, ctrlKey, metaKey));
}

}
