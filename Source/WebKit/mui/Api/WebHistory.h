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

#ifndef WebHistory_H
#define WebHistory_H

#include "WebKitTypes.h"
#include "SQLiteDatabase.h"
#include <vector>

namespace WebCore {
    class PageGroup;
}

class WebPreferences;
class WebHistoryItem;
class WebError;
class WebVisitedLinkStore;

enum
{
	HISTORY_SORT_NONE,
	HISTORY_SORT_BY_URL,
	HISTORY_SORT_BY_TITLE,
	HISTORY_SORT_BY_ACCESSTIME,
};

class WebHistory {

public:
    /**
     * createInstance
     * @brief creates an Instance of WebHistory
     * @return WebHistory
     */
    static WebHistory* createInstance();

private:

    /**
     * WebHistory construtor
     */
    WebHistory();

public:

    /**
     * ~WebHistory destructor
     */
    virtual ~WebHistory();

    /**
     * optionalSharedHistory
     * @brief Returns a shared WebHistory instance initialized with the default history file.
     * @result A WebHistory object.
     */
    virtual WebHistory* optionalSharedHistory();

    virtual void setOptionalSharedHistory(WebHistory* history);

    /**
     * @brief The designated initializer for WebHistory.
     * @param url The URL to use to initialize the WebHistory.
     * @result Returns 0 if sucessfully or a WebError containing the error.
     */
    virtual WebError* loadFromURL(const char* url);

    /**
     *  @brief Save history to URL. It is the client's responsibility to call this at appropriate times.
     *  @param url The URL to use to save the WebHistory.
     *  @result Returns 0 if everything was alright, a point to a WebError if something failed.
     */
    virtual WebError* saveToURL(const char* url);

    /**
     * @brief add some WebHistoryItem to the history.
     * @param items An array of WebHistoryItems to add to the WebHistory.
     */
    virtual void addItems(std::vector<WebHistoryItem*> items);

    /**
     *  @brief remove some WebHistoryItem to the history.
     *  @param items An array of WebHistoryItems to remove from the WebHistory.
     */
    virtual void removeItems(std::vector<WebHistoryItem*> items);

    /**
     * @brief remove all the history items.
     */
    virtual void removeAllItems();

    /**
     *  @brief Get an item for a specific URL
     *  @param url The URL of the history item to search for
     *  @result Returns an item matching the URL. If none is found, 0 is returned.
     */
    virtual WebHistoryItem* itemForURL(const char* url);

    /**
     * @brief Limits the number of items that will be stored by the WebHistory.
     * @param limit The maximum number of items that will be stored by the WebHistory.
     */
    virtual void setHistoryItemLimit(int limit);

    /**
     * @brief returns the current limit.
     */
    virtual int historyItemLimit();

    /**
     * @brief setVisitedLinkTrackingEnabled enable the link tracking code inside WebCore. if disabled, selector such as :visited would not work.
     * @param visitedLinkTrackingEnabled the new value.
     * @note: setting this to true is needed for the acid3 test (http://acid3.acidtests.org).
     */
    virtual void setVisitedLinkTrackingEnabled(bool visitedLinkTrackingEnabled);

    /**
     * removeAllVisitedLinks
     * @brief removeAllVisitedLinks remove the visited link cache. This will make links that were matched by the :visited selector to not be applied anymoe.
     */
    virtual void removeAllVisitedLinks();

    /**
     * setHistoryAgeInDaysLimit:
     * @brief sets the maximum number of days to be read from stored history.
     * @param limit The maximum number of days to be read from stored history.
     */
    virtual void setHistoryAgeInDaysLimit(int limit);

    /**
     * historyAgeInDaysLimit
     * @return Returns the maximum number of days to be read from stored history.
     */
    virtual int historyAgeInDaysLimit();


    /**
     * @brief return a reference to the shared WebHistory
     * @return the shared WebHistory*
     */
    static WebHistory* sharedHistory();

    /**
     * @brief add a visited URL to the history.
     * @param url the URL
     * @param title the page title
     * @param httpMethod the HTTP method used to get the page (GET, POST ...)
     * @param wasFailure whether the page load was a failure (true) or successfully (false).
     */
    void visitedURL(const char* url, const char* title, const char* httpMethod, bool wasFailure);

    void addVisitedLinksToVisitedLinkStore(WebVisitedLinkStore&);


    /**
     * @brief get the WebHistoryItem for a URL
     * @param url the URL
     * @return a WebHistoryItem, 0 if nothing is found.
     */
    WebHistoryItem* itemForURLString(const char* url) const;

    /**
     * @brief returns all the WebHistoryItem for the history
     * @return a std::vector containing the WebHistoryItem.
     */
    virtual std::vector<WebHistoryItem*> allItems();

protected:
    friend class WebChromeClient;

public:
	std::vector<WebHistoryItem *> *historyList();

private:
    enum NotificationType {
        kWebHistoryItemsAddedNotification = 0,
        kWebHistoryItemsRemovedNotification = 1,
        kWebHistoryAllItemsRemovedNotification = 2,
        kWebHistoryLoadedNotification = 3,
        kWebHistoryItemsDiscardedWhileLoadingNotification = 4,
        kWebHistorySavedNotification = 5
    };

    WebPreferences *m_preferences;

public:
	bool loadHistoryFromDatabase(int sortCriterium, bool desc = false, std::vector<WebHistoryItem *> *destList = NULL);
	bool insertHistoryItemIntoDatabase(WTF::String& url, WTF::String& title, double lastAccessed);
};

#endif
