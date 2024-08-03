/*
 * Copyright (C) 2016 Igalia S.L.
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
#include "AcceleratedBackingStore.h"

#include "WebPageProxy.h"
#include <WebCore/CairoUtilities.h>
#include <WebCore/PlatformDisplay.h>

#if PLATFORM(WAYLAND)
#include "AcceleratedBackingStoreWayland.h"
#endif

#if USE(REDIRECTED_XCOMPOSITE_WINDOW)
#include "AcceleratedBackingStoreX11.h"
#endif

using namespace WebCore;

namespace WebKit {

std::unique_ptr<AcceleratedBackingStore> AcceleratedBackingStore::create(WebPageProxy& webPage)
{
#if PLATFORM(WAYLAND) && USE(EGL)
    if (PlatformDisplay::sharedDisplay().type() == PlatformDisplay::Type::Wayland)
        return AcceleratedBackingStoreWayland::create(webPage);
#endif
#if USE(REDIRECTED_XCOMPOSITE_WINDOW)
    if (PlatformDisplay::sharedDisplay().type() == PlatformDisplay::Type::X11)
        return AcceleratedBackingStoreX11::create(webPage);
#endif
    return nullptr;
}

AcceleratedBackingStore::AcceleratedBackingStore(WebPageProxy& webPage)
    : m_webPage(webPage)
{
}

bool AcceleratedBackingStore::paint(cairo_t* cr, const IntRect& clipRect)
{
    if (m_webPage.drawsBackground())
        return true;

    const WebCore::Color& color = m_webPage.backgroundColor();
    if (color.hasAlpha()) {
        cairo_rectangle(cr, clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_fill(cr);
    }

    if (color.alpha() > 0) {
        setSourceRGBAFromColor(cr, color);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_rectangle(cr, clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
        cairo_fill(cr);
    }

    return true;
}

} // namespace WebKit
