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

#include "DataObjectMorphOS.h"
#include <WebCore/DataTransfer.h>
#include <WebCore/DragController.h>
#include <WebCore/Pasteboard.h>
#include <WebCore/SharedBuffer.h>
#include "WebDragClient.h"
#include "WebDropSource.h"
#include "WebView.h"

#include <WebCore/DragData.h>
#include <WebCore/Font.h>
#include <WebCore/FontDescription.h>
#include <WebCore/FontSelector.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/Page.h>
#include <WebCore/StringTruncator.h>

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


void WebDragClient::willPerformDragDestinationAction(DragDestinationAction action, const DragData& dragData)
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

void WebDragClient::startDrag(DragItem item, DataTransfer& dataTransfer, Frame& frame)
{
    D(kprintf("startDrag image %p islink %d imageOrigin (%d %d) dragPoint (%d %d) clipboard %p\n", image, isLink, imageOrigin.x(), imageOrigin.y(), dragPoint.x(), dragPoint.y(), clipboard));

    BalWidget *widget = m_webView->viewWindow();
    // Freed only when next drag images comes or when client is destroyed
    m_dragImage = WTFMove(item.image);

    if(widget)
    {
        RefPtr<DataObjectMorphOS> dataObject = dataTransfer.pasteboard().dataObject();
        D(kprintf("dataObject %p\n", dataObject.get()));

        if (item.sourceAction != DragSourceActionLink)
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
        set(widget->browser, MA_OWBBrowser_DragImage, m_dragImage.get().get());
        set(widget->browser, MA_OWBBrowser_DragData, dataObject.get());
        set(widget->browser, MA_OWBBrowser_DragOperation, dataTransfer.sourceOperation());

        DoMethod(widget->browser, MUIM_DoDrag, 0x80000000,0x80000000, 0);

#if !OS(AROS) // AROS has asynchronous drag & drop by default
        D(kprintf("dragEnded\n"));
        core(m_webView)->dragController().dragEnded();

        D(kprintf("after MUIM_DoDrag, resetting drag data\n"));
        set(widget->browser, MA_OWBBrowser_DragURL, "");
        set(widget->browser, MA_OWBBrowser_DragImage, 0);
        set(widget->browser, MA_OWBBrowser_DragData, 0);
        set(widget->browser, MA_OWBBrowser_DragOperation, DragOperationNone);
#endif
    }
}

