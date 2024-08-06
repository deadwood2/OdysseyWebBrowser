#ifndef __UTILS_H__
#define __UTILS_H__

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include "gui.h"

WTF::String truncate(const WTF::String& url, unsigned int size);
ULONG strescape(CONST_STRPTR s, STRPTR out);
void format_size(STRPTR s, ULONG size, QUAD n);
void format_time(STRPTR buffer, ULONG size, ULONG seconds);
void format_time_compact(STRPTR buffer, ULONG size, ULONG seconds);
char *utf8_to_local(const char *in);
char *local_to_utf8(const char *in);
ULONG name_match(CONST_STRPTR name, CONST_STRPTR pattern);
STRPTR get_accepted_languages(STRPTR code, ULONG len);
STRPTR get_language(STRPTR code, ULONG len);
long get_GMT_offset(void);
long get_DST_offset(void);
APTR open_dictionary(char *language);
void close_dictionary();
APTR get_dictionary();
STRPTR get_dictionary_language();
bool dictionary_can_learn();
WTF::Vector<WTF::String> get_available_dictionaries();
bool canAllocateMemory(long long size);
void enable_blanker(struct Screen *screen, ULONG enable);
bool rexx_send(char *hostname, char *cmd);
char *rexx_result(void);

struct external_notification
{
	STRPTR type;
	STRPTR message;
};

int send_external_notification(struct external_notification *notification);

#endif
