include(platform/Cairo.cmake)
include(platform/Curl.cmake)
include(platform/ImageDecoders.cmake)
include(platform/TextureMapper.cmake)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/freetype"
    "${WEBCORE_DIR}/platform/graphics/texmap"
    "${WEBCORE_DIR}/platform/network/curl"
    "${WEBCORE_DIR}/platform/mui"
    "${WEBCORE_DIR}/platform/bal"
    "${WEBCORE_DIR}/platform/mediacapabilities"
    "${WEBKITLEGACY_DIR}/mui/UI"
    "${WEBKITLEGACY_DIR}/mui/UI/AROS/include"
)

list(APPEND WebCore_SOURCES

    loader/AdBlock.cpp

    platform/bal/ObserverServiceBookmarklet.cpp
    platform/bal/ObserverServiceData.cpp

    platform/Cursor.cpp

    platform/graphics/freetype/FontCacheFreeType.cpp
    platform/graphics/freetype/FontCustomPlatformDataFreeType.cpp
    platform/graphics/freetype/FontPlatformDataFreeType.cpp
    platform/graphics/freetype/GlyphPageTreeNodeFreeType.cpp
    platform/graphics/freetype/SimpleFontDataFreeType.cpp

    platform/graphics/ImageSource.cpp
    platform/graphics/WOFFFileFormat.cpp

    platform/linux/FileIOLinux.cpp

    platform/mui/acinerella.c
    platform/mui/ContextMenuMorphOS.cpp
    platform/mui/ContextMenuItemMorphOS.cpp
    platform/mui/CursorMorphOS.cpp
    platform/mui/DataObjectMorphOS.cpp
    platform/mui/DragControllerMorphOS.cpp
    platform/mui/DragDataMorphOS.cpp
    platform/mui/DragImageMorphOS.cpp
    platform/mui/EditorMUI.cpp
    platform/mui/EventHandlerMorphOS.cpp
    platform/mui/EventLoopMorphOS.cpp
    platform/mui/IconMorphOS.cpp
    platform/mui/ImageMorphOS.cpp
    platform/mui/IntPointMorphOS.cpp
    platform/mui/IntRectMorphOS.cpp
    platform/mui/LocalizedStringsMorphOS.cpp
    platform/mui/MainThreadSharedTimerMorphOS.cpp
    platform/mui/MediaPlayerPrivateMorphOS.cpp
    platform/mui/MIMETypeRegistryMorphOS.cpp
    platform/mui/NetworkStateNotifierMUI.cpp
    platform/mui/PasteboardMorphOS.cpp
    platform/mui/PlatformKeyboardEventMorphOS.cpp
    platform/mui/PlatformMouseEventMorphOS.cpp
    platform/mui/PlatformScreenMorphOS.cpp
    platform/mui/PlatformWheelEventMorphOS.cpp
    platform/mui/PopupMenuMorphOS.cpp
    platform/mui/RenderThemeMorphOS.cpp
    platform/mui/ScrollbarThemeMorphOS.cpp
    platform/mui/SearchPopupMenuMorphOS.cpp
    platform/mui/SSLKeyGeneratorMorphOS.cpp
    platform/mui/WidgetMorphOS.cpp

    platform/image-decoders/cairo/ImageBackingStoreCairo.cpp

    platform/network/curl/CookieDatabaseBackingStoreCurl.cpp
    platform/network/curl/CookieManagerCurl.cpp
    platform/network/curl/CookieMapCurl.cpp
    platform/network/curl/CookieParserCurl.cpp
    platform/network/curl/ParsedCookieCurl.cpp
    platform/network/HTTPParsers.cpp
    platform/network/NetworkStorageSession.cpp

    platform/PlatformStrategies.cpp

    platform/posix/FileSystemPOSIX.cpp
    platform/posix/SharedBufferPOSIX.cpp

    platform/text/Hyphenation.cpp
    platform/text/LocaleICU.cpp
    platform/text/TextCodecICU.cpp
    platform/text/TextEncodingDetectorICU.cpp
)

list(APPEND WebCore_LIBRARIES
    ${CAIRO_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${ICU_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${PNG_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    ${CAIRO_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${ICU_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

set(WebCore_FORWARDING_HEADERS_DIRECTORIES
    .
    accessibility
    bindings
    bridge
    contentextensions
    css
    dom
    editing
    fileapi
    history
    html
    inspector
    loader
    page
    platform
    plugins
    rendering
    storage
    style
    svg
    websockets
    workers
    xml

    Modules/geolocation
    Modules/indexeddb
    Modules/mediastream
    Modules/websockets

    Modules/indexeddb/client
    Modules/indexeddb/legacy
    Modules/indexeddb/server
    Modules/indexeddb/shared
    Modules/notifications
    Modules/webdatabase

    bindings/js

    bridge/c
    bridge/jsc

    css/parser

    html/forms
    html/parser
    html/shadow
    html/track

    loader/appcache
    loader/archive
    loader/cache
    loader/icon


    page/animation
    page/csp
    page/scrolling

    platform/animation
    platform/audio
    platform/graphics
    platform/mediacapabilities
    platform/mock
    platform/network
    platform/network/curl
    platform/sql
    platform/text


    platform/graphics/filters
    platform/graphics/opengl
    platform/graphics/opentype
    platform/graphics/texmap
    platform/graphics/transforms
    platform/graphics/win

    platform/mediastream/libwebrtc

    platform/text/transcoder

    rendering/line
    rendering/shapes
    rendering/style
    rendering/svg

    svg/animation
    svg/graphics
    svg/properties

    svg/graphics/filters
)

set(WebCore_FORWARDING_HEADERS_FILES
)

WEBKIT_CREATE_FORWARDING_HEADERS(WebCore DIRECTORIES ${WebCore_FORWARDING_HEADERS_DIRECTORIES} FILES ${WebCore_FORWARDING_HEADERS_FILES})

