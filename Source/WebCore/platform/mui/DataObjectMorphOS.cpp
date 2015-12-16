/*
 * Copyright (C) 2009, Martin Robinson
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DataObjectMorphOS.h"

#include "markup.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static void replaceNonBreakingSpaceWithSpace(String& str)
{
    static const UChar NonBreakingSpaceCharacter = 0xA0;
    static const UChar SpaceCharacter = ' ';
    str.replace(NonBreakingSpaceCharacter, SpaceCharacter);
}

String DataObjectMorphOS::text() const
{
    if (m_range) {
        String text = m_range->text();
        replaceNonBreakingSpaceWithSpace(text);
        return text;
    }
    return m_text;
}

String DataObjectMorphOS::markup() const
{
    if (m_range)
        return createMarkup(*m_range, 0, AnnotateForInterchange, false, ResolveNonLocalURLs);
    return m_markup;
}

void DataObjectMorphOS::setText(const String& newText)
{
    m_range = nullptr;
    m_text = newText;
    replaceNonBreakingSpaceWithSpace(m_text);
}

void DataObjectMorphOS::setMarkup(const String& newMarkup)
{
    m_range = nullptr;
    m_markup = newMarkup;
}

void DataObjectMorphOS::setURIList(const String& uriListString)
{
    m_uriList = uriListString;

    // This code is originally from: platform/chromium/ChromiumDataObject.cpp.
    // FIXME: We should make this code cross-platform eventually.

    // Line separator is \r\n per RFC 2483 - however, for compatibility
    // reasons we also allow just \n here.
    Vector<String> uriList;
    uriListString.split('\n', uriList);

    // Process the input and copy the first valid URL into the url member.
    // In case no URLs can be found, subsequent calls to getData("URL")
    // will get an empty string. This is in line with the HTML5 spec (see
    // "The DragEvent and DataTransfer interfaces"). Also extract all filenames
    // from the URI list.
    bool setURL = false;
    for (size_t i = 0; i < uriList.size(); ++i) {
        String& line = uriList[i];
        line = line.stripWhiteSpace();
        if (line.isEmpty())
            continue;
        if (line[0] == '#')
            continue;

        URL url = URL(URL(), line);
        if (url.isValid()) {
            if (!setURL) {
                m_url = url;
                setURL = true;
            }

			m_filenames.append(line);
        }
    }
}

void DataObjectMorphOS::setURL(const URL& url, const String& label)
{
    m_url = url;
    m_uriList = url;
    setText(url.string());

    String actualLabel(label);
    if (actualLabel.isEmpty())
        actualLabel = url;

    StringBuilder markup;
    markup.append("<a href=\"");
    markup.append(url.string());
    markup.append("\">");
	markup.append(actualLabel);
    markup.append("</a>");
    setMarkup(markup.toString());
}

void DataObjectMorphOS::clearText()
{
    m_range = nullptr;
    m_text = "";
}

void DataObjectMorphOS::clearMarkup()
{
    m_range = nullptr;
    m_markup = "";
}

String DataObjectMorphOS::urlLabel() const
{
    if (hasText())
        return text();

    if (hasURL())
        return url();

    return String();
}

void DataObjectMorphOS::clearAllExceptFilenames()
{
    m_text = "";
    m_markup = "";
    m_uriList = "";
    m_url = URL();
    m_image = nullptr;
    m_range = nullptr;
}

void DataObjectMorphOS::clearAll()
{
    clearAllExceptFilenames();
    m_filenames.clear();
}

DataObjectMorphOS* DataObjectMorphOS::forClipboard(int clipboard)
{
	static HashMap<int, RefPtr<DataObjectMorphOS> > objectMap;

    if (!objectMap.contains(clipboard)) {
		RefPtr<DataObjectMorphOS> dataObject = DataObjectMorphOS::create();
        objectMap.set(clipboard, dataObject);
        return dataObject.get();
    }

	HashMap<int, RefPtr<DataObjectMorphOS> >::iterator it = objectMap.find(clipboard);
    return it->value.get();
}

}
