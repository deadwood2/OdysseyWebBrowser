/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Holger Hans Peter Freyther <zecke@selfish.org>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2020 Apple Inc.  All rights reserved.
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
#include "ImageBufferUtilitiesCairo.h"

#if USE(CAIRO)

#include "CairoUtilities.h"
#include "MIMETypeRegistry.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(GTK)
#include "GRefPtrGtk.h"
#include "GdkCairoUtilities.h"
#include <gtk/gtk.h>
#include <wtf/glib/GUniquePtr.h>
#endif

#if OS(MORPHOS)
#define __WANT_PNG_1_6__
#include <png.h>
#include <proto/dos.h>
#include <string.h>
#include <proto/random.h>
#endif

namespace WebCore {

#if !PLATFORM(GTK)
#if OS(MORPHOS)

struct pngWriteStruct
{
    Vector<uint8_t>* output;
    bool error;
};

static void png_error_callback(png_structp png_save_ptr, png_const_charp error_msg)
{
    pngWriteStruct *ws;
    ws = static_cast<pngWriteStruct*>(png_get_error_ptr(png_save_ptr));
    if (ws)
        ws->error = true;

    longjmp (png_jmpbuf(png_save_ptr), 1);
}

static void png_warning_callback(png_structp png_save_ptr, png_const_charp warning_msg)
{
}

static void png_write_func(png_structp png_ptr, png_bytep data, png_size_t length)
{
    pngWriteStruct *ws = static_cast<pngWriteStruct*>(png_get_io_ptr(png_ptr));

    if (!ws->output->tryAppend(std::span(data, length))) {
        png_error(png_ptr, "write function failed");
    }
}

static void png_flush_func(png_structp png_ptr)
{
}

static void unpremultiply_data(png_structp png, png_row_infop row_info, png_bytep data)
{
    unsigned int i;

    for (i = 0; i < row_info->rowbytes; i += 4) {
        uint8_t *b = &data[i];
        uint32_t pixel;
        uint8_t  alpha;

        memcpy (&pixel, b, sizeof (uint32_t));
        alpha = (pixel & 0xff000000) >> 24;
        if (alpha == 0) {
            b[0] = b[1] = b[2] = b[3] = 0;
        } else {
            b[0] = (((pixel & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
            b[1] = (((pixel & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
            b[2] = (((pixel & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
            b[3] = alpha;
        }
    }
}

static bool encodeImage(cairo_surface_t* image, const String& mimeType, Vector<uint8_t>* output)
{
    pngWriteStruct ws;

    ws.error = false;
    ws.output = output;

    auto png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &ws, png_error_callback, png_warning_callback);
    if (!png_ptr) {
        return false;
    }

    png_infop info_ptr;

    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    }

    png_set_write_fn(png_ptr, &ws, png_write_func, png_flush_func);

    int w = cairo_image_surface_get_width(image);
    int h = cairo_image_surface_get_height(image);

    png_set_IHDR(png_ptr, info_ptr, w, h, 8,
          PNG_COLOR_TYPE_RGB_ALPHA,
          PNG_INTERLACE_NONE,
          PNG_COMPRESSION_TYPE_BASE,
          PNG_FILTER_TYPE_BASE);

    png_color_8 sig_bit;
    sig_bit.red = 8;
    sig_bit.green = 8;
    sig_bit.blue = 8;
    sig_bit.alpha = 8;
    png_set_sBIT(png_ptr, info_ptr, &sig_bit);

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);
    png_set_write_user_transform_fn(png_ptr, unpremultiply_data);

    const unsigned int stride = cairo_image_surface_get_stride(image);
    unsigned char *pixels = cairo_image_surface_get_data(image);
    unsigned char *ptr;
    png_bytep row_ptr;
    int y;

    for (y = 0, ptr = pixels; y < h; y++, ptr += stride) {
        row_ptr = (png_bytep)ptr;
        png_write_rows(png_ptr, &row_ptr, 1);
    }

    png_write_end (png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

    return !ws.error;
}
#else
static cairo_status_t writeFunction(void* output, const unsigned char* data, unsigned length)
{
    if (!reinterpret_cast<Vector<uint8_t>*>(output)->tryAppend(std::span { data, length }))
        return CAIRO_STATUS_WRITE_ERROR;
    return CAIRO_STATUS_SUCCESS;
}

static bool encodeImage(cairo_surface_t* image, const String& mimeType, Vector<uint8_t>* output)
{
    ASSERT_UNUSED(mimeType, mimeType == "image/png"_s); // Only PNG output is supported for now.

    return cairo_surface_write_to_png_stream(image, writeFunction, output) == CAIRO_STATUS_SUCCESS;
}
#endif

Vector<uint8_t> encodeData(cairo_surface_t* image, const String& mimeType, std::optional<double>)
{
    Vector<uint8_t> encodedImage;
    if (!image || !encodeImage(image, mimeType, &encodedImage))
        return { };
    return encodedImage;
}
#else
static bool encodeImage(cairo_surface_t* surface, const String& mimeType, std::optional<double> quality, GUniqueOutPtr<gchar>& buffer, gsize& bufferSize)
{
    // List of supported image encoding types comes from the GdkPixbuf documentation.
    // http://developer.gnome.org/gdk-pixbuf/stable/gdk-pixbuf-File-saving.html#gdk-pixbuf-save-to-bufferv

    String type = mimeType.substring(sizeof "image");
    if (type != "jpeg"_s && type != "png"_s && type != "tiff"_s && type != "ico"_s && type != "bmp"_s)
        return false;

    auto pixbuf = cairoSurfaceToGdkPixbuf(surface);
    if (!pixbuf)
        return false;

    GUniqueOutPtr<GError> error;
    if (type == "jpeg"_s && quality && *quality >= 0.0 && *quality <= 1.0) {
        String qualityString = String::number(static_cast<int>(*quality * 100.0 + 0.5));
        gdk_pixbuf_save_to_buffer(pixbuf.get(), &buffer.outPtr(), &bufferSize, type.utf8().data(), &error.outPtr(), "quality", qualityString.utf8().data(), NULL);
    } else
        gdk_pixbuf_save_to_buffer(pixbuf.get(), &buffer.outPtr(), &bufferSize, type.utf8().data(), &error.outPtr(), NULL);

    return !error;
}

Vector<uint8_t> encodeData(cairo_surface_t* image, const String& mimeType, std::optional<double> quality)
{
    GUniqueOutPtr<gchar> buffer;
    gsize bufferSize;
    if (!encodeImage(image, mimeType, quality, buffer, bufferSize))
        return { };

    return std::span { reinterpret_cast<const uint8_t*>(buffer.get()), bufferSize };
}
#endif // !PLATFORM(GTK)

} // namespace WebCore

#endif // USE(CAIRO)
