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
#include "WebHistoryItem.h"
#include "WebHistoryItem_p.h"

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <HistoryItem.h>

#include <clib/debug_protos.h>

using namespace WebCore;

static HashMap<HistoryItem*, WebHistoryItem*>& historyItemWrappers()
{
    static HashMap<HistoryItem*, WebHistoryItem*> staticHistoryItemWrappers;
    return staticHistoryItemWrappers;
}

WebHistoryItem::WebHistoryItem(WebHistoryItemPrivate *priv)
: d(priv)
{
    ASSERT(!historyItemWrappers().contains(d->m_historyItem.get()));
    historyItemWrappers().set(d->m_historyItem.get(), this);
}

WebHistoryItem::~WebHistoryItem()
{
    ASSERT(historyItemWrappers().contains(d->m_historyItem.get()));
    historyItemWrappers().remove(d->m_historyItem.get());
    delete d;
}

WebHistoryItem* WebHistoryItem::createInstance()
{
    WebHistoryItemPrivate *priv = new WebHistoryItemPrivate(HistoryItem::create());
    WebHistoryItem* instance = new WebHistoryItem(priv);
    return instance;
}

WebHistoryItem* WebHistoryItem::createInstance(WebHistoryItemPrivate *priv)
{
    WebHistoryItem* instance;

    instance = historyItemWrappers().get(priv->m_historyItem.get());

    if (!instance)
        instance = new WebHistoryItem(priv);

    return instance;
}

bool WebHistoryItem::hasURLString()
{
    return d->m_historyItem->urlString().isEmpty() ? false : true;
}

void WebHistoryItem::setTitle(const char* title)
{
    d->m_historyItem->setTitle(title);
}

const char* WebHistoryItem::RSSFeedReferrer()
{
    return strdup(d->m_historyItem->referrer().utf8().data());
}

void WebHistoryItem::setRSSFeedReferrer(const char* url)
{
    d->m_historyItem->setReferrer(url);
}

bool WebHistoryItem::hasPageCache()
{
    // FIXME - TODO
    return false;
}

void WebHistoryItem::setHasPageCache(bool /*hasCache*/)
{
    // FIXME - TODO
}

const char* WebHistoryItem::target()
{
    return strdup(d->m_historyItem->target().utf8().data());
}

bool WebHistoryItem::isTargetItem()
{
    return d->m_historyItem->isTargetItem() ? true : false;
}

std::vector<WebHistoryItem*> WebHistoryItem::children()
{
    std::vector<WebHistoryItem*> child;
    const HistoryItemVector& coreChildren = d->m_historyItem->children();
    if (coreChildren.isEmpty())
        return child;
    size_t childCount = coreChildren.size();

    for (unsigned i = 0; i < childCount; ++i) {
        WebHistoryItemPrivate *priv = new WebHistoryItemPrivate((WebCore::HistoryItem *)coreChildren[i].ptr());
        WebHistoryItem* item = WebHistoryItem::createInstance(priv);
        child.push_back(item);
    }

    return child;
}

void WebHistoryItem::initWithURLString(const char* urlString, const char* title, double lastVisited)
{
    historyItemWrappers().remove(d->m_historyItem.get());
    d->m_historyItem = HistoryItem::create(urlString, title);
    historyItemWrappers().set(d->m_historyItem.get(), this);
}

void WebHistoryItem::initWithURLString(const String & urlString, const String & title, double lastVisited)
{
    historyItemWrappers().remove(d->m_historyItem.get());
    d->m_historyItem = HistoryItem::create(urlString, title);
    historyItemWrappers().set(d->m_historyItem.get(), this);
}

const char* WebHistoryItem::originalURLString()
{
	return strdup(d->m_historyItem->originalURLString().utf8().data());
}

const char* WebHistoryItem::URLString()
{
	return strdup(d->m_historyItem->urlString().utf8().data());
}

const char* WebHistoryItem::title()
{
	return strdup(d->m_historyItem->title().utf8().data());
}

void WebHistoryItem::setAlternateTitle(const char* title)
{
    m_alternateTitle = title;
}

const char* WebHistoryItem::alternateTitle()
{
    return m_alternateTitle.c_str();
}

bool WebHistoryItem::lastVisitWasFailure()
{
    return d->m_historyItem->lastVisitWasFailure();
}

void WebHistoryItem::setLastVisitWasFailure(bool wasFailure)
{
    d->m_historyItem->setLastVisitWasFailure(wasFailure);
}

