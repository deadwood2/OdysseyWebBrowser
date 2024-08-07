set(WebKit_OUTPUT_NAME WebKit)
set(WebKit_WebProcess_OUTPUT_NAME WebKitWebProcess)
set(WebKit_NetworkProcess_OUTPUT_NAME WebKitNetworkProcess)
set(WebKit_PluginProcess_OUTPUT_NAME WebKitPluginProcess)
set(WebKit_StorageProcess_OUTPUT_NAME WebKitStorageProcess)

file(MAKE_DIRECTORY ${DERIVED_SOURCES_WEBKIT_DIR})

add_definitions(-DBUILDING_WEBKIT)

list(APPEND WebKit_SOURCES
    NetworkProcess/win/NetworkProcessMainWin.cpp
    NetworkProcess/win/SystemProxyWin.cpp

    Platform/win/LoggingWin.cpp
    Platform/win/ModuleWin.cpp
    Platform/win/SharedMemoryWin.cpp

    Platform/IPC/win/AttachmentWin.cpp
    Platform/IPC/win/ConnectionWin.cpp

    StorageProcess/win/StorageProcessMainWin.cpp

    WebProcess/InjectedBundle/win/InjectedBundleWin.cpp

    WebProcess/MediaCache/WebMediaKeyStorageManager.cpp

    WebProcess/Plugins/Netscape/win/PluginProxyWin.cpp

    WebProcess/WebCoreSupport/win/WebContextMenuClientWin.cpp
    WebProcess/WebCoreSupport/win/WebPopupMenuWin.cpp

    WebProcess/WebPage/AcceleratedDrawingArea.cpp
    WebProcess/WebPage/AcceleratedSurface.cpp
    WebProcess/WebPage/DrawingAreaImpl.cpp

    WebProcess/WebPage/CoordinatedGraphics/CompositingCoordinator.cpp
    WebProcess/WebPage/CoordinatedGraphics/CoordinatedLayerTreeHost.cpp
    WebProcess/WebPage/CoordinatedGraphics/ThreadedCoordinatedLayerTreeHost.cpp

    WebProcess/WebPage/win/WebInspectorUIWin.cpp
    WebProcess/WebPage/win/WebPageWin.cpp

    WebProcess/win/WebProcessMainWin.cpp
    WebProcess/win/WebProcessWin.cpp
)

# DerivedSources/JavaScriptCore/inspector/InspectorBackendCommands.js is
# expected in DerivedSources/WebInspectorUI/UserInterface/Protocol/.
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
    DEPENDS ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js
    COMMAND cp ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
)

list(APPEND WebKit_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/NetworkProcess/win"
    "${WEBKIT_DIR}/Platform/classifier"
    "${WEBKIT_DIR}/PluginProcess/win"
    "${WEBKIT_DIR}/Shared/API/c/win"
    "${WEBKIT_DIR}/Shared/CoordinatedGraphics"
    "${WEBKIT_DIR}/Shared/CoordinatedGraphics/threadedcompositor"
    "${WEBKIT_DIR}/Shared/Plugins/win"
    "${WEBKIT_DIR}/Shared/unix"
    "${WEBKIT_DIR}/Shared/win"
    "${WEBKIT_DIR}/StorageProcess/win"
    "${WEBKIT_DIR}/UIProcess/API/C/cairo"
    "${WEBKIT_DIR}/UIProcess/API/C/win"
    "${WEBKIT_DIR}/UIProcess/API/cpp/win"
    "${WEBKIT_DIR}/UIProcess/API/win"
    "${WEBKIT_DIR}/UIProcess/Plugins/win"
    "${WEBKIT_DIR}/UIProcess/win"
    "${WEBKIT_DIR}/WebProcess/InjectedBundle/API/win"
    "${WEBKIT_DIR}/WebProcess/InjectedBundle/API/win/DOM"
    "${WEBKIT_DIR}/WebProcess/win"
    "${WEBKIT_DIR}/WebProcess/WebCoreSupport/win"
    "${WEBKIT_DIR}/WebProcess/WebPage/CoordinatedGraphics"
    "${WEBKIT_DIR}/WebProcess/WebPage/win"
    "${WEBKIT_DIR}/win"
)

list(APPEND WebKit_SYSTEM_INCLUDE_DIRECTORIES
    ${CAIRO_INCLUDE_DIRS}
)

set(WebKitCommonIncludeDirectories ${WebKit_INCLUDE_DIRECTORIES})
set(WebKitCommonSystemIncludeDirectories ${WebKit_SYSTEM_INCLUDE_DIRECTORIES})

list(APPEND WebProcess_SOURCES
    WebProcess/EntryPoint/win/WebProcessMain.cpp
)

list(APPEND NetworkProcess_SOURCES
    NetworkProcess/EntryPoint/win/NetworkProcessMain.cpp
)

list(APPEND StorageProcess_SOURCES
    StorageProcess/EntryPoint/win/StorageProcessMain.cpp
)

if (${ENABLE_PLUGIN_PROCESS})
    list(APPEND PluginProcess_SOURCES
    )
endif ()

if (${WTF_PLATFORM_WIN_CAIRO})
    add_definitions(-DUSE_CAIRO=1 -DUSE_CURL=1)

    list(APPEND WebKit_SOURCES
        NetworkProcess/cache/NetworkCacheCodersCurl.cpp
        NetworkProcess/cache/NetworkCacheDataCurl.cpp
        NetworkProcess/cache/NetworkCacheIOChannelCurl.cpp

        NetworkProcess/curl/NetworkProcessCurl.cpp
        NetworkProcess/curl/RemoteNetworkingContextCurl.cpp

        Shared/Authentication/curl/AuthenticationManagerCurl.cpp

        Shared/curl/WebCoreArgumentCodersCurl.cpp

        WebProcess/Cookies/curl/WebCookieManagerCurl.cpp

        WebProcess/WebCoreSupport/curl/WebFrameNetworkingContext.cpp
    )

    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBKIT_DIR}/UIProcess/WebCoreSupport/curl"
    )

    list(APPEND WebKit_LIBRARIES
        ${OPENSSL_LIBRARIES}
        mfuuid.lib
        strmiids.lib
    )
endif ()

set(SharedWebKitLibraries
    ${WebKit_LIBRARIES}
)

WEBKIT_WRAP_SOURCELIST(${WebKit_SOURCES})

set(WebKit_FORWARDING_HEADERS_DIRECTORIES
    Platform
    Shared
    UIProcess

    NetworkProcess/Downloads

    Platform/IPC

    Shared/API

    Shared/API/c

    Shared/API/c/cf

    UIProcess/API/c
    UIProcess/API/cpp

    WebProcess/WebPage

    WebProcess/InectedBundle/API/c
)

WEBKIT_MAKE_FORWARDING_HEADERS(WebKit DIRECTORIES ${WebKit_FORWARDING_HEADERS_DIRECTORIES})
