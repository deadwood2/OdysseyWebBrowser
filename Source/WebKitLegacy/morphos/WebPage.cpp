#include "WebKit.h"

#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/BackForwardController.h>
#include <WebCore/CacheStorageProvider.h>
#include <WebCore/Chrome.h>
#include <WebCore/CookieJar.h>
#include <WebCore/DatabaseManager.h>
#include <WebCore/DeprecatedGlobalSettings.h>
#include <WebCore/Document.h>
#include <WebCore/DocumentLoader.h>
#include <WebCore/DragController.h>
#include <WebCore/DragData.h>
#include <WebCore/DragItem.h>
#include <WebCore/Editing.h>
#include <WebCore/Editor.h>
#include <WebCore/Event.h>
#include <WebCore/EventHandler.h>
#include <WebCore/FocusController.h>
#include <WebCore/FontAttributes.h>
#include <WebCore/FontCache.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameSelection.h>
#include <WebCore/FrameTree.h>
#include <WebCore/FrameView.h>
#include <WebCore/FullscreenManager.h>
#include <WebCore/GCController.h>
#include <WebCore/GeolocationController.h>
#include <WebCore/GeolocationError.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/HTMLOListElement.h>
#include <WebCore/HTMLUListElement.h>
#include <WebCore/HTMLVideoElement.h>
#include <WebCore/HistoryController.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/JSCSSStyleDeclaration.h>
#include <WebCore/JSDocument.h>
#include <WebCore/JSElement.h>
#include <WebCore/JSNodeList.h>
#include <WebCore/JSNotification.h>
#include <WebCore/LibWebRTCProvider.h>
#include <WebCore/LocalizedStrings.h>
#include <WebCore/LogInitialization.h>
#include <WebCore/MIMETypeRegistry.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/MemoryRelease.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/NodeList.h>
#include <WebCore/Notification.h>
#include <WebCore/NotificationController.h>
#include <WebCore/Page.h>
#include <WebCore/PrintContext.h>
//#include <WebCore/PageCache.h>
#include <WebCore/PageConfiguration.h>
#include <WebCore/PageGroup.h>
#include <WebCore/PathUtilities.h>
#include <WebCore/ProgressTracker.h>
#include <WebCore/RenderTheme.h>
#include <WebCore/RenderView.h>
#include <WebCore/RenderWidget.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceLoadObserver.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/RuntimeApplicationChecks.h>
#include <WebCore/RuntimeEnabledFeatures.h>
//#include <WebCore/SchemeRegistry.h>
#include <WebCore/ScriptController.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityPolicy.h>
#include <WebCore/Settings.h>
#include <WebCore/ShouldTreatAsContinuingLoad.h>
#include <WebCore/SocketProvider.h>
#include <WebCore/StyleProperties.h>
#include <WebCore/TextResourceDecoder.h>
#include <WebCore/ThreadCheck.h>
#include <WebCore/UserAgent.h>
#include <WebCore/UserContentController.h>
#include <WebCore/UserGestureIndicator.h>
#include <WebCore/UserScript.h>
#include <WebCore/UserStyleSheet.h>
#include <WebCore/ValidationBubble.h>
#include <WebCore/Widget.h>
#include <WebCore/StorageNamespaceProvider.h>
#include <WebCore/FrameLoadRequest.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/PlatformEvent.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/DeprecatedGlobalSettings.h>
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/UserInputBridge.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/EventNames.h>
#include <WebCore/WindowsKeyboardCodes.h>
#include <WebCore/RenderLayerCompositor.h>
#include <WebCore/ContextMenuController.h>
#include <WebCore/MediaRecorderProvider.h>
#include <WebCore/ScriptState.h>
#include <WebCore/AutofillElements.h>
#include <WebCore/DataTransfer.h>
#include <WebCore/Pasteboard.h>
#include <JavaScriptCore/VM.h>
#include <WebCore/CommonVM.h>
#include <WebCore/GraphicsContextCairo.h>
#include <wtf/ASCIICType.h>
#include <wtf/HexNumber.h>
#include <WebCore/DummySpeechRecognitionProvider.h>

#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/ArrayPrototype.h>
#include <JavaScriptCore/CatchScope.h>
#include <JavaScriptCore/DateInstance.h>
#include <JavaScriptCore/Exception.h>
#include <JavaScriptCore/InitializeThreading.h>
#include <JavaScriptCore/JSCJSValue.h>
#include <JavaScriptCore/JSLock.h>
#include <JavaScriptCore/JSValueRef.h>

#include <wtf/URLHelpers.h>
#include <wtf/RunLoop.h>

#include "WebPage.h"
#include "WebFrame.h"
#include "../../WebCoreSupport/PageStorageSessionProvider.h"
#include "WebCoreSupport/WebEditorClient.h"
#include "WebCoreSupport/WebChromeClient.h"
#include "WebCoreSupport/WebPluginInfoProvider.h"
#include "WebCoreSupport/WebPageGroup.h"
#include "BackForwardClient.h"
#include <WebCoreSupport/WebVisitedLinkStore.h>
#include "WebCoreSupport/WebPlatformStrategies.h"
#include "WebCoreSupport/WebInspectorClient.h"
#include "WebCoreSupport/WebFrameLoaderClient.h"
#include "WebCoreSupport/WebContextMenuClient.h"
#include "WebCoreSupport/WebProgressTrackerClient.h"
#include "WebCoreSupport/WebNotificationClient.h"
#include "../../WebCoreSupport/WebBroadcastChannelRegistry.h"
#include "WebApplicationCache.h"
#include "../../Storage/WebDatabaseProvider.h"
#include "WebDocumentLoader.h"
#include "WebDragClient.h"
#include "WebProcess.h"
#include <iostream>
#include <vector>
#include <functional>
#include <string.h>

#if ENABLE(MEDIA_STREAM)
#include "WebUserMediaClient.h"
#endif

#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-ps.h>

#include <utility>
#include <cstdio>

#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/layers.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <graphics/rpattr.h>
#include <intuition/intuition.h>
#include <intuition/pointerclass.h>
#include <intuition/intuimessageclass.h>
#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#include <devices/rawkeycodes.h>
#include <proto/openurl.h>
#include <libraries/openurl.h>

#include <libeventprofiler.h>

// we cannot include libraries/mui.h here...
enum
{
	MUIKEY_RELEASE = -2, /* not a real key, faked when MUIKEY_PRESS is released */
	MUIKEY_NONE    = -1,
	MUIKEY_PRESS,
	MUIKEY_TOGGLE,
	MUIKEY_UP,
	MUIKEY_DOWN,
	MUIKEY_PAGEUP,
	MUIKEY_PAGEDOWN,
	MUIKEY_TOP,
	MUIKEY_BOTTOM,
	MUIKEY_LEFT,
	MUIKEY_RIGHT,
	MUIKEY_WORDLEFT,
	MUIKEY_WORDRIGHT,
	MUIKEY_LINESTART,
	MUIKEY_LINEEND,
	MUIKEY_GADGET_NEXT,
	MUIKEY_GADGET_PREV,
	MUIKEY_GADGET_OFF,
	MUIKEY_WINDOW_CLOSE,
	MUIKEY_WINDOW_NEXT,
	MUIKEY_WINDOW_PREV,
	MUIKEY_HELP,
	MUIKEY_POPUP,
	MUIKEY_CUT,
	MUIKEY_COPY,
	MUIKEY_PASTE,
	MUIKEY_UNDO,
	MUIKEY_REDO,
	MUIKEY_DELETE,
	MUIKEY_BACKSPACE,
	MUIKEY_ICONIFY,
};

extern "C" {
	void dprintf(const char *, ...);
};

#define D(x) 

using namespace std;
using namespace WebCore;

namespace {

	inline constexpr int ceilingDivide(const int value, const int divider)
	{
		return 1 + ((value - 1) / divider);
	}
}

class TiledDamage
{
	static constexpr int m_tileSize = 64;
	static constexpr int m_bitDivider = 32;

	class EncapsulatingRect
	{
	public:

		EncapsulatingRect() = default;
		~EncapsulatingRect() = default;

		inline int x() const { return m_x; }
		inline int y() const { return m_y; }
		inline int width() const { return m_width; }
		inline int height() const { return m_height; }

		inline void encapsulateCoords(int x, int y, int width, int height)
		{
			if (isValid())
			{
				int maxX = std::max(x + width - 1, m_x + m_width - 1);
				m_x = std::min(m_x, x);
				m_width = (maxX - m_x) + 1;
				int maxY = std::max(y + height - 1, m_y + m_height - 1);
				m_y = std::min(m_y, y);
				m_height = (maxY - m_y) + 1;
			}
			else
			{
				m_x = x;
				m_y = y;
				m_width = width;
				m_height = height;
			}
		}
		
		inline void inflateWidthToPoint(int xPlusWidth)
		{
			m_width = xPlusWidth - m_x;
		}

		inline bool isValid() const { return m_width > 0; }

		inline void reset()
		{
			m_width = -1;
		}
		
		inline void clip(const int width, const int height)
		{
			m_width = std::min(m_width, width - m_x);
			m_height = std::min(m_height, height - m_y);
		}

	protected:
		int m_x;
		int m_y;
		int m_width = -1;
		int m_height;
	};

public:

	TiledDamage() = default;
	~TiledDamage() = default;

	inline int width() const { return m_width; }
	inline int height() const { return m_height; }
	inline int rows() const { return m_rows; }
	inline int columns() const { return m_columns; }

	void resize(const int width, const int height)
	{
		if (width != m_width || height != m_height)
		{
			m_width = width;
			m_height = height;
			
			m_rows = ceilingDivide(width, m_tileSize);
			m_columns = ceilingDivide(height, m_tileSize);
			m_cells = m_rows * m_columns;
			
			m_damage.resize(m_cells);
			for (int i = 0; i < m_cells; i++)
				m_damage[i] = true;
			
			m_damageRect.reset();
			m_damageRect.encapsulateCoords(0, 0, m_rows, m_columns);
		}
	}

	void invalidate()
	{
		for (int i = 0; i < m_cells; i++)
			m_damage[i] = true;
		m_damageRect.reset();
		m_damageRect.encapsulateCoords(0, 0, m_rows, m_columns);
	}
	
	void invalidate(int x, int y, int width, int height)
	{
		if (x < 0)
		{
			width += x;
			x = 0;
		}
		
		if (y < 0)
		{
			height += y;
			y = 0;
		}

		if (x + width > m_width)
			width = m_width - x;
		if (y + height > m_height)
			height = m_height - y;

		if (width < 0 || height < 0)
			return;

		// switch to tile bits from units...
		int maxX = ceilingDivide(x + width, m_tileSize);
		int maxY = ceilingDivide(y + height, m_tileSize);
		x /= m_tileSize;
		y /= m_tileSize;
		width = maxX - x;
		height = maxY - y;

		// so now x and y are row and column numbers...
		m_damageRect.encapsulateCoords(x, y, width, height);
		
		// damage the tiles...
		int bitIndex = x + (y * m_rows);
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
				m_damage[bitIndex + j] = true;
			bitIndex += m_rows;
		}
	}
	
	void visitDamagedTiles(std::function<void(const int x, const int y, const int width, const int height)> &&visitor)
	{
		EncapsulatingRect lastRect;
		EncapsulatingRect rect;

		for (int y = m_damageRect.y(); y < m_damageRect.y() + m_damageRect.height(); y++)
		{
			for (int x = m_damageRect.x(); x < m_damageRect.x() + m_damageRect.width(); x++)
			{
				if (m_damage[(y * m_rows) + x])
				{
					if (rect.isValid())
					{
						rect.inflateWidthToPoint(((x + 1) * m_tileSize));
					}
					else
					{
						rect.encapsulateCoords(x * m_tileSize, y * m_tileSize, m_tileSize, m_tileSize);
					}
				}
				else if (rect.isValid())
				{
					rect.clip(m_width, m_height);
					visitor(rect.x(), rect.y(), rect.width(), rect.height());
					rect.reset();
				}
			}
			
			if (rect.isValid())
			{
				if (!lastRect.isValid() || (lastRect.x() == rect.x() && lastRect.width() == rect.width()))
				{
					lastRect.encapsulateCoords(rect.x(), rect.y(), rect.width(), rect.height());
				}
				else if (lastRect.isValid())
				{
					lastRect.clip(m_width, m_height);
					visitor(lastRect.x(), lastRect.y(), lastRect.width(), lastRect.height());
					lastRect.reset();
					lastRect.encapsulateCoords(rect.x(), rect.y(), rect.width(), rect.height());
				}
				
				rect.reset();
			}
		}

		if (lastRect.isValid())
		{
			lastRect.clip(m_width, m_height);
			visitor(lastRect.x(), lastRect.y(), lastRect.width(), lastRect.height());
		}
	}
	
	void clear()
	{
		m_damageRect.reset();
		for (int i = 0; i < m_cells; i++)
			m_damage[i] = false;
	}
	
	bool hasDamage() const { return m_damageRect.isValid(); }

protected:
	std::vector<bool> m_damage;
	EncapsulatingRect m_damageRect;
	int m_width = 0;
	int m_height = 0;
	int m_rows;
	int m_columns;
	int m_cells;
};

namespace WebKit {

class DeferredPageDestructor {
public:
    static void createDeferredPageDestructor(std::unique_ptr<WebCore::Page> page)
    {
        new DeferredPageDestructor(WTFMove(page));
    }

private:
    DeferredPageDestructor(std::unique_ptr<WebCore::Page> page)
        : m_page(WTFMove(page))
    {
        tryDestruction();
    }

    void tryDestruction()
    {
        if (m_page->insideNestedRunLoop()) {
            m_page->whenUnnested([this] { tryDestruction(); });
            return;
        }
		D(dprintf("%s bye\n", __PRETTY_FUNCTION__));
        m_page = nullptr;
        delete this;
    }

    std::unique_ptr<WebCore::Page> m_page;
};

} // namespace WebKit

namespace WebKit {

class MediaRecorderProvider final : public WebCore::MediaRecorderProvider {
public:
    MediaRecorderProvider() = default;
};

class WebViewDrawContext
{
	int m_width = -1;
	int m_height = -1;
	int m_scrollY = 0;
	bool m_partialDamage = false;
	bool m_didScroll = false;

	TiledDamage m_damage;

	cairo_surface_t *m_surface = nullptr ;
	cairo_t *m_cairo = nullptr ;
	WebCore::GraphicsContextCairo *m_platformContext = nullptr;

public:
	WebViewDrawContext(const int width, const int height)
		: m_surface(nullptr)
		, m_cairo(nullptr)
		, m_platformContext(nullptr)
	{
		resize(width, height);
	}
	
	~WebViewDrawContext()
	{
		if (m_platformContext)
			delete m_platformContext;
		if (m_cairo)
			cairo_destroy(m_cairo);
		if (m_surface)
			cairo_surface_destroy(m_surface);
	}
	
	int width() const { return m_width; }
	int height() const { return m_height; }
	void onDidScroll()
	{
		m_didScroll = true;
	}

	void invalidate(const WebCore::IntRect& rect)
	{
		EP_SCOPE(invalidate);
		int width = rect.width();
		int height = rect.height();
		
		// WebCore does this for some reason...
		if (width == 0 || height == 0)
			return;

		if (width < m_width || height < m_height)
			m_partialDamage = true;

		m_damage.invalidate(rect.x(), rect.y(), rect.width(), rect.height());
	}
	
	void invalidate()
	{
		EP_SCOPE(invalidateall);
		m_damage.invalidate();
	}
	
	void repair(WebCore::FrameView *frameView, WebCore::InterpolationQuality interpolation)
	{
		EP_SCOPE(repair);
		
		if (WebCore::InterpolationQuality::Default != interpolation)
		{
			m_platformContext->setImageInterpolationQuality(interpolation);
		}

		m_damage.visitDamagedTiles([&](const int x, const int y, const int width, const int height) {
			EP_SCOPE(tile)
			WebCore::IntRect ir(x, y, width, height);
			/// NOTE: bad shit happens when clipping is used w/o save/restore, cairo seems to be happily
			/// trashing memory w/o this
			m_platformContext->save();
			/// NOTE: clipping IS important. WebCore will paint whole elements that overlap the paint area
			/// in their actual bounds otherwise, but will not paint any children that do not (so a button
			/// frame overlapping would clear button text if the text wasn't overlapping)
			m_platformContext->clip(WebCore::FloatRect(x, y, width, height));
			EP_BEGIN(paint);
			frameView->paint(*m_platformContext, ir);
			EP_END(paint);
			m_platformContext->restore();
		});
	}
	
	void repaint(RastPort *rp, const int outX, const int outY)
	{
		EP_SCOPE(repaint);
		cairo_surface_flush(m_surface);
		const unsigned int stride = cairo_image_surface_get_stride(m_surface);
		unsigned char *src = cairo_image_surface_get_data(m_surface);

		m_damage.visitDamagedTiles([&](const int x, const int y, const int width, const int height) {
			EP_SCOPE(wpa);
			WritePixelArray(src, x, y, stride, rp, outX + x, outY + y, width, height, RECTFMT_ARGB);
		});
	}
	
	void repaintAll(RastPort *rp, const int outX, const int outY)
	{
		EP_SCOPE(repaint);
		cairo_surface_flush(m_surface);
		const unsigned int stride = cairo_image_surface_get_stride(m_surface);
		unsigned char *src = cairo_image_surface_get_data(m_surface);
		EP_SCOPE(wpa);
		WritePixelArray(src, 0, 0, stride, rp, outX, outY, m_width, m_height, RECTFMT_ARGB);
	}

	void draw(WebCore::FrameView *frameView, RastPort *rp, const int x, const int y, const int width, const int height,
		int scrollX, int scrollY, bool update, WebCore::InterpolationQuality interpolation)
	{
		if (!m_platformContext)
			return;
		EP_SCOPE(draw);

		(void)scrollX;
		(void)scrollY;
		(void)width;
		(void)height;

#if 0 // doesn't repaint correctly
		struct Window *window = (struct Window *)rp->Layer->Window;

		// Only trigger fast path if we've scrolled from outside (by scroller, etc) and there was
		// no partial damage done to the site (meaning some components have displaced)
		if (m_scrollY != scrollY && update && window && !m_partialDamage && m_didScroll)
		{
			int delta = scrollY - m_scrollY;
			m_scrollY = scrollY;

			if (abs(delta) < m_height)
			{
				LockLayerUpdates(rp->Layer);
			
				if (delta > 0)
					m_damage.invalidate(0, m_height - delta, m_width, delta);
				else
					m_damage.invalidate(0, 0, m_width, -delta);

				repair(frameView, interpolation);
				ScrollWindowRaster(window, 0, delta, x, y, x + width - 1, y + height - 1);

				cairo_surface_flush(m_surface);
				const unsigned int stride = cairo_image_surface_get_stride(m_surface);
				unsigned char *src = cairo_image_surface_get_data(m_surface);

				if (delta > 0)
					WritePixelArray(src, 0, height - delta, stride, rp, x, y + height - delta, width, delta, RECTFMT_ARGB);
				else
					WritePixelArray(src, 0, 0, stride, rp, x, y, width, -delta, RECTFMT_ARGB);
				
				UnlockLayerUpdates(rp->Layer);
				m_damage.invalidate();
				m_partialDamage = false;
				m_didScroll = false;
				return;
			}
		}
#endif

// dprintf("paint %d @ %d %d %d %d\n", update, x, y, m_width, m_height);

		repair(frameView, interpolation);
		if (update)
			repaint(rp, x, y);
		else
			repaintAll(rp, x, y);
		m_damage.clear();
		m_partialDamage = false;
		m_didScroll = false;
	}

	bool resize(const int width, const int height)
	{
		EP_SCOPE(resize);
		if (width != m_width || height != m_height)
		{
			if (m_platformContext)
				delete m_platformContext;
			if (m_cairo)
				cairo_destroy(m_cairo);
			if (m_surface)
				cairo_surface_destroy(m_surface);

			m_surface = nullptr;
			m_cairo = nullptr;
			m_platformContext = nullptr;
			
			m_width = width;
			m_height = height;
			
			if ((m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height)))
			{
				if ((m_cairo = cairo_create(m_surface)))
				{
					if (!(m_platformContext = new WebCore::GraphicsContextCairo(m_cairo)))
					{
						cairo_destroy(m_cairo);
						cairo_surface_destroy(m_surface);
						m_surface = nullptr;
						m_cairo = nullptr;
					}
					
					m_damage.resize(m_width, m_height);
					return true;
				}
				else
				{
					cairo_surface_destroy(m_surface);
					m_surface = nullptr;
				}
			}
		}
		
		return false;
	}


#if 0
	void drawLayer(WebCore::GraphicsLayer *layer, WebCore::GraphicsContext& gc, WebCore::FloatRect &fr)
	{
		layer->paintGraphicsLayerContents(gc, fr);
		
		const Vector<Ref<GraphicsLayer>>& children = layer->children();
		for (auto it = children.begin(); it != children.end(); it++)
		{
			WebCore::GraphicsLayer *child = it->ptr();
			dprintf("child %p pos %f %f size %f %f bounds %f %f backing %p\n", child, child->position().x(), child->position().y(),
				child->size().width(), child->size().height(), child->boundsOrigin().x(), child->boundsOrigin().y(), child->tiledBacking());
			WebCore::FloatRect xfr(fr.x() + child->position().x(), fr.y() + child->position().y(), child->size().width(), child->size().height());
			drawLayer(child, gc, xfr);
		}
	}
#endif

private:

};

static bool needsToRotatePageForPrinting(bool landscape, int pagesPerSheet)
{
	(void)landscape;
	if (pagesPerSheet == 2 || pagesPerSheet == 6)
		return true;
	return false;
}

static int numColumnsForPrinting(bool landscape, int pagesPerSheet)
{
	switch (pagesPerSheet)
	{
	case 9:
		return 3;
	case 6:
		return landscape ? 2 : 3;
	case 4:
		return 2;
	case 2:
		return landscape ? 1 : 2;
	default:
		return 1;
	}
}

static int numRowsForPrinting(bool landscape, int pagesPerSheet)
{
	switch (pagesPerSheet)
	{
	case 9:
		return 3;
	case 6:
		return landscape ? 3 : 2;
	case 4:
		return 2;
	case 2:
		return landscape ? 2 : 1;
	default:
		return 1;
	}
}


class WebViewPrintingContext
{
public:
	WebViewPrintingContext() = default;
	~WebViewPrintingContext()
	{
		if (_surface)
			cairo_surface_destroy(_surface);
		if (_surfaceScaled)
			cairo_surface_destroy(_surfaceScaled);
		if (_printCairo)
			cairo_destroy(_printCairo);
		if (_printSurface)
			cairo_surface_destroy(_printSurface);
		if (_psFile)
			Close(_psFile);
	}

	cairo_surface_t *surface() { return _surface; }
	int width() { return _surface ? cairo_image_surface_get_width(_surface) : -1; }
	int height() { return _surface ? cairo_image_surface_get_height(_surface) : -1; }

	bool ensureSurface(int width, int height, int forPage)
	{
		if (width == this->width() && height == this->height() && forPage == _selectedPage)
			return false;

		if (_surface)
			cairo_surface_destroy(_surface);

		if (_surfaceScaled)
			cairo_surface_destroy(_surfaceScaled);
		_surfaceScaled = nullptr;

		_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		_selectedPage = forPage;
		return true;
	}
	
	static cairo_status_t pswritefunc(void *closure, const unsigned char *data, unsigned int length)
	{
		BPTR out = BPTR(closure);
		Write(out, APTR(data), length);
		return CAIRO_STATUS_SUCCESS;
	}
	
	bool ensurePrintSurface(float width, float height, bool landscape, int psLevel, LONG pagesPerSheet, const char *file, WebCore::FloatBoxExtent& margins)
	{
		if (_printCairo)
			cairo_destroy(_printCairo);
		_printCairo = nullptr;
		
		if (_printSurface)
			cairo_surface_destroy(_printSurface);

		_margins = margins;
		_landscape = landscape;
		_printWidth = width;
		_printHeight = height;
		_pps = pagesPerSheet;
		_isPDF = false;

		_psFile = Open(file, MODE_NEWFILE);
		
		if (_psFile != 0)
		{
			if (needsToRotatePageForPrinting(landscape, pagesPerSheet))
				_printSurface = cairo_ps_surface_create_for_stream(pswritefunc, (void *)_psFile, height + margins.top() + margins.bottom(), width + margins.left() + margins.right());
			else
				_printSurface = cairo_ps_surface_create_for_stream(pswritefunc, (void *)_psFile, width + margins.left() + margins.right(), height + margins.top() + margins.bottom());

			if (_printSurface)
			{
				cairo_ps_surface_restrict_to_level(_printSurface, psLevel == 2 ? CAIRO_PS_LEVEL_2 : CAIRO_PS_LEVEL_3);
				_printCairo = cairo_create(_printSurface);
				if (_printCairo)
					return true;
			}
			Close(_psFile);
			_psFile = 0;
		}
		
		return false;
	}

	bool ensurePrintSurface(float width, float height, bool landscape, LONG pagesPerSheet, const char *file, WebCore::FloatBoxExtent& margins)
	{
		if (_printCairo)
			cairo_destroy(_printCairo);
		_printCairo = nullptr;
		
		if (_printSurface)
			cairo_surface_destroy(_printSurface);
		
		if (_psFile)
			Close(_psFile);

		_margins = margins;
		_landscape = landscape;
		_printWidth = width;
		_printHeight = height;
		_pps = pagesPerSheet;
		_isPDF = true;

		_printSurface = cairo_pdf_surface_create(file, width + margins.left() + margins.right(), height + margins.top() + margins.bottom());
		if (_printSurface)
		{
			_printCairo = cairo_create(_printSurface);
			if (_printCairo)
				return true;
		}
		return false;
	}
	
	void startPage(float fullwidth, float fullheight)
	{
		if (_isPDF)
		{
			cairo_pdf_surface_set_size(_printSurface, fullwidth, fullheight);
		}
		else
		{
			if (fullwidth > fullheight)
			{
				cairo_ps_surface_set_size(_printSurface, fullheight, fullwidth);
				cairo_ps_surface_dsc_begin_page_setup (_printSurface);
				cairo_ps_surface_dsc_comment(_printSurface, "%%PageOrientation: Landscape");
			}
			else
			{
				cairo_ps_surface_set_size(_printSurface, fullwidth, fullheight);
				cairo_ps_surface_dsc_begin_page_setup (_printSurface);
				cairo_ps_surface_dsc_comment(_printSurface, "%%PageOrientation: Portrait");
			}
		}
	}

	void endPage()
	{
		cairo_show_page(_printCairo);
	}
	
	float pageWidth() { return _printWidth; }
	float pageHeight() { return _printHeight; }

	cairo_t *printCairo() { return _printCairo; }
	cairo_surface_t *printSurface() { return _printSurface; }
	WebCore::FloatBoxExtent margins() { return _margins; }
	bool landscape() { return _landscape; }
	LONG pagesPerSheet() { return _pps; }
	bool isPDF() { return _isPDF; }
	bool isPS() { return _psFile != 0; }

	cairo_surface_t *scaledSurface(int width, int height)
	{
		if (!_surface)
			return nullptr;
		if (_surfaceScaled && cairo_image_surface_get_width(_surfaceScaled) == width && cairo_image_surface_get_height(_surfaceScaled) == height)
			return _surfaceScaled;
		if (_surfaceScaled)
			cairo_surface_destroy(_surfaceScaled);
		_surfaceScaled = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		if (_surfaceScaled)
		{
			auto cairo = cairo_create(_surfaceScaled);
			if (cairo)
			{
				cairo_save(cairo);
				cairo_set_source_rgba(cairo, 0, 0, 0, 0);
				cairo_rectangle(cairo, 0, 0, width, height);
				cairo_scale(cairo, ((double)width) / ((double)this->width()), ((double)height) / ((double)this->height()));
				cairo_pattern_set_filter(cairo_get_source(cairo), CAIRO_FILTER_GOOD);
				cairo_set_source_surface(cairo, _surface, 0, 0);
				cairo_paint(cairo);
				cairo_surface_flush(_surfaceScaled);
				cairo_destroy(cairo);
			}
			else
			{
				if (_surfaceScaled)
					cairo_surface_destroy(_surfaceScaled);
				_surfaceScaled = nullptr;
			}
		}
		return _surfaceScaled;
	}
	
protected:
	int _selectedPage { -1 };
	float _printWidth, _printHeight;
	bool _landscape { false };
	bool _isPDF { false };
	BPTR _psFile { 0 };
	LONG _pps;
	cairo_surface_t *_surface { nullptr };
	cairo_surface_t *_surfaceScaled { nullptr };
	cairo_surface_t *_printSurface { nullptr };
	cairo_t *_printCairo { nullptr };
	WebCore::FloatBoxExtent _margins;
};

static void doPrint(float printableWidth, float printableHeight, WebCore::FloatBoxExtent &computedMargins, LONG sheet, LONG pagesPerSheet, bool landscape, bool isPreview, cairo_t *cairo, WebViewPrintingContext *printingContext, WebCore::PrintContext *context)
{
	cairo_set_source_rgb(cairo, 1.0, 1.0, 1.0);
	cairo_paint(cairo);

	WebCore::GraphicsContextCairo gc(cairo);

	gc.setImageInterpolationQuality(WebCore::InterpolationQuality::High);

	int numColumns = numColumnsForPrinting(landscape, pagesPerSheet);
	int numRows = numRowsForPrinting(landscape, pagesPerSheet);
	bool needsToRotate = needsToRotatePageForPrinting(landscape, pagesPerSheet);

	float printScale;
	float printScaleXt;

	if (needsToRotate)
	{
		printScale = (printableHeight - (14.4f * float(numColumns - 1))) / float(numColumns) / context->pageRect(sheet * pagesPerSheet).width();
		printScaleXt = (printableWidth - (14.4f * float(numRows - 1))) / float(numRows) / context->pageRect(sheet * pagesPerSheet).height();
	}
	else
	{
		printScale = (printableWidth - (14.4f * float(numColumns - 1))) / float(numColumns) / context->pageRect(sheet * pagesPerSheet).width();
		printScaleXt = (printableHeight - (14.4f * float(numRows - 1))) / float(numRows) / context->pageRect(sheet * pagesPerSheet).height();
	}

	if (printScaleXt < printScale)
		printScale = printScaleXt;

	float translateMarginLeft;
	float translateMarginTop;
	float translateColumn;
	float translateRow;

	if (landscape)
	{
		if (needsToRotate)
		{
			translateMarginLeft = computedMargins.left() / printScale;
			translateMarginTop = computedMargins.top() / printScale;

			translateColumn = (printableHeight / printScale / float(numColumns)) + (7.2f / printScale);
			translateRow = (printableWidth / printScale / float(numRows)) + (7.2f / printScale);
		}
		else
		{
			translateMarginLeft = computedMargins.top() / printScale;
			translateMarginTop = computedMargins.right() / printScale;

			translateColumn = (printableWidth / printScale / float(numRows)) + (7.2f / printScale);
			translateRow = (printableHeight / printScale / float(numColumns)) + (7.2f / printScale);
		}
	}
	else
	{

		if (needsToRotate)
		{
			translateMarginLeft = computedMargins.top() / printScale;
			translateMarginTop = computedMargins.left() / printScale;

			translateColumn = (printableHeight / printScale / float(numColumns)) + (7.2f / printScale);
			translateRow = (printableWidth / printScale / float(numRows)) + (7.2f / printScale);
		}
		else
		{
			translateMarginLeft = computedMargins.left() / printScale;
			translateMarginTop = computedMargins.top() / printScale;

			translateColumn = (printableWidth / printScale / float(numRows)) + (7.2f / printScale);
			translateRow = (printableHeight / printScale / float(numColumns)) + (7.2f / printScale);
		}
	}

	int printIndex = 0;
	for (int i = sheet * pagesPerSheet; i < (sheet + 1) * pagesPerSheet; i++)
	{
		cairo_matrix_t matrix;
		if (i >= int(context->pageCount()))
			break;
		int column = printIndex % numColumns;
		int row = printIndex / numColumns;

		gc.save();
		
		if (((landscape && !needsToRotate) || (!landscape && needsToRotate)) && printingContext->isPS() && !isPreview)
		{
			if (needsToRotate)
				cairo_translate(cairo, 0, printableHeight + computedMargins.top() + computedMargins.bottom());
			else
				cairo_translate(cairo, 0, printableWidth + computedMargins.top() + computedMargins.bottom());
			cairo_matrix_init (&matrix, 0, -1, 1, 0, 0,  0);
			cairo_transform(cairo, &matrix);
		}
		
		gc.scale(printScale);

		gc.translate(translateMarginLeft + (translateColumn * float(column)), translateMarginTop + (translateRow * float(row)));
		context->spoolPage(gc, i, context->pageRect(i).width());
		
		printIndex ++;
		gc.restore();
	}

}

WebCore::Page* core(WebPage *webpage)
{
	if (webpage)
    	return webpage->corePage();
	return nullptr;
}

WebCore::Frame& mainframe(WebCore::Page& page)
{
	return page.mainFrame();
}

const WebCore::Frame& mainframe(const WebCore::Page& page)
{
	return page.mainFrame();
}

WebPage *kit(WebCore::Page* page)
{
	return WebPage::fromCorePage(page);
}

Ref<WebPage> WebPage::create(WebCore::PageIdentifier pageID, WebPageCreationParameters&& parameters)
{
    Ref<WebPage> page = adoptRef(*new WebPage(pageID, WTFMove(parameters)));
    return page;
}

WebPage::WebPage(WebCore::PageIdentifier pageID, WebPageCreationParameters&& parameters)
	: m_mainFrame(WebFrame::create())
	, m_pageID(pageID)
{
	(void)parameters;

//	WebCore::DeprecatedGlobalSettings::setUsesOverlayScrollbars(true);
    static bool didOneTimeInitialization;
    if (!didOneTimeInitialization) {
		JSC::initialize();
		WTF::initializeMainThread();
		WebCore::NetworkStorageSession::permitProcessToUseCookieAPI(true);

#if !LOG_DISABLED || !RELEASE_LOG_DISABLED
        initializeLogChannelsIfNecessary();
#endif // !LOG_DISABLED || !RELEASE_LOG_DISABLED

        // Initialize our platform strategies first before invoking the rest
        // of the initialization code which may depend on the strategies.
        WebPlatformStrategies::initialize();

        auto& memoryPressureHandler = MemoryPressureHandler::singleton();
        memoryPressureHandler.setLowMemoryHandler([] (Critical critical, Synchronous synchronous) {
            WebCore::releaseMemory(critical, synchronous);
        });
        memoryPressureHandler.install();

        didOneTimeInitialization = true;
     }

	m_webPageGroup = WebPageGroup::getOrCreate("meh", "PROGDIR:Cache/Storage");
	auto storageProvider = PageStorageSessionProvider::create();

#if 0
        [[self preferences] privateBrowsingEnabled] ? PAL::SessionID::legacyPrivateSessionID() : PAL::SessionID::defaultSessionID(),
#endif
	WebCore::PageConfiguration pageConfiguration(
		WebProcess::singleton().sessionID(),
        makeUniqueRef<WebEditorClient>(this),
        WebCore::SocketProvider::create(),
        makeUniqueRef<WebCore::LibWebRTCProvider>(),
        WebProcess::singleton().cacheStorageProvider(),
        m_webPageGroup->userContentController(),
        BackForwardClientMorphOS::create(this),
        WebCore::CookieJar::create(storageProvider.copyRef()),
        makeUniqueRef<WebProgressTrackerClient>(*this),
        makeUniqueRef<WebFrameLoaderClient>(m_mainFrame.copyRef()),
        makeUniqueRef<WebCore::DummySpeechRecognitionProvider>(),
        makeUniqueRef<MediaRecorderProvider>(),
        WebBroadcastChannelRegistry::getOrCreate(false),
        WebCore::DummyPermissionController::create()
        );

	pageConfiguration.chromeClient = new WebChromeClient(*this);
	pageConfiguration.inspectorClient = new WebInspectorClient(this);
//    pageConfiguration.loaderClientForMainFrame = new WebFrameLoaderClient();
    pageConfiguration.storageNamespaceProvider = &m_webPageGroup->storageNamespaceProvider();
    pageConfiguration.visitedLinkStore = &m_webPageGroup->visitedLinkStore();
    pageConfiguration.pluginInfoProvider = &WebPluginInfoProvider::singleton();
    pageConfiguration.applicationCacheStorage = &WebApplicationCache::storage();
    pageConfiguration.databaseProvider = &WebDatabaseProvider::singleton();
	pageConfiguration.contextMenuClient = new WebContextMenuClient(this);
	pageConfiguration.dragClient = makeUnique<WebDragClient>(this);

//dprintf("%s:%d chromeclient %p\n", __PRETTY_FUNCTION__, __LINE__, pageConfiguration.chromeClient);

	m_page = new WebCore::Page(WTFMove(pageConfiguration));
	storageProvider->setPage(*m_page);

	WebCore::Settings& settings = m_page->settings();
    settings.setAllowDisplayOfInsecureContent(false);
    settings.setAllowRunningOfInsecureContent(false);
    settings.setLoadsImagesAutomatically(true);
    settings.setScriptEnabled(true);
    settings.setScriptMarkupEnabled(true);
    settings.setDeferredCSSParserEnabled(true);
    settings.setDeviceWidth(1920);
    settings.setDeviceHeight(1080);
//    settings.setEnforceCSSMIMETypeInNoQuirksMode(true);
    settings.setShrinksStandaloneImagesToFit(true);
    settings.setSubpixelAntialiasedLayerTextEnabled(true);
    settings.setAuthorAndUserStylesEnabled(true);
    settings.setFixedFontFamily("Courier New");
    settings.setDefaultFixedFontSize(13);
    settings.setResizeObserverEnabled(true);
	settings.setEditingBehaviorType(EditingBehaviorType::Unix);
	settings.setShouldRespectImageOrientation(true);
	settings.setTextAreasAreResizable(true);
	settings.setIntersectionObserverEnabled(true);
	settings.setDataTransferItemsEnabled(true);

#if 1
	settings.setForceCompositingMode(false);
	settings.setAcceleratedCompositingEnabled(false);
	settings.setAcceleratedDrawingEnabled(false);
    settings.setCanvasColorSpaceEnabled(true);
	// settings.setAccelerated2dCanvasEnabled(false);
	settings.setAcceleratedCompositedAnimationsEnabled(false);
	settings.setAcceleratedCompositingForFixedPositionEnabled(false);
	settings.setAcceleratedFiltersEnabled(false);
//    settings.setFrameFlattening(FrameFlattening::FullyEnabled);
#else
    settings.setFrameFlattening(FrameFlattening::FullyEnabled);
#endif

//	settings.setTreatsAnyTextCSSLinkAsStylesheet(true);
//	settings.setUsePreHTML5ParserQuirks(true);

	settings.setWebGLEnabled(false);


//     settings.setStorageBlockingPolicy(SecurityOrigin::StorageBlockingPolicy::BlockAllStorage);
	settings.setLocalStorageDatabasePath(String("PROGDIR:Cache/LocalStorage"));
#if (!MORPHOS_MINIMAL)
	settings.setWebAudioEnabled(true);
//	settings.setAudioWorkletEnabled(true);
//	settings.setModernUnprefixedWebAudioEnabled(true);
//	settings.setPrefixedWebAudioEnabled(true);
	settings.setMediaEnabled(true);
	settings.setLocalStorageEnabled(true);
	settings.setOfflineWebApplicationCacheEnabled(true);
	settings.setMaximumSourceBufferSize(32 * 1024 * 1024);
#endif

// 	settings.setDeveloperExtrasEnabled(true);
	settings.setXSSAuditorEnabled(true);
	settings.setVisualViewportAPIEnabled(true);

	settings.setHiddenPageCSSAnimationSuspensionEnabled(true);
	settings.setAnimatedImageAsyncDecodingEnabled(false);

    settings.setWebAnimationsCompositeOperationsEnabled(true);
    settings.setWebAnimationsMutableTimelinesEnabled(true);
    settings.setCSSCustomPropertiesAndValuesEnabled(true);

	settings.setViewportFitEnabled(true);
	settings.setConstantPropertiesEnabled(true);

#if ENABLE(FULLSCREEN_API)
       settings.setFullScreenEnabled(true);
#endif

	// the default
	settings.setTouchEventEmulationEnabled(false);

// crashy
//    settings.setDiagnosticLoggingEnabled(true);
//	settings.setLogsPageMessagesToSystemConsoleEnabled(true);
	
	settings.setRequestAnimationFrameEnabled(true);
	settings.setUserStyleSheetLocation(WTF::URL(WTF::URL(), WTF::String("file:///PROGDIR:Resources/morphos.css")));

#if ENABLE(VIDEO)
       settings.setInvisibleAutoplayNotPermitted(true);
       settings.setAudioPlaybackRequiresUserGesture(true);
       settings.setVideoPlaybackRequiresUserGesture(true);
#endif

#if ENABLE(MEDIA_STREAM)
    WebCore::provideUserMediaTo(m_page, new WebUserMediaClient(*this));
    settings.setMediaStreamEnabled(true);
    settings.setMediaDevicesEnabled(true);
#endif

#if ENABLE(NOTIFICATIONS)
    WebCore::provideNotification(m_page, new WebNotificationClient(this));
#endif

	m_page->effectiveAppearanceDidChange(m_darkMode, false);

    // m_mainFrame = WebFrame::createWithCoreMainFrame(this, &m_page->mainFrame());
//    static_cast<WebFrameLoaderClient&>(m_page->mainFrame().loader().client()).setWebFrame(m_mainFrame.get());

	m_mainFrame->initWithCoreMainFrame(*this, m_page->mainFrame());
    m_page->layoutIfNeeded();

    m_page->setIsVisible(true);
    m_page->setIsInWindow(true);
	m_page->setActivityState(ActivityState::WindowIsActive);
}

WebPage::~WebPage()
{
	D(dprintf("%s(%p)\n", __PRETTY_FUNCTION__, this));
	clearDelegateCallbacks();
	delete m_printingContext;
	delete m_drawContext;
	delete m_autofillElements;
}

WebCore::Page *WebPage::corePage()
{
	return m_page;
}

const WebCore::Page *WebPage::corePage() const
{
	return m_page;
}

WebPage* WebPage::fromCorePage(WebCore::Page* page)
{
	if (page)
		return &static_cast<WebChromeClient&>(page->chrome().client()).page();
	return nullptr;
}

void WebPage::load(const char *url, bool ignoreCaches)
{
	static uint64_t navid = 1;

    auto* coreFrame = m_mainFrame->coreFrame();
	WTF::URL baseCoreURL = WTF::URL(WTF::URL(), WTF::String(url));
	WebCore::ResourceRequest request(baseCoreURL);
	
	if (ignoreCaches)
		request.setCachePolicy(ResourceRequestCachePolicy::ReloadIgnoringCacheData);

	corePage()->userInputBridge().stopLoadingFrame(*coreFrame);
	m_pendingNavigationID = navid ++;
	coreFrame->loader().load(FrameLoadRequest(*coreFrame, request));
	exitFullscreen();

//    coreFrame->loader().urlSelected(baseCoreURL, { }, nullptr, LockHistory::No, LockBackForwardList::No, MaybeSendReferrer, ShouldOpenExternalURLsPolicy::ShouldNotAllow);
}

void WebPage::loadData(const char *data, size_t length, const char *url)
{
	WTF::URL baseURL = url ? WTF::URL(WTF::URL(), WTF::String(url)) : WTF::aboutBlankURL();

    ResourceRequest request(baseURL);
    ResourceResponse response(WTF::aboutBlankURL(), "text/html", length, "UTF-8");
    SubstituteData substituteData(WebCore::SharedBuffer::create(data, length), WTF::aboutBlankURL(), response, SubstituteData::SessionHistoryVisibility::Hidden);

	auto* coreFrame = m_mainFrame->coreFrame();
    coreFrame->loader().load(FrameLoadRequest(*coreFrame, request, substituteData));
	exitFullscreen();
}

bool WebPage::reload(const char *url)
{
	auto *mainframe = mainFrame();
	if (mainframe)
	{
		WTF::URL expectedURL = WTF::URL(WTF::URL(), WTF::String(url));
		
		exitFullscreen();

		OptionSet<ReloadOption> options;
		options.add(ReloadOption::FromOrigin);

		DocumentLoader *loader = mainframe->loader().documentLoader();
		
		bool canReload = loader && loader->request().url() == expectedURL;
		
		if (canReload)
		{
			mainframe->loader().reload(options);
		}
		else
		{
			load(url, true);
		}

		return true;
	}
	
	return false;
}

void WebPage::stop()
{
	auto *mainframe = mainFrame();
	if (mainframe)
		mainframe->loader().stopForUserCancel();
	exitFullscreen();
}

WebCore::CertificateInfo WebPage::getCertificate(void)
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame)
		return { };

    DocumentLoader* documentLoader = coreFrame->loader().documentLoader();
    if (!documentLoader)
        return { };

    return valueOrCompute(documentLoader->response().certificateInfo(), [] { return CertificateInfo(); });
}

void WebPage::run(const char *js)
{
	WTF::RefPtr<WebPage> protect(this);

	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame)
		return;

	coreFrame->script().executeScriptIgnoringException(js, true);
}

void *WebPage::evaluate(const char *js, WTF::Function<void *(const char *)>&& cb)
{
	WTF::RefPtr<WebPage> protect(this);

	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame)
		return nullptr;

	auto result = coreFrame->script().executeScriptIgnoringException(js, true);
	if (!m_mainFrame->coreFrame() || !result || (!result.isBoolean() && !result.isString() && !result.isNumber()))
	{
		return cb("");
	}

    auto state = mainWorldExecState(coreFrame);
    JSC::JSLockHolder lock(state);
	WTF::String string = result.toWTFString(state);
	auto ustring = string.utf8();
	return cb(ustring.data());
}

void *WebPage::getInnerHTML(WTF::Function<void *(const char *)>&& cb)
{
    WebCore::Frame* coreFrame = m_mainFrame->coreFrame();
	if (coreFrame)
	{
		WTF::String inner = coreFrame->document()->documentElement()->innerHTML();
		auto uinner = inner.utf8();
		return cb(uinner.data());
	}
	
	return nullptr;
}

void WebPage::setInnerHTML(const char *html)
{
    WebCore::Frame* coreFrame = m_mainFrame->coreFrame();
	if (coreFrame)
	{
		coreFrame->document()->documentElement()->setInnerHTML(WTF::String::fromUTF8(html));
	}
}

bool WebPage::goBack()
{
	return m_page->backForward().goBack();
}

bool WebPage::goForward()
{
	return m_page->backForward().goForward();
}

bool WebPage::canGoBack()
{
	return m_page->backForward().canGoBackOrForward(-1);
}

bool WebPage::canGoForward()
{
	return m_page->backForward().canGoBackOrForward(1);
}

void WebPage::goToItem(WebCore::HistoryItem& item)
{
	m_page->goToItem(item, FrameLoadType::IndexedBackForward, ShouldTreatAsContinuingLoad::No);
}

WTF::RefPtr<WebKit::BackForwardClientMorphOS> WebPage::backForwardClient()
{
	Ref<BackForwardClientMorphOS> client(static_cast<BackForwardClientMorphOS&>(m_page->backForward().client()));
	return client;
}

void WebPage::willBeDisposed()
{
	m_orphaned = true;
	auto *mainframe = mainFrame();
	D(dprintf("%s: mf %p\n", __PRETTY_FUNCTION__, mainframe));
	exitFullscreen();
	clearDelegateCallbacks();
//	stop();
	if (mainframe)
		mainframe->loader().detachFromParent();
		
    auto* page = m_page;
    m_page = nullptr;
    WebKit::DeferredPageDestructor::createDeferredPageDestructor(std::unique_ptr<WebCore::Page>(page));
		
	D(dprintf("%s done mf %p\n", __PRETTY_FUNCTION__, mainframe));
}

Frame* WebPage::mainFrame() const
{
    return m_page ? &m_page->mainFrame() : nullptr;
}

FrameView* WebPage::mainFrameView() const
{
    if (Frame* frame = mainFrame())
        return frame->view();
	
    return nullptr;
}

PAL::SessionID WebPage::sessionID() const
{
	return m_page->sessionID();
}

bool WebPage::javaScriptEnabled() const
{
	return m_page->settings().isScriptEnabled();
}

void WebPage::setJavaScriptEnabled(bool enabled)
{
	return m_page->settings().setScriptEnabled(enabled);
}

bool WebPage::adBlockingEnabled() const
{
	return m_adBlocking;
}

void WebPage::setAdBlockingEnabled(bool enabled)
{
	m_adBlocking = enabled;
}

bool WebPage::touchEventsEnabled() const
{
	return m_page->settings().isTouchEventEmulationEnabled();
}

void WebPage::setTouchEventsEnabled(bool enabled)
{
	m_page->settings().setTouchEventEmulationEnabled(enabled);
}

bool WebPage::thirdPartyCookiesAllowed() const
{
	return m_page->settings().isThirdPartyCookieBlockingDisabled();
}

void WebPage::setThirdPartyCookiesAllowed(bool blocked)
{
	m_page->settings().setIsThirdPartyCookieBlockingDisabled(blocked);
}

bool WebPage::darkModeEnabled() const
{
	return m_darkMode;
}

void WebPage::setDarkModeEnabled(bool enabled)
{
	if (enabled != m_darkMode)
	{
		m_darkMode = enabled;
		m_page->effectiveAppearanceDidChange(m_darkMode, false);
	}
}

void WebPage::setRequiresUserGestureForMediaPlayback(bool requiresGesture)
{
#if ENABLE(VIDEO)
	m_page->settings().setAudioPlaybackRequiresUserGesture(requiresGesture);
	m_page->settings().setVideoPlaybackRequiresUserGesture(requiresGesture);
#else
	(void)requiresGesture;
#endif
}

bool WebPage::requiresUserGestureForMediaPlayback()
{
#if ENABLE(VIDEO)
	return m_page->settings().audioPlaybackRequiresUserGesture();
#else
	return true;
#endif
}

void WebPage::setInvisiblePlaybackNotAllowed(bool invisible)
{
#if ENABLE(VIDEO)
	m_page->settings().setInvisibleAutoplayNotPermitted(invisible);
#else
	(void)invisible;
#endif
}

bool WebPage::invisiblePlaybackNotAllowed()
{
#if ENABLE(VIDEO)
	return m_page->settings().invisibleAutoplayNotPermitted();
#else
	return true;
#endif
}

void WebPage::goActive()
{
	corePage()->userInputBridge().focusSetActive(true);
	corePage()->userInputBridge().focusSetFocused(true);
	m_justWentActive = true;
	m_isActive = true;
}

void WebPage::goInactive()
{
	m_justWentActive = false;
	m_isActive = false;
	corePage()->userInputBridge().focusSetFocused(false);
}

void WebPage::goVisible()
{
	m_isVisible = true;
	corePage()->setIsVisible(true);
	corePage()->userInputBridge().focusSetActive(true);
}

void WebPage::goHidden()
{
	m_isVisible = false;
	corePage()->setIsVisible(false);
	corePage()->userInputBridge().focusSetActive(false);
}

void WebPage::setLowPowerMode(bool lowPowerMode)
{
	corePage()->setLowPowerModeEnabledOverrideForTesting(lowPowerMode);
}

bool WebPage::localStorageEnabled()
{
	WebCore::Settings& settings = m_page->settings();
	return settings.localStorageEnabled();
}

void WebPage::setLocalStorageEnabled(bool enabled)
{
	WebCore::Settings& settings = m_page->settings();
	settings.setLocalStorageEnabled(enabled);
}

bool WebPage::offlineCacheEnabled()
{
	WebCore::Settings& settings = m_page->settings();
	return settings.offlineWebApplicationCacheEnabled();
}

void WebPage::setOfflineCacheEnabled(bool enabled)
{
	WebCore::Settings& settings = m_page->settings();
	settings.setOfflineWebApplicationCacheEnabled(enabled);
}

void WebPage::startLiveResize()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	coreFrame->view()->willStartLiveResize();
}

void WebPage::endLiveResize()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	coreFrame->view()->willEndLiveResize();
	corePage()->setIsVisible(true);
	corePage()->userInputBridge().focusSetActive(true);
}

void WebPage::setFocusedElement(WebCore::Element *element)
{
	// this is called by the Chrome
	if (element)
		m_focusedElement = makeRef(*element);
	else
		m_focusedElement = nullptr;
}

void WebPage::setFullscreenElement(WebCore::Element *element)
{
	if (element)
	{
		m_fullscreenElement = makeRef(*element);
        m_fullscreenElement->document().fullscreenManager().willEnterFullscreen(*m_fullscreenElement);
        m_fullscreenElement->document().fullscreenManager().didEnterFullscreen();

		if (_fZoomChangedByWheel)
			_fZoomChangedByWheel();
			
		if (_fEnterFullscreen)
		{
			_fEnterFullscreen();

			if (m_drawContext)
				m_drawContext->invalidate();
		}
    }
	else
	{
		if (m_fullscreenElement)
		{
			m_fullscreenElement->document().fullscreenManager().willExitFullscreen();
			m_fullscreenElement->document().fullscreenManager().didExitFullscreen();

            if (_fZoomChangedByWheel)
                _fZoomChangedByWheel();
                
			if (_fExitFullscreen)
			{
				_fExitFullscreen();

				if (m_drawContext)
					m_drawContext->invalidate();
			}
		}
		
		m_fullscreenElement = nullptr;
	}
}

WebCore::FullscreenManager* WebPage::fullscreenManager()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (coreFrame)
		return &coreFrame->document()->fullscreenManager();
	return nullptr;
}

bool WebPage::isFullscreen() const
{
	return nullptr != m_fullscreenElement;
}

void WebPage::exitFullscreen()
{
	setFullscreenElement(nullptr);
}

WebCore::IntRect WebPage::getElementBounds(WebCore::Element *e)
{
	if (e && e->isConnected())
		return e->boundsInRootViewSpace();
	return { };
}

void WebPage::startedEditingElement(WebCore::HTMLInputElement *input)
{
	if (nullptr == input)
		return;
	if (nullptr == m_autofillElements)
		m_autofillElements = new WebCore::AutofillElements();
	if (m_autofillElements)
	{
		if (m_autofillElements->computeAutofillElements(*input))
		{
			if (_fHasAutofill)
				_fHasAutofill();
		}
		else
		{
			delete m_autofillElements;
			m_autofillElements = nullptr;
		}
	}
}

bool WebPage::hasAutofillElements()
{
	if (m_autofillElements)
		return true;
	return false;
}

void WebPage::clearAutofillElements()
{
	if (m_autofillElements)
		delete m_autofillElements;
	m_autofillElements = nullptr;
}

void WebPage::setAutofillElements(const WTF::String &login, const WTF::String &password)
{
	if (m_autofillElements)
		m_autofillElements->autofill(login, password);
}

bool WebPage::getAutofillElements(WTF::String &outlogin, WTF::String &outPassword)
{
	if (m_autofillElements)
	{
		if (m_autofillElements->username())
		{
			outlogin = m_autofillElements->username()->value();
		}
		
		if (m_autofillElements->password())
		{
			outPassword = m_autofillElements->password()->value();
		}
		
		return true;
	}

	return false;
}

void WebPage::setCursor(int cursor)
{
	if (m_cursor != cursor)
	{
		m_cursor = cursor;

		_fSetCursor(mouseCursorToSet(0, true));
	}
}

void WebPage::addResourceRequest(unsigned long identifier, const WebCore::ResourceRequest& request)
{
    if (!request.url().protocolIsInHTTPFamily())
        return;

    if (m_mainFrameProgressCompleted || m_orphaned)
        return;

    ASSERT(!m_trackedNetworkResourceRequestIdentifiers.contains(identifier));
    bool wasEmpty = m_trackedNetworkResourceRequestIdentifiers.isEmpty();
    m_trackedNetworkResourceRequestIdentifiers.add(identifier);

    if (wasEmpty && _fDidStartLoading)
    	_fDidStartLoading();
}

void WebPage::removeResourceRequest(unsigned long identifier)
{
    if (!m_trackedNetworkResourceRequestIdentifiers.remove(identifier))
    {
        return;
	}

	if (m_trackedNetworkResourceRequestIdentifiers.isEmpty() && _fDidStopLoading)
	{
		_fDidStopLoading();
	}
}

void WebPage::didStartPageTransition()
{
	m_transitioning = true;
}

void WebPage::didCompletePageTransition()
{
}

void WebPage::didCommitLoad(WebFrame& frame)
{
//    resetFocusedElementForFrame(frame);

    if (!frame.isMainFrame())
    {
        return;
    }

    m_transitioning = false;

#if 0
    // If previous URL is invalid, then it's not a real page that's being navigated away from.
    // Most likely, this is actually the first load to be committed in this page.
    if (frame->coreFrame()->loader().previousURL().isValid())
        reportUsedFeatures();
#endif

    // Only restore the scale factor for standard frame loads (of the main frame).
    if (frame.coreFrame()->loader().loadType() == WebCore::FrameLoadType::Standard) {
        Page* page = frame.coreFrame()->page();

        if (page && page->pageScaleFactor() != 1)
            scalePage(1, IntPoint());
    }

#if 0
#if ENABLE(VIEWPORT_RESIZING)
    m_shrinkToFitContentTimer.stop();
#endif

#if ENABLE(TEXT_AUTOSIZING)
    m_textAutoSizingAdjustmentTimer.stop();
#endif

    WebProcess::singleton().updateActivePages();

    updateMainFrameScrollOffsetPinning();

    updateMockAccessibilityElementAfterCommittingLoad();
#endif
}

void WebPage::didFinishDocumentLoad(WebFrame& frame)
{
    if (!frame.isMainFrame())
    {
        return;
	}

	corePage()->resumeActiveDOMObjectsAndAnimations();

#if ENABLE(VIEWPORT_RESIZING) && 0
    scheduleShrinkToFitContent();
#endif
}

void WebPage::didFinishLoad(WebFrame& frame)
{
    if (!frame.isMainFrame())
        return;

#if ENABLE(VIEWPORT_RESIZING) && 0
    scheduleShrinkToFitContent();
#endif
}

void WebPage::didFailLoad(const WebCore::ResourceError& error)
{
	if (_fDidFailWithError)
		_fDidFailWithError(error);
}

Ref<DocumentLoader> WebPage::createDocumentLoader(Frame& frame, const ResourceRequest& request, const SubstituteData& substituteData)
{
    Ref<WebDocumentLoader> documentLoader = WebDocumentLoader::create(request, substituteData);

    if (frame.isMainFrame()) {
        if (m_pendingNavigationID) {
            documentLoader->setNavigationID(m_pendingNavigationID);
            m_pendingNavigationID = 0;
        }

#if 0
        if (m_pendingWebsitePolicies) {
            WebsitePoliciesData::applyToDocumentLoader(WTFMove(*m_pendingWebsitePolicies), documentLoader);
            m_pendingWebsitePolicies = WTF::nullopt;
        }
#endif
    }

    return documentLoader;
}

void WebPage::updateCachedDocumentLoader(WebDocumentLoader& documentLoader, Frame& frame)
{
    if (m_pendingNavigationID && frame.isMainFrame()) {
        documentLoader.setNavigationID(m_pendingNavigationID);
        m_pendingNavigationID = 0;
    }
}

WebCore::IntSize WebPage::size() const
{
	if (m_drawContext)
		return WebCore::IntSize(m_drawContext->width(), m_drawContext->height());
	return WebCore::IntSize();
}

double WebPage::totalScaleFactor() const
{
    return m_page->pageScaleFactor();
}

double WebPage::pageScaleFactor() const
{
    return totalScaleFactor() / viewScaleFactor();
}

double WebPage::viewScaleFactor() const
{
    return m_page->viewScaleFactor();
}

float WebPage::pageZoomFactor() const
{
	auto* coreFrame = m_mainFrame->coreFrame();
	return coreFrame->pageZoomFactor();
}

float WebPage::textZoomFactor() const
{
	auto* coreFrame = m_mainFrame->coreFrame();
	return coreFrame->textZoomFactor();
}

void WebPage::setPageAndTextZoomFactors(float pageZoomFactor, float textZoomFactor)
{
	auto* coreFrame = m_mainFrame->coreFrame();
	coreFrame->setPageAndTextZoomFactors(pageZoomFactor, textZoomFactor);
}

void WebPage::scalePage(double scale, const IntPoint& origin)
{
    double totalScale = scale * viewScaleFactor();
    bool willChangeScaleFactor = totalScale != totalScaleFactor();

    m_page->setPageScaleFactor(totalScale, origin);

    // We can't early return before setPageScaleFactor because the origin might be different.
    if (!willChangeScaleFactor)
    {
        return;
	}

	if (m_drawContext)
	{
		m_drawContext->invalidate();
	}
	
	if (_fInvalidate)
	{
		_fInvalidate(false);
	}
}

void WebPage::setAlwaysShowsHorizontalScroller(bool alwaysShowsHorizontalScroller)
{
    if (alwaysShowsHorizontalScroller == m_alwaysShowsHorizontalScroller)
        return;

    m_alwaysShowsHorizontalScroller = alwaysShowsHorizontalScroller;
    auto view = corePage()->mainFrame().view();
    if (!alwaysShowsHorizontalScroller)
        view->setHorizontalScrollbarLock(false);
    view->setHorizontalScrollbarMode(alwaysShowsHorizontalScroller ? ScrollbarAlwaysOn : m_mainFrameIsScrollable ? ScrollbarAuto : ScrollbarAlwaysOff, alwaysShowsHorizontalScroller || !m_mainFrameIsScrollable);
}

void WebPage::setAlwaysShowsVerticalScroller(bool alwaysShowsVerticalScroller)
{
    if (alwaysShowsVerticalScroller == m_alwaysShowsVerticalScroller)
        return;

    m_alwaysShowsVerticalScroller = alwaysShowsVerticalScroller;
    auto view = corePage()->mainFrame().view();
    if (!alwaysShowsVerticalScroller)
        view->setVerticalScrollbarLock(false);
    view->setVerticalScrollbarMode(alwaysShowsVerticalScroller ? ScrollbarAlwaysOn : m_mainFrameIsScrollable ? ScrollbarAuto : ScrollbarAlwaysOff, alwaysShowsVerticalScroller || !m_mainFrameIsScrollable);
}

void WebPage::repaint(const WebCore::IntRect& rect)
{
	if (!m_drawContext)
		return;

	WebCore::IntRect realRect(0, 0, m_drawContext->width(), m_drawContext->height());
	realRect.intersect(rect);

//	if (rect.x() < 0)
//	DumpTaskState(FindTask(0));

#if 0
	// dprintf("%s %ld:%ld x %ld:%ld\n", __PRETTY_FUNCTION__, rect.x(), rect.y(), rect.width(), rect.height());
	dprintf("%s %ld:%ld x %ld:%ld\n", __PRETTY_FUNCTION__, realRect.x(), realRect.y(),
		realRect.width(), realRect.height());
#endif

	if (m_drawContext)
		m_drawContext->invalidate(realRect);

	if (_fInvalidate)
		_fInvalidate(false);
}

void WebPage::internalScroll(int scrollX, int scrollY)
{
	(void)scrollX;
	(void)scrollY;

	if (!m_ignoreScroll)
	{
		auto* coreFrame = m_mainFrame->coreFrame();
		if (!coreFrame)
			return;
		WebCore::FrameView *view = coreFrame->view();
		WebCore::ScrollPosition sp = view->scrollPosition();
		if (_fScroll)
			_fScroll(sp.x(), sp.y());
		if (_fInvalidate)
			_fInvalidate(false);
	}
}

void WebPage::frameSizeChanged(WebCore::Frame& frame, int width, int height)
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame)
		return;
	WebCore::FrameView *view = coreFrame->view();

	if (coreFrame == &frame)
	{
		WebCore::ScrollPosition sp = view->scrollPosition();
		if (_fSetDocumentSize)
			_fSetDocumentSize(width, height);
		if (_fScroll)
			_fScroll(sp.x(), sp.y());
	}
}

void WebPage::closeWindow()
{

}

void WebPage::closeWindowSoon()
{
	delete m_drawContext;
	m_drawContext = nullptr;
}

void WebPage::closeWindowTimerFired()
{

}

void WebPage::setVisibleSize(const int width, const int height)
{
	bool resized = false;
	if (nullptr == m_drawContext)
	{
		m_drawContext = new WebViewDrawContext(width, height);
		resized = true;
	}
	else
	{
		resized = m_drawContext->resize(width, height);
	}

	if (resized)
	{

  		auto* coreFrame = m_mainFrame->coreFrame();
//  		coreFrame->document()->updateStyleIfNeeded();
		coreFrame->view()->resize(width, height);
		coreFrame->view()->availableContentSizeChanged(WebCore::ScrollableArea::AvailableSizeChangeReason::AreaSizeChanged);
		coreFrame->view()->updateLayoutAndStyleIfNeededRecursive();

//		corePage()->mainFrame().view()->enableAutoSizeMode(true, { width, height });
#if 0
		WebCore::FrameView* view = m_page->mainFrame().view();
		if (view)
		{
			view->resize(width, height);
		}

  		auto* coreFrame = m_mainFrame->coreFrame();

		if (coreFrame && coreFrame->view())
		{
			WebCore::FloatSize logicalSize(width, height);
			auto clientRect = enclosingIntRect(WebCore::FloatRect(WebCore::FloatPoint(), logicalSize));
			coreFrame->view()->resize(clientRect.size());
		}
#endif
	}
}

void WebPage::setScroll(const int x, const int y)
{
	if (!m_drawContext)
		return;
	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame)
		return;
	WebCore::FrameView *view = coreFrame->view();

	m_ignoreScroll = true;
	view->setScrollPosition(WebCore::ScrollPosition(x, y));
	m_drawContext->onDidScroll();
	m_ignoreScroll = false;

	if (_fInvalidate)
		_fInvalidate(false);
}

int WebPage::scrollLeft()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame)
		return 0;
	WebCore::FrameView *view = coreFrame->view();
	return view->scrollPosition().x();
}

int WebPage::scrollTop()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame)
		return 0;
	WebCore::FrameView *view = coreFrame->view();
	return view->scrollPosition().y();
}

void WebPage::scrollBy(int xDelta, int yDelta, WebPage::WebPageScrollByMode mode, WebCore::Frame *inFrame)
{
	auto* coreFrame = inFrame ? inFrame : m_mainFrame->coreFrame();
	if (!coreFrame)
		return;
	WebCore::FrameView *view = coreFrame->view();
	WebCore::ScrollPosition sp = view->scrollPosition();
	WebCore::ScrollPosition spMin = view->minimumScrollPosition();
	WebCore::ScrollPosition spMax = view->maximumScrollPosition();

	if (mode == WebPageScrollByMode::Pages)
	{
		xDelta *= Scrollbar::pageStep(view->visibleWidth());
		yDelta *= Scrollbar::pageStep(view->visibleHeight());
	}
	else if (mode == WebPageScrollByMode::Units)
	{
		xDelta *= Scrollbar::pixelsPerLineStep();
		yDelta *= Scrollbar::pixelsPerLineStep();
	}

	int x = sp.x() - xDelta;
	int y = sp.y() - yDelta;

	x = std::max(x, spMin.x());
	x = std::min(x, spMax.x());
	
	y = std::max(y, spMin.y());
	y = std::min(y, spMax.y());

	view->setScrollPosition(WebCore::ScrollPosition(x, y));

	if (_fInvalidate)
		_fInvalidate(false);
}

void WebPage::wheelScrollOrZoomBy(const int xDelta, const int yDelta, ULONG qualifiers, WebCore::Frame *inFrame)
{
	if (qualifiers & IEQUALIFIER_CONTROL)
	{
		float factor = pageZoomFactor();
		if (yDelta < 0)
			factor -= 0.05;
		else
			factor += 0.05;
		setPageAndTextZoomFactors(factor, textZoomFactor());
		
		if (_fZoomChangedByWheel)
			_fZoomChangedByWheel();
	}
	else
	{
		scrollBy(xDelta, yDelta, WebPageScrollByMode::Units, inFrame);
	}
}

void WebPage::draw(struct RastPort *rp, const int x, const int y, const int width, const int height, bool updateMode)
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame || !m_drawContext)
	{
		return;
	}
	
    WebCore::FrameView* frameView = coreFrame->view();
    if (!frameView)
	{
		return;
	}

    m_page->updateRendering();
	m_page->finalizeRenderingUpdate({ });
	
	if (m_needsCompositingFlush)
	{
		m_needsCompositingFlush = false;

		OptionSet<FinalizeRenderingUpdateFlags> flags;
		m_page->finalizeRenderingUpdate(flags);

		coreFrame->view()->availableContentSizeChanged(WebCore::ScrollableArea::AvailableSizeChangeReason::AreaSizeChanged);
		coreFrame->view()->updateLayoutAndStyleIfNeededRecursive();
	}

#if 0
	frameView->updateLayoutAndStyleIfNeededRecursive();
//	frameView->updateCompositingLayersAfterLayout();
	frameView->setPaintBehavior(PaintBehavior::FlattenCompositingLayers);
#endif
//	IntSize s = frameView->autoSizingIntrinsicContentSize();
	auto scroll = frameView->scrollPosition();

//	dprintf("draw to %p at %d %d : %dx%d, %d %d renderable %d\n", rp, x,y, width, height, s.width(), s.height(), frameView->isSoftwareRenderable());

	// FrameTree& tree = coreFrame->tree();

#if 0
    for (Frame* child = tree.firstRenderedChild(); child; child = child->tree().traverseNextRendered(coreFrame)) {
        if (!child->view())
            continue;
		dprintf("maybe render child frame %p\n", child);
    }
#endif

#if 0
	if (frameView->renderView())
	{
    	dprintf("rv compositior 3d %d rgl %p\n", frameView->renderView()->compositor().has3DContent(), frameView->renderView()->compositor().rootGraphicsLayer());
    	if (frameView->renderView()->compositor().rootGraphicsLayer())
    	{
    		auto *rlayer = frameView->renderView()->compositor().rootGraphicsLayer();
    		const Vector<Ref<GraphicsLayer>>& children = rlayer->children();
    		for (auto it = children.begin(); it != children.end(); it++)
    		{
    			const FloatPoint position = it->ptr()->position();
				dprintf("child layer %p at %f %f\n", it->ptr(), position.x(), position.y());
			}
		}
	}
#endif

	auto interpolation = m_interpolation;
	if (frameView->frame().document()->isImageDocument())
		interpolation = m_imageInterpolation;

	m_drawContext->draw(frameView, rp, x, y, width, height, scroll.x(), scroll.y(), updateMode, interpolation);
}

void WebPage::printPreview(struct RastPort *rp, const int x, const int y, const int paintWidth, const int paintHeight,
	LONG sheet, LONG pagesPerSheet, float printableWidth, float printableHeight, bool landscape,
	const WebCore::FloatBoxExtent& margins, WebCore::PrintContext *context, bool printBackgrounds)
{
	if (!context || context->pageCount() < 1)
	{
		SetRPAttrs(rp, RPTAG_FgColor, 0x909090, RPTAG_PenMode, FALSE, TAG_DONE);
		RectFill(rp, x, y, x + paintWidth - 1, y + paintHeight - 1);
		return;
	}

	m_page->updateRendering();
	m_page->finalizeRenderingUpdate({ });

	// this is the visible area we want to paint in
	float width = paintWidth;
	float height = paintHeight;

	// leave out some margins...
	width *= 0.8f;
	height *= 0.8f;

	// check the page margins...
	WebCore::FloatBoxExtent computedMargins = context->computedPageMargin(margins);

	// the surface in which the page will be printed to
	float surfaceWidthF = printableWidth + computedMargins.left() + computedMargins.right();
	float surfaceHeightF = printableHeight + computedMargins.top() + computedMargins.bottom();

	if (landscape)
	{
		std::swap(printableWidth, printableHeight);
	 	surfaceWidthF = printableWidth + computedMargins.top() + computedMargins.bottom();
		surfaceHeightF = printableHeight + computedMargins.left() + computedMargins.right();
	}
	
	bool needsToRotate = needsToRotatePageForPrinting(landscape, pagesPerSheet);
	
	if (needsToRotate)
	{
		if (landscape)
		{
	 		surfaceHeightF = printableWidth + computedMargins.top() + computedMargins.bottom();
			surfaceWidthF = printableHeight + computedMargins.left() + computedMargins.right();
		}
		else
		{
			surfaceHeightF = printableWidth + computedMargins.left() + computedMargins.right();
			surfaceWidthF = printableHeight + computedMargins.top() + computedMargins.bottom();
		}
	}
	
	// Given width and height, calculate a scaling factor so that the whole page fits
	// inside the given constraints. May want to add some margins while at it too...
	// This is the surface will paint to the rastport
	float scaleWidth = width / surfaceWidthF;
	float scaleHeight = height / surfaceHeightF;
	float scale = scaleWidth < scaleHeight ? scaleWidth : scaleHeight;

	auto scaledSize = WebCore::ceiledIntSize(WebCore::LayoutSize(surfaceWidthF * scale, surfaceHeightF * scale));

	bool repaintSurface = false;
	
	if (!m_printingContext)
	{
		m_printingContext = new WebViewPrintingContext();
		if (!m_printingContext)
		{
			SetRPAttrs(rp, RPTAG_FgColor, 0x909090, RPTAG_PenMode, FALSE, TAG_DONE);
			RectFill(rp, x, y, x + paintWidth - 1, y + paintHeight - 1);
			return;
		}
		repaintSurface = true;
	}
	
	if (m_printingContext->ensureSurface(ceilf(surfaceWidthF), ceilf(surfaceHeightF), sheet * pagesPerSheet))
	{
		repaintSurface = true;
	}

	WebCore::Settings& settings = m_page->settings();
	settings.setShouldPrintBackgrounds(printBackgrounds);

	if (repaintSurface)
	{
		auto *surface = m_printingContext->surface();
		auto *cairo = cairo_create(surface);

		if (cairo)
		{
//			cairo_set_source_rgb(cairo, 0.0f, 1.0, 1.0);
			doPrint(printableWidth, printableHeight, computedMargins, sheet, pagesPerSheet, landscape, true, cairo, m_printingContext, context);

			cairo_surface_flush(surface);
			cairo_destroy(cairo);
		}
	}

	SetRPAttrs(rp, RPTAG_FgColor, 0x909090, RPTAG_PenMode, FALSE, TAG_DONE);
	RectFill(rp, x, y, x + paintWidth - 1, y + paintHeight - 1);

	auto *surface = m_printingContext->scaledSurface(scaledSize.width(), scaledSize.height());
	
	const unsigned int stride = cairo_image_surface_get_stride(surface);
	unsigned char *src = cairo_image_surface_get_data(surface);

	SetRPAttrs(rp, RPTAG_FgColor, 0, RPTAG_PenMode, FALSE, TAG_DONE);

	int paintX = x + ((paintWidth - scaledSize.width()) / 2);
	int paintY = y + ((paintHeight - scaledSize.height()) / 2);

	RectFill(rp, paintX - 1, paintY - 1, paintX + scaledSize.width(), paintY - 1);
	RectFill(rp, paintX - 1, paintY + scaledSize.height(), paintX + scaledSize.width(), paintY + scaledSize.height());

	RectFill(rp, paintX - 1, paintY - 1, paintX - 1, paintY + scaledSize.height() - 1);
	RectFill(rp, paintX + scaledSize.width(), paintY - 1, paintX + scaledSize.width(), paintY + scaledSize.height() - 1);

	WritePixelArray(src, 0, 0, stride, rp, paintX, paintY,
		scaledSize.width(), scaledSize.height(), RECTFMT_ARGB);

}

void WebPage::printStart(float pageWidth, float pageHeight, bool landscape, LONG pagesPerSheet,
	WebCore::FloatBoxExtent margins, WebCore::PrintContext *context,
	int psLevel, bool printBackgrounds, const char *file)
{
	(void)context;

	if (!m_printingContext)
	{
		m_printingContext = new WebViewPrintingContext();
	}

	if (!m_printingContext)
	{
		return;
	}

	WebCore::Settings& settings = m_page->settings();
	settings.setShouldPrintBackgrounds(printBackgrounds);

	m_printingContext->ensurePrintSurface(pageWidth, pageHeight, landscape, psLevel, pagesPerSheet, file, margins);
}

void WebPage::pdfStart(float pageWidth, float pageHeight, bool landscape, LONG pagesPerSheet,
	WebCore::FloatBoxExtent margins, WebCore::PrintContext *context, bool printBackgrounds, const char *file)
{
	(void)context;

	if (!m_printingContext)
	{
		m_printingContext = new WebViewPrintingContext();
	}

	if (!m_printingContext)
	{
		return;
	}

	WebCore::Settings& settings = m_page->settings();
	settings.setShouldPrintBackgrounds(printBackgrounds);

	m_printingContext->ensurePrintSurface(pageWidth, pageHeight, landscape, pagesPerSheet, file, margins);
}

// O0 or crashes. wtf?
#pragma GCC push_options
#pragma GCC optimize ("O0")

bool WebPage::printSpool(WebCore::PrintContext *context, int sheetNoZeroBased)
{
	if (m_printingContext && m_printingContext->printCairo())
	{
		auto *cairo = m_printingContext->printCairo();

		WebCore::FloatBoxExtent computedMargins = context->computedPageMargin(m_printingContext->margins());

		float printableWidth = m_printingContext->pageWidth();
		float printableHeight = m_printingContext->pageHeight();
		bool landscape = m_printingContext->landscape();
		LONG pagesPerSheet = m_printingContext->pagesPerSheet();

		// the surface in which the page will be printed to
		float surfaceWidthF = printableWidth + computedMargins.left() + computedMargins.right();
		float surfaceHeightF = printableHeight + computedMargins.top() + computedMargins.bottom();

		if (m_printingContext->landscape())
		{
			std::swap(printableWidth, printableHeight);
			surfaceWidthF = printableWidth + computedMargins.top() + computedMargins.bottom();
			surfaceHeightF = printableHeight + computedMargins.left() + computedMargins.right();
		}
		
		bool needsToRotate = needsToRotatePageForPrinting(landscape, pagesPerSheet);
		
		if (needsToRotate)
		{
			if (landscape)
			{
				surfaceHeightF = printableWidth + computedMargins.top() + computedMargins.bottom();
				surfaceWidthF = printableHeight + computedMargins.left() + computedMargins.right();
			}
			else
			{
				surfaceHeightF = printableWidth + computedMargins.left() + computedMargins.right();
				surfaceWidthF = printableHeight + computedMargins.top() + computedMargins.bottom();
			}
		}

		m_printingContext->startPage(surfaceWidthF, surfaceHeightF);

		doPrint(printableWidth, printableHeight, computedMargins, sheetNoZeroBased, pagesPerSheet, landscape, false, cairo, m_printingContext, context);

		m_printingContext->endPage();

		return true;
	}
	
	return false;
}

void WebPage::printingFinished(void)
{
	delete m_printingContext;
	m_printingContext = nullptr;
}

#pragma GCC pop_options

void WebPage::invalidate()
{
	if (m_drawContext)
		m_drawContext->invalidate();
	if (_fInvalidate)
		_fInvalidate(true);
}

bool WebPage::search(const WTF::String &string, WebCore::FindOptions &options, bool& outWrapped)
{
	WebCore::Page *cp = corePage();

	if (cp)
	{
		WebCore::DidWrap didWrap(WebCore::DidWrap::No);
		bool found = cp->findString(string, options, &didWrap);
		outWrapped = didWrap == WebCore::DidWrap::Yes;
		return found;
	}
	
	return false;
}

void WebPage::loadUserStyleSheet(const WTF::String &path)
{
	WebCore::Settings& settings = m_page->settings();
	if (path.length() == 0)
	{
		settings.setUserStyleSheetLocation(WTF::URL(WTF::URL(), WTF::String("file:///PROGDIR:Resources/morphos.css")));
	}
	else
	{
		settings.setUserStyleSheetLocation(WTF::URL(WTF::URL(), path));
	}
}

bool WebPage::allowsScrolling()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (coreFrame)
		return coreFrame->view()->canHaveScrollbars();
	return false;
}

void WebPage::setAllowsScrolling(bool allows)
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (coreFrame)
		coreFrame->view()->setCanHaveScrollbars(allows);
}

bool WebPage::editable()
{
	return m_page->isEditable();
}

void WebPage::setEditable(bool editable)
{
	m_page->setEditable(editable);

	if (editable)
	{
		auto* coreFrame = m_mainFrame->coreFrame();
		coreFrame->document()->securityOrigin().grantLoadLocalResources();
		WebEditorClient *client = reinterpret_cast<WebEditorClient *>(coreFrame->editor().client());
		client->onSpellcheckingLanguageChanged(); // force spellchecker pass, if enabled
	}
}

WTF::String WebPage::primaryLanguage()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	WebEditorClient *client = reinterpret_cast<WebEditorClient *>(coreFrame->editor().client());
	return client->primarySpellCheckingLanguage();
}

WTF::String WebPage::additionalLanguage()
{
	auto* coreFrame = m_mainFrame->coreFrame();
	WebEditorClient *client = reinterpret_cast<WebEditorClient *>(coreFrame->editor().client());
	return client->additionalSpellCheckingLanguage();
}

void WebPage::setSpellingLanguages(const WTF::String &language, const WTF::String &additional)
{
	auto* coreFrame = m_mainFrame->coreFrame();
	WebEditorClient *client = reinterpret_cast<WebEditorClient *>(coreFrame->editor().client());
	if (client)
		client->setSpellCheckingLanguages(language, additional);
}

void WebPage::flushCompositing()
{
	m_needsCompositingFlush = true;

	if (_fInvalidate)
		_fInvalidate(false);

}

bool WebPage::drawRect(const int x, const int y, const int width, const int height, struct RastPort *rp)
{
	auto* coreFrame = m_mainFrame->coreFrame();
	if (!coreFrame || !m_drawContext)
	{
		return false;
	}
	
	if (m_transitioning)
		return false;
	
    WebCore::FrameView* frameView = coreFrame->view();
    if (!frameView)
    {
    	return false;
	}

	auto *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	if (nullptr == surface)
	{
		return false;
	}

	auto *cairo = cairo_create(surface);
	if (nullptr == cairo)
	{
		cairo_surface_destroy(surface);
		return false;
	}

	{
		WebCore::GraphicsContextCairo gc(cairo);
		WebCore::IntRect rect(0, 0,width, height);
		gc.save();
		gc.clip(rect);
		gc.setImageInterpolationQuality(WebCore::InterpolationQuality::Default);

		OptionSet<WebCore::PaintBehavior> oldBehavior = frameView->paintBehavior();
		OptionSet<WebCore::PaintBehavior> paintBehavior = oldBehavior;
		auto oldScroll = frameView->scrollPosition();

		paintBehavior.add(WebCore::PaintBehavior::FlattenCompositingLayers);
		paintBehavior.add(WebCore::PaintBehavior::Snapshotting);
		frameView->setPaintBehavior(paintBehavior);
		m_ignoreScroll = true;
		frameView->WebCore::ScrollView::scrollTo(WebCore::ScrollPosition(x, y));

		frameView->paint(gc, rect);

		frameView->setPaintBehavior(oldBehavior);
		frameView->WebCore::ScrollView::scrollTo(oldScroll);
		gc.restore();
		m_ignoreScroll = false;

		cairo_surface_flush(surface);
		const unsigned int stride = cairo_image_surface_get_stride(surface);
		unsigned char *src = cairo_image_surface_get_data(surface);

		WritePixelArray(src, 0, 0, stride, rp, 0, 0, width, height, RECTFMT_ARGB);
	}

	cairo_destroy(cairo);
	cairo_surface_destroy(surface);

	return true;
}

static inline WebCore::MouseButton imsgToButton(IntuiMessage *imsg)
{
	if (IDCMP_MOUSEBUTTONS == imsg->Class || IDCMP_MOUSEMOVE == imsg->Class)
	{
		switch (imsg->Code)
		{
		case SELECTUP:
		case SELECTDOWN: return WebCore::MouseButton::LeftButton;
		case MENUUP:
		case MENUDOWN: return WebCore::MouseButton::RightButton;
		case MIDDLEUP:
		case MIDDLEDOWN: return WebCore::MouseButton::MiddleButton;
		default:
			if (imsg->Qualifier & IEQUALIFIER_LEFTBUTTON)
				return WebCore::MouseButton::LeftButton;
			if (imsg->Qualifier & IEQUALIFIER_RBUTTON)
				return WebCore::MouseButton::RightButton;
			if (imsg->Qualifier & IEQUALIFIER_MIDBUTTON)
				return WebCore::MouseButton::MiddleButton;
			break;
		}
	}
	
	return WebCore::MouseButton::NoButton;
}

static inline WebCore::PlatformEvent::Type imsgToEventType(IntuiMessage *imsg)
{
	switch (imsg->Class)
	{
	case IDCMP_MOUSEBUTTONS:
		switch (imsg->Code)
		{
		case SELECTDOWN:
		case MENUDOWN:
		case MIDDLEDOWN:
			return WebCore::PlatformEvent::Type::MousePressed;
		default:
			return WebCore::PlatformEvent::Type::MouseReleased;
		}
	}
	
	return WebCore::PlatformEvent::Type::MouseMoved;
}

static const unsigned ControlKey = 1 << 0;
static const unsigned AltKey = 1 << 1;
static const unsigned ShiftKey = 1 << 2;


struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static const KeyDownEntry keyDownEntries[] = {
    { VK_LEFT,   0,                  "MoveLeft"                                    },
    { VK_LEFT,   ShiftKey,           "MoveLeftAndModifySelection"                  },
    { VK_LEFT,   ControlKey,            "MoveWordLeft"                                },
    { VK_LEFT,   ControlKey | ShiftKey, "MoveWordLeftAndModifySelection"              },
    { VK_RIGHT,  0,                  "MoveRight"                                   },
    { VK_RIGHT,  ShiftKey,           "MoveRightAndModifySelection"                 },
    { VK_RIGHT,  ControlKey,            "MoveWordRight"                               },
    { VK_RIGHT,  ControlKey | ShiftKey, "MoveWordRightAndModifySelection"             },
    { VK_UP,     0,                  "MoveUp"                                      },
    { VK_UP,     ShiftKey,           "MoveUpAndModifySelection"                    },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection"                },
    { VK_DOWN,   0,                  "MoveDown"                                    },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection"                  },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection"              },
    { VK_PRIOR,  0,                  "MovePageUp"                                  },
    { VK_NEXT,   0,                  "MovePageDown"                                },
    { VK_HOME,   0,                  "MoveToBeginningOfLine"                       },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection"     },
    { VK_HOME,   ControlKey,            "MoveToBeginningOfDocument"                   },
    { VK_HOME,   ControlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },

    { VK_END,    0,                  "MoveToEndOfLine"                             },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection"           },
    { VK_END,    ControlKey,            "MoveToEndOfDocument"                         },
    { VK_END,    ControlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection"       },

    { VK_BACK,   0,                  "DeleteBackward"                              },
    { VK_BACK,   ShiftKey,           "DeleteBackward"                              },
    { VK_DELETE, 0,                  "DeleteForward"                               },
    { VK_BACK,   ControlKey,            "DeleteWordBackward"                          },
    { VK_DELETE, ControlKey,            "DeleteWordForward"                           },
	
    { 'B',       ControlKey,            "ToggleBold"                                  },
    { 'I',       ControlKey,            "ToggleItalic"                                },

    { VK_ESCAPE, 0,                  "Cancel"                                      },
    { VK_OEM_PERIOD, ControlKey,        "Cancel"                                      },
    { VK_TAB,    0,                  "InsertTab"                                   },
    { VK_TAB,    ShiftKey,           "InsertBacktab"                               },
    { VK_RETURN, 0,                  "InsertNewline"                               },
    { VK_RETURN, ControlKey,            "InsertNewline"                               },
    { VK_RETURN, AltKey,             "InsertNewline"                               },
    { VK_RETURN, ShiftKey,           "InsertNewline"                               },
    { VK_RETURN, AltKey | ShiftKey,  "InsertNewline"                               },

    // It's not quite clear whether clipboard shortcuts and Undo/Redo should be handled
    // in the application or in WebKit. We chose WebKit.
    { 'C',       ControlKey,            "Copy"                                        },
    { 'V',       ControlKey,            "Paste"                                       },
    { 'X',       ControlKey,            "Cut"                                         },
    { 'A',       ControlKey,            "SelectAll"                                   },
    { VK_INSERT, ControlKey,            "Copy"                                        },
    { VK_DELETE, ShiftKey,           "Cut"                                         },
    { VK_INSERT, ShiftKey,           "Paste"                                       },
    { 'Z',       ControlKey,            "Undo"                                        },
    { 'Z',       ControlKey | ShiftKey, "Redo"                                        },
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',   0,                  "InsertTab"                                   },
    { '\t',   ShiftKey,           "InsertBacktab"                               },
    { '\r',   0,                  "InsertNewline"                               },
    { '\r',   ControlKey,            "InsertNewline"                               },
    { '\r',   AltKey,             "InsertNewline"                               },
    { '\r',   ShiftKey,           "InsertNewline"                               },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               },
};

static const char* interpretKeyEvent(const KeyboardEvent* evt)
{
    ASSERT(evt->type() == eventNames().keydownEvent || evt->type() == eventNames().keypressEvent);

    static HashMap<int, const char*>* keyDownCommandsMap = 0;
    static HashMap<int, const char*>* keyPressCommandsMap = 0;

    if (!keyDownCommandsMap) {
        keyDownCommandsMap = new HashMap<int, const char*>;
        keyPressCommandsMap = new HashMap<int, const char*>;

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyDownEntries); ++i)
            keyDownCommandsMap->set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);

        for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyPressEntries); ++i)
            keyPressCommandsMap->set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
    }

    unsigned modifiers = 0;
    if (evt->shiftKey())
        modifiers |= ShiftKey;
    if (evt->altKey())
        modifiers |= AltKey;
    if (evt->ctrlKey())
        modifiers |= ControlKey;

    if (evt->type() == eventNames().keydownEvent) {
        int mapKey = modifiers << 16 | evt->keyCode();
        return mapKey ? keyDownCommandsMap->get(mapKey) : 0;
    }

    int mapKey = modifiers << 16 | evt->charCode();
    return mapKey ? keyPressCommandsMap->get(mapKey) : 0;
}

bool WebPage::handleEditingKeyboardEvent(WebCore::KeyboardEvent& event)
{
    auto* frame = downcast<WebCore::Node>(event.target())->document().frame();
    ASSERT(frame);

    auto* keyEvent = event.underlyingPlatformEvent();
    if (!keyEvent || keyEvent->isSystemKey())  // do not treat this as text input if it's a system key event
        return false;

    auto command = frame->editor().command(interpretKeyEvent(&event));

    if (keyEvent->type() == PlatformEvent::RawKeyDown) {
        // WebKit doesn't have enough information about mode to decide how commands that just insert text if executed via Editor should be treated,
        // so we leave it upon WebCore to either handle them immediately (e.g. Tab that changes focus) or let a keypress event be generated
        // (e.g. Tab that inserts a Tab character, or Enter).
        return !command.isTextInsertion() && command.execute(&event);
    }

    if (command.execute(&event))
        return true;

    // Don't insert null or control characters as they can result in unexpected behaviour
    if (event.charCode() < ' ')
        return false;

    return frame->editor().insertText(keyEvent->text(), &event);
}

bool WebPage::checkDownloadable(IntuiMessage *imsg, const int mouseX, const int mouseY, WTF::URL &outURL)
{
	auto position = m_mainFrame->coreFrame()->view()->windowToContents(WebCore::IntPoint(mouseX, mouseY));
    constexpr OptionSet<HitTestRequest::Type> hitType { HitTestRequest::Type::ReadOnly, HitTestRequest::Type::Active,
		HitTestRequest::Type::DisallowUserAgentShadowContent, HitTestRequest::Type::AllowChildFrameContent };
	auto hitTestResult = m_mainFrame->coreFrame()->eventHandler().hitTestResultAtPoint(position, hitType);
	(void)imsg;
	if (hitTestResult.isOverLink())
		outURL = hitTestResult.absoluteLinkURL();
	else if (hitTestResult.image())
		outURL = hitTestResult.absoluteImageURL();
	return hitTestResult.isOverLink() || hitTestResult.image();
}

int WebPage::mouseCursorToSet(ULONG qualifiers, bool mouseInside)
{
	if (m_trackMiddleDidScroll && m_trackMiddle)
	{
		return POINTERTYPE_VERTICALRESIZE;
	}
	
	if (m_cursorLock != POINTERTYPE_NORMAL)
		return m_cursorLock;
	
	if (mouseInside || m_trackMouse)
	{
		if (m_cursorOverLink && (qualifiers & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)))
		{
			return POINTERTYPE_ALTERNATIVECHOICE;
		}
	
		return m_cursor;
	}
	
	return POINTERTYPE_NORMAL;
}

bool WebPage::handleIntuiMessage(IntuiMessage *imsg, const int mouseX, const int mouseY, bool mouseInside, bool isDefaultHandler)
{
	WebCore::Page *cp = corePage();
	if (!cp)
		return false;

	auto& bridge = cp->userInputBridge();
	auto& focusController = m_page->focusController();

	switch (imsg->Class)
	{
	case IDCMP_MOUSEMOVE:
	case IDCMP_MOUSEBUTTONS:
	case IDCMP_MOUSEHOVER:
		{
			if (imsg->Class == IDCMP_MOUSEMOVE)
				m_clickCount = 0;
			else if (imsg->Code == SELECTDOWN || imsg->Code == MIDDLEDOWN)
				m_clickCount ++;
			
			WebCore::PlatformMouseEvent pme(
				WebCore::IntPoint(mouseX, mouseY),
				WebCore::IntPoint(imsg->IDCMPWindow->LeftEdge + imsg->MouseX, imsg->IDCMPWindow->TopEdge + imsg->MouseY),
				imsgToButton(imsg),
				imsgToEventType(imsg),
				m_clickCount,
				(imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) != 0,
				(imsg->Qualifier & IEQUALIFIER_CONTROL) != 0,
				(imsg->Qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)) != 0,
				(imsg->Qualifier & (IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)) != 0,
				WTF::WallTime::fromRawSeconds(imsg->Seconds),
				imsg->Class == IDCMP_MOUSEBUTTONS ? WebCore::ForceAtClick : 0.0,
				WebCore::SyntheticClickType::NoTap);
			
			m_lastQualifier = imsg->Qualifier;

			if (m_dragging)
			{
				switch (imsg->Class)
				{
				case IDCMP_MOUSEBUTTONS:
					if (SELECTUP == imsg->Code)
						endDragging(mouseX, mouseY, imsg->IDCMPWindow->LeftEdge + imsg->MouseX, imsg->IDCMPWindow->TopEdge + imsg->MouseY, true);
					else
						endDragging(mouseX, mouseY, imsg->IDCMPWindow->LeftEdge + imsg->MouseX, imsg->IDCMPWindow->TopEdge + imsg->MouseY, false);
					break;
					
				case IDCMP_MOUSEMOVE:
					{
					    IntPoint adjustedClientPosition(pme.position().x() - m_page->dragController().dragOffset().x(), pme.position().y() - m_page->dragController().dragOffset().y());
						IntPoint adjustedGlobalPosition(pme.globalPosition().x() - m_page->dragController().dragOffset().x(), pme.globalPosition().y() - m_page->dragController().dragOffset().y());
						DragData drag(&m_dragData, adjustedClientPosition, adjustedGlobalPosition, DragOperation::Copy);

						if (m_dragInside != mouseInside)
						{
							if (mouseInside)
								m_page->dragController().dragEntered(drag);
							else
								m_page->dragController().dragExited(drag);
							m_dragInside = mouseInside;
						}
						else
						{
							m_page->dragController().dragUpdated(drag);
						}
						
						if (_fMoveDragWindow)
							_fMoveDragWindow(adjustedGlobalPosition.x(), adjustedGlobalPosition.y());
					}
					break;
				}
				
				return true;
			}

			switch (imsg->Class)
			{
			case IDCMP_MOUSEBUTTONS:
				switch (imsg->Code)
				{
				case SELECTDOWN:
					if (mouseInside)
					{
						if (_fGoActive)
							_fGoActive();

						bridge.handleMousePressEvent(pme);
						m_trackMouse = true;
						return true;
					}
					break;

				case MIDDLEDOWN:
					if (mouseInside)
					{
						m_trackMiddle = true;
						m_trackMouse = true;
						m_trackMiddleDidScroll = false;
						m_middleClick[0] = mouseX;
						m_middleClick[1] = mouseY;
						return true;
					}
					break;
				case MIDDLEUP:
					if (!m_trackMiddleDidScroll && (mouseInside || m_trackMouse))
					{
						auto position = m_mainFrame->coreFrame()->view()->windowToContents(pme.position());
						constexpr OptionSet<HitTestRequest::Type> hitType { WebCore::HitTestRequest::Type::ReadOnly, WebCore::HitTestRequest::Type::Active, WebCore::HitTestRequest::Type::DisallowUserAgentShadowContent, WebCore::HitTestRequest::Type::AllowChildFrameContent };
            			auto hitTestResult = m_mainFrame->coreFrame()->eventHandler().hitTestResultAtPoint(position, hitType);
						bool isMouseDownOnLinkOrImage = hitTestResult.isOverLink() || hitTestResult.image();

						if (isMouseDownOnLinkOrImage)
						{
							if (imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))
							{
								// open in new window...
								if (_fNewTabWindow)
								{
									if (hitTestResult.isOverLink())
										_fNewTabWindow(hitTestResult.absoluteLinkURL(), WebViewDelegateOpenWindowMode::NewWindow);
									else
										_fNewTabWindow(hitTestResult.absoluteImageURL(), WebViewDelegateOpenWindowMode::NewWindow);
								}
							}
							else if (imsg->Qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT))
							{
								// download
								if (_fDownload)
								{
									if (hitTestResult.isOverLink())
										_fDownload(hitTestResult.absoluteLinkURL(), hitTestResult.linkSuggestedFilename());
									else
										_fDownload(hitTestResult.absoluteImageURL(), { });
								}
							}
							else
							{
								// open in new bg tab...
								if (_fNewTabWindow)
								{
									if (hitTestResult.isOverLink())
										_fNewTabWindow(hitTestResult.absoluteLinkURL(), WebViewDelegateOpenWindowMode::BackgroundTab);
									else
										_fNewTabWindow(hitTestResult.absoluteImageURL(), WebViewDelegateOpenWindowMode::BackgroundTab);
								}
							}
						}
						bool wasTrackMouse(m_trackMouse);
						m_trackMouse = false;
						m_trackMiddle = false;
						m_trackMiddleDidScroll = false;
						return wasTrackMouse;
					}
					m_trackMiddle = false;
					m_trackMouse = false;
					m_trackMiddleDidScroll = false;
					if (_fSetCursor)
						_fSetCursor(mouseCursorToSet(imsg->Qualifier, mouseInside));
					break;
				case MENUDOWN:
					// This is consistent with Safari
					// Other browsers are a mess: Vivaldi does Down, Up, ContextMenu, Firefox & Chrome do Down, ContextMenu, Up
					if (mouseInside)
					{
						if (_fGoActive)
							_fGoActive();

						bool doEvent = true;
						bool shouldShowMenu = true;

						m_page->contextMenuController().clearContextMenu();

						switch (m_cmHandling)
						{
						case ContextMenuHandling::Override:
							doEvent = false;
							break;
						case ContextMenuHandling::OverrideWithControl:
							doEvent = (imsg->Qualifier & IEQUALIFIER_CONTROL) == 0;
							break;
						case ContextMenuHandling::OverrideWithAlt:
							doEvent = (imsg->Qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)) == 0;
							break;
						case ContextMenuHandling::OverrideWithShift:
							doEvent = (imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) == 0;
							break;
						case ContextMenuHandling::Default:
						default:
							break;
						}

						auto position = m_mainFrame->coreFrame()->view()->windowToContents(pme.position());
						constexpr OptionSet<HitTestRequest::Type> hitType { WebCore::HitTestRequest::Type::ReadOnly, WebCore::HitTestRequest::Type::Active, WebCore::HitTestRequest::Type::DisallowUserAgentShadowContent, WebCore::HitTestRequest::Type::AllowChildFrameContent };
						auto result = m_mainFrame->coreFrame()->eventHandler().hitTestResultAtPoint(position, hitType);

						if (doEvent)
						{
							bool rmbHandled = bridge.handleMousePressEvent(pme);
							Frame* targetFrame = result.innerNonSharedNode() ? result.innerNonSharedNode()->document().frame() : &m_page->focusController().focusedOrMainFrame();

							if (targetFrame)
							{
								if (bridge.handleContextMenuEvent(pme, *targetFrame))
									shouldShowMenu = m_page->contextMenuController().contextMenu() ? (m_page->contextMenuController().contextMenu()->items().size() > 0) : false;
							}

							if (rmbHandled && !shouldShowMenu)
							{
								m_trackMouse = true;
							}

						}


						if (shouldShowMenu && _fContextMenu)
						{
							bool handled;
							if (m_page->contextMenuController().contextMenu() &&
								m_page->contextMenuController().contextMenu()->items().size() > 0)
								handled = _fContextMenu(WebCore::IntPoint(mouseX, mouseY), m_page->contextMenuController().contextMenu()->items(), result);
							else
								handled = _fContextMenu(WebCore::IntPoint(mouseX, mouseY), { }, result);
							if (handled)
								WebKit::WebProcess::singleton().returnedFromConstrainedRunLoop();
						}

						return true;
					}
					break;
				case MENUUP:
					if (m_trackMouse)
					{
						m_trackMouse = false;
						return bridge.handleMouseReleaseEvent(pme);
					}
					break;
				default:
					if (mouseInside || m_trackMouse)
					{
						bridge.handleMouseReleaseEvent(pme);
						bool wasTrackMouse(m_trackMouse);
						m_trackMouse = false;
						return wasTrackMouse;
					}
					break;
				}
				break;
			case IDCMP_MOUSEMOVE:
			case IDCMP_MOUSEHOVER:
				if (mouseInside || m_trackMouse)
				{
					WTF::URL hoverURL;
					m_cursorOverLink = checkDownloadable(imsg, mouseX, mouseY, hoverURL);

					int deltaX = 0;
					int deltaY = 0;
					
					if (m_trackMiddle)
					{
						deltaX = m_middleClick[0] - mouseX;
						deltaY = m_middleClick[1] - mouseY;
					}

					if (m_trackMiddleDidScroll || (abs(deltaX) > 5) || (abs(deltaY) > 5))
					{
						m_middleClick[0] = mouseX;
						m_middleClick[1] = mouseY;
						scrollBy(-deltaX, -deltaY, WebPageScrollByMode::Pixels);
						m_trackMiddleDidScroll = true;
					}

					if (!m_trackMiddleDidScroll)
					{
						bridge.handleMouseMoveEvent(pme);

						if (m_hoveredURL != hoverURL)
						{
							m_hoveredURL = hoverURL;
							if (_fHoveredURLChanged)
							{
								_fHoveredURLChanged(m_hoveredURL);
							}
						}
					}

					if (_fSetCursor)
						_fSetCursor(mouseCursorToSet(imsg->Qualifier, mouseInside));

					return m_trackMouse;
				}
				else
				{
					m_cursorOverLink = false;
					if (_fSetCursor)
						_fSetCursor(0);
				}
				break;
			}
		}
		break;
		
	case IDCMP_RAWKEY:
		{
			ULONG code = imsg->Code & ~IECODE_UP_PREFIX;
			BOOL up = (imsg->Code & IECODE_UP_PREFIX) == IECODE_UP_PREFIX;

// dprintf("rawkey %lx up %d\n", imsg->Code& ~IECODE_UP_PREFIX, up);

			m_lastQualifier = imsg->Qualifier;

			switch (code)
			{
			case NM_WHEEL_UP:
			case NM_WHEEL_DOWN:
				if (mouseInside && !up)
				{
					float wheelTicksY = (code == NM_WHEEL_UP) ? 1 : -1;
					float deltaY = (code == NM_WHEEL_UP) ? Scrollbar::pixelsPerLineStep() : -Scrollbar::pixelsPerLineStep();
					WebCore::PlatformWheelEvent pke(WebCore::IntPoint(mouseX, mouseY),
						WebCore::IntPoint(imsg->IDCMPWindow->LeftEdge + imsg->MouseX, imsg->IDCMPWindow->TopEdge + imsg->MouseY),
						0, deltaY,
						0, wheelTicksY,
						ScrollByPixelWheelEvent,
						(imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) != 0,
						(imsg->Qualifier & IEQUALIFIER_CONTROL) != 0,
						(imsg->Qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)) != 0,
						(imsg->Qualifier & (IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)) != 0
						);
					
					auto position = m_mainFrame->coreFrame()->view()->windowToContents(pke.position());
					constexpr OptionSet<HitTestRequest::Type> hitType { WebCore::HitTestRequest::Type::ReadOnly, WebCore::HitTestRequest::Type::Active, WebCore::HitTestRequest::Type::DisallowUserAgentShadowContent, WebCore::HitTestRequest::Type::AllowChildFrameContent };
					auto result = m_mainFrame->coreFrame()->eventHandler().hitTestResultAtPoint(position, hitType);
					Frame* targetFrame = result.innerNonSharedNode() ? result.innerNonSharedNode()->document().frame() : &m_page->focusController().focusedOrMainFrame();
					bool handled = bridge.handleWheelEvent(pke, { WheelEventProcessingSteps::MainThreadForScrolling, WheelEventProcessingSteps::MainThreadForBlockingDOMEventDispatch });
					if (!handled)
						wheelScrollOrZoomBy(0, (code == NM_WHEEL_UP) ? 1 : -1, imsg->Qualifier, targetFrame);

					return true;
				}
				break;
			
			case NM_WHEEL_LEFT:
			case NM_WHEEL_RIGHT:
				if (mouseInside && !up)
				{
					float wheelTicksX = (code == NM_WHEEL_LEFT) ? 1 : -1;
					float deltaX = (code == NM_WHEEL_LEFT) ? Scrollbar::pixelsPerLineStep() : -Scrollbar::pixelsPerLineStep();
					WebCore::PlatformWheelEvent pke(WebCore::IntPoint(mouseX, mouseY),
						WebCore::IntPoint(imsg->IDCMPWindow->LeftEdge + imsg->MouseX, imsg->IDCMPWindow->TopEdge + imsg->MouseY),
						deltaX, 0,
						wheelTicksX, 0,
						ScrollByPixelWheelEvent,
						(imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) != 0,
						(imsg->Qualifier & IEQUALIFIER_CONTROL) != 0,
						(imsg->Qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT)) != 0,
						(imsg->Qualifier & (IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)) != 0
						);
					
					auto position = m_mainFrame->coreFrame()->view()->windowToContents(pke.position());
					constexpr OptionSet<HitTestRequest::Type> hitType { WebCore::HitTestRequest::Type::ReadOnly, WebCore::HitTestRequest::Type::Active, WebCore::HitTestRequest::Type::DisallowUserAgentShadowContent, WebCore::HitTestRequest::Type::AllowChildFrameContent };
					auto result = m_mainFrame->coreFrame()->eventHandler().hitTestResultAtPoint(position, hitType);
					Frame* targetFrame = result.innerNonSharedNode() ? result.innerNonSharedNode()->document().frame() : &m_page->focusController().focusedOrMainFrame();
					bool handled = bridge.handleWheelEvent(pke, { WheelEventProcessingSteps::MainThreadForScrolling, WheelEventProcessingSteps::MainThreadForBlockingDOMEventDispatch });
					if (!handled)
						wheelScrollOrZoomBy((code == NM_WHEEL_LEFT) ? 1 : -1, 0, imsg->Qualifier, targetFrame);

					return true;
				}
				break;
				
			case RAWKEY_TAB:
				if (!m_isActive)
					return false;
				if (!up)
				{
					if (m_justWentActive)
					{
						Frame& frame = m_page->focusController().focusedOrMainFrame();
						frame.document()->setFocusedElement(0);
						m_page->focusController().setInitialFocus((imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) ?
							FocusDirection::Backward : FocusDirection::Forward, nullptr);
						m_justWentActive = false;
					}
					else
					{
						bool rc = focusController.advanceFocus((imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)) ? FocusDirection::Backward : FocusDirection::Forward, nullptr);
						if ((!rc || !m_focusedElement) && _fActivateNext && _fActivatePrevious)
						{
							if (imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT))
								_fActivatePrevious();
							else
								_fActivateNext();
						}
					}
				}
				return true;
			
			default:
				if (m_isActive || isDefaultHandler)
				{
					bool handled = false;
					bool doHandle = true;

					if (0 != ((IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND) & imsg->Qualifier))
						doHandle = false;

					if (m_isActive && code == RAWKEY_ESCAPE)
					{
						handled = true;
						doHandle = false;
						if (_fGoInactive)
							_fGoInactive();
					}

					if (doHandle)
					{
						handled = bridge.handleKeyEvent(WebCore::PlatformKeyboardEvent(imsg));
					}

					#define KEYQUALIFIERS (IEQUALIFIER_LALT|IEQUALIFIER_RALT|IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND|IEQUALIFIER_CONTROL)

					if (!handled)
					{
						switch (code)
						{
						case RAWKEY_LALT:
						case RAWKEY_RALT:
							if (_fSetCursor)
								_fSetCursor(mouseCursorToSet(imsg->Qualifier, mouseInside));
							break;
						
						case RAWKEY_PAGEUP:
							if (!up && m_drawContext && (0 == (imsg->Qualifier & KEYQUALIFIERS)))
							{
								scrollBy(0, 1, WebPageScrollByMode::Pages, m_page->focusController().focusedFrame());
								return true;
							}
							break;

						case RAWKEY_PAGEDOWN:
							if (!up && m_drawContext && (0 == (imsg->Qualifier & KEYQUALIFIERS)))
							{
								scrollBy(0, -1, WebPageScrollByMode::Pages, m_page->focusController().focusedFrame());
								return true;
							}
							break;
							
						case RAWKEY_SPACE:
							if (!up && m_drawContext)
							{
								if (0 == (imsg->Qualifier & KEYQUALIFIERS))
								{
									scrollBy(0, -1, WebPageScrollByMode::Pages, m_page->focusController().focusedFrame());
								}
								else if (((IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) & (imsg->Qualifier & KEYQUALIFIERS)) != 0)
								{
									scrollBy(0, 1, WebPageScrollByMode::Pages, m_page->focusController().focusedFrame());
								}
								return true;
							}
							break;

						case RAWKEY_DOWN:
							if (!up && m_drawContext && (0 == (imsg->Qualifier & KEYQUALIFIERS)))
							{
								scrollBy(0, -1, WebPageScrollByMode::Units, m_page->focusController().focusedFrame());
								return true;
							}
							break;

						case RAWKEY_UP:
							if (!up && m_drawContext && (0 == (imsg->Qualifier & KEYQUALIFIERS)))
							{
								scrollBy(0, 1, WebPageScrollByMode::Units, m_page->focusController().focusedFrame());
								return true;
							}
							break;

						case RAWKEY_HOME:
						case RAWKEY_END:
							if (!up && (0 == (imsg->Qualifier & KEYQUALIFIERS)))
							{
								auto* coreFrame = m_page->focusController().focusedFrame() ? m_page->focusController().focusedFrame() : m_mainFrame->coreFrame();
								
								if (coreFrame)
								{
									WebCore::FrameView *view = coreFrame->view();
									WebCore::ScrollPosition sp = view->scrollPosition();
									WebCore::ScrollPosition spMin = view->minimumScrollPosition();
									WebCore::ScrollPosition spMax = view->maximumScrollPosition();

									view->setScrollPosition(WebCore::ScrollPosition(sp.x(), code == RAWKEY_HOME ? spMin.y() : spMax.y()));
									
									if (_fInvalidate)
										_fInvalidate(false);
								}
								return true;
							}
							break;
						}
						return false;
					}
					return true;
				}
				break;
			}
		}
		break;
		
	case IDCMP_INACTIVEWINDOW:
		m_trackMiddle = false;
		m_trackMouse = false;
		endDragging(mouseX, mouseY, imsg->IDCMPWindow->LeftEdge + imsg->MouseX, imsg->IDCMPWindow->TopEdge + imsg->MouseY, false);
		break;
	}

	return false;
}

bool WebPage::handleMUIKey(int muikey, bool isDefaultHandler)
{
	if (!m_isActive && !isDefaultHandler)
		return false;
	if (!m_page)
		return false;
	const char *editorCommand = nullptr;

	auto& focusController = m_page->focusController();
	switch (muikey)
	{
	case MUIKEY_GADGET_NEXT:
		focusController.advanceFocus(FocusDirection::Forward, nullptr);
		return true;
	case MUIKEY_GADGET_PREV:
		focusController.advanceFocus(FocusDirection::Backward, nullptr);
		return true;
	case MUIKEY_GADGET_OFF:
		return true;
	case MUIKEY_CUT:
		editorCommand = "Cut";
		break;
	case MUIKEY_COPY:
		editorCommand = "Copy";
		break;
	case MUIKEY_PASTE:
		editorCommand = "Paste";
		break;
	case MUIKEY_UNDO:
		editorCommand = "Undo";
		break;
	case MUIKEY_REDO:
		editorCommand = "Redo";
		break;
	default:
		break;
	}
	
	if (editorCommand && focusController.focusedFrame())
	{
		auto command = focusController.focusedFrame()->editor().command(editorCommand);
		if (command.execute())
			return true;
	}

	return false;
}

void WebPage::onContextMenuItemSelected(ULONG action, const char *title)
{
	WebCore::ContextMenuAction cmaction = WebCore::ContextMenuAction(action);
	WTF::String wtftitle = WTF::String::fromUTF8(title);
	WebCore::Page *page = corePage();
	if (!page)
		return;
	page->contextMenuController().contextMenuItemSelected(cmaction, wtftitle);
}

WebCore::Frame * WebPage::fromHitTest(WebCore::HitTestResult &hitTest) const
{
	return hitTest.innerNonSharedNode()->document().frame();
}

bool WebPage::hitTestImageToClipboard(WebCore::HitTestResult &hitTest) const
{
	WebCore::Frame *frame = fromHitTest(hitTest);

	if (frame)
	{
		Ref<Frame> protector(*frame);
		frame->editor().copyImage(hitTest);
	}

	return false;
}

bool WebPage::hitTestSaveImageToFile(WebCore::HitTestResult &, const WTF::String &) const
{
	return false;
}

void WebPage::hitTestReplaceSelectedTextWidth(WebCore::HitTestResult &hitTest, const WTF::String &text) const
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		frame->editor().insertText(text, nullptr);
	}
}

void WebPage::hitTestCutSelectedText(WebCore::HitTestResult &hitTest) const
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		frame->editor().command("Cut").execute();
	}
}

void WebPage::hitTestCopySelectedText(WebCore::HitTestResult &hitTest) const
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		frame->editor().command("Copy").execute();
	}
}

void WebPage::hitTestPaste(WebCore::HitTestResult &hitTest) const
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		frame->editor().command("Paste").execute();
	}
}

void WebPage::hitTestSelectAll(WebCore::HitTestResult &hitTest) const
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		frame->editor().command("SelectAll").execute();
	}
}

void WebPage::hitTestSetImageFloat(WebCore::HitTestResult &hitTest, ContextMenuImageFloat imageFloat)
{
	WebCore::StyledElement *element = reinterpret_cast<WebCore::StyledElement*>(hitTest.targetElement());
	if (element && hitTest.image())
	{
		switch (imageFloat)
		{
		case ContextMenuImageFloat::Left:
			element->setInlineStyleProperty(CSSPropertyFloat, CSSValueLeft, true);
			break;
		case ContextMenuImageFloat::Right:
			element->setInlineStyleProperty(CSSPropertyFloat, CSSValueRight, true);
			break;
		default:
			element->setInlineStyleProperty(CSSPropertyFloat, CSSValueNone, true);
			break;
		}
	}
}

WebPage::ContextMenuImageFloat WebPage::hitTestImageFloat(WebCore::HitTestResult &hitTest) const
{
	WebCore::StyledElement *element = reinterpret_cast<WebCore::StyledElement*>(hitTest.targetElement());
	if (element && hitTest.image() && element->inlineStyle())
	{
		auto property = element->inlineStyle()->getPropertyValue(CSSPropertyFloat);
		if (equalIgnoringASCIICase(property, "left"))
			return ContextMenuImageFloat::Left;
		if (equalIgnoringASCIICase(property, "right"))
			return ContextMenuImageFloat::Right;
	}
	return ContextMenuImageFloat::None;
}


WTF::String WebPage::misspelledWord(WebCore::HitTestResult &hitTest)
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		return frame->editor().misspelledWordAtCaretOrRange(hitTest.innerNode());
	}
	
	return WTF::String();
}

void WebPage::markWord(WebCore::HitTestResult &hitTest)
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		VisibleSelection selection = frame->selection().selection();
		if (!selection.isContentEditable() || selection.isNone())
			return;

		frame->selection().moveTo(frame->visiblePositionForPoint(hitTest.roundedPointInInnerNodeFrame()));

#if 0
		VisibleSelection wordSelection(selection.base());
		wordSelection.expandUsingGranularity(WordGranularity);
		
		frame->selection().setSelection(wordSelection);
#endif
	}
}

WTF::Vector<WTF::String> WebPage::misspelledWordSuggestions(WebCore::HitTestResult &hitTest)
{
	WTF::Vector<WTF::String> out;

	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		auto miss = frame->editor().misspelledWordAtCaretOrRange(hitTest.innerNode());
		WebEditorClient *client = reinterpret_cast<WebEditorClient *>(frame->editor().client());
		client->getGuessesForWord(miss, out);
	}

	return out;
}

void WebPage::learnMisspelled(WebCore::HitTestResult &hitTest)
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		auto miss = frame->editor().misspelledWordAtCaretOrRange(hitTest.innerNode());
		frame->editor().textChecker()->learnWord(miss);
	}
}

void WebPage::ignoreMisspelled(WebCore::HitTestResult &hitTest)
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		auto miss = frame->editor().misspelledWordAtCaretOrRange(hitTest.innerNode());
		frame->editor().textChecker()->ignoreWordInSpellDocument(miss);
	}
}

void WebPage::replaceMisspelled(WebCore::HitTestResult &hitTest, const WTF::String &replacement)
{
	WebCore::Frame *frame = fromHitTest(hitTest);
	if (frame)
	{
		VisibleSelection selection = frame->selection().selection();
		if (!selection.isContentEditable() || selection.isNone())
			return;

		VisibleSelection wordSelection(selection.base());
		wordSelection.expandUsingGranularity(TextGranularity::WordGranularity);
		auto wordRange = wordSelection.toNormalizedRange();
		if (!wordRange)
			return;


		frame->editor().replaceRangeForSpellChecking(*wordRange, replacement);
	}
}

void WebPage::startDownload(const WTF::URL &url)
{
	auto protocol = url.protocol().toString();
	if (WTF::equalIgnoringASCIICase(protocol, "ftp"))
	{
		auto udata = url.string().ascii();
		struct TagItem urltags[] = { { URL_Launch, TRUE }, { URL_Show, TRUE }, { TAG_DONE, 0 } };
		URL_OpenA((STRPTR)udata.data(), urltags);
	}
	else if (WTF::equalIgnoringASCIICase(protocol, "mailto"))
	{
		auto udata = url.string().ascii();
		struct TagItem urltags[] = { { URL_Launch, TRUE }, { URL_Show, TRUE }, { TAG_DONE, 0 } };
		URL_OpenA((STRPTR)udata.data(), urltags);
	}
	else
	{
		m_mainFrame->startDownload(url);
	}
}

bool WebPage::canUndo()
{
	if (m_page)
	{
		auto& focusController = m_page->focusController();
		auto& editor = focusController.focusedFrame()->editor();
		return editor.canUndo();
	}

	return false;
}

bool WebPage::canRedo()
{
	if (m_page)
	{
		auto& focusController = m_page->focusController();
		auto& editor = focusController.focusedFrame()->editor();
		return editor.canRedo();
	}

	return false;
}

void WebPage::undo()
{
	if (m_page)
	{
		auto& focusController = m_page->focusController();
		auto& editor = focusController.focusedFrame()->editor();
		editor.undo();
	}
}

void WebPage::redo()
{
	if (m_page)
	{
		auto& focusController = m_page->focusController();
		auto& editor = focusController.focusedFrame()->editor();
		editor.redo();
	}
}

void WebPage::startDrag(WebCore::DragItem&& item, WebCore::DataTransfer& transfer, WebCore::Frame&)
{
	m_dragging = true;
	m_dragImage = WTFMove(item.image);
	m_dragData = transfer.pasteboard().selectionData();

	RefPtr<cairo_surface_t> imageRef = m_dragImage.get();
	if (!!imageRef)
	{
		auto width = cairo_image_surface_get_width(imageRef.get());
		auto height = cairo_image_surface_get_height(imageRef.get());

		if (_fOpenDragWindow)
			_fOpenDragWindow(m_page->dragController().dragOffset().x(), m_page->dragController().dragOffset().y(), width, height);
	}
}

void WebPage::endDragging(int mouseX, int mouseY, int mouseGlobalX, int mouseGlobalY, bool doDrop)
{
	IntPoint adjustedClientPosition(mouseX - m_page->dragController().dragOffset().x(), mouseY - m_page->dragController().dragOffset().y());
	IntPoint adjustedGlobalPosition(mouseGlobalX - m_page->dragController().dragOffset().x(), mouseGlobalY - m_page->dragController().dragOffset().y());

	if (m_dragInside && doDrop)
	{
		// Drop!
		DragData drag(&m_dragData, adjustedClientPosition, adjustedGlobalPosition, DragOperation::Copy);

		m_page->dragController().performDragOperation(drag);
		m_page->dragController().dragEnded();

		PlatformMouseEvent event(adjustedClientPosition, adjustedGlobalPosition, LeftButton, PlatformEvent::MouseMoved, 0, false, false, false, false, WallTime::now(), 0, WebCore::NoTap);
		m_page->mainFrame().eventHandler().dragSourceEndedAt(event, m_page->dragController().sourceDragOperationMask());
	}
	else
	{
		m_page->dragController().dragEnded();
	}

	m_dragging = false;
	m_dragImage = WebCore::DragImage();

	if (_fCloseDragWindow)
		_fCloseDragWindow();

	WebKit::WebProcess::singleton().returnedFromConstrainedRunLoop();
}

void WebPage::drawDragImage(struct RastPort *rp, const int x, const int y, const int width, const int height)
{
	RefPtr<cairo_surface_t> imageRef = m_dragImage.get();
	if (!!imageRef)
	{
		const unsigned int stride = cairo_image_surface_get_stride(imageRef.get());
		unsigned char *src = cairo_image_surface_get_data(imageRef.get());
		WritePixelArray(src, 0, 0, stride, rp, x, y, width, height, RECTFMT_ARGB);
	}
}

}
