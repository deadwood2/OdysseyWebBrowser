#ifndef __OWB_LOCALE_H__
#define __OWB_LOCALE_H__

#if defined(__cplusplus)
extern "C" {
#endif

extern struct Locale *locale;

ULONG locale_init(void);
void  locale_cleanup(void);

#if defined(__cplusplus)
}
#endif

#endif 
