/*
 * Copyright (C) 2017 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebDriverService.h"

#include "Capabilities.h"
#include "CommandResult.h"
#include <wtf/JSONValues.h>

namespace WebDriver {

Capabilities WebDriverService::platformCapabilities()
{
    Capabilities capabilities;
    capabilities.platformName = String("linux");
    capabilities.setWindowRect = false;
    return capabilities;
}

bool WebDriverService::platformValidateCapability(const String& name, const RefPtr<JSON::Value>& value) const
{
    if (name != "wpe:browserOptions")
        return true;

    RefPtr<JSON::Object> browserOptions;
    if (!value->asObject(browserOptions))
        return false;

    if (browserOptions->isNull())
        return true;

    // If browser options are provided, binary is required.
    String binary;
    if (!browserOptions->getString("binary"_s, binary))
        return false;

    RefPtr<JSON::Value> browserArgumentsValue;
    RefPtr<JSON::Array> browserArguments;
    if (browserOptions->getValue("args"_s, browserArgumentsValue) && !browserArgumentsValue->asArray(browserArguments))
        return false;

    unsigned browserArgumentsLength = browserArguments->length();
    for (unsigned i = 0; i < browserArgumentsLength; ++i) {
        RefPtr<JSON::Value> value = browserArguments->get(i);
        String argument;
        if (!value->asString(argument))
            return false;
    }

    return true;
}

bool WebDriverService::platformMatchCapability(const String&, const RefPtr<JSON::Value>&) const
{
    return true;
}

void WebDriverService::platformParseCapabilities(const JSON::Object& matchedCapabilities, Capabilities& capabilities) const
{
    capabilities.browserBinary = String("MiniBrowser");
    capabilities.browserArguments = Vector<String> { "--automation"_s };

    RefPtr<JSON::Object> browserOptions;
    if (!matchedCapabilities.getObject("wpe:browserOptions"_s, browserOptions))
        return;

    String browserBinary;
    if (browserOptions->getString("binary"_s, browserBinary)) {
        capabilities.browserBinary = browserBinary;
        capabilities.browserArguments = std::nullopt;
    }

    RefPtr<JSON::Array> browserArguments;
    if (browserOptions->getArray("args"_s, browserArguments) && browserArguments->length()) {
        unsigned browserArgumentsLength = browserArguments->length();
        capabilities.browserArguments = Vector<String>();
        capabilities.browserArguments->reserveInitialCapacity(browserArgumentsLength);
        for (unsigned i = 0; i < browserArgumentsLength; ++i) {
            RefPtr<JSON::Value> value = browserArguments->get(i);
            String argument;
            value->asString(argument);
            ASSERT(!argument.isNull());
            capabilities.browserArguments->uncheckedAppend(WTFMove(argument));
        }
    }
}

} // namespace WebDriver
