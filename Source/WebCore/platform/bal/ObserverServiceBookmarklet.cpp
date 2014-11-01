/*
 * Copyright (C) 2007 Pleyo.  All rights reserved.
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

/**
 * @file ObserverService.cpp
 *
 * BAL Observer Service implementation
 */

#include "config.h"
#include "ObserverServiceBookmarklet.h"
#include "ObserverBookmarklet.h"
#include "Bookmarklet.h"
#include <wtf/text/CString.h>

namespace WebCore {
static bool observerServiceBookmarkletIsDeleted = false;

ObserverServiceBookmarklet* ObserverServiceBookmarklet::createObserverService()
{
    static ObserverServiceBookmarklet observerSingleton;
    return &observerSingleton;
}


ObserverServiceBookmarklet::ObserverServiceBookmarklet()
{
}

void ObserverServiceBookmarklet::registerObserver(const String& topic, Observer* observer)
{
    if (observerServiceBookmarkletIsDeleted)
        return;
    Vector<ObserverBookmarklet*> listObserver = m_topicData.get(topic);
    Vector<ObserverBookmarklet*>::iterator it;
    bool isSet = false;

    // Check that observer does not exist in listObserver
    // This asserts that observer is not added twice
    for (it = listObserver.begin(); it != listObserver.end(); it++) {
        if (*it == observer) {
            isSet = true;
            break;
        }
    }
    if (!isSet)
        listObserver.append(static_cast<ObserverBookmarklet*>(observer));

    // If topic does not exist, add it to the Hashmap
    // else set listObserver as a ref cannot be get on it via get() method
    if (listObserver.size() <= 1)
      /*pair<HashMap<String, Vector<ObserverBookmarklet*> >::iterator, bool> result  = */m_topicData.add(topic, listObserver);
    else
        m_topicData.set(topic, listObserver);
}

void ObserverServiceBookmarklet::notifyObserver(const String& topic, Bookmarklet *bookmarklet)
{
    if (observerServiceBookmarkletIsDeleted)
        return;
//    DBGML(MODULE_FACILITIES, LEVEL_INFO, "notify data %s for topic %s\n", data.utf8().data(), topic.utf8().data());

    Vector<ObserverBookmarklet*> listObserver = m_topicData.get(topic);
    Vector<ObserverBookmarklet*>::iterator it;
    for (it = listObserver.begin(); it != listObserver.end(); it++)
        (*it)->observe(topic, bookmarklet);
}

ObserverServiceBookmarklet::~ObserverServiceBookmarklet()
{
    observerServiceBookmarkletIsDeleted = true;
}

void ObserverServiceBookmarklet::removeObserver(const String& topic, Observer* observer)
{
    if (observerServiceBookmarkletIsDeleted)
        return;
    Vector<ObserverBookmarklet*> listObserver = m_topicData.get(topic);
    if (!listObserver.size())
        return;

    // If listObserver has one observer or less, topic can be safely removed from Hashmap
    // else just remove observer from list and set Hashmap with new listObserver
    if (listObserver.size() <= 1) {
        m_topicData.remove(topic);
    } else {
        int i = 0;
        Vector<ObserverBookmarklet*>::iterator it;
        for (it = listObserver.begin(); it != listObserver.end(); it++, i++) {
            if (*it == observer) {
                listObserver.remove(i);
                m_topicData.set(topic, listObserver);
                break;
            }
        }
    }
}

}
