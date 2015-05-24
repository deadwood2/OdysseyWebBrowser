/*
 * Copyright (C) 2008 Pleyo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Pleyo nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PLEYO AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PLEYO OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DataObjectMorphOS.h"
#include "DataTransfer.h"
#include "DragController.h"
#include "Pasteboard.h"
#include "SharedBuffer.h"
#include "WebDragClient.h"
#include "WebDropSource.h"
#include "WebView.h"

#include <DragData.h>
#include <Font.h>
#include <FontDescription.h>
#include <FontSelector.h>
#include <Frame.h>
#include <FrameView.h>
#include <GraphicsContext.h>
#include <Page.h>
#include <StringTruncator.h>

#include <wtf/text/CString.h>
#include "gui.h"
#include "utils.h"
#include <clib/debug_protos.h>
#define D(x)

using namespace WebCore;

WebDragClient::WebDragClient(WebView* webView)
    : m_webView(webView) 
{
    ASSERT(webView);
}

WebDragClient::~WebDragClient()
{
}

DragDestinationAction WebDragClient::actionMaskForDrag(DragData& dragData)
{
    return DragDestinationActionAny;
}

void WebDragClient::willPerformDragDestinationAction(DragDestinationAction action, DragData& dragData)
{
	D(kprintf("willPerformDragDestinationAction action %d dragData %p\n", action, dragData));
}

DragSourceAction WebDragClient::dragSourceActionMaskForPoint(const IntPoint& windowPoint)
{
    D(kprintf("dragSourceActionMaskForPoint (%d %d)\n", windowPoint.x(), windowPoint.y()));
    return DragSourceActionAny;
}

void WebDragClient::willPerformDragSourceAction(DragSourceAction action, const IntPoint& startPos, DataTransfer&)
{
	D(kprintf("willPerformDragSourceAction %d (%d %d)\n", action, startPos.x(), startPos.y()));
    // FIXME: Implement this!
    // See WebKit/win/WebCoreSupport/WebDragClient.cpp for how to implement it.
}

void WebDragClient::startDrag(DragImageRef image, const IntPoint& imageOrigin, const IntPoint& dragPoint, DataTransfer& dataTransfer, Frame& frame, bool isLink)
{
    D(kprintf("startDrag image %p islink %d imageOrigin (%d %d) dragPoint (%d %d) clipboard %p\n", image, isLink, imageOrigin.x(), imageOrigin.y(), dragPoint.x(), dragPoint.y(), clipboard));

    BalWidget *widget = m_webView->viewWindow();

    if(widget)
    {
	RefPtr<DataObjectMorphOS> dataObject = dataTransfer.pasteboard().dataObject();
	D(kprintf("dataObject %p\n", dataObject.get()));

	if(!isLink)
	{
	    DragData dragData(dataObject.get(), IntPoint(0, 0), IntPoint(0,0), dataTransfer.sourceOperation());
	    char *data = NULL;
			
	    if(dragData.containsURL())
	    {
		data = utf8_to_local(dragData.asURL().utf8().data());
	    }
	    else if(dragData.containsPlainText())
	    {	
		data = utf8_to_local(dragData.asPlainText().utf8().data());
	    }
	    else
	    {
		data = strdup("<object>");
	    }

	    set(widget->browser, MA_OWBBrowser_DragURL, data);
	    free(data);
	}
	set(widget->browser, MA_OWBBrowser_DragImage, image);
	set(widget->browser, MA_OWBBrowser_DragData, dataObject.get());
	set(widget->browser, MA_OWBBrowser_DragOperation, dataTransfer.sourceOperation());

        DoMethod(widget->browser, MUIM_DoDrag, 0x80000000,0x80000000, 0);

	D(kprintf("dragEnded\n"));
	core(m_webView)->dragController().dragEnded();

	D(kprintf("after MUIM_DoDrag, resetting drag data\n"));
        set(widget->browser, MA_OWBBrowser_DragURL, "");
	set(widget->browser, MA_OWBBrowser_DragImage, 0);
	set(widget->browser, MA_OWBBrowser_DragData, 0);
	set(widget->browser, MA_OWBBrowser_DragOperation, DragOperationNone);
    }
}

DragImageRef WebDragClient::createDragImageForLink(URL& url, const String& inLabel, Frame*)
{
	D(kprintf("createDragImageForLink %s %s\n", url.string().latin1().data(), inLabel.latin1().data()));

	BalWidget *widget = m_webView->viewWindow();
	if(widget)
	{
		set(widget->browser, MA_OWBBrowser_DragURL, url.string().latin1().data());
	}
	return 0;
}

void WebDragClient::dragControllerDestroyed()
{
    delete this;
}
