/*
 *  Copyright (C) 2010,2017 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DragImage.h"

#include <wtf/text/CString.h>
#include "BitmapImage.h"
#include "FrameView.h"
#include "Frame.h"
#include "Element.h"
#include "HostWindow.h"
#include "Image.h"
#include "RefPtrCairo.h"
#include "gui.h"
#include <cairo.h>
#include <clib/debug_protos.h>

#define D(x)

namespace WebCore {

IntSize dragImageSize(DragImageRef image)
{
    if (image)
        return { cairo_image_surface_get_width(image.get()), cairo_image_surface_get_height(image.get()) };

    return { 0, 0 };
}

void deleteDragImage(DragImageRef)
{
    // Since this is a RefPtr, there's nothing additional we need to do to
    // delete it. It will be released when it falls out of scope.
}

DragImageRef scaleDragImage(DragImageRef image, FloatSize scale)
{
    if (!image)
        return nullptr;

    IntSize imageSize = dragImageSize(image);
    IntSize scaledSize(imageSize);
    scaledSize.scale(scale.width(), scale.height());
    if (imageSize == scaledSize)
        return image;

    RefPtr<cairo_surface_t> scaledSurface = adoptRef(cairo_surface_create_similar(image.get(), CAIRO_CONTENT_COLOR_ALPHA, scaledSize.width(), scaledSize.height()));

    RefPtr<cairo_t> context = adoptRef(cairo_create(scaledSurface.get()));
    cairo_scale(context.get(), scale.width(), scale.height());
    cairo_pattern_set_extend(cairo_get_source(context.get()), CAIRO_EXTEND_PAD);
    cairo_pattern_set_filter(cairo_get_source(context.get()), CAIRO_FILTER_BEST);
    cairo_set_operator(context.get(), CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(context.get(), image.get(), 0, 0);
    cairo_paint(context.get());

    return scaledSurface;
}

DragImageRef dissolveDragImageToFraction(DragImageRef image, float fraction)
{
    if (!image)
        return nullptr;

    RefPtr<cairo_t> context = adoptRef(cairo_create(image.get()));
    cairo_set_operator(context.get(), CAIRO_OPERATOR_DEST_IN);
    cairo_set_source_rgba(context.get(), 0, 0, 0, fraction);
    cairo_paint(context.get());
    return image;
}

DragImageRef createDragImageFromImage(Image* image, ImageOrientationDescription)
{
    return image->nativeImageForCurrentFrame();
}

DragImageRef createDragImageIconForCachedImageFilename(const String&)
{
    return nullptr;
}

DragImageRef createDragImageForLink(Element& element, URL& url, const String&, TextIndicatorData&, FontRenderingMode fontRenderingMode, float)
{
    D(kprintf("createDragImageForLink %s %s\n", url.string().latin1().data(), inLabel.latin1().data()));

    FrameView *frameView = element.document().frame()->view();
    BalWidget *widget = frameView->hostWindow()->platformPageClient();

    if(widget)
    {
        set(widget->browser, MA_OWBBrowser_DragURL, url.string().latin1().data());
    }
    return nullptr;
}

DragImageRef createDragImageForColor(const Color&, const FloatRect&, float, Path&)
{
    return nullptr;
}

}
