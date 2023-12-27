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

#ifndef InteractionInformationAtPosition_h
#define InteractionInformationAtPosition_h

#include "ArgumentCoders.h"
#include "ShareableBitmap.h"
#include <WebCore/IntPoint.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(IOS)
#include <WebCore/SelectionRect.h>
#endif

namespace WebKit {

#if PLATFORM(IOS)
struct InteractionInformationAtPosition {
    InteractionInformationAtPosition()
        : nodeAtPositionIsAssistedNode(false)
        , isSelectable(false)
        , isNearMarkedText(false)
        , touchCalloutEnabled(true)
    {
    }

    WebCore::IntPoint point;
    bool nodeAtPositionIsAssistedNode;
    bool isSelectable;
    bool isNearMarkedText;
    bool touchCalloutEnabled;
    String clickableElementName;
    String url;
    String title;
    WebCore::IntRect bounds;
    RefPtr<ShareableBitmap> image;

    void encode(IPC::ArgumentEncoder&) const;
    static bool decode(IPC::ArgumentDecoder&, InteractionInformationAtPosition&);
};
#endif

}

#endif // InteractionInformationAtPosition_h
