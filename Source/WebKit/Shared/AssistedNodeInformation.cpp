/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "AssistedNodeInformation.h"

#include "WebCoreArgumentCoders.h"

namespace WebKit {

#if PLATFORM(IOS)
void OptionItem::encode(IPC::Encoder& encoder) const
{
    encoder << text;
    encoder << isGroup;
    encoder << isSelected;
    encoder << disabled;
    encoder << parentGroupID;
}

bool OptionItem::decode(IPC::Decoder& decoder, OptionItem& result)
{
    if (!decoder.decode(result.text))
        return false;

    if (!decoder.decode(result.isGroup))
        return false;

    if (!decoder.decode(result.isSelected))
        return false;

    if (!decoder.decode(result.disabled))
        return false;

    if (!decoder.decode(result.parentGroupID))
        return false;
    
    return true;
}

void AssistedNodeInformation::encode(IPC::Encoder& encoder) const
{
    encoder << elementRect;
    encoder << selectionRect;
    encoder << minimumScaleFactor;
    encoder << maximumScaleFactor;
    encoder << maximumScaleFactorIgnoringAlwaysScalable;
    encoder << nodeFontSize;
    encoder << hasNextNode;
    encoder << hasPreviousNode;
    encoder << isAutocorrect;
    encoder << isRTL;
    encoder.encodeEnum(autocapitalizeType);
    encoder.encodeEnum(elementType);
    encoder << formAction;
    encoder << selectOptions;
    encoder << selectedIndex;
    encoder << isMultiSelect;
    encoder << isReadOnly;
    encoder << allowsUserScaling;
    encoder << allowsUserScalingIgnoringAlwaysScalable;
    encoder << insideFixedPosition;
    encoder << value;
    encoder << valueAsNumber;
    encoder << title;
    encoder.encodeEnum(autofillFieldName);
}

bool AssistedNodeInformation::decode(IPC::Decoder& decoder, AssistedNodeInformation& result)
{
    if (!decoder.decode(result.elementRect))
        return false;

    if (!decoder.decode(result.selectionRect))
        return false;

    if (!decoder.decode(result.minimumScaleFactor))
        return false;

    if (!decoder.decode(result.maximumScaleFactor))
        return false;
    
    if (!decoder.decode(result.maximumScaleFactorIgnoringAlwaysScalable))
        return false;

    if (!decoder.decode(result.nodeFontSize))
        return false;

    if (!decoder.decode(result.hasNextNode))
        return false;

    if (!decoder.decode(result.hasPreviousNode))
        return false;

    if (!decoder.decode(result.isAutocorrect))
        return false;

    if (!decoder.decode(result.isRTL))
        return false;

    if (!decoder.decodeEnum(result.autocapitalizeType))
        return false;

    if (!decoder.decodeEnum(result.elementType))
        return false;

    if (!decoder.decode(result.formAction))
        return false;

    if (!decoder.decode(result.selectOptions))
        return false;

    if (!decoder.decode(result.selectedIndex))
        return false;

    if (!decoder.decode(result.isMultiSelect))
        return false;

    if (!decoder.decode(result.isReadOnly))
        return false;

    if (!decoder.decode(result.allowsUserScaling))
        return false;
    
    if (!decoder.decode(result.allowsUserScalingIgnoringAlwaysScalable))
        return false;

    if (!decoder.decode(result.insideFixedPosition))
        return false;

    if (!decoder.decode(result.value))
        return false;

    if (!decoder.decode(result.valueAsNumber))
        return false;

    if (!decoder.decode(result.title))
        return false;

    if (!decoder.decodeEnum(result.autofillFieldName))
        return false;

    return true;
}
#endif

}
