/*
 * Copyright (C) 2017 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "UTIRegistry.h"

#if USE(CG)

#include <wtf/HashSet.h>
#include <wtf/NeverDestroyed.h>

#if !PLATFORM(IOS)
#include <ApplicationServices/ApplicationServices.h>
#else
#include <ImageIO/ImageIO.h>
#endif

#if ENABLE(WEB_ARCHIVE) || ENABLE(MHTML)
#include "ArchiveFactory.h"
#endif

namespace WebCore {

HashSet<String>& allowedImageUTIs()
{
    // CG at least supports the following standard image types:
    static NeverDestroyed<HashSet<String>> s_allowedImageUTIs = std::initializer_list<String> {
        "com.compuserve.gif",
        "com.microsoft.bmp",
        "com.microsoft.cur",
        "com.microsoft.ico",
        "public.jpeg",
        "public.jpeg-2000",
        "public.mpo-image",
        "public.png",
        "public.tiff",
    };

#ifndef NDEBUG
    // But make sure that all of them are really supported.
    static bool checked = false;
    if (!checked) {
        RetainPtr<CFArrayRef> systemImageUTIs = adoptCF(CGImageSourceCopyTypeIdentifiers());
        CFIndex count = CFArrayGetCount(systemImageUTIs.get());
        for (auto& imageUTI : s_allowedImageUTIs.get()) {
            RetainPtr<CFStringRef> string = imageUTI.createCFString();
            ASSERT(CFArrayContainsValue(systemImageUTIs.get(), CFRangeMake(0, count), string.get()));
        }
        checked = true;
    }
#endif

    return s_allowedImageUTIs.get();
}

bool isAllowedImageUTI(const String& imageUTI)
{
    return !imageUTI.isEmpty() && allowedImageUTIs().contains(imageUTI);
}

}

#endif
