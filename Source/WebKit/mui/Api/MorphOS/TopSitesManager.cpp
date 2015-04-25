/*
 * Copyright (C) 2013 Fabien Coeurjoly. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "TopSitesManager.h"

#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>
#include <wtf/CurrentTime.h>
#include "BitmapImage.h"
#include "FileIOLinux.h"
#include "URL.h"
#include "SharedBuffer.h"
#include "SQLiteDatabase.h"
#include "SQLiteStatement.h"
#include "WebHistory.h"
#include "WebHistoryItem.h"
#include "WebPreferences.h"
#include "WebView.h"
#include "WebFrame.h"
#include <wtf/text/WTFString.h>

#include "gui.h"
#include <clib/debug_protos.h>

/*

Ideas:
- filter webinspector
- placeholder for empty pictures
- in template generation, maybe just instanciate js objects. Then it can be scripted nicely at web level
In theory, it could also be a history per date/most visited view

*/

namespace WebCore {

#define TOPSITESDB "PROGDIR:Conf/TopSites.db"
#define TOPSITES_TEMPLATE_COVERFLOW_PATH "PROGDIR:Resource/TopSites_CoverFlow.html"
#define TOPSITES_TEMPLATE_GRID_PATH "PROGDIR:Resource/TopSites_Grid.html"
#define SCREENSHOT_WIDTH 640
#define MAXENTRIES 9
#define HOUR 60*60
#define DAY 24*HOUR
#define SCREENSHOT_UPDATE_DELAY 1*HOUR

static SQLiteDatabase m_topSitesDB;
static bool m_disableScreenShots = getenv("OWB_DISABLE_TOPSITES_SCREENSHOTS");
static bool m_disableTopSites = getenv("OWB_DISABLE_TOPSITES");

TopSitesManager& TopSitesManager::getInstance()
{
	static TopSitesManager instance;
    return instance;
}

TopSitesManager::TopSitesManager()
	: m_maxEntries(MAXENTRIES)
	, m_filterMode(SHOW_ALL)
	, m_displayMode(TEMPLATE_GRID)	
	, m_screenshotSize(SCREENSHOT_WIDTH)
{ 
	m_topSitesDB.open(TOPSITESDB);
	
	if(m_topSitesDB.isOpen())
	{
		if(!m_topSitesDB.tableExists("topsites"))
		{
			m_topSitesDB.executeCommand("CREATE TABLE topsites (url TEXT, title TEXT, screenshot BLOB, visitCount INTEGER, lastAccessed DOUBLE);");	
		}
		
		if(!m_topSitesDB.tableExists("settings"))
		{
			m_topSitesDB.executeCommand("CREATE TABLE settings (displayMode INTEGER, maxEntries INTEGER, screenshotSize INTEGER, filterMode INTEGER);");
			
			SQLiteStatement insertStmt(m_topSitesDB, "INSERT INTO settings (displayMode, maxEntries, screenshotSize, filterMode) VALUES (?1, ?2, ?3, ?4);");

			if(insertStmt.prepare())
				return;

			insertStmt.bindInt(1, m_displayMode);
			insertStmt.bindInt(2, m_maxEntries);	
			insertStmt.bindInt(3, m_screenshotSize);
			insertStmt.bindInt(4, m_filterMode);

			if(!insertStmt.executeCommand())
				return;
		}			
		else
		{
			SQLiteStatement select(m_topSitesDB, "SELECT displayMode INTEGER, maxEntries INTEGER, screenshotSize INTEGER, filterMode INTEGER FROM settings;");
			if(select.prepare())
				return;

			if(select.step() == SQLITE_ROW)
			{
				m_displayMode = (displaymode_t) select.getColumnInt(0);
				m_maxEntries  = select.getColumnInt(1);
				m_screenshotSize = select.getColumnInt(2);
				m_filterMode = (filtermode_t) select.getColumnInt(3);
			}
		}
		
		pruneOlderEntries();
	}
	
	m_placeholderImage = SharedBuffer::createWithContentsOfFile("PROGDIR:resource/TopSites_Default.png");
	m_closeImage = SharedBuffer::createWithContentsOfFile("PROGDIR:resource/close.png");
}

TopSitesManager::~TopSitesManager()
{
	if(m_topSitesDB.isOpen())
		m_topSitesDB.close();
}

void TopSitesManager::setDisplayMode(displaymode_t mode) 
{ 
	m_displayMode = mode; 

	SQLiteStatement updateStmt(m_topSitesDB, "UPDATE settings SET displayMode=?1;");

	if(updateStmt.prepare())
	{
		return;
	}

	updateStmt.bindInt(1, m_displayMode);

	if(!updateStmt.executeCommand())
		return;
}

void TopSitesManager::setFilterMode(filtermode_t mode) 
{ 
	m_filterMode = mode; 

	SQLiteStatement updateStmt(m_topSitesDB, "UPDATE settings SET filterMode=?1;");

	if(updateStmt.prepare())
	{
		return;
	}

	updateStmt.bindInt(1, m_filterMode);

	if(!updateStmt.executeCommand())
		return;
}

void TopSitesManager::setMaxEntries(int maxEntries) 
{ 
	m_maxEntries = maxEntries; 

	SQLiteStatement updateStmt(m_topSitesDB, "UPDATE settings SET maxEntries=?1;");

	if(updateStmt.prepare())
	{
		return;
	}

	updateStmt.bindInt(1, m_maxEntries);

	if(!updateStmt.executeCommand())
		return;
}

void TopSitesManager::setScreenshotSize(int width)
{
	m_screenshotSize = width;

	SQLiteStatement updateStmt(m_topSitesDB, "UPDATE settings SET screenshotSize=?1;");

	if(updateStmt.prepare())
	{
		return;
	}

	updateStmt.bindInt(1, m_screenshotSize);

	if(!updateStmt.executeCommand())
		return;
}

int TopSitesManager::entries()
{
	SQLiteStatement select(m_topSitesDB, "SELECT count(*) FROM topsites;");

	if(select.prepare())
		return 0;
		
	if(select.step() == SQLITE_ROW)
	{
		return select.getColumnInt(0);
	}

	return 0;
}

String TopSitesManager::title(URL &url)
{
	String title;

	SQLiteStatement select(m_topSitesDB, "SELECT title FROM topsites WHERE url=?1;");
	if(select.prepare())
		return title;

	select.bindText(1, url.string());
		
	if(select.step() == SQLITE_ROW)
	{
		title = select.getColumnText(0);
	}	
	
	return title;
}

PassRefPtr<Image> TopSitesManager::screenshot(URL &url)
{
	RefPtr<Image> image;
	
	SQLiteStatement select(m_topSitesDB, "SELECT screenshot FROM topsites WHERE url=?1;");

	if(select.prepare())
		return image.release();
	
	select.bindText(1, url.string());
	
    if (select.step() == SQLITE_ROW)
    {
        Vector<char> data;
        RefPtr<SharedBuffer> imageData;
        
        select.getColumnBlobAsVector(0, data);
        
        imageData = SharedBuffer::create(data.data(), data.size());
        
		image = BitmapImage::create();                                                            
		image->setData(imageData, true);
    }
		
	return image.release();
}

double TopSitesManager::lastAccessed(URL &url)
{
	double lastAccessed = 0;
	
	SQLiteStatement select(m_topSitesDB, "SELECT lastAccessed FROM topsites WHERE url=?1;");

	if(select.prepare())
		return lastAccessed;

	select.bindText(1, url.string());
		
	if(select.step() == SQLITE_ROW)
	{
		lastAccessed = select.getColumnDouble(0);
	}	
	
	return lastAccessed;
}

int TopSitesManager::visitCount(URL &url)
{
	int visitCount = 0;
	
	SQLiteStatement select(m_topSitesDB, "SELECT visitCount FROM topsites WHERE url=?1;");

	if(select.prepare())
		return visitCount;

	select.bindText(1, url.string());
		
	if(select.step() == SQLITE_ROW)
	{
		visitCount = select.getColumnInt(0);
	}	
	
	return visitCount;
}

bool TopSitesManager::contains(URL &url)
{
	bool found = false;
	
	SQLiteStatement select(m_topSitesDB, "SELECT count(*) FROM topsites WHERE url=?1;");

	if(select.prepare())
		return found;

	select.bindText(1, url.string());
		
	if(select.step() == SQLITE_ROW)
	{
		found = select.getColumnInt(0) > 0;
	}	
	
	return found;	
}

bool TopSitesManager::hasScreenshot(URL &url)
{
	bool hasScreenshot = false;
	SQLiteStatement select(m_topSitesDB, "SELECT screenshot FROM topsites WHERE url=?1;");

	if(select.prepare())
		return hasScreenshot;

	select.bindText(1, url.string());
	
    if (select.step() == SQLITE_ROW)
    {
		hasScreenshot = select.isColumnNull(0);
    }
    
    return hasScreenshot;
}

bool TopSitesManager::shouldAppear(URL &url)
{
	bool result = false;
	int minVisitCount = 0;
	
	SQLiteStatement select(m_topSitesDB, "SELECT MIN(visitCount) FROM topsites ORDER BY visitCount DESC LIMIT 0, ?1;");

	if(select.prepare())
		return result;

	select.bindInt(1, maxEntries());
		
	if(select.step() == SQLITE_ROW)
	{
		minVisitCount = select.getColumnInt(0);
	}	
	
	result = visitCount(url) >= minVisitCount;
	
	return result;
}

int TopSitesManager::requiredVisitCount()
{
	int minVisitCount = 0;
	
	SQLiteStatement select(m_topSitesDB, "SELECT MIN(visitCount) FROM(SELECT visitCount FROM topsites ORDER BY visitCount DESC LIMIT 0, ?1);");

	if(select.prepare())
		return minVisitCount;

	select.bindInt(1, maxEntries());
		
	if(select.step() == SQLITE_ROW)
	{
		minVisitCount = select.getColumnInt(0);
	}	
	
	//kprintf("requiredVisitCount %d\n", minVisitCount);
	//kprintf("entries < maxEntries %d\n",entries() < maxEntries());
	
	return (entries() < maxEntries()) ? 1 : minVisitCount;
}

void TopSitesManager::update(WebView *webView, URL &url, String &title)
{
	if(m_disableTopSites)
		return;

	if(url.string().startsWith("topsites:"))
	{
		size_t index = url.string().find('?');
		if(index != notFound)
		{
			String query = url.string().substring(index + 1);
			if(!query.isEmpty())
			{
				//kprintf("query : <%s>\n", query.utf8().data());

				if(query.startsWith("remove="))
				{
					String argument = query.substring(String("remove=").length());
					URL urlToRemove = URL(ParsedURLString, argument);
					remove(urlToRemove);
				}
				else if(query.startsWith("maxEntries="))
				{
					String argument = query.substring(String("maxEntries=").length());
					setMaxEntries(argument.toInt());
				}
				else if(query.startsWith("displayMode="))
				{
					String argument = query.substring(String("displayMode=").length());
					setDisplayMode((displaymode_t) argument.toInt());
				}
				else if(query.startsWith("filterMode="))
				{
					String argument = query.substring(String("filterMode=").length());
					setFilterMode((filtermode_t) argument.toInt());
				}							
				else if(query.startsWith("screenshotSize="))
				{
					String argument = query.substring(String("screenshotSize=").length());
					setScreenshotSize(argument.toInt());
				}
				
				generateTemplate(webView, "topsites://");
			}
		}
	}

	if(shouldAdd(url))
	{
		addOrUpdate(webView, url, title);
	}
}

bool TopSitesManager::shouldAdd(URL &url)
{	
	if(url.string().startsWith("topsites:"))
		return false;

	if(url.string().endsWith("webinspector/inspector.html"))
		return false;

	if(url.protocolIs("data:"))
		return false;

	return true;
}


void TopSitesManager::pruneOlderEntries()
{
	SQLiteStatement updateStmt(m_topSitesDB, "UPDATE topsites SET screenshot = NULL WHERE visitCount <= ?1;");

	if(updateStmt.prepare())
	{
		return;
	}

	updateStmt.bindInt(1, requiredVisitCount() - 1); // Improve this

	if(!updateStmt.executeCommand())
		return;	
		
	double maxAge = WebPreferences::sharedStandardPreferences()->historyAgeInDaysLimit();
	double minAge = currentTime() - maxAge*DAY;
	SQLiteStatement deleteStmt(m_topSitesDB, "DELETE FROM topsites WHERE lastAccessed < ?1;");

	if(deleteStmt.prepare())
	{
		return;
	}

	deleteStmt.bindDouble(1, minAge);
	
	deleteStmt.executeCommand();		
}

bool TopSitesManager::addOrUpdate(WebView *webView, URL &url, String &title)
{	
	double timestamp = currentTime();
	int visitCount = 1;
	double lastAccessed = 0;
	Vector<char> screenshot;
	bool found = false;

	//kprintf("addOrUpdate <%s>\n", url.string().utf8().data());

	SQLiteStatement select(m_topSitesDB, "SELECT visitCount, screenshot, lastAccessed FROM topsites WHERE url=?1;");
	if(select.prepare())
		return false;

	select.bindText(1, url.string());
		
	if(select.step() == SQLITE_ROW)
	{
		found = true;
		visitCount = select.getColumnInt(0) + 1;
		select.getColumnBlobAsVector(1, screenshot);
		lastAccessed = select.getColumnDouble(2);		
	}	

	//kprintf("visitCount %d required : %d screenshot %d timestamp %f lastaccessed %d\n", visitCount, requiredVisitCount(), screenshot.size(), timestamp, lastAccessed);
	bool generateScreenshot = !m_disableScreenShots && (visitCount >= requiredVisitCount()) && (screenshot.size() == 0 || (timestamp >= lastAccessed + SCREENSHOT_UPDATE_DELAY));

	if(found)
	{	
		SQLiteStatement updateStmt(m_topSitesDB, "UPDATE topsites SET title=?1, visitCount=?2, lastAccessed=?3 WHERE url=?4;");

		if(updateStmt.prepare())
		{
			return false;
	    }

		updateStmt.bindText(1, title);
		updateStmt.bindInt(2, visitCount);
		updateStmt.bindDouble(3, timestamp);
		updateStmt.bindText(4, url.string());

		if(!updateStmt.executeCommand())
			return false;				
	}	
	else
	{		
		SQLiteStatement insertStmt(m_topSitesDB, "INSERT INTO topsites (url, title, screenshot, visitCount, lastAccessed) VALUES (?1, ?2, ?3, ?4, ?5);");

		if(insertStmt.prepare())
			return false;

		insertStmt.bindText(1, url.string());
		insertStmt.bindText(2, title);	
		insertStmt.bindNull(3);	
		insertStmt.bindInt(4, visitCount);		
		insertStmt.bindDouble(5, timestamp);

		if(!insertStmt.executeCommand())
			return false;			
	}	
	
	if(generateScreenshot)
	{
		int width = m_screenshotSize;
		int height;
		Vector<char> imageData;

		//kprintf("Generate screenshot for <%s>\n", url.string().utf8().data());
		webView->screenshot(width, height, &imageData);
		
		SQLiteStatement updateScreenShotStmt(m_topSitesDB, "UPDATE topsites SET screenshot=?1 WHERE url=?2;");

		if(updateScreenShotStmt.prepare())
		{
			return false;
		}

		updateScreenShotStmt.bindBlob(1, imageData.data(), imageData.size());			
		updateScreenShotStmt.bindText(2, url.string());		

		if(!updateScreenShotStmt.executeCommand())
			return false;
	}		
	
	return true;
}

void TopSitesManager::remove(URL &url)
{
	SQLiteStatement deleteStmt(m_topSitesDB, "DELETE FROM topsites WHERE url=?1;");

	if(deleteStmt.prepare())
	{
		return;
	}

	deleteStmt.bindText(1, url.string());
	
	deleteStmt.executeCommand();
}

void TopSitesManager::generateTemplate(WebView *webView, String originurl)
{
	String templateData;
	String templatePath;

	if(m_disableTopSites)
		return;

	switch(m_displayMode)
	{
		case TEMPLATE_3D:
		{
			templatePath = TOPSITES_TEMPLATE_COVERFLOW_PATH;
			break;
		}

		case TEMPLATE_GRID:
		{
			templatePath = TOPSITES_TEMPLATE_GRID_PATH;
			break;
		}
	}
	
	OWBFile f(templatePath);
	
	if(f.open('r') != -1)
	{
		char *fileContents = f.read(f.getSize());

		if(fileContents)
		{
			templateData = String(fileContents);
			delete [] fileContents;
		}

		f.close();
	}	

	if(!templateData.isEmpty())
	{
		String contents;
		int countEntries = 0;

		SQLiteStatement select(m_topSitesDB, "SELECT url, title, screenshot, lastAccessed, visitCount FROM topsites ORDER by visitCount DESC, lastAccessed DESC;");

		if(select.prepare())
			return;
			
		while(select.step() == SQLITE_ROW && countEntries < maxEntries())
		{
			bool showEntry = false;
			String url = select.getColumnText(0);
						
			switch(m_filterMode)
			{
				case SHOW_ALL_BUT_BOOKMARKS:
				{
					int isBookmark = DoMethod((Object *) getv(app, MA_OWBApp_BookmarkWindow), MM_Bookmarkgroup_ContainsURL, url.utf8().data());
					showEntry = !isBookmark;
					break;
				}
					
				case SHOW_BOOKMARKS:
				{
					int isBookmark = DoMethod((Object *) getv(app, MA_OWBApp_BookmarkWindow), MM_Bookmarkgroup_ContainsURL, url.utf8().data());
					showEntry = isBookmark;
					break;
				}
					
				case SHOW_ALL:
				{
					showEntry = true;
					break;
				}

				case SHOW_CUSTOM:
				{
					showEntry = true;
					// user-defined ones only
					break;
				}				
			}
			
			if(showEntry)
			{
				Vector<char> data, base64Data, closeBase64Data;
				String title = select.getColumnText(1);    
				select.getColumnBlobAsVector(2, data);
				
				if(data.size() == 0 && m_placeholderImage && m_placeholderImage->data())
				{
					base64Encode(m_placeholderImage->data(), m_placeholderImage->size(), base64Data);
				}
				else
				{
					base64Encode(data, base64Data);
				}
				
				if(m_closeImage && m_closeImage->data())
				{
					base64Encode(m_closeImage->data(), m_closeImage->size(), closeBase64Data);
				}

				String screenshotURL =  "data:image/png;base64," + base64Data;
				String removeURL = "data:image/png;base64," + closeBase64Data;
				//double timestamp = select.getColumnDouble(3);
				int visits = select.getColumnInt(4);

				countEntries++;

				switch(m_displayMode)
				{
					case TEMPLATE_3D:		
					{		
						contents.append("<div class=\"item\" href=\"" + url + "\">");
						
						contents.append("<img class=\"content\" title=\"" + title  + "\" src=\"" + screenshotURL + "\" alt=\"" + title  + "\" visitcount=\"" + String::number(visits) + "\">");

						contents.append("<div class=\"caption\">");
						contents.append("<h4><a href=\"topsites://?remove=" + url + "\"><img src=\"" + removeURL + "\"></a>&nbsp;<a href=\"" + url + "\">" + title + "</h4></a>");
						contents.append("</div>");

						contents.append("</div>");
					
						break;
					}
						
					case TEMPLATE_GRID:
					{
						contents.append("<li class=\"clearfix\">");
						contents.append("<a href=\"" + url + "\"><img src=\"" + screenshotURL + "\" visitcount=\"" + String::number(visits) + "\"></a>");
						
						contents.append("<div class=\"meta\">");
						contents.append("<h4>" + title + "</h4>");
						contents.append("</div>");

						contents.append("<div class=\"remove\">");
						contents.append("<span><a href=\"topsites://?remove=" + url + "\"><img src=\"" + removeURL + "\"></a></span>");
						contents.append("</div>");
						
						contents.append("</li>");
						
						break;
					}
				}		
			}
		}
		
		if(contents.isEmpty())
		{
			contents = " ";
		}
		
		templateData.replace("{{topsites}}", contents);
		templateData.replace("{{maxEntries}}", String::number(m_maxEntries));
		templateData.replace("{{displayMode}}", String::number(m_displayMode));
		templateData.replace("{{filterMode}}", String::number(m_filterMode));
		templateData.replace("{{screenshotSize}}", String::number(m_screenshotSize));

		webView->mainFrame()->loadHTMLString(templateData.utf8().data(), originurl.utf8().data());
	}
}

} // namespace WebCore
