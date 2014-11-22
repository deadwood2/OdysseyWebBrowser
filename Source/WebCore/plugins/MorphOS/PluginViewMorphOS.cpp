/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
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
#include "PluginView.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "Element.h"
#include "EventNames.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "FrameTree.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "HostWindow.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformMouseEvent.h"
#include "PluginDebug.h"
#include "PluginMainThreadScheduler.h"
#include "PluginPackage.h"
#include "RenderLayer.h"
#include "Settings.h"
#include "JSDOMBinding.h"
#include "ScriptController.h"
#include "npruntime_impl.h"
#include "runtime_root.h"
#include <runtime/JSLock.h>
#include <runtime/JSCJSValue.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <clib/debug_protos.h>

using JSC::ExecState;
using JSC::Interpreter;
using JSC::JSObject;

using std::min;

using namespace WTF;

namespace WebCore {

using namespace HTMLNames;

void PluginView::updatePluginWidget()
{
    if (!parent() || !m_isWindowed)
        return;

    ASSERT(parent()->isFrameView());
    FrameView* frameView = static_cast<FrameView*>(parent());

    IntRect oldWindowRect = m_windowRect;
    IntRect oldClipRect = m_clipRect;

    m_windowRect = IntRect(frameView->contentsToWindow(frameRect().location()), frameRect().size());
    m_clipRect = windowClipRect();
    m_clipRect.move(-m_windowRect.x(), -m_windowRect.y());

	//kprintf("PluginView::updatePluginWidget %p [%d %d %d %d]\n", platformPluginWidget(), m_windowRect.x(), m_windowRect.y(), m_windowRect.width(), m_windowRect.height());

    if (platformPluginWidget() && (m_windowRect != oldWindowRect || m_clipRect != oldClipRect))
		setNPWindowRect(m_windowRect);
}

void PluginView::setFocus(bool /*focused*/)
{
}

void PluginView::show()
{
}

void PluginView::hide()
{
}

struct IEvent {
	LONG type;
	LONG Class;
	LONG Code;
	LONG Qualifier;
	APTR drawable;
	LONG x;
	LONG y;
	LONG width;
	LONG height;
};

void PluginView::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_isStarted || m_status !=  PluginStatusLoadedSuccessfully) {
        // Draw the "missing plugin" image
        paintMissingPluginIcon(context, rect);
        return;
    }

	setNPWindowRect(rect);

	if(!m_isWindowed)
	{
		IEvent npEvent;
	    IntRect exposedRect(rect);
	    exposedRect.intersect(frameRect());
	    exposedRect.move(-frameRect().x(), -frameRect().y());

		npEvent.type = 0;
		npEvent.drawable = (void*) platformPluginWidget() ? platformPluginWidget()->browser : 0;
	    npEvent.x = exposedRect.x();
	    npEvent.y = exposedRect.y();
	    npEvent.width = exposedRect.x() + exposedRect.width(); // flash bug? it thinks width is the right in transparent mode
	    npEvent.height = exposedRect.y() + exposedRect.height(); // flash bug? it thinks height is the bottom in transparent mode

		if (m_plugin->pluginFuncs()->event)
		{
			m_plugin->pluginFuncs()->event(m_instance, (NPEvent) &npEvent);
		}
	}
}

void PluginView::handleKeyboardEvent(KeyboardEvent* event)
{
	IEvent npEvent;

	npEvent.type = 1;

	JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
	if (!m_plugin->pluginFuncs()->event || !m_plugin->pluginFuncs()->event(m_instance, (NPEvent) &npEvent))
        event->setDefaultHandled();
}

void PluginView::handleMouseEvent(MouseEvent* event)
{
	IEvent npEvent;

	npEvent.type = 1;

	JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
	if (!m_plugin->pluginFuncs()->event  || !m_plugin->pluginFuncs()->event(m_instance, (NPEvent) &npEvent))
        event->setDefaultHandled();
}

void PluginView::setParent(ScrollView* parent)
{
    Widget::setParent(parent);

    if (parent)
        init();
}

void PluginView::setNPWindowRect(const IntRect& rect)
{
    if (!m_isStarted || !parent() || !m_plugin->pluginFuncs()->setwindow)
        return;

    m_npWindow.x = m_windowRect.x();
    m_npWindow.y = m_windowRect.y();
    m_npWindow.width = m_windowRect.width();
    m_npWindow.height = m_windowRect.height();

    m_npWindow.clipRect.left = m_clipRect.x();
    m_npWindow.clipRect.top = m_clipRect.y();
    m_npWindow.clipRect.right = m_clipRect.width();
    m_npWindow.clipRect.bottom = m_clipRect.height();
	/*
    if (m_npWindow.x < 0 || m_npWindow.y < 0 ||
        m_npWindow.width <= 0 || m_npWindow.height <= 0)
        return;
	*/

	//kprintf("PluginView::setNPWindowRect(%d %d %d %d)\n", m_npWindow.x, m_npWindow.y, m_npWindow.width, m_npWindow.height);

    PluginView::setCurrentPluginView(this);
    JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
    setCallingPlugin(true);
    m_plugin->pluginFuncs()->setwindow(m_instance, &m_npWindow);
    setCallingPlugin(false);
    PluginView::setCurrentPluginView(0);
}

void PluginView::setParentVisible(bool visible)
{
}

NPError PluginView::handlePostReadFile(Vector<char>& buffer, uint32_t len, const char* buf)
{
    String filename(buf, len);

    if (filename.startsWith("file:///"))
        filename = filename.substring(8);

    //FIXME - read the file data into buffer
	FILE* fileHandle = fopen((filename.latin1()).data(), "r");

    if (fileHandle == 0)
        return NPERR_FILE_NOT_FOUND;

    //buffer.resize();

    int bytesRead = fread(buffer.data(), 1, 0, fileHandle);

    fclose(fileHandle);

    if (bytesRead <= 0)
        return NPERR_FILE_NOT_FOUND;

    return NPERR_NO_ERROR;
}

bool PluginView::platformGetValueStatic(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVToolkit:
		*static_cast<uint32_t*>(value) = 0;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVSupportsXEmbedBool:
        *static_cast<NPBool*>(value) = false;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVjavascriptEnabledBool:
        *static_cast<NPBool*>(value) = true;
        *result = NPERR_NO_ERROR;
        return true;

    case NPNVSupportsWindowless:
        *static_cast<NPBool*>(value) = false;
        *result = NPERR_NO_ERROR;
        return true;

    default:
        return false;
    }
}

bool PluginView::platformGetValue(NPNVariable variable, void* value, NPError* result)
{
    switch (variable) {
    case NPNVxDisplay:
        *result = NPERR_GENERIC_ERROR;
        return true;

        case NPNVnetscapeWindow: {
            *result = NPERR_NO_ERROR;
            return true;
        }

    default:
        return false;
    }
}

void PluginView::invalidateRegion(NPRegion)
{
	//kprintf("PluginView::invalidateRegion\n");
}

void PluginView::invalidateRect(NPRect* rect)
{
	//kprintf("PluginView::invalidateRect [%d %d %d %d]\n", rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);

    if (!rect) {
        invalidate();
        return;
    }

    IntRect r(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
    Widget::invalidateRect(r);
}

void PluginView::invalidateRect(const IntRect& rect)
{
	//kprintf("PluginView::invalidateRect2 [%d %d %d %d]\n", rect.x(), rect.y(), rect.width(), rect.height());
	Widget::invalidateRect(rect);
}

void PluginView::forceRedraw()
{
	//kprintf("PluginView::forceRedraw\n");
	invalidate();
}

bool PluginView::platformStart()
{
    ASSERT(m_isStarted);
    ASSERT(m_status == PluginStatusLoadedSuccessfully);

/*
    if (m_plugin->pluginFuncs()->getvalue) {
        PluginView::setCurrentPluginView(this);
        JSC::JSLock::DropAllLocks dropAllLocks(JSC::SilenceAssertionsOnly);
        setCallingPlugin(true);
        m_plugin->pluginFuncs()->getvalue(m_instance, NPPVpluginNeedsXEmbed, &m_needsXEmbed);
        setCallingPlugin(false);
        PluginView::setCurrentPluginView(0);
    }
*/

	setPlatformPluginWidget(m_parentFrame.get()->view()->hostWindow()->platformPageClient());

	//kprintf("platformStart() widget %p\n", m_parentFrame.get()->view()->hostWindow()->platformPageClient());

    show();

    if (m_isWindowed)
	{
        m_npWindow.type = NPWindowTypeWindow;
		m_npWindow.window = (void*) platformPluginWidget() ? platformPluginWidget()->browser : 0;
	}
    else {
        m_npWindow.type = NPWindowTypeDrawable;
		m_npWindow.window = 0;
    }

    // TODO remove in favor of null events, like mac port?
/*
    if (!(m_plugin->quirks().contains(PluginQuirkDeferFirstSetWindowCall)))
        updatePluginWidget(); // was: setNPWindowIfNeeded(), but this doesn't produce 0x0 rects at first go
*/
    updatePluginWidget();

    return true;
}

void PluginView::platformDestroy()
{
}

} // namespace WebCore
