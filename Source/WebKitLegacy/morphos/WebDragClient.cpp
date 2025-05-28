#include "WebDragClient.h"
#include "WebPage.h"

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
//	dprintf("%s\n", __PRETTY_FUNCTION__);
	m_page->startDrag(WTFMove(item), transfer, frame);
}

void WebDragClient::didConcludeEditDrag()
{
}

} // namespace WebKit
