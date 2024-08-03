include(platform/GStreamer.cmake)
include(platform/ImageDecoders.cmake)
include(platform/Linux.cmake)

if (USE_TEXTURE_MAPPER)
    include(platform/TextureMapper.cmake)
endif ()

set(WebCore_OUTPUT_NAME WebCoreGTK)

list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}"
    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${JAVASCRIPTCORE_DIR}/API"
    "${JAVASCRIPTCORE_DIR}/assembler"
    "${JAVASCRIPTCORE_DIR}/bytecode"
    "${JAVASCRIPTCORE_DIR}/bytecompiler"
    "${JAVASCRIPTCORE_DIR}/dfg"
    "${JAVASCRIPTCORE_DIR}/disassembler"
    "${JAVASCRIPTCORE_DIR}/heap"
    "${JAVASCRIPTCORE_DIR}/debugger"
    "${JAVASCRIPTCORE_DIR}/interpreter"
    "${JAVASCRIPTCORE_DIR}/jit"
    "${JAVASCRIPTCORE_DIR}/llint"
    "${JAVASCRIPTCORE_DIR}/parser"
    "${JAVASCRIPTCORE_DIR}/profiler"
    "${JAVASCRIPTCORE_DIR}/runtime"
    "${JAVASCRIPTCORE_DIR}/yarr"
    "${THIRDPARTY_DIR}/ANGLE/"
    "${THIRDPARTY_DIR}/ANGLE/include/KHR"
    "${WEBCORE_DIR}/accessibility/atk"
    "${WEBCORE_DIR}/editing/atk"
    "${WEBCORE_DIR}/page/gtk"
    "${WEBCORE_DIR}/platform/cairo"
    "${WEBCORE_DIR}/platform/gamepad"
    "${WEBCORE_DIR}/platform/gamepad/deprecated"
    "${WEBCORE_DIR}/platform/gamepad/glib"
    "${WEBCORE_DIR}/platform/geoclue"
    "${WEBCORE_DIR}/platform/gtk"
    "${WEBCORE_DIR}/platform/graphics/cairo"
    "${WEBCORE_DIR}/platform/graphics/egl"
    "${WEBCORE_DIR}/platform/graphics/glx"
    "${WEBCORE_DIR}/platform/graphics/gtk"
    "${WEBCORE_DIR}/platform/graphics/freetype"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/"
    "${WEBCORE_DIR}/platform/graphics/harfbuzz/ng"
    "${WEBCORE_DIR}/platform/graphics/opengl"
    "${WEBCORE_DIR}/platform/graphics/opentype"
    "${WEBCORE_DIR}/platform/graphics/wayland"
    "${WEBCORE_DIR}/platform/graphics/x11"
    "${WEBCORE_DIR}/platform/mediastream/gtk"
    "${WEBCORE_DIR}/platform/mock/mediasource"
    "${WEBCORE_DIR}/platform/network/gtk"
    "${WEBCORE_DIR}/platform/network/soup"
    "${WEBCORE_DIR}/platform/text/gtk"
    "${WTF_DIR}"
)

list(APPEND WebCore_SOURCES
    accessibility/atk/AXObjectCacheAtk.cpp
    accessibility/atk/AccessibilityObjectAtk.cpp
    accessibility/atk/WebKitAccessibleHyperlink.cpp
    accessibility/atk/WebKitAccessibleInterfaceAction.cpp
    accessibility/atk/WebKitAccessibleInterfaceComponent.cpp
    accessibility/atk/WebKitAccessibleInterfaceDocument.cpp
    accessibility/atk/WebKitAccessibleInterfaceEditableText.cpp
    accessibility/atk/WebKitAccessibleInterfaceHyperlinkImpl.cpp
    accessibility/atk/WebKitAccessibleInterfaceHypertext.cpp
    accessibility/atk/WebKitAccessibleInterfaceImage.cpp
    accessibility/atk/WebKitAccessibleInterfaceSelection.cpp
    accessibility/atk/WebKitAccessibleInterfaceTable.cpp
    accessibility/atk/WebKitAccessibleInterfaceTableCell.cpp
    accessibility/atk/WebKitAccessibleInterfaceText.cpp
    accessibility/atk/WebKitAccessibleInterfaceValue.cpp
    accessibility/atk/WebKitAccessibleUtil.cpp
    accessibility/atk/WebKitAccessibleWrapperAtk.cpp

    editing/atk/FrameSelectionAtk.cpp

    loader/soup/CachedRawResourceSoup.cpp
    loader/soup/SubresourceLoaderSoup.cpp

    platform/KillRingNone.cpp

    platform/audio/glib/AudioBusGLib.cpp

    platform/crypto/gnutls/CryptoDigestGnuTLS.cpp

    platform/gamepad/glib/GamepadsGlib.cpp

    platform/geoclue/GeolocationProviderGeoclue1.cpp
    platform/geoclue/GeolocationProviderGeoclue2.cpp

    platform/glib/EventLoopGlib.cpp
    platform/glib/FileSystemGlib.cpp
    platform/glib/KeyedDecoderGlib.cpp
    platform/glib/KeyedEncoderGlib.cpp
    platform/glib/MainThreadSharedTimerGLib.cpp
    platform/glib/SharedBufferGlib.cpp

    platform/graphics/GLContext.cpp
    platform/graphics/GraphicsContext3DPrivate.cpp

    platform/graphics/cairo/BackingStoreBackendCairoImpl.cpp
    platform/graphics/cairo/BackingStoreBackendCairoX11.cpp
    platform/graphics/cairo/BitmapImageCairo.cpp
    platform/graphics/cairo/CairoUtilities.cpp
    platform/graphics/cairo/FloatRectCairo.cpp
    platform/graphics/cairo/FontCairo.cpp
    platform/graphics/cairo/FontCairoHarfbuzzNG.cpp
    platform/graphics/cairo/GradientCairo.cpp
    platform/graphics/cairo/GraphicsContext3DCairo.cpp
    platform/graphics/cairo/GraphicsContextCairo.cpp
    platform/graphics/cairo/ImageBufferCairo.cpp
    platform/graphics/cairo/ImageCairo.cpp
    platform/graphics/cairo/IntRectCairo.cpp
    platform/graphics/cairo/PathCairo.cpp
    platform/graphics/cairo/PatternCairo.cpp
    platform/graphics/cairo/PlatformContextCairo.cpp
    platform/graphics/cairo/PlatformPathCairo.cpp
    platform/graphics/cairo/RefPtrCairo.cpp
    platform/graphics/cairo/TransformationMatrixCairo.cpp

    platform/graphics/egl/GLContextEGL.cpp
    platform/graphics/egl/GLContextEGLWayland.cpp
    platform/graphics/egl/GLContextEGLX11.cpp

    platform/graphics/freetype/FontCacheFreeType.cpp
    platform/graphics/freetype/FontCustomPlatformDataFreeType.cpp
    platform/graphics/freetype/FontPlatformDataFreeType.cpp
    platform/graphics/freetype/GlyphPageTreeNodeFreeType.cpp
    platform/graphics/freetype/SimpleFontDataFreeType.cpp

    platform/graphics/glx/GLContextGLX.cpp

    platform/graphics/gstreamer/ImageGStreamerCairo.cpp

    platform/graphics/harfbuzz/HarfBuzzFace.cpp
    platform/graphics/harfbuzz/HarfBuzzFaceCairo.cpp
    platform/graphics/harfbuzz/HarfBuzzShaper.cpp

    platform/graphics/opengl/Extensions3DOpenGLCommon.cpp
    platform/graphics/opengl/GraphicsContext3DOpenGLCommon.cpp
    platform/graphics/opengl/TemporaryOpenGLSetting.cpp

    platform/graphics/opentype/OpenTypeVerticalData.cpp

    platform/graphics/wayland/PlatformDisplayWayland.cpp

    platform/graphics/x11/PlatformDisplayX11.cpp
    platform/graphics/x11/XErrorTrapper.cpp
    platform/graphics/x11/XUniqueResource.cpp

    platform/gtk/DragDataGtk.cpp
    platform/gtk/ErrorsGtk.cpp
    platform/gtk/MIMETypeRegistryGtk.cpp
    platform/gtk/PasteboardGtk.cpp
    platform/gtk/ScrollAnimatorGtk.cpp
    platform/gtk/SelectionData.cpp
    platform/gtk/TemporaryLinkStubs.cpp
    platform/gtk/UserAgentGtk.cpp

    platform/image-decoders/cairo/ImageDecoderCairo.cpp

    platform/mediastream/gtk/SDPProcessorScriptResourceGtk.cpp

    platform/network/gtk/CredentialBackingStore.cpp

    platform/network/soup/AuthenticationChallengeSoup.cpp
    platform/network/soup/CertificateInfo.cpp
    platform/network/soup/CookieJarSoup.cpp
    platform/network/soup/CookieStorageSoup.cpp
    platform/network/soup/CredentialStorageSoup.cpp
    platform/network/soup/DNSSoup.cpp
    platform/network/soup/GRefPtrSoup.cpp
    platform/network/soup/NetworkStorageSessionSoup.cpp
    platform/network/soup/ProxyServerSoup.cpp
    platform/network/soup/ResourceErrorSoup.cpp
    platform/network/soup/ResourceHandleSoup.cpp
    platform/network/soup/ResourceRequestSoup.cpp
    platform/network/soup/ResourceResponseSoup.cpp
    platform/network/soup/SocketStreamHandleImplSoup.cpp
    platform/network/soup/SoupNetworkSession.cpp
    platform/network/soup/SynchronousLoaderClientSoup.cpp
    platform/network/soup/WebKitSoupRequestGeneric.cpp

    platform/soup/PublicSuffixSoup.cpp
    platform/soup/SharedBufferSoup.cpp
    platform/soup/URLSoup.cpp

    platform/text/Hyphenation.cpp
    platform/text/LocaleICU.cpp

    platform/text/enchant/TextCheckerEnchant.cpp

    platform/text/hyphen/HyphenationLibHyphen.cpp

    platform/unix/LoggingUnix.cpp
)

list(APPEND WebCorePlatformGTK_SOURCES
    editing/gtk/EditorGtk.cpp

    page/gtk/DragControllerGtk.cpp
    page/gtk/EventHandlerGtk.cpp

    platform/graphics/PlatformDisplay.cpp

    platform/graphics/gtk/ColorGtk.cpp
    platform/graphics/gtk/GdkCairoUtilities.cpp
    platform/graphics/gtk/IconGtk.cpp
    platform/graphics/gtk/ImageBufferGtk.cpp
    platform/graphics/gtk/ImageGtk.cpp

    platform/gtk/CursorGtk.cpp
    platform/gtk/DragImageGtk.cpp
    platform/gtk/GRefPtrGtk.cpp
    platform/gtk/GtkUtilities.cpp
    platform/gtk/GtkVersioning.c
    platform/gtk/LocalizedStringsGtk.cpp
    platform/gtk/PasteboardHelper.cpp
    platform/gtk/PlatformKeyboardEventGtk.cpp
    platform/gtk/PlatformMouseEventGtk.cpp
    platform/gtk/PlatformPasteboardGtk.cpp
    platform/gtk/PlatformScreenGtk.cpp
    platform/gtk/PlatformWheelEventGtk.cpp
    platform/gtk/RenderThemeGadget.cpp
    platform/gtk/ScrollbarThemeGtk.cpp
    platform/gtk/SoundGtk.cpp
    platform/gtk/WidgetGtk.cpp

    rendering/RenderThemeGtk.cpp
)

if (USE_GEOCLUE2)
    list(APPEND WebCore_DERIVED_SOURCES
        ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface.c
    )
    execute_process(COMMAND pkg-config --variable dbus_interface geoclue-2.0 OUTPUT_VARIABLE GEOCLUE_DBUS_INTERFACE)
    add_custom_command(
         OUTPUT ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface.c ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface.h
         COMMAND gdbus-codegen --interface-prefix org.freedesktop.GeoClue2. --c-namespace Geoclue --generate-c-code ${DERIVED_SOURCES_WEBCORE_DIR}/Geoclue2Interface ${GEOCLUE_DBUS_INTERFACE}
    )
endif ()

list(APPEND WebCore_USER_AGENT_STYLE_SHEETS
    ${WEBCORE_DIR}/css/mediaControlsGtk.css
)

set(WebCore_USER_AGENT_SCRIPTS
    ${WEBCORE_DIR}/English.lproj/mediaControlsLocalizedStrings.js
    ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsBase.js
    ${WEBCORE_DIR}/Modules/mediacontrols/mediaControlsGtk.js
)

set(WebCore_USER_AGENT_SCRIPTS_DEPENDENCIES ${WEBCORE_DIR}/platform/gtk/RenderThemeGtk.cpp)

set(WebCore_SDP_PROCESSOR_SCRIPTS ${WEBCORE_DIR}/Modules/mediastream/sdp.js)
set(WebCore_SDP_PROCESSOR_SCRIPTS_DEPENDENCIES ${WEBCORE_DIR}/platform/mediastream/gtk/SDPProcessorScriptResourceGtk.cpp)

list(APPEND WebCore_LIBRARIES
    ${ATK_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${ENCHANT_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
    ${GEOCLUE_LIBRARIES}
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GMODULE_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GNUTLS_LIBRARIES}
    ${GUDEV_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${LIBSECRET_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${HYPHEN_LIBRARIES}
    ${SQLITE_LIBRARIES}
    ${X11_X11_LIB}
    ${X11_Xcomposite_LIB}
    ${X11_Xdamage_LIB}
    ${X11_Xrender_LIB}
    ${X11_Xt_LIB}
    ${ZLIB_LIBRARIES}
    WTF
)

list(APPEND WebCoreTestSupport_LIBRARIES WTF)

list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
    ${ATK_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${ENCHANT_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
    ${GEOCLUE_INCLUDE_DIRS}
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GNUTLS_INCLUDE_DIRS}
    ${GUDEV_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

if (USE_OPENGL_ES_2)
    list(APPEND WebCore_SOURCES
        platform/graphics/opengl/Extensions3DOpenGLES.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGLES.cpp
    )
endif ()

if (USE_OPENGL)
    list(APPEND WebCore_SOURCES
        platform/graphics/OpenGLShims.cpp

        platform/graphics/opengl/Extensions3DOpenGL.cpp
        platform/graphics/opengl/GraphicsContext3DOpenGL.cpp
    )
endif ()

if (ENABLE_PLUGIN_PROCESS_GTK2)
    # WebKitPluginProcess2 needs a version of WebCore compiled against GTK+2, so we've isolated all the GTK+
    # dependent files into a separate library which can be used to construct a GTK+2 WebCore
    # for the plugin process.
    add_library(WebCorePlatformGTK2 ${WebCore_LIBRARY_TYPE} ${WebCorePlatformGTK_SOURCES})
    add_dependencies(WebCorePlatformGTK2 WebCore)
    WEBKIT_SET_EXTRA_COMPILER_FLAGS(WebCorePlatformGTK2)
    set_property(TARGET WebCorePlatformGTK2
        APPEND
        PROPERTY COMPILE_DEFINITIONS GTK_API_VERSION_2=1
    )
    target_include_directories(WebCorePlatformGTK2 PRIVATE
        ${WebCore_INCLUDE_DIRECTORIES}
        ${GTK2_INCLUDE_DIRS}
        ${GDK2_INCLUDE_DIRS}
    )
    target_include_directories(WebCorePlatformGTK2 SYSTEM PRIVATE
        ${WebCore_SYSTEM_INCLUDE_DIRECTORIES}
    )
    target_link_libraries(WebCorePlatformGTK2
         ${WebCore_LIBRARIES}
         ${GTK2_LIBRARIES}
         ${GDK2_LIBRARIES}
    )
endif ()

if (ENABLE_WAYLAND_TARGET)
    list(APPEND WebCore_SYSTEM_INCLUDE_DIRECTORIES
        ${WAYLAND_INCLUDE_DIRS}
    )
    list(APPEND WebCore_LIBRARIES
        ${WAYLAND_LIBRARIES}
    )
endif ()

add_library(WebCorePlatformGTK ${WebCore_LIBRARY_TYPE} ${WebCorePlatformGTK_SOURCES})
add_dependencies(WebCorePlatformGTK WebCore)
WEBKIT_SET_EXTRA_COMPILER_FLAGS(WebCorePlatformGTK)
target_include_directories(WebCorePlatformGTK PRIVATE
    ${WebCore_INCLUDE_DIRECTORIES}
)
target_include_directories(WebCorePlatformGTK SYSTEM PRIVATE
    ${WebCore_SYSTEM_INCLUDE_DIRECTORIES}
    ${GTK_INCLUDE_DIRS}
    ${GDK_INCLUDE_DIRS}
)
target_link_libraries(WebCorePlatformGTK
    ${WebCore_LIBRARIES}
    ${GTK_LIBRARIES}
    ${GDK_LIBRARIES}
)

include_directories(
    ${WebCore_INCLUDE_DIRECTORIES}
    "${WEBCORE_DIR}/bindings/gobject/"
    "${DERIVED_SOURCES_DIR}"
    "${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}"
)

include_directories(SYSTEM
    ${WebCore_SYSTEM_INCLUDE_DIRECTORIES}
)

list(APPEND GObjectDOMBindings_SOURCES
    bindings/gobject/ConvertToUTF8String.cpp
    bindings/gobject/DOMObjectCache.cpp
    bindings/gobject/GObjectEventListener.cpp
    bindings/gobject/GObjectNodeFilterCondition.cpp
    bindings/gobject/GObjectXPathNSResolver.cpp
    bindings/gobject/WebKitDOMCustom.cpp
    bindings/gobject/WebKitDOMDeprecated.cpp
    bindings/gobject/WebKitDOMEventTarget.cpp
    bindings/gobject/WebKitDOMHTMLPrivate.cpp
    bindings/gobject/WebKitDOMNodeFilter.cpp
    bindings/gobject/WebKitDOMObject.cpp
    bindings/gobject/WebKitDOMPrivate.cpp
    bindings/gobject/WebKitDOMXPathNSResolver.cpp
    ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups.h
    ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups-unstable.h
    ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines.h
    ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines-unstable.h
    ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdom.h
)

list(APPEND GObjectDOMBindingsStable_IDL_FILES
    css/CSSRule.idl
    css/CSSRuleList.idl
    css/CSSStyleDeclaration.idl
    css/CSSStyleSheet.idl
    css/CSSValue.idl
    css/MediaList.idl
    css/StyleSheet.idl
    css/StyleSheetList.idl

    dom/Attr.idl
    dom/CDATASection.idl
    dom/CharacterData.idl
    dom/Comment.idl
    dom/DOMImplementation.idl
    dom/Document.idl
    dom/DocumentFragment.idl
    dom/DocumentType.idl
    dom/Element.idl
    dom/Event.idl
    dom/KeyboardEvent.idl
    dom/MouseEvent.idl
    dom/NamedNodeMap.idl
    dom/Node.idl
    dom/NodeIterator.idl
    dom/NodeList.idl
    dom/ProcessingInstruction.idl
    dom/Range.idl
    dom/Text.idl
    dom/TreeWalker.idl
    dom/UIEvent.idl
    dom/WheelEvent.idl

    fileapi/Blob.idl
    fileapi/File.idl
    fileapi/FileList.idl

    html/HTMLAnchorElement.idl
    html/HTMLAppletElement.idl
    html/HTMLAreaElement.idl
    html/HTMLBRElement.idl
    html/HTMLBaseElement.idl
    html/HTMLBodyElement.idl
    html/HTMLButtonElement.idl
    html/HTMLCanvasElement.idl
    html/HTMLCollection.idl
    html/HTMLDListElement.idl
    html/HTMLDirectoryElement.idl
    html/HTMLDivElement.idl
    html/HTMLDocument.idl
    html/HTMLElement.idl
    html/HTMLEmbedElement.idl
    html/HTMLFieldSetElement.idl
    html/HTMLFontElement.idl
    html/HTMLFormElement.idl
    html/HTMLFrameElement.idl
    html/HTMLFrameSetElement.idl
    html/HTMLHRElement.idl
    html/HTMLHeadElement.idl
    html/HTMLHeadingElement.idl
    html/HTMLHtmlElement.idl
    html/HTMLIFrameElement.idl
    html/HTMLImageElement.idl
    html/HTMLInputElement.idl
    html/HTMLLIElement.idl
    html/HTMLLabelElement.idl
    html/HTMLLegendElement.idl
    html/HTMLLinkElement.idl
    html/HTMLMapElement.idl
    html/HTMLMarqueeElement.idl
    html/HTMLMenuElement.idl
    html/HTMLMetaElement.idl
    html/HTMLModElement.idl
    html/HTMLOListElement.idl
    html/HTMLObjectElement.idl
    html/HTMLOptGroupElement.idl
    html/HTMLOptionElement.idl
    html/HTMLOptionsCollection.idl
    html/HTMLParagraphElement.idl
    html/HTMLParamElement.idl
    html/HTMLPreElement.idl
    html/HTMLQuoteElement.idl
    html/HTMLScriptElement.idl
    html/HTMLSelectElement.idl
    html/HTMLStyleElement.idl
    html/HTMLTableCaptionElement.idl
    html/HTMLTableCellElement.idl
    html/HTMLTableColElement.idl
    html/HTMLTableElement.idl
    html/HTMLTableRowElement.idl
    html/HTMLTableSectionElement.idl
    html/HTMLTextAreaElement.idl
    html/HTMLTitleElement.idl
    html/HTMLUListElement.idl

    page/DOMWindow.idl

    xml/XPathExpression.idl
    xml/XPathResult.idl
)

list(APPEND GObjectDOMBindingsUnstable_IDL_FILES
    Modules/battery/BatteryManager.idl

    Modules/gamepad/deprecated/Gamepad.idl
    Modules/gamepad/deprecated/GamepadList.idl

    Modules/geolocation/Geolocation.idl

    Modules/mediasource/VideoPlaybackQuality.idl

    Modules/mediastream/MediaDevices.idl
    Modules/mediastream/NavigatorMediaDevices.idl
    Modules/mediastream/MediaTrackSupportedConstraints.idl

    Modules/quota/StorageInfo.idl
    Modules/quota/StorageQuota.idl

    Modules/speech/DOMWindowSpeechSynthesis.idl
    Modules/speech/SpeechSynthesis.idl
    Modules/speech/SpeechSynthesisEvent.idl
    Modules/speech/SpeechSynthesisUtterance.idl
    Modules/speech/SpeechSynthesisVoice.idl

    Modules/webdatabase/Database.idl

    css/DOMCSSNamespace.idl
    css/MediaQueryList.idl
    css/StyleMedia.idl

    dom/DOMNamedFlowCollection.idl
    dom/DOMStringList.idl
    dom/DOMStringMap.idl
    dom/MessagePort.idl
    dom/Touch.idl
    dom/WebKitNamedFlow.idl

    html/DOMTokenList.idl
    html/HTMLDetailsElement.idl
    html/HTMLKeygenElement.idl
    html/HTMLMediaElement.idl
    html/MediaController.idl
    html/MediaError.idl
    html/TimeRanges.idl
    html/ValidityState.idl

    loader/appcache/DOMApplicationCache.idl

    page/BarProp.idl
    page/DOMSelection.idl
    page/History.idl
    page/Location.idl
    page/Navigator.idl
    page/Performance.idl
    page/PerformanceEntry.idl
    page/PerformanceNavigation.idl
    page/PerformanceTiming.idl
    page/Screen.idl
    page/UserMessageHandler.idl
    page/UserMessageHandlersNamespace.idl
    page/WebKitNamespace.idl
    page/WebKitPoint.idl

    plugins/DOMMimeType.idl
    plugins/DOMMimeTypeArray.idl
    plugins/DOMPlugin.idl
    plugins/DOMPluginArray.idl

    storage/Storage.idl
)

if (ENABLE_WEB_ANIMATIONS)
    list(APPEND GObjectDOMBindingsUnstable_IDL_FILES
        animation/Animatable.idl
        animation/AnimationEffect.idl
        animation/AnimationTimeline.idl
        animation/DocumentAnimation.idl
        animation/DocumentTimeline.idl
        animation/KeyframeEffect.idl
        animation/WebAnimation.idl
    )
endif ()

if (ENABLE_VIDEO OR ENABLE_WEB_AUDIO)
    list(APPEND GObjectDOMBindingsUnstable_IDL_FILES
        html/HTMLAudioElement.idl
        html/HTMLVideoElement.idl

        html/track/AudioTrack.idl
        html/track/AudioTrackList.idl
        html/track/DataCue.idl
        html/track/TextTrack.idl
        html/track/TextTrackCue.idl
        html/track/TextTrackCueList.idl
        html/track/TextTrackList.idl
        html/track/TrackEvent.idl
        html/track/VTTCue.idl
        html/track/VideoTrack.idl
        html/track/VideoTrackList.idl
    )
endif ()

if (ENABLE_QUOTA)
    list(APPEND GObjectDOMBindingsUnstable_IDL_FILES
        Modules/quota/DOMWindowQuota.idl
        Modules/quota/NavigatorStorageQuota.idl
        Modules/quota/StorageErrorCallback.idl
        Modules/quota/StorageInfo.idl
        Modules/quota/StorageQuota.idl
        Modules/quota/StorageQuotaCallback.idl
        Modules/quota/StorageUsageCallback.idl
        Modules/quota/WorkerNavigatorStorageQuota.idl
    )
endif ()

set(GObjectDOMBindings_STATIC_CLASS_LIST Custom Deprecated EventTarget NodeFilter Object XPathNSResolver)

set(GObjectDOMBindingsStable_CLASS_LIST ${GObjectDOMBindings_STATIC_CLASS_LIST})
set(GObjectDOMBindingsStable_INSTALLED_HEADERS
     ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups.h
     ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines.h
     ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdom.h
     ${WEBCORE_DIR}/bindings/gobject/WebKitDOMCustom.h
     ${WEBCORE_DIR}/bindings/gobject/WebKitDOMDeprecated.h
     ${WEBCORE_DIR}/bindings/gobject/WebKitDOMEventTarget.h
     ${WEBCORE_DIR}/bindings/gobject/WebKitDOMNodeFilter.h
     ${WEBCORE_DIR}/bindings/gobject/WebKitDOMObject.h
     ${WEBCORE_DIR}/bindings/gobject/WebKitDOMXPathNSResolver.h
)

set(GObjectDOMBindingsUnstable_INSTALLED_HEADERS
     ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups-unstable.h
     ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines-unstable.h
     ${WEBCORE_DIR}/bindings/gobject/WebKitDOMCustomUnstable.h
)

foreach (file ${GObjectDOMBindingsStable_IDL_FILES})
    get_filename_component(classname ${file} NAME_WE)
    list(APPEND GObjectDOMBindingsStable_CLASS_LIST ${classname})
    list(APPEND GObjectDOMBindingsStable_INSTALLED_HEADERS ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/WebKitDOM${classname}.h)
    list(APPEND GObjectDOMBindingsUnstable_INSTALLED_HEADERS ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/WebKitDOM${classname}Unstable.h)
endforeach ()

foreach (file ${GObjectDOMBindingsUnstable_IDL_FILES})
    get_filename_component(classname ${file} NAME_WE)
    list(APPEND GObjectDOMBindingsUnstable_CLASS_LIST ${classname})
    list(APPEND GObjectDOMBindingsUnstable_INSTALLED_HEADERS ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/WebKitDOM${classname}.h)
endforeach ()

set(GOBJECT_DOM_BINDINGS_FEATURES_DEFINES "LANGUAGE_GOBJECT=1 ${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}")
string(REPLACE "ENABLE_INDEXED_DATABASE=1" "" GOBJECT_DOM_BINDINGS_FEATURES_DEFINES ${GOBJECT_DOM_BINDINGS_FEATURES_DEFINES})
string(REPLACE REGEX "ENABLE_SVG[A-Z_]+=1" "" GOBJECT_DOM_BINDINGS_FEATURES_DEFINES ${GOBJECT_DOM_BINDINGS_FEATURES_DEFINES})

file(MAKE_DIRECTORY ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR})

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines.h
    DEPENDS ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl
    COMMAND echo ${GObjectDOMBindingsStable_CLASS_LIST} | ${PERL_EXECUTABLE} ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl defines > ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines.h
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines-unstable.h
    DEPENDS ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl
    COMMAND echo ${GObjectDOMBindingsUnstable_CLASS_LIST} | ${PERL_EXECUTABLE} ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl defines-unstable > ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomdefines-unstable.h
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdom.h
    DEPENDS ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl
    COMMAND echo ${GObjectDOMBindingsStable_CLASS_LIST} | ${PERL_EXECUTABLE} ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl gdom > ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdom.h
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups.h
    DEPENDS ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl
    COMMAND echo ${GObjectDOMBindingsStable_CLASS_LIST} | ${PERL_EXECUTABLE} ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl autocleanups > ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups.h
)

add_custom_command(
    OUTPUT ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups-unstable.h
    DEPENDS ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl
    COMMAND echo ${GObjectDOMBindingsUnstable_CLASS_LIST} | ${PERL_EXECUTABLE} ${WEBCORE_DIR}/bindings/scripts/gobject-generate-headers.pl autocleanups > ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/webkitdomautocleanups-unstable.h
)

# Some of the static headers are included by generated public headers with include <webkitdom/WebKitDOMFoo.h>.
# We need those headers in the derived sources to be in webkitdom directory.
set(GObjectDOMBindings_STATIC_HEADER_NAMES ${GObjectDOMBindings_STATIC_CLASS_LIST} CustomUnstable)
foreach (classname ${GObjectDOMBindings_STATIC_HEADER_NAMES})
    add_custom_command(
        OUTPUT ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/WebKitDOM${classname}.h
        DEPENDS ${WEBCORE_DIR}/bindings/gobject/WebKitDOM${classname}.h
        COMMAND ln -n -s -f ${WEBCORE_DIR}/bindings/gobject/WebKitDOM${classname}.h ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}
    )
    list(APPEND GObjectDOMBindings_STATIC_GENERATED_SOURCES ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/WebKitDOM${classname}.h)
endforeach ()

add_custom_target(fake-generated-webkitdom-headers
    DEPENDS ${GObjectDOMBindings_STATIC_GENERATED_SOURCES}
)

set(GObjectDOMBindings_IDL_FILES ${GObjectDOMBindingsStable_IDL_FILES} ${GObjectDOMBindingsUnstable_IDL_FILES})
set(ADDITIONAL_BINDINGS_DEPENDENCIES
    ${WEBCORE_DIR}/bindings/gobject/webkitdom.symbols
    ${WINDOW_CONSTRUCTORS_FILE}
    ${WORKERGLOBALSCOPE_CONSTRUCTORS_FILE}
    ${DEDICATEDWORKERGLOBALSCOPE_CONSTRUCTORS_FILE}
)
GENERATE_BINDINGS(GObjectDOMBindings_SOURCES
    "${GObjectDOMBindings_IDL_FILES}"
    "${WEBCORE_DIR}"
    "${IDL_INCLUDES}"
    "${GOBJECT_DOM_BINDINGS_FEATURES_DEFINES}"
    ${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}
    WebKitDOM GObject cpp
    ${IDL_ATTRIBUTES_FILE}
    ${SUPPLEMENTAL_DEPENDENCY_FILE}
    ${ADDITIONAL_BINDINGS_DEPENDENCIES})

add_definitions(-DBUILDING_WEBKIT)
add_definitions(-DWEBKIT_DOM_USE_UNSTABLE_API)

add_library(GObjectDOMBindings STATIC ${GObjectDOMBindings_SOURCES})

WEBKIT_SET_EXTRA_COMPILER_FLAGS(GObjectDOMBindings)

add_dependencies(GObjectDOMBindings
    WebCore
    fake-generated-webkitdom-headers
)

file(WRITE ${CMAKE_BINARY_DIR}/gtkdoc-webkitdom.cfg
    "[webkitdomgtk-${WEBKITGTK_API_VERSION}]\n"
    "pkgconfig_file=${WebKit2_PKGCONFIG_FILE}\n"
    "namespace=webkit_dom\n"
    "cflags=-I${CMAKE_SOURCE_DIR}/Source\n"
    "       -I${WEBCORE_DIR}/bindings\n"
    "       -I${WEBCORE_DIR}/bindings/gobject\n"
    "       -I${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}\n"
    "doc_dir=${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}/docs\n"
    "source_dirs=${DERIVED_SOURCES_GOBJECT_DOM_BINDINGS_DIR}\n"
    "            ${WEBCORE_DIR}/bindings/gobject\n"
    "headers=${GObjectDOMBindingsStable_INSTALLED_HEADERS}\n"
    "main_sgml_file=webkitdomgtk-docs.sgml\n"
)

install(FILES ${GObjectDOMBindingsStable_INSTALLED_HEADERS}
        DESTINATION "${WEBKITGTK_HEADER_INSTALL_DIR}/webkitdom"
)

# Make unstable header optional if they don't exist
install(FILES ${GObjectDOMBindingsUnstable_INSTALLED_HEADERS}
        DESTINATION "${WEBKITGTK_HEADER_INSTALL_DIR}/webkitdom"
        OPTIONAL
)

# Some installed headers are not on the list of headers used for gir generation.
set(GObjectDOMBindings_GIR_HEADERS ${GObjectDOMBindingsStable_INSTALLED_HEADERS})
list(REMOVE_ITEM GObjectDOMBindings_GIR_HEADERS
     bindings/gobject/WebKitDOMEventTarget.h
     bindings/gobject/WebKitDOMNodeFilter.h
     bindings/gobject/WebKitDOMObject.h
     bindings/gobject/WebKitDOMXPathNSResolver.h
)

# Propagate this variable to the parent scope, so that it can be used in other parts of the build.
set(GObjectDOMBindings_GIR_HEADERS ${GObjectDOMBindings_GIR_HEADERS} PARENT_SCOPE)

if (ENABLE_SMOOTH_SCROLLING)
    list(APPEND WebCore_SOURCES
        platform/ScrollAnimationSmooth.cpp
    )
endif ()

if (ENABLE_SUBTLE_CRYPTO)
    list(APPEND WebCore_SOURCES
        crypto/CryptoAlgorithm.cpp
        crypto/CryptoAlgorithmDescriptionBuilder.cpp
        crypto/CryptoAlgorithmRegistry.cpp
        crypto/CryptoKey.cpp
        crypto/CryptoKeyPair.cpp
        crypto/WebKitSubtleCrypto.cpp

        crypto/algorithms/CryptoAlgorithmAES_CBC.cpp
        crypto/algorithms/CryptoAlgorithmAES_KW.cpp
        crypto/algorithms/CryptoAlgorithmHMAC.cpp
        crypto/algorithms/CryptoAlgorithmRSAES_PKCS1_v1_5.cpp
        crypto/algorithms/CryptoAlgorithmRSASSA_PKCS1_v1_5.cpp
        crypto/algorithms/CryptoAlgorithmRSA_OAEP.cpp
        crypto/algorithms/CryptoAlgorithmSHA1.cpp
        crypto/algorithms/CryptoAlgorithmSHA224.cpp
        crypto/algorithms/CryptoAlgorithmSHA256.cpp
        crypto/algorithms/CryptoAlgorithmSHA384.cpp
        crypto/algorithms/CryptoAlgorithmSHA512.cpp

        crypto/gnutls/CryptoAlgorithmAES_CBCGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmAES_KWGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmHMACGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmRSAES_PKCS1_v1_5GnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmRSASSA_PKCS1_v1_5GnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmRSA_OAEPGnuTLS.cpp
        crypto/gnutls/CryptoAlgorithmRegistryGnuTLS.cpp
        crypto/gnutls/CryptoKeyRSAGnuTLS.cpp
        crypto/gnutls/SerializedCryptoKeyWrapGnuTLS.cpp

        crypto/keys/CryptoKeyAES.cpp
        crypto/keys/CryptoKeyDataOctetSequence.cpp
        crypto/keys/CryptoKeyDataRSAComponents.cpp
        crypto/keys/CryptoKeyHMAC.cpp
        crypto/keys/CryptoKeySerializationRaw.cpp
    )
endif ()
