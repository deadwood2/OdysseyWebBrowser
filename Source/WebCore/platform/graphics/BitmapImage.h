/*
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2005, 2006, 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2008-2009 Torch Mobile, Inc.
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

#ifndef BitmapImage_h
#define BitmapImage_h

#include "Image.h"
#include "Color.h"
#include "ImageOrientation.h"
#include "ImageSource.h"
#include "IntSize.h"

#if USE(CG) || USE(APPKIT)
#include <wtf/RetainPtr.h>
#endif

#if USE(APPKIT)
OBJC_CLASS NSImage;
#endif

#if PLATFORM(WIN)
typedef struct HBITMAP__ *HBITMAP;
#endif

namespace WebCore {
    struct FrameData;
}

namespace WTF {
    template<> struct VectorTraits<WebCore::FrameData> : public SimpleClassVectorTraits {
        static const bool canInitializeWithMemset = false; // Not all FrameData members initialize to 0.
    };
}

namespace WebCore {

class Timer;

namespace NativeImage {
    IntSize size(const NativeImagePtr&);
    bool hasAlpha(const NativeImagePtr&);
    Color singlePixelSolidColor(const NativeImagePtr&);
}

// ================================================
// FrameData Class
// ================================================

struct FrameData {
public:
    FrameData()
        : m_haveMetadata(false)
        , m_isComplete(false)
        , m_hasAlpha(true)
    {
    }

    ~FrameData()
    { 
        clear(true);
    }

    // Clear the cached image data on the frame, and (optionally) the metadata.
    // Returns whether there was cached image data to clear.
    bool clear(bool clearMetadata);
    
    unsigned usedFrameBytes() const { return m_image ? m_frameBytes : 0; }

    NativeImagePtr m_image;
    ImageOrientation m_orientation { DefaultImageOrientation };
    SubsamplingLevel m_subsamplingLevel { 0 };
    float m_duration { 0 };
    bool m_haveMetadata : 1;
    bool m_isComplete : 1;
    bool m_hasAlpha : 1;
    unsigned m_frameBytes { 0 };
};

// =================================================
// BitmapImage Class
// =================================================

class BitmapImage final : public Image {
    friend class GeneratedImage;
    friend class CrossfadeGeneratedImage;
    friend class GradientImage;
    friend class GraphicsContext;
public:
    static Ref<BitmapImage> create(NativeImagePtr&& nativeImage, ImageObserver* observer = nullptr)
    {
        return adoptRef(*new BitmapImage(WTFMove(nativeImage), observer));
    }
    static Ref<BitmapImage> create(ImageObserver* observer = nullptr)
    {
        return adoptRef(*new BitmapImage(observer));
    }
#if PLATFORM(WIN)
    WEBCORE_EXPORT static RefPtr<BitmapImage> create(HBITMAP);
#endif
    virtual ~BitmapImage();
    
    bool hasSingleSecurityOrigin() const override;

    // FloatSize due to override.
    FloatSize size() const override;
    IntSize sizeRespectingOrientation() const;

    Optional<IntPoint> hotSpot() const override;

    unsigned decodedSize() const { return m_decodedSize; }

    bool dataChanged(bool allDataReceived) override;
    String filenameExtension() const override;

    // It may look unusual that there is no start animation call as public API. This is because
    // we start and stop animating lazily. Animation begins whenever someone draws the image. It will
    // automatically pause once all observers no longer want to render the image anywhere.
    void stopAnimation() override;
    void resetAnimation() override;

    void drawPattern(GraphicsContext&, const FloatRect& srcRect, const AffineTransform& patternTransform,
        const FloatPoint& phase, const FloatSize& spacing, CompositeOperator, const FloatRect& destRect, BlendMode = BlendModeNormal) override;

    // Accessors for native image formats.

#if USE(APPKIT)
    NSImage* getNSImage() override;
#endif

#if PLATFORM(COCOA)
    CFDataRef getTIFFRepresentation() override;
#endif

#if USE(CG)
    WEBCORE_EXPORT CGImageRef getCGImageRef() override;
    CGImageRef getFirstCGImageRefOfSize(const IntSize&) override;
    RetainPtr<CFArrayRef> getCGImageArray() override;
#endif

#if PLATFORM(WIN)
    bool getHBITMAP(HBITMAP) override;
    bool getHBITMAPOfSize(HBITMAP, const IntSize*) override;
#endif

#if PLATFORM(GTK)
    GdkPixbuf* getGdkPixbuf() override;
#endif

#if PLATFORM(EFL)
    Evas_Object* getEvasObject(Evas*) override;
#endif

    NativeImagePtr nativeImageForCurrentFrame() override;
    ImageOrientation orientationForCurrentFrame() override { return frameOrientationAtIndex(currentFrame()); }

    bool currentFrameKnownToBeOpaque() override;

    bool isAnimated() const override { return m_frameCount > 1; }
    
    bool canAnimate();

    void setAllowSubsampling(bool allowSubsampling) { m_source.setAllowSubsampling(allowSubsampling); }

    size_t currentFrame() const { return m_currentFrame; }
    
private:
    bool isBitmapImage() const override { return true; }

    void updateSize() const;

protected:
    enum RepetitionCountStatus {
      Unknown,    // We haven't checked the source's repetition count.
      Uncertain,  // We have a repetition count, but it might be wrong (some GIFs have a count after the image data, and will report "loop once" until all data has been decoded).
      Certain     // The repetition count is known to be correct.
    };

    WEBCORE_EXPORT BitmapImage(NativeImagePtr&&, ImageObserver* = nullptr);
    WEBCORE_EXPORT BitmapImage(ImageObserver* = nullptr);

#if PLATFORM(WIN)
    void drawFrameMatchingSourceSize(GraphicsContext&, const FloatRect& dstRect, const IntSize& srcSize, CompositeOperator) override;
#endif
    void draw(GraphicsContext&, const FloatRect& dstRect, const FloatRect& srcRect, CompositeOperator, BlendMode, ImageOrientationDescription) override;

#if USE(WINGDI)
    virtual void drawPattern(GraphicsContext&, const FloatRect& srcRect, const AffineTransform& patternTransform,
        const FloatPoint& phase, const FloatSize& spacing, CompositeOperator, const FloatRect& destRect);
#endif

    size_t frameCount();

    NativeImagePtr frameImageAtIndex(size_t, float presentationScaleHint = 1);
    NativeImagePtr copyUnscaledFrameImageAtIndex(size_t);

    bool haveFrameImageAtIndex(size_t);

    bool frameIsCompleteAtIndex(size_t);
    float frameDurationAtIndex(size_t);
    bool frameHasAlphaAtIndex(size_t);
    ImageOrientation frameOrientationAtIndex(size_t);

    // Decodes and caches a frame. Never accessed except internally.
    enum ImageFrameCaching { CacheMetadataOnly, CacheMetadataAndFrame };
    void cacheFrame(size_t index, SubsamplingLevel, ImageFrameCaching = CacheMetadataAndFrame);

    // Called before accessing m_frames[index] for info without decoding. Returns false on index out of bounds.
    bool ensureFrameIsCached(size_t index, ImageFrameCaching = CacheMetadataAndFrame);

    // Called to invalidate cached data. When |destroyAll| is true, we wipe out
    // the entire frame buffer cache and tell the image source to destroy
    // everything; this is used when e.g. we want to free some room in the image
    // cache. If |destroyAll| is false, we only delete frames up to the current
    // one; this is used while animating large images to keep memory footprint
    // low without redecoding the whole image on every frame.
    void destroyDecodedData(bool destroyAll = true) override;

    // If the image is large enough, calls destroyDecodedData() and passes
    // |destroyAll| along.
    void destroyDecodedDataIfNecessary(bool destroyAll);

    // Generally called by destroyDecodedData(), destroys whole-image metadata
    // and notifies observers that the memory footprint has (hopefully)
    // decreased by |frameBytesCleared|.
    enum class ClearedSource { No, Yes };
    void destroyMetadataAndNotify(unsigned frameBytesCleared, ClearedSource);

    // Whether or not size is available yet.
    bool isSizeAvailable();

    // Called after asking the source for any information that may require
    // decoding part of the image (e.g., the image size). We need to report
    // the partially decoded data to our observer so it has an accurate
    // account of the BitmapImage's memory usage.
    void didDecodeProperties() const;

    // Animation.
    int repetitionCount(bool imageKnownToBeComplete);  // |imageKnownToBeComplete| should be set if the caller knows the entire image has been decoded.
    bool shouldAnimate();
    void startAnimation(CatchUpAnimation = CatchUp) override;
    void advanceAnimation();

    // Function that does the real work of advancing the animation. When
    // skippingFrames is true, we're in the middle of a loop trying to skip over
    // a bunch of animation frames, so we should not do things like decode each
    // one or notify our observers.
    // Returns whether the animation was advanced.
    enum AnimationAdvancement { Normal, SkippingFramesToCatchUp };
    bool internalAdvanceAnimation(AnimationAdvancement = Normal);

    // Handle platform-specific data
    void invalidatePlatformData();

    Color singlePixelSolidColor() override;

#if !ASSERT_DISABLED
    bool notSolidColor() override;
#endif

private:
    void clearTimer();
    void startTimer(double delay);

    void dump(TextStream&) const override;

    ImageSource m_source;
    mutable IntSize m_size; // The size to use for the overall image (will just be the size of the first image).
    mutable IntSize m_sizeRespectingOrientation;

    size_t m_currentFrame { 0 }; // The index of the current frame of animation.
    Vector<FrameData, 1> m_frames; // An array of the cached frames of the animation. We have to ref frames to pin them in the cache.

    std::unique_ptr<Timer> m_frameTimer;
    int m_repetitionCount { cAnimationNone }; // How many total animation loops we should do. This will be cAnimationNone if this image type is incapable of animation.
    RepetitionCountStatus m_repetitionCountStatus { Unknown };
    int m_repetitionsComplete { 0 }; // How many repetitions we've finished.
    double m_desiredFrameStartTime { 0 }; // The system time at which we hope to see the next call to startAnimation().

#if USE(APPKIT)
    mutable RetainPtr<NSImage> m_nsImage; // A cached NSImage of frame 0. Only built lazily if someone actually queries for one.
#endif
#if USE(CG)
    mutable RetainPtr<CFDataRef> m_tiffRep; // Cached TIFF rep for frame 0. Only built lazily if someone queries for one.
#endif

    // The value of this data member is a missing value if we haven’t analyzed to check for a solid color or not, but an invalid
    // color if we have analyzed and decided it’s not a solid color, and a valid color if we have analyzed and decide that the
    // solid color optimization applies. The analysis, we do, handles only the case of 1x1 solid color images.
    Optional<Color> m_solidColor;

    unsigned m_decodedSize { 0 }; // The current size of all decoded frames.
    mutable unsigned m_decodedPropertiesSize { 0 }; // The size of data decoded by the source to determine image properties (e.g. size, frame count, etc).
    size_t m_frameCount;

#if PLATFORM(IOS)
    // FIXME: We should expose a setting to enable/disable progressive loading remove the PLATFORM(IOS)-guard.
    double m_progressiveLoadChunkTime { 0 };
    uint16_t m_progressiveLoadChunkCount { 0 };
#endif
    bool m_animationFinished : 1; // Whether or not we've completed the entire animation.

    bool m_allDataReceived : 1; // Whether or not we've received all our data.
    mutable bool m_haveSize : 1; // Whether or not our |m_size| member variable has the final overall image size yet.
    bool m_sizeAvailable : 1; // Whether or not we can obtain the size of the first image frame yet from ImageIO.
    mutable bool m_haveFrameCount : 1;
    bool m_animationFinishedWhenCatchingUp : 1;

    RefPtr<Image> m_cachedImage;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_IMAGE(BitmapImage)

#endif // BitmapImage_h
