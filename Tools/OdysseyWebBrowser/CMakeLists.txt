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
    WebCore
    WebKit
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
    -lavcodec
    -liconv
    -lz
    -lavformat
    -lbz2
    -lavutil
    -lswresample
    -lswscale
)

include_directories(${OdysseyWebBrowser_INCLUDE_DIRECTORIES})
add_executable(owb ${OdysseyWebBrowser_SOURCES})
target_link_libraries(owb ${OdysseyWebBrowser_LIBRARIES})
