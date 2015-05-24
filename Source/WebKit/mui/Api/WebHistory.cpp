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

 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringView.h>
#include "WebHistory.h"

#include "WebHistoryItem.h"
#include "WebPreferences.h"
#include "WebVisitedLinkStore.h"
#include "SQLiteDatabase.h"
#include "SQLiteStatement.h"
#include <wtf/CurrentTime.h>
#include "wtf/Vector.h"
#include "PageGroup.h"
#include "HistoryItem.h"
#include "IconDatabase.h"

#include "gui.h"
#include <clib/debug_protos.h>
#undef String
#undef PageGroup

using namespace WebCore;
using namespace std;

#define HISTORYDB "PROGDIR:Conf/History.db"
static SQLiteDatabase m_historyDB;

static std::vector<WebHistoryItem *> m_historyList;

std::vector<WebHistoryItem *> *WebHistory::historyList()
{
	return &m_historyList;
}

bool WebHistory::loadHistoryFromDatabase(int sortCriterium, bool desc, std::vector<WebHistoryItem *> *destList)
{
	unsigned int maxItems =  historyItemLimit();
	unsigned int maxAge = historyAgeInDaysLimit();

	if(!m_historyDB.isOpen() && !m_historyDB.open(HISTORYDB))
	{
		LOG_ERROR("Cannot open the history database");
		return false;
    }

    // Check that the database is correctly initialized.
	if(!m_historyDB.tableExists(String("history")))
	{
		m_historyDB.close();
		return false;
    }

	/* Prune older entries */
	double minAge = currentTime() - maxAge*24*3600;
	SQLiteStatement deleteStmt(m_historyDB, String("DELETE FROM history WHERE lastAccessed < ?1;"));

	if(deleteStmt.prepare())
	{
		return false;
    }

    // Binds all the values
	if(deleteStmt.bindDouble(1, minAge))
	{
		LOG_ERROR("Cannot save history");
		//return false;
    }

	if(!deleteStmt.executeCommand()) {
		LOG_ERROR("Cannot save history");
		//return false;
    }

	/* Remove extranumerous entries */
	char deleteStmt2buf[512];
	snprintf(deleteStmt2buf, sizeof(deleteStmt2buf), "DELETE FROM history WHERE lastAccessed < (SELECT lastAccessed FROM history ORDER BY lastAccessed ASC LIMIT 1 OFFSET (SELECT count(*) FROM history) - min(%d, (SELECT count(*) FROM history)));", maxItems);
	m_historyDB.executeCommand(String(deleteStmt2buf));

	String request = "SELECT url, title, lastAccessed FROM history";
	String end = ";";
	String direction;

	if(desc)
		direction = " DESC";
	else
		direction = " ASC";

	switch(sortCriterium)
	{
		default:
		case HISTORY_SORT_NONE:
			break;

		case HISTORY_SORT_BY_URL:
		        request.append(" ORDER BY length(url), url");
			request.append(direction);
			break;

		case HISTORY_SORT_BY_TITLE:
		        request.append(" ORDER BY title");
			request.append(direction);
			break;

		case HISTORY_SORT_BY_ACCESSTIME:
		        request.append(" ORDER BY lastAccessed");
			request.append(direction);
			break;
	}

	request.append(end);

    // Problem if the database is not connected
	SQLiteStatement select(m_historyDB, request);

	if(select.prepare())
	{
		LOG_ERROR("Cannot retrieve history in the database");
		return false;
    }

	while(select.step() == SQLITE_ROW)
	{
        // There is a row to fetch
		WebHistoryItem *item = WebHistoryItem::createInstance();

		if(item)
		{
			item->initWithURLString(select.getColumnText(0),
									select.getColumnText(1),
									select.getColumnDouble(2));

			char *title = (char *) item->title();
			free(title);

			if(!destList)
			{
				destList = &m_historyList;
			}

			destList->push_back(item);

			// Retain it for icondatabase
			iconDatabase().retainIconForPageURL(select.getColumnText(0));
		}
	}

	return true;
}

bool WebHistory::insertHistoryItemIntoDatabase(String& url, String& title, double lastAccessed)
{
	if(!m_historyDB.isOpen() && !m_historyDB.open(HISTORYDB))
	{
		LOG_ERROR("Cannot open the history database");
		return false;
    }

	if(!m_historyDB.tableExists(String("history")))
	{
		m_historyDB.executeCommand(String("CREATE TABLE history (url TEXT, title TEXT, lastAccessed DOUBLE);"));
	}

	SQLiteStatement deleteStmt(m_historyDB, String("DELETE FROM history WHERE url=?1;"));

	if(deleteStmt.prepare())
	{
		return false;
    }

    // Binds all the values
	if(deleteStmt.bindText(1, url))
	{
		LOG_ERROR("Cannot save history");
    }

	if(!deleteStmt.executeCommand()) {
		LOG_ERROR("Cannot save history");
    }

	SQLiteStatement insert(m_historyDB, String("INSERT INTO history (url, title, lastAccessed) VALUES (?1, ?2, ?3);"));

	if(insert.prepare())
	{
		return false;
    }

    // Binds all the values
	if(insert.bindText(1, url) || insert.bindText(2, title) || insert.bindDouble(3, lastAccessed))
	{
		LOG_ERROR("Cannot save history");
		return false;
    }

	if(!insert.executeCommand()) {
		LOG_ERROR("Cannot save history");
		return false;
    }

	return true;
}

WebHistory::WebHistory()
: m_preferences(0)
{
    m_preferences = WebPreferences::sharedStandardPreferences();

	WebVisitedLinkStore::setShouldTrackVisitedLinks(true);

	loadHistoryFromDatabase(HISTORY_SORT_BY_ACCESSTIME, false);
}

WebHistory::~WebHistory()
{
	m_historyDB.close();

/*
	for(unsigned int i = 0; i < m_historyList.size(); i++)
	{
		delete m_historyList[i];
		m_historyList[i] = NULL;
	}
*/
	m_preferences = 0;
}

WebHistory* WebHistory::createInstance()
{
    WebHistory* instance = new WebHistory();
    return instance;
}

WebHistory* WebHistory::sharedHistory()
{
    static WebHistory sharedHistory = *WebHistory::createInstance();
    return &sharedHistory;
}

WebHistory* WebHistory::optionalSharedHistory()
{
    return sharedHistory();
}

void WebHistory::setOptionalSharedHistory(WebHistory* history)
{
    if (sharedHistory() == history)
        return;
    *sharedHistory() = *history;
    WebVisitedLinkStore::setShouldTrackVisitedLinks(sharedHistory());
    WebVisitedLinkStore::removeAllVisitedLinks();
}

WebError* WebHistory::loadFromURL(const char* url)
{
    return 0;
}


WebError* WebHistory::saveToURL(const char* url)
{
    return 0;
}



void WebHistory::addItems(vector<WebHistoryItem*> items)
{
}

void WebHistory::removeItems(vector<WebHistoryItem*> items)
{
}

void WebHistory::removeAllItems()
{
    WebVisitedLinkStore::removeAllVisitedLinks();
}

WebHistoryItem* WebHistory::itemForURL(const char* url)
{
	//kprintf("WebHistory::itemForURL(%s)\n", url.latin1().data());

	for(unsigned int i=0; i < m_historyList.size(); i++)
	{
		char *wurl = (char *) m_historyList[i]->URLString();
		WTF::String itemURL = wurl;
		free(wurl);

		if(itemURL == url)
		{

			return m_historyList[i];
		}
	}

	free((char *)url);

    return 0;
}

void WebHistory::addVisitedLinksToVisitedLinkStore(WebVisitedLinkStore& visitedLinkStore)
{
    for (auto * url : m_historyList)
        visitedLinkStore.addVisitedLink(url->URLString());
}

void WebHistory::setVisitedLinkTrackingEnabled(bool visitedLinkTrackingEnabled)
{
    WebVisitedLinkStore::setShouldTrackVisitedLinks(visitedLinkTrackingEnabled);
}

void WebHistory::removeAllVisitedLinks()
{
    WebVisitedLinkStore::removeAllVisitedLinks();
}

void WebHistory::setHistoryItemLimit(int limit)
{
    if (!m_preferences)
        return;
    m_preferences->setHistoryItemLimit(limit);
}

int WebHistory::historyItemLimit()
{
    if (!m_preferences)
        return 0;
    return m_preferences->historyItemLimit();
}

void WebHistory::setHistoryAgeInDaysLimit(int limit)
{
    if (!m_preferences)
        return ;
    m_preferences->setHistoryAgeInDaysLimit(limit);
}

int WebHistory::historyAgeInDaysLimit()
{
    if (!m_preferences)
        return 0;
    return m_preferences->historyAgeInDaysLimit();
}


void WebHistory::visitedURL(const char* url, const char* title, const char* httpMethod, bool wasFailure)
{
	double lastAccessed = currentTime();

	// Don't allow duplicates
	for(std::vector<WebHistoryItem *>::iterator it = m_historyList.begin(); it != m_historyList.end(); it++)
	{
		WebHistoryItem *item = *(it);
		char *wurl = (char *) item->URLString();
		WTF::String itemURL = wurl;
		free(wurl);

		if(itemURL == url)
		{
			m_historyList.erase(it, it+1);
			DoMethod(app, MM_History_Remove, item);
			delete item; // Careful with that one
			break;
		}
	}

	WebHistoryItem *item = WebHistoryItem::createInstance();

	if(item)
	{
		item->initWithURLString(String::fromUTF8(url), String::fromUTF8(title), lastAccessed);
		m_historyList.push_back(item);

		DoMethod(app, MM_History_Insert, item);

		if(getv(app, MA_OWBApp_SaveHistory))
		{
			char *wurl   = (char *) item->URLString();
			char *wtitle = (char *) item->title();

			String u = String::fromUTF8(wurl);
			String t = String::fromUTF8(wtitle);

			insertHistoryItemIntoDatabase(u, t, lastAccessed);
			free(wurl);
			free(wtitle);
		}

	}

	free((char *)url);
	free((char *)title);
	free((char *)httpMethod);
}

WebHistoryItem* WebHistory::itemForURLString(const char* urlString) const
{
	//kprintf("WebHistory::itemForURLString(%s)\n", urlString.latin1().data());
	free((char *)urlString);
    return 0;
}

std::vector<WebHistoryItem*> WebHistory::allItems()
{
    vector<WebHistoryItem*> items;
    return items;
}
