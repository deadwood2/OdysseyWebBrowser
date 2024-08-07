# -----------------------------------------------------------------------------
# Add module directories
# -----------------------------------------------------------------------------
# FIXME: Port bmalloc to Windows. https://bugs.webkit.org/show_bug.cgi?id=143310
# if (NOT WIN32)
#     add_subdirectory(bmalloc)
# endif ()

add_subdirectory(WTF)

add_subdirectory(JavaScriptCore)

add_subdirectory(WebCore)

if (WIN32 AND ENABLE_GRAPHICS_CONTEXT_3D)
    add_subdirectory(ThirdParty/ANGLE)
endif ()

if (ENABLE_API_TESTS)
    add_subdirectory(ThirdParty/gtest)
endif ()

if (ENABLE_WEBKIT)
    add_subdirectory(WebKit)
endif ()

if (ENABLE_WEBKIT2)
    add_subdirectory(WebKit2)
endif ()

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

# -----------------------------------------------------------------------------
# Set compiler flags for all targets
# -----------------------------------------------------------------------------
# WEBKIT_SET_EXTRA_COMPILER_FLAGS(bmalloc ${ADDITIONAL_COMPILER_FLAGS})
WEBKIT_SET_EXTRA_COMPILER_FLAGS(WTF ${ADDITIONAL_COMPILER_FLAGS})
WEBKIT_SET_EXTRA_COMPILER_FLAGS(JavaScriptCore ${ADDITIONAL_COMPILER_FLAGS})
WEBKIT_SET_EXTRA_COMPILER_FLAGS(WebCoreTestSupport ${ADDITIONAL_COMPILER_FLAGS})
WEBKIT_SET_EXTRA_COMPILER_FLAGS(WebCore ${ADDITIONAL_COMPILER_FLAGS})

if (${PORT} STREQUAL "MUI")
    set(ADDITIONAL_COMPILER_FLAGS MUI_DISABLE_WARNINGS)
endif ()

if (ENABLE_WEBKIT)
    WEBKIT_SET_EXTRA_COMPILER_FLAGS(WebKit ${ADDITIONAL_COMPILER_FLAGS})
endif ()

if (ENABLE_WEBKIT2)
    WEBKIT_SET_EXTRA_COMPILER_FLAGS(WebKit2 ${ADDITIONAL_COMPILER_FLAGS})
endif ()
