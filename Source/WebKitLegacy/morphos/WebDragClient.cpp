#include "WebDragClient.h"
#include "WebPage.h"
#include <WebCore/LocalFrame.h>

extern "C" { void dprintf(const char *,...); }

namespace WebKit {
using namespace WebCore;

void WebDragClient::willPerformDragDestinationAction(DragDestinationAction , const DragData&)
{
//	dprintf("%s\n", __PRETTY_FUNCTION__);
}

void WebDragClient::willPerformDragSourceAction(DragSourceAction, const IntPoint&, DataTransfer&)
{
//	dprintf("%s\n", __PRETTY_FUNCTION__);
}

OptionSet<WebCore::DragSourceAction> WebDragClient::dragSourceActionMaskForPoint(const WebCore::IntPoint& )
{
//	dprintf("%s\n", __PRETTY_FUNCTION__);
    return WebCore::anyDragSourceAction(); //m_page->allowedDragSourceActions();
}

void WebDragClient::startDrag(DragItem item, DataTransfer& transfer, Frame& frame)
{
    auto* localFrame = dynamicDowncast<LocalFrame>(frame);
    if (!localFrame)
        return;
//	dprintf("%s\n", __PRETTY_FUNCTION__);
    auto page = m_page.get();
    if (page)
        page->startDrag(WTFMove(item), transfer, *localFrame);
}

void WebDragClient::didConcludeEditDrag()
{
}

} // namespace WebKit
