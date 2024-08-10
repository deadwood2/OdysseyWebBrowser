/*
 * Copyright (C) 2015-2017 Apple Inc. All rights reserved.
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

#import <wtf/spi/darwin/dyldSPI.h>

#if USE(APPLE_INTERNAL_SDK)
#import <WebKitAdditions/VersionChecksAdditions.h>
#else
#define DYLD_IOS_VERSION_FIRST_WITH_LAZY_GESTURE_RECOGNIZER_INSTALLATION 0
#define DYLD_IOS_VERSION_FIRST_WITH_PROCESS_SWAP_ON_CROSS_SITE_NAVIGATION 0
#endif

namespace WebKit {

enum class SDKVersion : uint32_t {
#if PLATFORM(IOS_FAMILY)
    FirstWithNetworkCache = DYLD_IOS_VERSION_9_0,
    FirstWithMediaTypesRequiringUserActionForPlayback = DYLD_IOS_VERSION_10_0,
    FirstWithExceptionsForDuplicateCompletionHandlerCalls = DYLD_IOS_VERSION_11_0,
    FirstToExcludeLocalStorageFromBackup = DYLD_IOS_VERSION_11_0,
    FirstWithExpiredOnlyReloadBehavior = DYLD_IOS_VERSION_11_0,
    FirstThatDisallowsSettingAnyXHRHeaderFromFileURLs = DYLD_IOS_VERSION_11_3,
    FirstThatDefaultsToPassiveTouchListenersOnDocument = DYLD_IOS_VERSION_11_3,
    FirstWhereScrollViewContentInsetsAreNotObscuringInsets = DYLD_IOS_VERSION_12_0,
    FirstWhereUIScrollViewDoesNotApplyKeyboardInsetsUnconditionally = DYLD_IOS_VERSION_12_0,
    FirstWithMainThreadReleaseAssertionInWebPageProxy = DYLD_IOS_VERSION_12_0,
    FirstWithLazyGestureRecognizerInstallation = DYLD_IOS_VERSION_FIRST_WITH_LAZY_GESTURE_RECOGNIZER_INSTALLATION,
    FirstWithProcessSwapOnCrossSiteNavigation = DYLD_IOS_VERSION_FIRST_WITH_PROCESS_SWAP_ON_CROSS_SITE_NAVIGATION,
#elif PLATFORM(MAC)
    FirstWithNetworkCache = DYLD_MACOSX_VERSION_10_11,
    FirstWithExceptionsForDuplicateCompletionHandlerCalls = DYLD_MACOSX_VERSION_10_13,
    FirstWithDropToNavigateDisallowedByDefault = DYLD_MACOSX_VERSION_10_13,
    FirstWithExpiredOnlyReloadBehavior = DYLD_MACOSX_VERSION_10_13,
    FirstWithMainThreadReleaseAssertionInWebPageProxy = DYLD_MACOSX_VERSION_10_14,
#endif
};

bool linkedOnOrAfter(SDKVersion);

}
