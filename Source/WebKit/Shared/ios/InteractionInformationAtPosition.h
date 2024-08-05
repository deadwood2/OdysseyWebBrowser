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

#pragma once

#if PLATFORM(IOS)

#include "ArgumentCoders.h"
#include "InteractionInformationRequest.h"
#include "ShareableBitmap.h"
#include <WebCore/IntPoint.h>
#include <WebCore/SelectionRect.h>
#include <WebCore/TextIndicator.h>
#include <WebCore/URL.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

struct InteractionInformationAtPosition {
    InteractionInformationRequest request;

    bool nodeAtPositionIsAssistedNode { false };
#if ENABLE(DATA_INTERACTION)
    bool hasSelectionAtPosition { false };
#endif
    bool isSelectable { false };
    bool isNearMarkedText { false };
    bool touchCalloutEnabled { true };
    bool isLink { false };
    bool isImage { false };
    bool isAttachment { false };
    bool isAnimatedImage { false };
    bool isElement { false };
#if ENABLE(DATA_DETECTION)
    bool isDataDetectorLink { false };
#endif
    WebCore::FloatPoint adjustedPointForNodeRespondingToClickEvents;
    WebCore::URL url;
    WebCore::URL imageURL;
    String title;
    String idAttribute;
    WebCore::IntRect bounds;
    RefPtr<ShareableBitmap> image;
    String textBefore;
    String textAfter;

    WebCore::TextIndicatorData linkIndicator;
#if ENABLE(DATA_DETECTION)
    String dataDetectorIdentifier;
    RetainPtr<NSArray> dataDetectorResults;
#endif

    // Copy compatible optional bits forward (for example, if we have a InteractionInformationAtPosition
    // with snapshots in it, and perform another request for the same point without requesting the snapshots,
    // we can fetch the cheap information and copy the snapshots into the new response).
    void mergeCompatibleOptionalInformation(const InteractionInformationAtPosition& oldInformation);

    void encode(IPC::Encoder&) const;
    static bool decode(IPC::Decoder&, InteractionInformationAtPosition&);
};

}

#endif // PLATFORM(IOS)
