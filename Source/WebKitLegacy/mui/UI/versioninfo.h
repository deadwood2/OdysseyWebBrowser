#ifndef __VERSIONINFO_H__
#define __VERSIONINFO_H__

#define VERSION  "9.0"
#define WEBKITVER "605.1.15"
#define REVISION "WebKit " WEBKITVER " (webkitgtk-2.46.1+wayfarer-9)"

/* Optional label after the version, for test releases:
   -DOWB_RELEASE_LABEL=<label without spaces> at configure time. */
#ifdef OWB_RELEASE_LABEL
#define OWB_LABEL_SUFFIX " " OWB_RELEASE_LABEL
#else
#define OWB_LABEL_SUFFIX ""
#endif

#endif /* __VERSIONINFO_H__ */
