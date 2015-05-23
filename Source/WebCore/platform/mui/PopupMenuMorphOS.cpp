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
#include "PopupMenuMorphOS.h"
#include "TextRun.h"

#undef accept

#include "FrameView.h"
#include "HostWindow.h"
#include "ObserverServiceData.h"
#include <wtf/text/WTFString.h>
#include <wtf/text/CString.h>

namespace WebCore {

const int popupWindowBorderWidth = 1;

PopupMenuMorphOS::PopupMenuMorphOS(PopupMenuClient* client)
    : m_popupClient(client)
{
}

PopupMenuMorphOS::~PopupMenuMorphOS()
{
}

void PopupMenuMorphOS::show(const IntRect& r, FrameView* v, int index)
{
    ASSERT(client());
    UNUSED_PARAM(index);

    RefPtr<PopupMenuMorphOS> protector(this);

    calculatePositionAndSize(r, v);
    if (clientRect().isEmpty())
        return;

    m_frameView = v;

    WebCore::ObserverServiceData::createObserverService()->notifyObserver("PopupMenuShow", "", this);
}

void PopupMenuMorphOS::hide()
{
    WebCore::ObserverServiceData::createObserverService()->notifyObserver("PopupMenuHide", "", this);
}

void PopupMenuMorphOS::updateFromElement()
{
    client()->setTextFromItem(client()->selectedIndex());
}

void PopupMenuMorphOS::disconnectClient()
{
    m_popupClient = 0;
}

void PopupMenuMorphOS::calculatePositionAndSize(const IntRect& r, FrameView* v)
{
    IntRect rScreenCoords(v->contentsToWindow(r.location()), r.size());
    rScreenCoords.setY(rScreenCoords.y() + rScreenCoords.height());

    m_itemHeight = rScreenCoords.height();

    int itemCount = client()->listSize();
    int naturalHeight = m_itemHeight * itemCount;

    int popupWidth = 0;
    for (int i = 0; i < itemCount; ++i) {
        String text = client()->itemText(i);
        if (text.isEmpty())
            continue;

        FontCascade itemFont = client()->menuStyle().font();
        if (client()->itemIsLabel(i)) {
            FontDescription d = itemFont.fontDescription();
            d.setWeight(d.bolderWeight());
            itemFont = FontCascade(d, itemFont.letterSpacing(), itemFont.wordSpacing());
            itemFont.update(m_popupClient->fontSelector());
        }

        popupWidth = std::max(popupWidth, (int) itemFont.width(TextRun(text)));
    }

    rScreenCoords.setHeight(naturalHeight);
    rScreenCoords.setWidth(popupWidth + 10);

    m_windowRect = rScreenCoords;
}

IntRect PopupMenuMorphOS::clientRect() const
{
    IntRect clientRect = m_windowRect;
    clientRect.inflate(-popupWindowBorderWidth);
    clientRect.setLocation(IntPoint(0, 0));
    return clientRect;
}

}
