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

#ifndef TopSitesManager_h
#define TopSitesManager_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#include "WebKitTypes.h"
#include "SQLiteDatabase.h"

namespace WTF {
class String;
}

class WebView;

namespace WebCore {

class URL;
class Image;
class SharedBuffer;

class TopSitesManager {
public:
	typedef enum { SHOW_ALL, SHOW_BOOKMARKS, SHOW_ALL_BUT_BOOKMARKS, SHOW_CUSTOM } filtermode_t;
	typedef enum { TEMPLATE_GRID, TEMPLATE_3D } displaymode_t;

	static TopSitesManager& getInstance();

	TopSitesManager();   
    virtual ~TopSitesManager();
    
	void generateTemplate(WebView *webView, WTF::String originurl);
	void update(WebView *webView, URL &url, WTF::String &title);
	void setDisplayMode(displaymode_t mode);
	void setFilterMode(filtermode_t mode);
	void setMaxEntries(int maxEntries);
	void setScreenshotSize(int width);

protected:	
	int maxEntries() { return m_maxEntries; }	
	int entries();
	bool contains(URL &url);
	bool hasScreenshot(URL &url);
	bool shouldAppear(URL &url);
	int requiredVisitCount();

	// Attributes
	WTF::String title(URL &url);
	WTF::PassRefPtr<Image> screenshot(URL &url);
	int visitCount(URL &url);
	double lastAccessed(URL &url);
	
	// Modifications
	bool shouldAdd(URL &url);
	bool addOrUpdate(WebView *webView, URL &url, WTF::String &title);
	void remove(URL &url);
	void pruneOlderEntries();

	int	m_maxEntries;
	filtermode_t m_filterMode;	
	displaymode_t m_displayMode;
	int m_screenshotSize;
	WTF::RefPtr<SharedBuffer> m_placeholderImage;
	WTF::RefPtr<SharedBuffer> m_closeImage;
	
};

} // WebCore

#endif // TopSitesManager_h
