/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#import "WKFoundation.h"

#if WK_API_ENABLED

#import "APIFullscreenClient.h"
#import "WeakObjCPtr.h"
#import <wtf/RetainPtr.h>

@class NSView;
@protocol _WKFullscreenDelegate;

namespace WebKit {

class FullscreenClient : public API::FullscreenClient {
public:
    explicit FullscreenClient(NSView *);
    ~FullscreenClient() { };

    RetainPtr<id<_WKFullscreenDelegate>> delegate();
    void setDelegate(id<_WKFullscreenDelegate>);

    void willEnterFullscreen(WebPageProxy*) override;
    void didEnterFullscreen(WebPageProxy*) override;
    void willExitFullscreen(WebPageProxy*) override;
    void didExitFullscreen(WebPageProxy*) override;

private:
    NSView *m_webView;
    WeakObjCPtr<id <_WKFullscreenDelegate> > m_delegate;

    struct {
        bool webViewWillEnterFullscreen : 1;
        bool webViewDidEnterFullscreen : 1;
        bool webViewWillExitFullscreen : 1;
        bool webViewDidExitFullscreen : 1;
    } m_delegateMethods;
};
    
} // namespace WebKit

#endif // WK_API_ENABLED
