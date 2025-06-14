include(platform/Cairo.cmake)
include(platform/Curl.cmake)
include(platform/ImageDecoders.cmake)
include(platform/TextureMapper.cmake)
include(platform/FreeType.cmake)

list(APPEND WebCore_PRIVATE_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/morphos"
    "${WEBCORE_DIR}/platform/graphics/freetype"
    "${WEBCORE_DIR}/platform/graphics/texmap"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/network/curl"
    "${WEBCORE_DIR}/platform/mui"
    "${WEBCORE_DIR}/platform/bal"
    "${WEBCORE_DIR}/platform/mediacapabilities"
    "${WEBKITLEGACY_DIR}/mui/UI"
    "${WEBKITLEGACY_DIR}/mui/UI/AROS/include"
)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${DERIVED_SOURCES_DIR}/ForwardingHeaders"
)

list(APPEND WebCore_SOURCES

    loader/AdBlock.cpp

    platform/bal/ObserverServiceBookmarklet.cpp
    platform/bal/ObserverServiceData.cpp

    platform/Cursor.cpp

    platform/generic/KeyedDecoderGeneric.cpp
    platform/generic/KeyedEncoderGeneric.cpp

    platform/graphics/ImageSource.cpp
    platform/graphics/WOFFFileFormat.cpp

    platform/audio/morphos/AudioDestinationMorphOS.cpp
    platform/audio/morphos/AudioBusMorphOS.cpp
    platform/audio/morphos/AudioFileReaderMorphOS.cpp
    platform/audio/morphos/AudioDestinationOutputMorphOS.cpp
    platform/audio/morphos/FFTFrameMorphOS.cpp
    platform/graphics/morphos/acinerella.c
    platform/graphics/morphos/AcinerellaAudioDecoder.cpp
    platform/graphics/morphos/AcinerellaBuffer.cpp
    platform/graphics/morphos/AcinerellaContainer.cpp
    platform/graphics/morphos/AcinerellaDecoder.cpp
    platform/graphics/morphos/AcinerellaHLS.cpp
    platform/graphics/morphos/AcinerellaMuxer.cpp
    platform/graphics/morphos/AcinerellaPointer.cpp
    platform/graphics/morphos/AcinerellaVideoDecoder.cpp
    platform/graphics/morphos/AudioTrackPrivateMorphOS.cpp
    platform/graphics/morphos/DisplayRefreshMonitorMorphOS.cpp
    platform/graphics/morphos/MediaDescriptionMorphOS.cpp
    platform/graphics/morphos/MediaPlayerPrivateMorphOS.cpp
    platform/graphics/morphos/MediaSampleMorphOS.cpp
    platform/graphics/morphos/MediaSourceBufferPrivateMorphOS.cpp
    platform/graphics/morphos/MediaSourcePrivateMorphOS.cpp
    platform/graphics/morphos/VideoTrackPrivateMorphOS.cpp

    platform/linux/FileIOLinux.cpp

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
    platform/mui/memmem.c
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

    platform/network/mui/CurlSSLHandleMUI.cpp

    platform/PlatformStrategies.cpp

    platform/posix/SharedBufferPOSIX.cpp

    platform/text/Hyphenation.cpp
    platform/text/LocaleICU.cpp
    platform/text/TextCodecICU.cpp
    platform/text/TextEncodingDetectorICU.cpp
)

list(APPEND WebCore_PRIVATE_FRAMEWORK_HEADERS
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

