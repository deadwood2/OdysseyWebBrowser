cmake_minimum_required(VERSION 3.10)
include(WebKitCommon)

find_package(Cairo REQUIRED)

set(MINIBROWSER_DIR "./morphos")

add_definitions(-DWEBCORE_EXPORT="" -Wno-protocol -Wundeclared-selector -fobjc-call-cxx-cdtors -fobjc-exceptions -fconstant-string-class=OBConstantString -Wignored-attributes)

set(MiniBrowser_SOURCES
    minibrowser.mm
)

set(MiniBrowser_INCLUDE_DIRECTORIES
    ${CMAKE_SOURCE_DIR}/Source
    ${FORWARDING_HEADERS_DIR}
    ${MINIBROWSER_DIR}
    ${WTF_FRAMEWORK_HEADERS_DIR}
    ${JavaScriptCore_FRAMEWORK_HEADERS_DIR}
    ${JavaScriptCore_PRIVATE_FRAMEWORK_HEADERS_DIR}
    ${PAL_FRAMEWORK_HEADERS_DIR}
    ${WebCore_PRIVATE_FRAMEWORK_HEADERS_DIR}
    ${WebKitLegacy_FRAMEWORK_HEADERS_DIR}
    ${WebKit_FRAMEWORK_HEADERS_DIR}
    ${WebKit_PRIVATE_FRAMEWORK_HEADERS_DIR}
    ${CAIRO_INCLUDE_DIRS}
    ${OBJC_INCLUDE}
)

set(MiniBrowser_LIBRARIES
    WebKitLegacy
)

set(EXECUTABLE_NAME MiniBrowser)
set(PRODUCT_NAME MiniBrowser)


include_directories(${MiniBrowser_INCLUDE_DIRECTORIES})
add_library(MiniBrowserlib ${MiniBrowser_SOURCES})
add_custom_target(MiniBrowser ALL COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../../link.sh" DEPENDS MiniBrowserlib WebKitLegacy)

#add_executable(MiniBrowser.db ${MiniBrowser_SOURCES})
#target_link_libraries(MiniBrowser.db ${MiniBrowser_LIBRARIES} mui.framework.a ob.framework objc)
#add_custom_command(OUTPUT MiniBrowser
#	COMMAND "ppc-morphos-strip -o $<TARGET_FILE:MiniBrowser> $<TARGET_FILE:MiniBrowser>.db"
#	DEPENDS MiniBrowser.db
#       )


#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -lob.framework -lmui.framework -lobjc")
