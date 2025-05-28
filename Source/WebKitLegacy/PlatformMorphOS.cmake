add_definitions(-DUSE_CAIRO=1 -DUSE_CURL=1 -DWEBKIT_EXPORTS=1 -DWEBCORE_EXPORT=WTF_EXPORT_DECLARATION -DPAL_EXPORT=WTF_EXPORT_DECLARATION -DWTF_EXPORT=WTF_EXPORT_DECLARATION
	-DJS_EXPORT_PRIVATE=WTF_EXPORT -DUSE_SYSTEM_MALLOC -DMORPHOS_MINIMAL=${MORPHOS_MINIMAL})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ")

list(APPEND WebKitLegacy_PRIVATE_INCLUDE_DIRECTORIES
    ${CAIRO_INCLUDE_DIRS}
    "${WEBKIT_LIBRARIES_DIR}/include"
    "${ROOTPATH}/morphoswb/development/tools/eventprofiler/"
)

list(APPEND WebKitLegacy_LIBRARIES PRIVATE WTF${DEBUG_SUFFIX})

list(APPEND WebKitLegacy_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}/../include/private"
    "${CMAKE_BINARY_DIR}/../include/private/JavaScriptCore"
    "${CMAKE_BINARY_DIR}/../include/private/WebCore"
    "${WEBKITLEGACY_DIR}/morphos"
    "${WEBKITLEGACY_DIR}/morphos/WebCoreSupport"
    "${WebKitLegacy_DERIVED_SOURCES_DIR}/include"
    "${WebKitLegacy_DERIVED_SOURCES_DIR}/Interfaces"
    ${SQLITE_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    ${WPE_INCLUDE_DIRS}
    ${OBJC_INCLUDE}
)

list(APPEND WebKitLegacy_SOURCES_Classes
    morphos/WebFrame.cpp
    morphos/WebPage.cpp
    morphos/WebProcess.cpp
    morphos/BackForwardClient.cpp
    morphos/WebApplicationCache.cpp
    morphos/storage/WebDatabaseProvider.cpp
    morphos/WebDocumentLoader.cpp
    morphos/CacheModel.cpp
    morphos/WebDragClient.cpp
    morphos/PopupMenu.cpp
    morphos/Gamepad.cpp
)

list(APPEND WebKitLegacy_SOURCES_Classes
    morphos/WkWebView.mm
    morphos/WkNetworkRequestMutable.mm
    morphos/WkHistory.mm
    morphos/WkSettings.mm
    morphos/WkCertificate.mm
    morphos/WkCertificateViewer.mm
    morphos/WkError.mm
    morphos/WkDownload.mm
    morphos/WkFileDialog.mm
    morphos/WkHitTest.mm
    morphos/WkFavIcon.mm
    morphos/WkPrinting.mm
    morphos/WkUserScript.mm
    morphos/WkMedia.mm
    morphos/WkNotification.mm
    morphos/WkResourceResponse.mm
    morphos/WkCache.mm
)

list(APPEND WebKitLegacy_SOURCES_WebCoreSupport
    morphos/WebCoreSupport/WebVisitedLinkStore.cpp
    morphos/WebCoreSupport/WebEditorClient.cpp
    morphos/WebCoreSupport/WebChromeClient.cpp
    morphos/WebCoreSupport/WebPluginInfoProvider.cpp
    morphos/WebCoreSupport/WebPlatformStrategies.cpp
    morphos/WebCoreSupport/WebInspectorClient.cpp
    morphos/WebCoreSupport/WebFrameLoaderClient.cpp
    morphos/WebCoreSupport/WebFrameNetworkingContext.cpp
    morphos/WebCoreSupport/WebContextMenuClient.cpp
    morphos/WebCoreSupport/WebPageGroup.cpp
    morphos/WebCoreSupport/WebProgressTrackerClient.cpp
    morphos/WebCoreSupport/WebNotificationClient.cpp
)

if (NOT MORPHOS_MINIMAL)
	list(APPEND WebKitLegacy_ABP
		morphos/ABPFilterParser/ABPFilterParser.cpp
		morphos/ABPFilterParser/BloomFilter.cpp
		morphos/ABPFilterParser/cosmeticFilter.cpp
		morphos/ABPFilterParser/filter.cpp
		morphos/ABPFilterParser/hashFn.cpp
	)
	add_definitions(-DENABLE_WEB_AUDIO=1)
	list(APPEND WebKitLegacy_SOURCES_Classes
		morphos/WebDatabaseManager.cpp
		morphos/WebCoreSupport/WebUserMediaClient.cpp
	)
endif()

list(APPEND WebKitLegacy_SOURCES ${WebKitLegacy_INCLUDES} ${WebKitLegacy_SOURCES_Classes} ${WebKitLegacy_SOURCES_WebCoreSupport} ${WebKitLegacy_ABP})

set(MM_FLAGS "-Wno-ignored-attributes -Wno-protocol -Wundeclared-selector -fobjc-call-cxx-cdtors -fobjc-exceptions -fconstant-string-class=OBConstantString -DDEBUG=0")

set_source_files_properties(morphos/WkWebView.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkNetworkRequestMutable.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkHistory.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkSettings.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkCertificate.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkCertificateViewer.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkError.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkDownload.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkFileDialog.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkHitTest.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkFavIcon.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkPrinting.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkUserScript.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkMedia.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkNotification.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkResourceResponse.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})
set_source_files_properties(morphos/WkCache.mm PROPERTIES COMPILE_FLAGS ${MM_FLAGS})

set(WebKitLegacy_OUTPUT_NAME
    WebKit${DEBUG_SUFFIX}
)
