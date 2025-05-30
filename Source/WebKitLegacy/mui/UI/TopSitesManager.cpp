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

#include "TopSitesManager.h"

#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>
#include <WebCore/BitmapImage.h>
#include "FileIOLinux.h"
#include <wtf/URL.h>
#include <WebCore/SharedBuffer.h>
#include <WebCore/SQLiteDatabase.h>
#include <WebCore/SQLiteStatement.h>
#include "WebHistory.h"
#include "WebHistoryItem.h"
#include "WebPreferences.h"
#include "WebView.h"
#include "WebFrame.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/StringToIntegerConversion.h>

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
            m_topSitesDB.executeCommand("CREATE TABLE topsites (url TEXT, title TEXT, screenshot BLOB, visitCount INTEGER, lastAccessed DOUBLE);"_s);    
        }
        
        if(!m_topSitesDB.tableExists("settings"))
        {
            m_topSitesDB.executeCommand("CREATE TABLE settings (displayMode INTEGER, maxEntries INTEGER, screenshotSize INTEGER, filterMode INTEGER);"_s);
            
            auto insertStmt = m_topSitesDB.prepareStatement("INSERT INTO settings (displayMode, maxEntries, screenshotSize, filterMode) VALUES (?1, ?2, ?3, ?4);"_s);

            if(!insertStmt)
                return;

            insertStmt->bindInt(1, m_displayMode);
            insertStmt->bindInt(2, m_maxEntries);    
            insertStmt->bindInt(3, m_screenshotSize);
            insertStmt->bindInt(4, m_filterMode);

            if(!insertStmt->executeCommand())
                return;
        }            
        else
        {
            auto select = m_topSitesDB.prepareStatement("SELECT displayMode INTEGER, maxEntries INTEGER, screenshotSize INTEGER, filterMode INTEGER FROM settings;"_s);
            if(!select)
                return;

            if(select->step() == SQLITE_ROW)
            {
                m_displayMode = (displaymode_t) select->columnInt(0);
                m_maxEntries  = select->columnInt(1);
                m_screenshotSize = select->columnInt(2);
                m_filterMode = (filtermode_t) select->columnInt(3);
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

    auto updateStmt = m_topSitesDB.prepareStatement("UPDATE settings SET displayMode=?1;"_s);

    if(!updateStmt)
    {
        return;
    }

    updateStmt->bindInt(1, m_displayMode);

    if(!updateStmt->executeCommand())
        return;
}

void TopSitesManager::setFilterMode(filtermode_t mode) 
{ 
    m_filterMode = mode; 

    auto updateStmt = m_topSitesDB.prepareStatement("UPDATE settings SET filterMode=?1;"_s);

    if(!updateStmt)
    {
        return;
    }

    updateStmt->bindInt(1, m_filterMode);

    if(!updateStmt->executeCommand())
        return;
}

void TopSitesManager::setMaxEntries(int maxEntries) 
{ 
    m_maxEntries = maxEntries; 

    auto updateStmt = m_topSitesDB.prepareStatement("UPDATE settings SET maxEntries=?1;"_s);

    if(!updateStmt)
    {
        return;
    }

    updateStmt->bindInt(1, m_maxEntries);

    if(!updateStmt->executeCommand())
        return;
}

void TopSitesManager::setScreenshotSize(int width)
{
    m_screenshotSize = width;

    auto updateStmt = m_topSitesDB.prepareStatement("UPDATE settings SET screenshotSize=?1;"_s);

    if(!updateStmt)
    {
        return;
    }

    updateStmt->bindInt(1, m_screenshotSize);

    if(!updateStmt->executeCommand())
        return;
}

int TopSitesManager::entries()
{
    auto select = m_topSitesDB.prepareStatement("SELECT count(*) FROM topsites;"_s);

    if(!select)
        return 0;
        
    if(select->step() == SQLITE_ROW)
    {
        return select->columnInt(0);
    }

    return 0;
}

String TopSitesManager::title(URL &url)
{
    String title;

    auto select = m_topSitesDB.prepareStatement("SELECT title FROM topsites WHERE url=?1;"_s);
    if(!select)
        return title;

    select->bindText(1, url.string());
        
    if(select->step() == SQLITE_ROW)
    {
        title = select->columnText(0);
    }    
    
    return title;
}

RefPtr<Image> TopSitesManager::screenshot(URL &url)
{
    RefPtr<Image> image;
    
    auto select = m_topSitesDB.prepareStatement("SELECT screenshot FROM topsites WHERE url=?1;"_s);

    if(!select)
        return image;
    
    select->bindText(1, url.string());
    
    if (select->step() == SQLITE_ROW)
    {
        Vector<uint8_t> data;
        RefPtr<SharedBuffer> imageData;
        
        data = select->columnBlob(0);
        
        imageData = SharedBuffer::create(data.data(), data.size());
        
        image = BitmapImage::create();                                                            
        image->setData(WTFMove(imageData), true);
    }
        
    return image;
}

double TopSitesManager::lastAccessed(URL &url)
{
    double lastAccessed = 0;
    
    auto select = m_topSitesDB.prepareStatement("SELECT lastAccessed FROM topsites WHERE url=?1;"_s);

    if(!select)
        return lastAccessed;

    select->bindText(1, url.string());
        
    if(select->step() == SQLITE_ROW)
    {
        lastAccessed = select->columnDouble(0);
    }    
    
    return lastAccessed;
}

int TopSitesManager::visitCount(URL &url)
{
    int visitCount = 0;
    
    auto select = m_topSitesDB.prepareStatement("SELECT visitCount FROM topsites WHERE url=?1;"_s);

    if(!select)
        return visitCount;

    select->bindText(1, url.string());
        
    if(select->step() == SQLITE_ROW)
    {
        visitCount = select->columnInt(0);
    }    
    
    return visitCount;
}

bool TopSitesManager::contains(URL &url)
{
    bool found = false;
    
    auto select = m_topSitesDB.prepareStatement("SELECT count(*) FROM topsites WHERE url=?1;"_s);

    if(!select)
        return found;

    select->bindText(1, url.string());
        
    if(select->step() == SQLITE_ROW)
    {
        found = select->columnInt(0) > 0;
    }    
    
    return found;    
}

bool TopSitesManager::hasScreenshot(URL &url)
{
    bool hasScreenshot = false;
    auto select = m_topSitesDB.prepareStatement("SELECT screenshot FROM topsites WHERE url=?1;"_s);

    if(!select)
        return hasScreenshot;

    select->bindText(1, url.string());
    
    if (select->step() == SQLITE_ROW)
    {
        hasScreenshot = !select->columnText(0).isEmpty();
    }
    
    return hasScreenshot;
}

bool TopSitesManager::shouldAppear(URL &url)
{
    bool result = false;
    int minVisitCount = 0;
    
    auto select = m_topSitesDB.prepareStatement("SELECT MIN(visitCount) FROM topsites ORDER BY visitCount DESC LIMIT 0, ?1;"_s);

    if(!select)
        return result;

    select->bindInt(1, maxEntries());
        
    if(select->step() == SQLITE_ROW)
    {
        minVisitCount = select->columnInt(0);
    }    
    
    result = visitCount(url) >= minVisitCount;
    
    return result;
}

int TopSitesManager::requiredVisitCount()
{
    int minVisitCount = 0;
    
    auto select = m_topSitesDB.prepareStatement("SELECT MIN(visitCount) FROM(SELECT visitCount FROM topsites ORDER BY visitCount DESC LIMIT 0, ?1);"_s);

    if(!select)
        return minVisitCount;

    select->bindInt(1, maxEntries());
        
    if(select->step() == SQLITE_ROW)
    {
        minVisitCount = select->columnInt(0);
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
                    URL urlToRemove = URL({ }, argument);
                    remove(urlToRemove);
                }
                else if(query.startsWith("maxEntries="))
                {
                    String argument = query.substring(String("maxEntries=").length());
                    setMaxEntries(parseIntegerAllowingTrailingJunk<int>(argument).value_or(0));
                }
                else if(query.startsWith("displayMode="))
                {
                    String argument = query.substring(String("displayMode=").length());
                    setDisplayMode((displaymode_t) parseIntegerAllowingTrailingJunk<int>(argument).value_or(0));
                }
                else if(query.startsWith("filterMode="))
                {
                    String argument = query.substring(String("filterMode=").length());
                    setFilterMode((filtermode_t) parseIntegerAllowingTrailingJunk<int>(argument).value_or(0));
                }                            
                else if(query.startsWith("screenshotSize="))
                {
                    String argument = query.substring(String("screenshotSize=").length());
                    setScreenshotSize(parseIntegerAllowingTrailingJunk<int>(argument).value_or(0));
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
    auto updateStmt = m_topSitesDB.prepareStatement("UPDATE topsites SET screenshot = NULL WHERE visitCount <= ?1;"_s);

    if(!updateStmt)
    {
        return;
    }

    updateStmt->bindInt(1, requiredVisitCount() - 1); // Improve this

    if(!updateStmt->executeCommand())
        return;    
        
    double maxAge = WebPreferences::sharedStandardPreferences()->historyAgeInDaysLimit();
    double minAge = MonotonicTime::now().secondsSinceEpoch().value() - maxAge*DAY;
    auto deleteStmt = m_topSitesDB.prepareStatement("DELETE FROM topsites WHERE lastAccessed < ?1;"_s);

    if(!deleteStmt)
    {
        return;
    }

    deleteStmt->bindDouble(1, minAge);
    
    deleteStmt->executeCommand();        
}

bool TopSitesManager::addOrUpdate(WebView *webView, URL &url, String &title)
{    
    double timestamp = MonotonicTime::now().secondsSinceEpoch().value();
    int visitCount = 1;
    double lastAccessed = 0;
    Vector<uint8_t> screenshot;
    bool found = false;

    //kprintf("addOrUpdate <%s>\n", url.string().utf8().data());

    auto select = m_topSitesDB.prepareStatement("SELECT visitCount, screenshot, lastAccessed FROM topsites WHERE url=?1;"_s);
    if(!select)
        return false;

    select->bindText(1, url.string());
        
    if(select->step() == SQLITE_ROW)
    {
        found = true;
        visitCount = select->columnInt(0) + 1;
        screenshot = select->columnBlob(1);
        lastAccessed = select->columnDouble(2);        
    }    

    //kprintf("visitCount %d required : %d screenshot %d timestamp %f lastaccessed %d\n", visitCount, requiredVisitCount(), screenshot.size(), timestamp, lastAccessed);
    bool generateScreenshot = !m_disableScreenShots && (visitCount >= requiredVisitCount()) && (screenshot.size() == 0 || (timestamp >= lastAccessed + SCREENSHOT_UPDATE_DELAY));

    if(found)
    {    
        auto updateStmt = m_topSitesDB.prepareStatement("UPDATE topsites SET title=?1, visitCount=?2, lastAccessed=?3 WHERE url=?4;"_s);

        if(!updateStmt)
        {
            return false;
        }

        updateStmt->bindText(1, title);
        updateStmt->bindInt(2, visitCount);
        updateStmt->bindDouble(3, timestamp);
        updateStmt->bindText(4, url.string());

        if(!updateStmt->executeCommand())
            return false;                
    }    
    else
    {        
        auto insertStmt = m_topSitesDB.prepareStatement("INSERT INTO topsites (url, title, screenshot, visitCount, lastAccessed) VALUES (?1, ?2, ?3, ?4, ?5);"_s);

        if(!insertStmt)
            return false;

        insertStmt->bindText(1, url.string());
        insertStmt->bindText(2, title);    
        insertStmt->bindNull(3);    
        insertStmt->bindInt(4, visitCount);        
        insertStmt->bindDouble(5, timestamp);

        if(!insertStmt->executeCommand())
            return false;            
    }    
    
    if(generateScreenshot)
    {
        int width = m_screenshotSize;
        int height;
        Vector<uint8_t> imageData;

        //kprintf("Generate screenshot for <%s>\n", url.string().utf8().data());
        webView->screenshot(width, height, &imageData);
        
        auto updateScreenShotStmt = m_topSitesDB.prepareStatement("UPDATE topsites SET screenshot=?1 WHERE url=?2;"_s);

        if(!updateScreenShotStmt)
        {
            return false;
        }

        updateScreenShotStmt->bindBlob(1, imageData);
        updateScreenShotStmt->bindText(2, url.string());        

        if(!updateScreenShotStmt->executeCommand())
            return false;
    }        
    
    return true;
}

void TopSitesManager::remove(URL &url)
{
    auto deleteStmt = m_topSitesDB.prepareStatement("DELETE FROM topsites WHERE url=?1;"_s);

    if(!deleteStmt)
    {
        return;
    }

    deleteStmt->bindText(1, url.string());
    
    deleteStmt->executeCommand();
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

        auto select = m_topSitesDB.prepareStatement("SELECT url, title, screenshot, lastAccessed, visitCount FROM topsites ORDER by visitCount DESC, lastAccessed DESC;"_s);

        if(!select)
            return;
            
        while(select->step() == SQLITE_ROW && countEntries < maxEntries())
        {
            bool showEntry = false;
            String url = select->columnText(0);
                        
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
                Vector<uint8_t> data;
                String base64Data, closeBase64Data;
                String title = select->columnText(1);    
                data = select->columnBlob(2);
                
                if(data.size() == 0 && m_placeholderImage && m_placeholderImage->data())
                {
                    base64Data = base64EncodeToString(m_placeholderImage->data(), m_placeholderImage->size());
                }
                else
                {
                    base64Data = base64EncodeToString(data);
                }
                
                if(m_closeImage && m_closeImage->data())
                {
                    closeBase64Data = base64EncodeToString(m_closeImage->data(), m_closeImage->size());
                }

                String screenshotURL =  "data:image/png;base64," + base64Data;
                String removeURL = "data:image/png;base64," + closeBase64Data;
                //double timestamp = select.getColumnDouble(3);
                int visits = select->columnInt(4);

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
