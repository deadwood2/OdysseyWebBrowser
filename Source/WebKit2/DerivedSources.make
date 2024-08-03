# Copyright (C) 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

VPATH = \
    $(WebKit2) \
    $(WebKit2)/DatabaseProcess \
    $(WebKit2)/DatabaseProcess/IndexedDB \
    $(WebKit2)/DatabaseProcess/mac \
    $(WebKit2)/NetworkProcess \
    $(WebKit2)/NetworkProcess/CustomProtocols \
    $(WebKit2)/NetworkProcess/mac \
    $(WebKit2)/PluginProcess \
    $(WebKit2)/PluginProcess/mac \
    $(WebKit2)/Shared/Plugins \
    $(WebKit2)/Shared \
    $(WebKit2)/Shared/API/Cocoa \
    $(WebKit2)/Shared/Authentication \
    $(WebKit2)/Shared/mac \
    $(WebKit2)/WebProcess/ApplePay \
    $(WebKit2)/WebProcess/ApplicationCache \
    $(WebKit2)/WebProcess/Automation \
    $(WebKit2)/WebProcess/Cookies \
    $(WebKit2)/WebProcess/Databases/IndexedDB \
    $(WebKit2)/WebProcess/FullScreen \
    $(WebKit2)/WebProcess/Geolocation \
    $(WebKit2)/WebProcess/IconDatabase \
    $(WebKit2)/WebProcess/MediaCache \
    $(WebKit2)/WebProcess/Network \
    $(WebKit2)/WebProcess/Notifications \
    $(WebKit2)/WebProcess/OriginData \
    $(WebKit2)/WebProcess/Plugins \
    $(WebKit2)/WebProcess/ResourceCache \
    $(WebKit2)/WebProcess/Scrolling \
    $(WebKit2)/WebProcess/Storage \
    $(WebKit2)/WebProcess/UserContent \
    $(WebKit2)/WebProcess/WebCoreSupport \
    $(WebKit2)/WebProcess/WebPage \
    $(WebKit2)/WebProcess/cocoa \
    $(WebKit2)/WebProcess/ios \
    $(WebKit2)/WebProcess \
    $(WebKit2)/UIProcess \
    $(WebKit2)/UIProcess/ApplePay \
    $(WebKit2)/UIProcess/Automation \
    $(WebKit2)/UIProcess/Cocoa \
    $(WebKit2)/UIProcess/Databases \
    $(WebKit2)/UIProcess/Downloads \
    $(WebKit2)/UIProcess/Network \
    $(WebKit2)/UIProcess/Network/CustomProtocols \
    $(WebKit2)/UIProcess/Notifications \
    $(WebKit2)/UIProcess/Plugins \
    $(WebKit2)/UIProcess/Storage \
    $(WebKit2)/UIProcess/UserContent \
    $(WebKit2)/UIProcess/mac \
    $(WebKit2)/UIProcess/ios \
    $(WEBKITADDITIONS_HEADER_SEARCH_PATHS) \
#

PYTHON = python
PERL = perl

ifeq ($(OS),Windows_NT)
    DELETE = cmd //C del
else
    DELETE = rm -f
endif

MESSAGE_RECEIVERS = \
    AuthenticationManager \
    ChildProcess \
    CustomProtocolManager \
    CustomProtocolManagerProxy \
    DatabaseProcess \
    DatabaseProcessProxy \
    DatabaseToWebProcessConnection \
    DownloadProxy \
    DrawingArea \
    DrawingAreaProxy \
    EventDispatcher \
    NPObjectMessageReceiver \
    NetworkConnectionToWebProcess \
    NetworkProcess \
    NetworkProcessConnection \
    NetworkProcessProxy \
    NetworkResourceLoader \
    PluginControllerProxy \
    PluginProcess \
    PluginProcessConnection \
    PluginProcessConnectionManager \
    PluginProcessProxy \
    PluginProxy \
    RemoteLayerTreeDrawingAreaProxy \
    RemoteObjectRegistry \
    RemoteScrollingCoordinator \
    SecItemShim \
    SecItemShimProxy \
    SmartMagnificationController \
    StorageAreaMap \
    StorageManager \
    ViewGestureController \
    ViewGestureGeometryCollector \
    ViewUpdateDispatcher \
    VisitedLinkStore \
    VisitedLinkTableController \
    WebAutomationSession \
    WebAutomationSessionProxy \
    WebConnection \
    WebCookieManager \
    WebCookieManagerProxy \
    WebFullScreenManager \
    WebFullScreenManagerProxy \
    WebGeolocationManager \
    WebGeolocationManagerProxy \
    WebIDBConnectionToClient \
    WebIDBConnectionToServer \
    WebIconDatabase \
    WebIconDatabaseProxy \
    WebInspector \
    WebInspectorProxy \
    WebInspectorUI \
    WebNotificationManager \
    WebPage \
    WebPageProxy \
    WebPasteboardProxy \
    WebPaymentCoordinator \
    WebPaymentCoordinatorProxy \
    WebPlaybackSessionManager \
    WebPlaybackSessionManagerProxy \
    WebProcess \
    WebProcessConnection \
    WebProcessPool \
    WebProcessProxy \
    WebResourceLoader \
    WebResourceLoadStatisticsStore \
    WebUserContentController \
    WebUserContentControllerProxy \
    WebVideoFullscreenManager \
    WebVideoFullscreenManagerProxy \
#

SCRIPTS = \
    $(WebKit2)/Scripts/generate-message-receiver.py \
    $(WebKit2)/Scripts/generate-messages-header.py \
    $(WebKit2)/Scripts/webkit/__init__.py \
    $(WebKit2)/Scripts/webkit/messages.py \
    $(WebKit2)/Scripts/webkit/model.py \
    $(WebKit2)/Scripts/webkit/parser.py \
#

FRAMEWORK_FLAGS = $(shell echo $(BUILT_PRODUCTS_DIR) $(FRAMEWORK_SEARCH_PATHS) | perl -e 'print "-F " . join(" -F ", split(" ", <>));')
HEADER_FLAGS = $(shell echo $(BUILT_PRODUCTS_DIR) $(HEADER_SEARCH_PATHS) | perl -e 'print "-I" . join(" -I", split(" ", <>));')

-include WebKitDerivedSourcesAdditions.make

.PHONY : all

all : \
    $(MESSAGE_RECEIVERS:%=%MessageReceiver.cpp) \
    $(MESSAGE_RECEIVERS:%=%Messages.h) \
#

%MessageReceiver.cpp : %.messages.in $(SCRIPTS)
	@echo Generating messages header for $*...
	@python $(WebKit2)/Scripts/generate-message-receiver.py $< > $@

%Messages.h : %.messages.in $(SCRIPTS)
	@echo Generating message receiver for $*...
	@python $(WebKit2)/Scripts/generate-messages-header.py $< > $@


# Some versions of clang incorrectly strip out // comments in c89 code.
# Use -traditional as a workaround, but only when needed since that causes
# other problems with later versions of clang.
ifeq ($(shell echo '//x' | $(CC) -E -P -x c -std=c89 - | grep x),)
TEXT_PREPROCESSOR_FLAGS=-E -P -x c -traditional -w
else
TEXT_PREPROCESSOR_FLAGS=-E -P -x c -std=c89 -w
endif

ifneq ($(SDKROOT),)
	SDK_FLAGS=-isysroot $(SDKROOT)
endif

SANDBOX_PROFILES = \
	com.apple.WebProcess.sb \
	com.apple.WebKit.Databases.sb \
	com.apple.WebKit.NetworkProcess.sb

all: $(SANDBOX_PROFILES)

%.sb : %.sb.in
	@echo Pre-processing $* sandbox profile...
	$(CC) $(SDK_FLAGS) $(TEXT_PREPROCESSOR_FLAGS) $(FRAMEWORK_FLAGS) $(HEADER_FLAGS) -include "wtf/Platform.h" $< > $@

AUTOMATION_PROTOCOL_GENERATOR_SCRIPTS = \
	$(JavaScriptCore_SCRIPTS_DIR)/cpp_generator_templates.py \
	$(JavaScriptCore_SCRIPTS_DIR)/cpp_generator.py \
	$(JavaScriptCore_SCRIPTS_DIR)/generate_cpp_backend_dispatcher_header.py \
	$(JavaScriptCore_SCRIPTS_DIR)/generate_cpp_backend_dispatcher_implementation.py \
	$(JavaScriptCore_SCRIPTS_DIR)/generate_cpp_protocol_types_header.py \
	$(JavaScriptCore_SCRIPTS_DIR)/generate_cpp_protocol_types_implementation.py \
	$(JavaScriptCore_SCRIPTS_DIR)/generator_templates.py \
	$(JavaScriptCore_SCRIPTS_DIR)/generator.py \
	$(JavaScriptCore_SCRIPTS_DIR)/models.py \
	$(JavaScriptCore_SCRIPTS_DIR)/generate-inspector-protocol-bindings.py \
#

AUTOMATION_PROTOCOL_INPUT_FILES = \
    $(WebKit2)/UIProcess/Automation/Automation.json \
#

AUTOMATION_PROTOCOL_OUTPUT_FILES = \
    AutomationBackendDispatchers.h \
    AutomationBackendDispatchers.cpp \
#

# JSON-RPC Backend Dispatchers, Type Builders
$(firstword $(AUTOMATION_PROTOCOL_OUTPUT_FILES)) : $(AUTOMATION_PROTOCOL_INPUT_FILES) $(AUTOMATION_PROTOCOL_GENERATOR_SCRIPTS)
	$(PYTHON) $(JavaScriptCore_SCRIPTS_DIR)/generate-inspector-protocol-bindings.py --framework WebKit --backend --outputDir . $(AUTOMATION_PROTOCOL_INPUT_FILES)

all : $(firstword $(AUTOMATION_PROTOCOL_OUTPUT_FILES))

%ScriptSource.h : %.js $(JavaScriptCore_SCRIPTS_DIR)/jsmin.py $(JavaScriptCore_SCRIPTS_DIR)/xxd.pl
	echo "//# sourceURL=__InjectedScript_$(notdir $<)" > $(basename $(notdir $<)).min.js
	$(PYTHON) $(JavaScriptCore_SCRIPTS_DIR)/jsmin.py < $< >> $(basename $(notdir $<)).min.js
	$(PERL) $(JavaScriptCore_SCRIPTS_DIR)/xxd.pl $(basename $(notdir $<))ScriptSource $(basename $(notdir $<)).min.js $@
	$(DELETE) $(basename $(notdir $<)).min.js

all : WebAutomationSessionProxyScriptSource.h
