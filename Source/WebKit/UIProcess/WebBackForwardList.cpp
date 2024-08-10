/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebBackForwardList.h"

#include "APIArray.h"
#include "Logging.h"
#include "SessionState.h"
#include "WebPageProxy.h"
#include <WebCore/DiagnosticLoggingClient.h>
#include <WebCore/DiagnosticLoggingKeys.h>
#include <wtf/DebugUtilities.h>
#include <wtf/text/StringBuilder.h>

namespace WebKit {
using namespace WebCore;

static const unsigned DefaultCapacity = 100;

WebBackForwardList::WebBackForwardList(WebPageProxy& page)
    : m_page(&page)
    , m_hasCurrentIndex(false)
    , m_currentIndex(0)
    , m_capacity(DefaultCapacity)
{
    LOG(BackForward, "(Back/Forward) Created WebBackForwardList %p", this);
}

WebBackForwardList::~WebBackForwardList()
{
    LOG(BackForward, "(Back/Forward) Destroying WebBackForwardList %p", this);

    // A WebBackForwardList should never be destroyed unless it's associated page has been closed or is invalid.
    ASSERT((!m_page && !m_hasCurrentIndex) || !m_page->isValid());
}

WebBackForwardListItem* WebBackForwardList::itemForID(const BackForwardItemIdentifier& identifier)
{
    if (!m_page)
        return nullptr;

    auto* item = WebBackForwardListItem::itemForID(identifier);
    if (!item)
        return nullptr;

    ASSERT(item->pageID() == m_page->pageID());
    return item;
}

void WebBackForwardList::pageClosed()
{
    LOG(BackForward, "(Back/Forward) WebBackForwardList %p had its page closed with current size %zu", this, m_entries.size());

    // We should have always started out with an m_page and we should never close the page twice.
    ASSERT(m_page);

    if (m_page) {
        size_t size = m_entries.size();
        for (size_t i = 0; i < size; ++i)
            didRemoveItem(m_entries[i]);
    }

    m_page = nullptr;
    m_entries.clear();
    m_hasCurrentIndex = false;
}

void WebBackForwardList::addItem(Ref<WebBackForwardListItem>&& newItem)
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    if (!m_capacity || !m_page)
        return;

    Vector<Ref<WebBackForwardListItem>> removedItems;
    
    if (m_hasCurrentIndex) {
        m_page->recordAutomaticNavigationSnapshot();

        // Toss everything in the forward list.
        unsigned targetSize = m_currentIndex + 1;
        removedItems.reserveCapacity(m_entries.size() - targetSize);
        while (m_entries.size() > targetSize) {
            didRemoveItem(m_entries.last());
            removedItems.append(WTFMove(m_entries.last()));
            m_entries.removeLast();
        }

        // Toss the first item if the list is getting too big, as long as we're not using it
        // (or even if we are, if we only want 1 entry).
        if (m_entries.size() == m_capacity && (m_currentIndex || m_capacity == 1)) {
            didRemoveItem(m_entries[0]);
            removedItems.append(WTFMove(m_entries[0]));
            m_entries.remove(0);

            if (m_entries.isEmpty())
                m_hasCurrentIndex = false;
            else
                m_currentIndex--;
        }
    } else {
        // If we have no current item index we should also not have any entries.
        ASSERT(m_entries.isEmpty());

        // But just in case it does happen in practice we'll get back in to a consistent state now before adding the new item.
        size_t size = m_entries.size();
        for (size_t i = 0; i < size; ++i) {
            didRemoveItem(m_entries[i]);
            removedItems.append(WTFMove(m_entries[i]));
        }
        m_entries.clear();
    }

    bool shouldKeepCurrentItem = true;

    if (!m_hasCurrentIndex) {
        ASSERT(m_entries.isEmpty());
        m_currentIndex = 0;
        m_hasCurrentIndex = true;
    } else {
        shouldKeepCurrentItem = m_page->shouldKeepCurrentBackForwardListItemInList(m_entries[m_currentIndex]);
        if (shouldKeepCurrentItem)
            m_currentIndex++;
    }

    auto* newItemPtr = newItem.ptr();
    if (!shouldKeepCurrentItem) {
        // m_current should never be pointing past the end of the entries Vector.
        // If it is, something has gone wrong and we should not try to swap in the new item.
        ASSERT(m_currentIndex < m_entries.size());

        removedItems.append(m_entries[m_currentIndex].copyRef());
        m_entries[m_currentIndex] = WTFMove(newItem);
    } else {
        // m_current should never be pointing more than 1 past the end of the entries Vector.
        // If it is, something has gone wrong and we should not try to insert the new item.
        ASSERT(m_currentIndex <= m_entries.size());

        if (m_currentIndex <= m_entries.size())
            m_entries.insert(m_currentIndex, WTFMove(newItem));
    }

    LOG(BackForward, "(Back/Forward) WebBackForwardList %p added an item. Current size %zu, current index %zu, threw away %zu items", this, m_entries.size(), m_currentIndex, removedItems.size());
    m_page->didChangeBackForwardList(newItemPtr, WTFMove(removedItems));
}

void WebBackForwardList::goToItem(WebBackForwardListItem& item)
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    if (!m_entries.size() || !m_page || !m_hasCurrentIndex)
        return;

    size_t targetIndex = notFound;
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].ptr() == &item) {
            targetIndex = i;
            break;
        }
    }

    // If the target item wasn't even in the list, there's nothing else to do.
    if (targetIndex == notFound) {
        LOG(BackForward, "(Back/Forward) WebBackForwardList %p could not go to item %s (%s) because it was not found", this, item.itemID().logString(), item.url().utf8().data());
        return;
    }

    if (targetIndex < m_currentIndex) {
        unsigned delta = m_entries.size() - targetIndex - 1;
        String deltaValue = delta > 10 ? "over10"_s : String::number(delta);
        m_page->logDiagnosticMessage(WebCore::DiagnosticLoggingKeys::backNavigationDeltaKey(), deltaValue, ShouldSample::No);
    }

    // If we're going to an item different from the current item, ask the client if the current
    // item should remain in the list.
    auto& currentItem = m_entries[m_currentIndex];
    bool shouldKeepCurrentItem = true;
    if (currentItem.ptr() != &item) {
        m_page->recordAutomaticNavigationSnapshot();
        shouldKeepCurrentItem = m_page->shouldKeepCurrentBackForwardListItemInList(m_entries[m_currentIndex]);
    }

    // If the client said to remove the current item, remove it and then update the target index.
    Vector<Ref<WebBackForwardListItem>> removedItems;
    if (!shouldKeepCurrentItem) {
        removedItems.append(currentItem.copyRef());
        m_entries.remove(m_currentIndex);
        targetIndex = notFound;
        for (size_t i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].ptr() == &item) {
                targetIndex = i;
                break;
            }
        }
        ASSERT(targetIndex != notFound);
    }

    m_currentIndex = targetIndex;

    LOG(BackForward, "(Back/Forward) WebBackForwardList %p going to item %s, is now at index %zu", this, item.itemID().logString(), targetIndex);

    m_page->didChangeBackForwardList(nullptr, WTFMove(removedItems));
}

WebBackForwardListItem* WebBackForwardList::currentItem() const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    return m_page && m_hasCurrentIndex ? m_entries[m_currentIndex].ptr() : nullptr;
}

WebBackForwardListItem* WebBackForwardList::backItem() const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    return m_page && m_hasCurrentIndex && m_currentIndex ? m_entries[m_currentIndex - 1].ptr() : nullptr;
}

WebBackForwardListItem* WebBackForwardList::forwardItem() const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    return m_page && m_hasCurrentIndex && m_entries.size() && m_currentIndex < m_entries.size() - 1 ? m_entries[m_currentIndex + 1].ptr() : nullptr;
}

WebBackForwardListItem* WebBackForwardList::itemAtIndex(int index) const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    if (!m_hasCurrentIndex || !m_page)
        return nullptr;
    
    // Do range checks without doing math on index to avoid overflow.
    if (index < -backListCount())
        return nullptr;
    
    if (index > forwardListCount())
        return nullptr;
        
    return m_entries[index + m_currentIndex].ptr();
}

int WebBackForwardList::backListCount() const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    return m_page && m_hasCurrentIndex ? m_currentIndex : 0;
}

int WebBackForwardList::forwardListCount() const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    return m_page && m_hasCurrentIndex ? m_entries.size() - (m_currentIndex + 1) : 0;
}

Ref<API::Array> WebBackForwardList::backList() const
{
    return backListAsAPIArrayWithLimit(backListCount());
}

Ref<API::Array> WebBackForwardList::forwardList() const
{
    return forwardListAsAPIArrayWithLimit(forwardListCount());
}

Ref<API::Array> WebBackForwardList::backListAsAPIArrayWithLimit(unsigned limit) const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    if (!m_page || !m_hasCurrentIndex)
        return API::Array::create();

    unsigned backListSize = static_cast<unsigned>(backListCount());
    unsigned size = std::min(backListSize, limit);
    if (!size)
        return API::Array::create();

    Vector<RefPtr<API::Object>> vector;
    vector.reserveInitialCapacity(size);

    ASSERT(backListSize >= size);
    for (unsigned i = backListSize - size; i < backListSize; ++i)
        vector.uncheckedAppend(m_entries[i].ptr());

    return API::Array::create(WTFMove(vector));
}

Ref<API::Array> WebBackForwardList::forwardListAsAPIArrayWithLimit(unsigned limit) const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    if (!m_page || !m_hasCurrentIndex)
        return API::Array::create();

    unsigned size = std::min(static_cast<unsigned>(forwardListCount()), limit);
    if (!size)
        return API::Array::create();

    Vector<RefPtr<API::Object>> vector;
    vector.reserveInitialCapacity(size);

    unsigned last = m_currentIndex + size;
    ASSERT(last < m_entries.size());
    for (unsigned i = m_currentIndex + 1; i <= last; ++i)
        vector.uncheckedAppend(m_entries[i].ptr());

    return API::Array::create(WTFMove(vector));
}

void WebBackForwardList::removeAllItems()
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    LOG(BackForward, "(Back/Forward) WebBackForwardList %p removeAllItems (has %zu of them)", this, m_entries.size());

    Vector<Ref<WebBackForwardListItem>> removedItems;

    for (auto& entry : m_entries) {
        didRemoveItem(entry);
        removedItems.append(WTFMove(entry));
    }

    m_entries.clear();
    m_hasCurrentIndex = false;
    m_page->didChangeBackForwardList(nullptr, WTFMove(removedItems));
}

void WebBackForwardList::clear()
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    LOG(BackForward, "(Back/Forward) WebBackForwardList %p clear (has %zu of them)", this, m_entries.size());

    size_t size = m_entries.size();
    if (!m_page || size <= 1)
        return;

    RefPtr<WebBackForwardListItem> currentItem = this->currentItem();
    Vector<Ref<WebBackForwardListItem>> removedItems;

    if (!currentItem) {
        // We should only ever have no current item if we also have no current item index.
        ASSERT(!m_hasCurrentIndex);

        // But just in case it does happen in practice we should get back into a consistent state now.
        for (size_t i = 0; i < size; ++i) {
            didRemoveItem(m_entries[i]);
            removedItems.append(WTFMove(m_entries[i]));
        }

        m_entries.clear();
        m_hasCurrentIndex = false;
        m_page->didChangeBackForwardList(nullptr, WTFMove(removedItems));

        return;
    }

    for (size_t i = 0; i < size; ++i) {
        if (m_entries[i].ptr() != currentItem)
            didRemoveItem(m_entries[i]);
    }

    removedItems.reserveCapacity(size - 1);
    for (size_t i = 0; i < size; ++i) {
        if (i != m_currentIndex && m_hasCurrentIndex)
            removedItems.append(WTFMove(m_entries[i]));
    }

    m_currentIndex = 0;

    m_entries.clear();
    if (currentItem)
        m_entries.append(currentItem.releaseNonNull());
    else
        m_hasCurrentIndex = false;
    m_page->didChangeBackForwardList(nullptr, WTFMove(removedItems));
}

BackForwardListState WebBackForwardList::backForwardListState(WTF::Function<bool (WebBackForwardListItem&)>&& filter) const
{
    ASSERT(!m_hasCurrentIndex || m_currentIndex < m_entries.size());

    BackForwardListState backForwardListState;
    if (m_hasCurrentIndex)
        backForwardListState.currentIndex = m_currentIndex;

    for (size_t i = 0; i < m_entries.size(); ++i) {
        auto& entry = m_entries[i];

        if (filter && !filter(entry)) {
            auto& currentIndex = backForwardListState.currentIndex;
            if (currentIndex && i <= currentIndex.value() && currentIndex.value())
                --currentIndex.value();

            continue;
        }

        backForwardListState.items.append(entry->itemState());
    }

    if (backForwardListState.items.isEmpty())
        backForwardListState.currentIndex = std::nullopt;
    else if (backForwardListState.items.size() <= backForwardListState.currentIndex.value())
        backForwardListState.currentIndex = backForwardListState.items.size() - 1;

    return backForwardListState;
}

void WebBackForwardList::restoreFromState(BackForwardListState backForwardListState)
{
    if (!m_page)
        return;

    Vector<Ref<WebBackForwardListItem>> items;
    items.reserveInitialCapacity(backForwardListState.items.size());

    for (auto& backForwardListItemState : backForwardListState.items) {
        backForwardListItemState.identifier = { Process::identifier(), generateObjectIdentifier<BackForwardItemIdentifier::ItemIdentifierType>() };
        items.uncheckedAppend(WebBackForwardListItem::create(WTFMove(backForwardListItemState), m_page->pageID()));
    }
    m_hasCurrentIndex = !!backForwardListState.currentIndex;
    m_currentIndex = backForwardListState.currentIndex.value_or(0);
    m_entries = WTFMove(items);

    LOG(BackForward, "(Back/Forward) WebBackForwardList %p restored from state (has %zu entries)", this, m_entries.size());
}

Vector<BackForwardListItemState> WebBackForwardList::filteredItemStates(Function<bool(WebBackForwardListItem&)>&& functor) const
{
    Vector<BackForwardListItemState> itemStates;
    itemStates.reserveInitialCapacity(m_entries.size());

    for (const auto& entry : m_entries) {
        if (functor(entry))
            itemStates.uncheckedAppend(entry->itemState());
    }

    return itemStates;
}

Vector<BackForwardListItemState> WebBackForwardList::itemStates() const
{
    return filteredItemStates([](WebBackForwardListItem&) {
        return true;
    });
}

void WebBackForwardList::didRemoveItem(WebBackForwardListItem& backForwardListItem)
{
    m_page->backForwardRemovedItem(backForwardListItem.itemID());

#if PLATFORM(COCOA)
    backForwardListItem.setSnapshot(nullptr);
#endif
}


#if !LOG_DISABLED
const char* WebBackForwardList::loggingString()
{
    StringBuilder builder;
    builder.append(String::format("WebBackForwardList %p - %zu entries, has current index %s (%zu)", this, m_entries.size(), m_hasCurrentIndex ? "YES" : "NO", m_hasCurrentIndex ? m_currentIndex : 0));

    for (size_t i = 0; i < m_entries.size(); ++i) {
        builder.append("\n");
        if (m_hasCurrentIndex && m_currentIndex == i)
            builder.append(" * ");
        else
            builder.append(" - ");

        builder.append(m_entries[i]->loggingString());
    }

    return debugString("\n", builder.toString());
}
#endif // !LOG_DISABLED

} // namespace WebKit
