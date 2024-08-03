/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_RTC)

#include "RTCIceCandidateEvent.h"

#include "EventNames.h"
#include "RTCIceCandidate.h"

namespace WebCore {

Ref<RTCIceCandidateEvent> RTCIceCandidateEvent::create(bool canBubble, bool cancelable, RefPtr<RTCIceCandidate>&& candidate)
{
    return adoptRef(*new RTCIceCandidateEvent(canBubble, cancelable, WTFMove(candidate)));
}

RTCIceCandidateEvent::RTCIceCandidateEvent(bool canBubble, bool cancelable, RefPtr<RTCIceCandidate>&& candidate)
    : Event(eventNames().icecandidateEvent, canBubble, cancelable)
    , m_candidate(WTFMove(candidate))
{
}

RTCIceCandidateEvent::~RTCIceCandidateEvent()
{
}

RTCIceCandidate* RTCIceCandidateEvent::candidate() const
{
    return m_candidate.get();
}

EventInterface RTCIceCandidateEvent::eventInterface() const
{
    return RTCIceCandidateEventInterfaceType;
}

} // namespace WebCore

#endif // ENABLE(WEB_RTC)

