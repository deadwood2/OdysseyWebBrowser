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
    capabilities.acceptInsecureCerts = false;
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
    if (!browserOptions->getString(ASCIILiteral("binary"), binary))
        return false;

    RefPtr<JSON::Value> browserArgumentsValue;
    RefPtr<JSON::Array> browserArguments;
    if (browserOptions->getValue(ASCIILiteral("args"), browserArgumentsValue) && !browserArgumentsValue->asArray(browserArguments))
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
    RefPtr<JSON::Object> browserOptions;
    if (!matchedCapabilities.getObject(ASCIILiteral("wpe:browserOptions"), browserOptions)) {
        capabilities.browserBinary = String("dyz");
        capabilities.browserArguments = Vector<String> { ASCIILiteral("--automation") };
        return;
    }

    String browserBinary;
    browserOptions->getString(ASCIILiteral("binary"), browserBinary);
    ASSERT(!browserBinary.isNull());
    capabilities.browserBinary = browserBinary;

    capabilities.browserArguments = Vector<String>();
    RefPtr<JSON::Array> browserArguments;
    if (browserOptions->getArray(ASCIILiteral("args"), browserArguments)) {
        unsigned browserArgumentsLength = browserArguments->length();
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
