/*
 * Copyright (C) 2004, 2006, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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

#pragma once

#include "FloatRect.h"
#include "HTMLElement.h"
#include "IntSize.h"
#include <memory>
#include <wtf/Forward.h>

#if ENABLE(WEBGL)
#include "WebGLContextAttributes.h"
#endif

namespace WebCore {

class CanvasRenderingContext;
class GraphicsContext;
class GraphicsContextStateSaver;
class HTMLCanvasElement;
class Image;
class ImageBuffer;
class ImageData;

namespace DisplayList {
using AsTextFlags = unsigned;
}

class CanvasObserver {
public:
    virtual ~CanvasObserver() { }

    virtual void canvasChanged(HTMLCanvasElement&, const FloatRect& changedRect) = 0;
    virtual void canvasResized(HTMLCanvasElement&) = 0;
    virtual void canvasDestroyed(HTMLCanvasElement&) = 0;
};

class HTMLCanvasElement final : public HTMLElement {
public:
    static Ref<HTMLCanvasElement> create(Document&);
    static Ref<HTMLCanvasElement> create(const QualifiedName&, Document&);
    virtual ~HTMLCanvasElement();

    void addObserver(CanvasObserver&);
    void removeObserver(CanvasObserver&);

    unsigned width() const { return size().width(); }
    unsigned height() const { return size().height(); }

    const IntSize& size() const { return m_size; }

    WEBCORE_EXPORT void setWidth(unsigned);
    WEBCORE_EXPORT void setHeight(unsigned);

    void setSize(const IntSize& newSize)
    { 
        if (newSize == size())
            return;
        m_ignoreReset = true; 
        setWidth(newSize.width());
        setHeight(newSize.height());
        m_ignoreReset = false;
        reset();
    }

    CanvasRenderingContext* getContext(const String&);

    static bool is2dType(const String&);
    CanvasRenderingContext* getContext2d(const String&);

#if ENABLE(WEBGL)
    static bool is3dType(const String&);
    CanvasRenderingContext* getContextWebGL(const String&, WebGLContextAttributes&& = { });
#endif

    static String toEncodingMimeType(const String& mimeType);
    WEBCORE_EXPORT ExceptionOr<String> toDataURL(const String& mimeType, std::optional<double> quality);
    ExceptionOr<String> toDataURL(const String& mimeType) { return toDataURL(mimeType, std::nullopt); }

    // Used for rendering
    void didDraw(const FloatRect&);
    void notifyObserversCanvasChanged(const FloatRect&);

    void paint(GraphicsContext&, const LayoutRect&);

    GraphicsContext* drawingContext() const;
    GraphicsContext* existingDrawingContext() const;

    CanvasRenderingContext* renderingContext() const { return m_context.get(); }

    ImageBuffer* buffer() const;
    Image* copiedImage() const;
    void clearCopiedImage();
    RefPtr<ImageData> getImageData();
    void makePresentationCopy();
    void clearPresentationCopy();

    FloatRect convertLogicalToDevice(const FloatRect&) const;
    FloatSize convertLogicalToDevice(const FloatSize&) const;

    FloatSize convertDeviceToLogical(const FloatSize&) const;

    SecurityOrigin* securityOrigin() const;
    void setOriginTainted() { m_originClean = false; }
    bool originClean() const { return m_originClean; }

    AffineTransform baseTransform() const;

    void makeRenderingResultsAvailable();
    bool hasCreatedImageBuffer() const { return m_hasCreatedImageBuffer; }

    bool shouldAccelerate(const IntSize&) const;

    WEBCORE_EXPORT void setUsesDisplayListDrawing(bool);
    WEBCORE_EXPORT void setTracksDisplayListReplay(bool);
    WEBCORE_EXPORT String displayListAsText(DisplayList::AsTextFlags) const;
    WEBCORE_EXPORT String replayDisplayListAsText(DisplayList::AsTextFlags) const;

    size_t memoryCost() const;
    size_t externalMemoryCost() const;

private:
    HTMLCanvasElement(const QualifiedName&, Document&);

    void parseAttribute(const QualifiedName&, const AtomicString&) final;
    RenderPtr<RenderElement> createElementRenderer(RenderStyle&&, const RenderTreePosition&) final;

    bool canContainRangeEndPoint() const final;
    bool canStartSelection() const final;

    void reset();

    void createImageBuffer() const;
    void clearImageBuffer() const;

    void setSurfaceSize(const IntSize&);
    void setImageBuffer(std::unique_ptr<ImageBuffer>) const;
    void releaseImageBufferAndContext();

    bool paintsIntoCanvasBuffer() const;

#if ENABLE(WEBGL)
    bool is3D() const;
#endif

    HashSet<CanvasObserver*> m_observers;
    std::unique_ptr<CanvasRenderingContext> m_context;

    FloatRect m_dirtyRect;
    IntSize m_size;

    bool m_originClean { true };
    bool m_ignoreReset { false };

    bool m_usesDisplayListDrawing { false };
    bool m_tracksDisplayListReplay { false };

    // m_createdImageBuffer means we tried to malloc the buffer.  We didn't necessarily get it.
    mutable bool m_hasCreatedImageBuffer { false };
    mutable bool m_didClearImageBuffer { false };
    mutable std::unique_ptr<ImageBuffer> m_imageBuffer;
    mutable std::unique_ptr<GraphicsContextStateSaver> m_contextStateSaver;
    
    mutable RefPtr<Image> m_presentedImage;
    mutable RefPtr<Image> m_copiedImage; // FIXME: This is temporary for platforms that have to copy the image buffer to render (and for CSSCanvasValue).
};

} // namespace WebCore
