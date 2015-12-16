/*
 * Copyright 2009 Fabien Coeurjoly <fabien.coeurjoly@wanadoo.fr>
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

#include <config.h>

#include "FileSystem.h"

#include "utils.h"
#include "owb_cat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <setjmp.h>

#include <rexx/storage.h>
#include <rexx/errors.h>
#include <libraries/locale.h>
#if OS(MORPHOS)
#include <proto/charsets.h>
#endif
#if OS(AROS)
#include <proto/codesets.h>
#endif
#include <proto/locale.h>
#include <proto/keymap.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#if OS(MORPHOS)
#include <magicaction/magicbeacon.h>
#include <proto/spellchecker.h>
#endif

using namespace WebCore;

String truncate(const String& url, unsigned int size)
{
	if (url.length() < size)
        return url;
	return url.substring(0, size) + "...";
}

ULONG strescape(CONST_STRPTR s, STRPTR out)
{
	UBYTE c;

	if (!s)
		return 0;

	if (out)
	{
		STRPTR o = out;

		while ((c = *s++))
		{
			if (c == '%')
			{
				*o++ = '%';
				*o++ = '%';
			}
			else
			{
				*o++ = c;
			}
		}

		*o = '\0';

		return o - out; /* return length of encoded string */
	}
	else
	{
		/*  If NULL was passed as second parameter then we just
		 *  calculate the length of the encoded URI. Useful to get
		 *  right size for dynamic allocations first.
		 */
		ULONG newlen = 0;

		while ((c = *s++))
		{
			if (c == '%')
			{
				newlen += 2;
			}
			else
			{
				newlen++;
			}
		}

		return newlen;
	}
}

void format_size(STRPTR s, ULONG size, QUAD n)
{
	struct Locale *l = OpenLocale(NULL);
	STRPTR point = ".";

	if (l)
	{
		point = l->loc_DecimalPoint;
		CloseLocale(l);
	}

	if (n < 1024 * 10)
	{
		snprintf(s, size, "%u %s", (unsigned int)n, GSI(MSG_CAPACITY_BYTE));
	}
	else if (n < 1024 * 1024)
	{
		snprintf(s, size, "%u%s%u %s", (unsigned int) ( n / 1024 ), point, (unsigned int)( n % 1024 * 10 / 1024 ), GSI(MSG_CAPACITY_KILOBYTE));
	}
	else if (n < 1024 * 1024 * 1024)
	{
		snprintf(s, size, "%u%s%u %s", (unsigned int) n / (1024 * 1024), point, (unsigned int)( n % (1024 * 1024) * 10 / (1024 * 1024) ), GSI(MSG_CAPACITY_MEGABYTE));
	}
	else if (n < (QUAD)1024 * 1024 * 1024 * 1024)
	{
		snprintf(s, size, "%u%s%u %s", (unsigned int) ( n / ( 1024 * 1024 * 1024 ) ), point, (unsigned int) ( n % (1024 * 1024 * 1024) * 10 / (1024 * 1024 * 1024 ) ), GSI(MSG_CAPACITY_GIGABYTE));
	}
	else
	{
		snprintf(s, size, "%u%s%u %s", (unsigned int) ( n / ( (QUAD)1024 * 1024 * 1024 * 1024 ) ), point, (unsigned int)( n % ((QUAD)1024 * 1024 * 1024 * 1024) * 10 / ((QUAD)1024 * 1024 * 1024 * 1024) ), GSI(MSG_CAPACITY_TERABYTE));
	}
}

void format_time(STRPTR buffer, ULONG size, ULONG seconds)
{
	ULONG d, h, m, s;
	char fmt[64];
	d = seconds / (3600 * 24); seconds = seconds % (3600 * 24); h = seconds / 3600; m = seconds / 60 % 60; s = seconds % 60;

	buffer[0] = 0;

	if (d > 0) // Let's hope not :)
	{
		snprintf(fmt, sizeof(fmt), GSI(((d == 1) ? MSG_PROGRESS_DAY : MSG_PROGRESS_DAYS)), d);
		strcat(buffer, fmt);
	}
	if (h > 0)
	{
		if (buffer[0]) strcat(buffer, ", ");
		snprintf(fmt, sizeof(fmt), GSI((h == 1 ? MSG_PROGRESS_HOUR : MSG_PROGRESS_HOURS)), h);
		strcat(buffer, fmt);
	}
	if (m > 0)
	{
		if (buffer[0]) strcat(buffer, ", ");
		snprintf(fmt, sizeof(fmt), GSI((m == 1 ? MSG_PROGRESS_MINUTE : MSG_PROGRESS_MINUTES)), m);
		strcat(buffer, fmt);
	}
	if (s > 0)
	{
		if (buffer[0]) strcat(buffer, ", ");
		snprintf(fmt, sizeof(fmt), GSI((s == 1 ? MSG_PROGRESS_SECOND : MSG_PROGRESS_SECONDS)), s);
		strcat(buffer, fmt);
	}
}

void format_time_compact(STRPTR buffer, ULONG size, ULONG seconds)
{
	ULONG d, h, m, s;
	char fmt[64];
	d = seconds / (3600 * 24); seconds = seconds % (3600 * 24); h = seconds / 3600; m = seconds / 60 % 60; s = seconds % 60;

	buffer[0] = 0;

	if (d > 0) // Let's hope not :)
	{
		snprintf(fmt, sizeof(fmt), GSI(((d == 1) ? MSG_PROGRESS_DAY : MSG_PROGRESS_DAYS)), d);
		strcat(buffer, fmt);
	}

	if (buffer[0]) strcat(buffer, ", ");
	snprintf(fmt, sizeof(fmt), "%.2lu", (unsigned long)h);
	strcat(buffer, fmt);

	strcat(buffer, ":");
	snprintf(fmt, sizeof(fmt), "%.2lu", (unsigned long)m);
	strcat(buffer, fmt);

	strcat(buffer, ":");
	snprintf(fmt, sizeof(fmt), "%.2lu", (unsigned long)s);
	strcat(buffer, fmt);
}

ULONG name_match(CONST_STRPTR name, CONST_STRPTR pattern)
{
	ULONG rc = FALSE;

	if (name && pattern)
	{
		TEXT statbuf[ 128 ];	/* To remove memory allocation in most cases */
		STRPTR dynbuf = NULL;
		STRPTR buf;
		ULONG len = strlen( pattern ) * 2 + 2;

		if ( len > sizeof( statbuf ) )
			buf = dynbuf = (STRPTR) malloc( len );
		else
			buf = (STRPTR) statbuf;

		if ( buf )
		{


			if (ParsePatternNoCase(pattern, (char *)buf, len) != -1)
			{
				if (MatchPatternNoCase(buf, (STRPTR) name))
				{
					rc = TRUE;
				}
			}
		}

		if ( dynbuf )
			free( dynbuf );
	}

	return (rc);
}

/*
 * Processes name (path) by quoting pattern characters.
 *
 * This is required for literal names for ParsePattern#?() and any
 * function using it (for example MatchFirst(), MatchNext()).
 */

STRPTR name_quotepattern(CONST_STRPTR path, STRPTR newpath, ULONG buflen)
{
	CONST_STRPTR a;
	STRPTR b;
	UBYTE c;

	/* already have buffer? */

	if ( !newpath || !buflen )
	{
		/* count specials to know amount of needed space */

		a = path;
		buflen = 1; /* space for terminating 0 */

		while ( ( c = *a++ ) )
		{
			buflen++;

			switch (c)
			{
				case '*':
				case '~':
				case '[':
				case ']':
				case '#':
				case '?':
				case '(':
				case ')':
				case '|':
					buflen++;
					break;
			}
		}

		newpath = (STRPTR) malloc( buflen );
		if ( !newpath )
		{
			return NULL;
		}
	}

	a = path;
	b = newpath;

	while ( ( c = *a++ ) )
	{
		switch (c)
		{
			case '*':
			case '~':
			case '[':
			case ']':
			case '#':
			case '?':
			case '(':
			case ')':
			case '|':
				if ( --buflen == 0 )
					goto out;

				*b++ = '\'';
				break;
		}

		if ( --buflen == 0 )
			break;

		*b++ = c;
	}
	out:

	*b = '\0';

	return newpath;
}

/* UTF8 management */

#if 0

size_t utf8_to_ucs4(const char *s_in, size_t l)
{
	size_t c = 0;
	const unsigned char *s = (const unsigned char *) s_in;

	if (!s)
		return 0;

	else if (l > 0 && *s < 0x80)
		c = *s;
	else if (l > 1 && (*s & 0xE0) == 0xC0 && (*(s+1) & 0xC0) == 0x80)
		c = ((*s & 0x1F) << 6) | (*(s+1) & 0x3F);
	else if (l > 2 && (*s & 0xF0) == 0xE0 && (*(s+1) & 0xC0) == 0x80 &&
			(*(s+2) & 0xC0) == 0x80)
		c = ((*s & 0x0F) << 12) | ((*(s+1) & 0x3F) << 6) |
							(*(s+2) & 0x3F);
	else if (l > 3 && (*s & 0xF8) == 0xF0 && (*(s+1) & 0xC0) == 0x80 &&
			(*(s+2) & 0xC0) == 0x80 && (*(s+3) & 0xC0) == 0x80)
		c = ((*s & 0x0F) << 18) | ((*(s+1) & 0x3F) << 12) |
				((*(s+2) & 0x3F) << 6) | (*(s+3) & 0x3F);
	else if (l > 4 && (*s & 0xFC) == 0xF8 && (*(s+1) & 0xC0) == 0x80 &&
			(*(s+2) & 0xC0) == 0x80 && (*(s+3) & 0xC0) == 0x80 &&
			(*(s+4) & 0xC0) == 0x80)
		c = ((*s & 0x0F) << 24) | ((*(s+1) & 0x3F) << 18) |
			((*(s+2) & 0x3F) << 12) | ((*(s+3) & 0x3F) << 6) |
			(*(s+4) & 0x3F);
	else if (l > 5 && (*s & 0xFE) == 0xFC && (*(s+1) & 0xC0) == 0x80 &&
			(*(s+2) & 0xC0) == 0x80 && (*(s+3) & 0xC0) == 0x80 &&
			(*(s+4) & 0xC0) == 0x80 && (*(s+5) & 0xC0) == 0x80)
		c = ((*s & 0x0F) << 28) | ((*(s+1) & 0x3F) << 24) |
			((*(s+2) & 0x3F) << 18) | ((*(s+3) & 0x3F) << 12) |
			((*(s+4) & 0x3F) << 6) | (*(s+5) & 0x3F);

	return c;
}

size_t utf8_length(const char *s)
{
	const char *__s = s;
	int l = 0;

	if(!s) return 0;

	while (*__s != '\0') {
		if ((*__s & 0x80) == 0x00)
			__s += 1;
		else if ((*__s & 0xE0) == 0xC0)
			__s += 2;
		else if ((*__s & 0xF0) == 0xE0)
			__s += 3;
		else if ((*__s & 0xF8) == 0xF0)
			__s += 4;
		else if ((*__s & 0xFC) == 0xF8)
			__s += 5;
		else if ((*__s & 0xFE) == 0xFC)
			__s += 6;
		l++;
	}

	return l;
}

size_t utf8_prev(const char *s, size_t o)
{
	while (o != 0 && (s[--o] & 0xC0) == 0x80)
		/* do nothing */;

	return o;
}

size_t utf8_next(const char *s, size_t l, size_t o)
{
	while (o != l && (s[++o] & 0xC0) == 0x80)
		/* do nothing */;

	return o;
}

static ULONG UTF8_GetOctets(ULONG utf32)
{
	ULONG octets = 0;

	if (utf32 > 0x10ffff)
	{
		utf32 = '?';
	}

	if (utf32)
	{
		octets = 1;

		if (utf32 > 0x7f)
		{
			octets = 2;

			if (utf32 > 0x7ff)
			{
				octets = 3;

				if (utf32 > 0xffff)
				{
					octets =  4;
				}
			}
		}
	}

	return octets;
}

char *utf8_to_local(const char *in)
{
	if(is_morphos2())
	{
		struct KeyMap *keymap;
		size_t inlen = strlen(in);
		size_t o = 0;
		char *result = (char *) malloc(inlen+1);
		char *ptr = result;

		keymap = AskKeyMapDefault();

		if(result)
		{
			while(o != inlen)
			{
				size_t onext = utf8_next(in, inlen, o);

				WCHAR c = (WCHAR) utf8_to_ucs4(in+o, onext-o+1);
				char outc = ToANSI(c, keymap);

				*ptr++ = outc;
				o = onext;
			}
			*ptr = 0;

		}
		return result;
	}
	else
	{
		return strdup(in); //XXX: unsupported
	}
}

char *local_to_utf8(const char *in)
{
	UBYTE *ret = NULL;

	if(is_morphos2())
	{
		struct KeyMap *keymap;
		ULONG length = 0;
		UBYTE c;
		UBYTE *local = (UBYTE *) in;

		keymap = AskKeyMapDefault();

		while((c = *local))
		{
			WCHAR wc;

			wc = ToUCS4(c, keymap);
			length += UTF8_GetOctets(wc);
			local++;
		}

		length++;

		ret = (UBYTE *) malloc(length);

		if(ret)
		{
			local = (UBYTE *) in;
			UBYTE *buffer = ret;

			while((c = *local))
		    {
				ULONG octets;
				WCHAR wc;

				wc = ToUCS4(c, keymap);
				octets = UTF8_GetOctets(wc);

				if(octets == 0)
					break;

				UTF8_Encode(wc, (char *) buffer);

				buffer += octets;
				local++;
			}

			*buffer = '\0';
		}
	}
	else
	{
		ret = (UBYTE *) strdup(in);
	}

	return (char *) ret;
}
#else

char *utf8_to_local(const char *in)
{
	if(!in)
	{
		return strdup("");
	}

#if OS(MORPHOS)
	if(CharsetsBase)
	{
		LONG dstmib = GetSystemCharset(NULL, 0);

		if(dstmib != MIBENUM_INVALID)
		{
			char *out = NULL;
			LONG dstlen = GetByteSize((APTR) in, -1, MIBENUM_UTF_8, dstmib);

			out	= (char *) malloc(dstlen + 1);

			if(out)
			{
				if(ConvertTagList((APTR) in, -1, (APTR) out, -1, MIBENUM_UTF_8, dstmib, NULL) != -1)
				{
					return out;
				}
				free(out);
			}
		}
	}
#endif
#if OS(AROS)
	{
        ULONG dataConvertedLength;
        STRPTR dataConverted = CodesetsUTF8ToStr(CSA_Source, (IPTR) in, CSA_DestLenPtr, (IPTR) &dataConvertedLength,
                TAG_END);

        if(dataConverted)
        {
            char * _ret = strdup(dataConverted);
            CodesetsFreeA(dataConverted, NULL);
            return _ret;
        }
	}
#endif

	return strdup(in); //XXX: fallback if anything fails
}

char *local_to_utf8(const char *in)
{
    if(!in)
    {
        return strdup("");
    }

#if OS(MORPHOS)
	if(CharsetsBase)
	{
		LONG srcmib = GetSystemCharset(NULL, 0);

		if(srcmib != MIBENUM_INVALID)
		{
			char *out = NULL;
			LONG dstlen = GetByteSize((APTR) in, -1, srcmib, MIBENUM_UTF_8);

			out	= (char *) malloc(dstlen + 1);

			if(out)
			{
				if(ConvertTagList((APTR) in, -1, (APTR) out, -1, srcmib, MIBENUM_UTF_8, NULL) != -1)
				{
					return out;
				}
				free(out);
			}
		}
	}
#endif
#if OS(AROS)
	{
	    struct codeset *utfCodeset = CodesetsFindA("UTF-8", NULL);
	    ULONG dataConvertedLength;
        STRPTR dataConverted = CodesetsConvertStr(CSA_Source, (IPTR) in, CSA_SourceLen, (IPTR) strlen(in),
                CSA_DestLenPtr, (IPTR) &dataConvertedLength, CSA_DestCodeset, (IPTR) utfCodeset, TAG_END);

        if(dataConverted)
        {
            char * _ret = strdup(dataConverted);
            CodesetsFreeA(dataConverted, NULL);
            return _ret;
        }
	}
#endif

	return strdup(in); //XXX: fallback if anything fails
}

#endif

/* Preferred Languages list */

struct countrycode
{
	STRPTR language;
	STRPTR code;
};

static struct countrycode countrycode_table[] = {
	{"dansk", "da"},
	{"deutsch", "de"},
	{"english", "en"},
	{"español", "sp"},
	{"français", "fr"},
	{"greek",    "gr"},
	{"italiano", "it"},
	{"magyar", "hu"},
	{"nederlands", "nl"},
	{"norsk", "no"},
	{"polski", "pl"},
	{"português", "pt"},
	{"suomi", "fi"},
	{"svenska", "sv"},
	{"türkiye", "tr"},
	{"czech", "cs"},
	{NULL, NULL}
};

static STRPTR getcode(STRPTR language)
{
	int i;
	for(i = 0; countrycode_table[i].language; i++)
	{
		if(!stricmp(countrycode_table[i].language, language))
		{
			return countrycode_table[i].code;
		}
	}

	return "en"; /* Just default to "en" */
}

STRPTR get_accepted_languages(STRPTR code, ULONG len)
{
	struct Locale *l = OpenLocale(NULL);
	STRPTR ret = NULL;

	if(l)
	{
		int i;
		ULONG clen = 0;
		code[0] = 0;
		ret = code;

		for(i=0; l->loc_PrefLanguages[i] && i < 10; i++)
		{
			char *ptr;

			ptr = (char *) getcode(l->loc_PrefLanguages[i]);

			if(ptr)
			{
				clen += strlen(ptr);

				if(clen < len)
					strcat((char *) code, ptr);

				if(!l->loc_PrefLanguages[i+1] || i+1 == 10)
				{
					clen += 3;

					if(clen < len)
						strcat((char *) code, ", *");
					break;
				}
				else
				{
					clen += 2;
					if(clen < len)
						strcat((char *) code, ", ");
				}
			}
		}

		CloseLocale(l);
	}

	return ret;
}

STRPTR get_language(STRPTR code, ULONG len)
{
	struct Locale *l = OpenLocale(NULL);
	STRPTR ret = NULL;

	if(l)
	{
		if(l->loc_PrefLanguages[0])
		{
			ret = code;
			stccpy(code, (char *) getcode(l->loc_PrefLanguages[0]), len);
		}

		CloseLocale(l);
	}

	if(!ret)
	{
		ret = code;
		stccpy(code, "en", len);
	}

	return ret;
}

long get_GMT_offset(void)
{
	struct Locale *l = OpenLocale(NULL);
	LONG ret = 0;

	if(l)
	{
		ret = l->loc_GMTOffset * 60;
		CloseLocale(l);
	}

	return ret;
}

long get_DST_offset(void)
{
	static long ret = -1;

	if(ret == -1)
	{
		char *dst = getenv("DST_OFFSET");
		if(dst)
		{
			ret = atol(dst);
		}
		else
		{
			ret = 0;
		}
	}

	return ret;
}

/* Rexx communication */

#define RESULT_LEN      1024
static char RESULT[RESULT_LEN];

bool rexx_send(char *hostname, char *cmd)
{
	struct MsgPort *RexxPort, *ReplyPort;
	struct RexxMsg *HostMsg, *answer;
	int result = RC_WARN;

	if (!stricmp (hostname, "COMMAND"))
		return SystemTagList(cmd,NULL);

	if ((RexxPort = (struct MsgPort *)FindPort (hostname)))
	{
		if ((ReplyPort = (struct MsgPort *)CreateMsgPort ()))
		{
			if ((HostMsg = CreateRexxMsg (ReplyPort, NULL, (UBYTE *)hostname)))
			{
				int len = strlen (cmd);
				if ((HostMsg->rm_Args[0] = (IPTR) CreateArgstring ((UBYTE *)cmd, len)))
				{
				    HostMsg->rm_Action = RXCOMM | RXFF_RESULT;
					PutMsg (RexxPort, (struct Message*)HostMsg);
				    WaitPort (ReplyPort);
					while (!(answer = (struct RexxMsg*)GetMsg (ReplyPort)));
				    result = answer->rm_Result1;
					if (result == RC_OK)
					{
						if (answer->rm_Result2)
						{
						    strncpy (RESULT,(char *)answer->rm_Result2, RESULT_LEN);
						    DeleteArgstring ((UBYTE *)answer->rm_Result2);
						}
						else
							RESULT[0] = '\0';
				    }
				    DeleteArgstring ((UBYTE *)HostMsg->rm_Args[0]);
				}
				else
					strcpy (RESULT, "Can't create argstring!");
				DeleteRexxMsg (HostMsg);
			}
			else
				strcpy (RESULT, "Can't create rexx message!");
		    DeleteMsgPort (ReplyPort);
		}
		else
			strcpy (RESULT, "Can't alloc reply port!");
	}
	else
		sprintf (RESULT, "Port \"%s\" not found!", hostname);

	return result == RC_OK;
}

char *rexx_result(void)
{
	return RESULT;
}

/* Notification */

int send_external_notification(struct external_notification *notification)
{
#if !OS(AROS)
	struct MsgPort *replyport, *port;
#endif
	ULONG result = 20; /* no port then tell user */

#if !OS(AROS)
	if( ( replyport = CreateMsgPort() ) )
	{
		struct MagicBeaconNotificationMessage mbnm;
		memset( &mbnm, 0, MBNM_MESSAGESIZE );

		mbnm.mbnm_NotificationName    = notification->type;
		mbnm.mbnm_NotificationMessage = notification->message;
		mbnm.mbnm_ReplyPort           = replyport;
		mbnm.mbnm_Length              = MBNM_MESSAGESIZE; /* again this must be set. */

		Forbid();
		if( ( port = FindPort( MBNP_NAME ) ) )
		{
			PutMsg( port, &mbnm.mbnm_Message );
			Permit();
			WaitPort( replyport );
		}
		else
		{
			Permit();
		}

		DeleteMsgPort( replyport );

		result = 0;
	}
#endif

	return( result );
}

/* Spell Checker Helpers */

static APTR g_dictionary = NULL;
static char g_dictionary_language[64];
extern struct Library * SpellCheckerBase;

APTR get_dictionary()
{
	return g_dictionary;
}

STRPTR get_dictionary_language()
{
	return (STRPTR) g_dictionary_language;
}

APTR open_dictionary(char *language)
{
#if OS(MORPHOS)
	struct Locale *l;

	if(!SpellCheckerBase) return NULL;

	if(!g_dictionary)
	{
		g_dictionary_language[0] = '\0';
	}

	l = OpenLocale(NULL);

	if(l)
	{
		if(l->loc_PrefLanguages[0])
		{
			STRPTR targetlanguage;
			STRPTR defaultlanguage = l->loc_PrefLanguages[0];

			targetlanguage = language ? language : defaultlanguage;

			if(targetlanguage && strcmp(targetlanguage, g_dictionary_language))
			{
				char dictionaryPath[256];

				stccpy(g_dictionary_language, targetlanguage, sizeof(g_dictionary_language));
				snprintf(dictionaryPath, sizeof(dictionaryPath), "LOCALE:Dictionaries/%s/base.dic", g_dictionary_language);

				if(g_dictionary)
				{
					CloseDictionary(g_dictionary);
				}

				g_dictionary = OpenDictionary(dictionaryPath, NULL);
			}
		}

		CloseLocale(l);
	}

	return g_dictionary;
#endif
#if OS(AROS)
	return NULL;
#endif
}

void close_dictionary()
{
#if OS(MORPHOS)
	if(SpellCheckerBase && g_dictionary)
	{
		CloseDictionary(g_dictionary);
		g_dictionary = NULL;
	}
#endif
	g_dictionary_language[0] = '\0';
}

Vector<String> get_available_dictionaries()
{
	Vector<String> dictionaries;

    DIR* dir = opendir("LOCALE:Dictionaries");

	if(dir)
	{
	    struct dirent* file;

		while ((file = readdir(dir)))
		{
			char *entry = local_to_utf8(file->d_name);

			if(entry)
			{
				dictionaries.append(String::fromUTF8(entry));
				free(entry);
			}
	    }

	    closedir(dir);
	}

	return dictionaries;
}

bool dictionary_can_learn()
{
	return (SpellCheckerBase->lib_Version > 50 || (SpellCheckerBase->lib_Version == 50 && SpellCheckerBase->lib_Revision >= 1));
}


/* Blanker support */

static int blanker_count = 0; /* not too useful, but it should be 0 at the end */

void enable_blanker(struct Screen *screen, ULONG enable)
{
	struct Library * ibase = (struct Library *) IntuitionBase;
	if(ibase)
	{
		if(ibase->lib_Version > 50 || (ibase->lib_Version == 50 && ibase->lib_Revision >=61))
		{
			if(enable)
			{
				blanker_count++;
			}
			else
			{
				blanker_count--;
			}

			if(!screen)
			{
				screen = LockPubScreen (NULL);
                UnlockPubScreen(NULL, screen);
			}

			if(screen)
			{
				//kprintf("%s blanker\n", enable ? "Enabling" : "Disabling" );
				SetAttrs(screen, SA_StopBlanker, enable ? FALSE : TRUE, TAG_DONE);				  
			}
		}
	}
}

/* Memory guards */

bool canAllocateMemory(long long size)
{
	return (long long) AvailMem(MEMF_LARGEST|MEMF_ANY) >= (size + 1024*1024);
}

#if OS(MORPHOS)
extern void *libnix_mempool;
extern jmp_buf bailout_env;

int morphos_crash(size_t size)
{
        char msg[1024];

	kprintf("[OWB: task %p] morphos_crash(%ld) invoked\nDumping StackFrame..\n.", FindTask(NULL), size);
	DumpTaskState(FindTask(NULL));

	if(size == 0)
	{
	    snprintf(msg, sizeof(msg), "Assertion failed.\n\nYou can either:\n - Crash: a hit will follow to dump stackframe and allocated heap memory will be freed\n - Retry: good luck with that.\n - Quit: the application should quit properly and give all memory back.");
	}
	else
	{
	    snprintf(msg, sizeof(msg), "Failed to allocate %u bytes.\n\nUnfortunately, WebKit doesn't check allocations and crashes in such situation.\n\nYou can either:\n - Crash: a hit will follow to dump stackframe and allocated heap memory will be freed\n - Retry the allocation: free some memory elsewhere first and then press retry.\n - Quit: the application should quit properly and give all memory back.", (unsigned int)size);
	}

	ULONG ret = MUI_RequestA(app, NULL, 0, "OWB Fatal Error", "*_Retry|_Quit|_Crash", msg, NULL);
	switch(ret)
	{
	    case 0:
	    {
	        if(libnix_mempool)
		    DeletePool(libnix_mempool);
	
	        *(int *)(uintptr_t)0xbbadbeef = 0;
	        ((void(*)())0)(); /* More reliable, but doesn't say BBADBEEF */

	        Wait(0);
		break;
	    }

	    case 1:
	        break;

        case 2:
            longjmp(bailout_env, -1);
		break;
	}
	
	return ret;
}
#endif
