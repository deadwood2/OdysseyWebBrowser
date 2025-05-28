#include "BackForwardClient.h"
#include <WebCore/HistoryItem.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/FrameLoaderClient.h>
#include <WebCore/BackForwardCache.h>
#include "WebPage.h"

using namespace WebCore;
using namespace WTF;

namespace WebKit {

static const unsigned DefaultCapacity = 100;
static const unsigned NoCurrentItemIndex = UINT_MAX;

using namespace WebCore;

BackForwardClientMorphOS::BackForwardClientMorphOS(WebPage *page)
    : m_page(page)
    , m_current(NoCurrentItemIndex)
    , m_capacity(DefaultCapacity)
    , m_closed(true)
    , m_enabled(true)
{
}

void BackForwardClientMorphOS::addItem(Ref<HistoryItem>&& newItem)
{
    if (!m_capacity || !m_enabled)
        return;
	
    // Toss anything in the forward list
    if (m_current != NoCurrentItemIndex) {
        unsigned targetSize = m_current + 1;
        while (m_entries.size() > targetSize) {
            Ref<HistoryItem> item = m_entries.takeLast();
            m_entryHash.remove(item.ptr());
            BackForwardCache::singleton().remove(item);
        }
    }

    // Toss the first item if the list is getting too big, as long as we're not using it
    // (or even if we are, if we only want 1 entry).
    if (m_entries.size() == m_capacity && (m_current || m_capacity == 1)) {
        Ref<HistoryItem> item = WTFMove(m_entries[0]);
        m_entries.remove(0);
        m_entryHash.remove(item.ptr());
        BackForwardCache::singleton().remove(item);
        --m_current;
    }

    m_entryHash.add(newItem.ptr());
    m_entries.insert(m_current + 1, WTFMove(newItem));
    ++m_current;
	
	if (m_page->_fHistoryChanged)
    	m_page->_fHistoryChanged();
}

void BackForwardClientMorphOS::goBack()
{
    ASSERT(m_current > 0);
    if (m_current > 0) {
        m_current--;

		if (m_page->_fHistoryChanged)
			m_page->_fHistoryChanged();
    }
}

void BackForwardClientMorphOS::goForward()
{
    ASSERT(m_current < m_entries.size() - 1);
    if (m_current < m_entries.size() - 1) {
        m_current++;

		if (m_page->_fHistoryChanged)
			m_page->_fHistoryChanged();
    }
}

void BackForwardClientMorphOS::goToItem(HistoryItem& item)
{
    if (!m_entries.size())
        return;

    unsigned int index = 0;
    for (; index < m_entries.size(); ++index)
        if (m_entries[index].ptr() == &item)
            break;
    if (index < m_entries.size()) {
        m_current = index;
		if (m_page->_fHistoryChanged)
			m_page->_fHistoryChanged();
    }
}

RefPtr<HistoryItem> BackForwardClientMorphOS::backItem()
{
    if (m_current && m_current != NoCurrentItemIndex)
        return m_entries[m_current - 1].copyRef();
    return nullptr;
}

RefPtr<HistoryItem> BackForwardClientMorphOS::currentItem()
{
    if (m_current != NoCurrentItemIndex)
        return m_entries[m_current].copyRef();
    return nullptr;
}

RefPtr<HistoryItem> BackForwardClientMorphOS::forwardItem()
{
    if (m_entries.size() && m_current < m_entries.size() - 1)
        return m_entries[m_current + 1].copyRef();
    return nullptr;
}

void BackForwardClientMorphOS::backListWithLimit(int limit, Vector<Ref<HistoryItem>>& list)
{
    list.clear();

    if (!m_entries.size())
        return;
	
    unsigned lastEntry = m_entries.size() - 1;
    if (m_current < lastEntry) {
        int last = std::min(m_current + limit, lastEntry);
        limit = m_current + 1;
        for (; limit <= last; ++limit)
            list.append(m_entries[limit].get());
    }
}

void BackForwardClientMorphOS::forwardListWithLimit(int limit, Vector<Ref<HistoryItem>>& list)
{
    list.clear();
    if (m_current != NoCurrentItemIndex) {
        unsigned first = std::max(static_cast<int>(m_current) - limit, 0);
        for (; first < m_current; ++first)
            list.append(m_entries[first].get());
    }
}

int BackForwardClientMorphOS::capacity()
{
    return m_capacity;
}

void BackForwardClientMorphOS::setCapacity(int size)
{
    while (size < static_cast<int>(m_entries.size())) {
        Ref<HistoryItem> item = m_entries.takeLast();
        m_entryHash.remove(item.ptr());
        BackForwardCache::singleton().remove(item);
    }

    if (!size)
        m_current = NoCurrentItemIndex;
    else if (m_current > m_entries.size() - 1) {
        m_current = m_entries.size() - 1;
    }
    m_capacity = size;
	if (m_page->_fHistoryChanged)
		m_page->_fHistoryChanged();
}

bool BackForwardClientMorphOS::enabled()
{
    return m_enabled;
}

void BackForwardClientMorphOS::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (!enabled) {
        int capacity = m_capacity;
        setCapacity(0);
        setCapacity(capacity);
		if (m_page->_fHistoryChanged)
			m_page->_fHistoryChanged();
    }
}

unsigned BackForwardClientMorphOS::backListCount() const
{
    return m_current == NoCurrentItemIndex ? 0 : m_current;
}

unsigned BackForwardClientMorphOS::forwardListCount() const
{
    return m_current == NoCurrentItemIndex ? 0 : m_entries.size() - m_current - 1;
}

RefPtr<HistoryItem> BackForwardClientMorphOS::itemAtIndex(int index)
{
    // Do range checks without doing math on index to avoid overflow.
    if (index < -static_cast<int>(m_current))
        return nullptr;
	
    if (index > static_cast<int>(forwardListCount()))
        return nullptr;

    return m_entries[index + m_current].copyRef();
}

Vector<Ref<HistoryItem>>& BackForwardClientMorphOS::entries()
{
    return m_entries;
}

void BackForwardClientMorphOS::close()
{
    m_entries.clear();
    m_entryHash.clear();
    m_closed = true;
}

bool BackForwardClientMorphOS::closed()
{
    return m_closed;
}

void BackForwardClientMorphOS::removeItem(HistoryItem* item)
{
    if (!item)
        return;
	
    for (unsigned i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].ptr() == item) {
            m_entries.remove(i);
            m_entryHash.remove(item);
            if (m_current == NoCurrentItemIndex || m_current < i)
                break;
            if (m_current > i)
                m_current--;
            else {
                size_t count = m_entries.size();
                if (m_current >= count)
                    m_current = count ? count - 1 : NoCurrentItemIndex;
            }
            break;
        }
    }

	if (m_page->_fHistoryChanged)
		m_page->_fHistoryChanged();
}

bool BackForwardClientMorphOS::containsItem(HistoryItem* entry)
{
    return m_entryHash.contains(entry);
}


}

