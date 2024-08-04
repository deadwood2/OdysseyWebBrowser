# Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
# Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer. 
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution. 
# 3.  Neither the name of Apple Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

VPATH = DOM $(BUILT_PRODUCTS_DIR)/DerivedSources/WebKitLegacy/WebCorePrivateHeaders

PRIVATE_HEADERS_DIR = $(BUILT_PRODUCTS_DIR)/$(PRIVATE_HEADERS_FOLDER_PATH)

.PHONY : all
all : \
    $(PRIVATE_HEADERS_DIR)/WebKitAvailability.h \
    $(PRIVATE_HEADERS_DIR)/WebScriptObject.h \
#

ifeq ($(PLATFORM_NAME), macosx)
all : \
    $(PRIVATE_HEADERS_DIR)/npapi.h \
    $(PRIVATE_HEADERS_DIR)/npfunctions.h \
    $(PRIVATE_HEADERS_DIR)/npruntime.h \
    $(PRIVATE_HEADERS_DIR)/nptypes.h \
#
endif

ifneq ($(PLATFORM_NAME), macosx)
all : \
    $(PRIVATE_HEADERS_DIR)/KeyEventCodesIOS.h \
    $(PRIVATE_HEADERS_DIR)/WAKAppKitStubs.h \
    $(PRIVATE_HEADERS_DIR)/WAKResponder.h \
    $(PRIVATE_HEADERS_DIR)/WAKView.h \
    $(PRIVATE_HEADERS_DIR)/WAKWindow.h \
    $(PRIVATE_HEADERS_DIR)/WKContentObservation.h \
    $(PRIVATE_HEADERS_DIR)/WKGraphics.h \
    $(PRIVATE_HEADERS_DIR)/WKTypes.h \
    $(PRIVATE_HEADERS_DIR)/WebCoreThread.h \
    $(PRIVATE_HEADERS_DIR)/WebCoreThreadMessage.h \
    $(PRIVATE_HEADERS_DIR)/WebCoreThreadRun.h \
    $(PRIVATE_HEADERS_DIR)/WebEvent.h \
#
endif

ifeq ($(findstring ENABLE_IOS_TOUCH_EVENTS, $(FEATURE_DEFINES)), ENABLE_IOS_TOUCH_EVENTS)
all : \
    $(PRIVATE_HEADERS_DIR)/WebEventRegion.h
endif

WEBCORE_HEADER_REPLACE_RULES = -e 's/<WebCore\//<WebKitLegacy\//' -e "s/(^ *)WEBCORE_EXPORT /\1/"
WEBCORE_HEADER_MIGRATE_CMD = sed -E $(WEBCORE_HEADER_REPLACE_RULES) $< > $@

$(PRIVATE_HEADERS_DIR)/% : % MigrateHeaders.make
	$(WEBCORE_HEADER_MIGRATE_CMD)
