/*
 * Copyright (C) 2015 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformDisplayWPE.h"

#if PLATFORM(WPE)

#include "GLContextEGL.h"
// FIXME: For now default to the GBM EGL platform, but this should really be
// somehow deducible from the build configuration.
#define __GBM__ 1
#if USE(LIBEPOXY)
#include "EpoxyEGL.h"
#else
#include <EGL/egl.h>
#endif
#include <wpe/renderer-backend-egl.h>

namespace WebCore {

PlatformDisplayWPE::PlatformDisplayWPE() = default;

PlatformDisplayWPE::~PlatformDisplayWPE()
{
    wpe_renderer_backend_egl_destroy(m_backend);
}

void PlatformDisplayWPE::initialize(int hostFd)
{
    m_backend = wpe_renderer_backend_egl_create(hostFd);

    m_eglDisplay = eglGetDisplay(wpe_renderer_backend_egl_get_native_display(m_backend));
    if (m_eglDisplay == EGL_NO_DISPLAY) {
        WTFLogAlways("PlatformDisplayWPE: could not create the EGL display.");
        return;
    }

    PlatformDisplay::initializeEGLDisplay();
}

} // namespace WebCore

#endif // PLATFORM(WPE)
