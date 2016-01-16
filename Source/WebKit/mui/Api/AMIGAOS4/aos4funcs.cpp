#ifndef MISSING_FUNCS_H
#define MISSING_FUNCS_H

#ifdef __cplusplus
extern "C" {
#endif



#if OS(AMIGAOS4)

#include <ctype.h>

int strcasecmp (const char * str1, const char * str2)
{
     int diff;

     /* No need to check *str2 since: a) str1 is equal str2 (both are 0),
        then *str1 will terminate the loop b) str1 and str2 are not equal
        (eg. *str2 is 0), then the diff part will be FALSE. I calculate
        the diff first since a) it's more probable that the first chars
        will be different and b) I don't need to initialize diff then. */
     while (!(diff = tolower (*str1) - tolower (*str2)) && *str1)
     {
        /* advance both strings. I do that here, since doing it in the
            check above would mean to advance the strings once too often */
        str1 ++;
        str2 ++;
     }

     /* Now return the difference. */
     return diff;
}



#endif


#endif