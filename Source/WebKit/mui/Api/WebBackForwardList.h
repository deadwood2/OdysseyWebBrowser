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

#ifndef WebBackForwardList_H
#define WebBackForwardList_H

#include "WebKitTypes.h"

/**
 *  @file  WebBackForwardList.h
 *  WebBackForwardList description
 *  Repository informations :
 * - $URL$
 * - $Rev$
 * - $Date: 2012/05/04 21:14:06 $
 */

class WebBackForwardListPrivate;
class WebHistoryItem;

class WebBackForwardList
{
public:
    /**
     * create new instance of WebBackForwardList
     * @param[in]: BackForwardList
     * @param[out]: WebBackForwardList
     * @code
     * WebBackForwardList *wbfl = WebBackForwardList::createInstance(bfl);
     * @endcode
     */
    static WebBackForwardList* createInstance(WebBackForwardListPrivate*);

protected:
    /**
     * WebBackForwardList constructor
     * @param[in]: BackForwardList
     */
    WebBackForwardList(WebBackForwardListPrivate*);

public:

    /**
     * WebBackForwardList destructor
     */
    virtual ~WebBackForwardList();

	//virtual void clear();
    /**
     * @param entry The entry to add.
        @discussion The added entry is inserted immediately after the current entry.
        If the current position in the list is not at the end of the list, elements in the
        forward list will be dropped at this point.  In addition, entries may be dropped to keep
        the size of the list within the maximum size addItem description
     */
    virtual void addItem(WebHistoryItem *item);

    /**
     * Move the current pointer back to the entry before the current entry.
     */
    virtual void goBack();

    /**
     * Move the current pointer ahead to the entry after the current entry
     */
    virtual void goForward();

    /**
     * Move the current pointer to the given entry.
       @param item The history item to move the pointer to
     */
    virtual void goToItem(WebHistoryItem *item);

    /**
     * Returns the entry right before the current entry.
       @result The entry right before the current entry, or nil if there isn't one.
     */
    virtual WebHistoryItem *backItem();

    /**
     * Returns the entry right after the current entry.
       @result The entry right after the current entry, or nil if there isn't one.
     */
    virtual WebHistoryItem *currentItem();

    /**
     * Returns a portion of the list before the current entry.
        @param limit A cap on the size of the array returned.
        @result An array of items before the current entry, or nil if there are none.  The entries are in the order that they were originally visited.
     */
    virtual WebHistoryItem *forwardItem();

    /**
     * Returns a portion of the list before the current entry.
        @param limit A cap on the size of the array returned.
        @result An array of items before the current entry, or nil if there are none.  The entries are in the order that they were originally visited.
     */
    virtual int backListWithLimit(int limit, WebHistoryItem **list);

    /**
     * Returns a portion of the list after the current entry.
        @param limit A cap on the size of the array returned.
        @result An array of items after the current entry, or nil if there are none.  The entries are in the order that they were originally visited.
     */
    virtual int forwardListWithLimit(int limit, WebHistoryItem **list);

    /**
     * Returns the list's maximum size.
        @result The list's maximum size.
     */
    virtual int capacity();

    /**
     * Sets the list's maximum size.
        @param size The new maximum size for the list.
     */
    virtual void setCapacity(int size);

    /**
     * Returns the back list's current count.
        @result The number of items in the list.
     */
    virtual int backListCount();

    /**
     * Returns the forward list's current count.
        @result The number of items in the list.
     */
    virtual int forwardListCount();

    /**
     * item The item that will be checked for presence in the WebBackForwardList.
        @result Returns YES if the item is in the list.
     */
    virtual bool containsItem(WebHistoryItem *item);

    /**
     * Returns an entry the given distance from the current entry.
        @param index Index of the desired list item relative to the current item; 0 is current item, -1 is back item, 1 is forward item, etc.
        @result The entry the given distance from the current entry. If index exceeds the limits of the list, nil is returned.
     */
    virtual WebHistoryItem *itemAtIndex(int index);


    /**
     * remove the item
     */
    virtual void removeItem(WebHistoryItem* item);

protected:
    WebBackForwardListPrivate *d;
};

#endif
