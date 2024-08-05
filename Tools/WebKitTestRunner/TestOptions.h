/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#ifndef TestOptions_h
#define TestOptions_h

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WTR {

struct TestOptions {
    bool useThreadedScrolling { false };
    bool useRemoteLayerTree { false };
    bool shouldShowWebView { false };
    bool useFlexibleViewport { false };
    bool useFixedLayout { false };
    bool isSVGTest { false };
    bool useDataDetection { false };
    bool useMockScrollbars { true };
    bool needsSiteSpecificQuirks { false };
    bool ignoresViewportScaleLimits { false };
    bool useCharacterSelectionGranularity { false };
    bool enableIntersectionObserver { false };
    bool enableModernMediaControls { true };
    bool enablePointerLock { false };
    bool enableCredentialManagement { false };
    bool enableIsSecureContextAttribute { true };
    bool enableInspectorAdditions { false };

    float deviceScaleFactor { 1 };
    Vector<String> overrideLanguages;
    
    TestOptions(const std::string& pathOrURL);

    // Add here options that can only be set upon PlatformWebView
    // initialization and make sure it's up to date when adding new
    // options to this struct. Otherwise, tests using those options
    // might fail if WTR is reusing an existing PlatformWebView.
    bool hasSameInitializationOptions(const TestOptions& options) const
    {
        if (useThreadedScrolling != options.useThreadedScrolling
            || overrideLanguages != options.overrideLanguages
            || useMockScrollbars != options.useMockScrollbars
            || needsSiteSpecificQuirks != options.needsSiteSpecificQuirks
            || useCharacterSelectionGranularity != options.useCharacterSelectionGranularity
            || enableIntersectionObserver != options.enableIntersectionObserver
            || enableModernMediaControls != options.enableModernMediaControls
            || enablePointerLock != options.enablePointerLock
            || enableCredentialManagement != options.enableCredentialManagement
            || enableIsSecureContextAttribute != options.enableIsSecureContextAttribute
            || enableInspectorAdditions != options.enableInspectorAdditions)
            return false;

        return true;
    }
};

}

#endif // TestOptions_h
