set(WEBINSPECTORUI_DIR "${CMAKE_SOURCE_DIR}/Source/WebInspectorUI")
set(WEB_INSPECTOR_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/WebKit.resources/WebInspectorUI)

add_custom_target(
    web-inspector-resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${WEBINSPECTORUI_DIR}/UserInterface ${WEB_INSPECTOR_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js ${WEB_INSPECTOR_DIR}/Protocol
    COMMAND ${CMAKE_COMMAND} -E copy ${WEBINSPECTORUI_DIR}/Localizations/en.lproj/localizedStrings.js ${WEB_INSPECTOR_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${WEBKIT2_DIR}/UIProcess/InspectorServer/front-end/inspectorPageIndex.html ${WEB_INSPECTOR_DIR}
    DEPENDS JavaScriptCore WebCore
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

if (EXISTS ${CMAKE_SOURCE_DIR}/../Internal/WebKit/WebKitSystemInterface/win/CMakeLists.txt)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../Internal/WebKit/WebKitSystemInterface/win ${CMAKE_CURRENT_BINARY_DIR}/WebKitSystemInterface)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../Internal/WebKit/WebKitQuartzCoreAdditions ${CMAKE_CURRENT_BINARY_DIR}/WebKitQuartzCoreAdditions)
endif ()

if (EXISTS ${CMAKE_SOURCE_DIR}/../Internal/Tools/WKTestBrowser/CMakeLists.txt)
    add_subdirectory(${CMAKE_SOURCE_DIR}/../Internal/Tools/WKTestBrowser ${CMAKE_CURRENT_BINARY_DIR}/WKTestBrowser)
endif ()

