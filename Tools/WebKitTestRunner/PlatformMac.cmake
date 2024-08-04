find_library(CARBON_LIBRARY Carbon)

find_library(CORESERVICES_LIBRARY CoreServices)
add_definitions(-iframework ${CORESERVICES_LIBRARY}/Versions/Current/Frameworks)

if ("${CURRENT_OSX_VERSION}" MATCHES "10.9")
set(WEBKITSYSTEMINTERFACE_LIBRARY libWebKitSystemInterfaceMavericks.a)
elif ("${CURRENT_OSX_VERSION}" MATCHES "10.10")
set(WEBKITSYSTEMINTERFACE_LIBRARY libWebKitSystemInterfaceYosemite.a)
else ()
set(WEBKITSYSTEMINTERFACE_LIBRARY libWebKitSystemInterfaceElCapitan.a)
endif ()
link_directories(../../WebKitLibraries)

list(APPEND WebKitTestRunner_LIBRARIES
    ${CARBON_LIBRARY}
    ${WEBKITSYSTEMINTERFACE_LIBRARY}
)

list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
    ${DERIVED_SOURCES_DIR}
    ${DERIVED_SOURCES_DIR}/WebCore
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders/JavaScriptCore
    ${DERIVED_SOURCES_DIR}/ForwardingHeaders/WebCore
    ${WEBCORE_DIR}/testing/cocoa
    ${WEBKIT_TESTRUNNER_DIR}/cf
    ${WEBKIT_TESTRUNNER_DIR}/cg
    ${WEBKIT_TESTRUNNER_DIR}/cocoa
    ${WEBKIT_TESTRUNNER_DIR}/mac
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac
    ${CMAKE_SOURCE_DIR}/WebKitLibraries
)

list(APPEND WebKitTestRunnerInjectedBundle_SOURCES
    ${WEBKIT_TESTRUNNER_DIR}/cocoa/CrashReporterInfo.mm

    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/cocoa/ActivateFontsCocoa.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/cocoa/InjectedBundlePageCocoa.mm

    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/AccessibilityControllerMac.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/AccessibilityNotificationHandler.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/AccessibilityTextMarkerRangeMac.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/InjectedBundleMac.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/AccessibilityCommonMac.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/AccessibilityTextMarkerMac.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/AccessibilityUIElementMac.mm
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/mac/TestRunnerMac.mm
)

list(APPEND WebKitTestRunner_SOURCES
    ${WEBKIT_TESTRUNNER_DIR}/cg/TestInvocationCG.cpp

    ${WEBKIT_TESTRUNNER_DIR}/cocoa/CrashReporterInfo.mm
    ${WEBKIT_TESTRUNNER_DIR}/cocoa/TestControllerCocoa.mm
    ${WEBKIT_TESTRUNNER_DIR}/cocoa/TestRunnerWKWebView.mm

    ${WEBKIT_TESTRUNNER_DIR}/mac/EventSenderProxy.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/PlatformWebViewMac.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/PoseAsClass.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/TestControllerMac.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/UIScriptControllerMac.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/WebKitTestRunnerDraggingInfo.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/WebKitTestRunnerEvent.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/WebKitTestRunnerPasteboard.mm
    ${WEBKIT_TESTRUNNER_DIR}/mac/main.mm
)

link_directories(../../WebKitLibraries)
