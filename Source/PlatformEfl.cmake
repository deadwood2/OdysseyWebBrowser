add_subdirectory(${WEBCORE_DIR}/platform/efl/DefaultTheme)

# DerivedSources/JavaScriptCore/inspector/InspectorBackendCommands.js is
# expected in DerivedSources/WebInspectorUI/UserInterface/Protocol/.
add_custom_command(
    OUTPUT ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
    DEPENDS ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js
    COMMAND cp ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/inspector/InspectorBackendCommands.js ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
)

add_custom_target(
    web-inspector-resources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${WEBINSPECTORUI_DIR}/UserInterface ${WEB_INSPECTOR_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js ${WEB_INSPECTOR_DIR}/Protocol
    COMMAND ${CMAKE_COMMAND} -E copy ${WEBINSPECTORUI_DIR}/Localizations/en.lproj/localizedStrings.js ${WEB_INSPECTOR_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${WEBKIT2_DIR}/UIProcess/InspectorServer/front-end/inspectorPageIndex.html ${WEB_INSPECTOR_DIR}
    DEPENDS JavaScriptCore WebCore ${DERIVED_SOURCES_WEBINSPECTORUI_DIR}/UserInterface/Protocol/InspectorBackendCommands.js
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
install(DIRECTORY "${CMAKE_BINARY_DIR}/${WEB_INSPECTOR_DIR}"
    DESTINATION ${DATA_INSTALL_DIR}
    FILES_MATCHING PATTERN "*.js"
                   PATTERN "*.html"
                   PATTERN "*.css"
                   PATTERN "*.gif"
                   PATTERN "*.png"
                   PATTERN "*.svg")

find_program(UGLIFYJS_EXECUTABLE uglifyjs)
if (UGLIFYJS_EXECUTABLE AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug"))
    file(GLOB frontend_js_files "${WEBINSPECTORUI_DIR}/UserInterface/*.js")
    set(all_js_files
        ${frontend_js_files}
        "${WEBINSPECTORUI_DIR}/Localizations/en.lproj/localizedStrings.js"
    )

    foreach (js_file ${all_js_files})
        get_filename_component(filename ${js_file} NAME)
        install(CODE
            "execute_process(
                COMMAND ${UGLIFYJS_EXECUTABLE} --overwrite ${filename}
                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${WEB_INSPECTOR_DIR})")
    endforeach ()

    file(GLOB codemirror_js_files "${WEBINSPECTORUI_DIR}/UserInterface/External/CodeMirror/*.js")
    foreach (js_file ${codemirror_js_files})
        get_filename_component(filename ${js_file} NAME)
        install(CODE
            "execute_process(
                COMMAND ${UGLIFYJS_EXECUTABLE} --overwrite ${filename}
                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${WEB_INSPECTOR_DIR}/External/CodeMirror/)")
    endforeach ()
endif ()
