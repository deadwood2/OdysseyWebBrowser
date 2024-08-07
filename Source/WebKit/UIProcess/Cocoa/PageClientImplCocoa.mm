/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#import "config.h"
#import "PageClientImplCocoa.h"

#import "WKWebViewInternal.h"

namespace WebKit {

void PageClientImplCocoa::isPlayingAudioWillChange()
{
#if WK_API_ENABLED
    [m_webView willChangeValueForKey:NSStringFromSelector(@selector(_isPlayingAudio))];
#endif
}

void PageClientImplCocoa::isPlayingAudioDidChange()
{
#if WK_API_ENABLED
    [m_webView didChangeValueForKey:NSStringFromSelector(@selector(_isPlayingAudio))];
#endif
}

#if ENABLE(ATTACHMENT_ELEMENT)

void PageClientImplCocoa::didInsertAttachment(const String& identifier, const String& source)
{
#if WK_API_ENABLED
    [m_webView _didInsertAttachment:identifier withSource:source];
#else
    UNUSED_PARAM(identifier);
#endif
}

void PageClientImplCocoa::didRemoveAttachment(const String& identifier)
{
#if WK_API_ENABLED
    [m_webView _didRemoveAttachment:identifier];
#else
    UNUSED_PARAM(identifier);
#endif
}

#endif
    
}
