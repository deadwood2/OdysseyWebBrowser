include(platform/Cairo.cmake)
include(platform/Curl.cmake)
include(platform/ImageDecoders.cmake)
include(platform/TextureMapper.cmake)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/JavaScriptCore"
    "${DirectX_INCLUDE_DIRS}"
    "${WEBKIT_LIBRARIES_DIR}/include"
    "${WEBKIT_LIBRARIES_DIR}/include/SQLite"
    "${WEBKIT_LIBRARIES_DIR}/include/zlib"
    "${WEBCORE_DIR}/loader/archive/cf"
    "${WEBCORE_DIR}/platform/cf"
)

list(APPEND WebCore_SOURCES
    page/win/FrameCairoWin.cpp

    platform/graphics/GLContext.cpp
    platform/graphics/PlatformDisplay.cpp

    platform/graphics/win/FontCustomPlatformDataCairo.cpp
    platform/graphics/win/FontPlatformDataCairoWin.cpp
    platform/graphics/win/GlyphPageTreeNodeCairoWin.cpp
    platform/graphics/win/GraphicsContextCairoWin.cpp
    platform/graphics/win/ImageCairoWin.cpp
    platform/graphics/win/MediaPlayerPrivateMediaFoundation.cpp
    platform/graphics/win/SimpleFontDataCairoWin.cpp

    platform/network/NetworkStorageSessionStub.cpp

    platform/text/win/LocaleWin.cpp

    platform/win/DelayLoadedModulesEnumerator.cpp
    platform/win/DragImageCairoWin.cpp
    platform/win/ImportedFunctionsEnumerator.cpp
    platform/win/ImportedModulesEnumerator.cpp
    platform/win/PEImage.cpp
)

list(APPEND WebCore_LIBRARIES
    ${DirectX_LIBRARIES}
    CFLite
    SQLite3
    comctl32
    crypt32
    iphlpapi
    libcurl_imp
    libjpeg
    libpng
    libxml2
    libxslt
    rpcrt4
    shlwapi
    usp10
    version
    winmm
    ws2_32
    zdll
)

list(APPEND WebCoreTestSupport_LIBRARIES
    ${CAIRO_LIBRARIES}
    CFLite
    shlwapi
)

list(APPEND WebCore_FORWARDING_HEADERS_DIRECTORIES
    platform/graphics/cairo

    platform/network/curl
)
