#pragma once

#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H && defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#endif

#include <stdint.h>
#include <utility>

#undef PAL_EXPORT
#undef WEBCORE_EXPORT
#undef JS_EXPORT_PRIVATE

#include <JavaScriptCore/JSExportMacros.h>
#include <WebCore/PlatformExportMacros.h>
#include <wtf/ExportMacros.h>
#include <pal/ExportMacros.h>
//#include <wtf/FeatureDefines.h>
#include <wtf/Platform.h>
#include <wtf/DisallowCType.h>

#ifdef __cplusplus

// These undefs match up with defines in WebKit2Prefix.h for Mac OS X.
// Helps us catch if anyone uses new or delete by accident in code and doesn't include "config.h".
#undef new
#undef delete
#include <wtf/FastMalloc.h>

#include <wtf/WeakPtr.h>

#endif
