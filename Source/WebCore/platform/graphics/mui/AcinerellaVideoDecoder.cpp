#define SYSTEM_PRIVATE
#include "AcinerellaVideoDecoder.h"
#include "AcinerellaContainer.h"

#if ENABLE(VIDEO)

#include "GraphicsContext.h"
#include "PlatformContextCairo.h"

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <cairo.h>

#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#define __NOLIBBASE__
#include <proto/cgxvideo.h>
#undef __NOLIBBASE__
#include <cybergraphx/cgxvideo.h>
#include <graphics/rpattr.h>
#include <proto/graphics.h>
#if OS(AROS)
#include <aros/debug.h>
#undef D
#define dprintf bug
#endif

#define D(x) 
#define DSYNC(x)
#define DOVL(x)
#define DFRAME(x) 

// #pragma GCC optimize ("O0")
// #define FORCEDECODE

#if (CGX_OVERLAY)
#define CGXVideoBase m_cgxVideo
#endif

namespace WebCore {
namespace Acinerella {

AcinerellaVideoDecoder::AcinerellaVideoDecoder(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLiveStream)
	: AcinerellaDecoder(client, acinerella, buffer, index, info, isLiveStream)
{
	m_fps = info.additional_info.video_info.frames_per_second;
	m_frameDuration = 1.f / m_fps;
	m_frameWidth = info.additional_info.video_info.frame_width;
	m_frameHeight = info.additional_info.video_info.frame_height;
	D(dprintf("\033[35m[VD]%s: %p fps %f %dx%d\033[0m\n", __func__, this, float(m_fps), m_frameWidth, m_frameHeight));
	
	auto decoder = acinerella->decoder(index);
#if (CGX_OVERLAY)
	ac_set_output_format(decoder, AC_OUTPUT_YUV420P);
    m_cgxVideo = OpenLibrary("cgxvideo.library", 43);
#endif
#if (CAIRO_BLIT)
	ac_set_output_format(decoder, AC_OUTPUT_RGBA32);
#endif
	m_pullThread = Thread::create("Acinerella Video Pump", [this] {
		pullThreadEntryPoint();
	});
	
}

AcinerellaVideoDecoder::~AcinerellaVideoDecoder()
{
	D(dprintf("\033[35m[VD]%s: %p\033[0m\n", __func__, this));
#if (CGX_OVERLAY)
	if (m_overlayHandle)
	{
		DetachVLayer(m_overlayHandle);
		DeleteVLayerHandle(m_overlayHandle);
		m_overlayHandle = nullptr;
	}

    if (m_cgxVideo)
        CloseLibrary(m_cgxVideo);
#endif
	if (!!m_pullThread)
	{
		m_pullEvent.signal();
		m_pullThread->waitForCompletion();
		m_pullThread = nullptr;
	}
#if (CGX_OVERLAY)
	if (m_overlayHandle)
	{
		DetachVLayer(m_overlayHandle);
		DeleteVLayerHandle(m_overlayHandle);
	}
#endif
}

void AcinerellaVideoDecoder::onDecoderChanged(RefPtr<AcinerellaPointer> acinerella)
{
	auto decoder = acinerella->decoder(m_index);
#if (CGX_OVERLAY)
	ac_set_output_format(decoder, AC_OUTPUT_YUV420P);
#endif
#if (CAIRO_BLIT)
	ac_set_output_format(decoder, AC_OUTPUT_RGBA32);
#endif
    ac_decoder_set_loopfilter(decoder, int(m_client->streamSettings().m_loopFilter));
}

bool AcinerellaVideoDecoder::isReadyToPlay() const
{
	return isWarmedUp() && m_didShowFirstFrame;
}

bool AcinerellaVideoDecoder::isWarmedUp() const
{
	return (bufferSize() >= readAheadTime()) || m_decoderEOF;
}

bool AcinerellaVideoDecoder::isPlaying() const
{
	return m_playing;
}

double AcinerellaVideoDecoder::position() const
{
	return m_position;
}

void AcinerellaVideoDecoder::startPlaying()
{
	D(dprintf("\033[35m[VD]%s: %p\033[0m\n", __func__, this));
	if (isReadyToPlay())
	{
		m_playing = true;
		onPositionChanged();
        ac_decoder_set_loopfilter(m_lastDecoder, int(m_client->streamSettings().m_loopFilter));
		m_pullEvent.signal();
		m_frameEvent.signal();
	}
}

void AcinerellaVideoDecoder::onGetReadyToPlay()
{
	D(dprintf("\033[35m[VD]%s: %p\033[0m\n", __func__, this));
	m_client->onDecoderWantsToRender(makeRef(*this));
}

void AcinerellaVideoDecoder::stopPlaying()
{
	D(dprintf("\033[35m[VD]%s: %p\033[0m\n", __func__, this));
	m_playing = false;
	m_pullEvent.signal();

	auto lock = holdLock(m_audioLock);
	m_hasAudioPosition = false;
}

void AcinerellaVideoDecoder::onThreadShutdown()
{
	D(dprintf("\033[35m[VD]%s: %p\033[0m\n", __func__, this));
	m_pullEvent.signal();
	m_frameEvent.signal();
	D(dprintf("\033[35m[VD]%s: %p done\033[0m\n", __func__, this));
}

void AcinerellaVideoDecoder::onTerminate()
{
	D(dprintf("\033[35m[VD]%s: %p\033[0m\n", __func__, this));
	m_pullEvent.signal();
	m_frameEvent.signal();
	if (!!m_pullThread)
		m_pullThread->waitForCompletion();
	m_pullThread = nullptr;
	D(dprintf("\033[35m[VD]%s: %p done\033[0m\n", __func__, this));
}

void AcinerellaVideoDecoder::onFrameDecoded(const AcinerellaDecodedFrame &frame)
{
	m_bufferedSeconds += m_frameDuration;
	DFRAME(dprintf("\033[35m[VD]%s: %p [>> %f pts %f]\033[0m\n", __func__, this, float(m_bufferedSeconds), float(frame.pts())));
	(void)frame;
	m_pullEvent.signal();
#if (CGX_OVERLAY)
	if (!m_didShowFirstFrame && m_overlayHandle)
#endif
#if (CAIRO_BLIT)
	if (!m_didShowFirstFrame)
#endif
	{
	// caled under locks!
		showFirstFrame(false);
	}
}

void AcinerellaVideoDecoder::flush()
{
	DSYNC(dprintf("\033[35m[VD]%s: %p\033[0m\n", __func__, this));
	AcinerellaDecoder::flush();
	m_hasAudioPosition = false;
	m_bufferedSeconds = 0;
	m_frameCount = 0;
}

void AcinerellaVideoDecoder::dumpStatus()
{
	auto lock = holdLock(m_lock);
	dprintf("[\033[35mV]: WM %d IR %d PL %d BUF %f POS %f FIRSTFRAME %d DECFR %d LIVE %d\033[0m\n",
		isWarmedUp(), isReadyToPlay(), isPlaying(), float(bufferSize()), float(position()), m_didShowFirstFrame, m_decodedFrames.size(), m_isLive);
}

void AcinerellaVideoDecoder::setAudioPresentationTime(double apts)
{
	DSYNC(dprintf("\033[35m[VD]%s: %p -> %f\033[0m\n", __func__, this, float(apts)));
	float delta;
	{
		auto lock = holdLock(m_audioLock);
		m_audioPositionRealTime = MonotonicTime::now();
		delta = fabs(m_audioPosition - apts);
		m_audioPosition = apts;
		m_hasAudioPosition = true;
	}

	if (delta > 2.0)
		m_frameEvent.signal(); // abort a possible long sleep
}

bool AcinerellaVideoDecoder::getAudioPresentationTime(double &time)
{
	auto lock = holdLock(m_audioLock);

	if (m_hasAudioPosition)
	{
		// Audio should be here...
		time = m_audioPosition + (MonotonicTime::now() - m_audioPositionRealTime).value();
		return true;
	}
	
	return false;
}

void AcinerellaVideoDecoder::setOverlayWindowCoords(struct ::Window *w, int scrollx, int scrolly, int mleft, int mtop, int mright, int mbottom, int width, int height)
{
#if (CGX_OVERLAY)
	{
		auto lock = holdLock(m_lock);
		
		DOVL(dprintf("\033[35m[VD]%s: window %p %d %d %d %d\033[0m\n", __func__, w, mleft, mtop, mright, mbottom));

		(void)scrollx;
		(void)scrolly;

		m_outerX = mleft;
		m_outerY = mtop;
		m_outerX2 = mright;
		m_outerY2 = mbottom;
		m_visibleWidth = width;
		m_visibleHeight = height;

		if (w)
		{
			m_windowWidth = w->Width;
			m_windowHeight = w->Height;
		}

		if (m_overlayWindow != w)
		{
			if (m_overlayHandle)
			{
				DetachVLayer(m_overlayHandle);
				DeleteVLayerHandle(m_overlayHandle);
				m_overlayHandle = nullptr;
			}

			m_overlayWindow = w;
			m_didShowFirstFrame = false;
			
			if (m_overlayWindow && !m_terminating && m_cgxVideo)
			{
				m_overlayHandle = CreateVLayerHandleTags(m_overlayWindow->WScreen,
					VOA_SrcType, SRCFMT_YCbCr420,
					VOA_UseColorKey, TRUE,
					VOA_UseBackfill, FALSE,
					VOA_SrcWidth, m_frameWidth & -8,
					VOA_SrcHeight, m_frameHeight & -2,
					VOA_DoubleBuffer, TRUE,
					TAG_DONE);
			
				if (m_overlayHandle)
				{
					AttachVLayerTags(m_overlayHandle, m_overlayWindow, VOA_ColorKeyFill, FALSE, TAG_DONE);
					m_overlayFillColor = GetVLayerAttr(m_overlayHandle, VOA_ColorKey);
					DOVL(dprintf("\033[35m[VD]%s: fill %08lx\033[0m\n", __func__, m_overlayFillColor));
					m_pullEvent.signal();
					
					dispatch([this]() {
						showFirstFrame(true);
					});
				}
				else
				{
					dprintf("\033[35m[VD]%s: failed creating vlayer for size %d %d\033[0m\n", __func__, m_frameWidth, m_frameHeight);
				}
			}
		}
	}

	if (m_overlayHandle && m_paintX2 > 0)
	{
		updateOverlayCoords();
	}
#endif
}

void AcinerellaVideoDecoder::showFirstFrame(bool locks)
{
	bool didShowFrame = false;

	if (locks)
	{
		auto lock = holdLock(m_lock);
		if (m_decodedFrames.size())
		{
			m_position = m_decodedFrames.front().pts();
			blitFrameLocked();
			didShowFrame = true;
		}
	}
	else
	{
		if (m_decodedFrames.size())
		{
			m_position = m_decodedFrames.front().pts();
			blitFrameLocked();
			didShowFrame = true;
		}
	}

	if (didShowFrame)
	{
		m_didShowFirstFrame = true;
		onPositionChanged();
		onReadyToPlay();
	}
}

void AcinerellaVideoDecoder::onCoolDown()
{
	setOverlayWindowCoords(nullptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

void AcinerellaVideoDecoder::updateOverlayCoords()
{
#if (CGX_OVERLAY)
	int offsetX = 0;
	int offsetY = 0;

	double frameRatio = double(m_frameWidth) / double(m_frameHeight);
	double frameRevRatio = double(m_frameHeight) / double(m_frameWidth);
	double visibleRatio = double(m_visibleWidth) / double(m_visibleHeight);

	DOVL(dprintf("\033[35m[VD]%s: frameratio %f frameRevRatio %f visibleRatio %f\033[0m\n", __func__, float(frameRatio), float(frameRevRatio), float(visibleRatio)));
	DOVL(dprintf("\033[35m[VD] visibleWith %d visibleHeight %d frameWidth %d frameHeight %d\033[0m\n", m_visibleWidth, m_visibleHeight, m_frameWidth, m_frameHeight));

	if (frameRatio < visibleRatio)
	{
		offsetX = m_visibleWidth - (double(m_visibleHeight) * frameRatio);
		offsetX /= 2;
	}
	else
	{
		offsetY = m_visibleHeight - (double(m_visibleWidth) * frameRevRatio);
		offsetY /= 2;
	}
//	offsetX = 0;
//	offsetY = 0;

	auto lock = holdLock(m_lock);
	if (m_overlayHandle)
	{
		SetVLayerAttrTags(m_overlayHandle,
			VOA_LeftIndent, m_outerX + offsetX,
			VOA_TopIndent, m_outerY + offsetY,
			VOA_RightIndent, m_outerX2 + offsetX,
			VOA_BottomIndent, m_outerY2 + offsetY,
			TAG_DONE);

		DOVL(dprintf("\033[35m[VD]%s: setting vlayer bounds %d %d %d %d win %d %d\033[0m\n", __func__,
			m_outerX + offsetX,
			m_outerY + offsetY,
			m_outerX2 + offsetX,
			m_outerY2 + offsetY,
			m_overlayWindow->Width,
			m_overlayWindow->Height));
	}
#endif
}

void AcinerellaVideoDecoder::paint(GraphicsContext& gc, const FloatRect& rect)
{
#if (CGX_OVERLAY)
	WebCore::PlatformContextCairo *context = gc.platformContext();
	cairo_t* cr = context->cr();
	cairo_save(cr);
	cairo_set_source_rgb(cr, double((m_overlayFillColor >> 16) & 0xff) / 255.0, double((m_overlayFillColor >> 8) & 0xff) / 255.0,
		double(m_overlayFillColor & 0xff) / 255.0);
	cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
	cairo_fill(cr);
	cairo_restore(cr);

	bool needsToSetCoords = m_paintX != rect.x() || m_paintY != rect.y() || m_paintX2 != ((rect.x() + rect.width()) - 1) || m_paintY2 != ((rect.y() + rect.height()) - 1);

	m_paintX = rect.x();
	m_paintY = rect.y();
	m_paintX2 = (rect.x() + rect.width()) - 1;
	m_paintY2 = (rect.y() + rect.height()) - 1;

	if (needsToSetCoords && m_client)
	{
		m_client->onDecoderRenderUpdate(makeRef(*this));
	}

#if 0
// ?!? WE cannot paint since the data is in planar yuv or some other cgxvideo format

	EP_SCOPE(paint);

	auto lock = holdLock(m_lock);
	if (m_decodedFrames.size())
	{
		MonotonicTime mtStart = MonotonicTime::now();
	
		const auto *frame = m_decodedFrames.front().frame();
		WebCore::PlatformContextCairo *context = gc.platformContext();
		cairo_t* cr = context->cr();
		auto *avFrame = ac_get_frame(m_decodedFrames.front().pointer()->decoder(m_index));
		// CAIRO_FORMAT_RGB24 is actually 00RRGGBB on BigEndian

		if (rect.width() == m_frameWidth && rect.height() == m_frameHeight)
		{
			auto surface = cairo_image_surface_create_for_data(avFrame->data[0], CAIRO_FORMAT_RGB24, m_frameWidth, m_frameHeight, avFrame->linesize[0]);
			if (surface)
			{
				cairo_save(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_translate(cr, rect.x(), rect.y());
				cairo_rectangle(cr, 0, 0, rect.width(), rect.height());
				cairo_clip(cr);
				cairo_set_source_surface(cr, surface, 0, 0);
				cairo_paint(cr);
				cairo_restore(cr);
				cairo_surface_destroy(surface);
			}
		}
		else
		{
			auto surface = cairo_image_surface_create_for_data(avFrame->data[0], CAIRO_FORMAT_RGB24, m_frameWidth, m_frameHeight, avFrame->linesize[0]);
			if (surface)
			{
				cairo_save(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_translate(cr, rect.x(), rect.y());
				cairo_rectangle(cr, 0, 0, rect.width(), rect.height());
				cairo_pattern_t *pattern = cairo_pattern_create_for_surface(surface);
				if (pattern)
				{
					cairo_matrix_t matrix;
					cairo_matrix_init_scale(&matrix, double(m_frameWidth) / rect.width(), double(m_frameHeight) / rect.height());
					cairo_pattern_set_matrix(pattern, &matrix);
					cairo_pattern_set_filter(pattern, CAIRO_FILTER_FAST);
					cairo_set_source(cr, pattern);
					cairo_clip(cr);
					cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
					cairo_paint(cr);
					cairo_pattern_destroy(pattern);
				}
				cairo_restore(cr);
				cairo_surface_destroy(surface);
			}
		}
		
		MonotonicTime mtEnd = MonotonicTime::now();
		Seconds decodingTime = (mtEnd - mtStart);

		m_accumulatedCairoCount ++;
		m_accumulatedCairoTime += decodingTime;

		if (m_accumulatedCairoCount % int(m_fps))
			D(dprintf("\033[35m[VD]%s: paint time %f, avg %f\033[0m\n", __func__, float(decodingTime.value()),
				float(m_accumulatedCairoTime.value() / float(m_accumulatedCairoCount))));
				
	}
#endif
#endif
#if (CAIRO_BLIT)
	auto lock = holdLock(m_lock);
	if (m_decodedFrames.size())
	{
		WebCore::PlatformContextCairo *context = gc.platformContext();
		cairo_t* cr = context->cr();
		ac_scale_to_rgb_decoder_frame(m_decodedFrames.front().frame(), m_decodedFrames.front().pointer()->decoder(m_index));
		auto *avFrame = ac_get_frame(m_decodedFrames.front().pointer()->decoder(m_index));
		// CAIRO_FORMAT_RGB24 is actually 00RRGGBB on BigEndian

		if (rect.width() == m_frameWidth && rect.height() == m_frameHeight)
		{
			auto surface = cairo_image_surface_create_for_data(avFrame->data[0], CAIRO_FORMAT_RGB24, m_frameWidth, m_frameHeight, avFrame->linesize[0]);
			if (surface)
			{
				cairo_save(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_translate(cr, rect.x(), rect.y());
				cairo_rectangle(cr, 0, 0, rect.width(), rect.height());
				cairo_clip(cr);
				cairo_set_source_surface(cr, surface, 0, 0);
				cairo_paint(cr);
				cairo_restore(cr);
				cairo_surface_destroy(surface);
			}
		}
		else
		{
			auto surface = cairo_image_surface_create_for_data(avFrame->data[0], CAIRO_FORMAT_RGB24, m_frameWidth, m_frameHeight, avFrame->linesize[0]);
			if (surface)
			{
				cairo_save(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_translate(cr, rect.x(), rect.y());
				cairo_rectangle(cr, 0, 0, rect.width(), rect.height());
				cairo_pattern_t *pattern = cairo_pattern_create_for_surface(surface);
				if (pattern)
				{
					cairo_matrix_t matrix;
					cairo_matrix_init_scale(&matrix, double(m_frameWidth) / rect.width(), double(m_frameHeight) / rect.height());
					cairo_pattern_set_matrix(pattern, &matrix);
					cairo_pattern_set_filter(pattern, CAIRO_FILTER_FAST);
					cairo_set_source(cr, pattern);
					cairo_clip(cr);
					cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
					cairo_paint(cr);
					cairo_pattern_destroy(pattern);
				}
				cairo_restore(cr);
				cairo_surface_destroy(surface);
			}
		}
	}
#endif
}

void AcinerellaVideoDecoder::blitFrameLocked()
{
#if (CGX_OVERLAY)
	if (m_overlayHandle && m_decodedFrames.size())
	{
		auto *frame = m_decodedFrames.front().frame();
		auto *avFrame = ac_get_frame_real(frame);

		if (avFrame && ((avFrame->width != m_frameWidth) || (avFrame->height != m_frameHeight)))
		{
			if (m_frameSizeTransition)
				return;
			m_frameSizeTransition = true;

			WTF::callOnMainThread([this, width = avFrame->width, height = avFrame->height, protectedThis = makeRef(*this)]() {
				auto lock = holdLock(m_lock);
				
				m_frameWidth = width;
				m_frameHeight = height;

				if (m_overlayHandle)
				{
					DetachVLayer(m_overlayHandle);
					DeleteVLayerHandle(m_overlayHandle);
					m_overlayHandle = nullptr;

					m_overlayHandle = CreateVLayerHandleTags(m_overlayWindow->WScreen,
						VOA_SrcType, SRCFMT_YCbCr420,
						VOA_UseColorKey, TRUE,
						VOA_UseBackfill, FALSE,
						VOA_SrcWidth, m_frameWidth & -8,
						VOA_SrcHeight, m_frameHeight & -2,
						VOA_DoubleBuffer, TRUE,
						TAG_DONE);
				
					if (m_overlayHandle)
					{
						AttachVLayerTags(m_overlayHandle, m_overlayWindow, VOA_ColorKeyFill, FALSE, TAG_DONE);
						m_overlayFillColor = GetVLayerAttr(m_overlayHandle, VOA_ColorKey);
					}

				}

				m_frameSizeTransition = false;
			});

			return;
		}

		if (avFrame && avFrame->data[0] && avFrame->data[1] && avFrame->data[2] &&
			avFrame->linesize[0] > 0 && avFrame->linesize[1] > 0 && avFrame->linesize[2] > 0)
		{
			int w = m_frameWidth & -8;
			int h = m_frameHeight & -2;
			int x = 0;
			int y = 0;
		
			UBYTE *pY, *pCb, *pCr;
			UBYTE *sY, *sCb, *sCr;
			ULONG ptY, stY, ptCb, stCb, ptCr, stCr;
			ULONG w2 = w >> 1;

			h &= -2;
			w &= -2;
	
			sY   = avFrame->data[0];
			sCb  = avFrame->data[1];
			sCr  = avFrame->data[2];
			stY  = avFrame->linesize[0];
			stCb = avFrame->linesize[1];
			stCr = avFrame->linesize[2];
			
			LockVLayer(m_overlayHandle);

			ptY = GetVLayerAttr(m_overlayHandle, VOA_Modulo) >> 1;
			ptCr = ptCb = ptY >> 1;
			pY = (UBYTE *)GetVLayerAttr(m_overlayHandle, VOA_BaseAddress);
			pCb = pY + (ptY * m_frameHeight);
			pCr = pCb + ((ptCb * m_frameHeight) >> 1);
			pY += (y * ptY) + x;
			pCb += ((y * ptCb) >> 1) + (x >> 1);
			pCr += ((y * ptCr) >> 1) + (x >> 1);

			if (stY == ptY && w == m_frameWidth)
			{
				memcpy(pY, sY, ptY * h);
				memcpy(pCb, sCb, (ptCb * h) >> 1);
				memcpy(pCr, sCr, (ptCr * h) >> 1);
			}
			else do
			{
				memcpy(pY, sY, w);

				pY += ptY;
				sY += stY;

				memcpy(pY, sY, w);
				memcpy(pCb, sCb, w2);
				memcpy(pCr, sCr, w2);

				sY += stY;
				sCb += stCb;
				sCr += stCr;

				pY += ptY;
				pCb += ptCb;
				pCr += ptCr;

				h -= 2;
			} while (h > 0);

			UnlockVLayer(m_overlayHandle);
		}
	}
#endif
#warning implement
}

void AcinerellaVideoDecoder::pullThreadEntryPoint()
{
	D(dprintf("\033[36m[VD]%s: %p\033[0m\n", __func__, this));
	SetTaskPri(FindTask(0), 3);

	while (!m_terminating)
	{
		m_pullEvent.waitFor(5_s);

#ifdef FORCEDECODE
		if (m_playing && !m_terminating)
#else
#if (CGX_OVERLAY)
		if ((!m_didShowFirstFrame || m_playing) && !m_terminating && m_overlayHandle)
#endif
#if (CAIRO_BLIT)
		if ((!m_didShowFirstFrame || m_playing) && !m_terminating)
#endif
#endif
		{
			D(dprintf("\033[36m[VD]%s: %p nf\033[0m\n", __func__, this));
			bool dropFrame = false;
			bool didShowFrame = false;

			while (m_playing && !m_terminating)
			{
				Seconds sleepFor = 0_s;
				double pts;

				// Grab time point (disregarding time it takes to swap vlayer buffers)
				auto timeDisplayed = MonotonicTime::now();

				{
					auto lock = holdLock(m_lock);

					// Show previous frame
					if (dropFrame)
					{
						if (m_decodedFrames.size())
						{
							pts = m_decodedFrames.front().pts();
							if (!m_isLive)
								m_position = pts;
							m_decodedFrames.pop();
							m_bufferedSeconds -= m_frameDuration;
							dropFrame = false;
							m_frameCount++;
							m_droppedFrameCount++;
						}
						else
						{
							break;
						}
					}
					else
					{
#if (CGX_OVERLAY)
						if (m_overlayHandle)
							SwapVLayerBuffer(m_overlayHandle);
#endif
#if (CAIRO_BLIT)
						dispatch([this] {
							m_client->onDecoderWantsToRender(makeRef(*this));
						});
#endif

						if (m_decodedFrames.size())
						{
							// Store current frame's pts
							pts = m_decodedFrames.front().pts();
							if (!m_isLive)
								m_position = pts;
							
							// Blit the frame into overlay backbuffer
							blitFrameLocked();

							// Pop the frame
							m_decodedFrames.pop();
							m_bufferedSeconds -= m_frameDuration;

							m_frameCount++;
							didShowFrame = true;
						}
						else
						{
							break;
						}
					}
				}

				bool changePosition = 0 == (m_frameCount % int(m_fps));

				dispatch([this, changePosition, didShowFrame]() {
                    if (changePosition)
                    {
                        if (m_isLive)
                        {
                            m_position += 1.f;
                            onPositionChanged();
                        }
                        else
                        {
                            onPositionChanged();
                        }
#if (CGX_OVERLAY)
                        HIDInput();
#endif
                    }

					if (didShowFrame && !m_didShowFirstFrame)
					{
						m_didShowFirstFrame = true;
						onReadyToPlay();
					}

					decodeUntilBufferFull();
				});

				double audioAt;
				bool canDropFrames = false;
				
				while (m_playing && !m_terminating)
				{
					{
						auto lock = holdLock(m_lock);

						// Get next presentation time
						if (m_decodedFrames.size())
						{
							double nextPts = m_decodedFrames.front().pts();

							if (nextPts <= pts || m_isLive)
							{
								sleepFor = Seconds(m_frameDuration);
								sleepFor -= MonotonicTime::now() - timeDisplayed;
							}
							else
							{
								if (getAudioPresentationTime(audioAt))
								{
									sleepFor = Seconds((pts - audioAt) + (nextPts - pts));
									if (audioAt > 1.0)
										canDropFrames = true;
								}
								else
								{
									// Sleep for the duration of last frame - time we've already spent
									sleepFor = Seconds(nextPts - pts);
									sleepFor -= MonotonicTime::now() - timeDisplayed;
								}
							}

							DSYNC(dprintf("\033[36m[VD]%s: pts %f next frame in %f diff pts %f audio at %f\033[0m\n", __func__, float(pts), float(sleepFor.value()), float(nextPts - pts),
								float(audioAt)));
								
							break;
						}
					}

					m_pullEvent.waitFor(5_s);
				}

				if (sleepFor.value() > 0.0)
				{
					if (sleepFor.value() > 1.0)
					{
						DSYNC(dprintf("\033[36m[VD]%s: long sleep %f to catch to %f\033[0m\n", __func__, float(sleepFor.value()), float(audioAt)));
					}

					m_frameEvent.waitFor(sleepFor);
				}
				else if (m_canDropKeyFrames && canDropFrames && sleepFor.value() < -4.0)
				{
					DSYNC(dprintf("\033[36m[VD]%s: dropping video frames until %f\033[0m\n", __func__, float(audioAt)));
					dispatch([this, audioAt]() {
						dropUntilPTS(audioAt + 3.0);
					});
				}
				else if (sleepFor.value() < -(m_frameDuration * 0.1))
				{
					dropFrame = true;
				}
			}
		}

	}
}

}
}

#endif
