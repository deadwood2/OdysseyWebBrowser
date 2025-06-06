set(OdysseyWebBrowser_SOURCES
    main.cpp
)

set(OdysseyWebBrowser_INCLUDE_DIRECTORIES
    "${CAIRO_INCLUDE_DIRS}"
    "${WEBKIT_DIR}/mui/Api/MorphOS"
    "${WEBKIT_DIR}/mui/Api/AROS/include"
    "${DERIVED_SOURCES_WEBCORE_DIR}"
)

set(OdysseyWebBrowser_LIBRARIES
    JavaScriptCore
    WebKitLegacy
    -Wl,--start-group WebCore -Wl,--end-group
    -lz
    -lcairo
    -lpixman
    -lfontconfig
    -lfreetype2
    -lxml2
    -lz
    -lcurl
    -ljpeg
    -lpng
    -lssl
    -lcrypto
    -licui18n
    -licuuc
    -licudata
    -lsqlite3
    -lxslt
    -ldebug
    -lopenurl
    -lrexxsyslib
    -lavformat
    -lbz2
    -lavcodec
    -liconv
    -lz
    -lavutil
    -lswresample
    -lswscale
    -lpsl
    -latomic
    ${ZLIB_LIBRARIES}
)

include_directories(${OdysseyWebBrowser_INCLUDE_DIRECTORIES})
add_executable(OWB ${OdysseyWebBrowser_SOURCES})
target_link_libraries(OWB ${OdysseyWebBrowser_LIBRARIES})
