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

#ifndef WTF_Encoding_h
#define WTF_Encoding_h

#include <stdlib.h>

#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/utf16.h>
#include <unicode/uidna.h>
#include <unicode/uscript.h>


namespace WTF {
    namespace Unicode {

static const char* whiteList[] = {"Common", "Inherited", "Arabic", "Armenian", "Bopomofo", "Canadian_Aboriginal", "Devanagari", "Deseret", "Gujarati", "Gurmukhi", "Hangul", "Han", "Hebrew", "Hiragana", "Katakana_Or_Hiragana", "Katakana", "Latin", "Tamil", "Thai", "Yi"};
static int whiteListSize = 20;
static uint32_t IDNScriptWhiteList[(USCRIPT_CODE_LIMIT + 31) / 32];
static bool IDNScriptWhiteListisInitialized = false;

        inline int IDNToASCII(const UChar *src, int srcLength, UChar *dest, int destCapacity, bool *error) {
            UErrorCode status = U_ZERO_ERROR;
            int length = uidna_IDNToASCII(src, srcLength, dest, destCapacity, UIDNA_ALLOW_UNASSIGNED, 0, &status);
            *error = !!U_FAILURE(status);
            return length;
        }

        inline int IDNToUnicode(const UChar *src, int srcLength, UChar *dest, int destCapacity, bool *error) {
            UErrorCode status = U_ZERO_ERROR;
            int length = uidna_IDNToUnicode(src, srcLength, dest, destCapacity, UIDNA_ALLOW_UNASSIGNED, 0, &status);
            *error = !!U_FAILURE(status);
            return length;
        }

        inline bool isLookalikeCharacter(int charCode)
        {
        // This function treats the following as unsafe, lookalike characters:
        // any non-printable character, any character considered as whitespace that isn't already converted to a space by ICU,
        // and any ignorable character.

        // We also considered the characters in Mozilla's blacklist (http://kb.mozillazine.org/Network.IDN.blacklist_chars),
        // and included all of these characters that ICU can encode.

            if (!u_isprint(charCode) || u_isUWhiteSpace(charCode) || u_hasBinaryProperty(charCode, UCHAR_DEFAULT_IGNORABLE_CODE_POINT))
                return true;

            switch (charCode) {
            case 0x00ED: /* LATIN SMALL LETTER I WITH ACUTE */
            case 0x01C3: /* LATIN LETTER RETROFLEX CLICK */
            case 0x0251: /* LATIN SMALL LETTER ALPHA */
            case 0x0261: /* LATIN SMALL LETTER SCRIPT G */
            case 0x0337: /* COMBINING SHORT SOLIDUS OVERLAY */
            case 0x0338: /* COMBINING LONG SOLIDUS OVERLAY */
            case 0x05B4: /* HEBREW POINT HIRIQ */
            case 0x05BC: /* HEBREW POINT DAGESH OR MAPIQ */
            case 0x05C3: /* HEBREW PUNCTUATION SOF PASUQ */
            case 0x05F4: /* HEBREW PUNCTUATION GERSHAYIM */
            case 0x0660: /* ARABIC INDIC DIGIT ZERO */
            case 0x06D4: /* ARABIC FULL STOP */
            case 0x06F0: /* EXTENDED ARABIC INDIC DIGIT ZERO */
            case 0x2027: /* HYPHENATION POINT */
            case 0x2039: /* SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
            case 0x203A: /* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */
            case 0x2044: /* FRACTION SLASH */
            case 0x2215: /* DIVISION SLASH */
            case 0x2216: /* SET MINUS */
            case 0x233F: /* APL FUNCTIONAL SYMBOL SLASH BAR */
            case 0x23AE: /* INTEGRAL EXTENSION */
            case 0x244A: /* OCR DOUBLE BACKSLASH */
            case 0x2571: /* BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT */
            case 0x2572: /* BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT */
            case 0x29F8: /* BIG SOLIDUS */
            case 0x29f6: /* SOLIDUS WITH OVERBAR */
            case 0x2AFB: /* TRIPLE SOLIDUS BINARY RELATION */
            case 0x2AFD: /* DOUBLE SOLIDUS OPERATOR */
            case 0x3008: /* LEFT ANGLE BRACKET */
            case 0x3014: /* LEFT TORTOISE SHELL BRACKET */
            case 0x3015: /* RIGHT TORTOISE SHELL BRACKET */
            case 0x3033: /* VERTICAL KANA REPEAT MARK UPPER HALF */
            case 0x3035: /* VERTICAL KANA REPEAT MARK LOWER HALF */
            case 0x321D: /* PARENTHESIZED KOREAN CHARACTER OJEON */
            case 0x321E: /* PARENTHESIZED KOREAN CHARACTER O HU */
            case 0x33DF: /* SQUARE A OVER M */
            case 0xFE14: /* PRESENTATION FORM FOR VERTICAL SEMICOLON */
            case 0xFE15: /* PRESENTATION FORM FOR VERTICAL EXCLAMATION MARK */
            case 0xFE3F: /* PRESENTATION FORM FOR VERTICAL LEFT ANGLE BRACKET */
            case 0xFE5D: /* SMALL LEFT TORTOISE SHELL BRACKET */
            case 0xFE5E: /* SMALL RIGHT TORTOISE SHELL BRACKET */
                return true;
            default:
                return false;
            }
        }

        inline void createIDNScriptWhiteList() {
            if (IDNScriptWhiteListisInitialized) 
                return;

            IDNScriptWhiteListisInitialized = true;
            for(int i = 0; i < whiteListSize; ++i) {
                int32_t script = u_getPropertyValueEnum(UCHAR_SCRIPT, whiteList[i]);
                if (script >= 0 && script < USCRIPT_CODE_LIMIT) {
                    size_t index = script / 32;
                    uint32_t mask = 1 << (script % 32);
                    IDNScriptWhiteList[index] |= mask;
                }
            }
        }

        inline bool allCharactersInIDNScriptWhiteList(const UChar *buffer, int32_t length) {
            createIDNScriptWhiteList();

            int32_t i = 0;
            while (i < length) {
                UChar32 c;
                U16_NEXT(buffer, i, length, c)
                UErrorCode error = U_ZERO_ERROR;
                UScriptCode script = uscript_getScript(c, &error);
                if (error != U_ZERO_ERROR) {
                    //LOG_ERROR("got ICU error while trying to look at scripts: %d", error);
                    return false;
                }
                if (script < 0) {
                    //LOG_ERROR("got negative number for script code from ICU: %d", script);
                    return false;
                }
                if (script >= USCRIPT_CODE_LIMIT) {
                    return false;
                }
                size_t index = script / 32;
                uint32_t mask = 1 << (script % 32);
                if (!(IDNScriptWhiteList[index] & mask)) {
                    return false;
                }

                if (isLookalikeCharacter(c))
                return false;
            }
            return true;
        }

    } // namespace Unicode
} // namespace WTF
#endif // WTF_Encoding_h
