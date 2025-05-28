#include "PopupMenu.h"
#include "WebPage.h"
#include <WebCore/IntRect.h>
#include <WebCore/FrameView.h>
#include <wtf/text/AtomString.h>

extern "C" { void dprintf(const char *,...); }

using namespace WebCore;
using namespace WTF;

namespace WebKit {

PopupMenuMorphOS::PopupMenuMorphOS(WebCore::PopupMenuClient* client, WebPage* page)
	: m_page(page)
	, m_client(client)
{

}

PopupMenuMorphOS::~PopupMenuMorphOS()
{

}

void PopupMenuMorphOS::show(const WebCore::IntRect& rect, WebCore::FrameView* view, int)
{
	WTF::Vector<WTF::String> items;
	RefPtr<PopupMenu> protectedThis(this);

	if (m_client && m_page->_fPopup)
	{
		for (int i =0 ; i < m_client->listSize(); i++)
			items.append(m_client->itemText(i));

    	IntRect rViewCoords(view->contentsToWindow(rect.location()), rect.size());

		int selection = m_page->_fPopup(rViewCoords, items);

		if (m_client)
		{
			m_client->popupDidHide();
			if (selection >= 0)
				m_client->valueChanged(selection);
		}

		hide();
	}
}

void PopupMenuMorphOS::hide()
{
	if (m_client)
		m_client->popupDidHide();
}

void PopupMenuMorphOS::updateFromElement()
{

}

void PopupMenuMorphOS::disconnectClient()
{
	m_client = nullptr;
}

SearchPopupMenuMorphOS::SearchPopupMenuMorphOS(PopupMenuClient* client, WebPage* page)
    : m_popup(adoptRef(*new PopupMenuMorphOS(client, page)))
{
}

PopupMenu* SearchPopupMenuMorphOS::popupMenu()
{
    return m_popup.ptr();
}

void SearchPopupMenuMorphOS::saveRecentSearches(const AtomString&, const Vector<RecentSearch>& /*searchItems*/)
{
}

void SearchPopupMenuMorphOS::loadRecentSearches(const AtomString&, Vector<RecentSearch>& /*searchItems*/)
{
}

bool SearchPopupMenuMorphOS::enabled()
{
    return false;
}
	
}
