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
#include "WebBackForwardList.h"
#include "WebBackForwardList_p.h"
#include "WebHistoryItem.h"
#include "WebHistoryItem_p.h"

#include "WebFrame.h"
#include "WebPreferences.h"

#include <BackForwardList.h>
#include <HistoryItem.h>
#include <Page.h>
#include <PageGroup.h>

using std::min;
using namespace WebCore;

static HashMap<BackForwardList*, WebBackForwardList*>& backForwardListWrappers()
{
    static HashMap<BackForwardList*, WebBackForwardList*> staticBackForwardListWrappers;
    return staticBackForwardListWrappers;
}

WebBackForwardList::WebBackForwardList(WebBackForwardListPrivate *priv)
    : d(priv)
{
    ASSERT(!backForwardListWrappers().contains(d->m_backForwardList));
    backForwardListWrappers().set(d->m_backForwardList, this);
}

WebBackForwardList::~WebBackForwardList()
{
    ASSERT(d->m_backForwardList->closed());

    ASSERT(backForwardListWrappers().contains(d->m_backForwardList));
    backForwardListWrappers().remove(d->m_backForwardList);
    delete d;
}

WebBackForwardList* WebBackForwardList::createInstance(WebBackForwardListPrivate *priv)
{
    WebBackForwardList* instance;

    instance = backForwardListWrappers().get(priv->m_backForwardList);

    if (!instance)
        instance = new WebBackForwardList(priv);

    return instance;
}

#if 0
void WebBackForwardList::clear()
{
    //shortcut to private BackForwardList
	RefPtr<WebCore::BackForwardListImpl> lst = d->m_backForwardList;

    //clear visited links
    WebCore::Page* page = lst->page();
    if (page && page->groupPtr())
        page->groupPtr()->removeVisitedLinks();

    //if count() == 0 then just return
    if (!lst->entries().size())
        return;

    RefPtr<WebCore::HistoryItem> current = lst->currentItem();
    int capacity = lst->capacity();
    lst->setCapacity(0);

    lst->setCapacity(capacity);   //revert capacity
    lst->addItem(current.get());  //insert old current item
    lst->goToItem(current.get()); //and set it as current again
}
#endif

void WebBackForwardList::addItem(WebHistoryItem* item)
{
    d->m_backForwardList->addItem(*item->getPrivateItem()->m_historyItem);
}

void WebBackForwardList::goBack()
{
    d->m_backForwardList->goBack();
}

void WebBackForwardList::goForward()
{
    d->m_backForwardList->goForward();
}

void WebBackForwardList::goToItem(WebHistoryItem* item)
{
    d->m_backForwardList->goToItem(item->getPrivateItem()->m_historyItem.get());
}

WebHistoryItem* WebBackForwardList::backItem()
{
    HistoryItem* historyItem = d->m_backForwardList->backItem();

    if (!historyItem)
        return 0;

    WebHistoryItemPrivate *priv = new WebHistoryItemPrivate(historyItem);
    return WebHistoryItem::createInstance(priv);
}

WebHistoryItem* WebBackForwardList::currentItem()
{
    HistoryItem* historyItem = d->m_backForwardList->currentItem();

    if (!historyItem)
        return 0;

    WebHistoryItemPrivate *priv = new WebHistoryItemPrivate(historyItem);
    return WebHistoryItem::createInstance(priv);
}

WebHistoryItem* WebBackForwardList::forwardItem()
{
    HistoryItem* historyItem = d->m_backForwardList->forwardItem();

    if (!historyItem)
        return 0;

    WebHistoryItemPrivate *priv = new WebHistoryItemPrivate(historyItem);
    return WebHistoryItem::createInstance(priv);
}

int WebBackForwardList::backListWithLimit(int limit, WebHistoryItem** list)
{
    HistoryItemVector historyItemVector;
    d->m_backForwardList->backListWithLimit(limit, historyItemVector);

    if (list)
        for (unsigned i = 0; i < historyItemVector.size(); i++) {
            WebHistoryItemPrivate *priv = new WebHistoryItemPrivate(historyItemVector[i].ptr());
            list[i] = WebHistoryItem::createInstance(priv);
        }

    return static_cast<int>(historyItemVector.size());
}

int WebBackForwardList::forwardListWithLimit(int limit, WebHistoryItem** list)
{
    HistoryItemVector historyItemVector;
    d->m_backForwardList->forwardListWithLimit(limit, historyItemVector);

    if (list)
        for (unsigned i = 0; i < historyItemVector.size(); i++) {
            WebHistoryItemPrivate *priv = new WebHistoryItemPrivate(historyItemVector[i].ptr());
            list[i] = WebHistoryItem::createInstance(priv);
        }

    return static_cast<int>(historyItemVector.size());
}

int WebBackForwardList::capacity()
{
    return (int)d->m_backForwardList->capacity();
}

void WebBackForwardList::setCapacity(int size)
{
    if (size < 0)
        return;
    
    d->m_backForwardList->setCapacity(size);
}

int WebBackForwardList::backListCount()
{
    return d->m_backForwardList->backListCount();
}

int WebBackForwardList::forwardListCount()
{
    return d->m_backForwardList->forwardListCount();
}

bool WebBackForwardList::containsItem(WebHistoryItem* item)
{
    if (!item)
        return false;

    return d->m_backForwardList->containsItem(item->getPrivateItem()->m_historyItem.get());
}

WebHistoryItem* WebBackForwardList::itemAtIndex(int index)
{
    HistoryItem* historyItem = d->m_backForwardList->itemAtIndex(index);

    if (!historyItem)
        return 0;

    WebHistoryItemPrivate *priv = new WebHistoryItemPrivate(historyItem);
    return WebHistoryItem::createInstance(priv);
}

void WebBackForwardList::removeItem(WebHistoryItem* item)
{
    if (!item)
        return;

    d->m_backForwardList->removeItem(item->getPrivateItem()->m_historyItem.get());
}
